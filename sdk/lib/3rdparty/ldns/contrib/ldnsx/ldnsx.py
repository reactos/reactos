# Copyright (C) Xelerance Corp. <http://www.xelerance.com/>.
# Author: Christopher Olah <colah@xelerance.com>
# License: BSD

""" Easy DNS (including DNSSEC) via ldns.

ldns is a great library. It is a powerful tool for
working with DNS. python-ldns it is a straight up clone of the C
interface, however that is not a very good interface for python. Its
documentation is incomplete and some functions don't work as
described. And some objects don't have a full python API.

ldnsx aims to fix this. It wraps around the ldns python bindings,
working around its limitations and providing a well-documented, more
pythonistic interface.

**WARNING:** 

**API subject to change.** No backwards compatibility guarantee. Write software using this version at your own risk!

Examples
--------

Query the default resolver for google.com's A records. Print the response
packet.

>>> import ldnsx
>>> resolver = ldnsx.resolver()
>>> print resolver.query("google.com","A")


Print the root NS records from f.root-servers.net; if we get a
response, else an error message.

>>> import ldnsx
>>> pkt = ldnsx.resolver("f.root-servers.net").query(".", "NS")
>>> if pkt:
>>>    for rr in pkt.answer():
>>>       print rr
>>> else:
>>>    print "response not received" 

"""

import time, sys, calendar, warnings, socket
try:
	import ldns
except ImportError:
	print >> sys.stderr, "ldnsx requires the ldns-python sub-package from http://www.nlnetlabs.nl/projects/ldns/"
	print >> sys.stderr, "Fedora/CentOS: yum install ldns-python"
	print >> sys.stderr, "Debian/Ubuntu: apt-get install python-ldns"
	print >> sys.stderr, "openSUSE: zypper in python-ldns"
	sys.exit(1)

__version__ = "0.1"

def isValidIP(ipaddr):
	try:
		v4 = socket.inet_pton(socket.AF_INET,ipaddr)
		return 4
	except:
		try:
			v6 = socket.inet_pton(socket.AF_INET6,ipaddr)
			return 6
		except:
			return 0

def query(name, rr_type, rr_class="IN", flags=["RD"], tries = 3, res=None):
	"""Convenience function. Creates a resolver and then queries it. Refer to resolver.query() 
	   * name -- domain to query for 
	   * rr_type -- rr_type to query for
	   * flags -- flags for query (list of strings)
	   * tries -- number of times to retry the query on failure
	   * res -- configurations for the resolver as a dict -- see resolver()
	   """
	if isinstance(res, list) or isinstance(res, tuple):
		res = resolver(*res)
	elif isinstance(res, dict):
		res = resolver(**res)
	else:
		res = resolver(res)
	return res.query(name, rr_type, rr_class, flags, tries)

def get_rrs(name, rr_type, rr_class="IN", tries = 3, strict = False, res=None, **kwds):
	"""Convenience function. Gets RRs for name of type rr_type trying tries times. 
	   If strict, it raises and exception on failure, otherwise it returns []. 
	   * name -- domain to query for 
	   * rr_type -- rr_type to query for
	   * flags -- flags for query (list of strings)
	   * tries -- number of times to retry the query on failure
	   * strict -- if the query fails, do we return [] or raise an exception?
	   * res -- configurations for the resolver as a dict -- see resolver()
	   * kwds -- query filters, refer to packet.answer()
       """
	if isinstance(res, list) or isinstance(res, tuple):
		res = resolver(*res)
	elif isinstance(res, dict):
		res = resolver(**res)
	else:
		res = resolver(res)
	if "|" in rr_type:
		pkt = res.query(name, "ANY", rr_class=rr_class, tries=tries)
	else:
		pkt = res.query(name, rr_type, rr_class=rr_class, tries=tries)
	if pkt:
		if rr_type in ["", "ANY", "*"]:
			return pkt.answer( **kwds)
		else:
			return pkt.answer(rr_type=rr_type, **kwds)
	else:
		if strict:
			raise Exception("LDNS couldn't complete query")
		else:
			return []

def secure_query(name, rr_type, rr_class="IN", flags=["RD"], tries = 1, flex=False, res=None):
	"""Convenience function. Creates a resolver and then does a DNSSEC query. Refer to resolver.query() 
	   * name -- domain to query for 
	   * rr_type -- rr_type to query for
	   * flags -- flags for query (list of strings)
	   * tries -- number of times to retry the query on failure
	   * flex -- if we can't verify data, exception or warning?
	   * res -- configurations for the resolver as a dict -- see resolver()"""
	if isinstance(res, list) or isinstance(res, tuple):
		res = resolver(*res)
	elif isinstance(res, dict):
		res = resolver(**res)
	else:
		res = resolver(res)
	pkt = res.query(name, rr_type, rr_class, flags, tries)
	if pkt.rcode() == "SERVFAIL":
		raise Exception("%s lookup failed (server error or dnssec validation failed)" % name)
	if pkt.rcode() == "NXDOMAIN":
		if "AD" in pkt.flags():
			raise Exception("%s lookup failed (non-existence proven by DNSSEC)" % name )
		else:
			raise Exception("%s lookup failed" % name )
	if pkt.rcode() == "NOERROR":
		if "AD" not in pkt.flags():
			if not flex:
				raise Exception("DNS lookup was insecure")
			else:
				warnings.warn("DNS lookup was insecure")
		return pkt
	else:
		raise Exception("unknown ldns error, %s" % pkt.rcode())



class resolver:
	""" A wrapper around ldns.ldns_resolver. 

			**Examples**

			Making resolvers is easy!

			>>> from ldnsx import resolver
			>>> resolver() # from /etc/resolv.conf
			<resolver: 192.168.111.9>
			>>> resolver("") # resolver with no nameservers
			<resolver: >
			>>> resolver("193.110.157.135") #resolver pointing to ip addr
			<resolver: 193.110.157.135>
			>>> resolver("f.root-servers.net") # resolver pointing ip address(es) resolved from name
			<resolver: 2001:500:2f::f, 192.5.5.241>
			>>> resolver("193.110.157.135, 193.110.157.136") 
			>>> # resolver pointing to multiple ip addr, first takes precedence.
			<resolver: 193.110.157.136, 193.110.157.135>

			So is playing around with their nameservers!

			>>> import ldnsx
			>>> res = ldnsx.resolver("192.168.1.1")
			>>> res.add_nameserver("192.168.1.2")
			>>> res.add_nameserver("192.168.1.3")
			>>> res.nameservers_ip()
			["192.168.1.1","192.168.1.2","192.168.1.3"]

			And querying!

			>>> from ldnsx import resolver
			>>> res= resolver()
			>>> res.query("cow.com","A")
			;; ->>HEADER<<- opcode: QUERY, rcode: NOERROR, id: 7663
			;; flags: qr rd ra ; QUERY: 1, ANSWER: 1, AUTHORITY: 0, ADDITIONAL: 0 
			;; QUESTION SECTION:
			;; cow.com.     IN      A
			;; ANSWER SECTION:
			cow.com.        300     IN      A       208.87.34.18
			;; AUTHORITY SECTION:
			;; ADDITIONAL SECTION:
			;; Query time: 313 msec
			;; SERVER: 192.168.111.9
			;; WHEN: Fri Jun  3 11:01:02 2011
			;; MSG SIZE  rcvd: 41

	
			"""
	
	def __init__(self, ns = None, dnssec = False, tcp = False, port = 53):
		"""resolver constructor
			
			* ns    --  the nameserver/comma delimited nameserver list
			            defaults to settings from /etc/resolv.conf
			* dnssec -- should the resolver try and use dnssec or not?
		    * tcp -- should the resolver use TCP
		             'auto' is a deprecated work around for old ldns problems
		    * port -- the port to use, must be the same for all nameservers

			"""
		# We construct based on a file and dump the nameservers rather than using
		# ldns_resolver_new() to avoid environment/configuration/magic specific 
		# bugs.
		self._ldns_resolver = ldns.ldns_resolver.new_frm_file("/etc/resolv.conf")
		if ns != None:
			self.drop_nameservers()
			nm_list = ns.split(',')
			nm_list = map(lambda s: s.strip(), nm_list)
			nm_list = filter(lambda s: s != "", nm_list)
			nm_list.reverse()
			for nm in nm_list:
				self.add_nameserver(nm)
		# Configure DNSSEC, tcp and port
		self.set_dnssec(dnssec)
		if tcp == 'auto':
			self.autotcp = True
			self._ldns_resolver.set_usevc(False)
		else:
			self.autotcp = False
			self._ldns_resolver.set_usevc(tcp)
		self._ldns_resolver.set_port(port)

	
	def query(self, name, rr_type, rr_class="IN", flags=["RD"], tries = 3):
		"""Run a query on the resolver.
				
			* name -- name to query for
			* rr_type -- the record type to query for
			* rr_class -- the class to query for, defaults to IN (Internet)
			* flags -- the flags to send the query with 
			* tries -- the number of times to attempt to achieve query in case of packet loss, etc
			
			**Examples**
			
			Let's get some A records!

			>>> google_a_records = resolver.query("google.com","A").answer()
			
			Using DNSSEC is easy :)

			>>> dnssec_pkt = ldnsx.resolver(dnssec=True).query("xelerance.com")
			
			We let you use strings to make things easy, but if you prefer stay close to DNS...

			>>> AAAA = 28
			>>> resolver.query("ipv6.google.com", AAAA)
			
			**More about rr_type**
			
			rr_type must be a supported resource record type. There are a large number of RR types:

			===========  ===================================  ==================
			TYPE         Value and meaning                    Reference
			===========  ===================================  ==================
			A            1 a host address                     [RFC1035]
			NS           2 an authoritative name server       [RFC1035]
			...
			AAAA         28 IP6 Address                       [RFC3596]
			...
			DS           43 Delegation Signer                 [RFC4034][RFC3658]
			...
			DNSKEY       48 DNSKEY                            [RFC4034][RFC3755]
			...
			Unassigned   32770-65279  
			Private use  65280-65534
			Reserved     65535 
			===========  ===================================  ==================
			
			(From http://www.iana.org/assignments/dns-parameters)

			RR types are given as a string (eg. "A"). In the case of Unassigned/Private use/Reserved ones,
			they are given as "TYPEXXXXX" where XXXXX is the number. ie. RR type 65280 is "TYPE65280". You 
			may also pass the integer, but you always be given the string.

			If the version of ldnsx you are using is old, it is possible that there could be new rr_types that
			we don't recognise mnemonic for. You can still use the number XXX or the string "TYPEXXX". To
			determine what rr_type mnemonics we support, please refer to resolver.supported_rr_types()

		"""
		# Determine rr_type int
		if rr_type in _rr_types.keys():
			_rr_type = _rr_types[rr_type]
		elif isinstance(rr_type,int):
			_rr_type = rr_type
		elif isinstance(rr_type,str) and rr_type[0:4] == "TYPE":
			try:
				_rr_type = int(rr_type[4:])
			except:
				raise Exception("%s is a bad RR type. TYPEXXXX: XXXX must be a number")
		else:
			raise Exception("ldnsx (version %s) does not support the RR type %s."  % (__version__, str(rr_type)) ) 
		# Determine rr_class int
		if   rr_class == "IN": _rr_class = ldns.LDNS_RR_CLASS_IN 
		elif rr_class == "CH": _rr_class = ldns.LDNS_RR_CLASS_CH
		elif rr_class == "HS": _rr_class = ldns.LDNS_RR_CLASS_HS
		else:
			raise Exception("ldnsx (version %s) does not support the RR class %s." % (__version__, str(rr_class)) ) 
		# Determine flags int
		_flags = 0
		if "QR" in flags:  _flags |= ldns.LDNS_QR
		if "AA" in flags:  _flags |= ldns.LDNS_AA
		if "TC" in flags:  _flags |= ldns.LDNS_TC
		if "RD" in flags:  _flags |= ldns.LDNS_RD
		if "CD" in flags:  _flags |= ldns.LDNS_CD
		if "RA" in flags:  _flags |= ldns.LDNS_RA
		if "AD" in flags:  _flags |= ldns.LDNS_AD
		# Query
		if tries == 0: return None
		try:
			pkt = self._ldns_resolver.query(name, _rr_type, _rr_class, _flags)
		except KeyboardInterrupt: #Since so much time is spent waiting on ldns, this is very common place for Ctr-C to fall
			raise
		except: #Since the ldns exception is not very descriptive...
			raise Exception("ldns backend ran into problems. Likely, the name you were querying for, %s, was invalid." % name)
		#Deal with failed queries
		if not pkt:
			if tries <= 1:
				return None
			else:
				# One of the major causes of none-packets is truncation of packets
				# When autotcp is set, we are in a flexible enough position to try and use tcp
				# to get around this.
				# Either way, we want to replace the resolver, since resolvers will sometimes
				# just freeze up.
				if self.autotcp:
					self = resolver( ",".join(self.nameservers_ip()),tcp=True, dnssec = self._ldns_resolver.dnssec())
					self.autotcp = True
					pkt = self.query(name, rr_type, rr_class=rr_class, flags=flags, tries = tries-1) 
					self._ldns_resolver.set_usevc(False)
					return pkt
				else:
					self = resolver( ",".join(self.nameservers_ip()), tcp = self._ldns_resolver.usevc(), dnssec = self._ldns_resolver.dnssec() )
					time.sleep(1) # It could be that things are failing because of a brief outage
					return self.query(name, rr_type, rr_class=rr_class, flags=flags, tries = tries-1) 
		elif self.autotcp:
			pkt = packet(pkt)
			if "TC" in pkt.flags():
				self._ldns_resolver.set_usevc(True)
				pkt2 = self.query(name, rr_type, rr_class=rr_class, flags=flags, tries = tries-1) 
				self._ldns_resolver.set_usevc(False)
				if pkt2: return packet(pkt2)
			return pkt
		return packet(pkt)
		#ret = []
		#for rr in pkt.answer().rrs():
		#	ret.append([str(rr.owner()),rr.ttl(),rr.get_class_str(),rr.get_type_str()]+[str(rdf) for rdf in rr.rdfs()])
		#return ret

	def suported_rr_types(self):
		""" Returns the supported DNS resource record types.

			Refer to resolver.query() for thorough documentation of resource 
			record types or refer to:

			http://www.iana.org/assignments/dns-parameters

		"""
		return _rr_types.keys()
	
	def AXFR(self,name):
		"""AXFR for name

			* name -- name to AXFR for
			
			This function is a generator. As it AXFRs it will yield you the records.

			**Example**

			Let's get a list of the tlds (gotta catch em all!):

			>>> tlds = []
			>>> for rr in resolver("f.root-servers.net").AXFR("."):
			>>>    if rr.rr_type() == "NS":
			>>>       tlds.append(rr.owner())

		"""
		#Dname seems to be unnecessary on some computers, but it is on others. Avoid bugs.
		if self._ldns_resolver.axfr_start(ldns.ldns_dname(name), ldns.LDNS_RR_CLASS_IN) != ldns.LDNS_STATUS_OK:
			raise Exception("Starting AXFR failed. Error: %s" % ldns.ldns_get_errorstr_by_id(status))
		pres = self._ldns_resolver.axfr_next()
		while pres:
			yield resource_record(pres)
			pres = self._ldns_resolver.axfr_next()

	def nameservers_ip(self):
		""" returns a list of the resolvers nameservers (as IP addr)
		
		"""
		nm_stack2 =[]
		nm_str_stack2=[]
		nm = self._ldns_resolver.pop_nameserver()
		while nm:
			nm_stack2.append(nm)
			nm_str_stack2.append(str(nm))
			nm = self._ldns_resolver.pop_nameserver()
		for nm in nm_stack2:
			self._ldns_resolver.push_nameserver(nm)
		nm_str_stack2.reverse()
		return nm_str_stack2


	def add_nameserver(self,ns):
		""" Add a nameserver, IPv4/IPv6/name.

		"""
		if isValidIP(ns) == 4:
			address = ldns.ldns_rdf_new_frm_str(ldns.LDNS_RDF_TYPE_A,ns)
			self._ldns_resolver.push_nameserver(address)
		elif isValidIP(ns) == 6:
			address = ldns.ldns_rdf_new_frm_str(ldns.LDNS_RDF_TYPE_AAAA,ns)
			self._ldns_resolver.push_nameserver(address)
		else:
			resolver = ldns.ldns_resolver.new_frm_file("/etc/resolv.conf")
			#address = resolver.get_addr_by_name(ns)
			address = resolver.get_addr_by_name(ldns.ldns_dname(ns))
			if not address:
				address = resolver.get_addr_by_name(ldns.ldns_dname(ns))
				if not address:
					raise Exception("Failed to resolve address for %s" % ns)
			for rr in address.rrs():
				self._ldns_resolver.push_nameserver_rr(rr)

	def drop_nameservers(self):
		"""Drops all nameservers.
			This function causes the resolver to forget all nameservers.

		"""
		while self._ldns_resolver.pop_nameserver():
			pass

	def set_nameservers(self, nm_list):
		"""Takes a list of nameservers and sets the resolver to use them
		
		"""
		self.drop_nameservers()
		for nm in nm_list:
			self.add_nameserver(nm)

	def __repr__(self):
		return "<resolver: %s>" % ", ".join(self.nameservers_ip())
	__str__ = __repr__

	def set_dnssec(self,new_dnssec_status):
		"""Set whether the resolver uses DNSSEC.
		
		"""
		self._ldns_resolver.set_dnssec(new_dnssec_status)

class packet:
	
	def _construct_rr_filter(self, **kwds):
		def match(pattern, target):
			if pattern[0] in ["<",">","!"]:
				rel = pattern[0]
				pattern=pattern[1:]
			elif pattern[0:2] in ["<=","=>"]:
				rel = pattern[0:2]
				pattern=pattern[2:]
			else:
				rel = "="
			for val in pattern.split("|"):
				if {"<" : target <  val,
				    ">" : target >  val,
				    "!" : target != val,
				    "=" : target == val,
				    ">=": target >= val,
				    "<=": target <= val}[rel]:
					return True
			return False
		def f(rr):
			for key in kwds.keys():
				if ( ( isinstance(kwds[key], list) and str(rr[key]) not in map(str,kwds[key]) )
				  or ( not isinstance(kwds[key], list) and not match(str(kwds[key]), str(rr[key])))):
					return False
			return True
		return f
	
	def __init__(self, pkt):
		self._ldns_pkt = pkt
	
	def __repr__(self):
		return str(self._ldns_pkt)
	__str__ = __repr__
	
	def rcode(self):
		"""Returns the rcode.

		Example returned value: "NOERROR"

		possible rcodes (via ldns): "FORMERR", "MASK", "NOERROR",
		"NOTAUTH", "NOTIMPL", "NOTZONE", "NXDOMAIN",
		"NXRSET", "REFUSED", "SERVFAIL", "SHIFT", 
		"YXDOMAIN", "YXRRSET"

		Refer to http://www.iana.org/assignments/dns-parameters
		section: DNS RCODEs
		"""
		return self._ldns_pkt.rcode2str()

	def opcode(self):
		"""Returns the rcode.

		Example returned value: "QUERY"

		"""
		return self._ldns_pkt.opcode2str()
	
	def flags(self):
		"""Return packet flags (as list of strings).
		
		Example returned value: ['QR', 'RA', 'RD']

		**What are the flags?**
		
		========  ====  =====================  =========
		Bit       Flag  Description            Reference
		========  ====  =====================  =========
		bit 5     AA    Authoritative Answer   [RFC1035]
		bit 6     TC    Truncated Response     [RFC1035]
		bit 7     RD    Recursion Desired      [RFC1035]
		bit 8     RA    Recursion Allowed      [RFC1035]
		bit 9           Reserved
		bit 10    AD    Authentic Data         [RFC4035]
		bit 11    CD    Checking Disabled      [RFC4035]
		========  ====  =====================  =========

		(from http://www.iana.org/assignments/dns-parameters)

		There is also QR. It is mentioned in other sources,
		though not the above page. It being false means that
		the packet is a query, it being true means that it is
		a response.

		"""
		ret = []
		if self._ldns_pkt.aa(): ret += ["AA"]
		if self._ldns_pkt.ad(): ret += ["AD"]
		if self._ldns_pkt.cd(): ret += ["CD"]
		if self._ldns_pkt.qr(): ret += ["QR"]
		if self._ldns_pkt.ra(): ret += ["RA"]
		if self._ldns_pkt.rd(): ret += ["RD"]
		if self._ldns_pkt.tc(): ret += ["TC"]
		return ret

	def answer(self, **filters):
		"""Returns the answer section.
		
		* filters -- a filtering mechanism
		
		Since a very common desire is to filter the resource records in a packet
		section, we provide a special tool for doing this: filters. They are a
		lot like regular python filters, but more convenient. If you set a 
		field equal to some value, you will only receive resource records for which
		it holds true.

		**Examples**

		>>> res = ldnsx.resolver()
		>>> pkt = res.query("google.ca","A")
		>>> pkt.answer()
		[google.ca.     28      IN      A       74.125.91.99
		, google.ca.    28      IN      A       74.125.91.105
		, google.ca.    28      IN      A       74.125.91.147
		, google.ca.    28      IN      A       74.125.91.103
		, google.ca.    28      IN      A       74.125.91.104
		, google.ca.    28      IN      A       74.125.91.106
		]

		To understand filters, consider the following:

		>>> pkt = ldnsx.query("cow.com","ANY")
		>>> pkt.answer()
		[cow.com.       276     IN      A       208.87.32.75
		, cow.com.      3576    IN      NS      sell.internettraffic.com.
		, cow.com.      3576    IN      NS      buy.internettraffic.com.
		, cow.com.      3576    IN      SOA     buy.internettraffic.com. hostmaster.hostingnet.com. 1308785320 10800 3600 604800 3600
		]
		>>> pkt.answer(rr_type="A")
		[cow.com.       276     IN      A       208.87.32.75
		]
		>>> pkt.answer(rr_type="A|NS")
		[cow.com.       276     IN      A       208.87.32.75
		, cow.com.      3576    IN      NS      sell.internettraffic.com.
		, cow.com.      3576    IN      NS      buy.internettraffic.com.
		]
		>>> pkt.answer(rr_type="!NS")
		[cow.com.       276     IN      A       208.87.32.75
		, cow.com.      3576    IN      SOA     buy.internettraffic.com. hostmaster.hostingnet.com. 1308785320 10800 3600 604800 3600
		]
		
		fields are the same as when indexing a resource record.
		note: ordering is alphabetical.
		"""
		ret =  [resource_record(rr) for rr in self._ldns_pkt.answer().rrs()]
		return filter(self._construct_rr_filter(**filters), ret)

	def authority(self, **filters):
		"""Returns the authority section.

		* filters -- a filtering mechanism
		
		Since a very common desire is to filter the resource records in a packet
		section, we provide a special tool for doing this: filters. They are a
		lot like regular python filters, but more convenient. If you set a 
		field equal to some value, you will only receive resource records for which
		it holds true. See answer() for details.

		**Examples**

		>>> res = ldnsx.resolver()
		>>> pkt = res.query("google.ca","A")
		>>> pkt.authority()
		[google.ca.     251090  IN      NS      ns3.google.com.
		, google.ca.    251090  IN      NS      ns1.google.com.
		, google.ca.    251090  IN      NS      ns2.google.com.
		, google.ca.    251090  IN      NS      ns4.google.com.
		]

		"""
		ret = [resource_record(rr) for rr in self._ldns_pkt.authority().rrs()]
		return filter(self._construct_rr_filter(**filters), ret)

	def additional(self, **filters):
		"""Returns the additional section.

		* filters -- a filtering mechanism
		
		Since a very common desire is to filter the resource records in a packet
		section, we provide a special tool for doing this: filters. They are a
		lot like regular python filters, but more convenient. If you set a 
		field equal to some value, you will only receive resource records for which
		it holds true. See answer() for details.

		**Examples**

		>>> res = ldnsx.resolver()
		>>> pkt = res.query("google.ca","A")
		>>> pkt.additional()
		[ns3.google.com.        268778  IN      A       216.239.36.10
		, ns1.google.com.       262925  IN      A       216.239.32.10
		, ns2.google.com.       255659  IN      A       216.239.34.10
		, ns4.google.com.       264489  IN      A       216.239.38.10
		]

		"""
		ret = [resource_record(rr) for rr in self._ldns_pkt.additional().rrs()]
		return filter(self._construct_rr_filter(**filters), ret)

	def question(self, **filters):
		"""Returns the question section.

		* filters -- a filtering mechanism
		
		Since a very common desire is to filter the resource records in a packet
		section, we provide a special tool for doing this: filters. They are a
		lot like regular python filters, but more convenient. If you set a 
		field equal to some value, you will only receive resource records for which
		it holds true. See answer() for details.

		"""
		ret = [resource_record(rr) for rr in self._ldns_pkt.question().rrs()]
		return filter(self._construct_rr_filter(**filters), ret)

class resource_record:
	
	_rdfs = None
	_iter_pos = None
	
	def __init__(self, rr):
		self._ldns_rr = rr
		self._rdfs = [str(rr.owner()),rr.ttl(),rr.get_class_str(),rr.get_type_str()]+[str(rdf) for rdf in rr.rdfs()]
	
	def __repr__(self):
		return str(self._ldns_rr)
	
	__str__ = __repr__

	def __iter__(self):
		self._iter_pos = 0
		return self

	def next(self):
		if self._iter_pos < len(self._rdfs):
			self._iter_pos += 1
			return self._rdfs[self._iter_pos-1]
		else:
			raise StopIteration

	def __len__(self):
		try:
			return len(self._rdfs)
		except:
			return 0

	def __getitem__(self, n):
		if isinstance(n, int):
			return self._rdfs[n]
		elif isinstance(n, str):
			n = n.lower()
			if n in ["owner"]:
				return self.owner()
			elif n in ["rr_type", "rr type", "type"]:
				return self.rr_type()
			elif n in ["rr_class", "rr class", "class"]:
				return self.rr_class()
			elif n in ["covered_type", "covered type", "type2"]:
				return self.covered_type()
			elif n in ["ttl"]:
				return self.ttl()
			elif n in ["ip"]:
				return self.ip()			
			elif n in ["alg", "algorithm"]:
				return self.alg()
			elif n in ["protocol"]:
				return self.protocol()
			elif n in ["flags"]:
				return self.flags()
			else:
				raise Exception("ldnsx (version %s) does not recognize the rr field %s" % (__version__,n) ) 
		else:
			raise TypeError("bad type %s for index resource record" % type(n) ) 
			

	#def rdfs(self):
	#	return self._rdfs.clone()
	
	def owner(self):
		"""Get the RR's owner"""
		return str(self._ldns_rr.owner())
	
	def rr_type(self):
		"""Get a RR's type """
		return self._ldns_rr.get_type_str()

	def covered_type(self):
		"""Get an RRSIG RR's covered type"""
		if self.rr_type() == "RRSIG":
			return self[4]
		else:
			return ""
	
	def rr_class(self):
		"""Get the RR's collapse"""
		return self._ldns_rr.get_class_str()
	
	def ttl(self):
		"""Get the RR's TTL"""
		return self._ldns_rr.ttl()

	def inception(self, out_format="UTC"):
		"""returns the inception time in format out_format, defaulting to a UTC string. 
		options for out_format are:

		   UTC -- a UTC string eg. 20110712192610 (2011/07/12 19:26:10) 
		   unix -- number of seconds since the epoch, Jan 1, 1970
		   struct_time -- the format used by python's time library
		"""
		# Something very strange is going on with inception/expiration dates in DNS.
		# According to RFC 4034 section 3.1.5 (http://tools.ietf.org/html/rfc4034#page-9)
		# the inception/expiration fields should be in seconds since Jan 1, 1970, the Unix
		# epoch (as is standard in unix). Yet all the packets I've seen provide UTC encoded
		# as a string instead, eg. "20110712192610" which is 2011/07/12 19:26:10. 
		#
		# It turns out that this is a standard thing that ldns is doing before the data gets 
		# to us.
		if self.rr_type() == "RRSIG":
			if out_format.lower() in ["utc", "utc str", "utc_str"]:
				return self[9]
			elif out_format.lower() in ["unix", "posix", "ctime"]:
				return calendar.timegm(time.strptime(self[9], "%Y%m%d%H%M%S"))
			elif out_format.lower() in ["relative"]:
				return calendar.timegm(time.strptime(self[9], "%Y%m%d%H%M%S")) - time.time()
			elif out_format.lower() in ["struct_time", "time.struct_time"]:
				return time.strptime(self[9], "%Y%m%d%H%M%S")
			else:
				raise Exception("unrecognized time format")
		else:
			return ""

	def expiration(self, out_format="UTC"):
		"""get expiration time. see inception() for more information"""
		if self.rr_type() == "RRSIG":
			if out_format.lower() in ["utc", "utc str", "utc_str"]:
				return self[8]
			elif out_format.lower() in ["unix", "posix", "ctime"]:
				return calendar.timegm(time.strptime(self[8], "%Y%m%d%H%M%S"))
			elif out_format.lower() in ["relative"]:
				return calendar.timegm(time.strptime(self[8], "%Y%m%d%H%M%S")) - time.time()
			elif out_format.lower() in ["struct_time", "time.struct_time"]:
				return time.strptime(self[8], "%Y%m%d%H%M%S")
			else:
				raise Exception("unrecognized time format")
		else:
			return ""

	def ip(self):
		""" IP address form A/AAAA record"""
		if self.rr_type() in ["A", "AAAA"]:
			return self[4]
		else:
			raise Exception("ldnsx does not support ip for records other than A/AAAA")

	def alg(self):
		"""Returns algorithm of RRSIG/DNSKEY/DS"""
		t = self.rr_type() 
		if t == "RRSIG":
			return int(self[5])
		elif t == "DNSKEY":
			return int(self[6])
		elif t == "DS":
			return int(self[5])
		else:
			return -1

	def protocol(self):
		""" Returns protocol of the DNSKEY"""
		t = self.rr_type() 
		if t == "DNSKEY":
			return int(self[5])
		else:
			return -1

	def flags(self):
		"""Return RR flags for DNSKEY """
		t = self.rr_type() 
		if t == "DNSKEY":
			ret = []
			n = int(self[4])
			for m in range(1):
				if 2**(15-m) & n:
					if   m == 7: ret.append("ZONE")
					elif m == 8: ret.append("REVOKE")
					elif m ==15: ret.append("SEP")
					else:        ret.append(m)
			return ret
		else:
			return []

_rr_types={
	"A"    : ldns.LDNS_RR_TYPE_A,
	"A6"   : ldns.LDNS_RR_TYPE_A6,
	"AAAA" : ldns.LDNS_RR_TYPE_AAAA,
	"AFSDB": ldns.LDNS_RR_TYPE_AFSDB,
	"ANY"  : ldns.LDNS_RR_TYPE_ANY,
	"APL"  : ldns.LDNS_RR_TYPE_APL,
	"ATMA" : ldns.LDNS_RR_TYPE_ATMA,
	"AXFR" : ldns.LDNS_RR_TYPE_AXFR,
	"CDNSKEY" : ldns.LDNS_RR_TYPE_CDNSKEY,
	"CDS" : ldns.LDNS_RR_TYPE_CDS,
	"CERT" : ldns.LDNS_RR_TYPE_CERT,
	"CNAME": ldns.LDNS_RR_TYPE_CNAME,
	"COUNT": ldns.LDNS_RR_TYPE_COUNT,
	"DHCID": ldns.LDNS_RR_TYPE_DHCID,
	"DLV"  : ldns.LDNS_RR_TYPE_DLV,
	"DNAME": ldns.LDNS_RR_TYPE_DNAME,
	"DNSKEY": ldns.LDNS_RR_TYPE_DNSKEY,
	"DS"   : ldns.LDNS_RR_TYPE_DS,
	"EID"  : ldns.LDNS_RR_TYPE_EID,
	"FIRST": ldns.LDNS_RR_TYPE_FIRST,
	"GID"  : ldns.LDNS_RR_TYPE_GID,
	"GPOS" : ldns.LDNS_RR_TYPE_GPOS,
	"HINFO": ldns.LDNS_RR_TYPE_HINFO,
	"IPSECKEY": ldns.LDNS_RR_TYPE_IPSECKEY,
	"ISDN" : ldns.LDNS_RR_TYPE_ISDN,
	"IXFR" : ldns.LDNS_RR_TYPE_IXFR,
	"KEY"  : ldns.LDNS_RR_TYPE_KEY,
	"KX"   : ldns.LDNS_RR_TYPE_KX,
	"LAST" : ldns.LDNS_RR_TYPE_LAST,
	"LOC"  : ldns.LDNS_RR_TYPE_LOC,
	"MAILA": ldns.LDNS_RR_TYPE_MAILA,
	"MAILB": ldns.LDNS_RR_TYPE_MAILB,
	"MB"   : ldns.LDNS_RR_TYPE_MB,
	"MD"   : ldns.LDNS_RR_TYPE_MD,
	"MF"   : ldns.LDNS_RR_TYPE_MF,
	"MG"   : ldns.LDNS_RR_TYPE_MG,
	"MINFO": ldns.LDNS_RR_TYPE_MINFO,
	"MR"   : ldns.LDNS_RR_TYPE_MR,
	"MX"   : ldns.LDNS_RR_TYPE_MX,
	"NAPTR": ldns.LDNS_RR_TYPE_NAPTR,
	"NIMLOC": ldns.LDNS_RR_TYPE_NIMLOC,
	"NS"   : ldns.LDNS_RR_TYPE_NS,
	"NSAP" : ldns.LDNS_RR_TYPE_NSAP,
	"NSAP_PTR" : ldns.LDNS_RR_TYPE_NSAP_PTR,
	"NSEC" : ldns.LDNS_RR_TYPE_NSEC,
	"NSEC3": ldns.LDNS_RR_TYPE_NSEC3,
	"NSEC3PARAM" : ldns.LDNS_RR_TYPE_NSEC3PARAM,
	"NSEC3PARAMS" : ldns.LDNS_RR_TYPE_NSEC3PARAMS,
	"NULL" : ldns.LDNS_RR_TYPE_NULL,
	"NXT"  : ldns.LDNS_RR_TYPE_NXT,
	"OPENPGPKEY" : ldns.LDNS_RR_TYPE_OPENPGPKEY,
	"OPT"  : ldns.LDNS_RR_TYPE_OPT,
	"PTR"  : ldns.LDNS_RR_TYPE_PTR,
	"PX"   : ldns.LDNS_RR_TYPE_PX,
	"RP"   : ldns.LDNS_RR_TYPE_RP,
	"RRSIG": ldns.LDNS_RR_TYPE_RRSIG,
	"RT"   : ldns.LDNS_RR_TYPE_RT,
	"SIG"  : ldns.LDNS_RR_TYPE_SIG,
	"SINK" : ldns.LDNS_RR_TYPE_SINK,
	"SOA"  : ldns.LDNS_RR_TYPE_SOA,
	"SRV"  : ldns.LDNS_RR_TYPE_SRV,
	"SSHFP": ldns.LDNS_RR_TYPE_SSHFP,
	"TLSA" : ldns.LDNS_RR_TYPE_TLSA,
	"TSIG" : ldns.LDNS_RR_TYPE_TSIG,
	"TXT"  : ldns.LDNS_RR_TYPE_TXT,
	"UID"  : ldns.LDNS_RR_TYPE_UID,
	"UINFO": ldns.LDNS_RR_TYPE_UINFO,
	"UNSPEC": ldns.LDNS_RR_TYPE_UNSPEC,
	"WKS"  : ldns.LDNS_RR_TYPE_WKS,
	"X25"  : ldns.LDNS_RR_TYPE_X25
}

