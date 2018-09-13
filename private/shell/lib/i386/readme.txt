The SHELL32 import library in this folder is from NT 4.0 (1381).
This is used to link retail versions, since it has lego info in
it.

SHELL32.W95 is the private import library from Win95.  Link to
this for debug versions to verify the component will load on 
browser-only Win95 or NT platforms.

These are for the use of components like shdocvw, which must be 
able to run in browser-only mode.  This assures we don't accidentally 
link to a more recent version of shell32 (the one in public\sdk\lib).

Refer the accompanying def files (shelldef.nt and shelldef.w95) to see 
what exports are available on those platforms.


USER32P.NT4:  This is actually taken from NT 1528, but it has the
right exports needed for desknt4.cpl.  The exports changed in the 
real user32p.lib, so we need this to link to.

