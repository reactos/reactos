This file contains information about the status the MSVCRT runtime in ReactOS.
The sources for this runtime have been cobbled together from all sorts of places 
from around the 'net and as such it has its share of bugs.

Please note that all of the MSVCRT.DLL runtime sources are license GPL unless
otherwise noted. The sources from WINE are dual licensed GPL/LGPL. 
If you update a function in the ~/wine directory please send a patch to wine-patches@winehq.com

TODO List:
Implement the remaining functions that are commented out in the .def file
Update source code headers for the license information.
Compleate the W32API conversion for all source files.
Write a decent regression test suite.
Convert all C++ style comments to C style comments.
????

WINE Port Notes:
More cleanup
Disable of remove duplicate code. (When in doubt check the def)
