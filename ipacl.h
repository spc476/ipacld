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

#ifndef IPACL_H
#define IPACL_H

/**********************************************************************/

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
		int                   *const restrict pfh,
		const struct sockaddr *const restrict sa,
		const unsigned int                    protocol
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
		const int                             reqport,
		int                   *const restrict pfh,
		const struct sockaddr *const restrict addr,
		const unsigned int                    protocol
	) __attribute__((nonnull,nothrow));

int ipacl_close(const int reqport) __attribute__((nothrow));

#endif
