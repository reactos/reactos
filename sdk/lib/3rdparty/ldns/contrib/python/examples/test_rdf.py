#!/usr/bin/env python

#
# ldns_rdf testing script.
#
# Do not use constructs that differ between Python 2 and 3.
# Use write on stdout or stderr.
#


import ldns
import sys
import os
import inspect


class_name = "ldns_rdf"
method_name = None
error_detected = False
temp_fname = "tmp_rdf.txt"


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
    try:
        # Should raise an Exception
        rdf = ldns.ldns_rdf()
        set_error()
    except Exception as e:
        pass


#if not error_detected:
if True:
    method_name = class_name + ".[comparison operators]"
    rdf1 = ldns.ldns_rdf.new_frm_str("0.0.0.0", ldns.LDNS_RDF_TYPE_A)
    rdf2 = ldns.ldns_rdf.new_frm_str("1.1.1.1", ldns.LDNS_RDF_TYPE_A)
    try:
        ret = rdf1 < rdf2
        if not isinstance(ret, bool):
            set_error()
        if ret != True:
            set_error()
    except:
        set_error()
    try:
        ret = rdf2 < rdf1
        if not isinstance(ret, bool):
            set_error()
        if ret != False:
            set_error()
    except:
        set_error()
    try:
        ret = rdf1 <= rdf2
        if not isinstance(ret, bool):
            set_error()
        if ret != True:
            set_error()
    except:
        set_error()
    try:
        ret = rdf2 <= rdf1
        if not isinstance(ret, bool):
            set_error()
        if ret != False:
            set_error()
    except:
        set_error()
    try:
        ret = rdf1 == rdf2
        if not isinstance(ret, bool):
            set_error()
        if ret != False:
            set_error()
    except:
        set_error()
    try:
        ret = rdf1 == rdf1
        if not isinstance(ret, bool):
            set_error()
        if ret != True:
            set_error()
    except:
        set_error()
    try:
        ret = rdf1 != rdf2
        if not isinstance(ret, bool):
            set_error()
        if ret != True:
            set_error()
    except:
        set_error()
    try:
        ret = rdf1 != rdf1
        if not isinstance(ret, bool):
            set_error()
        if ret != False:
            set_error()
    except:
        set_error()
    try:
        ret = rdf1 > rdf2
        if not isinstance(ret, bool):
            set_error()
        if ret != False:
            set_error()
    except:
        set_error()
    try:
        ret = rdf2 > rdf1
        if not isinstance(ret, bool):
            set_error()
        if ret != True:
            set_error()
    except:
        set_error()
    try:
        ret = rdf1 >= rdf2
        if not isinstance(ret, bool):
            set_error()
        if ret != False:
            set_error()
    except:
        set_error()
    try:
        ret = rdf2 >= rdf1
        if not isinstance(ret, bool):
            set_error()
        if ret != True:
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = "ldns_rdf_new()"
    sys.stderr.write("%s not tested.\n" % (method_name))


#if not error_detected:
if True:
    method_name = "ldns_rdf_new_frm_data()"
    sys.stderr.write("%s not tested.\n" % (method_name))


#if not error_detected:
if True:
    method_name = "ldns_rdf_new_frm_str()"
    try:
        rdf = ldns.ldns_rdf_new_frm_str(ldns.LDNS_RDF_TYPE_DNAME, "www.nic.cz")
    except:
        set_error()
    try:
        rdf = ldns.ldns_rdf_new_frm_str("", "www.nic.cz")
        et_error()
    except TypeError:
        pass
    except:
        set_error()
    try:
        rdf = ldns.ldns_rdf_new_frm_str(ldns.LDNS_RDF_TYPE_DNAME, 1)
    except TypeError:
        pass
    except:
        set_error()
    

#if not error_detected:
if True:
    method_name = "ldns_rdf_new_frm_fp()"
    f = open(temp_fname, "w")
    f.write("217.31.205.50")
    f.close()
    f = open(temp_fname, "r")
    try:
        status, rdf = ldns.ldns_rdf_new_frm_fp(ldns.LDNS_RDF_TYPE_A, f)
        if status != ldns.LDNS_STATUS_OK:
            set_error()
        if rdf == None:
            set_error()
    except:
        set_error()
    try:
        # Reading past file end.
        status, rdf = ldns.ldns_rdf_new_frm_fp(ldns.LDNS_RDF_TYPE_AAAA, f)
        if status == ldns.LDNS_STATUS_OK:
            set_error()
        if rdf != None:
            set_error()
    except:
        set_error()
    f.close()
    f = open(temp_fname, "r")
    try:
        status, rdf = ldns.ldns_rdf_new_frm_fp(ldns.LDNS_RDF_TYPE_AAAA, f)
        if status != ldns.LDNS_STATUS_OK:
            set_error()
        if rdf != None:
            set_error()
    except:
        set_error()
    f.close()
    os.remove(temp_fname)
    try:
        status, rdf = ldns.ldns_rdf_new_frm_fp("", f)
    except TypeError:
        pass
    except:
        set_error()
    try:
        status, rdf = ldns.ldns_rdf_new_frm_fp(ldns.LDNS_RDF_TYPE_AAAA, "")
    except TypeError:
        pass
    except:
        set_error()


#if not error_detected:
if True:
    method_name = "ldns_rdf_new_frm_fp_l()"
    f = open(temp_fname, "w")
    f.write("217.31.205.50\n194.0.12.1")
    f.close()
    f = open(temp_fname, "r")
    try:
        status, rdf, line = ldns.ldns_rdf_new_frm_fp_l(ldns.LDNS_RDF_TYPE_A, f)
        if status != ldns.LDNS_STATUS_OK:
            set_error()
        if rdf == None:
            set_error()
    except:
        set_error()
    try:
        status, rdf, line = ldns.ldns_rdf_new_frm_fp_l(ldns.LDNS_RDF_TYPE_A, f)
        if status != ldns.LDNS_STATUS_OK:
            set_error()
        if rdf == None:
            set_error()
    except:
        set_error()
    try:
        # Reading past file end.
        status, rdf, line = ldns.ldns_rdf_new_frm_fp_l(ldns.LDNS_RDF_TYPE_A, f)
        if status == ldns.LDNS_STATUS_OK:
            set_error()
        if rdf != None:
            set_error()
    except:
        set_error()
    f.close()
    os.remove(temp_fname)
    try:
        status, rdf = ldns.ldns_rdf_new_frm_fp_l("", f)
    except TypeError:
        pass
    except:
        set_error()
    try:
        status, rdf = ldns.ldns_rdf_new_frm_fp_l(ldns.LDNS_RDF_TYPE_AAAA, "")
    except TypeError:
        pass
    except:
        set_error()


#if not error_detected:
if True:
    method_name = "ldns_drf.absolute()"
    rdf = ldns.ldns_rdf_new_frm_str(ldns.LDNS_RDF_TYPE_DNAME, "www.nic.cz.")
    try:
        ret = rdf.absolute()
        if not isinstance(ret, bool):
            set_error()
        if not ret:
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = "ldns_rdf.address_reverse()"
    rdf = ldns.ldns_rdf_new_frm_str(ldns.LDNS_RDF_TYPE_A, "194.0.12.1")
    try:
        ret = rdf.address_reverse()
        if ret == None:
            set_error()
    except:
        set_error()
    rdf = ldns.ldns_rdf_new_frm_str(ldns.LDNS_RDF_TYPE_AAAA, "::1")
    try:
        ret = rdf.address_reverse()
        if ret == None:
            set_error()
    except:
        set_error()
    rdf = ldns.ldns_rdf_new_frm_str(ldns.LDNS_RDF_TYPE_DNAME, "www.nic.cz.")
    try:
        ret = rdf.address_reverse()
        if ret != None:
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = "ldns_rdf.cat()"
    rdf1 = ldns.ldns_rdf_new_frm_str(ldns.LDNS_RDF_TYPE_DNAME, "www.nic")
    rdf2 = ldns.ldns_rdf_new_frm_str(ldns.LDNS_RDF_TYPE_DNAME, "cz.")
    try:
        ret = rdf1.cat(rdf2)
        if ret != ldns.LDNS_STATUS_OK:
            set_error()
    except:
        set_error()
    rdf1 = ldns.ldns_rdf_new_frm_str(ldns.LDNS_RDF_TYPE_DNAME, "www.nic.")
    rdf2 = ldns.ldns_rdf_new_frm_str(ldns.LDNS_RDF_TYPE_A, "127.0.0.1")
    try:
        ret = rdf1.cat(rdf2)
        if ret == ldns.LDNS_STATUS_OK:
            set_error()
    except:
        set_error()
    try:
        ret = rdf2.cat(rdf1)
        if ret == ldns.LDNS_STATUS_OK:
            set_error()
    except:
        set_error()
    try:
        ret = rdf2.cat("")
        set_error()
    except TypeError:
        pass
    except:
        set_error()


#if not error_detected:
if True:
    method_name = "ldns_rdf.cat_clone()"
    rdf1 = ldns.ldns_rdf_new_frm_str(ldns.LDNS_RDF_TYPE_DNAME, "www.nic")
    rdf2 = ldns.ldns_rdf_new_frm_str(ldns.LDNS_RDF_TYPE_DNAME, "cz.")
    try:
        ret = rdf1.cat_clone(rdf2)
        if ret == None:
            set_error()
    except:
        set_error()
    rdf1 = ldns.ldns_rdf_new_frm_str(ldns.LDNS_RDF_TYPE_DNAME, "www.nic.")
    rdf2 = ldns.ldns_rdf_new_frm_str(ldns.LDNS_RDF_TYPE_A, "127.0.0.1")
    try:
        ret = rdf1.cat_clone(rdf2)
        if ret != None:
            set_error()
    except:
        set_error()
    try:
        ret = rdf2.cat_clone(rdf1)
        if ret != None:
            set_error()
    except:
        set_error()
    try:
        ret = rdf2.cat_clone("")
        set_error()
    except TypeError:
        pass
    except:
        set_error()


#if not error_detected:
if True:
    method_name = "ldns_rdf.clone()"
    rdf = ldns.ldns_rdf_new_frm_str(ldns.LDNS_RDF_TYPE_DNAME, "www.nic.cz.")
    try:
        ret = rdf.clone()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = "ldns_rdf.data()"
    rdf = ldns.ldns_rdf_new_frm_str(ldns.LDNS_RDF_TYPE_DNAME, "www.nic.cz.")
    try:
        ret = rdf.data()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = "ldns_rdf.data_as_bytearray()"
    rdf = ldns.ldns_rdf_new_frm_str(ldns.LDNS_RDF_TYPE_DNAME, "www.nic.cz.")
    try:
        ret = rdf.data_as_bytearray()
        if not isinstance(ret, bytearray):
            set_error()
        if len(ret) != 12:
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = "ldns_rdf.dname_compare()"
    rdf1 = ldns.ldns_rdf_new_frm_str(ldns.LDNS_RDF_TYPE_DNAME, "www.nic.cz.")
    rdf2 = ldns.ldns_rdf_new_frm_str(ldns.LDNS_RDF_TYPE_DNAME, "nic.cz.")
    try:
        ret = rdf1.dname_compare(rdf2)
        if ret != 1:
            set_error()
    except:
        set_error()
    try:
        ret = rdf2.dname_compare(rdf1)
        if ret != -1:
            set_error()
    except:
        set_error()
    try:
        ret = rdf1.dname_compare(rdf1)
        if ret != 0:
            set_error()
    except:
        set_error()
    rdf1 = ldns.ldns_rdf_new_frm_str(ldns.LDNS_RDF_TYPE_DNAME, "www.nic.cz.")
    rdf2 = ldns.ldns_rdf_new_frm_str(ldns.LDNS_RDF_TYPE_A, "127.0.0.1")
    try:
        ret = rdf1.dname_compare(rdf2)
        set_error()
    except Exception:
        pass
    except:
        set_error()


#if not error_detected:
if True:
    method_name = "ldns_rdf.dname_new_frm_str()"
    try:
        rdf = ldns.ldns_rdf.dname_new_frm_str("www.nic.cz.")
        if rdf == None:
            set_error()
    except:
        set_error()
    try:
        rdf = ldns.ldns_rdf.dname_new_frm_str("")
        if rdf != None:
            set_error()
    except:
        set_error()
    try:
        rdf = ldns.ldns_rdf.dname_new_frm_str(1)
        set_error()
    except TypeError:
        pass
    except:
        set_error()


#if not error_detected:
if True:
    method_name = "ldns_rdf.get_type()"
    rdf = ldns.ldns_rdf_new_frm_str(ldns.LDNS_RDF_TYPE_DNAME, "www.nic.cz.")
    try:
        ret = rdf.get_type()
        if not isinstance(ret, int):
            set_error()
        if ret != ldns.LDNS_RDF_TYPE_DNAME:
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = "ldns_rdf.get_type_str()"
    rdf = ldns.ldns_rdf_new_frm_str(ldns.LDNS_RDF_TYPE_DNAME, "www.nic.cz.")
    try:
        ret = rdf.get_type_str()
        if not isinstance(ret, str):
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = "ldns_rdf.interval()"
    rdf1 = ldns.ldns_rdf_new_frm_str(ldns.LDNS_RDF_TYPE_DNAME, "a.ns.nic.cz.")
    rdf2 = ldns.ldns_rdf_new_frm_str(ldns.LDNS_RDF_TYPE_DNAME, "b.ns.nic.cz.")
    rdf3 = ldns.ldns_rdf_new_frm_str(ldns.LDNS_RDF_TYPE_DNAME, "c.ns.nic.cz.")
    try:
        ret = rdf1.interval(rdf2, rdf3)
        if ret != -1:
            set_error()
    except:
        set_error()
    try:
        ret = rdf2.interval(rdf1, rdf3)
        if ret != 1:
            set_error()
    except:
        set_error()
    rdf1 = ldns.ldns_rdf_new_frm_str(ldns.LDNS_RDF_TYPE_A, "194.0.12.1")
    rdf2 = ldns.ldns_rdf_new_frm_str(ldns.LDNS_RDF_TYPE_DNAME, "b.ns.nic.cz.")
    rdf3 = ldns.ldns_rdf_new_frm_str(ldns.LDNS_RDF_TYPE_DNAME, "c.ns.nic.cz.")
    try:
        ret = rdf1.interval(rdf2, rdf3)
        set_error()
    except Exception:
        pass
    except:
        set_error()
    try:
        ret = rdf2.interval("", rdf3)
        set_error()
    except TypeError:
        pass
    except:
        set_error()

#if not error_detected:
if True:
    method_name = "ldns_rdf.is_subdomain()"
    rdf1 = ldns.ldns_rdf_new_frm_str(ldns.LDNS_RDF_TYPE_DNAME, "nic.cz.")
    rdf2 = ldns.ldns_rdf_new_frm_str(ldns.LDNS_RDF_TYPE_DNAME, "www.nic.cz.")
    try:
        ret = rdf1.is_subdomain(rdf2)
        if not isinstance(ret, bool):
            set_error()
        if ret == True:
            set_error()
        ret = rdf2.is_subdomain(rdf1)
        if ret != True:
            set_error()
    except:
        set_error()
    rdf1 = ldns.ldns_rdf_new_frm_str(ldns.LDNS_RDF_TYPE_A, "194.0.12.1")
    rdf2 = ldns.ldns_rdf_new_frm_str(ldns.LDNS_RDF_TYPE_DNAME, "www.nic.cz.")
    try:
        ret = rdf1.is_subdomain(rdf2)
        if ret != False:
            set_error()
    except:
        set_error()
    try:
        ret = rdf2.is_subdomain(rdf1)
        if ret != False:
            set_error()
    except:
        set_error()
    try:
        ret = rdf2.is_subdomain("")
        set_error()
    except TypeError:
        pass
    except:
        set_error()


#if not error_detected:
if True:
    method_name = "ldns_rdf.label()"
    rdf = ldns.ldns_rdf_new_frm_str(ldns.LDNS_RDF_TYPE_DNAME, "www.nic.cz.")
    try:
        ret = rdf.label(0)
        if not isinstance(ret, ldns.ldns_rdf):
            set_error()
    except:
        set_error()
    try:
        ret = rdf.label(10)
        if ret != None:
            set_error()
    except:
        set_error()
    try:
        ret = rdf.label("")
    except TypeError:
        pass
    except:
        set_error()
    rdf = ldns.ldns_rdf_new_frm_str(ldns.LDNS_RDF_TYPE_A, "127.0.0.1")
    try:
        ret = rdf.label(0)
        if ret != None:
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = "ldns_rdf.label_count()"
    rdf = ldns.ldns_rdf_new_frm_str(ldns.LDNS_RDF_TYPE_DNAME, "www.nic.cz.")
    try:
        ret = rdf.label_count()
        if (not isinstance(ret, int)) and (not isinstance(ret, long)):
            set_error()
        if ret != 3:
            set_error()
    except:
        set_error()
    rdf = ldns.ldns_rdf_new_frm_str(ldns.LDNS_RDF_TYPE_A, "127.0.0.1")
    try:
        ret = rdf.label_count()
        if (not isinstance(ret, int)) and (not isinstance(ret, long)):
            set_error()
        if ret != 0:
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = "ldns_rdf.left_chop()"
    rdf = ldns.ldns_rdf_new_frm_str(ldns.LDNS_RDF_TYPE_DNAME, "www.nic.cz.")
    try:
        ret = rdf.left_chop()
        if not isinstance(ret, ldns.ldns_rdf):
            set_error()
    except:
        set_error()
    rdf = ldns.ldns_rdf_new_frm_str(ldns.LDNS_RDF_TYPE_A, "127.0.0.1")
    try:
        ret = rdf.left_chop()
        if ret != None:
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = "ldns_rdf.make_canonical()"
    rdf = ldns.ldns_rdf_new_frm_str(ldns.LDNS_RDF_TYPE_DNAME, "WWW.NIC.CZ.")
    try:
        rdf.make_canonical()
        if rdf.__str__() != "www.nic.cz.":
            set_error()
    except:
        set_error()
    rdf = ldns.ldns_rdf_new_frm_str(ldns.LDNS_RDF_TYPE_A, "127.0.0.1")
    try:
        rdf.make_canonical()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = "ldns_rdf.new_frm_str()"
    try:
        rdf = ldns.ldns_rdf.new_frm_str("www.nic.cz.", ldns.LDNS_RDF_TYPE_DNAME)
    except:
        set_error()
    try:
        rdf = ldns.ldns_rdf.new_frm_str("www.nic.cz.", ldns.LDNS_RDF_TYPE_AAAA)
        set_error()
    except Exception:
        pass
    except:
        set_error()
    try:
        rdf = ldns.ldns_rdf.new_frm_str("www.nic.cz.", ldns.LDNS_RDF_TYPE_AAAA, raiseException = False)
        if rdf != None:
            set_error()
    except:
        set_error()
    try:
        rdf = ldns.ldns_rdf.new_frm_str("", "www.nic.cz")
        et_error()
    except TypeError:
        pass
    except:
        set_error()
    try:
        rdf = ldns.ldns_rdf.new_frm_str(ldns.LDNS_RDF_TYPE_DNAME, 1)
    except TypeError:
        pass
    except:
        set_error()


#if not error_detected:
if True:
    rdf = ldns.ldns_rdf_new_frm_str(ldns.LDNS_RDF_TYPE_A, "127.0.0.1")
    f = open(temp_fname, "w")
    try:
        rdf.print_to_file(f)
    except:
        set_error()
    f.close()
    f = open(temp_fname, "r")
    if f.read() != "127.0.0.1":
        set_error()
    f.close()
    os.remove(temp_fname)


#if not error_detected:
if True:
    rdf = ldns.ldns_rdf_new_frm_str(ldns.LDNS_RDF_TYPE_DNAME, "www.nic.cz.")
    try:
        ret = rdf.reverse()
        if not isinstance(ret, ldns.ldns_rdf):
            set_error()
        if ret.__str__() != "cz.nic.www.":
            set_error()
    except:
        set_error()
    rdf = ldns.ldns_rdf_new_frm_str(ldns.LDNS_RDF_TYPE_A, "127.0.0.1")
    try:
        ret = rdf.reverse()
        set_error()
    except Exception:
        pass
    except:
        set_error()


#if not error_detected:
if True:
    method_name = "ldns_rdf.set_data()"
    sys.stderr.write("%s not tested.\n" % (method_name))


#if not error_detected:
if True:
    method_name = "ldns_rdf.set_size()"
    sys.stderr.write("%s not tested.\n" % (method_name))


#if not error_detected:
if True:
    method_name = "ldns_rdf.set_type()"
    sys.stderr.write("%s not tested.\n" % (method_name))


#if not error_detected:
if True:
    method_name = "ldns_rdf.size()"
    rdf = ldns.ldns_rdf_new_frm_str(ldns.LDNS_RDF_TYPE_DNAME, "www.nic.cz.")
    try:
        ret = rdf.size()
        if ret != 12:
            set_error()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = "ldns_rdf.write_to_buffer()"
    rdf = ldns.ldns_rdf_new_frm_str(ldns.LDNS_RDF_TYPE_DNAME, "www.nic.cz.")
    buf = ldns.ldns_buffer(1024)
    try:
        ret = rdf.write_to_buffer(buf)
        if ret != ldns.LDNS_STATUS_OK:
            set_error()
        if buf.position() != 12:
            set_error()
    except:
        set_error()
    try:
        ret = rdf.write_to_buffer("")
    except TypeError:
        pass
    except:
        set_error()


#if not error_detected:
if True:
    method_name = "ldns_rdf.write_to_buffer_canonical()"
    rdf = ldns.ldns_rdf_new_frm_str(ldns.LDNS_RDF_TYPE_DNAME, "WWW.NIC.CZ.")
    buf = ldns.ldns_buffer(1024)
    try:
        ret = rdf.write_to_buffer_canonical(buf)
        if ret != ldns.LDNS_STATUS_OK:
            set_error()
        if buf.position() != 12:
            set_error()
    except:
        set_error()
    try:
        ret = rdf.write_to_buffer_canonical("")
    except TypeError:
        pass
    except:
        set_error()


if not error_detected:
    sys.stdout.write("%s: passed.\n" % (os.path.basename(__file__)))
else:
    sys.stdout.write("%s: errors detected.\n" % (os.path.basename(__file__)))
    sys.exit(1)
