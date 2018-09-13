/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    cmwrapr.c

Abstract:

    This module contains the source for wrapper routines called by the
    hive code, which in turn call the appropriate NT routines.

Author:

    Bryan M. Willman (bryanwi) 16-Dec-1991

Revision History:

--*/

#include    "cmp.h"

ULONG perftouchbuffer = 0;

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,CmpAllocate)
#ifdef POOL_TAGGING
#pragma alloc_text(PAGE,CmpAllocateTag)
#endif
#pragma alloc_text(PAGE,CmpFree)
#pragma alloc_text(PAGE,CmpDoFileSetSize)
#pragma alloc_text(PAGE,CmpFileRead)
#pragma alloc_text(PAGE,CmpFileWrite)
#pragma alloc_text(PAGE,CmpFileFlush)
#endif

extern BOOLEAN CmpNoWrite;

//
// never read more than 64k, neither the filesystem nor some disk drivers
// like it much.
//
#define MAX_FILE_IO 0x10000

#define CmpIoFileRead       1
#define CmpIoFileWrite      2
#define CmpIoFileSetSize    3
#define CmpIoFileFlush      4

extern struct {
    ULONG       Action;
    HANDLE      Handle;
    NTSTATUS    Status;
} CmRegistryIODebug;

//
// Storage management
//

PVOID
CmpAllocate(
    ULONG   Size,
    BOOLEAN UseForIo
    )
/*++

Routine Description:

    This routine makes more memory available to a hive.

    It is environment specific.

Arguments:

    Size - amount of space caller wants

    UseForIo - TRUE if object allocated will be target of I/O,
               FALSE if not.

Return Value:

    NULL if failure, address of allocated block if not.

--*/
{
    PVOID   result;
    ULONG   pooltype;
#if DBG
    PVOID   Caller;
    PVOID   CallerCaller;
    RtlGetCallersAddress(&Caller, &CallerCaller);
#endif

    if (CmpClaimGlobalQuota(Size) == FALSE) {
        return NULL;
    }

    pooltype = (UseForIo) ? PagedPoolCacheAligned : PagedPool;
    result = ExAllocatePoolWithTag(
                pooltype,
                Size,
                CM_POOL_TAG
                );

#if DBG
    CMLOG(CML_MINOR, CMS_POOL) {
        KdPrint(("**CmpAllocate: allocate:%08lx, ", Size));
        KdPrint(("type:%d, at:%08lx  ", PagedPool, result));
        KdPrint(("c:%08lx  cc:%08lx\n", Caller, CallerCaller));
    }
#endif

    if (result == NULL) {
        CmpReleaseGlobalQuota(Size);
    }

    return result;
}

#ifdef POOL_TAGGING
PVOID
CmpAllocateTag(
    ULONG   Size,
    BOOLEAN UseForIo,
    ULONG   Tag
    )
/*++

Routine Description:

    This routine makes more memory available to a hive.

    It is environment specific.

Arguments:

    Size - amount of space caller wants

    UseForIo - TRUE if object allocated will be target of I/O,
               FALSE if not.

Return Value:

    NULL if failure, address of allocated block if not.

--*/
{
    PVOID   result;
    ULONG   pooltype;
#if DBG
    PVOID   Caller;
    PVOID   CallerCaller;
    RtlGetCallersAddress(&Caller, &CallerCaller);
#endif

    if (CmpClaimGlobalQuota(Size) == FALSE) {
        return NULL;
    }

    pooltype = (UseForIo) ? PagedPoolCacheAligned : PagedPool;
    result = ExAllocatePoolWithTag(
                pooltype,
                Size,
                Tag
                );

#if DBG
    CMLOG(CML_MINOR, CMS_POOL) {
        KdPrint(("**CmpAllocate: allocate:%08lx, ", Size));
        KdPrint(("type:%d, at:%08lx  ", PagedPool, result));
        KdPrint(("c:%08lx  cc:%08lx\n", Caller, CallerCaller));
    }
#endif

    if (result == NULL) {
        CmpReleaseGlobalQuota(Size);
    }

    return result;
}
#endif


VOID
CmpFree(
    PVOID   MemoryBlock,
    ULONG   GlobalQuotaSize
    )
/*++

Routine Description:

    This routine frees memory that has been allocated by the registry.

    It is environment specific


Arguments:

    MemoryBlock - supplies address of memory object to free

    GlobalQuotaSize - amount of global quota to release

Return Value:

    NONE

--*/
{
#if DBG
    PVOID   Caller;
    PVOID   CallerCaller;
    RtlGetCallersAddress(&Caller, &CallerCaller);
    CMLOG(CML_MINOR, CMS_POOL) {
        KdPrint(("**FREEING:%08lx c,cc:%08lx,%08lx\n", MemoryBlock, Caller, CallerCaller));
    }
#endif
    ASSERT(GlobalQuotaSize > 0);
    CmpReleaseGlobalQuota(GlobalQuotaSize);
    ExFreePool(MemoryBlock);
    return;
}


NTSTATUS
CmpDoFileSetSize(
    PHHIVE      Hive,
    ULONG       FileType,
    ULONG       FileSize
    )
/*++

Routine Description:

    This routine sets the size of a file.  It must not return until
    the size is guaranteed.

    It is environment specific.

    Must be running in the context of the cmp worker thread.

Arguments:

    Hive - Hive we are doing I/O for

    FileType - which supporting file to use

    FileSize - 32 bit value to set the file's size to

Return Value:

    FALSE if failure
    TRUE if success

--*/
{
    PCMHIVE CmHive;
    HANDLE  FileHandle;
    NTSTATUS Status;
    FILE_END_OF_FILE_INFORMATION FileInfo;
    IO_STATUS_BLOCK IoStatus;
    BOOLEAN oldFlag;

    ASSERT(FIELD_OFFSET(CMHIVE, Hive) == 0);

    CmHive = (PCMHIVE)Hive;
    FileHandle = CmHive->FileHandles[FileType];
    if (FileHandle == NULL) {
        return TRUE;
    }

    //
    // disable hard error popups, to avoid self deadlock on bogus devices
    //
    oldFlag = IoSetThreadHardErrorMode(FALSE);

    FileInfo.EndOfFile.HighPart = 0L;
    FileInfo.EndOfFile.LowPart  = FileSize;

    ASSERT_PASSIVE_LEVEL();

    Status = ZwSetInformationFile(
                FileHandle,
                &IoStatus,
                (PVOID)&FileInfo,
                sizeof(FILE_END_OF_FILE_INFORMATION),
                FileEndOfFileInformation
                );

    if (NT_SUCCESS(Status)) {
        ASSERT(IoStatus.Status == Status);
    } else {
        
        //
        // set debugging info
        //
        CmRegistryIODebug.Action = CmpIoFileSetSize;
        CmRegistryIODebug.Handle = FileHandle;
        CmRegistryIODebug.Status = Status;
        DbgPrint("CmpFileSetSize:\tHandle=%08lx  NewLength=%08lx  \n", FileHandle, FileSize);
    }

    //
    // restore hard error popups mode
    //
    IoSetThreadHardErrorMode(oldFlag);

    return Status;
}

NTSTATUS
CmpCreateEvent(
    IN EVENT_TYPE  eventType,
    OUT PHANDLE eventHandle,
    OUT PKEVENT *event
    )
{
    NTSTATUS status;
    OBJECT_ATTRIBUTES obja;

    InitializeObjectAttributes( &obja, NULL, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL );
    status = ZwCreateEvent(
        eventHandle,
        EVENT_ALL_ACCESS,
        &obja,
        eventType,
        FALSE);
    
    if (!NT_SUCCESS(status)) {
        return status;
    }
    
    status = ObReferenceObjectByHandle(
        *eventHandle,
        EVENT_ALL_ACCESS,
        NULL,
        KernelMode,
        event,
        NULL);
    
    if (!NT_SUCCESS(status)) {
        ZwClose(*eventHandle);
        return status;
    }
    return status;
}


BOOLEAN
CmpFileRead (
    PHHIVE      Hive,
    ULONG       FileType,
    PULONG      FileOffset,
    PVOID       DataBuffer,
    ULONG       DataLength
    )
/*++

Routine Description:

    This routine reads in a buffer from a file.

    It is environment specific.

    NOTE:   We assume the handle is opened for asynchronous access,
            and that we, and not the IO system, are keeping the
            offset pointer.

    NOTE:   Only 32bit offsets are supported, even though the underlying
            IO system on NT supports 64 bit offsets.

Arguments:

    Hive - Hive we are doing I/O for

    FileType - which supporting file to use

    FileOffset - pointer to variable providing 32bit offset on input,
                 and receiving new 32bit offset on output.

    DataBuffer - pointer to buffer

    DataLength - length of buffer

Return Value:

    FALSE if failure
    TRUE if success

--*/
{
    NTSTATUS status;
    LARGE_INTEGER   Offset;
    IO_STATUS_BLOCK IoStatus;
    PCMHIVE CmHive;
    HANDLE  FileHandle;
    ULONG LengthToRead;
    HANDLE eventHandle = NULL;
    PKEVENT eventObject = NULL;

    ASSERT(FIELD_OFFSET(CMHIVE, Hive) == 0);
    CmHive = (PCMHIVE)Hive;
    FileHandle = CmHive->FileHandles[FileType];
    if (FileHandle == NULL) {
        return TRUE;
    }

    CMLOG(CML_MAJOR, CMS_IO) {
        KdPrint(("CmpFileRead:\n"));
        KdPrint(("\tHandle=%08lx  Offset=%08lx  ", FileHandle, *FileOffset));
        KdPrint(("Buffer=%08lx  Length=%08lx\n", DataBuffer, DataLength));
    }

    //
    // Detect attempt to read off end of 2gig file (this should be irrelevent)
    //
    if ((0xffffffff - *FileOffset) < DataLength) {
        CMLOG(CML_MAJOR, CMS_IO_ERROR) KdPrint(("CmpFileRead: runoff\n"));
        return FALSE;
    }

    status = CmpCreateEvent(
        SynchronizationEvent,
        &eventHandle,
        &eventObject);
    if (!NT_SUCCESS(status))
        return FALSE;

    //
    // We'd really like to just call the filesystems and have them do
    // the right thing.  But the filesystem will attempt to lock our
    // entire buffer into memory, and that may fail for large requests.
    // So we split our reads into 64k chunks and call the filesystem for
    // each one.
    //
    ASSERT_PASSIVE_LEVEL();
    while (DataLength > 0) {

        //
        // Convert ULONG to Large
        //
        Offset.LowPart = *FileOffset;
        Offset.HighPart = 0L;

        //
        // trim request down if necessary.
        //
        if (DataLength > MAX_FILE_IO) {
            LengthToRead = MAX_FILE_IO;
        } else {
            LengthToRead = DataLength;
        }

        status = ZwReadFile(
                    FileHandle,
                    eventHandle,
                    NULL,               // apcroutine
                    NULL,               // apccontext
                    &IoStatus,
                    DataBuffer,
                    LengthToRead,
                    &Offset,
                    NULL                // key
                    );

        if (STATUS_PENDING == status) {
            status = KeWaitForSingleObject(eventObject, Executive,
                                           KernelMode, FALSE, NULL);
            ASSERT(STATUS_SUCCESS == status);
            status = IoStatus.Status;
        }

        //
        // adjust offsets
        //
        *FileOffset = Offset.LowPart + LengthToRead;
        DataLength -= LengthToRead;
        (PUCHAR)DataBuffer += LengthToRead;

        if (NT_SUCCESS(status)) {
            ASSERT(IoStatus.Status == status);
            if (IoStatus.Information != LengthToRead) {
                CMLOG(CML_MAJOR, CMS_IO_ERROR) {
                    KdPrint(("CmpFileRead:\n\t"));
                    KdPrint(("Failure1: status = %08lx  ", status));
                    KdPrint(("IoInformation = %08lx\n", IoStatus.Information));
                }
                ObDereferenceObject(eventObject);
                ZwClose(eventHandle);
                return FALSE;
            }
        } else {
            //
            // set debugging info
            //
            CmRegistryIODebug.Action = CmpIoFileRead;
            CmRegistryIODebug.Handle = FileHandle;
            CmRegistryIODebug.Status = status;
            DbgPrint("CmpFileRead:\tFailure2: status = %08lx  IoStatus = %08lx\n", status, IoStatus.Status);

            ObDereferenceObject(eventObject);
            ZwClose(eventHandle);
            return FALSE;
        }

    }
    ObDereferenceObject(eventObject);
    ZwClose(eventHandle);
    return TRUE;
}



BOOLEAN
CmpFileWrite(
    PHHIVE      Hive,
    ULONG       FileType,
    PCMP_OFFSET_ARRAY offsetArray,
    ULONG offsetArrayCount,
    PULONG      FileOffset
    )
/*++

Routine Description:

    This routine writes an array of buffers out to a file.

    It is environment specific.

    NOTE:   We assume the handle is opened for asynchronous access,
            and that we, and not the IO system, are keeping the
            offset pointer.

    NOTE:   Only 32bit offsets are supported, even though the underlying
            IO system on NT supports 64 bit offsets.

Arguments:

    Hive - Hive we are doing I/O for

    FileType - which supporting file to use

    offsetArray - array of structures where each structure holds a 32bit offset
                  into the Hive file and pointer the a buffer written to that
                  file offset.

    offsetArrayCount - number of elements in the offsetArray.

    FileOffset - returns the file offset after the last write to the file.

Return Value:

    FALSE if failure
    TRUE if success

--*/
{
    NTSTATUS status;
    LARGE_INTEGER   Offset;
    PCMHIVE CmHive;
    HANDLE  FileHandle;
    ULONG LengthToWrite;
    LONG WaitBufferCount = 0;
    LONG idx;
    ULONG arrayCount = 0;
    PVOID       DataBuffer;
    ULONG       DataLength;
    HANDLE eventHandles[MAXIMUM_WAIT_OBJECTS];
    PKEVENT eventObjects[MAXIMUM_WAIT_OBJECTS];
    KWAIT_BLOCK waitBlockArray[MAXIMUM_WAIT_OBJECTS];
    IO_STATUS_BLOCK IoStatus[MAXIMUM_WAIT_OBJECTS];
    BOOLEAN ret_val = TRUE;

    if (CmpNoWrite) {
        return TRUE;
    }

    ASSERT(FIELD_OFFSET(CMHIVE, Hive) == 0);
    CmHive = (PCMHIVE)Hive;
    FileHandle = CmHive->FileHandles[FileType];
    if (FileHandle == NULL) {
        return TRUE;
    }

    CMLOG(CML_MAJOR, CMS_IO) {
        KdPrint(("CmpFileWrite:\n"));
        KdPrint(("\tHandle=%08lx  ", FileHandle));
    }

    for (idx = 0; idx < MAXIMUM_WAIT_OBJECTS; idx++) {
        eventHandles[idx] = NULL;
    }

    // Bring pages being written into memory first to allow disk to write
    // buffer contiguously.
    for (idx = 0; (ULONG) idx < offsetArrayCount; idx++) {
        char * start = offsetArray[idx].DataBuffer;
        char * end = (char *) start + offsetArray[idx].DataLength;
        while (start < end) {
            // perftouchbuffer globally declared so that compiler won't try
            // to remove it and this loop (if its smart enough?).
            perftouchbuffer += (ULONG) *start;
            start += PAGE_SIZE;
        }
    }

    //
    // We'd really like to just call the filesystems and have them do
    // the right thing.  But the filesystem will attempt to lock our
    // entire buffer into memory, and that may fail for large requests.
    // So we split our reads into 64k chunks and call the filesystem for
    // each one.
    //
    ASSERT_PASSIVE_LEVEL();
    arrayCount = 0;
    DataLength = 0;
    // This outer loop is hit more than once if the MAXIMUM_WAIT_OBJECTS limit
    // is hit before the offset array is drained.
    while (arrayCount < offsetArrayCount) {
        WaitBufferCount = 0;

        // This loop fills the wait buffer.
        while ((arrayCount < offsetArrayCount) &&
               (WaitBufferCount < MAXIMUM_WAIT_OBJECTS)) {

            // If data length isn't zero than the wait buffer filled before the
            // buffer in the last offsetArray element was sent to write file.
            if (DataLength == 0) {
                *FileOffset = offsetArray[arrayCount].FileOffset;
                DataBuffer =  offsetArray[arrayCount].DataBuffer;
                DataLength =  offsetArray[arrayCount].DataLength;
                //
                // Detect attempt to read off end of 2gig file
                // (this should be irrelevent)
                //
                if ((0xffffffff - *FileOffset) < DataLength) {
                    CMLOG(CML_MAJOR, CMS_IO_ERROR) KdPrint(("CmpFileWrite: runoff\n"));
                    goto Error_Exit;
                }
            }
            // else still more to write out of last buffer.

            while ((DataLength > 0) && (WaitBufferCount < MAXIMUM_WAIT_OBJECTS)) {

                //
                // Convert ULONG to Large
                //
                Offset.LowPart = *FileOffset;
                Offset.HighPart = 0L;

                //
                // trim request down if necessary.
                //
                if (DataLength > MAX_FILE_IO) {
                    LengthToWrite = MAX_FILE_IO;
                } else {
                    LengthToWrite = DataLength;
                }

                // Previously created events are reused.
                if (eventHandles[WaitBufferCount] == NULL) {
                    status = CmpCreateEvent(SynchronizationEvent,
                                            &eventHandles[WaitBufferCount],
                                            &eventObjects[WaitBufferCount]);
                    if (!NT_SUCCESS(status)) {
                        // Make sure we don't try to clean this up.
                        eventHandles[WaitBufferCount] = NULL;
                        goto Error_Exit;
                    }
                }
                        
                status = ZwWriteFile(FileHandle,
                                     eventHandles[WaitBufferCount],
                                     NULL,               // apcroutine
                                     NULL,               // apccontext
                                     &IoStatus[WaitBufferCount],
                                     DataBuffer,
                                     LengthToWrite,
                                     &Offset,
                                     NULL);
                        
                if (!NT_SUCCESS(status)) {
                    goto Error_Exit;
                }

                WaitBufferCount++;
                
                //
                // adjust offsets
                //
                *FileOffset = Offset.LowPart + LengthToWrite;
                DataLength -= LengthToWrite;
                (PUCHAR)DataBuffer += LengthToWrite;
            } // while (DataLength > 0 && WaitBufferCount < MAXIMUM_WAIT_OBJECTS)
            
            arrayCount++;
            
        } // while (arrayCount < offsetArrayCount && 
          //        WaitBufferCount < MAXIMUM_WAIT_OBJECTS)

        status = KeWaitForMultipleObjects(WaitBufferCount, 
                                          eventObjects,
                                          WaitAll,
                                          Executive,
                                          KernelMode, 
                                          FALSE, 
                                          NULL,
                                          waitBlockArray);
        
        if (!NT_SUCCESS(status))
            goto Error_Exit;
        
        for (idx = 0; idx < WaitBufferCount; idx++) {
            if (!NT_SUCCESS(IoStatus[idx].Status)) {
                ret_val = FALSE;
                goto Done;
            }
        }
        
        // There may still be more to do if the last element held a big buffer
        // and the wait buffer filled before it was all sent to the file.
        if (DataLength > 0) {
            arrayCount--;
        }

    } // while (arrayCount < offsetArrayCount)

    ret_val = TRUE;

    goto Done;
Error_Exit:
    //
    // set debugging info
    //
    CmRegistryIODebug.Action = CmpIoFileWrite;
    CmRegistryIODebug.Handle = FileHandle;
    CmRegistryIODebug.Status = status;
    DbgPrint("CmpFileWrite: error exiting %d\n", status);
    //
    // if WaitBufferCount > 0 then we have successfully issued
    // some I/Os, but not all of them. This is an error, but we
    // cannot return from this routine until all the successfully
    // issued I/Os have completed.
    //
    if (WaitBufferCount > 0) {
        status = KeWaitForMultipleObjects(WaitBufferCount, 
                                          eventObjects,
                                          WaitAll,
                                          Executive,
                                          KernelMode, 
                                          FALSE, 
                                          NULL,
                                          waitBlockArray);
    }


    ret_val = FALSE;
Done:
    idx = 0;
    // Clean up open event handles and objects.
    while ((idx < MAXIMUM_WAIT_OBJECTS) && (eventHandles[idx] != NULL)) {
        ObDereferenceObject(eventObjects[idx]);
        ZwClose(eventHandles[idx]);
        idx++;
    }
    return ret_val;
}


BOOLEAN
CmpFileFlush (
    PHHIVE      Hive,
    ULONG       FileType
    )
/*++

Routine Description:

    This routine performs a flush on a file handle.

Arguments:

    Hive - Hive we are doing I/O for

    FileType - which supporting file to use

Return Value:

    FALSE if failure
    TRUE if success

--*/
{
    NTSTATUS status;
    IO_STATUS_BLOCK IoStatus;
    PCMHIVE CmHive;
    HANDLE  FileHandle;

    ASSERT(FIELD_OFFSET(CMHIVE, Hive) == 0);
    CmHive = (PCMHIVE)Hive;
    FileHandle = CmHive->FileHandles[FileType];
    if (FileHandle == NULL) {
        return TRUE;
    }

    if (CmpNoWrite) {
        return TRUE;
    }

    CMLOG(CML_MAJOR, CMS_IO) {
        KdPrint(("CmpFileFlush:\n\tHandle = %08lx\n", FileHandle));
    }

    ASSERT_PASSIVE_LEVEL();

    status = ZwFlushBuffersFile(
                FileHandle,
                &IoStatus
                );

    if (NT_SUCCESS(status)) {
        ASSERT(IoStatus.Status == status);
        return TRUE;
    } else {
        //
        // set debugging info
        //
        CmRegistryIODebug.Action = CmpIoFileFlush;
        CmRegistryIODebug.Handle = FileHandle;
        CmRegistryIODebug.Status = status;
        DbgPrint("CmpFileFlush:\tFailure1: status = %08lx  IoStatus = %08lx\n",status,IoStatus.Status);
        return FALSE;
    }
    return TRUE;
}
