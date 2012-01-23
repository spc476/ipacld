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
