Since you have the source distribution, you will need to compile the
libraries before you can install them.

UNIX INSTRUCTIONS:
------------------

Go to the source directory you extracted (here). There is a script you
must run which will checks your system for certain features, so that the
library can be compiled on a variety of UNIX systems.  Run this script
by typing "./configure" in that directory.  After that, you can look at
the Makefile it made if you like, and then you run "make" to create the
"libncftp.a" and "libStrn.a" library files.

Finally, install the libraries and headers, by doing "make install".

View the libncftp.html file for the rest of the documentation.  An easy
way to do that is use a URL of file://Localhost/path/to/libncftp.html
with your favorite browser.


WINDOWS INSTRUCTIONS:
---------------------

You will need Visual C++ 6.0 or greater to build the library and sample
programs.  This version includes two supplementary libraries which you
must build and link with your applications: a string utility library
(Strn) and a Winsock utility library (sio).  Unlike the UNIX version of
LibNcFTP, where the library can be built with or without the sio library,
the sio library is required for the Windows version.

Keep the source hierarchy intact, so that the samples and libraries
build without problems.  Build the Strn library first, then the sio
library, and then the LibNcFTP library.  To do that, open the appropriate
.dsw file within Visual Studio, and then select "Rebuild All" from the
"Build" menu.  Be sure to build both the Debug and the Release versions
for each library.

When that is complete you should be able to build the sample programs,
which are in the libncftp\samples directory.

View the libncftp.html file for the rest of the documentation.  An easy
way to do that is use a URL of file://Localhost/path/to/libncftp.html
with your favorite browser.  Note that there may be UNIX-specific
instructions which you should ignore.

To build your own applications using LibNcFTP, you'll need to make sure
you configure your project to find the header files and library files.

Your application may not use the sio or Strn libraries directly, but
you still need to link with them.  For example, the "simpleget" sample
uses "..\..\Debug,..\..\..\Strn\Debug,..\..\..\sio\Debug" in the
project option for additional library paths for the linker, and
"kernel32.lib user32.lib gdi32.lib advapi32.lib shell32.lib
 ws2_32.lib Strn.lib sio.lib libncftp.lib" for the list of libraries
to link with.  Note that LibNcFTP uses advapi32 and shell32, in
addition to the usual "kernel32.lib user32.lib gdi32.lib".  Of course,
it also needs to link with Winsock (ws2_32.lib).

Similarly, you'll need to make sure one of your additional include
directories points to the LibNcFTP directory containing ncftp.h.  The
"simpleget" sample uses "..\.." since it is in a subdirectory of the
library itself.  If you actually use functions from Strn or sio (as
some of the samples do), you'll need to have your project look in
their directories for their headers as well.

About Winsock2:  This version of the library was designed for use with
Winsock version 2.  Note that older versions of Windows 95 do not include
Winsock version 2, but can be upgraded by getting the updater from
Microsoft:  http://www.microsoft.com/windows95/downloads/contents/wuadmintools/s_wunetworkingtools/w95sockets2/default.asp

However, the library should also work with Winsock 1.1.  That is left as
an exercise to the coder to change the Winsock initialization to use 1.1
and to link with the 1.1 library (wsock32.lib).