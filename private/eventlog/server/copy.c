/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    copy.c

Abstract:

    This module contains the routine to copy a file.

Author:

    Dan Hinsley (DanHi) 24-Feb-1991

Revision History:

    02-Feb-1994     Danl
        Fixed memory leak where ioBuffer wasn't getting free'd when doing
        an error exit from ElfpCopyFile.

--*/

//
// INCLUDES
//

#include <eventp.h>


NTSTATUS
ElfpCopyFile (
    IN HANDLE SourceHandle,
    IN PUNICODE_STRING TargetFileName
    )

/*++

Routine Description:

    This routine copies or appends from the source file to the target file.
    If the target file already exists, the copy fails.

Arguments:

    SourceHandle - An open handle to the source file.

    TargetFileName - The name of the file to copy to.


Return Value:

    NTSTATUS - STATUS_SUCCESS or error.

--*/

{

    NTSTATUS Status;

    IO_STATUS_BLOCK IoStatusBlock;
    FILE_STANDARD_INFORMATION sourceStandardInfo;

    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE TargetHandle;

    PCHAR ioBuffer;
    ULONG ioBufferSize;
    ULONG bytesRead;

    //
    // Get the size of the file so we can set the attributes of the target
    // file.
    //

    Status = NtQueryInformationFile(
                 SourceHandle,
                 &IoStatusBlock,
                 &sourceStandardInfo,
                 sizeof(sourceStandardInfo),
                 FileStandardInformation
                 );

    if ( !NT_SUCCESS(Status) ) {

        return(Status);
    }

    //
    // Open the target file, fail if the file already exists.
    //

    InitializeObjectAttributes(
                    &ObjectAttributes,
                    TargetFileName,
                    OBJ_CASE_INSENSITIVE,
                    NULL,
                    NULL
                    );

    Status = NtCreateFile(&TargetHandle,
                        GENERIC_WRITE | SYNCHRONIZE,
                        &ObjectAttributes,
                        &IoStatusBlock,
                        &(sourceStandardInfo.EndOfFile),
                        FILE_ATTRIBUTE_NORMAL,
                        0,                       // Share access
                        FILE_CREATE,
                        FILE_SYNCHRONOUS_IO_ALERT | FILE_SEQUENTIAL_ONLY,
                        NULL,                    // EA buffer
                        0                        // EA length
                        );

    if (!NT_SUCCESS(Status)) {
        return(Status);
    }

    //
    // Allocate a buffer to use for the data copy.
    //

    ioBufferSize = 4096;

    ioBuffer = ElfpAllocateBuffer ( ioBufferSize);

    if ( ioBuffer == NULL ) {

        return (STATUS_NO_MEMORY);
    }

    //
    // Copy data--read from source, write to target.  Do this until
    // all the data is written or an error occurs.
    //

    while ( TRUE ) {

        Status = NtReadFile(
                         SourceHandle,
                         NULL,                // Event
                         NULL,                // ApcRoutine
                         NULL,                // ApcContext
                         &IoStatusBlock,
                         ioBuffer,
                         ioBufferSize,
                         NULL,                // ByteOffset
                         NULL                 // Key
                         );

        if ( !NT_SUCCESS(Status) && Status != STATUS_END_OF_FILE ) {

            ElfDbgPrint(("[ELF] Copy failed reading source file - %X\n",
                Status));
            ElfpFreeBuffer(ioBuffer);
            return (Status);

        }

        if ( IoStatusBlock.Information == 0 ||
             Status == STATUS_END_OF_FILE ) {
            break;
        }

        bytesRead = (ULONG)IoStatusBlock.Information;

        Status = NtWriteFile(
                          TargetHandle,
                          NULL,               // Event
                          NULL,               // ApcRoutine
                          NULL,               // ApcContext
                          &IoStatusBlock,
                          ioBuffer,
                          bytesRead,
                          NULL,               // ByteOffset
                          NULL                // Key
                          );

        if ( !NT_SUCCESS(Status) ) {
            ElfDbgPrint(("[ELF] Copy failed writing target file - %X\n",
                Status));
            ElfpFreeBuffer(ioBuffer);
            return (Status);
        }
    }

    ElfpFreeBuffer ( ioBuffer );

    Status = NtClose(TargetHandle);

    ASSERT(NT_SUCCESS(Status));

    return STATUS_SUCCESS;

} // ElfpCopyFile

