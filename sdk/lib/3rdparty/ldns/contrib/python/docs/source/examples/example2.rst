.. _ex_dnssec:

Querying DNS-SEC validators
===========================

This basic example shows how to query validating resolver and
evaluate answer.

Resolving step by step
------------------------

For DNS queries, we need to initialize ldns resolver (covered in previous example).
   
::
   
   # Create resolver
   resolver = ldns.ldns_resolver.new_frm_file("/etc/resolv.conf")
   resolver.set_dnssec(True)

   # Custom resolver
   if argc > 2:
      # Clear previous nameservers
      ns = resolver.pop_nameserver()
      while ns != None:
         ns = resolver.pop_nameserver()
      ip = ldns.ldns_rdf.new_frm_str(sys.argv[2], ldns.LDNS_RDF_TYPE_A)
      resolver.push_nameserver(ip)

Note the second line :meth:`resolver.set_dnssec`, which enables DNSSEC OK bit
in queries in order to get meaningful results.

As we have resolver initialized, we can start querying for domain names :

::
   
   # Resolve DNS name
   pkt = resolver.query(name, ldns.LDNS_RR_TYPE_A, ldns.LDNS_RR_CLASS_IN)
   if pkt and pkt.answer():

Now we evaluate result, where two flags are crucial :

 * Return code
 * AD flag (authenticated)

When return code is `SERVFAIL`, it means that validating resolver marked requested
name as **bogus** (or bad configuration).

**AD** flag is set if domain name is authenticated **(secure)** or false if
it's insecure.

Complete source code
--------------------

 .. literalinclude:: ../../../examples/ldns-dnssec.py
    :language: python


Testing
-------

In order to get meaningful results, you have to enter IP address of validating
resolver or setup your own (see howto).

Execute `./example2.py` with options `domain name` and `resolver IP`,
example:

::

   user@localhost# ./example2.py www.dnssec.cz 127.0.0.1 # Secure (Configured Unbound running on localhost)
   user@localhost# ./example2.py www.rhybar.cz 127.0.0.1 # Bogus

Howto setup Unbound as validating resolver
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Install Unbound according to instructions.
Modify following options in `unbound.conf` (located in `/etc` or `/usr/local/etc`)/


Uncomment `module-config` and set `validator` before iterator.

::

   module-config: "validator iterator"

Download DLV keys and update path in `unbound.conf`::

   # DLV keys
   # Download from http://ftp.isc.org/www/dlv/dlv.isc.org.key
   dlv-anchor-file: "/usr/local/etc/unbound/dlv.isc.org.key"

Update trusted keys (`.cz` for example)::

   # Trusted keys
   # For current key, see www.dnssec.cz
   trusted-keys-file: "/usr/local/etc/unbound/trusted.key"
   
Now you should have well configured Unbound, so run it::

   user@localhost# unbound -dv
   
