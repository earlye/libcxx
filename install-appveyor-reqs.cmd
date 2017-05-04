@echo on
if NOT EXIST C:\projects\deps (
  mkdir C:\projects\deps
)
cd C:\projects\deps

::###########################################################################
:: Setup the path to Clang-cl
::###########################################################################
if NOT EXIST llvm-install (
    appveyor DownloadFile http://efcs.ca/downloads/llvm-tot-win32.zip -FileName llvm-package.zip
    7z x llvm-package.zip -o"C:\projects\deps\llvm-install" > nul
)
if "%CLANG_VERSION%"=="ToT" (
    move "C:\Program Files\LLVM" "C:\Program Files\LLVM_BAK"
    mklink /D "C:\Program Files\LLVM" "C:\projects\deps\llvm-install"
)
@set PATH="C:\Program Files\LLVM\bin";%PATH%
clang-cl -v

::###########################################################################
:: Install a recent CMake
::###########################################################################
if NOT EXIST cmake (
  appveyor DownloadFile https://cmake.org/files/v3.7/cmake-3.7.2-win64-x64.zip -FileName cmake.zip
  7z x cmake.zip -oC:\projects\deps > nul
  move C:\projects\deps\cmake-* C:\projects\deps\cmake
  rm cmake.zip
)
@set PATH=C:\projects\deps\cmake\bin;%PATH%
cmake --version

::###########################################################################
:: Install Ninja
::###########################################################################
if NOT EXIST ninja (
  appveyor DownloadFile https://github.com/ninja-build/ninja/releases/download/v1.6.0/ninja-win.zip -FileName ninja.zip
  7z x ninja.zip -oC:\projects\deps\ninja > nul
  rm ninja.zip
)
@set PATH=C:\projects\deps\ninja;%PATH%
ninja --version

::###########################################################################
:: Setup the cached copy of LLVM
::###########################################################################
git clone --depth=1 https://github.com/llvm-mirror/llvm.git

@echo off
