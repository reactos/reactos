/******************************************************************************
 * ldns_dnssec.i: DNSSEC zone, name, rrs
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
 ******************************************************************************/
%nodefaultctor ldns_dnssec_rrs; //no default constructor & destructor
%nodefaultdtor ldns_dnssec_rrs;

%newobject ldns_dnssec_rrs_new;
%delobject ldns_dnssec_rrs_free;

%extend ldns_dnssec_rrs {
  %pythoncode %{

        def __init__(self):
            """Creates a new entry for 1 pointer to an rr and 1 pointer to the next rrs.
               
               :returns: (ldns_dnssec_rrs) the allocated data
            """
            self.this = _ldns.ldns_dnssec_rrs_new()
            if not self.this:
                raise Exception("Can't create rrs instance")

        __swig_destroy__ = _ldns.ldns_dnssec_rrs_free

        #LDNS_DNSSEC_RRS_METHODS_#
        def add_rr(self,rr):
            """Adds an RR to the list of RRs.
               
               The list will remain ordered
               
               :param rr:
                   the RR to add
               :returns: (ldns_status) LDNS_STATUS_OK on success
            """
            return _ldns.ldns_dnssec_rrs_add_rr(self,rr)
            #parameters: ldns_dnssec_rrs *,ldns_rr *,
            #retvals: ldns_status
        #_LDNS_DNSSEC_RRS_METHODS#
 %}
}

// ================================================================================
// DNNSEC RRS
// ================================================================================
%nodefaultctor ldns_dnssec_rrsets; //no default constructor & destructor
%nodefaultdtor ldns_dnssec_rrsets;

%newobject ldns_dnssec_rrsets_new;
%delobject ldns_dnssec_rrsets_free;

%extend ldns_dnssec_rrsets {
  %pythoncode %{
        def __init__(self):
            """Creates a new list (entry) of RRsets.
               
               :returns: (ldns_dnssec_rrsets \*) instance
            """
            self.this = _ldns.ldns_dnssec_rrsets_new()
            if not self.this:
                raise Exception("Can't create rrsets instance")

        __swig_destroy__ = _ldns.ldns_dnssec_rrsets_free

        def print_to_file(self, file, follow):
            """Print the given list of rrsets to the given file descriptor.

               :param file: file pointer
               :param follow: if set to false, only print the first RRset
            """
            _ldns.ldns_dnssec_rrsets_print(file,self,follow)
            #parameters: FILE *,ldns_dnssec_rrsets *,bool,
            #retvals: 

        #LDNS_DNSSEC_RRSETS_METHODS_#
        def add_rr(self,rr):
            """Add an ldns_rr to the corresponding RRset in the given list of RRsets.
               
               If it is not present, add it as a new RRset with 1 record.
               
               :param rr:
                   the rr to add to the list of rrsets
               :returns: (ldns_status) LDNS_STATUS_OK on success
            """
            return _ldns.ldns_dnssec_rrsets_add_rr(self,rr)
            #parameters: ldns_dnssec_rrsets *,ldns_rr *,
            #retvals: ldns_status

        def set_type(self,atype):
            """Sets the RR type of the rrset (that is head of the given list).
               
               :param atype:
               :returns: (ldns_status) LDNS_STATUS_OK on success
            """
            return _ldns.ldns_dnssec_rrsets_set_type(self,atype)
            #parameters: ldns_dnssec_rrsets *,ldns_rr_type,
            #retvals: ldns_status

        def type(self):
            """Returns the rr type of the rrset (that is head of the given list).
               
               :returns: (ldns_rr_type) the rr type
            """
            return _ldns.ldns_dnssec_rrsets_type(self)
            #parameters: ldns_dnssec_rrsets *,
            #retvals: ldns_rr_type
        #_LDNS_DNSSEC_RRSETS_METHODS#
 %}
}

// ================================================================================
// DNNSEC NAME
// ================================================================================
%nodefaultctor ldns_dnssec_name; //no default constructor & destructor
%nodefaultdtor ldns_dnssec_name;

%newobject ldns_dnssec_name_new;
%delobject ldns_dnssec_name_free;

%extend ldns_dnssec_name {
  %pythoncode %{
        def __init__(self):
            """Create a new instance of dnssec name."""
            self.this = _ldns.ldns_dnssec_name_new()
            if not self.this:
               raise Exception("Can't create dnssec name instance")

        __swig_destroy__ = _ldns.ldns_dnssec_name_free

        def print_to_file(self,file):
            """Prints the RRs in the dnssec name structure to the given file descriptor.

               :param file: file pointer
            """
            _ldns.ldns_dnssec_name_print(file, self)
            #parameters: FILE *,ldns_dnssec_name *,

        @staticmethod
        def new_frm_rr(raiseException=True):
            """Create a new instance of dnssec name for the given RR.
               
               :returns: (ldns_dnssec_name) instance
            """
            name = _ldns.ldns_dnssec_name_new_frm_rr(self)
            if (not name) and (raiseException): 
               raise Exception("Can't create dnssec name")
            return name

        #LDNS_DNSSEC_NAME_METHODS_#
        def add_rr(self,rr):
            """Inserts the given rr at the right place in the current dnssec_name No checking is done whether the name matches.
               
               :param rr:
                   The RR to add
               :returns: (ldns_status) LDNS_STATUS_OK on success, error code otherwise
            """
            return _ldns.ldns_dnssec_name_add_rr(self,rr)
            #parameters: ldns_dnssec_name *,ldns_rr *,
            #retvals: ldns_status

        def find_rrset(self,atype):
            """Find the RRset with the given type in within this name structure.
               
               :param atype:
               :returns: (ldns_dnssec_rrsets \*) the RRset, or NULL if not present
            """
            return _ldns.ldns_dnssec_name_find_rrset(self,atype)
            #parameters: ldns_dnssec_name *,ldns_rr_type,
            #retvals: ldns_dnssec_rrsets *

        def name(self):
            """Returns the domain name of the given dnssec_name structure.
               
               :returns: (ldns_rdf \*) the domain name
            """
            return _ldns.ldns_dnssec_name_name(self)
            #parameters: ldns_dnssec_name *,
            #retvals: ldns_rdf *

        def set_name(self,dname):
            """Sets the domain name of the given dnssec_name structure.
               
               :param dname:
                   the domain name to set it to. This data is *not* copied.
            """
            _ldns.ldns_dnssec_name_set_name(self,dname)
            #parameters: ldns_dnssec_name *,ldns_rdf *,
            #retvals: 

        def set_nsec(self,nsec):
            """Sets the NSEC(3) RR of the given dnssec_name structure.
               
               :param nsec:
                   the nsec rr to set it to. This data is *not* copied.
            """
            _ldns.ldns_dnssec_name_set_nsec(self,nsec)
            #parameters: ldns_dnssec_name *,ldns_rr *,
            #retvals: 
        #_LDNS_DNSSEC_NAME_METHODS#
 %}
}

// ================================================================================
// DNNSEC ZONE
// ================================================================================
%nodefaultctor ldns_dnssec_zone; //no default constructor & destructor
%nodefaultdtor ldns_dnssec_zone;

%newobject ldns_dnssec_zone_new;
%delobject ldns_dnssec_zone_free;

%inline %{
ldns_status ldns_dnssec_zone_sign_defcb(ldns_dnssec_zone *zone, ldns_rr_list *new_rrs, ldns_key_list *key_list, int cbtype)
{
 if (cbtype == 0) 
    return ldns_dnssec_zone_sign(zone, new_rrs, key_list, ldns_dnssec_default_add_to_signatures, NULL);
 if (cbtype == 1) 
    return ldns_dnssec_zone_sign(zone, new_rrs, key_list, ldns_dnssec_default_leave_signatures, NULL);
 if (cbtype == 2) 
    return ldns_dnssec_zone_sign(zone, new_rrs, key_list, ldns_dnssec_default_delete_signatures, NULL);

 return ldns_dnssec_zone_sign(zone, new_rrs, key_list, ldns_dnssec_default_replace_signatures, NULL);
}

ldns_status ldns_dnssec_zone_add_rr_(ldns_dnssec_zone *zone, ldns_rr *rr)
{
  ldns_rr *new_rr;
  ldns_status status;

  new_rr = ldns_rr_clone(rr);

  /*
   * A clone of the RR is created to be stored in the DNSSEC zone.
   * The Python engine frees a RR object as soon it's reference count
   * reaches zero. The code must avoid double freeing or accessing of freed
   * memory.
   */

  status = ldns_dnssec_zone_add_rr(zone, new_rr);

  if (status != LDNS_STATUS_OK) {
    ldns_rr_free(new_rr);
  }

  return status;
}
%}

%extend ldns_dnssec_zone {
  %pythoncode %{

        def __init__(self):
            """Creates a new dnssec_zone instance"""
            self.this = _ldns.ldns_dnssec_zone_new()
            if not self.this:
               raise Exception("Can't create dnssec zone instance")

        __swig_destroy__ = _ldns.ldns_dnssec_zone_free

        def print_to_file(self,file):
            """Prints the complete zone to the given file descriptor.
               
               :param file: file pointer
            """
            _ldns.ldns_dnssec_zone_print(file, self)
            #parameters: FILE *, ldns_dnssec_zone *,
            #retvals: 

        def create_nsec3s(self,new_rrs,algorithm,flags,iterations,salt_length,salt):
            """Adds NSEC3 records to the zone.
               
               :param new_rrs:
               :param algorithm:
               :param flags:
               :param iterations:
               :param salt_length:
               :param salt:
               :returns: (ldns_status) 
            """
            return _ldns.ldns_dnssec_zone_create_nsec3s(self,new_rrs,algorithm,flags,iterations,salt_length,salt)
            #parameters: ldns_dnssec_zone *,ldns_rr_list *,uint8_t,uint8_t,uint16_t,uint8_t,uint8_t *,
            #retvals: ldns_status

        def create_nsecs(self,new_rrs):
            """Adds NSEC records to the given dnssec_zone.
               
               :param new_rrs:
                   ldns_rr's created by this function are added to this rr list, so the caller can free them later
               :returns: (ldns_status) LDNS_STATUS_OK on success, an error code otherwise
            """
            return _ldns.ldns_dnssec_zone_create_nsecs(self,new_rrs)
            #parameters: ldns_dnssec_zone *,ldns_rr_list *,
            #retvals: ldns_status

        def create_rrsigs(self,new_rrs,key_list,func,arg):
            """Adds signatures to the zone.
               
               :param new_rrs:
                   the RRSIG RRs that are created are also added to this list, so the caller can free them later
               :param key_list:
                   list of keys to sign with.
               :param func:
                   Callback function to decide what keys to use and what to do with old signatures
               :param arg:
                   Optional argument for the callback function
               :returns: (ldns_status) LDNS_STATUS_OK on success, error otherwise
            """
            return _ldns.ldns_dnssec_zone_create_rrsigs(self,new_rrs,key_list,func,arg)
            #parameters: ldns_dnssec_zone *,ldns_rr_list *,ldns_key_list *,int(*)(ldns_rr *, void *),void *,
            #retvals: ldns_status

        def sign_cb(self,new_rrs,key_list,func,arg):
            """signs the given zone with the given keys (with callback function)
               
               :param new_rrs:
                   newly created resource records are added to this list, to free them later
               :param key_list:
                   the list of keys to sign the zone with
               :param func:
                   callback function that decides what to do with old signatures.
                   This function takes an ldns_rr and an optional arg argument, and returns one of four values: 

                     * LDNS_SIGNATURE_LEAVE_ADD_NEW - leave the signature and add a new one for the corresponding key 

                     * LDNS_SIGNATURE_REMOVE_ADD_NEW - remove the signature and replace is with a new one from the same key 

                     * LDNS_SIGNATURE_LEAVE_NO_ADD - leave the signature and do not add a new one with the corresponding key 

                     * LDNS_SIGNATURE_REMOVE_NO_ADD - remove the signature and do not replace

               :param arg:
                   optional argument for the callback function
               :returns: (ldns_status) LDNS_STATUS_OK on success, an error code otherwise
            """
            return _ldns.ldns_dnssec_zone_sign(self,new_rrs,key_list,func,arg)
            #parameters: ldns_dnssec_zone *,ldns_rr_list *,ldns_key_list *,int(*)(ldns_rr *, void *),void *,
            #retvals: ldns_status

        def sign(self,new_rrs,key_list, cbtype=3):
            """signs the given zone with the given keys
               
               :param new_rrs:
                   newly created resource records are added to this list, to free them later
               :param key_list:
                   the list of keys to sign the zone with
               :param cb_type: 
                   specifies how to deal with old signatures, possible values:

                     *  0 - ldns_dnssec_default_add_to_signatures, 

                     *  1 - ldns_dnssec_default_leave_signatures,

                     *  2 - ldns_dnssec_default_delete_signatures,

                     *  3 - ldns_dnssec_default_replace_signatures 

               :returns: (ldns_status) LDNS_STATUS_OK on success, an error code otherwise
            """
            return _ldns.ldns_dnssec_zone_sign_defcb(self,new_rrs,key_list, cbtype)
            #parameters: ldns_dnssec_zone *,ldns_rr_list *,ldns_key_list *,
            #retvals: ldns_status

        def sign_nsec3(self,new_rrs,key_list,func,arg,algorithm,flags,iterations,salt_length,salt):
            """signs the given zone with the given new zone, with NSEC3
               
               :param new_rrs:
                   newly created resource records are added to this list, to free them later
               :param key_list:
                   the list of keys to sign the zone with
               :param func:
                   callback function that decides what to do with old signatures
               :param arg:
                   optional argument for the callback function
               :param algorithm:
                   the NSEC3 hashing algorithm to use
               :param flags:
                   NSEC3 flags
               :param iterations:
                   the number of NSEC3 hash iterations to use
               :param salt_length:
                   the length (in octets) of the NSEC3 salt
               :param salt:
                   the NSEC3 salt data
               :returns: (ldns_status) LDNS_STATUS_OK on success, an error code otherwise
            """
            return _ldns.ldns_dnssec_zone_sign_nsec3(self,new_rrs,key_list,func,arg,algorithm,flags,iterations,salt_length,salt)
            #parameters: ldns_dnssec_zone *,ldns_rr_list *,ldns_key_list *,int(*)(ldns_rr *, void *),void *,uint8_t,uint8_t,uint16_t,uint8_t,uint8_t *,
            #retvals: ldns_status

        #LDNS_DNSSEC_ZONE_METHODS_#
        def add_empty_nonterminals(self):
            """Adds explicit dnssec_name structures for the empty nonterminals in this zone.
               
               (this is needed for NSEC3 generation)
               
               :returns: (ldns_status) 
            """
            return _ldns.ldns_dnssec_zone_add_empty_nonterminals(self)
            #parameters: ldns_dnssec_zone *,
            #retvals: ldns_status

        def add_rr(self,rr):
            """Adds the given RR to the zone.
               
               It find whether there is a dnssec_name with that name present. 
               If so, add it to that, if not create a new one.  
               Special handling of NSEC and RRSIG provided.
               
               :param rr:
                   The RR to add
               :returns: (ldns_status) LDNS_STATUS_OK on success, an error code otherwise
            """
            return _ldns.ldns_dnssec_zone_add_rr_(self,rr)
            #parameters: ldns_dnssec_zone *,ldns_rr *,
            #retvals: ldns_status

        def find_rrset(self,dname,atype):
            """Find the RRset with the given name and type in the zone.
               
               :param dname:
                   the domain name of the RRset to find
               :param atype:
               :returns: (ldns_dnssec_rrsets \*) the RRset, or NULL if not present
            """
            return _ldns.ldns_dnssec_zone_find_rrset(self,dname,atype)
            #parameters: ldns_dnssec_zone *,ldns_rdf *,ldns_rr_type,
            #retvals: ldns_dnssec_rrsets *

        #_LDNS_DNSSEC_ZONE_METHODS#
 %}
}
