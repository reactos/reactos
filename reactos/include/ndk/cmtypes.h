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
    PlugPlayControlGetDeviceDepth
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
typedef struct _PLUGPLAY_CONTOL_DEPTH_DATA
{
    UNICODE_STRING DeviceInstance;
    ULONG Depth;
} PLUGPLAY_CONTROL_DEPTH_DATA, *PPLUGPLAY_CONTROL_DEPTH_DATA;

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

#endif

