
#ifndef IPACL_PROTO_H
#define IPACL_PROTO_H

/***************************************************************************
* NOTE: None of the fields need to be sent in network byte order, since the
* 	packets never go out the actual network, but to another process on
* 	the same system.
***************************************************************************/

#ifdef __SunOS
#  pragma pack(1)
#endif

typedef struct ipaclraw_head
{
  uint16_t family;	/* ip, ipv6 */
  uint16_t proto;	/* Protocol to run on top of IP */
} __attribute__((packed)) ipaclraw_head__t;

typedef struct ipaclraw_ipv4
{
  uint16_t family;
  uint16_t proto;
  uint16_t port;	/* if applicable for proto */
  uint16_t rsvp;
  uint32_t addr;
} __attribute__((packed)) ipaclraw_ipv4__t;

typedef struct ipaclraw_ipv6
{
  uint16_t family;
  uint16_t proto;
  uint16_t port;	/* if applicable for proto */
  uint16_t rsvp;
  uint8_t  addr[16];
} __attribute__((packed)) ipaclraw_ipv6__t;

typedef union ipaclraw
{
  ipaclraw_head__t head;
  ipaclraw_ipv4__t ipv4;
  ipaclraw_ipv6__t ipv6;
} __attribute__((packed)) ipaclraw__t;

typedef struct ipaclrep
{
  uint32_t err;
} __attribute__((packed)) ipaclrep__t;

#ifdef __SunOS
#  pragma pack
#endif

/**********************************************************************/

#endif
