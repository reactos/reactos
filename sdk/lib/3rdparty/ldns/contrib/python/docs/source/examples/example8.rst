Signing of a zone file
===============================

This example shows how to sign the content of the given zone file

.. literalinclude:: ../../../examples/ldns-signzone.py
   :language: python

In order to be able sign a zone file, you have to generate a key-pair using ``ldns-keygen.py``. Don't forget to modify tag number.

Signing consists of three steps

1. In the first step, the content of a zone file is read and parsed. This can be done using :class:`ldns.ldns_zone` class.

2. In the second step, the private and public key is read and public key is inserted into zone (as DNSKEY). 

3. In the last step, the DNSSEC zone instance is created and all the RRs from zone file are copied here. Then, all the records are signed using :meth:`ldns.ldns_zone.sign` method. If the signing was successful, the content of DNSSEC zone is written to a file.
