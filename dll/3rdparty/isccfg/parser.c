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

/* $Id: parser.c,v 1.129 2008/09/25 04:02:39 tbox Exp $ */

/*! \file */

#include <config.h>

#include <isc/buffer.h>
#include <isc/dir.h>
#include <isc/formatcheck.h>
#include <isc/lex.h>
#include <isc/log.h>
#include <isc/mem.h>
#include <isc/net.h>
#include <isc/netaddr.h>
#include <isc/print.h>
#include <isc/string.h>
#include <isc/sockaddr.h>
#include <isc/netscope.h>
#include <isc/util.h>
#include <isc/symtab.h>

#include <isccfg/cfg.h>
#include <isccfg/grammar.h>
#include <isccfg/log.h>

/* Shorthand */
#define CAT CFG_LOGCATEGORY_CONFIG
#define MOD CFG_LOGMODULE_PARSER

#define MAP_SYM 1 	/* Unique type for isc_symtab */

#define TOKEN_STRING(pctx) (pctx->token.value.as_textregion.base)

/* Check a return value. */
#define CHECK(op) 						\
	do { result = (op); 					\
		if (result != ISC_R_SUCCESS) goto cleanup; 	\
	} while (0)

/* Clean up a configuration object if non-NULL. */
#define CLEANUP_OBJ(obj) \
	do { if ((obj) != NULL) cfg_obj_destroy(pctx, &(obj)); } while (0)


/*
 * Forward declarations of static functions.
 */

static void
free_tuple(cfg_parser_t *pctx, cfg_obj_t *obj);

static isc_result_t
parse_list(cfg_parser_t *pctx, const cfg_type_t *type, cfg_obj_t **ret);

static void
print_list(cfg_printer_t *pctx, const cfg_obj_t *obj);

static void
free_list(cfg_parser_t *pctx, cfg_obj_t *obj);

static isc_result_t
create_listelt(cfg_parser_t *pctx, cfg_listelt_t **eltp);

static isc_result_t
create_string(cfg_parser_t *pctx, const char *contents, const cfg_type_t *type,
	      cfg_obj_t **ret);

static void
free_string(cfg_parser_t *pctx, cfg_obj_t *obj);

static isc_result_t
create_map(cfg_parser_t *pctx, const cfg_type_t *type, cfg_obj_t **objp);

static void
free_map(cfg_parser_t *pctx, cfg_obj_t *obj);

static isc_result_t
parse_symtab_elt(cfg_parser_t *pctx, const char *name,
		 cfg_type_t *elttype, isc_symtab_t *symtab,
		 isc_boolean_t callback);

static void
free_noop(cfg_parser_t *pctx, cfg_obj_t *obj);

static isc_result_t
cfg_getstringtoken(cfg_parser_t *pctx);

static void
parser_complain(cfg_parser_t *pctx, isc_boolean_t is_warning,
		unsigned int flags, const char *format, va_list args);

/*
 * Data representations.  These correspond to members of the
 * "value" union in struct cfg_obj (except "void", which does
 * not need a union member).
 */

cfg_rep_t cfg_rep_uint32 = { "uint32", free_noop };
cfg_rep_t cfg_rep_uint64 = { "uint64", free_noop };
cfg_rep_t cfg_rep_string = { "string", free_string };
cfg_rep_t cfg_rep_boolean = { "boolean", free_noop };
cfg_rep_t cfg_rep_map = { "map", free_map };
cfg_rep_t cfg_rep_list = { "list", free_list };
cfg_rep_t cfg_rep_tuple = { "tuple", free_tuple };
cfg_rep_t cfg_rep_sockaddr = { "sockaddr", free_noop };
cfg_rep_t cfg_rep_netprefix = { "netprefix", free_noop };
cfg_rep_t cfg_rep_void = { "void", free_noop };

/*
 * Configuration type definitions.
 */

/*%
 * An implicit list.  These are formed by clauses that occur multiple times.
 */
static cfg_type_t cfg_type_implicitlist = {
	"implicitlist", NULL, print_list, NULL, &cfg_rep_list, NULL };

/* Functions. */

void
cfg_print_obj(cfg_printer_t *pctx, const cfg_obj_t *obj) {
	obj->type->print(pctx, obj);
}

void
cfg_print_chars(cfg_printer_t *pctx, const char *text, int len) {
	pctx->f(pctx->closure, text, len);
}

static void
print_open(cfg_printer_t *pctx) {
	cfg_print_chars(pctx, "{\n", 2);
	pctx->indent++;
}

static void
print_indent(cfg_printer_t *pctx) {
	int indent = pctx->indent;
	while (indent > 0) {
		cfg_print_chars(pctx, "\t", 1);
		indent--;
	}
}

static void
print_close(cfg_printer_t *pctx) {
	pctx->indent--;
	print_indent(pctx);
	cfg_print_chars(pctx, "}", 1);
}

isc_result_t
cfg_parse_obj(cfg_parser_t *pctx, const cfg_type_t *type, cfg_obj_t **ret) {
	isc_result_t result;
	INSIST(ret != NULL && *ret == NULL);
	result = type->parse(pctx, type, ret);
	if (result != ISC_R_SUCCESS)
		return (result);
	INSIST(*ret != NULL);
	return (ISC_R_SUCCESS);
}

void
cfg_print(const cfg_obj_t *obj,
	  void (*f)(void *closure, const char *text, int textlen),
	  void *closure)
{
	cfg_printer_t pctx;
	pctx.f = f;
	pctx.closure = closure;
	pctx.indent = 0;
	obj->type->print(&pctx, obj);
}


/* Tuples. */

isc_result_t
cfg_create_tuple(cfg_parser_t *pctx, const cfg_type_t *type, cfg_obj_t **ret) {
	isc_result_t result;
	const cfg_tuplefielddef_t *fields = type->of;
	const cfg_tuplefielddef_t *f;
	cfg_obj_t *obj = NULL;
	unsigned int nfields = 0;
	int i;

	for (f = fields; f->name != NULL; f++)
		nfields++;

	CHECK(cfg_create_obj(pctx, type, &obj));
	obj->value.tuple = isc_mem_get(pctx->mctx,
				       nfields * sizeof(cfg_obj_t *));
	if (obj->value.tuple == NULL) {
		result = ISC_R_NOMEMORY;
		goto cleanup;
	}
	for (f = fields, i = 0; f->name != NULL; f++, i++)
		obj->value.tuple[i] = NULL;
	*ret = obj;
	return (ISC_R_SUCCESS);

 cleanup:
	if (obj != NULL)
		isc_mem_put(pctx->mctx, obj, sizeof(*obj));
	return (result);
}

isc_result_t
cfg_parse_tuple(cfg_parser_t *pctx, const cfg_type_t *type, cfg_obj_t **ret)
{
	isc_result_t result;
	const cfg_tuplefielddef_t *fields = type->of;
	const cfg_tuplefielddef_t *f;
	cfg_obj_t *obj = NULL;
	unsigned int i;

	CHECK(cfg_create_tuple(pctx, type, &obj));
	for (f = fields, i = 0; f->name != NULL; f++, i++)
		CHECK(cfg_parse_obj(pctx, f->type, &obj->value.tuple[i]));

	*ret = obj;
	return (ISC_R_SUCCESS);

 cleanup:
	CLEANUP_OBJ(obj);
	return (result);
}

void
cfg_print_tuple(cfg_printer_t *pctx, const cfg_obj_t *obj) {
	unsigned int i;
	const cfg_tuplefielddef_t *fields = obj->type->of;
	const cfg_tuplefielddef_t *f;
	isc_boolean_t need_space = ISC_FALSE;

	for (f = fields, i = 0; f->name != NULL; f++, i++) {
		const cfg_obj_t *fieldobj = obj->value.tuple[i];
		if (need_space)
			cfg_print_chars(pctx, " ", 1);
		cfg_print_obj(pctx, fieldobj);
		need_space = ISC_TF(fieldobj->type->print != cfg_print_void);
	}
}

void
cfg_doc_tuple(cfg_printer_t *pctx, const cfg_type_t *type) {
	const cfg_tuplefielddef_t *fields = type->of;
	const cfg_tuplefielddef_t *f;
	isc_boolean_t need_space = ISC_FALSE;

	for (f = fields; f->name != NULL; f++) {
		if (need_space)
			cfg_print_chars(pctx, " ", 1);
		cfg_doc_obj(pctx, f->type);
		need_space = ISC_TF(f->type->print != cfg_print_void);
	}
}

static void
free_tuple(cfg_parser_t *pctx, cfg_obj_t *obj) {
	unsigned int i;
	const cfg_tuplefielddef_t *fields = obj->type->of;
	const cfg_tuplefielddef_t *f;
	unsigned int nfields = 0;

	if (obj->value.tuple == NULL)
		return;

	for (f = fields, i = 0; f->name != NULL; f++, i++) {
		CLEANUP_OBJ(obj->value.tuple[i]);
		nfields++;
	}
	isc_mem_put(pctx->mctx, obj->value.tuple,
		    nfields * sizeof(cfg_obj_t *));
}

isc_boolean_t
cfg_obj_istuple(const cfg_obj_t *obj) {
	REQUIRE(obj != NULL);
	return (ISC_TF(obj->type->rep == &cfg_rep_tuple));
}

const cfg_obj_t *
cfg_tuple_get(const cfg_obj_t *tupleobj, const char* name) {
	unsigned int i;
	const cfg_tuplefielddef_t *fields;
	const cfg_tuplefielddef_t *f;

	REQUIRE(tupleobj != NULL && tupleobj->type->rep == &cfg_rep_tuple);

	fields = tupleobj->type->of;
	for (f = fields, i = 0; f->name != NULL; f++, i++) {
		if (strcmp(f->name, name) == 0)
			return (tupleobj->value.tuple[i]);
	}
	INSIST(0);
	return (NULL);
}

isc_result_t
cfg_parse_special(cfg_parser_t *pctx, int special) {
	isc_result_t result;
	CHECK(cfg_gettoken(pctx, 0));
	if (pctx->token.type == isc_tokentype_special &&
	    pctx->token.value.as_char == special)
		return (ISC_R_SUCCESS);

	cfg_parser_error(pctx, CFG_LOG_NEAR, "'%c' expected", special);
	return (ISC_R_UNEXPECTEDTOKEN);
 cleanup:
	return (result);
}

/*
 * Parse a required semicolon.  If it is not there, log
 * an error and increment the error count but continue
 * parsing.  Since the next token is pushed back,
 * care must be taken to make sure it is eventually
 * consumed or an infinite loop may result.
 */
static isc_result_t
parse_semicolon(cfg_parser_t *pctx) {
	isc_result_t result;
	CHECK(cfg_gettoken(pctx, 0));
	if (pctx->token.type == isc_tokentype_special &&
	    pctx->token.value.as_char == ';')
		return (ISC_R_SUCCESS);

	cfg_parser_error(pctx, CFG_LOG_BEFORE, "missing ';'");
	cfg_ungettoken(pctx);
 cleanup:
	return (result);
}

/*
 * Parse EOF, logging and returning an error if not there.
 */
static isc_result_t
parse_eof(cfg_parser_t *pctx) {
	isc_result_t result;
	CHECK(cfg_gettoken(pctx, 0));

	if (pctx->token.type == isc_tokentype_eof)
		return (ISC_R_SUCCESS);

	cfg_parser_error(pctx, CFG_LOG_NEAR, "syntax error");
	return (ISC_R_UNEXPECTEDTOKEN);
 cleanup:
	return (result);
}

/* A list of files, used internally for pctx->files. */

static cfg_type_t cfg_type_filelist = {
	"filelist", NULL, print_list, NULL, &cfg_rep_list,
	&cfg_type_qstring
};

isc_result_t
cfg_parser_create(isc_mem_t *mctx, isc_log_t *lctx, cfg_parser_t **ret) {
	isc_result_t result;
	cfg_parser_t *pctx;
	isc_lexspecials_t specials;

	REQUIRE(mctx != NULL);
	REQUIRE(ret != NULL && *ret == NULL);

	pctx = isc_mem_get(mctx, sizeof(*pctx));
	if (pctx == NULL)
		return (ISC_R_NOMEMORY);

	pctx->mctx = mctx;
	pctx->lctx = lctx;
	pctx->lexer = NULL;
	pctx->seen_eof = ISC_FALSE;
	pctx->ungotten = ISC_FALSE;
	pctx->errors = 0;
	pctx->warnings = 0;
	pctx->open_files = NULL;
	pctx->closed_files = NULL;
	pctx->line = 0;
	pctx->callback = NULL;
	pctx->callbackarg = NULL;
	pctx->token.type = isc_tokentype_unknown;

	memset(specials, 0, sizeof(specials));
	specials['{'] = 1;
	specials['}'] = 1;
	specials[';'] = 1;
	specials['/'] = 1;
	specials['"'] = 1;
	specials['!'] = 1;

	CHECK(isc_lex_create(pctx->mctx, 1024, &pctx->lexer));

	isc_lex_setspecials(pctx->lexer, specials);
	isc_lex_setcomments(pctx->lexer, (ISC_LEXCOMMENT_C |
					 ISC_LEXCOMMENT_CPLUSPLUS |
					 ISC_LEXCOMMENT_SHELL));

	CHECK(cfg_create_list(pctx, &cfg_type_filelist, &pctx->open_files));
	CHECK(cfg_create_list(pctx, &cfg_type_filelist, &pctx->closed_files));

	*ret = pctx;
	return (ISC_R_SUCCESS);

 cleanup:
	if (pctx->lexer != NULL)
		isc_lex_destroy(&pctx->lexer);
	CLEANUP_OBJ(pctx->open_files);
	CLEANUP_OBJ(pctx->closed_files);
	isc_mem_put(mctx, pctx, sizeof(*pctx));
	return (result);
}

static isc_result_t
parser_openfile(cfg_parser_t *pctx, const char *filename) {
	isc_result_t result;
	cfg_listelt_t *elt = NULL;
	cfg_obj_t *stringobj = NULL;

	result = isc_lex_openfile(pctx->lexer, filename);
	if (result != ISC_R_SUCCESS) {
		cfg_parser_error(pctx, 0, "open: %s: %s",
			     filename, isc_result_totext(result));
		goto cleanup;
	}

	CHECK(create_string(pctx, filename, &cfg_type_qstring, &stringobj));
	CHECK(create_listelt(pctx, &elt));
	elt->obj = stringobj;
	ISC_LIST_APPEND(pctx->open_files->value.list, elt, link);

	return (ISC_R_SUCCESS);
 cleanup:
	CLEANUP_OBJ(stringobj);
	return (result);
}

void
cfg_parser_setcallback(cfg_parser_t *pctx,
		       cfg_parsecallback_t callback,
		       void *arg)
{
	pctx->callback = callback;
	pctx->callbackarg = arg;
}

/*
 * Parse a configuration using a pctx where a lexer has already
 * been set up with a source.
 */
static isc_result_t
parse2(cfg_parser_t *pctx, const cfg_type_t *type, cfg_obj_t **ret) {
	isc_result_t result;
	cfg_obj_t *obj = NULL;

	result = cfg_parse_obj(pctx, type, &obj);

	if (pctx->errors != 0) {
		/* Errors have been logged. */
		if (result == ISC_R_SUCCESS)
			result = ISC_R_FAILURE;
		goto cleanup;
	}

	if (result != ISC_R_SUCCESS) {
		/* Parsing failed but no errors have been logged. */
		cfg_parser_error(pctx, 0, "parsing failed");
		goto cleanup;
	}

	CHECK(parse_eof(pctx));

	*ret = obj;
	return (ISC_R_SUCCESS);

 cleanup:
	CLEANUP_OBJ(obj);
	return (result);
}

isc_result_t
cfg_parse_file(cfg_parser_t *pctx, const char *filename,
	       const cfg_type_t *type, cfg_obj_t **ret)
{
	isc_result_t result;

	REQUIRE(filename != NULL);

	CHECK(parser_openfile(pctx, filename));
	CHECK(parse2(pctx, type, ret));
 cleanup:
	return (result);
}


isc_result_t
cfg_parse_buffer(cfg_parser_t *pctx, isc_buffer_t *buffer,
	const cfg_type_t *type, cfg_obj_t **ret)
{
	isc_result_t result;
	REQUIRE(buffer != NULL);
	CHECK(isc_lex_openbuffer(pctx->lexer, buffer));
	CHECK(parse2(pctx, type, ret));
 cleanup:
	return (result);
}

void
cfg_parser_destroy(cfg_parser_t **pctxp) {
	cfg_parser_t *pctx = *pctxp;
	isc_lex_destroy(&pctx->lexer);
	/*
	 * Cleaning up open_files does not
	 * close the files; that was already done
	 * by closing the lexer.
	 */
	CLEANUP_OBJ(pctx->open_files);
	CLEANUP_OBJ(pctx->closed_files);
	isc_mem_put(pctx->mctx, pctx, sizeof(*pctx));
	*pctxp = NULL;
}

/*
 * void
 */
isc_result_t
cfg_parse_void(cfg_parser_t *pctx, const cfg_type_t *type, cfg_obj_t **ret) {
	UNUSED(type);
	return (cfg_create_obj(pctx, &cfg_type_void, ret));
}

void
cfg_print_void(cfg_printer_t *pctx, const cfg_obj_t *obj) {
	UNUSED(pctx);
	UNUSED(obj);
}

void
cfg_doc_void(cfg_printer_t *pctx, const cfg_type_t *type) {
	UNUSED(pctx);
	UNUSED(type);
}

isc_boolean_t
cfg_obj_isvoid(const cfg_obj_t *obj) {
	REQUIRE(obj != NULL);
	return (ISC_TF(obj->type->rep == &cfg_rep_void));
}

cfg_type_t cfg_type_void = {
	"void", cfg_parse_void, cfg_print_void, cfg_doc_void, &cfg_rep_void,
	NULL };


/*
 * uint32
 */
isc_result_t
cfg_parse_uint32(cfg_parser_t *pctx, const cfg_type_t *type, cfg_obj_t **ret) {
	isc_result_t result;
	cfg_obj_t *obj = NULL;
	UNUSED(type);

	CHECK(cfg_gettoken(pctx, ISC_LEXOPT_NUMBER | ISC_LEXOPT_CNUMBER));
	if (pctx->token.type != isc_tokentype_number) {
		cfg_parser_error(pctx, CFG_LOG_NEAR, "expected number");
		return (ISC_R_UNEXPECTEDTOKEN);
	}

	CHECK(cfg_create_obj(pctx, &cfg_type_uint32, &obj));

	obj->value.uint32 = pctx->token.value.as_ulong;
	*ret = obj;
 cleanup:
	return (result);
}

void
cfg_print_cstr(cfg_printer_t *pctx, const char *s) {
	cfg_print_chars(pctx, s, strlen(s));
}

void
cfg_print_rawuint(cfg_printer_t *pctx, unsigned int u) {
	char buf[32];
	snprintf(buf, sizeof(buf), "%u", u);
	cfg_print_cstr(pctx, buf);
}

void
cfg_print_uint32(cfg_printer_t *pctx, const cfg_obj_t *obj) {
	cfg_print_rawuint(pctx, obj->value.uint32);
}

isc_boolean_t
cfg_obj_isuint32(const cfg_obj_t *obj) {
	REQUIRE(obj != NULL);
	return (ISC_TF(obj->type->rep == &cfg_rep_uint32));
}

isc_uint32_t
cfg_obj_asuint32(const cfg_obj_t *obj) {
	REQUIRE(obj != NULL && obj->type->rep == &cfg_rep_uint32);
	return (obj->value.uint32);
}

cfg_type_t cfg_type_uint32 = {
	"integer", cfg_parse_uint32, cfg_print_uint32, cfg_doc_terminal,
	&cfg_rep_uint32, NULL
};


/*
 * uint64
 */
isc_boolean_t
cfg_obj_isuint64(const cfg_obj_t *obj) {
	REQUIRE(obj != NULL);
	return (ISC_TF(obj->type->rep == &cfg_rep_uint64));
}

isc_uint64_t
cfg_obj_asuint64(const cfg_obj_t *obj) {
	REQUIRE(obj != NULL && obj->type->rep == &cfg_rep_uint64);
	return (obj->value.uint64);
}

void
cfg_print_uint64(cfg_printer_t *pctx, const cfg_obj_t *obj) {
	char buf[32];
	snprintf(buf, sizeof(buf), "%" ISC_PRINT_QUADFORMAT "u",
		 obj->value.uint64);
	cfg_print_cstr(pctx, buf);
}

cfg_type_t cfg_type_uint64 = {
	"64_bit_integer", NULL, cfg_print_uint64, cfg_doc_terminal,
	&cfg_rep_uint64, NULL
};

/*
 * qstring (quoted string), ustring (unquoted string), astring
 * (any string)
 */

/* Create a string object from a null-terminated C string. */
static isc_result_t
create_string(cfg_parser_t *pctx, const char *contents, const cfg_type_t *type,
	      cfg_obj_t **ret)
{
	isc_result_t result;
	cfg_obj_t *obj = NULL;
	int len;

	CHECK(cfg_create_obj(pctx, type, &obj));
	len = strlen(contents);
	obj->value.string.length = len;
	obj->value.string.base = isc_mem_get(pctx->mctx, len + 1);
	if (obj->value.string.base == 0) {
		isc_mem_put(pctx->mctx, obj, sizeof(*obj));
		return (ISC_R_NOMEMORY);
	}
	memcpy(obj->value.string.base, contents, len);
	obj->value.string.base[len] = '\0';

	*ret = obj;
 cleanup:
	return (result);
}

isc_result_t
cfg_parse_qstring(cfg_parser_t *pctx, const cfg_type_t *type, cfg_obj_t **ret) {
	isc_result_t result;
	UNUSED(type);

	CHECK(cfg_gettoken(pctx, CFG_LEXOPT_QSTRING));
	if (pctx->token.type != isc_tokentype_qstring) {
		cfg_parser_error(pctx, CFG_LOG_NEAR, "expected quoted string");
		return (ISC_R_UNEXPECTEDTOKEN);
	}
	return (create_string(pctx,
			      TOKEN_STRING(pctx),
			      &cfg_type_qstring,
			      ret));
 cleanup:
	return (result);
}

static isc_result_t
parse_ustring(cfg_parser_t *pctx, const cfg_type_t *type, cfg_obj_t **ret) {
	isc_result_t result;
	UNUSED(type);

	CHECK(cfg_gettoken(pctx, 0));
	if (pctx->token.type != isc_tokentype_string) {
		cfg_parser_error(pctx, CFG_LOG_NEAR, "expected unquoted string");
		return (ISC_R_UNEXPECTEDTOKEN);
	}
	return (create_string(pctx,
			      TOKEN_STRING(pctx),
			      &cfg_type_ustring,
			      ret));
 cleanup:
	return (result);
}

isc_result_t
cfg_parse_astring(cfg_parser_t *pctx, const cfg_type_t *type,
		  cfg_obj_t **ret)
{
	isc_result_t result;
	UNUSED(type);

	CHECK(cfg_getstringtoken(pctx));
	return (create_string(pctx,
			      TOKEN_STRING(pctx),
			      &cfg_type_qstring,
			      ret));
 cleanup:
	return (result);
}

isc_boolean_t
cfg_is_enum(const char *s, const char *const *enums) {
	const char * const *p;
	for (p = enums; *p != NULL; p++) {
		if (strcasecmp(*p, s) == 0)
			return (ISC_TRUE);
	}
	return (ISC_FALSE);
}

static isc_result_t
check_enum(cfg_parser_t *pctx, cfg_obj_t *obj, const char *const *enums) {
	const char *s = obj->value.string.base;
	if (cfg_is_enum(s, enums))
		return (ISC_R_SUCCESS);
	cfg_parser_error(pctx, 0, "'%s' unexpected", s);
	return (ISC_R_UNEXPECTEDTOKEN);
}

isc_result_t
cfg_parse_enum(cfg_parser_t *pctx, const cfg_type_t *type, cfg_obj_t **ret) {
	isc_result_t result;
	cfg_obj_t *obj = NULL;
	CHECK(parse_ustring(pctx, NULL, &obj));
	CHECK(check_enum(pctx, obj, type->of));
	*ret = obj;
	return (ISC_R_SUCCESS);
 cleanup:
	CLEANUP_OBJ(obj);
	return (result);
}

void
cfg_doc_enum(cfg_printer_t *pctx, const cfg_type_t *type) {
	const char * const *p;
	cfg_print_chars(pctx, "( ", 2);
	for (p = type->of; *p != NULL; p++) {
		cfg_print_cstr(pctx, *p);
		if (p[1] != NULL)
			cfg_print_chars(pctx, " | ", 3);
	}
	cfg_print_chars(pctx, " )", 2);
}

void
cfg_print_ustring(cfg_printer_t *pctx, const cfg_obj_t *obj) {
	cfg_print_chars(pctx, obj->value.string.base, obj->value.string.length);
}

static void
print_qstring(cfg_printer_t *pctx, const cfg_obj_t *obj) {
	cfg_print_chars(pctx, "\"", 1);
	cfg_print_ustring(pctx, obj);
	cfg_print_chars(pctx, "\"", 1);
}

static void
free_string(cfg_parser_t *pctx, cfg_obj_t *obj) {
	isc_mem_put(pctx->mctx, obj->value.string.base,
		    obj->value.string.length + 1);
}

isc_boolean_t
cfg_obj_isstring(const cfg_obj_t *obj) {
	REQUIRE(obj != NULL);
	return (ISC_TF(obj->type->rep == &cfg_rep_string));
}

const char *
cfg_obj_asstring(const cfg_obj_t *obj) {
	REQUIRE(obj != NULL && obj->type->rep == &cfg_rep_string);
	return (obj->value.string.base);
}

/* Quoted string only */
cfg_type_t cfg_type_qstring = {
	"quoted_string", cfg_parse_qstring, print_qstring, cfg_doc_terminal,
	&cfg_rep_string, NULL
};

/* Unquoted string only */
cfg_type_t cfg_type_ustring = {
	"string", parse_ustring, cfg_print_ustring, cfg_doc_terminal,
	&cfg_rep_string, NULL
};

/* Any string (quoted or unquoted); printed with quotes */
cfg_type_t cfg_type_astring = {
	"string", cfg_parse_astring, print_qstring, cfg_doc_terminal,
	&cfg_rep_string, NULL
};

/*
 * Booleans
 */

isc_boolean_t
cfg_obj_isboolean(const cfg_obj_t *obj) {
	REQUIRE(obj != NULL);
	return (ISC_TF(obj->type->rep == &cfg_rep_boolean));
}

isc_boolean_t
cfg_obj_asboolean(const cfg_obj_t *obj) {
	REQUIRE(obj != NULL && obj->type->rep == &cfg_rep_boolean);
	return (obj->value.boolean);
}

static isc_result_t
parse_boolean(cfg_parser_t *pctx, const cfg_type_t *type, cfg_obj_t **ret)
{
	isc_result_t result;
	isc_boolean_t value;
	cfg_obj_t *obj = NULL;
	UNUSED(type);

	result = cfg_gettoken(pctx, 0);
	if (result != ISC_R_SUCCESS)
		return (result);

	if (pctx->token.type != isc_tokentype_string)
		goto bad_boolean;

	if ((strcasecmp(TOKEN_STRING(pctx), "true") == 0) ||
	    (strcasecmp(TOKEN_STRING(pctx), "yes") == 0) ||
	    (strcmp(TOKEN_STRING(pctx), "1") == 0)) {
		value = ISC_TRUE;
	} else if ((strcasecmp(TOKEN_STRING(pctx), "false") == 0) ||
		   (strcasecmp(TOKEN_STRING(pctx), "no") == 0) ||
		   (strcmp(TOKEN_STRING(pctx), "0") == 0)) {
		value = ISC_FALSE;
	} else {
		goto bad_boolean;
	}

	CHECK(cfg_create_obj(pctx, &cfg_type_boolean, &obj));
	obj->value.boolean = value;
	*ret = obj;
	return (result);

 bad_boolean:
	cfg_parser_error(pctx, CFG_LOG_NEAR, "boolean expected");
	return (ISC_R_UNEXPECTEDTOKEN);

 cleanup:
	return (result);
}

static void
print_boolean(cfg_printer_t *pctx, const cfg_obj_t *obj) {
	if (obj->value.boolean)
		cfg_print_chars(pctx, "yes", 3);
	else
		cfg_print_chars(pctx, "no", 2);
}

cfg_type_t cfg_type_boolean = {
	"boolean", parse_boolean, print_boolean, cfg_doc_terminal,
	&cfg_rep_boolean, NULL
};

/*
 * Lists.
 */

isc_result_t
cfg_create_list(cfg_parser_t *pctx, const cfg_type_t *type, cfg_obj_t **obj) {
	isc_result_t result;
	CHECK(cfg_create_obj(pctx, type, obj));
	ISC_LIST_INIT((*obj)->value.list);
 cleanup:
	return (result);
}

static isc_result_t
create_listelt(cfg_parser_t *pctx, cfg_listelt_t **eltp) {
	cfg_listelt_t *elt;
	elt = isc_mem_get(pctx->mctx, sizeof(*elt));
	if (elt == NULL)
		return (ISC_R_NOMEMORY);
	elt->obj = NULL;
	ISC_LINK_INIT(elt, link);
	*eltp = elt;
	return (ISC_R_SUCCESS);
}

static void
free_list_elt(cfg_parser_t *pctx, cfg_listelt_t *elt) {
	cfg_obj_destroy(pctx, &elt->obj);
	isc_mem_put(pctx->mctx, elt, sizeof(*elt));
}

static void
free_list(cfg_parser_t *pctx, cfg_obj_t *obj) {
	cfg_listelt_t *elt, *next;
	for (elt = ISC_LIST_HEAD(obj->value.list);
	     elt != NULL;
	     elt = next)
	{
		next = ISC_LIST_NEXT(elt, link);
		free_list_elt(pctx, elt);
	}
}

isc_result_t
cfg_parse_listelt(cfg_parser_t *pctx, const cfg_type_t *elttype,
		  cfg_listelt_t **ret)
{
	isc_result_t result;
	cfg_listelt_t *elt = NULL;
	cfg_obj_t *value = NULL;

	CHECK(create_listelt(pctx, &elt));

	result = cfg_parse_obj(pctx, elttype, &value);
	if (result != ISC_R_SUCCESS)
		goto cleanup;

	elt->obj = value;

	*ret = elt;
	return (ISC_R_SUCCESS);

 cleanup:
	isc_mem_put(pctx->mctx, elt, sizeof(*elt));
	return (result);
}

/*
 * Parse a homogeneous list whose elements are of type 'elttype'
 * and where each element is terminated by a semicolon.
 */
static isc_result_t
parse_list(cfg_parser_t *pctx, const cfg_type_t *listtype, cfg_obj_t **ret)
{
	cfg_obj_t *listobj = NULL;
	const cfg_type_t *listof = listtype->of;
	isc_result_t result;
	cfg_listelt_t *elt = NULL;

	CHECK(cfg_create_list(pctx, listtype, &listobj));

	for (;;) {
		CHECK(cfg_peektoken(pctx, 0));
		if (pctx->token.type == isc_tokentype_special &&
		    pctx->token.value.as_char == /*{*/ '}')
			break;
		CHECK(cfg_parse_listelt(pctx, listof, &elt));
		CHECK(parse_semicolon(pctx));
		ISC_LIST_APPEND(listobj->value.list, elt, link);
		elt = NULL;
	}
	*ret = listobj;
	return (ISC_R_SUCCESS);

 cleanup:
	if (elt != NULL)
		free_list_elt(pctx, elt);
	CLEANUP_OBJ(listobj);
	return (result);
}

static void
print_list(cfg_printer_t *pctx, const cfg_obj_t *obj) {
	const cfg_list_t *list = &obj->value.list;
	const cfg_listelt_t *elt;

	for (elt = ISC_LIST_HEAD(*list);
	     elt != NULL;
	     elt = ISC_LIST_NEXT(elt, link)) {
		print_indent(pctx);
		cfg_print_obj(pctx, elt->obj);
		cfg_print_chars(pctx, ";\n", 2);
	}
}

isc_result_t
cfg_parse_bracketed_list(cfg_parser_t *pctx, const cfg_type_t *type,
		     cfg_obj_t **ret)
{
	isc_result_t result;
	CHECK(cfg_parse_special(pctx, '{'));
	CHECK(parse_list(pctx, type, ret));
	CHECK(cfg_parse_special(pctx, '}'));
 cleanup:
	return (result);
}

void
cfg_print_bracketed_list(cfg_printer_t *pctx, const cfg_obj_t *obj) {
	print_open(pctx);
	print_list(pctx, obj);
	print_close(pctx);
}

void
cfg_doc_bracketed_list(cfg_printer_t *pctx, const cfg_type_t *type) {
	cfg_print_chars(pctx, "{ ", 2);
	cfg_doc_obj(pctx, type->of);
	cfg_print_chars(pctx, "; ... }", 7);
}

/*
 * Parse a homogeneous list whose elements are of type 'elttype'
 * and where elements are separated by space.  The list ends
 * before the first semicolon.
 */
isc_result_t
cfg_parse_spacelist(cfg_parser_t *pctx, const cfg_type_t *listtype,
		    cfg_obj_t **ret)
{
	cfg_obj_t *listobj = NULL;
	const cfg_type_t *listof = listtype->of;
	isc_result_t result;

	CHECK(cfg_create_list(pctx, listtype, &listobj));

	for (;;) {
		cfg_listelt_t *elt = NULL;

		CHECK(cfg_peektoken(pctx, 0));
		if (pctx->token.type == isc_tokentype_special &&
		    pctx->token.value.as_char == ';')
			break;
		CHECK(cfg_parse_listelt(pctx, listof, &elt));
		ISC_LIST_APPEND(listobj->value.list, elt, link);
	}
	*ret = listobj;
	return (ISC_R_SUCCESS);

 cleanup:
	CLEANUP_OBJ(listobj);
	return (result);
}

void
cfg_print_spacelist(cfg_printer_t *pctx, const cfg_obj_t *obj) {
	const cfg_list_t *list = &obj->value.list;
	const cfg_listelt_t *elt;

	for (elt = ISC_LIST_HEAD(*list);
	     elt != NULL;
	     elt = ISC_LIST_NEXT(elt, link)) {
		cfg_print_obj(pctx, elt->obj);
		if (ISC_LIST_NEXT(elt, link) != NULL)
			cfg_print_chars(pctx, " ", 1);
	}
}

isc_boolean_t
cfg_obj_islist(const cfg_obj_t *obj) {
	REQUIRE(obj != NULL);
	return (ISC_TF(obj->type->rep == &cfg_rep_list));
}

const cfg_listelt_t *
cfg_list_first(const cfg_obj_t *obj) {
	REQUIRE(obj == NULL || obj->type->rep == &cfg_rep_list);
	if (obj == NULL)
		return (NULL);
	return (ISC_LIST_HEAD(obj->value.list));
}

const cfg_listelt_t *
cfg_list_next(const cfg_listelt_t *elt) {
	REQUIRE(elt != NULL);
	return (ISC_LIST_NEXT(elt, link));
}

/*
 * Return the length of a list object.  If obj is NULL or is not
 * a list, return 0.
 */
unsigned int
cfg_list_length(const cfg_obj_t *obj, isc_boolean_t recurse) {
	const cfg_listelt_t *elt;
	unsigned int count = 0;

	if (obj == NULL || !cfg_obj_islist(obj))
		return (0U);
	for (elt = cfg_list_first(obj);
	     elt != NULL;
	     elt = cfg_list_next(elt)) {
		if (recurse && cfg_obj_islist(elt->obj)) {
			count += cfg_list_length(elt->obj, recurse);
		} else {
			count++;
		}
	}
	return (count);
}

const cfg_obj_t *
cfg_listelt_value(const cfg_listelt_t *elt) {
	REQUIRE(elt != NULL);
	return (elt->obj);
}

/*
 * Maps.
 */

/*
 * Parse a map body.  That's something like
 *
 *   "foo 1; bar { glub; }; zap true; zap false;"
 *
 * i.e., a sequence of option names followed by values and
 * terminated by semicolons.  Used for the top level of
 * the named.conf syntax, as well as for the body of the
 * options, view, zone, and other statements.
 */
isc_result_t
cfg_parse_mapbody(cfg_parser_t *pctx, const cfg_type_t *type, cfg_obj_t **ret)
{
	const cfg_clausedef_t * const *clausesets = type->of;
	isc_result_t result;
	const cfg_clausedef_t * const *clauseset;
	const cfg_clausedef_t *clause;
	cfg_obj_t *value = NULL;
	cfg_obj_t *obj = NULL;
	cfg_obj_t *eltobj = NULL;
	cfg_obj_t *includename = NULL;
	isc_symvalue_t symval;
	cfg_list_t *list = NULL;

	CHECK(create_map(pctx, type, &obj));

	obj->value.map.clausesets = clausesets;

	for (;;) {
		cfg_listelt_t *elt;

	redo:
		/*
		 * Parse the option name and see if it is known.
		 */
		CHECK(cfg_gettoken(pctx, 0));

		if (pctx->token.type != isc_tokentype_string) {
			cfg_ungettoken(pctx);
			break;
		}

		/*
		 * We accept "include" statements wherever a map body
		 * clause can occur.
		 */
		if (strcasecmp(TOKEN_STRING(pctx), "include") == 0) {
			/*
			 * Turn the file name into a temporary configuration
			 * object just so that it is not overwritten by the
			 * semicolon token.
			 */
			CHECK(cfg_parse_obj(pctx, &cfg_type_qstring, &includename));
			CHECK(parse_semicolon(pctx));
			CHECK(parser_openfile(pctx, includename->
					      value.string.base));
			 cfg_obj_destroy(pctx, &includename);
			 goto redo;
		}

		clause = NULL;
		for (clauseset = clausesets; *clauseset != NULL; clauseset++) {
			for (clause = *clauseset;
			     clause->name != NULL;
			     clause++) {
				if (strcasecmp(TOKEN_STRING(pctx),
					   clause->name) == 0)
					goto done;
			}
		}
	done:
		if (clause == NULL || clause->name == NULL) {
			cfg_parser_error(pctx, CFG_LOG_NOPREP, "unknown option");
			/*
			 * Try to recover by parsing this option as an unknown
			 * option and discarding it.
			 */
			CHECK(cfg_parse_obj(pctx, &cfg_type_unsupported, &eltobj));
			cfg_obj_destroy(pctx, &eltobj);
			CHECK(parse_semicolon(pctx));
			continue;
		}

		/* Clause is known. */

		/* Issue warnings if appropriate */
		if ((clause->flags & CFG_CLAUSEFLAG_OBSOLETE) != 0)
			cfg_parser_warning(pctx, 0, "option '%s' is obsolete",
				       clause->name);
		if ((clause->flags & CFG_CLAUSEFLAG_NOTIMP) != 0)
			cfg_parser_warning(pctx, 0, "option '%s' is "
				       "not implemented", clause->name);
		if ((clause->flags & CFG_CLAUSEFLAG_NYI) != 0)
			cfg_parser_warning(pctx, 0, "option '%s' is "
				       "not implemented", clause->name);
		/*
		 * Don't log options with CFG_CLAUSEFLAG_NEWDEFAULT
		 * set here - we need to log the *lack* of such an option,
		 * not its presence.
		 */

		/* See if the clause already has a value; if not create one. */
		result = isc_symtab_lookup(obj->value.map.symtab,
					   clause->name, 0, &symval);

		if ((clause->flags & CFG_CLAUSEFLAG_MULTI) != 0) {
			/* Multivalued clause */
			cfg_obj_t *listobj = NULL;
			if (result == ISC_R_NOTFOUND) {
				CHECK(cfg_create_list(pctx,
						  &cfg_type_implicitlist,
						  &listobj));
				symval.as_pointer = listobj;
				result = isc_symtab_define(obj->value.
						   map.symtab,
						   clause->name,
						   1, symval,
						   isc_symexists_reject);
				if (result != ISC_R_SUCCESS) {
					cfg_parser_error(pctx, CFG_LOG_NEAR,
						     "isc_symtab_define(%s) "
						     "failed", clause->name);
					isc_mem_put(pctx->mctx, list,
						    sizeof(cfg_list_t));
					goto cleanup;
				}
			} else {
				INSIST(result == ISC_R_SUCCESS);
				listobj = symval.as_pointer;
			}

			elt = NULL;
			CHECK(cfg_parse_listelt(pctx, clause->type, &elt));
			CHECK(parse_semicolon(pctx));

			ISC_LIST_APPEND(listobj->value.list, elt, link);
		} else {
			/* Single-valued clause */
			if (result == ISC_R_NOTFOUND) {
				isc_boolean_t callback =
					ISC_TF((clause->flags &
						CFG_CLAUSEFLAG_CALLBACK) != 0);
				CHECK(parse_symtab_elt(pctx, clause->name,
						       clause->type,
						       obj->value.map.symtab,
						       callback));
				CHECK(parse_semicolon(pctx));
			} else if (result == ISC_R_SUCCESS) {
				cfg_parser_error(pctx, CFG_LOG_NEAR, "'%s' redefined",
					     clause->name);
				result = ISC_R_EXISTS;
				goto cleanup;
			} else {
				cfg_parser_error(pctx, CFG_LOG_NEAR,
					     "isc_symtab_define() failed");
				goto cleanup;
			}
		}
	}


	*ret = obj;
	return (ISC_R_SUCCESS);

 cleanup:
	CLEANUP_OBJ(value);
	CLEANUP_OBJ(obj);
	CLEANUP_OBJ(eltobj);
	CLEANUP_OBJ(includename);
	return (result);
}

static isc_result_t
parse_symtab_elt(cfg_parser_t *pctx, const char *name,
		 cfg_type_t *elttype, isc_symtab_t *symtab,
		 isc_boolean_t callback)
{
	isc_result_t result;
	cfg_obj_t *obj = NULL;
	isc_symvalue_t symval;

	CHECK(cfg_parse_obj(pctx, elttype, &obj));

	if (callback && pctx->callback != NULL)
		CHECK(pctx->callback(name, obj, pctx->callbackarg));

	symval.as_pointer = obj;
	CHECK(isc_symtab_define(symtab, name,
				1, symval,
				isc_symexists_reject));
	return (ISC_R_SUCCESS);

 cleanup:
	CLEANUP_OBJ(obj);
	return (result);
}

/*
 * Parse a map; e.g., "{ foo 1; bar { glub; }; zap true; zap false; }"
 */
isc_result_t
cfg_parse_map(cfg_parser_t *pctx, const cfg_type_t *type, cfg_obj_t **ret) {
	isc_result_t result;
	CHECK(cfg_parse_special(pctx, '{'));
	CHECK(cfg_parse_mapbody(pctx, type, ret));
	CHECK(cfg_parse_special(pctx, '}'));
 cleanup:
	return (result);
}

/*
 * Subroutine for cfg_parse_named_map() and cfg_parse_addressed_map().
 */
static isc_result_t
parse_any_named_map(cfg_parser_t *pctx, cfg_type_t *nametype, const cfg_type_t *type,
		    cfg_obj_t **ret)
{
	isc_result_t result;
	cfg_obj_t *idobj = NULL;
	cfg_obj_t *mapobj = NULL;

	CHECK(cfg_parse_obj(pctx, nametype, &idobj));
	CHECK(cfg_parse_map(pctx, type, &mapobj));
	mapobj->value.map.id = idobj;
	idobj = NULL;
	*ret = mapobj;
 cleanup:
	CLEANUP_OBJ(idobj);
	return (result);
}

/*
 * Parse a map identified by a string name.  E.g., "name { foo 1; }".
 * Used for the "key" and "channel" statements.
 */
isc_result_t
cfg_parse_named_map(cfg_parser_t *pctx, const cfg_type_t *type, cfg_obj_t **ret) {
	return (parse_any_named_map(pctx, &cfg_type_astring, type, ret));
}

/*
 * Parse a map identified by a network address.
 * Used to be used for the "server" statement.
 */
isc_result_t
cfg_parse_addressed_map(cfg_parser_t *pctx, const cfg_type_t *type, cfg_obj_t **ret) {
	return (parse_any_named_map(pctx, &cfg_type_netaddr, type, ret));
}

/*
 * Parse a map identified by a network prefix.
 * Used for the "server" statement.
 */
isc_result_t
cfg_parse_netprefix_map(cfg_parser_t *pctx, const cfg_type_t *type, cfg_obj_t **ret) {
	return (parse_any_named_map(pctx, &cfg_type_netprefix, type, ret));
}

void
cfg_print_mapbody(cfg_printer_t *pctx, const cfg_obj_t *obj) {
	isc_result_t result = ISC_R_SUCCESS;

	const cfg_clausedef_t * const *clauseset;

	for (clauseset = obj->value.map.clausesets;
	     *clauseset != NULL;
	     clauseset++)
	{
		isc_symvalue_t symval;
		const cfg_clausedef_t *clause;

		for (clause = *clauseset;
		     clause->name != NULL;
		     clause++) {
			result = isc_symtab_lookup(obj->value.map.symtab,
						   clause->name, 0, &symval);
			if (result == ISC_R_SUCCESS) {
				cfg_obj_t *obj = symval.as_pointer;
				if (obj->type == &cfg_type_implicitlist) {
					/* Multivalued. */
					cfg_list_t *list = &obj->value.list;
					cfg_listelt_t *elt;
					for (elt = ISC_LIST_HEAD(*list);
					     elt != NULL;
					     elt = ISC_LIST_NEXT(elt, link)) {
						print_indent(pctx);
						cfg_print_cstr(pctx, clause->name);
						cfg_print_chars(pctx, " ", 1);
						cfg_print_obj(pctx, elt->obj);
						cfg_print_chars(pctx, ";\n", 2);
					}
				} else {
					/* Single-valued. */
					print_indent(pctx);
					cfg_print_cstr(pctx, clause->name);
					cfg_print_chars(pctx, " ", 1);
					cfg_print_obj(pctx, obj);
					cfg_print_chars(pctx, ";\n", 2);
				}
			} else if (result == ISC_R_NOTFOUND) {
				; /* do nothing */
			} else {
				INSIST(0);
			}
		}
	}
}

void
cfg_doc_mapbody(cfg_printer_t *pctx, const cfg_type_t *type) {
	const cfg_clausedef_t * const *clauseset;
	const cfg_clausedef_t *clause;

	for (clauseset = type->of; *clauseset != NULL; clauseset++) {
		for (clause = *clauseset;
		     clause->name != NULL;
		     clause++) {
			cfg_print_cstr(pctx, clause->name);
			cfg_print_chars(pctx, " ", 1);
			cfg_doc_obj(pctx, clause->type);
			cfg_print_chars(pctx, ";", 1);
			/* XXX print flags here? */
			cfg_print_chars(pctx, "\n\n", 2);
		}
	}
}

static struct flagtext {
	unsigned int flag;
	const char *text;
} flagtexts[] = {
	{ CFG_CLAUSEFLAG_NOTIMP, "not implemented" },
	{ CFG_CLAUSEFLAG_NYI, "not yet implemented" },
	{ CFG_CLAUSEFLAG_OBSOLETE, "obsolete" },
	{ CFG_CLAUSEFLAG_NEWDEFAULT, "default changed" },
	{ CFG_CLAUSEFLAG_TESTONLY, "test only" },
	{ 0, NULL }
};

void
cfg_print_map(cfg_printer_t *pctx, const cfg_obj_t *obj) {
	if (obj->value.map.id != NULL) {
		cfg_print_obj(pctx, obj->value.map.id);
		cfg_print_chars(pctx, " ", 1);
	}
	print_open(pctx);
	cfg_print_mapbody(pctx, obj);
	print_close(pctx);
}

static void
print_clause_flags(cfg_printer_t *pctx, unsigned int flags) {
	struct flagtext *p;
	isc_boolean_t first = ISC_TRUE;
	for (p = flagtexts; p->flag != 0; p++) {
		if ((flags & p->flag) != 0) {
			if (first)
				cfg_print_chars(pctx, " // ", 4);
			else
				cfg_print_chars(pctx, ", ", 2);
			cfg_print_cstr(pctx, p->text);
			first = ISC_FALSE;
		}
	}
}

void
cfg_doc_map(cfg_printer_t *pctx, const cfg_type_t *type) {
	const cfg_clausedef_t * const *clauseset;
	const cfg_clausedef_t *clause;

	if (type->parse == cfg_parse_named_map) {
		cfg_doc_obj(pctx, &cfg_type_astring);
		cfg_print_chars(pctx, " ", 1);
	} else if (type->parse == cfg_parse_addressed_map) {
		cfg_doc_obj(pctx, &cfg_type_netaddr);
		cfg_print_chars(pctx, " ", 1);
	} else if (type->parse == cfg_parse_netprefix_map) {
		cfg_doc_obj(pctx, &cfg_type_netprefix);
		cfg_print_chars(pctx, " ", 1);
	}

	print_open(pctx);

	for (clauseset = type->of; *clauseset != NULL; clauseset++) {
		for (clause = *clauseset;
		     clause->name != NULL;
		     clause++) {
			print_indent(pctx);
			cfg_print_cstr(pctx, clause->name);
			if (clause->type->print != cfg_print_void)
				cfg_print_chars(pctx, " ", 1);
			cfg_doc_obj(pctx, clause->type);
			cfg_print_chars(pctx, ";", 1);
			print_clause_flags(pctx, clause->flags);
			cfg_print_chars(pctx, "\n", 1);
		}
	}
	print_close(pctx);
}

isc_boolean_t
cfg_obj_ismap(const cfg_obj_t *obj) {
	REQUIRE(obj != NULL);
	return (ISC_TF(obj->type->rep == &cfg_rep_map));
}

isc_result_t
cfg_map_get(const cfg_obj_t *mapobj, const char* name, const cfg_obj_t **obj) {
	isc_result_t result;
	isc_symvalue_t val;
	const cfg_map_t *map;

	REQUIRE(mapobj != NULL && mapobj->type->rep == &cfg_rep_map);
	REQUIRE(name != NULL);
	REQUIRE(obj != NULL && *obj == NULL);

	map = &mapobj->value.map;

	result = isc_symtab_lookup(map->symtab, name, MAP_SYM, &val);
	if (result != ISC_R_SUCCESS)
		return (result);
	*obj = val.as_pointer;
	return (ISC_R_SUCCESS);
}

const cfg_obj_t *
cfg_map_getname(const cfg_obj_t *mapobj) {
	REQUIRE(mapobj != NULL && mapobj->type->rep == &cfg_rep_map);
	return (mapobj->value.map.id);
}


/* Parse an arbitrary token, storing its raw text representation. */
static isc_result_t
parse_token(cfg_parser_t *pctx, const cfg_type_t *type, cfg_obj_t **ret) {
	cfg_obj_t *obj = NULL;
	isc_result_t result;
	isc_region_t r;

	UNUSED(type);

	CHECK(cfg_create_obj(pctx, &cfg_type_token, &obj));
	CHECK(cfg_gettoken(pctx, CFG_LEXOPT_QSTRING));
	if (pctx->token.type == isc_tokentype_eof) {
		cfg_ungettoken(pctx);
		result = ISC_R_EOF;
		goto cleanup;
	}

	isc_lex_getlasttokentext(pctx->lexer, &pctx->token, &r);

	obj->value.string.base = isc_mem_get(pctx->mctx, r.length + 1);
	if (obj->value.string.base == NULL) {
		result = ISC_R_NOMEMORY;
		goto cleanup;
	}
	obj->value.string.length = r.length;
	memcpy(obj->value.string.base, r.base, r.length);
	obj->value.string.base[r.length] = '\0';
	*ret = obj;
	return (result);

 cleanup:
	if (obj != NULL)
		isc_mem_put(pctx->mctx, obj, sizeof(*obj));
	return (result);
}

cfg_type_t cfg_type_token = {
	"token", parse_token, cfg_print_ustring, cfg_doc_terminal,
	&cfg_rep_string, NULL
};

/*
 * An unsupported option.  This is just a list of tokens with balanced braces
 * ending in a semicolon.
 */

static isc_result_t
parse_unsupported(cfg_parser_t *pctx, const cfg_type_t *type, cfg_obj_t **ret) {
	cfg_obj_t *listobj = NULL;
	isc_result_t result;
	int braces = 0;

	CHECK(cfg_create_list(pctx, type, &listobj));

	for (;;) {
		cfg_listelt_t *elt = NULL;

		CHECK(cfg_peektoken(pctx, 0));
		if (pctx->token.type == isc_tokentype_special) {
			if (pctx->token.value.as_char == '{')
				braces++;
			else if (pctx->token.value.as_char == '}')
				braces--;
			else if (pctx->token.value.as_char == ';')
				if (braces == 0)
					break;
		}
		if (pctx->token.type == isc_tokentype_eof || braces < 0) {
			cfg_parser_error(pctx, CFG_LOG_NEAR, "unexpected token");
			result = ISC_R_UNEXPECTEDTOKEN;
			goto cleanup;
		}

		CHECK(cfg_parse_listelt(pctx, &cfg_type_token, &elt));
		ISC_LIST_APPEND(listobj->value.list, elt, link);
	}
	INSIST(braces == 0);
	*ret = listobj;
	return (ISC_R_SUCCESS);

 cleanup:
	CLEANUP_OBJ(listobj);
	return (result);
}

cfg_type_t cfg_type_unsupported = {
	"unsupported", parse_unsupported, cfg_print_spacelist, cfg_doc_terminal,
	&cfg_rep_list, NULL
};

/*
 * Try interpreting the current token as a network address.
 *
 * If CFG_ADDR_WILDOK is set in flags, "*" can be used as a wildcard
 * and at least one of CFG_ADDR_V4OK and CFG_ADDR_V6OK must also be set.  The
 * "*" is interpreted as the IPv4 wildcard address if CFG_ADDR_V4OK is
 * set (including the case where CFG_ADDR_V4OK and CFG_ADDR_V6OK are both set),
 * and the IPv6 wildcard address otherwise.
 */
static isc_result_t
token_addr(cfg_parser_t *pctx, unsigned int flags, isc_netaddr_t *na) {
	char *s;
	struct in_addr in4a;
	struct in6_addr in6a;

	if (pctx->token.type != isc_tokentype_string)
		return (ISC_R_UNEXPECTEDTOKEN);

	s = TOKEN_STRING(pctx);
	if ((flags & CFG_ADDR_WILDOK) != 0 && strcmp(s, "*") == 0) {
		if ((flags & CFG_ADDR_V4OK) != 0) {
			isc_netaddr_any(na);
			return (ISC_R_SUCCESS);
		} else if ((flags & CFG_ADDR_V6OK) != 0) {
			isc_netaddr_any6(na);
			return (ISC_R_SUCCESS);
		} else {
			INSIST(0);
		}
	} else {
		if ((flags & (CFG_ADDR_V4OK | CFG_ADDR_V4PREFIXOK)) != 0) {
			if (inet_pton(AF_INET, s, &in4a) == 1) {
				isc_netaddr_fromin(na, &in4a);
				return (ISC_R_SUCCESS);
			}
		}
		if ((flags & CFG_ADDR_V4PREFIXOK) != 0 &&
		    strlen(s) <= 15U) {
			char buf[64];
			int i;

			strcpy(buf, s);
			for (i = 0; i < 3; i++) {
				strcat(buf, ".0");
				if (inet_pton(AF_INET, buf, &in4a) == 1) {
					isc_netaddr_fromin(na, &in4a);
					return (ISC_R_SUCCESS);
				}
			}
		}
		if ((flags & CFG_ADDR_V6OK) != 0 &&
		    strlen(s) <= 127U) {
			char buf[128]; /* see lib/bind9/getaddresses.c */
			char *d; /* zone delimiter */
			isc_uint32_t zone = 0; /* scope zone ID */

			strcpy(buf, s);
			d = strchr(buf, '%');
			if (d != NULL)
				*d = '\0';

			if (inet_pton(AF_INET6, buf, &in6a) == 1) {
				if (d != NULL) {
#ifdef ISC_PLATFORM_HAVESCOPEID
					isc_result_t result;

					result = isc_netscope_pton(AF_INET6,
								   d + 1,
								   &in6a,
								   &zone);
					if (result != ISC_R_SUCCESS)
						return (result);
#else
				return (ISC_R_BADADDRESSFORM);
#endif
				}

				isc_netaddr_fromin6(na, &in6a);
				isc_netaddr_setzone(na, zone);
				return (ISC_R_SUCCESS);
			}
		}
	}
	return (ISC_R_UNEXPECTEDTOKEN);
}

isc_result_t
cfg_parse_rawaddr(cfg_parser_t *pctx, unsigned int flags, isc_netaddr_t *na) {
	isc_result_t result;
	const char *wild = "";
	const char *prefix = "";

	CHECK(cfg_gettoken(pctx, 0));
	result = token_addr(pctx, flags, na);
	if (result == ISC_R_UNEXPECTEDTOKEN) {
		if ((flags & CFG_ADDR_WILDOK) != 0)
			wild = " or '*'";
		if ((flags & CFG_ADDR_V4PREFIXOK) != 0)
			wild = " or IPv4 prefix";
		if ((flags & CFG_ADDR_MASK) == CFG_ADDR_V4OK)
			cfg_parser_error(pctx, CFG_LOG_NEAR,
					 "expected IPv4 address%s%s",
					 prefix, wild);
		else if ((flags & CFG_ADDR_MASK) == CFG_ADDR_V6OK)
			cfg_parser_error(pctx, CFG_LOG_NEAR,
					 "expected IPv6 address%s%s",
					 prefix, wild);
		else
			cfg_parser_error(pctx, CFG_LOG_NEAR,
					 "expected IP address%s%s",
					 prefix, wild);
	}
 cleanup:
	return (result);
}

isc_boolean_t
cfg_lookingat_netaddr(cfg_parser_t *pctx, unsigned int flags) {
	isc_result_t result;
	isc_netaddr_t na_dummy;
	result = token_addr(pctx, flags, &na_dummy);
	return (ISC_TF(result == ISC_R_SUCCESS));
}

isc_result_t
cfg_parse_rawport(cfg_parser_t *pctx, unsigned int flags, in_port_t *port) {
	isc_result_t result;

	CHECK(cfg_gettoken(pctx, ISC_LEXOPT_NUMBER));

	if ((flags & CFG_ADDR_WILDOK) != 0 &&
	    pctx->token.type == isc_tokentype_string &&
	    strcmp(TOKEN_STRING(pctx), "*") == 0) {
		*port = 0;
		return (ISC_R_SUCCESS);
	}
	if (pctx->token.type != isc_tokentype_number) {
		cfg_parser_error(pctx, CFG_LOG_NEAR,
			     "expected port number or '*'");
		return (ISC_R_UNEXPECTEDTOKEN);
	}
	if (pctx->token.value.as_ulong >= 65536U) {
		cfg_parser_error(pctx, CFG_LOG_NEAR,
			     "port number out of range");
		return (ISC_R_UNEXPECTEDTOKEN);
	}
	*port = (in_port_t)(pctx->token.value.as_ulong);
	return (ISC_R_SUCCESS);
 cleanup:
	return (result);
}

void
cfg_print_rawaddr(cfg_printer_t *pctx, const isc_netaddr_t *na) {
	isc_result_t result;
	char text[128];
	isc_buffer_t buf;

	isc_buffer_init(&buf, text, sizeof(text));
	result = isc_netaddr_totext(na, &buf);
	RUNTIME_CHECK(result == ISC_R_SUCCESS);
	cfg_print_chars(pctx, isc_buffer_base(&buf), isc_buffer_usedlength(&buf));
}

/* netaddr */

static unsigned int netaddr_flags = CFG_ADDR_V4OK | CFG_ADDR_V6OK;
static unsigned int netaddr4_flags = CFG_ADDR_V4OK;
static unsigned int netaddr4wild_flags = CFG_ADDR_V4OK | CFG_ADDR_WILDOK;
static unsigned int netaddr6_flags = CFG_ADDR_V6OK;
static unsigned int netaddr6wild_flags = CFG_ADDR_V6OK | CFG_ADDR_WILDOK;

static isc_result_t
parse_netaddr(cfg_parser_t *pctx, const cfg_type_t *type, cfg_obj_t **ret) {
	isc_result_t result;
	cfg_obj_t *obj = NULL;
	isc_netaddr_t netaddr;
	unsigned int flags = *(const unsigned int *)type->of;

	CHECK(cfg_create_obj(pctx, type, &obj));
	CHECK(cfg_parse_rawaddr(pctx, flags, &netaddr));
	isc_sockaddr_fromnetaddr(&obj->value.sockaddr, &netaddr, 0);
	*ret = obj;
	return (ISC_R_SUCCESS);
 cleanup:
	CLEANUP_OBJ(obj);
	return (result);
}

static void
cfg_doc_netaddr(cfg_printer_t *pctx, const cfg_type_t *type) {
	const unsigned int *flagp = type->of;
	int n = 0;
	if (*flagp != CFG_ADDR_V4OK && *flagp != CFG_ADDR_V6OK)
		cfg_print_chars(pctx, "( ", 2);
	if (*flagp & CFG_ADDR_V4OK) {
		cfg_print_cstr(pctx, "<ipv4_address>");
		n++;
	}
	if (*flagp & CFG_ADDR_V6OK) {
		if (n != 0)
			cfg_print_chars(pctx, " | ", 3);
		cfg_print_cstr(pctx, "<ipv6_address>");
		n++;
	}
	if (*flagp & CFG_ADDR_WILDOK) {
		if (n != 0)
			cfg_print_chars(pctx, " | ", 3);
		cfg_print_chars(pctx, "*", 1);
		n++;
	}
	if (*flagp != CFG_ADDR_V4OK && *flagp != CFG_ADDR_V6OK)
		cfg_print_chars(pctx, " )", 2);
}

cfg_type_t cfg_type_netaddr = {
	"netaddr", parse_netaddr, cfg_print_sockaddr, cfg_doc_netaddr,
	&cfg_rep_sockaddr, &netaddr_flags
};

cfg_type_t cfg_type_netaddr4 = {
	"netaddr4", parse_netaddr, cfg_print_sockaddr, cfg_doc_netaddr,
	&cfg_rep_sockaddr, &netaddr4_flags
};

cfg_type_t cfg_type_netaddr4wild = {
	"netaddr4wild", parse_netaddr, cfg_print_sockaddr, cfg_doc_netaddr,
	&cfg_rep_sockaddr, &netaddr4wild_flags
};

cfg_type_t cfg_type_netaddr6 = {
	"netaddr6", parse_netaddr, cfg_print_sockaddr, cfg_doc_netaddr,
	&cfg_rep_sockaddr, &netaddr6_flags
};

cfg_type_t cfg_type_netaddr6wild = {
	"netaddr6wild", parse_netaddr, cfg_print_sockaddr, cfg_doc_netaddr,
	&cfg_rep_sockaddr, &netaddr6wild_flags
};

/* netprefix */

isc_result_t
cfg_parse_netprefix(cfg_parser_t *pctx, const cfg_type_t *type,
		    cfg_obj_t **ret)
{
	cfg_obj_t *obj = NULL;
	isc_result_t result;
	isc_netaddr_t netaddr;
	unsigned int addrlen, prefixlen;
	UNUSED(type);

	CHECK(cfg_parse_rawaddr(pctx, CFG_ADDR_V4OK | CFG_ADDR_V4PREFIXOK |
				CFG_ADDR_V6OK, &netaddr));
	switch (netaddr.family) {
	case AF_INET:
		addrlen = 32;
		break;
	case AF_INET6:
		addrlen = 128;
		break;
	default:
		addrlen = 0;
		INSIST(0);
		break;
	}
	CHECK(cfg_peektoken(pctx, 0));
	if (pctx->token.type == isc_tokentype_special &&
	    pctx->token.value.as_char == '/') {
		CHECK(cfg_gettoken(pctx, 0)); /* read "/" */
		CHECK(cfg_gettoken(pctx, ISC_LEXOPT_NUMBER));
		if (pctx->token.type != isc_tokentype_number) {
			cfg_parser_error(pctx, CFG_LOG_NEAR,
				     "expected prefix length");
			return (ISC_R_UNEXPECTEDTOKEN);
		}
		prefixlen = pctx->token.value.as_ulong;
		if (prefixlen > addrlen) {
			cfg_parser_error(pctx, CFG_LOG_NOPREP,
				     "invalid prefix length");
			return (ISC_R_RANGE);
		}
	} else {
		prefixlen = addrlen;
	}
	CHECK(cfg_create_obj(pctx, &cfg_type_netprefix, &obj));
	obj->value.netprefix.address = netaddr;
	obj->value.netprefix.prefixlen = prefixlen;
	*ret = obj;
	return (ISC_R_SUCCESS);
 cleanup:
	cfg_parser_error(pctx, CFG_LOG_NEAR, "expected network prefix");
	return (result);
}

static void
print_netprefix(cfg_printer_t *pctx, const cfg_obj_t *obj) {
	const cfg_netprefix_t *p = &obj->value.netprefix;

	cfg_print_rawaddr(pctx, &p->address);
	cfg_print_chars(pctx, "/", 1);
	cfg_print_rawuint(pctx, p->prefixlen);
}

isc_boolean_t
cfg_obj_isnetprefix(const cfg_obj_t *obj) {
	REQUIRE(obj != NULL);
	return (ISC_TF(obj->type->rep == &cfg_rep_netprefix));
}

void
cfg_obj_asnetprefix(const cfg_obj_t *obj, isc_netaddr_t *netaddr,
		    unsigned int *prefixlen) {
	REQUIRE(obj != NULL && obj->type->rep == &cfg_rep_netprefix);
	*netaddr = obj->value.netprefix.address;
	*prefixlen = obj->value.netprefix.prefixlen;
}

cfg_type_t cfg_type_netprefix = {
	"netprefix", cfg_parse_netprefix, print_netprefix, cfg_doc_terminal,
	&cfg_rep_netprefix, NULL
};

static isc_result_t
parse_sockaddrsub(cfg_parser_t *pctx, const cfg_type_t *type,
		  int flags, cfg_obj_t **ret)
{
	isc_result_t result;
	isc_netaddr_t netaddr;
	in_port_t port = 0;
	cfg_obj_t *obj = NULL;

	CHECK(cfg_create_obj(pctx, type, &obj));
	CHECK(cfg_parse_rawaddr(pctx, flags, &netaddr));
	CHECK(cfg_peektoken(pctx, 0));
	if (pctx->token.type == isc_tokentype_string &&
	    strcasecmp(TOKEN_STRING(pctx), "port") == 0) {
		CHECK(cfg_gettoken(pctx, 0)); /* read "port" */
		CHECK(cfg_parse_rawport(pctx, flags, &port));
	}
	isc_sockaddr_fromnetaddr(&obj->value.sockaddr, &netaddr, port);
	*ret = obj;
	return (ISC_R_SUCCESS);

 cleanup:
	CLEANUP_OBJ(obj);
	return (result);
}

static unsigned int sockaddr_flags = CFG_ADDR_V4OK | CFG_ADDR_V6OK;
cfg_type_t cfg_type_sockaddr = {
	"sockaddr", cfg_parse_sockaddr, cfg_print_sockaddr, cfg_doc_sockaddr,
	&cfg_rep_sockaddr, &sockaddr_flags
};

isc_result_t
cfg_parse_sockaddr(cfg_parser_t *pctx, const cfg_type_t *type, cfg_obj_t **ret) {
	const unsigned int *flagp = type->of;
	return (parse_sockaddrsub(pctx, &cfg_type_sockaddr, *flagp, ret));
}

void
cfg_print_sockaddr(cfg_printer_t *pctx, const cfg_obj_t *obj) {
	isc_netaddr_t netaddr;
	in_port_t port;
	char buf[ISC_NETADDR_FORMATSIZE];

	isc_netaddr_fromsockaddr(&netaddr, &obj->value.sockaddr);
	isc_netaddr_format(&netaddr, buf, sizeof(buf));
	cfg_print_cstr(pctx, buf);
	port = isc_sockaddr_getport(&obj->value.sockaddr);
	if (port != 0) {
		cfg_print_chars(pctx, " port ", 6);
		cfg_print_rawuint(pctx, port);
	}
}

void
cfg_doc_sockaddr(cfg_printer_t *pctx, const cfg_type_t *type) {
	const unsigned int *flagp = type->of;
	int n = 0;
	cfg_print_chars(pctx, "( ", 2);
	if (*flagp & CFG_ADDR_V4OK) {
		cfg_print_cstr(pctx, "<ipv4_address>");
		n++;
	}
	if (*flagp & CFG_ADDR_V6OK) {
		if (n != 0)
			cfg_print_chars(pctx, " | ", 3);
		cfg_print_cstr(pctx, "<ipv6_address>");
		n++;
	}
	if (*flagp & CFG_ADDR_WILDOK) {
		if (n != 0)
			cfg_print_chars(pctx, " | ", 3);
		cfg_print_chars(pctx, "*", 1);
		n++;
	}
	cfg_print_chars(pctx, " ) ", 3);
	if (*flagp & CFG_ADDR_WILDOK) {
		cfg_print_cstr(pctx, "[ port ( <integer> | * ) ]");
	} else {
		cfg_print_cstr(pctx, "[ port <integer> ]");
	}
}

isc_boolean_t
cfg_obj_issockaddr(const cfg_obj_t *obj) {
	REQUIRE(obj != NULL);
	return (ISC_TF(obj->type->rep == &cfg_rep_sockaddr));
}

const isc_sockaddr_t *
cfg_obj_assockaddr(const cfg_obj_t *obj) {
	REQUIRE(obj != NULL && obj->type->rep == &cfg_rep_sockaddr);
	return (&obj->value.sockaddr);
}

isc_result_t
cfg_gettoken(cfg_parser_t *pctx, int options) {
	isc_result_t result;

	if (pctx->seen_eof)
		return (ISC_R_SUCCESS);

	options |= (ISC_LEXOPT_EOF | ISC_LEXOPT_NOMORE);

 redo:
	pctx->token.type = isc_tokentype_unknown;
	result = isc_lex_gettoken(pctx->lexer, options, &pctx->token);
	pctx->ungotten = ISC_FALSE;
	pctx->line = isc_lex_getsourceline(pctx->lexer);

	switch (result) {
	case ISC_R_SUCCESS:
		if (pctx->token.type == isc_tokentype_eof) {
			result = isc_lex_close(pctx->lexer);
			INSIST(result == ISC_R_NOMORE ||
			       result == ISC_R_SUCCESS);

			if (isc_lex_getsourcename(pctx->lexer) != NULL) {
				/*
				 * Closed an included file, not the main file.
				 */
				cfg_listelt_t *elt;
				elt = ISC_LIST_TAIL(pctx->open_files->
						    value.list);
				INSIST(elt != NULL);
				ISC_LIST_UNLINK(pctx->open_files->
						value.list, elt, link);
				ISC_LIST_APPEND(pctx->closed_files->
						value.list, elt, link);
				goto redo;
			}
			pctx->seen_eof = ISC_TRUE;
		}
		break;

	case ISC_R_NOSPACE:
		/* More understandable than "ran out of space". */
		cfg_parser_error(pctx, CFG_LOG_NEAR, "token too big");
		break;

	case ISC_R_IOERROR:
		cfg_parser_error(pctx, 0, "%s",
				 isc_result_totext(result));
		break;

	default:
		cfg_parser_error(pctx, CFG_LOG_NEAR, "%s",
				 isc_result_totext(result));
		break;
	}
	return (result);
}

void
cfg_ungettoken(cfg_parser_t *pctx) {
	if (pctx->seen_eof)
		return;
	isc_lex_ungettoken(pctx->lexer, &pctx->token);
	pctx->ungotten = ISC_TRUE;
}

isc_result_t
cfg_peektoken(cfg_parser_t *pctx, int options) {
	isc_result_t result;
	CHECK(cfg_gettoken(pctx, options));
	cfg_ungettoken(pctx);
 cleanup:
	return (result);
}

/*
 * Get a string token, accepting both the quoted and the unquoted form.
 * Log an error if the next token is not a string.
 */
static isc_result_t
cfg_getstringtoken(cfg_parser_t *pctx) {
	isc_result_t result;

	result = cfg_gettoken(pctx, CFG_LEXOPT_QSTRING);
	if (result != ISC_R_SUCCESS)
		return (result);

	if (pctx->token.type != isc_tokentype_string &&
	    pctx->token.type != isc_tokentype_qstring) {
		cfg_parser_error(pctx, CFG_LOG_NEAR, "expected string");
		return (ISC_R_UNEXPECTEDTOKEN);
	}
	return (ISC_R_SUCCESS);
}

void
cfg_parser_error(cfg_parser_t *pctx, unsigned int flags, const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	parser_complain(pctx, ISC_FALSE, flags, fmt, args);
	va_end(args);
	pctx->errors++;
}

void
cfg_parser_warning(cfg_parser_t *pctx, unsigned int flags, const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	parser_complain(pctx, ISC_TRUE, flags, fmt, args);
	va_end(args);
	pctx->warnings++;
}

#define MAX_LOG_TOKEN 30 /* How much of a token to quote in log messages. */

static char *
current_file(cfg_parser_t *pctx) {
	static char none[] = "none";
	cfg_listelt_t *elt;
	cfg_obj_t *fileobj;

	if (pctx->open_files == NULL)
		return (none);
	elt = ISC_LIST_TAIL(pctx->open_files->value.list);
	if (elt == NULL)
	      return (none);

	fileobj = elt->obj;
	INSIST(fileobj->type == &cfg_type_qstring);
	return (fileobj->value.string.base);
}

static void
parser_complain(cfg_parser_t *pctx, isc_boolean_t is_warning,
		unsigned int flags, const char *format,
		va_list args)
{
	char tokenbuf[MAX_LOG_TOKEN + 10];
	static char where[ISC_DIR_PATHMAX + 100];
	static char message[2048];
	int level = ISC_LOG_ERROR;
	const char *prep = "";
	size_t len;

	if (is_warning)
		level = ISC_LOG_WARNING;

	snprintf(where, sizeof(where), "%s:%u: ",
		 current_file(pctx), pctx->line);

	len = vsnprintf(message, sizeof(message), format, args);
	if (len >= sizeof(message))
		FATAL_ERROR(__FILE__, __LINE__,
			    "error message would overflow");

	if ((flags & (CFG_LOG_NEAR|CFG_LOG_BEFORE|CFG_LOG_NOPREP)) != 0) {
		isc_region_t r;

		if (pctx->ungotten)
			(void)cfg_gettoken(pctx, 0);

		if (pctx->token.type == isc_tokentype_eof) {
			snprintf(tokenbuf, sizeof(tokenbuf), "end of file");
		} else if (pctx->token.type == isc_tokentype_unknown) {
			flags = 0;
			tokenbuf[0] = '\0';
		} else {
			isc_lex_getlasttokentext(pctx->lexer,
						 &pctx->token, &r);
			if (r.length > MAX_LOG_TOKEN)
				snprintf(tokenbuf, sizeof(tokenbuf),
					 "'%.*s...'", MAX_LOG_TOKEN, r.base);
			else
				snprintf(tokenbuf, sizeof(tokenbuf),
					 "'%.*s'", (int)r.length, r.base);
		}

		/* Choose a preposition. */
		if (flags & CFG_LOG_NEAR)
			prep = " near ";
		else if (flags & CFG_LOG_BEFORE)
			prep = " before ";
		else
			prep = " ";
	} else {
		tokenbuf[0] = '\0';
	}
	isc_log_write(pctx->lctx, CAT, MOD, level,
		      "%s%s%s%s", where, message, prep, tokenbuf);
}

void
cfg_obj_log(const cfg_obj_t *obj, isc_log_t *lctx, int level,
	    const char *fmt, ...) {
	va_list ap;
	char msgbuf[2048];

	if (! isc_log_wouldlog(lctx, level))
		return;

	va_start(ap, fmt);

	vsnprintf(msgbuf, sizeof(msgbuf), fmt, ap);
	isc_log_write(lctx, CAT, MOD, level,
		      "%s:%u: %s",
		      obj->file == NULL ? "<unknown file>" : obj->file,
		      obj->line, msgbuf);
	va_end(ap);
}

const char *
cfg_obj_file(const cfg_obj_t *obj) {
	return (obj->file);
}

unsigned int
cfg_obj_line(const cfg_obj_t *obj) {
	return (obj->line);
}

isc_result_t
cfg_create_obj(cfg_parser_t *pctx, const cfg_type_t *type, cfg_obj_t **ret) {
	cfg_obj_t *obj;

	obj = isc_mem_get(pctx->mctx, sizeof(cfg_obj_t));
	if (obj == NULL)
		return (ISC_R_NOMEMORY);
	obj->type = type;
	obj->file = current_file(pctx);
	obj->line = pctx->line;
	*ret = obj;
	return (ISC_R_SUCCESS);
}

static void
map_symtabitem_destroy(char *key, unsigned int type,
		       isc_symvalue_t symval, void *userarg)
{
	cfg_obj_t *obj = symval.as_pointer;
	cfg_parser_t *pctx = (cfg_parser_t *)userarg;

	UNUSED(key);
	UNUSED(type);

	cfg_obj_destroy(pctx, &obj);
}


static isc_result_t
create_map(cfg_parser_t *pctx, const cfg_type_t *type, cfg_obj_t **ret) {
	isc_result_t result;
	isc_symtab_t *symtab = NULL;
	cfg_obj_t *obj = NULL;

	CHECK(cfg_create_obj(pctx, type, &obj));
	CHECK(isc_symtab_create(pctx->mctx, 5, /* XXX */
				map_symtabitem_destroy,
				pctx, ISC_FALSE, &symtab));
	obj->value.map.symtab = symtab;
	obj->value.map.id = NULL;

	*ret = obj;
	return (ISC_R_SUCCESS);

 cleanup:
	if (obj != NULL)
		isc_mem_put(pctx->mctx, obj, sizeof(*obj));
	return (result);
}

static void
free_map(cfg_parser_t *pctx, cfg_obj_t *obj) {
	CLEANUP_OBJ(obj->value.map.id);
	isc_symtab_destroy(&obj->value.map.symtab);
}

isc_boolean_t
cfg_obj_istype(const cfg_obj_t *obj, const cfg_type_t *type) {
	return (ISC_TF(obj->type == type));
}

/*
 * Destroy 'obj', a configuration object created in 'pctx'.
 */
void
cfg_obj_destroy(cfg_parser_t *pctx, cfg_obj_t **objp) {
	cfg_obj_t *obj = *objp;
	obj->type->rep->free(pctx, obj);
	isc_mem_put(pctx->mctx, obj, sizeof(cfg_obj_t));
	*objp = NULL;
}

static void
free_noop(cfg_parser_t *pctx, cfg_obj_t *obj) {
	UNUSED(pctx);
	UNUSED(obj);
}

void
cfg_doc_obj(cfg_printer_t *pctx, const cfg_type_t *type) {
	type->doc(pctx, type);
}

void
cfg_doc_terminal(cfg_printer_t *pctx, const cfg_type_t *type) {
	cfg_print_chars(pctx, "<", 1);
	cfg_print_cstr(pctx, type->name);
	cfg_print_chars(pctx, ">", 1);
}

void
cfg_print_grammar(const cfg_type_t *type,
	void (*f)(void *closure, const char *text, int textlen),
	void *closure)
{
	cfg_printer_t pctx;
	pctx.f = f;
	pctx.closure = closure;
	pctx.indent = 0;
	cfg_doc_obj(&pctx, type);
}
