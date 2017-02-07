/* 
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           
 * FILE:              subsys/win32k/eng/debug.c
 * PROGRAMER:         Jason Filby
 */

#include <win32k.h>

#define NDEBUG
#include <debug.h>

/*
 * @implemented
 */
VOID APIENTRY
EngDebugPrint(PCHAR StandardPrefix,
	      PCHAR DebugMessage,
	      va_list ap)
{
    vDbgPrintExWithPrefix(StandardPrefix,
                          -1,
                          DPFLTR_ERROR_LEVEL,
                          DebugMessage,
                          ap);
}

/* EOF */
