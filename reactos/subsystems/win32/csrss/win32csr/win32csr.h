/* $Id$
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

typedef struct Object_tt
{
  LONG Type;
  struct tagCSRSS_CONSOLE *Console;
  LONG HandleCount;
} Object_t;

typedef struct _CSRSS_HANDLE
{
  Object_t *Object;
  DWORD Access;
  BOOL Inheritable;
} CSRSS_HANDLE, *PCSRSS_HANDLE;

typedef VOID (WINAPI *CSR_CLEANUP_OBJECT_PROC)(Object_t *Object);

typedef struct tagCSRSS_OBJECT_DEFINITION
{
  LONG Type;
  CSR_CLEANUP_OBJECT_PROC CsrCleanupObjectProc;
} CSRSS_OBJECT_DEFINITION, *PCSRSS_OBJECT_DEFINITION;

/* handle.c */
NTSTATUS FASTCALL Win32CsrInsertObject(PCSRSS_PROCESS_DATA ProcessData,
                                       PHANDLE Handle,
                                       Object_t *Object,
                                       DWORD Access,
                                       BOOL Inheritable);
NTSTATUS FASTCALL Win32CsrLockObject(PCSRSS_PROCESS_DATA ProcessData,
                                     HANDLE Handle,
                                     Object_t **Object,
                                     DWORD Access,
                                     long Type);
VOID FASTCALL Win32CsrUnlockObject(Object_t *Object);
NTSTATUS FASTCALL Win32CsrReleaseObject(PCSRSS_PROCESS_DATA ProcessData,
                                        HANDLE Object);
NTSTATUS WINAPI Win32CsrReleaseConsole(PCSRSS_PROCESS_DATA ProcessData);
NTSTATUS WINAPI Win32CsrDuplicateHandleTable(PCSRSS_PROCESS_DATA SourceProcessData,
                                             PCSRSS_PROCESS_DATA TargetProcessData);
CSR_API(CsrGetInputHandle);
CSR_API(CsrGetOutputHandle);
CSR_API(CsrCloseHandle);
CSR_API(CsrVerifyHandle);
CSR_API(CsrDuplicateHandle);
CSR_API(CsrGetInputWaitHandle);

NTSTATUS FASTCALL Win32CsrEnumProcesses(CSRSS_ENUM_PROCESS_PROC EnumProc,
                                        PVOID Context);

/* exitros.c */
CSR_API(CsrExitReactos);
CSR_API(CsrSetLogonNotifyWindow);
CSR_API(CsrRegisterLogonProcess);


/* EOF */
