
CC = gcc -std=c99 -Wall -Wextra -pedantic

readacl.so : readacl.c
	$(CC) -shared -fPIC -o $@ $<
	

clean:
	/bin/rm -rf readacl.so *~