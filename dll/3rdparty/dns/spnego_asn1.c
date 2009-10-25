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

/* $Id: spnego_asn1.c,v 1.4 2007/06/19 23:47:16 tbox Exp $ */

/*! \file
 * \brief Method routines generated from SPNEGO ASN.1 module.
 * See spnego_asn1.pl for details.  Do not edit.
 */

/* Generated from spnego.asn1 */
/* Do not edit */

#ifndef __asn1_h__
#define __asn1_h__


#ifndef __asn1_common_definitions__
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

#define ASN1_MALLOC_ENCODE(T, B, BL, S, L, R)                  \
  do {                                                         \
    (BL) = length_##T((S));                                    \
    (B) = malloc((BL));                                        \
    if((B) == NULL) {                                          \
      (R) = ENOMEM;                                            \
    } else {                                                   \
      (R) = encode_##T(((unsigned char*)(B)) + (BL) - 1, (BL), \
                       (S), (L));                              \
      if((R) != 0) {                                           \
        free((B));                                             \
        (B) = NULL;                                            \
      }                                                        \
    }                                                          \
  } while (0)

#endif

/*
 * MechType ::= OBJECT IDENTIFIER
 */

typedef oid MechType;

static int encode_MechType(unsigned char *, size_t, const MechType *, size_t *);
static int decode_MechType(const unsigned char *, size_t, MechType *, size_t *);
static void free_MechType(MechType *);
/* unused declaration: length_MechType */
/* unused declaration: copy_MechType */


/*
 * MechTypeList ::= SEQUENCE OF MechType
 */

typedef struct MechTypeList {
	unsigned int len;
	MechType *val;
} MechTypeList;

static int encode_MechTypeList(unsigned char *, size_t, const MechTypeList *, size_t *);
static int decode_MechTypeList(const unsigned char *, size_t, MechTypeList *, size_t *);
static void free_MechTypeList(MechTypeList *);
/* unused declaration: length_MechTypeList */
/* unused declaration: copy_MechTypeList */


/*
 * ContextFlags ::= BIT STRING { delegFlag(0), mutualFlag(1), replayFlag(2),
 * sequenceFlag(3), anonFlag(4), confFlag(5), integFlag(6) }
 */

typedef struct ContextFlags {
	unsigned int delegFlag:1;
	unsigned int mutualFlag:1;
	unsigned int replayFlag:1;
	unsigned int sequenceFlag:1;
	unsigned int anonFlag:1;
	unsigned int confFlag:1;
	unsigned int integFlag:1;
} ContextFlags;


static int encode_ContextFlags(unsigned char *, size_t, const ContextFlags *, size_t *);
static int decode_ContextFlags(const unsigned char *, size_t, ContextFlags *, size_t *);
static void free_ContextFlags(ContextFlags *);
/* unused declaration: length_ContextFlags */
/* unused declaration: copy_ContextFlags */
/* unused declaration: ContextFlags2int */
/* unused declaration: int2ContextFlags */
/* unused declaration: asn1_ContextFlags_units */

/*
 * NegTokenInit ::= SEQUENCE { mechTypes[0]    MechTypeList, reqFlags[1]
 * ContextFlags OPTIONAL, mechToken[2]    OCTET STRING OPTIONAL,
 * mechListMIC[3]  OCTET STRING OPTIONAL }
 */

typedef struct NegTokenInit {
	MechTypeList mechTypes;
	ContextFlags *reqFlags;
	octet_string *mechToken;
	octet_string *mechListMIC;
} NegTokenInit;

static int encode_NegTokenInit(unsigned char *, size_t, const NegTokenInit *, size_t *);
static int decode_NegTokenInit(const unsigned char *, size_t, NegTokenInit *, size_t *);
static void free_NegTokenInit(NegTokenInit *);
/* unused declaration: length_NegTokenInit */
/* unused declaration: copy_NegTokenInit */


/*
 * NegTokenResp ::= SEQUENCE { negState[0]       ENUMERATED {
 * accept-completed(0), accept-incomplete(1), reject(2), request-mic(3) }
 * OPTIONAL, supportedMech[1]  MechType OPTIONAL, responseToken[2]  OCTET
 * STRING OPTIONAL, mechListMIC[3]    OCTET STRING OPTIONAL }
 */

typedef struct NegTokenResp {
	enum {
		accept_completed = 0,
		accept_incomplete = 1,
		reject = 2,
		request_mic = 3
	} *negState;

	MechType *supportedMech;
	octet_string *responseToken;
	octet_string *mechListMIC;
} NegTokenResp;

static int encode_NegTokenResp(unsigned char *, size_t, const NegTokenResp *, size_t *);
static int decode_NegTokenResp(const unsigned char *, size_t, NegTokenResp *, size_t *);
static void free_NegTokenResp(NegTokenResp *);
/* unused declaration: length_NegTokenResp */
/* unused declaration: copy_NegTokenResp */




#endif				/* __asn1_h__ */
/* Generated from spnego.asn1 */
/* Do not edit */


#define BACK if (e) return e; p -= l; len -= l; ret += l

static int
encode_MechType(unsigned char *p, size_t len, const MechType * data, size_t * size)
{
	size_t ret = 0;
	size_t l;
	int i, e;

	i = 0;
	e = encode_oid(p, len, data, &l);
	BACK;
	*size = ret;
	return 0;
}

#define FORW if(e) goto fail; p += l; len -= l; ret += l

static int
decode_MechType(const unsigned char *p, size_t len, MechType * data, size_t * size)
{
	size_t ret = 0, reallen;
	size_t l;
	int e;

	memset(data, 0, sizeof(*data));
	reallen = 0;
	e = decode_oid(p, len, data, &l);
	FORW;
	if (size)
		*size = ret;
	return 0;
fail:
	free_MechType(data);
	return e;
}

static void
free_MechType(MechType * data)
{
	free_oid(data);
}

/* unused function: length_MechType */


/* unused function: copy_MechType */

/* Generated from spnego.asn1 */
/* Do not edit */


#define BACK if (e) return e; p -= l; len -= l; ret += l

static int
encode_MechTypeList(unsigned char *p, size_t len, const MechTypeList * data, size_t * size)
{
	size_t ret = 0;
	size_t l;
	int i, e;

	i = 0;
	for (i = (data)->len - 1; i >= 0; --i) {
		int oldret = ret;
		ret = 0;
		e = encode_MechType(p, len, &(data)->val[i], &l);
		BACK;
		ret += oldret;
	}
	e = der_put_length_and_tag(p, len, ret, ASN1_C_UNIV, CONS, UT_Sequence, &l);
	BACK;
	*size = ret;
	return 0;
}

#define FORW if(e) goto fail; p += l; len -= l; ret += l

static int
decode_MechTypeList(const unsigned char *p, size_t len, MechTypeList * data, size_t * size)
{
	size_t ret = 0, reallen;
	size_t l;
	int e;

	memset(data, 0, sizeof(*data));
	reallen = 0;
	e = der_match_tag_and_length(p, len, ASN1_C_UNIV, CONS, UT_Sequence, &reallen, &l);
	FORW;
	if (len < reallen)
		return ASN1_OVERRUN;
	len = reallen;
	{
		size_t origlen = len;
		int oldret = ret;
		ret = 0;
		(data)->len = 0;
		(data)->val = NULL;
		while (ret < origlen) {
			(data)->len++;
			(data)->val = realloc((data)->val, sizeof(*((data)->val)) * (data)->len);
			e = decode_MechType(p, len, &(data)->val[(data)->len - 1], &l);
			FORW;
			len = origlen - ret;
		}
		ret += oldret;
	}
	if (size)
		*size = ret;
	return 0;
fail:
	free_MechTypeList(data);
	return e;
}

static void
free_MechTypeList(MechTypeList * data)
{
	while ((data)->len) {
		free_MechType(&(data)->val[(data)->len - 1]);
		(data)->len--;
	}
	free((data)->val);
	(data)->val = NULL;
}

/* unused function: length_MechTypeList */


/* unused function: copy_MechTypeList */

/* Generated from spnego.asn1 */
/* Do not edit */


#define BACK if (e) return e; p -= l; len -= l; ret += l

static int
encode_ContextFlags(unsigned char *p, size_t len, const ContextFlags * data, size_t * size)
{
	size_t ret = 0;
	size_t l;
	int i, e;

	i = 0;
	{
		unsigned char c = 0;
		*p-- = c;
		len--;
		ret++;
		c = 0;
		*p-- = c;
		len--;
		ret++;
		c = 0;
		*p-- = c;
		len--;
		ret++;
		c = 0;
		if (data->integFlag)
			c |= 1 << 1;
		if (data->confFlag)
			c |= 1 << 2;
		if (data->anonFlag)
			c |= 1 << 3;
		if (data->sequenceFlag)
			c |= 1 << 4;
		if (data->replayFlag)
			c |= 1 << 5;
		if (data->mutualFlag)
			c |= 1 << 6;
		if (data->delegFlag)
			c |= 1 << 7;
		*p-- = c;
		*p-- = 0;
		len -= 2;
		ret += 2;
	}

	e = der_put_length_and_tag(p, len, ret, ASN1_C_UNIV, PRIM, UT_BitString, &l);
	BACK;
	*size = ret;
	return 0;
}

#define FORW if(e) goto fail; p += l; len -= l; ret += l

static int
decode_ContextFlags(const unsigned char *p, size_t len, ContextFlags * data, size_t * size)
{
	size_t ret = 0, reallen;
	size_t l;
	int e;

	memset(data, 0, sizeof(*data));
	reallen = 0;
	e = der_match_tag_and_length(p, len, ASN1_C_UNIV, PRIM, UT_BitString, &reallen, &l);
	FORW;
	if (len < reallen)
		return ASN1_OVERRUN;
	p++;
	len--;
	reallen--;
	ret++;
	data->delegFlag = (*p >> 7) & 1;
	data->mutualFlag = (*p >> 6) & 1;
	data->replayFlag = (*p >> 5) & 1;
	data->sequenceFlag = (*p >> 4) & 1;
	data->anonFlag = (*p >> 3) & 1;
	data->confFlag = (*p >> 2) & 1;
	data->integFlag = (*p >> 1) & 1;
	p += reallen;
	len -= reallen;
	ret += reallen;
	if (size)
		*size = ret;
	return 0;
fail:
	free_ContextFlags(data);
	return e;
}

static void
free_ContextFlags(ContextFlags * data)
{
	(void)data;
}

/* unused function: length_ContextFlags */


/* unused function: copy_ContextFlags */


/* unused function: ContextFlags2int */


/* unused function: int2ContextFlags */


/* unused variable: ContextFlags_units */

/* unused function: asn1_ContextFlags_units */

/* Generated from spnego.asn1 */
/* Do not edit */


#define BACK if (e) return e; p -= l; len -= l; ret += l

static int
encode_NegTokenInit(unsigned char *p, size_t len, const NegTokenInit * data, size_t * size)
{
	size_t ret = 0;
	size_t l;
	int i, e;

	i = 0;
	if ((data)->mechListMIC) {
		int oldret = ret;
		ret = 0;
		e = encode_octet_string(p, len, (data)->mechListMIC, &l);
		BACK;
		e = der_put_length_and_tag(p, len, ret, ASN1_C_CONTEXT, CONS, 3, &l);
		BACK;
		ret += oldret;
	}
	if ((data)->mechToken) {
		int oldret = ret;
		ret = 0;
		e = encode_octet_string(p, len, (data)->mechToken, &l);
		BACK;
		e = der_put_length_and_tag(p, len, ret, ASN1_C_CONTEXT, CONS, 2, &l);
		BACK;
		ret += oldret;
	}
	if ((data)->reqFlags) {
		int oldret = ret;
		ret = 0;
		e = encode_ContextFlags(p, len, (data)->reqFlags, &l);
		BACK;
		e = der_put_length_and_tag(p, len, ret, ASN1_C_CONTEXT, CONS, 1, &l);
		BACK;
		ret += oldret;
	} {
		int oldret = ret;
		ret = 0;
		e = encode_MechTypeList(p, len, &(data)->mechTypes, &l);
		BACK;
		e = der_put_length_and_tag(p, len, ret, ASN1_C_CONTEXT, CONS, 0, &l);
		BACK;
		ret += oldret;
	}
	e = der_put_length_and_tag(p, len, ret, ASN1_C_UNIV, CONS, UT_Sequence, &l);
	BACK;
	*size = ret;
	return 0;
}

#define FORW if(e) goto fail; p += l; len -= l; ret += l

static int
decode_NegTokenInit(const unsigned char *p, size_t len, NegTokenInit * data, size_t * size)
{
	size_t ret = 0, reallen;
	size_t l;
	int e;

	memset(data, 0, sizeof(*data));
	reallen = 0;
	e = der_match_tag_and_length(p, len, ASN1_C_UNIV, CONS, UT_Sequence, &reallen, &l);
	FORW;
	{
		int dce_fix;
		if ((dce_fix = fix_dce(reallen, &len)) < 0)
			return ASN1_BAD_FORMAT;
		{
			size_t newlen, oldlen;

			e = der_match_tag(p, len, ASN1_C_CONTEXT, CONS, 0, &l);
			if (e)
				return e;
			else {
				p += l;
				len -= l;
				ret += l;
				e = der_get_length(p, len, &newlen, &l);
				FORW;
				{
					int dce_fix;
					oldlen = len;
					if ((dce_fix = fix_dce(newlen, &len)) < 0)
						return ASN1_BAD_FORMAT;
					e = decode_MechTypeList(p, len, &(data)->mechTypes, &l);
					FORW;
					if (dce_fix) {
						e = der_match_tag_and_length(p, len, (Der_class) 0, (Der_type) 0, 0, &reallen, &l);
						FORW;
					} else
						len = oldlen - newlen;
				}
			}
		}
		{
			size_t newlen, oldlen;

			e = der_match_tag(p, len, ASN1_C_CONTEXT, CONS, 1, &l);
			if (e)
				(data)->reqFlags = NULL;
			else {
				p += l;
				len -= l;
				ret += l;
				e = der_get_length(p, len, &newlen, &l);
				FORW;
				{
					int dce_fix;
					oldlen = len;
					if ((dce_fix = fix_dce(newlen, &len)) < 0)
						return ASN1_BAD_FORMAT;
					(data)->reqFlags = malloc(sizeof(*(data)->reqFlags));
					if ((data)->reqFlags == NULL)
						return ENOMEM;
					e = decode_ContextFlags(p, len, (data)->reqFlags, &l);
					FORW;
					if (dce_fix) {
						e = der_match_tag_and_length(p, len, (Der_class) 0, (Der_type) 0, 0, &reallen, &l);
						FORW;
					} else
						len = oldlen - newlen;
				}
			}
		}
		{
			size_t newlen, oldlen;

			e = der_match_tag(p, len, ASN1_C_CONTEXT, CONS, 2, &l);
			if (e)
				(data)->mechToken = NULL;
			else {
				p += l;
				len -= l;
				ret += l;
				e = der_get_length(p, len, &newlen, &l);
				FORW;
				{
					int dce_fix;
					oldlen = len;
					if ((dce_fix = fix_dce(newlen, &len)) < 0)
						return ASN1_BAD_FORMAT;
					(data)->mechToken = malloc(sizeof(*(data)->mechToken));
					if ((data)->mechToken == NULL)
						return ENOMEM;
					e = decode_octet_string(p, len, (data)->mechToken, &l);
					FORW;
					if (dce_fix) {
						e = der_match_tag_and_length(p, len, (Der_class) 0, (Der_type) 0, 0, &reallen, &l);
						FORW;
					} else
						len = oldlen - newlen;
				}
			}
		}
		{
			size_t newlen, oldlen;

			e = der_match_tag(p, len, ASN1_C_CONTEXT, CONS, 3, &l);
			if (e)
				(data)->mechListMIC = NULL;
			else {
				p += l;
				len -= l;
				ret += l;
				e = der_get_length(p, len, &newlen, &l);
				FORW;
				{
					int dce_fix;
					oldlen = len;
					if ((dce_fix = fix_dce(newlen, &len)) < 0)
						return ASN1_BAD_FORMAT;
					(data)->mechListMIC = malloc(sizeof(*(data)->mechListMIC));
					if ((data)->mechListMIC == NULL)
						return ENOMEM;
					e = decode_octet_string(p, len, (data)->mechListMIC, &l);
					FORW;
					if (dce_fix) {
						e = der_match_tag_and_length(p, len, (Der_class) 0, (Der_type) 0, 0, &reallen, &l);
						FORW;
					} else
						len = oldlen - newlen;
				}
			}
		}
		if (dce_fix) {
			e = der_match_tag_and_length(p, len, (Der_class) 0, (Der_type) 0, 0, &reallen, &l);
			FORW;
		}
	}
	if (size)
		*size = ret;
	return 0;
fail:
	free_NegTokenInit(data);
	return e;
}

static void
free_NegTokenInit(NegTokenInit * data)
{
	free_MechTypeList(&(data)->mechTypes);
	if ((data)->reqFlags) {
		free_ContextFlags((data)->reqFlags);
		free((data)->reqFlags);
		(data)->reqFlags = NULL;
	}
	if ((data)->mechToken) {
		free_octet_string((data)->mechToken);
		free((data)->mechToken);
		(data)->mechToken = NULL;
	}
	if ((data)->mechListMIC) {
		free_octet_string((data)->mechListMIC);
		free((data)->mechListMIC);
		(data)->mechListMIC = NULL;
	}
}

/* unused function: length_NegTokenInit */


/* unused function: copy_NegTokenInit */

/* Generated from spnego.asn1 */
/* Do not edit */


#define BACK if (e) return e; p -= l; len -= l; ret += l

static int
encode_NegTokenResp(unsigned char *p, size_t len, const NegTokenResp * data, size_t * size)
{
	size_t ret = 0;
	size_t l;
	int i, e;

	i = 0;
	if ((data)->mechListMIC) {
		int oldret = ret;
		ret = 0;
		e = encode_octet_string(p, len, (data)->mechListMIC, &l);
		BACK;
		e = der_put_length_and_tag(p, len, ret, ASN1_C_CONTEXT, CONS, 3, &l);
		BACK;
		ret += oldret;
	}
	if ((data)->responseToken) {
		int oldret = ret;
		ret = 0;
		e = encode_octet_string(p, len, (data)->responseToken, &l);
		BACK;
		e = der_put_length_and_tag(p, len, ret, ASN1_C_CONTEXT, CONS, 2, &l);
		BACK;
		ret += oldret;
	}
	if ((data)->supportedMech) {
		int oldret = ret;
		ret = 0;
		e = encode_MechType(p, len, (data)->supportedMech, &l);
		BACK;
		e = der_put_length_and_tag(p, len, ret, ASN1_C_CONTEXT, CONS, 1, &l);
		BACK;
		ret += oldret;
	}
	if ((data)->negState) {
		int oldret = ret;
		ret = 0;
		e = encode_enumerated(p, len, (data)->negState, &l);
		BACK;
		e = der_put_length_and_tag(p, len, ret, ASN1_C_CONTEXT, CONS, 0, &l);
		BACK;
		ret += oldret;
	}
	e = der_put_length_and_tag(p, len, ret, ASN1_C_UNIV, CONS, UT_Sequence, &l);
	BACK;
	*size = ret;
	return 0;
}

#define FORW if(e) goto fail; p += l; len -= l; ret += l

static int
decode_NegTokenResp(const unsigned char *p, size_t len, NegTokenResp * data, size_t * size)
{
	size_t ret = 0, reallen;
	size_t l;
	int e;

	memset(data, 0, sizeof(*data));
	reallen = 0;
	e = der_match_tag_and_length(p, len, ASN1_C_UNIV, CONS, UT_Sequence, &reallen, &l);
	FORW;
	{
		int dce_fix;
		if ((dce_fix = fix_dce(reallen, &len)) < 0)
			return ASN1_BAD_FORMAT;
		{
			size_t newlen, oldlen;

			e = der_match_tag(p, len, ASN1_C_CONTEXT, CONS, 0, &l);
			if (e)
				(data)->negState = NULL;
			else {
				p += l;
				len -= l;
				ret += l;
				e = der_get_length(p, len, &newlen, &l);
				FORW;
				{
					int dce_fix;
					oldlen = len;
					if ((dce_fix = fix_dce(newlen, &len)) < 0)
						return ASN1_BAD_FORMAT;
					(data)->negState = malloc(sizeof(*(data)->negState));
					if ((data)->negState == NULL)
						return ENOMEM;
					e = decode_enumerated(p, len, (data)->negState, &l);
					FORW;
					if (dce_fix) {
						e = der_match_tag_and_length(p, len, (Der_class) 0, (Der_type) 0, 0, &reallen, &l);
						FORW;
					} else
						len = oldlen - newlen;
				}
			}
		}
		{
			size_t newlen, oldlen;

			e = der_match_tag(p, len, ASN1_C_CONTEXT, CONS, 1, &l);
			if (e)
				(data)->supportedMech = NULL;
			else {
				p += l;
				len -= l;
				ret += l;
				e = der_get_length(p, len, &newlen, &l);
				FORW;
				{
					int dce_fix;
					oldlen = len;
					if ((dce_fix = fix_dce(newlen, &len)) < 0)
						return ASN1_BAD_FORMAT;
					(data)->supportedMech = malloc(sizeof(*(data)->supportedMech));
					if ((data)->supportedMech == NULL)
						return ENOMEM;
					e = decode_MechType(p, len, (data)->supportedMech, &l);
					FORW;
					if (dce_fix) {
						e = der_match_tag_and_length(p, len, (Der_class) 0, (Der_type) 0, 0, &reallen, &l);
						FORW;
					} else
						len = oldlen - newlen;
				}
			}
		}
		{
			size_t newlen, oldlen;

			e = der_match_tag(p, len, ASN1_C_CONTEXT, CONS, 2, &l);
			if (e)
				(data)->responseToken = NULL;
			else {
				p += l;
				len -= l;
				ret += l;
				e = der_get_length(p, len, &newlen, &l);
				FORW;
				{
					int dce_fix;
					oldlen = len;
					if ((dce_fix = fix_dce(newlen, &len)) < 0)
						return ASN1_BAD_FORMAT;
					(data)->responseToken = malloc(sizeof(*(data)->responseToken));
					if ((data)->responseToken == NULL)
						return ENOMEM;
					e = decode_octet_string(p, len, (data)->responseToken, &l);
					FORW;
					if (dce_fix) {
						e = der_match_tag_and_length(p, len, (Der_class) 0, (Der_type) 0, 0, &reallen, &l);
						FORW;
					} else
						len = oldlen - newlen;
				}
			}
		}
		{
			size_t newlen, oldlen;

			e = der_match_tag(p, len, ASN1_C_CONTEXT, CONS, 3, &l);
			if (e)
				(data)->mechListMIC = NULL;
			else {
				p += l;
				len -= l;
				ret += l;
				e = der_get_length(p, len, &newlen, &l);
				FORW;
				{
					int dce_fix;
					oldlen = len;
					if ((dce_fix = fix_dce(newlen, &len)) < 0)
						return ASN1_BAD_FORMAT;
					(data)->mechListMIC = malloc(sizeof(*(data)->mechListMIC));
					if ((data)->mechListMIC == NULL)
						return ENOMEM;
					e = decode_octet_string(p, len, (data)->mechListMIC, &l);
					FORW;
					if (dce_fix) {
						e = der_match_tag_and_length(p, len, (Der_class) 0, (Der_type) 0, 0, &reallen, &l);
						FORW;
					} else
						len = oldlen - newlen;
				}
			}
		}
		if (dce_fix) {
			e = der_match_tag_and_length(p, len, (Der_class) 0, (Der_type) 0, 0, &reallen, &l);
			FORW;
		}
	}
	if (size)
		*size = ret;
	return 0;
fail:
	free_NegTokenResp(data);
	return e;
}

static void
free_NegTokenResp(NegTokenResp * data)
{
	if ((data)->negState) {
		free((data)->negState);
		(data)->negState = NULL;
	}
	if ((data)->supportedMech) {
		free_MechType((data)->supportedMech);
		free((data)->supportedMech);
		(data)->supportedMech = NULL;
	}
	if ((data)->responseToken) {
		free_octet_string((data)->responseToken);
		free((data)->responseToken);
		(data)->responseToken = NULL;
	}
	if ((data)->mechListMIC) {
		free_octet_string((data)->mechListMIC);
		free((data)->mechListMIC);
		(data)->mechListMIC = NULL;
	}
}

/* unused function: length_NegTokenResp */


/* unused function: copy_NegTokenResp */

/* Generated from spnego.asn1 */
/* Do not edit */


/* CHOICE */
/* unused variable: asn1_NegotiationToken_dummy_holder */
