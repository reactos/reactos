/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           GDI Driver Brush Functions
 * FILE:              subsys/win32k/eng/debug.c
 * PROGRAMER:         Jason Filby
 * REVISION HISTORY:
 *                 11/7/1999: Created
 */

#include <ddk/ntddk.h>

VOID STDCALL EngDebugPrint(PCHAR StandardPrefix, PCHAR DebugMessage, va_list ap)
{
  DbgPrint(StandardPrefix);
  DbgPrint(DebugMessage, ap);
  DbgPrint("\n");
}
