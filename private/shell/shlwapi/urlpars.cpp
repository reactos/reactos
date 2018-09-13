    /*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    urlpars.cpp

Abstract:

    Contains all the worker routines for Combine and Canonicalize

    Contents:
        (ConvertChar)

Author:

    Zeke Lucas (zekel) 16-Dez-96

    Ahsan Kabir (akabir): UrlCombine parser rewritten in July-Sept98

Environment:

    Win32(s) user-mode DLL

Revision History:

    there is about one percent of this derived
    from the Spyglass or MSHTML/WININET codebase

--*/

#include "priv.h"
#include <shstr.h>

#ifdef UNIX
#include <shlobj.h>
#endif

#include <intshcut.h>
#include <w95wraps.h>

#ifdef UNIX
#include "unixstuff.h"
#endif

#define DM_PERF     0           // perf stats

#define PF_LOGSCHEMEHITS    0x00000001


#ifndef CPP_FUNCTIONS
#define CPP_FUNCTIONS
#include <crtfree.h>
#endif

#define USE_FAST_PARSER
#ifdef DEBUG
//#define PROOFREAD_PARSES
#endif

// Same as in wininet; however, this is only theoretical, since urls aren't necessarily so
// constrained. However, this is true throughout the product, so we'll have to do this.

#define INTERNET_MAX_PATH_LENGTH    2048
#define INTERNET_MAX_SCHEME_LENGTH  32

#define HEX_ESCAPE L'%'
#define HEX_ESCAPE_A '%'

#define TERMSTR(pch)      *(pch) = L'\0'

// (WCHAR) 8 is backspace
#define DEADSEGCHAR       ((WCHAR) 8)
#define KILLSEG(pch)      *(pch) = DEADSEGCHAR

#define CR          L'\r'
#define LF          L'\n'
#define TAB         L'\t'
#define SPC         L' '
#define SLASH       L'/'
#define WHACK       L'\\'
#define QUERY       L'?'
#define POUND       L'#'
#define SEMICOLON   L';'
#define COLON       L':'
#define BAR         L'|'
#define DOT         L'.'


#define UPF_SCHEME_OPAQUE           0x00000001  //  should not be treated as heriarchical
#define UPF_SCHEME_INTERNET         0x00000002
#define UPF_SCHEME_NOHISTORY        0x00000004
#define UPF_SCHEME_CONVERT          0x00000008  //  treat slashes and whacks as equiv
#define UPF_SCHEME_DONTCORRECT      0x00000010  //  Don't try to autocorrect to this scheme


#define UPF_SEG_ABSOLUTE            0x00000100  //  the initial segment is the root
#define UPF_SEG_LOCKFIRST           0x00000200  //  this is for file parsing
#define UPF_SEG_EMPTYSEG            0x00000400  //  this was an empty string, but is still important
#define UPF_EXSEG_DIRECTORY         0x00001000  //  the final segment is a "directory" (trailing slash)

#define UPF_FILEISPATHURL           0x10000000  //  this is for file paths, dont unescape because they are actually dos paths
//
//  the masks are for inheritance purposes during BlendParts
//  if you inherit that part you inherit that mask
//
#define UPF_SCHEME_MASK             0x000000FF
#define UPF_SEG_MASK                0x00000F00
#define UPF_EXSEG_MASK              0x0000F000


//  right now these masks are unused, and can be recycled
#define UPF_SERVER_MASK             0x000F0000
#define UPF_QUERY_MASK              0x0F000000

extern "C" int _StrCmpNA(LPCSTR lpStr1, LPCSTR lpStr2, int nChar, BOOL fMBCS);
extern "C" LPSTR _StrChrA(LPCSTR lpStart, WORD wMatch, BOOL fMBCS);
typedef struct _UrlParts
{
    DWORD   dwFlags;
    LPWSTR  pszScheme;
    URL_SCHEME eScheme;
    LPWSTR  pszServer;
    LPWSTR  pszSegments;
    DWORD   cSegments;
    LPWSTR  pszExtraSegs;
    DWORD   cExtraSegs;
    LPWSTR  pszQuery;
    LPWSTR  pszFragment;
} URLPARTS, *PURLPARTS;


HRESULT SHUrlParse(LPCWSTR pszBase, LPCWSTR pszUrl, PSHSTRW pstrOut, DWORD dwFlags);
HRESULT SHUrlCreateFromPath(LPCWSTR pszPath, PSHSTRW pstrOut, DWORD dwFlags);

// Ansi wrappers might overwrite the unicode core's return value
// We should try to prevent that
HRESULT ReconcileHresults(HRESULT hr1, HRESULT hr2)
{
    return (hr2==S_OK) ? hr1 : hr2;
}



PRIVATE CONST WORD isSafe[96] =

/*   Bit 0       alphadigit     -- 'a' to 'z', '0' to '9', 'A' to 'Z'
**   Bit 1       Hex            -- '0' to '9', 'a' to 'f', 'A' to 'F'
**   Bit 2       valid scheme   -- alphadigit | "-" | "." | "+"
**   Bit 3       mark           -- "%" | "$"| "-" | "_" | "." | "!" | "~" | "*" | "'" | "(" | ")" | ","
*/
/*   0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F */
//    {0, 8, 0, 0, 8, 8, 0, 8, 8, 8, 8, 4, 8,12,12, 0,    /* 2x   !"#$%&'()*+,-./  */
// IE4 BETA1: allow + through unmolested.  Should consider other options
// post beta1.  12feb97 tonyci
    {0, 8, 0, 0, 8, 8, 0, 8, 8, 8, 8, 12, 8,12,12, 0,    /* 2x   !"#$%&'()*+,-./  */
     3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 8, 8, 0, 8, 0, 0,    /* 3x  0123456789:;<=>?  */
     8, 3, 3, 3, 3, 3, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1,    /* 4x  @ABCDEFGHIJKLMNO  */
     1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 8,    /* 5X  PQRSTUVWXYZ[\]^_  */
     0, 3, 3, 3, 3, 3, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1,    /* 6x  `abcdefghijklmno  */
     1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 8, 0};   /* 7X  pqrstuvwxyz{|}~  DEL */

PRIVATE const WCHAR hex[] = L"0123456789ABCDEF";

PRIVATE inline BOOL IsSafe(WCHAR ch, WORD mask)
{
    if(((ch > 31 ) && (ch < 128) && (isSafe[ch - 32] & mask)))
        return TRUE;

    return FALSE;
}

#define IsAlphaDigit(c)         IsSafe(c, 1)
#define IsHex(c)                IsSafe(c, 2)
#define IsValidSchemeCharA(c)    IsSafe(c, 5)
#define IsSafePathChar(c)       ((c > 0xff) || IsSafe(c, 9))
#define IsUpper(c)              ((c) >= 'A' && (c) <= 'Z')

PRIVATE inline BOOL IsAsciiCharW(WCHAR ch)
{
    return (!(ch >> 8) && ((CHAR) ch));
}

PRIVATE inline WCHAR Ascii_ToLowerW(WCHAR ch)
{
    return (ch >= L'A' && ch <= L'Z') ? (ch - L'A' + L'a') : ch;
}

BOOL IsValidSchemeCharW(WCHAR ch)
{
    if(IsAsciiCharW(ch))
        return IsSafe( (CHAR) ch, 5);
    return FALSE;
}



WCHAR const c_szHttpScheme[]           = L"http";
WCHAR const c_szFileScheme[]           = L"file";
WCHAR const c_szFTPScheme[]            = L"ftp";
WCHAR const c_szGopherScheme[]         = L"gopher";
WCHAR const c_szMailToScheme[]         = L"mailto";
WCHAR const c_szNewsScheme[]           = L"news";
WCHAR const c_szNNTPScheme[]           = L"nntp";
WCHAR const c_szTelnetScheme[]         = L"telnet";
WCHAR const c_szWAISScheme[]           = L"wais";
WCHAR const c_szMkScheme[]             = L"mk";
WCHAR const c_szHttpsScheme[]          = L"https";
WCHAR const c_szLocalScheme[]          = L"local";
WCHAR const c_szShellScheme[]          = L"shell";
WCHAR const c_szJSScheme[]             = L"javascript";
WCHAR const c_szVSScheme[]             = L"vbscript";
WCHAR const c_szAboutScheme[]          = L"about";
WCHAR const c_szSnewsScheme[]          = L"snews";
WCHAR const c_szResScheme[]            = L"res";

const struct
{
    LPCWSTR pszScheme;
    URL_SCHEME eScheme;
    DWORD cchScheme;
    DWORD dwFlags;
} g_mpUrlSchemeTypes[] =
    {
    // Because we use a linear search, sort this in the order of
    // most common usage.
    { c_szHttpScheme,   URL_SCHEME_HTTP,      SIZECHARS(c_szHttpScheme) - 1,     UPF_SCHEME_INTERNET|UPF_SCHEME_CONVERT},
    { c_szFileScheme,   URL_SCHEME_FILE,      SIZECHARS(c_szFileScheme) - 1,     UPF_SCHEME_CONVERT},
    { c_szFTPScheme,    URL_SCHEME_FTP,       SIZECHARS(c_szFTPScheme) - 1,      UPF_SCHEME_INTERNET|UPF_SCHEME_CONVERT},
    { c_szHttpsScheme,  URL_SCHEME_HTTPS,     SIZECHARS(c_szHttpsScheme) -1,     UPF_SCHEME_INTERNET|UPF_SCHEME_CONVERT|UPF_SCHEME_DONTCORRECT},
    { c_szNewsScheme,   URL_SCHEME_NEWS,      SIZECHARS(c_szNewsScheme) - 1,     UPF_SCHEME_INTERNET|UPF_SCHEME_CONVERT},
    { c_szMailToScheme, URL_SCHEME_MAILTO,    SIZECHARS(c_szMailToScheme) - 1,   UPF_SCHEME_OPAQUE},
    { c_szGopherScheme, URL_SCHEME_GOPHER,    SIZECHARS(c_szGopherScheme) - 1,   UPF_SCHEME_INTERNET|UPF_SCHEME_CONVERT},
    { c_szNNTPScheme,   URL_SCHEME_NNTP,      SIZECHARS(c_szNNTPScheme) - 1,     UPF_SCHEME_INTERNET|UPF_SCHEME_CONVERT},
    { c_szTelnetScheme, URL_SCHEME_TELNET,    SIZECHARS(c_szTelnetScheme) - 1,   UPF_SCHEME_INTERNET|UPF_SCHEME_CONVERT},
    { c_szWAISScheme,   URL_SCHEME_WAIS,      SIZECHARS(c_szWAISScheme) - 1,     0},
    { c_szMkScheme,     URL_SCHEME_MK,        SIZECHARS(c_szMkScheme) - 1,       UPF_SCHEME_NOHISTORY},
    { c_szShellScheme,  URL_SCHEME_SHELL,     SIZECHARS(c_szShellScheme) - 1,    UPF_SCHEME_OPAQUE},
    { c_szLocalScheme,  URL_SCHEME_LOCAL,     SIZECHARS(c_szLocalScheme) - 1,    0},
    { c_szJSScheme,     URL_SCHEME_JAVASCRIPT,SIZECHARS(c_szJSScheme) - 1,       UPF_SCHEME_OPAQUE|UPF_SCHEME_NOHISTORY},
    { c_szVSScheme,     URL_SCHEME_VBSCRIPT,  SIZECHARS(c_szVSScheme) - 1,       UPF_SCHEME_OPAQUE|UPF_SCHEME_NOHISTORY},
    { c_szSnewsScheme,  URL_SCHEME_SNEWS,     SIZECHARS(c_szSnewsScheme) - 1,    UPF_SCHEME_INTERNET|UPF_SCHEME_CONVERT},
    { c_szAboutScheme,  URL_SCHEME_ABOUT,     SIZECHARS(c_szAboutScheme) - 1,    UPF_SCHEME_OPAQUE|UPF_SCHEME_NOHISTORY},
    { c_szResScheme,    URL_SCHEME_RES,       SIZECHARS(c_szResScheme) - 1,      UPF_SCHEME_NOHISTORY}
    };

PRIVATE int _StrCmpNMixed(LPCSTR psz, LPCWSTR pwz, DWORD cch)
{
    int iRet = 0;

    //
    //  we dont have to real mbcs conversion here because we are
    //  guaranteed to have only ascii chars here
    //

    for (;cch; psz++, pwz++, cch--)
    {
        WCHAR ch = *psz;
        if (ch != *pwz)
        {
            //
            //  this makes it case insensitive
            if (IsUpper(ch) && (ch + 32) == *pwz)
                continue;

            if(ch > *pwz)
                iRet = 1;
            else
                iRet = -1;
            break;
        }
    }

    return iRet;
}

//***   g_iScheme -- cache for g_mpUrlSchemeTypes
// DESCRIPTION
//  we call GetSchemeTypeAndFlags many times for the same scheme.  if
//  it's the 0th table entry, no biggee.  if it's a later entry linear
//  search sucks.  add a 1-element MRU cache.  even for the most common
//  (by far) case of "http" (0th entry), we *still* win due to the cheaper
//  StrCmpC and skipped loop.
// NOTES
//  g_iScheme refs/sets are atomic so no need for lock
int g_iScheme;      // last guy we hit

#ifdef DEBUG
int g_cSTTot, g_cSTHit, g_cSTHit0;
#endif

//
//  all of the pszScheme to nScheme functions are necessary at this point
//  because some parsing is vioent, and some is necessarily soft
//
PRIVATE URL_SCHEME
GetSchemeTypeAndFlagsW(LPCWSTR pszScheme, DWORD cchScheme, LPDWORD pdwFlags)
{
    DWORD i;

    ASSERT(pszScheme);


#ifdef DEBUG
    if ((g_cSTTot % 10) == 0)
        TraceMsg(DM_PERF, "gstaf: tot=%d hit=%d hit0=%d", g_cSTTot, g_cSTHit, g_cSTHit0);
#endif
    DBEXEC(TRUE, g_cSTTot++);
    // check cache 1st
    i = g_iScheme;
    if (cchScheme == g_mpUrlSchemeTypes[i].cchScheme
      && StrCmpNCW(pszScheme, g_mpUrlSchemeTypes[i].pszScheme, cchScheme) == 0)
    {
        DBEXEC(TRUE, i == 0 ? g_cSTHit0++ : g_cSTHit++);
Lhit:
        if (pdwFlags)
            *pdwFlags = g_mpUrlSchemeTypes[i].dwFlags;

        // update cache (unconditionally)
        g_iScheme = i;

        return g_mpUrlSchemeTypes[i].eScheme;
    }

    for (i = 0; i < ARRAYSIZE(g_mpUrlSchemeTypes); i++)
    {
        if(cchScheme == g_mpUrlSchemeTypes[i].cchScheme
          && 0 == StrCmpNIW(pszScheme, g_mpUrlSchemeTypes[i].pszScheme, cchScheme))
            goto Lhit;
    }

    if (pdwFlags)
    {
        *pdwFlags = 0;
    }
    return URL_SCHEME_UNKNOWN;
}

PRIVATE URL_SCHEME
GetSchemeTypeAndFlagsA(LPCSTR pszScheme, DWORD cchScheme, LPDWORD pdwFlags)
{
    DWORD i;

    ASSERT(pszScheme);


    for (i = 0; i < ARRAYSIZE(g_mpUrlSchemeTypes); i++)
    {
        if(0 == _StrCmpNMixed(pszScheme, g_mpUrlSchemeTypes[i].pszScheme, cchScheme))
        {
            if (pdwFlags)
                *pdwFlags = g_mpUrlSchemeTypes[i].dwFlags;
            return g_mpUrlSchemeTypes[i].eScheme;
        }
    }

    if (pdwFlags)
    {
        *pdwFlags = 0;
    }
    return URL_SCHEME_UNKNOWN;
}

PRIVATE DWORD GetSchemeFlags(URL_SCHEME eScheme)
{
    DWORD i;

    for (i = 0; i < ARRAYSIZE(g_mpUrlSchemeTypes); i++)
    {
        if(eScheme == g_mpUrlSchemeTypes[i].eScheme)
        {
            return g_mpUrlSchemeTypes[i].dwFlags;
        }
    }
    return 0;
}


/*----------------------------------------------------------
Purpose: Return the scheme ordinal type (URL_SCHEME_*) based on the
         URL string.


Returns: URL_SCHEME_ ordinal
Cond:    --
*/

PRIVATE inline BOOL IsSameSchemeW(LPCWSTR pszLocal, LPCWSTR pszGlobal, DWORD cch)
{
    ASSERT(pszLocal);
    ASSERT(pszGlobal);
    ASSERT(cch);

    return !StrCmpNIW(pszLocal, pszGlobal, cch);
}

PRIVATE BOOL IsSameSchemeA(LPCSTR pszLocal, LPCWSTR pszGlobal, DWORD cch)
{
    ASSERT(pszLocal);
    ASSERT(pszGlobal);
    ASSERT(cch);

    return !_StrCmpNMixed(pszLocal, pszGlobal, cch);
}


PRIVATE URL_SCHEME
SchemeTypeFromStringA(
   LPCSTR psz,
   DWORD cch)
{
   DWORD i;

   // psz is a counted string (by cch), not a null-terminated string,
   // so use IS_VALID_READ_BUFFER instead of IS_VALID_STRING_PTRA.
   ASSERT(IS_VALID_READ_BUFFER(psz, CHAR, cch));
   ASSERT(cch);
   // We use a linear search.  A binary search wouldn't pay off
   // because the list isn't big enough, and we can sort the list
   // according to the most popular protocol schemes and pay off
   // bigger.

   for (i = 0; i < ARRAYSIZE(g_mpUrlSchemeTypes); i++)
   {
       if(cch == g_mpUrlSchemeTypes[i].cchScheme &&
           IsSameSchemeA(psz, g_mpUrlSchemeTypes[i].pszScheme, cch))
            return g_mpUrlSchemeTypes[i].eScheme;
   }

   return URL_SCHEME_UNKNOWN;
}


PRIVATE URL_SCHEME
SchemeTypeFromStringW(
   LPCWSTR psz,
   DWORD cch)
{
   DWORD i;

   // psz is a counted string (by cch), not a null-terminated string,
   // so use IS_VALID_READ_BUFFER instead of IS_VALID_STRING_PTRW.
   ASSERT(IS_VALID_READ_BUFFER(psz, WCHAR, cch));
   ASSERT(cch);

   // We use a linear search.  A binary search wouldn't pay off
   // because the list isn't big enough, and we can sort the list
   // according to the most popular protocol schemes and pay off
   // bigger.

   for (i = 0; i < ARRAYSIZE(g_mpUrlSchemeTypes); i++)
   {
       if(cch == g_mpUrlSchemeTypes[i].cchScheme &&
           IsSameSchemeW(psz, g_mpUrlSchemeTypes[i].pszScheme, cch))
            return g_mpUrlSchemeTypes[i].eScheme;
   }

   return URL_SCHEME_UNKNOWN;
}

//
//  these are used during path fumbling that i do
//  each string between a path delimiter ( '/' or '\')
//  is a segment.  we dont ever really care about
//  empty ("") segments, so it is best to use
//  NextLiveSegment().
//
inline PRIVATE LPWSTR
NextSegment(LPWSTR psz)
{
    ASSERT (psz);
    return psz + lstrlenW(psz) + 1;
}

#define IsLiveSegment(p)    ((p) && (*p) != DEADSEGCHAR)

PRIVATE LPWSTR
NextLiveSegment(LPWSTR pszSeg, DWORD *piSeg, DWORD cSegs)
{
    if(pszSeg) do
    {
        //
        //  count the number of dead segments that we skip.
        //  if the segment isnt dead, then we can just skip one,
        //  the current one.
        //
        DWORD cSkip;
        for (cSkip = 0; (*pszSeg) == DEADSEGCHAR; pszSeg++, cSkip++);
        cSkip = cSkip ? cSkip : 1;

        if((*piSeg) + cSkip < cSegs)
        {

            pszSeg = NextSegment(pszSeg);
            (*piSeg) += cSkip;
        }
        else
            pszSeg = NULL;

    } while (pszSeg && (*pszSeg == DEADSEGCHAR));

    return pszSeg;
}

PRIVATE LPWSTR
LastLiveSegment(LPWSTR pszSeg, DWORD cSegs, BOOL fFailIfFirst)
{
    DWORD iSeg = 0;
    LPWSTR pszLast = NULL;
    BOOL fLastIsFirst = FALSE;

    if(cSegs)
    {
        if(IsLiveSegment(pszSeg))
        {
            pszLast = pszSeg;
            fLastIsFirst = TRUE;
        }

        while(pszSeg = NextLiveSegment(pszSeg, &iSeg, cSegs))
        {
            if(!pszLast)
                fLastIsFirst = TRUE;
            else
                fLastIsFirst = FALSE;

            pszLast = pszSeg;
        }

        if(fFailIfFirst && fLastIsFirst)
            pszLast = NULL;
    }

    return pszLast;
}

PRIVATE LPWSTR
FirstLiveSegment(LPWSTR pszSeg, DWORD *piSeg, DWORD cSegs)
{
    ASSERT(piSeg);

    *piSeg = 0;

    if(!pszSeg || !cSegs)
        return NULL;

    if(!IsLiveSegment(pszSeg))
        pszSeg = NextLiveSegment(pszSeg, piSeg, cSegs);

    return pszSeg;
}

inline BOOL IsDosDrive(LPCWSTR p)
{
    return (*p && p[1] == COLON);
}

inline BOOL IsDosPath(LPCWSTR p)
{
    return (*p == WHACK ||  IsDosDrive(p));
}

inline BOOL IsDriveUrl(const WCHAR *p)
{
    return (*p && p[1] == BAR);
}

inline BOOL IsDrive(LPCWSTR p)
{
    return (IsDosDrive(p) || IsDriveUrl(p));
}

inline BOOL IsSeparator(const WCHAR *p)
{
    return (*p == SLASH || *p == WHACK );
}

inline BOOL IsAbsolute(const WCHAR *p)
{
#ifndef UNIX
    return (IsSeparator(p) || IsDrive(p));
#else
    return (IsSeparator(p)) ;
#endif
}

#define IsUNC(pathW) PathIsUNCW(pathW)

inline BOOL IsDot(LPCWSTR p)     // if p == "." return TRUE
{
    return (*p == DOT && !p[1]);
}

inline BOOL IsDotDot(LPCWSTR p)  // if p == ".." return TRUE
{
    return (*p == DOT && p[1] == DOT && !p[2]);
}

//+---------------------------------------------------------------------------
//
//  Method:     ConvertChar
//
//  Synopsis:
//
//  Arguments:  [szStr] --
//              [cIn] --
//              [cOut] --
//
//  Returns:
//
//  History:    03-20-96    JoeS (Joe Souza)    Created
//
//  Notes:
//
//----------------------------------------------------------------------------
static void ConvertChar(LPWSTR ptr, WCHAR cIn, WCHAR cOut, BOOL fProtectExtra)
{
    while (*ptr)
    {
        if (fProtectExtra && (*ptr == QUERY || *ptr == POUND ))
        {
            break;
        }

        if (*ptr == cIn)
        {
            *ptr = cOut;
        }

        ptr++;
    }
}

PUBLIC void WininetFixFileSlashes(WCHAR *p)
{
    // NB: This function assumes that p points to a file URL.
    // The file URL *MUST* be of the form "file://...".
    // HTParse() guarantees that this will be so.

    int schemelen = 0;

    schemelen = SIZECHARS(L"file://") - 1;

    /* In UNIX system, we don't need to convert the SLASH to WHACK */
    if (p && lstrlenW(p) > schemelen)
    {
#ifdef UNIX
        ConvertChar(p + schemelen, WHACK, SLASH, TRUE);
#else
        ConvertChar(p + schemelen, SLASH, WHACK, TRUE);
#endif
    }
}

//
//  in the URL spec, it says that all whitespace should be ignored
//  due to the fact that it is possible to introduce
//  new whitespace and eliminate other whitespace
//  however, we are only going to strip out TAB CR LF
//  because we consider SPACE's to be significant.
//

PRIVATE inline BOOL IsInsignificantWhite(WCHAR ch)
{
    return (ch == TAB ||
            ch == CR ||
            ch == LF);
}

#define IsWhite(c)      ((DWORD) (c) > 32 ? FALSE : TRUE)

PRIVATE void TrimAndStripInsignificantWhite(WCHAR *psz)
{
    ASSERT(psz);

    if(*psz)
    {

        LPCWSTR pszSrc = psz;
        LPWSTR pszDest = psz;
        LPWSTR pszLastSpace = NULL;

        // first trim the front side by just moving the source pointer.
        while(*pszSrc && IsWhite(*pszSrc)) {
            pszSrc++;
        }

        //
        // Copy the body stripping "insignificant" white spaces.
        // Remember the last white space to trim trailing space later.
        //
        while (*pszSrc)
        {
            if(IsInsignificantWhite(*pszSrc)) {
                pszSrc++;
            } else {
                if (IsWhite(*pszSrc)) {
                    if (pszLastSpace==NULL) {
                        pszLastSpace = pszDest;
                    }
                } else {
                    pszLastSpace = NULL;
                }

                *pszDest++ = *pszSrc++;
            }
        }

        // Trim the trailing space
        if (pszLastSpace) {
            *pszLastSpace = L'\0';
        } else {
            *pszDest = L'\0';
        }

    }

#if 0
    // BUGBUG - NETSCAPE compatibility - zekel 29-JAN-97
    //  we want to leave one space in the string
    if(*psz== L'\0')
    {
        psz[0] = SPC;
        psz[1] = L'\0';
    }
#endif

}


/*+++

  BreakUrl()
    Break a URL for its consituent parts

  Parameters
  IN -
            the URL to crack open, need not be fully qualified

  OUT -
    parts       absolute or relative may be nonzero (but not both).
                host, anchor and access may be nonzero if they were specified.
                Any which are nonzero point to zero terminated strings.

  Returns
    VOID

  Details -

  WARNING !! function munges the incoming buffer

---*/

struct EXTKEY
{
    PCSTR  szExt;
    PCWSTR wszExt;
    DWORD cchExt;
};

const EXTKEY ExtTable[] = {
       {    ".html",        L".html",           ARRAYSIZE(".html") - 1 },
       {    ".htm",         L".htm",            ARRAYSIZE(".htm") - 1  },
       {    ".xml",         L".xml",            ARRAYSIZE(".xml") - 1  },
       {    ".doc",         L".doc",            ARRAYSIZE(".doc") - 1  },
       {    ".xls",         L".xls",            ARRAYSIZE(".xls") - 1  },
       {    ".ppt",         L".ppt",            ARRAYSIZE(".ppt") - 1  },
       {    ".rtf",         L".rtf",            ARRAYSIZE(".rtf") - 1  },
       {    ".dot",         L".dot",            ARRAYSIZE(".dot") - 1  },
       {    ".xlw",         L".xlw",            ARRAYSIZE(".xlw") - 1  },
       {    ".pps",         L".pps",            ARRAYSIZE(".pps") - 1  },
       {    ".xlt",         L".xlt",            ARRAYSIZE(".xlt") - 1  },
       {    ".pot",         L".pot",            ARRAYSIZE(".pot") - 1  }
};

inline BOOL CompareExtA(PCSTR psz, DWORD_PTR cch)
{
    for (DWORD i=0; i < ARRAYSIZE(ExtTable); i++)
    {
        if (ExtTable[i].cchExt>cch)
            continue;

        if (!StrCmpNIA(psz - (LONG_PTR)ExtTable[i].cchExt, ExtTable[i].szExt, ExtTable[i].cchExt))
            return TRUE;
    }
    return FALSE;
}

inline BOOL CompareExtW(PCWSTR pwsz, DWORD_PTR cch)
{
    for (DWORD i=0; i < ARRAYSIZE(ExtTable); i++)
    {
        if (ExtTable[i].cchExt>cch)
            continue;

        if (!StrCmpNIW(pwsz - (LONG_PTR)ExtTable[i].cchExt, ExtTable[i].wszExt, ExtTable[i].cchExt))
            return TRUE;
    }
    return FALSE;
}


PRIVATE LPCSTR FindFragmentA(LPCSTR psz, BOOL fMBCS, BOOL fIsFile)
{
    CHAR *pch = _StrChrA(psz, POUND, fMBCS);
    if(pch && fIsFile)
    {
        CHAR *pchQuery = _StrChrA(psz, QUERY, fMBCS);
        if (pchQuery && (pchQuery < pch))
            goto exit;

        do
        {
            LONG_PTR cch = pch - psz;

            // BUGBUG: we shouldn't hardcode ".htm".

            //
            //  BUGBUGHACK - #s are significant in dospaths - zekel 9-JUL-97
            //  so we want to check the path in front and make sure
            //  that it is an html file.  we believe this heuristic should work
            //  in about 99% of all cases.
            //
            // if it is not an html file it is not a hash
            if (CompareExtA(pch, cch))
            {
                break;
            }
        } while (pch = _StrChrA(++pch, POUND, fMBCS));
    }
exit:
    return pch;
}

PRIVATE LPCWSTR FindFragmentW(LPCWSTR psz, BOOL fIsFile)
{
    WCHAR *pch = StrChrW(psz, POUND);
    if(pch && fIsFile)
    {
        WCHAR *pchQuery = StrChrW(psz, QUERY);
        if (pchQuery && (pchQuery < pch))
            goto exit;

        do
        {
            LONG_PTR cch = pch - psz;

            // BUGBUG: we shouldn't hardcode ".htm".

            //
            //  BUGBUGHACK - #s are significant in dospaths - zekel 9-JUL-97
            //  so we want to check the path in front and make sure
            //  that it is an html file.  we believe this heuristic should work
            //  in about 99% of all cases.
            //
            // if it is not an html file it is not a hash
            if (CompareExtW(pch, cch))
            {
                break;
            }

        } while (pch = StrChrW(++pch, POUND));
    }
exit:
    return pch;
}

PRIVATE VOID BreakFragment(LPWSTR *ppsz, PURLPARTS parts)
{
    ASSERT(ppsz);
    ASSERT(*ppsz);

    //
    //  BUGBUG - Opaque URLs are not allowed to use fragments - zekel 27-feb-97
    //  Is it possible for an opaque URL to use a fragment?
    //  right now we assume not.  i suspect so but will leave it this way for now
    //  this is especially important to javascript and vbscript
    //
    if(!**ppsz || parts->dwFlags & UPF_SCHEME_OPAQUE)
        return;

    WCHAR *pch = (LPWSTR) FindFragmentW(*ppsz, parts->eScheme == URL_SCHEME_FILE);

    if (pch)
    {
        TERMSTR(pch);
        parts->pszFragment = pch +1;
    }
}

PRIVATE inline BOOL IsUrlPrefixA(LPCSTR psz)
{
    //
    // Optimized for this particular case. Notice that most of it
    // will be lego-ized out anyway.
    //
    if (psz[0]=='u' || psz[0]=='U') {
        if (psz[1]=='r' || psz[1]=='R') {
            if (psz[2]=='l' || psz[2]=='L') {
                return TRUE;
            }
        }
    }
    return FALSE;
    // return !StrCmpNIA(psz, c_szURLPrefixA, c_cchURLPrefix);
}

PRIVATE inline BOOL IsUrlPrefixW(LPCWSTR psz)
{
    //
    // Optimized for this particular case. Notice that most of it
    // will be lego-ized out anyway.
    //
    if (psz[0]==L'u' || psz[0]==L'U') {
        if (psz[1]==L'r' || psz[1]==L'R') {
            if (psz[2]==L'l' || psz[2]==L'L') {
                return TRUE;
            }
        }
    }
    return FALSE;
    // return !StrCmpNIW(psz, c_szURLPrefixW, c_cchURLPrefix);
}

//
//  if FindScheme() succeeds, it returns a pointer to the scheme,
//  and the cch holds the count of chars for the scheme
//  if it fails, and cch is non-zero then cch is how much should be skipped.
//  this is to allow "URL:/foo/bar", a relative URL with the "URL:" prefix.
//
LPCSTR FindSchemeA(LPCSTR psz, LPDWORD pcchScheme)
{
    LPCSTR pch;
    DWORD cch;

    ASSERT(pcchScheme);
    ASSERT(psz);

    *pcchScheme = 0;

    for (pch = psz, cch = 0; *pch; pch++, cch++)
    {
        if (*pch == ':')
        {
            if (IsUrlPrefixA(psz))
            {
                psz = pch +1;

                //  set pcchScheme to skip past "URL:"
                *pcchScheme = cch + 1;

                //  reset cch for the scheme len
                cch = -1;
                continue;
            }
            else
            {
                //
                //  Scheme found if it is at least two characters
                if(cch > 1)
                {
                    *pcchScheme = cch;
                    return psz;
                }
                break;
            }
        }
        if(!IsValidSchemeCharA(*pch))
            break;
    }

    return NULL;
}

//
//  FindSchemeW() around for Perf reasons for ParseURL()
//  Any changes in either FindScheme() needs to reflected in the other
//
LPCWSTR FindSchemeW(LPCWSTR psz, LPDWORD pcchScheme, BOOL fAllowSemicolon = FALSE)
{
    LPCWSTR pch;
    DWORD cch;

    ASSERT(pcchScheme);
    ASSERT(psz);

    *pcchScheme = 0;

    for (pch = psz, cch = 0; *pch; pch++, cch++)
    {

        if (*pch == L':' ||

            // Autocorrect permits a semicolon typo
            (fAllowSemicolon && *pch == L';'))
        {
            if (IsUrlPrefixW(psz))
            {
                psz = pch +1;

                //  set pcchScheme to skip past "URL:"
                *pcchScheme = cch + 1;

                //  reset cch for the scheme len
                cch = -1;
                continue;
            }
            else
            {
                //
                //  Scheme found if it is at least two characters
                if(cch > 1)
                {
                    *pcchScheme = cch;
                    return psz;
                }
                break;
            }
        }
        if(!IsValidSchemeCharW(*pch))
            break;
    }

    return NULL;
}

PRIVATE DWORD
CountSlashes(LPCWSTR *ppsz)
{
    DWORD cSlashes = 0;
    LPCWSTR pch = *ppsz;

    while (IsSeparator(pch))
    {
        *ppsz = pch;
        pch++;
        cSlashes++;
    }

    return cSlashes;
}


PRIVATE LPCWSTR
FindDosPath(LPCWSTR psz)
{
    if (IsDosDrive(psz) || IsUNC(psz))
    {
        return psz;
    }
    else
    {
        DWORD cch;
        LPCWSTR pszScheme = FindSchemeW(psz, &cch);

        if (pszScheme && URL_SCHEME_FILE == GetSchemeTypeAndFlagsW(pszScheme, cch, NULL))
        {
            LPCWSTR pch = psz + cch + 1;
            DWORD c = CountSlashes(&pch);

            switch (c)
            {
            case 2:
                if(IsDosDrive(++pch))
                    return pch;
                break;

            case 4:
                return --pch;
            }
        }
    }
    return NULL;
}


/*+++

  WininetCopyUrlForParse()
    this copies the url and prepends a "file://" if necessary
    This should never be called except from wininet
    everyone else should be calling UrlCreateFromPath()

  Parameters
  IN -
    pszDst      the destination buffer
    pszSrc      source buffer

  OUT -
    pszDst      is filled with a Live URL

  Returns
  VOID

  NOTE - Assume "file:" if no scheme and it looks like fully-qualified file path.
---*/
static const WCHAR c_szFileSchemeString[] = L"file://";

PRIVATE HRESULT
WininetCopyUrlForParse(PSHSTRW pstrDst, LPCWSTR pszSrc)
{
#ifndef UNIX
    if (IsDrive(pszSrc) || IsUNC(pszSrc))
    {
        //
        // NOTE: the first SetStr will always succeed
        // because the default buffer is more than "file://"
        pstrDst->SetStr(c_szFileSchemeString);
        return pstrDst->Append(pszSrc);
    }
    else
#endif /* !UNIX */
        return pstrDst->SetStr(pszSrc);

}

PRIVATE HRESULT
CopyUrlForParse(LPCWSTR pszUrl, PSHSTRW pstrUrl, DWORD dwFlags)
{
    LPCWSTR pch;
    HRESULT hr;
    //
    //  now we will make copies of the URLs so that we can rip them apart
    //  WininetCopyUrlForParse() will prepend a file: if it wants...
    //

    if(dwFlags & URL_WININET_COMPATIBILITY)
    {
        hr = WininetCopyUrlForParse(pstrUrl, pszUrl);
    }
    else if(pch = FindDosPath(pszUrl))
    {
        hr = SHUrlCreateFromPath(pch, pstrUrl, dwFlags);
    }
    else
    {
        hr = pstrUrl->SetStr(pszUrl);
    }

    // Trim leading and trailing whitespace
    // Remove tab and CRLF characters.  Netscape does this.
    if(SUCCEEDED(hr))
        TrimAndStripInsignificantWhite((LPWSTR) *pstrUrl);


    return hr;
}


PRIVATE VOID BreakScheme(LPWSTR *ppsz, PURLPARTS parts)
{
    if(!**ppsz || IsDrive(*ppsz))
        return;

    DWORD cch;

    //
    //  if FindScheme() succeeds, it returns a pointer to the scheme,
    //  and the cch holds the count of chars for the scheme
    //  if it fails, and cch is none zero then cch is how much should be skipped.
    //  this is to allow "URL:/foo/bar", a relative URL with the "URL:" prefix.
    //
    if(NULL != (parts->pszScheme = (LPWSTR) FindSchemeW(*ppsz, &cch)))
    {
        parts->pszScheme[cch] = '\0';
        CharLowerW(parts->pszScheme);

        //  put the pointer past the scheme for next Break()
        *ppsz = parts->pszScheme + cch + 1;


#ifdef DEBUG
        if (g_dwPrototype & PF_LOGSCHEMEHITS)
        {
            //  this is for logging of url schemes, to make sure that we have the right order
            int c = GetPrivateProfileIntW(L"SchemeHits", parts->pszScheme, 0, L"UrlPars.ini");
            WCHAR szc[25];
            wsprintfW(szc, L"%d", ++c);
            WritePrivateProfileStringW(L"SchemeHits", parts->pszScheme, szc, L"UrlPars.ini");
        }
#endif //DEBUG


        parts->eScheme = GetSchemeTypeAndFlagsW(parts->pszScheme, cch, &parts->dwFlags);
    }
    else if (cch)
        *ppsz += cch + 1;
}


PRIVATE VOID BreakQuery(LPWSTR *ppsz, PURLPARTS parts)
{
    WCHAR *pch;

    if(!**ppsz)
        return;

    if(parts->dwFlags & UPF_SCHEME_OPAQUE)
        return;

    pch = StrChrW(*ppsz, QUERY);

    //
    //  BUGBUG NETSCAPE COMPATBILITY - zekel - 27-JAN-97
    //  we will also get http://foo#frag?query
    //  even tho legally it should be http://foo?query#frag
    //  of course we will put it back together the right way.
    //
    if(!pch && parts->pszFragment)
        pch = StrChrW(parts->pszFragment, QUERY);

    //  found our query string...
    if (pch)
    {
        TERMSTR(pch);
        parts->pszQuery = pch + 1;
    }
}

PRIVATE VOID MkBreakServer(LPWSTR *ppsz, PURLPARTS parts)
{
    //
    //  NOTE:  we dont convert WHACKs to SLASHs because mk can be of the
    //  form <mk:@class:\\Server\Share\file.itl/path/in/the/file.gif
    //  and we want to preserve the DOS/UNC path as it is
    //

    if (**ppsz == TEXT('@'))
    {
        WCHAR *pch;
        // treat everything to separator as host
        //
        parts->pszServer = *ppsz;

        pch = StrChrW(*ppsz ,SLASH);
        if (pch)
        {
            parts->dwFlags |= UPF_SEG_ABSOLUTE;
            TERMSTR(pch);
            *ppsz = pch + 1;
        }
        else
            *ppsz += lstrlenW(*ppsz);
    }
}

PRIVATE VOID DefaultBreakServer(LPWSTR *ppsz, PURLPARTS parts)
{
    if (**ppsz == SLASH)
    {
        parts->dwFlags |= UPF_SEG_ABSOLUTE;

        (*ppsz)++;

        if (**ppsz == SLASH)
        {
            // we have a winner!
            WCHAR * pch;

            parts->pszServer = (*ppsz) + 1;

            pch = StrChrW(parts->pszServer, SLASH);

            if(pch)
            {
                TERMSTR(pch);
                *ppsz = pch + 1;
            }
            else
                *ppsz = *ppsz + lstrlenW(*ppsz);
        }
    }
    else if(parts->pszScheme)
        parts->dwFlags |= UPF_SCHEME_OPAQUE;
}

PRIVATE VOID FileBreakServer(LPWSTR *ppsz, PURLPARTS parts)
{
    LPWSTR pch;

    //  CountSlashes() will set *ppsz to the last slash
    DWORD cSlashes = CountSlashes((LPCWSTR *)ppsz);

    if(cSlashes || IsDrive(*ppsz))
        parts->dwFlags |= UPF_SEG_ABSOLUTE;

    switch (cSlashes)
    {
    case 0:
        break;

    case 4:
        // we identify file://\\UNC as a true DOS path with no escaped characters
        parts->dwFlags |= UPF_FILEISPATHURL;

        // fall through

    case 2:
        if(IsDrive((*ppsz) + 1))
        {
            //  this is a root drive
            TERMSTR(*ppsz);
            parts->pszServer = *ppsz;
            (*ppsz)++;
            // we identify file://C:\PATH as a true DOS path with no escaped characters
            parts->dwFlags |= UPF_FILEISPATHURL;
            break;
        } //else fallthru to UNC handling
        // fall through

    case 5:
    case 6:
        //
        // cases like "file:////..." or "file://///..."
        // we see this as a UNC path
        // lets set the server
        //
        parts->pszServer = ++(*ppsz);
        for(pch = *ppsz; *pch && !IsSeparator(pch); pch++);

        if(pch && *pch)
        {
            TERMSTR(pch);
            *ppsz = pch + 1;
        }
        else
            *ppsz = pch + lstrlenW(pch);
        break;

    case 1:
        //
        //we think of "file:/..." as on the local machine
        // so we have zero length pszServer
        //
    case 3:
        //
        //we think of file:///... as properly normalized on the local machine
        // so we have zero length pszServer
        //
    default:
        //  there is just too many, we pretend that there is just one and ignore
        //  the rest
        TERMSTR(*ppsz);
        parts->pszServer = *ppsz;
        (*ppsz)++;
        break;
    }

    //  detect file://localserver/c:/path
    if(parts->pszServer && !StrCmpIW(parts->pszServer, L"localhost"))
        parts->pszServer = NULL;
}

PRIVATE VOID BreakServer(LPWSTR *ppsz, PURLPARTS parts, BOOL fConvert)
{
    if(!**ppsz || parts->dwFlags & UPF_SCHEME_OPAQUE)
        return;

    //
    //  BUGBUG - we pretend that whacks are the equiv of slashes - zekel 17-MAR-97
    //  this is because the internet uses slashes and DOS
    //  uses whacks.  so for useability's sake we allow both.
    //  but not in all cases.  in particular, the "mk:" stream
    //  protocol depends upon the buggy behavior of one of IE30's
    //  many URL parsers treating relative URLs with whacks as one
    //  segment.
    //  NOTE:  IE30 had inconsistent behavior WRT URLs.  so we handled
    //  this case differently depending on when we saw, looked, touched, or
    //  played with these URLs.  wininet would always convert, but mshtml
    //  sometimes would other times not.
    //
    //  with MK: we cannot convert the base, or the relative
    //  but in breakpath we have to allow for the use of WHACK
    //  to indicate a root path
    //
    //  we dont have to fProtectExtra because query and fragments
    //  are already broken off if necessary.
    if (fConvert)
        ConvertChar(*ppsz, WHACK, SLASH, FALSE);

    switch(parts->eScheme)
    {
    case URL_SCHEME_FILE:
        FileBreakServer(ppsz, parts);
        break;

    case URL_SCHEME_MK:
        MkBreakServer(ppsz, parts);
        break;

    default:
        DefaultBreakServer(ppsz, parts);
        break;
    }
}

PRIVATE VOID DefaultBreakSegments(LPWSTR psz, PURLPARTS parts)
{
    WCHAR *pch;

    while (pch = StrChrW(psz, SLASH))
    {
        parts->cSegments++;
        TERMSTR(pch);
        psz = pch + 1;
    }

    if(!*psz || IsDot(psz) || IsDotDot(psz))
    {
        if (!*psz && parts->cSegments > 1)
            parts->cSegments--;

        parts->dwFlags |= UPF_EXSEG_DIRECTORY;
    }
}

PRIVATE VOID DefaultBreakPath(LPWSTR *ppsz, PURLPARTS parts)
{
    if(!**ppsz)
        return;

    //
    //  this will keep the drive letter from being backed up over
    //  during canonicalization.  if we want keep the UNC share
    //  from being backed up we should do it here
    //  or in FileBreakServer() similarly
    //
    if(IsDrive(*ppsz))
    {
        parts->dwFlags |= UPF_SEG_LOCKFIRST;
        // also convert "c|" to "c:"
    }

    parts->pszSegments = *ppsz;
    parts->cSegments = 1;

    if(!(parts->dwFlags & UPF_SCHEME_OPAQUE))
        DefaultBreakSegments(parts->pszSegments, parts);

}

PRIVATE VOID BreakPath(LPWSTR *ppsz, PURLPARTS parts)
{
    if(!**ppsz)
        return;

    if (parts->dwFlags & UPF_SCHEME_OPAQUE)
    {
        parts->pszSegments = *ppsz;
        parts->cSegments = 1;
    }
    else
    {
        //
        //  we only need to check for absolute when there was
        //  no server segment.  if there was a server segment,
        //  then absolute has already been set, and we need
        //  to preserve any separators that exist in the path
        //
        if(!parts->pszServer && IsSeparator(*ppsz))
        {
            parts->dwFlags |= UPF_SEG_ABSOLUTE;
            (*ppsz)++;
        }

        DefaultBreakPath(ppsz, parts);
    }
}

#define BreakUrl(s, p)         BreakUrls(s, p, NULL, NULL, NULL, 0)


BOOL _ShouldBreakBase(PURLPARTS parts, LPCWSTR pszBase)
{
    if (pszBase)
    {
        if (!parts->pszScheme)
            return TRUE;

        DWORD cch;
        LPCWSTR pszScheme = FindSchemeW(pszBase, &cch);

        //  this means that this will only optimize on known schemes
        //  if both urls use URL_SCHEME_UNKNOWN...then we parse both.
        if (pszScheme && parts->eScheme == GetSchemeTypeAndFlagsW(pszScheme, cch, NULL))
            return TRUE;

    }

    return FALSE;
}

//
//  **BreakUrls()**
//  RETURNS
//  S_OK        if the two urls need to be blended
//  S_FALSE     if pszUrl is absolute, or there is no pszBase
//  failure     some sort of memory allocation error
//
PRIVATE HRESULT
BreakUrls(LPWSTR pszUrl, PURLPARTS parts, LPCWSTR pszBase, PSHSTRW pstrBase, PURLPARTS partsBase, DWORD dwFlags)
{
    HRESULT hr = S_FALSE;
    ASSERT(pszUrl && parts);

    ZeroMemory(parts, SIZEOF(URLPARTS));

    if(!*pszUrl)
        parts->dwFlags |= UPF_SEG_EMPTYSEG;

    //
    //  WARNING: this order is specific, according to the proposed standard
    //
    if(*pszUrl || pszBase)
    {
        BOOL fConvert;

        BreakScheme(&pszUrl, parts);
        BreakFragment(&pszUrl, parts);
        BreakQuery(&pszUrl, parts);

        //
        //  this is the first time that we need to access
        //  pszBase if it exists, so this is when we copy and parse
        //
        if (_ShouldBreakBase(parts, pszBase))
        {
            hr = CopyUrlForParse(pszBase, pstrBase, dwFlags);

            //  this will be some kind of memory error
            if(FAILED(hr))
                return hr;

            // ASSERT(hr != S_FALSE);

            BreakUrl(pstrBase->GetStr(), partsBase);
            fConvert = (partsBase->dwFlags & UPF_SCHEME_CONVERT);
        }
        else
            fConvert = (parts->dwFlags & UPF_SCHEME_CONVERT);

        BreakServer(&pszUrl, parts, fConvert);
        BreakPath(&pszUrl, parts);
    }

    return hr;
}


/*+++
  BlendParts()  & all dependant Blend* functions
        Blends the parts structures into one, taking the relavent
        bits from each one and dumping the unused data.

  Parameters
  IN -
    partsUrl        the primary or relative parts   - Takes precedence
    partsBase       the base or referrers parts

  OUT -
    partsOut        the combined result

  Returns
  VOID -

  NOTE:  this will frequently NULL out the entire partsBase.
---*/

PRIVATE VOID
BlendScheme(PURLPARTS partsUrl, PURLPARTS partsBase, PURLPARTS partsOut)
{
    if(partsUrl->pszScheme)
    {
        LPCWSTR pszScheme = partsOut->pszScheme = partsUrl->pszScheme;
        URL_SCHEME eScheme = partsOut->eScheme = partsUrl->eScheme;

        partsOut->dwFlags |= (partsUrl->dwFlags & UPF_SCHEME_MASK);

        //
        //  this checks to make sure that these are the same scheme, and
        //  that the scheme is allowed to be used in relative URLs
        //  file: is not allowed to because of weirdness with drive letters
        //  and \\UNC\shares
        //
        if ((eScheme && (eScheme != partsBase->eScheme) || eScheme == URL_SCHEME_FILE) ||
            (!partsBase->pszScheme) ||
            (partsBase->pszScheme && StrCmpW(pszScheme, partsBase->pszScheme)))
        {
            //  they are different schemes.  DUMP partsBase.

            ZeroMemory(partsBase, SIZEOF(URLPARTS));
        }
    }
    else
    {
        partsOut->pszScheme = partsBase->pszScheme;
        partsOut->eScheme = partsBase->eScheme;
        partsOut->dwFlags |= (partsBase->dwFlags & UPF_SCHEME_MASK);
    }
}

PRIVATE VOID
BlendServer(PURLPARTS partsUrl, PURLPARTS partsBase, PURLPARTS partsOut)
{
    ASSERT(partsUrl && partsBase && partsOut);

    //
    //  if we have different hosts then everything but the pszAccess is DUMPED
    //
    if(partsUrl->pszServer)
    {
        partsOut->pszServer = partsUrl->pszServer;
        // NOTUSED partsOut->dwFlags |= (partsUrl->dwFlags & UPF_SERVER_MASK);

        if ((partsBase->pszServer && StrCmpW(partsUrl->pszServer, partsBase->pszServer)))
        {
            //  they are different Servers.  DUMP partsBase.

            ZeroMemory(partsBase, SIZEOF(URLPARTS));
        }
    }
    else
    {
        partsOut->pszServer = partsBase->pszServer;
        // NOTUSED partsOut->dwFlags |= (partsBase->dwFlags & UPF_SERVER_MASK);
    }
}

PRIVATE VOID
BlendPath(PURLPARTS partsUrl, PURLPARTS partsBase, PURLPARTS partsOut)
{
    ASSERT(partsUrl && partsBase && partsOut);

    if (partsUrl->dwFlags & UPF_SEG_ABSOLUTE)
    {
        if((partsBase->dwFlags & UPF_SEG_LOCKFIRST) &&
            !(partsUrl->dwFlags & UPF_SEG_LOCKFIRST))
        {
            // this keeps the drive letters when necessary
            partsOut->pszSegments = partsBase->pszSegments;
            partsOut->cSegments = 1;  // only keep the first segment
            partsOut->dwFlags |= (partsBase->dwFlags & UPF_SEG_MASK) ;

            partsOut->pszExtraSegs = partsUrl->pszSegments;
            partsOut->cExtraSegs = partsUrl->cSegments;
            partsOut->dwFlags |= (partsUrl->dwFlags & UPF_EXSEG_MASK);
        }
        else
        {


            //  just use the absolute path

            partsOut->pszSegments = partsUrl->pszSegments;
            partsOut->cSegments = partsUrl->cSegments;
            partsOut->dwFlags |= (partsUrl->dwFlags & (UPF_SEG_MASK |UPF_EXSEG_MASK) );
        }

        ZeroMemory(partsBase, SIZEOF(URLPARTS));

    }
    else if ((partsBase->dwFlags & UPF_SEG_ABSOLUTE))
    {
        //  Adopt path not name
        partsOut->pszSegments = partsBase->pszSegments;
        partsOut->cSegments = partsBase->cSegments;
        partsOut->dwFlags |= (partsBase->dwFlags & UPF_SEG_MASK );

        if(partsUrl->cSegments || partsUrl->dwFlags & UPF_SEG_EMPTYSEG)
        {
            //
            // this a relative path that needs to be combined
            //

            partsOut->pszExtraSegs = partsUrl->pszSegments;
            partsOut->cExtraSegs = partsUrl->cSegments;
            partsOut->dwFlags |= (partsUrl->dwFlags & UPF_EXSEG_MASK );

            if (!(partsBase->dwFlags & UPF_EXSEG_DIRECTORY))
            {
                //
                //  knock off the file name segment
                //  as long as the it isnt the first or the first is not locked
                //  or it isnt a dotdot.  in the case of http://site/dir/, dir/ is
                //  not actually killed, only the NULL terminator following it is.
                //
                LPWSTR pszLast = LastLiveSegment(partsOut->pszSegments, partsOut->cSegments, partsOut->dwFlags & UPF_SEG_LOCKFIRST);

                if(pszLast && !IsDotDot(pszLast))
                {
                    if(partsUrl->dwFlags & UPF_SEG_EMPTYSEG)
                        partsOut->dwFlags |= UPF_EXSEG_DIRECTORY;

                    KILLSEG(pszLast);
                }
            }
        }
        else
            partsOut->dwFlags |= (partsBase->dwFlags & UPF_EXSEG_MASK);
    }
    else if (partsUrl->cSegments)
    {
        partsOut->pszSegments = partsUrl->pszSegments;
        partsOut->cSegments = partsUrl->cSegments;
        partsOut->dwFlags |= (partsUrl->dwFlags & (UPF_SEG_MASK |UPF_EXSEG_MASK) );
    }
    else if (partsBase->cSegments)
    {
        partsOut->pszSegments = partsBase->pszSegments;
        partsOut->cSegments = partsBase->cSegments;
        partsOut->dwFlags |= (partsBase->dwFlags & (UPF_SEG_MASK |UPF_EXSEG_MASK) );

    }

    //  regardless, we want to zero if we have relative segs
    if (partsUrl->cSegments)
        ZeroMemory(partsBase, SIZEOF(URLPARTS));

}

PRIVATE VOID
BlendQuery(PURLPARTS partsUrl, PURLPARTS partsBase, PURLPARTS partsOut)
{
    if(partsUrl->pszQuery)
    {
        LPCWSTR pszQuery = partsOut->pszQuery = partsUrl->pszQuery;

        // NOTUSED partsOut->dwFlags |= (partsUrl->dwFlags & UPF_Query_MASK);

        if ((partsBase->pszQuery && StrCmpW(pszQuery, partsBase->pszQuery)))
        {
            //  they are different Querys.  DUMP partsBase.

            ZeroMemory(partsBase, SIZEOF(URLPARTS));
        }
    }
    else
    {
        partsOut->pszQuery = partsBase->pszQuery;
        // NOTUSED partsOut->dwFlags |= (partsBase->dwFlags & UPF_Query_MASK);
    }
}

PRIVATE VOID
BlendFragment(PURLPARTS partsUrl, PURLPARTS partsBase, PURLPARTS partsOut)
{
    if(partsUrl->pszFragment || partsUrl->cSegments)
    {
        LPCWSTR pszFragment = partsOut->pszFragment = partsUrl->pszFragment;

        // NOTUSED partsOut->dwFlags |= (partsUrl->dwFlags & UPF_Fragment_MASK);

        if ((partsBase->pszFragment && StrCmpW(pszFragment, partsBase->pszFragment)))
        {
            //  they are different Fragments.  DUMP partsBase.

            ZeroMemory(partsBase, SIZEOF(URLPARTS));
        }
    }
    else
    {
        partsOut->pszFragment = partsBase->pszFragment;
        // NOTUSED partsOut->dwFlags |= (partsBase->dwFlags & UPF_Fragment_MASK);
    }
}

PRIVATE VOID
BlendParts(PURLPARTS partsUrl, PURLPARTS partsBase, PURLPARTS partsOut)
{
    //
    //  partsUrl always takes priority over partsBase
    //

    ASSERT(partsUrl && partsBase && partsOut);

    ZeroMemory(partsOut, SIZEOF(URLPARTS));

    BlendScheme( partsUrl,  partsBase,  partsOut);
    BlendServer( partsUrl,  partsBase,  partsOut);
    BlendPath( partsUrl,  partsBase,  partsOut);
    BlendQuery( partsUrl,  partsBase,  partsOut);
    BlendFragment( partsUrl,  partsBase,  partsOut);

}

PRIVATE VOID
CanonServer(PURLPARTS parts)
{
    //
    //  we only do stuff if this server is an internet style
    //  server.  that way it uses FQDNs and IP port numbers
    //
    if (parts->pszServer && (parts->dwFlags & UPF_SCHEME_INTERNET))
    {

        LPWSTR pszName = StrRChrW(parts->pszServer, NULL, L'@');

        if(!pszName)
            pszName = parts->pszServer;

        //  this should just point to the FQDN:Port
        CharLowerW(pszName);

        //
        //  Ignore default port numbers, and trailing dots on FQDNs
        //  which will only cause identical adresses to look different
        //
        {
            WCHAR *pch = StrChrW(pszName, COLON);

            if (pch && parts->eScheme)
            {
                BOOL fIgnorePort = FALSE;

                //
                //  BUGBUG we should actually be getting this from
                //  the services file to find out the default protocol port
                //  but we dont think that most people will change them - zekel 17-Dec-96
                //
                switch(parts->eScheme)
                {
                case URL_SCHEME_HTTP:
                        if(StrCmpW(pch, L":80") == 0)
                            fIgnorePort = TRUE;
                        break;

                case URL_SCHEME_FTP:
                        if(StrCmpW(pch, L":21") == 0)
                            fIgnorePort = TRUE;
                        break;

                case URL_SCHEME_GOPHER:
                        if(StrCmpW(pch, L":70") == 0)
                            fIgnorePort = TRUE;
                        break;

                case URL_SCHEME_HTTPS:
                        if(StrCmpW(pch, L":443") == 0)
                            fIgnorePort = TRUE;
                        break;

                default:
                    break;
                }
                if(fIgnorePort)
                    TERMSTR(pch);  // It is the default: ignore it
            }

        }
    }
}


PRIVATE VOID
CanonCombineSegs(PURLPARTS parts)
{
    ASSERT(parts);
    ASSERT(parts->pszExtraSegs && parts->cExtraSegs);

    LPWSTR pszLast = LastLiveSegment(parts->pszSegments, parts->cSegments, parts->dwFlags & UPF_SEG_LOCKFIRST);

    LPWSTR pszExtra = parts->pszExtraSegs;
    DWORD iExtra = 0;
    DWORD cExtras = parts->cExtraSegs;

    if(!IsLiveSegment(pszExtra))
        pszExtra = NextLiveSegment(pszExtra, &iExtra, cExtras);

    while(pszExtra && IsDotDot(pszExtra))
    {
        if (pszLast)
            KILLSEG(pszLast);

        KILLSEG(pszExtra);

        pszLast = LastLiveSegment(parts->pszSegments, parts->cSegments, parts->dwFlags & UPF_SEG_LOCKFIRST);
        pszExtra = NextLiveSegment(pszExtra, &iExtra, cExtras);
    }
}

PRIVATE VOID
CanonSegments(LPWSTR pszSeg,
              DWORD cSegs,
              BOOL fLockFirst)

{
    DWORD  iSeg = 0;
    LPWSTR pszLastSeg = NULL;
    LPWSTR pszFirstSeg = pszSeg;
    BOOL fLastIsFirst = TRUE;
    BOOL fFirstSeg = TRUE;

    ASSERT (pszSeg && cSegs);

    pszSeg = FirstLiveSegment(pszSeg, &iSeg, cSegs);

    while (pszSeg)
    {
        if(IsDot(pszSeg))
        {
            //  if it is just a "." we can discard the segment
            KILLSEG(pszSeg);
        }

        else if(IsDotDot(pszSeg))
        {
            //  if it is ".." then we discard it and the last seg

            //
            //  if we are at the first (root) or
            //  the last is the root and it is locked
            //  then we dont want to do anything
            //
            if(pszLastSeg && !IsDotDot(pszLastSeg) && !(fLastIsFirst && fLockFirst))
            {
                KILLSEG(pszLastSeg);
                pszLastSeg = NULL;
                KILLSEG(pszSeg);
            }
        }

        if(IsLiveSegment(pszSeg))
        {
            if(!pszLastSeg && fFirstSeg)
                fLastIsFirst = TRUE;
            else
                fLastIsFirst = FALSE;

            pszLastSeg = pszSeg;
            fFirstSeg = FALSE;
        }
        else
        {
            pszLastSeg = LastLiveSegment(pszFirstSeg, iSeg, fLockFirst);
        }

        pszSeg = NextLiveSegment(pszSeg, &iSeg, cSegs);

    }
}

PRIVATE VOID
CanonPath(PURLPARTS parts)
{

    ASSERT(parts);

    if(parts->cSegments)
        CanonSegments(parts->pszSegments, parts->cSegments, (parts->dwFlags & UPF_SEG_LOCKFIRST));

    if(parts->cExtraSegs)
        CanonSegments(parts->pszExtraSegs, parts->cExtraSegs, FALSE);

    if(parts->cExtraSegs)
        CanonCombineSegs(parts);
}


PRIVATE VOID
CanonParts(PURLPARTS parts)
{
    ASSERT(parts);

    //CanonScheme(parts);
    CanonServer(parts);
    CanonPath(parts);
    //CanonQuery(parts);
    //CanonFragment(parts);
}

PRIVATE HRESULT
BuildScheme(PURLPARTS parts, DWORD dwFlags, PSHSTRW pstr)
{
    HRESULT hr = S_OK;

    ASSERT(parts && pstr);

    if(parts->pszScheme)
    {
        hr = pstr->Append(parts->pszScheme);
        if(SUCCEEDED(hr))
            hr = pstr->Append(COLON);
    }

    return hr;

}

PRIVATE HRESULT
BuildServer(PURLPARTS parts, DWORD dwFlags, PSHSTRW pstr)
{
    HRESULT hr = S_OK;

    ASSERT(parts && pstr);

    switch(parts->eScheme)
    {
    case URL_SCHEME_MK:
    // CraigC's "mk:" has no // but acts like it does
        break;

    case URL_SCHEME_FILE:
        if ((dwFlags & URL_WININET_COMPATIBILITY) || (dwFlags & URL_FILE_USE_PATHURL))
        {
            if(parts->pszServer && *parts->pszServer)
                hr = pstr->Append(L"////");
            else if (parts->pszSegments && IsDrive(parts->pszSegments))
                hr = pstr->Append(SLASH);
            else if (parts->dwFlags & UPF_SEG_ABSOLUTE)
                hr = pstr->Append(L"//");
        }
        else if (parts->dwFlags & UPF_SEG_ABSOLUTE)
            hr = pstr->Append(L"//");
        break;

    default:
        if(parts->pszServer && SUCCEEDED(hr))
            hr = pstr->Append(L"//");
        break;
    }

    if(parts->pszServer && SUCCEEDED(hr))
            hr = pstr->Append(parts->pszServer);

    return hr;
}

PRIVATE HRESULT
BuildSegments(LPWSTR pszSeg, DWORD cSegs, PSHSTRW pstr, BOOL fRoot, BOOL *pfSlashLast)
{
    DWORD iSeg = 0;
    HRESULT hr = S_FALSE;

    *pfSlashLast = FALSE;

    ASSERT(pszSeg && pstr);

    pszSeg = FirstLiveSegment(pszSeg, &iSeg, cSegs);

    if(!fRoot && pszSeg)
    {
        hr = pstr->Append(pszSeg);

        if(SUCCEEDED(hr))
            pszSeg = NextLiveSegment(pszSeg, &iSeg, cSegs);
        else
            pszSeg = NULL;
    }

    while (pszSeg)
    {
        hr = pstr->Append(SLASH);
        if(SUCCEEDED(hr) && *pszSeg)
        {
            hr = pstr->Append(pszSeg);
            *pfSlashLast = FALSE;
        }
        else
            *pfSlashLast = TRUE;

        if(SUCCEEDED(hr))
            pszSeg = NextLiveSegment(pszSeg, &iSeg, cSegs);
        else
            break;
    }

    return hr;

}


PRIVATE HRESULT
BuildPath(PURLPARTS parts, DWORD dwFlags, PSHSTRW pstr)
{
    HRESULT hr = S_OK;
    BOOL fSlashLast = FALSE;
    DWORD iSeg;
    LPWSTR pszSegFirst = NULL;

    ASSERT(parts && pstr);

    if(parts->cSegments)
    {
        hr = BuildSegments(parts->pszSegments, parts->cSegments, pstr, parts->dwFlags & UPF_SEG_ABSOLUTE, &fSlashLast);

        if (fSlashLast)
            pstr->Append(SLASH);


    }

    if(SUCCEEDED(hr) && parts->cExtraSegs)
    {
        BOOL f = fSlashLast;

        hr = BuildSegments(parts->pszExtraSegs, parts->cExtraSegs, pstr, !fSlashLast, &fSlashLast);

        if (fSlashLast)
            pstr->Append(SLASH);

        if (hr == S_FALSE)
            fSlashLast = f;

    }

    //  trailing slash on a server name for IIS
    if( !fSlashLast &&
        (
          (parts->dwFlags & UPF_EXSEG_DIRECTORY) ||
          //  if this is just a server name by itself
          (!FirstLiveSegment(parts->pszSegments, &iSeg, parts->cSegments) &&
          !FirstLiveSegment(parts->pszExtraSegs, &iSeg, parts->cExtraSegs) &&
          parts->dwFlags & UPF_SEG_ABSOLUTE)
        )
      )
      {
        hr = pstr->Append(SLASH);
      }

    return hr;
}



PRIVATE HRESULT
BuildQuery(PURLPARTS parts, DWORD dwFlags, PSHSTRW pstr)
{
    HRESULT hr = S_OK;

    ASSERT(parts && pstr);

    if(parts->pszQuery)
    {
        hr = pstr->Append(QUERY);
        if(SUCCEEDED(hr))
            hr = pstr->Append(parts->pszQuery);
    }

    return hr;

}

PRIVATE HRESULT
BuildFragment(PURLPARTS parts, DWORD dwFlags, PSHSTRW pstr)
{
    HRESULT hr = S_OK;

    ASSERT(parts && pstr);

    if(parts->pszFragment)
    {
        hr = pstr->Append(POUND);
        if(SUCCEEDED(hr))
            hr = pstr->Append(parts->pszFragment);
    }

    return hr;

}

PRIVATE HRESULT
BuildUrl(PURLPARTS parts, DWORD dwFlags, PSHSTRW pstr)
{
    HRESULT hr;

    ASSERT(parts && pstr);

    if(
        (SUCCEEDED(hr = BuildScheme(parts, dwFlags, pstr)))      &&
        (SUCCEEDED(hr = BuildServer(parts, dwFlags, pstr)))      &&
        (SUCCEEDED(hr = BuildPath(parts, dwFlags, pstr)))        &&
        (SUCCEEDED(hr = BuildQuery(parts, dwFlags, pstr)))
        )
        hr = BuildFragment(parts, dwFlags, pstr);

    return hr;
}

/*+++

  SHUrlEscape()
    Escapes an URL
    right now, i am only escaping stuff in the Path part of the URL

  Parameters
  IN -
    pszUrl      URL to examine
    pstrOut     SHSTR destination
    dwFlags     the relevant URL_* flags,

  Returns
  HRESULT -
    SUCCESS     S_OK
    ERROR       only E_OUTOFMEMORY


  Helper Routines
    Escape*(part)           each part gets its own escape routine (ie EscapeScheme)
    EscapeSpaces            will only escape spaces (WININET compatibility mostly)
    EscapeSegmentsGetNeededSize     gets the required size of destination buffer for all path segments
    EscapeLiveSegment               does the work of escaping each path segment
---*/

PRIVATE HRESULT
EscapeSpaces(LPCWSTR psz, PSHSTRW pstr, DWORD dwFlags)
{
    HRESULT hr = S_OK;
    LPCWSTR pch;
    DWORD cSpaces = 0;


    ASSERT(psz && pstr);


    pstr->Reset();

    for (pch = psz; *pch; pch++)
    {
        if (*pch == SPC)
            cSpaces++;
    }

    if(cSpaces)
    {
        hr = pstr->SetSize(lstrlenW(psz) + cSpaces * 2 + 1);
        if(SUCCEEDED(hr))
        {
            LPWSTR pchOut = (LPWSTR) *pstr;

            for (pch = psz; *pch; pch++)
            {
                if ((*pch == POUND || *pch == QUERY) && (dwFlags & URL_DONT_ESCAPE_EXTRA_INFO))
                {
                    StrCpyW(pchOut, pch);
                    pchOut += lstrlenW(pchOut);
                    break;
                }

                if (*pch == SPC)
                {
                    *pchOut++ = HEX_ESCAPE;
                    *pchOut++ = L'2';
                    *pchOut++ = L'0';
                }
                else
                {
                    *pchOut++ = *pch;
                }
            }

            TERMSTR(pchOut);
        }

    }
    else
        hr = pstr->SetStr(psz);

    return hr;
}


inline PRIVATE HRESULT
EscapeScheme(PURLPARTS partsUrl, DWORD dwFlags, PURLPARTS partsOut, PSHSTRW pstr)
{
    ASSERT(partsUrl && partsOut);

    partsOut->pszScheme = partsUrl->pszScheme;
    partsOut->eScheme = partsUrl->eScheme;

    return S_OK;
}

inline PRIVATE HRESULT
EscapeServer(PURLPARTS partsUrl, DWORD dwFlags, PURLPARTS partsOut, PSHSTRW pstr)
{
    ASSERT(partsUrl && partsOut);

    partsOut->pszServer = partsUrl->pszServer;

    return S_OK;
}

inline PRIVATE HRESULT
EscapeQuery(PURLPARTS partsUrl, DWORD dwFlags, PURLPARTS partsOut, PSHSTRW pstr)
{
    ASSERT(partsUrl && partsOut);

    partsOut->pszQuery = partsUrl->pszQuery;

    return S_OK;
}

inline PRIVATE HRESULT
EscapeFragment(PURLPARTS partsUrl, DWORD dwFlags, PURLPARTS partsOut, PSHSTRW pstr)
{
    ASSERT(partsUrl && partsOut);

    partsOut->pszFragment = partsUrl->pszFragment;

    return S_OK;
}

PRIVATE BOOL
GetEscapeStringSize(LPWSTR psz, DWORD dwFlags, LPDWORD pcch)

{
    BOOL fResize = FALSE;
    ASSERT(psz);
    ASSERT(pcch);


    for (*pcch = 0; *psz; psz++)
    {
        (*pcch)++;

        if(!IsSafePathChar(*psz) ||
            ((dwFlags & URL_ESCAPE_PERCENT) && (*psz == HEX_ESCAPE)))
        {
            fResize = TRUE;
            *pcch += 2;
        }

    }

    // for the NULL term
    (*pcch)++;

    return fResize;
}

PRIVATE DWORD
EscapeSegmentsGetNeededSize(LPWSTR pszSegments, DWORD cSegs, DWORD dwFlags)
{
    DWORD cchNeeded = 0;
    BOOL fResize = FALSE;
    LPWSTR pszSeg;
    DWORD iSeg;

    ASSERT(pszSegments && cSegs);

    pszSeg = FirstLiveSegment(pszSegments, &iSeg, cSegs);

    while (IsLiveSegment(pszSeg))
    {
        DWORD cch;

        if(GetEscapeStringSize(pszSeg, dwFlags, &cch))
            fResize = TRUE;
        cchNeeded += cch;

        pszSeg = NextLiveSegment(pszSeg, &iSeg, cSegs);
    }

    return fResize ? cchNeeded : 0;
}

PRIVATE VOID
EscapeString(LPCWSTR pszSeg, DWORD dwFlags, LPWSTR *ppchOut)
{
    LPWSTR pchIn;   // This pointer has been trusted to not modify it's contents, just iterate.
    LPWSTR pchOut = *ppchOut;
    WCHAR ch;

    for (pchIn = (LPWSTR)pszSeg; *pchIn; pchIn++)
    {
        ch = *pchIn;

        if (!IsSafePathChar(ch) ||
            ((dwFlags & URL_ESCAPE_PERCENT) && (ch == HEX_ESCAPE)))
        {
            *pchOut++ = HEX_ESCAPE;
            *pchOut++ = hex[(ch >> 4) & 15];
            *pchOut++ = hex[ch & 15];

        }
        else
            *pchOut++ = *pchIn;
    }

    TERMSTR(pchOut);

    // move past the terminator
    pchOut++;

    *ppchOut = pchOut;

}

PRIVATE HRESULT
EscapeSegments(LPWSTR pszSegments, DWORD cSegs, DWORD dwFlags, PURLPARTS partsOut, PSHSTRW pstr)
{
    DWORD cchNeeded;

    HRESULT hr = S_OK;

    ASSERT(pszSegments && cSegs && partsOut && pstr);

    cchNeeded = EscapeSegmentsGetNeededSize(pszSegments, cSegs, dwFlags);

    if(cchNeeded)
    {
        ASSERT(pstr);

        hr = pstr->SetSize(cchNeeded);

        if(SUCCEEDED(hr))
        {
            LPWSTR pchOut = (LPWSTR) *pstr;
            LPWSTR pszSeg;
            DWORD iSeg;

            partsOut->pszSegments = pchOut;
            partsOut->cSegments = 0;

            pszSeg = FirstLiveSegment(pszSegments, &iSeg, cSegs);

            while (IsLiveSegment(pszSeg))
            {
                EscapeString(pszSeg, dwFlags, &pchOut);
                partsOut->cSegments++;

                pszSeg = NextLiveSegment(pszSeg, &iSeg, cSegs);
            }


        }

    }
    else
    {
        partsOut->cSegments = cSegs;
        partsOut->pszSegments = pszSegments;
    }


    return hr;
}

PRIVATE HRESULT
EscapePath(PURLPARTS partsUrl, DWORD dwFlags, PURLPARTS partsOut, PSHSTRW pstr)
{
    HRESULT hr = S_OK;

    ASSERT(partsUrl && partsOut && pstr);

    if(partsUrl->cSegments)
    {
        hr = EscapeSegments(partsUrl->pszSegments, partsUrl->cSegments, dwFlags, partsOut, pstr);

    }
    else
    {
        partsOut->cSegments = 0;
        partsOut->pszSegments = NULL;
    }

    return hr;
}

HRESULT
SHUrlEscape (LPCWSTR pszUrl,
             PSHSTRW pstrOut,
             DWORD dwFlags)
{
#ifdef TESTING_SPACES_ONLY
    return EscapeSpaces(pszUrl, pstrOut, dwFlags);
#else //TESTING_SPACES_ONLY

    SHSTRW strUrl;
    HRESULT hr;

    ASSERT(pszUrl && pstrOut);
    if(!pszUrl || !pstrOut)
        return E_INVALIDARG;

    //
    //  EscapeSpaces is remarkably stupid,
    //  but so is this kind of functionality...
    //  it doesnt do any kind of real parsing, it
    //  only looks for spaces and escapes them...
    //
    if(dwFlags & URL_ESCAPE_SPACES_ONLY)
        return EscapeSpaces(pszUrl, pstrOut, dwFlags);

    //  We are just passed a segment so we only want to
    //  escape that and nothing else.  Don't look for
    //  URL pieces.
    if(dwFlags & URL_ESCAPE_SEGMENT_ONLY)
    {
        URLPARTS partsOut;
        SHSTRW strTemp;

        EscapeSegments((LPWSTR)pszUrl, 1, dwFlags, &partsOut, &strTemp);
        pstrOut->SetStr(partsOut.pszSegments);
        return S_OK;
    }

    pstrOut->Reset();

    hr = strUrl.SetStr(pszUrl);

    if(SUCCEEDED(hr))
    {
        URLPARTS partsUrl, partsOut;
        SHSTRW strPath;

        BreakUrl(strUrl, &partsUrl);

        ZeroMemory(&partsOut, SIZEOF(URLPARTS));
        //
        //  NOTE the only function here that is really active right now is the EscapePath
        //  if some other part needs to be escaped, then add a new SHSTR in the 4th param
        //  and change the appropriate subroutine
        //

        if(
            (SUCCEEDED(hr = EscapeScheme(&partsUrl, dwFlags, &partsOut, NULL)))
            && (SUCCEEDED(hr = EscapeServer(&partsUrl, dwFlags, &partsOut, NULL)))
            && (SUCCEEDED(hr = EscapePath(&partsUrl, dwFlags, &partsOut, &strPath)))
            && (SUCCEEDED(hr = EscapeQuery(&partsUrl, dwFlags, &partsOut, NULL)))
            && (SUCCEEDED(hr = EscapeFragment(&partsUrl, dwFlags, &partsOut, NULL)))
           )
        {
            partsOut.dwFlags = partsUrl.dwFlags;

            hr = BuildUrl(&partsOut, dwFlags, pstrOut);
        }
    }
    else
        hr = E_OUTOFMEMORY;

    return hr;
#endif //TESTING_SPACES_ONLY
}


/*+++

  SHUrlUnescape()
    Unescapes a string in place.  this is ok because
    it should never grow

  Parameters
  IN -
    psz         string to unescape inplace
    dwFlags     the relevant URL_* flags,

  Returns
  HRESULT -
    SUCCESS     S_OK
    ERROR       DOESNT error right now


  Helper Routines
    HexToWord               takes a hexdigit and returns WORD with the right number or -1
    IsEscapedChar           looks at a ptr for "%XX" where X is a hexdigit
    TranslateEscapedChar    translates "%XX" to an 8 bit char
---*/

PRIVATE WORD
HexToWord(WCHAR ch)
{
    if(ch >= TEXT('0') && ch <= TEXT('9'))
        return (WORD) ch - TEXT('0');
    if(ch >= TEXT('A') && ch <= TEXT('F'))
        return (WORD) ch - TEXT('A') + 10;
    if(ch >= TEXT('a') && ch <= TEXT('f'))
        return (WORD) ch - TEXT('a') + 10;

    ASSERT(FALSE);  //we have tried to use a non-hex number
    return (WORD) -1;
}

PRIVATE BOOL inline
IsEscapedOctetW(LPCWSTR pch)
{
    return (pch[0] == HEX_ESCAPE && IsHex(pch[1]) && IsHex(pch[2])) ? TRUE : FALSE;
}

PRIVATE BOOL inline
IsEscapedOctetA(LPCSTR pch)
{
    return (pch[0] == HEX_ESCAPE_A && IsHex((WCHAR)pch[1]) && IsHex((WCHAR)pch[2])) ? TRUE : FALSE;
}

PRIVATE WCHAR
TranslateEscapedOctetW(LPCWSTR pch)
{
    WCHAR ch;
    ASSERT(IsEscapedOctetW(pch));

    pch++;
    ch = (WCHAR) HexToWord(*pch++) * 16; // hi nibble
    ch += HexToWord(*pch); // lo nibble

    return ch;
}

PRIVATE CHAR
TranslateEscapedOctetA(LPCSTR pch)
{
    CHAR ch;
    ASSERT(IsEscapedOctetA(pch));

    pch++;
    ch = (CHAR) HexToWord(*pch++) * 16; // hi nibble
    ch += HexToWord(*pch); // lo nibble

    return ch;
}


HRESULT SHUrlUnescapeA(LPSTR psz, DWORD dwFlags)
{
    CHAR *pchSrc = psz;
    CHAR *pchDst = psz;

    while (*pchSrc)
    {
        if ((*pchSrc == POUND || *pchSrc == QUERY) && (dwFlags & URL_DONT_ESCAPE_EXTRA_INFO))
        {
            StrCpyA(pchDst, pchSrc);
            pchDst += lstrlenA(pchDst);
            break;
        }

        if (IsEscapedOctetA(pchSrc))
        {
            CHAR ch =  TranslateEscapedOctetA(pchSrc);

            *pchDst++ = ch;

            pchSrc += 3; // enuff for "%XX"
        }
        else
        {
            *pchDst++ = *pchSrc++;
        }
    }

    TERMSTR(pchDst);

    return S_OK;
}


HRESULT SHUrlUnescapeW(LPWSTR psz, DWORD dwFlags)
{
    WCHAR *pchSrc = psz;
    WCHAR *pchDst = psz;

    while (*pchSrc)
    {
        if ((*pchSrc == POUND || *pchSrc == QUERY) && (dwFlags & URL_DONT_ESCAPE_EXTRA_INFO))
        {
            StrCpyW(pchDst, pchSrc);
            pchDst += lstrlenW(pchDst);
            break;
        }

        if (IsEscapedOctetW(pchSrc))
        {
            WCHAR ch =  TranslateEscapedOctetW(pchSrc);

            *pchDst++ = ch;

            pchSrc += 3; // enuff for "%XX"
        }
        else
        {
            *pchDst++ = *pchSrc++;
        }
    }

    TERMSTR(pchDst);

    return S_OK;

}

PRIVATE HRESULT
BuildDosPath(PURLPARTS parts, PSHSTRW pstrOut, DWORD dwFlags)
{
    HRESULT hr;
    //  this will disable a preceding slash when there is a drive
    if(parts->pszSegments && IsDrive(parts->pszSegments))
        parts->dwFlags = (parts->dwFlags & ~UPF_SEG_ABSOLUTE);


    //  if there is a zero length server then
    //  we skip building it
    if(parts->pszServer && !*parts->pszServer)
        parts->pszServer = NULL;


    //  this prevents all the special file goo checking
    parts->eScheme = URL_SCHEME_UNKNOWN;

    //
    //  then go ahead and put the path together
    if( (SUCCEEDED(hr = BuildServer(parts, dwFlags, pstrOut))) &&
        (!parts->cSegments || SUCCEEDED(hr = BuildPath(parts, dwFlags, pstrOut)))
      )
    {
        //  then decode it cuz paths arent escaped
        if (IsFlagSet(dwFlags, URL_FILE_USE_PATHURL))
            WininetFixFileSlashes((LPWSTR)*pstrOut);
        else
#ifndef UNIX
            ConvertChar((LPWSTR)*pstrOut, SLASH, WHACK, TRUE);
#else
            ConvertChar((LPWSTR)*pstrOut, WHACK, SLASH, TRUE);
#endif

        if(IsFlagClear(parts->dwFlags, UPF_FILEISPATHURL))
            SHUrlUnescapeW((LPWSTR)*pstrOut, dwFlags);

        if(IsDriveUrl((LPWSTR)*pstrOut))
            ((LPWSTR)*pstrOut)[1] = COLON;
    }

    return hr;

}
HRESULT
SHPathCreateFromUrl(LPCWSTR pszUrl, PSHSTRW pstrOut, DWORD dwFlags)
{
    HRESULT hr;
    SHSTRW strUrl;

    ASSERT(pszUrl && pstrOut);

    pstrOut->Reset();
    hr = strUrl.SetStr(pszUrl);

    if(SUCCEEDED(hr))
    {
        URLPARTS partsUrl;

        //  first we need to break it open
        BreakUrl((LPWSTR) strUrl, &partsUrl);

        //  then we make sure it is a file:
        if(partsUrl.eScheme == URL_SCHEME_FILE)
        {
            hr = BuildDosPath(&partsUrl, pstrOut, dwFlags);
        }
        else
            hr = E_INVALIDARG;
    }
    return hr;
}


HRESULT
SHUrlCreateFromPath(LPCWSTR pszPath, PSHSTRW pstrOut, DWORD dwFlags)
{
        HRESULT hr;
        SHSTRW strPath;
        ASSERT(pszPath && pstrOut);

        if(PathIsURLW(pszPath))
        {
            if(SUCCEEDED(hr = pstrOut->SetStr(pszPath)))
                return S_FALSE;
            else
                return hr;
        }


        pstrOut->Reset();
        hr = strPath.SetStr(pszPath);

        TrimAndStripInsignificantWhite((LPWSTR)strPath);

        if(SUCCEEDED(hr))
        {
            URLPARTS partsIn, partsOut;
            SHSTRW strEscapedPath, strEscapedServer;
            LPWSTR pch = (LPWSTR)strPath;


            ZeroMemory(&partsIn, SIZEOF(URLPARTS));

            partsIn.pszScheme = (LPWSTR)c_szFileScheme;
            partsIn.eScheme = URL_SCHEME_FILE;
            partsIn.dwFlags = UPF_SCHEME_CONVERT;

            //  first break the path
            BreakFragment(&pch, &partsIn);
            BreakServer(&pch, &partsIn, TRUE);
            BreakPath(&pch, &partsIn);

            partsOut = partsIn;

            //  then escape the path if we arent using path URLs
            if (IsFlagClear(dwFlags, URL_FILE_USE_PATHURL))
            {
                hr = EscapePath(&partsIn, dwFlags | URL_ESCAPE_PERCENT, &partsOut, &strEscapedPath);

                if(SUCCEEDED(hr) && partsOut.pszServer)
                {
                    //
                    //  i am treating the pszServer exactly like a path segment
                    //

                    DWORD cchNeeded;

                    if(GetEscapeStringSize(partsOut.pszServer, dwFlags | URL_ESCAPE_PERCENT, &cchNeeded) &&
                        SUCCEEDED(hr = strEscapedServer.SetSize(cchNeeded)))
                    {
                        pch = (LPWSTR) strEscapedServer;

                        EscapeString(partsOut.pszServer, dwFlags | URL_ESCAPE_PERCENT, &pch);
                        partsOut.pszServer = (LPWSTR) strEscapedServer;
                    }
                }
            }

            if(!partsOut.pszServer && IsFlagSet(partsOut.dwFlags, UPF_SEG_ABSOLUTE))
                partsOut.pszServer = L"";

            //  then build the URL
            if(SUCCEEDED(hr))
            {
                if(URL_SCHEME_FILE == partsOut.eScheme && IsFlagSet(dwFlags, URL_FILE_USE_PATHURL))
                {
                    if (SUCCEEDED(hr = pstrOut->SetStr(c_szFileSchemeString)))
                        hr = BuildDosPath(&partsOut, pstrOut, dwFlags);
                }
                else
                    hr = BuildUrl(&partsOut, dwFlags, pstrOut);
            }

            if (SUCCEEDED(hr) && (IsFlagSet(dwFlags, URL_WININET_COMPATIBILITY)))
                WininetFixFileSlashes((LPWSTR) *pstrOut);
        }

        return hr;
}


/*+++

  SHUrlParse()
    Canonicalize an URL
    or Combine and Canonicalize two URLs

  Parameters
  IN -
    pszBase     the base or referring URL, may be NULL
    pszUrl      the relative URL
    dwFlags     the relevant URL_* flags,

  Returns
  HRESULT -
    SUCCESS     S_OK
    ERROR       appropriate error, usually just E_OUTOFMEMORY;

  NOTE:  pszUrl will always take precedence over pszBase.

---*/
HRESULT SHUrlParse(LPCWSTR pszBase, LPCWSTR pszUrl, PSHSTRW pstrOut, DWORD dwFlags)
{
    HRESULT hr = S_OK;
    URLPARTS partsUrl, partsOut, partsBase;

    SHSTRW strBase;
    SHSTRW strUrl;
    ASSERT(pszUrl);
    ASSERT(pstrOut);

    TraceMsgW(TF_URL | TF_FUNC, "entering SHUrlParse(%s, %s, 0x%X", pszBase,pszUrl ? pszUrl : L"NULL", dwFlags);

    pstrOut->Reset();

    //
    // Don't bother parsing if all we have in an inter-page link as the
    // pszUrl and no pszBase to parse
    //

    if (pszUrl[0] == POUND && (!pszBase || !*pszBase))
    {
        hr = pstrOut->SetStr(pszUrl);

        goto quit;
    }


    //
    //  for Perf reasons we want to parse the relative url first.
    //  if it is an absolute URL, we need never look at the base.
    //

    hr = CopyUrlForParse(pszUrl, &strUrl, dwFlags);

    if(FAILED(hr))
        goto quit;

    // -- Cybersitter compat ----
    // Some bug fix broke the original parser. No time to go back and
    // fix it, but since we know what to expect, we'll return this straight instead.
    // Basically, when we canonicalize ://, we produce :///
    if (!StrCmpW((PWSTR)strUrl, L"://"))
    {
        hr = pstrOut->SetStr(L":///");
        goto quit;
    }

    //
    //  BreakUrls will decide if it is necessary to look at the relative
    //
    hr = BreakUrls((LPWSTR) strUrl, &partsUrl, pszBase, &strBase, &partsBase, dwFlags);

    if(FAILED(hr))
        goto quit;

    if(S_OK == hr)    {
        //
        //  this is where the real combination logic happens
        //  this first parts is the one that takes precedence
        //
        BlendParts(&partsUrl, &partsBase, &partsOut);
    }
    else
        partsOut = partsUrl;


    //
    //  we will now do the work of putting it together
    //  if these fail, it is because we are out of memory.
    //

    if (!(dwFlags & URL_DONT_SIMPLIFY))
        CanonParts(&partsOut);

    if(URL_SCHEME_FILE == partsOut.eScheme && IsFlagSet(dwFlags, URL_FILE_USE_PATHURL))
    {
        if (SUCCEEDED(hr = pstrOut->SetStr(c_szFileSchemeString)))
            hr = BuildDosPath(&partsOut, pstrOut, dwFlags);
    }
    else
        hr = BuildUrl(&partsOut, dwFlags, pstrOut);


    if(SUCCEEDED(hr))
    {
        if (dwFlags & URL_UNESCAPE)
            SHUrlUnescapeW((LPWSTR) *pstrOut, dwFlags);

        if (dwFlags & URL_ESCAPE_SPACES_ONLY || dwFlags & URL_ESCAPE_UNSAFE)
        {
            //
            //  we are going to reuse strUrl here
            //
            hr = strUrl.SetStr(*pstrOut);

            if(SUCCEEDED(hr))
                hr = SHUrlEscape((LPWSTR)strUrl, pstrOut, dwFlags);
        }
    }

    if (SUCCEEDED(hr) &&
        (IsFlagSet(dwFlags, URL_WININET_COMPATIBILITY))  &&
        (partsOut.eScheme == URL_SCHEME_FILE))
        WininetFixFileSlashes((LPWSTR) *pstrOut);


quit:


    if(FAILED(hr))
    {
        pstrOut->Reset();
        TraceMsg(TF_URL | TF_FUNC, TEXT("FAILED SHUrlParse() hr = 0x%X\n"), hr);
    }
    else
        TraceMsgW(TF_URL | TF_FUNC, "SUCCEEDED SHUrlParse() %s\n", (LPWSTR)*pstrOut);


    return hr;

}

typedef struct _LOGON {
    LPWSTR pszUser;
    LPWSTR pszPass;
    LPWSTR pszHost;
    LPWSTR pszPort;
} LOGON, *PLOGON;

PRIVATE void
BreakLogon(LPWSTR psz, PLOGON plo)
{
    ASSERT(psz);
    ASSERT(plo);

    WCHAR *pch = StrChrW(psz, L'@');
    if(pch)
    {
        TERMSTR(pch);
        plo->pszHost = pch + 1;

        plo->pszUser = psz;
        pch = StrChrW(psz, COLON);
        if (pch)
        {
            TERMSTR(pch);
            plo->pszPass = pch + 1;
        }
    }
    else
        plo->pszHost = psz;

    pch = StrChrW(plo->pszHost, COLON);
    if (pch)
    {
        TERMSTR(pch);
        plo->pszPort = pch + 1;
    }
}

PRIVATE HRESULT
InternetGetPart(DWORD dwPart, PURLPARTS parts, PSHSTRW pstr, DWORD dwFlags)
{
    HRESULT hr = E_FAIL;

    if(parts->pszServer)
    {
        LOGON lo = {0};

        BreakLogon(parts->pszServer, &lo);

        switch (dwPart)
        {
        case URL_PART_HOSTNAME:
            hr = pstr->Append(lo.pszHost);
            break;

        case URL_PART_USERNAME:
            hr = pstr->Append(lo.pszUser);
            break;

        case URL_PART_PASSWORD:
            hr = pstr->Append(lo.pszPass);
            break;

        case URL_PART_PORT:
            hr = pstr->Append(lo.pszPort);
            break;

        default:
            ASSERT(FALSE);
        }
    }
    return hr;
}

PRIVATE HRESULT
SHUrlGetPart(PSHSTRW pstrIn, PSHSTRW pstrOut, DWORD dwPart, DWORD dwFlags)
{
    ASSERT(pstrIn);
    ASSERT(pstrOut);
    ASSERT(dwPart);

    HRESULT hr = S_OK;

    URLPARTS parts;

    BreakUrl((LPWSTR)*pstrIn, &parts);

    if(dwFlags & URL_PARTFLAG_KEEPSCHEME)
    {
        hr = pstrOut->SetStr(parts.pszScheme);
        if(SUCCEEDED(hr))
            hr = pstrOut->Append(COLON);
    }
    else
        pstrOut->Reset();

    if(SUCCEEDED(hr))
    {
        switch (dwPart)
        {
        case URL_PART_SCHEME:
            hr = pstrOut->SetStr(parts.pszScheme);
            break;

        case URL_PART_HOSTNAME:
            if (parts.eScheme == URL_SCHEME_FILE)
            {
                hr = pstrOut->SetStr(parts.pszServer);
                break;
            }
            // else fall through
        case URL_PART_USERNAME:
        case URL_PART_PASSWORD:
        case URL_PART_PORT:
            if(parts.dwFlags & UPF_SCHEME_INTERNET)
            {
                hr = InternetGetPart(dwPart, &parts, pstrOut, dwFlags);
            }
            else
                hr = E_FAIL;
            break;

        case URL_PART_QUERY:
            hr = pstrOut->SetStr(parts.pszQuery);
            break;

        default:
            ASSERT(FALSE);
            hr = E_UNEXPECTED;
        }
    }

    return hr;
}

#define c_szURLPrefixesKey "Software\\Microsoft\\Windows\\CurrentVersion\\URL\\Prefixes"
const WCHAR c_szDefaultURLPrefixKey[]   = L"Software\\Microsoft\\Windows\\CurrentVersion\\URL\\DefaultPrefix";

PRIVATE inline LPCWSTR SkipLeadingSlashes(LPCWSTR psz)
{
    // Skip two leading slashes.

    if (psz[0] == SLASH && psz[1] == SLASH)
        psz += 2;

    return psz;
}

PRIVATE HRESULT
UrlGuessScheme(LPCWSTR pszUrl, PSHSTRW pstr)
{
    HRESULT hr = S_FALSE;

    ASSERT(pszUrl && pstr);

    HKEY hkeyPrefixes;

    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, c_szURLPrefixesKey, 0, KEY_QUERY_VALUE, &hkeyPrefixes)
        == ERROR_SUCCESS)
    {
        DWORD dwiValue;
        CHAR rgchValueName[MAX_PATH];
        DWORD cchValueName = SIZECHARS(rgchValueName);
        DWORD dwType;
        CHAR rgchPrefix[MAX_PATH];
        DWORD cbPrefix = SIZEOF(rgchPrefix);

        //  need to get past the initial two slashes if applicable
        pszUrl = SkipLeadingSlashes(pszUrl);

        for (dwiValue = 0;
             RegEnumValueA(hkeyPrefixes, dwiValue, rgchValueName,
                          &cchValueName, NULL, &dwType, (PBYTE)rgchPrefix,
                          &cbPrefix) == ERROR_SUCCESS;
             dwiValue++)
        {
            WCHAR wszValue[MAX_PATH];

            MultiByteToWideChar(CP_ACP, 0, rgchValueName, -1, wszValue, ARRAYSIZE(wszValue));

            //  we check to make sure that we match and there is something more
            if (!StrCmpNIW(pszUrl, wszValue, cchValueName) && pszUrl[cchValueName])
            {
                MultiByteToWideChar(CP_ACP, 0, rgchPrefix, -1, wszValue, ARRAYSIZE(wszValue));
                if(SUCCEEDED(hr = pstr->SetStr(wszValue)))
                    hr = pstr->Append(pszUrl);
                break;
            }

            cchValueName = SIZECHARS(rgchValueName);
            cbPrefix = SIZEOF(rgchPrefix);
        }

        RegCloseKey(hkeyPrefixes);
    }

    return(hr);
}

/*----------------------------------------------------------
Purpose: Grabs the default URL prefix in the registry and applies
         it to the given URL.

Returns: S_OK
         S_FALSE if there is no default prefix

*/
const WCHAR c_szDefaultScheme[] = L"http://";

HRESULT
UrlApplyDefaultScheme(
    LPCWSTR pszUrl,
    PSHSTRW pstr)
{
    HRESULT hr = S_FALSE;
    WCHAR szDef[MAX_PATH];
    DWORD cbSize = SIZEOF(szDef);

    ASSERT(pszUrl && pstr);
    ASSERT(!PathIsURLW(pszUrl));

    DWORD dwType;
    if (NO_ERROR == SHRegGetUSValueW(c_szDefaultURLPrefixKey, NULL, &dwType, (LPVOID)szDef, &cbSize, TRUE, (LPVOID)c_szDefaultScheme, SIZEOF(c_szDefaultScheme)))
    {
        pszUrl = SkipLeadingSlashes(pszUrl);

        if(SUCCEEDED(hr = pstr->SetStr(szDef)))
            hr = pstr->Append(pszUrl);
    }

    return hr;
}

/*----------------------------------------------------------
Purpose: Guesses a URL protocol based upon a list in the registry,
         compared to the first few characters of the given
         URL suffix.

Returns: S_OK if a URL protocol is determined
         S_FALSE if there were no problems but no prefix was prepended

*/
HRESULT
SHUrlApplyScheme(
    LPCWSTR pszUrl,
    PSHSTRW pstrOut,
    DWORD dwFlags)
{
    HRESULT hr = S_FALSE;

    ASSERT(IS_VALID_STRING_PTRW(pszUrl, -1));

    //
    // if there is already scheme there, we do nothing
    //  unless the caller insists.  this is to support
    //  a string that looks like www.foo.com:8001.
    //  this is a site that needs to be guessed at but
    //  it also could be a valid scheme since '.' and '-'
    //  are both valid scheme chars.
    //
    DWORD cch;
    if((dwFlags & URL_APPLY_FORCEAPPLY) || !FindSchemeW(pszUrl, &cch))
    {
        if(dwFlags & URL_APPLY_GUESSSCHEME)
            hr = UrlGuessScheme(pszUrl, pstrOut);

        if (hr != S_OK && (dwFlags & URL_APPLY_GUESSFILE))
        {
            LPCWSTR psz = FindDosPath(pszUrl);

            //  only change hr if we actually converted.
            if(psz && SUCCEEDED(SHUrlCreateFromPath(psz, pstrOut, 0)))
                hr = S_OK;
        }

        if (hr != S_OK && (dwFlags & URL_APPLY_DEFAULT || !dwFlags))
            hr = UrlApplyDefaultScheme(pszUrl, pstrOut);
    }

    return hr;
}



PRIVATE HRESULT
CopyOutA(PSHSTRA pstr, LPSTR psz, LPDWORD pcch)
{
    HRESULT hr = S_OK;
    DWORD cch;
    ASSERT(pstr);
    ASSERT(psz);
    ASSERT(pcch);

    cch = pstr->GetLen();
    if((*pcch > cch) && psz)
        lstrcpyA(psz, pstr->GetStr());
    else
        hr = E_POINTER;

    *pcch = cch + (FAILED(hr) ? 1 : 0);

    return hr;
}

//***   StrCopyOutW --
// NOTES
//  WARNING: must match semantics of CopyOutW! (esp. the *pcchOut part)
PRIVATE HRESULT
StrCopyOutW(LPCWSTR pszIn, LPWSTR pszOut, LPDWORD pcchOut)
{
    DWORD cch;

    cch = lstrlenW(pszIn);
    if (cch < *pcchOut && pszOut) {
        *pcchOut = cch;
        StrCpyW(pszOut, pszIn);
        return S_OK;
    }
    else {
        *pcchOut = cch + 1;
        return E_POINTER;
    }
}

//***
// NOTES
//  WARNING: StrCopyOutW must match this func, so if you change this change
// it too
PRIVATE HRESULT
CopyOutW(PSHSTRW pstr, LPWSTR psz, LPDWORD pcch)
{
    HRESULT hr = S_OK;
    DWORD cch;
    ASSERT(pstr);
    ASSERT(psz);
    ASSERT(pcch);

    cch = pstr->GetLen();
    if((*pcch > cch) && psz)
        StrCpyW(psz, pstr->GetStr());
    else
        hr = E_POINTER;

    *pcch = cch + (FAILED(hr) ? 1 : 0);

    return hr;
}


LWSTDAPI
UrlCanonicalizeA(LPCSTR pszIn,
                LPSTR pszOut,
                LPDWORD pcchOut,
                DWORD dwFlags)
{
    HRESULT hr;
    SHSTRA straOut;

    if (!pszIn
        || !pszOut
        || !pcchOut
        || !*pcchOut)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        RIP(IS_VALID_STRING_PTR(pszIn, INTERNET_MAX_PATH_LENGTH));
        RIP(IS_VALID_WRITE_PTR(pcchOut, DWORD));
        RIP(IS_VALID_WRITE_BUFFER(pszOut, CHAR, *pcchOut));

        hr = UrlCombineA("", pszIn, pszOut, pcchOut, dwFlags);
    }
    return hr;

}


LWSTDAPI
UrlEscapeA(LPCSTR pszIn,
                LPSTR pszOut,
                LPDWORD pcchOut,
                DWORD dwFlags)
{
    HRESULT hr;
    SHSTRA straOut;

    if (!pszIn || !pszOut ||
        !pcchOut || !*pcchOut)
        hr = E_INVALIDARG;
    else
    {
        SHSTRW strwOut;
        SHSTRW strUrl;

        if(SUCCEEDED(strUrl.SetStr(pszIn)))
            hr = SHUrlEscape((LPWSTR) strUrl, &strwOut, dwFlags);
        else
            hr = E_OUTOFMEMORY;

        if(SUCCEEDED(hr))
            hr = ReconcileHresults(hr, straOut.SetStr(strwOut));
    }

    if(SUCCEEDED(hr))
        hr = ReconcileHresults(hr, CopyOutA(&straOut, pszOut, pcchOut));

    return hr;
}

LWSTDAPI
UrlGetPartA(LPCSTR pszIn, LPSTR pszOut, LPDWORD pcchOut, DWORD dwPart, DWORD dwFlags)
{
    HRESULT hr;
    SHSTRA straOut;

    if (!pszIn || !pszOut ||
        !pcchOut || !*pcchOut || dwPart == URL_PART_NONE)
        hr = E_INVALIDARG;
    else
    {
        SHSTRW strwOut;
        SHSTRW strwIn;

        if(SUCCEEDED(strwIn.SetStr(pszIn)))
            hr = SHUrlGetPart(&strwIn, &strwOut, dwPart, dwFlags);
        else
            hr = E_OUTOFMEMORY;

        if(SUCCEEDED(hr))
            hr = ReconcileHresults(hr, straOut.SetStr(strwOut));
    }

    if(SUCCEEDED(hr))
        hr = ReconcileHresults(hr, CopyOutA(&straOut, pszOut, pcchOut));

    return hr;

}

LWSTDAPI_(BOOL) UrlIsA(LPCSTR pszURL, URLIS UrlIs)
{
    BOOL fRet = FALSE;

    if(pszURL)
    {
        DWORD cchScheme, dwFlags;
        LPCSTR pszScheme = FindSchemeA(pszURL, &cchScheme);

        if(pszScheme)
        {
            URL_SCHEME eScheme = GetSchemeTypeAndFlagsA(pszScheme, cchScheme, &dwFlags);

            switch (UrlIs)
            {
            case URLIS_URL:
                fRet = TRUE;
                break;

            case URLIS_OPAQUE:
                fRet = (dwFlags & UPF_SCHEME_OPAQUE);
                break;

            case URLIS_NOHISTORY:
                fRet = (dwFlags & UPF_SCHEME_NOHISTORY);
                break;

            case URLIS_FILEURL:
                fRet = (eScheme == URL_SCHEME_FILE);
                break;

            default:
                //  if it cant be done quck and dirty
                //  then we need to thunk to the wide version
                SHSTRW strUrl;
                if (SUCCEEDED(strUrl.SetStr(pszURL)))
                {
                    fRet = UrlIsW(strUrl, UrlIs);
                }
            }
        }
    }
    return fRet;
}

LWSTDAPI_(BOOL) UrlIsW(LPCWSTR pszURL, URLIS UrlIs)
{
    BOOL fRet = FALSE;

    if(pszURL)
    {
        DWORD cchScheme, dwFlags;
        LPCWSTR pszScheme = FindSchemeW(pszURL, &cchScheme);

        if(pszScheme)
        {
            SHSTRW str;
            URL_SCHEME eScheme = GetSchemeTypeAndFlagsW(pszScheme, cchScheme, &dwFlags);

            switch (UrlIs)
            {
            case URLIS_URL:
                fRet = TRUE;
                break;

            case URLIS_OPAQUE:
                fRet = (dwFlags & UPF_SCHEME_OPAQUE);
                break;

            case URLIS_NOHISTORY:
                fRet = (dwFlags & UPF_SCHEME_NOHISTORY);
                break;

            case URLIS_FILEURL:
                fRet = (eScheme == URL_SCHEME_FILE);
                break;

            case URLIS_APPLIABLE:
                if (eScheme == URL_SCHEME_UNKNOWN)
                {
                    if (S_OK == UrlGuessScheme(pszURL, &str))
                        fRet = TRUE;
                }
                break;

            //  these cases need a broken URL
            case URLIS_DIRECTORY:
            case URLIS_HASQUERY:
                {
                    URLPARTS parts;

                    if (SUCCEEDED(str.SetStr(pszURL))
                    &&  SUCCEEDED(BreakUrl(str, &parts)))
                    {
                        switch(UrlIs)
                        {
                        case URLIS_DIRECTORY:
                            //  if the last seg has a trailing slash, or
                            //  if there are no path segments at all...
                            fRet = (!parts.cSegments || (parts.dwFlags & UPF_EXSEG_DIRECTORY));
                            break;

                        case URLIS_HASQUERY:
                            fRet = (parts.pszQuery && *parts.pszQuery);
                            break;

                        default:
                            ASSERT(FALSE);
                            break;
                        }
                    }
                }
                break;

            default:
                AssertMsg(FALSE, "UrlIs() called with invalid flag");

            }
        }
    }
    return fRet;
}


LWSTDAPI_(BOOL) UrlIsOpaqueA(LPCSTR pszURL)
{
    return UrlIsA(pszURL, URLIS_OPAQUE);
}

LWSTDAPI_(BOOL) UrlIsOpaqueW(LPCWSTR pszURL)
{
    return UrlIsW(pszURL, URLIS_OPAQUE);
}


LWSTDAPI_(BOOL) UrlIsNoHistoryA(LPCSTR pszURL)
{
    return UrlIsA(pszURL, URLIS_NOHISTORY);
}

LWSTDAPI_(BOOL) UrlIsNoHistoryW(LPCWSTR pszURL)
{
    return UrlIsW(pszURL, URLIS_NOHISTORY);
}

LWSTDAPI_(LPCSTR) UrlGetLocationA(LPCSTR pszURL)
{
    CPINFO cpinfo;
    BOOL fMBCS = (GetCPInfo(CP_ACP, &cpinfo) && cpinfo.LeadByte[0]);

    if(pszURL)
    {
        DWORD cchScheme, dwFlags;
        LPCSTR pszScheme = FindSchemeA(pszURL, &cchScheme);
        if(pszScheme)
        {
            URL_SCHEME eScheme = GetSchemeTypeAndFlagsA(pszScheme, cchScheme, &dwFlags);

            return (dwFlags & UPF_SCHEME_OPAQUE) ? NULL : FindFragmentA(pszURL, fMBCS, (eScheme == URL_SCHEME_FILE));
        }
    }
    return NULL;
}

LWSTDAPI_(LPCWSTR) UrlGetLocationW(LPCWSTR wzURL)
{
    if(wzURL)
    {
        DWORD cchScheme, dwFlags;
        LPCWSTR pszScheme = FindSchemeW(wzURL, &cchScheme);
        if(pszScheme)
        {
            URL_SCHEME eScheme = GetSchemeTypeAndFlagsW(pszScheme, cchScheme, &dwFlags);

            return (dwFlags & UPF_SCHEME_OPAQUE) ? NULL : FindFragmentW(wzURL, (eScheme == URL_SCHEME_FILE));
        }
    }
    return NULL;
}


LWSTDAPI_(int)
UrlCompareA(LPCSTR psz1, LPCSTR psz2, BOOL fIgnoreSlash)
{
    SHSTRW str1, str2;

    if (psz1 && psz2)
    {
        if(SUCCEEDED(str1.SetStr(psz1)) && SUCCEEDED(str2.SetStr(psz2)) &&
            SUCCEEDED(SHUrlUnescapeW((LPWSTR)str1, 0)) && SUCCEEDED(SHUrlUnescapeW((LPWSTR)str2, 0)) )
        {
            if(fIgnoreSlash)
            {
                LPWSTR pch;

                pch = (LPWSTR)str1 + str1.GetLen() - 1;
                if(*pch == SLASH)
                    TERMSTR(pch);

                pch = (LPWSTR)str2 + str2.GetLen() - 1;
                if(*pch == SLASH)
                    TERMSTR(pch);
            }

            return StrCmpW((LPWSTR) str1, (LPWSTR) str2);
        }
    }

    return lstrcmpA(psz1, psz2);
}

LWSTDAPI
UrlUnescapeA(LPSTR pszUrl, LPSTR pszOut, LPDWORD pcchOut, DWORD dwFlags)
{
    if(dwFlags & URL_UNESCAPE_INPLACE)
    {
        return SHUrlUnescapeA(pszUrl, dwFlags);
    }

    if (!pszUrl
        || !pcchOut
        || !*pcchOut
        || !pszOut)
    {
        return E_INVALIDARG;
    }

    RIP(IS_VALID_STRING_PTRA(pszUrl, INTERNET_MAX_PATH_LENGTH));
    RIP(IS_VALID_WRITE_PTR(pcchOut, DWORD));
    RIP(IS_VALID_WRITE_BUFFER(pszOut, CHAR, *pcchOut));

    SHSTRA str;
    HRESULT hr = str.SetStr(pszUrl);
    if(SUCCEEDED(hr))
    {
        SHUrlUnescapeA((LPSTR)str, dwFlags);
        hr = CopyOutA(&str, pszOut, pcchOut);
    }
    return hr;
}




LWSTDAPI
PathCreateFromUrlA(LPCSTR pszIn, LPSTR pszOut, LPDWORD pcchOut, DWORD dwFlags)
{
    HRESULT hr;
    SHSTRA straOut;

    if (!pszIn || !pszOut ||
        !pcchOut || !*pcchOut )
        hr = E_INVALIDARG;
    else
    {
        SHSTRW strwOut;
        SHSTRW strwIn;

        if(SUCCEEDED(strwIn.SetStr(pszIn)))
            hr = SHPathCreateFromUrl((LPWSTR) strwIn, &strwOut, dwFlags);
        else
            hr = E_OUTOFMEMORY;

        if(SUCCEEDED(hr))
            hr = straOut.SetStr(strwOut);
    }

    if(SUCCEEDED(hr) )
        hr = CopyOutA(&straOut, pszOut, pcchOut);

    return hr;
}

LWSTDAPI
UrlCreateFromPathA(LPCSTR pszIn, LPSTR pszOut, LPDWORD pcchOut, DWORD dwFlags)
{
    HRESULT hr;
    SHSTRA straOut;

    if (!pszIn || !pszOut ||
        !pcchOut || !*pcchOut )
        hr = E_INVALIDARG;
    else
    {
        SHSTRW strwOut;
        SHSTRW strwIn;

        if(SUCCEEDED(strwIn.SetStr(pszIn)))
            hr = SHUrlCreateFromPath((LPWSTR) strwIn, &strwOut, dwFlags);
        else
            hr = E_OUTOFMEMORY;

        if(SUCCEEDED(hr))
        {
            hr = ReconcileHresults(hr, straOut.SetStr(strwOut));
        }
    }

    if(SUCCEEDED(hr) )
        hr = ReconcileHresults(hr, CopyOutA(&straOut, pszOut, pcchOut));

    return hr;

}

LWSTDAPI
UrlApplySchemeA(LPCSTR pszIn, LPSTR pszOut, LPDWORD pcchOut, DWORD dwFlags)
{
    HRESULT hr;
    SHSTRA straOut;

    if (!pszIn || !pszOut ||
        !pcchOut || !*pcchOut )
        hr = E_INVALIDARG;
    else
    {
        SHSTRW strwOut;
        SHSTRW strwIn;

        if(SUCCEEDED(strwIn.SetStr(pszIn)))
            hr = SHUrlApplyScheme((LPWSTR) strwIn, &strwOut, dwFlags);
        else
            hr = E_OUTOFMEMORY;

        if(S_OK == (hr))
            hr = straOut.SetStr(strwOut);
    }

    if(S_OK == (hr))
        hr = CopyOutA(&straOut, pszOut, pcchOut);

    return hr;

}

// PERF_CACHE
//***   g_szUCCanon -- 1-element cache for UrlCanonicalizeW
// DESCRIPTION
//  it turns out a large # of our calls a) are for the same thing,
// and b) have pszOut(canon)=pszIn(raw).  so cache the most recent guy.
LONG g_lockUC;
WCHAR g_szUCCanon[64];      // post-canon guy (also used for pre-canon check)
DWORD g_dwUCFlags;

#ifdef DEBUG
int g_cUCTot, g_cUCHit;
#endif

LWSTDAPI
UrlCanonicalizeW(LPCWSTR pszUrl,
           LPWSTR pszCanonicalized,
           LPDWORD pcchCanonicalized,
           DWORD dwFlags)
{
    HRESULT hr;
    SHSTRW strwOut;

    if (!pszUrl
        || !pszCanonicalized
        || !pcchCanonicalized
        || !*pcchCanonicalized)
    {
        hr = E_INVALIDARG;
    }
    else
    {
#ifdef DEBUG
        if ((g_cUCTot % 10) == 0)
            TraceMsg(DM_PERF, "uc: tot=%d hit=%d", g_cUCTot, g_cUCHit);
#endif
        RIP(IS_VALID_STRING_PTRW(pszUrl, INTERNET_MAX_PATH_LENGTH));
        RIP(IS_VALID_WRITE_PTR(pcchCanonicalized, DWORD));
        RIP(IS_VALID_WRITE_BUFFER(pszCanonicalized, WCHAR, *pcchCanonicalized));

        DBEXEC(TRUE, g_cUCTot++);
        // try the cache 1st
        if (InterlockedExchange(&g_lockUC, 1) == 0) {
            hr = E_FAIL;
            if ((g_dwUCFlags==dwFlags)
                &&
                (!(dwFlags & URL_ESCAPE_PERCENT))
                &&
                StrCmpCW(pszUrl, g_szUCCanon) == 0)
            {
                DBEXEC(TRUE, g_cUCHit++);
                DWORD cchTmp = *pcchCanonicalized;
                hr = StrCopyOutW(g_szUCCanon, pszCanonicalized, pcchCanonicalized);
                if (FAILED(hr))
                    *pcchCanonicalized = cchTmp;    // restore!
            }
            InterlockedExchange(&g_lockUC, 0);
            if (SUCCEEDED(hr))
                return hr;
        }

        hr = UrlCombineW(L"", pszUrl, pszCanonicalized, pcchCanonicalized, dwFlags);
        if (SUCCEEDED(hr) && *pcchCanonicalized < ARRAYSIZE(g_szUCCanon)) {
            if (InterlockedExchange(&g_lockUC, 1) == 0) {
                StrCpyW(g_szUCCanon, pszCanonicalized);
                g_dwUCFlags = dwFlags;
                InterlockedExchange(&g_lockUC, 0);
            }
        }
    }

    return hr;
}

LWSTDAPI
UrlEscapeW(LPCWSTR pszUrl,
           LPWSTR pszEscaped,
           LPDWORD pcchEscaped,
           DWORD dwFlags)
{
    HRESULT hr;
    SHSTRW strwOut;

    if (!pszUrl || !pszEscaped ||
        !pcchEscaped || !*pcchEscaped)
        hr = E_INVALIDARG;
    else
    {
        hr = SHUrlEscape(pszUrl, &strwOut, dwFlags);
    }

    if(SUCCEEDED(hr) )
        hr = CopyOutW(&strwOut, pszEscaped, pcchEscaped);

    return hr;
}


LWSTDAPI_(int)
UrlCompareW(LPCWSTR psz1, LPCWSTR psz2, BOOL fIgnoreSlash)
{

    if (psz1 && psz2)
    {
        SHSTRW str1, str2;

        if( SUCCEEDED(str1.SetStr(psz1)) && SUCCEEDED(str2.SetStr(psz2)) &&
            SUCCEEDED(SHUrlUnescapeW((LPWSTR)str1, 0)) && SUCCEEDED(SHUrlUnescapeW((LPWSTR)str2, 0)))
        {
            if(fIgnoreSlash)
            {
                LPWSTR pch;

                pch = (LPWSTR)str1 + str1.GetLen() - 1;
                if(*pch == SLASH)
                    TERMSTR(pch);

                pch = (LPWSTR)str2 + str2.GetLen() - 1;
                if(*pch == SLASH)
                    TERMSTR(pch);
            }

            return StrCmpW((LPWSTR) str1, (LPWSTR) str2);
        }
    }

    return StrCmpW(psz1, psz2);
}



LWSTDAPI
UrlUnescapeW(LPWSTR pszUrl, LPWSTR pszOut, LPDWORD pcchOut, DWORD dwFlags)
{
    if(dwFlags & URL_UNESCAPE_INPLACE)
    {
        return SHUrlUnescapeW(pszUrl, dwFlags);
    }

    if (!pszUrl
        || !pcchOut
        || !*pcchOut
        || !pszOut)
    {
        return E_INVALIDARG;
    }

    RIP(IS_VALID_STRING_PTRW(pszUrl, INTERNET_MAX_PATH_LENGTH));
    RIP(IS_VALID_WRITE_PTR(pcchOut, DWORD));
    RIP(IS_VALID_WRITE_BUFFER(pszOut, WCHAR, *pcchOut));

    SHSTRW str;
    HRESULT hr = str.SetStr(pszUrl);
    if(SUCCEEDED(hr))
    {
        SHUrlUnescapeW((LPWSTR)str, dwFlags);
        hr = CopyOutW(&str, pszOut, pcchOut);
    }

    return hr;
}


LWSTDAPI
PathCreateFromUrlW
           (LPCWSTR pszIn,
           LPWSTR pszOut,
           LPDWORD pcchOut,
           DWORD dwFlags)
{
    HRESULT hr;
    SHSTRW strOut;

    if (!pszIn || !pszOut ||
        !pcchOut || !*pcchOut )
        hr = E_INVALIDARG;
    else
        hr = SHPathCreateFromUrl(pszIn, &strOut, dwFlags);

    if(SUCCEEDED(hr) )
        hr = CopyOutW(&strOut, pszOut, pcchOut);

    return hr;

}

LWSTDAPI
UrlCreateFromPathW
           (LPCWSTR pszIn,
           LPWSTR pszOut,
           LPDWORD pcchOut,
           DWORD dwFlags)
{
    HRESULT hr;
    SHSTRW strOut;

    if (!pszIn || !pszOut ||
        !pcchOut || !*pcchOut )
        hr = E_INVALIDARG;
    else
        hr = SHUrlCreateFromPath(pszIn, &strOut, dwFlags);

    if(SUCCEEDED(hr) )
        hr = ReconcileHresults(hr, CopyOutW(&strOut, pszOut, pcchOut));

    return hr;

}

LWSTDAPI
UrlGetPartW(LPCWSTR pszIn, LPWSTR pszOut, LPDWORD pcchOut, DWORD dwPart, DWORD dwFlags)
{
    SHSTRW strIn, strOut;
    HRESULT hr;

    if (!pszIn || !pszOut ||
        !pcchOut || !*pcchOut || !dwPart)
        hr = E_INVALIDARG;
    else if (SUCCEEDED(hr = strIn.SetStr(pszIn)))
        hr = SHUrlGetPart(&strIn, &strOut, dwPart, dwFlags);

    if(SUCCEEDED(hr) )
        hr = CopyOutW(&strOut, pszOut, pcchOut);

    return hr;
}


LWSTDAPI
UrlApplySchemeW
           (LPCWSTR pszIn,
           LPWSTR pszOut,
           LPDWORD pcchOut,
           DWORD dwFlags)
{
    HRESULT hr;
    SHSTRW strOut;

    if (!pszIn || !pszOut ||
        !pcchOut || !*pcchOut )
        hr = E_INVALIDARG;
    else
        hr = SHUrlApplyScheme(pszIn, &strOut, dwFlags);

    if(S_OK == (hr))
        hr = CopyOutW(&strOut, pszOut, pcchOut);

    return hr;

}

//
//  this is the same table used by both URLMON and WININET's cache
//
const static BYTE Translate[256] =
{
    1, 14,110, 25, 97,174,132,119,138,170,125,118, 27,233,140, 51,
    87,197,177,107,234,169, 56, 68, 30,  7,173, 73,188, 40, 36, 65,
    49,213,104,190, 57,211,148,223, 48,115, 15,  2, 67,186,210, 28,
    12,181,103, 70, 22, 58, 75, 78,183,167,238,157,124,147,172,144,
    176,161,141, 86, 60, 66,128, 83,156,241, 79, 46,168,198, 41,254,
    178, 85,253,237,250,154,133, 88, 35,206, 95,116,252,192, 54,221,
    102,218,255,240, 82,106,158,201, 61,  3, 89,  9, 42,155,159, 93,
    166, 80, 50, 34,175,195,100, 99, 26,150, 16,145,  4, 33,  8,189,
    121, 64, 77, 72,208,245,130,122,143, 55,105,134, 29,164,185,194,
    193,239,101,242,  5,171,126, 11, 74, 59,137,228,108,191,232,139,
    6, 24, 81, 20,127, 17, 91, 92,251,151,225,207, 21, 98,113,112,
    84,226, 18,214,199,187, 13, 32, 94,220,224,212,247,204,196, 43,
    249,236, 45,244,111,182,153,136,129, 90,217,202, 19,165,231, 71,
    230,142, 96,227, 62,179,246,114,162, 53,160,215,205,180, 47,109,
    44, 38, 31,149,135,  0,216, 52, 63, 23, 37, 69, 39,117,146,184,
    163,200,222,235,248,243,219, 10,152,131,123,229,203, 76,120,209
};

PRIVATE void _HashData(LPBYTE pbData, DWORD cbData, LPBYTE pbHash, DWORD cbHash)
{
    DWORD i, j;
    //  seed the hash
    for (i = cbHash; i-- > 0;)
        pbHash[i] = (BYTE) i;

    //  do the hash
    for (j = cbData; j-- > 0;)
    {
        for (i = cbHash; i-- > 0;)
            pbHash[i] = Translate[pbHash[i] ^ pbData[j]];
    }
}

LWSTDAPI
HashData(LPBYTE pbData, DWORD cbData, LPBYTE pbHash, DWORD cbHash)
{
    if(EVAL(IS_VALID_READ_BUFFER(pbData, BYTE, cbData)) && EVAL(IS_VALID_WRITE_BUFFER(pbHash, BYTE, cbHash)))
    {
        _HashData(pbData, cbData, pbHash, cbHash);
        return S_OK;
    }
    return E_INVALIDARG;
}


LWSTDAPI
UrlHashA(LPCSTR psz, LPBYTE pb, DWORD cb)
{
    HRESULT hr = E_INVALIDARG;

    if(EVAL(IS_VALID_STRING_PTRA(psz, -1)) && EVAL(IS_VALID_WRITE_BUFFER(pb, BYTE, cb)))
    {
        _HashData((LPBYTE) psz, lstrlenA(psz), pb, cb);
        return S_OK;
    }

    return hr;
}


LWSTDAPI
UrlHashW(LPCWSTR psz, LPBYTE pb, DWORD cb)
{
    HRESULT hr;
    SHSTRA str;
    if (SUCCEEDED( hr = str.SetStr(psz)))
        hr = UrlHashA((LPSTR)str, pb, cb);

    return hr;
}



/***************************** ParseURL Functions *****************************/
//  these were originally in URL.DLL and then moved to shlwapi.
//  i just added them from url.c for reuse of code.
//  ParseURL now does no MBCS thunks, to keep it fast.
//
//  declarations for ParseURL() APIs
//

typedef const PARSEDURLA   CPARSEDURLA;
typedef const PARSEDURLA * PCPARSEDURLA;

typedef const PARSEDURLW   CPARSEDURLW;
typedef const PARSEDURLW * PCPARSEDURLW;


#ifdef DEBUG

BOOL
IsValidPCPARSEDURLA(
   LPCSTR pcszURL,
   PCPARSEDURLA pcpu)
{
   return(IS_VALID_READ_PTR(pcpu, CPARSEDURLA) &&
          (IS_VALID_STRING_PTRA(pcpu->pszProtocol, -1) &&
           EVAL(IsStringContainedA(pcszURL, pcpu->pszProtocol)) &&
           EVAL(pcpu->cchProtocol < (UINT)lstrlenA(pcpu->pszProtocol))) &&
          (IS_VALID_STRING_PTRA(pcpu->pszSuffix, -1) &&
           EVAL(IsStringContainedA(pcszURL, pcpu->pszSuffix)) &&
           EVAL(pcpu->cchSuffix <= (UINT)lstrlenA(pcpu->pszSuffix))) &&
          EVAL(pcpu->cchProtocol + pcpu->cchSuffix < (UINT)lstrlenA(pcszURL)));
}

BOOL
IsValidPCPARSEDURLW(
   LPCWSTR pcszURL,
   PCPARSEDURLW pcpu)
{
   return(IS_VALID_READ_PTR(pcpu, CPARSEDURLW) &&
          (IS_VALID_STRING_PTRW(pcpu->pszProtocol, -1) &&
           EVAL(IsStringContainedW(pcszURL, pcpu->pszProtocol)) &&
           EVAL(pcpu->cchProtocol < (UINT)lstrlenW(pcpu->pszProtocol))) &&
          (IS_VALID_STRING_PTRW(pcpu->pszSuffix, -1) &&
           EVAL(IsStringContainedW(pcszURL, pcpu->pszSuffix)) &&
           EVAL(pcpu->cchSuffix <= (UINT)lstrlenW(pcpu->pszSuffix))) &&
          EVAL(pcpu->cchProtocol + pcpu->cchSuffix < (UINT)lstrlenW(pcszURL)));
}

#endif


/*----------------------------------------------------------
Purpose: Parse the given path into the PARSEDURL structure.

  ******
  ******  This function must not do any extraneous
  ******  things.  It must be small and fast.
  ******

    Returns: NOERROR if a valid URL format
    URL_E_INVALID_SYNTAX if not

      Cond:    --
*/
STDMETHODIMP
ParseURLA(
          LPCSTR pcszURL,
          PPARSEDURLA ppu)
{
    HRESULT hr = E_INVALIDARG;

    RIP(IS_VALID_STRING_PTRA(pcszURL, -1));
    RIP(IS_VALID_WRITE_PTR(ppu, PARSEDURLA));

    if (pcszURL && ppu && SIZEOF(*ppu) == ppu->cbSize)
    {
        DWORD cch;
        hr = URL_E_INVALID_SYNTAX;      // assume error

        ppu->pszProtocol = FindSchemeA(pcszURL, &cch);

        if(ppu->pszProtocol)
        {
            ppu->cchProtocol = cch;

            // Determine protocol scheme number
            ppu->nScheme = SchemeTypeFromStringA(ppu->pszProtocol, cch);

            ppu->pszSuffix = ppu->pszProtocol + cch + 1;

            //
            //  BUGBUG - Backwards compatibility - zekel 28-feb-97
            //  ParseURL() believes in file: urls like "file://C:\foo\bar"
            //  and some pieces of code will use it to get the Dos Path.
            //  new code should always call PathCreateFromUrl() to
            //  get the dos path of a file: URL.
            //
            //  i am leaving this behavior in case some compat stuff is out there.
            //
            if (URL_SCHEME_FILE == ppu->nScheme &&
                '/' == ppu->pszSuffix[0] && '/' == ppu->pszSuffix[1])
            {
                // Yes; skip the "//"
                ppu->pszSuffix += 2;

#ifndef UNIX
                // FOR UNIX: If we have /vobs/build, we don't want to make
                // There might be a third slash.  Skip it.
                if ('/' == *ppu->pszSuffix)
                    ppu->pszSuffix++;
#endif

            }

            ppu->cchSuffix = lstrlenA(ppu->pszSuffix);

            hr = S_OK;
        }
    }

#ifdef DEBUG
    if (hr == S_OK)
    {
        CHAR rgchDebugProtocol[MAX_PATH];
        CHAR rgchDebugSuffix[MAX_PATH];

        // (+ 1) for null terminator.

        lstrcpynA(rgchDebugProtocol, ppu->pszProtocol,
            min(ppu->cchProtocol + 1, SIZECHARS(rgchDebugProtocol)));

        // (+ 1) for null terminator.

        lstrcpynA(rgchDebugSuffix, ppu->pszSuffix,
            min(ppu->cchSuffix + 1, SIZECHARS(rgchDebugSuffix)));

        TraceMsgA(TF_URL, "ParseURL(): Parsed protocol \"%s\" and suffix \"%s\" from URL \"%s\".",
            rgchDebugProtocol,
            rgchDebugSuffix,
            pcszURL);
    }
    else
    {
        TraceMsgA(TF_URL, "ParseURL(): Failed to parse \"%s\"", pcszURL);
    }
#endif


    ASSERT(FAILED(hr) ||
        EVAL(IsValidPCPARSEDURLA(pcszURL, ppu)));

    return(hr);
}


/*----------------------------------------------------------
Purpose: Parse the given path into the PARSEDURL structure.

  ******
  ******  This function must not do any extraneous
  ******  things.  It must be small and fast.
  ******

    Returns: NOERROR if a valid URL format
    URL_E_INVALID_SYNTAX if not

      Cond:    --
*/
STDMETHODIMP
ParseURLW(
          LPCWSTR pcszURL,
          PPARSEDURLW ppu)
{
    HRESULT hr = E_INVALIDARG;

    RIP(IS_VALID_STRING_PTRW(pcszURL, -1));
    RIP(IS_VALID_WRITE_PTR(ppu, PARSEDURLW));

    if (pcszURL && ppu && SIZEOF(*ppu) == ppu->cbSize)
    {
        DWORD cch;
        hr = URL_E_INVALID_SYNTAX;      // assume error

        ppu->pszProtocol = FindSchemeW(pcszURL, &cch);

        if(ppu->pszProtocol)
        {
            ppu->cchProtocol = cch;

            // Determine protocol scheme number
            ppu->nScheme = SchemeTypeFromStringW(ppu->pszProtocol, cch);

            ppu->pszSuffix = ppu->pszProtocol + cch + 1;

            //
            //  BUGBUG - Backwards compatibility - zekel 28-feb-97
            //  ParseURL() believes in file: urls like "file://C:\foo\bar"
            //  and some pieces of code will use it to get the Dos Path.
            //  new code should always call PathCreateFromUrl() to
            //  get the dos path of a file: URL.
            //
            //  i am leaving this behavior in case some compat stuff is out there.
            //
            if (URL_SCHEME_FILE == ppu->nScheme &&
                '/' == ppu->pszSuffix[0] && '/' == ppu->pszSuffix[1])
            {
                // Yes; skip the "//"
                ppu->pszSuffix += 2;

#ifndef UNIX
                // There might be a third slash.  Skip it.
                // IEUNIX - On UNIX, it's a root directory, so don't skip it!
                if ('/' == *ppu->pszSuffix)
                    ppu->pszSuffix++;
#endif
            }

            ppu->cchSuffix = lstrlenW(ppu->pszSuffix);

            hr = S_OK;
        }
    }


#ifdef DEBUG
    if (hr==S_OK)
    {
        WCHAR rgchDebugProtocol[MAX_PATH];
        WCHAR rgchDebugSuffix[MAX_PATH];

        // (+ 1) for null terminator.

        StrCpyNW(rgchDebugProtocol, ppu->pszProtocol,
            min(ppu->cchProtocol + 1, SIZECHARS(rgchDebugProtocol)));

        // (+ 1) for null terminator.

        StrCpyNW(rgchDebugSuffix, ppu->pszSuffix,
            min(ppu->cchSuffix + 1, SIZECHARS(rgchDebugSuffix)));

        TraceMsg(TF_URL, "ParseURL(): Parsed protocol \"%s\" and suffix \"%s\" from URL \"%s\".",
            rgchDebugProtocol,
            rgchDebugSuffix,
            pcszURL);
    }
    else
    {
        TraceMsg(TF_URL, "ParseURL(): Failed to parse \"%s\"", pcszURL);
    }
#endif

    ASSERT(FAILED(hr) ||
        EVAL(IsValidPCPARSEDURLW(pcszURL, ppu)));

    return(hr);
}

#ifdef USE_FAST_PARSER

// GetSchemeTypeAndFlagsSpecialW
// performs the same behavior as GetSchemeTypeAndFlagsW plus, when successful
// copies the canonicalised form of the scheme back.

PRIVATE URL_SCHEME
GetSchemeTypeAndFlagsSpecialW(LPWSTR pszScheme, DWORD cchScheme, LPDWORD pdwFlags)
{
    DWORD i;

    ASSERT(pszScheme);


#ifdef DEBUG
    if ((g_cSTTot % 10) == 0)
        TraceMsg(DM_PERF, "gstaf: tot=%d hit=%d hit0=%d", g_cSTTot, g_cSTHit, g_cSTHit0);
#endif
    DBEXEC(TRUE, g_cSTTot++);
    // check cache 1st
    i = g_iScheme;
    if (cchScheme == g_mpUrlSchemeTypes[i].cchScheme
      && StrCmpNCW(pszScheme, g_mpUrlSchemeTypes[i].pszScheme, cchScheme) == 0)
    {
        DBEXEC(TRUE, i == 0 ? g_cSTHit0++ : g_cSTHit++);
Lhit:
        if (pdwFlags)
            *pdwFlags = g_mpUrlSchemeTypes[i].dwFlags;

        // update cache (unconditionally)
        g_iScheme = i;

        // We need to do this because the scheme might not be canonicalised
        memcpy(pszScheme, g_mpUrlSchemeTypes[i].pszScheme, cchScheme*sizeof(WCHAR));
        return g_mpUrlSchemeTypes[i].eScheme;
    }

    for (i = 0; i < ARRAYSIZE(g_mpUrlSchemeTypes); i++)
    {
        if(cchScheme == g_mpUrlSchemeTypes[i].cchScheme
          && 0 == StrCmpNIW(pszScheme, g_mpUrlSchemeTypes[i].pszScheme, cchScheme))
            goto Lhit;
    }

    if (pdwFlags)
    {
        *pdwFlags = 0;
    }
    return URL_SCHEME_UNKNOWN;
}



// URL_STRING --------------------------------------------------------------------------------------

// is a container for the combined URL. It attempts to construct a string from the information
// fed into it. If there is not enough buffer space available, it will measure how much additional
// space will be required to hold the string.

WCHAR wszBogus[] = L"";


// US_* are the various modes of transforming characters fed into the container.
// US_NOTHING   do nothing to the character.
// US_UNESCAPE  turn entries of the form %xx into the unescaped form
// US_ESCAPE_UNSAFE transform invalid path characters into %xx sequences
// US_ESCAPE_SPACES transform only spaces in to %20 sequences

enum
{
    US_NOTHING,
    US_UNESCAPE,
    US_ESCAPE_UNSAFE,
    US_ESCAPE_SPACES
};

class URL_STRING
{
protected:
    URL_SCHEME _eScheme;
    DWORD _ccWork, _ccMark, _ccLastWhite, _ccQuery, _ccFragment, _ccBuffer, _dwSchemeInfo;
    DWORD _dwOldFlags, _dwFlags, _dwMode;
    BOOL _fFixSlashes, _fExpecting, _fError;
    WCHAR _wchLast, _wszInternalString[1024];
    PWSTR _pszWork;

    VOID baseAccept(WCHAR wch);
    VOID TrackWhiteSpace(WCHAR wch);

public:
    URL_STRING(DWORD dwFlags);
    ~URL_STRING();

    VOID CleanAccept(WCHAR wch);
    VOID Accept(WCHAR wch);
    VOID Accept(PWSTR a_psz);
    VOID Contract(BOOL fContractLevel = TRUE);
    VOID TrimEndWhiteSpace();

    PWSTR GetStart();
    LONG GetTotalLength();
    BOOL AnyProblems();

    VOID NoteScheme(URL_SCHEME a_eScheme, DWORD a_dwSchemeInfo);
    VOID AddSchemeNote(DWORD a_dwSchemeInfo);
    DWORD GetSchemeNotes();
    URL_SCHEME QueryScheme();

    VOID Mark();
    VOID ClearMark();
    VOID EraseMarkedText();
    DWORD CompareMarkWith(PWSTR psz);
    DWORD CompareLast(PCWSTR psz, DWORD cc);

    VOID EnableMunging();
    VOID DisableMunging();
    VOID DisableSlashFixing();
    VOID BackupFlags();
    VOID RestoreFlags();
    VOID AddFlagNote(DWORD dwFlag);

    VOID NotifyQuery();
    VOID NotifyFragment();
    VOID DropQuery();
    VOID DropFragment();
};

// -------------------------------------------------------------------------------

URL_STRING::URL_STRING(DWORD dwFlags)
{
    _ccBuffer = ARRAYSIZE(_wszInternalString);
    _ccWork = 1;
    _pszWork = _wszInternalString;
    _ccQuery = _ccFragment = _ccMark = 0;

    _eScheme = URL_SCHEME_UNKNOWN;
    _dwOldFlags = _dwFlags = dwFlags;
    _dwMode = US_NOTHING;

    _fFixSlashes = TRUE;
    _fError = _fExpecting = FALSE;
}

URL_STRING::~URL_STRING()
{
    if (_ccBuffer > ARRAYSIZE(_wszInternalString))
    {
        LocalFree(_pszWork);
    }
}

// -------------------------------------------------------------------------------
// These are the standard functions used for adding characters to an url.

VOID URL_STRING::baseAccept(WCHAR wch)
{
    _pszWork[_ccWork-1] = (_fFixSlashes
                    ? ((wch!=WHACK) ? wch : SLASH)
                    : wch);
    _ccWork++;
    if (_ccWork>_ccBuffer)
    {
        if (!_fError)
        {
            PWSTR psz = (PWSTR)LocalAlloc(LPTR, 2*_ccBuffer*sizeof(WCHAR));
            if (!psz)
            {
                _ccWork--;
                _fError = TRUE;
                return;
            }
            memcpy(psz, _pszWork, (_ccWork-1)*sizeof(WCHAR));
            if (_ccBuffer>ARRAYSIZE(_wszInternalString))
            {
                LocalFree(_pszWork);
            }
            _ccBuffer *= 2;
            _pszWork = psz;
        }
        else
        {
            _ccWork--;
        }
    }
}


VOID URL_STRING::TrackWhiteSpace(WCHAR wch)
{
    if (IsWhite(wch))
    {
        if (!_ccLastWhite)
        {
            _ccLastWhite = _ccWork;
        }
    }
    else
    {
        _ccLastWhite = 0;
    }
}


// -- URL_STRING::Accept ----------------------------
// Based on the current munging mode, transform the character into the
// desired form and add it to the string.

VOID URL_STRING::Accept(WCHAR wch)
{
    TrackWhiteSpace(wch);

    switch (_dwMode)
    {
    case US_NOTHING:
        break;

    case US_UNESCAPE:
        if (_fExpecting)
        {
            if (!IsHex(wch))
            {
                baseAccept(HEX_ESCAPE);
                if (_wchLast!=L'\0')
                {
                    baseAccept(_wchLast);
                }
                _fExpecting = FALSE;
                break;
            }
            else if (_wchLast!=L'\0')
            {
                wch = (HexToWord(_wchLast)*16) + HexToWord(wch);
                TrackWhiteSpace(wch);
                _fExpecting = FALSE;
                if ((wch==WHACK) && _fFixSlashes)
                {
                    _fFixSlashes = FALSE;
                    baseAccept(wch);
                    _fFixSlashes = TRUE;
                    return;
                }
                break;
            }
            else
            {
                _wchLast = wch;
            }
            return;
        }
        if (wch==HEX_ESCAPE)
        {
            _fExpecting = TRUE;
            _wchLast = L'\0';
            return;
        }
        break;

     case US_ESCAPE_UNSAFE:
        if ((wch==SLASH)
            ||
            (wch==WHACK && _fFixSlashes)
            ||
            (IsSafePathChar(wch) && (wch!=HEX_ESCAPE || !(_dwFlags & URL_ESCAPE_PERCENT))))
        {
            break;
        }

        baseAccept(L'%');
        baseAccept(hex[(wch >> 4) & 15]);
        baseAccept(hex[wch & 15]);
        return;

    case US_ESCAPE_SPACES:
        if (wch==SPC)
        {
            baseAccept(L'%');
            baseAccept(L'2');
            baseAccept(L'0');
            return;
        }
        break;
     default:
        ASSERT(FALSE);
    }
    baseAccept(wch);
}

// -- Accept --------------------------------
// Accept only a string
VOID URL_STRING::Accept(PWSTR psz)
{
    while (*psz)
    {
        Accept(*psz);
        psz++;
    }
}

// -- Contract
// Whenever we call Contract, we're pointing past the last separator. We want to
// omit the segment between this separator and the one before it.
// This should be used ONLY when we're examining the path segment of the urls.

VOID URL_STRING::Contract(BOOL fContractLevel)
{
    ASSERT(_ccWork && _ccMark);

    // _ccWork is 1 after wherever the next character will be placed
    // subtract +1 to derive what the last character in the url is
    DWORD _ccEnd = _ccWork-1 - 1;
    if (_eScheme!=URL_SCHEME_MK)
    {
        if (!fContractLevel && (_pszWork[_ccEnd]==SLASH || _pszWork[_ccEnd]==WHACK))
        {
            return;
        }
        do
        {
            _ccEnd--;
        }
        while ((_ccEnd>=_ccMark-1) && _pszWork[_ccEnd]!=SLASH && _pszWork[_ccEnd]!=WHACK);
    }
    else
    {
        if (!fContractLevel && (_pszWork[_ccEnd]==SLASH))
        {
            return;
        }
        do
        {
            _ccEnd--;
        }
        while ((_ccEnd>=_ccMark-1) && _pszWork[_ccEnd]!=SLASH);
    }
    if (_ccEnd<_ccMark-1)
    {
        _ccEnd = _ccMark-1;
    }
    else
    {
        _ccEnd++;
    }
    _ccWork = _ccEnd + 1;
}

VOID URL_STRING::TrimEndWhiteSpace()
{
    if (_ccLastWhite)
    {
        _ccWork = _ccLastWhite;
        _ccLastWhite = 0;
    }
}


VOID URL_STRING::CleanAccept(WCHAR wch)
{
    baseAccept(wch);
}


// -------------------------------------------------------------------------------
// These member functions return information about the url that is being formed

PWSTR URL_STRING::GetStart()
{
    return _pszWork;
}

LONG URL_STRING::GetTotalLength()
{
    return _ccWork - 1;
}

BOOL URL_STRING::AnyProblems()
{
    return _fError;
}

// -------------------------------------------------------------------------------

VOID URL_STRING::NoteScheme(URL_SCHEME a_eScheme, DWORD a_dwSchemeInfo)
{
    _eScheme = a_eScheme;
    _dwSchemeInfo = a_dwSchemeInfo;
    _fFixSlashes = a_dwSchemeInfo & UPF_SCHEME_CONVERT;
}

VOID URL_STRING::AddSchemeNote(DWORD a_dwSchemeInfo)
{
    _dwSchemeInfo |= a_dwSchemeInfo;
    _fFixSlashes = _dwSchemeInfo & UPF_SCHEME_CONVERT;
}

DWORD URL_STRING::GetSchemeNotes()
{
    return _dwSchemeInfo;
}

URL_SCHEME URL_STRING::QueryScheme()
{
    return _eScheme;
}

// -------------------------------------------------------------------------------

VOID URL_STRING::Mark()
{
    _ccMark = _ccWork;
}

VOID URL_STRING::ClearMark()
{
    _ccMark = 0;
}

VOID URL_STRING::EraseMarkedText()
{
    if (_ccMark)
    {
        _ccWork = _ccMark;
        _ccMark = 0;
    }
}

DWORD URL_STRING::CompareMarkWith(PWSTR psz)
{
    if (_ccMark)
    {
        *(_pszWork + _ccWork - 1) = L'\0';
        return (StrCmpW(_pszWork + _ccMark - 1, psz));
    }
    // In other words, return that the string isn't present.
    return 1;
}

DWORD URL_STRING::CompareLast(PCWSTR psz, DWORD cc)
{
    if (_ccWork > cc)
    {
        return StrCmpNIW(_pszWork + _ccWork - 1 - cc, psz, cc);
    }
    return 1;
}

// -------------------------------------------------------------------------------

VOID URL_STRING::NotifyQuery()
{
    if (!_ccQuery)
    {
        _ccQuery = _ccWork;
    }
}

VOID URL_STRING::NotifyFragment()
{
    if (!_ccFragment)
    {
        _ccFragment = _ccWork;
        CleanAccept(POUND);
    }
}

VOID URL_STRING::DropQuery()
{
    if (_ccQuery)
    {
        _ccWork = _ccQuery;
        _ccQuery = _ccFragment = 0;
    }
}

VOID URL_STRING::DropFragment()
{
    if (_ccFragment)
    {
        _ccWork = _ccFragment;
        _ccFragment = 0;
    }
}

// -------------------------------------------------------------------------------
// These member functions are for determining how the url's characters are going
// to be represented

VOID URL_STRING::EnableMunging()
{
    _dwMode = US_NOTHING;

    // For opaque urls, munge ONLY if we're explicitly asked to URL_ESCAPE or URL_UNESCAPE,
    // but NOT URL_ESCAPE_SPACES_ONLY

    // For query and fragment, never allow for URL_ESCAPE_UNSAFE and for
    //    others ONLY when URL_DONT_ESCAPE_EXTRA_INFO is specified

    if ((_dwSchemeInfo & UPF_SCHEME_OPAQUE)
        && (_dwFlags & URL_ESCAPE_SPACES_ONLY))
        return;

    if ((_ccQuery || _ccFragment)
        && ((_dwFlags & (URL_DONT_ESCAPE_EXTRA_INFO | URL_ESCAPE_UNSAFE))))
        return;

    if (_dwFlags & URL_UNESCAPE)
    {
        _dwMode = US_UNESCAPE;
    }
    else if (_dwFlags & URL_ESCAPE_UNSAFE)
    {
        _dwMode = US_ESCAPE_UNSAFE;
    }
    else if (_dwFlags & URL_ESCAPE_SPACES_ONLY)
    {
        _dwMode = US_ESCAPE_SPACES;
    }
}

VOID URL_STRING::DisableMunging()
{
    _dwMode = US_NOTHING;
}

VOID URL_STRING::DisableSlashFixing()
{
    _fFixSlashes = FALSE;
}
VOID URL_STRING::AddFlagNote(DWORD dwFlag)
{
    _dwFlags |= dwFlag;
}

VOID URL_STRING::BackupFlags()
{
    _dwOldFlags = _dwFlags;
}

VOID URL_STRING::RestoreFlags()
{
    ASSERT((_eScheme==URL_SCHEME_FILE) || (_dwFlags==_dwOldFlags));
    _dwFlags = _dwOldFlags;
    EnableMunging();
}

// -------------------------------------------------------------------------------


// URL ------------------------------------------------------------------------------------
// The URL class is used to examine the base and relative URLs to determine what
// will go into the URL_STRING container. The difference should be clear:
// URL instances look, but don't touch. URL_STRINGs are used solely to build urls.


class URL
{
private:
    PWSTR _pszUrl, _pszWork;
    URL_SCHEME _eScheme;
    DWORD _dwSchemeNotes, _dwFlags;
    BOOL _fPathCompressionOn;
    BOOL _fIgnoreQuery;

    WCHAR SmallForm(WCHAR wch);
    BOOL IsAlpha(WCHAR ch);
    PWSTR IsUrlPrefix(PWSTR psz);
    BOOL IsLocalDrive(PWSTR psz);
    BOOL IsQualifiedDrive(PWSTR psz);
    BOOL DetectSymbols(WCHAR wch1, WCHAR wch2 = '\0', WCHAR wch3 = '\0');

    PWSTR NextChar(PWSTR psz);
    PWSTR FeedUntil(PWSTR psz, URL_STRING* pus, WCHAR wchDelim1 = '\0', WCHAR wchDelim2 = '\0', WCHAR wchDelim3 = '\0', WCHAR wchDelim4 = '\0');

    BOOL DetectFileServer();
    BOOL DetectMkServer();
    BOOL DefaultDetectServer();
    VOID FeedDefaultServer(URL_STRING* pus);
    VOID FeedFileServer(URL_STRING* pus);
    VOID FeedFtpServer(URL_STRING* pus);
    VOID FeedMkServer(URL_STRING* pus);
    PWSTR FeedPort(PWSTR psz, URL_STRING* pus);

public:
    VOID Setup(PWSTR pszInUrl, DWORD a_dwFlags = 0);
    VOID Reset();
    BOOL IsReset();

    BOOL DetectAndFeedScheme(URL_STRING* pus, BOOL fReconcileSchemes = FALSE);
    VOID SetScheme(URL_SCHEME eScheme, DWORD dwFlag);
    URL_SCHEME GetScheme();
    VOID AddSchemeNote(DWORD dwFlag);
    DWORD GetSchemeNotes();

    BOOL DetectServer();
    BOOL DetectAbsolutePath();
    BOOL DetectPath();
    BOOL DetectQueryOrFragment();
    BOOL DetectQuery();
    BOOL DetectLocalDrive();
    BOOL DetectSlash();
    BOOL DetectAnything();
    WCHAR PeekNext();

    VOID FeedPath(URL_STRING* pus, BOOL fMarkServer = TRUE);
    PWSTR CopySegment(PWSTR psz, URL_STRING* pus, BOOL* pfContinue);
    DWORD DetectDots(PWSTR* ppsz);
    VOID StopPathCompression();

    VOID FeedServer(URL_STRING* pus);
    VOID FeedLocalDrive(URL_STRING* pus);
    VOID FeedQueryAndFragment(URL_STRING* pus);
    VOID IgnoreQuery();
};

// -------------------------------------------------------------------------------

VOID URL::Setup(PWSTR pszInUrl, DWORD a_dwFlags)
{
    while (*pszInUrl && IsWhite(*pszInUrl))
    {
        pszInUrl++;
    }
    _pszWork = _pszUrl = pszInUrl;
    _eScheme = URL_SCHEME_UNKNOWN;
    _dwSchemeNotes = 0;
    _dwFlags = a_dwFlags;
    _fPathCompressionOn = TRUE;
    _fIgnoreQuery = FALSE;
}

VOID URL::Reset()
{
    _pszWork = wszBogus;
}

BOOL URL::IsReset()
{
    return (_pszWork==wszBogus);
}

// -------------------------------------------------------------------------------

inline WCHAR URL::SmallForm(WCHAR wch)
{
    return (wch < L'A' || wch > L'Z') ? wch : (wch - L'A' + L'a');
}

inline BOOL URL::IsAlpha(WCHAR ch)
{
    return ((ch >= 'a') && (ch <= 'z'))
           ||
           ((ch >= 'A') && (ch <= 'Z'));
}


inline PWSTR URL::IsUrlPrefix(PWSTR psz)
{
    // We want to skip instances of "URL:"
    psz = NextChar(psz);
    if (*psz==L'u' || *psz==L'U')
    {
        psz = NextChar(psz+1);
        if (*psz==L'r' || *psz==L'R')
        {
            psz = NextChar(psz+1);
            if (*psz==L'l' || *psz==L'L')
            {
                psz = NextChar(psz+1);
                if (*psz==COLON)
                {
                    return NextChar(psz+1);
                }
            }
        }
    }
    return NULL;
}

inline BOOL URL::IsLocalDrive(PWSTR psz)
{
    psz = NextChar(psz);
    return (IsAlpha(*psz)
            &&
            ((*NextChar(psz+1)==COLON) || (*NextChar(psz+1)==BAR)));
}

// -- IsQualifiedDrive --------
// On Win32 systems, a qualified drive is either
// i. <letter>:  or ii. \\UNC\
// Under unix, it's only /.

inline BOOL URL::IsQualifiedDrive(PWSTR psz)
{
    psz = NextChar(psz);
    BOOL fResult = IsLocalDrive(psz);
    if (!fResult && *psz==WHACK)
    {
        psz = NextChar(psz+1);
        fResult = *psz==WHACK;
    }
    return fResult;
}

// -- DetectSymbols -------------
// This is used to help determine what part of the URL we have reached.
inline BOOL URL::DetectSymbols(WCHAR wch1, WCHAR wch2, WCHAR wch3)
{
    ASSERT(_pszWork);
    PWSTR psz = NextChar(_pszWork);
    return (*psz && (*psz==wch1 || *psz==wch2 || *psz==wch3));
}

BOOL URL::DetectSlash()
{
    return DetectSymbols(SLASH, WHACK);
}

BOOL URL::DetectAnything()
{
    return (*NextChar(_pszWork)!=L'\0');
}

// -- NextChar -------------------------------------
// We use NextChar instead of *psz because we want to
// ignore characters such as TAB, CR, etc.
inline PWSTR URL::NextChar(PWSTR psz)
{
    while (IsInsignificantWhite(*psz))
    {
        psz++;
    }
    return psz;
}

WCHAR URL::PeekNext()
{
    return (*NextChar(NextChar(_pszWork)+1));
}


// -------------------------------------------------------------------------------

inline PWSTR URL::FeedUntil(PWSTR psz, URL_STRING* pus, WCHAR wchDelim1, WCHAR wchDelim2, WCHAR wchDelim3, WCHAR wchDelim4)
{
    psz = NextChar(psz);
    while (*psz && *psz!=wchDelim1 && *psz!=wchDelim2 && *psz!=wchDelim3 && *psz!=wchDelim4)
    {
        pus->Accept(*psz);
        psz = NextChar(psz+1);
    }
    return psz;
}

// -------------------------------------------------------------------------------

VOID URL::SetScheme(URL_SCHEME eScheme, DWORD dwFlag)
{
    _eScheme = eScheme;
    _dwSchemeNotes = dwFlag;
}

URL_SCHEME URL::GetScheme()
{
    return _eScheme;
}

VOID URL::AddSchemeNote(DWORD dwFlag)
{
    _dwSchemeNotes |= dwFlag;
}

DWORD URL::GetSchemeNotes()
{
    return _dwSchemeNotes;
}

BOOL URL::DetectAndFeedScheme(URL_STRING* pus, BOOL fReconcileSchemes)
{
    ASSERT(_pszWork);
    ASSERT(!fReconcileSchemes || (fReconcileSchemes && pus->QueryScheme()!=URL_SCHEME_FILE));

    PWSTR psz = NextChar(_pszWork);
    BOOL fResult = (IsQualifiedDrive(_pszWork));
    if (fResult)
    {
        //
        // Detected a File URL that isn't explicitly marked as such, ie C:\foo,
        //   in this case, we need to confirm that we're not overwriting
        //   a fully qualified relative URL with an Accept("file:"), although
        //   if the relative URL is the same scheme as the base, we now
        //   need to make the BASE-file URL take precedence.
        //

        _eScheme = URL_SCHEME_FILE;

        if (!fReconcileSchemes)
        {
            pus->Accept((PWSTR)c_szFileScheme);
            pus->Accept(COLON);
            _dwSchemeNotes = g_mpUrlSchemeTypes[1].dwFlags;
            pus->NoteScheme(_eScheme, _dwSchemeNotes);
            pus->AddFlagNote(URL_ESCAPE_PERCENT | URL_ESCAPE_UNSAFE);
        }
        else if (pus->QueryScheme() != URL_SCHEME_FILE)
        {
            Reset();
        }

        goto exit;
    }

    for (;;)
    {
        while (IsValidSchemeCharW(*psz))
        {
            psz = NextChar(psz + 1);
        }
        if (*psz!=COLON)
        {
            break;
        }
        if (IsUrlPrefix(_pszWork))
        {
        // However, we want to skip instances of URL:
            _pszWork = NextChar(psz+1);
            continue;
        }

        DWORD ccScheme = 0;
        PWSTR pszClone = NextChar(_pszWork);

        if (!fReconcileSchemes)
        {
            while (pszClone<=psz)
            {
                pus->Accept(SmallForm(*pszClone));
                ccScheme++;
                pszClone = NextChar(pszClone+1);
            }
            _pszWork = pszClone;
            // Subtract one for the colon
            ccScheme--;
            // BUG BUG Since we're smallifying the scheme above, we might be able to
            // avoid calling this func, call GetSchemeTypeAndFlags instead.
            _eScheme = GetSchemeTypeAndFlagsSpecialW(pus->GetStart(), ccScheme, &_dwSchemeNotes);
            pus->NoteScheme(_eScheme, _dwSchemeNotes);
        }
        else
        {
            PWSTR pszKnownScheme = pus->GetStart();
            while (pszClone<=psz && SmallForm(*pszClone)==*pszKnownScheme)
            {
                pszClone = NextChar(pszClone+1);
                pszKnownScheme++;
            }
            if (pszClone<=psz)
            {
                Reset();
            }
            else
            {
                _pszWork = pszClone;
            }
        }
        fResult = TRUE;
        break;
    }
 exit:
    return fResult;
}

// -------------------------------------------------------------------------------

BOOL URL::DetectServer()
{
    ASSERT(_pszWork);
    BOOL fRet;

    switch (_eScheme)
    {
    case URL_SCHEME_FILE:
        fRet = DetectFileServer();
        break;

    case URL_SCHEME_MK:
        fRet = DetectMkServer();
        break;

    default:
        fRet = DefaultDetectServer();
        break;
    }
    return fRet;
}

BOOL URL::DetectLocalDrive()
{
    return IsLocalDrive(_pszWork);
}

BOOL URL::DetectFileServer()
{
    ASSERT(_pszWork);
    PWSTR psz = _pszWork;

    BOOL fResult = IsLocalDrive(_pszWork);
    if (fResult)
    {
        _dwSchemeNotes |= UPF_FILEISPATHURL;
    }
    else
    {
        fResult = DetectSymbols(SLASH, WHACK);
    }
    return fResult;
}

BOOL URL::DetectMkServer()
{
    ASSERT(_pszWork);
    PWSTR psz = NextChar(_pszWork);
    BOOL fResult = (*psz==L'@');
    if (fResult)
    {
        _pszWork = NextChar(psz + 1);
    }
    return fResult;
}

BOOL URL::DefaultDetectServer()
{
    BOOL fResult = FALSE;
    if (DetectSymbols(SLASH, WHACK))
    {
        PWSTR psz = NextChar(_pszWork + 1);
        fResult = ((*psz==SLASH) || (*psz==WHACK));
    }
    return fResult;
}

VOID URL::FeedServer(URL_STRING* pus)
{
    ASSERT(_pszWork);
    switch (_eScheme)
    {
    case URL_SCHEME_FILE:
        FeedFileServer(pus);
        break;

    case URL_SCHEME_MK:
        FeedMkServer(pus);
        break;

    case URL_SCHEME_FTP:
        FeedFtpServer(pus);
        break;

    default:
        FeedDefaultServer(pus);
        break;
    }
}

VOID URL::FeedMkServer(URL_STRING* pus)
{
    ASSERT(_pszWork);
     pus->EnableMunging();
    pus->Accept(L'@');
    PWSTR psz = FeedUntil(_pszWork, pus, SLASH);
    if (!*_pszWork)
    {
        pus->TrimEndWhiteSpace();
    }
    else
    {
        _pszWork = NextChar(psz+1);
    }
    pus->Accept(SLASH);
}

VOID URL::FeedLocalDrive(URL_STRING* pus)
{
    pus->Accept(*NextChar(_pszWork));
    _pszWork = NextChar(_pszWork+1);
    pus->Accept(*_pszWork);
    _pszWork = NextChar(_pszWork+1);
    pus->DisableMunging();
}

VOID URL::FeedFileServer(URL_STRING* pus)
{
    PWSTR psz = NextChar(_pszWork);

    // pus->BackupFlags();
    while (*psz==SLASH || *psz==WHACK)
    {
        psz = NextChar(psz+1);
    }

    DWORD dwSlashes = (DWORD)(psz - _pszWork);
    switch (dwSlashes)
    {
    case 4:
        pus->AddFlagNote(URL_ESCAPE_PERCENT | URL_ESCAPE_UNSAFE);
        _dwSchemeNotes |= UPF_FILEISPATHURL;
     // 4 to 6 slashes == 1 UNC
    case 2:
        if (IsLocalDrive(psz))
        {
            pus->AddFlagNote(URL_ESCAPE_PERCENT | URL_ESCAPE_UNSAFE);
        }

    case 5:
    case 6:
        pus->Accept(SLASH);
        pus->Accept(SLASH);
         if (!IsLocalDrive(psz))
        {
            pus->EnableMunging();
            psz = FeedUntil(psz, pus, SLASH, WHACK);
            if (!*psz)
            {
                pus->TrimEndWhiteSpace();
                Reset();
            }
            else
            {
                _pszWork = NextChar(psz+1);
            }
        }
        else
        {
            _pszWork = psz;
        }
        pus->Accept(SLASH);
        break;

    // If there are no slashes, then it can't be a UNC.
    case 0:
        if (IsLocalDrive(psz))
        {
            pus->AddFlagNote(URL_ESCAPE_PERCENT | URL_ESCAPE_UNSAFE);
        }


    // We think of "file:/" and "file:///" to be on the local machine
    // And if there are more slashes than we typically handle, screw it, we'll treat them as 1.
    case 1:
    case 3:
    // This is a not-good-case
    default:
        pus->Accept(SLASH);
        pus->Accept(SLASH);
        pus->Accept(SLASH);
        _pszWork = NextChar(psz);
        break;
    }
}


VOID URL::FeedFtpServer(URL_STRING* pus)
{
    ASSERT(_pszWork);

    PWSTR psz = NextChar(_pszWork);

    if (*psz==WHACK || *psz==SLASH)
    {
        pus->Accept(*psz);
        psz = NextChar(psz+1);
    }
    if (*psz==WHACK || *psz==SLASH)
    {
        pus->Accept(*psz);
        psz = NextChar(psz+1);
    }

    pus->EnableMunging();

    // The following is a grotesque and gruesome hack. We need to preserve case for
    // embedded username/password

    _pszWork = psz;

    BOOL fPossibleUserPasswordCombo = FALSE;
    while (*psz && *psz!=SLASH && *psz!=POUND && *psz!=QUERY)
    {
        if (*psz==L'@')
        {
            fPossibleUserPasswordCombo = TRUE;
            break;
        }
        psz = NextChar(psz+1);
    }

    psz = _pszWork;
    if (fPossibleUserPasswordCombo)
    {
        while (*psz!=L'@')
        {
            pus->Accept(*psz);
            psz = NextChar(psz+1);
        }
    }

    // This still leaves the issue of slashes, colons, ?s, @s, and #s in passwords; I guess they
    // ought to be escaped. (You just can't win, sometimes.)

    while (*psz && *psz!=SLASH && *psz!=COLON && *psz!=QUERY && *psz!=POUND)
    {
        pus->Accept(SmallForm(*psz));
        psz = NextChar(psz+1);
    }

    if (*psz==COLON)
    {
        psz = FeedPort(psz, pus);
    }
    pus->DisableMunging();

    _pszWork = psz;
    if (!*psz)
    {
        pus->TrimEndWhiteSpace();
        pus->Accept(SLASH);
    }
    else
    {
        if (*psz==QUERY || *psz==POUND)
        {
            pus->Accept(SLASH);
        }
        else
        {
            pus->Accept(*psz);
            _pszWork = NextChar(psz+1);
        }
    }
}


VOID URL::FeedDefaultServer(URL_STRING* pus)
{
    ASSERT(_pszWork);

    PWSTR psz = NextChar(_pszWork);
    if (!(_dwSchemeNotes & UPF_SCHEME_INTERNET))
    {
        pus->DisableSlashFixing();
    }

    if (*psz==WHACK || *psz==SLASH)
    {
        pus->Accept(*psz);
        psz = NextChar(psz+1);
    }
    if (*psz==WHACK || *psz==SLASH)
    {
        pus->Accept(*psz);
        psz = NextChar(psz+1);
    }
    if (_dwSchemeNotes & UPF_SCHEME_INTERNET)
    {
        pus->EnableMunging();

        while (*psz && *psz!=WHACK && *psz!=SLASH && *psz!=COLON && *psz!=QUERY && *psz!=POUND)
        {
            pus->Accept(SmallForm(*psz));
            psz = NextChar(psz+1);
        }
        if (*psz==COLON)
        {
            psz = FeedPort(psz, pus);
        }
        pus->DisableMunging();
    }
    else
    {
        while (*psz && *psz!=SLASH)
        {
            pus->Accept(*psz);
            psz = NextChar(psz+1);
        }
    }
    _pszWork = psz;
    if (!*psz)
    {
        pus->TrimEndWhiteSpace();
        if ((_eScheme!=URL_SCHEME_UNKNOWN) && !(_dwSchemeNotes & UPF_SCHEME_OPAQUE))
        {
            pus->Accept(SLASH);
        }
    }
    else
    {
        if (*psz==QUERY || *psz==POUND)
        {
            pus->Accept(SLASH);
        }
        else
        {
            pus->Accept(*psz);
            _pszWork = NextChar(psz+1);
        }
    }
}

PWSTR URL::FeedPort(PWSTR psz, URL_STRING* pus)
{
    BOOL fIgnorePort = FALSE;
    pus->Mark();
    psz = FeedUntil(psz, pus, SLASH, WHACK, POUND, QUERY);

    if (!(_dwFlags & URL_DONT_SIMPLIFY))
    {
        // Here, decide whether or not to ignore the port
        //  BUGBUG we should actually be getting this from
        //  the services file to find out the default protocol port
        //  but we dont think that most people will change them - zekel 17-Dec-96
        switch(_eScheme)
        {
        case URL_SCHEME_HTTP:
            if (pus->CompareMarkWith(L":80")==0)
                fIgnorePort = TRUE;
            break;

        case URL_SCHEME_HTTPS:
            if (pus->CompareMarkWith(L":443")==0)
                fIgnorePort = TRUE;
            break;

        case URL_SCHEME_FTP:
            if (pus->CompareMarkWith(L":21")==0)
                fIgnorePort = TRUE;
            break;

        case URL_SCHEME_GOPHER:
            if (pus->CompareMarkWith(L":70")==0)
                fIgnorePort = TRUE;
            break;
        }
    }
    if (fIgnorePort)
    {
        pus->EraseMarkedText();
    }
    else
    {
        pus->ClearMark();
    }
    return psz;
}

// -------------------------------------------------------------------------------

BOOL URL::DetectAbsolutePath()
{
    BOOL fResult = FALSE;
    if (_dwSchemeNotes & UPF_SCHEME_OPAQUE)
    {
        fResult = TRUE;
    }
    else if (DetectSymbols(SLASH, WHACK))
    {
        fResult = TRUE;
        _pszWork = NextChar(_pszWork+1);
    }
    return fResult;
}

BOOL URL::DetectPath()
{
    return (*NextChar(_pszWork) && !DetectSymbols(QUERY, POUND));
}

VOID URL::FeedPath(URL_STRING* pus, BOOL fMarkServer)
{
    ASSERT(_pszWork);
    PWSTR psz = NextChar(_pszWork);
    if (fMarkServer)
    {
        pus->Mark();
    }
    if (_dwSchemeNotes & UPF_SCHEME_OPAQUE)
    {
        _pszWork = FeedUntil(psz, pus);
        pus->TrimEndWhiteSpace();
    }
    else
    {
        DWORD cDots;
        BOOL fContinue = TRUE;
        do
        {
            cDots = 0;
            PWSTR pszTmp = psz;
            if (_fPathCompressionOn)
            {
                cDots = DetectDots(&psz);
            }

            if (cDots)
            {
                if (cDots==2)
                {
                    pus->Contract();
                }
                continue;
            }
            psz = CopySegment(pszTmp, pus, &fContinue);
        }
        while (fContinue);
        _pszWork = psz;
        if (!*_pszWork)
        {
            pus->TrimEndWhiteSpace();
        }
    }
}

// pfContinue indicates whether there's anything following that would
// be of relevance to a path
PWSTR URL::CopySegment(PWSTR psz, URL_STRING* pus, BOOL* pfContinue)
{
    ASSERT(pfContinue);
    BOOL fStop = FALSE;
    psz = NextChar(psz);
    while (!fStop)
    {
        switch (*psz)
        {
        case POUND:
            if (_eScheme==URL_SCHEME_FILE)
            {
                // Since #s are valid for dos paths, we have to accept them except
                // for when they follow a .htm/.html file (See FindFragmentA/W)
                 // However, some inconsistencies may still arise...
                for (DWORD i=0; i < ARRAYSIZE(ExtTable); i++)
                {
                    if (!pus->CompareLast(ExtTable[i].wszExt, ExtTable[i].cchExt))
                        break;
                }
                // If we haven't found a matching file extension, we'll treat as a filename character.
                if (i==ARRAYSIZE(ExtTable))
                {
                    pus->Accept(*psz);
                    psz = NextChar(psz+1);
                    break;
                }
            }
            goto next;

        case QUERY:
            // We're going to support query as a legitimate character in file urls.
            // *sigh*
             if (_eScheme==URL_SCHEME_FILE)
            {
                if (_fIgnoreQuery)
                {
                    psz = wszBogus;
                }
                else
                {
                    pus->CleanAccept(*psz);
                    psz = NextChar(psz+1);
                    break;
                }
            }
        case L'\0':
        next:
            *pfContinue = FALSE;
            fStop = TRUE;
            break;

        case SLASH:
        case WHACK:
            fStop = TRUE;
            // fall through

        default:
            pus->Accept(*psz);
            psz = NextChar(psz+1);
            break;
        }
    }
    return psz;
}

DWORD URL::DetectDots(PWSTR* ppsz)
{
    PWSTR psz;
    if (ppsz)
    {
        psz = *ppsz;
    }
    else
    {
        psz = NextChar(_pszWork);
    }

    DWORD cDots = 0;
    if (*psz==DOT)
    {
        psz = NextChar(psz+1);
        cDots++;
        if (*psz==DOT)
        {
            psz = NextChar(psz+1);
            cDots++;
        }
        switch (*psz)
        {
        case WHACK:
            if (_eScheme==URL_SCHEME_MK)
            {
                cDots = 0;
            }

        case SLASH:
            psz = NextChar(psz+1);
            break;

        case QUERY:
        case POUND:
        case L'\0':
            break;
         default:
            cDots = 0;
            break;
        }
    }
    if (ppsz)
    {
        *ppsz = psz;
    }
    return cDots;
}

VOID URL::StopPathCompression()
{
    _fPathCompressionOn = FALSE;
}


// -------------------------------------------------------------------------------

BOOL URL::DetectQueryOrFragment()
{
    return (DetectSymbols(QUERY, POUND));
}

BOOL URL::DetectQuery()
{
    return (DetectSymbols(QUERY));
}

VOID URL::IgnoreQuery()
{
    ASSERT(_eScheme==URL_SCHEME_FILE);
    _fIgnoreQuery = TRUE;
}

VOID URL::FeedQueryAndFragment(URL_STRING* pus)
{
    ASSERT(_pszWork);
    if (_dwSchemeNotes & UPF_SCHEME_OPAQUE)
    {
        PWSTR psz = NextChar(_pszWork);
        while (*psz)
        {
            pus->Accept(*psz);
            psz = NextChar(psz+1);
        }
        _pszWork = psz;
        return;
    }

    PWSTR psz = NextChar(_pszWork);

    // This is okay since *psz must equal { ? | # }
    if (*psz==QUERY)
    {
        pus->CleanAccept(QUERY);
    }

    // By munging, I mean taking an URL of form http://a/b#c?d and producing http://a/b?d#c
    // We do this by default; however, we won't do this when we've been passed a fragment only
    // as a relative url

    // Query's always override.

    if (*psz==QUERY)
    {
        pus->DropQuery();
        pus->NotifyQuery();
        pus->EnableMunging();

        psz = NextChar(psz+1);
        while (*psz)
        {
            if (*psz==POUND)
            {
                pus->NotifyFragment();
            }
            else
            {
                pus->Accept(*psz);
            }
            psz = NextChar(psz+1);
        }
    }
    else
    {
        // This line of code will determine whether we've been passed a fragment for a relative url
        // For properly formed base urls, this won't matter.
        BOOL fMunge = psz!=NextChar(_pszUrl);

        pus->DropFragment();
        pus->NotifyFragment();
        pus->EnableMunging();

        psz = NextChar(psz+1);

        // The following line is bogus. It just keeps going until the end. Not good.
        // We MAY or MAY NOT fix this, depending on how much people scream at me.
        // This may be an issue for Netscape compatibility.

        // What we could do is: when either query or fragment would be blank, preserve as is.
        // This would minimise breaking compatibility across the board.
        // -- AKABIR, 09/28/98
        while ((*psz==QUERY && !fMunge) || *psz)
        {
            if (*psz==QUERY)
            {
                pus->CleanAccept(QUERY);
            }
            else
            {
                pus->Accept(*psz);
            }
            psz = NextChar(psz+1);
        }

        if (*psz==QUERY)
        {
            pus->DropFragment();
            pus->NotifyQuery();
            pus->CleanAccept(*psz);
            psz = NextChar(psz+1);
            while (*psz)
            {
                pus->Accept(*psz);
                psz = NextChar(psz+1);
            }
            pus->TrimEndWhiteSpace();

            pus->NotifyFragment();
            psz = NextChar(_pszWork);
            pus->CleanAccept(*psz);
            psz = NextChar(psz+1);
            while (*psz!=QUERY)
            {
                pus->Accept(*psz);
                psz = NextChar(psz+1);
            }
        }
    }
    pus->TrimEndWhiteSpace();
    pus->ClearMark();
}

// -------------------------------------------------------------------------------

HRESULT
BlendUrls(URL& urlBase, URL& urlRelative, URL_STRING* pusOut, DWORD dwFlags)
{
    HRESULT hr = S_OK;

    // -- SCHEME --------------------------------------------------------------------------
    // Examine each url's scheme.
    // We won't continue to use urlBase IF
    // 1. their tokenized schemes are not identical
    // 2. the scheme is a file
    // 3. the actual string schemes are not identical

    //  this checks to make sure that these are the same scheme, and
    //  that the scheme is allowed to be used in relative URLs
    //  file: is not allowed to because of weirdness with drive letters
    //  and \\UNC\shares

    BOOL fBaseServerDetected = FALSE, fRelativeServerDetected = FALSE;
    BOOL fDetectAbsoluteRelPath = FALSE;
    BOOL fDetectedRelScheme = urlRelative.DetectAndFeedScheme(pusOut);
    BOOL fDetectedBaseScheme = FALSE;
    if (fDetectedRelScheme
        && ((pusOut->QueryScheme()==URL_SCHEME_FILE)
           || (urlRelative.GetSchemeNotes() & UPF_SCHEME_OPAQUE)))
    {
        urlBase.Reset();
    }
    else if ((fDetectedBaseScheme = urlBase.DetectAndFeedScheme(pusOut, fDetectedRelScheme)))
    {
        if (!fDetectedRelScheme)
        {
            urlRelative.SetScheme(urlBase.GetScheme(), urlBase.GetSchemeNotes());
        }
    }

    // We fall back on the original parser for those cases we don't handle yet.
    // (dwFlags & URL_FILE_USE_PATHURL) || (dwFlags & URL_WININET_COMPATIBILITY)
    if (((pusOut->QueryScheme()==URL_SCHEME_FILE)
         || (!(fDetectedRelScheme || fDetectedBaseScheme)))
        && ((dwFlags & URL_FILE_USE_PATHURL) || (dwFlags & URL_WININET_COMPATIBILITY)))
    {
        hr = E_FAIL;
        goto exit;
    }


    if ((pusOut->QueryScheme()==URL_SCHEME_UNKNOWN))
    {
        // BUG BUG For IE4 compat, we need to use the old parser. However
        // if we're passed URL_PLUGGABLE_PROTOCOL, we'll use this parser.

        if (!(dwFlags & URL_PLUGGABLE_PROTOCOL))
        {
            hr = E_FAIL;
            goto exit;
        }

        urlRelative.StopPathCompression();

            // Same schemes, so now we look at the base url to divine the opacity
        if (urlBase.DetectAnything() && !urlBase.IsReset())
        {
            if (!urlBase.DetectSlash())
            {
                if (!urlRelative.DetectQueryOrFragment())
                {
                    urlBase.Reset();
                }
                urlBase.AddSchemeNote(UPF_SCHEME_OPAQUE);
                urlRelative.AddSchemeNote(UPF_SCHEME_OPAQUE);
                pusOut->AddSchemeNote(UPF_SCHEME_OPAQUE);
            }
        }
        else if (!urlRelative.DetectSlash())
        {
            // If urlBase is reset, that means the schemes are different,
            // so we only have urlRelative to figure out opacity.

            urlRelative.AddSchemeNote(UPF_SCHEME_OPAQUE);
            pusOut->AddSchemeNote(UPF_SCHEME_OPAQUE);
        }
    }
    else if (pusOut->QueryScheme()==URL_SCHEME_FTP)
    {
        // For ftp urls, we'll assume that we're being passed properly formed urls.
        // Some ftp sites allow backslashes in their object filenames, so we should
        // allow access to these. Also, domain passwords would otherwise need escaping.
        pusOut->DisableSlashFixing();
    }

    if (dwFlags & URL_DONT_SIMPLIFY)
    {
        urlBase.StopPathCompression();
        urlRelative.StopPathCompression();
    }

    // -- SERVER --------------------------------------------------------------------------
    // Decide on the server to use.
    // Question: if urlBase and UrlRelative have the same explicit server, isn't it pointless
    // to continue looking at url base anyway?

    pusOut->EnableMunging();
    if (!(pusOut->GetSchemeNotes() & UPF_SCHEME_OPAQUE))
    {
        if (urlRelative.DetectServer()
            && !(urlBase.DetectServer() && (urlRelative.PeekNext()!=SLASH) && (urlRelative.PeekNext()!=WHACK)))
        {
            fRelativeServerDetected = TRUE;
            urlRelative.FeedServer(pusOut);
            urlBase.Reset();
        }
        else if (urlBase.DetectServer())
        {
            fBaseServerDetected = TRUE;
            urlBase.FeedServer(pusOut);
        }
    }

    // -- PATH ----------------------------------------------------------------------------
    // Figure out the path
    // If the relative url has a path, and it starts with a slash/whack, forget about the
    // base's path and stuff. Otherwise, inherit the base and attach the relative
    // Potential problem: when rel path is empty, we expect to knock of the last base segment

    if (pusOut->QueryScheme()==URL_SCHEME_FILE)
    {
        // Hack for back compat
        // If the relative url consists of a query string, we'll append that to
        // our resultant url, rather than the base's query string
        if (urlRelative.DetectQuery())
        {
            urlBase.IgnoreQuery();
        }
        else
        {

            BOOL fResult1 = urlRelative.DetectAbsolutePath();
            BOOL fResult2 = urlRelative.DetectLocalDrive();

            if (fResult2)
            {
                urlBase.Reset();
                urlRelative.FeedLocalDrive(pusOut);
                if (urlRelative.DetectAbsolutePath())
                {
                    pusOut->Accept(SLASH);
                }
            }
            else
            {
                if (urlBase.DetectLocalDrive())
                {
                    urlBase.FeedLocalDrive(pusOut);
                    if (fResult1)
                    {
                        pusOut->Accept(SLASH);
                        urlBase.Reset();
                    }
                    else if (urlBase.DetectAbsolutePath())
                    {
                        pusOut->Accept(SLASH);
                    }
                }
                else if (fResult1)
                {
                    if (fRelativeServerDetected)
                    {
                        pusOut->Accept(SLASH);
                    }
                    urlBase.Reset();
                }
            }
        }
    }
    else if (pusOut->QueryScheme()==URL_SCHEME_UNKNOWN)
    {
        if (pusOut->GetSchemeNotes() & UPF_SCHEME_OPAQUE)
        {
            if (!urlRelative.DetectAnything())
            {
                urlRelative.Reset();
            }
        }
        else
        {
        // This code fragment is for urls with unknown schemes, that are to be
        // treated hierarchically. Note that the authority (which has been passed in
        // already) is terminated with /, ?, or \0. The / is *optional*, and should be
        // appended if and only if the urls being combined call for it.
            if (urlBase.IsReset())
            {
            // At this point, we're examining only the relative url. We've been brought to
            // a stop by the presence of /, ? or \0. So
                if (urlRelative.DetectSlash() && !fDetectedRelScheme)
                {
                    pusOut->Accept(SLASH);
                }
            }
            else
            {
            // In this case, we have both the relative and base urls to look at.
            // What's the terminator for the base url
                if ((urlRelative.DetectSlash()
                        || (!urlBase.DetectAnything()
                           && urlRelative.DetectAnything()
                           && !urlRelative.DetectQuery()))
                    && !fDetectedRelScheme)
                {
                    pusOut->Accept(SLASH);
                }
            }
        }
    }

    pusOut->EnableMunging();

    if ((fBaseServerDetected && (fDetectAbsoluteRelPath = urlRelative.DetectAbsolutePath())))
    {
        if (!fRelativeServerDetected)
        {
            pusOut->RestoreFlags();
        }
        if (fDetectAbsoluteRelPath && urlRelative.DetectDots(NULL))
        {
            urlRelative.StopPathCompression();
        }
        urlRelative.FeedPath(pusOut);
        urlBase.Reset();
    }
    else if (urlBase.DetectPath())
    {
        urlBase.FeedPath(pusOut);
        // We don't want to contract the base path's free segment if
        // a. the scheme is opaque
        // b. the relative url has a path
        // c. the relative url has no path, just a fragment/query
        if (!(urlBase.GetSchemeNotes() & UPF_SCHEME_OPAQUE))
        {
            pusOut->RestoreFlags();

            if (urlRelative.DetectPath()
               || !urlRelative.DetectQueryOrFragment())
            {
                if (urlRelative.DetectPath() || !fDetectedRelScheme)
                {
                    pusOut->Contract(FALSE);
                }
                if (fDetectedRelScheme)
                {
                    urlRelative.StopPathCompression();
                }
                urlRelative.FeedPath(pusOut, FALSE);
                urlBase.Reset();
            }
            else
            {
                urlRelative.FeedPath(pusOut, FALSE);
            }
        }
        else
        {
            urlRelative.StopPathCompression();
            urlRelative.FeedPath(pusOut, FALSE);
        }
    }
    else if (urlRelative.DetectPath())
    {
        if (!fRelativeServerDetected)
        {
            pusOut->RestoreFlags();
        }
        else if (urlRelative.DetectDots(NULL))
        {
            urlRelative.StopPathCompression();
        }
        urlRelative.FeedPath(pusOut);
        urlBase.Reset();
    }
    pusOut->ClearMark();

    pusOut->DisableSlashFixing();
    // -- QUERY AND FRAGMENT -----------------------------------------------------------
    // Figure out the query
    if (urlBase.DetectQueryOrFragment())
    {
        urlBase.FeedQueryAndFragment(pusOut);
    }
    if (urlRelative.DetectQueryOrFragment())
    {
        urlRelative.FeedQueryAndFragment(pusOut);
    }
    pusOut->CleanAccept(L'\0');

    if (pusOut->AnyProblems())
    {
        hr = E_OUTOFMEMORY;
    }
exit:
    return hr;
}


HRESULT
FormUrlCombineResultW(LPCWSTR pszBase,
           LPCWSTR pszRelative,
           LPWSTR pszCombined,
           LPDWORD pcchCombined,
           DWORD dwFlags)
{
    if ((dwFlags & URL_ESCAPE_UNSAFE)
        && (dwFlags & URL_ESCAPE_SPACES_ONLY))
    {
    // In the original parser, ESCAPE_SPACES_ONLY takes precedence over ESCAPE_UNSAFE
    // Deactivate UNSAFE
        dwFlags ^= URL_ESCAPE_UNSAFE;
    }

    DWORD dwTempFlags = dwFlags;
    if (dwFlags & URL_UNESCAPE)
    {
        if (dwFlags & URL_ESCAPE_UNSAFE)
        {
            dwTempFlags ^= URL_ESCAPE_UNSAFE;
        }
        if (dwFlags & URL_ESCAPE_SPACES_ONLY)
        {
            dwTempFlags ^= URL_ESCAPE_SPACES_ONLY;
        }
    }

    // Make a copy of the relative url if the client wants to either
    // a. unescape and escape the URL (since roundtripping is not guaranteed), or
    // b. use the same location for relative URL's buffer for the combined url
    HRESULT hr;
    URL curlBase, curlRelative;
    curlBase.Setup((PWSTR)pszBase);
    curlRelative.Setup((PWSTR)pszRelative);
    URL_STRING us(dwTempFlags);

    hr = BlendUrls(curlBase, curlRelative, &us, dwTempFlags);

    if (SUCCEEDED(hr))
    {
        DWORD ccBuffer = us.GetTotalLength();
        if ((dwFlags & URL_UNESCAPE)
            && (dwFlags & (URL_ESCAPE_UNSAFE | URL_ESCAPE_SPACES_ONLY)))
        {
            // No need to strip out URL_UNESCAPE
            hr = UrlEscapeW(us.GetStart(), pszCombined, pcchCombined, dwFlags);
            goto exit;
        }
        if (ccBuffer > *pcchCombined)
        {
            hr = E_POINTER;
        }
        else if (pszCombined)
        {
            memcpy(pszCombined, us.GetStart(), ccBuffer*sizeof(WCHAR));
            // We return only the number of characters, not buffer size required.
            ccBuffer--;
        }
        *pcchCombined = ccBuffer;
    }
    else if (hr==E_FAIL)
    {
//        ASSERT(((dwFlags & URL_FILE_USE_PATHURL) || (dwFlags & URL_WININET_COMPATIBILITY)));

        // We fall back on the original parser for those cases we don't handle yet.
        // We should do this if and only if the new parser
        // doesn't handle the flags cited above
        // or we're passed a pluggable protocol without the forcing flag.
        SHSTRW strwOut;
        hr = SHUrlParse(pszBase, pszRelative, &strwOut, dwFlags);
        if(SUCCEEDED(hr))
        {
            hr = ReconcileHresults(hr, CopyOutW(&strwOut, pszCombined, pcchCombined));
        }
    }

exit:
    return hr;
}

HRESULT
FormUrlCombineResultA(LPCSTR pszBase,
           LPCSTR pszRelative,
           LPSTR pszCombined,
           LPDWORD pcchCombined,
           DWORD dwFlags)
{
    if ((dwFlags & URL_ESCAPE_UNSAFE)
        &&
        (dwFlags & URL_ESCAPE_SPACES_ONLY))
    {
    // In the original parser, ESCAPE_SPACES_ONLY takes precedence over ESCAPE_UNSAFE
    // Deactivate UNSAFE
        dwFlags ^= URL_ESCAPE_UNSAFE;
    }

    // Make a copy of the relative url if the client wants to either
    // a. unescape and escape the URL (since roundtripping is not guaranteed), or
    // b. use the same location for relative URL's buffer for the combined url
    SHSTRW strwBase;
    SHSTRW strwRelative;
    HRESULT hr;

    if (!(SUCCEEDED(strwBase.SetStr(pszBase)) && SUCCEEDED(strwRelative.SetStr(pszRelative))))
    {
        return E_OUTOFMEMORY;
    }

    DWORD dwTempFlags = dwFlags;
    if (dwFlags & URL_UNESCAPE)
    {
        if (dwFlags & URL_ESCAPE_UNSAFE)
        {
            dwTempFlags ^= URL_ESCAPE_UNSAFE;
        }
        if (dwFlags & URL_ESCAPE_SPACES_ONLY)
        {
            dwTempFlags ^= URL_ESCAPE_SPACES_ONLY;
        }
    }

    URL curlBase, curlRelative;
    curlBase.Setup((PWSTR)strwBase);
    curlRelative.Setup((PWSTR)strwRelative);
    URL_STRING us(dwTempFlags);

    hr = BlendUrls(curlBase, curlRelative, &us, dwTempFlags);

    if (SUCCEEDED(hr))
    {
        SHSTRA straOut;
        if ((dwFlags & URL_UNESCAPE)
            && (dwFlags & (URL_ESCAPE_UNSAFE | URL_ESCAPE_SPACES_ONLY)))
        {
            SHSTRW strwTemp;
            // No need to strip out URL_UNESCAPE
            hr = SHUrlEscape(us.GetStart(), &strwTemp, dwFlags);
            hr = ReconcileHresults(hr, straOut.SetStr((PWSTR)strwTemp));
        }
        else
        {
            hr = straOut.SetStr(us.GetStart());
        }
        if (SUCCEEDED(hr))
        {
            hr = CopyOutA(&straOut, pszCombined, pcchCombined);
        }
    }
    else if (hr==E_FAIL)
    {
//        ASSERT(((dwFlags & URL_FILE_USE_PATHURL) || (dwFlags & URL_WININET_COMPATIBILITY)));

        // We fall back on the original parser for those cases we don't handle yet.
        // We should do this if and only if the new parser
        // doesn't handle the flags cited above
        SHSTRW strwOut;

        hr = SHUrlParse((LPWSTR) strwBase, (LPWSTR)strwRelative, &strwOut, dwFlags);
        if (SUCCEEDED(hr))
        {
            SHSTRA straOut;
            hr = ReconcileHresults(hr, straOut.SetStr(strwOut));
            if(SUCCEEDED(hr))
                hr = ReconcileHresults(hr, CopyOutA(&straOut, pszCombined, pcchCombined));
        }
    }

    return hr;
}


#ifdef PROOFREAD_PARSES

EXTERN_C DWORD g_dwProofMode;

enum
{
    PP_COMPARE,
    PP_ORIGINAL_ONLY,
    PP_NEW_ONLY
};

//#define SHOW_MESSAGEBOX

VOID LogData(PWSTR pszMsg)
{
    SHSTRA str;
    str.SetStr(pszMsg);
    CHAR szFileName[MAX_PATH];
    DWORD dwSize = MAX_PATH;
    CHAR szComputerName[MAX_PATH];
    HANDLE hResultsFile = NULL;
    strcpy(szFileName, "\\\\BANYAN\\IPTD\\AKABIR\\1315\\");
    if (!GetComputerNameA(szComputerName, &dwSize))
    {
        goto exit;
    }
    lstrcatA(szFileName, szComputerName);
    hResultsFile = CreateFileA( szFileName,
                        GENERIC_WRITE,
                        FILE_SHARE_WRITE | FILE_SHARE_READ,
                        NULL,
                        OPEN_ALWAYS,
                        0,
                        NULL);
    if (INVALID_HANDLE_VALUE == hResultsFile)
        hResultsFile = NULL;
    if (hResultsFile)
    {
        if (SetFilePointer(hResultsFile, 0, NULL, FILE_END)==0xFFFFFFFF)
        {
            goto exit;
        }
        DWORD dwFoo;
        if (0==WriteFile(hResultsFile, (PVOID)(PSTR)str, lstrlenW(pszMsg), &dwFoo, NULL))
        {
            DWORD dwE = GetLastError();
        }
    }
exit:
    if (hResultsFile)
    {
        CloseHandle(hResultsFile);
    }
}

HRESULT ProofreadParses(HRESULT hr,
           LPCWSTR pszBase,
           LPCWSTR pszRelative,
           LPWSTR pszCombined,
           PDWORD pcchCombined,
           DWORD dwFlags,
           DWORD dwSize
           )
{
    static WCHAR szLast[2084];
    SHSTRW strwOut;

switch(g_dwProofMode)
{
case PP_COMPARE:
    {
    HRESULT hr2 = SHUrlParse(pszBase, pszRelative, &strwOut, dwFlags);
    WCHAR wstr[2084];
    DWORD ccLen = min(2084, dwSize), ccUrl = SUCCEEDED(hr) ? *pcchCombined : 0;
    if(SUCCEEDED(hr2))
    {
        hr2 = CopyOutW(&strwOut, wstr, &ccLen);
        if (hr2 == E_POINTER && hr == E_POINTER)
        {
            goto exitpoint;
        }
        // Check if cached combine equals the new parser's result
        if (!StrCmpW(pszCombined, szLast))
        {
            goto exitpoint;
        }
        // Check if cached combine equals the old parser's result
        if (!StrCmpW(wstr, szLast))
        {
            *pcchCombined = ccLen;
            StrCpyNW(pszCombined, wstr, ccLen + 1);
            hr = hr2;
            goto exitpoint;
        }
        if (SUCCEEDED(hr))
        {
            StrCpyNW(szLast, wstr, ccLen);

            if (!StrCmpW(wstr, pszCombined))
            {
                goto exitpoint;
            }

            DWORD dwBogus;
            if ((dwFlags & URL_ESCAPE_SPACES_ONLY) && !(dwFlags & URL_UNESCAPE))
            {
                PCWSTR psz = FindSchemeW(pszCombined, &dwBogus);
                DWORD dw;
                if (psz
                    &&
                    (URL_SCHEME_UNKNOWN
                        !=GetSchemeTypeAndFlagsW(psz, dwBogus, &dw))
                    &&
                    (dw & UPF_SCHEME_OPAQUE))
                {
                    goto exitpoint;
                }
            }

            // Filter
            // base: "http://foo/bar/"
            // rel:  ""
            // old:  "http://foo/bar"
            // new:  "http://foo/bar/"
            if ((*pszRelative==L'\0')
                &&
                (!StrCmpNW(pszCombined, wstr, ccLen))
                &&
                (ccUrl==(ccLen+1))
                &&
                (pszCombined[ccLen]==L'/'))
            {
                goto exitpoint;
            }

            // Filter
            // base: "http://foo/bar/what?ho"
            // rel:  ""
            // old:  "http://foo/bar/?ho"
            // new:  "http://foo/bar/"
            if ((*pszRelative==L'\0')
                &&
                (!StrCmpNW(pszCombined, wstr, ccUrl))
                &&
                (wstr[ccUrl]==QUERY))
            {
                goto exitpoint;
            }

            // Filter
            // base: "http://foo/bar/what?ho"
            // rel:  "/"
            // old:  "http://foo"
            // new:  "http://foo/"
            if ((*pszRelative==L'/')
                &&
                (!StrCmpNW(pszCombined, wstr, ccLen))
                &&
                (ccUrl==(ccLen+1))
                &&
                (pszCombined[ccLen]==L'/'))
            {
                goto exitpoint;
            }

            WCHAR wmsg[8192];
            wnsprintfW(wmsg,
                    ARRAYSIZE(wmsg),
                    L"Flags:%#x\nBase:\"%s\"\nRelative:\"%s\"\nOriginal result:\"%s\"\nNew result:\"%s\"\nUse original, not new, result?\n",
                    dwFlags,
                    pszBase,
                    pszRelative,
                    wstr,
                    pszCombined
                    );
#ifdef SHOW_MESSAGEBOX
            if (IDYES==MessageBoxW(
                NULL,
                wmsg,
                L"CONTACT AKABIR: URLCOMBINE FAILURE",
                MB_YESNO | MB_ICONERROR | MB_TASKMODAL))
            {
                StrCpyNW(pszCombined, wstr, dwSize);
                *pcchCombined = ccLen;
            }
            else
            {
                StrCpyNW(szLast, pszCombined, *pcchCombined);
            }
#endif
            LogData(wmsg);
        }
        else
        {
            WCHAR wmsg[8192];
            wnsprintfW(wmsg,
                    ARRAYSIZE(wmsg),
                    L"Flags:%#x\nBase:\"%s\"\nRelative:\"%s\"\nFAILED:%#x\nExpected:\"%s\"\n",
                    dwFlags,
                    pszBase,
                    pszRelative,
                    hr,
                    wstr);
#ifdef SHOW_MESSAGEBOX
            MessageBoxW(
                NULL,
                wmsg,
                L"CONTACT AKABIR: URLCOMBINE FAILURE",
                MB_OK | MB_ICONERROR | MB_TASKMODAL);
#endif
            StrCpyNW(pszCombined, wstr, dwSize);
            *pcchCombined = ccLen;
            LogData(wmsg);
        }
        hr = hr2;
    }
    }
    break;

case PP_NEW_ONLY:
    break;

case PP_ORIGINAL_ONLY:
    {
    hr = SHUrlParse(pszBase, pszRelative, &strwOut, dwFlags);
    if(SUCCEEDED(hr))
    {
        hr = CopyOutW(&strwOut, pszCombined, pcchCombined);
    }
    }
    break;
    }

exitpoint:
    return hr;
}

#endif //PROOFREAD_PARSES

LWSTDAPI
UrlCombineW(LPCWSTR pszBase,
           LPCWSTR pszRelative,
           LPWSTR pszCombined,
           LPDWORD pcchCombined,
           DWORD dwFlags)
{
    HRESULT hr = E_INVALIDARG;

    if (pszBase && pszRelative && pcchCombined)
    {
        RIP(IS_VALID_STRING_PTRW(pszBase, INTERNET_MAX_PATH_LENGTH));
        RIP(IS_VALID_STRING_PTRW(pszRelative, INTERNET_MAX_PATH_LENGTH));
        RIP(IS_VALID_WRITE_PTR(pcchCombined, DWORD));
        RIP((!pszCombined || IS_VALID_WRITE_BUFFER(pszCombined, WCHAR, *pcchCombined)));

#ifdef PROOFREAD_PARSES
        DWORD dwSize = *pcchCombined;
#endif

        hr = FormUrlCombineResultW(pszBase, pszRelative, pszCombined, pcchCombined, dwFlags);

#ifdef PROOFREAD_PARSES
        hr = ProofreadParses(hr, pszBase, pszRelative, pszCombined, pcchCombined, dwFlags, dwSize);
#endif
    }
    return hr;
}

LWSTDAPI
UrlCombineA(LPCSTR pszBase,
           LPCSTR pszRelative,
           LPSTR pszOut,
           LPDWORD pcchOut,
           DWORD dwFlags)
{
    HRESULT hr;

    if (!pszBase
        || !pszRelative
        || !pcchOut)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        RIP(IS_VALID_STRING_PTRA(pszBase, INTERNET_MAX_PATH_LENGTH));
        RIP(IS_VALID_STRING_PTRA(pszRelative, INTERNET_MAX_PATH_LENGTH));
        RIP(IS_VALID_WRITE_PTR(pcchOut, DWORD));
        RIP((!pszOut || IS_VALID_WRITE_BUFFER(pszOut, CHAR, *pcchOut)));

        hr = FormUrlCombineResultA(pszBase, pszRelative, pszOut, pcchOut, dwFlags);
    }

    return hr;
}

#else // end USE_FAST_PARSER

LWSTDAPI
UrlCombineW(LPCWSTR pszBase,
           LPCWSTR pszRelative,
           LPWSTR pszCombined,
           LPDWORD pcchCombined,
           DWORD dwFlags)
{
    HRESULT hr = E_INVALIDARG;

    if (pszBase && pszRelative && pcchCombined)
    {
        SHSTRW strwOut;
        hr = SHUrlParse(pszBase, pszRelative, &strwOut, dwFlags);
        if(SUCCEEDED(hr))
        {
            hr = CopyOutW(&strwOut, pszCombined, pcchCombined);
        }
    }
    return hr;
}

LWSTDAPI
UrlCombineA(LPCSTR pszBase,
           LPCSTR pszRelative,
           LPSTR pszOut,
           LPDWORD pcchOut,
           DWORD dwFlags)
{
    HRESULT hr;
    SHSTRA straOut;

    if (!pszBase || !pszRelative || !pcchOut)
        hr = E_INVALIDARG;
    else
    {
        SHSTRW strwOut;
        SHSTRW strwBase;
        SHSTRW strwRelative;

        if(SUCCEEDED(strwBase.SetStr(pszBase)) && SUCCEEDED(strwRelative.SetStr(pszRelative)))
            hr = SHUrlParse((LPWSTR) strwBase, (LPWSTR)strwRelative, &strwOut, dwFlags);
        else
            hr = E_OUTOFMEMORY;

        if(SUCCEEDED(hr))
            hr = straOut.SetStr(strwOut);
    }

    if(SUCCEEDED(hr) )
        hr = CopyOutA(&straOut, pszOut, pcchOut);

    return hr;
}

#endif // !USE_FAST_PARSER


//
// Combines the desired scheme with the string after the scheme with a : in between.  For
// some protocols, a // is placed after the colon.
//

PRIVATE HRESULT ColonSlashSlashW
(
    LPCWSTR pszScheme,       // url protocol (lower-case)
    LPCWSTR pszAfterScheme,  // string to append after the protocol
    LPWSTR pszTranslatedUrl, // output buffer
    int cchMax               // size of output buffer
)
{
    StrCpyNW(pszTranslatedUrl, pszScheme, cchMax);

    // Append : after scheme and possibly a // as well.
    int cchScheme = lstrlenW(pszScheme);
    if (cchMax > cchScheme + 3)
    {
        pszTranslatedUrl[cchScheme] = L':';

        // Number of characters to skip over in the buffer (how many non alphanums originally
        // followed the protocol)
        int cchSkip = 0;

        // Number of characters past the protocol: to skip over in the URL (Do we insert slashes?)
        int cchSlashes = 0;


        // Modify this conditional to include any other protocols to always follow with ://
        // Right now, http, https and ftp are automatic
        if (!StrCmpW(pszScheme, L"http") || !StrCmpW(pszScheme, L"ftp") || !StrCmpW(pszScheme, L"https") )
        {
            //
            // When preparing to copy the contents of pszAfterScheme into pszUrl, we can
            // skip over as many as 3 non alpha numeric characters, since we are adding ://
            // to the protocol directly
            //
            while ((cchSkip < 3) && pszAfterScheme[cchSkip] && !IsCharAlphaNumericW(pszAfterScheme[cchSkip]))
            {
                cchSkip++;
            }
            pszTranslatedUrl[cchScheme+1] = L'/';
            pszTranslatedUrl[cchScheme+2] = L'/';
            pszTranslatedUrl[cchScheme+3] = L'\0';
            cchSlashes = 2;
        }
        else
        // some other protocol
        {
            // just skip over colon
            cchSkip = 1;
            pszTranslatedUrl[cchScheme+1] = L'\0';

        }

        // Copy the rest of the Url from the UrlBuffer into the Url
        StrCatBuffW(pszTranslatedUrl, pszAfterScheme + cchSkip, cchMax);
    }

    return S_OK;
}

//
// Scans the url for a scheme and if it does not match the known schemes, then
// a closest match is found.
//
LWSTDAPI
UrlFixupW
(
    LPCWSTR pcszUrl,         // URL to correct
    LPWSTR pszTranslatedUrl, // buffer for corrected url (can be same as pcszUrl)
    DWORD cchMax             // size of pszTranslatedUrl
    )
{
    HRESULT hr = S_OK;

    //
    // Find the scheme
    //
    WCHAR szScheme[INTERNET_MAX_SCHEME_LENGTH];
    ULONG cchScheme = 0;
    LPCWSTR pszScheme = FindSchemeW(pcszUrl, &cchScheme, TRUE);
    if (NULL == pszScheme || cchScheme > (ARRAYSIZE(szScheme)-1))
    {
        // No scheme found
        return S_FALSE;
    }

    for (ULONG cch=0; cch < cchScheme; ++cch, ++pszScheme)
    {
        szScheme[cch] = Ascii_ToLowerW(*pszScheme);
    }
    szScheme[cch] = L'\0';
    LPCWSTR pszAfterScheme = pszScheme;

    //
    // If input and output buffers are the same, copy the stuff after the scheme
    // to another buffer so it doesn't get clobbered when we recombine.
    //
    WCHAR szBuf[INTERNET_MAX_PATH_LENGTH];
    if (pcszUrl == pszTranslatedUrl)
    {
        StrCpyNW(szBuf, pszAfterScheme, ARRAYSIZE(szBuf));
        pszAfterScheme = szBuf;
    }

    //
    // See if it matches any of our known schemes
    //
    BOOL fKnownScheme = FALSE;
    for (ULONG i = 0; i < ARRAYSIZE(g_mpUrlSchemeTypes); ++i)
    {
        if (StrCmpW(szScheme, g_mpUrlSchemeTypes[i].pszScheme) == 0)
        {
            fKnownScheme = TRUE;
            break;
        }
    }

    //
    // If it matches a known scheme, then just fix :// if it's ftp or http
    //
    if (fKnownScheme ||

        // Check for pluggable protocols too
        NO_ERROR == SHGetValueW(HKEY_CLASSES_ROOT, szScheme, L"URL Protocol",
                               NULL, NULL, NULL))
    {
        ColonSlashSlashW(szScheme, pszAfterScheme, pszTranslatedUrl, cchMax);
        return S_OK;
    }

    //
    // Try to find a good match for the mispelled scheme
    //

    // These are weights used in the heuristic for the protocol matching
    // iFloor is roughly the minimum percentage of characters that must match in
    // order to make a change
    const int cFloor = 60;

    // A match in the first character has the greatest weight
    const int cCorrectFirstChar = 150;

    // Any other matched character
    const int cCorrectChar = 100;

    // The weight given to a character that only matches the preceding
    // or subsequent character in the protocol
    const int cOffByOneChar = 80;

    // We penalize characters that are off by one, but if we have already
    // observed the offset and subsequent characters continue the offset, we add this
    const int cOffsetBonus = 20;

    // The value of the best "match" found so far. Higher is a better match.
    int iBestEval = 0;

    // The protocol that's the best fit for the misspelled one
    LPCWSTR pszBestMatch = NULL;

    ULONG cchProt;
    for (ULONG j = 0; j < ARRAYSIZE(g_mpUrlSchemeTypes); ++j)
    {
        // Is this one we don't correct to?
        //
        // Note:  https is removed from this list.  The potential for an intended "http" to
        // be corrected to "https" is too high, and "http" is far more common.  All this
        // means is that if someone wants to get to an https site, they have to have it right.
        //
        if (IsFlagSet(g_mpUrlSchemeTypes[j].dwFlags, UPF_SCHEME_DONTCORRECT))
            continue;

        LPCWSTR pszProtocol = g_mpUrlSchemeTypes[j].pszScheme;
        cchProt = g_mpUrlSchemeTypes[j].cchScheme;

        // Evaluation of the fit of the currently tested protocol
        int iEval = 0;

        //
        // Keep track of the positive or negative offset in the protocol
        // such as "qhttp" instead of "http" or "elnet" instead of "telnet'
        //
        int iPosOffset = 0;
        int iNegOffset = 0;

        //
        // The first character has the most weight. "htp" corrects
        // to "http" and not "ftp"    "ftt" corrects to "ftp"
        //
        if (*szScheme == *pszProtocol)
        {
            iEval += cCorrectFirstChar;
        }

        // Check for a negative offset
        else if(*szScheme == pszProtocol[1])
        {
            iEval += cOffByOneChar;
            iNegOffset = 1;
        }

        //
        // We go through the characters in the protocol, even to the
        // terminating null if iPosOffset == 1 (it is never more than 1)
        // This is so the final "p" in "qhttp" gets a chance to be compared
        //
        for (i=1; i < cchProt + iPosOffset; i++)
        {
            // No points for null terminations matching
            if (szScheme[i] == L'\0')
                break;

            //
            // Check for adjacent character match
            //
            if (szScheme[i] == pszProtocol[i])
            {
                iEval += cCorrectChar;
            }
            else
            {
                if (szScheme[i] == pszProtocol[i - 1])
                {
                    iEval += cOffByOneChar;
                    if (iPosOffset)
                        iEval += cOffsetBonus;
                    else
                        iPosOffset = 1;
                }
                else
                {
                    if(szScheme[i] == pszProtocol[i + 1])
                    {
                        iEval += cOffByOneChar;
                        if (iNegOffset)
                            iEval += cOffsetBonus;
                        else
                            iNegOffset = 1;
                    }
                }
            }
        }

        // Divide the Evaluated value by the MAX(cchScheme, cchProt)
        iEval = iEval / (cchScheme > cchProt ? cchScheme : cchProt);

        // A new best match?
        if (iEval > iBestEval)
        {
            iBestEval = iEval;
            pszBestMatch = pszProtocol;

            //
            // If we found an unquestionably good match (only 1 non-firstchar typo),
            // break out of the loop
            //
            if (iEval >= 100)
                break;
         }
    }

    // If a good enough match was found, then correct url
    if (iBestEval >= cFloor)
    {
        ColonSlashSlashW(pszBestMatch, pszAfterScheme, pszTranslatedUrl,cchMax);
    }
    else
    {
        hr = S_FALSE;
    }
    return hr;
}

