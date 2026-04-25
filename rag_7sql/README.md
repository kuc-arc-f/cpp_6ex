# rag_7sql

 Version: 0.9.1

 date    : 2026/04/24

 update :

***

C++ windows , Server RAG + SQLite

* LLVM CLang use
* nmake (Visual studio 2026)
* model: gemma-4-E2B-it-Q4_K_S.gguf
* Embedding-model : Qwen3-Embedding-0.6B-Q8_0.gguf
* llama.cpp , llama-server

***
### vector data add

https://github.com/kuc-arc-f/cpp_6ex/tree/main/rag_6sql

***
### related

https://www.sqlite.org/download.html

* sqlite-amalgamation-*.zip , download
* sqlite3.h , sqlite3.c

***
### setup
* llama-server start
* port 8080: Qwen3-Embedding-0.6B
* port 8090: gemma-4-E2B

```
#Qwen3-Embedding-0.6B

/home/user123/llama-server -m /var/lm_data/Qwen3-Embedding-0.6B-Q8_0.gguf --embedding  -c 1024 --port 8080

#gemma-4-E2B

/usr/local/llama-b8642/llama-server -m /var/lm_data/unsloth/gemma-4-E2B-it-Q4_K_S.gguf \
 --chat-template-kwargs '{"enable_thinking": false}' --port 8090 

```

***
### related

https://huggingface.co/unsloth/gemma-4-E2B-it-GGUF

https://huggingface.co/Qwen/Qwen3-Embedding-0.6B-GGUF


***
* front-build , React

```
npm i
npm run build
```
***
* build
```
nmake all
```

***
### Makefile

* change: include path, lib path

```
CXX = clang++
CXXFLAG_1= -I./include -I/prog/vcpkg/installed/x64-windows/include
CXXFLAG_LIB_1= -L/prog/vcpkg/installed/x64-windows/lib  -L./lib -lsqlite3 -llibcurl

CXXFLAGS = -std=c++17 $(CXXFLAG_1) $(CXXFLAG_LIB_1)

TARGET = server.exe
all: $(TARGET)

$(TARGET): main.o
    $(CXX) $(CXXFLAGS) main.o -o $(TARGET)

main.o: main.cpp
    $(CXX) $(CXXFLAGS) -c main.cpp

clean:
    del *.o *.exe
```

***

* start
```
.\server.exe
```

***


