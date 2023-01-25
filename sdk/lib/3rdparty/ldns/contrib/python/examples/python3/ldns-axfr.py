#!/usr/bin/python
# vim:fileencoding=utf-8
#
# AXFR client with IDN (Internationalized Domain Names) support
#

import ldns
import encodings.idna

def utf2name(name):
    return '.'.join([encodings.idna.ToASCII(a).decode("utf-8") for a in name.split('.')])
def name2utf(name):
    return '.'.join([encodings.idna.ToUnicode(a) for a in name.split('.')])


resolver = ldns.ldns_resolver.new_frm_file("/etc/resolv.conf")

#addr = ldns.ldns_get_rr_list_addr_by_name(resolver, "zone.nic.cz", ldns.LDNS_RR_CLASS_IN, ldns.LDNS_RD);
addr = resolver.get_addr_by_name("zone.nic.cz", ldns.LDNS_RR_CLASS_IN, ldns.LDNS_RD);
if (not addr):
    raise Exception("Can't retrieve server address")

print("Addr_by_name:",str(addr).replace("\n","; "))

#remove all nameservers
while resolver.pop_nameserver(): 
    pass

#insert server addr
for rr in addr.rrs():
    resolver.push_nameserver_rr(rr)

#AXFR transfer
status = resolver.axfr_start(utf2name("háčkyčárky.cz"), ldns.LDNS_RR_CLASS_IN)
if status != ldns.LDNS_STATUS_OK:
    raise Exception("Can't start AXFR. Error: %s" % ldns.ldns_get_errorstr_by_id(status))

#Print results
while True:
    rr = resolver.axfr_next()
    if not rr: 
        break

    rdf = rr.owner()
    if (rdf.get_type() == ldns.LDNS_RDF_TYPE_DNAME):
        print("RDF owner: type=",rdf.get_type_str(),"data=",name2utf(str(rdf)))
    else:
        print("RDF owner: type=",rdf.get_type_str(),"data=",str(rdf))
    print("   RR type=", rr.get_type_str()," ttl=",rr.ttl())
    for rdf in rr.rdfs():
        if (rdf.get_type() == ldns.LDNS_RDF_TYPE_DNAME):
            print("   RDF: type=",rdf.get_type_str(),"data=",name2utf(str(rdf)))
        else:
            print("   RDF: type=",rdf.get_type_str(),"data=",str(rdf))

    print()
