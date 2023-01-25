#!/usr/bin/python
# This example shows how to sign a given zone file with private key

import ldns
import sys, os, time

#private key TAG which identifies the private key 
#use ldns-keygen.py in order to obtain private key
keytag = 30761

# Read zone file
#-------------------------------------------------------------

zone = ldns.ldns_zone.new_frm_fp(open("zone.txt","r"), None, 0, ldns.LDNS_RR_CLASS_IN)
soa = zone.soa()
origin = soa.owner()

# Prepare keys
#-------------------------------------------------------------

#Read private key from file
keyfile = open("key-%s-%d.private" % (origin, keytag), "r");
key = ldns.ldns_key.new_frm_fp(keyfile)

#Read public key from file
pubfname = "key-%s-%d.key" % (origin, keytag)
pubkey = None
if os.path.isfile(pubfname):
   pubkeyfile = open(pubfname, "r");
   pubkey,_,_,_ = ldns.ldns_rr.new_frm_fp(pubkeyfile)

if not pubkey:
   #Create new public key
   pubkey = key.key_to_rr()

#Set key expiration
key.set_expiration(int(time.time()) + 365*60*60*24) #365 days

#Set key owner (important step)
key.set_pubkey_owner(origin)

#Insert DNSKEY RR
zone.push_rr(pubkey)

# Sign zone
#-------------------------------------------------------------

#Create keylist and push private key
keys = ldns.ldns_key_list()
keys.push_key(key)

#Add SOA
signed_zone = ldns.ldns_dnssec_zone()
signed_zone.add_rr(soa)

#Add RRs
for rr in zone.rrs().rrs():
   print "RR:",str(rr),
   signed_zone.add_rr(rr)

added_rrs = ldns.ldns_rr_list()
status = signed_zone.sign(added_rrs, keys)
if (status == ldns.LDNS_STATUS_OK):
   signed_zone.print_to_file(open("zone_signed.txt","w"))

