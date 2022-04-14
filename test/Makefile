CPPFLAGS = -Wall -Werror -pedantic -pthread -std=c++17 -ggdb3
EXTRAFLAGS = -lpqxx -lpq -ltbb -lboost_thread -lprotobuf
TARGETS = test-database
all: $(TARGETS)

test-database: test-database.o ../DataBase/Database.o
	g++  $(CPPFLAGS) -o test-database test-database.o ../DataBase/Database.o $(EXTRAFLAGS)
test-database.o: test-database.cpp
	g++  $(CPPFLAGS) -c test-database.cpp $(EXTRAFLAGS)



.PHONY clean:
	rm -f *~ *.o test-receiver test-sender
