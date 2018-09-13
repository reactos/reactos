// stdafx.cpp : source file that includes just the standard includes
//  stdafx.pch will be the pre-compiled header
//  stdafx.obj will contain the pre-compiled type information


#include "priv.h"

#ifndef FAVORITESTOSHDOCVW

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
#endif
