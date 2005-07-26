/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS System Libraries
 * FILE:            lib/rtl/rtl.h
 * PURPOSE:         Run-Time Libary Header
 * PROGRAMMER:      Alex Ionescu
 */

/* INCLUDES ******************************************************************/

/* C Headers */
#include <stdio.h>

/* PSDK/NDK Headers */
#include <windows.h>
#define NTOS_MODE_USER
#include <ndk/ntndk.h>

/* Helper Header */
#include <reactos/helper.h>

/* FIXME: Move this somewhere else, maybe */
#ifdef DBG
extern VOID FASTCALL CHECK_PAGED_CODE_RTL(char *file, int line);
#define PAGED_CODE_RTL() CHECK_PAGED_CODE_RTL(__FILE__, __LINE__)
#else
#define PAGED_CODE_RTL()
#endif

extern PVOID RtlpAllocateMemory(UINT Bytes, ULONG Tag);
extern VOID RtlpFreeMemory(PVOID Mem, ULONG Tag);

#define RtlpAllocateStringMemory RtlpAllocateMemory
#define RtlpFreeStringMemory RtlpFreeMemory

#define TAG_USTR        TAG('U', 'S', 'T', 'R')
#define TAG_ASTR        TAG('A', 'S', 'T', 'R')
#define TAG_OSTR        TAG('O', 'S', 'T', 'R')

/* EOF */
