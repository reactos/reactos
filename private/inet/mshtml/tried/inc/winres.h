// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (C) 1992-1999 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

// winres.h - Windows resource definitions
//  extracted from WINUSER.H and COMMCTRL.H

#ifdef _AFX_MINREBUILD
#pragma component(minrebuild, off)
#endif

#define VS_VERSION_INFO     1

#ifdef APSTUDIO_INVOKED
#define APSTUDIO_HIDDEN_SYMBOLS // Ignore following symbols
#endif

#ifndef WINVER
#define WINVER 0x0400   // default to Windows Version 4.0
#endif

#include <winresrc.h>

#ifdef _MAC
#define DS_WINDOWSUI    0x8000L
#endif

// operation messages sent to DLGINIT
#define LB_ADDSTRING    (WM_USER+1)
#define CB_ADDSTRING    (WM_USER+3)

#ifdef APSTUDIO_INVOKED
#undef APSTUDIO_HIDDEN_SYMBOLS
#endif

#ifdef IDC_STATIC
#undef IDC_STATIC
#endif
#define IDC_STATIC      (-1)

#ifdef _AFX_MINREBUILD
#pragma component(minrebuild, on)
#endif
