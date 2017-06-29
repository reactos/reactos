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
#include <stdio.h>
#include <stdlib.h>
//#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <rpc/types.h>
#include <rpc/xdr.h>
#include <rpc/auth.h>
#include <rpc/auth_sspi.h>
#include <rpc/clnt.h>

static void authsspi_nextverf(AUTH *auth);
static bool_t authsspi_marshal(AUTH *auth, XDR *xdrs, u_int *seq);
static bool_t authsspi_refresh(AUTH *auth, void *);
static bool_t authsspi_validate(AUTH *auth, struct opaque_auth *verf, u_int seq);
static void	authsspi_destroy(AUTH *auth);
static void	authsspi_destroy_context(AUTH *auth);
static bool_t authsspi_wrap(AUTH *auth, XDR *xdrs, xdrproc_t xdr_func, caddr_t xdr_ptr);
static bool_t authsspi_unwrap(AUTH *auth, XDR *xdrs, xdrproc_t xdr_func, caddr_t xdr_ptr, u_int seq);

static struct auth_ops authsspi_ops = {
	authsspi_nextverf,
	authsspi_marshal,
	authsspi_validate,
	authsspi_refresh,
	authsspi_destroy,
	authsspi_wrap,
	authsspi_unwrap
};

struct rpc_sspi_data {
	bool_t              established;	/* context established */
    bool_t              inprogress;
	sspi_buffer_desc    gc_wire_verf;	/* save GSS_S_COMPLETE NULL RPC verfier
                                         * to process at end of context negotiation*/
	CLIENT              *clnt;		    /* client handle */
	sspi_name_t         name;		    /* service name */
	struct rpc_sspi_sec *sec;		    /* security tuple */
    CtxtHandle          ctx;            /* context id */
	struct rpc_sspi_cred gc;		    /* client credentials */
	u_int               win;		    /* sequence window */
    TimeStamp           expiry;
};

#define	AUTH_PRIVATE(auth)	((struct rpc_sspi_data *)auth->ah_private)

static struct timeval AUTH_TIMEOUT = { 25, 0 };
void print_rpc_gss_sec(struct rpc_sspi_sec *ptr);
void print_negotiated_attrs(PCtxtHandle ctx);

AUTH *
authsspi_create(CLIENT *clnt, sspi_name_t name, struct rpc_sspi_sec *sec)
{
	AUTH *auth, *save_auth;
	struct rpc_sspi_data *gd;

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

#if 0
	if (name != SSPI_C_NO_NAME) {
		if (gss_duplicate_name(&min_stat, name, &gd->name)
						!= GSS_S_COMPLETE) {
			rpc_createerr.cf_stat = RPC_SYSTEMERROR;
			rpc_createerr.cf_error.re_errno = ENOMEM;
			free(auth);
			return (NULL);
		}
	}
	else
#else
    gd->name = strdup(name);
#endif

	gd->clnt = clnt;
	SecInvalidateHandle(&gd->ctx);
	gd->sec = sec;

	gd->gc.gc_v = RPCSEC_SSPI_VERSION;
	gd->gc.gc_proc = RPCSEC_SSPI_INIT;
	gd->gc.gc_svc = gd->sec->svc;

	auth->ah_ops = &authsspi_ops;
	auth->ah_private = (caddr_t)gd;

	save_auth = clnt->cl_auth;
	clnt->cl_auth = auth;

	if (!authsspi_refresh(auth, NULL))
		auth = NULL;

	clnt->cl_auth = save_auth;

	return (auth);
}

AUTH *
authsspi_create_default(CLIENT *clnt, char *service, int svc)
{
	AUTH *auth = NULL;
	uint32_t maj_stat = 0;
	sspi_buffer_desc sname;
    sspi_name_t name = SSPI_C_NO_NAME;
    unsigned char sec_pkg_name[] = "Kerberos";
    struct rpc_sspi_sec *sec;

	log_debug("in authgss_create_default() for %s", service);

	sname.value = service;
	sname.length = (int)strlen(service);
#if 0
	maj_stat = gss_import_name(&min_stat, &sname,
		(gss_OID)GSS_C_NT_HOSTBASED_SERVICE,
		&name);
#else
    maj_stat = sspi_import_name(&sname, &name);
#endif
	if (maj_stat != SEC_E_OK) {
		log_debug("authgss_create_default: sspi_import_name failed with %x", maj_stat);
		return (NULL);
	}
    sec = calloc(1, sizeof(struct rpc_sspi_sec));
    if (sec == NULL)
        goto out_err;
    sec->svc = svc;
    // Let's acquire creds here for now
    maj_stat = AcquireCredentialsHandleA(NULL, sec_pkg_name, SECPKG_CRED_BOTH,
        NULL, NULL, NULL, NULL, &sec->cred, &sec->expiry);
    if (maj_stat != SEC_E_OK) {
        log_debug("authgss_create_default: AcquireCredentialsHandleA failed with %x", maj_stat);
        free(sec);
        goto out;
    }

	auth = authsspi_create(clnt, name, sec);
    if (auth == NULL)
        goto out_free_sec;

out:
	if (name != SSPI_C_NO_NAME) {
#if 0
 		gss_release_name(&min_stat, &name);
#else
        free(name);
#endif
	}

	return (auth);
out_free_sec:
    if (rpc_createerr.cf_error.re_errno == ENOMEM) {
        FreeCredentialsHandle(&sec->cred);
        free(sec);
    }
out_err:
    rpc_createerr.cf_stat = RPC_SYSTEMERROR;
	rpc_createerr.cf_error.re_errno = ENOMEM;
    goto out;
}

static void
authsspi_nextverf(AUTH *auth)
{
	log_debug("in authgss_nextverf()");
	/* no action necessary */
}

static bool_t
authsspi_marshal(AUTH *auth, XDR *xdrs, u_int *seq)
{
	XDR tmpxdrs;
	char tmp[MAX_AUTH_BYTES];
	struct rpc_sspi_data *gd;
	sspi_buffer_desc rpcbuf, checksum;
	uint32_t maj_stat;
	bool_t xdr_stat;

    log_debug("in authgss_marshal()");

	gd = AUTH_PRIVATE(auth);

	if (gd->established) {
		gd->gc.gc_seq++;
        *seq = gd->gc.gc_seq;
    }

	xdrmem_create(&tmpxdrs, tmp, sizeof(tmp), XDR_ENCODE);

	if (!xdr_rpc_sspi_cred(&tmpxdrs, &gd->gc)) {
        log_debug("authsspi_marshal: xdr_rpc_sspi_cred failed");
		XDR_DESTROY(&tmpxdrs);
		return (FALSE);
	}
	auth->ah_cred.oa_flavor = RPCSEC_GSS;
	auth->ah_cred.oa_base = tmp;
	auth->ah_cred.oa_length = XDR_GETPOS(&tmpxdrs);

	XDR_DESTROY(&tmpxdrs);

	if (!xdr_opaque_auth(xdrs, &auth->ah_cred)) {
        log_debug("authsspi_marshal: failed to xdr GSS CRED");
		return (FALSE);
    }
	if (gd->gc.gc_proc == RPCSEC_SSPI_INIT ||
	    gd->gc.gc_proc == RPCSEC_SSPI_CONTINUE_INIT) {
		return (xdr_opaque_auth(xdrs, &_null_auth));
	}
	/* Checksum serialized RPC header, up to and including credential. */
	rpcbuf.length = XDR_GETPOS(xdrs) - 4;
	//XDR_SETPOS(xdrs, 0);
	//rpcbuf.value = XDR_INLINE(xdrs, rpcbuf.length);
	rpcbuf.value = xdrrec_getoutbase(xdrs) + 1;

#if 0
	maj_stat = gss_get_mic(&min_stat, gd->ctx, gd->sec.qop,
			    &rpcbuf, &checksum);
#else
    maj_stat = sspi_get_mic(&gd->ctx, 0, gd->gc.gc_seq, &rpcbuf, &checksum);
#endif
	if (maj_stat != SEC_E_OK) {
		log_debug("authsspi_marshal: sspi_get_mic failed with %x", maj_stat);
		if (maj_stat == SEC_E_NO_AUTHENTICATING_AUTHORITY) {
			gd->established = FALSE;
			authsspi_destroy_context(auth);
		}
		return (FALSE);
	}
	auth->ah_verf.oa_flavor = RPCSEC_GSS;
	auth->ah_verf.oa_base = checksum.value;
	auth->ah_verf.oa_length = checksum.length;
	xdr_stat = xdr_opaque_auth(xdrs, &auth->ah_verf);
#if 0
	gss_release_buffer(&min_stat, &checksum);
#else
    sspi_release_buffer(&checksum);
#endif
	return (xdr_stat);
}

static bool_t
authsspi_validate(AUTH *auth, struct opaque_auth *verf, u_int seq)
{
	struct rpc_sspi_data *gd;
	u_int num, qop_state, cur_seq;
	sspi_buffer_desc signbuf, checksum;
	uint32_t maj_stat;

	log_debug("in authgss_validate(for seq=%d)", seq);

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
			return (FALSE);
		}
		memcpy(gd->gc_wire_verf.value, verf->oa_base, verf->oa_length);
		gd->gc_wire_verf.length = verf->oa_length;
		return (TRUE);
  	}

    if (gd->gc.gc_proc == RPCSEC_SSPI_DESTROY) 
        return TRUE;

	if (gd->gc.gc_proc == RPCSEC_SSPI_INIT ||
	        gd->gc.gc_proc == RPCSEC_SSPI_CONTINUE_INIT) {
		num = htonl(gd->win);
	}
	else {
        if (seq == -1) {
            num = htonl(gd->gc.gc_seq);
            cur_seq = gd->gc.gc_seq;
        }
	    else {
            num = htonl(seq);
            cur_seq = seq;
        }
    }

	signbuf.value = &num;
	signbuf.length = sizeof(num);

	checksum.value = verf->oa_base;
	checksum.length = verf->oa_length;
#if 0
	maj_stat = gss_verify_mic(&min_stat, gd->ctx, &signbuf,
				  &checksum, &qop_state);
#else
    maj_stat = sspi_verify_mic(&gd->ctx, cur_seq, &signbuf, &checksum, &qop_state);
#endif
	if (maj_stat != SEC_E_OK) {
		log_debug("authsspi_validate: VerifySignature failed with %x", maj_stat);
		if (maj_stat == SEC_E_NO_AUTHENTICATING_AUTHORITY) {
			gd->established = FALSE;
			authsspi_destroy_context(auth);
		}
		return (FALSE);
	}
	return (TRUE);
}

static bool_t
authsspi_refresh(AUTH *auth, void *tmp)
{
	struct rpc_sspi_data *gd;
	struct rpc_sspi_init_res gr;
    sspi_buffer_desc *recv_tokenp, send_token;
	uint32_t maj_stat, call_stat, ret_flags, i;
    unsigned long flags = 
        ISC_REQ_MUTUAL_AUTH|ISC_REQ_INTEGRITY|ISC_REQ_ALLOCATE_MEMORY;
    SecBufferDesc out_desc, in_desc;
    SecBuffer wtkn[1], rtkn[1];    

    log_debug("in authgss_refresh()");

	gd = AUTH_PRIVATE(auth);

	if ((gd->established && tmp == NULL) || gd->inprogress)
		return (TRUE);
    else if (tmp) {
        log_debug("trying to refresh credentials\n");
        DeleteSecurityContext(&gd->ctx);
        sspi_release_buffer(&gd->gc.gc_ctx);
        SecInvalidateHandle(&gd->ctx);
        mem_free(gd->gc_wire_verf.value, gd->gc_wire_verf.length);
        gd->gc_wire_verf.value = NULL;
        gd->gc_wire_verf.length = 0;
        gd->established = FALSE;        
        gd->gc.gc_proc = RPCSEC_SSPI_INIT;
    }

	/* GSS context establishment loop. */
	memset(&gr, 0, sizeof(gr));
	recv_tokenp = SSPI_C_NO_BUFFER;
    send_token.length = 0;
    send_token.value = NULL;

	print_rpc_gss_sec(gd->sec);

    if (gd->sec->svc == RPCSEC_SSPI_SVC_PRIVACY)
        flags |= ISC_REQ_CONFIDENTIALITY;

    for (i=0;;i++) {
		/* print the token we just received */
		if (recv_tokenp != SSPI_C_NO_BUFFER) {
			log_debug("The token we just received (length %d):",
				  recv_tokenp->length);
			log_hexdump(0, "", recv_tokenp->value, recv_tokenp->length, 0);
		}
#if 0
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
#else
        gd->inprogress = TRUE;
        out_desc.cBuffers = 1;
        out_desc.pBuffers = wtkn;
        out_desc.ulVersion = SECBUFFER_VERSION;
        wtkn[0].BufferType = SECBUFFER_TOKEN;
        wtkn[0].cbBuffer = send_token.length;
        wtkn[0].pvBuffer = send_token.value;
        log_debug("calling InitializeSecurityContextA for %s", gd->name);

        maj_stat = InitializeSecurityContextA(
                        &gd->sec->cred, 
                        ((i==0)?NULL:&gd->ctx),
                        gd->name, 
                        flags, 
                        0, 
                        SECURITY_NATIVE_DREP,  
                        ((i==0)?NULL:&in_desc),
                        0, 
                        &gd->ctx, 
                        &out_desc, 
                        &ret_flags, 
                        &gd->expiry);
#endif
		if (recv_tokenp != SSPI_C_NO_BUFFER) {
#if 0
			gss_release_buffer(&min_stat, &gr.gr_token);
#else
            sspi_release_buffer(&gr.gr_token);
#endif
			recv_tokenp = SSPI_C_NO_BUFFER;
		}
		if (maj_stat != SEC_E_OK && maj_stat != SEC_I_CONTINUE_NEEDED) {
			log_debug("InitializeSecurityContext failed with %x", maj_stat);
			break;
		}
        send_token.length = wtkn[0].cbBuffer;
        send_token.value = wtkn[0].pvBuffer;
		if (send_token.length != 0) {
			memset(&gr, 0, sizeof(gr));

			/* print the token we are about to send */
			log_debug("The token being sent (length %d):",
				  send_token.length);
			log_hexdump(0, "", send_token.value, send_token.length, 0);

			call_stat = clnt_call(gd->clnt, NULLPROC,
					      (xdrproc_t)xdr_rpc_sspi_init_args,
					      &send_token,
					      (xdrproc_t)xdr_rpc_sspi_init_res,
					      (caddr_t)&gr, AUTH_TIMEOUT);
#if 0
			gss_release_buffer(&min_stat, &send_token);
#else
            // 11/29/2010 [aglo] can't call sspi_relase_buffer, causes heap 
            // corruption (later) to try and free the buffer directly.
            FreeContextBuffer(send_token.value);
#endif
			if (call_stat != RPC_SUCCESS ||
			    (gr.gr_major != SEC_E_OK &&
			     gr.gr_major != SEC_I_CONTINUE_NEEDED))
				break;

			if (gr.gr_ctx.length != 0) {
#if 0
				if (gd->gc.gc_ctx.value)
					gss_release_buffer(&min_stat,
							   &gd->gc.gc_ctx);
#else 
                sspi_release_buffer(&gd->gc.gc_ctx);
#endif
				gd->gc.gc_ctx = gr.gr_ctx;
			}
			if (gr.gr_token.length != 0) {
				if (maj_stat != SEC_I_CONTINUE_NEEDED)
					break;
				recv_tokenp = &gr.gr_token;
                in_desc.cBuffers = 1;
                in_desc.pBuffers = rtkn;
                in_desc.ulVersion = SECBUFFER_VERSION;
                rtkn[0].BufferType = SECBUFFER_TOKEN;
                rtkn[0].cbBuffer = gr.gr_token.length;
                rtkn[0].pvBuffer = gr.gr_token.value;
			}
			gd->gc.gc_proc = RPCSEC_SSPI_CONTINUE_INIT;
		}

		/* GSS_S_COMPLETE => check gss header verifier,
		 * usually checked in gss_validate
		 */
		if (maj_stat == SEC_E_OK) {
			sspi_buffer_desc bufin;
			u_int seq, qop_state = 0;
            
            print_negotiated_attrs(&gd->ctx);

			seq = htonl(gr.gr_win);
			bufin.value = (unsigned char *)&seq;
			bufin.length = sizeof(seq);
#if 0
			maj_stat = gss_verify_mic(&min_stat, gd->ctx,
				&bufin, &bufout, &qop_state);
#else
            maj_stat = sspi_verify_mic(&gd->ctx, 0, &bufin, &gd->gc_wire_verf, &qop_state);
#endif
			if (maj_stat != SEC_E_OK) {
				log_debug("authgss_refresh: sspi_verify_mic failed with %x", maj_stat);
				if (maj_stat == SEC_E_NO_AUTHENTICATING_AUTHORITY) {
					gd->established = FALSE;
					authsspi_destroy_context(auth);
				}
				break;
			}
			gd->established = TRUE;
            gd->inprogress = FALSE;
			gd->gc.gc_proc = RPCSEC_SSPI_DATA;
			gd->gc.gc_seq = 0;
			gd->win = gr.gr_win;
            log_debug("authgss_refresh: established GSS context");
			break;
		}
	}
	/* End context negotiation loop. */
	if (gd->gc.gc_proc != RPCSEC_SSPI_DATA) {
		if (gr.gr_token.length != 0)
#if 0
			gss_release_buffer(&min_stat, &gr.gr_token);
#else
            sspi_release_buffer(&gr.gr_token);
#endif
		authsspi_destroy(auth);
		auth = NULL;
		rpc_createerr.cf_stat = RPC_AUTHERROR;

		return (FALSE);
	}
	return (TRUE);
}

bool_t
authsspi_service(AUTH *auth, int svc)
{
	struct rpc_sspi_data	*gd;

	log_debug("in authgss_service()");

	if (!auth) 
        return(FALSE);
	gd = AUTH_PRIVATE(auth);
	if (!gd || !gd->established)
		return (FALSE);
	gd->sec->svc = svc;
	gd->gc.gc_svc = svc;
	return (TRUE);
}

static void
authsspi_destroy_context(AUTH *auth)
{
	struct rpc_sspi_data *gd;

	log_debug("in authgss_destroy_context()");

	gd = AUTH_PRIVATE(auth);
    if (gd == NULL) return;

	if (SecIsValidHandle(&gd->ctx)) {
		if (gd->established) {
			gd->gc.gc_proc = RPCSEC_SSPI_DESTROY;
			clnt_call(gd->clnt, NULLPROC, (xdrproc_t)xdr_void, NULL,
				  (xdrproc_t)xdr_void, NULL, AUTH_TIMEOUT);
            DeleteSecurityContext(&gd->ctx);
		}
        sspi_release_buffer(&gd->gc.gc_ctx);
        SecInvalidateHandle(&gd->ctx);
#if 0
		gss_release_buffer(&min_stat, &gd->gc.gc_ctx);
		/* XXX ANDROS check size of context  - should be 8 */
		memset(&gd->gc.gc_ctx, 0, sizeof(gd->gc.gc_ctx));
		gss_delete_sec_context(&min_stat, &gd->ctx, NULL);
#endif
	}

	/* free saved wire verifier (if any) */
	mem_free(gd->gc_wire_verf.value, gd->gc_wire_verf.length);
	gd->gc_wire_verf.value = NULL;
	gd->gc_wire_verf.length = 0;

	gd->established = FALSE;
}

static void
authsspi_destroy(AUTH *auth)
{
	struct rpc_sspi_data *gd;

	log_debug("in authgss_destroy()");

	gd = AUTH_PRIVATE(auth);
    if (gd == NULL) return;

	authsspi_destroy_context(auth);

#if 0
    if (gd->name != SSPI_C_NO_NAME)
		gss_release_name(&min_stat, &gd->name);
#else
    free(gd->name);
#endif
    FreeCredentialsHandle(&gd->sec->cred);
    free(gd->sec);
	free(gd);
	free(auth);
}

bool_t
authsspi_wrap(AUTH *auth, XDR *xdrs, xdrproc_t xdr_func, caddr_t xdr_ptr)
{
	struct rpc_sspi_data	*gd;

	log_debug("in authgss_wrap()");

	gd = AUTH_PRIVATE(auth);

	if (!gd->established || gd->sec->svc == RPCSEC_SSPI_SVC_NONE) {
		return ((*xdr_func)(xdrs, xdr_ptr));
	}
	return (xdr_rpc_sspi_data(xdrs, xdr_func, xdr_ptr,
				 &gd->ctx, gd->sec->qop,
				 gd->sec->svc, gd->gc.gc_seq));
}

bool_t
authsspi_unwrap(AUTH *auth, XDR *xdrs, xdrproc_t xdr_func, caddr_t xdr_ptr, u_int seq)
{
	struct rpc_sspi_data	*gd;

	log_debug("in authgss_unwrap()");

	gd = AUTH_PRIVATE(auth);

	if (!gd->established || gd->sec->svc == RPCSEC_SSPI_SVC_NONE) {
		return ((*xdr_func)(xdrs, xdr_ptr));
	}
	return (xdr_rpc_sspi_data(xdrs, xdr_func, xdr_ptr,
				 &gd->ctx, gd->sec->qop,
				 gd->sec->svc, seq));
}

#ifdef __REACTOS__
uint32_t sspi_get_mic(void *dummy, u_int qop, u_int seq, 
                        sspi_buffer_desc *bufin, sspi_buffer_desc *bufout)
{
    PCtxtHandle ctx = dummy;
#else
uint32_t sspi_get_mic(PCtxtHandle ctx, u_int qop, u_int seq, 
                        sspi_buffer_desc *bufin, sspi_buffer_desc *bufout)
{
#endif
    uint32_t maj_stat;
    SecPkgContext_Sizes ContextSizes;
    SecBufferDesc desc;
    SecBuffer sec_tkn[2];    

    log_hexdump(0, "sspi_get_mic: calculating checksum of", bufin->value, bufin->length, 0);

    memset(&ContextSizes, 0, sizeof(ContextSizes));
    maj_stat = QueryContextAttributesA(ctx, SECPKG_ATTR_SIZES, &ContextSizes);
    if (maj_stat != SEC_E_OK) return maj_stat;

    if (ContextSizes.cbMaxSignature == 0) return SEC_E_INTERNAL_ERROR;

    desc.cBuffers = 2;
    desc.pBuffers = sec_tkn;
    desc.ulVersion = SECBUFFER_VERSION;
    sec_tkn[0].BufferType = SECBUFFER_DATA;
    sec_tkn[0].cbBuffer = bufin->length;
    sec_tkn[0].pvBuffer = bufin->value;
    sec_tkn[1].BufferType = SECBUFFER_TOKEN;
    sec_tkn[1].cbBuffer = ContextSizes.cbMaxSignature;
    sec_tkn[1].pvBuffer = calloc(ContextSizes.cbMaxSignature, sizeof(char));
    if (sec_tkn[1].pvBuffer == NULL) return SEC_E_INSUFFICIENT_MEMORY;

    maj_stat = MakeSignature(ctx, 0, &desc, seq);
    if (maj_stat == SEC_E_OK) {
        bufout->length = sec_tkn[1].cbBuffer;
        bufout->value = sec_tkn[1].pvBuffer;
        log_hexdump(0, "sspi_get_mic: verifier is", bufout->value, bufout->length, 0);
    } else
        free(sec_tkn[1].pvBuffer);

    return maj_stat;
}

#ifndef __REACTOS__
uint32_t sspi_verify_mic(PCtxtHandle ctx, u_int seq, sspi_buffer_desc *bufin, 
                            sspi_buffer_desc *bufout, u_int *qop_state)
{
#else
uint32_t sspi_verify_mic(void *dummy, u_int seq, sspi_buffer_desc *bufin, 
                            sspi_buffer_desc *bufout, u_int *qop_state)
{
    PCtxtHandle ctx = dummy;
#endif
    SecBufferDesc desc;
    SecBuffer sec_tkn[2];    

    desc.cBuffers = 2;
    desc.pBuffers = sec_tkn;
    desc.ulVersion = SECBUFFER_VERSION;
    sec_tkn[0].BufferType = SECBUFFER_DATA;
    sec_tkn[0].cbBuffer = bufin->length;
    sec_tkn[0].pvBuffer = bufin->value;
    sec_tkn[1].BufferType = SECBUFFER_TOKEN;
    sec_tkn[1].cbBuffer = bufout->length;
    sec_tkn[1].pvBuffer = bufout->value;

    log_hexdump(0, "sspi_verify_mic: calculating checksum over", bufin->value, bufin->length, 0);
    log_hexdump(0, "sspi_verify_mic: received checksum ", bufout->value, bufout->length, 0);

    return VerifySignature(ctx, &desc, seq, qop_state);
}

void sspi_release_buffer(sspi_buffer_desc *buf)
{
    if (buf->value)
        free(buf->value);
    buf->value = NULL;
    buf->length = 0;
}

uint32_t sspi_import_name(sspi_buffer_desc *name_in, sspi_name_t *name_out)
{
    *name_out = calloc(name_in->length + 5, sizeof(char));
    if (*name_out == NULL)
        return SEC_E_INSUFFICIENT_MEMORY;

    strcpy(*name_out, "nfs/");
    strncat(*name_out, name_in->value, name_in->length);

    log_debug("imported service name is: %s\n", *name_out);

    return SEC_E_OK;
}

#ifndef __REACTOS__
uint32_t sspi_wrap(PCtxtHandle ctx, u_int seq, sspi_buffer_desc *bufin, 
                   sspi_buffer_desc *bufout, u_int *conf_state)
{
#else
uint32_t sspi_wrap(void *dummy, u_int seq, sspi_buffer_desc *bufin, 
                   sspi_buffer_desc *bufout, u_int *conf_state)
{
    PCtxtHandle ctx = dummy;
#endif
    uint32_t maj_stat;
    SecBufferDesc BuffDesc;
    SecBuffer SecBuff[3];
    ULONG ulQop = 0;
    SecPkgContext_Sizes ContextSizes;
    PBYTE p;

    maj_stat = QueryContextAttributes(ctx, SECPKG_ATTR_SIZES,
       &ContextSizes);
    if (maj_stat != SEC_E_OK) 
        goto out;

    BuffDesc.ulVersion = 0;
    BuffDesc.cBuffers = 3;
    BuffDesc.pBuffers = SecBuff;

    SecBuff[0].cbBuffer = ContextSizes.cbSecurityTrailer;
    SecBuff[0].BufferType = SECBUFFER_TOKEN;
    SecBuff[0].pvBuffer = malloc(ContextSizes.cbSecurityTrailer);

    SecBuff[1].cbBuffer = bufin->length;
    SecBuff[1].BufferType = SECBUFFER_DATA;
    SecBuff[1].pvBuffer = bufin->value;
    log_hexdump(0, "plaintext:", bufin->value, bufin->length, 0);

    SecBuff[2].cbBuffer = ContextSizes.cbBlockSize;
    SecBuff[2].BufferType = SECBUFFER_PADDING;
    SecBuff[2].pvBuffer = malloc(ContextSizes.cbBlockSize);

    maj_stat = EncryptMessage(ctx, ulQop, &BuffDesc, seq);
    if (maj_stat != SEC_E_OK)
        goto out_free;

    bufout->length = SecBuff[0].cbBuffer + SecBuff[1].cbBuffer + SecBuff[2].cbBuffer;
    p = bufout->value = malloc(bufout->length);
    memcpy(p, SecBuff[0].pvBuffer, SecBuff[0].cbBuffer);
    p += SecBuff[0].cbBuffer;
    memcpy(p, SecBuff[1].pvBuffer, SecBuff[1].cbBuffer);
    p += SecBuff[1].cbBuffer;
    memcpy(p, SecBuff[2].pvBuffer, SecBuff[2].cbBuffer);
out_free:
    free(SecBuff[0].pvBuffer);
    free(SecBuff[2].pvBuffer);

    if (!maj_stat)
        log_hexdump(0, "cipher:", bufout->value, bufout->length, 0);
out:
    return maj_stat;
}

#ifndef __REACTOS__
uint32_t sspi_unwrap(PCtxtHandle ctx, u_int seq, sspi_buffer_desc *bufin, 
                     sspi_buffer_desc *bufout, u_int *conf_state, 
                     u_int *qop_state)
{
#else
uint32_t sspi_unwrap(void *dummy, u_int seq, sspi_buffer_desc *bufin, 
                     sspi_buffer_desc *bufout, u_int *conf_state, 
                     u_int *qop_state)
{
    PCtxtHandle ctx = dummy;
#endif
    uint32_t maj_stat;
    SecBufferDesc BuffDesc;
    SecBuffer SecBuff[2];
    ULONG ulQop = 0;

    BuffDesc.ulVersion    = 0;
    BuffDesc.cBuffers     = 2;
    BuffDesc.pBuffers     = SecBuff;

    SecBuff[0].cbBuffer   = bufin->length;
    SecBuff[0].BufferType = SECBUFFER_STREAM;
    SecBuff[0].pvBuffer   = bufin->value;

    SecBuff[1].cbBuffer   = 0;
    SecBuff[1].BufferType = SECBUFFER_DATA;
    SecBuff[1].pvBuffer   = NULL;

    log_hexdump(0, "cipher:", bufin->value, bufin->length, 0);

    maj_stat = DecryptMessage(ctx, &BuffDesc, seq, &ulQop);
    if (maj_stat != SEC_E_OK) return maj_stat;

    bufout->length = SecBuff[1].cbBuffer;
    bufout->value = malloc(bufout->length);
    memcpy(bufout->value, SecBuff[1].pvBuffer, bufout->length);

    log_hexdump(0, "data:", bufout->value, bufout->length, 0);

    *conf_state = 1;
    *qop_state = 0;

    return SEC_E_OK;
}

/* useful as i add more mechanisms */
#define DEBUG
#ifdef DEBUG
#define fd_out stdout
void print_rpc_gss_sec(struct rpc_sspi_sec *ptr)
{
    int i;
    char *p;

	fprintf(fd_out, "rpc_gss_sec:");
	if(ptr->mech == NULL)
		fprintf(fd_out, "NULL gss_OID mech");
	else {
		fprintf(fd_out, "     mechanism_OID: {");
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
				fprintf(fd_out, " %u %u", first, second);
				p++;
			}
			else {
				fprintf(fd_out, " %u", (unsigned char)*p++);
			}
		fprintf(fd_out, " }\n");
	}
	fprintf(fd_out, "     qop: %d\n", ptr->qop);
	fprintf(fd_out, "     service: %d\n", ptr->svc);
	fprintf(fd_out, "     cred: %p\n", ptr->cred);
}

void print_negotiated_attrs(PCtxtHandle ctx)
{
    SecPkgContext_Sizes ContextSizes;
    unsigned long  flags;
    uint32_t maj_stat;

    maj_stat = QueryContextAttributesA(ctx, SECPKG_ATTR_FLAGS, &flags);
    if (maj_stat != SEC_E_OK) return;

    log_debug("negotiated flags %x\n", flags);
    if (flags & ISC_REQ_DELEGATE) log_debug("ISC_REQ_DELEGATE");
    if (flags & ISC_REQ_MUTUAL_AUTH) log_debug("ISC_REQ_MUTUAL_AUTH");
    if (flags & ISC_REQ_REPLAY_DETECT) log_debug("ISC_REQ_REPLAY_DETECT");
    if (flags & ISC_REQ_SEQUENCE_DETECT) log_debug("ISC_REQ_SEQUENCE_DETECT");
    if (flags & ISC_REQ_CONFIDENTIALITY) log_debug("ISC_REQ_CONFIDENTIALITY");
    if (flags & ISC_REQ_USE_SESSION_KEY) log_debug("ISC_REQ_USE_SESSION_KEY");
    if (flags & ISC_REQ_PROMPT_FOR_CREDS) log_debug("ISC_REQ_PROMPT_FOR_CREDS");
    if (flags & ISC_REQ_USE_SUPPLIED_CREDS) log_debug("ISC_REQ_USE_SUPPLIED_CREDS");
    if (flags & ISC_REQ_ALLOCATE_MEMORY) log_debug("ISC_REQ_ALLOCATE_MEMORY");
    if (flags & ISC_REQ_USE_DCE_STYLE) log_debug("ISC_REQ_USE_DCE_STYLE");
    if (flags & ISC_REQ_DATAGRAM) log_debug("ISC_REQ_DATAGRAM");
    if (flags & ISC_REQ_CONNECTION) log_debug("ISC_REQ_CONNECTION");
    if (flags & ISC_REQ_CALL_LEVEL) log_debug("ISC_REQ_CALL_LEVEL");
    if (flags & ISC_REQ_FRAGMENT_SUPPLIED) log_debug("ISC_REQ_FRAGMENT_SUPPLIED");
    if (flags & ISC_REQ_EXTENDED_ERROR) log_debug("ISC_REQ_EXTENDED_ERROR");
    if (flags & ISC_REQ_STREAM) log_debug("ISC_REQ_STREAM");
    if (flags & ISC_REQ_INTEGRITY) log_debug("ISC_REQ_INTEGRITY");
    if (flags & ISC_REQ_IDENTIFY) log_debug("ISC_REQ_IDENTIFY");
    if (flags & ISC_REQ_NULL_SESSION) log_debug("ISC_REQ_NULL_SESSION");
    if (flags & ISC_REQ_MANUAL_CRED_VALIDATION) log_debug("ISC_REQ_MANUAL_CRED_VALIDATION");

    maj_stat = QueryContextAttributesA(ctx, SECPKG_ATTR_SIZES, &ContextSizes);
    if (maj_stat != SEC_E_OK) return;

    log_debug("signature size is %d\n", ContextSizes.cbMaxSignature);

}

void log_hexdump(bool_t on, const u_char *title, const u_char *buf, 
                    int len, int offset)
{
	int i, j, jm, c;

    if (!on) return;

	fprintf(fd_out, "%04x: %s (len=%d)\n", GetCurrentThreadId(), title, len);
	for (i = 0; i < len; i += 0x10) {
		fprintf(fd_out, "  %04x: ", (u_int)(i + offset));
		jm = len - i;
		jm = jm > 16 ? 16 : jm;

		for (j = 0; j < jm; j++) {
			if ((j % 2) == 1)
				fprintf(fd_out, "%02x ", (u_int) buf[i+j]);
			else
				fprintf(fd_out, "%02x", (u_int) buf[i+j]);
		}
		for (; j < 16; j++) {
			if ((j % 2) == 1) fprintf(fd_out, "   ");
			else fprintf(fd_out, "  ");
		}
		fprintf(fd_out, " ");

		for (j = 0; j < jm; j++) {
			c = buf[i+j];
			c = isprint(c) ? c : '.';
			fprintf(fd_out, "%c", c);
		}
		fprintf(fd_out, "\n");
	}
    fflush(fd_out);
}

void log_debug(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	fprintf(fd_out, "%04x: rpcsec_gss: ", GetCurrentThreadId());
	vfprintf(fd_out, fmt, ap);
	fprintf(fd_out, "\n");
    fflush(fd_out);
	va_end(ap);
}
#else
void print_rpc_gss_sec(struct rpc_sspi_sec *ptr) { return; }
void print_negotiated_flags(unsigned long  flags) {return; }
void log_hexdump(bool_t on, const u_char *title, const u_char *buf, 
                    int len, int offset) { return; }
void log_debug(const char *fmt, ...) { return; }
#endif
