
CC = gcc -std=c99 -Wall -Wextra -pedantic

all: readacl.so ipacl.o ipacl-codec.o

ipacl.o : ipacl.c ipacl.h ipacl-proto.h
	$(CC) -c $<

ipacl-codec.o : ipacl-codec.c ipacl-proto.h
	$(CC) -c $<
	
readacl.so : readacl.c
	$(CC) -shared -fPIC -o $@ $<
	

clean:
	/bin/rm -rf readacl.so *~ *.o
