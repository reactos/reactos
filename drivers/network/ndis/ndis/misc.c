/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS NDIS library
 * FILE:        ndis/misc.c
 */

#include "ndissys.h"

extern LONG CancelId;

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
   ExInterlockedAddUlong ( Addend, Increment, &SpinLock->SpinLock );
}

/*
 * @implemented
 */
VOID
EXPORT
NdisInterlockedAddLargeInteger(
    IN PLARGE_INTEGER Addend,
    IN LARGE_INTEGER Increment,
    IN PNDIS_SPIN_LOCK SpinLock)
{
    /* This needs to be verified. The documentation
     * seems to be missing but it is exported by
     * NDIS 5.1 so I'm implementing it like the other
     * interlocked routines
     */

    ExInterlockedAddLargeInteger(Addend, Increment, &SpinLock->SpinLock);
}

/*
 * @implemented
 */
LONG
EXPORT
NdisCompareAnsiString(
    IN PNDIS_ANSI_STRING String1,
    IN PNDIS_ANSI_STRING String2,
    BOOLEAN CaseInSensitive)
{
    /* This one needs to be verified also. See the
     * comment in NdisInterlockedAddLargeInteger
     */

    return RtlCompareString(String1, String2, CaseInSensitive);
}

/*
 * @implemented
 */
LONG
EXPORT
NdisCompareUnicodeString(
    IN PNDIS_STRING String1,
    IN PNDIS_STRING String2,
    IN BOOLEAN CaseInSensitive)
{
    /* This one needs to be verified also. See the
     * comment in NdisInterlockedAddLargeInteger
     */

    return RtlCompareUnicodeString(String1, String2, CaseInSensitive);
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
  return ExInterlockedInsertHeadList ( ListHead, ListEntry, &SpinLock->SpinLock );
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
  return ExInterlockedInsertTailList ( ListHead, ListEntry, &SpinLock->SpinLock );
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
  return ExInterlockedRemoveHeadList ( ListHead, &SpinLock->SpinLock );
}

typedef struct _NDIS_HANDLE_OBJECT
{
  HANDLE FileHandle;
  BOOLEAN Mapped;
  ULONG FileLength;
  PVOID MapBuffer;
} NDIS_HANDLE_OBJECT, *PNDIS_HANDLE_OBJECT;

FORCEINLINE
PNDIS_HANDLE_OBJECT
NDIS_HANDLE_TO_POBJECT ( NDIS_HANDLE handle )
{
  return (PNDIS_HANDLE_OBJECT)handle;
}

FORCEINLINE
NDIS_HANDLE
NDIS_POBJECT_TO_HANDLE ( PNDIS_HANDLE_OBJECT obj )
{
  return (NDIS_HANDLE)obj;
}

static const WCHAR NDIS_FILE_FOLDER[] = L"\\SystemRoot\\System32\\Drivers\\";

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
      NDIS_DbgPrint(MIN_TRACE, ("File already mapped\n"));
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

  NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));

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

  NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));

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

  NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));

  ASSERT ( Status && FileName );

  *Status = NDIS_STATUS_SUCCESS;
  FullFileName.Buffer = NULL;

  FullFileName.Length = sizeof(NDIS_FILE_FOLDER) - sizeof(UNICODE_NULL);
  FullFileName.MaximumLength = FileName->MaximumLength + FullFileName.Length;
  FullFileName.Buffer = ExAllocatePool ( NonPagedPool, FullFileName.MaximumLength );

  if ( !FullFileName.Buffer )
  {
    NDIS_DbgPrint(MIN_TRACE, ("Insufficient resources\n"));
    *Status = NDIS_STATUS_RESOURCES;
    goto cleanup;
  }

  FileHandleObject = ExAllocatePool ( NonPagedPool, sizeof(NDIS_HANDLE_OBJECT) );
  if ( !FileHandleObject )
  {
    NDIS_DbgPrint(MIN_TRACE, ("Insufficient resources\n"));
    *Status = NDIS_STATUS_RESOURCES;
    goto cleanup;
  }
  memset ( FileHandleObject, 0, sizeof(NDIS_HANDLE_OBJECT) );

  memmove ( FullFileName.Buffer, NDIS_FILE_FOLDER, FullFileName.Length );
  *Status = RtlAppendUnicodeStringToString ( &FullFileName, FileName );
  if ( !NT_SUCCESS(*Status) )
  {
    NDIS_DbgPrint(MIN_TRACE, ("RtlAppendUnicodeStringToString failed (%x)\n", *Status));
    *Status = NDIS_STATUS_FAILURE;
    goto cleanup;
  }

  InitializeObjectAttributes ( &ObjectAttributes,
    &FullFileName,
    OBJ_CASE_INSENSITIVE|OBJ_KERNEL_HANDLE,
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
    FILE_OPEN, // ULONG CreateDisposition
    FILE_SYNCHRONOUS_IO_NONALERT, // ULONG CreateOptions
    0, // PVOID EaBuffer
    0 ); // ULONG EaLength

  if ( !NT_SUCCESS(*Status) )
  {
    NDIS_DbgPrint(MIN_TRACE, ("ZwCreateFile failed (%x) Name %wZ\n", *Status, FileName));
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
	return KeNumberProcessors;
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
    NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));

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

    NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));

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
  return (PSINGLE_LIST_ENTRY)ExInterlockedPopEntrySList ( ListHead, Lock );
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
  return (PSINGLE_LIST_ENTRY)ExInterlockedPushEntrySList ( ListHead, (PSLIST_ENTRY)ListEntry, Lock );
}


VOID
NTAPI
ndisProcWorkItemHandler(PVOID pContext)
{
    PNDIS_WORK_ITEM pNdisItem = (PNDIS_WORK_ITEM)pContext;

    NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));

    pNdisItem->Routine(pNdisItem, pNdisItem->Context);
}

NDIS_STATUS
EXPORT
NdisScheduleWorkItem(
    IN PNDIS_WORK_ITEM  pWorkItem)
{
    PWORK_QUEUE_ITEM pntWorkItem = (PWORK_QUEUE_ITEM)pWorkItem->WrapperReserved;

    NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));

    ExInitializeWorkItem(pntWorkItem, ndisProcWorkItemHandler, pWorkItem);
    ExQueueWorkItem(pntWorkItem, DelayedWorkQueue);
    return NDIS_STATUS_SUCCESS;
}

/*
 * @implemented
 */
VOID
EXPORT
NdisGetCurrentProcessorCpuUsage(
    PULONG  pCpuUsage)
/*
 * FUNCTION: Returns how busy the current processor is as a percentage
 * ARGUMENTS:
 *     pCpuUsage = Pointer to a buffer to place CPU usage
 */
{
    NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));

    ExGetCurrentProcessorCpuUsage(pCpuUsage);
}

/*
 * @implemented
 */
ULONG
EXPORT
NdisGetSharedDataAlignment(VOID)
{
    NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));

    return KeGetRecommendedSharedDataAlignment();
}

/*
 * @implemented
 */
UINT
EXPORT
NdisGetVersion(VOID)
{
    NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));

    return NDIS_VERSION;
}

/*
 * @implemented
 */
UCHAR
EXPORT
NdisGeneratePartialCancelId(VOID)
{
    UCHAR PartialCancelId;

    PartialCancelId = (UCHAR)InterlockedIncrement(&CancelId);

    NDIS_DbgPrint(MAX_TRACE, ("Cancel ID %u\n", PartialCancelId));

    return PartialCancelId;
}

/* EOF */
