# Makefile for httpserver
# (C) Ramsey Kant 2011-2025

DEST = httpserver
CLANG_FORMAT = clang-format
LDFLAGS ?=
CXXFLAGS ?=
CXX = clang++

CXXFLAGS += -Wall -Wextra -Wno-sign-compare -Wno-missing-field-initializers \
			-Wformat -Wformat=2 -Wimplicit-fallthrough \
			-march=x86-64-v3 -fPIE -flto=auto \
			-fexceptions \
			-fno-omit-frame-pointer -mno-omit-leaf-frame-pointer \
			-fno-delete-null-pointer-checks -fno-strict-aliasing \
			-pedantic -std=c++23

LDFLAGS += -flto=auto

ifeq ($(DEBUG), 1)
CXXFLAGS += -Og -g -ggdb3 -DDEBUG=1 \
			-fasynchronous-unwind-tables \
			-D_LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_DEBUG \
			-fstack-protector-all
else
CXXFLAGS += -O3 -DNDEBUG
endif

SOURCES = $(sort $(wildcard src/*.cpp))
OBJECTS = $(SOURCES:.cpp=.o)
CLEANFILES = $(OBJECTS) bin/$(DEST)

all: make-src

make-src: $(DEST)

$(DEST): $(OBJECTS)
	$(CXX) $(LDFLAGS) $(CXXFLAGS) $(OBJECTS) -o bin/$@

clean:
	rm -f $(CLEANFILES)

debug:
	$(MAKE) DEBUG=1 all

asan:
	CXXFLAGS=-fsanitize=address,undefined $(MAKE) debug

.PHONY: all make-src clean debug asan
