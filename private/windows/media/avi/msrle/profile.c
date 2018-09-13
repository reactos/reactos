/*
 * MSRLE32:
 *
 * profile.c
 *
 * win32/win16 utility functions to read and write profile items
 * for VFW
 *
 * ONLY mmGetProfileIntA is supported here
 *
 */

#if defined(_WIN32) && defined(UNICODE)
// This whole file is only used for 32 bit code.  It is the implementation
// that allows Win GetProfilexxx calls to use the registry.

#include <windows.h>
#include <windowsx.h>

#include <profile.key>
#include <win32.h>
#include <stdlib.h>  // for atoi

#include "msrle.h"
#ifdef DEBUG
#include "profile.h"

static HKEY GetKeyA(LPCSTR appname, BOOL fCreate)
{
    HKEY key = 0;
    char achName[MAX_PATH];

    lstrcpyA(achName, KEYNAMEA);
    lstrcatA(achName, appname);

    if ((!fCreate && RegOpenKeyA(ROOTKEY, achName, &key) == ERROR_SUCCESS)
        || (fCreate && RegCreateKeyA(ROOTKEY, achName, &key) == ERROR_SUCCESS)) {
    }

    return(key);
}

#define GetKey GetKeyA

/*
 * read a UINT from the profile, or return default if
 * not found.
 */
UINT
mmGetProfileIntA(LPCSTR appname, LPCSTR valuename, INT uDefault)
{
    DWORD dwType;
    INT value = uDefault;
    DWORD dwData;
    int cbData;

    HKEY key = GetKeyA(appname, FALSE);

    if (key) {

        cbData = sizeof(dwData);
        if (RegQueryValueExA(
            key,
            (LPSTR)valuename,
            NULL,
            &dwType,
            (PBYTE) &dwData,
            &cbData) == ERROR_SUCCESS) {
            if (dwType == REG_DWORD || dwType == REG_BINARY) {
                value = (INT)dwData;
            } else if (dwType == REG_SZ) {
		value = atoi((LPSTR) &dwData);
	    }
	}

        RegCloseKey(key);
    }

    return((UINT)value);
}

#endif // DEBUG
#endif // defined(_WIN32) && defined(UNICODE)
