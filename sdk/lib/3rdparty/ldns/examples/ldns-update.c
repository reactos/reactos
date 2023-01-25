/* $Id: ldns-update.c,v 1.1 2005/09/13 09:37:05 ho Exp $ */
/*
 * Example of the update functionality
 *
 * See the file LICENSE for the license
 */


#include "config.h"

#include <strings.h>
#include <ldns/ldns.h>

/* dynamic update stuff */
static ldns_resolver *
ldns_update_resolver_new(const char *fqdn, const char *zone,
    ldns_rr_class class, uint16_t port, ldns_tsig_credentials *tsig_cred, ldns_rdf **zone_rdf)
{
        ldns_resolver   *r1, *r2;
        ldns_pkt        *query = NULL, *resp = NULL;
        ldns_rr_list    *nslist, *iplist;
        ldns_rdf        *soa_zone, *soa_mname = NULL, *ns_name;
        size_t          i;
        ldns_status     s;

        if (class == 0) {
                class = LDNS_RR_CLASS_IN;
        }

        if (port == 0) {
                port = LDNS_PORT;
        }

        /* First, get data from /etc/resolv.conf */
        s = ldns_resolver_new_frm_file(&r1, NULL);
        if (s != LDNS_STATUS_OK) {
                return NULL;
        }

        r2 = ldns_resolver_new();
        if (!r2) {
                goto bad;
        }
        ldns_resolver_set_port(r2, port);

        /* TSIG key data available? Copy into the resolver. */
        if (tsig_cred) {
                ldns_resolver_set_tsig_algorithm(r2, ldns_tsig_algorithm(tsig_cred));
                ldns_resolver_set_tsig_keyname(r2, ldns_tsig_keyname(tsig_cred));
                ldns_resolver_set_tsig_keydata(r2, ldns_tsig_keydata(tsig_cred));
        }

        /* Now get SOA zone, mname, NS, and construct r2. [RFC2136 4.3] */

        /* Explicit 'zone' or no? */
        if (zone) {
                soa_zone = ldns_dname_new_frm_str(zone);
                if (ldns_update_soa_mname(soa_zone, r1, class, &soa_mname)
                    != LDNS_STATUS_OK) {
                        goto bad;
		}
        } else {
                if (ldns_update_soa_zone_mname(fqdn, r1, class, &soa_zone,
                        &soa_mname) != LDNS_STATUS_OK) {
                        goto bad;
		}
        }

        /* Pass zone_rdf on upwards. */
        *zone_rdf = ldns_rdf_clone(soa_zone);

        /* NS */
        query = ldns_pkt_query_new(soa_zone, LDNS_RR_TYPE_NS, class, LDNS_RD);
        if (!query) {
                goto bad;
	}
        soa_zone = NULL;

        ldns_pkt_set_random_id(query);

        if (ldns_resolver_send_pkt(&resp, r1, query) != LDNS_STATUS_OK) {
                dprintf("%s", "NS query failed!\n");
                goto bad;
        }
        ldns_pkt_free(query);
        if (!resp) {
                goto bad;
	}
        /* Match SOA MNAME to NS list, adding it first */
        nslist = ldns_pkt_answer(resp);
        for (i = 0; i < ldns_rr_list_rr_count(nslist); i++) {
                ns_name = ldns_rr_rdf(ldns_rr_list_rr(nslist, i), 0);
                if (!ns_name)
                        continue;
                if (ldns_rdf_compare(soa_mname, ns_name) == 0) {
                        /* Match */
                        iplist = ldns_get_rr_list_addr_by_name(r1, ns_name, class, 0);
                        (void) ldns_resolver_push_nameserver_rr_list(r2, iplist);
			ldns_rr_list_deep_free(iplist);
                        break;
                }
        }

        /* Then all the other NSs. XXX Randomize? */
        for (i = 0; i < ldns_rr_list_rr_count(nslist); i++) {
                ns_name = ldns_rr_rdf(ldns_rr_list_rr(nslist, i), 0);
                if (!ns_name)
                        continue;
                if (ldns_rdf_compare(soa_mname, ns_name) != 0) {
                        /* No match, add it now. */
                        iplist = ldns_get_rr_list_addr_by_name(r1, ns_name, class, 0);
                        (void) ldns_resolver_push_nameserver_rr_list(r2, iplist);
			ldns_rr_list_deep_free(iplist);
                }
        }

        ldns_resolver_set_random(r2, false);
        ldns_pkt_free(resp);
        ldns_resolver_deep_free(r1);
	if (soa_mname)
		ldns_rdf_deep_free(soa_mname);
        return r2;

  bad:
        if (r1)
                ldns_resolver_deep_free(r1);
        if (r2)
                ldns_resolver_deep_free(r2);
        if (query)
                ldns_pkt_free(query);
        if (resp)
                ldns_pkt_free(resp);
	if (soa_mname)
		ldns_rdf_deep_free(soa_mname);
        return NULL;
}


static ldns_status
ldns_update_send_simple_addr(const char *fqdn, const char *zone,
    const char *ipaddr, uint16_t p, uint32_t ttl, ldns_tsig_credentials *tsig_cred)
{
        ldns_resolver   *res;
        ldns_pkt        *u_pkt = NULL, *r_pkt;
        ldns_rr_list    *up_rrlist;
        ldns_rr         *up_rr;
        ldns_rdf        *zone_rdf = NULL;
        char            *rrstr;
        uint32_t        rrstrlen, status = LDNS_STATUS_OK;

        if (!fqdn || strlen(fqdn) == 0)
                return LDNS_STATUS_ERR;

        /* Create resolver */
        res = ldns_update_resolver_new(fqdn, zone, 0, p, tsig_cred, &zone_rdf);
        if (!res || !zone_rdf) {
                goto cleanup;
	}

        /* Set up the update section. */
        up_rrlist = ldns_rr_list_new();
        if (!up_rrlist) {
                goto cleanup;
	}
        /* Create input for ldns_rr_new_frm_str() */
        if (ipaddr) {
                /* We're adding A or AAAA */
                rrstrlen = strlen(fqdn) + sizeof (" IN AAAA ") + strlen(ipaddr) + 1;
                rrstr = (char *)malloc(rrstrlen);
                if (!rrstr) {
                        ldns_rr_list_deep_free(up_rrlist);
                        goto cleanup;
                }
                snprintf(rrstr, rrstrlen, "%s IN %s %s", fqdn,
                    strchr(ipaddr, ':') ? "AAAA" : "A", ipaddr);

                if (ldns_rr_new_frm_str(&up_rr, rrstr, ttl, NULL, NULL) !=
                                LDNS_STATUS_OK) {
                        ldns_rr_list_deep_free(up_rrlist);
                        free(rrstr);
                        goto cleanup;
                }
                free(rrstr);
                ldns_rr_list_push_rr(up_rrlist, up_rr);
        } else {
                /* We're removing A and/or AAAA from 'fqdn'. [RFC2136 2.5.2] */
                up_rr = ldns_rr_new();
                ldns_rr_set_owner(up_rr, ldns_dname_new_frm_str(fqdn));
                ldns_rr_set_ttl(up_rr, 0);
                ldns_rr_set_class(up_rr, LDNS_RR_CLASS_ANY);

                ldns_rr_set_type(up_rr, LDNS_RR_TYPE_A);
                ldns_rr_list_push_rr(up_rrlist, ldns_rr_clone(up_rr));

                ldns_rr_set_type(up_rr, LDNS_RR_TYPE_AAAA);
                ldns_rr_list_push_rr(up_rrlist, up_rr);
        }

        /* Create update packet. */
        u_pkt = ldns_update_pkt_new(zone_rdf, LDNS_RR_CLASS_IN, NULL, up_rrlist, NULL);
        zone_rdf = NULL;
        if (!u_pkt) {
                ldns_rr_list_deep_free(up_rrlist);
                goto cleanup;
        }
        ldns_pkt_set_random_id(u_pkt);

        /* Add TSIG */
        if (tsig_cred)
                if (ldns_update_pkt_tsig_add(u_pkt, res) != LDNS_STATUS_OK) {
                        goto cleanup;
		}

        if (ldns_resolver_send_pkt(&r_pkt, res, u_pkt) != LDNS_STATUS_OK) {
                goto cleanup;
	}
        ldns_pkt_free(u_pkt);
        if (!r_pkt) {
                goto cleanup;
	}
        if (ldns_pkt_get_rcode(r_pkt) != LDNS_RCODE_NOERROR) {
                ldns_lookup_table *t = ldns_lookup_by_id(ldns_rcodes,
                                (int)ldns_pkt_get_rcode(r_pkt));
                if (t) {
                        dprintf(";; UPDATE response was %s\n", t->name);
                } else {
                        dprintf(";; UPDATE response was (%d)\n", ldns_pkt_get_rcode(r_pkt));
                }
                status = LDNS_STATUS_ERR;
        }
        ldns_pkt_free(r_pkt);
        ldns_resolver_deep_free(res);
        return status;

  cleanup:
        if (res)
                ldns_resolver_deep_free(res);
        if (u_pkt)
                ldns_pkt_free(u_pkt);
	if (zone_rdf)
		ldns_rdf_deep_free(zone_rdf);
        return LDNS_STATUS_ERR;
}


static void
usage(FILE *fp, char *prog)
{
        fprintf(fp, "%s domain [zone] ip tsig_name tsig_alg tsig_hmac\n", prog);
        fprintf(fp, "  send a dynamic update packet to <ip>\n\n");
        fprintf(fp, "  Use 'none' instead of ip to remove any previous address\n");
        fprintf(fp, "  If 'zone'  is not specified, try to figure it out from the zone's SOA\n");
        fprintf(fp, "  Example: %s my.example.org 1.2.3.4\n", prog);
}


int
main(int argc, char **argv)
{
	char		*fqdn, *ipaddr, *zone, *prog;
	ldns_status	ret;
	ldns_tsig_credentials	tsig_cr, *tsig_cred;
	int		c = 2;
	uint32_t	defttl = 300;
	uint32_t 	port = 53;
	
	prog = _strdup(argv[0]);

	switch (argc) {
	case 3:
	case 4:
	case 6:
	case 7:
		break;
	default:
		usage(stderr, prog);
		exit(EXIT_FAILURE);
	}

	fqdn = argv[1]; 
	c = 2;
	if (argc == 4 || argc == 7) {
		zone = argv[c++];
	} else {
		zone = NULL;
	}
	
	if (strcmp(argv[c], "none") == 0) {
		ipaddr = NULL;
	} else {
		ipaddr = argv[c];
	}
	c++;
	if (argc == 6 || argc == 7) {
		tsig_cr.keyname = argv[c++];
		if (strncasecmp(argv[c], "hmac-sha1", 9) == 0) {
			tsig_cr.algorithm = (char*)"hmac-sha1.";
		} else if (strncasecmp(argv[c], "hmac-md5", 8) == 0) {
			tsig_cr.algorithm = (char*)"hmac-md5.sig-alg.reg.int.";
		} else {
			fprintf(stderr, "Unknown algorithm, try \"hmac-md5\" "
			    "or \"hmac-sha1\".\n");
			exit(EXIT_FAILURE);
		}
		tsig_cr.keydata = argv[++c];
		tsig_cred = &tsig_cr;
	} else {
		tsig_cred = NULL;
	}

	printf(";; trying UPDATE with FQDN \"%s\" and IP \"%s\"\n",
	    fqdn, ipaddr ? ipaddr : "<none>");
	if (argc == 6 || argc == 7) {
		printf(";; tsig: \"%s\" \"%s\" \"%s\"\n", tsig_cr.keyname,
			tsig_cr.algorithm, tsig_cr.keydata);
	}

	ret = ldns_update_send_simple_addr(fqdn, zone, ipaddr, port, defttl, tsig_cred);
	exit(ret);
}
