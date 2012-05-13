# Makefile for httpserver
# (C) Ramsey Kant 2011-2012

CC = g++
OBJS = ByteBuffer.o Client.o HTTPMessage.o HTTPRequest.o HTTPResponse.o HTTPServer.o  main.o Resource.o ResourceManager.o
# Debug Flags
DEBUGFLAGS = -g -O0 -fpermissive -Wall
# Production Flags
PRODFLAGS = -Wall -O3
# Active Flags
FLAGS = -Iinclude/ $(DEBUGFLAGS)
LINK = -lpthread -lboost_thread-mt

all: $(OBJS)
	$(CC) $(FLAGS) *.o -o bin/httpserver $(LINK)

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

main.o: Testers.h main.cpp
	$(CC) $(FLAGS) -c main.cpp -o $@

Resource.o: Resource.cpp
	$(CC) $(FLAGS) -c Resource.cpp -o $@

ResourceManager.o: ResourceManager.cpp
	$(CC) $(FLAGS) -c ResourceManager.cpp -o $@

.PHONY: clean
clean:
	rm -f bin/httpserver
	rm -f *.o

