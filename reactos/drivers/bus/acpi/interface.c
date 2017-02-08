#include "precomp.h"

#define NDEBUG
#include <debug.h>

VOID
NTAPI
AcpiInterfaceReference(PVOID Context)
{
  UNIMPLEMENTED;
}

VOID
NTAPI
AcpiInterfaceDereference(PVOID Context)
{
  UNIMPLEMENTED;
}

NTSTATUS
NTAPI
AcpiInterfaceConnectVector(PDEVICE_OBJECT Context,
                           ULONG GpeNumber,
                           KINTERRUPT_MODE Mode,
                           BOOLEAN Shareable,
                           PGPE_SERVICE_ROUTINE ServiceRoutine,
                           PVOID ServiceContext,
                           PVOID ObjectContext)
{
  UNIMPLEMENTED;

  return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
AcpiInterfaceDisconnectVector(PVOID ObjectContext)
{
  UNIMPLEMENTED;

  return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
AcpiInterfaceEnableEvent(PDEVICE_OBJECT Context,
                         PVOID ObjectContext)
{
  UNIMPLEMENTED;

  return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
AcpiInterfaceDisableEvent(PDEVICE_OBJECT Context,
                          PVOID ObjectContext)
{
  UNIMPLEMENTED;

  return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
AcpiInterfaceClearStatus(PDEVICE_OBJECT Context,
                         PVOID ObjectContext)
{
  UNIMPLEMENTED;

  return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
AcpiInterfaceNotificationsRegister(PDEVICE_OBJECT Context,
                                   PDEVICE_NOTIFY_CALLBACK NotificationHandler,
                                   PVOID NotificationContext)
{
  UNIMPLEMENTED;

  return STATUS_NOT_IMPLEMENTED;
}

VOID
NTAPI
AcpiInterfaceNotificationsUnregister(PDEVICE_OBJECT Context,
                                     PDEVICE_NOTIFY_CALLBACK NotificationHandler)
{
  UNIMPLEMENTED;
}

NTSTATUS
Bus_PDO_QueryInterface(PPDO_DEVICE_DATA DeviceData,
                       PIRP Irp)
{
  PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
  PACPI_INTERFACE_STANDARD AcpiInterface;

  if (IrpSp->Parameters.QueryInterface.Version != 1)
  {
      DPRINT1("Invalid version number: %d\n",
              IrpSp->Parameters.QueryInterface.Version);
      return STATUS_INVALID_PARAMETER;
  }

  if (RtlCompareMemory(IrpSp->Parameters.QueryInterface.InterfaceType,
                        &GUID_ACPI_INTERFACE_STANDARD, sizeof(GUID)) == sizeof(GUID))
  {
      DPRINT("GUID_ACPI_INTERFACE_STANDARD\n");

      if (IrpSp->Parameters.QueryInterface.Size < sizeof(ACPI_INTERFACE_STANDARD))
      {
          DPRINT1("Buffer too small! (%d)\n", IrpSp->Parameters.QueryInterface.Size);
          return STATUS_BUFFER_TOO_SMALL;
      }

     AcpiInterface = (PACPI_INTERFACE_STANDARD)IrpSp->Parameters.QueryInterface.Interface;

     AcpiInterface->InterfaceReference = AcpiInterfaceReference;
     AcpiInterface->InterfaceDereference = AcpiInterfaceDereference;
     AcpiInterface->GpeConnectVector = AcpiInterfaceConnectVector;
     AcpiInterface->GpeDisconnectVector = AcpiInterfaceDisconnectVector;
     AcpiInterface->GpeEnableEvent = AcpiInterfaceEnableEvent;
     AcpiInterface->GpeDisableEvent = AcpiInterfaceDisableEvent;
     AcpiInterface->GpeClearStatus = AcpiInterfaceClearStatus;
     AcpiInterface->RegisterForDeviceNotifications = AcpiInterfaceNotificationsRegister;
     AcpiInterface->UnregisterForDeviceNotifications = AcpiInterfaceNotificationsUnregister;

     return STATUS_SUCCESS;
  }
  else
  {
      DPRINT1("Invalid GUID\n");
      return STATUS_NOT_SUPPORTED;
  }
}
