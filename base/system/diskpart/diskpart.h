/*
 * PROJECT:         ReactOS DiskPart
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            base/system/diskpart/diskpart.h
 * PURPOSE:         Manages all the partitions of the OS in an interactive way.
 * PROGRAMMERS:     Lee Schroeder
 */

#ifndef DISKPART_H
#define DISKPART_H

/* INCLUDES ******************************************************************/

#include <stdio.h>
#include <stdlib.h>

#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <winnls.h>
#include <winreg.h>
#include <wincon.h>
#include <winioctl.h>
#include <winuser.h>
#include <ntsecapi.h>

#include <errno.h>
#include <strsafe.h>

#include <conutils.h>

/*
#define NTOS_MODE_USER
#include <ndk/exfuncs.h>
#include <ndk/iofuncs.h>
#include <ndk/obfuncs.h>
#include <ndk/psfuncs.h>
#include <ndk/rtlfuncs.h>
#include <ndk/umfuncs.h>
*/

#define NTOS_MODE_USER
#include <ndk/cmfuncs.h>
#include <ndk/exfuncs.h>
#include <ndk/iofuncs.h>
#include <ndk/kefuncs.h>
#include <ndk/mmfuncs.h>
#include <ndk/obfuncs.h>
#include <ndk/psfuncs.h>
#include <ndk/rtlfuncs.h>
#include <ndk/setypes.h>
#include <ndk/umfuncs.h>

#include <ntddscsi.h>
#include <ntddstor.h>
#include <mountmgr.h>

#include <fmifs/fmifs.h>
#include <guiddef.h>
#include <diskguid.h>

#include "resource.h"

//#define DUMP_PARTITION_TABLE
//#define DUMP_PARTITION_LIST

/* DEFINES *******************************************************************/

typedef struct _COMMAND
{
    PWSTR cmd1;
    PWSTR cmd2;
    PWSTR cmd3;
    BOOL (*func)(INT, WCHAR**);
    INT help;
    DWORD help_detail;
} COMMAND, *PCOMMAND;

extern COMMAND cmds[];

/* NOERR codes for the program */
//#define ERROR_NONE      0
//#define ERROR_FATAL     1
//#define ERROR_CMD_ARG   2
//#define ERROR_FILE      3
//#define ERROR_SERVICE   4
//#define ERROR_SYNTAX    5

#define MAX_STRING_SIZE 1024
#define MAX_ARGS_COUNT 256


typedef enum _FORMATSTATE
{
    Unformatted,
    UnformattedOrDamaged,
    UnknownFormat,
    Preformatted,
    Formatted
} FORMATSTATE, *PFORMATSTATE;

typedef enum _VOLUME_TYPE
{
    VOLUME_TYPE_CDROM,
    VOLUME_TYPE_PARTITION,
    VOLUME_TYPE_REMOVABLE,
    VOLUME_TYPE_UNKNOWN
} VOLUME_TYPE, *PVOLUME_TYPE;

typedef struct _MBR_PARTITION_DATA
{
    BOOLEAN BootIndicator;
    UCHAR PartitionType;
} MBR_PARTITION_DATA, *PMBR_PARTITION_DATA;

typedef struct _GPT_PARTITION_DATA
{
    GUID PartitionType;
    GUID PartitionId;
    DWORD64 Attributes;
} GPT_PARTITION_DATA, *PGPT_PARTITION_DATA;

typedef struct _PARTENTRY
{
    LIST_ENTRY ListEntry;

    struct _DISKENTRY *DiskEntry;

    ULARGE_INTEGER StartSector;
    ULARGE_INTEGER SectorCount;

    union
    {
        MBR_PARTITION_DATA Mbr;
        GPT_PARTITION_DATA Gpt;
    };

    ULONG OnDiskPartitionNumber;
    ULONG PartitionNumber;
    ULONG PartitionIndex;

    CHAR DriveLetter;
    CHAR VolumeLabel[17];
    CHAR FileSystemName[9];
    FORMATSTATE FormatState;

    BOOLEAN LogicalPartition;

    /* Partition is partitioned disk space */
    BOOLEAN IsPartitioned;

    /* Partition is new. Table does not exist on disk yet */
    BOOLEAN New;

    /* Partition was created automatically. */
    BOOLEAN AutoCreate;

    /* Partition must be checked */
    BOOLEAN NeedsCheck;

    struct _FILE_SYSTEM_ITEM *FileSystem;
} PARTENTRY, *PPARTENTRY;


typedef struct _BIOSDISKENTRY
{
    LIST_ENTRY ListEntry;
    ULONG DiskNumber;
    ULONG Signature;
    ULONG Checksum;
    BOOLEAN Recognized;
    CM_DISK_GEOMETRY_DEVICE_DATA DiskGeometry;
    CM_INT13_DRIVE_PARAMETER Int13DiskData;
} BIOSDISKENTRY, *PBIOSDISKENTRY;


typedef struct _DISKENTRY
{
    LIST_ENTRY ListEntry;

    PWSTR Description;
    PWSTR Location;
    STORAGE_BUS_TYPE BusType;

    ULONGLONG Cylinders;
    ULONG TracksPerCylinder;
    ULONG SectorsPerTrack;
    ULONG BytesPerSector;

    ULARGE_INTEGER SectorCount;
    ULONG SectorAlignment;
    ULONG CylinderAlignment;

    ULARGE_INTEGER StartSector;
    ULARGE_INTEGER EndSector;

    BOOLEAN BiosFound;
    ULONG BiosDiskNumber;
//    ULONG Signature;
//    ULONG Checksum;

    ULONG DiskNumber;
    USHORT Port;
    USHORT PathId;
    USHORT TargetId;
    USHORT Lun;

    /* Has the partition list been modified? */
    BOOLEAN Dirty;

    BOOLEAN NewDisk;
    DWORD PartitionStyle;

    UNICODE_STRING DriverName;

    PDRIVE_LAYOUT_INFORMATION_EX LayoutBuffer;

    PPARTENTRY ExtendedPartition;

    LIST_ENTRY PrimaryPartListHead;
    LIST_ENTRY LogicalPartListHead;

} DISKENTRY, *PDISKENTRY;

typedef struct _VOLENTRY
{
    LIST_ENTRY ListEntry;

    ULONG VolumeNumber;
    WCHAR VolumeName[MAX_PATH];
    WCHAR DeviceName[MAX_PATH];
    DWORD SerialNumber;

    WCHAR DriveLetter;

    PWSTR pszLabel;
    PWSTR pszFilesystem;
    VOLUME_TYPE VolumeType;
    ULARGE_INTEGER Size;

    ULARGE_INTEGER TotalAllocationUnits;
    ULARGE_INTEGER AvailableAllocationUnits;
    ULONG SectorsPerAllocationUnit;
    ULONG BytesPerSector;

    PVOLUME_DISK_EXTENTS pExtents;

} VOLENTRY, *PVOLENTRY;

#define SIZE_1KB    (1024ULL)
#define SIZE_10KB   (10ULL * 1024ULL)
#define SIZE_1MB    (1024ULL * 1024ULL)
#define SIZE_10MB   (10ULL * 1024ULL * 1024ULL)
#define SIZE_1GB    (1024ULL * 1024ULL * 1024ULL)
#define SIZE_10GB   (10ULL * 1024ULL * 1024ULL * 1024ULL)
#define SIZE_1TB    (1024ULL * 1024ULL * 1024ULL * 1024ULL)
#define SIZE_10TB   (10ULL * 1024ULL * 1024ULL * 1024ULL * 1024ULL)


/* GLOBAL VARIABLES ***********************************************************/

extern LIST_ENTRY DiskListHead;
extern LIST_ENTRY BiosDiskListHead;
extern LIST_ENTRY VolumeListHead;

extern PDISKENTRY CurrentDisk;
extern PPARTENTRY CurrentPartition;
extern PVOLENTRY  CurrentVolume;

/* PROTOTYPES *****************************************************************/

/* active.c */
BOOL active_main(INT argc, LPWSTR *argv);

/* add.c */
BOOL add_main(INT argc, LPWSTR *argv);

/* assign.c */
BOOL assign_main(INT argc, LPWSTR *argv);

/* attach.c */
BOOL attach_main(INT argc, LPWSTR *argv);

/* attributes.h */
BOOL attributes_main(INT argc, LPWSTR *argv);

/* automount.c */
BOOL automount_main(INT argc, LPWSTR *argv);

/* break.c */
BOOL break_main(INT argc, LPWSTR *argv);

/* clean.c */
BOOL clean_main(INT argc, LPWSTR *argv);

/* compact.c */
BOOL compact_main(INT argc, LPWSTR *argv);

/* convert.c */
NTSTATUS
CreateDisk(
    _In_ ULONG DiskNumber,
    _In_ PCREATE_DISK DiskInfo);

BOOL
ConvertGPT(
    _In_ INT argc,
    _In_ PWSTR *argv);

BOOL
ConvertMBR(
    _In_ INT argc,
    _In_ PWSTR *argv);

/* create.c */
BOOL
CreateEfiPartition(
    _In_ INT argc,
    _In_ PWSTR *argv);

BOOL
CreateExtendedPartition(
    _In_ INT argc,
    _In_ PWSTR *argv);

BOOL
CreateLogicalPartition(
    _In_ INT argc,
    _In_ PWSTR *argv);

BOOL
CreateMsrPartition(
    _In_ INT argc,
    _In_ PWSTR *argv);

BOOL
CreatePrimaryPartition(
    _In_ INT argc,
    _In_ PWSTR *argv);

/* delete.c */
BOOL
DeleteDisk(
    _In_ INT argc,
    _In_ PWSTR *argv);

BOOL
DeletePartition(
    _In_ INT argc,
    _In_ PWSTR *argv);

BOOL
DeleteVolume(
    _In_ INT argc,
    _In_ PWSTR *argv);


/* detach.c */
BOOL detach_main(INT argc, LPWSTR *argv);

/* detail.c */
BOOL
DetailDisk(
    INT argc,
    PWSTR *argv);

BOOL
DetailPartition(
    INT argc,
    PWSTR *argv);

BOOL
DetailVolume(
    INT argc,
    PWSTR *argv);

/* diskpart.c */

/* dump.c */
BOOL
DumpDisk(
    _In_ INT argc,
    _In_ LPWSTR *argv);

BOOL
DumpPartition(
    _In_ INT argc,
    _In_ LPWSTR *argv);


/* expand.c */
BOOL expand_main(INT argc, LPWSTR *argv);

/* extend.c */
BOOL extend_main(INT argc, LPWSTR *argv);

/* filesystem.c */
BOOL filesystems_main(INT argc, LPWSTR *argv);

/* format.c */
BOOL format_main(INT argc, LPWSTR *argv);

/* gpt.c */
BOOL gpt_main(INT argc, LPWSTR *argv);

/* help.c */
BOOL help_main(INT argc, LPWSTR *argv);
VOID HelpCommandList(VOID);
BOOL HelpCommand(PCOMMAND pCommand);

/* import. c */
BOOL import_main(INT argc, LPWSTR *argv);

/* inactive.c */
BOOL inactive_main(INT argc, LPWSTR *argv);

/* interpreter.c */
BOOL InterpretScript(LPWSTR line);
BOOL InterpretCmd(INT argc, LPWSTR *argv);
VOID InterpretMain(VOID);

/* list.c */
BOOL
ListDisk(
    INT argc,
    PWSTR *argv);

BOOL
ListPartition(
    INT argc,
    PWSTR *argv);

BOOL
ListVolume(
    INT argc,
    PWSTR *argv);

BOOL
ListVirtualDisk(
    INT argc,
    PWSTR *argv);

VOID
PrintDisk(
    _In_ PDISKENTRY DiskEntry);

VOID
PrintVolume(
    _In_ PVOLENTRY VolumeEntry);

/* merge.c */
BOOL merge_main(INT argc, LPWSTR *argv);

/* misc.c */
BOOL
IsDecString(
    _In_ PWSTR pszDecString);

BOOL
IsHexString(
    _In_ PWSTR pszHexString);

BOOL
HasPrefix(
    _In_ PWSTR pszString,
    _In_ PWSTR pszPrefix,
    _Out_opt_ PWSTR *pszSuffix);

ULONGLONG
RoundingDivide(
    _In_ ULONGLONG Dividend,
    _In_ ULONGLONG Divisor);

PWSTR
DuplicateQuotedString(
    _In_ PWSTR pszInString);

PWSTR
DuplicateString(
    _In_ PWSTR pszInString);

VOID
CreateGUID(
    _Out_ GUID *pGuid);

VOID
CreateSignature(
    _Out_ PDWORD pSignature);

VOID
PrintGUID(
    _Out_ PWSTR pszBuffer,
    _In_ GUID *pGuid);

BOOL
StringToGUID(
    _Out_ GUID *pGuid,
    _In_ PWSTR pszString);

VOID
PrintBusType(
    _Out_ PWSTR pszBuffer,
    _In_ INT cchBufferMax,
    _In_ STORAGE_BUS_TYPE Bustype);

/* offline.c */
BOOL offline_main(INT argc, LPWSTR *argv);

/* online.c */
BOOL online_main(INT argc, LPWSTR *argv);

/* partlist.c */
#ifdef DUMP_PARTITION_TABLE
VOID
DumpPartitionTable(
    _In_ PDISKENTRY DiskEntry);
#endif

#ifdef DUMP_PARTITION_LIST
VOID
DumpPartitionList(
    _In_ PDISKENTRY DiskEntry);
#endif

ULONGLONG
AlignDown(
    _In_ ULONGLONG Value,
    _In_ ULONG Alignment);

NTSTATUS
CreatePartitionList(VOID);

VOID
DestroyPartitionList(VOID);

NTSTATUS
CreateVolumeList(VOID);

VOID
DestroyVolumeList(VOID);

VOID
ScanForUnpartitionedMbrDiskSpace(
    PDISKENTRY DiskEntry);

VOID
ScanForUnpartitionedGptDiskSpace(
    PDISKENTRY DiskEntry);

VOID
ReadLayoutBuffer(
    _In_ HANDLE FileHandle,
    _In_ PDISKENTRY DiskEntry);

NTSTATUS
WriteMbrPartitions(
    _In_ PDISKENTRY DiskEntry);

NTSTATUS
WriteGptPartitions(
    _In_ PDISKENTRY DiskEntry);

VOID
UpdateMbrDiskLayout(
    _In_ PDISKENTRY DiskEntry);

VOID
UpdateGptDiskLayout(
    _In_ PDISKENTRY DiskEntry,
    _In_ BOOL DeleteEntry);

PPARTENTRY
GetPrevUnpartitionedEntry(
    _In_ PPARTENTRY PartEntry);

PPARTENTRY
GetNextUnpartitionedEntry(
    _In_ PPARTENTRY PartEntry);

ULONG
GetPrimaryPartitionCount(
    _In_ PDISKENTRY DiskEntry);

NTSTATUS
DismountVolume(
    _In_ PPARTENTRY PartEntry);

PVOLENTRY
GetVolumeFromPartition(
    _In_ PPARTENTRY PartEntry);

VOID
RemoveVolume(
    _In_ PVOLENTRY VolumeEntry);


/* recover.c */
BOOL recover_main(INT argc, LPWSTR *argv);

/* remove.c */
BOOL remove_main(INT argc, LPWSTR *argv);

/* repair.c */
BOOL repair_main(INT argc, LPWSTR *argv);

/* rescan.c */
BOOL rescan_main(INT argc, LPWSTR *argv);

/* retain.c */
BOOL retain_main(INT argc, LPWSTR *argv);

/* san.c */
BOOL san_main(INT argc, LPWSTR *argv);

/* select.c */
BOOL
SelectDisk(
    INT argc,
    PWSTR *argv);

BOOL
SelectPartition(
    INT argc,
    PWSTR *argv);

BOOL
SelectVolume(
    INT argc,
    PWSTR *argv);
/*
BOOL
SelectVirtualDisk(
    INT argc,
    PWSTR *argv);
*/
/* setid.c */
BOOL setid_main(INT argc, LPWSTR *argv);

/* shrink.c */
BOOL shrink_main(INT argc, LPWSTR *argv);

/* uniqueid.c */
BOOL
UniqueIdDisk(
    _In_ INT argc,
    _In_ PWSTR *argv);

#endif /* DISKPART_H */
