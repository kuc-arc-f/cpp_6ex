CXX = clang++
CXXFLAG_1= -I./include -I/prog/vcpkg/installed/x64-windows/include
CXXFLAG_2= -I/prog/postgresql-18.3-2/pgsql/include
CXXFLAG_LIB_1= -L/prog/postgresql-18.3-2/pgsql/lib
CXXFLAG_LIB_2= -L/prog/vcpkg/installed/x64-windows/lib -llibcurl -llibpq 

CXXFLAGS = -std=c++17 $(CXXFLAG_1) $(CXXFLAG_2) $(CXXFLAG_LIB_1) $(CXXFLAG_LIB_2)
#CXXFLAGS = -std=c++17 $(CXXFLAG_1) $(CXXFLAG_2)

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