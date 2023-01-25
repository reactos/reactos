#!/usr/bin/python
#
# MX is a small program that prints out the mx records for a particular domain
#
import ldnsx

resolver = ldnsx.resolver()

pkt = resolver.query("nic.cz", "MX")
if pkt:
    for rr in pkt.answer(rr_type = "MX"):
        rdf = rr.owner()
        print rr
		#Could also do:
		#print rr[0], rr[1], rr[2], rr[3], " ".join(rr[4:])
		#print rr.owner(), rr.ttl(), rr.rr_clas(), rr.rr_type(), " ".join(rr[4:])

