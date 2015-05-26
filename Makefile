CPP=g++
CFLAGS+=-g  -ggdb3 -Wall -Wextra -Wno-unused-parameter -DDEBUG -fstack-check
INCLUDE+=-Ilibutp -Icxx
LDFLAGS += libutp/libutp.a
TEST_LDFLAGS= -Llibtap -ltap 
TEST_CPPFLAGS=-fprofile-arcs -ftest-coverage  -Ilibtap

test: cxx/Storjutp.cpp tests/test.c
	cd libutp;make
	cd libtap;make
	$(CPP) $(INCLUDE) ${CFLAGS}  ${TEST_CPPFLAGS}  -o $@ $^  ${LDFLAGS}   ${TEST_LDFLAGS} -std=c++11 -pthread 

python: ${FILES} cxx/utpbinder_python.cpp cxx/Storjutp.cpp
	cd libutp;make
	python setup.py build_ext -i

clean:
	rm -f *.o
	rm -f bin/*
