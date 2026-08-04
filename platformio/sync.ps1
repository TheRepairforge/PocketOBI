# Regenerate the PlatformIO mirror from the canonical Arduino sources at the repo
# root. Run after editing ../PocketOBI.ino or ../OneWire2.* so the PlatformIO
# build stays identical. (../PocketOBI.ino is the single source of truth.)
$ErrorActionPreference = "Stop"
$root = Join-Path $PSScriptRoot ".."
New-Item -ItemType Directory -Force -Path (Join-Path $PSScriptRoot "src") | Out-Null
New-Item -ItemType Directory -Force -Path (Join-Path $PSScriptRoot "lib\OneWire2\util") | Out-Null
Copy-Item (Join-Path $root "PocketOBI.ino") (Join-Path $PSScriptRoot "src\PocketOBI.ino") -Force
Copy-Item (Join-Path $root "OneWire2.h")    (Join-Path $PSScriptRoot "lib\OneWire2\") -Force
Copy-Item (Join-Path $root "OneWire2.cpp")  (Join-Path $PSScriptRoot "lib\OneWire2\") -Force
Copy-Item (Join-Path $root "util\*")        (Join-Path $PSScriptRoot "lib\OneWire2\util\") -Force
Write-Host "PlatformIO mirror synced from the root sources."
