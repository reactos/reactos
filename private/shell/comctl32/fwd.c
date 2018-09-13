//============================================================================
//
// 
//
//
//============================================================================

#include "ctlspriv.h"


LPSTR 
FwdStrChrA(
    LPCSTR lpStart, 
    WORD wMatch)
    {
    return StrChrA(lpStart, wMatch);
    }


LPWSTR 
FwdStrChrW(
    LPCWSTR lpStart, 
    WORD wMatch)
    {
    return StrChrW(lpStart, wMatch);
    }


LPSTR 
FwdStrRChrA(
    LPCSTR lpStart, 
    LPCSTR lpEnd, 
    WORD wMatch)
    {
    return StrRChrA(lpStart, lpEnd, wMatch);
    }

LPWSTR 
FwdStrRChrW(
    LPCWSTR lpStart, 
    LPCWSTR lpEnd, 
    WORD wMatch)
    {
    return StrRChrW(lpStart, lpEnd, wMatch);
    }


int 
FwdStrCmpNA(
    LPCSTR lpStr1, 
    LPCSTR lpStr2, 
    int nChar)
    {
    return StrCmpNA(lpStr1, lpStr2, nChar);
    }


int 
FwdStrCmpNW(
    LPCWSTR lpStr1, 
    LPCWSTR lpStr2, 
    int nChar)
    {
    return StrCmpNW(lpStr1, lpStr2, nChar);
    }


LPSTR 
FwdStrStrA(
    LPCSTR lpFirst, 
    LPCSTR lpSrch)
    {
    return StrStrA(lpFirst, lpSrch);
    }


LPWSTR 
FwdStrStrW(
    LPCWSTR lpFirst, 
    LPCWSTR lpSrch)
    {
    return StrStrW(lpFirst, lpSrch);
    }


int 
FwdStrCmpNIA(
    LPCSTR lpStr1, 
    LPCSTR lpStr2, 
    int nChar)
    {
    return StrCmpNIA(lpStr1, lpStr2, nChar);
    }


int 
FwdStrCmpNIW(
    LPCWSTR lpStr1, 
    LPCWSTR lpStr2, 
    int nChar)
    {
    return StrCmpNIW(lpStr1, lpStr2, nChar);
    }


LPSTR 
FwdStrStrIA(
    LPCSTR lpFirst, 
    LPCSTR lpSrch)
    {
    return StrStrIA(lpFirst, lpSrch);
    }


LPWSTR 
FwdStrStrIW(
    LPCWSTR lpFirst, 
    LPCWSTR lpSrch)
    {
    return StrStrIW(lpFirst, lpSrch);
    }


int 
FwdStrCSpnA(
    LPCSTR lpStr, 
    LPCSTR lpSet)
    {
    return StrCSpnA(lpStr, lpSet);
    }


int 
FwdStrCSpnW(
    LPCWSTR lpStr, 
    LPCWSTR lpSet)
    {
    return StrCSpnW(lpStr, lpSet);
    }


int 
FwdStrToIntA(
    LPCSTR lpSrc)
    {
    return StrToIntA(lpSrc);
    }


int 
FwdStrToIntW(
    LPCWSTR lpSrc)
    {
    return StrToIntW(lpSrc);
    }


LPSTR 
FwdStrChrIA(
    LPCSTR lpStart, 
    WORD wMatch)
    {
    return StrChrIA(lpStart, wMatch);
    }


LPWSTR 
FwdStrChrIW(
    LPCWSTR lpStart, 
    WORD wMatch)
    {
    return StrChrIW(lpStart, wMatch);
    }


LPSTR 
FwdStrRChrIA(
    LPCSTR lpStart, 
    LPCSTR lpEnd, 
    WORD wMatch)
    {
    return StrRChrIA(lpStart, lpEnd, wMatch);
    }


LPWSTR 
FwdStrRChrIW(
    LPCWSTR lpStart, 
    LPCWSTR lpEnd, 
    WORD wMatch)
    {
    return StrRChrIW(lpStart, lpEnd, wMatch);
    }


LPSTR 
FwdStrRStrIA(
    LPCSTR lpSource, 
    LPCSTR lpLast, 
    LPCSTR lpSrch)
    {
    return StrRStrIA(lpSource, lpLast, lpSrch);
    }


LPWSTR 
FwdStrRStrIW(
    LPCWSTR lpSource, 
    LPCWSTR lpLast, 
    LPCWSTR lpSrch)
    {
    return StrRStrIW(lpSource, lpLast, lpSrch);
    }


int 
FwdStrCSpnIA(
    LPCSTR lpStr, 
    LPCSTR lpSet)
    {
    return StrCSpnIA(lpStr, lpSet);
    }


int 
FwdStrCSpnIW(
    LPCWSTR lpStr, 
    LPCWSTR lpSet)
    {
    return StrCSpnIW(lpStr, lpSet);
    }


LPSTR
FwdStrPBrkA(
    LPCSTR psz, 
    LPCSTR pszSet)
    {
    return StrPBrkA(psz, pszSet);
    }


LPWSTR
FwdStrPBrkW(
    LPCWSTR psz, 
    LPCWSTR pszSet)
    {
    return StrPBrkW(psz, pszSet);
    }


int
FwdStrSpnA(
    LPCSTR psz,
    LPCSTR pszSet)
    {
    return StrSpnA(psz, pszSet);
    }


int
FwdStrSpnW(
    LPCWSTR psz,
    LPCWSTR pszSet)
    {
    return StrSpnW(psz, pszSet);
    }


BOOL 
FwdStrToIntExA(
    LPCSTR    pszString,
    DWORD     dwFlags,          // STIF_ bitfield 
    int FAR * piRet)
    {
    return StrToIntExA(pszString, dwFlags, piRet);
    }


BOOL 
FwdStrToIntExW(
    LPCWSTR   pszString,
    DWORD     dwFlags,          // STIF_ bitfield 
    int FAR * piRet)
    {
    return StrToIntExW(pszString, dwFlags, piRet);
    }


LPWSTR 
FwdStrCpyW(
    LPWSTR psz1, 
    LPCWSTR psz2)
    {
    return StrCpyW(psz1, psz2);
    }


LPSTR 
FwdStrDupA(
    LPCSTR lpsz)
    {
    return StrDupA(lpsz);
    }


LPWSTR 
FwdStrDupW(
    LPCWSTR lpsz)
    {
    return StrDupW(lpsz);
    }


int 
FwdStrCmpW(
    LPCWSTR lpsz1, 
    LPCWSTR lpsz2)
    {
    return StrCmpW(lpsz1, lpsz2);
    }
