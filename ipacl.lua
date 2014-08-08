-- ***************************************************************
--
-- Copyright 2012 by Sean Conner.  All Rights Reserved.
--
-- This program is free software: you can redistribute it and/or modify
-- it under the terms of the GNU General Public License as published by
-- the Free Software Foundation, either version 3 of the License, or
-- (at your option) any later version.
--
-- This program is distributed in the hope that it will be useful,
-- but WITHOUT ANY WARRANTY; without even the implied warranty of
-- MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
-- GNU General Public License for more details.
--
-- You should have received a copy of the GNU General Public License
-- along with this program.  If not, see <http://www.gnu.org/licenses/>.
--
-- Comments, questions and criticisms can be sent to: sean@conman.org
--
-- ********************************************************************

local syslog = require "org.conman.syslog"
local unix = require "org.conman.unix"
local net  = require "org.conman.net"
local fsys = require "org.conman.fsys"

local function A(addr,proto,port)
  return tostring(net.address(addr,proto,port))
end

-- *****************************************************************

local users = 
{
  gopher = 
  { 
    [A('0.0.0.0'	, 'tcp' , 'gopher')] = true ,
    [A('192.168.1.10'	, 'tcp' , 'gopher')] = true
  },
  
  tftp =
  {
    [A('0.0.0.0'	, 'udp' , 'tftp')] = true ,
    [A('192.168.1.10'	, 'udp' , 'tftp')] = true
  },
  
  ntp =
  {
    [A('0.0.0.0'	, 'udp' , 'daytime')] = true,
    [A('0.0.0.0'	, 'tcp' , 'daytime')] = true
  },
  
  nobody =
  {
    [A('0.0.0.0'	, 'tcp' , 'finger')] = true,
  }
}  

-- *************************************************************

for user,aclist in pairs(users) do
  for addr,acl in pairs(aclist) do
    syslog(
    	"debug",
    	"user=%s addr=%s allow=%s",
    	user,
    	addr,
    	tostring(acl)
    )
  end
end


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
  
  syslog('debug',"uname=%s addr=%s value=%s",uname,tostring(addr),tostring(users[uname][tostring(addr)]))
  return users[uname][tostring(addr)]
end
