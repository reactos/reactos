#!/usr/bin/env python

#
# ldns_pkt testing script.
#
# Do not use constructs that differ between Python 2 and 3.
# Use write on stdout or stderr.
#


import ldns
import sys
import os
import inspect


class_name = "ldns_pkt"
method_name = None
error_detected = False
temp_fname = "tmp_pkt.txt"


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
    method_name = class_name + ".aa()"
    pkt = ldns.ldns_pkt.new_query_frm_str("test.nic.cz",ldns.LDNS_RR_TYPE_ANY, ldns.LDNS_RR_CLASS_IN, ldns.LDNS_QR | ldns.LDNS_AA)
    try:
        ret = pkt.aa()
        if not isinstance(ret, bool):
            set_error()
        if ret != True:
            set_error()
    except:
        set_error()
    pkt = ldns.ldns_pkt.new_query_frm_str("test.nic.cz",ldns.LDNS_RR_TYPE_ANY, ldns.LDNS_RR_CLASS_IN, ldns.LDNS_QR)
    try:
        ret = pkt.aa()
        if not isinstance(ret, bool):
            set_error()
        if ret != False:
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".ad()"
    pkt = ldns.ldns_pkt.new_query_frm_str("test.nic.cz",ldns.LDNS_RR_TYPE_ANY, ldns.LDNS_RR_CLASS_IN, ldns.LDNS_QR | ldns.LDNS_AD)
    try:
        ret = pkt.ad()
        if not isinstance(ret, bool):
            set_error()
        if ret != True:
            set_error()
    except:
        set_error()
    pkt = ldns.ldns_pkt.new_query_frm_str("test.nic.cz",ldns.LDNS_RR_TYPE_ANY, ldns.LDNS_RR_CLASS_IN, ldns.LDNS_QR)
    try:
        ret = pkt.ad()
        if not isinstance(ret, bool):
            set_error()
        if ret != False:
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".additional()"
    pkt = ldns.ldns_pkt.new_query_frm_str("test.nic.cz",ldns.LDNS_RR_TYPE_ANY, ldns.LDNS_RR_CLASS_IN, ldns.LDNS_QR | ldns.LDNS_AD)
    try:
        ret = pkt.additional()
        if not isinstance(ret, ldns.ldns_rr_list):
            set_error()
        if ret.rr_count() != 0:
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".algorithm2str()"
    try:
        ret = ldns.ldns_pkt.algorithm2str(ldns.LDNS_DSA)
        if not isinstance(ret, str):
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".all()"
    pkt = ldns.ldns_pkt.new_query_frm_str("test.nic.cz",ldns.LDNS_RR_TYPE_ANY, ldns.LDNS_RR_CLASS_IN, ldns.LDNS_QR | ldns.LDNS_AD)
    try:
        ret = pkt.all()
        if not isinstance(ret, ldns.ldns_rr_list):
            set_error()
        if ret.rr_count() != 1:
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".all_noquestion()"
    pkt = ldns.ldns_pkt.new_query_frm_str("test.nic.cz",ldns.LDNS_RR_TYPE_ANY, ldns.LDNS_RR_CLASS_IN, ldns.LDNS_QR | ldns.LDNS_AD)
    try:
        ret = pkt.all_noquestion()
        if not isinstance(ret, ldns.ldns_rr_list):
            set_error()
        if ret.rr_count() != 0:
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".ancount()"
    pkt = ldns.ldns_pkt.new_query_frm_str("test.nic.cz",ldns.LDNS_RR_TYPE_ANY, ldns.LDNS_RR_CLASS_IN, ldns.LDNS_QR | ldns.LDNS_AD)
    try:
        ret = pkt.ancount()
        if (not isinstance(ret, int)) and (not isinstance(ret, long)):
            set_error()
        if ret != 0:
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".answer()"
    pkt = ldns.ldns_pkt.new_query_frm_str("test.nic.cz",ldns.LDNS_RR_TYPE_ANY, ldns.LDNS_RR_CLASS_IN, ldns.LDNS_QR | ldns.LDNS_AD)
    try:
        ret = pkt.answer()
        if not isinstance(ret, ldns.ldns_rr_list):
            set_error()
        if ret.rr_count() != 0:
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".answerfrom()"
    pkt = ldns.ldns_pkt.new_query_frm_str("test.nic.cz",ldns.LDNS_RR_TYPE_ANY, ldns.LDNS_RR_CLASS_IN, ldns.LDNS_QR | ldns.LDNS_AD)
    try:
        ret = pkt.answerfrom()
        if ret != None:
            set_error()
    except:
        set_error()
    resolver = ldns.ldns_resolver.new_frm_file("/etc/resolv.conf")
    pkt = resolver.query("www.nic.cz", ldns.LDNS_RR_TYPE_A, ldns.LDNS_RR_CLASS_IN)
    try:
        ret = pkt.answerfrom()
        if not isinstance(ret, ldns.ldns_rdf):
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".arcount()"
    pkt = ldns.ldns_pkt.new_query_frm_str("test.nic.cz",ldns.LDNS_RR_TYPE_ANY, ldns.LDNS_RR_CLASS_IN, ldns.LDNS_QR | ldns.LDNS_AD)
    try:
        ret = pkt.arcount()
        if (not isinstance(ret, int)) and (not isinstance(ret, long)):
            set_error()
        if ret != 0:
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".authority()"
    pkt = ldns.ldns_pkt.new_query_frm_str("test.nic.cz",ldns.LDNS_RR_TYPE_ANY, ldns.LDNS_RR_CLASS_IN, ldns.LDNS_QR | ldns.LDNS_AD)
    try:
        ret = pkt.authority()
        if not isinstance(ret, ldns.ldns_rr_list):
            set_error()
        if ret.rr_count() != 0:
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".cd()"
    pkt = ldns.ldns_pkt.new_query_frm_str("test.nic.cz",ldns.LDNS_RR_TYPE_ANY, ldns.LDNS_RR_CLASS_IN, ldns.LDNS_CD)
    try:
        ret = pkt.cd()
        if not isinstance(ret, bool):
            set_error()
        if ret != True:
            set_error()
    except:
        set_error()
    pkt = ldns.ldns_pkt.new_query_frm_str("test.nic.cz",ldns.LDNS_RR_TYPE_ANY, ldns.LDNS_RR_CLASS_IN, ldns.LDNS_QR)
    try:
        ret = pkt.cd()
        if not isinstance(ret, bool):
            set_error()
        if ret != False:
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".cert_algorithm2str()"
    try:
        ret = ldns.ldns_pkt.cert_algorithm2str(ldns.LDNS_CERT_PGP)
        if not isinstance(ret, str):
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".clone()"
    pkt = ldns.ldns_pkt.new_query_frm_str("test.nic.cz",ldns.LDNS_RR_TYPE_ANY, ldns.LDNS_RR_CLASS_IN, ldns.LDNS_QR | ldns.LDNS_AD)
    try:
        ret = pkt.clone()
        if not isinstance(ret, ldns.ldns_pkt):
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".ends()"
    pkt = ldns.ldns_pkt.new_query_frm_str("test.nic.cz",ldns.LDNS_RR_TYPE_ANY, ldns.LDNS_RR_CLASS_IN, ldns.LDNS_RD | ldns.LDNS_AD)
    try:
        ret = pkt.edns()
        if not isinstance(ret, bool):
            set_error()
        if ret != False:
            set_error()
    except:
        set_error()
    resolver = ldns.ldns_resolver.new_frm_file("/etc/resolv.conf")
    resolver.set_dnssec(True)
    pkt = resolver.query("www.nic.cz", ldns.LDNS_RR_TYPE_A, ldns.LDNS_RR_CLASS_IN)
    try:
        ret = pkt.edns()
        if not isinstance(ret, bool):
            set_error()
        if ret != True:
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".ends_data()"
    pkt = ldns.ldns_pkt.new_query_frm_str("test.nic.cz",ldns.LDNS_RR_TYPE_ANY, ldns.LDNS_RR_CLASS_IN, ldns.LDNS_RD | ldns.LDNS_AD)
    try:
        ret = pkt.edns_data()
        if ret != None:
            set_error()
    except:
        set_error()
    #resolver = ldns.ldns_resolver.new_frm_file("/etc/resolv.conf")
    #resolver.set_dnssec(True)
    #pkt = resolver.query("www.nic.cz", ldns.LDNS_RR_TYPE_A, ldns.LDNS_RR_CLASS_IN)
    #try:
    #    ret = pkt.edns_data()
    #    print ret
    #    if not isinstance(ret, ldns.ldns_rdf):
    #        set_error()
    #    if ret != True:
    #        set_error()
    #except:
    #    set_error()


#if not error_detected:
if True:
    method_name = class_name + ".edns_do()"
    pkt = ldns.ldns_pkt.new_query_frm_str("test.nic.cz",ldns.LDNS_RR_TYPE_ANY, ldns.LDNS_RR_CLASS_IN, ldns.LDNS_RD | ldns.LDNS_AD)
    try:
        ret = pkt.edns_do()
        if not isinstance(ret, bool):
            set_error()
        if ret != False:
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".edns_extended_rcode()"
    pkt = ldns.ldns_pkt.new_query_frm_str("test.nic.cz",ldns.LDNS_RR_TYPE_ANY, ldns.LDNS_RR_CLASS_IN, ldns.LDNS_RD | ldns.LDNS_AD)
    try:
        ret = pkt.edns_extended_rcode()
        if (not isinstance(ret, int)) and (not isinstance(ret, long)):
            set_error()
        if ret != 0:
            set_error()
    except:
        set_error()
    #resolver = ldns.ldns_resolver.new_frm_file("/etc/resolv.conf")
    #resolver.set_dnssec(True)
    #pkt = resolver.query("www.nic.cz", ldns.LDNS_RR_TYPE_A, ldns.LDNS_RR_CLASS_IN)
    #try:
    #    ret = pkt.edns_extended_rcode()
    #    if (not isinstance(ret, int)) and (not isinstance(ret, long)):
    #        set_error()
    #    if ret != 0:
    #        set_error()
    #except:
    #    set_error()


#if not error_detected:
if True:
    method_name = class_name + ".edns_udp_size()"
    pkt = ldns.ldns_pkt.new_query_frm_str("test.nic.cz",ldns.LDNS_RR_TYPE_ANY, ldns.LDNS_RR_CLASS_IN, ldns.LDNS_RD | ldns.LDNS_AD)
    try:
        ret = pkt.edns_udp_size()
        if (not isinstance(ret, int)) and (not isinstance(ret, long)):
            set_error()
        if ret != 0:
            set_error()
    except:
        set_error()
    resolver = ldns.ldns_resolver.new_frm_file("/etc/resolv.conf")
    resolver.set_dnssec(True)
    pkt = resolver.query("www.nic.cz", ldns.LDNS_RR_TYPE_A, ldns.LDNS_RR_CLASS_IN)
    try:
        ret = pkt.edns_udp_size()
        if (not isinstance(ret, int)) and (not isinstance(ret, long)):
            set_error()
        if ret == 0: # Don't know the actual size, but must be greater than 0.
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".edns_version()"
    pkt = ldns.ldns_pkt.new_query_frm_str("test.nic.cz",ldns.LDNS_RR_TYPE_ANY, ldns.LDNS_RR_CLASS_IN, ldns.LDNS_RD | ldns.LDNS_AD)
    try:
        ret = pkt.edns_version()
        if (not isinstance(ret, int)) and (not isinstance(ret, long)):
            set_error()
        if ret != 0:
            set_error()
    except:
        set_error()
    #resolver = ldns.ldns_resolver.new_frm_file("/etc/resolv.conf")
    #resolver.set_dnssec(True)
    #pkt = resolver.query("www.nic.cz", ldns.LDNS_RR_TYPE_A, ldns.LDNS_RR_CLASS_IN)
    #try:
    #    ret = pkt.edns_version()
    #    if (not isinstance(ret, int)) and (not isinstance(ret, long)):
    #        set_error()
    #    if ret != 0:
    #        set_error()
    #except:
    #    set_error()


#if not error_detected:
if True:
    method_name = class_name + ".edns_z()"
    pkt = ldns.ldns_pkt.new_query_frm_str("test.nic.cz",ldns.LDNS_RR_TYPE_ANY, ldns.LDNS_RR_CLASS_IN, ldns.LDNS_RD | ldns.LDNS_AD)
    try:
        ret = pkt.edns_z()
        if (not isinstance(ret, int)) and (not isinstance(ret, long)):
            set_error()
        if ret != 0:
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".empty()"
    pkt = ldns.ldns_pkt.new_query_frm_str("test.nic.cz",ldns.LDNS_RR_TYPE_ANY, ldns.LDNS_RR_CLASS_IN, ldns.LDNS_RD | ldns.LDNS_AD)
    try:
        ret = pkt.empty()
        if not isinstance(ret, bool):
            set_error()
        if ret != False:
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".get_opcode()"
    pkt = ldns.ldns_pkt.new_query_frm_str("test.nic.cz",ldns.LDNS_RR_TYPE_ANY, ldns.LDNS_RR_CLASS_IN, ldns.LDNS_RD | ldns.LDNS_AD)
    try:
        ret = pkt.get_opcode()
        if (not isinstance(ret, int)) and (not isinstance(ret, long)):
            set_error()
        if ret != ldns.LDNS_PACKET_QUERY:
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".get_rcode()"
    pkt = ldns.ldns_pkt.new_query_frm_str("test.nic.cz",ldns.LDNS_RR_TYPE_ANY, ldns.LDNS_RR_CLASS_IN, ldns.LDNS_RD | ldns.LDNS_AD)
    try:
        ret = pkt.get_rcode()
        if (not isinstance(ret, int)) and (not isinstance(ret, long)):
            set_error()
        if ret != ldns.LDNS_RCODE_NOERROR:
            set_error()
    except:
        set_error()
    resolver = ldns.ldns_resolver.new_frm_file("/etc/resolv.conf")
    resolver.set_dnssec(True)
    pkt = resolver.query("nonexistent_domain.nic.cz", ldns.LDNS_RR_TYPE_A, ldns.LDNS_RR_CLASS_IN)
    try:
        ret = pkt.get_rcode()
        if (not isinstance(ret, int)) and (not isinstance(ret, long)):
            set_error()
        if ret != ldns.LDNS_RCODE_NXDOMAIN:
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".get_section_clone()"
    pkt = ldns.ldns_pkt.new_query_frm_str("test.nic.cz",ldns.LDNS_RR_TYPE_ANY, ldns.LDNS_RR_CLASS_IN, ldns.LDNS_RD | ldns.LDNS_AD)
    try:
        ret = pkt.get_section_clone(ldns.LDNS_SECTION_ANY)
        if not isinstance(ret, ldns.ldns_rr_list):
            set_error()
    except:
        set_error()
    try:
        ret = pkt.get_section_clone(ldns.LDNS_SECTION_ANSWER)
        if not isinstance(ret, ldns.ldns_rr_list):
            set_error()
    except:
        set_error()
    try:
        ret = pkt.get_section_clone("bad argument")
        set_error()
    except TypeError as e:
        pass
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".id()"
    pkt = ldns.ldns_pkt.new_query_frm_str("test.nic.cz",ldns.LDNS_RR_TYPE_ANY, ldns.LDNS_RR_CLASS_IN, ldns.LDNS_RD | ldns.LDNS_AD)
    try:
        ret = pkt.id()
        if (not isinstance(ret, int)) and (not isinstance(ret, long)):
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".new()"
    try:
        pkt = ldns.ldns_pkt.new()
        if not isinstance(pkt, ldns.ldns_pkt):
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".new_query()"
    dname = ldns.ldns_dname("test.nic.cz.")
    try:
        pkt = ldns.ldns_pkt.new_query(dname, ldns.LDNS_RR_TYPE_A, ldns.LDNS_RR_CLASS_IN, ldns.LDNS_QR | ldns.LDNS_RD)
        if not isinstance(pkt, ldns.ldns_pkt):
            set_error()
    except:
        set_error()
    rdf = ldns.ldns_rdf_new_frm_str(ldns.LDNS_RDF_TYPE_DNAME, "test.nic.cz.")
    try:
        pkt = ldns.ldns_pkt.new_query(rdf, ldns.LDNS_RR_TYPE_A, ldns.LDNS_RR_CLASS_IN, ldns.LDNS_QR | ldns.LDNS_RD)
        if not isinstance(pkt, ldns.ldns_pkt):
            set_error()
    except:
        set_error()
    try:
        pkt = ldns.ldns_pkt.new_query("bad argument", ldns.LDNS_RR_TYPE_A, ldns.LDNS_RR_CLASS_IN, ldns.LDNS_QR | ldns.LDNS_RD)
        set_error()
    except TypeError as e:
        pass
    except:
        set_error()
    try:
        pkt = ldns.ldns_pkt.new_query(dname, "bad argument", ldns.LDNS_RR_CLASS_IN, ldns.LDNS_QR | ldns.LDNS_RD)
        set_error()
    except TypeError as e:
        pass
    except:
        set_error()
    try:
        pkt = ldns.ldns_pkt.new_query(dname, ldns.LDNS_RR_TYPE_A, "bad argument", ldns.LDNS_QR | ldns.LDNS_RD)
        set_error()
    except TypeError as e:
        pass
    except:
        set_error()
    try:
        pkt = ldns.ldns_pkt.new_query(dname, ldns.LDNS_RR_TYPE_A, ldns.LDNS_RR_CLASS_IN, "bad argument")
        set_error()
    except TypeError as e:
        pass
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".new_query_frm_str()"
    try:
        pkt = ldns.ldns_pkt.new_query_frm_str("test.nic.cz", ldns.LDNS_RR_TYPE_ANY, ldns.LDNS_RR_CLASS_IN, ldns.LDNS_QR | ldns.LDNS_AA)
    except:
        set_error()
    try:
        pkt = ldns.ldns_pkt.new_query_frm_str(pkt, ldns.LDNS_RR_TYPE_ANY, ldns.LDNS_RR_CLASS_IN, ldns.LDNS_QR | ldns.LDNS_AA)
        set_error()
    except TypeError as e:
        pass
    except:
        set_error()
    try:
        pkt = ldns.ldns_pkt.new_query_frm_str("test.nic.cz", "bad argument", ldns.LDNS_RR_CLASS_IN, ldns.LDNS_QR | ldns.LDNS_AA)
        set_error()
    except TypeError as e:
        pass
    except:
        set_error()
    try:
        pkt = ldns.ldns_pkt.new_query_frm_str("test.nic.cz", ldns.LDNS_RR_TYPE_ANY, "bad argument", ldns.LDNS_QR | ldns.LDNS_AA)
        set_error()
    except TypeError as e:
        pass
    except:
        set_error()
    try:
        pkt = ldns.ldns_pkt.new_query_frm_str("test.nic.cz", ldns.LDNS_RR_TYPE_ANY, ldns.LDNS_RR_CLASS_IN, "bad argument")
        set_error()
    except TypeError as e:
        pass
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".nscount()"
    pkt = ldns.ldns_pkt.new_query_frm_str("test.nic.cz",ldns.LDNS_RR_TYPE_ANY, ldns.LDNS_RR_CLASS_IN, ldns.LDNS_RD | ldns.LDNS_AD)
    try:
        ret = pkt.nscount()
        if (not isinstance(ret, int)) and (not isinstance(ret, long)):
            set_error()
        if ret != 0:
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".opcode2str()"
    pkt = ldns.ldns_pkt.new_query_frm_str("test.nic.cz",ldns.LDNS_RR_TYPE_ANY, ldns.LDNS_RR_CLASS_IN, ldns.LDNS_RD | ldns.LDNS_AD)
    try:
        ret = pkt.opcode2str()
        if not isinstance(ret, str):
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".print_to_file()"
    pkt = ldns.ldns_pkt.new_query_frm_str("test.nic.cz",ldns.LDNS_RR_TYPE_ANY, ldns.LDNS_RR_CLASS_IN, ldns.LDNS_RD | ldns.LDNS_AD)
    f = open(temp_fname, "w")
    try:
        pkt.print_to_file(f)
    except:
        set_error()
    f.close()
    f = open(temp_fname, "r")
    if len(f.readlines()) != 14:
        set_error()
    f.close()
    os.remove(temp_fname)


#if not error_detected:
if True:
    method_name = class_name + ".push_rr()"
    pkt = ldns.ldns_pkt.new_query_frm_str("test.nic.cz",ldns.LDNS_RR_TYPE_ANY, ldns.LDNS_RR_CLASS_IN, ldns.LDNS_RD | ldns.LDNS_AD)
    rr = ldns.ldns_rr.new_frm_str("test1 600 IN A 0.0.0.0")
    try:
        ret = pkt.push_rr(ldns.LDNS_SECTION_ANSWER, rr)
        if not isinstance(ret, bool):
            set_error()
        if ret != True:
            set_error()
    except:
        set_error()
    try:
        ret = pkt.push_rr("bad argument", rr)
        set_error()
    except TypeError as e:
        pass
    except:
        set_error()
    try:
        ret = pkt.push_rr(ldns.LDNS_SECTION_ANSWER, "bad argument")
        set_error()
    except TypeError as e:
        pass
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".push_rr_list()"
    pkt = ldns.ldns_pkt.new_query_frm_str("test.nic.cz",ldns.LDNS_RR_TYPE_ANY, ldns.LDNS_RR_CLASS_IN, ldns.LDNS_RD | ldns.LDNS_AD)
    rrl = ldns.ldns_rr_list.new()
    rr = ldns.ldns_rr.new_frm_str("test1 600 IN A 0.0.0.0")
    rrl.push_rr(rr)
    rrl.push_rr(rr)
    try:
        ret = pkt.push_rr_list(ldns.LDNS_SECTION_ANSWER, rrl)
        if not isinstance(ret, bool):
            set_error()
        if ret != True:
            set_error()
    except:
        set_error()
    try:
        ret = pkt.push_rr_list("bad argument", rrl)
        set_error()
    except TypeError as e:
        pass
    except:
        set_error()
    try:
        ret = pkt.push_rr_list(ldns.LDNS_SECTION_ANSWER, "bad argument")
        set_error()
    except TypeError as e:
        pass
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".qdcount()"
    pkt = ldns.ldns_pkt.new_query_frm_str("test.nic.cz",ldns.LDNS_RR_TYPE_ANY, ldns.LDNS_RR_CLASS_IN, ldns.LDNS_QR | ldns.LDNS_AD)
    try:
        ret = pkt.qdcount()
        if (not isinstance(ret, int)) and (not isinstance(ret, long)):
            set_error()
        if ret != 1:
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".qr()"
    pkt = ldns.ldns_pkt.new_query_frm_str("test.nic.cz",ldns.LDNS_RR_TYPE_ANY, ldns.LDNS_RR_CLASS_IN, ldns.LDNS_QR | ldns.LDNS_AA)
    try:
        ret = pkt.qr()
        if not isinstance(ret, bool):
            set_error()
        if ret != True:
            set_error()
    except:
        set_error()
    pkt = ldns.ldns_pkt.new_query_frm_str("test.nic.cz",ldns.LDNS_RR_TYPE_ANY, ldns.LDNS_RR_CLASS_IN, ldns.LDNS_AA)
    try:
        ret = pkt.qr()
        if not isinstance(ret, bool):
            set_error()
        if ret != False:
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".querytime()"
    pkt = ldns.ldns_pkt.new_query_frm_str("test.nic.cz",ldns.LDNS_RR_TYPE_ANY, ldns.LDNS_RR_CLASS_IN, ldns.LDNS_QR | ldns.LDNS_AD)
    try:
        ret = pkt.querytime()
        if (not isinstance(ret, int)) and (not isinstance(ret, long)):
            set_error()
        if ret != 0:
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".question()"
    pkt = ldns.ldns_pkt.new_query_frm_str("test.nic.cz",ldns.LDNS_RR_TYPE_ANY, ldns.LDNS_RR_CLASS_IN, ldns.LDNS_QR | ldns.LDNS_AD)
    try:
        ret = pkt.question()
        if not isinstance(ret, ldns.ldns_rr_list):
            set_error()
        if ret.rr_count() != 1:
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".ra()"
    pkt = ldns.ldns_pkt.new_query_frm_str("test.nic.cz",ldns.LDNS_RR_TYPE_ANY, ldns.LDNS_RR_CLASS_IN, ldns.LDNS_QR | ldns.LDNS_RA)
    try:
        ret = pkt.ra()
        if not isinstance(ret, bool):
            set_error()
        if ret != True:
            set_error()
    except:
        set_error()
    pkt = ldns.ldns_pkt.new_query_frm_str("test.nic.cz",ldns.LDNS_RR_TYPE_ANY, ldns.LDNS_RR_CLASS_IN, ldns.LDNS_QR)
    try:
        ret = pkt.ra()
        if not isinstance(ret, bool):
            set_error()
        if ret != False:
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".rcode2str()"
    pkt = ldns.ldns_pkt.new_query_frm_str("test.nic.cz",ldns.LDNS_RR_TYPE_ANY, ldns.LDNS_RR_CLASS_IN, ldns.LDNS_QR | ldns.LDNS_RA)
    try:
        ret = pkt.rcode2str()
        if not isinstance(ret, str):
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".rd()"
    pkt = ldns.ldns_pkt.new_query_frm_str("test.nic.cz",ldns.LDNS_RR_TYPE_ANY, ldns.LDNS_RR_CLASS_IN, ldns.LDNS_QR | ldns.LDNS_RD)
    try:
        ret = pkt.rd()
        if not isinstance(ret, bool):
            set_error()
        if ret != True:
            set_error()
    except:
        set_error()
    pkt = ldns.ldns_pkt.new_query_frm_str("test.nic.cz",ldns.LDNS_RR_TYPE_ANY, ldns.LDNS_RR_CLASS_IN, ldns.LDNS_QR)
    try:
        ret = pkt.rd()
        if not isinstance(ret, bool):
            set_error()
        if ret != False:
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".reply_type()"
    pkt = ldns.ldns_pkt.new_query_frm_str("test.nic.cz",ldns.LDNS_RR_TYPE_ANY, ldns.LDNS_RR_CLASS_IN, ldns.LDNS_QR | ldns.LDNS_RD)
    try:
        ret = pkt.reply_type()
        if ret != ldns.LDNS_PACKET_ANSWER:
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".rr()"
    pkt = ldns.ldns_pkt.new_query_frm_str("test.nic.cz", ldns.LDNS_RR_TYPE_A, ldns.LDNS_RR_CLASS_IN, ldns.LDNS_QR | ldns.LDNS_RD)
    rr = ldns.ldns_rr.new_frm_str("test1 600 IN A 0.0.0.0")
    pkt.push_rr(ldns.LDNS_SECTION_ANSWER, rr)
    try:
        ret = pkt.rr(ldns.LDNS_SECTION_ANSWER, rr)
        if not isinstance(ret, bool):
            set_error()
        if ret != True:
            set_error()
    except:
        set_error()
    try:
        ret = pkt.rr(ldns.LDNS_SECTION_QUESTION, rr)
        if not isinstance(ret, bool):
            set_error()
        if ret != False:
            set_error()
    except:
        set_error()
    try:
        ret = pkt.rr("bad argument", rr)
        set_error()
    except TypeError as e:
        pass
    except:
        set_error()
    try:
        ret = pkt.rr(ldns.LDNS_SECTION_QUESTION, "bad argument")
        set_error()
    except TypeError as e:
        pass
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".rr_list_by_name()"
    pkt = ldns.ldns_pkt.new_query_frm_str("test.nic.cz", ldns.LDNS_RR_TYPE_A, ldns.LDNS_RR_CLASS_IN, ldns.LDNS_QR | ldns.LDNS_RD)
    rr = ldns.ldns_rr.new_frm_str("test1 600 IN A 0.0.0.0")
    pkt.push_rr(ldns.LDNS_SECTION_ANSWER, rr)
    rdf = ldns.ldns_rdf_new_frm_str(ldns.LDNS_RDF_TYPE_DNAME, "test1")
    try:
        ret = pkt.rr_list_by_name(rdf, ldns.LDNS_SECTION_ANSWER)
        if not isinstance(ret, ldns.ldns_rr_list):
            set_error()
    except:
        set_error()
    try:
        ret = pkt.rr_list_by_name(rdf, ldns.LDNS_SECTION_QUESTION)
        if ret != None:
            set_error()
    except:
        set_error()
    try:
        ret = pkt.rr_list_by_name("bad argument", ldns.LDNS_SECTION_ANSWER)
        set_error()
    except TypeError as e:
        pass
    except:
        set_error()
    try:
        ret = pkt.rr_list_by_name(rdf, "bad argument")
        set_error()
    except TypeError as e:
        pass
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".rr_list_by_name_and_type()"
    pkt = ldns.ldns_pkt.new_query_frm_str("test.nic.cz.", ldns.LDNS_RR_TYPE_A, ldns.LDNS_RR_CLASS_IN, ldns.LDNS_QR | ldns.LDNS_RD)
    rr = ldns.ldns_rr.new_frm_str("test1 600 IN A 0.0.0.0")
    pkt.push_rr(ldns.LDNS_SECTION_ANSWER, rr)
    rdf = ldns.ldns_rdf_new_frm_str(ldns.LDNS_RDF_TYPE_DNAME, "test1")
    try:
        ret = pkt.rr_list_by_name_and_type(rdf, ldns.LDNS_RR_TYPE_A, ldns.LDNS_SECTION_ANSWER)
        if not isinstance(ret, ldns.ldns_rr_list):
            set_error()
    except:
        set_error()
    try:
        ret = pkt.rr_list_by_name_and_type(rdf, ldns.LDNS_RR_TYPE_AAAA, ldns.LDNS_SECTION_ANSWER)
        if ret != None:
            set_error()
    except:
        set_error()
    #try:
    #    ret = pkt.rr_list_by_name_and_type("bad argument", ldns.LDNS_RR_TYPE_A, ldns.LDNS_SECTION_ANSWER)
    #    set_error()
    #except TypeError as e:
    #    pass
    #except:
    #    set_error()
    try:
        ret = pkt.rr_list_by_name_and_type(rdf, "bad argument", ldns.LDNS_SECTION_ANSWER)
        set_error()
    except TypeError as e:
        pass
    except:
        set_error()
    try:
        ret = pkt.rr_list_by_name_and_type(rdf, ldns.LDNS_RR_TYPE_A, "bad argument")
        set_error()
    except TypeError as e:
        pass
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".rr_list_by_type()"
    pkt = ldns.ldns_pkt.new_query_frm_str("test.nic.cz.", ldns.LDNS_RR_TYPE_A, ldns.LDNS_RR_CLASS_IN, ldns.LDNS_QR | ldns.LDNS_RD)
    rr = ldns.ldns_rr.new_frm_str("test1 600 IN A 0.0.0.0")
    pkt.push_rr(ldns.LDNS_SECTION_ANSWER, rr)
    try:
        ret = pkt.rr_list_by_type(ldns.LDNS_RR_TYPE_A, ldns.LDNS_SECTION_ANSWER)
        if not isinstance(ret, ldns.ldns_rr_list):
            set_error()
    except:
        set_error()
    try:
        ret = pkt.rr_list_by_type(ldns.LDNS_RR_TYPE_AAAA, ldns.LDNS_SECTION_ANSWER)
        if ret != None:
            set_error()
    except:
        set_error()
    try:
        ret = pkt.rr_list_by_type("bad argument", ldns.LDNS_SECTION_ANSWER)
        set_error()
    except TypeError as e:
        pass
    except:
        set_error()
    try:
        ret = pkt.rr_list_by_type(ldns.LDNS_RR_TYPE_A, "bad argument")
        set_error()
    except TypeError as e:
        pass
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".safe_push_rr()"
    pkt = ldns.ldns_pkt.new()
    rr = ldns.ldns_rr.new_frm_str("test1 600 IN A 0.0.0.0")
    try:
        ret = pkt.safe_push_rr(ldns.LDNS_SECTION_ANSWER, rr)
        if not isinstance(ret, bool):
            set_error()
        if ret != True:
            set_error()
    except:
        set_error()
    try:
        ret = pkt.safe_push_rr(ldns.LDNS_SECTION_ANSWER, rr)
        if not isinstance(ret, bool):
            set_error()
        if ret != False:
            set_error()
    except:
        set_error()
    try:
        ret = pkt.safe_push_rr("bad argument", rr)
        set_error()
    except TypeError as e:
        pass
    except:
        set_error()
    try:
        ret = pkt.safe_push_rr(ldns.LDNS_SECTION_ANSWER, "bad argument")
        set_error()
    except TypeError as e:
        pass
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".safe_push_rr_list()"
    pkt = ldns.ldns_pkt.new()
    rrl = ldns.ldns_rr_list.new()
    rr = ldns.ldns_rr.new_frm_str("test1 600 IN A 0.0.0.0")
    rrl.push_rr(rr)
    try:
        ret = pkt.safe_push_rr_list(ldns.LDNS_SECTION_ANSWER, rrl)
        if not isinstance(ret, bool):
            set_error()
        if ret != True:
            set_error()
    except:
        set_error()
    try:
        ret = pkt.safe_push_rr_list(ldns.LDNS_SECTION_ANSWER, rrl)
        if not isinstance(ret, bool):
            set_error()
        if ret != False:
            set_error()
    except:
        set_error()
    try:
        ret = pkt.safe_push_rr_list("bad argument", rrl)
        set_error()
    except TypeError as e:
        pass
    except:
        set_error()
    try:
        ret = pkt.safe_push_rr_list(ldns.LDNS_SECTION_ANSWER, "bad argument")
        set_error()
    except TypeError as e:
        pass
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".set_aa()"
    pkt = ldns.ldns_pkt.new()
    try:
        pkt.set_aa(True)
        if pkt.aa() != True:
            set_error()
    except:
        set_error()
    try:
        pkt.set_aa(False)
        if pkt.aa() != False:
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".set_ad()"
    pkt = ldns.ldns_pkt.new()
    try:
        pkt.set_ad(True)
        if pkt.ad() != True:
            set_error()
    except:
        set_error()
    try:
        pkt.set_ad(False)
        if pkt.ad() != False:
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".set_additional()"
    pkt = ldns.ldns_pkt.new()
    rrl = ldns.ldns_rr_list.new()
    rr = ldns.ldns_rr.new_frm_str("test1 600 IN A 0.0.0.0")
    rrl.push_rr(rr)
    try:
        pkt.set_additional(rrl)
        if not isinstance(pkt.additional() , ldns.ldns_rr_list):
            set_error()
        if pkt.additional() != rrl:
            set_error()
    except:
        set_error()
    try:
        pkt.set_additional("bad argument")
        set_error()
    except TypeError as e:
        pass
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".set_ancount()"
    pkt = ldns.ldns_pkt.new()
    try:
        pkt.set_ancount(1)
        ret = pkt.ancount()
        if ret != 1:
            set_error()
    except:
        set_error()
    try:
        pkt.set_ancount("bad argument")
        set_error()
    except TypeError as e:
        pass
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".set_answer()"
    pkt = ldns.ldns_pkt.new()
    rrl = ldns.ldns_rr_list.new()
    rr = ldns.ldns_rr.new_frm_str("test1 600 IN A 0.0.0.0")
    rrl.push_rr(rr)
    try:
        pkt.set_answer(rrl)
        if not isinstance(pkt.additional() , ldns.ldns_rr_list):
            set_error()
        if pkt.answer() != rrl:
            set_error()
    except:
        set_error()
    try:
        pkt.set_answer("bad argument")
        set_error()
    except TypeError as e:
        pass
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".set_answerfrom()"
    pkt = ldns.ldns_pkt.new()
    rdf = ldns.ldns_rdf_new_frm_str(ldns.LDNS_RDF_TYPE_A, "127.0.0.1")
    try:
        pkt.set_answerfrom(rdf)
        ret = pkt.answerfrom()
        if ret != rdf:
            set_error()
    except:
        set_error()
    try:
        pkt.set_answerfrom("bad argument")
        set_error()
    except TypeError as e:
        pass
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".set_arcount()"
    pkt = ldns.ldns_pkt.new()
    try:
        pkt.set_arcount(1)
        ret = pkt.arcount()
        if ret != 1:
            set_error()
    except:
        set_error()
    try:
        pkt.set_arcount("bad argument")
        set_error()
    except TypeError as e:
        pass
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".set_authority()"
    pkt = ldns.ldns_pkt.new()
    rrl = ldns.ldns_rr_list.new()
    rr = ldns.ldns_rr.new_frm_str("test1 600 IN A 0.0.0.0")
    rrl.push_rr(rr)
    try:
        pkt.set_authority(rrl)
        if not isinstance(pkt.additional() , ldns.ldns_rr_list):
            set_error()
        if pkt.authority() != rrl:
            set_error()
    except:
        set_error()
    try:
        pkt.set_authority("bad argument")
        set_error()
    except TypeError as e:
        pass
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".set_cd()"
    pkt = ldns.ldns_pkt.new()
    try:
        pkt.set_cd(True)
        if pkt.cd() != True:
            set_error()
    except:
        set_error()
    try:
        pkt.set_cd(False)
        if pkt.cd() != False:
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".set_edns_data()"
    pkt = ldns.ldns_pkt.new()
    rdf = ldns.ldns_rdf_new_frm_str(ldns.LDNS_RDF_TYPE_A, "127.0.0.1")
    try:
        pkt.set_edns_data(rdf)
        ret = pkt.edns_data()
        if ret != rdf:
            set_error()
    except:
        set_error()
    try:
        pkt.set_edns_data("bad argument")
        set_error()
    except TypeError as e:
        pass
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".set_edns_do()"
    pkt = ldns.ldns_pkt.new()
    try:
        pkt.set_edns_do(True)
        if pkt.edns_do() != True:
            set_error()
    except:
        set_error()
    try:
        pkt.set_edns_do(False)
        if pkt.edns_do() != False:
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".set_edns_extended_rcode()"
    pkt = ldns.ldns_pkt.new()
    try:
        pkt.set_edns_extended_rcode(8)
        ret = pkt.edns_extended_rcode()
        if ret != 8:
            set_error()
    except:
        set_error()
    try:
        pkt.set_edns_extended_rcode("bad argument")
        set_error()
    except TypeError as e:
        pass
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".set_edns_udp_size()"
    pkt = ldns.ldns_pkt.new()
    try:
        pkt.set_edns_udp_size(4096)
        ret = pkt.edns_udp_size()
        if ret != 4096:
            set_error()
    except:
        set_error()
    try:
        pkt.set_edns_udp_size("bad argument")
        set_error()
    except TypeError as e:
        pass
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".set_edns_version()"
    pkt = ldns.ldns_pkt.new()
    try:
        pkt.set_edns_version(8)
        ret = pkt.edns_version()
        if ret != 8:
            set_error()
    except:
        set_error()
    try:
        pkt.set_edns_version("bad argument")
        set_error()
    except TypeError as e:
        pass
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".set_edns_z()"
    pkt = ldns.ldns_pkt.new()
    try:
        pkt.set_edns_z(4096)
        ret = pkt.edns_z()
        if ret != 4096:
            set_error()
    except:
        set_error()
    try:
        pkt.set_edns_z("bad argument")
        set_error()
    except TypeError as e:
        pass
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".set_flags()"
    pkt = ldns.ldns_pkt.new()
    try:
        ret = pkt.set_flags(ldns.LDNS_AA | ldns.LDNS_AD)
        if not isinstance(ret, bool):
            set_error()
        if ret != True:
            set_error()
    except:
        set_error()
    try:
        pkt.set_flags("bad argument")
        set_error()
    except TypeError as e:
        pass
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".set_id()"
    pkt = ldns.ldns_pkt.new()
    try:
        pkt.set_id(4096)
        ret = pkt.id()
        if ret != 4096:
            set_error()
    except:
        set_error()
    try:
        pkt.set_id("bad argument")
        set_error()
    except TypeError as e:
        pass
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".set_nscount()"
    pkt = ldns.ldns_pkt.new()
    try:
        pkt.set_nscount(1)
        ret = pkt.nscount()
        if ret != 1:
            set_error()
    except:
        set_error()
    try:
        pkt.set_nscount("bad argument")
        set_error()
    except TypeError as e:
        pass
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".set_opcode()"
    pkt = ldns.ldns_pkt.new()
    try:
        pkt.set_opcode(ldns.LDNS_PACKET_QUERY)
        ret = pkt.get_opcode()
        if ret != ldns.LDNS_PACKET_QUERY:
            set_error()
    except:
        set_error()
    try:
        pkt.set_opcode("bad argument")
        set_error()
    except TypeError as e:
        pass
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".set_qdcount()"
    pkt = ldns.ldns_pkt.new()
    try:
        pkt.set_qdcount(10)
        ret = pkt.qdcount()
        if ret != 10:
            set_error()
    except:
        set_error()
    try:
        pkt.set_qdcount("bad argument")
        set_error()
    except TypeError as e:
        pass
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".set_qr()"
    pkt = ldns.ldns_pkt.new()
    try:
        pkt.set_qr(True)
        if pkt.qr() != True:
            set_error()
    except:
        set_error()
    try:
        pkt.set_qr(False)
        if pkt.qr() != False:
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".set_querytime()"
    pkt = ldns.ldns_pkt.new()
    try:
        pkt.set_querytime(65536)
        ret = pkt.querytime()
        if ret != 65536:
            set_error()
    except:
        set_error()
    try:
        pkt.set_querytime("bad argument")
        set_error()
    except TypeError as e:
        pass
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".set_question()"
    pkt = ldns.ldns_pkt.new()
    rrl = ldns.ldns_rr_list.new()
    rr = ldns.ldns_rr.new_frm_str("test1 600 IN A 0.0.0.0")
    rrl.push_rr(rr)
    try:
        pkt.set_question(rrl)
        if not isinstance(pkt.additional() , ldns.ldns_rr_list):
            set_error()
        if pkt.question() != rrl:
            set_error()
    except:
        set_error()
    try:
        pkt.set_question("bad argument")
        set_error()
    except TypeError as e:
        pass
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".set_ra()"
    pkt = ldns.ldns_pkt.new()
    try:
        pkt.set_ra(True)
        if pkt.ra() != True:
            set_error()
    except:
        set_error()
    try:
        pkt.set_ra(False)
        if pkt.ra() != False:
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".set_random_id()"
    pkt = ldns.ldns_pkt.new()
    try:
        pkt.set_random_id()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".set_rcode()"
    pkt = ldns.ldns_pkt.new()
    try:
        pkt.set_rcode(127)
        ret = pkt.get_rcode()
        if ret != 127:
            set_error()
    except:
        set_error()
    try:
        pkt.set_rcode("bad argument")
        set_error()
    except TypeError as e:
        pass
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".set_rd()"
    pkt = ldns.ldns_pkt.new()
    try:
        pkt.set_rd(True)
        if pkt.rd() != True:
            set_error()
    except:
        set_error()
    try:
        pkt.set_rd(False)
        if pkt.rd() != False:
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".set_section_count()"
    pkt = ldns.ldns_pkt.new()
    try:
        pkt.set_section_count(ldns.LDNS_PACKET_QUESTION, 4096)
        ret = pkt.qdcount()
        if ret != 4096:
            set_error()
    except:
        set_error()
    try:
        pkt.set_section_count("bad argument", 4096)
        set_error()
    except TypeError as e:
        pass
    except:
        set_error()
    try:
        pkt.set_section_count(ldns.LDNS_PACKET_QUESTION, "bad argument")
        set_error()
    except TypeError as e:
        pass
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".set_size()"
    pkt = ldns.ldns_pkt.new()
    try:
        pkt.set_size(512)
        ret = pkt.size()
        if ret != 512:
            set_error()
    except:
        set_error()
    try:
        pkt.set_size("bad argument")
        set_error()
    except TypeError as e:
        pass
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".set_tc()"
    pkt = ldns.ldns_pkt.new()
    try:
        pkt.set_tc(True)
        if pkt.tc() != True:
            set_error()
    except:
        set_error()
    try:
        pkt.set_tc(False)
        if pkt.tc() != False:
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".set_timestamp()"
    sys.stderr.write("%s not tested.\n" % (method_name))


#if not error_detected:
if True:
    method_name = class_name + ".set_tsig()"
    pkt = ldns.ldns_pkt.new()
    rr = ldns.ldns_rr.new_frm_str("test1 600 IN A 0.0.0.0")
    try:
        pkt.set_tsig(rr)
        ret = pkt.tsig()
        if ret != rr:
            set_error()
    except:
        set_error()
    try:
        pkt.set_tsig("bad argument")
        set_error()
    except TypeError as e:
        pass
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".size()"
    pkt = ldns.ldns_pkt.new()
    pkt.set_size(512)
    try:
        ret = pkt.size()
        if (not isinstance(ret, int)) and (not isinstance(ret, long)):
            set_error()
        if ret != 512:
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".tc()"
    pkt = ldns.ldns_pkt.new()
    pkt.set_tc(True)
    try:
        ret = pkt.tc()
        if not isinstance(ret, bool):
            set_error()
        if ret != True:
            set_error()
    except:
        set_error()
    pkt.set_tc(False)
    try:
        ret = pkt.tc()
        if not isinstance(ret, bool):
            set_error()
        if ret != False:
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".timestamp()"
    sys.stderr.write("%s not tested.\n" % (method_name))


#if not error_detected:
if True:
    method_name = class_name + ".tsig()"
    pkt = ldns.ldns_pkt.new()
    try:
        ret = pkt.tsig()
        if ret != None:
            set_error()
    except:
        set_error()
    rr = ldns.ldns_rr.new_frm_str("test1 600 IN A 0.0.0.0")
    pkt.set_tsig(rr)
    try:
        ret = pkt.tsig()
        if not isinstance(ret, ldns.ldns_rr):
            set_error()
        if ret != rr:
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".update_pkt_tsig_add()"
    pkt = ldns.ldns_pkt.new()
    resolver = ldns.ldns_resolver.new_frm_file("/etc/resolv.conf")
    try:
        ret = pkt.update_pkt_tsig_add(resolver)
        if ret != ldns.LDNS_STATUS_OK:
            set_error()
    except:
        set_error()
    try:
        ret = pkt.update_pkt_tsig_add("bad argument")
        set_error()
    except TypeError as e:
        pass
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".update_prcount()"
    pkt = ldns.ldns_pkt.new()
    try:
        ret = pkt.update_prcount()
        if (not isinstance(ret, int)) and (not isinstance(ret, long)):
            set_error()
        if ret != 0:
            set_error()
    except:
        set_error()
    pkt.update_set_prcount(127)
    try:
        ret = pkt.update_prcount()
        if (not isinstance(ret, int)) and (not isinstance(ret, long)):
            set_error()
        if ret != 127:
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".update_set_adcount()"
    pkt = ldns.ldns_pkt.new()
    try:
        pkt.update_set_adcount(4096)
        ret = pkt.update_ad()
        if ret != 4096:
            set_error()
    except:
        set_error()
    try:
        pkt.update_set_adcount("bad argument")
        set_error()
    except TypeError as e:
        pass
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".update_set_prcount()"
    pkt = ldns.ldns_pkt.new()
    try:
        pkt.update_set_prcount(4096)
        ret = pkt.update_prcount()
        if ret != 4096:
            set_error()
    except:
        set_error()
    try:
        pkt.update_set_prcount("bad argument")
        set_error()
    except TypeError as e:
        pass
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".update_set_upcount()"
    pkt = ldns.ldns_pkt.new()
    try:
        pkt.update_set_upcount(4096)
        ret = pkt.update_upcount()
        if ret != 4096:
            set_error()
    except:
        set_error()
    try:
        pkt.update_set_upcount("bad argument")
        set_error()
    except TypeError as e:
        pass
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".update_set_zo()"
    pkt = ldns.ldns_pkt.new()
    try:
        pkt.update_set_zo(4096)
        ret = pkt.update_zocount()
        if ret != 4096:
            set_error()
    except:
        set_error()
    try:
        pkt.update_set_zo("bad argument")
        set_error()
    except TypeError as e:
        pass
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".update_upcount()"
    pkt = ldns.ldns_pkt.new()
    try:
        ret = pkt.update_upcount()
        if (not isinstance(ret, int)) and (not isinstance(ret, long)):
            set_error()
        if ret != 0:
            set_error()
    except:
        set_error()
    pkt.update_set_upcount(127)
    try:
        ret = pkt.update_upcount()
        if (not isinstance(ret, int)) and (not isinstance(ret, long)):
            set_error()
        if ret != 127:
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".update_zocount()"
    pkt = ldns.ldns_pkt.new()
    try:
        ret = pkt.update_zocount()
        if (not isinstance(ret, int)) and (not isinstance(ret, long)):
            set_error()
        if ret != 0:
            set_error()
    except:
        set_error()
    pkt.update_set_zo(127)
    try:
        ret = pkt.update_zocount()
        if (not isinstance(ret, int)) and (not isinstance(ret, long)):
            set_error()
        if ret != 127:
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".write_to_buffer()"
    pkt = pkt = ldns.ldns_pkt.new_query_frm_str("test.nic.cz",ldns.LDNS_RR_TYPE_ANY, ldns.LDNS_RR_CLASS_IN, ldns.LDNS_QR | ldns.LDNS_AA)
    buf = buf = ldns.ldns_buffer(4096)
    try:
        ret = pkt.write_to_buffer(buf)
        if ret != ldns.LDNS_STATUS_OK:
            set_error()
    except:
        set_error()


if not error_detected:
    sys.stdout.write("%s: passed.\n" % (os.path.basename(__file__)))
else:
    sys.stdout.write("%s: errors detected.\n" % (os.path.basename(__file__)))
    sys.exit(1)
