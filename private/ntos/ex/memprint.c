/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    memprint.c

Abstract:

    This module contains the routines to implement in-memory DbgPrint.
    DbgPrint text is stored in a large circular buffer, and optionally
    written to a file and/or the debug console.  Output to file is
    buffered to allow high performance by the file system.

Author:

    David Treadwell (davidtr) 05-Oct-1990

Revision History:

--*/

#include "exp.h"
#pragma hdrstop

#include <stdarg.h>
#include <string.h>
#include <memprint.h>
#undef DbgPrint

//
// Forward declarations.
//

VOID
MemPrintWriteCompleteApc (
    IN PVOID ApcContext,
    IN PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG Reserved
    );

VOID
MemPrintWriteThread (
    IN PVOID Dummy
    );


//
// The maximum message size is the largest message that can be written
// by a single call to MemPrint.

#define MEM_PRINT_MAX_MESSAGE_SIZE 256

//
// These macros aid in determining the size of a subbuffer and the
// subbuffer corresponding to an index into the circular buffer.
//

#define MEM_PRINT_SUBBUFFER_SIZE (MemPrintBufferSize / MemPrintSubbufferCount)

#define GET_MEM_PRINT_SUBBUFFER(i) ((CSHORT)( (i) / MEM_PRINT_SUBBUFFER_SIZE ))

//
// The definition of the header put before each message if the
// MEM_PRINT_FLAG_HEADER bit of MemPrintFlags is turned on.
//

typedef struct _MEM_PRINT_MESSAGE_HEADER {
    USHORT Size;
    USHORT Type;
} MEM_PRINT_MESSAGE_HEADER, *PMEM_PRINT_MESSAGE_HEADER;

//
// Global data.  It is all protected by MemPrintSpinLock.
//

CLONG MemPrintBufferSize = MEM_PRINT_DEF_BUFFER_SIZE;
CLONG MemPrintSubbufferCount = MEM_PRINT_DEF_SUBBUFFER_COUNT;
PCHAR MemPrintBuffer;

ULONG MemPrintFlags = MEM_PRINT_FLAG_CONSOLE;

KSPIN_LOCK MemPrintSpinLock;

CHAR MemPrintTempBuffer[MEM_PRINT_MAX_MESSAGE_SIZE];

BOOLEAN MemPrintInitialized = FALSE;

//
// MemPrintIndex stores the current index into the circular buffer.
//

CLONG MemPrintIndex = 0;

//
// MemPrintCurrentSubbuffer stores the index of the subbuffer currently
// being used to hold data.  It has a range between 0 and
// MemPrintSubbufferCount-1.
//

CLONG MemPrintCurrentSubbuffer = 0;

//
// The MemPrintSubbufferWriting array is used to indicate when a
// subbuffer is being written to disk.  While this occurs, new data
// cannot be written to the subbuffer.
//

BOOLEAN MemPrintSubbufferWriting[MEM_PRINT_MAX_SUBBUFFER_COUNT];

//
// The MemPrintSubbufferFullEvent array is used to communicate between
// threads calling MemPrintMemory and the thread that writes the log
// file.  When a subbuffer is full and ready to be written to disk,
// the corresponding event in this array is signaled, which causes
// the write thread to wake up and perform the write.
//

KEVENT MemPrintSubbufferFullEvent[MEM_PRINT_MAX_SUBBUFFER_COUNT];


VOID
MemPrintInitialize (
    VOID
    )

/*++

Routine Description:

    This is the initialization routine for the in-memory DbgPrint routine.
    It should be called before the first call to MemPrint to set up the
    various structures used and to start the log file write thread.

Arguments:

    None.

Return Value:

    None.

--*/

{
    CLONG i;
    NTSTATUS status;
    HANDLE threadHandle;

    if ( MemPrintInitialized ) {
        return;
    }

    //
    // Allocate memory for the circular buffer that will receive
    // the text and data.  If we can't do it, try again with a buffer
    // half as large.  If that fails, quit trying.
    //

    MemPrintBuffer = ExAllocatePoolWithTag( NonPagedPool, MemPrintBufferSize, 'rPeM' );

    if ( MemPrintBuffer == NULL ) {

        MemPrintBufferSize /= 2;
        DbgPrint( "Unable to allocate DbgPrint buffer--trying size = %ld\n",
                      MemPrintBufferSize );
        MemPrintBuffer = ExAllocatePoolWithTag( NonPagedPool, MemPrintBufferSize, 'rPeM' );

        if ( MemPrintBuffer == NULL ) {
            DbgPrint( "Couldn't allocate DbgPrint buffer.\n" );
            return;
        } else {
            //DbgPrint( "MemPrint buffer from %lx to %lx\n",
            //            MemPrintBuffer, MemPrintBuffer + MemPrintBufferSize );
        }

    } else {
        //DbgPrint( "MemPrint buffer from %lx to %lx\n",
        //              MemPrintBuffer, MemPrintBuffer + MemPrintBufferSize );
    }

    //
    // Allocate the spin lock that protects access to the various
    // pointers and the circular buffer.  This ensures integrity of the
    // buffer.
    //

    KeInitializeSpinLock( &MemPrintSpinLock );

    //
    // Make sure that the subbuffer count is in range.  (We assume that
    // the number is a power of 2.)
    //

    if ( MemPrintSubbufferCount < 2 ) {
        MemPrintSubbufferCount = 2;
    } else if ( MemPrintSubbufferCount > MEM_PRINT_MAX_SUBBUFFER_COUNT ) {
        MemPrintSubbufferCount = MEM_PRINT_MAX_SUBBUFFER_COUNT;
    }

    //
    // Initialize the array of BOOLEANs that determines which subbuffers
    // are being written to disk and therefore cannot be used to store
    // new DbgPrint data.
    //
    // Initialize the array of events that indicates that a subbuffer is
    // ready to be written to disk.
    //

    for ( i = 0; i < MemPrintSubbufferCount; i++ ) {
        MemPrintSubbufferWriting[i] = FALSE;
        KeInitializeEvent(
            &MemPrintSubbufferFullEvent[i],
            SynchronizationEvent,
            FALSE
            );
    }

    //
    // Start the thread that writes subbuffers from the large circular
    // buffer to disk.
    //

    status = PsCreateSystemThread(
                &threadHandle,
                PROCESS_ALL_ACCESS,
                NULL,
                NtCurrentProcess(),
                NULL,
                MemPrintWriteThread,
                NULL
                );

    if ( !NT_SUCCESS(status) ) {
        DbgPrint( "MemPrintInitialize: PsCreateSystemThread failed: %X\n",
                      status );
        return;
    }

    MemPrintInitialized = TRUE;
    ZwClose( threadHandle );

    return;

} // MemPrintInitialize


VOID
MemPrint (
    CHAR *Format, ...
    )

/*++

Routine Description:

    This routine is called in place of DbgPrint to process in-memory
    printing.

Arguments:

    Format - A format string in the style of DbgPrint.

           - formatting arguments.

Return Value:

    None.

--*/

{
    va_list arglist;
    KIRQL oldIrql;
    CLONG nextSubbuffer;
    PMEM_PRINT_MESSAGE_HEADER messageHeader;
    CHAR tempBuffer[MEM_PRINT_MAX_MESSAGE_SIZE];

    va_start(arglist, Format);
    _vsnprintf( tempBuffer, sizeof( tempBuffer ), Format, arglist );
    va_end(arglist);

    //
    // If memory DbgPrint has not been initialized, simply print to the
    // console.
    //

    if ( !MemPrintInitialized ) {

        DbgPrint( "%s", tempBuffer );
        return;
    }

    //
    // Acquire the spin lock that synchronizes access to the pointers
    // and circular buffer.
    //

    KeAcquireSpinLock( &MemPrintSpinLock, &oldIrql );

    //
    // Make sure that the request will fit.  xx_sprintf will just dump
    // all it gets, so assume the message is maximum size, and, if the
    // request would go into the next subbuffer and it is writing, fail
    // the request.
    //

    nextSubbuffer =
        GET_MEM_PRINT_SUBBUFFER( MemPrintIndex + MEM_PRINT_MAX_MESSAGE_SIZE );

    if (  nextSubbuffer != MemPrintCurrentSubbuffer ) {

        //
        // The request will go to a new subbuffer.  Check if we should
        // wrap around to the first subbuffer (i.e. start of circular
        // buffer).
        //

        if ( nextSubbuffer == MemPrintSubbufferCount ) {
            nextSubbuffer = 0;
        }

        //
        // Is that subbuffer available for use?
        //

        if ( MemPrintSubbufferWriting[nextSubbuffer] ) {

            //
            // It is in use.  Print to the console.  Oh well.
            //

            KeReleaseSpinLock( &MemPrintSpinLock, oldIrql );

            DbgPrint( "%s", tempBuffer );

            return;
        }

        //
        // If we went to subbuffer 0 and it is available to receive
        // data, set up the "end of last subbuffer" conditions and reset
        // the index into the circular buffer.  By setting a special
        // type value in the message header that precedes the garbage at
        // the end of the last subbuffer, an interpreter program can
        // know to skip over the garbage by using the size in the
        // header.  This is done instead of writing only good data so
        // that we can write just full sectors to disk, thereby
        // enhancing write performance.
        //

        if ( nextSubbuffer == 0 ) {

            //
            // Set up the message header.  This always gets done at the
            // end of the circular buffer, regardless of the flags bit.
            //

            messageHeader =
                (PMEM_PRINT_MESSAGE_HEADER)&MemPrintBuffer[MemPrintIndex];
            RtlStoreUshort(
                &messageHeader->Size,
                (USHORT)(MemPrintBufferSize - MemPrintIndex - 1)
                );
            RtlStoreUshort(
                &messageHeader->Type,
                (USHORT)0xffff
                );

            //
            // Zero out the rest of the subbuffer.
            //

            for ( MemPrintIndex += sizeof(MEM_PRINT_MESSAGE_HEADER);
                  MemPrintIndex < MemPrintBufferSize;
                  MemPrintIndex++ ) {

                MemPrintBuffer[MemPrintIndex] = 0;
            }

            //
            // Reset the index to start at the beginning of the circular
            // buffer.
            //

            MemPrintIndex = 0;
        }
    }

    //
    // Store a pointer to the location that will contain the message
    // header.
    //

    messageHeader = (PMEM_PRINT_MESSAGE_HEADER)&MemPrintBuffer[MemPrintIndex];

    if ( MemPrintFlags & MEM_PRINT_FLAG_HEADER ) {
        MemPrintIndex += sizeof(MEM_PRINT_MESSAGE_HEADER);
    }

    //
    // Dump the formatted string to the subbuffer.  xx_sprintf is a special
    // version of sprintf that takes a variable argument list.
    //

    ASSERT( MemPrintIndex + MEM_PRINT_MAX_MESSAGE_SIZE -
                sizeof(MEM_PRINT_MESSAGE_HEADER) <= MemPrintBufferSize );


    RtlMoveMemory( &MemPrintBuffer[MemPrintIndex], tempBuffer, strlen(tempBuffer)+1 );

    MemPrintIndex += strlen(tempBuffer);

    //
    // Write the total message size to the message header.
    //

    if ( MemPrintFlags & MEM_PRINT_FLAG_HEADER ) {
        messageHeader->Size =
            (USHORT)( &MemPrintBuffer[MemPrintIndex] - (PCHAR)messageHeader );
        messageHeader->Type = (USHORT)0xdead;
        messageHeader++;
    }

    //
    // If it was too large, there's a potential problem with writing off
    // the end of the circular buffer.  Print the offending message to
    // the console and breakpoint.
    //

    if ( &MemPrintBuffer[MemPrintIndex] - (PCHAR)messageHeader >
                                                MEM_PRINT_MAX_MESSAGE_SIZE ) {
        DbgPrint( "Message too long!! :\n" );
        DbgPrint( "%s", messageHeader );
        DbgBreakPoint( );
    }

    //
    // Print to the console if the appropriate flag is on.
    //

    if ( MemPrintFlags & MEM_PRINT_FLAG_CONSOLE ) {
        DbgPrint( "%s", messageHeader );
    }

    //
    // Calculate whether we have stepped into a new subbuffer.
    //

    nextSubbuffer = GET_MEM_PRINT_SUBBUFFER( MemPrintIndex );

    if ( nextSubbuffer != MemPrintCurrentSubbuffer ) {

        //DbgPrint( "Subbuffer %ld complete.\n", MemPrintCurrentSubbuffer );

        //
        // We did step into a new subbuffer, so set the boolean to
        // indicate that the old subbuffer is writing to disk, thereby
        // preventing it from being overwritten until the write is
        // complete.
        //

        MemPrintSubbufferWriting[MemPrintCurrentSubbuffer] = TRUE;

        //
        // Set the event that will wake up the thread writing subbuffers
        // to disk.
        //

        KeSetEvent(
            &MemPrintSubbufferFullEvent[MemPrintCurrentSubbuffer],
            2,
            FALSE
            );

        //
        // Update the current subbuffer.
        //

        MemPrintCurrentSubbuffer = nextSubbuffer;
    }

    KeReleaseSpinLock( &MemPrintSpinLock, oldIrql );

    return;

} // MemPrint


VOID
MemPrintFlush (
    VOID
    )

/*++

Routine Description:

    This routine causes the current subbuffer to be written to disk,
    regardless of how full it is.  The unwritten part of the subbuffer
    is zeroed before writing.

Arguments:

    None.

Return Value:

    None.

--*/

{
    KIRQL oldIrql;
    PMEM_PRINT_MESSAGE_HEADER messageHeader;
    CLONG nextSubbufferIndex;
    LARGE_INTEGER delayInterval;

    //
    // Acquire the spin lock that protects memory DbgPrint variables.
    //

    KeAcquireSpinLock( &MemPrintSpinLock, &oldIrql );

    DbgPrint( "Flushing subbuffer %ld\n", MemPrintCurrentSubbuffer );

    //
    // Set up the header that indicates that unused space follows.
    //

    messageHeader =
        (PMEM_PRINT_MESSAGE_HEADER)&MemPrintBuffer[MemPrintIndex];
    messageHeader->Size =
        (USHORT)(MemPrintBufferSize - MemPrintIndex - 1);
    messageHeader->Type = (USHORT)0xffff;

    //
    // Determine where the next subbuffer starts.
    //

    nextSubbufferIndex =
        (MemPrintCurrentSubbuffer + 1) * MEM_PRINT_SUBBUFFER_SIZE;

    //
    // Zero out the rest of the subbuffer.
    //

    for ( MemPrintIndex += sizeof(MEM_PRINT_MESSAGE_HEADER);
          MemPrintIndex < nextSubbufferIndex;
          MemPrintIndex++ ) {

        MemPrintBuffer[MemPrintIndex] = 0;
    }

    //
    // Indicate that the subbuffer should be written to disk.
    //

    MemPrintSubbufferWriting[MemPrintCurrentSubbuffer] = TRUE;

    KeSetEvent(
        &MemPrintSubbufferFullEvent[MemPrintCurrentSubbuffer],
        8,
        FALSE
        );

    //
    // Increment the current subbuffer so that it corresponds with the
    // buffer index.
    //

    MemPrintCurrentSubbuffer++;

    KeReleaseSpinLock( &MemPrintSpinLock, oldIrql );

    //
    // Delay so that the memory print write thread wakes up and performs
    // the write to disk.
    //
    // !!! This is obviously not a perfect solution--the write thread
    //     may never wake up, so this could complete before the flush
    //     is really done.
    //

    delayInterval.QuadPart = -10*10*1000*1000;

    DbgPrint( "Delaying...\n" );
    KeDelayExecutionThread( KernelMode, TRUE, &delayInterval );
    DbgPrint( "Woke up.\n" );

    return;

} // MemPrintFlush


VOID
MemPrintWriteThread (
    IN PVOID Dummy
    )

/*++

Routine Description:

    The log file write thread executes this routine.  It sets up the
    log file for writing, then waits for subbuffers to fill, writing
    them to disk when they do.  When the log file fills, new space
    for it is allocated on disk to prevent the file system from
    having to do it.

Arguments:

    Dummy - Ignored.

Return Value:

    None.

--*/

{
    NTSTATUS status;
    IO_STATUS_BLOCK ioStatusBlock[MEM_PRINT_MAX_SUBBUFFER_COUNT];
    IO_STATUS_BLOCK localIoStatusBlock;
    CLONG i;
    KPRIORITY threadPriorityLevel;

    NTSTATUS waitStatus;
    PVOID waitObjects[64];
    KWAIT_BLOCK waitBlockArray[MEM_PRINT_MAX_SUBBUFFER_COUNT];

    OBJECT_ATTRIBUTES objectAttributes;
    PCHAR fileName = MEM_PRINT_LOG_FILE_NAME;
    ANSI_STRING fileNameString;
    HANDLE fileHandle;

    LARGE_INTEGER fileAllocation;
    LARGE_INTEGER fileAllocationIncrement;
    LARGE_INTEGER totalBytesWritten;
    LARGE_INTEGER writeSize;

    LARGE_INTEGER delayInterval;
    ULONG attempts = 0;

    UNICODE_STRING UnicodeFileName;

    Dummy;

    //
    // Initialize the string containing the file name and the object
    // attributes structure that will describe the log file to open.
    //

    RtlInitAnsiString( &fileNameString, fileName );
    status = RtlAnsiStringToUnicodeString(&UnicodeFileName,&fileNameString,TRUE);
    if ( !NT_SUCCESS(status) ) {
        NtTerminateThread( NtCurrentThread(), status );
    }

    InitializeObjectAttributes(
        &objectAttributes,
        &UnicodeFileName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    //
    // Set the allocation size of the log file to be three times the
    // size of the circular buffer.  When it fills up, we'll extend
    // it.
    //

    fileAllocationIncrement.LowPart = MemPrintBufferSize * 8;
    fileAllocationIncrement.HighPart = 0;
    fileAllocation = fileAllocationIncrement;

    //
    // Open the log file.
    //
    // !!! The loop here is to help avoid a system initialization
    //     timing problem, and should be removed when the problem is
    //     fixed.
    //

    while ( TRUE ) {

        status = NtCreateFile(
                     &fileHandle,
                     FILE_WRITE_DATA,
                     &objectAttributes,
                     &localIoStatusBlock,
                     &fileAllocation,
                     FILE_ATTRIBUTE_NORMAL,
                     FILE_SHARE_READ,
                     FILE_OVERWRITE_IF,
                     FILE_SEQUENTIAL_ONLY,
                     NULL,
                     0L
                     );

        if ( (status != STATUS_OBJECT_PATH_NOT_FOUND) || (++attempts >= 3) ) {
            RtlFreeUnicodeString(&UnicodeFileName);
            break;
        }

        delayInterval.QuadPart = -5*10*1000*1000;    // five second delay
        KeDelayExecutionThread( KernelMode, FALSE, &delayInterval );

    }

    if ( !NT_SUCCESS(status) ) {
        DbgPrint( "NtCreateFile for log file failed: %X\n", status );
        NtTerminateThread( NtCurrentThread(), status );
    }

    //
    // Initialize the total bytes written and write size variables.
    //

    totalBytesWritten.LowPart = 0;
    totalBytesWritten.HighPart = 0;
    writeSize.LowPart = MEM_PRINT_SUBBUFFER_SIZE;
    writeSize.HighPart = 0;

    //
    // Set up the wait objects array for a call to KeWaitForMultipleObjects.
    //

    for ( i = 0; i < MemPrintSubbufferCount; i++ ) {
        waitObjects[i] = &MemPrintSubbufferFullEvent[i];
    }

    //
    // Set the priority of the write thread.
    //

    threadPriorityLevel = LOW_REALTIME_PRIORITY + 1;

    status = NtSetInformationThread(
                NtCurrentThread(),
                ThreadPriority,
                &threadPriorityLevel,
                sizeof(threadPriorityLevel)
                );

    if ( !NT_SUCCESS(status) ) {
        DbgPrint( "Unable to set error log thread priority: %X\n", status );
    }

    //
    // Loop waiting for one of the subbuffer full events to be signaled.
    // When one is signaled, wake up and write the subbuffer to the log
    // file.
    //

    while ( TRUE ) {

        waitStatus = KeWaitForMultipleObjects(
                         (CCHAR)MemPrintSubbufferCount,
                         waitObjects,
                         WaitAny,
                         Executive,
                         KernelMode,
                         TRUE,
                         NULL,
                         waitBlockArray
                         );

        if ( !NT_SUCCESS(waitStatus) ) {
            DbgPrint( "KeWaitForMultipleObjects failed: %X\n", waitStatus );
            NtTerminateThread( NtCurrentThread(), waitStatus );
        } //else {
            //DbgPrint( "Writing subbuffer %ld...\n", waitStatus );
        //}

        ASSERT( (CCHAR)waitStatus < (CCHAR)MemPrintSubbufferCount );

        //
        // Check the DbgPrint flags to see if we really want to write
        // this.
        //

        if ( (MemPrintFlags & MEM_PRINT_FLAG_FILE) == 0 ) {

            KIRQL oldIrql;

            KeAcquireSpinLock( &MemPrintSpinLock, &oldIrql );
            MemPrintSubbufferWriting[ waitStatus ] = FALSE;
            KeReleaseSpinLock( &MemPrintSpinLock, oldIrql );

            continue;
        }

        //
        // Start the write operation.  The APC routine will handle
        // checking the return status from the write and resetting
        // the MemPrintSubbufferWriting boolean.
        //

        status = NtWriteFile(
                     fileHandle,
                     NULL,
                     MemPrintWriteCompleteApc,
                     (PVOID)waitStatus,
                     &ioStatusBlock[waitStatus],
                     &MemPrintBuffer[waitStatus * MEM_PRINT_SUBBUFFER_SIZE],
                     MEM_PRINT_SUBBUFFER_SIZE,
                     &totalBytesWritten,
                     NULL
                     );

        if ( !NT_SUCCESS(status) ) {
            DbgPrint( "NtWriteFile for log file failed: %X\n", status );
        }

        //
        // Update the count of bytes written to the log file.
        //

        totalBytesWritten.QuadPart = totalBytesWritten.QuadPart + writeSize.QuadPart;

        //
        // Extend the file if we have reached the end of what we have
        // thus far allocated for the file.  This increases performance
        // by extending the file here rather than in the file system,
        // which would have to extend it each time a write past end of
        // file comes in.
        //

        if ( totalBytesWritten.QuadPart >= fileAllocation.QuadPart ) {

            fileAllocation.QuadPart =
                        fileAllocation.QuadPart + fileAllocationIncrement.QuadPart;

            DbgPrint( "Enlarging log file to %ld bytes.\n",
                          fileAllocation.LowPart );

            status = NtSetInformationFile(
                         fileHandle,
                         &localIoStatusBlock,
                         &fileAllocation,
                         sizeof(fileAllocation),
                         FileAllocationInformation
                         );

            if ( !NT_SUCCESS(status) ) {
                DbgPrint( "Attempt to extend log file failed: %X\n", status );
                fileAllocation.QuadPart =
                        fileAllocation.QuadPart - fileAllocationIncrement.QuadPart;
            }
        }
    }

    return;

} // MemPrintWriteThread


VOID
MemPrintWriteCompleteApc (
    IN PVOID ApcContext,
    IN PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG Reserved
    )

/*++

Routine Description:

    This APC routine is called when subbuffer writes to disk complete.
    It checks for success, printing a message if the write failed.
    It also sets the appropriate MemPrintSubbufferWriting location to
    FALSE so that the subbuffer can be reused.

Arguments:

    ApcContext - contains the index of the subbuffer just written.

    IoStatusBlock - the status block for the operation.

    Reserved - not used; reserved for future use.

Return Value:

    None.

--*/

{
    KIRQL oldIrql;

    if ( !NT_SUCCESS(IoStatusBlock->Status) ) {
        DbgPrint( "NtWriteFile for subbuffer %ld failed: %X\n",
                      ApcContext, IoStatusBlock->Status );
        return;
    }

    //DbgPrint( "Write complete for subbuffer %ld.\n", ApcContext );
    DbgPrint( "." );

    //
    // Acquire the spin lock that protects memory print global variables
    // and set the subbuffer writing boolean to FALSE so that other
    // threads can write to the subbuffer if necessary.
    //

    KeAcquireSpinLock( &MemPrintSpinLock, &oldIrql );
    MemPrintSubbufferWriting[ (ULONG_PTR)ApcContext ] = FALSE;
    KeReleaseSpinLock( &MemPrintSpinLock, oldIrql );

    return;

    Reserved;

} // MemPrintWriteCompleteApc
