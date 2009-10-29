/*
 * Copyright (C) 2004-2008  Internet Systems Consortium, Inc. ("ISC")
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

/* $Id: lwconfig.c,v 1.46.332.2 2008/12/30 23:46:49 tbox Exp $ */

/*! \file */

/**
 * Module for parsing resolv.conf files.
 *
 *    lwres_conf_init() creates an empty lwres_conf_t structure for
 *    lightweight resolver context ctx.
 *
 *    lwres_conf_clear() frees up all the internal memory used by that
 *    lwres_conf_t structure in resolver context ctx.
 *
 *    lwres_conf_parse() opens the file filename and parses it to initialise
 *    the resolver context ctx's lwres_conf_t structure.
 *
 *    lwres_conf_print() prints the lwres_conf_t structure for resolver
 *    context ctx to the FILE fp.
 *
 * \section lwconfig_return Return Values
 *
 *    lwres_conf_parse() returns #LWRES_R_SUCCESS if it successfully read and
 *    parsed filename. It returns #LWRES_R_FAILURE if filename could not be
 *    opened or contained incorrect resolver statements.
 *
 *    lwres_conf_print() returns #LWRES_R_SUCCESS unless an error occurred
 *    when converting the network addresses to a numeric host address
 *    string. If this happens, the function returns #LWRES_R_FAILURE.
 *
 * \section lwconfig_see See Also
 *
 *    stdio(3), \link resolver resolver \endlink
 *
 * \section files Files
 *
 *    /etc/resolv.conf
 */

#include <config.h>

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <lwres/lwbuffer.h>
#include <lwres/lwres.h>
#include <lwres/net.h>
#include <lwres/result.h>

#include "assert_p.h"
#include "context_p.h"


#if ! defined(NS_INADDRSZ)
#define NS_INADDRSZ	 4
#endif

#if ! defined(NS_IN6ADDRSZ)
#define NS_IN6ADDRSZ	16
#endif

static lwres_result_t
lwres_conf_parsenameserver(lwres_context_t *ctx,  FILE *fp);

static lwres_result_t
lwres_conf_parselwserver(lwres_context_t *ctx,  FILE *fp);

static lwres_result_t
lwres_conf_parsedomain(lwres_context_t *ctx, FILE *fp);

static lwres_result_t
lwres_conf_parsesearch(lwres_context_t *ctx,  FILE *fp);

static lwres_result_t
lwres_conf_parsesortlist(lwres_context_t *ctx,  FILE *fp);

static lwres_result_t
lwres_conf_parseoption(lwres_context_t *ctx,  FILE *fp);

static void
lwres_resetaddr(lwres_addr_t *addr);

static lwres_result_t
lwres_create_addr(const char *buff, lwres_addr_t *addr, int convert_zero);

static int lwresaddr2af(int lwresaddrtype);


static int
lwresaddr2af(int lwresaddrtype)
{
	int af = 0;

	switch (lwresaddrtype) {
	case LWRES_ADDRTYPE_V4:
		af = AF_INET;
		break;

	case LWRES_ADDRTYPE_V6:
		af = AF_INET6;
		break;
	}

	return (af);
}


/*!
 * Eat characters from FP until EOL or EOF. Returns EOF or '\n'
 */
static int
eatline(FILE *fp) {
	int ch;

	ch = fgetc(fp);
	while (ch != '\n' && ch != EOF)
		ch = fgetc(fp);

	return (ch);
}


/*!
 * Eats white space up to next newline or non-whitespace character (of
 * EOF). Returns the last character read. Comments are considered white
 * space.
 */
static int
eatwhite(FILE *fp) {
	int ch;

	ch = fgetc(fp);
	while (ch != '\n' && ch != EOF && isspace((unsigned char)ch))
		ch = fgetc(fp);

	if (ch == ';' || ch == '#')
		ch = eatline(fp);

	return (ch);
}


/*!
 * Skip over any leading whitespace and then read in the next sequence of
 * non-whitespace characters. In this context newline is not considered
 * whitespace. Returns EOF on end-of-file, or the character
 * that caused the reading to stop.
 */
static int
getword(FILE *fp, char *buffer, size_t size) {
	int ch;
	char *p = buffer;

	REQUIRE(buffer != NULL);
	REQUIRE(size > 0U);

	*p = '\0';

	ch = eatwhite(fp);

	if (ch == EOF)
		return (EOF);

	do {
		*p = '\0';

		if (ch == EOF || isspace((unsigned char)ch))
			break;
		else if ((size_t) (p - buffer) == size - 1)
			return (EOF);	/* Not enough space. */

		*p++ = (char)ch;
		ch = fgetc(fp);
	} while (1);

	return (ch);
}

static void
lwres_resetaddr(lwres_addr_t *addr) {
	REQUIRE(addr != NULL);

	memset(addr->address, 0, LWRES_ADDR_MAXLEN);
	addr->family = 0;
	addr->length = 0;
}

static char *
lwres_strdup(lwres_context_t *ctx, const char *str) {
	char *p;

	REQUIRE(str != NULL);
	REQUIRE(strlen(str) > 0U);

	p = CTXMALLOC(strlen(str) + 1);
	if (p != NULL)
		strcpy(p, str);

	return (p);
}

/*% intializes data structure for subsequent config parsing. */
void
lwres_conf_init(lwres_context_t *ctx) {
	int i;
	lwres_conf_t *confdata;

	REQUIRE(ctx != NULL);
	confdata = &ctx->confdata;

	confdata->nsnext = 0;
	confdata->lwnext = 0;
	confdata->domainname = NULL;
	confdata->searchnxt = 0;
	confdata->sortlistnxt = 0;
	confdata->resdebug = 0;
	confdata->ndots = 1;
	confdata->no_tld_query = 0;

	for (i = 0; i < LWRES_CONFMAXNAMESERVERS; i++)
		lwres_resetaddr(&confdata->nameservers[i]);

	for (i = 0; i < LWRES_CONFMAXSEARCH; i++)
		confdata->search[i] = NULL;

	for (i = 0; i < LWRES_CONFMAXSORTLIST; i++) {
		lwres_resetaddr(&confdata->sortlist[i].addr);
		lwres_resetaddr(&confdata->sortlist[i].mask);
	}
}

/*% Frees up all the internal memory used by the config data structure, returning it to the lwres_context_t. */
void
lwres_conf_clear(lwres_context_t *ctx) {
	int i;
	lwres_conf_t *confdata;

	REQUIRE(ctx != NULL);
	confdata = &ctx->confdata;

	for (i = 0; i < confdata->nsnext; i++)
		lwres_resetaddr(&confdata->nameservers[i]);

	if (confdata->domainname != NULL) {
		CTXFREE(confdata->domainname,
			strlen(confdata->domainname) + 1);
		confdata->domainname = NULL;
	}

	for (i = 0; i < confdata->searchnxt; i++) {
		if (confdata->search[i] != NULL) {
			CTXFREE(confdata->search[i],
				strlen(confdata->search[i]) + 1);
			confdata->search[i] = NULL;
		}
	}

	for (i = 0; i < LWRES_CONFMAXSORTLIST; i++) {
		lwres_resetaddr(&confdata->sortlist[i].addr);
		lwres_resetaddr(&confdata->sortlist[i].mask);
	}

	confdata->nsnext = 0;
	confdata->lwnext = 0;
	confdata->domainname = NULL;
	confdata->searchnxt = 0;
	confdata->sortlistnxt = 0;
	confdata->resdebug = 0;
	confdata->ndots = 1;
	confdata->no_tld_query = 0;
}

static lwres_result_t
lwres_conf_parsenameserver(lwres_context_t *ctx,  FILE *fp) {
	char word[LWRES_CONFMAXLINELEN];
	int res;
	lwres_conf_t *confdata;
	lwres_addr_t address;

	confdata = &ctx->confdata;

	if (confdata->nsnext == LWRES_CONFMAXNAMESERVERS)
		return (LWRES_R_SUCCESS);

	res = getword(fp, word, sizeof(word));
	if (strlen(word) == 0U)
		return (LWRES_R_FAILURE); /* Nothing on line. */
	else if (res == ' ' || res == '\t')
		res = eatwhite(fp);

	if (res != EOF && res != '\n')
		return (LWRES_R_FAILURE); /* Extra junk on line. */

	res = lwres_create_addr(word, &address, 1);
	if (res == LWRES_R_SUCCESS &&
	    ((address.family == LWRES_ADDRTYPE_V4 && ctx->use_ipv4 == 1) ||
	     (address.family == LWRES_ADDRTYPE_V6 && ctx->use_ipv6 == 1))) {
		confdata->nameservers[confdata->nsnext++] = address;
	}

	return (LWRES_R_SUCCESS);
}

static lwres_result_t
lwres_conf_parselwserver(lwres_context_t *ctx,  FILE *fp) {
	char word[LWRES_CONFMAXLINELEN];
	int res;
	lwres_conf_t *confdata;

	confdata = &ctx->confdata;

	if (confdata->lwnext == LWRES_CONFMAXLWSERVERS)
		return (LWRES_R_SUCCESS);

	res = getword(fp, word, sizeof(word));
	if (strlen(word) == 0U)
		return (LWRES_R_FAILURE); /* Nothing on line. */
	else if (res == ' ' || res == '\t')
		res = eatwhite(fp);

	if (res != EOF && res != '\n')
		return (LWRES_R_FAILURE); /* Extra junk on line. */

	res = lwres_create_addr(word,
				&confdata->lwservers[confdata->lwnext++], 1);
	if (res != LWRES_R_SUCCESS)
		return (res);

	return (LWRES_R_SUCCESS);
}

static lwres_result_t
lwres_conf_parsedomain(lwres_context_t *ctx,  FILE *fp) {
	char word[LWRES_CONFMAXLINELEN];
	int res, i;
	lwres_conf_t *confdata;

	confdata = &ctx->confdata;

	res = getword(fp, word, sizeof(word));
	if (strlen(word) == 0U)
		return (LWRES_R_FAILURE); /* Nothing else on line. */
	else if (res == ' ' || res == '\t')
		res = eatwhite(fp);

	if (res != EOF && res != '\n')
		return (LWRES_R_FAILURE); /* Extra junk on line. */

	if (confdata->domainname != NULL)
		CTXFREE(confdata->domainname,
			strlen(confdata->domainname) + 1); /*  */

	/*
	 * Search and domain are mutually exclusive.
	 */
	for (i = 0; i < LWRES_CONFMAXSEARCH; i++) {
		if (confdata->search[i] != NULL) {
			CTXFREE(confdata->search[i],
				strlen(confdata->search[i])+1);
			confdata->search[i] = NULL;
		}
	}
	confdata->searchnxt = 0;

	confdata->domainname = lwres_strdup(ctx, word);

	if (confdata->domainname == NULL)
		return (LWRES_R_FAILURE);

	return (LWRES_R_SUCCESS);
}

static lwres_result_t
lwres_conf_parsesearch(lwres_context_t *ctx,  FILE *fp) {
	int idx, delim;
	char word[LWRES_CONFMAXLINELEN];
	lwres_conf_t *confdata;

	confdata = &ctx->confdata;

	if (confdata->domainname != NULL) {
		/*
		 * Search and domain are mutually exclusive.
		 */
		CTXFREE(confdata->domainname,
			strlen(confdata->domainname) + 1);
		confdata->domainname = NULL;
	}

	/*
	 * Remove any previous search definitions.
	 */
	for (idx = 0; idx < LWRES_CONFMAXSEARCH; idx++) {
		if (confdata->search[idx] != NULL) {
			CTXFREE(confdata->search[idx],
				strlen(confdata->search[idx])+1);
			confdata->search[idx] = NULL;
		}
	}
	confdata->searchnxt = 0;

	delim = getword(fp, word, sizeof(word));
	if (strlen(word) == 0U)
		return (LWRES_R_FAILURE); /* Nothing else on line. */

	idx = 0;
	while (strlen(word) > 0U) {
		if (confdata->searchnxt == LWRES_CONFMAXSEARCH)
			goto ignore; /* Too many domains. */

		confdata->search[idx] = lwres_strdup(ctx, word);
		if (confdata->search[idx] == NULL)
			return (LWRES_R_FAILURE);
		idx++;
		confdata->searchnxt++;

	ignore:
		if (delim == EOF || delim == '\n')
			break;
		else
			delim = getword(fp, word, sizeof(word));
	}

	return (LWRES_R_SUCCESS);
}

static lwres_result_t
lwres_create_addr(const char *buffer, lwres_addr_t *addr, int convert_zero) {
	struct in_addr v4;
	struct in6_addr v6;

	if (lwres_net_aton(buffer, &v4) == 1) {
		if (convert_zero) {
			unsigned char zeroaddress[] = {0, 0, 0, 0};
			unsigned char loopaddress[] = {127, 0, 0, 1};
			if (memcmp(&v4, zeroaddress, 4) == 0)
				memcpy(&v4, loopaddress, 4);
		}
		addr->family = LWRES_ADDRTYPE_V4;
		addr->length = NS_INADDRSZ;
		memcpy((void *)addr->address, &v4, NS_INADDRSZ);

	} else if (lwres_net_pton(AF_INET6, buffer, &v6) == 1) {
		addr->family = LWRES_ADDRTYPE_V6;
		addr->length = NS_IN6ADDRSZ;
		memcpy((void *)addr->address, &v6, NS_IN6ADDRSZ);
	} else {
		return (LWRES_R_FAILURE); /* Unrecognised format. */
	}

	return (LWRES_R_SUCCESS);
}

static lwres_result_t
lwres_conf_parsesortlist(lwres_context_t *ctx,  FILE *fp) {
	int delim, res, idx;
	char word[LWRES_CONFMAXLINELEN];
	char *p;
	lwres_conf_t *confdata;

	confdata = &ctx->confdata;

	delim = getword(fp, word, sizeof(word));
	if (strlen(word) == 0U)
		return (LWRES_R_FAILURE); /* Empty line after keyword. */

	while (strlen(word) > 0U) {
		if (confdata->sortlistnxt == LWRES_CONFMAXSORTLIST)
			return (LWRES_R_FAILURE); /* Too many values. */

		p = strchr(word, '/');
		if (p != NULL)
			*p++ = '\0';

		idx = confdata->sortlistnxt;
		res = lwres_create_addr(word, &confdata->sortlist[idx].addr, 1);
		if (res != LWRES_R_SUCCESS)
			return (res);

		if (p != NULL) {
			res = lwres_create_addr(p,
						&confdata->sortlist[idx].mask,
						0);
			if (res != LWRES_R_SUCCESS)
				return (res);
		} else {
			/*
			 * Make up a mask.
			 */
			confdata->sortlist[idx].mask =
				confdata->sortlist[idx].addr;

			memset(&confdata->sortlist[idx].mask.address, 0xff,
			       confdata->sortlist[idx].addr.length);
		}

		confdata->sortlistnxt++;

		if (delim == EOF || delim == '\n')
			break;
		else
			delim = getword(fp, word, sizeof(word));
	}

	return (LWRES_R_SUCCESS);
}

static lwres_result_t
lwres_conf_parseoption(lwres_context_t *ctx,  FILE *fp) {
	int delim;
	long ndots;
	char *p;
	char word[LWRES_CONFMAXLINELEN];
	lwres_conf_t *confdata;

	REQUIRE(ctx != NULL);
	confdata = &ctx->confdata;

	delim = getword(fp, word, sizeof(word));
	if (strlen(word) == 0U)
		return (LWRES_R_FAILURE); /* Empty line after keyword. */

	while (strlen(word) > 0U) {
		if (strcmp("debug", word) == 0) {
			confdata->resdebug = 1;
		} else if (strcmp("no_tld_query", word) == 0) {
			confdata->no_tld_query = 1;
		} else if (strncmp("ndots:", word, 6) == 0) {
			ndots = strtol(word + 6, &p, 10);
			if (*p != '\0') /* Bad string. */
				return (LWRES_R_FAILURE);
			if (ndots < 0 || ndots > 0xff) /* Out of range. */
				return (LWRES_R_FAILURE);
			confdata->ndots = (lwres_uint8_t)ndots;
		}

		if (delim == EOF || delim == '\n')
			break;
		else
			delim = getword(fp, word, sizeof(word));
	}

	return (LWRES_R_SUCCESS);
}

/*% parses a file and fills in the data structure. */
lwres_result_t
lwres_conf_parse(lwres_context_t *ctx, const char *filename) {
	FILE *fp = NULL;
	char word[256];
	lwres_result_t rval, ret;
	lwres_conf_t *confdata;
	int stopchar;

	REQUIRE(ctx != NULL);
	confdata = &ctx->confdata;

	REQUIRE(filename != NULL);
	REQUIRE(strlen(filename) > 0U);
	REQUIRE(confdata != NULL);

	errno = 0;
	if ((fp = fopen(filename, "r")) == NULL)
		return (LWRES_R_NOTFOUND);

	ret = LWRES_R_SUCCESS;
	do {
		stopchar = getword(fp, word, sizeof(word));
		if (stopchar == EOF) {
			rval = LWRES_R_SUCCESS;
			break;
		}

		if (strlen(word) == 0U)
			rval = LWRES_R_SUCCESS;
		else if (strcmp(word, "nameserver") == 0)
			rval = lwres_conf_parsenameserver(ctx, fp);
		else if (strcmp(word, "lwserver") == 0)
			rval = lwres_conf_parselwserver(ctx, fp);
		else if (strcmp(word, "domain") == 0)
			rval = lwres_conf_parsedomain(ctx, fp);
		else if (strcmp(word, "search") == 0)
			rval = lwres_conf_parsesearch(ctx, fp);
		else if (strcmp(word, "sortlist") == 0)
			rval = lwres_conf_parsesortlist(ctx, fp);
		else if (strcmp(word, "options") == 0)
			rval = lwres_conf_parseoption(ctx, fp);
		else {
			/* unrecognised word. Ignore entire line */
			rval = LWRES_R_SUCCESS;
			stopchar = eatline(fp);
			if (stopchar == EOF) {
				break;
			}
		}
		if (ret == LWRES_R_SUCCESS && rval != LWRES_R_SUCCESS)
			ret = rval;
	} while (1);

	fclose(fp);

	return (ret);
}

/*% Prints the config data structure to the FILE. */
lwres_result_t
lwres_conf_print(lwres_context_t *ctx, FILE *fp) {
	int i;
	int af;
	char tmp[sizeof("ffff:ffff:ffff:ffff:ffff:ffff:255.255.255.255")];
	const char *p;
	lwres_conf_t *confdata;
	lwres_addr_t tmpaddr;

	REQUIRE(ctx != NULL);
	confdata = &ctx->confdata;

	REQUIRE(confdata->nsnext <= LWRES_CONFMAXNAMESERVERS);

	for (i = 0; i < confdata->nsnext; i++) {
		af = lwresaddr2af(confdata->nameservers[i].family);

		p = lwres_net_ntop(af, confdata->nameservers[i].address,
				   tmp, sizeof(tmp));
		if (p != tmp)
			return (LWRES_R_FAILURE);

		fprintf(fp, "nameserver %s\n", tmp);
	}

	for (i = 0; i < confdata->lwnext; i++) {
		af = lwresaddr2af(confdata->lwservers[i].family);

		p = lwres_net_ntop(af, confdata->lwservers[i].address,
				   tmp, sizeof(tmp));
		if (p != tmp)
			return (LWRES_R_FAILURE);

		fprintf(fp, "lwserver %s\n", tmp);
	}

	if (confdata->domainname != NULL) {
		fprintf(fp, "domain %s\n", confdata->domainname);
	} else if (confdata->searchnxt > 0) {
		REQUIRE(confdata->searchnxt <= LWRES_CONFMAXSEARCH);

		fprintf(fp, "search");
		for (i = 0; i < confdata->searchnxt; i++)
			fprintf(fp, " %s", confdata->search[i]);
		fputc('\n', fp);
	}

	REQUIRE(confdata->sortlistnxt <= LWRES_CONFMAXSORTLIST);

	if (confdata->sortlistnxt > 0) {
		fputs("sortlist", fp);
		for (i = 0; i < confdata->sortlistnxt; i++) {
			af = lwresaddr2af(confdata->sortlist[i].addr.family);

			p = lwres_net_ntop(af,
					   confdata->sortlist[i].addr.address,
					   tmp, sizeof(tmp));
			if (p != tmp)
				return (LWRES_R_FAILURE);

			fprintf(fp, " %s", tmp);

			tmpaddr = confdata->sortlist[i].mask;
			memset(&tmpaddr.address, 0xff, tmpaddr.length);

			if (memcmp(&tmpaddr.address,
				   confdata->sortlist[i].mask.address,
				   confdata->sortlist[i].mask.length) != 0) {
				af = lwresaddr2af(
					    confdata->sortlist[i].mask.family);
				p = lwres_net_ntop
					(af,
					 confdata->sortlist[i].mask.address,
					 tmp, sizeof(tmp));
				if (p != tmp)
					return (LWRES_R_FAILURE);

				fprintf(fp, "/%s", tmp);
			}
		}
		fputc('\n', fp);
	}

	if (confdata->resdebug)
		fprintf(fp, "options debug\n");

	if (confdata->ndots > 0)
		fprintf(fp, "options ndots:%d\n", confdata->ndots);

	if (confdata->no_tld_query)
		fprintf(fp, "options no_tld_query\n");

	return (LWRES_R_SUCCESS);
}

/*% Returns a pointer to the current config structure. */
lwres_conf_t *
lwres_conf_get(lwres_context_t *ctx) {
	REQUIRE(ctx != NULL);

	return (&ctx->confdata);
}
