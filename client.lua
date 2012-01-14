#!/usr/bin/env lua

proc = require "org.conman.process"
net  = require "org.conman.net"

lname = string.format("/tmp/acl-request.%d",proc.PID)

raddr = net.address("/tmp/acl")
laddr = net.address(lname)
sock  = net.socket(laddr.family,'udp')
sock:bind(laddr)

sock:write(raddr,"blah")
os.remove(lname)
