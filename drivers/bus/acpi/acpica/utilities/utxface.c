/******************************************************************************
 *
 * Module Name: utxface - External interfaces, miscellaneous utility functions
 *
 *****************************************************************************/

/*
 * Copyright (C) 2000 - 2018, Intel Corp.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer,
 *    without modification.
 * 2. Redistributions in binary form must reproduce at minimum a disclaimer
 *    substantially similar to the "NO WARRANTY" disclaimer below
 *    ("Disclaimer") and any redistribution must be conditioned upon
 *    including a substantially similar Disclaimer requirement for further
 *    binary redistribution.
 * 3. Neither the names of the above-listed copyright holders nor the names
 *    of any contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 *
 * NO WARRANTY
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDERS OR CONTRIBUTORS BE LIABLE FOR SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGES.
 */

#define EXPORT_ACPI_INTERFACES

#include "acpi.h"
#include "accommon.h"
#include "acdebug.h"

#define _COMPONENT          ACPI_UTILITIES
        ACPI_MODULE_NAME    ("utxface")


/*******************************************************************************
 *
 * FUNCTION:    AcpiTerminate
 *
 * PARAMETERS:  None
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Shutdown the ACPICA subsystem and release all resources.
 *
 ******************************************************************************/

ACPI_STATUS ACPI_INIT_FUNCTION
AcpiTerminate (
    void)
{
    ACPI_STATUS         Status;


    ACPI_FUNCTION_TRACE (AcpiTerminate);


    /* Shutdown and free all resources */

    AcpiUtSubsystemShutdown ();

    /* Free the mutex objects */

    AcpiUtMutexTerminate ();

    /* Now we can shutdown the OS-dependent layer */

    Status = AcpiOsTerminate ();
    return_ACPI_STATUS (Status);
}

ACPI_EXPORT_SYMBOL_INIT (AcpiTerminate)


#ifndef ACPI_ASL_COMPILER
/*******************************************************************************
 *
 * FUNCTION:    AcpiSubsystemStatus
 *
 * PARAMETERS:  None
 *
 * RETURN:      Status of the ACPI subsystem
 *
 * DESCRIPTION: Other drivers that use the ACPI subsystem should call this
 *              before making any other calls, to ensure the subsystem
 *              initialized successfully.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiSubsystemStatus (
    void)
{

    if (AcpiGbl_StartupFlags & ACPI_INITIALIZED_OK)
    {
        return (AE_OK);
    }
    else
    {
        return (AE_ERROR);
    }
}

ACPI_EXPORT_SYMBOL (AcpiSubsystemStatus)


/*******************************************************************************
 *
 * FUNCTION:    AcpiGetSystemInfo
 *
 * PARAMETERS:  OutBuffer       - A buffer to receive the resources for the
 *                                device
 *
 * RETURN:      Status          - the status of the call
 *
 * DESCRIPTION: This function is called to get information about the current
 *              state of the ACPI subsystem. It will return system information
 *              in the OutBuffer.
 *
 *              If the function fails an appropriate status will be returned
 *              and the value of OutBuffer is undefined.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiGetSystemInfo (
    ACPI_BUFFER             *OutBuffer)
{
    ACPI_SYSTEM_INFO        *InfoPtr;
    ACPI_STATUS             Status;


    ACPI_FUNCTION_TRACE (AcpiGetSystemInfo);


    /* Parameter validation */

    Status = AcpiUtValidateBuffer (OutBuffer);
    if (ACPI_FAILURE (Status))
    {
        return_ACPI_STATUS (Status);
    }

    /* Validate/Allocate/Clear caller buffer */

    Status = AcpiUtInitializeBuffer (OutBuffer, sizeof (ACPI_SYSTEM_INFO));
    if (ACPI_FAILURE (Status))
    {
        return_ACPI_STATUS (Status);
    }

    /*
     * Populate the return buffer
     */
    InfoPtr = (ACPI_SYSTEM_INFO *) OutBuffer->Pointer;
    InfoPtr->AcpiCaVersion = ACPI_CA_VERSION;

    /* System flags (ACPI capabilities) */

    InfoPtr->Flags = ACPI_SYS_MODE_ACPI;

    /* Timer resolution - 24 or 32 bits  */

    if (AcpiGbl_FADT.Flags & ACPI_FADT_32BIT_TIMER)
    {
        InfoPtr->TimerResolution = 24;
    }
    else
    {
        InfoPtr->TimerResolution = 32;
    }

    /* Clear the reserved fields */

    InfoPtr->Reserved1 = 0;
    InfoPtr->Reserved2 = 0;

    /* Current debug levels */

    InfoPtr->DebugLayer = AcpiDbgLayer;
    InfoPtr->DebugLevel = AcpiDbgLevel;

    return_ACPI_STATUS (AE_OK);
}

ACPI_EXPORT_SYMBOL (AcpiGetSystemInfo)


/*******************************************************************************
 *
 * FUNCTION:    AcpiGetStatistics
 *
 * PARAMETERS:  Stats           - Where the statistics are returned
 *
 * RETURN:      Status          - the status of the call
 *
 * DESCRIPTION: Get the contents of the various system counters
 *
 ******************************************************************************/

ACPI_STATUS
AcpiGetStatistics (
    ACPI_STATISTICS         *Stats)
{
    ACPI_FUNCTION_TRACE (AcpiGetStatistics);


    /* Parameter validation */

    if (!Stats)
    {
        return_ACPI_STATUS (AE_BAD_PARAMETER);
    }

    /* Various interrupt-based event counters */

    Stats->SciCount = AcpiSciCount;
    Stats->GpeCount = AcpiGpeCount;

    memcpy (Stats->FixedEventCount, AcpiFixedEventCount,
        sizeof (AcpiFixedEventCount));

    /* Other counters */

    Stats->MethodCount = AcpiMethodCount;
    return_ACPI_STATUS (AE_OK);
}

ACPI_EXPORT_SYMBOL (AcpiGetStatistics)


/*****************************************************************************
 *
 * FUNCTION:    AcpiInstallInitializationHandler
 *
 * PARAMETERS:  Handler             - Callback procedure
 *              Function            - Not (currently) used, see below
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Install an initialization handler
 *
 * TBD: When a second function is added, must save the Function also.
 *
 ****************************************************************************/

ACPI_STATUS
AcpiInstallInitializationHandler (
    ACPI_INIT_HANDLER       Handler,
    UINT32                  Function)
{

    if (!Handler)
    {
        return (AE_BAD_PARAMETER);
    }

    if (AcpiGbl_InitHandler)
    {
        return (AE_ALREADY_EXISTS);
    }

    AcpiGbl_InitHandler = Handler;
    return (AE_OK);
}

ACPI_EXPORT_SYMBOL (AcpiInstallInitializationHandler)


/*****************************************************************************
 *
 * FUNCTION:    AcpiPurgeCachedObjects
 *
 * PARAMETERS:  None
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Empty all caches (delete the cached objects)
 *
 ****************************************************************************/

ACPI_STATUS
AcpiPurgeCachedObjects (
    void)
{
    ACPI_FUNCTION_TRACE (AcpiPurgeCachedObjects);


    (void) AcpiOsPurgeCache (AcpiGbl_StateCache);
    (void) AcpiOsPurgeCache (AcpiGbl_OperandCache);
    (void) AcpiOsPurgeCache (AcpiGbl_PsNodeCache);
    (void) AcpiOsPurgeCache (AcpiGbl_PsNodeExtCache);

    return_ACPI_STATUS (AE_OK);
}

ACPI_EXPORT_SYMBOL (AcpiPurgeCachedObjects)


/*****************************************************************************
 *
 * FUNCTION:    AcpiInstallInterface
 *
 * PARAMETERS:  InterfaceName       - The interface to install
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Install an _OSI interface to the global list
 *
 ****************************************************************************/

ACPI_STATUS
AcpiInstallInterface (
    ACPI_STRING             InterfaceName)
{
    ACPI_STATUS             Status;
    ACPI_INTERFACE_INFO     *InterfaceInfo;


    /* Parameter validation */

    if (!InterfaceName || (strlen (InterfaceName) == 0))
    {
        return (AE_BAD_PARAMETER);
    }

    Status = AcpiOsAcquireMutex (AcpiGbl_OsiMutex, ACPI_WAIT_FOREVER);
    if (ACPI_FAILURE (Status))
    {
        return (Status);
    }

    /* Check if the interface name is already in the global list */

    InterfaceInfo = AcpiUtGetInterface (InterfaceName);
    if (InterfaceInfo)
    {
        /*
         * The interface already exists in the list. This is OK if the
         * interface has been marked invalid -- just clear the bit.
         */
        if (InterfaceInfo->Flags & ACPI_OSI_INVALID)
        {
            InterfaceInfo->Flags &= ~ACPI_OSI_INVALID;
            Status = AE_OK;
        }
        else
        {
            Status = AE_ALREADY_EXISTS;
        }
    }
    else
    {
        /* New interface name, install into the global list */

        Status = AcpiUtInstallInterface (InterfaceName);
    }

    AcpiOsReleaseMutex (AcpiGbl_OsiMutex);
    return (Status);
}

ACPI_EXPORT_SYMBOL (AcpiInstallInterface)


/*****************************************************************************
 *
 * FUNCTION:    AcpiRemoveInterface
 *
 * PARAMETERS:  InterfaceName       - The interface to remove
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Remove an _OSI interface from the global list
 *
 ****************************************************************************/

ACPI_STATUS
AcpiRemoveInterface (
    ACPI_STRING             InterfaceName)
{
    ACPI_STATUS             Status;


    /* Parameter validation */

    if (!InterfaceName || (strlen (InterfaceName) == 0))
    {
        return (AE_BAD_PARAMETER);
    }

    Status = AcpiOsAcquireMutex (AcpiGbl_OsiMutex, ACPI_WAIT_FOREVER);
    if (ACPI_FAILURE (Status))
    {
        return (Status);
    }

    Status = AcpiUtRemoveInterface (InterfaceName);

    AcpiOsReleaseMutex (AcpiGbl_OsiMutex);
    return (Status);
}

ACPI_EXPORT_SYMBOL (AcpiRemoveInterface)


/*****************************************************************************
 *
 * FUNCTION:    AcpiInstallInterfaceHandler
 *
 * PARAMETERS:  Handler             - The _OSI interface handler to install
 *                                    NULL means "remove existing handler"
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Install a handler for the predefined _OSI ACPI method.
 *              invoked during execution of the internal implementation of
 *              _OSI. A NULL handler simply removes any existing handler.
 *
 ****************************************************************************/

ACPI_STATUS
AcpiInstallInterfaceHandler (
    ACPI_INTERFACE_HANDLER  Handler)
{
    ACPI_STATUS             Status;


    Status = AcpiOsAcquireMutex (AcpiGbl_OsiMutex, ACPI_WAIT_FOREVER);
    if (ACPI_FAILURE (Status))
    {
        return (Status);
    }

    if (Handler && AcpiGbl_InterfaceHandler)
    {
        Status = AE_ALREADY_EXISTS;
    }
    else
    {
        AcpiGbl_InterfaceHandler = Handler;
    }

    AcpiOsReleaseMutex (AcpiGbl_OsiMutex);
    return (Status);
}

ACPI_EXPORT_SYMBOL (AcpiInstallInterfaceHandler)


/*****************************************************************************
 *
 * FUNCTION:    AcpiUpdateInterfaces
 *
 * PARAMETERS:  Action              - Actions to be performed during the
 *                                    update
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Update _OSI interface strings, disabling or enabling OS vendor
 *              string or/and feature group strings.
 *
 ****************************************************************************/

ACPI_STATUS
AcpiUpdateInterfaces (
    UINT8                   Action)
{
    ACPI_STATUS             Status;


    Status = AcpiOsAcquireMutex (AcpiGbl_OsiMutex, ACPI_WAIT_FOREVER);
    if (ACPI_FAILURE (Status))
    {
        return (Status);
    }

    Status = AcpiUtUpdateInterfaces (Action);

    AcpiOsReleaseMutex (AcpiGbl_OsiMutex);
    return (Status);
}


/*****************************************************************************
 *
 * FUNCTION:    AcpiCheckAddressRange
 *
 * PARAMETERS:  SpaceId             - Address space ID
 *              Address             - Start address
 *              Length              - Length
 *              Warn                - TRUE if warning on overlap desired
 *
 * RETURN:      Count of the number of conflicts detected.
 *
 * DESCRIPTION: Check if the input address range overlaps any of the
 *              ASL operation region address ranges.
 *
 ****************************************************************************/

UINT32
AcpiCheckAddressRange (
    ACPI_ADR_SPACE_TYPE     SpaceId,
    ACPI_PHYSICAL_ADDRESS   Address,
    ACPI_SIZE               Length,
    BOOLEAN                 Warn)
{
    UINT32                  Overlaps;
    ACPI_STATUS             Status;


    Status = AcpiUtAcquireMutex (ACPI_MTX_NAMESPACE);
    if (ACPI_FAILURE (Status))
    {
        return (0);
    }

    Overlaps = AcpiUtCheckAddressRange (SpaceId, Address,
        (UINT32) Length, Warn);

    (void) AcpiUtReleaseMutex (ACPI_MTX_NAMESPACE);
    return (Overlaps);
}

ACPI_EXPORT_SYMBOL (AcpiCheckAddressRange)

#endif /* !ACPI_ASL_COMPILER */


/*******************************************************************************
 *
 * FUNCTION:    AcpiDecodePldBuffer
 *
 * PARAMETERS:  InBuffer            - Buffer returned by _PLD method
 *              Length              - Length of the InBuffer
 *              ReturnBuffer        - Where the decode buffer is returned
 *
 * RETURN:      Status and the decoded _PLD buffer. User must deallocate
 *              the buffer via ACPI_FREE.
 *
 * DESCRIPTION: Decode the bit-packed buffer returned by the _PLD method into
 *              a local struct that is much more useful to an ACPI driver.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiDecodePldBuffer (
    UINT8                   *InBuffer,
    ACPI_SIZE               Length,
    ACPI_PLD_INFO           **ReturnBuffer)
{
    ACPI_PLD_INFO           *PldInfo;
    UINT32                  *Buffer = ACPI_CAST_PTR (UINT32, InBuffer);
    UINT32                  Dword;


    /* Parameter validation */

    if (!InBuffer || !ReturnBuffer || (Length < ACPI_PLD_REV1_BUFFER_SIZE))
    {
        return (AE_BAD_PARAMETER);
    }

    PldInfo = ACPI_ALLOCATE_ZEROED (sizeof (ACPI_PLD_INFO));
    if (!PldInfo)
    {
        return (AE_NO_MEMORY);
    }

    /* First 32-bit DWord */

    ACPI_MOVE_32_TO_32 (&Dword, &Buffer[0]);
    PldInfo->Revision =             ACPI_PLD_GET_REVISION (&Dword);
    PldInfo->IgnoreColor =          ACPI_PLD_GET_IGNORE_COLOR (&Dword);
    PldInfo->Red =                  ACPI_PLD_GET_RED (&Dword);
    PldInfo->Green =                ACPI_PLD_GET_GREEN (&Dword);
    PldInfo->Blue =                 ACPI_PLD_GET_BLUE (&Dword);

    /* Second 32-bit DWord */

    ACPI_MOVE_32_TO_32 (&Dword, &Buffer[1]);
    PldInfo->Width =                ACPI_PLD_GET_WIDTH (&Dword);
    PldInfo->Height =               ACPI_PLD_GET_HEIGHT(&Dword);

    /* Third 32-bit DWord */

    ACPI_MOVE_32_TO_32 (&Dword, &Buffer[2]);
    PldInfo->UserVisible =          ACPI_PLD_GET_USER_VISIBLE (&Dword);
    PldInfo->Dock =                 ACPI_PLD_GET_DOCK (&Dword);
    PldInfo->Lid =                  ACPI_PLD_GET_LID (&Dword);
    PldInfo->Panel =                ACPI_PLD_GET_PANEL (&Dword);
    PldInfo->VerticalPosition =     ACPI_PLD_GET_VERTICAL (&Dword);
    PldInfo->HorizontalPosition =   ACPI_PLD_GET_HORIZONTAL (&Dword);
    PldInfo->Shape =                ACPI_PLD_GET_SHAPE (&Dword);
    PldInfo->GroupOrientation =     ACPI_PLD_GET_ORIENTATION (&Dword);
    PldInfo->GroupToken =           ACPI_PLD_GET_TOKEN (&Dword);
    PldInfo->GroupPosition =        ACPI_PLD_GET_POSITION (&Dword);
    PldInfo->Bay =                  ACPI_PLD_GET_BAY (&Dword);

    /* Fourth 32-bit DWord */

    ACPI_MOVE_32_TO_32 (&Dword, &Buffer[3]);
    PldInfo->Ejectable =            ACPI_PLD_GET_EJECTABLE (&Dword);
    PldInfo->OspmEjectRequired =    ACPI_PLD_GET_OSPM_EJECT (&Dword);
    PldInfo->CabinetNumber =        ACPI_PLD_GET_CABINET (&Dword);
    PldInfo->CardCageNumber =       ACPI_PLD_GET_CARD_CAGE (&Dword);
    PldInfo->Reference =            ACPI_PLD_GET_REFERENCE (&Dword);
    PldInfo->Rotation =             ACPI_PLD_GET_ROTATION (&Dword);
    PldInfo->Order =                ACPI_PLD_GET_ORDER (&Dword);

    if (Length >= ACPI_PLD_REV2_BUFFER_SIZE)
    {
        /* Fifth 32-bit DWord (Revision 2 of _PLD) */

        ACPI_MOVE_32_TO_32 (&Dword, &Buffer[4]);
        PldInfo->VerticalOffset =       ACPI_PLD_GET_VERT_OFFSET (&Dword);
        PldInfo->HorizontalOffset =     ACPI_PLD_GET_HORIZ_OFFSET (&Dword);
    }

    *ReturnBuffer = PldInfo;
    return (AE_OK);
}

ACPI_EXPORT_SYMBOL (AcpiDecodePldBuffer)
