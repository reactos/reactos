/*
 * Copyright (C) 2004-2008  Internet Systems Consortium, Inc. ("ISC")
 * Copyright (C) 2000-2002  Internet Software Consortium.
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

/*
 * $Id: gssapi_link.c,v 1.12 2008/11/11 03:55:01 marka Exp $
 */

#include <config.h>

#ifdef GSSAPI

#include <isc/buffer.h>
#include <isc/mem.h>
#include <isc/string.h>
#include <isc/util.h>

#include <dst/result.h>

#include "dst_internal.h"
#include "dst_parse.h"

#include <dst/gssapi.h>

#define INITIAL_BUFFER_SIZE 1024
#define BUFFER_EXTRA 1024

#define REGION_TO_GBUFFER(r, gb) \
	do { \
		(gb).length = (r).length; \
		(gb).value = (r).base; \
	} while (0)


struct dst_gssapi_signverifyctx {
	isc_buffer_t *buffer;
};

/*%
 * Allocate a temporary "context" for use in gathering data for signing
 * or verifying.
 */
static isc_result_t
gssapi_create_signverify_ctx(dst_key_t *key, dst_context_t *dctx) {
	dst_gssapi_signverifyctx_t *ctx;
	isc_result_t result;

	UNUSED(key);

	ctx = isc_mem_get(dctx->mctx, sizeof(dst_gssapi_signverifyctx_t));
	if (ctx == NULL)
		return (ISC_R_NOMEMORY);
	ctx->buffer = NULL;
	result = isc_buffer_allocate(dctx->mctx, &ctx->buffer,
				     INITIAL_BUFFER_SIZE);
	if (result != ISC_R_SUCCESS) {
		isc_mem_put(dctx->mctx, ctx, sizeof(dst_gssapi_signverifyctx_t));
		return (result);
	}

	dctx->ctxdata.gssctx = ctx;

	return (ISC_R_SUCCESS);
}

/*%
 * Destroy the temporary sign/verify context.
 */
static void
gssapi_destroy_signverify_ctx(dst_context_t *dctx) {
	dst_gssapi_signverifyctx_t *ctx = dctx->ctxdata.gssctx;

	if (ctx != NULL) {
		if (ctx->buffer != NULL)
			isc_buffer_free(&ctx->buffer);
		isc_mem_put(dctx->mctx, ctx, sizeof(dst_gssapi_signverifyctx_t));
		dctx->ctxdata.gssctx = NULL;
	}
}

/*%
 * Add data to our running buffer of data we will be signing or verifying.
 * This code will see if the new data will fit in our existing buffer, and
 * copy it in if it will.  If not, it will attempt to allocate a larger
 * buffer and copy old+new into it, and free the old buffer.
 */
static isc_result_t
gssapi_adddata(dst_context_t *dctx, const isc_region_t *data) {
	dst_gssapi_signverifyctx_t *ctx = dctx->ctxdata.gssctx;
	isc_buffer_t *newbuffer = NULL;
	isc_region_t r;
	unsigned int length;
	isc_result_t result;

	result = isc_buffer_copyregion(ctx->buffer, data);
	if (result == ISC_R_SUCCESS)
		return (ISC_R_SUCCESS);

	length = isc_buffer_length(ctx->buffer) + data->length + BUFFER_EXTRA;

	result = isc_buffer_allocate(dctx->mctx, &newbuffer, length);
	if (result != ISC_R_SUCCESS)
		return (result);

	isc_buffer_usedregion(ctx->buffer, &r);
	(void)isc_buffer_copyregion(newbuffer, &r);
	(void)isc_buffer_copyregion(newbuffer, data);

	isc_buffer_free(&ctx->buffer);
	ctx->buffer = newbuffer;

	return (ISC_R_SUCCESS);
}

/*%
 * Sign.
 */
static isc_result_t
gssapi_sign(dst_context_t *dctx, isc_buffer_t *sig) {
	dst_gssapi_signverifyctx_t *ctx = dctx->ctxdata.gssctx;
	isc_region_t message;
	gss_buffer_desc gmessage, gsig;
	OM_uint32 minor, gret;
	gss_ctx_id_t gssctx = dctx->key->keydata.gssctx;
	char buf[1024];

	/*
	 * Convert the data we wish to sign into a structure gssapi can
	 * understand.
	 */
	isc_buffer_usedregion(ctx->buffer, &message);
	REGION_TO_GBUFFER(message, gmessage);

	/*
	 * Generate the signature.
	 */
	gret = gss_get_mic(&minor, gssctx, GSS_C_QOP_DEFAULT, &gmessage,
			   &gsig);

	/*
	 * If it did not complete, we log the result and return a generic
	 * failure code.
	 */
	if (gret != GSS_S_COMPLETE) {
		gss_log(3, "GSS sign error: %s",
			gss_error_tostring(gret, minor, buf, sizeof(buf)));
		return (ISC_R_FAILURE);
	}

	/*
	 * If it will not fit in our allocated buffer, return that we need
	 * more space.
	 */
	if (gsig.length > isc_buffer_availablelength(sig)) {
		gss_release_buffer(&minor, &gsig);
		return (ISC_R_NOSPACE);
	}

	/*
	 * Copy the output into our buffer space, and release the gssapi
	 * allocated space.
	 */
	isc_buffer_putmem(sig, gsig.value, gsig.length);
	if (gsig.length != 0)
		gss_release_buffer(&minor, &gsig);

	return (ISC_R_SUCCESS);
}

/*%
 * Verify.
 */
static isc_result_t
gssapi_verify(dst_context_t *dctx, const isc_region_t *sig) {
	dst_gssapi_signverifyctx_t *ctx = dctx->ctxdata.gssctx;
	isc_region_t message, r;
	gss_buffer_desc gmessage, gsig;
	OM_uint32 minor, gret;
	gss_ctx_id_t gssctx = dctx->key->keydata.gssctx;
	unsigned char *buf;
	char err[1024];

	/*
	 * Convert the data we wish to sign into a structure gssapi can
	 * understand.
	 */
	isc_buffer_usedregion(ctx->buffer, &message);
	REGION_TO_GBUFFER(message, gmessage);

	/*
	 * XXXMLG
	 * It seem that gss_verify_mic() modifies the signature buffer,
	 * at least on Heimdal's implementation.  Copy it here to an allocated
	 * buffer.
	 */
	buf = isc_mem_allocate(dst__memory_pool, sig->length);
	if (buf == NULL)
		return (ISC_R_FAILURE);
	memcpy(buf, sig->base, sig->length);
	r.base = buf;
	r.length = sig->length;
	REGION_TO_GBUFFER(r, gsig);

	/*
	 * Verify the data.
	 */
	gret = gss_verify_mic(&minor, gssctx, &gmessage, &gsig, NULL);

	isc_mem_free(dst__memory_pool, buf);

	/*
	 * Convert return codes into something useful to us.
	 */
	if (gret != GSS_S_COMPLETE) {
		gss_log(3, "GSS verify error: %s",
			gss_error_tostring(gret, minor, err, sizeof(err)));
		if (gret == GSS_S_DEFECTIVE_TOKEN ||
		    gret == GSS_S_BAD_SIG ||
		    gret == GSS_S_DUPLICATE_TOKEN ||
		    gret == GSS_S_OLD_TOKEN ||
		    gret == GSS_S_UNSEQ_TOKEN ||
		    gret == GSS_S_GAP_TOKEN ||
		    gret == GSS_S_CONTEXT_EXPIRED ||
		    gret == GSS_S_NO_CONTEXT ||
		    gret == GSS_S_FAILURE)
			return(DST_R_VERIFYFAILURE);
		else
			return (ISC_R_FAILURE);
	}

	return (ISC_R_SUCCESS);
}

static isc_boolean_t
gssapi_compare(const dst_key_t *key1, const dst_key_t *key2) {
	gss_ctx_id_t gsskey1 = key1->keydata.gssctx;
	gss_ctx_id_t gsskey2 = key2->keydata.gssctx;

	/* No idea */
	return (ISC_TF(gsskey1 == gsskey2));
}

static isc_result_t
gssapi_generate(dst_key_t *key, int unused) {
	UNUSED(key);
	UNUSED(unused);

	/* No idea */
	return (ISC_R_FAILURE);
}

static isc_boolean_t
gssapi_isprivate(const dst_key_t *key) {
	UNUSED(key);
	return (ISC_TRUE);
}

static void
gssapi_destroy(dst_key_t *key) {
	REQUIRE(key != NULL);
	dst_gssapi_deletectx(key->mctx, &key->keydata.gssctx);
	key->keydata.gssctx = NULL;
}

static dst_func_t gssapi_functions = {
	gssapi_create_signverify_ctx,
	gssapi_destroy_signverify_ctx,
	gssapi_adddata,
	gssapi_sign,
	gssapi_verify,
	NULL, /*%< computesecret */
	gssapi_compare,
	NULL, /*%< paramcompare */
	gssapi_generate,
	gssapi_isprivate,
	gssapi_destroy,
	NULL, /*%< todns */
	NULL, /*%< fromdns */
	NULL, /*%< tofile */
	NULL, /*%< parse */
	NULL, /*%< cleanup */
	NULL  /*%< fromlabel */
};

isc_result_t
dst__gssapi_init(dst_func_t **funcp) {
	REQUIRE(funcp != NULL);
	if (*funcp == NULL)
		*funcp = &gssapi_functions;
	return (ISC_R_SUCCESS);
}

#else
int  gssapi_link_unneeded = 1;
#endif

/*! \file */
