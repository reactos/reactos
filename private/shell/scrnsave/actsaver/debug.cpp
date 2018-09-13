/////////////////////////////////////////////////////////////////////////////
// DEBUG.CPP
//
// Debugging functions.
//
// History:
//
// Author   Date        Description
// ------   ----        -----------
// jaym     04/26/97    Stolen from \nt\private\shell\lib
// jaym     04/26/97    Removed try/except
/////////////////////////////////////////////////////////////////////////////
#include "precomp.h"

// Define some things for debug.h
//
#define SZ_DEBUGINI     "actsaver.ini"
#define SZ_DEBUGSECTION "actsaver"
#define SZ_MODULE       "ACTSAVER"
#define DECLARE_DEBUG
#include <debug.h>

#ifdef DEBUG

DWORD g_dwDumpFlags     = 0;        // DF_*
#ifdef FULL_DEBUG
DWORD g_dwBreakFlags    = BF_ONVALIDATE;        // BF_*
#else
DWORD g_dwBreakFlags    = 0;        // BF_*
#endif
DWORD g_dwTraceFlags    = 0;        // TF_*
DWORD g_dwPrototype     = 0;        
DWORD g_dwFuncTraceFlags = 0;       // FTF_*

// TLS slot used to store depth for CcshellFuncMsg indentation

static DWORD g_tlsStackDepth = TLS_OUT_OF_INDEXES;

// Hack stack depth counter used when g_tlsStackDepth is not available

static DWORD g_dwHackStackDepth = 0;

static char g_szIndentLeader[] = "                                                                                ";

static WCHAR g_wszIndentLeader[] = L"                                                                                ";


#pragma data_seg(DATASEG_READONLY)

static CHAR const FAR c_szNewline[] = "\r\n";   // (Deliberately CHAR)
static WCHAR const FAR c_wszNewline[] = TEXTW("\r\n");

#pragma data_seg()

extern CHAR const FAR c_szTrace[];              // (Deliberately CHAR)
extern CHAR const FAR c_szErrorDbg[];           // (Deliberately CHAR)
extern CHAR const FAR c_szWarningDbg[];         // (Deliberately CHAR)
extern WCHAR const FAR c_wszTrace[];
extern WCHAR const FAR c_wszErrorDbg[]; 
extern WCHAR const FAR c_wszWarningDbg[];

extern const CHAR  FAR c_szAssertMsg[];
extern CHAR const FAR c_szAssertFailed[];
extern const WCHAR  FAR c_wszAssertMsg[];
extern WCHAR const FAR c_wszAssertFailed[];


void
SetPrefixStringA(
    OUT LPSTR pszBuf,
    IN  DWORD dwFlags)
{
    if (TF_ALWAYS == dwFlags)
        lstrcpyA(pszBuf, c_szTrace);
    else if (IsFlagSet(dwFlags, TF_WARNING))
        lstrcpyA(pszBuf, c_szWarningDbg);
    else if (IsFlagSet(dwFlags, TF_ERROR))
        lstrcpyA(pszBuf, c_szErrorDbg);
    else
        lstrcpyA(pszBuf, c_szTrace);
}


void
SetPrefixStringW(
    OUT LPWSTR pszBuf,
    IN  DWORD  dwFlags)
{
    if (TF_ALWAYS == dwFlags)
        lstrcpyW(pszBuf, c_wszTrace);
    else if (IsFlagSet(dwFlags, TF_WARNING))
        lstrcpyW(pszBuf, c_wszWarningDbg);
    else if (IsFlagSet(dwFlags, TF_ERROR))
        lstrcpyW(pszBuf, c_wszErrorDbg);
    else
        lstrcpyW(pszBuf, c_wszTrace);
}


// Hack!  The MSDEV debugger has some smarts where if it sees
// an ASSERT (all caps) in the source, and there is a debug break,
// then it sticks up a sorta friendly assert message box.
// For the debug function below where the break occurs inside,
// we add a nop ASSERT line in here to fake MSDEV to give us
// a friendly message box.

#undef ASSERT
#define ASSERT(f)   DEBUG_BREAK


//
//  Smart debug functions
//



/*----------------------------------------------------------
Purpose: Displays assertion string.

Returns: TRUE to debugbreak
Cond:    --
*/
BOOL
CcshellAssertFailedA(
    LPCSTR pszFile,
    int line,
    LPCSTR pszEval,
    BOOL bBreakInside)
{
    BOOL bRet = FALSE;
    LPCSTR psz;
    CHAR ach[256];

    // Strip off path info from filename string, if present.
    //
    for (psz = pszFile + lstrlenA(pszFile); psz != pszFile; psz=CharPrevA(pszFile, psz))
    {
        if ((CharPrevA(pszFile, psz)!= (psz-2)) && *(psz - 1) == '\\')
            break;
    }
    wsprintfA(ach, c_szAssertFailed, psz, line, pszEval);
    OutputDebugStringA(ach);

    if (IsFlagSet(g_dwBreakFlags, BF_ONVALIDATE))
    {
        if (bBreakInside)
        {
            // See the hack we have above about redefining ASSERT
            ASSERT(0);
        }
        else
            bRet = TRUE;
    }

    return bRet;
}


/*----------------------------------------------------------
Purpose: Displays assertion string.

Returns: --
Cond:    --
*/
BOOL
CcshellAssertFailedW(
    LPCWSTR pszFile,
    int line,
    LPCWSTR pszEval,
    BOOL bBreakInside)
{
    BOOL bRet = FALSE;
    LPCWSTR psz;
    WCHAR ach[256];

    // Strip off path info from filename string, if present.
    //
    for (psz = pszFile + lstrlenW(pszFile); psz && (psz != pszFile); psz=CharPrevW(pszFile, psz))
    {
        if ((CharPrevW(pszFile, psz)!= (psz-2)) && *(psz - 1) == TEXT('\\'))
            break;
    }

    // If psz == NULL, CharPrevW failed which implies we are running on Win95.  We can get this
    // if we get an assert in some of the W functions in shlwapi...  Call the A version of assert...
    if (!psz)
    {
        char szFile[MAX_PATH];
        char szEval[256];   // since the total output is thhis size should be enough...

        WideCharToMultiByte(CP_ACP, 0, pszFile, -1, szFile, ARRAYSIZE(szFile), NULL, NULL);
        WideCharToMultiByte(CP_ACP, 0, pszEval, -1, szEval, ARRAYSIZE(szEval), NULL, NULL);
        return CcshellAssertFailedA(szFile, line, szEval, bBreakInside);
    }

    wsprintfW(ach, c_wszAssertFailed, psz, line, pszEval);
    OutputDebugStringW(ach);

    if (IsFlagSet(g_dwBreakFlags, BF_ONVALIDATE))
    {
        if (bBreakInside)
        {
            // See the hack we have above about redefining ASSERT
            ASSERT(0);
        }
        else
            bRet = TRUE;
    }

    return bRet;
}


/*----------------------------------------------------------
Purpose: Keep track of the stack depth for function call trace
         messages.

Returns: --
Cond:    --
*/
void
CcshellStackEnter(void)
    {
    if (TLS_OUT_OF_INDEXES != g_tlsStackDepth)
        {
        DWORD dwDepth;

        dwDepth = PtrToUlong(TlsGetValue(g_tlsStackDepth));

        TlsSetValue(g_tlsStackDepth, (LPVOID)(dwDepth + 1));
        }
    else
        {
        g_dwHackStackDepth++;
        }
    }


/*----------------------------------------------------------
Purpose: Keep track of the stack depth for functionc all trace
         messages.

Returns: --
Cond:    --
*/
void
CcshellStackLeave(void)
    {
    if (TLS_OUT_OF_INDEXES != g_tlsStackDepth)
        {
        DWORD dwDepth;

        dwDepth = PtrToUlong(TlsGetValue(g_tlsStackDepth));

        if (EVAL(0 < dwDepth))
            {
            EVAL(TlsSetValue(g_tlsStackDepth, (LPVOID)(dwDepth - 1)));
            }
        }
    else
        {
        if (EVAL(0 < g_dwHackStackDepth))
            {
            g_dwHackStackDepth--;
            }
        }
    }


/*----------------------------------------------------------
Purpose: Return the stack depth.

Returns: see above
Cond:    --
*/
static
DWORD
CcshellGetStackDepth(void)
    {
    DWORD dwDepth;

    if (TLS_OUT_OF_INDEXES != g_tlsStackDepth)
        {
        dwDepth = PtrToUlong(TlsGetValue(g_tlsStackDepth));
        }
    else
        {
        dwDepth = g_dwHackStackDepth;
        }

    return dwDepth;
    }


/*----------------------------------------------------------
Purpose: This function converts a multi-byte string to a
         wide-char string.

         If pszBuf is non-NULL and the converted string can fit in
         pszBuf, then *ppszWide will point to the given buffer.
         Otherwise, this function will allocate a buffer that can
         hold the converted string.

         If pszAnsi is NULL, then *ppszWide will be freed.  Note
         that pszBuf must be the same pointer between the call
         that converted the string and the call that frees the
         string.

Returns: TRUE
         FALSE (if out of memory)

Cond:    --
*/
BOOL
UnicodeFromAnsi(
    LPWSTR * ppwszWide,
    LPCSTR pszAnsi,           // NULL to clean up
    LPWSTR pwszBuf,
    int cchBuf)
    {
    BOOL bRet;

    // Convert the string?
    if (pszAnsi)
        {
        // Yes; determine the converted string length
        int cch;
        LPWSTR pwsz;
        int cchAnsi = lstrlenA(pszAnsi)+1;

        cch = MultiByteToWideChar(CP_ACP, 0, pszAnsi, cchAnsi, NULL, 0);

        // String too big, or is there no buffer?
        if (cch > cchBuf || NULL == pwszBuf)
            {
            // Yes; allocate space
            cchBuf = cch + 1;
            pwsz = (LPWSTR)LocalAlloc(LPTR, CbFromCchW(cchBuf));
            }
        else
            {
            // No; use the provided buffer
            pwsz = pwszBuf;
            }

        if (pwsz)
            {
            // Convert the string
            cch = MultiByteToWideChar(CP_ACP, 0, pszAnsi, cchAnsi, pwsz, cchBuf);
            bRet = (0 < cch);
            }
        else
            {
            bRet = FALSE;
            }

        *ppwszWide = pwsz;
        }
    else
        {
        // No; was this buffer allocated?
        if (*ppwszWide && pwszBuf != *ppwszWide)
            {
            // Yes; clean up
            LocalFree((HLOCAL)*ppwszWide);
            *ppwszWide = NULL;
            }
        bRet = TRUE;
        }

    return bRet;
    }


/*----------------------------------------------------------
Purpose: Wide-char version of CcshellAssertMsgA
Returns: --
Cond:    --
*/
void
CDECL
CcshellAssertMsgW(
    BOOL f,
    LPCSTR pszMsg, ...)
{
    va_list vArgs;

    if (!f)
    {
        WCHAR * ach = new WCHAR [1024+40];

        if (ach == NULL)
            return;

        lstrcpyW(ach, c_wszAssertMsg);
        int cch = lstrlenW(ach);
        va_start(vArgs, pszMsg);

        // (We convert the string, rather than simply input an
        // LPCWSTR parameter, so the caller doesn't have to wrap
        // all the string constants with the TEXT() macro.)

        LPWSTR  pwsz;
        WCHAR   wszBuf[1024];
        if (UnicodeFromAnsi(&pwsz, pszMsg, wszBuf, SIZECHARS(wszBuf)))
        {
            wvsprintfW(&ach[cch], pwsz, vArgs);
            UnicodeFromAnsi(&pwsz, NULL, wszBuf, 0);
        }

        va_end(vArgs);
        OutputDebugStringW(ach);
        OutputDebugStringW(c_wszNewline);

        if (IsFlagSet(g_dwBreakFlags, BF_ONVALIDATE))
            ASSERT(0);

        delete [] ach;
    }
}


/*----------------------------------------------------------
Purpose: Wide-char version of CcshellDebugMsgA.  Note this
         function deliberately takes an ANSI format string
         so our trace messages don't all need to be wrapped
         in TEXT().

Returns: --
Cond:    --
*/
void
CDECL
CcshellDebugMsgW(
    DWORD flag,
    LPCSTR pszMsg, ...)         // (this is deliberately CHAR)
{
    va_list vArgs;

    if  (
        (TF_ALWAYS == flag)
        ||
        (IsFlagSet(g_dwTraceFlags, flag) && flag)
        )
    {
        WCHAR * ach = new WCHAR [1024+40];

        if (ach == NULL)
            return;

        SetPrefixStringW(ach, flag);
        int cch = lstrlenW(ach);
        va_start(vArgs, pszMsg);

        // (We convert the string, rather than simply input an
        // LPCWSTR parameter, so the caller doesn't have to wrap
        // all the string constants with the TEXT() macro.)

        LPWSTR  pwsz;
        WCHAR   wszBuf[1024];
        if (UnicodeFromAnsi(&pwsz, pszMsg, wszBuf, SIZECHARS(wszBuf)))
        {
            wvsprintfW(&ach[cch], pwsz, vArgs);
            UnicodeFromAnsi(&pwsz, NULL, wszBuf, 0);
        }

        va_end(vArgs);
        OutputDebugStringW(ach);
        OutputDebugStringW(c_wszNewline);

        if (TF_ALWAYS != flag &&
            ((flag & TF_ERROR) && IsFlagSet(g_dwBreakFlags, BF_ONERRORMSG) ||
             (flag & TF_WARNING) && IsFlagSet(g_dwBreakFlags, BF_ONWARNMSG)))
        {
            DEBUG_BREAK;
        }

        delete [] ach;
    }

}


/*----------------------------------------------------------
Purpose: Wide-char version of CcshellFuncMsgA.  Note this
         function deliberately takes an ANSI format string
         so our trace messages don't all need to be wrapped
         in TEXT().

Returns: --
Cond:    --
*/
void
CDECL
CcshellFuncMsgW(
    DWORD flag,
    LPCSTR pszMsg, ...)         // (this is deliberately CHAR)
{
    va_list vArgs;

    if  (
        IsFlagSet(g_dwTraceFlags, TF_FUNC)
        &&
        IsFlagSet(g_dwFuncTraceFlags, flag)
        )
    {
        WCHAR * ach = new WCHAR [1024+40];

        if (ach == NULL)
            return;

        // Determine the indentation for trace message based on
        // stack depth.

        LPWSTR  pszLeaderEnd;
        DWORD   dwStackDepth = CcshellGetStackDepth();

        if (dwStackDepth < SIZECHARS(g_szIndentLeader))
            pszLeaderEnd = &g_wszIndentLeader[dwStackDepth];
        else
            pszLeaderEnd = &g_wszIndentLeader[SIZECHARS(g_wszIndentLeader)-1];

        WCHAR chSave = *pszLeaderEnd;
        *pszLeaderEnd = '\0';

        wsprintfW(ach, L"%s %s", c_wszTrace, g_wszIndentLeader);
        *pszLeaderEnd = chSave;

        // Compose remaining string

        int cch = lstrlenW(ach);
        va_start(vArgs, pszMsg);

        // (We convert the string, rather than simply input an
        // LPCWSTR parameter, so the caller doesn't have to wrap
        // all the string constants with the TEXT() macro.)

        LPWSTR  pwsz;
        WCHAR   wszBuf[1024];
        if (UnicodeFromAnsi(&pwsz, pszMsg, wszBuf, SIZECHARS(wszBuf)))
        {
            wvsprintfW(&ach[cch], pwsz, vArgs);
            UnicodeFromAnsi(&pwsz, NULL, wszBuf, 0);
        }

        va_end(vArgs);
        OutputDebugStringW(ach);
        OutputDebugStringW(c_wszNewline);

        delete [] ach;
    }
}


/*----------------------------------------------------------
Purpose: Assert failed message only
Returns: --
Cond:    --
*/
void
CDECL
CcshellAssertMsgA(
    BOOL f,
    LPCSTR pszMsg, ...)
{
    va_list vArgs;

    if (!f)
    {
        CHAR * ach = new CHAR [1024+40];

        if (ach == NULL)
            return;

        lstrcpyA(ach, c_szAssertMsg);
        int cch = lstrlenA(ach);
        va_start(vArgs, pszMsg);
        wvsprintfA(&ach[cch], pszMsg, vArgs);
        va_end(vArgs);
        OutputDebugStringA(ach);
        OutputDebugStringA(c_szNewline);

        if (IsFlagSet(g_dwBreakFlags, BF_ONVALIDATE))
            ASSERT(0);

        delete [] ach;
    }
}


/*----------------------------------------------------------
Purpose: Debug spew
Returns: --
Cond:    --
*/
void
CDECL
CcshellDebugMsgA(
    DWORD flag,
    LPCSTR pszMsg, ...)
{
    va_list vArgs;

    if  (
        (TF_ALWAYS == flag)
        ||
        (IsFlagSet(g_dwTraceFlags, flag) && flag)
        )
    {
        CHAR * ach = new CHAR [1024+40];

        if (ach == NULL)
            return;

        SetPrefixStringA(ach, flag);
        int cch = lstrlenA(ach);
        va_start(vArgs, pszMsg);
        wvsprintfA(&ach[cch], pszMsg, vArgs);
        va_end(vArgs);
        OutputDebugStringA(ach);
        OutputDebugStringA(c_szNewline);

        if (TF_ALWAYS != flag &&
            ((flag & TF_ERROR) && IsFlagSet(g_dwBreakFlags, BF_ONERRORMSG) ||
             (flag & TF_WARNING) && IsFlagSet(g_dwBreakFlags, BF_ONWARNMSG)))
        {
            DEBUG_BREAK;
        }

        delete [] ach;
    }
}


/*----------------------------------------------------------
Purpose: Debug spew for function trace calls
Returns: --
Cond:    --
*/
void
CDECL
CcshellFuncMsgA(
    DWORD flag,
    LPCSTR pszMsg, ...)
    {
    va_list vArgs;

    if  (
        IsFlagSet(g_dwTraceFlags, TF_FUNC)
        &&
        IsFlagSet(g_dwFuncTraceFlags, flag)
        )
    {
        CHAR * ach = new CHAR [1024+40];    // Largest path plus extra

        if (ach == NULL)
            return;

        // Determine the indentation for trace message based on
        // stack depth.

        LPSTR pszLeaderEnd;
        DWORD dwStackDepth = CcshellGetStackDepth();

        if (dwStackDepth < sizeof(g_szIndentLeader))
            pszLeaderEnd = &g_szIndentLeader[dwStackDepth];
        else
            pszLeaderEnd = &g_szIndentLeader[sizeof(g_szIndentLeader)-1];

        CHAR chSave = *pszLeaderEnd;
        *pszLeaderEnd = '\0';

        wsprintfA(ach, "%s %s", c_szTrace, g_szIndentLeader);
        *pszLeaderEnd = chSave;

        // Compose remaining string

        int cch = lstrlenA(ach);
        va_start(vArgs, pszMsg);
        wvsprintfA(&ach[cch], pszMsg, vArgs);
        va_end(vArgs);
        OutputDebugStringA(ach);
        OutputDebugStringA(c_szNewline);

        delete [] ach;
    }
}


//
//  Debug .ini functions
//


#pragma data_seg(DATASEG_READONLY)

// (c_szCcshellIniFile and c_szCcshellIniSecDebug are declared in debug.h)
extern CHAR const FAR c_szCcshellIniFile[];
extern CHAR const FAR c_szCcshellIniSecDebug[];

// (These are deliberately CHAR)
CHAR const FAR c_szNull[] = "";
CHAR const FAR c_szZero[] = "0";
CHAR const FAR c_szIniKeyBreakFlags[] = "BreakFlags";
CHAR const FAR c_szIniKeyTraceFlags[] = "TraceFlags";
CHAR const FAR c_szIniKeyFuncTraceFlags[] = "FuncTraceFlags";
CHAR const FAR c_szIniKeyDumpFlags[] = "DumpFlags";
CHAR const FAR c_szIniKeyProtoFlags[] = "Prototype";

#pragma data_seg()


// Some of the .ini processing code was pimped from the sync engine.
//

typedef struct _INIKEYHEADER
    {
    LPCTSTR pszSectionName;
    LPCTSTR pszKeyName;
    LPCTSTR pszDefaultRHS;
    } INIKEYHEADER;

typedef struct _BOOLINIKEY
    {
    INIKEYHEADER ikh;
    LPDWORD puStorage;
    DWORD dwFlag;
    } BOOLINIKEY;

typedef struct _INTINIKEY
    {
    INIKEYHEADER ikh;
    LPDWORD puStorage;
    } INTINIKEY;


#define PutIniIntCmp(idsSection, idsKey, nNewValue, nSave) \
    if ((nNewValue) != (nSave)) PutIniInt(idsSection, idsKey, nNewValue)

#define WritePrivateProfileInt(szApp, szKey, i, lpFileName) \
    {CHAR sz[7]; \
    WritePrivateProfileString(szApp, szKey, SzFromInt(sz, i), lpFileName);}


#ifdef BOOL_INI_VALUES
/* Boolean TRUE strings used by IsIniYes() (comparison is case-insensitive) */

static LPCTSTR s_rgpszTrue[] =
    {
    TEXT("1"),
    TEXT("On"),
    TEXT("True"),
    TEXT("Y"),
    TEXT("Yes")
    };

/* Boolean FALSE strings used by IsIniYes() (comparison is case-insensitive) */

static LPCTSTR s_rgpszFalse[] =
    {
    TEXT("0"),
    TEXT("Off"),
    TEXT("False"),
    TEXT("N"),
    TEXT("No")
    };
#endif


#ifdef BOOL_INI_VALUES
/*----------------------------------------------------------
Purpose: Determines whether a string corresponds to a boolean
          TRUE value.
Returns: The boolean value (TRUE or FALSE)
Cond:    --
*/
BOOL
PRIVATE
IsIniYes(
    LPCTSTR psz)
    {
    int i;
    BOOL bNotFound = TRUE;
    BOOL bResult;

    Assert(psz);

    /* Is the value TRUE? */

    for (i = 0; i < ARRAYSIZE(s_rgpszTrue); i++)
        {
        if (IsSzEqual(psz, s_rgpszTrue[i]))
            {
            bResult = TRUE;
            bNotFound = FALSE;
            break;
            }
        }

    /* Is the value FALSE? */

    if (bNotFound)
        {
        for (i = 0; i < ARRAYSIZE(s_rgpszFalse); i++)
            {
            if (IsSzEqual(psz, s_rgpszFalse[i]))
                {
                bResult = FALSE;
                bNotFound = FALSE;
                break;
                }
            }

        /* Is the value a known string? */

        if (bNotFound)
            {
            /* No.  Whine about it. */

            TraceMsg(TF_WARNING, "IsIniYes() called on unknown Boolean RHS '%s'.", psz);
            bResult = FALSE;
            }
        }

    return bResult;
    }


/*----------------------------------------------------------
Purpose: Process keys with boolean RHSs.
Returns: --
Cond:    --
*/
void
PRIVATE
ProcessBooleans(void)
    {
    int i;

    for (i = 0; i < ARRAYSIZE(s_rgbik); i++)
        {
        DWORD dwcbKeyLen;
        TCHAR szRHS[MAX_BUF];
        BOOLINIKEY * pbik = &(s_rgbik[i]);
        LPCTSTR lpcszRHS;

        /* Look for key. */

        dwcbKeyLen = GetPrivateProfileString(pbik->ikh.pszSectionName,
                                   pbik->ikh.pszKeyName, TEXT(""), szRHS,
                                   SIZECHARS(szRHS), c_szCcshellIniFile);

        if (dwcbKeyLen)
            lpcszRHS = szRHS;
        else
            lpcszRHS = pbik->ikh.pszDefaultRHS;

        if (IsIniYes(lpcszRHS))
            {
            if (IsFlagClear(*(pbik->puStorage), pbik->dwFlag))
                TraceMsg(TF_GENERAL, "ProcessIniFile(): %s set in %s![%s].",
                         pbik->ikh.pszKeyName,
                         c_szCcshellIniFile,
                         pbik->ikh.pszSectionName);

            SetFlag(*(pbik->puStorage), pbik->dwFlag);
            }
        else
            {
            if (IsFlagSet(*(pbik->puStorage), pbik->dwFlag))
                TraceMsg(TF_GENERAL, "ProcessIniFile(): %s cleared in %s![%s].",
                         pbik->ikh.pszKeyName,
                         c_szCcshellIniFile,
                         pbik->ikh.pszSectionName);

            ClearFlag(*(pbik->puStorage), pbik->dwFlag);
            }
        }
    }
#endif


/*----------------------------------------------------------
Purpose: Special verion of atoi.  Supports hexadecimal too.

         If this function returns FALSE, *piRet is set to 0.

Returns: TRUE if the string is a number, or contains a partial number
         FALSE if the string is not a number

Cond:    --
*/
static
BOOL
MyStrToIntExA(
    LPCSTR    pszString,
    DWORD     dwFlags,          // STIF_ bitfield
    int FAR * piRet)
    {
    #define IS_DIGIT(ch)    InRange(ch, '0', '9')

    BOOL bRet;
    int n;
    BOOL bNeg = FALSE;
    LPCSTR psz;
    LPCSTR pszAdj;

    // Skip leading whitespace
    //
    for (psz = pszString; *psz == ' ' || *psz == '\n' || *psz == '\t'; psz = CharNextA(psz))
        ;

    // Determine possible explicit signage
    //
    if (*psz == '+' || *psz == '-')
        {
        bNeg = (*psz == '+') ? FALSE : TRUE;
        psz++;
        }

    // Or is this hexadecimal?
    //
    pszAdj = CharNextA(psz);
    if ((STIF_SUPPORT_HEX & dwFlags) &&
        *psz == '0' && (*pszAdj == 'x' || *pszAdj == 'X'))
        {
        // Yes

        // (Never allow negative sign with hexadecimal numbers)
        bNeg = FALSE;
        psz = CharNextA(pszAdj);

        pszAdj = psz;

        // Do the conversion
        //
        for (n = 0; ; psz = CharNextA(psz))
            {
            if (IS_DIGIT(*psz))
                n = 0x10 * n + *psz - '0';
            else
                {
                CHAR ch = *psz;
                int n2;

                if (ch >= 'a')
                    ch -= 'a' - 'A';

                n2 = ch - 'A' + 0xA;
                if (n2 >= 0xA && n2 <= 0xF)
                    n = 0x10 * n + n2;
                else
                    break;
                }
            }

        // Return TRUE if there was at least one digit
        bRet = (psz != pszAdj);
        }
    else
        {
        // No
        pszAdj = psz;

        // Do the conversion
        for (n = 0; IS_DIGIT(*psz); psz = CharNextA(psz))
            n = 10 * n + *psz - '0';

        // Return TRUE if there was at least one digit
        bRet = (psz != pszAdj);
        }

    *piRet = bNeg ? -n : n;

    return bRet;
    }


#ifdef UNICODE

/*----------------------------------------------------------
Purpose: This function converts a wide-char string to a multi-byte
         string.

         If pszBuf is non-NULL and the converted string can fit in
         pszBuf, then *ppszAnsi will point to the given buffer.
         Otherwise, this function will allocate a buffer that can
         hold the converted string.

         If pszWide is NULL, then *ppszAnsi will be freed.  Note
         that pszBuf must be the same pointer between the call
         that converted the string and the call that frees the
         string.

Returns: TRUE
         FALSE (if out of memory)

Cond:    --
*/
static
BOOL
MyAnsiFromUnicode(
    LPSTR * ppszAnsi,
    LPCWSTR pwszWide,        // NULL to clean up
    LPSTR pszBuf,
    int cchBuf)
    {
    BOOL bRet;

    // Convert the string?
    if (pwszWide)
        {
        // Yes; determine the converted string length
        int cch;
        LPSTR psz;

        cch = WideCharToMultiByte(CP_ACP, 0, pwszWide, -1, NULL, 0, NULL, NULL);

        // String too big, or is there no buffer?
        if (cch > cchBuf || NULL == pszBuf)
            {
            // Yes; allocate space
            cchBuf = cch + 1;
            psz = (LPSTR)LocalAlloc(LPTR, CbFromCchA(cchBuf));
            }
        else
            {
            // No; use the provided buffer
            Assert(pszBuf);
            psz = pszBuf;
            }

        if (psz)
            {
            // Convert the string
            cch = WideCharToMultiByte(CP_ACP, 0, pwszWide, -1, psz, cchBuf, NULL, NULL);
            bRet = (0 < cch);
            }
        else
            {
            bRet = FALSE;
            }

        *ppszAnsi = psz;
        }
    else
        {
        // No; was this buffer allocated?
        if (*ppszAnsi && pszBuf != *ppszAnsi)
            {
            // Yes; clean up
            LocalFree((HLOCAL)*ppszAnsi);
            *ppszAnsi = NULL;
            }
        bRet = TRUE;
        }

    return bRet;
    }


/*----------------------------------------------------------
Purpose: Wide-char wrapper for StrToIntExA.

Returns: see StrToIntExA
Cond:    --
*/
static
BOOL
MyStrToIntExW(
    LPCWSTR   pwszString,
    DWORD     dwFlags,          // STIF_ bitfield
    int FAR * piRet)
    {
    // Most strings will simply use this temporary buffer, but AnsiFromUnicode
    // will allocate a buffer if the supplied string is bigger.
    CHAR szBuf[MAX_PATH];

    LPSTR pszString;
    BOOL bRet = MyAnsiFromUnicode(&pszString, pwszString, szBuf, SIZECHARS(szBuf));

    if (bRet)
        {
        bRet = MyStrToIntExA(pszString, dwFlags, piRet);
        MyAnsiFromUnicode(&pszString, NULL, szBuf, 0);
        }
    return bRet;
    }
#endif // UNICODE


#ifdef UNICODE
#define MyStrToIntEx        MyStrToIntExW
#else
#define MyStrToIntEx        MyStrToIntExA
#endif



/*----------------------------------------------------------
Purpose: This function reads a .ini file to determine the debug
         flags to set.  The .ini file and section are specified
         by the following manifest constants:

                SZ_DEBUGINI
                SZ_DEBUGSECTION

         The debug variables that are set by this function are
         g_dwDumpFlags, g_dwTraceFlags, g_dwBreakFlags, and
         g_dwFuncTraceFlags, g_dwPrototype.

Returns: TRUE if initialization is successful
Cond:    --
*/
BOOL
PUBLIC
CcshellGetDebugFlags(void)
    {
    CHAR szRHS[MAX_PATH];
    int val;

    // BUGBUG (scotth): Yes, COMCTL32 exports StrToIntEx, but I
    //  don't want to cause a dependency delta and force everyone
    //  to get a new comctl32 just because they built debug.
    //  So use a local version of StrToIntEx.

    // Trace Flags

    GetPrivateProfileStringA(c_szCcshellIniSecDebug,
                            c_szIniKeyTraceFlags,
                            c_szNull,
                            szRHS,
                            SIZECHARS(szRHS),
                            c_szCcshellIniFile);

    if (MyStrToIntExA(szRHS, STIF_SUPPORT_HEX, &val))
        g_dwTraceFlags = (DWORD)val;

    TraceMsgA(TF_GENERAL, "CcshellGetDebugFlags(): %s set to %#08x.",
             c_szIniKeyTraceFlags, g_dwTraceFlags);

    // Function trace Flags

    GetPrivateProfileStringA(c_szCcshellIniSecDebug,
                            c_szIniKeyFuncTraceFlags,
                            c_szNull,
                            szRHS,
                            SIZECHARS(szRHS),
                            c_szCcshellIniFile);

    if (MyStrToIntExA(szRHS, STIF_SUPPORT_HEX, &val))
        g_dwFuncTraceFlags = (DWORD)val;

    TraceMsgA(TF_GENERAL, "CcshellGetDebugFlags(): %s set to %#08x.",
             c_szIniKeyFuncTraceFlags, g_dwFuncTraceFlags);

    // Dump Flags

    GetPrivateProfileStringA(c_szCcshellIniSecDebug,
                            c_szIniKeyDumpFlags,
                            c_szNull,
                            szRHS,
                            SIZECHARS(szRHS),
                            c_szCcshellIniFile);

    if (MyStrToIntExA(szRHS, STIF_SUPPORT_HEX, &val))
        g_dwDumpFlags = (DWORD)val;

    TraceMsgA(TF_GENERAL, "CcshellGetDebugFlags(): %s set to %#08x.",
             c_szIniKeyDumpFlags, g_dwDumpFlags);

    // Break Flags

    GetPrivateProfileStringA(c_szCcshellIniSecDebug,
                            c_szIniKeyBreakFlags,
                            c_szNull,
                            szRHS,
                            SIZECHARS(szRHS),
                            c_szCcshellIniFile);

    if (MyStrToIntExA(szRHS, STIF_SUPPORT_HEX, &val))
        g_dwBreakFlags = (DWORD)val;

    TraceMsgA(TF_GENERAL, "CcshellGetDebugFlags(): %s set to %#08x.",
             c_szIniKeyBreakFlags, g_dwBreakFlags);

    // Prototype Flags

    GetPrivateProfileStringA(c_szCcshellIniSecDebug,
                            c_szIniKeyProtoFlags,
                            c_szNull,
                            szRHS,
                            SIZECHARS(szRHS),
                            c_szCcshellIniFile);

    if (MyStrToIntExA(szRHS, STIF_SUPPORT_HEX, &val))
        g_dwPrototype = (DWORD)val;

    TraceMsgA(TF_GENERAL, "CcshellGetDebugFlags(): %s set to %#08x.",
             c_szIniKeyProtoFlags, g_dwPrototype);

    return TRUE;
    }

struct GUIDLOOKUP
{
    GUID    rguid;
    LPCTSTR szDescription;
};

GUIDLOOKUP c_guidlookup[] =
{
    {
        {0x4C96BE40L, 0x915C, 0x11CF, {0x99, 0xD3, 0x00, 0xAA, 0x00, 0x4A, 0xE8, 0x37}},
        TEXT("SID_STopLevelBrowser")
    },
    {
        {0xF77459A0, 0xBF9A, 0x11CF, {0xBA, 0x4E, 0x00, 0xC0, 0x4F, 0xD7, 0x08, 0x16}},
        TEXT("IMimeInfo")
    },
    {
        {0x3050f216, 0x98b5, 0x11cf, {0xbb, 0x82, 0x00, 0xaa, 0x00, 0xbd, 0xce, 0x0b}},
        TEXT("IHTMLDialog")
    }, 
    {
        {0x86D52E11, 0x94A8, 0x11d0, {0x82, 0xAF, 0x00, 0xC0, 0x4F, 0xD5, 0xAE, 0x38}},
        TEXT("ITargetFrame2")
    }, 
    
};

/////////////////////////////////////////////////////////////////////////////
// DebugIIDName, DebugCLSIDName
//
// These functions convert an IID or CLSID to a string name for debugging
// purposes (e.g. IID_IUnknown is converted to "IUnknown").
/////////////////////////////////////////////////////////////////////////////
LPCTSTR DebugGUIDName
(
    REFGUID rguid,
    LPCTSTR szKey,
    LPTSTR  pchName
)
{
    OLECHAR achIID[100];        // interface ID (e.g. "{nnn-nnn-...}")  
    TCHAR   ach[150];

    // In case of error, clear <pchName>
    pchName[0] = 0;

    // Convert <rguid> to a string (e.g. "{nnn-nnn-...}")
    StringFromGUID2(rguid, achIID, sizeof(achIID));
    wsprintf(ach, TEXT("%hs\\%ls"), szKey, (LPOLESTR) achIID);

    // Look up <achIID> in the registration database
#ifdef UNICODE
    TCHAR pchNameTemp[300];
    LONG cchNameTemp;
    cchNameTemp = MAX_PATH;

    if (RegQueryValue(  HKEY_CLASSES_ROOT,
                        ach,
                        pchNameTemp,
                        &cchNameTemp) != ERROR_SUCCESS)
    {
        for (int i = 0; i < ARRAYSIZE(c_guidlookup); i++)
        {
            if (InlineIsEqualGUID(c_guidlookup[i].rguid, rguid))
            {
                wsprintf(pchNameTemp, TEXT("%s"), c_guidlookup[i].szDescription);
                break;
            }
        }

        // If <achIID> isn't in the registration database, use <achIID> itself
        if (i == ARRAYSIZE(c_guidlookup))
            wsprintf(pchNameTemp, TEXT("%ls"), (LPOLESTR) achIID);
    }

    wcstombs(pchName, pchNameTemp, MAX_PATH);
#else
    LONG cchNameTemp;
    cchNameTemp = MAX_PATH;
    if (RegQueryValue(  HKEY_CLASSES_ROOT,
                        ach,
                        pchName,
                        &cchNameTemp) != ERROR_SUCCESS)
    {
        for (int i = 0; i < ARRAYSIZE(c_guidlookup); i++)
        {
            if (InlineIsEqualGUID(c_guidlookup[i].rguid, rguid))
            {
                wsprintf(pchName, TEXT("%s"), c_guidlookup[i].szDescription);
                break;
            }
        }

        // If <achIID> isn't in the registration database, use <achIID> itself
        if (i == ARRAYSIZE(c_guidlookup))
            wsprintf(pchName, TEXT("%ls"), (LPOLESTR) achIID);
    }
#endif  // UNICODE
    return pchName;
}

/////////////////////////////////////////////////////////////////////////////
// DebugIIDName
/////////////////////////////////////////////////////////////////////////////
LPCTSTR DebugIIDName
(
    REFIID  riid,
    LPTSTR  pchName
)
{
    return DebugGUIDName(riid, TEXT("Interface"), pchName);
}

/////////////////////////////////////////////////////////////////////////////
// DebugCLSIDName
/////////////////////////////////////////////////////////////////////////////
LPCTSTR DebugCLSIDName
(
    REFCLSID    rclsid,
    LPTSTR      pchName
)
{
    return DebugGUIDName(rclsid, TEXT("Clsid"), pchName);
}
#endif // DEBUG
