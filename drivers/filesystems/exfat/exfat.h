/*
 * PROJECT:     ReactOS exFAT filesystem driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Driver-wide definitions
 * COPYRIGHT:   Copyright 2026 Ahmed ARIF <arif.ing@outlook.com>
 */

#pragma once

#include <ntifs.h>
#include <ntdddisk.h>
#include <pseh/pseh2.h>
#include <section_attribs.h>

#include <ff.h>
#include <diskio.h>

#define EXFAT_DEVICE_NAME L"\\ExFat"

#define TAG_EXFAT_FCB    'FfxE'
#define TAG_EXFAT_CCB    'CfxE'
#define TAG_EXFAT_PATH   'PfxE'
#define TAG_EXFAT_FATFS  'LfxE'
#define TAG_EXFAT_IO     'IfxE'

#define EXFAT_READ_AHEAD_GRANULARITY (64 * 1024)
#define EXFAT_FCB_CACHE_LIMIT        128
#define EXFAT_NEGATIVE_CACHE_SIZE    64
#define EXFAT_DIR_INDEX_SLOTS        4
#define EXFAT_DIR_INDEX_MAX_NAMES    16384
#define EXFAT_FATFS_NAME_BUFFER_SIZE \
    (((FF_MAX_LFN + 1) * sizeof(WCHAR)) + (((FF_MAX_LFN + 44U) / 15) * 32))
#define EXFAT_FATFS_ALLOCATION_SIGNATURE 'afxE'

typedef union _EXFAT_FATFS_ALLOCATION_HEADER
{
    struct
    {
        ULONG Signature;
        BOOLEAN FromLookaside;
    } Fields;
    ULONG_PTR Alignment[2];
} EXFAT_FATFS_ALLOCATION_HEADER, *PEXFAT_FATFS_ALLOCATION_HEADER;

#define EXFAT_FCB_SIGNATURE 0x5846

#define EXFAT_NAME_OFFSET           3
#define EXFAT_NAME_LENGTH           8
#define EXFAT_SECTOR_SHIFT_OFFSET   108
#define EXFAT_BOOT_SIGNATURE_OFFSET 510
#define EXFAT_MIN_SECTOR_SHIFT      9
#define EXFAT_MAX_SECTOR_SHIFT      12

typedef struct _EXFAT_VCB EXFAT_VCB, *PEXFAT_VCB;

typedef struct _EXFAT_NEGATIVE_ENTRY
{
    UNICODE_STRING PathName;
    ULONG Generation;
    FRESULT Result;
} EXFAT_NEGATIVE_ENTRY, *PEXFAT_NEGATIVE_ENTRY;

typedef struct _EXFAT_DIR_CHILD
{
    ULONGLONG NameHash;
    ULONG NextHash;
    ULONG NameOffset;
    USHORT NameLength;
    ULONG FileAttributes;
    LARGE_INTEGER CreationTime;
    LARGE_INTEGER LastWriteTime;
    LARGE_INTEGER FileSize;
    LARGE_INTEGER AllocationSize;
    BYTE Attributes;
    WORD ModDate;
    WORD ModTime;
    WORD CrtDate;
    WORD CrtTime;
} EXFAT_DIR_CHILD, *PEXFAT_DIR_CHILD;

/*
 * Complete child listing of one directory, built with a single validated
 * f_readdir() sweep; makes every subsequent lookup in that directory,
 * positive or negative, a memory hit instead of a FatFs directory scan.
 */
typedef struct _EXFAT_DIR_INDEX
{
    UNICODE_STRING DirPath;
    PEXFAT_DIR_CHILD Children;
    PULONG HashBuckets;
    PWCHAR NamePool;
    ULONG Count;
    ULONG Capacity;
    ULONG HashBucketCount;
    ULONG PoolUsed;
    ULONG PoolCapacity;
    ULONG LastUsed;
    BOOLEAN Unindexable;
} EXFAT_DIR_INDEX, *PEXFAT_DIR_INDEX;

typedef struct _EXFAT_FCB
{
    FSRTL_COMMON_FCB_HEADER Header;
    SECTION_OBJECT_POINTERS SectionObjectPointers;
    ERESOURCE MainResource;
    ERESOURCE PagingIoResource;
    FILE_LOCK FileLock;
    SHARE_ACCESS ShareAccess;
    LIST_ENTRY ListEntry;
    PEXFAT_VCB Vcb;
    UNICODE_STRING PathName;
    TCHAR* FatPath;
    LARGE_INTEGER CreationTime;
    LARGE_INTEGER LastAccessTime;
    LARGE_INTEGER LastWriteTime;
    LARGE_INTEGER ChangeTime;
    ULONGLONG IndexNumber;
    ULONG FileAttributes;
    LONG ReferenceCount;
    ULONG OpenHandleCount;
    FIL FatFile;
    PDWORD ClusterMap;
    BOOLEAN FatFileOpen;
    BOOLEAN FatFileWritable;
    BOOLEAN IsDirectory;
    BOOLEAN IsVolume;
    BOOLEAN DeletePending;
    BOOLEAN DeleteCompleted;
} EXFAT_FCB, *PEXFAT_FCB;

typedef struct _EXFAT_CCB
{
    PFILE_OBJECT FileObject;
    ACCESS_MASK DesiredAccess;
    BOOLEAN HandleOpen;
    BOOLEAN ShareAccessSet;
    BOOLEAN CleanedUp;
    ULONG DirectoryIndex;
    BOOLEAN DirectoryIndexBacked;
    UNICODE_STRING SearchPattern;
    DIR Directory;
} EXFAT_CCB, *PEXFAT_CCB;

struct _EXFAT_VCB
{
    PDEVICE_OBJECT DeviceObject;
    PDEVICE_OBJECT StorageDevice;
    PVPB Vpb;
    FATFS FileSystem;
    BYTE DriveNumber;
    ULONG BytesPerSector;
    ULONG BytesPerCluster;
    ULONGLONG SectorCount;
    ULONG SerialNumber;
    BOOLEAN ReadOnly;
    BOOLEAN Mounted;
    BOOLEAN Locked;
    PFILE_OBJECT LockOwner;
    ERESOURCE Resource;
    ERESOURCE FatFsResource;
    LIST_ENTRY FcbListHead;
    ULONG CachedFcbCount;
    ULONG NamespaceGeneration;
    EXFAT_NEGATIVE_ENTRY NegativeCache[EXFAT_NEGATIVE_CACHE_SIZE];
    EXFAT_DIR_INDEX DirIndexes[EXFAT_DIR_INDEX_SLOTS];
    ULONG DirIndexTick;
    LONG OpenHandleCount;
    PEXFAT_FCB VolumeFcb;
    PFILE_OBJECT StreamFileObject;
    PVOID SectorCacheAllocation;
    PVOID SectorCacheBuffer;
    LBA_t* SectorCacheTags;
    PUCHAR SectorCacheDirty;
    PUCHAR SectorCacheNextWay;
    ULONG SectorCacheEntries;
    ULONG SectorCacheSets;
    ULONG SectorCacheBlockSectors;
    ULONG SectorCacheDirtyCount;
    PVOID ZeroBuffer;
};

typedef struct _EXFAT_GLOBAL_DATA
{
    PDRIVER_OBJECT DriverObject;
    PDEVICE_OBJECT DeviceObject;
    ERESOURCE VolumeListResource;
    NPAGED_LOOKASIDE_LIST CcbLookaside;
    NPAGED_LOOKASIDE_LIST FatFsNameBufferLookaside;
    CACHE_MANAGER_CALLBACKS CacheManagerCallbacks;
    FAST_IO_DISPATCH FastIoDispatch;
    PEXFAT_VCB Volumes[FF_VOLUMES];
} EXFAT_GLOBAL_DATA, *PEXFAT_GLOBAL_DATA;

extern PEXFAT_GLOBAL_DATA ExFatGlobalData;

DRIVER_INITIALIZE DriverEntry;
DRIVER_DISPATCH ExFatFsdDispatch;

NTSTATUS ExFatCreate(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS ExFatClose(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS ExFatCleanup(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS ExFatRead(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS ExFatWrite(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS ExFatQueryInformation(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS ExFatSetInformation(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS ExFatQueryVolumeInformation(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS ExFatSetVolumeInformation(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS ExFatDirectoryControl(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS ExFatFileSystemControl(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS ExFatFlushBuffers(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS ExFatLockControl(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS ExFatShutdown(PDEVICE_OBJECT DeviceObject, PIRP Irp);

NTSTATUS ExFatMapResult(FRESULT Result);
PVOID ExFatGetUserBuffer(PIRP Irp, BOOLEAN PagingIo);
NTSTATUS ExFatLockUserBuffer(PIRP Irp, ULONG Length, LOCK_OPERATION Operation);
NTSTATUS ExFatReadWriteDevice(PDEVICE_OBJECT DeviceObject, UCHAR MajorFunction, PVOID Buffer, ULONG Length, PLARGE_INTEGER Offset, BOOLEAN OverrideVerify);
NTSTATUS ExFatRawWriteDevice(PEXFAT_VCB Vcb, PVOID Buffer, ULONG Length, PLARGE_INTEGER Offset);
NTSTATUS ExFatFlushStorageDevice(PEXFAT_VCB Vcb);
NTSTATUS ExFatFlushSectorCache(PEXFAT_VCB Vcb);
VOID ExFatFreeSectorCache(PEXFAT_VCB Vcb);
NTSTATUS ExFatDeviceIoControl(PDEVICE_OBJECT DeviceObject, ULONG ControlCode, PVOID InputBuffer, ULONG InputLength, PVOID OutputBuffer, PULONG OutputLength);
VOID ExFatBuildDrivePath(PEXFAT_VCB Vcb, TCHAR Path[3]);
TCHAR* ExFatBuildFatPath(PEXFAT_VCB Vcb, PUNICODE_STRING PathName);
NTSTATUS ExFatBuildFullPath(PFILE_OBJECT FileObject, PUNICODE_STRING FullPath, PWCHAR PathBuffer, USHORT PathBufferSize, PULONGLONG PathHash);
VOID ExFatFreeUnicodeString(PUNICODE_STRING String);
ULONG ExFatFatAttributesToNt(BYTE Attributes);
BYTE ExFatNtAttributesToFat(ULONG Attributes);
LARGE_INTEGER ExFatFatTimeToSystemTime(WORD Date, WORD Time);
VOID ExFatSystemTimeToFatTime(PLARGE_INTEGER SystemTime, PWORD Date, PWORD Time);
ULONGLONG ExFatRoundUp(ULONGLONG Value, ULONG Alignment);

PEXFAT_FCB ExFatCreateFcb(PEXFAT_VCB Vcb, PUNICODE_STRING PathName, FILINFO* Information, BOOLEAN IsVolume);
PEXFAT_FCB ExFatFindFcb(PEXFAT_VCB Vcb, PUNICODE_STRING PathName, ULONGLONG PathHash);
VOID ExFatReferenceFcb(PEXFAT_FCB Fcb);
VOID ExFatDereferenceFcb(PEXFAT_FCB Fcb);
VOID ExFatPurgeCachedFcbs(PEXFAT_VCB Vcb);
BOOLEAN ExFatLookupNegative(PEXFAT_VCB Vcb, PUNICODE_STRING PathName, ULONGLONG PathHash, FRESULT* Result);
VOID ExFatRememberNegative(PEXFAT_VCB Vcb, PUNICODE_STRING PathName, ULONGLONG PathHash, FRESULT Result);
VOID ExFatSplitPath(PUNICODE_STRING FullPath, PUNICODE_STRING ParentPath, PUNICODE_STRING LeafName);
PEXFAT_DIR_INDEX ExFatEnsureDirIndex(PEXFAT_VCB Vcb, PUNICODE_STRING DirPath);
PEXFAT_DIR_INDEX ExFatLookupDirIndex(PEXFAT_VCB Vcb, PUNICODE_STRING DirPath);
PEXFAT_DIR_CHILD ExFatDirIndexLookup(PEXFAT_DIR_INDEX Index, PUNICODE_STRING LeafName);
VOID ExFatDirIndexInsert(PEXFAT_VCB Vcb, PUNICODE_STRING ParentPath, PUNICODE_STRING LeafName, FILINFO* Information, PEXFAT_FCB Fcb);
VOID ExFatDirIndexRemove(PEXFAT_VCB Vcb, PUNICODE_STRING PathName);
VOID ExFatDirIndexUpdateFromFcb(PEXFAT_FCB Fcb);
VOID ExFatDropDirIndexes(PEXFAT_VCB Vcb);
VOID ExFatUpdateFcbFromInfo(PEXFAT_FCB Fcb, FILINFO* Information);
ULONGLONG ExFatHashPath(PUNICODE_STRING PathName);

VOID ExFatAcquireFatFs(PEXFAT_VCB Vcb);
VOID ExFatReleaseFatFs(PEXFAT_VCB Vcb);
FRESULT ExFatEnsureFcbFile(PEXFAT_FCB Fcb, BOOLEAN WriteAccess);
FRESULT ExFatCloseFcbFile(PEXFAT_FCB Fcb);
FRESULT ExFatSeekFcbFile(PEXFAT_FCB Fcb, FSIZE_t Offset);
FRESULT ExFatZeroFileRange(PEXFAT_FCB Fcb, FSIZE_t Start, FSIZE_t End);
BOOLEAN ExFatFileIsContiguous(PEXFAT_FCB Fcb);
BOOLEAN ExFatDirectFileIo(PEXFAT_VCB Vcb, PEXFAT_FCB Fcb, UCHAR MajorFunction, PIRP OriginalIrp, PVOID Buffer, PMDL SourceMdl, ULONG MdlOffset, ULONGLONG ByteOffset, ULONG Length);
VOID ExFatInvalidateFcbClusterMap(PEXFAT_FCB Fcb);

BOOLEAN NTAPI ExFatAcquireForLazyWrite(PVOID Context, BOOLEAN Wait);
VOID NTAPI ExFatReleaseFromLazyWrite(PVOID Context);
BOOLEAN NTAPI ExFatAcquireForReadAhead(PVOID Context, BOOLEAN Wait);
VOID NTAPI ExFatReleaseFromReadAhead(PVOID Context);

BOOLEAN NTAPI ExFatFastIoCheckIfPossible(PFILE_OBJECT FileObject, PLARGE_INTEGER FileOffset, ULONG Length, BOOLEAN Wait, ULONG LockKey, BOOLEAN CheckForReadOperation, PIO_STATUS_BLOCK IoStatus, PDEVICE_OBJECT DeviceObject);
VOID NTAPI ExFatAcquireFileForNtCreateSection(PFILE_OBJECT FileObject);
VOID NTAPI ExFatReleaseFileForNtCreateSection(PFILE_OBJECT FileObject);

#define ExFatIsWriteAccess(Access) \
    BooleanFlagOn((Access), FILE_WRITE_DATA | FILE_APPEND_DATA | FILE_WRITE_EA | FILE_WRITE_ATTRIBUTES | DELETE)
