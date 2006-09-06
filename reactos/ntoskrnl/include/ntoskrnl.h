/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel
 * FILE:            ntoskrnl/include/ntoskrnl.h
 * PURPOSE:         Main Kernel Header
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ******************************************************************/

/* We are the Kernel */
#define NTKERNELAPI
#define _NTSYSTEM_

/* DDK/IFS/NDK Headers */
#ifdef __GNUC__
#include <ntddk.h>
#endif
#include <ntifs.h>
#include <wdmguid.h>
#include <arc/arc.h>
#include <ndk/ntndk.h>
#undef TEXT
#define TEXT(s) L##s
#include <regstr.h>

/* FIXME: Temporary until CC Ros is gone */
#include <ccros.h>
#include <rosldr.h>

/* Disk Dump Driver Header */
#include <diskdump/diskdump.h>

/* C Headers */
#include <stdio.h>
#include <ctype.h>
#include <malloc.h>
#include <wchar.h>

/* SEH support with PSEH */
#include <pseh/pseh.h>

/* ReactOS Headers */
#include <reactos/version.h>
#include <reactos/resource.h>
#include <reactos/bugcodes.h>
#include <reactos/rossym.h>
#define ExRaiseStatus RtlRaiseStatus
#include <reactos/probe.h>

/* PNP GUIDs */
#include <umpnpmgr/sysguid.h>

/* Helper Header */
#include <reactos/helper.h>

/* Internal Headers */
#include "internal/ntoskrnl.h"
#include "config.h"
