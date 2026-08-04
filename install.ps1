[CmdletBinding()]
param()

$ErrorActionPreference = "Stop"
$rootDir = $PSScriptRoot
Set-Location $rootDir

$msixPath = Join-Path $rootDir "build\out\NewPilot.msix"
$cerPath = Join-Path $rootDir "build\out\NewPilotDevCert.cer"

if (-not (Test-Path $msixPath)) {
    Write-Host "Package not found. Running build.ps1 first..." -ForegroundColor Yellow
    & (Join-Path $rootDir "build.ps1")
}

Write-Host "=== Installing Developer Certificate ===" -ForegroundColor Cyan

if (Test-Path $cerPath) {
    Import-Certificate -FilePath $cerPath -CertStoreLocation Cert:\CurrentUser\Root | Out-Null
    Import-Certificate -FilePath $cerPath -CertStoreLocation Cert:\CurrentUser\TrustedPeople | Out-Null

    $isAdmin = ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
    if ($isAdmin) {
        Import-Certificate -FilePath $cerPath -CertStoreLocation Cert:\LocalMachine\Root | Out-Null
        Import-Certificate -FilePath $cerPath -CertStoreLocation Cert:\LocalMachine\TrustedPeople | Out-Null
        Write-Host "Certificate imported to LocalMachine Root & TrustedPeople." -ForegroundColor Green
    } else {
        Write-Host "Certificate imported to CurrentUser Root & TrustedPeople." -ForegroundColor Green
    }
}

Write-Host "`n=== Installing NewPilot MSIX Package ===" -ForegroundColor Cyan
Add-AppxPackage -Path $msixPath -ForceApplicationShutdown

Write-Host "`n=======================================================" -ForegroundColor Green
Write-Host "SUCCESS! NewPilot installed successfully!" -ForegroundColor Green
Write-Host "You can now select NewPilot in Windows 11 Settings ->" -ForegroundColor Green
Write-Host "Bluetooth & devices -> Keyboard -> Customize Copilot key on keyboard" -ForegroundColor Green
Write-Host "=======================================================" -ForegroundColor Green
