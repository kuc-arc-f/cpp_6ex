# http_client_2

 Version: 0.9.1

 date    : 2026/04/21

 update :

***

C++ win, http-client example

* LLVM CLang use
***
* LIB add ,vcpkg  install

```
#curl
.\vcpkg install curl:x64-windows

#nlohmann-json
.\vcpkg install nlohmann-json:x64-windows
```

***
* build
```
build.bat
```

***
* build.bat
* change , vcpkg path 

```
clang++ -std=c++17 -O2 -I./include ^
-I/prog/vcpkg/installed/x64-windows/include ^
-L/prog/vcpkg/installed/x64-windows/lib -llibcurl ^
main.cpp -o main.exe
```

***
