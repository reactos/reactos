/* $Id: handle.c,v 1.11 2002/09/07 15:13:08 chorns Exp $
 *
 * reactos/subsys/csrss/api/handle.c
 *
 * Console I/O functions
 *
 * ReactOS Operating System
 */

/* INCLUDES ******************************************************************/

#include <windows.h>
#define NTOS_USER_MODE
#include <ntos.h>

#include <csrss/csrss.h>
#include "api.h"

/* FUNCTIONS *****************************************************************/

NTSTATUS STDCALL CsrGetObject( PCSRSS_PROCESS_DATA ProcessData, HANDLE Handle, Object_t **Object )
{
   ULONG h = (((ULONG)Handle) >> 2) - 1;
  //   DbgPrint( "CsrGetObject, Object: %x, %x, %x\n", Object, Handle, ProcessData->HandleTableSize );
   if( h >= ProcessData->HandleTableSize )
     {
       DbgPrint( "CsrGetObject returning invalid handle\n" );
       return STATUS_INVALID_HANDLE;
     }
   *Object = ProcessData->HandleTable[h];
   //   DbgPrint( "CsrGetObject returning\n" );
   return *Object ? STATUS_SUCCESS : STATUS_INVALID_HANDLE;
}


NTSTATUS STDCALL CsrReleaseObject(PCSRSS_PROCESS_DATA ProcessData,
			  HANDLE Handle)
{
   Object_t *Object;
   ULONG h = (((ULONG)Handle) >> 2) - 1;
   if( h >= ProcessData->HandleTableSize || ProcessData->HandleTable[h] == 0 )
      return STATUS_INVALID_HANDLE;
   /* dec ref count */
   Object = ProcessData->HandleTable[h];
   if( InterlockedDecrement( &Object->ReferenceCount ) == 0 )
      switch( Object->Type )
	 {
	 case CSRSS_CONSOLE_MAGIC: CsrDeleteConsole( (PCSRSS_CONSOLE) Object );
	    break;
	 case CSRSS_SCREEN_BUFFER_MAGIC: CsrDeleteScreenBuffer( (PCSRSS_SCREEN_BUFFER) Object );
	    break;
	 default: DbgPrint( "CSR: Error: releaseing unknown object type" );
	 }
   ProcessData->HandleTable[h] = 0;
   return STATUS_SUCCESS;
}

NTSTATUS STDCALL CsrInsertObject( PCSRSS_PROCESS_DATA ProcessData, PHANDLE Handle, Object_t *Object )
{
   ULONG i;
   PVOID* NewBlock;

   for (i = 0; i < ProcessData->HandleTableSize; i++)
     {
	if (ProcessData->HandleTable[i] == NULL)
	  {
	     ProcessData->HandleTable[i] = Object;
	     *Handle = (HANDLE)(((i + 1) << 2) | 0x3);
	     InterlockedIncrement( &Object->ReferenceCount );
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
   InterlockedIncrement( &Object->ReferenceCount );
   ProcessData->HandleTableSize = ProcessData->HandleTableSize + 64;
   return(STATUS_SUCCESS);
}


/* EOF */
