/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel
 * FILE:            ntoskrnl/include/ntoskrnl.h
 * PURPOSE:         Main Kernel Header
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ******************************************************************/

/* Version Data */
#undef __MSVCRT__
#include <psdk/ntverp.h>

/* DDK/IFS/NDK Headers */
#define _REALLY_GET_CALLERS_CALLER_
#ifdef _MSC_VER
#include <excpt.h>
#include <ntdef.h>
#undef DECLSPEC_IMPORT
#define DECLSPEC_IMPORT
#endif
#include <ntifs.h>
#include <wdmguid.h>
#include <arc/arc.h>
#include <ntndk.h>
#undef TEXT
#define TEXT(s) L##s
#include <regstr.h>

/* FIXME: Temporary until CC Ros is gone */
#include <ccros.h>
#include <rosldr.h>

/* Disk Dump Driver Header */
#include <diskdump/diskdump.h>

/* C Headers */
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <malloc.h>
#include <wchar.h>

/* SEH support with PSEH */
#include <pseh/pseh.h>

/* ReactOS Headers */
#include <reactos/buildno.h>
#include <reactos/bugcodes.h>
#define ExRaiseStatus RtlRaiseStatus
#include <reactos/probe.h>

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

/* Helper Header */
#include <reactos/helper.h>

/* Internal Headers */
#include "internal/ntoskrnl.h"
#include "config.h"

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
