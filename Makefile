# Makefile for httpserver
# (C) Ramsey Kant 2011-2012

CC := clang++
SRCDIR := src
BINDIR := bin
BUILDDIR := build
TARGET := httpserver

# Debug Flags
DEBUGFLAGS := -g -O0 -Wall
# Production Flags
PRODFLAGS := -Wall -O3
# Active Flags
CFLAGS := -std=c++11 -stdlib=libc++ -Iinclude/ $(DEBUGFLAGS)
LINK := -stdlib=libc++ $(DEBUGFLAGS)
 
SRCEXT := cpp
SOURCES := $(shell find $(SRCDIR) -type f -name *.$(SRCEXT))
OBJECTS := $(patsubst $(SRCDIR)/%,$(BUILDDIR)/%,$(SOURCES:.$(SRCEXT)=.o))

$(TARGET): $(OBJECTS)
	@echo " Linking..."; $(CC) $(LINK) $^ -o $(BINDIR)/$(TARGET)
 
$(BUILDDIR)/%.o: $(SRCDIR)/%.$(SRCEXT)
	@mkdir -p $(BUILDDIR)
	@echo " CC $<"; $(CC) $(CFLAGS) -c -o $@ $<
 
clean:
	@echo " Cleaning..."; $(RM) -r $(BUILDDIR) $(BINDIR)/$(TARGET)
 
.PHONY: clean
