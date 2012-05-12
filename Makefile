# Makefile for httpserver
# (C) Ramsey Kant 2011-2012

CC = g++
OBJS = 
# Debug Flags
DEBUGFLAGS = -g -O0 -fpermissive -Wall
# Production Flags
PRODFLAGS = -Wall -O3
# Active Flags
FLAGS = $(DEBUGFLAGS)



.PHONY: clean
clean:
	rm -f bin/test
	rm -f bin/*.o

