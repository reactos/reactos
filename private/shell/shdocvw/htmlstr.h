/*
 * htmlstr.h
 *
 * HTML string constants
 *
 */

#ifndef _HTMLSTR_H
#define _HTMLSTR_H

#if !defined( WIN16 ) || !defined( __WATCOMC__ )

#ifdef DEFINE_STRING_CONSTANTS

#ifndef UNIX

#define MAKEBSTR(name, count, strdata) \
    extern "C" CDECL const WORD DATA_##name [] = {(count * sizeof(OLECHAR)), 0x00, L##strdata}; \
    extern "C" CDECL BSTR name = (BSTR)& DATA_##name[2];

#else

// IEUNIX : Trying to get same memory layout as above.

struct UNIX_BSTR_FORMAT {
        DWORD cbCount;                                         \
        WCHAR data[] ;                                         \
};

#define MAKEBSTR(name, count, strdata)                         \
    const struct UNIX_BSTR_FORMAT STRUCT_##name = {(count * sizeof(OLECHAR)), L##strdata};  \
    extern "C" CDECL BSTR name = (BSTR) &STRUCT_##name.data;

#endif


#define STR_GLOBAL(x,y)         extern "C" CDECL const TCHAR x[] = TEXT(y)
#define STR_GLOBAL_ANSI(x,y)    extern "C" CDECL const char x[] = y
#define STR_GLOBAL_WIDE(x,y)    extern "C" CDECL const WCHAR x[] = L##y

#else
#define MAKEBSTR(name, count, strdata) extern "C" CDECL LPCWSTR name

#define STR_GLOBAL(x,y)         extern "C" CDECL const TCHAR x[]
#define STR_GLOBAL_ANSI(x,y)    extern "C" CDECL const char x[]
#define STR_GLOBAL_WIDE(x,y)    extern "C" CDECL const WCHAR x[]
#endif

#else // !WIN16 || !__WATCOMC__

#ifdef DEFINE_STRING_CONSTANTS
#define MAKEBSTR(name, count, strdata) \
    extern "C" const char CDECL DATA_##name [] = {(count * sizeof(OLECHAR)), 0x00, strdata}; \
    extern "C" BSTR  CDECL name = (BSTR)& DATA_##name[2];


#define STR_GLOBAL(x,y)         extern "C" const TCHAR CDECL x[] = TEXT(y)
#define STR_GLOBAL_ANSI(x,y)    extern "C" const char CDECL x[] = y
#define STR_GLOBAL_WIDE(x,y)    extern "C" const WCHAR CDECL x[] = y

#else
#define MAKEBSTR(name, count, strdata) extern "C" LPCWSTR CDECL name

#define STR_GLOBAL(x,y)         extern "C" const TCHAR CDECL x[]
#define STR_GLOBAL_ANSI(x,y)    extern "C" const char CDECL x[]
#define STR_GLOBAL_WIDE(x,y)    extern "C" const WCHAR CDECL x[]
#endif

#endif // !WIN16 || !__WATCOMC__


MAKEBSTR(c_bstr_AfterBegin, 10, "AfterBegin");
MAKEBSTR(c_bstr_BeforeEnd,   9, "BeforeEnd");
MAKEBSTR(c_bstr_SRC,        3,  "src");
MAKEBSTR(c_bstr_HREF,       4,  "HREF");
MAKEBSTR(c_bstr_IMG,        3,  "IMG");
MAKEBSTR(c_bstr_BGSOUND,    7,  "BGSOUND");
MAKEBSTR(c_bstr_BASE,       4,  "BASE");
MAKEBSTR(c_bstr_Word,       4,  "Word");
MAKEBSTR(c_bstr_Character,  9,  "Character");
MAKEBSTR(c_bstr_StartToEnd, 10, "StartToEnd");
MAKEBSTR(c_bstr_EndToEnd,   8,  "EndToEnd");
MAKEBSTR(c_bstr_StartToStart,   12,  "StartToStart");
MAKEBSTR(c_bstr_EndToStart, 10, "EndToStart");
MAKEBSTR(c_bstr_ANCHOR, 1, "A");
MAKEBSTR(c_bstr_BLOCKQUOTE,10,  "BLOCKQUOTE");
MAKEBSTR(c_bstr_BACKGROUND,10,  "background");
MAKEBSTR(c_bstr_BODY,      4,   "BODY");
MAKEBSTR(c_bstr_TABLE,     5,   "TABLE");
MAKEBSTR(c_bstr_TD,        2,   "TD");
MAKEBSTR(c_bstr_TH,        2,   "TH");
MAKEBSTR(c_bstr_FRAME,     5,   "FRAME");
MAKEBSTR(c_bstr_IFRAME,    6,   "IFRAME");
MAKEBSTR(c_bstr_FRAMESET,  8,   "FRAMESET");
MAKEBSTR(c_bstr_LINK,      4,   "LINK");
MAKEBSTR(c_bstr_REL,       3,   "REL");
MAKEBSTR(c_bstr_STYLESHEET, 10, "stylesheet");
MAKEBSTR(c_bstr_DYNSRC,    6,   "DYNSRC");
MAKEBSTR(c_bstr_INPUT,     5,   "INPUT" );
MAKEBSTR(c_bstr_AREA,     4,   "AREA" );
MAKEBSTR(c_bstr_ON,     2,   "on" );
MAKEBSTR(c_bstr_SCRIPT, 6,  "SCRIPT" );
MAKEBSTR(c_bstr_EMPTY,  0, "");
MAKEBSTR(c_bstr_DesignOff, 3, "off" );
MAKEBSTR(c_bstr_BLANK, 11, "about:blank");
#endif //_HTMLSTR_H
