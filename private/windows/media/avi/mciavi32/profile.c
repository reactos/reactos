/*
 * profile.c
 *
 * win32/win16 utility functions to read and write profile items
 * for multimedia tools
 */

#include <windows.h>
#include <windowsx.h>
#include <profile.key>

/*
 * read a UINT from the profile, or return default if
 * not found.
 */
#ifdef _WIN32
UINT mmGetProfileIntA(LPCSTR appname, LPCSTR valuename, INT uDefault)
{
    CHAR achName[MAX_PATH];
    HKEY hkey;
    DWORD dwType;
    INT value = uDefault;
    DWORD dwData;
    int cbData;

    lstrcpyA(achName, KEYNAMEA);
    lstrcatA(achName, appname);
    if (RegOpenKeyA(ROOTKEY, achName, &hkey) == ERROR_SUCCESS) {

        cbData = sizeof(dwData);
        if (RegQueryValueExA(
            hkey,
            (LPSTR)valuename,
            NULL,
            &dwType,
            (PBYTE) &dwData,
            &cbData) == ERROR_SUCCESS) {
                if (dwType == REG_DWORD) {
                    value = (INT)dwData;
                }
        }

        RegCloseKey(hkey);
    }

    return((UINT)value);
}

UINT
mmGetProfileInt(LPCTSTR appname, LPCTSTR valuename, INT uDefault)
{
    TCHAR achName[MAX_PATH];
    HKEY hkey;
    DWORD dwType;
    INT value = uDefault;
    DWORD dwData;
    int cbData;

    lstrcpy(achName, KEYNAME);
    lstrcat(achName, appname);
    if (RegOpenKey(ROOTKEY, achName, &hkey) == ERROR_SUCCESS) {

        cbData = sizeof(dwData);
        if (RegQueryValueEx(
            hkey,
            (LPTSTR)valuename,
            NULL,
            &dwType,
            (PBYTE) &dwData,
            &cbData) == ERROR_SUCCESS) {
                if (dwType == REG_DWORD) {
                    value = (INT)dwData;
                }
        }

        RegCloseKey(hkey);
    }

    return((UINT)value);
}
#endif


/*
 * write a UINT to the profile, if it is not the
 * same as the default or the value already there
 */
#ifdef _WIN32
VOID
mmWriteProfileInt(LPCTSTR appname, LPCTSTR valuename, INT Value)
{
    // If we would write the same as already there... return.
    if (mmGetProfileInt(appname, valuename, !Value) == (UINT)Value) {
        return;
    }

    {
        TCHAR achName[MAX_PATH];
        HKEY hkey;

        lstrcpy(achName, KEYNAME);
        lstrcat(achName, appname);
        if (RegCreateKey(ROOTKEY, achName, &hkey) == ERROR_SUCCESS) {
            RegSetValueEx(
                hkey,
                valuename,
                0,
                REG_DWORD,
                (PBYTE) &Value,
                sizeof(Value)
            );

            RegCloseKey(hkey);
        }
    }

}
#else
// For Win16 we use a macro and assume we have been passed a string value
//    char ach[12];
//
//    wsprintf(ach, "%d", Value);
//
//    WriteProfileString(
//        appname,
//        valuename,
//        ach);
}
#endif


/*
 * read a string from the profile into pResult.
 * result is number of bytes written into pResult
 */
#ifdef _WIN32
DWORD
mmGetProfileStringA(
    LPCSTR appname,
    LPCSTR valuename,
    LPCSTR pDefault,
    LPSTR pResult,
    int cbResult
)
{
    CHAR achName[MAX_PATH];
    HKEY hkey;
    DWORD dwType;

    lstrcpyA(achName, KEYNAMEA);
    lstrcatA(achName, appname);
    if (RegOpenKeyA(ROOTKEY, achName, &hkey) == ERROR_SUCCESS) {

        cbResult = cbResult * sizeof(TCHAR);
        if (RegQueryValueExA(
            hkey,
            (LPSTR)valuename,
            NULL,
            &dwType,
            (LPBYTE)pResult,
            &cbResult) == ERROR_SUCCESS) {

                if (dwType == REG_SZ) {
                    // cbResult is set to the size including null
                    RegCloseKey(hkey);
                    return(cbResult - 1);
                }
        }

        RegCloseKey(hkey);
    }

    // if we got here, we didn't find it, or it was the wrong type - return
    // the default string
    lstrcpyA(pResult, pDefault);
    return(lstrlenA(pDefault));
}

DWORD
mmGetProfileString(
    LPCTSTR appname,
    LPCTSTR valuename,
    LPCTSTR pDefault,
    LPTSTR pResult,
    int cbResult
)
{
    TCHAR achName[MAX_PATH];
    HKEY hkey;
    DWORD dwType;

    lstrcpy(achName, KEYNAME);
    lstrcat(achName, appname);
    if (RegOpenKey(ROOTKEY, achName, &hkey) == ERROR_SUCCESS) {

        cbResult = cbResult * sizeof(TCHAR);
        if (RegQueryValueEx(
            hkey,
            (LPTSTR)valuename,
            NULL,
            &dwType,
            (LPBYTE)pResult,
            &cbResult) == ERROR_SUCCESS) {

                if (dwType == REG_SZ) {
                    // cbResult is set to the size including null
                    RegCloseKey(hkey);
                    return(cbResult/sizeof(TCHAR) - 1);
                }
        }

        RegCloseKey(hkey);
    }

    // if we got here, we didn't find it, or it was the wrong type - return
    // the default string
    lstrcpy(pResult, pDefault);
    return(lstrlen(pDefault));
}
#endif


/*
 * write a string to the profile
 */
#ifdef _WIN32
VOID
mmWriteProfileString(LPCTSTR appname, LPCTSTR valuename, LPCTSTR pData)
{
    TCHAR achName[MAX_PATH];
    HKEY hkey;

    lstrcpy(achName, KEYNAME);
    lstrcat(achName, appname);
    if (RegCreateKey(ROOTKEY, achName, &hkey) == ERROR_SUCCESS) {
        if (pData) {
            RegSetValueEx(
                hkey,
                valuename,
                0,
                REG_SZ,
                (LPBYTE)pData,
                (lstrlen(pData) + 1) * sizeof(TCHAR)
            );
        } else {
            RegDeleteValue(
                hkey,
                (LPTSTR)valuename
            );
        }

        RegCloseKey(hkey);
    }
}
#endif

