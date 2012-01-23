
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

#include "ipacl-proto.h"
#include "ipacl.h"

#define IPACL_SOCK	"ipacl:sock"
#define NET_ADDR        "net:addr"

/**********************************************************************/

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

typedef struct ipaclsock
{
  int fh;
} ipaclsock__t;

/************************************************************************/

static int	ipacllua_request	(lua_State *const);
static int	ipacllua_open		(lua_State *const);

static int	ipacllua___tostring_c	(lua_State *const);
static int	ipacllua_do_request	(lua_State *const);
static int	ipacllua_close		(lua_State *const);

/************************************************************************/

static const luaL_reg mipacl_reg[] =
{
  { "request"		, ipacllua_request	} ,
  { "open"		, ipacllua_open		} ,
  { NULL		, NULL			}
};

static const luaL_reg mipacl_csockmeta[] =
{
  { "__tostring"	, ipacllua___tostring_c	} ,
  { "__gc"		, ipacllua_close	} ,
  { "do_request"	, ipacllua_do_request	} ,
  { "close"		, ipacllua_close	} ,
  { NULL		, NULL			}
};

/************************************************************************/

static int ipacllua_request(lua_State *const L)
{
  int fh;
  int rc;
  
  if (lua_isuserdata(L,1))
  {
    sockaddr_all__t *addr = luaL_checkudata(L,1,NET_ADDR);
    
    if ((addr->sa.sa_family != AF_INET) && (addr->sa.sa_family != AF_INET6))
      return luaL_error(L,"incorrect parameter type");
    
    if (lua_isnumber(L,2))
    {
      unsigned int proto = lua_tonumber(L,2);
      rc = ipacl_request_addr(&fh,&addr->sa,proto);
    }
    else if (lua_isstring(L,2))
    {
      struct protoent *e = getprotobyname(lua_tostring(L,2));
      if (e == NULL)
      {
        lua_pushnil(L);
        lua_pushinteger(L,ENOPROTOOPT);
        return 2;
      }
      rc = ipacl_request_addr(&fh,&addr->sa,e->p_proto);
    }
    else
      return luaL_error(L,"incorrect parameter type"); 
  }
  
  else if (lua_isstring(L,1))
  {
    const char *ip = lua_tostring(L,1);
    
    if (lua_isnumber(L,2) && lua_isnumber(L,3))
    {
      unsigned int proto = lua_tonumber(L,2);
      unsigned int port  = lua_tonumber(L,3);
      rc = ipacl_request(&fh,ip,proto,port);
    }
    else if (lua_isstring(L,2) && lua_isstring(L,3))
    {
      const char *proto = lua_tostring(L,2);
      const char *port  = lua_tostring(L,3);
      rc = ipacl_request_s(&fh,ip,proto,port);
    }
    else
      return luaL_error(L,"incorrect paramter types");    
  }
  else
    return luaL_error(L,"incorrect parameter type");
  
  if (rc == 0)
    lua_pushinteger(L,fh);
  else
    lua_pushnil(L);
  
  lua_pushinteger(L,rc);
  return 2;  
}

/************************************************************************/

static int ipacllua_open(lua_State *const L)
{
  ipaclsock__t *sock;
  int           rc;
  
  sock = lua_newuserdata(L,sizeof(ipaclsock__t));
  rc   = ipacl_open(&sock->fh);
  
  if (rc == 0)
  {
    luaL_getmetatable(L,IPACL_SOCK);
    lua_setmetatable(L,-2);
  }
  else
    lua_pushnil(L);
  
  lua_pushinteger(L,rc);
  return 2;
}

/************************************************************************/

static int ipacllua___tostring_c(lua_State *const L)
{
  ipaclsock__t *sock;
  
  sock = luaL_checkudata(L,1,IPACL_SOCK);
  lua_pushfstring(L,"IPACL-CLIENT:%d",sock->fh);
  return 1;
}

/************************************************************************/

static int ipacllua_do_request(lua_State *const L)
{
  ipaclsock__t *sock;
  int           fh;
  int           rc;
  
  sock = luaL_checkudata(L,1,IPACL_SOCK);
  
  if (lua_isuserdata(L,2))
  {
    sockaddr_all__t *addr = luaL_checkudata(L,2,NET_ADDR);
    
    if ((addr->sa.sa_family != AF_INET) && (addr->sa.sa_family != AF_INET6))
      return luaL_error(L,"incorrect parameter type");
    
    if (lua_isnumber(L,3))
    {
      unsigned int proto = lua_tonumber(L,3);
      rc = ipacl_do_request_addr(sock->fh,&fh,&addr->sa,proto);
    }
    else if (lua_isstring(L,3))
    {
      struct protoent *e = getprotobyname(lua_tostring(L,2));
      if (e == NULL)
      {
        lua_pushnil(L);
        lua_pushinteger(L,ENOPROTOOPT);
        return 2;
      }
      rc = ipacl_do_request_addr(sock->fh,&fh,&addr->sa,e->p_proto);
    }
    else
      return luaL_error(L,"incorrect parameter type");
  }
  
  else if (lua_isstring(L,2))
  {
    const char *ip = lua_tostring(L,2);
    
    if (lua_isnumber(L,3) && lua_isnumber(L,4))
    {
      unsigned int proto = lua_tonumber(L,3);
      unsigned int port  = lua_tonumber(L,4);
      rc = ipacl_do_request(sock->fh,&fh,ip,proto,port);
    }
    else if (lua_isstring(L,3) && lua_isstring(L,4))
    {
      const char *proto = lua_tostring(L,3);
      const char *port  = lua_tostring(L,4);
      rc = ipacl_do_request_s(sock->fh,&fh,ip,proto,port);
    }
    else
      return luaL_error(L,"incorrect parameter types");
  }
  else
    return luaL_error(L,"incorrect parameter type");
  
  if (rc == 0)
    lua_pushinteger(L,fh);
  else
    lua_pushnil(L);
  
  lua_pushinteger(L,rc);
  return 2;
}

/************************************************************************/

static int ipacllua_close(lua_State *const L)
{
  ipaclsock__t *sock;
  
  sock = (ipaclsock__t *)luaL_checkudata(L,1,IPACL_SOCK);
  lua_pushinteger(L,ipacl_close(sock->fh));
  return 1;
}

/************************************************************************/

int luaopen_org_conman_net_ipacl(lua_State *const L)
{
  luaL_newmetatable(L,IPACL_SOCK);
  luaL_register(L,NULL,mipacl_csockmeta);
  lua_pushvalue(L,-1);
  lua_setfield(L,-2,"__index");
  
  luaL_register(L,"org.conman.net.ipacl",mipacl_reg);
  return 1;
}

/************************************************************************/

