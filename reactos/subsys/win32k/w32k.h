/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Graphics Subsystem
 * FILE:            subsys/win32k/w32k.h
 * PURPOSE:         Main Win32K Header
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ******************************************************************/

/* We are Win32K */
#define __WIN32K__

/* ReactOS Config */
#include <roscfg.h>

/* DDK/NDK/SDK Headers */
#include <ddk/ntddk.h>
#include <ddk/ntifs.h>
#include <ddk/ddrawint.h>
#include <ddk/d3dnthal.h>
#include <ddk/winddi.h>
#include <ddk/ntddmou.h>
#include <windows.h>    /* FIXME ? */
#include <windowsx.h>   /* FIXME ? */
#include <ndk/ntndk.h>

/* FIXME: ReactOS will be R-Rated if I really write what I'm about to */
NTSTATUS 
STDCALL
MmCopyFromCaller(PVOID Dest, const VOID *Src, ULONG NumberOfBytes);
NTSTATUS 
STDCALL
MmCopyToCaller(PVOID Dest, const VOID *Src, ULONG NumberOfBytes);

/* SEH Support with PSEH */
#include <pseh.h>

/* CSRSS Header */
#include <csrss/csrss.h>

/* Helper Header */
#include <reactos/helper.h>

/* External Win32K Header */
#include <win32k/win32k.h>
#include <win32k/win32.h>

/* Internal Win32K Header */
#include "include/win32k.h"

