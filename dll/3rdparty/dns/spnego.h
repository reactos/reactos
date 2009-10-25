/*
 * Copyright (C) 2006, 2007  Internet Systems Consortium, Inc. ("ISC")
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

/* $Id: spnego.h,v 1.4 2007/06/19 23:47:16 tbox Exp $ */

/*! \file
 * \brief
 * Entry points into portable SPNEGO implementation.
 * See spnego.c for information on the SPNEGO implementation itself.
 */

#ifndef _SPNEGO_H_
#define _SPNEGO_H_

/*%
 * Wrapper for GSSAPI gss_init_sec_context(), using portable SPNEGO
 * implementation instead of the one that's part of the GSSAPI
 * library.  Takes arguments identical to the standard GSSAPI
 * function, uses standard gss_init_sec_context() to handle
 * everything inside the SPNEGO wrapper.
 */
OM_uint32
gss_init_sec_context_spnego(OM_uint32 *,
			    const gss_cred_id_t,
			    gss_ctx_id_t *,
			    const gss_name_t,
			    const gss_OID,
			    OM_uint32,
			    OM_uint32,
			    const gss_channel_bindings_t,
			    const gss_buffer_t,
			    gss_OID *,
			    gss_buffer_t,
			    OM_uint32 *,
			    OM_uint32 *);

/*%
 * Wrapper for GSSAPI gss_accept_sec_context(), using portable SPNEGO
 * implementation instead of the one that's part of the GSSAPI
 * library.  Takes arguments identical to the standard GSSAPI
 * function.  Checks the OID of the input token to see if it's SPNEGO;
 * if so, processes it, otherwise hands the call off to the standard
 * gss_accept_sec_context() function.
 */
OM_uint32 gss_accept_sec_context_spnego(OM_uint32 *,
					gss_ctx_id_t *,
					const gss_cred_id_t,
					const gss_buffer_t,
					const gss_channel_bindings_t,
					gss_name_t *,
					gss_OID *,
					gss_buffer_t,
					OM_uint32 *,
					OM_uint32 *,
					gss_cred_id_t *);


#endif
