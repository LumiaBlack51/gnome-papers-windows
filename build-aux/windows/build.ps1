param(
    [string]$MsysRoot = 'E:\BuildTools\msys64',
    [string]$Version = '51.beta.1-win-save.1'
)
$ErrorActionPreference = 'Stop'
$projectRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$bash = Join-Path $MsysRoot 'usr\bin\bash.exe'
if (!(Test-Path $bash)) { throw "MSYS2 not found at $MsysRoot" }
$env:MSYSTEM = 'UCRT64'
$env:CHERE_INVOKING = '1'
Push-Location $projectRoot
try {
    git submodule update --init --checkout test-data
    if ($LASTEXITCODE) { throw 'Test fixture checkout failed' }
    & $bash -lc 'export GETTEXTDATADIR=/ucrt64/share/gettext; meson setup build --buildtype=release --strip -Ddocumentation=false -Duser_doc=false -Dnautilus=false -Dkeyring=disabled -Dintrospection=disabled -Dpreviewer=false -Dthumbnailer=false -Dspell_check=disabled && meson compile -C build'
    if ($LASTEXITCODE) { throw 'Compilation failed' }
    & $bash -lc 'export PATH="$PWD/build/libdocument:$PWD/build/libview:$PATH"; meson test -C build pps-jobs-test --print-errorlogs'
    if ($LASTEXITCODE) { throw 'Save regression tests failed' }
    & (Join-Path $PSScriptRoot 'bundle.ps1') -MsysRoot $MsysRoot
    & (Join-Path $MsysRoot 'ucrt64\bin\makensis.exe') "-DAPP_VERSION=$Version" (Join-Path $PSScriptRoot 'papers.nsi')
    if ($LASTEXITCODE) { throw 'Installer creation failed' }
} finally {
    Pop-Location
}
