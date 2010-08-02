/*
* PROJECT:         ReactOS Kernel
* LICENSE:         GPL - See COPYING in the top level directory
* FILE:            ntoskrnl/fstub/halstub.c
* PURPOSE:         I/O Stub HAL Routines
* PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
*/

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

HAL_DISPATCH HalDispatchTable =
{
    HAL_DISPATCH_VERSION,
    (pHalQuerySystemInformation)NULL,
    (pHalSetSystemInformation)NULL,
    (pHalQueryBusSlots)NULL,
    0,
    xHalExamineMBR,
    xHalIoAssignDriveLetters,
    xHalIoReadPartitionTable,
    xHalIoSetPartitionInformation,
    xHalIoWritePartitionTable,
    (pHalHandlerForBus)NULL,
    (pHalReferenceBusHandler)NULL,
    (pHalReferenceBusHandler)NULL,
    (pHalInitPnpDriver)NULL,
    (pHalInitPowerManagement)NULL,
    (pHalGetDmaAdapter) NULL,
    (pHalGetInterruptTranslator)NULL,
    (pHalStartMirroring)NULL,
    (pHalEndMirroring)NULL,
    (pHalMirrorPhysicalMemory)NULL,
    xHalEndOfBoot,
    (pHalMirrorVerify)NULL
};

HAL_PRIVATE_DISPATCH HalPrivateDispatchTable =
{
    HAL_PRIVATE_DISPATCH_VERSION,
    (pHalHandlerForBus)NULL,
    (pHalHandlerForConfigSpace)NULL,
    (pHalLocateHiberRanges)NULL,
    (pHalRegisterBusHandler)NULL,
    xHalSetWakeEnable,
    (pHalSetWakeAlarm)NULL,
    (pHalTranslateBusAddress)NULL,
    (pHalAssignSlotResources)NULL,
    xHalHaltSystem,
    (pHalFindBusAddressTranslation)NULL,
    (pHalResetDisplay)NULL,
    (pHalAllocateMapRegisters)NULL,
    (pKdSetupPciDeviceForDebugging)NULL,
    (pKdReleasePciDeviceForDebugging)NULL,
    (pKdGetAcpiTablePhase0)NULL,
    (pKdCheckPowerButton)NULL,
    (pHalVectorToIDTEntry)xHalVectorToIDTEntry,
    (pKdMapPhysicalMemory64)NULL,
    (pKdUnmapVirtualAddress)NULL
};

/* FUNCTIONS *****************************************************************/

UCHAR
NTAPI
xHalVectorToIDTEntry(IN ULONG Vector)
{
    /* Return the vector */
    return Vector;
}

VOID
NTAPI
xHalHaltSystem(VOID)
{
    /* Halt execution */
    while (TRUE);
}

VOID
NTAPI
xHalEndOfBoot(VOID)
{
    /* Nothing */
    return;
}

VOID
NTAPI
xHalSetWakeEnable(IN BOOLEAN Enable)
{
    /* Nothing */
    return;
}
