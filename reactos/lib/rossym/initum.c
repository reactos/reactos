/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            lib/rossym/initum.c
 * PURPOSE:         Initialize library for use in user mode
 * 
 * PROGRAMMERS:     Ge van Geldorp (gvg@reactos.com)
 */

#include <windows.h>
#include <reactos/rossym.h>
#include "rossympriv.h"

#define NDEBUG
#include <debug.h>

static PVOID
RosSymAllocMemUM(ULONG_PTR Size)
{
  return HeapAlloc(GetProcessHeap(), 0, Size);
}

static VOID
RosSymFreeMemUM(PVOID Area)
{
  HeapFree(GetProcessHeap(), 0, Area);
}

VOID
RosSymInitUserMode(VOID)
{
  static ROSSYM_CALLBACKS KmCallbacks =
    {
      RosSymAllocMemUM,
      RosSymFreeMemUM,
      RosSymZwReadFile,
      RosSymZwSeekFile
    };

  RosSymInit(&KmCallbacks);
}

/* EOF */
