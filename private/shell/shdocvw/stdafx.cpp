// stdafx.cpp : source file that includes just the standard includes
//  stdafx.pch will be the pre-compiled header
//  stdafx.obj will contain the pre-compiled type information


#include "priv.h"

#ifndef _ATL_STATIC_REGISTRY
#define _ATL_STATIC_REGISTRY
#endif

#include "stdafx.h"

// HACKHACK (scotth): windowsx.h #define SubclassWindow.  ATL 2.1 headers
//  (namely atlwin.h and atlwin.cpp) have a member function with the
//  same name.
#ifdef SubclassWindow
#undef SubclassWindow
#endif

#undef  lstrcmpi
#define lstrcmpi StrCmpI
#undef  lstrcpyn
#define lstrcpyn StrCpyN
#undef  lstrcat
#define lstrcat  StrCatW
#undef  lstrcpy
#undef  StrCpyW
#define lstrcpy    StrCpyW
#undef  wsprintf
#undef  wsprintfW
#define wsprintf   wsprintfW

#ifdef _ATL_STATIC_REGISTRY
#include <statreg.h>
#include <statreg.cpp>
#endif

#undef ATL_DEBUG_QI 
#include <atlimpl.cpp>

//HACK!!!!
#define OleCreatePropertyFrame(a, b, c, d, e, f, g, h, i, j, k) S_OK

#include <atlctl.cpp>
#include <atlwin.cpp>

#undef  lstrcpy
#undef  wsprintf
#undef  lstrcmpi
#undef  lstrcpyn
#undef  lstrcat
#define lstrcmpi    Do_not_use_lstrcmpi_use_StrCpyNI
#define lstrcpyn    Do_not_use_lstrcpyn_use_StrCpyN
#define lstrcpy     Do_not_use_lstrcpy_use_StrCpyN
#define lstrcat     Do_not_use_lstrcat_use_StrCatBuff
#define wsprintf    Do_not_use_wsprintf_use_wnsprintf
#define StrCpyW     Do_not_use_StrCpyW_use_StrCpyNW

