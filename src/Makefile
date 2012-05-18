# Makefile for httpserver
# (C) Ramsey Kant 2011-2012

CC = clang++
OBJS = ByteBuffer.o Client.o HTTPMessage.o HTTPRequest.o HTTPResponse.o HTTPServer.o  main.o Resource.o ResourceHost.o
# Debug Flags
DEBUGFLAGS = -g -O0 -Wall
# Production Flags
PRODFLAGS = -Wall -O3
# Active Flags
FLAGS = -std=c++11 -stdlib=libc++ -Iinclude/ $(DEBUGFLAGS)
LINK = -stdlib=libc++ $(DEBUGFLAGS)

all: $(OBJS)
	$(CC) -stdlib=libc++ $(LINK) *.o -o ../bin/httpserver

ByteBuffer.o: ByteBuffer.cpp
	$(CC) $(FLAGS) -c ByteBuffer.cpp -o $@

Client.o: Client.cpp
	$(CC) $(FLAGS) -c Client.cpp -o $@

HTTPMessage.o: HTTPMessage.cpp
	$(CC) $(FLAGS) -c HTTPMessage.cpp -o $@

HTTPRequest.o: HTTPRequest.cpp
	$(CC) $(FLAGS) -c HTTPRequest.cpp -o $@

HTTPResponse.o: HTTPResponse.cpp
	$(CC) $(FLAGS) -c HTTPResponse.cpp -o $@

HTTPServer.o: HTTPServer.cpp
	$(CC) $(FLAGS) -c HTTPServer.cpp -o $@

main.o: main.cpp
	$(CC) $(FLAGS) -c main.cpp -o $@

Resource.o: Resource.cpp
	$(CC) $(FLAGS) -c Resource.cpp -o $@

ResourceHost.o: ResourceHost.cpp
	$(CC) $(FLAGS) -c ResourceHost.cpp -o $@

.PHONY: clean
clean:
	rm -f ../bin/httpserver
	rm -f *.o

