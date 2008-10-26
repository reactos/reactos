/* $Id$
 *
 * reactos/subsys/csrss/api/handle.c
 *
 * CSRSS handle functions
 *
 * ReactOS Operating System
 */

/* INCLUDES ******************************************************************/

#include <csrss.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

static unsigned ObjectDefinitionsCount = 0;
static PCSRSS_OBJECT_DEFINITION ObjectDefinitions = NULL;

static BOOL
CsrIsConsoleHandle(HANDLE Handle)
{
  return ((ULONG_PTR)Handle & 0x10000003) == 0x3;
}


NTSTATUS FASTCALL
CsrRegisterObjectDefinitions(PCSRSS_OBJECT_DEFINITION NewDefinitions)
{
  unsigned NewCount;
  PCSRSS_OBJECT_DEFINITION Scan;
  PCSRSS_OBJECT_DEFINITION New;

  NewCount = 0;
  for (Scan = NewDefinitions; 0 != Scan->Type; Scan++)
    {
      NewCount++;
    }

  New = RtlAllocateHeap(CsrssApiHeap, 0,
                        (ObjectDefinitionsCount + NewCount)
                        * sizeof(CSRSS_OBJECT_DEFINITION));
  if (NULL == New)
    {
      DPRINT1("Unable to allocate memory\n");
      return STATUS_NO_MEMORY;
    }
  if (0 != ObjectDefinitionsCount)
    {
      RtlCopyMemory(New, ObjectDefinitions,
                    ObjectDefinitionsCount * sizeof(CSRSS_OBJECT_DEFINITION));
      RtlFreeHeap(CsrssApiHeap, 0, ObjectDefinitions);
    }
  RtlCopyMemory(New + ObjectDefinitionsCount, NewDefinitions,
                NewCount * sizeof(CSRSS_OBJECT_DEFINITION));
  ObjectDefinitions = New;
  ObjectDefinitionsCount += NewCount;

  return STATUS_SUCCESS;
}

NTSTATUS STDCALL CsrGetObject( PCSRSS_PROCESS_DATA ProcessData, HANDLE Handle, Object_t **Object, DWORD Access )
{
  ULONG_PTR h = (ULONG_PTR)Handle >> 2;
  DPRINT("CsrGetObject, Object: %x, %x, %x\n", Object, Handle, ProcessData ? ProcessData->HandleTableSize : 0);

  RtlEnterCriticalSection(&ProcessData->HandleTableLock);
  if (!CsrIsConsoleHandle(Handle) || h >= ProcessData->HandleTableSize
      || (*Object = ProcessData->HandleTable[h].Object) == NULL
      || ~ProcessData->HandleTable[h].Access & Access)
    {
      DPRINT1("CsrGetObject returning invalid handle (%x)\n", Handle);
      RtlLeaveCriticalSection(&ProcessData->HandleTableLock);
      return STATUS_INVALID_HANDLE;
    }
  _InterlockedIncrement(&(*Object)->ReferenceCount);
  RtlLeaveCriticalSection(&ProcessData->HandleTableLock);
  //   DbgPrint( "CsrGetObject returning\n" );
  return STATUS_SUCCESS;
}


NTSTATUS STDCALL
CsrReleaseObjectByPointer(Object_t *Object)
{
  unsigned DefIndex;

  /* dec ref count */
  if (_InterlockedDecrement(&Object->ReferenceCount) == 0)
    {
      for (DefIndex = 0; DefIndex < ObjectDefinitionsCount; DefIndex++)
        {
          if (Object->Type == ObjectDefinitions[DefIndex].Type)
            {
              (ObjectDefinitions[DefIndex].CsrCleanupObjectProc)(Object);
              return STATUS_SUCCESS;
            }
        }

      DPRINT1("CSR: Error: releasing unknown object type 0x%x", Object->Type);
    }

  return STATUS_SUCCESS;
}


NTSTATUS STDCALL
CsrReleaseObject(PCSRSS_PROCESS_DATA ProcessData,
                 HANDLE Handle)
{
  ULONG_PTR h = (ULONG_PTR)Handle >> 2;
  Object_t *Object;

  RtlEnterCriticalSection(&ProcessData->HandleTableLock);
  if (h >= ProcessData->HandleTableSize
      || (Object = ProcessData->HandleTable[h].Object) == NULL)
    {
      RtlLeaveCriticalSection(&ProcessData->HandleTableLock);
      return STATUS_INVALID_HANDLE;
    }
  ProcessData->HandleTable[h].Object = NULL;
  RtlLeaveCriticalSection(&ProcessData->HandleTableLock);

  return CsrReleaseObjectByPointer(Object);
}

NTSTATUS STDCALL CsrInsertObject(PCSRSS_PROCESS_DATA ProcessData,
                                 PHANDLE Handle,
                                 Object_t *Object,
                                 DWORD Access,
                                 BOOL Inheritable)
{
   ULONG i;
   PVOID* Block;

   RtlEnterCriticalSection(&ProcessData->HandleTableLock);

   for (i = 0; i < ProcessData->HandleTableSize; i++)
     {
	if (ProcessData->HandleTable[i].Object == NULL)
	  {
            break;
	  }
     }
   if (i >= ProcessData->HandleTableSize)
     {
       Block = RtlAllocateHeap(CsrssApiHeap,
			       HEAP_ZERO_MEMORY,
			       (ProcessData->HandleTableSize + 64) * sizeof(CSRSS_HANDLE));
       if (Block == NULL)
         {
           RtlLeaveCriticalSection(&ProcessData->HandleTableLock);
	   return(STATUS_UNSUCCESSFUL);
         }
       RtlCopyMemory(Block,
		     ProcessData->HandleTable,
		     ProcessData->HandleTableSize * sizeof(CSRSS_HANDLE));
       Block = _InterlockedExchangePointer((volatile void*)&ProcessData->HandleTable, Block);
       RtlFreeHeap( CsrssApiHeap, 0, Block );
       ProcessData->HandleTableSize += 64;
     }
   ProcessData->HandleTable[i].Object = Object;
   ProcessData->HandleTable[i].Access = Access;
   ProcessData->HandleTable[i].Inheritable = Inheritable;
   *Handle = (HANDLE)(ULONG_PTR)((i << 2) | 0x3);
   _InterlockedIncrement( &Object->ReferenceCount );
   RtlLeaveCriticalSection(&ProcessData->HandleTableLock);
   return(STATUS_SUCCESS);
}

NTSTATUS STDCALL CsrDuplicateHandleTable(PCSRSS_PROCESS_DATA SourceProcessData,
                                         PCSRSS_PROCESS_DATA TargetProcessData)
{
    ULONG i;

    if (TargetProcessData->HandleTableSize)
    {
        return STATUS_INVALID_PARAMETER;
    }

    RtlEnterCriticalSection(&SourceProcessData->HandleTableLock);

    /* we are called from CreateProcessData, it isn't necessary to lock the target process data */

    TargetProcessData->HandleTable = RtlAllocateHeap(CsrssApiHeap,
			                             HEAP_ZERO_MEMORY,
			                             SourceProcessData->HandleTableSize * sizeof(CSRSS_HANDLE));
    if (TargetProcessData->HandleTable == NULL)
    {
        RtlLeaveCriticalSection(&SourceProcessData->HandleTableLock);
	return(STATUS_UNSUCCESSFUL);
    }
    TargetProcessData->HandleTableSize = SourceProcessData->HandleTableSize;
    for (i = 0; i < SourceProcessData->HandleTableSize; i++)
    {
        if (SourceProcessData->HandleTable[i].Object != NULL
            && SourceProcessData->HandleTable[i].Inheritable)
        {
            TargetProcessData->HandleTable[i] = SourceProcessData->HandleTable[i];
            _InterlockedIncrement( &SourceProcessData->HandleTable[i].Object->ReferenceCount );
        }
    }
   RtlLeaveCriticalSection(&SourceProcessData->HandleTableLock);
   return(STATUS_SUCCESS);
}

NTSTATUS STDCALL CsrVerifyObject( PCSRSS_PROCESS_DATA ProcessData, HANDLE Handle )
{
  ULONG_PTR h = (ULONG_PTR)Handle >> 2;

  if (h >= ProcessData->HandleTableSize
      || ProcessData->HandleTable[h].Object == NULL)
    {
      return STATUS_INVALID_HANDLE;
    }

  return STATUS_SUCCESS;
}

/* EOF */
