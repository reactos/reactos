#include "provpch.h"
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
LPWSTR MakeWideStrFromAnsi
(
    LPSTR psz,
    BYTE  bType
)
{
    LPWSTR pwsz;
    int i;

    // arg checking.
    //
    if (!psz)
        return NULL;

    // compute the length of the required BSTR
    //
    i =  MultiByteToWideChar(CP_ACP, 0, psz, -1, NULL, 0);
    if (i <= 0) return NULL;

    // allocate the widestr, +1 for terminating null
    //
    switch (bType) {
    case STR_BSTR:
        pwsz = (LPWSTR) SysAllocStringLen(NULL, i-1); // SysAllocStringLen adds 1
        break;
    case STR_OLESTR:
        pwsz = (LPWSTR) CoTaskMemAlloc(i * sizeof(WCHAR));
        break;
    default:
        return(NULL);
    }

    if (!pwsz) return NULL;
    MultiByteToWideChar(CP_ACP, 0, psz, -1, pwsz, i);
    pwsz[i - 1] = 0;
    return pwsz;
}

int _purecall()
{
        DebugBreak();
        return 0;
}

extern "C" const int _fltused = 0;

//=---------------------------------------------------------------------------=
// overloaded new
//=---------------------------------------------------------------------------=
// for the retail case, we'll just use the win32 Local* heap management
// routines for speed and size
//
// Parameters:
//    size_t         - [in] what size do we alloc
//
// Output:
//    VOID *         - new memoery.
//
// Notes:
//
void * _cdecl operator new
(
    size_t    size
)
{
    return HeapAlloc(g_hHeap, 0, size);
}

//=---------------------------------------------------------------------------=
// overloaded delete
//=---------------------------------------------------------------------------=
// retail case just uses win32 Local* heap mgmt functions
//
// Parameters:
//    void *        - [in] free me!
//
// Notes:
//
void _cdecl operator delete ( void *ptr)
{
    HeapFree(g_hHeap, 0, ptr);
}

void * malloc(size_t n)
{
#ifdef _MALLOC_ZEROINIT
        void* p = HeapAlloc(g_hHeap, 0, n);
        if (p != NULL)
                memset(p, 0, n);
        return p;
#else
        return HeapAlloc(g_hHeap, 0, n);
#endif
}

void * calloc(size_t n, size_t s)
{
#ifdef _MALLOC_ZEROINIT
        return malloc(n * s);
#else
        void* p = malloc(n * s);
        if (p != NULL)
                memset(p, 0, n * s);
        return p;
#endif
}

void* realloc(void* p, size_t n)
{
        if (p == NULL)
                return malloc(n);

        return HeapReAlloc(g_hHeap, 0, p, n);
}

void free(void* p)
{
        if (p == NULL)
                return;

        HeapFree(g_hHeap, 0, p);
}



