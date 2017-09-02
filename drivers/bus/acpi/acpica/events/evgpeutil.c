/******************************************************************************
 *
 * Module Name: evgpeutil - GPE utilities
 *
 *****************************************************************************/

/*
 * Copyright (C) 2000 - 2017, Intel Corp.
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

#define _COMPONENT          ACPI_EVENTS
        ACPI_MODULE_NAME    ("evgpeutil")


#if (!ACPI_REDUCED_HARDWARE) /* Entire module */
/*******************************************************************************
 *
 * FUNCTION:    AcpiEvWalkGpeList
 *
 * PARAMETERS:  GpeWalkCallback     - Routine called for each GPE block
 *              Context             - Value passed to callback
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Walk the GPE lists.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiEvWalkGpeList (
    ACPI_GPE_CALLBACK       GpeWalkCallback,
    void                    *Context)
{
    ACPI_GPE_BLOCK_INFO     *GpeBlock;
    ACPI_GPE_XRUPT_INFO     *GpeXruptInfo;
    ACPI_STATUS             Status = AE_OK;
    ACPI_CPU_FLAGS          Flags;


    ACPI_FUNCTION_TRACE (EvWalkGpeList);


    Flags = AcpiOsAcquireLock (AcpiGbl_GpeLock);

    /* Walk the interrupt level descriptor list */

    GpeXruptInfo = AcpiGbl_GpeXruptListHead;
    while (GpeXruptInfo)
    {
        /* Walk all Gpe Blocks attached to this interrupt level */

        GpeBlock = GpeXruptInfo->GpeBlockListHead;
        while (GpeBlock)
        {
            /* One callback per GPE block */

            Status = GpeWalkCallback (GpeXruptInfo, GpeBlock, Context);
            if (ACPI_FAILURE (Status))
            {
                if (Status == AE_CTRL_END) /* Callback abort */
                {
                    Status = AE_OK;
                }
                goto UnlockAndExit;
            }

            GpeBlock = GpeBlock->Next;
        }

        GpeXruptInfo = GpeXruptInfo->Next;
    }

UnlockAndExit:
    AcpiOsReleaseLock (AcpiGbl_GpeLock, Flags);
    return_ACPI_STATUS (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiEvGetGpeDevice
 *
 * PARAMETERS:  GPE_WALK_CALLBACK
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Matches the input GPE index (0-CurrentGpeCount) with a GPE
 *              block device. NULL if the GPE is one of the FADT-defined GPEs.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiEvGetGpeDevice (
    ACPI_GPE_XRUPT_INFO     *GpeXruptInfo,
    ACPI_GPE_BLOCK_INFO     *GpeBlock,
    void                    *Context)
{
    ACPI_GPE_DEVICE_INFO    *Info = Context;


    /* Increment Index by the number of GPEs in this block */

    Info->NextBlockBaseIndex += GpeBlock->GpeCount;

    if (Info->Index < Info->NextBlockBaseIndex)
    {
        /*
         * The GPE index is within this block, get the node. Leave the node
         * NULL for the FADT-defined GPEs
         */
        if ((GpeBlock->Node)->Type == ACPI_TYPE_DEVICE)
        {
            Info->GpeDevice = GpeBlock->Node;
        }

        Info->Status = AE_OK;
        return (AE_CTRL_END);
    }

    return (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiEvGetGpeXruptBlock
 *
 * PARAMETERS:  InterruptNumber             - Interrupt for a GPE block
 *              GpeXruptBlock               - Where the block is returned
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Get or Create a GPE interrupt block. There is one interrupt
 *              block per unique interrupt level used for GPEs. Should be
 *              called only when the GPE lists are semaphore locked and not
 *              subject to change.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiEvGetGpeXruptBlock (
    UINT32                  InterruptNumber,
    ACPI_GPE_XRUPT_INFO     **GpeXruptBlock)
{
    ACPI_GPE_XRUPT_INFO     *NextGpeXrupt;
    ACPI_GPE_XRUPT_INFO     *GpeXrupt;
    ACPI_STATUS             Status;
    ACPI_CPU_FLAGS          Flags;


    ACPI_FUNCTION_TRACE (EvGetGpeXruptBlock);


    /* No need for lock since we are not changing any list elements here */

    NextGpeXrupt = AcpiGbl_GpeXruptListHead;
    while (NextGpeXrupt)
    {
        if (NextGpeXrupt->InterruptNumber == InterruptNumber)
        {
            *GpeXruptBlock = NextGpeXrupt;
            return_ACPI_STATUS (AE_OK);
        }

        NextGpeXrupt = NextGpeXrupt->Next;
    }

    /* Not found, must allocate a new xrupt descriptor */

    GpeXrupt = ACPI_ALLOCATE_ZEROED (sizeof (ACPI_GPE_XRUPT_INFO));
    if (!GpeXrupt)
    {
        return_ACPI_STATUS (AE_NO_MEMORY);
    }

    GpeXrupt->InterruptNumber = InterruptNumber;

    /* Install new interrupt descriptor with spin lock */

    Flags = AcpiOsAcquireLock (AcpiGbl_GpeLock);
    if (AcpiGbl_GpeXruptListHead)
    {
        NextGpeXrupt = AcpiGbl_GpeXruptListHead;
        while (NextGpeXrupt->Next)
        {
            NextGpeXrupt = NextGpeXrupt->Next;
        }

        NextGpeXrupt->Next = GpeXrupt;
        GpeXrupt->Previous = NextGpeXrupt;
    }
    else
    {
        AcpiGbl_GpeXruptListHead = GpeXrupt;
    }

    AcpiOsReleaseLock (AcpiGbl_GpeLock, Flags);

    /* Install new interrupt handler if not SCI_INT */

    if (InterruptNumber != AcpiGbl_FADT.SciInterrupt)
    {
        Status = AcpiOsInstallInterruptHandler (InterruptNumber,
            AcpiEvGpeXruptHandler, GpeXrupt);
        if (ACPI_FAILURE (Status))
        {
            ACPI_EXCEPTION ((AE_INFO, Status,
                "Could not install GPE interrupt handler at level 0x%X",
                InterruptNumber));
            return_ACPI_STATUS (Status);
        }
    }

    *GpeXruptBlock = GpeXrupt;
    return_ACPI_STATUS (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiEvDeleteGpeXrupt
 *
 * PARAMETERS:  GpeXrupt        - A GPE interrupt info block
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Remove and free a GpeXrupt block. Remove an associated
 *              interrupt handler if not the SCI interrupt.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiEvDeleteGpeXrupt (
    ACPI_GPE_XRUPT_INFO     *GpeXrupt)
{
    ACPI_STATUS             Status;
    ACPI_CPU_FLAGS          Flags;


    ACPI_FUNCTION_TRACE (EvDeleteGpeXrupt);


    /* We never want to remove the SCI interrupt handler */

    if (GpeXrupt->InterruptNumber == AcpiGbl_FADT.SciInterrupt)
    {
        GpeXrupt->GpeBlockListHead = NULL;
        return_ACPI_STATUS (AE_OK);
    }

    /* Disable this interrupt */

    Status = AcpiOsRemoveInterruptHandler (
        GpeXrupt->InterruptNumber, AcpiEvGpeXruptHandler);
    if (ACPI_FAILURE (Status))
    {
        return_ACPI_STATUS (Status);
    }

    /* Unlink the interrupt block with lock */

    Flags = AcpiOsAcquireLock (AcpiGbl_GpeLock);
    if (GpeXrupt->Previous)
    {
        GpeXrupt->Previous->Next = GpeXrupt->Next;
    }
    else
    {
        /* No previous, update list head */

        AcpiGbl_GpeXruptListHead = GpeXrupt->Next;
    }

    if (GpeXrupt->Next)
    {
        GpeXrupt->Next->Previous = GpeXrupt->Previous;
    }
    AcpiOsReleaseLock (AcpiGbl_GpeLock, Flags);

    /* Free the block */

    ACPI_FREE (GpeXrupt);
    return_ACPI_STATUS (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiEvDeleteGpeHandlers
 *
 * PARAMETERS:  GpeXruptInfo        - GPE Interrupt info
 *              GpeBlock            - Gpe Block info
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Delete all Handler objects found in the GPE data structs.
 *              Used only prior to termination.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiEvDeleteGpeHandlers (
    ACPI_GPE_XRUPT_INFO     *GpeXruptInfo,
    ACPI_GPE_BLOCK_INFO     *GpeBlock,
    void                    *Context)
{
    ACPI_GPE_EVENT_INFO     *GpeEventInfo;
    ACPI_GPE_NOTIFY_INFO    *Notify;
    ACPI_GPE_NOTIFY_INFO    *Next;
    UINT32                  i;
    UINT32                  j;


    ACPI_FUNCTION_TRACE (EvDeleteGpeHandlers);


    /* Examine each GPE Register within the block */

    for (i = 0; i < GpeBlock->RegisterCount; i++)
    {
        /* Now look at the individual GPEs in this byte register */

        for (j = 0; j < ACPI_GPE_REGISTER_WIDTH; j++)
        {
            GpeEventInfo = &GpeBlock->EventInfo[((ACPI_SIZE) i *
                ACPI_GPE_REGISTER_WIDTH) + j];

            if ((ACPI_GPE_DISPATCH_TYPE (GpeEventInfo->Flags) ==
                    ACPI_GPE_DISPATCH_HANDLER) ||
                (ACPI_GPE_DISPATCH_TYPE (GpeEventInfo->Flags) ==
                    ACPI_GPE_DISPATCH_RAW_HANDLER))
            {
                /* Delete an installed handler block */

                ACPI_FREE (GpeEventInfo->Dispatch.Handler);
                GpeEventInfo->Dispatch.Handler = NULL;
                GpeEventInfo->Flags &= ~ACPI_GPE_DISPATCH_MASK;
            }
            else if (ACPI_GPE_DISPATCH_TYPE (GpeEventInfo->Flags) ==
                ACPI_GPE_DISPATCH_NOTIFY)
            {
                /* Delete the implicit notification device list */

                Notify = GpeEventInfo->Dispatch.NotifyList;
                while (Notify)
                {
                    Next = Notify->Next;
                    ACPI_FREE (Notify);
                    Notify = Next;
                }

                GpeEventInfo->Dispatch.NotifyList = NULL;
                GpeEventInfo->Flags &= ~ACPI_GPE_DISPATCH_MASK;
            }
        }
    }

    return_ACPI_STATUS (AE_OK);
}

#endif /* !ACPI_REDUCED_HARDWARE */
