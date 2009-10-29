/*
 * Copyright (C) 2004, 2005, 2007  Internet Systems Consortium, Inc. ("ISC")
 * Copyright (C) 2000-2003  Internet Software Consortium.
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

/* $Id: getrrset.c,v 1.18 2007/06/19 23:47:22 tbox Exp $ */

/*! \file */

/**
 * DESCRIPTION
 * 
 *    lwres_getrrsetbyname() gets a set of resource records associated with
 *    a hostname, class, and type. hostname is a pointer a to
 *    null-terminated string. The flags field is currently unused and must
 *    be zero.
 * 
 *    After a successful call to lwres_getrrsetbyname(), *res is a pointer
 *    to an #rrsetinfo structure, containing a list of one or more #rdatainfo
 *    structures containing resource records and potentially another list of
 *    rdatainfo structures containing SIG resource records associated with
 *    those records. The members #rri_rdclass and #rri_rdtype are copied from
 *    the parameters. #rri_ttl and #rri_name are properties of the obtained
 *    rrset. The resource records contained in #rri_rdatas and #rri_sigs are
 *    in uncompressed DNS wire format. Properties of the rdataset are
 *    represented in the #rri_flags bitfield. If the #RRSET_VALIDATED bit is
 *    set, the data has been DNSSEC validated and the signatures verified.
 * 
 *    All of the information returned by lwres_getrrsetbyname() is
 *    dynamically allocated: the rrsetinfo and rdatainfo structures, and the
 *    canonical host name strings pointed to by the rrsetinfostructure.
 *    Memory allocated for the dynamically allocated structures created by a
 *    successful call to lwres_getrrsetbyname() is released by
 *    lwres_freerrset(). rrset is a pointer to a struct rrset created by a
 *    call to lwres_getrrsetbyname().
 * 
 *    The following structures are used:
 * 
 * \code
 * struct  rdatainfo {
 *         unsigned int            rdi_length;     // length of data
 *         unsigned char           *rdi_data;      // record data
 * };
 * 
 * struct  rrsetinfo {
 *         unsigned int            rri_flags;      // RRSET_VALIDATED...
 *         unsigned int            rri_rdclass;    // class number
 *         unsigned int            rri_rdtype;     // RR type number
 *         unsigned int            rri_ttl;        // time to live
 *         unsigned int            rri_nrdatas;    // size of rdatas array
 *         unsigned int            rri_nsigs;      // size of sigs array
 *         char                    *rri_name;      // canonical name
 *         struct rdatainfo        *rri_rdatas;    // individual records
 *         struct rdatainfo        *rri_sigs;      // individual signatures
 * };
 * \endcode
 * 
 * \section getrrset_return Return Values
 * 
 *    lwres_getrrsetbyname() returns zero on success, and one of the
 *    following error codes if an error occurred:
 * 
 * \li   #ERRSET_NONAME: the name does not exist
 * 
 * \li   #ERRSET_NODATA:
 *           the name exists, but does not have data of the desired type
 * 
 * \li   #ERRSET_NOMEMORY:
 *           memory could not be allocated
 * 
 * \li   #ERRSET_INVAL:
 *           a parameter is invalid
 * 
 * \li   #ERRSET_FAIL:
 *           other failure
 */

#include <config.h>

#include <string.h>
#include <errno.h>
#include <stdlib.h>

#include <lwres/lwres.h>
#include <lwres/net.h>
#include <lwres/netdb.h>	/* XXX #include <netdb.h> */

#include "assert_p.h"

/*!
 * Structure to map results
 */
static unsigned int
lwresult_to_result(lwres_result_t lwresult) {
	switch (lwresult) {
	case LWRES_R_SUCCESS:	return (ERRSET_SUCCESS);
	case LWRES_R_NOMEMORY:	return (ERRSET_NOMEMORY);
	case LWRES_R_NOTFOUND:	return (ERRSET_NONAME);
	case LWRES_R_TYPENOTFOUND: return (ERRSET_NODATA);
	default:		return (ERRSET_FAIL);
	}
}

/*@{*/
/*!
 * malloc / calloc functions that guarantee to only
 * return NULL if there is an error, like they used
 * to before the ANSI C committee broke them.
 */

static void *
sane_malloc(size_t size) {
	if (size == 0U)
		size = 1;
	return (malloc(size));
}

static void *
sane_calloc(size_t number, size_t size) {
	size_t len = number * size;
	void *mem  = sane_malloc(len);
	if (mem != NULL)
		memset(mem, 0, len);
	return (mem);
}
/*@}*/

/*% Returns a set of resource records associated with a hostname, class, and type. hostname is a pointer a to null-terminated string. */
int
lwres_getrrsetbyname(const char *hostname, unsigned int rdclass,
		     unsigned int rdtype, unsigned int flags,
		     struct rrsetinfo **res)
{
	lwres_context_t *lwrctx = NULL;
	lwres_result_t lwresult;
	lwres_grbnresponse_t *response = NULL;
	struct rrsetinfo *rrset = NULL;
	unsigned int i;
	unsigned int lwflags;
	unsigned int result;

	if (rdclass > 0xffff || rdtype > 0xffff) {
		result = ERRSET_INVAL;
		goto fail;
	}

	/*
	 * Don't allow queries of class or type ANY
	 */
	if (rdclass == 0xff || rdtype == 0xff) {
		result = ERRSET_INVAL;
		goto fail;
	}

	lwresult = lwres_context_create(&lwrctx, NULL, NULL, NULL, 0);
	if (lwresult != LWRES_R_SUCCESS) {
		result = lwresult_to_result(lwresult);
		goto fail;
	}
	(void) lwres_conf_parse(lwrctx, lwres_resolv_conf);

	/*
	 * If any input flags were defined, lwflags would be set here
	 * based on them
	 */
	UNUSED(flags);
	lwflags = 0;

	lwresult = lwres_getrdatabyname(lwrctx, hostname,
					(lwres_uint16_t)rdclass, 
					(lwres_uint16_t)rdtype,
					lwflags, &response);
	if (lwresult != LWRES_R_SUCCESS) {
		result = lwresult_to_result(lwresult);
		goto fail;
	}

	rrset = sane_malloc(sizeof(struct rrsetinfo));
	if (rrset == NULL) {
		result = ERRSET_NOMEMORY;
		goto fail;
	}
	rrset->rri_name = NULL;
	rrset->rri_rdclass = response->rdclass;
	rrset->rri_rdtype = response->rdtype;
	rrset->rri_ttl = response->ttl;
	rrset->rri_flags = 0;
	rrset->rri_nrdatas = 0;
	rrset->rri_rdatas = NULL;
	rrset->rri_nsigs = 0;
	rrset->rri_sigs = NULL;

	rrset->rri_name = sane_malloc(response->realnamelen + 1);
	if (rrset->rri_name == NULL) {
		result = ERRSET_NOMEMORY;
		goto fail;
	}
	strncpy(rrset->rri_name, response->realname, response->realnamelen);
	rrset->rri_name[response->realnamelen] = 0;

	if ((response->flags & LWRDATA_VALIDATED) != 0)
		rrset->rri_flags |= RRSET_VALIDATED;

	rrset->rri_nrdatas = response->nrdatas;
	rrset->rri_rdatas = sane_calloc(rrset->rri_nrdatas,
				   sizeof(struct rdatainfo));
	if (rrset->rri_rdatas == NULL) {
		result = ERRSET_NOMEMORY;
		goto fail;
	}
	for (i = 0; i < rrset->rri_nrdatas; i++) {
		rrset->rri_rdatas[i].rdi_length = response->rdatalen[i];
		rrset->rri_rdatas[i].rdi_data =
				sane_malloc(rrset->rri_rdatas[i].rdi_length);
		if (rrset->rri_rdatas[i].rdi_data == NULL) {
			result = ERRSET_NOMEMORY;
			goto fail;
		}
		memcpy(rrset->rri_rdatas[i].rdi_data, response->rdatas[i],
		       rrset->rri_rdatas[i].rdi_length);
	}
	rrset->rri_nsigs = response->nsigs;
	rrset->rri_sigs = sane_calloc(rrset->rri_nsigs,
				      sizeof(struct rdatainfo));
	if (rrset->rri_sigs == NULL) {
		result = ERRSET_NOMEMORY;
		goto fail;
	}
	for (i = 0; i < rrset->rri_nsigs; i++) {
		rrset->rri_sigs[i].rdi_length = response->siglen[i];
		rrset->rri_sigs[i].rdi_data =
				sane_malloc(rrset->rri_sigs[i].rdi_length);
		if (rrset->rri_sigs[i].rdi_data == NULL) {
			result = ERRSET_NOMEMORY;
			goto fail;
		}
		memcpy(rrset->rri_sigs[i].rdi_data, response->sigs[i],
		       rrset->rri_sigs[i].rdi_length);
	}

	lwres_grbnresponse_free(lwrctx, &response);
	lwres_conf_clear(lwrctx);
	lwres_context_destroy(&lwrctx);
	*res = rrset;
	return (ERRSET_SUCCESS);
 fail:
	if (rrset != NULL)
		lwres_freerrset(rrset);
	if (response != NULL)
		lwres_grbnresponse_free(lwrctx, &response);
	if (lwrctx != NULL) {
		lwres_conf_clear(lwrctx);
		lwres_context_destroy(&lwrctx);
	}
	return (result);
}

/*% Releases memory allocated for the dynamically allocated structures created by a successful call to lwres_getrrsetbyname(). */
void
lwres_freerrset(struct rrsetinfo *rrset) {
	unsigned int i;
	for (i = 0; i < rrset->rri_nrdatas; i++) {
		if (rrset->rri_rdatas[i].rdi_data == NULL)
			break;
		free(rrset->rri_rdatas[i].rdi_data);
	}
	free(rrset->rri_rdatas);
	for (i = 0; i < rrset->rri_nsigs; i++) {
		if (rrset->rri_sigs[i].rdi_data == NULL)
			break;
		free(rrset->rri_sigs[i].rdi_data);
	}
	free(rrset->rri_sigs);
	free(rrset->rri_name);
	free(rrset);
}
