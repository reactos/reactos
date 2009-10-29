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

/* $Id: namedconf.c,v 1.92 2008/09/27 23:35:31 jinmei Exp $ */

/*! \file */

#include <config.h>

#include <string.h>

#include <isc/lex.h>
#include <isc/result.h>
#include <isc/string.h>
#include <isc/util.h>

#include <isccfg/cfg.h>
#include <isccfg/grammar.h>
#include <isccfg/log.h>

#define TOKEN_STRING(pctx) (pctx->token.value.as_textregion.base)

/*% Check a return value. */
#define CHECK(op) 						\
	do { result = (op); 					\
		if (result != ISC_R_SUCCESS) goto cleanup; 	\
	} while (0)

/*% Clean up a configuration object if non-NULL. */
#define CLEANUP_OBJ(obj) \
	do { if ((obj) != NULL) cfg_obj_destroy(pctx, &(obj)); } while (0)


/*%
 * Forward declarations of static functions.
 */

static isc_result_t
parse_enum_or_other(cfg_parser_t *pctx, const cfg_type_t *enumtype,
		    const cfg_type_t *othertype, cfg_obj_t **ret);

static isc_result_t
parse_keyvalue(cfg_parser_t *pctx, const cfg_type_t *type, cfg_obj_t **ret);

static isc_result_t
parse_optional_keyvalue(cfg_parser_t *pctx, const cfg_type_t *type, cfg_obj_t **ret);

static void
print_keyvalue(cfg_printer_t *pctx, const cfg_obj_t *obj);

static void
doc_keyvalue(cfg_printer_t *pctx, const cfg_type_t *type);

static void
doc_optional_keyvalue(cfg_printer_t *pctx, const cfg_type_t *type);

static cfg_type_t cfg_type_acl;
static cfg_type_t cfg_type_addrmatchelt;
static cfg_type_t cfg_type_bracketed_aml;
static cfg_type_t cfg_type_bracketed_namesockaddrkeylist;
static cfg_type_t cfg_type_bracketed_sockaddrlist;
static cfg_type_t cfg_type_bracketed_sockaddrnameportlist;
static cfg_type_t cfg_type_controls;
static cfg_type_t cfg_type_controls_sockaddr;
static cfg_type_t cfg_type_destinationlist;
static cfg_type_t cfg_type_dialuptype;
static cfg_type_t cfg_type_ixfrdifftype;
static cfg_type_t cfg_type_key;
static cfg_type_t cfg_type_logfile;
static cfg_type_t cfg_type_logging;
static cfg_type_t cfg_type_logseverity;
static cfg_type_t cfg_type_lwres;
static cfg_type_t cfg_type_masterselement;
static cfg_type_t cfg_type_nameportiplist;
static cfg_type_t cfg_type_negated;
static cfg_type_t cfg_type_notifytype;
static cfg_type_t cfg_type_optional_allow;
static cfg_type_t cfg_type_optional_class;
static cfg_type_t cfg_type_optional_facility;
static cfg_type_t cfg_type_optional_keyref;
static cfg_type_t cfg_type_optional_port;
static cfg_type_t cfg_type_options;
static cfg_type_t cfg_type_portiplist;
static cfg_type_t cfg_type_querysource4;
static cfg_type_t cfg_type_querysource6;
static cfg_type_t cfg_type_querysource;
static cfg_type_t cfg_type_server;
static cfg_type_t cfg_type_server_key_kludge;
static cfg_type_t cfg_type_size;
static cfg_type_t cfg_type_sizenodefault;
static cfg_type_t cfg_type_sockaddr4wild;
static cfg_type_t cfg_type_sockaddr6wild;
static cfg_type_t cfg_type_statschannels;
static cfg_type_t cfg_type_view;
static cfg_type_t cfg_type_viewopts;
static cfg_type_t cfg_type_zone;
static cfg_type_t cfg_type_zoneopts;
static cfg_type_t cfg_type_dynamically_loadable_zones;
static cfg_type_t cfg_type_dynamically_loadable_zones_opts;

/*
 * Clauses that can be found in a 'dynamically loadable zones' statement
 */
static cfg_clausedef_t
dynamically_loadable_zones_clauses[] = {
	{ "database", &cfg_type_astring, 0 },
	{ NULL, NULL, 0 }
};

/*
 * A dynamically loadable zones statement.
 */
static cfg_tuplefielddef_t dynamically_loadable_zones_fields[] = {
	{ "name", &cfg_type_astring, 0 },
	{ "options", &cfg_type_dynamically_loadable_zones_opts, 0 },
	{ NULL, NULL, 0 }
};

static cfg_type_t cfg_type_dynamically_loadable_zones = {
	"dlz", cfg_parse_tuple, cfg_print_tuple, cfg_doc_tuple,
	&cfg_rep_tuple,
	dynamically_loadable_zones_fields
	};


/*% tkey-dhkey */

static cfg_tuplefielddef_t tkey_dhkey_fields[] = {
	{ "name", &cfg_type_qstring, 0 },
	{ "keyid", &cfg_type_uint32, 0 },
	{ NULL, NULL, 0 }
};

static cfg_type_t cfg_type_tkey_dhkey = {
	"tkey-dhkey", cfg_parse_tuple, cfg_print_tuple, cfg_doc_tuple, &cfg_rep_tuple,
	tkey_dhkey_fields
};

/*% listen-on */

static cfg_tuplefielddef_t listenon_fields[] = {
	{ "port", &cfg_type_optional_port, 0 },
	{ "acl", &cfg_type_bracketed_aml, 0 },
	{ NULL, NULL, 0 }
};
static cfg_type_t cfg_type_listenon = {
	"listenon", cfg_parse_tuple, cfg_print_tuple, cfg_doc_tuple, &cfg_rep_tuple, listenon_fields };

/*% acl */

static cfg_tuplefielddef_t acl_fields[] = {
	{ "name", &cfg_type_astring, 0 },
	{ "value", &cfg_type_bracketed_aml, 0 },
	{ NULL, NULL, 0 }
};

static cfg_type_t cfg_type_acl = {
	"acl", cfg_parse_tuple, cfg_print_tuple, cfg_doc_tuple, &cfg_rep_tuple, acl_fields };

/*% masters */
static cfg_tuplefielddef_t masters_fields[] = {
	{ "name", &cfg_type_astring, 0 },
	{ "port", &cfg_type_optional_port, 0 },
	{ "addresses", &cfg_type_bracketed_namesockaddrkeylist, 0 },
	{ NULL, NULL, 0 }
};

static cfg_type_t cfg_type_masters = {
	"masters", cfg_parse_tuple, cfg_print_tuple, cfg_doc_tuple, &cfg_rep_tuple, masters_fields };

/*%
 * "sockaddrkeylist", a list of socket addresses with optional keys
 * and an optional default port, as used in the masters option.
 * E.g.,
 *   "port 1234 { mymasters; 10.0.0.1 key foo; 1::2 port 69; }"
 */

static cfg_tuplefielddef_t namesockaddrkey_fields[] = {
	{ "masterselement", &cfg_type_masterselement, 0 },
	{ "key", &cfg_type_optional_keyref, 0 },
	{ NULL, NULL, 0 },
};

static cfg_type_t cfg_type_namesockaddrkey = {
	"namesockaddrkey", cfg_parse_tuple, cfg_print_tuple, cfg_doc_tuple, &cfg_rep_tuple,
	namesockaddrkey_fields
};

static cfg_type_t cfg_type_bracketed_namesockaddrkeylist = {
	"bracketed_namesockaddrkeylist", cfg_parse_bracketed_list,
	cfg_print_bracketed_list, cfg_doc_bracketed_list, &cfg_rep_list, &cfg_type_namesockaddrkey
};

static cfg_tuplefielddef_t namesockaddrkeylist_fields[] = {
	{ "port", &cfg_type_optional_port, 0 },
	{ "addresses", &cfg_type_bracketed_namesockaddrkeylist, 0 },
	{ NULL, NULL, 0 }
};
static cfg_type_t cfg_type_namesockaddrkeylist = {
	"sockaddrkeylist", cfg_parse_tuple, cfg_print_tuple, cfg_doc_tuple, &cfg_rep_tuple,
	namesockaddrkeylist_fields
};

/*%
 * A list of socket addresses with an optional default port,
 * as used in the also-notify option.  E.g.,
 * "port 1234 { 10.0.0.1; 1::2 port 69; }"
 */
static cfg_tuplefielddef_t portiplist_fields[] = {
	{ "port", &cfg_type_optional_port, 0 },
	{ "addresses", &cfg_type_bracketed_sockaddrlist, 0 },
	{ NULL, NULL, 0 }
};
static cfg_type_t cfg_type_portiplist = {
	"portiplist", cfg_parse_tuple, cfg_print_tuple, cfg_doc_tuple, &cfg_rep_tuple,
	portiplist_fields
};

/*%
 * A public key, as in the "pubkey" statement.
 */
static cfg_tuplefielddef_t pubkey_fields[] = {
	{ "flags", &cfg_type_uint32, 0 },
	{ "protocol", &cfg_type_uint32, 0 },
	{ "algorithm", &cfg_type_uint32, 0 },
	{ "key", &cfg_type_qstring, 0 },
	{ NULL, NULL, 0 }
};
static cfg_type_t cfg_type_pubkey = {
	"pubkey", cfg_parse_tuple, cfg_print_tuple, cfg_doc_tuple, &cfg_rep_tuple, pubkey_fields };

/*%
 * A list of RR types, used in grant statements.
 * Note that the old parser allows quotes around the RR type names.
 */
static cfg_type_t cfg_type_rrtypelist = {
	"rrtypelist", cfg_parse_spacelist, cfg_print_spacelist, cfg_doc_terminal,
	&cfg_rep_list, &cfg_type_astring
};

static const char *mode_enums[] = { "grant", "deny", NULL };
static cfg_type_t cfg_type_mode = {
	"mode", cfg_parse_enum, cfg_print_ustring, cfg_doc_enum, &cfg_rep_string,
	&mode_enums
};

static const char *matchtype_enums[] = {
	"name", "subdomain", "wildcard", "self", "selfsub", "selfwild",
	"krb5-self", "ms-self", "krb5-subdomain", "ms-subdomain",
	"tcp-self", "6to4-self", NULL };
static cfg_type_t cfg_type_matchtype = {
	"matchtype", cfg_parse_enum, cfg_print_ustring, cfg_doc_enum, &cfg_rep_string,
	&matchtype_enums
};

/*%
 * A grant statement, used in the update policy.
 */
static cfg_tuplefielddef_t grant_fields[] = {
	{ "mode", &cfg_type_mode, 0 },
	{ "identity", &cfg_type_astring, 0 }, /* domain name */
	{ "matchtype", &cfg_type_matchtype, 0 },
	{ "name", &cfg_type_astring, 0 }, /* domain name */
	{ "types", &cfg_type_rrtypelist, 0 },
	{ NULL, NULL, 0 }
};
static cfg_type_t cfg_type_grant = {
	"grant", cfg_parse_tuple, cfg_print_tuple, cfg_doc_tuple, &cfg_rep_tuple, grant_fields };

static cfg_type_t cfg_type_updatepolicy = {
	"update_policy", cfg_parse_bracketed_list, cfg_print_bracketed_list, cfg_doc_bracketed_list,
	&cfg_rep_list, &cfg_type_grant
};

/*%
 * A view statement.
 */
static cfg_tuplefielddef_t view_fields[] = {
	{ "name", &cfg_type_astring, 0 },
	{ "class", &cfg_type_optional_class, 0 },
	{ "options", &cfg_type_viewopts, 0 },
	{ NULL, NULL, 0 }
};
static cfg_type_t cfg_type_view = {
	"view", cfg_parse_tuple, cfg_print_tuple, cfg_doc_tuple, &cfg_rep_tuple, view_fields };

/*%
 * A zone statement.
 */
static cfg_tuplefielddef_t zone_fields[] = {
	{ "name", &cfg_type_astring, 0 },
	{ "class", &cfg_type_optional_class, 0 },
	{ "options", &cfg_type_zoneopts, 0 },
	{ NULL, NULL, 0 }
};
static cfg_type_t cfg_type_zone = {
	"zone", cfg_parse_tuple, cfg_print_tuple, cfg_doc_tuple, &cfg_rep_tuple, zone_fields };

/*%
 * A "category" clause in the "logging" statement.
 */
static cfg_tuplefielddef_t category_fields[] = {
	{ "name", &cfg_type_astring, 0 },
	{ "destinations", &cfg_type_destinationlist,0 },
	{ NULL, NULL, 0 }
};
static cfg_type_t cfg_type_category = {
	"category", cfg_parse_tuple, cfg_print_tuple, cfg_doc_tuple, &cfg_rep_tuple, category_fields };


/*%
 * A trusted key, as used in the "trusted-keys" statement.
 */
static cfg_tuplefielddef_t trustedkey_fields[] = {
	{ "name", &cfg_type_astring, 0 },
	{ "flags", &cfg_type_uint32, 0 },
	{ "protocol", &cfg_type_uint32, 0 },
	{ "algorithm", &cfg_type_uint32, 0 },
	{ "key", &cfg_type_qstring, 0 },
	{ NULL, NULL, 0 }
};
static cfg_type_t cfg_type_trustedkey = {
	"trustedkey", cfg_parse_tuple, cfg_print_tuple, cfg_doc_tuple, &cfg_rep_tuple,
	trustedkey_fields
};

static keyword_type_t wild_class_kw = { "class", &cfg_type_ustring };

static cfg_type_t cfg_type_optional_wild_class = {
	"optional_wild_class", parse_optional_keyvalue, print_keyvalue,
	doc_optional_keyvalue, &cfg_rep_string, &wild_class_kw
};

static keyword_type_t wild_type_kw = { "type", &cfg_type_ustring };

static cfg_type_t cfg_type_optional_wild_type = {
	"optional_wild_type", parse_optional_keyvalue,
	print_keyvalue, doc_optional_keyvalue, &cfg_rep_string, &wild_type_kw
};

static keyword_type_t wild_name_kw = { "name", &cfg_type_qstring };

static cfg_type_t cfg_type_optional_wild_name = {
	"optional_wild_name", parse_optional_keyvalue,
	print_keyvalue, doc_optional_keyvalue, &cfg_rep_string, &wild_name_kw
};

/*%
 * An rrset ordering element.
 */
static cfg_tuplefielddef_t rrsetorderingelement_fields[] = {
	{ "class", &cfg_type_optional_wild_class, 0 },
	{ "type", &cfg_type_optional_wild_type, 0 },
	{ "name", &cfg_type_optional_wild_name, 0 },
	{ "order", &cfg_type_ustring, 0 }, /* must be literal "order" */
	{ "ordering", &cfg_type_ustring, 0 },
	{ NULL, NULL, 0 }
};
static cfg_type_t cfg_type_rrsetorderingelement = {
	"rrsetorderingelement", cfg_parse_tuple, cfg_print_tuple, cfg_doc_tuple, &cfg_rep_tuple,
	rrsetorderingelement_fields
};

/*%
 * A global or view "check-names" option.  Note that the zone
 * "check-names" option has a different syntax.
 */

static const char *checktype_enums[] = { "master", "slave", "response", NULL };
static cfg_type_t cfg_type_checktype = {
	"checktype", cfg_parse_enum, cfg_print_ustring, cfg_doc_enum,
	&cfg_rep_string, &checktype_enums
};

static const char *checkmode_enums[] = { "fail", "warn", "ignore", NULL };
static cfg_type_t cfg_type_checkmode = {
	"checkmode", cfg_parse_enum, cfg_print_ustring, cfg_doc_enum,
	&cfg_rep_string, &checkmode_enums
};

static cfg_tuplefielddef_t checknames_fields[] = {
	{ "type", &cfg_type_checktype, 0 },
	{ "mode", &cfg_type_checkmode, 0 },
	{ NULL, NULL, 0 }
};
static cfg_type_t cfg_type_checknames = {
	"checknames", cfg_parse_tuple, cfg_print_tuple, cfg_doc_tuple, &cfg_rep_tuple,
	checknames_fields
};

static cfg_type_t cfg_type_bracketed_sockaddrlist = {
	"bracketed_sockaddrlist", cfg_parse_bracketed_list, cfg_print_bracketed_list, cfg_doc_bracketed_list,
	&cfg_rep_list, &cfg_type_sockaddr
};

static cfg_type_t cfg_type_rrsetorder = {
	"rrsetorder", cfg_parse_bracketed_list, cfg_print_bracketed_list, cfg_doc_bracketed_list,
	&cfg_rep_list, &cfg_type_rrsetorderingelement
};

static keyword_type_t port_kw = { "port", &cfg_type_uint32 };

static cfg_type_t cfg_type_optional_port = {
	"optional_port", parse_optional_keyvalue, print_keyvalue,
	doc_optional_keyvalue, &cfg_rep_uint32, &port_kw
};

/*% A list of keys, as in the "key" clause of the controls statement. */
static cfg_type_t cfg_type_keylist = {
	"keylist", cfg_parse_bracketed_list, cfg_print_bracketed_list, cfg_doc_bracketed_list, &cfg_rep_list,
	&cfg_type_astring
};

static cfg_type_t cfg_type_trustedkeys = {
	"trusted-keys", cfg_parse_bracketed_list, cfg_print_bracketed_list, cfg_doc_bracketed_list, &cfg_rep_list,
	&cfg_type_trustedkey
};

static const char *forwardtype_enums[] = { "first", "only", NULL };
static cfg_type_t cfg_type_forwardtype = {
	"forwardtype", cfg_parse_enum, cfg_print_ustring, cfg_doc_enum, &cfg_rep_string,
	&forwardtype_enums
};

static const char *zonetype_enums[] = {
	"master", "slave", "stub", "hint", "forward", "delegation-only", NULL };
static cfg_type_t cfg_type_zonetype = {
	"zonetype", cfg_parse_enum, cfg_print_ustring, cfg_doc_enum,
	&cfg_rep_string, &zonetype_enums
};

static const char *loglevel_enums[] = {
	"critical", "error", "warning", "notice", "info", "dynamic", NULL };
static cfg_type_t cfg_type_loglevel = {
	"loglevel", cfg_parse_enum, cfg_print_ustring, cfg_doc_enum, &cfg_rep_string,
	&loglevel_enums
};

static const char *transferformat_enums[] = {
	"many-answers", "one-answer", NULL };
static cfg_type_t cfg_type_transferformat = {
	"transferformat", cfg_parse_enum, cfg_print_ustring, cfg_doc_enum, &cfg_rep_string,
	&transferformat_enums
};

/*%
 * The special keyword "none", as used in the pid-file option.
 */

static void
print_none(cfg_printer_t *pctx, const cfg_obj_t *obj) {
	UNUSED(obj);
	cfg_print_chars(pctx, "none", 4);
}

static cfg_type_t cfg_type_none = {
	"none", NULL, print_none, NULL, &cfg_rep_void, NULL
};

/*%
 * A quoted string or the special keyword "none".  Used in the pid-file option.
 */
static isc_result_t
parse_qstringornone(cfg_parser_t *pctx, const cfg_type_t *type,
		    cfg_obj_t **ret)
{
	isc_result_t result;
	CHECK(cfg_gettoken(pctx, CFG_LEXOPT_QSTRING));
	if (pctx->token.type == isc_tokentype_string &&
	    strcasecmp(TOKEN_STRING(pctx), "none") == 0)
		return (cfg_create_obj(pctx, &cfg_type_none, ret));
	cfg_ungettoken(pctx);
	return (cfg_parse_qstring(pctx, type, ret));
 cleanup:
	return (result);
}

static void
doc_qstringornone(cfg_printer_t *pctx, const cfg_type_t *type) {
	UNUSED(type);
	cfg_print_chars(pctx, "( <quoted_string> | none )", 26);
}

static cfg_type_t cfg_type_qstringornone = {
	"qstringornone", parse_qstringornone, NULL, doc_qstringornone, NULL, NULL };

/*%
 * keyword hostname
 */

static void
print_hostname(cfg_printer_t *pctx, const cfg_obj_t *obj) {
	UNUSED(obj);
	cfg_print_chars(pctx, "hostname", 4);
}

static cfg_type_t cfg_type_hostname = {
	"hostname", NULL, print_hostname, NULL, &cfg_rep_boolean, NULL
};

/*%
 * "server-id" argument.
 */

static isc_result_t
parse_serverid(cfg_parser_t *pctx, const cfg_type_t *type,
		    cfg_obj_t **ret)
{
	isc_result_t result;
	CHECK(cfg_gettoken(pctx, CFG_LEXOPT_QSTRING));
	if (pctx->token.type == isc_tokentype_string &&
	    strcasecmp(TOKEN_STRING(pctx), "none") == 0)
		return (cfg_create_obj(pctx, &cfg_type_none, ret));
	if (pctx->token.type == isc_tokentype_string &&
	    strcasecmp(TOKEN_STRING(pctx), "hostname") == 0) {
		return (cfg_create_obj(pctx, &cfg_type_hostname, ret));
	}
	cfg_ungettoken(pctx);
	return (cfg_parse_qstring(pctx, type, ret));
 cleanup:
	return (result);
}

static void
doc_serverid(cfg_printer_t *pctx, const cfg_type_t *type) {
	UNUSED(type);
	cfg_print_chars(pctx, "( <quoted_string> | none | hostname )", 26);
}

static cfg_type_t cfg_type_serverid = {
	"serverid", parse_serverid, NULL, doc_serverid, NULL, NULL };

/*%
 * Port list.
 */
static cfg_tuplefielddef_t porttuple_fields[] = {
	{ "loport", &cfg_type_uint32, 0 },
	{ "hiport", &cfg_type_uint32, 0 },
	{ NULL, NULL, 0 }
};
static cfg_type_t cfg_type_porttuple = {
	"porttuple", cfg_parse_tuple, cfg_print_tuple, cfg_doc_tuple,
	&cfg_rep_tuple, porttuple_fields
};

static isc_result_t
parse_port(cfg_parser_t *pctx, cfg_obj_t **ret) {
	isc_result_t result;

	CHECK(cfg_parse_uint32(pctx, NULL, ret));
	if ((*ret)->value.uint32 > 0xffff) {
		cfg_parser_error(pctx, CFG_LOG_NEAR, "invalid port");
		cfg_obj_destroy(pctx, ret);
		result = ISC_R_RANGE;
	}

 cleanup:
	return (result);
}

static isc_result_t
parse_portrange(cfg_parser_t *pctx, const cfg_type_t *type, cfg_obj_t **ret) {
	isc_result_t result;
	cfg_obj_t *obj = NULL;

	UNUSED(type);

	CHECK(cfg_peektoken(pctx, ISC_LEXOPT_NUMBER | ISC_LEXOPT_CNUMBER));
	if (pctx->token.type == isc_tokentype_number)
		CHECK(parse_port(pctx, ret));
	else {
		CHECK(cfg_gettoken(pctx, 0));
		if (pctx->token.type != isc_tokentype_string ||
		    strcasecmp(TOKEN_STRING(pctx), "range") != 0) {
			cfg_parser_error(pctx, CFG_LOG_NEAR,
					 "expected integer or 'range'");
			return (ISC_R_UNEXPECTEDTOKEN);
		}
		CHECK(cfg_create_tuple(pctx, &cfg_type_porttuple, &obj));
		CHECK(parse_port(pctx, &obj->value.tuple[0]));
		CHECK(parse_port(pctx, &obj->value.tuple[1]));
		if (obj->value.tuple[0]->value.uint32 >
		    obj->value.tuple[1]->value.uint32) {
			cfg_parser_error(pctx, CFG_LOG_NOPREP,
					 "low port '%u' must not be larger "
					 "than high port",
					 obj->value.tuple[0]->value.uint32);
			result = ISC_R_RANGE;
			goto cleanup;
		}
		*ret = obj;
		obj = NULL;
	}

 cleanup:
	if (obj != NULL)
		cfg_obj_destroy(pctx, &obj);
	return (result);
}

static cfg_type_t cfg_type_portrange = {
	"portrange", parse_portrange, NULL, cfg_doc_terminal,
	NULL, NULL
};

static cfg_type_t cfg_type_bracketed_portlist = {
	"bracketed_sockaddrlist", cfg_parse_bracketed_list,
	cfg_print_bracketed_list, cfg_doc_bracketed_list,
	&cfg_rep_list, &cfg_type_portrange
};

/*%
 * Clauses that can be found within the top level of the named.conf
 * file only.
 */
static cfg_clausedef_t
namedconf_clauses[] = {
	{ "options", &cfg_type_options, 0 },
	{ "controls", &cfg_type_controls, CFG_CLAUSEFLAG_MULTI },
	{ "acl", &cfg_type_acl, CFG_CLAUSEFLAG_MULTI },
	{ "masters", &cfg_type_masters, CFG_CLAUSEFLAG_MULTI },
	{ "logging", &cfg_type_logging, 0 },
	{ "view", &cfg_type_view, CFG_CLAUSEFLAG_MULTI },
	{ "lwres", &cfg_type_lwres, CFG_CLAUSEFLAG_MULTI },
	{ "statistics-channels", &cfg_type_statschannels,
	  CFG_CLAUSEFLAG_MULTI },
	{ NULL, NULL, 0 }
};

/*%
 * Clauses that can occur at the top level or in the view
 * statement, but not in the options block.
 */
static cfg_clausedef_t
namedconf_or_view_clauses[] = {
	{ "key", &cfg_type_key, CFG_CLAUSEFLAG_MULTI },
	{ "zone", &cfg_type_zone, CFG_CLAUSEFLAG_MULTI },
	/* only 1 DLZ per view allowed */
	{ "dlz", &cfg_type_dynamically_loadable_zones, 0 },
	{ "server", &cfg_type_server, CFG_CLAUSEFLAG_MULTI },
	{ "trusted-keys", &cfg_type_trustedkeys, CFG_CLAUSEFLAG_MULTI },
	{ NULL, NULL, 0 }
};

/*%
 * Clauses that can be found within the 'options' statement.
 */
static cfg_clausedef_t
options_clauses[] = {
	{ "use-v4-udp-ports", &cfg_type_bracketed_portlist, 0 },
	{ "use-v6-udp-ports", &cfg_type_bracketed_portlist, 0 },
	{ "avoid-v4-udp-ports", &cfg_type_bracketed_portlist, 0 },
	{ "avoid-v6-udp-ports", &cfg_type_bracketed_portlist, 0 },
	{ "blackhole", &cfg_type_bracketed_aml, 0 },
	{ "coresize", &cfg_type_size, 0 },
	{ "datasize", &cfg_type_size, 0 },
	{ "deallocate-on-exit", &cfg_type_boolean, CFG_CLAUSEFLAG_OBSOLETE },
	{ "directory", &cfg_type_qstring, CFG_CLAUSEFLAG_CALLBACK },
	{ "dump-file", &cfg_type_qstring, 0 },
	{ "fake-iquery", &cfg_type_boolean, CFG_CLAUSEFLAG_OBSOLETE },
	{ "files", &cfg_type_size, 0 },
	{ "has-old-clients", &cfg_type_boolean, CFG_CLAUSEFLAG_OBSOLETE },
	{ "heartbeat-interval", &cfg_type_uint32, 0 },
	{ "host-statistics", &cfg_type_boolean, CFG_CLAUSEFLAG_NOTIMP },
	{ "host-statistics-max", &cfg_type_uint32, CFG_CLAUSEFLAG_NOTIMP },
	{ "hostname", &cfg_type_qstringornone, 0 },
	{ "interface-interval", &cfg_type_uint32, 0 },
	{ "listen-on", &cfg_type_listenon, CFG_CLAUSEFLAG_MULTI },
	{ "listen-on-v6", &cfg_type_listenon, CFG_CLAUSEFLAG_MULTI },
	{ "match-mapped-addresses", &cfg_type_boolean, 0 },
	{ "memstatistics-file", &cfg_type_qstring, 0 },
	{ "memstatistics", &cfg_type_boolean, 0 },
	{ "multiple-cnames", &cfg_type_boolean, CFG_CLAUSEFLAG_OBSOLETE },
	{ "named-xfer", &cfg_type_qstring, CFG_CLAUSEFLAG_OBSOLETE },
	{ "pid-file", &cfg_type_qstringornone, 0 },
	{ "port", &cfg_type_uint32, 0 },
	{ "querylog", &cfg_type_boolean, 0 },
	{ "recursing-file", &cfg_type_qstring, 0 },
	{ "random-device", &cfg_type_qstring, 0 },
	{ "recursive-clients", &cfg_type_uint32, 0 },
	{ "reserved-sockets", &cfg_type_uint32, 0 },
	{ "serial-queries", &cfg_type_uint32, CFG_CLAUSEFLAG_OBSOLETE },
	{ "serial-query-rate", &cfg_type_uint32, 0 },
	{ "server-id", &cfg_type_serverid, 0 },
	{ "stacksize", &cfg_type_size, 0 },
	{ "statistics-file", &cfg_type_qstring, 0 },
	{ "statistics-interval", &cfg_type_uint32, CFG_CLAUSEFLAG_NYI },
	{ "tcp-clients", &cfg_type_uint32, 0 },
	{ "tcp-listen-queue", &cfg_type_uint32, 0 },
	{ "tkey-dhkey", &cfg_type_tkey_dhkey, 0 },
	{ "tkey-gssapi-credential", &cfg_type_qstring, 0 },
	{ "tkey-domain", &cfg_type_qstring, 0 },
	{ "transfers-per-ns", &cfg_type_uint32, 0 },
	{ "transfers-in", &cfg_type_uint32, 0 },
	{ "transfers-out", &cfg_type_uint32, 0 },
	{ "treat-cr-as-space", &cfg_type_boolean, CFG_CLAUSEFLAG_OBSOLETE },
	{ "use-id-pool", &cfg_type_boolean, CFG_CLAUSEFLAG_OBSOLETE },
	{ "use-ixfr", &cfg_type_boolean, 0 },
	{ "version", &cfg_type_qstringornone, 0 },
	{ "flush-zones-on-shutdown", &cfg_type_boolean, 0 },
	{ NULL, NULL, 0 }
};


static cfg_type_t cfg_type_namelist = {
	"namelist", cfg_parse_bracketed_list, cfg_print_bracketed_list,
	cfg_doc_bracketed_list, &cfg_rep_list, &cfg_type_qstring };

static keyword_type_t exclude_kw = { "exclude", &cfg_type_namelist };

static cfg_type_t cfg_type_optional_exclude = {
	"optional_exclude", parse_optional_keyvalue, print_keyvalue,
	doc_optional_keyvalue, &cfg_rep_list, &exclude_kw };

static cfg_type_t cfg_type_algorithmlist = {
	"algorithmlist", cfg_parse_bracketed_list, cfg_print_bracketed_list,
	cfg_doc_bracketed_list, &cfg_rep_list, &cfg_type_astring };

static cfg_tuplefielddef_t disablealgorithm_fields[] = {
	{ "name", &cfg_type_astring, 0 },
	{ "algorithms", &cfg_type_algorithmlist, 0 },
	{ NULL, NULL, 0 }
};

static cfg_type_t cfg_type_disablealgorithm = {
	"disablealgorithm", cfg_parse_tuple, cfg_print_tuple, cfg_doc_tuple,
	&cfg_rep_tuple, disablealgorithm_fields
};

static cfg_tuplefielddef_t mustbesecure_fields[] = {
	{ "name", &cfg_type_astring, 0 },
	{ "value", &cfg_type_boolean, 0 },
	{ NULL, NULL, 0 }
};

static cfg_type_t cfg_type_mustbesecure = {
	"mustbesecure", cfg_parse_tuple, cfg_print_tuple, cfg_doc_tuple,
	&cfg_rep_tuple, mustbesecure_fields
};

static const char *masterformat_enums[] = { "text", "raw", NULL };
static cfg_type_t cfg_type_masterformat = {
	"masterformat", cfg_parse_enum, cfg_print_ustring, cfg_doc_enum,
	&cfg_rep_string, &masterformat_enums
};

/*%
 * dnssec-lookaside
 */

static keyword_type_t trustanchor_kw = { "trust-anchor", &cfg_type_astring };

static cfg_type_t cfg_type_trustanchor = {
	"trust-anchor", parse_keyvalue, print_keyvalue, doc_keyvalue,
	&cfg_rep_string, &trustanchor_kw
};

static cfg_tuplefielddef_t lookaside_fields[] = {
	{ "domain", &cfg_type_astring, 0 },
	{ "trust-anchor", &cfg_type_trustanchor, 0 },
	{ NULL, NULL, 0 }
};

static cfg_type_t cfg_type_lookaside = {
	"lookaside", cfg_parse_tuple, cfg_print_tuple, cfg_doc_tuple,
	&cfg_rep_tuple, lookaside_fields
};

/*%
 * Clauses that can be found within the 'view' statement,
 * with defaults in the 'options' statement.
 */

static cfg_clausedef_t
view_clauses[] = {
	{ "acache-cleaning-interval", &cfg_type_uint32, 0 },
	{ "acache-enable", &cfg_type_boolean, 0 },
	{ "additional-from-auth", &cfg_type_boolean, 0 },
	{ "additional-from-cache", &cfg_type_boolean, 0 },
	{ "allow-query-cache", &cfg_type_bracketed_aml, 0 },
	{ "allow-query-cache-on", &cfg_type_bracketed_aml, 0 },
	{ "allow-recursion", &cfg_type_bracketed_aml, 0 },
	{ "allow-recursion-on", &cfg_type_bracketed_aml, 0 },
	{ "allow-v6-synthesis", &cfg_type_bracketed_aml,
	  CFG_CLAUSEFLAG_OBSOLETE },
	{ "auth-nxdomain", &cfg_type_boolean, CFG_CLAUSEFLAG_NEWDEFAULT },
	{ "cache-file", &cfg_type_qstring, 0 },
	{ "check-names", &cfg_type_checknames, CFG_CLAUSEFLAG_MULTI },
	{ "cleaning-interval", &cfg_type_uint32, 0 },
	{ "clients-per-query", &cfg_type_uint32, 0 },
	{ "disable-algorithms", &cfg_type_disablealgorithm,
	  CFG_CLAUSEFLAG_MULTI },
	{ "disable-empty-zone", &cfg_type_astring, CFG_CLAUSEFLAG_MULTI },
	{ "dnssec-accept-expired", &cfg_type_boolean, 0 },
	{ "dnssec-enable", &cfg_type_boolean, 0 },
	{ "dnssec-lookaside", &cfg_type_lookaside, CFG_CLAUSEFLAG_MULTI },
	{ "dnssec-must-be-secure",  &cfg_type_mustbesecure,
	  CFG_CLAUSEFLAG_MULTI },
	{ "dnssec-validation", &cfg_type_boolean, 0 },
	{ "dual-stack-servers", &cfg_type_nameportiplist, 0 },
	{ "edns-udp-size", &cfg_type_uint32, 0 },
	{ "empty-contact", &cfg_type_astring, 0 },
	{ "empty-server", &cfg_type_astring, 0 },
	{ "empty-zones-enable", &cfg_type_boolean, 0 },
	{ "fetch-glue", &cfg_type_boolean, CFG_CLAUSEFLAG_OBSOLETE },
	{ "ixfr-from-differences", &cfg_type_ixfrdifftype, 0 },
	{ "lame-ttl", &cfg_type_uint32, 0 },
	{ "max-acache-size", &cfg_type_sizenodefault, 0 },
	{ "max-cache-size", &cfg_type_sizenodefault, 0 },
	{ "max-cache-ttl", &cfg_type_uint32, 0 },
	{ "max-clients-per-query", &cfg_type_uint32, 0 },
	{ "max-ncache-ttl", &cfg_type_uint32, 0 },
	{ "max-udp-size", &cfg_type_uint32, 0 },
	{ "min-roots", &cfg_type_uint32, CFG_CLAUSEFLAG_NOTIMP },
	{ "minimal-responses", &cfg_type_boolean, 0 },
	{ "preferred-glue", &cfg_type_astring, 0 },
	{ "provide-ixfr", &cfg_type_boolean, 0 },
	/*
	 * Note that the query-source option syntax is different
	 * from the other -source options.
	 */
	{ "query-source", &cfg_type_querysource4, 0 },
	{ "query-source-v6", &cfg_type_querysource6, 0 },
	{ "queryport-pool-ports", &cfg_type_uint32, CFG_CLAUSEFLAG_OBSOLETE },
	{ "queryport-pool-updateinterval", &cfg_type_uint32,
	  CFG_CLAUSEFLAG_OBSOLETE },
	{ "recursion", &cfg_type_boolean, 0 },
	{ "request-ixfr", &cfg_type_boolean, 0 },
	{ "request-nsid", &cfg_type_boolean, 0 },
	{ "rfc2308-type1", &cfg_type_boolean, CFG_CLAUSEFLAG_NYI },
	{ "root-delegation-only",  &cfg_type_optional_exclude, 0 },
	{ "rrset-order", &cfg_type_rrsetorder, 0 },
	{ "sortlist", &cfg_type_bracketed_aml, 0 },
	{ "suppress-initial-notify", &cfg_type_boolean, CFG_CLAUSEFLAG_NYI },
	{ "topology", &cfg_type_bracketed_aml, CFG_CLAUSEFLAG_NOTIMP },
	{ "transfer-format", &cfg_type_transferformat, 0 },
	{ "use-queryport-pool", &cfg_type_boolean, CFG_CLAUSEFLAG_OBSOLETE },
	{ "zero-no-soa-ttl-cache", &cfg_type_boolean, 0 },
	{ NULL, NULL, 0 }
};

/*%
 * Clauses that can be found within the 'view' statement only.
 */
static cfg_clausedef_t
view_only_clauses[] = {
	{ "match-clients", &cfg_type_bracketed_aml, 0 },
	{ "match-destinations", &cfg_type_bracketed_aml, 0 },
	{ "match-recursive-only", &cfg_type_boolean, 0 },
	{ NULL, NULL, 0 }
};

/*%
 * Sig-validity-interval.
 */
static isc_result_t
parse_optional_uint32(cfg_parser_t *pctx, const cfg_type_t *type,
		      cfg_obj_t **ret)
{
	isc_result_t result;
	UNUSED(type);

	CHECK(cfg_peektoken(pctx, ISC_LEXOPT_NUMBER | ISC_LEXOPT_CNUMBER));
	if (pctx->token.type == isc_tokentype_number) {
		CHECK(cfg_parse_obj(pctx, &cfg_type_uint32, ret));
	} else {
		CHECK(cfg_parse_obj(pctx, &cfg_type_void, ret));
	}
 cleanup:
	return (result);
}

static void
doc_optional_uint32(cfg_printer_t *pctx, const cfg_type_t *type) {
	UNUSED(type);
	cfg_print_chars(pctx, "[ <integer> ]", 13);
}

static cfg_type_t cfg_type_optional_uint32 = {
	"optional_uint32", parse_optional_uint32, NULL, doc_optional_uint32,
	NULL, NULL };

static cfg_tuplefielddef_t validityinterval_fields[] = {
	{ "validity", &cfg_type_uint32, 0 },
	{ "re-sign", &cfg_type_optional_uint32, 0 },
	{ NULL, NULL, 0 }
};

static cfg_type_t cfg_type_validityinterval = {
	"validityinterval", cfg_parse_tuple, cfg_print_tuple, cfg_doc_tuple,
	&cfg_rep_tuple, validityinterval_fields
};

/*%
 * Clauses that can be found in a 'zone' statement,
 * with defaults in the 'view' or 'options' statement.
 */
static cfg_clausedef_t
zone_clauses[] = {
	{ "allow-notify", &cfg_type_bracketed_aml, 0 },
	{ "allow-query", &cfg_type_bracketed_aml, 0 },
	{ "allow-query-on", &cfg_type_bracketed_aml, 0 },
	{ "allow-transfer", &cfg_type_bracketed_aml, 0 },
	{ "allow-update", &cfg_type_bracketed_aml, 0 },
	{ "allow-update-forwarding", &cfg_type_bracketed_aml, 0 },
	{ "also-notify", &cfg_type_portiplist, 0 },
	{ "alt-transfer-source", &cfg_type_sockaddr4wild, 0 },
	{ "alt-transfer-source-v6", &cfg_type_sockaddr6wild, 0 },
	{ "check-integrity", &cfg_type_boolean, 0 },
	{ "check-mx", &cfg_type_checkmode, 0 },
	{ "check-mx-cname", &cfg_type_checkmode, 0 },
	{ "check-sibling", &cfg_type_boolean, 0 },
	{ "check-srv-cname", &cfg_type_checkmode, 0 },
	{ "check-wildcard", &cfg_type_boolean, 0 },
	{ "dialup", &cfg_type_dialuptype, 0 },
	{ "forward", &cfg_type_forwardtype, 0 },
	{ "forwarders", &cfg_type_portiplist, 0 },
	{ "key-directory", &cfg_type_qstring, 0 },
	{ "maintain-ixfr-base", &cfg_type_boolean, CFG_CLAUSEFLAG_OBSOLETE },
	{ "masterfile-format", &cfg_type_masterformat, 0 },
	{ "max-ixfr-log-size", &cfg_type_size, CFG_CLAUSEFLAG_OBSOLETE },
	{ "max-journal-size", &cfg_type_sizenodefault, 0 },
	{ "max-refresh-time", &cfg_type_uint32, 0 },
	{ "max-retry-time", &cfg_type_uint32, 0 },
	{ "max-transfer-idle-in", &cfg_type_uint32, 0 },
	{ "max-transfer-idle-out", &cfg_type_uint32, 0 },
	{ "max-transfer-time-in", &cfg_type_uint32, 0 },
	{ "max-transfer-time-out", &cfg_type_uint32, 0 },
	{ "min-refresh-time", &cfg_type_uint32, 0 },
	{ "min-retry-time", &cfg_type_uint32, 0 },
	{ "multi-master", &cfg_type_boolean, 0 },
	{ "notify", &cfg_type_notifytype, 0 },
	{ "notify-delay", &cfg_type_uint32, 0 },
	{ "notify-source", &cfg_type_sockaddr4wild, 0 },
	{ "notify-source-v6", &cfg_type_sockaddr6wild, 0 },
	{ "notify-to-soa", &cfg_type_boolean, 0 },
	{ "nsec3-test-zone", &cfg_type_boolean, CFG_CLAUSEFLAG_TESTONLY },
	{ "sig-signing-nodes", &cfg_type_uint32, 0 },
	{ "sig-signing-signatures", &cfg_type_uint32, 0 },
	{ "sig-signing-type", &cfg_type_uint32, 0 },
	{ "sig-validity-interval", &cfg_type_validityinterval, 0 },
	{ "transfer-source", &cfg_type_sockaddr4wild, 0 },
	{ "transfer-source-v6", &cfg_type_sockaddr6wild, 0 },
	{ "try-tcp-refresh", &cfg_type_boolean, 0 },
	{ "update-check-ksk", &cfg_type_boolean, 0 },
	{ "use-alt-transfer-source", &cfg_type_boolean, 0 },
	{ "zero-no-soa-ttl", &cfg_type_boolean, 0 },
	{ "zone-statistics", &cfg_type_boolean, 0 },
	{ NULL, NULL, 0 }
};

/*%
 * Clauses that can be found in a 'zone' statement
 * only.
 */
static cfg_clausedef_t
zone_only_clauses[] = {
	{ "type", &cfg_type_zonetype, 0 },
	{ "file", &cfg_type_qstring, 0 },
	{ "journal", &cfg_type_qstring, 0 },
	{ "ixfr-base", &cfg_type_qstring, CFG_CLAUSEFLAG_OBSOLETE },
	{ "ixfr-tmp-file", &cfg_type_qstring, CFG_CLAUSEFLAG_OBSOLETE },
	{ "masters", &cfg_type_namesockaddrkeylist, 0 },
	{ "pubkey", &cfg_type_pubkey,
	  CFG_CLAUSEFLAG_MULTI | CFG_CLAUSEFLAG_OBSOLETE },
	{ "update-policy", &cfg_type_updatepolicy, 0 },
	{ "database", &cfg_type_astring, 0 },
	{ "delegation-only", &cfg_type_boolean, 0 },
	/*
	 * Note that the format of the check-names option is different between
	 * the zone options and the global/view options.  Ugh.
	 */
	{ "check-names", &cfg_type_checkmode, 0 },
	{ "ixfr-from-differences", &cfg_type_boolean, 0 },
	{ NULL, NULL, 0 }
};


/*% The top-level named.conf syntax. */

static cfg_clausedef_t *
namedconf_clausesets[] = {
	namedconf_clauses,
	namedconf_or_view_clauses,
	NULL
};

LIBISCCFG_EXTERNAL_DATA cfg_type_t cfg_type_namedconf = {
	"namedconf", cfg_parse_mapbody, cfg_print_mapbody, cfg_doc_mapbody,
	&cfg_rep_map, namedconf_clausesets
};

/*% The "options" statement syntax. */

static cfg_clausedef_t *
options_clausesets[] = {
	options_clauses,
	view_clauses,
	zone_clauses,
	NULL
};
static cfg_type_t cfg_type_options = {
	"options", cfg_parse_map, cfg_print_map, cfg_doc_map, &cfg_rep_map, options_clausesets };

/*% The "view" statement syntax. */

static cfg_clausedef_t *
view_clausesets[] = {
	view_only_clauses,
	namedconf_or_view_clauses,
	view_clauses,
	zone_clauses,
	dynamically_loadable_zones_clauses,
	NULL
};
static cfg_type_t cfg_type_viewopts = {
	"view", cfg_parse_map, cfg_print_map, cfg_doc_map, &cfg_rep_map, view_clausesets };

/*% The "zone" statement syntax. */

static cfg_clausedef_t *
zone_clausesets[] = {
	zone_only_clauses,
	zone_clauses,
	NULL
};
static cfg_type_t cfg_type_zoneopts = {
	"zoneopts", cfg_parse_map, cfg_print_map,
	cfg_doc_map, &cfg_rep_map, zone_clausesets };

/*% The "dynamically loadable zones" statement syntax. */

static cfg_clausedef_t *
dynamically_loadable_zones_clausesets[] = {
	dynamically_loadable_zones_clauses,
	NULL
};
static cfg_type_t cfg_type_dynamically_loadable_zones_opts = {
	"dynamically_loadable_zones_opts", cfg_parse_map,
	cfg_print_map, cfg_doc_map, &cfg_rep_map,
	dynamically_loadable_zones_clausesets
};

/*%
 * Clauses that can be found within the 'key' statement.
 */
static cfg_clausedef_t
key_clauses[] = {
	{ "algorithm", &cfg_type_astring, 0 },
	{ "secret", &cfg_type_astring, 0 },
	{ NULL, NULL, 0 }
};

static cfg_clausedef_t *
key_clausesets[] = {
	key_clauses,
	NULL
};
static cfg_type_t cfg_type_key = {
	"key", cfg_parse_named_map, cfg_print_map,
	cfg_doc_map, &cfg_rep_map, key_clausesets
};


/*%
 * Clauses that can be found in a 'server' statement.
 */
static cfg_clausedef_t
server_clauses[] = {
	{ "bogus", &cfg_type_boolean, 0 },
	{ "provide-ixfr", &cfg_type_boolean, 0 },
	{ "request-ixfr", &cfg_type_boolean, 0 },
	{ "support-ixfr", &cfg_type_boolean, CFG_CLAUSEFLAG_OBSOLETE },
	{ "transfers", &cfg_type_uint32, 0 },
	{ "transfer-format", &cfg_type_transferformat, 0 },
	{ "keys", &cfg_type_server_key_kludge, 0 },
	{ "edns", &cfg_type_boolean, 0 },
	{ "edns-udp-size", &cfg_type_uint32, 0 },
	{ "max-udp-size", &cfg_type_uint32, 0 },
	{ "notify-source", &cfg_type_sockaddr4wild, 0 },
	{ "notify-source-v6", &cfg_type_sockaddr6wild, 0 },
	{ "query-source", &cfg_type_querysource4, 0 },
	{ "query-source-v6", &cfg_type_querysource6, 0 },
	{ "transfer-source", &cfg_type_sockaddr4wild, 0 },
	{ "transfer-source-v6", &cfg_type_sockaddr6wild, 0 },
	{ NULL, NULL, 0 }
};
static cfg_clausedef_t *
server_clausesets[] = {
	server_clauses,
	NULL
};
static cfg_type_t cfg_type_server = {
	"server", cfg_parse_netprefix_map, cfg_print_map, cfg_doc_map, &cfg_rep_map,
	server_clausesets
};


/*%
 * Clauses that can be found in a 'channel' clause in the
 * 'logging' statement.
 *
 * These have some additional constraints that need to be
 * checked after parsing:
 *  - There must exactly one of file/syslog/null/stderr
 *
 */
static cfg_clausedef_t
channel_clauses[] = {
	/* Destinations.  We no longer require these to be first. */
	{ "file", &cfg_type_logfile, 0 },
	{ "syslog", &cfg_type_optional_facility, 0 },
	{ "null", &cfg_type_void, 0 },
	{ "stderr", &cfg_type_void, 0 },
	/* Options.  We now accept these for the null channel, too. */
	{ "severity", &cfg_type_logseverity, 0 },
	{ "print-time", &cfg_type_boolean, 0 },
	{ "print-severity", &cfg_type_boolean, 0 },
	{ "print-category", &cfg_type_boolean, 0 },
	{ NULL, NULL, 0 }
};
static cfg_clausedef_t *
channel_clausesets[] = {
	channel_clauses,
	NULL
};
static cfg_type_t cfg_type_channel = {
	"channel", cfg_parse_named_map, cfg_print_map, cfg_doc_map,
	&cfg_rep_map, channel_clausesets
};

/*% A list of log destination, used in the "category" clause. */
static cfg_type_t cfg_type_destinationlist = {
	"destinationlist", cfg_parse_bracketed_list, cfg_print_bracketed_list, cfg_doc_bracketed_list,
	&cfg_rep_list, &cfg_type_astring };

/*%
 * Clauses that can be found in a 'logging' statement.
 */
static cfg_clausedef_t
logging_clauses[] = {
	{ "channel", &cfg_type_channel, CFG_CLAUSEFLAG_MULTI },
	{ "category", &cfg_type_category, CFG_CLAUSEFLAG_MULTI },
	{ NULL, NULL, 0 }
};
static cfg_clausedef_t *
logging_clausesets[] = {
	logging_clauses,
	NULL
};
static cfg_type_t cfg_type_logging = {
	"logging", cfg_parse_map, cfg_print_map, cfg_doc_map, &cfg_rep_map, logging_clausesets };


static isc_result_t
parse_unitstring(char *str, isc_resourcevalue_t *valuep) {
	char *endp;
	unsigned int len;
	isc_uint64_t value;
	isc_uint64_t unit;

	value = isc_string_touint64(str, &endp, 10);
	if (*endp == 0) {
		*valuep = value;
		return (ISC_R_SUCCESS);
	}

	len = strlen(str);
	if (len < 2 || endp[1] != '\0')
		return (ISC_R_FAILURE);

	switch (str[len - 1]) {
	case 'k':
	case 'K':
		unit = 1024;
		break;
	case 'm':
	case 'M':
		unit = 1024 * 1024;
		break;
	case 'g':
	case 'G':
		unit = 1024 * 1024 * 1024;
		break;
	default:
		return (ISC_R_FAILURE);
	}
	if (value > ISC_UINT64_MAX / unit)
		return (ISC_R_FAILURE);
	*valuep = value * unit;
	return (ISC_R_SUCCESS);
}

static isc_result_t
parse_sizeval(cfg_parser_t *pctx, const cfg_type_t *type, cfg_obj_t **ret) {
	isc_result_t result;
	cfg_obj_t *obj = NULL;
	isc_uint64_t val;

	UNUSED(type);

	CHECK(cfg_gettoken(pctx, 0));
	if (pctx->token.type != isc_tokentype_string) {
		result = ISC_R_UNEXPECTEDTOKEN;
		goto cleanup;
	}
	CHECK(parse_unitstring(TOKEN_STRING(pctx), &val));

	CHECK(cfg_create_obj(pctx, &cfg_type_uint64, &obj));
	obj->value.uint64 = val;
	*ret = obj;
	return (ISC_R_SUCCESS);

 cleanup:
	cfg_parser_error(pctx, CFG_LOG_NEAR, "expected integer and optional unit");
	return (result);
}

/*%
 * A size value (number + optional unit).
 */
static cfg_type_t cfg_type_sizeval = {
	"sizeval", parse_sizeval, cfg_print_uint64, cfg_doc_terminal,
	&cfg_rep_uint64, NULL };

/*%
 * A size, "unlimited", or "default".
 */

static isc_result_t
parse_size(cfg_parser_t *pctx, const cfg_type_t *type, cfg_obj_t **ret) {
	return (parse_enum_or_other(pctx, type, &cfg_type_sizeval, ret));
}

static const char *size_enums[] = { "unlimited", "default", NULL };
static cfg_type_t cfg_type_size = {
	"size", parse_size, cfg_print_ustring, cfg_doc_terminal,
	&cfg_rep_string, size_enums
};

/*%
 * A size or "unlimited", but not "default".
 */
static const char *sizenodefault_enums[] = { "unlimited", NULL };
static cfg_type_t cfg_type_sizenodefault = {
	"size_no_default", parse_size, cfg_print_ustring, cfg_doc_terminal,
	&cfg_rep_string, sizenodefault_enums
};

/*%
 * optional_keyvalue
 */
static isc_result_t
parse_maybe_optional_keyvalue(cfg_parser_t *pctx, const cfg_type_t *type,
			      isc_boolean_t optional, cfg_obj_t **ret)
{
	isc_result_t result;
	cfg_obj_t *obj = NULL;
	const keyword_type_t *kw = type->of;

	CHECK(cfg_peektoken(pctx, 0));
	if (pctx->token.type == isc_tokentype_string &&
	    strcasecmp(TOKEN_STRING(pctx), kw->name) == 0) {
		CHECK(cfg_gettoken(pctx, 0));
		CHECK(kw->type->parse(pctx, kw->type, &obj));
		obj->type = type; /* XXX kludge */
	} else {
		if (optional) {
			CHECK(cfg_parse_void(pctx, NULL, &obj));
		} else {
			cfg_parser_error(pctx, CFG_LOG_NEAR, "expected '%s'",
				     kw->name);
			result = ISC_R_UNEXPECTEDTOKEN;
			goto cleanup;
		}
	}
	*ret = obj;
 cleanup:
	return (result);
}

static isc_result_t
parse_enum_or_other(cfg_parser_t *pctx, const cfg_type_t *enumtype,
		    const cfg_type_t *othertype, cfg_obj_t **ret)
{
	isc_result_t result;
	CHECK(cfg_peektoken(pctx, 0));
	if (pctx->token.type == isc_tokentype_string &&
	    cfg_is_enum(TOKEN_STRING(pctx), enumtype->of)) {
		CHECK(cfg_parse_enum(pctx, enumtype, ret));
	} else {
		CHECK(cfg_parse_obj(pctx, othertype, ret));
	}
 cleanup:
	return (result);
}

static void
doc_enum_or_other(cfg_printer_t *pctx, const cfg_type_t *type) {
	cfg_doc_terminal(pctx, type);
#if 0 /* XXX */
	cfg_print_chars(pctx, "( ", 2);...
#endif

}

static isc_result_t
parse_keyvalue(cfg_parser_t *pctx, const cfg_type_t *type, cfg_obj_t **ret) {
	return (parse_maybe_optional_keyvalue(pctx, type, ISC_FALSE, ret));
}

static isc_result_t
parse_optional_keyvalue(cfg_parser_t *pctx, const cfg_type_t *type, cfg_obj_t **ret) {
	return (parse_maybe_optional_keyvalue(pctx, type, ISC_TRUE, ret));
}

static void
print_keyvalue(cfg_printer_t *pctx, const cfg_obj_t *obj) {
	const keyword_type_t *kw = obj->type->of;
	cfg_print_cstr(pctx, kw->name);
	cfg_print_chars(pctx, " ", 1);
	kw->type->print(pctx, obj);
}

static void
doc_keyvalue(cfg_printer_t *pctx, const cfg_type_t *type) {
	const keyword_type_t *kw = type->of;
	cfg_print_cstr(pctx, kw->name);
	cfg_print_chars(pctx, " ", 1);
	cfg_doc_obj(pctx, kw->type);
}

static void
doc_optional_keyvalue(cfg_printer_t *pctx, const cfg_type_t *type) {
	const keyword_type_t *kw = type->of;
	cfg_print_chars(pctx, "[ ", 2);
	cfg_print_cstr(pctx, kw->name);
	cfg_print_chars(pctx, " ", 1);
	cfg_doc_obj(pctx, kw->type);
	cfg_print_chars(pctx, " ]", 2);
}

static const char *dialup_enums[] = {
	"notify", "notify-passive", "refresh", "passive", NULL };
static isc_result_t
parse_dialup_type(cfg_parser_t *pctx, const cfg_type_t *type, cfg_obj_t **ret) {
	return (parse_enum_or_other(pctx, type, &cfg_type_boolean, ret));
}
static cfg_type_t cfg_type_dialuptype = {
	"dialuptype", parse_dialup_type, cfg_print_ustring, doc_enum_or_other,
	&cfg_rep_string, dialup_enums
};

static const char *notify_enums[] = { "explicit", "master-only", NULL };
static isc_result_t
parse_notify_type(cfg_parser_t *pctx, const cfg_type_t *type, cfg_obj_t **ret) {
	return (parse_enum_or_other(pctx, type, &cfg_type_boolean, ret));
}
static cfg_type_t cfg_type_notifytype = {
	"notifytype", parse_notify_type, cfg_print_ustring, doc_enum_or_other,
	&cfg_rep_string, notify_enums,
};

static const char *ixfrdiff_enums[] = { "master", "slave", NULL };
static isc_result_t
parse_ixfrdiff_type(cfg_parser_t *pctx, const cfg_type_t *type, cfg_obj_t **ret) {
	return (parse_enum_or_other(pctx, type, &cfg_type_boolean, ret));
}
static cfg_type_t cfg_type_ixfrdifftype = {
	"ixfrdiff", parse_ixfrdiff_type, cfg_print_ustring, doc_enum_or_other,
	&cfg_rep_string, ixfrdiff_enums,
};

static keyword_type_t key_kw = { "key", &cfg_type_astring };

LIBISCCFG_EXTERNAL_DATA cfg_type_t cfg_type_keyref = {
	"keyref", parse_keyvalue, print_keyvalue, doc_keyvalue,
	&cfg_rep_string, &key_kw
};

static cfg_type_t cfg_type_optional_keyref = {
	"optional_keyref", parse_optional_keyvalue, print_keyvalue,
	doc_optional_keyvalue, &cfg_rep_string, &key_kw
};

/*%
 * A "controls" statement is represented as a map with the multivalued
 * "inet" and "unix" clauses.
 */

static keyword_type_t controls_allow_kw = {
	"allow", &cfg_type_bracketed_aml };

static cfg_type_t cfg_type_controls_allow = {
	"controls_allow", parse_keyvalue,
	print_keyvalue, doc_keyvalue,
	&cfg_rep_list, &controls_allow_kw
};

static keyword_type_t controls_keys_kw = {
	"keys", &cfg_type_keylist };

static cfg_type_t cfg_type_controls_keys = {
	"controls_keys", parse_optional_keyvalue,
	print_keyvalue, doc_optional_keyvalue,
	&cfg_rep_list, &controls_keys_kw
};

static cfg_tuplefielddef_t inetcontrol_fields[] = {
	{ "address", &cfg_type_controls_sockaddr, 0 },
	{ "allow", &cfg_type_controls_allow, 0 },
	{ "keys", &cfg_type_controls_keys, 0 },
	{ NULL, NULL, 0 }
};

static cfg_type_t cfg_type_inetcontrol = {
	"inetcontrol", cfg_parse_tuple, cfg_print_tuple, cfg_doc_tuple, &cfg_rep_tuple,
	inetcontrol_fields
};

static keyword_type_t controls_perm_kw = {
	"perm", &cfg_type_uint32 };

static cfg_type_t cfg_type_controls_perm = {
	"controls_perm", parse_keyvalue,
	print_keyvalue, doc_keyvalue,
	&cfg_rep_uint32, &controls_perm_kw
};

static keyword_type_t controls_owner_kw = {
	"owner", &cfg_type_uint32 };

static cfg_type_t cfg_type_controls_owner = {
	"controls_owner", parse_keyvalue,
	print_keyvalue, doc_keyvalue,
	&cfg_rep_uint32, &controls_owner_kw
};

static keyword_type_t controls_group_kw = {
	"group", &cfg_type_uint32 };

static cfg_type_t cfg_type_controls_group = {
	"controls_allow", parse_keyvalue,
	print_keyvalue, doc_keyvalue,
	&cfg_rep_uint32, &controls_group_kw
};

static cfg_tuplefielddef_t unixcontrol_fields[] = {
	{ "path", &cfg_type_qstring, 0 },
	{ "perm", &cfg_type_controls_perm, 0 },
	{ "owner", &cfg_type_controls_owner, 0 },
	{ "group", &cfg_type_controls_group, 0 },
	{ "keys", &cfg_type_controls_keys, 0 },
	{ NULL, NULL, 0 }
};

static cfg_type_t cfg_type_unixcontrol = {
	"unixcontrol", cfg_parse_tuple, cfg_print_tuple, cfg_doc_tuple, &cfg_rep_tuple,
	unixcontrol_fields
};

static cfg_clausedef_t
controls_clauses[] = {
	{ "inet", &cfg_type_inetcontrol, CFG_CLAUSEFLAG_MULTI },
	{ "unix", &cfg_type_unixcontrol, CFG_CLAUSEFLAG_MULTI },
	{ NULL, NULL, 0 }
};

static cfg_clausedef_t *
controls_clausesets[] = {
	controls_clauses,
	NULL
};
static cfg_type_t cfg_type_controls = {
	"controls", cfg_parse_map, cfg_print_map, cfg_doc_map, &cfg_rep_map,	&controls_clausesets
};

/*%
 * A "statistics-channels" statement is represented as a map with the
 * multivalued "inet" clauses.
 */
static void
doc_optional_bracketed_list(cfg_printer_t *pctx, const cfg_type_t *type) {
	const keyword_type_t *kw = type->of;
	cfg_print_chars(pctx, "[ ", 2);
	cfg_print_cstr(pctx, kw->name);
	cfg_print_chars(pctx, " ", 1);
	cfg_doc_obj(pctx, kw->type);
	cfg_print_chars(pctx, " ]", 2);
}

static cfg_type_t cfg_type_optional_allow = {
	"optional_allow", parse_optional_keyvalue, print_keyvalue,
	doc_optional_bracketed_list, &cfg_rep_list, &controls_allow_kw
};

static cfg_tuplefielddef_t statserver_fields[] = {
	{ "address", &cfg_type_controls_sockaddr, 0 }, /* reuse controls def */
	{ "allow", &cfg_type_optional_allow, 0 },
	{ NULL, NULL, 0 }
};

static cfg_type_t cfg_type_statschannel = {
	"statschannel", cfg_parse_tuple, cfg_print_tuple, cfg_doc_tuple,
	&cfg_rep_tuple, statserver_fields
};

static cfg_clausedef_t
statservers_clauses[] = {
	{ "inet", &cfg_type_statschannel, CFG_CLAUSEFLAG_MULTI },
	{ NULL, NULL, 0 }
};

static cfg_clausedef_t *
statservers_clausesets[] = {
	statservers_clauses,
	NULL
};

static cfg_type_t cfg_type_statschannels = {
	"statistics-channels", cfg_parse_map, cfg_print_map, cfg_doc_map,
	&cfg_rep_map,	&statservers_clausesets
};

/*%
 * An optional class, as used in view and zone statements.
 */
static isc_result_t
parse_optional_class(cfg_parser_t *pctx, const cfg_type_t *type, cfg_obj_t **ret) {
	isc_result_t result;
	UNUSED(type);
	CHECK(cfg_peektoken(pctx, 0));
	if (pctx->token.type == isc_tokentype_string)
		CHECK(cfg_parse_obj(pctx, &cfg_type_ustring, ret));
	else
		CHECK(cfg_parse_obj(pctx, &cfg_type_void, ret));
 cleanup:
	return (result);
}

static cfg_type_t cfg_type_optional_class = {
	"optional_class", parse_optional_class, NULL, cfg_doc_terminal,
	NULL, NULL
};

static isc_result_t
parse_querysource(cfg_parser_t *pctx, const cfg_type_t *type, cfg_obj_t **ret) {
	isc_result_t result;
	cfg_obj_t *obj = NULL;
	isc_netaddr_t netaddr;
	in_port_t port;
	unsigned int have_address = 0;
	unsigned int have_port = 0;
	const unsigned int *flagp = type->of;

	if ((*flagp & CFG_ADDR_V4OK) != 0)
		isc_netaddr_any(&netaddr);
	else if ((*flagp & CFG_ADDR_V6OK) != 0)
		isc_netaddr_any6(&netaddr);
	else
		INSIST(0);

	port = 0;

	for (;;) {
		CHECK(cfg_peektoken(pctx, 0));
		if (pctx->token.type == isc_tokentype_string) {
			if (strcasecmp(TOKEN_STRING(pctx),
				       "address") == 0)
			{
				/* read "address" */
				CHECK(cfg_gettoken(pctx, 0));
				CHECK(cfg_parse_rawaddr(pctx, *flagp,
							&netaddr));
				have_address++;
			} else if (strcasecmp(TOKEN_STRING(pctx), "port") == 0)
			{
				/* read "port" */
				CHECK(cfg_gettoken(pctx, 0));
				CHECK(cfg_parse_rawport(pctx,
							CFG_ADDR_WILDOK,
							&port));
				have_port++;
			} else if (have_port == 0 && have_address == 0) {
				return (cfg_parse_sockaddr(pctx, type, ret));
			} else {
				cfg_parser_error(pctx, CFG_LOG_NEAR,
					     "expected 'address' or 'port'");
				return (ISC_R_UNEXPECTEDTOKEN);
			}
		} else
			break;
	}
	if (have_address > 1 || have_port > 1 ||
	    have_address + have_port == 0) {
		cfg_parser_error(pctx, 0, "expected one address and/or port");
		return (ISC_R_UNEXPECTEDTOKEN);
	}

	CHECK(cfg_create_obj(pctx, &cfg_type_querysource, &obj));
	isc_sockaddr_fromnetaddr(&obj->value.sockaddr, &netaddr, port);
	*ret = obj;
	return (ISC_R_SUCCESS);

 cleanup:
	cfg_parser_error(pctx, CFG_LOG_NEAR, "invalid query source");
	CLEANUP_OBJ(obj);
	return (result);
}

static void
print_querysource(cfg_printer_t *pctx, const cfg_obj_t *obj) {
	isc_netaddr_t na;
	isc_netaddr_fromsockaddr(&na, &obj->value.sockaddr);
	cfg_print_chars(pctx, "address ", 8);
	cfg_print_rawaddr(pctx, &na);
	cfg_print_chars(pctx, " port ", 6);
	cfg_print_rawuint(pctx, isc_sockaddr_getport(&obj->value.sockaddr));
}

static unsigned int sockaddr4wild_flags = CFG_ADDR_WILDOK | CFG_ADDR_V4OK;
static unsigned int sockaddr6wild_flags = CFG_ADDR_WILDOK | CFG_ADDR_V6OK;

static cfg_type_t cfg_type_querysource4 = {
	"querysource4", parse_querysource, NULL, cfg_doc_terminal,
	NULL, &sockaddr4wild_flags
};

static cfg_type_t cfg_type_querysource6 = {
	"querysource6", parse_querysource, NULL, cfg_doc_terminal,
	NULL, &sockaddr6wild_flags
};

static cfg_type_t cfg_type_querysource = {
	"querysource", NULL, print_querysource, NULL, &cfg_rep_sockaddr, NULL
};

/*% addrmatchelt */

static isc_result_t
parse_addrmatchelt(cfg_parser_t *pctx, const cfg_type_t *type, cfg_obj_t **ret) {
	isc_result_t result;
	UNUSED(type);

	CHECK(cfg_peektoken(pctx, CFG_LEXOPT_QSTRING));

	if (pctx->token.type == isc_tokentype_string ||
	    pctx->token.type == isc_tokentype_qstring) {
		if (pctx->token.type == isc_tokentype_string &&
		    (strcasecmp(TOKEN_STRING(pctx), "key") == 0)) {
			CHECK(cfg_parse_obj(pctx, &cfg_type_keyref, ret));
		} else {
			if (cfg_lookingat_netaddr(pctx, CFG_ADDR_V4OK |
						  CFG_ADDR_V4PREFIXOK |
						  CFG_ADDR_V6OK))
			{
				CHECK(cfg_parse_netprefix(pctx, NULL, ret));
			} else {
				CHECK(cfg_parse_astring(pctx, NULL, ret));
			}
		}
	} else if (pctx->token.type == isc_tokentype_special) {
		if (pctx->token.value.as_char == '{') {
			/* Nested match list. */
			CHECK(cfg_parse_obj(pctx, &cfg_type_bracketed_aml, ret));
		} else if (pctx->token.value.as_char == '!') {
			CHECK(cfg_gettoken(pctx, 0)); /* read "!" */
			CHECK(cfg_parse_obj(pctx, &cfg_type_negated, ret));
		} else {
			goto bad;
		}
	} else {
	bad:
		cfg_parser_error(pctx, CFG_LOG_NEAR,
			     "expected IP match list element");
		return (ISC_R_UNEXPECTEDTOKEN);
	}
 cleanup:
	return (result);
}

/*%
 * A negated address match list element (like "! 10.0.0.1").
 * Somewhat sneakily, the caller is expected to parse the
 * "!", but not to print it.
 */

static cfg_tuplefielddef_t negated_fields[] = {
	{ "value", &cfg_type_addrmatchelt, 0 },
	{ NULL, NULL, 0 }
};

static void
print_negated(cfg_printer_t *pctx, const cfg_obj_t *obj) {
	cfg_print_chars(pctx, "!", 1);
	cfg_print_tuple(pctx, obj);
}

static cfg_type_t cfg_type_negated = {
	"negated", cfg_parse_tuple, print_negated, NULL, &cfg_rep_tuple,
	&negated_fields
};

/*% An address match list element */

static cfg_type_t cfg_type_addrmatchelt = {
	"address_match_element", parse_addrmatchelt, NULL, cfg_doc_terminal,
	NULL, NULL
};

/*% A bracketed address match list */

static cfg_type_t cfg_type_bracketed_aml = {
	"bracketed_aml", cfg_parse_bracketed_list, cfg_print_bracketed_list,
	cfg_doc_bracketed_list, &cfg_rep_list, &cfg_type_addrmatchelt
};

/*%
 * The socket address syntax in the "controls" statement is silly.
 * It allows both socket address families, but also allows "*",
 * whis is gratuitously interpreted as the IPv4 wildcard address.
 */
static unsigned int controls_sockaddr_flags =
	CFG_ADDR_V4OK | CFG_ADDR_V6OK | CFG_ADDR_WILDOK;
static cfg_type_t cfg_type_controls_sockaddr = {
	"controls_sockaddr", cfg_parse_sockaddr, cfg_print_sockaddr,
	cfg_doc_sockaddr, &cfg_rep_sockaddr, &controls_sockaddr_flags
};

/*%
 * Handle the special kludge syntax of the "keys" clause in the "server"
 * statement, which takes a single key with or without braces and semicolon.
 */
static isc_result_t
parse_server_key_kludge(cfg_parser_t *pctx, const cfg_type_t *type, cfg_obj_t **ret)
{
	isc_result_t result;
	isc_boolean_t braces = ISC_FALSE;
	UNUSED(type);

	/* Allow opening brace. */
	CHECK(cfg_peektoken(pctx, 0));
	if (pctx->token.type == isc_tokentype_special &&
	    pctx->token.value.as_char == '{') {
		result = cfg_gettoken(pctx, 0);
		braces = ISC_TRUE;
	}

	CHECK(cfg_parse_obj(pctx, &cfg_type_astring, ret));

	if (braces) {
		/* Skip semicolon if present. */
		CHECK(cfg_peektoken(pctx, 0));
		if (pctx->token.type == isc_tokentype_special &&
		    pctx->token.value.as_char == ';')
			CHECK(cfg_gettoken(pctx, 0));

		CHECK(cfg_parse_special(pctx, '}'));
	}
 cleanup:
	return (result);
}
static cfg_type_t cfg_type_server_key_kludge = {
	"server_key", parse_server_key_kludge, NULL, cfg_doc_terminal,
	NULL, NULL
};


/*%
 * An optional logging facility.
 */

static isc_result_t
parse_optional_facility(cfg_parser_t *pctx, const cfg_type_t *type, cfg_obj_t **ret)
{
	isc_result_t result;
	UNUSED(type);

	CHECK(cfg_peektoken(pctx, CFG_LEXOPT_QSTRING));
	if (pctx->token.type == isc_tokentype_string ||
	    pctx->token.type == isc_tokentype_qstring) {
		CHECK(cfg_parse_obj(pctx, &cfg_type_astring, ret));
	} else {
		CHECK(cfg_parse_obj(pctx, &cfg_type_void, ret));
	}
 cleanup:
	return (result);
}

static cfg_type_t cfg_type_optional_facility = {
	"optional_facility", parse_optional_facility, NULL, cfg_doc_terminal,
	NULL, NULL };


/*%
 * A log severity.  Return as a string, except "debug N",
 * which is returned as a keyword object.
 */

static keyword_type_t debug_kw = { "debug", &cfg_type_uint32 };
static cfg_type_t cfg_type_debuglevel = {
	"debuglevel", parse_keyvalue,
	print_keyvalue, doc_keyvalue,
	&cfg_rep_uint32, &debug_kw
};

static isc_result_t
parse_logseverity(cfg_parser_t *pctx, const cfg_type_t *type, cfg_obj_t **ret) {
	isc_result_t result;
	UNUSED(type);

	CHECK(cfg_peektoken(pctx, 0));
	if (pctx->token.type == isc_tokentype_string &&
	    strcasecmp(TOKEN_STRING(pctx), "debug") == 0) {
		CHECK(cfg_gettoken(pctx, 0)); /* read "debug" */
		CHECK(cfg_peektoken(pctx, ISC_LEXOPT_NUMBER));
		if (pctx->token.type == isc_tokentype_number) {
			CHECK(cfg_parse_uint32(pctx, NULL, ret));
		} else {
			/*
			 * The debug level is optional and defaults to 1.
			 * This makes little sense, but we support it for
			 * compatibility with BIND 8.
			 */
			CHECK(cfg_create_obj(pctx, &cfg_type_uint32, ret));
			(*ret)->value.uint32 = 1;
		}
		(*ret)->type = &cfg_type_debuglevel; /* XXX kludge */
	} else {
		CHECK(cfg_parse_obj(pctx, &cfg_type_loglevel, ret));
	}
 cleanup:
	return (result);
}

static cfg_type_t cfg_type_logseverity = {
	"log_severity", parse_logseverity, NULL, cfg_doc_terminal,
	NULL, NULL };

/*%
 * The "file" clause of the "channel" statement.
 * This is yet another special case.
 */

static const char *logversions_enums[] = { "unlimited", NULL };
static isc_result_t
parse_logversions(cfg_parser_t *pctx, const cfg_type_t *type, cfg_obj_t **ret) {
	return (parse_enum_or_other(pctx, type, &cfg_type_uint32, ret));
}

static cfg_type_t cfg_type_logversions = {
	"logversions", parse_logversions, cfg_print_ustring, cfg_doc_terminal,
	&cfg_rep_string, logversions_enums
};

static cfg_tuplefielddef_t logfile_fields[] = {
	{ "file", &cfg_type_qstring, 0 },
	{ "versions", &cfg_type_logversions, 0 },
	{ "size", &cfg_type_size, 0 },
	{ NULL, NULL, 0 }
};

static isc_result_t
parse_logfile(cfg_parser_t *pctx, const cfg_type_t *type, cfg_obj_t **ret) {
	isc_result_t result;
	cfg_obj_t *obj = NULL;
	const cfg_tuplefielddef_t *fields = type->of;

	CHECK(cfg_create_tuple(pctx, type, &obj));

	/* Parse the mandatory "file" field */
	CHECK(cfg_parse_obj(pctx, fields[0].type, &obj->value.tuple[0]));

	/* Parse "versions" and "size" fields in any order. */
	for (;;) {
		CHECK(cfg_peektoken(pctx, 0));
		if (pctx->token.type == isc_tokentype_string) {
			CHECK(cfg_gettoken(pctx, 0));
			if (strcasecmp(TOKEN_STRING(pctx),
				       "versions") == 0 &&
			    obj->value.tuple[1] == NULL) {
				CHECK(cfg_parse_obj(pctx, fields[1].type,
					    &obj->value.tuple[1]));
			} else if (strcasecmp(TOKEN_STRING(pctx),
					      "size") == 0 &&
				   obj->value.tuple[2] == NULL) {
				CHECK(cfg_parse_obj(pctx, fields[2].type,
					    &obj->value.tuple[2]));
			} else {
				break;
			}
		} else {
			break;
		}
	}

	/* Create void objects for missing optional values. */
	if (obj->value.tuple[1] == NULL)
		CHECK(cfg_parse_void(pctx, NULL, &obj->value.tuple[1]));
	if (obj->value.tuple[2] == NULL)
		CHECK(cfg_parse_void(pctx, NULL, &obj->value.tuple[2]));

	*ret = obj;
	return (ISC_R_SUCCESS);

 cleanup:
	CLEANUP_OBJ(obj);
	return (result);
}

static void
print_logfile(cfg_printer_t *pctx, const cfg_obj_t *obj) {
	cfg_print_obj(pctx, obj->value.tuple[0]); /* file */
	if (obj->value.tuple[1]->type->print != cfg_print_void) {
		cfg_print_chars(pctx, " versions ", 10);
		cfg_print_obj(pctx, obj->value.tuple[1]);
	}
	if (obj->value.tuple[2]->type->print != cfg_print_void) {
		cfg_print_chars(pctx, " size ", 6);
		cfg_print_obj(pctx, obj->value.tuple[2]);
	}
}


static void
doc_logfile(cfg_printer_t *pctx, const cfg_type_t *type) {
	UNUSED(type);
	cfg_print_cstr(pctx, "<quoted_string>");
	cfg_print_chars(pctx, " ", 1);
	cfg_print_cstr(pctx, "[ versions ( \"unlimited\" | <integer> ) ]");
	cfg_print_chars(pctx, " ", 1);
	cfg_print_cstr(pctx, "[ size <size> ]");
}

static cfg_type_t cfg_type_logfile = {
	"log_file", parse_logfile, print_logfile, doc_logfile,
	&cfg_rep_tuple, logfile_fields
};

/*% An IPv4 address with optional port, "*" accepted as wildcard. */
static cfg_type_t cfg_type_sockaddr4wild = {
	"sockaddr4wild", cfg_parse_sockaddr, cfg_print_sockaddr,
	cfg_doc_sockaddr, &cfg_rep_sockaddr, &sockaddr4wild_flags
};

/*% An IPv6 address with optional port, "*" accepted as wildcard. */
static cfg_type_t cfg_type_sockaddr6wild = {
	"v6addrportwild", cfg_parse_sockaddr, cfg_print_sockaddr,
	cfg_doc_sockaddr, &cfg_rep_sockaddr, &sockaddr6wild_flags
};

/*%
 * lwres
 */

static cfg_tuplefielddef_t lwres_view_fields[] = {
	{ "name", &cfg_type_astring, 0 },
	{ "class", &cfg_type_optional_class, 0 },
	{ NULL, NULL, 0 }
};
static cfg_type_t cfg_type_lwres_view = {
	"lwres_view", cfg_parse_tuple, cfg_print_tuple, cfg_doc_tuple, &cfg_rep_tuple,
	lwres_view_fields
};

static cfg_type_t cfg_type_lwres_searchlist = {
	"lwres_searchlist", cfg_parse_bracketed_list, cfg_print_bracketed_list,
	cfg_doc_bracketed_list, &cfg_rep_list, &cfg_type_astring };

static cfg_clausedef_t
lwres_clauses[] = {
	{ "listen-on", &cfg_type_portiplist, 0 },
	{ "view", &cfg_type_lwres_view, 0 },
	{ "search", &cfg_type_lwres_searchlist, 0 },
	{ "ndots", &cfg_type_uint32, 0 },
	{ NULL, NULL, 0 }
};

static cfg_clausedef_t *
lwres_clausesets[] = {
	lwres_clauses,
	NULL
};
static cfg_type_t cfg_type_lwres = {
	"lwres", cfg_parse_map, cfg_print_map, cfg_doc_map, &cfg_rep_map,
	lwres_clausesets
};

/*%
 * rndc
 */

static cfg_clausedef_t
rndcconf_options_clauses[] = {
	{ "default-key", &cfg_type_astring, 0 },
	{ "default-port", &cfg_type_uint32, 0 },
	{ "default-server", &cfg_type_astring, 0 },
	{ "default-source-address", &cfg_type_netaddr4wild, 0 },
	{ "default-source-address-v6", &cfg_type_netaddr6wild, 0 },
	{ NULL, NULL, 0 }
};

static cfg_clausedef_t *
rndcconf_options_clausesets[] = {
	rndcconf_options_clauses,
	NULL
};

static cfg_type_t cfg_type_rndcconf_options = {
	"rndcconf_options", cfg_parse_map, cfg_print_map, cfg_doc_map,
	&cfg_rep_map, rndcconf_options_clausesets
};

static cfg_clausedef_t
rndcconf_server_clauses[] = {
	{ "key", &cfg_type_astring, 0 },
	{ "port", &cfg_type_uint32, 0 },
	{ "source-address", &cfg_type_netaddr4wild, 0 },
	{ "source-address-v6", &cfg_type_netaddr6wild, 0 },
	{ "addresses", &cfg_type_bracketed_sockaddrnameportlist, 0 },
	{ NULL, NULL, 0 }
};

static cfg_clausedef_t *
rndcconf_server_clausesets[] = {
	rndcconf_server_clauses,
	NULL
};

static cfg_type_t cfg_type_rndcconf_server = {
	"rndcconf_server", cfg_parse_named_map, cfg_print_map, cfg_doc_map,
	&cfg_rep_map, rndcconf_server_clausesets
};

static cfg_clausedef_t
rndcconf_clauses[] = {
	{ "key", &cfg_type_key, CFG_CLAUSEFLAG_MULTI },
	{ "server", &cfg_type_rndcconf_server, CFG_CLAUSEFLAG_MULTI },
	{ "options", &cfg_type_rndcconf_options, 0 },
	{ NULL, NULL, 0 }
};

static cfg_clausedef_t *
rndcconf_clausesets[] = {
	rndcconf_clauses,
	NULL
};

LIBISCCFG_EXTERNAL_DATA cfg_type_t cfg_type_rndcconf = {
	"rndcconf", cfg_parse_mapbody, cfg_print_mapbody, cfg_doc_mapbody,
	&cfg_rep_map, rndcconf_clausesets
};

static cfg_clausedef_t
rndckey_clauses[] = {
	{ "key", &cfg_type_key, 0 },
	{ NULL, NULL, 0 }
};

static cfg_clausedef_t *
rndckey_clausesets[] = {
	rndckey_clauses,
	NULL
};

LIBISCCFG_EXTERNAL_DATA cfg_type_t cfg_type_rndckey = {
	"rndckey", cfg_parse_mapbody, cfg_print_mapbody, cfg_doc_mapbody,
	&cfg_rep_map, rndckey_clausesets
};

static cfg_tuplefielddef_t nameport_fields[] = {
	{ "name", &cfg_type_astring, 0 },
	{ "port", &cfg_type_optional_port, 0 },
	{ NULL, NULL, 0 }
};
static cfg_type_t cfg_type_nameport = {
	"nameport", cfg_parse_tuple, cfg_print_tuple, cfg_doc_tuple,
	&cfg_rep_tuple, nameport_fields
};

static void
doc_sockaddrnameport(cfg_printer_t *pctx, const cfg_type_t *type) {
	UNUSED(type);
	cfg_print_chars(pctx, "( ", 2);
	cfg_print_cstr(pctx, "<quoted_string>");
	cfg_print_chars(pctx, " ", 1);
	cfg_print_cstr(pctx, "[ port <integer> ]");
	cfg_print_chars(pctx, " | ", 3);
	cfg_print_cstr(pctx, "<ipv4_address>");
	cfg_print_chars(pctx, " ", 1);
	cfg_print_cstr(pctx, "[ port <integer> ]");
	cfg_print_chars(pctx, " | ", 3);
	cfg_print_cstr(pctx, "<ipv6_address>");
	cfg_print_chars(pctx, " ", 1);
	cfg_print_cstr(pctx, "[ port <integer> ]");
	cfg_print_chars(pctx, " )", 2);
}

static isc_result_t
parse_sockaddrnameport(cfg_parser_t *pctx, const cfg_type_t *type,
		       cfg_obj_t **ret)
{
	isc_result_t result;
	cfg_obj_t *obj = NULL;
	UNUSED(type);

	CHECK(cfg_peektoken(pctx, CFG_LEXOPT_QSTRING));
	if (pctx->token.type == isc_tokentype_string ||
	    pctx->token.type == isc_tokentype_qstring) {
		if (cfg_lookingat_netaddr(pctx, CFG_ADDR_V4OK | CFG_ADDR_V6OK))
			CHECK(cfg_parse_sockaddr(pctx, &cfg_type_sockaddr, ret));
		else {
			const cfg_tuplefielddef_t *fields =
						   cfg_type_nameport.of;
			CHECK(cfg_create_tuple(pctx, &cfg_type_nameport,
					       &obj));
			CHECK(cfg_parse_obj(pctx, fields[0].type,
					    &obj->value.tuple[0]));
			CHECK(cfg_parse_obj(pctx, fields[1].type,
					    &obj->value.tuple[1]));
			*ret = obj;
			obj = NULL;
		}
	} else {
		cfg_parser_error(pctx, CFG_LOG_NEAR,
			     "expected IP address or hostname");
		return (ISC_R_UNEXPECTEDTOKEN);
	}
 cleanup:
	CLEANUP_OBJ(obj);
	return (result);
}

static cfg_type_t cfg_type_sockaddrnameport = {
	"sockaddrnameport_element", parse_sockaddrnameport, NULL,
	 doc_sockaddrnameport, NULL, NULL
};

static cfg_type_t cfg_type_bracketed_sockaddrnameportlist = {
	"bracketed_sockaddrnameportlist", cfg_parse_bracketed_list,
	cfg_print_bracketed_list, cfg_doc_bracketed_list,
	&cfg_rep_list, &cfg_type_sockaddrnameport
};

/*%
 * A list of socket addresses or name with an optional default port,
 * as used in the dual-stack-servers option.  E.g.,
 * "port 1234 { dual-stack-servers.net; 10.0.0.1; 1::2 port 69; }"
 */
static cfg_tuplefielddef_t nameportiplist_fields[] = {
	{ "port", &cfg_type_optional_port, 0 },
	{ "addresses", &cfg_type_bracketed_sockaddrnameportlist, 0 },
	{ NULL, NULL, 0 }
};

static cfg_type_t cfg_type_nameportiplist = {
	"nameportiplist", cfg_parse_tuple, cfg_print_tuple, cfg_doc_tuple,
	&cfg_rep_tuple, nameportiplist_fields
};

/*%
 * masters element.
 */

static void
doc_masterselement(cfg_printer_t *pctx, const cfg_type_t *type) {
	UNUSED(type);
	cfg_print_chars(pctx, "( ", 2);
	cfg_print_cstr(pctx, "<masters>");
	cfg_print_chars(pctx, " | ", 3);
	cfg_print_cstr(pctx, "<ipv4_address>");
	cfg_print_chars(pctx, " ", 1);
	cfg_print_cstr(pctx, "[ port <integer> ]");
	cfg_print_chars(pctx, " | ", 3);
	cfg_print_cstr(pctx, "<ipv6_address>");
	cfg_print_chars(pctx, " ", 1);
	cfg_print_cstr(pctx, "[ port <integer> ]");
	cfg_print_chars(pctx, " )", 2);
}

static isc_result_t
parse_masterselement(cfg_parser_t *pctx, const cfg_type_t *type,
		     cfg_obj_t **ret)
{
	isc_result_t result;
	cfg_obj_t *obj = NULL;
	UNUSED(type);

	CHECK(cfg_peektoken(pctx, CFG_LEXOPT_QSTRING));
	if (pctx->token.type == isc_tokentype_string ||
	    pctx->token.type == isc_tokentype_qstring) {
		if (cfg_lookingat_netaddr(pctx, CFG_ADDR_V4OK | CFG_ADDR_V6OK))
			CHECK(cfg_parse_sockaddr(pctx, &cfg_type_sockaddr, ret));
		else
			CHECK(cfg_parse_astring(pctx, &cfg_type_astring, ret));
	} else {
		cfg_parser_error(pctx, CFG_LOG_NEAR,
			     "expected IP address or masters name");
		return (ISC_R_UNEXPECTEDTOKEN);
	}
 cleanup:
	CLEANUP_OBJ(obj);
	return (result);
}

static cfg_type_t cfg_type_masterselement = {
	"masters_element", parse_masterselement, NULL,
	 doc_masterselement, NULL, NULL
};
