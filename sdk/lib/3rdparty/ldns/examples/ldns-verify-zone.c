/*
 * read a zone file from disk and prints it, one RR per line
 *
 * (c) NLnetLabs 2008
 *
 * See the file LICENSE for the license
 *
 * Missing from the checks: empty non-terminals
 */

#include "config.h"
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>

#include <ldns/ldns.h>

#include <errno.h>

#ifdef HAVE_SSL
#include <openssl/err.h>

static int verbosity = 3;
static time_t check_time = 0;
static int32_t inception_offset = 0;
static int32_t expiration_offset = 0;
static bool do_sigchase = false;
static bool no_nomatch_msg = false;

static FILE* myout;
static FILE* myerr;

static void
update_error(ldns_status* result, ldns_status status)
{
	if (status != LDNS_STATUS_OK) {
		if (*result == LDNS_STATUS_OK || *result == LDNS_STATUS_ERR || 
		    (  *result == LDNS_STATUS_CRYPTO_NO_MATCHING_KEYTAG_DNSKEY 
		     && status != LDNS_STATUS_ERR 
		    )) {
			*result = status;
		}
	}
}

static void
print_type(FILE* stream, ldns_rr_type type)
{
	const ldns_rr_descriptor *descriptor  = ldns_rr_descript(type);

	if (descriptor && descriptor->_name) {
		fprintf(stream, "%s", descriptor->_name);
	} else {
		fprintf(stream, "TYPE%u", type);
	}
}

static ldns_status
read_key_file(const char *filename, ldns_rr_list *keys)
{
	ldns_status status = LDNS_STATUS_ERR;
	ldns_rr *rr;
	FILE *fp;
	uint32_t my_ttl = 0;
	ldns_rdf *my_origin = NULL;
	ldns_rdf *my_prev = NULL;
	int line_nr;

	if (!(fp = fopen(filename, "r"))) {
		return LDNS_STATUS_FILE_ERR;
	}
	while (!feof(fp)) {
		status = ldns_rr_new_frm_fp_l(&rr, fp, &my_ttl, &my_origin,
				&my_prev, &line_nr);

		if (status == LDNS_STATUS_OK) {

			if (   ldns_rr_get_type(rr) == LDNS_RR_TYPE_DS
			    || ldns_rr_get_type(rr) == LDNS_RR_TYPE_DNSKEY)

				ldns_rr_list_push_rr(keys, rr);

		} else if (   status == LDNS_STATUS_SYNTAX_EMPTY
		           || status == LDNS_STATUS_SYNTAX_TTL
		           || status == LDNS_STATUS_SYNTAX_ORIGIN
		           || status == LDNS_STATUS_SYNTAX_INCLUDE)

			status = LDNS_STATUS_OK;
		else
			break;
	}
	fclose(fp);
	return status;
}



static void
print_rr_error(FILE* stream, ldns_rr* rr, const char* msg)
{
	if (verbosity > 0) {
		fprintf(stream, "Error: %s for ", msg);
		ldns_rdf_print(stream, ldns_rr_owner(rr));
		fprintf(stream, "\t");
		print_type(stream, ldns_rr_get_type(rr));
		fprintf(stream, "\n");
	}
}

static void
print_rr_status_error(FILE* stream, ldns_rr* rr, ldns_status status)
{
	if (status != LDNS_STATUS_OK) {
		print_rr_error(stream, rr, ldns_get_errorstr_by_id(status));
		if (verbosity > 0 && status == LDNS_STATUS_SSL_ERR) {
#if OPENSSL_VERSION_NUMBER < 0x10100000 || defined(HAVE_LIBRESSL)
			ERR_load_crypto_strings();
#endif
			ERR_print_errors_fp(stream);
		}
	}
}

static void
print_rrs_status_error(FILE* stream, ldns_rr_list* rrs, ldns_status status,
		ldns_dnssec_rrs* cur_sig)
{
	if (status != LDNS_STATUS_OK) {
		if (ldns_rr_list_rr_count(rrs) > 0) {
			print_rr_status_error(stream, ldns_rr_list_rr(rrs, 0),
					status);
		} else if (verbosity > 0) {
			fprintf(stream, "Error: %s for <unknown>\n",
					ldns_get_errorstr_by_id(status));
		}
		if (verbosity >= 4) {
			fprintf(stream, "RRSet:\n");
			ldns_rr_list_print(stream, rrs);
			fprintf(stream, "Signature:\n");
			ldns_rr_print(stream, cur_sig->rr);
			fprintf(stream, "\n");
		}
	}
}

static ldns_status
rrsig_check_time_margins(ldns_rr* rrsig
#if 0 /* Passing those as arguments becomes sensible when 
       * rrsig_check_time_margins will be added to the library.
       */
	,time_t check_time, int32_t inception_offset, int32_t expiration_offset
#endif
	)
{
	int32_t inception, expiration;

	inception  = ldns_rdf2native_int32(ldns_rr_rrsig_inception (rrsig));
	expiration = ldns_rdf2native_int32(ldns_rr_rrsig_expiration(rrsig));

	if (((int32_t) (check_time - inception_offset))  - inception  < 0) {
		return LDNS_STATUS_CRYPTO_SIG_NOT_INCEPTED_WITHIN_MARGIN;
	}
	if (expiration - ((int32_t) (check_time + expiration_offset)) < 0) {
		return LDNS_STATUS_CRYPTO_SIG_EXPIRED_WITHIN_MARGIN;
	}
	return LDNS_STATUS_OK;
}

static ldns_status
verify_rrs(ldns_rr_list* rrset_rrs, ldns_dnssec_rrs* cur_sig,
		ldns_rr_list* keys)
{
	ldns_status status, result = LDNS_STATUS_OK;
	ldns_dnssec_rrs *cur_sig_bak = cur_sig;

	/* A single valid signature validates the RRset */
	while (cur_sig) {
		if (ldns_verify_rrsig_keylist_time( rrset_rrs, cur_sig->rr
		                                  , keys, check_time, NULL)
		||  rrsig_check_time_margins(cur_sig->rr))
			cur_sig = cur_sig->next;
		else
			return LDNS_STATUS_OK;
	}
	/* Without any valid signature, do print all errors.  */
	for (cur_sig = cur_sig_bak; cur_sig; cur_sig = cur_sig->next) {
		status = ldns_verify_rrsig_keylist_time(rrset_rrs,
		    cur_sig->rr, keys, check_time, NULL);
		status = status ? status 
		       : rrsig_check_time_margins(cur_sig->rr);
		if (!status)
			; /* pass */
		else if (!no_nomatch_msg || status !=
		    LDNS_STATUS_CRYPTO_NO_MATCHING_KEYTAG_DNSKEY)
			print_rrs_status_error(
			    myerr, rrset_rrs, status, cur_sig);
		update_error(&result, status);
	}
	return result;
}

static ldns_status
verify_dnssec_rrset(ldns_rdf *zone_name, ldns_rdf *name,
		ldns_dnssec_rrsets *rrset, ldns_rr_list *keys) 
{
	ldns_rr_list *rrset_rrs;
	ldns_dnssec_rrs *cur_rr, *cur_sig;
	ldns_status status;

	if (!rrset->rrs) return LDNS_STATUS_OK;

	rrset_rrs = ldns_rr_list_new();
	cur_rr = rrset->rrs;
	while(cur_rr && cur_rr->rr) {
		ldns_rr_list_push_rr(rrset_rrs, cur_rr->rr);
		cur_rr = cur_rr->next;
	}
	cur_sig = rrset->signatures;
	if (cur_sig) {
		status = verify_rrs(rrset_rrs, cur_sig, keys);

	} else /* delegations may be unsigned (on opt out...) */
	       if (rrset->type != LDNS_RR_TYPE_NS || 
			       ldns_dname_compare(name, zone_name) == 0) {
		
		print_rr_error(myerr, rrset->rrs->rr, "no signatures");
		status = LDNS_STATUS_CRYPTO_NO_RRSIG;
	} else {
		status = LDNS_STATUS_OK;
	}
	ldns_rr_list_free(rrset_rrs);

	return status;
}

static ldns_status
verify_single_rr(ldns_rr *rr, ldns_dnssec_rrs *signature_rrs,
		ldns_rr_list *keys)
{
	ldns_rr_list *rrset_rrs;
	ldns_status status;

	rrset_rrs = ldns_rr_list_new();
	ldns_rr_list_push_rr(rrset_rrs, rr);

	status = verify_rrs(rrset_rrs, signature_rrs, keys);

	ldns_rr_list_free(rrset_rrs);

	return status;
}

static ldns_status
verify_next_hashed_name(ldns_dnssec_zone* zone, ldns_dnssec_name *name)
{
	ldns_rbnode_t *next_node;
	ldns_dnssec_name *next_name;
	int cmp;
	char *next_owner_str;
	ldns_rdf *next_owner_dname;

	assert(name->hashed_name != NULL);

	next_node = ldns_rbtree_search(zone->hashed_names, name->hashed_name);
	assert(next_node != NULL);
	do {
		next_node = ldns_rbtree_next(next_node);
		if (next_node == LDNS_RBTREE_NULL) {
			next_node = ldns_rbtree_first(zone->hashed_names);
		}
		next_name = (ldns_dnssec_name *) next_node->data;
	} while (! next_name->nsec);

	next_owner_str = ldns_rdf2str(ldns_nsec3_next_owner(name->nsec));
	next_owner_dname = ldns_dname_new_frm_str(next_owner_str);
	cmp = ldns_dname_compare(next_owner_dname, next_name->hashed_name);
	ldns_rdf_deep_free(next_owner_dname);
	LDNS_FREE(next_owner_str);
	if (cmp != 0) {
		if (verbosity > 0) {
			fprintf(myerr, "Error: The NSEC3 record for ");
			ldns_rdf_print(stdout, name->name);
			fprintf(myerr, " points to the wrong next hashed owner"
					" name\n\tshould point to ");
			ldns_rdf_print(myerr, next_name->name);
			fprintf(myerr, ", whose hashed name is ");
			ldns_rdf_print(myerr, next_name->hashed_name);
			fprintf(myerr, "\n");
		}
		return LDNS_STATUS_ERR;
	} else {
		return LDNS_STATUS_OK;
	}
}

static bool zone_is_nsec3_optout(ldns_dnssec_zone* zone)
{
	static int remember = -1;
	
	if (remember == -1) {
		remember = ldns_dnssec_zone_is_nsec3_optout(zone) ? 1 : 0;
	}
	return remember == 1;
}

static ldns_status
verify_nsec(ldns_dnssec_zone* zone, ldns_rbnode_t *cur_node,
		ldns_rr_list *keys)
{
	ldns_rbnode_t *next_node;
	ldns_dnssec_name *name, *next_name;
	ldns_status status, result;
	result = LDNS_STATUS_OK;

	name = (ldns_dnssec_name *) cur_node->data;
	if (name->nsec) {
		if (name->nsec_signatures) {
			status = verify_single_rr(name->nsec,
					name->nsec_signatures, keys);

			update_error(&result, status);
		} else {
			if (verbosity > 0) {
				fprintf(myerr,
					"Error: the NSEC(3) record of ");
				ldns_rdf_print(myerr, name->name);
				fprintf(myerr, " has no signatures\n");
			}
			update_error(&result, LDNS_STATUS_ERR);
		}
		/* check whether the NSEC record points to the right name */
		switch (ldns_rr_get_type(name->nsec)) {
			case LDNS_RR_TYPE_NSEC:
				/* simply try next name */
				next_node = ldns_rbtree_next(cur_node);
				if (next_node == LDNS_RBTREE_NULL) {
					next_node = ldns_rbtree_first(
							zone->names);
				}
				next_node = ldns_dnssec_name_node_next_nonglue(
						next_node);
				if (!next_node) {
					next_node =
					    ldns_dnssec_name_node_next_nonglue(
						ldns_rbtree_first(zone->names));
				}
				next_name = (ldns_dnssec_name*)next_node->data;
				if (ldns_dname_compare(next_name->name,
							ldns_rr_rdf(name->nsec,
								0)) != 0) {
					if (verbosity > 0) {
						fprintf(myerr, "Error: the "
							"NSEC record for ");
						ldns_rdf_print(myerr,
								name->name);
						fprintf(myerr, " points to "
							"the wrong "
							"next owner name\n");
					}
					if (verbosity >= 4) {
						fprintf(myerr, "\t: ");
						ldns_rdf_print(myerr,
							ldns_rr_rdf(
								name->nsec,
								0));
						fprintf(myerr, " i.s.o. ");
						ldns_rdf_print(myerr,
							next_name->name);
						fprintf(myerr, ".\n");
					}
					update_error(&result,
							LDNS_STATUS_ERR);
				}
				break;
			case LDNS_RR_TYPE_NSEC3:
				/* find the hashed next name in the tree */
				/* this is expensive, do we need to add 
				 * support for this in the structs?
				 * (ie. pointer to next hashed name?)
				 */
				status = verify_next_hashed_name(zone, name);
				update_error(&result, status);
				break;
			default:
				break;
		}
	} else {
		if (zone_is_nsec3_optout(zone) &&
		    (ldns_dnssec_name_is_glue(name) ||
		     (    ldns_dnssec_rrsets_contains_type(name->rrsets,
							   LDNS_RR_TYPE_NS)
		      && !ldns_dnssec_rrsets_contains_type(name->rrsets,
							   LDNS_RR_TYPE_DS)))) {
			/* ok, no problem, but we need to remember to check
			 * whether the chain does not actually point to this
			 * name later */
		} else {
			if (verbosity > 0) {
				fprintf(myerr,
					"Error: there is no NSEC(3) for ");
				ldns_rdf_print(myerr, name->name);
				fprintf(myerr, "\n");
			}
			update_error(&result, LDNS_STATUS_ERR);
		}
	}
	return result;
}

static ldns_status
verify_dnssec_name(ldns_rdf *zone_name, ldns_dnssec_zone* zone,
		ldns_rbnode_t *cur_node, ldns_rr_list *keys,
		bool detached_zonemd)
{
	ldns_status result = LDNS_STATUS_OK;
	ldns_status status;
	ldns_dnssec_rrsets *cur_rrset;
	ldns_dnssec_name *name;
	int on_delegation_point;
	/* for NSEC chain checks */

	name = (ldns_dnssec_name *) cur_node->data;
	if (verbosity >= 5) {
		fprintf(myout, "Checking: ");
		ldns_rdf_print(myout, name->name);
		fprintf(myout, "\n");
	}

	if (ldns_dnssec_name_is_glue(name)) {
		/* glue */
		cur_rrset = name->rrsets;
		while (cur_rrset) {
			if (cur_rrset->signatures) {
				if (verbosity > 0) {
					fprintf(myerr, "Error: ");
					ldns_rdf_print(myerr, name->name);
					fprintf(myerr, "\t");
					print_type(myerr, cur_rrset->type);
					fprintf(myerr, " has signature(s),"
							" but is occluded"
							" (or glue)\n");
				}
				result = LDNS_STATUS_ERR;
			}
			cur_rrset = cur_rrset->next;
		}
		if (name->nsec) {
			if (verbosity > 0) {
				fprintf(myerr, "Error: ");
				ldns_rdf_print(myerr, name->name);
				fprintf(myerr, " has an NSEC(3),"
						" but is occluded"
						" (or glue)\n");
			}
			result = LDNS_STATUS_ERR;
		}
	} else {
		/* not glue, do real verify */

		on_delegation_point =
			    ldns_dnssec_rrsets_contains_type(name->rrsets,
					LDNS_RR_TYPE_NS)
			&& !ldns_dnssec_rrsets_contains_type(name->rrsets,
					LDNS_RR_TYPE_SOA);
		cur_rrset = name->rrsets;
		while(cur_rrset) {

			/* Do not check occluded rrsets
			 * on the delegation point
			 */
			if ((on_delegation_point && 
			     (cur_rrset->type == LDNS_RR_TYPE_NS ||
			      cur_rrset->type == LDNS_RR_TYPE_DS)) ||
			    (!on_delegation_point &&
			     cur_rrset->type != LDNS_RR_TYPE_RRSIG &&
			     cur_rrset->type != LDNS_RR_TYPE_NSEC &&

			     (   cur_rrset->type != LDNS_RR_TYPE_ZONEMD
			     || !detached_zonemd || cur_rrset->signatures))) {

				status = verify_dnssec_rrset(zone_name,
						name->name, cur_rrset, keys);
				update_error(&result, status);
			}
			cur_rrset = cur_rrset->next;
		}
		status = verify_nsec(zone, cur_node, keys);
		update_error(&result, status);
	}
	return result;
}

static void
add_keys_with_matching_ds(ldns_dnssec_rrsets* from_keys, ldns_rr_list *dss,
		ldns_rr_list *to_keys)
{
	size_t i;
	ldns_rr* ds_rr;
	ldns_dnssec_rrs *cur_key;

	for (i = 0; i < ldns_rr_list_rr_count(dss); i++) {

		if (ldns_rr_get_type(ds_rr = ldns_rr_list_rr(dss, i)) 
				== LDNS_RR_TYPE_DS) {

			for (cur_key = from_keys->rrs; cur_key;
					cur_key = cur_key->next ) {

				if (ldns_rr_compare_ds(cur_key->rr, ds_rr)) {
					ldns_rr_list_push_rr(to_keys,
							cur_key->rr);
					break;
				}
			}
		}
	}
}

static ldns_resolver *p_ldns_new_res(ldns_resolver** new_res, ldns_status *s)
{
	assert(new_res && s);
	if (!(*s = ldns_resolver_new_frm_file(new_res, NULL))) {
		ldns_resolver_set_dnssec(*new_res, 1);
		ldns_resolver_set_dnssec_cd(*new_res, 1);
		return *new_res;
	}
	ldns_resolver_free(*new_res);
	return (*new_res = NULL);
}

static ldns_status
sigchase(ldns_resolver* res, ldns_rdf *zone_name, ldns_dnssec_rrsets *zonekeys,
		ldns_rr_list *keys)
{
	ldns_dnssec_rrs* cur_key;
	ldns_status status;
	ldns_resolver* new_res = NULL;
	ldns_rdf* parent_name = NULL;
	ldns_rr_list* parent_keys = NULL;
	ldns_rr_list* ds_keys = NULL;

	add_keys_with_matching_ds(zonekeys, keys, keys);

	/* First try to authenticate the keys offline.
	 * When do_sigchase is given validation may continue lookup up
	 * keys online. Reporting the failure of the offline validation
	 * should then be suppressed.
	 */
	no_nomatch_msg = do_sigchase;
	status = verify_dnssec_rrset(zone_name, zone_name, zonekeys, keys);
	no_nomatch_msg = false;

	/* Continue online on validation failure when the -S option was given.
	 */
	if (  !do_sigchase
	    || status != LDNS_STATUS_CRYPTO_NO_MATCHING_KEYTAG_DNSKEY
	    || ldns_dname_label_count(zone_name) == 0 ) {
		if (verbosity > 0) {
			fprintf(myerr, "Cannot chase the root: %s\n"
			             , ldns_get_errorstr_by_id(status));
		}

	} else if (!res && !(res = p_ldns_new_res(&new_res, &status))) {
		if (verbosity > 0) {
			fprintf(myerr, "Could not create resolver: %s\n"
			             , ldns_get_errorstr_by_id(status));
		}
	} else if (!(parent_name = ldns_dname_left_chop(zone_name))) {
		status = LDNS_STATUS_MEM_ERR;

	/*
	 * Use the (authenticated) keys of the parent zone ...
	 */
	} else if (!(parent_keys = ldns_fetch_valid_domain_keys(res,
				parent_name, keys, &status))) {
		if (verbosity > 0) {
			fprintf(myerr,
				"Could not get valid DNSKEY RRset to "
				"validate domain's DS: %s\n",
				ldns_get_errorstr_by_id(status)
				);
		}
	/*
	 * ... to validate the DS for the zone ...
	 */
	} else if (!(ds_keys = ldns_validate_domain_ds(res, zone_name,
				parent_keys))) {
		status = LDNS_STATUS_CRYPTO_NO_TRUSTED_DS;
		if (verbosity > 0) {
			fprintf(myerr,
				"Could not get valid DS RRset for domain: %s\n",
				ldns_get_errorstr_by_id(status)
				);
		}
	} else {
		/*
		 * ... to use it to add the KSK to the trusted keys ...
		 */
		add_keys_with_matching_ds(zonekeys, ds_keys, keys);

		/*
		 * ... to validate all zonekeys ...
		 */
		status = verify_dnssec_rrset(zone_name, zone_name,
				zonekeys, keys);
	}
	/*
	 * ... so they can all be added to our list of trusted keys.
	 */
	ldns_resolver_deep_free(new_res);
	ldns_rdf_deep_free(parent_name);
	ldns_rr_list_free(parent_keys);
	ldns_rr_list_free(ds_keys);

	if (status == LDNS_STATUS_OK)
		for (cur_key = zonekeys->rrs; cur_key; cur_key = cur_key->next)
			ldns_rr_list_push_rr(keys, cur_key->rr);
	return status;
}

static ldns_status
verify_dnssec_zone(ldns_dnssec_zone *dnssec_zone, ldns_rdf *zone_name,
		ldns_rr_list *keys, bool apexonly, int percentage,
		bool detached_zonemd) 
{
	ldns_rbnode_t *cur_node;
	ldns_dnssec_rrsets *cur_key_rrset;
	ldns_dnssec_rrs *cur_key;
	ldns_status status;
	ldns_status result = LDNS_STATUS_OK;

	cur_key_rrset = ldns_dnssec_zone_find_rrset(dnssec_zone, zone_name,
			LDNS_RR_TYPE_DNSKEY);
	if (!cur_key_rrset || !cur_key_rrset->rrs) {
		if (verbosity > 0) {
			fprintf(myerr,
				"Error: No DNSKEY records at zone apex\n");
		}
		result = LDNS_STATUS_ERR;
	} else {
		/* are keys given with -k to use for validation? */
		if (ldns_rr_list_rr_count(keys) > 0) {
			if ((result = sigchase(NULL, zone_name, cur_key_rrset,
							keys)))
				goto error;
		} else
			for (cur_key = cur_key_rrset->rrs; cur_key;
					cur_key = cur_key->next) 
				ldns_rr_list_push_rr(keys, cur_key->rr);

		cur_node = ldns_rbtree_first(dnssec_zone->names);
		if (cur_node == LDNS_RBTREE_NULL) {
			if (verbosity > 0) {
				fprintf(myerr, "Error: Empty zone?\n");
			}
			result = LDNS_STATUS_ERR;
		}
		if (apexonly) {
			/*
			 * In this case, only the first node in the treewalk
			 * below should be checked.
			 */
			assert( cur_node->data == dnssec_zone->soa );
			/* 
			 * Although the percentage option doesn't make sense
			 * here, we set it to 100 to force the first node to 
			 * be checked.
			 */
			percentage = 100;
		}
		while (cur_node != LDNS_RBTREE_NULL) {
			/* should we check this one? saves calls to random. */
			if (percentage == 100 
			    || ((random() % 100) >= 100 - percentage)) {
				status = verify_dnssec_name(zone_name,
						dnssec_zone, cur_node, keys,
						detached_zonemd);
				update_error(&result, status);
				if (apexonly)
					break;
			}
			cur_node = ldns_rbtree_next(cur_node);
		}
	}
error:
	ldns_rr_list_free(keys);
	return result;
}

static void print_usage(FILE *out, const char *progname)
{
	fprintf(out, "Usage: %s [OPTIONS] <zonefile>\n", progname);
	fprintf(out, "\tReads the zonefile and checks for DNSSEC errors.\n");
	fprintf(out, "\nIt checks whether NSEC(3)s are present, "
	       "and verifies all signatures\n");
	fprintf(out, "It also checks the NSEC(3) chain, but it "
	       "will error on opted-out delegations\n");
	fprintf(out, "It also checks whether ZONEMDs are present, and if so, "
	       "needs one of them to match the zone's data.\n");
	fprintf(out, "\nOPTIONS:\n");
	fprintf(out, "\t-h\t\tshow this text\n");
	fprintf(out, "\t-a\t\tapex only, check only the zone apex\n");
	fprintf(out, "\t-e <period>\tsignatures may not expire "
	       "within this period.\n\t\t\t"
	       "(default no period is used)\n");
	fprintf(out, "\t-i <period>\tsignatures must have been "
	       "valid at least this long.\n\t\t\t"
	       "(default signatures should just be valid now)\n");
	fprintf(out, "\t-k <file>\tspecify a file that contains a "
	       "trusted DNSKEY or DS rr.\n\t\t\t"
	       "This option may be given more than once.\n"
	       "\t\t\tDefault is %s\n", LDNS_TRUST_ANCHOR_FILE);
	fprintf(out, "\t-p [0-100]\tonly checks this percentage of "
	       "the zone.\n\t\t\tDefaults to 100\n");
	fprintf(out, "\t-S\t\tchase signature(s) to a known key. "
	       "The network may be\n\t\t\taccessed to "
	       "validate the zone's DNSKEYs. (implies -k)\n");
	fprintf(out, "\t-t YYYYMMDDhhmmss | [+|-]offset\n\t\t\t"
	       "set the validation time either by an "
	       "absolute time\n\t\t\tvalue or as an "
	       "offset in seconds from <now>.\n\t\t\t"
	       "For data that came from the network (while "
	       "chasing),\n\t\t\tsystem time will be used "
	       "for validating it regardless.\n");
	fprintf(out, "\t-v\t\tshows the version and exits\n");
	fprintf(out, "\t-V [0-5]\tset verbosity level (default 3)\n");
	fprintf(out, "\t-Z\t\tRequires a valid ZONEMD RR to be present.\n");
	fprintf(out, "\t\t\tWhen given once, this option will permit verifying"
	       "\n\t\t\tjust the ZONEMD RR of an unsigned zone. When given "
	       "\n\t\t\tmore than once, the zone needs to be validly DNSSEC"
	       "\n\t\t\tsigned as well. With three times a -Z option (-ZZZ)"
	       "\n\t\t\ta ZONEMD RR without signatures is allowed.");
	fprintf(out, "\n<period>s are given in ISO 8601 duration format: "
	       "P[n]Y[n]M[n]DT[n]H[n]M[n]S\n");
	fprintf(out, "\nif no file is given standard input is read\n");
}

int
main(int argc, char **argv)
{
	char *filename;
	FILE *fp;
	int line_nr = 0;
	int c;
	ldns_status s;
	ldns_dnssec_zone *dnssec_zone = NULL;
	ldns_status result = LDNS_STATUS_ERR;
	bool apexonly = false;
	int percentage = 100;
	struct tm tm;
	ldns_duration_type *duration;
	ldns_rr_list *keys = ldns_rr_list_new();
	size_t nkeys = 0;
	const char *progname = argv[0];
	int zonemd_required = 0;
	ldns_dnssec_rrsets *zonemd_rrset;

	check_time = ldns_time(NULL);
	myout = stdout;
	myerr = stderr;

	while ((c = getopt(argc, argv, "ae:hi:k:vV:p:St:Z")) != -1) {
		switch(c) {
                case 'a':
                        apexonly = true;
                        break;
		case 'h':
			print_usage(stdout, progname);
			exit(EXIT_SUCCESS);
			break;
		case 'e':
		case 'i':
			duration = ldns_duration_create_from_string(optarg);
			if (!duration) {
				if (verbosity > 0) {
					fprintf(myerr,
						"<period> should be in ISO "
						"8601 duration format: "
						"P[n]Y[n]M[n]DT[n]H[n]M[n]S\n"
						);
				}
                                exit(EXIT_FAILURE);
			}
			if (c == 'e')
				expiration_offset =
					ldns_duration2time(duration);
			else
				inception_offset =
					ldns_duration2time(duration);
			break;
		case 'k':
			s = read_key_file(optarg, keys);
			if (s == LDNS_STATUS_FILE_ERR) {
				if (verbosity > 0) {
					fprintf(myerr,
						"Error opening %s: %s\n",
						optarg, strerror(errno));
				}
			}
			if (s != LDNS_STATUS_OK) {
				if (verbosity > 0) {
					fprintf(myerr,
						"Could not parse key file "
						"%s: %s\n",optarg,
						ldns_get_errorstr_by_id(s));
				}
                                exit(EXIT_FAILURE);
			}
			if (ldns_rr_list_rr_count(keys) == nkeys) {
				if (verbosity > 0) {
					fprintf(myerr,
						"No keys found in file %s\n",
						optarg);
				}
				exit(EXIT_FAILURE);
			}
			nkeys = ldns_rr_list_rr_count(keys);
			break;
                case 'p':
                        percentage = atoi(optarg);
                        if (percentage < 0 || percentage > 100) {
				if (verbosity > 0) {
	                        	fprintf(myerr,
						"percentage needs to fall "
						"between 0..100\n");
				}
                                exit(EXIT_FAILURE);
                        }
                        srandom(time(NULL) ^ getpid());
                        break;
		case 'S':
			do_sigchase = true;
			/* may chase */
			break;
		case 't':
			if (strlen(optarg) == 14 &&
			    sscanf(optarg, "%4d%2d%2d%2d%2d%2d",
				    &tm.tm_year, &tm.tm_mon,
				    &tm.tm_mday, &tm.tm_hour,
				    &tm.tm_min , &tm.tm_sec ) == 6) {

				tm.tm_year -= 1900;
				tm.tm_mon--;
				check_time = ldns_mktime_from_utc(&tm);
			}
			else  {
				check_time += atoi(optarg);
			}
			break;
		case 'v':
			printf("verify-zone version %s (ldns version %s)\n",
					LDNS_VERSION, ldns_version());
			exit(EXIT_SUCCESS);
			break;
		case 'V':
			verbosity = atoi(optarg);
			break;
		case 'Z':
			zonemd_required += 1;
			break;
		}
	}
	if (do_sigchase && nkeys == 0) {
		(void) read_key_file(LDNS_TRUST_ANCHOR_FILE, keys);
		nkeys = ldns_rr_list_rr_count(keys);

		if (nkeys == 0) {
			if (verbosity > 0) {
				fprintf(myerr, "Unable to chase "
						"signature without keys.\n");
			}
			exit(EXIT_FAILURE);
		}
	}

	argc -= optind;
	argv += optind;

	if (argc == 0) {
		fp = stdin;
	} else if (argc == 1) {
		filename = argv[0];

		fp = fopen(filename, "r");
		if (!fp) {
			if (verbosity > 0) {
				fprintf(myerr, "Unable to open %s: %s\n",
					filename, strerror(errno));
			}
			exit(EXIT_FAILURE);
		}
	} else {
		print_usage(stderr, progname);
		exit(EXIT_FAILURE);
	}

	s = ldns_dnssec_zone_new_frm_fp_l(&dnssec_zone, fp, NULL, 0,
			LDNS_RR_CLASS_IN, &line_nr);
	if (s != LDNS_STATUS_OK) {
		if (verbosity > 0) {
			fprintf(myerr, "%s at line %d\n",
				ldns_get_errorstr_by_id(s), line_nr);
		}
                exit(EXIT_FAILURE);
	}
	if (!dnssec_zone->soa) {
		if (verbosity > 0) {
			fprintf(myerr,
				"; Error: no SOA in the zone\n");
		}
		exit(EXIT_FAILURE);
	}

	result = ldns_dnssec_zone_mark_glue(dnssec_zone);
	if (result != LDNS_STATUS_OK) {
		if (verbosity > 0) {
			fprintf(myerr,
				"There were errors identifying the "
				"glue in the zone\n");
		}
	}
	if (verbosity >= 5) {
		ldns_dnssec_zone_print(myout, dnssec_zone);
	}
	zonemd_rrset = ldns_dnssec_zone_find_rrset(dnssec_zone,
				dnssec_zone->soa->name, LDNS_RR_TYPE_ZONEMD);

	if (zonemd_required == 1
	&&  !ldns_dnssec_zone_find_rrset(dnssec_zone,
			       	dnssec_zone->soa->name, LDNS_RR_TYPE_DNSKEY))
		result = LDNS_STATUS_OK;
	else
		result = verify_dnssec_zone(dnssec_zone,
				dnssec_zone->soa->name, keys, apexonly,
				percentage, zonemd_required > 2);

	if (zonemd_rrset) {
		ldns_status zonemd_result
		    = ldns_dnssec_zone_verify_zonemd(dnssec_zone);
		
		if (zonemd_result)
			fprintf( myerr, "Could not validate zone digest: %s\n"
			       , ldns_get_errorstr_by_id(zonemd_result));

		else if (verbosity > 3)
			fprintf( myout
			       , "Zone digest matched the zone content\n");

		if (zonemd_result)
			result = zonemd_result;

	} else if (zonemd_required)
		result = LDNS_STATUS_NO_ZONEMD;

	if (result == LDNS_STATUS_OK) {
		if (verbosity >= 3) {
			fprintf(myout, "Zone is verified and complete\n");
		}
	} else if (verbosity > 0)
		fprintf(myerr, "There were errors in the zone\n");

	ldns_dnssec_zone_deep_free(dnssec_zone);
	fclose(fp);
	exit(result);
}

#else

int
main(int argc, char **argv)
{
	fprintf(stderr, "ldns-verify-zone needs OpenSSL support, "
			"which has not been compiled in\n");
	return 1;
}
#endif /* HAVE_SSL */

