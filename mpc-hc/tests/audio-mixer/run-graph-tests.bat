@echo off
setlocal
pushd "%~dp0..\.."
call build.user.bat
call "%MPCHC_VS_PATH%\Common7\Tools\VsDevCmd.bat" -no_logo -arch=amd64
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /MT /DUNICODE /D_UNICODE /DNOMINMAX /DNDEBUG /D_CONSOLE /D_WIN32_WINNT=0x0601 /Iinclude /Isrc\DSUtil /Isrc\thirdparty /Isrc\thirdparty\ResizableLib\ResizableLib /Isrc\thirdparty\LAVFilters\msvc\ffmpeg\cl\generated\x64 /Isrc\thirdparty\LAVFilters\src\ffmpeg tests\audio-mixer\graph_tests.cpp src\mpc-hc\AudioMixerSource.cpp src\mpc-hc\AudioMixerDecoder.cpp /Febin\mpc-hc_x64\graph_tests.exe /Fobin\mixer-tests\ /link bin\lib\Release_x64\BaseClasses.lib strmiids.lib winmm.lib version.lib /SUBSYSTEM:CONSOLE
if errorlevel 1 exit /b 1
for %%F in (three-tracks.mkv three-tracks.mp4 three-tracks.mov three-tracks.avi three-tracks.webm) do (
 bin\mpc-hc_x64\graph_tests.exe "%CD%\bin\mixer-tests\media" %%F
 if errorlevel 1 exit /b 1
)
exit /b 0
