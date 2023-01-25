#!/usr/bin/python
import ldns

#Read zone from file
zone = ldns.ldns_zone.new_frm_fp(open("zone.txt","r"), None, 0, ldns.LDNS_RR_CLASS_IN)
print zone

print "SOA:", zone.soa()
for r in zone.rrs().rrs():
   print "RR:", r


zone = ldns.ldns_zone()
#print zone

