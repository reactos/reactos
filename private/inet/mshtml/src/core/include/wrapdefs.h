//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       wrapdefs.h
//
//  Contents:   Definitions of variables and functions required for wrapping
//              functions where there are differences between Windows95 and
//              Windows NT.
//
//              If you want to add your own wrapped function, see the
//              directions in wrapfns.h.
//----------------------------------------------------------------------------

#ifndef I_WRAPDEFS_HXX_
#define I_WRAPDEFS_HXX_
#pragma INCMSG("--- Beg 'wrapdefs.h'")

// define some other languages until ntdefs.h catches up
#ifndef LANG_KASHMIRI
#define LANG_KASHMIRI     0x60       
#endif
#ifndef LANG_MANIPURI
#define LANG_MANIPURI     0x58       
#endif
#ifndef LANG_NAPALI
#define LANG_NAPALI       0x61       
#endif
#ifndef LANG_SINDHI
#define LANG_SINDHI       0x59
#endif
#ifndef LANG_YIDDISH
#define LANG_YIDDISH      0x3d       
#endif


extern DWORD g_dwPlatformID;        // VER_PLATFORM_WIN32S/WIN32_WINDOWS/WIN32_WINNT
#ifndef WIN16

extern DWORD g_dwPlatformVersion;   // (dwMajorVersion << 16) + (dwMinorVersion)
extern BOOL  g_fUnicodePlatform;
extern BOOL  g_fTerminalServer;     // TRUE if running under NT Terminal Server, FALSE otherwise
extern BOOL  g_fNLS95Support;
extern BOOL  g_fFarEastWin9X;
extern BOOL  g_fFarEastWinNT;
extern BOOL  g_fExtTextOutWBuggy;
extern BOOL  g_fExtTextOutGlyphCrash;
extern BOOL  g_fBidiSupport;        // COMPLEXSCRIPT
extern BOOL  g_fComplexScriptInput;
extern BOOL  g_fMirroredBidiLayout;

void InitUnicodeWrappers();
UINT GetLatinCodepage();            // Most likely 1252
HRESULT DoFileDownLoad(const TCHAR *pchHref);

#if defined(_M_IX86) && !defined(WINCE)
    #define USE_UNICODE_WRAPPERS 1
    int DrawTextInCodePage(UINT uCP, HDC hDC, LPCWSTR lpString, int nCount, LPRECT lpRect, UINT uFormat);
#else
    #define USE_UNICODE_WRAPPERS 0
    #define DrawTextInCodePage(a,b,c,d,e,f) DrawText((b),(c),(d),(e),(f))
#endif

BOOL IsTerminalServer();


//+------------------------------------------------------------------------
//
// Returns the global function pointer for a unicode function.
//
//-------------------------------------------------------------------------

#if USE_UNICODE_WRAPPERS==1
    #define UNICODE_FN(fn)  g_pufn##fn
#else
    #define UNICODE_FN(fn)  fn
#endif

//+------------------------------------------------------------------------
//
// Turn off warnings from dllimport.
//
//-------------------------------------------------------------------------
BOOL IsFarEastLCID( LCID lcid );
BOOL IsBidiLCID( LCID lcid ); // COMPLEXSCRIPT
BOOL IsComplexLCID( LCID lcid );

#ifndef BYPASS_UNICODE_WRAPPERS

#ifndef X_SHELLAPI_H_
#define X_SHELLAPI_H_
#define _SHELL32_
#define _SHDOCVW_
#pragma INCMSG("--- Beg <shellapi.h>")
#include <shellapi.h>
#pragma INCMSG("--- End <shellapi.h>")
#endif

#ifndef X_COMMCTRL_H_
#define X_COMMCTRL_H_
#undef WINCOMMCTRLAPI
#define WINCOMMCTRLAPI
#pragma INCMSG("--- Beg <commctrl.h>")
#include <commctrl.h>
#pragma INCMSG("--- End <commctrl.h>")
#endif

#ifndef X_COMCTRLP_H_
#define X_COMCTRLP_H_
#undef WINCOMMCTRLAPI
#define WINCOMMCTRLAPI
#pragma INCMSG("--- Beg <comctrlp.h>")
#include <comctrlp.h>
#pragma INCMSG("--- End <comctrlp.h>")
#endif

#ifndef X_INTSHCUT_H_
#define X_INTSHCUT_H_
#define _INTSHCUT_
#pragma INCMSG("--- Beg <intshcut.h>")
#include <intshcut.h>
#pragma INCMSG("--- End <intshcut.h>")
#endif

#endif

//+------------------------------------------------------------------------
//
//  Declaration of global function pointers to unicode or wrapped functions.
//
//-------------------------------------------------------------------------

#if USE_UNICODE_WRAPPERS==1

#define STRUCT_ENTRY(FnName, FnType, FnParamList, FnArgs) \
        extern FnType (__stdcall *g_pufn##FnName) FnParamList;

#define STRUCT_ENTRY_VOID(FnName, FnParamList, FnArgs) \
        extern void (__stdcall *g_pufn##FnName) FnParamList;

#define STRUCT_ENTRY_NOCONVERT(FnName, FnType, FnParamList, FnArgs) \
        extern FnType (__stdcall *g_pufn##FnName) FnParamList;

#define STRUCT_ENTRY_VOID_NOCONVERT(FnName, FnParamList, FnArgs) \
        extern void (__stdcall *g_pufn##FnName) FnParamList;

#include "wrapfns.h"

#undef STRUCT_ENTRY
#undef STRUCT_ENTRY_VOID
#undef STRUCT_ENTRY_NOCONVERT
#undef STRUCT_ENTRY_VOID_NOCONVERT

#endif


//+------------------------------------------------------------------------
//
//  Define inline functions which call functions in the table. The
//  functions are defined from entries in wrapfns.h.
//
//-------------------------------------------------------------------------

#if USE_UNICODE_WRAPPERS==1 && !defined(BYPASS_UNICODE_WRAPPERS)

#define STRUCT_ENTRY(FnName, FnType, FnParamList, FnArgs) \
        inline FnType _stdcall FnName FnParamList {return (*g_pufn##FnName) FnArgs;}

#define STRUCT_ENTRY_VOID(FnName, FnParamList, FnArgs) \
        inline void _stdcall FnName FnParamList {(*g_pufn##FnName) FnArgs;}

#define STRUCT_ENTRY_NOCONVERT(FnName, FnType, FnParamList, FnArgs) \
        inline FnType _stdcall FnName FnParamList {return (*g_pufn##FnName) FnArgs;}

#define STRUCT_ENTRY_VOID_NOCONVERT(FnName, FnParamList, FnArgs) \
        inline void _stdcall FnName FnParamList {(*g_pufn##FnName) FnArgs;}


#include "wrapfns.h"

#undef STRUCT_ENTRY
#undef STRUCT_ENTRY_VOID
#undef STRUCT_ENTRY_NOCONVERT
#undef STRUCT_ENTRY_VOID_NOCONVERT

//-------------------------------------------------------------------------
//
// Handle wsprintf specially, as it has a variable length argument list.
//
//-------------------------------------------------------------------------

#undef wsprintf

inline int
wsprintf(LPTSTR pwszOut, LPCTSTR pwszFormat, ...)
{
    int i;
    va_list arglist;

    va_start(arglist, pwszFormat);
    i = wvsprintf(pwszOut, pwszFormat, arglist);
    va_end(arglist);

    return i;
}

#endif

#else
// WIN16...
// We try to define the minimum possible here!
#define UNICODE_FN(fn)  fn

#define InitUnicodeWrappers()

// GetLatinCodepage Most likely 1252, but it's really irrelevant.
#define GetLatinCodepage() 1252

#define DrawTextInCodePage(a,b,c,d,e,f) DrawText((b),(c),(d),(e),(f))

HRESULT DoFileDownLoad(const TCHAR *pchHref);

#endif // ndef WIN16

#pragma INCMSG("--- End 'wrapdefs.h'")
#else
#pragma INCMSG("*** Dup 'wrapdefs.h'")
#endif
