/*
 * Copyright (C) 2004-2008  Internet Systems Consortium, Inc. ("ISC")
 * Copyright (C) 2002, 2003  Internet Software Consortium.
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

/* $Id: grammar.h,v 1.17 2008/09/25 04:02:39 tbox Exp $ */

#ifndef ISCCFG_GRAMMAR_H
#define ISCCFG_GRAMMAR_H 1

/*! \file isccfg/grammar.h */

#include <isc/lex.h>
#include <isc/netaddr.h>
#include <isc/sockaddr.h>
#include <isc/region.h>
#include <isc/types.h>

#include <isccfg/cfg.h>

/*
 * Definitions shared between the configuration parser
 * and the grammars; not visible to users of the parser.
 */

/*% Clause may occur multiple times (e.g., "zone") */
#define CFG_CLAUSEFLAG_MULTI 		0x00000001
/*% Clause is obsolete */
#define CFG_CLAUSEFLAG_OBSOLETE 	0x00000002
/*% Clause is not implemented, and may never be */
#define CFG_CLAUSEFLAG_NOTIMP	 	0x00000004
/*% Clause is not implemented yet */
#define CFG_CLAUSEFLAG_NYI 		0x00000008
/*% Default value has changed since earlier release */
#define CFG_CLAUSEFLAG_NEWDEFAULT	0x00000010
/*%
 * Clause needs to be interpreted during parsing
 * by calling a callback function, like the
 * "directory" option.
 */
#define CFG_CLAUSEFLAG_CALLBACK		0x00000020
/*% A option that is only used in testing. */
#define CFG_CLAUSEFLAG_TESTONLY		0x00000040

typedef struct cfg_clausedef cfg_clausedef_t;
typedef struct cfg_tuplefielddef cfg_tuplefielddef_t;
typedef struct cfg_printer cfg_printer_t;
typedef ISC_LIST(cfg_listelt_t) cfg_list_t;
typedef struct cfg_map cfg_map_t;
typedef struct cfg_rep cfg_rep_t;

/*
 * Function types for configuration object methods
 */

typedef isc_result_t (*cfg_parsefunc_t)(cfg_parser_t *, const cfg_type_t *type,
					cfg_obj_t **);
typedef void	     (*cfg_printfunc_t)(cfg_printer_t *, const cfg_obj_t *);
typedef void	     (*cfg_docfunc_t)(cfg_printer_t *, const cfg_type_t *);
typedef void	     (*cfg_freefunc_t)(cfg_parser_t *, cfg_obj_t *);

/*
 * Structure definitions
 */

/*%
 * A configuration printer object.  This is an abstract
 * interface to a destination to which text can be printed
 * by calling the function 'f'.
 */
struct cfg_printer {
	void (*f)(void *closure, const char *text, int textlen);
	void *closure;
	int indent;
};

/*% A clause definition. */
struct cfg_clausedef {
	const char      *name;
	cfg_type_t      *type;
	unsigned int	flags;
};

/*% A tuple field definition. */
struct cfg_tuplefielddef {
	const char      *name;
	cfg_type_t      *type;
	unsigned int	flags;
};

/*% A configuration object type definition. */
struct cfg_type {
	const char *name;	/*%< For debugging purposes only */
	cfg_parsefunc_t	parse;
	cfg_printfunc_t print;
	cfg_docfunc_t	doc;	/*%< Print grammar description */
	cfg_rep_t *	rep;	/*%< Data representation */
	const void *	of;	/*%< Additional data for meta-types */
};

/*% A keyword-type definition, for things like "port <integer>". */
typedef struct {
	const char *name;
	const cfg_type_t *type;
} keyword_type_t;

struct cfg_map {
	cfg_obj_t	 *id; /*%< Used for 'named maps' like keys, zones, &c */
	const cfg_clausedef_t * const *clausesets; /*%< The clauses that
						      can occur in this map;
						      used for printing */
	isc_symtab_t     *symtab;
};

typedef struct cfg_netprefix cfg_netprefix_t;

struct cfg_netprefix {
	isc_netaddr_t address; /* IP4/IP6 */
	unsigned int prefixlen;
};

/*%
 * A configuration data representation.
 */
struct cfg_rep {
	const char *	name;	/*%< For debugging only */
	cfg_freefunc_t 	free;	/*%< How to free this kind of data. */
};

/*%
 * A configuration object.  This is the main building block
 * of the configuration parse tree.
 */

struct cfg_obj {
	const cfg_type_t *type;
	union {
		isc_uint32_t  	uint32;
		isc_uint64_t  	uint64;
		isc_textregion_t string; /*%< null terminated, too */
		isc_boolean_t 	boolean;
		cfg_map_t	map;
		cfg_list_t	list;
		cfg_obj_t **	tuple;
		isc_sockaddr_t	sockaddr;
		cfg_netprefix_t netprefix;
	}               value;
	const char *	file;
	unsigned int    line;
};


/*% A list element. */
struct cfg_listelt {
	cfg_obj_t               *obj;
	ISC_LINK(cfg_listelt_t)  link;
};

/*% The parser object. */
struct cfg_parser {
	isc_mem_t *	mctx;
	isc_log_t *	lctx;
	isc_lex_t *	lexer;
	unsigned int    errors;
	unsigned int    warnings;
	isc_token_t     token;

	/*% We are at the end of all input. */
	isc_boolean_t	seen_eof;

	/*% The current token has been pushed back. */
	isc_boolean_t	ungotten;

	/*%
	 * The stack of currently active files, represented
	 * as a configuration list of configuration strings.
	 * The head is the top-level file, subsequent elements
	 * (if any) are the nested include files, and the
	 * last element is the file currently being parsed.
	 */
	cfg_obj_t *	open_files;

	/*%
	 * Names of files that we have parsed and closed
	 * and were previously on the open_file list.
	 * We keep these objects around after closing
	 * the files because the file names may still be
	 * referenced from other configuration objects
	 * for use in reporting semantic errors after
	 * parsing is complete.
	 */
	cfg_obj_t *	closed_files;

	/*%
	 * Current line number.  We maintain our own
	 * copy of this so that it is available even
	 * when a file has just been closed.
	 */
	unsigned int	line;

	cfg_parsecallback_t callback;
	void *callbackarg;
};


/*@{*/
/*%
 * Flags defining whether to accept certain types of network addresses.
 */
#define CFG_ADDR_V4OK 		0x00000001
#define CFG_ADDR_V4PREFIXOK 	0x00000002
#define CFG_ADDR_V6OK 		0x00000004
#define CFG_ADDR_WILDOK		0x00000008
#define CFG_ADDR_MASK		(CFG_ADDR_V6OK|CFG_ADDR_V4OK)
/*@}*/

/*@{*/
/*%
 * Predefined data representation types.
 */
LIBISCCFG_EXTERNAL_DATA extern cfg_rep_t cfg_rep_uint32;
LIBISCCFG_EXTERNAL_DATA extern cfg_rep_t cfg_rep_uint64;
LIBISCCFG_EXTERNAL_DATA extern cfg_rep_t cfg_rep_string;
LIBISCCFG_EXTERNAL_DATA extern cfg_rep_t cfg_rep_boolean;
LIBISCCFG_EXTERNAL_DATA extern cfg_rep_t cfg_rep_map;
LIBISCCFG_EXTERNAL_DATA extern cfg_rep_t cfg_rep_list;
LIBISCCFG_EXTERNAL_DATA extern cfg_rep_t cfg_rep_tuple;
LIBISCCFG_EXTERNAL_DATA extern cfg_rep_t cfg_rep_sockaddr;
LIBISCCFG_EXTERNAL_DATA extern cfg_rep_t cfg_rep_netprefix;
LIBISCCFG_EXTERNAL_DATA extern cfg_rep_t cfg_rep_void;
/*@}*/

/*@{*/
/*%
 * Predefined configuration object types.
 */
LIBISCCFG_EXTERNAL_DATA extern cfg_type_t cfg_type_boolean;
LIBISCCFG_EXTERNAL_DATA extern cfg_type_t cfg_type_uint32;
LIBISCCFG_EXTERNAL_DATA extern cfg_type_t cfg_type_uint64;
LIBISCCFG_EXTERNAL_DATA extern cfg_type_t cfg_type_qstring;
LIBISCCFG_EXTERNAL_DATA extern cfg_type_t cfg_type_astring;
LIBISCCFG_EXTERNAL_DATA extern cfg_type_t cfg_type_ustring;
LIBISCCFG_EXTERNAL_DATA extern cfg_type_t cfg_type_sockaddr;
LIBISCCFG_EXTERNAL_DATA extern cfg_type_t cfg_type_netaddr;
LIBISCCFG_EXTERNAL_DATA extern cfg_type_t cfg_type_netaddr4;
LIBISCCFG_EXTERNAL_DATA extern cfg_type_t cfg_type_netaddr4wild;
LIBISCCFG_EXTERNAL_DATA extern cfg_type_t cfg_type_netaddr6;
LIBISCCFG_EXTERNAL_DATA extern cfg_type_t cfg_type_netaddr6wild;
LIBISCCFG_EXTERNAL_DATA extern cfg_type_t cfg_type_netprefix;
LIBISCCFG_EXTERNAL_DATA extern cfg_type_t cfg_type_void;
LIBISCCFG_EXTERNAL_DATA extern cfg_type_t cfg_type_token;
LIBISCCFG_EXTERNAL_DATA extern cfg_type_t cfg_type_unsupported;
/*@}*/

isc_result_t
cfg_gettoken(cfg_parser_t *pctx, int options);

isc_result_t
cfg_peektoken(cfg_parser_t *pctx, int options);

void
cfg_ungettoken(cfg_parser_t *pctx);

#define CFG_LEXOPT_QSTRING (ISC_LEXOPT_QSTRING | ISC_LEXOPT_QSTRINGMULTILINE)

isc_result_t
cfg_create_obj(cfg_parser_t *pctx, const cfg_type_t *type, cfg_obj_t **objp);

void
cfg_print_rawuint(cfg_printer_t *pctx, unsigned int u);

isc_result_t
cfg_parse_uint32(cfg_parser_t *pctx, const cfg_type_t *type, cfg_obj_t **ret);

void
cfg_print_uint32(cfg_printer_t *pctx, const cfg_obj_t *obj);

void
cfg_print_uint64(cfg_printer_t *pctx, const cfg_obj_t *obj);

isc_result_t
cfg_parse_qstring(cfg_parser_t *pctx, const cfg_type_t *type, cfg_obj_t **ret);

void
cfg_print_ustring(cfg_printer_t *pctx, const cfg_obj_t *obj);

isc_result_t
cfg_parse_astring(cfg_parser_t *pctx, const cfg_type_t *type, cfg_obj_t **ret);

isc_result_t
cfg_parse_rawaddr(cfg_parser_t *pctx, unsigned int flags, isc_netaddr_t *na);

void
cfg_print_rawaddr(cfg_printer_t *pctx, const isc_netaddr_t *na);

isc_boolean_t
cfg_lookingat_netaddr(cfg_parser_t *pctx, unsigned int flags);

isc_result_t
cfg_parse_rawport(cfg_parser_t *pctx, unsigned int flags, in_port_t *port);

isc_result_t
cfg_parse_sockaddr(cfg_parser_t *pctx, const cfg_type_t *type, cfg_obj_t **ret);

void
cfg_print_sockaddr(cfg_printer_t *pctx, const cfg_obj_t *obj);

void
cfg_doc_sockaddr(cfg_printer_t *pctx, const cfg_type_t *type);

isc_result_t
cfg_parse_netprefix(cfg_parser_t *pctx, const cfg_type_t *type, cfg_obj_t **ret);

isc_result_t
cfg_parse_special(cfg_parser_t *pctx, int special);
/*%< Parse a required special character 'special'. */

isc_result_t
cfg_create_tuple(cfg_parser_t *pctx, const cfg_type_t *type, cfg_obj_t **objp);

isc_result_t
cfg_parse_tuple(cfg_parser_t *pctx, const cfg_type_t *type, cfg_obj_t **ret);

void
cfg_print_tuple(cfg_printer_t *pctx, const cfg_obj_t *obj);

void
cfg_doc_tuple(cfg_printer_t *pctx, const cfg_type_t *type);

isc_result_t
cfg_create_list(cfg_parser_t *pctx, const cfg_type_t *type, cfg_obj_t **objp);

isc_result_t
cfg_parse_listelt(cfg_parser_t *pctx, const cfg_type_t *elttype,
		  cfg_listelt_t **ret);

isc_result_t
cfg_parse_bracketed_list(cfg_parser_t *pctx, const cfg_type_t *type, cfg_obj_t **ret);

void
cfg_print_bracketed_list(cfg_printer_t *pctx, const cfg_obj_t *obj);

void
cfg_doc_bracketed_list(cfg_printer_t *pctx, const cfg_type_t *type);

isc_result_t
cfg_parse_spacelist(cfg_parser_t *pctx, const cfg_type_t *type, cfg_obj_t **ret);

void
cfg_print_spacelist(cfg_printer_t *pctx, const cfg_obj_t *obj);

isc_result_t
cfg_parse_enum(cfg_parser_t *pctx, const cfg_type_t *type, cfg_obj_t **ret);

void
cfg_doc_enum(cfg_printer_t *pctx, const cfg_type_t *type);

void
cfg_print_chars(cfg_printer_t *pctx, const char *text, int len);
/*%< Print 'len' characters at 'text' */

void
cfg_print_cstr(cfg_printer_t *pctx, const char *s);
/*%< Print the null-terminated string 's' */

isc_result_t
cfg_parse_map(cfg_parser_t *pctx, const cfg_type_t *type, cfg_obj_t **ret);

isc_result_t
cfg_parse_named_map(cfg_parser_t *pctx, const cfg_type_t *type, cfg_obj_t **ret);

isc_result_t
cfg_parse_addressed_map(cfg_parser_t *pctx, const cfg_type_t *type, cfg_obj_t **ret);

isc_result_t
cfg_parse_netprefix_map(cfg_parser_t *pctx, const cfg_type_t *type, cfg_obj_t **
ret);

void
cfg_print_map(cfg_printer_t *pctx, const cfg_obj_t *obj);

void
cfg_doc_map(cfg_printer_t *pctx, const cfg_type_t *type);

isc_result_t
cfg_parse_mapbody(cfg_parser_t *pctx, const cfg_type_t *type, cfg_obj_t **ret);

void
cfg_print_mapbody(cfg_printer_t *pctx, const cfg_obj_t *obj);

void
cfg_doc_mapbody(cfg_printer_t *pctx, const cfg_type_t *type);

isc_result_t
cfg_parse_void(cfg_parser_t *pctx, const cfg_type_t *type, cfg_obj_t **ret);

void
cfg_print_void(cfg_printer_t *pctx, const cfg_obj_t *obj);

void
cfg_doc_void(cfg_printer_t *pctx, const cfg_type_t *type);

isc_result_t
cfg_parse_obj(cfg_parser_t *pctx, const cfg_type_t *type, cfg_obj_t **ret);

void
cfg_print_obj(cfg_printer_t *pctx, const cfg_obj_t *obj);

void
cfg_doc_obj(cfg_printer_t *pctx, const cfg_type_t *type);
/*%<
 * Print a description of the grammar of an arbitrary configuration
 * type 'type'
 */

void
cfg_doc_terminal(cfg_printer_t *pctx, const cfg_type_t *type);
/*%<
 * Document the type 'type' as a terminal by printing its
 * name in angle brackets, e.g., &lt;uint32>.
 */

void
cfg_parser_error(cfg_parser_t *pctx, unsigned int flags,
		 const char *fmt, ...) ISC_FORMAT_PRINTF(3, 4);
/*!
 * Pass one of these flags to cfg_parser_error() to include the
 * token text in log message.
 */
#define CFG_LOG_NEAR    0x00000001	/*%< Say "near <token>" */
#define CFG_LOG_BEFORE  0x00000002	/*%< Say "before <token>" */
#define CFG_LOG_NOPREP  0x00000004	/*%< Say just "<token>" */

void
cfg_parser_warning(cfg_parser_t *pctx, unsigned int flags,
		   const char *fmt, ...) ISC_FORMAT_PRINTF(3, 4);

isc_boolean_t
cfg_is_enum(const char *s, const char *const *enums);
/*%< Return true iff the string 's' is one of the strings in 'enums' */

#endif /* ISCCFG_GRAMMAR_H */
