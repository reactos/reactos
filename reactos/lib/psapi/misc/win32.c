/* $Id: win32.c,v 1.2 2002/08/29 23:57:54 hyperion Exp $
 */
/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        reactos/lib/psapi/misc/win32.c
 * PURPOSE:     Win32 interfaces for PSAPI
 * PROGRAMMER:  KJK::Hyperion <noog@libero.it>
 * UPDATE HISTORY:
 *              10/06/2002: Created
 */

#include <windows.h>
#include <psapi.h>
#include <stdlib.h>
#include <ddk/ntddk.h>
#include <internal/psapi.h>

/* Memory allocators for PSAPI */
PVOID STDCALL PsaMalloc(IN OUT PVOID Context, IN ULONG Size)
{
 return malloc(Size);
}

PVOID STDCALL PsaRealloc(IN OUT PVOID Context, IN PVOID Addr, IN ULONG Size)
{
 return realloc(Addr, Size);
}

VOID STDCALL PsaFree(IN OUT PVOID Context, IN PVOID Addr)
{
 free(Addr);
}

/* EmptyWorkingSet */
BOOL STDCALL EmptyWorkingSet(HANDLE hProcess)
{
 NTSTATUS nErrCode;
 QUOTA_LIMITS qlProcessQuota;
 
 /* query the working set */
 nErrCode = NtQueryInformationProcess
 (
  hProcess,
  ProcessQuotaLimits,
  &qlProcessQuota,
  sizeof(qlProcessQuota),
  NULL
 );

 /* failure */
 if(!NT_SUCCESS(nErrCode))
  goto fail;
 
 /* empty the working set */
 qlProcessQuota.MinimumWorkingSetSize = -1;
 qlProcessQuota.MaximumWorkingSetSize = -1;
 
 /* set the working set */
 nErrCode = NtSetInformationProcess
 (
  hProcess,
  ProcessQuotaLimits,
  &qlProcessQuota,
  sizeof(qlProcessQuota)
 );

 /* success */
 if(NT_SUCCESS(nErrCode))
  return (TRUE);
 
fail:
 /* failure */
 SetLastError(RtlNtStatusToDosError(nErrCode));
 return (FALSE);
}

/* EnumDeviceDrivers */
/* callback context */
typedef struct _ENUM_DEVICE_DRIVERS_CONTEXT
{
 LPVOID *lpImageBase;
 DWORD nCount;
} ENUM_DEVICE_DRIVERS_CONTEXT, *PENUM_DEVICE_DRIVERS_CONTEXT;

/* callback routine */
NTSTATUS STDCALL EnumDeviceDriversCallback
(
 IN ULONG ModuleCount,
 IN PSYSTEM_MODULE_ENTRY CurrentModule,
 IN OUT PVOID CallbackContext
)
{
 register PENUM_DEVICE_DRIVERS_CONTEXT peddcContext =
  (PENUM_DEVICE_DRIVERS_CONTEXT)CallbackContext;

 /* no more buffer space */
 if(peddcContext->nCount == 0)
  return STATUS_INFO_LENGTH_MISMATCH;

 /* return current module */
 *(peddcContext->lpImageBase) = CurrentModule->BaseAddress;
 
 /* go to next array slot */
 (peddcContext->lpImageBase) ++;
 (peddcContext->nCount) --;
 
 return STATUS_SUCCESS;
}

/* exported interface */
BOOL STDCALL EnumDeviceDrivers
(
 LPVOID *lpImageBase, 
 DWORD cb,            
 LPDWORD lpcbNeeded   
)
{
 register NTSTATUS nErrCode;
 ENUM_DEVICE_DRIVERS_CONTEXT eddcContext = {lpImageBase, cb / sizeof(PVOID)};

 cb /= sizeof(PVOID);

 /* do nothing if the buffer is empty */
 if(cb == 0 || lpImageBase == NULL)
 {
  *lpcbNeeded = 0;
  return (TRUE);
 }
 
 /* enumerate the system modules */
 nErrCode = PsaEnumerateSystemModules
 (
  &EnumDeviceDriversCallback,
  &eddcContext,
  NULL
 );
 
 /* return the count of bytes returned */
 *lpcbNeeded = (cb - eddcContext.nCount) * sizeof(PVOID);

 /* success */
 if(NT_SUCCESS(nErrCode) || nErrCode == STATUS_INFO_LENGTH_MISMATCH)
  return (TRUE);
 else
 {
  /* failure */
  SetLastError(RtlNtStatusToDosError(nErrCode));
  return (FALSE);
 }
}

/* EnumProcesses */
/* callback context */
typedef struct _ENUM_PROCESSES_CONTEXT
{
 DWORD *lpidProcess;
 DWORD nCount;
} ENUM_PROCESSES_CONTEXT, *PENUM_PROCESSES_CONTEXT;

/* callback routine */
NTSTATUS STDCALL EnumProcessesCallback
(
 IN PSYSTEM_PROCESS_INFORMATION CurrentProcess,
 IN OUT PVOID CallbackContext
)
{
 register PENUM_PROCESSES_CONTEXT pepcContext =
  (PENUM_PROCESSES_CONTEXT)CallbackContext;

 /* no more buffer space */
 if(pepcContext->nCount == 0)
  return STATUS_INFO_LENGTH_MISMATCH;

 /* return current process */
 *(pepcContext->lpidProcess) = CurrentProcess->ProcessId;
 
 /* go to next array slot */
 (pepcContext->lpidProcess) ++;
 (pepcContext->nCount) --;
 
 return STATUS_SUCCESS;
}

/* exported interface */
BOOL STDCALL EnumProcesses
(
 DWORD *lpidProcess,
 DWORD cb,
 LPDWORD lpcbNeeded
)
{
 register NTSTATUS nErrCode;
 ENUM_PROCESSES_CONTEXT epcContext = {lpidProcess, cb / sizeof(DWORD)};
 
 cb /= sizeof(DWORD);

 /* do nothing if the buffer is empty */
 if(cb == 0 || lpidProcess == NULL)
 {
  *lpcbNeeded = 0;
  return (TRUE);
 }
 
 /* enumerate the process ids */
 nErrCode = PsaEnumerateProcesses(&EnumProcessesCallback, &epcContext, NULL);
 
 *lpcbNeeded = (cb - epcContext.nCount) * sizeof(DWORD);

 /* success */
 if(NT_SUCCESS(nErrCode) || nErrCode == STATUS_INFO_LENGTH_MISMATCH)
  return (TRUE);
 else
 {
  /* failure */
  SetLastError(RtlNtStatusToDosError(nErrCode));
  return (FALSE);
 }
}

/* EnumProcessModules */
/* callback context */
typedef struct _ENUM_PROCESS_MODULES_CONTEXT
{
 HMODULE *lphModule;
 DWORD nCount;
} ENUM_PROCESS_MODULES_CONTEXT, *PENUM_PROCESS_MODULES_CONTEXT;

/* callback routine */
NTSTATUS STDCALL EnumProcessModulesCallback
(
 IN HANDLE ProcessHandle,
 IN PLDR_MODULE CurrentModule,
 IN OUT PVOID CallbackContext
)
{
 register PENUM_PROCESS_MODULES_CONTEXT pepmcContext = 
  (PENUM_PROCESS_MODULES_CONTEXT)CallbackContext;

 /* no more buffer space */
 if(pepmcContext->nCount == 0)
  return STATUS_INFO_LENGTH_MISMATCH;

 /* return current process */
 *(pepmcContext->lphModule) = CurrentModule->BaseAddress;
 
 /* go to next array slot */
 (pepmcContext->lphModule) ++;
 (pepmcContext->nCount) --;
 
 return STATUS_SUCCESS;
}

/* exported interface */
BOOL STDCALL EnumProcessModules(
  HANDLE hProcess,      // handle to process
  HMODULE *lphModule,   // array of module handles
  DWORD cb,             // size of array
  LPDWORD lpcbNeeded    // number of bytes required
)
{
 register NTSTATUS nErrCode;
 ENUM_PROCESS_MODULES_CONTEXT epmcContext = {lphModule, cb / sizeof(HMODULE)};
 
 cb /= sizeof(DWORD);

 /* do nothing if the buffer is empty */
 if(cb == 0 || lphModule == NULL)
 {
  *lpcbNeeded = 0;
  return (TRUE);
 }
 
 /* enumerate the process modules */
 nErrCode = PsaEnumerateProcessModules
 (
  hProcess,
  &EnumProcessModulesCallback,
  &epmcContext
 );
 
 *lpcbNeeded = (cb - epmcContext.nCount) * sizeof(DWORD);

 /* success */
 if(NT_SUCCESS(nErrCode) || nErrCode == STATUS_INFO_LENGTH_MISMATCH)
  return (TRUE);
 else
 {
  /* failure */
  SetLastError(RtlNtStatusToDosError(nErrCode));
  return (FALSE);
 }
}

/* EOF */

