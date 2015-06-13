[![Build Status](https://travis-ci.org/StorjPlatform/Storjutp.svg?branch=master)](https://travis-ci.org/StorjPlatform/Storjutp)
[![Coverage Status](https://coveralls.io/repos/StorjPlatform/Storjutp/badge.svg?branch=master)](https://coveralls.io/r/StorjPlatform/Storjutp?branch=master)

# File Transfer by [libutp](https://github.com/bittorrent/libutp)

This module trnasfer files by [libutp](https://github.com/bittorrent/libutp).
libutp uses [LEDBAT](http://en.wikipedia.org/wiki/LEDBAT) technology, 
which does not interrupt other works that uses network, like web browsing,
downloading, etc, i.e. Storjutp transfer files in _background_.

## Requirements
This requires 
* `g++` (v4.8 or higher for test)
* `Python` (2.x or 3.x)

## Installation

To compile 

    $ python setup.py install
    
To run the associated tests:

    $ git submodule update --recursive --init
    $ make test
    $ LD_LIBRARY_PATH=libtap ./test

To run the associated tests for python:

    $ python setup.py test -a "--doctest-modules --pep8 -v tests/ storjutp/"

for Windows OS, [Cygwin](https://www.cygwin.com/) must be installed first.

1. download cygwin installer from [here](https://www.cygwin.com/setup-x86.exe) and run it.
1. go forward to package selection, and select packages below, and go forward to install them.

under devel category:

1. gcc-g++
1. make
2. git

under Python category:

1. python
1. python3
1. python-setuptools
1. python3-setuptools

After that run c:\cygwin\cygwin.bat, where you can install and run StorjTelehash.

## Usage

API Document for utpbinder is [here](https://rawgit.com/StorjPlatform/Storjutp/master/docs/html/utpbinder.html)

API Document for storjutp is [here](https://rawgit.com/StorjPlatform/Storjutp/master/docs/html/storjutp.html)

# Contribution
Improvements to the codebase and pull requests are encouraged.


