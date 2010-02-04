#pragma once

#define DBG 1
#define TRAP_DEBUG

// #define CONFIG_SMP

// #define KDBG
#define _WINKD_
// #define AUTO_ENABLE_BOCHS
// #define DbgPrint DbgPrintNo

#define _NTSYSTEM_
#define _NTDLLBUILD_
#define _NTOSKRNL_
#define __NTOSKRNL__
#define _CRTIMP
#define _LIBCNT_
#define _IN_KERNEL_

// #define DECLSPEC_IMPORT
// #define DECLSPEC_EXPORT
#define NTKERNELAPI

#define _SETUPAPI_VER 0x502

#define USE_COMPILER_EXCEPTIONS
#define _CRT_SECURE_NO_WARNINGS
#define _LIB
#define _SEH_ENABLE_TRACE
#define _USE_32BIT_TIME_T

#undef __MSVCRT__

#define SECT_INIT "TINIT"
#pragma section(SECT_INIT, read, execute, shared, nopage, discard)
#define SECT_INIT_FN(x) _SECTION_FN(SECT_INIT, x)
#define SECT_INIT_FND
#define SECT_PAGL "pagelk"
#define SECT_PAGU "pagepo"

#include <reactos_cfg.h>
#include <cpu.h>

// !!! temp disable some warnings
_NOWARN_MSC(4244)	// ULONG to USHORT
_NOWARN_MSC(4018)	// ULONG to USHORT


#include <ntverp.h>

/* DDK/IFS/NDK Headers */
#define _REALLY_GET_CALLERS_CALLER
#include <excpt.h>
#include <ntdef.h>
#undef DECLSPEC_IMPORT
#define DECLSPEC_IMPORT
#include <ntifs.h>
#include <ntdddisk.h>
#include <wdmguid.h>
#include <arc/arc.h>
#undef NTHALAPI
#define NTHALAPI __declspec(dllimport)
#include <ntndk.h>
#undef TEXT
#define TEXT(s) L##s
#include <regstr.h>

/* FIXME: Temporary until Winldr is used */
#include <rosldr.h>

/* C Headers */
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <malloc.h>
#include <wchar.h>
#include <intrin2.h>

/* SEH support with PSEH */
#include <pseh/pseh2.h>

/* ReactOS Headers */
#include <reactos/buildno.h>
#include <reactos/bugcodes.h>

/* SetupLDR Support */
#include <arc/setupblk.h>

/* KD Support */
#define NOEXTAPI
#include <windbgkd.h>
#include <wdbgexts.h>
#include <kddll.h>
#ifndef _WINKD_
#include <reactos/rossym.h>
#endif

/* PNP GUIDs */
#include <umpnpmgr/sysguid.h>

/* Internal Headers */
#include <internal\ntoskrnl.h>
#include "config.h"

#include <rtlp.h>
#include <internal\dbgp.h>
#include <kd64.h>

// _INTRINSIC(_enable)
// _INTRINSIC(_disable)
// _INTRINSIC(_BitScanReverse)
// _INTRINSIC(_BitScanReverse)

//
// Define the internal versions of external and public global data
//
#define IoFileObjectType                _IoFileObjectType
#define PsThreadType                    _PsThreadType
#define PsProcessType                   _PsProcessType
#define ExEventObjectType               _ExEventObjectType
#define ExSemaphoreObjectType           _ExSemaphoreObjectType
#define KdDebuggerEnabled               _KdDebuggerEnabled
#define KdDebuggerNotPresent            _KdDebuggerNotPresent
#define NlsOemLeadByteInfo              _NlsOemLeadByteInfo
extern PUSHORT _NlsOemLeadByteInfo;
#define FsRtlLegalAnsiCharacterArray    _FsRtlLegalAnsiCharacterArray
#undef LEGAL_ANSI_CHARACTER_ARRAY
#undef NLS_MB_CODE_PAGE_TAG
#undef NLS_OEM_LEAD_BYTE_INFO
#define LEGAL_ANSI_CHARACTER_ARRAY      FsRtlLegalAnsiCharacterArray
#define NLS_MB_CODE_PAGE_TAG            NlsMbOemCodePageTag
#define NLS_OEM_LEAD_BYTE_INFO          _NlsOemLeadByteInfo
#undef KD_DEBUGGER_ENABLED
#undef KD_DEBUGGER_NOT_PRESENT
#define KD_DEBUGGER_ENABLED             KdDebuggerEnabled
#define KD_DEBUGGER_NOT_PRESENT         KdDebuggerNotPresent
#define HalDispatchTable                _HalDispatchTable
#undef HALDISPATCH
#define HALDISPATCH                     (&HalDispatchTable)
#define ExRaiseStatus RtlRaiseStatus

void __test(void);
