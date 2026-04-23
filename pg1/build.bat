clang++ -std=c++17 -O2 -I./include ^
-I/prog/postgresql-18.3-2/pgsql/include ^
-L/prog/postgresql-18.3-2/pgsql/lib -L./lib -llibpq ^
main.cpp -o main.exe
