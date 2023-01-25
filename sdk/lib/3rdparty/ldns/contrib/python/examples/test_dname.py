#!/usr/bin/env python

#
# ldns_dname testing script.
#
# Do not use constructs that differ between Python 2 and 3.
# Use write on stdout or stderr.
#


import ldns
import sys
import os
import inspect


class_name = "ldns_dname"
method_name = None
error_detected = False
temp_fname = "tmp_dname.txt"


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
    method_name = class_name + ".__init__()"
    rdf1 = ldns.ldns_rdf_new_frm_str(ldns.LDNS_RDF_TYPE_DNAME, "test.nic.cz.")
    rdf2 = ldns.ldns_rdf_new_frm_str(ldns.LDNS_RDF_TYPE_A, "217.31.205.50")
    try:
        dname = ldns.ldns_dname("www.nic.cz.")
        if not isinstance(dname, ldns.ldns_dname):
            set_error()
    except:
        set_error()
    #
    # Error when printing a dname which was created from an empty string.
    # Must find out why.
    #
    try:
        dname = ldns.ldns_dname(rdf1)
        if not isinstance(dname, ldns.ldns_dname):
            set_error()
    except:
        set_error()
    # Test whether rdf1 and dname independent.
    dname.cat(dname)
    if dname.__str__() == rdf1.__str__():
        set_error()
    # Test whether rdf1 and dname are dependent.
    dname = ldns.ldns_dname(rdf1, clone=False)
    dname.cat(dname)
    if dname.__str__() != rdf1.__str__():
        set_error()
    # Test whether constructs from non-dname rdfs.
    try:
        dname = ldns.ldns_dname(rdf2)
        set_error()
    except TypeError:
        pass
    except:
        set_error()
    try:
        dname = ldns.ldns_dname(1)
        set_error()
    except TypeError:
        pass
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".[comparison operators]"
    dn1 = ldns.ldns_dname("a.test")
    dn2 = ldns.ldns_dname("b.test")
    try:
        ret = dn1 < dn2
        if not isinstance(ret, bool):
            set_error()
        if ret != True:
            set_error()
    except:
        set_error()
    try:
        ret = dn2 < dn1
        if not isinstance(ret, bool):
            set_error()
        if ret != False:
            set_error()
    except:
        set_error()
    try:
        ret = dn1 <= dn2
        if not isinstance(ret, bool):
            set_error()
        if ret != True:
            set_error()
    except:
        set_error()
    try:
        ret = dn2 <= dn1
        if not isinstance(ret, bool):
            set_error()
        if ret != False:
            set_error()
    except:
        set_error()
    try:
        ret = dn1 == dn2
        if not isinstance(ret, bool):
            set_error()
        if ret != False:
            set_error()
    except:
        set_error()
    try:
        ret = dn1 == dn1
        if not isinstance(ret, bool):
            set_error()
        if ret != True:
            set_error()
    except:
        set_error()
    try:
        ret = dn1 != dn2
        if not isinstance(ret, bool):
            set_error()
        if ret != True:
            set_error()
    except:
        set_error()
    try:
        ret = dn1 != dn1
        if not isinstance(ret, bool):
            set_error()
        if ret != False:
            set_error()
    except:
        set_error()
    try:
        ret = dn1 > dn2
        if not isinstance(ret, bool):
            set_error()
        if ret != False:
            set_error()
    except:
        set_error()
    try:
        ret = dn2 > dn1
        if not isinstance(ret, bool):
            set_error()
        if ret != True:
            set_error()
    except:
        set_error()
    try:
        ret = dn1 >= dn2
        if not isinstance(ret, bool):
            set_error()
        if ret != False:
            set_error()
    except:
        set_error()
    try:
        ret = dn2 >= dn1
        if not isinstance(ret, bool):
            set_error()
        if ret != True:
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".absolute()"
    dname = ldns.ldns_dname("www.nic.cz.")
    try:
        ret = dname.absolute()
        if not isinstance(ret, bool):
            set_error()
        if ret != True:
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".cat()"
    rdf1 = ldns.ldns_rdf_new_frm_str(ldns.LDNS_RDF_TYPE_DNAME, "test.nic.cz.")
    rdf2 = ldns.ldns_rdf_new_frm_str(ldns.LDNS_RDF_TYPE_A, "217.31.205.50")
    dname = ldns.ldns_dname("www.nic.cz.")
    try:
        ret = dname.cat(dname)
        if ret != ldns.LDNS_STATUS_OK:
            set_error()
        if dname.__str__() != "www.nic.cz.www.nic.cz.":
            set_error()
    except:
        set_error()
    try:
        ret = dname.cat(rdf1)
        if ret != ldns.LDNS_STATUS_OK:
            set_error()
        if dname.__str__() != "www.nic.cz.www.nic.cz.test.nic.cz.":
            set_error()
    except:
        set_error()
    try:
        ret = dname.cat(rdf2)
        if ret == ldns.LDNS_STATUS_OK:
            set_error()
    except:
        set_error()
    try:
        ret = dname.cat("")
        set_error()
    except TypeError:
        pass
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".cat_clone()"
    rdf1 = ldns.ldns_rdf_new_frm_str(ldns.LDNS_RDF_TYPE_DNAME, "test.nic.cz.")
    rdf2 = ldns.ldns_rdf_new_frm_str(ldns.LDNS_RDF_TYPE_A, "217.31.205.50")
    dname = ldns.ldns_dname("www.nic.cz.")
    try:
        ret = dname.cat_clone(dname)
        if not isinstance(ret, ldns.ldns_dname):
            set_error()
        if ret.__str__() != "www.nic.cz.www.nic.cz.":
            set_error()
    except:
        set_error()
    try:
        ret = dname.cat_clone(rdf1)
        if not isinstance(ret, ldns.ldns_dname):
            set_error()
        if ret.__str__() != "www.nic.cz.test.nic.cz.":
            set_error()
    except:
        set_error()
    try:
        ret = dname.cat_clone(rdf2)
        if ret != None:
            set_error()
    except:
        set_error()
    try:
        ret = dname.cat_clone("")
    except TypeError:
        pass
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".interval()"
    dn1 = ldns.ldns_dname("a.ns.nic.cz.")
    dn2 = ldns.ldns_dname("b.ns.nic.cz.")
    dn3 = ldns.ldns_dname("c.ns.nic.cz.")
    try:
        ret = dn1.interval(dn2, dn3)
        if ret != -1:
            set_error()
    except:
        set_error()
    try:
        ret = dn2.interval(dn1, dn3)
        if ret != 1:
            set_error()
    except:
        set_error()
    rdf4 = ldns.ldns_rdf_new_frm_str(ldns.LDNS_RDF_TYPE_DNAME, "d.ns.nic.cz.")
    rdf5 = ldns.ldns_rdf_new_frm_str(ldns.LDNS_RDF_TYPE_A, "194.0.12.1")
    try:
        ret = dn1.interval(dn2, rdf4)
        if ret != -1:
            set_error()
    except:
        set_error()
    try:
        ret = dn2.interval(dn1, rdf4)
        if ret != 1:
            set_error()
    except:
        set_error()
    try:
        ret = dn1.interval(dn2, rdf5)
        set_error()
    except Exception:
        pass
    except:
        set_error()
    try:
        ret = dn1.interval(dn2, "")
        set_error()
    except TypeError:
        pass
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".is_subdomain()"
    dn1 = ldns.ldns_dname("nic.cz.")
    dn2 = ldns.ldns_dname("www.nic.cz.")
    rdf3 = ldns.ldns_rdf_new_frm_str(ldns.LDNS_RDF_TYPE_DNAME, "www.nic.cz.")
    try:
        ret = dn1.is_subdomain(dn2)
        if not isinstance(ret, bool):
            set_error()
        if ret == True:
            set_error()
        ret = dn2.is_subdomain(dn1)
        if ret != True:
            set_error()
    except:
        set_error()
    try:
        ret = dn1.is_subdomain(rdf3)
        if not isinstance(ret, bool):
            set_error()
        if ret == True:
            set_error()
    except:
        set_error()
    rdf4 = ldns.ldns_rdf_new_frm_str(ldns.LDNS_RDF_TYPE_A, "194.0.12.1")
    try:
        ret = dn1.is_subdomain(rdf4)
        if ret != False:
            set_error()
    except:
        set_error()
    try:
        ret = dn1.is_subdomain("")
        set_error()
    except TypeError:
        pass
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".label()"
    dn = ldns.ldns_dname("nic.cz.")
    try:
        ret = dn.label(0)
        if not isinstance(ret, ldns.ldns_dname):
            set_error()
    except:
        set_error()
    try:
        ret = dn.label(10)
        if ret != None:
            set_error()
    except:
        set_error()
    try:
        ret = dn.label("")
    except TypeError:
        pass
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".label_count()"
    dn = ldns.ldns_dname("www.nic.cz.")
    try:
        ret = dn.label_count()
        if (not isinstance(ret, int)) and (not isinstance(ret, long)):
            set_error()
        if ret != 3:
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".left_chop()"
    dn = ldns.ldns_dname("www.nic.cz.")
    try:
        ret = dn.left_chop()
        if not isinstance(ret, ldns.ldns_dname):
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".make_canonical()"
    dn = ldns.ldns_dname("WWW.NIC.CZ.")
    try:
        dn.make_canonical()
        if dn.__str__() != "www.nic.cz.":
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".new_frm_rdf()"
    # Tested via constructor call.


#if not error_detected:
if True:
    method_name = class_name + ".new_frm_str()"
    # Tested via constructor call.


#if not error_detected:
if True:
    method_name = class_name + ".reverse()"
    dn = ldns.ldns_dname("www.nic.cz.")
    try:
        ret = dn.reverse()
        if not isinstance(ret, ldns.ldns_dname):
            set_error()
        if ret.__str__() != "cz.nic.www.":
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = class_name + ".write_to_buffer()"
    dn = ldns.ldns_dname("www.nic.cz.")
    buf = ldns.ldns_buffer(1024)
    try:
        ret = dn.write_to_buffer(buf)
        if ret != ldns.LDNS_STATUS_OK:
            set_error()
        if buf.position() != 12:
            set_error()
    except:
        set_error()
    try:
        ret = dn.write_to_buffer("")
    except TypeError:
        pass
    except:
        set_error()


if not error_detected:
    sys.stdout.write("%s: passed.\n" % (os.path.basename(__file__)))
else:
    sys.stdout.write("%s: errors detected.\n" % (os.path.basename(__file__)))
    sys.exit(1)
