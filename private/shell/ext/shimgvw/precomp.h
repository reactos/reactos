// precomp.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#if !defined(PRECOMP_H__50F16B1C_467E_11D1_8271_00C04FC3183B__INCLUDED_)
#define PRECOMP_H__50F16B1C_467E_11D1_8271_00C04FC3183B__INCLUDED_

#define STRICT

#define _ATL_APARTMENT_THREADED

#pragma warning(disable : 4100 4310)

#include <atlbase.h>
#include <shellapi.h>
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
#include <atlcom.h>
#include <atlctl.h>
#include <atlwin.h>

#include <windows.h>
#include <commctrl.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <shlwapip.h>

#pragma warning(default : 4100 4310)

#ifdef _DEBUG
#define ASSERT(x)   _ASSERTE(x)
#else
#define ASSERT(x)
#endif

#define ARRAYSIZE(x)    (sizeof(x)/sizeof(x[0]))

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(PRECOMP_H__50F16B1C_467E_11D1_8271_00C04FC3183B__INCLUDED_)
