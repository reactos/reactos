/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS System Libraries
 * FILE:            lib/ntdll/inc/ntdll.h
 * PURPOSE:         Native Libary Header
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ******************************************************************/

/* SDK/DDK/NDK Headers. */
#include <windows.h>
#define NTOS_MODE_USER
#include <ndk/ntndk.h>

/* Helper Macros */
#include <reactos/helper.h>

/* NTDLL Public Headers. FIXME: Combine/clean these after NDK */
#include <ntdll/csr.h>
#include <ntdll/rtl.h>
#include <ntdll/ldr.h>
#include <ntdll/ntdll.h>

/* EOF */
