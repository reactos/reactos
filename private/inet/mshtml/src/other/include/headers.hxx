//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       Common header file for the entire forms project.
//
//  History:    31-Dec-93         LyleC           Created
//              19-Jul-94         DonCl           setup for Corona builds
//              26-Jul-94         DonCl           types2/Corona checkin
//              12-Oct-94         DonCl           removed all the
//                                                DAYTONA/CHICAGO/CORONA
//                                                stuff for Forms^3 proj.
//              12-Dec-94         RobBear         Copied from core
//              5-26-95             kfl             added win2mac.h
//
//  Note:       This file is very order-dependent.  Don't switch files around
//              just for the heck of it!
//
//
//----------------------------------------------------------------------------

#define _OLEAUT32_
#define _LARGE_INTEGER_SUPPORT_
#define WIN32_LEAN_AND_MEAN
#define INC_OLE2

//
// ********* Public Cairo/Windows includes
//

// Force loading of OEM resources for rendering controls under Daytona
#define OEMRESOURCE

#ifdef _MAC
#   define _WINERROR_
#   define GUID_DEFINED
#   define __OBJECTID_DEFINED
#   define ITab MacITab
#   define _OBJBASE_H_
#   undef INC_OLE2
#endif

#ifndef X_SHLWRAP_H_
#define X_SHLWRAP_H_
#include "shlwrap.h"
#endif

#include <w4warn.h>

#ifndef X_WINDOWS_H_
#define X_WINDOWS_H_
#include <windows.h>
#endif

#include <w4warn.h> // windows.h reenables some pragmas

#ifndef X_WINDOWSX_H_
#define X_WINDOWSX_H_
#include <windowsx.h>
#endif

#ifndef X_OLEACC_H_
#define X_OLEACC_H_
#include <oleacc.h>
#endif

#ifndef X_COMMDLG_H_
#define X_COMMDLG_H_
#include <commdlg.h>
#endif

#ifndef X_DLGS_H_
#define X_DLGS_H_
#include <dlgs.h>
#endif

#ifndef X_UNALIGNED_HPP_
#define X_UNALIGNED_HPP_
#include <unaligned.hpp>
#endif

#undef WINCOMMCTRLAPI
#define WINCOMMCTRLAPI
#include <commctrl.h>

#ifndef X_WRAPDEFS_H_
#define X_WRAPDEFS_H_
#include <wrapdefs.h>
#endif

// Component includes

#ifndef X_DOCOBJ_H_
#define X_DOCOBJ_H_
#include <docobj.h>
#endif

#ifndef X_OLECTL_H_
#define X_OLECTL_H_
#include <olectl.h>
#endif

#ifndef X_OBJEXT_H_
#define X_OBJEXT_H_
#include <objext.h>
#endif

#ifndef X_OLEACC_H_
#define X_OLEACC_H_
#include <oleacc.h>
#endif

#define MSO_NO_ASSERTS

#ifndef X_MULTINFO_H_
#define X_MULTINFO_H_
#include <multinfo.h>
#endif

#ifndef X_MFMWRAP_H_
#define X_MFMWRAP_H_
#include <mfmwrap.h>
#endif

#ifndef X_COREGUID_H_
#define X_COREGUID_H_
#include <coreguid.h>
#endif

#ifndef X_OTHRGUID_H_
#define X_OTHRGUID_H_
#include <othrguid.h>
#endif

#ifndef X_OTHRDISP_H_
#define X_OTHRDISP_H_
#include <othrdisp.h>
#endif

//
// ********* CRunTime Includes
//

#ifndef X_LIMITS_H_
#define X_LIMITS_H_
#include <limits.h>
#endif

#ifndef X_STDDEF_H_
#define X_STDDEF_H_
#include <stddef.h>
#endif

#ifndef X_SEARCH_H_
#define X_SEARCH_H_
#include <search.h>
#endif

#ifndef X_STRING_H_
#define X_STRING_H_
#include <string.h>
#endif

#ifndef X_TCHAR_H_
#define X_TCHAR_H_
#include <tchar.h>
#endif

#ifndef X_SHLWAPI_H_
#define X_SHLWAPI_H_
#include <shlwapi.h>
#endif

#ifndef X_SHLWAPIP_H_
#define X_SHLWAPIP_H_
#include <shlwapip.h>
#endif

//
// *********  Public Forms includes
//
#define  INC_OLE_SRVR_UTILS
#define  INC_STATUS_BAR

#include "w4warn.h"

#ifndef X_COREDISP_H_
#define X_COREDISP_H_
#include "coredisp.h"
#endif

#ifndef X_QI_IMPL_H_
#define X_QI_IMPL_H_
#include <qi_impl.h>
#endif

#ifndef X_F3DEBUG_H_
#define X_F3DEBUG_H_
#include <f3debug.h>
#endif

#ifndef X_F3UTIL_HXX_
#define X_F3UTIL_HXX_
#include <f3util.hxx>
#endif

#ifndef X_CDUTIL_HXX_
#define X_CDUTIL_HXX_
#include <cdutil.hxx>
#endif

#ifndef X_CSTR_HXX_
#define X_CSTR_HXX_
#include <cstr.hxx>
#endif

#ifndef X_FORMSARY_HXX_
#define X_FORMSARY_HXX_
#include <formsary.hxx>
#endif

#ifndef X_DLL_HXX_
#define X_DLL_HXX_
#include <dll.hxx>
#endif

#ifndef X_CDBASE_HXX_
#define X_CDBASE_HXX_
#include <cdbase.hxx>
#endif

#ifndef X_PLATFORM_H_
#define X_PLATFORM_H_
#include <platform.h>
#endif

//
// *********  Private Forms includes
//

#ifndef X_MSHTMLRC_H_
#define X_MSHTMLRC_H_
#include "mshtmlrc.h"
#endif

#ifndef X_UNDO_HXX_
#define X_UNDO_HXX_
#include <undo.hxx>
#endif

//
// *********  Directory-specific includes should stay within that directory.
//

typedef UINT CODEPAGE;
