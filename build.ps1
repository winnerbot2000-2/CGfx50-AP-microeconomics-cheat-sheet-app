param(
    [switch]$SkipGenerate,
    [switch]$SkipValidate
)

$ErrorActionPreference = "Stop"

if (-not $SkipGenerate) {
    python tools\extract_apmicro_content.py
    python tools\generate_icons.py
}

if (-not $SkipValidate) {
    python tools\validate_content.py
    python tools\run_smoke_tests.py
}

if (-not (Get-Command fxsdk -ErrorAction SilentlyContinue)) {
    throw "Missing dependency: fxsdk. Install fxSDK/gint and rerun `fxsdk build-cg`."
}

fxsdk build-cg
