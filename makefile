CC = g++
CFLAGS = -Wall -Wextra -std=c++17 -O2

.PHONY: all clean

all: serwer

serwer: http.o server.o main.o
	$(CC) -o $@ $^ -lstdc++fs

http.o: http.cpp http.h
	$(CC) $(CFLAGS) -c $<

server.o: server.cpp server.h http.o
	$(CC) $(CFLAGS) -c $<

main.o: main.cpp http.h server.h
	g++ -Wall -Wextra -std=c++17 -c $<

clean:
	rm -f *.o serwer
