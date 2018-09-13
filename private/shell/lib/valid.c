//
//  Validation code
//

#include "proj.h"
#pragma  hdrstop

//
//  Validations
//

// Hack: size of the internal data structures, as defined in comctl32\da.c.
// We measure this by the number of DWORD-sized fields.
#ifdef DEBUG
#define CB_DSA      (6 * sizeof(DWORD))
#define CB_DPA      (6 * sizeof(DWORD))
#else
#define CB_DSA      (5 * sizeof(DWORD))
#define CB_DPA      (5 * sizeof(DWORD))
#endif

BOOL
IsValidHDSA(
    HDSA hdsa)
{
    return (IS_VALID_WRITE_BUFFER(hdsa, BYTE, CB_DSA));
}


BOOL
IsValidHDPA(
    HDPA hdpa)
{
    return (IS_VALID_WRITE_BUFFER(hdpa, BYTE, CB_DPA));
}

BOOL
IsValidPIDL(
    LPCITEMIDLIST pidl)
{
    return (IS_VALID_READ_PTR(pidl, USHORT) &&
            IS_VALID_READ_BUFFER((LPBYTE)pidl+sizeof(USHORT), BYTE, pidl->mkid.cb) &&
            (0 == _ILNext(pidl)->mkid.cb || IS_VALID_PIDL(_ILNext(pidl))));
}


BOOL 
IsValidHWND(
    HWND hwnd)
{
    /* Ask User if this is a valid window. */

    return(IsWindow(hwnd));
}


BOOL
IsValidHMENU(
    HMENU hmenu)
{
    return IsMenu(hmenu);
}    


BOOL 
IsValidHANDLE(
    HANDLE hnd)
{
    return(NULL != hnd && INVALID_HANDLE_VALUE != hnd);
}


BOOL 
IsValidHANDLE2(
    HANDLE hnd)
{
    return(hnd != INVALID_HANDLE_VALUE);
}


BOOL 
IsValidShowCmd(
    int nShow)
{
    BOOL bResult;
 
    switch (nShow)
    {
       case SW_HIDE:
       case SW_SHOWNORMAL:
       case SW_SHOWMINIMIZED:
       case SW_SHOWMAXIMIZED:
       case SW_SHOWNOACTIVATE:
       case SW_SHOW:
       case SW_MINIMIZE:
       case SW_SHOWMINNOACTIVE:
       case SW_SHOWNA:
       case SW_RESTORE:
       case SW_SHOWDEFAULT:
          bResult = TRUE;
          break;
 
       default:
          bResult = FALSE;
          TraceMsg(TF_ERROR, "IsValidShowCmd(): Invalid show command %d.",
                     nShow);
          break;
    }
 
    return(bResult);
}


BOOL 
IsValidPathA(
    LPCSTR pcszPath)
{
    return(IS_VALID_STRING_PTRA(pcszPath, MAX_PATH) &&
           EVAL((UINT)lstrlenA(pcszPath) < MAX_PATH));
}

BOOL 
IsValidPathW(
    LPCWSTR pcszPath)
{
    return(IS_VALID_STRING_PTRW(pcszPath, MAX_PATH) &&
           EVAL((UINT)lstrlenW(pcszPath) < MAX_PATH));
}


BOOL 
IsValidPathResultA(
    HRESULT hr, 
    LPCSTR pcszPath,
    UINT cchPathBufLen)
{
    return((hr == S_OK &&
            EVAL(IsValidPathA(pcszPath)) &&
            EVAL((UINT)lstrlenA(pcszPath) < cchPathBufLen)) ||
           (hr != S_OK &&
            EVAL(! cchPathBufLen ||
                 ! pcszPath ||
                 ! *pcszPath)));
}

BOOL 
IsValidPathResultW(
    HRESULT hr, 
    LPCWSTR pcszPath,
    UINT cchPathBufLen)
{
    return((hr == S_OK &&
            EVAL(IsValidPathW(pcszPath)) &&
            EVAL((UINT)lstrlenW(pcszPath) < cchPathBufLen)) ||
           (hr != S_OK &&
            EVAL(! cchPathBufLen ||
                 ! pcszPath ||
                 ! *pcszPath)));
}


BOOL 
IsValidExtensionA(
    LPCSTR pcszExt)
{
    return(IS_VALID_STRING_PTRA(pcszExt, MAX_PATH) &&
           EVAL(lstrlenA(pcszExt) < MAX_PATH) &&
           EVAL(*pcszExt == '.'));
}

BOOL 
IsValidExtensionW(
    LPCWSTR pcszExt)
{
    return(IS_VALID_STRING_PTRW(pcszExt, MAX_PATH) &&
           EVAL(lstrlenW(pcszExt) < MAX_PATH) &&
           EVAL(*pcszExt == TEXTW('.')));
}


BOOL 
IsValidIconIndexA(
    HRESULT hr, 
    LPCSTR pcszIconFile,
    UINT cchIconFileBufLen, 
    int niIcon)
{
    return(EVAL(IsValidPathResultA(hr, pcszIconFile, cchIconFileBufLen)) &&
           EVAL(hr == S_OK ||
                ! niIcon));
}

BOOL 
IsValidIconIndexW(
    HRESULT hr, 
    LPCWSTR pcszIconFile,
    UINT cchIconFileBufLen, 
    int niIcon)
{
    return(EVAL(IsValidPathResultW(hr, pcszIconFile, cchIconFileBufLen)) &&
           EVAL(hr == S_OK ||
                ! niIcon));
}


BOOL IsStringContainedA(LPCSTR pcszBigger, LPCSTR pcszSuffix)
{
    ASSERT(IS_VALID_STRING_PTRA(pcszBigger, -1));
    ASSERT(IS_VALID_STRING_PTRA(pcszSuffix, -1));
    
    return (pcszSuffix >= pcszBigger && 
            pcszSuffix <= pcszBigger + lstrlenA(pcszBigger));
}


BOOL IsStringContainedW(LPCWSTR pcszBigger, LPCWSTR pcszSuffix)
{
    ASSERT(IS_VALID_STRING_PTRW(pcszBigger, -1));
    ASSERT(IS_VALID_STRING_PTRW(pcszSuffix, -1));
    
    return (pcszSuffix >= pcszBigger && 
            pcszSuffix <= pcszBigger + lstrlenW(pcszBigger));
}

