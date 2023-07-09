// stdafx.cpp : source file that includes just the standard includes
//  stdafx.pch will be the pre-compiled header
//  stdafx.obj will contain the pre-compiled type information

#include "priv.h"

// Do not build this file if on Win9X or NT4
#ifndef DOWNLEVEL_PLATFORM
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

#include <atlimpl.cpp>
#include <atlctl.cpp>
#include <atlwin.cpp>

#endif //DOWNLEVEL_PLATFORM
