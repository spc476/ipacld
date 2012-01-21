
local unix = require "org.conman.unix"
local net  = require "org.conman.net"
local fsys = require "org.conman.fsys"

local function A(addr,port,proto)
  return tostring(net.address(addr,port,proto))
end

-- *****************************************************************

local users = 
{
  gopher = 
  { 
    [A('0.0.0.0'	,'gopher','tcp')] = true ,
    [A('192.168.1.10'	,'gopher','tcp')] = true
  },
  
  tftp =
  {
    [A('0.0.0.0'	, 'tftp','udp')] = true ,
    [A('192.168.1.10'	, 'tftp','udp')] = true
  },
  
  ntp =
  {
    [A('0.0.0.0' , 'daytime' , 'udp')] = true,
    [A('0.0.0.0' , 'daytime' , 'tcp')] = true
  },
  
  nobody =
  {
    [A('0.0.0.0','finger','tcp')] = true,
  }
}  

-- *************************************************************

function request_okay(cred,addr,proto)

  syslog('debug',string.format(
  		"checking %d:%d %s %s",
  		cred.uid,
  		cred.gid,
  		tostring(addr),
  		tostring(proto)
  	))
  	
  if  cred.uid == 9999 
  and cred.gid == 9999
  and addr.port == 17
  then
    local exe = fsys.readlink(string.format("/proc/%d/exe",cred.pid))
    return exe == '/home/spc/source/daemon/misc/qotd'
  end

  if unix.users[cred.uid] == nil then
    syslog('debug',"NO UID")
    return false
  end
  
  if unix.groups[cred.gid] == nil then
    syslog('debug',"NO GID")
    return false
  end
  
  if unix.users[cred.uid].userid == 'spc' then
    return true
  end
  
  if proto ~= 'tcp' and proto ~= 'udp' then
    syslog('debug',"BAD PROTO")
    return false
  end

  uname = unix.users[cred.uid].userid
  if users[uname] == nil then
    syslog('debug',string.format("%s not in users table",uname))
    return false
  end
  
  syslog('debug',string.format(
  		"lookup %s %s %s",
  		uname,
  		tostring(addr),
  		tostring(users[uname][tostring(addr)])
  	))
  
  
  
  return users[uname][tostring(addr)]
end

xx_users = users