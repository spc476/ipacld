
#define _BSD_SOURCE
#define _FORTIFY_SOURCE 0

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/poll.h>
#include <netdb.h>
#include <unistd.h>

#include "ipacl-proto.h"
#include "ipacl.h"

typedef union
{
  struct sockaddr     sa;
  struct sockaddr_in  sin;
  struct sockaddr_in6 sin6;
} addr__t;

/*************************************************************************/

static int encode(
		ipaclraw__t           *const restrict raw,
		size_t                *const restrict rawsize,
		const struct sockaddr *const restrict addr,
		const unsigned int                    protocol
	) __attribute__((nonnull,nothrow));
	
/*************************************************************************/

int ipacl_request_s(
	int        *const restrict pfh,
	const char *const restrict ip,
	const char *const restrict protocol,
	const char *const restrict port
)
{
  int fh;
  int rc;
  
  assert(pfh      != NULL);
  assert(ip       != NULL);
  assert(protocol != NULL);
  assert(port     != NULL);
  
  if ((rc = ipacl_open(&fh)) != 0)
    return rc;
  rc = ipacl_do_request_s(fh,pfh,ip,protocol,port);
  ipacl_close(fh);
  return rc;
}

/************************************************************************/

int ipacl_request(
	int        *const restrict pfh,
	const char *const restrict ip,
	const unsigned int         protocol,
	const unsigned int         port
)
{
  int fh;
  int rc;
  
  assert(pfh != NULL);
  assert(ip  != NULL);
  assert(protocol <= 65535u);
  assert(port     <= 65535u);
  
  if ((rc = ipacl_open(&fh)) != 0)
    return rc;
  rc = ipacl_do_request(fh,pfh,ip,protocol,port);
  ipacl_close(fh);
  return rc;
}

/************************************************************************/

int ipacl_request_addr(
	int                   *const restrict pfh,
	const struct sockaddr *const restrict addr,
	const unsigned int                    protocol
)
{
  int fh;
  int rc;
  
  assert(pfh      != NULL);
  assert(addr     != NULL);
  assert(protocol <= 65535u);
  assert((addr->sa_family == AF_INET) || (addr->sa_family == AF_INET6));

  if ((rc = ipacl_open(&fh)) != 0)
    return rc;
  rc = ipacl_do_request_addr(fh,pfh,addr,protocol);
  ipacl_close(fh);
  return rc;
}

/*********************************************************************/

int ipacl_open(int *const pfh)
{
  struct sockaddr_un local;
  
  assert(pfh != NULL);
  
  memset(&local,0,sizeof(local));  
  snprintf(local.sun_path,sizeof(local.sun_path),"/tmp/ipacl-request.%lu",(unsigned long)getpid());
  remove(local.sun_path);

  local.sun_family = AF_LOCAL;
  *pfh = socket(AF_LOCAL,SOCK_DGRAM,0);
  
  if (*pfh == -1)
    return errno;
  
  if (bind(*pfh,(struct sockaddr *)&local,sizeof(local)) < 0)
  {
    int err = errno;
    close(*pfh);
    *pfh = -1;
    return err;
  }
  
  return 0;
}

/*********************************************************************/

int ipacl_do_request_s(
	const int                  reqport,
	int        *const restrict pfh,
	const char *const restrict ip,
	const char *const restrict protocol,
	const char *const restrict port
)
{
  struct servent  *ps;
  struct protoent *pp;
  
  assert(reqport  >= 0);
  assert(pfh      != NULL);
  assert(ip       != NULL);
  assert(protocol != NULL);
  assert(port     != NULL);
  
  ps = getservbyname(port,protocol);
  if (ps == NULL)
    return EINVAL;
  pp = getprotobyname(protocol);
  if (pp == NULL)
    return EINVAL;
  
  return ipacl_do_request(reqport,pfh,ip,pp->p_proto,ntohs(ps->s_port));
}

/************************************************************************/

int ipacl_do_request(
	const int                  reqport,
	int        *const restrict pfh,
	const char *const restrict ip,
	const unsigned int         protocol,
	const unsigned int         port
)
{
  addr__t addr;
  
  assert(reqport  >= 0);
  assert(pfh      != NULL);
  assert(ip       != NULL);
  assert(protocol <= 65535u);
  assert(port     <= 65535u);
  
  if (inet_pton(AF_INET,ip,&addr.sin.sin_addr.s_addr))
  {
    addr.sin.sin_family = AF_INET;
    addr.sin.sin_port   = htons(port);
  }
  else if (inet_pton(AF_INET6,ip,&addr.sin6.sin6_addr.s6_addr))
  {
    addr.sin6.sin6_family = AF_INET6;
    addr.sin6.sin6_port   = htons(port);
  }
  else
  {
    assert(0);
    return EINVAL;
  }
  
  return ipacl_do_request_addr(reqport,pfh,&addr.sa,protocol);
}

/********************************************************************/

int ipacl_do_request_addr(
	const int                             reqport,
	int                   *const restrict pfh,
	const struct sockaddr *const restrict addr,
	const unsigned int                    protocol
)
{
  static const struct sockaddr_un devacl = 
  {
    .sun_family = AF_LOCAL,
    .sun_path   = "/dev/ipacl"
  };
  
  ipaclraw__t     packet;
  ipaclrep__t     reply;
  struct pollfd   fdlist;
  struct msghdr   msg;
  struct cmsghdr *cmsg;
  struct iovec    iovec;
  size_t          size;
  ssize_t         bytes;
  int             rc;
  union
  {
    struct cmsghdr cmsg;
    char           data[CMSG_SPACE(sizeof(int))];
  } control;
  
  assert(reqport  >= 0);
  assert(pfh      != NULL);
  assert(addr     != NULL);
  assert(protocol <= 65535u);
  
  encode(&packet,&size,addr,protocol);  
  bytes = sendto(
  		reqport,
  		&packet,
  		size,
  		0,
  		(struct sockaddr *)&devacl,
  		sizeof(struct sockaddr_un)
  	);
  
  if (bytes < (ssize_t)size)
    return (bytes < 0) ? errno : EPROTO;
  
  fdlist.events = POLLIN;
  fdlist.fd     = reqport;
  rc = poll(&fdlist,1,1000);
  if (rc < 1)
    return errno;
  
  memset(&msg    ,0,sizeof(msg));
  memset(&control,0,sizeof(control));
  memset(&iovec  ,0,sizeof(iovec));
  
  control.cmsg.cmsg_len   = CMSG_LEN(sizeof(int));
  control.cmsg.cmsg_level = SOL_SOCKET;
  control.cmsg.cmsg_type  = SCM_RIGHTS;
  iovec.iov_base          = &reply;
  iovec.iov_len           = sizeof(reply);
  msg.msg_control         = control.data;
  msg.msg_controllen      = sizeof(control.data);
  msg.msg_iov             = &iovec;
  msg.msg_iovlen          = 1;
  msg.msg_name            = NULL;
  msg.msg_namelen         = 0;
  
  bytes = recvmsg(reqport,&msg,0);
  if (bytes == -1)
    return errno;
  
  if (bytes < (ssize_t)sizeof(reply))
    return EPROTO;
  
  if (reply.err != 0)
    return reply.err;
  
  cmsg = CMSG_FIRSTHDR(&msg);
  if (
          (cmsg             == NULL)
       || (cmsg->cmsg_len   != CMSG_LEN(sizeof(int)))
       || (cmsg->cmsg_level != SOL_SOCKET)
       || (cmsg->cmsg_type  != SCM_RIGHTS)
     )
  {
    return EINVAL;
  }
  
  *pfh = *((int *)CMSG_DATA(cmsg));
  if (*pfh < 0)
    return EBADF;
  
  return 0;
}
  
/********************************************************************/

int ipacl_close(const int reqport)
{
  char fname[FILENAME_MAX];
  
  if (close(reqport) < 0)
    return errno;
  
  snprintf(fname,sizeof(fname),"/tmp/ipacl-request.%lu",(unsigned long)getpid());
  
  if (remove(fname) < 0)
    return errno;
    
  return 0;
}

/******************************************************************/

static int encode(
	ipaclraw__t           *const restrict raw,
	size_t                *const restrict rawsize,
	const struct sockaddr *const restrict addr,
	const unsigned int                    protocol
)
{
  assert(raw      != NULL);
  assert(rawsize  != NULL);
  assert(addr     != NULL);
  assert(protocol <= 65535u);
  
  raw->net.type     = IPACLT_IP;
  raw->net.protocol = protocol;
  
  switch(addr->sa_family)
  {
    case AF_INET:
         *rawsize = sizeof(ipaclraw_ipv4__t);
         memcpy(&raw->ipv4.sin,addr,sizeof(struct sockaddr_in));
         return 0;
         
    case AF_INET6:
         *rawsize = sizeof(ipaclraw_ipv6__t);
         memcpy(&raw->ipv6.sin6,addr,sizeof(struct sockaddr_in6));
         return 0;
         
    default:
         assert(0);
         return EINVAL;
  }  
}

/*************************************************************************/
