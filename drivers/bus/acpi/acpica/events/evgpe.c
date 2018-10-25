/******************************************************************************
 *
 * Module Name: evgpe - General Purpose Event handling and dispatch
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
#include "acnamesp.h"

#define _COMPONENT          ACPI_EVENTS
        ACPI_MODULE_NAME    ("evgpe")

#if (!ACPI_REDUCED_HARDWARE) /* Entire module */

/* Local prototypes */

static void ACPI_SYSTEM_XFACE
AcpiEvAsynchExecuteGpeMethod (
    void                    *Context);

static void ACPI_SYSTEM_XFACE
AcpiEvAsynchEnableGpe (
    void                    *Context);


/*******************************************************************************
 *
 * FUNCTION:    AcpiEvUpdateGpeEnableMask
 *
 * PARAMETERS:  GpeEventInfo            - GPE to update
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Updates GPE register enable mask based upon whether there are
 *              runtime references to this GPE
 *
 ******************************************************************************/

ACPI_STATUS
AcpiEvUpdateGpeEnableMask (
    ACPI_GPE_EVENT_INFO     *GpeEventInfo)
{
    ACPI_GPE_REGISTER_INFO  *GpeRegisterInfo;
    UINT32                  RegisterBit;


    ACPI_FUNCTION_TRACE (EvUpdateGpeEnableMask);


    GpeRegisterInfo = GpeEventInfo->RegisterInfo;
    if (!GpeRegisterInfo)
    {
        return_ACPI_STATUS (AE_NOT_EXIST);
    }

    RegisterBit = AcpiHwGetGpeRegisterBit (GpeEventInfo);

    /* Clear the run bit up front */

    ACPI_CLEAR_BIT (GpeRegisterInfo->EnableForRun, RegisterBit);

    /* Set the mask bit only if there are references to this GPE */

    if (GpeEventInfo->RuntimeCount)
    {
        ACPI_SET_BIT (GpeRegisterInfo->EnableForRun, (UINT8) RegisterBit);
    }

    GpeRegisterInfo->EnableMask = GpeRegisterInfo->EnableForRun;
    return_ACPI_STATUS (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiEvEnableGpe
 *
 * PARAMETERS:  GpeEventInfo            - GPE to enable
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Clear a GPE of stale events and enable it.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiEvEnableGpe (
    ACPI_GPE_EVENT_INFO     *GpeEventInfo)
{
    ACPI_STATUS             Status;


    ACPI_FUNCTION_TRACE (EvEnableGpe);


    /* Clear the GPE (of stale events) */

    Status = AcpiHwClearGpe (GpeEventInfo);
    if (ACPI_FAILURE (Status))
    {
        return_ACPI_STATUS (Status);
    }

    /* Enable the requested GPE */

    Status = AcpiHwLowSetGpe (GpeEventInfo, ACPI_GPE_ENABLE);
    return_ACPI_STATUS (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiEvMaskGpe
 *
 * PARAMETERS:  GpeEventInfo            - GPE to be blocked/unblocked
 *              IsMasked                - Whether the GPE is masked or not
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Unconditionally mask/unmask a GPE during runtime.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiEvMaskGpe (
    ACPI_GPE_EVENT_INFO     *GpeEventInfo,
    BOOLEAN                 IsMasked)
{
    ACPI_GPE_REGISTER_INFO  *GpeRegisterInfo;
    UINT32                  RegisterBit;


    ACPI_FUNCTION_TRACE (EvMaskGpe);


    GpeRegisterInfo = GpeEventInfo->RegisterInfo;
    if (!GpeRegisterInfo)
    {
        return_ACPI_STATUS (AE_NOT_EXIST);
    }

    RegisterBit = AcpiHwGetGpeRegisterBit (GpeEventInfo);

    /* Perform the action */

    if (IsMasked)
    {
        if (RegisterBit & GpeRegisterInfo->MaskForRun)
        {
            return_ACPI_STATUS (AE_BAD_PARAMETER);
        }

        (void) AcpiHwLowSetGpe (GpeEventInfo, ACPI_GPE_DISABLE);
        ACPI_SET_BIT (GpeRegisterInfo->MaskForRun, (UINT8) RegisterBit);
    }
    else
    {
        if (!(RegisterBit & GpeRegisterInfo->MaskForRun))
        {
            return_ACPI_STATUS (AE_BAD_PARAMETER);
        }

        ACPI_CLEAR_BIT (GpeRegisterInfo->MaskForRun, (UINT8) RegisterBit);
        if (GpeEventInfo->RuntimeCount &&
            !GpeEventInfo->DisableForDispatch)
        {
            (void) AcpiHwLowSetGpe (GpeEventInfo, ACPI_GPE_ENABLE);
        }
    }

    return_ACPI_STATUS (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiEvAddGpeReference
 *
 * PARAMETERS:  GpeEventInfo            - Add a reference to this GPE
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Add a reference to a GPE. On the first reference, the GPE is
 *              hardware-enabled.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiEvAddGpeReference (
    ACPI_GPE_EVENT_INFO     *GpeEventInfo)
{
    ACPI_STATUS             Status = AE_OK;


    ACPI_FUNCTION_TRACE (EvAddGpeReference);


    if (GpeEventInfo->RuntimeCount == ACPI_UINT8_MAX)
    {
        return_ACPI_STATUS (AE_LIMIT);
    }

    GpeEventInfo->RuntimeCount++;
    if (GpeEventInfo->RuntimeCount == 1)
    {
        /* Enable on first reference */

        Status = AcpiEvUpdateGpeEnableMask (GpeEventInfo);
        if (ACPI_SUCCESS (Status))
        {
            Status = AcpiEvEnableGpe (GpeEventInfo);
        }

        if (ACPI_FAILURE (Status))
        {
            GpeEventInfo->RuntimeCount--;
        }
    }

    return_ACPI_STATUS (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiEvRemoveGpeReference
 *
 * PARAMETERS:  GpeEventInfo            - Remove a reference to this GPE
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Remove a reference to a GPE. When the last reference is
 *              removed, the GPE is hardware-disabled.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiEvRemoveGpeReference (
    ACPI_GPE_EVENT_INFO     *GpeEventInfo)
{
    ACPI_STATUS             Status = AE_OK;


    ACPI_FUNCTION_TRACE (EvRemoveGpeReference);


    if (!GpeEventInfo->RuntimeCount)
    {
        return_ACPI_STATUS (AE_LIMIT);
    }

    GpeEventInfo->RuntimeCount--;
    if (!GpeEventInfo->RuntimeCount)
    {
        /* Disable on last reference */

        Status = AcpiEvUpdateGpeEnableMask (GpeEventInfo);
        if (ACPI_SUCCESS (Status))
        {
            Status = AcpiHwLowSetGpe (GpeEventInfo, ACPI_GPE_DISABLE);
        }

        if (ACPI_FAILURE (Status))
        {
            GpeEventInfo->RuntimeCount++;
        }
    }

    return_ACPI_STATUS (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiEvLowGetGpeInfo
 *
 * PARAMETERS:  GpeNumber           - Raw GPE number
 *              GpeBlock            - A GPE info block
 *
 * RETURN:      A GPE EventInfo struct. NULL if not a valid GPE (The GpeNumber
 *              is not within the specified GPE block)
 *
 * DESCRIPTION: Returns the EventInfo struct associated with this GPE. This is
 *              the low-level implementation of EvGetGpeEventInfo.
 *
 ******************************************************************************/

ACPI_GPE_EVENT_INFO *
AcpiEvLowGetGpeInfo (
    UINT32                  GpeNumber,
    ACPI_GPE_BLOCK_INFO     *GpeBlock)
{
    UINT32                  GpeIndex;


    /*
     * Validate that the GpeNumber is within the specified GpeBlock.
     * (Two steps)
     */
    if (!GpeBlock ||
        (GpeNumber < GpeBlock->BlockBaseNumber))
    {
        return (NULL);
    }

    GpeIndex = GpeNumber - GpeBlock->BlockBaseNumber;
    if (GpeIndex >= GpeBlock->GpeCount)
    {
        return (NULL);
    }

    return (&GpeBlock->EventInfo[GpeIndex]);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiEvGetGpeEventInfo
 *
 * PARAMETERS:  GpeDevice           - Device node. NULL for GPE0/GPE1
 *              GpeNumber           - Raw GPE number
 *
 * RETURN:      A GPE EventInfo struct. NULL if not a valid GPE
 *
 * DESCRIPTION: Returns the EventInfo struct associated with this GPE.
 *              Validates the GpeBlock and the GpeNumber
 *
 *              Should be called only when the GPE lists are semaphore locked
 *              and not subject to change.
 *
 ******************************************************************************/

ACPI_GPE_EVENT_INFO *
AcpiEvGetGpeEventInfo (
    ACPI_HANDLE             GpeDevice,
    UINT32                  GpeNumber)
{
    ACPI_OPERAND_OBJECT     *ObjDesc;
    ACPI_GPE_EVENT_INFO     *GpeInfo;
    UINT32                  i;


    ACPI_FUNCTION_ENTRY ();


    /* A NULL GpeDevice means use the FADT-defined GPE block(s) */

    if (!GpeDevice)
    {
        /* Examine GPE Block 0 and 1 (These blocks are permanent) */

        for (i = 0; i < ACPI_MAX_GPE_BLOCKS; i++)
        {
            GpeInfo = AcpiEvLowGetGpeInfo (GpeNumber,
                AcpiGbl_GpeFadtBlocks[i]);
            if (GpeInfo)
            {
                return (GpeInfo);
            }
        }

        /* The GpeNumber was not in the range of either FADT GPE block */

        return (NULL);
    }

    /* A Non-NULL GpeDevice means this is a GPE Block Device */

    ObjDesc = AcpiNsGetAttachedObject ((ACPI_NAMESPACE_NODE *) GpeDevice);
    if (!ObjDesc ||
        !ObjDesc->Device.GpeBlock)
    {
        return (NULL);
    }

    return (AcpiEvLowGetGpeInfo (GpeNumber, ObjDesc->Device.GpeBlock));
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiEvGpeDetect
 *
 * PARAMETERS:  GpeXruptList        - Interrupt block for this interrupt.
 *                                    Can have multiple GPE blocks attached.
 *
 * RETURN:      INTERRUPT_HANDLED or INTERRUPT_NOT_HANDLED
 *
 * DESCRIPTION: Detect if any GP events have occurred. This function is
 *              executed at interrupt level.
 *
 ******************************************************************************/

UINT32
AcpiEvGpeDetect (
    ACPI_GPE_XRUPT_INFO     *GpeXruptList)
{
    ACPI_STATUS             Status;
    ACPI_GPE_BLOCK_INFO     *GpeBlock;
    ACPI_NAMESPACE_NODE     *GpeDevice;
    ACPI_GPE_REGISTER_INFO  *GpeRegisterInfo;
    ACPI_GPE_EVENT_INFO     *GpeEventInfo;
    UINT32                  GpeNumber;
    ACPI_GPE_HANDLER_INFO   *GpeHandlerInfo;
    UINT32                  IntStatus = ACPI_INTERRUPT_NOT_HANDLED;
    UINT8                   EnabledStatusByte;
    UINT64                  StatusReg;
    UINT64                  EnableReg;
    ACPI_CPU_FLAGS          Flags;
    UINT32                  i;
    UINT32                  j;


    ACPI_FUNCTION_NAME (EvGpeDetect);

    /* Check for the case where there are no GPEs */

    if (!GpeXruptList)
    {
        return (IntStatus);
    }

    /*
     * We need to obtain the GPE lock for both the data structs and registers
     * Note: Not necessary to obtain the hardware lock, since the GPE
     * registers are owned by the GpeLock.
     */
    Flags = AcpiOsAcquireLock (AcpiGbl_GpeLock);

    /* Examine all GPE blocks attached to this interrupt level */

    GpeBlock = GpeXruptList->GpeBlockListHead;
    while (GpeBlock)
    {
        GpeDevice = GpeBlock->Node;

        /*
         * Read all of the 8-bit GPE status and enable registers in this GPE
         * block, saving all of them. Find all currently active GP events.
         */
        for (i = 0; i < GpeBlock->RegisterCount; i++)
        {
            /* Get the next status/enable pair */

            GpeRegisterInfo = &GpeBlock->RegisterInfo[i];

            /*
             * Optimization: If there are no GPEs enabled within this
             * register, we can safely ignore the entire register.
             */
            if (!(GpeRegisterInfo->EnableForRun |
                  GpeRegisterInfo->EnableForWake))
            {
                ACPI_DEBUG_PRINT ((ACPI_DB_INTERRUPTS,
                    "Ignore disabled registers for GPE %02X-%02X: "
                    "RunEnable=%02X, WakeEnable=%02X\n",
                    GpeRegisterInfo->BaseGpeNumber,
                    GpeRegisterInfo->BaseGpeNumber + (ACPI_GPE_REGISTER_WIDTH - 1),
                    GpeRegisterInfo->EnableForRun,
                    GpeRegisterInfo->EnableForWake));
                continue;
            }

            /* Read the Status Register */

            Status = AcpiHwRead (&StatusReg, &GpeRegisterInfo->StatusAddress);
            if (ACPI_FAILURE (Status))
            {
                goto UnlockAndExit;
            }

            /* Read the Enable Register */

            Status = AcpiHwRead (&EnableReg, &GpeRegisterInfo->EnableAddress);
            if (ACPI_FAILURE (Status))
            {
                goto UnlockAndExit;
            }

            ACPI_DEBUG_PRINT ((ACPI_DB_INTERRUPTS,
                "Read registers for GPE %02X-%02X: Status=%02X, Enable=%02X, "
                "RunEnable=%02X, WakeEnable=%02X\n",
                GpeRegisterInfo->BaseGpeNumber,
                GpeRegisterInfo->BaseGpeNumber + (ACPI_GPE_REGISTER_WIDTH - 1),
                (UINT32) StatusReg, (UINT32) EnableReg,
                GpeRegisterInfo->EnableForRun,
                GpeRegisterInfo->EnableForWake));

            /* Check if there is anything active at all in this register */

            EnabledStatusByte = (UINT8) (StatusReg & EnableReg);
            if (!EnabledStatusByte)
            {
                /* No active GPEs in this register, move on */

                continue;
            }

            /* Now look at the individual GPEs in this byte register */

            for (j = 0; j < ACPI_GPE_REGISTER_WIDTH; j++)
            {
                /* Examine one GPE bit */

                GpeEventInfo = &GpeBlock->EventInfo[((ACPI_SIZE) i *
                    ACPI_GPE_REGISTER_WIDTH) + j];
                GpeNumber = j + GpeRegisterInfo->BaseGpeNumber;

                if (EnabledStatusByte & (1 << j))
                {
                    /* Invoke global event handler if present */

                    AcpiGpeCount++;
                    if (AcpiGbl_GlobalEventHandler)
                    {
                        AcpiGbl_GlobalEventHandler (ACPI_EVENT_TYPE_GPE,
                            GpeDevice, GpeNumber,
                            AcpiGbl_GlobalEventHandlerContext);
                    }

                    /* Found an active GPE */

                    if (ACPI_GPE_DISPATCH_TYPE (GpeEventInfo->Flags) ==
                        ACPI_GPE_DISPATCH_RAW_HANDLER)
                    {
                        /* Dispatch the event to a raw handler */

                        GpeHandlerInfo = GpeEventInfo->Dispatch.Handler;

                        /*
                         * There is no protection around the namespace node
                         * and the GPE handler to ensure a safe destruction
                         * because:
                         * 1. The namespace node is expected to always
                         *    exist after loading a table.
                         * 2. The GPE handler is expected to be flushed by
                         *    AcpiOsWaitEventsComplete() before the
                         *    destruction.
                         */
                        AcpiOsReleaseLock (AcpiGbl_GpeLock, Flags);
                        IntStatus |= GpeHandlerInfo->Address (
                            GpeDevice, GpeNumber, GpeHandlerInfo->Context);
                        Flags = AcpiOsAcquireLock (AcpiGbl_GpeLock);
                    }
                    else
                    {
                        /*
                         * Dispatch the event to a standard handler or
                         * method.
                         */
                        IntStatus |= AcpiEvGpeDispatch (GpeDevice,
                            GpeEventInfo, GpeNumber);
                    }
                }
            }
        }

        GpeBlock = GpeBlock->Next;
    }

UnlockAndExit:

    AcpiOsReleaseLock (AcpiGbl_GpeLock, Flags);
    return (IntStatus);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiEvAsynchExecuteGpeMethod
 *
 * PARAMETERS:  Context (GpeEventInfo) - Info for this GPE
 *
 * RETURN:      None
 *
 * DESCRIPTION: Perform the actual execution of a GPE control method. This
 *              function is called from an invocation of AcpiOsExecute and
 *              therefore does NOT execute at interrupt level - so that
 *              the control method itself is not executed in the context of
 *              an interrupt handler.
 *
 ******************************************************************************/

static void ACPI_SYSTEM_XFACE
AcpiEvAsynchExecuteGpeMethod (
    void                    *Context)
{
    ACPI_GPE_EVENT_INFO     *GpeEventInfo = Context;
    ACPI_STATUS             Status = AE_OK;
    ACPI_EVALUATE_INFO      *Info;
    ACPI_GPE_NOTIFY_INFO    *Notify;


    ACPI_FUNCTION_TRACE (EvAsynchExecuteGpeMethod);


    /* Do the correct dispatch - normal method or implicit notify */

    switch (ACPI_GPE_DISPATCH_TYPE (GpeEventInfo->Flags))
    {
    case ACPI_GPE_DISPATCH_NOTIFY:
        /*
         * Implicit notify.
         * Dispatch a DEVICE_WAKE notify to the appropriate handler.
         * NOTE: the request is queued for execution after this method
         * completes. The notify handlers are NOT invoked synchronously
         * from this thread -- because handlers may in turn run other
         * control methods.
         *
         * June 2012: Expand implicit notify mechanism to support
         * notifies on multiple device objects.
         */
        Notify = GpeEventInfo->Dispatch.NotifyList;
        while (ACPI_SUCCESS (Status) && Notify)
        {
            Status = AcpiEvQueueNotifyRequest (
                Notify->DeviceNode, ACPI_NOTIFY_DEVICE_WAKE);

            Notify = Notify->Next;
        }
        break;

    case ACPI_GPE_DISPATCH_METHOD:

        /* Allocate the evaluation information block */

        Info = ACPI_ALLOCATE_ZEROED (sizeof (ACPI_EVALUATE_INFO));
        if (!Info)
        {
            Status = AE_NO_MEMORY;
        }
        else
        {
            /*
             * Invoke the GPE Method (_Lxx, _Exx) i.e., evaluate the
             * _Lxx/_Exx control method that corresponds to this GPE
             */
            Info->PrefixNode = GpeEventInfo->Dispatch.MethodNode;
            Info->Flags = ACPI_IGNORE_RETURN_VALUE;

            Status = AcpiNsEvaluate (Info);
            ACPI_FREE (Info);
        }

        if (ACPI_FAILURE (Status))
        {
            ACPI_EXCEPTION ((AE_INFO, Status,
                "while evaluating GPE method [%4.4s]",
                AcpiUtGetNodeName (GpeEventInfo->Dispatch.MethodNode)));
        }
        break;

    default:

        goto ErrorExit; /* Should never happen */
    }

    /* Defer enabling of GPE until all notify handlers are done */

    Status = AcpiOsExecute (OSL_NOTIFY_HANDLER,
        AcpiEvAsynchEnableGpe, GpeEventInfo);
    if (ACPI_SUCCESS (Status))
    {
        return_VOID;
    }

ErrorExit:
    AcpiEvAsynchEnableGpe (GpeEventInfo);
    return_VOID;
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiEvAsynchEnableGpe
 *
 * PARAMETERS:  Context (GpeEventInfo) - Info for this GPE
 *              Callback from AcpiOsExecute
 *
 * RETURN:      None
 *
 * DESCRIPTION: Asynchronous clear/enable for GPE. This allows the GPE to
 *              complete (i.e., finish execution of Notify)
 *
 ******************************************************************************/

static void ACPI_SYSTEM_XFACE
AcpiEvAsynchEnableGpe (
    void                    *Context)
{
    ACPI_GPE_EVENT_INFO     *GpeEventInfo = Context;
    ACPI_CPU_FLAGS          Flags;


    Flags = AcpiOsAcquireLock (AcpiGbl_GpeLock);
    (void) AcpiEvFinishGpe (GpeEventInfo);
    AcpiOsReleaseLock (AcpiGbl_GpeLock, Flags);

    return;
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiEvFinishGpe
 *
 * PARAMETERS:  GpeEventInfo        - Info for this GPE
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Clear/Enable a GPE. Common code that is used after execution
 *              of a GPE method or a synchronous or asynchronous GPE handler.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiEvFinishGpe (
    ACPI_GPE_EVENT_INFO     *GpeEventInfo)
{
    ACPI_STATUS             Status;


    if ((GpeEventInfo->Flags & ACPI_GPE_XRUPT_TYPE_MASK) ==
            ACPI_GPE_LEVEL_TRIGGERED)
    {
        /*
         * GPE is level-triggered, we clear the GPE status bit after
         * handling the event.
         */
        Status = AcpiHwClearGpe (GpeEventInfo);
        if (ACPI_FAILURE (Status))
        {
            return (Status);
        }
    }

    /*
     * Enable this GPE, conditionally. This means that the GPE will
     * only be physically enabled if the EnableMask bit is set
     * in the EventInfo.
     */
    (void) AcpiHwLowSetGpe (GpeEventInfo, ACPI_GPE_CONDITIONAL_ENABLE);
    GpeEventInfo->DisableForDispatch = FALSE;
    return (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiEvGpeDispatch
 *
 * PARAMETERS:  GpeDevice           - Device node. NULL for GPE0/GPE1
 *              GpeEventInfo        - Info for this GPE
 *              GpeNumber           - Number relative to the parent GPE block
 *
 * RETURN:      INTERRUPT_HANDLED or INTERRUPT_NOT_HANDLED
 *
 * DESCRIPTION: Dispatch a General Purpose Event to either a function (e.g. EC)
 *              or method (e.g. _Lxx/_Exx) handler.
 *
 *              This function executes at interrupt level.
 *
 ******************************************************************************/

UINT32
AcpiEvGpeDispatch (
    ACPI_NAMESPACE_NODE     *GpeDevice,
    ACPI_GPE_EVENT_INFO     *GpeEventInfo,
    UINT32                  GpeNumber)
{
    ACPI_STATUS             Status;
    UINT32                  ReturnValue;


    ACPI_FUNCTION_TRACE (EvGpeDispatch);


    /*
     * Always disable the GPE so that it does not keep firing before
     * any asynchronous activity completes (either from the execution
     * of a GPE method or an asynchronous GPE handler.)
     *
     * If there is no handler or method to run, just disable the
     * GPE and leave it disabled permanently to prevent further such
     * pointless events from firing.
     */
    Status = AcpiHwLowSetGpe (GpeEventInfo, ACPI_GPE_DISABLE);
    if (ACPI_FAILURE (Status))
    {
        ACPI_EXCEPTION ((AE_INFO, Status,
            "Unable to disable GPE %02X", GpeNumber));
        return_UINT32 (ACPI_INTERRUPT_NOT_HANDLED);
    }

    /*
     * If edge-triggered, clear the GPE status bit now. Note that
     * level-triggered events are cleared after the GPE is serviced.
     */
    if ((GpeEventInfo->Flags & ACPI_GPE_XRUPT_TYPE_MASK) ==
            ACPI_GPE_EDGE_TRIGGERED)
    {
        Status = AcpiHwClearGpe (GpeEventInfo);
        if (ACPI_FAILURE (Status))
        {
            ACPI_EXCEPTION ((AE_INFO, Status,
                "Unable to clear GPE %02X", GpeNumber));
            (void) AcpiHwLowSetGpe (
                GpeEventInfo, ACPI_GPE_CONDITIONAL_ENABLE);
            return_UINT32 (ACPI_INTERRUPT_NOT_HANDLED);
        }
    }

    GpeEventInfo->DisableForDispatch = TRUE;

    /*
     * Dispatch the GPE to either an installed handler or the control
     * method associated with this GPE (_Lxx or _Exx). If a handler
     * exists, we invoke it and do not attempt to run the method.
     * If there is neither a handler nor a method, leave the GPE
     * disabled.
     */
    switch (ACPI_GPE_DISPATCH_TYPE (GpeEventInfo->Flags))
    {
    case ACPI_GPE_DISPATCH_HANDLER:

        /* Invoke the installed handler (at interrupt level) */

        ReturnValue = GpeEventInfo->Dispatch.Handler->Address (
            GpeDevice, GpeNumber,
            GpeEventInfo->Dispatch.Handler->Context);

        /* If requested, clear (if level-triggered) and reenable the GPE */

        if (ReturnValue & ACPI_REENABLE_GPE)
        {
            (void) AcpiEvFinishGpe (GpeEventInfo);
        }
        break;

    case ACPI_GPE_DISPATCH_METHOD:
    case ACPI_GPE_DISPATCH_NOTIFY:
        /*
         * Execute the method associated with the GPE
         * NOTE: Level-triggered GPEs are cleared after the method completes.
         */
        Status = AcpiOsExecute (OSL_GPE_HANDLER,
            AcpiEvAsynchExecuteGpeMethod, GpeEventInfo);
        if (ACPI_FAILURE (Status))
        {
            ACPI_EXCEPTION ((AE_INFO, Status,
                "Unable to queue handler for GPE %02X - event disabled",
                GpeNumber));
        }
        break;

    default:
        /*
         * No handler or method to run!
         * 03/2010: This case should no longer be possible. We will not allow
         * a GPE to be enabled if it has no handler or method.
         */
        ACPI_ERROR ((AE_INFO,
            "No handler or method for GPE %02X, disabling event",
            GpeNumber));
        break;
    }

    return_UINT32 (ACPI_INTERRUPT_HANDLED);
}

#endif /* !ACPI_REDUCED_HARDWARE */
