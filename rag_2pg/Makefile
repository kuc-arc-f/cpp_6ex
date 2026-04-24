CXX = clang++
#CXXFLAGS = -std=c++17
CXXFLAGS = -std=c++17 -I./include -I/prog/vcpkg/installed/x64-windows/include -I/prog/postgresql-18.3-2/pgsql/include -L/prog/postgresql-18.3-2/pgsql/lib -L/prog/vcpkg/installed/x64-windows/lib -llibcurl -llibpq 

all: main.exe

main.exe: main.o pgvector_client.o
    $(CXX) $(CXXFLAGS) main.o pgvector_client.o -o main.exe

pgvector_client.o: src/pgvector_client.cpp
    $(CXX) $(CXXFLAGS) -c src/pgvector_client.cpp

main.o: main.cpp
    $(CXX) $(CXXFLAGS) -c main.cpp

clean:
    del *.o *.exe