#include "stdafx.h"
#include "resource.h"
#include "initguid.h"

#include "util.h"
//=--------------------------------------------------------------------------=
// miscellaneous [useful] numerical constants
//=--------------------------------------------------------------------------=
// the length of a guid once printed out with -'s, leading and trailing bracket,
// plus 1 for NULL
//
#define GUID_STR_LEN    40

HANDLE g_hHeap;

//=--------------------------------------------------------------------------=
// DeleteKeyAndSubKeys
//=--------------------------------------------------------------------------=
// delete's a key and all of it's subkeys.
//
// Parameters:
//    HKEY                - [in] delete the descendant specified
//    LPSTR               - [in] i'm the descendant specified
//
// Output:
//    BOOL                - TRUE OK, FALSE baaaad.
//
// Notes:
//    - I don't feel too bad about implementing this recursively, since the
//      depth isn't likely to get all the great.
//    - RegDeleteKey() works recursivly on Win9x, but not NT
//
BOOL DeleteKeyAndSubKeys
(
    HKEY    hkIn,
    LPSTR   pszSubKey
)
{
    HKEY  hk;
    TCHAR szTmp[MAX_PATH];
    DWORD dwTmpSize;
    long  l;
    BOOL  f;
    int   x;

    l = RegOpenKeyEx(hkIn, pszSubKey, 0, KEY_ALL_ACCESS, &hk);
    if (l != ERROR_SUCCESS) return FALSE;

    // loop through all subkeys, blowing them away.
    //
    f = TRUE;
    x = 0;
    while (f) {
        dwTmpSize = MAX_PATH;
        l = RegEnumKeyEx(hk, x, szTmp, &dwTmpSize, 0, NULL, NULL, NULL);
        if (l != ERROR_SUCCESS) break;
        f = DeleteKeyAndSubKeys(hk, szTmp);
        x++;
    }

    // there are no subkeys left, [or we'll just generate an error and return FALSE].
    // let's go blow this dude away.
    //
    RegCloseKey(hk);
    l = RegDeleteKey(hkIn, pszSubKey);

    return (l == ERROR_SUCCESS) ? TRUE : FALSE;
}

//=--------------------------------------------------------------------------=
// StringFromGuid
//=--------------------------------------------------------------------------=
// returns an ANSI string from a CLSID or GUID
//
// Parameters:
//    REFIID               - [in]  clsid to make string out of.
//    LPSTR                - [in]  buffer in which to place resultant GUID.
//
// Output:
//    int                  - number of chars written out.
//
// Notes:
//
int StringFromGuid
(
    const CLSID*   piid,
    LPTSTR   pszBuf
)
{
    return wsprintf(pszBuf, TEXT("{%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}"), piid->Data1,
            piid->Data2, piid->Data3, piid->Data4[0], piid->Data4[1], piid->Data4[2],
            piid->Data4[3], piid->Data4[4], piid->Data4[5], piid->Data4[6], piid->Data4[7]);
}

// Copy WideStr to another on Win95

void CopyWideStr(LPWSTR pwszTarget, LPWSTR pwszSource)
{
   while(*pwszSource != 0)
   {
      *pwszTarget = *pwszSource;   
      pwszSource++;
      pwszTarget++;
   }
   *pwszTarget = 0;
}





//=--------------------------------------------------------------------------=
// MakeWideFromAnsi
//=--------------------------------------------------------------------------=
// given a string, make a BSTR out of it.
//
// Parameters:
//    LPSTR         - [in]
//    BYTE          - [in]
//
// Output:
//    LPWSTR        - needs to be cast to final desired result
//
// Notes:
//
LPWSTR MakeWideStrFromAnsi(LPSTR psz)
{
    LPWSTR pwsz;
    int i;

    // arg checking.
    //
    if (!psz) return NULL;

    // compute the length of the required BSTR
    //
    if ((i = MultiByteToWideChar(CP_ACP, 0, psz, -1, NULL, 0)) <= 0)
        return NULL;

    // allocate the widestr, +1 for terminating null
    //
    pwsz = (LPWSTR) CoTaskMemAlloc(i * sizeof(WCHAR));
    
    if (!pwsz) return NULL;
    MultiByteToWideChar(CP_ACP, 0, psz, -1, pwsz, i);
    pwsz[i - 1] = 0;
    return pwsz;
}

LPSTR MakeAnsiStrFromWide(LPWSTR pwsz)
{
    LPSTR psz;
    int i;

    // arg checking.
    //
    if (!pwsz)
        return NULL;

    // compute the length of the required BSTR
    //
    if ((i = WideCharToMultiByte(CP_ACP, 0, pwsz, -1, NULL, 0, NULL, NULL)) <= 0)
        return NULL;
    
    // allocate the ansistr, +1 for terminating null
    //
    psz = (LPSTR) CoTaskMemAlloc(i * sizeof(CHAR));
    if (!psz) return NULL;

    WideCharToMultiByte(CP_ACP, 0, pwsz, -1, psz, i, NULL, NULL);
    psz[i - 1] = 0;
    return psz;
}



