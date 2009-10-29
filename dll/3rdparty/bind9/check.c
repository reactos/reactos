/*
 * Copyright (C) 2004-2009  Internet Systems Consortium, Inc. ("ISC")
 * Copyright (C) 2001-2003  Internet Software Consortium.
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

/* $Id: check.c,v 1.95.12.4 2009/06/03 00:06:01 marka Exp $ */

/*! \file */

#include <config.h>

#include <stdlib.h>

#include <isc/buffer.h>
#include <isc/log.h>
#include <isc/mem.h>
#include <isc/netaddr.h>
#include <isc/parseint.h>
#include <isc/region.h>
#include <isc/result.h>
#include <isc/sockaddr.h>
#include <isc/string.h>
#include <isc/symtab.h>
#include <isc/util.h>

#include <dns/acl.h>
#include <dns/fixedname.h>
#include <dns/rdataclass.h>
#include <dns/rdatatype.h>
#include <dns/secalg.h>

#include <isccfg/aclconf.h>
#include <isccfg/cfg.h>

#include <bind9/check.h>

static void
freekey(char *key, unsigned int type, isc_symvalue_t value, void *userarg) {
	UNUSED(type);
	UNUSED(value);
	isc_mem_free(userarg, key);
}

static isc_result_t
check_orderent(const cfg_obj_t *ent, isc_log_t *logctx) {
	isc_result_t result = ISC_R_SUCCESS;
	isc_result_t tresult;
	isc_textregion_t r;
	dns_fixedname_t fixed;
	const cfg_obj_t *obj;
	dns_rdataclass_t rdclass;
	dns_rdatatype_t rdtype;
	isc_buffer_t b;
	const char *str;

	dns_fixedname_init(&fixed);
	obj = cfg_tuple_get(ent, "class");
	if (cfg_obj_isstring(obj)) {

		DE_CONST(cfg_obj_asstring(obj), r.base);
		r.length = strlen(r.base);
		tresult = dns_rdataclass_fromtext(&rdclass, &r);
		if (tresult != ISC_R_SUCCESS) {
			cfg_obj_log(obj, logctx, ISC_LOG_ERROR,
				    "rrset-order: invalid class '%s'",
				    r.base);
			result = ISC_R_FAILURE;
		}
	}

	obj = cfg_tuple_get(ent, "type");
	if (cfg_obj_isstring(obj)) {

		DE_CONST(cfg_obj_asstring(obj), r.base);
		r.length = strlen(r.base);
		tresult = dns_rdatatype_fromtext(&rdtype, &r);
		if (tresult != ISC_R_SUCCESS) {
			cfg_obj_log(obj, logctx, ISC_LOG_ERROR,
				    "rrset-order: invalid type '%s'",
				    r.base);
			result = ISC_R_FAILURE;
		}
	}

	obj = cfg_tuple_get(ent, "name");
	if (cfg_obj_isstring(obj)) {
		str = cfg_obj_asstring(obj);
		isc_buffer_init(&b, str, strlen(str));
		isc_buffer_add(&b, strlen(str));
		tresult = dns_name_fromtext(dns_fixedname_name(&fixed), &b,
					    dns_rootname, ISC_FALSE, NULL);
		if (tresult != ISC_R_SUCCESS) {
			cfg_obj_log(obj, logctx, ISC_LOG_ERROR,
				    "rrset-order: invalid name '%s'", str);
			result = ISC_R_FAILURE;
		}
	}

	obj = cfg_tuple_get(ent, "order");
	if (!cfg_obj_isstring(obj) ||
	    strcasecmp("order", cfg_obj_asstring(obj)) != 0) {
		cfg_obj_log(ent, logctx, ISC_LOG_ERROR,
			    "rrset-order: keyword 'order' missing");
		result = ISC_R_FAILURE;
	}

	obj = cfg_tuple_get(ent, "ordering");
	if (!cfg_obj_isstring(obj)) {
	    cfg_obj_log(ent, logctx, ISC_LOG_ERROR,
			"rrset-order: missing ordering");
		result = ISC_R_FAILURE;
	} else if (strcasecmp(cfg_obj_asstring(obj), "fixed") == 0) {
#if !DNS_RDATASET_FIXED
		cfg_obj_log(obj, logctx, ISC_LOG_WARNING,
			    "rrset-order: order 'fixed' was disabled at "
			    "compilation time");
#endif
	} else if (strcasecmp(cfg_obj_asstring(obj), "random") != 0 &&
		   strcasecmp(cfg_obj_asstring(obj), "cyclic") != 0) {
		cfg_obj_log(obj, logctx, ISC_LOG_ERROR,
			    "rrset-order: invalid order '%s'",
			    cfg_obj_asstring(obj));
		result = ISC_R_FAILURE;
	}
	return (result);
}

static isc_result_t
check_order(const cfg_obj_t *options, isc_log_t *logctx) {
	isc_result_t result = ISC_R_SUCCESS;
	isc_result_t tresult;
	const cfg_listelt_t *element;
	const cfg_obj_t *obj = NULL;

	if (cfg_map_get(options, "rrset-order", &obj) != ISC_R_SUCCESS)
		return (result);

	for (element = cfg_list_first(obj);
	     element != NULL;
	     element = cfg_list_next(element))
	{
		tresult = check_orderent(cfg_listelt_value(element), logctx);
		if (tresult != ISC_R_SUCCESS)
			result = tresult;
	}
	return (result);
}

static isc_result_t
check_dual_stack(const cfg_obj_t *options, isc_log_t *logctx) {
	const cfg_listelt_t *element;
	const cfg_obj_t *alternates = NULL;
	const cfg_obj_t *value;
	const cfg_obj_t *obj;
	const char *str;
	dns_fixedname_t fixed;
	dns_name_t *name;
	isc_buffer_t buffer;
	isc_result_t result = ISC_R_SUCCESS;
	isc_result_t tresult;

	(void)cfg_map_get(options, "dual-stack-servers", &alternates);

	if (alternates == NULL)
		return (ISC_R_SUCCESS);

	obj = cfg_tuple_get(alternates, "port");
	if (cfg_obj_isuint32(obj)) {
		isc_uint32_t val = cfg_obj_asuint32(obj);
		if (val > ISC_UINT16_MAX) {
			cfg_obj_log(obj, logctx, ISC_LOG_ERROR,
				    "port '%u' out of range", val);
			result = ISC_R_FAILURE;
		}
	}
	obj = cfg_tuple_get(alternates, "addresses");
	for (element = cfg_list_first(obj);
	     element != NULL;
	     element = cfg_list_next(element)) {
		value = cfg_listelt_value(element);
		if (cfg_obj_issockaddr(value))
			continue;
		obj = cfg_tuple_get(value, "name");
		str = cfg_obj_asstring(obj);
		isc_buffer_init(&buffer, str, strlen(str));
		isc_buffer_add(&buffer, strlen(str));
		dns_fixedname_init(&fixed);
		name = dns_fixedname_name(&fixed);
		tresult = dns_name_fromtext(name, &buffer, dns_rootname,
					   ISC_FALSE, NULL);
		if (tresult != ISC_R_SUCCESS) {
			cfg_obj_log(obj, logctx, ISC_LOG_ERROR,
				    "bad name '%s'", str);
			result = ISC_R_FAILURE;
		}
		obj = cfg_tuple_get(value, "port");
		if (cfg_obj_isuint32(obj)) {
			isc_uint32_t val = cfg_obj_asuint32(obj);
			if (val > ISC_UINT16_MAX) {
				cfg_obj_log(obj, logctx, ISC_LOG_ERROR,
					    "port '%u' out of range", val);
				result = ISC_R_FAILURE;
			}
		}
	}
	return (result);
}

static isc_result_t
check_forward(const cfg_obj_t *options,  const cfg_obj_t *global,
	      isc_log_t *logctx)
{
	const cfg_obj_t *forward = NULL;
	const cfg_obj_t *forwarders = NULL;

	(void)cfg_map_get(options, "forward", &forward);
	(void)cfg_map_get(options, "forwarders", &forwarders);

	if (forwarders != NULL && global != NULL) {
		const char *file = cfg_obj_file(global);
		unsigned int line = cfg_obj_line(global);
		cfg_obj_log(forwarders, logctx, ISC_LOG_ERROR,
			    "forwarders declared in root zone and "
			    "in general configuration: %s:%u",
			    file, line);
		return (ISC_R_FAILURE);
	}
	if (forward != NULL && forwarders == NULL) {
		cfg_obj_log(forward, logctx, ISC_LOG_ERROR,
			    "no matching 'forwarders' statement");
		return (ISC_R_FAILURE);
	}
	return (ISC_R_SUCCESS);
}

static isc_result_t
disabled_algorithms(const cfg_obj_t *disabled, isc_log_t *logctx) {
	isc_result_t result = ISC_R_SUCCESS;
	isc_result_t tresult;
	const cfg_listelt_t *element;
	const char *str;
	isc_buffer_t b;
	dns_fixedname_t fixed;
	dns_name_t *name;
	const cfg_obj_t *obj;

	dns_fixedname_init(&fixed);
	name = dns_fixedname_name(&fixed);
	obj = cfg_tuple_get(disabled, "name");
	str = cfg_obj_asstring(obj);
	isc_buffer_init(&b, str, strlen(str));
	isc_buffer_add(&b, strlen(str));
	tresult = dns_name_fromtext(name, &b, dns_rootname, ISC_FALSE, NULL);
	if (tresult != ISC_R_SUCCESS) {
		cfg_obj_log(obj, logctx, ISC_LOG_ERROR,
			    "bad domain name '%s'", str);
		result = tresult;
	}

	obj = cfg_tuple_get(disabled, "algorithms");

	for (element = cfg_list_first(obj);
	     element != NULL;
	     element = cfg_list_next(element))
	{
		isc_textregion_t r;
		dns_secalg_t alg;
		isc_result_t tresult;

		DE_CONST(cfg_obj_asstring(cfg_listelt_value(element)), r.base);
		r.length = strlen(r.base);

		tresult = dns_secalg_fromtext(&alg, &r);
		if (tresult != ISC_R_SUCCESS) {
			isc_uint8_t ui;
			result = isc_parse_uint8(&ui, r.base, 10);
		}
		if (tresult != ISC_R_SUCCESS) {
			cfg_obj_log(cfg_listelt_value(element), logctx,
				    ISC_LOG_ERROR, "invalid algorithm '%s'",
				    r.base);
			result = tresult;
		}
	}
	return (result);
}

static isc_result_t
nameexist(const cfg_obj_t *obj, const char *name, int value,
	  isc_symtab_t *symtab, const char *fmt, isc_log_t *logctx,
	  isc_mem_t *mctx)
{
	char *key;
	const char *file;
	unsigned int line;
	isc_result_t result;
	isc_symvalue_t symvalue;

	key = isc_mem_strdup(mctx, name);
	if (key == NULL)
		return (ISC_R_NOMEMORY);
	symvalue.as_cpointer = obj;
	result = isc_symtab_define(symtab, key, value, symvalue,
				   isc_symexists_reject);
	if (result == ISC_R_EXISTS) {
		RUNTIME_CHECK(isc_symtab_lookup(symtab, key, value,
						&symvalue) == ISC_R_SUCCESS);
		file = cfg_obj_file(symvalue.as_cpointer);
		line = cfg_obj_line(symvalue.as_cpointer);

		if (file == NULL)
			file = "<unknown file>";
		cfg_obj_log(obj, logctx, ISC_LOG_ERROR, fmt, key, file, line);
		isc_mem_free(mctx, key);
		result = ISC_R_EXISTS;
	} else if (result != ISC_R_SUCCESS) {
		isc_mem_free(mctx, key);
	}
	return (result);
}

static isc_result_t
mustbesecure(const cfg_obj_t *secure, isc_symtab_t *symtab, isc_log_t *logctx,
	     isc_mem_t *mctx)
{
	const cfg_obj_t *obj;
	char namebuf[DNS_NAME_FORMATSIZE];
	const char *str;
	dns_fixedname_t fixed;
	dns_name_t *name;
	isc_buffer_t b;
	isc_result_t result = ISC_R_SUCCESS;

	dns_fixedname_init(&fixed);
	name = dns_fixedname_name(&fixed);
	obj = cfg_tuple_get(secure, "name");
	str = cfg_obj_asstring(obj);
	isc_buffer_init(&b, str, strlen(str));
	isc_buffer_add(&b, strlen(str));
	result = dns_name_fromtext(name, &b, dns_rootname, ISC_FALSE, NULL);
	if (result != ISC_R_SUCCESS) {
		cfg_obj_log(obj, logctx, ISC_LOG_ERROR,
			    "bad domain name '%s'", str);
	} else {
		dns_name_format(name, namebuf, sizeof(namebuf));
		result = nameexist(secure, namebuf, 1, symtab,
				   "dnssec-must-be-secure '%s': already "
				   "exists previous definition: %s:%u",
				   logctx, mctx);
	}
	return (result);
}

static isc_result_t
checkacl(const char *aclname, cfg_aclconfctx_t *actx, const cfg_obj_t *zconfig,
	 const cfg_obj_t *voptions, const cfg_obj_t *config,
	 isc_log_t *logctx, isc_mem_t *mctx)
{
	isc_result_t result;
	const cfg_obj_t *aclobj = NULL;
	const cfg_obj_t *options;
	dns_acl_t *acl = NULL;

	if (zconfig != NULL) {
		options = cfg_tuple_get(zconfig, "options");
		cfg_map_get(options, aclname, &aclobj);
	}
	if (voptions != NULL && aclobj == NULL)
		cfg_map_get(voptions, aclname, &aclobj);
	if (config != NULL && aclobj == NULL) {
		options = NULL;
		cfg_map_get(config, "options", &options);
		if (options != NULL)
			cfg_map_get(options, aclname, &aclobj);
	}
	if (aclobj == NULL)
		return (ISC_R_SUCCESS);
	result = cfg_acl_fromconfig(aclobj, config, logctx,
				    actx, mctx, 0, &acl);
	if (acl != NULL)
		dns_acl_detach(&acl);
	return (result);
}

static isc_result_t
check_viewacls(cfg_aclconfctx_t *actx, const cfg_obj_t *voptions,
	       const cfg_obj_t *config, isc_log_t *logctx, isc_mem_t *mctx)
{
	isc_result_t result = ISC_R_SUCCESS, tresult;
	int i = 0;

	static const char *acls[] = { "allow-query", "allow-query-on",
		"allow-query-cache", "allow-query-cache-on",
		"blackhole", "match-clients", "match-destinations",
		"sortlist", NULL };

	while (acls[i] != NULL) {
		tresult = checkacl(acls[i++], actx, NULL, voptions, config,
				   logctx, mctx);
		if (tresult != ISC_R_SUCCESS)
			result = tresult;
	}
	return (result);
}

/*
 * Check allow-recursion and allow-recursion-on acls, and also log a
 * warning if they're inconsistent with the "recursion" option.
 */
static isc_result_t
check_recursionacls(cfg_aclconfctx_t *actx, const cfg_obj_t *voptions,
		    const char *viewname, const cfg_obj_t *config,
		    isc_log_t *logctx, isc_mem_t *mctx)
{
	const cfg_obj_t *options, *aclobj, *obj = NULL;
	dns_acl_t *acl = NULL;
	isc_result_t result = ISC_R_SUCCESS, tresult;
	isc_boolean_t recursion;
	const char *forview = " for view ";
	int i = 0;

	static const char *acls[] = { "allow-recursion", "allow-recursion-on",
				      NULL };

	if (voptions != NULL)
		cfg_map_get(voptions, "recursion", &obj);
	if (obj == NULL && config != NULL) {
		options = NULL;
		cfg_map_get(config, "options", &options);
		if (options != NULL)
			cfg_map_get(options, "recursion", &obj);
	}
	if (obj == NULL)
		recursion = ISC_TRUE;
	else
		recursion = cfg_obj_asboolean(obj);

	if (viewname == NULL) {
		viewname = "";
		forview = "";
	}

	for (i = 0; acls[i] != NULL; i++) {
		aclobj = options = NULL;
		acl = NULL;

		if (voptions != NULL)
			cfg_map_get(voptions, acls[i], &aclobj);
		if (config != NULL && aclobj == NULL) {
			options = NULL;
			cfg_map_get(config, "options", &options);
			if (options != NULL)
				cfg_map_get(options, acls[i], &aclobj);
		}
		if (aclobj == NULL)
			continue;

		tresult = cfg_acl_fromconfig(aclobj, config, logctx,
					    actx, mctx, 0, &acl);

		if (tresult != ISC_R_SUCCESS)
			result = tresult;

		if (acl == NULL)
			continue;

		if (recursion == ISC_FALSE && !dns_acl_isnone(acl)) {
			cfg_obj_log(aclobj, logctx, ISC_LOG_WARNING,
				    "both \"recursion no;\" and "
				    "\"%s\" active%s%s",
				    acls[i], forview, viewname);
		}

		if (acl != NULL)
			dns_acl_detach(&acl);
	}

	return (result);
}

typedef struct {
	const char *name;
	unsigned int scale;
	unsigned int max;
} intervaltable;

static isc_result_t
check_options(const cfg_obj_t *options, isc_log_t *logctx, isc_mem_t *mctx) {
	isc_result_t result = ISC_R_SUCCESS;
	isc_result_t tresult;
	unsigned int i;
	const cfg_obj_t *obj = NULL;
	const cfg_obj_t *resignobj = NULL;
	const cfg_listelt_t *element;
	isc_symtab_t *symtab = NULL;
	dns_fixedname_t fixed;
	const char *str;
	dns_name_t *name;
	isc_buffer_t b;

	static intervaltable intervals[] = {
	{ "cleaning-interval", 60, 28 * 24 * 60 },	/* 28 days */
	{ "heartbeat-interval", 60, 28 * 24 * 60 },	/* 28 days */
	{ "interface-interval", 60, 28 * 24 * 60 },	/* 28 days */
	{ "max-transfer-idle-in", 60, 28 * 24 * 60 },	/* 28 days */
	{ "max-transfer-idle-out", 60, 28 * 24 * 60 },	/* 28 days */
	{ "max-transfer-time-in", 60, 28 * 24 * 60 },	/* 28 days */
	{ "max-transfer-time-out", 60, 28 * 24 * 60 },	/* 28 days */
	{ "statistics-interval", 60, 28 * 24 * 60 },	/* 28 days */
	};

	/*
	 * Check that fields specified in units of time other than seconds
	 * have reasonable values.
	 */
	for (i = 0; i < sizeof(intervals) / sizeof(intervals[0]); i++) {
		isc_uint32_t val;
		obj = NULL;
		(void)cfg_map_get(options, intervals[i].name, &obj);
		if (obj == NULL)
			continue;
		val = cfg_obj_asuint32(obj);
		if (val > intervals[i].max) {
			cfg_obj_log(obj, logctx, ISC_LOG_ERROR,
				    "%s '%u' is out of range (0..%u)",
				    intervals[i].name, val,
				    intervals[i].max);
			result = ISC_R_RANGE;
		} else if (val > (ISC_UINT32_MAX / intervals[i].scale)) {
			cfg_obj_log(obj, logctx, ISC_LOG_ERROR,
				    "%s '%d' is out of range",
				    intervals[i].name, val);
			result = ISC_R_RANGE;
		}
	}

	obj = NULL;
	cfg_map_get(options, "sig-validity-interval", &obj);
	if (obj != NULL) {
		isc_uint32_t validity, resign = 0;

		validity = cfg_obj_asuint32(cfg_tuple_get(obj, "validity"));
		resignobj = cfg_tuple_get(obj, "re-sign");
		if (!cfg_obj_isvoid(resignobj))
			resign = cfg_obj_asuint32(resignobj);

		if (validity > 3660 || validity == 0) { /* 10 years */
			cfg_obj_log(obj, logctx, ISC_LOG_ERROR,
				    "%s '%u' is out of range (1..3660)",
				    "sig-validity-interval", validity);
			result = ISC_R_RANGE;
		}

		if (!cfg_obj_isvoid(resignobj)) {
			if (resign > 3660 || resign == 0) { /* 10 years */
				cfg_obj_log(obj, logctx, ISC_LOG_ERROR,
					    "%s '%u' is out of range (1..3660)",
					    "sig-validity-interval (re-sign)",
					    validity);
				result = ISC_R_RANGE;
			} else if ((validity > 7 && validity < resign) ||
				   (validity <= 7 && validity * 24 < resign)) {
				cfg_obj_log(obj, logctx, ISC_LOG_ERROR,
					    "validity interval (%u days) "
					    "less than re-signing interval "
					    "(%u %s)", validity, resign,
					    (validity > 7) ? "days" : "hours");
				result = ISC_R_RANGE;
			}
		}
	}

	obj = NULL;
	(void)cfg_map_get(options, "preferred-glue", &obj);
	if (obj != NULL) {
		const char *str;
		str = cfg_obj_asstring(obj);
		if (strcasecmp(str, "a") != 0 &&
		    strcasecmp(str, "aaaa") != 0 &&
		    strcasecmp(str, "none") != 0)
			cfg_obj_log(obj, logctx, ISC_LOG_ERROR,
				    "preferred-glue unexpected value '%s'",
				    str);
	}

	obj = NULL;
	(void)cfg_map_get(options, "root-delegation-only", &obj);
	if (obj != NULL) {
		if (!cfg_obj_isvoid(obj)) {
			const cfg_listelt_t *element;
			const cfg_obj_t *exclude;
			const char *str;
			dns_fixedname_t fixed;
			dns_name_t *name;
			isc_buffer_t b;

			dns_fixedname_init(&fixed);
			name = dns_fixedname_name(&fixed);
			for (element = cfg_list_first(obj);
			     element != NULL;
			     element = cfg_list_next(element)) {
				exclude = cfg_listelt_value(element);
				str = cfg_obj_asstring(exclude);
				isc_buffer_init(&b, str, strlen(str));
				isc_buffer_add(&b, strlen(str));
				tresult = dns_name_fromtext(name, &b,
							   dns_rootname,
							   ISC_FALSE, NULL);
				if (tresult != ISC_R_SUCCESS) {
					cfg_obj_log(obj, logctx, ISC_LOG_ERROR,
						    "bad domain name '%s'",
						    str);
					result = tresult;
				}
			}
		}
	}

	/*
	 * Set supported DNSSEC algorithms.
	 */
	obj = NULL;
	(void)cfg_map_get(options, "disable-algorithms", &obj);
	if (obj != NULL) {
		for (element = cfg_list_first(obj);
		     element != NULL;
		     element = cfg_list_next(element))
		{
			obj = cfg_listelt_value(element);
			tresult = disabled_algorithms(obj, logctx);
			if (tresult != ISC_R_SUCCESS)
				result = tresult;
		}
	}

	dns_fixedname_init(&fixed);
	name = dns_fixedname_name(&fixed);

	/*
	 * Check the DLV zone name.
	 */
	obj = NULL;
	(void)cfg_map_get(options, "dnssec-lookaside", &obj);
	if (obj != NULL) {
		tresult = isc_symtab_create(mctx, 100, freekey, mctx,
					    ISC_FALSE, &symtab);
		if (tresult != ISC_R_SUCCESS)
			result = tresult;
		for (element = cfg_list_first(obj);
		     element != NULL;
		     element = cfg_list_next(element))
		{
			const char *dlv;

			obj = cfg_listelt_value(element);

			dlv = cfg_obj_asstring(cfg_tuple_get(obj, "domain"));
			isc_buffer_init(&b, dlv, strlen(dlv));
			isc_buffer_add(&b, strlen(dlv));
			tresult = dns_name_fromtext(name, &b, dns_rootname,
						    ISC_TRUE, NULL);
			if (tresult != ISC_R_SUCCESS) {
				cfg_obj_log(obj, logctx, ISC_LOG_ERROR,
					    "bad domain name '%s'", dlv);
				result = tresult;
				continue;
			}
			if (symtab != NULL) {
				tresult = nameexist(obj, dlv, 1, symtab,
						    "dnssec-lookaside '%s': "
						    "already exists previous "
						    "definition: %s:%u",
						    logctx, mctx);
				if (tresult != ISC_R_SUCCESS &&
				    result == ISC_R_SUCCESS)
					result = tresult;
			}
			/*
			 * XXXMPA to be removed when multiple lookaside
			 * namespaces are supported.
			 */
			if (!dns_name_equal(dns_rootname, name)) {
				cfg_obj_log(obj, logctx, ISC_LOG_ERROR,
					    "dnssec-lookaside '%s': "
					    "non-root not yet supported", dlv);
				if (result == ISC_R_SUCCESS)
					result = ISC_R_FAILURE;
			}
			dlv = cfg_obj_asstring(cfg_tuple_get(obj,
					       "trust-anchor"));
			isc_buffer_init(&b, dlv, strlen(dlv));
			isc_buffer_add(&b, strlen(dlv));
			tresult = dns_name_fromtext(name, &b, dns_rootname,
						    ISC_TRUE, NULL);
			if (tresult != ISC_R_SUCCESS) {
				cfg_obj_log(obj, logctx, ISC_LOG_ERROR,
					    "bad domain name '%s'", dlv);
				if (result == ISC_R_SUCCESS)
					result = tresult;
			}
		}
		if (symtab != NULL)
			isc_symtab_destroy(&symtab);
	}

	/*
	 * Check dnssec-must-be-secure.
	 */
	obj = NULL;
	(void)cfg_map_get(options, "dnssec-must-be-secure", &obj);
	if (obj != NULL) {
		isc_symtab_t *symtab = NULL;
		tresult = isc_symtab_create(mctx, 100, freekey, mctx,
					    ISC_FALSE, &symtab);
		if (tresult != ISC_R_SUCCESS)
			result = tresult;
		for (element = cfg_list_first(obj);
		     element != NULL;
		     element = cfg_list_next(element))
		{
			obj = cfg_listelt_value(element);
			tresult = mustbesecure(obj, symtab, logctx, mctx);
			if (tresult != ISC_R_SUCCESS)
				result = tresult;
		}
		if (symtab != NULL)
			isc_symtab_destroy(&symtab);
	}

	/*
	 * Check empty zone configuration.
	 */
	obj = NULL;
	(void)cfg_map_get(options, "empty-server", &obj);
	if (obj != NULL) {
		str = cfg_obj_asstring(obj);
		isc_buffer_init(&b, str, strlen(str));
		isc_buffer_add(&b, strlen(str));
		tresult = dns_name_fromtext(dns_fixedname_name(&fixed), &b,
					    dns_rootname, ISC_FALSE, NULL);
		if (tresult != ISC_R_SUCCESS) {
			cfg_obj_log(obj, logctx, ISC_LOG_ERROR,
				    "empty-server: invalid name '%s'", str);
			result = ISC_R_FAILURE;
		}
	}

	obj = NULL;
	(void)cfg_map_get(options, "empty-contact", &obj);
	if (obj != NULL) {
		str = cfg_obj_asstring(obj);
		isc_buffer_init(&b, str, strlen(str));
		isc_buffer_add(&b, strlen(str));
		tresult = dns_name_fromtext(dns_fixedname_name(&fixed), &b,
					    dns_rootname, ISC_FALSE, NULL);
		if (tresult != ISC_R_SUCCESS) {
			cfg_obj_log(obj, logctx, ISC_LOG_ERROR,
				    "empty-contact: invalid name '%s'", str);
			result = ISC_R_FAILURE;
		}
	}

	obj = NULL;
	(void)cfg_map_get(options, "disable-empty-zone", &obj);
	for (element = cfg_list_first(obj);
	     element != NULL;
	     element = cfg_list_next(element))
	{
		obj = cfg_listelt_value(element);
		str = cfg_obj_asstring(obj);
		isc_buffer_init(&b, str, strlen(str));
		isc_buffer_add(&b, strlen(str));
		tresult = dns_name_fromtext(dns_fixedname_name(&fixed), &b,
					    dns_rootname, ISC_FALSE, NULL);
		if (tresult != ISC_R_SUCCESS) {
			cfg_obj_log(obj, logctx, ISC_LOG_ERROR,
				    "disable-empty-zone: invalid name '%s'",
				    str);
			result = ISC_R_FAILURE;
		}
	}

	/*
	 * Check that server-id is not too long.
	 * 1024 bytes should be big enough.
	 */
	obj = NULL;
	(void)cfg_map_get(options, "server-id", &obj);
	if (obj != NULL && cfg_obj_isstring(obj) &&
	    strlen(cfg_obj_asstring(obj)) > 1024U) {
		cfg_obj_log(obj, logctx, ISC_LOG_ERROR,
			    "'server-id' too big (>1024 bytes)");
		result = ISC_R_FAILURE;
	}

	return (result);
}

static isc_result_t
get_masters_def(const cfg_obj_t *cctx, const char *name, const cfg_obj_t **ret) {
	isc_result_t result;
	const cfg_obj_t *masters = NULL;
	const cfg_listelt_t *elt;

	result = cfg_map_get(cctx, "masters", &masters);
	if (result != ISC_R_SUCCESS)
		return (result);
	for (elt = cfg_list_first(masters);
	     elt != NULL;
	     elt = cfg_list_next(elt)) {
		const cfg_obj_t *list;
		const char *listname;

		list = cfg_listelt_value(elt);
		listname = cfg_obj_asstring(cfg_tuple_get(list, "name"));

		if (strcasecmp(listname, name) == 0) {
			*ret = list;
			return (ISC_R_SUCCESS);
		}
	}
	return (ISC_R_NOTFOUND);
}

static isc_result_t
validate_masters(const cfg_obj_t *obj, const cfg_obj_t *config,
		 isc_uint32_t *countp, isc_log_t *logctx, isc_mem_t *mctx)
{
	isc_result_t result = ISC_R_SUCCESS;
	isc_result_t tresult;
	isc_uint32_t count = 0;
	isc_symtab_t *symtab = NULL;
	isc_symvalue_t symvalue;
	const cfg_listelt_t *element;
	const cfg_listelt_t **stack = NULL;
	isc_uint32_t stackcount = 0, pushed = 0;
	const cfg_obj_t *list;

	REQUIRE(countp != NULL);
	result = isc_symtab_create(mctx, 100, NULL, NULL, ISC_FALSE, &symtab);
	if (result != ISC_R_SUCCESS) {
		*countp = count;
		return (result);
	}

 newlist:
	list = cfg_tuple_get(obj, "addresses");
	element = cfg_list_first(list);
 resume:
	for ( ;
	     element != NULL;
	     element = cfg_list_next(element))
	{
		const char *listname;
		const cfg_obj_t *addr;
		const cfg_obj_t *key;

		addr = cfg_tuple_get(cfg_listelt_value(element),
				     "masterselement");
		key = cfg_tuple_get(cfg_listelt_value(element), "key");

		if (cfg_obj_issockaddr(addr)) {
			count++;
			continue;
		}
		if (!cfg_obj_isvoid(key)) {
			cfg_obj_log(key, logctx, ISC_LOG_ERROR,
				    "unexpected token '%s'",
				    cfg_obj_asstring(key));
			if (result == ISC_R_SUCCESS)
				result = ISC_R_FAILURE;
		}
		listname = cfg_obj_asstring(addr);
		symvalue.as_cpointer = addr;
		tresult = isc_symtab_define(symtab, listname, 1, symvalue,
					    isc_symexists_reject);
		if (tresult == ISC_R_EXISTS)
			continue;
		tresult = get_masters_def(config, listname, &obj);
		if (tresult != ISC_R_SUCCESS) {
			if (result == ISC_R_SUCCESS)
				result = tresult;
			cfg_obj_log(addr, logctx, ISC_LOG_ERROR,
				    "unable to find masters list '%s'",
				    listname);
			continue;
		}
		/* Grow stack? */
		if (stackcount == pushed) {
			void * new;
			isc_uint32_t newlen = stackcount + 16;
			size_t newsize, oldsize;

			newsize = newlen * sizeof(*stack);
			oldsize = stackcount * sizeof(*stack);
			new = isc_mem_get(mctx, newsize);
			if (new == NULL)
				goto cleanup;
			if (stackcount != 0) {
				void *ptr;

				DE_CONST(stack, ptr);
				memcpy(new, stack, oldsize);
				isc_mem_put(mctx, ptr, oldsize);
			}
			stack = new;
			stackcount = newlen;
		}
		stack[pushed++] = cfg_list_next(element);
		goto newlist;
	}
	if (pushed != 0) {
		element = stack[--pushed];
		goto resume;
	}
 cleanup:
	if (stack != NULL) {
		void *ptr;

		DE_CONST(stack, ptr);
		isc_mem_put(mctx, ptr, stackcount * sizeof(*stack));
	}
	isc_symtab_destroy(&symtab);
	*countp = count;
	return (result);
}

static isc_result_t
check_update_policy(const cfg_obj_t *policy, isc_log_t *logctx) {
	isc_result_t result = ISC_R_SUCCESS;
	isc_result_t tresult;
	const cfg_listelt_t *element;
	const cfg_listelt_t *element2;
	dns_fixedname_t fixed;
	const char *str;
	isc_buffer_t b;

	for (element = cfg_list_first(policy);
	     element != NULL;
	     element = cfg_list_next(element))
	{
		const cfg_obj_t *stmt = cfg_listelt_value(element);
		const cfg_obj_t *identity = cfg_tuple_get(stmt, "identity");
		const cfg_obj_t *matchtype = cfg_tuple_get(stmt, "matchtype");
		const cfg_obj_t *dname = cfg_tuple_get(stmt, "name");
		const cfg_obj_t *typelist = cfg_tuple_get(stmt, "types");

		dns_fixedname_init(&fixed);
		str = cfg_obj_asstring(identity);
		isc_buffer_init(&b, str, strlen(str));
		isc_buffer_add(&b, strlen(str));
		tresult = dns_name_fromtext(dns_fixedname_name(&fixed), &b,
					    dns_rootname, ISC_FALSE, NULL);
		if (tresult != ISC_R_SUCCESS) {
			cfg_obj_log(identity, logctx, ISC_LOG_ERROR,
				    "'%s' is not a valid name", str);
			result = tresult;
		}

		dns_fixedname_init(&fixed);
		str = cfg_obj_asstring(dname);
		isc_buffer_init(&b, str, strlen(str));
		isc_buffer_add(&b, strlen(str));
		tresult = dns_name_fromtext(dns_fixedname_name(&fixed), &b,
					    dns_rootname, ISC_FALSE, NULL);
		if (tresult != ISC_R_SUCCESS) {
			cfg_obj_log(dname, logctx, ISC_LOG_ERROR,
				    "'%s' is not a valid name", str);
			result = tresult;
		}
		if (tresult == ISC_R_SUCCESS &&
		    strcasecmp(cfg_obj_asstring(matchtype), "wildcard") == 0 &&
		    !dns_name_iswildcard(dns_fixedname_name(&fixed))) {
			cfg_obj_log(identity, logctx, ISC_LOG_ERROR,
				    "'%s' is not a wildcard", str);
			result = ISC_R_FAILURE;
		}

		for (element2 = cfg_list_first(typelist);
		     element2 != NULL;
		     element2 = cfg_list_next(element2))
		{
			const cfg_obj_t *typeobj;
			isc_textregion_t r;
			dns_rdatatype_t type;

			typeobj = cfg_listelt_value(element2);
			DE_CONST(cfg_obj_asstring(typeobj), r.base);
			r.length = strlen(r.base);

			tresult = dns_rdatatype_fromtext(&type, &r);
			if (tresult != ISC_R_SUCCESS) {
				cfg_obj_log(typeobj, logctx, ISC_LOG_ERROR,
					    "'%s' is not a valid type", r.base);
				result = tresult;
			}
		}
	}
	return (result);
}

#define MASTERZONE	1
#define SLAVEZONE	2
#define STUBZONE	4
#define HINTZONE	8
#define FORWARDZONE	16
#define DELEGATIONZONE	32
#define CHECKACL	64

typedef struct {
	const char *name;
	int allowed;
} optionstable;

static isc_result_t
check_zoneconf(const cfg_obj_t *zconfig, const cfg_obj_t *voptions,
	       const cfg_obj_t *config, isc_symtab_t *symtab,
	       dns_rdataclass_t defclass, cfg_aclconfctx_t *actx,
	       isc_log_t *logctx, isc_mem_t *mctx)
{
	const char *zname;
	const char *typestr;
	unsigned int ztype;
	const cfg_obj_t *zoptions;
	const cfg_obj_t *obj = NULL;
	isc_result_t result = ISC_R_SUCCESS;
	isc_result_t tresult;
	unsigned int i;
	dns_rdataclass_t zclass;
	dns_fixedname_t fixedname;
	isc_buffer_t b;
	isc_boolean_t root = ISC_FALSE;

	static optionstable options[] = {
	{ "allow-query", MASTERZONE | SLAVEZONE | STUBZONE | CHECKACL },
	{ "allow-notify", SLAVEZONE | CHECKACL },
	{ "allow-transfer", MASTERZONE | SLAVEZONE | CHECKACL },
	{ "notify", MASTERZONE | SLAVEZONE },
	{ "also-notify", MASTERZONE | SLAVEZONE },
	{ "dialup", MASTERZONE | SLAVEZONE | STUBZONE },
	{ "delegation-only", HINTZONE | STUBZONE | DELEGATIONZONE },
	{ "forward", MASTERZONE | SLAVEZONE | STUBZONE | FORWARDZONE },
	{ "forwarders", MASTERZONE | SLAVEZONE | STUBZONE | FORWARDZONE },
	{ "maintain-ixfr-base", MASTERZONE | SLAVEZONE },
	{ "max-ixfr-log-size", MASTERZONE | SLAVEZONE },
	{ "notify-source", MASTERZONE | SLAVEZONE },
	{ "notify-source-v6", MASTERZONE | SLAVEZONE },
	{ "transfer-source", SLAVEZONE | STUBZONE },
	{ "transfer-source-v6", SLAVEZONE | STUBZONE },
	{ "max-transfer-time-in", SLAVEZONE | STUBZONE },
	{ "max-transfer-time-out", MASTERZONE | SLAVEZONE },
	{ "max-transfer-idle-in", SLAVEZONE | STUBZONE },
	{ "max-transfer-idle-out", MASTERZONE | SLAVEZONE },
	{ "max-retry-time", SLAVEZONE | STUBZONE },
	{ "min-retry-time", SLAVEZONE | STUBZONE },
	{ "max-refresh-time", SLAVEZONE | STUBZONE },
	{ "min-refresh-time", SLAVEZONE | STUBZONE },
	{ "sig-validity-interval", MASTERZONE },
	{ "sig-re-signing-interval", MASTERZONE },
	{ "sig-signing-nodes", MASTERZONE },
	{ "sig-signing-type", MASTERZONE },
	{ "sig-signing-signatures", MASTERZONE },
	{ "zone-statistics", MASTERZONE | SLAVEZONE | STUBZONE },
	{ "allow-update", MASTERZONE | CHECKACL },
	{ "allow-update-forwarding", SLAVEZONE | CHECKACL },
	{ "file", MASTERZONE | SLAVEZONE | STUBZONE | HINTZONE },
	{ "journal", MASTERZONE | SLAVEZONE },
	{ "ixfr-base", MASTERZONE | SLAVEZONE },
	{ "ixfr-tmp-file", MASTERZONE | SLAVEZONE },
	{ "masters", SLAVEZONE | STUBZONE },
	{ "pubkey", MASTERZONE | SLAVEZONE | STUBZONE },
	{ "update-policy", MASTERZONE },
	{ "database", MASTERZONE | SLAVEZONE | STUBZONE },
	{ "key-directory", MASTERZONE },
	{ "check-wildcard", MASTERZONE },
	{ "check-mx", MASTERZONE },
	{ "integrity-check", MASTERZONE },
	{ "check-mx-cname", MASTERZONE },
	{ "check-srv-cname", MASTERZONE },
	{ "masterfile-format", MASTERZONE | SLAVEZONE | STUBZONE | HINTZONE },
	{ "update-check-ksk", MASTERZONE },
	{ "try-tcp-refresh", SLAVEZONE },
	};

	static optionstable dialups[] = {
	{ "notify", MASTERZONE | SLAVEZONE },
	{ "notify-passive", SLAVEZONE },
	{ "refresh", SLAVEZONE | STUBZONE },
	{ "passive", SLAVEZONE | STUBZONE },
	};

	zname = cfg_obj_asstring(cfg_tuple_get(zconfig, "name"));

	zoptions = cfg_tuple_get(zconfig, "options");

	obj = NULL;
	(void)cfg_map_get(zoptions, "type", &obj);
	if (obj == NULL) {
		cfg_obj_log(zconfig, logctx, ISC_LOG_ERROR,
			    "zone '%s': type not present", zname);
		return (ISC_R_FAILURE);
	}

	typestr = cfg_obj_asstring(obj);
	if (strcasecmp(typestr, "master") == 0)
		ztype = MASTERZONE;
	else if (strcasecmp(typestr, "slave") == 0)
		ztype = SLAVEZONE;
	else if (strcasecmp(typestr, "stub") == 0)
		ztype = STUBZONE;
	else if (strcasecmp(typestr, "forward") == 0)
		ztype = FORWARDZONE;
	else if (strcasecmp(typestr, "hint") == 0)
		ztype = HINTZONE;
	else if (strcasecmp(typestr, "delegation-only") == 0)
		ztype = DELEGATIONZONE;
	else {
		cfg_obj_log(obj, logctx, ISC_LOG_ERROR,
			    "zone '%s': invalid type %s",
			    zname, typestr);
		return (ISC_R_FAILURE);
	}

	obj = cfg_tuple_get(zconfig, "class");
	if (cfg_obj_isstring(obj)) {
		isc_textregion_t r;

		DE_CONST(cfg_obj_asstring(obj), r.base);
		r.length = strlen(r.base);
		result = dns_rdataclass_fromtext(&zclass, &r);
		if (result != ISC_R_SUCCESS) {
			cfg_obj_log(obj, logctx, ISC_LOG_ERROR,
				    "zone '%s': invalid class %s",
				    zname, r.base);
			return (ISC_R_FAILURE);
		}
		if (zclass != defclass) {
			cfg_obj_log(obj, logctx, ISC_LOG_ERROR,
				    "zone '%s': class '%s' does not "
				    "match view/default class",
				    zname, r.base);
			return (ISC_R_FAILURE);
		}
	}

	/*
	 * Look for an already existing zone.
	 * We need to make this canonical as isc_symtab_define()
	 * deals with strings.
	 */
	dns_fixedname_init(&fixedname);
	isc_buffer_init(&b, zname, strlen(zname));
	isc_buffer_add(&b, strlen(zname));
	tresult = dns_name_fromtext(dns_fixedname_name(&fixedname), &b,
				    dns_rootname, ISC_TRUE, NULL);
	if (tresult != ISC_R_SUCCESS) {
		cfg_obj_log(zconfig, logctx, ISC_LOG_ERROR,
			    "zone '%s': is not a valid name", zname);
		result = ISC_R_FAILURE;
	} else {
		char namebuf[DNS_NAME_FORMATSIZE];

		dns_name_format(dns_fixedname_name(&fixedname),
				namebuf, sizeof(namebuf));
		tresult = nameexist(zconfig, namebuf, ztype == HINTZONE ? 1 : 2,
				    symtab, "zone '%s': already exists "
				    "previous definition: %s:%u", logctx, mctx);
		if (tresult != ISC_R_SUCCESS)
			result = tresult;
		if (dns_name_equal(dns_fixedname_name(&fixedname),
				   dns_rootname))
			root = ISC_TRUE;
	}

	/*
	 * Look for inappropriate options for the given zone type.
	 * Check that ACLs expand correctly.
	 */
	for (i = 0; i < sizeof(options) / sizeof(options[0]); i++) {
		obj = NULL;
		if ((options[i].allowed & ztype) == 0 &&
		    cfg_map_get(zoptions, options[i].name, &obj) ==
		    ISC_R_SUCCESS)
		{
			if (strcmp(options[i].name, "allow-update") != 0 ||
			    ztype != SLAVEZONE) {
				cfg_obj_log(obj, logctx, ISC_LOG_ERROR,
					    "option '%s' is not allowed "
					    "in '%s' zone '%s'",
					    options[i].name, typestr, zname);
					result = ISC_R_FAILURE;
			} else
				cfg_obj_log(obj, logctx, ISC_LOG_WARNING,
					    "option '%s' is not allowed "
					    "in '%s' zone '%s'",
					    options[i].name, typestr, zname);
		}
		obj = NULL;
		if ((options[i].allowed & ztype) != 0 &&
		    (options[i].allowed & CHECKACL) != 0) {

			tresult = checkacl(options[i].name, actx, zconfig,
					   voptions, config, logctx, mctx);
			if (tresult != ISC_R_SUCCESS)
				result = tresult;
		}

	}

	/*
	 * Slave & stub zones must have a "masters" field.
	 */
	if (ztype == SLAVEZONE || ztype == STUBZONE) {
		obj = NULL;
		if (cfg_map_get(zoptions, "masters", &obj) != ISC_R_SUCCESS) {
			cfg_obj_log(zoptions, logctx, ISC_LOG_ERROR,
				    "zone '%s': missing 'masters' entry",
				    zname);
			result = ISC_R_FAILURE;
		} else {
			isc_uint32_t count;
			tresult = validate_masters(obj, config, &count,
						   logctx, mctx);
			if (tresult != ISC_R_SUCCESS && result == ISC_R_SUCCESS)
				result = tresult;
			if (tresult == ISC_R_SUCCESS && count == 0) {
				cfg_obj_log(zoptions, logctx, ISC_LOG_ERROR,
					    "zone '%s': empty 'masters' entry",
					    zname);
				result = ISC_R_FAILURE;
			}
		}
	}

	/*
	 * Master zones can't have both "allow-update" and "update-policy".
	 */
	if (ztype == MASTERZONE) {
		isc_result_t res1, res2;
		obj = NULL;
		res1 = cfg_map_get(zoptions, "allow-update", &obj);
		obj = NULL;
		res2 = cfg_map_get(zoptions, "update-policy", &obj);
		if (res1 == ISC_R_SUCCESS && res2 == ISC_R_SUCCESS) {
			cfg_obj_log(obj, logctx, ISC_LOG_ERROR,
				    "zone '%s': 'allow-update' is ignored "
				    "when 'update-policy' is present",
				    zname);
			result = ISC_R_FAILURE;
		} else if (res2 == ISC_R_SUCCESS &&
			   check_update_policy(obj, logctx) != ISC_R_SUCCESS)
			result = ISC_R_FAILURE;
		obj = NULL;
		res1 = cfg_map_get(zoptions, "sig-signing-type", &obj);
		if (res1 == ISC_R_SUCCESS) {
			isc_uint32_t type = cfg_obj_asuint32(obj);
			if (type < 0xff00U || type > 0xffffU)
				cfg_obj_log(obj, logctx, ISC_LOG_ERROR,
					    "sig-signing-type: %u out of "
					    "range [%u..%u]", type,
					    0xff00U, 0xffffU);
			result = ISC_R_FAILURE;
		}
	}

	/*
	 * Check the excessively complicated "dialup" option.
	 */
	if (ztype == MASTERZONE || ztype == SLAVEZONE || ztype == STUBZONE) {
		const cfg_obj_t *dialup = NULL;
		(void)cfg_map_get(zoptions, "dialup", &dialup);
		if (dialup != NULL && cfg_obj_isstring(dialup)) {
			const char *str = cfg_obj_asstring(dialup);
			for (i = 0;
			     i < sizeof(dialups) / sizeof(dialups[0]);
			     i++)
			{
				if (strcasecmp(dialups[i].name, str) != 0)
					continue;
				if ((dialups[i].allowed & ztype) == 0) {
					cfg_obj_log(obj, logctx,
						    ISC_LOG_ERROR,
						    "dialup type '%s' is not "
						    "allowed in '%s' "
						    "zone '%s'",
						    str, typestr, zname);
					result = ISC_R_FAILURE;
				}
				break;
			}
			if (i == sizeof(dialups) / sizeof(dialups[0])) {
				cfg_obj_log(obj, logctx, ISC_LOG_ERROR,
					    "invalid dialup type '%s' in zone "
					    "'%s'", str, zname);
				result = ISC_R_FAILURE;
			}
		}
	}

	/*
	 * Check that forwarding is reasonable.
	 */
	obj = NULL;
	if (root) {
		if (voptions != NULL)
			(void)cfg_map_get(voptions, "forwarders", &obj);
		if (obj == NULL) {
			const cfg_obj_t *options = NULL;
			(void)cfg_map_get(config, "options", &options);
			if (options != NULL)
				(void)cfg_map_get(options, "forwarders", &obj);
		}
	}
	if (check_forward(zoptions, obj, logctx) != ISC_R_SUCCESS)
		result = ISC_R_FAILURE;

	/*
	 * Check various options.
	 */
	tresult = check_options(zoptions, logctx, mctx);
	if (tresult != ISC_R_SUCCESS)
		result = tresult;

	/*
	 * If the zone type is rbt/rbt64 then master/hint zones
	 * require file clauses.
	 */
	obj = NULL;
	tresult = cfg_map_get(zoptions, "database", &obj);
	if (tresult == ISC_R_NOTFOUND ||
	    (tresult == ISC_R_SUCCESS &&
	     (strcmp("rbt", cfg_obj_asstring(obj)) == 0 ||
	      strcmp("rbt64", cfg_obj_asstring(obj)) == 0))) {
		obj = NULL;
		tresult = cfg_map_get(zoptions, "file", &obj);
		if (tresult != ISC_R_SUCCESS &&
		    (ztype == MASTERZONE || ztype == HINTZONE)) {
			cfg_obj_log(zconfig, logctx, ISC_LOG_ERROR,
				    "zone '%s': missing 'file' entry",
				    zname);
			result = tresult;
		}
	}

	return (result);
}


typedef struct keyalgorithms {
	const char *name;
	isc_uint16_t size;
} algorithmtable;

isc_result_t
bind9_check_key(const cfg_obj_t *key, isc_log_t *logctx) {
	const cfg_obj_t *algobj = NULL;
	const cfg_obj_t *secretobj = NULL;
	const char *keyname = cfg_obj_asstring(cfg_map_getname(key));
	const char *algorithm;
	int i;
	size_t len = 0;
	static const algorithmtable algorithms[] = {
		{ "hmac-md5", 128 },
		{ "hmac-md5.sig-alg.reg.int", 0 },
		{ "hmac-md5.sig-alg.reg.int.", 0 },
		{ "hmac-sha1", 160 },
		{ "hmac-sha224", 224 },
		{ "hmac-sha256", 256 },
		{ "hmac-sha384", 384 },
		{ "hmac-sha512", 512 },
		{  NULL, 0 }
	};

	(void)cfg_map_get(key, "algorithm", &algobj);
	(void)cfg_map_get(key, "secret", &secretobj);
	if (secretobj == NULL || algobj == NULL) {
		cfg_obj_log(key, logctx, ISC_LOG_ERROR,
			    "key '%s' must have both 'secret' and "
			    "'algorithm' defined",
			    keyname);
		return (ISC_R_FAILURE);
	}

	algorithm = cfg_obj_asstring(algobj);
	for (i = 0; algorithms[i].name != NULL; i++) {
		len = strlen(algorithms[i].name);
		if (strncasecmp(algorithms[i].name, algorithm, len) == 0 &&
		    (algorithm[len] == '\0' ||
		     (algorithms[i].size != 0 && algorithm[len] == '-')))
			break;
	}
	if (algorithms[i].name == NULL) {
		cfg_obj_log(algobj, logctx, ISC_LOG_ERROR,
			    "unknown algorithm '%s'", algorithm);
		return (ISC_R_NOTFOUND);
	}
	if (algorithm[len] == '-') {
		isc_uint16_t digestbits;
		isc_result_t result;
		result = isc_parse_uint16(&digestbits, algorithm + len + 1, 10);
		if (result == ISC_R_SUCCESS || result == ISC_R_RANGE) {
			if (result == ISC_R_RANGE ||
			    digestbits > algorithms[i].size) {
				cfg_obj_log(algobj, logctx, ISC_LOG_ERROR,
					    "key '%s' digest-bits too large "
					    "[%u..%u]", keyname,
					    algorithms[i].size / 2,
					    algorithms[i].size);
				return (ISC_R_RANGE);
			}
			if ((digestbits % 8) != 0) {
				cfg_obj_log(algobj, logctx, ISC_LOG_ERROR,
					    "key '%s' digest-bits not multiple"
					    " of 8", keyname);
				return (ISC_R_RANGE);
			}
			/*
			 * Recommended minima for hmac algorithms.
			 */
			if ((digestbits < (algorithms[i].size / 2U) ||
			     (digestbits < 80U)))
				cfg_obj_log(algobj, logctx, ISC_LOG_WARNING,
					    "key '%s' digest-bits too small "
					    "[<%u]", keyname,
					    algorithms[i].size/2);
		} else {
			cfg_obj_log(algobj, logctx, ISC_LOG_ERROR,
				    "key '%s': unable to parse digest-bits",
				    keyname);
			return (result);
		}
	}
	return (ISC_R_SUCCESS);
}

/*
 * Check key list for duplicates key names and that the key names
 * are valid domain names as these keys are used for TSIG.
 *
 * Check the key contents for validity.
 */
static isc_result_t
check_keylist(const cfg_obj_t *keys, isc_symtab_t *symtab,
	      isc_mem_t *mctx, isc_log_t *logctx)
{
	char namebuf[DNS_NAME_FORMATSIZE];
	dns_fixedname_t fname;
	dns_name_t *name;
	isc_result_t result = ISC_R_SUCCESS;
	isc_result_t tresult;
	const cfg_listelt_t *element;

	dns_fixedname_init(&fname);
	name = dns_fixedname_name(&fname);
	for (element = cfg_list_first(keys);
	     element != NULL;
	     element = cfg_list_next(element))
	{
		const cfg_obj_t *key = cfg_listelt_value(element);
		const char *keyid = cfg_obj_asstring(cfg_map_getname(key));
		isc_symvalue_t symvalue;
		isc_buffer_t b;
		char *keyname;

		isc_buffer_init(&b, keyid, strlen(keyid));
		isc_buffer_add(&b, strlen(keyid));
		tresult = dns_name_fromtext(name, &b, dns_rootname,
					    ISC_FALSE, NULL);
		if (tresult != ISC_R_SUCCESS) {
			cfg_obj_log(key, logctx, ISC_LOG_ERROR,
				    "key '%s': bad key name", keyid);
			result = tresult;
			continue;
		}
		tresult = bind9_check_key(key, logctx);
		if (tresult != ISC_R_SUCCESS)
			return (tresult);

		dns_name_format(name, namebuf, sizeof(namebuf));
		keyname = isc_mem_strdup(mctx, namebuf);
		if (keyname == NULL)
			return (ISC_R_NOMEMORY);
		symvalue.as_cpointer = key;
		tresult = isc_symtab_define(symtab, keyname, 1, symvalue,
					    isc_symexists_reject);
		if (tresult == ISC_R_EXISTS) {
			const char *file;
			unsigned int line;

			RUNTIME_CHECK(isc_symtab_lookup(symtab, keyname,
					    1, &symvalue) == ISC_R_SUCCESS);
			file = cfg_obj_file(symvalue.as_cpointer);
			line = cfg_obj_line(symvalue.as_cpointer);

			if (file == NULL)
				file = "<unknown file>";
			cfg_obj_log(key, logctx, ISC_LOG_ERROR,
				    "key '%s': already exists "
				    "previous definition: %s:%u",
				    keyid, file, line);
			isc_mem_free(mctx, keyname);
			result = tresult;
		} else if (tresult != ISC_R_SUCCESS) {
			isc_mem_free(mctx, keyname);
			return (tresult);
		}
	}
	return (result);
}

static struct {
	const char *v4;
	const char *v6;
} sources[] = {
	{ "transfer-source", "transfer-source-v6" },
	{ "notify-source", "notify-source-v6" },
	{ "query-source", "query-source-v6" },
	{ NULL, NULL }
};

/*
 * RNDC keys are not normalised unlike TSIG keys.
 *
 * 	"foo." is different to "foo".
 */
static isc_boolean_t
rndckey_exists(const cfg_obj_t *keylist, const char *keyname) {
	const cfg_listelt_t *element;
	const cfg_obj_t *obj;
	const char *str;

	if (keylist == NULL)
		return (ISC_FALSE);

	for (element = cfg_list_first(keylist);
	     element != NULL;
	     element = cfg_list_next(element))
	{
		obj = cfg_listelt_value(element);
		str = cfg_obj_asstring(cfg_map_getname(obj));
		if (!strcasecmp(str, keyname))
			return (ISC_TRUE);
	}
	return (ISC_FALSE);
}

static isc_result_t
check_servers(const cfg_obj_t *config, const cfg_obj_t *voptions,
	      isc_symtab_t *symtab, isc_log_t *logctx)
{
	dns_fixedname_t fname;
	isc_result_t result = ISC_R_SUCCESS;
	isc_result_t tresult;
	const cfg_listelt_t *e1, *e2;
	const cfg_obj_t *v1, *v2, *keys;
	const cfg_obj_t *servers;
	isc_netaddr_t n1, n2;
	unsigned int p1, p2;
	const cfg_obj_t *obj;
	char buf[ISC_NETADDR_FORMATSIZE];
	char namebuf[DNS_NAME_FORMATSIZE];
	const char *xfr;
	const char *keyval;
	isc_buffer_t b;
	int source;
	dns_name_t *keyname;

	servers = NULL;
	if (voptions != NULL)
		(void)cfg_map_get(voptions, "server", &servers);
	if (servers == NULL)
		(void)cfg_map_get(config, "server", &servers);
	if (servers == NULL)
		return (ISC_R_SUCCESS);

	for (e1 = cfg_list_first(servers); e1 != NULL; e1 = cfg_list_next(e1)) {
		v1 = cfg_listelt_value(e1);
		cfg_obj_asnetprefix(cfg_map_getname(v1), &n1, &p1);
		/*
		 * Check that unused bits are zero.
		 */
		tresult = isc_netaddr_prefixok(&n1, p1);
		if (tresult != ISC_R_SUCCESS) {
			INSIST(tresult == ISC_R_FAILURE);
			isc_netaddr_format(&n1, buf, sizeof(buf));
			cfg_obj_log(v1, logctx, ISC_LOG_ERROR,
				    "server '%s/%u': invalid prefix "
				    "(extra bits specified)", buf, p1);
			result = tresult;
		}
		source = 0;
		do {
			obj = NULL;
			if (n1.family == AF_INET)
				xfr = sources[source].v6;
			else
				xfr = sources[source].v4;
			(void)cfg_map_get(v1, xfr, &obj);
			if (obj != NULL) {
				isc_netaddr_format(&n1, buf, sizeof(buf));
				cfg_obj_log(v1, logctx, ISC_LOG_ERROR,
					    "server '%s/%u': %s not legal",
					    buf, p1, xfr);
				result = ISC_R_FAILURE;
			}
		} while (sources[++source].v4 != NULL);
		e2 = e1;
		while ((e2 = cfg_list_next(e2)) != NULL) {
			v2 = cfg_listelt_value(e2);
			cfg_obj_asnetprefix(cfg_map_getname(v2), &n2, &p2);
			if (p1 == p2 && isc_netaddr_equal(&n1, &n2)) {
				const char *file = cfg_obj_file(v1);
				unsigned int line = cfg_obj_line(v1);

				if (file == NULL)
					file = "<unknown file>";

				isc_netaddr_format(&n2, buf, sizeof(buf));
				cfg_obj_log(v2, logctx, ISC_LOG_ERROR,
					    "server '%s/%u': already exists "
					    "previous definition: %s:%u",
					    buf, p2, file, line);
				result = ISC_R_FAILURE;
			}
		}
		keys = NULL;
		cfg_map_get(v1, "keys", &keys);
		if (keys != NULL) {
			/*
			 * Normalize key name.
			 */
			keyval = cfg_obj_asstring(keys);
			dns_fixedname_init(&fname);
			isc_buffer_init(&b, keyval, strlen(keyval));
			isc_buffer_add(&b, strlen(keyval));
			keyname = dns_fixedname_name(&fname);
			tresult = dns_name_fromtext(keyname, &b, dns_rootname,
						    ISC_FALSE, NULL);
			if (tresult != ISC_R_SUCCESS) {
				cfg_obj_log(keys, logctx, ISC_LOG_ERROR,
					    "bad key name '%s'", keyval);
				result = ISC_R_FAILURE;
				continue;
			}
			dns_name_format(keyname, namebuf, sizeof(namebuf));
			tresult = isc_symtab_lookup(symtab, namebuf, 1, NULL);
			if (tresult != ISC_R_SUCCESS) {
				cfg_obj_log(keys, logctx, ISC_LOG_ERROR,
					    "unknown key '%s'", keyval);
				result = ISC_R_FAILURE;
			}
		}
	}
	return (result);
}

static isc_result_t
check_viewconf(const cfg_obj_t *config, const cfg_obj_t *voptions,
	       const char *viewname, dns_rdataclass_t vclass,
	       isc_log_t *logctx, isc_mem_t *mctx)
{
	const cfg_obj_t *zones = NULL;
	const cfg_obj_t *keys = NULL;
	const cfg_listelt_t *element;
	isc_symtab_t *symtab = NULL;
	isc_result_t result = ISC_R_SUCCESS;
	isc_result_t tresult = ISC_R_SUCCESS;
	cfg_aclconfctx_t actx;
	const cfg_obj_t *obj;
	isc_boolean_t enablednssec, enablevalidation;

	/*
	 * Check that all zone statements are syntactically correct and
	 * there are no duplicate zones.
	 */
	tresult = isc_symtab_create(mctx, 100, freekey, mctx,
				    ISC_FALSE, &symtab);
	if (tresult != ISC_R_SUCCESS)
		return (ISC_R_NOMEMORY);

	cfg_aclconfctx_init(&actx);

	if (voptions != NULL)
		(void)cfg_map_get(voptions, "zone", &zones);
	else
		(void)cfg_map_get(config, "zone", &zones);

	for (element = cfg_list_first(zones);
	     element != NULL;
	     element = cfg_list_next(element))
	{
		isc_result_t tresult;
		const cfg_obj_t *zone = cfg_listelt_value(element);

		tresult = check_zoneconf(zone, voptions, config, symtab,
					 vclass, &actx, logctx, mctx);
		if (tresult != ISC_R_SUCCESS)
			result = ISC_R_FAILURE;
	}

	isc_symtab_destroy(&symtab);

	/*
	 * Check that forwarding is reasonable.
	 */
	if (voptions == NULL) {
		const cfg_obj_t *options = NULL;
		(void)cfg_map_get(config, "options", &options);
		if (options != NULL)
			if (check_forward(options, NULL,
					  logctx) != ISC_R_SUCCESS)
				result = ISC_R_FAILURE;
	} else {
		if (check_forward(voptions, NULL, logctx) != ISC_R_SUCCESS)
			result = ISC_R_FAILURE;
	}

	/*
	 * Check that dual-stack-servers is reasonable.
	 */
	if (voptions == NULL) {
		const cfg_obj_t *options = NULL;
		(void)cfg_map_get(config, "options", &options);
		if (options != NULL)
			if (check_dual_stack(options, logctx) != ISC_R_SUCCESS)
				result = ISC_R_FAILURE;
	} else {
		if (check_dual_stack(voptions, logctx) != ISC_R_SUCCESS)
			result = ISC_R_FAILURE;
	}

	/*
	 * Check that rrset-order is reasonable.
	 */
	if (voptions != NULL) {
		if (check_order(voptions, logctx) != ISC_R_SUCCESS)
			result = ISC_R_FAILURE;
	}

	/*
	 * Check that all key statements are syntactically correct and
	 * there are no duplicate keys.
	 */
	tresult = isc_symtab_create(mctx, 100, freekey, mctx,
				    ISC_FALSE, &symtab);
	if (tresult != ISC_R_SUCCESS)
		return (ISC_R_NOMEMORY);

	(void)cfg_map_get(config, "key", &keys);
	tresult = check_keylist(keys, symtab, mctx, logctx);
	if (tresult == ISC_R_EXISTS)
		result = ISC_R_FAILURE;
	else if (tresult != ISC_R_SUCCESS) {
		isc_symtab_destroy(&symtab);
		return (tresult);
	}

	if (voptions != NULL) {
		keys = NULL;
		(void)cfg_map_get(voptions, "key", &keys);
		tresult = check_keylist(keys, symtab, mctx, logctx);
		if (tresult == ISC_R_EXISTS)
			result = ISC_R_FAILURE;
		else if (tresult != ISC_R_SUCCESS) {
			isc_symtab_destroy(&symtab);
			return (tresult);
		}
	}

	/*
	 * Global servers can refer to keys in views.
	 */
	if (check_servers(config, voptions, symtab, logctx) != ISC_R_SUCCESS)
		result = ISC_R_FAILURE;

	isc_symtab_destroy(&symtab);

	/*
	 * Check that dnssec-enable/dnssec-validation are sensible.
	 */
	obj = NULL;
	if (voptions != NULL)
		(void)cfg_map_get(voptions, "dnssec-enable", &obj);
	if (obj == NULL)
		(void)cfg_map_get(config, "dnssec-enable", &obj);
	if (obj == NULL)
		enablednssec = ISC_TRUE;
	else
		enablednssec = cfg_obj_asboolean(obj);

	obj = NULL;
	if (voptions != NULL)
		(void)cfg_map_get(voptions, "dnssec-validation", &obj);
	if (obj == NULL)
		(void)cfg_map_get(config, "dnssec-validation", &obj);
	if (obj == NULL)
		enablevalidation = ISC_FALSE;	/* XXXMPA Change for 9.5. */
	else
		enablevalidation = cfg_obj_asboolean(obj);

	if (enablevalidation && !enablednssec)
		cfg_obj_log(obj, logctx, ISC_LOG_WARNING,
			    "'dnssec-validation yes;' and 'dnssec-enable no;'");

	if (voptions != NULL)
		tresult = check_options(voptions, logctx, mctx);
	else
		tresult = check_options(config, logctx, mctx);
	if (tresult != ISC_R_SUCCESS)
		result = tresult;

	tresult = check_viewacls(&actx, voptions, config, logctx, mctx);
	if (tresult != ISC_R_SUCCESS)
		result = tresult;

	tresult = check_recursionacls(&actx, voptions, viewname,
				      config, logctx, mctx);
	if (tresult != ISC_R_SUCCESS)
		result = tresult;

	cfg_aclconfctx_destroy(&actx);

	return (result);
}

static const char *
default_channels[] = {
	"default_syslog",
	"default_stderr",
	"default_debug",
	"null",
	NULL
};

static isc_result_t
bind9_check_logging(const cfg_obj_t *config, isc_log_t *logctx,
		    isc_mem_t *mctx)
{
	const cfg_obj_t *categories = NULL;
	const cfg_obj_t *category;
	const cfg_obj_t *channels = NULL;
	const cfg_obj_t *channel;
	const cfg_listelt_t *element;
	const cfg_listelt_t *delement;
	const char *channelname;
	const char *catname;
	const cfg_obj_t *fileobj = NULL;
	const cfg_obj_t *syslogobj = NULL;
	const cfg_obj_t *nullobj = NULL;
	const cfg_obj_t *stderrobj = NULL;
	const cfg_obj_t *logobj = NULL;
	isc_result_t result = ISC_R_SUCCESS;
	isc_result_t tresult;
	isc_symtab_t *symtab = NULL;
	isc_symvalue_t symvalue;
	int i;

	(void)cfg_map_get(config, "logging", &logobj);
	if (logobj == NULL)
		return (ISC_R_SUCCESS);

	result = isc_symtab_create(mctx, 100, NULL, NULL, ISC_FALSE, &symtab);
	if (result != ISC_R_SUCCESS)
		return (result);

	symvalue.as_cpointer = NULL;
	for (i = 0; default_channels[i] != NULL; i++) {
		tresult = isc_symtab_define(symtab, default_channels[i], 1,
					    symvalue, isc_symexists_replace);
		if (tresult != ISC_R_SUCCESS)
			result = tresult;
	}

	cfg_map_get(logobj, "channel", &channels);

	for (element = cfg_list_first(channels);
	     element != NULL;
	     element = cfg_list_next(element))
	{
		channel = cfg_listelt_value(element);
		channelname = cfg_obj_asstring(cfg_map_getname(channel));
		fileobj = syslogobj = nullobj = stderrobj = NULL;
		(void)cfg_map_get(channel, "file", &fileobj);
		(void)cfg_map_get(channel, "syslog", &syslogobj);
		(void)cfg_map_get(channel, "null", &nullobj);
		(void)cfg_map_get(channel, "stderr", &stderrobj);
		i = 0;
		if (fileobj != NULL)
			i++;
		if (syslogobj != NULL)
			i++;
		if (nullobj != NULL)
			i++;
		if (stderrobj != NULL)
			i++;
		if (i != 1) {
			cfg_obj_log(channel, logctx, ISC_LOG_ERROR,
				    "channel '%s': exactly one of file, syslog, "
				    "null, and stderr must be present",
				     channelname);
			result = ISC_R_FAILURE;
		}
		tresult = isc_symtab_define(symtab, channelname, 1,
					    symvalue, isc_symexists_replace);
		if (tresult != ISC_R_SUCCESS)
			result = tresult;
	}

	cfg_map_get(logobj, "category", &categories);

	for (element = cfg_list_first(categories);
	     element != NULL;
	     element = cfg_list_next(element))
	{
		category = cfg_listelt_value(element);
		catname = cfg_obj_asstring(cfg_tuple_get(category, "name"));
		if (isc_log_categorybyname(logctx, catname) == NULL) {
			cfg_obj_log(category, logctx, ISC_LOG_ERROR,
				    "undefined category: '%s'", catname);
			result = ISC_R_FAILURE;
		}
		channels = cfg_tuple_get(category, "destinations");
		for (delement = cfg_list_first(channels);
		     delement != NULL;
		     delement = cfg_list_next(delement))
		{
			channel = cfg_listelt_value(delement);
			channelname = cfg_obj_asstring(channel);
			tresult = isc_symtab_lookup(symtab, channelname, 1,
						    &symvalue);
			if (tresult != ISC_R_SUCCESS) {
				cfg_obj_log(channel, logctx, ISC_LOG_ERROR,
					    "undefined channel: '%s'",
					    channelname);
				result = tresult;
			}
		}
	}
	isc_symtab_destroy(&symtab);
	return (result);
}

static isc_result_t
bind9_check_controlskeys(const cfg_obj_t *control, const cfg_obj_t *keylist,
			 isc_log_t *logctx)
{
	isc_result_t result = ISC_R_SUCCESS;
	const cfg_obj_t *control_keylist;
	const cfg_listelt_t *element;
	const cfg_obj_t *key;
	const char *keyval;

	control_keylist = cfg_tuple_get(control, "keys");
	if (cfg_obj_isvoid(control_keylist))
		return (ISC_R_SUCCESS);

	for (element = cfg_list_first(control_keylist);
	     element != NULL;
	     element = cfg_list_next(element))
	{
		key = cfg_listelt_value(element);
		keyval = cfg_obj_asstring(key);

		if (!rndckey_exists(keylist, keyval)) {
			cfg_obj_log(key, logctx, ISC_LOG_ERROR,
				    "unknown key '%s'", keyval);
			result = ISC_R_NOTFOUND;
		}
	}
	return (result);
}

static isc_result_t
bind9_check_controls(const cfg_obj_t *config, isc_log_t *logctx,
		     isc_mem_t *mctx)
{
	isc_result_t result = ISC_R_SUCCESS, tresult;
	cfg_aclconfctx_t actx;
	const cfg_listelt_t *element, *element2;
	const cfg_obj_t *allow;
	const cfg_obj_t *control;
	const cfg_obj_t *controls;
	const cfg_obj_t *controlslist = NULL;
	const cfg_obj_t *inetcontrols;
	const cfg_obj_t *unixcontrols;
	const cfg_obj_t *keylist = NULL;
	const char *path;
	isc_uint32_t perm, mask;
	dns_acl_t *acl = NULL;
	isc_sockaddr_t addr;
	int i;

	(void)cfg_map_get(config, "controls", &controlslist);
	if (controlslist == NULL)
		return (ISC_R_SUCCESS);

	(void)cfg_map_get(config, "key", &keylist);

	cfg_aclconfctx_init(&actx);

	/*
	 * INET: Check allow clause.
	 * UNIX: Check "perm" for sanity, check path length.
	 */
	for (element = cfg_list_first(controlslist);
	     element != NULL;
	     element = cfg_list_next(element)) {
		controls = cfg_listelt_value(element);
		unixcontrols = NULL;
		inetcontrols = NULL;
		(void)cfg_map_get(controls, "unix", &unixcontrols);
		(void)cfg_map_get(controls, "inet", &inetcontrols);
		for (element2 = cfg_list_first(inetcontrols);
		     element2 != NULL;
		     element2 = cfg_list_next(element2)) {
			control = cfg_listelt_value(element2);
			allow = cfg_tuple_get(control, "allow");
			tresult = cfg_acl_fromconfig(allow, config, logctx,
						     &actx, mctx, 0, &acl);
			if (acl != NULL)
				dns_acl_detach(&acl);
			if (tresult != ISC_R_SUCCESS)
				result = tresult;
			tresult = bind9_check_controlskeys(control, keylist,
							   logctx);
			if (tresult != ISC_R_SUCCESS)
				result = tresult;
		}
		for (element2 = cfg_list_first(unixcontrols);
		     element2 != NULL;
		     element2 = cfg_list_next(element2)) {
			control = cfg_listelt_value(element2);
			path = cfg_obj_asstring(cfg_tuple_get(control, "path"));
			tresult = isc_sockaddr_frompath(&addr, path);
			if (tresult == ISC_R_NOSPACE) {
				cfg_obj_log(control, logctx, ISC_LOG_ERROR,
					    "unix control '%s': path too long",
					    path);
				result = ISC_R_NOSPACE;
			}
			perm = cfg_obj_asuint32(cfg_tuple_get(control, "perm"));
			for (i = 0; i < 3; i++) {
#ifdef NEED_SECURE_DIRECTORY
				mask = (0x1 << (i*3));	/* SEARCH */
#else
				mask = (0x6 << (i*3)); 	/* READ + WRITE */
#endif
				if ((perm & mask) == mask)
					break;
			}
			if (i == 0) {
				cfg_obj_log(control, logctx, ISC_LOG_WARNING,
					    "unix control '%s' allows access "
					    "to everyone", path);
			} else if (i == 3) {
				cfg_obj_log(control, logctx, ISC_LOG_WARNING,
					    "unix control '%s' allows access "
					    "to nobody", path);
			}
			tresult = bind9_check_controlskeys(control, keylist,
							   logctx);
			if (tresult != ISC_R_SUCCESS)
				result = tresult;
		}
	}
	cfg_aclconfctx_destroy(&actx);
	return (result);
}

isc_result_t
bind9_check_namedconf(const cfg_obj_t *config, isc_log_t *logctx,
		      isc_mem_t *mctx)
{
	const cfg_obj_t *options = NULL;
	const cfg_obj_t *views = NULL;
	const cfg_obj_t *acls = NULL;
	const cfg_obj_t *kals = NULL;
	const cfg_obj_t *obj;
	const cfg_listelt_t *velement;
	isc_result_t result = ISC_R_SUCCESS;
	isc_result_t tresult;
	isc_symtab_t *symtab = NULL;

	static const char *builtin[] = { "localhost", "localnets",
					 "any", "none"};

	(void)cfg_map_get(config, "options", &options);

	if (options != NULL &&
	    check_options(options, logctx, mctx) != ISC_R_SUCCESS)
		result = ISC_R_FAILURE;

	if (bind9_check_logging(config, logctx, mctx) != ISC_R_SUCCESS)
		result = ISC_R_FAILURE;

	if (bind9_check_controls(config, logctx, mctx) != ISC_R_SUCCESS)
		result = ISC_R_FAILURE;

	if (options != NULL &&
	    check_order(options, logctx) != ISC_R_SUCCESS)
		result = ISC_R_FAILURE;

	(void)cfg_map_get(config, "view", &views);

	if (views != NULL && options != NULL)
		if (check_dual_stack(options, logctx) != ISC_R_SUCCESS)
			result = ISC_R_FAILURE;

	if (views == NULL) {
		if (check_viewconf(config, NULL, NULL, dns_rdataclass_in,
				   logctx, mctx) != ISC_R_SUCCESS)
			result = ISC_R_FAILURE;
	} else {
		const cfg_obj_t *zones = NULL;

		(void)cfg_map_get(config, "zone", &zones);
		if (zones != NULL) {
			cfg_obj_log(zones, logctx, ISC_LOG_ERROR,
				    "when using 'view' statements, "
				    "all zones must be in views");
			result = ISC_R_FAILURE;
		}
	}

	tresult = isc_symtab_create(mctx, 100, NULL, NULL, ISC_TRUE, &symtab);
	if (tresult != ISC_R_SUCCESS)
		result = tresult;
	for (velement = cfg_list_first(views);
	     velement != NULL;
	     velement = cfg_list_next(velement))
	{
		const cfg_obj_t *view = cfg_listelt_value(velement);
		const cfg_obj_t *vname = cfg_tuple_get(view, "name");
		const cfg_obj_t *voptions = cfg_tuple_get(view, "options");
		const cfg_obj_t *vclassobj = cfg_tuple_get(view, "class");
		dns_rdataclass_t vclass = dns_rdataclass_in;
		isc_result_t tresult = ISC_R_SUCCESS;
		const char *key = cfg_obj_asstring(vname);
		isc_symvalue_t symvalue;

		if (cfg_obj_isstring(vclassobj)) {
			isc_textregion_t r;

			DE_CONST(cfg_obj_asstring(vclassobj), r.base);
			r.length = strlen(r.base);
			tresult = dns_rdataclass_fromtext(&vclass, &r);
			if (tresult != ISC_R_SUCCESS)
				cfg_obj_log(vclassobj, logctx, ISC_LOG_ERROR,
					    "view '%s': invalid class %s",
					    cfg_obj_asstring(vname), r.base);
		}
		if (tresult == ISC_R_SUCCESS && symtab != NULL) {
			symvalue.as_cpointer = view;
			tresult = isc_symtab_define(symtab, key, vclass,
						    symvalue,
						    isc_symexists_reject);
			if (tresult == ISC_R_EXISTS) {
				const char *file;
				unsigned int line;
				RUNTIME_CHECK(isc_symtab_lookup(symtab, key,
					   vclass, &symvalue) == ISC_R_SUCCESS);
				file = cfg_obj_file(symvalue.as_cpointer);
				line = cfg_obj_line(symvalue.as_cpointer);
				cfg_obj_log(view, logctx, ISC_LOG_ERROR,
					    "view '%s': already exists "
					    "previous definition: %s:%u",
					    key, file, line);
				result = tresult;
			} else if (tresult != ISC_R_SUCCESS) {
				result = tresult;
			} else if ((strcasecmp(key, "_bind") == 0 &&
				    vclass == dns_rdataclass_ch) ||
				   (strcasecmp(key, "_default") == 0 &&
				    vclass == dns_rdataclass_in)) {
				cfg_obj_log(view, logctx, ISC_LOG_ERROR,
					    "attempt to redefine builtin view "
					    "'%s'", key);
				result = ISC_R_EXISTS;
			}
		}
		if (tresult == ISC_R_SUCCESS)
			tresult = check_viewconf(config, voptions, key,
						 vclass, logctx, mctx);
		if (tresult != ISC_R_SUCCESS)
			result = ISC_R_FAILURE;
	}
	if (symtab != NULL)
		isc_symtab_destroy(&symtab);

	if (views != NULL && options != NULL) {
		obj = NULL;
		tresult = cfg_map_get(options, "cache-file", &obj);
		if (tresult == ISC_R_SUCCESS) {
			cfg_obj_log(obj, logctx, ISC_LOG_ERROR,
				    "'cache-file' cannot be a global "
				    "option if views are present");
			result = ISC_R_FAILURE;
		}
	}

	cfg_map_get(config, "acl", &acls);

	if (acls != NULL) {
		const cfg_listelt_t *elt;
		const cfg_listelt_t *elt2;
		const char *aclname;

		for (elt = cfg_list_first(acls);
		     elt != NULL;
		     elt = cfg_list_next(elt)) {
			const cfg_obj_t *acl = cfg_listelt_value(elt);
			unsigned int line = cfg_obj_line(acl);
			unsigned int i;

			aclname = cfg_obj_asstring(cfg_tuple_get(acl, "name"));
			for (i = 0;
			     i < sizeof(builtin) / sizeof(builtin[0]);
			     i++)
				if (strcasecmp(aclname, builtin[i]) == 0) {
					cfg_obj_log(acl, logctx, ISC_LOG_ERROR,
						    "attempt to redefine "
						    "builtin acl '%s'",
						    aclname);
					result = ISC_R_FAILURE;
					break;
				}

			for (elt2 = cfg_list_next(elt);
			     elt2 != NULL;
			     elt2 = cfg_list_next(elt2)) {
				const cfg_obj_t *acl2 = cfg_listelt_value(elt2);
				const char *name;
				name = cfg_obj_asstring(cfg_tuple_get(acl2,
								      "name"));
				if (strcasecmp(aclname, name) == 0) {
					const char *file = cfg_obj_file(acl);

					if (file == NULL)
						file = "<unknown file>";

					cfg_obj_log(acl2, logctx, ISC_LOG_ERROR,
						    "attempt to redefine "
						    "acl '%s' previous "
						    "definition: %s:%u",
						     name, file, line);
					result = ISC_R_FAILURE;
				}
			}
		}
	}

	tresult = cfg_map_get(config, "kal", &kals);
	if (tresult == ISC_R_SUCCESS) {
		const cfg_listelt_t *elt;
		const cfg_listelt_t *elt2;
		const char *aclname;

		for (elt = cfg_list_first(kals);
		     elt != NULL;
		     elt = cfg_list_next(elt)) {
			const cfg_obj_t *acl = cfg_listelt_value(elt);

			aclname = cfg_obj_asstring(cfg_tuple_get(acl, "name"));

			for (elt2 = cfg_list_next(elt);
			     elt2 != NULL;
			     elt2 = cfg_list_next(elt2)) {
				const cfg_obj_t *acl2 = cfg_listelt_value(elt2);
				const char *name;
				name = cfg_obj_asstring(cfg_tuple_get(acl2,
								      "name"));
				if (strcasecmp(aclname, name) == 0) {
					const char *file = cfg_obj_file(acl);
					unsigned int line = cfg_obj_line(acl);

					if (file == NULL)
						file = "<unknown file>";

					cfg_obj_log(acl2, logctx, ISC_LOG_ERROR,
						    "attempt to redefine "
						    "kal '%s' previous "
						    "definition: %s:%u",
						     name, file, line);
					result = ISC_R_FAILURE;
				}
			}
		}
	}

	return (result);
}
