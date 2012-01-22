
CC     = gcc -std=c99 -Wall -Wextra -pedantic -g
LUALIB = /usr/local/lib/lua/5.1

all: readacl.so ipacl-client.o ipacl-server.o ipacltest ipacl_s.so

ipacltest : ipacltest.o ipacl-client.o
	$(CC) -o $@ ipacltest.o ipacl-client.o

ipacl_s.so : ipacllua-s.c ipacl-server.c ipacl-proto.h ipacl-server.h
	$(CC) -shared -fPIC -o $@ ipacllua-s.c ipacl-server.c
	
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
	
install:
	install -d $(LUALIB)/org
	install -d $(LUALIB)/org/conman
	install -d $(LUALIB)/org/conman/net
	install ipacl_s.so $(LUALIB)/org/conman/net

remove:
	/bin/rm -rf $(LUALIB)/org/conman/net/ipacl_s.so

clean:
	/bin/rm -rf readacl.so *~ *.o ipacltest


