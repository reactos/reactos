Build instructions for TCP/IP protocol driver
---------------------------------------------

Building with Visual C++ and Windows NT DDK:

Variables:
%BASEDIR%     = path to NT4 DDK (e.g. c:\ntddk)
%DDKBUILDENV% = DDK build environment (free or checked)

DDK environment variables must be set! (run setenv.bat)

    - Create the directory objects/i386/%DDKBUILDENV%
    - Run "build" to build the driver


Building with Mingw32 and ReactOS include files:

    - Build NDIS.SYS (i.e. "make ndis")
    - Run "make tcpip" FROM THE ReactOS ROOT DIRECTORY to build the driver
