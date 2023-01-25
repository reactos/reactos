#!/usr/bin/python
# -*- coding: utf-8 -*-
import ldns
import sys

debug = True

# Check args
argc = len(sys.argv)
name = "www.nic.cz"
if argc < 2:
   print "Usage:", sys.argv[0], "domain [resolver_addr]"
   sys.exit(1)
else:
   name = sys.argv[1]

# Create resolver
resolver = ldns.ldns_resolver.new_frm_file("/etc/resolv.conf")
resolver.set_dnssec(True)

# Custom resolver
if argc > 2:
   # Clear previous nameservers
   ns = resolver.pop_nameserver()
   while ns != None:
      ns = resolver.pop_nameserver()
   ip = ldns.ldns_rdf.new_frm_str(sys.argv[2], ldns.LDNS_RDF_TYPE_A)
   resolver.push_nameserver(ip)

# Resolve DNS name
pkt = resolver.query(name, ldns.LDNS_RR_TYPE_A, ldns.LDNS_RR_CLASS_IN)
if pkt and pkt.answer():

   # Debug
   if debug:
      print "NS returned:", pkt.get_rcode(), "(AA: %d AD: %d)" % ( pkt.ad(), pkt.ad() )

   # SERVFAIL indicated bogus name
   if pkt.get_rcode() is ldns.LDNS_RCODE_SERVFAIL:
      print name, "is bogus"

   # Check AD (Authenticated) bit
   if pkt.get_rcode() is ldns.LDNS_RCODE_NOERROR:
      if pkt.ad(): print name, "is secure"
      else:        print name, "is insecure"
