/* $Id$
 * 
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/po/power.c
 * PURPOSE:         Power Manager
 * 
 * PROGRAMMERS:     Casper S. Hornstrup (chorns@users.sourceforge.net)
 */

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

PDEVICE_NODE PopSystemPowerDeviceNode = NULL;
BOOLEAN PopAcpiPresent = FALSE;

/*
 * @implemented
 */
NTSTATUS
STDCALL
PoCallDriver(
  IN PDEVICE_OBJECT DeviceObject,
  IN OUT PIRP Irp)
{
  NTSTATUS Status;

  Status = IoCallDriver(DeviceObject, Irp);

  return Status;
}

/*
 * @unimplemented
 */
PULONG
STDCALL
PoRegisterDeviceForIdleDetection(
  IN PDEVICE_OBJECT DeviceObject,
  IN ULONG ConservationIdleTime,
  IN ULONG PerformanceIdleTime,
  IN DEVICE_POWER_STATE State)
{
  return NULL;
}

/*
 * @unimplemented
 */
PVOID
STDCALL
PoRegisterSystemState(
  IN PVOID StateHandle,
  IN EXECUTION_STATE Flags)
{
  return NULL;
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
PoRequestPowerIrp(
  IN PDEVICE_OBJECT DeviceObject,
  IN UCHAR MinorFunction,  
  IN POWER_STATE PowerState,
  IN PREQUEST_POWER_COMPLETE CompletionFunction,
  IN PVOID Context,
  OUT PIRP *Irp   OPTIONAL)
{
  return STATUS_NOT_IMPLEMENTED;
}

VOID
STDCALL
PoSetDeviceBusy(
  PULONG IdlePointer)
{
}

/*
 * @unimplemented
 */
POWER_STATE
STDCALL
PoSetPowerState(
  IN PDEVICE_OBJECT DeviceObject,
  IN POWER_STATE_TYPE Type,
  IN POWER_STATE State)
{
  POWER_STATE ps;
  
  ASSERT_IRQL(DISPATCH_LEVEL);

  ps.SystemState = PowerSystemWorking;  // Fully on
  ps.DeviceState = PowerDeviceD0;       // Fully on

  return ps;
}

/*
 * @unimplemented
 */
VOID
STDCALL
PoSetSystemState(
  IN EXECUTION_STATE Flags)
{
}

/*
 * @unimplemented
 */
VOID
STDCALL
PoStartNextPowerIrp(
  IN PIRP Irp)
{
}

/*
 * @unimplemented
 */
VOID
STDCALL
PoUnregisterSystemState(
  IN PVOID StateHandle)
{
}

NTSTATUS
PopSetSystemPowerState(
  SYSTEM_POWER_STATE PowerState)
{    
  IO_STATUS_BLOCK IoStatusBlock;
  PDEVICE_OBJECT DeviceObject;
  PIO_STACK_LOCATION IrpSp;
  PDEVICE_OBJECT Fdo;
  NTSTATUS Status;
  KEVENT Event;
  PIRP Irp;
  
  if (!PopAcpiPresent) return STATUS_NOT_IMPLEMENTED;

  Status = IopGetSystemPowerDeviceObject(&DeviceObject);
  if (!NT_SUCCESS(Status)) {
    CPRINT("No system power driver available\n");
    return STATUS_UNSUCCESSFUL;
  }

  Fdo = IoGetAttachedDeviceReference(DeviceObject);

  if (Fdo == DeviceObject)
    {
      DPRINT("An FDO was not attached\n");
      return STATUS_UNSUCCESSFUL;
    }

  KeInitializeEvent(&Event,
	  NotificationEvent,
	  FALSE);

  Irp = IoBuildSynchronousFsdRequest(IRP_MJ_POWER,
    Fdo,
	  NULL,
	  0,
	  NULL,
	  &Event,
	  &IoStatusBlock);

  IrpSp = IoGetNextIrpStackLocation(Irp);
  IrpSp->MinorFunction = IRP_MN_SET_POWER;
  IrpSp->Parameters.Power.Type = SystemPowerState;
  IrpSp->Parameters.Power.State.SystemState = PowerState;

	Status = PoCallDriver(Fdo, Irp);
	if (Status == STATUS_PENDING)
	  {
		  KeWaitForSingleObject(&Event,
		                        Executive,
		                        KernelMode,
		                        FALSE,
		                        NULL);
      Status = IoStatusBlock.Status;
    }

  ObDereferenceObject(Fdo);

  return Status;
}

VOID 
INIT_FUNCTION
PoInit(PLOADER_PARAMETER_BLOCK LoaderBlock, 
       BOOLEAN ForceAcpiDisable)
{
  if (ForceAcpiDisable)
    {
      /* Set the ACPI State to False if it's been forced that way */
      PopAcpiPresent = FALSE;
    }
  else
    {
      /* Otherwise check the LoaderBlock's Flag */
      PopAcpiPresent = (LoaderBlock->Flags & MB_FLAGS_ACPI_TABLE) ? TRUE : FALSE;
    }
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
NtInitiatePowerAction (
	IN POWER_ACTION SystemAction,
	IN SYSTEM_POWER_STATE MinSystemState,
 	IN ULONG Flags,
	IN BOOLEAN Asynchronous)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSTATUS 
STDCALL 
NtPowerInformation(
	IN POWER_INFORMATION_LEVEL PowerInformationLevel,
	IN PVOID InputBuffer  OPTIONAL,
	IN ULONG InputBufferLength,
	OUT PVOID OutputBuffer  OPTIONAL,
	IN ULONG OutputBufferLength
	)
{
   NTSTATUS Status;
   
   PAGED_CODE();

   DPRINT("NtPowerInformation(PowerInformationLevel 0x%x, InputBuffer 0x%x, "
          "InputBufferLength 0x%x, OutputBuffer 0x%x, OutputBufferLength 0x%x)\n",
          PowerInformationLevel,
          InputBuffer, InputBufferLength,
          OutputBuffer, OutputBufferLength);
   switch (PowerInformationLevel)
   {
   case SystemBatteryState:
      {
      PSYSTEM_BATTERY_STATE BatteryState = (PSYSTEM_BATTERY_STATE)OutputBuffer;

      if (InputBuffer != NULL)
         return STATUS_INVALID_PARAMETER;
      if (OutputBufferLength < sizeof(SYSTEM_BATTERY_STATE))
         return STATUS_BUFFER_TOO_SMALL;

      /* Just zero the struct (and thus set BatteryState->BatteryPresent = FALSE) */
      RtlZeroMemory(BatteryState, sizeof(SYSTEM_BATTERY_STATE));
      BatteryState->EstimatedTime = (ULONG)-1;

      Status = STATUS_SUCCESS;
      break;
      }

   default:
      Status = STATUS_NOT_IMPLEMENTED;
      DPRINT1("PowerInformationLevel 0x%x is UNIMPLEMENTED! Have a nice day.\n",
              PowerInformationLevel);
      for (;;);
      break;
   }

   return Status;
}

/* EOF */
