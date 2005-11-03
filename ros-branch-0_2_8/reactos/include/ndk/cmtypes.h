/*
 * PROJECT:         ReactOS Native Headers
 * FILE:            include/ndk/cmtypes.h
 * PURPOSE:         Definitions for Config Manager Types not defined in DDK/IFS
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 * UPDATE HISTORY:
 *                  Created 06/10/04
 */

#ifndef _CMTYPES_H
#define _CMTYPES_H

/* DEPENDENCIES **************************************************************/
#include <cfg.h>
#include "iotypes.h"

/* EXPORTED DATA *************************************************************/

/* CONSTANTS *****************************************************************/
#define MAX_BUS_NAME 24

/* PLUGPLAY_CONTROL_RELATED_DEVICE_DATA.Relation values */
#define PNP_GET_PARENT_DEVICE  1
#define PNP_GET_CHILD_DEVICE   2
#define PNP_GET_SIBLING_DEVICE 3

/* PLUGPLAY_CONTROL_STATUS_DATA.Operation values */
#define PNP_GET_DEVICE_STATUS    0
#define PNP_SET_DEVICE_STATUS    1
#define PNP_CLEAR_DEVICE_STATUS  2

/* ENUMERATIONS **************************************************************/

#ifdef NTOS_MODE_USER
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

typedef enum _PLUGPLAY_VIRTUAL_BUS_TYPE
{
    Root,
    MaxPlugPlayVirtualBusType
} PLUGPLAY_VIRTUAL_BUS_TYPE, *PPLUGPLAY_VIRTUAL_BUS_TYPE;

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
/* TYPES *********************************************************************/

#ifdef NTOS_MODE_USER
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

#else

typedef struct _REG_DELETE_KEY_INFORMATION
{
    PVOID Object;
} REG_DELETE_KEY_INFORMATION, *PREG_DELETE_KEY_INFORMATION;

typedef struct _REG_SET_VALUE_KEY_INFORMATION
{
    PVOID Object;
    PUNICODE_STRING ValueName;
    ULONG TitleIndex;
    ULONG Type;
    PVOID Data;
    ULONG DataSize;
} REG_SET_VALUE_KEY_INFORMATION, *PREG_SET_VALUE_KEY_INFORMATION;

typedef struct _REG_DELETE_VALUE_KEY_INFORMATION 
{
    PVOID Object;
    PUNICODE_STRING ValueName;
} REG_DELETE_VALUE_KEY_INFORMATION, *PREG_DELETE_VALUE_KEY_INFORMATION;

typedef struct _REG_SET_INFORMATION_KEY_INFORMATION
{
    PVOID Object;
    KEY_SET_INFORMATION_CLASS KeySetInformationClass;
    PVOID KeySetInformation;
    ULONG KeySetInformationLength;
} REG_SET_INFORMATION_KEY_INFORMATION, *PREG_SET_INFORMATION_KEY_INFORMATION;

typedef struct _REG_ENUMERATE_KEY_INFORMATION
{
    PVOID Object;
    ULONG Index;
    KEY_INFORMATION_CLASS KeyInformationClass;
    PVOID KeyInformation;
    ULONG Length;
    PULONG ResultLength;
} REG_ENUMERATE_KEY_INFORMATION, *PREG_ENUMERATE_KEY_INFORMATION;

typedef struct _REG_ENUMERATE_VALUE_KEY_INFORMATION 
{
    PVOID Object;
    ULONG Index;
    KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass;
    PVOID KeyValueInformation;
    ULONG Length;
    PULONG ResultLength;
} REG_ENUMERATE_VALUE_KEY_INFORMATION, *PREG_ENUMERATE_VALUE_KEY_INFORMATION;

typedef struct _REG_QUERY_KEY_INFORMATION
{
    PVOID Object;
    KEY_INFORMATION_CLASS KeyInformationClass;
    PVOID KeyInformation;
    ULONG Length;
    PULONG ResultLength;
} REG_QUERY_KEY_INFORMATION, *PREG_QUERY_KEY_INFORMATION;

typedef struct _REG_QUERY_VALUE_KEY_INFORMATION 
{
    PVOID Object;
    PUNICODE_STRING ValueName;
    KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass;
    PVOID KeyValueInformation;
    ULONG Length;
    PULONG ResultLength;
} REG_QUERY_VALUE_KEY_INFORMATION, *PREG_QUERY_VALUE_KEY_INFORMATION;

typedef struct _REG_QUERY_MULTIPLE_VALUE_KEY_INFORMATION
{
    PVOID Object;
    PKEY_VALUE_ENTRY ValueEntries;
    ULONG EntryCount;
    PVOID ValueBuffer;
    PULONG BufferLength;
    PULONG RequiredBufferLength;
} REG_QUERY_MULTIPLE_VALUE_KEY_INFORMATION, *PREG_QUERY_MULTIPLE_VALUE_KEY_INFORMATION;

typedef struct _REG_PRE_CREATE_KEY_INFORMATION 
{
    PUNICODE_STRING CompleteName;
} REG_PRE_CREATE_KEY_INFORMATION, *PREG_PRE_CREATE_KEY_INFORMATION;

typedef struct _REG_POST_CREATE_KEY_INFORMATION 
{
    PUNICODE_STRING CompleteName;
    PVOID Object;
    NTSTATUS Status;
} REG_POST_CREATE_KEY_INFORMATION, *PREG_POST_CREATE_KEY_INFORMATION;

typedef struct _REG_PRE_OPEN_KEY_INFORMATION 
{
    PUNICODE_STRING  CompleteName;
} REG_PRE_OPEN_KEY_INFORMATION, *PREG_PRE_OPEN_KEY_INFORMATION;

typedef struct _REG_POST_OPEN_KEY_INFORMATION 
{
    PUNICODE_STRING CompleteName;
    PVOID Object;
    NTSTATUS Status;
} REG_POST_OPEN_KEY_INFORMATION, *PREG_POST_OPEN_KEY_INFORMATION;

typedef struct _REG_POST_OPERATION_INFORMATION 
{
    PVOID Object;
    NTSTATUS Status;
} REG_POST_OPERATION_INFORMATION,*PREG_POST_OPERATION_INFORMATION;

#endif

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

/* Class 0x0A */
typedef struct _PLUGPLAY_CONTROL_PROPERTY_DATA
{
    UNICODE_STRING DeviceInstance;
    ULONG Property;
    PVOID Buffer;
    ULONG BufferSize;
} PLUGPLAY_CONTROL_PROPERTY_DATA, *PPLUGPLAY_CONTROL_PROPERTY_DATA;

/* Class 0x0C */
typedef struct _PLUGPLAY_CONTROL_RELATED_DEVICE_DATA
{
    UNICODE_STRING TargetDeviceInstance;
    ULONG Relation; /* 1: Parent  2: Child  3: Sibling */
    UNICODE_STRING RelatedDeviceInstance;
} PLUGPLAY_CONTROL_RELATED_DEVICE_DATA, *PPLUGPLAY_CONTROL_RELATED_DEVICE_DATA;

/* Class 0x0E */
typedef struct _PLUGPLAY_CONTOL_STATUS_DATA
{
    UNICODE_STRING DeviceInstance;
    ULONG Operation;       /* 0: Get  1: Set  2: Clear */
    ULONG DeviceStatus;    /* DN_       see cfg.h */
    ULONG DeviceProblem;   /* CM_PROB_  see cfg.h */
} PLUGPLAY_CONTROL_STATUS_DATA, *PPLUGPLAY_CONTROL_STATUS_DATA;

/* Class 0x0F */
typedef struct _PLUGPLAY_CONTROL_DEPTH_DATA
{
    UNICODE_STRING DeviceInstance;
    ULONG Depth;
} PLUGPLAY_CONTROL_DEPTH_DATA, *PPLUGPLAY_CONTROL_DEPTH_DATA;

/* Class 0x14 */
typedef struct _PLUGPLAY_CONTROL_RESET_DEVICE_DATA
{
   UNICODE_STRING DeviceInstance;
} PLUGPLAY_CONTROL_RESET_DEVICE_DATA, *PPLUGPLAY_CONTROL_RESET_DEVICE_DATA;

typedef struct _PLUGPLAY_BUS_TYPE
{
    PLUGPLAY_BUS_CLASS BusClass;
    union
    {
        INTERFACE_TYPE SystemBusType;
        PLUGPLAY_VIRTUAL_BUS_TYPE PlugPlayVirtualBusType;
    };
} PLUGPLAY_BUS_TYPE, *PPLUGPLAY_BUS_TYPE;

typedef struct _PLUGPLAY_BUS_INSTANCE
{
    PLUGPLAY_BUS_TYPE BusType;
    ULONG BusNumber;
    WCHAR BusName[MAX_BUS_NAME];
} PLUGPLAY_BUS_INSTANCE, *PPLUGPLAY_BUS_INSTANCE;

#ifdef NTOS_MODE_USER

#include <pshpack1.h>
typedef struct _CM_PARTIAL_RESOURCE_DESCRIPTOR {
  UCHAR Type;
  UCHAR ShareDisposition;
  USHORT Flags;
  union {
    struct {
      PHYSICAL_ADDRESS Start;
      ULONG Length;
    } Generic;
    struct {
      PHYSICAL_ADDRESS Start;
      ULONG Length;
    } Port;
    struct {
      ULONG Level;
      ULONG Vector;
      ULONG Affinity;
    } Interrupt;
    struct {
      PHYSICAL_ADDRESS Start;
      ULONG Length;
    } Memory;
    struct {
      ULONG Channel;
      ULONG Port;
      ULONG Reserved1;
    } Dma;
    struct {
      ULONG Data[3];
    } DevicePrivate;
    struct {
      ULONG Start;
      ULONG Length;
      ULONG Reserved;
    } BusNumber;
    struct {
      ULONG DataSize;
      ULONG Reserved1;
      ULONG Reserved2;
    } DeviceSpecificData;
  } u;
} CM_PARTIAL_RESOURCE_DESCRIPTOR, *PCM_PARTIAL_RESOURCE_DESCRIPTOR;

/* CM_PARTIAL_RESOURCE_DESCRIPTOR.Type */

#define CmResourceTypeNull                0
#define CmResourceTypePort                1
#define CmResourceTypeInterrupt           2
#define CmResourceTypeMemory              3
#define CmResourceTypeDma                 4
#define CmResourceTypeDeviceSpecific      5
#define CmResourceTypeBusNumber           6
#define CmResourceTypeMaximum             7
#define CmResourceTypeNonArbitrated     128
#define CmResourceTypeConfigData        128
#define CmResourceTypeDevicePrivate     129
#define CmResourceTypePcCardConfig      130
#define CmResourceTypeMfCardConfig      131

/* CM_PARTIAL_RESOURCE_DESCRIPTOR.ShareDisposition */

typedef enum _CM_SHARE_DISPOSITION {
  CmResourceShareUndetermined,
  CmResourceShareDeviceExclusive,
  CmResourceShareDriverExclusive,
  CmResourceShareShared
} CM_SHARE_DISPOSITION;

/* CM_PARTIAL_RESOURCE_DESCRIPTOR.Flags if Type = CmResourceTypePort */

#define CM_RESOURCE_PORT_MEMORY           0x0000
#define CM_RESOURCE_PORT_IO               0x0001
#define CM_RESOURCE_PORT_10_BIT_DECODE    0x0004
#define CM_RESOURCE_PORT_12_BIT_DECODE    0x0008
#define CM_RESOURCE_PORT_16_BIT_DECODE    0x0010
#define CM_RESOURCE_PORT_POSITIVE_DECODE  0x0020
#define CM_RESOURCE_PORT_PASSIVE_DECODE   0x0040
#define CM_RESOURCE_PORT_WINDOW_DECODE    0x0080

/* CM_PARTIAL_RESOURCE_DESCRIPTOR.Flags if Type = CmResourceTypeInterrupt */

#define CM_RESOURCE_INTERRUPT_LEVEL_SENSITIVE 0x0000
#define CM_RESOURCE_INTERRUPT_LATCHED         0x0001

/* CM_PARTIAL_RESOURCE_DESCRIPTOR.Flags if Type = CmResourceTypeMemory */

#define CM_RESOURCE_MEMORY_READ_WRITE     0x0000
#define CM_RESOURCE_MEMORY_READ_ONLY      0x0001
#define CM_RESOURCE_MEMORY_WRITE_ONLY     0x0002
#define CM_RESOURCE_MEMORY_PREFETCHABLE   0x0004
#define CM_RESOURCE_MEMORY_COMBINEDWRITE  0x0008
#define CM_RESOURCE_MEMORY_24             0x0010
#define CM_RESOURCE_MEMORY_CACHEABLE      0x0020

/* CM_PARTIAL_RESOURCE_DESCRIPTOR.Flags if Type = CmResourceTypeDma */

#define CM_RESOURCE_DMA_8                 0x0000
#define CM_RESOURCE_DMA_16                0x0001
#define CM_RESOURCE_DMA_32                0x0002
#define CM_RESOURCE_DMA_8_AND_16          0x0004
#define CM_RESOURCE_DMA_BUS_MASTER        0x0008
#define CM_RESOURCE_DMA_TYPE_A            0x0010
#define CM_RESOURCE_DMA_TYPE_B            0x0020
#define CM_RESOURCE_DMA_TYPE_F            0x0040

typedef struct _CM_PARTIAL_RESOURCE_LIST {
  USHORT  Version;
  USHORT  Revision;
  ULONG  Count;
  CM_PARTIAL_RESOURCE_DESCRIPTOR PartialDescriptors[1];
} CM_PARTIAL_RESOURCE_LIST, *PCM_PARTIAL_RESOURCE_LIST;

typedef struct _CM_FULL_RESOURCE_DESCRIPTOR {
  INTERFACE_TYPE  InterfaceType;
  ULONG  BusNumber;
  CM_PARTIAL_RESOURCE_LIST  PartialResourceList;
} CM_FULL_RESOURCE_DESCRIPTOR, *PCM_FULL_RESOURCE_DESCRIPTOR;

typedef struct _CM_RESOURCE_LIST {
  ULONG  Count;
  CM_FULL_RESOURCE_DESCRIPTOR  List[1];
} CM_RESOURCE_LIST, *PCM_RESOURCE_LIST;

typedef struct _CM_INT13_DRIVE_PARAMETER {
  USHORT  DriveSelect;
  ULONG  MaxCylinders;
  USHORT  SectorsPerTrack;
  USHORT  MaxHeads;
  USHORT  NumberDrives;
} CM_INT13_DRIVE_PARAMETER, *PCM_INT13_DRIVE_PARAMETER;

typedef struct _CM_DISK_GEOMETRY_DEVICE_DATA
{
  ULONG BytesPerSector;
  ULONG NumberOfCylinders;
  ULONG SectorsPerTrack;
  ULONG NumberOfHeads;
} CM_DISK_GEOMETRY_DEVICE_DATA, *PCM_DISK_GEOMETRY_DEVICE_DATA;

#include <poppack.h>

#endif

#endif

