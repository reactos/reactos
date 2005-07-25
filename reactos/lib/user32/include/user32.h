/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS System Libraries
 * FILE:            lib/user32/include/user32.h
 * PURPOSE:         Win32 User Library
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ******************************************************************/

#ifndef USER32_H
#define USER32_H

/* SDK/NDK Headers */
#include <windows.h>
#include <windowsx.h>
#define NTOS_MODE_USER
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

/* Internal User32 Headers */
#include "user32p.h"

/* FIXME: FILIP */
HGDIOBJ STDCALL  NtGdiSelectObject(HDC  hDC, HGDIOBJ  hGDIObj);
DWORD STDCALL GdiGetCharDimensions(HDC, LPTEXTMETRICW, DWORD *);

#endif /* USER32_H */
