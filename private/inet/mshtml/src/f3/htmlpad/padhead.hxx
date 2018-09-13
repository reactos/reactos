//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       Common header file for the Core project.
//
//  Note:       This file is very order-dependent.  Don't switch files around
//              just for the heck of it!
//
//----------------------------------------------------------------------------

#define _OLEAUT32_
#define INC_OLE2
#define WIN32_LEAN_AND_MEAN
#define OEMRESOURCE
#define __types_h__  // prevent inclusion from cdutil

// Windows includes

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

#ifndef X_COMMDLG_H_
#define X_COMMDLG_H_
#include <commdlg.h>
#endif

// C runtime includes

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

// Debug file

#if DBG == 0
#define DBG_SAVE 0
#elif DBG == 1
#define DBG_SAVE 1
#elif DBG == 2
#define DBG_SAVE 2
#elif DBG == 3
#define DBG_SAVE 3
#endif

#undef DBG
#define DBG 1

#ifndef X_F3DEBUG_H_
#define X_F3DEBUG_H_
#include <f3debug.h>
#endif

#undef DBG
#define DBG DBG_SAVE

#if DBG_SAVE==0

#undef TFAIL
#undef TW32
#undef THR

#undef TFAIL_NOTRACE
#undef TW32_NOTRACE
#undef THR_NOTRACE

#undef IGNORE_FAIL
#undef IGNORE_W32
#undef IGNORE_HR

#define TFAIL(e, x)             (x)
#define TW32(e, x)              (x)
#define THR(x)                  (x)

#define TFAIL_NOTRACE(e, x)     (x)
#define TW32_NOTRACE(e, x)      (x)
#define THR_NOTRACE(x)          (x)

#define IGNORE_FAIL(e, x)       (x)
#define IGNORE_W32(e,x)         (x)
#define IGNORE_HR(x)            (x)

#undef SRETURN
#undef RRETURN
#undef RRETURN1
#undef RRETURN2
#undef RRETURN3
#undef RRETURN4
#undef SRETURN_NOTRACE
#undef RRETURN_NOTRACE
#undef RRETURN1_NOTRACE
#undef RRETURN2_NOTRACE
#undef RRETURN3_NOTRACE
#undef RRETURN4_NOTRACE

#define SRETURN(hr)                 return (hr)
#define RRETURN(hr)                 return (hr)
#define RRETURN1(hr, s1)            return (hr)
#define RRETURN2(hr, s1, s2)        return (hr)
#define RRETURN3(hr, s1, s2, s3)    return (hr)
#define RRETURN4(hr, s1, s2, s3, s4)return (hr)

#define SRETURN_NOTRACE(hr)                 return (hr)
#define RRETURN_NOTRACE(hr)                 return (hr)
#define RRETURN1_NOTRACE(hr, s1)            return (hr)
#define RRETURN2_NOTRACE(hr, s1, s2)        return (hr)
#define RRETURN3_NOTRACE(hr, s1, s2, s3)    return (hr)
#define RRETURN4_NOTRACE(hr, s1, s2, s3, s4)return (hr)

#endif

// Core includes

#include <w4warn.h>

#ifndef X_WRAPDEFS_H_
#define X_WRAPDEFS_H_
#include <wrapdefs.h>
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

// Site includes

#ifndef X_MSHTMCID_H_
#define X_MSHTMCID_H_
#include <mshtmcid.h>
#endif

#ifndef X_MSHTMDID_H_
#define X_MSHTMDID_H_
#include <mshtmdid.h>
#endif

#ifndef X_MSHTMHST_H_
#define X_MSHTMHST_H_
#include <mshtmhst.h>
#endif

struct PROPERTYDESC_BASIC_ABSTRACT;
struct PROPERTYDESC_BASIC;
struct PROPERTYDESC_NUMPROP_ABSTRACT;
struct PROPERTYDESC_NUMPROP;
struct PROPERTYDESC_NUMPROP_GETSET;
struct PROPERTYDESC_NUMPROP_ENUMREF;
struct PROPERTYDESC_METHOD;
struct PROPERTYDESC_CSTR_GETSET;

#ifndef X_MSHTMEXT_H_
#define X_MSHTMEXT_H_
#include "mshtmext.h"
#endif

#ifndef X_INTERNAL_H_
#define X_INTERNAL_H_
#include "internal.h"
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

#ifndef X_URLMON_H_
#define X_URLMON_H_
#include <urlmon.h>
#endif

#ifndef X_PERHIST_H_
#define X_PERHIST_H_
#include <perhist.h>
#endif

#ifndef X_ACTIVSCP_H_
#define X_ACTIVSCP_H_
#include <activscp.h>
#endif

#ifndef X_MULTINFO_H_
#define X_MULTINFO_H_
#include <multinfo.h>
#endif

#ifndef X_VERVEC_H_
#define X_VERVEC_H_
#include <vervec.h>
#endif

// Local includes
#ifndef X_THREAD_HXX_
#define X_THREAD_HXX_
#include "thread.hxx"
#endif

#ifndef X_PAD_HXX_
#define X_PAD_HXX_
#include "pad.hxx"
#endif

#ifndef X_COMMCTRL_H_
#define X_COMMCTRL_H_
#define WINCOMMCTRLAPI
#include <commctrl.h>
#endif

#ifndef X_UWININET_H_
#define X_UWININET_H_
#include "uwininet.h"
#endif

#ifndef X_URLMON_H_
#define X_URLMON_H_
#include <urlmon.h>
#endif

#ifndef X_DWNNOT_H_
#define X_DWNNOT_H_
#include <dwnnot.h>
#endif
