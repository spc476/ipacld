
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
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
  
  umask(011);
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
