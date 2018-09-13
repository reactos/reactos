//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       common.h
//
//--------------------------------------------------------------------------

#ifndef __common_h
#define __common_h

//
// ARRAYSIZE, SIZEOF, and ResultFromShort are defined in private shell
// headers, but those headers tend to move around, change, and break.
// Define the macros here so we don't have to include the headers only for
// this purpose.
//
#ifndef ARRAYSIZE
#define ARRAYSIZE(a)    (sizeof(a)/sizeof(a[0]))
#define SIZEOF(a)       sizeof(a)
#endif

#ifndef ResultFromShort
#define ResultFromShort(i)      MAKE_HRESULT(SEVERITY_SUCCESS, 0, (USHORT)(i))
#endif


//
// Avoid bringing in C runtime code for NO reason
//
#if defined(__cplusplus)
inline void * __cdecl operator new(size_t size) { return (void *)LocalAlloc(LPTR, size); }
inline void __cdecl operator delete(void *ptr) { LocalFree(ptr); }
extern "C" inline __cdecl _purecall(void) { return 0; }
#endif  // __cplusplus

#include "debug.h"
#include "unknown.h"
#include "strings.h"
#include "priv.h"
#include "msgpopup.h"

/*-----------------------------------------------------------------------------
/ Flags to control the trace output from parts of the common library
/----------------------------------------------------------------------------*/
#define TRACE_COMMON_STR       0x80000000
#define TRACE_COMMON_ASSERT    0x40000000
#define TRACE_COMMON_MISC      0x20000000

/*-----------------------------------------------------------------------------
/ Misc functions (misc.cpp)
/----------------------------------------------------------------------------*/
HRESULT CallRegInstall(HMODULE hModule, LPCSTR pszSection);

#if(_WIN32_WINNT < 0x0500)
typedef int (CALLBACK *_PFNDPAENUMCALLBACK)(LPVOID p, LPVOID pData);
void DPA_DestroyCallback(LPVOID, _PFNDPAENUMCALLBACK, LPVOID);
#ifndef DA_LAST
#define DA_LAST         (0x7FFFFFFF)
#endif
#ifndef DPA_AppendPtr
#define DPA_AppendPtr(hdpa, pitem)  DPA_InsertPtr(hdpa, DA_LAST, pitem)
#endif
#endif


/*-----------------------------------------------------------------------------
/ Exit macros for macro
/   - these assume that a label "exit_gracefully:" prefixes the prolog
/     to your function
/----------------------------------------------------------------------------*/
#define ExitGracefully(hr, result, text)            \
            { TraceMsg(text); hr = result; goto exit_gracefully; }

#define FailGracefully(hr, text)                    \
	    { if ( FAILED(hr) ) { TraceMsg(text); goto exit_gracefully; } }


/*-----------------------------------------------------------------------------
/ Interface helper macros
/----------------------------------------------------------------------------*/
#define DoRelease(pInterface)                       \
        { if ( pInterface ) { pInterface->Release(); pInterface = NULL; } }


/*-----------------------------------------------------------------------------
/ String helper macros
/----------------------------------------------------------------------------*/
#define StringByteCopy(pDest, iOffset, sz)          \
        { memcpy(&(((LPBYTE)pDest)[iOffset]), sz, StringByteSize(sz)); }

#define StringByteSize(sz)                          \
        ((lstrlen(sz)+1)*SIZEOF(TCHAR))


/*-----------------------------------------------------------------------------
/ Other helpful macros
/----------------------------------------------------------------------------*/
#define ByteOffset(base, offset)                    \
        (((LPBYTE)base)+offset)

#endif
