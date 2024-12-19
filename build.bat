cmake -S . -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=1 -G "Visual Studio 17 2022"
cmake --build build
.\build\Debug\groundwork.exe
