/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             services/parallel/parallel.c
 * PURPOSE:          Parallel port driver
 * PROGRAMMER:       David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *              ??/??/??: Created
 *              18/06/98: Made more NT like
 */

/* FUNCTIONS **************************************************************/

#include <wdm.h>

#include "parallel.h"

#define NDEBUG
#include <debug.h>


#define LP_B (0x378)
#define LP_S (READ_PORT_UCHAR((PUCHAR)(LP_B+1)))
#define LP_C (LP_B+2)

NTSTATUS NTAPI
DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath);

static void Parallel_Reset(void)
/*
 * FUNCTION: Resets the device attached to the parallel port
 */
{
   int i;

   WRITE_PORT_UCHAR((PUCHAR)LP_C,0);
   for (i=0;i<LP_DELAY;i++);
   WRITE_PORT_UCHAR((PUCHAR)LP_C,LP_PSELECP | LP_PINITP);
}

static void Parallel_putchar(unsigned char ch)
/*
 * FUNCTION: Writes a character to the parallel port
 * ARGUMENTS:
 *          ch = character to write
 */
{

	int count=0;
	int status;
	int wait=0;

	do
	  {
	     status=LP_S;
	     count++;
	  }
	while ( count < 500000 && !(status & LP_PBUSY) );

	if (count==500000)
	  {
	     DPRINT("printer_putchar(): timed out\n");
	     return;
	  }

	WRITE_PORT_UCHAR((PUCHAR)LP_B,ch);
	while (wait != 10000) { wait++; }
	WRITE_PORT_UCHAR((PUCHAR)LP_C, (LP_PSELECP | LP_PINITP | LP_PSTROBE ));
	while (wait) { wait--; }
	WRITE_PORT_UCHAR((PUCHAR)LP_C, LP_PSELECP | LP_PINITP);
}

static DRIVER_DISPATCH Dispatch;
static NTSTATUS NTAPI
Dispatch(PDEVICE_OBJECT DeviceObject, PIRP Irp)
/*
 * FUNCTION: Handles user mode requests
 * ARGUMENTS:
 *           DeviceObject = Device for request
 *           Irp = I/O request packet describing request
 * RETURNS: Success or failure
 */
{
   PIO_STACK_LOCATION Stack = IoGetCurrentIrpStackLocation(Irp);
   NTSTATUS status;
   ULONG i;

   switch (Stack->MajorFunction)
     {
      case IRP_MJ_CREATE:
	DPRINT("(Parallel Port Driver) Creating\n");
	Parallel_Reset();
	status = STATUS_SUCCESS;
	break;

      case IRP_MJ_CLOSE:
	status = STATUS_SUCCESS;
	break;

      case IRP_MJ_WRITE:
	DPRINT("(Parallel Port Driver) Writing %d bytes\n",
	       Stack->Parameters.Write.Length);
	for (i=0;i<Stack->Parameters.Write.Length;i++)
	  {
	     Parallel_putchar(((char *)Irp->UserBuffer)[i]);
	  }
	status = STATUS_SUCCESS;
	break;

      default:
	status = STATUS_NOT_IMPLEMENTED;
	break;
     }

   Irp->IoStatus.Status = status;
   Irp->IoStatus.Information = 0;

   IoCompleteRequest(Irp, IO_NO_INCREMENT);
   return(status);
}

NTSTATUS NTAPI
DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
/*
 * FUNCTION: Called by the system to initialize the driver
 * ARGUMENTS:
 *           DriverObject = object describing this driver
 *           RegistryPath = path to our configuration entries
 * RETURNS: Success or failure
 */
{
   PDEVICE_OBJECT DeviceObject;
   UNICODE_STRING DeviceName = RTL_CONSTANT_STRING(L"\\Device\\Parallel");
   NTSTATUS Status;

   DPRINT("Parallel Port Driver 0.0.1\n");

   Status = IoCreateDevice(DriverObject,
			   0,
			   &DeviceName,
			   FILE_DEVICE_PARALLEL_PORT,
			   0,
			   FALSE,
			   &DeviceObject);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }

   DeviceObject->Flags=0;
   DriverObject->MajorFunction[IRP_MJ_CLOSE] = Dispatch;
   DriverObject->MajorFunction[IRP_MJ_CREATE] = Dispatch;
   DriverObject->MajorFunction[IRP_MJ_WRITE] = Dispatch;
   DriverObject->DriverUnload = NULL;

   return(STATUS_SUCCESS);
}

/* EOF */
