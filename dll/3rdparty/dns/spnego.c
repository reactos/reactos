/*
 * Copyright (C) 2006-2009  Internet Systems Consortium, Inc. ("ISC")
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

/* $Id: spnego.c,v 1.8.118.2 2009/01/18 23:47:40 tbox Exp $ */

/*! \file
 * \brief
 * Portable SPNEGO implementation.
 *
 * This is part of a portable implementation of the SPNEGO protocol
 * (RFCs 2478 and 4178).  This implementation uses the RFC 4178 ASN.1
 * module but is not a full implementation of the RFC 4178 protocol;
 * at the moment, we only support GSS-TSIG with Kerberos
 * authentication, so we only need enough of the SPNEGO protocol to
 * support that.
 *
 * The files that make up this portable SPNEGO implementation are:
 * \li	spnego.c	(this file)
 * \li	spnego.h	(API SPNEGO exports to the rest of lib/dns)
 * \li	spnego.asn1	(SPNEGO ASN.1 module)
 * \li	spnego_asn1.c	(routines generated from spngo.asn1)
 * \li	spnego_asn1.pl	(perl script to generate spnego_asn1.c)
 *
 * Everything but the functions exported in spnego.h is static, to
 * avoid possible conflicts with other libraries (particularly Heimdal,
 * since much of this code comes from Heimdal by way of mod_auth_kerb).
 *
 * spnego_asn1.c is shipped as part of lib/dns because generating it
 * requires both Perl and the Heimdal ASN.1 compiler.  See
 * spnego_asn1.pl for further details.  We've tried to eliminate all
 * compiler warnings from the generated code, but you may see a few
 * when using a compiler version we haven't tested yet.
 */

/*
 * Portions of this code were derived from mod_auth_kerb and Heimdal.
 * These packages are available from:
 *
 *   http://modauthkerb.sourceforge.net/
 *   http://www.pdc.kth.se/heimdal/
 *
 * and were released under the following licenses:
 *
 * ----------------------------------------------------------------
 *
 * Copyright (c) 2004 Masarykova universita
 * (Masaryk University, Brno, Czech Republic)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the University nor the names of its contributors may
 *    be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * ----------------------------------------------------------------
 *
 * Copyright (c) 1997 - 2003 Kungliga Tekniska Högskolan
 * (Royal Institute of Technology, Stockholm, Sweden).
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * XXXSRA We should omit this file entirely in Makefile.in via autoconf,
 * but this will keep it from generating errors until that's written.
 */

#ifdef GSSAPI

/*
 * XXXSRA Some of the following files are almost certainly unnecessary,
 * but using this list (borrowed from gssapictx.c) gets rid of some
 * whacky compilation errors when building with MSVC and should be
 * harmless in any case.
 */

#include <config.h>

#include <stdlib.h>
#include <errno.h>

#include <isc/buffer.h>
#include <isc/dir.h>
#include <isc/entropy.h>
#include <isc/lex.h>
#include <isc/mem.h>
#include <isc/once.h>
#include <isc/random.h>
#include <isc/string.h>
#include <isc/time.h>
#include <isc/util.h>

#include <dns/fixedname.h>
#include <dns/name.h>
#include <dns/rdata.h>
#include <dns/rdataclass.h>
#include <dns/result.h>
#include <dns/types.h>
#include <dns/keyvalues.h>
#include <dns/log.h>

#include <dst/gssapi.h>
#include <dst/result.h>

#include "dst_internal.h"

/*
 * The API we export
 */
#include "spnego.h"

/* asn1_err.h */
/* Generated from ../../../lib/asn1/asn1_err.et */

typedef enum asn1_error_number {
	ASN1_BAD_TIMEFORMAT = 1859794432,
	ASN1_MISSING_FIELD = 1859794433,
	ASN1_MISPLACED_FIELD = 1859794434,
	ASN1_TYPE_MISMATCH = 1859794435,
	ASN1_OVERFLOW = 1859794436,
	ASN1_OVERRUN = 1859794437,
	ASN1_BAD_ID = 1859794438,
	ASN1_BAD_LENGTH = 1859794439,
	ASN1_BAD_FORMAT = 1859794440,
	ASN1_PARSE_ERROR = 1859794441
} asn1_error_number;

#define ERROR_TABLE_BASE_asn1 1859794432

#define __asn1_common_definitions__

typedef struct octet_string {
	size_t length;
	void *data;
} octet_string;

typedef char *general_string;

typedef char *utf8_string;

typedef struct oid {
	size_t length;
	unsigned *components;
} oid;

/* der.h */

typedef enum {
	ASN1_C_UNIV = 0, ASN1_C_APPL = 1,
	ASN1_C_CONTEXT = 2, ASN1_C_PRIVATE = 3
} Der_class;

typedef enum {
	PRIM = 0, CONS = 1
} Der_type;

/* Universal tags */

enum {
	UT_Boolean = 1,
	UT_Integer = 2,
	UT_BitString = 3,
	UT_OctetString = 4,
	UT_Null = 5,
	UT_OID = 6,
	UT_Enumerated = 10,
	UT_Sequence = 16,
	UT_Set = 17,
	UT_PrintableString = 19,
	UT_IA5String = 22,
	UT_UTCTime = 23,
	UT_GeneralizedTime = 24,
	UT_VisibleString = 26,
	UT_GeneralString = 27
};

#define ASN1_INDEFINITE 0xdce0deed

static int
der_get_length(const unsigned char *p, size_t len,
	       size_t * val, size_t * size);

static int
der_get_octet_string(const unsigned char *p, size_t len,
		     octet_string * data, size_t * size);
static int
der_get_oid(const unsigned char *p, size_t len,
	    oid * data, size_t * size);
static int
der_get_tag(const unsigned char *p, size_t len,
	    Der_class * class, Der_type * type,
	    int *tag, size_t * size);

static int
der_match_tag(const unsigned char *p, size_t len,
	      Der_class class, Der_type type,
	      int tag, size_t * size);
static int
der_match_tag_and_length(const unsigned char *p, size_t len,
			 Der_class class, Der_type type, int tag,
			 size_t * length_ret, size_t * size);

static int
decode_oid(const unsigned char *p, size_t len,
	   oid * k, size_t * size);

static int
decode_enumerated(const unsigned char *p, size_t len,
		  unsigned *num, size_t *size);

static int
decode_octet_string(const unsigned char *, size_t, octet_string *, size_t *);

static int
der_put_int(unsigned char *p, size_t len, int val, size_t *);

static int
der_put_length(unsigned char *p, size_t len, size_t val, size_t *);

static int
der_put_octet_string(unsigned char *p, size_t len,
		     const octet_string * data, size_t *);
static int
der_put_oid(unsigned char *p, size_t len,
	    const oid * data, size_t * size);
static int
der_put_tag(unsigned char *p, size_t len, Der_class class, Der_type type,
	    int tag, size_t *);
static int
der_put_length_and_tag(unsigned char *, size_t, size_t,
		       Der_class, Der_type, int, size_t *);

static int
encode_enumerated(unsigned char *p, size_t len,
		  const unsigned *data, size_t *);

static int
encode_octet_string(unsigned char *p, size_t len,
		    const octet_string * k, size_t *);
static int
encode_oid(unsigned char *p, size_t len,
	   const oid * k, size_t *);

static void
free_octet_string(octet_string * k);

static void
free_oid  (oid * k);

static size_t
length_len(size_t len);

static int
fix_dce(size_t reallen, size_t * len);

/*
 * Include stuff generated by the ASN.1 compiler.
 */

#include "spnego_asn1.c"

static unsigned char gss_krb5_mech_oid_bytes[] = {
	0x2a, 0x86, 0x48, 0x86, 0xf7, 0x12, 0x01, 0x02, 0x02
};

static gss_OID_desc gss_krb5_mech_oid_desc = {
	sizeof(gss_krb5_mech_oid_bytes),
	gss_krb5_mech_oid_bytes
};

static gss_OID GSS_KRB5_MECH = &gss_krb5_mech_oid_desc;

static unsigned char gss_mskrb5_mech_oid_bytes[] = {
	0x2a, 0x86, 0x48, 0x82, 0xf7, 0x12, 0x01, 0x02, 0x02
};

static gss_OID_desc gss_mskrb5_mech_oid_desc = {
	sizeof(gss_mskrb5_mech_oid_bytes),
	gss_mskrb5_mech_oid_bytes
};

static gss_OID GSS_MSKRB5_MECH = &gss_mskrb5_mech_oid_desc;

static unsigned char gss_spnego_mech_oid_bytes[] = {
	0x2b, 0x06, 0x01, 0x05, 0x05, 0x02
};

static gss_OID_desc gss_spnego_mech_oid_desc = {
	sizeof(gss_spnego_mech_oid_bytes),
	gss_spnego_mech_oid_bytes
};

static gss_OID GSS_SPNEGO_MECH = &gss_spnego_mech_oid_desc;

/* spnegokrb5_locl.h */

static OM_uint32
gssapi_spnego_encapsulate(OM_uint32 *,
			  unsigned char *,
			  size_t,
			  gss_buffer_t,
			  const gss_OID);

static OM_uint32
gssapi_spnego_decapsulate(OM_uint32 *,
			  gss_buffer_t,
			  unsigned char **,
			  size_t *,
			  const gss_OID);

/* mod_auth_kerb.c */

static int
cmp_gss_type(gss_buffer_t token, gss_OID oid)
{
	unsigned char *p;
	size_t len;

	if (token->length == 0)
		return (GSS_S_DEFECTIVE_TOKEN);

	p = token->value;
	if (*p++ != 0x60)
		return (GSS_S_DEFECTIVE_TOKEN);
	len = *p++;
	if (len & 0x80) {
		if ((len & 0x7f) > 4)
			return (GSS_S_DEFECTIVE_TOKEN);
		p += len & 0x7f;
	}
	if (*p++ != 0x06)
		return (GSS_S_DEFECTIVE_TOKEN);

	if (((OM_uint32) *p++) != oid->length)
		return (GSS_S_DEFECTIVE_TOKEN);

	return (memcmp(p, oid->elements, oid->length));
}

/* accept_sec_context.c */
/*
 * SPNEGO wrapper for Kerberos5 GSS-API kouril@ics.muni.cz, 2003 (mostly
 * based on Heimdal code)
 */

static OM_uint32
code_NegTokenArg(OM_uint32 * minor_status,
		 const NegTokenResp * resp,
		 unsigned char **outbuf,
		 size_t * outbuf_size)
{
	OM_uint32 ret;
	u_char *buf;
	size_t buf_size, buf_len;

	buf_size = 1024;
	buf = malloc(buf_size);
	if (buf == NULL) {
		*minor_status = ENOMEM;
		return (GSS_S_FAILURE);
	}
	do {
		ret = encode_NegTokenResp(buf + buf_size - 1,
					  buf_size,
					  resp, &buf_len);
		if (ret == 0) {
			size_t tmp;

			ret = der_put_length_and_tag(buf + buf_size - buf_len - 1,
						     buf_size - buf_len,
						     buf_len,
						     ASN1_C_CONTEXT,
						     CONS,
						     1,
						     &tmp);
			if (ret == 0)
				buf_len += tmp;
		}
		if (ret) {
			if (ret == ASN1_OVERFLOW) {
				u_char *tmp;

				buf_size *= 2;
				tmp = realloc(buf, buf_size);
				if (tmp == NULL) {
					*minor_status = ENOMEM;
					free(buf);
					return (GSS_S_FAILURE);
				}
				buf = tmp;
			} else {
				*minor_status = ret;
				free(buf);
				return (GSS_S_FAILURE);
			}
		}
	} while (ret == ASN1_OVERFLOW);

	*outbuf = malloc(buf_len);
	if (*outbuf == NULL) {
		*minor_status = ENOMEM;
		free(buf);
		return (GSS_S_FAILURE);
	}
	memcpy(*outbuf, buf + buf_size - buf_len, buf_len);
	*outbuf_size = buf_len;

	free(buf);

	return (GSS_S_COMPLETE);
}

static OM_uint32
send_reject(OM_uint32 * minor_status,
	    gss_buffer_t output_token)
{
	NegTokenResp resp;
	OM_uint32 ret;

	resp.negState = malloc(sizeof(*resp.negState));
	if (resp.negState == NULL) {
		*minor_status = ENOMEM;
		return (GSS_S_FAILURE);
	}
	*(resp.negState) = reject;

	resp.supportedMech = NULL;
	resp.responseToken = NULL;
	resp.mechListMIC = NULL;

	ret = code_NegTokenArg(minor_status, &resp,
			       (unsigned char **)&output_token->value,
			       &output_token->length);
	free_NegTokenResp(&resp);
	if (ret)
		return (ret);

	return (GSS_S_BAD_MECH);
}

static OM_uint32
send_accept(OM_uint32 * minor_status,
	    gss_buffer_t output_token,
	    gss_buffer_t mech_token,
	    const gss_OID pref)
{
	NegTokenResp resp;
	OM_uint32 ret;

	memset(&resp, 0, sizeof(resp));
	resp.negState = malloc(sizeof(*resp.negState));
	if (resp.negState == NULL) {
		*minor_status = ENOMEM;
		return (GSS_S_FAILURE);
	}
	*(resp.negState) = accept_completed;

	resp.supportedMech = malloc(sizeof(*resp.supportedMech));
	if (resp.supportedMech == NULL) {
		free_NegTokenResp(&resp);
		*minor_status = ENOMEM;
		return (GSS_S_FAILURE);
	}
	ret = der_get_oid(pref->elements,
			  pref->length,
			  resp.supportedMech,
			  NULL);
	if (ret) {
		free_NegTokenResp(&resp);
		*minor_status = ENOMEM;
		return (GSS_S_FAILURE);
	}
	if (mech_token != NULL && mech_token->length != 0) {
		resp.responseToken = malloc(sizeof(*resp.responseToken));
		if (resp.responseToken == NULL) {
			free_NegTokenResp(&resp);
			*minor_status = ENOMEM;
			return (GSS_S_FAILURE);
		}
		resp.responseToken->length = mech_token->length;
		resp.responseToken->data = mech_token->value;
	}

	ret = code_NegTokenArg(minor_status, &resp,
			       (unsigned char **)&output_token->value,
			       &output_token->length);
	if (resp.responseToken != NULL) {
		free(resp.responseToken);
		resp.responseToken = NULL;
	}
	free_NegTokenResp(&resp);
	if (ret)
		return (ret);

	return (GSS_S_COMPLETE);
}

OM_uint32
gss_accept_sec_context_spnego(OM_uint32 *minor_status,
			      gss_ctx_id_t *context_handle,
			      const gss_cred_id_t acceptor_cred_handle,
			      const gss_buffer_t input_token_buffer,
			      const gss_channel_bindings_t input_chan_bindings,
			      gss_name_t *src_name,
			      gss_OID *mech_type,
			      gss_buffer_t output_token,
			      OM_uint32 *ret_flags,
			      OM_uint32 *time_rec,
			      gss_cred_id_t *delegated_cred_handle)
{
	NegTokenInit init_token;
	OM_uint32 major_status;
	OM_uint32 minor_status2;
	gss_buffer_desc ibuf, obuf;
	gss_buffer_t ot = NULL;
	gss_OID pref = GSS_KRB5_MECH;
	unsigned char *buf;
	size_t buf_size;
	size_t len, taglen, ni_len;
	int found = 0;
	int ret;
	unsigned i;

	/*
	 * Before doing anything else, see whether this is a SPNEGO
	 * PDU.  If not, dispatch to the GSSAPI library and get out.
	 */

	if (cmp_gss_type(input_token_buffer, GSS_SPNEGO_MECH))
		return (gss_accept_sec_context(minor_status,
					       context_handle,
					       acceptor_cred_handle,
					       input_token_buffer,
					       input_chan_bindings,
					       src_name,
					       mech_type,
					       output_token,
					       ret_flags,
					       time_rec,
					       delegated_cred_handle));

	/*
	 * If we get here, it's SPNEGO.
	 */

	memset(&init_token, 0, sizeof(init_token));

	ret = gssapi_spnego_decapsulate(minor_status, input_token_buffer,
					&buf, &buf_size, GSS_SPNEGO_MECH);
	if (ret)
		return (ret);

	ret = der_match_tag_and_length(buf, buf_size, ASN1_C_CONTEXT, CONS,
				       0, &len, &taglen);
	if (ret)
		return (ret);

	ret = decode_NegTokenInit(buf + taglen, len, &init_token, &ni_len);
	if (ret) {
		*minor_status = EINVAL;	/* XXX */
		return (GSS_S_DEFECTIVE_TOKEN);
	}

	for (i = 0; !found && i < init_token.mechTypes.len; ++i) {
		char mechbuf[17];
		size_t mech_len;

		ret = der_put_oid(mechbuf + sizeof(mechbuf) - 1,
				  sizeof(mechbuf),
				  &init_token.mechTypes.val[i],
				  &mech_len);
		if (ret)
			return (GSS_S_DEFECTIVE_TOKEN);
		if (mech_len == GSS_KRB5_MECH->length &&
		    memcmp(GSS_KRB5_MECH->elements,
			   mechbuf + sizeof(mechbuf) - mech_len,
			   mech_len) == 0) {
			found = 1;
			break;
		}
		if (mech_len == GSS_MSKRB5_MECH->length &&
		    memcmp(GSS_MSKRB5_MECH->elements,
			   mechbuf + sizeof(mechbuf) - mech_len,
			   mech_len) == 0) {
			found = 1;
			if (i == 0)
				pref = GSS_MSKRB5_MECH;
			break;
		}
	}

	if (!found)
		return (send_reject(minor_status, output_token));

	if (i == 0 && init_token.mechToken != NULL) {
		ibuf.length = init_token.mechToken->length;
		ibuf.value = init_token.mechToken->data;

		major_status = gss_accept_sec_context(minor_status,
						      context_handle,
						      acceptor_cred_handle,
						      &ibuf,
						      input_chan_bindings,
						      src_name,
						      mech_type,
						      &obuf,
						      ret_flags,
						      time_rec,
						      delegated_cred_handle);
		if (GSS_ERROR(major_status)) {
			send_reject(&minor_status2, output_token);
			return (major_status);
		}
		ot = &obuf;
	}
	ret = send_accept(&minor_status2, output_token, ot, pref);
	if (ot != NULL && ot->length != 0)
		gss_release_buffer(&minor_status2, ot);

	return (ret);
}

/* decapsulate.c */

static OM_uint32
gssapi_verify_mech_header(u_char ** str,
			  size_t total_len,
			  const gss_OID mech)
{
	size_t len, len_len, mech_len, foo;
	int e;
	u_char *p = *str;

	if (total_len < 1)
		return (GSS_S_DEFECTIVE_TOKEN);
	if (*p++ != 0x60)
		return (GSS_S_DEFECTIVE_TOKEN);
	e = der_get_length(p, total_len - 1, &len, &len_len);
	if (e || 1 + len_len + len != total_len)
		return (GSS_S_DEFECTIVE_TOKEN);
	p += len_len;
	if (*p++ != 0x06)
		return (GSS_S_DEFECTIVE_TOKEN);
	e = der_get_length(p, total_len - 1 - len_len - 1,
			   &mech_len, &foo);
	if (e)
		return (GSS_S_DEFECTIVE_TOKEN);
	p += foo;
	if (mech_len != mech->length)
		return (GSS_S_BAD_MECH);
	if (memcmp(p, mech->elements, mech->length) != 0)
		return (GSS_S_BAD_MECH);
	p += mech_len;
	*str = p;
	return (GSS_S_COMPLETE);
}

/*
 * Remove the GSS-API wrapping from `in_token' giving `buf and buf_size' Does
 * not copy data, so just free `in_token'.
 */

static OM_uint32
gssapi_spnego_decapsulate(OM_uint32 *minor_status,
			  gss_buffer_t input_token_buffer,
			  unsigned char **buf,
			  size_t *buf_len,
			  const gss_OID mech)
{
	u_char *p;
	OM_uint32 ret;

	p = input_token_buffer->value;
	ret = gssapi_verify_mech_header(&p,
					input_token_buffer->length,
					mech);
	if (ret) {
		*minor_status = ret;
		return (GSS_S_FAILURE);
	}
	*buf_len = input_token_buffer->length -
		(p - (u_char *) input_token_buffer->value);
	*buf = p;
	return (GSS_S_COMPLETE);
}

/* der_free.c */

static void
free_octet_string(octet_string *k)
{
	free(k->data);
	k->data = NULL;
}

static void
free_oid(oid *k)
{
	free(k->components);
	k->components = NULL;
}

/* der_get.c */

/*
 * All decoding functions take a pointer `p' to first position in which to
 * read, from the left, `len' which means the maximum number of characters we
 * are able to read, `ret' were the value will be returned and `size' where
 * the number of used bytes is stored. Either 0 or an error code is returned.
 */

static int
der_get_unsigned(const unsigned char *p, size_t len,
		 unsigned *ret, size_t *size)
{
	unsigned val = 0;
	size_t oldlen = len;

	while (len--)
		val = val * 256 + *p++;
	*ret = val;
	if (size)
		*size = oldlen;
	return (0);
}

static int
der_get_int(const unsigned char *p, size_t len,
	    int *ret, size_t *size)
{
	int val = 0;
	size_t oldlen = len;

	if (len > 0) {
		val = (signed char)*p++;
		while (--len)
			val = val * 256 + *p++;
	}
	*ret = val;
	if (size)
		*size = oldlen;
	return (0);
}

static int
der_get_length(const unsigned char *p, size_t len,
	       size_t *val, size_t *size)
{
	size_t v;

	if (len <= 0)
		return (ASN1_OVERRUN);
	--len;
	v = *p++;
	if (v < 128) {
		*val = v;
		if (size)
			*size = 1;
	} else {
		int e;
		size_t l;
		unsigned tmp;

		if (v == 0x80) {
			*val = ASN1_INDEFINITE;
			if (size)
				*size = 1;
			return (0);
		}
		v &= 0x7F;
		if (len < v)
			return (ASN1_OVERRUN);
		e = der_get_unsigned(p, v, &tmp, &l);
		if (e)
			return (e);
		*val = tmp;
		if (size)
			*size = l + 1;
	}
	return (0);
}

static int
der_get_octet_string(const unsigned char *p, size_t len,
		     octet_string *data, size_t *size)
{
	data->length = len;
	data->data = malloc(len);
	if (data->data == NULL && data->length != 0)
		return (ENOMEM);
	memcpy(data->data, p, len);
	if (size)
		*size = len;
	return (0);
}

static int
der_get_oid(const unsigned char *p, size_t len,
	    oid *data, size_t *size)
{
	int n;
	size_t oldlen = len;

	if (len < 1)
		return (ASN1_OVERRUN);

	data->components = malloc(len * sizeof(*data->components));
	if (data->components == NULL && len != 0)
		return (ENOMEM);
	data->components[0] = (*p) / 40;
	data->components[1] = (*p) % 40;
	--len;
	++p;
	for (n = 2; len > 0; ++n) {
		unsigned u = 0;

		do {
			--len;
			u = u * 128 + (*p++ % 128);
		} while (len > 0 && p[-1] & 0x80);
		data->components[n] = u;
	}
	if (p[-1] & 0x80) {
		free_oid(data);
		return (ASN1_OVERRUN);
	}
	data->length = n;
	if (size)
		*size = oldlen;
	return (0);
}

static int
der_get_tag(const unsigned char *p, size_t len,
	    Der_class *class, Der_type *type,
	    int *tag, size_t *size)
{
	if (len < 1)
		return (ASN1_OVERRUN);
	*class = (Der_class) (((*p) >> 6) & 0x03);
	*type = (Der_type) (((*p) >> 5) & 0x01);
	*tag = (*p) & 0x1F;
	if (size)
		*size = 1;
	return (0);
}

static int
der_match_tag(const unsigned char *p, size_t len,
	      Der_class class, Der_type type,
	      int tag, size_t *size)
{
	size_t l;
	Der_class thisclass;
	Der_type thistype;
	int thistag;
	int e;

	e = der_get_tag(p, len, &thisclass, &thistype, &thistag, &l);
	if (e)
		return (e);
	if (class != thisclass || type != thistype)
		return (ASN1_BAD_ID);
	if (tag > thistag)
		return (ASN1_MISPLACED_FIELD);
	if (tag < thistag)
		return (ASN1_MISSING_FIELD);
	if (size)
		*size = l;
	return (0);
}

static int
der_match_tag_and_length(const unsigned char *p, size_t len,
			 Der_class class, Der_type type, int tag,
			 size_t *length_ret, size_t *size)
{
	size_t l, ret = 0;
	int e;

	e = der_match_tag(p, len, class, type, tag, &l);
	if (e)
		return (e);
	p += l;
	len -= l;
	ret += l;
	e = der_get_length(p, len, length_ret, &l);
	if (e)
		return (e);
	p += l;
	len -= l;
	ret += l;
	if (size)
		*size = ret;
	return (0);
}

static int
decode_enumerated(const unsigned char *p, size_t len,
		  unsigned *num, size_t *size)
{
	size_t ret = 0;
	size_t l, reallen;
	int e;

	e = der_match_tag(p, len, ASN1_C_UNIV, PRIM, UT_Enumerated, &l);
	if (e)
		return (e);
	p += l;
	len -= l;
	ret += l;
	e = der_get_length(p, len, &reallen, &l);
	if (e)
		return (e);
	p += l;
	len -= l;
	ret += l;
	e = der_get_int(p, reallen, num, &l);
	if (e)
		return (e);
	p += l;
	len -= l;
	ret += l;
	if (size)
		*size = ret;
	return (0);
}

static int
decode_octet_string(const unsigned char *p, size_t len,
		    octet_string *k, size_t *size)
{
	size_t ret = 0;
	size_t l;
	int e;
	size_t slen;

	e = der_match_tag(p, len, ASN1_C_UNIV, PRIM, UT_OctetString, &l);
	if (e)
		return (e);
	p += l;
	len -= l;
	ret += l;

	e = der_get_length(p, len, &slen, &l);
	if (e)
		return (e);
	p += l;
	len -= l;
	ret += l;
	if (len < slen)
		return (ASN1_OVERRUN);

	e = der_get_octet_string(p, slen, k, &l);
	if (e)
		return (e);
	p += l;
	len -= l;
	ret += l;
	if (size)
		*size = ret;
	return (0);
}

static int
decode_oid(const unsigned char *p, size_t len,
	   oid *k, size_t *size)
{
	size_t ret = 0;
	size_t l;
	int e;
	size_t slen;

	e = der_match_tag(p, len, ASN1_C_UNIV, PRIM, UT_OID, &l);
	if (e)
		return (e);
	p += l;
	len -= l;
	ret += l;

	e = der_get_length(p, len, &slen, &l);
	if (e)
		return (e);
	p += l;
	len -= l;
	ret += l;
	if (len < slen)
		return (ASN1_OVERRUN);

	e = der_get_oid(p, slen, k, &l);
	if (e)
		return (e);
	p += l;
	len -= l;
	ret += l;
	if (size)
		*size = ret;
	return (0);
}

static int
fix_dce(size_t reallen, size_t *len)
{
	if (reallen == ASN1_INDEFINITE)
		return (1);
	if (*len < reallen)
		return (-1);
	*len = reallen;
	return (0);
}

/* der_length.c */

static size_t
len_unsigned(unsigned val)
{
	size_t ret = 0;

	do {
		++ret;
		val /= 256;
	} while (val);
	return (ret);
}

static size_t
length_len(size_t len)
{
	if (len < 128)
		return (1);
	else
		return (len_unsigned(len) + 1);
}


/* der_put.c */

/*
 * All encoding functions take a pointer `p' to first position in which to
 * write, from the right, `len' which means the maximum number of characters
 * we are able to write.  The function returns the number of characters
 * written in `size' (if non-NULL). The return value is 0 or an error.
 */

static int
der_put_unsigned(unsigned char *p, size_t len, unsigned val, size_t *size)
{
	unsigned char *base = p;

	if (val) {
		while (len > 0 && val) {
			*p-- = val % 256;
			val /= 256;
			--len;
		}
		if (val != 0)
			return (ASN1_OVERFLOW);
		else {
			*size = base - p;
			return (0);
		}
	} else if (len < 1)
		return (ASN1_OVERFLOW);
	else {
		*p = 0;
		*size = 1;
		return (0);
	}
}

static int
der_put_int(unsigned char *p, size_t len, int val, size_t *size)
{
	unsigned char *base = p;

	if (val >= 0) {
		do {
			if (len < 1)
				return (ASN1_OVERFLOW);
			*p-- = val % 256;
			len--;
			val /= 256;
		} while (val);
		if (p[1] >= 128) {
			if (len < 1)
				return (ASN1_OVERFLOW);
			*p-- = 0;
			len--;
		}
	} else {
		val = ~val;
		do {
			if (len < 1)
				return (ASN1_OVERFLOW);
			*p-- = ~(val % 256);
			len--;
			val /= 256;
		} while (val);
		if (p[1] < 128) {
			if (len < 1)
				return (ASN1_OVERFLOW);
			*p-- = 0xff;
			len--;
		}
	}
	*size = base - p;
	return (0);
}

static int
der_put_length(unsigned char *p, size_t len, size_t val, size_t *size)
{
	if (len < 1)
		return (ASN1_OVERFLOW);
	if (val < 128) {
		*p = val;
		*size = 1;
		return (0);
	} else {
		size_t l;
		int e;

		e = der_put_unsigned(p, len - 1, val, &l);
		if (e)
			return (e);
		p -= l;
		*p = 0x80 | l;
		*size = l + 1;
		return (0);
	}
}

static int
der_put_octet_string(unsigned char *p, size_t len,
		     const octet_string *data, size_t *size)
{
	if (len < data->length)
		return (ASN1_OVERFLOW);
	p -= data->length;
	len -= data->length;
	memcpy(p + 1, data->data, data->length);
	*size = data->length;
	return (0);
}

static int
der_put_oid(unsigned char *p, size_t len,
	    const oid *data, size_t *size)
{
	unsigned char *base = p;
	int n;

	for (n = data->length - 1; n >= 2; --n) {
		unsigned	u = data->components[n];

		if (len < 1)
			return (ASN1_OVERFLOW);
		*p-- = u % 128;
		u /= 128;
		--len;
		while (u > 0) {
			if (len < 1)
				return (ASN1_OVERFLOW);
			*p-- = 128 + u % 128;
			u /= 128;
			--len;
		}
	}
	if (len < 1)
		return (ASN1_OVERFLOW);
	*p-- = 40 * data->components[0] + data->components[1];
	*size = base - p;
	return (0);
}

static int
der_put_tag(unsigned char *p, size_t len, Der_class class, Der_type type,
	    int tag, size_t *size)
{
	if (len < 1)
		return (ASN1_OVERFLOW);
	*p = (class << 6) | (type << 5) | tag;	/* XXX */
	*size = 1;
	return (0);
}

static int
der_put_length_and_tag(unsigned char *p, size_t len, size_t len_val,
		       Der_class class, Der_type type, int tag, size_t *size)
{
	size_t ret = 0;
	size_t l;
	int e;

	e = der_put_length(p, len, len_val, &l);
	if (e)
		return (e);
	p -= l;
	len -= l;
	ret += l;
	e = der_put_tag(p, len, class, type, tag, &l);
	if (e)
		return (e);
	p -= l;
	len -= l;
	ret += l;
	*size = ret;
	return (0);
}

static int
encode_enumerated(unsigned char *p, size_t len, const unsigned *data,
		  size_t *size)
{
	unsigned num = *data;
	size_t ret = 0;
	size_t l;
	int e;

	e = der_put_int(p, len, num, &l);
	if (e)
		return (e);
	p -= l;
	len -= l;
	ret += l;
	e = der_put_length_and_tag(p, len, l, ASN1_C_UNIV, PRIM, UT_Enumerated, &l);
	if (e)
		return (e);
	p -= l;
	len -= l;
	ret += l;
	*size = ret;
	return (0);
}

static int
encode_octet_string(unsigned char *p, size_t len,
		    const octet_string *k, size_t *size)
{
	size_t ret = 0;
	size_t l;
	int e;

	e = der_put_octet_string(p, len, k, &l);
	if (e)
		return (e);
	p -= l;
	len -= l;
	ret += l;
	e = der_put_length_and_tag(p, len, l, ASN1_C_UNIV, PRIM, UT_OctetString, &l);
	if (e)
		return (e);
	p -= l;
	len -= l;
	ret += l;
	*size = ret;
	return (0);
}

static int
encode_oid(unsigned char *p, size_t len,
	   const oid *k, size_t *size)
{
	size_t ret = 0;
	size_t l;
	int e;

	e = der_put_oid(p, len, k, &l);
	if (e)
		return (e);
	p -= l;
	len -= l;
	ret += l;
	e = der_put_length_and_tag(p, len, l, ASN1_C_UNIV, PRIM, UT_OID, &l);
	if (e)
		return (e);
	p -= l;
	len -= l;
	ret += l;
	*size = ret;
	return (0);
}


/* encapsulate.c */

static void
gssapi_encap_length(size_t data_len,
		    size_t *len,
		    size_t *total_len,
		    const gss_OID mech)
{
	size_t len_len;

	*len = 1 + 1 + mech->length + data_len;

	len_len = length_len(*len);

	*total_len = 1 + len_len + *len;
}

static u_char *
gssapi_mech_make_header(u_char *p,
			size_t len,
			const gss_OID mech)
{
	int e;
	size_t len_len, foo;

	*p++ = 0x60;
	len_len = length_len(len);
	e = der_put_length(p + len_len - 1, len_len, len, &foo);
	if (e || foo != len_len)
		return (NULL);
	p += len_len;
	*p++ = 0x06;
	*p++ = mech->length;
	memcpy(p, mech->elements, mech->length);
	p += mech->length;
	return (p);
}

/*
 * Give it a krb5_data and it will encapsulate with extra GSS-API wrappings.
 */

static OM_uint32
gssapi_spnego_encapsulate(OM_uint32 * minor_status,
			  unsigned char *buf,
			  size_t buf_size,
			  gss_buffer_t output_token,
			  const gss_OID mech)
{
	size_t len, outer_len;
	u_char *p;

	gssapi_encap_length(buf_size, &len, &outer_len, mech);

	output_token->length = outer_len;
	output_token->value = malloc(outer_len);
	if (output_token->value == NULL) {
		*minor_status = ENOMEM;
		return (GSS_S_FAILURE);
	}
	p = gssapi_mech_make_header(output_token->value, len, mech);
	if (p == NULL) {
		if (output_token->length != 0)
			gss_release_buffer(minor_status, output_token);
		return (GSS_S_FAILURE);
	}
	memcpy(p, buf, buf_size);
	return (GSS_S_COMPLETE);
}

/* init_sec_context.c */
/*
 * SPNEGO wrapper for Kerberos5 GSS-API kouril@ics.muni.cz, 2003 (mostly
 * based on Heimdal code)
 */

static int
add_mech(MechTypeList * mech_list, gss_OID mech)
{
	MechType *tmp;
	int ret;

	tmp = realloc(mech_list->val, (mech_list->len + 1) * sizeof(*tmp));
	if (tmp == NULL)
		return (ENOMEM);
	mech_list->val = tmp;

	ret = der_get_oid(mech->elements, mech->length,
			  &mech_list->val[mech_list->len], NULL);
	if (ret)
		return (ret);

	mech_list->len++;
	return (0);
}

/*
 * return the length of the mechanism in token or -1
 * (which implies that the token was bad - GSS_S_DEFECTIVE_TOKEN
 */

static ssize_t
gssapi_krb5_get_mech(const u_char *ptr,
		     size_t total_len,
		     const u_char **mech_ret)
{
	size_t len, len_len, mech_len, foo;
	const u_char *p = ptr;
	int e;

	if (total_len < 1)
		return (-1);
	if (*p++ != 0x60)
		return (-1);
	e = der_get_length (p, total_len - 1, &len, &len_len);
	if (e || 1 + len_len + len != total_len)
		return (-1);
	p += len_len;
	if (*p++ != 0x06)
		return (-1);
	e = der_get_length (p, total_len - 1 - len_len - 1,
			    &mech_len, &foo);
	if (e)
		return (-1);
	p += foo;
	*mech_ret = p;
	return (mech_len);
}

static OM_uint32
spnego_initial(OM_uint32 *minor_status,
	       const gss_cred_id_t initiator_cred_handle,
	       gss_ctx_id_t *context_handle,
	       const gss_name_t target_name,
	       const gss_OID mech_type,
	       OM_uint32 req_flags,
	       OM_uint32 time_req,
	       const gss_channel_bindings_t input_chan_bindings,
	       const gss_buffer_t input_token,
	       gss_OID *actual_mech_type,
	       gss_buffer_t output_token,
	       OM_uint32 *ret_flags,
	       OM_uint32 *time_rec)
{
	NegTokenInit token_init;
	OM_uint32 major_status, minor_status2;
	gss_buffer_desc	krb5_output_token = GSS_C_EMPTY_BUFFER;
	unsigned char *buf = NULL;
	size_t buf_size;
	size_t len;
	int ret;

	(void)mech_type;

	memset(&token_init, 0, sizeof(token_init));

	ret = add_mech(&token_init.mechTypes, GSS_KRB5_MECH);
	if (ret) {
		*minor_status = ret;
		ret = GSS_S_FAILURE;
		goto end;
	}

	major_status = gss_init_sec_context(minor_status,
					    initiator_cred_handle,
					    context_handle,
					    target_name,
					    GSS_KRB5_MECH,
					    req_flags,
					    time_req,
					    input_chan_bindings,
					    input_token,
					    actual_mech_type,
					    &krb5_output_token,
					    ret_flags,
					    time_rec);
	if (GSS_ERROR(major_status)) {
		ret = major_status;
		goto end;
	}
	if (krb5_output_token.length > 0) {
		token_init.mechToken = malloc(sizeof(*token_init.mechToken));
		if (token_init.mechToken == NULL) {
			*minor_status = ENOMEM;
			ret = GSS_S_FAILURE;
			goto end;
		}
		token_init.mechToken->data = krb5_output_token.value;
		token_init.mechToken->length = krb5_output_token.length;
	}
	/*
	 * The MS implementation of SPNEGO seems to not like the mechListMIC
	 * field, so we omit it (it's optional anyway)
	 */

	buf_size = 1024;
	buf = malloc(buf_size);

	do {
		ret = encode_NegTokenInit(buf + buf_size - 1,
					  buf_size,
					  &token_init, &len);
		if (ret == 0) {
			size_t tmp;

			ret = der_put_length_and_tag(buf + buf_size - len - 1,
						     buf_size - len,
						     len,
						     ASN1_C_CONTEXT,
						     CONS,
						     0,
						     &tmp);
			if (ret == 0)
				len += tmp;
		}
		if (ret) {
			if (ret == ASN1_OVERFLOW) {
				u_char *tmp;

				buf_size *= 2;
				tmp = realloc(buf, buf_size);
				if (tmp == NULL) {
					*minor_status = ENOMEM;
					ret = GSS_S_FAILURE;
					goto end;
				}
				buf = tmp;
			} else {
				*minor_status = ret;
				ret = GSS_S_FAILURE;
				goto end;
			}
		}
	} while (ret == ASN1_OVERFLOW);

	ret = gssapi_spnego_encapsulate(minor_status,
					buf + buf_size - len, len,
					output_token, GSS_SPNEGO_MECH);
	if (ret == GSS_S_COMPLETE)
		ret = major_status;

end:
	if (token_init.mechToken != NULL) {
		free(token_init.mechToken);
		token_init.mechToken = NULL;
	}
	free_NegTokenInit(&token_init);
	if (krb5_output_token.length != 0)
		gss_release_buffer(&minor_status2, &krb5_output_token);
	if (buf)
		free(buf);

	return (ret);
}

static OM_uint32
spnego_reply(OM_uint32 *minor_status,
	     const gss_cred_id_t initiator_cred_handle,
	     gss_ctx_id_t *context_handle,
	     const gss_name_t target_name,
	     const gss_OID mech_type,
	     OM_uint32 req_flags,
	     OM_uint32 time_req,
	     const gss_channel_bindings_t input_chan_bindings,
	     const gss_buffer_t input_token,
	     gss_OID *actual_mech_type,
	     gss_buffer_t output_token,
	     OM_uint32 *ret_flags,
	     OM_uint32 *time_rec)
{
	OM_uint32 ret;
	NegTokenResp resp;
	unsigned char *buf;
	size_t buf_size;
	u_char oidbuf[17];
	size_t oidlen;
	gss_buffer_desc sub_token;
	ssize_t mech_len;
	const u_char *p;
	size_t len, taglen;

	(void)mech_type;

	output_token->length = 0;
	output_token->value  = NULL;

	/*
	 * SPNEGO doesn't include gss wrapping on SubsequentContextToken
	 * like the Kerberos 5 mech does. But lets check for it anyway.
	 */

	mech_len = gssapi_krb5_get_mech(input_token->value,
					input_token->length,
					&p);

	if (mech_len < 0) {
		buf = input_token->value;
		buf_size = input_token->length;
	} else if ((size_t)mech_len == GSS_KRB5_MECH->length &&
		   memcmp(GSS_KRB5_MECH->elements, p, mech_len) == 0)
		return (gss_init_sec_context(minor_status,
					     initiator_cred_handle,
					     context_handle,
					     target_name,
					     GSS_KRB5_MECH,
					     req_flags,
					     time_req,
					     input_chan_bindings,
					     input_token,
					     actual_mech_type,
					     output_token,
					     ret_flags,
					     time_rec));
	else if ((size_t)mech_len == GSS_SPNEGO_MECH->length &&
		 memcmp(GSS_SPNEGO_MECH->elements, p, mech_len) == 0) {
		ret = gssapi_spnego_decapsulate(minor_status,
						input_token,
						&buf,
						&buf_size,
						GSS_SPNEGO_MECH);
		if (ret)
			return (ret);
	} else
		return (GSS_S_BAD_MECH);

	ret = der_match_tag_and_length(buf, buf_size,
				       ASN1_C_CONTEXT, CONS, 1, &len, &taglen);
	if (ret)
		return (ret);

	if(len > buf_size - taglen)
		return (ASN1_OVERRUN);

	ret = decode_NegTokenResp(buf + taglen, len, &resp, NULL);
	if (ret) {
		*minor_status = ENOMEM;
		return (GSS_S_FAILURE);
	}

	if (resp.negState == NULL ||
	    *(resp.negState) == reject ||
	    resp.supportedMech == NULL) {
		free_NegTokenResp(&resp);
		return (GSS_S_BAD_MECH);
	}

	ret = der_put_oid(oidbuf + sizeof(oidbuf) - 1,
			  sizeof(oidbuf),
			  resp.supportedMech,
			  &oidlen);
	if (ret || oidlen != GSS_KRB5_MECH->length ||
	    memcmp(oidbuf + sizeof(oidbuf) - oidlen,
		   GSS_KRB5_MECH->elements,
		   oidlen) != 0) {
		free_NegTokenResp(&resp);
		return GSS_S_BAD_MECH;
	}

	if (resp.responseToken != NULL) {
		sub_token.length = resp.responseToken->length;
		sub_token.value  = resp.responseToken->data;
	} else {
		sub_token.length = 0;
		sub_token.value  = NULL;
	}

	ret = gss_init_sec_context(minor_status,
				   initiator_cred_handle,
				   context_handle,
				   target_name,
				   GSS_KRB5_MECH,
				   req_flags,
				   time_req,
				   input_chan_bindings,
				   &sub_token,
				   actual_mech_type,
				   output_token,
				   ret_flags,
				   time_rec);
	if (ret) {
		free_NegTokenResp(&resp);
		return (ret);
	}

	/*
	 * XXXSRA I don't think this limited implementation ever needs
	 * to check the MIC -- our preferred mechanism (Kerberos)
	 * authenticates its own messages and is the only mechanism
	 * we'll accept, so if the mechanism negotiation completes
	 * successfully, we don't need the MIC.  See RFC 4178.
	 */

	free_NegTokenResp(&resp);
	return (ret);
}



OM_uint32
gss_init_sec_context_spnego(OM_uint32 *minor_status,
			    const gss_cred_id_t initiator_cred_handle,
			    gss_ctx_id_t *context_handle,
			    const gss_name_t target_name,
			    const gss_OID mech_type,
			    OM_uint32 req_flags,
			    OM_uint32 time_req,
			    const gss_channel_bindings_t input_chan_bindings,
			    const gss_buffer_t input_token,
			    gss_OID *actual_mech_type,
			    gss_buffer_t output_token,
			    OM_uint32 *ret_flags,
			    OM_uint32 *time_rec)
{
	/* Dirty trick to suppress compiler warnings */

	/* Figure out whether we're starting over or processing a reply */

	if (input_token == GSS_C_NO_BUFFER || input_token->length == 0)
		return (spnego_initial(minor_status,
				       initiator_cred_handle,
				       context_handle,
				       target_name,
				       mech_type,
				       req_flags,
				       time_req,
				       input_chan_bindings,
				       input_token,
				       actual_mech_type,
				       output_token,
				       ret_flags,
				       time_rec));
	else
		return (spnego_reply(minor_status,
				     initiator_cred_handle,
				     context_handle,
				     target_name,
				     mech_type,
				     req_flags,
				     time_req,
				     input_chan_bindings,
				     input_token,
				     actual_mech_type,
				     output_token,
				     ret_flags,
				     time_rec));
}

#endif /* GSSAPI */
