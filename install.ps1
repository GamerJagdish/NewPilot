[CmdletBinding()]
param()

$ErrorActionPreference = "Stop"

Write-Host "=======================================================" -ForegroundColor Cyan
Write-Host "            NewPilot All-in-One Installer              " -ForegroundColor Cyan
Write-Host "=======================================================" -ForegroundColor Cyan

# -------------------------------------------------------------
# Helper: Extract Package Version from MSIX Archive
# -------------------------------------------------------------
function Get-PackageVersionFromMsix {
    param([string]$path)
    try {
        Add-Type -AssemblyName "System.IO.Compression.FileSystem" -ErrorAction SilentlyContinue
        $zip = [System.IO.Compression.ZipFile]::OpenRead((Convert-Path $path))
        $entry = $zip.Entries | Where-Object { $_.FullName -eq "AppxManifest.xml" }
        if ($entry) {
            $stream = $entry.Open()
            $reader = New-Object System.IO.StreamReader($stream)
            $content = $reader.ReadToEnd()
            $reader.Close()
            $stream.Close()
            $zip.Dispose()
            $xml = [xml]$content
            return $xml.Package.Identity.Version
        }
        if ($zip) { $zip.Dispose() }
    } catch {}
    return "Unknown"
}

# -------------------------------------------------------------
# 1. Check Administrator Privileges
# -------------------------------------------------------------
$isAdmin = ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
if (-not $isAdmin) {
    Write-Host "`n[Notice] For complete certificate trust, running PowerShell as Administrator is recommended." -ForegroundColor Yellow
}

# -------------------------------------------------------------
# 2. Resolve or Download Assets
# -------------------------------------------------------------
$rootDir = $PSScriptRoot

$msixPath = ""
$cerPath = ""

if ($rootDir) {
    $candidates = @(
        (Join-Path $rootDir "build\out\NewPilot.msix"),
        (Join-Path $rootDir "NewPilot.msix")
    )
    foreach ($path in $candidates) {
        if (Test-Path $path) { $msixPath = (Get-Item $path).FullName; break }
    }

    $cerCandidates = @(
        (Join-Path $rootDir "build\out\NewPilotDevCert.cer"),
        (Join-Path $rootDir "NewPilotDevCert.cer")
    )
    foreach ($path in $cerCandidates) {
        if (Test-Path $path) { $cerPath = (Get-Item $path).FullName; break }
    }

    if ((-not $msixPath -or -not $cerPath) -and (Test-Path (Join-Path $rootDir "build.ps1"))) {
        Write-Host "`nAssets not found locally. Building NewPilot from source..." -ForegroundColor Yellow
        & (Join-Path $rootDir "build.ps1")
        $msixPath = (Get-Item (Join-Path $rootDir "build\out\NewPilot.msix")).FullName
        $cerPath = (Get-Item (Join-Path $rootDir "build\out\NewPilotDevCert.cer")).FullName
    }
}

# Web execution fallback (irm ... | iex)
if (-not $msixPath -or -not (Test-Path $msixPath)) {
    Write-Host "`nDownloading latest NewPilot package files from GitHub..." -ForegroundColor Cyan
    $tempDir = Join-Path $env:TEMP "NewPilotInstall"
    if (-not (Test-Path $tempDir)) { New-Item -ItemType Directory -Force -Path $tempDir | Out-Null }
    
    $msixUrl = "https://raw.githubusercontent.com/GamerJagdish/NewPilot/main/build/out/NewPilot.msix"
    $cerUrl = "https://raw.githubusercontent.com/GamerJagdish/NewPilot/main/build/out/NewPilotDevCert.cer"

    $msixPath = Join-Path $tempDir "NewPilot.msix"
    $cerPath = Join-Path $tempDir "NewPilotDevCert.cer"

    [System.Net.ServicePointManager]::SecurityProtocol = [System.Net.SecurityProtocolType]::Tls12
    Invoke-WebRequest -Uri $msixUrl -OutFile $msixPath -UseBasicParsing
    Invoke-WebRequest -Uri $cerUrl -OutFile $cerPath -UseBasicParsing
}

if (-not (Test-Path $msixPath)) {
    throw "Error: Could not locate or download NewPilot.msix package file."
}

$targetVersion = Get-PackageVersionFromMsix -path $msixPath

# -------------------------------------------------------------
# 3. Trust Developer Certificate
# -------------------------------------------------------------
if ($cerPath -and (Test-Path $cerPath)) {
    Write-Host "`n[1/3] Trusting Security Certificate..." -ForegroundColor Cyan
    
    Import-Certificate -FilePath $cerPath -CertStoreLocation Cert:\CurrentUser\Root | Out-Null
    Import-Certificate -FilePath $cerPath -CertStoreLocation Cert:\CurrentUser\TrustedPeople | Out-Null

    if ($isAdmin) {
        Import-Certificate -FilePath $cerPath -CertStoreLocation Cert:\LocalMachine\Root | Out-Null
        Import-Certificate -FilePath $cerPath -CertStoreLocation Cert:\LocalMachine\TrustedPeople | Out-Null
        Write-Host "Certificate trusted in LocalMachine stores." -ForegroundColor Green
    } else {
        Write-Host "Certificate trusted in CurrentUser stores." -ForegroundColor Green
    }
}

# -------------------------------------------------------------
# 4. Stop Running Instances & Detect / Remove Old Package
# -------------------------------------------------------------
Write-Host "`n[2/3] Checking previous installation..." -ForegroundColor Cyan
Stop-Process -Name NewPilot -Force -ErrorAction SilentlyContinue

$existingPkg = Get-AppxPackage -Name "NewPilot"
if ($existingPkg) {
    Write-Host "Found installed package: v$($existingPkg.Version)" -ForegroundColor Yellow
    $existingPkg | Remove-AppxPackage -ErrorAction SilentlyContinue
    Write-Host "Previous package (v$($existingPkg.Version)) uninstalled." -ForegroundColor Green
} else {
    Write-Host "No previous installation detected." -ForegroundColor Green
}

# -------------------------------------------------------------
# 5. Install NewPilot MSIX Package
# -------------------------------------------------------------
Write-Host "`n[3/3] Installing NewPilot v$targetVersion..." -ForegroundColor Cyan
Add-AppxPackage -Path $msixPath -ForceApplicationShutdown
Write-Host "NewPilot v$targetVersion installed successfully!" -ForegroundColor Green

Write-Host "`n=======================================================" -ForegroundColor Green
Write-Host "   SUCCESS! NewPilot v$targetVersion is installed!      " -ForegroundColor Green
Write-Host "=======================================================" -ForegroundColor Green

$pkg = Get-AppxPackage -Name "NewPilot"
if ($pkg) {
    Write-Host "`nOpening NewPilot Settings window..." -ForegroundColor Cyan
    Start-Sleep -Seconds 1
    $aumid = "shell:AppsFolder\" + $pkg.PackageFamilyName + "!Settings"
    Start-Process "explorer.exe" -ArgumentList $aumid -ErrorAction SilentlyContinue
}
