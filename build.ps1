[CmdletBinding()]
param(
    [string]$Configuration = "Release"
)

$ErrorActionPreference = "Stop"

$rootDir = $PSScriptRoot
Set-Location $rootDir

Write-Host "=== Building NewPilot Native C++ Executable ===" -ForegroundColor Cyan
cmake -B build -S . -G "Visual Studio 17 2022" -A x64
cmake --build build --config $Configuration

$exePath = Join-Path $rootDir "build\$Configuration\NewPilot.exe"
if (-not (Test-Path $exePath)) {
    throw "Executable not found at $exePath"
}

$exeSizeKB = [math]::Round(((Get-Item $exePath).Length / 1KB), 2)
Write-Host "Compiled binary size: $exeSizeKB KB" -ForegroundColor Green

Write-Host "`n=== Preparing Packaging Staging Directory ===" -ForegroundColor Cyan
$stagingDir = Join-Path $rootDir "build\staging"
$outDir = Join-Path $rootDir "build\out"

if (Test-Path $stagingDir) { Remove-Item $stagingDir -Recurse -Force }
if (Test-Path $outDir) { Remove-Item $outDir -Recurse -Force }

New-Item -ItemType Directory -Force -Path $stagingDir | Out-Null
New-Item -ItemType Directory -Force -Path $outDir | Out-Null

Copy-Item $exePath -Destination (Join-Path $stagingDir "NewPilot.exe")
Copy-Item (Join-Path $rootDir "packaging\AppxManifest.xml") -Destination (Join-Path $stagingDir "AppxManifest.xml")
Copy-Item (Join-Path $rootDir "packaging\resources\Images") -Destination (Join-Path $stagingDir "Images") -Recurse

# Find SDK tools dynamically
$sdkBin = "C:\Program Files (x86)\Windows Kits\10\bin\10.0.26100.0\x64"
$makeappx = Join-Path $sdkBin "makeappx.exe"
$signtool = Join-Path $sdkBin "signtool.exe"
$makepri = Join-Path $sdkBin "makepri.exe"

if (-not (Test-Path $makeappx)) {
    $makeappx = (Get-ChildItem -Path "C:\Program Files (x86)\Windows Kits\10\bin" -Filter "makeappx.exe" -Recurse | Where-Object { $_.FullName -like "*x64*" } | Select-Object -First 1).FullName
}
if (-not (Test-Path $signtool)) {
    $signtool = (Get-ChildItem -Path "C:\Program Files (x86)\Windows Kits\10\bin" -Filter "signtool.exe" -Recurse | Where-Object { $_.FullName -like "*x64*" } | Select-Object -First 1).FullName
}
if (-not (Test-Path $makepri)) {
    $makepri = (Get-ChildItem -Path "C:\Program Files (x86)\Windows Kits\10\bin" -Filter "makepri.exe" -Recurse | Where-Object { $_.FullName -like "*x64*" } | Select-Object -First 1).FullName
}

Write-Host "`n=== Indexing Resources with makepri ===" -ForegroundColor Cyan
$priConfig = Join-Path $stagingDir "priconfig.xml"
$priOut = Join-Path $stagingDir "resources.pri"
& $makepri createconfig /cf $priConfig /dq en-US /pv 10.0.0 /o | Out-Null
& $makepri new /pr $stagingDir /cf $priConfig /of $priOut /o | Out-Null

Write-Host "`n=== Packaging MSIX Package with makeappx ===" -ForegroundColor Cyan
$msixPath = Join-Path $outDir "NewPilot.msix"
& $makeappx pack /d $stagingDir /p $msixPath /o

if (-not (Test-Path $msixPath)) {
    throw "Failed to create MSIX package at $msixPath"
}

$msixSizeKB = [math]::Round(((Get-Item $msixPath).Length / 1KB), 2)
Write-Host "Created MSIX Package: $msixPath ($msixSizeKB KB)" -ForegroundColor Green

Write-Host "`n=== Managing Developer Certificate ===" -ForegroundColor Cyan
$certSubject = "CN=NewPilot Development"
$cert = Get-ChildItem Cert:\CurrentUser\My | Where-Object { $_.Subject -eq $certSubject } | Select-Object -First 1

if (-not $cert) {
    Write-Host "Creating developer self-signed certificate '$certSubject'..." -ForegroundColor Yellow
    $cert = New-SelfSignedCertificate -Type Custom -Subject $certSubject -KeyUsage DigitalSignature -FriendlyName "NewPilot Development Certificate" -CertStoreLocation Cert:\CurrentUser\My -TextExtension @("2.5.29.37={text}1.3.6.1.5.5.7.3.3", "2.5.29.19={text}")
}

# Export .cer file
$cerPath = Join-Path $outDir "NewPilotDevCert.cer"
Export-Certificate -Cert $cert -FilePath $cerPath -Force | Out-Null
Write-Host "Exported developer certificate to: $cerPath" -ForegroundColor Green

# Trust cert locally in CurrentUser Root and TrustedPeople
Import-Certificate -FilePath $cerPath -CertStoreLocation Cert:\CurrentUser\Root | Out-Null
Import-Certificate -FilePath $cerPath -CertStoreLocation Cert:\CurrentUser\TrustedPeople | Out-Null

Write-Host "`n=== Signing MSIX Package with signtool ===" -ForegroundColor Cyan
& $signtool sign /fd SHA256 /sha1 $cert.Thumbprint $msixPath

Write-Host "`n=======================================================" -ForegroundColor Green
Write-Host "SUCCESS! NewPilot MSIX package is built and signed!" -ForegroundColor Green
Write-Host "Package path: $msixPath ($msixSizeKB KB)" -ForegroundColor Green
Write-Host "Certificate path: $cerPath" -ForegroundColor Green
Write-Host "Executable size: $exePath ($exeSizeKB KB)" -ForegroundColor Green
Write-Host "=======================================================" -ForegroundColor Green
