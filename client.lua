#!/usr/bin/env lua

net = require "org.conman.net"

raddr = net.address("/tmp/acl")
laddr = net.address("/tmp/blah")
sock  = net.socket(laddr.family,'udp')
sock:bind(laddr)

sock:write(raddr,"blah")
os.remove("/tmp/blah")
