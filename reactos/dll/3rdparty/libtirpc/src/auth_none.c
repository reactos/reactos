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
 */

/*
#if defined(LIBC_SCCS) && !defined(lint)
static char *sccsid = "@(#)auth_none.c 1.19 87/08/11 Copyr 1984 Sun Micro";
static char *sccsid = "@(#)auth_none.c	2.1 88/07/29 4.0 RPCSRC";
#endif
//#include <sys/cdefs.h>
__FBSDID("$FreeBSD: src/lib/libc/rpc/auth_none.c,v 1.12 2002/03/22 23:18:35 obrien Exp $");
*/


/*
 * auth_none.c
 * Creates a client authentication handle for passing "null"
 * credentials and verifiers to remote systems.
 *
 * Copyright (C) 1984, Sun Microsystems, Inc.
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

#include <wintirpc.h>
//#include <pthread.h>
#include <reentrant.h>
#include <assert.h>
#include <stdlib.h>
#include <rpc/types.h>
#include <rpc/xdr.h>
#include <rpc/auth.h>

#define MAX_MARSHAL_SIZE 20

/*
 * Authenticator operations routines
 */

static bool_t authnone_marshal (AUTH *, XDR *, u_int *);
static void authnone_verf (AUTH *);
static bool_t authnone_validate (AUTH *, struct opaque_auth *, u_int);
static bool_t authnone_refresh (AUTH *, void *);
static void authnone_destroy (AUTH *);

extern bool_t xdr_opaque_auth();

static struct auth_ops *authnone_ops();

static struct authnone_private {
	AUTH	no_client;
	char	marshalled_client[MAX_MARSHAL_SIZE];
	u_int	mcnt;
} *authnone_private;

AUTH *
authnone_create()
{
	struct authnone_private *ap = authnone_private;
	XDR xdr_stream;
	XDR *xdrs;
	extern mutex_t authnone_lock;

	mutex_lock(&authnone_lock);
	if (ap == 0) {
		ap = (struct authnone_private *)calloc(1, sizeof (*ap));
		if (ap == 0) {
			mutex_unlock(&authnone_lock);
			return (0);
		}
		authnone_private = ap;
	}
	if (!ap->mcnt) {
		ap->no_client.ah_cred = ap->no_client.ah_verf = _null_auth;
		ap->no_client.ah_ops = authnone_ops();
		xdrs = &xdr_stream;
		xdrmem_create(xdrs, ap->marshalled_client,
		    (u_int)MAX_MARSHAL_SIZE, XDR_ENCODE);
		(void)xdr_opaque_auth(xdrs, &ap->no_client.ah_cred);
		(void)xdr_opaque_auth(xdrs, &ap->no_client.ah_verf);
		ap->mcnt = XDR_GETPOS(xdrs);
		XDR_DESTROY(xdrs);
	}
	mutex_unlock(&authnone_lock);
	return (&ap->no_client);
}

/*ARGSUSED*/
static bool_t
authnone_marshal(AUTH *client, XDR *xdrs, u_int *seq)
{
	struct authnone_private *ap;
	bool_t dummy;
	extern mutex_t authnone_lock;

	assert(xdrs != NULL);

	ap = authnone_private;
	if (ap == NULL) {
		mutex_unlock(&authnone_lock);
		return (FALSE);
	}
	dummy = (*xdrs->x_ops->x_putbytes)(xdrs,
	    ap->marshalled_client, ap->mcnt);
	mutex_unlock(&authnone_lock);
	return (dummy);
}

/* All these unused parameters are required to keep ANSI-C from grumbling */
/*ARGSUSED*/
static void
authnone_verf(AUTH *client)
{
}

/*ARGSUSED*/
static bool_t
authnone_validate(AUTH *client, struct opaque_auth *opaque, u_int seq)
{

	return (TRUE);
}

/*ARGSUSED*/
static bool_t
authnone_refresh(AUTH *client, void *dummy)
{

	return (FALSE);
}

/*ARGSUSED*/
static void
authnone_destroy(AUTH *client)
{
}

static int
authnone_wrap(AUTH *auth, XDR *xdrs, xdrproc_t func, caddr_t args)
{
    return ((*func)(xdrs, args));
}

static int
authnone_unwrap(AUTH *auth, XDR *xdrs, xdrproc_t func, caddr_t args, u_int seq)
{
    return ((*func)(xdrs, args));
}

static struct auth_ops *
authnone_ops()
{
	static struct auth_ops ops;
	extern mutex_t ops_lock;
 
/* VARIABLES PROTECTED BY ops_lock: ops */
 
	mutex_lock(&ops_lock);
	if (ops.ah_nextverf == NULL) {
		ops.ah_nextverf = authnone_verf;
		ops.ah_marshal = authnone_marshal;
		ops.ah_validate = authnone_validate;
		ops.ah_refresh = authnone_refresh;
		ops.ah_destroy = authnone_destroy;
        ops.ah_wrap = authnone_wrap;
        ops.ah_unwrap = authnone_unwrap;
	}
	mutex_unlock(&ops_lock);
	return (&ops);
}
