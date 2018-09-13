//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1995
//
//  File:       unicwrap.cxx
//
//  Contents:   Wrappers for all Unicode functions used in the Forms^3 project.
//              Any Unicode parameters/structure fields/buffers are converted
//              to ANSI, and then the corresponding ANSI version of the function
//              is called.
//
//----------------------------------------------------------------------------

#include "precomp.hxx"

#ifndef X_SHELLAPI_H_
#define X_SHELLAPI_H_
#define _SHELL32_
#define _SHDOCVW_
#include <shellapi.h>
#endif

#ifndef X_COMMCTRL_H_
#define X_COMMCTRL_H_
#include <commctrl.h>       // for treeview
#endif

#ifndef X_USP_HXX_
#define X_USP_HXX_
#include "usp.hxx"
#endif

#ifndef X_UNICWRAP_HXX_
#define X_UNICWRAP_HXX_
#include "unicwrap.hxx"
#endif

#ifndef LANG_BURMESE
#define LANG_BURMESE     0x55       // Burma
#endif
#ifndef LANG_KHMER
#define LANG_KHMER       0x53       // Cambodia
#endif
#ifndef LANG_LAO
#define LANG_LAO         0x54       // Lao
#endif
#ifndef LANG_MONGOLIAN
#define LANG_MONGOLIAN   0x50       // Mongolia
#endif
#ifndef LANG_TIBETAN
#define LANG_TIBETAN     0x51       // Tibet
#endif

DeclareTag(tagUniWrap, "UniWrap", "Unicode wrappers information");

MtDefine(CStrInW_pwstr, Utilities, "CStrInW::_pwstr")
MtDefine(CStrIn_pstr,   Utilities, "CStrIn::_pstr")
MtDefine(CStrOut_pstr,  Utilities, "CStrOut::_pstr")
MtDefine(IsTerminalServer_szProductSuite, Utilities, "IsTerminalServer_szProductSuite");

DWORD   g_dwPlatformVersion;            // (dwMajorVersion << 16) + (dwMinorVersion)
DWORD   g_dwPlatformID;                 // VER_PLATFORM_WIN32S/WIN32_WINDOWS/WIN32_WINNT
DWORD   g_dwPlatformBuild;              // Build number
DWORD   g_dwPlatformServicePack;        // Service Pack
BOOL    g_fUnicodePlatform;
BOOL    g_fTerminalServer;              // TRUE if running under NT Terminal Server, FALSE otherwise
BOOL    g_fNLS95Support;
UINT    g_uLatinCodepage;
BOOL    g_fGotLatinCodepage = FALSE;
BOOL    g_fFarEastWin9X;
BOOL    g_fFarEastWinNT;
BOOL    g_fExtTextOutWBuggy;
BOOL    g_fExtTextOutGlyphCrash;
BOOL    g_fBidiSupport; // COMPLEXSCRIPT
BOOL    g_fComplexScriptInput;
BOOL    g_fMirroredBidiLayout;

//+------------------------------------------------------------------------
//
//  Define prototypes of wrapped functions.
//
//-------------------------------------------------------------------------

#if USE_UNICODE_WRAPPERS==1 /* { */

#define STRUCT_ENTRY(FnName, FnType, FnParamList, FnArgs)   \
        FnType _stdcall FnName##Wrap FnParamList ;

#define STRUCT_ENTRY_VOID(FnName, FnParamList, FnArgs)      \
        void _stdcall FnName##Wrap FnParamList ;

#if DBG==1 /* { */

#define STRUCT_ENTRY_NOCONVERT(FnName, FnType, FnParamList, FnArgs) \
        FnType _stdcall FnName##Wrap FnParamList ;

#define STRUCT_ENTRY_VOID_NOCONVERT(FnName, FnParamList, FnArgs) \
        void _stdcall FnName##Wrap FnParamList ;

#else

#define STRUCT_ENTRY_NOCONVERT(FnName, FnType, FnParamList, FnArgs)
#define STRUCT_ENTRY_VOID_NOCONVERT(FnName, FnParamList, FnArgs)

#endif /* } */

#include "wrapfns.h"

#undef STRUCT_ENTRY
#undef STRUCT_ENTRY_VOID
#undef STRUCT_ENTRY_NOCONVERT
#undef STRUCT_ENTRY_VOID_NOCONVERT


//+------------------------------------------------------------------------
//
//  Unicode function globals initialized to point to wrapped functions.
//
//-------------------------------------------------------------------------

#define STRUCT_ENTRY(FnName, FnType, FnParamList, FnArgs)   \
        FnType (_stdcall *g_pufn##FnName) FnParamList = &FnName##Wrap;

#define STRUCT_ENTRY_VOID(FnName, FnParamList, FnArgs)      \
        void   (_stdcall *g_pufn##FnName) FnParamList = &FnName##Wrap;

#if DBG==1 /* { */

#define STRUCT_ENTRY_NOCONVERT(FnName, FnType, FnParamList, FnArgs) \
        FnType (_stdcall *g_pufn##FnName) FnParamList = &FnName##Wrap;

#define STRUCT_ENTRY_VOID_NOCONVERT(FnName, FnParamList, FnArgs)    \
        void   (_stdcall *g_pufn##FnName) FnParamList = &FnName##Wrap;

#else

#define STRUCT_ENTRY_NOCONVERT(FnName, FnType, FnParamList, FnArgs) \
        FnType (_stdcall *g_pufn##FnName) FnParamList;

#define STRUCT_ENTRY_VOID_NOCONVERT(FnName, FnParamList, FnArgs)    \
        void   (_stdcall *g_pufn##FnName) FnParamList;

#endif /* } */

#include "wrapfns.h"

#undef STRUCT_ENTRY
#undef STRUCT_ENTRY_VOID
#undef STRUCT_ENTRY_NOCONVERT
#undef STRUCT_ENTRY_VOID_NOCONVERT

#endif /* } */

//+---------------------------------------------------------------------------
//
//  Function:       IsFarEastLCID(lcid)
//
//  Returns:        True iff lcid is a Far East locale.
//
//----------------------------------------------------------------------------

BOOL
IsFarEastLCID(LCID lcid)
{
    switch (PRIMARYLANGID(LANGIDFROMLCID(lcid)))
    {
        case LANG_CHINESE:
        case LANG_JAPANESE:
        case LANG_KOREAN:
            return TRUE;
    }

    return FALSE;
}

//+---------------------------------------------------------------------------
//  COMPLEXSCRIPT
//  Function:       IsBidiLCID(lcid)
//
//  Returns:        True iff lcid is a right to left locale.
//
//----------------------------------------------------------------------------

BOOL 
IsBidiLCID(LCID lcid)
{
    switch(PRIMARYLANGID(LANGIDFROMLCID(lcid)))
    {
        case LANG_ARABIC:
        case LANG_FARSI:
        case LANG_HEBREW:
            return TRUE;
    }

    return FALSE;
}

//+---------------------------------------------------------------------------
//  COMPLEXSCRIPT
//  Function:       IsComplexLCID(lcid)
//
//  Returns:        True iff lcid is a complex script locale.
//
//----------------------------------------------------------------------------

BOOL 
IsComplexLCID(LCID lcid)
{
    switch(PRIMARYLANGID(LANGIDFROMLCID(lcid)))
    {
        case LANG_ARABIC:
        case LANG_ASSAMESE:
        case LANG_BENGALI:
        case LANG_BURMESE:
        case LANG_FARSI:
        case LANG_GUJARATI:
        case LANG_HEBREW:
        case LANG_HINDI:
        case LANG_KANNADA:
        case LANG_KASHMIRI:
        case LANG_KHMER:
        case LANG_KONKANI:
        case LANG_LAO:
        case LANG_MALAYALAM:
        case LANG_MANIPURI:
        case LANG_MARATHI:
        case LANG_MONGOLIAN:
        case LANG_NAPALI:
        case LANG_ORIYA:
        case LANG_PUNJABI:
        case LANG_SANSKRIT:
        case LANG_SINDHI:
        case LANG_TAMIL:
        case LANG_TELUGU:
        case LANG_THAI:
        case LANG_TIBETAN:
        case LANG_URDU:
        case LANG_VIETNAMESE:
        case LANG_YIDDISH:
           return TRUE;
    }

    return FALSE;
}

//+---------------------------------------------------------------------------
//
//  Function:   InitUnicodeWrappers
//
//  Synopsis:   Determines the platform we are running on and
//              initializes pointers to unicode functions.
//
//----------------------------------------------------------------------------

void
InitUnicodeWrappers()
{
#ifndef WINCE
    OSVERSIONINFOA ovi;
#else //WINCE
    OSVERSIONINFO ovi;
#endif //WINCE
    const UINT acp = GetACP();
#ifndef UNIX
    const BOOL fFarEastLCID = IsFarEastLCID(GetSystemDefaultLCID());
#else
    const BOOL fFarEastLCID = FALSE;  // IEUNIX BUGBUG
#endif

#ifndef WINCE
    ovi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA);
    Verify(GetVersionExA(&ovi));
#else //WINCE
    ovi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    Verify(GetVersionEx(&ovi));
#endif //WINCE

    g_dwPlatformVersion     = (ovi.dwMajorVersion << 16) + ovi.dwMinorVersion;
    g_dwPlatformID          = ovi.dwPlatformId;
    g_dwPlatformBuild       = ovi.dwBuildNumber;

    if (g_dwPlatformID == VER_PLATFORM_WIN32_NT)
    {
        char * pszBeg = ovi.szCSDVersion;

        if (*pszBeg)
        {
            char * pszEnd = pszBeg + lstrlenA(pszBeg);
            
            while (pszEnd > pszBeg)
            {
                char c = pszEnd[-1];

                if (c < '0' || c > '9')
                    break;

                pszEnd -= 1;
            }

            while (*pszEnd)
            {
                g_dwPlatformServicePack *= 10;
                g_dwPlatformServicePack += *pszEnd - '0';
                pszEnd += 1;
            }
        }
    }

#ifndef WINCE
    g_fUnicodePlatform      = (g_dwPlatformID == VER_PLATFORM_WIN32_NT ||
                               g_dwPlatformID == VER_PLATFORM_WIN32_UNIX);

    g_fNLS95Support         = (g_dwPlatformID == VER_PLATFORM_WIN32_WINDOWS ||
                              (g_dwPlatformID == VER_PLATFORM_WIN32_NT &&
                                 ovi.dwMajorVersion >= 3)) ? TRUE : FALSE;

    g_fFarEastWin9X         = fFarEastLCID &&
                              g_dwPlatformID == VER_PLATFORM_WIN32_WINDOWS;

    g_fFarEastWinNT         = fFarEastLCID &&
                              g_dwPlatformID == VER_PLATFORM_WIN32_NT;

    // NB (cthrash) ExtTextOutW and related functions are buggy under the
    // following OSes: Win95 PRC All, Win95 TC Golden
    
    g_fExtTextOutWBuggy     = g_dwPlatformID == VER_PLATFORM_WIN32_WINDOWS &&
                              ((   acp == 950 // CP_TWN
                                && g_dwPlatformVersion == 0x00040000) ||
                               (   acp == 936 // CP_CHN_GB
                                && g_dwPlatformVersion < 0x0004000a ));

    // NB (mikejoch) ExtTextOut(... , ETO_GLYPH_INDEX, ...) crashes under all
    // FE Win95 OSes -- JPN, KOR, CHT, & CHS. Fixed for Win98.
    g_fExtTextOutGlyphCrash = fFarEastLCID &&
                              g_dwPlatformID == VER_PLATFORM_WIN32_WINDOWS &&
                              g_dwPlatformVersion < 0x0004000a;

    g_fMirroredBidiLayout  = ((g_dwPlatformID == VER_PLATFORM_WIN32_WINDOWS &&
                                g_dwPlatformVersion >= 0x0004000a) ||
                               (g_dwPlatformID == VER_PLATFORM_WIN32_NT &&
                                g_dwPlatformVersion >= 0x00050000));

#ifndef UNIX
    HKL aHKL[32];
    UINT uKeyboards = GetKeyboardLayoutList(32, aHKL);
    // check all keyboard layouts for existance of a RTL language.
    // bounce out when the first one is encountered.
    for(UINT i = 0; i < uKeyboards; i++)
    {
        if(IsBidiLCID(LOWORD(aHKL[i])))
        {
            g_fBidiSupport = TRUE;
            g_fComplexScriptInput = TRUE;
            break;
        }

        if(IsComplexLCID(LOWORD(aHKL[i])))
        {
            g_fComplexScriptInput = TRUE;
        }
        
    }
#else //UNIX
    g_fBidiSupport = FALSE;
    g_fComplexScriptInput = FALSE;
#endif
#else //WINCE
    g_fUnicodePlatform      = TRUE;

    g_fNLS95Support         = TRUE;

    g_fFarEastWin9X         = FALSE;

    g_fFarEastWinNT         = fFarEastLCID;

    g_fExtTextOutWBuggy     = FALSE;

    g_fExtTextOutGlyphCrash = FALSE;

    g_fBidiSupport          = FALSE;

    g_fComplexScriptInput   = FALSE;

    g_fMirroredBidiLayout = FALSE;

#endif //WINCE



#if USE_UNICODE_WRAPPERS==1     /* { */

    //
    // If the platform is unicode, then overwrite function table to point
    // to the unicode functions.
    //

    if (g_fUnicodePlatform)
    {
        #define STRUCT_ENTRY(FnName, FnType, FnParamList, FnArgs)   \
                g_pufn##FnName = &FnName##W;

        #define STRUCT_ENTRY_VOID(FnName, FnParamList, FnArgs)      \
                g_pufn##FnName = &FnName##W;

        #define STRUCT_ENTRY_NOCONVERT(FnName, FnType, FnParamList, FnArgs) \
                g_pufn##FnName = &FnName##W;

        #define STRUCT_ENTRY_VOID_NOCONVERT(FnName, FnParamList, FnArgs)    \
                g_pufn##FnName = &FnName##W;

        #include "wrapfns.h"

        #undef STRUCT_ENTRY
        #undef STRUCT_ENTRY_VOID
        #undef STRUCT_ENTRY_NOCONVERT
        #undef STRUCT_ENTRY_VOID_NOCONVERT
    }
    else
    {
        //
        // If we are not doing conversions of trivial wrapper functions, initialize pointers
        // to point to operating system functions.
        //

        #define STRUCT_ENTRY(FnName, FnType, FnParamList, FnArgs)
        #define STRUCT_ENTRY_VOID(FnName, FnParamList, FnArgs)
        #define STRUCT_ENTRY_NOCONVERT(FnName, FnType, FnParamList, FnArgs) \
                g_pufn##FnName = (FnType (_stdcall *)FnParamList) &FnName##A;

        #define STRUCT_ENTRY_VOID_NOCONVERT(FnName, FnParamList, FnArgs)    \
                g_pufn##FnName = (void (_stdcall *)FnParamList) &FnName##A;

        #include "wrapfns.h"

        #undef STRUCT_ENTRY
        #undef STRUCT_ENTRY_VOID
        #undef STRUCT_ENTRY_NOCONVERT
        #undef STRUCT_ENTRY_VOID_NOCONVERT

    }
#else
    {
        // RISC workaround for CP wrappers.

        #define STRUCT_ENTRY(FnName, FnType, FnParamList, FnArgs)
        #define STRUCT_ENTRY_VOID(FnName, FnParamList, FnArgs)
        #define STRUCT_ENTRY_NOCONVERT(FnName, FnType, FnParamList, FnArgs)
        #define STRUCT_ENTRY_VOID_NOCONVERT(FnName, FnParamList, FnArgs)

        #include "wrapfns.h"

        #undef STRUCT_ENTRY
        #undef STRUCT_ENTRY_VOID
        #undef STRUCT_ENTRY_NOCONVERT
        #undef STRUCT_ENTRY_VOID_NOCONVERT
    }    
#endif /* } */

    g_fTerminalServer = IsTerminalServer();
}

//+------------------------------------------------------------------------
//
//  Wrapper function utilities.
//  NOTE: normally these would also be surrounded by an #ifndef NO_UNICODE_WRAPPERS
//        but the string conversion functions are needed for dealing with
//        wininet.
//
//-------------------------------------------------------------------------

int MbcsFromUnicode(LPSTR pstr, int cch, LPCWSTR pwstr, int cwch = -1);
int UnicodeFromMbcs(LPWSTR pwstr, int cwch, LPCSTR pstr, int cch = -1);


//+---------------------------------------------------------------------------
//
//  Member:     CConvertStr::Free
//
//  Synopsis:   Frees string if alloc'd and initializes to NULL.
//
//----------------------------------------------------------------------------

void
CConvertStr::Free()
{
    if (_pstr != _ach && HIWORD64(_pstr) != 0)
    {
        delete [] _pstr;
    }

    _pstr = NULL;
}


//+---------------------------------------------------------------------------
//
//  Member:     CConvertStrW::Free
//
//  Synopsis:   Frees string if alloc'd and initializes to NULL.
//
//----------------------------------------------------------------------------

void
CConvertStrW::Free()
{
    if (_pwstr != _awch && HIWORD64(_pwstr) != 0)
    {
        delete [] _pwstr;
    }

    _pwstr = NULL;
}


//+---------------------------------------------------------------------------
//
//  Member:     CStrInW::Init
//
//  Synopsis:   Converts a LPSTR function argument to a LPWSTR.
//
//  Arguments:  [pstr] -- The function argument.  May be NULL or an atom
//                              (HIWORD64(pwstr) == 0).
//
//              [cch]  -- The number of characters in the string to
//                          convert.  If -1, the string is assumed to be
//                          NULL terminated and its length is calculated.
//
//  Modifies:   [this]
//
//----------------------------------------------------------------------------

void
CStrInW::Init(LPCSTR pstr, int cch)
{
    int cchBufReq;

    _cwchLen = 0;

    // Check if string is NULL or an atom.
    if (HIWORD64(pstr) == 0)
    {
        _pwstr = (LPWSTR) pstr;
        return;
    }

    Assert(cch == -1 || cch > 0);

    //
    // Convert string to preallocated buffer, and return if successful.
    //
    // Since the passed in buffer may not be null terminated, we have
    // a problem if cch==ARRAYSIZE(_awch), because MultiByteToWideChar
    // will succeed, and we won't be able to null terminate the string!
    // Decrease our buffer by one for this case.
    //
    _cwchLen = MultiByteToWideChar(
            CP_ACP, 0, pstr, cch, _awch, ARRAY_SIZE(_awch)-1);

    if (_cwchLen > 0)
    {
        // Some callers don't NULL terminate.
        //
        // We could check "if (-1 != cch)" before doing this,
        // but always doing the null is less code.
        //
        _awch[_cwchLen] = 0;

        if(_awch[_cwchLen-1] == 0)
            _cwchLen--;                // account for terminator

        _pwstr = _awch;
        return;
    }

    //
    // Alloc space on heap for buffer.
    //

    cchBufReq = MultiByteToWideChar( CP_ACP, 0, pstr, cch, NULL, 0 );

    // Again, leave room for null termination
    cchBufReq++;

    Assert(cchBufReq > 0);
    _pwstr = new(Mt(CStrInW_pwstr)) WCHAR[cchBufReq];
    if (!_pwstr)
    {
        // On failure, the argument will point to the empty string.
        _awch[0] = 0;
        _pwstr = _awch;
        return;
    }

    Assert(HIWORD64(_pwstr));
    _cwchLen = MultiByteToWideChar(
            CP_ACP, 0, pstr, cch, _pwstr, cchBufReq );

    // Again, make sure we're always null terminated
    Assert(_cwchLen < cchBufReq);
    _pwstr[_cwchLen] = 0;

    if (0 == _pwstr[_cwchLen-1]) // account for terminator
        _cwchLen--;
}


//+---------------------------------------------------------------------------
//
//  Member:     CStrIn::CStrIn
//
//  Synopsis:   Inits the class.
//
//  NOTE:       Don't inline this function or you'll increase code size
//              by pushing -1 on the stack for each call.
//
//----------------------------------------------------------------------------

CStrIn::CStrIn(LPCWSTR pwstr) : CConvertStr(CP_ACP)
{
    Init(pwstr, -1);
}


CStrIn::CStrIn(UINT uCP, LPCWSTR pwstr) : CConvertStr(uCP==1200?CP_ACP:uCP)
{
    Init(pwstr, -1);
}


//+---------------------------------------------------------------------------
//
//  Member:     CStrIn::Init
//
//  Synopsis:   Converts a LPWSTR function argument to a LPSTR.
//
//  Arguments:  [pwstr] -- The function argument.  May be NULL or an atom
//                              (HIWORD64(pwstr) == 0).
//
//              [cwch]  -- The number of characters in the string to
//                          convert.  If -1, the string is assumed to be
//                          NULL terminated and its length is calculated.
//
//  Modifies:   [this]
//
//----------------------------------------------------------------------------

void
CStrIn::Init(LPCWSTR pwstr, int cwch)
{
    int cchBufReq;

#if DBG == 1 /* { */
    int errcode;
#endif /* } */

    _cchLen = 0;

    // Check if string is NULL or an atom.
    if (HIWORD64(pwstr) == 0)
    {
        _pstr = (LPSTR) pwstr;
        return;
    }

    if ( cwch == 0 )
    {
        *_ach = '\0';
        _pstr = _ach;
        return;
    }

    Assert(cwch == -1 || cwch > 0);
    //
    // Convert string to preallocated buffer, and return if successful.
    //

    _cchLen = WideCharToMultiByte(
            _uCP, 0, pwstr, cwch, _ach, ARRAY_SIZE(_ach)-1, NULL, NULL);

    if (_cchLen > 0)
    {
        // This is DBCS safe since byte before _cchLen is last character
        _ach[_cchLen] = 0;
        // BUGBUG DBCS REVIEW: this may not be safe if the last character
        // was a multibyte character...
        if (_ach[_cchLen-1]==0)
            _cchLen--;          // account for terminator
        _pstr = _ach;
        return;
    }

    //
    // Alloc space on heap for buffer.
    //
    cchBufReq = WideCharToMultiByte(
            _uCP, 0, pwstr, cwch, NULL, 0, NULL, NULL);

    Assert(cchBufReq > 0);

    cchBufReq++; // may need to append NUL

    TraceTag((tagUniWrap, "CStrIn: Allocating buffer for argument (_uCP=%ld,cwch=%ld,pwstr=%lX,cchBufReq=%ld)",
             _uCP, cwch, pwstr, cchBufReq));

    _pstr = new(Mt(CStrIn_pstr)) char[cchBufReq];
    if (!_pstr)
    {
        // On failure, the argument will point to the empty string.
        TraceTag((tagUniWrap, "CStrIn: No heap space for wrapped function argument."));
        _ach[0] = 0;
        _pstr = _ach;
        return;
    }

    Assert(HIWORD64(_pstr));
    _cchLen = WideCharToMultiByte(
            _uCP, 0, pwstr, cwch, _pstr, cchBufReq, NULL, NULL);

#if DBG == 1 /* { */
    if (_cchLen < 0)
    {
        errcode = GetLastError();
        TraceTag((tagError, "WideCharToMultiByte failed with errcode %ld", errcode));
        Assert(0 && "WideCharToMultiByte failed in unicode wrapper.");
    }
#endif /* } */

        // Again, make sure we're always null terminated
    Assert(_cchLen < cchBufReq);
    _pstr[_cchLen] = 0;
    if (0 == _pstr[_cchLen-1]) // account for terminator
        _cchLen--;
}


//+---------------------------------------------------------------------------
//
//  Member:     CStrInMulti::CStrInMulti
//
//  Synopsis:   Converts mulitple LPWSTRs to a multiple LPSTRs.
//
//  Arguments:  [pwstr] -- The strings to convert.
//
//  Modifies:   [this]
//
//----------------------------------------------------------------------------

CStrInMulti::CStrInMulti(LPCWSTR pwstr)
{
    LPCWSTR pwstrT;

    // We don't handle atoms because we don't need to.
    Assert(HIWORD64(pwstr));

    //
    // Count number of characters to convert.
    //

    pwstrT = pwstr;
    if (pwstr)
    {
        do {
            while (*pwstrT++)
                ;

        } while (*pwstrT++);
    }

    Init(pwstr, pwstrT - pwstr);
}



//+---------------------------------------------------------------------------
//
//  Member:     CStrOut::CStrOut
//
//  Synopsis:   Allocates enough space for an out buffer.
//
//  Arguments:  [pwstr]   -- The Unicode buffer to convert to when destroyed.
//                              May be NULL.
//
//              [cwchBuf] -- The size of the buffer in characters.
//
//  Modifies:   [this].
//
//----------------------------------------------------------------------------

CStrOut::CStrOut(LPWSTR pwstr, int cwchBuf) : CConvertStr(CP_ACP)
{
    Assert(cwchBuf >= 0);

    _pwstr = pwstr;
    _cwchBuf = cwchBuf;

    if (!pwstr)
    {
        _cwchBuf = 0;
        _pstr = NULL;
        return;
    }

    Assert(HIWORD64(pwstr));

    // Initialize buffer in case Windows API returns an error.
    _ach[0] = 0;

    // Use preallocated buffer if big enough.
    if (cwchBuf * 2 <= ARRAY_SIZE(_ach))
    {
        _pstr = _ach;
        return;
    }

    // Allocate buffer.
    TraceTag((tagUniWrap, "CStrOut: Allocating buffer for wrapped function argument."));
    _pstr = new(Mt(CStrOut_pstr)) char[cwchBuf * 2];
    if (!_pstr)
    {
        //
        // On failure, the argument will point to a zero-sized buffer initialized
        // to the empty string.  This should cause the Windows API to fail.
        //

        TraceTag((tagUniWrap, "CStrOut: No heap space for wrapped function argument."));
        Assert(cwchBuf > 0);
        _pwstr[0] = 0;
        _cwchBuf = 0;
        _pstr = _ach;
        return;
    }

    Assert(HIWORD64(_pstr));
    _pstr[0] = 0;
}

//+---------------------------------------------------------------------------
//
//  Member:     CStrOut::ConvertIncludingNul
//
//  Synopsis:   Converts the buffer from MBCS to Unicode
//
//  Return:     Character count INCLUDING the trailing '\0'
//
//----------------------------------------------------------------------------

int
CStrOut::ConvertIncludingNul()
{
    int cch;

    if (!_pstr)
        return 0;

    cch = MultiByteToWideChar(_uCP, 0, _pstr, -1, _pwstr, _cwchBuf);

#if DBG == 1 /* { */
    if (cch <= 0 && _cwchBuf > 0)
    {
        int errcode = GetLastError();
        AssertSz(errcode != S_OK, "MultiByteToWideChar failed in unicode wrapper.");
    }
#endif /* } */

    Free();
    return cch;
}


//+---------------------------------------------------------------------------
//
//  Member:     CStrOut::ConvertExcludingNul
//
//  Synopsis:   Converts the buffer from MBCS to Unicode
//
//  Return:     Character count EXCLUDING the trailing '\0'
//
//----------------------------------------------------------------------------

int
CStrOut::ConvertExcludingNul()
{
    int ret = ConvertIncludingNul();
    if (ret > 0)
    {
        ret -= 1;
    }
    return ret;
}

//+---------------------------------------------------------------------------
//
//  Member:     CStrOut::~CStrOut
//
//  Synopsis:   Converts the buffer from MBCS to Unicode.
//
//  Note:       Don't inline this function, or you'll increase code size as
//              both ConvertIncludingNul() and CConvertStr::~CConvertStr will be
//              called inline.
//
//----------------------------------------------------------------------------

CStrOut::~CStrOut()
{
    ConvertIncludingNul();
}

//+---------------------------------------------------------------------------
//
//  Function:   MbcsFromUnicode
//
//  Synopsis:   Converts a string to MBCS from Unicode.
//
//  Arguments:  [pstr]  -- The buffer for the MBCS string.
//              [cch]   -- The size of the MBCS buffer, including space for
//                              NULL terminator.
//
//              [pwstr] -- The Unicode string to convert.
//              [cwch]  -- The number of characters in the Unicode string to
//                              convert, including NULL terminator.  If this
//                              number is -1, the string is assumed to be
//                              NULL terminated.  -1 is supplied as a
//                              default argument.
//
//  Returns:    If [pstr] is NULL or [cch] is 0, 0 is returned.  Otherwise,
//              the number of characters converted, including the terminating
//              NULL, is returned (note that converting the empty string will
//              return 1).  If the conversion fails, 0 is returned.
//
//  Modifies:   [pstr].
//
//----------------------------------------------------------------------------

int
MbcsFromUnicode(LPSTR pstr, int cch, LPCWSTR pwstr, int cwch)
{
    int ret;

#if DBG == 1 /* { */
    int errcode;
#endif /* } */

    Assert(cch >= 0);
    if (!pstr || cch == 0)
        return 0;

    Assert(pwstr);
    Assert(cwch == -1 || cwch > 0);

    ret = WideCharToMultiByte(CP_ACP, 0, pwstr, cwch, pstr, cch, NULL, NULL);

#if DBG == 1 /* { */
    if (ret <= 0)
    {
        errcode = GetLastError();
        Assert(0 && "WideCharToMultiByte failed in unicode wrapper.");
    }
#endif /* } */

    return ret;
}

//+---------------------------------------------------------------------------
//
//  Function:   UnicodeFromMbcs
//
//  Synopsis:   Converts a string to Unicode from MBCS.
//
//  Arguments:  [pwstr] -- The buffer for the Unicode string.
//              [cwch]  -- The size of the Unicode buffer, including space for
//                              NULL terminator.
//
//              [pstr]  -- The MBCS string to convert.
//              [cch]  -- The number of characters in the MBCS string to
//                              convert, including NULL terminator.  If this
//                              number is -1, the string is assumed to be
//                              NULL terminated.  -1 is supplied as a
//                              default argument.
//
//  Returns:    If [pwstr] is NULL or [cwch] is 0, 0 is returned.  Otherwise,
//              the number of characters converted, including the terminating
//              NULL, is returned (note that converting the empty string will
//              return 1).  If the conversion fails, 0 is returned.
//
//  Modifies:   [pwstr].
//
//----------------------------------------------------------------------------

int
UnicodeFromMbcs(LPWSTR pwstr, int cwch, LPCSTR pstr, int cch)
{
    int ret;

#if DBG == 1 /* { */
    int errcode;
#endif /* } */

    Assert(cwch >= 0);

    if (!pwstr || cwch == 0)
        return 0;

    Assert(pstr);
    Assert(cch == -1 || cch > 0);

    ret = MultiByteToWideChar(CP_ACP, 0, pstr, cch, pwstr, cwch);

#if DBG == 1 /* { */
    if (ret <= 0)
    {
        errcode = GetLastError();
        Assert(0 && "MultiByteToWideChar failed in unicode wrapper.");
    }
#endif /* } */

    return ret;
}



//+------------------------------------------------------------------------
//
//  Implementation of the wrapped functions
//
//-------------------------------------------------------------------------

#if USE_UNICODE_WRAPPERS==1 /* { */

#if DBG==1 /* { */
BOOL WINAPI
ChooseColorWrap(LPCHOOSECOLORW lpcc)
{
    Assert(!lpcc->lpTemplateName);
    return ChooseColorA((LPCHOOSECOLORA) lpcc);
}
#endif /* } */


BOOL WINAPI
ChooseFontWrap(LPCHOOSEFONTW lpcfw)
{
    BOOL            ret;
    CHOOSEFONTA     cfa;
    LOGFONTA        lfa;
    LPLOGFONTW      lplfw;

    Assert(!lpcfw->lpTemplateName);
    Assert(!lpcfw->lpszStyle);

    Assert(sizeof(CHOOSEFONTA) == sizeof(CHOOSEFONTW));
    memcpy(&cfa, lpcfw, sizeof(CHOOSEFONTA));

    memcpy(&lfa, lpcfw->lpLogFont, offsetof(LOGFONTA, lfFaceName));
    MbcsFromUnicode(lfa.lfFaceName, ARRAY_SIZE(lfa.lfFaceName), lpcfw->lpLogFont->lfFaceName);

    cfa.lpLogFont = &lfa;

    ret = ChooseFontA(&cfa);

    if (ret)
    {
        lplfw = lpcfw->lpLogFont;
        memcpy(lpcfw, &cfa, sizeof(CHOOSEFONTW));
        lpcfw->lpLogFont = lplfw;

        memcpy(lpcfw->lpLogFont, &lfa, offsetof(LOGFONTW, lfFaceName));
        UnicodeFromMbcs(lpcfw->lpLogFont->lfFaceName, ARRAY_SIZE(lpcfw->lpLogFont->lfFaceName), (LPCSTR)(lfa.lfFaceName));
    }
    return ret;
}

#if DBG == 1 /* { */
HINSTANCE WINAPI
LoadLibraryWrap(LPCWSTR lpLibFileName)
{
    Assert(0 && "LoadLibrary called - use LoadLibraryEx instead");
    return 0;
}
#endif /* } */

int
DrawTextInCodePage(UINT uCP, HDC hDC, LPCWSTR lpString, int nCount, LPRECT lpRect, UINT uFormat)
{
    if (g_fUnicodePlatform)
        return DrawTextW(hDC, (LPWSTR)lpString, nCount, lpRect, uFormat);
    else
    {
        CStrIn str(uCP,lpString,nCount);
        return DrawTextA(hDC, str, str.strlen(), lpRect, uFormat);
    }
}

#endif /* } */

// Everything after this is present on all platforms (Unicode or not)

//+---------------------------------------------------------------------------
//      GetLatinCodepage
//----------------------------------------------------------------------------

UINT
GetLatinCodepage()
{
    if (!g_fGotLatinCodepage)
    {
        // When converting Latin-1 characters, we will use g_uLatinCodepage.
        // The first choice is to use Windows-1252 (Windows Latin-1).  That
        // should be present in all systems, unless deliberately removed.  If
        // that fails, we go for our second choice, which is MS-DOS Latin-1.
        // If that fails, we'll just go with CP_ACP. (cthrash)

        if ( !IsValidCodePage( g_uLatinCodepage = 1252 ) &&
             !IsValidCodePage( g_uLatinCodepage = 850 ) )
        {
                g_uLatinCodepage = CP_ACP;
        }

        g_fGotLatinCodepage = TRUE;
    }

    return(g_uLatinCodepage);
}


// BUGBUG these definitions come from shlwapip.h.  The right way to use them
// is to #include <shlwapi.h> with _WIN32_IE set to 0x501 or better, which
// is done by changing WIN32_IE_VERSION in common.inc.  However, doing this
// causes conflicts between shlwapip.h and shellapi.h.  So until the shell
// folks get their story straight, I'm just reproducing the definitions I
// need here. (SamBent)

#if !defined(GMI_TSCLIENT)
//
//  GMI_TSCLIENT tells you whether you are running as a Terminal Server
//  client and should disable your animations.
//
#define GMI_TSCLIENT            0x0003  // Returns nonzero if TS client

STDAPI_(DWORD_PTR) SHGetMachineInfo(UINT gmi);

#endif // !defined(GMI_TSCLIENT)


BOOL IsTerminalServer()
{
    return !!SHGetMachineInfo(GMI_TSCLIENT);
}
