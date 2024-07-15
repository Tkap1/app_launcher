
@echo off

cls

if NOT defined VSCMD_ARG_TGT_ARCH (
	call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" amd64
)

taskkill /F /IM app_launcher.exe > NUL 2>&1

if not exist build\NUL mkdir build

SETLOCAL ENABLEDELAYEDEXPANSION

set files=..\src\main.cpp
set comp=-nologo -std:c++20 -Zc:strictStrings- -W4 -wd4505 -wd4324 -wd4127 -FC -I ../../my_libs -Gm- -GR- -EHa- -Dm_app -D_CRT_SECURE_NO_WARNINGS -Fe:app_launcher.exe
set linker=user32.lib Shell32.lib Gdi32.lib Opengl32.lib Dwmapi.lib Ole32.lib -INCREMENTAL:NO

set debug=0
if !debug!==0 (
	set comp=!comp! -O2 -MT -Dm_no_console
	set linker=!linker! -subsystem:windows
)
if !debug!==1 (
	set comp=!comp! -O2 -Dm_debug -MTd -Zi
	set linker=!linker! -subsystem:windows
)
if !debug!==2 (
	set comp=!comp! -Od -Dm_debug -Zi -MTd
)

pushd build
	cl !files! !comp! -link !linker! > temp_compiler_output.txt
popd
if !errorlevel!==0 goto success

goto end

:success
copy build\app_launcher.exe app_launcher.exe > NUL

:end
copy build\temp_compiler_output.txt compiler_output.txt > NUL
type build\temp_compiler_output.txt

