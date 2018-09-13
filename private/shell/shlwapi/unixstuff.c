/*
 * AR: Varma: REVIEW:
 * File contains wrappers for few symbols in shlwapi.src, that were exported
 * as an alias to other symbols. Chose wrappers instead of macros as they are
 * exported as an alias.
 */

#include "priv.h"

BOOL IntlStrEqWorkerA(BOOL fCaseSens, LPCSTR lpString1, LPCSTR lpString2, int nChar) {
    return StrIsIntlEqualA(fCaseSens, lpString1, lpString2, nChar);
}

BOOL IntlStrEqWorkerW(BOOL fCaseSens, LPCWSTR lpString1, LPCWSTR lpString2, int nChar) {
    return StrIsIntlEqualW(fCaseSens, lpString1, lpString2, nChar);
}

STDAPI_(DWORD) SHDeleteOrphanKeyA( IN  HKEY    hkey, IN  LPCSTR  pszSubKey)
{
    return SHDeleteEmptyKeyA( hkey, pszSubKey );
}
 
STDAPI_(DWORD) SHDeleteOrphanKeyW( IN  HKEY    hkey, IN  LPCWSTR  pszSubKey)
{
    return SHDeleteEmptyKeyW( hkey, pszSubKey );
}
 
STDAPI_(BOOL) IsCharAlphaW(WCHAR wch) { return IsCharAlphaWrap(wch); }
STDAPI_(BOOL) IsCharAlphaNumericW(WCHAR wch) { return IsCharAlphaNumericWrap(wch); }
STDAPI_(BOOL) IsCharUpperW(WCHAR wch) { return IsCharUpperWrap(wch); }
STDAPI_(BOOL) IsCharLowerW(WCHAR wch) { return IsCharLowerWrap(wch); }


EXTERN_C HANDLE MapHandle(HANDLE hData, DWORD dwSource, DWORD dwDest, DWORD dwDesiredAccess, DWORD dwFlags)
{
    return SHMapHandle( hData, dwSource, dwDest, dwDesiredAccess, dwFlags );
}

int DrawTextExW(HDC hdc, LPWSTR lpchTextW, int cchTextW, LPRECT lprc, UINT dwDTFormat, LPDRAWTEXTPARAMS lpDTParams)
{
    int    iResult = 0;
    LPSTR  lpchTextA = NULL;
    int    cchTextA = -1;

    cchTextA = WideCharToMultiByte(CP_ACP, 0, lpchTextW, cchTextW, NULL, 0, NULL, NULL);
    ASSERT(cchTextA > 0);

    lpchTextA = (LPSTR) LocalAlloc(LPTR, cchTextA+1);
    if (!lpchTextA)
       goto cleanup;

    iResult = WideCharToMultiByte(CP_ACP, 0, lpchTextW, cchTextW, lpchTextA, cchTextA, NULL, NULL);

    if (iResult <= 0)
       goto cleanup;
    
    iResult = DrawTextExA(hdc, lpchTextA, cchTextA, lprc, dwDTFormat, lpDTParams);

cleanup:

    if (lpchTextA)
       LocalFree(lpchTextA);

    return iResult;
}

int SHAnsiToAnsiOld(LPCSTR pszSrc, LPSTR pszDst, int cchBuf)
{
   return SHAnsiToAnsi( pszSrc, pszDst, cchBuf );
}

int SHUnicodeToUnicodeOld(LPCWSTR pszSrc, LPWSTR pszDst, int cchBuf)
{
   return SHUnicodeToUnicode( pszSrc, pszDst, cchBuf );
}

// HtmlHelp Stubs.
HWND WINAPI HtmlHelpA( HWND hwndCaller, LPCSTR pszFile, UINT uCommand, DWORD dwData)
{
    MwNotYetImplemented("HtmlHelpA");
    return 0;
}

HWND WINAPI HtmlHelpW( HWND hwndCaller, LPCWSTR pszFile, UINT uCommand, DWORD dwData)
{
    MwNotYetImplemented("HtmlHelpW");
    return 0;
}

HWND MLHtmlHelpA(HWND hwndCaller, LPCSTR pszFile, UINT uCommand, DWORD dwData, DWORD dwCrossCodePage)
{
    MwNotYetImplemented("MLHtmlHelpA");
    return 0;
}

HWND MLHtmlHelpW(HWND hwndCaller, LPCWSTR pszFile, UINT uCommand, DWORD dwData, DWORD dwCrossCodePage)
{
    MwNotYetImplemented("MLHtmlHelpW");
    return 0;
}

LWSTDAPI SHCreateStreamOnFileAOld(LPCSTR pszFile, DWORD grfMode, IStream** ppstm)
{
    return SHCreateStreamOnFileA(pszFile, grfMode, ppstm);
}

LWSTDAPI SHCreateStreamOnFileWOld(LPCWSTR pwszFile, DWORD grfMode, IStream** ppstm)
{
    return SHCreateStreamOnFileW(pwszFile, grfMode, ppstm);
}
