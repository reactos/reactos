/*
*/

#include <sndblst.h>

BOOLEAN
CheckIrq(
    PDEVICE_OBJECT DeviceObject)
{
/*    PSOUND_BLASTER_PARAMETERS parameters = DeviceObject->DriverExtension;*/

    /* TODO */

    return TRUE;
}

BOOLEAN NTAPI
ServiceSoundBlasterInterrupt(
    IN  PKINTERRUPT Interrupt,
    IN  PVOID Context)
{
    DPRINT("* Processing ISR *\n");
    return FALSE;
}

NTSTATUS
EnableIrq(
    PDEVICE_OBJECT DeviceObject)
{
    PSOUND_BLASTER_PARAMETERS parameters = DeviceObject->DeviceExtension;
    ULONG vector;
    KIRQL irq_level;
    KAFFINITY affinity;
    NTSTATUS status = STATUS_SUCCESS;

    vector = HalGetInterruptVector(Isa,
                                   0,
                                   parameters->irq,
                                   parameters->irq,
                                   &irq_level,
                                   &affinity);

    DPRINT("Vector is 0x%x\n", vector);

    status = IoConnectInterrupt(&parameters->interrupt,
                                ServiceSoundBlasterInterrupt,
                                DeviceObject,
                                (PKSPIN_LOCK) NULL,
                                vector,
                                irq_level,
                                irq_level,
                                Latched, /* Latched / LevelSensitive */
                                FALSE,  /* shareable */
                                affinity,
                                FALSE);

    if ( status == STATUS_INVALID_PARAMETER )
        status = STATUS_DEVICE_CONFIGURATION_ERROR;

    return status;
}
