#########################################################################
#
# Copyright 2012 by Sean Conner.  All Rights Reserved.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
#
# Comments, questions and criticisms can be sent to: sean@conman.org
#
########################################################################

CC     = gcc -std=c99 -Wall -Wextra -pedantic -g
LUALIB = /usr/local/lib/lua/5.1

all:	ipacl-client.o	\
	ipacl-server.o	\
	ipacltest	\
	ipaclsrv	\
	ipacl_s.so	\
	ipacl.so

ipacltest : ipacltest.o ipacl-client.o
	$(CC) -o $@ ipacltest.o ipacl-client.o

ipaclsrv : ipaclsrv.c ipacl-server.o
	$(CC) -o $@ ipaclsrv.c ipacl-server.o

ipacl.so : ipacllua.c ipacl-client.c ipacl-proto.h ipacl.h
	$(CC) -shared -fPIC -o $@ ipacllua.c ipacl-client.c
	
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
	
install:
	install -d $(LUALIB)/org
	install -d $(LUALIB)/org/conman
	install -d $(LUALIB)/org/conman/net
	install ipacl_s.so $(LUALIB)/org/conman/net
	install ipacl.so   $(LUALIB)/org/conman/net

remove:
	/bin/rm -rf $(LUALIB)/org/conman/net/ipacl_s.so
	/bin/rm -rf $(LUALIB)/org/conman/net/ipacl.so
	
clean:
	/bin/rm -rf *.so *~ *.o ipacltest ipaclsrv


