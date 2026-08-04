[CmdletBinding()]
param(
    [string]$Configuration = "Release",
    [string]$StoreIdentityName = "GamerJagdish.NewPilot",
    [string]$StorePublisher = "CN=CAC481E5-AF67-48DF-8DF8-A641563FA629",
    [string]$StorePublisherDisplayName = "GamerJagdish"
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

# -------------------------------------------------------------
# 1. BUILD MICROSOFT STORE PACKAGE (Unsigned for Partner Center)
# -------------------------------------------------------------
Write-Host "`n=== Packaging Microsoft Store Package (Unsigned for Partner Center) ===" -ForegroundColor Cyan
$rawManifest = Get-Content (Join-Path $rootDir "packaging\AppxManifest.xml") -Raw

# Stamp Store Identity Credentials
$storeManifest = $rawManifest -replace 'Name="NewPilot"', "Name=`"$StoreIdentityName`""
$storeManifest = $storeManifest -replace 'Publisher="CN=GamerJagdish"', "Publisher=`"$StorePublisher`""
$storeManifest = $storeManifest -replace '<PublisherDisplayName>GamerJagdish</PublisherDisplayName>', "<PublisherDisplayName>$StorePublisherDisplayName</PublisherDisplayName>"

Set-Content -Path (Join-Path $stagingDir "AppxManifest.xml") -Value $storeManifest -Encoding UTF8

$priConfig = Join-Path $stagingDir "priconfig.xml"
$priOut = Join-Path $stagingDir "resources.pri"
& $makepri createconfig /cf $priConfig /dq en-US /pv 10.0.0 /o | Out-Null
& $makepri new /pr $stagingDir /cf $priConfig /of $priOut /o | Out-Null

$storeMsixPath = Join-Path $outDir "NewPilot.Store.msix"
& $makeappx pack /d $stagingDir /p $storeMsixPath /o
$storeMsixSizeKB = [math]::Round(((Get-Item $storeMsixPath).Length / 1KB), 2)
Write-Host "Created Store Package: $storeMsixPath ($storeMsixSizeKB KB)" -ForegroundColor Green

# -------------------------------------------------------------
# 2. BUILD LOCAL SIDELOAD PACKAGE (Signed for local testing)
# -------------------------------------------------------------
Write-Host "`n=== Packaging Local Sideload MSIX Package ===" -ForegroundColor Cyan
Set-Content -Path (Join-Path $stagingDir "AppxManifest.xml") -Value $rawManifest -Encoding UTF8
& $makepri new /pr $stagingDir /cf $priConfig /of $priOut /o | Out-Null

$msixPath = Join-Path $outDir "NewPilot.msix"
& $makeappx pack /d $stagingDir /p $msixPath /o
$msixSizeKB = [math]::Round(((Get-Item $msixPath).Length / 1KB), 2)

Write-Host "`n=== Managing Developer Certificate (GamerJagdish) ===" -ForegroundColor Cyan
$certSubject = "CN=GamerJagdish"
$cert = Get-ChildItem Cert:\CurrentUser\My | Where-Object { $_.Subject -eq $certSubject } | Select-Object -First 1

if (-not $cert) {
    Write-Host "Creating developer self-signed certificate '$certSubject'..." -ForegroundColor Yellow
    $cert = New-SelfSignedCertificate -Type Custom -Subject $certSubject -KeyUsage DigitalSignature -FriendlyName "GamerJagdish Development Certificate" -CertStoreLocation Cert:\CurrentUser\My -TextExtension @("2.5.29.37={text}1.3.6.1.5.5.7.3.3", "2.5.29.19={text}")
}

# Export .cer file
$cerPath = Join-Path $outDir "NewPilotDevCert.cer"
Export-Certificate -Cert $cert -FilePath $cerPath -Force | Out-Null
Write-Host "Exported developer certificate to: $cerPath" -ForegroundColor Green

# Trust cert locally in CurrentUser Root and TrustedPeople
Import-Certificate -FilePath $cerPath -CertStoreLocation Cert:\CurrentUser\Root | Out-Null
Import-Certificate -FilePath $cerPath -CertStoreLocation Cert:\CurrentUser\TrustedPeople | Out-Null

Write-Host "`n=== Signing Local Sideload Package ===" -ForegroundColor Cyan
& $signtool sign /fd SHA256 /sha1 $cert.Thumbprint $msixPath

Write-Host "`n=======================================================" -ForegroundColor Green
Write-Host "SUCCESS! Packages built and ready!" -ForegroundColor Green
Write-Host "Store Package (Upload to Partner Center): $storeMsixPath ($storeMsixSizeKB KB)" -ForegroundColor Green
Write-Host "Sideload Package (Local testing): $msixPath ($msixSizeKB KB)" -ForegroundColor Green
Write-Host "Executable Size: $exePath ($exeSizeKB KB)" -ForegroundColor Green
Write-Host "=======================================================" -ForegroundColor Green
