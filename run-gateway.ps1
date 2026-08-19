$gateway = Join-Path $PSScriptRoot 'cpp\build\gateway.exe'

if (-not (Test-Path -LiteralPath $gateway)) {
    throw "Gateway executable not found. Build the cpp CMake target first."
}

& $gateway
