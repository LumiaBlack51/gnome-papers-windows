param([Parameter(Mandatory = $true)][string]$KeepVersion)
$ErrorActionPreference = 'Stop'
$projectRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$rootPrefix = $projectRoot.TrimEnd('\') + '\'
$keepInstaller = "papers-$KeepVersion-installer-x64.exe"
if (!(Test-Path -LiteralPath (Join-Path $projectRoot $keepInstaller)) -or
    !(Test-Path -LiteralPath (Join-Path $projectRoot "$keepInstaller.sha256"))) {
    throw 'The release installer and checksum must exist before cleanup.'
}
$expected = (Get-Content -LiteralPath (Join-Path $projectRoot "$keepInstaller.sha256") -Raw).Trim().Split(' ')[0]
$algorithm = [Security.Cryptography.SHA256]::Create()
$stream = [IO.File]::OpenRead((Join-Path $projectRoot $keepInstaller))
try {
    $actual = [BitConverter]::ToString($algorithm.ComputeHash($stream)).Replace('-', '')
} finally {
    $stream.Dispose()
    $algorithm.Dispose()
}
if ($expected -notmatch '^[0-9a-fA-F]{64}$' -or $actual -ne $expected) {
    throw 'The release installer does not match its SHA-256 checksum.'
}
$targets = @('build', '_build', 'dist', 'build-installer', 'target', 'cargo-home', '.cache', 'untracked', 'logs') |
    ForEach-Object { Join-Path $projectRoot $_ }
$targets += @(Get-ChildItem -LiteralPath $projectRoot -File |
    Where-Object { $_.Name -like 'papers-*-installer-x64.exe*' -and
        $_.Name -ne $keepInstaller -and $_.Name -ne "$keepInstaller.sha256" } |
    ForEach-Object { $_.FullName })
# Validate every absolute target before deleting any of them. Refuse junctions.
foreach ($target in $targets) {
    $absolute = [IO.Path]::GetFullPath($target)
    if (!$absolute.StartsWith($rootPrefix, [StringComparison]::OrdinalIgnoreCase)) {
        throw "Cleanup target is outside the checkout: $absolute"
    }
    if (Test-Path -LiteralPath $absolute) {
        $item = Get-Item -LiteralPath $absolute -Force
        if ($item.Attributes -band [IO.FileAttributes]::ReparsePoint) {
            throw "Refusing to recursively clean a linked path: $absolute"
        }
        $relative = $absolute.Substring($rootPrefix.Length).Replace('\', '/')
        $tracked = git -C $projectRoot ls-files -- $relative
        if ($LASTEXITCODE -or $tracked) { throw "Cleanup target contains tracked files: $relative" }
    }
}
foreach ($target in $targets) {
    if (Test-Path -LiteralPath $target) {
        Remove-Item -LiteralPath $target -Recurse -Force
        Write-Host "Removed $target"
    }
}
