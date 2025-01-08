# Makefile for httpserver
# (C) Ramsey Kant 2011-2025

DEST = httpserver
CLANG_FORMAT = clang-format
LDFLAGS ?=
CXXFLAGS ?=
CXX = clang++

CXXFLAGS += -Wall -Wextra -Wno-sign-compare -Wno-missing-field-initializers \
			-Wformat -Wformat=2 -Wimplicit-fallthrough \
			-march=x86-64-v2 -fPIE \
			-fexceptions \
			-fno-omit-frame-pointer -mno-omit-leaf-frame-pointer \
			-fno-delete-null-pointer-checks -fno-strict-aliasing \
			-pedantic -std=c++23

LDFLAGS += -fuse-ld=lld

ifeq ($(DEBUG), 1)
CXXFLAGS += -O2 -g -DDEBUG=1 \
			-fasynchronous-unwind-tables \
			-D_LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_DEBUG \
			-fstack-protector-all
else
CXXFLAGS += -O2 -DNDEBUG -flto=auto
LDFLAGS += -flto=auto
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
	CXXFLAGS=-fsanitize=address $(MAKE) debug

.PHONY: all make-src clean debug asan
