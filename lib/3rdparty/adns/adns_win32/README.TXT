This is the ADNS resolver library for Windows, ported
by Jarle (jgaa) Aase, jgaa@jgaa.com

ADNS is not officially suppoted for Windows, but this
port provides a 100% native Windows DLL and linker
library for the DLL - suitable for traditional compilers 
and linkers under Windows. The library itself is ported to
Microsot Visual C++ 6.0 SP 2.

The library is tested under Windows 2000, but should work
with all versions from Windows NT 4 and up, and Windows98
and up. Windows95 is not supported.

If you want to use the library in your own C/C++ project,
you must set the following manifests before including adns.h:

#define ADNS_MAP_UNIXAPI
#define ADNS_DLL

ADNS_MAP_UNIXAPI will enable the Win32 port in the header-
files (else, they will use the defauts for a UNIX system)

ADNS_DLL will enable linking to the exported DLL functions.
Currently I have only made the DLL available. There are two 
reasons for this: 

  1) The DNS protocol is a mess - and it's difficult to make
     a foolproof implementation. If a security-problem is
	 found (or other bugs) - the fix can usually be deployed 
	 just by replacing the DLL. 

  2) The GPL/LGPL lisence of ADNS (see GPL-vs-LGPL) will allow
     you to use the .dll in your project - as far as I can tell.
	 A statically linked version will only be allowed in GPL
	 licensed projects.

For more information about this port, see http://adns.jgaa.com


