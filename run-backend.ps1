$javaHome = if ($env:JAVA_HOME) { $env:JAVA_HOME } else { 'C:\Program Files\Microsoft\jdk-17.0.8.7-hotspot' }
$jar = Join-Path $PSScriptRoot 'backend\target\backend-0.0.1-SNAPSHOT.jar'

if (-not (Test-Path -LiteralPath $jar)) {
    throw "Backend jar not found. Run Maven package in backend first."
}

& (Join-Path $javaHome 'bin\java.exe') -jar $jar
