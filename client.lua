#!/usr/bin/env lua

errno = require "org.conman.errno"
proc  = require "org.conman.process"
fsys  = require "org.conman.fsys"
net   = require "org.conman.net"
        require "org.conman.math".randomseed()
        require "readacl"

proc.sig.catch(proc.sig.INT)
fsys.umask("--x--x--x")

lname = string.format("/tmp/ipacl-request.%d",proc.PID)

raddr = net.address("/dev/ipacl")
laddr = net.address(lname)
sock  = net.socket(laddr.family,'udp')
sock:bind(laddr)

list =
{
  { net.address("192.168.1.10"	,  70    , "tcp")	, 'tcp' } ,
  { net.address("fc00::1"	,  70    , "tcp")	, 'tcp' } ,
  { net.address("192.168.1.10"	, 'qotd' , "udp")	, 'udp' } ,
  { net.address("fc00::1"	,  70    , "udp")	, 'udp' } ,  
  { net.address("192.168.1.10"	, 253    , "raw")	, 'raw' } ,
  { net.address("0.0.0.0"	, 253	 , "raw")	, 'raw'	} ,
}

i       = math.random(#list)
print(list[i][1],list[i][2])
request = acl_encode(list[i][1],list[i][2])
sock:write(raddr,request)

_,fh,err = readfd(sock)

print(fh,err,errno.strerror(err)) io.stdout:flush()

if err ~= 0 then
  os.remove(lname)
  os.exit(1)
end

if list[i][2] == 'tcp' then
  newsock = net.socketfd(fh)
  newsock:listen()
end

proc.sleep(300)
os.remove(lname)
