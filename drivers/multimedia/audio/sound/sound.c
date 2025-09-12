/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             mkernel/modules/sound/sound.c
 * PURPOSE:          SoundBlaster 16 Driver
 * PROGRAMMER:       Snatched from David Welch (welch@mcmail.com)
 *		     Modified for Soundblaster by Robert Bergkvist (fragdance@hotmail.com)
 * UPDATE HISTORY:
 *              ??/??/??: Created
 *
 */

/* FUNCTIONS **************************************************************/

#include <ntddk.h>
#include <string.h>
#include <devices.h>
#include "sb16.h"
#include "dsp.h"
#include "mixer.h"
#include "wave.h"

#define NDEBUG
#include <debug.h>

SB16 sb16;
sb_status sb16_getenvironment(void);

#if 0
static NTSTATUS NTAPI Dispatch(PDEVICE_OBJECT DeviceObject, PIRP Irp)
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
      	DPRINT1("(SoundBlaster 16 Driver WaveOut) Creating\n");
 	reset_dsp(sb16.base);
       	status = STATUS_SUCCESS;
	break;

     case IRP_MJ_CLOSE:
	status = STATUS_SUCCESS;
	break;

      case IRP_MJ_WRITE:
        DPRINT1("(SoundBlaster 16 Driver) Writing %d bytes\n",Stack->Parameters.Write.Length);
        sb16_play((WAVE_HDR*)Irp->UserBuffer);
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

NTSTATUS ModuleEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
/*
 * FUNCTION: Called by the system to initialize the driver
 * ARGUMENTS:
 *           DriverObject = object describing this driver
 *           RegistryPath = path to our configuration entries
 * RETURNS: Success or failure
 */
{
#if 0
	PDEVICE_OBJECT DeviceObject;
	NTSTATUS ret;

	DPRINT1("SoundBlaster 16 Driver 0.0.1\n");
  	if(sb16_getenvironment()!=SB_TRUE)
  	{
  		DPRINT1("Soundblaster 16 not found\n");
	  	return 0;
	}
	ret = IoCreateDevice(DriverObject,0,L"\\Device\\WaveOut",FILE_DEVICE_WAVE_OUT,0,FALSE,&DeviceObject);
	if (ret!=STATUS_SUCCESS)
		return(ret);

	DeviceObject->Flags=0;
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = Dispatch;
	DriverObject->MajorFunction[IRP_MJ_CREATE] =Dispatch;
	DriverObject->MajorFunction[IRP_MJ_WRITE] = Dispatch;
	DriverObject->MajorFunction[IRP_MJ_WRITE] = Dispatch;
	DriverObject->DriverUnload = NULL;
#endif
	return(STATUS_SUCCESS);
}
#endif

sb_status sb16_getenvironment(void)
{
	if(detect_dsp(&sb16)!=SB_TRUE)
	{
		DPRINT1("Detect DSP failed!!!\n");
		return SB_FALSE;
	}
	DPRINT1("DSP base address 0x%x\n",sb16.base);
	get_irq(&sb16);
	DPRINT1("IRQ: %d\n",sb16.irq);
	get_dma(&sb16);
	DPRINT1("DMA8: 0x%x DMA16: 0x%x\n",sb16.dma8,sb16.dma16);
	return SB_TRUE;
}

