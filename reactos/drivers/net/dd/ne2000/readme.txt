Build instructions for NE2000 driver
------------------------------------

Building with Visual C++ and Windows NT DDK:

Variables:
%BASEDIR%     = path to NT4 DDK (e.g. c:\ntddk)
%DDKBUILDENV% = DDK build environment (free or checked)

DDK environment variables must be set! (run setenv.bat)

    - Create the directory objects/i386/%DDKBUILDENV%
    - Run "build" to build the driver


Building with Mingw32 and ReactOS include files:

    - Run "make ne2000" FROM THE ReactOS ROOT DIRECTORY to build the driver
