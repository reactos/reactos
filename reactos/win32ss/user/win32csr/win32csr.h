/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            subsys/csrss/win32csr/win32csr.h
 * PURPOSE:         Interface to win32csr.dll
 */

#pragma once

#include <objbase.h>

extern HANDLE Win32CsrApiHeap;
extern HINSTANCE Win32CsrDllHandle;

typedef struct Object_tt
{
  LONG Type;
  struct tagCSRSS_CONSOLE *Console;
  LONG AccessRead, AccessWrite;
  LONG ExclusiveRead, ExclusiveWrite;
  LONG HandleCount;
} Object_t;

typedef struct _CSRSS_HANDLE
{
  Object_t *Object;
  DWORD Access;
  BOOL Inheritable;
  DWORD ShareMode;
} CSRSS_HANDLE, *PCSRSS_HANDLE;

typedef VOID (WINAPI *CSR_CLEANUP_OBJECT_PROC)(Object_t *Object);

typedef struct tagCSRSS_OBJECT_DEFINITION
{
  LONG Type;
  CSR_CLEANUP_OBJECT_PROC CsrCleanupObjectProc;
} CSRSS_OBJECT_DEFINITION, *PCSRSS_OBJECT_DEFINITION;

/* handle.c */
NTSTATUS FASTCALL Win32CsrInsertObject(PCSR_PROCESS ProcessData,
                                       PHANDLE Handle,
                                       Object_t *Object,
                                       DWORD Access,
                                       BOOL Inheritable,
                                       DWORD ShareMode);
NTSTATUS FASTCALL Win32CsrLockObject(PCSR_PROCESS ProcessData,
                                     HANDLE Handle,
                                     Object_t **Object,
                                     DWORD Access,
                                     long Type);
VOID FASTCALL Win32CsrUnlockObject(Object_t *Object);
NTSTATUS FASTCALL Win32CsrReleaseObject(PCSR_PROCESS ProcessData,
                                        HANDLE Object);
VOID WINAPI Win32CsrReleaseConsole(PCSR_PROCESS ProcessData);
NTSTATUS WINAPI Win32CsrDuplicateHandleTable(PCSR_PROCESS SourceProcessData,
                                             PCSR_PROCESS TargetProcessData);
CSR_API(CsrGetHandle);
CSR_API(CsrCloseHandle);
CSR_API(CsrVerifyHandle);
CSR_API(CsrDuplicateHandle);
CSR_API(CsrGetInputWaitHandle);

BOOL FASTCALL Win32CsrValidateBuffer(PCSR_PROCESS ProcessData,
                                     PVOID Buffer,
                                     SIZE_T NumElements,
                                     SIZE_T ElementSize);
NTSTATUS FASTCALL Win32CsrEnumProcesses(CSRSS_ENUM_PROCESS_PROC EnumProc,
                                        PVOID Context);

/* exitros.c */
CSR_API(CsrExitReactos);
CSR_API(CsrSetLogonNotifyWindow);
CSR_API(CsrRegisterLogonProcess);

CSR_API(CsrSoundSentry);

/* EOF */
