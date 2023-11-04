/*	$NetBSD: types.h,v 1.13 2000/06/13 01:02:44 thorpej Exp $	*/

/*
 * Copyright (c) 2009, Sun Microsystems, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * - Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 * - Neither the name of Sun Microsystems, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived
 *   from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 *	from: @(#)types.h 1.18 87/07/24 SMI
 *	from: @(#)types.h	2.3 88/08/15 4.0 RPCSRC
 * $FreeBSD: src/include/rpc/types.h,v 1.10.6.1 2003/12/18 00:59:50 peter Exp $
 */

/* NFSv4.1 client for Windows
 * Copyright © 2012 The Regents of the University of Michigan
 *
 * Olga Kornievskaia <aglo@umich.edu>
 * Casey Bodley <cbodley@umich.edu>
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * without any warranty; without even the implied warranty of merchantability
 * or fitness for a particular purpose.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 */

/*
 * Rpc additions to <sys/types.h>
 */
#ifndef _TIRPC_TYPES_H
#define _TIRPC_TYPES_H

#include <sys/types.h>
#ifdef __REACTOS__
#include <windef.h>
#endif
//#include <sys/_null.h>

// Windows mappings of data types
// Fixed size things
typedef INT16      int16_t;
typedef INT32      int32_t;
typedef INT64      int64_t;
typedef UINT16   u_int16_t;
typedef UINT32   u_int32_t;
typedef UINT32	 uint32_t;
typedef UINT64   u_int64_t;
typedef UINT64   uint64_t;
typedef PCHAR      caddr_t;
// Scalable things
typedef UCHAR    u_char;
typedef unsigned short   u_short;
typedef UINT32   u_int;
typedef UINT32   uint;

typedef INT64      quad_t;
typedef UINT64   u_quad_t;

typedef UINT     uid_t;
typedef UINT     gid_t;
#ifndef __REACTOS__
typedef DWORD    pid_t;
#endif

#ifndef _SSIZE_T_DEFINED
#define _SSIZE_T_DEFINED
//typedef SIZE_T  size_t;  //This is causing a "benign redefinition error"
typedef SSIZE_T ssize_t;
// End of Windows...
#endif

typedef int32_t bool_t;
typedef int32_t enum_t;

typedef u_int32_t rpcprog_t;
typedef u_int32_t rpcvers_t;
typedef u_int32_t rpcproc_t;
typedef u_int32_t rpcprot_t;
typedef u_int32_t rpcport_t;
typedef   int32_t rpc_inline_t;

#ifndef NULL
#	define NULL	0
#endif
#define __dontcare__	-1

#ifndef FALSE
#	define FALSE	(0)
#endif
#ifndef TRUE
#	define TRUE	(1)
#endif

#define mem_alloc(bsize)	calloc(1, bsize)
#define mem_free(ptr, bsize)	free(ptr)

//#include <sys/time.h>
//#include <sys/param.h>
#include <stdlib.h>
#include <netconfig.h>
#ifdef __REACTOS__
#include <ws2def.h>
#include <winsock2.h>
#endif

/*
 * The netbuf structure is defined here, because FreeBSD / NetBSD only use
 * it inside the RPC code. It's in <xti.h> on SVR4, but it would be confusing
 * to have an xti.h, since FreeBSD / NetBSD does not support XTI/TLI.
 */

/*
 * The netbuf structure is used for transport-independent address storage.
 */
struct netbuf {
  unsigned int maxlen;
  unsigned int len;
  void *buf;
};

/*
 * The format of the addres and options arguments of the XTI t_bind call.
 * Only provided for compatibility, it should not be used.
 */

struct t_bind {
  struct netbuf   addr;
  unsigned int    qlen;
};

/*
 * Internal library and rpcbind use. This is not an exported interface, do
 * not use.
 */
struct __rpc_sockinfo {
	ADDRESS_FAMILY si_af; 
	int si_proto;
	int si_socktype;
	int si_alen;
};

#endif /* _TIRPC_TYPES_H */
