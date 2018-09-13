//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992, 1993  Microsoft Corporation.  All Rights Reserved.
//
//--------------------------------------------------------------------------;
//
//  debug.h
//
//  Description:
//
//
//
//==========================================================================;

#ifndef _INC_DEBUG
#define _INC_DEBUG
#ifdef __cplusplus
extern "C"
{
#endif



#ifndef _WIN32
#ifndef LPCTSTR
#define LPCTSTR LPCSTR
#endif
#ifndef TCHAR
#define TCHAR char
#endif
#endif


//--------------------------------------------------------------------------;
//
//  The following is the only stuff that should need to be changed when
//  moving this debug code from one project component to another.
//
//--------------------------------------------------------------------------;

//
//  DEBUG_MODULE_NAME is the module name of the component you are
//  building.  In the [debug] section of WIN.INI you can add
//  an entry MYMODULE=n to set the debug level for you module.
//  You can use debug statements like:
//	DPF(2, "My debug string");
//  This output would appear only if MYMODULE=n appears in WIN.INI
//  and n>=2.
//
#ifdef _WIN32
#define DEBUG_MODULE_NAME       "MSVFW32"    // key name and prefix for output
#else
#define DEBUG_MODULE_NAME       "MSVIDEO"   // key name and prefix for output
#endif

//
//  You can also specify certain types of debug information.  For example,
//  you may have much debug output that is associated only with initialization.
//  By adding an entry to the following enumeration, and then adding the
//  corresponding string to the following array of strings, you can specify
//  a debug level for different types of debug information.  Using the
//  initialization example, you can add an entry like "MYMODULENAME_dbgInit=n"
//  to the [debug] section to set a debug level for debug information
//  associated only with initialization.  You would use debug statements like:
//	DPFS(dbgInit, 3, "My debug string");
//  This output would appear only if MYMODULENAME_dbgInit=n appears in WIN.INI
//  and n>=3.  This would be usefull when you only want to debug the logic
//  associated only with a certain part of you program.
//
//  DO NOT CHANGE the first entry in the enum and the aszDbgSpecs.
//
enum {
    dbgNone=0,
    dbgInit,
    dbgThunks
};

#ifdef _INC_DEBUG_CODE
LPCSTR aszDbgSpecs[] = {
    "\0",
    "_dbgInit",
    "_dbgThunks"
};
#endif

//--------------------------------------------------------------------------;
//
//  You should NOT need to modify anthing below here when
//  moving this debug code from one project component to another.
//
//--------------------------------------------------------------------------;


//
//
//
#ifdef DEBUG
    #define DEBUG_SECTION       "Debug"     // section name for
    #define DEBUG_MAX_LINE_LEN  255         // max line length (bytes!)
#endif


//
//  based code makes since only in win 16 (to try and keep stuff out of
//  [fixed] data segments, etc)...
//
#ifndef BCODE
#ifdef _WIN32
    #define BCODE
#else
    #define BCODE           _based(_segname("_CODE"))
#endif
#endif



//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
//
//
//
//  #pragma message(REMIND("this is a reminder"))
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

#define DEBUG_QUOTE(x)      #x
#define DEBUG_QQUOTE(y)     DEBUG_QUOTE(y)
#define REMIND(sz)          __FILE__ "(" DEBUG_QQUOTE(__LINE__) ") : " sz

#ifdef DEBUG
    BOOL WINAPI DbgEnable(UINT uDbgSpec, BOOL fEnable);
    UINT WINAPI DbgGetLevel(UINT uDbgSpec);
    UINT WINAPI DbgSetLevel(UINT uDbgSpec, UINT uLevel);
    VOID WINAPI DbgInitialize(BOOL fEnable);
    void WINAPI _Assert( char * szFile, int iLine );

    void FAR CDECL dprintfS(UINT uDbgSpec, UINT uDbgLevel, LPSTR szFmt, ...);
    void FAR CDECL dprintf(UINT uDbgLevel, LPSTR szFmt, ...);

    #define D(x)        {x;}
    #define DPFS	dprintfS
    #define DPF		dprintf
    #define DPI(sz)     {static char BCODE ach[] = sz; OutputDebugStr(ach);}
    #define ASSERT(x)   if( !(x) )  _Assert( __FILE__, __LINE__)
#else
    #define DbgEnable(x)        FALSE
    #define DbgGetLevel()       0
    #define DbgSetLevel(x)      0
    #define DbgInitialize(x)

    #ifdef _MSC_VER
    #pragma warning(disable:4002)
    #endif

    #define D(x)
    #define DPFS()
    #define DPF()
    #define DPI(sz)
    #define ASSERT(x)
#endif


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
//
//
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

#ifdef RDEBUG
    #define DebugErr(flags, sz)         {static char BCODE szx[] = DEBUG_MODULE_NAME ": " sz; DebugOutput((flags) | DBF_MMSYSTEM, szx);}
    #define DebugErr1(flags, sz, a)     {static char BCODE szx[] = DEBUG_MODULE_NAME ": " sz; DebugOutput((flags) | DBF_MMSYSTEM, szx, a);}
    #define DebugErr2(flags, sz, a, b)  {static char BCODE szx[] = DEBUG_MODULE_NAME ": " sz; DebugOutput((flags) | DBF_MMSYSTEM, szx, a, b);}
#else
    #define DebugErr(flags, sz)
    #define DebugErr1(flags, sz, a)
    #define DebugErr2(flags, sz, a, b)
#endif

#ifdef __cplusplus
}
#endif
#endif  // _INC_DEBUG
