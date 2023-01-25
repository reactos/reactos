/******************************************************************************
 * ldns_packet.i: LDNS packet class
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

/* Creates a temporary instance of (ldns_pkt *). */
%typemap(in,numinputs=0,noblock=1) (ldns_pkt **)
{
  ldns_pkt *$1_pkt;
  $1 = &$1_pkt;
}
          
/* Result generation, appends (ldns_pkt *) after the result. */
%typemap(argout,noblock=1) (ldns_pkt **)
{
  $result = SWIG_Python_AppendOutput($result,
    SWIG_NewPointerObj(SWIG_as_voidptr($1_pkt),
      SWIGTYPE_p_ldns_struct_pkt, SWIG_POINTER_OWN |  0 ));
}

%newobject ldns_pkt_new;
%newobject ldns_pkt_clone;
%newobject ldns_pkt_rr_list_by_type;
%newobject ldns_pkt_rr_list_by_name_and_type;
%newobject ldns_pkt_rr_list_by_name;
%newobject ldns_update_pkt_new;


%nodefaultctor ldns_struct_pkt; /* No default constructor. */
%nodefaultdtor ldns_struct_pkt; /* No default destructor. */

%rename(ldns_pkt) ldns_struct_pkt;

%newobject ldns_pkt2str;
%newobject ldns_pkt_opcode2str;
%newobject ldns_pkt_rcode2str;
%newobject ldns_pkt_algorithm2str;
%newobject ldns_pkt_cert_algorithm2str;
%newobject ldns_pkt_get_section_clone;


/* Clone data on pull. */


%newobject _ldns_pkt_additional;
%rename(__ldns_pkt_additional) ldns_pkt_additional;
%inline
%{
  ldns_rr_list * _ldns_pkt_additional(ldns_pkt *p)
  {
    return ldns_rr_list_clone(ldns_pkt_additional(p));
  }
%}

%newobject _ldns_pkt_answer;
%rename(__ldns_pkt_answer) ldns_pkt_answer;
%inline
%{
  ldns_rr_list * _ldns_pkt_answer(ldns_pkt *p)
  {
    return ldns_rr_list_clone(ldns_pkt_answer(p));
  }
%}

%newobject _ldns_pkt_answerfrom;
%rename(__ldns_pkt_answerfrom) ldns_pkt_answerfrom;
%inline
%{
  ldns_rdf * _ldns_pkt_answerfrom(ldns_pkt *p)
  {
    ldns_rdf *rdf;

    rdf = ldns_pkt_answerfrom(p);
    if (rdf != NULL) {
      rdf = ldns_rdf_clone(rdf);
    }
    return rdf;
  }
%}

%newobject _ldns_pkt_authority;
%rename(__ldns_pkt_authority) ldns_pkt_authority;
%inline
%{
  ldns_rr_list * _ldns_pkt_authority(ldns_pkt *p)
  {
    return ldns_rr_list_clone(ldns_pkt_authority(p));
  }
%}

%newobject _ldns_pkt_edns_data;
%rename(__ldns_pkt_edns_data) ldns_pkt_edns_data;
%inline
%{
  ldns_rdf * _ldns_pkt_edns_data(ldns_pkt *p)
  {
    ldns_rdf *rdf;

    rdf = ldns_pkt_edns_data(p);
    if (rdf != NULL) {
      rdf = ldns_rdf_clone(rdf);
    }
    return rdf;
  }
%}

%newobject _ldns_pkt_tsig;
%rename(__ldns_pkt_tsig) ldns_pkt_tsig;
%inline
%{
  ldns_rr * _ldns_pkt_tsig(const ldns_pkt *pkt)
  {
    return ldns_rr_clone(ldns_pkt_tsig(pkt));
  }
%}

%newobject _ldns_pkt_question;
%rename(__ldns_pkt_question) ldns_pkt_question;
%inline
%{
  ldns_rr_list * _ldns_pkt_question(ldns_pkt *p)
  {
    return ldns_rr_list_clone(ldns_pkt_question(p));
  }
%}

/* End of pull cloning. */

/* Clone data on push. */

%newobject _ldns_pkt_query_new;
%rename(__ldns_pkt_query_new) ldns_pkt_query_new;
%inline
%{
  ldns_pkt * _ldns_pkt_query_new(ldns_rdf *rr_name, ldns_rr_type rr_type,
    ldns_rr_class rr_class, uint16_t flags)
  {
    return ldns_pkt_query_new(ldns_rdf_clone(rr_name), rr_type, rr_class,
             flags);
  }
%}

%rename(__ldns_pkt_push_rr) ldns_pkt_push_rr;
%inline
%{
  bool _ldns_pkt_push_rr(ldns_pkt *p, ldns_pkt_section sec, ldns_rr *rr)
  {
    return ldns_pkt_push_rr(p, sec, ldns_rr_clone(rr));
  }
%}

%rename(__ldns_pkt_safe_push_rr) ldns_pkt_safe_push_rr;
%inline
%{
  bool _ldns_pkt_safe_push_rr(ldns_pkt *pkt, ldns_pkt_section sec,
    ldns_rr *rr)
  {
    /* Prevents memory leaks when fails. */
    ldns_rr *rr_clone = NULL;
    bool ret;

    if (rr != NULL) {
      rr_clone = ldns_rr_clone(rr);
    }
    ret = ldns_pkt_safe_push_rr(pkt, sec, rr_clone);
    if (!ret) {
      ldns_rr_free(rr_clone);
    }

    return ret;
  }
%}

%rename(__ldns_pkt_push_rr_list) ldns_pkt_push_rr_list;
%inline
%{
  bool _ldns_pkt_push_rr_list(ldns_pkt *p, ldns_pkt_section sec,
    ldns_rr_list *rrl)
  {
    return ldns_pkt_push_rr_list(p, sec, ldns_rr_list_clone(rrl));
  }
%}

%rename(__ldns_pkt_safe_push_rr_list) ldns_pkt_safe_push_rr_list;
%inline
%{
  bool _ldns_pkt_safe_push_rr_list(ldns_pkt *p, ldns_pkt_section s,
    ldns_rr_list *rrl)
  {
    /* Prevents memory leaks when fails. */
    ldns_rr_list *rrl_clone = NULL;
    bool ret;

    if (rrl != NULL) {
      rrl_clone = ldns_rr_list_clone(rrl);
    }
    ret = ldns_pkt_safe_push_rr_list(p, s, rrl_clone);
    if (!ret) {
      ldns_rr_list_free(rrl_clone);
    }

    return ret;
  }
%}

%rename(__ldns_pkt_set_additional) ldns_pkt_set_additional;
%inline
%{
  void _ldns_pkt_set_additional(ldns_pkt *p, ldns_rr_list *rrl)
  {
    ldns_rr_list *rrl_clone = NULL;
    if (rrl != NULL) {
      rrl_clone = ldns_rr_list_clone(rrl);
    }
    /* May leak memory, when overwriting pointer value. */
    ldns_pkt_set_additional(p, rrl_clone);
  }
%}

%rename(__ldns_pkt_set_answer) ldns_pkt_set_answer;
%inline
%{
  void _ldns_pkt_set_answer(ldns_pkt *p, ldns_rr_list *rrl)
  {
    ldns_rr_list *rrl_clone = NULL;
    if (rrl != NULL) {
      rrl_clone = ldns_rr_list_clone(rrl);
    }
    /* May leak memory, when overwriting pointer value. */
    ldns_pkt_set_answer(p, rrl_clone);
  }
%}

%rename (__ldns_pkt_set_answerfrom) ldns_pkt_set_answerfrom;
%inline
%{
  void _ldns_pkt_set_answerfrom(ldns_pkt *packet, ldns_rdf *rdf)
  {
    ldns_rdf *rdf_clone = NULL;
    if (rdf != NULL) {
      rdf_clone = ldns_rdf_clone(rdf);
    }
    /* May leak memory, when overwriting pointer value. */
    ldns_pkt_set_answerfrom(packet, rdf_clone);
  }
%}

%rename(__ldns_pkt_set_authority) ldns_pkt_set_authority;
%inline
%{
  void _ldns_pkt_set_authority(ldns_pkt *p, ldns_rr_list *rrl)
  {
    ldns_rr_list *rrl_clone = NULL;
    if (rrl != NULL) {
      rrl_clone = ldns_rr_list_clone(rrl);
    }
    /* May leak memory, when overwriting pointer value. */
    ldns_pkt_set_authority(p, rrl_clone);
  }
%}

%rename(__ldns_pkt_set_edns_data) ldns_pkt_set_edns_data;
%inline
%{
  void _ldns_pkt_set_edns_data(ldns_pkt *packet, ldns_rdf *rdf)
  {
    ldns_rdf *rdf_clone = NULL;
    if (rdf != NULL) {
      rdf_clone = ldns_rdf_clone(rdf);
    }
    /* May leak memory, when overwriting pointer value. */
    ldns_pkt_set_edns_data(packet, rdf_clone);
  }
%}

%rename(__ldns_pkt_set_question) ldns_pkt_set_question;
%inline
%{
  void _ldns_pkt_set_question(ldns_pkt *p, ldns_rr_list *rrl)
  {
    ldns_rr_list *rrl_clone = NULL;
    if (rrl != NULL) {
      rrl_clone = ldns_rr_list_clone(rrl);
    }
    /* May leak memory, when overwriting pointer value. */
    ldns_pkt_set_question(p, rrl_clone);
  }
%}

%rename(__ldns_pkt_set_tsig) ldns_pkt_set_tsig;
%inline
%{
  void _ldns_pkt_set_tsig(ldns_pkt *pkt, ldns_rr *rr)
  {
    ldns_rr *rr_clone = NULL;
    if (rr != NULL) {
      rr_clone = ldns_rr_clone(rr);
    }
    /* May leak memory, when overwriting pointer value. */
    ldns_pkt_set_tsig(pkt, rr_clone);
  }
%}

/* End of push cloning. */


/* ========================================================================= */
/* Debugging related code. */
/* ========================================================================= */

#ifdef LDNS_DEBUG
%rename(__ldns_pkt_free) ldns_pkt_free;
%inline
%{
  /*!
   * @brief Prints information about deallocated pkt and deallocates.
   */
  void _ldns_pkt_free (ldns_pkt* p) {
    printf("******** LDNS_PKT free 0x%lX ************\n",
      (long unsigned int) p);
    ldns_pkt_free(p);
  }
%}
#else /* !LDNS_DEBUG */
%rename(_ldns_pkt_free) ldns_pkt_free;
#endif /* LDNS_DEBUG */


/* ========================================================================= */
/* Added C code. */
/* ========================================================================= */

/* None. */


/* ========================================================================= */
/* Encapsulating Python code. */
/* ========================================================================= */

%feature("docstring") ldns_struct_pkt "LDNS packet object. 

The :class:`ldns_pkt` object contains DNS packed (either a query or an answer).
It is the complete representation of what you actually send to a name server,
and what you get back (see :class:`ldns.ldns_resolver`).

**Usage**

>>> import ldns
>>> resolver = ldns.ldns_resolver.new_frm_file(\"/etc/resolv.conf\")
>>> pkt = resolver.query(\"nic.cz\", ldns.LDNS_RR_TYPE_NS,ldns.LDNS_RR_CLASS_IN)
>>> print pkt
;; ->>HEADER<<- opcode: QUERY, rcode: NOERROR, id: 63004
;; flags: qr rd ra ; QUERY: 1, ANSWER: 3, AUTHORITY: 0, ADDITIONAL: 0 
;; QUESTION SECTION:
;; nic.cz.	IN	NS
;; ANSWER SECTION:
nic.cz.	758	IN	NS	a.ns.nic.cz.
nic.cz.	758	IN	NS	c.ns.nic.cz.
nic.cz.	758	IN	NS	e.ns.nic.cz.
;; AUTHORITY SECTION:
;; ADDITIONAL SECTION:
;; Query time: 8 msec
;; SERVER: 82.100.38.2
;; WHEN: Thu Jan 11 12:54:33 2009
;; MSG SIZE  rcvd: 75

This simple example instances a resolver in order to resolve NS for nic.cz."

%extend ldns_struct_pkt {
 
  %pythoncode
  %{
        def __init__(self):
            """
               Cannot be created directly from Python.
            """
            raise Exception("This class can't be created directly. " +
                "Please use: ldns_pkt_new, ldns_pkt_query_new " +
                "or ldns_pkt_query_new_frm_str")

        __swig_destroy__ = _ldns._ldns_pkt_free

        #
        # LDNS_PKT_CONSTRUCTORS_
        #

        @staticmethod
        def new():
            """
               Creates new empty packet structure.

               :return: (:class:`ldns_pkt` ) New empty packet.
            """
            return _ldns.ldns_pkt_new()

        @staticmethod
        def new_query(rr_name, rr_type, rr_class, flags):
            """
               Creates a packet with a query in it for the given name,
               type and class.
               
               :param rr_name: The name to query for.
               :type rr_name: :class:`ldns_dname`
               :param rr_type: The type to query for.
               :type rr_type: ldns_rr_type
               :param rr_class: The class to query for.
               :type rr_class: ldns_rr_class
               :param flags: Packet flags.
               :type flags: uint16_t
               :throws TypeError: When arguments of inappropriate types.
               :return: (:class:`ldns_pkt`) New object.

               .. note::
                   The type checking of parameter `rr_name` is benevolent.
                   It allows also to pass a dname :class:`ldns_rdf` object.
                   This will probably change in future.
            """
            if (not isinstance(rr_name, ldns_dname)) and \
               isinstance(rr_name, ldns_rdf) and \
               rr_name.get_type() == _ldns.LDNS_RDF_TYPE_DNAME:
                warnings.warn("The ldns_pkt.new_query() method will" +
                    " drop the possibility to accept ldns_rdf." +
                    " Convert argument to ldns_dname.",
                    PendingDeprecationWarning, stacklevel=2)
            if not isinstance(rr_name, ldns_rdf):
                raise TypeError("Parameter must be derived from ldns_rdf.")
            if (rr_name.get_type() != _ldns.LDNS_RDF_TYPE_DNAME):
                raise Exception("Operands must be ldns_dname.")
            return _ldns._ldns_pkt_query_new(rr_name, rr_type, rr_class, flags)

        @staticmethod
        def new_query_frm_str(rr_name, rr_type, rr_class, flags, raiseException = True):
            """
               Creates a query packet for the given name, type, class.
               
               :param rr_name: The name to query for.
               :type rr_name: str
               :param rr_type: The type to query for.
               :type rr_type: ldns_rr_type
               :param rr_class: The class to query for.
               :type rr_class: ldns_rr_class
               :param flags: Packet flags.
               :type flags: uint16_t
               :param raiseException: If True, an exception occurs in case a
                   packet object can't be created.
               :throws TypeError: When arguments of inappropriate types.
               :throws Exception: When raiseException set and packet couldn't
                   be created.
               :return: (:class:`ldns_pkt`) Query packet object or None.
                   If the object can't be created and raiseException is True,
                   an exception occurs.


               **Usage**

               >>> pkt = ldns.ldns_pkt.new_query_frm_str("test.nic.cz",ldns.LDNS_RR_TYPE_ANY, ldns.LDNS_RR_CLASS_IN, ldns.LDNS_QR | ldns.LDNS_AA)
               >>> rra = ldns.ldns_rr.new_frm_str("test.nic.cz. IN A 192.168.1.1",300)
               >>> list = ldns.ldns_rr_list()
               >>> if (rra): list.push_rr(rra)
               >>> pkt.push_rr_list(ldns.LDNS_SECTION_ANSWER, list)
               >>> print pkt
               ;; ->>HEADER<<- opcode: QUERY, rcode: NOERROR, id: 0
               ;; flags: qr aa ; QUERY: 1, ANSWER: 1, AUTHORITY: 0, ADDITIONAL: 0 
               ;; QUESTION SECTION:
               ;; test.nic.cz.	IN	ANY
               ;; ANSWER SECTION:
               test.nic.cz.	300	IN	A	192.168.1.1
               ;; AUTHORITY SECTION:
               ;; ADDITIONAL SECTION:
               ;; Query time: 0 msec
               ;; WHEN: Thu Jan  1 01:00:00 1970
               ;; MSG SIZE  rcvd: 0
            """
            status, pkt = _ldns.ldns_pkt_query_new_frm_str(rr_name, rr_type, rr_class, flags)
            if status != LDNS_STATUS_OK:
                if (raiseException): raise Exception("Can't create query packet, error: %d" % status)
                return None
            return pkt

        #
        # _LDNS_PKT_CONSTRUCTORS
        #
     
        def __str__(self):
            """
               Converts the data in the DNS packet to presentation format.

               :return: (str)
            """
            return _ldns.ldns_pkt2str(self)

        def opcode2str(self):
            """
               Converts a packet opcode to its mnemonic and returns that as an
               allocated null-terminated string.

               :return: (str)
            """
            return _ldns.ldns_pkt_opcode2str(self.get_opcode())

        def rcode2str(self):
            """
               Converts a packet rcode to its mnemonic and returns that as an
               allocated null-terminated string.

               :return: (str)
            """
            return _ldns.ldns_pkt_rcode2str(self.get_rcode())

        def print_to_file(self, output):
            """
               Prints the data in the DNS packet to the given file stream
               (in presentation format).

               :param output: Opened file to write to.
               :type output: file
               :throws TypeError: When arguments of inappropriate types.
            """
            _ldns.ldns_pkt_print(output, self)
            #parameters: FILE *,const ldns_pkt *,

        def write_to_buffer(self, buffer):
            """
               Copies the packet data to the buffer in wire format.
               
               :param buffer: Buffer to append the result to.
               :type buffer: :class:`ldns_buffer`
               :throws TypeError: When arguments of inappropriate types.
               :return: (ldns_status) ldns_status
            """
            return _ldns.ldns_pkt2buffer_wire(buffer, self)
            #parameters: ldns_buffer *,const ldns_pkt *,
            #retvals: ldns_status

        @staticmethod
        def algorithm2str(alg):
            """
               Converts a signing algorithms to its mnemonic and returns that
               as an allocated null-terminated string.

               :param alg: The algorithm to convert to text.
               :type alg: ldns_algorithm
               :return: (str)
            """
            return _ldns.ldns_pkt_algorithm2str(alg)
            #parameters: ldns_algorithm,

        @staticmethod
        def cert_algorithm2str(alg):
            """
               Converts a cert algorithm to its mnemonic and returns that as an
               allocated null-terminated string.

               :param alg: Cert algorithm to convert to text.
               :type alg: ldns_cert_algorithm
               :return: (str)
            """
            return _ldns.ldns_pkt_cert_algorithm2str(alg)
            #parameters: ldns_algorithm,

        #
        # LDNS_PKT_METHODS_
        #

        def aa(self):
            """
               Read the packet's aa bit.
               
               :return: (bool) Value of the bit.
            """
            return _ldns.ldns_pkt_aa(self)
            #parameters: const ldns_pkt *,
            #retvals: bool

        def ad(self):
            """
               Read the packet's ad bit.
               
               :return: (bool) Value of the bit.
            """
            return _ldns.ldns_pkt_ad(self)
            #parameters: const ldns_pkt *,
            #retvals: bool

        def additional(self):
            """
               Return the packet's additional section.
               
               :return: (:class:`ldns_rr_list`) The additional section.
            """
            return _ldns._ldns_pkt_additional(self)
            #parameters: const ldns_pkt *,
            #retvals: ldns_rr_list *

        def all(self):
            """
               Return the packet's question, answer, authority and additional
               sections concatenated.

               :return: (:class:`ldns_rr_list`) Concatenated sections.
            """
            return _ldns.ldns_pkt_all(self)
            #parameters: const ldns_pkt *,
            #retvals: ldns_rr_list *

        def all_noquestion(self):
            """
               Return the packet's answer, authority and additional sections
               concatenated.
               Like :meth:`all` but without the questions.

               :return: (:class:`ldns_rr_list`) Concatenated sections except
                   questions.
            """
            return _ldns.ldns_pkt_all_noquestion(self)
            #parameters: const ldns_pkt *,
            #retvals: ldns_rr_list *

        def ancount(self):
            """
               Return the packet's an count.
               
               :return: (int) The an count.
            """
            return _ldns.ldns_pkt_ancount(self)
            #parameters: const ldns_pkt *,
            #retvals: uint16_t

        def answer(self):
            """
               Return the packet's answer section.
               
               :return: (:class:`ldns_rr_list`) The answer section.
            """
            return _ldns._ldns_pkt_answer(self)
            #parameters: const ldns_pkt *,
            #retvals: ldns_rr_list *

        def answerfrom(self):
            """
               Return the packet's answerfrom.
               
               :return: (:class:`ldns_rdf`) The name of the server.
            """
            return _ldns._ldns_pkt_answerfrom(self)
            #parameters: const ldns_pkt *,
            #retvals: ldns_rdf *

        def arcount(self):
            """
               Return the packet's ar count.
               
               :return: (int) The ar count.
            """
            return _ldns.ldns_pkt_arcount(self)
            #parameters: const ldns_pkt *,
            #retvals: uint16_t

        def authority(self):
            """
               Return the packet's authority section.
               
               :return: (:class:`ldns_rr_list`) The authority section.
            """
            return _ldns._ldns_pkt_authority(self)
            #parameters: const ldns_pkt *,
            #retvals: ldns_rr_list *

        def cd(self):
            """
               Read the packet's cd bit.
               
               :return: (bool) Value of the bit.
            """
            return _ldns.ldns_pkt_cd(self)
            #parameters: const ldns_pkt *,
            #retvals: bool

        def clone(self):
            """
               Clones the packet, creating a fully allocated copy.
               
               :return: (:class:`ldns_pkt`) New packet clone.
            """
            return _ldns.ldns_pkt_clone(self)
            #parameters: ldns_pkt *,
            #retvals: ldns_pkt *

        def edns(self):
            """
               Returns True if this packet needs and EDNS rr to be sent.
               
               At the moment the only reason is an expected packet size larger
               than 512 bytes, but for instance DNSSEC would be a good reason
               too.
               
               :return: (bool) True if packet needs EDNS rr.
            """
            return _ldns.ldns_pkt_edns(self)
            #parameters: const ldns_pkt *,
            #retvals: bool

        def edns_data(self):
            """
               Return the packet's edns data.
               
               :return: (:class:`ldns_rdf`) The edns data.
            """
            return _ldns._ldns_pkt_edns_data(self)
            #parameters: const ldns_pkt *,
            #retvals: ldns_rdf *

        def edns_do(self):
            """
               Return the packet's edns do bit
               
               :return: (bool) The bit's value.
            """
            return _ldns.ldns_pkt_edns_do(self)
            #parameters: const ldns_pkt *,
            #retvals: bool

        def edns_extended_rcode(self):
            """
               Return the packet's edns extended rcode.
               
               :return: (uint8_t) The rcode.
            """
            return _ldns.ldns_pkt_edns_extended_rcode(self)
            #parameters: const ldns_pkt *,
            #retvals: uint8_t

        def edns_udp_size(self):
            """
               Return the packet's edns udp size.
               
               :return: (uint16_t) The udp size.
            """
            return _ldns.ldns_pkt_edns_udp_size(self)
            #parameters: const ldns_pkt *,
            #retvals: uint16_t

        def edns_version(self):
            """
               Return the packet's edns version.
               
               :return: (uint8_t) The edns version.
            """
            return _ldns.ldns_pkt_edns_version(self)
            #parameters: const ldns_pkt *,
            #retvals: uint8_t

        def edns_z(self):
            """
               Return the packet's edns z value.
               
               :return: (uint16_t) The z value.
            """
            return _ldns.ldns_pkt_edns_z(self)
            #parameters: const ldns_pkt *,
            #retvals: uint16_t

        def empty(self):
            """
               Check if a packet is empty.
               
               :return: (bool) True: empty, False: not empty
            """
            return _ldns.ldns_pkt_empty(self)
            #parameters: ldns_pkt *,
            #retvals: bool

        def get_opcode(self):
            """
               Read the packet's code.
               
               :return: (ldns_pkt_opcode) the opcode
            """
            return _ldns.ldns_pkt_get_opcode(self)
            #parameters: const ldns_pkt *,
            #retvals: ldns_pkt_opcode

        def get_rcode(self):
            """
               Return the packet's response code.
               
               :return: (ldns_pkt_rcode) The response code.
            """
            return _ldns.ldns_pkt_get_rcode(self)
            #parameters: const ldns_pkt *,
            #retvals: ldns_pkt_rcode

        def get_section_clone(self, s):
            """
               Return the selected rr_list's in the packet.
               
               :param s: What section(s) to return.
               :type s: ldns_pkt_section
               :throws TypeError: When arguments of inappropriate types.
               :return: (:class:`ldns_rr_list`) RR list with the rr's or None
                   if none were found.
            """
            return _ldns.ldns_pkt_get_section_clone(self, s)
            #parameters: const ldns_pkt *,ldns_pkt_section,
            #retvals: ldns_rr_list *

        def id(self):
            """
               Read the packet id.
               
               :return: (uint16_t) The packet id.
            """
            return _ldns.ldns_pkt_id(self)
            #parameters: const ldns_pkt *,
            #retvals: uint16_t

        def nscount(self):
            """
               Return the packet's ns count.
               
               :return: (uint16_t) The ns count.
            """
            return _ldns.ldns_pkt_nscount(self)
            #parameters: const ldns_pkt *,
            #retvals: uint16_t

        def push_rr(self, section, rr):
            """
               Push an rr on a packet.
               
               :param section: Where to put it.
               :type section: ldns_pkt_section
               :param rr: RR to push.
               :type rr: :class:`ldns_rr`
               :throws TypeError: When arguments of inappropriate types.
               :return: (bool) A boolean which is True when the rr was added.
            """
            return _ldns._ldns_pkt_push_rr(self,section,rr)
            #parameters: ldns_pkt *,ldns_pkt_section,ldns_rr *,
            #retvals: bool

        def push_rr_list(self, section, list):
            """
               Push a rr_list on a packet.
               
               :param section: Where to put it.
               :type section: ldns_pkt_section
               :param list: The rr_list to push.
               :type list: :class:`ldns_rr_list`
               :throws TypeError: When arguments of inappropriate types.
               :return: (bool) A boolean which is True when the rr was added.
            """
            return _ldns._ldns_pkt_push_rr_list(self,section,list)
            #parameters: ldns_pkt *,ldns_pkt_section,ldns_rr_list *,
            #retvals: bool

        def qdcount(self):
            """
               Return the packet's qd count.
               
               :return: (uint16_t) The qd count.
            """
            return _ldns.ldns_pkt_qdcount(self)
            #parameters: const ldns_pkt *,
            #retvals: uint16_t

        def qr(self):
            """
               Read the packet's qr bit.
               
               :return: (bool) value of the bit
            """
            return _ldns.ldns_pkt_qr(self)
            #parameters: const ldns_pkt *,
            #retvals: bool

        def querytime(self):
            """
               Return the packet's query time.
               
               :return: (uint32_t) The query time.
            """
            return _ldns.ldns_pkt_querytime(self)
            #parameters: const ldns_pkt *,
            #retvals: uint32_t

        def question(self):
            """
               Return the packet's question section.
               
               :return: (:class:`ldns_rr_list`) The question section.
            """
            return _ldns._ldns_pkt_question(self)
            #parameters: const ldns_pkt *,
            #retvals: ldns_rr_list *

        def ra(self):
            """
               Read the packet's ra bit.
               
               :return: (bool) Value of the bit.
            """
            return _ldns.ldns_pkt_ra(self)
            #parameters: const ldns_pkt *,
            #retvals: bool

        def rd(self):
            """
               Read the packet's rd bit.
               
               :return: (bool) Value of the bit.
            """
            return _ldns.ldns_pkt_rd(self)
            #parameters: const ldns_pkt *,
            #retvals: bool

        def reply_type(self):
            """
               Looks inside the packet to determine what kind of packet it is,
               AUTH, NXDOMAIN, REFERRAL, etc.
               
               :return: (ldns_pkt_type) The type of packet.
            """
            return _ldns.ldns_pkt_reply_type(self)
            #parameters: ldns_pkt *,
            #retvals: ldns_pkt_type

        def rr(self, sec, rr):
            """
               Check to see if an rr exist in the packet.
               
               :param sec: In which section to look.
               :type sec: ldns_pkt_section
               :param rr: The rr to look for.
               :type rr: :class:`ldns_rr`
               :throws TypeError: When arguments of inappropriate types.
               :return: (bool) Return True is exists.
            """
            return _ldns.ldns_pkt_rr(self, sec, rr)
            #parameters: ldns_pkt *,ldns_pkt_section,ldns_rr *,
            #retvals: bool

        def rr_list_by_name(self, r, s):
            """
               Return all the rr with a specific name from a packet.
               
               :param r: The name.
               :type r: :class:`ldns_rdf`
               :param s: The packet's section.
               :type s: ldns_pkt_section
               :throws TypeError: When arguments of inappropriate types.
               :return: (:class:`ldns_rr_list`) A list with the rr's or None
                   if none were found.
            """
            return _ldns.ldns_pkt_rr_list_by_name(self,r,s)
            #parameters: ldns_pkt *,ldns_rdf *,ldns_pkt_section,
            #retvals: ldns_rr_list *

        def rr_list_by_name_and_type(self, ownername, atype, sec):
            """
               Return all the rr with a specific type and type from a packet.
               
               :param ownername: The name.
               :type ownername: :class:`ldns_rdf`
               :param atype: The type.
               :type atype: ldns_rr_type
               :param sec: The packet's section.
               :type sec: ldns_pkt_section
               :throws TypeError: When arguments of inappropriate types.
               :return: (:class:`ldns_rr_list`) A list with the rr's or None
                   if none were found.
            """
            return _ldns.ldns_pkt_rr_list_by_name_and_type(self, ownername, atype, sec)
            #parameters: const ldns_pkt *,const ldns_rdf *,ldns_rr_type,ldns_pkt_section,
            #retvals: ldns_rr_list *

        def rr_list_by_type(self, t, s):
            """
               Return all the rr with a specific type from a packet.
               
               :param t: The type.
               :type t: ldns_rr_type
               :param s: The packet's section.
               :type s: ldns_pkt_section
               :throws TypeError: When arguments of inappropriate types.
               :return: (:class:`ldns_rr_list`) A list with the rr's or None
                   if none were found.
            """
            return _ldns.ldns_pkt_rr_list_by_type(self, t, s)
            #parameters: const ldns_pkt *,ldns_rr_type,ldns_pkt_section,
            #retvals: ldns_rr_list *

        def safe_push_rr(self, sec, rr):
            """
               Push an rr on a packet, provided the RR is not there.
               
               :param sec: Where to put it.
               :type sec: ldns_pkt_section
               :param rr: RR to push.
               :type rr: :class:`ldns_rr`
               :throws TypeError: When arguments of inappropriate types.
               :return: (bool) A boolean which is True when the rr was added.
            """
            return _ldns._ldns_pkt_safe_push_rr(self,sec,rr)
            #parameters: ldns_pkt *,ldns_pkt_section,ldns_rr *,
            #retvals: bool

        def safe_push_rr_list(self, sec, list):
            """
               Push an rr_list to a packet, provided the RRs are not already
               there.
               
               :param sec: Where to put it.
               :type sec: ldns_pkt_section
               :param list: The rr_list to push.
               :type list: :class:`ldns_rr_list`
               :throws TypeError: When arguments of inappropriate types.
               :return: (bool) A boolean which is True when the list was added.
            """
            return _ldns._ldns_pkt_safe_push_rr_list(self, sec, list)
            #parameters: ldns_pkt *,ldns_pkt_section,ldns_rr_list *,
            #retvals: bool

        def set_aa(self, b):
            """
               Set the packet's aa bit.
               
               :param b: The value to set.
               :type b: bool
            """
            _ldns.ldns_pkt_set_aa(self, b)
            #parameters: ldns_pkt *,bool,
            #retvals: 

        def set_ad(self, b):
            """
               Set the packet's ad bit.
               
               :param b: The value to set.
               :type b: bool
            """
            _ldns.ldns_pkt_set_ad(self, b)
            #parameters: ldns_pkt *,bool,
            #retvals: 

        def set_additional(self, rr):
            """
               Directly set the additional section.
               
               :param rr: The rr list to set.
               :type rr: :class:`ldns_rr_list`
               :throws TypeError: When arguments of inappropriate types.
            """
            _ldns._ldns_pkt_set_additional(self, rr)
            #parameters: ldns_pkt *,ldns_rr_list *,
            #retvals: 

        def set_ancount(self, c):
            """
               Set the packet's an count.
               
               :param c: The count.
               :type c: int
               :throws TypeError: When arguments of inappropriate types.
            """
            _ldns.ldns_pkt_set_ancount(self, c)
            #parameters: ldns_pkt *,uint16_t,
            #retvals: 

        def set_answer(self, rr):
            """
               Directly set the answer section.
               
               :param rr: The rr list to set.
               :type rr: :class:`ldns_rr_list`
               :throws TypeError: When arguments of inappropriate types.
            """
            _ldns._ldns_pkt_set_answer(self, rr)
            #parameters: ldns_pkt *,ldns_rr_list *,
            #retvals: 

        def set_answerfrom(self, r):
            """
               Set the packet's answering server.
               
               :param r: The address.
               :type r: :class:`ldns_rdf`
               :throws TypeError: When arguments of inappropriate types.
            """
            _ldns._ldns_pkt_set_answerfrom(self, r)
            #parameters: ldns_pkt *,ldns_rdf *,
            #retvals: 

        def set_arcount(self, c):
            """
               Set the packet's arcount.
               
               :param c: The count.
               :type c: int
               :throws TypeError: When arguments of inappropriate types.
            """
            _ldns.ldns_pkt_set_arcount(self,c)
            #parameters: ldns_pkt *,uint16_t,
            #retvals: 

        def set_authority(self, rr):
            """
               Directly set the authority section.
               
               :param rr: The rr list to set.
               :type rr: :class:`ldns_rr_list`
               :throws TypeError: When arguments of inappropriate types.
            """
            _ldns._ldns_pkt_set_authority(self, rr)
            #parameters: ldns_pkt *,ldns_rr_list *,
            #retvals: 

        def set_cd(self, b):
            """
               Set the packet's cd bit.
               
               :param b: The value to set.
               :type b: bool
            """
            _ldns.ldns_pkt_set_cd(self, b)
            #parameters: ldns_pkt *,bool,
            #retvals: 

        def set_edns_data(self, data):
            """
               Set the packet's edns data.
               
               :param data: The data.
               :type data: :class:`ldns_rdf`
               :throws TypeError: When arguments of inappropriate types.
            """
            _ldns._ldns_pkt_set_edns_data(self, data)
            #parameters: ldns_pkt *,ldns_rdf *,
            #retvals: 

        def set_edns_do(self, value):
            """
               Set the packet's edns do bit.
               
               :param value: The bit's new value.
               :type value: bool
            """
            _ldns.ldns_pkt_set_edns_do(self, value)
            #parameters: ldns_pkt *,bool,
            #retvals: 

        def set_edns_extended_rcode(self, c):
            """
               Set the packet's edns extended rcode.
               
               :param c: The code.
               :type c: uint8_t
               :throws TypeError: When arguments of inappropriate types.
            """
            _ldns.ldns_pkt_set_edns_extended_rcode(self, c)
            #parameters: ldns_pkt *,uint8_t,
            #retvals: 

        def set_edns_udp_size(self, s):
            """
               Set the packet's edns udp size.
               
               :param s: The size.
               :type s: uint16_t
               :throws TypeError: When arguments of inappropriate types.
            """
            _ldns.ldns_pkt_set_edns_udp_size(self, s)
            #parameters: ldns_pkt *,uint16_t,
            #retvals: 

        def set_edns_version(self, v):
            """
               Set the packet's edns version.
               
               :param v: The version.
               :type v: uint8_t
               :throws TypeError: When arguments of inappropriate types.
            """
            _ldns.ldns_pkt_set_edns_version(self, v)
            #parameters: ldns_pkt *,uint8_t,
            #retvals: 

        def set_edns_z(self, z):
            """
               Set the packet's edns z value.
               
               :param z: The value.
               :type z: uint16_t
               :throws TypeError: When arguments of inappropriate types.
            """
            _ldns.ldns_pkt_set_edns_z(self, z)
            #parameters: ldns_pkt *,uint16_t,
            #retvals: 

        def set_flags(self, flags):
            """
               Sets the flags in a packet.
               
               :param flags: ORed values: LDNS_QR| LDNS_AR for instance.
               :type flags: int
               :throws TypeError: When arguments of inappropriate types.
               :return: (bool) True on success, False otherwise.
            """
            return _ldns.ldns_pkt_set_flags(self, flags)
            #parameters: ldns_pkt *,uint16_t,
            #retvals: bool

        def set_id(self, id):
            """
               Set the packet's id.
               
               :param id: The id to set.
               :type id: uint16_t
               :throws TypeError: When arguments of inappropriate types.
            """
            _ldns.ldns_pkt_set_id(self, id)
            #parameters: ldns_pkt *,uint16_t,
            #retvals: 

        def set_nscount(self, c):
            """
               Set the packet's ns count.
               
               :param c: The count.
               :type c: int
               :throws TypeError: When arguments of inappropriate types.
            """
            _ldns.ldns_pkt_set_nscount(self, c)
            #parameters: ldns_pkt *,uint16_t,
            #retvals: 

        def set_opcode(self, c):
            """
               Set the packet's opcode.
               
               :param c: The opcode.
               :type c: ldns_pkt_opcode
               :throws TypeError: When arguments of inappropriate types.
            """
            _ldns.ldns_pkt_set_opcode(self, c)
            #parameters: ldns_pkt *,ldns_pkt_opcode,
            #retvals: 

        def set_qdcount(self, c):
            """
               Set the packet's qd count.
               
               :param c: The count.
               :type c: int
               :throws TypeError: When arguments of inappropriate types.
            """
            _ldns.ldns_pkt_set_qdcount(self, c)
            #parameters: ldns_pkt *,uint16_t,
            #retvals: 

        def set_qr(self, b):
            """
               Set the packet's qr bit.
               
               :param b: The value to set.
               :type b: bool
            """
            _ldns.ldns_pkt_set_qr(self, b)
            #parameters: ldns_pkt *,bool,
            #retvals: 

        def set_querytime(self, t):
            """
               Set the packet's query time.
               
               :param t: The query time in msec.
               :type t: uint32_t
               :throws TypeError: When arguments of inappropriate types.
            """
            _ldns.ldns_pkt_set_querytime(self, t)
            #parameters: ldns_pkt *,uint32_t,
            #retvals: 

        def set_question(self, rr):
            """
               Directly set the question section.
               
               :param rr: The rr list to set.
               :type rr: :class:`ldns_rr_list`
               :throws TypeError: When arguments of inappropriate types.
            """
            _ldns._ldns_pkt_set_question(self, rr)
            #parameters: ldns_pkt *,ldns_rr_list *,
            #retvals: 

        def set_ra(self, b):
            """
               Set the packet's ra bit.
               
               :param b: The value to set.
               :type b: bool
            """
            _ldns.ldns_pkt_set_ra(self, b)
            #parameters: ldns_pkt *,bool,
            #retvals: 

        def set_random_id(self):
            """
               Set the packet's id to a random value.
            """
            _ldns.ldns_pkt_set_random_id(self)
            #parameters: ldns_pkt *,
            #retvals: 

        def set_rcode(self, c):
            """
               Set the packet's response code.
               
               :param c: The rcode.
               :type c: uint8_t
               :throws TypeError: When arguments of inappropriate types.
            """
            _ldns.ldns_pkt_set_rcode(self, c)
            #parameters: ldns_pkt *,uint8_t,
            #retvals: 

        def set_rd(self, b):
            """
               Set the packet's rd bit.
               
               :param b: The value to set.
               :type b: bool
            """
            _ldns.ldns_pkt_set_rd(self, b)
            #parameters: ldns_pkt *,bool,
            #retvals: 

        def set_section_count(self, s, x):
            """
               Set a packet's section count to x.
               
               :param s: The section.
               :type s: ldns_pkt_section
               :param x: The section count.
               :type x: uint16_t
               :throws TypeError: When arguments of inappropriate types.
            """
            _ldns.ldns_pkt_set_section_count(self, s, x)
            #parameters: ldns_pkt *,ldns_pkt_section,uint16_t,
            #retvals: 

        def set_size(self, s):
            """
               Set the packet's size.
               
               :param s: The size.
               :type s: int
               :throws TypeError: When arguments of inappropriate types.
            """
            _ldns.ldns_pkt_set_size(self,s)
            #parameters: ldns_pkt *,size_t,
            #retvals: 

        def set_tc(self, b):
            """
               Set the packet's tc bit.
               
               :param b: The value to set.
               :type b: bool
            """
            _ldns.ldns_pkt_set_tc(self, b)
            #parameters: ldns_pkt *,bool,
            #retvals: 

        def set_timestamp(self, timeval):
            """
               Set the packet's time stamp.

               :param timestamp: The time stamp.
               :type timestamp: struct timeval
               :throws TypeError: When arguments of inappropriate types.
            """
            _ldns.ldns_pkt_set_timestamp(self, timeval)
            #parameters: ldns_pkt *,struct timeval,
            #retvals: 

        def set_tsig(self, t):
            """
               Set the packet's tsig rr.
               
               :param t: The tsig rr.
               :type t: :class:`ldns_rr`
               :throws TypeError: When arguments of inappropriate types.
            """
            _ldns._ldns_pkt_set_tsig(self, t)
            #parameters: ldns_pkt *,ldns_rr *,
            #retvals: 

        def size(self):
            """
               Return the packet's size in bytes.
               
               :return: (size_t) The size.
            """
            return _ldns.ldns_pkt_size(self)
            #parameters: const ldns_pkt *,
            #retvals: size_t

        def tc(self):
            """
               Read the packet's tc bit.
               
               :return: (bool) Value of the bit.
            """
            return _ldns.ldns_pkt_tc(self)
            #parameters: const ldns_pkt *,
            #retvals: bool

        def timestamp(self):
            """
               Return the packet's time stamp.
               
               :return: (struct timeval) The time stamp.
            """
            return _ldns.ldns_pkt_timestamp(self)
            #parameters: const ldns_pkt *,
            #retvals: struct timeval

        def tsig(self):
            """
               Return the packet's tsig pseudo rr's.
               
               :return: (:class:`ldns_rr`) The tsig rr.
            """
            return _ldns._ldns_pkt_tsig(self)
            #parameters: const ldns_pkt *,
            #retvals: ldns_rr *

        #
        # _LDNS_PKT_METHODS#
        #

        #
        # LDNS update methods
        #

        #
        # LDNS_METHODS_
        #

        def update_ad(self):
            """
               Get the ad count.

               :return: (uint16_t) The ad count.
            """
            return _ldns.ldns_update_ad(self)
            #parameters: ldns_pkt *
            #retvals: uint16_t

        def update_pkt_tsig_add(self, r):
            """
               Add tsig credentials to a packet from a resolver.
               
               :param r: Resolver to copy from.
               :type r: :class:`ldns_resolver`
               :throws TypeError: When arguments of inappropriate types.
               :return: (ldns_status) Status whether successful or not.
            """
            return _ldns.ldns_update_pkt_tsig_add(self, r)
            #parameters: ldns_pkt *,ldns_resolver *,
            #retvals: ldns_status

        def update_prcount(self):
            """
               Get the pr count.
               
               :return: (uint16_t) The pr count.
            """
            return _ldns.ldns_update_prcount(self)
            #parameters: const ldns_pkt *,
            #retvals: uint16_t

        def update_set_adcount(self, c):
            """
               Set the ad count.
               
               :param c: The ad count to set.
               :type c: uint16_t
               :throws TypeError: When arguments of inappropriate types.
            """
            _ldns.ldns_update_set_adcount(self, c)
            #parameters: ldns_pkt *,uint16_t,
            #retvals: 

        def update_set_prcount(self, c):
            """
               Set the pr count.
               
               :param c: The pr count to set.
               :type c: uint16_t
               :throws TypeError: When arguments of inappropriate types.
            """
            _ldns.ldns_update_set_prcount(self, c)
            #parameters: ldns_pkt *,uint16_t,
            #retvals: 

        def update_set_upcount(self, c):
            """
               Set the up count.
               
               :param c: The up count to set.
               :type c: uint16_t
               :throws TypeError: When arguments of inappropriate types.
            """
            _ldns.ldns_update_set_upcount(self,c)
            #parameters: ldns_pkt *,uint16_t,
            #retvals: 

        def update_set_zo(self, c):
            """
               Set the zo count.

               :param c: The zo count to set.
               :type c: uint16_t
               :throws TypeError: When arguments of inappropriate types.
            """
            _ldns.ldns_update_set_zo(self, c)
            #parameters: ldns_pkt *,uint16_t,
            #retvals: 

        def update_upcount(self):
            """
               Get the up count.
               
               :return: (uint16_t) The up count.
            """
            return _ldns.ldns_update_upcount(self)
            #parameters: const ldns_pkt *,
            #retvals: uint16_t

        def update_zocount(self):
            """
               Get the zo count.
               
               :return: (uint16_t) The zo count.
            """
            return _ldns.ldns_update_zocount(self)
            #parameters: const ldns_pkt *,
            #retvals: uint16_t

        #
        # _LDNS_METHODS
        #
  %}
} 
