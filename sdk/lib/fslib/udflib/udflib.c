/*
 * PROJECT: ReactOS UDF filesystem library
 * LICENSE: GPL-2.0-or-later
 * PURPOSE: Native NT handle/heap adapter for the BSD UDF 2.50 formatter.
 */
#include "compat.h"
#include "core/udf_write.h"
#include "core/newfs_udf.h"
#include <ntddstor.h>
#include <ntstrsafe.h>

/* The imported engine has global state. Serialize it and free every engine
 * allocation, including allocations left behind by a failed operation. */
typedef struct DECLSPEC_ALIGN(MEMORY_ALLOCATION_ALIGNMENT) _UDF_ALLOCATION
{
    LIST_ENTRY Link;
    SIZE_T Size;
} UDF_ALLOCATION;
static LIST_ENTRY Allocations;
static LONG Busy;
static HANDLE Volume;
static ULONGLONG VolumeSectors;
static ULONG Seed;
static NTSTATUS IoError;
static ULONGLONG LastSector;
static const char *FormatStage;
struct mmc_discinfo mmc_discinfo;
uint32_t udf_physical_alignment = 1;
uint32_t udf_physical_offset;
int fd, format_flags, media_accesstype, check_surface, wrtrack_skew;
int meta_perc = 5;
float meta_fract = 0.05f;
char *dev, *format_str;

/* A locked partition may still have a filesystem mounted on it. RawFS can
 * reject a flush, while UDFS rejects one on a dirty read-only mount even
 * though direct writes through the volume handle succeeded. In either case
 * flush the identified backing disk before reporting the new descriptors
 * complete. */
static NTSTATUS UdfFlushBackingDisk(HANDLE Partition)
{
    STORAGE_DEVICE_NUMBER Number;
    WCHAR DevicePath[64];
    UNICODE_STRING Name;
    OBJECT_ATTRIBUTES Attributes;
    IO_STATUS_BLOCK IoStatus;
    HANDLE Disk;
    NTSTATUS Status;

    Status = NtDeviceIoControlFile(Partition, NULL, NULL, NULL, &IoStatus,
                                   IOCTL_STORAGE_GET_DEVICE_NUMBER, NULL, 0,
                                   &Number, sizeof(Number));
    if (!NT_SUCCESS(Status)) return Status;
    if (Number.DeviceType != FILE_DEVICE_DISK || Number.DeviceNumber == MAXULONG)
        return STATUS_NOT_SUPPORTED;
    Status = RtlStringCchPrintfW(DevicePath, sizeof(DevicePath) / sizeof(WCHAR),
                                  L"\\Device\\Harddisk%lu\\Partition0", Number.DeviceNumber);
    if (!NT_SUCCESS(Status)) return Status;
    RtlInitUnicodeString(&Name, DevicePath);
    InitializeObjectAttributes(&Attributes, &Name, OBJ_CASE_INSENSITIVE, NULL, NULL);
    Status = NtOpenFile(&Disk, FILE_WRITE_DATA | SYNCHRONIZE, &Attributes, &IoStatus,
                        FILE_SHARE_READ | FILE_SHARE_WRITE, FILE_SYNCHRONOUS_IO_NONALERT);
    if (!NT_SUCCESS(Status)) return Status;
    Status = NtFlushBuffersFile(Disk, &IoStatus);
    NtClose(Disk);
    return Status;
}

void *udf_malloc(size_t Size)
{
    UDF_ALLOCATION *Allocation;
    if (Size > MAXULONG_PTR - sizeof(*Allocation))
        RtlRaiseStatus(STATUS_INTEGER_OVERFLOW);
    Allocation = RtlAllocateHeap(RtlGetProcessHeap(), 0, sizeof(*Allocation) + Size);
    if (!Allocation) RtlRaiseStatus(STATUS_NO_MEMORY);
    Allocation->Size = Size;
    InsertTailList(&Allocations, &Allocation->Link);
    return Allocation + 1;
}
void *udf_calloc(size_t Count, size_t Size)
{
    void *Buffer;
    if (Size && Count > MAXULONG_PTR / Size) RtlRaiseStatus(STATUS_INTEGER_OVERFLOW);
    Buffer = udf_malloc(Count * Size);
    memset(Buffer, 0, Count * Size);
    return Buffer;
}
void udf_free(void *Buffer)
{
    UDF_ALLOCATION *Allocation;
    if (!Buffer) return;
    Allocation = (UDF_ALLOCATION *)Buffer - 1;
    RemoveEntryList(&Allocation->Link);
    RtlFreeHeap(RtlGetProcessHeap(), 0, Allocation);
}
void *udf_realloc(void *Old, size_t Size)
{
    void *Buffer = udf_malloc(Size);
    if (Old)
    {
        UDF_ALLOCATION *Allocation = (UDF_ALLOCATION *)Old - 1;
        memcpy(Buffer, Old, min(Size, Allocation->Size));
        udf_free(Old);
    }
    return Buffer;
}
char *udf_strdup(const char *Text)
{
    size_t Length = strlen(Text) + 1;
    char *Copy = udf_malloc(Length);
    memcpy(Copy, Text, Length);
    return Copy;
}
int udf_printf(const char *Format, ...) { return 0; }
int udf_fprintf(void *Stream, const char *Format, ...) { return 0; }
void udf_errx(int Error, const char *Format, ...) { RtlRaiseStatus(STATUS_FILE_CORRUPT_ERROR); }
void udf_assert(const char *Expression, const char *File, unsigned Line)
{
    DbgPrint("UdfFormat: engine assertion %s (%s:%u)\n", Expression, File, Line);
    RtlRaiseStatus(STATUS_FILE_CORRUPT_ERROR);
}
long udf_random(void) { return RtlRandom(&Seed); }
time_t udf_time(time_t *Result)
{
    LARGE_INTEGER Now;
    ULONG Seconds;
    NtQuerySystemTime(&Now);
    RtlTimeToSecondsSince1970(&Now, &Seconds);
    if (Result) *Result = Seconds;
    return Seconds;
}
struct tm *udf_gmtime_r(const time_t *Seconds, struct tm *Result)
{
    LARGE_INTEGER Now;
    TIME_FIELDS Fields;
    RtlSecondsSince1970ToTime((ULONG)*Seconds, &Now);
    RtlTimeToTimeFields(&Now, &Fields);
    memset(Result, 0, sizeof(*Result));
    Result->tm_year = Fields.Year - 1900;
    Result->tm_mon = Fields.Month - 1;
    Result->tm_mday = Fields.Day;
    Result->tm_hour = Fields.Hour;
    Result->tm_min = Fields.Minute;
    Result->tm_sec = Fields.Second;
    return Result;
}
int udf_gettimeofday(struct timeval *Result, void *Unused)
{
    LARGE_INTEGER Now;
    ULONG Seconds;
    NtQuerySystemTime(&Now);
    RtlTimeToSecondsSince1970(&Now, &Seconds);
    Result->tv_sec = Seconds;
    Result->tv_usec = (Now.QuadPart % 10000000) / 10;
    return 0;
}

int udf_update_trackinfo(struct mmc_discinfo *Disc, struct mmc_trackinfo *Track)
{
    memset(Track, 0, sizeof(*Track));
    Track->tracknr = Track->sessionnr = 1;
    Track->packet_size = 32;
    Track->track_size = Disc->last_possible_lba;
    Track->next_writable = Track->last_recorded = Disc->last_possible_lba;
    return 0;
}
int udf_surface_check(void) { return EINVAL; }
int udf_write_sector(void *Data, uint64_t Sector)
{
    LARGE_INTEGER Offset;
    IO_STATUS_BLOCK IoStatus;
    if (Sector >= VolumeSectors) return EINVAL;
    LastSector = Sector;
    Offset.QuadPart = Sector * 512;
    IoError = NtWriteFile(Volume, NULL, NULL, NULL, &IoStatus, Data, 512, &Offset, NULL);
    if (!NT_SUCCESS(IoError)) return EIO;
    if (IoStatus.Information != 512) { IoError = STATUS_DEVICE_DATA_ERROR; return EIO; }
    return 0;
}

static void UdfQueryPhysicalAlignment(HANDLE Partition, ULONG LogicalSectorSize)
{
    IO_STATUS_BLOCK IoStatus;
    STORAGE_DEVICE_NUMBER DeviceNumber;
    STORAGE_PROPERTY_QUERY Query = { StorageAccessAlignmentProperty, PropertyStandardQuery, {0} };
    STORAGE_ACCESS_ALIGNMENT_DESCRIPTOR Alignment = {0};
    PARTITION_INFORMATION_EX PartitionInfo;
    ULONG AlignmentSectors, PartitionOffset;
    NTSTATUS Status;

    udf_physical_alignment = 1;
    udf_physical_offset = 0;

    if (!NT_SUCCESS(NtDeviceIoControlFile(Partition, NULL, NULL, NULL, &IoStatus,
                                          IOCTL_STORAGE_GET_DEVICE_NUMBER, NULL, 0,
                                          &DeviceNumber, sizeof(DeviceNumber))) ||
        DeviceNumber.DeviceType != FILE_DEVICE_DISK)
        return;

    Status = NtDeviceIoControlFile(Partition, NULL, NULL, NULL, &IoStatus,
                                   IOCTL_STORAGE_QUERY_PROPERTY, &Query, sizeof(Query),
                                   &Alignment, sizeof(Alignment));
    if (!NT_SUCCESS(Status) || IoStatus.Information < sizeof(Alignment) ||
        Alignment.Size < sizeof(Alignment) ||
        Alignment.BytesPerLogicalSector != LogicalSectorSize ||
        Alignment.BytesPerPhysicalSector < LogicalSectorSize ||
        Alignment.BytesPerPhysicalSector % LogicalSectorSize ||
        Alignment.BytesOffsetForSectorAlignment >= Alignment.BytesPerPhysicalSector ||
        Alignment.BytesOffsetForSectorAlignment % LogicalSectorSize)
        return;

    AlignmentSectors = Alignment.BytesPerPhysicalSector / LogicalSectorSize;
    Status = NtDeviceIoControlFile(Partition, NULL, NULL, NULL, &IoStatus,
                                   IOCTL_DISK_GET_PARTITION_INFO_EX, NULL, 0,
                                   &PartitionInfo, sizeof(PartitionInfo));
    if (!AlignmentSectors || !NT_SUCCESS(Status) ||
        IoStatus.Information < sizeof(PartitionInfo) ||
        PartitionInfo.StartingOffset.QuadPart < 0 ||
        PartitionInfo.StartingOffset.QuadPart % LogicalSectorSize)
        return;

    PartitionOffset = (ULONG)((PartitionInfo.StartingOffset.QuadPart / LogicalSectorSize) %
                              AlignmentSectors);
    udf_physical_alignment = AlignmentSectors;
    udf_physical_offset = (ULONG)(((ULONGLONG)PartitionOffset +
        Alignment.BytesOffsetForSectorAlignment / LogicalSectorSize) % AlignmentSectors);
}

static void Progress(PFMIFSCALLBACK Callback, ULONG Percent)
{
    if (Callback && !Callback(PROGRESS, 0, &Percent)) RtlRaiseStatus(STATUS_CANCELLED);
}

BOOLEAN NTAPI UdfFormat(PUNICODE_STRING DriveRoot, PFMIFSCALLBACK Callback,
                       BOOLEAN QuickFormat, BOOLEAN BackwardCompatible,
                       MEDIA_TYPE MediaType, PUNICODE_STRING Label, ULONG ClusterSize)
{
    OBJECT_ATTRIBUTES Attributes;
    IO_STATUS_BLOCK IoStatus;
    GET_LENGTH_INFORMATION Length;
    DISK_GEOMETRY Geometry;
    NTSTATUS Status;
    BOOLEAN Success = FALSE, Locked = FALSE;
    WCHAR LabelBuffer[64];
    char Utf8Label[128];
    ULONG Written;
    int Error = 0;

    if (ClusterSize && ClusterSize != 512) return FALSE;
    if (MediaType != FixedMedia && MediaType != RemovableMedia) return FALSE;
    if (InterlockedCompareExchange(&Busy, 1, 0)) return FALSE;
    udf_physical_alignment = 1;
    udf_physical_offset = 0;
    FormatStage = "open";
    IoError = STATUS_SUCCESS;
    LastSector = 0;
    InitializeListHead(&Allocations);
    Volume = NULL;
    InitializeObjectAttributes(&Attributes, DriveRoot, OBJ_CASE_INSENSITIVE, NULL, NULL);
    Status = NtOpenFile(&Volume, FILE_GENERIC_READ | FILE_GENERIC_WRITE | SYNCHRONIZE,
                        &Attributes, &IoStatus, FILE_SHARE_READ | FILE_SHARE_WRITE,
                        FILE_SYNCHRONOUS_IO_NONALERT);
    if (!NT_SUCCESS(Status)) goto Done;
    Status = NtDeviceIoControlFile(Volume, NULL, NULL, NULL, &IoStatus,
                                   IOCTL_DISK_GET_LENGTH_INFO, NULL, 0, &Length, sizeof(Length));
    if (!NT_SUCCESS(Status)) goto Done;
    Status = NtDeviceIoControlFile(Volume, NULL, NULL, NULL, &IoStatus,
                                   IOCTL_DISK_GET_DRIVE_GEOMETRY, NULL, 0, &Geometry, sizeof(Geometry));
    if (!NT_SUCCESS(Status) || Geometry.BytesPerSector != 512 ||
        Length.Length.QuadPart < 32LL * 1024 * 1024 ||
        Length.Length.QuadPart % 512 || (ULONGLONG)Length.Length.QuadPart / 512 > MAXULONG)
    { Status = STATUS_NOT_SUPPORTED; goto Done; }
    VolumeSectors = Length.Length.QuadPart / 512;
    UdfQueryPhysicalAlignment(Volume, Geometry.BytesPerSector);
    Status = NtFsControlFile(Volume, NULL, NULL, NULL, &IoStatus,
                             FSCTL_LOCK_VOLUME, NULL, 0, NULL, 0);
    if (!NT_SUCCESS(Status)) goto Done;
    Locked = TRUE;
    _SEH2_TRY
    {
        Progress(Callback, 0);
        if (!QuickFormat)
        {
            const ULONG Chunk = 1024 * 1024;
            void *Zero = udf_calloc(1, Chunk);
            ULONGLONG Position;
            LARGE_INTEGER Offset;
            for (Position = 0; Position < (ULONGLONG)Length.Length.QuadPart; Position += Chunk)
            {
                ULONG Count = min((ULONGLONG)Chunk, (ULONGLONG)Length.Length.QuadPart - Position);
                Offset.QuadPart = Position;
                Status = NtWriteFile(Volume, NULL, NULL, NULL, &IoStatus, Zero, Count, &Offset, NULL);
                if (!NT_SUCCESS(Status)) RtlRaiseStatus(Status);
                if (IoStatus.Information != Count) RtlRaiseStatus(STATUS_DEVICE_DATA_ERROR);
                Progress(Callback, (ULONG)(Position * 80 / Length.Length.QuadPart));
            }
            udf_free(Zero);
        }
        udf_init_create_context();
        memset(&layout, 0, sizeof(layout));
        memset(&mmc_discinfo, 0, sizeof(mmc_discinfo));
        context.min_udf = context.max_udf = 0x250;
        context.app_name = "*ReactOS Setup";
        context.impl_name = "*ReactOS UDF formatter";
        context.gmtoff = 0;
        if (Label && Label->Length)
        {
            if (Label->Length > sizeof(LabelBuffer) - sizeof(WCHAR))
                RtlRaiseStatus(STATUS_INVALID_VOLUME_LABEL);
            memcpy(LabelBuffer, Label->Buffer, Label->Length);
            LabelBuffer[Label->Length / sizeof(WCHAR)] = 0;
            Written = 0;
            for (ULONG Index = 0; Index < Label->Length / sizeof(WCHAR); ++Index)
            {
                ULONG Character = LabelBuffer[Index];
                ULONG Bytes = Character < 0x80 ? 1 : (Character < 0x800 ? 2 : 3);
                if (!Character || (Character >= 0xd800 && Character <= 0xdfff) ||
                    Character == 0xfffe || Character == 0xfeff ||
                    Written + Bytes >= sizeof(Utf8Label))
                    RtlRaiseStatus(STATUS_INVALID_VOLUME_LABEL);
                if (Bytes == 3) Utf8Label[Written++] = 0xe0 | (Character >> 12);
                if (Bytes >= 2) Utf8Label[Written++] = (Bytes == 2 ? 0xc0 : 0x80) | ((Character >> 6) & (Bytes == 2 ? 0x1f : 0x3f));
                Utf8Label[Written++] = (Bytes == 1 ? Character : 0x80 | (Character & 0x3f));
            }
            Utf8Label[Written] = 0;
            context.logvol_name = udf_strdup(Utf8Label);
        }
        else context.logvol_name = udf_strdup("ReactOS");
        context.fileset_name = udf_strdup("ReactOS");
        mmc_discinfo.mmc_class = MMC_CLASS_DISC;
        mmc_discinfo.mmc_cur = MMC_CAP_RECORDABLE | MMC_CAP_REWRITABLE | MMC_CAP_HW_DEFECTFREE;
        mmc_discinfo.disc_state = MMC_STATE_CLOSED;
        mmc_discinfo.last_session_state = MMC_STATE_CLOSED;
        mmc_discinfo.num_sessions = mmc_discinfo.num_tracks = 1;
        mmc_discinfo.first_track = mmc_discinfo.first_track_last_session = mmc_discinfo.last_track_last_session = 1;
        mmc_discinfo.sector_size = 512;
        mmc_discinfo.last_possible_lba = VolumeSectors - 1;
        Seed = udf_time(NULL) ^ (ULONG)VolumeSectors;
        IoError = STATUS_SUCCESS;
        FormatStage = "volume identifiers";
        Error = udf_proces_names();
        if (!Error) { FormatStage = "format profile"; Error = udf_derive_format(0, 0, 1); }
        if (!Error) { FormatStage = "descriptor prefix"; Error = udf_do_newfs_prefix(); }
        Progress(Callback, 85);
        if (!Error) { FormatStage = "root directory"; Error = udf_do_rootdir(); }
        if (!Error) { FormatStage = "descriptor postfix"; Error = udf_do_newfs_postfix(); }
        if (Error) RtlRaiseStatus(NT_SUCCESS(IoError) ? STATUS_UNSUCCESSFUL : IoError);
        FormatStage = "flush";
        Status = NtFlushBuffersFile(Volume, &IoStatus);
        if (Status == STATUS_INVALID_DEVICE_REQUEST ||
            Status == STATUS_VOLUME_DIRTY)
        {
            FormatStage = "backing disk flush";
            Status = UdfFlushBackingDisk(Volume);
        }
        if (!NT_SUCCESS(Status)) RtlRaiseStatus(Status);
        Success = TRUE;
        Progress(Callback, 100);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
        Success = FALSE;
    }
    _SEH2_END;
Done:
    while (!IsListEmpty(&Allocations))
    {
        UDF_ALLOCATION *Allocation = CONTAINING_RECORD(RemoveHeadList(&Allocations), UDF_ALLOCATION, Link);
        RtlFreeHeap(RtlGetProcessHeap(), 0, Allocation);
    }
    if (Volume)
    {
        if (Locked)
        {
            /* Keep the locked file object valid until all writes and the final
             * device flush complete. Dismount afterwards so a new open probes
             * the new filesystem, as in the existing native format providers. */
            NTSTATUS Dismount = NtFsControlFile(Volume, NULL, NULL, NULL, &IoStatus,
                                                FSCTL_DISMOUNT_VOLUME, NULL, 0, NULL, 0);
            if (Success && !NT_SUCCESS(Dismount))
            {
                FormatStage = "dismount";
                Status = Dismount;
                Success = FALSE;
            }
            NtFsControlFile(Volume, NULL, NULL, NULL, &IoStatus, FSCTL_UNLOCK_VOLUME, NULL, 0, NULL, 0);
        }
        NtClose(Volume);
    }
    InterlockedExchange(&Busy, 0);
    if (!Success)
    {
        char Message[192];
        TEXTOUTPUT Output;
        _snprintf(Message, sizeof(Message), "UDF format failed at %s: status=%08lx engine=%d I/O=%08lx sector=%I64u\n",
                  FormatStage, Status, Error, IoError, LastSector);
        Message[sizeof(Message) - 1] = 0;
        Output.Lines = 1;
        Output.Output = Message;
        if (Callback) Callback(OUTPUT, 0, &Output);
        DbgPrint("%s", Message);
    }
    if (Callback) Callback(DONE, 0, &Success);
    return Success;
}
