/* $Id$
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

#define WIN32_NO_STATUS
#include <windows.h>
#define NTOS_MODE_USER
#include <ndk/ntndk.h>

#define NDEBUG
#include <debug.h>

#include <epsapi/epsapi.h>

NTSTATUS NTAPI
PsaEnumerateSystemModules(IN PSYSMOD_ENUM_ROUTINE Callback,
                          IN OUT PVOID CallbackContext)
{
  PRTL_PROCESS_MODULES psmModules;
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
PsaCaptureSystemModules(OUT PRTL_PROCESS_MODULES *SystemModules)
{
  SIZE_T nSize = 0;
  PRTL_PROCESS_MODULES psmModules = NULL;
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
  nSize = sizeof(RTL_PROCESS_MODULES) +
          (nSize * sizeof(RTL_PROCESS_MODULES));

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
PsaWalkSystemModules(IN PRTL_PROCESS_MODULES SystemModules,
                     IN PSYSMOD_ENUM_ROUTINE Callback,
                     IN OUT PVOID CallbackContext)
{
  ULONG i;
  NTSTATUS Status;

  /* repeat until all modules have been returned */
  for(i = 0; i < SystemModules->NumberOfModules; i++)
  {
    /* return current module to the callback */
    Status = Callback(&(SystemModules->Modules[i]), CallbackContext);

    if(!NT_SUCCESS(Status))
    {
      return Status;
    }
  }

  return STATUS_SUCCESS;
}

PRTL_PROCESS_MODULE_INFORMATION FASTCALL
PsaWalkFirstSystemModule(IN PRTL_PROCESS_MODULES SystemModules)
{
  return &(SystemModules->Modules[0]);
}

PRTL_PROCESS_MODULE_INFORMATION FASTCALL
PsaWalkNextSystemModule(IN PRTL_PROCESS_MODULES CurrentSystemModule)
{
  return (PRTL_PROCESS_MODULE_INFORMATION)((ULONG_PTR)CurrentSystemModule +
                                            (FIELD_OFFSET(RTL_PROCESS_MODULES, Modules[1]) -
                                             FIELD_OFFSET(RTL_PROCESS_MODULES, Modules[0])));
}

/* EOF */
