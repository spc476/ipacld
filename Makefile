
CC = gcc -std=c99 -Wall -Wextra -pedantic

all: readacl.so ipacl.o

ipacl.o : ipacl.c ipacl.h ipacl-proto.h
	$(CC) -c $<
	
readacl.so : readacl.c
	$(CC) -shared -fPIC -o $@ $<
	

clean:
	/bin/rm -rf readacl.so *~