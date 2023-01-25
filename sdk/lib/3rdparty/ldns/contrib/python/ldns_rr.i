/******************************************************************************
 * ldns_rr.i: LDNS resource records (RR), RR list
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
 *       contributors may be used to endorse or promote products derived from
 *       this software without specific prior written permission.
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
 *****************************************************************************/


/* ========================================================================= */
/* SWIG setting and definitions. */
/* ========================================================================= */

/* Creates a temporary instance of (ldns_rr *). */
%typemap(in, numinputs=0, noblock=1) (ldns_rr **)
{
  ldns_rr *$1_rr;
  $1 = &$1_rr;
}
          
/* Result generation, appends (ldns_rr *) after the result. */
%typemap(argout, noblock=1) (ldns_rr **) 
{
  $result = SWIG_Python_AppendOutput($result,
    SWIG_NewPointerObj(SWIG_as_voidptr($1_rr),
      SWIGTYPE_p_ldns_struct_rr, SWIG_POINTER_OWN |  0 ));
}

%nodefaultctor ldns_struct_rr; /* No default constructor. */
%nodefaultdtor ldns_struct_rr; /* No default destructor. */

%ignore ldns_struct_rr::_rdata_fields;

%newobject ldns_rr_clone;
%newobject ldns_rr_new;
%newobject ldns_rr_new_frm_type;
%newobject ldns_rr_pop_rdf;
%delobject ldns_rr_free;

%rename(ldns_rr) ldns_struct_rr;

%newobject ldns_rr2str;
%newobject ldns_rr_type2str;
%newobject ldns_rr_class2str;
%newobject ldns_read_anchor_file;


/* Clone rdf data on pull. */

/* Clone will fail with NULL argument. */

%newobject _ldns_rr_rdf;
%rename(__ldns_rr_rdf) ldns_rr_rdf;
%inline
%{
  ldns_rdf * _ldns_rr_rdf(ldns_rr *rr, size_t i)
  {
    ldns_rdf *rdf;
    rdf = ldns_rr_rdf(rr, i);
    return (rdf != NULL) ? ldns_rdf_clone(rdf) : NULL;
  }
%}

%newobject _ldns_rr_rrsig_algorithm;
%rename(__ldns_rr_rrsig_algorithm) ldns_rr_rrsig_algorithm;
%inline
%{
  ldns_rdf * _ldns_rr_rrsig_algorithm(ldns_rr *rr) {
    ldns_rdf *rdf;
    rdf = ldns_rr_rrsig_algorithm(rr);
    return (rdf != NULL) ? ldns_rdf_clone(rdf) : NULL;
  }
%}

%newobject _ldns_rr_dnskey_algorithm;
%rename(__ldns_rr_dnskey_algorithm) ldns_rr_dnskey_algorithm;
%inline
%{
  ldns_rdf * _ldns_rr_dnskey_algorithm(ldns_rr *rr)
  {
    ldns_rdf *rdf;
    rdf = ldns_rr_dnskey_algorithm(rr);
    return (rdf != NULL) ? ldns_rdf_clone(rdf) : NULL;
  }
%}

%newobject _ldns_rr_dnskey_flags;
%rename(__ldns_rr_dnskey_flags) ldns_rr_dnskey_flags;
%inline
 %{
  ldns_rdf * _ldns_rr_dnskey_flags(ldns_rr *rr)
  {
    ldns_rdf *rdf;
    rdf = ldns_rr_dnskey_flags(rr);
    return (rdf != NULL) ? ldns_rdf_clone(rdf) : NULL;
  }
%}

%newobject _ldns_rr_dnskey_key;
%rename(__ldns_rr_dnskey_key) ldns_rr_dnskey_key;
%inline
%{
  ldns_rdf * _ldns_rr_dnskey_key(ldns_rr *rr)
  {
    ldns_rdf *rdf;
    rdf = ldns_rr_dnskey_key(rr);
    return (rdf != NULL) ? ldns_rdf_clone(rdf) : NULL;
  }
%}

%newobject _ldns_rr_dnskey_protocol;
%rename(__ldns_rr_dnskey_protocol) ldns_rr_dnskey_protocol;
%inline
%{
  ldns_rdf * _ldns_rr_dnskey_protocol(ldns_rr *rr)
  {
    ldns_rdf *rdf;
    rdf = ldns_rr_dnskey_protocol(rr);
    return (rdf != NULL) ? ldns_rdf_clone(rdf) : NULL;
  }
%}


%newobject _ldns_rr_owner;
%rename(__ldns_rr_owner) ldns_rr_owner;
%inline
%{
  ldns_rdf * _ldns_rr_owner(ldns_rr *rr)
  {
    ldns_rdf *rdf;
    rdf = ldns_rr_owner(rr);
    return (rdf != NULL) ? ldns_rdf_clone(rdf) : NULL;
  }
%}


%newobject _ldns_rr_a_address;
%rename(__ldns_rr_a_address) ldns_rr_a_address;
%inline
%{
  ldns_rdf * _ldns_rr_a_address(ldns_rr *rr)
  {
    ldns_rdf *rdf;
    rdf = ldns_rr_a_address(rr);
    return (rdf != NULL) ? ldns_rdf_clone(rdf) : NULL;
  }
%}


%newobject _ldns_rr_mx_exchange;
%rename(__ldns_rr_mx_exchange) ldns_rr_mx_exchange;
%inline
%{
  ldns_rdf * _ldns_rr_mx_exchange(ldns_rr *rr)
  {
    ldns_rdf *rdf;
    rdf = ldns_rr_mx_exchange(rr);
    return (rdf != NULL) ? ldns_rdf_clone(rdf) : NULL;
  }
%}


%newobject _ldns_rr_mx_preference;
%rename(__ldns_rr_mx_preference) ldns_rr_mx_preference;
%inline
%{
  ldns_rdf * _ldns_rr_mx_preference(ldns_rr *rr)
  {
    ldns_rdf *rdf;
    rdf = ldns_rr_mx_preference(rr);
    return (rdf != NULL) ? ldns_rdf_clone(rdf) : NULL;
  }
%}


%newobject _ldns_rr_ns_nsdname;
%rename(__ldns_rr_ns_nsdname) ldns_rr_ns_nsdname;
%inline
%{
  ldns_rdf * _ldns_rr_ns_nsdname(ldns_rr *rr)
  {
    ldns_rdf *rdf;
    rdf = ldns_rr_ns_nsdname(rr);
    return (rdf != NULL) ? ldns_rdf_clone(rdf) : NULL;
  }
%}


%newobject _ldns_rr_rrsig_expiration;
%rename(__ldns_rr_rrsig_expiration) ldns_rr_rrsig_expiration;
%inline
%{
  ldns_rdf * _ldns_rr_rrsig_expiration(ldns_rr *rr)
  {
    ldns_rdf *rdf;
    rdf = ldns_rr_rrsig_expiration(rr);
    return (rdf != NULL) ? ldns_rdf_clone(rdf) : NULL;
  }
%}


%newobject _ldns_rr_rrsig_inception;
%rename(__ldns_rr_rrsig_inception) ldns_rr_rrsig_inception;
%inline
%{
  ldns_rdf * _ldns_rr_rrsig_inception(ldns_rr *rr)
  {
    ldns_rdf *rdf;
    rdf = ldns_rr_rrsig_inception(rr);
    return (rdf != NULL) ? ldns_rdf_clone(rdf) : NULL;
  }
%}


%newobject _ldns_rr_rrsig_keytag;
%rename(__ldns_rr_rrsig_keytag) ldns_rr_rrsig_keytag;
%inline
%{
  ldns_rdf * _ldns_rr_rrsig_keytag(ldns_rr *rr)
  {
    ldns_rdf *rdf;
    rdf = ldns_rr_rrsig_keytag(rr);
    return (rdf != NULL) ? ldns_rdf_clone(rdf) : NULL;
  }
%}


%newobject _ldns_rr_rrsig_labels;
%rename(__ldns_rr_rrsig_labels) ldns_rr_rrsig_labels;
%inline
%{
  ldns_rdf * _ldns_rr_rrsig_labels(ldns_rr *rr)
  {
    ldns_rdf *rdf;
    rdf = ldns_rr_rrsig_labels(rr);
    return (rdf != NULL) ? ldns_rdf_clone(rdf) : NULL;
  }
%}


%newobject _ldns_rr_rrsig_origttl;
%rename(__ldns_rr_rrsig_origttl) ldns_rr_rrsig_origttl;
%inline
%{
  ldns_rdf * _ldns_rr_rrsig_origttl(ldns_rr *rr)
  {
    ldns_rdf *rdf;
    rdf = ldns_rr_rrsig_origttl(rr);
    return (rdf != NULL) ? ldns_rdf_clone(rdf) : NULL;
  }
%}


%newobject _ldns_rr_rrsig_sig;
%rename(__ldns_rr_rrsig_sig) ldns_rr_rrsig_sig;
%inline
%{
  ldns_rdf * _ldns_rr_rrsig_sig(ldns_rr *rr)
  {
    ldns_rdf *rdf;
    rdf = ldns_rr_rrsig_sig(rr);
    return (rdf != NULL) ? ldns_rdf_clone(rdf) : NULL;
  }
%}


%newobject _ldns_rr_rrsig_signame;
%rename(__ldns_rr_rrsig_signame) ldns_rr_rrsig_signame;
%inline
%{
  ldns_rdf * _ldns_rr_rrsig_signame(ldns_rr *rr)
  {
    ldns_rdf *rdf;
    rdf = ldns_rr_rrsig_signame(rr);
    return (rdf != NULL) ? ldns_rdf_clone(rdf) : NULL;
  }
%}


%newobject _ldns_rr_rrsig_typecovered;
%rename(__ldns_rr_rrsig_typecovered) ldns_rr_rrsig_typecovered;
%inline
%{
  ldns_rdf * _ldns_rr_rrsig_typecovered(ldns_rr *rr)
  {
    ldns_rdf *rdf;
    rdf = ldns_rr_rrsig_typecovered(rr);
    return (rdf != NULL) ? ldns_rdf_clone(rdf) : NULL;
  }
%}

/* End of pull cloning. */

/* Clone rdf data on push. */

%rename(__ldns_rr_a_set_address) ldns_rr_a_set_address;
%inline
%{
  bool _ldns_rr_a_set_address(ldns_rr *rr, ldns_rdf *rdf)
  {
    return ldns_rr_a_set_address(rr, ldns_rdf_clone(rdf));
  }
%}

%rename(__ldns_rr_dnskey_set_algorithm) ldns_rr_dnskey_set_algorithm;
%inline
%{
  bool _ldns_rr_dnskey_set_algorithm(ldns_rr *rr, ldns_rdf *rdf)
  {
    return ldns_rr_dnskey_set_algorithm(rr, ldns_rdf_clone(rdf));
  }
%}

%rename(__ldns_rr_dnskey_set_flags) ldns_rr_dnskey_set_flags;
%inline
%{
  bool _ldns_rr_dnskey_set_flags(ldns_rr *rr, ldns_rdf *rdf)
  {
    return ldns_rr_dnskey_set_flags(rr, ldns_rdf_clone(rdf));
  }
%}

%rename(__ldns_rr_dnskey_set_key) ldns_rr_dnskey_set_key;
%inline
%{
  bool _ldns_rr_dnskey_set_key(ldns_rr *rr, ldns_rdf *rdf)
  {
    return ldns_rr_dnskey_set_key(rr, ldns_rdf_clone(rdf));
  }
%}

%rename(__ldns_rr_dnskey_set_protocol) ldns_rr_dnskey_set_protocol;
%inline
%{
  bool _ldns_rr_dnskey_set_protocol(ldns_rr *rr, ldns_rdf *rdf)
  {
    return ldns_rr_dnskey_set_protocol(rr, ldns_rdf_clone(rdf));
  }
%}

%rename(__ldns_rr_push_rdf) ldns_rr_push_rdf;
%inline
%{
  bool _ldns_rr_push_rdf(ldns_rr *rr, ldns_rdf *rdf)
  {
    return ldns_rr_push_rdf(rr, ldns_rdf_clone(rdf));
  }
%}

%rename(__ldns_rr_rrsig_set_algorithm) ldns_rr_rrsig_set_algorithm;
%inline
%{
  bool _ldns_rr_rrsig_set_algorithm(ldns_rr *rr, ldns_rdf *rdf)
  {
    return ldns_rr_rrsig_set_algorithm(rr, ldns_rdf_clone(rdf));
  }
%}

%rename(__ldns_rr_rrsig_set_expiration) ldns_rr_rrsig_set_expiration;
%inline
%{
  bool _ldns_rr_rrsig_set_expiration(ldns_rr *rr, ldns_rdf *rdf)
  {
    return ldns_rr_rrsig_set_expiration(rr, ldns_rdf_clone(rdf));
  }
%}

%rename(__ldns_rr_rrsig_set_inception) ldns_rr_rrsig_set_inception;
%inline
%{
  bool _ldns_rr_rrsig_set_inception(ldns_rr *rr, ldns_rdf *rdf)
  {
    return ldns_rr_rrsig_set_inception(rr, ldns_rdf_clone(rdf));
  }
%}

%rename(__ldns_rr_rrsig_set_keytag) ldns_rr_rrsig_set_keytag;
%inline
%{
  bool _ldns_rr_rrsig_set_keytag(ldns_rr *rr, ldns_rdf *rdf)
  {
    return ldns_rr_rrsig_set_keytag(rr, ldns_rdf_clone(rdf));
  }
%}

%rename(__ldns_rr_rrsig_set_labels) ldns_rr_rrsig_set_labels;
%inline
%{
  bool _ldns_rr_rrsig_set_labels(ldns_rr *rr, ldns_rdf *rdf)
  {
    return ldns_rr_rrsig_set_labels(rr, ldns_rdf_clone(rdf));
  }
%}

%rename(__ldns_rr_rrsig_set_origttl) ldns_rr_rrsig_set_origttl;
%inline
%{
  bool _ldns_rr_rrsig_set_origttl(ldns_rr *rr, ldns_rdf *rdf)
  {
    return ldns_rr_rrsig_set_origttl(rr, ldns_rdf_clone(rdf));
  }
%}

%rename(__ldns_rr_rrsig_set_sig) ldns_rr_rrsig_set_sig;
%inline
%{
  bool _ldns_rr_rrsig_set_sig(ldns_rr *rr, ldns_rdf *rdf) {
    return ldns_rr_rrsig_set_sig(rr, ldns_rdf_clone(rdf));
  }
%}

%rename(__ldns_rr_rrsig_set_signame) ldns_rr_rrsig_set_signame;
%inline
%{
  bool _ldns_rr_rrsig_set_signame(ldns_rr *rr, ldns_rdf *rdf)
  {
    return ldns_rr_rrsig_set_signame(rr, ldns_rdf_clone(rdf));
  }
%}

%rename(__ldns_rr_rrsig_set_typecovered) ldns_rr_rrsig_set_typecovered;
%inline
%{
  bool _ldns_rr_rrsig_set_typecovered(ldns_rr *rr, ldns_rdf *rdf)
  {
    return ldns_rr_rrsig_set_typecovered(rr, ldns_rdf_clone(rdf));
  }
%}

%rename(__ldns_rr_set_owner) ldns_rr_set_owner;
%inline
%{
  void _ldns_rr_set_owner(ldns_rr *rr, ldns_rdf *rdf)
  {
    ldns_rr_set_owner(rr, ldns_rdf_clone(rdf));
  }
%}

%newobject _ldns_rr_set_rdf;
%rename(__ldns_rr_set_rdf) ldns_rr_set_rdf;
%inline
%{
  ldns_rdf * _ldns_rr_set_rdf(ldns_rr *rr, ldns_rdf *rdf, size_t pos)
  {
    /* May leak memory on unsuccessful calls. */
    ldns_rdf *new, *ret;

    new = ldns_rdf_clone(rdf);

    if ((ret = ldns_rr_set_rdf(rr, new, pos)) == NULL) {
        ldns_rdf_deep_free(new);
    }

    return ret;
  }
%}

/* End of push cloning. */

%rename(_ldns_rr_new_frm_str) ldns_rr_new_frm_str;
%rename(_ldns_rr_new_frm_fp_l) ldns_rr_new_frm_fp_l;
%rename(_ldns_rr_new_frm_fp) ldns_rr_new_frm_fp;


/* ========================================================================= */
/* Debugging related code. */
/* ========================================================================= */


#ifdef LDNS_DEBUG
%rename(__ldns_rr_free) ldns_rr_free;
%inline %{
  void _ldns_rr_free (ldns_rr *r)
  {
    printf("******** LDNS_RR free 0x%lX ************\n", (long unsigned int)r);
    ldns_rr_free(r);
  }
%}
#else /* !LDNS_DEBUG */
%rename(_ldns_rr_free) ldns_rr_free;
#endif /* LDNS_DEBUG */


/* ========================================================================= */
/* Added C code. */
/* ========================================================================= */

/* None. */


/* ========================================================================= */
/* Encapsulating Python code. */
/* ========================================================================= */


%feature("docstring") ldns_struct_rr "Resource Record (RR).

The RR is the basic DNS element that contains actual data. This class allows
to create RR and manipulate with the content.

Use :meth:`ldns_rr_new`, :meth:`ldns_rr_new_frm_type`, :meth:`new_frm_fp`,
:meth:`new_frm_fp_l`, :meth:`new_frm_str` or :meth:`new_question_frm_str`
to create :class:`ldns_rr` instances.
"

%extend ldns_struct_rr {
 
  %pythoncode
  %{
        def __init__(self):
            raise Exception("This class can't be created directly. " +
                "Please use: ldns_rr_new(), ldns_rr_new_frm_type(), " +
                "new_frm_fp(), new_frm_fp_l(), new_frm_str() or " +
                "new_question_frm_str()")
       
        __swig_destroy__ = _ldns._ldns_rr_free

        #
        # LDNS_RR_CONSTRUCTORS_
        #

        @staticmethod
        def new_frm_str(string, default_ttl=0, origin=None, prev=None, raiseException=True):
            """
               Creates an rr object from a string.

               The string should be a fully filled-in rr, like "owner_name
               [space] TTL [space] CLASS [space] TYPE [space] RDATA."
               
               :param string: The string to convert.
               :type string: str
               :param default_ttl: Default ttl value for the rr.
                   If 0 DEF_TTL will be used.
               :type default_ttl: int
               :param origin: When the owner is relative add this.
               :type origin: :class:`ldns_dname`
               :param prev: The previous owner name.
               :type prev: :class:`ldns_rdf`
               :param raiseException: If True, an exception occurs in case a rr
                   instance can't be created.
               :throws Exception: If `raiseException` is set and fails.
               :throws TypeError: When parameters of incorrect types.
               :return: (:class:`ldns_rr`) RR instance or None.

               .. note::
                   The type checking of `origin` is benevolent.
                   It allows also to pass a dname :class:`ldns_rdf` object.
                   This will probably change in future.
        
               **Usage**

               >>> import ldns
               >>> rr = ldns.ldns_rr.new_frm_str("www.nic.cz. IN A 192.168.1.1", 300)
               >>> print rr
               www.nic.cz.  300  IN  A  192.168.1.1
               >>> rr = ldns.ldns_rr.new_frm_str("test.nic.cz. 600 IN A 192.168.1.2")
               >>> print rr
               test.nic.cz.  600  IN  A  192.168.1.2
               
            """
            if (not isinstance(origin, ldns_dname)) and \
               isinstance(origin, ldns_rdf) and \
               origin.get_type() == _ldns.LDNS_RDF_TYPE_DNAME:
                warnings.warn("The ldns_rr.new_frm_str() method will" +
                    " drop the possibility to accept ldns_rdf as origin." +
                    " Convert argument to ldns_dname.",
                    PendingDeprecationWarning, stacklevel=2)
            status, rr, prev = _ldns.ldns_rr_new_frm_str_(string, default_ttl,
                origin, prev)
            if status != LDNS_STATUS_OK:
                if (raiseException):
                    raise Exception("Can't create RR, error: %d" % status)
                return None
            return rr

        @staticmethod
        def new_question_frm_str(string, default_ttl=0, origin=None, prev=None, raiseException=True):
            """
               Creates an rr object from a string.

               The string is like :meth:`new_frm_str` but without rdata.

               :param string: The string to convert.
               :type string: str
               :param origin: When the owner is relative add this.
               :type origin: :class:`ldns_dname`
               :param prev: The previous owner name.
               :type prev: :class:`ldns_rdf`
               :param raiseException: If True, an exception occurs in case
                   a rr instance can't be created.
               :throws Exception: If `raiseException` is set and fails.
               :throws TypeError: When parameters of incorrect types.
               :return: (:class:`ldns_rr`) RR instance or None. If the object
                   can't be created and `raiseException` is True,
                   an exception occurs.

               .. note::
                   The type checking of `origin` is benevolent.
                   It allows also to pass a dname :class:`ldns_rdf` object.
                   This will probably change in future.
            """
            if (not isinstance(origin, ldns_dname)) and \
               isinstance(origin, ldns_rdf) and \
               origin.get_type() == _ldns.LDNS_RDF_TYPE_DNAME:
                warnings.warn("The ldns_rr.new_question_frm_str() method will" +
                    " drop the possibility to accept ldns_rdf as origin." +
                    " Convert argument to ldns_dname.",
                    PendingDeprecationWarning, stacklevel=2)
            status, rr, prev = _ldns.ldns_rr_new_question_frm_str_(string,
                origin, prev)
            if status != LDNS_STATUS_OK:
                if (raiseException):
                    raise Exception("Can't create RR, error: %d" % status)
                return None
            return rr

        @staticmethod
        def new_frm_str_prev(string, default_ttl=0, origin=None, prev=None, raiseException=True):
            """
               Creates an rr object from a string.

               The string should be a fully filled-in rr, like "owner_name
               [space] TTL [space] CLASS [space] TYPE [space] RDATA".
               
               :param string: The string to convert.
               :type string: str
               :param default_ttl: Default ttl value for the rr.
                   If 0 DEF_TTL will be used.
               :type default_ttl: int
               :param origin: When the owner is relative add this.
               :type origin: :class:`ldns_dname`
               :param prev: The previous owner name.
               :type prev: :class:`ldns_rdf`
               :param raiseException: If True, an exception occurs in case when
                   a rr instance can't be created.
               :throws Exception: If `raiseException` is set and fails.
               :throws TypeError: When parameters of incorrect types.
               :return: None when fails, otherwise a tuple containing:

                  * rr - (:class:`ldns_rr`) RR instance or None.
                    If the object can't be created and `raiseException`
                    is True, an exception occurs.
        
                  * prev - (:class:`ldns_rdf`) Owner name found in this string
                    or None.

               .. note::
                   The type checking of `origin` is benevolent.
                   It allows also to pass a dname :class:`ldns_rdf` object.
                   This will probably change in future.
            """
            if (not isinstance(origin, ldns_dname)) and \
               isinstance(origin, ldns_rdf) and \
               origin.get_type() == _ldns.LDNS_RDF_TYPE_DNAME:
                warnings.warn("The ldns_rr.new_frm_str_prev() method will" +
                    " drop the possibility to accept ldns_rdf as origin." +
                    " Convert argument to ldns_dname.",
                    PendingDeprecationWarning, stacklevel=2)
            status, rr, prev = _ldns.ldns_rr_new_frm_str_(string, default_ttl,
                origin, prev)
            if status != LDNS_STATUS_OK:
                if (raiseException):
                    raise Exception("Can't create RR, error: %d" % status)
                return None
            return rr, prev

        @staticmethod
        def new_frm_fp(file, default_ttl=0, origin=None, prev=None, raiseException=True):
            """
               Creates a new rr from a file containing a string.
              
               :param file: Opened file.
               :param default_ttl: If 0 DEF_TTL will be used.
               :type default_ttl: int
               :param origin: When the owner is relative add this.
               :type origin: :class:`ldns_dname`
               :param prev: When the owner is white spaces use this.
               :type prev: :class:`ldns_rdf`
               :param raiseException: If True, an exception occurs in case
                   a resolver object can't be created.
               :throws Exception: If `raiseException` is set and the input
                   cannot be read.
               :throws TypeError: When parameters of incorrect types.
               :return: None when fails, otherwise a tuple containing:

                   * rr - (:class:`ldns_rr`) RR object or None. If the object
                     can't be created and `raiseException` is True,
                     an exception occurs.

                   * ttl - (int) None or TTL if the file contains a TTL
                     directive.

                   * origin - (:class:`ldns_rdf`) None or dname rdf if the file
                     contains a ORIGIN directive.

                   * prev - (:class:`ldns_rdf`) None or updated value
                     of prev parameter.

               .. note::
                   The type checking of `origin` is benevolent.
                   It allows also to pass a dname :class:`ldns_rdf` object.
                   This will probably change in future.
            """
            if (not isinstance(origin, ldns_dname)) and \
               isinstance(origin, ldns_rdf) and \
               origin.get_type() == _ldns.LDNS_RDF_TYPE_DNAME:
                warnings.warn("The ldns_rr.new_frm_fp() method will" +
                    " drop the possibility to accept ldns_rdf as origin." +
                    " Convert argument to ldns_dname.",
                    PendingDeprecationWarning, stacklevel=2)
            res = _ldns.ldns_rr_new_frm_fp_(file, default_ttl, origin, prev)
            if res[0] != LDNS_STATUS_OK:
                if (raiseException):
                    raise Exception("Can't create RR, error: %d" % res[0])
                return None
            return res[1:]

        @staticmethod
        def new_frm_fp_l(file, default_ttl=0, origin=None, prev=None, raiseException=True):
            """
               Creates a new rr from a file containing a string.
              
               :param file: Opened file.
               :param default_ttl: If 0 DEF_TTL will be used.
               :type default_ttl: int
               :param origin: When the owner is relative add this.
               :type origin: :class:`ldns_dname`
               :param prev: When the owner is white spaces use this.
               :type prev: :class:`ldns_rdf`
               :param raiseException: If True, an exception occurs in case
                   a resolver object can't be created.
               :throws Exception: If `raiseException` is set and the input
                   cannot be read.
               :throws TypeError: When parameters of incorrect types.
               :return: None when fails, otherwise a tuple containing:

                  * rr - (:class:`ldns_rr`) RR object or None. If the object
                    can't be created and `raiseException` is True,
                    an exception occurs.

                  * line - (int) line number (for debugging).

                  * ttl - (int) None or TTL if the file contains a TTL
                    directive .

                  * origin - (:class:`ldns_rdf`) None or dname rdf if the file
                    contains a ORIGIN directive.

                  * prev - (:class:`ldns_rdf`) None or updated value of prev
                    parameter.

               .. note::
                   The type checking of `origin` is benevolent.
                   It allows also to pass a dname :class:`ldns_rdf` object.
                   This will probably change in future.
            """
            if (not isinstance(origin, ldns_dname)) and \
               isinstance(origin, ldns_rdf) and \
               origin.get_type() == _ldns.LDNS_RDF_TYPE_DNAME:
                warnings.warn("The ldns_rr.new_frm_fp_l() method will" +
                    " drop the possibility to accept ldns_rdf as origin." +
                    " Convert argument to ldns_dname.",
                    PendingDeprecationWarning, stacklevel=2)
            res = _ldns.ldns_rr_new_frm_fp_l_(file, default_ttl, origin, prev)
            if res[0] != LDNS_STATUS_OK:
                if (raiseException):
                    raise Exception("Can't create RR, error: %d" % res[0])
                return None
            return res[1:]

        #
        # _LDNS_RR_CONSTRUCTORS
        #

        def __str__(self):
            """
               Converts the data in the resource record to presentation format.

               :return: (str)
            """
            return _ldns.ldns_rr2str(self)

        def __cmp__(self, other):
            """
               Compares two rrs.
               
               The TTL is not looked at.
               
               :param other: The second RR one.
               :type other: :class:`ldns_rr`
               :throws TypeError: When `other` of non-:class:`ldns_rr` type.
               :return: (int) 0 if equal, -1 if `self` comes before `other`,
                   1 if `other` RR comes before `self`.
            """
            return _ldns.ldns_rr_compare(self, other)

        def __lt__(self, other):
            """
               Compares two rrs.
               
               The TTL is not looked at.
               
               :param other: The second RR one.
               :type other: :class:`ldns_rr`
               :throws TypeError: When `other` of non-:class:`ldns_rr` type.
               :return: (bool) True when `self` is less than 'other'.
            """
            return _ldns.ldns_rr_compare(self, other) == -1

        def __le__(self, other):
            """
               Compares two rrs.
               
               The TTL is not looked at.
               
               :param other: The second RR one.
               :type other: :class:`ldns_rr`
               :throws TypeError: When `other` of non-:class:`ldns_rr` type.
               :return: (bool) True when `self` is less than or equal to
                   'other'.
            """
            return _ldns.ldns_rr_compare(self, other) != 1

        def __eq__(self, other):
            """
               Compares two rrs.
               
               The TTL is not looked at.
               
               :param other: The second RR one.
               :type other: :class:`ldns_rr`
               :throws TypeError: When `other` of non-:class:`ldns_rr` type.
               :return: (bool) True when `self` is equal to 'other'.
            """
            return _ldns.ldns_rr_compare(self, other) == 0

        def __ne__(self, other):
            """
               Compares two rrs.
               
               The TTL is not looked at.
               
               :param other: The second RR one.
               :type other: :class:`ldns_rr`
               :throws TypeError: When `other` of non-:class:`ldns_rr` type.
               :return: (bool) True when `self` is not equal to 'other'.
            """
            return _ldns.ldns_rr_compare(self, other) != 0

        def __gt__(self, other):
            """
               Compares two rrs.
               
               The TTL is not looked at.
               
               :param other: The second RR one.
               :type other: :class:`ldns_rr`
               :throws TypeError: When `other` of non-:class:`ldns_rr` type.
               :return: (bool) True when `self` is greater than 'other'.
            """
            return _ldns.ldns_rr_compare(self, other) == 1

        def __ge__(self, other):
            """
               Compares two rrs.
               
               The TTL is not looked at.
               
               :param other: The second RR one.
               :type other: :class:`ldns_rr`
               :throws TypeError: When `other` of non-:class:`ldns_rr` type.
               :return: (bool) True when `self` is greater than or equal to
                    'other'.
            """
            return _ldns.ldns_rr_compare(self, other) != -1

        @staticmethod
        def class_by_name(string):
            """
               Retrieves a class identifier value by looking up its name.

               :param string: Class name.
               :type string: str
               :throws TypeError: when `string` of inappropriate type.
               :return: (int) Class identifier value, or 0 if not valid
                   class name given.
            """
            return _ldns.ldns_get_rr_class_by_name(string)

        def rdfs(self):
            """
               Returns a generator object of rdata records.

               :return: Generator of :class:`ldns_rdf`.
            """
            for i in range(0, self.rd_count()):
                yield self.rdf(i)

        def print_to_file(self, output):
            """
               Prints the data in the resource record to the given file stream
               (in presentation format).

               :param output: Opened file stream.
               :throws TypeError: When `output` not a file.
            """
            _ldns.ldns_rr_print(output, self)
            #parameters: FILE *, const ldns_rr *,

        def get_type_str(self):
            """
               Converts an RR type value to its string representation,
               and returns that string.

               :return: (str) containing type identification.
            """
            return _ldns.ldns_rr_type2str(self.get_type())
            #parameters: const ldns_rr_type,

        def get_class_str(self):
            """
               Converts an RR class value to its string representation,
               and returns that string.

               :return: (str) containing class identification.
            """
            return _ldns.ldns_rr_class2str(self.get_class())
            #parameters: const ldns_rr_class,

        @staticmethod
        def dnskey_key_size_raw(keydata, len, alg):
            """
               Get the length of the keydata in bits.

               :param keydata: Key raw data.
               :type keydata: unsigned char \*
               :param len: Number of bytes of `keydata`.
               :type len: size_t
               :param alg: Algorithm identifier.
               :type alg: ldns_algorithm

               :return: (size_t) The length of key data in bits.
            """
            return _ldns.ldns_rr_dnskey_key_size_raw(keydata, len, alg)
            #parameters: const unsigned char *,const size_t,const ldns_algorithm,
            #retvals: size_t

        def write_to_buffer(self,buffer,section):
            """
               Copies the rr data to the buffer in wire format.
               
               :param buffer: Buffer to append the result to.
               :type buffer: :class:`ldns_buffer`
               :param section: The section in the packet this rr is supposed
                   to be in (to determine whether to add rdata or not).
               :type section: int
               :throws TypeError: when arguments of mismatching types passed.
               :return: (ldns_status) ldns_status
            """
            return _ldns.ldns_rr2buffer_wire(buffer, self, section)
            #parameters: ldns_buffer *,const ldns_rr *,int,
            #retvals: ldns_status

        def write_to_buffer_canonical(self,buffer,section):
            """
               Copies the rr data to the buffer in wire format, in canonical
               format according to RFC3597 (every dname in rdata fields
               of RR's mentioned in that RFC will be converted to lower-case).
               
               :param buffer: Buffer to append the result to.
               :type buffer: :class:`ldns_buffer`
               :param section: The section in the packet this rr is supposed
                   to be in (to determine whether to add rdata or not).
               :type section: int
               :throws TypeError: when arguments of mismatching types passed.
               :return: (ldns_status) ldns_status
            """
            return _ldns.ldns_rr2buffer_wire_canonical(buffer,self,section)
            #parameters: ldns_buffer *,const ldns_rr *,int,
            #retvals: ldns_status

        def write_data_to_buffer(self, buffer):
            """
               Converts an rr's rdata to wire format, while excluding the
               owner name and all the stuff before the rdata.
               
               This is needed in DNSSEC key-tag calculation, the ds
               calculation from the key and maybe elsewhere.
               
               :param buffer: Buffer to append the result to.
               :type buffer: :class:`ldns_buffer`
               :throws TypeError: when `buffer` of non-:class:`ldns_buffer`
                   type.
               :return: (ldns_status) ldns_status
            """
            return _ldns.ldns_rr_rdata2buffer_wire(buffer,self)
            #parameters: ldns_buffer *, const ldns_rr *,
            #retvals: ldns_status

        def write_rrsig_to_buffer(self, buffer):
            """
               Converts a rrsig to wire format BUT EXCLUDE the rrsig rdata.

               This is needed in DNSSEC verification.
               
               :param buffer: Buffer to append the result to.
               :type buffer: :class:`ldns_buffer`
               :throws TypeError: when `buffer` of non-:class:`ldns_buffer`
                   type.
               :return: (ldns_status) ldns_status
            """
            return _ldns.ldns_rrsig2buffer_wire(buffer,self)
            #parameters: ldns_buffer *,const ldns_rr *,
            #retvals: ldns_status

        #
        # LDNS_RR_METHODS_
        #

        def a_address(self):
            """
               Returns the address rdf of a LDNS_RR_TYPE_A or LDNS_RR_TYPE_AAAA
               rr.
               
               :return: (:class:`ldns_rdf`) with the address or None on
                   failure.
            """
            return _ldns._ldns_rr_a_address(self)
            #parameters: const ldns_rr *,
            #retvals: ldns_rdf *

        def a_set_address(self, f):
            """
               Sets the address of a LDNS_RR_TYPE_A or LDNS_RR_TYPE_AAAA rr.
               
               :param f: The address to be set.
               :type f: :class:`ldns_rdf`
               :throws TypeError: When `f` of non-:class:`ldns_rdf` type.
               :return: (bool) True on success, False otherwise.
            """
            return _ldns._ldns_rr_a_set_address(self, f)
            #parameters: ldns_rr *, ldns_rdf *,
            #retvals: bool

        def clone(self):
            """
               Clones a rr and all its data.
               
               :return: (:class:`ldns_rr`) The new rr or None on failure.
            """
            return _ldns.ldns_rr_clone(self)
            #parameters: const ldns_rr *,
            #retvals: ldns_rr *

        def compare_ds(self, rr2):
            """
               Returns True if the given rr's are equal. 
               
               Also returns True if one record is a DS that represents the
               same DNSKEY record as the other record.
               
               :param rr2: The second rr.
               :type rr2: :class:`ldns_rr`
               :throws TypeError: When `rr2` of non-:class:`ldns_rr` type.
               :return: (bool) True if equal otherwise False.
            """
            return _ldns.ldns_rr_compare_ds(self, rr2)
            #parameters: const ldns_rr *, const ldns_rr *,
            #retvals: bool

        def compare_no_rdata(self, rr2):
            """
               Compares two rrs, up to the rdata.
               
               :param rr2: Rhe second rr.
               :type rr2: :class:`ldns_rr`
               :throws TypeError: When `rr2` of non-:class:`ldns_rr` type.
               :return: (int) 0 if equal, negative integer if `self` comes
                   before `rr2`, positive integer if `rr2` comes before `self`.
            """
            return _ldns.ldns_rr_compare_no_rdata(self, rr2)
            #parameters: const ldns_rr *, const ldns_rr *,
            #retvals: int

        def dnskey_algorithm(self):
            """
               Returns the algorithm of a LDNS_RR_TYPE_DNSKEY rr.
               
               :return: (:class:`ldns_rdf`) with the algorithm or None
                   on failure.
            """
            return _ldns._ldns_rr_dnskey_algorithm(self)
            #parameters: const ldns_rr *,
            #retvals: ldns_rdf *

        def dnskey_flags(self):
            """
               Returns the flags of a LDNS_RR_TYPE_DNSKEY rr.
               
               :return: (:class:`ldns_rdf`) with the flags or None on failure.
            """
            return _ldns._ldns_rr_dnskey_flags(self)
            #parameters: const ldns_rr *,
            #retvals: ldns_rdf *

        def dnskey_key(self):
            """
               Returns the key data of a LDNS_RR_TYPE_DNSKEY rr.
               
               :return: (:class:`ldns_rdf`) with the key data or None on
                   failure.
            """
            return _ldns._ldns_rr_dnskey_key(self)
            #parameters: const ldns_rr *,
            #retvals: ldns_rdf *

        def dnskey_key_size(self):
            """
               Get the length of the keydata in bits.
               
               :return: (size_t) the keysize in bits.
            """
            return _ldns.ldns_rr_dnskey_key_size(self)
            #parameters: const ldns_rr *,
            #retvals: size_t

        def dnskey_protocol(self):
            """
               Returns the protocol of a LDNS_RR_TYPE_DNSKEY rr.
               
               :return: (:class:`ldns_rdf`) with the protocol or None on
                   failure.
            """
            return _ldns._ldns_rr_dnskey_protocol(self)
            #parameters: const ldns_rr *,
            #retvals: ldns_rdf *

        def dnskey_set_algorithm(self, f):
            """
               Sets the algorithm of a LDNS_RR_TYPE_DNSKEY rr
               
               :param f: The algorithm to set.
               :type f: :class:`ldns_rdf`
               :throws TypeError: When `f` of non-:class:`ldns_rdf` type.
               :return: (bool) True on success, False otherwise.
            """
            return _ldns._ldns_rr_dnskey_set_algorithm(self, f)
            #parameters: ldns_rr *, ldns_rdf *,
            #retvals: bool

        def dnskey_set_flags(self, f):
            """
               Sets the flags of a LDNS_RR_TYPE_DNSKEY rr.
               
               :param f: The flags to be set.
               :type f: :class:`ldns_rdf`
               :throws TypeError: When `f` of non-:class:`ldns_rdf` type.
               :return: (bool) True on success, False otherwise.
            """
            return _ldns._ldns_rr_dnskey_set_flags(self, f)
            #parameters: ldns_rr *, ldns_rdf *,
            #retvals: bool

        def dnskey_set_key(self, f):
            """
               Sets the key data of a LDNS_RR_TYPE_DNSKEY rr.
               
               :param f: The key data to set.
               :type f: :class:`ldns_rdf`
               :throws TypeError: When `f` of non-:class:`ldns_rdf` type.
               :return: (bool) True on success, False otherwise.
            """
            return _ldns._ldns_rr_dnskey_set_key(self, f)
            #parameters: ldns_rr *, ldns_rdf *,
            #retvals: bool

        def dnskey_set_protocol(self,f):
            """
               Sets the protocol of a LDNS_RR_TYPE_DNSKEY rr.
               
               :param f: The protocol to set.
               :type f: :class:`ldns_rdf`
               :throws TypeError: When `f` of non-:class:`ldns_rdf` type.
               :return: (bool) True on success, False otherwise.
            """
            return _ldns._ldns_rr_dnskey_set_protocol(self,f)
            #parameters: ldns_rr *,ldns_rdf *,
            #retvals: bool

        def get_class(self):
            """
               Returns the class of the rr.
               
               :return: (int) The class identifier of the rr.
            """
            return _ldns.ldns_rr_get_class(self)
            #parameters: const ldns_rr *,
            #retvals: ldns_rr_class

        def get_type(self):
            """
               Returns the type of the rr.
               
               :return: (int) The type identifier of the rr.
            """
            return _ldns.ldns_rr_get_type(self)
            #parameters: const ldns_rr *,
            #retvals: ldns_rr_type

        def is_question(self):
            """
               Returns the question flag of a rr structure.

               :return: (bool) True if question flag is set.
            """
            return _ldns.ldns_rr_is_question(self)

        def label_count(self):
            """
               Counts the number of labels of the owner name.
               
               :return: (int) The number of labels.
            """
            return _ldns.ldns_rr_label_count(self)
            #parameters: ldns_rr *,
            #retvals: uint8_t

        def mx_exchange(self):
            """
               Returns the mx host of a LDNS_RR_TYPE_MX rr.
               
               :return: (:class:`ldns_rdf`) with the name of the MX host
                  or None on failure.
            """
            return _ldns._ldns_rr_mx_exchange(self)
            #parameters: const ldns_rr *,
            #retvals: ldns_rdf *

        def mx_preference(self):
            """
               Returns the mx preference of a LDNS_RR_TYPE_MX rr.
               
               :return: (:class:`ldns_rdf`) with the preference or None
                   on failure.
            """
            return _ldns._ldns_rr_mx_preference(self)
            #parameters: const ldns_rr *,
            #retvals: ldns_rdf *

        def ns_nsdname(self):
            """
               Returns the name of a LDNS_RR_TYPE_NS rr.
               
               :return: (:class:`ldns_rdf`) A dname rdf with the name or
                   None on failure.
            """
            return _ldns._ldns_rr_ns_nsdname(self)
            #parameters: const ldns_rr *,
            #retvals: ldns_rdf *

        def owner(self):
            """
               Returns the owner name of an rr structure.
               
               :return: (:class:`ldns_dname`) Owner name or None on failure.
            """
            rdf = _ldns._ldns_rr_owner(self)
            if rdf:
                rdf = ldns_dname(rdf, clone=False)
            return rdf
            #parameters: const ldns_rr *,
            #retvals: ldns_rdf *

        def pop_rdf(self):
            """
               Removes a rd_field member, it will be popped from the last
               position.
               
               :return: (:class:`ldns_rdf`) rdf which was popped, None if
                   nothing.
            """
            return _ldns.ldns_rr_pop_rdf(self)
            #parameters: ldns_rr *,
            #retvals: ldns_rdf *

        def push_rdf(self,f):
            """
               Sets rd_field member, it will be placed in the next available
               spot.
               
               :param f: The rdf to be appended.
               :type f: :class:`ldns_rdf`
               :throws TypeError: When `f` of non-:class:`ldns_rdf` type.
               :return: (bool) Returns True if success, False otherwise.
            """
            return _ldns._ldns_rr_push_rdf(self, f)
            #parameters: ldns_rr *, const ldns_rdf *,
            #retvals: bool

        def rd_count(self):
            """
               Returns the rd_count of an rr structure.
               
               :return: (size_t) the rd count of the rr.
            """
            return _ldns.ldns_rr_rd_count(self)
            #parameters: const ldns_rr *,
            #retvals: size_t

        def rdf(self, nr):
            """
               Returns the rdata field with the given index.
               
               :param nr: The index of the rdf to return.
               :type nr: positive int
               :throws TypeError: When `nr` not a positive integer.
               :return: (:class:`ldns_rdf`) The given rdf or None if fails.
            """
            return _ldns._ldns_rr_rdf(self, nr)
            #parameters: const ldns_rr *, size_t,
            #retvals: ldns_rdf *

        def rrsig_algorithm(self):
            """
               Returns the algorithm identifier of a LDNS_RR_TYPE_RRSIG RR.
               
               :return: (:class:`ldns_rdf`) with the algorithm or None
                   on failure.
            """
            return _ldns._ldns_rr_rrsig_algorithm(self)
            #parameters: const ldns_rr *,
            #retvals: ldns_rdf *

        def rrsig_expiration(self):
            """
               Returns the expiration time of a LDNS_RR_TYPE_RRSIG RR.
               
               :return: (:class:`ldns_rdf`) with the expiration time or None
                   on failure.
            """
            return _ldns._ldns_rr_rrsig_expiration(self)
            #parameters: const ldns_rr *,
            #retvals: ldns_rdf *

        def rrsig_inception(self):
            """
               Returns the inception time of a LDNS_RR_TYPE_RRSIG RR.
               
               :return: (:class:`ldns_rdf`) with the inception time or None
                   on failure.
            """
            return _ldns._ldns_rr_rrsig_inception(self)
            #parameters: const ldns_rr *,
            #retvals: ldns_rdf *

        def rrsig_keytag(self):
            """
               Returns the keytag of a LDNS_RR_TYPE_RRSIG RR.
               
               :return: (:class:`ldns_rdf`) with the keytag or None on failure.
            """
            return _ldns._ldns_rr_rrsig_keytag(self)
            #parameters: const ldns_rr *,
            #retvals: ldns_rdf *

        def rrsig_labels(self):
            """
               Returns the number of labels of a LDNS_RR_TYPE_RRSIG RR.
               
               :return: (:class:`ldns_rdf`) with the number of labels or None
                   on failure.
            """
            return _ldns._ldns_rr_rrsig_labels(self)
            #parameters: const ldns_rr *,
            #retvals: ldns_rdf *

        def rrsig_origttl(self):
            """
               Returns the original TTL of a LDNS_RR_TYPE_RRSIG RR.
               
               :return: (:class:`ldns_rdf`) with the original TTL or None
                   on failure.
            """
            return _ldns._ldns_rr_rrsig_origttl(self)
            #parameters: const ldns_rr *,
            #retvals: ldns_rdf *

        def rrsig_set_algorithm(self, f):
            """
               Sets the algorithm of a LDNS_RR_TYPE_RRSIG rr.
               
               :param f: The algorithm to set.
               :type f: :class:`ldns_rdf`
               :throws TypeError: when `f` of non-:class:`ldns_rdf` type.
               :return: (bool) True on success, False otherwise.
            """
            return _ldns._ldns_rr_rrsig_set_algorithm(self, f)
            #parameters: ldns_rr *, ldns_rdf *,
            #retvals: bool

        def rrsig_set_expiration(self, f):
            """
               Sets the expiration date of a LDNS_RR_TYPE_RRSIG rr.
               
               :param f: The expiration date to set.
               :type f: :class:`ldns_rdf`
               :throws TypeError: when `f` of non-:class:`ldns_rdf` type.
               :return: (bool) True on success, False otherwise.
            """
            return _ldns._ldns_rr_rrsig_set_expiration(self, f)
            #parameters: ldns_rr *, ldns_rdf *,
            #retvals: bool

        def rrsig_set_inception(self, f):
            """
               Sets the inception date of a LDNS_RR_TYPE_RRSIG rr.
               
               :param f: The inception date to set.
               :type f: :class:`ldns_rdf`
               :throws TypeError: when `f` of non-:class:`ldns_rdf` type.
               :return: (bool) True on success, False otherwise.
            """
            return _ldns._ldns_rr_rrsig_set_inception(self, f)
            #parameters: ldns_rr *, ldns_rdf *,
            #retvals: bool

        def rrsig_set_keytag(self, f):
            """
               Sets the keytag of a LDNS_RR_TYPE_RRSIG rr.
               
               :param f: The keytag to set.
               :type f: :class:`ldns_rdf`
               :throws TypeError: when `f` of non-:class:`ldns_rdf` type.
               :return: (bool) True on success, False otherwise.
            """
            return _ldns._ldns_rr_rrsig_set_keytag(self, f)
            #parameters: ldns_rr *, ldns_rdf *,
            #retvals: bool

        def rrsig_set_labels(self, f):
            """
               Sets the number of labels of a LDNS_RR_TYPE_RRSIG rr.
               
               :param f: The number of labels to set.
               :type f: :class:`ldns_rdf`
               :throws TypeError: when `f` of non-:class:`ldns_rdf` type.
               :return: (bool) True on success, False otherwise.
            """
            return _ldns._ldns_rr_rrsig_set_labels(self, f)
            #parameters: ldns_rr *, ldns_rdf *,
            #retvals: bool

        def rrsig_set_origttl(self, f):
            """
               Sets the original TTL of a LDNS_RR_TYPE_RRSIG rr.
               
               :param f: The original TTL to set.
               :type f: :class:`ldns_rdf`
               :throws TypeError: when `f` of non-:class:`ldns_rdf` type.
               :return: (bool) True on success, False otherwise.
            """
            return _ldns._ldns_rr_rrsig_set_origttl(self, f)
            #parameters: ldns_rr *, ldns_rdf *,
            #retvals: bool

        def rrsig_set_sig(self, f):
            """
               Sets the signature data of a LDNS_RR_TYPE_RRSIG rr.
               
               :param f: The signature data to set.
               :type f: :class:`ldns_rdf`
               :throws TypeError: when `f` of non-:class:`ldns_rdf` type.
               :return: (bool) True on success, False otherwise.
            """
            return _ldns._ldns_rr_rrsig_set_sig(self, f)
            #parameters: ldns_rr *, ldns_rdf *,
            #retvals: bool

        def rrsig_set_signame(self, f):
            """
               Sets the signers name of a LDNS_RR_TYPE_RRSIG rr.
               
               :param f: The signers name to set.
               :type f: :class:`ldns_rdf`
               :throws TypeError: when `f` of non-:class:`ldns_rdf` type.
               :return: (bool) True on success, False otherwise.
            """
            return _ldns._ldns_rr_rrsig_set_signame(self, f)
            #parameters: ldns_rr *, ldns_rdf *,
            #retvals: bool

        def rrsig_set_typecovered(self, f):
            """
               Sets the typecovered of a LDNS_RR_TYPE_RRSIG rr.
               
               :param f: The type covered to set.
               :type f: :class:`ldns_rdf`
               :throws TypeError: when `f` of non-:class:`ldns_rdf` type.
               :return: (bool) True on success, False otherwise.
            """
            return _ldns._ldns_rr_rrsig_set_typecovered(self, f)
            #parameters: ldns_rr *, ldns_rdf *,
            #retvals: bool

        def rrsig_sig(self):
            """
               Returns the signature data of a LDNS_RR_TYPE_RRSIG RR.
               
               :return: (:class:`ldns_rdf`) with the signature data or None
                   on failure.
            """
            return _ldns._ldns_rr_rrsig_sig(self)
            #parameters: const ldns_rr *,
            #retvals: ldns_rdf *

        def rrsig_signame(self):
            """
               Returns the signers name of a LDNS_RR_TYPE_RRSIG RR.
               
               :return: (:class:`ldns_rdf`) with the signers name or None
                   on failure.
            """
            return _ldns._ldns_rr_rrsig_signame(self)
            #parameters: const ldns_rr *,
            #retvals: ldns_rdf *

        def rrsig_typecovered(self):
            """
               Returns the type covered of a LDNS_RR_TYPE_RRSIG rr.
               
               :return: (:class:`ldns_rdf`) with the type covered or None
                   on failure.
            """
            return _ldns._ldns_rr_rrsig_typecovered(self)
            #parameters: const ldns_rr *,
            #retvals: ldns_rdf *

        def set_class(self, rr_class):
            """
               Sets the class in the rr.
               
               :param rr_class: Set to this class.
               :type rr_class: int
               :throws TypeError: when `rr_class` of non-integer type.
            """
            _ldns.ldns_rr_set_class(self, rr_class)
            #parameters: ldns_rr *, ldns_rr_class,
            #retvals: 

        def set_owner(self, owner):
            """
               Sets the owner in the rr structure.
               
               :param owner: Owner name.
               :type owner: :class:`ldns_dname`
               :throws TypeError: when `owner` of non-:class:`ldns_dname` type.

               .. note::
                   The type checking of `owner` is benevolent.
                   It allows also to pass a dname :class:`ldns_rdf` object.
                   This will probably change in future.
            """
            if (not isinstance(owner, ldns_dname)) and \
               isinstance(owner, ldns_rdf) and \
               owner.get_type() == _ldns.LDNS_RDF_TYPE_DNAME:
                warnings.warn("The ldns_rr.new_frm_str() method will" +
                    " drop the possibility to accept ldns_rdf as owner." +
                    " Convert argument to ldns_dname.",
                    PendingDeprecationWarning, stacklevel=2)
            _ldns._ldns_rr_set_owner(self, owner)
            #parameters: ldns_rr *, ldns_rdf *,
            #retvals:

        def set_question(self, question):
            """
               Sets the question flag in the rr structure.

               :param question: Question flag.
               :type question: bool
            """
            _ldns.ldns_rr_set_question(self, question)
            #parameters: ldns_rr *, bool,
            #retvals:

        def set_rd_count(self, count):
            """
               Sets the rd_count in the rr.
               
               :param count: Set to this count.
               :type count: positive int
               :throws TypeError: when `count` of non-integer type.
            """
            _ldns.ldns_rr_set_rd_count(self, count)
            #parameters: ldns_rr *, size_t,
            #retvals: 

        def set_rdf(self, f, position):
            """
               Sets a rdf member, it will be set on the position given.
               
               The old value is returned, like pop.
               
               :param f: The rdf to be set.
               :type f: :class:`ldns_rdf`
               :param position: The position the set the rdf.
               :type position: positive int
               :throws TypeError: when mismatching types passed.
               :return: (:class:`ldns_rdf`) the old value in the rr, None
                   on failure.
            """
            return _ldns._ldns_rr_set_rdf(self, f, position)
            #parameters: ldns_rr *, const ldns_rdf *, size_t,
            #retvals: ldns_rdf *

        def set_ttl(self, ttl):
            """
               Sets the ttl in the rr structure.
               
               :param ttl: Set to this ttl.
               :type ttl: positive int
               :throws TypeError: when `ttl` of non-integer type.
            """
            _ldns.ldns_rr_set_ttl(self, ttl)
            #parameters: ldns_rr *, uint32_t,
            #retvals: 

        def set_type(self, rr_type):
            """
               Sets the type in the rr.
               
               :param rr_type: Set to this type.
               :type rr_type: integer
               :throws TypeError: when `rr_type` of non-integer type.
            """
            _ldns.ldns_rr_set_type(self, rr_type)
            #parameters: ldns_rr *, ldns_rr_type,
            #retvals:

        def to_canonical(self):
            """
               Converts each dname in a rr to its canonical form.
            """
            _ldns.ldns_rr2canonical(self)

        def ttl(self):
            """
               Returns the ttl of an rr structure.
               
               :return: (int) the ttl of the rr.
            """
            return _ldns.ldns_rr_ttl(self)
            #parameters: const ldns_rr *,
            #retvals: uint32_t

        @staticmethod
        def type_by_name(string):
            """
               Retrieves a rr type identifier value by looking up its name.

               Returns 0 if invalid name passed.

               :param string: RR type name.
               :type string: str
               :throws TypeError: when `string` of inappropriate type.
               :return: (int) RR type identifier, or 0 if no matching value
                   to identifier found.
            """
            return _ldns.ldns_get_rr_type_by_name(string)

        def uncompressed_size(self):
            """
               Calculates the uncompressed size of an RR.
               
               :return: (integer) size of the rr.
            """
            return _ldns.ldns_rr_uncompressed_size(self)
            #parameters: const ldns_rr *,
            #retvals: size_t

        #
        # _LDNS_RR_METHODS
        #
  %}
}


/* ========================================================================= */
/* SWIG setting and definitions. */
/* ========================================================================= */


%nodefaultctor ldns_struct_rr_list; /* No default constructor. */
%nodefaultdtor ldns_struct_rr_list; /* No default destructor. */

%ignore ldns_struct_rr_list::_rrs;

%newobject ldns_rr_list_cat_clone;
%newobject ldns_rr_list_clone;
%newobject ldns_rr_list_pop_rr;
%newobject ldns_rr_list_pop_rr_list;
%newobject ldns_rr_list_pop_rrset;
%newobject ldns_rr_list_rr;
%newobject ldns_rr_list_new;
%newobject ldns_get_rr_list_hosts_frm_file;
%newobject ldns_rr_list_subtype_by_rdf;
%newobject ldns_rr_list2str;
%delobject ldns_rr_list_deep_free;
%delobject ldns_rr_list_free;

/* Clone data on push. */

%rename(__ldns_rr_list_push_rr) ldns_rr_list_push_rr;
%inline
%{
  bool _ldns_rr_list_push_rr(ldns_rr_list* r, ldns_rr *rr)
  {
    bool ret;
    ldns_rr *new;

    new = ldns_rr_clone(rr);
    if (!(ret = ldns_rr_list_push_rr(r, new))) {
      ldns_rr_free(new);
    }
    return ret;
  }
%}

%rename(__ldns_rr_list_push_rr_list) ldns_rr_list_push_rr_list;
%inline
%{
  bool _ldns_rr_list_push_rr_list(ldns_rr_list* r, ldns_rr_list *r2)
  {
    bool ret;
    ldns_rr_list *new;

    new = ldns_rr_list_clone(r2);
    if (!(ret = ldns_rr_list_push_rr_list(r, new))) {
      ldns_rr_list_deep_free(new);
    }
    return ret;
  }
%}


%newobject _ldns_rr_list_set_rr;
%rename(__ldns_rr_list_set_rr) ldns_rr_list_set_rr;
%inline
%{
  ldns_rr * _ldns_rr_list_set_rr(ldns_rr_list * rrl, ldns_rr *rr,
    size_t idx)
  {
    ldns_rr *ret;
    ldns_rr *new;

    new = ldns_rr_clone(rr);
    if ((ret = ldns_rr_list_set_rr(rrl, new, idx)) == NULL) {
       ldns_rr_free(new);
    }
    return ret;
  }
%}


%rename(__ldns_rr_list_cat) ldns_rr_list_cat;
%inline
%{
  bool _ldns_rr_list_cat(ldns_rr_list *r, ldns_rr_list *r2)
  {
    return ldns_rr_list_cat(r, ldns_rr_list_clone(r2));
  }
%}


/* End clone data on push. */


/* Clone data on pull. */

%newobject _ldns_rr_list_rr;
%rename(__ldns_rr_list_rr) ldns_rr_list_rr;
%inline
%{
  ldns_rr * _ldns_rr_list_rr(ldns_rr_list *r, int i)
  {
    ldns_rr *rr;
    rr = ldns_rr_list_rr(r, i);
    return (rr != NULL) ? ldns_rr_clone(rr) : NULL;
  }
%}

%newobject _ldns_rr_list_owner;
%rename(__ldns_rr_list_owner) ldns_rr_list_owner;
%inline
%{
  ldns_rdf * _ldns_rr_list_owner(ldns_rr_list *r)
  {
    ldns_rdf *rdf;
    rdf = ldns_rr_list_owner(r);
    return (rdf != NULL) ? ldns_rdf_clone(rdf) : NULL;
  }
%}


/* End clone data on pull. */


/* ========================================================================= */
/* Debugging related code. */
/* ========================================================================= */


%rename(ldns_rr_list) ldns_struct_rr_list;
#ifdef LDNS_DEBUG
%rename(__ldns_rr_list_deep_free) ldns_rr_list_deep_free;
%rename(__ldns_rr_list_free) ldns_rr_list_free;
%inline
%{
  void _ldns_rr_list_deep_free(ldns_rr_list *r)
  {
    printf("******** LDNS_RR_LIST deep free 0x%lX ************\n",
      (long unsigned int) r);
    ldns_rr_list_deep_free(r);
  }

  void _ldns_rr_list_free(ldns_rr_list *r)
  {
    printf("******** LDNS_RR_LIST deep free 0x%lX ************\n",
      (long unsigned int) r);
    ldns_rr_list_free(r);
  }
%}
#else
%rename(_ldns_rr_list_deep_free) ldns_rr_list_deep_free;
%rename(_ldns_rr_list_free) ldns_rr_list_free;
#endif


/* ========================================================================= */
/* Added C code. */
/* ========================================================================= */


/* None. */


/* ========================================================================= */
/* Encapsulating Python code. */
/* ========================================================================= */


%feature("docstring") ldns_struct_rr_list "List of Resource Records.

This class contains a list of RR's (see :class:`ldns.ldns_rr`).
"

%extend ldns_struct_rr_list {
 
  %pythoncode
  %{
        def __init__(self):
            self.this = _ldns.ldns_rr_list_new()
            if not self.this:
                raise Exception("Can't create new RR_LIST")
       
        __swig_destroy__ = _ldns._ldns_rr_list_deep_free

        #
        # LDNS_RR_LIST_CONSTRUCTORS_
        #

        @staticmethod
        def new(raiseException=True):
            """
               Creates an empty RR List object.

               :param raiseException: Set to True if an exception should
                   signal an error.
               :type raiseException: bool
               :throws Exception: when `raiseException` is True and error
                   occurs.
               :return: :class:`ldns_rr_list` Empty RR list.
            """
            rrl = _ldns.ldns_rr_list_new()
            if (not rrl) and raiseException:
                raise Exception("Can't create RR List.")
            return rrl

        @staticmethod
        def new_frm_file(filename="/etc/hosts", raiseException=True):
            """
               Creates an RR List object from file content.

               Goes through a file and returns a rr list containing
               all the defined hosts in there.

               :param filename: The filename to use.
               :type filename: str
               :param raiseException: Set to True if an exception should
                   signal an error.
               :type raiseException: bool
               :throws TypeError: when `filename` of inappropriate type.
               :throws Exception: when `raiseException` is True and error
                   occurs.
               :return: RR List object or None. If the object can't be
                   created and `raiseException` is True, an exception occurs.

               **Usage**

               >>> alist = ldns.ldns_rr_list.new_frm_file()
               >>> print alist
               localhost.	3600	IN	A	127.0.0.1
               ...

            """
            rr = _ldns.ldns_get_rr_list_hosts_frm_file(filename)
            if (not rr) and (raiseException):
                raise Exception("Can't create RR List.")
            return rr

        #
        # _LDNS_RR_LIST_CONSTRUCTORS
        #

        def __str__(self):
            """
               Converts a list of resource records to presentation format.

               :return: (str) Presentation format.
            """
            return _ldns.ldns_rr_list2str(self)

        def print_to_file(self, output):
            """
               Print a rr_list to output.

               :param output: Opened file to print to.
               :throws TypeError: when `output` of inappropriate type.
            """
            _ldns.ldns_rr_list_print(output, self)


        def to_canonical(self):
            """
               Converts each dname in each rr in a rr_list to its canonical
               form.
            """
            _ldns.ldns_rr_list2canonical(self)
            #parameters: ldns_rr_list *,
            #retvals: 

        def rrs(self):
            """
               Returns a generator object of a list of rr records.

               :return: (generator) generator object.
            """
            for i in range(0, self.rr_count()):
                yield self.rr(i)

        def is_rrset(self):
            """
               Checks if the rr list is a rr set.

               :return: (bool) True if rr list is a rr set.
            """
            return _ldns.ldns_is_rrset(self)

        def __cmp__(self, rrl2):
            """
               Compares two rr lists.
               
               :param rrl2: The second one.
               :type rrl2: :class:`ldns_rr_list`
               :throws TypeError: when `rrl2` of non-:class:`ldns_rr_list`
                   type.
               :return: (int) 0 if equal, -1 if this list comes before
                   `rrl2`, 1 if `rrl2` comes before this list.
            """
            return _ldns.ldns_rr_list_compare(self, rrl2)

        def __lt__(self, other):
            """
               Compares two rr lists.
               
               :param other: The second one.
               :type other: :class:`ldns_rr_list`
               :throws TypeError: when `other` of non-:class:`ldns_rr_list`
                   type.
               :return: (bool) True when `self` is less than 'other'.
            """
            return _ldns.ldns_rr_list_compare(self, other) == -1

        def __le__(self, other):
            """
               Compares two rr lists.
               
               :param other: The second one.
               :type other: :class:`ldns_rr_list`
               :throws TypeError: when `other` of non-:class:`ldns_rr_list`
                   type.
               :return: (bool) True when `self` is less than or equal to
                   'other'.
            """
            return _ldns.ldns_rr_list_compare(self, other) != 1

        def __eq__(self, other):
            """
               Compares two rr lists.
               
               :param other: The second one.
               :type other: :class:`ldns_rr_list`
               :throws TypeError: when `other` of non-:class:`ldns_rr_list`
                   type.
               :return: (bool) True when `self` is equal to 'other'.
            """
            return _ldns.ldns_rr_list_compare(self, other) == 0

        def __ne__(self, other):
            """
               Compares two rr lists.
               
               :param other: The second one.
               :type other: :class:`ldns_rr_list`
               :throws TypeError: when `other` of non-:class:`ldns_rr_list`
                   type.
               :return: (bool) True when `self` is not equal to 'other'.
            """
            return _ldns.ldns_rr_list_compare(self, other) != 0

        def __gt__(self, other):
            """
               Compares two rr lists.
               
               :param other: The second one.
               :type other: :class:`ldns_rr_list`
               :throws TypeError: when `other` of non-:class:`ldns_rr_list`
                   type.
               :return: (bool) True when `self` is greater than 'other'.
            """
            return _ldns.ldns_rr_list_compare(self, other) == 1

        def __ge__(self, other):
            """
               Compares two rr lists.
               
               :param other: The second one.
               :type other: :class:`ldns_rr_list`
               :throws TypeError: when `other` of non-:class:`ldns_rr_list`
                   type.
               :return: (bool) True when `self` is greater than or equal to
                   'other'.
            """
            return _ldns.ldns_rr_list_compare(self, other) != -1

        def write_to_buffer(self, buffer):
            """
               Copies the rr_list data to the buffer in wire format.
               
               :param buffer: Output buffer to append the result to.
               :type buffer: :class:`ldns_buffer`
               :throws TypeError: when `buffer` of non-:class:`ldns_buffer`
                   type.
               :return: (ldns_status) ldns_status
            """
            return _ldns.ldns_rr_list2buffer_wire(buffer, self)

        #
        # LDNS_RR_LIST_METHODS_
        #

        def cat(self, right):
            """
               Concatenates two ldns_rr_lists together.
               
               This modifies rr list (to extend it and adds RRs from right).
               
               :param right: The right-hand side.
               :type right: :class:`ldns_rr_list`
               :throws TypeError: when `right` of non-:class:`ldns_rr_list`
                   type.
               :return: (bool) True if success.
            """
            return _ldns._ldns_rr_list_cat(self, right)
            #parameters: ldns_rr_list *, ldns_rr_list *,
            #retvals: bool

        def cat_clone(self, right):
            """
               Concatenates two ldns_rr_lists together, creates a new list
               of the rr's (instead of appending the content to an existing
               list).
               
               :param right: The right-hand side.
               :type right: :class:`ldns_rr_list`
               :throws TypeError: when `right` of non-:class:`ldns_rr_list`
                   type.
               :return: (:class:`ldns_rr_list`) rr list with left-hand side +
                   right-hand side concatenated, on None on error.
            """
            return _ldns.ldns_rr_list_cat_clone(self, right)
            #parameters: ldns_rr_list *, ldns_rr_list *,
            #retvals: ldns_rr_list *

        def clone(self):
            """
               Clones an rrlist.
               
               :return: (:class:`ldns_rr_list`) the cloned rr list,
                   or None on error.
            """
            return _ldns.ldns_rr_list_clone(self)
            #parameters: const ldns_rr_list *,
            #retvals: ldns_rr_list *

        def contains_rr(self, rr):
            """
               Returns True if the given rr is one of the rrs in the list,
               or if it is equal to one.
               
               :param rr: The rr to check.
               :type rr: :class:`ldns_rr`
               :throws TypeError: when `rr` of non-:class:`ldns_rr` type.
               :return: (bool) True if rr_list contains `rr`, False otherwise.
            """
            return _ldns.ldns_rr_list_contains_rr(self, rr)
            #parameters: const ldns_rr_list *, ldns_rr *,
            #retvals: bool

        def owner(self):
            """
               Returns the owner domain name rdf of the first element of
               the RR. If there are no elements present, None is returned.
               
               :return: (:class:`ldns_dname`) dname of the first element,
                   or None if the list is empty.
            """
            rdf = _ldns._ldns_rr_list_owner(self)
            if rdf:
                rdf = ldns_dname(rdf, clone=False)
            return rdf
            #parameters: const ldns_rr_list *,
            #retvals: ldns_rdf *

        def pop_rr(self):
            """
               Pops the last rr from an rrlist.
               
               :return: (:class:`ldns_rr`) None if nothing to pop.
                   Otherwise the popped RR.
            """
            rr = _ldns.ldns_rr_list_pop_rr(self)
            return rr
            #parameters: ldns_rr_list *,
            #retvals: ldns_rr *

        def pop_rr_list(self, size):
            """
               Pops an rr_list of size s from an rrlist.
               
               :param size: The number of rr's to pop.
               :type size: positive int
               :throws TypeError: when `size` of inappropriate type.
               :return: (:class:`ldns_rr_list`) None if nothing to pop.
                   Otherwise the popped rr list.
            """
            return _ldns.ldns_rr_list_pop_rr_list(self, size)
            #parameters: ldns_rr_list *, size_t,
            #retvals: ldns_rr_list *

        def pop_rrset(self):
            """
               Pops the first rrset from the list, the list must be sorted,
               so that all rr's from each rrset are next to each other.
               
               :return: (:class:`ldns_rr_list`) the first rrset, or None when
                   empty.
            """
            return _ldns.ldns_rr_list_pop_rrset(self)
            #parameters: ldns_rr_list *,
            #retvals: ldns_rr_list *

        def push_rr(self, rr):
            """
               Pushes an rr to an rrlist.
               
               :param rr: The rr to push.
               :type rr: :class:`ldns_rr`
               :throws TypeError: when `rr` of non-:class:`ldns_rr` type.
               :return: (bool) False on error, otherwise True.
            """
            return _ldns._ldns_rr_list_push_rr(self, rr)
            #parameters: ldns_rr_list *, const ldns_rr *,
            #retvals: bool

        def push_rr_list(self, push_list):
            """
               Pushes an rr list to an rr list.
               
               :param push_list: The rr_list to push.
               :type push_list: :class:`ldns_rr_list`
               :throws TypeError: when `push_list` of non-:class:`ldns_rr_list`
                   type.
               :returns: (bool) False on error, otherwise True.
            """
            return _ldns._ldns_rr_list_push_rr_list(self, push_list)
            #parameters: ldns_rr_list *, const ldns_rr_list *,
            #retvals: bool

        def rr(self, nr):
            """
               Returns a specific rr of an rrlist.
               
               :param nr: Index of the desired rr.
               :type nr: positive int
               :throws TypeError: when `nr` of inappropriate type.
               :return: (:class:`ldns_rr`) The rr at position `nr`, or None
                   if failed.
            """
            return _ldns._ldns_rr_list_rr(self, nr)
            #parameters: const ldns_rr_list *, size_t,
            #retvals: ldns_rr *

        def rr_count(self):
            """
               Returns the number of rr's in an rr_list.
               
               :return: (int) The number of rr's.
            """
            return _ldns.ldns_rr_list_rr_count(self)
            #parameters: const ldns_rr_list *,
            #retvals: size_t

        def set_rr(self, r, idx):
            """
               Set a rr on a specific index in a ldns_rr_list.
               
               :param r: The rr to set.
               :type r: :class:`ldns_rr`
               :param idx: Index into the rr_list.
               :type idx: positive int
               :throws TypeError: when parameters of inappropriate types.
               :return: (:class:`ldns_rr`) the old rr which was stored in
                   the rr_list, or None if the index was too large
                   to set a specific rr.
            """
            return _ldns._ldns_rr_list_set_rr(self, r, idx)
            #parameters: ldns_rr_list *, const ldns_rr *, size_t,
            #retvals: ldns_rr *

        def set_rr_count(self, count):
            """
               Sets the number of rr's in an rr_list.
               
               :param count: The number of rr in this list.
               :type count: positive int
               :throws TypeError: when `count` of non-integer type.
               :throws Exception: when `count` out of acceptable range.

               .. warning::
                   Don't use this method unless you really know what you
                   are doing.
            """
            # The function C has a tendency to generate an assertion fail when 
            # the count exceeds the list's capacity -- therefore the checking
            # code.
            if isinstance(count, int) and \
               ((count < 0) or (count > self._rr_capacity)):
                raise Exception("Given count %d is out of range " % (count) +
                    "of the rr list's capacity %d." % (self._rr_capacity))
            _ldns.ldns_rr_list_set_rr_count(self, count)
            #parameters: ldns_rr_list *, size_t,
            #retvals: 

        def sort(self):
            """
               Sorts an rr_list (canonical wire format).
            """
            _ldns.ldns_rr_list_sort(self)
            #parameters: ldns_rr_list *,
            #retvals: 

        def subtype_by_rdf(self, r, pos):
            """
               Return the rr_list which matches the rdf at position field.
               
               Think type-covered stuff for RRSIG.
               
               :param r: The rdf to use for the comparison.
               :type r: :class:`ldns_rdf`
               :param pos: At which position we can find the rdf.
               :type pos: positive int
               :throws TypeError: when parameters of inappropriate types.
               :return: (:class:`ldns_rr_list`) a new rr list with only
                   the RRs that match, or None when nothing matches.
            """
            return _ldns.ldns_rr_list_subtype_by_rdf(self, r, pos)
            #parameters: ldns_rr_list *, ldns_rdf *, size_t,
            #retvals: ldns_rr_list *

        def type(self):
            """
               Returns the type of the first element of the RR.

               If there are no elements present, 0 is returned.
               
               :return: (int) rr_type of the first element,
                   or 0 if the list is empty.
            """
            return _ldns.ldns_rr_list_type(self)
            #parameters: const ldns_rr_list *,
            #retvals: ldns_rr_type

        #
        # _LDNS_RR_LIST_METHODS
        #
  %}
}


/* ========================================================================= */
/* SWIG setting and definitions. */
/* ========================================================================= */


%newobject ldns_rr_descript;

%nodefaultctor ldns_struct_rr_descriptor; /* No default constructor. */
%nodefaultdtor ldns_struct_rr_descriptor; /* No default destructor.*/
%rename(ldns_rr_descriptor) ldns_struct_rr_descriptor;


/* ========================================================================= */
/* Debugging related code. */
/* ========================================================================= */

/* None. */


/* ========================================================================= */
/* Added C code. */
/* ========================================================================= */


%inline
%{
   /*
    * Does nothing, but keeps the SWIG wrapper quiet about absent destructor.
    */
   void ldns_rr_descriptor_dummy_free(const ldns_rr_descriptor *rd)
   {
     (void) rd;
   }
%}


/* ========================================================================= */
/* Encapsulating Python code. */
/* ========================================================================= */


%feature("docstring") ldns_struct_rr_descriptor "Resource Record descriptor.

This structure contains, for all rr types, the rdata fields that are defined.

In order to create a class instance use :meth:`ldns_rr_descriptor`.
"

%extend ldns_struct_rr_descriptor {
  %pythoncode
  %{
        def __init__(self, rr_type):
            """
               Returns the resource record descriptor for the given type.

               :param rr_type: RR type. 
               :type rr_type: int
               :throws TypeError: when `rr_type` of inappropriate type.
               :return: (:class:`ldns_rr_descriptor`) RR descriptor class.
            """
            self.this = self.ldns_rr_descriptor(rr_type)

        def __str__(self):
            raise Exception("The content of this class cannot be printed.")

        __swig_destroy__ = _ldns.ldns_rr_descriptor_dummy_free

        #
        # LDNS_RR_DESCRIPTOR_CONSTRUCTORS_
        #

        @staticmethod
        def ldns_rr_descriptor(rr_type):
            """
               Returns the resource record descriptor for the given type.

               :param rr_type: RR type. 
               :type rr_type: int
               :throws TypeError: when `rr_type` of inappropriate type.
               :return: (:class:`ldns_rr_descriptor`) RR descriptor class.
            """
            return _ldns.ldns_rr_descript(rr_type)
            #parameters: uint16_t
            #retvals: const ldns_rr_descriptor *

        #
        # _LDNS_RR_DESCRIPTOR_CONSTRUCTORS
        #

        #
        # LDNS_RR_DESCRIPTOR_METHODS_
        #

        def field_type(self, field):
            """
               Returns the rdf type for the given rdata field number of the
               rr type for the given descriptor.
               
               :param field: The field number.
               :type field: positive int
               :throws TypeError: when `field` of non-integer type.
               :return: (int) the rdf type for the field.
            """
            return _ldns.ldns_rr_descriptor_field_type(self, field)
            #parameters: const ldns_rr_descriptor *, size_t,
            #retvals: ldns_rdf_type

        def maximum(self):
            """
               Returns the maximum number of rdata fields of the rr type this
               descriptor describes.
               
               :return: (int) the maximum number of rdata fields.
            """
            return _ldns.ldns_rr_descriptor_maximum(self)
            #parameters: const ldns_rr_descriptor *,
            #retvals: size_t

        def minimum(self):
            """
               Returns the minimum number of rdata fields of the rr type this
               descriptor describes.
               
               :return: (int) the minimum number of rdata fields.
            """
            return _ldns.ldns_rr_descriptor_minimum(self)
            #parameters: const ldns_rr_descriptor *,
            #retvals: size_t

        #
        # _LDNS_RR_DESCRIPTOR_METHODS
        #
  %}
}


/* ========================================================================= */
/* Added C code. */
/* ========================================================================= */


/*
 * rrsig checking wrappers
 *
 * Copying of rr pointers into the good_keys list leads to double free
 * problems, therefore we provide two options - either ignore the keys
 * or get list of indexes of the keys. The latter allows fetching of the
 * keys later on from the original key set.
 */

%rename(__ldns_verify_rrsig_keylist) ldns_verify_rrsig_keylist;
%inline
%{
  ldns_status ldns_verify_rrsig_keylist_status_only(ldns_rr_list *rrset,
    ldns_rr *rrsig, const ldns_rr_list *keys)
  {
    ldns_rr_list *good_keys = ldns_rr_list_new();
    ldns_status status = ldns_verify_rrsig_keylist(rrset, rrsig, keys,
        good_keys);
    ldns_rr_list_free(good_keys);
    return status;
  }
%}

%rename(__ldns_verify_rrsig_keylist) ldns_verify_rrsig_keylist;
%inline
%{
  PyObject* ldns_verify_rrsig_keylist_(ldns_rr_list *rrset,
    ldns_rr *rrsig, const ldns_rr_list *keys)
  {
    PyObject* tuple;
    PyObject* keylist;
    ldns_rr_list *good_keys = ldns_rr_list_new();
    ldns_status status = ldns_verify_rrsig_keylist(rrset, rrsig, keys,
      good_keys);

    tuple = PyTuple_New(2);
    PyTuple_SetItem(tuple, 0, SWIG_From_int(status)); 
    keylist = PyList_New(0);
    if (status == LDNS_STATUS_OK) {
      unsigned int i;
      for (i = 0; i < ldns_rr_list_rr_count(keys); i++) {
         if (ldns_rr_list_contains_rr(good_keys, ldns_rr_list_rr(keys, i))) {
           PyList_Append(keylist, SWIG_From_int(i));
         }
      }
    }
    PyTuple_SetItem(tuple, 1, keylist);
    ldns_rr_list_free(good_keys);
    return tuple;
  }
%}


%rename(__ldns_verify_rrsig_keylist_notime) ldns_verify_rrsig_keylist_notime;
%inline
%{
  ldns_status ldns_verify_rrsig_keylist_notime_status_only(ldns_rr_list *rrset,
    ldns_rr *rrsig, const ldns_rr_list *keys)
  {
    ldns_rr_list *good_keys = ldns_rr_list_new();
    ldns_status status = ldns_verify_rrsig_keylist_notime(rrset, rrsig, keys,
      good_keys);
    ldns_rr_list_free(good_keys);
    return status;
  }
%}

%rename(__ldns_verify_rrsig_keylist_notime) ldns_verify_rrsig_keylist_notime;
%inline
%{
  PyObject* ldns_verify_rrsig_keylist_notime_(ldns_rr_list *rrset,
    ldns_rr *rrsig, const ldns_rr_list *keys)
  {
    PyObject* tuple;
    PyObject* keylist;
    ldns_rr_list *good_keys = ldns_rr_list_new();
    ldns_status status = ldns_verify_rrsig_keylist_notime(rrset, rrsig, keys,
      good_keys);

    tuple = PyTuple_New(2);
    PyTuple_SetItem(tuple, 0, SWIG_From_int(status)); 
    keylist = PyList_New(0);
    if (status == LDNS_STATUS_OK) {
      unsigned int i;
      for (i = 0; i < ldns_rr_list_rr_count(keys); i++) {
        if (ldns_rr_list_contains_rr(good_keys, ldns_rr_list_rr(keys, i))) {
          PyList_Append(keylist, SWIG_From_int(i));
        }
      }
    }
    PyTuple_SetItem(tuple, 1, keylist);
    ldns_rr_list_free(good_keys);
    return tuple;
  }
%}

/* End of rrsig checking wrappers. */
