// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//------------------------------------------------------------------------------
//

//
//  File:       Always.h
//  Note:       This file is very order-dependent.  Don't switch files around
//              capriciously.
//------------------------------------------------------------------------------

#pragma once
#ifdef _MANAGED
#pragma unmanaged
#endif

#define _OLEAUT32_
#define INC_OLE2
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif /* WIN32_LEAN_AND_MEAN */
#define OEMRESOURCE

#ifndef AVALON_NOMEMSYSTEM

// Note:    Make it so you can't use the CRT strdup functions.  Use
//          MemAllocString instead. We want to do this so that the memory is
//          allocated and tagged with our allocators.
#define _strdup CRT__strdup_DontUse
#define _wcsdup CRT__wcsdup_DontUse
#define strdup  CRT_strdup_DontUse

// Also don't let people use the CRT malloc/realloc/calloc/free functions
#define malloc  CRT_malloc_DontUse
#define realloc CRT_realloc_DontUse
#define calloc  CRT_calloc_DontUse
#define free    CRT_free_DontUse
#endif

//
// Make HRESULT_FROM_WIN32 an inline method rather than a macro
//
#define INLINE_HRESULT_FROM_WIN32

// Windows include
#include <w4warn.h>


#include <windows.h>
#include <w4warn.h> // windows.h reenables some pragmas
#include <windowsx.h>

#ifdef AVALON_INCLUDE_NT_HEADERS

#include <windef.h>

#endif // AVALON_INCLUDE_NT_HEADERS

// C runtime includes
#include <stdlib.h>
#include <limits.h>
#include <stddef.h>
#include <search.h>
#include <string.h>
#include <tchar.h>

// We want to include this here so that
// no one else can.
#include <malloc.h>


#ifndef AVALON_NOMEMSYSTEM
#undef _strdup
#undef _wcsdup
#undef strdup
#undef malloc
#undef realloc
#undef calloc
#undef free

// Note:    (jbeda) If you get an error pointing to these functions please look
//          at at the note above
__declspec(deprecated) char *  __cdecl _strdup(const char *);
__declspec(deprecated) _Check_return_ char *  __cdecl strdup(_In_opt_z_ const char *);
__declspec(deprecated) WCHAR * __cdecl _wcsdup(const WCHAR *);
__declspec(deprecated) void * __cdecl malloc(size_t);
__declspec(deprecated) void * __cdecl realloc(void *, size_t);
__declspec(deprecated) void * __cdecl calloc(size_t, size_t);
__declspec(deprecated) void   __cdecl free(void *);
#endif

#if !defined( UNIX )
#define __endexcept
#endif // UNIX

#include <w4warn.h>
#include "AvalonDebugP.h"


#ifdef _PREFIX_
    // __pfx_assume and __pfx_assert are not automatically declared
    #if __cplusplus
        extern "C" void __pfx_assert(bool, const char *);
        extern "C" void __pfx_assume(bool, const char *);
    #else
        void __pfx_assert(int, const char *);
        void __pfx_assume(int, const char *);
    #endif
#else
    #define __pfx_assert(Exp, Msg) do {} while ( UNCONDITIONAL_EXPR(false) )
    #define __pfx_assume(Exp, Msg) do {} while ( UNCONDITIONAL_EXPR(false) )
#endif

