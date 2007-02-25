/*
    Routines to simplify the use of DMA for Sound Blaster driver
*/

#include <ntddk.h>
#include "sndblst.h"

#if 0
BOOLEAN CheckDMA(PDEVICE_EXTENSION Device)
{
    // Don't forget to check for Compaq machines (they can't be configured
    // manually...)

    return TRUE;    // for now...

// if (! CompaqBA)
// return (BOOLEAN) !((INPORT(pHw, BOARD_ID) & 0x80) && Device->DMA == 0);
// else
// {
//    UCHAR CompaqPIDR;
//    UCHAR Expected = (UCHAR)(Device->DMA == 0 ? 0x40 :
//                             Device->DMA == 1 ? 0x80 :
//                             0xc0);
// CompaqPIDR = READ_PORT_UCHAR(pHw->CompaqBA + BOARD_ID);
// if (CompaqPIDR != 0xff)
// {
//    if ((UCHAR)(CompaqPIDR & 0xc0) == Expected)
//        return true;
// }
// }

    return FALSE;
}
#endif


static IO_ALLOCATION_ACTION STDCALL SoundProgramDMA(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID MapRegisterBase,
    IN PVOID Context)
{
    PDEVICE_EXTENSION Device = DeviceObject->DeviceExtension;
    ULONG zzz;
    PUCHAR VirtualAddress = (PUCHAR) MmGetMdlVirtualAddress(Device->Mdl);

    DPRINT("IoMapTransfer\n");
    IoMapTransfer(Device->Adapter,
                    Device->Mdl,
                    MapRegisterBase,
                    (PUCHAR) MmGetMdlVirtualAddress(Device->Mdl),
                    &Device->BufferSize,    // is this right?
                    TRUE);

    DPRINT("VBuffer == 0x%x (really 0x%x?) Bufsize == %u\n", Device->VirtualBuffer, MmGetPhysicalAddress(Device->VirtualBuffer), Device->BufferSize);

    DPRINT("Writing %u bytes of garbage...\n", Device->BufferSize);
    // Write some garbage:
    for (zzz = 0; zzz < Device->BufferSize; zzz ++)
        *(VirtualAddress + zzz) = (UCHAR) zzz % 200;

    DPRINT("done\n");

    KeSetEvent(Context, 0, FALSE);

    return KeepObject;
}


BOOLEAN CreateDMA(PDEVICE_OBJECT DeviceObject)
{
    DEVICE_DESCRIPTION Desc;
    ULONG MappedRegs = 0;
    PDEVICE_EXTENSION Device = DeviceObject->DeviceExtension;
    KEVENT DMAEvent;
    KIRQL OldIrql;

    // Buffersize should already be set but it isn't yet !
    Device->BufferSize = SB_BUFSIZE;
    DPRINT("Bufsize == %u\n", Device->BufferSize);

    RtlZeroMemory(&Desc, sizeof(DEVICE_DESCRIPTION));

    // Init memory!
    Desc.Version = DEVICE_DESCRIPTION_VERSION;
    Desc.Master = FALSE;    // Slave
    Desc.ScatterGather = FALSE; // Don't think so anyway
    Desc.DemandMode = FALSE;    // == !SingleModeDMA
    Desc.AutoInitialize = TRUE; // ?
    Desc.Dma32BitAddresses = FALSE; // I don't think we can
    Desc.IgnoreCount = FALSE; // Should be OK
    Desc.Reserved1 = 0;
//    Desc.Reserved2 = 0;
    Desc.BusNumber = 0;
    Desc.DmaChannel = Device->DMA;    // Our channel :)
    Desc.InterfaceType = Isa;   // (BusType == MicroChannel) ? MicroChannel : Isa;
    Desc.DmaWidth = 0;    // hmm... 8 bits?
    Desc.DmaSpeed = 0;     // double hmm (Compatible it should be)
    Desc.MaximumLength = Device->BufferSize;
//    Desc.MinimumLength = 0;
    Desc.DmaPort = 0;

    DPRINT("Calling HalGetAdapter(), asking for %d mapped regs\n", MappedRegs);

    Device->Adapter = HalGetAdapter(&Desc, &MappedRegs);

    DPRINT("Called\n");

    if (! Device->Adapter)
    {
        DPRINT("HalGetAdapter() FAILED\n");
        return FALSE;
    }

    DPRINT("Bufsize == %u\n", Device->BufferSize);

    if (MappedRegs < BYTES_TO_PAGES(Device->BufferSize))
    {
        DPRINT("Could only allocate %u mapping registers\n", MappedRegs);

        if (MappedRegs == 0)
            return FALSE;

        Device->BufferSize = MappedRegs * PAGE_SIZE;
        DPRINT("Bufsize == %u\n", Device->BufferSize);
    }

    DPRINT("Allocated %u mapping registers\n", MappedRegs);

    // Check if we already have memory here...

    // Check to make sure we're >= minimum

    DPRINT("Allocating buffer\n");

    DPRINT("Bufsize == %u\n", Device->BufferSize);

    Device->VirtualBuffer = HalAllocateCommonBuffer(Device->Adapter, Device->BufferSize,
                                                &Device->Buffer, FALSE);

    // For some reason BufferSize == 0 here?!
//    DPRINT("Buffer == 0x%x Bufsize == %u\n", Device->Buffer, Device->BufferSize);
    DPRINT("Bufsize == %u,", Device->BufferSize);
    DPRINT("Buffer == 0x%x\n", Device->Buffer);

    if (! Device->VirtualBuffer)
    {
        DPRINT("Could not allocate buffer :(\n");
        // should try again with smaller buffer...
        return FALSE;
    }

//    DPRINT("Buffer == 0x%x Bufsize == %u\n", Device->Buffer, Device->BufferSize);
    DPRINT("Bufsize == %u,", Device->BufferSize);
    DPRINT("Buffer == 0x%x\n", Device->Buffer);

    DPRINT("Calling IoAllocateMdl()\n");
    Device->Mdl = IoAllocateMdl(Device->VirtualBuffer, Device->BufferSize, FALSE, FALSE, NULL);
    DPRINT("Bufsize == %u\n", Device->BufferSize);

    // IS THIS RIGHT:
    if (! Device->Mdl)
    {
        DPRINT("IoAllocateMdl() FAILED\n");
        // Free the HAL buffer
        return FALSE;
    }

    DPRINT("VBuffer == 0x%x Mdl == %u Bufsize == %u\n", Device->VirtualBuffer, Device->Mdl, Device->BufferSize);

    DPRINT("Calling MmBuildMdlForNonPagedPool\n");
    MmBuildMdlForNonPagedPool(Device->Mdl);

    DPRINT("Bufsize == %u\n", Device->BufferSize);

    // part II:
    KeInitializeEvent(&DMAEvent, SynchronizationEvent, FALSE);
    // Raise IRQL
    KeRaiseIrql(DISPATCH_LEVEL,&OldIrql);
    IoAllocateAdapterChannel(Device->Adapter, DeviceObject,
                            BYTES_TO_PAGES(Device->BufferSize),
                            SoundProgramDMA, &DMAEvent);
    // Lower IRQL
    KeLowerIrql(OldIrql);
    DPRINT("VBuffer == 0x%x Bufsize == %u\n", Device->VirtualBuffer, Device->BufferSize);
    KeWaitForSingleObject(&DMAEvent, Executive, KernelMode, FALSE, NULL);


//    if (MappedRegs == 0)
//        MappedRegs = 2;
//    else
//        MappedRegs ++;


//    Status = IoAllocateAdapterChannel(
//                    Adapter,
//                    DeviceObject,
//                    MappedRegs,
//                    CALLBACK,
//                    DeviceObject); // Context
    return TRUE;
}

