/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS System Libraries
 * FILE:            lib/user32/include/user32.h
 * PURPOSE:         Win32 User Library
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ******************************************************************/

/* C Headers */
#include <assert.h>
#include <stdio.h>
#include <math.h>

/* SDK/NDK Headers */
#define _USER32_
#define OEMRESOURCE
#define NTOS_MODE_USER
#define WIN32_NO_STATUS
#include <windows.h>
#include <windowsx.h>
#include <winnls32.h>
#include <strsafe.h>
#include <usp10.h>
#include <ndk/cmfuncs.h>
#include <ndk/kefuncs.h>
#include <ndk/obfuncs.h>
#include <ndk/rtlfuncs.h>
#include <ndk/umfuncs.h>

/* CSRSS Headers */
#include <csrss/csrss.h>

/* Public Win32K Headers */
#include <ntusrtyp.h>
#include <ntuser.h>
#include <callback.h>

/* Undocumented user definitions*/
#include <undocuser.h>

/* WINE Headers */
#include <wine/unicode.h>

/* SEH Support with PSEH */
#include <pseh/pseh2.h>

/* Internal User32 Headers */
#include "user32p.h"

/* User macros */
#include "user_x.h"
