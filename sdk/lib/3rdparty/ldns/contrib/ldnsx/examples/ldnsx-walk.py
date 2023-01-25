#!/usr/bin/python
# vim:fileencoding=utf-8
#
# Walk a domain that's using NSEC and print in zonefile format.

import sys
import ldnsx

def walk(domain):
        res = ldnsx.resolver("193.110.157.136", dnssec=True)
        pkt = res.query(domain, 666)
        try:
                nsec_rr = pkt.authority(rr_type="NSEC")[0]
        except:
                print "no NSEC found, domain is not signed or using NSEC3"
                sys.exit()
        for rr_type in nsec_rr[5].split(' ')[:-1]:
                for rr in ldnsx.get_rrs(domain, rr_type):
                        print str(rr)[:-1]
        next_rec = nsec_rr[4]
        if (next_rec != domain) and (next_rec[-len(domain):] == domain):
                walk(next_rec)

walk("xelerance.com")

