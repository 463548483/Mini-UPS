CPPFLAGS = -Wall -Werror -pedantic -pthread -std=c++17 -ggdb3
EXTRAFLAGS = -lpqxx -lpq -ltbb -lboost_thread -lprotobuf
TARGETS = server

all: $(TARGETS)

server: server.o Server/BaseServer.o Server/BaseSocket.o Utils/utils.o DataBase/Database.o  protos/world_ups.pb.cc protos/world_amazon.pb.cc protos/ups_amazon.pb.cc
	g++ $(CPPFLAGS) -o server server.o DataBase/Database.o Server/BaseSocket.o Server/BaseServer.o Utils/utils.o protos/world_ups.pb.cc protos/world_amazon.pb.cc protos/ups_amazon.pb.cc $(EXTRAFLAGS)

server.o: server.cpp protos/world_ups.pb.h Server/BaseServer.hpp Utils/utils.hpp
	g++ $(CPPFLAGS) -c server.cpp $(EXTRAFLAGS)

.PHONY clean:
	rm -f *~ *.o server