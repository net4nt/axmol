# //////////////////////////////////////////////////////////////////////////////////////////
# // A multi-platform support c++11 library with focus on asynchronous socket I/O for any
# // client application.
# //////////////////////////////////////////////////////////////////////////////////////////
#
# The MIT License (MIT)
#
# Copyright (c) 2012-2025 HALX99
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#
#
# The 1k/1kiss.ps1, the core script of project 1kiss(1k)
# options
#  -p: build target platform: win32,winrt(winuwp),linux,android,osx(mac),ios,tvos,watchos,wasm
#      for android: will search ndk in sdk_root which is specified by env:ANDROID_HOME first,
#      if not found, by default will install ndk-r16b or can be specified by option: -cc 'ndk-r23c'
#  -a: build arch: x86,x64,armv7,arm64
#  -d: the build workspace, i.e project root which contains root CMakeLists.txt, empty use script run working directory aka cwd
#  -cc: The C/C++ compiler toolchain: clang, msvc, gcc(mingw) or empty use default installed on current OS
#       msvc: msvc-120, msvc-141
#       ndk: ndk-r16b, ndk-r16b+
#  -xt: cross build tool, default: cmake, for android can be gradlew, can be path of cross build tool program
#  -xc: cross build tool configure options: i.e.  -xc '-Dbuild'
#  -xb: cross build tool build options: i.e. -xb '--config','Release'
#  -prefix: the install location for missing tools in system, default is "$HOME/.1kiss"
#  -sdk: specific windows sdk version, i.e. -sdk '10.0.19041.0', leave empty, cmake will auto choose latest avaiable
#  -setupOnly: this param present, only execute step: setup
#  -configOnly: if this param present, will skip build step
# support matrix
#   | OS       |   Build targets      |  C/C++ compiler toolchain | Cross Build tool |
#   +----------+----------------------+---------------------------+------------------|
#   | Windows  |  win32,winrt        | msvc,clang,gcc(mingw)     | cmake            |
#   | Linux    | linux,android        | gcc,ndk                   | cmake,gradle     |
#   | macOS    | osx,ios,tvos,watchos | xcode                     | cmake            |
# android gradle, there a two props:
#   - -P__1K_CMAKE_VERSION
#   - -P__1k_ARCHS
# startsWith('__1K') indicate 1k gradle props
# startsWith('_1K') indicate it's a cmake config option
#
param(
    [switch]$configOnly,
    [switch]$setupOnly,
    [switch]$forceConfig,
    [switch]$ndkOnly,
    [switch]$rebuild
)

$myRoot = $PSScriptRoot

$HOST_WIN = 0 # targets: win,uwp,android
$HOST_LINUX = 1 # targets: linux,android
$HOST_MAC = 2 # targets: android,ios,osx(macos),tvos,watchos

# 0: windows, 1: linux, 2: macos
$Global:IsWin = $IsWindows -or ("$env:OS" -eq 'Windows_NT')
if ($Global:IsWin) { $HOST_OS_INT = $HOST_WIN }
else {
    if ($IsLinux) { $HOST_OS_INT = $HOST_LINUX }
    elseif ($IsMacOS) { $HOST_OS_INT = $HOST_MAC }
    else {
        throw "Unsupported host OS to run 1k/1kiss.ps1"
    }
}
$Global:ENV_PATH_SEP = @(':', ';')[$IsWin]
$Global:EXE_SUFFIX = @('', '.exe')[$IsWin]

$Script:cmake_generator = $null

$is_pwsh_ise = !!$psISE
# perfer utf-8 encoding
if ($Global:IsWin) {
    if (!$OutputEncoding -or $OutputEncoding.CodePage -ne 65001) {
        $OutputEncoding = [System.Text.Encoding]::UTF8
    }

    try {
        [System.Console]::OutputEncoding = $OutputEncoding
    }
    catch {
        # the [Console]::OutputEncoding not allow to modify inside powershell ise
        $is_pwsh_ise = $true
    }
}
$ErrorActionPreference = @('Stop', 'Continue')[$is_pwsh_ise]

# import VersionEx and others
. (Join-Path $PSScriptRoot 'extensions.ps1')

class _1kiss {
    [void] println($msg) {
        Write-Host "1kiss: $msg"
    }

    [void] print($msg) {
        Write-Host "1kiss: $msg" -NoNewline
    }

    [System.Boolean] isfile([string]$path) {
        return $path -and (Test-Path $path -PathType Leaf)
    }

    [System.Boolean] isdir([string]$path) {
        return $path -and (Test-Path $path -PathType Container)
    }

    [void] mkdirs([string]$path) {
        if (!(Test-Path $path -PathType Container)) {
            New-Item $path -ItemType Directory 1>$null
        }
    }

    [void] rmdirs([string]$path) {
        if ($this.isdir($path)) {
            $this.println("Deleting $path ...")
            Remove-Item $path -Recurse -Force
        }
    }

    [void] del([string]$path) {
        if ($this.isfile($path)) {
            $this.println("Deleting $path ...")
            Remove-Item $path -Force
        }
    }

    [void] mv([string]$path, [string]$dest) {
        if ($this.isdir($path) -and !$this.isdir($dest)) {
            Move-Item $path $dest
        }
    }
    [void] addpath([string]$path) { $this.addpath($path, $false) }
    [void] addpath([string]$path, [bool]$append) {
        if (!$path -or $env:PATH.Contains($path)) { return }
        if (!$append) { $env:PATH = "$path$Global:ENV_PATH_SEP$env:PATH" }
        else { $env:PATH = "$env:PATH$Global:ENV_PATH_SEP$path" }
    }

    [void] pause($msg) {
        $shoud_pause = $false
        do {
            if (!$Global:IsWin) { break }
            $myProcess = [System.Diagnostics.Process]::GetCurrentProcess()
            if ($myProcess.ProcessName.EndsWith('_ise')) { break }
            $parentProcess = $myProcess.Parent
            if (!$parentProcess) {
                $myPID = $myProcess.Id
                $instance = Get-WmiObject Win32_Process -Filter "ProcessId = $myPID"
                $parentProcess = Get-Process -Id $instance.ParentProcessID -ErrorAction SilentlyContinue
                if (!$parentProcess) { break }
            }

            $executed_from_explorer = ($parentProcess.ProcessName -like "explorer")
            if ($executed_from_explorer) {
                $procesCmdLineArgs = "$([System.Environment]::GetCommandLineArgs())"
                if ($procesCmdLineArgs.Contains('.ps1') -and !$procesCmdLineArgs.Contains('-noexit')) {
                    $shoud_pause = $true
                }
            }
        } while ($false)
        if ($shoud_pause) {
            $this.print("$msg, press any key to continue . . .")
            cmd /c pause 1>$null
        }
        else {
            $this.println($msg)
        }
    }

    [bool] isabspath($path) {
        return [IO.Path]::IsPathRooted($path)
    }

    # Get full path without exist check
    [string] realpath($path) {
        return $Global:ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($path)
    }

    [string] hash($content) {
        $stringAsStream = [System.IO.MemoryStream]::new()
        $writer = [System.IO.StreamWriter]::new($stringAsStream)
        $writer.write($content)
        $writer.Flush()
        $stringAsStream.Position = 0
        return (Get-FileHash -InputStream $stringAsStream -Algorithm MD5).Hash
    }

    [void] insert([ref]$arr, $item) {
        if ($item -and !$arr.Value.Contains($item)) {
            $arr.Value += $item
        }
    }
}
$1k = [_1kiss]::new()

# ---------------------- manifest --------------------
# mode:
# x.y.z+         : >=
# x.y.z          : ==
# *              : any
# x.y.z~x2.y2.z2 : range
$manifest = @{
    # cl.exe @link.exe 14.39 VS2022 17.9.x
    # exactly match format: '14.42.*'
    msvc         = '14.39+';
    vs           = '12.0+';
    ndk          = 'r23c';
    xcode        = '13.0.0+'; # range
    # _EMIT_STL_ERROR(STL1000, "Unexpected compiler version, expected Clang xx.x.x or newer.");
    # clang-cl msvc14.37 require 16.0.0+
    # clang-cl msvc14.40 require 17.0.0+
    llvm         = '17.0.6+';
    gcc          = '9.0.0+';
    cmake        = '3.23.0~3.31.1+';
    ninja        = '1.10.0+';
    python       = '3.8.0+';
    jdk          = '17.0.10+'; # jdk17+ works for android cmdlinetools 7.0+
    emsdk        = '3.1.53+';
    cmdlinetools = '12.0'; # android cmdline-tools
    buildtools   = '34.0.0'; # android build-tools
    target_sdk   = '35'      # android platforms
}

# the default generator requires explicit specified: osx, ios, android, wasm
$cmake_generators = @{
    'android' = 'Ninja'
    'wasm'    = 'Ninja'
    'wasm64'  = 'Ninja'
    'osx'     = 'Xcode'
    'ios'     = 'Xcode'
    'tvos'    = 'Xcode'
    'watchos' = 'Xcode'
    'linux'   = 'Unix Makefiles'
}

$channels = @{}

# refer to: https://developer.android.com/studio#command-line-tools-only
$cmdlinetools_revs = @{
    '12.0' = '11076708'
    '19.0' = '13114758'
}

$ndk_r23d_rev = '12186248'
# $ndk_r25d_rev = '12161346'

# eva: evaluated_args
$options = @{
    p      = $null
    a      = $null
    d      = $null
    cc     = $null
    t      = '' # cb_target
    xt     = 'cmake'
    prefix = $null
    xc     = @()
    xb     = @()
    j      = -1
    O      = -1
    sdk    = ''
    minsdk = $null
    dll    = $false
    u      = $false # whether delete 1kdist cross-platform prebuilt folder: path/to/_x
    dm     = $false # dump compiler preprocessors
    i      = $false # perform install
    scope  = 'local'
    aab    = $false
}

$optName = $null
foreach ($arg in $args) {
    if (!$optName) {
        if ($arg.StartsWith('-')) {
            $optName = $arg.SubString(1)
            if ($optName.EndsWith(':')) {
                $optName = $optName.TrimEnd(':')
            }
            $flag_tag = [string]$optName[0]
            if ($flag_tag -in 'j', 'O') {
                $flag_val = $null
                if ([int]::TryParse($optName.substring(1), [ref] $flag_val)) {
                    $optName = $null
                    $options.$flag_tag = $flag_val
                    continue
                }
            }
            if ($options[$optName] -is [bool]) {
                $options[$optName] = $true
                $optName = $null
            }
        }
    }
    else {
        if ($options.Contains($optName)) {
            $options[$optName] = $arg
        }
        else {
            $1k.println("Warning: ignore unrecognized option: $optName")
        }
        $optName = $null
    }
}

# job count
if ($options.j -eq -1) {
    $options.j = $([Environment]::ProcessorCount)
}

# translate xtool args
if ($options.xc.GetType() -eq [string]) {
    $options.xc = $options.xc.Split(' ')
}
if ($options.xb.GetType() -eq [string]) {
    $options.xb = $options.xb.Split(' ')
}

[VersionEx]$pwsh_ver = [Regex]::Match($PSVersionTable.PSVersion.ToString(), '(\d+\.)+(\*|\d+)').Value
if ([VersionEx]$pwsh_ver -lt [VersionEx]"7.0") {
    [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
}

$osVer = if ($IsWin) { "Microsoft Windows $([System.Environment]::OSVersion.Version.ToString())" } else { $PSVersionTable.OS }

# arm64,x64
# uname -m: arm64/aarch64,x86_64
if ($IsWin) {
    $__1k_archs = @{9="x64"; 10="arm64"}
    $__1k_arch_code = [int](Get-CimInstance Win32_Processor).Architecture[0]
    $HOST_CPU = $__1k_archs[$__1k_arch_code]
} else {
    $HOST_CPU = [System.Runtime.InteropServices.RuntimeInformation]::OSArchitecture.ToString().ToLower()
}

$1k.println("PowerShell $pwsh_ver on $osVer")

# determine build target os
$TARGET_OS = $options.p
if (!$TARGET_OS) {
    # choose host target if not specified by command line automatically
    $TARGET_OS = $options.p = @('win32', 'linux', 'osx')[$HOST_OS_INT]
}
else {
    $target_os_norm = @{winuwp = 'winrt'; mac = 'osx' }[$TARGET_OS]
    if ($target_os_norm) {
        $TARGET_OS = $target_os_norm
    }
}

$Global:target_minsdk = $options.minsdk
if (!$Global:target_minsdk) {
    $Global:target_minsdk = @{osx = '10.15'; winrt = '10.0.17763.0' }[$TARGET_OS]
}

# define some useful global vars
function eval($str, $raw = $false) {
    if (!$raw) {
        return Invoke-Expression "`"$str`""
    }
    else {
        return Invoke-Expression $str
    }
}

function create_symlink($sourcePath, $destPath) {
    & "$myRoot\fsync.ps1" -s $sourcePath -d $destPath -l $true 2>$null
    if(!$?) {
        throw "create_symlink $destPath ==> $sourcePath fail"
    }
}

$Global:is_wasm = $TARGET_OS.StartsWith('wasm')
$Global:is_win32 = $TARGET_OS -eq 'win32'
$Global:is_winrt = $TARGET_OS -eq 'winrt'
$Global:is_mac = $TARGET_OS -eq 'osx'
$Global:is_linux = $TARGET_OS -eq 'linux'
$Global:is_android = $TARGET_OS -eq 'android'
$Global:is_ios = $TARGET_OS -eq 'ios'
$Global:is_tvos = $TARGET_OS -eq 'tvos'
$Global:is_watchos = $TARGET_OS -eq 'watchos'
$Global:is_ios_sim = $false
$Global:is_win_family = $Global:is_winrt -or $Global:is_win32
$Global:is_darwin_embed_family = $Global:is_ios -or $Global:is_tvos -or $Global:is_watchos
$Global:is_darwin_family = $Global:is_mac -or $Global:is_darwin_embed_family
$Global:is_gh_act = "$env:GITHUB_ACTIONS" -eq 'true'

$Script:cmake_ver = ''

$Script:use_msvcr14x = $null
$null = [bool]::TryParse($env:use_msvcr14x, [ref]$Script:use_msvcr14x)

if (!$is_wasm) {
    $TARGET_CPU = $options.a
    if (!$TARGET_CPU) {
        $TARGET_CPU = @{'ios' = 'arm64'; 'tvos' = 'arm64'; 'watchos' = 'arm64'; 'android' = 'arm64'; }[$TARGET_OS]
        if (!$TARGET_CPU) {
            $TARGET_CPU = $HOST_CPU
        }
        $options.a = $TARGET_CPU
    }
    elseif ($TARGET_CPU -eq 'arm') {
        $TARGET_CPU = $options.a = 'armv7'
    }
}
else {
    $TARGET_CPU = $options.a = '*'
}

$Global:darwin_sim_suffix = $null
if ($Global:is_darwin_embed_family) {
    if ($options.sdk.StartsWith('sim')) {
        $Global:darwin_sim_suffix = '_sim'
    }
    $Global:is_ios_sim = $Global:darwin_sim_suffix -or ($TARGET_CPU -eq 'x64')
}
$Global:is_darwin_embed_device = $Global:is_darwin_embed_family -and !$Global:is_ios_sim

if (!$setupOnly) {
    $1k.println("$(Out-String -InputObject $options)")
}

$HOST_OS = @('windows', 'linux', 'macos')[$HOST_OS_INT]

# determine toolchain
$TOOLCHAIN = $options.cc
$toolchains = @{
    'win32'   = 'msvc';
    'winrt'   = 'msvc';
    'linux'   = 'gcc';
    'android' = 'clang'; # xcode clang
    'osx'     = 'clang'; # xcode clang
    'ios'     = 'clang'; # xcode clang
    'tvos'    = 'clang'; # xcode clang
    'watchos' = 'clang'; # xcode clang
    'wasm'    = 'clang'; # emcc clang
    'wasm64'  = 'clang'; # emcc clang
}
if (!$TOOLCHAIN) {
    $TOOLCHAIN = $toolchains[$TARGET_OS]
}
if (!$TOOLCHAIN) {
    throw "1kiss: Unsupported target os: $TARGET_OS"
}
$TOOLCHAIN_INFO = $TOOLCHAIN.Split('-')
$TOOLCHAIN_VER = $null
if ($TOOLCHAIN_INFO.Count -ge 2) {
    $toolVer = $TOOLCHAIN_INFO[$TOOLCHAIN_INFO.Count - 1]
    if ($toolVer -match "\d+") {
        $TOOLCHAIN_NAME = $TOOLCHAIN_INFO[0..($TOOLCHAIN_INFO.Count - 2)] -join '-'
        $TOOLCHAIN_VER = $toolVer
    }
}
if (!$TOOLCHAIN_VER) {
    $TOOLCHAIN_NAME = $TOOLCHAIN
}

$Global:is_clang = $TOOLCHAIN_NAME -eq 'clang'
$Global:is_msvc = $TOOLCHAIN_NAME -eq 'msvc'

# load toolset manifest
$manifest_file = Join-Path $myRoot 'manifest.ps1'
if ($1k.isfile($manifest_file)) {
    . $manifest_file
}

if($1k.isfile($Global:__1k_user_profile)) {
    $1k.println("Loading user build profile: $__1k_user_profile")
    $profile_entries = ConvertFrom-Props (Get-Content $__1k_user_profile)
    foreach($entry in $profile_entries.GetEnumerator()) {
        $manifest[$entry.Key] = $entry.Value
    }
}

function unescape_path([string]$Path) {
    return ($Path -replace '\\:', ':') -replace '\\\\', '\'
}

$Script:preferred_sdk_dir = $null
if($1k.isfile($Global:__1k_android_local_profile)) {
    $1k.println("Loading android local profile: $__1k_android_local_profile")
    $profile_entries = ConvertFrom-Props (Get-Content $__1k_android_local_profile)
    if ($profile_entries.Contains('sdk.dir')) {
        $Script:preferred_sdk_dir = unescape_path $profile_entries['sdk.dir']
    }
}

$install_prefix = if ($options.prefix) { $options.prefix } else { Join-Path $HOME '.1kiss' }
if (!$1k.isdir($install_prefix)) {
    $1k.mkdirs($install_prefix)
}
if ($Global:download_path) {
    $1k.mkdirs($Global:download_path)
}
else {
    $Global:download_path = $install_prefix
}

$1k.println("proj_dir=$((Get-Location).Path), install_prefix=$install_prefix")

# 1kdist
$sentry_file = Join-Path $myRoot '.gitee'
$mirror = if ($1k.isfile($sentry_file)) { 'gitee' } else { 'github' }
$mirror_conf_file = $1k.realpath("$myRoot/../manifest.json")
$mirror_current = $null
$devtools_url_base = $null
$1kdist_ver = $null

if ($1k.isfile($mirror_conf_file)) {
    $mirror_conf = ConvertFrom-Json (Get-Content $mirror_conf_file -raw)
    $mirror_current = $mirror_conf.mirrors.$mirror
    $mirror_url_base = "https://$($mirror_current.host)/"
    $1kdist_url_base = $mirror_url_base

    $1kdist_url_base += $mirror_current.'1kdist'
    $devtools_url_base += "$1kdist_url_base/devtools"
    $1kdist_ver = $mirror_conf.versions.'1kdist'
    $1kdist_url_base += "/$1kdist_ver"
}
else {
    $mirror_url_base = 'https://github.com/'
    $1kdist_url_base = $mirror_url_base
}

function 1kdist_url($filename) {
    return "$1kdist_url_base/$filename"
}

function devtool_url($filename) {
    return "$devtools_url_base/$filename"
}

# accept x.y.z-rc1
function version_eq($ver1, $ver2) {
    return $ver1 -eq $ver2
}
function version_like($ver1, $ver2) {
    return $ver1 -like $ver2
}

# $ver2: accept x.y.z-rc1
function version_ge($ver1, $ver2) {
    $validatedVer = [Regex]::Match($ver1, '(\d+\.)+(-)?(\*|\d+)')
    if ($validatedVer.Success) {
        return [VersionEx]$validatedVer.Value -ge [VersionEx]$ver2
    }
    return $false
}

# $verMain and $verMax not accept x.y.z-rc1
function version_in_range($ver1, $verMin, $verMax) {
    $validatedVer = [Regex]::Match($ver1, '(\d+\.)+(-)?(\*|\d+)')
    if ($validatedVer.Success) {
        $typedVer1 = [VersionEx]$validatedVer.Value
        $typedVerMin = [VersionEx]$verMin
        $typedVerMax = [VersionEx]$verMax
        return $typedVer1 -ge $typedVerMin -and $typedVer1 -le $typedVerMax
    }
    return $false
}

# validate $env:PATH to avoid unexpected shell script behavior
if ([Regex]::Match($env:PATH, "`'|`"").Success) {
    throw "Please remove any `' or `" from your PATH list"
}

# trim and get preferred version
function trim_ver($pattern) {
    $vers = $pattern.Split('~')
    return $vers[$vers.Count -gt 1].TrimLast('+')
}

# validate cmd follow symlink recurse
function validate_cmd_fs($source, $root) {
    $fileinfo = Get-Item $source
    if (!$fileinfo.Target) {
        if ($source -ne $root) {
            $1k.println("info: the cmd follow symlink $root ==> $source")
        }
        return $true
    }
    $target = $fileinfo.Target
    if (![IO.Path]::IsPathRooted($target)) {
        # convert symlink target to fullpath
        $target = $1k.realpath($(Join-Path $(Split-Path $source -Parent) $target))
    }
    if (!$1k.isfile($target)) {
        $1k.println("warning: the symlink target $root ==> $target is missing")
        return $false
    }
    if ($target -eq $root) {
        $1k.println("warning: detected cycle symlink for cmd $root")
        return $true
    }
    return (validate_cmd_fs $target $root)
}

function validate_cmd($source) {
    return (validate_cmd_fs $source $source)
}

function find_cmd($cmd) {
    $cmd_info = (Get-Command $cmd -ErrorAction SilentlyContinue)
    if ($cmd_info -and (validate_cmd $cmd_info.Source)) {
        return $cmd_info
    }

    return $null
}
function find_prog($name, $path = $null, $mode = 'ONLY', $cmd = $null, $params = @('--version'), $silent = $false, $usefv = $false) {
    if ($path) {
        $storedPATH = $env:PATH
        if ($mode -eq 'ONLY') {
            $env:PATH = $path
        }
        elseif ($mode -eq 'BOTH') {
            $env:PATH = "$path$ENV_PATH_SEP$env:PATH"
        }
    }
    else {
        $storedPATH = $null
    }
    if (!$cmd) { $cmd = $name }

    $params = [array]$params

    # try get match expr and preferred ver
    $checkVerCond = $null
    $minimalVer = ''
    $preferredVer = ''
    $wildcardVer = ''
    $requiredVer = $manifest[$name]
    if ($requiredVer) {
        $preferredVer = $null
        if ($requiredVer -eq '*') {
            $checkVerCond = '$True'
            $preferredVer = 'latest'
        }
        else {
            $verArr = $requiredVer.Split('~')
            $isRange = $verArr.Count -gt 1
            $minimalVer = $verArr[0]
            $preferredVer = $verArr[$isRange]
            if ($preferredVer.EndsWith('+')) {
                $preferredVer = $preferredVer.TrimEnd('+')
                if ($minimalVer.EndsWith('+')) { $minimalVer = $minimalVer.TrimEnd('+') }
                $checkVerCond = '$(version_ge $foundVer $minimalVer)'
            }
            else {
                if ($isRange) {
                    $checkVerCond = '$(version_in_range $foundVer $minimalVer $preferredVer)'
                }
                else {
                    if (!$preferredVer.Contains('*')) {
                        $checkVerCond = '$(version_eq $foundVer $preferredVer)'
                    }
                    else {
                        $wildcardVer = $preferredVer
                        $preferredVer = $wildcardVer.TrimEnd('.*')
                        $checkVerCond = '$(version_like $foundVer $wildcardVer)'
                    }
                }
            }
        }
        if (!$checkVerCond) {
            throw "Invalid tool $name=$requiredVer in manifest"
        }
    }

    # find command
    $cmd_info = find_cmd $cmd

    # needs restore immidiately since further cmd invoke maybe require system bins
    if ($path) {
        $env:PATH = "$path$ENV_PATH_SEP$storedPATH"
    }

    $found_rets = $null # prog_path,prog_version
    if ($cmd_info) {
        $prog_path = $cmd_info.Source

        if (!$usefv) {
            $verStr = $(. $cmd @params 2>$null) | Select-Object -First 1
            if ($LASTEXITCODE) {
                Write-Warning "1kiss: Get version of $cmd fail"
                $Global:LASTEXITCODE = 0
            }
            if (!$verStr -or $verStr.Contains('--version')) {
                $verInfo = $cmd_info.Version
                $verStr = "$($verInfo.Major).$($verInfo.Minor).$($verInfo.Build)"
            }

            # can match x.y.z-rc3 or x.y.z-65a239b
            $matchInfo = [Regex]::Match($verStr, '(\d+\.)+(\*|\d+)(\-[a-z0-9]+)?')
            $foundVer = $matchInfo.Value
        }
        else {
            $foundVer = "$($cmd_info.Version)"
        }
        [void]$minimalVer
        if ($checkVerCond) {
            $matched = Invoke-Expression $checkVerCond
            if ($matched) {
                if (!$silent) { $1k.println("Using ${name}: $prog_path, version: $foundVer") }
                $found_rets = $prog_path, $foundVer
            }
            else {
                # if (!$silent) { $1k.println("The installed ${name}: $prog_path, version: $foundVer not match required: $requiredVer") }
                $found_rets = $null, $preferredVer
            }
        }
        else {
            if (!$silent) { $1k.println("Using ${name}: $prog_path, version: $foundVer") }
            $found_rets = $prog_path, $foundVer
        }
    }
    else {
        if ($preferredVer) {
            $found_rets = $null, $preferredVer
        }
        else {
            throw "Not found $name, and it's not in manifest"
        }
    }

    if ($storedPATH) {
        $env:PATH = $storedPATH
    }

    $Error.Clear()

    return $found_rets
}

function exec_prog($prog, $params) {
    # & $prog_name $params
    for ($i = 0; $i -lt $params.Count; $i++) {
        $param = "'"
        $param += $params[$i]
        $param += "'"
        $params[$i] = $param
    }
    $strParams = "$params"
    return Invoke-Expression -Command "$prog $strParams"
}

function download_file($url, $out) {
    if ($1k.isfile($out)) { return }
    $1k.println("Downloading $url to $out ...")
    Invoke-WebRequest -Uri $url -OutFile $out
}

function download_and_expand($url, $out, $dest) {

    download_file $url $out
    try {
        $1k.mkdirs($dest)
        if ($out.EndsWith('.zip')) {
            if ($IsWin) {
                Expand-Archive -Path $out -DestinationPath $dest -Force
            }
            else {
                unzip -d $dest $out | Out-Null
            }
        }
        elseif ($out.EndsWith('.tar.gz')) {
            tar xf "$out" -C $dest | Out-Host
        }
        elseif ($out.EndsWith('.7z') -or $out.EndsWith('.exe')) {
            7z x "$out" "-o$dest" -bsp1 -y | Out-Host
        }
        elseif ($out.EndsWith('.sh')) {
            chmod 'u+x' "$out" | Out-Host
        }
        if (!$?) { throw "1kiss: Expand fail" }
    }
    catch {
        Remove-Item $out -Force
        throw "1kiss: Expand archive $out fail, please try again"
    }
}

function resolve_path ($path, $prefix = $null) {
    if ($1k.isabspath($path)) {
        return $path
    }
    else {
        if (!$prefix) { $prefix = $install_prefix }
        return Join-Path $prefix $path
    }
}

function fetch_pkg($url, $out = $null, $exrep = $null, $prefix = $null) {
    if (!$out) { $out = Join-Path $download_path $(Split-Path $url.Split('?')[0] -Leaf) }
    else { $out = resolve_path $out $download_path }

    $pfn_rename = $null

    if ($exrep) {
        $exrep = $exrep.Split('=')
        if ($exrep.Count -eq 1) {
            # single file
            if (!$prefix) {
                $prefix = resolve_path $exrep[0]
            }
            else {
                $prefix = resolve_path $prefix
            }
        }
        else {
            $prefix = resolve_path $prefix
            $inst_dst = Join-Path $prefix $exrep[1]
            $pfn_rename = {
                # move to plain folder name
                $full_path = (Get-ChildItem -Path $prefix -Filter $exrep[0]).FullName
                if ($full_path) {
                    $1k.mv($full_path, $inst_dst)
                }
                else {
                    throw "1kiss: rename $($exrep[0]) to $inst_dst fail"
                }
            }
            if ($1k.isdir($inst_dst)) { $1k.rmdirs($inst_dst) }
        }
    }
    else {
        $prefix = $install_prefix
    }

    download_and_expand $url $out $prefix

    if ($pfn_rename) { &$pfn_rename }
}

#
# Find latest installed: Visual Studio 12 2013 +
#   installationVersion
#   installationPath
#   instanceId: used for EnterDevShell
# result:
#   $Global:VS_INST
#
$Global:VS_INST = $null
function find_vs() {
    if (!$Global:VS_INST) {
        $VSWHERE_EXE = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
        $eap = $ErrorActionPreference
        $ErrorActionPreference = 'SilentlyContinue'

        $required_vs_ver = $manifest['vs']
        if (!$required_vs_ver) { $required_vs_ver = '12.0+' }

        # refer: https://learn.microsoft.com/en-us/visualstudio/install/workload-and-component-ids?view=vs-2022
        $require_comps = @('Microsoft.VisualStudio.Component.VC.Tools.x86.x64', 'Microsoft.VisualStudio.Product.BuildTools')
        $vs_installs = ConvertFrom-Json "$(&$VSWHERE_EXE -products * -version $required_vs_ver.TrimEnd('+') -format 'json' -requires $require_comps -requiresAny -prerelease)"
        $ErrorActionPreference = $eap

        if ($vs_installs) {
            $vs_inst_latest = $null
            $vs_ver = ''
            foreach ($vs_inst in $vs_installs) {
                $inst_ver = [VersionEx]$vs_inst.installationVersion
                if ($vs_ver -lt $inst_ver) {
                    $vs_ver = $inst_ver
                    $vs_inst_latest = $vs_inst
                }
            }
            $Global:VS_INST = $vs_inst_latest
        }
        else {
            Write-Warning "Visual studio not found, your build may not work, required: $required_vs_ver"
        }
    }
}

function install_msvc($ver, $arch) {

    $__install_code = @'
# install specified msvc for vs2022
param(
    $ver = '14.39',
    $arch = 'x64',
    $vs_major = 17,
    $vs_minor_base = 9,
    $msvc_minor_base = 39
)

$msvc_minor = [int]$ver.Split('.')[1]
$vs_minor = $vs_minor_base + ($msvc_minor - $msvc_minor_base)
$vs_ver = "$vs_major.$vs_minor"

$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$vs_installs = ConvertFrom-Json "$(&$vswhere -version "$vs_major.0" -format 'json')"

$vs_installer = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\setup.exe"
$vs_path = $vs_installs[0].installationPath

function install_msvc_comp($comp_id) {
    Write-Host "Installing $comp_id into $vs_path ..."
    &$vs_installer modify --quiet --installPath $vs_path --add $comp_id | Out-Host
    if ($?) {
        Write-Host "Install $comp_id success."
    }
    else {
        Write-Error "Install $comp_id fail!"
        exit 1
    }
}

# msvc
$vs_arch = @{x64 = 'x86.x64'; x86 = 'x86.x64'; arm64 = 'ARM64'; arm = 'ARM' }[$arch]
$msvc_comp_id = "Microsoft.VisualStudio.Component.VC.$ver.$vs_ver.$vs_arch" # refer to: https://learn.microsoft.com/en-us/visualstudio/install/workload-component-id-vs-build-tools?view=vs-2022
install_msvc_comp $msvc_comp_id

# mfc
$suffix = @{x64 = ''; x86 = ''; arm64 = '.ARM64'; arm = '.ARM' }[$arch]
$mfc_comp_id = "Microsoft.VisualStudio.Component.VC.$ver.$vs_ver.MFC$suffix"
install_msvc_comp $mfc_comp_id
'@
    $1k.println("Installing $ver, please press YES in UAC dialog, and don't close popup install window ...")
    $__install_script = [System.IO.Path]::GetTempFileName() + '.ps1'
    [System.IO.File]::WriteAllText($__install_script, $__install_code)
    $process = Start-Process powershell -ArgumentList "-File `"`"$__install_script`"`" -ver $ver -arch $arch" -Verb runas -PassThru -Wait
    $install_ret = $process.ExitCode
    [System.IO.File]::Delete($__install_script)

    if ($install_ret -eq 0) {
        $1k.println("Install msvc-$ver succeed")
    }
    else {
        throw "Install msvc-$ver fail!"
    }
}

# setup nuget, not add to path
function setup_nuget() {
    if (!$manifest['nuget']) { return $null }
    $nuget_bin = Join-Path $install_prefix 'nuget'
    $nuget_prog, $nuget_ver = find_prog -name 'nuget' -path $nuget_bin -mode 'BOTH' -params 'help' -silent $true
    if (!$nuget_prog) {
        $1k.rmdirs($nuget_bin)
        $1k.mkdirs($nuget_bin)

        $nuget_prog = Join-Path $nuget_bin 'nuget.exe'
        download_file "https://dist.nuget.org/win-x86-commandline/v$nuget_ver/nuget.exe" $nuget_prog
        if (!$1k.isfile($nuget_prog)) {
            throw "Install nuget fail"
        }
    }
    $1k.addpath($nuget_bin)
    $1k.println("Using nuget: $nuget_prog, version: $nuget_ver")
    return $nuget_prog
}

# setup python3, not install automatically
# ensure python3.exe is python.exe to solve unexpected error, i.e.
# google gclient require python3.exe, on windows 10/11 will locate to
# a dummy C:\Users\halx99\AppData\Local\Microsoft\WindowsApps\python3.exe cause
# shit strange error
function setup_python3() {
    if (!$manifest['python']) { return $null }
    $python_prog, $python_ver = find_prog -name 'python'
    if (!$python_prog) {
        throw "python $($manifest['python']) required!"
    }

    $python3_prog, $_ = find_prog -name 'python' -cmd 'python3' -silent $true
    if (!$python3_prog) {
        $python3_prog = Join-Path (Split-Path $python_prog -Parent) (Split-Path $python_prog -Leaf).Replace('python', 'python3')
        create_symlink $python_prog $python3_prog
    }
}

# setup axslcc, not add to path
function setup_axslcc() {
    if (!$manifest['axslcc']) { return $null }
    $axslcc_bin = Join-Path $install_prefix 'axslcc'
    $axslcc_prog, $axslcc_ver = find_prog -name 'axslcc' -path $axslcc_bin -mode 'BOTH'
    if ($axslcc_prog) {
        return $axslcc_prog
    }

    $axslcc_prog = (Join-Path $axslcc_bin "axslcc$EXE_SUFFIX")
    if($1k.isfile($axslcc_prog)) {
        $1k.del($axslcc_prog)
    }

    $suffix = @('win64.zip', 'linux.tar.gz', 'osx{0}.tar.gz')[$HOST_OS_INT]
    if ($IsMacOS) {
        if ([System.VersionEx]$axslcc_ver -ge [System.VersionEx]'1.9.4.1') {
            $suffix = $suffix -f "-$HOST_CPU"
        }
        else {
            $suffix = $suffix -f ''
        }
    }

    $glscc_base_url = $mirror_current.axslcc
    fetch_pkg "$mirror_url_base$glscc_base_url/v$axslcc_ver/axslcc-$axslcc_ver-$suffix" -exrep "axslcc"

    if ($1k.isfile($axslcc_prog)) {
        $1k.println("Using axslcc: $axslcc_prog, version: $axslcc_ver")
    } else {
        throw "Install axslcc fail"
    }
}

function setup_ninja() {
    if (!$manifest['ninja']) { return $null }
    $suffix = @('win', 'linux', 'mac')[$HOST_OS_INT]
    $ninja_bin = Join-Path $install_prefix 'ninja'
    $ninja_prog, $ninja_ver = find_prog -name 'ninja'
    if ($ninja_prog) {
        return $ninja_prog
    }

    $ninja_prog, $ninja_ver = find_prog -name 'ninja' -path $ninja_bin -silent $true
    if (!$ninja_prog) {
        fetch_pkg "https://github.com/ninja-build/ninja/releases/download/v$ninja_ver/ninja-$suffix.zip" -exrep 'ninja'
    }
    $1k.addpath($ninja_bin)
    $ninja_prog = (Join-Path $ninja_bin "ninja$EXE_SUFFIX")

    $1k.println("Using ninja: $ninja_prog, version: $ninja_ver")
    return $ninja_prog
}

# setup cmake
function setup_cmake($skipOS = $false) {
    $cmake_prog, $cmake_ver = find_prog -name 'cmake'
    if ($cmake_prog -and (!$skipOS -or $cmake_prog.Contains($myRoot))) {
        return $cmake_prog, $cmake_ver
    }

    $cmake_root = $(Join-Path $install_prefix 'cmake')
    $cmake_bin = Join-Path $cmake_root 'bin'
    $cmake_prog, $cmake_ver = find_prog -name 'cmake' -path $cmake_bin -mode 'ONLY' -silent $true
    if (!$cmake_prog) {
        $1k.rmdirs($cmake_root)

        $cmake_suffix = @(".zip", ".sh", ".tar.gz")[$HOST_OS_INT]
        if ($HOST_OS_INT -ne $HOST_MAC) {
            $cmake_pkg_name = "cmake-$cmake_ver-$HOST_OS-x86_64"
        }
        else {
            $cmake_pkg_name = "cmake-$cmake_ver-$HOST_OS-universal"
        }

        $cmake_pkg_path = Join-Path $install_prefix "$cmake_pkg_name$cmake_suffix"

        $assemble_url = $channels['cmake']
        if (!$assemble_url) {
            $cmake_url = "https://github.com/Kitware/CMake/releases/download/v$cmake_ver/$cmake_pkg_name$cmake_suffix"
        }
        else {
            $cmake_url = & $assemble_url -FileName "$cmake_pkg_name$cmake_suffix"
        }

        $cmake_dir = Join-Path $install_prefix $cmake_pkg_name
        if ($IsMacOS) {
            $cmake_app_contents = Join-Path $cmake_dir 'CMake.app/Contents'
        }
        if (!$1k.isdir($cmake_dir)) {
            if ($IsLinux) {
                fetch_pkg $cmake_url -out $cmake_pkg_path
            }
            else {
                fetch_pkg $cmake_url
            }
        }

        if ($1k.isdir($cmake_dir)) {
            $cmake_root0 = $cmake_dir
            if ($IsMacOS) {
                $cmake_app_contents = Join-Path $cmake_dir 'CMake.app/Contents'
                if ($1k.isdir($cmake_app_contents)) {
                    $cmake_root0 = $cmake_app_contents
                }
                sudo xattr -r -d com.apple.quarantine "$cmake_root0/bin/cmake"
            }
            $1k.mv($cmake_root0, $cmake_root)

            if ($1k.isdir($cmake_dir)) {
                $1k.rmdirs($cmake_dir)
            }
        }
        elseif ($IsLinux) {
            if ($options.scope -ne 'global') {
                $1k.mkdirs($cmake_root)
                & "$cmake_pkg_path" '--skip-license' '--exclude-subdir' "--prefix=$cmake_root" 1>$null 2>$null
            }
            else {
                $cmake_bin = '/usr/local/bin'
                sudo bash "$cmake_pkg_path" '--skip-license' '--prefix=/usr/local' 1>$null 2>$null
            }
            if (!$?) { Remove-Item $cmake_pkg_path -Force }
        }

        $cmake_prog, $_ = find_prog -name 'cmake' -path $cmake_bin -silent $true
        if (!$cmake_prog) {
            throw "Install cmake $cmake_ver fail"
        }

        $1k.println("Using cmake: $cmake_prog, version: $cmake_ver")
    }

    $1k.addpath($cmake_bin)
    return $cmake_prog, $cmake_ver
}

function ensure_cmake_ninja($cmake_prog, $ninja_prog) {
    # ensure ninja in cmake_bin
    $cmake_bin = Split-Path $cmake_prog -Parent
    $cmake_ninja_prog, $__ = find_prog -name 'ninja' -path $cmake_bin -mode 'ONLY' -silent $true
    if (!$cmake_ninja_prog) {
        $ninja_symlink_target = Join-Path $cmake_bin (Split-Path $ninja_prog -Leaf)
        # try link ninja exist cmake bin directory
        create_symlink $ninja_prog $ninja_symlink_target
    }
    return $?
}

function setup_nsis() {
    if (!$manifest['nsis']) { return $null }
    $nsis_bin = Join-Path $install_prefix "nsis"
    $nsis_prog, $nsis_ver = find_prog -name 'nsis' -cmd 'makensis' -params '/VERSION'
    if ($nsis_prog) {
        return $nsis_prog
    }

    $nsis_prog, $nsis_ver = find_prog -name 'nsis' -cmd 'makensis' -params '/VERSION' -path $nsis_bin -silent $true
    if (!$nsis_prog) {
        fetch_pkg "https://nchc.dl.sourceforge.net/project/nsis/NSIS%203/$nsis_ver/nsis-$nsis_ver.zip" -exrep "nsis-$nsis_ver=nsis"
    }
    $1k.addpath($nsis_bin)
    $nsis_prog = (Join-Path $nsis_bin "makensis$EXE_SUFFIX")
    $1k.println("Using nsis: $nsis_prog, version: $nsis_ver")
    return $nsis_prog
}

function setup_nasm() {
    if (!$manifest['nasm']) { return $null }
    $nasm_prog, $nasm_ver = find_prog -name 'nasm' -path "$install_prefix/nasm" -mode 'BOTH' -silent $true

    if (!$nasm_prog) {
        if ($IsWin) {
            $nasm_bin = Join-Path $install_prefix "nasm-$nasm_ver"
            if (!$1k.isdir($nasm_bin)) {
                fetch_pkg "https://www.nasm.us/pub/nasm/releasebuilds/$nasm_ver/win64/nasm-$nasm_ver-win64.zip"
            }
            $1k.addpath($nasm_bin)
        }
        elseif ($IsLinux) {
            if ($(which dpkg)) {
                sudo apt install nasm
            }
        }
        elseif ($IsMacOS) {
            brew install nasm
        }
    }

    $nasm_prog, $nasm_ver = find_prog -name 'nasm' -path "$install_prefix/nasm" -mode 'BOTH' -silent $true
    if ($nasm_prog) {
        $1k.println("Using nasm: $nasm_prog, version: $nasm_ver")
    }
}

function setup_jdk() {
    if (!$manifest['jdk']) { return $null }
    $arch_suffix = if ($HOST_CPU -eq 'x64') { 'x64' } else { 'aarch64' }
    $suffix = @("windows-$arch_suffix.zip", "linux-$arch_suffix.tar.gz", "macOS-$arch_suffix.tar.gz")[$HOST_OS_INT]
    $javac_prog, $jdk_ver = find_prog -name 'jdk' -cmd 'javac'
    if ($javac_prog) {
        return $javac_prog
    }

    $jdk_root = Join-Path $install_prefix "jdk"
    $java_home = if (!$IsMacOS) { $jdk_root } else { Join-Path $jdk_root 'Contents/Home' }
    $jdk_bin = Join-Path $java_home 'bin'

    $javac_prog, $jdk_ver = find_prog -name 'jdk' -cmd 'javac' -path $jdk_bin -silent $true
    if (!$javac_prog) {
        fetch_pkg "https://aka.ms/download-jdk/microsoft-jdk-$jdk_ver-$suffix" -exrep "jdk-$jdk_ver+*=jdk"
    }

    $env:JAVA_HOME = $java_home
    $env:CLASSPATH = ".;$java_home\lib\dt.jar;$java_home\lib\tools.jar"
    $1k.addpath($jdk_bin)
    $javac_prog = find_prog -name 'jdk' -cmd 'javac' -path $jdk_bin -mode 'ONLY' -silent $true
    if (!$javac_prog) {
        throw "Install jdk $jdk_ver fail"
    }

    $1k.println("Using jdk: $javac_prog, version: $jdk_ver")
}

function setup_unzip() {
    if ($IsWin) { return }
    $unzip_cmd_info = Get-Command 'unzip' -ErrorAction SilentlyContinue
    if (!$unzip_cmd_info) {
        if ($IsLinux) {
            if ($(which dpkg)) {
                sudo apt install unzip
            }
            elseif($(which pacman)) {
                sudo pacman -S --needed --noconfirm unzip
            }
            else {
                Write-Warning 'Current linux distro is not official supported'
            }
        }
        elseif ($IsMacOS) {
            brew install unzip
        }
        $unzip_cmd_info = Get-Command 'unzip' -ErrorAction SilentlyContinue
        if (!$unzip_cmd_info) {
            throw "setup unzip fail"
        }
    }
}

function setup_7z() {
    # ensure 7z_prog
    $7z_cmd_info = Get-Command '7z' -ErrorAction SilentlyContinue
    if (!$7z_cmd_info) {
        if ($IsWin) {
            $7z_prog = Join-Path $install_prefix '7z2301-x64/7z.exe'
            if (!$1k.isfile($7z_prog)) {
                fetch_pkg $(devtool_url '7z2301-x64.zip')
            }

            $7z_bin = Split-Path $7z_prog -Parent
            $1k.addpath($7z_bin)
        }
        elseif ($IsLinux) {
            if ($(which dpkg)) { sudo apt install p7zip-full }
        }
        elseif ($IsMacOS) {
            brew install p7zip
        }

        $7z_cmd_info = Get-Command '7z' -ErrorAction SilentlyContinue
        if (!$7z_cmd_info) {
            throw "setup 7z fail"
        }
    }
}

function setup_llvm() {
    if (!$manifest.Contains('llvm')) { return $null }
    $clang_prog, $clang_ver = find_prog -name 'llvm' -cmd "clang"
    if (!$clang_prog) {
        $llvm_root = Join-Path $install_prefix 'LLVM'
        $llvm_bin = Join-Path $llvm_root 'bin'
        $clang_prog, $clang_ver = find_prog -name 'llvm' -cmd "clang" -path $llvm_bin -silent $true
        if (!$clang_prog) {
            setup_7z
            fetch_pkg "https://github.com/llvm/llvm-project/releases/download/llvmorg-${clang_ver}/LLVM-${clang_ver}-win64.exe" -exrep 'LLVM'

            $clang_prog, $clang_ver = find_prog -name 'llvm' -cmd "clang" -path $llvm_bin -silent $true
            if (!$clang_prog) {
                throw "setup $clang_ver fail"
            }
        }
        $1k.addpath($llvm_bin)
        $1k.println("Using llvm: $clang_prog, version: $clang_ver")
    }
}

function setup_android_sdk() {
    if (!$manifest['ndk']) { return $null }
    $ndk_ver = $TOOLCHAIN_VER
    if (!$ndk_ver) {
        $ndk_ver = $manifest['ndk']
    }

    $IsGraterThan = if ($ndk_ver.EndsWith('+')) { '+' } else { $null }
    if ($IsGraterThan) {
        $ndk_ver = $ndk_ver.Substring(0, $ndk_ver.Length - 1)
    }


    $ndk_minor_base = [int][char]'a'

    # looking up require ndk installed in exists sdk roots
    $selected_sdk_root = $null
    if($Script:preferred_sdk_dir) {
        $selected_sdk_root = $Script:preferred_sdk_dir
        $1k.println("Using android sdk dir (Preferred): $selected_sdk_root")
    }
    elseif ($env:ANDROID_HOME) {
        $selected_sdk_root = $env:ANDROID_HOME
        $1k.println("Using android sdk dir from env:ANDROID_HOME: $selected_sdk_root")
    }
    elseif($env:ANDROID_SDK_ROOT) {
        $selected_sdk_root = $env:ANDROID_SDK_ROOT
        $1k.println("Using android sdk dir from env:ANDROID_SDK_ROOT: $selected_sdk_root")
    }
    else {
        $selected_sdk_root = Join-Path $install_prefix 'adt/sdk'
        $1k.println("Using android sdk dir from axmol external: $selected_sdk_root")
    }

    # C:\Users\halx99\AppData\Roaming\Google\AndroidStudio2024.3\options\android.sdk.path.xml
    $sdk_root = $selected_sdk_root
    $1k.mkdirs($sdk_root)

    $find_ndk_in = {
        param($sdk_dir)
        if (!$sdk_dir -or !$1k.isdir($sdk_dir)) {
            return $null
        }
        $1k.println("Looking require $ndk_ver$IsGraterThan in $sdk_dir")

        $ndk_major = ($ndk_ver -replace '[^0-9]', '')
        $ndk_minor_off = "$ndk_major".Length + 1
        $ndk_minor = if ($ndk_minor_off -lt $ndk_ver.Length) { "$([int][char]$ndk_ver.Substring($ndk_minor_off) - $ndk_minor_base)" } else { '0' }
        $ndk_rev_base = "$ndk_major.$ndk_minor"

        $ndk_parent = Join-Path $sdk_dir 'ndk'
        if (!$1k.isdir($ndk_parent)) {
            return $null
        }

        # find ndk in sdk
        $ndks = [ordered]@{}
        $ndk_rev_max = '0.0'
        foreach ($item in $(Get-ChildItem -Path "$ndk_parent")) {
            $ndkDir = $item.FullName
            $sourceProps = "$ndkDir/source.properties"
            if ($1k.isfile($sourceProps)) {
                $verLine = $(Get-Content $sourceProps | Select-Object -Index 1)
                $ndk_rev = $($verLine -split '=').Trim()[1].split('.')[0..1] -join '.'
                $ndks.Add($ndk_rev, $ndkDir)
                if ($ndk_rev_max -le $ndk_rev) {
                    $ndk_rev_max = $ndk_rev
                }
            }
        }

        $ndk_dir = $null
        if ($IsGraterThan) {
            if ($ndk_rev_max -ge $ndk_rev_base) {
                $ndk_dir = $ndks[$ndk_rev_max]
            }
        }
        else {
            $ndk_dir = $ndks[$ndk_rev_base]
        }

        if ($null -ne $ndk_dir) {
            $1k.println("Found $ndk_dir in $sdk_dir ...")
            return $ndk_dir
        }
    }

    $ndk_root = &$find_ndk_in $selected_sdk_root

    $sdk_comps = @()

    ### cmdline-tools ###
    $cmdlinetools_ver = trim_ver $manifest['cmdlinetools']
    $cmdlinetools_rev = $cmdlinetools_revs[$cmdlinetools_ver]

    $sdkmanager_prog, $sdkmanager_ver = $null, $null
    $cmdlinetools_prefix = Join-Path $sdk_root "cmdline-tools"
    $cmdlinetools_bin = Join-Path $cmdlinetools_prefix "$cmdlinetools_ver/bin"
    $sdkmanager_prog, $sdkmanager_ver = (find_prog -name 'cmdlinetools' -cmd 'sdkmanager' -path $cmdlinetools_bin -params "--version", "--sdk_root=$sdk_root")
    if (!$sdkmanager_prog) {
        $suffix = @('win', 'linux', 'mac')[$HOST_OS_INT]
        if (!$sdkmanager_prog) {
            $1k.println("Installing cmdlinetools version: $sdkmanager_ver ...")

            $cmdlinetools_pkg_name = "commandlinetools-$suffix-$($cmdlinetools_rev)_latest.zip"
            $cmdlinetools_url = "https://dl.google.com/android/repository/$cmdlinetools_pkg_name"
            fetch_pkg $cmdlinetools_url -o $cmdlinetools_pkg_name -exrep "cmdline-tools=$cmdlinetools_ver" -prefix $cmdlinetools_prefix
            $sdkmanager_prog, $_ = (find_prog -name 'cmdlinetools' -cmd 'sdkmanager' -path $cmdlinetools_bin -params "--version", "--sdk_root=$sdk_root" -silent $True)
            if (!$sdkmanager_prog) {
                throw "Install cmdlinetools version: $sdkmanager_ver fail"
            }
        }
    }

    ### NDK ###
    if (!$1k.isdir("$ndk_root")) {
        $ndk_prefix = Join-Path $sdk_root 'ndk'

        if ($ndk_ver -eq 'r23d') {
            # for android 15 16KB page size support, ndk-r23d only available on ci.android.com, refer:
            # - https://developer.android.com/about/versions/15/behavior-changes-all#16-kb
            # - https://developer.android.google.cn/about/versions/15/behavior-changes-all?hl=zh-cn#16-kb
            # IF fail, you can visit download url by browser:
            # - https://ci.android.com/builds/submitted/12186248/win64/latest/android-ndk-12186248-windows-x86_64.zip
            # - https://ci.android.com/builds/submitted/12186248/linux/latest/android-ndk-12186248-linux-x86_64.zip
            # - https://ci.android.com/builds/submitted/12186248/darwin_mac/latest/android-ndk-12186248-darwin-x86_64.zip

            $1k.println("Not found suitable android ndk, installing from ci.android.com ...")

            $_artifact = @("android-ndk-${ndk_r23d_rev}-windows-x86_64.zip",
                "android-ndk-${ndk_r23d_rev}-linux-x86_64.zip",
                "android-ndk-${ndk_r23d_rev}-darwin-x86_64.zip")[$HOST_OS_INT]
            $_target_os = @('win64', 'linux', 'darwin_mac')[$HOST_OS_INT]
            . (Join-Path $myRoot 'resolv-url.ps1') -artifact $_artifact -target $_target_os -build_id $ndk_r23d_rev -manifest gcloud -out_var 'artifact_info'
            $artifact_url = $artifact_info[0].messageData
            $full_ver = "23.3.${ndk_r23d_rev}"
            $ndk_root = Join-Path $ndk_prefix $full_ver
            fetch_pkg $artifact_url -o $_artifact -exrep "android-ndk-r23d-canary=$full_ver" -prefix $ndk_prefix
            if (!$1k.isdir($ndk_root)) { throw "Install android-ndk-r23d fail, please try again" }
        }
        else {
            $1k.println("Not found suitable android ndk, installing ndk-$ndk_ver by sdkmanager ...")

            $matchInfos = (exec_prog -prog $sdkmanager_prog -params "--sdk_root=$sdk_root", '--list' | Select-String 'ndk;')
            if ($null -ne $matchInfos -and $matchInfos.Count -gt 0) {
                $ndks = @{}
                foreach ($matchInfo in $matchInfos) {
                    $fullVer = $matchInfo.Line.Trim().Split(' ')[0] # "ndk;23.2.8568313"
                    $verNums = $fullVer.Split(';')[1].Split('.')
                    $ndkVer = 'r'
                    $ndkVer += $verNums[0]

                    $ndk_minor = [int]$verNums[1]
                    if ($ndk_minor -gt 0) {
                        $ndkVer += [char]($ndk_minor_base + $ndk_minor)
                    }
                    if (!$ndks.Contains($ndkVer)) {
                        $ndks.Add($ndkVer, $fullVer)
                    }
                }

                $ndkFullVer = $ndks[$ndk_ver]
                $full_ver = $ndkFullVer.Split(';')[1]
                $ndk_root = Join-Path $ndk_prefix $full_ver
                $sdk_comps += $ndkFullVer
            }
        }
    }

    if (!$ndkOnly) {
        $sdk_comps_list = 'platform-tools', "platforms/android-$($manifest['target_sdk'])", "build-tools/$($manifest['buildtools'])"
        foreach ($comp in $sdk_comps_list) {
            if (!$1k.isfile("$sdk_root/$comp/source.properties") -or $updateAdt) {
                $sdk_comps += $comp.Replace('/', ';')
            }
        }
    }

    if ($sdk_comps) {
        $sdk_cmdline_args = '--verbose', "--sdk_root=$sdk_root"
        $sdk_cmdline_args += $sdk_comps
        ((1..10 | ForEach-Object { "yes"; Start-Sleep -Milliseconds 100 }) | . $sdkmanager_prog --licenses --sdk_root=$sdk_root) | Out-Host
        exec_prog -prog $sdkmanager_prog -params $sdk_cmdline_args | Out-Host
    }

    return $sdk_root, $ndk_root
}

# enable emsdk emcmake
function setup_emsdk() {
    $emcc_prog, $emcc_ver = find_prog -name 'emsdk' -cmd 'emcc' -silent $true
    if (!$emcc_prog) {
        # no suitable emcc toolchain found, use official emsdk to setup
        $1k.println('Not found emcc toolchain in $env:PATH, setup emsdk ...')
        $emsdk_cmd = (Get-Command emsdk -ErrorAction SilentlyContinue)
        if (!$emsdk_cmd) {
            $emsdk_root = Join-Path $install_prefix 'emsdk'
            if (!$1k.isdir($emsdk_root)) {
                git clone 'https://github.com/emscripten-core/emsdk.git' $emsdk_root
            }
            else {
                git -C $emsdk_root pull
            }
        }
        else {
            $emsdk_root = Split-Path $emsdk_cmd.Source -Parent
        }

        $emcmake = (Get-Command emcmake -ErrorAction SilentlyContinue)
        if (!$emcmake) {
            Push-Location $emsdk_root
            ./emsdk install $emcc_ver
            ./emsdk activate $emcc_ver
            . ./emsdk_env.ps1
            Pop-Location
        }
    }
    else {
        $1k.println("Using emcc: $emcc_prog, version: $emcc_ver")
    }
}

function setup_msvc() {
    $cl_prog, $cl_ver = find_prog -name 'msvc' -cmd 'cl' -silent $true -usefv $true
    if (!$cl_prog) {
        if ($Global:VS_INST) {
            $vs_path = $Global:VS_INST.installationPath
            $dev_cmd_args = "-arch=$target_cpu -host_arch=x64 -no_logo"

            # if explicit version specified, use it
            if (!$manifest['msvc'].EndsWith('+')) { $dev_cmd_args += " -vcvars_ver=$cl_ver" }

            Import-Module "$vs_path\Common7\Tools\Microsoft.VisualStudio.DevShell.dll"
            $1k.println("Enter vs dev shell ...")
            Enter-VsDevShell -VsInstanceId $Global:VS_INST.instanceId -SkipAutomaticLocation -DevCmdArguments $dev_cmd_args -ErrorAction SilentlyContinue
            if ($?) {
                $1k.println('Enter vs dev shell success.')
            }
            else {
                # vs2022 x64,x86 share same msvc component
                install_msvc $cl_ver $TARGET_CPU
                $1k.println("Enter vs dev shell ...")
                Enter-VsDevShell -VsInstanceId $Global:VS_INST.instanceId -SkipAutomaticLocation -DevCmdArguments $dev_cmd_args -ErrorAction SilentlyContinue
                if ($?) {
                    $1k.println('Enter vs dev shell success.')
                }
                else {
                    throw "Enter vs dev shell fail, please check your vs installation"
                }
            }

            $cl_prog, $cl_ver = find_prog -name 'msvc' -cmd 'cl' -silent $true -usefv $true
            $1k.println("Using msvc: $cl_prog, version: $cl_ver")
        }
        else {
            Write-Warning "MSVC not found, your build may not work, required: $cl_ver"
        }
    }

    # msvc14x support
    if ($Script:use_msvcr14x) {
        if (!"$env:LIB".Contains('msvcr14x')) {
            $msvcr14x_root = $env:msvcr14x_ROOT
            $env:Platform = $target_cpu
            Invoke-Expression -Command "$msvcr14x_root\msvcr14x_nmake.ps1"
        }

        println "LIB=$env:LIB"
    }
}

function setup_xcode() {
    $xcode_prog, $xcode_ver = find_prog -name 'xcode' -cmd "xcodebuild" -params @('-version')
    if (!$xcode_prog) {
        throw "The command 'xcodebuild' not work, if you confirm Xcode was installed on this computer, please execute 'sudo xcode-select -switch /Applications/Xcode.app' and try again"
    }
}

# google gn build system, current windows only for build angleproject/dawn on windows
function setup_gclient() {
    if (!$ninja_prog) {
        $ninja_prog = setup_ninja
    }

    if ($Global:is_win_family) {
        setup_msvc
    }

    setup_python3

    # setup gclient tool
    # download depot_tools
    # git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git $gclient_dir
    $gclient_dir = Join-Path $install_prefix 'depot_tools'
    if (!$1k.isdir($gclient_dir)) {
        if ($IsWin) {
            $1k.mkdirs($gclient_dir)
            Invoke-WebRequest -Uri "https://storage.googleapis.com/chrome-infra/depot_tools.zip" -OutFile "${gclient_dir}.zip"
            Expand-Archive -Path "${gclient_dir}.zip" -DestinationPath $gclient_dir
        }
        else {
            git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git $gclient_dir
        }

    }
    $1k.addpath($gclient_dir, $true)
    $env:DEPOT_TOOLS_WIN_TOOLCHAIN = 0
}

# preprocess methods:
function preprocess_win() {
    $outputOptions = @()

    if ($options.sdk) {
        $outputOptions += "-DCMAKE_SYSTEM_VERSION=$($options.sdk)"
    }

    if ($Global:is_msvc) {
        # Generate vs2019 on github ci
        # Determine arch name
        $arch = if ($options.a -eq 'x86') { 'Win32' } else { $options.a }

        # arch
        $vs_ver = [VersionEx]$Global:VS_INST.installationVersion
        if ($vs_ver -ge [VersionEx]'16.0') {
            $outputOptions += '-A', $arch
            if ($TOOLCHAIN_VER -match '^\d+$') {
                $outputOptions += "-Tv$TOOLCHAIN_VER"
            } elseif($TOOLCHAIN_VER -match '^\d+\.\d+$') {
                $outputOptions += '-T', "version=$TOOLCHAIN_VER"
            }
        }
        else {
            if (!$TOOLCHAIN_VER) { $TOOLCHAIN_VER = "$($vs_ver.Major)0" }
            $gens = @{
                '120' = 'Visual Studio 12 2013';
                '140' = 'Visual Studio 14 2015'
                "150" = 'Visual Studio 15 2017';
            }
            $Script:cmake_generator = $gens[$TOOLCHAIN_VER]
            if (!$Script:cmake_generator) {
                throw "Unsupported toolchain: $TOOLCHAIN$TOOLCHAIN_VER"
            }
            if ($options.a -eq "x64") {
                $Script:cmake_generator += ' Win64'
            }
        }

        # platform
        if ($Global:is_winrt) {
            $outputOptions += '-DCMAKE_SYSTEM_NAME=WindowsStore', '-DCMAKE_SYSTEM_VERSION=10.0'
            if ($Global:target_minsdk) {
                $outputOptions += "-DCMAKE_VS_WINDOWS_TARGET_PLATFORM_MIN_VERSION=$Global:target_minsdk"
            }
        }

        if ($options.dll) {
            $outputOptions += '-DBUILD_SHARED_LIBS=TRUE'
        }
    }
    elseif ($Global:is_clang) {
        $outputOptions += '-DCMAKE_C_COMPILER=clang', '-DCMAKE_CXX_COMPILER=clang++'
        $Script:cmake_generator = 'Ninja Multi-Config'
    }
    else {
        # Generate mingw
        $Script:cmake_generator = 'Ninja Multi-Config'
    }
    # refer: https://devblogs.microsoft.com/powershell/array-literals-in-powershell
    return , $outputOptions
}

function preprocess_linux() {
    $outputOptions = @()
    if ($Global:is_clang) {
        $outputOptions += '-DCMAKE_C_COMPILER=clang', '-DCMAKE_CXX_COMPILER=clang++'
    }
    return , $outputOptions
}

$ninja_prog = $null
$is_gradlew = $options.xt.Contains('gradlew')
function preprocess_andorid() {
    $outputOptions = @()

    $t_archs = @{arm64 = 'arm64-v8a'; armv7 = 'armeabi-v7a'; x64 = 'x86_64'; x86 = 'x86'; }

    if ($is_gradlew) {
        if ($options.a.GetType() -eq [object[]]) {
            $archlist = [string[]]$options.a
        }
        else {
            $archlist = $options.a.Split(';')
        }
        for ($i = 0; $i -lt $archlist.Count; ++$i) {
            $arch = $archlist[$i]
            $archlist[$i] = $t_archs[$arch]
        }

        $archs = $archlist -join ':' # TODO: modify gradle, split by ';'

        $outputOptions += "-P__1K_CMAKE_VERSION=$($Script:cmake_ver.TrimLast('-'))"
        $outputOptions += "-P__1K_ARCHS=$archs"
        $outputOptions += '--parallel', '--info'
    }
    else {
        if (!$ndk_root) {
            throw "ndk_root not specified!"
        }
        $cmake_toolchain_file = Join-Path $ndk_root 'build/cmake/android.toolchain.cmake'
        $arch = $t_archs[$options.a]
        $outputOptions += "-DCMAKE_TOOLCHAIN_FILE=$cmake_toolchain_file", "-DANDROID_ABI=$arch"
        $outputOptions += "-DANDROID_USE_LEGACY_TOOLCHAIN_FILE=false"
        # If set to ONLY, then only the roots in CMAKE_FIND_ROOT_PATH will be searched
        # If set to BOTH, then the host system paths and the paths in CMAKE_FIND_ROOT_PATH will be searched
        # If set to NEVER, then the roots in CMAKE_FIND_ROOT_PATH will be ignored and only the host system root will be used
        # CMAKE_FIND_ROOT_PATH is preferred for additional search directories when cross-compiling
        # $outputOptions += '-DCMAKE_FIND_ROOT_PATH_MODE_PACKAGE=BOTH'
        # $outputOptions += '-DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=BOTH'
        # $outputOptions += '-DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=BOTH'
        # by default, we want find host program only when cross-compiling
        $outputOptions += '-DCMAKE_FIND_ROOT_PATH_MODE_PROGRAM=NEVER'
    }

    return , $outputOptions
}

function preprocess_osx() {
    $outputOptions = @()
    $arch = $options.a
    if ($arch -eq 'x64') {
        $arch = 'x86_64'
    }

    $outputOptions += "-DCMAKE_OSX_ARCHITECTURES=$arch"
    if ($Global:target_minsdk) {
        $outputOptions += "-DCMAKE_OSX_DEPLOYMENT_TARGET=$Global:target_minsdk"
    }
    return , $outputOptions
}

# build ios famliy (ios,tvos,watchos)
function preprocess_ios() {
    $outputOptions = @()
    $arch = $options.a
    if ($arch -eq 'x64') {
        $arch = 'x86_64'
    }
    if (!$cmake_toolchain_file) {
        $cmake_toolchain_file = Join-Path $myRoot 'ios.cmake'
        $outputOptions += "-DCMAKE_TOOLCHAIN_FILE=$cmake_toolchain_file", "-DARCHS=$arch"
        if ($Global:is_tvos) {
            $outputOptions += '-DPLAT=tvOS'
        }
        elseif ($Global:is_watchos) {
            $outputOptions += '-DPLAT=watchOS'
        }
        if ($Global:is_ios_sim) {
            $outputOptions += '-DSIMULATOR=TRUE'
        }
    }
    return , $outputOptions
}

function preprocess_wasm() {
    return , @()
}

function validHostAndToolchain() {
    $appleTable = @{
        'host'      = @{'macos' = $True };
        'toolchain' = @{'clang' = $True; };
    };
    $validTable = @{
        'win32'   = @{
            'host'      = @{'windows' = $True };
            'toolchain' = @{'msvc' = $True; 'clang' = $True; 'gcc' = $True };
        };
        'winrt'   = @{
            'host'      = @{'windows' = $True };
            'toolchain' = @{'msvc' = $True; };
        };
        'linux'   = @{
            'host'      = @{'linux' = $True };
            'toolchain' = @{'gcc' = $True; 'clang' = $True };
        };
        'android' = @{
            'host'      = @{'windows' = $True; 'linux' = $True; 'macos' = $True };
            'toolchain' = @{'clang' = $True; };
        };
        'osx'     = $appleTable;
        'ios'     = $appleTable;
        'tvos'    = $appleTable;
        'watchos' = $appleTable;
        'wasm'    = @{
            'host'      = @{'windows' = $True; 'linux' = $True; 'macos' = $True };
            'toolchain' = @{'clang' = $True; };
        };
        'wasm64'  = @{
            'host'      = @{'windows' = $True; 'linux' = $True; 'macos' = $True };
            'toolchain' = @{'clang' = $True; };
        };
    }
    $validInfo = $validTable[$TARGET_OS]
    $validOS = $validInfo.host[$HOST_OS]
    if (!$validOS) {
        throw "Can't build target $TARGET_OS on $HOST_OS"
    }
    $validToolchain = $validInfo.toolchain[$TOOLCHAIN_NAME]
    if (!$validToolchain) {
        throw "Can't build target $TARGET_OS with toolchain $TOOLCHAIN_NAME"
    }
}

$preprocessTable = @{
    'win32'   = ${function:preprocess_win};
    'winrt'   = ${function:preprocess_win};
    'linux'   = ${function:preprocess_linux};
    'android' = ${function:preprocess_andorid};
    'osx'     = ${function:preprocess_osx};
    'ios'     = ${function:preprocess_ios};
    'tvos'    = ${function:preprocess_ios};
    'watchos' = ${function:preprocess_ios};
    'wasm'    = ${Function:preprocess_wasm};
    'wasm64'  = ${Function:preprocess_wasm};
}

validHostAndToolchain

########## setup build tools if not installed #######
setup_unzip
setup_axslcc
$cmake_prog, $Script:cmake_ver = setup_cmake

if ($Global:is_win_family) {
    find_vs
    $nuget_prog = setup_nuget
}

if ($Global:is_win32) {
    $nsis_prog = setup_nsis
    if (!$Global:is_msvc) {
        $ninja_prog = setup_ninja
    }
    if ($Global:is_clang) {
        setup_llvm
    }
}
elseif ($Global:is_android) {
    $ninja_prog = setup_ninja
    # ensure ninja in cmake_bin
    if (!(ensure_cmake_ninja $cmake_prog $ninja_prog)) {
        $cmake_prog, $Script:cmake_ver = setup_cmake -skipOS $true
        if (!(ensure_cmake_ninja $cmake_prog $ninja_prog)) {
            $1k.println("Ensure ninja in cmake bin directory fail")
        }
    }

    setup_jdk # setup android sdk cmdlinetools require jdk
    $sdk_root, $ndk_root = setup_android_sdk
    $env:ANDROID_HOME = $sdk_root
    $env:ANDROID_NDK = $ndk_root
    # sync ndk env vars for some library required, i.e. will fix openssl issues:
    # no NDK xxx-linux-android-gcc on $PATH at (eval 10) line 142.
    # Note: github action vm also have follow env vars
    $env:ANDROID_NDK_HOME = $ndk_root
    $env:ANDROID_NDK_ROOT = $ndk_root

    $ndk_host = @('windows', 'linux', 'darwin')[$HOST_OS_INT]
    $env:ANDROID_NDK_BIN = Join-Path $ndk_root "toolchains/llvm/prebuilt/$ndk_host-x86_64/bin"
    function active_ndk_toolchain() {
        $1k.addpath($env:ANDROID_NDK_BIN)
        $clang_prog, $clang_ver = find_prog -name 'clang'
    }
}
elseif ($Global:is_wasm) {
    $ninja_prog = setup_ninja
    . setup_emsdk
}

$is_host_target = $Global:is_win32 -or $Global:is_linux -or $Global:is_mac
$is_host_cpu = $HOST_CPU -eq $TARGET_CPU
$cmake_target = $null

if (!$setupOnly) {
    $BUILD_DIR = $null
    $SOURCE_DIR = $null

    function resolve_out_dir($prefix) {
        if ($is_host_target) {
            if (!$is_host_cpu) {
                $out_dir = "${prefix}${TARGET_CPU}"
            }
            else {
                $out_dir = $prefix.TrimEnd("_")
            }
        }
        else {
            $out_dir = "${prefix}${TARGET_OS}"
            if ($TARGET_CPU -ne '*') {
                $out_dir += "_$TARGET_CPU"
            }
        }
        if ($Global:is_ios_sim) {
            $out_dir += $Global:darwin_sim_suffix
        }
        return $1k.realpath($out_dir)
    }

    $stored_cwd = $(Get-Location).Path
    if ($options.d) {
        Set-Location $options.d
    }

    # parsing build optimize flag from build_options
    $buildOptions = [array]$options.xb
    $nopts = $buildOptions.Count
    $optimize_flag = $null
    if ($options.O -ne -1) {
        $optimize_flag = @('Debug', 'MinSizeRel', 'RelWithDebInfo', 'Release')[$options.O]
    }
    $evaluated_build_args = @()
    for ($i = 0; $i -lt $nopts; ++$i) {
        $optv = $buildOptions[$i]
        switch ($optv) {
            '--config' {
                $optimize_flag = $buildOptions[$i++ + 1]
            }
            '--target' {
                $cmake_target = $buildOptions[$i++ + 1]
            }
            default {
                $evaluated_build_args += $optv
            }
        }
    }

    if ($options.xt -ne 'gn') {
        $BUILD_ALL_OPTIONS = $evaluated_build_args
        if (!$optimize_flag) {
            $optimize_flag = 'RelWithDebInfo'
        }
        $BUILD_ALL_OPTIONS += '--config', $optimize_flag

        # enter building steps
        $1k.println("Building target $TARGET_OS on $HOST_OS with toolchain $TOOLCHAIN ...")

        # step1. preprocess cross make options
        $CONFIG_ALL_OPTIONS = & $preprocessTable[$TARGET_OS]

        if (!$is_win_family) {
            $cm_cflags = '-fPIC'
            if ($TARGET_OS -eq 'wasm64') {
                $cm_cflags += ' -sMEMORY64'
                $CONFIG_ALL_OPTIONS += '-DEMSCRIPTEN_SYSTEM_PROCESSOR=x86_64', '-DCMAKE_CXX_FLAGS=-sMEMORY64'
            }

            $CONFIG_ALL_OPTIONS += "-DCMAKE_C_FLAGS=$cm_cflags"
        }

        if (!$CONFIG_ALL_OPTIONS) {
            $CONFIG_ALL_OPTIONS = @()
        }

        if ($env:__1K_CXXSTD) {
            $CONFIG_ALL_OPTIONS += "-DCMAKE_CXX_STANDARD=$env:__1K_CXXSTD"
        }

        if ($options.u) {
            $CONFIG_ALL_OPTIONS += '-D_1KFETCH_UPGRADE=TRUE'
        }
        else {
            $CONFIG_ALL_OPTIONS += '-D_1KFETCH_UPGRADE=FALSE'
        }

        # determine generator, build_dir, inst_dir for non gradlew projects
        if (!$is_gradlew) {
            $INST_DIR = $null
            $xopt_presets = 0
            $xprefix_optname = '-DCMAKE_INSTALL_PREFIX='
            $xopts = [array]$options.xc
            $evaluated_xopts = @()
            for ($opti = 0; $opti -lt $xopts.Count; ++$opti) {
                $opt = $xopts[$opti]
                if ($opt.StartsWith('-B')) {
                    if ($opt.Length -gt 2) {
                        $BUILD_DIR = $opt.Substring(2).Trim()
                    }
                    elseif (++$opti -lt $xopts.Count) {
                        $BUILD_DIR = $xopts[$opti]
                    }
                    ++$xopt_presets
                }
                elseif ($opt.StartsWith('-S')) {
                    if ($opt.Length -gt 2) {
                        $SOURCE_DIR = $opt.Substring(2).Trim()
                    }
                    elseif (++$opti -lt $xopts.Count) {
                        $SOURCE_DIR = $xopts[$opti]
                    }
                    ++$xopt_presets
                }
                elseif ($opt.startsWith('-G')) {
                    if ($opt.Length -gt 2) {
                        $cmake_generator = $opt.Substring(2).Trim()
                    }
                    elseif (++$opti -lt $xopts.Count) {
                        $cmake_generator = $xopts[$opti]
                    }
                    ++$xopt_presets
                }
                elseif ($opt.StartsWith($xprefix_optname)) {
                    ++$xopt_presets
                    $INST_DIR = $opt.SubString($xprefix_optname.Length)
                }
                else {
                    $evaluated_xopts += $opt
                }
            }

            $use_default = !$cmake_generator
            if ($use_default) {
                $cmake_generator = $cmake_generators[$TARGET_OS]
            }

            if ($cmake_generator) {
                $using_ninja = $cmake_generator.StartsWith('Ninja')

                if (!$is_wasm -and (!$use_default -or $TARGET_OS -ne $HOST_OS)) {
                    $CONFIG_ALL_OPTIONS += '-G', $cmake_generator
                }

                if ($cmake_generator -eq 'Unix Makefiles' -or $using_ninja) {
                    $CONFIG_ALL_OPTIONS += "-DCMAKE_BUILD_TYPE=$optimize_flag"
                }

                if ($using_ninja -and $Global:is_android) {
                    $CONFIG_ALL_OPTIONS += "-DCMAKE_MAKE_PROGRAM=$ninja_prog"
                }

                if ($cmake_generator -eq 'Xcode') {
                    setup_xcode
                }
            }

            if (!$BUILD_DIR) {
                $BUILD_DIR = resolve_out_dir 'build_'
            }
            if (!$INST_DIR) {
                $INST_DIR = resolve_out_dir 'install_'
            }

            if ($rebuild) {
                $1k.rmdirs($BUILD_DIR)
                $1k.rmdirs($INST_DIR)
            }
        }
        else {
            # android gradle
            # replace all cmake config options -DXXX to -P_1K_XXX
            $evaluated_xopts = @()
            foreach ($opt in $options.xc) {
                if ($opt.startsWith('-D')) {
                    $evaluated_xopts += "-P_1K_$($opt.substring(2))"
                }
                elseif ($opt.startsWith('-P')) {
                    $evaluated_xopts += $opt
                } # ignore unknown option type
            }
        }

        # step2. apply additional cross make options
        if ($evaluated_xopts.Count -gt 0) {
            $1k.println("Apply additional cross make options: $($evaluated_xopts), Count={0}" -f $evaluated_xopts.Count)
            $CONFIG_ALL_OPTIONS += $evaluated_xopts
        }

        $1k.println("CONFIG_ALL_OPTIONS=$CONFIG_ALL_OPTIONS, Count={0}" -f $CONFIG_ALL_OPTIONS.Count)

        if ($Global:is_android -and $is_gradlew) {
            $build_tool = (Get-Command $options.xt).Source
            $build_tool_dir = Split-Path $build_tool -Parent
            Push-Location $build_tool_dir
            if (!$configOnly) {
                $build_task = @('assemble', 'bundle')[$options.aab]
                if ($optimize_flag -eq 'Debug') {
                    & $build_tool ${build_task}Debug $CONFIG_ALL_OPTIONS | Out-Host
                }
                else {
                    & $build_tool ${build_task}Release $CONFIG_ALL_OPTIONS | Out-Host
                }
            }
            else {
                if ($optimize_flag -eq 'Debug') {
                    & $build_tool configureCMakeDebug prepareKotlinBuildScriptModel $CONFIG_ALL_OPTIONS | Out-Host
                }
                else {
                    & $build_tool configureCMakeRelWithDebInfo prepareKotlinBuildScriptModel $CONFIG_ALL_OPTIONS | Out-Host
                }
            }
            Pop-Location
        }
        else {
            # step3. configure
            $workDir = $(Get-Location).Path
            $cmakeEntryFile = 'CMakeLists.txt'
            $mainDep = if (!$SOURCE_DIR) { Join-Path $workDir $cmakeEntryFile } else { $(Join-Path $SOURCE_DIR $cmakeEntryFile) }
            if ($1k.isfile($mainDep)) {
                $mainDepChanged = $false
                # A Windows file time is a 64-bit value that represents the number of 100-nanosecond
                $tempFileItem = Get-Item $mainDep
                $lastWriteTime = $tempFileItem.LastWriteTime.ToFileTimeUTC()
                $tempFile = Join-Path $BUILD_DIR '1k_cache.txt'

                $storeHash = 0
                if ($1k.isfile($tempFile)) {
                    $storeHash = Get-Content $tempFile -Raw
                }
                $hashValue = $1k.hash("$CONFIG_ALL_OPTIONS#$lastWriteTime")
                $mainDepChanged = "$storeHash" -ne "$hashValue"
                $cmakeCachePath = $1k.realpath("$BUILD_DIR/CMakeCache.txt")

                if ($mainDepChanged -or !$1k.isfile($cmakeCachePath) -or $forceConfig) {
                    $config_cmd = if (!$is_wasm) { 'cmake' } else { 'emcmake' }
                    if ($is_wasm) {
                        $CONFIG_ALL_OPTIONS = @('cmake') + $CONFIG_ALL_OPTIONS
                    }

                    if ($options.dm) {
                        $1k.println("Dumping compiler preprocessors ...")
                        $dm_dir = Join-Path $PSScriptRoot 'dm'
                        $dm_build_dir = Join-Path $dm_dir 'build'
                        &$config_cmd $CONFIG_ALL_OPTIONS -S $dm_dir -B $dm_build_dir | Out-Host ; Remove-Item $dm_build_dir -Recurse -Force
                        $1k.println("Finish dump compiler preprocessors")
                    }
                    $CONFIG_ALL_OPTIONS += '-B', $BUILD_DIR, "-DCMAKE_INSTALL_PREFIX=$INST_DIR"
                    if ($SOURCE_DIR) { $CONFIG_ALL_OPTIONS += '-S', $SOURCE_DIR }
                    $1k.println("CMake config command: $config_cmd $CONFIG_ALL_OPTIONS")
                    &$config_cmd $CONFIG_ALL_OPTIONS | Out-Host
                    Set-Content $tempFile $hashValue -NoNewline
                }

                if (!$configOnly) {
                    if (!$is_gradlew) {
                        if (!$1k.isfile($cmakeCachePath)) {
                            Set-Location $stored_cwd
                            throw "The cmake generate incomplete, pelase add '-f' to re-generate again"
                        }
                    }

                    # step4. build
                    # apply additional build options
                    $BUILD_ALL_OPTIONS += "--parallel", "$($options.j)"


                    $1k.println("BUILD_ALL_OPTIONS=$BUILD_ALL_OPTIONS, Count={0}" -f $BUILD_ALL_OPTIONS.Count)

                    # forward non-cmake args to underlaying build toolchain, must at last
                    $forward_options = @()
                    if (($cmake_generator -eq 'Xcode') -and !$BUILD_ALL_OPTIONS.Contains('--verbose')) {
                        $forward_options += '--', '-quiet'
                    }

                    $cm_targets = $options.t

                    if ($cm_targets) {
                        if ($cm_targets -isnot [array]) {
                            $cm_targets = "$cm_targets".Split(',')
                        }
                    }
                    else {
                        $cm_targets = @()
                    }
                    if ($cmake_target) {
                        if ($cm_targets.Contains($cmake_target)) {
                            $cm_targets += $cmake_target
                        }
                    }
                    else {
                        $cmake_target = $cm_targets[-1]
                    }

                    if ($cm_targets) {
                        foreach ($target in $cm_targets) {
                            $1k.println("cmake --build $BUILD_DIR $BUILD_ALL_OPTIONS --target $target")
                            cmake --build $BUILD_DIR $BUILD_ALL_OPTIONS --target $target $forward_options | Out-Host
                            if (!$?) {
                                Set-Location $stored_cwd
                                exit $LASTEXITCODE
                            }
                        }
                    }
                    else {
                        $1k.println("cmake --build $BUILD_DIR $BUILD_ALL_OPTIONS")
                        cmake --build $BUILD_DIR $BUILD_ALL_OPTIONS $forward_options | Out-Host
                        if (!$?) {
                            Set-Location $stored_cwd
                            exit $LASTEXITCODE
                        }
                    }

                    if ($options.i) {
                        $install_args = @($BUILD_DIR, '--config', $optimize_flag)
                        cmake --install $install_args | Out-Host
                    }
                }
            }
            else {
                $1k.println("Missing file: $cmakeEntryFile")
            }
        }

        Set-Location $stored_cwd
    }
    else {
        # google gclient/gn build system
        # refer: https://chromium.googlesource.com/chromium/src/+/eca97f87e275a7c9c5b7f13a65ff8635f0821d46/tools/gn/docs/reference.md#args_specifies-build-arguments-overrides-examples

        $stored_env_path = $null
        $gn_buildargs_overrides = @()

        if ($Global:is_winrt) {
            $gn_buildargs_overrides += 'target_os=\"winuwp\"'
        }
        elseif ($Global:is_ios) {
            $gn_buildargs_overrides += 'target_os=\"ios\"'
            if ($Global:is_ios_sim) {
                $gn_buildargs_overrides += 'target_environment=\"simulator\"'
            }
        }
        elseif ($Global:is_android) {
            $gn_buildargs_overrides += 'target_os=\"android\"'
            $stored_env_path = $env:PATH
            active_ndk_toolchain
        }
        $gn_target_cpu = if ($TARGET_CPU -ne 'armv7') { $TARGET_CPU } else { 'arm' }
        $gn_buildargs_overrides += "target_cpu=\`"$gn_target_cpu\`""

        if ($options.xc) {
            $gn_buildargs_overrides += $options.xc
        }

        if ($Global:is_darwin_embed_device) {
            $gn_buildargs_overrides += 'ios_enable_code_signing=false'
        }

        if ($Script:use_msvcr14x) {
            $gn_buildargs_overrides += 'use_msvcr14x=true'
        }

        if ($optimize_flag -eq 'Debug') {
            $gn_buildargs_overrides += 'is_debug=true'
        }
        else {
            $gn_buildargs_overrides += 'is_debug=false'
        }

        Write-Output ("gn_buildargs_overrides=$gn_buildargs_overrides, Count={0}" -f $gn_buildargs_overrides.Count)

        $BUILD_DIR = resolve_out_dir 'out/'

        if ($rebuild) {
            $1k.rmdirs($BUILD_DIR)
        }

        $gn_gen_args = @('gen', $BUILD_DIR)
        if ($Global:is_win_family) {
            $sln_name = Split-Path $(Get-Location).Path -Leaf
            $gn_gen_args += '--ide=vs2022', "--sln=$sln_name"
        }

        if ($gn_buildargs_overrides) {
            $gn_gen_args += "--args=`"$gn_buildargs_overrides`""
        }

        Write-Output "Executing command: {gn $gn_gen_args}"

        # Note:
        #  1. powershell 7.2.12 works: gn $gn_gen_args, but 7.4.0 not works
        #  2. only --args="target_cpu=\"x64\"" works
        #  3. --args='target_cpu="x64" not work invoke from pwsh
        if ($IsWin) {
            cmd /c "gn $gn_gen_args"
        }
        else {
            if ([VersionEx]$pwsh_ver -ge [VersionEx]'7.3') {
                bash -c "gn $gn_gen_args"
            }
            else {
                gn $gn_gen_args
            }
        }

        # build
        autoninja -C $BUILD_DIR --verbose $options.t

        # restore env:PATH
        if ($stored_env_path) {
            $env:PATH = $stored_env_path
        }
    }

    $Global:BUILD_DIR = $BUILD_DIR
    $env:buildResult = ConvertTo-Json @{
        buildDir     = $BUILD_DIR
        targetOS     = $TARGET_OS
        targetCPU    = $TARGET_CPU
        hostCPU      = $HOST_CPU
        isHostTarget = $is_host_target
        compilerID   = $TOOLCHAIN_NAME
    }
}

exit $LASTEXITCODE
