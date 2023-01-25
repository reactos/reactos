#!/usr/bin/python
# vim:fileencoding=utf-8
#
# AXFR client with IDN (Internationalized Domain Names) support
#

import ldns
import encodings.idna

def utf2name(name):
    return '.'.join([encodings.idna.ToASCII(a) for a in name.split('.')])
def name2utf(name):
    return '.'.join([encodings.idna.ToUnicode(a) for a in name.split('.')])

resolver = ldnsx.resolver("zone.nic.cz")

#Print results
for rr in resolver.AXFR(utf2name(u"háčkyčárky.cz")):
   # rdf = rr.owner()
   # if (rdf.get_type() == ldns.LDNS_RDF_TYPE_DNAME):
   #     print "RDF owner: type=",rr.type(),"data=",name2utf(rr.owner())
   # else:
   #     print "RDF owner: type=",rdf.get_type_str(),"data=",str(rdf)
   # print "   RR type=", rr.get_type_str()," ttl=",rr.ttl()
   # for rdf in rr.rdfs():
   #     if (rdf.get_type() == ldns.LDNS_RDF_TYPE_DNAME):
   #         print "   RDF: type=",rdf.get_type_str(),"data=",name2utf(str(rdf))
   #     else:
   #         print "   RDF: type=",rdf.get_type_str(),"data=",str(rdf)

