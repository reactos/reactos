/* $Id: module.c,v 1.3 2002/08/31 15:36:55 hyperion Exp $
*/
/*
 * COPYRIGHT:   See COPYING in the top level directory
 * LICENSE:     See LGPL.txt in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        reactos/lib/psapi/enum/module.c
 * PURPOSE:     Enumerate system and process modules
 * PROGRAMMER:  KJK::Hyperion <noog@libero.it>
 * UPDATE HISTORY:
 *              10/06/2002: Created
 *              29/08/2002: Generalized the interface to improve reusability,
 *                          more efficient use of memory operations
 */

#include <ddk/ntddk.h>
#include <debug.h>
#include <internal/psapi.h>
#include <ntdll/ldr.h>

NTSTATUS
STDCALL
PsaEnumerateSystemModules
(
 IN PSYSMOD_ENUM_ROUTINE Callback,
 IN OUT PVOID CallbackContext
)
{
 ULONG nSize;
 register NTSTATUS nErrCode = STATUS_SUCCESS;
 register PULONG pnModuleCount = &nSize;
 register PSYSTEM_MODULE_ENTRY psmeCurModule;
 register ULONG nModuleCount;

 /* initial probe */
 nErrCode = NtQuerySystemInformation
 (
  SystemModuleInformation,
  pnModuleCount,
  sizeof(nSize),
  NULL
 );
 
 if(nErrCode != STATUS_INFO_LENGTH_MISMATCH && !NT_SUCCESS(nErrCode))
 {
  /* failure */
  DPRINT(FAILED_WITH_STATUS, "NtQuerySystemInformation", nErrCode);
  return nErrCode;
 }

 /* RATIONALE: the loading of a system module is a rare occurrence. To minimize
    memory operations that could be expensive, or fragment the pool/heap, we try
    to determine the buffer size in advance, knowing that the number of elements
    is unlikely to change */
 nSize = sizeof(ULONG) + nSize * sizeof(SYSTEM_MODULE_ENTRY);
 pnModuleCount = NULL;
 
 do
 {
  register void * pTmp;

  /* free the buffer, and reallocate it to the new size. RATIONALE: since we
     ignore the buffer's content at this point, there's no point in a realloc(),
     that could end up copying a large chunk of data we'd discard anyway */
  free(pnModuleCount);
  pTmp = malloc(nSize);
  
  if(pTmp == NULL)
  {
   /* failure */
   nErrCode = STATUS_NO_MEMORY;
   goto esm_Finalize;
  }
  
  pnModuleCount = pTmp;
  
  /* query the information */
  nErrCode = NtQuerySystemInformation
  (
   SystemModuleInformation,
   pnModuleCount,
   nSize,
   NULL
  );

  /* double the buffer for the next loop */
  nSize += nSize;
 }
 /* repeat until the buffer is big enough */
 while(nErrCode == STATUS_INFO_LENGTH_MISMATCH);

 if(!NT_SUCCESS(nErrCode))
 {
  /* failure */
  DPRINT(FAILED_WITH_STATUS, "NtQuerySystemInformation", nErrCode);
  goto esm_Finalize;
 }

 /* the array of modules starts right after an ULONG storing their count */
 psmeCurModule = (PSYSTEM_MODULE_ENTRY)(pnModuleCount + 1);

 nModuleCount = *pnModuleCount;

 /* repeat until all modules have been returned */
 while(nModuleCount > 0)
 {
  /* return current module to the callback */
  nErrCode = Callback(nModuleCount, psmeCurModule, CallbackContext);
  
  if(!NT_SUCCESS(nErrCode))
   /* failure */
   goto esm_Finalize;
  
  /* next module */
  psmeCurModule ++;
  nModuleCount --;
 }

esm_Finalize:
 /* free the buffer */
 free(pnModuleCount);

 return (nErrCode);
}

NTSTATUS
STDCALL
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
