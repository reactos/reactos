#!/usr/bin/python
#
# MX is a small program that prints out the mx records for a particular domain
#
import ldns

resolver = ldns.ldns_resolver.new_frm_file("/etc/resolv.conf")

pkt = resolver.query("nic.cz", ldns.LDNS_RR_TYPE_MX,ldns.LDNS_RR_CLASS_IN)
if (pkt) and (pkt.answer()):

    for rr in pkt.answer().rrs():
        if (rr.get_type() != ldns.LDNS_RR_TYPE_MX):
            continue

        rdf = rr.owner()
        print(rdf," ",rr.ttl()," ",rr.get_class_str()," ",rr.get_type_str()," ", end=" ")
        print(" ".join(str(rdf) for rdf in rr.rdfs()))

