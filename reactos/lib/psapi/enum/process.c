/* $Id: process.c,v 1.3 2002/08/31 15:36:55 hyperion Exp $
*/
/*
 * COPYRIGHT:   See COPYING in the top level directory
 * LICENSE:     See LGPL.txt in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        reactos/lib/psapi/enum/process.c
 * PURPOSE:     Enumerate processes
 * PROGRAMMER:  KJK::Hyperion <noog@libero.it>
 * UPDATE HISTORY:
 *              10/06/2002: Created
 *              29/08/2002: Generalized the interface to improve reusability,
 *                          more efficient use of memory operations
 */

#include <stdlib.h>
#include <ddk/ntddk.h>
#include <debug.h>
#include <internal/psapi.h>

NTSTATUS
STDCALL
PsaEnumerateProcesses
(
 IN PPROC_ENUM_ROUTINE Callback,
 IN OUT PVOID CallbackContext
)
{
 register NTSTATUS nErrCode = STATUS_SUCCESS;
 PSYSTEM_PROCESS_INFORMATION pInfoBuffer = NULL;
 PSYSTEM_PROCESS_INFORMATION pInfoHead = NULL;
 ULONG nSize = 32768;

 /* FIXME: if the system has loaded several processes and threads, the buffer
    could get really big. But if there's several processes and threads, the
    system is already under stress, and a huge buffer could only make things
    worse. The function should be profiled to see what's the average minimum
    buffer size, to succeed on the first shot */
 do
 {
  void * pTmp;
  
  /* free the buffer, and reallocate it to the new size. RATIONALE: since we
     ignore the buffer's contents at this point, there's no point in a realloc()
     that could end up copying a large chunk of data we'd discard anyway */
  free(pInfoBuffer);
  pTmp = malloc(nSize);
  
  if(pTmp == NULL)
  {
   /* failure */
   DPRINT(FAILED_WITH_STATUS, "malloc", STATUS_NO_MEMORY);
   nErrCode = STATUS_NO_MEMORY;
   goto esp_Finalize;
  }
  
  pInfoBuffer = pTmp;
  
  /* query the information */
  nErrCode = NtQuerySystemInformation
  (
   SystemProcessesAndThreadsInformation,
   pInfoBuffer,
   nSize,
   NULL
  );

  /* double the buffer size */
  nSize += nSize;
 }
 /* repeat until the buffer is big enough */
 while(nErrCode == STATUS_INFO_LENGTH_MISMATCH);
 
 /* failure */
 if(!NT_SUCCESS(nErrCode))
 {
  DPRINT(FAILED_WITH_STATUS, "NtQuerySystemInformation", nErrCode);
  goto esp_Finalize;
 }
 
 /* list head */
 pInfoHead = pInfoBuffer;

 /* scan the list */
 while(1)
 {
  /* notify the callback */
  nErrCode = Callback(pInfoHead, CallbackContext);

  /* if the callback returned an error or this is the end of the process list,
     break out */
  if(!NT_SUCCESS(nErrCode) || pInfoHead->RelativeOffset == 0)
   break;
  
  /* move to the next process */
  pInfoHead = 
   (SYSTEM_PROCESS_INFORMATION*)
   ((ULONG)pInfoHead + pInfoHead->RelativeOffset);
 }

esp_Finalize:
 /* free the buffer */
 free(pInfoBuffer);
 
 /* return the last status */
 return (nErrCode);
}

/* EOF */

