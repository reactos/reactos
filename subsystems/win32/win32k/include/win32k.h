/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Win32K
 * FILE:            subsystems/win32/win32k/include/win32k.h
 * PURPOSE:         Main Win32K Header
 * PROGRAMMER:      Aleksey Bragin (aleksey@reactos.org)
 */

/* INCLUDES ******************************************************************/

/* Version Data */
#undef __MSVCRT__
#include <psdk/ntverp.h>
#define _WIN32_WINNT _WIN32_WINNT_WS03
#define NTDDI_VERSION NTDDI_WS03SP1
#define WINVER 0x600

/* Initial DDK/IFS Headers */
#ifdef _MSC_VER
#include <excpt.h>
#include <ntdef.h>
#undef DECLSPEC_IMPORT
#define DECLSPEC_IMPORT
#endif
#include <ntifs.h>

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

/* This set of headers is greatly incompatible */
/* TODO: Either fix ddrawi.h + all dependencies, or create a new temporary
         header */
#define _NOCSECT_TYPE
// #include <ddrawi.h>
typedef LPVOID LPVIDMEM;
typedef LPVOID LPVMEMHEAP;
typedef LPVOID LPSURFACEALIGNMENT;

/* NDK Headers */
#include <ntndk.h>

/* SEH Support with PSEH */
#include <pseh/pseh.h>

/* CSRSS Header */
#include <csrss/csrss.h>

/* Helper Header */
#include <reactos/helper.h>

/* Probe and capture */
#include <reactos/probe.h>

/* Public Win32K Headers */
#include <win32k/callback.h>
#include <win32k/ntusrtyp.h>
#include <win32k/ntuser.h>

/* FIXME */
#if 0
#include <win32k/ntgdityp.h>
#include <ntgdi.h>
#endif
