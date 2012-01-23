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
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/poll.h>
#include <netdb.h>
#include <unistd.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "ipacl-proto.h"
#include "ipacl-server.h"

#define IPACL_SERVER	"ipacl:server"
#define NET_SOCK	"net:sock"
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

typedef struct sock
{
  int fh;
} sock__t;

typedef struct ipaclsock
{
  int fh;
} ipaclsock__t;

/************************************************************************/

static int	ipaclluas_open		(lua_State *const);

static int	ipaclluas___tostring	(lua_State *const);
static int	ipaclluas_read		(lua_State *const);
static int	ipaclluas_send_err	(lua_State *const);
static int	ipaclluas_send_fd	(lua_State *const);
static int	ipaclluas_close		(lua_State *const);

/************************************************************************/

static const luaL_reg mipacls_reg[] =
{
  { "open"		, ipaclluas_open	} ,
  { NULL		, NULL			}
};

static const luaL_reg mipacl_servermeta[] =
{
  { "__tostring"	, ipaclluas___tostring	} ,
  { "__gc"		, ipaclluas_close	} ,
  { "read"		, ipaclluas_read	} ,
  { "send_err"		, ipaclluas_send_err	} ,
  { "send_fd"		, ipaclluas_send_fd	} ,
  { "close"		, ipaclluas_close	} ,
  { NULL		, NULL			}
};

/************************************************************************/

static int ipaclluas_open(lua_State *const L)
{
  ipaclsock__t *sock;
  int           rc;
  
  sock = lua_newuserdata(L,sizeof(ipaclsock__t));
  rc   = ipacls_open(&sock->fh);
  
  if (rc == 0)
  {
    luaL_getmetatable(L,IPACL_SERVER);
    lua_setmetatable(L,-2);
  }
  else
    lua_pushnil(L);
  
  lua_pushinteger(L,rc);
  return 2;
}

/************************************************************************/

static int ipaclluas___tostring(lua_State *const L)
{
  ipaclsock__t *sock;
  
  sock = luaL_checkudata(L,1,IPACL_SERVER);
  lua_pushfstring(L,"IPACL-SERVER:%d",sock->fh);
  return 1;
}

/************************************************************************/

static int ipaclluas_read(lua_State *const L)
{
  ipaclsock__t    *sock;
  struct ucred     cred;
  sockaddr_all__t *remote;
  sockaddr_all__t *addr;
  struct protoent *e;
  unsigned int     proto;
  int              rc;
  
  sock   = (ipaclsock__t *)luaL_checkudata(L,1,IPACL_SERVER);
  remote = lua_newuserdata(L,sizeof(sockaddr_all__t));
  lua_createtable(L,0,3);
  addr   = lua_newuserdata(L,sizeof(sockaddr_all__t));
  rc     = ipacls_read_request(sock->fh,&remote->ssun,&cred,&addr->sa,&proto);

  if (rc != 0)
  {
    lua_pushnil(L);
    lua_pushnil(L);
    lua_pushnil(L);
    lua_pushnil(L);
    lua_pushinteger(L,rc);
    return 5;
  }
  
  luaL_getmetatable(L,NET_ADDR);
  lua_setmetatable(L,-4);
  luaL_getmetatable(L,NET_ADDR);
  lua_setmetatable(L,-2);
  
  lua_pushinteger(L,cred.pid);
  lua_setfield(L,-3,"pid");
  lua_pushinteger(L,cred.uid);
  lua_setfield(L,-3,"uid");
  lua_pushinteger(L,cred.gid);
  lua_setfield(L,-3,"gid");
  
  e = getprotobynumber(proto);
  if (e == NULL)
    lua_pushinteger(L,proto);
  else
    lua_pushstring(L,e->p_name);

  lua_pushinteger(L,0);
  return 5;
}

/************************************************************************/

static int ipaclluas_send_err(lua_State *const L)
{
  ipaclsock__t    *sock;
  sockaddr_all__t *remote;
  int              err;
  int              rc;
  
  sock   = luaL_checkudata(L,1,IPACL_SERVER);
  remote = luaL_checkudata(L,2,NET_ADDR);
  err    = luaL_checkinteger(L,3);
  rc     = ipacls_send_err(sock->fh,&remote->ssun,err);
  
  lua_pushboolean(L,rc == 0);
  lua_pushinteger(L,rc);
  return 2;
}

/************************************************************************/

static int ipaclluas_send_fd(lua_State *const L)
{
  ipaclsock__t    *sock;
  sockaddr_all__t *remote;
  int              fh;
  int              rc;
  
  sock   = luaL_checkudata(L,1,IPACL_SERVER);
  remote = luaL_checkudata(L,2,NET_ADDR);
  
  if (lua_isnumber(L,3))
    fh = lua_tointeger(L,3);
  else if (lua_isuserdata(L,3))
  {
    sock__t *s = luaL_checkudata(L,3,NET_SOCK);
    fh = s->fh;  
  }
  else
    return luaL_error(L,"wrong type of file descriptor");

  rc = ipacls_send_fd(sock->fh,&remote->ssun,fh);

  lua_pushboolean(L,rc == 0);
  lua_pushinteger(L,rc);
  return 2;
}

/************************************************************************/

static int ipaclluas_close(lua_State *const L)
{
  ipaclsock__t *sock;
  
  sock = luaL_checkudata(L,1,IPACL_SERVER);
  lua_pushinteger(L,ipacls_close(sock->fh));
  return 1;
}

/************************************************************************/

int luaopen_org_conman_net_ipacl_s(lua_State *const L)
{
  luaL_newmetatable(L,IPACL_SERVER);
  luaL_register(L,NULL,mipacl_servermeta);
  lua_pushvalue(L,-1);
  lua_setfield(L,-2,"__index");
  
  luaL_register(L,"org.conman.net.ipacl_s",mipacls_reg);
  return 1;
}

/************************************************************************/

