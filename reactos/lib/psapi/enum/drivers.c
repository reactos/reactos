/* $Id: drivers.c,v 1.1 2003/04/03 00:06:23 hyperion Exp $
*/
/*
 * COPYRIGHT:   See COPYING in the top level directory
 * LICENSE:     See LGPL.txt in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        reactos/lib/psapi/enum/drivers.c
 * PURPOSE:     Enumerate system modules
 * PROGRAMMER:  KJK::Hyperion <noog@libero.it>
 * UPDATE HISTORY:
 *              02/04/2003: Created
 */

#include <ddk/ntddk.h>
#include <debug.h>
#include <stddef.h>

#include "internal/psapi.h"

NTSTATUS
NTAPI
PsaEnumerateSystemModules
(
 IN PSYSMOD_ENUM_ROUTINE Callback,
 IN OUT PVOID CallbackContext
)
{
 register NTSTATUS nErrCode = STATUS_SUCCESS;
 PSYSTEM_MODULES psmModules = NULL;

#if 0
 __try
 {
#endif
  do
  {
   /* capture the system modules */
   nErrCode = PsaCaptureSystemModules(&psmModules);
   
   if(!NT_SUCCESS(nErrCode))
    /* failure */
    break;
 
   /* walk the system modules */
   nErrCode = PsaWalkSystemModules(psmModules, Callback, CallbackContext);
  }
  while(0);
#if 0
 }
 __finally
 {
#endif
  /* free the capture */
  PsaFreeCapture(psmModules);
#if 0
 }
#endif
 
 /* return the last status */
 return nErrCode;
}

NTSTATUS
NTAPI
PsaCaptureSystemModules
(
 OUT PSYSTEM_MODULES * SystemModules
)
{
 SIZE_T nSize = 0;
 register NTSTATUS nErrCode;
 register PSYSTEM_MODULES psmModules = (PSYSTEM_MODULES)&nSize;

#if 0
 __try
 {
#endif
  do
  {
   /* initial probe. We just get the count of system modules */
   nErrCode = NtQuerySystemInformation
   (
    SystemModuleInformation,
    psmModules,
    sizeof(nSize),
    NULL
   );

   if(nErrCode != STATUS_INFO_LENGTH_MISMATCH && !NT_SUCCESS(nErrCode))
   {
    /* failure */
    DPRINT(FAILED_WITH_STATUS, "NtQuerySystemInformation", nErrCode);
    break;
   }

   /* RATIONALE: the loading of a system module is a rare occurrence. To
      minimize memory operations that could be expensive, or fragment the
      pool/heap, we try to determine the buffer size in advance, knowing that
      the number of elements is unlikely to change */
   nSize =
    sizeof(*psmModules) +
    (psmModules->Count - 1) * sizeof(SYSTEM_MODULE_INFORMATION);

   psmModules = NULL;

   do
   {
    register void * pTmp;
  
    /* free the buffer, and reallocate it to the new size. RATIONALE: since we
       ignore the buffer's content at this point, there's no point in a realloc,
       that could end up copying a large chunk of data we'd discard anyway */
    PsaiFree(psmModules);
    pTmp = PsaiMalloc(nSize);
    
    if(pTmp == NULL)
    {
     /* failure */
     nErrCode = STATUS_NO_MEMORY;
     DPRINT(FAILED_WITH_STATUS, "PsaiMalloc", nErrCode);
     break;
    }

    psmModules = pTmp;

    /* query the information */
    nErrCode = NtQuerySystemInformation
    (
     SystemModuleInformation,
     psmModules,
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
    break;
   }

   /* success */
   *SystemModules = psmModules;

   nErrCode = STATUS_SUCCESS;
  }
  while(0);
#if 0
 }
 __finally
 {
#endif
  /* in case of failure, free the buffer */
  if(!NT_SUCCESS(nErrCode))
   PsaiFree(psmModules);
#if 0
 }
#endif

 /* return the last status */
 return (nErrCode);
}

NTSTATUS
NTAPI
PsaWalkSystemModules
(
 IN PSYSTEM_MODULES SystemModules,
 IN PSYSMOD_ENUM_ROUTINE Callback,
 IN OUT PVOID CallbackContext
)
{
 register NTSTATUS nErrCode;
 register SIZE_T i;

 /* repeat until all modules have been returned */
 for(i = 0; i < SystemModules->Count; ++ i)
 {
  /* return current module to the callback */
  nErrCode = Callback(&(SystemModules->Modules[i]), CallbackContext);
  
  if(!NT_SUCCESS(nErrCode))
   /* failure */
   return nErrCode;
 }

 /* success */
 return STATUS_SUCCESS;
}

PSYSTEM_MODULE_INFORMATION
FASTCALL
PsaWalkFirstSystemModule
(
 IN PSYSTEM_MODULES SystemModules
)
{ 
 return &(SystemModules->Modules[0]);
}

PSYSTEM_MODULE_INFORMATION
FASTCALL
PsaWalkNextSystemModule
(
 IN PSYSTEM_MODULE_INFORMATION CurrentSystemModule
)
{
 return (PSYSTEM_MODULE_INFORMATION)
 (
  (ULONG_PTR)CurrentSystemModule +
  (
   offsetof(SYSTEM_MODULES, Modules[1]) -
   offsetof(SYSTEM_MODULES, Modules[0])
  )
 );
}

/* EOF */
