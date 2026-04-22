# sqlite_2

 Version: 0.9.1

 date    : 2026/04/21

 update :

***

C++ Windows , SQLite example

* LLVM CLang use

***
### related

https://www.sqlite.org/download.html

* sqlite-amalgamation-*.zip , download
* sqlite3.h , sqlite3.c

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
-L/prog/vcpkg/installed/x64-windows/lib -L./lib -lsqlite3 ^
main.cpp -o main.exe
```

***
* test-data

* add
```
.\main.exe add hello
```
***
* delete , id input

```
.\main.exe rm 1
```

***
* list

```
.\main.exe list
```
***
