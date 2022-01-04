/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:
 * FILE:              win32ss/gdi/eng/debug.c
 * PROGRAMER:         Jason Filby
 */

#include <win32k.h>

#define NDEBUG
#include <debug.h>

/*
 * @implemented
 */
VOID
APIENTRY
EngDebugPrint(
    _In_z_ PCHAR StandardPrefix,
    _In_z_ PCHAR DebugMessage,
    _In_ va_list ap)
{
    vDbgPrintExWithPrefix(StandardPrefix,
                          -1,
                          DPFLTR_ERROR_LEVEL,
                          DebugMessage,
                          ap);
}

/* EOF */
