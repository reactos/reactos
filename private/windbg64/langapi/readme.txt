    IMPORTANT   August 19, 1994

This is the next generation of the LANGAPI project.
The macro _VC_VER is used to determine the LANGAPI version.
This macro has the value 300 for VC3.
CONDITIONALIZE ANY EDITS USING _VC_VER.
Make sure any new include file includes "..\include\vcver.h"

-Jan-

	IMPORTANT	May 21, 1993
	
	Please read the framework surrounding the LANGAPI project

	Contant Jan de Rie before making changes to this project.


The idea behind the LANGAPI project is to have a SMALL project, that
will fit painlessly on everyone's machine, which contains include
files that are shared between several projects.  We want to prevent
that several project include supposedly identical files, which
diverge over time.

No binary files are allowed in LANGAPI since the maintenance would
become prohibitive as well as the size.

The general include files, usable by anyone, will reside in the
LANGAPI\INCLUDE directory; these files are clean and do not require
any typenames to be defined.  More specialized include files, for
example the ones describing the compiler IL, or the ones used in the
debugger DLL's, are in separate subdirectories.  These files usually
assume that certain type names are present.


Please use the following convention in your makefiles when using the
LANGAPI project:

!ifndef LANGAPI
LANGAPI = \langapi
!endif

INCLUDES = ... -I$(LANGAPI)



It may be a good idea to keep track of the projects that use these
files, so that we can alert users if something bad happens.


LANGAPI\INCLUDE:

	cvinfo.h		compiler FE (\\ikura\slm\cxxfe)
				cvpack,
				debugger

	typesrvr.h		compiler FE (\\ikura\slm\cxxfe)
				cvpack, debugger


LANGAPI\IL:			compiler FE, IL dumpers(\\ikura\slm\cxxfe)
				IL converters


LANGAPI\DEBUGGER:

	
	

