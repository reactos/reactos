Guidelines for adding code to shlwapi.dll
-----------------------------------------

Shlwapi is considered to be the repository for shared utility code in IE.
We do not want it to be a dumping ground. Also, the lw in shlwapi indicates
that it is intended to be light weight. Hence follow these guidelines

1. Make sure that it is indeed a utility function - others will want to use it.
2. Document the utility function with comments in your source file.
3. Tell other groups about the existence of these new utility functions. 
Preferably, identify modules that can be trimmed by using these utilities and do 
the trimming yourself.
4. Defer all initialization for your API-Set until one of these APIs is first called.
We do not want the loading of shlwapi.dll to introduce a large burden on the shell.
