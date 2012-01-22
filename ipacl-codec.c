
#define _BSD_SOURCE
#define _FORTIFY_SOURCE 0

#include <assert.h>
#include <string.h>
#include <errno.h>

#include <syslog.h>

#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netdb.h>

#include "ipacl-proto.h"

/*************************************************************************/

const struct sockaddr_un ipacl_port =
{
  .sun_family = AF_LOCAL,
  .sun_path   = "/dev/ipacl"
};

/*************************************************************************/

int ipacl_encode(
	ipaclraw__t         *const restrict raw,
	size_t              *const restrict rawsize,
	const ipacl_addr__t *const restrict addr,
	const unsigned int                  protocol
)
{
  assert(raw      != NULL);
  assert(rawsize  != NULL);
  assert(addr     != NULL);
  assert(protocol <= 65535u);
  
  switch(addr->sa.sa_family)
  {
    case AF_INET:
         raw->ipv4.family = IPACLF_IPv4;
         raw->ipv4.proto  = protocol;
         raw->ipv4.port   = addr->sin.sin_port;
         raw->ipv4.rsvp   = 0;
         raw->ipv4.addr   = addr->sin.sin_addr.s_addr;
         *rawsize         = sizeof(ipaclraw_ipv4__t);
         return 0;
         
    case AF_INET6:
         raw->ipv6.family = IPACLF_IPv6;
         raw->ipv6.proto  = protocol;
         raw->ipv6.port   = addr->sin6.sin6_port;
         raw->ipv6.rsvp   = 0;
         memcpy(raw->ipv6.addr,addr->sin6.sin6_addr.s6_addr,16);
         *rawsize = sizeof(ipaclraw_ipv6__t);
         return 0;
         
    default:
         assert(0);
         return EINVAL;
  }  
}

/*************************************************************************/

int ipacl_decode(
	ipacl_addr__t     *const restrict addr,
	size_t		  *const restrict addrsize,
	unsigned int      *const restrict pprotocol,
	const ipaclraw__t *const restrict raw,
	const size_t                      rawsize
)
{
  assert(addr      != NULL);
  assert(addrsize  != NULL);
  assert(pprotocol != NULL);
  assert(raw       != NULL);

  switch(raw->head.family)
  {
    case IPACLF_IPv4:
         if (rawsize < sizeof(ipaclraw_ipv4__t))
           return EINVAL;
         
         *addrsize                 = sizeof(addr->sin);
         *pprotocol                = raw->ipv4.proto;
         addr->sin.sin_family      = AF_INET;
         addr->sin.sin_port        = raw->ipv4.port;
         addr->sin.sin_addr.s_addr = raw->ipv4.addr;
         return 0;
         
    case IPACLF_IPv6:
         if (rawsize < sizeof(ipaclraw_ipv6__t))
           return EINVAL;
         
         *addrsize              = sizeof(addr->sin6);
         *pprotocol             = raw->ipv6.proto;
         addr->sin6.sin6_family = AF_INET6;
         addr->sin6.sin6_port   = raw->ipv6.port;
         memcpy(addr->sin6.sin6_addr.s6_addr,raw->ipv6.addr,16);
         return 0;

    default:
         return EINVAL;
  }
}

/**********************************************************************/

