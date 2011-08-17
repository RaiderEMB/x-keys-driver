all: xkeys.c
	gcc -o xkeys xkeys.c -lusb-1.0 libxdo.so -g
