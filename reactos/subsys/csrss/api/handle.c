/* $Id: handle.c,v 1.4 2000/03/24 22:25:39 dwelch Exp $
 *
 * reactos/subsys/csrss/api/handle.c
 *
 * Console I/O functions
 *
 * ReactOS Operating System
 */

/* INCLUDES ******************************************************************/

#include <ddk/ntddk.h>

#include <csrss/csrss.h>
#include "api.h"

/* FUNCTIONS *****************************************************************/

NTSTATUS CsrGetObject(PCSRSS_PROCESS_DATA ProcessData,
		      HANDLE Handle,
		      PVOID* Object)
{
   *Object = ProcessData->HandleTable[((ULONG)Handle) - 1];
   return(STATUS_SUCCESS);
}

NTSTATUS CsrReleaseObject(PCSRSS_PROCESS_DATA ProcessData,
			  PVOID Object)
{
}

NTSTATUS CsrInsertObject(PCSRSS_PROCESS_DATA ProcessData,
			 PHANDLE Handle,
			 PVOID Object)
{
   ULONG i;
   PVOID* NewBlock;
   
   for (i = 0; i < ProcessData->HandleTableSize; i++)
     {
	if (ProcessData->HandleTable[i] == NULL)
	  {
	     ProcessData->HandleTable[i] = Object;
	     *Handle = (HANDLE)((i << 8) | 0x3);
	     return(STATUS_SUCCESS);
	  }
     }
   NewBlock = RtlAllocateHeap(CsrssApiHeap,
			      HEAP_ZERO_MEMORY,
			      (ProcessData->HandleTableSize + 64) * 
			      sizeof(HANDLE));
   if (NewBlock == NULL)
     {
	return(STATUS_UNSUCCESSFUL);
     }
   RtlCopyMemory(NewBlock, 
		 ProcessData->HandleTable,
		 ProcessData->HandleTableSize * sizeof(HANDLE));
   ProcessData->HandleTable = NewBlock;
   ProcessData->HandleTable[i] = Object;   
   *Handle = (HANDLE)(((i + 1) << 8) | 0x3);
   ProcessData->HandleTableSize = ProcessData->HandleTableSize + 64;
   
   return(STATUS_SUCCESS);
}
