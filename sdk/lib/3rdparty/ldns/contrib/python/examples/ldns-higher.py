#!/usr/bin/python
import ldns

resolver = ldns.ldns_resolver.new_frm_file("/etc/resolv.conf")

dnn = ldns.ldns_dname("www.google.com")
print dnn.get_type_str(), dnn

dna = ldns.ldns_rdf.new_frm_str("74.125.43.99",ldns.LDNS_RDF_TYPE_A)
print dna.get_type_str(), dna

name = resolver.get_name_by_addr(dna)
if (not name): raise Exception("Can't retrieve server name")
for rr in name.rrs():
    print rr

name = resolver.get_name_by_addr("74.125.43.99")
if (not name): raise Exception("Can't retrieve server name")
for rr in name.rrs():
    print rr

addr = resolver.get_addr_by_name(dnn)
if (not addr): raise Exception("Can't retrieve server address")
for rr in addr.rrs():
    print rr

addr = resolver.get_addr_by_name("www.google.com")
if (not addr): raise Exception("Can't retrieve server address")
for rr in addr.rrs():
    print rr

hosts = ldns.ldns_rr_list.new_frm_file("/etc/hosts")
if (not hosts): raise Exception("Can't retrieve the content of file")
for rr in hosts.rrs():
    print rr

