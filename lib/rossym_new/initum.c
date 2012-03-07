/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            lib/rossym/initum.c
 * PURPOSE:         Initialize library for use in user mode
 *
 * PROGRAMMERS:     Ge van Geldorp (gvg@reactos.com)
 */

#include <precomp.h>

static PVOID
RosSymAllocMemUM(ULONG_PTR Size)
{
  return RtlAllocateHeap(RtlGetProcessHeap(), 0, Size);
}

static VOID
RosSymFreeMemUM(PVOID Area)
{
  RtlFreeHeap(RtlGetProcessHeap(), 0, Area);
}

static BOOLEAN
RosSymGetMemUM(PVOID FileContext, ULONG_PTR *Target, PVOID SourceMem, ULONG Size)
{
  return FALSE;
}

VOID
RosSymInitUserMode(VOID)
{
  static ROSSYM_CALLBACKS KmCallbacks =
    {
      RosSymAllocMemUM,
      RosSymFreeMemUM,
      RosSymZwReadFile,
      RosSymZwSeekFile,
	  RosSymGetMemUM
    };

  RosSymInit(&KmCallbacks);
}

/* EOF */
