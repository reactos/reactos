/*
  authgss_prot.c

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
#include <stdarg.h>
#include <rpc/types.h>
#include <rpc/xdr.h>
#include <rpc/auth.h>
#include <rpc/auth_gss.h>
#include <rpc/rpc.h>
#include <gssapi/gssapi.h>

bool_t
xdr_rpc_gss_cred(XDR *xdrs, struct rpc_gss_cred *p)
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
xdr_rpc_gss_init_args(XDR *xdrs, gss_buffer_desc *p)
{
	bool_t xdr_stat;

	xdr_stat = xdr_bytes(xdrs, (char **)&p->value,
			      (u_int *)&p->length, MAX_NETOBJ_SZ);

	log_debug("xdr_rpc_gss_init_args: %s %s (token %p:%d)",
		  (xdrs->x_op == XDR_ENCODE) ? "encode" : "decode",
		  (xdr_stat == TRUE) ? "success" : "failure",
		  p->value, p->length);

	return (xdr_stat);
}

bool_t
xdr_rpc_gss_init_res(XDR *xdrs, struct rpc_gss_init_res *p)
{
	bool_t xdr_stat;

	xdr_stat = (xdr_bytes(xdrs, (char **)&p->gr_ctx.value,
			      (u_int *)&p->gr_ctx.length, MAX_NETOBJ_SZ) &&
		    xdr_u_int(xdrs, &p->gr_major) &&
		    xdr_u_int(xdrs, &p->gr_minor) &&
		    xdr_u_int(xdrs, &p->gr_win) &&
		    xdr_bytes(xdrs, (char **)&p->gr_token.value,
			      (u_int *)&p->gr_token.length, MAX_NETOBJ_SZ));

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
xdr_rpc_gss_wrap_data(XDR *xdrs, xdrproc_t xdr_func, caddr_t xdr_ptr,
		      gss_ctx_id_t ctx, gss_qop_t qop,
		      rpc_gss_svc_t svc, u_int seq)
{
	gss_buffer_desc	databuf, wrapbuf;
	OM_uint32	maj_stat, min_stat;
	int		start, end, conf_state;
	bool_t		xdr_stat;

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

	if (svc == RPCSEC_GSS_SVC_INTEGRITY) {
		/* Marshal databody_integ length. */
		XDR_SETPOS(xdrs, start);
		if (!xdr_u_int(xdrs, (u_int *)&databuf.length))
			return (FALSE);

		/* Checksum rpc_gss_data_t. */
		maj_stat = gss_get_mic(&min_stat, ctx, qop,
				       &databuf, &wrapbuf);
		if (maj_stat != GSS_S_COMPLETE) {
			log_debug("gss_get_mic failed");
			return (FALSE);
		}
		/* Marshal checksum. */
		XDR_SETPOS(xdrs, end);
		xdr_stat = xdr_bytes(xdrs, (char **)&wrapbuf.value,
				     (u_int *)&wrapbuf.length, MAX_NETOBJ_SZ);
		gss_release_buffer(&min_stat, &wrapbuf);
	}
	else if (svc == RPCSEC_GSS_SVC_PRIVACY) {
		/* Encrypt rpc_gss_data_t. */
		maj_stat = gss_wrap(&min_stat, ctx, TRUE, qop, &databuf,
				    &conf_state, &wrapbuf);
		if (maj_stat != GSS_S_COMPLETE) {
			log_status("gss_wrap", maj_stat, min_stat);
			return (FALSE);
		}
		/* Marshal databody_priv. */
		XDR_SETPOS(xdrs, start);
		xdr_stat = xdr_bytes(xdrs, (char **)&wrapbuf.value,
				     (u_int *)&wrapbuf.length, MAX_NETOBJ_SZ);
		gss_release_buffer(&min_stat, &wrapbuf);
	}
	return (xdr_stat);
}

bool_t
xdr_rpc_gss_unwrap_data(XDR *xdrs, xdrproc_t xdr_func, caddr_t xdr_ptr,
			gss_ctx_id_t ctx, gss_qop_t qop,
			rpc_gss_svc_t svc, u_int seq)
{
	XDR		tmpxdrs;
	gss_buffer_desc	databuf, wrapbuf;
	OM_uint32	maj_stat, min_stat;
	u_int		seq_num, qop_state;
	int			conf_state;
	bool_t		xdr_stat;

	if (xdr_func == (xdrproc_t)xdr_void || xdr_ptr == NULL)
		return (TRUE);

	memset(&databuf, 0, sizeof(databuf));
	memset(&wrapbuf, 0, sizeof(wrapbuf));

	if (svc == RPCSEC_GSS_SVC_INTEGRITY) {
		/* Decode databody_integ. */
		if (!xdr_bytes(xdrs, (char **)&databuf.value, (u_int *)&databuf.length,
			       MAX_NETOBJ_SZ)) {
			log_debug("xdr decode databody_integ failed");
			return (FALSE);
		}
		/* Decode checksum. */
		if (!xdr_bytes(xdrs, (char **)&wrapbuf.value, (u_int *)&wrapbuf.length,
			       MAX_NETOBJ_SZ)) {
			gss_release_buffer(&min_stat, &databuf);
			log_debug("xdr decode checksum failed");
			return (FALSE);
		}
		/* Verify checksum and QOP. */
		maj_stat = gss_verify_mic(&min_stat, ctx, &databuf,
					  &wrapbuf, &qop_state);
		gss_release_buffer(&min_stat, &wrapbuf);

		if (maj_stat != GSS_S_COMPLETE || qop_state != qop) {
			gss_release_buffer(&min_stat, &databuf);
			log_status("gss_verify_mic", maj_stat, min_stat);
			return (FALSE);
		}
	}
	else if (svc == RPCSEC_GSS_SVC_PRIVACY) {
		/* Decode databody_priv. */
		if (!xdr_bytes(xdrs, (char **)&wrapbuf.value, (u_int *)&wrapbuf.length,
			       MAX_NETOBJ_SZ)) {
			log_debug("xdr decode databody_priv failed");
			return (FALSE);
		}
		/* Decrypt databody. */
		maj_stat = gss_unwrap(&min_stat, ctx, &wrapbuf, &databuf,
				      &conf_state, &qop_state);

		gss_release_buffer(&min_stat, &wrapbuf);

		/* Verify encryption and QOP. */
		if (maj_stat != GSS_S_COMPLETE || qop_state != qop ||
			conf_state != TRUE) {
			gss_release_buffer(&min_stat, &databuf);
			log_status("gss_unwrap", maj_stat, min_stat);
			return (FALSE);
		}
	}
	/* Decode rpc_gss_data_t (sequence number + arguments). */
	xdrmem_create(&tmpxdrs, databuf.value, databuf.length, XDR_DECODE);
	xdr_stat = (xdr_u_int(&tmpxdrs, &seq_num) &&
		    (*xdr_func)(&tmpxdrs, xdr_ptr));
	XDR_DESTROY(&tmpxdrs);
	gss_release_buffer(&min_stat, &databuf);

	/* Verify sequence number. */
	if (xdr_stat == TRUE && seq_num != seq) {
		log_debug("wrong sequence number in databody");
		return (FALSE);
	}
	return (xdr_stat);
}

bool_t
xdr_rpc_gss_data(XDR *xdrs, xdrproc_t xdr_func, caddr_t xdr_ptr,
		 gss_ctx_id_t ctx, gss_qop_t qop,
		 rpc_gss_svc_t svc, u_int seq)
{
	switch (xdrs->x_op) {

	case XDR_ENCODE:
		return (xdr_rpc_gss_wrap_data(xdrs, xdr_func, xdr_ptr,
					      ctx, qop, svc, seq));
	case XDR_DECODE:
		return (xdr_rpc_gss_unwrap_data(xdrs, xdr_func, xdr_ptr,
						ctx, qop,svc, seq));
	case XDR_FREE:
		return (TRUE);
	}
	return (FALSE);
}

#ifdef DEBUG
#include <ctype.h>

void
log_debug(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	fprintf(stderr, "rpcsec_gss: ");
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, "\n");
	va_end(ap);
}

void
log_status(char *m, OM_uint32 maj_stat, OM_uint32 min_stat)
{
	OM_uint32 min;
	gss_buffer_desc msg;
	int msg_ctx = 0;

	fprintf(stderr, "rpcsec_gss: %s: ", m);

	gss_display_status(&min, maj_stat, GSS_C_GSS_CODE, GSS_C_NULL_OID,
			   &msg_ctx, &msg);
	fprintf(stderr, "%s - ", (char *)msg.value);
	gss_release_buffer(&min, &msg);

	gss_display_status(&min, min_stat, GSS_C_MECH_CODE, GSS_C_NULL_OID,
			   &msg_ctx, &msg);
	fprintf(stderr, "%s\n", (char *)msg.value);
	gss_release_buffer(&min, &msg);
}

void
log_hexdump(const u_char *buf, int len, int offset)
{
	u_int i, j, jm;
	int c;

	fprintf(stderr, "\n");
	for (i = 0; i < len; i += 0x10) {
		fprintf(stderr, "  %04x: ", (u_int)(i + offset));
		jm = len - i;
		jm = jm > 16 ? 16 : jm;

		for (j = 0; j < jm; j++) {
			if ((j % 2) == 1)
				fprintf(stderr, "%02x ", (u_int) buf[i+j]);
			else
				fprintf(stderr, "%02x", (u_int) buf[i+j]);
		}
		for (; j < 16; j++) {
			if ((j % 2) == 1) printf("   ");
			else fprintf(stderr, "  ");
		}
		fprintf(stderr, " ");

		for (j = 0; j < jm; j++) {
			c = buf[i+j];
			c = isprint(c) ? c : '.';
			fprintf(stderr, "%c", c);
		}
		fprintf(stderr, "\n");
	}
}

#else

void
log_debug(const char *fmt, ...)
{
}

void
log_status(char *m, OM_uint32 maj_stat, OM_uint32 min_stat)
{
}

void
log_hexdump(const u_char *buf, int len, int offset)
{
}

#endif


