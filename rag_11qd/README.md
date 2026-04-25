# rag_11qd

 Version: 0.9.1

 date    : 2026/04/24

 update :

***

C++ Windows , qdrant + RAG Search

* LLVM CLang use
* nmake (Visual Studio 2026)
* model: gemma-4-E2B-it-Q4_K_S.gguf
* Embedding-model : Qwen3-Embedding-0.6B-Q8_0.gguf
* llama.cpp , llama-server

***
### related

https://huggingface.co/unsloth/gemma-4-E2B-it-GGUF

https://huggingface.co/Qwen/Qwen3-Embedding-0.6B-GGUF

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
* LIB add ,vcpkg install
```
#curl
.\vcpkg install curl:x64-windows

#nlohmann-json
.\vcpkg install nlohmann-json:x64-windows
```

***
* build
```
nmake all
```

***
* Makefile
* change: include path , lib path

```
CXX = clang++
CXXFLAG_1= -I./include -I/prog/vcpkg/installed/x64-windows/include
CXXFLAG_LIB_1= -L/prog/vcpkg/installed/x64-windows/lib -llibcurl

CXXFLAGS = -std=c++17 $(CXXFLAG_1) $(CXXFLAG_LIB_1)

TARGET = main.exe
all: $(TARGET)

$(TARGET): main.o
    $(CXX) $(CXXFLAGS) main.o -o $(TARGET)

main.o: main.cpp
    $(CXX) $(CXXFLAGS) -c main.cpp

clean:
    del *.o *.exe
```

***
* init

```
.\main.exe init
```

* embed
```
.\main.exe embed
```

* search
```
.\main.exe search hello
```

***
### blog

https://zenn.dev/knaka0209/scraps/23b0810b58207b

