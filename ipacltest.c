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
#include <stdlib.h>
#include <string.h>

#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/poll.h>
#include <netdb.h>
#include <unistd.h>

#include "ipacl.h"

/************************************************************************/

int main(int argc,char *argv[])
{
  int fh;
  int rc;
  
  if (argc < 4)
  {
    fprintf(stderr,"usage: %s ipaddr protocol port\n",argv[0]);
    return EXIT_FAILURE;
  }
  
  rc = ipacl_request_s(&fh,argv[1],argv[2],argv[3]);
  if (rc != 0)
  {
    fprintf(stderr,"failed: %s\n",strerror(rc));
    return EXIT_FAILURE;
  }
  
  printf("success\n");
  fgetc(stdin);
  return EXIT_SUCCESS;
}

