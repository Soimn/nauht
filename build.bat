@echo off

setlocal

cd %~dp0

if not exist build mkdir build
cd build

set "enabled_warnings= -Xclang -std=c11 -Wall -Wextra -Wshadow -Wconversion"
set "ignored_warnings= -Wno-unused-function -Wno-c23-compat -Wno-declaration-after-statement -Wno-missing-prototypes -Wno-unused-parameter -Wno-unsafe-buffer-usage -Wno-cast-qual -Wno-unused-macros"

set "common_compile_options= %enabled_warnings% %ignored_warnings% -arch:AVX2"
set "common_link_options= -incremental:no -opt:ref"

if "%1"=="debug" (
	set "compile_options=%common_compile_options% -Od -Zo -Z7 -RTC1 -DNAUHT_DEBUG"
	set "link_options=%common_link_options% -DEBUG:FULL libucrtd.lib libvcruntimed.lib"
) else if "%1"=="release" (
	set "compile_options=%common_compile_options% -O2 -Zo -Z7"
	set "link_options=%common_link_options% libvcruntime.lib"
) else (
  goto invalid_arguments
)

if "%2"=="" (
	set "compile_platform_layer=yes"
) else if "%2"=="game" (
	set "compile_platform_layer=no"
) else (
	goto invalid_arguments
)

clang-cl %compile_options% ..\src\game.c -LD -link %link_options% -pdb:nauht_game.pdb -out:nauht_game_tmp.dll -export:Tick -export:Init
if "%errorlevel%" neq "0" (
	goto end
)

REM move /Y nauht_game_tmp.dll nauht_game.dll >nul
move /Y nauht_game_tmp.dll nauht_game.dll

if "%compile_platform_layer%"=="yes" (
	clang-cl %compile_options% ..\src\platform_win32.c -link %link_options% -pdb:nauht.pdb -out:nauht.exe user32.lib gdi32.lib dxguid.lib dxgi.lib d3d11.lib
)

goto end

:invalid_arguments
echo Invalid arguments^. Usage^: build ^<debug or release^> ^[game^]
goto end

:end
endlocal
