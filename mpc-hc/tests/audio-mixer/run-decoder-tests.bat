@echo off
setlocal
pushd "%~dp0..\.."
call build.user.bat
call "%MPCHC_VS_PATH%\Common7\Tools\VsDevCmd.bat" -no_logo -arch=amd64
if errorlevel 1 exit /b 1
if not exist "bin\mpc-hc_x64" mkdir "bin\mpc-hc_x64"
cl /nologo /std:c++17 /EHsc /W4 /I src\thirdparty\LAVFilters\msvc\ffmpeg\cl\generated\x64 /I src\thirdparty\LAVFilters\src\ffmpeg tests\audio-mixer\decoder_tests.cpp src\mpc-hc\AudioMixerDecoder.cpp /Febin\mpc-hc_x64\decoder_tests.exe /Fobin\mixer-tests\
if errorlevel 1 exit /b 1
bin\mpc-hc_x64\decoder_tests.exe "%CD%\bin\mixer-tests\media"
exit /b %errorlevel%
