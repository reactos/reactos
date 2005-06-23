/*
 * ntpnp.h
 *
 * Plug-and-play interface routines
 *
 * Contributors:
 *   Created by Filip Navara <xnavara@volny.cz>
 *
 * THIS SOFTWARE IS NOT COPYRIGHTED
 *
 * This source code is offered for use in the public domain. You may
 * use, modify or distribute it freely.
 *
 * This code is distributed in the hope that it will be useful but
 * WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 * DISCLAIMED. This includes but is not limited to warranties of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef __NTPNP_H
#define __NTPNP_H

/*
 * TODO:
 *    - Describe the undocumented GUIDs.
 *    - Finish the description of NtPlugPlayControl.
 */

/*
 * Undocumented GUIDs used by NtGetPlugPlayEvent.
 */

DEFINE_GUID(GUID_DEVICE_STANDBY_VETOED, 0x03B21C13, 0x18D6, 0x11D3, 0x97, 0xDB, 0x00, 0xA0, 0xC9, 0x40, 0x52, 0x2E);
DEFINE_GUID(GUID_DEVICE_KERNEL_INITIATED_EJECT, 0x14689B54, 0x0703, 0x11D3, 0x97, 0xD2, 0x00, 0xA0, 0xC9, 0x40, 0x52, 0x2E);
DEFINE_GUID(GUID_DEVICE_THERMAL_ZONE, 0x4AFA3D51, 0x74A7, 0x11D0, 0xBE, 0x5E, 0x00, 0xA0, 0xC9, 0x06, 0x28, 0x57);
DEFINE_GUID(GUID_DEVICE_SYS_BUTTON, 0x4AFA3D53, 0x74A7, 0x11D0, 0xBE, 0x5E, 0x00, 0xA0, 0xC9, 0x06, 0x28, 0x57);
DEFINE_GUID(GUID_DEVICE_REMOVAL_VETOED, 0x60DBD5FA, 0xDDD2, 0x11D2, 0x97, 0xB8, 0x00, 0xA0, 0xC9, 0x40, 0x52, 0x2E);
DEFINE_GUID(GUID_DEVICE_HIBERNATE_VETOED, 0x61173AD9, 0x194F, 0x11D3, 0x97, 0xDC, 0x00, 0xA0, 0xC9, 0x40, 0x52, 0x2E);
DEFINE_GUID(GUID_DEVICE_BATTERY, 0x72631E54, 0x78A4, 0x11D0, 0xBC, 0xF7, 0x00, 0xAA, 0x00, 0xB7, 0xB3, 0x2A);
DEFINE_GUID(GUID_DEVICE_SAFE_REMOVAL, 0x8FBEF967, 0xD6C5, 0x11D2, 0x97, 0xB5, 0x00, 0xA0, 0xC9, 0x40, 0x52, 0x2E);
#ifndef __USE_W32API
DEFINE_GUID(GUID_DEVICE_INTERFACE_ARRIVAL, 0xCB3A4004, 0x46F0, 0x11D0, 0xB0, 0x8F, 0x00, 0x60, 0x97, 0x13, 0x05, 0x3F);
DEFINE_GUID(GUID_DEVICE_INTERFACE_REMOVAL, 0xCB3A4005, 0x46F0, 0x11D0, 0xB0, 0x8F, 0x00, 0x60, 0x97, 0x13, 0x05, 0x3F);
#endif
DEFINE_GUID(GUID_DEVICE_ARRIVAL, 0xCB3A4009, 0x46F0, 0x11D0, 0xB0, 0x8F, 0x00, 0x60, 0x97, 0x13, 0x05, 0x3F);
DEFINE_GUID(GUID_DEVICE_ENUMERATED, 0xCB3A400A, 0x46F0, 0x11D0, 0xB0, 0x8F, 0x00, 0x60, 0x97, 0x13, 0x05, 0x3F);
DEFINE_GUID(GUID_DEVICE_ENUMERATE_REQUEST, 0xCB3A400B, 0x46F0, 0x11D0, 0xB0, 0x8F, 0x00, 0x60, 0x97, 0x13, 0x05, 0x3F);
DEFINE_GUID(GUID_DEVICE_START_REQUEST, 0xCB3A400C, 0x46F0, 0x11D0, 0xB0, 0x8F, 0x00, 0x60, 0x97, 0x13, 0x05, 0x3F);
DEFINE_GUID(GUID_DEVICE_REMOVE_PENDING, 0xCB3A400D, 0x46F0, 0x11D0, 0xB0, 0x8F, 0x00, 0x60, 0x97, 0x13, 0x05, 0x3F);
DEFINE_GUID(GUID_DEVICE_QUERY_AND_REMOVE, 0xCB3A400E, 0x46F0, 0x11D0, 0xB0, 0x8F, 0x00, 0x60, 0x97, 0x13, 0x05, 0x3F);
DEFINE_GUID(GUID_DEVICE_EJECT, 0xCB3A400F, 0x46F0, 0x11D0, 0xB0, 0x8F, 0x00, 0x60, 0x97, 0x13, 0x05, 0x3F);
DEFINE_GUID(GUID_DEVICE_NOOP, 0xCB3A4010, 0x46F0, 0x11D0, 0xB0, 0x8F, 0x00, 0x60, 0x97, 0x13, 0x05, 0x3F);
DEFINE_GUID(GUID_DEVICE_WARM_EJECT_VETOED, 0xCBF4C1F9, 0x18D5, 0x11D3, 0x97, 0xDB, 0x00, 0xA0, 0xC9, 0x40, 0x52, 0x2E);
DEFINE_GUID(GUID_DEVICE_SURPRISE_REMOVAL, 0xCE5AF000, 0x80DD, 0x11D2, 0xA8, 0x8D, 0x00, 0xA0, 0xC9, 0x69, 0x6B, 0x4B);
DEFINE_GUID(GUID_DEVICE_EJECT_VETOED, 0xCF7B71E8, 0xD8FD, 0x11D2, 0x97, 0xB5, 0x00, 0xA0, 0xC9, 0x40, 0x52, 0x2E);
DEFINE_GUID(GUID_DEVICE_EVENT_RBC, 0xD0744792, 0xA98E, 0x11D2, 0x91, 0x7A, 0x00, 0xA0, 0xC9, 0x06, 0x8F, 0xF3);


#ifndef __GUIDS_ONLY__ /* This is defined to build libwdmguid.a */

typedef enum _PLUGPLAY_EVENT_CATEGORY {
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

/*
 * Plug and Play event structure used by NtGetPlugPlayEvent.
 *
 * EventGuid
 *    Can be one of the following values:
 *       GUID_HWPROFILE_QUERY_CHANGE
 *       GUID_HWPROFILE_CHANGE_CANCELLED
 *       GUID_HWPROFILE_CHANGE_COMPLETE
 *       GUID_TARGET_DEVICE_QUERY_REMOVE
 *       GUID_TARGET_DEVICE_REMOVE_CANCELLED
 *       GUID_TARGET_DEVICE_REMOVE_COMPLETE
 *       GUID_PNP_CUSTOM_NOTIFICATION
 *       GUID_PNP_POWER_NOTIFICATION
 *       GUID_DEVICE_* (see above)
 *
 * EventCategory
 *    Type of the event that happened.
 *
 * Result
 *    ?
 *
 * Flags
 *    ?
 *
 * TotalSize
 *    Size of the event block including the device IDs and other
 *    per category specific fields.
 */

typedef struct _PLUGPLAY_EVENT_BLOCK {
   GUID EventGuid;
   PLUGPLAY_EVENT_CATEGORY EventCategory;
   PULONG Result;
   ULONG Flags;
   ULONG TotalSize;
   PDEVICE_OBJECT DeviceObject;
   union {
      struct {
         GUID ClassGuid;
         WCHAR SymbolicLinkName[ANYSIZE_ARRAY];
      } DeviceClass;
      struct {
         WCHAR DeviceIds[ANYSIZE_ARRAY];
      } TargetDevice;
      struct {
         WCHAR DeviceId[ANYSIZE_ARRAY];
      } InstallDevice;
      struct {
         PVOID NotificationStructure;
         WCHAR DeviceIds[ANYSIZE_ARRAY];
      } CustomNotification;
      struct {
         PVOID Notification;
      } ProfileNotification;
      struct {
         ULONG NotificationCode;
         ULONG NotificationData;
      } PowerNotification;
      struct {
         PNP_VETO_TYPE VetoType;
         WCHAR DeviceIdVetoNameBuffer[ANYSIZE_ARRAY];
      } VetoNotification;
      struct {
         GUID BlockedDriverGuid;
      } BlockedDriverNotification;
   };
} PLUGPLAY_EVENT_BLOCK, *PPLUGPLAY_EVENT_BLOCK;

/*
 * NtGetPlugPlayEvent
 *
 * Returns one Plug & Play event from a global queue.
 *
 * Parameters
 *    Reserved1
 *    Reserved2
 *       Always set to zero.
 *
 *    Buffer
 *       The buffer that will be filled with the event information on
 *       successful return from the function.
 *
 *    BufferSize
 *       Size of the buffer pointed by the Buffer parameter. If the
 *       buffer size is not large enough to hold the whole event
 *       information, error STATUS_BUFFER_TOO_SMALL is returned and
 *       the buffer remains untouched.
 *
 * Return Values
 *    STATUS_PRIVILEGE_NOT_HELD
 *    STATUS_BUFFER_TOO_SMALL
 *    STATUS_SUCCESS
 *
 * Remarks
 *    This function isn't multi-thread safe!
NTSTATUS STDCALL
NtGetPlugPlayEvent(
   ULONG Reserved1,
   ULONG Reserved2,
   PPLUGPLAY_EVENT_BLOCK Buffer,
   ULONG BufferSize);
 */

/*
 * NtPlugPlayControl
 *
 * A function for doing various Plug & Play operations from user mode.
 *
 * Parameters
 *    ControlCode
 *       0x00   Reenumerate device tree
 *
 *              Buffer points to UNICODE_STRING decribing the instance
 *              path (like "HTREE\ROOT\0" or "Root\ACPI_HAL\0000"). For
 *              more information about instance paths see !devnode command
 *              in kernel debugger or look at "Inside Windows 2000" book,
 *              chapter "Driver Loading, Initialization, and Installation".
 *
 *       0x01   Register new device
 *       0x02   Deregister device
 *       0x03   Initialize device
 *       0x04   Start device
 *       0x06   Query and remove device
 *       0x07   User response
 *
 *              Called after processing the message from NtGetPlugPlayEvent.
 *
 *       0x08   Generate legacy device
 *       0x09   Get interface device list
 *       0x0A   Get property data
 *       0x0B   Device class association (Registration)
 *       0x0C   Get related device
 *       0x0D   Get device interface alias
 *       0x0E   Get/set/clear device status
 *       0x0F   Get device depth
 *       0x10   Query device relations
 *       0x11   Query target device relation
 *       0x12   Query conflict list
 *       0x13   Retrieve dock data
 *       0x14   Reset device
 *       0x15   Halt device
 *       0x16   Get blocked driver data
 *
 *    Buffer
 *       The buffer contains information that is specific to each control
 *       code. The buffer is read-only.
 *
 *    BufferSize
 *       Size of the buffer pointed by the Buffer parameter. If the
 *       buffer size specifies incorrect value for specified control
 *       code, error ??? is returned.
 *
 * Return Values
 *    STATUS_PRIVILEGE_NOT_HELD
 *    STATUS_SUCCESS
 *    ...
 */

#define PLUGPLAY_USER_RESPONSE      0x07
#define PLUGPLAY_GET_PROPERTY       0x0A
#define PLUGPLAY_GET_RELATED_DEVICE 0x0C
#define PLUGPLAY_DEVICE_STATUS      0x0E


typedef struct _PLUGPLAY_PROPERTY_DATA
{
  UNICODE_STRING DeviceInstance;
  ULONG Property;
  PVOID Buffer;
  ULONG BufferSize;
} PLUGPLAY_PROPERTY_DATA, *PPLUGPLAY_PROPERTY_DATA;


/* PLUGPLAY_GET_RELATED_DEVICE (Code 0x0C) */

/* Relation values */
#define PNP_GET_PARENT_DEVICE  1
#define PNP_GET_CHILD_DEVICE   2
#define PNP_GET_SIBLING_DEVICE 3

typedef struct _PLUGPLAY_RELATED_DEVICE_DATA
{
  UNICODE_STRING DeviceInstance;
  UNICODE_STRING RelatedDeviceInstance;
  ULONG Relation; /* 1: Parent  2: Child  3: Sibling */
} PLUGPLAY_RELATED_DEVICE_DATA, *PPLUGPLAY_RELATED_DEVICE_DATA;


/* PLUGPLAY_DEVICE_STATUS (Code 0x0E) */

/* Action values */
#define PNP_GET_DEVICE_STATUS    0
#define PNP_SET_DEVICE_STATUS    1
#define PNP_CLEAR_DEVICE_STATUS  2


typedef struct _PLUGPLAY_DEVICE_STATUS_DATA
{
  UNICODE_STRING DeviceInstance;
  ULONG Action;   /* 0: Get  1: Set  2: Clear */
  ULONG Problem;  /* CM_PROB_  see cfg.h */
  ULONG Flags;    /* DN_       see cfg.h */
} PLUGPLAY_DEVICE_STATUS_DATA, *PPLUGPLAY_DEVICE_STATUS_DATA;

#endif /* __GUIDS_ONLY__ */

#endif /* __NTPNP_H */
