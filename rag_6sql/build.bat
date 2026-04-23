clang++ -std=c++20 -O2 -I./include ^
-I/prog/vcpkg/installed/x64-windows/include ^
-L/prog/vcpkg/installed/x64-windows/lib -L./lib -lsqlite3  -llibcurl ^
main.cpp -o main.exe
