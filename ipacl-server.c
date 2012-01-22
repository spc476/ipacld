
#define _BSD_SOURCE
#define _FORTIFY_SOURCE 0

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <errno.h>
#include <assert.h>

#include <syslog.h>

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

/************************************************************************/

static int decode(
		struct sockaddr   *const restrict addr,
		size_t            *const restrict addrsize,
		unsigned int      *const restrict pprotocol,
		const ipaclraw__t *const restrict raw,
		const size_t                      rawsize
	) __attribute__((nonnull,nothrow));

/************************************************************************/

static const struct sockaddr_un mipacl_port =
{
  .sun_family = AF_LOCAL,
  .sun_path   = "/dev/ipacl"
};

/***********************************************************************/

int ipacls_open(int *const pfh)
{
  int optval = 1;
  
  assert(pfh != NULL);
  
  remove(mipacl_port.sun_path);

  *pfh = socket(AF_LOCAL,SOCK_DGRAM,0);
  if (*pfh == -1)
    return errno;
  if (bind(*pfh,(struct sockaddr *)&mipacl_port,sizeof(mipacl_port)) < 0)
  {
    int err = errno;
    close(*pfh);
    *pfh = -1;
    return err;
  }
  
  if (setsockopt(*pfh,SOL_SOCKET,SO_PASSCRED,&optval,sizeof(optval)) == -1)
  {
    int err = errno;
    close(*pfh);
    *pfh = -1;
    return err;
  }

  return 0;
}

/**************************************************************************/

int ipacls_read_request(
		const int                          reqport,
		struct sockaddr_un *const restrict remote,
		struct ucred       *const restrict cred,
		struct sockaddr    *const restrict addr,
		unsigned int       *const restrict protocol
)
{
  struct msghdr   msg;
  struct cmsghdr *cmsg;
  struct iovec    iovec;
  ipaclraw__t     packet;
  ssize_t         bytes;
  size_t          dummy;
  union
  {
    struct cmsghdr cmsg;
    char           data[CMSG_SPACE(sizeof(struct ucred))];
  } control;
  
  assert(reqport  >= 0);
  assert(remote   != NULL);
  assert(cred     != NULL);
  assert(addr     != NULL);
  assert(protocol != NULL);
  
  memset(&control,0,sizeof(control));
  memset(&iovec,  0,sizeof(iovec));
  memset(&msg,    0,sizeof(msg));
  
  control.cmsg.cmsg_len   = CMSG_LEN(sizeof(struct ucred));
  control.cmsg.cmsg_level = SOL_SOCKET;
  control.cmsg.cmsg_type  = SCM_CREDENTIALS;
  iovec.iov_base          = &packet;
  iovec.iov_len           = sizeof(packet);
  msg.msg_control         = control.data;
  msg.msg_controllen      = sizeof(control.data);
  msg.msg_iov             = &iovec;
  msg.msg_iovlen          = 1;
  msg.msg_name            = (struct sockaddr *)remote;
  msg.msg_namelen         = sizeof(struct sockaddr_un);
  
  bytes = recvmsg(reqport,&msg,0);

  if (bytes == -1)
    return errno;
  
  cmsg = CMSG_FIRSTHDR(&msg);
  if (
          (cmsg             == NULL)
       || (cmsg->cmsg_len   != CMSG_LEN(sizeof(struct ucred)))
       || (cmsg->cmsg_level != SOL_SOCKET)
       || (cmsg->cmsg_type  != SCM_CREDENTIALS)
     )
  {
    return EINVAL;
  }
  
  memcpy(cred,CMSG_DATA(cmsg),sizeof(struct ucred));
  return decode(addr,&dummy,protocol,&packet,bytes);
}

/***********************************************************************/

int ipacls_send_err(
	const int                          reqport,
	struct sockaddr_un *const restrict remote,
	const int                          err
)
{
  ipaclrep__t reply;
  ssize_t     bytes;
  
  assert(reqport            >= 0);
  assert(remote             != NULL);
  assert(remote->sun_family == AF_LOCAL);
  assert(err                >  0);
  
  reply.err = err;
  bytes     = sendto(reqport,&reply,sizeof(reply),0,(struct sockaddr *)remote,sizeof(struct sockaddr_un));
  if (bytes == -1)
    return errno;
  if (bytes != sizeof(reply))
    return EPROTO;
  return 0;
}

/***********************************************************************/

int ipacls_send_fd(
	const int                          reqport,
	struct sockaddr_un *const restrict remote,
	const int                          fh
)
{
  struct msghdr msg;
  struct iovec  iovec;
  ipaclrep__t   reply;
  ssize_t       bytes;
  union
  {
    struct cmsghdr cmsg;
    char           data[CMSG_SPACE(sizeof(int))];
  } control;
  
  assert(reqport            >= 0);
  assert(remote             != NULL);
  assert(remote->sun_family == AF_LOCAL);
  assert(fh                 >= 0);
  
  memset(&msg,    0,sizeof(msg));
  memset(&control,0,sizeof(control));
  memset(&iovec,  0,sizeof(iovec));
  
  reply.err               = 0;
  msg.msg_control         = control.data;
  msg.msg_controllen      = sizeof(control.data);
  msg.msg_iov             = &iovec;
  msg.msg_iovlen          = 1;
  msg.msg_name            = (struct sockaddr *)remote;
  msg.msg_namelen         = sizeof(struct sockaddr_un);
  control.cmsg.cmsg_len   = CMSG_LEN(sizeof(int));
  control.cmsg.cmsg_level = SOL_SOCKET;
  control.cmsg.cmsg_type  = SCM_RIGHTS;
  *((int *)CMSG_DATA(CMSG_FIRSTHDR(&msg))) = fh;
  iovec.iov_base          = &reply;
  iovec.iov_len           = sizeof(reply);
  
  bytes = sendmsg(reqport,&msg,0);
  if (bytes == -1)
    return errno;
  
  if (bytes != sizeof(reply))
    return EPROTO;
  
  return 0;
}

/**********************************************************************/

int ipacls_close(const int reqport)
{
  assert(reqport >= 0);
  
  if (close(reqport) < 0)
    return errno;
  if (remove(mipacl_port.sun_path) < 0)
    return errno;
  return 0;
}

/*********************************************************************/

static int decode(
	struct sockaddr   *const restrict addr,
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
    case AF_INET:
         if (rawsize < sizeof(ipaclraw_ipv4__t))
           return EINVAL;
         
         *addrsize                              = sizeof(struct sockaddr_in);
         *pprotocol                             = raw->ipv4.proto;
         ((addr__t *)addr)->sin.sin_family      = AF_INET;
         ((addr__t *)addr)->sin.sin_port        = raw->ipv4.port;
         ((addr__t *)addr)->sin.sin_addr.s_addr = raw->ipv4.addr;
         return 0;
         
    case AF_INET6:
         if (rawsize < sizeof(ipaclraw_ipv6__t))
           return EINVAL;
         
         *addrsize                           = sizeof(struct sockaddr_in6);
         *pprotocol                          = raw->ipv6.proto;
         ((addr__t *)addr)->sin6.sin6_family = AF_INET6;
         ((addr__t *)addr)->sin6.sin6_port   = raw->ipv6.port;
         memcpy(((addr__t *)addr)->sin6.sin6_addr.s6_addr,raw->ipv6.addr,16);
         return 0;

    default:
         return EINVAL;
  }
}

/**********************************************************************/

