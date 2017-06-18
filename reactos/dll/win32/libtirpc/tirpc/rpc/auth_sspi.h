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

#ifndef _TIRPC_AUTH_GSS_H
#define _TIRPC_AUTH_GSS_H

#include <rpc/clnt.h>
#define SECURITY_WIN32
#include <security.h>

/* RPCSEC_SSPI control procedures. */
typedef enum {
	RPCSEC_SSPI_DATA = 0,
	RPCSEC_SSPI_INIT = 1,
	RPCSEC_SSPI_CONTINUE_INIT = 2,
	RPCSEC_SSPI_DESTROY = 3
} rpc_sspi_proc_t;

/* RPCSEC_SSPI services. */
typedef enum {
	RPCSEC_SSPI_SVC_NONE = 1,
	RPCSEC_SSPI_SVC_INTEGRITY = 2,
	RPCSEC_SSPI_SVC_PRIVACY = 3
} rpc_sspi_svc_t;

#define RPCSEC_SSPI_VERSION	1

#define sspi_name_t SEC_CHAR *
#define sspi_qop_t uint32_t

typedef struct _sspi_OID_desc {
    int length;
    void *elements;
} sspi_OID_desc, *sspi_OID;

typedef struct _sspi_buffer_desc {
    int length;
    void *value;
} sspi_buffer_desc, *sspi_buffer_t;

#define SSPI_C_NO_NAME ((sspi_name_t) NULL)
#define SSPI_C_NO_BUFFER ((sspi_buffer_t) NULL)
#define SSPI_C_NO_CONTEXT ((PCtxtHandle) NULL)

/* RPCSEC_SSPI security triple. */
struct rpc_sspi_sec {
	sspi_OID        mech;		/* mechanism */
	uint32_t        qop;		/* quality of protection */
	rpc_sspi_svc_t	svc;		/* service */
    CredHandle      cred;       /* cred handle */
	u_int		    req_flags;	/* req flags for init_sec_context */
    TimeStamp       expiry;
};

/* Credentials. */
struct rpc_sspi_cred {
	u_int		        gc_v;		/* version */
	rpc_sspi_proc_t	    gc_proc;	/* control procedure */
	u_int		        gc_seq;		/* sequence number */
	rpc_sspi_svc_t	    gc_svc;		/* service */
	sspi_buffer_desc	gc_ctx;		/* server's returned context handle */
};

/* Context creation response. */
struct rpc_sspi_init_res {
	sspi_buffer_desc    gr_ctx;		/* context handle */
	u_int			    gr_major;	/* major status */
	u_int			    gr_minor;	/* minor status */
	u_int			    gr_win;		/* sequence window */
	sspi_buffer_desc    gr_token;	/* token */
};

/* Prototypes. */
__BEGIN_DECLS
bool_t xdr_rpc_sspi_cred(XDR *xdrs, struct rpc_sspi_cred *p);
bool_t xdr_rpc_sspi_init_args(XDR *xdrs, sspi_buffer_desc *p);
bool_t xdr_rpc_sspi_init_res(XDR *xdrs, struct rpc_sspi_init_res *p);
bool_t xdr_rpc_sspi_data(XDR *xdrs, xdrproc_t xdr_func, caddr_t xdr_ptr, 
                         PCtxtHandle ctx, sspi_qop_t qop, 
                         rpc_sspi_svc_t svc, u_int seq);
AUTH *authsspi_create(CLIENT *, sspi_name_t, struct rpc_sspi_sec *);
AUTH *authsspi_create_default(CLIENT *, char *, int);
bool_t authsspi_service(AUTH *auth, int svc);
uint32_t sspi_get_mic(void *ctx, u_int qop, u_int seq, 
                      sspi_buffer_desc *bufin, sspi_buffer_desc *bufout);
uint32_t sspi_verify_mic(void *ctx, u_int seq, sspi_buffer_desc *bufin, 
                         sspi_buffer_desc *bufout, u_int *qop_state);
uint32_t sspi_wrap(void *ctx, u_int seq, sspi_buffer_desc *bufin, 
                         sspi_buffer_desc *bufout, u_int *conf_state);
uint32_t sspi_unwrap(void *ctx, u_int seq, sspi_buffer_desc *bufin, 
                     sspi_buffer_desc *bufout, u_int *conf_state, 
                     u_int *qop_state);
void sspi_release_buffer(sspi_buffer_desc *buf);
uint32_t sspi_import_name(sspi_buffer_desc *name_in, sspi_name_t *name_out);

void log_debug(const char *fmt, ...);
void log_status(char *m, uint32_t major, uint32_t minor);
void log_hexdump(bool_t on, const u_char *title, const u_char *buf, int len, int offset);

__END_DECLS

#endif /* !_TIRPC_AUTH_GSS_H */
