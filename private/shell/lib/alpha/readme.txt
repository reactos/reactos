Import libraries in this folder are from NT 1381.  

These are for the use of components like shdocvw, which must be 
able to run in browser-only mode.  This assures we don't accidentally 
link to a more recent version of shell32 (the one in public\sdk\lib).

One exception is user32p.nt4.  It is actually from NT 1528, but it
has the right exports to serve for desknt4.cpl, which needs a private
export that has changed for NT5.