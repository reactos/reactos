/* $Id$
*/
/*
 * COPYRIGHT:   See COPYING in the top level directory
 * LICENSE:     See LGPL.txt in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        reactos/lib/epsapi/enum/module.c
 * PURPOSE:     Enumerate process modules
 * PROGRAMMER:  KJK::Hyperion <noog@libero.it>
 * UPDATE HISTORY:
 *              10/06/2002: Created
 *              29/08/2002: Generalized the interface to improve reusability,
 *                          more efficient use of memory operations
 *              12/02/2003: malloc and free renamed to PsaiMalloc and PsaiFree,
 *                          for better reusability
 *              02/04/2003: System modules enumeration moved into its own file
 *              12/04/2003: internal PSAPI renamed EPSAPI (Extended PSAPI) and
 *                          isolated in its own library to clear the confusion
 *                          and improve reusability
 */
#define WIN32_NO_STATUS
#include <windows.h>
#define NTOS_MODE_USER
#include <ndk/ntndk.h>

#define NDEBUG
#include <debug.h>

#include <epsapi/epsapi.h>

NTSTATUS NTAPI
PsaEnumerateProcessModules(IN HANDLE ProcessHandle,
                           IN PPROCMOD_ENUM_ROUTINE Callback,
                           IN OUT PVOID CallbackContext)
{
  NTSTATUS Status;

  /* current process - use direct memory copy */
  /* FIXME - compare process id instead of a handle */
  if(ProcessHandle == NtCurrentProcess())
  {
    PLIST_ENTRY ListHead, Current;

#if 0
    __try
    {
#endif
      ListHead = &(NtCurrentPeb()->Ldr->InLoadOrderModuleList);
      Current = ListHead->Flink;
 
      while(Current != ListHead)
      {
        PLDR_DATA_TABLE_ENTRY LoaderModule = CONTAINING_RECORD(Current, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);
   
        /* return the current module to the callback */
        Status = Callback(ProcessHandle, LoaderModule, CallbackContext);
    
        if(!NT_SUCCESS(Status))
        {
          goto Failure;
        }
    
        Current = LoaderModule->InLoadOrderLinks.Flink;
      }
#if 0
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
      return GetExceptionCode();
    }
#endif
  }
  else
  {
    PROCESS_BASIC_INFORMATION BasicInformation;
    PPEB_LDR_DATA LoaderData;
    LDR_DATA_TABLE_ENTRY LoaderModule;
    PLIST_ENTRY ListHead, Current;
 
    /* query the process basic information (includes the PEB address) */
    Status = NtQueryInformationProcess(ProcessHandle,
                                       ProcessBasicInformation,
                                       &BasicInformation,
                                       sizeof(BasicInformation),
                                       NULL);
 
    if(!NT_SUCCESS(Status))
    {
      DPRINT(FAILED_WITH_STATUS, "NtQueryInformationProcess", Status);
      goto Failure;
    }
 
    /* get the address of the PE Loader data */
    Status = NtReadVirtualMemory(ProcessHandle,
                                 &(BasicInformation.PebBaseAddress->Ldr),
                                 &LoaderData,
                                 sizeof(LoaderData),
                                 NULL);
 
    if(!NT_SUCCESS(Status))
    {
      DPRINT(FAILED_WITH_STATUS, "NtReadVirtualMemory", Status);
      goto Failure;
    }
 
    /* head of the module list: the last element in the list will point to this */
    ListHead = &LoaderData->InLoadOrderModuleList;
 
    /* get the address of the first element in the list */
    Status = NtReadVirtualMemory(ProcessHandle,
                                 &(LoaderData->InLoadOrderModuleList.Flink),
                                 &Current,
                                 sizeof(Current),
                                 NULL);
 
    while(Current != ListHead)
    {
      /* read the current module */
      Status = NtReadVirtualMemory(ProcessHandle,
                                   CONTAINING_RECORD(Current, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks),
                                   &LoaderModule,
                                   sizeof(LoaderModule),
                                   NULL);
 
      if(!NT_SUCCESS(Status))
      {
        DPRINT(FAILED_WITH_STATUS, "NtReadVirtualMemory", Status);
        goto Failure;
      }

      /* return the current module to the callback */
      Status = Callback(ProcessHandle, &LoaderModule, CallbackContext);
   
      if(!NT_SUCCESS(Status))
      {
        goto Failure;
      }
    
      /* address of the next module in the list */
      Current = LoaderModule.InLoadOrderLinks.Flink;
    }
  }

  return STATUS_SUCCESS;

Failure:
  return Status;
}

/* EOF */
