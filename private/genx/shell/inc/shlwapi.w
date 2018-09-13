;begin_both
/*****************************************************************************\
*                                                                             *
* shlwapi.h - Interface for the Windows light-weight utility APIs             *
*                                                                             *
* Version 1.0                                                                 *
*                                                                             *
* Copyright (c) 1991-1998, Microsoft Corp.      All rights reserved.          *
*                                                                             *
\*****************************************************************************/

;end_both

#ifndef _INC_SHLWAPI
#define _INC_SHLWAPI
#ifndef _INC_SHLWAPIP                                                ;internal
#define _INC_SHLWAPIP                                                ;internal

;begin_both
#ifndef NOSHLWAPI

#include <objbase.h>
;end_both

#ifndef _WINRESRC_
#ifndef _WIN32_IE
#define _WIN32_IE 0x0501
#else
#if (_WIN32_IE < 0x0400) && defined(_WIN32_WINNT) && (_WIN32_WINNT >= 0x0500)
#error _WIN32_IE setting conflicts with _WIN32_WINNT setting
#endif
#endif
#endif


#ifdef UNIX
typedef interface IInternetSecurityMgrSite IInternetSecurityMgrSite;
typedef interface IInternetSecurityManager IInternetSecurityManager;
typedef interface IInternetHostSecurityManager IInternetHostSecurityManager;
#endif

//
// Define API decoration for direct importing of DLL references.
//
#ifndef WINSHLWAPI
#if !defined(_SHLWAPI_)
#define LWSTDAPI          EXTERN_C DECLSPEC_IMPORT HRESULT STDAPICALLTYPE
#define LWSTDAPI_(type)   EXTERN_C DECLSPEC_IMPORT type STDAPICALLTYPE
#define LWSTDAPIV         EXTERN_C DECLSPEC_IMPORT HRESULT STDAPIVCALLTYPE
#define LWSTDAPIV_(type)  EXTERN_C DECLSPEC_IMPORT type STDAPIVCALLTYPE
#else
#define LWSTDAPI          STDAPI
#define LWSTDAPI_(type)   STDAPI_(type)
#define LWSTDAPIV         STDAPIV
#define LWSTDAPIV_(type)  STDAPIV_(type)
#endif
#endif // WINSHLWAPI

;begin_both
#ifdef _WIN32
#include <pshpack1.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

//
// Users of this header may define any number of these constants to avoid
// the definitions of each functional group.
//
//    NO_SHLWAPI_STRFCNS    String functions
//    NO_SHLWAPI_PATH       Path functions
//    NO_SHLWAPI_REG        Registry functions
//    NO_SHLWAPI_UALSTR     Unaligned string functions                  ;internal
//    NO_SHLWAPI_STREAM     Stream functions
//    NO_SHLWAPI_HTTP       HTTP helper routines                        ;internal
//    NO_SHLWAPI_INTERNAL   Other random internal things                ;internal
//    NO_SHLWAPI_GDI        GDI helper functions
//    NO_SHLWAPI_UNITHUNK   Unicode wrapper functions                   ;internal
//    NO_SHLWAPI_TPS        Thread Pool Services                        ;internal
//    NO_SHLWAPI_MLUI       Multi Language UI functions                 ;internal


#ifndef NO_SHLWAPI_STRFCNS
//
//=============== String Routines ===================================
//

;end_both
LWSTDAPI_(LPSTR)    StrChrA(LPCSTR lpStart, WORD wMatch);
LWSTDAPI_(LPWSTR)   StrChrW(LPCWSTR lpStart, WCHAR wMatch);
LWSTDAPI_(LPSTR)    StrChrIA(LPCSTR lpStart, WORD wMatch);
LWSTDAPI_(LPWSTR)   StrChrIW(LPCWSTR lpStart, WCHAR wMatch);
LWSTDAPI_(int)      StrCmpNA(LPCSTR lpStr1, LPCSTR lpStr2, int nChar);
LWSTDAPI_(int)      StrCmpNW(LPCWSTR lpStr1, LPCWSTR lpStr2, int nChar);
LWSTDAPI_(int)      StrCmpNIA(LPCSTR lpStr1, LPCSTR lpStr2, int nChar);
LWSTDAPI_(int)      StrCmpNIW(LPCWSTR lpStr1, LPCWSTR lpStr2, int nChar);
LWSTDAPI_(int)      StrCSpnA(LPCSTR lpStr, LPCSTR lpSet);
LWSTDAPI_(int)      StrCSpnW(LPCWSTR lpStr, LPCWSTR lpSet);
LWSTDAPI_(int)      StrCSpnIA(LPCSTR lpStr, LPCSTR lpSet);
LWSTDAPI_(int)      StrCSpnIW(LPCWSTR lpStr, LPCWSTR lpSet);
LWSTDAPI_(LPSTR)    StrDupA(LPCSTR lpSrch);
LWSTDAPI_(LPWSTR)   StrDupW(LPCWSTR lpSrch);
LWSTDAPI_(LPSTR)    StrFormatByteSizeA(DWORD dw, LPSTR szBuf, UINT uiBufSize);
LWSTDAPI_(LPSTR)    StrFormatByteSize64A(LONGLONG qdw, LPSTR szBuf, UINT uiBufSize);
LWSTDAPI_(LPWSTR)   StrFormatByteSizeW(LONGLONG qdw, LPWSTR szBuf, UINT uiBufSize);
LWSTDAPI_(LPWSTR)   StrFormatKBSizeW(LONGLONG qdw, LPWSTR szBuf, UINT uiBufSize);
LWSTDAPI_(LPSTR)    StrFormatKBSizeA(LONGLONG qdw, LPSTR szBuf, UINT uiBufSize);
LWSTDAPI_(int)      StrFromTimeIntervalA(LPSTR pszOut, UINT cchMax, DWORD dwTimeMS, int digits);
LWSTDAPI_(int)      StrFromTimeIntervalW(LPWSTR pszOut, UINT cchMax, DWORD dwTimeMS, int digits);
LWSTDAPI_(BOOL)     StrIsIntlEqualA(BOOL fCaseSens, LPCSTR lpString1, LPCSTR lpString2, int nChar);
LWSTDAPI_(BOOL)     StrIsIntlEqualW(BOOL fCaseSens, LPCWSTR lpString1, LPCWSTR lpString2, int nChar);
LWSTDAPI_(LPSTR)    StrNCatA(LPSTR psz1, LPCSTR psz2, int cchMax);
LWSTDAPI_(LPWSTR)   StrNCatW(LPWSTR psz1, LPCWSTR psz2, int cchMax);
LWSTDAPI_(LPSTR)    StrPBrkA(LPCSTR psz, LPCSTR pszSet);
LWSTDAPI_(LPWSTR)   StrPBrkW(LPCWSTR psz, LPCWSTR pszSet);
LWSTDAPI_(LPSTR)    StrRChrA(LPCSTR lpStart, LPCSTR lpEnd, WORD wMatch);
LWSTDAPI_(LPWSTR)   StrRChrW(LPCWSTR lpStart, LPCWSTR lpEnd, WCHAR wMatch);
LWSTDAPI_(LPSTR)    StrRChrIA(LPCSTR lpStart, LPCSTR lpEnd, WORD wMatch);
LWSTDAPI_(LPWSTR)   StrRChrIW(LPCWSTR lpStart, LPCWSTR lpEnd, WCHAR wMatch);
LWSTDAPI_(LPSTR)    StrRStrIA(LPCSTR lpSource, LPCSTR lpLast, LPCSTR lpSrch);
LWSTDAPI_(LPWSTR)   StrRStrIW(LPCWSTR lpSource, LPCWSTR lpLast, LPCWSTR lpSrch);
LWSTDAPI_(int)      StrSpnA(LPCSTR psz, LPCSTR pszSet);
LWSTDAPI_(int)      StrSpnW(LPCWSTR psz, LPCWSTR pszSet);
LWSTDAPI_(LPSTR)    StrStrA(LPCSTR lpFirst, LPCSTR lpSrch);
LWSTDAPI_(LPWSTR)   StrStrW(LPCWSTR lpFirst, LPCWSTR lpSrch);
LWSTDAPI_(LPSTR)    StrStrIA(LPCSTR lpFirst, LPCSTR lpSrch);
LWSTDAPI_(LPWSTR)   StrStrIW(LPCWSTR lpFirst, LPCWSTR lpSrch);
LWSTDAPI_(int)      StrToIntA(LPCSTR lpSrc);
LWSTDAPI_(int)      StrToIntW(LPCWSTR lpSrc);
LWSTDAPI_(BOOL)     StrToIntExA(LPCSTR pszString, DWORD dwFlags, int * piRet);
LWSTDAPI_(BOOL)     StrToIntExW(LPCWSTR pszString, DWORD dwFlags, int * piRet);
LWSTDAPI_(BOOL)     StrTrimA(LPSTR psz, LPCSTR pszTrimChars);
LWSTDAPI_(BOOL)     StrTrimW(LPWSTR psz, LPCWSTR pszTrimChars);

LWSTDAPI_(LPWSTR)   StrCatW(LPWSTR psz1, LPCWSTR psz2);
LWSTDAPI_(int)      StrCmpW(LPCWSTR psz1, LPCWSTR psz2);
LWSTDAPI_(int)      StrCmpIW(LPCWSTR psz1, LPCWSTR psz2);
LWSTDAPI_(LPWSTR)   StrCpyW(LPWSTR psz1, LPCWSTR psz2);
LWSTDAPI_(LPWSTR)   StrCpyNW(LPWSTR psz1, LPCWSTR psz2, int cchMax);
;begin_internal
LWSTDAPI_(LPSTR)    StrCpyNXA(LPSTR psz1, LPCSTR psz2, int cchMax);
LWSTDAPI_(LPWSTR)   StrCpyNXW(LPWSTR psz1, LPCWSTR psz2, int cchMax);
;end_internal

LWSTDAPI_(LPWSTR)   StrCatBuffW(LPWSTR pszDest, LPCWSTR pszSrc, int cchDestBuffSize);
LWSTDAPI_(LPSTR)    StrCatBuffA(LPSTR pszDest, LPCSTR pszSrc, int cchDestBuffSize);

LWSTDAPI_(BOOL)     ChrCmpIA(WORD w1, WORD w2);
LWSTDAPI_(BOOL)     ChrCmpIW(WCHAR w1, WCHAR w2);

LWSTDAPI_(int)      wvnsprintfA(LPSTR lpOut, int cchLimitIn, LPCSTR lpFmt, va_list arglist);
LWSTDAPI_(int)      wvnsprintfW(LPWSTR lpOut, int cchLimitIn, LPCWSTR lpFmt, va_list arglist);
LWSTDAPIV_(int)     wnsprintfA(LPSTR lpOut, int cchLimitIn, LPCSTR lpFmt, ...);
LWSTDAPIV_(int)     wnsprintfW(LPWSTR lpOut, int cchLimitIn, LPCWSTR lpFmt, ...);

#define StrIntlEqNA( s1, s2, nChar) StrIsIntlEqualA( TRUE, s1, s2, nChar)
#define StrIntlEqNW( s1, s2, nChar) StrIsIntlEqualW( TRUE, s1, s2, nChar)
#define StrIntlEqNIA(s1, s2, nChar) StrIsIntlEqualA(FALSE, s1, s2, nChar)
#define StrIntlEqNIW(s1, s2, nChar) StrIsIntlEqualW(FALSE, s1, s2, nChar)

LWSTDAPI StrRetToStr%(struct _STRRET *pstr, const UNALIGNED struct _ITEMIDLIST *pidl, LPTSTR% *ppsz);
LWSTDAPI StrRetToBuf%(struct _STRRET *pstr, const UNALIGNED struct _ITEMIDLIST *pidl, LPTSTR% pszBuf, UINT cchBuf);

// helper to duplicate a string using the task allocator

LWSTDAPI SHStrDup%(LPCTSTR% psz, WCHAR **ppwsz);


;begin_internal

#define ORD_SHLOADREGUISTRINGA  438
#define ORD_SHLOADREGUISTRINGW  439
LWSTDAPI SHLoadRegUIString%(HKEY hkey, LPCTSTR% pszValue, LPTSTR% pszOutBuf, UINT cchOutBuf);

LWSTDAPI_(BOOL) IsCharCntrlW(WCHAR wch);
LWSTDAPI_(BOOL) IsCharDigitW(WCHAR wch);
LWSTDAPI_(BOOL) IsCharXDigitW(WCHAR wch);
LWSTDAPI_(BOOL) IsCharSpaceW(WCHAR wch);
LWSTDAPI_(BOOL) IsCharBlankW(WCHAR wch);
LWSTDAPI_(BOOL) IsCharPunctW(WCHAR wch);
LWSTDAPI_(BOOL) GetStringType3ExW( LPCWSTR, int, LPWORD );

// StrCmp*C* - Compare strings using C runtime collation rules.
// These functions are faster than the StrCmp family of functions
// above and can be used when the character set of the strings is
// known to be limited to seven ASCII character set.

LWSTDAPI_(int)  StrCmpNCA(LPCSTR lpStr1, LPCSTR lpStr2, int nChar);
LWSTDAPI_(int)  StrCmpNCW(LPCWSTR lpStr1, LPCWSTR lpStr2, int nChar);
LWSTDAPI_(int)  StrCmpNICA(LPCSTR lpStr1, LPCSTR lpStr2, int nChar);
LWSTDAPI_(int)  StrCmpNICW(LPCWSTR lpStr1, LPCWSTR lpStr2, int nChar);
LWSTDAPI_(int)  StrCmpCA(LPCSTR lpStr1, LPCSTR lpStr2);
LWSTDAPI_(int)  StrCmpCW(LPCWSTR lpStr1, LPCWSTR lpStr2);
LWSTDAPI_(int)  StrCmpICA(LPCSTR lpStr1, LPCSTR lpStr2);
LWSTDAPI_(int)  StrCmpICW(LPCWSTR lpStr1, LPCWSTR lpStr2);

// This is a true-Unicode version of CompareString.  It only supports
// STRING_SORT, however.  After better test coverage, it shall replace
// the CompareString Unicode wrapper itself.  In the mean time, we only
// call this from the find dialog/OM method of Trident.

LWSTDAPI_(int)  CompareStringAltW( LCID lcid, DWORD dwFlags, LPCWSTR lpchA, int cchA, LPCWSTR lpchB, int cchB );

;end_internal

#ifdef UNICODE
#define StrChr                  StrChrW
#define StrRChr                 StrRChrW
#define StrChrI                 StrChrIW
#define StrRChrI                StrRChrIW
#define StrCmpN                 StrCmpNW
#define StrCmpNI                StrCmpNIW
#define StrStr                  StrStrW
#define StrStrI                 StrStrIW
#define StrDup                  StrDupW
#define StrRStrI                StrRStrIW
#define StrCSpn                 StrCSpnW
#define StrCSpnI                StrCSpnIW
#define StrSpn                  StrSpnW
#define StrToInt                StrToIntW
#define StrPBrk                 StrPBrkW
#define StrToIntEx              StrToIntExW
#define StrFromTimeInterval     StrFromTimeIntervalW
#define StrIntlEqN              StrIntlEqNW
#define StrIntlEqNI             StrIntlEqNIW
#define StrFormatByteSize       StrFormatByteSizeW
#define StrFormatByteSize64     StrFormatByteSizeW
#define StrFormatKBSize         StrFormatKBSizeW
#define StrNCat                 StrNCatW
#define StrTrim                 StrTrimW
#define StrCatBuff              StrCatBuffW
#define ChrCmpI                 ChrCmpIW
#define wvnsprintf              wvnsprintfW
#define wnsprintf               wnsprintfW
#define StrIsIntlEqual          StrIsIntlEqualW

;begin_internal
//
// Macros for IsCharAlpha, IsCharAlphaNumeric, IsCharLower, IsCharUpper
// are in winuser.h
//
//

#define IsCharCntrl             IsCharCntrlW
#define IsCharDigit             IsCharDigitW
#define IsCharXDigit            IsCharXDigitW
#define IsCharSpace             IsCharSpaceW
#define IsCharBlank             IsCharBlankW
#define IsCharPunct             IsCharPunctW
#define GetStringType3Ex        GetStringType3ExW

;end_internal

#else
#define StrChr                  StrChrA
#define StrRChr                 StrRChrA
#define StrChrI                 StrChrIA
#define StrRChrI                StrRChrIA
#define StrCmpN                 StrCmpNA
#define StrCmpNI                StrCmpNIA
#define StrStr                  StrStrA
#define StrStrI                 StrStrIA
#define StrDup                  StrDupA
#define StrRStrI                StrRStrIA
#define StrCSpn                 StrCSpnA
#define StrCSpnI                StrCSpnIA
#define StrSpn                  StrSpnA
#define StrToInt                StrToIntA
#define StrPBrk                 StrPBrkA
#define StrToIntEx              StrToIntExA
#define StrFromTimeInterval     StrFromTimeIntervalA
#define StrIntlEqN              StrIntlEqNA
#define StrIntlEqNI             StrIntlEqNIA
#define StrFormatByteSize       StrFormatByteSizeA
#define StrFormatByteSize64     StrFormatByteSize64A
#define StrFormatKBSize         StrFormatKBSizeA
#define StrNCat                 StrNCatA
#define StrTrim                 StrTrimA
#define StrCatBuff              StrCatBuffA
#define ChrCmpI                 ChrCmpIA
#define wvnsprintf              wvnsprintfA
#define wnsprintf               wnsprintfA
#define StrIsIntlEqual          StrIsIntlEqualA
#endif

;begin_internal

#ifdef UNICODE

#define StrCmpNC                StrCmpNCW
#define StrCmpNIC               StrCmpNICW
#define StrCmpC                 StrCmpCW
#define StrCmpIC                StrCmpICW
#define StrCpyNX                StrCpyNXW

#else

#define StrCmpNC                StrCmpNCA
#define StrCmpNIC               StrCmpNICA
#define StrCmpC                 StrCmpCA
#define StrCmpIC                StrCmpICA
#define StrCpyNX                StrCpyNXA

#endif

;end_internal

// Backward compatible to NT's non-standard naming (strictly
// for comctl32)
//
LWSTDAPI_(BOOL)     IntlStrEqWorkerA(BOOL fCaseSens, LPCSTR lpString1, LPCSTR lpString2, int nChar);
LWSTDAPI_(BOOL)     IntlStrEqWorkerW(BOOL fCaseSens, LPCWSTR lpString1, LPCWSTR lpString2, int nChar);

#define IntlStrEqNA( s1, s2, nChar) IntlStrEqWorkerA( TRUE, s1, s2, nChar)
#define IntlStrEqNW( s1, s2, nChar) IntlStrEqWorkerW( TRUE, s1, s2, nChar)
#define IntlStrEqNIA(s1, s2, nChar) IntlStrEqWorkerA(FALSE, s1, s2, nChar)
#define IntlStrEqNIW(s1, s2, nChar) IntlStrEqWorkerW(FALSE, s1, s2, nChar)

#ifdef UNICODE
#define IntlStrEqN              IntlStrEqNW
#define IntlStrEqNI             IntlStrEqNIW
#else
#define IntlStrEqN              IntlStrEqNA
#define IntlStrEqNI             IntlStrEqNIA
#endif

#define SZ_CONTENTTYPE_HTMLA       "text/html"
#define SZ_CONTENTTYPE_HTMLW       L"text/html"
#define SZ_CONTENTTYPE_CDFA        "application/x-cdf"
#define SZ_CONTENTTYPE_CDFW        L"application/x-cdf"

#ifdef UNICODE
#define SZ_CONTENTTYPE_HTML     SZ_CONTENTTYPE_HTMLW
#define SZ_CONTENTTYPE_CDF      SZ_CONTENTTYPE_CDFW
#else
#define SZ_CONTENTTYPE_HTML     SZ_CONTENTTYPE_HTMLA
#define SZ_CONTENTTYPE_CDF      SZ_CONTENTTYPE_CDFA
#endif

#define PathIsHTMLFileA(pszPath)     PathIsContentTypeA(pszPath, SZ_CONTENTTYPE_HTMLA)
#define PathIsHTMLFileW(pszPath)     PathIsContentTypeW(pszPath, SZ_CONTENTTYPE_HTMLW)

// Flags for StrToIntEx
#define STIF_DEFAULT        0x00000000L
#define STIF_SUPPORT_HEX    0x00000001L


#define StrCatA                 lstrcatA
#define StrCmpA                 lstrcmpA
#define StrCmpIA                lstrcmpiA
#define StrCpyA                 lstrcpyA
#define StrCpyNA                lstrcpynA


#define StrToLong               StrToInt
#define StrNCmp                 StrCmpN
#define StrNCmpI                StrCmpNI
#define StrNCpy                 StrCpyN
#define StrCatN                 StrNCat

#ifdef UNICODE
#define StrCat                  StrCatW
#define StrCmp                  StrCmpW
#define StrCmpI                 StrCmpIW
#define StrCpy                  StrCpyW
#define StrCpyN                 StrCpyNW
#define StrCatBuff              StrCatBuffW
#else
#define StrCat                  lstrcatA
#define StrCmp                  lstrcmpA
#define StrCmpI                 lstrcmpiA
#define StrCpy                  lstrcpyA
#define StrCpyN                 lstrcpynA
#define StrCatBuff              StrCatBuffA
#endif

;begin_both

#endif //  NO_SHLWAPI_STRFCNS

;end_both
;begin_both

#ifndef NO_SHLWAPI_PATH

//
//=============== Path Routines ===================================
//

;end_both
LWSTDAPI_(LPTSTR%)  PathAddBackslash%(LPTSTR% pszPath);
LWSTDAPI_(BOOL)     PathAddExtension%(LPTSTR% pszPath, LPCTSTR% pszExt);
LWSTDAPI_(BOOL)     PathAppendA(LPSTR pszPath, LPCSTR pMore);
LWSTDAPI_(BOOL)     PathAppendW(LPWSTR pszPath, LPCWSTR pMore);
LWSTDAPI_(LPTSTR%)  PathBuildRoot%(LPTSTR% pszRoot, int iDrive);
LWSTDAPI_(BOOL)     PathCanonicalizeA(LPSTR pszBuf, LPCSTR pszPath);
LWSTDAPI_(BOOL)     PathCanonicalizeW(LPWSTR pszBuf, LPCWSTR pszPath);
LWSTDAPI_(LPTSTR%)  PathCombine%(LPTSTR% pszDest, LPCTSTR% pszDir, LPCTSTR% pszFile);
LWSTDAPI_(BOOL)     PathCompactPathA(HDC hDC, LPSTR pszPath, UINT dx);
LWSTDAPI_(BOOL)     PathCompactPathW(HDC hDC, LPWSTR pszPath, UINT dx);
LWSTDAPI_(BOOL)     PathCompactPathExA(LPSTR pszOut, LPCSTR pszSrc, UINT cchMax, DWORD dwFlags);
LWSTDAPI_(BOOL)     PathCompactPathExW(LPWSTR pszOut, LPCWSTR pszSrc, UINT cchMax, DWORD dwFlags);
LWSTDAPI_(int)      PathCommonPrefixA(LPCSTR pszFile1, LPCSTR pszFile2, LPSTR achPath);
LWSTDAPI_(int)      PathCommonPrefixW(LPCWSTR pszFile1, LPCWSTR pszFile2, LPWSTR achPath);
LWSTDAPI_(BOOL)     PathFileExists%(LPCTSTR% pszPath);
LWSTDAPI_(LPTSTR%)  PathFindExtension%(LPCTSTR% pszPath);
LWSTDAPI_(LPTSTR%)  PathFindFileName%(LPCTSTR% pszPath);
LWSTDAPI_(LPTSTR%)  PathFindNextComponent%(LPCTSTR% pszPath);
LWSTDAPI_(BOOL)     PathFindOnPathA(LPSTR pszPath, LPCSTR * ppszOtherDirs);
LWSTDAPI_(BOOL)     PathFindOnPathW(LPWSTR pszPath, LPCWSTR * ppszOtherDirs);
LWSTDAPI_(LPTSTR%)  PathGetArgs%(LPCTSTR% pszPath);
LWSTDAPI_(LPCTSTR%) PathFindSuffixArray%(LPCTSTR% pszPath, const LPCTSTR% *apszSuffix, int iArraySize);
STDAPI_(BOOL)       PathIsLFNFileSpec%(LPCTSTR% lpName);

LWSTDAPI_(UINT)     PathGetCharTypeA(UCHAR ch);
LWSTDAPI_(UINT)     PathGetCharTypeW(WCHAR ch);

// Return flags for PathGetCharType
#define GCT_INVALID             0x0000
#define GCT_LFNCHAR             0x0001
#define GCT_SHORTCHAR           0x0002
#define GCT_WILD                0x0004
#define GCT_SEPARATOR           0x0008

LWSTDAPI_(int)      PathGetDriveNumber%(LPCTSTR% pszPath);
LWSTDAPI_(BOOL)     PathIsDirectory%(LPCTSTR% pszPath);
LWSTDAPI_(BOOL)     PathIsDirectoryEmpty%(LPCTSTR% pszPath);
LWSTDAPI_(BOOL)     PathIsFileSpec%(LPCTSTR% pszPath);
LWSTDAPI_(BOOL)     PathIsPrefix%(LPCTSTR% pszPrefix, LPCTSTR% pszPath);
LWSTDAPI_(BOOL)     PathIsRelative%(LPCTSTR% pszPath);
LWSTDAPI_(BOOL)     PathIsRoot%(LPCTSTR% pszPath);
LWSTDAPI_(BOOL)     PathIsSameRoot%(LPCTSTR% pszPath1, LPCTSTR% pszPath2);
LWSTDAPI_(BOOL)     PathIsUNC%(LPCTSTR% pszPath);
LWSTDAPI_(BOOL)     PathIsNetworkPath%(LPCTSTR% pszPath);
LWSTDAPI_(BOOL)     PathIsUNCServer%(LPCTSTR% pszPath);
LWSTDAPI_(BOOL)     PathIsUNCServerShare%(LPCTSTR% pszPath);
LWSTDAPI_(BOOL)     PathIsContentTypeA(LPCSTR pszPath, LPCSTR pszContentType);
LWSTDAPI_(BOOL)     PathIsContentTypeW(LPCWSTR pszPath, LPCWSTR pszContentType);
LWSTDAPI_(BOOL)     PathIsURL%(LPCTSTR% pszPath);
LWSTDAPI_(BOOL)     PathMakePrettyA(LPSTR pszPath);
LWSTDAPI_(BOOL)     PathMakePrettyW(LPWSTR pszPath);
LWSTDAPI_(BOOL)     PathMatchSpecA(LPCSTR pszFile, LPCSTR pszSpec);
LWSTDAPI_(BOOL)     PathMatchSpecW(LPCWSTR pszFile, LPCWSTR pszSpec);
LWSTDAPI_(int)      PathParseIconLocationA(LPSTR pszIconFile);
LWSTDAPI_(int)      PathParseIconLocationW(LPWSTR pszIconFile);
LWSTDAPI_(void)     PathQuoteSpacesA(LPSTR lpsz);
LWSTDAPI_(void)     PathQuoteSpacesW(LPWSTR lpsz);
LWSTDAPI_(BOOL)     PathRelativePathToA(LPSTR pszPath, LPCSTR pszFrom, DWORD dwAttrFrom, LPCSTR pszTo, DWORD dwAttrTo);
LWSTDAPI_(BOOL)     PathRelativePathToW(LPWSTR pszPath, LPCWSTR pszFrom, DWORD dwAttrFrom, LPCWSTR pszTo, DWORD dwAttrTo);
LWSTDAPI_(void)     PathRemoveArgsA(LPSTR pszPath);
LWSTDAPI_(void)     PathRemoveArgsW(LPWSTR pszPath);
LWSTDAPI_(LPTSTR%)  PathRemoveBackslash%(LPTSTR% pszPath);
LWSTDAPI_(void)     PathRemoveBlanksA(LPSTR pszPath);
LWSTDAPI_(void)     PathRemoveBlanksW(LPWSTR pszPath);
LWSTDAPI_(void)     PathRemoveExtensionA(LPSTR pszPath);
LWSTDAPI_(void)     PathRemoveExtensionW(LPWSTR pszPath);
LWSTDAPI_(BOOL)     PathRemoveFileSpecA(LPSTR pszPath);
LWSTDAPI_(BOOL)     PathRemoveFileSpecW(LPWSTR pszPath);
LWSTDAPI_(BOOL)     PathRenameExtensionA(LPSTR pszPath, LPCSTR pszExt);
LWSTDAPI_(BOOL)     PathRenameExtensionW(LPWSTR pszPath, LPCWSTR pszExt);
LWSTDAPI_(BOOL)     PathSearchAndQualifyA(LPCSTR pszPath, LPSTR pszBuf, UINT cchBuf);
LWSTDAPI_(BOOL)     PathSearchAndQualifyW(LPCWSTR pszPath, LPWSTR pszBuf, UINT cchBuf);
LWSTDAPI_(void)     PathSetDlgItemPathA(HWND hDlg, int id, LPCSTR pszPath);
LWSTDAPI_(void)     PathSetDlgItemPathW(HWND hDlg, int id, LPCWSTR pszPath);
LWSTDAPI_(LPTSTR%)  PathSkipRoot%(LPCTSTR% pszPath);
LWSTDAPI_(void)     PathStripPath%(LPTSTR% pszPath);
LWSTDAPI_(BOOL)     PathStripToRoot%(LPTSTR% pszPath);
LWSTDAPI_(void)     PathUnquoteSpacesA(LPSTR lpsz);
LWSTDAPI_(void)     PathUnquoteSpacesW(LPWSTR lpsz);
LWSTDAPI_(BOOL)     PathMakeSystemFolder%(LPCTSTR% pszPath);
LWSTDAPI_(BOOL)     PathUnmakeSystemFolder%(LPCTSTR% pszPath);
LWSTDAPI_(BOOL)     PathIsSystemFolder%(LPCTSTR% pszPath, DWORD dwAttrb);
LWSTDAPI_(void)     PathUndecorate%(LPTSTR% pszPath);
LWSTDAPI_(BOOL)     PathUnExpandEnvStrings%(LPCTSTR% pszPath, LPTSTR% pszBuf, UINT cchBuf);

;begin_internal

#if (_WIN32_IE >= 0x0501)

LWSTDAPI_(BOOL)     PathUnExpandEnvStringsForUser%(HANDLE hToken, LPCTSTR% pszPath, LPTSTR% pszBuf, UINT cchBuf);

#endif // (_WIN32_IE >= 0x0501)

;end_internal

#ifdef UNICODE
#define PathAppend              PathAppendW
#define PathCanonicalize        PathCanonicalizeW
#define PathCompactPath         PathCompactPathW
#define PathCompactPathEx       PathCompactPathExW
#define PathCommonPrefix        PathCommonPrefixW
#define PathFindOnPath          PathFindOnPathW
#define PathGetCharType         PathGetCharTypeW
#define PathIsContentType       PathIsContentTypeW
#define PathIsHTMLFile          PathIsHTMLFileW
#define PathMakePretty          PathMakePrettyW
#define PathMatchSpec           PathMatchSpecW
#define PathParseIconLocation   PathParseIconLocationW
#define PathQuoteSpaces         PathQuoteSpacesW
#define PathRelativePathTo      PathRelativePathToW
#define PathRemoveArgs          PathRemoveArgsW
#define PathRemoveBlanks        PathRemoveBlanksW
#define PathRemoveExtension     PathRemoveExtensionW
#define PathRemoveFileSpec      PathRemoveFileSpecW
#define PathRenameExtension     PathRenameExtensionW
#define PathSearchAndQualify    PathSearchAndQualifyW
#define PathSetDlgItemPath      PathSetDlgItemPathW
#define PathUnquoteSpaces       PathUnquoteSpacesW
#else
#define PathAppend              PathAppendA
#define PathCanonicalize        PathCanonicalizeA
#define PathCompactPath         PathCompactPathA
#define PathCompactPathEx       PathCompactPathExA
#define PathCommonPrefix        PathCommonPrefixA
#define PathFindOnPath          PathFindOnPathA
#define PathGetCharType         PathGetCharTypeA
#define PathIsContentType       PathIsContentTypeA
#define PathIsHTMLFile          PathIsHTMLFileA
#define PathMakePretty          PathMakePrettyA
#define PathMatchSpec           PathMatchSpecA
#define PathParseIconLocation   PathParseIconLocationA
#define PathQuoteSpaces         PathQuoteSpacesA
#define PathRelativePathTo      PathRelativePathToA
#define PathRemoveArgs          PathRemoveArgsA
#define PathRemoveBlanks        PathRemoveBlanksA
#define PathRemoveExtension     PathRemoveExtensionA
#define PathRemoveFileSpec      PathRemoveFileSpecA
#define PathRenameExtension     PathRenameExtensionA
#define PathSearchAndQualify    PathSearchAndQualifyA
#define PathSetDlgItemPath      PathSetDlgItemPathA
#define PathUnquoteSpaces       PathUnquoteSpacesA
#endif

typedef enum {
    URL_SCHEME_INVALID     = -1,
    URL_SCHEME_UNKNOWN     =  0,
    URL_SCHEME_FTP,
    URL_SCHEME_HTTP,
    URL_SCHEME_GOPHER,
    URL_SCHEME_MAILTO,
    URL_SCHEME_NEWS,
    URL_SCHEME_NNTP,
    URL_SCHEME_TELNET,
    URL_SCHEME_WAIS,
    URL_SCHEME_FILE,
    URL_SCHEME_MK,
    URL_SCHEME_HTTPS,
    URL_SCHEME_SHELL,
    URL_SCHEME_SNEWS,
    URL_SCHEME_LOCAL,
    URL_SCHEME_JAVASCRIPT,
    URL_SCHEME_VBSCRIPT,
    URL_SCHEME_ABOUT,
    URL_SCHEME_RES,
    URL_SCHEME_MAXVALUE
} URL_SCHEME;

typedef enum {
    URL_PART_NONE       = 0,
    URL_PART_SCHEME     = 1,
    URL_PART_HOSTNAME,
    URL_PART_USERNAME,
    URL_PART_PASSWORD,
    URL_PART_PORT,
    URL_PART_QUERY,
} URL_PART;

typedef enum {
    URLIS_URL,
    URLIS_OPAQUE,
    URLIS_NOHISTORY,
    URLIS_FILEURL,
    URLIS_APPLIABLE,
    URLIS_DIRECTORY,
    URLIS_HASQUERY,
} URLIS;

#define URL_UNESCAPE                    0x10000000
#define URL_ESCAPE_UNSAFE               0x20000000
#define URL_PLUGGABLE_PROTOCOL          0x40000000
#define URL_WININET_COMPATIBILITY       0x80000000
#define URL_DONT_ESCAPE_EXTRA_INFO      0x02000000
#define URL_DONT_UNESCAPE_EXTRA_INFO    URL_DONT_ESCAPE_EXTRA_INFO
#define URL_BROWSER_MODE                URL_DONT_ESCAPE_EXTRA_INFO
#define URL_ESCAPE_SPACES_ONLY          0x04000000
#define URL_DONT_SIMPLIFY               0x08000000
#define URL_NO_META                     URL_DONT_SIMPLIFY
#define URL_UNESCAPE_INPLACE            0x00100000
#define URL_CONVERT_IF_DOSPATH          0x00200000
#define URL_UNESCAPE_HIGH_ANSI_ONLY     0x00400000
#define URL_INTERNAL_PATH               0x00800000  // Will escape #'s in paths
#define URL_FILE_USE_PATHURL            0x00010000
#define URL_ESCAPE_PERCENT              0x00001000
#define URL_ESCAPE_SEGMENT_ONLY         0x00002000  // Treat the entire URL param as one URL segment.

#define URL_PARTFLAG_KEEPSCHEME         0x00000001

#define URL_APPLY_DEFAULT               0x00000001
#define URL_APPLY_GUESSSCHEME           0x00000002
#define URL_APPLY_GUESSFILE             0x00000004
#define URL_APPLY_FORCEAPPLY            0x00000008


LWSTDAPI_(int)          UrlCompareA(LPCSTR psz1, LPCSTR psz2, BOOL fIgnoreSlash);
LWSTDAPI_(int)          UrlCompareW(LPCWSTR psz1, LPCWSTR psz2, BOOL fIgnoreSlash);
LWSTDAPI                UrlCombineA(LPCSTR pszBase, LPCSTR pszRelative, LPSTR pszCombined, LPDWORD pcchCombined, DWORD dwFlags);
LWSTDAPI                UrlCombineW(LPCWSTR pszBase, LPCWSTR pszRelative, LPWSTR pszCombined, LPDWORD pcchCombined, DWORD dwFlags);
LWSTDAPI                UrlCanonicalizeA(LPCSTR pszUrl, LPSTR pszCanonicalized, LPDWORD pcchCanonicalized, DWORD dwFlags);
LWSTDAPI                UrlCanonicalizeW(LPCWSTR pszUrl, LPWSTR pszCanonicalized, LPDWORD pcchCanonicalized, DWORD dwFlags);
LWSTDAPI_(BOOL)         UrlIsOpaqueA(LPCSTR pszURL);
LWSTDAPI_(BOOL)         UrlIsOpaqueW(LPCWSTR pszURL);
LWSTDAPI_(BOOL)         UrlIsNoHistoryA(LPCSTR pszURL);
LWSTDAPI_(BOOL)         UrlIsNoHistoryW(LPCWSTR pszURL);
#define                 UrlIsFileUrlA(pszURL) UrlIsA(pszURL, URLIS_FILEURL)
#define                 UrlIsFileUrlW(pszURL) UrlIsW(pszURL, URLIS_FILEURL)
LWSTDAPI_(BOOL)         UrlIsA(LPCSTR pszUrl, URLIS UrlIs);
LWSTDAPI_(BOOL)         UrlIsW(LPCWSTR pszUrl, URLIS UrlIs);
LWSTDAPI_(LPCSTR)       UrlGetLocationA(LPCSTR psz1);
LWSTDAPI_(LPCWSTR)      UrlGetLocationW(LPCWSTR psz1);
LWSTDAPI                UrlUnescapeA(LPSTR pszUrl, LPSTR pszUnescaped, LPDWORD pcchUnescaped, DWORD dwFlags);
LWSTDAPI                UrlUnescapeW(LPWSTR pszUrl, LPWSTR pszUnescaped, LPDWORD pcchUnescaped, DWORD dwFlags);
LWSTDAPI                UrlEscapeA(LPCSTR pszUrl, LPSTR pszEscaped, LPDWORD pcchEscaped, DWORD dwFlags);
LWSTDAPI                UrlEscapeW(LPCWSTR pszUrl, LPWSTR pszEscaped, LPDWORD pcchEscaped, DWORD dwFlags);
LWSTDAPI                UrlCreateFromPathA(LPCSTR pszPath, LPSTR pszUrl, LPDWORD pcchUrl, DWORD dwFlags);
LWSTDAPI                UrlCreateFromPathW(LPCWSTR pszPath, LPWSTR pszUrl, LPDWORD pcchUrl, DWORD dwFlags);
LWSTDAPI                PathCreateFromUrlA(LPCSTR pszUrl, LPSTR pszPath, LPDWORD pcchPath, DWORD dwFlags);
LWSTDAPI                PathCreateFromUrlW(LPCWSTR pszUrl, LPWSTR pszPath, LPDWORD pcchPath, DWORD dwFlags);
LWSTDAPI                UrlHashA(LPCSTR pszUrl, LPBYTE pbHash, DWORD cbHash);
LWSTDAPI                UrlHashW(LPCWSTR pszUrl, LPBYTE pbHash, DWORD cbHash);
LWSTDAPI                UrlGetPartW(LPCWSTR pszIn, LPWSTR pszOut, LPDWORD pcchOut, DWORD dwPart, DWORD dwFlags);
LWSTDAPI                UrlGetPartA(LPCSTR pszIn, LPSTR pszOut, LPDWORD pcchOut, DWORD dwPart, DWORD dwFlags);
LWSTDAPI                UrlApplySchemeA(LPCSTR pszIn, LPSTR pszOut, LPDWORD pcchOut, DWORD dwFlags);
LWSTDAPI                UrlApplySchemeW(LPCWSTR pszIn, LPWSTR pszOut, LPDWORD pcchOut, DWORD dwFlags);
LWSTDAPI                HashData(LPBYTE pbData, DWORD cbData, LPBYTE pbHash, DWORD cbHash);


;begin_internal
LWSTDAPI                UrlFixupW(LPCWSTR pszIn, LPWSTR pszOut, DWORD cchOut);  
;end_internal

#ifdef UNICODE
#define UrlCompare              UrlCompareW
#define UrlCombine              UrlCombineW
#define UrlCanonicalize         UrlCanonicalizeW
#define UrlIsOpaque             UrlIsOpaqueW
#define UrlIsFileUrl            UrlIsFileUrlW
#define UrlGetLocation          UrlGetLocationW
#define UrlUnescape             UrlUnescapeW
#define UrlEscape               UrlEscapeW
#define UrlCreateFromPath       UrlCreateFromPathW
#define PathCreateFromUrl       PathCreateFromUrlW
#define UrlHash                 UrlHashW
#define UrlGetPart              UrlGetPartW
#define UrlApplyScheme          UrlApplySchemeW
#define UrlIs                   UrlIsW

;begin_internal
#define UrlFixup                UrlFixupW
;end_internal

#else //!UNICODE
#define UrlCompare              UrlCompareA
#define UrlCombine              UrlCombineA
#define UrlCanonicalize         UrlCanonicalizeA
#define UrlIsOpaque             UrlIsOpaqueA
#define UrlIsFileUrl            UrlIsFileUrlA
#define UrlGetLocation          UrlGetLocationA
#define UrlUnescape             UrlUnescapeA
#define UrlEscape               UrlEscapeA
#define UrlCreateFromPath       UrlCreateFromPathA
#define PathCreateFromUrl       PathCreateFromUrlA
#define UrlHash                 UrlHashA
#define UrlGetPart              UrlGetPartA
#define UrlApplyScheme          UrlApplySchemeA
#define UrlIs                   UrlIsA

;begin_internal
// no UrlFixupA
;end_internal

#endif //UNICODE

#define UrlEscapeSpaces(pszUrl, pszEscaped, pcchEscaped)        UrlCanonicalize(pszUrl, pszEscaped, pcchEscaped, URL_ESCAPE_SPACES_ONLY |URL_DONT_ESCAPE_EXTRA_INFO )
#define UrlUnescapeInPlace(pszUrl, dwFlags)                     UrlUnescape(pszUrl, NULL, NULL, dwFlags | URL_UNESCAPE_INPLACE)

;begin_internal
//
// Internal APIs which we're not yet sure whether to make public
//

// Private IHlinkFrame::Navigate flags related to history
// This navigate should not go in the History ShellFolder
#define SHHLNF_WRITENOHISTORY 0x08000000
// This navigate should not automatically select History ShellFolder
#define SHHLNF_NOAUTOSELECT       0x04000000

// The order of these flags is important.  See the source before
// changing these.

#define PFOPEX_NONE        0x00000000
#define PFOPEX_PIF         0x00000001
#define PFOPEX_COM         0x00000002
#define PFOPEX_EXE         0x00000004
#define PFOPEX_BAT         0x00000008
#define PFOPEX_LNK         0x00000010
#define PFOPEX_CMD         0x00000020
#define PFOPEX_OPTIONAL    0x00000040   // Search only if Extension not present
#define PFOPEX_DEFAULT     (PFOPEX_CMD | PFOPEX_COM | PFOPEX_BAT | PFOPEX_PIF | PFOPEX_EXE | PFOPEX_LNK)

LWSTDAPI_(BOOL)     PathFileExistsDefExt%(LPTSTR% pszPath, UINT uFlags);
LWSTDAPI_(BOOL)     PathFindOnPathEx%(LPTSTR% pszPath, LPCTSTR% * ppszOtherDirs, UINT uFlags);
LWSTDAPI_(LPCTSTR%) PathSkipLeadingSlashes%(LPCTSTR% pszURL);

LWSTDAPI_(UINT)     SHGetSystemWindowsDirectory%(LPTSTR% lpBuffer, UINT uSize);


#if (_WIN32_IE >= 0x0501)
//
// These are functions that used to be duplicated in shell32, but have
// be consolidated here. They are exported privately until someone decides
// we really want to document them.
//
LWSTDAPI_(BOOL) PathFileExistsAndAttributes%(LPCTSTR% pszPath, OPTIONAL DWORD* pdwAttributes);
LWSTDAPI_(void) FixSlashesAndColon%(LPTSTR% pszPath);
LWSTDAPI_(LPCTSTR%) NextPath%(LPCTSTR% lpPath, LPTSTR% szPath, int cchPath);
LWSTDAPI_(LPTSTR%) CharUpperNoDBCS%(LPTSTR% psz);
LWSTDAPI_(LPTSTR%) CharLowerNoDBCS%(LPTSTR% psz);


//
// flags for PathIsValidChar()
//
#define PIVC_ALLOW_QUESTIONMARK     0x00000001  // treat '?' as valid
#define PIVC_ALLOW_STAR             0x00000002  // treat '*' as valid
#define PIVC_ALLOW_DOT              0x00000004  // treat '.' as valid
#define PIVC_ALLOW_SLASH            0x00000008  // treat '\\' as valid
#define PIVC_ALLOW_COLON            0x00000010  // treat ':' as valid
#define PIVC_ALLOW_SEMICOLON        0x00000020  // treat ';' as valid
#define PIVC_ALLOW_COMMA            0x00000040  // treat ',' as valid
#define PIVC_ALLOW_SPACE            0x00000080  // treat ' ' as valid
#define PIVC_ALLOW_NONALPAHABETIC   0x00000100  // treat non-alphabetic exteneded chars as valid
#define PIVC_ALLOW_QUOTE            0x00000200  // treat '"' as valid

//
// standard masks for PathIsValidChar()
//
#define PIVC_SFN_NAME               (PIVC_ALLOW_DOT | PIVC_ALLOW_NONALPAHABETIC)
#define PIVC_SFN_FULLPATH           (PIVC_SFN_NAME | PIVC_ALLOW_COLON | PIVC_ALLOW_SLASH)
#define PIVC_LFN_NAME               (PIVC_ALLOW_DOT | PIVC_ALLOW_NONALPAHABETIC | PIVC_ALLOW_SEMICOLON | PIVC_ALLOW_COMMA | PIVC_ALLOW_SPACE)
#define PIVC_LFN_FULLPATH           (PIVC_LFN_NAME | PIVC_ALLOW_COLON | PIVC_ALLOW_SLASH)
#define PIVC_SFN_FILESPEC           (PIVC_SFN_FULLPATH | PIVC_ALLOW_STAR | PIVC_ALLOW_QUESTIONMARK)
#define PIVC_LFN_FILESPEC           (PIVC_LFN_FULLPATH | PIVC_ALLOW_STAR | PIVC_ALLOW_QUESTIONMARK)

LWSTDAPI_(BOOL) PathIsValidCharA(UCHAR ch, DWORD dwFlags);
LWSTDAPI_(BOOL) PathIsValidCharW(WCHAR ch, DWORD dwFlags);
#ifdef UNICODE
#define PathIsValidChar  PathIsValidCharW
#else
#define PathIsValidChar  PathIsValidCharA
#endif // !UNICODE

#endif // (_WIN32_IE >= 0x0501)


// parsed URL information returned by ParseURL()
//
// Internet_CrackURL is the correct function for external components
// to use. URL.DLL calls this function to do some work and the shell
// uses this function as a leight-weight parsing function as well.

typedef struct tagPARSEDURL% {
    DWORD     cbSize;
    // Pointers into the buffer that was provided to ParseURL
    LPCTSTR%  pszProtocol;
    UINT      cchProtocol;
    LPCTSTR%  pszSuffix;
    UINT      cchSuffix;
    UINT      nScheme;            // One of URL_SCHEME_*
    } PARSEDURL%, * PPARSEDURL%;

LWSTDAPI            ParseURL%(LPCTSTR% pcszURL, PARSEDURL% * ppu);


;end_internal

;begin_both

#endif //  NO_SHLWAPI_PATH

;end_both

;begin_both

#ifndef NO_SHLWAPI_REG
//
//=============== Registry Routines ===================================
//

;end_both
// SHDeleteEmptyKey mimics RegDeleteKey as it behaves on NT.
// SHDeleteKey mimics RegDeleteKey as it behaves on Win95.

LWSTDAPI_(DWORD)    SHDeleteEmptyKey%(HKEY hkey, LPCTSTR% pszSubKey);
LWSTDAPI_(DWORD)    SHDeleteKey%(HKEY hkey, LPCTSTR% pszSubKey);
LWSTDAPI_(HKEY)     SHRegDuplicateHKey(HKEY hkey);

;begin_internal
// BUGBUG (scotth): SHDeleteOrphanKey is the old name for SHDeleteEmptyKey.
//                  This will be removed soon.  SHDeleteOrphanKey already
//                  maps to SHDeleteEmptyKey in the DLL exports.

LWSTDAPI_(DWORD)    SHDeleteOrphanKey%(HKEY hkey, LPCTSTR% pszSubKey);
;end_internal

// These functions open the key, get/set/delete the value, then close
// the key.

LWSTDAPI_(DWORD)    SHDeleteValue%(HKEY hkey, LPCTSTR% pszSubKey, LPCTSTR% pszValue);
LWSTDAPI_(DWORD)    SHGetValue%(HKEY hkey, LPCTSTR% pszSubKey, LPCTSTR% pszValue, DWORD *pdwType, void *pvData, DWORD *pcbData);
LWSTDAPI_(DWORD)    SHSetValue%(HKEY hkey, LPCTSTR% pszSubKey, LPCTSTR% pszValue, DWORD dwType, LPCVOID pvData, DWORD cbData);

// These functions work just like RegQueryValueEx, except if the
// data type is REG_EXPAND_SZ, then these will go ahead and expand
// out the string.  *pdwType will always be massaged to REG_SZ
// if this happens.  REG_SZ values are also guaranteed to be null
// terminated.

LWSTDAPI_(DWORD)    SHQueryValueEx%(HKEY hkey, LPCTSTR% pszValue, DWORD *pdwReserved, DWORD *pdwType, void *pvData, DWORD *pcbData);

// Enumeration functions support.

LWSTDAPI_(LONG)     SHEnumKeyExA(HKEY hkey, DWORD dwIndex, LPSTR pszName, LPDWORD pcchName);
LWSTDAPI_(LONG)     SHEnumKeyExW(HKEY hkey, DWORD dwIndex, LPWSTR pszName, LPDWORD pcchName);
LWSTDAPI_(LONG)     SHEnumValueA(HKEY hkey, DWORD dwIndex, LPSTR pszValueName, LPDWORD pcchValueName, LPDWORD pdwType, void *pvData, LPDWORD pcbData);
LWSTDAPI_(LONG)     SHEnumValueW(HKEY hkey, DWORD dwIndex, LPWSTR pszValueName, LPDWORD pcchValueName, LPDWORD pdwType, void *pvData, LPDWORD pcbData);
LWSTDAPI_(LONG)     SHQueryInfoKeyA(HKEY hkey, LPDWORD pcSubKeys, LPDWORD pcchMaxSubKeyLen, LPDWORD pcValues, LPDWORD pcchMaxValueNameLen);
LWSTDAPI_(LONG)     SHQueryInfoKeyW(HKEY hkey, LPDWORD pcSubKeys, LPDWORD pcchMaxSubKeyLen, LPDWORD pcValues, LPDWORD pcchMaxValueNameLen);

// recursive key copy
LWSTDAPI_(DWORD)     SHCopyKeyA(HKEY hkeySrc, LPCSTR   szSrcSubKey, HKEY hkeyDest, DWORD fReserved);
LWSTDAPI_(DWORD)     SHCopyKeyW(HKEY hkeySrc, LPCWSTR wszSrcSubKey, HKEY hkeyDest, DWORD fReserved);

// Getting and setting file system paths with environment variables

LWSTDAPI_(DWORD)    SHRegGetPathA(HKEY hKey, LPCSTR pcszSubKey, LPCSTR pcszValue, LPSTR pszPath, DWORD dwFlags);
LWSTDAPI_(DWORD)    SHRegGetPathW(HKEY hKey, LPCWSTR pcszSubKey, LPCWSTR pcszValue, LPWSTR pszPath, DWORD dwFlags);
LWSTDAPI_(DWORD)    SHRegSetPathA(HKEY hKey, LPCSTR pcszSubKey, LPCSTR pcszValue, LPCSTR pcszPath, DWORD dwFlags);
LWSTDAPI_(DWORD)    SHRegSetPathW(HKEY hKey, LPCWSTR pcszSubKey, LPCWSTR pcszValue, LPCWSTR pcszPath, DWORD dwFlags);

#ifdef UNICODE
#define SHEnumKeyEx           SHEnumKeyExW
#define SHEnumValue           SHEnumValueW
#define SHQueryInfoKey        SHQueryInfoKeyW
#define SHCopyKey             SHCopyKeyW
#define SHRegGetPath          SHRegGetPathW
#define SHRegSetPath          SHRegSetPathW
#else
#define SHEnumKeyEx           SHEnumKeyExA
#define SHEnumValue           SHEnumValueA
#define SHQueryInfoKey        SHQueryInfoKeyA
#define SHCopyKey             SHCopyKeyA
#define SHRegGetPath          SHRegGetPathA
#define SHRegSetPath          SHRegSetPathA
#endif


//////////////////////////////////////////////
// User Specific Registry Access Functions
//////////////////////////////////////////////

//
// Type definitions.
//

typedef enum
{
    SHREGDEL_DEFAULT = 0x00000000,       // Delete's HKCU, or HKLM if HKCU is not found.
    SHREGDEL_HKCU    = 0x00000001,       // Delete HKCU only
    SHREGDEL_HKLM    = 0x00000010,       // Delete HKLM only.
    SHREGDEL_BOTH    = 0x00000011,       // Delete both HKCU and HKLM.
} SHREGDEL_FLAGS;

typedef enum
{
    SHREGENUM_DEFAULT = 0x00000000,       // Enumerates HKCU or HKLM if not found.
    SHREGENUM_HKCU    = 0x00000001,       // Enumerates HKCU only
    SHREGENUM_HKLM    = 0x00000010,       // Enumerates HKLM only.
    SHREGENUM_BOTH    = 0x00000011,       // Enumerates both HKCU and HKLM without duplicates.
                                          // This option is NYI.
} SHREGENUM_FLAGS;

#define     SHREGSET_HKCU           0x00000001       // Write to HKCU if empty.
#define     SHREGSET_FORCE_HKCU     0x00000002       // Write to HKCU.
#define     SHREGSET_HKLM           0x00000004       // Write to HKLM if empty.
#define     SHREGSET_FORCE_HKLM     0x00000008       // Write to HKLM.
#define     SHREGSET_DEFAULT        (SHREGSET_FORCE_HKCU | SHREGSET_HKLM)          // Default is SHREGSET_FORCE_HKCU | SHREGSET_HKLM.

typedef HANDLE HUSKEY;  // HUSKEY is a Handle to a User Specific KEY.
typedef HUSKEY *PHUSKEY;

LWSTDAPI_(LONG)        SHRegCreateUSKeyA(LPCSTR pszPath, REGSAM samDesired, HUSKEY hRelativeUSKey, PHUSKEY phNewUSKey, DWORD dwFlags);
LWSTDAPI_(LONG)        SHRegCreateUSKeyW(LPCWSTR pwzPath, REGSAM samDesired, HUSKEY hRelativeUSKey, PHUSKEY phNewUSKey, DWORD dwFlags);
LWSTDAPI_(LONG)        SHRegOpenUSKeyA(LPCSTR pszPath, REGSAM samDesired, HUSKEY hRelativeUSKey, PHUSKEY phNewUSKey, BOOL fIgnoreHKCU);
LWSTDAPI_(LONG)        SHRegOpenUSKeyW(LPCWSTR pwzPath, REGSAM samDesired, HUSKEY hRelativeUSKey, PHUSKEY phNewUSKey, BOOL fIgnoreHKCU);
LWSTDAPI_(LONG)        SHRegQueryUSValueA(HUSKEY hUSKey, LPCSTR pszValue, LPDWORD pdwType, void *pvData, LPDWORD pcbData, BOOL fIgnoreHKCU, void *pvDefaultData, DWORD dwDefaultDataSize);
LWSTDAPI_(LONG)        SHRegQueryUSValueW(HUSKEY hUSKey, LPCWSTR pwzValue, LPDWORD pdwType, void *pvData, LPDWORD pcbData, BOOL fIgnoreHKCU, void *pvDefaultData, DWORD dwDefaultDataSize);
LWSTDAPI_(LONG)        SHRegWriteUSValueA(HUSKEY hUSKey, LPCSTR pszValue, DWORD dwType, const void *pvData, DWORD cbData, DWORD dwFlags);
LWSTDAPI_(LONG)        SHRegWriteUSValueW(HUSKEY hUSKey, LPCWSTR pwzValue, DWORD dwType, const void *pvData, DWORD cbData, DWORD dwFlags);
LWSTDAPI_(LONG)        SHRegDeleteUSValueA(HUSKEY hUSKey, LPCSTR pszValue, SHREGDEL_FLAGS delRegFlags);
LWSTDAPI_(LONG)        SHRegDeleteEmptyUSKeyW(HUSKEY hUSKey, LPCWSTR pwzSubKey, SHREGDEL_FLAGS delRegFlags);
LWSTDAPI_(LONG)        SHRegDeleteEmptyUSKeyA(HUSKEY hUSKey, LPCSTR pszSubKey, SHREGDEL_FLAGS delRegFlags);
LWSTDAPI_(LONG)        SHRegDeleteUSValueW(HUSKEY hUSKey, LPCWSTR pwzValue, SHREGDEL_FLAGS delRegFlags);
LWSTDAPI_(LONG)        SHRegEnumUSKeyA(HUSKEY hUSKey, DWORD dwIndex, LPSTR pszName, LPDWORD pcchName, SHREGENUM_FLAGS enumRegFlags);
LWSTDAPI_(LONG)        SHRegEnumUSKeyW(HUSKEY hUSKey, DWORD dwIndex, LPWSTR pwzName, LPDWORD pcchName, SHREGENUM_FLAGS enumRegFlags);
LWSTDAPI_(LONG)        SHRegEnumUSValueA(HUSKEY hUSkey, DWORD dwIndex, LPSTR pszValueName, LPDWORD pcchValueName, LPDWORD pdwType, void *pvData, LPDWORD pcbData, SHREGENUM_FLAGS enumRegFlags);
LWSTDAPI_(LONG)        SHRegEnumUSValueW(HUSKEY hUSkey, DWORD dwIndex, LPWSTR pszValueName, LPDWORD pcchValueName, LPDWORD pdwType, void *pvData, LPDWORD pcbData, SHREGENUM_FLAGS enumRegFlags);
LWSTDAPI_(LONG)        SHRegQueryInfoUSKeyA(HUSKEY hUSKey, LPDWORD pcSubKeys, LPDWORD pcchMaxSubKeyLen, LPDWORD pcValues, LPDWORD pcchMaxValueNameLen, SHREGENUM_FLAGS enumRegFlags);
LWSTDAPI_(LONG)        SHRegQueryInfoUSKeyW(HUSKEY hUSKey, LPDWORD pcSubKeys, LPDWORD pcchMaxSubKeyLen, LPDWORD pcValues, LPDWORD pcchMaxValueNameLen, SHREGENUM_FLAGS enumRegFlags);
LWSTDAPI_(LONG)        SHRegCloseUSKey(HUSKEY hUSKey);


// These calls are equal to an SHRegOpenUSKey, SHRegQueryUSValue, and then a SHRegCloseUSKey.
LWSTDAPI_(LONG)        SHRegGetUSValueA(LPCSTR pszSubKey, LPCSTR pszValue, LPDWORD pdwType, void * pvData, LPDWORD pcbData, BOOL fIgnoreHKCU, void *pvDefaultData, DWORD dwDefaultDataSize);
LWSTDAPI_(LONG)        SHRegGetUSValueW(LPCWSTR pwzSubKey, LPCWSTR pwzValue, LPDWORD pdwType, void * pvData, LPDWORD pcbData, BOOL fIgnoreHKCU, void *pvDefaultData, DWORD dwDefaultDataSize);
LWSTDAPI_(LONG)        SHRegSetUSValueA(LPCSTR pszSubKey, LPCSTR pszValue, DWORD dwType, const void *pvData, DWORD cbData, DWORD dwFlags);
LWSTDAPI_(LONG)        SHRegSetUSValueW(LPCWSTR pwzSubKey, LPCWSTR pwzValue, DWORD dwType, const void *pvData, DWORD cbData, DWORD dwFlags);
LWSTDAPI_(int)         SHRegGetIntW(HKEY hk, LPCWSTR pwzKey, int iDefault);

#ifdef UNICODE
#define SHRegCreateUSKey        SHRegCreateUSKeyW
#define SHRegOpenUSKey          SHRegOpenUSKeyW
#define SHRegQueryUSValue       SHRegQueryUSValueW
#define SHRegWriteUSValue       SHRegWriteUSValueW
#define SHRegDeleteUSValue      SHRegDeleteUSValueW
#define SHRegDeleteEmptyUSKey   SHRegDeleteEmptyUSKeyW
#define SHRegEnumUSKey          SHRegEnumUSKeyW
#define SHRegEnumUSValue        SHRegEnumUSValueW
#define SHRegQueryInfoUSKey     SHRegQueryInfoUSKeyW
#define SHRegGetUSValue         SHRegGetUSValueW
#define SHRegSetUSValue         SHRegSetUSValueW
#define SHRegGetInt             SHRegGetIntW
#else
#define SHRegCreateUSKey        SHRegCreateUSKeyA
#define SHRegOpenUSKey          SHRegOpenUSKeyA
#define SHRegQueryUSValue       SHRegQueryUSValueA
#define SHRegWriteUSValue       SHRegWriteUSValueA
#define SHRegDeleteUSValue      SHRegDeleteUSValueA
#define SHRegDeleteEmptyUSKey   SHRegDeleteEmptyUSKeyA
#define SHRegEnumUSKey          SHRegEnumUSKeyA
#define SHRegEnumUSValue        SHRegEnumUSValueA
#define SHRegQueryInfoUSKey     SHRegQueryInfoUSKeyA
#define SHRegGetUSValue         SHRegGetUSValueA
#define SHRegSetUSValue         SHRegSetUSValueA
#endif

LWSTDAPI_(BOOL) SHRegGetBoolUSValueA(LPCSTR pszSubKey, LPCSTR pszValue, BOOL fIgnoreHKCU, BOOL fDefault);
LWSTDAPI_(BOOL) SHRegGetBoolUSValueW(LPCWSTR pszSubKey, LPCWSTR pszValue, BOOL fIgnoreHKCU, BOOL fDefault);

#ifdef UNICODE
#define SHRegGetBoolUSValue SHRegGetBoolUSValueW
#else
#define SHRegGetBoolUSValue SHRegGetBoolUSValueA
#endif

//
//  Association APIs
//
//  these APIs are to assist in accessing the data in HKCR
//  getting the Command strings and exe paths
//  for different verbs and extensions are simplified this way
//

enum {
    ASSOCF_INIT_NOREMAPCLSID           = 0x00000001,  //  do not remap clsids to progids
    ASSOCF_INIT_BYEXENAME              = 0x00000002,  //  executable is being passed in
    ASSOCF_OPEN_BYEXENAME              = 0x00000002,  //  executable is being passed in
    ASSOCF_INIT_DEFAULTTOSTAR          = 0x00000004,  //  treat "*" as the BaseClass
    ASSOCF_INIT_DEFAULTTOFOLDER        = 0x00000008,  //  treat "Folder" as the BaseClass
    ASSOCF_NOUSERSETTINGS              = 0x00000010,  //  dont use HKCU
    ASSOCF_NOTRUNCATE                  = 0x00000020,  //  dont truncate the return string
    ASSOCF_VERIFY                      = 0x00000040,  //  verify data is accurate (DISK HITS)
    ASSOCF_REMAPRUNDLL                 = 0x00000080,  //  actually gets info about rundlls target if applicable
    ASSOCF_NOFIXUPS                    = 0x00000100,  //  attempt to fix errors if found
    ASSOCF_IGNOREBASECLASS             = 0x00000200,  //  dont recurse into the baseclass
};

typedef DWORD ASSOCF;


typedef enum {
    ASSOCSTR_COMMAND      = 1,  //  shell\verb\command string
    ASSOCSTR_EXECUTABLE,        //  the executable part of command string
    ASSOCSTR_FRIENDLYDOCNAME,   //  friendly name of the document type
    ASSOCSTR_FRIENDLYAPPNAME,   //  friendly name of executable
    ASSOCSTR_NOOPEN,            //  noopen value
    ASSOCSTR_SHELLNEWVALUE,     //  query values under the shellnew key
    ASSOCSTR_DDECOMMAND,        //  template for DDE commands
    ASSOCSTR_DDEIFEXEC,         //  DDECOMMAND to use if just create a process
    ASSOCSTR_DDEAPPLICATION,    //  Application name in DDE broadcast
    ASSOCSTR_DDETOPIC,          //  Topic Name in DDE broadcast
    ASSOCSTR_INFOTIP,           //  info tip for an item, or list of properties to create info tip from
    ASSOCSTR_MAX                //  last item in enum...
} ASSOCSTR;

typedef enum {
    ASSOCKEY_SHELLEXECCLASS = 1,  //  the key that should be passed to ShellExec(hkeyClass)
    ASSOCKEY_APP,                 //  the "Application" key for the association
    ASSOCKEY_CLASS,               //  the progid or class key
    ASSOCKEY_BASECLASS,           //  the BaseClass key
    ASSOCKEY_MAX                  //  last item in enum...
} ASSOCKEY;

typedef enum {
    ASSOCDATA_MSIDESCRIPTOR = 1,  //  Component Descriptor to pass to MSI APIs
    ASSOCDATA_NOACTIVATEHANDLER,  //  restrict attempts to activate window
    ASSOCDATA_QUERYCLASSSTORE,    //  should check with the NT Class Store
    ASSOCDATA_HASPERUSERASSOC,    //  defaults to user specified association
    ASSOCDATA_MAX
} ASSOCDATA;

typedef enum {
    ASSOCENUM_NONE
} ASSOCENUM;

#undef INTERFACE
#define INTERFACE IQueryAssociations

DECLARE_INTERFACE_( IQueryAssociations, IUnknown )
{
    // IUnknown methods
    STDMETHOD (QueryInterface)(THIS_ REFIID riid, void **ppv) PURE;
    STDMETHOD_(ULONG, AddRef) ( THIS ) PURE;
    STDMETHOD_(ULONG, Release) ( THIS ) PURE;

    // IAssociations methods
    STDMETHOD (Init)(THIS_ ASSOCF flags, LPCWSTR pszAssoc, HKEY hkProgid, HWND hwnd) PURE;
    STDMETHOD (GetString)(THIS_ ASSOCF flags, ASSOCSTR str, LPCWSTR pszExtra, LPWSTR pszOut, DWORD *pcchOut) PURE;
    STDMETHOD (GetKey)(THIS_ ASSOCF flags, ASSOCKEY key, LPCWSTR pszExtra, HKEY *phkeyOut) PURE;
    STDMETHOD (GetData)(THIS_ ASSOCF flags, ASSOCDATA data, LPCWSTR pszExtra, LPVOID pvOut, DWORD *pcbOut) PURE;
    STDMETHOD (GetEnum)(THIS_ ASSOCF flags, ASSOCENUM assocenum, LPCWSTR pszExtra, REFIID riid, LPVOID *ppvOut) PURE;
};

#define CLSID_QueryAssociations IID_IQueryAssociations

;begin_internal

typedef struct tagAssocDDEExec
{
    LPCWSTR pszDDEExec;
    LPCWSTR pszApplication;
    LPCWSTR pszTopic;
    BOOL fNoActivateHandler;
} ASSOCDDEEXEC;

typedef struct tagAssocVerb
{
    LPCWSTR pszVerb;
    LPCWSTR pszTitle;
    LPCWSTR pszFriendlyAppName;
    LPCWSTR pszApplication;
    LPCWSTR pszParams;
    ASSOCDDEEXEC *pDDEExec;
} ASSOCVERB;

typedef struct tagAssocShell
{
    ASSOCVERB *rgVerbs;
    DWORD cVerbs;
    DWORD iDefaultVerb;
} ASSOCSHELL;

typedef struct tagAssocProgid
{
    DWORD cbSize;
    LPCWSTR pszProgid;
    LPCWSTR pszFriendlyDocName;
    LPCWSTR pszDefaultIcon;
    ASSOCSHELL *pShellKey;
    LPCWSTR pszExtensions;
} ASSOCPROGID;

typedef struct tagAssocApp
{
    DWORD cbSize;
    LPCWSTR pszFriendlyAppName;
    ASSOCSHELL *pShellKey;
} ASSOCAPP;

enum {
    ASSOCMAKEF_VERIFY                  = 0x00000040,  //  verify data is accurate (DISK HITS)
    ASSOCMAKEF_USEEXPAND               = 0x00000200,  //  strings have environment vars and need REG_EXPAND_SZ
    ASSOCMAKEF_SUBSTENV                = 0x00000400,  //  attempt to use std env if they match...
    ASSOCMAKEF_VOLATILE                = 0x00000800,  //  the progid will not persist between sessions
    ASSOCMAKEF_DELETE                  = 0x00002000,  //  remove this association if possible
};

typedef DWORD ASSOCMAKEF;

LWSTDAPI AssocMakeProgid(ASSOCMAKEF flags, LPCWSTR pszApplication, ASSOCPROGID *pProgid, HKEY *phkProgid);
LWSTDAPI AssocMakeApp(ASSOCMAKEF flags, LPCWSTR pszApplication, ASSOCAPP *pApp, HKEY *phkApp);

LWSTDAPI AssocMakeApplicationByKey%(ASSOCMAKEF flags, HKEY hkAssoc, LPCTSTR% pszVerb);
LWSTDAPI AssocMakeFileExtsToApplication%(ASSOCMAKEF flags, LPCTSTR% pszExt, LPCTSTR% pszApplication);

LWSTDAPI AssocCopyVerbs(HKEY hkSrc, HKEY hkDst);

;end_internal

LWSTDAPI AssocCreate(CLSID clsid, REFIID riid, LPVOID *ppv);

//  wrappers for the interface
LWSTDAPI AssocQueryString%(ASSOCF flags, ASSOCSTR str, LPCTSTR% pszAssoc, LPCTSTR% pszExtra, LPTSTR% pszOut, DWORD *pcchOut);
LWSTDAPI AssocQueryStringByKey%(ASSOCF flags, ASSOCSTR str, HKEY hkAssoc, LPCTSTR% pszExtra, LPTSTR% pszOut, DWORD *pcchOut);
LWSTDAPI AssocQueryKey%(ASSOCF flags, ASSOCKEY key, LPCTSTR% pszAssoc, LPCTSTR% pszExtra, HKEY *phkeyOut);


;begin_both

#endif //  NO_SHLWAPI_REG

;end_both

;begin_internal

#ifndef NO_SHLWAPI_UALSTR
#include <uastrfnc.h>
#endif //  NO_SHLWAPI_UALSTR

;end_internal
;begin_both

#ifndef NO_SHLWAPI_STREAM
//
//=============== Stream Routines ===================================
//
;end_both
;begin_internal
//
//  We must say "struct IStream" instead of "IStream" in case we are
//  #include'd before <ole2.h>.
//
;end_internal

LWSTDAPI_(struct IStream *) SHOpenRegStream%(HKEY hkey, LPCTSTR% pszSubkey, LPCTSTR% pszValue, DWORD grfMode);
LWSTDAPI_(struct IStream *) SHOpenRegStream2%(HKEY hkey, LPCTSTR% pszSubkey, LPCTSTR% pszValue, DWORD grfMode);
// New code always wants new implementation...
#undef SHOpenRegStream
#define SHOpenRegStream SHOpenRegStream2

LWSTDAPI SHCreateStreamOnFile%(LPCTSTR% pszFile, DWORD grfMode, struct IStream **ppstm);

;begin_internal

LWSTDAPI_(struct IStream *) SHCreateMemStream(LPBYTE pInit, UINT cbInit);

// SHCreateStreamWrapper creates an IStream that spans multiple IStream implementations.
// NOTE: STGM_READ is the only mode currently supported
LWSTDAPI SHCreateStreamWrapper(IStream *aStreams[], UINT cStreams, DWORD grfMode, IStream **ppstm);


// These functions read, write, and maintain a list of DATABLOCK_HEADERs.
// Blocks can be of any size (cbSize) and they are added, found, and removed
// by dwSignature. Each block is guranteed to be aligned on a DWORD boundary
// in memory. The stream format is identical to Windows 95 and NT 4
// CShellLink's "EXP" data format (with one bug fix: stream data is NULL
// terminated on write...)
//
// SHReadDataBlocks and SHAddDataBlock will allocate your pdbList for you.
//
// SHFindDataBlock returns a pointer into the pdbList.
//
// SHAddDataBlock and SHRemoveDataBlock return TRUE if ppdbList modified.
//
#ifndef __DATABLOCKHEADER_DEFINED
#define __DATABLOCKHEADER_DEFINED
typedef struct tagDATABLOCKHEADER {
    DWORD   cbSize;             // Size of this extra data block
    DWORD   dwSignature;        // signature of this extra data block
} DATABLOCK_HEADER, *LPDATABLOCK_HEADER, *LPDBLIST;
#endif

STDAPI SHWriteDataBlockList(struct IStream* pstm, LPDBLIST pdbList);
STDAPI SHReadDataBlockList(struct IStream* pstm, LPDBLIST * ppdbList);
STDAPI_(void) SHFreeDataBlockList(LPDBLIST pdbList);
STDAPI_(BOOL) SHAddDataBlock(LPDBLIST * ppdbList, LPDATABLOCK_HEADER pdb);
STDAPI_(BOOL) SHRemoveDataBlock(LPDBLIST * ppdbList, DWORD dwSignature);
STDAPI_(void *) SHFindDataBlock(LPDBLIST pdbList, DWORD dwSignature);


// FUNCTION: SHCheckDiskForMedia
//
// hwnd - NULL means no UI will be displayed.  Non-NULL means
// punkEnableModless - Make caller modal during UI. (OPTIONAL)
// pszPath - Path that needs verification.
// wFunc - Type of operation (FO_MOVE, FO_COPY, FO_DELETE, FO_RENAME - shellapi.h)
//
// NOTE: USE NT5's SHPathPrepareForWrite() instead, it's MUCH MUCH BETTER.

STDAPI_(BOOL) SHCheckDiskForMediaA(HWND hwnd, IUnknown * punkEnableModless, LPCSTR pszPath, UINT wFunc);
STDAPI_(BOOL) SHCheckDiskForMediaW(HWND hwnd, IUnknown * punkEnableModless, LPCWSTR pwzPath, UINT wFunc);

#ifdef UNICODE
#define SHCheckDiskForMedia      SHCheckDiskForMediaW
#else
#define SHCheckDiskForMedia      SHCheckDiskForMediaA
#endif
;end_internal

;begin_both

#endif // NO_SHLWAPI_STREAM

;end_both


;begin_internal
#ifndef NO_SHLWAPI_MLUI
//
//=============== Multi Language UI Routines ===================================
//
;end_internal

;begin_internal

#define     ORD_SHGETWEBFOLDERFILEPATHA 440
#define     ORD_SHGETWEBFOLDERFILEPATHW 441
LWSTDAPI    SHGetWebFolderFilePath%(LPCTSTR% pszFileName, LPTSTR% pszMUIPath, UINT cchMUIPath);

// Use MLLoadLibrary to get the ML-resource file.  This function tags the file so
// all standard shlwapi wrap functions automatically get ML-behavior.
//

#define ORD_MLLOADLIBRARYA  377
#define ORD_MLLOADLIBRARYW  378
LWSTDAPI_(HINSTANCE) MLLoadLibrary%(LPCTSTR% lpLibFileName, HMODULE hModule, DWORD dwCrossCodePage);
LWSTDAPI_(BOOL) MLFreeLibrary(HMODULE hModule);

#define ML_NO_CROSSCODEPAGE     0
#define ML_CROSSCODEPAGE_NT     1
#define ML_CROSSCODEPAGE        2
#define ML_SHELL_LANGUAGE       4
#define ML_CROSSCODEPAGE_MASK   7

// If you are a global distributable a-la comctl32 that doesn't follow the IE5
// PlugUI resource layout, then load your own hinstance and poke it into shlwapi
// using these functions:
//
LWSTDAPI MLSetMLHInstance(HINSTANCE hInst, LANGID lidUI);
LWSTDAPI MLClearMLHInstance(HINSTANCE hInst);

// Of course you need to know what UI language to use:
//
#define ORD_MLGETUILANGUAGE 376
LWSTDAPI_(LANGID) MLGetUILanguage(void);

// Super internal and you probably don't need this one, but comctl32 does
// some font munging in PlugUI cases on your apps behalf:
//
LWSTDAPI_(BOOL) MLIsMLHInstance(HINSTANCE hInst);


LWSTDAPI_(HRESULT) MLBuildResURL%(LPCTSTR% szLibFile, HMODULE hModule, DWORD dwCrossCodePage, LPCTSTR% szResourceName, LPTSTR% pszResURL, int nBufSize);
#define ORD_MLWINHELPA      395
#define ORD_MLWINHELPW      397
LWSTDAPI_(BOOL) MLWinHelp%(HWND hWndCaller, LPCTSTR% lpszHelp, UINT uCommand, DWORD_PTR dwData);
#define ORD_MLHTMLHELPA     396
#define ORD_MLHTMLHELPW     398
LWSTDAPI_(HWND) MLHtmlHelp%(HWND hWndCaller, LPCTSTR% pszFile, UINT uCommand, DWORD_PTR dwData, DWORD dwCrossCodePage);

;end_internal

;begin_internal

#endif // NO_SHLWAPI_MLUI
;end_internal

;begin_internal

#ifndef NO_SHLWAPI_HTTP
//
//=============== HTTP helper Routines ===================================
//  The calling thread must have called CoInitialize() before using this
//  function - it will create a format enumerator and associate it as a
// property with the IShellBrowser passed in, so that it will be reused.
//

;begin_internal
//
//  We must say "struct IWhatever" instead of "IWhatever" in case we are
//  #include'd before <ole2.h>.
//
;end_internal
STDAPI RegisterDefaultAcceptHeaders(struct IBindCtx* pbc, struct IShellBrowser* psb);

LWSTDAPI RunRegCommand(HWND hwnd, HKEY hkey, LPCWSTR pszKey);
LWSTDAPI RunIndirectRegCommand(HWND hwnd, HKEY hkey, LPCWSTR pszKey, LPCWSTR pszVerb);
LWSTDAPI SHRunIndirectRegClientCommand(HWND hwnd, LPCWSTR pszClient);

LWSTDAPI   GetAcceptLanguagesA(LPSTR psz, LPDWORD pcch);
LWSTDAPI   GetAcceptLanguagesW(LPWSTR pwz, LPDWORD pcch);

#ifdef UNICODE
#define GetAcceptLanguages      GetAcceptLanguagesW
#else
#define GetAcceptLanguages      GetAcceptLanguagesA
#endif

#endif // NO_SHLWAPI_HTTP

;end_internal

;begin_internal

LWSTDAPI_(HWND) SHHtmlHelpOnDemandW(HWND hwnd, LPCWSTR pszFile, UINT uCommand, DWORD_PTR dwData, DWORD dwCrossCodePage, BOOL bUseML);
LWSTDAPI_(HWND) SHHtmlHelpOnDemandA(HWND hwnd, LPCSTR pszFile, UINT uCommand, DWORD_PTR dwData, DWORD dwCrossCodePage, BOOL bUseML);
LWSTDAPI_(BOOL) SHWinHelpOnDemandW(HWND hwnd, LPCWSTR pszFile, UINT uCommand, DWORD_PTR dwData, BOOL bUseML);
LWSTDAPI_(BOOL) SHWinHelpOnDemandA(HWND hwnd, LPCSTR pszFile, UINT uCommand, DWORD_PTR dwData, BOOL bUseML);
LWSTDAPI_(BOOL) WINAPI Shell_GetCachedImageIndexWrapW(LPCWSTR pszIconPath, int iIconIndex, UINT uIconFlags);
LWSTDAPI_(BOOL) WINAPI Shell_GetCachedImageIndexWrapA(LPCSTR pszIconPath, int iIconIndex, UINT uIconFlags);

#ifdef UNICODE
#define SHHtmlHelpOnDemand      SHHtmlHelpOnDemandW
#define SHWinHelpOnDemand       SHWinHelpOnDemandW
#define Shell_GetCachedImageIndexWrap Shell_GetCachedImageIndexWrapW
#else
#define SHHtmlHelpOnDemand      SHHtmlHelpOnDemandA
#define SHWinHelpOnDemand       SHWinHelpOnDemandA
#define Shell_GetCachedImageIndexWrap Shell_GetCachedImageIndexWrapA
#endif

;end_internal

;begin_internal
#ifndef NO_SHLWAPI_STOPWATCH
//
//=============== Performance timing macros and prototypes ================

// StopWatch performance mode flags used in dwFlags param in API's and in Mode key at
// HKLM\software\microsoft\windows\currentversion\explorer\performance
// NOTE: low word is used for the mode, high word is used to change the default painter timer interval.
//       If we need more mode bits then we'll need a new reg key for paint timer
#define SPMODE_SHELL      0x00000001
#define SPMODE_DEBUGOUT   0x00000002
#define SPMODE_TEST       0x00000004
#define SPMODE_BROWSER    0x00000008
#define SPMODE_FLUSH      0x00000010
#define SPMODE_EVENT      0x00000020
#define SPMODE_JAVA       0x00000040
#define SPMODE_FORMATTEXT 0x00000080
#define SPMODE_PROFILE    0x00000100
#define SPMODE_DEBUGBREAK 0x00000200
#define SPMODE_MSGTRACE   0x00000400
#define SPMODE_PERFTAGS   0x00000800
#define SPMODE_MEMWATCH   0x00001000
#define SPMODE_DBMON      0x00002000
#define SPMODE_15         0x00004000
#define SPMODE_16         0x00008000
#define SPMODE_RESERVED   0xffff0000
#define SPMODES (SPMODE_SHELL | SPMODE_BROWSER | SPMODE_JAVA |  SPMODE_MSGTRACE | SPMODE_MEMWATCH | SPMODE_DBMON)

// StopWatch node types used in memory log to identify the type of node
#define EMPTY_NODE  0x0
#define START_NODE  0x1
#define LAP_NODE    0x2
#define STOP_NODE   0x3
#define OUT_OF_NODES 0x4

// StopWatch timing ids used to identify the type of timing being performed
#define SWID_STARTUP         0x0
#define SWID_FRAME           0x1
#define SWID_COPY            0x2
#define SWID_TREE            0x3
#define SWID_BROWSER_FRAME   0x4
#define SWID_JAVA_APP        0x5
#define SWID_MENU            0x6
#define SWID_BITBUCKET       0x7
#define SWID_EXPLBAR         0x8
#define SWID_MSGDISPATCH     0x9
#define SWID_TRACEMSG        0xa
#define SWID_DBMON_DLLLOAD   0xb
#define SWID_DBMON_EXCEPTION 0xc
#define SWID_MASK_BROWSER_STOPBTN 0x8000000     // identifies BROWSER_FRAME stop caused by stop button

#define SWID_MASKS         SWID_MASK_BROWSER_STOPBTN // add any SWID_MASK_* defines here

#define SWID(dwId) (dwId & (~SWID_MASKS))

// The following StopWatch messages are used to drive the timer msg handler.  The timer proc is used
// as a means of delaying while watching paint messages.  If the defined number of timer ticks has
// passed without getting any paint messages, then we mark the time of the last paint message we've
// saved as the stop time.
#define SWMSG_PAINT    1    // paint message rcvd
#define SWMSG_TIMER    2    // timer tick
#define SWMSG_CREATE   3    // init handler and create timer
#define SWMSG_STATUS   4    // get status of whether timing is active or not

#define ID_STOPWATCH_TIMER 0xabcd   // Timer id

// Stopwatch defaults
#define STOPWATCH_MAX_NODES                 100
#define STOPWATCH_DEFAULT_PAINT_INTERVAL   1000
#define STOPWATCH_DEFAULT_MAX_DISPATCH_TIME 150
#define STOPWATCH_DEFAULT_MAX_MSG_TIME     1000
#define STOPWATCH_DEFAULT_MAX_MSG_INTERVAL   50
#define STOPWATCH_DEFAULT_CLASSNAMES TEXT("Internet Explorer_Server") TEXT("\0") TEXT("SHELLDLL_DefView") TEXT("\0") TEXT("SysListView32") TEXT("\0\0")

#define MEMWATCH_DEFAULT_PAGES  512
#define MEMWATCH_DEFAULT_TIME  1000
#define MEMWATCH_DEFAULT_FLAGS    0


#ifdef UNICODE
#define StopWatch StopWatchW
#else
#define StopWatch StopWatchA
#endif

#define StopWatch_Start(dwId, pszDesc, dwFlags) StopWatch(dwId, pszDesc, START_NODE, dwFlags, 0)
#define StopWatch_Lap(dwId, pszDesc, dwFlags)   StopWatch(dwId, pszDesc, LAP_NODE, dwFlags, 0)
#define StopWatch_Stop(dwId, pszDesc, dwFlags)  StopWatch(dwId, pszDesc, STOP_NODE, dwFlags, 0)
#define StopWatch_StartTimed(dwId, pszDesc, dwFlags, dwCount)  StopWatch(dwId, pszDesc, START_NODE, dwFlags, dwCount)
#define StopWatch_LapTimed(dwId, pszDesc, dwFlags, dwCount)  StopWatch(dwId, pszDesc, LAP_NODE, dwFlags, dwCount)
#define StopWatch_StopTimed(dwId, pszDesc, dwFlags, dwCount)  StopWatch(dwId, pszDesc, STOP_NODE, dwFlags, dwCount)

VOID InitStopWatchMode(VOID);

// EXPORTED FUNCTIONS
DWORD WINAPI StopWatchW(DWORD dwId, LPCWSTR pszDesc, DWORD dwType, DWORD dwFlags, DWORD dwCount);
DWORD WINAPI StopWatchA(DWORD dwId, LPCSTR pszDesc, DWORD dwType, DWORD dwFlags, DWORD dwCount);
DWORD WINAPI StopWatchMode(VOID);
DWORD WINAPI StopWatchFlush(VOID);
BOOL WINAPI StopWatch_TimerHandler(HWND hwnd, UINT uInc, DWORD dwFlag, MSG *pmsg);
VOID WINAPI StopWatch_CheckMsg(HWND hwnd, MSG msg, LPCSTR lpStr);
VOID WINAPI StopWatch_MarkFrameStart(LPCSTR lpExplStr);
VOID WINAPI StopWatch_MarkSameFrameStart(HWND hwnd);
VOID WINAPI StopWatch_MarkJavaStop(LPCSTR  lpStringToSend, HWND hwnd, BOOL fChType);
DWORD WINAPI GetPerfTime(VOID);
VOID WINAPI StopWatch_SetMsgLastLocation(DWORD dwLast);
DWORD WINAPI StopWatch_DispatchTime(BOOL fStartTime, MSG msg, DWORD dwStart);

extern DWORD g_dwStopWatchMode;
//
//=============== End Performance timing macros and prototypes ================

#endif //#ifndef NO_SHLWAPI_STOPWATCH

;end_internal

;begin_internal

#ifndef NO_SHLWAPI_INTERNAL
//
//=============== Internal helper routines ===================================

//
//  Declare some OLE interfaces we need to refer to and which aren't
//  already defined in objbase.h
//

#ifndef RC_INVOKED /* { rc doesn't like these long symbol names */
#ifndef __IOleCommandTarget_FWD_DEFINED__
#define __IOleCommandTarget_FWD_DEFINED__
typedef struct IOleCommandTarget IOleCommandTarget;
#endif  /* __IOleCommandTarget_FWD_DEFINED__ */

#ifndef __IDropTarget_FWD_DEFINED__
#define __IDropTarget_FWD_DEFINED__
typedef struct IDropTarget IDropTarget;
#endif  /* __IDropTarget_FWD_DEFINED__ */

#ifndef __IPropertyBag_FWD_DEFINED__
#define __IPropertyBag_FWD_DEFINED__
typedef struct IPropertyBag IPropertyBag;
#endif  /* __IPropertyBag_FWD_DEFINED__ */

#ifndef __IConnectionPoint_FWD_DEFINED__
#define __IConnectionPoint_FWD_DEFINED__
typedef struct IConnectionPoint IConnectionPoint;
#endif  /* __IConnectionPoint_FWD_DEFINED__ */

    LWSTDAPI_(void) IUnknown_AtomicRelease(void ** ppunk);
    LWSTDAPI_(BOOL) SHIsSameObject(IUnknown* punk1, IUnknown* punk2);
    LWSTDAPI IUnknown_GetWindow(IUnknown* punk, HWND* phwnd);
    LWSTDAPI IUnknown_SetOwner(IUnknown* punk, IUnknown* punkOwner);
    LWSTDAPI IUnknown_SetSite(IUnknown *punk, IUnknown *punkSite);
    LWSTDAPI IUnknown_GetSite(IUnknown *punk, REFIID riid, void **ppvOut);
    LWSTDAPI IUnknown_EnableModless(IUnknown * punk, BOOL fEnabled);
    LWSTDAPI IUnknown_GetClassID(IUnknown *punk, CLSID *pclsid);
    LWSTDAPI IUnknown_QueryService(IUnknown* punk, REFGUID guidService, REFIID riid, void ** ppvOut);
    LWSTDAPI IUnknown_HandleIRestrict(IUnknown * punk, const GUID * pguidID, DWORD dwRestrictAction, VARIANT * pvarArgs, DWORD * pdwRestrictionResult);
    LWSTDAPI IUnknown_OnFocusOCS(IUnknown *punk, BOOL fGotFocus);
    LWSTDAPI IUnknown_TranslateAcceleratorOCS(IUnknown *punk, LPMSG lpMsg, DWORD grfMods);
    LWSTDAPI_(void) IUnknown_Set(IUnknown ** ppunk, IUnknown * punk);

    LWSTDAPI SHWeakQueryInterface(IUnknown *punkOuter, IUnknown *punkTarget, REFIID riid, void **ppvOut);
    LWSTDAPI_(void) SHWeakReleaseInterface(IUnknown *punkOuter, IUnknown **ppunk);

    // Helper macros for the Weak interface functions.
    #define     SHQueryInnerInterface           SHWeakQueryInterface
    #define     SHReleaseInnerInterface         SHWeakReleaseInterface
    #define     SHReleaseOuterInterface         SHWeakReleaseInterface

    __inline HRESULT SHQueryOuterInterface(IUnknown *punkOuter, REFIID riid, void **ppvOut)
    {
        return SHWeakQueryInterface(punkOuter, punkOuter, riid, ppvOut);
    }

#if defined(__IOleAutomationTypes_INTERFACE_DEFINED__) && \
    defined(__IOleCommandTarget_INTERFACE_DEFINED__)
    LWSTDAPI IUnknown_QueryStatus(IUnknown *punk, const GUID *pguidCmdGroup, ULONG cCmds, OLECMD rgCmds[], OLECMDTEXT *pcmdtext);
    LWSTDAPI IUnknown_Exec(IUnknown* punk, const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANTARG *pvarargIn, VARIANTARG *pvarargOut);

    // Some of the many connection point helper functions available in
    // connect.cpp.  We export only the ones people actually use.  If
    // you need a helper function, maybe it's already in connect.cpp
    // and merely needs to be exported.

    LWSTDAPI SHPackDispParamsV(DISPPARAMS * pdispparams, VARIANTARG *rgvt,
                               UINT cArgs, va_list arglist);
    LWSTDAPIV SHPackDispParams(DISPPARAMS * pdispparams, VARIANTARG *rgvt,
                               UINT cArgs, ...);

    typedef HRESULT (CALLBACK *SHINVOKECALLBACK)(IDispatch *pdisp, struct SHINVOKEPARAMS *pinv);

    typedef struct SHINVOKEPARAMS {
        UINT flags;                     // mandatory
        DISPID dispidMember;            // mandatory
        const IID*piid;                 // IPFL_USEDEFAULTS will fill this in
        LCID lcid;                      // IPFL_USEDEFAULTS will fill this in
        WORD wFlags;                    // IPFL_USEDEFAULTS will fill this in
        DISPPARAMS * pdispparams;       // mandatory, may be NULL
        VARIANT * pvarResult;           // IPFL_USEDEFAULTS will fill this in
        EXCEPINFO * pexcepinfo;         // IPFL_USEDEFAULTS will fill this in
        UINT * puArgErr;                // IPFL_USEDEFAULTS will fill this in
        SHINVOKECALLBACK Callback;      // required if IPFL_USECALLBACK
    } SHINVOKEPARAMS, *LPSHINVOKEPARAMS;

    #define IPFL_USECALLBACK        0x0001
    #define IPFL_USEDEFAULTS        0x0002

#if 0 // These functions not yet needed
    LWSTDAPI IConnectionPoint_InvokeIndirect(IConnectionPoint *pcp,
                            SHINVOKEPARAMS *pinv);
#endif

    LWSTDAPI IConnectionPoint_InvokeWithCancel(IConnectionPoint *pcp,
                    DISPID dispidMember, DISPPARAMS * pdispparams,
                    LPBOOL pfCancel, LPVOID *ppvCancel);
    LWSTDAPI IConnectionPoint_SimpleInvoke(IConnectionPoint *pcp,
                    DISPID dispidMember, DISPPARAMS * pdispparams);

#if 0 // These functions not yet needed
    LWSTDAPI IConnectionPoint_InvokeParamV(IConnectionPoint *pcp,
                    DISPID dispidMember, VARIANTARG *rgvarg,
                    UINT cArgs, va_list ap);
    LWSTDAPIV IConnectionPoint_InvokeParam(IConnectionPoint *pcp,
                    DISPID dispidMember, VARIANTARG *rgvarg, UINT cArgs, ...)
#endif

    LWSTDAPI IConnectionPoint_OnChanged(IConnectionPoint *pcp, DISPID dispid);

#if 0 // These functions not yet needed
    LWSTDAPI IUnknown_FindConnectionPoint(IUnknown *punk,
                    REFIID riidCP, IConnectionPoint **pcpOut);
#endif

    LWSTDAPI IUnknown_CPContainerInvokeIndirect(IUnknown *punk, REFIID riidCP,
                SHINVOKEPARAMS *pinv);
    LWSTDAPIV IUnknown_CPContainerInvokeParam(IUnknown *punk, REFIID riidCP,
                DISPID dispidMember, VARIANTARG *rgvarg, UINT cArgs, ...);
    LWSTDAPI IUnknown_CPContainerOnChanged(IUnknown *punk, DISPID dispid);

#endif /* IOleAutomationTypes && IOleCommandTarget */
#endif  /* } !RC_INVOKED */

    LWSTDAPI IStream_Read(IStream *pstm, void *pv, ULONG cb);
    LWSTDAPI IStream_Write(IStream *pstm, const void *pv, ULONG cb);
    LWSTDAPI IStream_Reset(IStream *pstm);
    LWSTDAPI IStream_Size(IStream *pstm, ULARGE_INTEGER *pui);

    LWSTDAPI_(BOOL) SHIsEmptyStream(IStream* pstm);

    LWSTDAPI SHSimulateDrop(IDropTarget *pdrop, IDataObject *pdtobj, DWORD grfKeyState,
                         const POINTL *ppt, DWORD *pdwEffect);

    LWSTDAPI SHLoadFromPropertyBag(IUnknown* punk, IPropertyBag* ppg);

    LWSTDAPI ConnectToConnectionPoint(IUnknown* punkThis, REFIID riidEvent, BOOL fConnect, IUnknown* punkTarget, DWORD* pdwCookie, IConnectionPoint** ppcpOut);

/*
 *  Like PrivateProfileString except that UNICODE strings are encoded so they
 *  will successfully round-trip.
 */
LWSTDAPI_(DWORD) SHGetIniStringW(LPCWSTR lpSection, LPCWSTR lpKey, LPWSTR lpBuf, DWORD nSize, LPCWSTR lpFile);
#define SHGetIniStringA(lpSection, lpKey, lpBuf, nSize, lpFile) \
        GetPrivateProfileStringA(lpSection, lpKey, "", lpBuf, nSize, lpFile)

LWSTDAPI_(BOOL) SHSetIniStringW(LPCWSTR lpSection, LPCWSTR lpKey, LPCWSTR lpString, LPCWSTR lpFile);
#define SHSetIniStringA  WritePrivateProfileStringA

LWSTDAPI CreateURLFileContentsW(LPCWSTR pwszUrl, LPSTR *ppszOut);
LWSTDAPI CreateURLFileContentsA(LPCSTR pszUrl, LPSTR *ppszOut);

#ifdef UNICODE
#define SHGetIniString SHGetIniStringW
#define SHSetIniString SHSetIniStringW
#define CreateURLFileContents CreateURLFileContentsW
#else
#define SHGetIniString SHGetIniStringA
#define SHSetIniString SHSetIniStringA
#define CreateURLFileContents CreateURLFileContentsA
#endif // UNICODE

/*
 *  We say `struct IShellFolder' because the definition of IShellFolder
 *  doesn't show up until later when shlobj.h is included.  Similarly, we
 *  insert a temporary definition of LPCITEMIDLIST.
 */

#define LPCITEMIDLIST const UNALIGNED struct _ITEMIDLIST *
#define  LPITEMIDLIST       UNALIGNED struct _ITEMIDLIST *

#define ISHGDN2_CANREMOVEFORPARSING     0x0001
LWSTDAPI IShellFolder_GetDisplayNameOf(struct IShellFolder *psf,
    LPCITEMIDLIST pidl, DWORD uFlags, struct _STRRET *pstr, DWORD dwFlags2);

LWSTDAPI IShellFolder_ParseDisplayName(struct IShellFolder *psf, HWND hwnd,
    struct IBindCtx *pbc, LPWSTR pszDisplayName, ULONG *pchEaten,
    LPITEMIDLIST *ppidl, ULONG *pdwAttributes);

LWSTDAPI IShellFolder_EnumObjects(struct IShellFolder *psf, HWND hwnd,
    DWORD grfFlags, struct IEnumIDList **ppenumIDList);

LWSTDAPI_(BOOL) SHIsExpandableFolder(struct IShellFolder *psf, LPCITEMIDLIST pidl);
LWSTDAPI IContextMenu_Invoke(struct IContextMenu* pcm, HWND hwndOwner, LPCSTR pVerb, UINT fFlags);

#undef LPCITEMIDLIST
#undef  LPITEMIDLIST

#ifdef UNICODE
// SHTruncateString takes a BUFFER SIZE, so subtract 1 to properly null terminate.
//
#define SHTruncateString(wzStr, cch)            ((cch) ? ((wzStr)[cch-1]=L'\0', (cch-1)) : 0)
#else
LWSTDAPI_(int)  SHTruncateString(CHAR *sz, int cchBufferSize);
#endif // UNICODE

// SHFormatDateTime flags
//  (FDTF_SHORTDATE and FDTF_LONGDATE are mutually exclusive, as is
//   FDTF_SHORTIME and FDTF_LONGTIME.)
//
#define FDTF_SHORTTIME      0x00000001      // eg, "7:48 PM"
#define FDTF_SHORTDATE      0x00000002      // eg, "3/29/98"
#define FDTF_DEFAULT        (FDTF_SHORTDATE | FDTF_SHORTTIME) // eg, "3/29/98 7:48 PM"
#define FDTF_LONGDATE       0x00000004      // eg, "Monday, March 29, 1998"
#define FDTF_LONGTIME       0x00000008      // eg. "7:48:33 PM"
#define FDTF_RELATIVE       0x00000010      // uses "Yesterday", etc. if possible
#define FDTF_LTRDATE        0x00000100      // Left To Right reading order
#define FDTF_RTLDATE        0x00000200      // Right To Left reading order

LWSTDAPI_(int)  SHFormatDateTime%(const FILETIME * pft, DWORD * pdwFlags, LPTSTR% pszBuf, UINT cchBuf);

LWSTDAPI_(SECURITY_ATTRIBUTES*) CreateAllAccessSecurityAttributes(SECURITY_ATTRIBUTES* psa, SECURITY_DESCRIPTOR* psd, PACL *ppacl);

LWSTDAPI_(int)  SHAnsiToUnicode(LPCSTR pszSrc, LPWSTR pwszDst, int cwchBuf);
LWSTDAPI_(int)  SHAnsiToUnicodeCP(UINT uiCP, LPCSTR pszSrc, LPWSTR pwszDst, int cwchBuf);
LWSTDAPI_(int)  SHAnsiToAnsi(LPCSTR pszSrc, LPSTR pszDst, int cchBuf);
LWSTDAPI_(int)  SHUnicodeToAnsi(LPCWSTR pwszSrc, LPSTR pszDst, int cchBuf);
LWSTDAPI_(int)  SHUnicodeToAnsiCP(UINT uiCP, LPCWSTR pwszSrc, LPSTR pszDst, int cchBuf);
LWSTDAPI_(int)  SHUnicodeToUnicode(LPCWSTR pwzSrc, LPWSTR pwzDst, int cwchBuf);
LWSTDAPI_(BOOL) DoesStringRoundTripA(LPCSTR pwszIn, LPSTR pszOut, UINT cchOut);
LWSTDAPI_(BOOL) DoesStringRoundTripW(LPCWSTR pwszIn, LPSTR pszOut, UINT cchOut);
#ifdef UNICODE
#define DoesStringRoundTrip     DoesStringRoundTripW
#else
#define DoesStringRoundTrip     DoesStringRoundTripA
#endif

// The return value from all SH<Type>To<Type> is the size of szDest including the terminater.
#ifdef UNICODE
#define SHTCharToUnicode(wzSrc, wzDest, cchSize)                SHUnicodeToUnicode(wzSrc, wzDest, cchSize)
#define SHTCharToUnicodeCP(uiCP, wzSrc, wzDest, cchSize)        SHUnicodeToUnicode(wzSrc, wzDest, cchSize)
#define SHTCharToAnsi(wzSrc, szDest, cchSize)                   SHUnicodeToAnsi(wzSrc, szDest, cchSize)
#define SHTCharToAnsiCP(uiCP, wzSrc, szDest, cchSize)           SHUnicodeToAnsiCP(uiCP, wzSrc, szDest, cchSize)
#define SHUnicodeToTChar(wzSrc, wzDest, cchSize)                SHUnicodeToUnicode(wzSrc, wzDest, cchSize)
#define SHUnicodeToTCharCP(uiCP, wzSrc, wzDest, cchSize)        SHUnicodeToUnicode(wzSrc, wzDest, cchSize)
#define SHAnsiToTChar(szSrc, wzDest, cchSize)                   SHAnsiToUnicode(szSrc, wzDest, cchSize)
#define SHAnsiToTCharCP(uiCP, szSrc, wzDest, cchSize)           SHAnsiToUnicodeCP(uiCP, szSrc, wzDest, cchSize)
#define SHOtherToTChar(szSrc, szDest, cchSize)                  SHAnsiToUnicode(szSrc, szDest, cchSize)
#define SHTCharToOther(szSrc, szDest, cchSize)                  SHUnicodeToAnsi(szSrc, szDest, cchSize)
#else // UNICODE
#define SHTCharToUnicode(szSrc, wzDest, cchSize)                SHAnsiToUnicode(szSrc, wzDest, cchSize)
#define SHTCharToUnicodeCP(uiCP, szSrc, wzDest, cchSize)        SHAnsiToUnicodeCP(uiCP, szSrc, wzDest, cchSize)
#define SHTCharToAnsi(szSrc, szDest, cchSize)                   SHAnsiToAnsi(szSrc, szDest, cchSize)
#define SHTCharToAnsiCP(uiCP, szSrc, szDest, cchSize)           SHAnsiToAnsi(szSrc, szDest, cchSize)
#define SHUnicodeToTChar(wzSrc, szDest, cchSize)                SHUnicodeToAnsi(wzSrc, szDest, cchSize)
#define SHUnicodeToTCharCP(uiCP, wzSrc, szDest, cchSize)        SHUnicodeToAnsiCP(uiCP, wzSrc, szDest, cchSize)
#define SHAnsiToTChar(szSrc, szDest, cchSize)                   SHAnsiToAnsi(szSrc, szDest, cchSize)
#define SHAnsiToTCharCP(uiCP, szSrc, szDest, cchSize)           SHAnsiToAnsi(szSrc, szDest, cchSize)
#define SHOtherToTChar(szSrc, szDest, cchSize)                  SHUnicodeToAnsi(szSrc, szDest, cchSize)
#define SHTCharToOther(szSrc, szDest, cchSize)                  SHAnsiToUnicode(szSrc, szDest, cchSize)
#endif // UNICODE

LWSTDAPI_(BOOL)    SHRegisterClassA(const WNDCLASSA* pwc);
LWSTDAPI_(BOOL)    SHRegisterClassW(const WNDCLASSW* pwc);
LWSTDAPI_(void)    SHUnregisterClassesA(HINSTANCE hinst, const LPCSTR *rgpszClasses, UINT cpsz);
LWSTDAPI_(void)    SHUnregisterClassesW(HINSTANCE hinst, const LPCWSTR *rgpszClasses, UINT cpsz);
LWSTDAPI_(int) SHMessageBoxCheckW(HWND hwnd, LPCWSTR pszText, LPCWSTR pszCaption, UINT uType, int iDefault, LPCWSTR pszRegVal);
LWSTDAPI_(int) SHMessageBoxCheckA(HWND hwnd, LPCSTR pszText, LPCSTR pszCaption, UINT uType, int iDefault, LPCSTR pszRegVal);
LWSTDAPI_(void) SHRestrictedMessageBox(HWND hwnd);
LWSTDAPI_(HMENU) SHGetMenuFromID(HMENU hmMain, UINT uID);
LWSTDAPI_(int) SHMenuIndexFromID(HMENU hm, UINT id);
LWSTDAPI_(void) SHRemoveDefaultDialogFont(HWND hDlg);
LWSTDAPI_(void) SHSetDefaultDialogFont(HWND hDlg, int idCtl);
LWSTDAPI_(void) SHRemoveAllSubMenus(HMENU hmenu);
LWSTDAPI_(void) SHEnableMenuItem(HMENU hmenu, UINT id, BOOL fEnable);
LWSTDAPI_(void) SHCheckMenuItem(HMENU hmenu, UINT id, BOOL fChecked);
LWSTDAPI_(void) SHSetWindowBits(HWND hWnd, int iWhich, DWORD dwBits, DWORD dwValue);
LWSTDAPI_(HMENU) SHLoadMenuPopup(HINSTANCE hinst, UINT id);
LWSTDAPI_(void) SHPropagateMessage(HWND hwndParent, UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL fSend);
LWSTDAPI_(void) SHSetParentHwnd(HWND hwnd, HWND hwndParent);
LWSTDAPI_(UINT) SHGetCurColorRes();
LWSTDAPI_(DWORD) SHWaitForSendMessageThread(HANDLE hThread, DWORD dwTimeout);
LWSTDAPI_(BOOL) SHVerbExistsNA(LPCSTR szExtension, LPCSTR pszVerb, LPSTR pszCommand, DWORD cchCommand);
LWSTDAPI_(void) SHFillRectClr(HDC hdc, LPRECT prc, COLORREF clr);
LWSTDAPI_(int) SHSearchMapInt(const int *src, const int *dst, int cnt, int val);
LWSTDAPI_(CHAR) SHStripMneumonicA(LPSTR pszMenu);
LWSTDAPI_(WCHAR) SHStripMneumonicW(LPWSTR pszMenu);
LWSTDAPI SHIsChildOrSelf(HWND hwndParent, HWND hwnd);
LWSTDAPI_(DWORD) SHGetValueGoodBootA(HKEY hkeyParent, LPCSTR pcszSubKey,
                                   LPCSTR pcszValue, PDWORD pdwValueType,
                                   PBYTE pbyteBuf, PDWORD pdwcbBufLen);
LWSTDAPI_(DWORD) SHGetValueGoodBootW(HKEY hkeyParent, LPCWSTR pcwzSubKey,
                                   LPCWSTR pcwzValue, PDWORD pdwValueType,
                                   PBYTE pbyteBuf, PDWORD pdwcbBufLen);
LWSTDAPI_(LRESULT) SHDefWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);


LWSTDAPI_(BOOL) SHGetFileDescription%(LPCTSTR% pszPath, LPCTSTR% pszVersionKeyIn, LPCTSTR% pszCutListIn, LPTSTR% pszDesc, UINT *pcchDesc);

LWSTDAPI_(int) SHMessageBoxCheckEx%(HWND hwnd, HINSTANCE hinst, LPCTSTR% pszTemplateName, DLGPROC pDlgProc, LPVOID pData, int iDefault, LPCTSTR% pszRegVal);

#define IDC_MESSAGEBOXCHECKEX 0x1202

// Prevents shell hang do to hung window on broadcast
LWSTDAPI_(LRESULT) SHSendMessageBroadcast%(UINT uMsg, WPARAM wParam, LPARAM lParam);

#ifdef UNICODE
#define SHGetValueGoodBoot      SHGetValueGoodBootW
#define SHStripMneumonic        SHStripMneumonicW
#define SHMessageBoxCheck       SHMessageBoxCheckW
#define SHRegisterClass         SHRegisterClassW
#define SHUnregisterClasses     SHUnregisterClassesW
#define SHSendMessageBroadcast  SHSendMessageBroadcastW
#else // UNICODE
#define SHGetValueGoodBoot      SHGetValueGoodBootA
#define SHStripMneumonic        SHStripMneumonicA
#define SHMessageBoxCheck       SHMessageBoxCheckA
#define SHRegisterClass         SHRegisterClassA
#define SHUnregisterClasses     SHUnregisterClassesA
#define SHSendMessageBroadcast  SHSendMessageBroadcastA
#endif // UNICODE





// Returns TRUE/FALSE depending on question

#define OS_WINDOWS                  0           // windows vs. NT
#define OS_NT                       1           // windows vs. NT
#define OS_WIN95                    2           // Win95 or greater
#define OS_NT4                      3           // NT4 or greater
#define OS_NT5                      4           // NT5 or greater
#define OS_MEMPHIS                  5           // Win98 or greater
#define OS_MEMPHIS_GOLD             6           // Win98 Gold
#define OS_WIN2000                  7           // Some derivative of Win2000
#define OS_WIN2000PRO               8           // Windows 2000 Professional (Workstation)
#define OS_WIN2000SERVER            9           // Windows 2000 Server
#define OS_WIN2000ADVSERVER         10          // Windows 2000 Advanced Server
#define OS_WIN2000DATACENTER        11          // Windows 2000 Data Center Server
#define OS_WIN2000TERMINAL          12          // Windows 2000 Terminal Server
#define OS_WIN2000EMBED             13          // Windows 2000 Embedded Edition
#define OS_TERMINALCLIENT           14          // Windows Terminal Client (Not version specific)
#define OS_TERMINALREMOTEADMIN      15          // Windows 2000 Terminal Server in "Remote Administration" mode
#define OS_WIN95GOLD                16          // Windows 95 Gold (Version 4.0 Build 1995)

#define OS_SERVERAPPLIANCE          21          // Server Appliance (based on Windows 2000 Advanced Server)


LWSTDAPI_(BOOL) IsOS(DWORD dwOS);

///// BEGIN Private CommandTarget helpers
//***   IOleCommandTarget helpers {

//***   octd -- OleCT direction
// NOTES
//  used both as a return value from IsXxxForward, and as an iUpDown
//  param for MayXxxForward.
enum octd {
    // do *not* change these values; we rely upon all 3 of:
    //  - sign +/-
    //  - powers of 2
    //  - (?) broadcast > down
    OCTD_DOWN=+1,
    OCTD_DOWNBROADCAST=+2,
    OCTD_UP=-1
};


#ifndef RC_INVOKED /* { rc doesn't like these long symbol names */
#ifdef __IOleCommandTarget_INTERFACE_DEFINED__
    HRESULT IsQSForward(const GUID *pguidCmdGroup, int cCmds, OLECMD rgCmds[]);
    // WARNING: note the hoaky cast of nCmdID to a struct ptr
    #define IsExecForward(pguidCmdGroup, nCmdID) \
        IsQSForward(pguidCmdGroup, 1, (OLECMD *) &nCmdID)

    HRESULT MayQSForward(IUnknown *punk, int iUpDown, const GUID *pguidCmdGroup,
        ULONG cCmds, OLECMD rgCmds[], OLECMDTEXT *pcmdtext);
    HRESULT MayExecForward(IUnknown *punk, int iUpDown, const GUID *pguidCmdGroup,
        DWORD nCmdID, DWORD nCmdexecopt, VARIANTARG *pvarargIn, VARIANTARG *pvarargOut);
#endif //__IOleCommandTarget_INTERFACE_DEFINED__
#endif  /* } !RC_INVOKED */
// }
///// end



#ifdef WANT_SHLWAPI_POSTSPLIT

typedef struct _FDSA {
    // cItem *must* be at beginning of struct for GetItemCount() to work
    int     cItem;          // # elements
    void *  aItem;          // data for elements (either static or dynamic)
    int     cItemAlloc;     // # of elements currently alloc'ed (>= cItem)
    int     cItemGrow:8;    // # of elements to grow cItemAlloc by
    int     cbItem:8;       // sizeof element
    DWORD   fAllocated:1;   // 1:overflowed from static to dynamic array
    DWORD     unused:15;
} FDSA, *PFDSA;

LWSTDAPI_(BOOL)  FDSA_Initialize(int cbItem, int cItemGrow, PFDSA pfdsa, void * aItemStatic, int cItemStatic);
LWSTDAPI_(BOOL)  FDSA_Destroy(PFDSA pfdsa);
LWSTDAPI_(int)   FDSA_InsertItem(PFDSA pfdsa, int index, void * pitem);
LWSTDAPI_(BOOL)  FDSA_DeleteItem(PFDSA pfdsa, int index);

#define FDSA_AppendItem(pfdsa, pitem)       FDSA_InsertItem(pfdsa, DA_LAST, pitem)
#define FDSA_GetItemPtr(pfdsa, i, type)     (&(((type *)((pfdsa)->aItem))[(i)]))
#define FDSA_GetItemCount(hdsa)      (*(int *)(hdsa))
#endif // WANT_SHLWAPI_POSTSPLIT




#if defined( __LPGUID_DEFINED__ )
// Copied from OLE source code
// format for string form of GUID is:
// ????{%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}
#define GUIDSTR_MAX (1+ 8 + 1 + 4 + 1 + 4 + 1 + 4 + 1 + 12 + 1 + 1)

LWSTDAPI_(BOOL) GUIDFromString%(LPCTSTR% psz, LPGUID pguid);

#endif

#ifdef _REFGUID_DEFINED
LWSTDAPI_(int) SHStringFromGUID%(UNALIGNED REFGUID rguid, LPTSTR% psz, int cchMax);

LWSTDAPI SHRegGetCLSIDKey%(UNALIGNED REFGUID rguid, LPCTSTR% lpszSubKey, BOOL fUserSpecific, BOOL fCreate, HKEY *phkey);

LWSTDAPI_(HANDLE) SHGlobalCounterCreate(REFGUID rguid);
LWSTDAPI_(HANDLE) SHGlobalCounterCreateNamed%(LPCTSTR% szName, LONG lInitialValue);
LWSTDAPI_(long) SHGlobalCounterGetValue(HANDLE hCounter);
LWSTDAPI_(long) SHGlobalCounterIncrement(HANDLE hCounter);
LWSTDAPI_(long) SHGlobalCounterDecrement(HANDLE hCounter);
#define         SHGlobalCounterDestroy      CloseHandle
#endif

// WNDPROCs are thunked by user to send ANSI/UNICODE messages (ex: WM_WININICHANGE)
// so providing a W version that automatically thunks to the A version
// is dangerous.  but we do it anyway.  if a caller needs to work on both win95 and NT
// it needs to be aware that on win95 the W version actually calls the A version.
// thus all worker windows on win95 are ANSI.  this should rarely affect worker wndprocs
// because they are internal, and the messages are usually custom.  but system messages
// like WM_WININICHANGE, and the WM_DDE* messages will be changed accordingly
HWND SHCreateWorkerWindowA(WNDPROC pfnWndProc, HWND hwndParent, DWORD dwExStyle, DWORD dwFlags, HMENU hmenu, void * p);
HWND SHCreateWorkerWindowW(WNDPROC pfnWndProc, HWND hwndParent, DWORD dwExStyle, DWORD dwFlags, HMENU hmenu, void * p);
#ifdef UNICODE
#define SHCreateWorkerWindow SHCreateWorkerWindowW
#else
#define SHCreateWorkerWindow SHCreateWorkerWindowA
#endif

BOOL    SHAboutInfoA(LPSTR lpszInfo, DWORD cchSize);
BOOL    SHAboutInfoW(LPWSTR lpszInfo, DWORD cchSize);

#ifdef UNICODE
#define SHAboutInfo SHAboutInfoW
#else
#define SHAboutInfo SHAboutInfoA
#endif

// Types for SHIsLowMemoryMachine
#define ILMM_IE4    0       // 1997-style machine

LWSTDAPI_(BOOL) SHIsLowMemoryMachine(DWORD dwType);

LWSTDAPI_(HINSTANCE) SHPinDllOfCLSID(const CLSID *pclsid);

// Menu Helpers
LWSTDAPI_(int)  GetMenuPosFromID(HMENU hmenu, UINT id);

LWSTDAPI        SHGetInverseCMAP(BYTE *pbMap, ULONG cbMap);

//
// Shared memory apis
//

LWSTDAPI_(HANDLE)   SHAllocShared(LPCVOID lpvData, DWORD dwSize, DWORD dwProcessId);
LWSTDAPI_(BOOL)     SHFreeShared(HANDLE hData,DWORD dwProcessId);
LWSTDAPI_(void *)   SHLockShared(HANDLE hData, DWORD dwProcessId);
LWSTDAPI_(BOOL)     SHUnlockShared(void *lpvData);
LWSTDAPI_(HANDLE)   SHMapHandle(HANDLE h, DWORD dwProcSrc, DWORD dwProcDest, DWORD dwDesiredAccess, DWORD dwFlags);

#ifdef UNIX
#include <urlmon.h>
#endif /* UNIX */

//
//  Zone Security APIs
//
LWSTDAPI ZoneCheckPathA(LPCSTR pszPath, DWORD dwActionType, DWORD dwFlags, IInternetSecurityMgrSite * pisms);
LWSTDAPI ZoneCheckPathW(LPCWSTR pwzPath, DWORD dwActionType, DWORD dwFlags, IInternetSecurityMgrSite * pisms);

LWSTDAPI ZoneCheckUrlA(LPCSTR pszUrl, DWORD dwActionType, DWORD dwFlags, IInternetSecurityMgrSite * pisms);
LWSTDAPI ZoneCheckUrlW(LPCWSTR pwzUrl, DWORD dwActionType, DWORD dwFlags, IInternetSecurityMgrSite * pisms);
LWSTDAPI ZoneCheckUrlExA(LPCSTR pszUrl, DWORD * pdwPolicy, DWORD dwPolicySize, DWORD * pdwContext, DWORD dwContextSize, DWORD dwActionType, DWORD dwFlags, IInternetSecurityMgrSite * pisms);
LWSTDAPI ZoneCheckUrlExW(LPCWSTR pwzUrl, DWORD * pdwPolicy, DWORD dwPolicySize, DWORD * pdwContext, DWORD dwContextSize, DWORD dwActionType, DWORD dwFlags, IInternetSecurityMgrSite * pisms);
LWSTDAPI ZoneCheckUrlExCacheA(LPCSTR pszUrl, DWORD * pdwPolicy, DWORD dwPolicySize, DWORD * pdwContext, DWORD dwContextSize,
                            DWORD dwActionType, DWORD dwFlags, IInternetSecurityMgrSite * pisms, IInternetSecurityManager ** ppismCache);
LWSTDAPI ZoneCheckUrlExCacheW(LPCWSTR pwzUrl, DWORD * pdwPolicy, DWORD dwPolicySize, DWORD * pdwContext, DWORD dwContextSize,
                            DWORD dwActionType, DWORD dwFlags, IInternetSecurityMgrSite * pisms, IInternetSecurityManager ** ppismCache);

LWSTDAPI ZoneCheckHost(IInternetHostSecurityManager * pihsm, DWORD dwActionType, DWORD dwFlags);
LWSTDAPI ZoneCheckHostEx(IInternetHostSecurityManager * pihsm, DWORD * pdwPolicy, DWORD dwPolicySize, DWORD * pdwContext,
                        DWORD dwContextSize, DWORD dwActionType, DWORD dwFlags);
LWSTDAPI_(int) ZoneComputePaneSize(HWND hwndStatus);
LWSTDAPI_(void) ZoneConfigureW(HWND hwnd, LPCWSTR pwszUrl);

#ifdef UNICODE
#define ZoneCheckUrl            ZoneCheckUrlW
#define ZoneCheckPath           ZoneCheckPathW
#define ZoneCheckUrlEx          ZoneCheckUrlExW
#define ZoneCheckUrlExCache     ZoneCheckUrlExCacheW
#else // UNICODE
#define ZoneCheckUrl            ZoneCheckUrlA
#define ZoneCheckPath           ZoneCheckPathA
#define ZoneCheckUrlEx          ZoneCheckUrlExA
#define ZoneCheckUrlExCache     ZoneCheckUrlExCacheA
#endif // UNICODE

LWSTDAPI SHRegisterValidateTemplate(LPCWSTR pwzTemplate, DWORD dwFlags);

// Flags for SHRegisterValidateTemplate
#define SHRVT_REGISTER                  0x00000001
#define SHRVT_VALIDATE                  0x00000002
#define SHRVT_PROMPTUSER                0x00000004
#define SHRVT_REGISTERIFPROMPTOK        0x00000008
#define SHRVT_VALID                     0x0000000f

BOOL RegisterGlobalHotkeyW(WORD wOldHotkey, WORD wNewHotkey,LPCWSTR pcwszPath);
BOOL RegisterGlobalHotkeyA(WORD wOldHotkey, WORD wNewHotkey,LPCSTR pcszPath);

LWSTDAPI_(UINT) WhichPlatform(void);

// Return values of WhichPlatform
#define PLATFORM_UNKNOWN     0
#define PLATFORM_IE3         1      // obsolete: use PLATFORM_BROWSERONLY
#define PLATFORM_BROWSERONLY 1      // browser-only (no new shell)
#define PLATFORM_INTEGRATED  2      // integrated shell

#ifdef UNICODE
#define RegisterGlobalHotkey    RegisterGlobalHotkeyW
#else // UNICODE
#define RegisterGlobalHotkey    RegisterGlobalHotkeyA
#endif // UNICODE

// qistub {

//***   QueryInterface helpers
// NOTES
//  ATL has a fancier version of this.  if we need to extend ours, we
//  should probably just switch to ATL's rather than reinvent.
// EXAMPLE
//  Cfoo::QI(REFIID riid, void **ppv)
//  {
//      // (the IID_xxx comments make grep'ing work!)
//      static const QITAB qit = {
//          QITABENT(Cfoo, Iiface1),    // IID_Iiface1
//          ...
//          QITABENT(Cfoo, IifaceN),    // IID_IifaceN
//          { 0 },                      // n.b. don't forget the 0
//      };
//
//      // n.b. make sure you don't cast 'this'
//      hr = QISearch(this, qit, riid, ppv);
//      if (FAILED(hr))
//          hr = SUPER::QI(riid, ppv);
//      // custom code could be added here for FAILED() case
//      return hr;
//  }

typedef struct
{
    const IID * piid;
    int         dwOffset;
} QITAB, *LPQITAB;
typedef const QITAB *LPCQITAB;

#define QITABENTMULTI(Cthis, Ifoo, Iimpl) \
    { (IID*) &IID_##Ifoo, OFFSETOFCLASS(Iimpl, Cthis) }

#define QITABENTMULTI2(Cthis, Ifoo, Iimpl) \
    { (IID*) &Ifoo, OFFSETOFCLASS(Iimpl, Cthis) }

#define QITABENT(Cthis, Ifoo) QITABENTMULTI(Cthis, Ifoo, Ifoo)

STDAPI QISearch(void* that, LPCQITAB pqit, REFIID riid, void **ppv);


#ifndef STATIC_CAST
//***   STATIC_CAST -- 'portable' static_cast<>
// NOTES
//  do *not* use SAFE_CAST (see comment in OFFSETOFCLASS)
#define STATIC_CAST(typ)   static_cast<typ>
#ifndef _X86_
    // assume only intel compiler (>=vc5) supports static_cast for now
    // we could key off of _MSC_VER >= 1100 but i'm not sure that will work
    //
    // a straight cast will give the correct result but no error checking,
    // so we'll have to catch errors on intel.
    #undef  STATIC_CAST
    #define STATIC_CAST(typ)   (typ)
#endif
#endif

#ifndef OFFSETOFCLASS
//***   OFFSETOFCLASS -- (stolen from ATL)
// we use STATIC_CAST not SAFE_CAST because the compiler gets confused
// (it doesn't constant-fold the ,-op in SAFE_CAST so we end up generating
// code for the table!)

#define OFFSETOFCLASS(base, derived) \
    ((DWORD)(DWORD_PTR)(STATIC_CAST(base*)((derived*)8))-8)
#endif

// } qistub


#if (_WIN32_IE >= 0x0500)

// SHRestrictionLookup
typedef struct
{
    INT     iFlag;
    LPCWSTR pszKey;
    LPCWSTR pszValue;
} SHRESTRICTIONITEMS;

LWSTDAPI_(DWORD) SHRestrictionLookup(INT iFlag, LPCWSTR pszBaseKey,
                                     const SHRESTRICTIONITEMS *pRestrictions,
                                     DWORD* rdwRestrictionItemValues);
LWSTDAPI_(DWORD) SHGetRestriction(LPCWSTR pszBaseKey, LPCWSTR pszGroup, LPCWSTR pszSubKey);

typedef INT_PTR (CALLBACK* SHDLGPROC)(void *lpData, HWND, UINT, WPARAM, LPARAM);
LWSTDAPI_(INT_PTR) SHDialogBox(HINSTANCE hInstance, LPCWSTR lpTemplateName,
    HWND hwndParent, SHDLGPROC lpDlgFunc, void*lpData);

LWSTDAPI SHInvokeDefaultCommand(HWND hwnd, struct IShellFolder* psf, const UNALIGNED struct _ITEMIDLIST *pidl);
LWSTDAPI SHInvokeCommand(HWND hwnd, struct IShellFolder* psf, const UNALIGNED struct _ITEMIDLIST *pidl, LPCSTR lpVerb);

#endif // _WIN32_IE >= 0x0500

//============= Internal Routines that are always to be built ================
LWSTDAPI_(DWORD)
GetLongPathNameWrapW(
        LPCWSTR lpszShortPath,
        LPWSTR lpszLongPath,
        DWORD cchBuffer);

LWSTDAPI_(DWORD)
GetLongPathNameWrapA(
        LPCSTR lpszShortPath,
        LPSTR lpszLongPath,
        DWORD cchBuffer);

#ifdef UNICODE
#define GetLongPathNameWrap         GetLongPathNameWrapW
#else
#define GetLongPathNameWrap         GetLongPathNameWrapA
#endif //UNICODE


//=============== Unicode Wrapper Routines ===================================

#if (_WIN32_IE >= 0x0500) && !defined(NO_SHLWAPI_UNITHUNK)

//
//  There are two styles of usage for the wrap functions.
//
//  *   Explicit wrapping.
//
//      If you explicitly call GetPropWrap (for example), then
//      your UNICODE build will call the wrapper function, and your ANSI
//      build will call the normal ANSI API directly.
//
//      Calls to GetProp, GetPropW, and GetPropA still go
//      directly to the underlying system DLL that implements them.
//
//      This lets you select which calls to UNICODE APIs should get
//      wrapped and which should go straight through to the OS
//      (and most likely fail on Win95).
//
//  *   Automatic wrapping.
//
//      If you #include <w95wraps.h>, then when you call GetProp,
//      your UNICODE build will call the wrapper function, and your ANSI
//      ANSI build will call the normal ANSI API directly.
//
//      This lets you just call the UNICODE APIs normally throughout
//      your code, and the wrappers will do their best.
//
//  Here's a table explaining what you get under the various scenarios.
//
//                    You Get
//                                                <w95wraps.h>  <w95wraps.h>
//      You Write     UNICODE       ANSI          UNICODE       ANSI
//      ============  ============  ============  ============  ============
//      GetProp       GetPropW      GetPropA      GetPropWrapW  GetPropA
//      GetPropWrap   GetPropWrapW  GetPropA      GetPropWrapW  GetPropA
//
//      GetPropW      GetPropW      GetPropW      GetPropWrapW  GetPropWrapW
//      GetPropA      GetPropA      GetPropA      GetPropA      GetPropA
//      GetPropWrapW  GetPropWrapW  GetPropWrapW  GetPropWrapW  GetPropWrapW
//      GetPropWrapA  GetPropA      GetPropA      GetPropA      GetPropA
//
//  Final quirk:  If you are running on a non-x86 platform, then the
//  wrap functions are forwarded to the unwrapped functions, since
//  the only OS that runs on non-x86 is NT.
//
//  Before using the wrapper functions, see the warnings at the top of
//  <w95wraps.h> to make sure you understand the consequences.
//
LWSTDAPI_(BOOL) IsCharAlphaWrapW(IN WCHAR ch);
LWSTDAPI_(BOOL) IsCharUpperWrapW(IN WCHAR ch);
LWSTDAPI_(BOOL) IsCharLowerWrapW(IN WCHAR ch);
LWSTDAPI_(BOOL) IsCharAlphaNumericWrapW(IN WCHAR ch);

LWSTDAPI_(BOOL)
AppendMenuWrapW(
    IN HMENU hMenu,
    IN UINT uFlags,
    IN UINT_PTR uIDNewItem,
    IN LPCWSTR lpNewItem
    );

LWSTDAPI_(LRESULT)
CallWindowProcWrapW(
    WNDPROC lpPrevWndFunc,
    HWND    hWnd,
    UINT    Msg,
    WPARAM  wParam,
    LPARAM  lParam);

#ifdef POST_IE5_BETA
LWSTDAPI_(BOOL) CallMsgFilterWrapW(LPMSG lpMsg, int nCode);
#endif

LWSTDAPI_(LPWSTR) CharLowerWrapW( LPWSTR pch );

LWSTDAPI_(DWORD)  CharLowerBuffWrapW( LPWSTR pch, DWORD cchLength );

LWSTDAPI_(LPWSTR) CharNextWrapW(LPCWSTR lpszCurrent);
LWSTDAPI_(LPWSTR) CharPrevWrapW(LPCWSTR lpszStart, LPCWSTR lpszCurrent);
LWSTDAPI_(BOOL)   CharToOemWrapW(LPCWSTR lpszSrc, LPSTR lpszDst);
LWSTDAPI_(LPWSTR) CharUpperWrapW( LPWSTR pch );
LWSTDAPI_(DWORD)  CharUpperBuffWrapW( LPWSTR pch, DWORD cchLength );

LWSTDAPI_(HRESULT) CLSIDFromStringWrap(LPOLESTR lpsz, LPCLSID pclsid);
LWSTDAPI_(HRESULT) CLSIDFromProgIDWrap(LPCOLESTR lpszProgID, LPCLSID lpclsid);

LWSTDAPI_(int)
CompareStringWrapW(
    LCID     Locale,
    DWORD    dwCmpFlags,
    LPCWSTR lpString1,
    int      cchCount1,
    LPCWSTR lpString2,
    int      cchCount2);

LWSTDAPI_(int)
CopyAcceleratorTableWrapW(
        HACCEL  hAccelSrc,
        LPACCEL lpAccelDst,
        int     cAccelEntries);

LWSTDAPI_(HACCEL)
CreateAcceleratorTableWrapW(LPACCEL lpAccel, int cEntries);

LWSTDAPI_(HDC)
CreateDCWrapW(
        LPCWSTR             lpszDriver,
        LPCWSTR             lpszDevice,
        LPCWSTR             lpszOutput,
        CONST DEVMODEW *    lpInitData);

LWSTDAPI_(BOOL)
CreateDirectoryWrapW(
        LPCWSTR                 lpPathName,
        LPSECURITY_ATTRIBUTES   lpSecurityAttributes);

LWSTDAPI_(HANDLE)
CreateEventWrapW(
        LPSECURITY_ATTRIBUTES   lpEventAttributes,
        BOOL                    bManualReset,
        BOOL                    bInitialState,
        LPCWSTR                 lpName);

LWSTDAPI_(HANDLE)
CreateFileWrapW(
        LPCWSTR                 lpFileName,
        DWORD                   dwDesiredAccess,
        DWORD                   dwShareMode,
        LPSECURITY_ATTRIBUTES   lpSecurityAttributes,
        DWORD                   dwCreationDisposition,
        DWORD                   dwFlagsAndAttributes,
        HANDLE                  hTemplateFile);


LWSTDAPI_(HFONT)
CreateFontIndirectWrapW(CONST LOGFONTW * plfw);

LWSTDAPI_(HDC)
CreateICWrapW(
        LPCWSTR             lpszDriver,
        LPCWSTR             lpszDevice,
        LPCWSTR             lpszOutput,
        CONST DEVMODEW *    lpInitData);

LWSTDAPI_(HWND)
CreateWindowExWrapW(
        DWORD       dwExStyle,
        LPCWSTR     lpClassName,
        LPCWSTR     lpWindowName,
        DWORD       dwStyle,
        int         X,
        int         Y,
        int         nWidth,
        int         nHeight,
        HWND        hWndParent,
        HMENU       hMenu,
        HINSTANCE   hInstance,
        void *     lpParam);

LWSTDAPI_(LRESULT)
DefWindowProcWrapW(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LWSTDAPI_(BOOL) DeleteFileWrapW(LPCWSTR pwsz);

LWSTDAPI_(LRESULT)
DispatchMessageWrapW(CONST MSG * lpMsg);

LWSTDAPI_(int)
DrawTextWrapW(
        HDC     hDC,
        LPCWSTR lpString,
        int     nCount,
        LPRECT  lpRect,
        UINT    uFormat);

LWSTDAPI_(int)
EnumFontFamiliesWrapW(
        HDC          hdc,
        LPCWSTR      lpszFamily,
        FONTENUMPROCW lpEnumFontProc,
        LPARAM       lParam);

LWSTDAPI_(int)
EnumFontFamiliesExWrapW(
        HDC          hdc,
        LPLOGFONTW   lplfw,
        FONTENUMPROCW lpEnumFontProc,
        LPARAM       lParam,
        DWORD        dwFlags );

LWSTDAPI_(BOOL)
EnumResourceNamesWrapW(
        HINSTANCE        hModule,
        LPCWSTR          lpType,
        ENUMRESNAMEPROCW lpEnumFunc,
        LONG_PTR         lParam);

LWSTDAPI_(BOOL)
ExtTextOutWrapW(
        HDC             hdc,
        int             x,
        int             y,
        UINT            fuOptions,
        CONST RECT *    lprc,
        LPCWSTR         lpString,
        UINT            nCount,
        CONST INT *     lpDx);

LWSTDAPI_(HANDLE)
FindFirstFileWrapW(
        LPCWSTR             lpFileName,
        LPWIN32_FIND_DATAW  pwszFd);

LWSTDAPI_(HRSRC)
FindResourceWrapW(HINSTANCE hModule, LPCWSTR lpName, LPCWSTR lpType);

LWSTDAPI_(HWND)
FindWindowWrapW(LPCWSTR lpClassName, LPCWSTR lpWindowName);

LWSTDAPI_(DWORD)
FormatMessageWrapW(
    DWORD       dwFlags,
    LPCVOID     lpSource,
    DWORD       dwMessageId,
    DWORD       dwLanguageId,
    LPWSTR      lpBuffer,
    DWORD       nSize,
    va_list *   Arguments);

LWSTDAPI_(BOOL)
GetClassInfoWrapW(HINSTANCE hModule, LPCWSTR lpClassName, LPWNDCLASSW lpWndClassW);

LWSTDAPI_(DWORD)
GetClassLongWrapW(HWND hWnd, int nIndex);

LWSTDAPI_(int)
GetClassNameWrapW(HWND hWnd, LPWSTR lpClassName, int nMaxCount);

LWSTDAPI_(int)
GetClipboardFormatNameWrapW(UINT format, LPWSTR lpFormatName, int cchFormatName);

LWSTDAPI_(DWORD)
GetCurrentDirectoryWrapW(DWORD nBufferLength, LPWSTR lpBuffer);

LWSTDAPI_(UINT)
GetDlgItemTextWrapW(
        HWND    hWndDlg,
        int     idControl,
        LPWSTR  lpsz,
        int     cchMax);

LWSTDAPI_(DWORD)
GetFileAttributesWrapW(LPCWSTR lpFileName);

STDAPI_(BOOL)
GetFileVersionInfoWrapW(LPWSTR pwzFilename, DWORD dwHandle, DWORD dwLen, LPVOID lpData);

STDAPI_(DWORD)
GetFileVersionInfoSizeWrapW(LPWSTR pwzFilename,  LPDWORD lpdwHandle);

LWSTDAPI_(DWORD)
GetFullPathNameWrapW( LPCWSTR lpFileName,
                     DWORD  nBufferLength,
                     LPWSTR lpBuffer,
                     LPWSTR *lpFilePart);

LWSTDAPI_(int)
GetLocaleInfoWrapW(LCID Locale, LCTYPE LCType, LPWSTR lpsz, int cchData);

LWSTDAPI_(int)
GetMenuStringWrapW(
        HMENU   hMenu,
        UINT    uIDItem,
        LPWSTR  lpString,
        int     nMaxCount,
        UINT    uFlag);

LWSTDAPI_(BOOL)
GetMessageWrapW(
        LPMSG   lpMsg,
        HWND    hWnd,
        UINT    wMsgFilterMin,
        UINT    wMsgFilterMax);

LWSTDAPI_(DWORD)
GetModuleFileNameWrapW(HINSTANCE hModule, LPWSTR pwszFilename, DWORD nSize);

LWSTDAPI_(UINT)
GetSystemDirectoryWrapW(LPWSTR lpBuffer, UINT uSize);

LWSTDAPI_(DWORD)
GetEnvironmentVariableWrapW(LPCWSTR lpName, LPWSTR lpBuffer, DWORD nSize);

LWSTDAPI_(DWORD)
SearchPathWrapW(
        LPCWSTR lpPathName,
        LPCWSTR lpFileName,
        LPCWSTR lpExtension,
        DWORD   cchReturnBuffer,
        LPWSTR  lpReturnBuffer,
        LPWSTR *  plpfilePart);

LWSTDAPI_(HMODULE)
GetModuleHandleWrapW(LPCWSTR lpModuleName);

LWSTDAPI_(int)
GetObjectWrapW(HGDIOBJ hgdiObj, int cbBuffer, void *lpvObj);

LWSTDAPI_(UINT)
GetPrivateProfileIntWrapW(
        LPCWSTR lpAppName,
        LPCWSTR lpKeyName,
        INT     nDefault,
        LPCWSTR lpFileName);

LWSTDAPI_(DWORD)
GetProfileStringWrapW(
        LPCWSTR lpAppName,
        LPCWSTR lpKeyName,
        LPCWSTR lpDefault,
        LPWSTR  lpBuffer,
        DWORD   dwBuffersize);

LWSTDAPI_(HANDLE)
GetPropWrapW(HWND hWnd, LPCWSTR lpString);

LWSTDAPI_(DWORD)
GetShortPathNameWrapW(
    LPCWSTR lpszLongPath,
    LPWSTR  lpszShortPath,
    DWORD    cchBuffer);

LWSTDAPI_(BOOL)
GetStringTypeExWrapW(LCID lcid, DWORD dwInfoType, LPCWSTR lpSrcStr, int cchSrc, LPWORD lpCharType);

LWSTDAPI_(UINT)
GetTempFileNameWrapW(
        LPCWSTR lpPathName,
        LPCWSTR lpPrefixString,
        UINT    uUnique,
        LPWSTR  lpTempFileName);

LWSTDAPI_(DWORD)
GetTempPathWrapW(DWORD nBufferLength, LPWSTR lpBuffer);

LWSTDAPI_(BOOL)
GetTextExtentPoint32WrapW(
        HDC     hdc,
        LPCWSTR pwsz,
        int     cb,
        LPSIZE  pSize);

LWSTDAPI_(int)
GetTextFaceWrapW(
        HDC    hdc,
        int    cch,
        LPWSTR lpFaceName);

LWSTDAPI_(BOOL)
GetTextMetricsWrapW(HDC hdc, LPTEXTMETRICW lptm);

LWSTDAPI_(BOOL)
GetUserNameWrapW(LPWSTR lpUserName, LPDWORD lpcchName);

LWSTDAPI_(LONG)
GetWindowLongWrapW(HWND hWnd, int nIndex);


LWSTDAPI_(int)
GetWindowTextWrapW(HWND hWnd, LPWSTR lpString, int nMaxCount);

LWSTDAPI_(int)
GetWindowTextLengthWrapW(HWND hWnd);

LWSTDAPI_(UINT)
GetWindowsDirectoryWrapW(LPWSTR lpWinPath, UINT cch);

LWSTDAPI_(BOOL)
InsertMenuWrapW(
        HMENU   hMenu,
        UINT    uPosition,
        UINT    uFlags,
        UINT_PTR  uIDNewItem,
        LPCWSTR lpNewItem);

LWSTDAPI_(BOOL)
IsDialogMessageWrapW(HWND hWndDlg, LPMSG lpMsg);

LWSTDAPI_(HACCEL)
LoadAcceleratorsWrapW(HINSTANCE hInstance, LPCWSTR lpTableName);

LWSTDAPI_(HBITMAP)
LoadBitmapWrapW(HINSTANCE hInstance, LPCWSTR lpBitmapName);

LWSTDAPI_(HCURSOR)
LoadCursorWrapW(HINSTANCE hInstance, LPCWSTR lpCursorName);

LWSTDAPI_(HICON)
LoadIconWrapW(HINSTANCE hInstance, LPCWSTR lpIconName);

LWSTDAPI_(HANDLE)
LoadImageWrapA(
        HINSTANCE hInstance,
        LPCSTR lpName,
        UINT uType,
        int cxDesired,
        int cyDesired,
        UINT fuLoad);

LWSTDAPI_(HANDLE)
LoadImageWrapW(
        HINSTANCE hInstance,
        LPCWSTR lpName,
        UINT uType,
        int cxDesired,
        int cyDesired,
        UINT fuLoad);

LWSTDAPI_(HINSTANCE)
LoadLibraryExWrapW(
        LPCWSTR lpLibFileName,
        HANDLE  hFile,
        DWORD   dwFlags);

LWSTDAPI_(HMENU)
LoadMenuWrapW(HINSTANCE hInstance, LPCWSTR lpMenuName);

LWSTDAPI_(int)
LoadStringWrapW(HINSTANCE hInstance, UINT uID, LPWSTR lpBuffer, int nBufferMax);

#ifndef UNIX
LWSTDAPI_(BOOL)
MessageBoxIndirectWrapW(CONST MSGBOXPARAMSW *pmbp);
#else
LWSTDAPI_(int)
MessageBoxIndirectWrapW(LPMSGBOXPARAMSW pmbp);
#endif /* UNIX */

LWSTDAPI_(BOOL)
ModifyMenuWrapW(
        HMENU   hMenu,
        UINT    uPosition,
        UINT    uFlags,
        UINT_PTR uIDNewItem,
        LPCWSTR lpNewItem);

LWSTDAPI_(BOOL)
GetCharWidth32WrapW(
     HDC hdc,
     UINT iFirstChar,
     UINT iLastChar,
     LPINT lpBuffer);

LWSTDAPI_(DWORD)
GetCharacterPlacementWrapW(
    HDC hdc,            // handle to device context
    LPCWSTR lpString,   // pointer to string
    int nCount,         // number of characters in string
    int nMaxExtent,     // maximum extent for displayed string
    LPGCP_RESULTSW lpResults, // pointer to buffer for placement result
    DWORD dwFlags       // placement flags
   );

LWSTDAPI_(BOOL)
CopyFileWrapW(LPCWSTR lpExistingFileName, LPCWSTR lpNewFileName, BOOL bFailIfExists);

LWSTDAPI_(BOOL)
MoveFileWrapW(LPCWSTR lpExistingFileName, LPCWSTR lpNewFileName);

LWSTDAPI_(BOOL)
OemToCharWrapW(LPCSTR lpszSrc, LPWSTR lpszDst);

LWSTDAPI_(HANDLE)
OpenEventWrapW(
        DWORD                   fdwAccess,
        BOOL                    fInherit,
        LPCWSTR                 lpszEventName);


LWSTDAPI_(void)
OutputDebugStringWrapW(LPCWSTR lpOutputString);

LWSTDAPI_(BOOL)
PeekMessageWrapW(
        LPMSG   lpMsg,
        HWND    hWnd,
        UINT    wMsgFilterMin,
        UINT    wMsgFilterMax,
        UINT    wRemoveMsg);

LWSTDAPI_(BOOL)
PlaySoundWrapW(
        LPCWSTR pszSound,
        HMODULE hmod,
        DWORD fdwSound);

LWSTDAPI_(BOOL)
PostMessageWrapW(
        HWND    hWnd,
        UINT    Msg,
        WPARAM  wParam,
        LPARAM  lParam);

LWSTDAPI_(BOOL)
PostThreadMessageWrapW(
        DWORD idThread,
        UINT Msg,
        WPARAM wParam,
        LPARAM lParam);

LWSTDAPI_(LONG)
RegCreateKeyWrapW(HKEY hKey, LPCWSTR lpSubKey, PHKEY phkResult);

LWSTDAPI_(LONG)
RegCreateKeyExWrapW(HKEY hKey, LPCWSTR lpSubKey, DWORD Reserved, LPWSTR lpClass, DWORD dwOptions, REGSAM samDesired, LPSECURITY_ATTRIBUTES lpSecurityAttributes, PHKEY phkResult, LPDWORD lpdwDisposition);

LWSTDAPI_(LONG)
RegDeleteKeyWrapW(HKEY hKey, LPCWSTR pwszSubKey);

LWSTDAPI_(LONG)
RegDeleteValueWrapW(HKEY hKey, LPCWSTR pwszSubKey);

LWSTDAPI_(LONG)
RegEnumKeyWrapW(
        HKEY    hKey,
        DWORD   dwIndex,
        LPWSTR  lpName,
        DWORD   cbName);

LWSTDAPI_(LONG)
RegEnumKeyExWrapW(
        HKEY        hKey,
        DWORD       dwIndex,
        LPWSTR      lpName,
        LPDWORD     lpcbName,
        LPDWORD     lpReserved,
        LPWSTR      lpClass,
        LPDWORD     lpcbClass,
        PFILETIME   lpftLastWriteTime);

LWSTDAPI_(LONG)
RegOpenKeyWrapW(HKEY hKey, LPCWSTR pwszSubKey, PHKEY phkResult);

LWSTDAPI_(LONG)
RegOpenKeyExWrapW(
        HKEY    hKey,
        LPCWSTR lpSubKey,
        DWORD   ulOptions,
        REGSAM  samDesired,
        PHKEY   phkResult);

LWSTDAPI_(LONG)
RegQueryInfoKeyWrapW(
        HKEY hKey,
        LPWSTR lpClass,
        LPDWORD lpcbClass,
        LPDWORD lpReserved,
        LPDWORD lpcSubKeys,
        LPDWORD lpcbMaxSubKeyLen,
        LPDWORD lpcbMaxClassLen,
        LPDWORD lpcValues,
        LPDWORD lpcbMaxValueNameLen,
        LPDWORD lpcbMaxValueLen,
        LPDWORD lpcbSecurityDescriptor,
        PFILETIME lpftLastWriteTime);

LWSTDAPI_(LONG)
RegQueryValueWrapW(
        HKEY    hKey,
        LPCWSTR pwszSubKey,
        LPWSTR  pwszValue,
        PLONG   lpcbValue);

LWSTDAPI_(LONG)
RegQueryValueExWrapW(
        HKEY    hKey,
        LPCWSTR lpValueName,
        LPDWORD lpReserved,
        LPDWORD lpType,
        LPBYTE  lpData,
        LPDWORD lpcbData);

LWSTDAPI_(LONG)
RegSetValueWrapW(
        HKEY    hKey,
        LPCWSTR lpSubKey,
        DWORD   dwType,
        LPCWSTR lpData,
        DWORD   cbData);

LWSTDAPI_(LONG)
RegSetValueExWrapW(
        HKEY        hKey,
        LPCWSTR     lpValueName,
        DWORD       Reserved,
        DWORD       dwType,
        CONST BYTE* lpData,
        DWORD       cbData);

LWSTDAPI_(ATOM)
RegisterClassWrapW(CONST WNDCLASSW * lpWndClass);

LWSTDAPI_(UINT)
RegisterClipboardFormatWrapW(LPCWSTR lpString);

LWSTDAPI_(UINT)
RegisterWindowMessageWrapW(LPCWSTR lpString);

LWSTDAPI_(BOOL)
RemoveDirectoryWrapW(LPCWSTR lpszDir);

LWSTDAPI_(HANDLE)
RemovePropWrapW(
        HWND    hWnd,
        LPCWSTR lpString);

LWSTDAPI_(LRESULT)
SendDlgItemMessageWrapW(
        HWND    hDlg,
        int     nIDDlgItem,
        UINT    Msg,
        WPARAM  wParam,
        LPARAM  lParam);

LWSTDAPI_(LRESULT)
SendMessageWrapW(
        HWND    hWnd,
        UINT    Msg,
        WPARAM  wParam,
        LPARAM  lParam);

LWSTDAPI_(LRESULT)
SendMessageTimeoutWrapW(
        HWND    hWnd,
        UINT    Msg,
        WPARAM  wParam,
        LPARAM  lParam,
        UINT    uFlags,
        UINT    uTimeout,
        PULONG_PTR lpdwResult);

LWSTDAPI_(BOOL)
SetCurrentDirectoryWrapW(LPCWSTR lpszCurDir);

LWSTDAPI_(BOOL)
SetDlgItemTextWrapW(HWND hDlg, int nIDDlgItem, LPCWSTR lpString);

LWSTDAPI_(BOOL)
SetMenuItemInfoWrapW(
    HMENU hMenu,
    UINT uItem,
    BOOL fByPosition,
    LPCMENUITEMINFOW lpmiiW);

LWSTDAPI_(BOOL)
SetPropWrapW(
    HWND    hWnd,
    LPCWSTR lpString,
    HANDLE  hData);

LWSTDAPI_(LONG)
SetWindowLongWrapW(HWND hWnd, int nIndex, LONG dwNewLong);

LWSTDAPI_(HHOOK)
SetWindowsHookExWrapW(
    int idHook,
    HOOKPROC lpfn,
    HINSTANCE hmod,
    DWORD dwThreadId);

LWSTDAPI_(int)
StartDocWrapW( HDC hDC, const DOCINFOW * lpdi );

LWSTDAPI_(BOOL)
SystemParametersInfoWrapW(
        UINT    uiAction,
        UINT    uiParam,
        void    *pvParam,
        UINT    fWinIni);

LWSTDAPI_(BOOL)
TrackPopupMenuWrap(HMENU hMenu, UINT uFlags, int x, int y, int nReserved, HWND hWnd, CONST RECT *prcRect);

LWSTDAPI_(BOOL)
TrackPopupMenuExWrap(HMENU hMenu, UINT uFlags, int x, int y, HWND hWnd, LPTPMPARAMS lptpm);

LWSTDAPI_(int)
TranslateAcceleratorWrapW(HWND hWnd, HACCEL hAccTable, LPMSG lpMsg);

LWSTDAPI_(BOOL)
UnregisterClassWrapW(LPCWSTR lpClassName, HINSTANCE hInstance);

STDAPI_(BOOL)
VerQueryValueWrapW(const LPVOID pBlock, LPWSTR pwzSubBlock, LPVOID *ppBuffer, PUINT puLen);

LWSTDAPI_(SHORT)
VkKeyScanWrapW(WCHAR ch);

LWSTDAPI_(BOOL)
WinHelpWrapW(HWND hwnd, LPCWSTR szFile, UINT uCmd, DWORD_PTR dwData);

LWSTDAPI_(int)
wvsprintfWrapW(LPWSTR pwszOut, LPCWSTR pwszFormat, va_list arglist);

STDAPI_(DWORD) WNetRestoreConnectionWrapW(IN HWND hwndParent, IN LPCWSTR pwzDevice);
STDAPI_(DWORD) WNetGetLastErrorWrapW(OUT LPDWORD pdwError, OUT LPWSTR pwzErrorBuf, IN DWORD cchErrorBufSize, OUT LPWSTR pwzNameBuf, IN DWORD cchNameBufSize);

LWSTDAPI_(int) DrawTextExWrapW(HDC hdc, LPWSTR pwzText, int cchText, LPRECT lprc, UINT dwDTFormat, LPDRAWTEXTPARAMS lpDTParams);
LWSTDAPI_(BOOL) GetMenuItemInfoWrapW(HMENU hMenu, UINT uItem, BOOL fByPosition, LPMENUITEMINFOW pmiiW);
LWSTDAPI_(BOOL) InsertMenuItemWrapW(HMENU hMenu, UINT uItem, BOOL fByPosition, LPCMENUITEMINFOW pmiiW);

LWSTDAPI_(HFONT) CreateFontWrapW(int nHeight, int nWidth, int nEscapement, int nOrientation, int fnWeight, DWORD fdwItalic, DWORD fdwUnderline,
                    DWORD fdwStrikeOut, DWORD fdwCharSet, DWORD fdwOutputPrecision, DWORD fdwClipPrecision,
                    DWORD fdwQuality, DWORD fdwPitchAndFamily, LPCWSTR lpszFace);
LWSTDAPI_(HDC) CreateMetaFileWrapW(LPCWSTR pwzFile);
LWSTDAPI_(HANDLE) CreateMutexWrapW(LPSECURITY_ATTRIBUTES lpMutexAttributes, BOOL bInitialOwner, LPCWSTR pwzName);
LWSTDAPI_(DWORD) ExpandEnvironmentStringsWrapW(LPCWSTR pwszSrc, LPWSTR pwszDst, DWORD cchSize);
LWSTDAPI_(DWORD) SHExpandEnvironmentStrings%(LPCTSTR% pszSrc, LPTSTR% pszDst, DWORD cchSize);
LWSTDAPI_(DWORD) SHExpandEnvironmentStringsForUser%(HANDLE hToken, LPCTSTR% pszSrc, LPTSTR% pszDst, DWORD cchSize);

LWSTDAPI_(HANDLE) CreateSemaphoreWrapW(LPSECURITY_ATTRIBUTES lpSemaphoreAttributes, LONG lInitialCount, LONG lMaximumCount, LPCWSTR pwzName);
LWSTDAPI_(BOOL) IsBadStringPtrWrapW(LPCWSTR pwzString, UINT_PTR ucchMax);
LWSTDAPI_(HINSTANCE) LoadLibraryWrapW(LPCWSTR pwzLibFileName);
LWSTDAPI_(int) GetTimeFormatWrapW(LCID Locale, DWORD dwFlags, CONST SYSTEMTIME * lpTime, LPCWSTR pwzFormat, LPWSTR pwzTimeStr, int cchTime);
LWSTDAPI_(int) GetDateFormatWrapW(LCID Locale, DWORD dwFlags, CONST SYSTEMTIME * lpDate, LPCWSTR pwzFormat, LPWSTR pwzDateStr, int cchDate);
LWSTDAPI_(DWORD) GetPrivateProfileStringWrapW(LPCWSTR pwzAppName, LPCWSTR pwzKeyName, LPCWSTR pwzDefault, LPWSTR pwzReturnedString, DWORD cchSize, LPCWSTR pwzFileName);
LWSTDAPI_(BOOL) WritePrivateProfileStringWrapW(LPCWSTR pwzAppName, LPCWSTR pwzKeyName, LPCWSTR pwzString, LPCWSTR pwzFileName);

#ifndef SHFILEINFO_DEFINED
#define SHFILEINFO_DEFINED

/*
 * The SHGetFileInfo API provides an easy way to get attributes
 * for a file given a pathname.
 *
 *   PARAMETERS
 *
 *     pszPath              file name to get info about
 *     dwFileAttributes     file attribs, only used with SHGFI_USEFILEATTRIBUTES
 *     psfi                 place to return file info
 *     cbFileInfo           size of structure
 *     uFlags               flags
 *
 *   RETURN
 *     TRUE if things worked
 */

typedef struct _SHFILEINFO%
{
    HICON       hIcon;                      // out: icon
    int         iIcon;                      // out: icon index
    DWORD       dwAttributes;               // out: SFGAO_ flags
    TCHAR%      szDisplayName[MAX_PATH];    // out: display name (or path)
    TCHAR%      szTypeName[80];             // out: type name
} SHFILEINFO%;


// NOTE: This is also in shellapi.h.  Please keep in synch.
#endif // !SHFILEINFO_DEFINED
LWSTDAPI_(DWORD_PTR) SHGetFileInfoWrapW(LPCWSTR pwzPath, DWORD dwFileAttributes, SHFILEINFOW *psfi, UINT cbFileInfo, UINT uFlags);

LWSTDAPI_(ATOM) RegisterClassExWrapW(CONST WNDCLASSEXW *pwcx);
LWSTDAPI_(BOOL) GetClassInfoExWrapW(HINSTANCE hinst, LPCWSTR pwzClass, LPWNDCLASSEXW lpwcx);

// This allows us to be included either before or after shellapi.h
#ifdef STRICT
LWSTDAPI_(UINT) DragQueryFileWrapW(struct HDROP__*,UINT,LPWSTR,UINT);
#else
LWSTDAPI_(UINT) DragQueryFileWrapW(HANDLE,UINT,LPWSTR,UINT);
#endif

LWSTDAPI_(HWND) FindWindowExWrapW(HWND hwndParent, HWND hwndChildAfter, LPCWSTR pwzClassName, LPCWSTR pwzWindowName);
LWSTDAPI_(UNALIGNED struct _ITEMIDLIST *) SHBrowseForFolderWrapW(struct _browseinfoW * pbiW);
LWSTDAPI_(BOOL) SHGetPathFromIDListWrapW(CONST UNALIGNED struct _ITEMIDLIST * pidl, LPWSTR pwzPath);
LWSTDAPI_(BOOL) SHGetNewLinkInfoWrapW(LPCWSTR pszpdlLinkTo, LPCWSTR pszDir, LPWSTR pszName, BOOL *pfMustCopy, UINT uFlags);
LWSTDAPI SHDefExtractIconWrapW(LPCWSTR pszIconFile, int iIndex, UINT uFlags, HICON *phiconLarge, HICON *phiconSmall, UINT nIconSize);
LWSTDAPI_(BOOL) GetUserNameWrapW(LPWSTR pszBuffer, LPDWORD pcch);
LWSTDAPI_(LONG) RegEnumValueWrapW(HKEY hkey, DWORD dwIndex, LPWSTR lpValueName, LPDWORD lpcbValueName, LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData);
LWSTDAPI_(BOOL) WritePrivateProfileStructWrapW(LPCWSTR lpszSection, LPCWSTR lpszKey, LPVOID lpStruct, UINT uSizeStruct, LPCWSTR szFile);
LWSTDAPI_(BOOL) GetPrivateProfileStructWrapW(LPCWSTR lpszSection, LPCWSTR lpszKey, LPVOID lpStruct, UINT uSizeStruct, LPCWSTR szFile);
LWSTDAPI_(BOOL) CreateProcessWrapW(LPCWSTR lpApplicationName, LPWSTR lpCommandLine, LPSECURITY_ATTRIBUTES lpProcessAttributes,
LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment, LPCWSTR lpCurrentDirectory,
LPSTARTUPINFOW lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation);
LWSTDAPI_(HICON) ExtractIconWrapW(HINSTANCE hInst, LPCWSTR lpszExeFileName, UINT nIconIndex);
#ifndef WIN32_LEAN_AND_MEAN
STDAPI_(UINT) DdeInitializeWrapW(LPDWORD pidInst, PFNCALLBACK pfnCallback, DWORD afCmd, DWORD ulRes);
STDAPI_(HSZ) DdeCreateStringHandleWrapW(DWORD idInst, LPCWSTR psz, int iCodePage);
STDAPI_(DWORD) DdeQueryStringWrapW(DWORD idInst, HSZ hsz, LPWSTR psz, DWORD cchMax, int iCodePage);
STDAPI_(BOOL) GetSaveFileNameWrapW(LPOPENFILENAMEW lpofn);
STDAPI_(BOOL) GetOpenFileNameWrapW(LPOPENFILENAMEW lpofn);
STDAPI_(BOOL) PrintDlgWrapW(LPPRINTDLGW lppd);
STDAPI_(BOOL) PageSetupDlgWrapW(LPPAGESETUPDLGW lppsd);
#endif
LWSTDAPI_(void) SHChangeNotifyWrap(LONG wEventId, UINT uFlags, LPCVOID dwItem1, LPCVOID dwItem2);
LWSTDAPI_(void) SHFlushSFCacheWrap(void);
LWSTDAPI_(BOOL) ShellExecuteExWrapW(struct _SHELLEXECUTEINFOW * pExecInfoW);
LWSTDAPI_(int) SHFileOperationWrapW(struct _SHFILEOPSTRUCTW * pFileOpW);
LWSTDAPI_(UINT) ExtractIconExWrapW(LPCWSTR pwzFile, int nIconIndex, HICON *phiconLarge, HICON *phiconSmall, UINT nIcons);
LWSTDAPI_(BOOL) SetFileAttributesWrapW(LPCWSTR pwzFile, DWORD dwFileAttributes);
LWSTDAPI_(int) GetNumberFormatWrapW(LCID Locale, DWORD dwFlags, LPCWSTR pwzValue, CONST NUMBERFMTW * pFormatW, LPWSTR pwzNumberStr, int cchNumber);
LWSTDAPI_(int) MessageBoxWrapW(HWND hwnd, LPCWSTR pwzText, LPCWSTR pwzCaption, UINT uType);
LWSTDAPI_(BOOL) FindNextFileWrapW(HANDLE hSearchHandle, LPWIN32_FIND_DATAW pFindFileDataW);

#ifdef UNICODE

#define IsCharAlphaWrap             IsCharAlphaWrapW
#define IsCharUpperWrap             IsCharUpperWrapW
#define IsCharLowerWrap             IsCharLowerWrapW
#define IsCharAlphaNumericWrap      IsCharAlphaNumericWrapW
#define AppendMenuWrap              AppendMenuWrapW
#ifdef POST_IE5_BETA
#define CallMsgFilterWrap           CallMsgFilterWrapW
#endif
#define CallWindowProcWrap          CallWindowProcWrapW
#define CharLowerWrap               CharLowerWrapW
#define CharLowerBuffWrap           CharLowerBuffWrapW
#define CharNextWrap                CharNextWrapW
#define CharPrevWrap                CharPrevWrapW
#define CharToOemWrap               CharToOemWrapW
#define CharUpperWrap               CharUpperWrapW
#define CharUpperBuffWrap           CharUpperBuffWrapW
#define CompareStringWrap           CompareStringWrapW
#define CopyAcceleratorTableWrap    CopyAcceleratorTableWrapW
#define CreateAcceleratorTableWrap  CreateAcceleratorTableWrapW
#define CreateDCWrap                CreateDCWrapW
#define CreateDirectoryWrap         CreateDirectoryWrapW
#define CreateEventWrap             CreateEventWrapW
#define CreateFontWrap              CreateFontWrapW
#define CreateFileWrap              CreateFileWrapW
#define CreateFontIndirectWrap      CreateFontIndirectWrapW
#define CreateICWrap                CreateICWrapW
#define CreateMetaFileWrap          CreateMetaFileWrapW
#define CreateMutexWrap             CreateMutexWrapW
#define CreateSemaphoreWrap         CreateSemaphoreWrapW
#define CreateWindowExWrap          CreateWindowExWrapW
#define DefWindowProcWrap           DefWindowProcWrapW
#define DeleteFileWrap              DeleteFileWrapW
#define DispatchMessageWrap         DispatchMessageWrapW
#define DrawTextExWrap              DrawTextExWrapW
#define DrawTextWrap                DrawTextWrapW
#define EnumFontFamiliesWrap        EnumFontFamiliesWrapW
#define EnumFontFamiliesExWrap      EnumFontFamiliesExWrapW
#define EnumResourceNamesWrap       EnumResourceNamesWrapW
#define ExpandEnvironmentStringsWrap ExpandEnvironmentStringsWrapW
#define ExtractIconExWrap           ExtractIconExWrapW
#define ExtTextOutWrap              ExtTextOutW
#define FindFirstFileWrap           FindFirstFileWrapW
#define FindNextFileWrap            FindNextFileWrapW
#define FindResourceWrap            FindResourceWrapW
#define FindWindowWrap              FindWindowWrapW
#define FindWindowExWrap            FindWindowExWrapW
#define FormatMessageWrap           FormatMessageWrapW
#define GetClassInfoWrap            GetClassInfoWrapW
#define GetClassInfoExWrap          GetClassInfoExWrapW
#define GetClassLongWrap            GetClassLongWrapW
#define GetClassNameWrap            GetClassNameWrapW
#define GetClipboardFormatNameWrap  GetClipboardFormatNameWrapW
#define GetCurrentDirectoryWrap     GetCurrentDirectoryWrapW
#define GetDlgItemTextWrap          GetDlgItemTextWrapW
#define GetFileAttributesWrap       GetFileAttributesWrapW
#define GetFullPathNameWrap         GetFullPathNameWrapW
#define GetLocaleInfoWrap           GetLocaleInfoWrapW
#define GetMenuItemInfoWrap         GetMenuItemInfoWrapW
#define GetMenuStringWrap           GetMenuStringWrapW
#define GetMessageWrap              GetMessageWrapW
#define GetModuleFileNameWrap       GetModuleFileNameWrapW
#define GetNumberFormatWrap         GetNumberFormatWrapW
#define GetSystemDirectoryWrap      GetSystemDirectoryWrapW
#define GetEnvironmentVariableWrap  GetEnvironmentVariableWrapW
#define GetModuleHandleWrap         GetModuleHandleWrapW
#define GetObjectWrap               GetObjectWrapW
#define GetPrivateProfileIntWrap    GetPrivateProfileIntWrapW
#define GetProfileStringWrap        GetProfileStringWrapW
#define GetPrivateProfileStringWrap GetPrivateProfileStringWrapW
#define WritePrivateProfileStringWrap WritePrivateProfileStringWrapW
#define GetPropWrap                 GetPropWrapW
#define GetStringTypeExWrap         GetStringTypeExWrapW
#define GetTempFileNameWrap         GetTempFileNameWrapW
#define GetTempPathWrap             GetTempPathWrapW
#define GetTextExtentPoint32Wrap    GetTextExtentPoint32WrapW
#define GetTextFaceWrap             GetTextFaceWrapW
#define GetTextMetricsWrap          GetTextMetricsWrapW
#define GetTimeFormatWrap           GetTimeFormatWrapW
#define GetDateFormatWrap           GetDateFormatWrapW
#define GetUserNameWrap             GetUserNameWrapW
#define GetWindowLongWrap           GetWindowLongWrapW
#define GetWindowTextWrap           GetWindowTextWrapW
#define GetWindowTextLengthWrap     GetWindowTextLengthWrapW
#define GetWindowsDirectoryWrap     GetWindowsDirectoryWrapW
#define InsertMenuItemWrap          InsertMenuItemWrapW
#define InsertMenuWrap              InsertMenuWrapW
#define IsBadStringPtrWrap          IsBadStringPtrWrapW
#define IsDialogMessageWrap         IsDialogMessageWrapW
#define LoadAcceleratorsWrap        LoadAcceleratorsWrapW
#define LoadBitmapWrap              LoadBitmapWrapW
#define LoadCursorWrap              LoadCursorWrapW
#define LoadIconWrap                LoadIconWrapW
#define LoadImageWrap               LoadImageWrapW
#define LoadLibraryWrap             LoadLibraryWrapW
#define LoadLibraryExWrap           LoadLibraryExWrapW
#define LoadMenuWrap                LoadMenuWrapW
#define LoadStringWrap              LoadStringWrapW
#define MessageBoxIndirectWrap      MessageBoxIndirectWrapW
#define MessageBoxWrap              MessageBoxWrapW
#define ModifyMenuWrap              ModifyMenuWrapW
#define GetCharWidth32Wrap          GetCharWidth32WrapW
#define GetCharacterPlacementWrap   GetCharacterPlacementWrapW
#define CopyFileWrap                CopyFileWrapW
#define MoveFileWrap                MoveFileWrapW
#define OemToCharWrap               OemToCharWrapW
#define OutputDebugStringWrap       OutputDebugStringWrapW
#define PeekMessageWrap             PeekMessageWrapW
#define PostMessageWrap             PostMessageWrapW
#define PostThreadMessageWrap       PostThreadMessageWrapW
#define RegCreateKeyWrap            RegCreateKeyWrapW
#define RegCreateKeyExWrap          RegCreateKeyExWrapW
#define RegDeleteKeyWrap            RegDeleteKeyWrapW
#define RegDeleteValueWrap          RegDeleteValueWrapW
#define RegEnumKeyWrap              RegEnumKeyWrapW
#define RegEnumKeyExWrap            RegEnumKeyExWrapW
#define RegOpenKeyWrap              RegOpenKeyWrapW
#define RegOpenKeyExWrap            RegOpenKeyExWrapW
#define RegQueryInfoKeyWrap         RegQueryInfoKeyWrapW
#define RegQueryValueWrap           RegQueryValueWrapW
#define RegQueryValueExWrap         RegQueryValueExWrapW
#define RegSetValueWrap             RegSetValueWrapW
#define RegSetValueExWrap           RegSetValueExWrapW
#define RegisterClassWrap           RegisterClassWrapW
#define RegisterClassExWrap         RegisterClassExWrapW
#define RegisterClipboardFormatWrap RegisterClipboardFormatWrapW
#define RegisterWindowMessageWrap   RegisterWindowMessageWrapW
#define RemovePropWrap              RemovePropWrapW
#define SearchPathWrap              SearchPathWrapW
#define SendDlgItemMessageWrap      SendDlgItemMessageWrapW
#define SendMessageWrap             SendMessageWrapW
#define SendMessageTimeoutWrap      SendMessageTimeoutWrapW
#define SetCurrentDirectoryWrap     SetCurrentDirectoryWrapW
#define SetDlgItemTextWrap          SetDlgItemTextWrapW
#define SetMenuItemInfoWrap         SetMenuItemInfoWrapW
#define SetPropWrap                 SetPropWrapW
#define SetFileAttributesWrap       SetFileAttributesWrapW
#define SetWindowLongWrap           SetWindowLongWrapW
#define SetWindowsHookExWrap        SetWindowsHookExWrapW
#define SHBrowseForFolderWrap       SHBrowseForFolderWrapW
#define ShellExecuteExWrap          ShellExecuteExWrapW
#define SHFileOperationWrap         SHFileOperationWrapW
#define SHGetFileInfoWrap           SHGetFileInfoWrapW
#define SHGetPathFromIDListWrap     SHGetPathFromIDListWrapW
#define StartDocWrap                StartDocWrapW
#define SystemParametersInfoWrap    SystemParametersInfoWrapW
#define TranslateAcceleratorWrap    TranslateAcceleratorWrapW
#define UnregisterClassWrap         UnregisterClassWrapW
#define VkKeyScanWrap               VkKeyScanWrapW
#define WinHelpWrap                 WinHelpWrapW
#define WNetRestoreConnectionWrap   WNetRestoreConnectionWrapW
#define WNetGetLastErrorWrap        WNetGetLastErrorWrapW
#define wvsprintfWrap               wvsprintfWrapW
#define CreateFontWrap              CreateFontWrapW
#define DrawTextExWrap              DrawTextExWrapW
#define GetMenuItemInfoWrap         GetMenuItemInfoWrapW
#define SetMenuItemInfoWrap         SetMenuItemInfoWrapW
#define InsertMenuItemWrap          InsertMenuItemWrapW
#define DragQueryFileWrap           DragQueryFileWrapW

#else

#define IsCharAlphaWrap             IsCharAlphaA
#define IsCharUpperWrap             IsCharUpperA
#define IsCharLowerWrap             IsCharLowerA
#define IsCharAlphaNumericWrap      IsCharAlphaNumericA
#define AppendMenuWrap              AppendMenuA
#ifdef POST_IE5_BETA
#define CallMsgFilterWrap           CallMsgFilterA
#endif
#define CallWindowProcWrap          CallWindowProcA
#define CharLowerWrap               CharLowerA
#define CharLowerBuffWrap           CharLowerBuffA
#define CharNextWrap                CharNextA
#define CharPrevWrap                CharPrevA
#define CharToOemWrap               CharToOemA
#define CharUpperWrap               CharUpperA
#define CharUpperBuffWrap           CharUpperBuffA
#define CompareStringWrap           CompareStringA
#define CopyAcceleratorTableWrap    CopyAcceleratorTableA
#define CreateAcceleratorTableWrap  CreateAcceleratorTableA
#define CreateDCWrap                CreateDCA
#define CreateDirectoryWrap         CreateDirectoryA
#define CreateEventWrap             CreateEventA
#define CreateFontWrap              CreateFontA
#define CreateFileWrap              CreateFileA
#define CreateFontIndirectWrap      CreateFontIndirectA
#define CreateICWrap                CreateICA
#define CreateMetaFileWrap          CreateMetaFileA
#define CreateMutexWrap             CreateMutexA
#define CreateSemaphoreWrap         CreateSemaphoreA
#define CreateWindowExWrap          CreateWindowExA
#define DefWindowProcWrap           DefWindowProcA
#define DeleteFileWrap              DeleteFileA
#define DispatchMessageWrap         DispatchMessageA
#define DrawTextExWrap              DrawTextExA
#define DrawTextWrap                DrawTextA
#define EnumFontFamiliesWrap        EnumFontFamiliesA
#define EnumFontFamiliesExWrap      EnumFontFamiliesExA
#define EnumResourceNamesWrap       EnumResourceNamesA
#define ExpandEnvironmentStringsWrap ExpandEnvironmentStringsA
#define ExtractIconExWrap           ExtractIconExA
#define ExtTextOutWrap              ExtTextOutA
#define FindFirstFileWrap           FindFirstFileA
#define FindResourceWrap            FindResourceA
#define FindNextFileWrap            FindNextFileA
#define FindWindowWrap              FindWindowA
#define FindWindowExWrap            FindWindowExA
#define FormatMessageWrap           FormatMessageA
#define GetClassInfoWrap            GetClassInfoA
#define GetClassInfoExWrap          GetClassInfoExA
#define GetClassLongWrap            GetClassLongA
#define GetClassNameWrap            GetClassNameA
#define GetClipboardFormatNameWrap  GetClipboardFormatNameA
#define GetCurrentDirectoryWrap     GetCurrentDirectoryA
#define GetDlgItemTextWrap          GetDlgItemTextA
#define GetFileAttributesWrap       GetFileAttributesA
#define GetFullPathNameWrap         GetFullPathNameA
#define GetLocaleInfoWrap           GetLocaleInfoA
#define GetMenuItemInfoWrap         GetMenuItemInfoA
#define GetMenuStringWrap           GetMenuStringA
#define GetMessageWrap              GetMessageA
#define GetModuleFileNameWrap       GetModuleFileNameA
#define GetNumberFormatWrap         GetNumberFormatA
#define GetPrivateProfileStringWrap GetPrivateProfileStringA
#define WritePrivateProfileStringWrap WritePrivateProfileStringA
#define GetSystemDirectoryWrap      GetSystemDirectoryA
#define GetEnvironmentVariableWrap  GetEnvironmentVariableA
#define SearchPathWrap              SearchPathA
#define GetModuleHandleWrap         GetModuleHandleA
#define GetObjectWrap               GetObjectA
#define GetPrivateProfileIntWrap    GetPrivateProfileIntA
#define GetProfileStringWrap        GetProfileStringA
#define GetPropWrap                 GetPropA
#define GetStringTypeExWrap         GetStringTypeExA
#define GetTempFileNameWrap         GetTempFileNameA
#define GetTempPathWrap             GetTempPathA
#define GetTextExtentPoint32Wrap    GetTextExtentPoint32A
#define GetTextFaceWrap             GetTextFaceA
#define GetTextMetricsWrap          GetTextMetricsA
#define GetTimeFormatWrap           GetTimeFormatA
#define GetDateFormatWrap           GetDateFormatA
#define GetUserNameWrap             GetUserNameA
#define GetWindowLongWrap           GetWindowLongA
#define GetWindowTextWrap           GetWindowTextA
#define GetWindowTextLengthWrap     GetWindowTextLengthA
#define GetWindowsDirectoryWrap     GetWindowsDirectoryA
#define InsertMenuItemWrap          InsertMenuItemA
#define InsertMenuWrap              InsertMenuA
#define IsBadStringPtrWrap          IsBadStringPtrA
#define IsDialogMessageWrap         IsDialogMessageA
#define LoadAcceleratorsWrap        LoadAcceleratorsA
#define LoadBitmapWrap              LoadBitmapA
#define LoadCursorWrap              LoadCursorA
#define LoadIconWrap                LoadIconA
#define LoadImageWrap               LoadImageWrapA
#define LoadLibraryWrap             LoadLibraryA
#define LoadLibraryExWrap           LoadLibraryExA
#define LoadMenuWrap                LoadMenuA
#define LoadStringWrap              LoadStringA
#define MessageBoxIndirectWrap      MessageBoxIndirectA
#define MessageBoxWrap              MessageBoxA
#define ModifyMenuWrap              ModifyMenuA
#define GetCharWidth32Wrap          GetCharWidth32A
#define GetCharacterPlacementWrap   GetCharacterPlacementA
#define CopyFileWrap                CopyFileA
#define MoveFileWrap                MoveFileA
#define OemToCharWrap               OemToCharA
#define OutputDebugStringWrap       OutputDebugStringA
#define PeekMessageWrap             PeekMessageA
#define PostMessageWrap             PostMessageA
#define PostThreadMessageWrap       PostThreadMessageA
#define RegCreateKeyWrap            RegCreateKeyA
#define RegCreateKeyExWrap          RegCreateKeyExA
#define RegDeleteKeyWrap            RegDeleteKeyA
#define RegDeleteValueWrap          RegDeleteValueA
#define RegEnumKeyWrap              RegEnumKeyA
#define RegEnumKeyExWrap            RegEnumKeyExA
#define RegOpenKeyWrap              RegOpenKeyA
#define RegOpenKeyExWrap            RegOpenKeyExA
#define RegQueryInfoKeyWrap         RegQueryInfoKeyA
#define RegQueryValueWrap           RegQueryValueA
#define RegQueryValueExWrap         RegQueryValueExA
#define RegSetValueWrap             RegSetValueA
#define RegSetValueExWrap           RegSetValueExA
#define RegisterClassWrap           RegisterClassA
#define RegisterClassExWrap         RegisterClassExA
#define RegisterClipboardFormatWrap RegisterClipboardFormatA
#define RegisterWindowMessageWrap   RegisterWindowMessageA
#define RemovePropWrap              RemovePropA
#define SendDlgItemMessageWrap      SendDlgItemMessageA
#define SendMessageWrap             SendMessageA
#define SendMessageTimeoutWrap      SendMessageTimeoutA
#define SetCurrentDirectoryWrap     SetCurrentDirectoryA
#define SetDlgItemTextWrap          SetDlgItemTextA
#define SetMenuItemInfoWrap         SetMenuItemInfoA
#define SetPropWrap                 SetPropA
#define SetWindowLongWrap           SetWindowLongA
#define SHBrowseForFolderWrap       SHBrowseForFolderA
#define ShellExecuteExWrap          ShellExecuteExA
#define SHFileOperationWrap         SHFileOperationA
#define SHGetFileInfoWrap           SHGetFileInfoA
#define SHGetPathFromIDListWrap     SHGetPathFromIDListA
#define SetFileAttributesWrap       SetFileAttributesA
#define SetWindowsHookExWrap        SetWindowsHookExA
#define StartDocWrap                StartDocA
#define SystemParametersInfoWrap    SystemParametersInfoA
#define TranslateAcceleratorWrap    TranslateAcceleratorA
#define UnregisterClassWrap         UnregisterClassA
#define VkKeyScanWrap               VkKeyScanA
#define WinHelpWrap                 WinHelpA
#define WNetRestoreConnectionWrap   WNetRestoreConnectionA
#define WNetGetLastErrorWrap        WNetGetLastErrorA
#define wvsprintfWrap               wvsprintfA
#define CreateFontWrap              CreateFontA
#define DrawTextExWrap              DrawTextExA
#define GetMenuItemInfoWrap         GetMenuItemInfoA
#define SetMenuItemInfoWrap         SetMenuItemInfoA
#define InsertMenuItemWrap          InsertMenuItemA
#define DragQueryFileWrap           DragQueryFileA
#endif

#endif // (_WIN32_IE >= 0x0500) && !defined(NO_SHLWAPI_UNITHUNK)

#if defined(UNIX) && defined(NO_SHLWAPI_UNITHUNK)
#define SHFlushSFCacheWrap()

#ifdef UNICODE
#define IsCharAlphaWrapW            IsCharAlphaW
#define IsCharUpperWrapW            IsCharUpperW
#define IsCharLowerWrapW            IsCharLowerW
#define IsCharAlphaNumericWrapW     IsCharAlphaNumericW
#define AppendMenuWrapW             AppendMenuW
#ifdef POST_IE5_BETA
#define CallMsgFilterWrapW          CallMsgFilterW
#endif
#define CallWindowProcWrapW         CallWindowProcW
#define CharLowerWrapW              CharLowerW
#define CharLowerBuffWrapW          CharLowerBuffW
#define CharNextWrapW               CharNextW
#define CharPrevWrapW               CharPrevW
#define CharToOemWrapW              CharToOemW
#define CharUpperWrapW              CharUpperW
#define CharUpperBuffWrapW          CharUpperBuffW
#define CompareStringWrapW          CompareStringW
#define CopyAcceleratorTableWrapW   CopyAcceleratorTableW
#define CreateAcceleratorTableWrapW CreateAcceleratorTableW
#define CreateDCWrapW               CreateDCW
#define CreateDirectoryWrapW        CreateDirectoryW
#define CreateEventWrapW            CreateEventW
#define CreateFontWrapW             CreateFontW
#define CreateFileWrapW             CreateFileW
#define CreateFontIndirectWrapW     CreateFontIndirectW
#define CreateICWrapW               CreateICW
#define CreateMetaFileWrapW         CreateMetaFileW
#define CreateMutexWrapW            CreateMutexW
#define CreateSemaphoreWrapW        CreateSemaphoreW
#define CreateWindowExWrapW         CreateWindowExW
#define DefWindowProcWrapW          DefWindowProcW
#define DeleteFileWrapW             DeleteFileW
#define DispatchMessageWrapW        DispatchMessageW
#define DrawTextExWrapW             DrawTextExW
#define DrawTextWrapW               DrawTextW
#define EnumFontFamiliesWrapW       EnumFontFamiliesW
#define EnumFontFamiliesExWrapW     EnumFontFamiliesExW
#define EnumResourceNamesWrapW      EnumResourceNamesW
#define ExpandEnvironmentStringsWrapW ExpandEnvironmentStringsW
#define ExtractIconExWrapW          ExtractIconExW
#define ExtTextOutWrapW             ExtTextOutW
#define FindFirstFileWrapW          FindFirstFileW
#define FindNextFileWrapW           FindNextFileW
#define FindResourceWrapW           FindResourceW
#define FindWindowWrapW             FindWindowW
#define FindWindowExWrapW           FindWindowExW
#define FormatMessageWrapW          FormatMessageW
#define GetClassInfoWrapW           GetClassInfoW
#define GetClassInfoExWrapW         GetClassInfoExW
#define GetClassLongWrapW           GetClassLongW
#define GetClassNameWrapW           GetClassNameW
#define GetClipboardFormatNameWrapW GetClipboardFormatNameW
#define GetCurrentDirectoryWrapW    GetCurrentDirectoryW
#define GetDlgItemTextWrapW         GetDlgItemTextW
#define GetFileAttributesWrapW      GetFileAttributesW
#define GetFullPathNameWrapW        GetFullPathNameW
#define GetLocaleInfoWrapW          GetLocaleInfoW
#define GetMenuStringWrapW          GetMenuStringW
#define GetMessageWrapW             GetMessageW
#define GetModuleFileNameWrapW      GetModuleFileNameW
#define GetNumberFormatWrapW        GetNumberFormatW
#define GetSystemDirectoryWrapW     GetSystemDirectoryW
#define GetModuleHandleWrapW        GetModuleHandleW
#define GetObjectWrapW              GetObjectW
#define GetPrivateProfileIntWrapW   GetPrivateProfileIntW
#define GetProfileStringWrapW       GetProfileStringW
#define GetPrivateProfileStringWrapW GetPrivateProfileStringW
#define WritePrivateProfileStringWrapW WritePrivateProfileStringW
#define GetPropWrapW                GetPropW
#define GetStringTypeExWrapW        GetStringTypeExW
#define GetTempFileNameWrapW        GetTempFileNameW
#define GetTempPathWrapW            GetTempPathW
#define GetTextExtentPoint32WrapW   GetTextExtentPoint32W
#define GetTextFaceWrapW            GetTextFaceW
#define GetTextMetricsWrapW         GetTextMetricsW
#define GetTimeFormatWrapW          GetTimeFormatW
#define GetDateFormatWrapW          GetDateFormatW
#define GetUserNameWrapW            GetUserNameW
#define GetWindowLongWrapW          GetWindowLongW
#define GetWindowTextWrapW          GetWindowTextW
#define GetWindowTextLengthWrapW    GetWindowTextLengthW
#define GetWindowsDirectoryWrapW    GetWindowsDirectoryW
#define InsertMenuItemWrapW         InsertMenuItemW
#define InsertMenuWrapW             InsertMenuW
#define IsBadStringPtrWrapW         IsBadStringPtrW
#define IsDialogMessageWrapW        IsDialogMessageW
#define LoadAcceleratorsWrapW       LoadAcceleratorsW
#define LoadBitmapWrapW             LoadBitmapW
#define LoadCursorWrapW             LoadCursorW
#define LoadIconWrapW               LoadIconW
#define LoadImageWrapW              LoadImageW
#define LoadLibraryWrapW            LoadLibraryW
#define LoadLibraryExWrapW          LoadLibraryExW
#define LoadMenuWrapW               LoadMenuW
#define LoadStringWrapW             LoadStringW
#define MessageBoxIndirectWrapW     MessageBoxIndirectW
#define MessageBoxWrapW             MessageBoxW
#define ModifyMenuWrapW             ModifyMenuW
#define GetCharWidth32WrapW         GetCharWidth32W
#define GetCharacterPlacementWrapW  GetCharacterPlacementW
#define CopyFileWrapW               CopyFileW
#define MoveFileWrapW               MoveFileW
#define OemToCharWrapW              OemToCharW
#define OutputDebugStringWrapW      OutputDebugStringW
#define PeekMessageWrapW            PeekMessageW
#define PostMessageWrapW            PostMessageW
#define PostThreadMessageWrapW      PostThreadMessageW
#define RegCreateKeyWrapW           RegCreateKeyW
#define RegCreateKeyExWrapW         RegCreateKeyExW
#define RegDeleteKeyWrapW           RegDeleteKeyW
#define RegDeleteValueWrapW         RegDeleteValueW
#define RegEnumKeyWrapW             RegEnumKeyW
#define RegEnumKeyExWrapW           RegEnumKeyExW
#define RegOpenKeyWrapW             RegOpenKeyW
#define RegOpenKeyExWrapW           RegOpenKeyExW
#define RegQueryInfoKeyWrapW        RegQueryInfoKeyW
#define RegQueryValueWrapW          RegQueryValueW
#define RegQueryValueExWrapW        RegQueryValueExW
#define RegSetValueWrapW            RegSetValueW
#define RegSetValueExWrapW          RegSetValueExW
#define RegisterClassWrapW          RegisterClassW
#define RegisterClassExWrapW        RegisterClassExW
#define RegisterClipboardFormatWrapWRegisterClipboardFormatW
#define RegisterWindowMessageWrapW  RegisterWindowMessageW
#define RemovePropWrapW             RemovePropW
#define SearchPathWrapW             SearchPathW
#define SendDlgItemMessageWrapW     SendDlgItemMessageW
#define SendMessageWrapW            SendMessageW
#define SetCurrentDirectoryWrapW    SetCurrentDirectoryW
#define SetDlgItemTextWrapW         SetDlgItemTextW
#define SetMenuItemInfoWrapW        SetMenuItemInfoW
#define SetPropWrapW                SetPropW
#define SetFileAttributesWrapW      SetFileAttributesW
#define SetWindowLongWrapW          SetWindowLongW
#define SetWindowsHookExWrapW       SetWindowsHookExW
#define SHBrowseForFolderWrapW      SHBrowseForFolderW
#define ShellExecuteExWrapW         ShellExecuteExW
#define SHFileOperationWrapW        SHFileOperationW
#define SHGetFileInfoWrapW          SHGetFileInfoW
#define SHGetPathFromIDListWrapW    SHGetPathFromIDListW
#define StartDocWrapW               StartDocW
#define SystemParametersInfoWrapW   SystemParametersInfoW
#define TranslateAcceleratorWrapW   TranslateAcceleratorW
#define UnregisterClassWrapW        UnregisterClassW
#define VkKeyScanWrapW              VkKeyScanW
#define WinHelpWrapW                WinHelpW
#define WNetRestoreConnectionWrapW  WNetRestoreConnectionW
#define WNetGetLastErrorWrapW       WNetGetLastErrorW
#define wvsprintfWrapW              wvsprintfW
#define CreateFontWrapW             CreateFontW
#define DrawTextExWrapW             DrawTextExW
#define SetMenuItemInfoWrapW        SetMenuItemInfoW
#define InsertMenuItemWrapW         InsertMenuItemW
#define DragQueryFileWrapW          DragQueryFileW

#define IsCharAlphaWrap             IsCharAlphaW
#define IsCharUpperWrap             IsCharUpperW
#define IsCharLowerWrap             IsCharLowerW
#define IsCharAlphaNumericWrap      IsCharAlphaNumericW
#define AppendMenuWrap              AppendMenuW
#ifdef POST_IE5_BETA
#define CallMsgFilterWrap           CallMsgFilterW
#endif
#define CallWindowProcWrap          CallWindowProcW
#define CharLowerWrap               CharLowerW
#define CharLowerBuffWrap           CharLowerBuffW
#define CharNextWrap                CharNextW
#define CharPrevWrap                CharPrevW
#define CharToOemWrap               CharToOemW
#define CharUpperWrap               CharUpperW
#define CharUpperBuffWrap           CharUpperBuffW
#define CompareStringWrap           CompareStringW
#define CopyAcceleratorTableWrap    CopyAcceleratorTableW
#define CreateAcceleratorTableWrap  CreateAcceleratorTableW
#define CreateDCWrap                CreateDCW
#define CreateDirectoryWrap         CreateDirectoryW
#define CreateEventWrap             CreateEventW
#define CreateFontWrap              CreateFontW
#define CreateFileWrap              CreateFileW
#define CreateFontIndirectWrap      CreateFontIndirectW
#define CreateICWrap                CreateICW
#define CreateMetaFileWrap          CreateMetaFileW
#define CreateMutexWrap             CreateMutexW
#define CreateSemaphoreWrap         CreateSemaphoreW
#define CreateWindowExWrap          CreateWindowExW
#define DefWindowProcWrap           DefWindowProcW
#define DeleteFileWrap              DeleteFileW
#define DispatchMessageWrap         DispatchMessageW
#define DrawTextExWrap              DrawTextExW
#define DrawTextWrap                DrawTextW
#define EnumFontFamiliesWrap        EnumFontFamiliesW
#define EnumFontFamiliesExWrap      EnumFontFamiliesExW
#define EnumResourceNamesWrap       EnumResourceNamesW
#define ExpandEnvironmentStringsWrap ExpandEnvironmentStringsW
#define ExtractIconExWrap           ExtractIconExW
#define ExtTextOutWrap              ExtTextOutW
#define FindFirstFileWrap           FindFirstFileW
#define FindNextFileWrap            FindNextFileW
#define FindResourceWrap            FindResourceW
#define FindWindowWrap              FindWindowW
#define FindWindowExWrap            FindWindowExW
#define FormatMessageWrap           FormatMessageW
#define GetClassInfoWrap            GetClassInfoW
#define GetClassInfoExWrap          GetClassInfoExW
#define GetClassLongWrap            GetClassLongW
#define GetClassNameWrap            GetClassNameW
#define GetClipboardFormatNameWrap  GetClipboardFormatNameW
#define GetCurrentDirectoryWrap     GetCurrentDirectoryW
#define GetDlgItemTextWrap          GetDlgItemTextW
#define GetFileAttributesWrap       GetFileAttributesW
#define GetFullPathNameWrap         GetFullPathNameW
#define GetLocaleInfoWrap           GetLocaleInfoW
#define GetMenuItemInfoWrap         GetMenuItemInfoWrapW
#define GetMenuStringWrap           GetMenuStringW
#define GetMessageWrap              GetMessageW
#define GetModuleFileNameWrap       GetModuleFileNameW
#define GetNumberFormatWrap         GetNumberFormatW
#define GetSystemDirectoryWrap      GetSystemDirectoryW
#define GetModuleHandleWrap         GetModuleHandleW
#define GetObjectWrap               GetObjectW
#define GetPrivateProfileIntWrap    GetPrivateProfileIntW
#define GetProfileStringWrap        GetProfileStringW
#define GetPrivateProfileStringWrap GetPrivateProfileStringW
#define WritePrivateProfileStringWrap WritePrivateProfileStringW
#define GetPropWrap                 GetPropW
#define GetStringTypeExWrap         GetStringTypeExW
#define GetTempFileNameWrap         GetTempFileNameW
#define GetTempPathWrap             GetTempPathW
#define GetTextExtentPoint32Wrap    GetTextExtentPoint32W
#define GetTextFaceWrap             GetTextFaceW
#define GetTextMetricsWrap          GetTextMetricsW
#define GetTimeFormatWrap           GetTimeFormatW
#define GetDateFormatWrap           GetDateFormatW
#define GetUserNameWrap             GetUserNameW
#define GetWindowLongWrap           GetWindowLongW
#define GetWindowTextWrap           GetWindowTextW
#define GetWindowTextLengthWrap     GetWindowTextLengthW
#define GetWindowsDirectoryWrap     GetWindowsDirectoryW
#define InsertMenuItemWrap          InsertMenuItemW
#define InsertMenuWrap              InsertMenuW
#define IsBadStringPtrWrap          IsBadStringPtrW
#define IsDialogMessageWrap         IsDialogMessageW
#define LoadAcceleratorsWrap        LoadAcceleratorsW
#define LoadBitmapWrap              LoadBitmapW
#define LoadCursorWrap              LoadCursorW
#define LoadIconWrap                LoadIconW
#define LoadImageWrap               LoadImageW
#define LoadLibraryWrap             LoadLibraryW
#define LoadLibraryExWrap           LoadLibraryExW
#define LoadMenuWrap                LoadMenuW
#define LoadStringWrap              LoadStringW
#define MessageBoxIndirectWrap      MessageBoxIndirectW
#define MessageBoxWrap              MessageBoxW
#define ModifyMenuWrap              ModifyMenuW
#define GetCharWidth32Wrap          GetCharWidth32W
#define GetCharacterPlacementWrap   GetCharacterPlacementW
#define CopyFileWrap                CopyFileW
#define MoveFileWrap                MoveFileW
#define OemToCharWrap               OemToCharW
#define OutputDebugStringWrap       OutputDebugStringW
#define PeekMessageWrap             PeekMessageW
#define PostMessageWrap             PostMessageW
#define PostThreadMessageWrap       PostThreadMessageW
#define RegCreateKeyWrap            RegCreateKeyW
#define RegCreateKeyExWrap          RegCreateKeyExW
#define RegDeleteKeyWrap            RegDeleteKeyW
#define RegDeleteValueWrap          RegDeleteValueW
#define RegEnumKeyWrap              RegEnumKeyW
#define RegEnumKeyExWrap            RegEnumKeyExW
#define RegOpenKeyWrap              RegOpenKeyW
#define RegOpenKeyExWrap            RegOpenKeyExW
#define RegQueryInfoKeyWrap         RegQueryInfoKeyW
#define RegQueryValueWrap           RegQueryValueW
#define RegQueryValueExWrap         RegQueryValueExW
#define RegSetValueWrap             RegSetValueW
#define RegSetValueExWrap           RegSetValueExW
#define RegisterClassWrap           RegisterClassW
#define RegisterClassExWrap         RegisterClassExW
#define RegisterClipboardFormatWrap RegisterClipboardFormatW
#define RegisterWindowMessageWrap   RegisterWindowMessageW
#define RemovePropWrap              RemovePropW
#define SearchPathWrap              SearchPathW
#define SendDlgItemMessageWrap      SendDlgItemMessageW
#define SendMessageWrap             SendMessageW
#define SetCurrentDirectoryWrap     SetCurrentDirectoryW
#define SetDlgItemTextWrap          SetDlgItemTextW
#define SetMenuItemInfoWrap         SetMenuItemInfoW
#define SetPropWrap                 SetPropW
#define SetFileAttributesWrap       SetFileAttributesW
#define SetWindowLongWrap           SetWindowLongW
#define SetWindowsHookExWrap        SetWindowsHookExW
#define SHBrowseForFolderWrap       SHBrowseForFolderW
#define ShellExecuteExWrap          ShellExecuteExW
#define SHFileOperationWrap         SHFileOperationW
#define SHGetFileInfoWrap           SHGetFileInfoW
#define SHGetPathFromIDListWrap     SHGetPathFromIDListW
#define StartDocWrap                StartDocW
#define SystemParametersInfoWrap    SystemParametersInfoW
#define TranslateAcceleratorWrap    TranslateAcceleratorW
#define UnregisterClassWrap         UnregisterClassW
#define VkKeyScanWrap               VkKeyScanW
#define WinHelpWrap                 WinHelpW
#define WNetRestoreConnectionWrap   WNetRestoreConnectionW
#define WNetGetLastErrorWrap        WNetGetLastErrorW
#define wvsprintfWrap               wvsprintfW
#define CreateFontWrap              CreateFontW
#define DrawTextExWrap              DrawTextExW
#define GetMenuItemInfoWrap         GetMenuItemInfoWrapW
#define SetMenuItemInfoWrap         SetMenuItemInfoW
#define InsertMenuItemWrap          InsertMenuItemW
#define DragQueryFileWrap           DragQueryFileW

#else

#define IsCharAlphaWrap             IsCharAlphaA
#define IsCharUpperWrap             IsCharUpperA
#define IsCharLowerWrap             IsCharLowerA
#define IsCharAlphaNumericWrap      IsCharAlphaNumericA
#define AppendMenuWrap              AppendMenuA
#ifdef POST_IE5_BETA
#define CallMsgFilterWrap           CallMsgFilterA
#endif
#define CallWindowProcWrap          CallWindowProcA
#define CharLowerWrap               CharLowerA
#define CharLowerBuffWrap           CharLowerBuffA
#define CharNextWrap                CharNextA
#define CharPrevWrap                CharPrevA
#define CharToOemWrap               CharToOemA
#define CharUpperWrap               CharUpperA
#define CharUpperBuffWrap           CharUpperBuffA
#define CompareStringWrap           CompareStringA
#define CopyAcceleratorTableWrap    CopyAcceleratorTableA
#define CreateAcceleratorTableWrap  CreateAcceleratorTableA
#define CreateDCWrap                CreateDCA
#define CreateDirectoryWrap         CreateDirectoryA
#define CreateEventWrap             CreateEventA
#define CreateFontWrap              CreateFontA
#define CreateFileWrap              CreateFileA
#define CreateFontIndirectWrap      CreateFontIndirectA
#define CreateICWrap                CreateICA
#define CreateMetaFileWrap          CreateMetaFileA
#define CreateMutexWrap             CreateMutexA
#define CreateSemaphoreWrap         CreateSemaphoreA
#define CreateWindowExWrap          CreateWindowExA
#define DefWindowProcWrap           DefWindowProcA
#define DeleteFileWrap              DeleteFileA
#define DispatchMessageWrap         DispatchMessageA
#define DrawTextExWrap              DrawTextExA
#define DrawTextWrap                DrawTextA
#define EnumFontFamiliesWrap        EnumFontFamiliesA
#define EnumFontFamiliesExWrap      EnumFontFamiliesExA
#define EnumResourceNamesWrap       EnumResourceNamesA
#define ExpandEnvironmentStringsWrap ExpandEnvironmentStringsA
#define ExtractIconExWrap           ExtractIconExA
#define ExtTextOutWrap              ExtTextOutA
#define FindFirstFileWrap           FindFirstFileA
#define FindResourceWrap            FindResourceA
#define FindNextFileWrap            FindNextFileA
#define FindWindowWrap              FindWindowA
#define FindWindowExWrap            FindWindowExA
#define FormatMessageWrap           FormatMessageA
#define GetClassInfoWrap            GetClassInfoA
#define GetClassInfoExWrap          GetClassInfoExA
#define GetClassLongWrap            GetClassLongA
#define GetClassNameWrap            GetClassNameA
#define GetClipboardFormatNameWrap  GetClipboardFormatNameA
#define GetCurrentDirectoryWrap     GetCurrentDirectoryA
#define GetDlgItemTextWrap          GetDlgItemTextA
#define GetFileAttributesWrap       GetFileAttributesA
#define GetFullPathNameWrap         GetFullPathNameA
#define GetLocaleInfoWrap           GetLocaleInfoA
#define GetMenuItemInfoWrap         GetMenuItemInfoA
#define GetMenuStringWrap           GetMenuStringA
#define GetMessageWrap              GetMessageA
#define GetModuleFileNameWrap       GetModuleFileNameA
#define GetNumberFormatWrap         GetNumberFormatA
#define GetPrivateProfileStringWrap GetPrivateProfileStringA
#define WritePrivateProfileStringWrap WritePrivateProfileStringA
#define GetSystemDirectoryWrap      GetSystemDirectoryA
#define SearchPathWrap              SearchPathA
#define GetModuleHandleWrap         GetModuleHandleA
#define GetObjectWrap               GetObjectA
#define GetPrivateProfileIntWrap    GetPrivateProfileIntA
#define GetProfileStringWrap        GetProfileStringA
#define GetPropWrap                 GetPropA
#define GetStringTypeExWrap         GetStringTypeExA
#define GetTempFileNameWrap         GetTempFileNameA
#define GetTempPathWrap             GetTempPathA
#define GetTextExtentPoint32Wrap    GetTextExtentPoint32A
#define GetTextFaceWrap             GetTextFaceA
#define GetTextMetricsWrap          GetTextMetricsA
#define GetTimeFormatWrap           GetTimeFormatA
#define GetDateFormatWrap           GetDateFormatA
#define GetUserNameWrap             GetUserNameA
#define GetWindowLongWrap           GetWindowLongA
#define GetWindowTextWrap           GetWindowTextA
#define GetWindowTextLengthWrap     GetWindowTextLengthA
#define GetWindowsDirectoryWrap     GetWindowsDirectoryA
#define InsertMenuItemWrap          InsertMenuItemA
#define InsertMenuWrap              InsertMenuA
#define IsBadStringPtrWrap          IsBadStringPtrA
#define IsDialogMessageWrap         IsDialogMessageA
#define LoadAcceleratorsWrap        LoadAcceleratorsA
#define LoadBitmapWrap              LoadBitmapA
#define LoadCursorWrap              LoadCursorA
#define LoadIconWrap                LoadIconA
#define LoadImageWrap               LoadImageWrapA
#define LoadLibraryWrap             LoadLibraryA
#define LoadLibraryExWrap           LoadLibraryExA
#define LoadMenuWrap                LoadMenuA
#define LoadStringWrap              LoadStringA
#define MessageBoxIndirectWrap      MessageBoxIndirectA
#define MessageBoxWrap              MessageBoxA
#define ModifyMenuWrap              ModifyMenuA
#define GetCharWidth32Wrap          GetCharWidth32A
#define GetCharacterPlacementWrap   GetCharacterPlacementA
#define CopyFileWrap                CopyFileA
#define MoveFileWrap                MoveFileA
#define OemToCharWrap               OemToCharA
#define OutputDebugStringWrap       OutputDebugStringA
#define PeekMessageWrap             PeekMessageA
#define PostMessageWrap             PostMessageA
#define PostThreadMessageWrap       PostThreadMessageA
#define RegCreateKeyWrap            RegCreateKeyA
#define RegCreateKeyExWrap          RegCreateKeyExA
#define RegDeleteKeyWrap            RegDeleteKeyA
#define RegDeleteValueWrap          RegDeleteValueA
#define RegEnumKeyWrap              RegEnumKeyA
#define RegEnumKeyExWrap            RegEnumKeyExA
#define RegOpenKeyWrap              RegOpenKeyA
#define RegOpenKeyExWrap            RegOpenKeyExA
#define RegQueryInfoKeyWrap         RegQueryInfoKeyA
#define RegQueryValueWrap           RegQueryValueA
#define RegQueryValueExWrap         RegQueryValueExA
#define RegSetValueWrap             RegSetValueA
#define RegSetValueExWrap           RegSetValueExA
#define RegisterClassWrap           RegisterClassA
#define RegisterClassExWrap         RegisterClassExA
#define RegisterClipboardFormatWrap RegisterClipboardFormatA
#define RegisterWindowMessageWrap   RegisterWindowMessageA
#define RemovePropWrap              RemovePropA
#define SendDlgItemMessageWrap      SendDlgItemMessageA
#define SendMessageWrap             SendMessageA
#define SetCurrentDirectoryWrap     SetCurrentDirectoryA
#define SetDlgItemTextWrap          SetDlgItemTextA
#define SetMenuItemInfoWrap         SetMenuItemInfoA
#define SetPropWrap                 SetPropA
#define SetWindowLongWrap           SetWindowLongA
#define SHBrowseForFolderWrap       SHBrowseForFolderA
#define ShellExecuteExWrap          ShellExecuteExA
#define SHFileOperationWrap         SHFileOperationA
#define SHGetFileInfoWrap           SHGetFileInfoA
#define SHGetPathFromIDListWrap     SHGetPathFromIDListA
#define SetFileAttributesWrap       SetFileAttributesA
#define SetWindowsHookExWrap        SetWindowsHookExA
#define StartDocWrap                StartDocA
#define SystemParametersInfoWrap    SystemParametersInfoA
#define TranslateAcceleratorWrap    TranslateAcceleratorA
#define UnregisterClassWrap         UnregisterClassA
#define VkKeyScanWrap               VkKeyScanA
#define WinHelpWrap                 WinHelpA
#define WNetRestoreConnectionWrap   WNetRestoreConnectionA
#define WNetGetLastErrorWrap        WNetGetLastErrorA
#define wvsprintfWrap               wvsprintfA
#define CreateFontWrap              CreateFontA
#define DrawTextExWrap              DrawTextExA
#define GetMenuItemInfoWrap         GetMenuItemInfoA
#define SetMenuItemInfoWrap         SetMenuItemInfoA
#define InsertMenuItemWrap          InsertMenuItemA
#define DragQueryFileWrap           DragQueryFileA
#endif
#endif // defined(UNIX) && defined(NO_SHLWAPI_UNITHUNK)

// Some functions are used to wrap unicode win95 functions AND to provide ML wrappers,
// so they are needed unless BOTH NO_SHLWAPI_UNITHUNG and NO_SHLWAPI_MLUI are defined
//
#if (_WIN32_IE >= 0x0500) && (!defined(NO_SHLWAPI_UNITHUNK) || !defined(NO_SHLWAPI_MLUI))

LWSTDAPI_(HWND)
CreateDialogIndirectParamWrapW(
        HINSTANCE       hInstance,
        LPCDLGTEMPLATEW hDialogTemplate,
        HWND            hWndParent,
        DLGPROC         lpDialogFunc,
        LPARAM          dwInitParam);

LWSTDAPI_(HWND)
CreateDialogParamWrapW(
        HINSTANCE   hInstance,
        LPCWSTR     lpTemplateName,
        HWND        hWndParent,
        DLGPROC     lpDialogFunc,
        LPARAM      dwInitParam);

LWSTDAPI_(INT_PTR)
DialogBoxIndirectParamWrapW(
        HINSTANCE       hInstance,
        LPCDLGTEMPLATEW hDialogTemplate,
        HWND            hWndParent,
        DLGPROC         lpDialogFunc,
        LPARAM          dwInitParam);

LWSTDAPI_(INT_PTR)
DialogBoxParamWrapW(
        HINSTANCE   hInstance,
        LPCWSTR     lpszTemplate,
        HWND        hWndParent,
        DLGPROC     lpDialogFunc,
        LPARAM      dwInitParam);

LWSTDAPI_(BOOL) SetWindowTextWrapW(HWND hWnd, LPCWSTR lpString);

int _cdecl ShellMessageBoxWrapW(HINSTANCE hInst, HWND hWnd, LPCWSTR pszMsg, LPCWSTR pszTitle, UINT fuStyle, ...);

LWSTDAPI_(BOOL) DeleteMenuWrap(HMENU hMenu, UINT uPosition, UINT uFlags);

LWSTDAPI_(BOOL) DestroyMenuWrap(HMENU hMenu);

#ifdef UNICODE

#define CreateDialogIndirectParamWrap CreateDialogIndirectParamWrapW
#define CreateDialogParamWrap       CreateDialogParamWrapW
#define DialogBoxIndirectParamWrap  DialogBoxIndirectParamWrapW
#define DialogBoxParamWrap          DialogBoxParamWrapW
#define SetWindowTextWrap           SetWindowTextWrapW
#define ShellMessageBoxWrap         ShellMessageBoxWrapW

#else

#define CreateDialogIndirectParamWrap CreateDialogIndirectParamA
#define CreateDialogParamWrap       CreateDialogParamA
#define DialogBoxIndirectParamWrap  DialogBoxIndirectParamA
#define DialogBoxParamWrap          DialogBoxParamA
#define SetWindowTextWrap           SetWindowTextA
#define ShellMessageBoxWrap         ShellMessageBoxA

#endif // UNICODE

#endif // (_WIN32_IE >= 0x0500) && !defined(NO_SHLWAPI_UNITHUNK) && !defined (NO_SHLWAPI_MLUI)


//=============== Thread Pool Services ===================================

#if (_WIN32_IE >= 0x0500) && !defined(NO_SHLWAPI_TPS)

//
// SHLWAPIP versions of KERNEL32 Thread Pool Services APIs
//

typedef void (NTAPI * WAITORTIMERCALLBACKFUNC)(void *, BOOLEAN);
typedef void (NTAPI * WORKERCALLBACKFUNC)(void *); // BUGBUG - why declare this?
typedef WAITORTIMERCALLBACKFUNC WAITORTIMERCALLBACK;

LWSTDAPI_(HANDLE)
SHRegisterWaitForSingleObject(
    IN HANDLE hObject,
    IN WAITORTIMERCALLBACKFUNC pfnCallback,
    IN LPVOID pContext,
    IN DWORD dwMilliseconds,
    IN LPCSTR lpszLibrary OPTIONAL,
    IN DWORD dwFlags
    );

//
// flags for SHRegisterWaitForSingleObject (keep separate from other TPS flags)
//

//
// SRWSO_NOREMOVE - if set, the handle is not to be removed from the list once
// signalled. Intended for use with auto-reset events that the caller wants to
// keep until unregistered
//

#define SRWSO_NOREMOVE      0x00000100

#define SRWSO_VALID_FLAGS   (SRWSO_NOREMOVE)

#define SRWSO_INVALID_FLAGS (~SRWSO_VALID_FLAGS)

LWSTDAPI_(BOOL)
SHUnregisterWait(
    IN HANDLE hWait
    );

typedef struct {
    DWORD dwStructSize;
    DWORD dwMinimumWorkerThreads;
    DWORD dwMaximumWorkerThreads;
    DWORD dwMaximumWorkerQueueDepth;
    DWORD dwWorkerThreadIdleTimeout;
    DWORD dwWorkerThreadCreationDelta;
    DWORD dwMinimumIoWorkerThreads;
    DWORD dwMaximumIoWorkerThreads;
    DWORD dwMaximumIoWorkerQueueDepth;
    DWORD dwIoWorkerThreadCreationDelta;
} SH_THREAD_POOL_LIMITS, *PSH_THREAD_POOL_LIMITS;

LWSTDAPI_(BOOL)
SHSetThreadPoolLimits(
    IN PSH_THREAD_POOL_LIMITS pLimits
    );

LWSTDAPI_(BOOL)
SHTerminateThreadPool(
    VOID
    );

LWSTDAPI_(BOOL)
SHQueueUserWorkItem(
    IN LPTHREAD_START_ROUTINE pfnCallback,
    IN LPVOID pContext,
    IN LONG lPriority,
    IN DWORD_PTR dwTag,
    OUT DWORD_PTR * pdwId OPTIONAL,
    IN LPCSTR pszModule OPTIONAL,
    IN DWORD dwFlags
    );

LWSTDAPI_(DWORD)
SHCancelUserWorkItems(
    IN DWORD_PTR dwTagOrId,
    IN BOOL bTag
    );

LWSTDAPI_(HANDLE)
SHCreateTimerQueue(
    VOID
    );

LWSTDAPI_(BOOL)
SHDeleteTimerQueue(
    IN HANDLE hQueue
    );

LWSTDAPI_(HANDLE)
SHSetTimerQueueTimer(
    IN HANDLE hQueue,
    IN WAITORTIMERCALLBACK pfnCallback,
    IN LPVOID pContext,
    IN DWORD dwDueTime,
    IN DWORD dwPeriod,
    IN LPCSTR lpszLibrary OPTIONAL,
    IN DWORD dwFlags
    );

LWSTDAPI_(BOOL)
SHChangeTimerQueueTimer(
    IN HANDLE hQueue,
    IN HANDLE hTimer,
    IN DWORD dwDueTime,
    IN DWORD dwPeriod
    );

LWSTDAPI_(BOOL)
SHCancelTimerQueueTimer(
    IN HANDLE hQueue,
    IN HANDLE hTimer
    );

//
// Thread Pool Services flags
//

//
// TPS_EXECUTEIO - execute in I/O thread (via APC). Default is non-IO thread
//

#define TPS_EXECUTEIO       0x00000001

//
// TPS_TAGGEDITEM - the dwTag parameter is meaningful
//

#define TPS_TAGGEDITEM      0x00000002

//
// TPS_DEMANDTHREAD - always create a new thread if none currently available.
// Used in situations where immediate response required
//

#define TPS_DEMANDTHREAD    0x00000004

//
// TPS_LONGEXECTIME - the work item will take relatively long time to execute.
// Used as management hint to TPS
//

#define TPS_LONGEXECTIME    0x00000008

//
// TPS_RESERVED_FLAGS - mask of bits reserved for internal use
//

#define TPS_RESERVED_FLAGS  0xFF000000

#define TPS_VALID_FLAGS     (TPS_EXECUTEIO      \
                            | TPS_TAGGEDITEM    \
                            | TPS_DEMANDTHREAD  \
                            | TPS_LONGEXECTIME  \
                            )
#define TPS_INVALID_FLAGS   (~TPS_VALID_FLAGS)

#endif // (_WIN32_IE >= 0x0500) && !defined(NO_SHLWAPI_TPS)


//
// Private MIME helper functions used by shdocvw & shell32
//
#if (_WIN32_IE >= 0x0500)

STDAPI_(BOOL) GetMIMETypeSubKeyA(LPCSTR pszMIMEType, LPSTR pszBuf, UINT cchBuf);
STDAPI_(BOOL) GetMIMETypeSubKeyW(LPCWSTR pszMIMEType, LPWSTR pszBuf, UINT cchBuf);

STDAPI_(BOOL) RegisterMIMETypeForExtensionA(LPCSTR pcszExtension, LPCSTR pcszMIMEContentType);
STDAPI_(BOOL) RegisterMIMETypeForExtensionW(LPCWSTR pcszExtension, LPCWSTR pcszMIMEContentType);

STDAPI_(BOOL) UnregisterMIMETypeForExtensionA(LPCSTR pcszExtension);
STDAPI_(BOOL) UnregisterMIMETypeForExtensionW(LPCWSTR pcszExtension);

STDAPI_(BOOL) RegisterExtensionForMIMETypeA(LPCSTR pcszExtension, LPCSTR pcszMIMEContentType);
STDAPI_(BOOL) RegisterExtensionForMIMETypeW(LPCWSTR pcszExtension, LPCWSTR pcszMIMEContentType);

STDAPI_(BOOL) UnregisterExtensionForMIMETypeA(LPCSTR pcszMIMEContentType);
STDAPI_(BOOL) UnregisterExtensionForMIMETypeW(LPCWSTR pcszMIMEContentType);

STDAPI_(BOOL) MIME_GetExtensionA(LPCSTR pcszMIMEType, LPSTR pszExtensionBuf, UINT ucExtensionBufLen);
STDAPI_(BOOL) MIME_GetExtensionW(LPCWSTR pcszMIMEType, LPWSTR pszExtensionBuf, UINT ucExtensionBufLen);

#ifdef UNICODE
#define GetMIMETypeSubKey               GetMIMETypeSubKeyW
#define RegisterMIMETypeForExtension    RegisterMIMETypeForExtensionW
#define UnregisterMIMETypeForExtension  UnregisterMIMETypeForExtensionW
#define RegisterExtensionForMIMEType    RegisterExtensionForMIMETypeW
#define UnregisterExtensionForMIMEType  UnregisterExtensionForMIMETypeW
#define MIME_GetExtension               MIME_GetExtensionW
#else
#define GetMIMETypeSubKey               GetMIMETypeSubKeyA
#define RegisterMIMETypeForExtension    RegisterMIMETypeForExtensionA
#define UnregisterMIMETypeForExtension  UnregisterMIMETypeForExtensionA
#define RegisterExtensionForMIMEType    RegisterExtensionForMIMETypeA
#define UnregisterExtensionForMIMEType  UnregisterExtensionForMIMETypeA
#define MIME_GetExtension               MIME_GetExtensionA
#endif

// Options for SHGetMachineInfo

//
//  Note that GMI_DOCKSTATE is unreliable for ACPI laptops.
//
#define GMI_DOCKSTATE           0x0000
    // Return values for SHGetMachineInfo(GMI_DOCKSTATE)
    #define GMID_NOTDOCKABLE         0  // Cannot be docked
    #define GMID_UNDOCKED            1  // Is undocked
    #define GMID_DOCKED              2  // Is docked

//
//  GMI_BATTERYSTATE reports on the presence and status of non-UPS
//  batteries.
//
#define GMI_BATTERYSTATE        0x0001
    // Return value for SHGetMachineInfo(GMI_BATTERYSTATE) is a bitmask
    #define GMIB_HASBATTERY          0x0001 // Can run on batteries
    #define GMIB_ONBATTERY           0x0002 // Is now on batteries

//
//  WARNING!  DANGER!  EVIL!
//
//  GMI_LAPTOP is not perfect.  It can be fooled by particular hardware
//  configurations.  You are much better off asking specifically why you
//  care about laptops and use one of the above GMI values instead.  For
//  example, if you want to scale back some intensive operation so you
//  don't drain the battery, use GMI_BATTERYSTATE instead.
//
#define GMI_LAPTOP              0x0002  // Returns nonzero if might be a laptop

#if (_WIN32_IE >= 0x0501)

//
//  GMI_TSCLIENT tells you whether you are running as a Terminal Server
//  client and should disable your animations.
//
#define GMI_TSCLIENT            0x0003  // Returns nonzero if TS client

#endif // (_WIN32_IE >= 0x0501)

STDAPI_(DWORD_PTR) SHGetMachineInfo(UINT gmi);

// support InterlockedCompareExchange() on Win95

STDAPI_(void *) SHInterlockedCompareExchange(void **ppDest, void *pExch, void *pComp);

#if !defined(_X86_)
// Win95 doesn't run on Alpha/UNIX so we can use the OS function directly
// Use a #define instead of a forwarder because it's an intrinsic on most
// compilers.
#define SHInterlockedCompareExchange InterlockedCompareExchangePointer
#endif

LWSTDAPI_(BOOL) SHMirrorIcon(HICON* phiconSmall, HICON* phiconLarge);


#endif // (_WIN32_IE >= 0x0500)


//  Raw Accelerator Table API
//
//  Allows an accelerator table grep without having to invoke ::TranslateAccelerator.
//  Useful for dealing with parent-child window accelerator conflicts.
//

//  HANDLE SHLoadRawAccelerators( HINSTANCE hInst, LPCTSTR lpTableName );
//  Loads the raw accelerator table.
//  hInst       Module instance containing the accelerator resource.
//  lpTableName Names the accelerator table resource to load.

//  The return value is a handle that can be passed to a SHQueryRawAcceleratorXXX function.
//  When the handle is no longer required, it should be freed with LocalFree().
STDAPI_(HANDLE) SHLoadRawAccelerators   ( HINSTANCE hInst, LPCTSTR lpTableName );

//  BOOL SHQueryRawAccelerator   ( HANDLE hcaAcc, IN BYTE fVirtMask, IN BYTE fVirt, IN WPARAM wKey, OUT OPTIONAL UINT* puCmdID );
//  Queries the raw accelererator table for the specified key
//  hcaAcc      Handle returned from SHLoadRawAccelerators().
//  fVirtMask   Relevant accelerator flags (any combo of FALT|FCONTROL|FNOINVERT|FSHIFT|FVIRTKEY)
//  fVirt       Accelerator flags to test (any combo of FALT|FCONTROL|FNOINVERT|FSHIFT|FVIRTKEY).
//  wKey        Accelerator key.  This can either be a virtual key (FVIRTKEY) or an ASCII char code.
//  puCmdID     Optional address to receive command identifier for the accelerator entry if
//              the key is in the table.
//  Returns nonzero if the key is in the accelerator table; otherwise 0.
STDAPI_(BOOL)   SHQueryRawAccelerator   ( HANDLE hcaAcc, IN BYTE fVirtMask, IN BYTE fVirt, IN WPARAM wKey, OUT OPTIONAL UINT* puCmdID );

//  BOOL SHQueryRawAcceleratorMsg( HANDLE hcaAcc, MSG* pmsg, OUT OPTIONAL UINT* puCmdID );
//  Determines whether the specified message is an accelerator message mapping to
//  an entry in the raw accelerator table.
//  hcaAcc      Handle returned from SHLoadRawAccelerators().
//  pmsg        Address of the message to test.
//  puCmdID     Optional address to receive command identifier for the accelerator entry if
//              the message maps to an accelerator in the table.
//  Returns nonzero if the message is a WM_KEYUP or WM_KEYDOWN and the key is in
//  the accelerator table; otherwise 0.
STDAPI_(BOOL)   SHQueryRawAcceleratorMsg( HANDLE hcaAcc, MSG* pmsg, OUT OPTIONAL UINT* puCmdID );
//
//


//
//====== End Internal functions  ===============================================
//
#endif // NO_SHLWAPI_INTERNAL

;end_internal

#if (_WIN32_IE >= 0x0500)

// SHAutoComplete
//      hwndEdit - HWND of editbox, ComboBox or ComboBoxEx.
//      dwFlags - Flags to indicate what to AutoAppend or AutoSuggest for the editbox.
//
// WARNING:
//    Caller needs to have called CoInitialize() or OleInitialize()
//    and cannot call CoUninit/OleUninit until after
//    WM_DESTROY on hwndEdit.
//
//  dwFlags values:
#define SHACF_DEFAULT                   0x00000000  // Currently (SHACF_FILESYSTEM | SHACF_URLALL)
#define SHACF_FILESYSTEM                0x00000001  // This includes the File System as well as the rest of the shell (Desktop\My Computer\Control Panel\)
#define SHACF_URLALL                    (SHACF_URLHISTORY | SHACF_URLMRU)
#define SHACF_URLHISTORY                0x00000002  // URLs in the User's History
#define SHACF_URLMRU                    0x00000004  // URLs in the User's Recently Used list.
#define SHACF_USETAB                    0x00000008  // URLs in the User's Recently Used list.
#define SHACF_FILESYS_ONLY              0x00000010  // Don't AutoComplete non-File System items.

#define SHACF_AUTOSUGGEST_FORCE_ON      0x10000000  // Ignore the registry default and force the feature on.
#define SHACF_AUTOSUGGEST_FORCE_OFF     0x20000000  // Ignore the registry default and force the feature off.
#define SHACF_AUTOAPPEND_FORCE_ON       0x40000000  // Ignore the registry default and force the feature on. (Also know as AutoComplete)
#define SHACF_AUTOAPPEND_FORCE_OFF      0x80000000  // Ignore the registry default and force the feature off. (Also know as AutoComplete)

LWSTDAPI SHAutoComplete(HWND hwndEdit, DWORD dwFlags);

STDAPI SHSetThreadRef(IUnknown *punk);
STDAPI SHGetThreadRef(IUnknown **ppunk);

LWSTDAPI_(BOOL) SHSkipJunction(struct IBindCtx* pbc, const CLSID *pclsid);
#endif

#define CTF_INSIST          0x00000001      // SHCreateThread() dwFlags - call pfnThreadProc synchronously if CreateThread() fails
#define CTF_THREAD_REF      0x00000002      // hold a reference to the creating thread
#define CTF_PROCESS_REF     0x00000004      // hold a reference to the creating process
#define CTF_COINIT          0x00000008      // init COM for the created thread

LWSTDAPI_(BOOL) SHCreateThread(LPTHREAD_START_ROUTINE pfnThreadProc, void *pData, DWORD dwFlags, LPTHREAD_START_ROUTINE pfnCallback);


#ifndef NO_SHLWAPI_GDI
//
//====== GDI helper functions  ================================================
//

LWSTDAPI_(HPALETTE) SHCreateShellPalette(HDC hdc);

#if (_WIN32_IE >= 0x0500)

LWSTDAPI_(void)     ColorRGBToHLS(COLORREF clrRGB, WORD* pwHue, WORD* pwLuminance, WORD* pwSaturation);
LWSTDAPI_(COLORREF) ColorHLSToRGB(WORD wHue, WORD wLuminance, WORD wSaturation);
LWSTDAPI_(COLORREF) ColorAdjustLuma(COLORREF clrRGB, int n, BOOL fScale);

;begin_internal
#ifdef NOTYET       // BUGBUG (scotth): once this is implemented, make this public
// SHGetCommonResourceID
//
// (use MAKEINTRESOURCE on the following IDs)

// These values are indexes into an internal table.  Be careful.    ;internal
#define SHGCR_BITMAP_WINDOWS_LOGO   MAKEINTRESOURCE(1)
#define SHGCR_AVI_FLASHLIGHT        MAKEINTRESOURCE(2)
#define SHGCR_AVI_FINDFILE          MAKEINTRESOURCE(3)
#define SHGCR_AVI_FINDCOMPUTER      MAKEINTRESOURCE(4)
#define SHGCR_AVI_FILEMOVE          MAKEINTRESOURCE(5)
#define SHGCR_AVI_FILECOPY          MAKEINTRESOURCE(6)
#define SHGCR_AVI_FILEDELETE        MAKEINTRESOURCE(7)
#define SHGCR_AVI_EMPTYWASTEBASKET  MAKEINTRESOURCE(8)
#define SHGCR_AVI_FILEREALDELETE    MAKEINTRESOURCE(9)      // Bypass Recycle Bin
#define SHGCR_AVI_DOWNLOAD          MAKEINTRESOURCE(10)

LWSTDAPI SHGetCommonResourceIDA(IN LPCSTR pszID, IN DWORD dwRes, OUT HMODULE * phmod, OUT UINT * pnID);
LWSTDAPI SHGetCommonResourceIDA(IN LPCSTR pszID, IN DWORD dwRes, OUT HMODULE * phmod, OUT UINT * pnID);

#ifdef UNICODE
#define SHGetCommonResourceID   SHGetCommonResourceIDW
#else
#define SHGetCommonResourceID   SHGetCommonResourceIDW
#endif
#endif // NOTYET
;end_internal

#endif  // _WIN32_IE >= 0x0500

#endif // NO_SHLWAPI_GDI


//
//====== DllGetVersion  =======================================================
//

typedef struct _DLLVERSIONINFO
{
    DWORD cbSize;
    DWORD dwMajorVersion;                   // Major version
    DWORD dwMinorVersion;                   // Minor version
    DWORD dwBuildNumber;                    // Build number
    DWORD dwPlatformID;                     // DLLVER_PLATFORM_*
} DLLVERSIONINFO;

// Platform IDs for DLLVERSIONINFO
#define DLLVER_PLATFORM_WINDOWS         0x00000001      // Windows 95
#define DLLVER_PLATFORM_NT              0x00000002      // Windows NT

#if (_WIN32_IE >= 0x0501)

typedef struct _DLLVERSIONINFO2
{
    DLLVERSIONINFO info1;
    // dwFlags is really for alignment purposes ;Internal
    DWORD dwFlags;                          // No flags currently defined
    ULONGLONG ullVersion;                   // Encoded as:
                                            // Major 0xFFFF 0000 0000 0000
                                            // Minor 0x0000 FFFF 0000 0000
                                            // Build 0x0000 0000 FFFF 0000
                                            // QFE   0x0000 0000 0000 FFFF
} DLLVERSIONINFO2;

#define DLLVER_MAJOR_MASK                    0xFFFF000000000000
#define DLLVER_MINOR_MASK                    0x0000FFFF00000000
#define DLLVER_BUILD_MASK                    0x00000000FFFF0000
#define DLLVER_QFE_MASK                      0x000000000000FFFF

#endif

#define MAKEDLLVERULL(major, minor, build, qfe) \
        (((ULONGLONG)(major) << 48) |        \
         ((ULONGLONG)(minor) << 32) |        \
         ((ULONGLONG)(build) << 16) |        \
         ((ULONGLONG)(  qfe) <<  0))

//
// The caller should always GetProcAddress("DllGetVersion"), not
// implicitly link to it.
//

typedef HRESULT (CALLBACK* DLLGETVERSIONPROC)(DLLVERSIONINFO *);

// DllInstall (to be implemented by self-installing DLLs)
STDAPI DllInstall(BOOL bInstall, LPCWSTR pszCmdLine);

;begin_internal
#if (_WIN32_IE >= 0x0501)
//
// ======== SHGetAppCompatFlags ================================================
//

//===========================================================================
// Shell Application Compatability flags

// SHGetAppCompatFlags flags
#define ACF_NONE               0x00000000
#define ACF_CONTEXTMENU        0x00000001
#define ACF_CORELINTERNETENUM  0x00000004 // corel suite 8 has this same problem as suite 7 but does not have context menu one so need new bit
#define ACF_OLDCREATEVIEWWND   0x00000004 // PowerDesk relies on CreateViewWindow returning S_OK
#define ACF_WIN95DEFVIEW       0x00000004   // for apps that depend on win95 defview behavior
#define ACF_DOCOBJECT          0x00000002
#define ACF_FLUSHNOWAITALWAYS  0x00000001
#define ACF_MYCOMPUTERFIRST    0x00000008 // MyComp must be first item on the desktop
#define ACF_OLDREGITEMGDN      0x00000010 // Win95-compatible GetDisplayNameOf on regitems
#define ACF_LOADCOLUMNHANDLER  0x00000040 // Dont delay load column handler.
#define ACF_ANSI               0x00000080 // For Apps that Pass in ANSI Strings
#define ACF_FOLDERSCUTASLINK   0x00000100 // make folder shortcuts look like .lnks that point to folders.
#define ACF_WIN95SHLEXEC       0x00000200 // dont use DDEWAIT when thunking to ShellExecEx()
#define ACF_STAROFFICE5PRINTER 0x00000400 // special return values from printer folder GAO
#define ACF_NOVALIDATEFSIDS    0x00000800 // FS pidls should not be validated.
#define ACF_APPISOFFICE        0x01000000 // calling app is office (95, 97, 2000, ++)
#define ACF_KNOWPERPROCESS     0x80000000 // We know the per process flags already.

                                    // The flags that are per-process
#define ACF_PERPROCESSFLAGS    (ACF_CONTEXTMENU | ACF_CORELINTERNETENUM | ACF_OLDCREATEVIEWWND | ACF_WIN95DEFVIEW | \
                                ACF_DOCOBJECT | ACF_FLUSHNOWAITALWAYS | ACF_MYCOMPUTERFIRST | ACF_OLDREGITEMGDN | \
                                ACF_LOADCOLUMNHANDLER | ACF_ANSI | ACF_STAROFFICE5PRINTER | ACF_NOVALIDATEFSIDS)

                                    // Flags that are per caller
#define ACF_PERCALLFLAGS        (ACF_APPISOFFICE | ACF_FOLDERSCUTASLINK)


LWSTDAPI_(DWORD) SHGetAppCompatFlags (DWORD dwFlagsNeeded);

#endif // (_WIN32_IE >= 0x0501)

;end_internal

;begin_both

#ifdef __cplusplus
}
#endif

#ifdef _WIN32
#include <poppack.h>
#endif

#endif

;end_both

#endif  // _INC_SHLWAPIP                                            ;internal
#endif  // _INC_SHLWAPI
