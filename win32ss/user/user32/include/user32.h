/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS System Libraries
 * FILE:            win32ss/user/user32/include/user32.h
 * PURPOSE:         Win32 User Library
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 */

#ifndef _USER32_PCH_
#define _USER32_PCH_

/* INCLUDES ******************************************************************/

/* C Headers */
#include <stdio.h>

/* PSDK/NDK Headers */

#define _USER32_
#define OEMRESOURCE
#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#include <windef.h>
#include <winbase.h>
#include <wingdi.h>
#include <winreg.h>
#include <winuser.h>
#include <imm.h>
#include <ddeml.h>
#include <dde.h>
#include <windowsx.h>

/* Undocumented user definitions*/
#include <undocuser.h>

#define NTOS_MODE_USER
#include <ndk/kefuncs.h>
#include <ndk/obfuncs.h>
#include <ndk/rtlfuncs.h>

/* PSEH for SEH Support */
#include <pseh/pseh2.h>

/* Public Win32K Headers */
#include <ntusrtyp.h>
#include <ntuser.h>
#include <callback.h>

/* CSRSS Header */
#include <csr/csr.h>
#include <win/winmsg.h>

/* WINE Headers */
#include <wine/unicode.h>
#include <wine/debug.h>

#if defined(_UNITY_BUILD_ENABLED_)
WINE_FORWARD_DECLARE_DEBUG_CHANNEL(button);
WINE_FORWARD_DECLARE_DEBUG_CHANNEL(combo);
WINE_FORWARD_DECLARE_DEBUG_CHANNEL(cursor);
WINE_FORWARD_DECLARE_DEBUG_CHANNEL(ddeml);
WINE_FORWARD_DECLARE_DEBUG_CHANNEL(edit);
WINE_FORWARD_DECLARE_DEBUG_CHANNEL(icon);
WINE_FORWARD_DECLARE_DEBUG_CHANNEL(listbox);
WINE_FORWARD_DECLARE_DEBUG_CHANNEL(mdi);
WINE_FORWARD_DECLARE_DEBUG_CHANNEL(menu);
WINE_FORWARD_DECLARE_DEBUG_CHANNEL(message);
WINE_FORWARD_DECLARE_DEBUG_CHANNEL(relay);
WINE_FORWARD_DECLARE_DEBUG_CHANNEL(resource);
WINE_FORWARD_DECLARE_DEBUG_CHANNEL(scrollbar);
WINE_FORWARD_DECLARE_DEBUG_CHANNEL(static);
WINE_FORWARD_DECLARE_DEBUG_CHANNEL(text);
WINE_FORWARD_DECLARE_DEBUG_CHANNEL(user32);
WINE_FORWARD_DECLARE_DEBUG_CHANNEL(winsta);
#endif

/* Internal User32 Headers */
#include "user32p.h"

/* User macros */
#include "user_x.h"

/* FIXME: this should be in a "public" GDI32 header */
typedef struct _PATRECT
{
    RECT r;
    HBRUSH hBrush;
} PATRECT, * PPATRECT;

#endif /* _USER32_PCH_ */
