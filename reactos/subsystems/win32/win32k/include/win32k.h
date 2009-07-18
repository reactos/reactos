/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Win32K
 * FILE:            subsystems/win32/win32k/include/win32k.h
 * PURPOSE:         Main Win32K Header
 * PROGRAMMER:      Aleksey Bragin (aleksey@reactos.org)
 */

/* INCLUDES ******************************************************************/

#define _NO_COM

/* DDK/NDK/SDK Headers */
#include <ddk/ntddk.h>
#include <ddk/ntddmou.h>
#include <ddk/ntifs.h>
#include <ddk/tvout.h>
#include <ndk/ntndk.h>

/* Win32 Headers */
/* FIXME: Defines in winbase.h that we need... */
typedef struct _SECURITY_ATTRIBUTES SECURITY_ATTRIBUTES, *LPSECURITY_ATTRIBUTES;
#define WINBASEAPI
#define STARTF_USESIZE 2
#define STARTF_USEPOSITION 4
#include <stdarg.h>
#include <stdlib.h>
#include <windef.h>
#include <winerror.h>
#include <wingdi.h>
#include <winddi.h>
#include <winuser.h>
#include <prntfont.h>
#include <dde.h>
#include <wincon.h>
#define _NOCSECT_TYPE
#include <ddrawi.h>

/* SEH Support with PSEH */
#include <pseh/pseh2.h>

/* CSRSS Header */
#include <csrss/csrss.h>

/* Public Win32K Headers */
#include <win32k/callback.h>
#include <win32k/ntusrtyp.h>
#include <win32k/ntuser.h>
#include <win32k/ntgdityp.h>
#include <win32k/ntgdihdl.h>
#define LANGPACK
#include <psdk/ntgdi.h>

/* Internal  Win32K Header */
#include <win32kp.h>

/* Probe and capture */
#include <reactos/probe.h>
