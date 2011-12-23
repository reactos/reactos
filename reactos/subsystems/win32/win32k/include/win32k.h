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
#undef NTDDI_VERSION
#define NTDDI_VERSION NTDDI_WS03SP1
#include <ntddk.h>
#include <ntddmou.h>
#include <ntifs.h>
#include <tvout.h>
#include <ntndk.h>

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
#define NT_BUILD_ENVIRONMENT
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
#include <psdk/ntgdi.h>

/* Internal  Win32K Header */
#include <win32kp.h>

/* Probe and capture */
#include <reactos/probe.h>
