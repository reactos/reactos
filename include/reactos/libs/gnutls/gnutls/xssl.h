/* -*- c -*-
 * Copyright (C) 2013 Nikos Mavrogiannopoulos
 *
 * This file is part of GnuTLS.
 *
 * The GnuTLS library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 *
 */

#ifndef GNUTLS_SBUF_H
#define GNUTLS_SBUF_H

/* *INDENT-OFF* */
#ifdef __cplusplus
extern "C" {
#endif
/* *INDENT-ON* */

#include <gnutls/gnutls.h>

/* Buffered session I/O */
typedef struct xssl_st *xssl_t;
typedef struct xssl_cred_st *xssl_cred_t;

ssize_t xssl_printf(xssl_t sb, const char *fmt, ...)
#ifdef __GNUC__
    __attribute__ ((format(printf, 2, 3)))
#endif
    ;

ssize_t xssl_write(xssl_t sb, const void *data, size_t data_size);

ssize_t xssl_flush(xssl_t sb);

ssize_t xssl_read(xssl_t sb, void *data, size_t data_size);

ssize_t
xssl_getdelim(xssl_t sbuf, char **lineptr, size_t * n, int delimiter);

#define xssl_getline(sbuf, ptr, n) xssl_getdelim(sbuf, ptr, n, '\n')

void xssl_deinit(xssl_t sb);

#define GNUTLS_SBUF_WRITE_FLUSHES (1<<0)
int xssl_sinit(xssl_t * isb, gnutls_session_t session, unsigned int flags);

gnutls_session_t xssl_get_session(xssl_t sb);

int xssl_client_init(xssl_t * isb, const char *hostname,
		     const char *service,
		     gnutls_transport_ptr fd,
		     const char *priority, xssl_cred_t cred,
		     unsigned int *status, unsigned int flags);

int xssl_server_init(xssl_t * isb,
		     gnutls_transport_ptr fd,
		     const char *priority, xssl_cred_t cred,
		     unsigned int *status, unsigned int flags);

/* High level credential structures */
typedef enum {
	GNUTLS_VMETHOD_NO_AUTH = 0,
	GNUTLS_VMETHOD_TOFU = 1 << 0,
	GNUTLS_VMETHOD_GIVEN_CAS = 1 << 1,
	GNUTLS_VMETHOD_SYSTEM_CAS = 1 << 2
} gnutls_vmethod_t;

typedef enum {
	GNUTLS_CINPUT_TYPE_FILE = 0,
	GNUTLS_CINPUT_TYPE_MEM = 1,
	GNUTLS_CINPUT_TYPE_PIN_FUNC = 2,
} gnutls_cinput_type_t;

typedef enum {
	GNUTLS_CINPUT_CAS = 1,	/* i1 contains the CAs */
	GNUTLS_CINPUT_CRLS = 2,	/* i1 contains the CRLs */
	GNUTLS_CINPUT_TOFU_DB = 3,	/* i1 contains the DB filename */
	GNUTLS_CINPUT_KEYPAIR = 4,	/* i1 contains the certificate, i2 the key 
					 * or i1.pin_fn contains the pin function, 
					 * and i2.udata the user pointer */
} gnutls_cinput_contents_t;

typedef struct gnutls_cinput_st {
	gnutls_cinput_type_t type;
	gnutls_cinput_contents_t contents;
	gnutls_x509_crt_fmt_t fmt;	/* if applicable */

	union {
		gnutls_pin_callback_t pin_fn;
		const char *file;
		gnutls_datum_t mem;
	} i1;

	union {
		void *udata;
		const char *file;
		gnutls_datum_t mem;
	} i2;

	unsigned long future_pad[8];
} gnutls_cinput_st;

int xssl_cred_init(xssl_cred_t * c, unsigned vflags,
		   gnutls_cinput_st * aux, unsigned aux_size);
void xssl_cred_deinit(xssl_cred_t cred);


/* *INDENT-OFF* */
#ifdef __cplusplus
}
#endif
/* *INDENT-ON* */

#endif				/* GNUTLS_SBUF_H */
