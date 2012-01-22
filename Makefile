
CC = gcc -std=c99 -Wall -Wextra -pedantic -g

all: readacl.so ipacl-client.o ipacl-server.o ipacltest


ipacltest : ipacltest.o ipacl-client.o
	$(CC) -o $@ ipacltest.o ipacl-client.o

ipacl-server.o : ipacl-server.c ipacl-proto.h ipacl-server.h
	$(CC) -c $<
	
ipacl-client.o : ipacl-client.c ipacl.h ipacl-proto.h
	$(CC) -c $<

ipacltest.o : ipacltest.c ipacl.h
	$(CC) -c $<

ipacl-codec.o : ipacl-codec.c ipacl-proto.h
	$(CC) -c $<
	
readacl.so : readacl.c
	$(CC) -shared -fPIC -o $@ $<
	

clean:
	/bin/rm -rf readacl.so *~ *.o ipacltest
