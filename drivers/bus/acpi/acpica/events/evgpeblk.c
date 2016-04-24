/******************************************************************************
 *
 * Module Name: evgpeblk - GPE block creation and initialization.
 *
 *****************************************************************************/

/*
 * Copyright (C) 2000 - 2016, Intel Corp.
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

#include "acpi.h"
#include "accommon.h"
#include "acevents.h"
#include "acnamesp.h"

#define _COMPONENT          ACPI_EVENTS
        ACPI_MODULE_NAME    ("evgpeblk")

#if (!ACPI_REDUCED_HARDWARE) /* Entire module */

/* Local prototypes */

static ACPI_STATUS
AcpiEvInstallGpeBlock (
    ACPI_GPE_BLOCK_INFO     *GpeBlock,
    UINT32                  InterruptNumber);

static ACPI_STATUS
AcpiEvCreateGpeInfoBlocks (
    ACPI_GPE_BLOCK_INFO     *GpeBlock);


/*******************************************************************************
 *
 * FUNCTION:    AcpiEvInstallGpeBlock
 *
 * PARAMETERS:  GpeBlock                - New GPE block
 *              InterruptNumber         - Xrupt to be associated with this
 *                                        GPE block
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Install new GPE block with mutex support
 *
 ******************************************************************************/

static ACPI_STATUS
AcpiEvInstallGpeBlock (
    ACPI_GPE_BLOCK_INFO     *GpeBlock,
    UINT32                  InterruptNumber)
{
    ACPI_GPE_BLOCK_INFO     *NextGpeBlock;
    ACPI_GPE_XRUPT_INFO     *GpeXruptBlock;
    ACPI_STATUS             Status;
    ACPI_CPU_FLAGS          Flags;


    ACPI_FUNCTION_TRACE (EvInstallGpeBlock);


    Status = AcpiUtAcquireMutex (ACPI_MTX_EVENTS);
    if (ACPI_FAILURE (Status))
    {
        return_ACPI_STATUS (Status);
    }

    Status = AcpiEvGetGpeXruptBlock (InterruptNumber, &GpeXruptBlock);
    if (ACPI_FAILURE (Status))
    {
        goto UnlockAndExit;
    }

    /* Install the new block at the end of the list with lock */

    Flags = AcpiOsAcquireLock (AcpiGbl_GpeLock);
    if (GpeXruptBlock->GpeBlockListHead)
    {
        NextGpeBlock = GpeXruptBlock->GpeBlockListHead;
        while (NextGpeBlock->Next)
        {
            NextGpeBlock = NextGpeBlock->Next;
        }

        NextGpeBlock->Next = GpeBlock;
        GpeBlock->Previous = NextGpeBlock;
    }
    else
    {
        GpeXruptBlock->GpeBlockListHead = GpeBlock;
    }

    GpeBlock->XruptBlock = GpeXruptBlock;
    AcpiOsReleaseLock (AcpiGbl_GpeLock, Flags);


UnlockAndExit:
    (void) AcpiUtReleaseMutex (ACPI_MTX_EVENTS);
    return_ACPI_STATUS (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiEvDeleteGpeBlock
 *
 * PARAMETERS:  GpeBlock            - Existing GPE block
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Remove a GPE block
 *
 ******************************************************************************/

ACPI_STATUS
AcpiEvDeleteGpeBlock (
    ACPI_GPE_BLOCK_INFO     *GpeBlock)
{
    ACPI_STATUS             Status;
    ACPI_CPU_FLAGS          Flags;


    ACPI_FUNCTION_TRACE (EvInstallGpeBlock);


    Status = AcpiUtAcquireMutex (ACPI_MTX_EVENTS);
    if (ACPI_FAILURE (Status))
    {
        return_ACPI_STATUS (Status);
    }

    /* Disable all GPEs in this block */

    Status = AcpiHwDisableGpeBlock (GpeBlock->XruptBlock, GpeBlock, NULL);

    if (!GpeBlock->Previous && !GpeBlock->Next)
    {
        /* This is the last GpeBlock on this interrupt */

        Status = AcpiEvDeleteGpeXrupt (GpeBlock->XruptBlock);
        if (ACPI_FAILURE (Status))
        {
            goto UnlockAndExit;
        }
    }
    else
    {
        /* Remove the block on this interrupt with lock */

        Flags = AcpiOsAcquireLock (AcpiGbl_GpeLock);
        if (GpeBlock->Previous)
        {
            GpeBlock->Previous->Next = GpeBlock->Next;
        }
        else
        {
            GpeBlock->XruptBlock->GpeBlockListHead = GpeBlock->Next;
        }

        if (GpeBlock->Next)
        {
            GpeBlock->Next->Previous = GpeBlock->Previous;
        }

        AcpiOsReleaseLock (AcpiGbl_GpeLock, Flags);
    }

    AcpiCurrentGpeCount -= GpeBlock->GpeCount;

    /* Free the GpeBlock */

    ACPI_FREE (GpeBlock->RegisterInfo);
    ACPI_FREE (GpeBlock->EventInfo);
    ACPI_FREE (GpeBlock);

UnlockAndExit:
    Status = AcpiUtReleaseMutex (ACPI_MTX_EVENTS);
    return_ACPI_STATUS (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiEvCreateGpeInfoBlocks
 *
 * PARAMETERS:  GpeBlock    - New GPE block
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Create the RegisterInfo and EventInfo blocks for this GPE block
 *
 ******************************************************************************/

static ACPI_STATUS
AcpiEvCreateGpeInfoBlocks (
    ACPI_GPE_BLOCK_INFO     *GpeBlock)
{
    ACPI_GPE_REGISTER_INFO  *GpeRegisterInfo = NULL;
    ACPI_GPE_EVENT_INFO     *GpeEventInfo = NULL;
    ACPI_GPE_EVENT_INFO     *ThisEvent;
    ACPI_GPE_REGISTER_INFO  *ThisRegister;
    UINT32                  i;
    UINT32                  j;
    ACPI_STATUS             Status;


    ACPI_FUNCTION_TRACE (EvCreateGpeInfoBlocks);


    /* Allocate the GPE register information block */

    GpeRegisterInfo = ACPI_ALLOCATE_ZEROED (
        (ACPI_SIZE) GpeBlock->RegisterCount *
        sizeof (ACPI_GPE_REGISTER_INFO));
    if (!GpeRegisterInfo)
    {
        ACPI_ERROR ((AE_INFO,
            "Could not allocate the GpeRegisterInfo table"));
        return_ACPI_STATUS (AE_NO_MEMORY);
    }

    /*
     * Allocate the GPE EventInfo block. There are eight distinct GPEs
     * per register. Initialization to zeros is sufficient.
     */
    GpeEventInfo = ACPI_ALLOCATE_ZEROED ((ACPI_SIZE) GpeBlock->GpeCount *
        sizeof (ACPI_GPE_EVENT_INFO));
    if (!GpeEventInfo)
    {
        ACPI_ERROR ((AE_INFO,
            "Could not allocate the GpeEventInfo table"));
        Status = AE_NO_MEMORY;
        goto ErrorExit;
    }

    /* Save the new Info arrays in the GPE block */

    GpeBlock->RegisterInfo = GpeRegisterInfo;
    GpeBlock->EventInfo = GpeEventInfo;

    /*
     * Initialize the GPE Register and Event structures. A goal of these
     * tables is to hide the fact that there are two separate GPE register
     * sets in a given GPE hardware block, the status registers occupy the
     * first half, and the enable registers occupy the second half.
     */
    ThisRegister = GpeRegisterInfo;
    ThisEvent = GpeEventInfo;

    for (i = 0; i < GpeBlock->RegisterCount; i++)
    {
        /* Init the RegisterInfo for this GPE register (8 GPEs) */

        ThisRegister->BaseGpeNumber = (UINT16)
            (GpeBlock->BlockBaseNumber + (i * ACPI_GPE_REGISTER_WIDTH));

        ThisRegister->StatusAddress.Address =
            GpeBlock->Address + i;

        ThisRegister->EnableAddress.Address =
            GpeBlock->Address + i + GpeBlock->RegisterCount;

        ThisRegister->StatusAddress.SpaceId   = GpeBlock->SpaceId;
        ThisRegister->EnableAddress.SpaceId   = GpeBlock->SpaceId;
        ThisRegister->StatusAddress.BitWidth  = ACPI_GPE_REGISTER_WIDTH;
        ThisRegister->EnableAddress.BitWidth  = ACPI_GPE_REGISTER_WIDTH;
        ThisRegister->StatusAddress.BitOffset = 0;
        ThisRegister->EnableAddress.BitOffset = 0;

        /* Init the EventInfo for each GPE within this register */

        for (j = 0; j < ACPI_GPE_REGISTER_WIDTH; j++)
        {
            ThisEvent->GpeNumber = (UINT8) (ThisRegister->BaseGpeNumber + j);
            ThisEvent->RegisterInfo = ThisRegister;
            ThisEvent++;
        }

        /* Disable all GPEs within this register */

        Status = AcpiHwWrite (0x00, &ThisRegister->EnableAddress);
        if (ACPI_FAILURE (Status))
        {
            goto ErrorExit;
        }

        /* Clear any pending GPE events within this register */

        Status = AcpiHwWrite (0xFF, &ThisRegister->StatusAddress);
        if (ACPI_FAILURE (Status))
        {
            goto ErrorExit;
        }

        ThisRegister++;
    }

    return_ACPI_STATUS (AE_OK);


ErrorExit:
    if (GpeRegisterInfo)
    {
        ACPI_FREE (GpeRegisterInfo);
    }
    if (GpeEventInfo)
    {
        ACPI_FREE (GpeEventInfo);
    }

    return_ACPI_STATUS (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiEvCreateGpeBlock
 *
 * PARAMETERS:  GpeDevice           - Handle to the parent GPE block
 *              GpeBlockAddress     - Address and SpaceID
 *              RegisterCount       - Number of GPE register pairs in the block
 *              GpeBlockBaseNumber  - Starting GPE number for the block
 *              InterruptNumber     - H/W interrupt for the block
 *              ReturnGpeBlock      - Where the new block descriptor is returned
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Create and Install a block of GPE registers. All GPEs within
 *              the block are disabled at exit.
 *              Note: Assumes namespace is locked.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiEvCreateGpeBlock (
    ACPI_NAMESPACE_NODE     *GpeDevice,
    UINT64                  Address,
    UINT8                   SpaceId,
    UINT32                  RegisterCount,
    UINT16                  GpeBlockBaseNumber,
    UINT32                  InterruptNumber,
    ACPI_GPE_BLOCK_INFO     **ReturnGpeBlock)
{
    ACPI_STATUS             Status;
    ACPI_GPE_BLOCK_INFO     *GpeBlock;
    ACPI_GPE_WALK_INFO      WalkInfo;


    ACPI_FUNCTION_TRACE (EvCreateGpeBlock);


    if (!RegisterCount)
    {
        return_ACPI_STATUS (AE_OK);
    }

    /* Allocate a new GPE block */

    GpeBlock = ACPI_ALLOCATE_ZEROED (sizeof (ACPI_GPE_BLOCK_INFO));
    if (!GpeBlock)
    {
        return_ACPI_STATUS (AE_NO_MEMORY);
    }

    /* Initialize the new GPE block */

    GpeBlock->Address = Address;
    GpeBlock->SpaceId = SpaceId;
    GpeBlock->Node = GpeDevice;
    GpeBlock->GpeCount = (UINT16) (RegisterCount * ACPI_GPE_REGISTER_WIDTH);
    GpeBlock->Initialized = FALSE;
    GpeBlock->RegisterCount = RegisterCount;
    GpeBlock->BlockBaseNumber = GpeBlockBaseNumber;

    /*
     * Create the RegisterInfo and EventInfo sub-structures
     * Note: disables and clears all GPEs in the block
     */
    Status = AcpiEvCreateGpeInfoBlocks (GpeBlock);
    if (ACPI_FAILURE (Status))
    {
        ACPI_FREE (GpeBlock);
        return_ACPI_STATUS (Status);
    }

    /* Install the new block in the global lists */

    Status = AcpiEvInstallGpeBlock (GpeBlock, InterruptNumber);
    if (ACPI_FAILURE (Status))
    {
        ACPI_FREE (GpeBlock->RegisterInfo);
        ACPI_FREE (GpeBlock->EventInfo);
        ACPI_FREE (GpeBlock);
        return_ACPI_STATUS (Status);
    }

    AcpiGbl_AllGpesInitialized = FALSE;

    /* Find all GPE methods (_Lxx or_Exx) for this block */

    WalkInfo.GpeBlock = GpeBlock;
    WalkInfo.GpeDevice = GpeDevice;
    WalkInfo.ExecuteByOwnerId = FALSE;

    Status = AcpiNsWalkNamespace (ACPI_TYPE_METHOD, GpeDevice,
        ACPI_UINT32_MAX, ACPI_NS_WALK_NO_UNLOCK,
        AcpiEvMatchGpeMethod, NULL, &WalkInfo, NULL);

    /* Return the new block */

    if (ReturnGpeBlock)
    {
        (*ReturnGpeBlock) = GpeBlock;
    }

    ACPI_DEBUG_PRINT_RAW ((ACPI_DB_INIT,
        "    Initialized GPE %02X to %02X [%4.4s] %u regs on interrupt 0x%X%s\n",
        (UINT32) GpeBlock->BlockBaseNumber,
        (UINT32) (GpeBlock->BlockBaseNumber + (GpeBlock->GpeCount - 1)),
        GpeDevice->Name.Ascii, GpeBlock->RegisterCount, InterruptNumber,
        InterruptNumber == AcpiGbl_FADT.SciInterrupt ? " (SCI)" : ""));

    /* Update global count of currently available GPEs */

    AcpiCurrentGpeCount += GpeBlock->GpeCount;
    return_ACPI_STATUS (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiEvInitializeGpeBlock
 *
 * PARAMETERS:  ACPI_GPE_CALLBACK
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Initialize and enable a GPE block. Enable GPEs that have
 *              associated methods.
 *              Note: Assumes namespace is locked.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiEvInitializeGpeBlock (
    ACPI_GPE_XRUPT_INFO     *GpeXruptInfo,
    ACPI_GPE_BLOCK_INFO     *GpeBlock,
    void                    *Ignored)
{
    ACPI_STATUS             Status;
    ACPI_GPE_EVENT_INFO     *GpeEventInfo;
    UINT32                  GpeEnabledCount;
    UINT32                  GpeIndex;
    UINT32                  i;
    UINT32                  j;


    ACPI_FUNCTION_TRACE (EvInitializeGpeBlock);


    /*
     * Ignore a null GPE block (e.g., if no GPE block 1 exists), and
     * any GPE blocks that have been initialized already.
     */
    if (!GpeBlock || GpeBlock->Initialized)
    {
        return_ACPI_STATUS (AE_OK);
    }

    /*
     * Enable all GPEs that have a corresponding method and have the
     * ACPI_GPE_CAN_WAKE flag unset. Any other GPEs within this block
     * must be enabled via the acpi_enable_gpe() interface.
     */
    GpeEnabledCount = 0;

    for (i = 0; i < GpeBlock->RegisterCount; i++)
    {
        for (j = 0; j < ACPI_GPE_REGISTER_WIDTH; j++)
        {
            /* Get the info block for this particular GPE */

            GpeIndex = (i * ACPI_GPE_REGISTER_WIDTH) + j;
            GpeEventInfo = &GpeBlock->EventInfo[GpeIndex];

            /*
             * Ignore GPEs that have no corresponding _Lxx/_Exx method
             * and GPEs that are used to wake the system
             */
            if ((ACPI_GPE_DISPATCH_TYPE (GpeEventInfo->Flags) == ACPI_GPE_DISPATCH_NONE) ||
                (ACPI_GPE_DISPATCH_TYPE (GpeEventInfo->Flags) == ACPI_GPE_DISPATCH_HANDLER) ||
                (ACPI_GPE_DISPATCH_TYPE (GpeEventInfo->Flags) == ACPI_GPE_DISPATCH_RAW_HANDLER) ||
                (GpeEventInfo->Flags & ACPI_GPE_CAN_WAKE))
            {
                continue;
            }

            Status = AcpiEvAddGpeReference (GpeEventInfo);
            if (ACPI_FAILURE (Status))
            {
                ACPI_EXCEPTION ((AE_INFO, Status,
                    "Could not enable GPE 0x%02X",
                    GpeIndex + GpeBlock->BlockBaseNumber));
                continue;
            }

            GpeEnabledCount++;
        }
    }

    if (GpeEnabledCount)
    {
        ACPI_INFO ((
            "Enabled %u GPEs in block %02X to %02X", GpeEnabledCount,
            (UINT32) GpeBlock->BlockBaseNumber,
            (UINT32) (GpeBlock->BlockBaseNumber + (GpeBlock->GpeCount - 1))));
    }

    GpeBlock->Initialized = TRUE;
    return_ACPI_STATUS (AE_OK);
}

#endif /* !ACPI_REDUCED_HARDWARE */
