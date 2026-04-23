# pg1

 Version: 0.9.1

 date    : 2026/04/22

 update :

***

C++ Windows PostgreSQL

* LLVM CLang use

***
* build
```
build.bat
```

***
* build.bat
* change , include path etc

```
clang++ -std=c++17 -O2 -I./include ^
-I/prog/postgresql-18.3-2/pgsql/include ^
-L/prog/postgresql-18.3-2/pgsql/lib -L./lib -llibpq ^
main.cpp -o main.exe
```

***
### blog

https://zenn.dev/knaka0209/scraps/a554a8b139db05

