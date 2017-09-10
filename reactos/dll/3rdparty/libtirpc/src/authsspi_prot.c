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
#include <stdarg.h>
//#include <rpc/types.h>
#include <rpc/xdr.h>
#include <rpc/auth.h>
#include <rpc/auth_sspi.h>
#include <rpc/rpc.h>
#include <security.h>

bool_t
xdr_rpc_sspi_cred(XDR *xdrs, struct rpc_sspi_cred *p)
{
	bool_t xdr_stat;

	xdr_stat = (xdr_u_int(xdrs, &p->gc_v) &&
		        xdr_enum(xdrs, (enum_t *)&p->gc_proc) &&
		        xdr_u_int(xdrs, &p->gc_seq) &&
		        xdr_enum(xdrs, (enum_t *)&p->gc_svc) &&
		        xdr_bytes(xdrs, (char **)&p->gc_ctx.value,
                (u_int *)&p->gc_ctx.length, MAX_AUTH_BYTES));

	log_debug("xdr_rpc_gss_cred: %s %s "
                "(v %d, proc %d, seq %d, svc %d, ctx %p:%d)",
                (xdrs->x_op == XDR_ENCODE) ? "encode" : "decode",
                (xdr_stat == TRUE) ? "success" : "failure",
                p->gc_v, p->gc_proc, p->gc_seq, p->gc_svc,
                p->gc_ctx.value, p->gc_ctx.length);

	return (xdr_stat);
}

bool_t
xdr_rpc_sspi_init_args(XDR *xdrs, sspi_buffer_desc *p)
{
	bool_t xdr_stat;

	xdr_stat = xdr_bytes(xdrs, (char **)&p->value,
                        (u_int *)&p->length, (u_int)(-1));

	log_debug("xdr_rpc_gss_init_args: %s %s (token %p:%d)",
                (xdrs->x_op == XDR_ENCODE) ? "encode" : "decode",
                (xdr_stat == TRUE) ? "success" : "failure",
                p->value, p->length);

	return (xdr_stat);
}

bool_t
xdr_rpc_sspi_init_res(XDR *xdrs, struct rpc_sspi_init_res *p)
{
	bool_t xdr_stat;

	xdr_stat = (xdr_bytes(xdrs, (char **)&p->gr_ctx.value,
                (u_int *)&p->gr_ctx.length, MAX_NETOBJ_SZ) &&
                xdr_u_int(xdrs, &p->gr_major) &&
                xdr_u_int(xdrs, &p->gr_minor) &&
                xdr_u_int(xdrs, &p->gr_win) &&
                xdr_bytes(xdrs, (char **)&p->gr_token.value,
                (u_int *)&p->gr_token.length, (u_int)(-1)));

	log_debug("xdr_rpc_gss_init_res %s %s "
                "(ctx %p:%d, maj %d, min %d, win %d, token %p:%d)",
                (xdrs->x_op == XDR_ENCODE) ? "encode" : "decode",
                (xdr_stat == TRUE) ? "success" : "failure",
                p->gr_ctx.value, p->gr_ctx.length,
                p->gr_major, p->gr_minor, p->gr_win,
                p->gr_token.value, p->gr_token.length);

	return (xdr_stat);
}

bool_t
xdr_rpc_sspi_wrap_data(XDR *xdrs, xdrproc_t xdr_func, caddr_t xdr_ptr,
		      PCtxtHandle ctx, sspi_qop_t qop,
		      rpc_sspi_svc_t svc, u_int seq)
{
	sspi_buffer_desc databuf, wrapbuf;
	uint32_t maj_stat;
	int start, end, conf_state;
	bool_t xdr_stat;

    log_debug("in xdr_rpc_sspi_wrap_data()");

    /* Skip databody length. */
	start = XDR_GETPOS(xdrs);
	XDR_SETPOS(xdrs, start + 4);

	/* Marshal rpc_gss_data_t (sequence number + arguments). */
	if (!xdr_u_int(xdrs, &seq) || !(*xdr_func)(xdrs, xdr_ptr))
		return (FALSE);
	end = XDR_GETPOS(xdrs);

	/* Set databuf to marshalled rpc_gss_data_t. */
	databuf.length = end - start - 4;
	XDR_SETPOS(xdrs, start + 4);
	databuf.value = XDR_INLINE(xdrs, databuf.length);

	xdr_stat = FALSE;

	if (svc == RPCSEC_SSPI_SVC_INTEGRITY) {
		/* Marshal databody_integ length. */
		XDR_SETPOS(xdrs, start);
		if (!xdr_u_int(xdrs, (u_int *)&databuf.length))
			return (FALSE);

		/* Checksum rpc_gss_data_t. */
#if 0
		maj_stat = gss_get_mic(&min_stat, ctx, qop,
				       &databuf, &wrapbuf);
#else
        maj_stat = sspi_get_mic(ctx, 0, seq, &databuf, &wrapbuf);
#endif
		if (maj_stat != SEC_E_OK) {
			log_debug("xdr_rpc_sspi_wrap_data: sspi_get_mic failed with %x", maj_stat);
			return (FALSE);
		}
		/* Marshal checksum. */
		XDR_SETPOS(xdrs, end);
		xdr_stat = xdr_bytes(xdrs, (char **)&wrapbuf.value,
                            (u_int *)&wrapbuf.length, (u_int)-1);
#if 0
		gss_release_buffer(&min_stat, &wrapbuf);
#else
        sspi_release_buffer(&wrapbuf);
#endif
	}
	else if (svc == RPCSEC_SSPI_SVC_PRIVACY) {
		/* Encrypt rpc_gss_data_t. */
#if 0
		maj_stat = gss_wrap(&min_stat, ctx, TRUE, qop, &databuf,
				    &conf_state, &wrapbuf);
#else
        maj_stat = sspi_wrap(ctx, 0, &databuf, &wrapbuf, &conf_state);
#endif
		if (maj_stat != SEC_E_OK) {
			log_debug("xdr_rpc_sspi_wrap_data: sspi_wrap failed with %x", maj_stat);
			return (FALSE);
		}
		/* Marshal databody_priv. */
		XDR_SETPOS(xdrs, start);
		xdr_stat = xdr_bytes(xdrs, (char **)&wrapbuf.value,
                            (u_int *)&wrapbuf.length, (u_int)-1);
#if 0
		gss_release_buffer(&min_stat, &wrapbuf);
#else
        sspi_release_buffer(&wrapbuf);
#endif
	}
	return (xdr_stat);
}

bool_t
xdr_rpc_sspi_unwrap_data(XDR *xdrs, xdrproc_t xdr_func, caddr_t xdr_ptr,
			PCtxtHandle ctx, sspi_qop_t qop,
			rpc_sspi_svc_t svc, u_int seq)
{
	XDR tmpxdrs;
	sspi_buffer_desc databuf, wrapbuf;
	uint32_t maj_stat;
	u_int seq_num, qop_state;
	int conf_state;
	bool_t xdr_stat;

    log_debug("in xdr_rpc_sspi_unwrap_data()");

	if (xdr_func == (xdrproc_t)xdr_void || xdr_ptr == NULL)
		return (TRUE);

	memset(&databuf, 0, sizeof(databuf));
	memset(&wrapbuf, 0, sizeof(wrapbuf));

	if (svc == RPCSEC_SSPI_SVC_INTEGRITY) {
		/* Decode databody_integ. */
		if (!xdr_bytes(xdrs, (char **)&databuf.value, (u_int *)&databuf.length,
                        (u_int)-1)) {
			log_debug("xdr_rpc_sspi_unwrap_data: xdr decode databody_integ failed");
			return (FALSE);
		}
		/* Decode checksum. */
		if (!xdr_bytes(xdrs, (char **)&wrapbuf.value, (u_int *)&wrapbuf.length,
                        MAX_NETOBJ_SZ)) {
#if 0
			gss_release_buffer(&min_stat, &databuf);
#else
            sspi_release_buffer(&databuf);
#endif
			log_debug("xdr_rpc_sspi_unwrap_data: xdr decode checksum failed");
			return (FALSE);
		}
		/* Verify checksum and QOP. */
#if 0
		maj_stat = gss_verify_mic(&min_stat, ctx, &databuf,
					  &wrapbuf, &qop_state);
#else
        maj_stat = sspi_verify_mic(ctx, seq, &databuf, &wrapbuf, &qop_state);
#endif
#if 0
		gss_release_buffer(&min_stat, &wrapbuf);
#else
        sspi_release_buffer(&wrapbuf);
#endif

		if (maj_stat != SEC_E_OK) {
#if 0
			gss_release_buffer(&min_stat, &databuf);
#else
            sspi_release_buffer(&databuf);
#endif
			log_debug("xdr_rpc_sspi_unwrap_data: sspi_verify_mic "
                        "failed with %x", maj_stat);
			return (FALSE);
		}
	}
	else if (svc == RPCSEC_SSPI_SVC_PRIVACY) {
		/* Decode databody_priv. */
		if (!xdr_bytes(xdrs, (char **)&wrapbuf.value, (u_int *)&wrapbuf.length,
                        (u_int)-1)) {
			log_debug("xdr_rpc_sspi_unwrap_data: xdr decode databody_priv failed");
			return (FALSE);
		}
		/* Decrypt databody. */
#if 0
		maj_stat = gss_unwrap(&min_stat, ctx, &wrapbuf, &databuf,
				      &conf_state, &qop_state);
#else
        maj_stat = sspi_unwrap(ctx, seq, &wrapbuf, &databuf, &conf_state, &qop_state);
#endif
#if 0
		gss_release_buffer(&min_stat, &wrapbuf);
#else
        sspi_release_buffer(&wrapbuf);
#endif
		/* Verify encryption and QOP. */
		if (maj_stat != SEC_E_OK) {
#if 0
			gss_release_buffer(&min_stat, &databuf);
#else
            sspi_release_buffer(&databuf);
#endif
			log_debug("xdr_rpc_sspi_unwrap_data: sspi_unwrap failed with %x", maj_stat);
			return (FALSE);
		}
	}
	/* Decode rpc_gss_data_t (sequence number + arguments). */
	xdrmem_create(&tmpxdrs, databuf.value, databuf.length, XDR_DECODE);
	xdr_stat = (xdr_u_int(&tmpxdrs, &seq_num) &&
                (*xdr_func)(&tmpxdrs, xdr_ptr));
	XDR_DESTROY(&tmpxdrs);
#if 0
	gss_release_buffer(&min_stat, &databuf);
#else
    sspi_release_buffer(&databuf);
#endif
	/* Verify sequence number. */
	if (xdr_stat == TRUE && seq_num != seq) {
		log_debug("wrong sequence number in databody received %d expected %d",
            seq_num, seq);
		return (FALSE);
	}

	return (xdr_stat);
}

bool_t
xdr_rpc_sspi_data(XDR *xdrs, xdrproc_t xdr_func, caddr_t xdr_ptr,
		 PCtxtHandle ctx, sspi_qop_t qop,
		 rpc_sspi_svc_t svc, u_int seq)
{
	switch (xdrs->x_op) {

	case XDR_ENCODE:
		return (xdr_rpc_sspi_wrap_data(xdrs, xdr_func, xdr_ptr,
                    ctx, qop, svc, seq));
	case XDR_DECODE:
		return (xdr_rpc_sspi_unwrap_data(xdrs, xdr_func, xdr_ptr,
                    ctx, qop, svc, seq));
	case XDR_FREE:
		return (TRUE);
	}
	return (FALSE);
}
