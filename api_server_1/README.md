# api_server_1

 Version: 0.9.1

 date    : 2026/04/22

 update :

***

C++ windows , api-server example

* LLVM CLang use
***
* LIB add ,vcpkg  install

```
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
main.cpp -o server.exe
```

***
* start
```
.\server.exe
```

***
### blog

https://zenn.dev/knaka0209/scraps/f9fa0c8f3de785

