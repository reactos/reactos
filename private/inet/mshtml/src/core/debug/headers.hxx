//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       headers.hxx
//
//  Contents:   include files for Forms Debug DLL
//
//----------------------------------------------------------------------------

#ifndef FORMDBG_HEADERS_HXX
#define FORMDBG_HEADERS_HXX

#if DBG==0
#define RETAILBUILD
#endif

#undef DBG
#define DBG  1

#ifndef INCMSG
#define INCMSG(x)
//#define INCMSG(x) message(x)
#endif

#define _OLEAUT32_
#define INC_OLE2
#define WIN32_LEAN_AND_MEAN
#define OEMRESOURCE

// Windows includes

#include <w4warn.h>

#ifndef X_WINDOWS_H_
#define X_WINDOWS_H_
#pragma INCMSG("--- Beg <windows.h>")
#include <windows.h>
#pragma INCMSG("--- End <windows.h>")
#endif

#include <w4warn.h> // windows.h reenables some pragmas

#ifndef X_WINDOWSX_H_
#define X_WINDOWSX_H_
#pragma INCMSG("--- Beg <windowsx.h>")
#include <windowsx.h>
#pragma INCMSG("--- End <windowsx.h>")
#endif

#ifndef X_PLATFORM_H_
#define X_PLATFORM_H_
#pragma INCMSG("--- Beg <platform.h>")
#include <platform.h>
#pragma INCMSG("--- End <platform.h>")
#endif

// C runtime includes

#ifndef X_LIMITS_H_
#define X_LIMITS_H_
#pragma INCMSG("--- Beg <limits.h>")
#include <limits.h>
#pragma INCMSG("--- End <limits.h>")
#endif

#ifndef X_STDDEF_H_
#define X_STDDEF_H_
#pragma INCMSG("--- Beg <stddef.h>")
#include <stddef.h>
#pragma INCMSG("--- End <stddef.h>")
#endif

#ifndef X_SEARCH_H_
#define X_SEARCH_H_
#pragma INCMSG("--- Beg <search.h>")
#include <search.h>
#pragma INCMSG("--- End <search.h>")
#endif

#ifndef X_STRING_H_
#define X_STRING_H_
#pragma INCMSG("--- Beg <string.h>")
#include <string.h>
#pragma INCMSG("--- End <string.h>")
#endif

#ifndef X_TCHAR_H_
#define X_TCHAR_H_
#pragma INCMSG("--- Beg <tchar.h>")
#include <tchar.h>
#pragma INCMSG("--- End <tchar.h>")
#endif

// Core includes

#ifndef X_F3DEBUG_H_
#define X_F3DEBUG_H_
#include <f3debug.h>
#endif

#ifndef X__F3DEBUG_H_
#define X__F3DEBUG_H_
#include "_f3debug.h"
#endif

#include <mshtmdbg.h>

#pragma warning(disable:4702) /* unreachable code */

#endif