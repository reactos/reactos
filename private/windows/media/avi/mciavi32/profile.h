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
UINT mmGetProfileInt(LPCTSTR appname, LPCTSTR valuename, INT uDefault);
UINT mmGetProfileIntA(LPCSTR appname, LPCSTR valuename, INT uDefault);

/*
 * write an INT to the profile, if it is not the
 * same as the value already there
 */
VOID mmWriteProfileInt(LPCTSTR appname, LPCTSTR valuename, INT uValue);

/*
 * read a string from the profile into pResult.
 * result is number of bytes written into pResult
 */
DWORD
mmGetProfileString(
    LPCTSTR appname,
    LPCTSTR valuename,
    LPCTSTR pDefault,
    LPTSTR pResult,
    int cbResult
);

DWORD
mmGetProfileStringA(
    LPCSTR appname,
    LPCSTR valuename,
    LPCSTR pDefault,
    LPSTR pResult,
    int cbResult
);

/*
 * write a string to the profile
 */
VOID mmWriteProfileString(LPCTSTR appname, LPCTSTR valuename, LPCTSTR pData);

#endif
