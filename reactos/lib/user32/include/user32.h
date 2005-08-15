/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS System Libraries
 * FILE:            lib/user32/include/user32.h
 * PURPOSE:         Win32 User Library
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ******************************************************************/

/* C Headers */
#include <stdio.h>
#include <math.h>

/* SDK/NDK Headers */
#define _USER32_
#define OEMRESOURCE
#define NTOS_MODE_USER
#include <windows.h>
#include <windowsx.h>
#include <winnls32.h>
#include <ndk/ntndk.h>

/* CSRSS Headers */
#include <csrss/csrss.h>

/* External Win32K Headers */
#include <win32k/ntuser.h>
#include <win32k/caret.h>
#include <win32k/callback.h>
#include <win32k/cursoricon.h>
#include <win32k/menu.h>
#include <win32k/paint.h>

/* WINE Headers */
#include <wine/debug.h>
#include <wine/unicode.h>

/* Internal User32 Headers */
#include "user32p.h"

/* FIXME: FILIP */
HGDIOBJ STDCALL  NtGdiSelectObject(HDC  hDC, HGDIOBJ  hGDIObj);
DWORD STDCALL GdiGetCharDimensions(HDC, LPTEXTMETRICW, DWORD *);
