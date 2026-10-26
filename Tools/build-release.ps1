#Requires -Version 5.1
<#
.SYNOPSIS
    构建并打包 SWS Trainer（x64：Release / ReleaseCN / ReleaseEN）。

.DESCRIPTION
    环境说明（本机踩过的坑）：
      Windows SDK 装在非默认根目录（例如 D:\UWPSDK\），而 64 位注册表视图里的
      KitsRoot10 指向一个只有 References/Catalogs 的壳目录 —— 直接用 MSBuild 编译时
      会报 “无法打开包括文件: math.h / Windows.h / d3d11.h / crtdbg.h”。
      本脚本自动探测真正可用的 SDK 根目录（含 um\d3d11.h 的最高版本），并通过
      /p:WindowsSdkDir、/p:WindowsSDKVersion、/p:WindowsTargetPlatformVersion
      显式传给 MSBuild。全局属性优先级高于工程文件，所以不依赖注册表、也不需要管理员权限。
      路径统一用正斜杠，避免 “Program Files (x86)\...\10\” 结尾反斜杠导致的参数转义问题。

.PARAMETER OutputDir
    产物目录，默认 Release\<yyyyMMdd>。

.PARAMETER NoZip
    只输出 DLL 目录，不生成 zip 包。

.EXAMPLE
    pwsh -File Tools\build-release.ps1
    pwsh -File Tools\build-release.ps1 -OutputDir D:\dist -NoZip
#>
[CmdletBinding()]
param(
    [string] $OutputDir,
    [switch] $NoZip
)

$ErrorActionPreference = 'Stop'

$repoRoot = Split-Path -Parent $PSScriptRoot
$solution = Join-Path $repoRoot 'Source\ImGui DirectX 11 Kiero Hook.sln'

# 配置 -> (工程输出相对路径, 打包后文件名)
$targets = @(
    [pscustomobject]@{ Configuration = 'Release';   Output = 'Source\x64\Release\SWS 1hAck Trainer.dll';  Package = 'SWS 1hAck Trainer.dll' },
    [pscustomobject]@{ Configuration = 'ReleaseCN'; Output = 'Source\x64\ReleaseCN\CN.SWS.Trainer.dll';   Package = 'CN.SWS.Trainer.dll' },
    [pscustomobject]@{ Configuration = 'ReleaseEN'; Output = 'Source\x64\ReleaseEN\EN.SWS.Trainer.dll';   Package = 'EN.SWS.Trainer.dll' }
)

$pf86 = ${env:ProgramFiles(x86)}
$pf64 = ${env:ProgramFiles}

function Find-MSBuild {
    $vswhere = if ($pf86) { Join-Path $pf86 'Microsoft Visual Studio\Installer\vswhere.exe' }
    if ($vswhere -and (Test-Path $vswhere)) {
        $found = & $vswhere -latest -products * -requires Microsoft.Component.MSBuild -find 'MSBuild\**\Bin\MSBuild.exe' 2>$null
        if ($found) { return ($found | Select-Object -First 1).Trim() }
    }

    $candidates = @()
    if ($pf86) {
        $candidates += Join-Path $pf86 'Microsoft Visual Studio\18\BuildTools\MSBuild\Current\Bin\MSBuild.exe'
        $candidates += Join-Path $pf86 'Microsoft Visual Studio\2022\BuildTools\MSBuild\Current\Bin\MSBuild.exe'
    }
    if ($pf64) {
        $candidates += Join-Path $pf64 'Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe'
        $candidates += Join-Path $pf64 'Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe'
        $candidates += Join-Path $pf64 'Microsoft Visual Studio\18\BuildTools\MSBuild\Current\Bin\MSBuild.exe'
    }
    foreach ($c in $candidates) { if (Test-Path $c) { return $c } }

    throw 'MSBuild.exe not found. Install Visual Studio 2022+ with the C++ workload.'
}

function Resolve-WindowsSdk {
    # 注册表两个视图都读（64 位 MSBuild 读到的那个可能指向壳目录，所以不能只信它）
    $roots = New-Object System.Collections.Generic.List[string]
    foreach ($key in @(
            'HKLM:\SOFTWARE\Microsoft\Windows Kits\Installed Roots',
            'HKLM:\SOFTWARE\WOW6432Node\Microsoft\Windows Kits\Installed Roots')) {
        try {
            $value = (Get-ItemProperty -Path $key -Name KitsRoot10 -ErrorAction Stop).KitsRoot10
            if ($value) { $roots.Add($value) }
        } catch { }
    }
    $roots.Add((Join-Path $pf86 'Windows Kits\10\'))
    if ($pf64 -and $pf64 -ne $pf86) {
        $roots.Add((Join-Path $pf64 'Windows Kits\10\'))
    }

    foreach ($root in $roots) {
        if (-not (Test-Path $root)) { continue }
        $includeRoot = Join-Path $root 'Include'
        if (-not (Test-Path $includeRoot)) { continue }

        # 取得含 um\d3d11.h 与 ucrt\math.h 的最高版本
        foreach ($dir in (Get-ChildItem $includeRoot -Directory | Sort-Object Name -Descending)) {
            $hasUm = Test-Path (Join-Path $dir.FullName 'um\d3d11.h')
            $hasCrt = Test-Path (Join-Path $dir.FullName 'ucrt\math.h')
            $hasLib = Test-Path (Join-Path $root "Lib\$($dir.Name)\um\x64\d3dcompiler.lib")
            if ($hasUm -and $hasCrt -and $hasLib) {
                # 统一用正斜杠 + 结尾斜杠，直接拼进 $(WindowsSdkDir)Include\... 也能work
                $normalizedRoot = ($root -replace '\\', '/').TrimEnd('/') + '/'
                return [pscustomobject]@{ Root = $normalizedRoot; Version = $dir.Name }
            }
        }
    }

    throw 'No usable Windows 10 SDK found (need Include\<ver>\um\d3d11.h + ucrt\math.h + Lib\<ver>\um\x64).'
}

if (-not (Test-Path $solution)) { throw "Solution not found: $solution" }

$msbuild = Find-MSBuild
$sdk = Resolve-WindowsSdk

Write-Host "MSBuild : $msbuild"
Write-Host "SDK     : $($sdk.Root) ($($sdk.Version))"
Write-Host ''

foreach ($t in $targets) {
    Write-Host "=== Building $($t.Configuration)|x64 ==="
    $msbuildArgs = @(
        $solution,
        '/nologo',
        '/m',
        '/v:minimal',
        "/p:Configuration=$($t.Configuration)",
        '/p:Platform=x64',
        "/p:WindowsTargetPlatformVersion=$($sdk.Version)",
        "/p:WindowsSdkDir=$($sdk.Root)",
        "/p:WindowsSDKVersion=$($sdk.Version)/"
    )
    & $msbuild @msbuildArgs
    if ($LASTEXITCODE -ne 0) { throw "Build failed: $($t.Configuration)|x64 (exit $LASTEXITCODE)" }

    $built = Join-Path $repoRoot $t.Output
    if (-not (Test-Path $built)) { throw "Expected artifact missing: $built" }
}

# ---- 打包 ----
if (-not $OutputDir) {
    $OutputDir = Join-Path $repoRoot ('Release\' + (Get-Date -Format 'yyyyMMdd'))
}
$staging = $OutputDir
New-Item -ItemType Directory -Force -Path $staging | Out-Null

$lines = New-Object System.Collections.Generic.List[string]
foreach ($t in $targets) {
    $built = Join-Path $repoRoot $t.Output
    $dest = Join-Path $staging $t.Package
    Copy-Item -LiteralPath $built -Destination $dest -Force

    $hash = (Get-FileHash -LiteralPath $dest -Algorithm SHA256).Hash
    $lines.Add(("{0}  {1}" -f $hash.ToLower(), $t.Package))
    Write-Host ("staged {0,-24} {1,9:N0} bytes  {2}" -f $t.Package, (Get-Item $dest).Length, $hash.Substring(0, 16))
}

# 版本信息（取 git 最近一次提交；工作区有未提交改动时标注 -dirty，避免误认为是纯净提交构建）
$revision = ''
try {
    $revision = (& git -C $repoRoot rev-parse --short HEAD 2>$null)
    if ($revision -and (& git -C $repoRoot status --porcelain 2>$null)) { $revision += '-dirty' }
} catch { }
$info = @(
    "SWS Trainer (Sword With Sauce 2.4.0 / UE 4.22.3)",
    "built : $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')",
    "commit: $revision",
    "toolchain: $msbuild",
    "sdk   : $($sdk.Root)$($sdk.Version)",
    ''
) + $lines
$info | Set-Content -Path (Join-Path $staging 'SHA256SUMS.txt') -Encoding ASCII

if (-not $NoZip) {
    $zip = Join-Path $repoRoot ('Release\SWS-Trainer-x64-' + (Get-Date -Format 'yyyyMMdd') + '.zip')
    if (Test-Path $zip) { Remove-Item -LiteralPath $zip -Force }
    Compress-Archive -Path (Join-Path $staging '*') -DestinationPath $zip -CompressionLevel Optimal
    Write-Host ''
    Write-Host "zip     : $zip"
}

Write-Host ''
Write-Host "包目录  : $staging"
