/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             mkernel/modules/sound/sound.c
 * PURPOSE:          SoundBlaster 16 Driver
 * PROGRAMMER:       Snatched from David Welch (welch@mcmail.com)
 *									 Modified for Soundblaster by Robert Bergkvist (fragdance@hotmail.com)
 * UPDATE HISTORY: 
 *              ??/??/??: Created
 *              
 */

/* FUNCTIONS **************************************************************/

#include <internal/halio.h>
#include <ddk/ntddk.h>
#include <internal/hal/ddk.h>
#include <internal/dma.h>
#include <internal/mm.h>
#include <internal/string.h>
#include <devices.h>
#include "sb16.h"
#include "dsp.h"
#include "mixer.h"
#include "in.h"
#include "wave.h"


SB16 sb16;
sb_status sb16_getenvironment(void);

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
   
   switch (Stack->MajorFunction)
     {
      case IRP_MJ_CREATE:
      	printk("(SoundBlaster 16 Driver WaveOut) Creating\n");
 	reset_dsp(sb16.base);
       	status = STATUS_SUCCESS;
	break;
	
     case IRP_MJ_CLOSE:
	status = STATUS_SUCCESS;
	break;
	
      case IRP_MJ_WRITE:
        printk("(SoundBlaster 16 Driver) Writing %d bytes\n",Stack->Parameters.Write.Length);
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
 * FUNCTION: Called by the system to initalize the driver
 * ARGUMENTS:
 *           DriverObject = object describing this driver
 *           RegistryPath = path to our configuration entries
 * RETURNS: Success or failure
 */
{
	PDEVICE_OBJECT DeviceObject;
	NTSTATUS ret;
   
	printk("SoundBlaster 16 Driver 0.0.1\n");
  	if(sb16_getenvironment()!=SB_TRUE)
  	{
  		printk("Soundblaster 16 not found\n");
	  	return 0;
	}
	ret = IoCreateDevice(DriverObject,0,"\\Device\\WaveOut",FILE_DEVICE_WAVE_OUT,0,FALSE,&DeviceObject);
	if (ret!=STATUS_SUCCESS)
		return(ret);
		
	DeviceObject->Flags=0;
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = Dispatch;
	DriverObject->MajorFunction[IRP_MJ_CREATE] =Dispatch;
	DriverObject->MajorFunction[IRP_MJ_WRITE] = Dispatch;
	DriverObject->MajorFunction[IRP_MJ_WRITE] = Dispatch;
	DriverObject->DriverUnload = NULL;
   
	return(STATUS_SUCCESS);
}

sb_status sb16_getenvironment(void)
{
	if(detect_dsp(&sb16)!=SB_TRUE)
	{
		printk("Detect DSP failed!!!\n");
		return SB_FALSE;
	}
	printk("DSP base address 0x%x\n",sb16.base);
	get_irq(&sb16);
	printk("IRQ: %d\n",sb16.irq);
	get_dma(&sb16);
	printk("DMA8: 0x%x DMA16: 0x%x\n",sb16.dma8,sb16.dma16);
	return SB_TRUE;
}

#include "dsp.c"
#include "mixer.c"
#include "wave.c"
