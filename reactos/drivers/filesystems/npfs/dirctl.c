/*
 * COPYRIGHT:  See COPYING in the top level directory
 * PROJECT:    ReactOS kernel
 * FILE:       drivers/filesystems/npfs/dirctl.c
 * PURPOSE:    Named pipe filesystem
 * PROGRAMMER: Eric Kohl
 */

/* INCLUDES ******************************************************************/

#include "npfs.h"

#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

static NTSTATUS
NpfsQueryDirectory(PNPFS_CCB Ccb,
                   PIRP Irp,
                   PULONG Size)
{
    PIO_STACK_LOCATION Stack;
    ULONG BufferLength = 0;
    PUNICODE_STRING SearchPattern = NULL;
    FILE_INFORMATION_CLASS FileInformationClass;
    ULONG FileIndex = 0;
    PUCHAR Buffer = NULL;
    BOOLEAN First = FALSE;
    PLIST_ENTRY CurrentEntry;
    PNPFS_VCB Vcb;
    PNPFS_FCB PipeFcb;
    ULONG PipeIndex;
    NTSTATUS Status = STATUS_SUCCESS;
    PFILE_NAMES_INFORMATION NamesBuffer;
    PFILE_DIRECTORY_INFORMATION DirectoryBuffer;
    PFILE_FULL_DIR_INFORMATION FullDirBuffer;
    PFILE_BOTH_DIR_INFORMATION BothDirBuffer;
    ULONG InfoSize = 0;
    ULONG NameLength;
    ULONG CurrentOffset = 0;
    ULONG LastOffset = 0;
    PULONG NextEntryOffset;

    Stack = IoGetCurrentIrpStackLocation(Irp);

    /* Obtain the callers parameters */
    BufferLength = Stack->Parameters.QueryDirectory.Length;
    SearchPattern = Stack->Parameters.QueryDirectory.FileName;
    FileInformationClass = Stack->Parameters.QueryDirectory.FileInformationClass;
    FileIndex = Stack->Parameters.QueryDirectory.FileIndex;

    DPRINT("SearchPattern: %p  '%wZ'\n", SearchPattern, SearchPattern);

    /* Determine Buffer for result */
    if (Irp->MdlAddress)
    {
        Buffer = MmGetSystemAddressForMdlSafe(Irp->MdlAddress,
                                              NormalPagePriority);
    }
    else
    {
        Buffer = Irp->UserBuffer;
    }

    /* Build the search pattern string */
    DPRINT("Ccb->u.Directory.SearchPattern.Buffer: %p\n", Ccb->u.Directory.SearchPattern.Buffer);
    if (Ccb->u.Directory.SearchPattern.Buffer == NULL)
    {
        First = TRUE;

        if (SearchPattern != NULL)
        {
            Ccb->u.Directory.SearchPattern.Buffer =
                ExAllocatePoolWithTag(NonPagedPool,
                                      SearchPattern->Length + sizeof(WCHAR),
                                      TAG_NPFS_NAMEBLOCK);
            if (Ccb->u.Directory.SearchPattern.Buffer == NULL)
            {
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            Ccb->u.Directory.SearchPattern.Length = SearchPattern->Length;
            Ccb->u.Directory.SearchPattern.MaximumLength = SearchPattern->Length + sizeof(WCHAR);
            RtlCopyMemory(Ccb->u.Directory.SearchPattern.Buffer,
                          SearchPattern->Buffer,
                          SearchPattern->Length);
            Ccb->u.Directory.SearchPattern.Buffer[SearchPattern->Length / sizeof(WCHAR)] = 0;
        }
        else
        {
            Ccb->u.Directory.SearchPattern.Buffer =
                ExAllocatePoolWithTag(NonPagedPool,
                                      2 * sizeof(WCHAR),
                                      TAG_NPFS_NAMEBLOCK);
            if (Ccb->u.Directory.SearchPattern.Buffer == NULL)
            {
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            Ccb->u.Directory.SearchPattern.Length = sizeof(WCHAR);
            Ccb->u.Directory.SearchPattern.MaximumLength = 2 * sizeof(WCHAR);
            Ccb->u.Directory.SearchPattern.Buffer[0] = L'*';
            Ccb->u.Directory.SearchPattern.Buffer[1] = 0;
        }
    }
    DPRINT("Search pattern: '%wZ'\n", &Ccb->u.Directory.SearchPattern);

    /* Determine the file index */
    if (First || (Stack->Flags & SL_RESTART_SCAN))
    {
        FileIndex = 0;
    }
    else if ((Stack->Flags & SL_INDEX_SPECIFIED) == 0)
    {
        FileIndex = Ccb->u.Directory.FileIndex + 1;
    }
    DPRINT("FileIndex: %lu\n", FileIndex);

    DPRINT("Buffer = %p  tofind = %wZ\n", Buffer, &Ccb->u.Directory.SearchPattern);

    switch (FileInformationClass)
    {
        case FileDirectoryInformation:
            InfoSize = sizeof(FILE_DIRECTORY_INFORMATION) - sizeof(WCHAR);
            break;

        case FileFullDirectoryInformation:
            InfoSize = sizeof(FILE_FULL_DIR_INFORMATION) - sizeof(WCHAR);
            break;

        case FileBothDirectoryInformation:
            InfoSize = sizeof(FILE_BOTH_DIR_INFORMATION) - sizeof(WCHAR);
            break;

        case FileNamesInformation:
            InfoSize = sizeof(FILE_NAMES_INFORMATION) - sizeof(WCHAR);
            break;

        default:
            DPRINT1("Invalid information class: %lu\n", FileInformationClass);
            return STATUS_INVALID_INFO_CLASS;
    }

    PipeIndex = 0;

    Vcb = Ccb->Fcb->Vcb;
    KeLockMutex(&Vcb->PipeListLock);
    CurrentEntry = Vcb->PipeListHead.Flink;
    while (CurrentEntry != &Vcb->PipeListHead &&
           Status == STATUS_SUCCESS)
    {
        /* Get the FCB of the next pipe */
        PipeFcb = CONTAINING_RECORD(CurrentEntry,
                                    NPFS_FCB,
                                    PipeListEntry);

        /* Make sure it is a pipe FCB */
        ASSERT(PipeFcb->Type == FCB_PIPE);

        DPRINT("PipeName: %wZ\n", &PipeFcb->PipeName);

        if (FsRtlIsNameInExpression(&Ccb->u.Directory.SearchPattern,
                                    &PipeFcb->PipeName,
                                    TRUE,
                                    NULL))
        {
            DPRINT("Found pipe: %wZ\n", &PipeFcb->PipeName);

            if (PipeIndex >= FileIndex)
            {
                /* Determine whether or not the full pipe name fits into the buffer */
                if (InfoSize + PipeFcb->PipeName.Length > BufferLength)
                {
                    NameLength = BufferLength - InfoSize;
                    Status = STATUS_BUFFER_OVERFLOW;
                }
                else
                {
                    NameLength = PipeFcb->PipeName.Length;
                    Status = STATUS_SUCCESS;
                }

                /* Initialize the information struct */
                RtlZeroMemory(&Buffer[CurrentOffset], InfoSize);

                switch (FileInformationClass)
                {
                    case FileDirectoryInformation:
                        DirectoryBuffer = (PFILE_DIRECTORY_INFORMATION)&Buffer[CurrentOffset];
                        DirectoryBuffer->FileIndex = PipeIndex;
                        DirectoryBuffer->FileAttributes = FILE_ATTRIBUTE_NORMAL;
                        DirectoryBuffer->EndOfFile.QuadPart = PipeFcb->CurrentInstances;
                        DirectoryBuffer->AllocationSize.LowPart = PipeFcb->MaximumInstances;
                        DirectoryBuffer->FileNameLength = NameLength;
                        RtlCopyMemory(DirectoryBuffer->FileName,
                                      PipeFcb->PipeName.Buffer,
                                      NameLength);
                        break;

                    case FileFullDirectoryInformation:
                        FullDirBuffer = (PFILE_FULL_DIR_INFORMATION)&Buffer[CurrentOffset];
                        FullDirBuffer->FileIndex = PipeIndex;
                        FullDirBuffer->FileAttributes = FILE_ATTRIBUTE_NORMAL;
                        FullDirBuffer->EndOfFile.QuadPart = PipeFcb->CurrentInstances;
                        FullDirBuffer->AllocationSize.LowPart = PipeFcb->MaximumInstances;
                        FullDirBuffer->FileNameLength = NameLength;
                        RtlCopyMemory(FullDirBuffer->FileName,
                                      PipeFcb->PipeName.Buffer,
                                      NameLength);
                        break;

                    case FileBothDirectoryInformation:
                        BothDirBuffer = (PFILE_BOTH_DIR_INFORMATION)&Buffer[CurrentOffset];
                        BothDirBuffer->NextEntryOffset = 0;
                        BothDirBuffer->FileIndex = PipeIndex;
                        BothDirBuffer->FileAttributes = FILE_ATTRIBUTE_NORMAL;
                        BothDirBuffer->EndOfFile.QuadPart = PipeFcb->CurrentInstances;
                        BothDirBuffer->AllocationSize.LowPart = PipeFcb->MaximumInstances;
                        BothDirBuffer->FileNameLength = NameLength;
                        RtlCopyMemory(BothDirBuffer->FileName,
                                      PipeFcb->PipeName.Buffer,
                                      NameLength);
                        break;

                    case FileNamesInformation:
                        NamesBuffer = (PFILE_NAMES_INFORMATION)&Buffer[CurrentOffset];
                        NamesBuffer->FileIndex = PipeIndex;
                        NamesBuffer->FileNameLength = NameLength;
                        RtlCopyMemory(NamesBuffer->FileName,
                                      PipeFcb->PipeName.Buffer,
                                      NameLength);
                        break;

                    default:
                        /* Should never happen! */
                        ASSERT(FALSE);
                        break;
                }

                DPRINT("CurrentOffset: %lu\n", CurrentOffset);

                /* Store the current pipe index in the CCB */
                Ccb->u.Directory.FileIndex = PipeIndex;

                /* Get the pointer to the previous entries NextEntryOffset */
                NextEntryOffset = (PULONG)&Buffer[LastOffset];

                /* Set the previous entries NextEntryOffset */
                *NextEntryOffset = CurrentOffset - LastOffset;

                /* Return the used buffer size */
                *Size = CurrentOffset + InfoSize + NameLength;

                /* Leave, if there is no space left in the buffer */
                if (Status == STATUS_BUFFER_OVERFLOW)
                {
                    KeUnlockMutex(&Vcb->PipeListLock);
                    return Status;
                }

                /* Leave, if we should return only one entry */
                if (Stack->Flags & SL_RETURN_SINGLE_ENTRY)
                {
                    KeUnlockMutex(&Vcb->PipeListLock);
                    return STATUS_SUCCESS;
                }

                /* Store the current offset for the next round */
                LastOffset = CurrentOffset;

                /* Set the offset for the next entry */
                CurrentOffset += ROUND_UP(InfoSize + NameLength, sizeof(ULONG));
            }

            PipeIndex++;
        }

        CurrentEntry = CurrentEntry->Flink;
    }
    KeUnlockMutex(&Vcb->PipeListLock);

    /* Return STATUS_NO_MORE_FILES if no matching pipe name was found */
    if (CurrentOffset == 0)
        Status = STATUS_NO_MORE_FILES;

    return Status;
}


NTSTATUS NTAPI
NpfsDirectoryControl(PDEVICE_OBJECT DeviceObject,
                     PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    PFILE_OBJECT FileObject;
    PNPFS_CCB Ccb;
    //PNPFS_FCB Fcb;
    NTSTATUS Status;
    ULONG Size = 0;

    UNREFERENCED_PARAMETER(DeviceObject);

    DPRINT("NpfsDirectoryControl() called\n");

    IoStack = IoGetCurrentIrpStackLocation(Irp);

    FileObject = IoStack->FileObject;

    if (NpfsGetCcb(FileObject, &Ccb) != CCB_DIRECTORY)
    {
        Status = STATUS_INVALID_PARAMETER;

        Irp->IoStatus.Status = Status;
        Irp->IoStatus.Information = 0;

        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        return Status;
    }

    //Fcb = Ccb->Fcb;

    switch (IoStack->MinorFunction)
    {
        case IRP_MN_QUERY_DIRECTORY:
            Status = NpfsQueryDirectory(Ccb,
                                        Irp,
                                        &Size);
            break;

        case IRP_MN_NOTIFY_CHANGE_DIRECTORY:
            DPRINT1("IRP_MN_NOTIFY_CHANGE_DIRECTORY\n");
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        default:
            DPRINT1("NPFS: MinorFunction %d\n", IoStack->MinorFunction);
            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;
    }

    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = Size;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;
}

/* EOF */
