//+------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1998.
//
//  File:       STDAFX.H
//
//  Contents:   Include file for standard ATL include files
//
//-------------------------------------------------------------------------

#if !defined(AFX_STDAFX_H__E3820655_B95E_11D1_A8BB_00C04FB6C702__INCLUDED_)
#define AFX_STDAFX_H__E3820655_B95E_11D1_A8BB_00C04FB6C702__INCLUDED_

// Define the ATL specific macros to get the right set of things
#define _USRDLL

#ifndef _ATL_STATIC_REGISTRY
#define _ATL_STATIC_REGISTRY
#endif

#ifndef _ATL_MIN_CRT
//#define _ATL_MIN_CRT
#endif

#ifndef _ATL_NO_SECURITY
#define _ATL_NO_SECURITY
#endif

//
// NOTE: ATL depends on the *presence* of _DEBUG to switch into DEBUG mode.
// Trident *always* sets _DEBUG to some value. This causes a problem when
// building a retail build in the Trident tree. 
// undef'ing _DEBUG when _DEBUG is zero solves the problem.
//
#if _DEBUG == 0
# undef _DEBUG
#endif

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#define malloc x_malloc
#define free x_free
#define realloc x_realloc

#ifdef UNIX
void  x_free(void *);
void *x_malloc(size_t);
void *x_realloc(void *, size_t);
#endif

#define lstrlenW _tcslen
#define lstrcmpiW StrCmpIW
#define lstrcpynW StrCpyNW
#define lstrcpyW StrCpyW
#define lstrcatW StrCatW

#undef HIMETRIC_PER_INCH

#undef SubclassWindow
#define _WIN32_WINNT 0x0400
#define _ATL_APARTMENT_THREADED

typedef INT Direction;

#pragma warning( disable : 4510 4610 )  

#include <atlbase.h>
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
#include <atlcom.h>
#include <atlctl.h>

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__E3820655_B95E_11D1_A8BB_00C04FB6C702__INCLUDED)
