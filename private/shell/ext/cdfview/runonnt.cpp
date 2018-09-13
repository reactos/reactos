#include "stdinc.h"

//============================================================================
// This file contains a bunch of Unicode/Ansi thunks to handle calling
// some internal functions that on Windows 95 the strings are Ansi,
// whereas the string on NT are unicode
//============================================================================

// First undefine everything that we are intercepting as to not forward back to us...
#undef PathCleanupSpec

#define THUNKMSG(psz)   TraceMsg(TF_THUNK, "shdv THUNK::%s", psz)


//
//  Now the thunks that allow us to run on Windows 95.
//
//
//
//  This thunks a unicode string to ANSI, but if it's an ordinal, then
//  we just leave it alone.
//

int _AorW_PathCleanupSpec(/*IN OPTIONAL*/ LPCTSTR pszDir, /*IN OUT*/ LPTSTR pszSpec)
{
    THUNKMSG(TEXT("PathCleanupSpec"));

    if (g_bRunningOnNT)
    {
        WCHAR wzDir[MAX_PATH];
        WCHAR wzSpec[MAX_PATH];
        LPWSTR pwszDir = wzDir;
        int iRet;

        if (pszDir)
            SHTCharToUnicode(pszDir, wzDir, ARRAYSIZE(wzDir));
        else
            pwszDir = NULL;

        SHTCharToUnicode(pszSpec, wzSpec, ARRAYSIZE(wzSpec));
        iRet = PathCleanupSpec((LPTSTR)pwszDir, (LPTSTR)wzSpec);

        SHUnicodeToTChar(wzSpec, pszSpec, MAX_PATH);
        return iRet;
    }
    else
    {
        CHAR szDir[MAX_PATH];
        CHAR szSpec[MAX_PATH];
        LPSTR pszDir2 = szDir;
        int iRet;

        if (pszDir)
            SHTCharToAnsi(pszDir, szDir, ARRAYSIZE(szDir));
        else
            pszDir2 = NULL;

        SHTCharToAnsi(pszSpec, szSpec, ARRAYSIZE(szSpec));
        iRet = PathCleanupSpec((LPTSTR)pszDir2, (LPTSTR)szSpec);

        SHAnsiToTChar(szSpec, pszSpec, MAX_PATH);
        return iRet;
    }
}

STDAPI Priv_SHDefExtractIcon(LPCTSTR pszIconFile, int iIndex, UINT uFlags,
                          HICON *phiconLarge, HICON *phiconSmall,
                          UINT nIconSize)
{
    HRESULT hr;
    ASSERT(uFlags == 0);

    //
    // W95 integrated mode supports SHDefExtractIcon.  This supports
    // matching the icon ectracted to the icon color depth.  ExtractIcon
    // doesn't.
    //
#ifndef UNIX
    if ((WhichPlatform() == PLATFORM_INTEGRATED))
    {
#ifdef UNICODE
        if (g_bRunningOnNT) 
        {
            return SHDefExtractIconW(pszIconFile, iIndex, uFlags,
                          phiconLarge, phiconSmall, nIconSize);
        } 
        else 
#endif
        {
            char szIconFile[MAX_PATH];
            SHUnicodeToAnsi(pszIconFile, szIconFile, ARRAYSIZE(szIconFile));
            hr = SHDefExtractIconA(szIconFile, iIndex, uFlags,
                          phiconLarge, phiconSmall, nIconSize);
        }
    }
    else
#endif /* !UNIX */
    {
        char szIconFile[MAX_PATH];
        SHUnicodeToAnsi(pszIconFile, szIconFile, ARRAYSIZE(szIconFile));
        hr = ExtractIconExA(szIconFile, iIndex, phiconLarge,
                           phiconSmall, 1) ? S_OK : E_FAIL;
    }

    return hr;
}
