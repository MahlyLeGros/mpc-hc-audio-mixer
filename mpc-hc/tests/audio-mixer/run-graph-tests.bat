@echo off
setlocal
pushd "%~dp0..\.."
call build.user.bat
call "%MPCHC_VS_PATH%\Common7\Tools\VsDevCmd.bat" -no_logo -arch=amd64
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /MT /DUNICODE /D_UNICODE /DNOMINMAX /DNDEBUG /D_CONSOLE /D_WIN32_WINNT=0x0601 /Iinclude /Isrc\DSUtil /Isrc\thirdparty /Isrc\thirdparty\ResizableLib\ResizableLib /Isrc\thirdparty\LAVFilters\msvc\ffmpeg\cl\generated\x64 /Isrc\thirdparty\LAVFilters\src\ffmpeg tests\audio-mixer\graph_tests.cpp src\mpc-hc\AudioMixerSource.cpp src\mpc-hc\AudioMixerDecoder.cpp /Febin\mpc-hc_x64\graph_tests.exe /Fobin\mixer-tests\ /link bin\lib\Release_x64\BaseClasses.lib strmiids.lib winmm.lib version.lib /SUBSYSTEM:CONSOLE
if errorlevel 1 exit /b 1
bin\mpc-hc_x64\graph_tests.exe "%CD%\bin\mixer-tests\media"
if errorlevel 1 exit /b 1
bin\mpc-hc_x64\graph_tests.exe "%CD%\bin\mixer-tests\media" three-tracks.mp4
exit /b %errorlevel%
