// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

// Do not build this file if on Win9X or NT4
#ifndef DOWNLEVEL_PLATFORM

#if !defined(AFX_STDAFX_H__3964D994_AC96_11D1_9851_00C04FD91972__INCLUDED_)
#define AFX_STDAFX_H__3964D994_AC96_11D1_9851_00C04FD91972__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#define STRICT



//#define _WIN32_WINNT 0x0400       // (scotth): Use what is defined in priv.h
#define _ATL_APARTMENT_THREADED

#define _ATL_NO_DEBUG_CRT           // use the shell debug facilities
#include <debug.h>

#include <atlbase.h>
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
#include <atlcom.h>
#include <atlctl.h>

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__3964D994_AC96_11D1_9851_00C04FD91972__INCLUDED)

#endif //DOWNLEVEL_PLATFORM

