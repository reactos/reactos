#ifndef __INCLUDE_DDK_PNPTYPES_H
#define __INCLUDE_DDK_PNPTYPES_H

// windows.h may be included before
#ifndef GUID_DEFINED
#define GUID_DEFINED

typedef struct _GUID {
  ULONG Data1;
  USHORT Data2;
  USHORT Data3;
  UCHAR Data4[8];
} GUID, *LPGUID;

#endif

typedef struct _DEVICE_CAPABILITIES {
  USHORT Size;
  USHORT Version;
  ULONG DeviceD1:1;
  ULONG DeviceD2:1;
  ULONG LockSupported:1;
  ULONG EjectSupported:1;
  ULONG Removable:1;
  ULONG DockDevice:1;
  ULONG UniqueID:1;
  ULONG SilentInstall:1;
  ULONG RawDeviceOK:1;
  ULONG SurpriseRemovalOK:1;
  ULONG WakeFromD0:1;
  ULONG WakeFromD1:1;
  ULONG WakeFromD2:1;
  ULONG WakeFromD3:1;
  ULONG HardwareDisabled:1;
  ULONG NonDynamic:1;
  ULONG WarmEjectSupported:1;
  ULONG Reserved:15;
  ULONG Address;
  ULONG UINumber;
  DEVICE_POWER_STATE DeviceState[PowerSystemMaximum];
  SYSTEM_POWER_STATE SystemWake;
  DEVICE_POWER_STATE DeviceWake;
  ULONG D1Latency;
  ULONG D2Latency;
  ULONG D3Latency;
} DEVICE_CAPABILITIES, *PDEVICE_CAPABILITIES;

typedef struct _DEVICE_INTERFACE_CHANGE_NOTIFICATION {
  USHORT Version;
  USHORT Size;
  GUID Event;
  //
  // Event-specific data
  //
  GUID InterfaceClassGuid;
  PUNICODE_STRING SymbolicLinkName;
} DEVICE_INTERFACE_CHANGE_NOTIFICATION, *PDEVICE_INTERFACE_CHANGE_NOTIFICATION;

typedef struct _HWPROFILE_CHANGE_NOTIFICATION {
  USHORT Version;
  USHORT Size;
  GUID Event;
  //
  // (No event-specific data)
  //
} HWPROFILE_CHANGE_NOTIFICATION, *PHWPROFILE_CHANGE_NOTIFICATION;

typedef struct _PLUGPLAY_NOTIFICATION_HEADER {
  USHORT Version; 
  USHORT Size; 
  GUID Event;
} PLUGPLAY_NOTIFICATION_HEADER, *PPLUGPLAY_NOTIFICATION_HEADER;

typedef ULONG PNP_DEVICE_STATE, *PPNP_DEVICE_STATE;

typedef struct _TARGET_DEVICE_CUSTOM_NOTIFICATION {
  USHORT Version;
  USHORT Size;
  GUID Event;
  //
  // Event-specific data
  //
  PFILE_OBJECT FileObject;
  LONG NameBufferOffset;
  UCHAR CustomDataBuffer[1];
} TARGET_DEVICE_CUSTOM_NOTIFICATION, *PTARGET_DEVICE_CUSTOM_NOTIFICATION;

typedef struct _TARGET_DEVICE_REMOVAL_NOTIFICATION {
  USHORT Version;
  USHORT Size;
  GUID Event;
  //
  // Event-specific data
  //
  PFILE_OBJECT FileObject;
} TARGET_DEVICE_REMOVAL_NOTIFICATION, *PTARGET_DEVICE_REMOVAL_NOTIFICATION;


// PnP Device Property for IoGetDeviceProperty
typedef enum {
  DevicePropertyDeviceDescription,
  DevicePropertyHardwareID,
  DevicePropertyCompatibleIDs,
  DevicePropertyBootConfiguration,
  DevicePropertyBootConfigurationTranslated,
  DevicePropertyClassName,
  DevicePropertyClassGuid,
  DevicePropertyDriverKeyName,
  DevicePropertyManufacturer,
  DevicePropertyFriendlyName,
  DevicePropertyLocationInformation,
  DevicePropertyPhysicalDeviceObjectName,
  DevicePropertyBusTypeGuid,
  DevicePropertyLegacyBusType,
  DevicePropertyBusNumber,
  DevicePropertyEnumeratorName,
  DevicePropertyAddress,
  DevicePropertyUINumber
} DEVICE_REGISTRY_PROPERTY;

// DevInstKeyType values for IoOpenDeviceRegistryKey
#define PLUGPLAY_REGKEY_DEVICE              1
#define PLUGPLAY_REGKEY_DRIVER              2
#define PLUGPLAY_REGKEY_CURRENT_HWPROFILE   4

// EventCategory for IoRegisterPlugPlayNotification
typedef enum _IO_NOTIFICATION_EVENT_CATEGORY {
  EventCategoryReserved,
  EventCategoryHardwareProfileChange,
  EventCategoryDeviceInterfaceChange,
  EventCategoryTargetDeviceChange
} IO_NOTIFICATION_EVENT_CATEGORY;

// CallbackRoutine for IoRegisterPlugPlayNotification
typedef
NTSTATUS
(*PDRIVER_NOTIFICATION_CALLBACK_ROUTINE)(
  IN PVOID NotificationStructure,
  IN PVOID Context);

// Callback for IoReportTargetDeviceChangeAsynchronous
typedef
VOID
(*PDEVICE_CHANGE_COMPLETE_CALLBACK)(
  IN PVOID Context);

// PNP/POWER values for IRP_MJ_PNP/IRP_MJ_POWER
typedef enum _DEVICE_RELATION_TYPE {
  BusRelations,
  EjectionRelations,
  PowerRelations,
  RemovalRelations,
  TargetDeviceRelation
} DEVICE_RELATION_TYPE, *PDEVICE_RELATION_TYPE;

typedef struct _DEVICE_RELATIONS {
  ULONG Count;
  PDEVICE_OBJECT Objects[1];  // variable length
} DEVICE_RELATIONS, *PDEVICE_RELATIONS;

typedef enum _DEVICE_USAGE_NOTIFICATION_TYPE {
  DeviceUsageTypeUndefined,
  DeviceUsageTypePaging,
  DeviceUsageTypeHibernation,
  DeviceUsageTypeDumpFile
} DEVICE_USAGE_NOTIFICATION_TYPE;


typedef struct _IO_REMOVE_LOCK_COMMON_BLOCK {
  BOOLEAN Removed;
  BOOLEAN Reserved[3];
  LONG IoCount;
  KEVENT RemoveEvent;
} IO_REMOVE_LOCK_COMMON_BLOCK;

typedef struct _IO_REMOVE_LOCK {
  IO_REMOVE_LOCK_COMMON_BLOCK Common;
} IO_REMOVE_LOCK, *PIO_REMOVE_LOCK;

#endif /* __INCLUDE_DDK_PNPTYPES_H */
