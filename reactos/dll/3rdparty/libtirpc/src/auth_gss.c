/*
  auth_gss.c

  RPCSEC_GSS client routines.

  Copyright (c) 2000 The Regents of the University of Michigan.
  All rights reserved.

  Copyright (c) 2000 Dug Song <dugsong@UMICH.EDU>.
  All rights reserved, all wrongs reversed.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:

  1. Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.
  2. Redistributions in binary form must reproduce the above copyright
     notice, this list of conditions and the following disclaimer in the
     documentation and/or other materials provided with the distribution.
  3. Neither the name of the University nor the names of its
     contributors may be used to endorse or promote products derived
     from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED
  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#include <stdio.h>
#include <stdlib.h>
//#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <rpc/types.h>
#include <rpc/xdr.h>
#include <rpc/auth.h>
#include <rpc/auth_gss.h>
#include <rpc/clnt.h>
#include <netinet/in.h>
#include <gssapi/gssapi.h>

static void	authgss_nextverf();
static bool_t	authgss_marshal();
static bool_t	authgss_refresh();
static bool_t	authgss_validate();
static void	authgss_destroy();
static void	authgss_destroy_context();
static bool_t	authgss_wrap();
static bool_t	authgss_unwrap();


/*
 * from mit-krb5-1.2.1 mechglue/mglueP.h:
 * Array of context IDs typed by mechanism OID
 */
typedef struct gss_union_ctx_id_t {
	gss_OID     mech_type;
	gss_ctx_id_t    internal_ctx_id;
} gss_union_ctx_id_desc, *gss_union_ctx_id_t;

static struct auth_ops authgss_ops = {
	authgss_nextverf,
	authgss_marshal,
	authgss_validate,
	authgss_refresh,
	authgss_destroy,
	authgss_wrap,
	authgss_unwrap
};

#ifdef DEBUG

/* useful as i add more mechanisms */
void
print_rpc_gss_sec(struct rpc_gss_sec *ptr)
{
int i;
char *p;

	log_debug("rpc_gss_sec:");
	if(ptr->mech == NULL)
		log_debug("NULL gss_OID mech");
	else {
		fprintf(stderr, "     mechanism_OID: {");
		p = (char *)ptr->mech->elements;
		for (i=0; i < ptr->mech->length; i++)
			/* First byte of OIDs encoded to save a byte */
			if (i == 0) {
				int first, second;
				if (*p < 40) {
					first = 0;
					second = *p;
				}
				else if (40 <= *p && *p < 80) {
					first = 1;
					second = *p - 40;
				}
				else if (80 <= *p && *p < 127) {
					first = 2;
					second = *p - 80;
				}
				else {
					/* Invalid value! */
					first = -1;
					second = -1;
				}
				fprintf(stderr, " %u %u", first, second);
				p++;
			}
			else {
				fprintf(stderr, " %u", (unsigned char)*p++);
			}
		fprintf(stderr, " }\n");
	}
	fprintf(stderr, "     qop: %d\n", ptr->qop);
	fprintf(stderr, "     service: %d\n", ptr->svc);
	fprintf(stderr, "     cred: %p\n", ptr->cred);
}
#endif /*DEBUG*/

struct rpc_gss_data {
	bool_t			 established;	/* context established */
	gss_buffer_desc		 gc_wire_verf;	/* save GSS_S_COMPLETE NULL RPC verfier
						 * to process at end of context negotiation*/
	CLIENT			*clnt;		/* client handle */
	gss_name_t		 name;		/* service name */
	struct rpc_gss_sec	 sec;		/* security tuple */
	gss_ctx_id_t		 ctx;		/* context id */
	struct rpc_gss_cred	 gc;		/* client credentials */
	u_int			 win;		/* sequence window */
};

#define	AUTH_PRIVATE(auth)	((struct rpc_gss_data *)auth->ah_private)

static struct timeval AUTH_TIMEOUT = { 25, 0 };

AUTH *
authgss_create(CLIENT *clnt, gss_name_t name, struct rpc_gss_sec *sec)
{
	AUTH			*auth, *save_auth;
	struct rpc_gss_data	*gd;
	OM_uint32		min_stat = 0;

	log_debug("in authgss_create()");

	memset(&rpc_createerr, 0, sizeof(rpc_createerr));

	if ((auth = calloc(sizeof(*auth), 1)) == NULL) {
		rpc_createerr.cf_stat = RPC_SYSTEMERROR;
		rpc_createerr.cf_error.re_errno = ENOMEM;
		return (NULL);
	}
	if ((gd = calloc(sizeof(*gd), 1)) == NULL) {
		rpc_createerr.cf_stat = RPC_SYSTEMERROR;
		rpc_createerr.cf_error.re_errno = ENOMEM;
		free(auth);
		return (NULL);
	}
#ifdef DEBUG
	fprintf(stderr, "authgss_create: name is %p\n", name);
#endif
	if (name != GSS_C_NO_NAME) {
		if (gss_duplicate_name(&min_stat, name, &gd->name)
						!= GSS_S_COMPLETE) {
			rpc_createerr.cf_stat = RPC_SYSTEMERROR;
			rpc_createerr.cf_error.re_errno = ENOMEM;
			free(auth);
			return (NULL);
		}
	}
	else
		gd->name = name;

#ifdef DEBUG
	fprintf(stderr, "authgss_create: gd->name is %p\n", gd->name);
#endif
	gd->clnt = clnt;
	gd->ctx = GSS_C_NO_CONTEXT;
	gd->sec = *sec;

	gd->gc.gc_v = RPCSEC_GSS_VERSION;
	gd->gc.gc_proc = RPCSEC_GSS_INIT;
	gd->gc.gc_svc = gd->sec.svc;

	auth->ah_ops = &authgss_ops;
	auth->ah_private = (caddr_t)gd;

	save_auth = clnt->cl_auth;
	clnt->cl_auth = auth;

	if (!authgss_refresh(auth))
		auth = NULL;

	clnt->cl_auth = save_auth;

	return (auth);
}

AUTH *
authgss_create_default(CLIENT *clnt, char *service, struct rpc_gss_sec *sec)
{
	AUTH			*auth;
	OM_uint32		 maj_stat = 0, min_stat = 0;
	gss_buffer_desc		 sname;
	gss_name_t		 name = GSS_C_NO_NAME;

	log_debug("in authgss_create_default()");


	sname.value = service;
	sname.length = strlen(service);

	maj_stat = gss_import_name(&min_stat, &sname,
		(gss_OID)GSS_C_NT_HOSTBASED_SERVICE,
		&name);

	if (maj_stat != GSS_S_COMPLETE) {
		log_status("gss_import_name", maj_stat, min_stat);
		rpc_createerr.cf_stat = RPC_AUTHERROR;
		return (NULL);
	}

	auth = authgss_create(clnt, name, sec);

	if (name != GSS_C_NO_NAME) {
#ifdef DEBUG
	fprintf(stderr, "authgss_create_default: freeing name %p\n", name);
#endif
 		gss_release_name(&min_stat, &name);
	}

	return (auth);
}

bool_t
authgss_get_private_data(AUTH *auth, struct authgss_private_data *pd)
{
	struct rpc_gss_data	*gd;

	log_debug("in authgss_get_private_data()");

	if (!auth || !pd)
		return (FALSE);

	gd = AUTH_PRIVATE(auth);

	if (!gd || !gd->established)
		return (FALSE);

	pd->pd_ctx = gd->ctx;
	pd->pd_ctx_hndl = gd->gc.gc_ctx;
	pd->pd_seq_win = gd->win;

	return (TRUE);
}

static void
authgss_nextverf(AUTH *auth)
{
	log_debug("in authgss_nextverf()");
	/* no action necessary */
}

static bool_t
authgss_marshal(AUTH *auth, XDR *xdrs)
{
	XDR			 tmpxdrs;
	char			 tmp[MAX_AUTH_BYTES];
	struct rpc_gss_data	*gd;
	gss_buffer_desc		 rpcbuf, checksum;
	OM_uint32		 maj_stat, min_stat;
	bool_t			 xdr_stat;

	log_debug("in authgss_marshal()");

	gd = AUTH_PRIVATE(auth);

	if (gd->established)
		gd->gc.gc_seq++;

	xdrmem_create(&tmpxdrs, tmp, sizeof(tmp), XDR_ENCODE);

	if (!xdr_rpc_gss_cred(&tmpxdrs, &gd->gc)) {
		XDR_DESTROY(&tmpxdrs);
		return (FALSE);
	}
	auth->ah_cred.oa_flavor = RPCSEC_GSS;
	auth->ah_cred.oa_base = tmp;
	auth->ah_cred.oa_length = XDR_GETPOS(&tmpxdrs);

	XDR_DESTROY(&tmpxdrs);

	if (!xdr_opaque_auth(xdrs, &auth->ah_cred))
		return (FALSE);

	if (gd->gc.gc_proc == RPCSEC_GSS_INIT ||
	    gd->gc.gc_proc == RPCSEC_GSS_CONTINUE_INIT) {
		return (xdr_opaque_auth(xdrs, &_null_auth));
	}
	/* Checksum serialized RPC header, up to and including credential. */
	rpcbuf.length = XDR_GETPOS(xdrs);
	XDR_SETPOS(xdrs, 0);
	rpcbuf.value = XDR_INLINE(xdrs, rpcbuf.length);

	maj_stat = gss_get_mic(&min_stat, gd->ctx, gd->sec.qop,
			    &rpcbuf, &checksum);

	if (maj_stat != GSS_S_COMPLETE) {
		log_status("gss_get_mic", maj_stat, min_stat);
		if (maj_stat == GSS_S_CONTEXT_EXPIRED) {
			gd->established = FALSE;
			authgss_destroy_context(auth);
		}
		return (FALSE);
	}
	auth->ah_verf.oa_flavor = RPCSEC_GSS;
	auth->ah_verf.oa_base = checksum.value;
	auth->ah_verf.oa_length = checksum.length;

	xdr_stat = xdr_opaque_auth(xdrs, &auth->ah_verf);
	gss_release_buffer(&min_stat, &checksum);

	return (xdr_stat);
}

static bool_t
authgss_validate(AUTH *auth, struct opaque_auth *verf)
{
	struct rpc_gss_data	*gd;
	u_int			 num, qop_state;
	gss_buffer_desc		 signbuf, checksum;
	OM_uint32		 maj_stat, min_stat;

	log_debug("in authgss_validate()");

	gd = AUTH_PRIVATE(auth);

	if (gd->established == FALSE) {
		/* would like to do this only on NULL rpc --
		 * gc->established is good enough.
		 * save the on the wire verifier to validate last
		 * INIT phase packet after decode if the major
		 * status is GSS_S_COMPLETE
		 */
		if ((gd->gc_wire_verf.value =
				mem_alloc(verf->oa_length)) == NULL) {
			fprintf(stderr, "gss_validate: out of memory\n");
			return (FALSE);
		}
		memcpy(gd->gc_wire_verf.value, verf->oa_base, verf->oa_length);
		gd->gc_wire_verf.length = verf->oa_length;
		return (TRUE);
  	}

	if (gd->gc.gc_proc == RPCSEC_GSS_INIT ||
	    gd->gc.gc_proc == RPCSEC_GSS_CONTINUE_INIT) {
		num = htonl(gd->win);
	}
	else num = htonl(gd->gc.gc_seq);

	signbuf.value = &num;
	signbuf.length = sizeof(num);

	checksum.value = verf->oa_base;
	checksum.length = verf->oa_length;

	maj_stat = gss_verify_mic(&min_stat, gd->ctx, &signbuf,
				  &checksum, &qop_state);
	if (maj_stat != GSS_S_COMPLETE || qop_state != gd->sec.qop) {
		log_status("gss_verify_mic", maj_stat, min_stat);
		if (maj_stat == GSS_S_CONTEXT_EXPIRED) {
			gd->established = FALSE;
			authgss_destroy_context(auth);
		}
		return (FALSE);
	}
	return (TRUE);
}

static bool_t
authgss_refresh(AUTH *auth)
{
	struct rpc_gss_data	*gd;
	struct rpc_gss_init_res	 gr;
	gss_buffer_desc		*recv_tokenp, send_token;
	OM_uint32		 maj_stat, min_stat, call_stat, ret_flags;

	log_debug("in authgss_refresh()");

	gd = AUTH_PRIVATE(auth);

	if (gd->established)
		return (TRUE);

	/* GSS context establishment loop. */
	memset(&gr, 0, sizeof(gr));
	recv_tokenp = GSS_C_NO_BUFFER;

#ifdef DEBUG
	print_rpc_gss_sec(&gd->sec);
#endif /*DEBUG*/

	for (;;) {
#ifdef DEBUG
		/* print the token we just received */
		if (recv_tokenp != GSS_C_NO_BUFFER) {
			log_debug("The token we just received (length %d):",
				  recv_tokenp->length);
			log_hexdump(recv_tokenp->value, recv_tokenp->length, 0);
		}
#endif
		maj_stat = gss_init_sec_context(&min_stat,
						gd->sec.cred,
						&gd->ctx,
						gd->name,
						gd->sec.mech,
						gd->sec.req_flags,
						0,		/* time req */
						NULL,		/* channel */
						recv_tokenp,
						NULL,		/* used mech */
						&send_token,
						&ret_flags,
						NULL);		/* time rec */

		if (recv_tokenp != GSS_C_NO_BUFFER) {
			gss_release_buffer(&min_stat, &gr.gr_token);
			recv_tokenp = GSS_C_NO_BUFFER;
		}
		if (maj_stat != GSS_S_COMPLETE &&
		    maj_stat != GSS_S_CONTINUE_NEEDED) {
			log_status("gss_init_sec_context", maj_stat, min_stat);
			break;
		}
		if (send_token.length != 0) {
			memset(&gr, 0, sizeof(gr));

#ifdef DEBUG
			/* print the token we are about to send */
			log_debug("The token being sent (length %d):",
				  send_token.length);
			log_hexdump(send_token.value, send_token.length, 0);
#endif

			call_stat = clnt_call(gd->clnt, NULLPROC,
					      (xdrproc_t)xdr_rpc_gss_init_args,
					      &send_token,
					      (xdrproc_t)xdr_rpc_gss_init_res,
					      (caddr_t)&gr, AUTH_TIMEOUT);

			gss_release_buffer(&min_stat, &send_token);

			if (call_stat != RPC_SUCCESS ||
			    (gr.gr_major != GSS_S_COMPLETE &&
			     gr.gr_major != GSS_S_CONTINUE_NEEDED))
				return FALSE;

			if (gr.gr_ctx.length != 0) {
				if (gd->gc.gc_ctx.value)
					gss_release_buffer(&min_stat,
							   &gd->gc.gc_ctx);
				gd->gc.gc_ctx = gr.gr_ctx;
			}
			if (gr.gr_token.length != 0) {
				if (maj_stat != GSS_S_CONTINUE_NEEDED)
					break;
				recv_tokenp = &gr.gr_token;
			}
			gd->gc.gc_proc = RPCSEC_GSS_CONTINUE_INIT;
		}

		/* GSS_S_COMPLETE => check gss header verifier,
		 * usually checked in gss_validate
		 */
		if (maj_stat == GSS_S_COMPLETE) {
			gss_buffer_desc   bufin;
			gss_buffer_desc   bufout;
			u_int seq, qop_state = 0;

			seq = htonl(gr.gr_win);
			bufin.value = (unsigned char *)&seq;
			bufin.length = sizeof(seq);
			bufout.value = (unsigned char *)gd->gc_wire_verf.value;
			bufout.length = gd->gc_wire_verf.length;

			maj_stat = gss_verify_mic(&min_stat, gd->ctx,
				&bufin, &bufout, &qop_state);

			if (maj_stat != GSS_S_COMPLETE
					|| qop_state != gd->sec.qop) {
				log_status("gss_verify_mic", maj_stat, min_stat);
				if (maj_stat == GSS_S_CONTEXT_EXPIRED) {
					gd->established = FALSE;
					authgss_destroy_context(auth);
				}
				return (FALSE);
			}
			gd->established = TRUE;
			gd->gc.gc_proc = RPCSEC_GSS_DATA;
			gd->gc.gc_seq = 0;
			gd->win = gr.gr_win;
			break;
		}
	}
	/* End context negotiation loop. */
	if (gd->gc.gc_proc != RPCSEC_GSS_DATA) {
		if (gr.gr_token.length != 0)
			gss_release_buffer(&min_stat, &gr.gr_token);

		authgss_destroy(auth);
		auth = NULL;
		rpc_createerr.cf_stat = RPC_AUTHERROR;

		return (FALSE);
	}
	return (TRUE);
}

bool_t
authgss_service(AUTH *auth, int svc)
{
	struct rpc_gss_data	*gd;

	log_debug("in authgss_service()");

	if (!auth)
		return(FALSE);
	gd = AUTH_PRIVATE(auth);
	if (!gd || !gd->established)
		return (FALSE);
	gd->sec.svc = svc;
	gd->gc.gc_svc = svc;
	return (TRUE);
}

static void
authgss_destroy_context(AUTH *auth)
{
	struct rpc_gss_data	*gd;
	OM_uint32		 min_stat;

	log_debug("in authgss_destroy_context()");

	gd = AUTH_PRIVATE(auth);

	if (gd->gc.gc_ctx.length != 0) {
		if (gd->established) {
			gd->gc.gc_proc = RPCSEC_GSS_DESTROY;
			clnt_call(gd->clnt, NULLPROC, (xdrproc_t)xdr_void, NULL,
				  (xdrproc_t)xdr_void, NULL, AUTH_TIMEOUT);
		}
		gss_release_buffer(&min_stat, &gd->gc.gc_ctx);
		/* XXX ANDROS check size of context  - should be 8 */
		memset(&gd->gc.gc_ctx, 0, sizeof(gd->gc.gc_ctx));
	}
	if (gd->ctx != GSS_C_NO_CONTEXT) {
		gss_delete_sec_context(&min_stat, &gd->ctx, NULL);
		gd->ctx = GSS_C_NO_CONTEXT;
	}

	/* free saved wire verifier (if any) */
	mem_free(gd->gc_wire_verf.value, gd->gc_wire_verf.length);
	gd->gc_wire_verf.value = NULL;
	gd->gc_wire_verf.length = 0;

	gd->established = FALSE;
}

static void
authgss_destroy(AUTH *auth)
{
	struct rpc_gss_data	*gd;
	OM_uint32		 min_stat;

	log_debug("in authgss_destroy()");

	gd = AUTH_PRIVATE(auth);

	authgss_destroy_context(auth);

#ifdef DEBUG
	fprintf(stderr, "authgss_destroy: freeing name %p\n", gd->name);
#endif
	if (gd->name != GSS_C_NO_NAME)
		gss_release_name(&min_stat, &gd->name);

	free(gd);
	free(auth);
}

bool_t
authgss_wrap(AUTH *auth, XDR *xdrs, xdrproc_t xdr_func, caddr_t xdr_ptr)
{
	struct rpc_gss_data	*gd;

	log_debug("in authgss_wrap()");

	gd = AUTH_PRIVATE(auth);

	if (!gd->established || gd->sec.svc == RPCSEC_GSS_SVC_NONE) {
		return ((*xdr_func)(xdrs, xdr_ptr));
	}
	return (xdr_rpc_gss_data(xdrs, xdr_func, xdr_ptr,
				 gd->ctx, gd->sec.qop,
				 gd->sec.svc, gd->gc.gc_seq));
}

bool_t
authgss_unwrap(AUTH *auth, XDR *xdrs, xdrproc_t xdr_func, caddr_t xdr_ptr)
{
	struct rpc_gss_data	*gd;

	log_debug("in authgss_unwrap()");

	gd = AUTH_PRIVATE(auth);

	if (!gd->established || gd->sec.svc == RPCSEC_GSS_SVC_NONE) {
		return ((*xdr_func)(xdrs, xdr_ptr));
	}
	return (xdr_rpc_gss_data(xdrs, xdr_func, xdr_ptr,
				 gd->ctx, gd->sec.qop,
				 gd->sec.svc, gd->gc.gc_seq));
}
