# Trust PilotControl developer certificate on local machine
$rootDir = Split-Path -Path $PSScriptRoot -Parent
$cerPath = Join-Path $rootDir "build\out\PilotControlDevCert.cer"

if (-not (Test-Path $cerPath)) {
    Write-Host "Certificate file not found at $cerPath" -ForegroundColor Red
    exit 1
}

Write-Host "Importing developer certificate into LocalMachine TrustedPeople & Root stores..." -ForegroundColor Cyan

try {
    Import-Certificate -FilePath $cerPath -CertStoreLocation Cert:\LocalMachine\TrustedPeople | Out-Null
    Import-Certificate -FilePath $cerPath -CertStoreLocation Cert:\LocalMachine\Root | Out-Null
    Write-Host "Successfully imported certificate into Local Machine stores!" -ForegroundColor Green
} catch {
    Write-Host "Importing to LocalMachine requires Administrator privileges." -ForegroundColor Yellow
    Write-Host "Attempting import to CurrentUser stores..." -ForegroundColor Yellow
    Import-Certificate -FilePath $cerPath -CertStoreLocation Cert:\CurrentUser\TrustedPeople | Out-Null
    Import-Certificate -FilePath $cerPath -CertStoreLocation Cert:\CurrentUser\Root | Out-Null
    Write-Host "Imported to CurrentUser stores." -ForegroundColor Green
}
