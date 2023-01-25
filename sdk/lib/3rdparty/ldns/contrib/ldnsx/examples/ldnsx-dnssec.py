#!/usr/bin/python
# -*- coding: utf-8 -*-
import ldnsx
import sys

debug = True

if len(sys.argv) < 2:
   print "Usage:", sys.argv[0], "domain [resolver_addr]"
   sys.exit(1)

name = sys.argv[1]

# Create resolver
resolver = ldnsx.resolver(dnssec=True)

# Custom resolver
if len(sys.argv) > 2:
   # Clear previous nameservers
   resolver.set_nameservers(sys.argv[2:])

# Resolve DNS name
pkt = resolver.query(name, "A")

if pkt and pkt.answer():

   # Debug
   if debug:
      print "NS returned:", pkt.rcode(), "(AA: %d AD: %d)" % ( "AA" in pkt.flags(), "AD" in pkt.flags() )

   # SERVFAIL indicated bogus name
   if pkt.rcode() == "SERVFAIL":
      print name, "failed to resolve"

   # Check AD (Authenticated) bit
   if pkt.rcode() == "NOERROR":
      if "AD" in pkt.flags(): print name, "is secure"
      else:                   print name, "is insecure"

