/*
 * PROJECT:         ReactOS DiskPart
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            base/system/diskpart/diskpart.c
 * PURPOSE:         Manages all the partitions of the OS in an interactive way
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
#include <winreg.h>
#include <winuser.h>
#include <wincon.h>

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

#include "resource.h"

/* DEFINES *******************************************************************/

typedef struct _COMMAND
{
    LPWSTR name;
    BOOL (*func)(INT, WCHAR**);
    INT help;
    INT help_desc;
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

typedef struct _PARTENTRY
{
    LIST_ENTRY ListEntry;

    struct _DISKENTRY *DiskEntry;

    ULARGE_INTEGER StartSector;
    ULARGE_INTEGER SectorCount;

    BOOLEAN BootIndicator;
    UCHAR PartitionType;
    ULONG HiddenSectors;
    ULONG PartitionNumber;
    ULONG PartitionIndex;

    CHAR DriveLetter;
    CHAR VolumeLabel[17];
    CHAR FileSystemName[9];

    BOOLEAN LogicalPartition;

    /* Partition is partitioned disk space */
    BOOLEAN IsPartitioned;

    /* Partition is new. Table does not exist on disk yet */
    BOOLEAN New;

    /* Partition was created automatically. */
    BOOLEAN AutoCreate;

    FORMATSTATE FormatState;

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

    ULONGLONG Cylinders;
    ULONG TracksPerCylinder;
    ULONG SectorsPerTrack;
    ULONG BytesPerSector;

    ULARGE_INTEGER SectorCount;
    ULONG SectorAlignment;
    ULONG CylinderAlignment;

    BOOLEAN BiosFound;
    ULONG BiosDiskNumber;
//    ULONG Signature;
//    ULONG Checksum;

    ULONG DiskNumber;
    USHORT Port;
    USHORT Bus;
    USHORT Id;

    /* Has the partition list been modified? */
    BOOLEAN Dirty;

    BOOLEAN NewDisk;
    BOOLEAN NoMbr; /* MBR is absent */

    UNICODE_STRING DriverName;

    PDRIVE_LAYOUT_INFORMATION LayoutBuffer;

    PPARTENTRY ExtendedPartition;

    LIST_ENTRY PrimaryPartListHead;
    LIST_ENTRY LogicalPartListHead;

} DISKENTRY, *PDISKENTRY;


/* GLOBAL VARIABLES ***********************************************************/

extern LIST_ENTRY DiskListHead;
extern LIST_ENTRY BiosDiskListHead;

extern PDISKENTRY CurrentDisk;
extern PPARTENTRY CurrentPartition;

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
BOOL convert_main(INT argc, LPWSTR *argv);

/* create.c */
BOOL create_main(INT argc, LPWSTR *argv);

/* delete.c */
BOOL delete_main(INT argc, LPWSTR *argv);

/* detach.c */
BOOL detach_main(INT argc, LPWSTR *argv);

/* detail.c */
BOOL detail_main(INT argc, LPWSTR *argv);

/* diskpart.c */
VOID PrintResourceString(INT resID, ...);

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
VOID help_cmdlist(VOID);

/* import. c */
BOOL import_main(INT argc, LPWSTR *argv);

/* inactive.c */
BOOL inactive_main(INT argc, LPWSTR *argv);

/* interpreter.c */
BOOL InterpretScript(LPWSTR line);
BOOL InterpretCmd(INT argc, LPWSTR *argv);
VOID InterpretMain(VOID);

/* list.c */
BOOL list_main(INT argc, LPWSTR *argv);

/* merge.c */
BOOL merge_main(INT argc, LPWSTR *argv);

/* offline.c */
BOOL offline_main(INT argc, LPWSTR *argv);

/* online.c */
BOOL online_main(INT argc, LPWSTR *argv);

/* partlist.c */
NTSTATUS
CreatePartitionList(VOID);

VOID
DestroyPartitionList(VOID);

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
BOOL select_main(INT argc, LPWSTR *argv);

/* setid.c */
BOOL setid_main(INT argc, LPWSTR *argv);

/* shrink.c */
BOOL shrink_main(INT argc, LPWSTR *argv);

/* uniqueid.c */
BOOL uniqueid_main(INT argc, LPWSTR *argv);

#endif /* DISKPART_H */
