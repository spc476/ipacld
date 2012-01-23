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

typedef enum ipacl_type
{
  IPACLT_IP
} ipacl_type__t;

typedef struct ipaclraw_head
{
  ipacl_type__t type;
} ipaclraw_head__t;

typedef struct ipaclraw_net
{
  ipacl_type__t   type;
  unsigned int    protocol;
  struct sockaddr sa;
} ipaclraw_net__t;

typedef struct ipaclraw_ipv4
{
  ipacl_type__t      type;
  unsigned int       protocol;
  struct sockaddr_in sin;
} ipaclraw_ipv4__t;

typedef struct ipaclraw_ipv6
{
  ipacl_type__t       type;
  unsigned int        protocol;
  struct sockaddr_in6 sin6;
} ipaclraw_ipv6__t;

typedef union ipaclraw
{
  ipaclraw_head__t head;
  ipaclraw_net__t  net;
  ipaclraw_ipv4__t ipv4;
  ipaclraw_ipv6__t ipv6;
} ipaclraw__t;

typedef struct ipaclrep
{
  int err;
} ipaclrep__t;

/**********************************************************************/

#endif
