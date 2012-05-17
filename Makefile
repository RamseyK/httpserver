# Makefile for httpserver
# (C) Ramsey Kant 2011-2012

CC = clang++
OBJS = ByteBuffer.o Client.o HTTPMessage.o HTTPRequest.o HTTPResponse.o HTTPServer.o  main.o Resource.o ResourceHost.o
# Debug Flags
DEBUGFLAGS = -g -O0 -Wall
# Production Flags
PRODFLAGS = -Wall -O3
# Active Flags
FLAGS = -Iinclude/ $(DEBUGFLAGS)
# LINK = -lpthread -lboost_thread-mt

all: $(OBJS)
	$(CC) $(FLAGS) src/*.o -o bin/httpserver

ByteBuffer.o: src/ByteBuffer.cpp
	$(CC) $(FLAGS) -c src/ByteBuffer.cpp -o src/$@

Client.o: src/Client.cpp
	$(CC) $(FLAGS) -c src/Client.cpp -o src/$@

HTTPMessage.o: src/HTTPMessage.cpp
	$(CC) $(FLAGS) -c src/HTTPMessage.cpp -o src/$@

HTTPRequest.o: src/HTTPRequest.cpp
	$(CC) $(FLAGS) -c src/HTTPRequest.cpp -o src/$@

HTTPResponse.o: src/HTTPResponse.cpp
	$(CC) $(FLAGS) -c src/HTTPResponse.cpp -o src/$@

HTTPServer.o: src/HTTPServer.cpp
	$(CC) $(FLAGS) -c src/HTTPServer.cpp -o src/$@

main.o: src/main.cpp
	$(CC) $(FLAGS) -c src/main.cpp -o src/$@

Resource.o: src/Resource.cpp
	$(CC) $(FLAGS) -c src/Resource.cpp -o src/$@

ResourceHost.o: src/ResourceHost.cpp
	$(CC) $(FLAGS) -c src/ResourceHost.cpp -o src/$@

.PHONY: clean
clean:
	rm -f bin/httpserver
	rm -f src/*.o

