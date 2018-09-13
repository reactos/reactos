// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#if !defined(AFX_STDAFX_H__2961904F_D2F6_11D0_BDE6_00AA001A1953__INCLUDED_)
#define AFX_STDAFX_H__2961904F_D2F6_11D0_BDE6_00AA001A1953__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#define STRICT
#define _WIN32_WINNT	0x0400
//#define _WIN32_IE		0x0500

#include <atlbase.h>

//===================================================================
// Project-specific includes
//===================================================================

/*
#define    NO_SHLWAPI_PATH       // Path functions
#define	   NO_SHLWAPI_INTERNAL   // Other random internal things
#define    NO_SHLWAPI_UALSTR     // Unaligned string functions
#define    NO_SHLWAPI_STREAM     // Stream functions
#define    NO_SHLWAPI_HTTP       // HTTP helper routines
#define    NO_SHLWAPI_GDI        // GDI helper functions
#define    NO_SHLWAPI_TPS        // Thread Pool Services

#include <shlwapi.h>
#include <shlwapip2.h>
#include <w95wraps.h>
*/

#include <list>
#include <assert.h>		// to disable asserts, define NDEBUG
#include "memmgr.h"		// custom memory manager definitions

//--------------------------------------------------
// MSAA SDK includes
//--------------------------------------------------

#include <oleacc.h>
#include <winable.h>
#include <winuser.h>

//--------------------------------------------------
// Trident includes
//--------------------------------------------------

#include <mshtml.h>
#include <mshtmdid.h>
#include <dispex.h>

//--------------------------------------------------
// we need to ensure dynamic binding to these methods.
// they should always be bound at run time.
//
// TODO: comment out the methods that are being
// run time bound.
//--------------------------------------------------

#define SetWinEventHook					FFFFFFFF
#define UnhookWinEvent					FFFFFFFF
#define BlockInput						FFFFFFFF
#define SendInput						FFFFFFFF
#define GetGUIThreadInfo				FFFFFFFF
#define GetWindowModuleFileNameW		FFFFFFFF
#define GetWindowModuleFileNameA		FFFFFFFF
#define NotifyWinEvent					FFFFFFFF
#define ObjectFromLresult				FFFFFFFF
#define CreateStdAccessibleObject		FFFFFFFF
#define AccessibleChildren				FFFFFFFF
#define AccessibleObjectFromEvent		FFFFFFFF
#define AccessibleObjectFromPoint		FFFFFFFF
#define AccessibleObjectFromWindow		FFFFFFFF
#define GetRoleTextW					FFFFFFFF
#define GetRoleTextA					FFFFFFFF
#define GetStateTextW					FFFFFFFF
#define GetStateTextA					FFFFFFFF
#define LresultFromObject				FFFFFFFF
#define WindowFromAccessibleObject		FFFFFFFF

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__2961904F_D2F6_11D0_BDE6_00AA001A1953__INCLUDED)
