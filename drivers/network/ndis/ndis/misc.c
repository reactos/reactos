/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS NDIS library
 * FILE:        ndis/misc.c
 */

#include "ndissys.h"

/*
 * @implemented
 */
#undef NdisInterlockedAddUlong
VOID
EXPORT
NdisInterlockedAddUlong (
    IN  PULONG          Addend,
    IN  ULONG           Increment,
    IN  PNDIS_SPIN_LOCK SpinLock)
{
   ExInterlockedAddUlong ( Addend, Increment, (PKSPIN_LOCK)SpinLock );
}


/*
 * @implemented
 */
#undef NdisInterlockedInsertHeadList
PLIST_ENTRY
EXPORT
NdisInterlockedInsertHeadList(
    IN  PLIST_ENTRY     ListHead,
    IN  PLIST_ENTRY     ListEntry,
    IN  PNDIS_SPIN_LOCK SpinLock)
{
  return ExInterlockedInsertHeadList ( ListHead, ListEntry, (PKSPIN_LOCK)SpinLock );
}


/*
 * @implemented
 */
#undef NdisInterlockedInsertTailList
PLIST_ENTRY
EXPORT
NdisInterlockedInsertTailList(
    IN  PLIST_ENTRY     ListHead,
    IN  PLIST_ENTRY     ListEntry,
    IN  PNDIS_SPIN_LOCK SpinLock)
{
  return ExInterlockedInsertTailList ( ListHead, ListEntry, (PKSPIN_LOCK)SpinLock );
}


/*
 * @implemented
 */
#undef NdisInterlockedRemoveHeadList
PLIST_ENTRY
EXPORT
NdisInterlockedRemoveHeadList(
    IN  PLIST_ENTRY     ListHead,
    IN  PNDIS_SPIN_LOCK SpinLock)
{
  return ExInterlockedRemoveHeadList ( ListHead, (PKSPIN_LOCK)SpinLock );
}

typedef struct _NDIS_HANDLE_OBJECT
{
  HANDLE FileHandle;
  BOOLEAN Mapped;
  ULONG FileLength;
  PVOID MapBuffer;
} NDIS_HANDLE_OBJECT, *PNDIS_HANDLE_OBJECT;

__inline
PNDIS_HANDLE_OBJECT
NDIS_HANDLE_TO_POBJECT ( NDIS_HANDLE handle )
{
  return (PNDIS_HANDLE_OBJECT)handle;
}

__inline
NDIS_HANDLE
NDIS_POBJECT_TO_HANDLE ( PNDIS_HANDLE_OBJECT obj )
{
  return (NDIS_HANDLE)obj;
}

const WCHAR* NDIS_FILE_FOLDER = L"\\SystemRoot\\System32\\Drivers\\";

/*
 * @implemented
 */
VOID
EXPORT
NdisMapFile(
    OUT PNDIS_STATUS    Status,
    OUT PVOID           *MappedBuffer,
    IN  NDIS_HANDLE     FileHandle)
{
  PNDIS_HANDLE_OBJECT HandleObject = (PNDIS_HANDLE_OBJECT) FileHandle;

  NDIS_DbgPrint(MAX_TRACE, ("called: FileHandle 0x%x\n", FileHandle));

  if (HandleObject->Mapped)
  {
      /* If a file already mapped we will return an error code */
      *Status = NDIS_STATUS_ALREADY_MAPPED;
      return;
  }

  HandleObject->Mapped = TRUE;
  *MappedBuffer = HandleObject->MapBuffer;

  /* Set returned status */
  *Status = STATUS_SUCCESS;
}

/*
 * @implemented
 */
VOID
EXPORT
NdisUnmapFile(
    IN  NDIS_HANDLE FileHandle)
{
  PNDIS_HANDLE_OBJECT HandleObject = (PNDIS_HANDLE_OBJECT) FileHandle;

  HandleObject->Mapped = FALSE;
}

/*
 * @implemented
 */
VOID
EXPORT
NdisCloseFile(
    IN  NDIS_HANDLE FileHandle)
{
  PNDIS_HANDLE_OBJECT FileHandleObject;

  ASSERT_IRQL(PASSIVE_LEVEL);

  ASSERT ( FileHandle );

  FileHandleObject = NDIS_HANDLE_TO_POBJECT(FileHandle);

  ASSERT ( FileHandleObject->FileHandle );

  if ( FileHandleObject->Mapped )
    NdisUnmapFile ( FileHandle );

  ZwClose ( FileHandleObject->FileHandle );

  memset ( FileHandleObject, 0, sizeof(NDIS_HANDLE_OBJECT) );

  ExFreePool ( FileHandleObject );
}


/*
 * @implemented
 */
VOID
EXPORT
NdisOpenFile(
    OUT PNDIS_STATUS            Status,
    OUT PNDIS_HANDLE            FileHandle,
    OUT PUINT                   FileLength,
    IN  PNDIS_STRING            FileName,
    IN  NDIS_PHYSICAL_ADDRESS   HighestAcceptableAddress)
{
  NDIS_STRING FullFileName;
  OBJECT_ATTRIBUTES ObjectAttributes;
  PNDIS_HANDLE_OBJECT FileHandleObject = NULL;
  IO_STATUS_BLOCK IoStatusBlock;

  ASSERT_IRQL(PASSIVE_LEVEL);

  *Status = NDIS_STATUS_SUCCESS;
  FullFileName.Buffer = NULL;

  ASSERT ( Status && FileName );

  FullFileName.Length = sizeof(NDIS_FILE_FOLDER);
  FullFileName.MaximumLength = FileName->MaximumLength + sizeof(NDIS_FILE_FOLDER);
  FullFileName.Buffer = ExAllocatePool ( NonPagedPool, FullFileName.MaximumLength );

  if ( !FullFileName.Buffer )
  {
    *Status = NDIS_STATUS_RESOURCES;
    goto cleanup;
  }

  FileHandleObject = ExAllocatePool ( NonPagedPool, sizeof(NDIS_HANDLE_OBJECT) );
  if ( !FileHandleObject )
  {
    *Status = NDIS_STATUS_RESOURCES;
    goto cleanup;
  }
  memset ( FileHandleObject, 0, sizeof(NDIS_HANDLE_OBJECT) );

  memmove ( FullFileName.Buffer, NDIS_FILE_FOLDER, FullFileName.Length );
  *Status = RtlAppendUnicodeStringToString ( &FullFileName, FileName );
  if ( !NT_SUCCESS(*Status) )
  {
    *Status = NDIS_STATUS_FAILURE;
    goto cleanup;
  }

  InitializeObjectAttributes ( &ObjectAttributes,
    &FullFileName,
    OBJ_CASE_INSENSITIVE,
    NULL,
    NULL );

  *Status = ZwCreateFile (
    &FileHandleObject->FileHandle,
    FILE_READ_DATA|SYNCHRONIZE,
    &ObjectAttributes,
    &IoStatusBlock,
    NULL, // PLARGE_INTEGER AllocationSize
    0, // ULONG FileAttributes
    FILE_SHARE_READ, // ULONG ShareAccess
    FILE_CREATE, // ULONG CreateDisposition
    FILE_SYNCHRONOUS_IO_NONALERT, // ULONG CreateOptions
    0, // PVOID EaBuffer
    0 ); // ULONG EaLength
  
  if ( !NT_SUCCESS(*Status) )
  {
    *Status = NDIS_STATUS_FAILURE;
  }

cleanup:
  if ( FullFileName.Buffer != NULL )
  {
    ExFreePool ( FullFileName.Buffer );
    FullFileName.Buffer = NULL;
  }
  if ( !NT_SUCCESS(*Status) )
  {
    if( FileHandleObject ) {
	ExFreePool ( FileHandleObject );
	FileHandleObject = NULL;
    }
    *FileHandle = NULL;
  }
  else
    *FileHandle = NDIS_POBJECT_TO_HANDLE(FileHandleObject);

  return;
}

/*
 * @implemented
 */
CCHAR
EXPORT
NdisSystemProcessorCount(
    VOID)
{
	return (CCHAR)KeNumberProcessors;
}


/*
 * @implemented
 */
VOID
EXPORT
NdisGetCurrentProcessorCounts(
    OUT PULONG  pIdleCount,
    OUT PULONG  pKernelAndUser,
    OUT PULONG  pIndex)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    ExGetCurrentProcessorCounts( (PULONG) pIdleCount, (PULONG) pKernelAndUser, (PULONG) pIndex); 
}


/*
 * @implemented
 */
VOID
EXPORT
NdisGetSystemUpTime(OUT PULONG pSystemUpTime)
{           
    ULONG Increment;
    LARGE_INTEGER TickCount;

    /* Get the increment and current tick count */
    Increment = KeQueryTimeIncrement();
    KeQueryTickCount(&TickCount);

    /* Convert to milliseconds and return */
    TickCount.QuadPart *= Increment;
    TickCount.QuadPart /= (10 * 1000);
    *pSystemUpTime = TickCount.LowPart;
}

/*
 * @implemented
 */
#undef NdisInterlockedDecrement
LONG
EXPORT
NdisInterlockedDecrement(
    IN  PLONG   Addend)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
  return InterlockedDecrement ( Addend );
}


/*
 * @implemented
 */
#undef NdisInterlockedIncrement
LONG
EXPORT
NdisInterlockedIncrement(
    IN  PLONG   Addend)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
  return InterlockedIncrement ( Addend );
}


/*
 * @implemented
 */
#undef NdisInterlockedPopEntrySList
PSINGLE_LIST_ENTRY
EXPORT
NdisInterlockedPopEntrySList(
    IN  PSLIST_HEADER   ListHead,
    IN  PKSPIN_LOCK     Lock)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
  return ExInterlockedPopEntrySList ( ListHead, Lock );
}


/*
 * @implemented
 */
#undef NdisInterlockedPushEntrySList
PSINGLE_LIST_ENTRY
EXPORT
NdisInterlockedPushEntrySList(
    IN  PSLIST_HEADER       ListHead,
    IN  PSINGLE_LIST_ENTRY  ListEntry,
    IN  PKSPIN_LOCK         Lock)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
  return ExInterlockedPushEntrySList ( ListHead, ListEntry, Lock );
}


VOID
NTAPI
ndisProcWorkItemHandler(PVOID pContext)
{
    PNDIS_WORK_ITEM pNdisItem = (PNDIS_WORK_ITEM)pContext;
    pNdisItem->Routine(pNdisItem, pNdisItem->Context);
}

EXPORT
NDIS_STATUS
NdisScheduleWorkItem(
    IN PNDIS_WORK_ITEM  pWorkItem)
{
    PWORK_QUEUE_ITEM pntWorkItem = (PWORK_QUEUE_ITEM)pWorkItem->WrapperReserved;
    ExInitializeWorkItem(pntWorkItem, ndisProcWorkItemHandler, pWorkItem);
    ExQueueWorkItem(pntWorkItem, CriticalWorkQueue);
    return NDIS_STATUS_SUCCESS;
}

/* EOF */
