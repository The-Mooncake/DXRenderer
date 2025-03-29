del /f /s /q Project 1>nul
rmdir /s /q Project

mkdir Project
cd Project

CALL "%ProgramFiles%\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat" 
cd

cmake .. -G "Visual Studio 17 2022" -A "x64" ^
-DCMAKE_TOOLCHAIN_FILE="%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake" 


pause
    