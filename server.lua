#!/usr/local/bin/lua

syslog = require "org.conman.syslog"
net    = require "org.conman.net"
fsys   = require "org.conman.fsys"
unix   = require "org.conman.unix"
errno  = require "org.conman.errno"
proc   = require "org.conman.process"
sig    = require "org.conman.process".sig
         require "readacl"

syslog.open('IPport','daemon')
syslog('notice',"IPport starting up")
local g_null = fsys.open("/dev/null","rw")
local g_sock

-- *********************************************************************

function log_success(remote,cred,addr,stype)
  syslog('notice',string.format("success: rem=%s uid=%s gid=%s family=%s addr=%s port=%s type=%s",
  		tostring(remote),
      		cred.uid,
      		cred.gid,
      		addr.family,
      		addr.addr,
      		addr.port,
      		stype))      		
end

-- *********************************************************************

function log_failure(remote,cred,addr,stype,err)
  syslog('err',string.format("failure: err=%q rem=%s uid=%s gid=%s family=%s addr=%s port=%s type=%s",
  		errno.strerror(err),
  		tostring(remote),
      		cred.uid,
      		cred.gid,
      		addr.family,
      		addr.addr,
      		addr.port,
      		stype))
end

-- *********************************************************************

function main()
  local remote
  local data
  local cred
  local err
  local addr
  local stype
  local sock
  local _
  
  remote,data,cred,err = readcred(g_sock)
  if err ~= 0 then
    if err == errno.EINTR then
      g_sock:close()
      os.remove("/tmp/acl")
      os.exit(1)
    end
    syslog('crit',string.format("readcred() = %s",errno.strerror(err)))
    return main()
  end
  
  if unix.users[cred.uid] ~= nil then
    cred.uid = unix.users[cred.uid].userid
  end
  
  if unix.groups[cred.gid] ~= nil then
    cred.gid = unix.groups[cred.gid].name
  end
  
  addr,stype,err = acl_decode(data)
  if err ~= 0 then
    log_failure(remote,cred,addr,stype,err)
    sendfd(g_sock,remote,g_null:fd(),err)
    return main()
  end
  
  sock,err = net.socket(addr.family,stype)
  if err ~= 0 then
    log_failure(remote,cred,addr,stype,err)
    sendfd(g_sock,remote,g_null:fd(),err)
    return main()
  end
  
  err = sock:bind(addr)
  if err ~= 0 then
    log_failure(remote,cred,addr,stype,err)
    sendfd(g_sock,remote,g_null:fd(),err)
    sock:close()
    return main()
  end

  log_success(remote,cred,addr,stype)  
  sendfd(g_sock,remote,sock:fd())
  sock:close()
  return main()
end

-- *************************************************************************  

sig.catch(sig.INT)
fsys.umask("--x--x--x")
os.remove("/tmp/acl")
laddr = net.address("/tmp/acl")
g_sock  = net.socket(laddr.family,'udp')
g_sock:bind(laddr)
recvcred(g_sock)

main()
