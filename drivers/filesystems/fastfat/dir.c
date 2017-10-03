/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             drivers/fs/vfat/dir.c
 * PURPOSE:          VFAT Filesystem : directory control
 * UPDATE HISTORY:
     19-12-1998 : created

*/

/* INCLUDES *****************************************************************/

#include "vfat.h"

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ****************************************************************/

/* Function like DosDateTimeToFileTime */
BOOLEAN
FsdDosDateTimeToSystemTime(
    PDEVICE_EXTENSION DeviceExt,
    USHORT DosDate,
    USHORT DosTime,
    PLARGE_INTEGER SystemTime)
{
    PDOSTIME pdtime = (PDOSTIME)&DosTime;
    PDOSDATE pddate = (PDOSDATE)&DosDate;
    TIME_FIELDS TimeFields;
    LARGE_INTEGER LocalTime;

    if (SystemTime == NULL)
        return FALSE;

    TimeFields.Milliseconds = 0;
    TimeFields.Second = pdtime->Second * 2;
    TimeFields.Minute = pdtime->Minute;
    TimeFields.Hour = pdtime->Hour;

    TimeFields.Day = pddate->Day;
    TimeFields.Month = pddate->Month;
    TimeFields.Year = (CSHORT)(DeviceExt->BaseDateYear + pddate->Year);

    RtlTimeFieldsToTime(&TimeFields, &LocalTime);
    ExLocalTimeToSystemTime(&LocalTime, SystemTime);

    return TRUE;
}

/* Function like FileTimeToDosDateTime */
BOOLEAN
FsdSystemTimeToDosDateTime(
    PDEVICE_EXTENSION DeviceExt,
    PLARGE_INTEGER SystemTime,
    PUSHORT pDosDate,
    PUSHORT pDosTime)
{
    PDOSTIME pdtime = (PDOSTIME)pDosTime;
    PDOSDATE pddate = (PDOSDATE)pDosDate;
    TIME_FIELDS TimeFields;
    LARGE_INTEGER LocalTime;

    if (SystemTime == NULL)
        return FALSE;

    ExSystemTimeToLocalTime(SystemTime, &LocalTime);
    RtlTimeToTimeFields(&LocalTime, &TimeFields);

    if (pdtime)
    {
        pdtime->Second = TimeFields.Second / 2;
        pdtime->Minute = TimeFields.Minute;
        pdtime->Hour = TimeFields.Hour;
    }

    if (pddate)
    {
        pddate->Day = TimeFields.Day;
        pddate->Month = TimeFields.Month;
        pddate->Year = (USHORT) (TimeFields.Year - DeviceExt->BaseDateYear);
    }

    return TRUE;
}

#define ULONG_ROUND_UP(x)   ROUND_UP((x), (sizeof(ULONG)))

static
NTSTATUS
VfatGetFileNameInformation(
    PVFAT_DIRENTRY_CONTEXT DirContext,
    PFILE_NAMES_INFORMATION pInfo,
    ULONG BufferLength)
{
    if ((sizeof(FILE_DIRECTORY_INFORMATION) + DirContext->LongNameU.Length) > BufferLength)
        return STATUS_BUFFER_OVERFLOW;

    pInfo->FileNameLength = DirContext->LongNameU.Length;
    pInfo->NextEntryOffset = ULONG_ROUND_UP(sizeof(FILE_DIRECTORY_INFORMATION) +
                                            DirContext->LongNameU.Length);

    RtlCopyMemory(pInfo->FileName,
                  DirContext->LongNameU.Buffer,
                  DirContext->LongNameU.Length);

    return STATUS_SUCCESS;
}

static
NTSTATUS
VfatGetFileDirectoryInformation(
    PVFAT_DIRENTRY_CONTEXT DirContext,
    PDEVICE_EXTENSION DeviceExt,
    PFILE_DIRECTORY_INFORMATION pInfo,
    ULONG BufferLength)
{
    if ((sizeof(FILE_DIRECTORY_INFORMATION) + DirContext->LongNameU.Length) > BufferLength)
        return STATUS_BUFFER_OVERFLOW;

    pInfo->FileNameLength = DirContext->LongNameU.Length;
    pInfo->NextEntryOffset = ULONG_ROUND_UP(sizeof(FILE_DIRECTORY_INFORMATION) +
                                            DirContext->LongNameU.Length);
    /* pInfo->FileIndex = ; */

    RtlCopyMemory(pInfo->FileName,
                  DirContext->LongNameU.Buffer,
                  DirContext->LongNameU.Length);

    if (vfatVolumeIsFatX(DeviceExt))
    {
        FsdDosDateTimeToSystemTime(DeviceExt,
                                   DirContext->DirEntry.FatX.CreationDate,
                                   DirContext->DirEntry.FatX.CreationTime,
                                   &pInfo->CreationTime);
        FsdDosDateTimeToSystemTime(DeviceExt,
                                   DirContext->DirEntry.FatX.AccessDate,
                                   DirContext->DirEntry.FatX.AccessTime,
                                   &pInfo->LastAccessTime);
        FsdDosDateTimeToSystemTime(DeviceExt,
                                   DirContext->DirEntry.FatX.UpdateDate,
                                   DirContext->DirEntry.FatX.UpdateTime,
                                   &pInfo->LastWriteTime);

        pInfo->ChangeTime = pInfo->LastWriteTime;

        if (BooleanFlagOn(DirContext->DirEntry.FatX.Attrib, FILE_ATTRIBUTE_DIRECTORY))
        {
            pInfo->EndOfFile.QuadPart = 0;
            pInfo->AllocationSize.QuadPart = 0;
        }
        else
        {
            pInfo->EndOfFile.u.HighPart = 0;
            pInfo->EndOfFile.u.LowPart = DirContext->DirEntry.FatX.FileSize;
            /* Make allocsize a rounded up multiple of BytesPerCluster */
            pInfo->AllocationSize.u.HighPart = 0;
            pInfo->AllocationSize.u.LowPart = ROUND_UP(DirContext->DirEntry.FatX.FileSize,
                                                       DeviceExt->FatInfo.BytesPerCluster);
        }

        pInfo->FileAttributes = DirContext->DirEntry.FatX.Attrib & 0x3f;
    }
    else
    {
        FsdDosDateTimeToSystemTime(DeviceExt,
                                   DirContext->DirEntry.Fat.CreationDate,
                                   DirContext->DirEntry.Fat.CreationTime,
                                   &pInfo->CreationTime);
        FsdDosDateTimeToSystemTime(DeviceExt,
                                   DirContext->DirEntry.Fat.AccessDate,
                                   0,
                                   &pInfo->LastAccessTime);
        FsdDosDateTimeToSystemTime(DeviceExt,
                                   DirContext->DirEntry.Fat.UpdateDate,
                                   DirContext->DirEntry.Fat.UpdateTime,
                                   &pInfo->LastWriteTime);

        pInfo->ChangeTime = pInfo->LastWriteTime;

        if (BooleanFlagOn(DirContext->DirEntry.Fat.Attrib, FILE_ATTRIBUTE_DIRECTORY))
        {
            pInfo->EndOfFile.QuadPart = 0;
            pInfo->AllocationSize.QuadPart = 0;
        }
        else
        {
            pInfo->EndOfFile.u.HighPart = 0;
            pInfo->EndOfFile.u.LowPart = DirContext->DirEntry.Fat.FileSize;
            /* Make allocsize a rounded up multiple of BytesPerCluster */
            pInfo->AllocationSize.u.HighPart = 0;
            pInfo->AllocationSize.u.LowPart = ROUND_UP(DirContext->DirEntry.Fat.FileSize,
                                                       DeviceExt->FatInfo.BytesPerCluster);
        }

        pInfo->FileAttributes = DirContext->DirEntry.Fat.Attrib & 0x3f;
    }

    return STATUS_SUCCESS;
}

static
NTSTATUS
VfatGetFileFullDirectoryInformation(
    PVFAT_DIRENTRY_CONTEXT DirContext,
    PDEVICE_EXTENSION DeviceExt,
    PFILE_FULL_DIR_INFORMATION pInfo,
    ULONG BufferLength)
{
    if ((sizeof(FILE_FULL_DIR_INFORMATION) + DirContext->LongNameU.Length) > BufferLength)
        return STATUS_BUFFER_OVERFLOW;

    pInfo->FileNameLength = DirContext->LongNameU.Length;
    pInfo->NextEntryOffset = ULONG_ROUND_UP(sizeof(FILE_FULL_DIR_INFORMATION) +
                                            DirContext->LongNameU.Length);
    /* pInfo->FileIndex = ; */
    /* pInfo->EaSize = ; */

    RtlCopyMemory(pInfo->FileName,
                  DirContext->LongNameU.Buffer,
                  DirContext->LongNameU.Length);

    if (vfatVolumeIsFatX(DeviceExt))
    {
        FsdDosDateTimeToSystemTime(DeviceExt,
                                   DirContext->DirEntry.FatX.CreationDate,
                                   DirContext->DirEntry.FatX.CreationTime,
                                   &pInfo->CreationTime);
        FsdDosDateTimeToSystemTime(DeviceExt,
                                   DirContext->DirEntry.FatX.AccessDate,
                                   DirContext->DirEntry.FatX.AccessTime,
                                   &pInfo->LastAccessTime);
        FsdDosDateTimeToSystemTime(DeviceExt,
                                   DirContext->DirEntry.FatX.UpdateDate,
                                   DirContext->DirEntry.FatX.UpdateTime,
                                   &pInfo->LastWriteTime);

        pInfo->ChangeTime = pInfo->LastWriteTime;
        pInfo->EndOfFile.u.HighPart = 0;
        pInfo->EndOfFile.u.LowPart = DirContext->DirEntry.FatX.FileSize;
        /* Make allocsize a rounded up multiple of BytesPerCluster */
        pInfo->AllocationSize.u.HighPart = 0;
        pInfo->AllocationSize.u.LowPart = ROUND_UP(DirContext->DirEntry.FatX.FileSize,
                                                   DeviceExt->FatInfo.BytesPerCluster);
        pInfo->FileAttributes = DirContext->DirEntry.FatX.Attrib & 0x3f;
    }
    else
    {
        FsdDosDateTimeToSystemTime(DeviceExt,
                                   DirContext->DirEntry.Fat.CreationDate,
                                   DirContext->DirEntry.Fat.CreationTime,
                                   &pInfo->CreationTime);
        FsdDosDateTimeToSystemTime(DeviceExt,
                                   DirContext->DirEntry.Fat.AccessDate,
                                   0,
                                   &pInfo->LastAccessTime);
        FsdDosDateTimeToSystemTime(DeviceExt,
                                   DirContext->DirEntry.Fat.UpdateDate,
                                   DirContext->DirEntry.Fat.UpdateTime,
                                   &pInfo->LastWriteTime);

        pInfo->ChangeTime = pInfo->LastWriteTime;
        pInfo->EndOfFile.u.HighPart = 0;
        pInfo->EndOfFile.u.LowPart = DirContext->DirEntry.Fat.FileSize;
        /* Make allocsize a rounded up multiple of BytesPerCluster */
        pInfo->AllocationSize.u.HighPart = 0;
        pInfo->AllocationSize.u.LowPart = ROUND_UP(DirContext->DirEntry.Fat.FileSize,
                                                   DeviceExt->FatInfo.BytesPerCluster);
        pInfo->FileAttributes = DirContext->DirEntry.Fat.Attrib & 0x3f;
    }

    return STATUS_SUCCESS;
}

static
NTSTATUS
VfatGetFileBothInformation(
    PVFAT_DIRENTRY_CONTEXT DirContext,
    PDEVICE_EXTENSION DeviceExt,
    PFILE_BOTH_DIR_INFORMATION pInfo,
    ULONG BufferLength)
{
    if ((sizeof(FILE_BOTH_DIR_INFORMATION) + DirContext->LongNameU.Length) > BufferLength)
        return STATUS_BUFFER_OVERFLOW;

    pInfo->EaSize = 0;

    if (vfatVolumeIsFatX(DeviceExt))
    {
        pInfo->FileNameLength = DirContext->LongNameU.Length;

        RtlCopyMemory(pInfo->FileName,
                      DirContext->LongNameU.Buffer,
                      DirContext->LongNameU.Length);

        pInfo->NextEntryOffset = ULONG_ROUND_UP(sizeof(FILE_BOTH_DIR_INFORMATION) +
                                                DirContext->LongNameU.Length);
        pInfo->ShortName[0] = 0;
        pInfo->ShortNameLength = 0;
        /* pInfo->FileIndex = ; */

        FsdDosDateTimeToSystemTime(DeviceExt,
                                   DirContext->DirEntry.FatX.CreationDate,
                                   DirContext->DirEntry.FatX.CreationTime,
                                   &pInfo->CreationTime);
        FsdDosDateTimeToSystemTime(DeviceExt,
                                   DirContext->DirEntry.FatX.AccessDate,
                                   DirContext->DirEntry.FatX.AccessTime,
                                   &pInfo->LastAccessTime);
        FsdDosDateTimeToSystemTime(DeviceExt,
                                   DirContext->DirEntry.FatX.UpdateDate,
                                   DirContext->DirEntry.FatX.UpdateTime,
                                   &pInfo->LastWriteTime);

        pInfo->ChangeTime = pInfo->LastWriteTime;

        if (BooleanFlagOn(DirContext->DirEntry.FatX.Attrib, FILE_ATTRIBUTE_DIRECTORY))
        {
            pInfo->EndOfFile.QuadPart = 0;
            pInfo->AllocationSize.QuadPart = 0;
        }
        else
        {
            pInfo->EndOfFile.u.HighPart = 0;
            pInfo->EndOfFile.u.LowPart = DirContext->DirEntry.FatX.FileSize;
            /* Make allocsize a rounded up multiple of BytesPerCluster */
            pInfo->AllocationSize.u.HighPart = 0;
            pInfo->AllocationSize.u.LowPart = ROUND_UP(DirContext->DirEntry.FatX.FileSize,
                                                       DeviceExt->FatInfo.BytesPerCluster);
        }

        pInfo->FileAttributes = DirContext->DirEntry.FatX.Attrib & 0x3f;
    }
    else
    {
        pInfo->FileNameLength = DirContext->LongNameU.Length;
        pInfo->NextEntryOffset = ULONG_ROUND_UP(sizeof(FILE_BOTH_DIR_INFORMATION) +
                                                DirContext->LongNameU.Length);

        RtlCopyMemory(pInfo->ShortName,
                      DirContext->ShortNameU.Buffer,
                      DirContext->ShortNameU.Length);

        pInfo->ShortNameLength = (CCHAR)DirContext->ShortNameU.Length;

        RtlCopyMemory(pInfo->FileName,
                      DirContext->LongNameU.Buffer,
                      DirContext->LongNameU.Length);

        /* pInfo->FileIndex = ; */

        FsdDosDateTimeToSystemTime(DeviceExt,
                                   DirContext->DirEntry.Fat.CreationDate,
                                   DirContext->DirEntry.Fat.CreationTime,
                                   &pInfo->CreationTime);
        FsdDosDateTimeToSystemTime(DeviceExt,
                                   DirContext->DirEntry.Fat.AccessDate,
                                   0,
                                   &pInfo->LastAccessTime);
        FsdDosDateTimeToSystemTime(DeviceExt,
                                   DirContext->DirEntry.Fat.UpdateDate,
                                   DirContext->DirEntry.Fat.UpdateTime,
                                   &pInfo->LastWriteTime);

        pInfo->ChangeTime = pInfo->LastWriteTime;

        if (BooleanFlagOn(DirContext->DirEntry.Fat.Attrib, FILE_ATTRIBUTE_DIRECTORY))
        {
            pInfo->EndOfFile.QuadPart = 0;
            pInfo->AllocationSize.QuadPart = 0;
        }
        else
        {
            pInfo->EndOfFile.u.HighPart = 0;
            pInfo->EndOfFile.u.LowPart = DirContext->DirEntry.Fat.FileSize;
            /* Make allocsize a rounded up multiple of BytesPerCluster */
            pInfo->AllocationSize.u.HighPart = 0;
            pInfo->AllocationSize.u.LowPart = ROUND_UP(DirContext->DirEntry.Fat.FileSize, DeviceExt->FatInfo.BytesPerCluster);
        }

        pInfo->FileAttributes = DirContext->DirEntry.Fat.Attrib & 0x3f;
    }

    return STATUS_SUCCESS;
}

static
NTSTATUS
DoQuery(
    PVFAT_IRP_CONTEXT IrpContext)
{
    NTSTATUS Status = STATUS_SUCCESS;
    LONG BufferLength = 0;
    PUNICODE_STRING pSearchPattern = NULL;
    FILE_INFORMATION_CLASS FileInformationClass;
    PUCHAR Buffer = NULL;
    PFILE_NAMES_INFORMATION Buffer0 = NULL;
    PVFATFCB pFcb;
    PVFATCCB pCcb;
    BOOLEAN FirstQuery = FALSE;
    BOOLEAN FirstCall = TRUE;
    VFAT_DIRENTRY_CONTEXT DirContext;
    WCHAR LongNameBuffer[LONGNAME_MAX_LENGTH + 1];
    WCHAR ShortNameBuffer[13];

    PIO_STACK_LOCATION Stack = IrpContext->Stack;

    pCcb = (PVFATCCB)IrpContext->FileObject->FsContext2;
    pFcb = (PVFATFCB)IrpContext->FileObject->FsContext;

    /* Determine Buffer for result : */
    BufferLength = Stack->Parameters.QueryDirectory.Length;
#if 0
    /* Do not probe the user buffer until SEH is available */
    if (IrpContext->Irp->RequestorMode != KernelMode &&
        IrpContext->Irp->MdlAddress == NULL &&
        IrpContext->Irp->UserBuffer != NULL)
    {
        ProbeForWrite(IrpContext->Irp->UserBuffer, BufferLength, 1);
    }
#endif
    Buffer = VfatGetUserBuffer(IrpContext->Irp, FALSE);

    if (!ExAcquireResourceExclusiveLite(&IrpContext->DeviceExt->DirResource,
                                        BooleanFlagOn(IrpContext->Flags, IRPCONTEXT_CANWAIT)))
    {
        Status = VfatLockUserBuffer(IrpContext->Irp, BufferLength, IoWriteAccess);
        if (NT_SUCCESS(Status))
            Status = STATUS_PENDING;

        return Status;
    }

    if (!ExAcquireResourceSharedLite(&pFcb->MainResource,
                                     BooleanFlagOn(IrpContext->Flags, IRPCONTEXT_CANWAIT)))
    {
        ExReleaseResourceLite(&IrpContext->DeviceExt->DirResource);
        Status = VfatLockUserBuffer(IrpContext->Irp, BufferLength, IoWriteAccess);
        if (NT_SUCCESS(Status))
            Status = STATUS_PENDING;

        return Status;
    }

    /* Obtain the callers parameters */
#ifdef _MSC_VER
    /* HACKHACK: Bug in the MS ntifs.h header:
     * FileName is really a PUNICODE_STRING, not a PSTRING */
    pSearchPattern = (PUNICODE_STRING)Stack->Parameters.QueryDirectory.FileName;
#else
    pSearchPattern = Stack->Parameters.QueryDirectory.FileName;
#endif
    FileInformationClass = Stack->Parameters.QueryDirectory.FileInformationClass;

    /* Allocate search pattern in case:
     * -> We don't have one already in context
     * -> We have been given an input pattern
     * -> The pattern length is not null
     * -> The pattern buffer is not null
     * Otherwise, we'll fall later and allocate a match all (*) pattern
     */
    if (pSearchPattern &&
        pSearchPattern->Length != 0 && pSearchPattern->Buffer != NULL)
    {
        if (!pCcb->SearchPattern.Buffer)
        {
            FirstQuery = TRUE;
            pCcb->SearchPattern.MaximumLength = pSearchPattern->Length + sizeof(WCHAR);
            pCcb->SearchPattern.Buffer = ExAllocatePoolWithTag(NonPagedPool,
                                                               pCcb->SearchPattern.MaximumLength,
                                                               TAG_VFAT);
            if (!pCcb->SearchPattern.Buffer)
            {
                ExReleaseResourceLite(&pFcb->MainResource);
                ExReleaseResourceLite(&IrpContext->DeviceExt->DirResource);
                return STATUS_INSUFFICIENT_RESOURCES;
            }
            RtlCopyUnicodeString(&pCcb->SearchPattern, pSearchPattern);
            pCcb->SearchPattern.Buffer[pCcb->SearchPattern.Length / sizeof(WCHAR)] = 0;
        }
    }
    else if (!pCcb->SearchPattern.Buffer)
    {
        FirstQuery = TRUE;
        pCcb->SearchPattern.MaximumLength = 2 * sizeof(WCHAR);
        pCcb->SearchPattern.Buffer = ExAllocatePoolWithTag(NonPagedPool,
                                                           2 * sizeof(WCHAR),
                                                           TAG_VFAT);
        if (!pCcb->SearchPattern.Buffer)
        {
            ExReleaseResourceLite(&pFcb->MainResource);
            ExReleaseResourceLite(&IrpContext->DeviceExt->DirResource);
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        pCcb->SearchPattern.Buffer[0] = L'*';
        pCcb->SearchPattern.Buffer[1] = 0;
        pCcb->SearchPattern.Length = sizeof(WCHAR);
    }

    if (BooleanFlagOn(IrpContext->Stack->Flags, SL_INDEX_SPECIFIED))
    {
        DirContext.DirIndex = pCcb->Entry = Stack->Parameters.QueryDirectory.FileIndex;
    }
    else if (FirstQuery || BooleanFlagOn(IrpContext->Stack->Flags, SL_RESTART_SCAN))
    {
        DirContext.DirIndex = pCcb->Entry = 0;
    }
    else
    {
        DirContext.DirIndex = pCcb->Entry;
    }

    DPRINT("Buffer=%p tofind=%wZ\n", Buffer, &pCcb->SearchPattern);

    DirContext.LongNameU.Buffer = LongNameBuffer;
    DirContext.LongNameU.MaximumLength = sizeof(LongNameBuffer);
    DirContext.ShortNameU.Buffer = ShortNameBuffer;
    DirContext.ShortNameU.MaximumLength = sizeof(ShortNameBuffer);

    while ((Status == STATUS_SUCCESS) && (BufferLength > 0))
    {
        Status = FindFile(IrpContext->DeviceExt,
                          pFcb,
                          &pCcb->SearchPattern,
                          &DirContext,
                          FirstCall);
        pCcb->Entry = DirContext.DirIndex;

        DPRINT("Found %wZ, Status=%x, entry %x\n", &DirContext.LongNameU, Status, pCcb->Entry);

        FirstCall = FALSE;
        if (NT_SUCCESS(Status))
        {
            switch (FileInformationClass)
            {
                case FileNameInformation:
                    Status = VfatGetFileNameInformation(&DirContext,
                                                        (PFILE_NAMES_INFORMATION)Buffer,
                                                        BufferLength);
                    break;

                case FileDirectoryInformation:
                    Status = VfatGetFileDirectoryInformation(&DirContext,
                                                             IrpContext->DeviceExt,
                                                             (PFILE_DIRECTORY_INFORMATION)Buffer,
                                                             BufferLength);
                    break;

                case FileFullDirectoryInformation:
                    Status = VfatGetFileFullDirectoryInformation(&DirContext,
                                                                 IrpContext->DeviceExt,
                                                                 (PFILE_FULL_DIR_INFORMATION)Buffer,
                                                                 BufferLength);
                    break;

                case FileBothDirectoryInformation:
                    Status = VfatGetFileBothInformation(&DirContext,
                                                        IrpContext->DeviceExt,
                                                        (PFILE_BOTH_DIR_INFORMATION)Buffer,
                                                        BufferLength);
                    break;

                default:
                    Status = STATUS_INVALID_INFO_CLASS;
                    break;
            }

            if (Status == STATUS_BUFFER_OVERFLOW || Status == STATUS_INVALID_INFO_CLASS)
                break;
        }
        else
        {
            Status = (FirstQuery ? STATUS_NO_SUCH_FILE : STATUS_NO_MORE_FILES);
            break;
        }

        Buffer0 = (PFILE_NAMES_INFORMATION) Buffer;
        Buffer0->FileIndex = DirContext.DirIndex;
        pCcb->Entry = ++DirContext.DirIndex;
        BufferLength -= Buffer0->NextEntryOffset;

        if (BooleanFlagOn(IrpContext->Stack->Flags, SL_RETURN_SINGLE_ENTRY))
            break;

        Buffer += Buffer0->NextEntryOffset;
    }

    if (Buffer0)
    {
        Buffer0->NextEntryOffset = 0;
        Status = STATUS_SUCCESS;
        IrpContext->Irp->IoStatus.Information = Stack->Parameters.QueryDirectory.Length - BufferLength;
    }

    ExReleaseResourceLite(&pFcb->MainResource);
    ExReleaseResourceLite(&IrpContext->DeviceExt->DirResource);

    return Status;
}

NTSTATUS VfatNotifyChangeDirectory(PVFAT_IRP_CONTEXT IrpContext)
{
    PVCB pVcb;
    PVFATFCB pFcb;
    PIO_STACK_LOCATION Stack;
    Stack = IrpContext->Stack;
    pVcb = IrpContext->DeviceExt;
    pFcb = (PVFATFCB) IrpContext->FileObject->FsContext;

    FsRtlNotifyFullChangeDirectory(pVcb->NotifySync,
                                   &(pVcb->NotifyList),
                                   IrpContext->FileObject->FsContext2,
                                   (PSTRING)&(pFcb->PathNameU),
                                   BooleanFlagOn(Stack->Flags, SL_WATCH_TREE),
                                   FALSE,
                                   Stack->Parameters.NotifyDirectory.CompletionFilter,
                                   IrpContext->Irp,
                                   NULL,
                                   NULL);

    /* We won't handle IRP completion */
    IrpContext->Flags &= ~IRPCONTEXT_COMPLETE;

    return STATUS_PENDING;
}

/*
 * FUNCTION: directory control : read/write directory informations
 */
NTSTATUS
VfatDirectoryControl(
    PVFAT_IRP_CONTEXT IrpContext)
{
    NTSTATUS Status = STATUS_SUCCESS;

    IrpContext->Irp->IoStatus.Information = 0;

    switch (IrpContext->MinorFunction)
    {
        case IRP_MN_QUERY_DIRECTORY:
            Status = DoQuery (IrpContext);
            break;

        case IRP_MN_NOTIFY_CHANGE_DIRECTORY:
            Status = VfatNotifyChangeDirectory(IrpContext);
            break;

        default:
            /* Error */
            DPRINT("Unexpected minor function %x in VFAT driver\n",
                   IrpContext->MinorFunction);
            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;
    }

    if (Status == STATUS_PENDING && BooleanFlagOn(IrpContext->Flags, IRPCONTEXT_COMPLETE))
    {
        return VfatMarkIrpContextForQueue(IrpContext);
    }

    return Status;
}

/* EOF */
