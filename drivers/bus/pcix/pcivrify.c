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

/**
 * @brief Reports a bridge whose bus numbers differ from the ones this driver tracks.
 */
static
VOID
NTAPI
PciVerifierCheckBridgeBusNumbers(
    _In_ PPCI_PDO_EXTENSION PdoExtension)
{
    PPCI_VERIFIER_DATA VerifierData;
    UCHAR BusNumbers[3];

    /* Type 1 and type 2 headers keep the bus numbers at the same offset */
    PciReadDeviceConfig(PdoExtension,
                        BusNumbers,
                        FIELD_OFFSET(PCI_COMMON_HEADER, u.type1.PrimaryBus),
                        sizeof(BusNumbers));

    if ((BusNumbers[0] == PdoExtension->Dependent.type1.PrimaryBus) &&
        (BusNumbers[1] == PdoExtension->Dependent.type1.SecondaryBus) &&
        (BusNumbers[2] == PdoExtension->Dependent.type1.SubordinateBus))
    {
        return;
    }

    VerifierData = PciVerifierRetrieveFailureData(1);
    ASSERT(VerifierData);
    VfFailSystemBIOS(PCI_VERIFIER_DETECTED_VIOLATION,
                     1,
                     VerifierData->FailureClass,
                     &VerifierData->AssertionControl,
                     VerifierData->DebuggerMessageText,
                     "%DevObj",
                     PdoExtension->PhysicalDeviceObject);
}

/**
 * @brief Checks every active bridge after firmware had the chance to reprogram it.
 */
static
VOID
NTAPI
PciVerifierCheckAllBridges(VOID)
{
    PPCI_FDO_EXTENSION FdoExtension;
    PPCI_PDO_EXTENSION PdoExtension;
    PAGED_CODE();

    KeEnterCriticalRegion();
    KeWaitForSingleObject(&PciGlobalLock, Executive, KernelMode, FALSE, NULL);

    for (FdoExtension = (PPCI_FDO_EXTENSION)PciFdoExtensionListHead.Next;
         FdoExtension;
         FdoExtension = (PPCI_FDO_EXTENSION)FdoExtension->List.Next)
    {
        KeWaitForSingleObject(&FdoExtension->ChildListLock, Executive, KernelMode, FALSE, NULL);

        for (PdoExtension = FdoExtension->ChildPdoList;
             PdoExtension;
             PdoExtension = PdoExtension->Next)
        {
            if ((PdoExtension->HeaderType != PCI_BRIDGE_TYPE) &&
                (PdoExtension->HeaderType != PCI_CARDBUS_BRIDGE_TYPE))
            {
                continue;
            }

            /* A bridge that is gone or powered off may have lost its settings legitimately */
            if (PdoExtension->NotPresent ||
                (PdoExtension->PowerState.CurrentDeviceState == PowerDeviceD3))
            {
                continue;
            }

            PciVerifierCheckBridgeBusNumbers(PdoExtension);
        }

        KeSetEvent(&FdoExtension->ChildListLock, IO_NO_INCREMENT, FALSE);
    }

    KeSetEvent(&PciGlobalLock, IO_NO_INCREMENT, FALSE);
    KeLeaveCriticalRegion();
}

DRIVER_NOTIFICATION_CALLBACK_ROUTINE PciVerifierProfileChangeCallback;

NTSTATUS
NTAPI
PciVerifierProfileChangeCallback(IN PVOID NotificationStructure,
                                 IN PVOID Context)
{
    PHWPROFILE_CHANGE_NOTIFICATION Notification = NotificationStructure;

    UNREFERENCED_PARAMETER(Context);
    PAGED_CODE();

    /* A dock or undock is finished, so firmware is done touching the buses */
    if (IsEqualGUIDAligned(&Notification->Event, &GUID_HWPROFILE_CHANGE_COMPLETE))
        PciVerifierCheckAllBridges();

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

/**
 * @brief Drops the hardware profile notification taken by PciVerifierInit.
 */
VOID
NTAPI
PciVerifierRelease(VOID)
{
    PAGED_CODE();

    if (PciVerifierRegistered)
    {
        IoUnregisterPlugPlayNotification(PciVerifierNotificationHandle);
        PciVerifierNotificationHandle = NULL;
        PciVerifierRegistered = FALSE;
    }
}

/* EOF */
