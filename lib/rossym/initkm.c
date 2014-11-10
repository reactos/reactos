/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            lib/rossym/initkm.c
 * PURPOSE:         Initialize library for use in kernel mode
 *
 * PROGRAMMERS:     Ge van Geldorp (gvg@reactos.com)
 */

#include <ntdef.h>
#undef DECLSPEC_IMPORT
#define DECLSPEC_IMPORT
#include <wdm.h>
#include <reactos/rossym.h>
#include "rossympriv.h"

#define TAG_ROSSYM 'MYSR'

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
