CPPFLAGS = -Wall -Werror -pedantic -pthread -std=c++17 -ggdb3
EXTRAFLAGS = -lpqxx -lpq
TARGETS = Database.o 

all: $(TARGETS)

Database.o: Database.cpp Database.hpp
	g++ $(CPPFLAGS) -c Database.cpp $(EXTRAFLAGS)

clean:
	rm -f *~ *.o