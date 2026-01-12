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

/* NOERR codes for the program */
#undef EXIT_SUCCESS
#undef EXIT_FAILURE

typedef enum _EXIT_CODE
{
    EXIT_SUCCESS = 0,
    EXIT_FATAL,
    EXIT_CMD_ARG,
    EXIT_FILE,
    EXIT_SERVICE,
    EXIT_SYNTAX,
    EXIT_EXIT       /* Only used by the exit command */
} EXIT_CODE;

typedef struct _COMMAND
{
    PWSTR cmd1;
    PWSTR cmd2;
    PWSTR cmd3;
    EXIT_CODE (*func)(INT, PWSTR*);
    INT help;
    DWORD help_detail;
} COMMAND, *PCOMMAND;

extern COMMAND cmds[];

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

    BOOL IsSystem;
    BOOL IsBoot;

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

    BOOL IsBoot;

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

    BOOL IsSystem;
    BOOL IsBoot;

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
EXIT_CODE
active_main(
    _In_ INT argc,
    _In_ PWSTR *argv);

/* add.c */
EXIT_CODE
add_main(
    _In_ INT argc,
    _In_ PWSTR *argv);

/* assign.c */
EXIT_CODE
assign_main(
    _In_ INT argc,
    _In_ PWSTR *argv);

/* attach.c */
EXIT_CODE
attach_main(
    _In_ INT argc,
    _In_ PWSTR *argv);

/* attributes.h */
EXIT_CODE
attributes_main(
    _In_ INT argc,
    _In_ PWSTR *argv);

/* automount.c */
EXIT_CODE
automount_main(
    _In_ INT argc,
    _In_ PWSTR *argv);

/* break.c */
EXIT_CODE
break_main(
    _In_ INT argc,
    _In_ PWSTR *argv);

/* clean.c */
EXIT_CODE
clean_main(
    _In_ INT argc,
    _In_ PWSTR *argv);

/* compact.c */
EXIT_CODE
compact_main(
    _In_ INT argc,
    _In_ PWSTR *argv);

/* convert.c */
NTSTATUS
CreateDisk(
    _In_ ULONG DiskNumber,
    _In_ PCREATE_DISK DiskInfo);

EXIT_CODE
ConvertGPT(
    _In_ INT argc,
    _In_ PWSTR *argv);

EXIT_CODE
ConvertMBR(
    _In_ INT argc,
    _In_ PWSTR *argv);

/* create.c */
EXIT_CODE
CreateEfiPartition(
    _In_ INT argc,
    _In_ PWSTR *argv);

EXIT_CODE
CreateExtendedPartition(
    _In_ INT argc,
    _In_ PWSTR *argv);

EXIT_CODE
CreateLogicalPartition(
    _In_ INT argc,
    _In_ PWSTR *argv);

EXIT_CODE
CreateMsrPartition(
    _In_ INT argc,
    _In_ PWSTR *argv);

EXIT_CODE
CreatePrimaryPartition(
    _In_ INT argc,
    _In_ PWSTR *argv);

/* delete.c */
EXIT_CODE
DeleteDisk(
    _In_ INT argc,
    _In_ PWSTR *argv);

EXIT_CODE
DeletePartition(
    _In_ INT argc,
    _In_ PWSTR *argv);

EXIT_CODE
DeleteVolume(
    _In_ INT argc,
    _In_ PWSTR *argv);


/* detach.c */
EXIT_CODE
detach_main(
    _In_ INT argc,
    _In_ PWSTR *argv);

/* detail.c */
EXIT_CODE
DetailDisk(
    INT argc,
    PWSTR *argv);

EXIT_CODE
DetailPartition(
    INT argc,
    PWSTR *argv);

EXIT_CODE
DetailVolume(
    INT argc,
    PWSTR *argv);

/* diskpart.c */

/* dump.c */
EXIT_CODE
DumpDisk(
    _In_ INT argc,
    _In_ LPWSTR *argv);

EXIT_CODE
DumpPartition(
    _In_ INT argc,
    _In_ LPWSTR *argv);


/* expand.c */
EXIT_CODE
expand_main(
    _In_ INT argc,
    _In_ PWSTR *argv);

/* extend.c */
EXIT_CODE
extend_main(
    _In_ INT argc,
    _In_ PWSTR *argv);

/* filesystem.c */
EXIT_CODE
filesystems_main(
    _In_ INT argc,
    _In_ PWSTR *argv);

/* format.c */
EXIT_CODE
format_main(
    _In_ INT argc,
    _In_ PWSTR *argv);

/* gpt.c */
EXIT_CODE
gpt_main(
    _In_ INT argc,
    _In_ PWSTR *argv);

/* help.c */
EXIT_CODE
help_main(
    _In_ INT argc,
    _In_ PWSTR *argv);

VOID
HelpCommandList(VOID);

EXIT_CODE
HelpCommand(
    _In_ PCOMMAND pCommand);

/* import. c */
EXIT_CODE
import_main(
    _In_ INT argc,
    _In_ PWSTR *argv);

/* inactive.c */
EXIT_CODE
inactive_main(
    _In_ INT argc,
    _In_ PWSTR *argv);

/* interpreter.c */
EXIT_CODE
InterpretScript(
    _In_ LPWSTR line);

EXIT_CODE
InterpretCmd(
    _In_ INT argc,
    _In_ PWSTR *argv);

VOID
InterpretMain(VOID);

/* list.c */
EXIT_CODE
ListDisk(
    INT argc,
    PWSTR *argv);

EXIT_CODE
ListPartition(
    INT argc,
    PWSTR *argv);

EXIT_CODE
ListVolume(
    INT argc,
    PWSTR *argv);

EXIT_CODE
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
EXIT_CODE
merge_main(
    _In_ INT argc,
    _In_ PWSTR *argv);

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

/* mountmgr.h */
BOOL
GetAutomountState(
    _Out_ PBOOL State);

BOOL
SetAutomountState(
    _In_ BOOL bEnable);

BOOL
ScrubAutomount(VOID);

BOOL
AssignDriveLetter(
    _In_ PWSTR DeviceName,
    _In_ WCHAR DriveLetter);

BOOL
AssignNextDriveLetter(
    _In_ PWSTR DeviceName,
    _Out_ PWCHAR DriveLetter);

BOOL
DeleteDriveLetter(
    _In_ WCHAR DriveLetter);

/* offline.c */
EXIT_CODE
offline_main(
    _In_ INT argc,
    _In_ PWSTR *argv);

/* online.c */
EXIT_CODE
online_main(
    _In_ INT argc,
    _In_ PWSTR *argv);

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
EXIT_CODE
recover_main(
    _In_ INT argc,
    _In_ PWSTR *argv);

/* remove.c */
EXIT_CODE
remove_main(
    _In_ INT argc,
    _In_ PWSTR *argv);

/* repair.c */
EXIT_CODE
repair_main(
    _In_ INT argc,
    _In_ PWSTR *argv);

/* rescan.c */
EXIT_CODE
rescan_main(
    _In_ INT argc,
    _In_ PWSTR *argv);

/* retain.c */
EXIT_CODE
retain_main(
    _In_ INT argc,
    _In_ PWSTR *argv);

/* san.c */
EXIT_CODE
san_main(
    _In_ INT argc,
    _In_ PWSTR *argv);

/* select.c */
EXIT_CODE
SelectDisk(
    INT argc,
    PWSTR *argv);

EXIT_CODE
SelectPartition(
    INT argc,
    PWSTR *argv);

EXIT_CODE
SelectVolume(
    INT argc,
    PWSTR *argv);
/*
EXIT_CODE
SelectVirtualDisk(
    INT argc,
    PWSTR *argv);
*/
/* setid.c */
EXIT_CODE
setid_main(
    _In_ INT argc,
    _In_ PWSTR *argv);

/* shrink.c */
EXIT_CODE
shrink_main(
    _In_ INT argc,
    _In_ PWSTR *argv);

/* uniqueid.c */
EXIT_CODE
UniqueIdDisk(
    _In_ INT argc,
    _In_ PWSTR *argv);

#endif /* DISKPART_H */
