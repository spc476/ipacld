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
  mode_t om;
  int    optval = 1;
  
  assert(pfh != NULL);
   
  remove(mipacl_port.sun_path);

  *pfh = socket(AF_LOCAL,SOCK_DGRAM,0);
  if (*pfh == -1)
    return errno;

  om = umask(0111);
  if (bind(*pfh,(struct sockaddr *)&mipacl_port,sizeof(mipacl_port)) < 0)
  {
    umask(om);
    int err = errno;
    close(*pfh);
    *pfh = -1;
    return err;
  }

  umask(om); 

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

  if (raw->head.type != IPACLT_IP)
    return EINVAL;
  
  switch(raw->net.sa.sa_family)
  {
    case AF_INET:
         if (rawsize < sizeof(ipaclraw_ipv4__t))
           return EINVAL;
         *addrsize = sizeof(struct sockaddr_in);
         break;
    case AF_INET6:
         if (rawsize < sizeof(ipaclraw_ipv6__t))
           return EINVAL;
         *addrsize = sizeof(struct sockaddr_in6);
         break;
    default:
         return EINVAL;
  }
  
  if (raw->net.protocol > 65535u)
    return EINVAL;
    
  *pprotocol = raw->net.protocol;
  memcpy(addr,&raw->net.sa,*addrsize);
  return 0;
}

/**********************************************************************/

