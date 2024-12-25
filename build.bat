cmake -G "Visual Studio 17 2022" -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=YES 
cmake --build build
.\build\Debug\groundwork.exe --debug
