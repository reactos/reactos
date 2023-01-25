#!/usr/bin/python
#
# This example shows how to generate public/private key pair
#
import ldns

algorithm = ldns.LDNS_SIGN_DSA
bits = 512

ldns.ldns_init_random(open("/dev/random","rb"), (bits+7)//8)

domain = ldns.ldns_dname("example.")

#generate a new key
key = ldns.ldns_key.new_frm_algorithm(algorithm, bits);
print(key)

#set owner
key.set_pubkey_owner(domain)

#create the public from the ldns_key
pubkey = key.key_to_rr()
#previous command is equivalent to
# pubkey = ldns.ldns_key2rr(key)
print(pubkey)

#calculate and set the keytag
key.set_keytag(ldns.ldns_calc_keytag(pubkey))

#build the DS record
ds = ldns.ldns_key_rr2ds(pubkey, ldns.LDNS_SHA1)
print(ds)

owner, tag = pubkey.owner(), key.keytag()

#write public key to .key file
fw = open("key-%s-%d.key" % (owner,tag), "wb")
pubkey.print_to_file(fw)

#write private key to .priv file
fw = open("key-%s-%d.private" % (owner,tag), "wb")
key.print_to_file(fw)

#write DS to .ds file
fw = open("key-%s-%d.ds" % (owner,tag), "wb")
ds.print_to_file(fw)
