#!/usr/bin/python

import ldns

pkt = ldns.ldns_pkt.new_query_frm_str("www.google.com",ldns.LDNS_RR_TYPE_ANY, ldns.LDNS_RR_CLASS_IN, ldns.LDNS_QR | ldns.LDNS_AA)

rra = ldns.ldns_rr.new_frm_str("www.google.com. IN A 192.168.1.1",300)
rrb = ldns.ldns_rr.new_frm_str("www.google.com. IN TXT Some\ Description",300)

list = ldns.ldns_rr_list()
if (rra): list.push_rr(rra)
if (rrb): list.push_rr(rrb)

pkt.push_rr_list(ldns.LDNS_SECTION_ANSWER, list)

print "Packet:"
print pkt
