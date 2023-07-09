/*++
Copyright (c) 1997  Microsoft Corporation

Module Name:
    mntmgr.h

Abstract:
    This file defines the internal data structure for the MOUNTMGR driver.

Author:
    norbertk
*/

#define MOUNTED_DEVICES_KEY         L"\\Registry\\Machine\\System\\MountedDevices"
#define MOUNTED_DEVICES_OFFLINE_KEY L"\\Registry\\Machine\\System\\MountedDevices\\Offline"

typedef struct _SYMBOLIC_LINK_NAME_ENTRY {
    LIST_ENTRY      ListEntry;
    UNICODE_STRING  SymbolicLinkName;
    BOOLEAN         IsInDatabase;
} SYMBOLIC_LINK_NAME_ENTRY, *PSYMBOLIC_LINK_NAME_ENTRY;

typedef struct _REPLICATED_UNIQUE_ID {
    LIST_ENTRY          ListEntry;
    PMOUNTDEV_UNIQUE_ID UniqueId;
} REPLICATED_UNIQUE_ID, *PREPLICATED_UNIQUE_ID;

struct _DEVICE_EXTENSION;
typedef struct _DEVICE_EXTENSION DEVICE_EXTENSION, *PDEVICE_EXTENSION;

typedef struct _MOUNTED_DEVICE_INFORMATION {
    LIST_ENTRY          ListEntry;
    LIST_ENTRY          SymbolicLinkNames;
    LIST_ENTRY          ReplicatedUniqueIds;
    UNICODE_STRING      NotificationName;
    PMOUNTDEV_UNIQUE_ID UniqueId;
    UNICODE_STRING      DeviceName;
    BOOLEAN             KeepLinksWhenOffline;
    UCHAR               SuggestedDriveLetter;
    BOOLEAN             NotAPdo;
    BOOLEAN             IsRemovable;
    PVOID               TargetDeviceNotificationEntry;
    PDEVICE_EXTENSION   Extension;
} MOUNTED_DEVICE_INFORMATION, *PMOUNTED_DEVICE_INFORMATION;

typedef struct _SAVED_LINKS_INFORMATION {
    LIST_ENTRY          ListEntry;
    LIST_ENTRY          SymbolicLinkNames;
    PMOUNTDEV_UNIQUE_ID UniqueId;
} SAVED_LINKS_INFORMATION, *PSAVED_LINKS_INFORMATION;

struct _DEVICE_EXTENSION {
    PDEVICE_OBJECT DeviceObject;// A pointer to our own device object.
    PDRIVER_OBJECT DriverObject;// A pointer to the driver object.
    LIST_ENTRY MountedDeviceList;// A linked list mounted devices.
    LIST_ENTRY DeadMountedDeviceList;// A linked list of unresponsive mounted devices.
    PVOID NotificationEntry;// Notification entry.
    KSEMAPHORE Mutex;// For synchronization.
    KSEMAPHORE RemoteDatabaseSemaphore;// Synchronization for the Remote databases.
    BOOLEAN AutomaticDriveLetterAssignment;// Specifies whether or not to automatically assign drive letters.
    LIST_ENTRY ChangeNotifyIrps;// Change notify list.  Protect with cancel spin lock.
    ULONG EpicNumber;// Change notify epic number.  Protect with 'mutex'.
    LIST_ENTRY SavedLinksList;// A list of saved links.
    BOOLEAN SuggestedDriveLettersProcessed;// Indicates whether or not the suggested drive letters have been processed.

    // A thread to be used for verifying remote databases.

    LIST_ENTRY WorkerQueue;
    KSEMAPHORE WorkerSemaphore;
    LONG WorkerRefCount;
    KSPIN_LOCK WorkerSpinLock;
};

typedef struct _MOUNTMGR_FILE_ENTRY {
    ULONG EntryLength;
    ULONG RefCount;
    USHORT VolumeNameOffset;
    USHORT VolumeNameLength;
    USHORT UniqueIdOffset;
    USHORT UniqueIdLength;
} MOUNTMGR_FILE_ENTRY, *PMOUNTMGR_FILE_ENTRY;