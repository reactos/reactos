//
// Debug squirty functions
//

#ifdef UNIX
// Define some things for debug.h
//
#define SZ_DEBUGINI         "ccshell.ini"
#define SZ_DEBUGSECTION     "shellib"
#define SZ_MODULE           "SHELLIB"
#endif

#include "proj.h"
#pragma  hdrstop
#include "shellp.h"

#include <platform.h> // LINE_SEPARATOR_STR and friends
#include <winbase.h> // for GetModuleFileNameA


#define DM_DEBUG              0

BOOL UnicodeFromAnsi(LPWSTR *, LPCSTR, LPWSTR, int);

#if defined(DEBUG) || defined(PRODUCT_PROF)
// (c_szCcshellIniFile and c_szCcshellIniSecDebug are declared in debug.h)
extern CHAR const FAR c_szCcshellIniFile[];
extern CHAR const FAR c_szCcshellIniSecDebug[];
HANDLE g_hDebugOutputFile = INVALID_HANDLE_VALUE;


void ShellDebugAppendToDebugFileA(LPCSTR pszOutputString)
{
    if (g_hDebugOutputFile != INVALID_HANDLE_VALUE)
    {
        DWORD cbWrite = lstrlenA(pszOutputString);
        WriteFile(g_hDebugOutputFile, pszOutputString, cbWrite, &cbWrite, NULL);
    }
}

void ShellDebugAppendToDebugFileW(LPCWSTR pszOutputString)
{
    if (g_hDebugOutputFile != INVALID_HANDLE_VALUE)
    {
        char szBuf[500];

        DWORD cbWrite = WideCharToMultiByte(CP_ACP, 0, pszOutputString, -1, szBuf, ARRAYSIZE(szBuf), NULL, NULL);

        WriteFile(g_hDebugOutputFile, szBuf, cbWrite, &cbWrite, NULL);
    }
}

#if 1 // Looking at the assertW stuff, it delegates already to assertA -- I'm not sure
      // why I really need these wrappers!  (What was broken on my build?  I don't know,
      // but obviously the stuff below was half baked -- there are still problems.)
      // So I'm removing this for now so as to not bother anyone else...  [mikesh]
      //
      // Fixed a few problems and it worked for me. (edwardp)
      //
//
// We cannot link to shlwapi, because comctl32 cannot link to shlwapi.
// Duplicate some functions here so unicode stuff can run on Win95 platforms.
//
VOID MyOutputDebugStringWrapW(LPCWSTR lpOutputString)
{
    if (staticIsOS(OS_NT))
    {
        OutputDebugStringW(lpOutputString);
        ShellDebugAppendToDebugFileW(lpOutputString);
    }
    else
    {
        char szBuf[500];

        WideCharToMultiByte(CP_ACP, 0, lpOutputString, -1, szBuf, ARRAYSIZE(szBuf), NULL, NULL);

        OutputDebugStringA(szBuf);
        ShellDebugAppendToDebugFileA(szBuf);
    }
}
#define OutputDebugStringW MyOutputDebugStringWrapW

VOID MyOutputDebugStringWrapA(LPCSTR lpOutputString)
{
    OutputDebugStringA(lpOutputString);
    ShellDebugAppendToDebugFileA(lpOutputString);
}

#define OutputDebugStringA MyOutputDebugStringWrapA

LPWSTR MyCharPrevWrapW(LPCWSTR lpszStart, LPCWSTR lpszCurrent)
{
    if (lpszCurrent == lpszStart)
    {
        return (LPWSTR) lpszStart;
    }
    else
    {
        return (LPWSTR) lpszCurrent - 1;
    }
}
#define CharPrevW MyCharPrevWrapW

int MywvsprintfWrapW(LPWSTR pwszOut, LPCWSTR pwszFormat, va_list arglist)
{
    if (staticIsOS(OS_NT))
    {
        return wvsprintfW(pwszOut, pwszFormat, arglist);
    }
    else
    {
        char szFormat[500];
        char szOut[1024+40]; // this is how big our ach buffers are
        int iRet;

        WideCharToMultiByte(CP_ACP, 0, pwszFormat, -1, szFormat, ARRAYSIZE(szFormat), NULL, NULL);

        iRet = wvsprintfA(szOut, szFormat, arglist);

        MultiByteToWideChar(CP_ACP, 0, szOut, -1, pwszOut, 1024+40);

        return iRet;
    }
}

#define wvsprintfW MywvsprintfWrapW

int MywsprintfWrapW(LPWSTR pwszOut, LPCWSTR pwszFormat, ...)
{
    int iRet;
    
    va_list ArgList;
    va_start(ArgList, pwszFormat);

    iRet = MywvsprintfWrapW(pwszOut, pwszFormat, ArgList);

    va_end(ArgList);

    return iRet;
}
#define wsprintfW MywsprintfWrapW

LPWSTR lstrcpyWrapW(LPWSTR pszDst, LPCWSTR pszSrc)
{
    while((*pszDst++ = *pszSrc++));

    return pszDst;
}
#define lstrcpyW lstrcpyWrapW

LPWSTR lstrcatWrapW(LPWSTR pszDst, LPCWSTR pszSrc)
{
    return lstrcpyWrapW(pszDst + lstrlenW(pszDst), pszSrc);
}
#define lstrcatW lstrcatWrapW

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

#endif

#ifdef DEBUG

EXTERN_C g_bUseNewLeakDetection = FALSE;

DWORD g_dwDumpFlags     = 0;        // DF_*

#ifdef FULL_DEBUG
DWORD g_dwTraceFlags    = TF_ERROR | TF_WARNING;     // TF_*
#ifndef BREAK_ON_ASSERTS
#define BREAK_ON_ASSERTS
#endif
#else
DWORD g_dwTraceFlags    = TF_ERROR;  // TF_*
#endif

#ifdef BREAK_ON_ASSERTS
DWORD g_dwBreakFlags    = BF_ASSERT;// BF_*
#else
DWORD g_dwBreakFlags    = 0;        // BF_*
#endif

DWORD g_dwPrototype     = 0;        
DWORD g_dwFuncTraceFlags = 0;       // FTF_*

// TLS slot used to store depth for CcshellFuncMsg indentation

static DWORD g_tlsStackDepth = TLS_OUT_OF_INDEXES;

// Hack stack depth counter used when g_tlsStackDepth is not available

static DWORD g_dwHackStackDepth = 0;

static char g_szIndentLeader[] = "                                                                                ";

static WCHAR g_wszIndentLeader[] = L"                                                                                ";


static CHAR const FAR c_szNewline[] = LINE_SEPARATOR_STR;   // (Deliberately CHAR)
static WCHAR const FAR c_wszNewline[] = TEXTW(LINE_SEPARATOR_STR);

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

extern CHAR const FAR c_szRip[];
extern CHAR const FAR c_szRipNoFn[];
extern CHAR const FAR c_szRipMsg[];
extern WCHAR const FAR c_wszRip[];
extern WCHAR const FAR c_wszRipNoFn[];


/*-------------------------------------------------------------------------
Purpose: Adds one of the following prefix strings to pszBuf:
           "t   MODULE  "
           "err MODULE  "
           "wrn MODULE  "

         Returns the count of characters written.
*/
int
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
    return lstrlenA(pszBuf);
}


int
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
    return lstrlenW(pszBuf);
}


static
LPCSTR
_PathFindFileNameA(
    LPCSTR pPath)
{
    LPCSTR pT;

    for (pT = pPath; *pPath; pPath = CharNextA(pPath)) {
        if ((pPath[0] == '\\' || pPath[0] == ':' || pPath[0] == '/')
            && pPath[1] &&  pPath[1] != '\\'  &&   pPath[1] != '/')
            pT = pPath + 1;
    }

    return pT;
}


static
LPCWSTR
_PathFindFileNameW(
    LPCWSTR pPath)
{
    LPCWSTR pT;

    for (pT = pPath; *pPath; pPath++) {
        if ((pPath[0] == TEXTW('\\') || pPath[0] == TEXTW(':') || pPath[0] == TEXTW('/'))
            && pPath[1] &&  pPath[1] != TEXTW('\\')  &&   pPath[1] != TEXTW('/'))
            pT = pPath + 1;
    }

    return pT;
}


/*-------------------------------------------------------------------------
Purpose: Returns TRUE if this process is a primary shell process.
*/
BOOL _IsShellProcess()
{
    CHAR szModuleName[MAX_PATH];
    
    if (GetModuleFileNameA(NULL, szModuleName, sizeof(CHAR) * MAX_PATH) > 0 )
    {                      
        if (StrStrIA(szModuleName, "explorer.exe") || 
            StrStrIA(szModuleName, "iexplore.exe") ||
            StrStrIA(szModuleName, "rundll32.exe") || 
            StrStrIA(szModuleName, "welcome.exe") ||
            StrStrIA(szModuleName, "mshtmpad.exe"))
        {
            // yes, the exe is a shell one
            return TRUE;
        }
    }

    // not a normal shell executable
    return FALSE;
}


// BUGBUG (scotth): Use the Ccshell functions.  _AssertMsg and
// _DebugMsg are obsolete.  They will be removed once all the 
// components don't have TEXT() wrapping their debug strings anymore.


void 
WINCAPI 
_AssertMsgA(
    BOOL f, 
    LPCSTR pszMsg, ...)
{
    CHAR ach[1024+40];
    va_list vArgs;

    if (!f)
    {
        int cch;

        lstrcpyA(ach, c_szAssertMsg);
        cch = lstrlenA(ach);
        va_start(vArgs, pszMsg);

        wvsprintfA(&ach[cch], pszMsg, vArgs);

        va_end(vArgs);
        OutputDebugStringA(ach);

        OutputDebugStringA(c_szNewline);

        if (IsFlagSet(g_dwBreakFlags, BF_ASSERT))
        {
            // MSDEV USERS:  This is not the real assert.  Hit 
            //               Shift-F11 to jump back to the caller.
            DEBUG_BREAK;                                                            // ASSERT
        }
    }
}

void 
WINCAPI 
_AssertMsgW(
    BOOL f, 
    LPCWSTR pszMsg, ...)
{
    WCHAR ach[1024+40];
    va_list vArgs;

    if (!f)
    {
        int cch;

        lstrcpyW(ach, c_wszAssertMsg);
        cch = lstrlenW(ach);
        va_start(vArgs, pszMsg);

        wvsprintfW(&ach[cch], pszMsg, vArgs);

        va_end(vArgs);
        OutputDebugStringW(ach);

        OutputDebugStringW(c_wszNewline);

        if (IsFlagSet(g_dwBreakFlags, BF_ASSERT))
        {
            // MSDEV USERS:  This is not the real assert.  Hit 
            //               Shift-F11 to jump back to the caller.
            DEBUG_BREAK;                                                            // ASSERT
        }
    }
}

void 
_AssertStrLenW(
    LPCWSTR pszStr, 
    int iLen)
{
    if (pszStr && iLen < lstrlenW(pszStr))
    {                                           
        // MSDEV USERS:  This is not the real assert.  Hit 
        //               Shift-F11 to jump back to the caller.
        DEBUG_BREAK;                                                            // ASSERT
    }
}

void 
_AssertStrLenA(
    LPCSTR pszStr, 
    int iLen)
{
    if (pszStr && iLen < lstrlenA(pszStr))
    {                                           
        // MSDEV USERS:  This is not the real assert.  Hit 
        //               Shift-F11 to jump back to the caller.
        DEBUG_BREAK;                                                            // ASSERT
    }
}

void 
WINCAPI 
_DebugMsgA(
    DWORD flag, 
    LPCSTR pszMsg, ...)
{
    CHAR ach[5*MAX_PATH+40];  // Handles 5*largest path + slop for message
    va_list vArgs;

    if (TF_ALWAYS == flag || (IsFlagSet(g_dwTraceFlags, flag) && flag))
    {
        int cch;

        cch = SetPrefixStringA(ach, flag);
        va_start(vArgs, pszMsg);

        try
        {
            wvsprintfA(&ach[cch], pszMsg, vArgs);
        }
        except(EXCEPTION_EXECUTE_HANDLER)
        {
            OutputDebugString(TEXT("CCSHELL: DebugMsg exception: "));
            OutputDebugStringA(pszMsg);
        }
        __endexcept

        va_end(vArgs);
        OutputDebugStringA(ach);
        OutputDebugStringA(c_szNewline);

        if (TF_ALWAYS != flag &&
            ((flag & TF_ERROR) && IsFlagSet(g_dwBreakFlags, BF_ONERRORMSG) ||
             (flag & TF_WARNING) && IsFlagSet(g_dwBreakFlags, BF_ONWARNMSG)))
        {
            // MSDEV USERS:  This is not the real assert.  Hit 
            //               Shift-F11 to jump back to the caller.
            DEBUG_BREAK;                                                            // ASSERT
        }
    }
}

void 
WINCAPI 
_DebugMsgW(
    DWORD flag, 
    LPCWSTR pszMsg, ...)
{
    WCHAR ach[5*MAX_PATH+40];  // Handles 5*largest path + slop for message
    va_list vArgs;

    if (TF_ALWAYS == flag || (IsFlagSet(g_dwTraceFlags, flag) && flag))
    {
        int cch;

        SetPrefixStringW(ach, flag);
        cch = lstrlenW(ach);
        va_start(vArgs, pszMsg);

        try
        {
            wvsprintfW(&ach[cch], pszMsg, vArgs);
        }
        except(EXCEPTION_EXECUTE_HANDLER)
        {
            OutputDebugString(TEXT("CCSHELL: DebugMsg exception: "));
            OutputDebugStringW(pszMsg);
        }
        __endexcept

        va_end(vArgs);
        OutputDebugStringW(ach);
        OutputDebugStringW(c_wszNewline);

        if (TF_ALWAYS != flag &&
            ((flag & TF_ERROR) && IsFlagSet(g_dwBreakFlags, BF_ONERRORMSG) ||
             (flag & TF_WARNING) && IsFlagSet(g_dwBreakFlags, BF_ONWARNMSG)))
        {
            // MSDEV USERS:  This is not the real assert.  Hit 
            //               Shift-F11 to jump back to the caller.
            DEBUG_BREAK;                                                        // ASSERT
        }
    }
}


//
//  Smart debug functions
//



/*----------------------------------------------------------
Purpose: Displays assertion string.

Returns: TRUE to debugbreak
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

    psz = _PathFindFileNameA(pszFile);
    wsprintfA(ach, c_szAssertFailed, psz, line, pszEval);
    OutputDebugStringA(ach);

    if (IsFlagSet(g_dwBreakFlags, BF_ASSERT))
    {
        if (bBreakInside)
        {
            // !!!  ASSERT  !!!!  ASSERT  !!!!  ASSERT !!!

            // MSDEV USERS:  This is not the real assert.  Hit 
            //               Shift-F11 to jump back to the caller.
            DEBUG_BREAK;                                                // ASSERT

            // !!!  ASSERT  !!!!  ASSERT  !!!!  ASSERT !!!
        }
        else
            bRet = TRUE;
    }

    return bRet;
}


/*----------------------------------------------------------
Purpose: Displays assertion string.

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
    WCHAR ach[1024];    // Some callers use more than 256

    psz = _PathFindFileNameW(pszFile);

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

    if (IsFlagSet(g_dwBreakFlags, BF_ASSERT))
    {
        if (bBreakInside)
        {
            // !!!  ASSERT  !!!!  ASSERT  !!!!  ASSERT !!!
            
            // MSDEV USERS:  This is not the real assert.  Hit 
            //               Shift-F11 to jump back to the caller.
            DEBUG_BREAK;                                                // ASSERT

            // !!!  ASSERT  !!!!  ASSERT  !!!!  ASSERT !!!
        }
        else
            bRet = TRUE;
    }

    return bRet;
}


/*----------------------------------------------------------
Purpose: Displays a RIP string.

Returns: TRUE to debugbreak
*/
BOOL
CcshellRipA(
    LPCSTR pszFile,
    int line,
    LPCSTR pszEval,
    BOOL bBreakInside)
{
    BOOL bRet = FALSE;
    LPCSTR psz;
    CHAR ach[256];

    psz = _PathFindFileNameA(pszFile);
    wsprintfA(ach, c_szRipNoFn, psz, line, pszEval);
    OutputDebugStringA(ach);

    if (_IsShellProcess() || IsFlagSet(g_dwBreakFlags, BF_RIP))
    {
        if (bBreakInside)
        {
            // !!!  RIP  !!!!  RIP  !!!!  RIP !!!
            
            // MSDEV USERS:  This is not the real RIP.  Hit 
            //               Shift-F11 to jump back to the caller.
            DEBUG_BREAK;                                                // ASSERT

            // !!!  RIP  !!!!  RIP  !!!!  RIP !!!
        }
        else
            bRet = TRUE;
    }

    return bRet;
}


/*----------------------------------------------------------
Purpose: Displays a RIP string.

*/
BOOL
CcshellRipW(
    LPCWSTR pszFile,
    int line,
    LPCWSTR pszEval,
    BOOL bBreakInside)
{
    BOOL bRet = FALSE;
    LPCWSTR psz;
    WCHAR ach[256];

    psz = _PathFindFileNameW(pszFile);

    // If psz == NULL, CharPrevW failed which implies we are running on Win95.  
    // We can get this if we get an assert in some of the W functions in 
    // shlwapi...  Call the A version of assert...
    if (!psz)
    {
        char szFile[MAX_PATH];
        char szEval[256];   // since the total output is thhis size should be enough...

        WideCharToMultiByte(CP_ACP, 0, pszFile, -1, szFile, ARRAYSIZE(szFile), NULL, NULL);
        WideCharToMultiByte(CP_ACP, 0, pszEval, -1, szEval, ARRAYSIZE(szEval), NULL, NULL);
        return CcshellRipA(szFile, line, szEval, bBreakInside);
    }

    wsprintfW(ach, c_wszRipNoFn, psz, line, pszEval);
    OutputDebugStringW(ach);

    if (_IsShellProcess() || IsFlagSet(g_dwBreakFlags, BF_RIP))
    {
        if (bBreakInside)
        {
            // !!!  RIP  !!!!  RIP  !!!!  RIP !!!

            // MSDEV USERS:  This is not the real assert.  Hit 
            //               Shift-F11 to jump back to the caller.
            DEBUG_BREAK;                                                // ASSERT

            // !!!  RIP  !!!!  RIP  !!!!  RIP !!!
        }
        else
            bRet = TRUE;
    }

    return bRet;
}

BOOL
WINCAPI 
CcshellRipMsgA(
    BOOL f, 
    LPCSTR pszMsg, ...)
{
    CHAR ach[1024+40];
    va_list vArgs;

    if (!f)
    {
        OutputDebugStringA(c_szRipMsg);

        va_start(vArgs, pszMsg);
        wvsprintfA(ach, pszMsg, vArgs);
        va_end(vArgs);
        OutputDebugStringA(ach);

        OutputDebugStringA(c_szNewline);

        if (_IsShellProcess() || IsFlagSet(g_dwBreakFlags, BF_RIP))
        {
            // MSDEV USERS:  This is not the real assert.  Hit 
            //               Shift-F11 to jump back to the caller.
            DEBUG_BREAK;                                                            // ASSERT
        }
    }
    return FALSE;
}

BOOL
WINCAPI 
CcshellRipMsgW(
    BOOL f, 
    LPCSTR pszMsg, ...)         // (this is deliberately CHAR)
{
    WCHAR ach[1024+40];
    va_list vArgs;

    if (!f)
    {
        LPWSTR pwsz;
        WCHAR wszBuf[128];
        OutputDebugStringA(c_szRipMsg);

        // (We convert the string, rather than simply input an
        // LPCWSTR parameter, so the caller doesn't have to wrap
        // all the string constants with the TEXT() macro.)

        ach[0] = L'\0';     // In case this fails
        if (UnicodeFromAnsi(&pwsz, pszMsg, wszBuf, SIZECHARS(wszBuf)))
        {
            va_start(vArgs, pszMsg);
            wvsprintfW(ach, pwsz, vArgs);
            va_end(vArgs);
            UnicodeFromAnsi(&pwsz, NULL, wszBuf, 0);
        }

        OutputDebugStringW(ach);
        OutputDebugStringA(c_szNewline);

        if (_IsShellProcess() || IsFlagSet(g_dwBreakFlags, BF_RIP))
        {
            // MSDEV USERS:  This is not the real assert.  Hit 
            //               Shift-F11 to jump back to the caller.
            DEBUG_BREAK;                                                            // ASSERT
        }
    }
    return FALSE;
}

/*----------------------------------------------------------
Purpose: Keep track of the stack depth for function call trace
         messages.

*/
void
CcshellStackEnter(void)
    {
    if (TLS_OUT_OF_INDEXES != g_tlsStackDepth)
        {
        DWORD dwDepth;

        dwDepth = PtrToUlong(TlsGetValue(g_tlsStackDepth));

        TlsSetValue(g_tlsStackDepth, (LPVOID)(ULONG_PTR)(dwDepth + 1));
        }
    else
        {
        g_dwHackStackDepth++;
        }
    }


/*----------------------------------------------------------
Purpose: Keep track of the stack depth for functionc all trace
         messages.

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
            EVAL(TlsSetValue(g_tlsStackDepth, (LPVOID)(ULONG_PTR)(dwDepth - 1)));
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
*/
void
CDECL
CcshellAssertMsgW(
    BOOL f,
    LPCSTR pszMsg, ...)
{
    WCHAR ach[1024+40];    // Largest path plus extra
    va_list vArgs;

    if (!f)
        {
        int cch;
        WCHAR wszBuf[1024];
        LPWSTR pwsz;

        lstrcpyW(ach, c_wszAssertMsg);
        cch = lstrlenW(ach);
        va_start(vArgs, pszMsg);

        // (We convert the string, rather than simply input an
        // LPCWSTR parameter, so the caller doesn't have to wrap
        // all the string constants with the TEXT() macro.)

        if (UnicodeFromAnsi(&pwsz, pszMsg, wszBuf, SIZECHARS(wszBuf)))
            {
            wvsprintfW(&ach[cch], pwsz, vArgs);
            UnicodeFromAnsi(&pwsz, NULL, wszBuf, 0);
            }

        va_end(vArgs);
        OutputDebugStringW(ach);
        OutputDebugStringW(c_wszNewline);

        if (IsFlagSet(g_dwBreakFlags, BF_ASSERT))
        {
            // !!!  ASSERT  !!!!  ASSERT  !!!!  ASSERT !!!
            
            // MSDEV USERS:  This is not the real assert.  Hit 
            //               Shift-F11 to jump back to the caller.
            DEBUG_BREAK;                                                // ASSERT

            // !!!  ASSERT  !!!!  ASSERT  !!!!  ASSERT !!!
        }
    }
}


/*----------------------------------------------------------
Purpose: Wide-char version of CcshellDebugMsgA.  Note this
         function deliberately takes an ANSI format string
         so our trace messages don't all need to be wrapped
         in TEXT().

*/
void
CDECL
CcshellDebugMsgW(
    DWORD flag,
    LPCSTR pszMsg, ...)         // (this is deliberately CHAR)
{
    WCHAR ach[1024+40];    // Largest path plus extra
    va_list vArgs;

    if (TF_ALWAYS == flag || (IsFlagSet(g_dwTraceFlags, flag) && flag))
    {
        int cch;
        WCHAR wszBuf[1024];
        LPWSTR pwsz;

        SetPrefixStringW(ach, flag);
        cch = lstrlenW(ach);
        va_start(vArgs, pszMsg);

        // (We convert the string, rather than simply input an
        // LPCWSTR parameter, so the caller doesn't have to wrap
        // all the string constants with the TEXT() macro.)

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
            // MSDEV USERS:  This is not the real assert.  Hit 
            //               Shift-F11 to jump back to the caller.
            DEBUG_BREAK;                                                // ASSERT
        }
    }
}


/*-------------------------------------------------------------------------
Purpose: Since the ATL code does not pass in a flag parameter,
         we'll hardcode and check for TF_ATL.
*/
void CDECL ShellAtlTraceW(LPCWSTR pszMsg, ...)
{
    WCHAR ach[1024+40];    // Largest path plus extra
    va_list vArgs;

    if (g_dwTraceFlags & TF_ATL)
    {
        int cch;

        SetPrefixStringW(ach, TF_ATL);
        lstrcatW(ach, L"(ATL) ");
        cch = lstrlenW(ach);
        va_start(vArgs, pszMsg);
        wvsprintfW(&ach[cch], pszMsg, vArgs);
        va_end(vArgs);
        OutputDebugStringW(ach);
    }
}


/*----------------------------------------------------------
Purpose: Wide-char version of CcshellFuncMsgA.  Note this
         function deliberately takes an ANSI format string
         so our trace messages don't all need to be wrapped
         in TEXT().

*/
void
CDECL
CcshellFuncMsgW(
    DWORD flag,
    LPCSTR pszMsg, ...)         // (this is deliberately CHAR)
    {
    WCHAR ach[1024+40];    // Largest path plus extra
    va_list vArgs;

    if (IsFlagSet(g_dwTraceFlags, TF_FUNC) &&
        IsFlagSet(g_dwFuncTraceFlags, flag))
        {
        int cch;
        WCHAR wszBuf[1024];
        LPWSTR pwsz;
        DWORD dwStackDepth;
        LPWSTR pszLeaderEnd;
        WCHAR chSave;

        // Determine the indentation for trace message based on
        // stack depth.

        dwStackDepth = CcshellGetStackDepth();

        if (dwStackDepth < SIZECHARS(g_szIndentLeader))
            {
            pszLeaderEnd = &g_wszIndentLeader[dwStackDepth];
            }
        else
            {
            pszLeaderEnd = &g_wszIndentLeader[SIZECHARS(g_wszIndentLeader)-1];
            }

        chSave = *pszLeaderEnd;
        *pszLeaderEnd = '\0';

        wsprintfW(ach, L"%s %s", c_wszTrace, g_wszIndentLeader);
        *pszLeaderEnd = chSave;

        // Compose remaining string

        cch = lstrlenW(ach);
        va_start(vArgs, pszMsg);

        // (We convert the string, rather than simply input an
        // LPCWSTR parameter, so the caller doesn't have to wrap
        // all the string constants with the TEXT() macro.)

        if (UnicodeFromAnsi(&pwsz, pszMsg, wszBuf, SIZECHARS(wszBuf)))
            {
            wvsprintfW(&ach[cch], pwsz, vArgs);
            UnicodeFromAnsi(&pwsz, NULL, wszBuf, 0);
            }

        va_end(vArgs);
        OutputDebugStringW(ach);
        OutputDebugStringW(c_wszNewline);
        }
    }


/*----------------------------------------------------------
Purpose: Assert failed message only
*/
void
CDECL
CcshellAssertMsgA(
    BOOL f,
    LPCSTR pszMsg, ...)
{
    CHAR ach[1024+40];    // Largest path plus extra
    va_list vArgs;

    if (!f)
    {
        int cch;

        lstrcpyA(ach, c_szAssertMsg);
        cch = lstrlenA(ach);
        va_start(vArgs, pszMsg);
        wvsprintfA(&ach[cch], pszMsg, vArgs);
        va_end(vArgs);
        OutputDebugStringA(ach);
        OutputDebugStringA(c_szNewline);

        if (IsFlagSet(g_dwBreakFlags, BF_ASSERT))
        {
            // !!!  ASSERT  !!!!  ASSERT  !!!!  ASSERT !!!
            
            // MSDEV USERS:  This is not the real assert.  Hit 
            //               Shift-F11 to jump back to the caller.
            DEBUG_BREAK;                                                // ASSERT

            // !!!  ASSERT  !!!!  ASSERT  !!!!  ASSERT !!!
        }
    }
}


/*----------------------------------------------------------
Purpose: Debug spew
*/
void
CDECL
CcshellDebugMsgA(
    DWORD flag,
    LPCSTR pszMsg, ...)
{
    CHAR ach[1024+40];    // Largest path plus extra
    va_list vArgs;

    if (TF_ALWAYS == flag || (IsFlagSet(g_dwTraceFlags, flag) && flag))
    {
        int cch;

        cch = SetPrefixStringA(ach, flag);
        va_start(vArgs, pszMsg);
        wvsprintfA(&ach[cch], pszMsg, vArgs);
        va_end(vArgs);
        OutputDebugStringA(ach);
        OutputDebugStringA(c_szNewline);

        if (TF_ALWAYS != flag &&
            ((flag & TF_ERROR) && IsFlagSet(g_dwBreakFlags, BF_ONERRORMSG) ||
             (flag & TF_WARNING) && IsFlagSet(g_dwBreakFlags, BF_ONWARNMSG)))
        {
            // MSDEV USERS:  This is not the real assert.  Hit 
            //               Shift-F11 to jump back to the caller.
            DEBUG_BREAK;                                                // ASSERT
        }
    }
}


/*-------------------------------------------------------------------------
Purpose: Since the ATL code does not pass in a flag parameter,
         we'll hardcode and check for TF_ATL.
*/
void CDECL ShellAtlTraceA(LPCSTR pszMsg, ...)
{
    CHAR ach[1024+40];    // Largest path plus extra
    va_list vArgs;

    if (g_dwTraceFlags & TF_ATL)
    {
        int cch;

        SetPrefixStringA(ach, TF_ATL);
        lstrcatA(ach, "(ATL) ");
        cch = lstrlenA(ach);
        va_start(vArgs, pszMsg);
        wvsprintfA(&ach[cch], pszMsg, vArgs);
        va_end(vArgs);
        OutputDebugStringA(ach);
    }
}


/*----------------------------------------------------------
Purpose: Debug spew for function trace calls
*/
void
CDECL
CcshellFuncMsgA(
    DWORD flag,
    LPCSTR pszMsg, ...)
    {
    CHAR ach[1024+40];    // Largest path plus extra
    va_list vArgs;

    if (IsFlagSet(g_dwTraceFlags, TF_FUNC) &&
        IsFlagSet(g_dwFuncTraceFlags, flag))
        {
        int cch;
        DWORD dwStackDepth;
        LPSTR pszLeaderEnd;
        CHAR chSave;

        // Determine the indentation for trace message based on
        // stack depth.

        dwStackDepth = CcshellGetStackDepth();

        if (dwStackDepth < sizeof(g_szIndentLeader))
            {
            pszLeaderEnd = &g_szIndentLeader[dwStackDepth];
            }
        else
            {
            pszLeaderEnd = &g_szIndentLeader[sizeof(g_szIndentLeader)-1];
            }

        chSave = *pszLeaderEnd;
        *pszLeaderEnd = '\0';

        wsprintfA(ach, "%s %s", c_szTrace, g_szIndentLeader);
        *pszLeaderEnd = chSave;

        // Compose remaining string

        cch = lstrlenA(ach);
        va_start(vArgs, pszMsg);
        wvsprintfA(&ach[cch], pszMsg, vArgs);
        va_end(vArgs);
        OutputDebugStringA(ach);
        OutputDebugStringA(c_szNewline);
        }
    }


/*-------------------------------------------------------------------------
Purpose: Spews a trace message if hrTest is a failure code.
*/
HRESULT 
TraceHR(
    HRESULT hrTest, 
    LPCSTR pszExpr, 
    LPCSTR pszFile, 
    int iLine)
{
    CHAR ach[1024+40];    // Largest path plus extra

    if (g_dwTraceFlags & TF_WARNING &&
        FAILED(hrTest))
    {
        int cch;

        cch = SetPrefixStringA(ach, TF_WARNING);
        wsprintfA(&ach[cch], "THR: Failure of \"%s\" at %s, line %d (%#08lx)", 
                   pszExpr, _PathFindFileNameA(pszFile), iLine, hrTest);
        OutputDebugStringA(ach);
        OutputDebugStringA(c_szNewline);

        if (IsFlagSet(g_dwBreakFlags, BF_THR))
        {
            // !!!  THR  !!!!  THR  !!!!  THR !!!

            // MSDEV USERS:  This is not the real assert.  Hit 
            //               Shift-F11 to jump back to the caller.
            DEBUG_BREAK;                                                // ASSERT

            // !!!  THR  !!!!  THR  !!!!  THR !!!
        }
    }
    return hrTest;
}


/*-------------------------------------------------------------------------
Purpose: Spews a trace message if bTest is false.
*/
BOOL 
TraceBool(
    BOOL bTest, 
    LPCSTR pszExpr, 
    LPCSTR pszFile, 
    int iLine)
{
    CHAR ach[1024+40];    // Largest path plus extra

    if (g_dwTraceFlags & TF_WARNING && !bTest)
    {
        int cch;

        cch = SetPrefixStringA(ach, TF_WARNING);
        wsprintfA(&ach[cch], "TBOOL: Failure of \"%s\" at %s, line %d", 
                   pszExpr, _PathFindFileNameA(pszFile), iLine);
        OutputDebugStringA(ach);
        OutputDebugStringA(c_szNewline);

        if (IsFlagSet(g_dwBreakFlags, BF_THR))
        {
            // !!!  TBOOL  !!!!  TBOOL  !!!!  TBOOL !!!

            // MSDEV USERS:  This is not the real assert.  Hit 
            //               Shift-F11 to jump back to the caller.
            DEBUG_BREAK;                                                // ASSERT

            // !!!  TBOOL  !!!!  TBOOL  !!!!  TBOOL !!!
        }
    }
    return bTest;
}


/*-------------------------------------------------------------------------
Purpose: Spews a trace message if iTest is -1.
*/
int 
TraceInt(
    int iTest, 
    LPCSTR pszExpr, 
    LPCSTR pszFile, 
    int iLine)
{
    CHAR ach[1024+40];    // Largest path plus extra

    if (g_dwTraceFlags & TF_WARNING && -1 == iTest)
    {
        int cch;

        cch = SetPrefixStringA(ach, TF_WARNING);
        wsprintfA(&ach[cch], "TINT: Failure of \"%s\" at %s, line %d", 
                   pszExpr, _PathFindFileNameA(pszFile), iLine);
        OutputDebugStringA(ach);
        OutputDebugStringA(c_szNewline);

        if (IsFlagSet(g_dwBreakFlags, BF_THR))
        {
            // !!!  TINT  !!!!  TINT  !!!!  TINT !!!

            // MSDEV USERS:  This is not the real assert.  Hit 
            //               Shift-F11 to jump back to the caller.
            DEBUG_BREAK;                                                // ASSERT

            // !!!  TINT  !!!!  TINT  !!!!  TINT !!!
        }
    }
    return iTest;
}


/*-------------------------------------------------------------------------
Purpose: Spews a trace message if pvTest is NULL.
*/
LPVOID 
TracePtr(
    LPVOID pvTest, 
    LPCSTR pszExpr, 
    LPCSTR pszFile, 
    int iLine)
{
    CHAR ach[1024+40];    // Largest path plus extra

    if (g_dwTraceFlags & TF_WARNING && NULL == pvTest)
    {
        int cch;

        cch = SetPrefixStringA(ach, TF_WARNING);
        wsprintfA(&ach[cch], "TPTR: Failure of \"%s\" at %s, line %d", 
                   pszExpr, _PathFindFileNameA(pszFile), iLine);
        OutputDebugStringA(ach);
        OutputDebugStringA(c_szNewline);

        if (IsFlagSet(g_dwBreakFlags, BF_THR))
        {
            // !!!  TPTR  !!!!  TPTR  !!!!  TPTR !!!

            // MSDEV USERS:  This is not the real assert.  Hit 
            //               Shift-F11 to jump back to the caller.
            DEBUG_BREAK;                                                // ASSERT

            // !!!  TPTR  !!!!  TPTR  !!!!  TPTR !!!
        }
    }
    return pvTest;
}


/*-------------------------------------------------------------------------
Purpose: Spews a trace message if dwTest is a Win32 failure code.
*/
DWORD  
TraceWin32(
    DWORD dwTest, 
    LPCSTR pszExpr, 
    LPCSTR pszFile, 
    int iLine)
{
    CHAR ach[1024+40];    // Largest path plus extra

    if (g_dwTraceFlags & TF_WARNING &&
        ERROR_SUCCESS != dwTest)
    {
        int cch;

        cch = SetPrefixStringA(ach, TF_WARNING);
        wsprintfA(&ach[cch], "TW32: Failure of \"%s\" at %s, line %d (%#08lx)", 
                   pszExpr, _PathFindFileNameA(pszFile), iLine, dwTest);
        OutputDebugStringA(ach);
        OutputDebugStringA(c_szNewline);

        if (IsFlagSet(g_dwBreakFlags, BF_THR))
        {
            // !!!  THR  !!!!  THR  !!!!  THR !!!

            // MSDEV USERS:  This is not the real assert.  Hit 
            //               Shift-F11 to jump back to the caller.
            DEBUG_BREAK;                                                // ASSERT

            // !!!  THR  !!!!  THR  !!!!  THR !!!
        }
    }
    return dwTest;
}



//
//  Debug .ini functions
//


#pragma data_seg(DATASEG_READONLY)

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
#ifdef FULL_DEBUG
    else
        g_dwTraceFlags = 3; // default to TF_ERROR and TF_WARNING trace messages
#endif

    TraceMsgA(DM_DEBUG, "CcshellGetDebugFlags(): %s set to %#08x.",
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

    TraceMsgA(DM_DEBUG, "CcshellGetDebugFlags(): %s set to %#08x.",
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

    TraceMsgA(DM_DEBUG, "CcshellGetDebugFlags(): %s set to %#08x.",
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
#ifdef FULL_DEBUG
    else
        g_dwBreakFlags = 5; // default to break on ASSERT and TF_ERROR
#endif

    TraceMsgA(DM_DEBUG, "CcshellGetDebugFlags(): %s set to %#08x.",
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

    // Are we using the new leak detection from shelldbg.dll?
    GetPrivateProfileStringA("ShellDbg",
                            "NewLeakDetection",
                            c_szNull,
                            szRHS,
                            SIZECHARS(szRHS),
                            c_szCcshellIniFile);

    if (MyStrToIntExA(szRHS, STIF_SUPPORT_HEX, &val))
        g_bUseNewLeakDetection = BOOLIFY(val);

    TraceMsgA(DM_DEBUG, "CcshellGetDebugFlags(): %s set to %#08x.",
             c_szIniKeyProtoFlags, g_dwPrototype);

    GetPrivateProfileStringA(c_szCcshellIniSecDebug,
                            "DebugOutputFile",
                            c_szNull,
                            szRHS,
                            SIZECHARS(szRHS),
                            c_szCcshellIniFile);
    if (szRHS != TEXT('\0'))
    {
        g_hDebugOutputFile = CreateFileA(szRHS, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    }

    return TRUE;
    }

// Function to call in allocspy.dll (GetShellMallocSpy)
typedef BOOL (__stdcall *pfnGSMS) (IShellMallocSpy **ppout);

STDAPI_(void) IMSAddToList(BOOL bAdd, void*pv, DWORD cb)
{
    static BOOL bDontTry=FALSE;
    static IShellMallocSpy *pms=NULL;

    if (!bDontTry && pms == NULL)
    {
        pfnGSMS pfnGetShellMallocSpy;
        HMODULE hmod;

        bDontTry = TRUE; // assume failure
        if (hmod = LoadLibraryA("ALLOCSPY.DLL"))
        {
            pfnGetShellMallocSpy = (pfnGSMS) GetProcAddress(hmod, "GetShellMallocSpy");

            pfnGetShellMallocSpy(&pms);
        }
    }
    if (bDontTry)
        return;

    if (bAdd)
        pms->lpVtbl->AddToList(pms, pv, cb);
    else
        pms->lpVtbl->RemoveFromList(pms, pv);
}


#endif // DEBUG

#ifdef PRODUCT_PROF

DWORD g_dwProfileCAP = 0;        

BOOL PUBLIC CcshellGetDebugFlags(void)
{
    CHAR szRHS[MAX_PATH];
    int val;

    GetPrivateProfileStringA(c_szCcshellIniSecDebug,
                            "Profile",
                            "",
                            szRHS,
                            SIZECHARS(szRHS),
                            c_szCcshellIniFile);

    if (MyStrToIntExA(szRHS, STIF_SUPPORT_HEX, &val))
        g_dwProfileCAP = (DWORD)val;

    return TRUE;
}
#endif // PRODUCT_PROF 


#ifdef DEBUG

// turn on path whacking for full-debug builds
#ifdef FULL_DEBUG
static BOOL g_fWhackPathBuffers = TRUE;
#else
static BOOL g_fWhackPathBuffers = FALSE;
#endif

void DEBUGWhackPathBufferA(LPSTR psz, UINT cch)
{
    if (g_fWhackPathBuffers)
    {
        if (psz && IS_VALID_WRITE_BUFFER(psz, char, cch))
        {
            FillMemory(psz, cch * sizeof(char), 0xFE);
        }
    }
}

void DEBUGWhackPathBufferW(LPWSTR psz, UINT cch)
{
    if (g_fWhackPathBuffers)
    {
        if (psz && IS_VALID_WRITE_BUFFER(psz, WCHAR, cch))
        {
            FillMemory(psz, cch * sizeof(WCHAR), 0xFE);
        }
    }
}

void DEBUGWhackPathStringA(LPSTR psz, UINT cch)
{
    if (g_fWhackPathBuffers)
    {
        if (psz && IS_VALID_WRITE_BUFFER(psz, char, cch) && IS_VALID_STRING_PTRA(psz, -1))
        {
            UINT len = lstrlenA(psz);

            if (len >= cch)
            {
                TraceMsg(TF_WARNING, "DEBUGWhackPathStringA: caller of caller passed strange Path string (strlen > buffer size)");
            }
            else
            {
                FillMemory(psz+len+1, (cch-len-1) * sizeof(char), 0xFE);
            }
        }
    }
}

void DEBUGWhackPathStringW(LPWSTR psz, UINT cch)
{
    if (g_fWhackPathBuffers)
    {
        if (psz && IS_VALID_WRITE_BUFFER(psz, WCHAR, cch) && IS_VALID_STRING_PTRW(psz, -1))
        {
            UINT len = lstrlenW(psz);

            if (len >= cch)
            {
                TraceMsg(TF_WARNING, "DEBUGWhackPathStringW: caller of caller passed strange Path string (strlen > buffer size)");
            }
            else
            {
                FillMemory(psz+len+1, (cch-len-1) * sizeof(WCHAR), 0xFE);
            }
        }
    }
}
#endif // DEBUG
