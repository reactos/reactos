Installation
===================================

**Prerequisites**

SWIG 1.3 and GNU make are required to build modules for Python 2.4 and higher
(but lower than 3). In order to build modules for Python 3.2 or higher,
SWIG in version 2.0.4 or higher is required.

Note that Python 3.0 and 3.1 are not supported.

In order to build this documentation the Sphinx Python documentation generator
is required.

**Download**

The latest source codes can be downloaded from `here`_.

.. _here: http://nlnetlabs.nl/projects/ldns/

**Compiling**

After downloading the source code archive (this example uses
ldns-1.6.13.tar.gz), pyLDNS can be enabled and compiled by typing::

	> tar -xzf ldns-1.6.13.tar.gz
	> cd ldns-1.6.13
	> ./configure --with-pyldns
	> make

You need GNU make to compile pyLDNS; SWIG and Python development libraries to
compile the extension module.

**Selecting Target Python Interpreter**

By default, the pyLDNS module builds for the default Python interpreter (i.e.,
the Python interpreter which can be accessed by just typing ``python`` in
the command line). If you desire to build the pyLDNS module for a different
Python version then you must specify the desired Python version by setting
the ``PYTHON_VERSION`` variable during the configure phase::

	> PYTHON_VERSION=3.2 ./configure --with-pyldns
	> make

By default the pyLDNS compiles from sources for a single Python interpreter.
Remember to execute scripts requiring pyLDNS in those Python interpreters which
have pyLDNS installed.

**Testing**

If the compilation is successful, you can test the python LDNS extension module
by executing the commands::

	> cd contrib/python
	> make testenv
	> ./ldns-mx.py

Again, remember to use the Python interpreter version which the pyLDNS module
has been compiled with.

The commands will start a new shell, in which several symbolic links will be
set-up. When you exit the shell, then symbolic links will be deleted.

In ``contrib/python/examples`` several simple Python scripts utilising pyLDNS
can be found. These scripts demonstrate the capabilities of the LDNS library.

**Installation**

To install the libraries and it's extensions type::

	> cd ldns-1.6.13
	> make install
