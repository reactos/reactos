Build instructions for NDIS library
-----------------------------------

 - Building of ndis is currently only supported with the mingw gcc compiler

 - from this directory:
    'make' to make the library
    'make clean' to clean it

 - from the top level reactos directory:
    'make ndis'

 - make must be run from the top level reactos directory to update the import
   library, unless you manually update it

-----------------------------
All of the below is outdated.
-----------------------------
Building with Visual C++ and Windows NT DDK:

Variables:
%BASEDIR%     = path to NT4 DDK (e.g. c:\ntddk)
%DDKBUILDENV% = DDK build environment (free or checked)

DDK environment variables must be set! (run setenv.bat)

    - Create the directory objects/i386/%DDKBUILDENV%
    - Run "build" to build the library


Building with Mingw32 and ReactOS include files:

    - Run "make ndis" FROM THE ReactOS ROOT DIRECTORY to build the library
