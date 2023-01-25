#!/usr/bin/env python

#
# ldns_resolver testing script.
#
# Do not use constructs that differ between Python 2 and 3.
# Use write on stdout or stderr.
#


import ldns
import sys
import os
import inspect


class_name = "ldns_resolver"
method_name = None
error_detected = False
temp_fname = "tmp_resolver.txt"


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
    method_name = class_name + ".axfr_complete()"
    sys.stderr.write("%s not tested.\n" % (method_name))


#if not error_detected:
if True:
    method_name = class_name + ".axfr_last_pkt()"
    sys.stderr.write("%s not tested.\n" % (method_name))


#if not error_detected:
if True:
    method_name = class_name + ".axfr_next()"
    sys.stderr.write("%s not tested.\n" % (method_name))


#if not error_detected:
if True:
    method_name = class_name + ".axfr_start()"
    sys.stderr.write("%s not tested.\n" % (method_name))


#if not error_detected:
if True:
    method_name = class_name + ".debug()"
    resolver = ldns.ldns_resolver.new_frm_file("/etc/resolv.conf")
    resolver.set_debug(False)
    try:
        ret = resolver.debug()
        if not isinstance(ret, bool):
            set_error()
        if ret != False:
            set_error()
    except:
        set_error()
    resolver.set_debug(True)
    try:
        ret = resolver.debug()
        if not isinstance(ret, bool):
            set_error()
        if ret != True:
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".dec_nameserver_count()"
    resolver = ldns.ldns_resolver.new_frm_file("/etc/resolv.conf")
    cnt = resolver.nameserver_count()
    try:
        resolver.dec_nameserver_count()
    except:
        set_error()
    if cnt != (resolver.nameserver_count() + 1):
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".defnames()"
    resolver = ldns.ldns_resolver.new_frm_file("/etc/resolv.conf")
    resolver.set_defnames(False)
    try:
        ret = resolver.defnames()
        if not isinstance(ret, bool):
            set_error()
        if ret != False:
            set_error()
    except:
        set_error()
    resolver.set_defnames(True)
    try:
        ret = resolver.defnames()
        if not isinstance(ret, bool):
            set_error()
        if ret != True:
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".dnsrch()"
    resolver = ldns.ldns_resolver.new_frm_file("/etc/resolv.conf")
    resolver.set_dnsrch(False)
    try:
        ret = resolver.dnsrch()
        if not isinstance(ret, bool):
            set_error()
        if ret != False:
            set_error()
    except:
        set_error()
    resolver.set_dnsrch(True)
    try:
        ret = resolver.dnsrch()
        if not isinstance(ret, bool):
            set_error()
        if ret != True:
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".dnssec()"
    resolver = ldns.ldns_resolver.new_frm_file("/etc/resolv.conf")
    resolver.set_dnssec(False)
    try:
        ret = resolver.dnssec()
        if not isinstance(ret, bool):
            set_error()
        if ret != False:
            set_error()
    except:
        set_error()
    resolver.set_dnssec(True)
    try:
        ret = resolver.dnssec()
        if not isinstance(ret, bool):
            set_error()
        if ret != True:
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".dnssec_anchors()"
    resolver = ldns.ldns_resolver.new_frm_file("/etc/resolv.conf")
    rrl = ldns.ldns_rr_list.new()
    try:
        ret = resolver.dnssec_anchors()
        if ret != None:
            set_error()
    except:
        set_error()
    resolver.set_dnssec_anchors(rrl)
    try:
        ret = resolver.dnssec_anchors()
        if not isinstance(ret, ldns.ldns_rr_list):
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".dnssec_cd()"
    resolver = ldns.ldns_resolver.new_frm_file("/etc/resolv.conf")
    resolver.set_dnssec_cd(False)
    try:
        ret = resolver.dnssec_cd()
        if not isinstance(ret, bool):
            set_error()
        if ret != False:
            set_error()
    except:
        set_error()
    resolver.set_dnssec_cd(True)
    try:
        ret = resolver.dnssec_cd()
        if not isinstance(ret, bool):
            set_error()
        if ret != True:
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".domain()"
    resolver = ldns.ldns_resolver.new_frm_file("/etc/resolv.conf")
    resolver.set_domain(None)
    try:
        ret = resolver.domain()
        if ret != None:
            set_error()
    except:
        set_error()
    dname = ldns.ldns_dname("example.com.")
    resolver.set_domain(dname)
    try:
        ret = resolver.domain()
        if not isinstance(ret, ldns.ldns_dname):
            set_error()
        if ret != dname:
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".edns_udp_size()"
    try:
        resolver = ldns.ldns_resolver.new()
        if not isinstance(resolver, ldns.ldns_resolver):
            set_error()
    except:
        set_error()



#if not error_detected:
if True:
    method_name = class_name + ".edns_udp_size()"
    resolver = ldns.ldns_resolver.new_frm_file("/etc/resolv.conf")
    resolver.set_edns_udp_size(4096)
    try:
        ret = resolver.edns_udp_size()
        if (not isinstance(ret, int)) and (not isinstance(ret, long)):
            set_error()
        if ret != 4096:
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".fail()"
    resolver = ldns.ldns_resolver.new_frm_file("/etc/resolv.conf")
    resolver.set_fail(False)
    try:
        ret = resolver.fail()
        if not isinstance(ret, bool):
            set_error()
        if ret != False:
            set_error()
    except:
        set_error()
    resolver.set_fail(True)
    try:
        ret = resolver.fail()
        if not isinstance(ret, bool):
            set_error()
        if ret != True:
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".fallback()"
    resolver = ldns.ldns_resolver.new_frm_file("/etc/resolv.conf")
    resolver.set_fallback(False)
    try:
        ret = resolver.fallback()
        if not isinstance(ret, bool):
            set_error()
        if ret != False:
            set_error()
    except:
        set_error()
    resolver.set_fallback(True)
    try:
        ret = resolver.fallback()
        if not isinstance(ret, bool):
            set_error()
        if ret != True:
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".get_addr_by_name()"
    resolver = ldns.ldns_resolver.new_frm_file("/etc/resolv.conf")
    try:
        ret = resolver.get_addr_by_name("www.google.com", ldns.LDNS_RR_CLASS_IN, ldns.LDNS_RD)
        if not isinstance(ret, ldns.ldns_rr_list):
            set_error()
    except:
        set_error()
    try:
        ret = resolver.get_addr_by_name(1, ldns.LDNS_RR_CLASS_IN, ldns.LDNS_RD)
        set_error()
    except TypeError as e:
        pass
    except:
        set_error()
    try:
        ret = resolver.get_addr_by_name("www.google.com", "bad argument", ldns.LDNS_RD)
        set_error()
    except TypeError as e:
        pass
    except:
        set_error()
    try:
        ret = resolver.get_addr_by_name("www.google.com", ldns.LDNS_RR_CLASS_IN, "bad argument")
        set_error()
    except TypeError as e:
        pass
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".get_name_by_addr()"
    resolver = ldns.ldns_resolver.new_frm_file("/etc/resolv.conf")
    try:
        addr = resolver.get_name_by_addr("8.8.8.8", ldns.LDNS_RR_CLASS_IN, ldns.LDNS_RD)
        if not isinstance(addr, ldns.ldns_rr_list):
            set_error()
    except:
        set_error()
    try:
        addr = resolver.get_name_by_addr(1, ldns.LDNS_RR_CLASS_IN, ldns.LDNS_RD)
        set_error()
    except TypeError as e:
        pass
    except:
        set_error()
    try:
        addr = resolver.get_name_by_addr("8.8.8.8", "bad argument", ldns.LDNS_RD)
        set_error()
    except TypeError as e:
        pass
    except:
        set_error()
    try:
        addr = resolver.get_name_by_addr("8.8.8.8", ldns.LDNS_RR_CLASS_IN, "bad argument")
        set_error()
    except TypeError as e:
        pass
    except:
        set_error()



#if not error_detected:
if True:
    method_name = class_name + ".igntc()"
    resolver = ldns.ldns_resolver.new_frm_file("/etc/resolv.conf")
    resolver.set_igntc(False)
    try:
        ret = resolver.igntc()
        if not isinstance(ret, bool):
            set_error()
        if ret != False:
            set_error()
    except:
        set_error()
    resolver.set_igntc(True)
    try:
        ret = resolver.igntc()
        if not isinstance(ret, bool):
            set_error()
        if ret != True:
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".incr_nameserver_count()"
    resolver = ldns.ldns_resolver.new_frm_file("/etc/resolv.conf")
    cnt = resolver.nameserver_count()
    try:
        resolver.incr_nameserver_count()
    except:
        set_error()
    if (cnt + 1) != resolver.nameserver_count():
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".ip6()"
    resolver = ldns.ldns_resolver.new_frm_file("/etc/resolv.conf")
    resolver.set_ip6(0)
    try:
        ret = resolver.ip6()
        if (not isinstance(ret, int)) and (not isinstance(ret, long)):
            set_error()
        if ret != 0:
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".nameserver_count()"
    resolver = ldns.ldns_resolver.new_frm_file("/etc/resolv.conf")
    resolver.set_nameserver_count(1)
    try:
        ret = resolver.nameserver_count()
        if (not isinstance(ret, int)) and (not isinstance(ret, long)):
            set_error()
        if ret != 1:
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".nameserver_rtt()"
    resolver = ldns.ldns_resolver.new_frm_file("/etc/resolv.conf")
    num = resolver.nameserver_count()
    for i in range(0, num):
        resolver.set_nameserver_rtt(i, i + 1)
    try:
        for i in range(0, num):
            ret = resolver.nameserver_rtt(i)
            if (not isinstance(ret, int)) and (not isinstance(ret, long)):
                set_error()
            if (i + 1) != ret:
                set_error()
    except:
        set_error()
    try:
        ret = resolver.nameserver_rtt("bad argument")
        set_error()
    except TypeError as e:
        pass
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".nameservers()"
    sys.stderr.write("%s not tested.\n" % (method_name))


#if not error_detected:
if True:
    method_name = class_name + ".nameservers_randomize()"
    resolver = ldns.ldns_resolver.new_frm_file("/etc/resolv.conf")
    try:
        resolver.nameservers_randomize()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".new_frm_file()"
    try:
        ret = ldns.ldns_resolver.new_frm_file("/etc/resolv.conf", raiseException=True)
        if not isinstance(ret, ldns.ldns_resolver):
            set_error()
    except:
        set_error()
    try:
        ret = ldns.ldns_resolver.new_frm_file(1, raiseException=True)
        set_error()
    except TypeError as e:
        pass
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".new_frm_fp()"
    fi = open("/etc/resolv.conf")
    try:
        ret = ldns.ldns_resolver.new_frm_fp(fi, raiseException=True)
        if not isinstance(ret, ldns.ldns_resolver):
            set_error()
    except:
        set_error()
    fi.close()
    try:
        ret = ldns.ldns_resolver.new_frm_fp(1, raiseException=True)
        set_error()
    except TypeError as e:
        pass
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".new_frm_fp_l()"
    fi = open("/etc/resolv.conf")
    try:
        ret, line = ldns.ldns_resolver.new_frm_fp_l(fi, raiseException=True)
        if not isinstance(ret, ldns.ldns_resolver):
            set_error()
        if (not isinstance(line, int)) and (not isinstance(line, long)):
            set_error()
    except:
        set_error()
    fi.close()
    try:
        ret, line = ldns.ldns_resolver.new_frm_fp_l(1, raiseException=True)
        set_error()
    except TypeError as e:
        pass
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".pop_nameserver()"
    resolver = ldns.ldns_resolver.new_frm_file("/etc/resolv.conf")
    cnt = resolver.nameserver_count()
    try:
        for i in range(0, cnt):
            ret = resolver.pop_nameserver()
            if not isinstance(ret, ldns.ldns_rdf):
                set_error()
    except:
        set_error()
    try:
        ret = resolver.pop_nameserver()
        if ret != None:
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".port()"
    resolver = ldns.ldns_resolver.new_frm_file("/etc/resolv.conf")
    resolver.set_port(12345)
    try:
        ret = resolver.port()
        if (not isinstance(ret, int)) and (not isinstance(ret, long)):
            set_error()
        if ret != 12345:
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".prepare_query_pkt()"
    resolver = ldns.ldns_resolver.new_frm_file("/etc/resolv.conf")
    try:
        ret = resolver.prepare_query_pkt("example.com.", ldns.LDNS_RR_TYPE_A, ldns.LDNS_RR_CLASS_IN, ldns.LDNS_RD, raiseException=True)
        if not isinstance(ret, ldns.ldns_pkt):
            set_error()
    except:
        set_error()
    try:
        ret = resolver.prepare_query_pkt(1, ldns.LDNS_RR_TYPE_A, ldns.LDNS_RR_CLASS_IN, ldns.LDNS_RD, raiseException=True)
        set_error()
    except TypeError as e:
        pass
    except:
        set_error()
    try:
        ret = resolver.prepare_query_pkt("example.com.", "bad argument", ldns.LDNS_RR_CLASS_IN, ldns.LDNS_RD, raiseException=True)
        set_error()
    except TypeError as e:
        pass
    except:
        set_error()
    try:
        ret = resolver.prepare_query_pkt("example.com.", ldns.LDNS_RR_TYPE_A, "bad argument", ldns.LDNS_RD, raiseException=True)
        set_error()
    except TypeError as e:
        pass
    except:
        set_error()
    try:
        ret = resolver.prepare_query_pkt("example.com.", ldns.LDNS_RR_TYPE_A, ldns.LDNS_RR_CLASS_IN, "bad argument", raiseException=True)
        set_error()
    except TypeError as e:
        pass
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".push_dnssec_anchor()"
    resolver = ldns.ldns_resolver.new_frm_file("/etc/resolv.conf")
    key = ldns.ldns_key.new_frm_algorithm(ldns.LDNS_SIGN_DSA, 512)
    domain = ldns.ldns_dname("example.")
    key.set_pubkey_owner(domain)
    pubkey = key.key_to_rr()
    ds = ldns.ldns_key_rr2ds(pubkey, ldns.LDNS_SHA1)
    try:
        ret = resolver.push_dnssec_anchor(ds)
        if ret != ldns.LDNS_STATUS_OK:
            set_error()
    except:
        set_error()
    rr = ldns.ldns_rr.new_frm_str("test1 600 IN A 0.0.0.0")
    try:
        ret = resolver.push_dnssec_anchor(rr)
        if ret == ldns.LDNS_STATUS_OK:
            set_error()
    except:
        set_error()
    try:
        ret = resolver.push_dnssec_anchor("bad argument")
        set_error()
    except TypeError as e:
        pass
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".push_nameserver()"
    resolver = ldns.ldns_resolver.new_frm_file("/etc/resolv.conf")
    rdf = ldns.ldns_rdf.new_frm_str("127.0.0.1", ldns.LDNS_RDF_TYPE_A)
    try:
        ret = resolver.push_nameserver(rdf)
        if ret != ldns.LDNS_STATUS_OK:
            set_error()
    except:
        set_error()
    rdf = ldns.ldns_rdf.new_frm_str("::1", ldns.LDNS_RDF_TYPE_AAAA)
    try:
        ret = resolver.push_nameserver(rdf)
        if ret != ldns.LDNS_STATUS_OK:
            set_error()
    except:
        set_error()
    rdf = ldns.ldns_rdf.new_frm_str("example.com.", ldns.LDNS_RDF_TYPE_DNAME)
    try:
        ret = resolver.push_nameserver(rdf)
        if ret == ldns.LDNS_STATUS_OK:
            set_error()
    except:
        set_error()
    try:
        ret = resolver.push_nameserver("bad argument")
        set_error()
    except TypeError as e:
        pass
    except:
        set_error()

#if not error_detected:
if True:
    method_name = class_name + ".push_nameserver_rr()"
    resolver = ldns.ldns_resolver.new_frm_file("/etc/resolv.conf")
    rr = ldns.ldns_rr.new_frm_str("test 600 IN A 127.0.0.1")
    try:
        ret = resolver.push_nameserver_rr(rr)
        if ret != ldns.LDNS_STATUS_OK:
            set_error()
    except:
        set_error()
    rr = ldns.ldns_rr.new_frm_str("test 600 IN AAAA ::1")
    try:
        ret = resolver.push_nameserver_rr(rr)
        if ret != ldns.LDNS_STATUS_OK:
            set_error()
    except:
        set_error()
    rr = ldns.ldns_rr.new_frm_str("test 600 IN NS 8.8.8.8")
    try:
        ret = resolver.push_nameserver_rr(rr)
        if ret == ldns.LDNS_STATUS_OK:
            set_error()
    except:
        set_error()
    try:
        ret = resolver.push_nameserver_rr("bad argument")
        set_error()
    except TypeError as e:
        pass
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".push_nameserver_rr_list()"
    resolver = ldns.ldns_resolver.new_frm_file("/etc/resolv.conf")
    rrl = ldns.ldns_rr_list.new()
    rr = ldns.ldns_rr.new_frm_str("test 600 IN A 127.0.0.1")
    rrl.push_rr(rr)
    try:
        ret = resolver.push_nameserver_rr_list(rrl)
        if ret != ldns.LDNS_STATUS_OK:
            set_error()
    except:
        set_error()
    rrl = ldns.ldns_rr_list.new()
    rr = ldns.ldns_rr.new_frm_str("test 600 IN AAAA ::1")
    rrl.push_rr(rr)
    try:
        ret = resolver.push_nameserver_rr_list(rrl)
        if ret != ldns.LDNS_STATUS_OK:
            set_error()
    except:
        set_error()
    rrl = ldns.ldns_rr_list.new()
    rr = ldns.ldns_rr.new_frm_str("test 600 IN NS 8.8.8.8")
    rrl.push_rr(rr)
    try:
        ret = resolver.push_nameserver_rr_list(rrl)
        if ret == ldns.LDNS_STATUS_OK:
            set_error()
    except:
        set_error()
    try:
        ret = resolver.push_nameserver_rr_list("bad argument")
        set_error()
    except TypeError as e:
        pass
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".push_searchlist()"
    resolver = ldns.ldns_resolver.new_frm_file("/etc/resolv.conf")
    resolver.push_searchlist("example.com.")
    try:
        resolver.push_searchlist("example.com.")
    except:
        set_error()
    try:
        resolver.push_searchlist(1)
        set_error()
    except TypeError as e:
        pass
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".query()"
    resolver = ldns.ldns_resolver.new_frm_file("/etc/resolv.conf")
    try:
        ret = resolver.query("www.nic.cz", ldns.LDNS_RR_TYPE_A, ldns.LDNS_RR_CLASS_IN, ldns.LDNS_RD)
        if not isinstance(ret, ldns.ldns_pkt):
            set_error()
    except:
        set_error()
    try:
        ret = resolver.query(1, ldns.LDNS_RR_TYPE_A, ldns.LDNS_RR_CLASS_IN, ldns.LDNS_RD)
        set_error()
    except TypeError as e:
        pass
    except:
        set_error()
    try:
        ret = resolver.query("www.nic.cz", "bad argument", ldns.LDNS_RR_CLASS_IN, ldns.LDNS_RD)
        set_error()
    except TypeError as e:
        pass
    except:
        set_error()
    try:
        ret = resolver.query("www.nic.cz", ldns.LDNS_RR_TYPE_A, "bad argument", ldns.LDNS_RD)
        set_error()
    except TypeError as e:
        pass
    except:
        set_error()
    try:
        ret = resolver.query("www.nic.cz", ldns.LDNS_RR_TYPE_A, ldns.LDNS_RR_CLASS_IN, "bad argument")
        set_error()
    except TypeError as e:
        pass
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".random()"
    resolver = ldns.ldns_resolver.new_frm_file("/etc/resolv.conf")
    resolver.set_random(False)
    try:
        ret = resolver.random()
        if not isinstance(ret, bool):
            set_error()
        if ret != False:
            set_error()
    except:
        set_error()
    resolver.set_random(True)
    try:
        ret = resolver.random()
        if not isinstance(ret, bool):
            set_error()
        if ret != True:
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".recursive()"
    resolver = ldns.ldns_resolver.new_frm_file("/etc/resolv.conf")
    resolver.set_recursive(False)
    try:
        ret = resolver.recursive()
        if not isinstance(ret, bool):
            set_error()
        if ret != False:
            set_error()
    except:
        set_error()
    resolver.set_recursive(True)
    try:
        ret = resolver.recursive()
        if not isinstance(ret, bool):
            set_error()
        if ret != True:
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".retrans()"
    resolver = ldns.ldns_resolver.new_frm_file("/etc/resolv.conf")
    resolver.set_retrans(127)
    try:
        ret = resolver.retrans()
        if (not isinstance(ret, int)) and (not isinstance(ret, long)):
            set_error()
        if ret != 127:
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".retry()"
    resolver = ldns.ldns_resolver.new_frm_file("/etc/resolv.conf")
    resolver.set_retry(4)
    try:
        ret = resolver.retry()
        if (not isinstance(ret, int)) and (not isinstance(ret, long)):
            set_error()
        if ret != 4:
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".rtt()"
    sys.stderr.write("%s not tested.\n" % (method_name))


#if not error_detected:
if True:
    method_name = class_name + ".search()"
    resolver = ldns.ldns_resolver.new_frm_file("/etc/resolv.conf")
    try:
        ret = resolver.search("www.nic.cz", ldns.LDNS_RR_TYPE_A, ldns.LDNS_RR_CLASS_IN, ldns.LDNS_RD)
        if not isinstance(ret, ldns.ldns_pkt):
            set_error()
    except:
        set_error()
    try:
        ret = resolver.search(1, ldns.LDNS_RR_TYPE_A, ldns.LDNS_RR_CLASS_IN, ldns.LDNS_RD)
        set_error()
    except TypeError as e:
        pass
    except:
        set_error()
    try:
        ret = resolver.search("www.nic.cz", "bad argument", ldns.LDNS_RR_CLASS_IN, ldns.LDNS_RD)
        set_error()
    except TypeError as e:
        pass
    except:
        set_error()
    try:
        ret = resolver.search("www.nic.cz", ldns.LDNS_RR_TYPE_A, "bad argument", ldns.LDNS_RD)
        set_error()
    except TypeError as e:
        pass
    except:
        set_error()
    try:
        ret = resolver.search("www.nic.cz", ldns.LDNS_RR_TYPE_A, ldns.LDNS_RR_CLASS_IN, "bad argument")
        set_error()
    except TypeError as e:
        pass
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".searchlist()"
    sys.stderr.write("%s not tested.\n" % (method_name))


#if not error_detected:
if True:
    method_name = class_name + ".searchlist_count()"
    resolver = ldns.ldns_resolver.new_frm_file("/etc/resolv.conf")
    try:
        ret = resolver.searchlist_count()
        if (not isinstance(ret, int)) and (not isinstance(ret, long)):
            set_error()
        if ret != 0:
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".send()"
    resolver = ldns.ldns_resolver.new_frm_file("/etc/resolv.conf")
    try:
        ret = resolver.send("www.nic.cz", ldns.LDNS_RR_TYPE_A, ldns.LDNS_RR_CLASS_IN, ldns.LDNS_RD)
        if not isinstance(ret, ldns.ldns_pkt):
            set_error()
    except:
        set_error()
    try:
        ret = resolver.send(1, ldns.LDNS_RR_TYPE_A, ldns.LDNS_RR_CLASS_IN, ldns.LDNS_RD)
        set_error()
    except TypeError as e:
        pass
    except:
        set_error()
    try:
        ret = resolver.send("www.nic.cz", "bad argument", ldns.LDNS_RR_CLASS_IN, ldns.LDNS_RD)
        set_error()
    except TypeError as e:
        pass
    except:
        set_error()
    try:
        ret = resolver.send("www.nic.cz", ldns.LDNS_RR_TYPE_A, "bad argument", ldns.LDNS_RD)
        set_error()
    except TypeError as e:
        pass
    except:
        set_error()
    try:
        ret = resolver.send("www.nic.cz", ldns.LDNS_RR_TYPE_A, ldns.LDNS_RR_CLASS_IN, "bad argument")
        set_error()
    except TypeError as e:
        pass
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".send_pkt()"
    resolver = ldns.ldns_resolver.new_frm_file("/etc/resolv.conf")
    pkt = ldns.ldns_pkt.new_query_frm_str("test.nic.cz",ldns.LDNS_RR_TYPE_ANY, ldns.LDNS_RR_CLASS_IN, ldns.LDNS_RD | ldns.LDNS_AD)
    try:
        status, ret = resolver.send_pkt(pkt)
        if status != ldns.LDNS_STATUS_OK:
            ste_error()
        if not isinstance(ret, ldns.ldns_pkt):
            set_error()
    except:
        set_error()
    try:
        status, ret = resolver.send_pkt("bad argument")
        set_error()
    except TypeError as e:
        pass
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".set_debug()"
    resolver = ldns.ldns_resolver.new_frm_file("/etc/resolv.conf")
    try:
        resolver.set_debug(False)
        ret = resolver.debug()
        if not isinstance(ret, bool):
            set_error()
        if ret != False:
            set_error()
    except:
        set_error()
    try:
        resolver.set_debug(True)
        ret = resolver.debug()
        if not isinstance(ret, bool):
            set_error()
        if ret != True:
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".set_defnames()"
    resolver = ldns.ldns_resolver.new_frm_file("/etc/resolv.conf")
    try:
        resolver.set_defnames(False)
        ret = resolver.defnames()
        if not isinstance(ret, bool):
            set_error()
        if ret != False:
            set_error()
    except:
        set_error()
    try:
        resolver.set_defnames(True)
        ret = resolver.defnames()
        if not isinstance(ret, bool):
            set_error()
        if ret != True:
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".set_dnsrch()"
    resolver = ldns.ldns_resolver.new_frm_file("/etc/resolv.conf")
    try:
        resolver.set_dnsrch(False)
        ret = resolver.dnsrch()
        if not isinstance(ret, bool):
            set_error()
        if ret != False:
            set_error()
    except:
        set_error()
    try:
        resolver.set_dnsrch(True)
        ret = resolver.dnsrch()
        if not isinstance(ret, bool):
            set_error()
        if ret != True:
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".set_dnssec()"
    resolver = ldns.ldns_resolver.new_frm_file("/etc/resolv.conf")
    try:
        resolver.set_dnssec(False)
        ret = resolver.dnssec()
        if not isinstance(ret, bool):
            set_error()
        if ret != False:
            set_error()
    except:
        set_error()
    try:
        resolver.set_dnssec(True)
        ret = resolver.dnssec()
        if not isinstance(ret, bool):
            set_error()
        if ret != True:
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".set_dnssec_anchors()"
    resolver = ldns.ldns_resolver.new_frm_file("/etc/resolv.conf")
    rrl = ldns.ldns_rr_list.new()
    try:
        resolver.set_dnssec_anchors(rrl)
        ret = resolver.dnssec_anchors()
        if not isinstance(ret, ldns.ldns_rr_list):
            set_error()
    except:
        set_error()
    try:
        resolver.set_dnssec_anchors("bad argument")
        set_error()
    except TypeError as e:
        pass
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".set_dnssec_cd()"
    resolver = ldns.ldns_resolver.new_frm_file("/etc/resolv.conf")
    try:
        resolver.set_dnssec_cd(False)
        ret = resolver.dnssec_cd()
        if not isinstance(ret, bool):
            set_error()
        if ret != False:
            set_error()
    except:
        set_error()
    try:
        resolver.set_dnssec_cd(True)
        ret = resolver.dnssec_cd()
        if not isinstance(ret, bool):
            set_error()
        if ret != True:
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".set_domain()"
    resolver = ldns.ldns_resolver.new_frm_file("/etc/resolv.conf")
    try:
        resolver.set_domain(None)
        ret = resolver.domain()
        if ret != None:
            set_error()
    except:
        set_error()
    dname = ldns.ldns_dname("example.com.")
    try:
        resolver.set_domain(dname)
        ret = resolver.domain()
        if not isinstance(ret, ldns.ldns_dname):
            set_error()
        if ret != dname:
            set_error()
    except:
        set_error()
    rdf = ldns.ldns_rdf.new_frm_str("example.com.", ldns.LDNS_RDF_TYPE_DNAME)
    try:
        resolver.set_domain(rdf)
        ret = resolver.domain()
        if not isinstance(ret, ldns.ldns_rdf):
            set_error()
        if ret != dname:
            set_error()
    except:
        set_error()
    resolver.set_domain("example.com.")
    try:
        resolver.set_domain("example.com.")
        ret = resolver.domain()
        if not isinstance(ret, ldns.ldns_dname):
            set_error()
        if ret != dname:
            set_error()
    except:
        set_error()
    rdf = ldns.ldns_rdf.new_frm_str("127.0.0.1", ldns.LDNS_RDF_TYPE_A)
    try:
        resolver.set_domain(rdf)
        set_error()
    except Exception as e:
        pass
    except:
        set_error()
    try:
        resolver.set_domain(1)
        set_error()
    except TypeError as e:
        pass
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".set_edns_udp_size()"
    resolver = ldns.ldns_resolver.new_frm_file("/etc/resolv.conf")
    try:
        resolver.set_edns_udp_size(4096)
        ret = resolver.edns_udp_size()
        if (not isinstance(ret, int)) and (not isinstance(ret, long)):
            set_error()
        if ret != 4096:
            set_error()
    except:
        set_error()
    try:
        resolver.set_edns_udp_size("bad argument")
        set_error()
    except TypeError as e:
        pass
    except:
        ste_error()


#if not error_detected:
if True:
    method_name = class_name + ".set_fail()"
    resolver = ldns.ldns_resolver.new_frm_file("/etc/resolv.conf")
    try:
        resolver.set_fail(False)
        ret = resolver.fail()
        if not isinstance(ret, bool):
            set_error()
        if ret != False:
            set_error()
    except:
        set_error()
    try:
        resolver.set_fail(True)
        ret = resolver.fail()
        if not isinstance(ret, bool):
            set_error()
        if ret != True:
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".set_fallback()"
    resolver = ldns.ldns_resolver.new_frm_file("/etc/resolv.conf")
    try:
        resolver.set_fallback(False)
        ret = resolver.fallback()
        if not isinstance(ret, bool):
            set_error()
        if ret != False:
            set_error()
    except:
        set_error()
    try:
        resolver.set_fallback(True)
        ret = resolver.fallback()
        if not isinstance(ret, bool):
            set_error()
        if ret != True:
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".set_igntc()"
    resolver = ldns.ldns_resolver.new_frm_file("/etc/resolv.conf")
    try:
        resolver.set_igntc(False)
        ret = resolver.igntc()
        if not isinstance(ret, bool):
            set_error()
        if ret != False:
            set_error()
    except:
        set_error()
    try:
        resolver.set_igntc(True)
        ret = resolver.igntc()
        if not isinstance(ret, bool):
            set_error()
        if ret != True:
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".set_ip6()"
    resolver = ldns.ldns_resolver.new_frm_file("/etc/resolv.conf")
    try:
        resolver.set_ip6(1)
        ret = resolver.ip6()
        if (not isinstance(ret, int)) and (not isinstance(ret, long)):
            set_error()
        if ret != 1:
            set_error()
    except:
        set_error()
    try:
        resolver.set_ip6("bad argument")
        set_error()
    except TypeError as e:
        pass
    except:
        ste_error()


#if not error_detected:
if True:
    method_name = class_name + ".set_nameserver_count()"
    resolver = ldns.ldns_resolver.new_frm_file("/etc/resolv.conf")
    try:
        resolver.set_nameserver_count(2)
        ret = resolver.nameserver_count()
        if (not isinstance(ret, int)) and (not isinstance(ret, long)):
            set_error()
        if ret != 2:
            set_error()
    except:
        set_error()
    try:
        resolver.set_nameserver_count("bad argument")
        set_error()
    except TypeError as e:
        pass
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".set_nameserver_rtt()"
    resolver = ldns.ldns_resolver.new_frm_file("/etc/resolv.conf")
    num = resolver.nameserver_count()
    try:
        for i in range(0, num):
            resolver.set_nameserver_rtt(i, i + 1)
            ret = resolver.nameserver_rtt(i)
            if (not isinstance(ret, int)) and (not isinstance(ret, long)):
                set_error()
            if (i + 1) != ret:
                set_error()
    except:
        set_error()
    try:
        ret = resolver.set_nameserver_rtt("bad argument", 0)
        set_error()
    except TypeError as e:
        pass
    except:
        set_error()
    try:
        ret = resolver.set_nameserver_rtt(0, "bad argument")
        set_error()
    except TypeError as e:
        pass
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".set_nameservers()"
    sys.stderr.write("%s not tested.\n" % (method_name))


#if not error_detected:
if True:
    method_name = class_name + ".set_port()"
    resolver = ldns.ldns_resolver.new_frm_file("/etc/resolv.conf")
    try:
        resolver.set_port(12345)
        ret = resolver.port()
        if (not isinstance(ret, int)) and (not isinstance(ret, long)):
            set_error()
        if ret != 12345:
            set_error()
    except:
        set_error()
    try:
        resolver.set_port("bad argument")
        set_error()
    except TypeError as e:
        pass
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".set_random()"
    resolver = ldns.ldns_resolver.new_frm_file("/etc/resolv.conf")
    try:
        resolver.set_random(False)
        ret = resolver.random()
        if not isinstance(ret, bool):
            set_error()
        if ret != False:
            set_error()
    except:
        set_error()
    try:
        resolver.set_random(True)
        ret = resolver.random()
        if not isinstance(ret, bool):
            set_error()
        if ret != True:
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".set_recursive()"
    resolver = ldns.ldns_resolver.new_frm_file("/etc/resolv.conf")
    try:
        resolver.set_recursive(False)
        ret = resolver.recursive()
        if not isinstance(ret, bool):
            set_error()
        if ret != False:
            set_error()
    except:
        set_error()
    try:
        resolver.set_recursive(True)
        ret = resolver.recursive()
        if not isinstance(ret, bool):
            set_error()
        if ret != True:
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".set_retrans()"
    resolver = ldns.ldns_resolver.new_frm_file("/etc/resolv.conf")
    try:
        resolver.set_retrans(127)
        ret = resolver.retrans()
        if (not isinstance(ret, int)) and (not isinstance(ret, long)):
            set_error()
        if ret != 127:
            set_error()
    except:
        set_error()
    try:
        resolver.set_retrans("bad argument")
        set_error()
    except TypeError as e:
        pass
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".set_retry()"
    resolver = ldns.ldns_resolver.new_frm_file("/etc/resolv.conf")
    try:
        resolver.set_retry(4)
        ret = resolver.retry()
        if (not isinstance(ret, int)) and (not isinstance(ret, long)):
            set_error()
        if ret != 4:
            set_error()
    except:
        set_error()
    try:
        resolver.set_retry("bad argument")
        set_error()
    except TypeError as e:
        pass
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".set_rtt()"
    sys.stderr.write("%s not tested.\n" % (method_name))


#if not error_detected:
if True:
    method_name = class_name + ".set_timeout()"
    sys.stderr.write("%s not tested.\n" % (method_name))


#if not error_detected:
if True:
    method_name = class_name + ".set_tsig_algorithm()"
    resolver = ldns.ldns_resolver.new_frm_file("/etc/resolv.conf")
    tsigstr = "hmac-md5.sig-alg.reg.int."
    try:
        resolver.set_tsig_algorithm(tsigstr)
        ret = resolver.tsig_algorithm()
        if not isinstance(ret, str):
            set_error()
        if ret != tsigstr:
            set_error()
    except:
        set_error()
    try:
        resolver.set_tsig_algorithm(1)
        set_error()
    except TypeError as e:
        pass
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".set_tsig_keydata()"
    resolver = ldns.ldns_resolver.new_frm_file("/etc/resolv.conf")
    tkdstr = "Humpty Dumpty sat on a wall, Humpty Dumpty had a great fall, All the King's horses and all the King's men, Couldn't put Humpty together again."
    try:
        resolver.set_tsig_keydata(tkdstr)
        ret = resolver.tsig_keydata()
        if not isinstance(ret, str):
            set_error()
        if ret != tkdstr:
            set_error()
    except:
        set_error()
    try:
        resolver.set_tsig_keydata(1)
        set_error()
    except TypeError as e:
        pass
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".set_tsig_keyname()"
    resolver = ldns.ldns_resolver.new_frm_file("/etc/resolv.conf")
    tknstr = "key 1"
    try:
        resolver.set_tsig_keyname(tknstr)
        ret = resolver.tsig_keyname()
        if not isinstance(ret, str):
            set_error()
        if ret != tknstr:
            set_error()
    except:
        set_error()
    try:
        resolver.set_tsig_keyname(1)
        set_error()
    except TypeError as e:
        pass
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".set_usevc()"
    resolver = ldns.ldns_resolver.new_frm_file("/etc/resolv.conf")
    try:
        resolver.set_usevc(False)
        ret = resolver.usevc()
        if not isinstance(ret, bool):
            set_error()
        if ret != False:
            set_error()
    except:
        set_error()
    try:
        resolver.set_usevc(True)
        ret = resolver.usevc()
        if not isinstance(ret, bool):
            set_error()
        if ret != True:
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".timeout()"
    sys.stderr.write("%s not tested.\n" % (method_name))


#if not error_detected:
if True:
    method_name = class_name + ".trusted_key()"
    resolver = ldns.ldns_resolver.new_frm_file("/etc/resolv.conf")
    key = ldns.ldns_key.new_frm_algorithm(ldns.LDNS_SIGN_DSA, 512)
    domain = ldns.ldns_dname("example.")
    key.set_pubkey_owner(domain)
    pubkey = key.key_to_rr()
    ds = ldns.ldns_key_rr2ds(pubkey, ldns.LDNS_SHA1)
    resolver.push_dnssec_anchor(ds)
    rrl = ldns.ldns_rr_list.new()
    try:
        ret = resolver.trusted_key(rrl)
        if ret != None:
            set_error()
    except:
        set_error()
    rrl.push_rr(ds)
    ret = resolver.trusted_key(rrl)
    try:
        ret = resolver.trusted_key(rrl)
        if not isinstance(ret, ldns.ldns_rr_list):
            set_error()
        if ret.rr_count() != 1:
            set_error()
    except:
        set_error()
    try:
        ret = resolver.trusted_key("bad argument")
        set_error()
    except TypeError as e:
        pass
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".tsig_algorithm()"
    resolver = ldns.ldns_resolver.new_frm_file("/etc/resolv.conf")
    try:
        ret = resolver.tsig_algorithm()
        if ret != None:
            set_error()
    except:
        set_error()
    tsigstr = "hmac-md5.sig-alg.reg.int."
    resolver.set_tsig_algorithm(tsigstr)
    try:
        ret = resolver.tsig_algorithm()
        if not isinstance(ret, str):
            set_error()
        if ret != tsigstr:
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".tsig_keydata()"
    resolver = ldns.ldns_resolver.new_frm_file("/etc/resolv.conf")
    try:
        ret = resolver.tsig_keydata()
        if ret != None:
            set_error()
    except:
        set_error()
    tkdstr = "Twas brillig, and the slithy toves Did gyre and gimble in the wabe; All mimsy were the borogoves, And the mome raths outgrabe."
    resolver.set_tsig_keydata(tkdstr)
    try:
        ret = resolver.tsig_keydata()
        if not isinstance(ret, str):
            set_error()
        if ret != tkdstr:
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".tsig_keyname()"
    resolver = ldns.ldns_resolver.new_frm_file("/etc/resolv.conf")
    try:
        ret = resolver.tsig_keyname()
        if ret != None:
            set_error()
    except:
        set_error()
    tknstr = "key 2"
    resolver.set_tsig_keyname(tknstr)
    try:
        ret = resolver.tsig_keyname()
        if not isinstance(ret, str):
            set_error()
        if ret != tknstr:
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".usevc()"
    resolver = ldns.ldns_resolver.new_frm_file("/etc/resolv.conf")
    resolver.set_usevc(False)
    try:
        ret = resolver.usevc()
        if not isinstance(ret, bool):
            set_error()
        if ret != False:
            set_error()
    except:
        set_error()
    resolver.set_usevc(True)
    try:
        ret = resolver.usevc()
        if not isinstance(ret, bool):
            set_error()
        if ret != True:
            set_error()
    except:
        set_error()


if not error_detected:
    sys.stdout.write("%s: passed.\n" % (os.path.basename(__file__)))
else:
    sys.stdout.write("%s: errors detected.\n" % (os.path.basename(__file__)))
    sys.exit(1)
