#pragma once

#ifndef _SWENUM_
#define _SWENUM_

#define IOCTL_SWENUM_INSTALL_INTERFACE \
  CTL_CODE(FILE_DEVICE_BUS_EXTENDER, 0x000, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_SWENUM_REMOVE_INTERFACE \
  CTL_CODE(FILE_DEVICE_BUS_EXTENDER, 0x001, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_SWENUM_GET_BUS_ID \
  CTL_CODE(FILE_DEVICE_BUS_EXTENDER, 0x002, METHOD_NEITHER, FILE_READ_ACCESS)

typedef struct _SWENUM_INSTALL_INTERFACE {
  GUID DeviceId;
  GUID InterfaceId;
  WCHAR ReferenceString[1];
} SWENUM_INSTALL_INTERFACE, *PSWENUM_INSTALL_INTERFACE;

#ifdef _KS_
#define STATIC_BUSID_SoftwareDeviceEnumerator STATIC_KSMEDIUMSETID_Standard
#define BUSID_SoftwareDeviceEnumerator KSMEDIUMSETID_Standard
#else
#define STATIC_BUSID_SoftwareDeviceEnumerator \
    0x4747B320L, 0x62CE, 0x11CF, {0xA5, 0xD6, 0x28, 0xDB, 0x04, 0xC1, 0x00, 0x00}
#endif /* _KS_ */

#ifdef _NTDDK_

#ifndef _KS_

typedef VOID
(NTAPI *PFNREFERENCEDEVICEOBJECT)(
  _In_ PVOID Context);

typedef VOID
(NTAPI *PFNDEREFERENCEDEVICEOBJECT)(
  _In_ PVOID Context);

typedef NTSTATUS
(NTAPI *PFNQUERYREFERENCESTRING)(
  _In_ PVOID Context,
  _Inout_ PWCHAR *String);

#endif /* !_KS_ */

#define BUS_INTERFACE_SWENUM_VERSION 0x100

typedef struct _BUS_INTERFACE_SWENUM {
  INTERFACE Interface;
  PFNREFERENCEDEVICEOBJECT ReferenceDeviceObject;
  PFNDEREFERENCEDEVICEOBJECT DereferenceDeviceObject;
  PFNQUERYREFERENCESTRING QueryReferenceString;
} BUS_INTERFACE_SWENUM, *PBUS_INTERFACE_SWENUM;

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KS_

KSDDKAPI
NTSTATUS
NTAPI
KsQuerySoftwareBusInterface(
  _In_ PDEVICE_OBJECT PnpDeviceObject,
  _Out_ PBUS_INTERFACE_SWENUM BusInterface);

KSDDKAPI
NTSTATUS
NTAPI
KsReferenceSoftwareBusObject(
  _In_ KSDEVICE_HEADER Header);

KSDDKAPI
VOID
NTAPI
KsDereferenceSoftwareBusObject(
  _In_ KSDEVICE_HEADER  Header);

_IRQL_requires_max_(PASSIVE_LEVEL)
KSDDKAPI
NTSTATUS
NTAPI
KsCreateBusEnumObject(
  _In_ PWSTR BusIdentifier,
  _In_ PDEVICE_OBJECT BusDeviceObject,
  _In_ PDEVICE_OBJECT PhysicalDeviceObject,
  _In_opt_ PDEVICE_OBJECT PnpDeviceObject,
  _In_opt_ REFGUID InterfaceGuid,
  _In_opt_ PWSTR ServiceRelativePath);

KSDDKAPI
NTSTATUS
NTAPI
KsGetBusEnumIdentifier(
  _Inout_ PIRP Irp);

_IRQL_requires_max_(PASSIVE_LEVEL)
KSDDKAPI
NTSTATUS
NTAPI
KsGetBusEnumPnpDeviceObject(
  _In_ PDEVICE_OBJECT DeviceObject,
  _Out_ PDEVICE_OBJECT *PnpDeviceObject);

_IRQL_requires_max_(PASSIVE_LEVEL)
KSDDKAPI
NTSTATUS
NTAPI
KsInstallBusEnumInterface(
  _In_ PIRP Irp);

_IRQL_requires_max_(PASSIVE_LEVEL)
KSDDKAPI
NTSTATUS
NTAPI
KsIsBusEnumChildDevice(
  _In_ PDEVICE_OBJECT DeviceObject,
  _Out_ PBOOLEAN ChildDevice);

_IRQL_requires_max_(PASSIVE_LEVEL)
KSDDKAPI
NTSTATUS
NTAPI
KsRemoveBusEnumInterface(
  _In_ PIRP Irp);

_IRQL_requires_max_(PASSIVE_LEVEL)
KSDDKAPI
NTSTATUS
NTAPI
KsServiceBusEnumPnpRequest(
  _In_ PDEVICE_OBJECT DeviceObject,
  _Inout_ PIRP Irp);

_IRQL_requires_max_(PASSIVE_LEVEL)
KSDDKAPI
NTSTATUS
NTAPI
KsServiceBusEnumCreateRequest(
  _In_ PDEVICE_OBJECT DeviceObject,
  _Inout_ PIRP Irp);

_IRQL_requires_max_(PASSIVE_LEVEL)
KSDDKAPI
NTSTATUS
NTAPI
KsGetBusEnumParentFDOFromChildPDO(
  _In_ PDEVICE_OBJECT DeviceObject,
  _Out_ PDEVICE_OBJECT *FunctionalDeviceObject);

#endif /* _KS_ */

#if defined(__cplusplus)
}
#endif

#endif /* _NTDDK_ */

#endif /* _SWENUM_ */
