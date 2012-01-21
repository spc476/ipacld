
#ifndef IPACL_H
#define IPACL_H

typedef union ipacl_addr
{
  struct sockaddr     sa;
  struct sockaddr_in  sin;
  struct sockaddr_in6 sin6;
} ipacl_addr__t;

/**********************************************************************/

extern const struct sockaddr_un ipacl_port;


int ipacl_request_s(
		int        *const restrict pfh,
		const char *const restrict ip,
		const char *const restrict protocol,
		const char *const restrict port
	) __attribute__((nonnull,nothrow));
	
int ipacl_request(
		int        *const restrict pfh,
		const char *const restrict ip,
		const unsigned int         protocol,
		const unsigned int         port
	) __attribute__((nonnull,nothrow));

int ipacl_request_addr(
		int                 *const restrict pfh,
		const ipacl_addr__t *const restrict addr,
		const unsigned int                  protocol
	) __attribute__((nonnull,nothrow));

int ipacl_open(int *const) __attribute__((nonnull,nothrow));

int ipacl_do_request_s(
		const int                  reqport,
		int        *const restrict pfh,
		const char *const restrict ip,
		const char *const restrict protocol,
		const char *const restrict port
	) __attribute__((nonnull,nothrow));

int ipacl_do_request(
		const int                  reqport,
		int        *const restrict pfh,
		const char *const restrict ip,
		const unsigned int         protocol,
		const unsigned int         port
	) __attribute__((nonnull,nothrow));

int ipacl_do_request_addr(
		const int                           reqport,
		int                 *const restrict pfh,
		const ipacl_addr__t *const restrict req,
		const unsigned int                  protocol
	) __attribute__((nonnull,nothrow));

int ipacl_close(const int reqport) __attribute__((nothrow));

#endif
