/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             services/sdisk/sdisk.c
 * PURPOSE:          Disk driver for Bochs
 * PROGRAMMER:       David Welch (welch@mcmail.com)
 * UPDATE HISTORY: 
 */

/* INCLUDES ****************************************************************/

#include <ddk/ntddk.h>
#include <internal/hal/io.h>

#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS **************************************************************/

#define PORT      (0x3ec)

static VOID SdWriteOffset(ULONG Offset)
{
   outl_p(PORT,Offset);
}

NTSTATUS Dispatch(PDEVICE_OBJECT DeviceObject, PIRP Irp)
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
   PCH Buffer;
   ULONG Length;
   ULONG Information = 0;
   
   switch (Stack->MajorFunction)
     {
      case IRP_MJ_CREATE:
        DPRINT("Creating\n",0);
	status = STATUS_SUCCESS;
	break;
	
      case IRP_MJ_CLOSE:
	status = STATUS_SUCCESS;
	break;
	
      case IRP_MJ_WRITE:
        DPRINT("Writing %d bytes\n",
               Stack->Parameters.Write.Length);
	Length = Stack->Parameters.Write.Length;
	if ((Length%512)>0)
	  {
	     Length = Length - (Length%512);
	  }
	Buffer = MmGetSystemAddressForMdl(Irp->MdlAddress);
	DPRINT("Buffer %x\n",Buffer);
	#if 0
	for (i=0;i<Length;i++)
	  {
	     if ((i%512)==0)
	       {
		  DPRINT("Offset %x\n",
			   Stack->Parameters.Write.ByteOffset.LowPart+i);
		  SdWriteOffset(Stack->Parameters.Write.ByteOffset.LowPart+i);
	       }
	     outb_p(PORT,Buffer[i]);
	     DbgPrint("%c",Buffer[i]);
	  }
	#endif
	for (i=0;i<(Length/512);i++)
	  {
	     DPRINT("Offset %x\n",
		    Stack->Parameters.Write.ByteOffset.LowPart+i);
		  SdWriteOffset(Stack->Parameters.Write.ByteOffset.LowPart+i);	  
	     outsb(PORT,Buffer,512);
	  }
	status = STATUS_SUCCESS;
	Information = Length;
	break;
      
      case IRP_MJ_READ:
        DPRINT("Reading %d bytes\n",
               Stack->Parameters.Write.Length);
	Length = Stack->Parameters.Write.Length;
	if ((Length%512)>0)
	  {
	     Length = Length - (Length%512);
	  }
	Buffer = MmGetSystemAddressForMdl(Irp->MdlAddress);
	for (i=0;i<Length;i++)
	  {
	     if ((i%512)==0)
	       {
		  DPRINT("Offset %d\n",
			   Stack->Parameters.Write.ByteOffset.LowPart+i);
		  SdWriteOffset(Stack->Parameters.Write.ByteOffset.LowPart+i);
	       }
	     Buffer[i]=inb_p(PORT);
	  }
	status = STATUS_SUCCESS;
	break;
	
      default:
        status = STATUS_NOT_IMPLEMENTED;
	break;
     }
   
   Irp->IoStatus.Status = status;
   Irp->IoStatus.Information = Information;
   
   IoCompleteRequest(Irp, IO_NO_INCREMENT);
   return(status);
}

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
   ANSI_STRING asymlink;
   UNICODE_STRING usymlink;
   
   DbgPrint("Simple Disk Driver 0.0.1\n");
          
   RtlInitAnsiString(&astr,"\\Device\\SDisk");
   RtlAnsiStringToUnicodeString(&ustr,&astr,TRUE);
   ret = IoCreateDevice(DriverObject,0,&ustr,
                        FILE_DEVICE_DISK,0,FALSE,&DeviceObject);
   if (ret!=STATUS_SUCCESS)
     {
	return(ret);
     }
   
   RtlInitAnsiString(&asymlink,"\\??\\C:");
   RtlAnsiStringToUnicodeString(&usymlink,&asymlink,TRUE);
   IoCreateSymbolicLink(&usymlink,&ustr);
   
   DeviceObject->Flags=DO_DIRECT_IO;
   DriverObject->MajorFunction[IRP_MJ_CLOSE] = Dispatch;
   DriverObject->MajorFunction[IRP_MJ_CREATE] = Dispatch;
   DriverObject->MajorFunction[IRP_MJ_READ] = Dispatch;
   DriverObject->MajorFunction[IRP_MJ_WRITE] = Dispatch;
   DriverObject->DriverUnload = NULL;
   
   return(STATUS_SUCCESS);
}

