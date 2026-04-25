# qdrant_2

 Version: 0.9.1

 date    : 2026/04/24

 update :

***

C++ Windows , qdrant database example

* LLVM CLang use
* nmake (Visual Studio 2026)

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
.\main.exe search
```
***
