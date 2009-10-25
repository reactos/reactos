/*
 * Copyright (C) 2004-2009  Internet Systems Consortium, Inc. ("ISC")
 * Copyright (C) 1999-2003  Internet Software Consortium.
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

/* $Id: master.c,v 1.171.120.2 2009/01/18 23:47:40 tbox Exp $ */

/*! \file */

#include <config.h>

#include <isc/event.h>
#include <isc/lex.h>
#include <isc/magic.h>
#include <isc/mem.h>
#include <isc/print.h>
#include <isc/serial.h>
#include <isc/stdio.h>
#include <isc/stdtime.h>
#include <isc/string.h>
#include <isc/task.h>
#include <isc/util.h>

#include <dns/callbacks.h>
#include <dns/events.h>
#include <dns/fixedname.h>
#include <dns/master.h>
#include <dns/name.h>
#include <dns/rdata.h>
#include <dns/rdataclass.h>
#include <dns/rdatalist.h>
#include <dns/rdataset.h>
#include <dns/rdatastruct.h>
#include <dns/rdatatype.h>
#include <dns/result.h>
#include <dns/soa.h>
#include <dns/time.h>
#include <dns/ttl.h>

/*!
 * Grow the number of dns_rdatalist_t (#RDLSZ) and dns_rdata_t (#RDSZ) structures
 * by these sizes when we need to.
 *
 */
/*% RDLSZ reflects the number of different types with the same name expected. */
#define RDLSZ 32
/*%
 * RDSZ reflects the number of rdata expected at a give name that can fit into
 * 64k.
 */
#define RDSZ 512

#define NBUFS 4
#define MAXWIRESZ 255

/*%
 * Target buffer size and minimum target size.
 * MINTSIZ must be big enough to hold the largest rdata record.
 * \brief
 * TSIZ >= MINTSIZ
 */
#define TSIZ (128*1024)
/*%
 * max message size - header - root - type - class - ttl - rdlen
 */
#define MINTSIZ (65535 - 12 - 1 - 2 - 2 - 4 - 2)
/*%
 * Size for tokens in the presentation format,
 * The largest tokens are the base64 blocks in KEY and CERT records,
 * Largest key allowed is about 1372 bytes but
 * there is no fixed upper bound on CERT records.
 * 2K is too small for some X.509s, 8K is overkill.
 */
#define TOKENSIZ (8*1024)

#define DNS_MASTER_BUFSZ 2048

typedef ISC_LIST(dns_rdatalist_t) rdatalist_head_t;

typedef struct dns_incctx dns_incctx_t;

/*%
 * Master file load state.
 */

struct dns_loadctx {
	unsigned int		magic;
	isc_mem_t		*mctx;
	dns_masterformat_t	format;

	dns_rdatacallbacks_t	*callbacks;
	isc_task_t		*task;
	dns_loaddonefunc_t	done;
	void			*done_arg;

	/* Common methods */
	isc_result_t		(*openfile)(dns_loadctx_t *lctx,
					    const char *filename);
	isc_result_t		(*load)(dns_loadctx_t *lctx);

	/* Members specific to the text format: */
	isc_lex_t		*lex;
	isc_boolean_t		keep_lex;
	unsigned int		options;
	isc_boolean_t		ttl_known;
	isc_boolean_t		default_ttl_known;
	isc_boolean_t		warn_1035;
	isc_boolean_t		warn_tcr;
	isc_boolean_t		warn_sigexpired;
	isc_boolean_t		seen_include;
	isc_uint32_t		ttl;
	isc_uint32_t		default_ttl;
	dns_rdataclass_t	zclass;
	dns_fixedname_t		fixed_top;
	dns_name_t		*top;			/*%< top of zone */

	/* Members specific to the raw format: */
	FILE			*f;
	isc_boolean_t		first;

	/* Which fixed buffers we are using? */
	unsigned int		loop_cnt;		/*% records per quantum,
							 * 0 => all. */
	isc_boolean_t		canceled;
	isc_mutex_t		lock;
	isc_result_t		result;
	/* locked by lock */
	isc_uint32_t		references;
	dns_incctx_t		*inc;
	isc_uint32_t		resign;
};

struct dns_incctx {
	dns_incctx_t		*parent;
	dns_name_t		*origin;
	dns_name_t		*current;
	dns_name_t		*glue;
	dns_fixedname_t		fixed[NBUFS];		/* working buffers */
	unsigned int		in_use[NBUFS];		/* covert to bitmap? */
	int			glue_in_use;
	int			current_in_use;
	int			origin_in_use;
	isc_boolean_t		drop;
	unsigned int		glue_line;
	unsigned int		current_line;
};

#define DNS_LCTX_MAGIC ISC_MAGIC('L','c','t','x')
#define DNS_LCTX_VALID(lctx) ISC_MAGIC_VALID(lctx, DNS_LCTX_MAGIC)

#define DNS_AS_STR(t) ((t).value.as_textregion.base)

static isc_result_t
openfile_text(dns_loadctx_t *lctx, const char *master_file);

static isc_result_t
openfile_raw(dns_loadctx_t *lctx, const char *master_file);

static isc_result_t
load_text(dns_loadctx_t *lctx);

static isc_result_t
load_raw(dns_loadctx_t *lctx);

static isc_result_t
pushfile(const char *master_file, dns_name_t *origin, dns_loadctx_t *lctx);

static isc_result_t
commit(dns_rdatacallbacks_t *, dns_loadctx_t *, rdatalist_head_t *,
       dns_name_t *, const char *, unsigned int);

static isc_boolean_t
is_glue(rdatalist_head_t *, dns_name_t *);

static dns_rdatalist_t *
grow_rdatalist(int, dns_rdatalist_t *, int, rdatalist_head_t *,
		rdatalist_head_t *, isc_mem_t *mctx);

static dns_rdata_t *
grow_rdata(int, dns_rdata_t *, int, rdatalist_head_t *, rdatalist_head_t *,
	   isc_mem_t *);

static void
load_quantum(isc_task_t *task, isc_event_t *event);

static isc_result_t
task_send(dns_loadctx_t *lctx);

static void
loadctx_destroy(dns_loadctx_t *lctx);

#define GETTOKEN(lexer, options, token, eol) \
	do { \
		result = gettoken(lexer, options, token, eol, callbacks); \
		switch (result) { \
		case ISC_R_SUCCESS: \
			break; \
		case ISC_R_UNEXPECTED: \
			goto insist_and_cleanup; \
		default: \
			if (MANYERRS(lctx, result)) { \
				SETRESULT(lctx, result); \
				LOGIT(result); \
				read_till_eol = ISC_TRUE; \
				goto next_line; \
			} else \
				goto log_and_cleanup; \
		} \
		if ((token)->type == isc_tokentype_special) { \
			result = DNS_R_SYNTAX; \
			if (MANYERRS(lctx, result)) { \
				SETRESULT(lctx, result); \
				LOGIT(result); \
				read_till_eol = ISC_TRUE; \
				goto next_line; \
			} else \
				goto log_and_cleanup; \
		} \
	} while (0)

#define COMMITALL \
	do { \
		result = commit(callbacks, lctx, &current_list, \
				ictx->current, source, ictx->current_line); \
		if (MANYERRS(lctx, result)) { \
			SETRESULT(lctx, result); \
		} else if (result != ISC_R_SUCCESS) \
			goto insist_and_cleanup; \
		result = commit(callbacks, lctx, &glue_list, \
				ictx->glue, source, ictx->glue_line); \
		if (MANYERRS(lctx, result)) { \
			SETRESULT(lctx, result); \
		} else if (result != ISC_R_SUCCESS) \
			goto insist_and_cleanup; \
		rdcount = 0; \
		rdlcount = 0; \
		isc_buffer_init(&target, target_mem, target_size); \
		rdcount_save = rdcount; \
		rdlcount_save = rdlcount; \
	} while (0)

#define WARNUNEXPECTEDEOF(lexer) \
	do { \
		if (isc_lex_isfile(lexer)) \
			(*callbacks->warn)(callbacks, \
				"%s: file does not end with newline", \
				source); \
	} while (0)

#define EXPECTEOL \
	do { \
		GETTOKEN(lctx->lex, 0, &token, ISC_TRUE); \
		if (token.type != isc_tokentype_eol) { \
			isc_lex_ungettoken(lctx->lex, &token); \
			result = DNS_R_EXTRATOKEN; \
			if (MANYERRS(lctx, result)) { \
				SETRESULT(lctx, result); \
				LOGIT(result); \
				read_till_eol = ISC_TRUE; \
				continue; \
			} else if (result != ISC_R_SUCCESS) \
				goto log_and_cleanup; \
		} \
	} while (0)

#define MANYERRS(lctx, result) \
		((result != ISC_R_SUCCESS) && \
		 (result != ISC_R_IOERROR) && \
		 ((lctx)->options & DNS_MASTER_MANYERRORS) != 0)

#define SETRESULT(lctx, r) \
		do { \
			if ((lctx)->result == ISC_R_SUCCESS) \
				(lctx)->result = r; \
		} while (0)

#define LOGITFILE(result, filename) \
	if (result == ISC_R_INVALIDFILE || result == ISC_R_FILENOTFOUND || \
	    result == ISC_R_IOERROR || result == ISC_R_TOOMANYOPENFILES || \
	    result == ISC_R_NOPERM) \
		(*callbacks->error)(callbacks, "%s: %s:%lu: %s: %s", \
				    "dns_master_load", source, line, \
				    filename, dns_result_totext(result)); \
	else LOGIT(result)

#define LOGIT(result) \
	if (result == ISC_R_NOMEMORY) \
		(*callbacks->error)(callbacks, "dns_master_load: %s", \
				    dns_result_totext(result)); \
	else \
		(*callbacks->error)(callbacks, "%s: %s:%lu: %s", \
				    "dns_master_load", \
				    source, line, dns_result_totext(result))


static unsigned char in_addr_arpa_data[]  = "\007IN-ADDR\004ARPA";
static unsigned char in_addr_arpa_offsets[] = { 0, 8, 13 };
static const dns_name_t in_addr_arpa =
{
	DNS_NAME_MAGIC,
	in_addr_arpa_data, 14, 3,
	DNS_NAMEATTR_READONLY | DNS_NAMEATTR_ABSOLUTE,
	in_addr_arpa_offsets, NULL,
	{(void *)-1, (void *)-1},
	{NULL, NULL}
};

static unsigned char ip6_int_data[]  = "\003IP6\003INT";
static unsigned char ip6_int_offsets[] = { 0, 4, 8 };
static const dns_name_t ip6_int =
{
	DNS_NAME_MAGIC,
	ip6_int_data, 9, 3,
	DNS_NAMEATTR_READONLY | DNS_NAMEATTR_ABSOLUTE,
	ip6_int_offsets, NULL,
	{(void *)-1, (void *)-1},
	{NULL, NULL}
};

static unsigned char ip6_arpa_data[]  = "\003IP6\004ARPA";
static unsigned char ip6_arpa_offsets[] = { 0, 4, 9 };
static const dns_name_t ip6_arpa =
{
	DNS_NAME_MAGIC,
	ip6_arpa_data, 10, 3,
	DNS_NAMEATTR_READONLY | DNS_NAMEATTR_ABSOLUTE,
	ip6_arpa_offsets, NULL,
	{(void *)-1, (void *)-1},
	{NULL, NULL}
};


static inline isc_result_t
gettoken(isc_lex_t *lex, unsigned int options, isc_token_t *token,
	 isc_boolean_t eol, dns_rdatacallbacks_t *callbacks)
{
	isc_result_t result;

	options |= ISC_LEXOPT_EOL | ISC_LEXOPT_EOF | ISC_LEXOPT_DNSMULTILINE |
		ISC_LEXOPT_ESCAPE;
	result = isc_lex_gettoken(lex, options, token);
	if (result != ISC_R_SUCCESS) {
		switch (result) {
		case ISC_R_NOMEMORY:
			return (ISC_R_NOMEMORY);
		default:
			(*callbacks->error)(callbacks,
					    "dns_master_load: %s:%lu:"
					    " isc_lex_gettoken() failed: %s",
					    isc_lex_getsourcename(lex),
					    isc_lex_getsourceline(lex),
					    isc_result_totext(result));
			return (result);
		}
		/*NOTREACHED*/
	}
	if (eol != ISC_TRUE)
		if (token->type == isc_tokentype_eol ||
		    token->type == isc_tokentype_eof) {
			(*callbacks->error)(callbacks,
			    "dns_master_load: %s:%lu: unexpected end of %s",
					    isc_lex_getsourcename(lex),
					    isc_lex_getsourceline(lex),
					    (token->type ==
					     isc_tokentype_eol) ?
					    "line" : "file");
			return (ISC_R_UNEXPECTEDEND);
		}
	return (ISC_R_SUCCESS);
}


void
dns_loadctx_attach(dns_loadctx_t *source, dns_loadctx_t **target) {

	REQUIRE(target != NULL && *target == NULL);
	REQUIRE(DNS_LCTX_VALID(source));

	LOCK(&source->lock);
	INSIST(source->references > 0);
	source->references++;
	INSIST(source->references != 0);	/* Overflow? */
	UNLOCK(&source->lock);

	*target = source;
}

void
dns_loadctx_detach(dns_loadctx_t **lctxp) {
	dns_loadctx_t *lctx;
	isc_boolean_t need_destroy = ISC_FALSE;

	REQUIRE(lctxp != NULL);
	lctx = *lctxp;
	REQUIRE(DNS_LCTX_VALID(lctx));

	LOCK(&lctx->lock);
	INSIST(lctx->references > 0);
	lctx->references--;
	if (lctx->references == 0)
		need_destroy = ISC_TRUE;
	UNLOCK(&lctx->lock);

	if (need_destroy)
		loadctx_destroy(lctx);
	*lctxp = NULL;
}

static void
incctx_destroy(isc_mem_t *mctx, dns_incctx_t *ictx) {
	dns_incctx_t *parent;

 again:
	parent = ictx->parent;
	ictx->parent = NULL;

	isc_mem_put(mctx, ictx, sizeof(*ictx));

	if (parent != NULL) {
		ictx = parent;
		goto again;
	}
}

static void
loadctx_destroy(dns_loadctx_t *lctx) {
	isc_mem_t *mctx;
	isc_result_t result;

	REQUIRE(DNS_LCTX_VALID(lctx));

	lctx->magic = 0;
	if (lctx->inc != NULL)
		incctx_destroy(lctx->mctx, lctx->inc);

	if (lctx->f != NULL) {
		result = isc_stdio_close(lctx->f);
		if (result != ISC_R_SUCCESS) {
			UNEXPECTED_ERROR(__FILE__, __LINE__,
					 "isc_stdio_close() failed: %s",
					 isc_result_totext(result));
		}
	}

	/* isc_lex_destroy() will close all open streams */
	if (lctx->lex != NULL && !lctx->keep_lex)
		isc_lex_destroy(&lctx->lex);

	if (lctx->task != NULL)
		isc_task_detach(&lctx->task);
	DESTROYLOCK(&lctx->lock);
	mctx = NULL;
	isc_mem_attach(lctx->mctx, &mctx);
	isc_mem_detach(&lctx->mctx);
	isc_mem_put(mctx, lctx, sizeof(*lctx));
	isc_mem_detach(&mctx);
}

static isc_result_t
incctx_create(isc_mem_t *mctx, dns_name_t *origin, dns_incctx_t **ictxp) {
	dns_incctx_t *ictx;
	isc_region_t r;
	int i;

	ictx = isc_mem_get(mctx, sizeof(*ictx));
	if (ictx == NULL)
		return (ISC_R_NOMEMORY);

	for (i = 0; i < NBUFS; i++) {
		dns_fixedname_init(&ictx->fixed[i]);
		ictx->in_use[i] = ISC_FALSE;
	}

	ictx->origin_in_use = 0;
	ictx->origin = dns_fixedname_name(&ictx->fixed[ictx->origin_in_use]);
	ictx->in_use[ictx->origin_in_use] = ISC_TRUE;
	dns_name_toregion(origin, &r);
	dns_name_fromregion(ictx->origin, &r);

	ictx->glue = NULL;
	ictx->current = NULL;
	ictx->glue_in_use = -1;
	ictx->current_in_use = -1;
	ictx->parent = NULL;
	ictx->drop = ISC_FALSE;
	ictx->glue_line = 0;
	ictx->current_line = 0;

	*ictxp = ictx;
	return (ISC_R_SUCCESS);
}

static isc_result_t
loadctx_create(dns_masterformat_t format, isc_mem_t *mctx,
	       unsigned int options, isc_uint32_t resign, dns_name_t *top,
	       dns_rdataclass_t zclass, dns_name_t *origin,
	       dns_rdatacallbacks_t *callbacks, isc_task_t *task,
	       dns_loaddonefunc_t done, void *done_arg, isc_lex_t *lex,
	       dns_loadctx_t **lctxp)
{
	dns_loadctx_t *lctx;
	isc_result_t result;
	isc_region_t r;
	isc_lexspecials_t specials;

	REQUIRE(lctxp != NULL && *lctxp == NULL);
	REQUIRE(callbacks != NULL);
	REQUIRE(callbacks->add != NULL);
	REQUIRE(callbacks->error != NULL);
	REQUIRE(callbacks->warn != NULL);
	REQUIRE(mctx != NULL);
	REQUIRE(dns_name_isabsolute(top));
	REQUIRE(dns_name_isabsolute(origin));
	REQUIRE((task == NULL && done == NULL) ||
		(task != NULL && done != NULL));

	lctx = isc_mem_get(mctx, sizeof(*lctx));
	if (lctx == NULL)
		return (ISC_R_NOMEMORY);
	result = isc_mutex_init(&lctx->lock);
	if (result != ISC_R_SUCCESS) {
		isc_mem_put(mctx, lctx, sizeof(*lctx));
		return (result);
	}

	lctx->inc = NULL;
	result = incctx_create(mctx, origin, &lctx->inc);
	if (result != ISC_R_SUCCESS)
		goto cleanup_ctx;

	lctx->format = format;
	switch (format) {
	default:
		INSIST(0);
	case dns_masterformat_text:
		lctx->openfile = openfile_text;
		lctx->load = load_text;
		break;
	case dns_masterformat_raw:
		lctx->openfile = openfile_raw;
		lctx->load = load_raw;
		break;
	}

	if (lex != NULL) {
		lctx->lex = lex;
		lctx->keep_lex = ISC_TRUE;
	} else {
		lctx->lex = NULL;
		result = isc_lex_create(mctx, TOKENSIZ, &lctx->lex);
		if (result != ISC_R_SUCCESS)
			goto cleanup_inc;
		lctx->keep_lex = ISC_FALSE;
		memset(specials, 0, sizeof(specials));
		specials['('] = 1;
		specials[')'] = 1;
		specials['"'] = 1;
		isc_lex_setspecials(lctx->lex, specials);
		isc_lex_setcomments(lctx->lex, ISC_LEXCOMMENT_DNSMASTERFILE);
	}

	lctx->ttl_known = ISC_FALSE;
	lctx->ttl = 0;
	lctx->default_ttl_known = ISC_FALSE;
	lctx->default_ttl = 0;
	lctx->warn_1035 = ISC_TRUE;	/* XXX Argument? */
	lctx->warn_tcr = ISC_TRUE;	/* XXX Argument? */
	lctx->warn_sigexpired = ISC_TRUE;	/* XXX Argument? */
	lctx->options = options;
	lctx->seen_include = ISC_FALSE;
	lctx->zclass = zclass;
	lctx->resign = resign;
	lctx->result = ISC_R_SUCCESS;

	dns_fixedname_init(&lctx->fixed_top);
	lctx->top = dns_fixedname_name(&lctx->fixed_top);
	dns_name_toregion(top, &r);
	dns_name_fromregion(lctx->top, &r);

	lctx->f = NULL;
	lctx->first = ISC_TRUE;

	lctx->loop_cnt = (done != NULL) ? 100 : 0;
	lctx->callbacks = callbacks;
	lctx->task = NULL;
	if (task != NULL)
		isc_task_attach(task, &lctx->task);
	lctx->done = done;
	lctx->done_arg = done_arg;
	lctx->canceled = ISC_FALSE;
	lctx->mctx = NULL;
	isc_mem_attach(mctx, &lctx->mctx);
	lctx->references = 1;			/* Implicit attach. */
	lctx->magic = DNS_LCTX_MAGIC;
	*lctxp = lctx;
	return (ISC_R_SUCCESS);

 cleanup_inc:
	incctx_destroy(mctx, lctx->inc);
 cleanup_ctx:
	isc_mem_put(mctx, lctx, sizeof(*lctx));
	return (result);
}

static isc_result_t
genname(char *name, int it, char *buffer, size_t length) {
	char fmt[sizeof("%04000000000d")];
	char numbuf[128];
	char *cp;
	char mode[2];
	int delta = 0;
	isc_textregion_t r;
	unsigned int n;
	unsigned int width;

	r.base = buffer;
	r.length = length;

	while (*name != '\0') {
		if (*name == '$') {
			name++;
			if (*name == '$') {
				if (r.length == 0)
					return (ISC_R_NOSPACE);
				r.base[0] = *name++;
				isc_textregion_consume(&r, 1);
				continue;
			}
			strcpy(fmt, "%d");
			/* Get format specifier. */
			if (*name == '{' ) {
				n = sscanf(name, "{%d,%u,%1[doxX]}",
					   &delta, &width, mode);
				switch (n) {
				case 1:
					break;
				case 2:
					n = snprintf(fmt, sizeof(fmt),
						     "%%0%ud", width);
					break;
				case 3:
					n = snprintf(fmt, sizeof(fmt),
						     "%%0%u%c", width, mode[0]);
					break;
				default:
					return (DNS_R_SYNTAX);
				}
				if (n >= sizeof(fmt))
					return (ISC_R_NOSPACE);
				/* Skip past closing brace. */
				while (*name != '\0' && *name++ != '}')
					continue;
			}
			n = snprintf(numbuf, sizeof(numbuf), fmt, it + delta);
			if (n >= sizeof(numbuf))
				return (ISC_R_NOSPACE);
			cp = numbuf;
			while (*cp != '\0') {
				if (r.length == 0)
					return (ISC_R_NOSPACE);
				r.base[0] = *cp++;
				isc_textregion_consume(&r, 1);
			}
		} else if (*name == '\\') {
			if (r.length == 0)
				return (ISC_R_NOSPACE);
			r.base[0] = *name++;
			isc_textregion_consume(&r, 1);
			if (*name == '\0')
				continue;
			if (r.length == 0)
				return (ISC_R_NOSPACE);
			r.base[0] = *name++;
			isc_textregion_consume(&r, 1);
		} else {
			if (r.length == 0)
				return (ISC_R_NOSPACE);
			r.base[0] = *name++;
			isc_textregion_consume(&r, 1);
		}
	}
	if (r.length == 0)
		return (ISC_R_NOSPACE);
	r.base[0] = '\0';
	return (ISC_R_SUCCESS);
}

static isc_result_t
openfile_text(dns_loadctx_t *lctx, const char *master_file) {
	return (isc_lex_openfile(lctx->lex, master_file));
}

static isc_result_t
openfile_raw(dns_loadctx_t *lctx, const char *master_file) {
	isc_result_t result;

	result = isc_stdio_open(master_file, "r", &lctx->f);
	if (result != ISC_R_SUCCESS && result != ISC_R_FILENOTFOUND) {
		UNEXPECTED_ERROR(__FILE__, __LINE__,
				 "isc_stdio_open() failed: %s",
				 isc_result_totext(result));
	}

	return (result);
}

static isc_result_t
generate(dns_loadctx_t *lctx, char *range, char *lhs, char *gtype, char *rhs,
	 const char *source, unsigned int line)
{
	char *target_mem = NULL;
	char *lhsbuf = NULL;
	char *rhsbuf = NULL;
	dns_fixedname_t ownerfixed;
	dns_name_t *owner;
	dns_rdata_t rdata = DNS_RDATA_INIT;
	dns_rdatacallbacks_t *callbacks;
	dns_rdatalist_t rdatalist;
	dns_rdatatype_t type;
	rdatalist_head_t head;
	int n;
	int target_size = MINTSIZ;	/* only one rdata at a time */
	isc_buffer_t buffer;
	isc_buffer_t target;
	isc_result_t result;
	isc_textregion_t r;
	unsigned int start, stop, step, i;
	dns_incctx_t *ictx;

	ictx = lctx->inc;
	callbacks = lctx->callbacks;
	dns_fixedname_init(&ownerfixed);
	owner = dns_fixedname_name(&ownerfixed);
	ISC_LIST_INIT(head);

	target_mem = isc_mem_get(lctx->mctx, target_size);
	rhsbuf = isc_mem_get(lctx->mctx, DNS_MASTER_BUFSZ);
	lhsbuf = isc_mem_get(lctx->mctx, DNS_MASTER_BUFSZ);
	if (target_mem == NULL || rhsbuf == NULL || lhsbuf == NULL) {
		result = ISC_R_NOMEMORY;
		goto error_cleanup;
	}
	isc_buffer_init(&target, target_mem, target_size);

	n = sscanf(range, "%u-%u/%u", &start, &stop, &step);
	if (n < 2 || stop < start) {
	       (*callbacks->error)(callbacks,
				  "%s: %s:%lu: invalid range '%s'",
				  "$GENERATE", source, line, range);
		result = DNS_R_SYNTAX;
		goto insist_cleanup;
	}
	if (n == 2)
		step = 1;

	/*
	 * Get type.
	 */
	r.base = gtype;
	r.length = strlen(gtype);
	result = dns_rdatatype_fromtext(&type, &r);
	if (result != ISC_R_SUCCESS) {
		(*callbacks->error)(callbacks,
				   "%s: %s:%lu: unknown RR type '%s'",
				   "$GENERATE", source, line, gtype);
		goto insist_cleanup;
	}

	switch (type) {
	case dns_rdatatype_ns:
	case dns_rdatatype_ptr:
	case dns_rdatatype_cname:
	case dns_rdatatype_dname:
		break;

	case dns_rdatatype_a:
	case dns_rdatatype_aaaa:
		if (lctx->zclass == dns_rdataclass_in ||
		    lctx->zclass == dns_rdataclass_ch ||
		    lctx->zclass == dns_rdataclass_hs)
			break;
		/* FALLTHROUGH */
	default:
	       (*callbacks->error)(callbacks,
				  "%s: %s:%lu: unsupported type '%s'",
				  "$GENERATE", source, line, gtype);
		result = ISC_R_NOTIMPLEMENTED;
		goto error_cleanup;
	}

	ISC_LIST_INIT(rdatalist.rdata);
	ISC_LINK_INIT(&rdatalist, link);
	for (i = start; i <= stop; i += step) {
		result = genname(lhs, i, lhsbuf, DNS_MASTER_BUFSZ);
		if (result != ISC_R_SUCCESS)
			goto error_cleanup;
		result = genname(rhs, i, rhsbuf, DNS_MASTER_BUFSZ);
		if (result != ISC_R_SUCCESS)
			goto error_cleanup;

		isc_buffer_init(&buffer, lhsbuf, strlen(lhsbuf));
		isc_buffer_add(&buffer, strlen(lhsbuf));
		isc_buffer_setactive(&buffer, strlen(lhsbuf));
		result = dns_name_fromtext(owner, &buffer, ictx->origin,
					   0, NULL);
		if (result != ISC_R_SUCCESS)
			goto error_cleanup;

		if ((lctx->options & DNS_MASTER_ZONE) != 0 &&
		    (lctx->options & DNS_MASTER_SLAVE) == 0 &&
		    !dns_name_issubdomain(owner, lctx->top))
		{
			char namebuf[DNS_NAME_FORMATSIZE];
			dns_name_format(owner, namebuf, sizeof(namebuf));
			/*
			 * Ignore out-of-zone data.
			 */
			(*callbacks->warn)(callbacks,
					   "%s:%lu: "
					   "ignoring out-of-zone data (%s)",
					   source, line, namebuf);
			continue;
		}

		isc_buffer_init(&buffer, rhsbuf, strlen(rhsbuf));
		isc_buffer_add(&buffer, strlen(rhsbuf));
		isc_buffer_setactive(&buffer, strlen(rhsbuf));

		result = isc_lex_openbuffer(lctx->lex, &buffer);
		if (result != ISC_R_SUCCESS)
			goto error_cleanup;

		isc_buffer_init(&target, target_mem, target_size);
		result = dns_rdata_fromtext(&rdata, lctx->zclass, type,
					    lctx->lex, ictx->origin, 0,
					    lctx->mctx, &target, callbacks);
		RUNTIME_CHECK(isc_lex_close(lctx->lex) == ISC_R_SUCCESS);
		if (result != ISC_R_SUCCESS)
			goto error_cleanup;

		rdatalist.type = type;
		rdatalist.covers = 0;
		rdatalist.rdclass = lctx->zclass;
		rdatalist.ttl = lctx->ttl;
		ISC_LIST_PREPEND(head, &rdatalist, link);
		ISC_LIST_APPEND(rdatalist.rdata, &rdata, link);
		result = commit(callbacks, lctx, &head, owner, source, line);
		ISC_LIST_UNLINK(rdatalist.rdata, &rdata, link);
		if (result != ISC_R_SUCCESS)
			goto error_cleanup;
		dns_rdata_reset(&rdata);
	}
	result = ISC_R_SUCCESS;
	goto cleanup;

 error_cleanup:
	if (result == ISC_R_NOMEMORY)
		(*callbacks->error)(callbacks, "$GENERATE: %s",
				    dns_result_totext(result));
	else
		(*callbacks->error)(callbacks, "$GENERATE: %s:%lu: %s",
				    source, line, dns_result_totext(result));

 insist_cleanup:
	INSIST(result != ISC_R_SUCCESS);

 cleanup:
	if (target_mem != NULL)
		isc_mem_put(lctx->mctx, target_mem, target_size);
	if (lhsbuf != NULL)
		isc_mem_put(lctx->mctx, lhsbuf, DNS_MASTER_BUFSZ);
	if (rhsbuf != NULL)
		isc_mem_put(lctx->mctx, rhsbuf, DNS_MASTER_BUFSZ);
	return (result);
}

static void
limit_ttl(dns_rdatacallbacks_t *callbacks, const char *source, unsigned int line,
	  isc_uint32_t *ttlp)
{
	if (*ttlp > 0x7fffffffUL) {
		(callbacks->warn)(callbacks,
				  "%s: %s:%lu: "
				  "$TTL %lu > MAXTTL, "
				  "setting $TTL to 0",
				  "dns_master_load",
				  source, line,
				  *ttlp);
		*ttlp = 0;
	}
}

static isc_result_t
check_ns(dns_loadctx_t *lctx, isc_token_t *token, const char *source,
	 unsigned long line)
{
	char *tmp = NULL;
	isc_result_t result = ISC_R_SUCCESS;
	void (*callback)(struct dns_rdatacallbacks *, const char *, ...);

	if ((lctx->options & DNS_MASTER_FATALNS) != 0)
		callback = lctx->callbacks->error;
	else
		callback = lctx->callbacks->warn;

	if (token->type == isc_tokentype_string) {
		struct in_addr addr;
		struct in6_addr addr6;

		tmp = isc_mem_strdup(lctx->mctx, DNS_AS_STR(*token));
		if (tmp == NULL)
			return (ISC_R_NOMEMORY);
		/*
		 * Catch both "1.2.3.4" and "1.2.3.4."
		 */
		if (tmp[strlen(tmp) - 1] == '.')
			tmp[strlen(tmp) - 1] = '\0';
		if (inet_aton(tmp, &addr) == 1 ||
		    inet_pton(AF_INET6, tmp, &addr6) == 1)
			result = DNS_R_NSISADDRESS;
	}
	if (result != ISC_R_SUCCESS)
		(*callback)(lctx->callbacks, "%s:%lu: NS record '%s' "
			    "appears to be an address",
			    source, line, DNS_AS_STR(*token));
	if (tmp != NULL)
		isc_mem_free(lctx->mctx, tmp);
	return (result);
}

static void
check_wildcard(dns_incctx_t *ictx, const char *source, unsigned long line,
	       dns_rdatacallbacks_t *callbacks)
{
	dns_name_t *name;

	name = (ictx->glue != NULL) ? ictx->glue : ictx->current;
	if (dns_name_internalwildcard(name)) {
		char namebuf[DNS_NAME_FORMATSIZE];

		dns_name_format(name, namebuf, sizeof(namebuf));
		(*callbacks->warn)(callbacks, "%s:%lu: warning: ownername "
				   "'%s' contains an non-terminal wildcard",
				   source, line, namebuf);
	}
}

static isc_result_t
load_text(dns_loadctx_t *lctx) {
	dns_rdataclass_t rdclass;
	dns_rdatatype_t type, covers;
	isc_uint32_t ttl_offset = 0;
	dns_name_t *new_name;
	isc_boolean_t current_has_delegation = ISC_FALSE;
	isc_boolean_t done = ISC_FALSE;
	isc_boolean_t finish_origin = ISC_FALSE;
	isc_boolean_t finish_include = ISC_FALSE;
	isc_boolean_t read_till_eol = ISC_FALSE;
	isc_boolean_t initialws;
	char *include_file = NULL;
	isc_token_t token;
	isc_result_t result = ISC_R_UNEXPECTED;
	rdatalist_head_t glue_list;
	rdatalist_head_t current_list;
	dns_rdatalist_t *this;
	dns_rdatalist_t *rdatalist = NULL;
	dns_rdatalist_t *new_rdatalist;
	int rdlcount = 0;
	int rdlcount_save = 0;
	int rdatalist_size = 0;
	isc_buffer_t buffer;
	isc_buffer_t target;
	isc_buffer_t target_ft;
	isc_buffer_t target_save;
	dns_rdata_t *rdata = NULL;
	dns_rdata_t *new_rdata;
	int rdcount = 0;
	int rdcount_save = 0;
	int rdata_size = 0;
	unsigned char *target_mem = NULL;
	int target_size = TSIZ;
	int new_in_use;
	unsigned int loop_cnt = 0;
	isc_mem_t *mctx;
	dns_rdatacallbacks_t *callbacks;
	dns_incctx_t *ictx;
	char *range = NULL;
	char *lhs = NULL;
	char *gtype = NULL;
	char *rhs = NULL;
	const char *source = "";
	unsigned long line = 0;
	isc_boolean_t explicit_ttl;
	isc_stdtime_t now;
	char classname1[DNS_RDATACLASS_FORMATSIZE];
	char classname2[DNS_RDATACLASS_FORMATSIZE];
	unsigned int options = 0;

	REQUIRE(DNS_LCTX_VALID(lctx));
	callbacks = lctx->callbacks;
	mctx = lctx->mctx;
	ictx = lctx->inc;

	ISC_LIST_INIT(glue_list);
	ISC_LIST_INIT(current_list);

	isc_stdtime_get(&now);

	/*
	 * Allocate target_size of buffer space.  This is greater than twice
	 * the maximum individual RR data size.
	 */
	target_mem = isc_mem_get(mctx, target_size);
	if (target_mem == NULL) {
		result = ISC_R_NOMEMORY;
		goto log_and_cleanup;
	}
	isc_buffer_init(&target, target_mem, target_size);
	target_save = target;

	if ((lctx->options & DNS_MASTER_CHECKNAMES) != 0)
		options |= DNS_RDATA_CHECKNAMES;
	if ((lctx->options & DNS_MASTER_CHECKNAMESFAIL) != 0)
		options |= DNS_RDATA_CHECKNAMESFAIL;
	if ((lctx->options & DNS_MASTER_CHECKMX) != 0)
		options |= DNS_RDATA_CHECKMX;
	if ((lctx->options & DNS_MASTER_CHECKMXFAIL) != 0)
		options |= DNS_RDATA_CHECKMXFAIL;
	source = isc_lex_getsourcename(lctx->lex);
	do {
		initialws = ISC_FALSE;
		line = isc_lex_getsourceline(lctx->lex);
		GETTOKEN(lctx->lex, ISC_LEXOPT_INITIALWS | ISC_LEXOPT_QSTRING,
			 &token, ISC_TRUE);
		line = isc_lex_getsourceline(lctx->lex);

		if (token.type == isc_tokentype_eof) {
			if (read_till_eol)
				WARNUNEXPECTEDEOF(lctx->lex);
			/* Pop the include stack? */
			if (ictx->parent != NULL) {
				COMMITALL;
				lctx->inc = ictx->parent;
				ictx->parent = NULL;
				incctx_destroy(lctx->mctx, ictx);
				RUNTIME_CHECK(isc_lex_close(lctx->lex) == ISC_R_SUCCESS);
				line = isc_lex_getsourceline(lctx->lex);
				source = isc_lex_getsourcename(lctx->lex);
				ictx = lctx->inc;
				EXPECTEOL;
				continue;
			}
			done = ISC_TRUE;
			continue;
		}

		if (token.type == isc_tokentype_eol) {
			read_till_eol = ISC_FALSE;
			continue;		/* blank line */
		}

		if (read_till_eol)
			continue;

		if (token.type == isc_tokentype_initialws) {
			/*
			 * Still working on the same name.
			 */
			initialws = ISC_TRUE;
		} else if (token.type == isc_tokentype_string ||
			   token.type == isc_tokentype_qstring) {

			/*
			 * "$" Support.
			 *
			 * "$ORIGIN" and "$INCLUDE" can both take domain names.
			 * The processing of "$ORIGIN" and "$INCLUDE" extends
			 * across the normal domain name processing.
			 */

			if (strcasecmp(DNS_AS_STR(token), "$ORIGIN") == 0) {
				GETTOKEN(lctx->lex, 0, &token, ISC_FALSE);
				finish_origin = ISC_TRUE;
			} else if (strcasecmp(DNS_AS_STR(token),
					      "$TTL") == 0) {
				GETTOKEN(lctx->lex, 0, &token, ISC_FALSE);
				result =
				   dns_ttl_fromtext(&token.value.as_textregion,
						    &lctx->ttl);
				if (MANYERRS(lctx, result)) {
					SETRESULT(lctx, result);
					lctx->ttl = 0;
				} else if (result != ISC_R_SUCCESS)
					goto insist_and_cleanup;
				limit_ttl(callbacks, source, line, &lctx->ttl);
				lctx->default_ttl = lctx->ttl;
				lctx->default_ttl_known = ISC_TRUE;
				EXPECTEOL;
				continue;
			} else if (strcasecmp(DNS_AS_STR(token),
					      "$INCLUDE") == 0) {
				COMMITALL;
				if ((lctx->options & DNS_MASTER_NOINCLUDE)
				    != 0)
				{
					(callbacks->error)(callbacks,
					   "%s: %s:%lu: $INCLUDE not allowed",
					   "dns_master_load",
					   source, line);
					result = DNS_R_REFUSED;
					goto insist_and_cleanup;
				}
				if (ttl_offset != 0) {
					(callbacks->error)(callbacks,
					   "%s: %s:%lu: $INCLUDE "
					   "may not be used with $DATE",
					   "dns_master_load",
					   source, line);
					result = DNS_R_SYNTAX;
					goto insist_and_cleanup;
				}
				GETTOKEN(lctx->lex, ISC_LEXOPT_QSTRING, &token,
					 ISC_FALSE);
				if (include_file != NULL)
					isc_mem_free(mctx, include_file);
				include_file = isc_mem_strdup(mctx,
							   DNS_AS_STR(token));
				if (include_file == NULL) {
					result = ISC_R_NOMEMORY;
					goto log_and_cleanup;
				}
				GETTOKEN(lctx->lex, 0, &token, ISC_TRUE);

				if (token.type == isc_tokentype_eol ||
				    token.type == isc_tokentype_eof) {
					if (token.type == isc_tokentype_eof)
						WARNUNEXPECTEDEOF(lctx->lex);
					isc_lex_ungettoken(lctx->lex, &token);
					/*
					 * No origin field.
					 */
					result = pushfile(include_file,
							  ictx->origin, lctx);
					if (MANYERRS(lctx, result)) {
						SETRESULT(lctx, result);
						LOGITFILE(result, include_file);
						continue;
					} else if (result != ISC_R_SUCCESS) {
						LOGITFILE(result, include_file);
						goto insist_and_cleanup;
					}
					ictx = lctx->inc;
					line = isc_lex_getsourceline(lctx->lex);
					source =
					       isc_lex_getsourcename(lctx->lex);
					continue;
				}
				/*
				 * There is an origin field.  Fall through
				 * to domain name processing code and do
				 * the actual inclusion later.
				 */
				finish_include = ISC_TRUE;
			} else if (strcasecmp(DNS_AS_STR(token),
					      "$DATE") == 0) {
				isc_int64_t dump_time64;
				isc_stdtime_t dump_time, current_time;
				GETTOKEN(lctx->lex, 0, &token, ISC_FALSE);
				isc_stdtime_get(&current_time);
				result = dns_time64_fromtext(DNS_AS_STR(token),
							     &dump_time64);
				if (MANYERRS(lctx, result)) {
					SETRESULT(lctx, result);
					LOGIT(result);
					dump_time64 = 0;
				} else if (result != ISC_R_SUCCESS)
					goto log_and_cleanup;
				dump_time = (isc_stdtime_t)dump_time64;
				if (dump_time != dump_time64) {
					UNEXPECTED_ERROR(__FILE__, __LINE__,
					 "%s: %s:%lu: $DATE outside epoch",
					 "dns_master_load", source, line);
					result = ISC_R_UNEXPECTED;
					goto insist_and_cleanup;
				}
				if (dump_time > current_time) {
					UNEXPECTED_ERROR(__FILE__, __LINE__,
					"%s: %s:%lu: "
					"$DATE in future, using current date",
					"dns_master_load", source, line);
					dump_time = current_time;
				}
				ttl_offset = current_time - dump_time;
				EXPECTEOL;
				continue;
			} else if (strcasecmp(DNS_AS_STR(token),
					      "$GENERATE") == 0) {
				/*
				 * Lazy cleanup.
				 */
				if (range != NULL)
					isc_mem_free(mctx, range);
				if (lhs != NULL)
					isc_mem_free(mctx, lhs);
				if (gtype != NULL)
					isc_mem_free(mctx, gtype);
				if (rhs != NULL)
					isc_mem_free(mctx, rhs);
				range = lhs = gtype = rhs = NULL;
				/* RANGE */
				GETTOKEN(lctx->lex, 0, &token, ISC_FALSE);
				range = isc_mem_strdup(mctx,
						     DNS_AS_STR(token));
				if (range == NULL) {
					result = ISC_R_NOMEMORY;
					goto log_and_cleanup;
				}
				/* LHS */
				GETTOKEN(lctx->lex, 0, &token, ISC_FALSE);
				lhs = isc_mem_strdup(mctx, DNS_AS_STR(token));
				if (lhs == NULL) {
					result = ISC_R_NOMEMORY;
					goto log_and_cleanup;
				}
				rdclass = 0;
				explicit_ttl = ISC_FALSE;
				/* CLASS? */
				GETTOKEN(lctx->lex, 0, &token, ISC_FALSE);
				if (dns_rdataclass_fromtext(&rdclass,
					    &token.value.as_textregion)
						== ISC_R_SUCCESS) {
					GETTOKEN(lctx->lex, 0, &token,
						 ISC_FALSE);
				}
				/* TTL? */
				if (dns_ttl_fromtext(&token.value.as_textregion,
						     &lctx->ttl)
						== ISC_R_SUCCESS) {
					limit_ttl(callbacks, source, line,
						  &lctx->ttl);
					lctx->ttl_known = ISC_TRUE;
					explicit_ttl = ISC_TRUE;
					GETTOKEN(lctx->lex, 0, &token,
						 ISC_FALSE);
				}
				/* CLASS? */
				if (rdclass == 0 &&
				    dns_rdataclass_fromtext(&rdclass,
						    &token.value.as_textregion)
						== ISC_R_SUCCESS)
					GETTOKEN(lctx->lex, 0, &token,
						 ISC_FALSE);
				/* TYPE */
				gtype = isc_mem_strdup(mctx,
						       DNS_AS_STR(token));
				if (gtype == NULL) {
					result = ISC_R_NOMEMORY;
					goto log_and_cleanup;
				}
				/* RHS */
				GETTOKEN(lctx->lex, 0, &token, ISC_FALSE);
				rhs = isc_mem_strdup(mctx, DNS_AS_STR(token));
				if (rhs == NULL) {
					result = ISC_R_NOMEMORY;
					goto log_and_cleanup;
				}
				if (!lctx->ttl_known &&
				    !lctx->default_ttl_known) {
					(*callbacks->error)(callbacks,
					    "%s: %s:%lu: no TTL specified",
					    "dns_master_load", source, line);
					result = DNS_R_NOTTL;
					if (MANYERRS(lctx, result)) {
						SETRESULT(lctx, result);
						lctx->ttl = 0;
					} else if (result != ISC_R_SUCCESS)
						goto insist_and_cleanup;
				} else if (!explicit_ttl &&
					   lctx->default_ttl_known) {
					lctx->ttl = lctx->default_ttl;
				}
				/*
				 * If the class specified does not match the
				 * zone's class print out a error message and
				 * exit.
				 */
				if (rdclass != 0 && rdclass != lctx->zclass) {
					goto bad_class;
				}
				result = generate(lctx, range, lhs, gtype, rhs,
						  source, line);
				if (MANYERRS(lctx, result)) {
					SETRESULT(lctx, result);
				} else if (result != ISC_R_SUCCESS)
					goto insist_and_cleanup;
				EXPECTEOL;
				continue;
			} else if (strncasecmp(DNS_AS_STR(token),
					       "$", 1) == 0) {
				(callbacks->error)(callbacks,
					   "%s: %s:%lu: "
					   "unknown $ directive '%s'",
					   "dns_master_load", source, line,
					   DNS_AS_STR(token));
				result = DNS_R_SYNTAX;
				if (MANYERRS(lctx, result)) {
					SETRESULT(lctx, result);
				} else if (result != ISC_R_SUCCESS)
					goto insist_and_cleanup;
			}

			/*
			 * Normal processing resumes.
			 *
			 * Find a free name buffer.
			 */
			for (new_in_use = 0; new_in_use < NBUFS; new_in_use++)
				if (!ictx->in_use[new_in_use])
					break;
			INSIST(new_in_use < NBUFS);
			dns_fixedname_init(&ictx->fixed[new_in_use]);
			new_name = dns_fixedname_name(&ictx->fixed[new_in_use]);
			isc_buffer_init(&buffer, token.value.as_region.base,
					token.value.as_region.length);
			isc_buffer_add(&buffer, token.value.as_region.length);
			isc_buffer_setactive(&buffer,
					     token.value.as_region.length);
			result = dns_name_fromtext(new_name, &buffer,
					  ictx->origin, ISC_FALSE, NULL);
			if (MANYERRS(lctx, result)) {
				SETRESULT(lctx, result);
				LOGIT(result);
				read_till_eol = ISC_TRUE;
				continue;
			} else if (result != ISC_R_SUCCESS)
				goto log_and_cleanup;

			/*
			 * Finish $ORIGIN / $INCLUDE processing if required.
			 */
			if (finish_origin) {
				if (ictx->origin_in_use != -1)
					ictx->in_use[ictx->origin_in_use] =
						ISC_FALSE;
				ictx->origin_in_use = new_in_use;
				ictx->in_use[ictx->origin_in_use] = ISC_TRUE;
				ictx->origin = new_name;
				finish_origin = ISC_FALSE;
				EXPECTEOL;
				continue;
			}
			if (finish_include) {
				finish_include = ISC_FALSE;
				result = pushfile(include_file, new_name, lctx);
				if (MANYERRS(lctx, result)) {
					SETRESULT(lctx, result);
					LOGITFILE(result, include_file);
					continue;
				} else if (result != ISC_R_SUCCESS) {
					LOGITFILE(result, include_file);
					goto insist_and_cleanup;
				}
				ictx = lctx->inc;
				line = isc_lex_getsourceline(lctx->lex);
				source = isc_lex_getsourcename(lctx->lex);
				continue;
			}

			/*
			 * "$" Processing Finished
			 */

			/*
			 * If we are processing glue and the new name does
			 * not match the current glue name, commit the glue
			 * and pop stacks leaving us in 'normal' processing
			 * state.  Linked lists are undone by commit().
			 */
			if (ictx->glue != NULL &&
			    dns_name_compare(ictx->glue, new_name) != 0) {
				result = commit(callbacks, lctx, &glue_list,
						ictx->glue, source,
						ictx->glue_line);
				if (MANYERRS(lctx, result)) {
					SETRESULT(lctx, result);
				} else if (result != ISC_R_SUCCESS)
					goto insist_and_cleanup;
				if (ictx->glue_in_use != -1)
					ictx->in_use[ictx->glue_in_use] =
						ISC_FALSE;
				ictx->glue_in_use = -1;
				ictx->glue = NULL;
				rdcount = rdcount_save;
				rdlcount = rdlcount_save;
				target = target_save;
			}

			/*
			 * If we are in 'normal' processing state and the new
			 * name does not match the current name, see if the
			 * new name is for glue and treat it as such,
			 * otherwise we have a new name so commit what we
			 * have.
			 */
			if ((ictx->glue == NULL) && (ictx->current == NULL ||
			    dns_name_compare(ictx->current, new_name) != 0)) {
				if (current_has_delegation &&
					is_glue(&current_list, new_name)) {
					rdcount_save = rdcount;
					rdlcount_save = rdlcount;
					target_save = target;
					ictx->glue = new_name;
					ictx->glue_in_use = new_in_use;
					ictx->in_use[ictx->glue_in_use] =
						ISC_TRUE;
				} else {
					result = commit(callbacks, lctx,
							&current_list,
							ictx->current,
							source,
							ictx->current_line);
					if (MANYERRS(lctx, result)) {
						SETRESULT(lctx, result);
					} else if (result != ISC_R_SUCCESS)
						goto insist_and_cleanup;
					rdcount = 0;
					rdlcount = 0;
					if (ictx->current_in_use != -1)
					    ictx->in_use[ictx->current_in_use] =
						ISC_FALSE;
					ictx->current_in_use = new_in_use;
					ictx->in_use[ictx->current_in_use] =
						ISC_TRUE;
					ictx->current = new_name;
					current_has_delegation = ISC_FALSE;
					isc_buffer_init(&target, target_mem,
							target_size);
				}
				/*
				 * Check for internal wildcards.
				 */
				if ((lctx->options & DNS_MASTER_CHECKWILDCARD)
						 != 0)
					check_wildcard(ictx, source, line,
						       callbacks);

			}
			if ((lctx->options & DNS_MASTER_ZONE) != 0 &&
			    (lctx->options & DNS_MASTER_SLAVE) == 0 &&
			    !dns_name_issubdomain(new_name, lctx->top))
			{
				char namebuf[DNS_NAME_FORMATSIZE];
				dns_name_format(new_name, namebuf,
						sizeof(namebuf));
				/*
				 * Ignore out-of-zone data.
				 */
				(*callbacks->warn)(callbacks,
				       "%s:%lu: "
				       "ignoring out-of-zone data (%s)",
				       source, line, namebuf);
				ictx->drop = ISC_TRUE;
			} else
				ictx->drop = ISC_FALSE;
		} else {
			UNEXPECTED_ERROR(__FILE__, __LINE__,
					 "%s:%lu: isc_lex_gettoken() returned "
					 "unexpected token type (%d)",
					 source, line, token.type);
			result = ISC_R_UNEXPECTED;
			if (MANYERRS(lctx, result)) {
				SETRESULT(lctx, result);
				LOGIT(result);
				continue;
			} else if (result != ISC_R_SUCCESS)
				goto insist_and_cleanup;
		}

		/*
		 * Find TTL, class and type.  Both TTL and class are optional
		 * and may occur in any order if they exist. TTL and class
		 * come before type which must exist.
		 *
		 * [<TTL>] [<class>] <type> <RDATA>
		 * [<class>] [<TTL>] <type> <RDATA>
		 */

		type = 0;
		rdclass = 0;

		GETTOKEN(lctx->lex, 0, &token, initialws);

		if (initialws) {
			if (token.type == isc_tokentype_eol) {
				read_till_eol = ISC_FALSE;
				continue;		/* blank line */
			}

			if (token.type == isc_tokentype_eof) {
				WARNUNEXPECTEDEOF(lctx->lex);
				read_till_eol = ISC_FALSE;
				isc_lex_ungettoken(lctx->lex, &token);
				continue;
			}

			if (ictx->current == NULL) {
				(*callbacks->error)(callbacks,
					"%s:%lu: no current owner name",
					source, line);
				result = DNS_R_NOOWNER;
				if (MANYERRS(lctx, result)) {
					SETRESULT(lctx, result);
					read_till_eol = ISC_TRUE;
					continue;
				} else if (result != ISC_R_SUCCESS)
					goto insist_and_cleanup;
			}
		}

		if (dns_rdataclass_fromtext(&rdclass,
					    &token.value.as_textregion)
				== ISC_R_SUCCESS)
			GETTOKEN(lctx->lex, 0, &token, ISC_FALSE);

		explicit_ttl = ISC_FALSE;
		if (dns_ttl_fromtext(&token.value.as_textregion, &lctx->ttl)
				== ISC_R_SUCCESS) {
			limit_ttl(callbacks, source, line, &lctx->ttl);
			explicit_ttl = ISC_TRUE;
			lctx->ttl_known = ISC_TRUE;
			GETTOKEN(lctx->lex, 0, &token, ISC_FALSE);
		}

		if (token.type != isc_tokentype_string) {
			UNEXPECTED_ERROR(__FILE__, __LINE__,
			"isc_lex_gettoken() returned unexpected token type");
			result = ISC_R_UNEXPECTED;
			if (MANYERRS(lctx, result)) {
				SETRESULT(lctx, result);
				read_till_eol = ISC_TRUE;
				continue;
			} else if (result != ISC_R_SUCCESS)
				goto insist_and_cleanup;
		}

		if (rdclass == 0 &&
		    dns_rdataclass_fromtext(&rdclass,
					    &token.value.as_textregion)
				== ISC_R_SUCCESS)
			GETTOKEN(lctx->lex, 0, &token, ISC_FALSE);

		if (token.type != isc_tokentype_string) {
			UNEXPECTED_ERROR(__FILE__, __LINE__,
			"isc_lex_gettoken() returned unexpected token type");
			result = ISC_R_UNEXPECTED;
			if (MANYERRS(lctx, result)) {
				SETRESULT(lctx, result);
				read_till_eol = ISC_TRUE;
				continue;
			} else if (result != ISC_R_SUCCESS)
				goto insist_and_cleanup;
		}

		result = dns_rdatatype_fromtext(&type,
						&token.value.as_textregion);
		if (result != ISC_R_SUCCESS) {
			(*callbacks->warn)(callbacks,
				   "%s:%lu: unknown RR type '%.*s'",
				   source, line,
				   token.value.as_textregion.length,
				   token.value.as_textregion.base);
			if (MANYERRS(lctx, result)) {
				SETRESULT(lctx, result);
				read_till_eol = ISC_TRUE;
				continue;
			} else if (result != ISC_R_SUCCESS)
				goto insist_and_cleanup;
		}

		/*
		 * If the class specified does not match the zone's class
		 * print out a error message and exit.
		 */
		if (rdclass != 0 && rdclass != lctx->zclass) {
  bad_class:

			dns_rdataclass_format(rdclass, classname1,
					      sizeof(classname1));
			dns_rdataclass_format(lctx->zclass, classname2,
					      sizeof(classname2));
			(*callbacks->error)(callbacks,
					    "%s:%lu: class '%s' != "
					    "zone class '%s'",
					    source, line,
					    classname1, classname2);
			result = DNS_R_BADCLASS;
			if (MANYERRS(lctx, result)) {
				SETRESULT(lctx, result);
				read_till_eol = ISC_TRUE;
				continue;
			} else if (result != ISC_R_SUCCESS)
				goto insist_and_cleanup;
		}

		if (type == dns_rdatatype_ns && ictx->glue == NULL)
			current_has_delegation = ISC_TRUE;

		/*
		 * RFC1123: MD and MF are not allowed to be loaded from
		 * master files.
		 */
		if ((lctx->options & DNS_MASTER_ZONE) != 0 &&
		    (lctx->options & DNS_MASTER_SLAVE) == 0 &&
		    (type == dns_rdatatype_md || type == dns_rdatatype_mf)) {
			char typename[DNS_RDATATYPE_FORMATSIZE];

			result = DNS_R_OBSOLETE;

			dns_rdatatype_format(type, typename, sizeof(typename));
			(*callbacks->error)(callbacks,
					    "%s:%lu: %s '%s': %s",
					    source, line,
					    "type", typename,
					    dns_result_totext(result));
			if (MANYERRS(lctx, result)) {
				SETRESULT(lctx, result);
			} else
				goto insist_and_cleanup;
		}

		/*
		 * Find a rdata structure.
		 */
		if (rdcount == rdata_size) {
			new_rdata = grow_rdata(rdata_size + RDSZ, rdata,
					       rdata_size, &current_list,
					       &glue_list, mctx);
			if (new_rdata == NULL) {
				result = ISC_R_NOMEMORY;
				goto log_and_cleanup;
			}
			rdata_size += RDSZ;
			rdata = new_rdata;
		}

		/*
		 * Peek at the NS record.
		 */
		if (type == dns_rdatatype_ns &&
		    lctx->zclass == dns_rdataclass_in &&
		    (lctx->options & DNS_MASTER_CHECKNS) != 0) {

			GETTOKEN(lctx->lex, 0, &token, ISC_FALSE);
			result = check_ns(lctx, &token, source, line);
			isc_lex_ungettoken(lctx->lex, &token);
			if ((lctx->options & DNS_MASTER_FATALNS) != 0) {
				if (MANYERRS(lctx, result)) {
					SETRESULT(lctx, result);
				} else if (result != ISC_R_SUCCESS)
					goto insist_and_cleanup;
			}
		}

		/*
		 * Check owner name.
		 */
		options &= ~DNS_RDATA_CHECKREVERSE;
		if ((lctx->options & DNS_MASTER_CHECKNAMES) != 0) {
			isc_boolean_t ok;
			dns_name_t *name;

			name = (ictx->glue != NULL) ? ictx->glue :
						      ictx->current;
			ok = dns_rdata_checkowner(name, lctx->zclass, type,
						  ISC_TRUE);
			if (!ok) {
				char namebuf[DNS_NAME_FORMATSIZE];
				const char *desc;
				dns_name_format(name, namebuf, sizeof(namebuf));
				result = DNS_R_BADOWNERNAME;
				desc = dns_result_totext(result);
				if ((lctx->options & DNS_MASTER_CHECKNAMESFAIL) != 0) {
					(*callbacks->error)(callbacks,
							    "%s:%lu: %s: %s",
							    source, line,
							    namebuf, desc);
					if (MANYERRS(lctx, result)) {
						SETRESULT(lctx, result);
					} else if (result != ISC_R_SUCCESS)
						goto cleanup;
				} else {
					(*callbacks->warn)(callbacks,
							   "%s:%lu: %s: %s",
							   source, line,
							   namebuf, desc);
				}
			}
			if (type == dns_rdatatype_ptr &&
			    (dns_name_issubdomain(name, &in_addr_arpa) ||
			     dns_name_issubdomain(name, &ip6_arpa) ||
			     dns_name_issubdomain(name, &ip6_int)))
				options |= DNS_RDATA_CHECKREVERSE;
		}

		/*
		 * Read rdata contents.
		 */
		dns_rdata_init(&rdata[rdcount]);
		target_ft = target;
		result = dns_rdata_fromtext(&rdata[rdcount], lctx->zclass,
					    type, lctx->lex, ictx->origin,
					    options, lctx->mctx, &target,
					    callbacks);
		if (MANYERRS(lctx, result)) {
			SETRESULT(lctx, result);
			continue;
		} else if (result != ISC_R_SUCCESS)
			goto insist_and_cleanup;

		if (ictx->drop) {
			target = target_ft;
			continue;
		}

		if (type == dns_rdatatype_soa &&
		    (lctx->options & DNS_MASTER_ZONE) != 0 &&
		    dns_name_compare(ictx->current, lctx->top) != 0) {
			char namebuf[DNS_NAME_FORMATSIZE];
			dns_name_format(ictx->current, namebuf,
					sizeof(namebuf));
			(*callbacks->error)(callbacks, "%s:%lu: SOA "
					    "record not at top of zone (%s)",
					    source, line, namebuf);
			result = DNS_R_NOTZONETOP;
			if (MANYERRS(lctx, result)) {
				SETRESULT(lctx, result);
				read_till_eol = ISC_TRUE;
				target = target_ft;
				continue;
			} else if (result != ISC_R_SUCCESS)
				goto insist_and_cleanup;
		}


		if (type == dns_rdatatype_rrsig ||
		    type == dns_rdatatype_sig)
			covers = dns_rdata_covers(&rdata[rdcount]);
		else
			covers = 0;

		if (!lctx->ttl_known && !lctx->default_ttl_known) {
			if (type == dns_rdatatype_soa) {
				(*callbacks->warn)(callbacks,
						   "%s:%lu: no TTL specified; "
						   "using SOA MINTTL instead",
						   source, line);
				lctx->ttl = dns_soa_getminimum(&rdata[rdcount]);
				limit_ttl(callbacks, source, line, &lctx->ttl);
				lctx->default_ttl = lctx->ttl;
				lctx->default_ttl_known = ISC_TRUE;
			} else if ((lctx->options & DNS_MASTER_HINT) != 0) {
				/*
				 * Zero TTL's are fine for hints.
				 */
				lctx->ttl = 0;
				lctx->default_ttl = lctx->ttl;
				lctx->default_ttl_known = ISC_TRUE;
			} else {
				(*callbacks->warn)(callbacks,
						   "%s:%lu: no TTL specified; "
						   "zone rejected",
						   source, line);
				result = DNS_R_NOTTL;
				if (MANYERRS(lctx, result)) {
					SETRESULT(lctx, result);
					lctx->ttl = 0;
				} else {
					goto insist_and_cleanup;
				}
			}
		} else if (!explicit_ttl && lctx->default_ttl_known) {
			lctx->ttl = lctx->default_ttl;
		} else if (!explicit_ttl && lctx->warn_1035) {
			(*callbacks->warn)(callbacks,
					   "%s:%lu: "
					   "using RFC1035 TTL semantics",
					   source, line);
			lctx->warn_1035 = ISC_FALSE;
		}

		if (type == dns_rdatatype_rrsig && lctx->warn_sigexpired) {
			dns_rdata_rrsig_t sig;
			result = dns_rdata_tostruct(&rdata[rdcount], &sig,
						    NULL);
			RUNTIME_CHECK(result == ISC_R_SUCCESS);
			if (isc_serial_lt(sig.timeexpire, now)) {
				(*callbacks->warn)(callbacks,
						   "%s:%lu: "
						   "signature has expired",
						   source, line);
				lctx->warn_sigexpired = ISC_FALSE;
			}
		}

		if ((type == dns_rdatatype_sig || type == dns_rdatatype_nxt) &&
		    lctx->warn_tcr && (lctx->options & DNS_MASTER_ZONE) != 0 &&
		    (lctx->options & DNS_MASTER_SLAVE) == 0) {
			(*callbacks->warn)(callbacks, "%s:%lu: old style DNSSEC "
					   " zone detected", source, line);
			lctx->warn_tcr = ISC_FALSE;
		}

		if ((lctx->options & DNS_MASTER_AGETTL) != 0) {
			/*
			 * Adjust the TTL for $DATE.  If the RR has already
			 * expired, ignore it.
			 */
			if (lctx->ttl < ttl_offset)
				continue;
			lctx->ttl -= ttl_offset;
		}

		/*
		 * Find type in rdatalist.
		 * If it does not exist create new one and prepend to list
		 * as this will minimise list traversal.
		 */
		if (ictx->glue != NULL)
			this = ISC_LIST_HEAD(glue_list);
		else
			this = ISC_LIST_HEAD(current_list);

		while (this != NULL) {
			if (this->type == type && this->covers == covers)
				break;
			this = ISC_LIST_NEXT(this, link);
		}

		if (this == NULL) {
			if (rdlcount == rdatalist_size) {
				new_rdatalist =
					grow_rdatalist(rdatalist_size + RDLSZ,
						       rdatalist,
						       rdatalist_size,
						       &current_list,
						       &glue_list,
						       mctx);
				if (new_rdatalist == NULL) {
					result = ISC_R_NOMEMORY;
					goto log_and_cleanup;
				}
				rdatalist = new_rdatalist;
				rdatalist_size += RDLSZ;
			}
			this = &rdatalist[rdlcount++];
			this->type = type;
			this->covers = covers;
			this->rdclass = lctx->zclass;
			this->ttl = lctx->ttl;
			ISC_LIST_INIT(this->rdata);
			if (ictx->glue != NULL)
				ISC_LIST_INITANDPREPEND(glue_list, this, link);
			else
				ISC_LIST_INITANDPREPEND(current_list, this,
							link);
		} else if (this->ttl != lctx->ttl) {
			(*callbacks->warn)(callbacks,
					   "%s:%lu: "
					   "TTL set to prior TTL (%lu)",
					   source, line, this->ttl);
			lctx->ttl = this->ttl;
		}

		ISC_LIST_APPEND(this->rdata, &rdata[rdcount], link);
		if (ictx->glue != NULL)
			ictx->glue_line = line;
		else
			ictx->current_line = line;
		rdcount++;

		/*
		 * We must have at least 64k as rdlen is 16 bits.
		 * If we don't commit everything we have so far.
		 */
		if ((target.length - target.used) < MINTSIZ)
			COMMITALL;
 next_line:
		;
	} while (!done && (lctx->loop_cnt == 0 || loop_cnt++ < lctx->loop_cnt));

	/*
	 * Commit what has not yet been committed.
	 */
	result = commit(callbacks, lctx, &current_list, ictx->current,
			source, ictx->current_line);
	if (MANYERRS(lctx, result)) {
		SETRESULT(lctx, result);
	} else if (result != ISC_R_SUCCESS)
		goto insist_and_cleanup;
	result = commit(callbacks, lctx, &glue_list, ictx->glue,
			source, ictx->glue_line);
	if (MANYERRS(lctx, result)) {
		SETRESULT(lctx, result);
	} else if (result != ISC_R_SUCCESS)
		goto insist_and_cleanup;

	if (!done) {
		INSIST(lctx->done != NULL && lctx->task != NULL);
		result = DNS_R_CONTINUE;
	} else if (result == ISC_R_SUCCESS && lctx->result != ISC_R_SUCCESS) {
		result = lctx->result;
	} else if (result == ISC_R_SUCCESS && lctx->seen_include)
		result = DNS_R_SEENINCLUDE;
	goto cleanup;

 log_and_cleanup:
	LOGIT(result);

 insist_and_cleanup:
	INSIST(result != ISC_R_SUCCESS);

 cleanup:
	while ((this = ISC_LIST_HEAD(current_list)) != NULL)
		ISC_LIST_UNLINK(current_list, this, link);
	while ((this = ISC_LIST_HEAD(glue_list)) != NULL)
		ISC_LIST_UNLINK(glue_list, this, link);
	if (rdatalist != NULL)
		isc_mem_put(mctx, rdatalist,
			    rdatalist_size * sizeof(*rdatalist));
	if (rdata != NULL)
		isc_mem_put(mctx, rdata, rdata_size * sizeof(*rdata));
	if (target_mem != NULL)
		isc_mem_put(mctx, target_mem, target_size);
	if (include_file != NULL)
		isc_mem_free(mctx, include_file);
	if (range != NULL)
		isc_mem_free(mctx, range);
	if (lhs != NULL)
		isc_mem_free(mctx, lhs);
	if (gtype != NULL)
		isc_mem_free(mctx, gtype);
	if (rhs != NULL)
		isc_mem_free(mctx, rhs);
	return (result);
}

static isc_result_t
pushfile(const char *master_file, dns_name_t *origin, dns_loadctx_t *lctx) {
	isc_result_t result;
	dns_incctx_t *ictx;
	dns_incctx_t *new = NULL;
	isc_region_t r;
	int new_in_use;

	REQUIRE(master_file != NULL);
	REQUIRE(DNS_LCTX_VALID(lctx));

	ictx = lctx->inc;
	lctx->seen_include = ISC_TRUE;

	result = incctx_create(lctx->mctx, origin, &new);
	if (result != ISC_R_SUCCESS)
		return (result);

	/* Set current domain. */
	if (ictx->glue != NULL || ictx->current != NULL) {
		for (new_in_use = 0; new_in_use < NBUFS; new_in_use++)
			if (!new->in_use[new_in_use])
				break;
		INSIST(new_in_use < NBUFS);
		new->current_in_use = new_in_use;
		new->current =
			dns_fixedname_name(&new->fixed[new->current_in_use]);
		new->in_use[new->current_in_use] = ISC_TRUE;
		dns_name_toregion((ictx->glue != NULL) ?
				   ictx->glue : ictx->current, &r);
		dns_name_fromregion(new->current, &r);
		new->drop = ictx->drop;
	}

	result = (lctx->openfile)(lctx, master_file);
	if (result != ISC_R_SUCCESS)
		goto cleanup;
	new->parent = ictx;
	lctx->inc = new;
	return (ISC_R_SUCCESS);

 cleanup:
	if (new != NULL)
		incctx_destroy(lctx->mctx, new);
	return (result);
}

static inline isc_result_t
read_and_check(isc_boolean_t do_read, isc_buffer_t *buffer,
	       size_t len, FILE *f)
{
	isc_result_t result;

	if (do_read) {
		INSIST(isc_buffer_availablelength(buffer) >= len);
		result = isc_stdio_read(isc_buffer_used(buffer), 1, len,
					f, NULL);
		if (result != ISC_R_SUCCESS)
			return (result);
		isc_buffer_add(buffer, len);
	} else if (isc_buffer_remaininglength(buffer) < len)
		return (ISC_R_RANGE);

	return (ISC_R_SUCCESS);
}

static isc_result_t
load_raw(dns_loadctx_t *lctx) {
	isc_result_t result = ISC_R_SUCCESS;
	isc_boolean_t done = ISC_FALSE;
	unsigned int loop_cnt = 0;
	dns_rdatacallbacks_t *callbacks;
	unsigned char namebuf[DNS_NAME_MAXWIRE];
	isc_region_t r;
	dns_name_t name;
	rdatalist_head_t head, dummy;
	dns_rdatalist_t rdatalist;
	isc_mem_t *mctx = lctx->mctx;
	dns_rdata_t *rdata = NULL;
	unsigned int rdata_size = 0;
	int target_size = TSIZ;
	isc_buffer_t target;
	unsigned char *target_mem = NULL;

	REQUIRE(DNS_LCTX_VALID(lctx));
	callbacks = lctx->callbacks;

	if (lctx->first) {
		dns_masterrawheader_t header;
		isc_uint32_t format, version, dumptime;
		size_t hdrlen = sizeof(format) + sizeof(version) +
			sizeof(dumptime);

		INSIST(hdrlen <= sizeof(header));
		isc_buffer_init(&target, &header, sizeof(header));

		result = isc_stdio_read(&header, 1, hdrlen, lctx->f, NULL);
		if (result != ISC_R_SUCCESS) {
			UNEXPECTED_ERROR(__FILE__, __LINE__,
					 "isc_stdio_read failed: %s",
					 isc_result_totext(result));
			return (result);
		}
		isc_buffer_add(&target, hdrlen);
		format = isc_buffer_getuint32(&target);
		if (format != dns_masterformat_raw) {
			(*callbacks->error)(callbacks,
					    "dns_master_load: "
					    "file format mismatch");
			return (ISC_R_NOTIMPLEMENTED);
		}

		version = isc_buffer_getuint32(&target);
		if (version > DNS_RAWFORMAT_VERSION) {
			(*callbacks->error)(callbacks,
					    "dns_master_load: "
					    "unsupported file format version");
			return (ISC_R_NOTIMPLEMENTED);
		}

		/* Empty read: currently, we do not use dumptime */
		dumptime = isc_buffer_getuint32(&target);

		lctx->first = ISC_FALSE;
	}

	ISC_LIST_INIT(head);
	ISC_LIST_INIT(dummy);
	dns_rdatalist_init(&rdatalist);

	/*
	 * Allocate target_size of buffer space.  This is greater than twice
	 * the maximum individual RR data size.
	 */
	target_mem = isc_mem_get(mctx, target_size);
	if (target_mem == NULL) {
		result = ISC_R_NOMEMORY;
		goto cleanup;
	}
	isc_buffer_init(&target, target_mem, target_size);

	/*
	 * In the following loop, we regard any error fatal regardless of
	 * whether "MANYERRORS" is set in the context option.  This is because
	 * normal errors should already have been checked at creation time.
	 * Besides, it is very unlikely that we can recover from an error
	 * in this format, and so trying to continue parsing erroneous data
	 * does not really make sense.
	 */
	for (loop_cnt = 0;
	     (lctx->loop_cnt == 0 || loop_cnt < lctx->loop_cnt);
	     loop_cnt++) {
		unsigned int i, rdcount, consumed_name;
		isc_uint16_t namelen;
		isc_uint32_t totallen;
		size_t minlen, readlen;
		isc_boolean_t sequential_read = ISC_FALSE;

		/* Read the data length */
		isc_buffer_clear(&target);
		INSIST(isc_buffer_availablelength(&target) >=
		       sizeof(totallen));
		result = isc_stdio_read(target.base, 1, sizeof(totallen),
					lctx->f, NULL);
		if (result == ISC_R_EOF) {
			result = ISC_R_SUCCESS;
			done = ISC_TRUE;
			break;
		}
		if (result != ISC_R_SUCCESS)
			goto cleanup;
		isc_buffer_add(&target, sizeof(totallen));
		totallen = isc_buffer_getuint32(&target);
		/*
		 * Validation: the input data must at least contain the common
		 * header.
		 */
		minlen = sizeof(totallen) + sizeof(isc_uint16_t) +
			sizeof(isc_uint16_t) + sizeof(isc_uint16_t) +
			sizeof(isc_uint32_t) + sizeof(isc_uint32_t);
		if (totallen < minlen) {
			result = ISC_R_RANGE;
			goto cleanup;
		}
		totallen -= sizeof(totallen);

		isc_buffer_clear(&target);
		if (totallen > isc_buffer_availablelength(&target)) {
			/*
			 * The default buffer size should typically be large
			 * enough to store the entire RRset.  We could try to
			 * allocate enough space if this is not the case, but
			 * it might cause a hazardous result when "totallen"
			 * is forged.  Thus, we'd rather take an inefficient
			 * but robust approach in this atypical case: read
			 * data step by step, and commit partial data when
			 * necessary.  Note that the buffer must be large
			 * enough to store the "header part", owner name, and
			 * at least one rdata (however large it is).
			 */
			sequential_read = ISC_TRUE;
			readlen = minlen - sizeof(totallen);
		} else {
			/*
			 * Typical case.  We can read the whole RRset at once
			 * with the default buffer.
			 */
			readlen = totallen;
		}
		result = isc_stdio_read(target.base, 1, readlen,
					lctx->f, NULL);
		if (result != ISC_R_SUCCESS)
			goto cleanup;
		isc_buffer_add(&target, readlen);

		/* Construct RRset headers */
		rdatalist.rdclass = isc_buffer_getuint16(&target);
		rdatalist.type = isc_buffer_getuint16(&target);
		rdatalist.covers = isc_buffer_getuint16(&target);
		rdatalist.ttl =  isc_buffer_getuint32(&target);
		rdcount = isc_buffer_getuint32(&target);
		if (rdcount == 0) {
			result = ISC_R_RANGE;
			goto cleanup;
		}
		INSIST(isc_buffer_consumedlength(&target) <= readlen);

		/* Owner name: length followed by name */
		result = read_and_check(sequential_read, &target,
					sizeof(namelen), lctx->f);
		if (result != ISC_R_SUCCESS)
			goto cleanup;
		namelen = isc_buffer_getuint16(&target);
		if (namelen > sizeof(namebuf)) {
			result = ISC_R_RANGE;
			goto cleanup;
		}

		result = read_and_check(sequential_read, &target, namelen,
					lctx->f);
		if (result != ISC_R_SUCCESS)
			goto cleanup;
		isc_buffer_setactive(&target, (unsigned int)namelen);
		isc_buffer_activeregion(&target, &r);
		dns_name_init(&name, NULL);
		dns_name_fromregion(&name, &r);
		isc_buffer_forward(&target, (unsigned int)namelen);
		consumed_name = isc_buffer_consumedlength(&target);

		/* Rdata contents. */
		if (rdcount > rdata_size) {
			dns_rdata_t *new_rdata = NULL;

			new_rdata = grow_rdata(rdata_size + RDSZ, rdata,
					       rdata_size, &head,
					       &dummy, mctx);
			if (new_rdata == NULL) {
				result = ISC_R_NOMEMORY;
				goto cleanup;
			}
			rdata_size += RDSZ;
			rdata = new_rdata;
		}

	continue_read:
		for (i = 0; i < rdcount; i++) {
			isc_uint16_t rdlen;

			dns_rdata_init(&rdata[i]);

			if (sequential_read &&
			    isc_buffer_availablelength(&target) < MINTSIZ) {
				unsigned int j;

				INSIST(i > 0); /* detect an infinite loop */

				/* Partial Commit. */
				ISC_LIST_APPEND(head, &rdatalist, link);
				result = commit(callbacks, lctx, &head, &name,
						NULL, 0);
				for (j = 0; j < i; j++) {
					ISC_LIST_UNLINK(rdatalist.rdata,
							&rdata[j], link);
					dns_rdata_reset(&rdata[j]);
				}
				if (result != ISC_R_SUCCESS)
					goto cleanup;

				/* Rewind the buffer and continue */
				isc_buffer_clear(&target);
				isc_buffer_add(&target, consumed_name);
				isc_buffer_forward(&target, consumed_name);

				rdcount -= i;
				i = 0;

				goto continue_read;
			}

			/* rdata length */
			result = read_and_check(sequential_read, &target,
						sizeof(rdlen), lctx->f);
			if (result != ISC_R_SUCCESS)
				goto cleanup;
			rdlen = isc_buffer_getuint16(&target);

			/* rdata */
			result = read_and_check(sequential_read, &target,
						rdlen, lctx->f);
			if (result != ISC_R_SUCCESS)
				goto cleanup;
			isc_buffer_setactive(&target, (unsigned int)rdlen);
			isc_buffer_activeregion(&target, &r);
			isc_buffer_forward(&target, (unsigned int)rdlen);
			dns_rdata_fromregion(&rdata[i], rdatalist.rdclass,
					     rdatalist.type, &r);

			ISC_LIST_APPEND(rdatalist.rdata, &rdata[i], link);
		}

		/*
		 * Sanity check.  Still having remaining space is not
		 * necessarily critical, but it very likely indicates broken
		 * or malformed data.
		 */
		if (isc_buffer_remaininglength(&target) != 0) {
			result = ISC_R_RANGE;
			goto cleanup;
		}

		ISC_LIST_APPEND(head, &rdatalist, link);

		/* Commit this RRset.  rdatalist will be unlinked. */
		result = commit(callbacks, lctx, &head, &name, NULL, 0);

		for (i = 0; i < rdcount; i++) {
			ISC_LIST_UNLINK(rdatalist.rdata, &rdata[i], link);
			dns_rdata_reset(&rdata[i]);
		}

		if (result != ISC_R_SUCCESS)
			goto cleanup;
	}

	if (!done) {
		INSIST(lctx->done != NULL && lctx->task != NULL);
		result = DNS_R_CONTINUE;
	} else if (result == ISC_R_SUCCESS && lctx->result != ISC_R_SUCCESS)
		result = lctx->result;

 cleanup:
	if (rdata != NULL)
		isc_mem_put(mctx, rdata, rdata_size * sizeof(*rdata));
	if (target_mem != NULL)
		isc_mem_put(mctx, target_mem, target_size);
	if (result != ISC_R_SUCCESS && result != DNS_R_CONTINUE) {
		(*callbacks->error)(callbacks, "dns_master_load: %s",
				    dns_result_totext(result));
	}

	return (result);
}

isc_result_t
dns_master_loadfile(const char *master_file, dns_name_t *top,
		    dns_name_t *origin,
		    dns_rdataclass_t zclass, unsigned int options,
		    dns_rdatacallbacks_t *callbacks, isc_mem_t *mctx)
{
	return (dns_master_loadfile3(master_file, top, origin, zclass, options,
				     0, callbacks, mctx, dns_masterformat_text));
}

isc_result_t
dns_master_loadfile2(const char *master_file, dns_name_t *top,
		     dns_name_t *origin,
		     dns_rdataclass_t zclass, unsigned int options,
		     dns_rdatacallbacks_t *callbacks, isc_mem_t *mctx,
		     dns_masterformat_t format)
{
	return (dns_master_loadfile3(master_file, top, origin, zclass, options,
				     0, callbacks, mctx, format));
}

isc_result_t
dns_master_loadfile3(const char *master_file, dns_name_t *top,
		     dns_name_t *origin, dns_rdataclass_t zclass,
		     unsigned int options, isc_uint32_t resign,
		     dns_rdatacallbacks_t *callbacks, isc_mem_t *mctx,
		     dns_masterformat_t format)
{
	dns_loadctx_t *lctx = NULL;
	isc_result_t result;

	result = loadctx_create(format, mctx, options, resign, top, zclass,
				origin, callbacks, NULL, NULL, NULL, NULL,
				&lctx);
	if (result != ISC_R_SUCCESS)
		return (result);

	result = (lctx->openfile)(lctx, master_file);
	if (result != ISC_R_SUCCESS)
		goto cleanup;

	result = (lctx->load)(lctx);
	INSIST(result != DNS_R_CONTINUE);

 cleanup:
	dns_loadctx_detach(&lctx);
	return (result);
}

isc_result_t
dns_master_loadfileinc(const char *master_file, dns_name_t *top,
		       dns_name_t *origin, dns_rdataclass_t zclass,
		       unsigned int options, dns_rdatacallbacks_t *callbacks,
		       isc_task_t *task, dns_loaddonefunc_t done,
		       void *done_arg, dns_loadctx_t **lctxp, isc_mem_t *mctx)
{
	return (dns_master_loadfileinc3(master_file, top, origin, zclass,
					options, 0, callbacks, task, done,
					done_arg, lctxp, mctx,
					dns_masterformat_text));
}

isc_result_t
dns_master_loadfileinc2(const char *master_file, dns_name_t *top,
			dns_name_t *origin, dns_rdataclass_t zclass,
			unsigned int options, dns_rdatacallbacks_t *callbacks,
			isc_task_t *task, dns_loaddonefunc_t done,
			void *done_arg, dns_loadctx_t **lctxp, isc_mem_t *mctx,
			dns_masterformat_t format)
{
	return (dns_master_loadfileinc3(master_file, top, origin, zclass,
					options, 0, callbacks, task, done,
					done_arg, lctxp, mctx, format));
}

isc_result_t
dns_master_loadfileinc3(const char *master_file, dns_name_t *top,
			dns_name_t *origin, dns_rdataclass_t zclass,
			unsigned int options, isc_uint32_t resign,
			dns_rdatacallbacks_t *callbacks, isc_task_t *task,
			dns_loaddonefunc_t done, void *done_arg,
			dns_loadctx_t **lctxp, isc_mem_t *mctx,
			dns_masterformat_t format)
{
	dns_loadctx_t *lctx = NULL;
	isc_result_t result;

	REQUIRE(task != NULL);
	REQUIRE(done != NULL);

	result = loadctx_create(format, mctx, options, resign, top, zclass,
				origin, callbacks, task, done, done_arg, NULL,
				&lctx);
	if (result != ISC_R_SUCCESS)
		return (result);

	result = (lctx->openfile)(lctx, master_file);
	if (result != ISC_R_SUCCESS)
		goto cleanup;

	result = task_send(lctx);
	if (result == ISC_R_SUCCESS) {
		dns_loadctx_attach(lctx, lctxp);
		return (DNS_R_CONTINUE);
	}

 cleanup:
	dns_loadctx_detach(&lctx);
	return (result);
}

isc_result_t
dns_master_loadstream(FILE *stream, dns_name_t *top, dns_name_t *origin,
		      dns_rdataclass_t zclass, unsigned int options,
		      dns_rdatacallbacks_t *callbacks, isc_mem_t *mctx)
{
	isc_result_t result;
	dns_loadctx_t *lctx = NULL;

	REQUIRE(stream != NULL);

	result = loadctx_create(dns_masterformat_text, mctx, options, 0, top,
				zclass, origin, callbacks, NULL, NULL, NULL,
				NULL, &lctx);
	if (result != ISC_R_SUCCESS)
		goto cleanup;

	result = isc_lex_openstream(lctx->lex, stream);
	if (result != ISC_R_SUCCESS)
		goto cleanup;

	result = (lctx->load)(lctx);
	INSIST(result != DNS_R_CONTINUE);

 cleanup:
	if (lctx != NULL)
		dns_loadctx_detach(&lctx);
	return (result);
}

isc_result_t
dns_master_loadstreaminc(FILE *stream, dns_name_t *top, dns_name_t *origin,
			 dns_rdataclass_t zclass, unsigned int options,
			 dns_rdatacallbacks_t *callbacks, isc_task_t *task,
			 dns_loaddonefunc_t done, void *done_arg,
			 dns_loadctx_t **lctxp, isc_mem_t *mctx)
{
	isc_result_t result;
	dns_loadctx_t *lctx = NULL;

	REQUIRE(stream != NULL);
	REQUIRE(task != NULL);
	REQUIRE(done != NULL);

	result = loadctx_create(dns_masterformat_text, mctx, options, 0, top,
				zclass, origin, callbacks, task, done,
				done_arg, NULL, &lctx);
	if (result != ISC_R_SUCCESS)
		goto cleanup;

	result = isc_lex_openstream(lctx->lex, stream);
	if (result != ISC_R_SUCCESS)
		goto cleanup;

	result = task_send(lctx);
	if (result == ISC_R_SUCCESS) {
		dns_loadctx_attach(lctx, lctxp);
		return (DNS_R_CONTINUE);
	}

 cleanup:
	if (lctx != NULL)
		dns_loadctx_detach(&lctx);
	return (result);
}

isc_result_t
dns_master_loadbuffer(isc_buffer_t *buffer, dns_name_t *top,
		      dns_name_t *origin, dns_rdataclass_t zclass,
		      unsigned int options,
		      dns_rdatacallbacks_t *callbacks, isc_mem_t *mctx)
{
	isc_result_t result;
	dns_loadctx_t *lctx = NULL;

	REQUIRE(buffer != NULL);

	result = loadctx_create(dns_masterformat_text, mctx, options, 0, top,
				zclass, origin, callbacks, NULL, NULL, NULL,
				NULL, &lctx);
	if (result != ISC_R_SUCCESS)
		return (result);

	result = isc_lex_openbuffer(lctx->lex, buffer);
	if (result != ISC_R_SUCCESS)
		goto cleanup;

	result = (lctx->load)(lctx);
	INSIST(result != DNS_R_CONTINUE);

 cleanup:
	dns_loadctx_detach(&lctx);
	return (result);
}

isc_result_t
dns_master_loadbufferinc(isc_buffer_t *buffer, dns_name_t *top,
			 dns_name_t *origin, dns_rdataclass_t zclass,
			 unsigned int options,
			 dns_rdatacallbacks_t *callbacks, isc_task_t *task,
			 dns_loaddonefunc_t done, void *done_arg,
			 dns_loadctx_t **lctxp, isc_mem_t *mctx)
{
	isc_result_t result;
	dns_loadctx_t *lctx = NULL;

	REQUIRE(buffer != NULL);
	REQUIRE(task != NULL);
	REQUIRE(done != NULL);

	result = loadctx_create(dns_masterformat_text, mctx, options, 0, top,
				zclass, origin, callbacks, task, done,
				done_arg, NULL, &lctx);
	if (result != ISC_R_SUCCESS)
		return (result);

	result = isc_lex_openbuffer(lctx->lex, buffer);
	if (result != ISC_R_SUCCESS)
		goto cleanup;

	result = task_send(lctx);
	if (result == ISC_R_SUCCESS) {
		dns_loadctx_attach(lctx, lctxp);
		return (DNS_R_CONTINUE);
	}

 cleanup:
	dns_loadctx_detach(&lctx);
	return (result);
}

isc_result_t
dns_master_loadlexer(isc_lex_t *lex, dns_name_t *top,
		     dns_name_t *origin, dns_rdataclass_t zclass,
		     unsigned int options,
		     dns_rdatacallbacks_t *callbacks, isc_mem_t *mctx)
{
	isc_result_t result;
	dns_loadctx_t *lctx = NULL;

	REQUIRE(lex != NULL);

	result = loadctx_create(dns_masterformat_text, mctx, options, 0, top,
				zclass, origin, callbacks, NULL, NULL, NULL,
				lex, &lctx);
	if (result != ISC_R_SUCCESS)
		return (result);

	result = (lctx->load)(lctx);
	INSIST(result != DNS_R_CONTINUE);

	dns_loadctx_detach(&lctx);
	return (result);
}

isc_result_t
dns_master_loadlexerinc(isc_lex_t *lex, dns_name_t *top,
			dns_name_t *origin, dns_rdataclass_t zclass,
			unsigned int options,
			dns_rdatacallbacks_t *callbacks, isc_task_t *task,
			dns_loaddonefunc_t done, void *done_arg,
			dns_loadctx_t **lctxp, isc_mem_t *mctx)
{
	isc_result_t result;
	dns_loadctx_t *lctx = NULL;

	REQUIRE(lex != NULL);
	REQUIRE(task != NULL);
	REQUIRE(done != NULL);

	result = loadctx_create(dns_masterformat_text, mctx, options, 0, top,
				zclass, origin, callbacks, task, done,
				done_arg, lex, &lctx);
	if (result != ISC_R_SUCCESS)
		return (result);

	result = task_send(lctx);
	if (result == ISC_R_SUCCESS) {
		dns_loadctx_attach(lctx, lctxp);
		return (DNS_R_CONTINUE);
	}

	dns_loadctx_detach(&lctx);
	return (result);
}

/*
 * Grow the slab of dns_rdatalist_t structures.
 * Re-link glue and current list.
 */
static dns_rdatalist_t *
grow_rdatalist(int new_len, dns_rdatalist_t *old, int old_len,
	       rdatalist_head_t *current, rdatalist_head_t *glue,
	       isc_mem_t *mctx)
{
	dns_rdatalist_t *new;
	int rdlcount = 0;
	ISC_LIST(dns_rdatalist_t) save;
	dns_rdatalist_t *this;

	new = isc_mem_get(mctx, new_len * sizeof(*new));
	if (new == NULL)
		return (NULL);

	ISC_LIST_INIT(save);
	this = ISC_LIST_HEAD(*current);
	while ((this = ISC_LIST_HEAD(*current)) != NULL) {
		ISC_LIST_UNLINK(*current, this, link);
		ISC_LIST_APPEND(save, this, link);
	}
	while ((this = ISC_LIST_HEAD(save)) != NULL) {
		ISC_LIST_UNLINK(save, this, link);
		new[rdlcount] = *this;
		ISC_LIST_APPEND(*current, &new[rdlcount], link);
		rdlcount++;
	}

	ISC_LIST_INIT(save);
	this = ISC_LIST_HEAD(*glue);
	while ((this = ISC_LIST_HEAD(*glue)) != NULL) {
		ISC_LIST_UNLINK(*glue, this, link);
		ISC_LIST_APPEND(save, this, link);
	}
	while ((this = ISC_LIST_HEAD(save)) != NULL) {
		ISC_LIST_UNLINK(save, this, link);
		new[rdlcount] = *this;
		ISC_LIST_APPEND(*glue, &new[rdlcount], link);
		rdlcount++;
	}

	INSIST(rdlcount == old_len);
	if (old != NULL)
		isc_mem_put(mctx, old, old_len * sizeof(*old));
	return (new);
}

/*
 * Grow the slab of rdata structs.
 * Re-link the current and glue chains.
 */
static dns_rdata_t *
grow_rdata(int new_len, dns_rdata_t *old, int old_len,
	   rdatalist_head_t *current, rdatalist_head_t *glue,
	   isc_mem_t *mctx)
{
	dns_rdata_t *new;
	int rdcount = 0;
	ISC_LIST(dns_rdata_t) save;
	dns_rdatalist_t *this;
	dns_rdata_t *rdata;

	new = isc_mem_get(mctx, new_len * sizeof(*new));
	if (new == NULL)
		return (NULL);
	memset(new, 0, new_len * sizeof(*new));

	/*
	 * Copy current relinking.
	 */
	this = ISC_LIST_HEAD(*current);
	while (this != NULL) {
		ISC_LIST_INIT(save);
		while ((rdata = ISC_LIST_HEAD(this->rdata)) != NULL) {
			ISC_LIST_UNLINK(this->rdata, rdata, link);
			ISC_LIST_APPEND(save, rdata, link);
		}
		while ((rdata = ISC_LIST_HEAD(save)) != NULL) {
			ISC_LIST_UNLINK(save, rdata, link);
			new[rdcount] = *rdata;
			ISC_LIST_APPEND(this->rdata, &new[rdcount], link);
			rdcount++;
		}
		this = ISC_LIST_NEXT(this, link);
	}

	/*
	 * Copy glue relinking.
	 */
	this = ISC_LIST_HEAD(*glue);
	while (this != NULL) {
		ISC_LIST_INIT(save);
		while ((rdata = ISC_LIST_HEAD(this->rdata)) != NULL) {
			ISC_LIST_UNLINK(this->rdata, rdata, link);
			ISC_LIST_APPEND(save, rdata, link);
		}
		while ((rdata = ISC_LIST_HEAD(save)) != NULL) {
			ISC_LIST_UNLINK(save, rdata, link);
			new[rdcount] = *rdata;
			ISC_LIST_APPEND(this->rdata, &new[rdcount], link);
			rdcount++;
		}
		this = ISC_LIST_NEXT(this, link);
	}
	INSIST(rdcount == old_len);
	if (old != NULL)
		isc_mem_put(mctx, old, old_len * sizeof(*old));
	return (new);
}

static isc_uint32_t
resign_fromlist(dns_rdatalist_t *this, isc_uint32_t resign) {
	dns_rdata_t *rdata;
	dns_rdata_rrsig_t sig;
	isc_uint32_t when;

	rdata = ISC_LIST_HEAD(this->rdata);
	INSIST(rdata != NULL);
	(void)dns_rdata_tostruct(rdata, &sig, NULL);
	when = sig.timeexpire - resign;

	rdata = ISC_LIST_NEXT(rdata, link);
	while (rdata != NULL) {
		(void)dns_rdata_tostruct(rdata, &sig, NULL);
		if (sig.timeexpire - resign < when)
			when = sig.timeexpire - resign;
		rdata = ISC_LIST_NEXT(rdata, link);
	}
	return (when);
}

/*
 * Convert each element from a rdatalist_t to rdataset then call commit.
 * Unlink each element as we go.
 */

static isc_result_t
commit(dns_rdatacallbacks_t *callbacks, dns_loadctx_t *lctx,
       rdatalist_head_t *head, dns_name_t *owner,
       const char *source, unsigned int line)
{
	dns_rdatalist_t *this;
	dns_rdataset_t dataset;
	isc_result_t result;
	char namebuf[DNS_NAME_FORMATSIZE];
	void    (*error)(struct dns_rdatacallbacks *, const char *, ...);

	this = ISC_LIST_HEAD(*head);
	error = callbacks->error;

	if (this == NULL)
		return (ISC_R_SUCCESS);
	do {
		dns_rdataset_init(&dataset);
		RUNTIME_CHECK(dns_rdatalist_tordataset(this, &dataset)
			      == ISC_R_SUCCESS);
		dataset.trust = dns_trust_ultimate;
		/*
		 * If this is a secure dynamic zone set the re-signing time.
		 */
		if (dataset.type == dns_rdatatype_rrsig &&
		    (lctx->options & DNS_MASTER_RESIGN) != 0) {
			dataset.attributes |= DNS_RDATASETATTR_RESIGN;
			dns_name_format(owner, namebuf, sizeof(namebuf));
			dataset.resign = resign_fromlist(this, lctx->resign);
		}
		result = ((*callbacks->add)(callbacks->add_private, owner,
					    &dataset));
		if (result == ISC_R_NOMEMORY) {
			(*error)(callbacks, "dns_master_load: %s",
				 dns_result_totext(result));
		} else if (result != ISC_R_SUCCESS) {
			dns_name_format(owner, namebuf, sizeof(namebuf));
			if (source != NULL) {
				(*error)(callbacks, "%s: %s:%lu: %s: %s",
					 "dns_master_load", source, line,
					 namebuf, dns_result_totext(result));
			} else {
				(*error)(callbacks, "%s: %s: %s",
					 "dns_master_load", namebuf,
					 dns_result_totext(result));
			}
		}
		if (MANYERRS(lctx, result))
			SETRESULT(lctx, result);
		else if (result != ISC_R_SUCCESS)
			return (result);
		ISC_LIST_UNLINK(*head, this, link);
		this = ISC_LIST_HEAD(*head);
	} while (this != NULL);
	return (ISC_R_SUCCESS);
}

/*
 * Returns ISC_TRUE if one of the NS rdata's contains 'owner'.
 */

static isc_boolean_t
is_glue(rdatalist_head_t *head, dns_name_t *owner) {
	dns_rdatalist_t *this;
	dns_rdata_t *rdata;
	isc_region_t region;
	dns_name_t name;

	/*
	 * Find NS rrset.
	 */
	this = ISC_LIST_HEAD(*head);
	while (this != NULL) {
		if (this->type == dns_rdatatype_ns)
			break;
		this = ISC_LIST_NEXT(this, link);
	}
	if (this == NULL)
		return (ISC_FALSE);

	rdata = ISC_LIST_HEAD(this->rdata);
	while (rdata != NULL) {
		dns_name_init(&name, NULL);
		dns_rdata_toregion(rdata, &region);
		dns_name_fromregion(&name, &region);
		if (dns_name_compare(&name, owner) == 0)
			return (ISC_TRUE);
		rdata = ISC_LIST_NEXT(rdata, link);
	}
	return (ISC_FALSE);
}

static void
load_quantum(isc_task_t *task, isc_event_t *event) {
	isc_result_t result;
	dns_loadctx_t *lctx;

	REQUIRE(event != NULL);
	lctx = event->ev_arg;
	REQUIRE(DNS_LCTX_VALID(lctx));

	if (lctx->canceled)
		result = ISC_R_CANCELED;
	else
		result = (lctx->load)(lctx);
	if (result == DNS_R_CONTINUE) {
		event->ev_arg = lctx;
		isc_task_send(task, &event);
	} else {
		(lctx->done)(lctx->done_arg, result);
		isc_event_free(&event);
		dns_loadctx_detach(&lctx);
	}
}

static isc_result_t
task_send(dns_loadctx_t *lctx) {
	isc_event_t *event;

	event = isc_event_allocate(lctx->mctx, NULL,
				   DNS_EVENT_MASTERQUANTUM,
				   load_quantum, lctx, sizeof(*event));
	if (event == NULL)
		return (ISC_R_NOMEMORY);
	isc_task_send(lctx->task, &event);
	return (ISC_R_SUCCESS);
}

void
dns_loadctx_cancel(dns_loadctx_t *lctx) {
	REQUIRE(DNS_LCTX_VALID(lctx));

	LOCK(&lctx->lock);
	lctx->canceled = ISC_TRUE;
	UNLOCK(&lctx->lock);
}
