/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            lib/rossym/initum.c
 * PURPOSE:         Initialize library for use in user mode
 *
 * PROGRAMMERS:     Ge van Geldorp (gvg@reactos.com)
 */

#define WIN32_NO_STATUS
#include <windows.h>
#include <reactos/rossym.h>
#include "rossympriv.h"
#define NTOS_MODE_USER
#include <ndk/ntndk.h>
#include <pseh/pseh.h>

#define NDEBUG
#include <debug.h>

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
RosSymGetMemUM(ULONG_PTR *Target, PVOID SourceMem, ULONG Size)
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
