
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

