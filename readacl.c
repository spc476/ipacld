
#define _BSD_SOURCE
#define _FORTIFY_SOURCE 0

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
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

typedef union sockaddr_all
{
  struct sockaddr sa;
  struct sockaddr_in sin;
  struct sockaddr_in6 sin6;
  struct sockaddr_un  ssun;
} sockaddr_all__t;

typedef struct sock
{
  int fh;
} sock__t;

/**************************************************************************/

static int socklua_peercred(lua_State *const L)
{
  sock__t      *sock;
  struct ucred  cred;
  size_t        credsize;
  
  sock     = luaL_checkudata(L,1,NET_SOCK);
  credsize = sizeof(cred);
  
  if (getsockopt(sock->fh,SOL_SOCKET,SO_PEERCRED,&cred,&credsize) == -1)
  {
    int err = errno;
    lua_pushnil(L);
    lua_pushinteger(L,err);
    return 2;
  }
  
  lua_createtable(L,0,3);
  lua_pushinteger(L,cred.pid);
  lua_setfield(L,-2,"pid");
  lua_pushinteger(L,cred.uid);
  lua_setfield(L,-2,"uid");
  lua_pushinteger(L,cred.gid);
  lua_setfield(L,-2,"gid");
  lua_pushinteger(L,0);
  return 2;
}

/***********************************************************************/

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

int luaopen_readacl(lua_State *const L)
{
  lua_pushcfunction(L,socklua_peercred);
  lua_setglobal(L,"peercred");
  lua_pushcfunction(L,sockacl_recvcred);
  lua_setglobal(L,"recvcred");
  lua_pushcfunction(L,socklua_readcred);
  lua_setglobal(L,"readcred");
  return 0;
}

