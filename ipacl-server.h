
#ifndef IPACL_SERVER_H
#define IPACL_SERVER_H

int ipacls_open(int *const) __attribute__((nonnull));

int ipacls_read_request(
		const int                          reqport,
		struct sockaddr_un *const restrict remote,
		struct ucred       *const restrict cred,
		struct sockaddr    *const restrict addr,
		unsigned int       *const restrict protocol
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
