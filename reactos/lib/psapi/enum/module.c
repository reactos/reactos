/* $Id: module.c,v 1.5 2003/04/03 00:06:23 hyperion Exp $
*/
/*
 * COPYRIGHT:   See COPYING in the top level directory
 * LICENSE:     See LGPL.txt in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        reactos/lib/psapi/enum/module.c
 * PURPOSE:     Enumerate process modules
 * PROGRAMMER:  KJK::Hyperion <noog@libero.it>
 * UPDATE HISTORY:
 *              10/06/2002: Created
 *              29/08/2002: Generalized the interface to improve reusability,
 *                          more efficient use of memory operations
 *              12/02/2003: malloc and free renamed to PsaiMalloc and PsaiFree,
 *                          for better reusability
 *              02/04/2003: System modules enumeration moved into its own file
 */

#include <ddk/ntddk.h>
#include <debug.h>
#include <internal/psapi.h>
#include <ntdll/ldr.h>

NTSTATUS
NTAPI
PsaEnumerateProcessModules
(
 IN HANDLE ProcessHandle,
 IN PPROCMOD_ENUM_ROUTINE Callback,
 IN OUT PVOID CallbackContext
)
{
 register NTSTATUS nErrCode;

 /* current process - use direct memory copy */
 if(ProcessHandle == NtCurrentProcess())
 {
  register PLIST_ENTRY pleListHead;
  register PLIST_ENTRY pleCurEntry;

#if 0
  /* FIXME: activate this when GCC supports SEH */
  __try
  {
#endif
   pleListHead = &(NtCurrentPeb()->Ldr->InLoadOrderModuleList);
   pleCurEntry = pleListHead->Flink;
 
   while(pleCurEntry != pleListHead)
   {
    register PLDR_MODULE plmModule = CONTAINING_RECORD
    (
     pleCurEntry,
     LDR_MODULE,
     InLoadOrderModuleList
    );
   
    /* return the current module to the callback */
    nErrCode = Callback(ProcessHandle, plmModule, CallbackContext);
    
    if(!NT_SUCCESS(nErrCode))
     /* failure */
     goto epm_Failure;
    
    pleCurEntry = plmModule->InLoadOrderModuleList.Flink;
   }
#if 0
  /* FIXME: activate this when GCC supports SEH */
  }
  __except(EXCEPTION_EXECUTE_HANDLER)
  {
   return GetExceptionCode();
  }
#endif
 }
 /* another process */
 else
 {
  PROCESS_BASIC_INFORMATION pbiInfo;
  PPEB_LDR_DATA ppldLdrData;
  LDR_MODULE lmModule;
  PLIST_ENTRY pleListHead;
  PLIST_ENTRY pleCurEntry;
 
  /* query the process basic information (includes the PEB address) */
  nErrCode = NtQueryInformationProcess
  (
   ProcessHandle,
   ProcessBasicInformation,
   &pbiInfo,
   sizeof(pbiInfo),
   NULL
  );
 
  if(!NT_SUCCESS(nErrCode))
  {
   /* failure */
   DPRINT(FAILED_WITH_STATUS, "NtQueryInformationProcess", nErrCode);
   goto epm_Failure;
  }
 
  /* get the address of the PE Loader data */
  nErrCode = NtReadVirtualMemory
  (
   ProcessHandle,
   &(pbiInfo.PebBaseAddress->Ldr),
   &ppldLdrData,
   sizeof(ppldLdrData),
   NULL
  );
 
  if(!NT_SUCCESS(nErrCode))
  {
   /* failure */
   DPRINT(FAILED_WITH_STATUS, "NtReadVirtualMemory", nErrCode);
   goto epm_Failure;
  }
 
  /* head of the module list: the last element in the list will point to this */
  pleListHead = &ppldLdrData->InLoadOrderModuleList;
 
  /* get the address of the first element in the list */
  nErrCode = NtReadVirtualMemory
  (
   ProcessHandle,
   &(ppldLdrData->InLoadOrderModuleList.Flink),
   &pleCurEntry,
   sizeof(pleCurEntry),
   NULL
  );
 
  while(pleCurEntry != pleListHead)
  {
   /* read the current module */
   nErrCode = NtReadVirtualMemory
   (
    ProcessHandle,
    CONTAINING_RECORD(pleCurEntry, LDR_MODULE, InLoadOrderModuleList),
    &lmModule,
    sizeof(lmModule),
    NULL
   );
 
   if(!NT_SUCCESS(nErrCode))
   {
    /* failure */
    DPRINT(FAILED_WITH_STATUS, "NtReadVirtualMemory", nErrCode);
    goto epm_Failure;
   }

   /* return the current module to the callback */
   nErrCode = Callback(ProcessHandle, &lmModule, CallbackContext);
   
   if(!NT_SUCCESS(nErrCode))
    /* failure */
    goto epm_Failure;
    
   /* address of the next module in the list */
   pleCurEntry = lmModule.InLoadOrderModuleList.Flink;
  }
 
 }

 /* success */
 return (STATUS_SUCCESS);

epm_Failure:
 /* failure */
 return (nErrCode);
}

/* EOF */
