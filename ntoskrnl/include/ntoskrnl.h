/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel
 * FILE:            ntoskrnl/include/ntoskrnl.h
 * PURPOSE:         Main Kernel Header
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 */

#ifndef _NTOSKRNL_PCH
#define _NTOSKRNL_PCH

/* INCLUDES ******************************************************************/

/* ARM Bringup Hack */
#ifdef _M_ARM
#define DbgPrint DbgPrintEarly
#endif

/* WDK hacks */
#ifdef _M_AMD64
#define IoAllocateAdapterChannel _IoAllocateAdapterChannel
#define KeGetCurrentThread _KeGetCurrentThread
#define RtlFillMemoryUlong _RtlFillMemoryUlong
#endif

/* Version Data */
#undef __MSVCRT__
#include <psdk/ntverp.h>

/* DDK/IFS/NDK Headers */
#define _REALLY_GET_CALLERS_CALLER
#include <excpt.h>
#include <ntdef.h>
#include <ntifs.h>
#include <wdmguid.h>
#include <diskguid.h>
#include <arc/arc.h>
#include <mountmgr.h>
#undef NTHALAPI
#define NTHALAPI __declspec(dllimport)
#include <ndk/asm.h>
#include <ndk/cctypes.h>
#include <ndk/cmfuncs.h>
#include <ndk/dbgkfuncs.h>
#include <ndk/exfuncs.h>
#include <ndk/halfuncs.h>
#include <ndk/inbvfuncs.h>
#include <ndk/iofuncs.h>
#include <ndk/kdfuncs.h>
#include <ndk/kefuncs.h>
#include <ndk/ldrfuncs.h>
#include <ndk/lpcfuncs.h>
#include <ndk/mmfuncs.h>
#include <ndk/muptypes.h>
#include <ndk/obfuncs.h>
#include <ndk/pofuncs.h>
#include <ndk/psfuncs.h>
#include <ndk/rtlfuncs.h>
#include <ndk/sefuncs.h>
#include <ndk/vftypes.h>
#undef TEXT
#define TEXT(s) L##s
#include <regstr.h>
#include <ntstrsafe.h>
#include <ntpoapi.h>
#include <ntintsafe.h>

/* C Headers */
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <malloc.h>
#include <wchar.h>

/* SEH support with PSEH */
#include <pseh/pseh2.h>

/* SetupLDR Support */
#include <arc/setupblk.h>

/* KD Support */
#define NOEXTAPI
#include <windbgkd.h>
#include <wdbgexts.h>
#include <kddll.h>
#ifdef __ROS_ROSSYM__
#include <reactos/rossym.h>
#endif

/* PNP GUIDs */
#include <umpnpmgr/sysguid.h>

/* SRM header */
#include <srmp.h>

#define ExRaiseStatus RtlRaiseStatus

/* Also defined in fltkernel.h, but we don't want the entire header */
#ifndef Add2Ptr
#define Add2Ptr(P,I) ((PVOID)((PUCHAR)(P) + (I)))
#endif
#ifndef PtrOffset
#define PtrOffset(B,O) ((ULONG)((ULONG_PTR)(O) - (ULONG_PTR)(B)))
#endif

//
// Switch for enabling global page support
//

//#define _GLOBAL_PAGES_ARE_AWESOME_


/* Internal Headers */
#include "internal/ntoskrnl.h"
#include "config.h"

#include <reactos/probe.h>
#include "internal/probe.h"
#include "resource.h"

#endif /* _NTOSKRNL_PCH */
