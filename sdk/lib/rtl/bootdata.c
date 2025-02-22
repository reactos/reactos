/*
 * PROJECT:         ReactOS Runtime Library
 * LICENSE:         See COPYING in the top level directory
 * FILE:            lib/rtl/bootdata.c
 * PURPOSE:         Boot Status Data Implementation
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 *                  Eric Kohl
 */

/* INCLUDES *****************************************************************/

#include <rtl.h>
#define NDEBUG
#include <debug.h>

typedef struct _RTL_BSD_ITEM_TABLE_ENTRY
{
    UCHAR Offset;
    UCHAR Size;
} RTL_BSD_ITEM_TABLE_ENTRY;

/* FUNCTIONS *****************************************************************/

PRTL_BSD_DATA DummyBsd;
RTL_BSD_ITEM_TABLE_ENTRY BsdItemTable[RtlBsdItemMax] =
{
    {
        FIELD_OFFSET(RTL_BSD_DATA, Version),
        sizeof(&DummyBsd->Version)
    },  // RtlBsdItemVersionNumber
    {
        FIELD_OFFSET(RTL_BSD_DATA, ProductType),
        sizeof(&DummyBsd->ProductType)
    },  // RtlBsdItemProductType
    {
        FIELD_OFFSET(RTL_BSD_DATA, AabEnabled),
        sizeof(&DummyBsd->AabEnabled)
    },  // RtlBsdItemAabEnabled
    {
        FIELD_OFFSET(RTL_BSD_DATA, AabTimeout),
        sizeof(&DummyBsd->AabTimeout)
    },  // RtlBsdItemAabTimeout
    {
        FIELD_OFFSET(RTL_BSD_DATA, LastBootSucceeded),
        sizeof(&DummyBsd->LastBootSucceeded)
    },  // RtlBsdItemBootGood
    {
        FIELD_OFFSET(RTL_BSD_DATA, LastBootShutdown),
        sizeof(&DummyBsd->LastBootShutdown)
    },  // RtlBsdItemBootShutdown
    {
        FIELD_OFFSET(RTL_BSD_DATA, SleepInProgress),
        sizeof(&DummyBsd->SleepInProgress)
    },  // RtlBsdSleepInProgress
    {
        FIELD_OFFSET(RTL_BSD_DATA, PowerTransition),
        sizeof(&DummyBsd->PowerTransition)
    },  // RtlBsdPowerTransition
    {
        FIELD_OFFSET(RTL_BSD_DATA, BootAttemptCount),
        sizeof(&DummyBsd->BootAttemptCount)
    },  // RtlBsdItemBootAttemptCount
    {
        FIELD_OFFSET(RTL_BSD_DATA, LastBootCheckpoint),
        sizeof(&DummyBsd->LastBootCheckpoint)
    },  // RtlBsdItemBootCheckpoint
    {
        FIELD_OFFSET(RTL_BSD_DATA, LastBootId),
        sizeof(&DummyBsd->LastBootId)
    },  // RtlBsdItemBootId
    {
        FIELD_OFFSET(RTL_BSD_DATA, LastSuccessfulShutdownBootId),
        sizeof(&DummyBsd->LastSuccessfulShutdownBootId)
    },  // RtlBsdItemShutdownBootId
    {
        FIELD_OFFSET(RTL_BSD_DATA, LastReportedAbnormalShutdownBootId),
        sizeof(&DummyBsd->LastReportedAbnormalShutdownBootId)
    },  // RtlBsdItemReportedAbnormalShutdownBootId
    {
        FIELD_OFFSET(RTL_BSD_DATA, ErrorInfo),
        sizeof(&DummyBsd->ErrorInfo)
    },  // RtlBsdItemErrorInfo
    {
        FIELD_OFFSET(RTL_BSD_DATA, PowerButtonPressInfo),
        sizeof(&DummyBsd->PowerButtonPressInfo)
    },  // RtlBsdItemPowerButtonPressInfo
    {
        FIELD_OFFSET(RTL_BSD_DATA, Checksum),
        sizeof(&DummyBsd->Checksum)
    },  // RtlBsdItemChecksum
};

/*
 * @implemented
 */
NTSTATUS
NTAPI
RtlCreateBootStatusDataFile (
    VOID
    )
{
    IO_STATUS_BLOCK IoStatusBlock;
    LARGE_INTEGER AllocationSize;
    LARGE_INTEGER ByteOffset;
    UNICODE_STRING FileName =
        RTL_CONSTANT_STRING(L"\\SystemRoot\\bootstat.dat");
    OBJECT_ATTRIBUTES ObjectAttributes =
        RTL_CONSTANT_OBJECT_ATTRIBUTES(&FileName, OBJ_CASE_INSENSITIVE);
    HANDLE FileHandle;
    NTSTATUS Status;
    RTL_BSD_DATA InitialBsd;

    /* Create the boot status data file */
    AllocationSize.QuadPart = 0x800;
    DBG_UNREFERENCED_LOCAL_VARIABLE(AllocationSize);
    Status = ZwCreateFile(&FileHandle,
                          FILE_GENERIC_READ | FILE_GENERIC_WRITE,
                          &ObjectAttributes,
                          &IoStatusBlock,
                          NULL, //&AllocationSize,
                          FILE_ATTRIBUTE_SYSTEM,
                          0,
                          FILE_CREATE,
                          FILE_SYNCHRONOUS_IO_NONALERT,
                          NULL,
                          0);
    if (NT_SUCCESS(Status))
    {
        /* Setup a sane looking initial BSD */
        RtlZeroMemory(&InitialBsd, sizeof(InitialBsd));
        InitialBsd.Version = sizeof(InitialBsd);
        InitialBsd.ProductType = NtProductWinNt;
        InitialBsd.AabEnabled = 1;
        InitialBsd.AabTimeout = 30;
        InitialBsd.LastBootSucceeded = TRUE;

        /* Write it to disk */
        ByteOffset.QuadPart = 0;
        Status = ZwWriteFile(FileHandle,
                             NULL,
                             NULL,
                             NULL,
                             &IoStatusBlock,
                             &InitialBsd,
                             sizeof(InitialBsd),
                             &ByteOffset,
                             NULL);
    }

    /* Close the file */
    ZwClose(FileHandle);

    return Status;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
RtlGetSetBootStatusData (
    _In_ HANDLE FileHandle,
    _In_ BOOLEAN Read,
    _In_ RTL_BSD_ITEM_TYPE DataClass,
    _In_ PVOID Buffer,
    _In_ ULONG BufferSize,
    _Out_opt_ PULONG ReturnLength
    )
{
    IO_STATUS_BLOCK IoStatusBlock;
    LARGE_INTEGER ByteOffset;
    NTSTATUS Status;

    DPRINT("RtlGetSetBootStatusData (%p %u %d %p %lu %p)\n",
           FileHandle, Read, DataClass, Buffer, BufferSize, ReturnLength);

    if (DataClass >= RtlBsdItemMax)
    {
        return STATUS_INVALID_PARAMETER;
    }

    if (BufferSize > BsdItemTable[DataClass].Size)
    {
        return STATUS_BUFFER_TOO_SMALL;
    }

    ByteOffset.HighPart = 0;
    ByteOffset.LowPart = BsdItemTable[DataClass].Offset;

    if (Read)
    {
        Status = ZwReadFile(FileHandle,
                            NULL,
                            NULL,
                            NULL,
                            &IoStatusBlock,
                            Buffer,
                            BufferSize,
                            &ByteOffset,
                            NULL);
    }
    else
    {
        Status = ZwWriteFile(FileHandle,
                             NULL,
                             NULL,
                             NULL,
                             &IoStatusBlock,
                             Buffer,
                             BufferSize,
                             &ByteOffset,
                             NULL);
    }

    if (NT_SUCCESS(Status))
    {
        if (ReturnLength)
        {
            *ReturnLength = BsdItemTable[DataClass].Size;
        }
    }

    return Status;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
RtlLockBootStatusData (
    _Out_ PHANDLE FileHandle
    )
{
    UNICODE_STRING FileName =
        RTL_CONSTANT_STRING(L"\\SystemRoot\\bootstat.dat");
    OBJECT_ATTRIBUTES ObjectAttributes =
        RTL_CONSTANT_OBJECT_ATTRIBUTES(&FileName, OBJ_CASE_INSENSITIVE);
    HANDLE LocalFileHandle;
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;

    /* Intialize the file handle */
    *FileHandle = NULL;

    /* Open the boot status data file */
    Status = ZwOpenFile(&LocalFileHandle,
                        FILE_ALL_ACCESS,
                        &ObjectAttributes,
                        &IoStatusBlock,
                        0,
                        FILE_SYNCHRONOUS_IO_NONALERT);
    if (NT_SUCCESS(Status))
    {
        /* Return the file handle */
        *FileHandle = LocalFileHandle;
    }

    return Status;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
RtlUnlockBootStatusData (
    _In_ HANDLE FileHandle
    )
{
    IO_STATUS_BLOCK IoStatusBlock;

    /* Flush the file and close it */
    ZwFlushBuffersFile(FileHandle, &IoStatusBlock);
    return ZwClose(FileHandle);
}

/* EOF */
