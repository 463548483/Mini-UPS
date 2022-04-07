CPPFLAGS = -Wall -Werror -pedantic -pthread -std=c++17 -ggdb3
EXTRAFLAGS = -lpqxx -lpq -ltbb -lboost_thread
TARGETS = BaseSocket.o BaseServer.o

all: $(TARGETS)

BaseSocket.o: BaseSocket.cpp BaseSocket.hpp
	g++ $(CPPFLAGS) -c BaseSocket.cpp $(EXTRAFLAGS)

BaseServer.o: BaseServer.cpp BaseServer.hpp BaseSocket.hpp ../DataBase/Database.hpp ../protos/world_ups.pb.h
	g++ $(CPPFLAGS) -c BaseServer.cpp $(EXTRAFLAGS)

.PHONY clean:
	rm -f *~ *.o