#!/usr/bin/env python

#
# ldns_rr and ldns_rr_list testing script.
#
# Do not use constructs that differ between Python 2 and 3.
# Use write on stdout or stderr.
#


import ldns
import sys
import os
import inspect


class_name = "ldns_rr"
method_name = None
error_detected = False
temp_fname = "tmp_rr.txt"


def set_error():
    """
        Writes an error message and sets error flag.
    """
    global class_name
    global method_name
    global error_detected
    error_detected = True
    sys.stderr.write("(line %d): malfunctioning method %s.\n" % \
       (inspect.currentframe().f_back.f_lineno, method_name))



#if not error_detected:
if True:
    method_name = class_name + ".[comparison operators]"
    rr1 = ldns.ldns_rr.new_frm_str("test1 600 IN A 0.0.0.0")
    rr2 = ldns.ldns_rr.new_frm_str("test2 600 IN A 1.1.1.1")
    try:
        ret = rr1 < rr2
        if not isinstance(ret, bool):
            set_error()
        if ret != True:
            set_error()
    except:
        set_error()
    try:
        ret = rr2 < rr1
        if not isinstance(ret, bool):
            set_error()
        if ret != False:
            set_error()
    except:
        set_error()
    try:
        ret = rr1 <= rr2
        if not isinstance(ret, bool):
            set_error()
        if ret != True:
            set_error()
    except:
        set_error()
    try:
        ret = rr2 <= rr1
        if not isinstance(ret, bool):
            set_error()
        if ret != False:
            set_error()
    except:
        set_error()
    try:
        ret = rr1 == rr2
        if not isinstance(ret, bool):
            set_error()
        if ret != False:
            set_error()
    except:
        set_error()
    try:
        ret = rr1 == rr1
        if not isinstance(ret, bool):
            set_error()
        if ret != True:
            set_error()
    except:
        set_error()
    try:
        ret = rr1 != rr2
        if not isinstance(ret, bool):
            set_error()
        if ret != True:
            set_error()
    except:
        set_error()
    try:
        ret = rr1 != rr1
        if not isinstance(ret, bool):
            set_error()
        if ret != False:
            set_error()
    except:
        set_error()
    try:
        ret = rr1 > rr2
        if not isinstance(ret, bool):
            set_error()
        if ret != False:
            set_error()
    except:
        set_error()
    try:
        ret = rr2 > rr1
        if not isinstance(ret, bool):
            set_error()
        if ret != True:
            set_error()
    except:
        set_error()
    try:
        ret = rr1 >= rr2
        if not isinstance(ret, bool):
            set_error()
        if ret != False:
            set_error()
    except:
        set_error()
    try:
        ret = rr2 >= rr1
        if not isinstance(ret, bool):
            set_error()
        if ret != True:
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + "__init__()"
    try:
        rr = ldns.ldns_rr()
        set_error()
    except Exception:
        pass
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".a_address()"
    rr = ldns.ldns_rr.new_frm_str("www.nic.cz 600 IN A 217.31.205.50")
    try:
        address = rr.a_address()
        if not isinstance(address, ldns.ldns_rdf):
            set_error()
        if address == None:
            set_error()
    except:
        set_error()
    rr = ldns.ldns_rr.new_frm_str("www.nic.cz 600 IN AAAA 2002:d91f:cd32::1")
    try:
        address = rr.a_address()
        if not isinstance(address, ldns.ldns_rdf):
            set_error()
        if address == None:
            set_error()
    except:
        set_error()
    rr = ldns.ldns_rr.new_frm_str("www.nic.cz 600 IN TXT text")
    try:
        address = rr.a_address()
        if isinstance(address, ldns.ldns_rdf):
            set_error()
        if address != None:
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".a_set_address()"
    rdf = ldns.ldns_rdf.new_frm_str("127.0.0.1", ldns.LDNS_RDF_TYPE_A)
    rr = ldns.ldns_rr.new_frm_str("test 600 IN A 0.0.0.0")
    try:
        ret = rr.a_set_address(rdf)
        if not isinstance(ret, bool):
            set_error()
        if ret != True:
            set_error()
    except:
        set_error()
    rdf = ldns.ldns_rdf.new_frm_str("::1", ldns.LDNS_RDF_TYPE_AAAA)
    rr = ldns.ldns_rr.new_frm_str("test 600 IN AAAA ::")
    try:
        ret = rr.a_set_address(rdf)
        if not isinstance(ret, bool):
            set_error()
        if ret != True:
            set_error()
    except:
        set_error()
    try:
        ret = rr.a_set_address("")
        set_error()
    except TypeError:
        pass
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".class_by_name()"
    try:
        ret = ldns.ldns_rr.class_by_name("IN")
        if not isinstance(ret, int):
            set_error()
        if ret != ldns.LDNS_RR_CLASS_IN:
            set_error()
    except:
        set_error()
    method_name = class_name + ".class_by_name()"
    try:
        ret = ldns.ldns_rr.class_by_name("AA")
        if not isinstance(ret, int):
            set_error()
        if ret != 0:
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".clone()"
    rr = ldns.ldns_rr.new_frm_str("test 600 IN TXT text")
    try:
        ret = rr.clone()
        if not isinstance(ret, ldns.ldns_rr):
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".compare_ds()"
    pubkey1 = ldns.ldns_rr.new_frm_str("example1. 3600 IN DNSKEY 256 3 3 APw7tG8Nf7MYXjt2Y6DmyWUVxVy73bRKvKbKoGXhAXJx2vbcGGxfXsScT0i4FIC2wsJ/8zy/otB5vymm3JHBf2+7cQvRdp12UMLAnzlfrbgZUpvV36D+q6ch7kbmFzaBfwRjOKhnZkRLCcMYPAdX1SrgKVNXaOzAl9KytbzGQs5MKEHU+a0PAwKfIvEsS/+pW6gKgBnL0uy4Gr5cYJ5rk48iwFXOlZ/B30gUS5dD+rNRJuR0ZgEkxtVIPVxxhQPtEI53JhlJ2nEy0CqNW88nYLmX402b ;{id = 34898 (zsk), size = 512b}")
    pubkey2 = ldns.ldns_rr.new_frm_str("example2. 3600 IN DNSKEY 256 3 3 ALBoD2+1xYpzrE7gjU5EwwBHG2HNiD1977LDZGh+8VNifMGjixMpgUN6xRhFjvRSsC/seMVXmUGq+msUDF2pHnUHbW/dbQbBxVMAqx2jT0LTvAx5wUPGltHHsa92K8VdzD8ynTFwPvjmk7g3hqRRzt4UTQIeK7DYgrOOgvDv+DYWVQctLwYP0ktm85b4cMtIUNRIf/N+K25pfK6BM/tHN8HOm4ECvm2U9zqHHfnxJFdiNK2PydkNeJZZGUOubSFVvaOMhZoEeAgkm3q5QcwXHsLAhacZ ;{id = 30944 (zsk), size = 512b}")
    ds1 = ldns.ldns_key_rr2ds(pubkey1, ldns.LDNS_SHA1)
    ds2 = ldns.ldns_key_rr2ds(pubkey2, ldns.LDNS_SHA1)
    try:
        ret = pubkey1.compare_ds(pubkey1)
        if not isinstance(ret, bool):
            set_error()
        if ret != True:
            set_error()
    except:
        set_error()
    try:
        ret = pubkey1.compare_ds(pubkey2)
        if not isinstance(ret, bool):
            set_error()
        if ret != False:
            set_error()
    except:
        set_error()
    try:
        ret = pubkey1.compare_ds(ds1)
        if not isinstance(ret, bool):
            set_error()
        if ret != True:
            set_error()
    except:
        set_error()
    try:
        ret = pubkey1.compare_ds(ds2)
        if not isinstance(ret, bool):
            set_error()
        if ret != False:
            set_error()
    except:
        set_error()
    try:
        pubkey1.compare_ds("")
        set_error()
    except TypeError:
        pass
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".compare_no_rdata()"
    rr1 = ldns.ldns_rr.new_frm_str("test 600 IN A 0.0.0.0")
    rr2 = ldns.ldns_rr.new_frm_str("test 600 IN AAAA ::")
    try:
        ret = rr1.compare_no_rdata(rr2)
        if not isinstance(ret, int):
            set_error()
        if ret != -27:
            set_error()
    except:
        set_error()
    try:
        rr1.compare_no_rdata("")
        set_error()
    except TypeError:
        pass
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".dnskey_algorithm()"
    pubkey = ldns.ldns_rr.new_frm_str("example1. 3600 IN DNSKEY 256 3 3 APw7tG8Nf7MYXjt2Y6DmyWUVxVy73bRKvKbKoGXhAXJx2vbcGGxfXsScT0i4FIC2wsJ/8zy/otB5vymm3JHBf2+7cQvRdp12UMLAnzlfrbgZUpvV36D+q6ch7kbmFzaBfwRjOKhnZkRLCcMYPAdX1SrgKVNXaOzAl9KytbzGQs5MKEHU+a0PAwKfIvEsS/+pW6gKgBnL0uy4Gr5cYJ5rk48iwFXOlZ/B30gUS5dD+rNRJuR0ZgEkxtVIPVxxhQPtEI53JhlJ2nEy0CqNW88nYLmX402b ;{id = 34898 (zsk), size = 512b}")
    try:
        ret = pubkey.dnskey_algorithm()
        if not isinstance(ret, ldns.ldns_rdf):
            set_error()
    except:
        set_error()
    rr = ldns.ldns_rr.new_frm_str("test 600 IN A 0.0.0.0")
    try:
        ret = rr.dnskey_algorithm()
        if isinstance(ret, ldns.ldns_rdf):
            set_error()
        if ret != None:
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".dnskey_flags()"
    pubkey = ldns.ldns_rr.new_frm_str("example1. 3600 IN DNSKEY 256 3 3 APw7tG8Nf7MYXjt2Y6DmyWUVxVy73bRKvKbKoGXhAXJx2vbcGGxfXsScT0i4FIC2wsJ/8zy/otB5vymm3JHBf2+7cQvRdp12UMLAnzlfrbgZUpvV36D+q6ch7kbmFzaBfwRjOKhnZkRLCcMYPAdX1SrgKVNXaOzAl9KytbzGQs5MKEHU+a0PAwKfIvEsS/+pW6gKgBnL0uy4Gr5cYJ5rk48iwFXOlZ/B30gUS5dD+rNRJuR0ZgEkxtVIPVxxhQPtEI53JhlJ2nEy0CqNW88nYLmX402b ;{id = 34898 (zsk), size = 512b}")
    try:
        ret = pubkey.dnskey_flags()
        if not isinstance(ret, ldns.ldns_rdf):
            set_error()
    except:
        set_error()
    rr = ldns.ldns_rr.new_frm_str("test 600 IN A 0.0.0.0")
    try:
        ret = rr.dnskey_flags()
        if isinstance(ret, ldns.ldns_rdf):
            set_error()
        if ret != None:
            set_error()
    except:
        set_errror()


#if not error_detected:
if True:
    method_name = class_name + ".dnskey_key()"
    pubkey = ldns.ldns_rr.new_frm_str("example1. 3600 IN DNSKEY 256 3 3 APw7tG8Nf7MYXjt2Y6DmyWUVxVy73bRKvKbKoGXhAXJx2vbcGGxfXsScT0i4FIC2wsJ/8zy/otB5vymm3JHBf2+7cQvRdp12UMLAnzlfrbgZUpvV36D+q6ch7kbmFzaBfwRjOKhnZkRLCcMYPAdX1SrgKVNXaOzAl9KytbzGQs5MKEHU+a0PAwKfIvEsS/+pW6gKgBnL0uy4Gr5cYJ5rk48iwFXOlZ/B30gUS5dD+rNRJuR0ZgEkxtVIPVxxhQPtEI53JhlJ2nEy0CqNW88nYLmX402b ;{id = 34898 (zsk), size = 512b}")
    try:
        ret = pubkey.dnskey_key()
        if not isinstance(ret, ldns.ldns_rdf):
            set_error()
    except:
        set_error()
    rr = ldns.ldns_rr.new_frm_str("test 600 IN A 0.0.0.0")
    try:
        ret = rr.dnskey_key()
        if isinstance(ret, ldns.ldns_rdf):
            set_error()
        if ret != None:
            set_error()
    except:
        set_errror()


#if not error_detected:
if True:
    method_name = class_name + ".dnskey_key_size()"
    pubkey = ldns.ldns_rr.new_frm_str("example1. 3600 IN DNSKEY 256 3 3 APw7tG8Nf7MYXjt2Y6DmyWUVxVy73bRKvKbKoGXhAXJx2vbcGGxfXsScT0i4FIC2wsJ/8zy/otB5vymm3JHBf2+7cQvRdp12UMLAnzlfrbgZUpvV36D+q6ch7kbmFzaBfwRjOKhnZkRLCcMYPAdX1SrgKVNXaOzAl9KytbzGQs5MKEHU+a0PAwKfIvEsS/+pW6gKgBnL0uy4Gr5cYJ5rk48iwFXOlZ/B30gUS5dD+rNRJuR0ZgEkxtVIPVxxhQPtEI53JhlJ2nEy0CqNW88nYLmX402b ;{id = 34898 (zsk), size = 512b}")
    try:
        ret = pubkey.dnskey_key_size()
        if (not isinstance(ret, int)) and (not isinstance(ret, long)):
            set_error()
        if ret != 512:
            set_error()
    except:
        set_error()
    rr = ldns.ldns_rr.new_frm_str("test 600 IN A 0.0.0.0")
    try:
        ret = rr.dnskey_key_size()
        if (not isinstance(ret, int)) and (not isinstance(ret, long)):
            set_error()
        if ret != 0:
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".dnskey_key_size_raw()"
    sys.stderr.write("%s not tested.\n" % (method_name))


#if not error_detected:
if True:
    method_name = class_name + ".dnskey_protocol()"
    pubkey = ldns.ldns_rr.new_frm_str("example1. 3600 IN DNSKEY 256 3 3 APw7tG8Nf7MYXjt2Y6DmyWUVxVy73bRKvKbKoGXhAXJx2vbcGGxfXsScT0i4FIC2wsJ/8zy/otB5vymm3JHBf2+7cQvRdp12UMLAnzlfrbgZUpvV36D+q6ch7kbmFzaBfwRjOKhnZkRLCcMYPAdX1SrgKVNXaOzAl9KytbzGQs5MKEHU+a0PAwKfIvEsS/+pW6gKgBnL0uy4Gr5cYJ5rk48iwFXOlZ/B30gUS5dD+rNRJuR0ZgEkxtVIPVxxhQPtEI53JhlJ2nEy0CqNW88nYLmX402b ;{id = 34898 (zsk), size = 512b}")
    try:
        ret = pubkey.dnskey_protocol()
        if not isinstance(ret, ldns.ldns_rdf):
            set_error()
    except:
        set_error()
    rr = ldns.ldns_rr.new_frm_str("test 600 IN A 0.0.0.0")
    try:
        ret = rr.dnskey_protocol()
        if isinstance(ret, ldns.ldns_rdf):
            set_error()
        if ret != None:
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".dnskey_set_algorithm()"
    pubkey = ldns.ldns_rr.new_frm_str("example1. 3600 IN DNSKEY 256 3 3 APw7tG8Nf7MYXjt2Y6DmyWUVxVy73bRKvKbKoGXhAXJx2vbcGGxfXsScT0i4FIC2wsJ/8zy/otB5vymm3JHBf2+7cQvRdp12UMLAnzlfrbgZUpvV36D+q6ch7kbmFzaBfwRjOKhnZkRLCcMYPAdX1SrgKVNXaOzAl9KytbzGQs5MKEHU+a0PAwKfIvEsS/+pW6gKgBnL0uy4Gr5cYJ5rk48iwFXOlZ/B30gUS5dD+rNRJuR0ZgEkxtVIPVxxhQPtEI53JhlJ2nEy0CqNW88nYLmX402b ;{id = 34898 (zsk), size = 512b}")
    rdf = ldns.ldns_rdf.new_frm_str("3", ldns.LDNS_RDF_TYPE_ALG)
    try:
        ret = pubkey.dnskey_set_algorithm(rdf)
        if not isinstance(ret, bool):
            set_error()
        if ret != True:
            set_error()
    except:
        set_error()
#    rdf = ldns.ldns_rdf.new_frm_str("0.0.0.0", ldns.LDNS_RDF_TYPE_A)
#    try:
#        ret = pubkey.dnskey_set_algorithm(rdf)
#        if not isinstance(ret, bool):
#            set_error()
#        if ret != False:
#            set_error()
#    except:
#        set_error()
    try:
        ret = pubkey.dnskey_set_algorithm("")
        set_error()
    except TypeError:
        pass
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".dnskey_set_flags()"
    pubkey = ldns.ldns_rr.new_frm_str("example1. 3600 IN DNSKEY 256 3 3 APw7tG8Nf7MYXjt2Y6DmyWUVxVy73bRKvKbKoGXhAXJx2vbcGGxfXsScT0i4FIC2wsJ/8zy/otB5vymm3JHBf2+7cQvRdp12UMLAnzlfrbgZUpvV36D+q6ch7kbmFzaBfwRjOKhnZkRLCcMYPAdX1SrgKVNXaOzAl9KytbzGQs5MKEHU+a0PAwKfIvEsS/+pW6gKgBnL0uy4Gr5cYJ5rk48iwFXOlZ/B30gUS5dD+rNRJuR0ZgEkxtVIPVxxhQPtEI53JhlJ2nEy0CqNW88nYLmX402b ;{id = 34898 (zsk), size = 512b}")
    rdf = ldns.ldns_rdf.new_frm_str("256", ldns.LDNS_RDF_TYPE_INT16)
    try:
        ret = pubkey.dnskey_set_flags(rdf)
        if not isinstance(ret, bool):
            set_error()
        if ret != True:
            set_error()
    except:
        set_error()
#    rdf = ldns.ldns_rdf.new_frm_str("0.0.0.0", ldns.LDNS_RDF_TYPE_A)
#    try:
#        ret = pubkey.dnskey_set_flags(rdf)
#        if not isinstance(ret, bool):
#            set_error()
#        if ret != False:
#            set_error()
#    except:
#        set_error()
    try:
        ret = pubkey.dnskey_set_flags("")
        set_error()
    except TypeError:
        pass
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".dnskey_set_key()"
    pubkey = ldns.ldns_rr.new_frm_str("example1. 3600 IN DNSKEY 256 3 3 APw7tG8Nf7MYXjt2Y6DmyWUVxVy73bRKvKbKoGXhAXJx2vbcGGxfXsScT0i4FIC2wsJ/8zy/otB5vymm3JHBf2+7cQvRdp12UMLAnzlfrbgZUpvV36D+q6ch7kbmFzaBfwRjOKhnZkRLCcMYPAdX1SrgKVNXaOzAl9KytbzGQs5MKEHU+a0PAwKfIvEsS/+pW6gKgBnL0uy4Gr5cYJ5rk48iwFXOlZ/B30gUS5dD+rNRJuR0ZgEkxtVIPVxxhQPtEI53JhlJ2nEy0CqNW88nYLmX402b ;{id = 34898 (zsk), size = 512b}")
    rdf = ldns.ldns_rdf.new_frm_str("AMLdYflByPu1GEPCnu9qPTqbnC8n5mftFmFVTFQI10aefiDqp5DLpjBdTxdmz/GACMZh1+YG/iLj0QYX7qRVIl0rR00iREozqj44YwUILHo3cASSRSeAzyidvlGT8QSMKOlOsD33ygtETpzW0XDmzWhyU3bv0O7lnGpbtqdzP/nsZDbdtf5XI0YBdi91HftqtQpIlMtCg+zIzATO4+QWGt0oDX/+jdB7Y/vBahxnz13stNYeGYslGBSZNgpB7HBKlTwB70sprZ8XmNGhj/NixqB6Bzae", ldns.LDNS_RDF_TYPE_B64)
    try:
        ret = pubkey.dnskey_set_key(rdf)
        if not isinstance(ret, bool):
            set_error()
        if ret != True:
            set_error()
    except:
        set_error()
#    rdf = ldns.ldns_rdf.new_frm_str("0.0.0.0", ldns.LDNS_RDF_TYPE_A)
#    try:
#        ret = pubkey.dnskey_set_key(rdf)
#        if not isinstance(ret, bool):
#            set_error()
#        if ret != False:
#            set_error()
#    except:
#        set_error()
    try:
        ret = pubkey.dnskey_set_key("")
        set_error()
    except TypeError:
        pass
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".dnskey_set_protocol()"
    pubkey = ldns.ldns_rr.new_frm_str("example1. 3600 IN DNSKEY 256 3 3 APw7tG8Nf7MYXjt2Y6DmyWUVxVy73bRKvKbKoGXhAXJx2vbcGGxfXsScT0i4FIC2wsJ/8zy/otB5vymm3JHBf2+7cQvRdp12UMLAnzlfrbgZUpvV36D+q6ch7kbmFzaBfwRjOKhnZkRLCcMYPAdX1SrgKVNXaOzAl9KytbzGQs5MKEHU+a0PAwKfIvEsS/+pW6gKgBnL0uy4Gr5cYJ5rk48iwFXOlZ/B30gUS5dD+rNRJuR0ZgEkxtVIPVxxhQPtEI53JhlJ2nEy0CqNW88nYLmX402b ;{id = 34898 (zsk), size = 512b}")
    rdf = ldns.ldns_rdf.new_frm_str("3", ldns.LDNS_RDF_TYPE_INT8)
    try:
        ret = pubkey.dnskey_set_protocol(rdf)
        if not isinstance(ret, bool):
            set_error()
        if ret != True:
            set_error()
    except:
        set_error()
#    rdf = ldns.ldns_rdf.new_frm_str("0.0.0.0", ldns.LDNS_RDF_TYPE_A)
#    try:
#        ret = pubkey.dnskey_set_protocol(rdf)
#        if not isinstance(ret, bool):
#            set_error()
#        if ret != False:
#            set_error()
#    except:
#        set_error()
    try:
        ret = pubkey.dnskey_set_protocol("")
        set_error()
    except TypeError:
        pass
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".get_class()"
    rr = ldns.ldns_rr.new_frm_str("test IN A 0.0.0.0", 600)
    try:
        ret = rr.get_class()
        if not isinstance(ret, int):
            set_error()
        if ret != ldns.LDNS_RR_CLASS_IN:
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".get_class_str()"
    rr = ldns.ldns_rr.new_frm_str("test CH A 0.0.0.0", 600)
    try:
        ret = rr.get_class_str()
        if not isinstance(ret, str):
            set_error()
        if ret != "CH":
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".get_type()"
    rr = ldns.ldns_rr.new_frm_str("test IN A 0.0.0.0", 600)
    try:
        ret = rr.get_type()
        if not isinstance(ret, int):
            set_error()
        if ret != 1:
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".get_type_str()"
    rr = ldns.ldns_rr.new_frm_str("test IN A 0.0.0.0", 600)
    try:
        ret = rr.get_type_str()
        if not isinstance(ret, str):
            set_error()
        if ret != "A":
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".is_question()"
    rr = ldns.ldns_rr.new_frm_str("test IN A 0.0.0.0", 600)
    try:
        ret = rr.is_question()
        if not isinstance(ret, bool):
            set_error()
        if ret != False:
            set_error()
    except:
        set_error()
    rr.set_question(True)
    try:
        ret = rr.is_question()
        if not isinstance(ret, bool):
            set_error()
        if ret != True:
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".label_count()"
    rr = ldns.ldns_rr.new_frm_str("test.dom. IN A 0.0.0.0", 600)
    try:
        ret = rr.label_count()
        if (not isinstance(ret, int)) and (not isinstance(ret, long)):
            set_error()
        if ret != 2:
            set_error()
    except:
        set_error()
    rr = ldns.ldns_rr.new_frm_str(". IN A 0.0.0.0", 600)
    try:
        ret = rr.label_count()
        if (not isinstance(ret, int)) and (not isinstance(ret, long)):
            set_error(string)
        if ret != 0:
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".mx_exchange()"
    rr = ldns.ldns_rr.new_frm_str("nic.cz. IN MX 15 mail4.nic.cz.", 600)
    try:
        ret = rr.mx_exchange()
        if not isinstance(ret, ldns.ldns_rdf):
            set_error()
    except:
        set_error()
    rr = ldns.ldns_rr.new_frm_str("test 600 IN A 0.0.0.0")
    try:
        ret = rr.mx_exchange()
        if isinstance(ret, ldns.ldns_rdf):
            set_error()
        if ret != None:
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".mx_preference()"
    rr = ldns.ldns_rr.new_frm_str("nic.cz. IN MX 15 mail4.nic.cz.", 600)
    try:
        ret = rr.mx_preference()
        if not isinstance(ret, ldns.ldns_rdf):
            set_error()
    except:
        set_error()
    rr = ldns.ldns_rr.new_frm_str("test 600 IN A 0.0.0.0")
    try:
        ret = rr.mx_preference()
        if isinstance(ret, ldns.ldns_rdf):
            set_error()
        if ret != None:
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".new_frm_fp()"
    f = open(temp_fname, "w")
    f.write("test 600 IN A 0.0.0.0")
    f.close()
    f = open(temp_fname, "r")
    rr, ttl, origin, prev = ldns.ldns_rr.new_frm_fp(f,
        origin=ldns.ldns_dname("nic.cz"))
    try:
        # Reading past file end.
        ret = ldns.ldns_rr.new_frm_fp(f, raiseException=False)
        if ret != None:
            set_error()
    except:
        set_error()
    try:
        # Reading past file end.
        rr, ttl, origin, prev = ldns.ldns_rr.new_frm_fp(f)
        set_error()
    except Exception:
        pass
    except:
        set_error()
    f.close()
    os.remove(temp_fname)


#if not error_detected:
if True:
    method_name = class_name + ".new_frm_fp_l()"
    f = open(temp_fname, "w")
    f.write("test 600 IN A 0.0.0.0")
    f.close()
    f = open(temp_fname, "r")
    rr, line, ttl, origin, prev = ldns.ldns_rr.new_frm_fp_l(f,
        origin=ldns.ldns_dname("nic.cz"))
    try:
        # Reading past file end.
        ret = ldns.ldns_rr.new_frm_fp_l(f, raiseException=False)
        if ret != None:
            set_error()
    except:
        set_error()
    try:
        # Reading past file end.
        rr, ttl, origin, prev = ldns.ldns_rr.new_frm_fp_l(f)
        set_error()
    except Exception:
        pass
    except:
        set_error()
    f.close()
    os.remove(temp_fname)


#if not error_detected:
if True:
    method_name = class_name + ".new_frm_str()"
    try:
        rr = ldns.ldns_rr.new_frm_str("test IN A 0.0.0.0", 600,
            origin=ldns.ldns_dname("nic.cz"))
        if not isinstance(rr, ldns.ldns_rr):
            set_error()
    except:
        set_error()
    try:
        rr = ldns.ldns_rr.new_frm_str(10)
        set_error()
    except TypeError:
        pass
    except:
        set_error()
    try:
        rr = ldns.ldns_rr.new_frm_str("")
        set_error()
    except Exception:
        pass
    except:
        set_error()
    try:
        rr = ldns.ldns_rr.new_frm_str("", raiseException=False)
        if rr != None:
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".new_frm_str_prev()"
    try:
        rr, prev = ldns.ldns_rr.new_frm_str_prev("test IN A 0.0.0.0", 600,
            origin=ldns.ldns_dname("nic.cz"))
        if not isinstance(rr, ldns.ldns_rr):
            set_error()
#        if prev != None:
#            set_error()
    except:
        set_error()
    try:
        rr = ldns.ldns_rr.new_frm_str_prev(10)
        set_error()
    except TypeError:
        pass
    except:
        set_error()
    try:
        rr = ldns.ldns_rr.new_frm_str_prev("")
        set_error()
    except Exception:
        pass
    except:
        set_error()
    try:
        rr = ldns.ldns_rr.new_frm_str_prev("", raiseException=False)
        if rr != None:
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".new_question_frm_str()"
    try:
        rr = ldns.ldns_rr.new_question_frm_str("test IN A", 600,
            origin=ldns.ldns_dname("nic.cz"))
        if not isinstance(rr, ldns.ldns_rr):
            set_error()
    except:
        set_error()
    try:
        rr = ldns.ldns_rr.new_question_frm_str(10)
        set_error()
    except TypeError:
        pass
    except:
        set_error()
    try:
        rr = ldns.ldns_rr.new_question_frm_str("")
        set_error()
    except Exception:
        pass
    except:
        set_error()
    try:
        rr = ldns.ldns_rr.new_question_frm_str("", raiseException=False)
        if rr != None:
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".ns_nsdname()"
    rr = ldns.ldns_rr.new_frm_str("nic.cz. 1800 IN NS a.ns.nic.cz.")
    try:
        ret = rr.ns_nsdname()
        if not isinstance(ret, ldns.ldns_rdf):
            set_error()
    except:
        set_error()
    rr = ldns.ldns_rr.new_frm_str("test 600 IN A 0.0.0.0")
    try:
        ret = rr.ns_nsdname()
        if isinstance(ret, ldns.ldns_rdf):
            set_error()
        if ret != None:
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".owner()"
    rr = ldns.ldns_rr.new_frm_str("nic.cz. 1800 IN NS a.ns.nic.cz.")
    try:
        ret = rr.owner()
        if not isinstance(ret, ldns.ldns_dname):
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".pop_rdf()"
    rr = ldns.ldns_rr.new_frm_str("test 600 IN A 0.0.0.0")
    try:
        ret = rr.pop_rdf()
        if not isinstance(ret, ldns.ldns_rdf):
            set_error()
        if ret.get_type() != ldns.LDNS_RDF_TYPE_A:
            set_error()
    except:
        set_error()
    try:
        ret = rr.pop_rdf()
        if isinstance(ret, ldns.ldns_rdf):
            set_error()
        if ret != None:
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".print_to_file()"
    rr = ldns.ldns_rr.new_frm_str("test 600 IN A 0.0.0.0")
    f = open(temp_fname, "w")
    try:
        rr.print_to_file(f)
    except:
        set_error()
    f.close()
    f = open(temp_fname, "r")
    if not f.readline():
        set_error()
    f.close()
    os.remove(temp_fname)


#if not error_detected:
if True:
    method_name = class_name + ".push_rdf()"
    rr = ldns.ldns_rr.new_frm_str("test 600 IN A 0.0.0.0")
    rdf = ldns.ldns_rdf.new_frm_str("1.1.1.1", ldns.LDNS_RDF_TYPE_A)
    try:
        ret = rr.push_rdf(rdf)
        if not isinstance(ret, bool):
            set_error()
        if ret != True:
            set_error()
    except:
        set_error()
    try:
        ret = rr.push_rdf("")
        set_error()
    except TypeError:
        pass
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".rd_count()"
    rr = ldns.ldns_rr.new_frm_str("test 600 IN A 0.0.0.0")
    try:
        ret = rr.rd_count()
        if (not isinstance(ret, int)) and (not isinstance(ret, long)):
            set_error()
        if ret != 1:
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".rdf()"
    rr = ldns.ldns_rr.new_frm_str("test 600 IN A 0.0.0.0")
    try:
        ret = rr.rdf(0)
        if not isinstance(ret, ldns.ldns_rdf):
            set_error()
    except:
        set_error()
    try:
        ret = rr.rdf(1)
        if isinstance(ret, ldns.ldns_rdf):
            set_error()
        if ret != None:
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".rdfs()"
    rr = ldns.ldns_rr.new_frm_str("test 600 IN A 0.0.0.0")
    try:
        ret = rr.rdfs()
        if len(list(ret)) != 1:
            set_error()
    except:
        set_error()



#if not error_detected:
if True:
    method_name = class_name + ".rrsig_algorithm()"
    rr = ldns.ldns_rr.new_frm_str("example. 600 IN RRSIG SOA 3 1 600 20130828153754 20120828153754 19031 example. AIoCFhwZJxIgYOBEyo3cxxWFZEsUPqkxnt38xEl1cFAHHC9iQN9mlEg=")
    try:
        ret = rr.rrsig_algorithm()
        if not isinstance(ret, ldns.ldns_rdf):
            set_error()
    except:
        set_error()
    rr = ldns.ldns_rr.new_frm_str("test 600 IN A 0.0.0.0")
    try:
        ret = rr.rrsig_algorithm()
        if isinstance(ret, ldns.ldns_rdf):
            set_error()
        if ret != None:
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".rrsig_expiration()"
    rr = ldns.ldns_rr.new_frm_str("example. 600 IN RRSIG SOA 3 1 600 20130828153754 20120828153754 19031 example. AIoCFhwZJxIgYOBEyo3cxxWFZEsUPqkxnt38xEl1cFAHHC9iQN9mlEg=")
    try:
        ret = rr.rrsig_expiration()
        if not isinstance(ret, ldns.ldns_rdf):
            set_error()
    except:
        set_error()
    rr = ldns.ldns_rr.new_frm_str("test 600 IN A 0.0.0.0")
    try:
        ret = rr.rrsig_expiration()
        if isinstance(ret, ldns.ldns_rdf):
            set_error()
        if ret != None:
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".rrsig_inception()"
    rr = ldns.ldns_rr.new_frm_str("example. 600 IN RRSIG SOA 3 1 600 20130828153754 20120828153754 19031 example. AIoCFhwZJxIgYOBEyo3cxxWFZEsUPqkxnt38xEl1cFAHHC9iQN9mlEg=")
    try:
        ret = rr.rrsig_inception()
        if not isinstance(ret, ldns.ldns_rdf):
            set_error()
    except:
        set_error()
    rr = ldns.ldns_rr.new_frm_str("test 600 IN A 0.0.0.0")
    try:
        ret = rr.rrsig_inception()
        if isinstance(ret, ldns.ldns_rdf):
            set_error()
        if ret != None:
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".rrsig_keytag()"
    rr = ldns.ldns_rr.new_frm_str("example. 600 IN RRSIG SOA 3 1 600 20130828153754 20120828153754 19031 example. AIoCFhwZJxIgYOBEyo3cxxWFZEsUPqkxnt38xEl1cFAHHC9iQN9mlEg=")
    try:
        ret = rr.rrsig_keytag()
        if not isinstance(ret, ldns.ldns_rdf):
            set_error()
    except:
        set_error()
    rr = ldns.ldns_rr.new_frm_str("test 600 IN A 0.0.0.0")
    try:
        ret = rr.rrsig_keytag()
        if isinstance(ret, ldns.ldns_rdf):
            set_error()
        if ret != None:
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".rrsig_labels()"
    rr = ldns.ldns_rr.new_frm_str("example. 600 IN RRSIG SOA 3 1 600 20130828153754 20120828153754 19031 example. AIoCFhwZJxIgYOBEyo3cxxWFZEsUPqkxnt38xEl1cFAHHC9iQN9mlEg=")
    try:
        ret = rr.rrsig_labels()
        if not isinstance(ret, ldns.ldns_rdf):
            set_error()
    except:
        set_error()
    rr = ldns.ldns_rr.new_frm_str("test 600 IN A 0.0.0.0")
    try:
        ret = rr.rrsig_labels()
        if isinstance(ret, ldns.ldns_rdf):
            set_error()
        if ret != None:
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".rrsig_origttl()"
    rr = ldns.ldns_rr.new_frm_str("example. 600 IN RRSIG SOA 3 1 600 20130828153754 20120828153754 19031 example. AIoCFhwZJxIgYOBEyo3cxxWFZEsUPqkxnt38xEl1cFAHHC9iQN9mlEg=")
    try:
        ret = rr.rrsig_origttl()
        if not isinstance(ret, ldns.ldns_rdf):
            set_error()
    except:
        set_error()
    rr = ldns.ldns_rr.new_frm_str("test 600 IN A 0.0.0.0")
    try:
        ret = rr.rrsig_origttl()
        if isinstance(ret, ldns.ldns_rdf):
            set_error()
        if ret != None:
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".rrsig_set_algorithm()"
    rr = ldns.ldns_rr.new_frm_str("example. 600 IN RRSIG SOA 3 1 600 20130828153754 20120828153754 19031 example. AIoCFhwZJxIgYOBEyo3cxxWFZEsUPqkxnt38xEl1cFAHHC9iQN9mlEg=")
    rdf = ldns.ldns_rdf.new_frm_str("3", ldns.LDNS_RDF_TYPE_ALG)
    try:
        ret = rr.rrsig_set_algorithm(rdf)
        if not isinstance(ret, bool):
            set_error()
        if ret != True:
            set_error()
    except:
        set_error()
#    rdf = ldns.ldns_rdf.new_frm_str("0.0.0.0", ldns.LDNS_RDF_TYPE_A)
#    try:
#        ret = rr.rrsig_set_algorithm(rdf)
#        if not isinstance(ret, bool):
#            set_error()
#        if ret != False:
#            set_error()
#    except:
#        set_error()
    try:
        ret = rr.rrsig_set_algorithm("")
        set_error()
    except TypeError:
        pass
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".rrsig_set_expiration()"
    rr = ldns.ldns_rr.new_frm_str("example. 600 IN RRSIG SOA 3 1 600 20130828153754 20120828153754 19031 example. AIoCFhwZJxIgYOBEyo3cxxWFZEsUPqkxnt38xEl1cFAHHC9iQN9mlEg=")
    rdf = ldns.ldns_rdf.new_frm_str("20130928153754", ldns.LDNS_RDF_TYPE_TIME)
    try:
        ret = rr.rrsig_set_expiration(rdf)
        if not isinstance(ret, bool):
            set_error()
        if ret != True:
            set_error()
    except:
        set_error()
#    rdf = ldns.ldns_rdf.new_frm_str("0.0.0.0", ldns.LDNS_RDF_TYPE_A)
#    try:
#        ret = rr.rrsig_set_expiration(rdf)
#        if not isinstance(ret, bool):
#            set_error()
#        if ret != False:
#            set_error()
#    except:
#        set_error()
    try:
        ret = rr.rrsig_set_expiration("")
        set_error()
    except TypeError:
        pass
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".rrsig_set_inception()"
    rr = ldns.ldns_rr.new_frm_str("example. 600 IN RRSIG SOA 3 1 600 20130828153754 20120828153754 19031 example. AIoCFhwZJxIgYOBEyo3cxxWFZEsUPqkxnt38xEl1cFAHHC9iQN9mlEg=")
    rdf = ldns.ldns_rdf.new_frm_str("20120728153754", ldns.LDNS_RDF_TYPE_TIME)
    try:
        ret = rr.rrsig_set_inception(rdf)
        if not isinstance(ret, bool):
            set_error()
        if ret != True:
            set_error()
    except:
        set_error()
#    rdf = ldns.ldns_rdf.new_frm_str("0.0.0.0", ldns.LDNS_RDF_TYPE_A)
#    try:
#        ret = rr.rrsig_set_inception(rdf)
#        if not isinstance(ret, bool):
#            set_error()
#        if ret != False:
#            set_error()
#    except:
#        set_error()
    try:
        ret = rr.rrsig_set_inception("")
        set_error()
    except TypeError:
        pass
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".rrsig_set_keytag()"
    rr = ldns.ldns_rr.new_frm_str("example. 600 IN RRSIG SOA 3 1 600 20130828153754 20120828153754 19031 example. AIoCFhwZJxIgYOBEyo3cxxWFZEsUPqkxnt38xEl1cFAHHC9iQN9mlEg=")
    rdf = ldns.ldns_rdf.new_frm_str("19032", ldns.LDNS_RDF_TYPE_INT16)
    try:
        ret = rr.rrsig_set_keytag(rdf)
        if not isinstance(ret, bool):
            set_error()
        if ret != True:
            set_error()
    except:
        set_error()
#    rdf = ldns.ldns_rdf.new_frm_str("0.0.0.0", ldns.LDNS_RDF_TYPE_A)
#    try:
#        ret = rr.rrsig_set_keytag(rdf)
#        if not isinstance(ret, bool):
#            set_error()
#        if ret != False:
#            set_error()
#    except:
#        set_error()
    try:
        ret = rr.rrsig_set_keytag("")
        set_error()
    except TypeError:
        pass
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".rrsig_set_labels()"
    rr = ldns.ldns_rr.new_frm_str("example. 600 IN RRSIG SOA 3 1 600 20130828153754 20120828153754 19031 example. AIoCFhwZJxIgYOBEyo3cxxWFZEsUPqkxnt38xEl1cFAHHC9iQN9mlEg=")
    rdf = ldns.ldns_rdf.new_frm_str("1", ldns.LDNS_RDF_TYPE_INT8)
    try:
        ret = rr.rrsig_set_labels(rdf)
        if not isinstance(ret, bool):
            set_error()
        if ret != True:
            set_error()
    except:
        set_error()
#    rdf = ldns.ldns_rdf.new_frm_str("0.0.0.0", ldns.LDNS_RDF_TYPE_A)
#    try:
#        ret = rr.rrsig_set_labels(rdf)
#        if not isinstance(ret, bool):
#            set_error()
#        if ret != False:
#            set_error()
#    except:
#        set_error()
    try:
        ret = rr.rrsig_set_labels("")
        set_error()
    except TypeError:
        pass
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".rrsig_set_origttl()"
    rr = ldns.ldns_rr.new_frm_str("example. 600 IN RRSIG SOA 3 1 600 20130828153754 20120828153754 19031 example. AIoCFhwZJxIgYOBEyo3cxxWFZEsUPqkxnt38xEl1cFAHHC9iQN9mlEg=")
    rdf = ldns.ldns_rdf.new_frm_str("1", ldns.LDNS_RDF_TYPE_INT8)
    try:
        ret = rr.rrsig_set_origttl(rdf)
        if not isinstance(ret, bool):
            set_error()
        if ret != True:
            set_error()
    except:
        set_error()
#    rdf = ldns.ldns_rdf.new_frm_str("0.0.0.0", ldns.LDNS_RDF_TYPE_A)
#    try:
#        ret = rr.rrsig_set_origttl(rdf)
#        if not isinstance(ret, bool):
#            set_error()
#        if ret != False:
#            set_error()
#    except:
#        set_error()
    try:
        ret = rr.rrsig_set_origttl("")
        set_error()
    except TypeError:
        pass
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".rrsig_set_sig()"
    rr = ldns.ldns_rr.new_frm_str("example. 600 IN RRSIG SOA 3 1 600 20130828153754 20120828153754 19031 example. AIoCFhwZJxIgYOBEyo3cxxWFZEsUPqkxnt38xEl1cFAHHC9iQN9mlEg=")
    rdf = ldns.ldns_rdf.new_frm_str("AIoCFhwZJxIgYOBEyo3cxxWFZEsUPqkxnt38xEl1cFAHHC9iQN9mlEg=", ldns.LDNS_RDF_TYPE_B64)
    try:
        ret = rr.rrsig_set_sig(rdf)
        if not isinstance(ret, bool):
            set_error()
        if ret != True:
            set_error()
    except:
        set_error()
#    rdf = ldns.ldns_rdf.new_frm_str("0.0.0.0", ldns.LDNS_RDF_TYPE_A)
#    try:
#        ret = rr.rrsig_set_sig(rdf)
#        if not isinstance(ret, bool):
#            set_error()
#        if ret != False:
#            set_error()
#    except:
#        set_error()
    try:
        ret = rr.rrsig_set_sig("")
        set_error()
    except TypeError:
        pass
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".rrsig_set_signame()"
    rr = ldns.ldns_rr.new_frm_str("example. 600 IN RRSIG SOA 3 1 600 20130828153754 20120828153754 19031 example. AIoCFhwZJxIgYOBEyo3cxxWFZEsUPqkxnt38xEl1cFAHHC9iQN9mlEg=")
    rdf = ldns.ldns_rdf.new_frm_str("example.", ldns.LDNS_RDF_TYPE_DNAME)
    try:
        ret = rr.rrsig_set_signame(rdf)
        if not isinstance(ret, bool):
            set_error()
        if ret != True:
            set_error()
    except:
        set_error()
#    rdf = ldns.ldns_rdf.new_frm_str("0.0.0.0", ldns.LDNS_RDF_TYPE_A)
#    try:
#        ret = rr.rrsig_set_signame(rdf)
#        if not isinstance(ret, bool):
#            set_error()
#        if ret != False:
#            set_error()
#    except:
#        set_error()
    try:
        ret = rr.rrsig_set_signame("")
        set_error()
    except TypeError:
        pass
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".rrsig_set_typecovered()"
    rr = ldns.ldns_rr.new_frm_str("example. 600 IN RRSIG SOA 3 1 600 20130828153754 20120828153754 19031 example. AIoCFhwZJxIgYOBEyo3cxxWFZEsUPqkxnt38xEl1cFAHHC9iQN9mlEg=")
    rdf = ldns.ldns_rdf.new_frm_str("SOA", ldns.LDNS_RDF_TYPE_TYPE)
    try:
        ret = rr.rrsig_set_typecovered(rdf)
        if not isinstance(ret, bool):
            set_error()
        if ret != True:
            set_error()
    except:
        set_error()
#    rdf = ldns.ldns_rdf.new_frm_str("0.0.0.0", ldns.LDNS_RDF_TYPE_A)
#    try:
#        ret = rr.rrsig_set_typecovered(rdf)
#        if not isinstance(ret, bool):
#            set_error()
#        if ret != False:
#            set_error()
#    except:
#        set_error()
    try:
        ret = rr.rrsig_set_typecovered("")
        set_error()
    except TypeError:
        pass
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".rrsig_sig()"
    rr = ldns.ldns_rr.new_frm_str("example. 600 IN RRSIG SOA 3 1 600 20130828153754 20120828153754 19031 example. AIoCFhwZJxIgYOBEyo3cxxWFZEsUPqkxnt38xEl1cFAHHC9iQN9mlEg=")
    try:
        ret = rr.rrsig_sig()
        if not isinstance(ret, ldns.ldns_rdf):
            set_error()
    except:
        set_error()
    rr = ldns.ldns_rr.new_frm_str("test 600 IN A 0.0.0.0")
    try:
        ret = rr.rrsig_sig()
        if isinstance(ret, ldns.ldns_rdf):
            set_error()
        if ret != None:
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".rrsig_signame()"
    rr = ldns.ldns_rr.new_frm_str("example. 600 IN RRSIG SOA 3 1 600 20130828153754 20120828153754 19031 example. AIoCFhwZJxIgYOBEyo3cxxWFZEsUPqkxnt38xEl1cFAHHC9iQN9mlEg=")
    try:
        ret = rr.rrsig_signame()
        if not isinstance(ret, ldns.ldns_rdf):
            set_error()
    except:
        set_error()
    rr = ldns.ldns_rr.new_frm_str("test 600 IN A 0.0.0.0")
    try:
        ret = rr.rrsig_signame()
        if isinstance(ret, ldns.ldns_rdf):
            set_error()
        if ret != None:
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".rrsig_typecovered()"
    rr = ldns.ldns_rr.new_frm_str("example. 600 IN RRSIG SOA 3 1 600 20130828153754 20120828153754 19031 example. AIoCFhwZJxIgYOBEyo3cxxWFZEsUPqkxnt38xEl1cFAHHC9iQN9mlEg=")
    try:
        ret = rr.rrsig_typecovered()
        if not isinstance(ret, ldns.ldns_rdf):
            set_error()
    except:
        set_error()
    rr = ldns.ldns_rr.new_frm_str("test 600 IN A 0.0.0.0")
    try:
        ret = rr.rrsig_typecovered()
        if isinstance(ret, ldns.ldns_rdf):
            set_error()
        if ret != None:
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".set_class()"
    rr = ldns.ldns_rr.new_frm_str("test 600 IN A 0.0.0.0")
    try:
        rr.set_class(ldns.LDNS_RR_CLASS_CH)
    except:
        set_error()
    try:
        rr.set_class("")
        set_error()
    except TypeError:
        pass
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".set_owner()"
    rr = ldns.ldns_rr.new_frm_str("test 600 IN A 0.0.0.0")
    rdf = ldns.ldns_dname("test2")
    try:
        rr.set_owner(rdf)
    except:
        set_error()
    try:
        rr.set_owner("")
        set_error()
    except TypeError:
        pass
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".set_question()"
    rr = ldns.ldns_rr.new_frm_str("test 600 IN A 0.0.0.0")
    try:
        rr.set_question(True)
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".set_rd_count()"
    rr = ldns.ldns_rr.new_frm_str("test 600 IN A 0.0.0.0")
    try:
        rr.set_rd_count(1)
    except:
        set_error()
    try:
        rr.set_rd_count("")
        set_error()
    except TypeError:
        pass
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".set_rdf()"
    rr = ldns.ldns_rr.new_frm_str("test 600 IN A 0.0.0.0")
    rdf = ldns.ldns_rdf.new_frm_str("1.1.1.1", ldns.LDNS_RDF_TYPE_A)
    rr.push_rdf(rdf)
    try:
        ret = rr.set_rdf(rdf, 0)
        if not isinstance(ret, ldns.ldns_rdf):
            set_error()
    except:
        set_error()
    try:
        ret = rr.set_rdf(rdf, 2)
        if isinstance(ret, ldns.ldns_rdf):
            set_error()
        if ret != None:
            set_error()
    except:
        set_error()
    try:
        rr.set_rdf("", 1)
        set_error()
    except TypeError:
        pass
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".set_ttl()"
    rr = ldns.ldns_rr.new_frm_str("test 600 IN A 0.0.0.0")
    try:
        rr.set_ttl(1)
    except:
        set_error()
    try:
        rr.set_ttl("")
        set_error()
    except TypeError:
        pass
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".set_type()"
    rr = ldns.ldns_rr.new_frm_str("test 600 IN A 0.0.0.0")
    try:
        rr.set_type(ldns.LDNS_RR_TYPE_A)
    except:
        set_error()
    try:
        rr.set_type("")
        set_error()
    except TypeError:
        pass
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".to_canonical()"
    rr = ldns.ldns_rr.new_frm_str("TEST 600 IN A 0.0.0.0")
    try:
        rr.to_canonical()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".ttl()"
    rr = ldns.ldns_rr.new_frm_str("test 600 IN A 0.0.0.0")
    try:
        ret = rr.ttl()
        if not isinstance(ret, int):
            set_error()
        if ret != 600:
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".type_by_name()"
    try:
        ret = ldns.ldns_rr.type_by_name("A")
        if not isinstance(ret, int):
            set_error()
        if ret != ldns.LDNS_RR_TYPE_A:
            set_error()
    except:
        set_error()
    try:
        ret = ldns.ldns_rr.type_by_name("AA")
        if not isinstance(ret, int):
            set_error()
        if ret != 0:
            set_error()
    except:
        set_error()
    try:
        ret = ldns.ldns_rr.type_by_name(1)
        set_error()
    except TypeError:
        pass
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".uncompressed_size()"
    rr = ldns.ldns_rr.new_frm_str("test 600 IN A 0.0.0.0")
    try:
        ret = rr.uncompressed_size()
        if (not isinstance(ret, int)) and (not isinstance(ret, long)):
            set_error()
        if ret != 20:
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".write_data_to_buffer()"
    sys.stderr.write("%s not tested.\n" % (method_name))


#if not error_detected:
if True:
    method_name = class_name + ".write_rrsig_to_buffer()"
    sys.stderr.write("%s not tested.\n" % (method_name))


#if not error_detected:
if True:
    method_name = class_name + ".write_to_buffer()"
    sys.stderr.write("%s not tested.\n" % (method_name))


#if not error_detected:
if True:
    method_name = class_name + ".write_to_buffer_canonical()"
    sys.stderr.write("%s not tested.\n" % (method_name))


###############################################################################
###############################################################################


class_name = "ldns_rr_descriptor"
method_name = None
error_detected = False
temp_fname = "tmp_rr_descriptor.txt"


#if not error_detected:
if True:
    method_name = class_name + ".field_type()"
    desc_a = ldns.ldns_rr_descriptor.ldns_rr_descriptor(ldns.LDNS_RR_TYPE_A)
    try:
        ret = desc_a.field_type(0)
        if not isinstance(ret, int):
            set_error()
        if ret != ldns.LDNS_RDF_TYPE_A:
            set_error()
    except:
        set_error()
    try:
        ret = desc_a.field_type("")
        set_error()
    except TypeError:
        pass
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".ldns_rr_descriptor()"
    try:
        ret = ldns.ldns_rr_descriptor.ldns_rr_descriptor(ldns.LDNS_RR_TYPE_A)
        if not isinstance(ret, ldns.ldns_rr_descriptor):
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".maximum()"
    desc_a = ldns.ldns_rr_descriptor.ldns_rr_descriptor(ldns.LDNS_RR_TYPE_A)
    try:
        ret = desc_a.maximum()
        if (not isinstance(ret, int)) and (not isinstance(ret, long)):
            set_error()
        if ret != 1:
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".minimum()"
    desc_a = ldns.ldns_rr_descriptor.ldns_rr_descriptor(ldns.LDNS_RR_TYPE_A)
    try:
        ret = desc_a.minimum()
        if (not isinstance(ret, int)) and (not isinstance(ret, long)):
            set_error()
        if ret != 1:
            set_error()
    except:
        set_error()


###############################################################################
###############################################################################


class_name = "ldns_rr_list"
method_name = None
error_detected = False
temp_fname = "tmp_rr_list.txt"


#if not error_detected:
if True:
    method_name = class_name + ".[comparison operators]"
    rrl1 = ldns.ldns_rr_list.new()
    rrl1.push_rr(ldns.ldns_rr.new_frm_str("test1 600 IN A 0.0.0.0"))
    rrl2 = ldns.ldns_rr_list.new()
    rrl2.push_rr(ldns.ldns_rr.new_frm_str("test2 600 IN A 1.1.1.1"))
    try:
        ret = rrl1 < rrl2
        if not isinstance(ret, bool):
            set_error()
        if ret != True:
            set_error()
    except:
        set_error()
    try:
        ret = rrl2 < rrl1
        if not isinstance(ret, bool):
            set_error()
        if ret != False:
            set_error()
    except:
        set_error()
    try:
        ret = rrl1 <= rrl2
        if not isinstance(ret, bool):
            set_error()
        if ret != True:
            set_error()
    except:
        set_error()
    try:
        ret = rrl2 <= rrl1
        if not isinstance(ret, bool):
            set_error()
        if ret != False:
            set_error()
    except:
        set_error()
    try:
        ret = rrl1 == rrl2
        if not isinstance(ret, bool):
            set_error()
        if ret != False:
            set_error()
    except:
        set_error()
    try:
        ret = rrl1 == rrl1
        if not isinstance(ret, bool):
            set_error()
        if ret != True:
            set_error()
    except:
        set_error()
    try:
        ret = rrl1 != rrl2
        if not isinstance(ret, bool):
            set_error()
        if ret != True:
            set_error()
    except:
        set_error()
    try:
        ret = rrl1 != rrl1
        if not isinstance(ret, bool):
            set_error()
        if ret != False:
            set_error()
    except:
        set_error()
    try:
        ret = rrl1 > rrl2
        if not isinstance(ret, bool):
            set_error()
        if ret != False:
            set_error()
    except:
        set_error()
    try:
        ret = rrl2 > rrl1
        if not isinstance(ret, bool):
            set_error()
        if ret != True:
            set_error()
    except:
        set_error()
    try:
        ret = rrl1 >= rrl2
        if not isinstance(ret, bool):
            set_error()
        if ret != False:
            set_error()
    except:
        set_error()
    try:
        ret = rrl2 >= rrl1
        if not isinstance(ret, bool):
            set_error()
        if ret != True:
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".cat()"
    rrl1 = ldns.ldns_rr_list.new()
    rrl2 = ldns.ldns_rr_list.new()
    rrl1.push_rr(ldns.ldns_rr.new_frm_str("test1 600 IN A 0.0.0.0"))
    rrl2.push_rr(ldns.ldns_rr.new_frm_str("test2 600 IN A 1.1.1.1"))
    try:
        ret = rrl1.cat(rrl2)
        if not isinstance(ret, bool):
            set_error()
        if ret != True:
            set_error()
    except:
        set_error()
    try:
        ret = rrl2.cat("")
        set_error()
    except TypeError:
        pass
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".cat_clone()"
    rrl1 = ldns.ldns_rr_list.new()
    rrl2 = ldns.ldns_rr_list.new()
    rrl1.push_rr(ldns.ldns_rr.new_frm_str("test1 600 IN A 0.0.0.0"))
    rrl2.push_rr(ldns.ldns_rr.new_frm_str("test2 600 IN A 1.1.1.1"))
    try:
        ret = rrl1.cat_clone(rrl2)
        if not isinstance(ret, ldns.ldns_rr_list):
            set_error()
    except:
        set_error()
    try:
        ret = rrl2.cat_clone("")
        set_error()
    except TypeError:
        pass
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".clone()"
    rrl = ldns.ldns_rr_list.new()
    rrl.push_rr(ldns.ldns_rr.new_frm_str("test1 600 IN A 0.0.0.0"))
    try:
        ret = rrl.clone()
        if not isinstance(ret, ldns.ldns_rr_list):
            set_error()
    except:
        set_error()

#if not error_detected:
if True:
    method_name = class_name + ".contains_rr()"
    rrl = ldns.ldns_rr_list.new()
    rr1 = ldns.ldns_rr.new_frm_str("test1 600 IN A 0.0.0.0")
    rr2 = ldns.ldns_rr.new_frm_str("test2 600 IN A 1.1.1.1")
    rrl.push_rr(rr1)
    try:
        ret = rrl.contains_rr(rr1)
        if not isinstance(ret, bool):
            set_error()
        if ret != True:
            set_error()
    except:
        set_error()
    try:
        ret = rrl.contains_rr(rr2)
        if not isinstance(ret, bool):
            set_error()
        if ret != False:
            set_error()
    except:
        set_error()
    try:
        ret = rrl.contains_rr("")
        set_error()
    except TypeError:
        pass
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".is_rrset()"
    rrl = ldns.ldns_rr_list.new()
    rr = ldns.ldns_rr.new_frm_str("test1 600 IN A 0.0.0.0")
    rrl.push_rr(rr)
    try:
        ret = rrl.is_rrset()
        if not isinstance(ret, bool):
            set_error()
        if ret != True:
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".new()"
    try:
        ret = ldns.ldns_rr_list.new()
        if not isinstance(ret, ldns.ldns_rr_list):
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".new_frm_file()"
    try:
        ret = ldns.ldns_rr_list.new_frm_file()
        if not isinstance(ret, ldns.ldns_rr_list):
            set_error()
    except:
        set_error()
    try:
        ret = ldns.ldns_rr_list.new_frm_file("test")
        set_error()
    except Exception:
        pass
    except:
        set_error()
    try:
        ret = ldns.ldns_rr_list.new_frm_file("test", raiseException=False)
        if isinstance(ret, ldns.ldns_rr_list):
            set_error()
        if ret != None:
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".owner()"
    rrl = ldns.ldns_rr_list.new()
    try:
        ret = rrl.owner()
        if isinstance(ret, ldns.ldns_rdf):
            set_error()
        if ret != None:
            set_error()
    except:
        set_error()
    rr = ldns.ldns_rr.new_frm_str("test1 600 IN A 0.0.0.0")
    rrl.push_rr(rr)
    try:
        ret = rrl.owner()
        if not isinstance(ret, ldns.ldns_dname):
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".pop_rr()"
    rrl = ldns.ldns_rr_list.new()
    rr = ldns.ldns_rr.new_frm_str("test1 600 IN A 0.0.0.0")
    rrl.push_rr(rr)
    try:
        ret = rrl.pop_rr()
        if not isinstance(ret, ldns.ldns_rr):
            set_error()
    except:
        set_error()
    try:
        ret = rrl.pop_rr()
        if isinstance(ret, ldns.ldns_rr):
            set_error()
        if ret != None:
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".pop_rr_list()"
    rrl = ldns.ldns_rr_list.new()
    rr = ldns.ldns_rr.new_frm_str("test1 600 IN A 0.0.0.0")
    rrl.push_rr(rr)
    rrl.push_rr(rr)
    rrl.push_rr(rr)
    try:
        ret = rrl.pop_rr_list(2)
        if not isinstance(ret, ldns.ldns_rr_list):
            set_error()
    except:
        set_error()
    try:
        ret = rrl.pop_rr_list(2)
        if not isinstance(ret, ldns.ldns_rr_list):
            set_error()
    except:
        set_error()
    try:
        ret = rrl.pop_rr_list(2)
        if isinstance(ret, ldns.ldns_rr_list):
            set_error()
        if ret != None:
            set_error()
    except:
        set_error()
    try:
        ret = rrl.pop_rr_list("")
        set_error()
    except TypeError:
        pass
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".pop_rrset()"
    rrl = ldns.ldns_rr_list.new()
    rr = ldns.ldns_rr.new_frm_str("test1 600 IN A 0.0.0.0")
    rrl.push_rr(rr)
    rrl.push_rr(rr)
    rrl.push_rr(rr)
    try:
        ret = rrl.pop_rrset()
        if not isinstance(ret, ldns.ldns_rr_list):
            set_error()
    except:
        set_error()
    try:
        ret = rrl.pop_rrset()
        if isinstance(ret, ldns.ldns_rr_list):
            set_error()
        if ret != None:
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".print_to_file()"
    rrl = ldns.ldns_rr_list.new()
    rr = ldns.ldns_rr.new_frm_str("test1 600 IN A 0.0.0.0")
    rrl.push_rr(rr)
    rrl.push_rr(rr)
    rrl.push_rr(rr)
    f = open(temp_fname, "w")
    try:
        rrl.print_to_file(f)
    except:
        set_error()
    f.close()
    f = open(temp_fname, "r")
    if len(f.readlines()) != 3:
        set_error()
    f.close()
    os.remove(temp_fname)


#if not error_detected:
if True:
    method_name = class_name + ".push_rr()"
    rrl = ldns.ldns_rr_list.new()
    rr = ldns.ldns_rr.new_frm_str("test1 600 IN A 0.0.0.0")
    try:
        ret = rrl.push_rr(rr)
        if not isinstance(ret, bool):
            set_error()
        if ret != True:
            set_error()
    except:
        set_error()
    try:
        ret = rrl.push_rr("")
        set_error()
    except TypeError:
        pass
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".push_rr_list()"
    rrl1 = ldns.ldns_rr_list.new()
    rr = ldns.ldns_rr.new_frm_str("test1 600 IN A 0.0.0.0")
    rrl1.push_rr(rr)
    rrl2 = rrl1.new()
    try:
        ret = rrl1.push_rr_list(rrl2)
        if not isinstance(ret, bool):
            set_error()
        if ret != True:
            set_error()
    except:
        set_error()
    try:
        ret = rrl.push_rr_list("")
        set_error()
    except TypeError:
        pass
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".rr()"
    rrl = ldns.ldns_rr_list.new()
    rr = ldns.ldns_rr.new_frm_str("test1 600 IN A 0.0.0.0")
    rrl.push_rr(rr)
    try:
        ret = rrl.rr(0)
        if not isinstance(ret, ldns.ldns_rr):
            set_error()
    except:
        set_error()
    try:
        ret = rrl.rr(1)
        if isinstance(ret, ldns.ldns_rr):
            set_error()
        if ret != None:
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".rr_count()"
    rrl = ldns.ldns_rr_list.new()
    try:
        ret = rrl.rr_count()
        if (not isinstance(ret, int)) and (not isinstance(ret, long)):
            set_error()
        if ret != 0:
            set_error()
    except:
        set_error()
    rr = ldns.ldns_rr.new_frm_str("test1 600 IN A 0.0.0.0")
    rrl.push_rr(rr)
    try:
        ret = rrl.rr_count()
        if (not isinstance(ret, int)) and (not isinstance(ret, long)):
            set_error()
        if ret != 1:
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".rrs()"
    rrl = ldns.ldns_rr_list.new()
    rr = ldns.ldns_rr.new_frm_str("test1 600 IN A 0.0.0.0")
    rrl.push_rr(rr)
    rrl.push_rr(rr)
    try:
        ret = list(rrl.rrs())
        if not isinstance(ret, list):
            set_error()
        if len(ret) != 2:
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".set_rr()"
    rrl = ldns.ldns_rr_list.new()
    rr = ldns.ldns_rr.new_frm_str("test1 600 IN A 0.0.0.0")
    rrl.push_rr(rr)
    rr = ldns.ldns_rr.new_frm_str("test2 600 IN A 1.1.1.1")
    ret = rrl.set_rr(rr, 0)
    try:
        ret = rrl.set_rr(rr, 0)
        if not isinstance(ret, ldns.ldns_rr):
            set_error()
    except:
        set_error()
    try:
        ret = rrl.set_rr(rr, 1)
        if isinstance(ret, ldns.ldns_rr):
            set_error()
        if ret != None:
            set_error()
    except:
        set_error()
    try:
        ret = rrl.set_rr("", 1)
        set_error()
    except TypeError:
        pass
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".set_rr_count()"
    rrl = ldns.ldns_rr_list.new()
    try:
        rrl.set_rr_count(0)
    except:
        set_error()
    rr = ldns.ldns_rr.new_frm_str("test1 600 IN A 0.0.0.0")
    rrl.push_rr(rr)
    try:
        rrl.set_rr_count("")
    except TypeError:
        pass
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".sort()"
    rrl = ldns.ldns_rr_list.new()
    try:
        rrl.sort()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".subtype_by_rdf()"
    rrl = ldns.ldns_rr_list.new()
    rr = ldns.ldns_rr.new_frm_str("test1 600 IN A 0.0.0.0")
    rrl.push_rr(rr)
    rr = ldns.ldns_rr.new_frm_str("test2 600 IN A 1.1.1.1")
    rrl.push_rr(rr)
    rr = ldns.ldns_rr.new_frm_str("test3 600 IN A 0.0.0.0")
    rrl.push_rr(rr)
    rdf = ldns.ldns_rdf.new_frm_str("0.0.0.0", ldns.LDNS_RDF_TYPE_A)
    try:
        ret = rrl.subtype_by_rdf(rdf, 0)
        if not isinstance(ret, ldns.ldns_rr_list):
            set_error()
        if ret.rr_count() != 2:
            set_error()
    except:
        set_error()
    rdf = ldns.ldns_rdf.new_frm_str("::", ldns.LDNS_RDF_TYPE_AAAA)
    try:
        ret = rrl.subtype_by_rdf(rdf, 0)
        if isinstance(ret, ldns.ldns_rr_list):
            set_error()
        if ret != None:
            set_error()
    except:
        set_error()
    try:
        ret = rrl.subtype_by_rdf("", 0)
        set_error()
    except TypeError:
        pass
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".to_canonical()"
    rrl = ldns.ldns_rr_list.new()
    rr = ldns.ldns_rr.new_frm_str("TEST1 600 IN A 0.0.0.0")
    rrl.push_rr(rr)
    try:
        rrl.to_canonical()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".type()"
    rrl = ldns.ldns_rr_list.new()
    try:
        ret = rrl.type()
        if not isinstance(ret, int):
            set_error()
        if ret != 0:
            set_error()
    except:
        set_error()
    rr = ldns.ldns_rr.new_frm_str("TEST1 600 IN A 0.0.0.0")
    rrl.push_rr(rr)
    try:
        ret = rrl.type()
        if not isinstance(ret, int):
            set_error()
        if ret != ldns.LDNS_RR_TYPE_A:
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".write_to_buffer()"
    sys.stderr.write("%s not tested.\n" % (method_name))


if not error_detected:
    sys.stdout.write("%s: passed.\n" % (os.path.basename(__file__)))
else:
    sys.stdout.write("%s: errors detected.\n" % (os.path.basename(__file__)))
    sys.exit(1)
