/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            lib/rossym/initkm.c
 * PURPOSE:         Initialize library for use in kernel mode
 *
 * PROGRAMMERS:     Ge van Geldorp (gvg@reactos.com)
 */

#define _NTOSKRNL_
#include <ntddk.h>
#include <reactos/rossym.h>
#include "rossympriv.h"

#define NDEBUG
#include <debug.h>

#define TAG(A, B, C, D) (ULONG)(((A)<<0) + ((B)<<8) + ((C)<<16) + ((D)<<24))
#define TAG_ROSSYM TAG('R', 'S', 'Y', 'M')

static PVOID
RosSymAllocMemKM(ULONG_PTR Size)
{
  return ExAllocatePoolWithTag(NonPagedPool, Size, TAG_ROSSYM);
}

static VOID
RosSymFreeMemKM(PVOID Area)
{
  ExFreePool(Area);
}

VOID
RosSymInitKernelMode(VOID)
{
  static ROSSYM_CALLBACKS KmCallbacks =
    {
      RosSymAllocMemKM,
      RosSymFreeMemKM,
      RosSymZwReadFile,
      RosSymZwSeekFile
    };

  RosSymInit(&KmCallbacks);
}

/* EOF */
