
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

  if  cred.uid == 9999 
  and cred.gid == 9999
  and addr.port == 17
  then
    local exe = fsys.readlink(string.format("/proc/%d/exe",cred.pid))
    return exe == '/home/spc/source/daemon/misc/qotd'
  end

  if unix.users[cred.uid] == nil then
    return false
  end
  
  if unix.groups[cred.gid] == nil then
    return false
  end
  
  if unix.users[cred.uid].userid == 'spc' then
    return true
  end
  
  if proto ~= 'tcp' and proto ~= 'udp' then
    return false
  end

  uname = unix.users[cred.uid].userid
  if users[uname] == nil then
    return false
  end
  
  return users[uname][tostring(addr)]
end
