/* $Id: process.c,v 1.1 2002/06/18 22:12:51 hyperion Exp $
*/
/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        reactos/lib/psapi/enum/process.c
 * PURPOSE:     Enumerate process ids
 * PROGRAMMER:  KJK::Hyperion <noog@libero.it>
 * UPDATE HISTORY:
 *              10/06/2002: Created
 */

#include <ddk/ntddk.h>
#include <internal/psapi.h>
#include <stdlib.h>

NTSTATUS
STDCALL
PsaEnumerateProcessIds
(
 OUT ULONG * ProcessIds,
 IN ULONG ProcessIdsLength,
 OUT ULONG * ReturnLength OPTIONAL
)
{
 NTSTATUS nErrCode = STATUS_SUCCESS;
 ULONG nSize = 32768;
 SYSTEM_PROCESS_INFORMATION * pInfoBuffer = NULL;
 SYSTEM_PROCESS_INFORMATION * pInfoHead = NULL;
 ULONG nBufSize;
 ULONG nRetLen = 0;

 /* ignore buffer size if buffer is null */
 if(ProcessIds == NULL)
  ProcessIdsLength = 0;
 /* ignore buffer if buffer size is zero */
 else if(ProcessIdsLength == 0)
  ProcessIds = NULL;
 
 do
 {
  void * pTmp;
  
  /* resize and/or move the buffer */
  pTmp = realloc(pInfoBuffer, nSize);
  
  if(pTmp == NULL)
  {
   /* failure */
   nErrCode = STATUS_NO_MEMORY;
   goto end;
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
  goto end;
 
 /* size of ProcessIds in elements */
 nBufSize = ProcessIdsLength / sizeof(*ProcessIds);
 /* list head */
 pInfoHead = pInfoBuffer;

 /* repeat until the buffer is empty */
 while(nBufSize > 0)
 {
  /* return the current process id */
  (*ProcessIds) = pInfoHead->ProcessId;
  
  /* move to the next buffer entry */
  ProcessIds ++;
  nBufSize --;
  nRetLen ++;
  
  /* end of process list */
  if(pInfoHead->RelativeOffset == 0)
   break;
  
  /* move to the next process */
  pInfoHead = 
   (SYSTEM_PROCESS_INFORMATION*)
   ((ULONG)pInfoHead + pInfoHead->RelativeOffset);
 }

 if(pInfoHead->RelativeOffset == 0)
  /* all process ids were returned */
  nErrCode = STATUS_SUCCESS;
 else
 {
  /* insufficient buffer */
  nErrCode = STATUS_INFO_LENGTH_MISMATCH;
  
  /* caller doesn't need buffer size */
  if(ReturnLength == NULL)
   goto end;
  
  /* repeat while there are still processes */
  while(pInfoHead->RelativeOffset != 0)
  {
   pInfoHead =
    (SYSTEM_PROCESS_INFORMATION*)
    ((ULONG)pInfoHead + pInfoHead->RelativeOffset);

   nRetLen++;
  }
  
 }

 /* used buffer size */
 if(ReturnLength)
  (*ReturnLength) = nRetLen * sizeof(DWORD);
 
end:
 /* free the buffer */
 free(pInfoBuffer);
 
 /* return the last status */
 return (nErrCode);
}

/* EOF */

