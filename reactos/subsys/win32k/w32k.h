/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Graphics Subsystem
 * FILE:            subsys/win32k/w32k.h
 * PURPOSE:         Main Win32K Header
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ******************************************************************/

/* DDK/NDK/SDK Headers */
#include <ddk/ntddk.h>
#include <ddk/ntddmou.h>
#include <ddk/ntifs.h>
#include <ndk/ntndk.h>

/* Win32 Headers */
/* FIXME: Defines in winbase.h that we need... */
typedef struct _SECURITY_ATTRIBUTES SECURITY_ATTRIBUTES, *LPSECURITY_ATTRIBUTES;
#define WINBASEAPI
#define STARTF_USESIZE 2
#define STARTF_USEPOSITION 4
#include <stdarg.h>
#include <windef.h>
#include <wingdi.h>
#include <winddi.h>
#include <winuser.h>
#include <dde.h>
#include <wincon.h>

/* SEH Support with PSEH */
#include <pseh/pseh.h>

/* CSRSS Header */
#include <csrss/csrss.h>

/* Helper Header */
#include <reactos/helper.h>

/* External Win32K Header */
#include <win32k/win32k.h>
#include <win32k/win32.h>

/* Internal Win32K Header */
#include "include/win32k.h"
