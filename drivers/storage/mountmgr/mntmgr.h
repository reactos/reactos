#ifndef _MNTMGR_H_
#define _MNTMGR_H_

#include <ntifs.h>
#include <mountdev.h>
#include <ntddvol.h>
#include <ntdddisk.h>
#include <wdmguid.h>
#include <ndk/psfuncs.h>
#include <ntdddisk.h>
#include <section_attribs.h>

typedef struct _DEVICE_EXTENSION
{
    PDEVICE_OBJECT DeviceObject;           // 0x0
    PDRIVER_OBJECT DriverObject;           // 0x4
    LIST_ENTRY DeviceListHead;             // 0x8
    LIST_ENTRY OfflineDeviceListHead;      // 0x10
    PVOID NotificationEntry;               // 0x18
    KSEMAPHORE DeviceLock;                 // 0x1C
    KSEMAPHORE RemoteDatabaseLock;         // 0x30
    ULONG AutomaticDriveLetter;            // 0x44
    LIST_ENTRY IrpListHead;                // 0x48
    ULONG EpicNumber;                      // 0x50
    LIST_ENTRY SavedLinksListHead;         // 0x54
    BOOLEAN ProcessedSuggestions;          // 0x5C
    BOOLEAN NoAutoMount;                   // 0x5D
    LIST_ENTRY WorkerQueueListHead;        // 0x60
    KSEMAPHORE WorkerSemaphore;            // 0x68
    LONG WorkerReferences;                 // 0x7C
    KSPIN_LOCK WorkerLock;                 // 0x80
    LIST_ENTRY UniqueIdWorkerItemListHead; // 0x84
    PMOUNTDEV_UNIQUE_ID DriveLetterData;   // 0x8C
    UNICODE_STRING RegistryPath;           // 0x90
    LONG WorkerThreadStatus;               // 0x98
    LIST_ENTRY OnlineNotificationListHead; // 0x9C
    ULONG OnlineNotificationWorkerActive;  // 0xA4
    ULONG OnlineNotificationCount;         // 0xA8
    KEVENT OnlineNotificationEvent;        // 0xAC
} DEVICE_EXTENSION, *PDEVICE_EXTENSION;    // 0xBC

typedef struct _DEVICE_INFORMATION
{
    LIST_ENTRY DeviceListEntry;             // 0x00
    LIST_ENTRY SymbolicLinksListHead;       // 0x08
    LIST_ENTRY ReplicatedUniqueIdsListHead; // 0x10
    LIST_ENTRY AssociatedDevicesHead;       // 0x18
    UNICODE_STRING SymbolicName;            // 0x20
    PMOUNTDEV_UNIQUE_ID UniqueId;           // 0x28
    UNICODE_STRING DeviceName;              // 0x2C
    BOOLEAN KeepLinks;                      // 0x34
    UCHAR SuggestedDriveLetter;             // 0x35
    BOOLEAN ManuallyRegistered;             // 0x36
    BOOLEAN Removable;                      // 0x37
    BOOLEAN LetterAssigned;                 // 0x38
    BOOLEAN NeedsReconcile;                 // 0x39
    BOOLEAN NoDatabase;                     // 0x3A
    BOOLEAN SkipNotifications;              // 0x3B
    ULONG Migrated;                         // 0x3C
    LONG MountState;                        // 0x40
    PVOID TargetDeviceNotificationEntry;    // 0x44
    PDEVICE_EXTENSION DeviceExtension;      // 0x48
} DEVICE_INFORMATION, *PDEVICE_INFORMATION; // 0x4C

typedef struct _SYMLINK_INFORMATION
{
    LIST_ENTRY SymbolicLinksListEntry;        // 0x00
    UNICODE_STRING Name;                      // 0x08
    BOOLEAN Online;                           // 0x10
} SYMLINK_INFORMATION, *PSYMLINK_INFORMATION; // 0x14

typedef struct _SAVED_LINK_INFORMATION
{
    LIST_ENTRY SavedLinksListEntry;                 // 0x0
    LIST_ENTRY SymbolicLinksListHead;               // 0x8
    PMOUNTDEV_UNIQUE_ID UniqueId;                   // 0x10
} SAVED_LINK_INFORMATION, *PSAVED_LINK_INFORMATION; // 0x14

typedef struct _UNIQUE_ID_REPLICATE
{
    LIST_ENTRY ReplicatedUniqueIdsListEntry;  // 0x0
    PMOUNTDEV_UNIQUE_ID UniqueId;             // 0x8
} UNIQUE_ID_REPLICATE, *PUNIQUE_ID_REPLICATE; // 0xC

typedef struct _DATABASE_ENTRY
{
    ULONG EntrySize;                // 0x00
    ULONG EntryReferences;          // 0x04
    USHORT SymbolicNameOffset;      // 0x08
    USHORT SymbolicNameLength;      // 0x0A
    USHORT UniqueIdOffset;          // 0x0C
    USHORT UniqueIdLength;          // 0x0E
} DATABASE_ENTRY, *PDATABASE_ENTRY; // 0x10

typedef struct _ASSOCIATED_DEVICE_ENTRY
{
    LIST_ENTRY AssociatedDevicesEntry;                // 0x00
    PDEVICE_INFORMATION DeviceInformation;            // 0x08
    UNICODE_STRING String;                            // 0x0C
} ASSOCIATED_DEVICE_ENTRY, *PASSOCIATED_DEVICE_ENTRY; // 0x14

typedef struct _DEVICE_INFORMATION_ENTRY
{
    LIST_ENTRY DeviceInformationEntry;                  // 0x00
    PDEVICE_INFORMATION DeviceInformation;              // 0x08
} DEVICE_INFORMATION_ENTRY, *PDEVICE_INFORMATION_ENTRY; // 0x0C

typedef struct _ONLINE_NOTIFICATION_WORK_ITEM
{
    WORK_QUEUE_ITEM WorkItem;                                     // 0x00
    PDEVICE_EXTENSION DeviceExtension;                            // 0x10
    UNICODE_STRING SymbolicName;                                  // 0x14
} ONLINE_NOTIFICATION_WORK_ITEM, *PONLINE_NOTIFICATION_WORK_ITEM; // 0x1C

typedef struct _RECONCILE_WORK_ITEM_CONTEXT
{
    PDEVICE_EXTENSION DeviceExtension;
    PDEVICE_INFORMATION DeviceInformation;
} RECONCILE_WORK_ITEM_CONTEXT, *PRECONCILE_WORK_ITEM_CONTEXT;

typedef struct _RECONCILE_WORK_ITEM
{
    LIST_ENTRY WorkerQueueListEntry;            // 0x00
    PIO_WORKITEM WorkItem;                      // 0x08
    PWORKER_THREAD_ROUTINE WorkerRoutine;       // 0x0C
    PVOID Context;                              // 0x10
    RECONCILE_WORK_ITEM_CONTEXT;                // 0x14
} RECONCILE_WORK_ITEM, *PRECONCILE_WORK_ITEM;   // 0x1C

typedef struct _MIGRATE_WORK_ITEM
{
    PIO_WORKITEM WorkItem;                 // 0x0
    PDEVICE_INFORMATION DeviceInformation; // 0x4
    PKEVENT Event;                         // 0x8
    NTSTATUS Status;                       // 0x0C
    HANDLE Database;                       // 0x10
} MIGRATE_WORK_ITEM, *PMIGRATE_WORK_ITEM;  // 0x14

typedef struct _UNIQUE_ID_WORK_ITEM
{
    LIST_ENTRY UniqueIdWorkerItemListEntry;   // 0x0
    PIO_WORKITEM WorkItem;                    // 0x8
    PDEVICE_EXTENSION DeviceExtension;        // 0xC
    PIRP Irp;                                 // 0x10
    PVOID IrpBuffer;                          // 0x14
    PKEVENT Event;                            // 0x1C
    UNICODE_STRING DeviceName;                // 0x20
    ULONG IrpBufferLength;                    // 0x28
    ULONG StackSize;                          // 0x2C
} UNIQUE_ID_WORK_ITEM, *PUNIQUE_ID_WORK_ITEM; // 0x30

/* Memory allocation helpers */
#define AllocatePool(Size) ExAllocatePoolWithTag(PagedPool, Size, 'AtnM')
#define FreePool(P)        ExFreePoolWithTag(P, 'AtnM')

/* Misc macros */
#define MAX(a, b)          ((a > b) ? a : b)

#define LETTER_POSITION     0xC
#define COLON_POSITION      0xD
#define DRIVE_LETTER_LENGTH 0x1C

/* mountmgr.c */

extern UNICODE_STRING DosDevicesMount;
extern PDEVICE_OBJECT gdeviceObject;
extern UNICODE_STRING ReparseIndex;
extern UNICODE_STRING DeviceFloppy;
extern UNICODE_STRING DeviceMount;
extern UNICODE_STRING DeviceCdRom;
extern UNICODE_STRING SafeVolumes;
extern UNICODE_STRING DosDevices;
extern UNICODE_STRING DosGlobal;
extern UNICODE_STRING Global;
extern UNICODE_STRING Volume;
extern KEVENT UnloadEvent;
extern LONG Unloading;

INIT_FUNCTION
DRIVER_INITIALIZE DriverEntry;

VOID
NTAPI
MountMgrCancel(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
MountMgrMountedDeviceArrival(
    IN PDEVICE_EXTENSION Extension,
    IN PUNICODE_STRING SymbolicName,
    IN BOOLEAN FromVolume
);

VOID
MountMgrMountedDeviceRemoval(
    IN PDEVICE_EXTENSION Extension,
    IN PUNICODE_STRING DeviceName
);

NTSTATUS
FindDeviceInfo(
    IN PDEVICE_EXTENSION DeviceExtension,
    IN PUNICODE_STRING SymbolicName,
    IN BOOLEAN DeviceNameGiven,
    OUT PDEVICE_INFORMATION * DeviceInformation
);

VOID
MountMgrFreeDeadDeviceInfo(
    IN PDEVICE_INFORMATION DeviceInformation
);

NTSTATUS
QueryDeviceInformation(
    IN PUNICODE_STRING SymbolicName,
    OUT PUNICODE_STRING DeviceName OPTIONAL,
    OUT PMOUNTDEV_UNIQUE_ID * UniqueId OPTIONAL,
    OUT PBOOLEAN Removable OPTIONAL,
    OUT PBOOLEAN GptDriveLetter OPTIONAL,
    OUT PBOOLEAN HasGuid OPTIONAL,
    IN OUT LPGUID StableGuid OPTIONAL,
    OUT PBOOLEAN Valid OPTIONAL
);

BOOLEAN
HasDriveLetter(
    IN PDEVICE_INFORMATION DeviceInformation
);

INIT_FUNCTION
BOOLEAN
MountmgrReadNoAutoMount(
    IN PUNICODE_STRING RegistryPath
);

/* database.c */

extern PWSTR DatabasePath;
extern PWSTR OfflinePath;

VOID
ReconcileThisDatabaseWithMaster(
    IN PDEVICE_EXTENSION DeviceExtension,
    IN PDEVICE_INFORMATION DeviceInformation
);

NTSTATUS
WaitForRemoteDatabaseSemaphore(
    IN PDEVICE_EXTENSION DeviceExtension
);

VOID
ReleaseRemoteDatabaseSemaphore(
    IN PDEVICE_EXTENSION DeviceExtension
);

VOID
ChangeRemoteDatabaseUniqueId(
    IN PDEVICE_INFORMATION DeviceInformation,
    IN PMOUNTDEV_UNIQUE_ID OldUniqueId,
    IN PMOUNTDEV_UNIQUE_ID NewUniqueId
);

VOID
ReconcileAllDatabasesWithMaster(
    IN PDEVICE_EXTENSION DeviceExtension
);

VOID
DeleteFromLocalDatabase(
    IN PUNICODE_STRING SymbolicLink,
    IN PMOUNTDEV_UNIQUE_ID UniqueId
);

VOID
DeleteRegistryDriveLetter(
    IN PMOUNTDEV_UNIQUE_ID UniqueId
);

VOID
DeleteNoDriveLetterEntry(
    IN PMOUNTDEV_UNIQUE_ID UniqueId
);

NTSTATUS
QueryVolumeName(
    IN HANDLE RootDirectory,
    IN PFILE_REPARSE_POINT_INFORMATION ReparsePointInformation,
    IN PUNICODE_STRING FileName OPTIONAL,
    OUT PUNICODE_STRING SymbolicName,
    OUT PUNICODE_STRING VolumeName
);

HANDLE
OpenRemoteDatabase(
    IN PDEVICE_INFORMATION DeviceInformation,
    IN BOOLEAN MigrateDatabase
);

PDATABASE_ENTRY
GetRemoteDatabaseEntry(
    IN HANDLE Database,
    IN LONG StartingOffset 
);

NTSTATUS
WriteRemoteDatabaseEntry(
    IN HANDLE Database,
    IN LONG Offset,
    IN PDATABASE_ENTRY Entry
);

NTSTATUS
CloseRemoteDatabase(
    IN HANDLE Database
);

NTSTATUS
AddRemoteDatabaseEntry(
    IN HANDLE Database,
    IN PDATABASE_ENTRY Entry
);

NTSTATUS
DeleteRemoteDatabaseEntry(
    IN HANDLE Database,
    IN LONG StartingOffset
);

VOID
NTAPI
ReconcileThisDatabaseWithMasterWorker(
    IN PVOID Parameter
);

/* device.c */

DRIVER_DISPATCH MountMgrDeviceControl;

/* notify.c */
VOID
IssueUniqueIdChangeNotifyWorker(
    IN PUNIQUE_ID_WORK_ITEM WorkItem,
    IN PMOUNTDEV_UNIQUE_ID UniqueId
);

VOID
WaitForOnlinesToComplete(
    IN PDEVICE_EXTENSION DeviceExtension
);

VOID
RegisterForTargetDeviceNotification(
    IN PDEVICE_EXTENSION DeviceExtension,
    IN PDEVICE_INFORMATION DeviceInformation
);

VOID
SendOnlineNotification(
    IN PUNICODE_STRING SymbolicName
);

VOID
IssueUniqueIdChangeNotify(
    IN PDEVICE_EXTENSION DeviceExtension,
    IN PUNICODE_STRING DeviceName,
    IN PMOUNTDEV_UNIQUE_ID UniqueId
);

VOID
PostOnlineNotification(
    IN PDEVICE_EXTENSION DeviceExtension,
    IN PUNICODE_STRING SymbolicName
);

VOID
MountMgrNotify(
    IN PDEVICE_EXTENSION DeviceExtension
);

VOID
MountMgrNotifyNameChange(
    IN PDEVICE_EXTENSION DeviceExtension,
    IN PUNICODE_STRING DeviceName,
    IN BOOLEAN ValidateVolume
);

/* uniqueid.c */
VOID
MountMgrUniqueIdChangeRoutine(
    IN PDEVICE_EXTENSION DeviceExtension,
    IN PMOUNTDEV_UNIQUE_ID OldUniqueId,
    IN PMOUNTDEV_UNIQUE_ID NewUniqueId
);

VOID
CreateNoDriveLetterEntry(
    IN PMOUNTDEV_UNIQUE_ID UniqueId
);

BOOLEAN
HasNoDriveLetterEntry(
    IN PMOUNTDEV_UNIQUE_ID UniqueId
);

VOID
UpdateReplicatedUniqueIds(
    IN PDEVICE_INFORMATION DeviceInformation,
    IN PDATABASE_ENTRY DatabaseEntry 
);

BOOLEAN
IsUniqueIdPresent(
    IN PDEVICE_EXTENSION DeviceExtension,
    IN PDATABASE_ENTRY DatabaseEntry 
);

/* point.c */
NTSTATUS
MountMgrCreatePointWorker(
    IN PDEVICE_EXTENSION DeviceExtension,
    IN PUNICODE_STRING SymbolicLinkName,
    IN PUNICODE_STRING DeviceName
);

NTSTATUS
QueryPointsFromSymbolicLinkName(
    IN PDEVICE_EXTENSION DeviceExtension,
    IN PUNICODE_STRING SymbolicName,
    IN PIRP Irp
);

NTSTATUS
QueryPointsFromMemory(
    IN PDEVICE_EXTENSION DeviceExtension,
    IN PIRP Irp,
    IN PMOUNTDEV_UNIQUE_ID UniqueId OPTIONAL,
    IN PUNICODE_STRING SymbolicName OPTIONAL
);

/* symlink.c */
NTSTATUS
GlobalCreateSymbolicLink(
    IN PUNICODE_STRING DosName,
    IN PUNICODE_STRING DeviceName
);

NTSTATUS
GlobalDeleteSymbolicLink(
    IN PUNICODE_STRING DosName
);

NTSTATUS
QuerySuggestedLinkName(
    IN PUNICODE_STRING SymbolicName,
    OUT PUNICODE_STRING SuggestedLinkName,
    OUT PBOOLEAN UseOnlyIfThereAreNoOtherLinks
);

NTSTATUS
QuerySymbolicLinkNamesFromStorage(
    IN PDEVICE_EXTENSION DeviceExtension,
    IN PDEVICE_INFORMATION DeviceInformation,
    IN PUNICODE_STRING SuggestedLinkName,
    IN BOOLEAN UseOnlyIfThereAreNoOtherLinks,
    OUT PUNICODE_STRING * SymLinks,
    OUT PULONG SymLinkCount,
    IN BOOLEAN HasGuid,
    IN LPGUID Guid
);

PSAVED_LINK_INFORMATION
RemoveSavedLinks(
    IN PDEVICE_EXTENSION DeviceExtension,
    IN PMOUNTDEV_UNIQUE_ID UniqueId
);

BOOLEAN
RedirectSavedLink(
    IN PSAVED_LINK_INFORMATION SavedLinkInformation,
    IN PUNICODE_STRING DosName,
    IN PUNICODE_STRING NewLink
);

VOID
SendLinkCreated(
    IN PUNICODE_STRING SymbolicName
);

NTSTATUS
CreateNewVolumeName(
    OUT PUNICODE_STRING VolumeName,
    IN PGUID VolumeGuid OPTIONAL
);

BOOLEAN
IsDriveLetter(
    PUNICODE_STRING SymbolicName
);

VOID
DeleteSymbolicLinkNameFromMemory(
    IN PDEVICE_EXTENSION DeviceExtension,
    IN PUNICODE_STRING SymbolicLink,
    IN BOOLEAN MarkOffline
);

NTSTATUS
MountMgrQuerySymbolicLink(
    IN PUNICODE_STRING SymbolicName,
    IN OUT PUNICODE_STRING LinkTarget
);

#endif /* _MNTMGR_H_ */
