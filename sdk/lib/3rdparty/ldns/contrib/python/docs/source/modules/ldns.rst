LDNS module documentation
================================

Here you can find the documentation of pyLDNS extension module. This module consists of several classes and a couple of functions.

.. toctree::
	:maxdepth: 1
	:glob:

	ldns_resolver
	ldns_pkt
	ldns_rr
	ldns_rdf
	ldns_dname
	ldns_rr_list
	ldns_zone
	ldns_key
	ldns_key_list
	ldns_buffer
	ldns_dnssec
	ldns_func




**Differences against libLDNS**

* You don't need to use ldns-compare functions, instances can be compared using standard operators <, >, = ::
	
	if (some_rr.owner() == another_rr.rdf(1)):
		pass

* Classes contain static methods that create new instances, the name of these methods starts with the new\_ prefix (e.g. :meth:`ldns.ldns_pkt.new_frm_file`).

* Is it possible to print the content of an object using ``print objinst`` (see :meth:`ldns.ldns_resolver.get_addr_by_name`).

* Classes contain write_to_buffer method that writes the content into buffer.

* All the methods that consume parameter of (const ldns_rdf) type allows to use string instead (see :meth:`ldns.ldns_resolver.query`).

