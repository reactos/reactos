/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             services/null/null.c
 * PURPOSE:          NULL device driver
 * PROGRAMMER:       David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *              13/08/98: Created
 */

/* INCLUDES ****************************************************************/

#include <ddk/ntddk.h>

/* FUNCTIONS **************************************************************/

NTSTATUS NullWrite(PIRP Irp, PIO_STACK_LOCATION stk)
{
   Irp->IoStatus.Information = stk->Parameters.Write.Length;
   return(STATUS_SUCCESS);
}

NTSTATUS NullRead(PIRP Irp, PIO_STACK_LOCATION stk)
{
   Irp->IoStatus.Information = 0;
   return(STATUS_END_OF_FILE);
}

NTSTATUS NullDispatch(PDEVICE_OBJECT DeviceObject, PIRP Irp)
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
   
   switch (Stack->MajorFunction)
     {
      case IRP_MJ_CREATE:
      case IRP_MJ_CLOSE:
	status = STATUS_SUCCESS;
	break;
	
      case IRP_MJ_WRITE:
	status = NullWrite(Irp,Stack);
        break;
	
      case IRP_MJ_READ:
	status = NullRead(Irp,Stack);
        break;
	
      default:
        status = STATUS_NOT_IMPLEMENTED;
     }

   Irp->IoStatus.Status = status;
   IoCompleteRequest(Irp,IO_NO_INCREMENT);
   return(status);
}

NTSTATUS NullUnload(PDRIVER_OBJECT DriverObject)
{
   return(STATUS_SUCCESS);
}

NTSTATUS STDCALL
DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
/*
 * FUNCTION: Called by the system to initalize the driver
 * ARGUMENTS:
 *           DriverObject = object describing this driver
 *           RegistryPath = path to our configuration entries
 * RETURNS: Success or failure
 */
{
   PDEVICE_OBJECT DeviceObject;
   NTSTATUS ret;
   ANSI_STRING ansi_device_name;
   UNICODE_STRING device_name;
   
   DbgPrint("Null Device Driver 0.0.2\n");
   
   RtlInitAnsiString(&ansi_device_name,"\\Device\\NUL");
   RtlAnsiStringToUnicodeString(&device_name,&ansi_device_name,TRUE);
   ret = IoCreateDevice(DriverObject,0,&device_name,
                        FILE_DEVICE_NULL,0,FALSE,&DeviceObject);
   if (ret!=STATUS_SUCCESS)
     {
	return(ret);
     }

   DeviceObject->Flags=0;
   DriverObject->MajorFunction[IRP_MJ_CLOSE] = NullDispatch;
   DriverObject->MajorFunction[IRP_MJ_CREATE] = NullDispatch;
   DriverObject->MajorFunction[IRP_MJ_WRITE] = NullDispatch;
   DriverObject->MajorFunction[IRP_MJ_READ] = NullDispatch;
   DriverObject->DriverUnload = NullUnload;
   
   return(STATUS_SUCCESS);
}

