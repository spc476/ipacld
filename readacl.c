
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

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#define NET_SOCK	"net:sock"
#define NET_ADDR	"net:addr"

#ifdef __SunOS
#  define AF_LOCAL	AF_UNIT
#  define SUN_LEN(x)	sizeof(struct sockaddr_un)
#endif

/*************************************************************************/

typedef enum aclfamily
{
  ACLF_IPv4,
  ACLF_IPv6,
} aclfamily__t;

	/*----------------------------------------------------------------
	; NOTE: None of the fields need to be sent in network byte order,
	; 	since the packets never go out the actual network, but to
	; 	another process on the same system.
	;----------------------------------------------------------------*/

#ifdef __SunOS
#  pragma pack(1)
#endif

typedef struct aclraw_head
{
  uint16_t family;	/* ip, ipv6 */
  uint16_t proto;	/* Protocol to run on top of IP */
} __attribute__((packed)) aclraw_head__t;

typedef struct aclraw_ipv4
{
  uint16_t family;
  uint16_t proto;
  uint16_t port;	/* if applicable for proto */
  uint16_t rsvp;
  uint32_t addr;
} __attribute__((packed)) aclraw_ipv4__t;

typedef struct aclraw_ipv6
{
  uint16_t family;
  uint16_t proto;
  uint16_t port;	/* if applicable for proto */
  uint16_t rsvp;
  uint8_t  addr[16];
} __attribute__((packed)) aclraw_ipv6__t;

typedef union aclraw
{
  aclraw_head__t head;
  aclraw_ipv4__t ipv4;
  aclraw_ipv6__t ipv6;
} __attribute__((packed)) aclraw__t;

typedef struct aclrep
{
  uint32_t err;
} __attribute__((packed)) aclrep__t;

#ifdef __SunOS
#  pragma pack
#endif

	/*----------------------------------------------------------
	; the follow structures are defined as for the Lua module
	; org.conman.net.  Not all fields are used here.
	;-----------------------------------------------------------*/

typedef union sockaddr_all
{
  struct sockaddr     sa;
  struct sockaddr_in  sin;
  struct sockaddr_in6 sin6;
  struct sockaddr_un  ssun; /* not used */
} sockaddr_all__t;

typedef struct sock
{
  int fh;
} sock__t;

/************************************************************************/

static int socklua_acl_encode(lua_State *const L)
{
  sockaddr_all__t *addr;
  aclraw__t        raw;
  size_t           size;
  int              proto;
  
  addr = luaL_checkudata(L,1,NET_ADDR);
  
  if (lua_isnumber(L,2))
    proto = lua_tointeger(L,2);
  else if (lua_isstring(L,2))
  {
    struct protoent *e = getprotobyname(lua_tostring(L,2));
    if (e == NULL)
    {
      lua_pushnil(L);
      lua_pushinteger(L,ENOPROTOOPT);
      return 2;
    }
    proto = e->p_proto;
  }
  
  switch(addr->sa.sa_family)
  {
    case AF_INET:
         size            = sizeof(aclraw_ipv4__t);
         raw.ipv4.family = ACLF_IPv4;
         raw.ipv4.proto  = proto;
         raw.ipv4.port   = addr->sin.sin_port;
         raw.ipv4.rsvp   = 0;
         raw.ipv4.addr   = addr->sin.sin_addr.s_addr;
         break;
         
    case AF_INET6:
         size            = sizeof(aclraw_ipv6__t);
         raw.ipv6.family = ACLF_IPv6;
         raw.ipv6.proto  = proto;
         raw.ipv6.port   = addr->sin6.sin6_port;
         raw.ipv6.rsvp   = 0;
         memcpy(raw.ipv6.addr,addr->sin6.sin6_addr.s6_addr,16);
         break;
         
    default:
    	 return luaL_error(L,"family not supported for this code");
  }
  
  lua_pushlstring(L,(char *)&raw,size);
  lua_pushinteger(L,0);
  return 2;
}

/*************************************************************************/

static int socklua_acl_decode(lua_State *const L)
{
  aclraw__t       *raw;
  size_t           size;
  sockaddr_all__t *addr;
  struct protoent *ent;
  int              proto;
  
  raw   = (aclraw__t *)luaL_checklstring(L,1,&size);
  proto = raw->head.proto;  
  addr  = lua_newuserdata(L,sizeof(sockaddr_all__t));  

  switch(raw->head.family)
  {
    case ACLF_IPv4:
         if (size < sizeof(aclraw_ipv4__t))
         {
           lua_pushnil(L);
           lua_pushnil(L);
           lua_pushinteger(L,EPROTO);
           return 3;
         }
         
         addr->sa.sa_family        = AF_INET;
         addr->sin.sin_port        = raw->ipv4.port;
         addr->sin.sin_addr.s_addr = raw->ipv4.addr;
         break;
         
    case ACLF_IPv6:
         if (size < sizeof(aclraw_ipv6__t))
         {
           lua_pushnil(L);
           lua_pushnil(L);
           lua_pushinteger(L,EPROTO);
           return 3;
         }
         
         addr->sa.sa_family   = AF_INET6;
         addr->sin6.sin6_port = raw->ipv6.port;
         memcpy(addr->sin6.sin6_addr.s6_addr,raw->ipv6.addr,16);
         break;
         
    default:
         lua_pushnil(L);
         lua_pushnil(L);
         lua_pushinteger(L,EPROTO);
         return 3;
  }

  luaL_getmetatable(L,NET_ADDR);
  lua_setmetatable(L,-2);

  ent = getprotobynumber(proto);
  if (ent == NULL)
    lua_pushinteger(L,proto);
  else
    lua_pushstring(L,ent->p_name);

  lua_pushinteger(L,0);
  return 3;
}

/************************************************************************/

static int sockacl_recvcred(lua_State *const L)
{
  sock__t *sock;
  int      optval;
  
  sock   = luaL_checkudata(L,1,NET_SOCK);
  optval = 1;

  if (setsockopt(sock->fh,SOL_SOCKET,SO_PASSCRED,&optval,sizeof(optval)) == -1)
  {
    int err = errno;
    lua_pushboolean(L,false);
    lua_pushinteger(L,err);
  }
  else
  {
    lua_pushboolean(L,true);
    lua_pushinteger(L,0);
  }

  return 2;
}

/**************************************************************************/

static int socklua_readcred(lua_State *const L)
{
  sock__t         *sock;
  sockaddr_all__t *remaddr;
  struct pollfd    fdlist;
  struct msghdr    msg;
  struct cmsghdr  *cmsg;
  struct ucred    *cred;
  struct iovec     iovec;
  char             buffer[65535uL];
  ssize_t          bytes;
  int              timeout;
  int              rc;
  union
  {
    struct cmsghdr cmsg;
    char           data[CMSG_SPACE(sizeof(struct ucred))];
  } control;
  
  sock = luaL_checkudata(L,1,NET_SOCK);
  fdlist.events = POLLIN;
  fdlist.fd     = sock->fh;
  
  if (lua_isnoneornil(L,2))
    timeout = -1;
  else
    timeout = (int)(lua_tonumber(L,2) * 1000.0);
  
  rc = poll(&fdlist,1,timeout);
  if (rc < 1)
  {
    int err = (rc == 0) ? ETIMEDOUT : errno;
    lua_pushnil(L);
    lua_pushnil(L);
    lua_pushnil(L);
    lua_pushinteger(L,err);
    return 4;
  }
  
  remaddr = lua_newuserdata(L,sizeof(sockaddr_all__t));
  luaL_getmetatable(L,NET_ADDR);
  lua_setmetatable(L,-2);
  
  memset(&msg,0,sizeof(struct msghdr));
  memset(&control,0,sizeof(control));
  memset(&iovec,0,sizeof(struct iovec));
  
  control.cmsg.cmsg_len   = CMSG_LEN(sizeof(struct ucred));
  control.cmsg.cmsg_level = SOL_SOCKET;
  control.cmsg.cmsg_type  = SCM_CREDENTIALS;
  iovec.iov_base          = buffer;
  iovec.iov_len           = sizeof(buffer);
  msg.msg_control         = control.data;
  msg.msg_controllen      = sizeof(control.data);
  msg.msg_iov             = &iovec;
  msg.msg_iovlen          = 1;
  msg.msg_name            = &remaddr->sa;
  msg.msg_namelen         = sizeof(sockaddr_all__t);
  
  bytes = recvmsg(sock->fh,&msg,0);
  if (bytes == -1)
  {
    int err = errno;
    lua_pushnil(L);
    lua_pushnil(L);
    lua_pushnil(L);
    lua_pushinteger(L,err);
    return 4;
  }
  
  cmsg = CMSG_FIRSTHDR(&msg);
  if (
           (cmsg == NULL) 
        || (cmsg->cmsg_len   != CMSG_LEN(sizeof(struct ucred)))
        || (cmsg->cmsg_level != SOL_SOCKET)
        || (cmsg->cmsg_type  != SCM_CREDENTIALS)
      )
  {
    lua_pushnil(L);
    lua_pushnil(L);
    lua_pushnil(L);
    lua_pushinteger(L,EINVAL);
    return 4;
  }
  
  cred = (struct ucred *)CMSG_DATA(cmsg);
  
  lua_pushlstring(L,buffer,bytes);
  lua_createtable(L,0,3);
  lua_pushinteger(L,cred->pid);
  lua_setfield(L,-2,"pid");
  lua_pushinteger(L,cred->uid);
  lua_setfield(L,-2,"uid");
  lua_pushinteger(L,cred->gid);
  lua_setfield(L,-2,"gid");
  
  lua_pushinteger(L,0);
  return 4;
}

/*************************************************************************/

static int socklua_senderr(lua_State *const L)
{
  sock__t         *sock;
  sockaddr_all__t *remaddr;
  aclrep__t        reply;
  ssize_t          bytes;
  
  sock      = luaL_checkudata(L,1,NET_SOCK);
  remaddr   = luaL_checkudata(L,2,NET_ADDR);
  reply.err = luaL_checkinteger(L,3);
  
  assert(remaddr->sa.sa_family == AF_UNIX);
  
  bytes = sendto(sock->fh,&reply,sizeof(reply),0,&remaddr->sa,sizeof(struct sockaddr_un));
  if (bytes < (ssize_t)sizeof(reply))
  {
    int err = (bytes < 0) ? errno : 0;
    lua_pushboolean(L,false);
    lua_pushinteger(L,err);
  }
  else
  {
    lua_pushboolean(L,true);
    lua_pushinteger(L,0);
  }
  return 2;
}

/*************************************************************************/

static int socklua_sendfd(lua_State *const L)
{
  sock__t         *sock;
  sockaddr_all__t *remaddr;
  struct msghdr    msg;
  int              fh;
  int              err;
  struct iovec     iovec;
  ssize_t          bytes;
  union
  {
    struct cmsghdr cmsg;
    char           data[CMSG_SPACE(sizeof(int))];
  } control;
  
  sock    = luaL_checkudata(L,1,NET_SOCK);
  remaddr = luaL_checkudata(L,2,NET_ADDR);
  fh      = luaL_checkinteger(L,3);
  err     = luaL_optint(L,4,0);
  
  assert(remaddr->sa.sa_family == AF_UNIX);
  
  memset(&msg,    0,sizeof(msg));
  memset(&control,0,sizeof(control));
  memset(&iovec,  0,sizeof(iovec));
  
  msg.msg_control         = control.data,
  msg.msg_controllen      = sizeof(control.data);
  msg.msg_iov             = &iovec;
  msg.msg_iovlen          = 1;
  msg.msg_name            = &remaddr->sa;
  msg.msg_namelen         = sizeof(struct sockaddr_un);
  control.cmsg.cmsg_len   = CMSG_LEN(sizeof(int));
  control.cmsg.cmsg_level = SOL_SOCKET;
  control.cmsg.cmsg_type  = SCM_RIGHTS;
  *((int *)CMSG_DATA(CMSG_FIRSTHDR(&msg))) = fh;
  iovec.iov_base          = &err;
  iovec.iov_len           = sizeof(int);
  
  bytes = sendmsg(sock->fh,&msg,0);
  if (bytes < 0)
  {
    int err = errno;
    lua_pushboolean(L,false);
    lua_pushinteger(L,err);
  }
  else
  {
    lua_pushboolean(L,true);
    lua_pushinteger(L,0);
  }
  
  return 2;
}

/*************************************************************************/

static int socklua_readfd(lua_State *const L)
{
  sock__t         *sock;
  sockaddr_all__t *remaddr;
  struct pollfd    fdlist;
  struct msghdr    msg;
  struct cmsghdr  *cmsg;
  struct iovec     iovec;
  aclrep__t       *rep;
  int              fh;
  char             buffer[65535uL];
  ssize_t          bytes;
  int              timeout;
  int              rc;
  union
  {
    struct cmsghdr cmsg;
    char           data[CMSG_SPACE(sizeof(int))];
  } control;
  
  sock = luaL_checkudata(L,1,NET_SOCK);
  fdlist.events = POLLIN;
  fdlist.fd     = sock->fh;
  
  if (lua_isnoneornil(L,2))
    timeout = -1;
  else
    timeout = (int)(lua_tonumber(L,2) * 1000.0);
  
  rc = poll(&fdlist,1,timeout);
  if (rc < 1)
  {
    int err = (rc == 0) ? ETIMEDOUT : errno;
    lua_pushnil(L);
    lua_pushnil(L);
    lua_pushinteger(L,err);
    return 3;
  }
  
  remaddr = lua_newuserdata(L,sizeof(sockaddr_all__t));
  luaL_getmetatable(L,NET_ADDR);
  lua_setmetatable(L,-2);
  
  memset(&msg,0,sizeof(msg));
  memset(&control,0,sizeof(control));
  memset(&iovec,0,sizeof(iovec));
  
  control.cmsg.cmsg_len   = CMSG_LEN(sizeof(int));
  control.cmsg.cmsg_level = SOL_SOCKET;
  control.cmsg.cmsg_type  = SCM_RIGHTS;
  iovec.iov_base          = buffer;
  iovec.iov_len           = sizeof(buffer);
  msg.msg_control         = control.data;
  msg.msg_controllen      = sizeof(control.data);
  msg.msg_iov             = &iovec;
  msg.msg_iovlen          = 1;
  msg.msg_name            = &remaddr->sa;
  msg.msg_namelen         = sizeof(sockaddr_all__t);
  
  bytes = recvmsg(sock->fh,&msg,0);
  if (bytes == -1)
  {
    int err = errno;
    lua_pushnil(L);
    lua_pushnil(L);
    lua_pushinteger(L,err);
    return 3;
  }
  
  rep = (aclrep__t *)buffer;
  if (rep->err != 0)
  {
    lua_pushnil(L);
    lua_pushnil(L);
    lua_pushinteger(L,rep->err);
    return 3;
  }
  
  cmsg = CMSG_FIRSTHDR(&msg);
  
  if (
          (cmsg             == NULL)
       || (cmsg->cmsg_len   != CMSG_LEN(sizeof(int)))
       || (cmsg->cmsg_level != SOL_SOCKET)
       || (cmsg->cmsg_type  != SCM_RIGHTS)
     )
  {
    lua_pushnil(L);
    lua_pushinteger(L,EINVAL);
    return 3;
  }
  
  fh = *((int *)CMSG_DATA(cmsg));
  if (fh < 0)
  {
    lua_pushnil(L);
    lua_pushinteger(L,EBADF);
    return 3;
  }
  
  if (bytes < (ssize_t)sizeof(aclrep__t))
  {
    lua_pushinteger(L,fh);
    lua_pushinteger(L,EPROTO);
    return 3;
  }
  
  lua_pushinteger(L,fh);
  lua_pushinteger(L,0);
  return 3;
}

/*************************************************************************/

int luaopen_readacl(lua_State *const L)
{
  lua_pushcfunction(L,sockacl_recvcred);
  lua_setglobal(L,"recvcred");
  lua_pushcfunction(L,socklua_readcred);
  lua_setglobal(L,"readcred");
  lua_pushcfunction(L,socklua_acl_encode);
  lua_setglobal(L,"acl_encode");
  lua_pushcfunction(L,socklua_acl_decode);
  lua_setglobal(L,"acl_decode");
  lua_pushcfunction(L,socklua_senderr);
  lua_setglobal(L,"senderr");
  lua_pushcfunction(L,socklua_sendfd);
  lua_setglobal(L,"sendfd");
  lua_pushcfunction(L,socklua_readfd);
  lua_setglobal(L,"readfd");
  return 0;
}

