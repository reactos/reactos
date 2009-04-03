/*++ NDK Version: 0098

Copyright (c) Alex Ionescu.  All rights reserved.

Header Name:

    cmtypes.h

Abstract:

    Type definitions for the Configuration Manager.

Author:

    Alex Ionescu (alexi@tinykrnl.org) - Updated - 27-Feb-2006

--*/

#ifndef _CMTYPES_H
#define _CMTYPES_H

//
// Dependencies
//
#include <umtypes.h>
#include <cfg.h>
#include <iotypes.h>

#define MAX_BUS_NAME 24

//
// PLUGPLAY_CONTROL_RELATED_DEVICE_DATA.Relations
//
#define PNP_GET_PARENT_DEVICE           1
#define PNP_GET_CHILD_DEVICE            2
#define PNP_GET_SIBLING_DEVICE          3

//
// PLUGPLAY_CONTROL_STATUS_DATA Operations
//
#define PNP_GET_DEVICE_STATUS           0
#define PNP_SET_DEVICE_STATUS           1
#define PNP_CLEAR_DEVICE_STATUS         2

#ifdef NTOS_MODE_USER

//
// Resource Type
//
#define CmResourceTypeNull                      0
#define CmResourceTypePort                      1
#define CmResourceTypeInterrupt                 2
#define CmResourceTypeMemory                    3
#define CmResourceTypeDma                       4
#define CmResourceTypeDeviceSpecific            5
#define CmResourceTypeBusNumber                 6
#define CmResourceTypeMaximum                   7
#define CmResourceTypeNonArbitrated             128
#define CmResourceTypeConfigData                128
#define CmResourceTypeDevicePrivate             129
#define CmResourceTypePcCardConfig              130
#define CmResourceTypeMfCardConfig              131


//
// Resource Descriptor Share Dispositions
//
typedef enum _CM_SHARE_DISPOSITION
{
    CmResourceShareUndetermined,
    CmResourceShareDeviceExclusive,
    CmResourceShareDriverExclusive,
    CmResourceShareShared
} CM_SHARE_DISPOSITION;

#endif

//
// Port Resource Descriptor Flags
//
#define CM_RESOURCE_PORT_MEMORY                 0x0000
#define CM_RESOURCE_PORT_IO                     0x0001
#define CM_RESOURCE_PORT_10_BIT_DECODE          0x0004
#define CM_RESOURCE_PORT_12_BIT_DECODE          0x0008
#define CM_RESOURCE_PORT_16_BIT_DECODE          0x0010
#define CM_RESOURCE_PORT_POSITIVE_DECODE        0x0020
#define CM_RESOURCE_PORT_PASSIVE_DECODE         0x0040
#define CM_RESOURCE_PORT_WINDOW_DECODE          0x0080

//
// Memory Resource Descriptor Flags
//
#define CM_RESOURCE_MEMORY_READ_WRITE     0x0000
#define CM_RESOURCE_MEMORY_READ_ONLY      0x0001
#define CM_RESOURCE_MEMORY_WRITE_ONLY     0x0002
#define CM_RESOURCE_MEMORY_PREFETCHABLE   0x0004
#define CM_RESOURCE_MEMORY_COMBINEDWRITE  0x0008
#define CM_RESOURCE_MEMORY_24             0x0010
#define CM_RESOURCE_MEMORY_CACHEABLE      0x0020

//
// DMA Resource Descriptor Flags
//
#define CM_RESOURCE_DMA_8                 0x0000
#define CM_RESOURCE_DMA_16                0x0001
#define CM_RESOURCE_DMA_32                0x0002
#define CM_RESOURCE_DMA_8_AND_16          0x0004
#define CM_RESOURCE_DMA_BUS_MASTER        0x0008
#define CM_RESOURCE_DMA_TYPE_A            0x0010
#define CM_RESOURCE_DMA_TYPE_B            0x0020
#define CM_RESOURCE_DMA_TYPE_F            0x0040

//
// NtInitializeRegistry Flags
//
#define CM_BOOT_FLAG_SMSS                 0x0000
#define CM_BOOT_FLAG_SETUP                0x0001
#define CM_BOOT_FLAG_ACCEPTED             0x0002
#define CM_BOOT_FLAG_MAX                  0x03E9

#ifdef NTOS_MODE_USER

//
// Information Classes for NtQueryKey
//
typedef enum _KEY_INFORMATION_CLASS
{
    KeyBasicInformation,
    KeyNodeInformation,
    KeyFullInformation,
    KeyNameInformation,
    KeyCachedInformation,
    KeyFlagsInformation
} KEY_INFORMATION_CLASS;

typedef enum _KEY_VALUE_INFORMATION_CLASS
{
    KeyValueBasicInformation,
    KeyValueFullInformation,
    KeyValuePartialInformation,
    KeyValueFullInformationAlign64,
    KeyValuePartialInformationAlign64
} KEY_VALUE_INFORMATION_CLASS;

typedef enum _KEY_SET_INFORMATION_CLASS
{
    KeyWriteTimeInformation,
    KeyUserFlagsInformation,
    MaxKeySetInfoClass
} KEY_SET_INFORMATION_CLASS;

#endif

//
// Plag and Play Classes
//
typedef enum _PLUGPLAY_CONTROL_CLASS
{
    PlugPlayControlUserResponse = 0x07,
    PlugPlayControlProperty = 0x0A,
    PlugPlayControlGetRelatedDevice = 0x0C,
    PlugPlayControlDeviceStatus = 0x0E,
    PlugPlayControlGetDeviceDepth,
    PlugPlayControlResetDevice = 0x14
} PLUGPLAY_CONTROL_CLASS;

typedef enum _PLUGPLAY_BUS_CLASS
{
    SystemBus,
    PlugPlayVirtualBus,
    MaxPlugPlayBusClass
} PLUGPLAY_BUS_CLASS, *PPLUGPLAY_BUS_CLASS;

//
// Plag and Play Bus Types
//
typedef enum _PLUGPLAY_VIRTUAL_BUS_TYPE
{
    Root,
    MaxPlugPlayVirtualBusType
} PLUGPLAY_VIRTUAL_BUS_TYPE, *PPLUGPLAY_VIRTUAL_BUS_TYPE;

//
// Plag and Play Event Categories
//
typedef enum _PLUGPLAY_EVENT_CATEGORY
{
    HardwareProfileChangeEvent,
    TargetDeviceChangeEvent,
    DeviceClassChangeEvent,
    CustomDeviceEvent,
    DeviceInstallEvent,
    DeviceArrivalEvent,
    PowerEvent,
    VetoEvent,
    BlockedDriverEvent,
    MaxPlugEventCategory
} PLUGPLAY_EVENT_CATEGORY;

#ifdef NTOS_MODE_USER

//
// Information Structures for NtQueryKeyInformation
//
typedef struct _KEY_WRITE_TIME_INFORMATION
{
    LARGE_INTEGER LastWriteTime;
} KEY_WRITE_TIME_INFORMATION, *PKEY_WRITE_TIME_INFORMATION;

typedef struct _KEY_USER_FLAGS_INFORMATION
{
    ULONG UserFlags;
} KEY_USER_FLAGS_INFORMATION, *PKEY_USER_FLAGS_INFORMATION;

typedef struct _KEY_FULL_INFORMATION
{
    LARGE_INTEGER LastWriteTime;
    ULONG TitleIndex;
    ULONG ClassOffset;
    ULONG ClassLength;
    ULONG SubKeys;
    ULONG MaxNameLen;
    ULONG MaxClassLen;
    ULONG Values;
    ULONG MaxValueNameLen;
    ULONG MaxValueDataLen;
    WCHAR Class[1];
} KEY_FULL_INFORMATION, *PKEY_FULL_INFORMATION;

typedef struct _KEY_NAME_INFORMATION
{
    WCHAR Name[1];
} KEY_NAME_INFORMATION, *PKEY_NAME_INFORMATION;


typedef struct _KEY_NODE_INFORMATION
{
    LARGE_INTEGER LastWriteTime;
    ULONG TitleIndex;
    ULONG ClassOffset;
    ULONG ClassLength;
    ULONG NameLength;
    WCHAR Name[1];
} KEY_NODE_INFORMATION, *PKEY_NODE_INFORMATION;

typedef struct _KEY_VALUE_ENTRY
{
    PUNICODE_STRING ValueName;
    ULONG DataLength;
    ULONG DataOffset;
    ULONG Type;
} KEY_VALUE_ENTRY, *PKEY_VALUE_ENTRY;

typedef struct _KEY_VALUE_PARTIAL_INFORMATION
{
    ULONG TitleIndex;
    ULONG Type;
    ULONG DataLength;
    UCHAR Data[1];
} KEY_VALUE_PARTIAL_INFORMATION, *PKEY_VALUE_PARTIAL_INFORMATION;

typedef struct _KEY_VALUE_BASIC_INFORMATION
{
    ULONG TitleIndex;
    ULONG Type;
    ULONG NameLength;
    WCHAR Name[1];
} KEY_VALUE_BASIC_INFORMATION, *PKEY_VALUE_BASIC_INFORMATION;

typedef struct _KEY_VALUE_FULL_INFORMATION
{
    ULONG TitleIndex;
    ULONG Type;
    ULONG DataOffset;
    ULONG DataLength;
    ULONG NameLength;
    WCHAR Name[1];
} KEY_VALUE_FULL_INFORMATION, *PKEY_VALUE_FULL_INFORMATION;

typedef struct _KEY_BASIC_INFORMATION
{
    LARGE_INTEGER LastWriteTime;
    ULONG TitleIndex;
    ULONG NameLength;
    WCHAR Name[1];
} KEY_BASIC_INFORMATION, *PKEY_BASIC_INFORMATION;

#endif

//
// Plug and Play Event Block
//
typedef struct _PLUGPLAY_EVENT_BLOCK
{
    GUID EventGuid;
    PLUGPLAY_EVENT_CATEGORY EventCategory;
    PULONG Result;
    ULONG Flags;
    ULONG TotalSize;
    PVOID DeviceObject;
    union
    {
        struct
        {
            GUID ClassGuid;
            WCHAR SymbolicLinkName[ANYSIZE_ARRAY];
        } DeviceClass;
        struct
        {
            WCHAR DeviceIds[ANYSIZE_ARRAY];
        } TargetDevice;
        struct
        {
            WCHAR DeviceId[ANYSIZE_ARRAY];
        } InstallDevice;
        struct
        {
            PVOID NotificationStructure;
            WCHAR DeviceIds[ANYSIZE_ARRAY];
        } CustomNotification;
        struct
        {
            PVOID Notification;
        } ProfileNotification;
        struct
        {
            ULONG NotificationCode;
            ULONG NotificationData;
        } PowerNotification;
        struct
        {
            PNP_VETO_TYPE VetoType;
            WCHAR DeviceIdVetoNameBuffer[ANYSIZE_ARRAY];
        } VetoNotification;
        struct
        {
            GUID BlockedDriverGuid;
        } BlockedDriverNotification;
    };
} PLUGPLAY_EVENT_BLOCK, *PPLUGPLAY_EVENT_BLOCK;

//
// Plug and Play Control Classes
//

//Class 0x0A
typedef struct _PLUGPLAY_CONTROL_PROPERTY_DATA
{
    UNICODE_STRING DeviceInstance;
    ULONG Property;
    PVOID Buffer;
    ULONG BufferSize;
} PLUGPLAY_CONTROL_PROPERTY_DATA, *PPLUGPLAY_CONTROL_PROPERTY_DATA;

// Class 0x0C
typedef struct _PLUGPLAY_CONTROL_RELATED_DEVICE_DATA
{
    UNICODE_STRING TargetDeviceInstance;
    ULONG Relation;
    PWCHAR RelatedDeviceInstance;
    ULONG RelatedDeviceInstanceLength;
} PLUGPLAY_CONTROL_RELATED_DEVICE_DATA, *PPLUGPLAY_CONTROL_RELATED_DEVICE_DATA;

// Class 0x0E
typedef struct _PLUGPLAY_CONTOL_STATUS_DATA
{
    UNICODE_STRING DeviceInstance;
    ULONG Operation;
    ULONG DeviceStatus;
    ULONG DeviceProblem;
} PLUGPLAY_CONTROL_STATUS_DATA, *PPLUGPLAY_CONTROL_STATUS_DATA;

// Class 0x0F
typedef struct _PLUGPLAY_CONTROL_DEPTH_DATA
{
    UNICODE_STRING DeviceInstance;
    ULONG Depth;
} PLUGPLAY_CONTROL_DEPTH_DATA, *PPLUGPLAY_CONTROL_DEPTH_DATA;

// Class 0x14
typedef struct _PLUGPLAY_CONTROL_RESET_DEVICE_DATA
{
   UNICODE_STRING DeviceInstance;
} PLUGPLAY_CONTROL_RESET_DEVICE_DATA, *PPLUGPLAY_CONTROL_RESET_DEVICE_DATA;

//
// Plug and Play Bus Type Definition
//
typedef struct _PLUGPLAY_BUS_TYPE
{
    PLUGPLAY_BUS_CLASS BusClass;
    union
    {
        INTERFACE_TYPE SystemBusType;
        PLUGPLAY_VIRTUAL_BUS_TYPE PlugPlayVirtualBusType;
    };
} PLUGPLAY_BUS_TYPE, *PPLUGPLAY_BUS_TYPE;

//
// Plug and Play Bus Instance Definition
//
typedef struct _PLUGPLAY_BUS_INSTANCE
{
    PLUGPLAY_BUS_TYPE BusType;
    ULONG BusNumber;
    WCHAR BusName[MAX_BUS_NAME];
} PLUGPLAY_BUS_INSTANCE, *PPLUGPLAY_BUS_INSTANCE;

#ifdef NTOS_MODE_USER

//
// Partial Resource Descriptor and List for Hardware
//
#include <pshpack1.h>
typedef struct _CM_PARTIAL_RESOURCE_DESCRIPTOR
{
    UCHAR Type;
    UCHAR ShareDisposition;
    USHORT Flags;
    union
    {
        struct
        {
            PHYSICAL_ADDRESS Start;
            ULONG Length;
        } Generic;
        struct
        {
            PHYSICAL_ADDRESS Start;
            ULONG Length;
        } Port;
        struct
        {
            ULONG Level;
            ULONG Vector;
            KAFFINITY Affinity;
        } Interrupt;
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
        struct
        {
            union
            {
                struct
                {
                    USHORT Reserved;
                    USHORT MessageCount;
                    ULONG Vector;
                    KAFFINITY Affinity;
                } Raw;
                struct
                {
                    ULONG Level;
                    ULONG Vector;
                    KAFFINITY Affinity;
                } Translated;
            };
        } MessageInterrupt;
#endif
        struct
        {
            PHYSICAL_ADDRESS Start;
            ULONG Length;
        } Memory;
        struct
        {
            ULONG Channel;
            ULONG Port;
            ULONG Reserved1;
        } Dma;
        struct
        {
            ULONG Data[3];
        } DevicePrivate;
        struct
        {
            ULONG Start;
            ULONG Length;
            ULONG Reserved;
        } BusNumber;
        struct
        {
            ULONG DataSize;
            ULONG Reserved1;
            ULONG Reserved2;
        } DeviceSpecificData;
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
        struct
        {
            PHYSICAL_ADDRESS Start;
            ULONG Length40;
        } Memory40;
        struct
        {
            PHYSICAL_ADDRESS Start;
            ULONG Length48;
        } Memory48;
        struct
        {
            PHYSICAL_ADDRESS Start;
            ULONG Length64;
        } Memory64;
#endif
    } u;
} CM_PARTIAL_RESOURCE_DESCRIPTOR, *PCM_PARTIAL_RESOURCE_DESCRIPTOR;

typedef struct _CM_PARTIAL_RESOURCE_LIST
{
    USHORT Version;
    USHORT Revision;
    ULONG Count;
    CM_PARTIAL_RESOURCE_DESCRIPTOR PartialDescriptors[1];
} CM_PARTIAL_RESOURCE_LIST, *PCM_PARTIAL_RESOURCE_LIST;

//
// Full Resource Descriptor and List for Hardware
//
typedef struct _CM_FULL_RESOURCE_DESCRIPTOR
{
    INTERFACE_TYPE InterfaceType;
    ULONG BusNumber;
    CM_PARTIAL_RESOURCE_LIST PartialResourceList;
} CM_FULL_RESOURCE_DESCRIPTOR, *PCM_FULL_RESOURCE_DESCRIPTOR;

typedef struct _CM_RESOURCE_LIST
{
    ULONG Count;
    CM_FULL_RESOURCE_DESCRIPTOR List[1];
} CM_RESOURCE_LIST, *PCM_RESOURCE_LIST;

//
// ROM Block Structure
//
typedef struct _CM_ROM_BLOCK
{
    ULONG Address;
    ULONG Size;
} CM_ROM_BLOCK, *PCM_ROM_BLOCK;

//
// Disk/INT13 Structures
//
typedef struct _CM_INT13_DRIVE_PARAMETER
{
    USHORT DriveSelect;
    ULONG MaxCylinders;
    USHORT SectorsPerTrack;
    USHORT MaxHeads;
    USHORT NumberDrives;
} CM_INT13_DRIVE_PARAMETER, *PCM_INT13_DRIVE_PARAMETER;

typedef struct _CM_DISK_GEOMETRY_DEVICE_DATA
{
    ULONG BytesPerSector;
    ULONG NumberOfCylinders;
    ULONG SectorsPerTrack;
    ULONG NumberOfHeads;
} CM_DISK_GEOMETRY_DEVICE_DATA, *PCM_DISK_GEOMETRY_DEVICE_DATA;

#include <poppack.h>

#endif // _!NTOS_MODE_USER

#endif // _CMTYPES_H



