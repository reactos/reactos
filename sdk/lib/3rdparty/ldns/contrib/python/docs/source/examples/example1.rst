Resolving the MX records
==============================

This basic example shows how to create a resolver which asks for MX records which contain the information about mail servers.

::

	#!/usr/bin/python
	#
	# MX is a small program that prints out the mx records for a particular domain
	#
	import ldns
	
	resolver = ldns.ldns_resolver.new_frm_file("/etc/resolv.conf")

	dname = ldns.ldns_dname("nic.cz")
	
	pkt = resolver.query(dname, ldns.LDNS_RR_TYPE_MX, ldns.LDNS_RR_CLASS_IN, ldns.LDNS_RD)
	if (pkt):
		mx = pkt.rr_list_by_type(ldns.LDNS_RR_TYPE_MX, ldns.LDNS_SECTION_ANSWER)
		if (mx):
			mx.sort()
			print mx

Resolving step by step
------------------------

First of all we import :mod:`ldns` extension module which make LDNS functions and classes accessible::

	import ldns

If importing fails, it means that Python cannot find the module or ldns library.

Then we create the resolver by :meth:`ldns.ldns_resolver.new_frm_file` constructor ::

	resolver = ldns.ldns_resolver.new_frm_file("/etc/resolv.conf")

and domain name variable dname::

	dname = ldns.ldns_dname("nic.cz")

To create a resolver you may also use::

	resolver = ldns.ldns_resolver.new_frm_file(None)

which behaves in the same manner as the command above.

In the third step we tell the resolver to query for our domain, type MX, of class IN::
	
	pkt = resolver.query(dname, ldns.LDNS_RR_TYPE_MX, ldns.LDNS_RR_CLASS_IN, ldns.LDNS_RD)

The function should return a packet if everything goes well and this packet will contain resource records we asked for. 
Note that there exists a simpler way. Instead of using a dname variable, we can use a string which will be automatically converted.
::

	pkt = resolver.query("fit.vutbr.cz", ldns.LDNS_RR_TYPE_MX, ldns.LDNS_RR_CLASS_IN, ldns.LDNS_RD)

Now, we test whether the resolver returns a packet and then get all RRs of type MX from the answer packet and store them in list mx::

	if (pkt):
		mx = pkt.rr_list_by_type(ldns.LDNS_RR_TYPE_MX, ldns.LDNS_SECTION_ANSWER)

If this list is not empty, we sort and print the content to stdout::

	if (mx):
		mx.sort()
		print mx

