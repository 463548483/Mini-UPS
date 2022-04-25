CPPFLAGS = -Wall -Werror -pedantic -pthread -std=c++17 -ggdb3
EXTRAFLAGS = -lpqxx -lpq -ltbb -lboost_thread -lprotobuf
TARGETS = test-Amazon
all: $(TARGETS)

test-database: test-database.o ../DataBase/Database.o
	g++  $(CPPFLAGS) -o test-database test-database.o ../DataBase/Database.o $(EXTRAFLAGS)
test-database.o: test-database.cpp
	g++  $(CPPFLAGS) -c test-database.cpp $(EXTRAFLAGS)

test-Amazon: test-Amazon.o ../Server/BaseServer.o
	g++ $(CPPFLAGS) -o test-Amazon test-Amazon.o ../DataBase/Database.o ../Server/BaseSocket.o ../Server/BaseServer.o ../Utils/utils.o ../protos/world_ups.pb.cc ../protos/world_amazon.pb.cc ../protos/ups_amazon.pb.cc $(EXTRAFLAGS)
test-Amazon.o: test-Amazon.cpp
	g++  $(CPPFLAGS) -c test-Amazon.cpp $(EXTRAFLAGS)



.PHONY clean:
	rm -f *~ *.o test-receiver test-sender test-Amazon
