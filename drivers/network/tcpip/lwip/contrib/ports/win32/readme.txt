lwIP for Win32

This is an example port of lwIP for Win32. It uses WinPCAP to send & receive packets.
To compile it, use the MSVC projects in the 'msvc' subdir, the CMake files, or the Makefile
in the 'mingw' subdir.

For all compilers/build systems:
- you have to set an environment variable PCAP_DIR pointing to the WinPcap Developer's
  Pack (containing 'include' and 'lib')
  alternatively, place the WinPcap Developer's pack next to the "lwip" folder:
  "winpcap\WpdPack"

You also will have to copy the file 'contrib/examples/example_app/lwipcfg.h.example' to
'contrib/examples/example_app/lwipcfg.h' and modify to suit your needs (WinPcap adapter number,
IP configuration, applications...).

Included in the contrib\ports\win32 directory is the network interface driver
using the winpcap library.

lwIP: http://savannah.nongnu.org/projects/lwip/
WinPCap: https://www.winpcap.org/devel.htm
Visual C++: http://www.microsoft.com/express/download/

To compile the unittests (msvc\lwIP_unittests.sln), download check (tested with v0.11.0) from
https://github.com/libcheck/check/releases/
and place it in a folder "check" next to the "contrib" folder.
