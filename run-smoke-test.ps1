$ErrorActionPreference = 'Stop'
$javaHome = 'C:\Program Files\Microsoft\jdk-17.0.8.7-hotspot'
$backendJar = Join-Path $PSScriptRoot 'backend\target\backend-0.0.1-SNAPSHOT.jar'
$gateway = Join-Path $PSScriptRoot 'cpp\build\gateway.exe'

if (-not (Test-Path -LiteralPath $backendJar)) { throw 'Build backend before running the smoke test.' }
if (-not (Test-Path -LiteralPath $gateway)) { throw 'Build gateway before running the smoke test.' }

& (Join-Path $PSScriptRoot 'start-infrastructure.ps1')
$backend = Start-Process -FilePath (Join-Path $javaHome 'bin\java.exe') -ArgumentList @('-jar', $backendJar) -WorkingDirectory (Join-Path $PSScriptRoot 'backend') -PassThru
$gatewayProcess = Start-Process -FilePath $gateway -WorkingDirectory (Join-Path $PSScriptRoot 'cpp\build') -PassThru

try {
    Start-Sleep -Seconds 8
    python (Join-Path $PSScriptRoot 'tools\smoke_test.py')
} finally {
    Stop-Process -Id $backend.Id, $gatewayProcess.Id -Force -ErrorAction SilentlyContinue
}
