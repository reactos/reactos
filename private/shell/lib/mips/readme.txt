Import libraries in this folder are from NT 1381.  

These are for the use of components like shdocvw, which must be 
able to run in browser-only mode.  This assures we don't accidentally 
link to a more recent version of shell32 (the one in public\sdk\lib).
