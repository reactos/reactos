/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            include/kernel32/process.h
 * PURPOSE:         Include file for lib/kernel32/proc.c
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */


#include <windows.h>
#include <ddk/ntddk.h>


NT_PEB *GetCurrentPeb(VOID);

typedef 
DWORD
(*WaitForInputIdleType)(
    HANDLE hProcess,
    DWORD dwMilliseconds);



