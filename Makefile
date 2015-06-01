CPP=g++
CFLAGS+=-g  -ggdb3 -Wall -Wextra -Wno-unused-parameter -DDEBUG -fstack-check
INCLUDE+=-Ilibutp -Icxx
LDFLAGS += libutp/libutp.a
TEST_LDFLAGS= -Llibtap -ltap 
TEST_CPPFLAGS=-fprofile-arcs -ftest-coverage  -Ilibtap

# Dynamically determine if librt is available.  If so, assume we need to link
# against it for clock_gettime(2).  This is required for clean builds on OSX;
# see <https://github.com/bittorrent/libutp/issues/1> for more.  This should
# probably be ported to CMake at some point, but is suitable for now.

lrt := $(shell echo 'int main() {}' | $(CC) -xc -o /dev/null - -lrt >/dev/null 2>&1; echo $$?)
ifeq ($(strip $(lrt)),0)
  LDFLAGS += -lrt
endif

test: cxx/Storjutp.cpp cxx/FileInfo.cpp tests/test.c cxx/Storjutp.cpp cxx/Storjutp.h
	cd libutp;make
	cd libtap;make
	$(CPP) $(INCLUDE) ${CFLAGS}  ${TEST_CPPFLAGS}  -o $@ $^  ${LDFLAGS}   ${TEST_LDFLAGS} -std=c++11 -pthread 

clean:
	rm -f *.o
	rm -f bin/*
