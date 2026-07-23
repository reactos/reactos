/*
 * PROJECT:     ReactOS exFAT filesystem driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     FatFs, block-device, path, and FCB support
 * COPYRIGHT:   Copyright 2026 Ahmed ARIF <arif.ing@outlook.com>
 */

#include "exfat.h"

#define NDEBUG
#include <debug.h>

PARTITION VolToPart[FF_VOLUMES] = { { 0, 0 } };

/* Keep the driver and FatFs configuration revisions in sync. */
C_ASSERT(FFCONF_DEF == 80386);

/*
 * Set-associative metadata cache: block-granular fills so a cold directory
 * sweep costs one IRP per block instead of one per sector, sized so the
 * boot-relevant metadata set (directories, FAT, bitmap) stays resident.
 * Tags store block numbers (sector / SectorCacheBlockSectors).
 */
#define EXFAT_SECTOR_CACHE_SIZE  (1024 * 1024)
#define EXFAT_SECTOR_CACHE_BLOCK 4096
#define EXFAT_SECTOR_CACHE_WAYS  4
#define EXFAT_SECTOR_CACHE_EMPTY ((LBA_t)~0ULL)

typedef struct _EXFAT_IO_CONTEXT
{
    KEVENT Event;
    IO_STATUS_BLOCK IoStatus;
    BOOLEAN OwnMdl;
    BOOLEAN UnlockPages;
} EXFAT_IO_CONTEXT, *PEXFAT_IO_CONTEXT;

static VOID ExFatInvalidateSectorCache(PEXFAT_VCB Vcb);
static VOID ExFatInvalidateSectorCacheRange(PEXFAT_VCB Vcb, LBA_t Sector, UINT Count);
static NTSTATUS ExFatFlushSectorCacheRange(PEXFAT_VCB Vcb, LBA_t Sector, UINT Count);

NTSTATUS
ExFatMapResult(
    FRESULT Result)
{
    switch (Result)
    {
        case FR_OK:
            return STATUS_SUCCESS;
        case FR_DISK_ERR:
            return STATUS_IO_DEVICE_ERROR;
        case FR_INT_ERR:
            return STATUS_FILE_CORRUPT_ERROR;
        case FR_NOT_READY:
            return STATUS_DEVICE_NOT_READY;
        case FR_NO_FILE:
            return STATUS_OBJECT_NAME_NOT_FOUND;
        case FR_NO_PATH:
            return STATUS_OBJECT_PATH_NOT_FOUND;
        case FR_INVALID_NAME:
        case FR_INVALID_PARAMETER:
            return STATUS_OBJECT_NAME_INVALID;
        case FR_DENIED:
            return STATUS_ACCESS_DENIED;
        case FR_EXIST:
            return STATUS_OBJECT_NAME_COLLISION;
        case FR_INVALID_OBJECT:
            return STATUS_FILE_INVALID;
        case FR_WRITE_PROTECTED:
            return STATUS_MEDIA_WRITE_PROTECTED;
        case FR_INVALID_DRIVE:
        case FR_NOT_ENABLED:
            return STATUS_VOLUME_DISMOUNTED;
        case FR_NO_FILESYSTEM:
            return STATUS_UNRECOGNIZED_VOLUME;
        case FR_TIMEOUT:
            return STATUS_IO_TIMEOUT;
        case FR_LOCKED:
            return STATUS_SHARING_VIOLATION;
        case FR_NOT_ENOUGH_CORE:
            return STATUS_INSUFFICIENT_RESOURCES;
        case FR_TOO_MANY_OPEN_FILES:
            return STATUS_TOO_MANY_OPENED_FILES;
        default:
            return STATUS_UNSUCCESSFUL;
    }
}

VOID
ExFatAcquireFatFs(
    PEXFAT_VCB Vcb)
{
    KeEnterCriticalRegion();
    ExAcquireResourceExclusiveLite(&Vcb->FatFsResource, TRUE);
}

VOID
ExFatReleaseFatFs(
    PEXFAT_VCB Vcb)
{
    ExReleaseResourceLite(&Vcb->FatFsResource);
    KeLeaveCriticalRegion();
}

FRESULT
ExFatCloseFcbFile(
    PEXFAT_FCB Fcb)
{
    FRESULT Result;

    if (!Fcb->FatFileOpen)
        return FR_OK;

    Result = f_close(&Fcb->FatFile);
    Fcb->FatFileOpen = FALSE;
    Fcb->FatFileWritable = FALSE;
    return Result;
}

FRESULT
ExFatEnsureFcbFile(
    PEXFAT_FCB Fcb,
    BOOLEAN WriteAccess)
{
    BYTE Mode;
    FRESULT Result;

    if (Fcb->FatFileOpen && (!WriteAccess || Fcb->FatFileWritable))
        return FR_OK;

    Result = ExFatCloseFcbFile(Fcb);
    if (Result != FR_OK)
        return Result;

    Mode = FA_READ | FA_OPEN_EXISTING;
    if (WriteAccess)
        Mode |= FA_WRITE;
    Result = f_open(&Fcb->FatFile, Fcb->FatPath, Mode);
    if (Result == FR_OK)
    {
        Fcb->FatFileOpen = TRUE;
        Fcb->FatFileWritable = WriteAccess;
    }
    return Result;
}

FRESULT
ExFatZeroFileRange(
    PEXFAT_FCB Fcb,
    FSIZE_t Start,
    FSIZE_t End)
{
    PEXFAT_VCB Vcb = Fcb->Vcb;
    FSIZE_t Remaining;
    UINT Chunk;
    UINT Written;
    FRESULT Result;

    if (End <= Start)
        return FR_OK;

    /* Callers hold the FatFs lock, which also guards this lazy allocation. */
    if (!Vcb->ZeroBuffer)
    {
        Vcb->ZeroBuffer = ExAllocatePoolWithTag(NonPagedPool,
                                                64 * 1024,
                                                TAG_EXFAT_IO);
        if (!Vcb->ZeroBuffer)
            return FR_NOT_ENOUGH_CORE;
        RtlZeroMemory(Vcb->ZeroBuffer, 64 * 1024);
    }

    Result = f_lseek(&Fcb->FatFile, Start);
    Remaining = End - Start;
    while (Result == FR_OK && Remaining != 0)
    {
        Chunk = (UINT)min(Remaining, (FSIZE_t)(64 * 1024));
        Written = 0;
        Result = f_write(&Fcb->FatFile, Vcb->ZeroBuffer, Chunk, &Written);
        if (Result == FR_OK && Written != Chunk)
            Result = FR_DISK_ERR;
        Remaining -= Written;
    }

    return Result;
}

PVOID
ExFatGetUserBuffer(
    PIRP Irp,
    BOOLEAN PagingIo)
{
    if (Irp->MdlAddress)
    {
        return MmGetSystemAddressForMdlSafe(Irp->MdlAddress,
                                            PagingIo ? HighPagePriority : NormalPagePriority);
    }

    if (Irp->AssociatedIrp.SystemBuffer)
        return Irp->AssociatedIrp.SystemBuffer;

    return Irp->UserBuffer;
}

NTSTATUS
ExFatLockUserBuffer(
    PIRP Irp,
    ULONG Length,
    LOCK_OPERATION Operation)
{
    if (Irp->MdlAddress || Length == 0)
        return STATUS_SUCCESS;

    IoAllocateMdl(Irp->UserBuffer, Length, FALSE, FALSE, Irp);
    if (!Irp->MdlAddress)
        return STATUS_INSUFFICIENT_RESOURCES;

    _SEH2_TRY
    {
        MmProbeAndLockPages(Irp->MdlAddress, Irp->RequestorMode, Operation);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        IoFreeMdl(Irp->MdlAddress);
        Irp->MdlAddress = NULL;
        _SEH2_YIELD(return _SEH2_GetExceptionCode());
    }
    _SEH2_END;

    return STATUS_SUCCESS;
}

static
NTSTATUS
NTAPI
ExFatReadWriteCompletion(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PVOID Context)
{
    PEXFAT_IO_CONTEXT IoContext = Context;
    PMDL Mdl;

    UNREFERENCED_PARAMETER(DeviceObject);

    IoContext->IoStatus = Irp->IoStatus;

    if (!IoContext->OwnMdl)
    {
        Irp->MdlAddress = NULL;
    }
    while (IoContext->OwnMdl && (Mdl = Irp->MdlAddress) != NULL)
    {
        Irp->MdlAddress = Mdl->Next;
        if (IoContext->UnlockPages)
            MmUnlockPages(Mdl);
        IoFreeMdl(Mdl);
    }

    IoFreeIrp(Irp);
    KeSetEvent(&IoContext->Event, IO_NO_INCREMENT, FALSE);
    return STATUS_MORE_PROCESSING_REQUIRED;
}

/*
 * Submit an I/O request built around a caller-prepared MDL and wait for it.
 * Optionally takes ownership of the MDL (and unlocks its pages if requested).
 * The IRP is hand-built and completed via event: FatFs can issue block I/O
 * from a paging fault at APC_LEVEL, where a synchronous request's completion
 * APC cannot run.
 */
static NTSTATUS
ExFatSubmitDeviceIo(
    PDEVICE_OBJECT DeviceObject,
    UCHAR MajorFunction,
    PMDL Mdl,
    BOOLEAN OwnMdl,
    BOOLEAN UnlockPages,
    ULONG Length,
    PLARGE_INTEGER Offset,
    BOOLEAN OverrideVerify)
{
    EXFAT_IO_CONTEXT IoContext;
    PIO_STACK_LOCATION Stack;
    PIRP Irp;

    KeInitializeEvent(&IoContext.Event, NotificationEvent, FALSE);
    IoContext.IoStatus.Status = STATUS_UNSUCCESSFUL;
    IoContext.IoStatus.Information = 0;
    IoContext.OwnMdl = OwnMdl;
    IoContext.UnlockPages = UnlockPages;

    Irp = IoAllocateIrp(DeviceObject->StackSize, FALSE);
    if (!Irp)
    {
        while (OwnMdl && Mdl)
        {
            PMDL Next = Mdl->Next;
            if (UnlockPages)
                MmUnlockPages(Mdl);
            IoFreeMdl(Mdl);
            Mdl = Next;
        }
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Irp->MdlAddress = Mdl;
    Irp->Tail.Overlay.Thread = PsGetCurrentThread();
    Irp->RequestorMode = KernelMode;
    if (MajorFunction == IRP_MJ_READ)
        Irp->Flags = IRP_READ_OPERATION;
    else if (MajorFunction == IRP_MJ_WRITE)
        Irp->Flags = IRP_WRITE_OPERATION;

    Stack = IoGetNextIrpStackLocation(Irp);
    Stack->MajorFunction = MajorFunction;
    if (MajorFunction == IRP_MJ_READ || MajorFunction == IRP_MJ_WRITE)
    {
        /* Parameters.Read and Parameters.Write share this layout. */
        Stack->Parameters.Read.Length = Length;
        Stack->Parameters.Read.ByteOffset = *Offset;
    }
    if (OverrideVerify)
        Stack->Flags |= SL_OVERRIDE_VERIFY_VOLUME;

    IoSetCompletionRoutine(Irp,
                           ExFatReadWriteCompletion,
                           &IoContext,
                           TRUE,
                           TRUE,
                           TRUE);
    IoCallDriver(DeviceObject, Irp);
    KeWaitForSingleObject(&IoContext.Event, Executive, KernelMode, FALSE, NULL);

    if (NT_SUCCESS(IoContext.IoStatus.Status) && Length != 0 &&
        IoContext.IoStatus.Information != Length)
    {
        return STATUS_DEVICE_DATA_ERROR;
    }
    return IoContext.IoStatus.Status;
}

/* For buffers known to be nonpaged: no PFN-lock probe/unlock cycle. */
static NTSTATUS
ExFatPoolReadWriteDevice(
    PDEVICE_OBJECT DeviceObject,
    UCHAR MajorFunction,
    PVOID PoolBuffer,
    ULONG Length,
    PLARGE_INTEGER Offset,
    BOOLEAN OverrideVerify)
{
    PMDL Mdl;

    Mdl = IoAllocateMdl(PoolBuffer, Length, FALSE, FALSE, NULL);
    if (!Mdl)
        return STATUS_INSUFFICIENT_RESOURCES;
    MmBuildMdlForNonPagedPool(Mdl);
    return ExFatSubmitDeviceIo(DeviceObject,
                               MajorFunction,
                               Mdl,
                               TRUE,
                               FALSE,
                               Length,
                               Offset,
                               OverrideVerify);
}

NTSTATUS
ExFatReadWriteDevice(
    PDEVICE_OBJECT DeviceObject,
    UCHAR MajorFunction,
    PVOID Buffer,
    ULONG Length,
    PLARGE_INTEGER Offset,
    BOOLEAN OverrideVerify)
{
    PVOID Allocation = NULL;
    PVOID IoBuffer = Buffer;
    PMDL Mdl = NULL;
    BOOLEAN UnlockPages = FALSE;
    ULONG AlignmentMask;
    NTSTATUS Status;

    if (Length != 0)
    {
        AlignmentMask = DeviceObject->AlignmentRequirement;
        if (((ULONG_PTR)Buffer & AlignmentMask) != 0)
        {
            if (Length > MAXULONG - AlignmentMask)
                return STATUS_INVALID_BUFFER_SIZE;

            Allocation = ExAllocatePoolWithTag(NonPagedPool,
                                               Length + AlignmentMask,
                                               TAG_EXFAT_IO);
            if (!Allocation)
                return STATUS_INSUFFICIENT_RESOURCES;

            IoBuffer = ALIGN_UP_POINTER_BY(Allocation, AlignmentMask + 1);
            if (MajorFunction == IRP_MJ_WRITE)
                RtlCopyMemory(IoBuffer, Buffer, Length);
        }

        Mdl = IoAllocateMdl(IoBuffer, Length, FALSE, FALSE, NULL);
        if (!Mdl)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }

        if (Allocation)
        {
            MmBuildMdlForNonPagedPool(Mdl);
        }
        else
        {
            _SEH2_TRY
            {
                MmProbeAndLockPages(Mdl,
                                    KernelMode,
                                    (MajorFunction == IRP_MJ_READ) ? IoWriteAccess
                                                                   : IoReadAccess);
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                IoFreeMdl(Mdl);
                Status = _SEH2_GetExceptionCode();
                _SEH2_YIELD(goto Cleanup);
            }
            _SEH2_END;
            UnlockPages = TRUE;
        }
    }

    Status = ExFatSubmitDeviceIo(DeviceObject,
                                 MajorFunction,
                                 Mdl,
                                 TRUE,
                                 UnlockPages,
                                 Length,
                                 Offset,
                                 OverrideVerify);

Cleanup:
    if (Allocation)
    {
        if (MajorFunction == IRP_MJ_READ && NT_SUCCESS(Status))
            RtlCopyMemory(Buffer, IoBuffer, Length);
        ExFreePoolWithTag(Allocation, TAG_EXFAT_IO);
    }
    return Status;
}

NTSTATUS
ExFatRawWriteDevice(
    PEXFAT_VCB Vcb,
    PVOID Buffer,
    ULONG Length,
    PLARGE_INTEGER Offset)
{
    NTSTATUS Status;

    /* Raw writes bypass disk_write(); keep the LBA cache coherent here. */
    Status = ExFatFlushSectorCache(Vcb);
    if (!NT_SUCCESS(Status))
        return Status;
    ExFatInvalidateSectorCache(Vcb);
    return ExFatReadWriteDevice(Vcb->StorageDevice,
                                IRP_MJ_WRITE,
                                Buffer,
                                Length,
                                Offset,
                                FALSE);
}

NTSTATUS
ExFatFlushStorageDevice(
    PEXFAT_VCB Vcb)
{
    return ExFatReadWriteDevice(Vcb->StorageDevice,
                                IRP_MJ_FLUSH_BUFFERS,
                                NULL,
                                0,
                                NULL,
                                TRUE);
}

NTSTATUS
ExFatDeviceIoControl(
    PDEVICE_OBJECT DeviceObject,
    ULONG ControlCode,
    PVOID InputBuffer,
    ULONG InputLength,
    PVOID OutputBuffer,
    PULONG OutputLength)
{
    IO_STATUS_BLOCK IoStatus;
    PIO_STACK_LOCATION Stack;
    KEVENT Event;
    PIRP Irp;
    NTSTATUS Status;

    KeInitializeEvent(&Event, NotificationEvent, FALSE);
    Irp = IoBuildDeviceIoControlRequest(ControlCode,
                                        DeviceObject,
                                        InputBuffer,
                                        InputLength,
                                        OutputBuffer,
                                        OutputLength ? *OutputLength : 0,
                                        FALSE,
                                        &Event,
                                        &IoStatus);
    if (!Irp)
        return STATUS_INSUFFICIENT_RESOURCES;

    Stack = IoGetNextIrpStackLocation(Irp);
    Stack->Flags |= SL_OVERRIDE_VERIFY_VOLUME;

    Status = IoCallDriver(DeviceObject, Irp);
    if (Status == STATUS_PENDING)
    {
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
        Status = IoStatus.Status;
    }

    if (OutputLength)
        *OutputLength = (ULONG)IoStatus.Information;

    return Status;
}

VOID
ExFatBuildDrivePath(
    PEXFAT_VCB Vcb,
    TCHAR Path[3])
{
    Path[0] = (TCHAR)(L'0' + Vcb->DriveNumber);
    Path[1] = L':';
    Path[2] = 0;
}

VOID
ExFatFreeUnicodeString(
    PUNICODE_STRING String)
{
    if (String->Buffer)
        ExFreePoolWithTag(String->Buffer, TAG_EXFAT_PATH);
    RtlZeroMemory(String, sizeof(*String));
}

static __inline WCHAR
ExFatUpcasePathCharacter(
    WCHAR Character)
{
    if (Character >= L'a' && Character <= L'z')
        return Character - (L'a' - L'A');
    if (Character <= 0x7F)
        return Character;
    return RtlUpcaseUnicodeChar(Character);
}

NTSTATUS
ExFatBuildFullPath(
    PFILE_OBJECT FileObject,
    PUNICODE_STRING FullPath,
    PWCHAR PathBuffer,
    USHORT PathBufferSize,
    PULONGLONG PathHash)
{
    PEXFAT_FCB RelatedFcb = NULL;
    USHORT RelatedLength = 0;
    USHORT NameLength = FileObject->FileName.Length;
    USHORT SeparatorLength = 0;
    USHORT PrefixLength = 0;
    ULONG TotalLength;
    PWCHAR Destination;
    ULONG Index;
    ULONGLONG Hash = 1469598103934665603ULL;
    BOOLEAN Trimmed = FALSE;
    WCHAR Character;

    RtlZeroMemory(FullPath, sizeof(*FullPath));

    if (FileObject->RelatedFileObject)
    {
        RelatedFcb = FileObject->RelatedFileObject->FsContext;
        if (!RelatedFcb || RelatedFcb->IsVolume)
            return STATUS_INVALID_PARAMETER;
        if (NameLength && (FileObject->FileName.Buffer[0] == L'\\' || FileObject->FileName.Buffer[0] == L'/'))
            return STATUS_INVALID_PARAMETER;
        RelatedLength = RelatedFcb->PathName.Length;
        if (NameLength && RelatedLength > sizeof(WCHAR))
            SeparatorLength = sizeof(WCHAR);
    }
    else if (!NameLength)
    {
        return STATUS_SUCCESS;
    }
    else if (FileObject->FileName.Buffer[0] != L'\\' && FileObject->FileName.Buffer[0] != L'/')
    {
        PrefixLength = sizeof(WCHAR);
    }

    TotalLength = RelatedLength + SeparatorLength + PrefixLength + NameLength;
    if (TotalLength > MAXUSHORT - sizeof(WCHAR))
        return STATUS_NAME_TOO_LONG;

    if (PathBuffer && PathBufferSize >= TotalLength + sizeof(WCHAR))
    {
        FullPath->Buffer = PathBuffer;
    }
    else
    {
        FullPath->Buffer = ExAllocatePoolWithTag(NonPagedPool,
                                                 TotalLength + sizeof(WCHAR),
                                                 TAG_EXFAT_PATH);
        if (!FullPath->Buffer)
            return STATUS_INSUFFICIENT_RESOURCES;
    }

    Destination = FullPath->Buffer;
    if (RelatedLength)
    {
        RtlCopyMemory(Destination, RelatedFcb->PathName.Buffer, RelatedLength);
        Destination += RelatedLength / sizeof(WCHAR);
    }
    if (SeparatorLength || PrefixLength)
        *Destination++ = L'\\';
    if (NameLength)
    {
        RtlCopyMemory(Destination, FileObject->FileName.Buffer, NameLength);
        Destination += NameLength / sizeof(WCHAR);
    }
    *Destination = UNICODE_NULL;

    FullPath->Length = (USHORT)TotalLength;
    FullPath->MaximumLength = (USHORT)(TotalLength + sizeof(WCHAR));
    for (Index = 0; Index < FullPath->Length / sizeof(WCHAR); ++Index)
    {
        if (FullPath->Buffer[Index] == L'/')
            FullPath->Buffer[Index] = L'\\';
        Character = ExFatUpcasePathCharacter(FullPath->Buffer[Index]);
        Hash ^= Character;
        Hash *= 1099511628211ULL;
    }

    while (FullPath->Length > sizeof(WCHAR) &&
           FullPath->Buffer[FullPath->Length / sizeof(WCHAR) - 1] == L'\\')
    {
        Trimmed = TRUE;
        FullPath->Length -= sizeof(WCHAR);
        FullPath->Buffer[FullPath->Length / sizeof(WCHAR)] = UNICODE_NULL;
    }

    if (Trimmed)
    {
        Hash = 1469598103934665603ULL;
        for (Index = 0; Index < FullPath->Length / sizeof(WCHAR); ++Index)
        {
            Character = ExFatUpcasePathCharacter(FullPath->Buffer[Index]);
            Hash ^= Character;
            Hash *= 1099511628211ULL;
        }
    }
    *PathHash = Hash;
    return STATUS_SUCCESS;
}

TCHAR*
ExFatBuildFatPath(
    PEXFAT_VCB Vcb,
    PUNICODE_STRING PathName)
{
    ULONG Characters = PathName->Length / sizeof(WCHAR);
    TCHAR* Path;
    ULONG Index;

    /* "<drive>:" + path (or "/" for the root) + terminator */
    Path = ExAllocatePoolWithTag(NonPagedPool,
                                 ((SIZE_T)Characters + 4) * sizeof(TCHAR),
                                 TAG_EXFAT_PATH);
    if (!Path)
        return NULL;

    Path[0] = (TCHAR)(L'0' + Vcb->DriveNumber);
    Path[1] = L':';
    for (Index = 0; Index < Characters; ++Index)
    {
        Path[Index + 2] = (PathName->Buffer[Index] == L'\\') ? L'/'
                                                             : PathName->Buffer[Index];
    }
    if (Characters == 0)
        Path[2 + Characters++] = L'/';
    Path[Characters + 2] = 0;
    return Path;
}

ULONG
ExFatFatAttributesToNt(
    BYTE Attributes)
{
    ULONG NtAttributes = 0;

    if (Attributes & AM_RDO)
        NtAttributes |= FILE_ATTRIBUTE_READONLY;
    if (Attributes & AM_HID)
        NtAttributes |= FILE_ATTRIBUTE_HIDDEN;
    if (Attributes & AM_SYS)
        NtAttributes |= FILE_ATTRIBUTE_SYSTEM;
    if (Attributes & AM_ARC)
        NtAttributes |= FILE_ATTRIBUTE_ARCHIVE;
    if (Attributes & AM_DIR)
        NtAttributes |= FILE_ATTRIBUTE_DIRECTORY;
    if (!NtAttributes)
        NtAttributes = FILE_ATTRIBUTE_NORMAL;

    return NtAttributes;
}

BYTE
ExFatNtAttributesToFat(
    ULONG Attributes)
{
    BYTE FatAttributes = 0;

    if (Attributes & FILE_ATTRIBUTE_READONLY)
        FatAttributes |= AM_RDO;
    if (Attributes & FILE_ATTRIBUTE_HIDDEN)
        FatAttributes |= AM_HID;
    if (Attributes & FILE_ATTRIBUTE_SYSTEM)
        FatAttributes |= AM_SYS;
    if (Attributes & FILE_ATTRIBUTE_ARCHIVE)
        FatAttributes |= AM_ARC;
    return FatAttributes;
}

LARGE_INTEGER
ExFatFatTimeToSystemTime(
    WORD Date,
    WORD Time)
{
    TIME_FIELDS Fields;
    LARGE_INTEGER LocalTime;
    LARGE_INTEGER SystemTime;

    SystemTime.QuadPart = 0;
    if (!Date)
        return SystemTime;

    RtlZeroMemory(&Fields, sizeof(Fields));
    Fields.Year = (CSHORT)(1980 + ((Date >> 9) & 0x7F));
    Fields.Month = (CSHORT)((Date >> 5) & 0x0F);
    Fields.Day = (CSHORT)(Date & 0x1F);
    Fields.Hour = (CSHORT)((Time >> 11) & 0x1F);
    Fields.Minute = (CSHORT)((Time >> 5) & 0x3F);
    Fields.Second = (CSHORT)((Time & 0x1F) * 2);
    if (!RtlTimeFieldsToTime(&Fields, &LocalTime))
        return SystemTime;

    ExLocalTimeToSystemTime(&LocalTime, &SystemTime);
    return SystemTime;
}

VOID
ExFatSystemTimeToFatTime(
    PLARGE_INTEGER SystemTime,
    PWORD Date,
    PWORD Time)
{
    LARGE_INTEGER LocalTime;
    TIME_FIELDS Fields;

    ExSystemTimeToLocalTime(SystemTime, &LocalTime);
    RtlTimeToTimeFields(&LocalTime, &Fields);
    if (Fields.Year < 1980)
        Fields.Year = 1980;
    if (Fields.Year > 2107)
        Fields.Year = 2107;

    *Date = (WORD)(((Fields.Year - 1980) << 9) | (Fields.Month << 5) | Fields.Day);
    *Time = (WORD)((Fields.Hour << 11) | (Fields.Minute << 5) | (Fields.Second / 2));
}

ULONGLONG
ExFatRoundUp(
    ULONGLONG Value,
    ULONG Alignment)
{
    if (!Value || !Alignment)
        return Value;
    return ((Value - 1) / Alignment + 1) * Alignment;
}

ULONGLONG
ExFatHashPath(
    PUNICODE_STRING PathName)
{
    ULONGLONG Hash = 1469598103934665603ULL;
    ULONG Index;
    WCHAR Character;

    for (Index = 0; Index < PathName->Length / sizeof(WCHAR); ++Index)
    {
        Character = ExFatUpcasePathCharacter(PathName->Buffer[Index]);
        Hash ^= Character;
        Hash *= 1099511628211ULL;
    }
    return Hash;
}

static NTSTATUS
ExFatSetFcbPath(
    PEXFAT_FCB Fcb,
    PUNICODE_STRING PathName)
{
    PWCHAR Buffer;
    TCHAR* FatPath;

    Buffer = ExAllocatePoolWithTag(NonPagedPool,
                                   PathName->Length + sizeof(WCHAR),
                                   TAG_EXFAT_PATH);
    if (!Buffer)
        return STATUS_INSUFFICIENT_RESOURCES;

    FatPath = ExFatBuildFatPath(Fcb->Vcb, PathName);
    if (!FatPath)
    {
        ExFreePoolWithTag(Buffer, TAG_EXFAT_PATH);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlCopyMemory(Buffer, PathName->Buffer, PathName->Length);
    Buffer[PathName->Length / sizeof(WCHAR)] = UNICODE_NULL;
    if (Fcb->PathName.Buffer)
        ExFreePoolWithTag(Fcb->PathName.Buffer, TAG_EXFAT_PATH);
    if (Fcb->FatPath)
        ExFreePoolWithTag(Fcb->FatPath, TAG_EXFAT_PATH);
    Fcb->PathName.Buffer = Buffer;
    Fcb->PathName.Length = PathName->Length;
    Fcb->PathName.MaximumLength = PathName->Length + sizeof(WCHAR);
    Fcb->FatPath = FatPath;
    Fcb->IndexNumber = ExFatHashPath(PathName);
    return STATUS_SUCCESS;
}

VOID
ExFatUpdateFcbFromInfo(
    PEXFAT_FCB Fcb,
    FILINFO* Information)
{
    Fcb->IsDirectory = !!(Information->fattrib & AM_DIR);
    Fcb->FileAttributes = ExFatFatAttributesToNt(Information->fattrib);
    Fcb->Header.FileSize.QuadPart = Fcb->IsDirectory ? 0 : Information->fsize;
    Fcb->Header.ValidDataLength = Fcb->Header.FileSize;
    Fcb->Header.AllocationSize.QuadPart = Fcb->IsDirectory ? 0 :
        ExFatRoundUp(Information->fsize, Fcb->Vcb->BytesPerCluster);
    Fcb->CreationTime = ExFatFatTimeToSystemTime(Information->crdate, Information->crtime);
    Fcb->LastWriteTime = ExFatFatTimeToSystemTime(Information->fdate, Information->ftime);
    Fcb->LastAccessTime = Fcb->LastWriteTime;
    Fcb->ChangeTime = Fcb->LastWriteTime;
}

PEXFAT_FCB
ExFatCreateFcb(
    PEXFAT_VCB Vcb,
    PUNICODE_STRING PathName,
    FILINFO* Information,
    BOOLEAN IsVolume)
{
    PEXFAT_FCB Fcb;

    Fcb = ExAllocatePoolWithTag(NonPagedPool, sizeof(*Fcb), TAG_EXFAT_FCB);
    if (!Fcb)
        return NULL;
    RtlZeroMemory(Fcb, sizeof(*Fcb));

    Fcb->Header.NodeTypeCode = EXFAT_FCB_SIGNATURE;
    Fcb->Header.NodeByteSize = sizeof(*Fcb);
    Fcb->Header.IsFastIoPossible = FastIoIsQuestionable;
    ExInitializeResourceLite(&Fcb->MainResource);
    ExInitializeResourceLite(&Fcb->PagingIoResource);
    Fcb->Header.Resource = &Fcb->MainResource;
    Fcb->Header.PagingIoResource = &Fcb->PagingIoResource;
    FsRtlInitializeFileLock(&Fcb->FileLock, NULL, NULL);
    Fcb->Vcb = Vcb;
    Fcb->ReferenceCount = 1;
    Fcb->IsVolume = IsVolume;

    if (!NT_SUCCESS(ExFatSetFcbPath(Fcb, PathName)))
    {
        FsRtlUninitializeFileLock(&Fcb->FileLock);
        ExDeleteResourceLite(&Fcb->PagingIoResource);
        ExDeleteResourceLite(&Fcb->MainResource);
        ExFreePoolWithTag(Fcb, TAG_EXFAT_FCB);
        return NULL;
    }

    if (IsVolume)
    {
        Fcb->Header.FileSize.QuadPart = Vcb->SectorCount * Vcb->BytesPerSector;
        Fcb->Header.ValidDataLength = Fcb->Header.FileSize;
        Fcb->Header.AllocationSize = Fcb->Header.FileSize;
    }
    else
    {
        ExFatUpdateFcbFromInfo(Fcb, Information);
    }

    InsertTailList(&Vcb->FcbListHead, &Fcb->ListEntry);
    return Fcb;
}

PEXFAT_FCB
ExFatFindFcb(
    PEXFAT_VCB Vcb,
    PUNICODE_STRING PathName,
    ULONGLONG PathHash)
{
    PLIST_ENTRY Entry;
    PEXFAT_FCB Fcb;

    for (Entry = Vcb->FcbListHead.Blink;
         Entry != &Vcb->FcbListHead;
         Entry = Entry->Blink)
    {
        Fcb = CONTAINING_RECORD(Entry, EXFAT_FCB, ListEntry);
        if (Fcb->DeleteCompleted)
            continue;
        if (Fcb->IndexNumber == PathHash &&
            Fcb->PathName.Length == PathName->Length &&
            (RtlCompareMemory(Fcb->PathName.Buffer,
                              PathName->Buffer,
                              PathName->Length) == PathName->Length ||
             RtlEqualUnicodeString(&Fcb->PathName, PathName, TRUE)))
        {
            if (InterlockedIncrement(&Fcb->ReferenceCount) == 1)
            {
                /* Revived from the closed-FCB cache; refresh its LRU rank. */
                Vcb->CachedFcbCount--;
                if (Fcb->ListEntry.Flink != &Vcb->FcbListHead)
                {
                    RemoveEntryList(&Fcb->ListEntry);
                    InsertTailList(&Vcb->FcbListHead, &Fcb->ListEntry);
                }
            }
            return Fcb;
        }
    }
    return NULL;
}

VOID
ExFatReferenceFcb(
    PEXFAT_FCB Fcb)
{
    InterlockedIncrement(&Fcb->ReferenceCount);
}

static VOID
ExFatFreeFcb(
    PEXFAT_FCB Fcb)
{
    /* Push final sizes/times back into the parent's child index. */
    ExFatDirIndexUpdateFromFcb(Fcb);

    RemoveEntryList(&Fcb->ListEntry);
    if (Fcb->FatFileOpen)
    {
        ExFatAcquireFatFs(Fcb->Vcb);
        ExFatCloseFcbFile(Fcb);
        ExFatReleaseFatFs(Fcb->Vcb);
    }
    FsRtlUninitializeFileLock(&Fcb->FileLock);
    ExDeleteResourceLite(&Fcb->PagingIoResource);
    ExDeleteResourceLite(&Fcb->MainResource);
    ExFatFreeUnicodeString(&Fcb->PathName);
    if (Fcb->FatPath)
        ExFreePoolWithTag(Fcb->FatPath, TAG_EXFAT_PATH);
    ExFreePoolWithTag(Fcb, TAG_EXFAT_FCB);
}

VOID
ExFatDereferenceFcb(
    PEXFAT_FCB Fcb)
{
    PEXFAT_VCB Vcb = Fcb->Vcb;
    PLIST_ENTRY Entry;
    PEXFAT_FCB Victim;

    if (InterlockedDecrement(&Fcb->ReferenceCount) != 0)
        return;

    /*
     * Keep recently closed FCBs (list head = oldest) so a reopen skips the
     * whole FatFs path walk; boot reopens the same binaries constantly.
     */
    if (!Fcb->IsVolume && !Fcb->DeletePending && Vcb->Mounted)
    {
        Vcb->CachedFcbCount++;
        if (Vcb->CachedFcbCount <= EXFAT_FCB_CACHE_LIMIT)
            return;

        for (Entry = Vcb->FcbListHead.Flink;
             Entry != &Vcb->FcbListHead;
             Entry = Entry->Flink)
        {
            Victim = CONTAINING_RECORD(Entry, EXFAT_FCB, ListEntry);
            if (Victim->ReferenceCount == 0 && !Victim->IsVolume)
            {
                ExFatFreeFcb(Victim);
                Vcb->CachedFcbCount--;
                return;
            }
        }
        return;
    }

    ExFatFreeFcb(Fcb);
}

VOID
ExFatPurgeCachedFcbs(
    PEXFAT_VCB Vcb)
{
    PLIST_ENTRY Entry;
    PLIST_ENTRY Next;
    PEXFAT_FCB Fcb;

    for (Entry = Vcb->FcbListHead.Flink;
         Entry != &Vcb->FcbListHead;
         Entry = Next)
    {
        Next = Entry->Flink;
        Fcb = CONTAINING_RECORD(Entry, EXFAT_FCB, ListEntry);
        if (Fcb->ReferenceCount == 0 && !Fcb->IsVolume)
        {
            ExFatFreeFcb(Fcb);
            Vcb->CachedFcbCount--;
        }
    }
}

BOOLEAN
ExFatLookupNegative(
    PEXFAT_VCB Vcb,
    PUNICODE_STRING PathName,
    ULONGLONG PathHash,
    FRESULT* Result)
{
    PEXFAT_NEGATIVE_ENTRY Entry;

    Entry = &Vcb->NegativeCache[PathHash % EXFAT_NEGATIVE_CACHE_SIZE];
    if (Entry->Generation == Vcb->NamespaceGeneration &&
        Entry->PathName.Buffer &&
        Entry->PathName.Length == PathName->Length &&
        RtlEqualUnicodeString(&Entry->PathName, PathName, TRUE))
    {
        *Result = Entry->Result;
        return TRUE;
    }
    return FALSE;
}

VOID
ExFatRememberNegative(
    PEXFAT_VCB Vcb,
    PUNICODE_STRING PathName,
    ULONGLONG PathHash,
    FRESULT Result)
{
    PEXFAT_NEGATIVE_ENTRY Entry;
    PWCHAR Buffer;

    if (!PathName->Length)
        return;
    Entry = &Vcb->NegativeCache[PathHash % EXFAT_NEGATIVE_CACHE_SIZE];
    Buffer = ExAllocatePoolWithTag(NonPagedPool, PathName->Length, TAG_EXFAT_PATH);
    if (!Buffer)
        return;
    RtlCopyMemory(Buffer, PathName->Buffer, PathName->Length);
    if (Entry->PathName.Buffer)
        ExFreePoolWithTag(Entry->PathName.Buffer, TAG_EXFAT_PATH);
    Entry->PathName.Buffer = Buffer;
    Entry->PathName.Length = PathName->Length;
    Entry->PathName.MaximumLength = PathName->Length;
    Entry->Generation = Vcb->NamespaceGeneration;
    Entry->Result = Result;
}

VOID
ExFatSplitPath(
    PUNICODE_STRING FullPath,
    PUNICODE_STRING ParentPath,
    PUNICODE_STRING LeafName)
{
    ULONG Characters = FullPath->Length / sizeof(WCHAR);
    ULONG Split = 0;
    ULONG Index;

    for (Index = 0; Index < Characters; Index++)
    {
        if (FullPath->Buffer[Index] == L'\\')
            Split = Index;
    }

    /* The parent keeps at least the root backslash. */
    ParentPath->Buffer = FullPath->Buffer;
    ParentPath->Length = (USHORT)(max(Split, 1UL) * sizeof(WCHAR));
    ParentPath->MaximumLength = ParentPath->Length;
    LeafName->Buffer = FullPath->Buffer + Split + 1;
    LeafName->Length = (Characters > Split + 1) ?
        (USHORT)((Characters - Split - 1) * sizeof(WCHAR)) : 0;
    LeafName->MaximumLength = LeafName->Length;
}

static VOID
ExFatFreeDirIndexHash(
    PEXFAT_DIR_INDEX Index)
{
    if (Index->HashBuckets)
        ExFreePoolWithTag(Index->HashBuckets, TAG_EXFAT_FATFS);
    Index->HashBuckets = NULL;
    Index->HashBucketCount = 0;
}

static VOID
ExFatResetDirIndex(
    PEXFAT_DIR_INDEX Index)
{
    if (Index->DirPath.Buffer)
        ExFreePoolWithTag(Index->DirPath.Buffer, TAG_EXFAT_PATH);
    if (Index->Children)
        ExFreePoolWithTag(Index->Children, TAG_EXFAT_FATFS);
    ExFatFreeDirIndexHash(Index);
    if (Index->NamePool)
        ExFreePoolWithTag(Index->NamePool, TAG_EXFAT_FATFS);
    RtlZeroMemory(Index, sizeof(*Index));
}

VOID
ExFatDropDirIndexes(
    PEXFAT_VCB Vcb)
{
    ULONG Slot;

    for (Slot = 0; Slot < EXFAT_DIR_INDEX_SLOTS; Slot++)
        ExFatResetDirIndex(&Vcb->DirIndexes[Slot]);
}

PEXFAT_DIR_INDEX
ExFatLookupDirIndex(
    PEXFAT_VCB Vcb,
    PUNICODE_STRING DirPath)
{
    PEXFAT_DIR_INDEX Index;
    ULONG Slot;

    for (Slot = 0; Slot < EXFAT_DIR_INDEX_SLOTS; Slot++)
    {
        Index = &Vcb->DirIndexes[Slot];
        if (Index->DirPath.Buffer &&
            Index->DirPath.Length == DirPath->Length &&
            RtlEqualUnicodeString(&Index->DirPath, DirPath, TRUE))
            return Index;
    }
    return NULL;
}

static PEXFAT_DIR_INDEX
ExFatFindDirIndex(
    PEXFAT_VCB Vcb,
    PUNICODE_STRING DirPath)
{
    PEXFAT_DIR_INDEX Index;

    Index = ExFatLookupDirIndex(Vcb, DirPath);
    if (Index)
        Index->LastUsed = ++Vcb->DirIndexTick;
    return Index;
}

static BOOLEAN
ExFatDirIndexAppend(
    PEXFAT_VCB Vcb,
    PEXFAT_DIR_INDEX Index,
    PCWSTR Name,
    USHORT NameLength,
    FILINFO* Information,
    PEXFAT_FCB Fcb)
{
    UNICODE_STRING NameString;
    PEXFAT_DIR_CHILD Child;
    PEXFAT_DIR_CHILD NewChildren;
    PWCHAR NewPool;
    ULONG NewCapacity;
    ULONG NameCharacters = NameLength / sizeof(WCHAR);

    if (Index->Count >= EXFAT_DIR_INDEX_MAX_NAMES || !NameCharacters)
        return FALSE;

    if (Index->Count == Index->Capacity)
    {
        NewCapacity = Index->Capacity ? Index->Capacity * 2 : 64;
        NewChildren = ExAllocatePoolWithTag(NonPagedPool,
                                            NewCapacity * sizeof(*Child),
                                            TAG_EXFAT_FATFS);
        if (!NewChildren)
            return FALSE;
        if (Index->Children)
        {
            RtlCopyMemory(NewChildren,
                          Index->Children,
                          Index->Count * sizeof(*Child));
            ExFreePoolWithTag(Index->Children, TAG_EXFAT_FATFS);
        }
        Index->Children = NewChildren;
        Index->Capacity = NewCapacity;
    }

    if (Index->PoolUsed + NameCharacters > Index->PoolCapacity)
    {
        NewCapacity = Index->PoolCapacity ? Index->PoolCapacity * 2 : 1024;
        while (NewCapacity < Index->PoolUsed + NameCharacters)
            NewCapacity *= 2;
        NewPool = ExAllocatePoolWithTag(NonPagedPool,
                                        NewCapacity * sizeof(WCHAR),
                                        TAG_EXFAT_FATFS);
        if (!NewPool)
            return FALSE;
        if (Index->NamePool)
        {
            RtlCopyMemory(NewPool,
                          Index->NamePool,
                          Index->PoolUsed * sizeof(WCHAR));
            ExFreePoolWithTag(Index->NamePool, TAG_EXFAT_FATFS);
        }
        Index->NamePool = NewPool;
        Index->PoolCapacity = NewCapacity;
    }

    RtlCopyMemory(Index->NamePool + Index->PoolUsed, Name, NameLength);
    NameString.Buffer = (PWCHAR)Name;
    NameString.Length = NameLength;
    NameString.MaximumLength = NameLength;

    Child = &Index->Children[Index->Count];
    Child->NameHash = ExFatHashPath(&NameString);
    Child->NextHash = 0;
    Child->NameOffset = Index->PoolUsed;
    Child->NameLength = NameLength;
    Child->Attributes = Information->fattrib;
    Child->ModDate = Information->fdate;
    Child->ModTime = Information->ftime;
    Child->CrtDate = Information->crdate;
    Child->CrtTime = Information->crtime;
    if (Fcb)
    {
        Child->FileAttributes = Fcb->FileAttributes;
        Child->CreationTime = Fcb->CreationTime;
        Child->LastWriteTime = Fcb->LastWriteTime;
        Child->FileSize = Fcb->Header.FileSize;
        Child->AllocationSize = Fcb->Header.AllocationSize;
    }
    else
    {
        Child->FileAttributes = ExFatFatAttributesToNt(Information->fattrib);
        Child->CreationTime = ExFatFatTimeToSystemTime(Information->crdate,
                                                       Information->crtime);
        Child->LastWriteTime = ExFatFatTimeToSystemTime(Information->fdate,
                                                        Information->ftime);
        Child->FileSize.QuadPart = (Information->fattrib & AM_DIR) ? 0 :
                                   Information->fsize;
        Child->AllocationSize.QuadPart = (Information->fattrib & AM_DIR) ? 0 :
            ExFatRoundUp(Information->fsize, Vcb->BytesPerCluster);
    }
    if (Index->HashBuckets)
    {
        ULONG Bucket = (ULONG)(Child->NameHash & (Index->HashBucketCount - 1));

        Child->NextHash = Index->HashBuckets[Bucket];
        Index->HashBuckets[Bucket] = Index->Count + 1;
    }
    Index->PoolUsed += NameCharacters;
    Index->Count++;
    return TRUE;
}

static BOOLEAN
ExFatBuildDirIndexHash(
    PEXFAT_DIR_INDEX Index)
{
    PULONG Buckets;
    PEXFAT_DIR_CHILD Child;
    ULONG BucketCount = 64;
    ULONG Bucket;
    ULONG Position;

    if (!Index->Count)
        return TRUE;
    while (BucketCount < Index->Count * 2)
        BucketCount *= 2;

    Buckets = ExAllocatePoolWithTag(NonPagedPool,
                                    BucketCount * sizeof(*Buckets),
                                    TAG_EXFAT_FATFS);
    if (!Buckets)
        return FALSE;
    RtlZeroMemory(Buckets, BucketCount * sizeof(*Buckets));

    for (Position = 0; Position < Index->Count; Position++)
    {
        Child = &Index->Children[Position];
        Bucket = (ULONG)(Child->NameHash & (BucketCount - 1));
        Child->NextHash = Buckets[Bucket];
        Buckets[Bucket] = Position + 1;
    }

    ExFatFreeDirIndexHash(Index);
    Index->HashBuckets = Buckets;
    Index->HashBucketCount = BucketCount;
    return TRUE;
}

PEXFAT_DIR_INDEX
ExFatEnsureDirIndex(
    PEXFAT_VCB Vcb,
    PUNICODE_STRING DirPath)
{
    PEXFAT_DIR_INDEX Index;
    PEXFAT_DIR_INDEX Victim;
    TCHAR* FatPath;
    UNICODE_STRING NameString;
    DIR Directory;
    FILINFO Information;
    FRESULT Result;
    PWCHAR PathCopy;
    ULONG Slot;
    BOOLEAN Overflow = FALSE;

    Index = ExFatFindDirIndex(Vcb, DirPath);
    if (Index)
        return Index->Unindexable ? NULL : Index;

    Victim = &Vcb->DirIndexes[0];
    for (Slot = 0; Slot < EXFAT_DIR_INDEX_SLOTS; Slot++)
    {
        if (!Vcb->DirIndexes[Slot].DirPath.Buffer)
        {
            Victim = &Vcb->DirIndexes[Slot];
            break;
        }
        if (Vcb->DirIndexes[Slot].LastUsed < Victim->LastUsed)
            Victim = &Vcb->DirIndexes[Slot];
    }
    ExFatResetDirIndex(Victim);

    PathCopy = ExAllocatePoolWithTag(NonPagedPool, DirPath->Length, TAG_EXFAT_PATH);
    if (!PathCopy)
        return NULL;
    FatPath = ExFatBuildFatPath(Vcb, DirPath);
    if (!FatPath)
    {
        ExFreePoolWithTag(PathCopy, TAG_EXFAT_PATH);
        return NULL;
    }

    RtlCopyMemory(PathCopy, DirPath->Buffer, DirPath->Length);
    Victim->DirPath.Buffer = PathCopy;
    Victim->DirPath.Length = DirPath->Length;
    Victim->DirPath.MaximumLength = DirPath->Length;
    Victim->LastUsed = ++Vcb->DirIndexTick;

    /* One validated sweep; every later lookup here skips FatFs entirely. */
    ExFatAcquireFatFs(Vcb);
    Result = f_opendir(&Directory, FatPath);
    if (Result == FR_OK)
    {
        for (;;)
        {
            RtlZeroMemory(&Information, sizeof(Information));
            Result = f_readdir(&Directory, &Information);
            if (Result != FR_OK || !Information.fname[0])
                break;
            RtlInitUnicodeString(&NameString, Information.fname);
            if (!ExFatDirIndexAppend(Vcb,
                                     Victim,
                                     NameString.Buffer,
                                     NameString.Length,
                                     &Information,
                                     NULL))
            {
                Overflow = TRUE;
                break;
            }
        }
        f_closedir(&Directory);
    }
    ExFatReleaseFatFs(Vcb);
    ExFreePoolWithTag(FatPath, TAG_EXFAT_PATH);

    if (Overflow)
    {
        /* Keep the slot as a marker so we do not resweep on every miss. */
        if (Victim->Children)
            ExFreePoolWithTag(Victim->Children, TAG_EXFAT_FATFS);
        if (Victim->NamePool)
            ExFreePoolWithTag(Victim->NamePool, TAG_EXFAT_FATFS);
        Victim->Children = NULL;
        Victim->NamePool = NULL;
        Victim->Count = 0;
        Victim->Capacity = 0;
        Victim->PoolUsed = 0;
        Victim->PoolCapacity = 0;
        Victim->Unindexable = TRUE;
        return NULL;
    }
    if (Result != FR_OK)
    {
        ExFatResetDirIndex(Victim);
        return NULL;
    }
    ExFatBuildDirIndexHash(Victim);
    return Victim;
}

PEXFAT_DIR_CHILD
ExFatDirIndexLookup(
    PEXFAT_DIR_INDEX Index,
    PUNICODE_STRING LeafName)
{
    ULONGLONG Hash = ExFatHashPath(LeafName);
    UNICODE_STRING ChildName;
    PEXFAT_DIR_CHILD Child;
    ULONG Entry;
    ULONG Position;

    if (Index->HashBuckets)
        Entry = Index->HashBuckets[Hash & (Index->HashBucketCount - 1)];
    else
        Entry = Index->Count ? 1 : 0;

    while (Entry)
    {
        Position = Entry - 1;
        if (Position >= Index->Count)
            return NULL;
        Child = &Index->Children[Position];
        if (Child->NameHash == Hash && Child->NameLength == LeafName->Length)
        {
            ChildName.Buffer = Index->NamePool + Child->NameOffset;
            ChildName.Length = Child->NameLength;
            ChildName.MaximumLength = Child->NameLength;
            if (RtlEqualUnicodeString(&ChildName, LeafName, TRUE))
                return Child;
        }
        Entry = Index->HashBuckets ? Child->NextHash : Entry + 1;
        if (!Index->HashBuckets && Entry > Index->Count)
            Entry = 0;
    }
    return NULL;
}

VOID
ExFatDirIndexInsert(
    PEXFAT_VCB Vcb,
    PUNICODE_STRING ParentPath,
    PUNICODE_STRING LeafName,
    FILINFO* Information,
    PEXFAT_FCB Fcb)
{
    PEXFAT_DIR_INDEX Index;

    Index = ExFatFindDirIndex(Vcb, ParentPath);
    if (!Index || Index->Unindexable)
        return;
    if (ExFatDirIndexLookup(Index, LeafName))
        return;
    if (!ExFatDirIndexAppend(Vcb,
                             Index,
                             LeafName->Buffer,
                             LeafName->Length,
                             Information,
                             Fcb))
        ExFatResetDirIndex(Index);
}

VOID
ExFatDirIndexRemove(
    PEXFAT_VCB Vcb,
    PUNICODE_STRING PathName)
{
    UNICODE_STRING ParentPath;
    UNICODE_STRING LeafName;
    PEXFAT_DIR_INDEX DeletedIndex;
    PEXFAT_DIR_INDEX Index;
    PEXFAT_DIR_CHILD Child;

    DeletedIndex = ExFatFindDirIndex(Vcb, PathName);
    if (DeletedIndex)
        ExFatResetDirIndex(DeletedIndex);

    ExFatSplitPath(PathName, &ParentPath, &LeafName);
    if (!LeafName.Length)
        return;
    Index = ExFatFindDirIndex(Vcb, &ParentPath);
    if (!Index || Index->Unindexable)
        return;
    Child = ExFatDirIndexLookup(Index, &LeafName);
    if (!Child)
        return;
    /* The name-pool hole is left behind; it is reclaimed on eviction. */
    *Child = Index->Children[--Index->Count];
    ExFatFreeDirIndexHash(Index);
    ExFatBuildDirIndexHash(Index);
}

VOID
ExFatDirIndexUpdateFromFcb(
    PEXFAT_FCB Fcb)
{
    UNICODE_STRING ParentPath;
    UNICODE_STRING LeafName;
    PEXFAT_DIR_INDEX Index;
    PEXFAT_DIR_CHILD Child;

    if (Fcb->IsVolume || Fcb->DeletePending)
        return;
    ExFatSplitPath(&Fcb->PathName, &ParentPath, &LeafName);
    if (!LeafName.Length)
        return;
    Index = ExFatFindDirIndex(Fcb->Vcb, &ParentPath);
    if (!Index || Index->Unindexable)
        return;
    Child = ExFatDirIndexLookup(Index, &LeafName);
    if (!Child)
        return;

    Child->FileAttributes = Fcb->FileAttributes;
    Child->CreationTime = Fcb->CreationTime;
    Child->LastWriteTime = Fcb->LastWriteTime;
    Child->FileSize = Fcb->Header.FileSize;
    Child->AllocationSize = Fcb->Header.AllocationSize;
    Child->Attributes = ExFatNtAttributesToFat(Fcb->FileAttributes);
    if (Fcb->IsDirectory)
        Child->Attributes |= AM_DIR;
    ExFatSystemTimeToFatTime(&Fcb->LastWriteTime, &Child->ModDate, &Child->ModTime);
    ExFatSystemTimeToFatTime(&Fcb->CreationTime, &Child->CrtDate, &Child->CrtTime);
}

BOOLEAN
NTAPI
ExFatFastIoCheckIfPossible(
    PFILE_OBJECT FileObject,
    PLARGE_INTEGER FileOffset,
    ULONG Length,
    BOOLEAN Wait,
    ULONG LockKey,
    BOOLEAN CheckForReadOperation,
    PIO_STATUS_BLOCK IoStatus,
    PDEVICE_OBJECT DeviceObject)
{
    PEXFAT_FCB Fcb = FileObject->FsContext;
    PEXFAT_CCB Ccb = FileObject->FsContext2;
    LARGE_INTEGER LargeLength;

    UNREFERENCED_PARAMETER(Wait);
    UNREFERENCED_PARAMETER(IoStatus);
    UNREFERENCED_PARAMETER(DeviceObject);

    if (!Fcb || !Ccb || Ccb->CleanedUp ||
        Fcb->IsDirectory || Fcb->IsVolume || Fcb->DeletePending ||
        FileOffset->QuadPart < 0 || Length > MAXLONGLONG - FileOffset->QuadPart)
    {
        return FALSE;
    }

    LargeLength.QuadPart = Length;
    if (CheckForReadOperation)
    {
        if (!(Ccb->DesiredAccess & (FILE_READ_DATA | FILE_EXECUTE)))
            return FALSE;
        return FsRtlFastCheckLockForRead(&Fcb->FileLock,
                                         FileOffset,
                                         &LargeLength,
                                         LockKey,
                                         FileObject,
                                         PsGetCurrentProcess());
    }

    if (Fcb->Vcb->ReadOnly ||
        !(Ccb->DesiredAccess & (FILE_WRITE_DATA | FILE_APPEND_DATA)))
    {
        return FALSE;
    }
    return FsRtlFastCheckLockForWrite(&Fcb->FileLock,
                                      FileOffset,
                                      &LargeLength,
                                      LockKey,
                                      FileObject,
                                      PsGetCurrentProcess());
}

BOOLEAN
NTAPI
ExFatAcquireForLazyWrite(
    PVOID Context,
    BOOLEAN Wait)
{
    PEXFAT_FCB Fcb = Context;

    if (!ExAcquireResourceExclusiveLite(&Fcb->MainResource, Wait))
        return FALSE;
    ASSERT(IoGetTopLevelIrp() == NULL);
    IoSetTopLevelIrp((PIRP)FSRTL_CACHE_TOP_LEVEL_IRP);
    return TRUE;
}

VOID
NTAPI
ExFatReleaseFromLazyWrite(
    PVOID Context)
{
    PEXFAT_FCB Fcb = Context;

    ASSERT(IoGetTopLevelIrp() == (PIRP)FSRTL_CACHE_TOP_LEVEL_IRP);
    IoSetTopLevelIrp(NULL);
    ExReleaseResourceLite(&Fcb->MainResource);
}

BOOLEAN
NTAPI
ExFatAcquireForReadAhead(
    PVOID Context,
    BOOLEAN Wait)
{
    PEXFAT_FCB Fcb = Context;

    if (!ExAcquireResourceSharedLite(&Fcb->MainResource, Wait))
        return FALSE;
    ASSERT(IoGetTopLevelIrp() == NULL);
    IoSetTopLevelIrp((PIRP)FSRTL_CACHE_TOP_LEVEL_IRP);
    return TRUE;
}

VOID
NTAPI
ExFatReleaseFromReadAhead(
    PVOID Context)
{
    PEXFAT_FCB Fcb = Context;

    ASSERT(IoGetTopLevelIrp() == (PIRP)FSRTL_CACHE_TOP_LEVEL_IRP);
    IoSetTopLevelIrp(NULL);
    ExReleaseResourceLite(&Fcb->MainResource);
}

VOID
NTAPI
ExFatAcquireFileForNtCreateSection(
    PFILE_OBJECT FileObject)
{
    PEXFAT_FCB Fcb = FileObject->FsContext;

    KeEnterCriticalRegion();
    ExAcquireResourceExclusiveLite(&Fcb->MainResource, TRUE);
}

VOID
NTAPI
ExFatReleaseFileForNtCreateSection(
    PFILE_OBJECT FileObject)
{
    PEXFAT_FCB Fcb = FileObject->FsContext;

    ExReleaseResourceLite(&Fcb->MainResource);
    KeLeaveCriticalRegion();
}

DSTATUS
disk_initialize(
    BYTE PhysicalDrive)
{
    PEXFAT_VCB Vcb;

    if (!ExFatGlobalData || PhysicalDrive >= FF_VOLUMES)
        return STA_NOINIT;
    Vcb = ExFatGlobalData->Volumes[PhysicalDrive];
    if (!Vcb || !Vcb->Mounted)
        return STA_NOINIT;
    return Vcb->ReadOnly ? STA_PROTECT : 0;
}

DSTATUS
disk_status(
    BYTE PhysicalDrive)
{
    return disk_initialize(PhysicalDrive);
}

static VOID
ExFatInvalidateSectorCache(
    PEXFAT_VCB Vcb)
{
    ULONG Index;

    for (Index = 0; Index < Vcb->SectorCacheEntries; Index++)
    {
        Vcb->SectorCacheTags[Index] = EXFAT_SECTOR_CACHE_EMPTY;
        Vcb->SectorCacheDirty[Index] = FALSE;
    }
    Vcb->SectorCacheDirtyCount = 0;
}

static ULONG
ExFatSectorCacheSet(
    PEXFAT_VCB Vcb,
    LBA_t Block)
{
    ULONGLONG Hash = (ULONGLONG)Block;

    /* Fold high block-number bits before selecting a cache set. */
    Hash ^= Hash >> 32;
    Hash ^= Hash >> 16;
    Hash ^= Hash >> 8;
    return (ULONG)(Hash % Vcb->SectorCacheSets);
}

static BOOLEAN
ExFatFindSectorCacheSlot(
    PEXFAT_VCB Vcb,
    LBA_t Block,
    PULONG Slot)
{
    ULONG FirstSlot = ExFatSectorCacheSet(Vcb, Block) * EXFAT_SECTOR_CACHE_WAYS;
    ULONG Way;

    for (Way = 0; Way < EXFAT_SECTOR_CACHE_WAYS; Way++)
    {
        if (Vcb->SectorCacheTags[FirstSlot + Way] == Block)
        {
            *Slot = FirstSlot + Way;
            return TRUE;
        }
    }
    return FALSE;
}

static ULONG
ExFatSelectSectorCacheSlot(
    PEXFAT_VCB Vcb,
    LBA_t Block)
{
    ULONG Set = ExFatSectorCacheSet(Vcb, Block);
    ULONG FirstSlot = Set * EXFAT_SECTOR_CACHE_WAYS;
    ULONG Way;

    for (Way = 0; Way < EXFAT_SECTOR_CACHE_WAYS; Way++)
    {
        if (Vcb->SectorCacheTags[FirstSlot + Way] == EXFAT_SECTOR_CACHE_EMPTY)
            break;
    }
    if (Way == EXFAT_SECTOR_CACHE_WAYS)
        Way = Vcb->SectorCacheNextWay[Set];
    Vcb->SectorCacheNextWay[Set] = (UCHAR)((Way + 1) % EXFAT_SECTOR_CACHE_WAYS);
    return FirstSlot + Way;
}

static NTSTATUS
ExFatFlushSectorCacheSlot(
    PEXFAT_VCB Vcb,
    ULONG Slot)
{
    LARGE_INTEGER Offset;
    ULONG BlockBytes;

    if (!Vcb->SectorCacheDirty[Slot] ||
        Vcb->SectorCacheTags[Slot] == EXFAT_SECTOR_CACHE_EMPTY)
    {
        return STATUS_SUCCESS;
    }

    BlockBytes = Vcb->SectorCacheBlockSectors * Vcb->BytesPerSector;
    Offset.QuadPart = Vcb->SectorCacheTags[Slot] *
                      Vcb->SectorCacheBlockSectors *
                      Vcb->BytesPerSector;
    if (!NT_SUCCESS(ExFatPoolReadWriteDevice(Vcb->StorageDevice,
                                             IRP_MJ_WRITE,
                                             (PUCHAR)Vcb->SectorCacheBuffer +
                                                 (SIZE_T)Slot * BlockBytes,
                                             BlockBytes,
                                             &Offset,
                                             TRUE)))
    {
        return STATUS_IO_DEVICE_ERROR;
    }

    Vcb->SectorCacheDirty[Slot] = FALSE;
    NT_ASSERT(Vcb->SectorCacheDirtyCount != 0);
    Vcb->SectorCacheDirtyCount--;
    return STATUS_SUCCESS;
}

NTSTATUS
ExFatFlushSectorCache(
    PEXFAT_VCB Vcb)
{
    ULONG Slot;
    NTSTATUS Status;

    if (!Vcb->SectorCacheDirtyCount)
        return STATUS_SUCCESS;

    for (Slot = 0; Slot < Vcb->SectorCacheEntries; Slot++)
    {
        Status = ExFatFlushSectorCacheSlot(Vcb, Slot);
        if (!NT_SUCCESS(Status))
            return Status;
    }
    return STATUS_SUCCESS;
}

static NTSTATUS
ExFatFlushSectorCacheRange(
    PEXFAT_VCB Vcb,
    LBA_t Sector,
    UINT Count)
{
    LBA_t FirstBlock;
    LBA_t LastBlock;
    LBA_t Block;
    ULONG Slot;
    NTSTATUS Status;

    if (!Vcb->SectorCacheEntries || !Vcb->SectorCacheDirtyCount || !Count)
        return STATUS_SUCCESS;

    FirstBlock = Sector / Vcb->SectorCacheBlockSectors;
    LastBlock = (Sector + Count - 1) / Vcb->SectorCacheBlockSectors;
    if (LastBlock - FirstBlock >= Vcb->SectorCacheEntries)
        return ExFatFlushSectorCache(Vcb);

    for (Block = FirstBlock; Block <= LastBlock; Block++)
    {
        if (!ExFatFindSectorCacheSlot(Vcb, Block, &Slot))
            continue;
        Status = ExFatFlushSectorCacheSlot(Vcb, Slot);
        if (!NT_SUCCESS(Status))
            return Status;
    }
    return STATUS_SUCCESS;
}

static VOID
ExFatInvalidateSectorCacheRange(
    PEXFAT_VCB Vcb,
    LBA_t Sector,
    UINT Count)
{
    LBA_t FirstBlock;
    LBA_t LastBlock;
    LBA_t Block;
    ULONG Slot;

    if (!Vcb->SectorCacheEntries || !Count)
        return;

    FirstBlock = Sector / Vcb->SectorCacheBlockSectors;
    LastBlock = (Sector + Count - 1) / Vcb->SectorCacheBlockSectors;
    if (LastBlock - FirstBlock >= Vcb->SectorCacheEntries)
    {
        ExFatInvalidateSectorCache(Vcb);
        return;
    }

    for (Block = FirstBlock; Block <= LastBlock; Block++)
    {
        if (ExFatFindSectorCacheSlot(Vcb, Block, &Slot))
        {
            if (Vcb->SectorCacheDirty[Slot])
            {
                NT_ASSERT(Vcb->SectorCacheDirtyCount != 0);
                Vcb->SectorCacheDirtyCount--;
            }
            Vcb->SectorCacheTags[Slot] = EXFAT_SECTOR_CACHE_EMPTY;
            Vcb->SectorCacheDirty[Slot] = FALSE;
        }
    }
}

static BOOLEAN
ExFatEnsureSectorCache(
    PEXFAT_VCB Vcb)
{
    ULONG AlignmentMask;
    ULONG BlockSize;
    ULONG Blocks;
    ULONG CacheSize;
    ULONG DirtySize;
    ULONG ReplacementSize;
    ULONG TagsSize;
    ULONG Index;

    if (Vcb->SectorCacheBuffer)
        return TRUE;

    AlignmentMask = Vcb->StorageDevice->AlignmentRequirement;
    BlockSize = max(EXFAT_SECTOR_CACHE_BLOCK, Vcb->BytesPerSector);
    Blocks = EXFAT_SECTOR_CACHE_SIZE / BlockSize;
    Blocks -= Blocks % EXFAT_SECTOR_CACHE_WAYS;
    if (!Blocks)
        return FALSE;
    CacheSize = Blocks * BlockSize;
    TagsSize = Blocks * sizeof(LBA_t);
    DirtySize = Blocks * sizeof(*Vcb->SectorCacheDirty);
    ReplacementSize = Blocks / EXFAT_SECTOR_CACHE_WAYS;
    if (CacheSize > MAXULONG - AlignmentMask - TagsSize - DirtySize - ReplacementSize)
        return FALSE;

    Vcb->SectorCacheAllocation = ExAllocatePoolWithTag(NonPagedPool,
                                                       TagsSize + DirtySize + ReplacementSize +
                                                           CacheSize + AlignmentMask,
                                                       TAG_EXFAT_IO);
    if (!Vcb->SectorCacheAllocation)
        return FALSE;
    Vcb->SectorCacheTags = Vcb->SectorCacheAllocation;
    for (Index = 0; Index < Blocks; Index++)
        Vcb->SectorCacheTags[Index] = EXFAT_SECTOR_CACHE_EMPTY;
    Vcb->SectorCacheDirty = (PUCHAR)Vcb->SectorCacheAllocation + TagsSize;
    RtlZeroMemory(Vcb->SectorCacheDirty, DirtySize);
    Vcb->SectorCacheNextWay = Vcb->SectorCacheDirty + DirtySize;
    RtlZeroMemory(Vcb->SectorCacheNextWay, ReplacementSize);
    Vcb->SectorCacheBuffer = ALIGN_UP_POINTER_BY(Vcb->SectorCacheNextWay + ReplacementSize,
                                                 AlignmentMask + 1);
    Vcb->SectorCacheBlockSectors = BlockSize / Vcb->BytesPerSector;
    Vcb->SectorCacheEntries = Blocks;
    Vcb->SectorCacheSets = ReplacementSize;
    Vcb->SectorCacheDirtyCount = 0;
    return TRUE;
}

VOID
ExFatFreeSectorCache(
    PEXFAT_VCB Vcb)
{
    if (Vcb->SectorCacheAllocation)
        ExFreePoolWithTag(Vcb->SectorCacheAllocation, TAG_EXFAT_IO);
    Vcb->SectorCacheAllocation = NULL;
    Vcb->SectorCacheBuffer = NULL;
    Vcb->SectorCacheTags = NULL;
    Vcb->SectorCacheDirty = NULL;
    Vcb->SectorCacheNextWay = NULL;
    Vcb->SectorCacheEntries = 0;
    Vcb->SectorCacheSets = 0;
    Vcb->SectorCacheBlockSectors = 0;
    Vcb->SectorCacheDirtyCount = 0;
}

DRESULT
disk_read(
    BYTE PhysicalDrive,
    BYTE* Buffer,
    LBA_t Sector,
    UINT Count)
{
    PEXFAT_VCB Vcb;
    LARGE_INTEGER Offset;
    PUCHAR CacheBlock;
    LBA_t Block;
    ULONG Slot;
    ULONG BlockBytes;
    ULONG Length;

    if (!ExFatGlobalData || PhysicalDrive >= FF_VOLUMES || !Buffer || !Count)
        return RES_PARERR;
    Vcb = ExFatGlobalData->Volumes[PhysicalDrive];
    if (!Vcb || !Vcb->Mounted)
        return RES_NOTRDY;
    if (Sector >= Vcb->SectorCount || Count > Vcb->SectorCount - Sector ||
        Count > MAXULONG / Vcb->BytesPerSector)
    {
        return RES_PARERR;
    }

    if (Count == 1 && ExFatEnsureSectorCache(Vcb))
    {
        Block = Sector / Vcb->SectorCacheBlockSectors;
        if (!ExFatFindSectorCacheSlot(Vcb, Block, &Slot))
            Slot = ExFatSelectSectorCacheSlot(Vcb, Block);
        BlockBytes = Vcb->SectorCacheBlockSectors * Vcb->BytesPerSector;
        CacheBlock = (PUCHAR)Vcb->SectorCacheBuffer + (SIZE_T)Slot * BlockBytes;

        if (Vcb->SectorCacheTags[Slot] != Block)
        {
            LBA_t Base = Block * Vcb->SectorCacheBlockSectors;

            if (Vcb->SectorCount - Base < Vcb->SectorCacheBlockSectors)
                goto Uncached; /* Volume tail shorter than a block. */

            if (!NT_SUCCESS(ExFatFlushSectorCacheSlot(Vcb, Slot)))
                return RES_ERROR;
            Vcb->SectorCacheTags[Slot] = EXFAT_SECTOR_CACHE_EMPTY;
            Offset.QuadPart = Base * Vcb->BytesPerSector;
            if (!NT_SUCCESS(ExFatPoolReadWriteDevice(Vcb->StorageDevice,
                                                     IRP_MJ_READ,
                                                     CacheBlock,
                                                     BlockBytes,
                                                     &Offset,
                                                     TRUE)))
            {
                return RES_ERROR;
            }
            Vcb->SectorCacheTags[Slot] = Block;
        }

        RtlCopyMemory(Buffer,
                      CacheBlock + (ULONG)(Sector - Block * Vcb->SectorCacheBlockSectors) *
                          Vcb->BytesPerSector,
                      Vcb->BytesPerSector);
        return RES_OK;
    }

Uncached:
    Offset.QuadPart = Sector * Vcb->BytesPerSector;
    Length = Count * Vcb->BytesPerSector;
    return NT_SUCCESS(ExFatReadWriteDevice(Vcb->StorageDevice,
                                           IRP_MJ_READ,
                                           Buffer,
                                           Length,
                                           &Offset,
                                           TRUE)) ? RES_OK : RES_ERROR;
}

DRESULT
disk_write(
    BYTE PhysicalDrive,
    const BYTE* Buffer,
    LBA_t Sector,
    UINT Count)
{
    PEXFAT_VCB Vcb;
    LARGE_INTEGER Offset;
    PUCHAR CacheBlock;
    LBA_t Block;
    ULONG Slot;
    ULONG BlockBytes;
    ULONG Length;

    if (!ExFatGlobalData || PhysicalDrive >= FF_VOLUMES || !Buffer || !Count)
        return RES_PARERR;
    Vcb = ExFatGlobalData->Volumes[PhysicalDrive];
    if (!Vcb || !Vcb->Mounted)
        return RES_NOTRDY;
    if (Vcb->ReadOnly)
        return RES_WRPRT;
    if (Sector >= Vcb->SectorCount || Count > Vcb->SectorCount - Sector ||
        Count > MAXULONG / Vcb->BytesPerSector)
    {
        return RES_PARERR;
    }

    Offset.QuadPart = Sector * Vcb->BytesPerSector;
    Length = Count * Vcb->BytesPerSector;

    if (Count > 1)
    {
        if (!NT_SUCCESS(ExFatFlushSectorCacheRange(Vcb, Sector, Count)))
            return RES_ERROR;
        ExFatInvalidateSectorCacheRange(Vcb, Sector, Count);
        return NT_SUCCESS(ExFatReadWriteDevice(Vcb->StorageDevice,
                                               IRP_MJ_WRITE,
                                               (PVOID)Buffer,
                                               Length,
                                               &Offset,
                                               TRUE)) ? RES_OK : RES_ERROR;
    }

    if (ExFatEnsureSectorCache(Vcb))
    {
        Block = Sector / Vcb->SectorCacheBlockSectors;
        if (!ExFatFindSectorCacheSlot(Vcb, Block, &Slot))
        {
            LBA_t Base = Block * Vcb->SectorCacheBlockSectors;

            if (Vcb->SectorCount - Base < Vcb->SectorCacheBlockSectors)
                goto Uncached;
            Slot = ExFatSelectSectorCacheSlot(Vcb, Block);
            if (!NT_SUCCESS(ExFatFlushSectorCacheSlot(Vcb, Slot)))
                return RES_ERROR;
            Vcb->SectorCacheTags[Slot] = EXFAT_SECTOR_CACHE_EMPTY;
            BlockBytes = Vcb->SectorCacheBlockSectors * Vcb->BytesPerSector;
            CacheBlock = (PUCHAR)Vcb->SectorCacheBuffer + (SIZE_T)Slot * BlockBytes;
            Offset.QuadPart = Base * Vcb->BytesPerSector;
            if (!NT_SUCCESS(ExFatPoolReadWriteDevice(Vcb->StorageDevice,
                                                     IRP_MJ_READ,
                                                     CacheBlock,
                                                     BlockBytes,
                                                     &Offset,
                                                     TRUE)))
            {
                return RES_ERROR;
            }
            Vcb->SectorCacheTags[Slot] = Block;
        }

        RtlCopyMemory((PUCHAR)Vcb->SectorCacheBuffer +
                          (SIZE_T)Slot * Vcb->SectorCacheBlockSectors * Vcb->BytesPerSector +
                          (ULONG)(Sector - Block * Vcb->SectorCacheBlockSectors) *
                              Vcb->BytesPerSector,
                      Buffer,
                      Vcb->BytesPerSector);
        if (!Vcb->SectorCacheDirty[Slot])
        {
            Vcb->SectorCacheDirty[Slot] = TRUE;
            Vcb->SectorCacheDirtyCount++;
        }
        return RES_OK;
    }

Uncached:
    return NT_SUCCESS(ExFatReadWriteDevice(Vcb->StorageDevice,
                                           IRP_MJ_WRITE,
                                           (PVOID)Buffer,
                                           Length,
                                           &Offset,
                                           TRUE)) ? RES_OK : RES_ERROR;
}

DRESULT
disk_ioctl(
    BYTE PhysicalDrive,
    BYTE Command,
    void* Buffer)
{
    PEXFAT_VCB Vcb;

    if (!ExFatGlobalData || PhysicalDrive >= FF_VOLUMES)
        return RES_PARERR;
    Vcb = ExFatGlobalData->Volumes[PhysicalDrive];
    if (!Vcb || !Vcb->Mounted)
        return RES_NOTRDY;

    switch (Command)
    {
        case CTRL_SYNC:
            /*
             * FatFs raises this after every metadata update. Keep dirty blocks
             * in the filesystem cache until write-through, explicit flush,
             * eviction, volume lock, or shutdown.
             */
            return RES_OK;
        case GET_SECTOR_COUNT:
            if (!Buffer)
                return RES_PARERR;
            *(LBA_t*)Buffer = Vcb->SectorCount;
            return RES_OK;
        case GET_SECTOR_SIZE:
            if (!Buffer)
                return RES_PARERR;
            *(WORD*)Buffer = (WORD)Vcb->BytesPerSector;
            return RES_OK;
        case GET_BLOCK_SIZE:
            if (!Buffer)
                return RES_PARERR;
            *(DWORD*)Buffer = 1;
            return RES_OK;
        default:
            return RES_PARERR;
    }
}

DWORD
get_fattime(VOID)
{
    LARGE_INTEGER SystemTime;
    WORD Date;
    WORD Time;

    KeQuerySystemTime(&SystemTime);
    ExFatSystemTimeToFatTime(&SystemTime, &Date, &Time);
    return ((DWORD)Date << 16) | Time;
}

void*
ff_memalloc(
    UINT Size)
{
    PEXFAT_FATFS_ALLOCATION_HEADER Header;

    if (Size > MAXULONG - sizeof(*Header))
        return NULL;

    if (Size == EXFAT_FATFS_NAME_BUFFER_SIZE)
    {
        Header = ExAllocateFromNPagedLookasideList(&ExFatGlobalData->FatFsNameBufferLookaside);
        if (Header)
            Header->Fields.FromLookaside = TRUE;
    }
    else
    {
        Header = ExAllocatePoolWithTag(NonPagedPool,
                                       sizeof(*Header) + Size,
                                       TAG_EXFAT_FATFS);
        if (Header)
            Header->Fields.FromLookaside = FALSE;
    }

    if (!Header)
        return NULL;
    Header->Fields.Signature = EXFAT_FATFS_ALLOCATION_SIGNATURE;
    return Header + 1;
}

void
ff_memfree(
    void* Allocation)
{
    PEXFAT_FATFS_ALLOCATION_HEADER Header;

    if (!Allocation)
        return;

    Header = (PEXFAT_FATFS_ALLOCATION_HEADER)Allocation - 1;
    ASSERT(Header->Fields.Signature == EXFAT_FATFS_ALLOCATION_SIGNATURE);
    if (Header->Fields.FromLookaside)
        ExFreeToNPagedLookasideList(&ExFatGlobalData->FatFsNameBufferLookaside, Header);
    else
        ExFreePoolWithTag(Header, TAG_EXFAT_FATFS);
}
