/* $Id: win32csr.h 55699 2012-02-19 06:44:09Z ion $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            subsys/csrss/win32csr/win32csr.h
 * PURPOSE:         Interface to win32csr.dll
 */

#pragma once

#include <windows.h>
#include <commctrl.h>

extern HANDLE Win32CsrApiHeap;
extern HINSTANCE Win32CsrDllHandle;

/*
typedef VOID (WINAPI *CSR_CLEANUP_OBJECT_PROC)(Object_t *Object);

typedef struct tagCSRSS_OBJECT_DEFINITION
{
  LONG Type;
  CSR_CLEANUP_OBJECT_PROC CsrCleanupObjectProc;
} CSRSS_OBJECT_DEFINITION, *PCSRSS_OBJECT_DEFINITION;
*/

NTSTATUS FASTCALL Win32CsrEnumProcesses(CSRSS_ENUM_PROCESS_PROC EnumProc,
                                        PVOID Context);

/* exitros.c */
/// Must go to winsrv.dll
CSR_API(CsrExitReactos);
CSR_API(CsrSetLogonNotifyWindow);
CSR_API(CsrRegisterLogonProcess);

/* EOF */
