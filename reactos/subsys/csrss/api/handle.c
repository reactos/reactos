/* $Id: handle.c,v 1.6 2000/04/23 17:44:53 phreak Exp $
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
#include <ntdll/rtl.h>

/* FUNCTIONS *****************************************************************/

NTSTATUS CsrGetObject( PCSRSS_PROCESS_DATA ProcessData, HANDLE Handle, Object_t **Object )
{
  //   DbgPrint( "CsrGetObject, Object: %x, %x, %x\n", Object, Handle, ProcessData->HandleTableSize );
   if( (((ULONG)Handle) >> 2) - 1 > ProcessData->HandleTableSize )
     {
       DbgPrint( "CsrGetObject returning invalid handle\n" );
       return STATUS_INVALID_HANDLE;
     }
   *Object = ProcessData->HandleTable[(((ULONG)Handle) >> 2) - 1];
   RtlEnterCriticalSection( &(*Object)->Lock );
   //   DbgPrint( "CsrGetObject returning\n" );
   return *Object ? STATUS_SUCCESS : STATUS_INVALID_HANDLE;
}

VOID CsrUnlockObject( Object_t *Object )
{
   RtlLeaveCriticalSection( &Object->Lock );
}

NTSTATUS CsrReleaseObject(PCSRSS_PROCESS_DATA ProcessData,
			  HANDLE Handle)
{
   Object_t *Object;
   //   DbgPrint( "CsrReleaseObject\n" );
   if( (((ULONG)Handle) >> 2) - 1 > ProcessData->HandleTableSize || ProcessData->HandleTable[(((ULONG)Handle) >> 2) - 1] == 0 )
      return STATUS_INVALID_HANDLE;
   /* dec ref count */
   Object = ProcessData->HandleTable[(((ULONG)Handle) >> 2) - 1];
   RtlEnterCriticalSection( &Object->Lock );
   if( --Object->ReferenceCount == 0 )
      switch( Object->Type )
	 {
	 case CSRSS_CONSOLE_MAGIC: CsrDeleteConsole( ProcessData, (PCSRSS_CONSOLE) Object );
	   DbgPrint( "Deleting Console\n" );
	    break;
	 default: DbgPrint( "CSR: Error: releaseing unknown object type" );
	 }
   DbgPrint( "Deleting object, refcount: %d\n", Object->ReferenceCount );
   ProcessData->HandleTable[(((ULONG)Handle) >> 2) - 1] = 0;
   RtlLeaveCriticalSection( &Object->Lock );
   return STATUS_SUCCESS;
}

NTSTATUS CsrInsertObject( PCSRSS_PROCESS_DATA ProcessData, PHANDLE Handle, Object_t *Object )
{
   ULONG i;
   PVOID* NewBlock;

   //   DbgPrint( "CsrInsertObject\n" );
   for (i = 0; i < ProcessData->HandleTableSize; i++)
     {
	if (ProcessData->HandleTable[i] == NULL)
	  {
	     ProcessData->HandleTable[i] = Object;
	     *Handle = (HANDLE)(((i + 1) << 2) | 0x3);
	     Object->ReferenceCount++;
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
   RtlFreeHeap( CsrssApiHeap, 0, ProcessData->HandleTable );
   ProcessData->HandleTable = (Object_t **)NewBlock;
   ProcessData->HandleTable[i] = Object;   
   *Handle = (HANDLE)(((i + 1) << 2) | 0x3);
   Object->ReferenceCount++;
   ProcessData->HandleTableSize = ProcessData->HandleTableSize + 64;
   return(STATUS_SUCCESS);
}



