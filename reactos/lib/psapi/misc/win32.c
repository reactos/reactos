/* $Id: win32.c,v 1.1 2002/06/18 22:14:07 hyperion Exp $
 */
/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        reactos/lib/psapi/misc/win32.c
 * PURPOSE:     Win32 stubs for PSAPI
 * PROGRAMMER:  KJK::Hyperion <noog@libero.it>
 * UPDATE HISTORY:
 *              10/06/2002: Created
 */

#include <windows.h>
#include <internal/psapi.h>
#include <psapi.h>

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

BOOL STDCALL EnumDeviceDrivers
(
 LPVOID *lpImageBase, 
 DWORD cb,            
 LPDWORD lpcbNeeded   
)
{
 NTSTATUS nErrCode;
 
 /* do nothing if the buffer is empty */
 if(cb < sizeof(DWORD) || lpImageBase == NULL)
 {
  *lpcbNeeded = 0;
  return (TRUE);
 }
 
 /* enumerate the system modules */
 nErrCode = PsaEnumerateSystemModules(lpImageBase, cb, lpcbNeeded);
 
 /* success */
 if(NT_SUCCESS(nErrCode))
  return (TRUE);
 
 /* failure or partial success */
 if(nErrCode == STATUS_INFO_LENGTH_MISMATCH)
 {
  /* insufficient buffer: ignore this error */
  *lpcbNeeded = cb;
  return (TRUE);
 }
 else
 {
  /* failure */
  SetLastError(RtlNtStatusToDosError(nErrCode));
  return (FALSE);
 }
}

BOOL STDCALL EnumProcesses
(
 DWORD *lpidProcess,
 DWORD cb,
 LPDWORD lpcbNeeded
)
{
 NTSTATUS nErrCode;
 
 /* do nothing if the buffer is empty */
 if(cb < sizeof(DWORD) || lpidProcess == NULL)
 {
  *lpcbNeeded = 0;
  return (TRUE);
 }
 
 /* enumerate the process ids */
 nErrCode = PsaEnumerateProcessIds(lpidProcess, cb, lpcbNeeded);
 
 /* success */
 if(NT_SUCCESS(nErrCode))
  return (TRUE);
 
 /* failure or partial success */
 if(nErrCode == STATUS_INFO_LENGTH_MISMATCH)
 {
  /* insufficient buffer: ignore this error */
  *lpcbNeeded = cb;
  return (TRUE);
 }
 else
 {
  /* failure */
  SetLastError(RtlNtStatusToDosError(nErrCode));
  return (FALSE);
 }
}

/* EOF */

