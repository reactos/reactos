/*++

Copyright (c) 1994-1998,  Microsoft Corporation  All rights reserved.

Module Name:

    share.h

Abstract:

    This module contains the header information that is shared between
    the internat.exe application and the indicdll.dll dynamic link library.

Revision History:

--*/



//
//  Constant Declarations.
//

#define WM_MYLANGUAGECHANGE       (WM_USER + 50)
#define WM_MYWINDOWACTIVATED      (WM_MYLANGUAGECHANGE + 1)
#define WM_MYWINDOWCREATED        (WM_MYLANGUAGECHANGE + 2)
#define WM_MYLANGUAGECHECK        (WM_MYLANGUAGECHANGE + 3)

#include <intlshar.h>
#include <debug.h>

#if !defined(INTERNAT_DLL)
  #define WININTERNATAPI DECLSPEC_IMPORT
#else
  #define WININTERNATAPI
#endif

#ifndef ARRAYSIZE
#define ARRAYSIZE(a)                (sizeof(a)/sizeof(a[0]))
#endif

//
//  Definitions for IME status.
//
#define IMESTAT_DISABLED          0
#define IMESTAT_CLOSE             1
#define IMESTAT_OPEN              2
#define IMESTAT_NATIVE            4
#define IMESTAT_FULLSHAPE         8
#define IMESTAT_ERROR            -1

#define IDM_IME_MENUSTART         1000

#define IS_IME_HKL(hkl)           (((DWORD)(hkl) & 0xf0000000) == 0xe0000000)

#ifndef DBG
#define ASSERT(exp)
#define VERIFY(exp)     exp
#else
#define VERIFY(exp)     ASSERT(exp)
#endif
