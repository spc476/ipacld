/*********************************************************************
*
* Copyright 2012 by Sean Conner.  All Rights Reserved.
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*   
* Comments, questions and criticisms can be sent to: sean@conman.org
*
*********************************************************************/

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/poll.h>
#include <netdb.h>
#include <unistd.h>

#include "ipacl-proto.h"
#include "ipacl-server.h"

typedef union
{
  struct sockaddr     sa;
  struct sockaddr_in  sin;
  struct sockaddr_in6 sin6;
} addr__t;

int main(
	int   argc   __attribute__((unused)),
	char *argv[] __attribute__((unused))
)
{
  int fh;
  int rc;
  
  rc = ipacls_open(&fh);
  if (rc != 0)
  {
    printf("ipacls_open() = %s",strerror(rc));
    return EXIT_FAILURE;
  }
  
  while(true)
  {
    struct sockaddr_un remote;
    struct ucred       cred;
    addr__t            addr;
    unsigned int       protocol;
    
    rc = ipacls_read_request(fh,&remote,&cred,&addr.sa,&protocol);
    if (rc != 0)
    {
      printf("ipacls_read_request() = %s",strerror(rc));
      continue;
    }
    
    printf("request---sending error\n");
    
    rc = ipacls_send_err(fh,&remote,EPERM);
    if (rc != 0)
    {
      printf("ipacls_send_err() = %s",strerror(rc));
      continue;
    }
  }
  
  return EXIT_SUCCESS;
}
