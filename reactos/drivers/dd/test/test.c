/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             services/test/test.c
 * PURPOSE:          Testing driver
 * PROGRAMMER:       David Welch (welch@mcmail.com)
 * UPDATE HISTORY: 
 *              ??/??/??: Created
 *              18/06/98: Made more NT like
 */

/* INCLUDES ****************************************************************/

#include <ddk/ntddk.h>

/* FUNCTIONS **************************************************************/

#if 0

NTSTATUS TestWrite(PIRP Irp, PIO_STACK_LOCATION Stk)
{
   PVOID Address;
   
   Address = MmGetSystemAddressForMdl(Irp->MdlAddress);
   DbgPrint("Asked to write '%s'\n",(PCH)Address);
   return(STATUS_SUCCESS);
}

NTSTATUS TestDispatch(PDEVICE_OBJECT DeviceObject, PIRP Irp)
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
   int i;
   
   switch (Stack->MajorFunction)
     {
      case IRP_MJ_CREATE:
        DbgPrint("(Test Driver) Creating\n");
	status = STATUS_SUCCESS;
	break;
	
      case IRP_MJ_CLOSE:
	status = STATUS_SUCCESS;
	break;
	
      case IRP_MJ_WRITE:
        DbgPrint("(Test Driver) Writing\n");
	status = TestWrite(Irp,Stack);
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

#endif

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
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
   ANSI_STRING astr;
   UNICODE_STRING ustr;
   
   DbgPrint("Test Driver 0.0.1\n");
   
   #if 0
   RtlInitAnsiString(&astr,"\\Device\\Test");
   RtlAnsiStringToUnicodeString(&ustr,&astr,TRUE);
   ret = IoCreateDevice(DriverObject,0,&ustr,
                        FILE_DEVICE_PARALLEL_PORT,0,FALSE,&DeviceObject);
   if (ret!=STATUS_SUCCESS)
     {
	return(ret);
     }

   DeviceObject->Flags=DO_DIRECT_IO;
   DriverObject->MajorFunction[IRP_MJ_CLOSE] = TestDispatch;
   DriverObject->MajorFunction[IRP_MJ_CREATE] = TestDispatch;
   DriverObject->MajorFunction[IRP_MJ_WRITE] = TestDispatch;
   DriverObject->MajorFunction[IRP_MJ_WRITE] = TestDispatch;
   DriverObject->DriverUnload = NULL;
   #endif
   return(STATUS_SUCCESS);
}

