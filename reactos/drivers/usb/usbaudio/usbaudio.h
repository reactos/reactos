#pragma once

#include <ntddk.h>
#include <portcls.h>
#include <ksmedia.h>
#include <hubbusif.h>
#include <usbbusif.h>
#include <usbioctl.h>
#include <usb.h>
#include <usbdlib.h>
#include <debug.h>

#define USBAUDIO_TAG 'AbsU'

typedef struct __DEVICE_EXTENSION__
{
    PDEVICE_OBJECT LowerDevice;                                  /* lower device*/
    PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor;       /* usb configuration descriptor */
    PUSB_DEVICE_DESCRIPTOR DeviceDescriptor;                     /* usb device descriptor */
    PUSBD_INTERFACE_INFORMATION InterfaceInfo;                   /* interface information */
    USBD_CONFIGURATION_HANDLE ConfigurationHandle;               /* configuration handle */

}DEVICE_EXTENSION, *PDEVICE_EXTENSION;

/* pool.c */
PVOID
NTAPI
AllocFunction(
    IN ULONG ItemSize);

VOID
NTAPI
FreeFunction(
    IN PVOID Item);

/* usbaudio.c */

NTSTATUS
NTAPI
USBAudioAddDevice(
  _In_ PKSDEVICE Device
);

NTSTATUS
NTAPI
USBAudioPnPStart(
  _In_     PKSDEVICE         Device,
  _In_     PIRP              Irp,
  _In_opt_ PCM_RESOURCE_LIST TranslatedResourceList,
  _In_opt_ PCM_RESOURCE_LIST UntranslatedResourceList
);

NTSTATUS
NTAPI
USBAudioPnPQueryStop(
  _In_ PKSDEVICE Device,
  _In_ PIRP      Irp
);

VOID
NTAPI
USBAudioPnPCancelStop(
  _In_ PKSDEVICE Device,
  _In_ PIRP      Irp
);

VOID
NTAPI
USBAudioPnPStop(
  _In_ PKSDEVICE Device,
  _In_ PIRP      Irp
);

NTSTATUS
NTAPI
USBAudioPnPQueryRemove(
  _In_ PKSDEVICE Device,
  _In_ PIRP      Irp
);

VOID
NTAPI
USBAudioPnPCancelRemove(
  _In_ PKSDEVICE Device,
  _In_ PIRP      Irp
);

VOID
NTAPI
USBAudioPnPRemove(
  _In_ PKSDEVICE Device,
  _In_ PIRP      Irp
);

NTSTATUS
NTAPI
USBAudioPnPQueryCapabilities(
  _In_    PKSDEVICE            Device,
  _In_    PIRP                 Irp,
  _Inout_ PDEVICE_CAPABILITIES Capabilities
);

VOID
NTAPI
USBAudioPnPSurpriseRemoval(
  _In_ PKSDEVICE Device,
  _In_ PIRP      Irp
);

NTSTATUS
NTAPI
USBAudioPnPQueryPower(
  _In_ PKSDEVICE          Device,
  _In_ PIRP               Irp,
  _In_ DEVICE_POWER_STATE DeviceTo,
  _In_ DEVICE_POWER_STATE DeviceFrom,
  _In_ SYSTEM_POWER_STATE SystemTo,
  _In_ SYSTEM_POWER_STATE SystemFrom,
  _In_ POWER_ACTION       Action
);

VOID
NTAPI
USBAudioPnPSetPower(
  _In_ PKSDEVICE          Device,
  _In_ PIRP               Irp,
  _In_ DEVICE_POWER_STATE To,
  _In_ DEVICE_POWER_STATE From
);
