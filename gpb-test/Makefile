CPPFLAGS = -Wall -Werror -pedantic -pthread -std=c++17 -ggdb3
EXTRAFLAGS = -lpqxx -lpq -ltbb -lboost_thread -lprotobuf
TARGETS = test-receiver test-sender

all: $(TARGETS)

test-receiver: test-receiver.o ../Server/BaseServer.o ../Server/BaseSocket.o ../Utils/utils.o ../DataBase/Database.o  world_ups.pb.cc
	g++ $(CPPFLAGS) -o test-receiver test-receiver.o ../DataBase/Database.o ../Server/BaseSocket.o ../Server/BaseServer.o ../Utils/utils.o world_ups.pb.cc $(EXTRAFLAGS)

test-sender: test-sender.o ../Server/BaseSocket.o ../Utils/utils.o world_ups.pb.cc
	g++ $(CPPFLAGS) -o test-sender test-sender.o ../Server/BaseSocket.o ../Utils/utils.o world_ups.pb.cc $(EXTRAFLAGS)

test-receiver.o: test-receiver.cpp world_ups.pb.h ../Server/BaseServer.hpp ../Utils/utils.hpp
	g++ $(CPPFLAGS) -c test-receiver.cpp $(EXTRAFLAGS)

test-sender.o: test-sender.cpp world_ups.pb.h ../Server/BaseSocket.hpp ../Utils/utils.hpp
	g++ $(CPPFLAGS) -c test-sender.cpp $(EXTRAFLAGS)

.PHONY clean:
	rm -f *~ *.o test-receiver test-sender