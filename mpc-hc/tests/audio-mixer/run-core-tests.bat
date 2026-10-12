@echo off
setlocal
pushd "%~dp0..\.."
if not exist "build.user.bat" exit /b 1
call build.user.bat
call "%MPCHC_VS_PATH%\Common7\Tools\VsDevCmd.bat" -no_logo -arch=amd64
if errorlevel 1 exit /b 1
if not exist "bin\mixer-tests" mkdir "bin\mixer-tests"
cl /nologo /std:c++17 /EHsc /W4 /WX tests\audio-mixer\core_tests.cpp /Febin\mixer-tests\core_tests.exe /Fobin\mixer-tests\core_tests.obj
if errorlevel 1 exit /b 1
bin\mixer-tests\core_tests.exe
exit /b %errorlevel%
