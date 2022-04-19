CPPFLAGS = -Wall -Werror -pedantic -pthread -std=c++17 -ggdb3
TARGETS = utils.o

all: $(TARGETS)

utils.o: utils.cpp utils.hpp
	g++ $(CPPFLAGS) -c utils.cpp

.PHONY clean:
	rm -f *~ *.o
