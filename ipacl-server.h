
#ifndef IPACL_SERVER_H
#define IPACL_SERVER_H

int ipacls_open(int *const) __attribute__((nonnull));

int ipacls_read_request(
		const int                          reqport,
		ipacl_addr__t      *const restrict addr,
		struct ucred       *const restrict cred,
		unsigned int       *const restrict protocol,
		struct sockaddr_un *const restrict remote
	) __attribute__((nonnull,nothrow));

int ipacls_send_err(
		const int                          reqport,
		struct sockaddr_un *const restrict remote,
		const int                          err
	) __attribute__((nonnull,nothrow));
	
int ipacls_send_fd(
		const int                          reqport,
		struct sockaddr_un *const restrict remote,
		const int                          fh
	) __attribute__((nonnull,nothrow));
	
int ipacls_close(const int reqport);

#endif
