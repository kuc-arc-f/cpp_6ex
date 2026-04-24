# rag_2pg

 Version: 0.9.1

 date    : 2026/04/22

 update :

***

C++ windows , RAG Search + PGVector

* model: gemma-4-E2B-it-Q4_K_S.gguf
* Embedding-model : Qwen3-Embedding-0.6B-Q8_0.gguf
* llama.cpp , llama-server
* LLVM CLang use
* nmake (Visual studio 2026)
 
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
table: ./table.sql

***
* LIB add ,vcpkg  install

```
#curl
.\vcpkg install curl:x64-windows

#nlohmann-json
.\vcpkg install nlohmann-json:x64-windows
```

***
### Makefile

* change: include path, lib path

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
