/*
 * utility functions to read and write values to the profile,
 * using win.ini for Win16 or current user\software\microsoft\mciavi
 * in the registry for Win32
 */


#ifndef _WIN32
// For Win16 calls, mmWriteProfileInt passes a STRING for parm 3
//VOID mmWriteProfileInt(LPSTR appname, LPSTR valuename, UINT uValue);

#define mmWriteProfileInt(app, value, default) \
          WriteProfileString(app, value, (LPSTR)default)

#define mmGetProfileInt(app, value, default) \
          GetProfileInt(app, value, (LPSTR)default)

#define mmGetProfileIntA(app, value, default) \
          GetProfileInt(app, value, (LPSTR)default)

#define mmWriteProfileString(appname, valuename, pData) \
          WriteProfileString(appname, valuename, pData)

#define mmGetProfileString(appname, valuename, pDefault, pResult, cbResult) \
          GetProfileString(appname, valuename, pDefault, pResult, cbResult)

#define mmGetProfileStringA(appname, valuename, pDefault, pResult, cbResult) \
          GetProfileString(appname, valuename, pDefault, pResult, cbResult)

#else

/*
 * read a UINT from the profile, or return default if
 * not found.
 */
UINT mmGetProfileInt(LPTSTR appname, LPTSTR valuename, INT uDefault);
UINT mmGetProfileIntA(LPSTR appname, LPSTR valuename, INT uDefault);

/*
 * write an INT to the profile, if it is not the
 * same as the value already there
 */
VOID mmWriteProfileInt(LPTSTR appname, LPTSTR valuename, INT uValue);

/*
 * read a string from the profile into pResult.
 * result is number of bytes written into pResult
 */
DWORD
mmGetProfileString(
    LPTSTR appname,
    LPTSTR valuename,
    LPTSTR pDefault,
    LPTSTR pResult,
    int cbResult
);

DWORD
mmGetProfileStringA(
    LPSTR appname,
    LPSTR valuename,
    LPSTR pDefault,
    LPSTR pResult,
    int cbResult
);

/*
 * write a string to the profile
 */
VOID mmWriteProfileString(LPTSTR appname, LPTSTR valuename, LPTSTR pData);

#endif
