/* $Id: module.c,v 1.1 2002/06/18 22:12:51 hyperion Exp $
*/
/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        reactos/lib/psapi/enum/module.c
 * PURPOSE:     Enumerate system modules
 * PROGRAMMER:  KJK::Hyperion <noog@libero.it>
 * UPDATE HISTORY:
 *              10/06/2002: Created
 */

#include <ddk/ntddk.h>
#include <internal/psapi.h>
#include <stdlib.h>

NTSTATUS
STDCALL
PsaEnumerateSystemModules
(
 OUT PVOID * Modules,
 IN ULONG ModulesLength,
 OUT ULONG * ReturnLength OPTIONAL
)
{
 NTSTATUS nErrCode = STATUS_SUCCESS;
 ULONG nSize = sizeof(ULONG);
 PULONG pnModuleCount = NULL;
 PSYSTEM_MODULE_ENTRY psmeCurModule;
 ULONG nBufSize;

 /* ignore buffer size if buffer is null */
 if(Modules == NULL)
  ModulesLength = 0;
 /* ignore buffer if buffer size is zero */
 else if(ModulesLength == 0)
  Modules = NULL;
 
 do
 {
  void * pTmp;
  
  /* resize and/or move the buffer */
  pTmp = realloc(pnModuleCount, nSize);
  
  if(pTmp == NULL)
  {
   /* failure */
   nErrCode = STATUS_NO_MEMORY;
   goto end;
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

  /* while this is less efficient than doubling the buffer size, it should be
     executed only once in most cases */
  nSize += sizeof(SYSTEM_MODULE_ENTRY) * (*pnModuleCount);
 }
 /* repeat until the buffer is big enough */
 while(nErrCode == STATUS_INFO_LENGTH_MISMATCH);

 /* the array of modules starts right after an ULONG storing their count */
 psmeCurModule = (PSYSTEM_MODULE_ENTRY)(pnModuleCount + 1);
 
 /* element count */
 nBufSize = ModulesLength / sizeof(*Modules);
 
 /* not enough elements in the buffer */
 if((*pnModuleCount) > nBufSize)
  nErrCode = STATUS_INFO_LENGTH_MISMATCH;
 /* too many elements in the buffer */
 else
  nBufSize = *pnModuleCount;
 
 /* return the needed buffer size */
 if(ReturnLength)
  (*ReturnLength) = (*pnModuleCount) * sizeof(*Modules);
 
 /* repeat until the buffer is empty or all modules have been returned */
 while(nBufSize > 0)
 {
  /* return current module base */
  (*Modules) = psmeCurModule->BaseAddress;
  
  /* next buffer element */
  Modules ++;
  nBufSize --;
  
  /* next module */
  psmeCurModule ++;
 }
 
end:
 free(pnModuleCount);
 return (nErrCode);
}

/* EOF */

