/* $Id: drivers.c,v 1.3 2004/11/03 22:43:00 weiden Exp $
*/
/*
 * COPYRIGHT:   See COPYING in the top level directory
 * LICENSE:     See LGPL.txt in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        reactos/lib/epsapi/enum/drivers.c
 * PURPOSE:     Enumerate system modules
 * PROGRAMMER:  KJK::Hyperion <noog@libero.it>
 * UPDATE HISTORY:
 *              02/04/2003: Created
 *              12/04/2003: internal PSAPI renamed EPSAPI (Extended PSAPI) and
 *                          isolated in its own library to clear the confusion
 *                          and improve reusability
 */

#include "precomp.h"

#define NDEBUG
#include <debug.h>

NTSTATUS NTAPI
PsaEnumerateSystemModules(IN PSYSMOD_ENUM_ROUTINE Callback,
                          IN OUT PVOID CallbackContext)
{
  PSYSTEM_MODULE_INFORMATION psmModules;
  NTSTATUS Status = STATUS_SUCCESS;

#if 0
  __try
  {
#else
  do
  {
#endif
  /* capture the system modules */
  Status = PsaCaptureSystemModules(&psmModules);
 
  if(!NT_SUCCESS(Status))
  {
    break;
  }

  /* walk the system modules */
  Status = PsaWalkSystemModules(psmModules, Callback, CallbackContext);
#if 0
  }
  __finally
  {
#else
  } while(0);
#endif
  /* free the capture */
  PsaFreeCapture(psmModules);
#if 0
  }
#endif
 
  return Status;
}

NTSTATUS NTAPI
PsaCaptureSystemModules(OUT PSYSTEM_MODULE_INFORMATION *SystemModules)
{
  SIZE_T nSize = 0;
  PSYSTEM_MODULE_INFORMATION psmModules;
  NTSTATUS Status;

#if 0
  __try
  {
#else
  do
  {
#endif
  /* initial probe. We just get the count of system modules */
  Status = NtQuerySystemInformation(SystemModuleInformation,
                                    &nSize,
                                    sizeof(nSize),
                                    NULL);

  if(!NT_SUCCESS(Status) && (Status != STATUS_INFO_LENGTH_MISMATCH))
  {
    DPRINT(FAILED_WITH_STATUS, "NtQuerySystemInformation", Status);
    break;
  }

  /* RATIONALE: the loading of a system module is a rare occurrence. To
     minimize memory operations that could be expensive, or fragment the
     pool/heap, we try to determine the buffer size in advance, knowing that
     the number of elements is unlikely to change */
  nSize = sizeof(SYSTEM_MODULE_INFORMATION) +
          ((psmModules->Count - 1) * sizeof(SYSTEM_MODULE_INFORMATION));

  psmModules = NULL;

  do
  {
    PVOID pTmp;
  
    /* free the buffer, and reallocate it to the new size. RATIONALE: since we
       ignore the buffer's content at this point, there's no point in a realloc,
       that could end up copying a large chunk of data we'd discard anyway */
    PsaiFree(psmModules);
    pTmp = PsaiMalloc(nSize);

    if(pTmp == NULL)
    {
      Status = STATUS_NO_MEMORY;
      DPRINT(FAILED_WITH_STATUS, "PsaiMalloc", Status);
      break;
    }

    psmModules = pTmp;

    /* query the information */
    Status = NtQuerySystemInformation(SystemModuleInformation,
                                      psmModules,
                                      nSize,
                                      NULL);

    /* double the buffer for the next loop */
    nSize *= 2;
  } while(Status == STATUS_INFO_LENGTH_MISMATCH);

  if(!NT_SUCCESS(Status))
  {
    DPRINT(FAILED_WITH_STATUS, "NtQuerySystemInformation", Status);
    break;
  }

  *SystemModules = psmModules;

  Status = STATUS_SUCCESS;
#if 0
  }
  __finally
  {
#else
  } while(0);
#endif
  /* in case of failure, free the buffer */
  if(!NT_SUCCESS(Status))
  {
    PsaiFree(psmModules);
  }
#if 0
  }
#endif

  return Status;
}

NTSTATUS NTAPI
PsaWalkSystemModules(IN PSYSTEM_MODULE_INFORMATION SystemModules,
                     IN PSYSMOD_ENUM_ROUTINE Callback,
                     IN OUT PVOID CallbackContext)
{
  ULONG i;
  NTSTATUS Status;

  /* repeat until all modules have been returned */
  for(i = 0; i < SystemModules->Count; i++)
  {
    /* return current module to the callback */
    Status = Callback(&(SystemModules->Module[i]), CallbackContext);
  
    if(!NT_SUCCESS(Status))
    {
      return Status;
    }
  }

  return STATUS_SUCCESS;
}

PSYSTEM_MODULE_INFORMATION_ENTRY FASTCALL
PsaWalkFirstSystemModule(IN PSYSTEM_MODULE_INFORMATION SystemModules)
{ 
  return &(SystemModules->Module[0]);
}

PSYSTEM_MODULE_INFORMATION_ENTRY FASTCALL
PsaWalkNextSystemModule(IN PSYSTEM_MODULE_INFORMATION CurrentSystemModule)
{
  return (PSYSTEM_MODULE_INFORMATION_ENTRY)((ULONG_PTR)CurrentSystemModule +
                                            (offsetof(SYSTEM_MODULE_INFORMATION, Module[1]) -
                                             offsetof(SYSTEM_MODULE_INFORMATION, Module[0])));
}

/* EOF */
