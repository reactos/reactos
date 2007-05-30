#include <ntddk.h>
#include "sndblst.h"

#if 0
BOOLEAN CheckIRQ(PDEVICE_EXTENSION Parameters)
{
    static CONST ULONG ValidIRQs[] = VALID_IRQS;
    int i;

    return TRUE;    // for now...

    // Check for Compaq!
    // if ...

    for (i = 0; ValidIRQs[i] != 0xffff; i ++)
    {
        // Consult the card

//        OUTPORT(pHw, BOARD_CONFIG, bConfig);
//        if (INPORT(pHEW, BOARD_ID) & 0x40)
//            pHW->ValidInterrupts |= (1 << ThisIRQ);

//        return (BOOLEAN)((pHw->ValidInterrupts & (1 << Interrupt)) &&
//                        (! ((INPORT(pHw, BOARD_ID) & 0x80) &&
//                        (Interrupt == 10 || Interrupt == 11)));

    }

    // else
    // Compaq stuff?
    {
        UCHAR Expected;

        switch (Parameters->IRQ)
        {
            case 10 :   Expected = 0x10;
            case 11 :   Expected = 0x20;
            case 7 :    Expected = 0x30;
            default :   return FALSE;
        }

//        CompaqPIDR = READ_PORT_UCHAR( ... )
        // ...
    }
}



BOOLEAN ISR(
    IN PKINTERRUPT pInterrupt,
    IN PVOID Context)
{
    DPRINT("*** Processing ISR ***\n");

    // What do we do here then?

    return FALSE;
}



NTSTATUS EnableIRQ(PDEVICE_OBJECT DeviceObject)
{
    PDEVICE_EXTENSION Parameters = DeviceObject->DeviceExtension;
    ULONG Vector;
    KIRQL IRQ_Level;
    KAFFINITY Affinity;
    NTSTATUS Status = STATUS_SUCCESS;

    Vector = HalGetInterruptVector(Isa,     // FIX THIS
                                   0,       // FIX THIS
                                   Parameters->IRQ,
                                   Parameters->IRQ,
                                   &IRQ_Level,
                                   &Affinity);

//    Status = IoConnectInterrupt(Parameters->Interrupt,  // Object
//                                ISR,    // Function
//                                DeviceObject,   // Context
//                                (PKSPIN_LOCK) NULL,
//                                Vector,
//                                IRQ_Level,
//                                IRQ_Level,
                                // mode - Latched or Level sensitive?
                                // share - if irq can be shared
//                                Affinity,
//                                FALSE);

    return Status == STATUS_INVALID_PARAMETER ?
                     STATUS_DEVICE_CONFIGURATION_ERROR : Status;
}
#endif

