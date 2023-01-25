/*
 * special zone file structures and functions for better dnssec handling
 */

#include <ldns/config.h>

#include <ldns/ldns.h>

ldns_dnssec_rrs *
ldns_dnssec_rrs_new(void)
{
	ldns_dnssec_rrs *new_rrs;
	new_rrs = LDNS_MALLOC(ldns_dnssec_rrs);
        if(!new_rrs) return NULL;
	new_rrs->rr = NULL;
	new_rrs->next = NULL;
	return new_rrs;
}

INLINE void
ldns_dnssec_rrs_free_internal(ldns_dnssec_rrs *rrs, int deep)
{
	ldns_dnssec_rrs *next;
	while (rrs) {
		next = rrs->next;
		if (deep) {
			ldns_rr_free(rrs->rr);
		}
		LDNS_FREE(rrs);
		rrs = next;
	}
}

void
ldns_dnssec_rrs_free(ldns_dnssec_rrs *rrs)
{
	ldns_dnssec_rrs_free_internal(rrs, 0);
}

void
ldns_dnssec_rrs_deep_free(ldns_dnssec_rrs *rrs)
{
	ldns_dnssec_rrs_free_internal(rrs, 1);
}

ldns_status
ldns_dnssec_rrs_add_rr(ldns_dnssec_rrs *rrs, ldns_rr *rr)
{
	int cmp;
	ldns_dnssec_rrs *new_rrs;
	if (!rrs || !rr) {
		return LDNS_STATUS_ERR;
	}

	/* this could be done more efficiently; name and type should already
	   be equal */
	cmp = ldns_rr_compare(rrs->rr, rr);
	if (cmp < 0) {
		if (rrs->next) {
			return ldns_dnssec_rrs_add_rr(rrs->next, rr);
		} else {
			new_rrs = ldns_dnssec_rrs_new();
			new_rrs->rr = rr;
			rrs->next = new_rrs;
		}
	} else if (cmp > 0) {
		/* put the current old rr in the new next, put the new
		   rr in the current container */
		new_rrs = ldns_dnssec_rrs_new();
		new_rrs->rr = rrs->rr;
		new_rrs->next = rrs->next;
		rrs->rr = rr;
		rrs->next = new_rrs;
	}
	/* Silently ignore equal rr's */
	return LDNS_STATUS_OK;
}

void
ldns_dnssec_rrs_print_fmt(FILE *out, const ldns_output_format *fmt,
	       const ldns_dnssec_rrs *rrs)
{
	if (!rrs) {
		if ((fmt->flags & LDNS_COMMENT_LAYOUT))
			fprintf(out, "; <void>");
	} else {
		if (rrs->rr) {
			ldns_rr_print_fmt(out, fmt, rrs->rr);
		}
		if (rrs->next) {
			ldns_dnssec_rrs_print_fmt(out, fmt, rrs->next);
		}
	}
}

void
ldns_dnssec_rrs_print(FILE *out, const ldns_dnssec_rrs *rrs)
{
	ldns_dnssec_rrs_print_fmt(out, ldns_output_format_default, rrs);
}


ldns_dnssec_rrsets *
ldns_dnssec_rrsets_new(void)
{
	ldns_dnssec_rrsets *new_rrsets;
	new_rrsets = LDNS_MALLOC(ldns_dnssec_rrsets);
        if(!new_rrsets) return NULL;
	new_rrsets->rrs = NULL;
	new_rrsets->type = 0;
	new_rrsets->signatures = NULL;
	new_rrsets->next = NULL;
	return new_rrsets;
}

INLINE void
ldns_dnssec_rrsets_free_internal(ldns_dnssec_rrsets *rrsets, int deep)
{
	if (rrsets) {
		if (rrsets->rrs) {
			ldns_dnssec_rrs_free_internal(rrsets->rrs, deep);
		}
		if (rrsets->next) {
			ldns_dnssec_rrsets_free_internal(rrsets->next, deep);
		}
		if (rrsets->signatures) {
			ldns_dnssec_rrs_free_internal(rrsets->signatures, deep);
		}
		LDNS_FREE(rrsets);
	}
}

void
ldns_dnssec_rrsets_free(ldns_dnssec_rrsets *rrsets)
{
	ldns_dnssec_rrsets_free_internal(rrsets, 0);
}

void
ldns_dnssec_rrsets_deep_free(ldns_dnssec_rrsets *rrsets)
{
	ldns_dnssec_rrsets_free_internal(rrsets, 1);
}

ldns_rr_type
ldns_dnssec_rrsets_type(const ldns_dnssec_rrsets *rrsets)
{
	if (rrsets) {
		return rrsets->type;
	} else {
		return 0;
	}
}

ldns_status
ldns_dnssec_rrsets_set_type(ldns_dnssec_rrsets *rrsets,
					   ldns_rr_type type)
{
	if (rrsets) {
		rrsets->type = type;
		return LDNS_STATUS_OK;
	}
	return LDNS_STATUS_ERR;
}

static ldns_dnssec_rrsets *
ldns_dnssec_rrsets_new_frm_rr(ldns_rr *rr)
{
	ldns_dnssec_rrsets *new_rrsets;
	ldns_rr_type rr_type;
	bool rrsig;

	new_rrsets = ldns_dnssec_rrsets_new();
	rr_type = ldns_rr_get_type(rr);
	if (rr_type == LDNS_RR_TYPE_RRSIG) {
		rrsig = true;
		rr_type = ldns_rdf2rr_type(ldns_rr_rrsig_typecovered(rr));
	} else {
		rrsig = false;
	}
	if (!rrsig) {
		new_rrsets->rrs = ldns_dnssec_rrs_new();
		new_rrsets->rrs->rr = rr;
	} else {
		new_rrsets->signatures = ldns_dnssec_rrs_new();
		new_rrsets->signatures->rr = rr;
	}
	new_rrsets->type = rr_type;
	return new_rrsets;
}

ldns_status
ldns_dnssec_rrsets_add_rr(ldns_dnssec_rrsets *rrsets, ldns_rr *rr)
{
	ldns_dnssec_rrsets *new_rrsets;
	ldns_rr_type rr_type;
	bool rrsig = false;
	ldns_status result = LDNS_STATUS_OK;

	if (!rrsets || !rr) {
		return LDNS_STATUS_ERR;
	}

	rr_type = ldns_rr_get_type(rr);

	if (rr_type == LDNS_RR_TYPE_RRSIG) {
		rrsig = true;
		rr_type = ldns_rdf2rr_type(ldns_rr_rrsig_typecovered(rr));
	}

	if (!rrsets->rrs && rrsets->type == 0 && !rrsets->signatures) {
		if (!rrsig) {
			rrsets->rrs = ldns_dnssec_rrs_new();
			rrsets->rrs->rr = rr;
			rrsets->type = rr_type;
		} else {
			rrsets->signatures = ldns_dnssec_rrs_new();
			rrsets->signatures->rr = rr;
			rrsets->type = rr_type;
		}
		return LDNS_STATUS_OK;
	}

	if (rr_type > ldns_dnssec_rrsets_type(rrsets)) {
		if (rrsets->next) {
			result = ldns_dnssec_rrsets_add_rr(rrsets->next, rr);
		} else {
			new_rrsets = ldns_dnssec_rrsets_new_frm_rr(rr);
			rrsets->next = new_rrsets;
		}
	} else if (rr_type < ldns_dnssec_rrsets_type(rrsets)) {
		/* move the current one into the new next, 
		   replace field of current with data from new rr */
		new_rrsets = ldns_dnssec_rrsets_new();
		new_rrsets->rrs = rrsets->rrs;
		new_rrsets->type = rrsets->type;
		new_rrsets->signatures = rrsets->signatures;
		new_rrsets->next = rrsets->next;
		if (!rrsig) {
			rrsets->rrs = ldns_dnssec_rrs_new();
			rrsets->rrs->rr = rr;
			rrsets->signatures = NULL;
		} else {
			rrsets->rrs = NULL;
			rrsets->signatures = ldns_dnssec_rrs_new();
			rrsets->signatures->rr = rr;
		}
		rrsets->type = rr_type;
		rrsets->next = new_rrsets;
	} else {
		/* equal, add to current rrsets */
		if (rrsig) {
			if (rrsets->signatures) {
				result = ldns_dnssec_rrs_add_rr(rrsets->signatures, rr);
			} else {
				rrsets->signatures = ldns_dnssec_rrs_new();
				rrsets->signatures->rr = rr;
			}
		} else {
			if (rrsets->rrs) {
				result = ldns_dnssec_rrs_add_rr(rrsets->rrs, rr);
			} else {
				rrsets->rrs = ldns_dnssec_rrs_new();
				rrsets->rrs->rr = rr;
			}
		}
	}

	return result;
}

static void
ldns_dnssec_rrsets_print_soa_fmt(FILE *out, const ldns_output_format *fmt,
		const ldns_dnssec_rrsets *rrsets,
		bool follow,
		bool show_soa)
{
	if (!rrsets) {
		if ((fmt->flags & LDNS_COMMENT_LAYOUT))
			fprintf(out, "; <void>\n");
	} else {
		if (rrsets->rrs &&
		    (show_soa ||
			ldns_rr_get_type(rrsets->rrs->rr) != LDNS_RR_TYPE_SOA
		    )
		   ) {
			ldns_dnssec_rrs_print_fmt(out, fmt, rrsets->rrs);
			if (rrsets->signatures) {
				ldns_dnssec_rrs_print_fmt(out, fmt, 
						rrsets->signatures);
			}
		}
		if (follow && rrsets->next) {
			ldns_dnssec_rrsets_print_soa_fmt(out, fmt, 
					rrsets->next, follow, show_soa);
		}
	}
}


void
ldns_dnssec_rrsets_print_fmt(FILE *out, const ldns_output_format *fmt,
		const ldns_dnssec_rrsets *rrsets, 
		bool follow)
{
	ldns_dnssec_rrsets_print_soa_fmt(out, fmt, rrsets, follow, true);
}

void
ldns_dnssec_rrsets_print(FILE *out, const ldns_dnssec_rrsets *rrsets, bool follow)
{
	ldns_dnssec_rrsets_print_fmt(out, ldns_output_format_default, 
			rrsets, follow);
}

ldns_dnssec_name *
ldns_dnssec_name_new(void)
{
	ldns_dnssec_name *new_name;

	new_name = LDNS_CALLOC(ldns_dnssec_name, 1);
	if (!new_name) {
		return NULL;
	}
	/*
	 * not needed anymore because CALLOC initializes everything to zero.

	new_name->name = NULL;
	new_name->rrsets = NULL;
	new_name->name_alloced = false;
	new_name->nsec = NULL;
	new_name->nsec_signatures = NULL;

	new_name->is_glue = false;
	new_name->hashed_name = NULL;

	 */
	return new_name;
}

ldns_dnssec_name *
ldns_dnssec_name_new_frm_rr(ldns_rr *rr)
{
	ldns_dnssec_name *new_name = ldns_dnssec_name_new();

	new_name->name = ldns_rr_owner(rr);
	if(ldns_dnssec_name_add_rr(new_name, rr) != LDNS_STATUS_OK) {
		ldns_dnssec_name_free(new_name);
		return NULL;
	}

	return new_name;
}

INLINE void
ldns_dnssec_name_free_internal(ldns_dnssec_name *name,
                               int deep)
{
	if (name) {
		if (name->name_alloced) {
			ldns_rdf_deep_free(name->name);
		}
		if (name->rrsets) {
			ldns_dnssec_rrsets_free_internal(name->rrsets, deep);
		}
		if (name->nsec && deep) {
			ldns_rr_free(name->nsec);
		}
		if (name->nsec_signatures) {
			ldns_dnssec_rrs_free_internal(name->nsec_signatures, deep);
		}
		if (name->hashed_name) {
			/* Hashed name is always allocated when signing,
			 * so always deep free
			 */
			ldns_rdf_deep_free(name->hashed_name);
		}
		LDNS_FREE(name);
	}
}

void
ldns_dnssec_name_free(ldns_dnssec_name *name)
{
  ldns_dnssec_name_free_internal(name, 0);
}

void
ldns_dnssec_name_deep_free(ldns_dnssec_name *name)
{
  ldns_dnssec_name_free_internal(name, 1);
}

ldns_rdf *
ldns_dnssec_name_name(const ldns_dnssec_name *name)
{
	if (name) {
		return name->name;
	}
	return NULL;
}

bool
ldns_dnssec_name_is_glue(const ldns_dnssec_name *name)
{
	if (name) {
		return name->is_glue;
	}
	return false;
}

void
ldns_dnssec_name_set_name(ldns_dnssec_name *rrset,
					 ldns_rdf *dname)
{
	if (rrset && dname) {
		rrset->name = dname;
	}
}


void
ldns_dnssec_name_set_nsec(ldns_dnssec_name *rrset, ldns_rr *nsec)
{
	if (rrset && nsec) {
		rrset->nsec = nsec;
	}
}

int
ldns_dnssec_name_cmp(const void *a, const void *b)
{
	ldns_dnssec_name *na = (ldns_dnssec_name *) a;
	ldns_dnssec_name *nb = (ldns_dnssec_name *) b;

	if (na && nb) {
		return ldns_dname_compare(ldns_dnssec_name_name(na),
							 ldns_dnssec_name_name(nb));
	} else if (na) {
		return 1;
	} else if (nb) {
		return -1;
	} else {
		return 0;
	}
}

ldns_status
ldns_dnssec_name_add_rr(ldns_dnssec_name *name,
				    ldns_rr *rr)
{
	ldns_status result = LDNS_STATUS_OK;
	ldns_rr_type rr_type;
	ldns_rr_type typecovered = 0;

	/* special handling for NSEC3 and NSECX covering RRSIGS */

	if (!name || !rr) {
		return LDNS_STATUS_ERR;
	}

	rr_type = ldns_rr_get_type(rr);

	if (rr_type == LDNS_RR_TYPE_RRSIG) {
		typecovered = ldns_rdf2rr_type(ldns_rr_rrsig_typecovered(rr));
	}

	if (rr_type == LDNS_RR_TYPE_NSEC ||
	    rr_type == LDNS_RR_TYPE_NSEC3) {
		/* XX check if is already set (and error?) */
		name->nsec = rr;
	} else if (typecovered == LDNS_RR_TYPE_NSEC ||
			 typecovered == LDNS_RR_TYPE_NSEC3) {
		if (name->nsec_signatures) {
			result = ldns_dnssec_rrs_add_rr(name->nsec_signatures, rr);
		} else {
			name->nsec_signatures = ldns_dnssec_rrs_new();
			name->nsec_signatures->rr = rr;
		}
	} else {
		/* it's a 'normal' RR, add it to the right rrset */
		if (name->rrsets) {
			result = ldns_dnssec_rrsets_add_rr(name->rrsets, rr);
		} else {
			name->rrsets = ldns_dnssec_rrsets_new();
			result = ldns_dnssec_rrsets_add_rr(name->rrsets, rr);
		}
	}
	return result;
}

ldns_dnssec_rrsets *
ldns_dnssec_name_find_rrset(const ldns_dnssec_name *name,
					   ldns_rr_type type) {
	ldns_dnssec_rrsets *result;

	result = name->rrsets;
	while (result) {
		if (result->type == type) {
			return result;
		} else {
			result = result->next;
		}
	}
	return NULL;
}

ldns_dnssec_rrsets *
ldns_dnssec_zone_find_rrset(const ldns_dnssec_zone *zone,
					   const ldns_rdf *dname,
					   ldns_rr_type type)
{
	ldns_rbnode_t *node;

	if (!zone || !dname || !zone->names) {
		return NULL;
	}

	node = ldns_rbtree_search(zone->names, dname);
	if (node) {
		return ldns_dnssec_name_find_rrset((ldns_dnssec_name *)node->data,
									type);
	} else {
		return NULL;
	}
}

static void
ldns_dnssec_name_print_soa_fmt(FILE *out, const ldns_output_format *fmt,
		const ldns_dnssec_name *name, 
		bool show_soa)
{
	if (name) {
		if(name->rrsets) {
			ldns_dnssec_rrsets_print_soa_fmt(out, fmt, 
					name->rrsets, true, show_soa);
		} else if ((fmt->flags & LDNS_COMMENT_LAYOUT)) {
			fprintf(out, ";; Empty nonterminal: ");
			ldns_rdf_print(out, name->name);
			fprintf(out, "\n");
		}
		if(name->nsec) {
			ldns_rr_print_fmt(out, fmt, name->nsec);
		}
		if (name->nsec_signatures) {
			ldns_dnssec_rrs_print_fmt(out, fmt, 
					name->nsec_signatures);
		}
	} else if ((fmt->flags & LDNS_COMMENT_LAYOUT)) {
		fprintf(out, "; <void>\n");
	}
}


void
ldns_dnssec_name_print_fmt(FILE *out, const ldns_output_format *fmt,
		const ldns_dnssec_name *name)
{
	ldns_dnssec_name_print_soa_fmt(out, fmt, name, true);
}

void
ldns_dnssec_name_print(FILE *out, const ldns_dnssec_name *name)
{
	ldns_dnssec_name_print_fmt(out, ldns_output_format_default, name);
}


ldns_dnssec_zone *
ldns_dnssec_zone_new(void)
{
	ldns_dnssec_zone *zone = LDNS_MALLOC(ldns_dnssec_zone);
        if(!zone) return NULL;
	zone->soa = NULL;
	zone->names = NULL;
	zone->hashed_names = NULL;
	zone->_nsec3params = NULL;

	return zone;
}

static bool
rr_is_rrsig_covering(ldns_rr* rr, ldns_rr_type t)
{
	return     ldns_rr_get_type(rr) == LDNS_RR_TYPE_RRSIG
		&& ldns_rdf2rr_type(ldns_rr_rrsig_typecovered(rr)) == t;
}

/* When the zone is first read into an list and then inserted into an
 * ldns_dnssec_zone (rbtree) the nodes of the rbtree are allocated close (next)
 * to each other. Because ldns-verify-zone (the only program that uses this
 * function) uses the rbtree mostly for sequential walking, this results
 * in a speed increase (of 15% on linux) because we have less CPU-cache misses.
 */
#define FASTER_DNSSEC_ZONE_NEW_FRM_FP 1 /* Because of L2 cache efficiency */

static ldns_status
ldns_dnssec_zone_add_empty_nonterminals_nsec3(
		ldns_dnssec_zone *zone, ldns_rbtree_t *nsec3s);

static void
ldns_todo_nsec3_ents_node_free(ldns_rbnode_t *node, void *arg) {
	(void) arg;
	ldns_rdf_deep_free((ldns_rdf *)node->key);
	LDNS_FREE(node);
}

ldns_status _ldns_rr_new_frm_fp_l_internal(ldns_rr **newrr, FILE *fp,
		uint32_t *default_ttl, ldns_rdf **origin, ldns_rdf **prev,
		int *line_nr, bool *explicit_ttl);

ldns_status
ldns_dnssec_zone_new_frm_fp_l(ldns_dnssec_zone** z, FILE* fp, const ldns_rdf* origin,
		uint32_t default_ttl, ldns_rr_class ATTR_UNUSED(c), int* line_nr)
{
	ldns_rr* cur_rr;
	size_t i;

	ldns_rdf *my_origin = NULL;
	ldns_rdf *my_prev = NULL;

	ldns_dnssec_zone *newzone = ldns_dnssec_zone_new();
	/* NSEC3s may occur before the names they refer to. We must remember
	   them and add them to the name later on, after the name is read.
	   We track not yet  matching NSEC3s*n the todo_nsec3s list */
	ldns_rr_list* todo_nsec3s = ldns_rr_list_new();
	/* when reading NSEC3s, there is a chance that we encounter nsecs
	   for empty nonterminals, whose nonterminals we cannot derive yet
	   because the needed information is to be read later.

	   nsec3_ents (where ent is e.n.t.; i.e. empty non terminal) will
	   hold the NSEC3s that still didn't have a matching name in the
	   zone tree, even after all names were read.  They can only match
	   after the zone is equipped with all the empty non terminals. */
	ldns_rbtree_t todo_nsec3_ents;
	ldns_rbnode_t *new_node;
	ldns_rr_list* todo_nsec3_rrsigs = ldns_rr_list_new();

	ldns_status status;

#ifdef FASTER_DNSSEC_ZONE_NEW_FRM_FP
	ldns_zone* zone = NULL;
#else
	ldns_rr  *prev_rr = NULL;
	uint32_t   my_ttl = default_ttl;
	/* RFC 1035 Section 5.1, says 'Omitted class and TTL values are default
	 * to the last explicitly stated values.'
	 */
	bool ttl_from_TTL = false;
	bool explicit_ttl = false;
#endif

	ldns_rbtree_init(&todo_nsec3_ents, ldns_dname_compare_v);

#ifdef FASTER_DNSSEC_ZONE_NEW_FRM_FP
	status = ldns_zone_new_frm_fp_l(&zone, fp, origin, default_ttl, c, line_nr);
	if (status != LDNS_STATUS_OK)
		goto error;
#endif
	if (!newzone || !todo_nsec3s || !todo_nsec3_rrsigs ) {
		status = LDNS_STATUS_MEM_ERR;
		goto error;
	}
	if (origin) {
		if (!(my_origin = ldns_rdf_clone(origin))) {
			status = LDNS_STATUS_MEM_ERR;
			goto error;
		}
		if (!(my_prev   = ldns_rdf_clone(origin))) {
			status = LDNS_STATUS_MEM_ERR;
			goto error;
		}
	}

#ifdef FASTER_DNSSEC_ZONE_NEW_FRM_FP
	if (ldns_zone_soa(zone)) {
		status = ldns_dnssec_zone_add_rr(newzone, ldns_zone_soa(zone));
		if (status != LDNS_STATUS_OK)
			goto error;
	}
	for (i = 0; i < ldns_rr_list_rr_count(ldns_zone_rrs(zone)); i++) {
		cur_rr = ldns_rr_list_rr(ldns_zone_rrs(zone), i);
		status = LDNS_STATUS_OK;
#else
	while (!feof(fp)) {
		/* If ttl came from $TTL line, then it should be the default.
		 * (RFC 2308 Section 4)
		 * Otherwise it "defaults to the last explicitly stated value"
		 * (RFC 1035 Section 5.1)
		 */
		if (ttl_from_TTL)
			my_ttl = default_ttl;
		status = ldns_rr_new_frm_fp_l(&cur_rr, fp, &my_ttl, &my_origin,
				&my_prev, line_nr, &explicit_ttl);
#endif
		switch (status) {
		case LDNS_STATUS_OK:
#ifndef FASTER_DNSSEC_ZONE_NEW_FRM_FP
			if (explicit_ttl) {
				if (!ttl_from_TTL) {
					/* No $TTL, so ttl "defaults to the
					 * last explicitly stated value"
					 * (RFC 1035 Section 5.1)
					 */
					my_ttl = ldns_rr_ttl(cur_rr);
				}
			/* When ttl is implicit, try to adhere to the rules as
			 * much as possible. (also for compatibility with bind)
			 * This was changed when fixing an issue with ZONEMD
			 * which hashes the TTL too.
			 */
			} else if (ldns_rr_get_type(cur_rr) == LDNS_RR_TYPE_SIG
			       ||  ldns_rr_get_type(cur_rr) == LDNS_RR_TYPE_RRSIG) {
				if (ldns_rr_rd_count(cur_rr) >= 4
				&&  ldns_rdf_get_type(ldns_rr_rdf(cur_rr, 3)) == LDNS_RDF_TYPE_INT32)

					/* SIG without explicit ttl get ttl
					 * from the original_ttl field
					 * (RFC 2535 Section 7.2)
					 *
					 * Similarly for RRSIG, but stated less
					 * specifically in the spec.
					 * (RFC 4034 Section 3)
					 */
					ldns_rr_set_ttl(cur_rr,
					    ldns_rdf2native_int32(
					        ldns_rr_rdf(rr, 3)));

			} else if (prev_rr
			       &&  ldns_rr_get_type(prev_rr) == ldns_rr_get_type(cur_rr)
			       &&  ldns_dname_compare( ldns_rr_owner(prev_rr)
			                             , ldns_rr_owner(cur_rr)) == 0)

				/* "TTLs of all RRs in an RRSet must be the same"
				 * (RFC 2881 Section 5.2)
				 */
				ldns_rr_set_ttl(cur_rr, ldns_rr_ttl(prev_rr));

			prev_rr = cur_rr;
#endif
			status = ldns_dnssec_zone_add_rr(newzone, cur_rr);
			if (status ==
				LDNS_STATUS_DNSSEC_NSEC3_ORIGINAL_NOT_FOUND) {

				if (rr_is_rrsig_covering(cur_rr,
							LDNS_RR_TYPE_NSEC3)){
					ldns_rr_list_push_rr(todo_nsec3_rrsigs,
							cur_rr);
				} else {
					ldns_rr_list_push_rr(todo_nsec3s,
						       	cur_rr);
				}
				status = LDNS_STATUS_OK;

			} else if (status != LDNS_STATUS_OK)
				goto error;

			break;

		case LDNS_STATUS_SYNTAX_TTL:	/* the ttl was set*/
#ifndef FASTER_DNSSEC_ZONE_NEW_FRM_FP
			default_ttl = my_ttl;
			ttl_from_TTL = true;
#endif
			status = LDNS_STATUS_OK;
			break;


		case LDNS_STATUS_SYNTAX_EMPTY:	/* empty line was seen */
		case LDNS_STATUS_SYNTAX_ORIGIN:	/* the origin was set*/
			status = LDNS_STATUS_OK;
			break;

		case LDNS_STATUS_SYNTAX_INCLUDE:/* $include not implemented */
			status =  LDNS_STATUS_SYNTAX_INCLUDE_ERR_NOTIMPL;
			break;

		default:
			goto error;
		}
	}

	for (i = 0; status == LDNS_STATUS_OK &&
			i < ldns_rr_list_rr_count(todo_nsec3s); i++) {
		cur_rr = ldns_rr_list_rr(todo_nsec3s, i);
		status = ldns_dnssec_zone_add_rr(newzone, cur_rr);
		if (status == LDNS_STATUS_DNSSEC_NSEC3_ORIGINAL_NOT_FOUND) {
			if (!(new_node = LDNS_MALLOC(ldns_rbnode_t))) {
				status = LDNS_STATUS_MEM_ERR;
				break;
			}
			new_node->key  = ldns_dname_label(ldns_rr_owner(cur_rr), 0);
			new_node->data = cur_rr;
			if (!ldns_rbtree_insert(&todo_nsec3_ents, new_node)) {
				LDNS_FREE(new_node);
				status = LDNS_STATUS_MEM_ERR;
				break;
			}
			status = LDNS_STATUS_OK;
		}
	}
	if (todo_nsec3_ents.count > 0)
		(void) ldns_dnssec_zone_add_empty_nonterminals_nsec3(
				newzone, &todo_nsec3_ents);
	for (i = 0; status == LDNS_STATUS_OK &&
			i < ldns_rr_list_rr_count(todo_nsec3_rrsigs); i++) {
		cur_rr = ldns_rr_list_rr(todo_nsec3_rrsigs, i);
		status = ldns_dnssec_zone_add_rr(newzone, cur_rr);
	}
	if (z) {
		*z = newzone;
		newzone = NULL;
	} else {
		ldns_dnssec_zone_free(newzone);
		newzone = NULL;
	}

error:
#ifdef FASTER_DNSSEC_ZONE_NEW_FRM_FP
	if (zone) {
		ldns_zone_free(zone);
	}
#endif
	ldns_rr_list_free(todo_nsec3_rrsigs);
	ldns_traverse_postorder(&todo_nsec3_ents,
			ldns_todo_nsec3_ents_node_free, NULL);
	ldns_rr_list_free(todo_nsec3s);

	if (my_origin) {
		ldns_rdf_deep_free(my_origin);
	}
	if (my_prev) {
		ldns_rdf_deep_free(my_prev);
	}
	if (newzone) {
		ldns_dnssec_zone_free(newzone);
	}
	return status;
}

ldns_status
ldns_dnssec_zone_new_frm_fp(ldns_dnssec_zone** z, FILE* fp, const ldns_rdf* origin,
		uint32_t ttl, ldns_rr_class ATTR_UNUSED(c))
{
	return ldns_dnssec_zone_new_frm_fp_l(z, fp, origin, ttl, c, NULL);
}

static void
ldns_dnssec_name_node_free(ldns_rbnode_t *node, void *arg) {
	(void) arg;
	ldns_dnssec_name_free((ldns_dnssec_name *)node->data);
	LDNS_FREE(node);
}

static void
ldns_dnssec_name_node_deep_free(ldns_rbnode_t *node, void *arg) {
	(void) arg;
	ldns_dnssec_name_deep_free((ldns_dnssec_name *)node->data);
	LDNS_FREE(node);
}

static void
ldns_hashed_names_node_free(ldns_rbnode_t *node, void *arg) {
	(void) arg;
	LDNS_FREE(node);
}

void
ldns_dnssec_zone_free(ldns_dnssec_zone *zone)
{
	if (zone) {
		if (zone->hashed_names) {
			ldns_traverse_postorder(zone->hashed_names,
					ldns_hashed_names_node_free, NULL);
			LDNS_FREE(zone->hashed_names);
		}
		if (zone->names) {
			/* destroy all name structures within the tree */
			ldns_traverse_postorder(zone->names,
						    ldns_dnssec_name_node_free,
						    NULL);
			LDNS_FREE(zone->names);
		}
		LDNS_FREE(zone);
	}
}

void
ldns_dnssec_zone_deep_free(ldns_dnssec_zone *zone)
{
	if (zone) {
		if (zone->hashed_names) {
			ldns_traverse_postorder(zone->hashed_names,
					ldns_hashed_names_node_free, NULL);
			LDNS_FREE(zone->hashed_names);
		}
		if (zone->names) {
			/* destroy all name structures within the tree */
			ldns_traverse_postorder(zone->names,
						    ldns_dnssec_name_node_deep_free,
						    NULL);
			LDNS_FREE(zone->names);
		}
		LDNS_FREE(zone);
	}
}

/* use for dname comparison in tree */
int
ldns_dname_compare_v(const void *a, const void *b) {
	return ldns_dname_compare((ldns_rdf *)a, (ldns_rdf *)b);
}

static void
ldns_dnssec_name_make_hashed_name(ldns_dnssec_zone *zone,
		ldns_dnssec_name* name, ldns_rr* nsec3rr);

static void
ldns_dnssec_zone_hashed_names_from_nsec3(
		ldns_dnssec_zone* zone, ldns_rr* nsec3rr)
{
	ldns_rbnode_t* current_node;
	ldns_dnssec_name* current_name;

	assert(zone != NULL);
	assert(nsec3rr != NULL);

	if (zone->hashed_names) {
		ldns_traverse_postorder(zone->hashed_names,
				ldns_hashed_names_node_free, NULL);
		LDNS_FREE(zone->hashed_names);
	}
	zone->_nsec3params = nsec3rr;

	/* So this is a NSEC3 zone.
	* Calculate hashes for all names already in the zone
	*/
	zone->hashed_names = ldns_rbtree_create(ldns_dname_compare_v);
	if (zone->hashed_names == NULL) {
		return;
	}
	for ( current_node  = ldns_rbtree_first(zone->names)
	    ; current_node != LDNS_RBTREE_NULL
	    ; current_node  = ldns_rbtree_next(current_node)
	    ) {
		current_name = (ldns_dnssec_name *) current_node->data;
		ldns_dnssec_name_make_hashed_name(zone, current_name, nsec3rr);

	}
}

static void
ldns_dnssec_name_make_hashed_name(ldns_dnssec_zone *zone,
		ldns_dnssec_name* name, ldns_rr* nsec3rr)
{
	ldns_rbnode_t* new_node;

	assert(name != NULL);
	if (! zone->_nsec3params) {
		if (! nsec3rr) {
			return;
		}
		ldns_dnssec_zone_hashed_names_from_nsec3(zone, nsec3rr);

	} else if (! nsec3rr) {
		nsec3rr = zone->_nsec3params;
	}
	name->hashed_name = ldns_nsec3_hash_name_frm_nsec3(nsec3rr, name->name);

	/* Also store in zone->hashed_names */
	if ((new_node = LDNS_MALLOC(ldns_rbnode_t))) {

		new_node->key  = name->hashed_name;
		new_node->data = name;

		if (ldns_rbtree_insert(zone->hashed_names, new_node) == NULL) {

				LDNS_FREE(new_node);
		}
	}
}


static ldns_rbnode_t *
ldns_dnssec_zone_find_nsec3_original(ldns_dnssec_zone *zone, ldns_rr *rr) {
	ldns_rdf *hashed_name;
	ldns_rbnode_t *to_return;

	if (ldns_rr_get_type(rr) == LDNS_RR_TYPE_NSEC3 && ! zone->_nsec3params){

		ldns_dnssec_zone_hashed_names_from_nsec3(zone, rr);
	}
	if (zone->hashed_names == NULL) {
		return NULL;
	}
	hashed_name = ldns_dname_label(ldns_rr_owner(rr), 0);
	if (hashed_name == NULL) {
		return NULL;
	}
	to_return = ldns_rbtree_search(zone->hashed_names, hashed_name);
	ldns_rdf_deep_free(hashed_name);
	return to_return;
}

ldns_status
ldns_dnssec_zone_add_rr(ldns_dnssec_zone *zone, ldns_rr *rr)
{
	ldns_status result = LDNS_STATUS_OK;
	ldns_dnssec_name *cur_name;
	ldns_rbnode_t *cur_node;
	ldns_rr_type type_covered = 0;

	if (!zone || !rr) {
		return LDNS_STATUS_ERR;
	}

	if (!zone->names) {
		zone->names = ldns_rbtree_create(ldns_dname_compare_v);
                if(!zone->names) return LDNS_STATUS_MEM_ERR;
	}

	/* we need the original of the hashed name if this is
	   an NSEC3, or an RRSIG that covers an NSEC3 */
	if (ldns_rr_get_type(rr) == LDNS_RR_TYPE_RRSIG) {
		type_covered = ldns_rdf2rr_type(ldns_rr_rrsig_typecovered(rr));
	}
	if (ldns_rr_get_type(rr) == LDNS_RR_TYPE_NSEC3 ||
	    type_covered == LDNS_RR_TYPE_NSEC3) {
		cur_node = ldns_dnssec_zone_find_nsec3_original(zone, rr);
		if (!cur_node) {
			return LDNS_STATUS_DNSSEC_NSEC3_ORIGINAL_NOT_FOUND;
		}
	} else {
		cur_node = ldns_rbtree_search(zone->names, ldns_rr_owner(rr));
	}
	if (!cur_node) {
		/* add */
		cur_name = ldns_dnssec_name_new_frm_rr(rr);
                if(!cur_name) return LDNS_STATUS_MEM_ERR;
		cur_node = LDNS_MALLOC(ldns_rbnode_t);
                if(!cur_node) {
                        ldns_dnssec_name_free(cur_name);
                        return LDNS_STATUS_MEM_ERR;
                }
		cur_node->key = ldns_rr_owner(rr);
		cur_node->data = cur_name;
		(void)ldns_rbtree_insert(zone->names, cur_node);
		ldns_dnssec_name_make_hashed_name(zone, cur_name, NULL);
	} else {
		cur_name = (ldns_dnssec_name *) cur_node->data;
		result = ldns_dnssec_name_add_rr(cur_name, rr);
	}
	if (ldns_rr_get_type(rr) == LDNS_RR_TYPE_SOA) {
		zone->soa = cur_name;
	}
	return result;
}

void
ldns_dnssec_zone_names_print_fmt(FILE *out, const ldns_output_format *fmt,
		const ldns_rbtree_t *tree, 
		bool print_soa)
{
	ldns_rbnode_t *node;
	ldns_dnssec_name *name;

	node = ldns_rbtree_first(tree);
	while (node != LDNS_RBTREE_NULL) {
		name = (ldns_dnssec_name *) node->data;
		ldns_dnssec_name_print_soa_fmt(out, fmt, name, print_soa);
		if ((fmt->flags & LDNS_COMMENT_LAYOUT))
			fprintf(out, ";\n");
		node = ldns_rbtree_next(node);
	}
}

void
ldns_dnssec_zone_names_print(FILE *out, const ldns_rbtree_t *tree, bool print_soa)
{
	ldns_dnssec_zone_names_print_fmt(out, ldns_output_format_default,
		       tree, print_soa);
}

void
ldns_dnssec_zone_print_fmt(FILE *out, const ldns_output_format *fmt,
	       const ldns_dnssec_zone *zone)
{
	if (zone) {
		if (zone->soa) {
			if ((fmt->flags & LDNS_COMMENT_LAYOUT)) {
				fprintf(out, ";; Zone: ");
				ldns_rdf_print(out, ldns_dnssec_name_name(
							zone->soa));
				fprintf(out, "\n;\n");
			}
			ldns_dnssec_rrsets_print_fmt(out, fmt,
					ldns_dnssec_name_find_rrset(
						zone->soa, 
						LDNS_RR_TYPE_SOA), 
					false);
			if ((fmt->flags & LDNS_COMMENT_LAYOUT))
				fprintf(out, ";\n");
		}

		if (zone->names) {
			ldns_dnssec_zone_names_print_fmt(out, fmt, 
					zone->names, false);
		}
	}
}

void
ldns_dnssec_zone_print(FILE *out, const ldns_dnssec_zone *zone)
{
	ldns_dnssec_zone_print_fmt(out, ldns_output_format_default, zone);
}

static ldns_status
ldns_dnssec_zone_add_empty_nonterminals_nsec3(
		ldns_dnssec_zone *zone, ldns_rbtree_t *nsec3s)
{
	ldns_dnssec_name *new_name;
	ldns_rdf *cur_name;
	ldns_rdf *next_name;
	ldns_rbnode_t *cur_node, *next_node, *new_node;

	/* for the detection */
	uint16_t i, cur_label_count, next_label_count;
	uint16_t soa_label_count = 0;
	ldns_rdf *l1, *l2;
	int lpos;

	if (!zone) {
		return LDNS_STATUS_ERR;
	}
	if (zone->soa && zone->soa->name) {
		soa_label_count = ldns_dname_label_count(zone->soa->name);
	}
	
	cur_node = ldns_rbtree_first(zone->names);
	while (cur_node != LDNS_RBTREE_NULL) {
		next_node = ldns_rbtree_next(cur_node);
		
		/* skip glue */
		while (next_node != LDNS_RBTREE_NULL && 
		       next_node->data &&
		       ((ldns_dnssec_name *)next_node->data)->is_glue
		) {
			next_node = ldns_rbtree_next(next_node);
		}

		if (next_node == LDNS_RBTREE_NULL) {
			next_node = ldns_rbtree_first(zone->names);
		}
		if (! cur_node->data || ! next_node->data) {
			return LDNS_STATUS_ERR;
		}
		cur_name = ((ldns_dnssec_name *)cur_node->data)->name;
		next_name = ((ldns_dnssec_name *)next_node->data)->name;
		cur_label_count = ldns_dname_label_count(cur_name);
		next_label_count = ldns_dname_label_count(next_name);

		/* Since the names are in canonical order, we can
		 * recognize empty non-terminals by their labels;
		 * every label after the first one on the next owner
		 * name is a non-terminal if it either does not exist
		 * in the current name or is different from the same
		 * label in the current name (counting from the end)
		 */
		for (i = 1; i < next_label_count - soa_label_count; i++) {
			lpos = (int)cur_label_count - (int)next_label_count + (int)i;
			if (lpos >= 0) {
				l1 = ldns_dname_clone_from(cur_name, (uint8_t)lpos);
			} else {
				l1 = NULL;
			}
			l2 = ldns_dname_clone_from(next_name, i);

			if (!l1 || ldns_dname_compare(l1, l2) != 0) {
				/* We have an empty nonterminal, add it to the
				 * tree
				 */
				ldns_rbnode_t *node = NULL;
				ldns_rdf *ent_name;

				if (!(ent_name = ldns_dname_clone_from(
						next_name, i))) {
					
					ldns_rdf_deep_free(l1);
					ldns_rdf_deep_free(l2);
					return LDNS_STATUS_MEM_ERR;
				}

				if (nsec3s && zone->_nsec3params) {
					ldns_rdf *ent_hashed_name;

					if (!(ent_hashed_name =
					    ldns_nsec3_hash_name_frm_nsec3(
							zone->_nsec3params,
							ent_name))) {
						ldns_rdf_deep_free(l1);
						ldns_rdf_deep_free(l2);
						ldns_rdf_deep_free(ent_name);
						return LDNS_STATUS_MEM_ERR;
					}
					node = ldns_rbtree_search(nsec3s, 
							ent_hashed_name);
					ldns_rdf_deep_free(ent_hashed_name);
					if (!node) {
						ldns_rdf_deep_free(l1);
						ldns_rdf_deep_free(l2);
						ldns_rdf_deep_free(ent_name);
						continue;
					}
				}
				new_name = ldns_dnssec_name_new();
				if (!new_name) {
					ldns_rdf_deep_free(l1);
					ldns_rdf_deep_free(l2);
					ldns_rdf_deep_free(ent_name);
					return LDNS_STATUS_MEM_ERR;
				}
				new_name->name = ent_name;
				new_name->name_alloced = true;
				new_node = LDNS_MALLOC(ldns_rbnode_t);
				if (!new_node) {
					ldns_rdf_deep_free(l1);
					ldns_rdf_deep_free(l2);
					ldns_dnssec_name_free(new_name);
					return LDNS_STATUS_MEM_ERR;
				}
				new_node->key = new_name->name;
				new_node->data = new_name;
				(void)ldns_rbtree_insert(zone->names, new_node);
				ldns_dnssec_name_make_hashed_name(
						zone, new_name, NULL);
				if (node)
					(void) ldns_dnssec_zone_add_rr(zone,
							(ldns_rr *)node->data);
			}
			ldns_rdf_deep_free(l1);
			ldns_rdf_deep_free(l2);
		}
		
		/* we might have inserted a new node after
		 * the current one so we can't just use next()
		 */
		if (next_node != ldns_rbtree_first(zone->names)) {
			cur_node = next_node;
		} else {
			cur_node = LDNS_RBTREE_NULL;
		}
	}
	return LDNS_STATUS_OK;
}

ldns_status
ldns_dnssec_zone_add_empty_nonterminals(ldns_dnssec_zone *zone)
{
	return ldns_dnssec_zone_add_empty_nonterminals_nsec3(zone, NULL);
}

bool
ldns_dnssec_zone_is_nsec3_optout(const ldns_dnssec_zone* zone)
{
	ldns_rr* nsec3;
	ldns_rbnode_t* node;

	if (ldns_dnssec_name_find_rrset(zone->soa, LDNS_RR_TYPE_NSEC3PARAM)) {
		node = ldns_rbtree_first(zone->names);
		while (node != LDNS_RBTREE_NULL) {
			nsec3 = ((ldns_dnssec_name*)node->data)->nsec;
			if (nsec3 &&ldns_rr_get_type(nsec3) 
					== LDNS_RR_TYPE_NSEC3 &&
					ldns_nsec3_optout(nsec3)) {
				return true;
			}
			node = ldns_rbtree_next(node);
		}
	}
	return false;
}

/*
 * Stuff for calculating and verifying zone digests
 */
typedef enum dnssec_zone_rr_iter_state {
	  DNSSEC_ZONE_RR_ITER_LT_RRSIG
	, DNSSEC_ZONE_RR_ITER_RRSIGs_NO_NSEC
	, DNSSEC_ZONE_RR_ITER_REST
	, DNSSEC_ZONE_RR_ITER_RRSIGs_NSEC
	, DNSSEC_ZONE_RR_ITER_RRSIGs_NSEC_REST
	, DNSSEC_ZONE_RR_ITER_NSEC3
	, DNSSEC_ZONE_RR_ITER_FINI
} dnssec_zone_rr_iter_state;

typedef struct dnssec_zone_rr_iter {
	ldns_dnssec_zone         *zone;
	ldns_rbnode_t            *node;
	ldns_dnssec_name         *name;
	ldns_dnssec_rrsets       *rrsets;
	ldns_dnssec_rrs          *rrs;
	ldns_dnssec_rrsets       *rrsets4rrsigs;
	ldns_rbnode_t            *nsec3_node;
	ldns_dnssec_name         *nsec3_name;
	dnssec_zone_rr_iter_state state;
	ldns_rdf                 *apex_name;
	uint8_t                   apex_labs;
} dnssec_zone_rr_iter;

INLINE void
dnssec_zone_rr_iter_set_state_for_next_name(dnssec_zone_rr_iter *i)
{
	/* Make sure the i->name is "in zone" (i.e. below the apex) */
	if (i->apex_name) {
		ldns_rdf *name = (ldns_rdf *)i->node->key;

		while (i->name && name != i->apex_name        /* not apex */

		&& (  ldns_dname_label_count(name) != i->apex_labs
		   || ldns_dname_compare(name, i->apex_name)) /* not apex */

		&& !ldns_dname_is_subdomain(name, i->apex_name) /* no sub */) {

			/* next name */
			i->node = ldns_rbtree_next(i->node);
			if (i->node == LDNS_RBTREE_NULL)
				i->name = NULL;
			else {
				i->name = (ldns_dnssec_name *)i->node->data;
				name = (ldns_rdf *)i->node->key;
			}
		}
	}
	/* determine state */
	if (!i->name) {
		if (!i->nsec3_name)
			i->state = DNSSEC_ZONE_RR_ITER_FINI;
		else {
			i->rrs = i->nsec3_name->nsec_signatures;
			i->state = DNSSEC_ZONE_RR_ITER_NSEC3;
		}
	} else if (!i->nsec3_name) {
		i->rrsets = i->name->rrsets;
		i->state = DNSSEC_ZONE_RR_ITER_LT_RRSIG;

	} else if (ldns_dname_compare( ldns_rr_owner(i->nsec3_name->nsec)
	                             , (ldns_rdf *)i->node->key) < 0) {
		i->rrs = i->nsec3_name->nsec_signatures;
		i->state = DNSSEC_ZONE_RR_ITER_NSEC3;
	} else {
		i->rrsets = i->name->rrsets;
		i->state = DNSSEC_ZONE_RR_ITER_LT_RRSIG;
	}
}

/**
 * Iterate over the RR's in the ldns_dnssec_zone in canonical order.
 * There are three possible paths through the RR's in a ldns_dnssec_name.
 * 
 * 1. There is no NSEC:
 *
 *    1.1. All the RRs in the name->rrsets with type < RRSIG,
 *         state: DNSSEC_ZONE_RR_ITER_LT_RRSIG
 *
 *    1.2. Then all the RRSIGs from name->rrsets (likely none)
 *         state: DNSSEC_ZONE_RR_ITER_RRSIGs_NO_NSEC
 *
 *    1.3. Finally the remaining RRs in name->rrsets (type > RRSIG)
 *         state: DNSSEC_ZONE_RR_ITER_REST
 *
 *
 * 2. There is a NSEC of type NSEC with this name:
 *
 *    2.1. All the RRs in the name->rrsets with type < RRSIG,
 *         state: DNSSEC_ZONE_RR_ITER_LT_RRSIG
 *
 *    2.2. Then all the RRSIGs from name->rrsets with type < NSEC
 *         state: DNSSEC_ZONE_RR_ITER_RRSIGs_NO_NSEC
 *
 *    2.3. Then the signatures of the NSEC RR, followed by
 *         the signatures of the remaining name->rrsets (type > NSEC),
 *         followed by the NSEC rr.
 *         state: DNSSEC_ZONE_RR_ITER_RRSIGs_NO_NSEC
 *
 *    2.4. Finally the remaining RRs in name->rrsets (type > RRSIG)
 *         state: DNSSEC_ZONE_RR_ITER_REST
 *
 *
 * 3. There is a NSEC of type NSEC3 for this name:
 *
 *    3.1. If the NSEC3 name is before the name for other RRsets in the zone,
 *         Then all signatures of the NSEC3 RR, followed by the NSEC3
 *         state: DNSSEC_ZONE_RR_ITER_NSEC3
 *
 *         otherwise follow path for "no NSEC" for the name for other RRsets
 */
static ldns_rr *
dnssec_zone_rr_iter_next(dnssec_zone_rr_iter *i)
{
	ldns_rr *nsec3;

	for (;;) {
		if (i->rrs) {
			ldns_rr *rr = i->rrs->rr;
			i->rrs = i->rrs->next;
			return rr;
		}
		switch (i->state) {
		case DNSSEC_ZONE_RR_ITER_LT_RRSIG:
			if (i->rrsets
			&&  i->rrsets->type < LDNS_RR_TYPE_RRSIG) {

				i->rrs = i->rrsets->rrs;
				i->rrsets = i->rrsets->next;
				break;
			}
			i->rrsets4rrsigs = i->name->rrsets;
			if (i->name->nsec && ldns_rr_get_type(i->name->nsec)
			                               == LDNS_RR_TYPE_NSEC) {

				i->state = DNSSEC_ZONE_RR_ITER_RRSIGs_NSEC;
				break;
			}
			i->state = DNSSEC_ZONE_RR_ITER_RRSIGs_NO_NSEC;
			/* fallthrough */

		case DNSSEC_ZONE_RR_ITER_RRSIGs_NO_NSEC:
			if (i->rrsets4rrsigs) {
				i->rrs = i->rrsets4rrsigs->signatures;
				i->rrsets4rrsigs = i->rrsets4rrsigs->next;
				break;
			}
			i->state = DNSSEC_ZONE_RR_ITER_REST;
			/* fallthrough */

		case DNSSEC_ZONE_RR_ITER_REST:
			if (i->rrsets) {
				i->rrs = i->rrsets->rrs;
				i->rrsets = i->rrsets->next;
				break;
			}
			/* next name */
			i->node = ldns_rbtree_next(i->node);
			i->name = i->node == LDNS_RBTREE_NULL ? NULL
				: (ldns_dnssec_name *)i->node->data;

			dnssec_zone_rr_iter_set_state_for_next_name(i);
			break;

		case DNSSEC_ZONE_RR_ITER_RRSIGs_NSEC:
			if (i->rrsets4rrsigs
			&&  i->rrsets4rrsigs->type < LDNS_RR_TYPE_NSEC) {

				i->rrs = i->rrsets4rrsigs->signatures;
				i->rrsets4rrsigs = i->rrsets4rrsigs->next;
				break;
			}
			i->state = DNSSEC_ZONE_RR_ITER_RRSIGs_NSEC_REST;
			i->rrs = i->name->nsec_signatures;
			break;

		case DNSSEC_ZONE_RR_ITER_RRSIGs_NSEC_REST:
			if (i->rrsets4rrsigs) {
				i->rrs = i->rrsets4rrsigs->signatures;
				i->rrsets4rrsigs = i->rrsets4rrsigs->next;
				break;
			}
			i->state = DNSSEC_ZONE_RR_ITER_REST;
			return i->name->nsec;

		case DNSSEC_ZONE_RR_ITER_NSEC3:
			nsec3 = i->nsec3_name->nsec;

			/* next nsec3 */
			do {
				i->nsec3_node
				    = ldns_rbtree_next(i->nsec3_node);
				i->nsec3_name
				    = i->nsec3_node == LDNS_RBTREE_NULL ? NULL
				    : (ldns_dnssec_name*)i->nsec3_node->data;

				/* names for glue can be in the hashed_names 
				 * tree, but will not have a NSEC3 
				 */
			} while (i->nsec3_name && !i->nsec3_name->nsec);

			dnssec_zone_rr_iter_set_state_for_next_name(i);
			return nsec3;

		case DNSSEC_ZONE_RR_ITER_FINI:
			return NULL;
		}
	}
}

static ldns_rr *
dnssec_zone_rr_iter_first(dnssec_zone_rr_iter *i, ldns_dnssec_zone *zone)
{
	if (!i || !zone)
		return NULL;

	memset(i, 0, sizeof(*i));
	i->zone = zone;
	if (zone->soa && zone->soa->name) {
		i->apex_name = zone->soa->name;
		i->apex_labs = ldns_dname_label_count(i->apex_name);
	} else
		i->apex_name = NULL;


	i->node = ldns_rbtree_first(zone->names);
	i->name = i->node == LDNS_RBTREE_NULL ? NULL
		: (ldns_dnssec_name *)i->node->data;

	if (zone->hashed_names) {
		do {
			i->nsec3_node = ldns_rbtree_first(zone->hashed_names);
			i->nsec3_name = i->nsec3_node == LDNS_RBTREE_NULL ?NULL
				      : (ldns_dnssec_name*)i->nsec3_node->data;
		} while (i->nsec3_name && !i->nsec3_name->nsec);
	}
	dnssec_zone_rr_iter_set_state_for_next_name(i);
	return dnssec_zone_rr_iter_next(i);
}

enum enum_zonemd_scheme {
        ZONEMD_SCHEME_FIRST  = 1,
        ZONEMD_SCHEME_SIMPLE = 1,
        ZONEMD_SCHEME_LAST   = 1
};
typedef enum enum_zonemd_scheme zonemd_scheme;

enum enum_zonemd_hash {
        ZONEMD_HASH_FIRST  = 1,
        ZONEMD_HASH_SHA384 = 1,
        ZONEMD_HASH_SHA512 = 2,
        ZONEMD_HASH_LAST   = 2
};
typedef enum enum_zonemd_hash zonemd_hash;

struct struct_zone_digester {
        ldns_sha384_CTX sha384_CTX;
        ldns_sha512_CTX sha512_CTX;
        unsigned simple_sha384 : 1;
        unsigned simple_sha512 : 1;
        unsigned double_sha384 : 1;
        unsigned double_sha512 : 1;
};
typedef struct struct_zone_digester zone_digester;

INLINE bool zone_digester_set(zone_digester *zd)
{ return zd && (zd->simple_sha384 || zd->simple_sha512); }

INLINE void zone_digester_init(zone_digester *zd)
{ memset(zd, 0, sizeof(*zd)); }

static ldns_status
zone_digester_add(zone_digester *zd, zonemd_scheme scheme, zonemd_hash hash)
{
	if (!zd)
		return LDNS_STATUS_NULL;

	switch (scheme) {
	case ZONEMD_SCHEME_SIMPLE:
		switch (hash) {
		case ZONEMD_HASH_SHA384:
			if (zd->double_sha384)
				return LDNS_STATUS_ZONEMD_DOUBLE_OCCURRENCE;

			else if (zd->simple_sha384) {
				zd->simple_sha384 = 0;
				zd->double_sha384 = 1;
				return LDNS_STATUS_ZONEMD_DOUBLE_OCCURRENCE;
			}
			ldns_sha384_init(&zd->sha384_CTX);
			zd->simple_sha384 = 1;
			break;

		case ZONEMD_HASH_SHA512:
			if (zd->double_sha512)
				return LDNS_STATUS_ZONEMD_DOUBLE_OCCURRENCE;

			else if (zd->simple_sha512) {
				zd->simple_sha512 = 0;
				zd->double_sha512 = 1;
				return LDNS_STATUS_ZONEMD_DOUBLE_OCCURRENCE;
			}
			ldns_sha512_init(&zd->sha512_CTX);
			zd->simple_sha512 = 1;
			break;
		default:
			return LDNS_STATUS_ZONEMD_UNKNOWN_HASH;
		}
		break;
	default:
		return LDNS_STATUS_ZONEMD_UNKNOWN_SCHEME;
	}
	return LDNS_STATUS_OK;
}

static ldns_status
zone_digester_update(zone_digester *zd, ldns_rr *rr)
{
	uint8_t data[65536];
	ldns_buffer buf;
	ldns_status st;
	
	buf._data = data;
	buf._position = 0;
	buf._limit = sizeof(data);
	buf._capacity = sizeof(data);
	buf._fixed = 1;
	buf._status = LDNS_STATUS_OK;

	if ((st = ldns_rr2buffer_wire_canonical(&buf, rr, LDNS_SECTION_ANSWER)))
		return st;

	if (zd->simple_sha384)
		ldns_sha384_update(&zd->sha384_CTX, data, buf._position);

	if (zd->simple_sha512)
		ldns_sha512_update(&zd->sha512_CTX, data, buf._position);

	return LDNS_STATUS_OK;
}

INLINE ldns_rr *
new_zonemd(ldns_rr *soa, zonemd_hash hash)
{
	ldns_rr  *rr     = NULL;
	uint8_t  *data   = NULL;
	ldns_rdf *rdf;
	size_t    md_len = hash == ZONEMD_HASH_SHA384
	                 ? LDNS_SHA384_DIGEST_LENGTH
	                 : LDNS_SHA512_DIGEST_LENGTH;

	if (!(rr = ldns_rr_new_frm_type(LDNS_RR_TYPE_ZONEMD)))
		return NULL;

	if (!(rdf = ldns_rdf_clone(ldns_rr_owner(soa))))
		goto error;

	ldns_rr_set_owner(rr, rdf);
	ldns_rr_set_class(rr, ldns_rr_get_class(soa));
	ldns_rr_set_ttl(rr, ldns_rr_ttl(soa));

	if (!(rdf = ldns_rdf_clone(ldns_rr_rdf(soa, 2))))
		goto error;
	ldns_rr_set_rdf(rr, rdf, 0);

	if (!(rdf = ldns_native2rdf_int8(LDNS_RDF_TYPE_INT8, 1)))
		goto error;
	ldns_rr_set_rdf(rr, rdf, 1);

	if (!(rdf = ldns_native2rdf_int8(LDNS_RDF_TYPE_INT8, hash)))
		goto error;
	ldns_rr_set_rdf(rr, rdf, 2);

	if (!(data = LDNS_XMALLOC(uint8_t, md_len)))
		goto error;

	if (!(rdf = ldns_rdf_new(LDNS_RDF_TYPE_HEX, md_len, data)))
		goto error;
	ldns_rr_set_rdf(rr, rdf, 3);

	return rr;
error:
	if (data)
		LDNS_FREE(data);
	ldns_rr_free(rr);
	return NULL;
}

static ldns_rr_list *
zone_digester_export(
		zone_digester *zd, ldns_rr *soa, ldns_status *ret_st)
{
	ldns_status st = LDNS_STATUS_OK;
	ldns_rr_list *rr_list = NULL;
	ldns_rr *sha384 = NULL;
	ldns_rr *sha512 = NULL;

	if (!zd || !soa)
		st = LDNS_STATUS_NULL;

	else if (ldns_rr_get_type(soa) != LDNS_RR_TYPE_SOA
	     ||  ldns_rr_rd_count(soa) < 3)
		st = LDNS_STATUS_ZONEMD_INVALID_SOA;

	else if (!(rr_list = ldns_rr_list_new()))
		st = LDNS_STATUS_MEM_ERR;

	else if (zd->simple_sha384
	     && !(sha384 = new_zonemd(soa, ZONEMD_HASH_SHA384)))
		st = LDNS_STATUS_MEM_ERR;

	else if (zd->simple_sha512
	     && !(sha512 = new_zonemd(soa, ZONEMD_HASH_SHA512)))
		st = LDNS_STATUS_MEM_ERR;

	else if (zd->simple_sha384
	     && !ldns_rr_list_push_rr(rr_list, sha384))
		st = LDNS_STATUS_MEM_ERR;

	else if (zd->simple_sha512
	     && !ldns_rr_list_push_rr(rr_list, sha512)) {
		if (zd->simple_sha384)
			sha384 = NULL; /* deleted by ldns_rr_list_deep_free */
		st = LDNS_STATUS_MEM_ERR;

	} else {
		if (sha384)
			ldns_sha384_final( ldns_rdf_data(ldns_rr_rdf(sha384,3))
			                 , &zd->sha384_CTX);
		if (sha512)
			ldns_sha512_final( ldns_rdf_data(ldns_rr_rdf(sha512,3))
			                 , &zd->sha512_CTX);
		return rr_list;
	}
	if (ret_st)
		*ret_st = st;
	if (sha384)
		ldns_rr_free(sha384);
	if (sha512)
		ldns_rr_free(sha512);
	if (rr_list)
		ldns_rr_list_deep_free(rr_list);
	return NULL;
}

static ldns_status
ldns_digest_zone(ldns_dnssec_zone *zone, zone_digester *zd)
{
	ldns_status st = LDNS_STATUS_OK;
	dnssec_zone_rr_iter rr_iter;
	ldns_rr *rr;
	ldns_rdf *apex_name; /* name of zone apex */

	if (!zone || !zd || !zone->soa || !zone->soa->name)
		return LDNS_STATUS_NULL;

	apex_name = zone->soa->name;
	for ( rr = dnssec_zone_rr_iter_first(&rr_iter, zone)
	    ; rr && !st
	    ; rr = dnssec_zone_rr_iter_next(&rr_iter)) {
		/* Skip apex ZONEMD RRs */
		if (ldns_rr_get_type(rr) == LDNS_RR_TYPE_ZONEMD
		&& !ldns_dname_compare(ldns_rr_owner(rr), apex_name))
			continue;
		/* Skip RRSIGs for apex ZONEMD RRs */
		if (ldns_rr_get_type(rr) == LDNS_RR_TYPE_RRSIG
		&&  LDNS_RR_TYPE_ZONEMD == ldns_rdf2rr_type(
				ldns_rr_rrsig_typecovered(rr))
		&& !ldns_dname_compare(ldns_rr_owner(rr), apex_name))
			continue;
		st = zone_digester_update(zd, rr);
	}
	return st;
}

ldns_status
ldns_dnssec_zone_verify_zonemd(ldns_dnssec_zone *zone)
{
	ldns_dnssec_rrsets *zonemd, *soa;
	zone_digester zd;
	ldns_dnssec_rrs *rrs;
	ldns_rr *soa_rr;
	ldns_status st;
	uint8_t simple_sha384[LDNS_SHA384_DIGEST_LENGTH];
	uint8_t simple_sha512[LDNS_SHA512_DIGEST_LENGTH];
	size_t valid_zonemds;

	if (!zone)
		return LDNS_STATUS_NULL;

	zonemd = ldns_dnssec_zone_find_rrset(
			zone, zone->soa->name, LDNS_RR_TYPE_ZONEMD);
	if (!zonemd) {
		ldns_rbnode_t *nsec3_node;

		/* we need proof of non-existence for ZONEMD at the apex */
		if (zone->soa->nsec) {
			if (ldns_nsec_bitmap_covers_type(ldns_nsec_get_bitmap(
							zone->soa->nsec),
					       	LDNS_RR_TYPE_ZONEMD))
				return LDNS_STATUS_NO_ZONEMD;

		} else if (!zone->soa->hashed_name || !zone->hashed_names)
			return LDNS_STATUS_NO_ZONEMD;

		else if (LDNS_RBTREE_NULL == 
		    (nsec3_node = ldns_rbtree_search( zone->hashed_names
						    , zone->soa->hashed_name)))
			return LDNS_STATUS_NO_ZONEMD;
		else {
			ldns_dnssec_name *nsec3
				= (ldns_dnssec_name *)nsec3_node->data;
			if (ldns_nsec_bitmap_covers_type(ldns_nsec_get_bitmap(
							nsec3->nsec),
						LDNS_RR_TYPE_ZONEMD))
				return LDNS_STATUS_NO_ZONEMD;
		}
		/* ZONEMD at apex does really not exist */
		return LDNS_STATUS_OK;
	}
	soa = ldns_dnssec_zone_find_rrset(
			zone, zone->soa->name, LDNS_RR_TYPE_SOA);
	if (!soa || !soa->rrs || !soa->rrs->rr)
		return LDNS_STATUS_ZONEMD_INVALID_SOA;

	soa_rr = soa->rrs->rr;
	if (ldns_rr_get_type(soa_rr) != LDNS_RR_TYPE_SOA
	||  ldns_rr_rd_count(soa_rr) < 3)
		return LDNS_STATUS_ZONEMD_INVALID_SOA;

	zone_digester_init(&zd);
	for (rrs = zonemd->rrs; rrs; rrs = rrs->next) {
		if (!rrs->rr
		||  ldns_rr_get_type(rrs->rr) != LDNS_RR_TYPE_ZONEMD
		||  ldns_rr_rd_count(rrs->rr) < 4)
			continue;

		/* serial should match SOA's serial */
		if (ldns_rdf2native_int32(ldns_rr_rdf(soa_rr, 2))
		    != ldns_rdf2native_int32(ldns_rr_rdf(rrs->rr, 0)))
			continue;
		
		/* Add (scheme, hash) to digester */
		zone_digester_add(&zd,
				ldns_rdf2native_int8(ldns_rr_rdf(rrs->rr, 1)),
				ldns_rdf2native_int8(ldns_rr_rdf(rrs->rr, 2)));
	}
	if (!zone_digester_set(&zd))
		return LDNS_STATUS_NO_VALID_ZONEMD;

	if ((st = ldns_digest_zone(zone, &zd)))
		return st;

	if (zd.simple_sha384)
		ldns_sha384_final(simple_sha384, &zd.sha384_CTX);
	if (zd.simple_sha512)
		ldns_sha512_final(simple_sha512, &zd.sha512_CTX);

	valid_zonemds = 0;
	for (rrs = zonemd->rrs; rrs; rrs = rrs->next) {
		if (!rrs->rr
		||  ldns_rr_get_type(rrs->rr) != LDNS_RR_TYPE_ZONEMD
		||  ldns_rr_rd_count(rrs->rr) < 4)
			continue;

		/* serial should match SOA's serial */
		if (ldns_rdf2native_int32(ldns_rr_rdf(soa_rr, 2))
		    != ldns_rdf2native_int32(ldns_rr_rdf(rrs->rr, 0)))
			continue;
		
		if (ZONEMD_SCHEME_SIMPLE !=
				ldns_rdf2native_int8(ldns_rr_rdf(rrs->rr, 1)))
			continue;

		if (ZONEMD_HASH_SHA384
				== ldns_rdf2native_int8(ldns_rr_rdf(rrs->rr,2))
		&&  LDNS_SHA384_DIGEST_LENGTH
				== ldns_rdf_size(ldns_rr_rdf(rrs->rr, 3))
		&&  memcmp( simple_sha384
			  , ldns_rdf_data(ldns_rr_rdf(rrs->rr, 3))
		          , LDNS_SHA384_DIGEST_LENGTH) == 0)

			valid_zonemds += 1;

		if (ZONEMD_HASH_SHA512
				== ldns_rdf2native_int8(ldns_rr_rdf(rrs->rr,2))
		&&  LDNS_SHA512_DIGEST_LENGTH
				== ldns_rdf_size(ldns_rr_rdf(rrs->rr, 3))
		&&  memcmp( simple_sha512
			  , ldns_rdf_data(ldns_rr_rdf(rrs->rr, 3))
		          , LDNS_SHA512_DIGEST_LENGTH) == 0)

			valid_zonemds += 1;
	}
	return valid_zonemds ? LDNS_STATUS_OK : LDNS_STATUS_NO_VALID_ZONEMD;
}

#ifdef HAVE_SSL
static ldns_status
rr_list2dnssec_rrs(ldns_rr_list *rr_list, ldns_dnssec_rrs **rrs,
		ldns_rr_list *new_rrs)
{
	ldns_rr *rr = NULL;

	if (!rr_list || !rrs)
		return LDNS_STATUS_NULL;

	if (ldns_rr_list_rr_count(rr_list) == 0)
		return LDNS_STATUS_OK;

	if (!*rrs) {
		if (!(*rrs = ldns_dnssec_rrs_new()))
			return LDNS_STATUS_MEM_ERR;
		(*rrs)->rr = ldns_rr_list_pop_rr(rr_list);
		if (new_rrs)
			ldns_rr_list_push_rr(new_rrs, (*rrs)->rr);
	}
	while ((rr = ldns_rr_list_pop_rr(rr_list))) {
		ldns_status st;
	       
		if ((st = ldns_dnssec_rrs_add_rr(*rrs, rr))) {
			ldns_rr_list_push_rr(rr_list, rr);
			return st;
		} else if (new_rrs)
			ldns_rr_list_push_rr(new_rrs, rr);
	}
	return LDNS_STATUS_OK;
}


ldns_status
dnssec_zone_equip_zonemd(ldns_dnssec_zone *zone,
		ldns_rr_list *new_rrs, ldns_key_list *key_list, int signflags)
{
	ldns_status st = LDNS_STATUS_OK;
	zone_digester zd;
	ldns_rr_list *zonemd_rr_list = NULL;
	ldns_rr_list *zonemd_rrsigs = NULL;
	ldns_dnssec_rrsets *soa_rrset;
	ldns_rr *soa_rr = NULL;
	ldns_dnssec_rrsets **rrset_ref;
	ldns_dnssec_rrsets *zonemd_rrset;

	zone_digester_init(&zd);
	if (signflags & LDNS_SIGN_WITH_ZONEMD_SIMPLE_SHA384)
		zone_digester_add(&zd, ZONEMD_SCHEME_SIMPLE
		                     , ZONEMD_HASH_SHA384);

	if (signflags & LDNS_SIGN_WITH_ZONEMD_SIMPLE_SHA512)
		zone_digester_add(&zd, ZONEMD_SCHEME_SIMPLE
		                     , ZONEMD_HASH_SHA512);

	if ((st = ldns_digest_zone(zone, &zd)))
		return st;

	soa_rrset = ldns_dnssec_zone_find_rrset(
			zone, zone->soa->name, LDNS_RR_TYPE_SOA);
	if (!soa_rrset || !soa_rrset->rrs || !soa_rrset->rrs->rr)
		return LDNS_STATUS_ZONEMD_INVALID_SOA;
	soa_rr = soa_rrset->rrs->rr;

	if (!(zonemd_rr_list = zone_digester_export(&zd, soa_rr, &st)))
		return st;
	
	/* - replace or add ZONEMD rrset */
	rrset_ref = &zone->soa->rrsets; /* scan rrsets at apex */
	while (*rrset_ref && (*rrset_ref)->type < LDNS_RR_TYPE_ZONEMD)
		rrset_ref = &(*rrset_ref)->next;
	if (*rrset_ref && (*rrset_ref)->type == LDNS_RR_TYPE_ZONEMD) {
		/* reuse zonemd rrset */
		zonemd_rrset = *rrset_ref;
		ldns_dnssec_rrs_free(zonemd_rrset->rrs);
		zonemd_rrset->rrs = NULL;
		ldns_dnssec_rrs_free(zonemd_rrset->signatures);
		zonemd_rrset->signatures = NULL;
	} else {
		/* insert zonemd rrset */
		zonemd_rrset = ldns_dnssec_rrsets_new();
		if (!zonemd_rrset) {
			ldns_rr_list_deep_free(zonemd_rr_list);
			return LDNS_STATUS_MEM_ERR;
		}
		zonemd_rrset->type = LDNS_RR_TYPE_ZONEMD;
		zonemd_rrset->next = *rrset_ref;
		*rrset_ref = zonemd_rrset;
	}
	if ((zonemd_rrsigs = ldns_sign_public(zonemd_rr_list, key_list)))
		st = rr_list2dnssec_rrs(  zonemd_rrsigs
		                       , &zonemd_rrset->signatures, new_rrs);
	if (!st)
		st = rr_list2dnssec_rrs(  zonemd_rr_list
		                       , &zonemd_rrset->rrs, new_rrs);
	ldns_rr_list_deep_free(zonemd_rr_list);
	ldns_rr_list_deep_free(zonemd_rrsigs);
	return st;
}

#endif /* HAVE_SSL */

