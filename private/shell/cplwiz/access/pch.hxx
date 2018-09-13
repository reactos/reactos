#ifndef __PRECOMPILED_HEADER_H
#define __PRECOMPILED_HEADER_H

// JMC: IS THIS NEEDED OR CORRECT?
#define _WIN32_WINNT 0x0500

#include <windows.h>
#include <windowsx.h>

#include <winuserp.h>
#include <commctrl.h>
//#include <comctrlp.h>

#include <regstr.h>
// #include "..\..\..\windows\inc\help.h"
#include <crtdbg.h>
#include <tchar.h>
#include <stdio.h>

//#include "msg.h"

#define ARRAYSIZE(a)   (sizeof(a) / sizeof((a)[0]))

extern HINSTANCE g_hInstDll;

#ifdef _DEBUG
#define VERIFY(xxx) _ASSERT(xxx)
#else
#define VERIFY(xxx) xxx
#endif

#endif // __PRECOMPILED_HEADER_H
