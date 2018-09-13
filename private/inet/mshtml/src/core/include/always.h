//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1992 - 1998.
//
//  File:       Common header file for the Trident project.
//
//  Note:       This file is very order-dependent.  Don't switch files around
//              just for the heck of it!
//
//----------------------------------------------------------------------------

#ifndef I_ALWAYS_H_
#define I_ALWAYS_H_

#ifndef INCMSG
#define INCMSG(x)
//#define INCMSG(x) message(x)
#endif

#pragma INCMSG("--- Beg 'always.h'")

#define _OLEAUT32_
#define INC_OLE2
#define WIN32_LEAN_AND_MEAN
#define OEMRESOURCE

// Windows includes

#ifndef X_SHLWRAP_H_
#define X_SHLWRAP_H_
#include "shlwrap.h"
#endif

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

#ifndef X_COMMDLG_H_
#define X_COMMDLG_H_
#pragma INCMSG("--- Beg <commdlg.h>")
#include <commdlg.h>
#pragma INCMSG("--- End <commdlg.h>")
#endif

#ifndef X_PLATFORM_H_
#define X_PLATFORM_H_
#pragma INCMSG("--- Beg <platform.h>")
#include <platform.h>
#pragma INCMSG("--- End <platform.h>")
#endif

#ifndef X_DOCOBJ_H_
#define X_DOCOBJ_H_
#pragma INCMSG("--- Beg <docobj.h>")
#include <docobj.h>
#pragma INCMSG("--- End <docobj.h>")
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

#define F3DLL_WRAPPERS_ONLY // Include wrappers for forms3.dll only.

#include <w4warn.h>

#ifndef X_COREDISP_H_
#define X_COREDISP_H_
#include "coredisp.h"
#endif

#ifndef X_WRAPDEFS_H_
#define X_WRAPDEFS_H_
#include "wrapdefs.h"
#endif

#ifndef X_F3DEBUG_H_
#define X_F3DEBUG_H_
#include "f3debug.h"
#endif

#ifndef X_F3UTIL_HXX_
#define X_F3UTIL_HXX_
#include "f3util.hxx"
#endif

#ifndef X_CDUTIL_HXX_
#define X_CDUTIL_HXX_
#include "cdutil.hxx"
#endif

#ifndef X_CSTR_HXX_
#define X_CSTR_HXX_
#include "cstr.hxx"
#endif

#ifndef X_FORMSARY_HXX_
#define X_FORMSARY_HXX_
#include "formsary.hxx"
#endif

#ifndef X_DLL_HXX_
#define X_DLL_HXX_
#include "dll.hxx"
#endif

#ifndef X_CDBASE_HXX_
#define X_CDBASE_HXX_
#include "cdbase.hxx"
#endif

#ifndef X_TYPES_H_
#define X_TYPES_H_
#pragma INCMSG("--- Beg 'types.h'")
#include "types.h"
#pragma INCMSG("--- End 'types.h'")
#endif

// This prevents you from having to include codepage.h if all you want is
// the typedef for CODEPAGE.

typedef UINT CODEPAGE;

#ifndef X_SHLWAPI_H_
#define X_SHLWAPI_H_
#pragma INCMSG("--- Beg <shlwapi.h>")
#include <shlwapi.h>
#pragma INCMSG("--- End <shlwapi.h>")
#endif

#ifndef X_SHLWAPIP_H_
#define X_SHLWAPIP_H_
#pragma INCMSG("--- Beg <shlwapip.h>")
#include <shlwapip.h>
#pragma INCMSG("--- End <shlwapip.h>")
#endif

#ifndef X_ACTIVSCP_H_
#define X_ACTIVSCP_H_
#include <activscp.h>
#endif

#pragma INCMSG("--- End 'always.h'")
#else
#pragma INCMSG("*** Dup 'always.h'")
#endif
