@echo off
setlocal
pushd "%~dp0..\.."
call build.user.bat
call "%MPCHC_VS_PATH%\Common7\Tools\VsDevCmd.bat" -no_logo -arch=amd64 -winsdk=%MPCHC_WINSDK_VER%
if errorlevel 1 exit /b 1
if not exist "bin\mixer-tests" mkdir "bin\mixer-tests"
cl /nologo /std:c++17 /EHsc /W4 /WX tests\audio-mixer\preferences_tests.cpp /Febin\mixer-tests\preferences_tests.exe /Fobin\mixer-tests\preferences_tests.obj
if errorlevel 1 exit /b 1
bin\mixer-tests\preferences_tests.exe
exit /b %errorlevel%
