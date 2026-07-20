# rag_17pg

 Version: 0.9.1

 date    : 2026/07/19
 
 update :

***

C++ windows CLI , RAG + PGVector

* OpenRouter
* embedding : Gemini-embedding-001
* LLVM CLang
* C/C++
* Visual studio 2026 community
* nmake
* windows

***
* .env
```
OPENROUTER_API_KEY=
OPENROUTER_MODEL=deepseek/deepseek-v4-flash
GEMINI_API_KEY=
```
***
table: ./table.sql

***
* LIB add ,vcpkg  install

```
.\vcpkg install curl:x64-windows
.\vcpkg install nlohmann-json:x64-windows
```

***
* Makefile
* include PATH etc, change

```
CXX = clang++
CXXFLAG_1= -I./include -I/prog/vcpkg/installed/x64-windows/include
CXXFLAG_2= -I/prog/postgresql-18.3-2/pgsql/include
CXXFLAG_LIB_1= -L/prog/postgresql-18.3-2/pgsql/lib
CXXFLAG_LIB_2= -L/prog/vcpkg/installed/x64-windows/lib -llibcurl -llibpq -lcpr

CXXFLAGS = -std=c++17 $(CXXFLAG_1) $(CXXFLAG_2) $(CXXFLAG_LIB_1) $(CXXFLAG_LIB_2)

TARGET = main.exe
all: $(TARGET)

$(TARGET): main.o pgvector_client.o
    $(CXX) $(CXXFLAGS) main.o pgvector_client.o -o $(TARGET)

pgvector_client.o: src/pgvector_client.cpp
    $(CXX) $(CXXFLAGS) -c src/pgvector_client.cpp

main.o: main.cpp
    $(CXX) $(CXXFLAGS) -c main.cpp

clean:
    del *.o *.exe
```


***
### build
```
nmake all
```

***
* vector data add
```
.\main.exe embed ./data
```
***
* search

```
.\main.exe search hello
```

***
