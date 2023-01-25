/******************************************************************************
 * ldns_resolver.i: LDNS resolver class
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

/* Creates temporary instance of (ldns_resolver *). */
%typemap(in,numinputs=0,noblock=1) (ldns_resolver **r)
{
  ldns_resolver *$1_res;
  $1 = &$1_res;
}
          
/* Result generation, appends (ldns_resolver *) after the result. */
%typemap(argout,noblock=1) (ldns_resolver **r)
{
  $result = SWIG_Python_AppendOutput($result,
    SWIG_NewPointerObj(SWIG_as_voidptr($1_res),
      SWIGTYPE_p_ldns_struct_resolver, SWIG_POINTER_OWN |  0 ));
}

%newobject ldns_resolver_new;
%newobject ldns_resolver_pop_nameserver;
%newobject ldns_resolver_query;
%newobject ldns_resolver_search;
%newobject ldns_axfr_next;
%newobject ldns_get_rr_list_addr_by_name;
%newobject ldns_get_rr_list_name_by_addr;

%delobject ldns_resolver_deep_free;
%delobject ldns_resolver_free;

%nodefaultctor ldns_struct_resolver; /* No default constructor. */
%nodefaultdtor ldns_struct_resolver; /* No default destructor. */

%ignore ldns_struct_resolver::_searchlist;
%ignore ldns_struct_resolver::_nameservers;
%ignore ldns_resolver_set_nameservers;

%rename(ldns_resolver) ldns_struct_resolver;


/* Clone data on pull. */

%newobject _ldns_axfr_last_pkt;
%rename(__ldns_axfr_last_pkt) ldns_axfr_last_pkt;
%inline
%{
  ldns_pkt * _ldns_axfr_last_pkt(const ldns_resolver *res)
  {
    return ldns_pkt_clone(ldns_axfr_last_pkt(res));
  }
%}

%newobject _ldns_resolver_dnssec_anchors;
%rename(__ldns_resolver_dnssec_anchors) ldns_resolver_dnssec_anchors;
%inline
%{
  ldns_rr_list * _ldns_resolver_dnssec_anchors(const ldns_resolver *res)
  {
    return ldns_rr_list_clone(ldns_resolver_dnssec_anchors(res));
  }
%}

%newobject _ldns_resolver_domain;
%rename(__ldns_resolver_domain) ldns_resolver_domain;
%inline
%{
  ldns_rdf * _ldns_resolver_domain(const ldns_resolver *res)
  {
    /* Prevents assertion failures. */
    ldns_rdf *rdf;
    rdf = ldns_resolver_domain(res);
    if (rdf != NULL) {
      rdf = ldns_rdf_clone(rdf);
    }
    return rdf;
  }
%}

%newobject _ldns_resolver_tsig_algorithm;
%rename(__ldns_resolver_tsig_algorithm) ldns_resolver_tsig_algorithm;
%inline
%{
  const char * _ldns_resolver_tsig_algorithm(const ldns_resolver *res)
  {
    const char *str;
    str = ldns_resolver_tsig_algorithm(res);
    if (str != NULL) {
      str = strdup(str);
    }
    return str;
  }
%}

%newobject _ldns_resolver_tsig_keydata;
%rename(__ldns_resolver_tsig_keydata) ldns_resolver_tsig_keydata;
%inline
%{
  const char * _ldns_resolver_tsig_keydata(const ldns_resolver *res)
  {
    const char *str;
    str = ldns_resolver_tsig_keydata(res);
    if (str != NULL) {
      str = strdup(str);
    }
    return str;
  }
%}

%newobject _ldns_resolver_tsig_keyname;
%rename(__ldns_resolver_tsig_keyname) ldns_resolver_tsig_keyname;
%inline
%{
  const char * _ldns_resolver_tsig_keyname(const ldns_resolver *res)
  {
    const char *str;
    str = ldns_resolver_tsig_keyname(res);
    if (str != NULL) {
      str = strdup(str);
    }
    return str;
  }
%}

/* End of pull cloning. */

/* Clone data on push. */

%rename(__ldns_resolver_set_dnssec_anchors) ldns_resolver_set_dnssec_anchors;
%inline
%{
  void _ldns_resolver_set_dnssec_anchors(ldns_resolver *res,
    ldns_rr_list * rrl)
  {
    ldns_rr_list *rrl_clone = NULL;
    if (rrl != NULL) {
      rrl_clone = ldns_rr_list_clone(rrl);
    }
    /* May leak memory, when overwriting pointer value. */
    ldns_resolver_set_dnssec_anchors(res, rrl_clone);
  }
%}

%rename(__ldns_resolver_set_domain) ldns_resolver_set_domain;
%inline
%{
  void _ldns_resolver_set_domain(ldns_resolver *res, ldns_rdf *rdf)
  {
    ldns_rdf *rdf_clone = NULL;
    if (rdf != NULL) {
      rdf_clone = ldns_rdf_clone(rdf);
    }
    /* May leak memory, when overwriting pointer value. */
    ldns_resolver_set_domain(res, rdf_clone);
  }
%}

/* End of push cloning. */


/* ========================================================================= */
/* Debugging related code. */
/* ========================================================================= */

#ifdef LDNS_DEBUG
%rename(__ldns_resolver_deep_free) ldns_resolver_deep_free;
%rename(__ldns_resolver_free) ldns_resolver_free;
%inline
%{
  /*!
   * @brief Prints information about deallocated resolver and deallocates.
   */
  void _ldns_resolver_deep_free(ldns_resolver *r)
  {
    printf("******** LDNS_RESOLVER deep free 0x%lX ************\n",
      (long unsigned int) r);
    ldns_resolver_deep_free(r);
  }

  /*!
   * @brief Prints information about deallocated resolver and deallocates.
   *
   * @note There should be no need to use this function in the wrapper code, as
   *    it is likely to leak memory.
   */
  void _ldns_resolver_free(ldns_resolver *r)
  {
    printf("******** LDNS_RESOLVER free 0x%lX ************\n",
      (long unsigned int) r);
    ldns_resolver_free(r);
  }
%}
#else /* !LDNS_DEBUG */
%rename(_ldns_resolver_deep_free) ldns_resolver_deep_free;
%rename(_ldns_resolver_free) ldns_resolver_free;
#endif /* LDNS_DEBUG */


/* ========================================================================= */
/* Added C code. */
/* ========================================================================= */

%newobject _replacement_ldns_resolver_trusted_key;
%inline
%{
  /*!
   * @brief Replaces the rrs in the list with their clones.
   *
   * Prevents memory corruption when automatically deallocating list content.
   */
  void _rr_list_replace_content_with_clones(ldns_rr_list *rrl)
  {
    size_t count;
    unsigned int i;

    if (rrl == NULL) {
      return;
    }

    count = ldns_rr_list_rr_count(rrl);
    for (i = 0; i < count; ++i) {
      ldns_rr_list_set_rr(rrl,
        ldns_rr_clone(ldns_rr_list_rr(rrl, i)),
        i);
    }
  }

  /*
   * @brief Behaves similarly to ldns_resolver_trusted_key().
   *
   * Prevents memory leakage by controlling the usage of content cloning.
   *
   * @return Newly allocated list of trusted key clones if any found,
   *     NULL else.
   */
  ldns_rr_list * _replacement_ldns_resolver_trusted_key(
    const ldns_resolver *res, ldns_rr_list *keys)
  {
    ldns_rr_list *trusted_keys = ldns_rr_list_new();

    if (ldns_resolver_trusted_key(res, keys, trusted_keys)) {
      _rr_list_replace_content_with_clones(trusted_keys);
    } else {
      ldns_rr_list_deep_free(trusted_keys); trusted_keys = NULL;
    }

    return trusted_keys;
  }
%}


/* ========================================================================= */
/* Encapsulating Python code. */
/* ========================================================================= */

%feature("docstring") ldns_struct_resolver "LDNS resolver object. 

The :class:`ldns_resolver` object keeps a list of name servers and can perform
queries.

**Usage**

>>> import ldns
>>> resolver = ldns.ldns_resolver.new_frm_file(\"/etc/resolv.conf\")
>>> pkt = resolver.query(\"www.nic.cz\", ldns.LDNS_RR_TYPE_A,ldns.LDNS_RR_CLASS_IN)
>>> if (pkt) and (pkt.answer()): 
>>>    print pkt.answer()
www.nic.cz.	1757	IN	A	217.31.205.50

This simple example instances a resolver in order to resolve www.nic.cz A type
record."

%extend ldns_struct_resolver {
 
  %pythoncode
  %{
        def __init__(self):
            """
               Cannot be created directly from Python.
            """
            raise Exception("This class can't be created directly. " +
                "Please use: new_frm_file(filename), new_frm_fp(file) " +
                "or new_frm_fp_l(file, line)")

        __swig_destroy__ = _ldns._ldns_resolver_deep_free

        #
        # LDNS_RESOLVER_CONSTRUCTORS_
        #

        @staticmethod
        def new():
            """
               Creates a new resolver object.

               :return: (:class:`ldns_resolver`) New resolver object or None.

               .. note::
                   The returned resolver object is unusable unless some
                   name servers are added.

               **Usage**
                 >>> resolver = ldns.ldns_resolver.new()
                 >>> ns_addr = ldns.ldns_rdf.new_frm_str("8.8.8.8", ldns.LDNS_RDF_TYPE_A)
                 >>> if not ns_addr: raise Exception("Can't create resolver address.")
                 >>> status = resolver.push_nameserver(ns_addr)
                 >>> if status != ldns.LDNS_STATUS_OK: raise Exception("Can't push resolver address.")
                 >>> pkt = resolver.query("www.nic.cz.", ldns.LDNS_RR_TYPE_A, ldns.LDNS_RR_CLASS_IN, ldns.LDNS_RD)
                 >>> if (pkt) and (pkt.answer()):
                 >>>     print pkt.answer()
                 www.nic.cz.     1265    IN      A       217.31.205.50
            """
            return _ldns.ldns_resolver_new()

        @staticmethod
        def new_frm_file(filename = "/etc/resolv.conf", raiseException=True):
            """
               Creates a resolver object from given file name
               
               :param filename: Name of file which contains resolver
                   information (usually /etc/resolv.conf).
               :type filename: str
               :param raiseException: If True, an exception occurs in case a
                   resolver object can't be created.
               :type raiseException: bool
               :throws TypeError: When arguments of inappropriate types.
               :throws Exception: When `raiseException` set and resolver
                   couldn't be created.
               :return: (:class:`ldns_resolver`) Resolver object or None.
                   An exception occurs if the object can't be created and
                   'raiseException' is True.
            """
            status, resolver = _ldns.ldns_resolver_new_frm_file(filename)
            if status != LDNS_STATUS_OK:
                if (raiseException):
                    raise Exception("Can't create resolver, error: %d" % status)
                return None
            return resolver

        @staticmethod
        def new_frm_fp(file, raiseException=True):
            """
               Creates a resolver object from file
               
               :param file: A file object.
               :type file: file
               :param raiseException: If True, an exception occurs in case a
                   resolver object can't be created.
               :type raiseException: bool
               :throws TypeError: When arguments of inappropriate types.
               :throws Exception: When `raiseException` set and resolver
                   couldn't be created.
               :return: (:class:`ldns_resolver`) Resolver object or None.
                   An exception occurs if the object can't be created and
                   `raiseException` is True.
            """
            status, resolver = _ldns.ldns_resolver_new_frm_fp(file)
            if status != LDNS_STATUS_OK:
                if (raiseException):
                    raise Exception("Can't create resolver, error: %d" % status)
                return None
            return resolver

        @staticmethod
        def new_frm_fp_l(file, raiseException=True):
            """
               Creates a resolver object from file
               
               :param file: A file object.
               :type file: file
               :param raiseException: If True, an exception occurs in case a
                   resolver instance can't be created.
               :type raiseException: bool
               :throws TypeError: When arguments of inappropriate types.
               :throws Exception: When `raiseException` set and resolver
                   couldn't be created.
               :return: 
                  * (:class:`ldns_resolver`) Resolver instance or None.
                      An exception occurs if an instance can't be created and
                      `raiseException` is True.

                  * (int) - The line number. (e.g., for debugging)
            """
            status, resolver, line = _ldns.ldns_resolver_new_frm_fp_l(file)
            if status != LDNS_STATUS_OK:
                if (raiseException):
                    raise Exception("Can't create resolver, error: %d" % status)
                return None
            return resolver, line

        #
        # _LDNS_RESOLVER_CONSTRUCTORS
        #

        # High level functions

        def get_addr_by_name(self, name, aclass = _ldns.LDNS_RR_CLASS_IN, flags = _ldns.LDNS_RD):
            """
               Ask the resolver about name and return all address records.

               :param name: The name to look for. String is automatically
                   converted to dname.
               :type name: :class:`ldns_dname` or str
               :param aclass: The class to use.
               :type aclass: ldns_rr_class
               :param flags: Give some optional flags to the query.
               :type flags: uint16_t
               :throws TypeError: When arguments of inappropriate types.
               :return: (:class:`ldns_rr_list`) RR List object or None.

               **Usage**
                 >>> addr = resolver.get_addr_by_name("www.google.com", ldns.LDNS_RR_CLASS_IN, ldns.LDNS_RD)
                 >>> if (not addr): raise Exception("Can't retrieve server address")
                 >>> for rr in addr.rrs():
                 >>>     print rr
                 www.l.google.com.	300	IN	A	74.125.43.99
                 www.l.google.com.	300	IN	A	74.125.43.103
                 www.l.google.com.	300	IN	A	74.125.43.104
                 www.l.google.com.	300	IN	A	74.125.43.147
            """
            rdf = name
            if isinstance(name, str):
                rdf =  _ldns.ldns_dname_new_frm_str(name)
            return _ldns.ldns_get_rr_list_addr_by_name(self, rdf, aclass, flags)

        def get_name_by_addr(self, addr, aclass = _ldns.LDNS_RR_CLASS_IN, flags = _ldns.LDNS_RD):
            """
               Ask the resolver about the address and return the name.

               :param name: (ldns_rdf of A or AAAA type) the addr to look for.
                   If a string is given, A or AAAA type is identified
                   automatically.
               :type name: :class:`ldns_rdf` of A or AAAA type
               :param aclass: The class to use.
               :type aclass: ldns_rr_class
               :param flags: Give some optional flags to the query.
               :type flags: uint16_t
               :throws TypeError: When arguments of inappropriate types.
               :return: (:class:`ldns_rr_list`) RR List object or None.

               **Usage**
                 >>> addr = resolver.get_name_by_addr("74.125.43.99", ldns.LDNS_RR_CLASS_IN, ldns.LDNS_RD)
                 >>> if (not addr): raise Exception("Can't retrieve server address")
                 >>> for rr in addr.rrs():
                 >>>     print rr
                 99.43.125.74.in-addr.arpa.	85641	IN	PTR	bw-in-f99.google.com.
                    
            """
            rdf = addr
            if isinstance(addr, str):
                if (addr.find("::") >= 0): #IPv6
                    rdf = _ldns.ldns_rdf_new_frm_str(_ldns.LDNS_RDF_TYPE_AAAA, addr)
                else:
                    rdf = _ldns.ldns_rdf_new_frm_str(_ldns.LDNS_RDF_TYPE_A, addr)
            return _ldns.ldns_get_rr_list_name_by_addr(self, rdf, aclass, flags)

        def print_to_file(self,output):
            """Print a resolver (in so far that is possible) state to output."""
            _ldns.ldns_resolver_print(output,self)

        def axfr_complete(self):
            """
               Returns True if the axfr transfer has completed
               (i.e., 2 SOA RRs and no errors were encountered).

               :return: (bool)
            """
            return _ldns.ldns_axfr_complete(self)
            #parameters: const ldns_resolver *,
            #retvals: bool

        def axfr_last_pkt(self):
            """
               Returns a last packet that was sent by the server in the AXFR
               transfer (usable for instance to get the error code on failure).

               :return: (:class:`ldns_pkt`) Last packet of the AXFR transfer.
            """
            return _ldns._ldns_axfr_last_pkt(self)
            #parameters: const ldns_resolver *,
            #retvals: ldns_pkt *

        def axfr_next(self):
            """
               Get the next stream of RRs in a AXFR.

               :return: (:class:`ldns_rr`) The next RR from the AXFR stream.
            """
            return _ldns.ldns_axfr_next(self)
            #parameters: ldns_resolver *,
            #retvals: ldns_rr *

        def axfr_start(self, domain, aclass):
            """
               Prepares the resolver for an axfr query. The query is sent and
               the answers can be read with :meth:`axfr_next`.

               :param domain: Domain to axfr.
               :type domain: :class:`dlsn_dname`
               :param aclass: The class to use.
               :type aclass: ldns_rr_class
               :throws TypeError: When arguments of inappropriate types.
               :return: (ldns_status) The status of the transfer.

               .. note::
                   The type checking of parameter `domain` is benevolent.
                   It allows also to pass a dname :class:`ldns_rdf` object.
                   This will probably change in future.

               **Usage**
               ::
    
                  status = resolver.axfr_start("nic.cz", ldns.LDNS_RR_CLASS_IN)
                  if (status != ldns.LDNS_STATUS_OK):
                      raise Exception("Can't start AXFR, error: %s" % ldns.ldns_get_errorstr_by_id(status))
                  #Print the results
                  while True:
                       rr = resolver.axfr_next()
                       if not rr: 
                          break

                       print rr
            """
            # TODO -- Add checking for ldns_rdf and ldns_dname.
            rdf = domain
            if isinstance(domain, str):
                rdf = _ldns.ldns_dname_new_frm_str(domain)
            return _ldns.ldns_axfr_start(self, rdf, aclass)
            #parameters: ldns_resolver *resolver, ldns_rdf *domain, ldns_rr_class c
            #retvals: int

        #
        # LDNS_RESOLVER_METHODS_
        #

        def debug(self):
            """
               Get the debug status of the resolver.
               
               :return: (bool) True if so, otherwise False.
            """
            return _ldns.ldns_resolver_debug(self)
            #parameters: const ldns_resolver *,
            #retvals: bool

        def dec_nameserver_count(self):
            """
               Decrement the resolver's name server count.
            """
            _ldns.ldns_resolver_dec_nameserver_count(self)
            #parameters: ldns_resolver *,
            #retvals: 

        def defnames(self):
            """
               Does the resolver apply default domain name.

               :return: (bool)
            """
            return _ldns.ldns_resolver_defnames(self)
            #parameters: const ldns_resolver *,
            #retvals: bool

        def dnsrch(self):
            """
               Does the resolver apply search list.

               :return: (bool)
            """
            return _ldns.ldns_resolver_dnsrch(self)
            #parameters: const ldns_resolver *,
            #retvals: bool

        def dnssec(self):
            """
               Does the resolver do DNSSEC.
               
               :return: (bool) True: yes, False: no.
            """
            return _ldns.ldns_resolver_dnssec(self)
            #parameters: const ldns_resolver *,
            #retvals: bool

        def dnssec_anchors(self):
            """
               Get the resolver's DNSSEC anchors.
               
               :return: (:class:`ldns_rr_list`) An rr list containing trusted
                   DNSSEC anchors.
            """
            return _ldns._ldns_resolver_dnssec_anchors(self)
            #parameters: const ldns_resolver *,
            #retvals: ldns_rr_list *

        def dnssec_cd(self):
            """
               Does the resolver set the CD bit.
               
               :return: (bool) True: yes, False: no.
            """
            return _ldns.ldns_resolver_dnssec_cd(self)
            #parameters: const ldns_resolver *,
            #retvals: bool

        def domain(self):
            """
               What is the default dname to add to relative queries.
               
               :return: (:class:`ldns_dname`) The dname which is added.
            """
            dname = _ldns._ldns_resolver_domain(self)
            if dname != None:
                return ldns_dname(_ldns._ldns_resolver_domain(self), clone=False)
            else:
                return dname
            #parameters: const ldns_resolver *,
            #retvals: ldns_rdf *

        def edns_udp_size(self):
            """
               Get the resolver's udp size.
               
               :return: (uint16_t) The udp mesg size.
            """
            return _ldns.ldns_resolver_edns_udp_size(self)
            #parameters: const ldns_resolver *,
            #retvals: uint16_t

        def fail(self):
            """
               Does the resolver only try the first name server.
               
               :return: (bool) True: yes, fail, False: no, try the others.
            """
            return _ldns.ldns_resolver_fail(self)
            #parameters: const ldns_resolver *,
            #retvals: bool

        def fallback(self):
            """
               Get the truncation fall-back status.
               
               :return: (bool) Whether the truncation fall*back mechanism
                   is used.
            """
            return _ldns.ldns_resolver_fallback(self)
            #parameters: const ldns_resolver *,
            #retvals: bool

        def igntc(self):
            """
               Does the resolver ignore the TC bit (truncated).
               
               :return: (bool) True: yes, False: no.
            """
            return _ldns.ldns_resolver_igntc(self)
            #parameters: const ldns_resolver *,
            #retvals: bool

        def incr_nameserver_count(self):
            """
               Increment the resolver's name server count.
            """
            _ldns.ldns_resolver_incr_nameserver_count(self)
            #parameters: ldns_resolver *,
            #retvals: 

        def ip6(self):
            """
               Does the resolver use ip6 or ip4.
               
               :return: (uint8_t) 0: both, 1: ip4, 2:ip6
            """
            return _ldns.ldns_resolver_ip6(self)
            #parameters: const ldns_resolver *,
            #retvals: uint8_t

        def nameserver_count(self):
            """
               How many name server are configured in the resolver.
               
               :return: (size_t) Number of name servers.
            """
            return _ldns.ldns_resolver_nameserver_count(self)
            #parameters: const ldns_resolver *,
            #retvals: size_t

        def nameserver_rtt(self, pos):
            """
               Return the used round trip time for a specific name server.
               
               :param pos: The index to the name server.
               :type pos: size_t
               :throws TypeError: When arguments of inappropriate types.
               :return: (size_t) The rrt, 0: infinite,
                   >0: undefined (as of * yet).
            """
            return _ldns.ldns_resolver_nameserver_rtt(self, pos)
            #parameters: const ldns_resolver *,size_t,
            #retvals: size_t

        def nameservers(self):
            """
               Return the configured name server ip address.
               
               :return: (ldns_rdf \*\*) A ldns_rdf pointer to a list of the
                   addresses.
            """
            # TODO -- Convert to list of ldns_rdf.
            return _ldns.ldns_resolver_nameservers(self)
            #parameters: const ldns_resolver *,
            #retvals: ldns_rdf **

        def nameservers_randomize(self):
            """
               Randomize the name server list in the resolver.
            """
            _ldns.ldns_resolver_nameservers_randomize(self)
            #parameters: ldns_resolver *,
            #retvals: 

        def pop_nameserver(self):
            """
               Pop the last name server from the resolver.
               
               :return: (:class:`ldns_rdf`) The popped address or None if empty.
            """
            return _ldns.ldns_resolver_pop_nameserver(self)
            #parameters: ldns_resolver *,
            #retvals: ldns_rdf *

        def port(self):
            """
               Get the port the resolver should use.
               
               :return: (uint16_t) The port number.
            """
            return _ldns.ldns_resolver_port(self)
            #parameters: const ldns_resolver *,
            #retvals: uint16_t

        def prepare_query_pkt(self, name, t, c, f, raiseException=True):
            """
               Form a query packet from a resolver and name/type/class combo.
               
               :param name: Query for this name.
               :type name: :class:`ldns_dname` or str
               :param t: Query for this type (may be 0, defaults to A).
               :type t: ldns_rr_type
               :param c: Query for this class (may be 0, default to IN).
               :type c: ldns_rr_class
               :param f: The query flags.
               :type f: uint16_t
               :throws TypeError: When arguments of inappropriate types.
               :throws Exception: When `raiseException` set and answer
                   couldn't be resolved.
               :return: (:class:`ldns_pkt`) Query packet or None.
                   An exception occurs if the object can't be created and
                   'raiseException' is True.
            """
            rdf = name
            if isinstance(name, str):
                rdf = _ldns.ldns_dname_new_frm_str(name)
            status, pkt = _ldns.ldns_resolver_prepare_query_pkt(self, rdf, t, c, f)
            if status != LDNS_STATUS_OK:
                if (raiseException):
                    raise Exception("Can't create resolver, error: %d" % status)
                return None
            return pkt
            #parameters: ldns_resolver *,const ldns_rdf *,ldns_rr_type,ldns_rr_class,uint16_t,
            #retvals: ldns_status,ldns_pkt **

        def push_dnssec_anchor(self, rr):
            """
               Push a new trust anchor to the resolver.
               It must be a DS or DNSKEY rr.
               
               :param rr: The RR to add as a trust anchor.
               :type rr: DS of DNSKEY :class:`ldns_rr`
               :throws TypeError: When arguments of inappropriate types.
               :return: (ldns_status) A status.
            """
            return _ldns.ldns_resolver_push_dnssec_anchor(self, rr)
            #parameters: ldns_resolver *,ldns_rr *,
            #retvals: ldns_status

        def push_nameserver(self, n):
            """
               Push a new name server to the resolver.
               It must be an IP address v4 or v6.
               
               :param n: The ip address.
               :type n: :class:`ldns_rdf` of A or AAAA type.
               :throws TypeError: When arguments of inappropriate types.
               :return: (ldns_status) A status.
            """
            return _ldns.ldns_resolver_push_nameserver(self, n)
            #parameters: ldns_resolver *,ldns_rdf *,
            #retvals: ldns_status

        def push_nameserver_rr(self, rr):
            """
               Push a new name server to the resolver.
               It must be an A or AAAA RR record type.
               
               :param rr: The resource record.
               :type rr: :class:`ldns_rr` of A or AAAA type.
               :throws TypeError: When arguments of inappropriate types.
               :return: (ldns_status) A status.
            """
            return _ldns.ldns_resolver_push_nameserver_rr(self, rr)
            #parameters: ldns_resolver *,ldns_rr *,
            #retvals: ldns_status

        def push_nameserver_rr_list(self, rrlist):
            """
               Push a new name server rr_list to the resolver.
               
               :param rrlist: The rr list to push.
               :type rrlist: :class:`ldns_rr_list`
               :throws TypeError: When arguments of inappropriate types.
               :return: (ldns_status) A status.
            """
            return _ldns.ldns_resolver_push_nameserver_rr_list(self, rrlist)
            #parameters: ldns_resolver *,ldns_rr_list *,
            #retvals: ldns_status

        def push_searchlist(self, rd):
            """
               Push a new rd to the resolver's search-list.
               
               :param rd: To push.
               :param rd: :class:`ldns_dname` or str
               :throws TypeError: When arguments of inappropriate types.

               .. note:
                   The function does not return any return status,
                   so the caller must ensure the correctness of the passed
                   values.
            """
            rdf = rd
            if isinstance(rd, str):
                rdf = _ldns.ldns_dname_new_frm_str(rd)
            _ldns.ldns_resolver_push_searchlist(self, rdf)
            #parameters: ldns_resolver *,ldns_rdf *,
            #retvals: 

        def query(self,name,atype=_ldns.LDNS_RR_TYPE_A,aclass=_ldns.LDNS_RR_CLASS_IN,flags=_ldns.LDNS_RD):
            """
               Send a query to a name server.
               
               :param name: The name to look for.
               :type name: :class:`ldns_dname` or str
               :param atype: The RR type to use.
               :type atype: ldns_rr_type
               :param aclass: The RR class to use.
               :type aclass: ldns_rr_class
               :param flags: Give some optional flags to the query.
               :type flags: uint16_t
               :throws TypeError: When arguments of inappropriate types.
               :return: (:class:`ldns_pkt`) A packet with the reply from the
                   name server if _defnames is true the default domain will
                   be added.
            """
            # Explicit conversion from string to ldns_rdf prevents memory leaks.
            # TODO -- Find out why.
            dname = name
            if isinstance(name, str):
                dname = _ldns.ldns_dname_new_frm_str(name)
            return _ldns.ldns_resolver_query(self, dname, atype, aclass, flags)
            #parameters: const ldns_resolver *,const ldns_rdf *,ldns_rr_type,ldns_rr_class,uint16_t,
            #retvals: ldns_pkt *

        def random(self):
            """
               Does the resolver randomize the name server before usage?
               
               :return: (bool) True: yes, False: no.
            """
            return _ldns.ldns_resolver_random(self)
            #parameters: const ldns_resolver *,
            #retvals: bool

        def recursive(self):
            """
               Is the resolver set to recurse?
               
               :return: (bool) True if so, otherwise False.
            """
            return _ldns.ldns_resolver_recursive(self)
            #parameters: const ldns_resolver *,
            #retvals: bool

        def retrans(self):
            """
               Get the retransmit interval.
               
               :return: (uint8_t) The retransmit interval.
            """
            return _ldns.ldns_resolver_retrans(self)
            #parameters: const ldns_resolver *,
            #retvals: uint8_t

        def retry(self):
            """
               Get the number of retries.
               
               :return: (uint8_t) The number of retries.
            """
            return _ldns.ldns_resolver_retry(self)
            #parameters: const ldns_resolver *,
            #retvals: uint8_t

        def rtt(self):
            """
               Return the used round trip times for the name servers.
               
               :return: (size_t \*) a size_t* pointer to the list. yet)
            """
            return _ldns.ldns_resolver_rtt(self)
            #parameters: const ldns_resolver *,
            #retvals: size_t *

        def search(self, name, atype=_ldns.LDNS_RR_TYPE_A, aclass=_ldns.LDNS_RR_CLASS_IN, flags=_ldns.LDNS_RD):
            """
               Send the query for using the resolver and take the search list
               into account The search algorithm is as follows: If the name is
               absolute, try it as-is, otherwise apply the search list.

               :param name: The name to look for.
               :type name: :class:`ldns_dname` or str
               :param atype: The RR type to use.
               :type atype: ldns_rr_type
               :param aclass: The RR class to use.
               :type aclass: ldns_rr_class
               :param flags: Give some optional flags to the query.
               :type flags: uint16_t
               :throws TypeError: When arguments of inappropriate types.
               :return: (:class:`ldns_pkt`) A packet with the reply from the
                   name server.
            """
            # Explicit conversion from string to ldns_rdf prevents memory leaks.
            # TODO -- Find out why.
            dname = name
            if isinstance(name, str):
                dname = _ldns.ldns_dname_new_frm_str(name)
            return _ldns.ldns_resolver_search(self, dname, atype, aclass, flags)
            #parameters: const ldns_resolver *,const ldns_rdf *,ldns_rr_type,ldns_rr_class,uint16_t,
            #retvals: ldns_pkt *

        def searchlist(self):
            """
               What is the search-list as used by the resolver.
               
               :return: (ldns_rdf \*\*) A ldns_rdf pointer to a list of the addresses.
            """
            return _ldns.ldns_resolver_searchlist(self)
            #parameters: const ldns_resolver *,
            #retvals: ldns_rdf \*\*

        def searchlist_count(self):
            """
               Return the resolver's search-list count.
               
               :return: (size_t) The search-list count.
            """
            return _ldns.ldns_resolver_searchlist_count(self)
            #parameters: const ldns_resolver *,
            #retvals: size_t

        def send(self, name, atype, aclass, flags, raiseException=True):
            """
               Send the query for name as-is.

               :param name: The name to look for.
               :type name: :class:`ldns_dname` or str
               :param atype: The RR type to use.
               :type atype: ldns_rr_type
               :param aclass: The RR class to use.
               :type aclass: ldns_rr_class
               :param flags: Give some optional flags to the query.
               :type flags: uint16_t
               :throws TypeError: When arguments of inappropriate types.
               :throws Exception: When `raiseException` set and answer
                   couldn't be resolved.
               :return: (:class:`ldns_pkt`) A packet with the reply from the
                   name server.
            """
            # Explicit conversion from string to ldns_rdf prevents memory leaks.
            # TODO -- Find out why.
            dname = name
            if isinstance(name, str):
                dname = _ldns.ldns_dname_new_frm_str(name)
            status, pkt = _ldns.ldns_resolver_send(self, dname, atype, aclass, flags)
            if status != LDNS_STATUS_OK:
                if (raiseException):
                    raise Exception("Can't create resolver, error: %d" % status)
                return None
            return pkt
            #parameters: ldns_resolver *,const ldns_rdf *,ldns_rr_type,ldns_rr_class,uint16_t,
            #retvals: ldns_status,ldns_pkt **

        def send_pkt(self, query_pkt):
            """
               Send the given packet to a name server.
               
               :param query_pkt: Query packet.
               :type query_pkt: :class:`ldns_pkt`
               :throws TypeError: When arguments of inappropriate types.
               :return: * (ldns_status) Return status.
                        * (:class:`ldns_pkt`) Response packet if returns status ok.
            """
            status, answer = _ldns.ldns_resolver_send_pkt(self, query_pkt)
            return _ldns.ldns_resolver_send_pkt(self,query_pkt)
            #parameters: ldns_resolver *,ldns_pkt *,
            #retvals: ldns_status,ldns_pkt **

        def set_debug(self, b):
            """
               Set the resolver debugging.
               
               :param b: True: debug on, False: debug off.
               :type b: bool
            """
            _ldns.ldns_resolver_set_debug(self, b)
            #parameters: ldns_resolver *,bool,
            #retvals: 

        def set_defnames(self, b):
            """
               Whether the resolver uses the name set with _set_domain.
               
               :param b: True: use the defaults, False: don't use them.
               :type b: bool
            """
            _ldns.ldns_resolver_set_defnames(self, b)
            #parameters: ldns_resolver *,bool,
            #retvals: 

        def set_dnsrch(self, b):
            """
               Whether the resolver uses the search list.
               
               :param b: True: use the list, False: don't use the list.
               :type b: bool
            """
            _ldns.ldns_resolver_set_dnsrch(self, b)
            #parameters: ldns_resolver *,bool,
            #retvals: 

        def set_dnssec(self, b):
            """
               Whether the resolver uses DNSSEC.
               
               :param b: True: use DNSSEC, False: don't use DNSSEC.
               :type b: bool
            """
            _ldns.ldns_resolver_set_dnssec(self, b)
            #parameters: ldns_resolver *,bool,
            #retvals: 

        def set_dnssec_anchors(self, l):
            """
               Set the resolver's DNSSEC anchor list directly.
               RRs should be of type DS or DNSKEY.
               
               :param l: The list of RRs to use as trust anchors.
               :type l: :class:`ldns_rr_list`
               :throws TypeError: When arguments of inappropriate types.
            """
            _ldns._ldns_resolver_set_dnssec_anchors(self, l)
            #parameters: ldns_resolver *,ldns_rr_list *,
            #retvals: 

        def set_dnssec_cd(self, b):
            """
               Whether the resolver uses the checking disable bit.
               
               :param b: True: enable, False: disable.
               :type b: bool
            """
            _ldns.ldns_resolver_set_dnssec_cd(self, b)
            #parameters: ldns_resolver *,bool,
            #retvals: 

        def set_domain(self, rd):
            """
               Set the resolver's default domain.
               This gets appended when no absolute name is given.
               
               :param rd: The name to append.
               :type rd: :class:`ldns_dname` or str
               :throws TypeError: When arguments of inappropriate types.
               :throws Exception: When `rd` a non dname rdf.

               .. note::
                   The type checking of parameter `rd` is benevolent.
                   It allows also to pass a dname :class:`ldns_rdf` object.
                   This will probably change in future.
            """
            # Also has to be able to pass None or dame string.
            if isinstance(rd, str):
                dname = _ldns.ldns_dname_new_frm_str(rd)
            elif (not isinstance(rd, ldns_dname)) and \
               isinstance(rd, ldns_rdf) and \
               rd.get_type() == _ldns.LDNS_RDF_TYPE_DNAME:
                warnings.warn("The ldns_resolver.set_domain() method" +
                    " will drop the possibility to accept ldns_rdf." +
                    " Convert argument to ldns_dname.",
                    PendingDeprecationWarning, stacklevel=2)
                dname = rd
            else:
                dname = rd
            if (not isinstance(dname, ldns_rdf)) and (dname != None):
                raise TypeError("Parameter must be derived from ldns_rdf.")
            if (isinstance(dname, ldns_rdf)) and \
               (dname.get_type() != _ldns.LDNS_RDF_TYPE_DNAME):
                raise Exception("Operands must be ldns_dname.")
            _ldns._ldns_resolver_set_domain(self, dname)
            #parameters: ldns_resolver *,ldns_rdf *,
            #retvals: 

        def set_edns_udp_size(self, s):
            """
               Set maximum udp size.
               
               :param s: The udp max size.
               :type s: uint16_t
               :throws TypeError: When arguments of inappropriate types.
            """
            _ldns.ldns_resolver_set_edns_udp_size(self,s)
            #parameters: ldns_resolver *,uint16_t,
            #retvals: 

        def set_fail(self, b):
            """
               Whether or not to fail after one failed query.
               
               :param b: True: yes fail, False: continue with next name server.
               :type b: bool
            """
            _ldns.ldns_resolver_set_fail(self, b)
            #parameters: ldns_resolver *,bool,
            #retvals: 

        def set_fallback(self, fallback):
            """
               Set whether the resolvers truncation fall-back mechanism is used
               when :meth:`query` is called.
               
               :param fallback: Whether to use the fall-back mechanism.
               :type fallback: bool
            """
            _ldns.ldns_resolver_set_fallback(self, fallback)
            #parameters: ldns_resolver *,bool,
            #retvals: 

        def set_igntc(self, b):
            """
               Whether or not to ignore the TC bit.
               
               :param b: True: yes ignore, False: don't ignore.
               :type b: bool
            """
            _ldns.ldns_resolver_set_igntc(self, b)
            #parameters: ldns_resolver *,bool,
            #retvals: 

        def set_ip6(self, i):
            """
               Whether the resolver uses ip6.
               
               :param i: 0: no pref, 1: ip4, 2: ip6
               :type i: uint8_t
               :throws TypeError: When arguments of inappropriate types.
            """
            _ldns.ldns_resolver_set_ip6(self, i)
            #parameters: ldns_resolver *,uint8_t,
            #retvals: 

        def set_nameserver_count(self, c):
            """
               Set the resolver's name server count directly.
               
               :param c: The name server count.
               :type c: size_t
               :throws TypeError: When arguments of inappropriate types.
            """
            _ldns.ldns_resolver_set_nameserver_count(self, c)
            #parameters: ldns_resolver *,size_t,
            #retvals: 

        def set_nameserver_rtt(self, pos, value):
            """
               Set round trip time for a specific name server.
               Note this currently differentiates between: unreachable and
               reachable.
               
               :param pos: The name server position.
               :type pos: size_t
               :param value: The rtt.
               :type value: size_t
               :throws TypeError: When arguments of inappropriate types.
            """
            _ldns.ldns_resolver_set_nameserver_rtt(self, pos, value)
            #parameters: ldns_resolver *,size_t,size_t,
            #retvals: 

        def set_nameservers(self, rd):
            """
               Set the resolver's name server count directly by using an
               rdf list.
               
               :param rd: The resolver addresses.
               :type rd: ldns_rdf \*\*
               :throws TypeError: When arguments of inappropriate types.
            """
            _ldns.ldns_resolver_set_nameservers(self, rd)
            #parameters: ldns_resolver *,ldns_rdf **,
            #retvals: 

        def set_port(self, p):
            """
               Set the port the resolver should use.
               
               :param p: The port number.
               :type p: uint16_t
               :throws TypeError: When arguments of inappropriate types.
            """
            _ldns.ldns_resolver_set_port(self, p)
            #parameters: ldns_resolver *,uint16_t,
            #retvals: 

        def set_random(self, b):
            """
               Should the name server list be randomized before each use.
               
               :param b: True: randomize, False: don't.
               :type b: bool
            """
            _ldns.ldns_resolver_set_random(self, b)
            #parameters: ldns_resolver *,bool,
            #retvals: 

        def set_recursive(self, b):
            """
               Set the resolver recursion.
               
               :param b: True: set to recurse, False: unset.
               :type b: bool
            """
            _ldns.ldns_resolver_set_recursive(self, b)
            #parameters: ldns_resolver *,bool,
            #retvals: 

        def set_retrans(self, re):
            """
               Set the resolver retrans time-out (in seconds).
               
               :param re: The retransmission interval in seconds.
               :type re: uint8_t
               :throws TypeError: When arguments of inappropriate types.
            """
            _ldns.ldns_resolver_set_retrans(self, re)
            #parameters: ldns_resolver *,uint8_t,
            #retvals: 

        def set_retry(self, re):
            """
               Set the resolver retry interval (in seconds).
               
               :param re: The retry interval.
               :type re: uint8_t
               :throws TypeError: When arguments of inappropriate types.
            """
            _ldns.ldns_resolver_set_retry(self,re)
            #parameters: ldns_resolver *,uint8_t,
            #retvals: 

        def set_rtt(self, rtt):
            """
               Set round trip time for all name servers.
               Note this currently differentiates between: unreachable and reachable.
               
               :param rtt: A list with the times.
               :type rtt: size \*
               :throws TypeError: When arguments of inappropriate types.
            """
            _ldns.ldns_resolver_set_rtt(self, rtt)
            #parameters: ldns_resolver *,size_t *,
            #retvals: 

        def set_timeout(self, timeout):
            """
               Set the resolver's socket time out when talking to remote hosts.
               
               :param timeout: The time-out to use.
               :param timeout: struct timeval
               :throws TypeError: When arguments of inappropriate types.
            """
            _ldns.ldns_resolver_set_timeout(self,timeout)
            #parameters: ldns_resolver *,struct timeval,
            #retvals: 

        def set_tsig_algorithm(self, tsig_algorithm):
            """
               Set the tsig algorithm.
               
               :param tsig_algorithm: The tsig algorithm.
               :param tsig_algorithm: str
               :throws TypeError: When arguments of inappropriate types.
            """
            _ldns.ldns_resolver_set_tsig_algorithm(self, tsig_algorithm)
            #parameters: ldns_resolver *,char *,
            #retvals: 

        def set_tsig_keydata(self, tsig_keydata):
            """
               Set the tsig key data.
               
               :param tsig_keydata: The key data.
               :type tsig_keydata: str
               :throws TypeError: When arguments of inappropriate types.
            """
            _ldns.ldns_resolver_set_tsig_keydata(self, tsig_keydata)
            #parameters: ldns_resolver *,char *,
            #retvals: 

        def set_tsig_keyname(self, tsig_keyname):
            """
               Set the tsig key name.
               
               :param tsig_keyname: The tsig key name.
               :type tsig_keyname: str
               :throws TypeError: When arguments of inappropriate types.
            """
            _ldns.ldns_resolver_set_tsig_keyname(self, tsig_keyname)
            #parameters: ldns_resolver *,char *,
            #retvals: 

        def set_usevc(self, b):
            """
               Whether the resolver uses a virtual circuit (TCP).
               
               :param b: True: use TCP, False: don't use TCP.
               :type b: bool
            """
            _ldns.ldns_resolver_set_usevc(self, b)
            #parameters: ldns_resolver *,bool,
            #retvals: 

        def timeout(self):
            """
               What is the time-out on socket connections.
               
               :return: (struct timeval) The time-out.
            """
            return _ldns.ldns_resolver_timeout(self)
            #parameters: const ldns_resolver *,
            #retvals: struct timeval

        def trusted_key(self, keys):
            """
               Returns true if at least one of the provided keys is a trust
               anchor.
               
               :param keys: The key set to check.
               :type keys: :class:`ldns_rr_list`
               :throws TypeError: When arguments of inappropriate types.
               :return: (:class:`ldns_rr_list`) List of trusted keys if at
                   least one of the provided keys is a configured trust anchor,
                   None else.
            """
            return _ldns._replacement_ldns_resolver_trusted_key(self, keys)
            #parameters: const ldns_resolver *,ldns_rr_list *,ldns_rr_list *,
            #retvals: bool

        def tsig_algorithm(self):
            """
               Return the tsig algorithm as used by the name server.
               
               :return: (str) The algorithm used.
            """
            return _ldns._ldns_resolver_tsig_algorithm(self)
            #parameters: const ldns_resolver *,
            #retvals: char *

        def tsig_keydata(self):
            """
               Return the tsig key data as used by the name server.
               
               :return: (str) The key data used.
            """
            return _ldns._ldns_resolver_tsig_keydata(self)
            #parameters: const ldns_resolver *,
            #retvals: char *

        def tsig_keyname(self):
            """
               Return the tsig key name as used by the name server.
               
               :return: (str) The name used.
            """
            return _ldns._ldns_resolver_tsig_keyname(self)
            #parameters: const ldns_resolver *,
            #retvals: char *

        def usevc(self):
            """
               Does the resolver use tcp or udp.
               
               :return: (bool) True: tcp, False: udp.
            """
            return _ldns.ldns_resolver_usevc(self)
            #parameters: const ldns_resolver *,
            #retvals: bool

        #
        # _LDNS_RESOLVER_METHODS
        #
  %}
} 
