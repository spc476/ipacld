/*********************************************************************
*
* Copyright 2012 by Sean Conner.  All Rights Reserved.
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*
* Comments, questions and criticisms can be sent to: sean@conman.org
*
*********************************************************************/

#ifndef IPACL_PROTO_H
#define IPACL_PROTO_H

/***************************************************************************
* NOTE: None of the fields need to be sent in network byte order, since the
* 	packets never go out the actual network, but to another process on
* 	the same system.  And because of this, we don't have to worry about
* 	packing the data into a neutral format.  This simplifies the
* 	protocol quite a bit.
***************************************************************************/

typedef enum
{
  IPACLT_IP
} ipacl_type__t;

typedef struct
{
  ipacl_type__t   type;
  unsigned int    protocol;
  struct sockaddr sa;
} ipacl_net__t;

typedef struct
{
  ipacl_type__t      type;
  unsigned int       protocol;
  struct sockaddr_in sin;
} ipacl_ipv4__t;

typedef struct
{
  ipacl_type__t       type;
  unsigned int        protocol;
  struct sockaddr_in6 sin6;
} ipacl_ipv6__t;

typedef union
{
  ipacl_type__t type;
  ipacl_net__t  net;
  ipacl_ipv4__t ipv4;
  ipacl_ipv6__t ipv6;
} ipaclreq__t;

typedef struct
{
  int err;
} ipaclrep__t;

/**********************************************************************/

#endif
