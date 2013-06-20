This is the ADNS resolver library for Windows, ported
by Jarle (jgaa) Aase, jgaa@jgaa.com

ADNS is not officially suppoted for Windows, but this
port provides a 100% native Windows DLL and linker
library for the DLL - suitable for traditional compilers 
and linkers under Windows. The library itself is ported to
Microsot Visual C++ 6.0.

The library is tested under Windows 2000 and XP, but should work
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


///////////////////////////////////////////////////////

October 13th 2005 jgaa: adns-1.0-win32-05
  - Fixed a problem with the return-value from adns_inet_aton()
    Thanks to Gerald Combs for reporting the problem.

October 7th 2004 jgaa: adns-1.0-win32-03

 - Fixed a problem with error-messages when the program
   works off-line.
   Thanks to Ulf Lamping for pointing ourt and solving the problem.


April 4th 2004 jgaa: adns-1.0-win32-03

 - Fixed broken gettimeofday() function.

 - Fixed problem with TCP connections, where the librarry
   failed to connect to DNS servers with TCP, and flooded
   the servers with TCP connection attempts.
 
 - Made sure that errno was handled corrcetly after all network
   (winsock) calls.

 - Fixed a few places where noblocking calls were not handled
   EAGAIN and EWOULDBLOCK is not the same under Windows.



