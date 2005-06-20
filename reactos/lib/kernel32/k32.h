/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS System Libraries
 * FILE:            lib/ntdll/inc/ntdll.h
 * PURPOSE:         Native Libary Header
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ******************************************************************/

/* PSDK/NDK Headers */
#include <windows.h>
#define NTOS_MODE_USER
#include <ndk/ntndk.h>

/* C Headers */
#include <wchar.h>

/* DDK Driver Headers */
#include <ddk/ntddbeep.h>
#include <ddk/ntddser.h>
#include <ddk/ntddtape.h>

/* FIXME: Clean this sh*t up */
#include <ntdll/base.h>
#include <ntdll/rtl.h>
#include <ntdll/dbg.h>
#include <ntdll/csr.h>
#include <ntdll/ldr.h>

/* Toolhelp & CSRSS Header */
#include <tlhelp32.h>
#include <csrss/csrss.h>

/* FIXME: KILL ROSRTL */
#include <rosrtl/thread.h>
#include <rosrtl/string.h>
#include <rosrtl/registry.h>

/* Internal Kernel32 Header */
#include "include/kernel32.h"

/* PSEH for SEH Support */
#include <pseh.h>

/* Helper Header */
#include <reactos/helper.h>
#include <reactos/buildno.h>

