$certSubject = "CN=GamerJagdish"
$cert = Get-ChildItem Cert:\CurrentUser\My | Where-Object { $_.Subject -eq $certSubject } | Select-Object -First 1

if (-not $cert) {
    Write-Host "Creating developer certificate for GamerJagdish..."
    $cert = New-SelfSignedCertificate -Type Custom -Subject $certSubject -KeyUsage DigitalSignature -FriendlyName "GamerJagdish Development Certificate" -CertStoreLocation Cert:\CurrentUser\My -TextExtension @("2.5.29.37={text}1.3.6.1.5.5.7.3.3", "2.5.29.19={text}")
}

$signtool = "C:\Program Files (x86)\Windows Kits\10\bin\10.0.26100.0\x64\signtool.exe"
$msixPath = "d:\code\NewPilot\build\out\NewPilot.msix"
$cerPath = "d:\code\NewPilot\build\out\NewPilotDevCert.cer"

Export-Certificate -Cert $cert -FilePath $cerPath -Force | Out-Null

Write-Host "Signing $msixPath with thumbprint $($cert.Thumbprint)..."
& $signtool sign /fd SHA256 /sha1 $cert.Thumbprint $msixPath

Write-Host "Signing completed successfully!"
