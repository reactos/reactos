/*
 * PROJECT:         ReactOS PCI Bus Driver
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            drivers/bus/pci/pcivrify.c
 * PURPOSE:         PCI Driver Verifier Support
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <pci.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

BOOLEAN PciVerifierRegistered;
PVOID PciVerifierNotificationHandle;

/* FUNCTIONS ******************************************************************/

NTSTATUS
NTAPI
PciVerifierProfileChangeCallback(IN PVOID NotificationStructure,
                                 IN PVOID Context)
{
    /* This function is not yet implemented */
    UNIMPLEMENTED;
    while (TRUE);
    return STATUS_SUCCESS;
}

VOID
NTAPI
PciVerifierInit(IN PDRIVER_OBJECT DriverObject)
{
    NTSTATUS Status;

    /* Check if the kernel driver verifier is enabled */
    if (VfIsVerificationEnabled(VFOBJTYPE_SYSTEM_BIOS, NULL))
    {
        /* Register a notification for changes, to keep track of the PCI tree */
        Status = IoRegisterPlugPlayNotification(EventCategoryHardwareProfileChange,
                                                0,
                                                NULL,
                                                DriverObject,
                                                PciVerifierProfileChangeCallback,
                                                NULL,
                                                &PciVerifierNotificationHandle);
        if (NT_SUCCESS(Status)) PciVerifierRegistered = TRUE;
    }
}

/* EOF */
