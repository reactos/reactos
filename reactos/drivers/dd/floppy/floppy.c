//
//  FLOPPY.C - NEC-765/8272A floppy device driver
//    written by Rex Jolliff
//    with help from various other sources, including but not limited to:
//    Art Baker's NT Device Driver Book, Linux Source, and the internet.
//
//  Modification History:
//    08/19/98  RJJ  Created.
//


#include <ddk/ntddk.h>
#include "floppy.h"

#define  VERSION  "V0.0.1"

//  ---------------------------------------------------  File Statics

//  ------------------------------------------------------  Functions

//    ModuleEntry
//
//  DESCRIPTION:
//    This function initializes the driver, locates and claims 
//    hardware resources, and creates various NT objects needed
//    to process I/O requests.
//
//  RUN LEVEL:
//    PASSIVE_LEVEL
//
//  ARGUMENTS:
//    IN  PDRIVER_OBJECT   DriverObject  System allocated Driver Object
//                                       for this driver
//    IN  PUNICODE_STRING  RegistryPath  Name of registry driver service 
//                                       key
//
//  RETURNS:
//    NTSTATUS  

NTSTATUS ModuleEntry(
    IN PDRIVER_OBJECT DriverObject, 
    IN PUNICODE_STRING RegistryPath
    ) {
  NTSTATUS  RC;
  PFLOPPY_DEVICE_EXTENSION  DeviceExtension;

    //  Export other driver entry points...
  DriverObject->DriverStartIo = FloppyStartIo;
  DriverObject->MajorFunction[IRP_MJ_CREATE] = FloppyDispatchOpenClose;
  DriverObject->MajorFunction[IRP_MJ_CLOSE] = FloppyDispatchOpenClose;
  DriverObject->MajorFunction[IRP_MJ_READ] = FloppyDispatchReadWrite;
  DriverObject->MajorFunction[IRP_MJ_WRITE] = FloppyDispatchReadWrite;
  DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = FloppyDispatchDeviceControl;

    //  FIXME: Try to detect floppy and abort if it fails

    //  Create a device object for the floppy
  RC = IoCreateDevice(DriverObject, 
                      sizeof(FLOPPY_DEVICE_EXTENSION), 
                      DeviceName, 
                      FILE_DEVICE_DISK, 
                      0, 
                      TRUE, 
                      &DeviceObject);
  if (!NT_SUCCESS(RC)) {
    DPRINT("Could not create device for floppy");
    return  STATUS_NO_SUCH_DEVICE;
  }

    //  Fill out Device Extension
  DeviceExtension = (PFLOPPY_DEVICE_EXTENSION)
      DeviceObject->DeviceExtension;
  DeviceExtension->Number = 0;
  DeviceExtension->PortBase = FLOPPY_PORT_BASE;
  DeviceExtension->Vector = FLOPPY_IRQ_VECTOR;
  DeviceExtension->IrqL = FLOPPY_DIRQL;
  DeviceExtension->DMASupported = FALSE;
  DeviceExtension->FloppyState = FloppyStateIdle;

    //  Register an interrupt handler for this device
  RC = IoConnectInterrupt(&DeviceExtension->Interrupt,
      FloppyIsr, DeviceExtension, NULL, 
      DeviceExtension->Vector, DeviceExtension->IrqL, 
      DeviceExtension->IrqL, FLOPPY_IRQ_MODE, 
      FALSE, FLOPPY_AFFINITY, FALSE);
  if (!NT_SUCCESS(RC)) {
    DPRINT("Could not Connect Interrupt %d\n", DeviceExtension->Vector);
    return  STATUS_NO_SUCH_DEVICE;
  }

    // FIXME: register a DMA handler if needed for this controller
    
  return  STATUS_SUCCESS;
}

//    FloppyGetControllerVersion
//
//  DESCRIPTION
//    Get the type/version of the floppy controller
//
//  RUN LEVEL:
//    PASSIVE_LEVEL
//
//  ARGUMENTS:
//    IN OUT  PFLOPPY_DEVICE_EXTENSION  DeviceExtension
//
//  RETURNS:
//    BOOL  success or failure
//
//  COMMENTS:
//    This routine (get_fdc_version) was originally written by David C. Niemi
//
static  BOOLEAN  FloppyGetControllerVersion(
    IN OUT PFLOPPY_DEVICE_EXTENSION  DeviceExtension
    ) {
  int Result;
  FLOPPY_CMD  CommandToSend;
  FLOPPY_RESULT  ResultReturned

  CommandToSend.CMd[0] = FLOPPY_CMD_FD_DUMPREGS;
  FloppyWriteCommand(DeviceExtension->PortBase, &CommandToSend, &ResultReturned);

       /* 82072 and better know DUMPREGS */
        if (FDCS->reset)
                return FDC_NONE;
        if ((r = result()) <= 0x00)
                return FDC_NONE;        /* No FDC present ??? */
        if ((r==1) && (reply_buffer[0] == 0x80)){
                printk(KERN_INFO "FDC %d is an 8272A\n",fdc);
                return FDC_8272A;       /* 8272a/765 don't know DUMPREGS */
        }
        if (r != 10) {
                printk("FDC %d init: DUMPREGS: unexpected return of %d bytes.\n",
                       fdc, r);
                return FDC_UNKNOWN;
        }

        if(!fdc_configure()) {
                printk(KERN_INFO "FDC %d is an 82072\n",fdc);
                return FDC_82072;       /* 82072 doesn't know CONFIGURE */
        }

        output_byte(FD_PERPENDICULAR);
        if(need_more_output() == MORE_OUTPUT) {
                output_byte(0);
        } else {
                printk(KERN_INFO "FDC %d is an 82072A\n", fdc);
                return FDC_82072A;      /* 82072A as found on Sparcs. */
        }

        output_byte(FD_UNLOCK);
        r = result();
        if ((r == 1) && (reply_buffer[0] == 0x80)){
                printk(KERN_INFO "FDC %d is a pre-1991 82077\n", fdc);
                return FDC_82077_ORIG;  /* Pre-1991 82077, doesn't know 
                                         * LOCK/UNLOCK */
        }
        if ((r != 1) || (reply_buffer[0] != 0x00)) {
                printk("FDC %d init: UNLOCK: unexpected return of %d bytes.\n",
                       fdc, r);
                return FDC_UNKNOWN;
        }
        output_byte(FD_PARTID);
        r = result();
        if (r != 1) {
                printk("FDC %d init: PARTID: unexpected return of %d bytes.\n",
                       fdc, r);
                return FDC_UNKNOWN;
        }
        if (reply_buffer[0] == 0x80) {
                printk(KERN_INFO "FDC %d is a post-1991 82077\n",fdc);
                return FDC_82077;       /* Revised 82077AA passes all the tests */
        }
        switch (reply_buffer[0] >> 5) {
                case 0x0:
                        /* Either a 82078-1 or a 82078SL running at 5Volt */
                        printk(KERN_INFO "FDC %d is an 82078.\n",fdc);
                        return FDC_82078;
                case 0x1:
                        printk(KERN_INFO "FDC %d is a 44pin 82078\n",fdc);
                        return FDC_82078;
                case 0x2:
                        printk(KERN_INFO "FDC %d is a S82078B\n", fdc);
                        return FDC_S82078B;
                case 0x3:
                        printk(KERN_INFO "FDC %d is a National Semiconductor PC87306\n", fdc);
                        return FDC_87306;
                default:
                        printk(KERN_INFO "FDC %d init: 82078 variant with unknown PARTID=%d.\n",
                               fdc, reply_buffer[0] >> 5);
                        return FDC_82078_UNKN;
        }
} /* get_fdc_version */


static  NTSTATUS  FloppyStartIo(
    IN PDEVICE_OBJECT DeviceObject, 
    IN PIRP Irp
    ) {
  PIO_STACK_LOCATION         IrpStack;
  PFLOPPY_DEVICE_EXTENSION      DeviceExtension;
  
  IrpStack = IoGetCurrentIrpStackLocation(Irp);
  DeviceExtension = (PFLOPPY_DEVICE_EXTENSION) DeviceObject->DeviceExtension;

  

}

FloppyDispatchOpenClose

FloppyDispatchReadWrite

FloppyDispatchDeviceControl

FloppyIsr


