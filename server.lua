
net   = require "org.conman.net"
fsys  = require "org.conman.fsys"
unix  = require "org.conman.unix"
errno = require "org.conman.errno"
        require "readacl"

fsys.umask("--x--x--x")

laddr = net.address("/tmp/acl")
sock  = net.socket(laddr.family,'udp')
sock:bind(laddr)

recvcred(sock)

while true do
  remote,data,cred,err = readcred(sock)
  
  if err ~= 0 then
    print("readcred()",errno.strerror(err))
    os.exit(1)
  end
  
  if unix.users[cred.uid] ~= nil then
    cred.uid = unix.users[cred.uid].userid
  end
  
  if unix.groups[cred.gid] ~= nil then
    cred.gid = unix.groups[cred.gid].name
  end
  
  print("remote",remote)
  print(
  	"cred",
  	string.format(
  		"pid=%d uid=%s gid=%s",
  		cred.pid,
  		cred.uid,
  		cred.gid
  	)
  )
end

