/*
 *  dbf.c
 *
 *  old registry stubs that call through to those now in kernel.
 *
 *  implement any compatibility hacks here.
 *
 *  currently we map the win 3.1 predefined keys (HKEY_CLASSES_ROOT)
 *  to the NT definitions (0x80000000).  real chicago apps should
 *  not call these APIs since they are now in kernel.
 *
 *  The old registry APIs returned ERROR_BADKEY for invalid HKEY passed
 *  in and used CANTOPEN only for unable to open registry file. The new
 *  NT compatile registry code uses BADKEY to indicate invalid subkey and
 *  CANTOPEN for invalid HKEY passed in. Since the only call where CANTOPEN
 *  in the old meaning could be returned from the new code is FLUSHKEY,
 *  we will always translate CANTOPEN to BADKEY.
 */

#include "shprv.h"
#include <winerror.h>

/* these are the error codes from Win 3.1 Windows.h */
#define MY_ERROR_BADDB             1L
#define MY_ERROR_BADKEY            2L
#define MY_ERROR_CANTOPEN          3L
#define MY_ERROR_CANTREAD          4L
#define MY_ERROR_CANTWRITE         5L
#define MY_ERROR_OUTOFMEMORY       6L
#define MY_ERROR_INVALID_PARAMETER 7L
#define MY_ERROR_ACCESS_DENIED     8L
/* end of win 3.1 registry error codes */

/*
 * RegErrorMap
 *	Win 3.1 compatibility error code mapping
 * 	Called after a VMM Registry call to map the Win32 Error code
 *		we get back from VMM to win 3.1 error codes for App 
 *		compatibility.
 *	Entry:	NewErrorCode 	WIN32 Registry error code
 *	Exit:	Win3.1 equivalent error code
 *
*/

LONG NEAR _fastcall RegErrorMap(LONG NewErrCode)
{
	// Successful -> no mapping 
	if (NewErrCode == ERROR_SUCCESS)
		return (ERROR_SUCCESS);

	/* map the new NT error code to old win 3.1 error */
	switch (NewErrCode) {

	    case ERROR_REGISTRY_CORRUPT:
	    case ERROR_BADDB:
		return(MY_ERROR_BADDB);

	    case ERROR_REGISTRY_IO_FAILED:
	    case ERROR_CANTWRITE:
		return(MY_ERROR_CANTWRITE);

	    case ERROR_BADKEY:
	    case ERROR_KEY_DELETED:
	    case ERROR_CANTOPEN:
            case ERROR_FILE_NOT_FOUND:
            case ERROR_NO_MORE_ITEMS:
		return(MY_ERROR_BADKEY);           

	    case ERROR_CANTREAD:
		return (MY_ERROR_CANTREAD);

	    case ERROR_INVALID_PARAMETER:
		return (MY_ERROR_INVALID_PARAMETER);


	    case ERROR_INSUFFICIENT_BUFFER:
	    case ERROR_MORE_DATA:
	    case ERROR_OUTOFMEMORY:
	    case ERROR_NOT_ENOUGH_MEMORY:         
		return(MY_ERROR_OUTOFMEMORY);

	    default:
		return(MY_ERROR_ACCESS_DENIED);
	}
}

static char g_szClasses[] = ".classes";

//  Very old applications may use an HKEY of 0 and specify a subkey starting
//  with .classes.  This is actually HKEY_CLASSES_ROOT, so we must strip off
//  the .classes to get at the subkey name.
//
//  NT's WOW32.DLL does the same thing (wshell.c).
LPCSTR NEAR _fastcall SkipClasses(LPCSTR lpSubKey)
{

    PSTR pStr;
    LPCSTR lpRealSubKey;

    if (HIWORD(lpSubKey) != NULL) {

        pStr = g_szClasses;
        lpRealSubKey = lpSubKey;

        while ((*pStr != '\0') &&
            ((char)(WORD)(DWORD)AnsiLower((LPSTR)(DWORD)(BYTE)*lpRealSubKey) ==
            *pStr)) {

            pStr++;
            lpRealSubKey++;

        }

        if (*pStr == '\0') {

            if (*lpRealSubKey == '\\')
                lpRealSubKey++;

            return lpRealSubKey;

        }

    }

    return lpSubKey;

}

LONG WINAPI MyRegOpenKey(HKEY hkey, LPCSTR lpSubKey, LPHKEY phkResult)
{
    switch (hkey) {
        case 0:     lpSubKey = SkipClasses(lpSubKey);
        case 1:     hkey = 0x80000000;
    }
    
    return RegErrorMap(RegOpenKey(hkey, lpSubKey, phkResult));
}

LONG WINAPI MyRegCreateKey(HKEY hkey, LPCSTR lpSubKey, LPHKEY phkResult)
{
    switch (hkey) {
        case 0:     lpSubKey = SkipClasses(lpSubKey);
        case 1:     hkey = 0x80000000;
    }
    
    return RegErrorMap(RegCreateKey(hkey, lpSubKey, phkResult));
}

LONG WINAPI MyRegDeleteKey(HKEY hkey, LPCSTR lpSubKey)
{
    //  B#6898(WIN95D):  Microsoft Office 4.2 and 4.3 delete tons of subkeys that
    //  they shouldn't touch during their uninstall (such as CLSID).  They
    //  somehow incorrectly link the fact that they didn't upgrade ole2.dll
    //  (stamped 2.1 in Win95, 2.01 in Win3.1) to the need to purge the registry
    //  of all ole2.reg items.
    //
    //  Because Win95 kinda needs the CLSID branch around for the shell to work,
    //  we add this special hack to check if the calling task is Acme setup and
    //  if the Office setup extension DLL is around.  If so, return access
    //  denied.  Their mssetup.dll doesn't check the return value, so any error
    //  code will do.
    if (hkey == 1) {
        char szModule[10];              //  eight chars+null+padding
        szModule[0] = '\0';             //  for safety.
        if (GetModuleName(GetCurrentTask(), szModule, sizeof(szModule)) &&
            (lstrcmp(szModule, "ACMSETUP") == 0) &&
            (GetModuleHandle("OFF40_BB") != NULL)) {
            return MY_ERROR_ACCESS_DENIED;
        }
    }

    switch (hkey) {
        case 0:     lpSubKey = SkipClasses(lpSubKey);
        case 1:     hkey = 0x80000000;
    }
    
    return RegErrorMap(RegDeleteKey(hkey, lpSubKey));
}

LONG WINAPI MyRegCloseKey(HKEY hkey)
{
    switch (hkey) {
        case 0:
        case 1:     hkey = 0x80000000;
    }
    
    return RegErrorMap(RegCloseKey(hkey));
}

LONG WINAPI MyRegQueryValue(HKEY hkey, LPCSTR lpSubKey, LPSTR lpValue, LONG FAR * lpcb)
{
    LONG     rc;
    DWORD    cbOriginalValue;
    LPBYTE   lpByte = NULL;

    // Fix MSTOOLBR.DLL unintialized cbValue by forcing it to be less than 64K
    // Win 3.1 Registry values are always less than 64K.
    cbOriginalValue = LOWORD(*lpcb);

    switch (hkey) {
        case 0:     lpSubKey = SkipClasses(lpSubKey);
        case 1:     hkey = 0x80000000;
    }

    rc = RegQueryValue(hkey, lpSubKey, lpValue, lpcb);

    //  In Win3.1, if you tried to query a value that wouldn't fit into the
    //  output buffer, then only the originally specified number of bytes would
    //  be copied and ERROR_SUCCESS would be returned.  WordPerfect 6.1 appears
    //  to have forgetten to reset 'lpcb' on their next call to RegQueryValue,
    //  so it is dependent on this fact (or else it will put up an error message
    //  box).
    if (rc == ERROR_MORE_DATA) {
        lpByte = (LPBYTE) MAKELP(GlobalAlloc(GPTR, *lpcb), 0);
        if (SELECTOROF(lpByte) == NULL) {
            return MY_ERROR_OUTOFMEMORY;
        }
        rc = RegQueryValue(hkey, lpSubKey, lpByte, lpcb);
        if (SELECTOROF(lpValue) && cbOriginalValue) {
            hmemcpy(lpValue, lpByte, cbOriginalValue);
            lpValue[cbOriginalValue - 1] = '\0';
        }
        GlobalFree(SELECTOROF(lpByte));
        *lpcb = cbOriginalValue;
    }

    return RegErrorMap(rc);
}

LONG WINAPI MyRegSetValue(HKEY hkey, LPCSTR lpSubKey, DWORD dwType, LPCSTR lpValue, DWORD cbValue)
{
    switch (hkey) {
        case 0:     lpSubKey = SkipClasses(lpSubKey);
        case 1:     hkey = 0x80000000;
    }

    //  In 3.1, the cbValue parameter really wasn't used... the registry code
    //  always calculated the size of the value as the string length of lpValue.
    return RegErrorMap(RegSetValue(hkey, lpSubKey, dwType, lpValue, 0));
}


LONG WINAPI MyRegEnumKey(HKEY hkey, DWORD dwIndex, LPSTR lpValue, DWORD dwMax)
{
    switch (hkey) {
        case 0:
        case 1:     hkey = 0x80000000;
    }
    
    return RegErrorMap(RegEnumKey(hkey,  dwIndex, lpValue, dwMax));
}
