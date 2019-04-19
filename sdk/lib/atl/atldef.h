/*
* PROJECT:     ReactOS ATL
* LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
* PURPOSE:     ATL Base definitions
* COPYRIGHT:   Copyright 2019 Mark Jansen (mark.jansen@reactos.org)
*/

#pragma once

#ifndef __REACTOS__
    #include <cstddef>
    #include <pseh/pseh2.h>
#endif

#define _ATL_PACKING 8


#ifndef AtlThrow
#ifndef _ATL_CUSTOM_THROW
#define AtlThrow(x) ATL::AtlThrowImp(x)
#endif
#endif


#ifndef ATLASSERT
#define ATLASSERT(expr) _ASSERTE(expr)
#endif


// ATLASSUME, ATLENSURE, ATLVERIFY, ...





#ifdef _ATL_DISABLE_NO_VTABLE
#define ATL_NO_VTABLE
#else
#define ATL_NO_VTABLE __declspec(novtable)
#endif

#ifndef ATL_DEPRECATED
#define ATL_DEPRECATED __declspec(deprecated)
#endif

// ATL_NOTHROW, ATL_FORCEINLINE, ATL_NOINLINE

// _ATL, ATL_VER, ATL_FILENAME_VER, ATL_FILENAME_VERNUM, ...



#define offsetofclass(base, derived) (reinterpret_cast<DWORD_PTR>(static_cast<base *>(reinterpret_cast<derived *>(_ATL_PACKING))) - _ATL_PACKING)



#ifndef _ATL_FREE_THREADED
#ifndef _ATL_APARTMENT_THREADED
#ifndef _ATL_SINGLE_THREADED
#define _ATL_FREE_THREADED
#endif
#endif
#endif

