#!/usr/bin/env lua
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

errno = require "org.conman.errno"
proc  = require "org.conman.process"
net   = require "org.conman.net"
ipacl = require "org.conman.net.ipacl"
        require "org.conman.math".randomseed()

proc.sig.catch(proc.sig.INT)

list = 
{
  { "192.168.1.10" 	, 'tcp' , 'gopher'	} ,
  { "fc00::1"		, 'tcp' , 'gopher'	} ,
  { "192.168.1.10"	, 'udp' , 'tftp'	} ,
  { "0.0.0.0"		, 'udp' , 'tftp'	} ,
  { "fc00::1"		, 89	, 0		}
}

i = math.random(#list)
print(list[i][1],list[i][2],list[1][3])

fh,e = ipacl.request(list[i][1],list[i][2],list[i][3])

if e ~= 0 then
  print("failed",errno.strerror(e))
  os.exit(1)
end

print("success")

if list[i][2] == 'tcp' then
  sock = net.socketfd(fh)
  sock:listen()
end

proc.sleep(300)
os.exit(0)
