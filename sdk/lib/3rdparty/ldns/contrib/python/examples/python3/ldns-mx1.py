#!/usr/bin/python
#
# MX is a small program that prints out the mx records for a particular domain
#
import ldns

dname = ldns.ldns_dname("nic.cz")
print(dname)

resolver = ldns.ldns_resolver.new_frm_file("/etc/resolv.conf")

pkt = resolver.query(dname, ldns.LDNS_RR_TYPE_MX,ldns.LDNS_RR_CLASS_IN)

if (pkt):
    mx = pkt.rr_list_by_type(ldns.LDNS_RR_TYPE_MX, ldns.LDNS_SECTION_ANSWER)
    if (mx):
       mx.sort()
       print(mx)
