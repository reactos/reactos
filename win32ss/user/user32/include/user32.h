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
#include <psdk/dbt.h>

/* WINE Headers */
#include <wine/unicode.h>
#include <wine/debug.h>

#include <winnls32.h>

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
