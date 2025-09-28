$ErrorActionPreference = "Stop"

try {
    Write-Host "Configuring project..."
    cmake -B build -S . -G "MinGW Makefiles"

    Write-Host "Building project..."
    cmake --build build --config Release

    Write-Host "Build completed successfully."
} catch {
    Write-Host "An error occurred:" -ForegroundColor Red
    Write-Host $_.Exception.Message -ForegroundColor Red
    exit 1
}
