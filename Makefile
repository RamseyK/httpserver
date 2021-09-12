# Makefile for httpserver
# (C) Ramsey Kant 2011-2021

CC := clang++
SRCDIR := src
BINDIR := bin
BUILDDIR := build
TARGET := httpserver
UNAME := $(shell uname)

# Debug Flags
DEBUGFLAGS := -g3 -O0 -Wall -fstack-check -fstack-protector-all

# Production Flags
PRODFLAGS := -Wall -O2 -fstack-check -fstack-protector-all

# ifeq ($(UNAME), Linux)
# # Linux Flags - only supported if libkqueue is compiled from Github sources
# CFLAGS := -std=c++14 -Iinclude/ $(PRODFLAGS)
# LINK := -lpthread -lkqueue $(PRODFLAGS)
# else
# OSX / BSD Flags
CFLAGS := -std=c++14 -Iinclude/ $(PRODFLAGS)
LINK := $(PRODFLAGS)
# endif
 
 
SRCEXT := cpp
SOURCES := $(shell find $(SRCDIR) -type f -name *.$(SRCEXT))
OBJECTS := $(patsubst $(SRCDIR)/%,$(BUILDDIR)/%,$(SOURCES:.$(SRCEXT)=.o))

$(TARGET): $(OBJECTS)
	@echo " Linking..."; $(CC) $^ $(LINK) -o $(BINDIR)/$(TARGET)

$(BUILDDIR)/%.o: $(SRCDIR)/%.$(SRCEXT)
	@mkdir -p $(BUILDDIR)
	@echo " CC $<"; $(CC) $(CFLAGS) -c -o $@ $<

clean:
	@echo " Cleaning..."; rm -r $(BUILDDIR) $(BINDIR)/$(TARGET)*

.PHONY: clean
