#!/usr/local/bin/lua

str    = require "org.conman.string"
syslog = require "org.conman.syslog"
net    = require "org.conman.net"
fsys   = require "org.conman.fsys"
unix   = require "org.conman.unix"
errno  = require "org.conman.errno"
proc   = require "org.conman.process"
sig    = require "org.conman.process".sig
         require "readacl"

dofile("acl.conf")

syslog.open('IPport','daemon')
syslog('notice',"IPport starting up")
local g_null = fsys.open("/dev/null","rw")
local g_sock
local g_acl = {}

-- *********************************************************************

function adjust_keys(tab,stype)
  local results = {}
  
  if tab == nil then
    return results
  end
  
  local function check_users(addr,list)
    local res = {}
    
    for name,value in pairs(list) do
      if unix.users[name] == nil then
        syslog('warn',string.format("%s:%s: user %q does not exist",stype,addr,name))
      else
        if type(value) == 'boolean' or type(value) == 'function' then
          res[name] = value
        else
          syslog('warn',string.format("%s:%s: user %q should be boolean or function",stype,addr,name))
        end
      end
    end
    
    return res
  end

  for name,value in pairs(tab) do
    local a,p = name:match("^(.*)%:([^%:]+)$")

    if a ~= nil and p ~= nil then
      local addr,err = net.address(a,p,stype)
      if err ~= 0 then
        syslog('warn',string.format("bad address: %q",name))
      else
        local key = string.format("%s:%d",addr.addr,addr.port)
        results[key] = check_users(key,value)
      end
    else
      syslog('warn',string.format("bad address: %q",name))
    end
  end
        
  return results
end

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
  local key
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
  else
    syslog('warn',string.format("user %d does not exist",cred.uid))
    sendfd(g_sock,remote,g_null:fd(),errno.EPERM)
    return main()
  end
  
  if unix.groups[cred.gid] ~= nil then
    cred.gid = unix.groups[cred.gid].name
  else
    syslog('warn',string.format("group %d does not exist",cred.gid))
    sendfd(g_sock,remote,g_null:fd(),errno.EPERM)
  end
  
  addr,stype,err = acl_decode(data)
  if err ~= 0 then
    log_failure(remote,cred,addr,stype,err)
    sendfd(g_sock,remote,g_null:fd(),err)
    return main()
  end

  key = string.format("%s:%s",addr.addr,addr.port)
  val = g_acl[stype]
  if val == nil then
    log_failure(remote,cred,addr,stype,errno.EINVAL)
    sendfd(g_sock,remote,g_null:fd(),errno.EINVAL)
    return main()
  end
  
  val = val[key]
  if val == nil then
    log_failure(remote,cred,addr,stype,errno.EPERM)
    sendfd(g_sock,remote,g_null:fd(),errno.EPERM)
    return main()
  end
  
  val = val[cred.uid]
  if val == nil then
    log_failure(remote,cred,addr,stype,errno.EPERM)
    sendfd(g_sock,remote,g_null:fd(),errno.EPERM)
    return main()
  end
  
  if type(val) == 'function' then 
    val = val() 
  end
  
  if not val then
    log_failure(remore,cred,addr,stype,errno.EPERM)
    sendfd(g_sock,remote,g_null:fd(),errno.EPERM)
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

function dump_addr(tab,stype)
  for name,users in pairs(tab) do
    local x = ""
    for user in pairs(users) do
      x = x .. " " .. user
    end
    print(string.format("%s:%s %s",stype,name,x))
  end
end

-- *************************************************************************

g_acl.tcp = adjust_keys(TCP,'tcp')
g_acl.udp = adjust_keys(UDP,'udp')
g_acl.raw = adjust_keys(RAW,'raw')

dump_addr(g_acl.tcp,'tcp')
dump_addr(g_acl.udp,'udp')
dump_addr(g_acl.raw,'raw')

sig.catch(sig.INT)
fsys.umask("--x--x--x")
os.remove("/tmp/acl")
laddr = net.address("/tmp/acl")
g_sock  = net.socket(laddr.family,'udp')
g_sock:bind(laddr)
recvcred(g_sock)

main()
