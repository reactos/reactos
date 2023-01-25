/******************************************************************************
 * ldns_zone.i: LDNS zone class
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

%typemap(in,numinputs=0,noblock=1) (ldns_zone **)
{
 ldns_zone *$1_zone;
 $1 = &$1_zone;
}
          
/* result generation */
%typemap(argout,noblock=1) (ldns_zone **)
{
 $result = SWIG_Python_AppendOutput($result, SWIG_NewPointerObj(SWIG_as_voidptr($1_zone), SWIGTYPE_p_ldns_struct_zone, SWIG_POINTER_OWN |  0 ));
}

%nodefaultctor ldns_struct_zone; //no default constructor & destructor
%nodefaultdtor ldns_struct_zone;

%newobject ldns_zone_new_frm_fp;
%newobject ldns_zone_new_frm_fp_l;
%newobject ldns_zone_new;
%delobject ldns_zone_free;
%delobject ldns_zone_deep_free;
%delobject ldns_zone_push_rr;
%delobject ldns_zone_push_rr_list;

%ignore ldns_struct_zone::_soa;
%ignore ldns_struct_zone::_rrs;

%rename(ldns_zone) ldns_struct_zone;

#ifdef LDNS_DEBUG
%rename(__ldns_zone_free) ldns_zone_free;
%rename(__ldns_zone_deep_free) ldns_zone_deep_free;
%inline %{
void _ldns_zone_free (ldns_zone* z) {
   printf("******** LDNS_ZONE free 0x%lX ************\n", (long unsigned int)z);
   ldns_zone_deep_free(z);
}
%}
#else
%rename(__ldns_zone_free) ldns_zone_free;
%rename(_ldns_zone_free) ldns_zone_deep_free;
#endif
%feature("docstring") ldns_struct_zone "Zone definitions

**Usage**

This class is able to read and parse the content of zone file by doing:

>>> import ldns
>>> zone = ldns.ldns_zone.new_frm_fp(open(\"zone.txt\",\"r\"), None, 0, ldns.LDNS_RR_CLASS_IN)
>>> print zone.soa()
example.	600	IN	SOA	example. admin.example. 2008022501 28800 7200 604800 18000
>>> print zone.rrs()
example.	600	IN	MX	10 mail.example.
example.	600	IN	NS	ns1.example.
example.	600	IN	NS	ns2.example.
example.	600	IN	A	192.168.1.1

The ``zone.txt`` file contains the following records::

   $ORIGIN example.
   $TTL 600
   
   example.        IN SOA  example. admin.example. (
                                   2008022501 ; serial
                                   28800      ; refresh (8 hours)
                                   7200       ; retry (2 hours)
                                   604800     ; expire (1 week)
                                   18000      ; minimum (5 hours)
                                   )
   
   @       IN      MX      10 mail.example.
   @       IN      NS      ns1
   @       IN      NS      ns2
   @       IN      A       192.168.1.1
"

%extend ldns_struct_zone {
 
 %pythoncode %{
        def __init__(self):
            self.this = _ldns.ldns_zone_new()
            if not self.this:
                raise Exception("Can't create zone.")
       
        __swig_destroy__ = _ldns._ldns_zone_free

        def __str__(self):
            return str(self.soa()) + "\n" + str(self.rrs())

        def print_to_file(self,output):
            """Prints the data in the zone to the given file stream (in presentation format)."""
            _ldns.ldns_zone_print(output,self)
            #parameters: FILE *,const ldns_zone *,

        #LDNS_ZONE_CONSTRUCTORS_#
        @staticmethod
        def new_frm_fp(file, origin, ttl, rr_class=_ldns.LDNS_RR_CLASS_IN, raiseException=True):
            """Creates a new zone object from given file pointer
               
               :param file: a file object
               :param origin: (ldns_rdf) the zones' origin
               :param ttl: default ttl to use
               :param rr_class: Default class to use (IN)
               :param raiseException: if True, an exception occurs in case a zone instance can't be created
               :returns: zone instance or None. If an instance can't be created and raiseException is True, an exception occurs.
            """
            status, zone = _ldns.ldns_zone_new_frm_fp(file, origin, ttl, rr_class)
            if status != LDNS_STATUS_OK:
                if (raiseException): raise Exception("Can't create zone, error: %s (%d)" % (_ldns.ldns_get_errorstr_by_id(status),status))
                return None
            return zone

        @staticmethod
        def new_frm_fp_l(file, origin, ttl, rr_class, raiseException=True):
            """Create a new zone from a file, keep track of the line numbering
               
               :param file: a file object
               :param origin: (ldns_rdf) the zones' origin
               :param ttl: default ttl to use
               :param rr_class: Default class to use (IN)
               :param raiseException: if True, an exception occurs in case a zone instance can't be created
               :returns: 
                   * zone - zone instance or None. If an instance can't be created and raiseException is True, an exception occurs.

                   * line - used for error msg, to get to the line number
            """
            status, zone = _ldns.ldns_zone_new_frm_fp_l(file, line)
            if status != LDNS_STATUS_OK:
                if (raiseException): raise Exception("Can't create zone, error: %d" % status)
                return None
            return zone
        #_LDNS_ZONE_CONSTRUCTORS#

        def sign(self,key_list):
            """Signs the zone, and returns a newly allocated signed zone.
               
               :param key_list:
                   list of keys to sign with
               :returns: (ldns_zone \*) signed zone
            """
            return _ldns.ldns_zone_sign(self,key_list)
            #parameters: const ldns_zone *,ldns_key_list *,
            #retvals: ldns_zone *

        def sign_nsec3(self,key_list,algorithm,flags,iterations,salt_length,salt):
            """Signs the zone with NSEC3, and returns a newly allocated signed zone.
               
               :param key_list:
                   list of keys to sign with
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
               :returns: (ldns_zone \*) signed zone
            """
            return _ldns.ldns_zone_sign_nsec3(self,key_list,algorithm,flags,iterations,salt_length,salt)
            #parameters: ldns_zone *,ldns_key_list *,uint8_t,uint8_t,uint16_t,uint8_t,uint8_t *,
            #retvals: ldns_zone *

        #LDNS_ZONE_METHODS_#
        def glue_rr_list(self):
            """Retrieve all resource records from the zone that are glue records.
               
               The resulting list does are pointer references to the zone's data.
               
               Due to the current zone implementation (as a list of rr's), this function is extremely slow. Another (probably better) way to do this is to use an ldns_dnssec_zone structure and the mark_glue function
               
               :returns: (ldns_rr_list \*) the rr_list with the glue
            """
            return _ldns.ldns_zone_glue_rr_list(self)
            #parameters: const ldns_zone *,
            #retvals: ldns_rr_list *

        def push_rr(self,rr):
            """push an single rr to a zone structure.
               
               This function use pointer copying, so the rr_list structure inside z is modified!
               
               :param rr:
                   the rr to add
               :returns: (bool) a true on success otherwise falsed
            """
            return _ldns.ldns_zone_push_rr(self,rr)
            #parameters: ldns_zone *,ldns_rr *,
            #retvals: bool

        def push_rr_list(self,list):
            """push an rrlist to a zone structure.
               
               This function use pointer copying, so the rr_list structure inside z is modified!
               
               :param list:
                   the list to add
               :returns: (bool) a true on success otherwise falsed
            """
            return _ldns.ldns_zone_push_rr_list(self,list)
            #parameters: ldns_zone *,ldns_rr_list *,
            #retvals: bool

        def rr_count(self):
            """Returns the number of resource records in the zone, NOT counting the SOA record.
               
               :returns: (size_t) the number of rr's in the zone
            """
            return _ldns.ldns_zone_rr_count(self)
            #parameters: const ldns_zone *,
            #retvals: size_t

        def rrs(self):
            """Get a list of a zone's content.
               
               Note that the SOA isn't included in this list. You need to get the with ldns_zone_soa.
               
               :returns: (ldns_rr_list \*) the rrs from this zone
            """
            return _ldns.ldns_zone_rrs(self)
            #parameters: const ldns_zone *,
            #retvals: ldns_rr_list *

        def set_rrs(self,rrlist):
            """Set the zone's contents.
               
               :param rrlist:
                   the rrlist to use
            """
            _ldns.ldns_zone_set_rrs(self,rrlist)
            #parameters: ldns_zone *,ldns_rr_list *,
            #retvals: 

        def set_soa(self,soa):
            """Set the zone's soa record.
               
               :param soa:
                   the soa to set
            """
            _ldns.ldns_zone_set_soa(self,soa)
            #parameters: ldns_zone *,ldns_rr *,
            #retvals: 

        def soa(self):
            """Return the soa record of a zone.
               
               :returns: (ldns_rr \*) the soa record in the zone
            """
            return _ldns.ldns_zone_soa(self)
            #parameters: const ldns_zone *,
            #retvals: ldns_rr *

        def sort(self):
            """Sort the rrs in a zone, with the current impl.
               
               this is slow
            """
            _ldns.ldns_zone_sort(self)
            #parameters: ldns_zone *,
            #retvals: 

            #_LDNS_ZONE_METHODS#
 %}
}
