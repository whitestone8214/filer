#!/bin/bash


if (test "$1" = "build"); then
	gcc -O3 -fPIC $(pkg-config --cflags --libs byter) -I. filer.c -shared -o libfiler.so
elif (test "$1" = "install"); then
	cp -f filer.h /usr/include/ || exit 1
	cp -f libfiler.so /lib/ || exit 1
	cp -f filer.pc /lib/pkgconfig/ || exit 1
fi
