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

//#include <sys/cdefs.h>

/*
 * auth_unix.c, Implements UNIX style authentication parameters.
 *
 * Copyright (C) 1984, Sun Microsystems, Inc.
 *
 * The system is very weak.  The client uses no encryption for it's
 * credentials and only sends null verifiers.  The server sends backs
 * null verifiers or optionally a verifier that suggests a new short hand
 * for the credentials.
 *
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
//#include <sys/param.h>

#include <assert.h>
//#include <err.h>
#include <stdio.h>
#include <stdlib.h>
//#include <unistd.h>
#include <string.h>

#include <rpc/types.h>
#include <rpc/xdr.h>
#include <rpc/auth.h>
#include <rpc/auth_unix.h>

/* auth_unix.c */
static void authunix_nextverf (AUTH *);
static bool_t authunix_marshal (AUTH *, XDR *, u_int *seq);
static bool_t authunix_validate (AUTH *, struct opaque_auth *, u_int);
static bool_t authunix_refresh (AUTH *, void *);
static void authunix_destroy (AUTH *);
static void marshal_new_auth (AUTH *);
static struct auth_ops *authunix_ops (void);

/*
 * This struct is pointed to by the ah_private field of an auth_handle.
 */
struct audata {
	struct opaque_auth	au_origcred;	/* original credentials */
	struct opaque_auth	au_shcred;	/* short hand cred */
	u_long			au_shfaults;	/* short hand cache faults */
	char			au_marshed[MAX_AUTH_BYTES];
	u_int			au_mpos;	/* xdr pos at end of marshed */
};
#define	AUTH_PRIVATE(auth)	((struct audata *)auth->ah_private)

/*
 * Create a unix style authenticator.
 * Returns an auth handle with the given stuff in it.
 */
AUTH *
authunix_create(machname, uid, gid, len, aup_gids)
	char *machname;
	uid_t uid;
	gid_t gid;
	int len;
	gid_t *aup_gids;
{
	struct authunix_parms aup;
	char mymem[MAX_AUTH_BYTES];
	struct timeval now;
	XDR xdrs;
	AUTH *auth;
	struct audata *au;

	/*
	 * Allocate and set up auth handle
	 */
	au = NULL;
	auth = mem_alloc(sizeof(*auth));
#ifndef _KERNEL
	if (auth == NULL) {
		// XXX warnx("authunix_create: out of memory");
		goto cleanup_authunix_create;
	}
#endif
	au = mem_alloc(sizeof(*au));
#ifndef _KERNEL
	if (au == NULL) {
		// XXX warnx("authunix_create: out of memory");
		goto cleanup_authunix_create;
	}
#endif
	auth->ah_ops = authunix_ops();
	auth->ah_private = (caddr_t)au;
	auth->ah_verf = au->au_shcred = _null_auth;
	au->au_shfaults = 0;
	au->au_origcred.oa_base = NULL;

	/*
	 * fill in param struct from the given params
	 */
	(void)gettimeofday(&now, NULL);
	aup.aup_time = now.tv_sec;
	aup.aup_machname = machname;
	aup.aup_uid = uid;
	aup.aup_gid = gid;
	aup.aup_len = (u_int)len;
	aup.aup_gids = aup_gids;

	/*
	 * Serialize the parameters into origcred
	 */
	xdrmem_create(&xdrs, mymem, MAX_AUTH_BYTES, XDR_ENCODE);
	if (! xdr_authunix_parms(&xdrs, &aup)) 
		abort();
	au->au_origcred.oa_length = len = XDR_GETPOS(&xdrs);
	au->au_origcred.oa_flavor = AUTH_UNIX;
#ifdef _KERNEL
	au->au_origcred.oa_base = mem_alloc((u_int) len);
#else
	if ((au->au_origcred.oa_base = mem_alloc((u_int) len)) == NULL) {
		// XXX warnx("authunix_create: out of memory");
		goto cleanup_authunix_create;
	}
#endif
	memmove(au->au_origcred.oa_base, mymem, (size_t)len);

	/*
	 * set auth handle to reflect new cred.
	 */
	auth->ah_cred = au->au_origcred;
	marshal_new_auth(auth);
	return (auth);
#ifndef _KERNEL
 cleanup_authunix_create:
	if (auth)
		mem_free(auth, sizeof(*auth));
	if (au) {
		if (au->au_origcred.oa_base)
			mem_free(au->au_origcred.oa_base, (u_int)len);
		mem_free(au, sizeof(*au));
	}
	return (NULL);
#endif
}

/*
 * Returns an auth handle with parameters determined by doing lots of
 * syscalls.
 */
AUTH *
authunix_create_default()
{
	int len;
	char machname[MAXHOSTNAMELEN + 1];
	uid_t uid;
	gid_t gid;
	gid_t gids[NGRPS];

	if (gethostname(machname, sizeof machname) == -1)
		abort();
	machname[sizeof(machname) - 1] = 0;
#if 0
	uid = geteuid();
	gid = getegid();
	if ((len = getgroups(NGRPS, gids)) < 0)
		abort();
#else
	// XXX Need to figure out what to do here!
	uid = 666;
	gid = 777;
	gids[0] = 0;
	len = 0;
#endif
	/* XXX: interface problem; those should all have been unsigned */
	return (authunix_create(machname, uid, gid, len, gids));
}

/*
 * authunix operations
 */

/* ARGSUSED */
static void
authunix_nextverf(auth)
	AUTH *auth;
{
	/* no action necessary */
}

static bool_t
authunix_marshal(auth, xdrs, seq)
	AUTH *auth;
	XDR *xdrs;
    u_int *seq;
{
	struct audata *au;

	assert(auth != NULL);
	assert(xdrs != NULL);

	au = AUTH_PRIVATE(auth);
	return (XDR_PUTBYTES(xdrs, au->au_marshed, au->au_mpos));
}

static bool_t
authunix_validate(auth, verf, seq)
	AUTH *auth;
	struct opaque_auth *verf;
    u_int seq;
{
	struct audata *au;
	XDR xdrs;

	assert(auth != NULL);
	assert(verf != NULL);

	if (verf->oa_flavor == AUTH_SHORT) {
		au = AUTH_PRIVATE(auth);
		xdrmem_create(&xdrs, verf->oa_base, verf->oa_length,
		    XDR_DECODE);

		if (au->au_shcred.oa_base != NULL) {
			mem_free(au->au_shcred.oa_base,
			    au->au_shcred.oa_length);
			au->au_shcred.oa_base = NULL;
		}
		if (xdr_opaque_auth(&xdrs, &au->au_shcred)) {
			auth->ah_cred = au->au_shcred;
		} else {
			xdrs.x_op = XDR_FREE;
			(void)xdr_opaque_auth(&xdrs, &au->au_shcred);
			au->au_shcred.oa_base = NULL;
			auth->ah_cred = au->au_origcred;
		}
		marshal_new_auth(auth);
	}
	return (TRUE);
}

static bool_t
authunix_refresh(AUTH *auth, void *dummy)
{
	struct audata *au = AUTH_PRIVATE(auth);
	struct authunix_parms aup;
	struct timeval now;
	XDR xdrs;
	int stat;

	assert(auth != NULL);

	if (auth->ah_cred.oa_base == au->au_origcred.oa_base) {
		/* there is no hope.  Punt */
		return (FALSE);
	}
	au->au_shfaults ++;

	/* first deserialize the creds back into a struct authunix_parms */
	aup.aup_machname = NULL;
	aup.aup_gids = NULL;
	xdrmem_create(&xdrs, au->au_origcred.oa_base,
	    au->au_origcred.oa_length, XDR_DECODE);
	stat = xdr_authunix_parms(&xdrs, &aup);
	if (! stat)
		goto done;

	/* update the time and serialize in place */
	(void)gettimeofday(&now, NULL);
	aup.aup_time = now.tv_sec;
	xdrs.x_op = XDR_ENCODE;
	XDR_SETPOS(&xdrs, 0);
	stat = xdr_authunix_parms(&xdrs, &aup);
	if (! stat)
		goto done;
	auth->ah_cred = au->au_origcred;
	marshal_new_auth(auth);
done:
	/* free the struct authunix_parms created by deserializing */
	xdrs.x_op = XDR_FREE;
	(void)xdr_authunix_parms(&xdrs, &aup);
	XDR_DESTROY(&xdrs);
	return (stat);
}

static void
authunix_destroy(auth)
	AUTH *auth;
{
	struct audata *au;

	assert(auth != NULL);

	au = AUTH_PRIVATE(auth);
	mem_free(au->au_origcred.oa_base, au->au_origcred.oa_length);

	if (au->au_shcred.oa_base != NULL)
		mem_free(au->au_shcred.oa_base, au->au_shcred.oa_length);

	mem_free(auth->ah_private, sizeof(struct audata));

	if (auth->ah_verf.oa_base != NULL)
		mem_free(auth->ah_verf.oa_base, auth->ah_verf.oa_length);

	mem_free(auth, sizeof(*auth));
}

/*
 * Marshals (pre-serializes) an auth struct.
 * sets private data, au_marshed and au_mpos
 */
static void
marshal_new_auth(auth)
	AUTH *auth;
{
	XDR	xdr_stream;
	XDR	*xdrs = &xdr_stream;
	struct audata *au;

	assert(auth != NULL);

	au = AUTH_PRIVATE(auth);
	xdrmem_create(xdrs, au->au_marshed, MAX_AUTH_BYTES, XDR_ENCODE);
	if ((! xdr_opaque_auth(xdrs, &(auth->ah_cred))) ||
	    (! xdr_opaque_auth(xdrs, &(auth->ah_verf))))
		assert(0); // XXX
		// XXX warnx("auth_none.c - Fatal marshalling problem");
	else
		au->au_mpos = XDR_GETPOS(xdrs);
	XDR_DESTROY(xdrs);
}

static bool_t
#ifndef __REACTOS__
authunix_wrap(AUTH *auth, XDR *xdrs, xdrproc_t func, caddr_t args, u_int seq)
#else
authunix_wrap(AUTH *auth, XDR *xdrs, xdrproc_t func, caddr_t args)
#endif
{
    return ((*func)(xdrs, args));
}
#ifdef __REACTOS__
static bool_t
authunix_unwrap(AUTH *auth, XDR *xdrs, xdrproc_t func, caddr_t args, u_int seq)
{
    return ((*func)(xdrs, args));
}
#endif

static struct auth_ops *
authunix_ops()
{
	static struct auth_ops ops;
	extern mutex_t ops_lock;

	/* VARIABLES PROTECTED BY ops_lock: ops */

	mutex_lock(&ops_lock);
	if (ops.ah_nextverf == NULL) {
		ops.ah_nextverf = authunix_nextverf;
		ops.ah_marshal = authunix_marshal;
		ops.ah_validate = authunix_validate;
		ops.ah_refresh = authunix_refresh;
		ops.ah_destroy = authunix_destroy;
		ops.ah_wrap = authunix_wrap;
#ifndef __REACTOS__
		ops.ah_unwrap = authunix_wrap;
#else
		ops.ah_unwrap = authunix_unwrap;
#endif
	}
	mutex_unlock(&ops_lock);
	return (&ops);
}
