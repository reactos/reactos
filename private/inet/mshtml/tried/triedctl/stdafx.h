// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently
// Copyright (c)1997-1999 Microsoft Corporation, All Rights Reserved

#if !defined(AFX_STDAFX_H__683364A5_B37D_11D1_ADC5_006008A5848C__INCLUDED_)
#define AFX_STDAFX_H__683364A5_B37D_11D1_ADC5_006008A5848C__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#ifndef STRICT
#define STRICT
#endif // STRICT

#define _WIN32_WINNT 0x0400
#define _ATL_APARTMENT_THREADED

#ifdef _UNICODE
#ifndef UNICODE
#define UNICODE         // UNICODE is used by Windows headers
#endif
#endif

#ifdef _DEBUG
#ifndef DEBUG
#define DEBUG
#endif
#endif

#pragma warning(disable:4786)	// Turncated label warnings
#pragma warning(disable: 4505)	// unreferenced local function has been removed

#include <windows.h>

// Note that this include MUST be at this location (after the above include)
#include "win95wrp.h"

#include <atlbase.h>

#define AGGREGATE_TRIDENT 0
#define DHTMLEDTRACE OutputDebugString

// DHTMLEdit 1.0 had a requirement to be able to register WITHOUT being able to run.
// Thus, URLMon and WinINet were loaded late and necessary routines were bound as needed.
// This reuqirement went away with 2.0, and stood in the way of Win95Wrap utilization.
//#define LATE_BIND_URLMON_WININET	1


//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
#pragma warning(disable: 4100 4189 4244)	// Necessary for ia64 build
#include <atlcom.h>
#pragma warning(disable: 4510 4610)			// Necessary for Win32 build W4
#include <atlctl.h>
#pragma warning(default: 4510 4610)			// Necessary for Win32 build W4
#pragma warning(default: 4100 4189 4244)	// Necessary for ia64 build

#include <mshtml.h>
#include <mshtmhst.h>
#include <mshtmcid.h>
#include <triedit.h>
#include <triedcid.h>
#include <comdef.h>


//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__683364A5_B37D_11D1_ADC5_006008A5848C__INCLUDED)
