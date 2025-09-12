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

PCI_VERIFIER_DATA PciVerifierFailureTable[PCI_VERIFIER_CODES] =
{
    {
        1,
        VFFAILURE_FAIL_LOGO,
        0,
        "The BIOS has reprogrammed the bus numbers of an active PCI device "
        "(!devstack %DevObj) during a dock or undock!"
    },
    {
        2,
        VFFAILURE_FAIL_LOGO,
        0,
        "A device in the system did not update it's PMCSR register in the spec "
        "mandated time (!devstack %DevObj, Power state D%Ulong)"
    },
    {
        3,
        VFFAILURE_FAIL_LOGO,
        0,
        "A driver controlling a PCI device has tried to access OS controlled "
        "configuration space registers (!devstack %DevObj, Offset 0x%Ulong1, "
        "Length 0x%Ulong2)"
    },
    {
        4,
        VFFAILURE_FAIL_UNDER_DEBUGGER,
        0,
        "A driver controlling a PCI device has tried to read or write from an "
        "invalid space using IRP_MN_READ/WRITE_CONFIG or via BUS_INTERFACE_STANDARD."
        "  NB: These functions take WhichSpace parameters of the form PCI_WHICHSPACE_*"
        " and not a BUS_DATA_TYPE (!devstack %DevObj, WhichSpace 0x%Ulong1)"
    },
};

/* FUNCTIONS ******************************************************************/

PPCI_VERIFIER_DATA
NTAPI
PciVerifierRetrieveFailureData(IN ULONG FailureCode)
{
    PPCI_VERIFIER_DATA VerifierData;

    /* Scan the verifier failure table for this code */
    VerifierData = PciVerifierFailureTable;
    while (VerifierData->FailureCode != FailureCode)
    {
        /* Keep searching */
        ++VerifierData;
        ASSERT(VerifierData < &PciVerifierFailureTable[PCI_VERIFIER_CODES]);
    }

    /* Return the entry for this code */
    return VerifierData;
}

DRIVER_NOTIFICATION_CALLBACK_ROUTINE PciVerifierProfileChangeCallback;

NTSTATUS
NTAPI
PciVerifierProfileChangeCallback(IN PVOID NotificationStructure,
                                 IN PVOID Context)
{
    UNREFERENCED_PARAMETER(NotificationStructure);
    UNREFERENCED_PARAMETER(Context);

    /* This function is not yet implemented */
    UNIMPLEMENTED_DBGBREAK();
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
