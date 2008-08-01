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
#include <internal/debug.h>

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
    (pHalEndOfBoot)NULL,
    (pHalMirrorVerify)NULL
};

HAL_PRIVATE_DISPATCH HalPrivateDispatchTable =
{
    HAL_PRIVATE_DISPATCH_VERSION,
    (pHalHandlerForBus)NULL,
    (pHalHandlerForConfigSpace)NULL,
    (pHalLocateHiberRanges)NULL,
    (pHalRegisterBusHandler)NULL,
    (pHalSetWakeEnable)NULL,
    (pHalSetWakeAlarm)NULL,
    (pHalTranslateBusAddress)NULL,
    (pHalAssignSlotResources)NULL,
    (pHalHaltSystem)NULL,
    (pHalFindBusAddressTranslation)NULL,
    (pHalResetDisplay)NULL,
    (pHalAllocateMapRegisters)NULL,
    (pKdSetupPciDeviceForDebugging)NULL,
    (pKdReleasePciDeviceForDebugging)NULL,
    (pKdGetAcpiTablePhase0)NULL,
    (pKdCheckPowerButton)NULL,
    (pHalVectorToIDTEntry)NULL,
    (pKdMapPhysicalMemory64)NULL,
    (pKdUnmapVirtualAddress)NULL
};

/* FUNCTIONS *****************************************************************/


/* EOF */
