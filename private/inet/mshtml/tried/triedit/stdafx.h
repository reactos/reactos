// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently
// Copyright (c)1997-1999 Microsoft Corporation, All Rights Reserved

#if !defined(AFX_STDAFX_H__438DA5D5_F171_11D0_984E_0000F80270F8__INCLUDED_)
#define AFX_STDAFX_H__438DA5D5_F171_11D0_984E_0000F80270F8__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#ifndef STRICT
#define STRICT
#endif

#define _ATL_STATIC_REGISTRY

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif

#define _ATL_APARTMENT_THREADED

#ifdef _UNICODE
#ifndef UNICODE
#define UNICODE         // UNICODE is used by Windows headers
#endif
#endif

#ifdef UNICODE
#ifndef _UNICODE
#define _UNICODE        // _UNICODE is used by C-runtime/MFC headers
#endif
#endif

#ifdef _DEBUG
#ifndef DEBUG
#define DEBUG
#endif
#endif

#pragma warning(disable: 4505)	// unreferenced local function has been removed

#include <windows.h>

// Note that this include MUST be at this location (after the above include)
#include "win95wrp.h"

#include <atlbase.h>

//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
#pragma warning(disable: 4100 4189 4244)	// Necessary for ia64 build
#include <atlcom.h>
#include <atlwin.h>
#pragma warning(default: 4100 4189 4244)	// Necessary for ia64 build

#include <mshtml.h>

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__438DA5D5_F171_11D0_984E_0000F80270F8__INCLUDED)
