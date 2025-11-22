# build-release.ps1
$ErrorActionPreference = "Stop"

# Ensure release folder exists
if (!(Test-Path "release")) {
    New-Item -ItemType Directory -Path "release" | Out-Null
}

Write-Host "Compiling repo-context-packager (release build)..."

g++ -std=c++17 -O2 -Wall -Wextra `
  src/Compressor.cpp `
  src/FileReader.cpp `
  src/GitInfoCollector.cpp `
  src/OutputFormatter.cpp `
  src/RepositoryScanner.cpp `
  src/cli.cpp `
  src/main.cpp `
  -o release/repo-context-packager.exe

Write-Host "Build completed: release\repo-context-packager.exe"
