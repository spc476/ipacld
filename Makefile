
CC = gcc -std=c99 -Wall -Wextra -pedantic -g

all: readacl.so ipacl-client.o ipacl-codec.o ipacl-server.o

ipacl-server.o : ipacl-server.c ipacl-proto.h ipacl-server.h
	$(CC) -c $<
	
ipacl-client.o : ipacl-client.c ipacl.h ipacl-proto.h
	$(CC) -c $<

ipacl-codec.o : ipacl-codec.c ipacl-proto.h
	$(CC) -c $<
	
readacl.so : readacl.c
	$(CC) -shared -fPIC -o $@ $<
	

clean:
	/bin/rm -rf readacl.so *~ *.o
