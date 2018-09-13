/*
 * AVICAP32:
 *
 * utility functions to read and write values to the profile,
 * using win.ini for Win16/Win95 or current
 * the registry for Win32 NT.  (Trivial to change to registry for Win95)
 *
 * The only routine that AVICAP32 uses is GetProfileIntA
 */

#if defined(_WIN32) && defined(UNICODE)

/*
 * read a UINT from the profile, or return default if
 * not found.
 */
UINT mmGetProfileIntA(LPCSTR appname, LPCSTR valuename, INT uDefault);

// Now map all instances of GetProfileIntA to mmGetProfileIntA
#define GetProfileIntA mmGetProfileIntA

#endif
