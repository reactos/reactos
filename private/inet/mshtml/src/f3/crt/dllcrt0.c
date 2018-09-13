/***
*dllcrt0.c - C runtime initialization routine for a DLL with linked-in C R-T
*
*       Copyright (c) 1989-1995, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       This the startup routine for a DLL which is linked with its own
*       C run-time code.  It is similar to the routine _mainCRTStartup()
*       in the file CRT0.C, except that there is no main() in a DLL.
*
*******************************************************************************/

#include <w4warn.h>
#define NOGDI
#define NOCRYPT
#include <windows.h>

CRITICAL_SECTION s_cs;
CRITICAL_SECTION g_csHeap;

/*
 * flag set iff _CRTDLL_INIT was called with DLL_PROCESS_ATTACH
 */
int __proc_attached = 0;
