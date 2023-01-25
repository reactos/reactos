/*
 * ldns.i: LDNS interface file
 *
 * Copyright (c) 2009, Zdenek Vasicek (vasicek AT fit.vutbr.cz)
 *                     Karel Slany    (slany AT fit.vutbr.cz)
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 *     * Redistributions of source code must retain the above copyright notice,
 *       this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the organization nor the names of its
 *       contributors may be used to endorse or promote products derived from this
 *       software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

%module ldns
#pragma SWIG nowarn=454
%{

#include "ldns.h"
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#include <ldns/util.h>
#include <ldns/buffer.h>
#include <ldns/common.h>
#include <ldns/dname.h>
#include <ldns/dnssec.h>
#include <ldns/dnssec_verify.h>
#include <ldns/dnssec_sign.h>
#include <ldns/error.h>
#include <ldns/higher.h>
#include <ldns/host2str.h>
#include <ldns/host2wire.h>
#include <ldns/net.h>
#include <ldns/packet.h>
#include <ldns/rdata.h>
#include <ldns/resolver.h>
#include <ldns/rr.h>
#include <ldns/str2host.h>
#include <ldns/tsig.h>
#include <ldns/update.h>
#include <ldns/wire2host.h>
#include <ldns/rr_functions.h>
#include <ldns/keys.h>
#include <ldns/parse.h>
#include <ldns/zone.h>
#include <ldns/dnssec_zone.h>
#include <ldns/rbtree.h>
%}

//#define LDNS_DEBUG
//#define SWIG_FILE3_DEBUG

%include "stdint.i" // uint_16_t is known type now
#ifdef PY3
%include "file_py3.i" // python 3 FILE *
#else
%include "file.i"     // FILE * 
#endif
%include "typemaps.i"


/* ========================================================================= */
/* Preliminary Python code. */
/* ========================================================================= */

%pythoncode
%{
	#
	# Use and don't ignore DeprecationWarning and
	# PendingDeprecationWarning.
	#
	import warnings
	warnings.filterwarnings("module", category=DeprecationWarning)
	warnings.filterwarnings("module", category=PendingDeprecationWarning)
%}


/* Tell SWIG how to handle ssize_t as input parameter. */
%typemap(in, noblock=1) (ssize_t)
{
  int $1_res = 0;
  $1_res = SWIG_AsVal_long($input, &$1);
  if (!SWIG_IsOK($1_res)) {
    SWIG_exception_fail(SWIG_ArgError($1_res), "in method '"
      "$symname" "', argument " "$argnum" " of type '"
      "$type""'");
  }
}


%inline %{
struct timeval* ldns_make_timeval(uint32_t sec, uint32_t usec)
{
        struct timeval* res = (struct timeval*)malloc(sizeof(*res));
        res->tv_sec = sec;
        res->tv_usec = usec;
        return res;
}
uint32_t ldns_read_timeval_sec(struct timeval* t) { 
        return (uint32_t)t->tv_sec; }
uint32_t ldns_read_timeval_usec(struct timeval* t) { 
        return (uint32_t)t->tv_usec; }
%}

%immutable ldns_struct_lookup_table::name;
%immutable ldns_struct_rr_descriptor::_name;
%immutable ldns_error_str;
%immutable ldns_signing_algorithms;
%immutable ldns_tsig_credentials_struct::algorithm;
%immutable ldns_tsig_credentials_struct::keyname;
%immutable ldns_tsig_credentials_struct::keydata;

//*_new_frm_fp_l
%apply int *OUTPUT { (int *line_nr) };

%apply uint32_t *OUTPUT { uint32_t *default_ttl};

// wire2pkt
%apply (char *STRING, int LENGTH) { (const char *str, int len) };

%include "ldns_packet.i"
%include "ldns_resolver.i"
%include "ldns_rr.i"

%include <ldns/rr.h>

%inline %{
int Python_str_Check(PyObject *o) {
#if PY_VERSION_HEX>=0x03000000
  return PyUnicode_Check(o);
#else
  return PyString_Check(o);
#endif
}
%}

%include "ldns_rdf.i"
%include "ldns_zone.i"
%include "ldns_key.i"
%include "ldns_buffer.i"
%include "ldns_dnssec.i"

%include <ldns/util.h>
  %include <ldns/buffer.h>
%include <ldns/dnssec.h>
%include <ldns/dnssec_verify.h>
  %include <ldns/dnssec_sign.h>
%include <ldns/error.h>
%include <ldns/higher.h>
  %include <ldns/host2str.h>
  %include <ldns/host2wire.h>
%include <ldns/net.h>
  %include <ldns/packet.h>
  %include <ldns/rdata.h>
  %include <ldns/resolver.h>
%include <ldns/str2host.h>
%include <ldns/tsig.h>
  %include <ldns/update.h>
%include <ldns/wire2host.h>
  %include <ldns/rr_functions.h>
  %include <ldns/keys.h>
%include <ldns/parse.h>
  %include <ldns/zone.h>
  %include <ldns/dnssec_zone.h>
%include <ldns/rbtree.h>
  %include <ldns/dname.h>

typedef struct ldns_dnssec_name { };
typedef struct ldns_dnssec_rrs { };
typedef struct ldns_dnssec_rrsets { };
typedef struct ldns_dnssec_zone { };
// ================================================================================

%include "ldns_dname.i"

%inline %{
 PyObject* ldns_rr_new_frm_str_(const char *str, uint32_t default_ttl, ldns_rdf* origin, ldns_rdf* prev) 
 //returns tuple (status, ldns_rr, prev)
 {
   PyObject* tuple;

   /*  origin and prev have to be cloned in order to decouple the data
    *  from the python wrapper
    */
   if (origin != NULL)
       origin = ldns_rdf_clone(origin);
   if (prev != NULL)
       prev = ldns_rdf_clone(prev);

   ldns_rdf *p_prev = prev;
   ldns_rdf **pp_prev = &p_prev;
   if (p_prev == 0) pp_prev = 0;

   ldns_rr *p_rr = 0;
   ldns_rr **pp_rr = &p_rr;

   ldns_status st = ldns_rr_new_frm_str(pp_rr, str, default_ttl, origin, pp_prev);

   tuple = PyTuple_New(3);
   PyTuple_SetItem(tuple, 0, SWIG_From_int(st)); 
   PyTuple_SetItem(tuple, 1, (st == LDNS_STATUS_OK) ? 
                             SWIG_NewPointerObj(SWIG_as_voidptr(p_rr), SWIGTYPE_p_ldns_struct_rr, SWIG_POINTER_OWN |  0 ) : 
		   (Py_INCREF(Py_None), Py_None));
   PyTuple_SetItem(tuple, 2, (p_prev != prev) ? 
                             SWIG_NewPointerObj(SWIG_as_voidptr(p_prev), SWIGTYPE_p_ldns_struct_rdf, SWIG_POINTER_OWN |  0 ) :
		   (Py_INCREF(Py_None), Py_None));
   return tuple;
 }

 PyObject* ldns_rr_new_frm_fp_(FILE *fp, uint32_t default_ttl,  ldns_rdf* origin,  ldns_rdf* prev) 
 //returns tuple (status, ldns_rr, ttl, origin, prev)
 {
   uint32_t defttl = default_ttl;
   uint32_t *p_defttl = &defttl;
   if (defttl == 0) p_defttl = 0;

   /*  origin and prev have to be cloned in order to decouple the data
    *  from the python wrapper
    */
   if (origin != NULL)
       origin = ldns_rdf_clone(origin);
   if (prev != NULL)
       prev = ldns_rdf_clone(prev);

   ldns_rdf *p_origin = origin;
   ldns_rdf **pp_origin = &p_origin;
   //if (p_origin == 0) pp_origin = 0;

   ldns_rdf *p_prev = prev;
   ldns_rdf **pp_prev = &p_prev;
   //if (p_prev == 0) pp_prev = 0;

   ldns_rr *p_rr = 0;
   ldns_rr **pp_rr = &p_rr;

   ldns_status st = ldns_rr_new_frm_fp(pp_rr, fp, p_defttl, pp_origin, pp_prev);

   PyObject* tuple;
   tuple = PyTuple_New(5);
   int idx = 0;
   PyTuple_SetItem(tuple, idx, SWIG_From_int(st)); 
   idx++;
   PyTuple_SetItem(tuple, idx, (st == LDNS_STATUS_OK) ? 
                             SWIG_NewPointerObj(SWIG_as_voidptr(p_rr), SWIGTYPE_p_ldns_struct_rr, SWIG_POINTER_OWN |  0 ) : 
		   (Py_INCREF(Py_None), Py_None));
   idx++;
   PyTuple_SetItem(tuple, idx, SWIG_From_int(defttl));
   idx++;
   PyTuple_SetItem(tuple, idx, SWIG_NewPointerObj(SWIG_as_voidptr(p_origin), SWIGTYPE_p_ldns_struct_rdf, SWIG_POINTER_OWN |  0 ));
   idx++;
   PyTuple_SetItem(tuple, idx, SWIG_NewPointerObj(SWIG_as_voidptr(p_prev), SWIGTYPE_p_ldns_struct_rdf, SWIG_POINTER_OWN |  0 ));
   return tuple;
 }

 PyObject* ldns_rr_new_frm_fp_l_(FILE *fp, uint32_t default_ttl,  ldns_rdf* origin,  ldns_rdf* prev) 
 //returns tuple (status, ldns_rr, line, ttl, origin, prev)
 {
   int linenr = 0;
   int *p_linenr = &linenr;

   uint32_t defttl = default_ttl;
   uint32_t *p_defttl = &defttl;
   if (defttl == 0) p_defttl = 0;

   /*  origin and prev have to be cloned in order to decouple the data
    *  from the python wrapper
    */
   if (origin != NULL)
       origin = ldns_rdf_clone(origin);
   if (prev != NULL)
       prev = ldns_rdf_clone(prev);

   ldns_rdf *p_origin = origin;
   ldns_rdf **pp_origin = &p_origin;
   //if (p_origin == 0) pp_origin = 0;

   ldns_rdf *p_prev = prev;
   ldns_rdf **pp_prev = &p_prev;
   //if (p_prev == 0) pp_prev = 0;

   ldns_rr *p_rr = 0;
   ldns_rr **pp_rr = &p_rr;

   ldns_status st = ldns_rr_new_frm_fp_l(pp_rr, fp, p_defttl, pp_origin, pp_prev, p_linenr);

   PyObject* tuple;
   tuple = PyTuple_New(6);
   int idx = 0;
   PyTuple_SetItem(tuple, idx, SWIG_From_int(st)); 
   idx++;
   PyTuple_SetItem(tuple, idx, (st == LDNS_STATUS_OK) ? 
                             SWIG_NewPointerObj(SWIG_as_voidptr(p_rr), SWIGTYPE_p_ldns_struct_rr, SWIG_POINTER_OWN |  0 ) : 
		   (Py_INCREF(Py_None), Py_None));
   idx++;
   PyTuple_SetItem(tuple, idx, SWIG_From_int(linenr));
   idx++;
   PyTuple_SetItem(tuple, idx, SWIG_From_int(defttl));
   idx++;
   PyTuple_SetItem(tuple, idx, SWIG_NewPointerObj(SWIG_as_voidptr(p_origin), SWIGTYPE_p_ldns_struct_rdf, SWIG_POINTER_OWN |  0 ));
   idx++;
   PyTuple_SetItem(tuple, idx, SWIG_NewPointerObj(SWIG_as_voidptr(p_prev), SWIGTYPE_p_ldns_struct_rdf, SWIG_POINTER_OWN |  0 ));
   return tuple;
 }


PyObject* ldns_rr_new_question_frm_str_(const char *str, ldns_rdf* origin, ldns_rdf* prev) 
 //returns tuple (status, ldns_rr, prev)
 {
   PyObject* tuple;

   /*  origin and prev have to be cloned in order to decouple the data
    *  from the python wrapper
    */
   if (origin != NULL)
       origin = ldns_rdf_clone(origin);
   if (prev != NULL)
       prev = ldns_rdf_clone(prev);

   ldns_rdf *p_prev = prev;
   ldns_rdf **pp_prev = &p_prev;
   if (p_prev == 0) pp_prev = 0;

   ldns_rr *p_rr = 0;
   ldns_rr **pp_rr = &p_rr;

   ldns_status st = ldns_rr_new_question_frm_str(pp_rr, str, origin, pp_prev);

   tuple = PyTuple_New(3);
   PyTuple_SetItem(tuple, 0, SWIG_From_int(st)); 
   PyTuple_SetItem(tuple, 1, (st == LDNS_STATUS_OK) ? 
                             SWIG_NewPointerObj(SWIG_as_voidptr(p_rr), SWIGTYPE_p_ldns_struct_rr, SWIG_POINTER_OWN |  0 ) : 
		   (Py_INCREF(Py_None), Py_None));
   PyTuple_SetItem(tuple, 2, (p_prev != prev) ? 
                             SWIG_NewPointerObj(SWIG_as_voidptr(p_prev), SWIGTYPE_p_ldns_struct_rdf, SWIG_POINTER_OWN |  0 ) :
		   (Py_INCREF(Py_None), Py_None));
   return tuple;
 }



PyObject* ldns_fetch_valid_domain_keys_(const ldns_resolver * res, const ldns_rdf * domain,
		const ldns_rr_list * keys)
 //returns tuple (status, result)
 {
   PyObject* tuple;

   ldns_rr_list *rrl = 0;
   ldns_status st = 0;
   rrl = ldns_fetch_valid_domain_keys(res, domain, keys, &st);


   tuple = PyTuple_New(2);
   PyTuple_SetItem(tuple, 0, SWIG_From_int(st)); 
   PyTuple_SetItem(tuple, 1, (st == LDNS_STATUS_OK) ? 
                             SWIG_NewPointerObj(SWIG_as_voidptr(rrl), SWIGTYPE_p_ldns_struct_rr_list, SWIG_POINTER_OWN |  0 ) : 
		   (Py_INCREF(Py_None), Py_None));
   return tuple;
 }

PyObject* ldns_wire2pkt_(const char *str, int len)
 //returns tuple (status, result)
 {
  PyObject *resultobj = 0;
  ldns_pkt *arg1 = NULL;
  uint8_t *arg2 = (uint8_t *) str;
  size_t arg3 = (size_t) len;
  ldns_status result;
  PyObject* tuple;
  
  result = (ldns_status)ldns_wire2pkt(&arg1,arg2,arg3);
  tuple = PyTuple_New(2);
  PyTuple_SetItem(tuple, 0, SWIG_From_int(result));
  if (result == LDNS_STATUS_OK)
     PyTuple_SetItem(tuple, 1, SWIG_NewPointerObj(SWIG_as_voidptr(arg1), SWIGTYPE_p_ldns_struct_pkt, SWIG_POINTER_OWN |  0 ));
  else {
     Py_INCREF(Py_None);
     PyTuple_SetItem(tuple, 1, Py_None);
  }
  return tuple;
}

PyObject* ldns_pkt2wire_(const ldns_pkt *pkt)
 //returns tuple (status, result)
 {
  PyObject *resultobj = 0;
  uint8_t *arg1 = NULL;
  size_t arg3;
  ldns_status result;
  PyObject* tuple;

  result = (ldns_status)ldns_pkt2wire(&arg1,pkt,&arg3);
  tuple = PyTuple_New(2);
  PyTuple_SetItem(tuple, 0, SWIG_From_int(result));
  if (result == LDNS_STATUS_OK)
     PyTuple_SetItem(tuple, 1, SWIG_FromCharPtrAndSize((char *)arg1, arg3));
  else {
     Py_INCREF(Py_None);
     PyTuple_SetItem(tuple, 1, Py_None);
  }
  LDNS_FREE(arg1);
  return tuple;
}

%}

%pythoncode %{
def ldns_fetch_valid_domain_keys(res, domain, keys):
    return _ldns.ldns_fetch_valid_domain_keys_(res, domain, keys)

def ldns_wire2pkt(data):
    return _ldns.ldns_wire2pkt_(data)

def ldns_pkt2wire(data):
    return _ldns.ldns_pkt2wire_(data)

def ldns_rr_iter_frm_fp_l(input_file):
    """Creates an iterator (generator) that returns individual parsed
    RRs from an open zone file."""
    # variables that preserve the parsers state
    my_ttl = 0;
    my_origin = None
    my_prev = None
    # additional state variables
    last_pos = 0
    line_nr = 0

    while True:
        ret = _ldns.ldns_rr_new_frm_fp_l_(input_file, my_ttl, my_origin, my_prev)
        s, rr, line_inc, new_ttl, new_origin, new_prev = ret  # unpack the result
        line_nr += line_inc # increase number of parsed lines
        my_prev = new_prev  # update ref to previous owner

        if s == _ldns.LDNS_STATUS_SYNTAX_TTL:
            my_ttl = new_ttl  # update default TTL
        elif s == _ldns.LDNS_STATUS_SYNTAX_ORIGIN:
            my_origin = new_origin  # update reference to origin
        elif s == _ldns.LDNS_STATUS_SYNTAX_EMPTY:
            if last_pos == input_file.tell():
                break  # no advance since last read - EOF
            last_pos = input_file.tell()
        elif s != _ldns.LDNS_STATUS_OK:
            raise ValueError("Parse error in line %d" % line_nr)
        else:
            # we are sure to have LDNS_STATUS_OK
            yield rr

%}
