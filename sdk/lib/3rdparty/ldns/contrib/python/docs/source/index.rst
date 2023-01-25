PyLDNS documentation
=======================================

PyLDNS provides an `LDNS`_ wrapper (Python extension module) - the thinnest layer over the library possible.  Everything you can do from the C API, you can do from Python, but with less effort. The purpose of porting LDNS library to Python is to simplify DNS programming and usage of LDNS, however, still preserve the performance of this library as the speed represents the main benefit of LDNS. The proposed object approach allows the users to be concentrated at the essential part of application only and don't bother with deallocation of objects and so on.

.. _LDNS: http://www.nlnetlabs.nl/projects/ldns/

Contents
----------
.. toctree::
	:maxdepth: 2

	install.rst
	examples/index.rst
	modules/ldns

Indices and tables
-------------------

* :ref:`genindex`
* :ref:`search`

