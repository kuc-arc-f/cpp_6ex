clang++ -std=c++17 -O2 -I./include ^
-I/prog/vcpkg/installed/x64-windows/include ^
-L/prog/vcpkg/installed/x64-windows/lib -llibcurl ^
main.cpp -o main.exe
