/*++

Copyright (c) 1998-2001 Klaus P. Gerlicher

Module Name:

    privateice.c

Abstract:

Environment:

Author:

    Klaus P. Gerlicher

	reactos port by:
 			Eugene Ingerman

Revision History:

    16-Jul-1998:	created
    15-Nov-2000:    general cleanup of source files
    19-Jan-2001:    renamed to privateice.c

	10/20/2001:		porting to reactos begins

Copyright notice:

  This file may be distributed under the terms of the GNU Public License.

--*/

////////////////////////////////////////////////////
// INCLUDES
////
/*
#include <linux/kernel.h>
#include <linux/module.h>
#include <asm/uaccess.h>
#include <linux/fs.h>
#include <linux/config.h>
#include <linux/sched.h>
#include <asm/unistd.h>
#include <linux/string.h>
*/

#include <ddk/ntddk.h>
#include <debug.h>

#include "precomp.h"
#include "serial.h"

////////////////////////////////////////////////////
// GLOBALS
////

BOOLEAN bDeviceAlreadyOpen = FALSE;

char tempPICE[1024];

////////////////////////////////////////////////////
// FUNCTIONS
////

//*************************************************************************
// pice_open()
//
//*************************************************************************

NTSTATUS STDCALL pice_open(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    DPRINT((0,"pice_open\n"));

    /* We don't want to talk to two processes at the
    * same time */
    if (bDeviceAlreadyOpen){
		IoCompleteRequest (Irp, IO_NO_INCREMENT);
		return STATUS_UNSUCCESSFUL;     /* is there a more descriptive status code for this case? */
	}

    bDeviceAlreadyOpen = TRUE;
	IoCompleteRequest (Irp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

//*************************************************************************
// pice_close()
//
//*************************************************************************
NTSTATUS STDCALL pice_close(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    DPRINT((0,"pice_close\n"));

	CleanUpPICE();                      // used to be in cleanup_module

	/* We're now ready for our next caller */
    bDeviceAlreadyOpen = FALSE;
	IoCompleteRequest (Irp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}


//*************************************************************************
// pice_ioctl()
//
//*************************************************************************

NTSTATUS STDCALL pice_ioctl(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
//	char* pFilename = (char*) ioctl_param;

	PIO_STACK_LOCATION IoStack = IoGetCurrentIrpStackLocation( Irp );

	ULONG Code = IoStack->Parameters.DeviceIoControl.IoControlCode;

	switch(Code)
	{
		case PICE_IOCTL_LOAD:
            break;
		case PICE_IOCTL_RELOAD:
            if(!ReloadSymbols())
            {
			    PICE_sprintf(tempPICE,"pICE: not able to reload symbols\n");
			    Print(OUTPUT_WINDOW,tempPICE);
            }
			break;
		case PICE_IOCTL_UNLOAD:
            UnloadSymbols();
			break;
		case PICE_IOCTL_BREAK:
			PICE_sprintf(tempPICE,"pICE: forcible break\n");
			Print(OUTPUT_WINDOW,tempPICE);
            __asm__ __volatile("int $3");
            break;
		case PICE_IOCTL_STATUS:
            {
                PDEBUGGER_STATUS_BLOCK ustatus_block_p;
                DEBUGGER_STATUS_BLOCK kstatus_block;

				ULONG OutLength = IoStack->Parameters.DeviceIoControl.OutputBufferLength;
				if( OutLength < sizeof( DEBUGGER_STATUS_BLOCK ) ){
					return STATUS_INVALID_PARAMETER;
				}

				ustatus_block_p = (PDEBUGGER_STATUS_BLOCK)Irp->AssociatedIrp.SystemBuffer;

                //kstatus_block.Test = 0x12345678;
                RtlCopyMemory(ustatus_block_p, &kstatus_block, sizeof(DEBUGGER_STATUS_BLOCK) );
            }
            break;
        default:
			IoCompleteRequest (Irp, IO_NO_INCREMENT);
			return STATUS_INVALID_PARAMETER;
	}
	IoCompleteRequest (Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}


NTSTATUS STDCALL DriverEntry(PDRIVER_OBJECT DriverObject,
			     PUNICODE_STRING RegistryPath)
/*
 * FUNCTION: Module entry point
 */
{
   PDEVICE_OBJECT DeviceObject;
   UNICODE_STRING DeviceName;
   UNICODE_STRING SymlinkName;

   DPRINT((0,"PICE Debugger\n"));

#if 0                                   // don't enable before completely ported
#ifdef DEBUG
    // first we enable output of debug strings to COM port
    DebugSetupSerial(1,115200);
#endif // DEBUG
#endif

   if(InitPICE()){
		DriverObject->MajorFunction[IRP_MJ_CREATE] = pice_open;
		//ei unimplemented DriverObject->MajorFunction[IRP_MJ_CLOSE] = pice_close;
		DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = pice_ioctl;

		RtlInitUnicodeStringFromLiteral(&DeviceName, L"\\Device\\Pice");
		IoCreateDevice(DriverObject,
				0,
				&DeviceName,
				PICE_DEVICE_DEBUGGER,
				0,
				TRUE,
				&DeviceObject);
		DeviceObject->Flags = DeviceObject->Flags | DO_BUFFERED_IO;

		RtlInitUnicodeStringFromLiteral(&SymlinkName, L"\\??\\Pice");
		IoCreateSymbolicLink(&SymlinkName, &DeviceName);

		return(STATUS_SUCCESS);
   }
}

