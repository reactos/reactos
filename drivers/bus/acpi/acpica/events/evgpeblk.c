/******************************************************************************
 *
 * Module Name: evgpeblk - GPE block creation and initialization.
 *
 *****************************************************************************/

/******************************************************************************
 *
 * 1. Copyright Notice
 *
 * Some or all of this work - Copyright (c) 1999 - 2009, Intel Corp.
 * All rights reserved.
 *
 * 2. License
 *
 * 2.1. This is your license from Intel Corp. under its intellectual property
 * rights.  You may have additional license terms from the party that provided
 * you this software, covering your right to use that party's intellectual
 * property rights.
 *
 * 2.2. Intel grants, free of charge, to any person ("Licensee") obtaining a
 * copy of the source code appearing in this file ("Covered Code") an
 * irrevocable, perpetual, worldwide license under Intel's copyrights in the
 * base code distributed originally by Intel ("Original Intel Code") to copy,
 * make derivatives, distribute, use and display any portion of the Covered
 * Code in any form, with the right to sublicense such rights; and
 *
 * 2.3. Intel grants Licensee a non-exclusive and non-transferable patent
 * license (with the right to sublicense), under only those claims of Intel
 * patents that are infringed by the Original Intel Code, to make, use, sell,
 * offer to sell, and import the Covered Code and derivative works thereof
 * solely to the minimum extent necessary to exercise the above copyright
 * license, and in no event shall the patent license extend to any additions
 * to or modifications of the Original Intel Code.  No other license or right
 * is granted directly or by implication, estoppel or otherwise;
 *
 * The above copyright and patent license is granted only if the following
 * conditions are met:
 *
 * 3. Conditions
 *
 * 3.1. Redistribution of Source with Rights to Further Distribute Source.
 * Redistribution of source code of any substantial portion of the Covered
 * Code or modification with rights to further distribute source must include
 * the above Copyright Notice, the above License, this list of Conditions,
 * and the following Disclaimer and Export Compliance provision.  In addition,
 * Licensee must cause all Covered Code to which Licensee contributes to
 * contain a file documenting the changes Licensee made to create that Covered
 * Code and the date of any change.  Licensee must include in that file the
 * documentation of any changes made by any predecessor Licensee.  Licensee
 * must include a prominent statement that the modification is derived,
 * directly or indirectly, from Original Intel Code.
 *
 * 3.2. Redistribution of Source with no Rights to Further Distribute Source.
 * Redistribution of source code of any substantial portion of the Covered
 * Code or modification without rights to further distribute source must
 * include the following Disclaimer and Export Compliance provision in the
 * documentation and/or other materials provided with distribution.  In
 * addition, Licensee may not authorize further sublicense of source of any
 * portion of the Covered Code, and must include terms to the effect that the
 * license from Licensee to its licensee is limited to the intellectual
 * property embodied in the software Licensee provides to its licensee, and
 * not to intellectual property embodied in modifications its licensee may
 * make.
 *
 * 3.3. Redistribution of Executable. Redistribution in executable form of any
 * substantial portion of the Covered Code or modification must reproduce the
 * above Copyright Notice, and the following Disclaimer and Export Compliance
 * provision in the documentation and/or other materials provided with the
 * distribution.
 *
 * 3.4. Intel retains all right, title, and interest in and to the Original
 * Intel Code.
 *
 * 3.5. Neither the name Intel nor any other trademark owned or controlled by
 * Intel shall be used in advertising or otherwise to promote the sale, use or
 * other dealings in products derived from or relating to the Covered Code
 * without prior written authorization from Intel.
 *
 * 4. Disclaimer and Export Compliance
 *
 * 4.1. INTEL MAKES NO WARRANTY OF ANY KIND REGARDING ANY SOFTWARE PROVIDED
 * HERE.  ANY SOFTWARE ORIGINATING FROM INTEL OR DERIVED FROM INTEL SOFTWARE
 * IS PROVIDED "AS IS," AND INTEL WILL NOT PROVIDE ANY SUPPORT,  ASSISTANCE,
 * INSTALLATION, TRAINING OR OTHER SERVICES.  INTEL WILL NOT PROVIDE ANY
 * UPDATES, ENHANCEMENTS OR EXTENSIONS.  INTEL SPECIFICALLY DISCLAIMS ANY
 * IMPLIED WARRANTIES OF MERCHANTABILITY, NONINFRINGEMENT AND FITNESS FOR A
 * PARTICULAR PURPOSE.
 *
 * 4.2. IN NO EVENT SHALL INTEL HAVE ANY LIABILITY TO LICENSEE, ITS LICENSEES
 * OR ANY OTHER THIRD PARTY, FOR ANY LOST PROFITS, LOST DATA, LOSS OF USE OR
 * COSTS OF PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES, OR FOR ANY INDIRECT,
 * SPECIAL OR CONSEQUENTIAL DAMAGES ARISING OUT OF THIS AGREEMENT, UNDER ANY
 * CAUSE OF ACTION OR THEORY OF LIABILITY, AND IRRESPECTIVE OF WHETHER INTEL
 * HAS ADVANCE NOTICE OF THE POSSIBILITY OF SUCH DAMAGES.  THESE LIMITATIONS
 * SHALL APPLY NOTWITHSTANDING THE FAILURE OF THE ESSENTIAL PURPOSE OF ANY
 * LIMITED REMEDY.
 *
 * 4.3. Licensee shall not export, either directly or indirectly, any of this
 * software or system incorporating such software without first obtaining any
 * required license or other approval from the U. S. Department of Commerce or
 * any other agency or department of the United States Government.  In the
 * event Licensee exports any such software from the United States or
 * re-exports any such software from a foreign destination, Licensee shall
 * ensure that the distribution and export/re-export of the software is in
 * compliance with all laws, regulations, orders, or other restrictions of the
 * U.S. Export Administration Regulations. Licensee agrees that neither it nor
 * any of its subsidiaries will export/re-export any technical data, process,
 * software, or service, directly or indirectly, to any country for which the
 * United States government or any agency thereof requires an export license,
 * other governmental approval, or letter of assurance, without first obtaining
 * such license, approval or letter.
 *
 *****************************************************************************/

#include "acpi.h"
#include "accommon.h"
#include "acevents.h"
#include "acnamesp.h"

#define _COMPONENT          ACPI_EVENTS
        ACPI_MODULE_NAME    ("evgpeblk")

/* Local prototypes */

static ACPI_STATUS
AcpiEvSaveMethodInfo (
    ACPI_HANDLE             ObjHandle,
    UINT32                  Level,
    void                    *ObjDesc,
    void                    **ReturnValue);

static ACPI_STATUS
AcpiEvMatchPrwAndGpe (
    ACPI_HANDLE             ObjHandle,
    UINT32                  Level,
    void                    *Info,
    void                    **ReturnValue);

static ACPI_GPE_XRUPT_INFO *
AcpiEvGetGpeXruptBlock (
    UINT32                  InterruptNumber);

static ACPI_STATUS
AcpiEvDeleteGpeXrupt (
    ACPI_GPE_XRUPT_INFO     *GpeXrupt);

static ACPI_STATUS
AcpiEvInstallGpeBlock (
    ACPI_GPE_BLOCK_INFO     *GpeBlock,
    UINT32                  InterruptNumber);

static ACPI_STATUS
AcpiEvCreateGpeInfoBlocks (
    ACPI_GPE_BLOCK_INFO     *GpeBlock);


/*******************************************************************************
 *
 * FUNCTION:    AcpiEvValidGpeEvent
 *
 * PARAMETERS:  GpeEventInfo                - Info for this GPE
 *
 * RETURN:      TRUE if the GpeEvent is valid
 *
 * DESCRIPTION: Validate a GPE event. DO NOT CALL FROM INTERRUPT LEVEL.
 *              Should be called only when the GPE lists are semaphore locked
 *              and not subject to change.
 *
 ******************************************************************************/

BOOLEAN
AcpiEvValidGpeEvent (
    ACPI_GPE_EVENT_INFO     *GpeEventInfo)
{
    ACPI_GPE_XRUPT_INFO     *GpeXruptBlock;
    ACPI_GPE_BLOCK_INFO     *GpeBlock;


    ACPI_FUNCTION_ENTRY ();


    /* No need for spin lock since we are not changing any list elements */

    /* Walk the GPE interrupt levels */

    GpeXruptBlock = AcpiGbl_GpeXruptListHead;
    while (GpeXruptBlock)
    {
        GpeBlock = GpeXruptBlock->GpeBlockListHead;

        /* Walk the GPE blocks on this interrupt level */

        while (GpeBlock)
        {
            if ((&GpeBlock->EventInfo[0] <= GpeEventInfo) &&
                (&GpeBlock->EventInfo[((ACPI_SIZE)
                    GpeBlock->RegisterCount) * 8] > GpeEventInfo))
            {
                return (TRUE);
            }

            GpeBlock = GpeBlock->Next;
        }

        GpeXruptBlock = GpeXruptBlock->Next;
    }

    return (FALSE);
}


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

            if ((GpeEventInfo->Flags & ACPI_GPE_DISPATCH_MASK) ==
                    ACPI_GPE_DISPATCH_HANDLER)
            {
                ACPI_FREE (GpeEventInfo->Dispatch.Handler);
                GpeEventInfo->Dispatch.Handler = NULL;
                GpeEventInfo->Flags &= ~ACPI_GPE_DISPATCH_MASK;
            }
        }
    }

    return_ACPI_STATUS (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiEvSaveMethodInfo
 *
 * PARAMETERS:  Callback from WalkNamespace
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Called from AcpiWalkNamespace. Expects each object to be a
 *              control method under the _GPE portion of the namespace.
 *              Extract the name and GPE type from the object, saving this
 *              information for quick lookup during GPE dispatch
 *
 *              The name of each GPE control method is of the form:
 *              "_Lxx" or "_Exx"
 *              Where:
 *                  L      - means that the GPE is level triggered
 *                  E      - means that the GPE is edge triggered
 *                  xx     - is the GPE number [in HEX]
 *
 ******************************************************************************/

static ACPI_STATUS
AcpiEvSaveMethodInfo (
    ACPI_HANDLE             ObjHandle,
    UINT32                  Level,
    void                    *ObjDesc,
    void                    **ReturnValue)
{
    ACPI_GPE_BLOCK_INFO     *GpeBlock = (void *) ObjDesc;
    ACPI_GPE_EVENT_INFO     *GpeEventInfo;
    UINT32                  GpeNumber;
    char                    Name[ACPI_NAME_SIZE + 1];
    UINT8                   Type;
    ACPI_STATUS             Status;


    ACPI_FUNCTION_TRACE (EvSaveMethodInfo);


    /*
     * _Lxx and _Exx GPE method support
     *
     * 1) Extract the name from the object and convert to a string
     */
    ACPI_MOVE_32_TO_32 (
        Name, &((ACPI_NAMESPACE_NODE *) ObjHandle)->Name.Integer);
    Name[ACPI_NAME_SIZE] = 0;

    /*
     * 2) Edge/Level determination is based on the 2nd character
     *    of the method name
     *
     * NOTE: Default GPE type is RUNTIME. May be changed later to WAKE
     * if a _PRW object is found that points to this GPE.
     */
    switch (Name[1])
    {
    case 'L':
        Type = ACPI_GPE_LEVEL_TRIGGERED;
        break;

    case 'E':
        Type = ACPI_GPE_EDGE_TRIGGERED;
        break;

    default:
        /* Unknown method type, just ignore it! */

        ACPI_DEBUG_PRINT ((ACPI_DB_LOAD,
            "Ignoring unknown GPE method type: %s "
            "(name not of form _Lxx or _Exx)",
            Name));
        return_ACPI_STATUS (AE_OK);
    }

    /* Convert the last two characters of the name to the GPE Number */

    GpeNumber = ACPI_STRTOUL (&Name[2], NULL, 16);
    if (GpeNumber == ACPI_UINT32_MAX)
    {
        /* Conversion failed; invalid method, just ignore it */

        ACPI_DEBUG_PRINT ((ACPI_DB_LOAD,
            "Could not extract GPE number from name: %s "
            "(name is not of form _Lxx or _Exx)",
            Name));
        return_ACPI_STATUS (AE_OK);
    }

    /* Ensure that we have a valid GPE number for this GPE block */

    if ((GpeNumber < GpeBlock->BlockBaseNumber) ||
        (GpeNumber >= (GpeBlock->BlockBaseNumber +
            (GpeBlock->RegisterCount * 8))))
    {
        /*
         * Not valid for this GPE block, just ignore it. However, it may be
         * valid for a different GPE block, since GPE0 and GPE1 methods both
         * appear under \_GPE.
         */
        return_ACPI_STATUS (AE_OK);
    }

    /*
     * Now we can add this information to the GpeEventInfo block for use
     * during dispatch of this GPE. Default type is RUNTIME, although this may
     * change when the _PRW methods are executed later.
     */
    GpeEventInfo = &GpeBlock->EventInfo[GpeNumber - GpeBlock->BlockBaseNumber];

    GpeEventInfo->Flags = (UINT8)
        (Type | ACPI_GPE_DISPATCH_METHOD | ACPI_GPE_TYPE_RUNTIME);

    GpeEventInfo->Dispatch.MethodNode = (ACPI_NAMESPACE_NODE *) ObjHandle;

    /* Update enable mask, but don't enable the HW GPE as of yet */

    Status = AcpiEvEnableGpe (GpeEventInfo, FALSE);

    ACPI_DEBUG_PRINT ((ACPI_DB_LOAD,
        "Registered GPE method %s as GPE number 0x%.2X\n",
        Name, GpeNumber));
    return_ACPI_STATUS (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiEvMatchPrwAndGpe
 *
 * PARAMETERS:  Callback from WalkNamespace
 *
 * RETURN:      Status. NOTE: We ignore errors so that the _PRW walk is
 *              not aborted on a single _PRW failure.
 *
 * DESCRIPTION: Called from AcpiWalkNamespace. Expects each object to be a
 *              Device. Run the _PRW method. If present, extract the GPE
 *              number and mark the GPE as a WAKE GPE.
 *
 ******************************************************************************/

static ACPI_STATUS
AcpiEvMatchPrwAndGpe (
    ACPI_HANDLE             ObjHandle,
    UINT32                  Level,
    void                    *Info,
    void                    **ReturnValue)
{
    ACPI_GPE_WALK_INFO      *GpeInfo = (void *) Info;
    ACPI_NAMESPACE_NODE     *GpeDevice;
    ACPI_GPE_BLOCK_INFO     *GpeBlock;
    ACPI_NAMESPACE_NODE     *TargetGpeDevice;
    ACPI_GPE_EVENT_INFO     *GpeEventInfo;
    ACPI_OPERAND_OBJECT     *PkgDesc;
    ACPI_OPERAND_OBJECT     *ObjDesc;
    UINT32                  GpeNumber;
    ACPI_STATUS             Status;


    ACPI_FUNCTION_TRACE (EvMatchPrwAndGpe);


    /* Check for a _PRW method under this device */

    Status = AcpiUtEvaluateObject (ObjHandle, METHOD_NAME__PRW,
                ACPI_BTYPE_PACKAGE, &PkgDesc);
    if (ACPI_FAILURE (Status))
    {
        /* Ignore all errors from _PRW, we don't want to abort the subsystem */

        return_ACPI_STATUS (AE_OK);
    }

    /* The returned _PRW package must have at least two elements */

    if (PkgDesc->Package.Count < 2)
    {
        goto Cleanup;
    }

    /* Extract pointers from the input context */

    GpeDevice = GpeInfo->GpeDevice;
    GpeBlock = GpeInfo->GpeBlock;

    /*
     * The _PRW object must return a package, we are only interested in the
     * first element
     */
    ObjDesc = PkgDesc->Package.Elements[0];

    if (ObjDesc->Common.Type == ACPI_TYPE_INTEGER)
    {
        /* Use FADT-defined GPE device (from definition of _PRW) */

        TargetGpeDevice = AcpiGbl_FadtGpeDevice;

        /* Integer is the GPE number in the FADT described GPE blocks */

        GpeNumber = (UINT32) ObjDesc->Integer.Value;
    }
    else if (ObjDesc->Common.Type == ACPI_TYPE_PACKAGE)
    {
        /* Package contains a GPE reference and GPE number within a GPE block */

        if ((ObjDesc->Package.Count < 2) ||
            ((ObjDesc->Package.Elements[0])->Common.Type !=
                ACPI_TYPE_LOCAL_REFERENCE) ||
            ((ObjDesc->Package.Elements[1])->Common.Type !=
                ACPI_TYPE_INTEGER))
        {
            goto Cleanup;
        }

        /* Get GPE block reference and decode */

        TargetGpeDevice = ObjDesc->Package.Elements[0]->Reference.Node;
        GpeNumber = (UINT32) ObjDesc->Package.Elements[1]->Integer.Value;
    }
    else
    {
        /* Unknown type, just ignore it */

        goto Cleanup;
    }

    /*
     * Is this GPE within this block?
     *
     * TRUE if and only if these conditions are true:
     *     1) The GPE devices match.
     *     2) The GPE index(number) is within the range of the Gpe Block
     *          associated with the GPE device.
     */
    if ((GpeDevice == TargetGpeDevice) &&
        (GpeNumber >= GpeBlock->BlockBaseNumber) &&
        (GpeNumber < GpeBlock->BlockBaseNumber +
            (GpeBlock->RegisterCount * 8)))
    {
        GpeEventInfo = &GpeBlock->EventInfo[GpeNumber -
            GpeBlock->BlockBaseNumber];

        /* Mark GPE for WAKE-ONLY but WAKE_DISABLED */

        GpeEventInfo->Flags &= ~(ACPI_GPE_WAKE_ENABLED | ACPI_GPE_RUN_ENABLED);

        Status = AcpiEvSetGpeType (GpeEventInfo, ACPI_GPE_TYPE_WAKE);
        if (ACPI_FAILURE (Status))
        {
            goto Cleanup;
        }

        Status = AcpiEvUpdateGpeEnableMasks (GpeEventInfo, ACPI_GPE_DISABLE);
    }

Cleanup:
    AcpiUtRemoveReference (PkgDesc);
    return_ACPI_STATUS (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiEvGetGpeXruptBlock
 *
 * PARAMETERS:  InterruptNumber      - Interrupt for a GPE block
 *
 * RETURN:      A GPE interrupt block
 *
 * DESCRIPTION: Get or Create a GPE interrupt block. There is one interrupt
 *              block per unique interrupt level used for GPEs. Should be
 *              called only when the GPE lists are semaphore locked and not
 *              subject to change.
 *
 ******************************************************************************/

static ACPI_GPE_XRUPT_INFO *
AcpiEvGetGpeXruptBlock (
    UINT32                  InterruptNumber)
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
            return_PTR (NextGpeXrupt);
        }

        NextGpeXrupt = NextGpeXrupt->Next;
    }

    /* Not found, must allocate a new xrupt descriptor */

    GpeXrupt = ACPI_ALLOCATE_ZEROED (sizeof (ACPI_GPE_XRUPT_INFO));
    if (!GpeXrupt)
    {
        return_PTR (NULL);
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
            ACPI_ERROR ((AE_INFO,
                "Could not install GPE interrupt handler at level 0x%X",
                InterruptNumber));
            return_PTR (NULL);
        }
    }

    return_PTR (GpeXrupt);
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

static ACPI_STATUS
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

    GpeXruptBlock = AcpiEvGetGpeXruptBlock (InterruptNumber);
    if (!GpeXruptBlock)
    {
        Status = AE_NO_MEMORY;
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
    Status = AcpiUtReleaseMutex (ACPI_MTX_EVENTS);
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

    AcpiCurrentGpeCount -= GpeBlock->RegisterCount * ACPI_GPE_REGISTER_WIDTH;

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
    GpeEventInfo = ACPI_ALLOCATE_ZEROED (
                        ((ACPI_SIZE) GpeBlock->RegisterCount *
                        ACPI_GPE_REGISTER_WIDTH) *
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
    GpeBlock->EventInfo    = GpeEventInfo;

    /*
     * Initialize the GPE Register and Event structures. A goal of these
     * tables is to hide the fact that there are two separate GPE register
     * sets in a given GPE hardware block, the status registers occupy the
     * first half, and the enable registers occupy the second half.
     */
    ThisRegister = GpeRegisterInfo;
    ThisEvent    = GpeEventInfo;

    for (i = 0; i < GpeBlock->RegisterCount; i++)
    {
        /* Init the RegisterInfo for this GPE register (8 GPEs) */

        ThisRegister->BaseGpeNumber = (UINT8) (GpeBlock->BlockBaseNumber +
                                             (i * ACPI_GPE_REGISTER_WIDTH));

        ThisRegister->StatusAddress.Address =
            GpeBlock->BlockAddress.Address + i;

        ThisRegister->EnableAddress.Address =
            GpeBlock->BlockAddress.Address + i + GpeBlock->RegisterCount;

        ThisRegister->StatusAddress.SpaceId   = GpeBlock->BlockAddress.SpaceId;
        ThisRegister->EnableAddress.SpaceId   = GpeBlock->BlockAddress.SpaceId;
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
    ACPI_GENERIC_ADDRESS    *GpeBlockAddress,
    UINT32                  RegisterCount,
    UINT8                   GpeBlockBaseNumber,
    UINT32                  InterruptNumber,
    ACPI_GPE_BLOCK_INFO     **ReturnGpeBlock)
{
    ACPI_STATUS             Status;
    ACPI_GPE_BLOCK_INFO     *GpeBlock;


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

    GpeBlock->Node = GpeDevice;
    GpeBlock->RegisterCount = RegisterCount;
    GpeBlock->BlockBaseNumber = GpeBlockBaseNumber;

    ACPI_MEMCPY (&GpeBlock->BlockAddress, GpeBlockAddress,
        sizeof (ACPI_GENERIC_ADDRESS));

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
        ACPI_FREE (GpeBlock);
        return_ACPI_STATUS (Status);
    }

    /* Find all GPE methods (_Lxx, _Exx) for this block */

    Status = AcpiNsWalkNamespace (ACPI_TYPE_METHOD, GpeDevice,
                ACPI_UINT32_MAX, ACPI_NS_WALK_NO_UNLOCK,
                AcpiEvSaveMethodInfo, NULL, GpeBlock, NULL);

    /* Return the new block */

    if (ReturnGpeBlock)
    {
        (*ReturnGpeBlock) = GpeBlock;
    }

    ACPI_DEBUG_PRINT ((ACPI_DB_INIT,
        "GPE %02X to %02X [%4.4s] %u regs on int 0x%X\n",
        (UINT32) GpeBlock->BlockBaseNumber,
        (UINT32) (GpeBlock->BlockBaseNumber +
                ((GpeBlock->RegisterCount * ACPI_GPE_REGISTER_WIDTH) -1)),
        GpeDevice->Name.Ascii,
        GpeBlock->RegisterCount,
        InterruptNumber));

    /* Update global count of currently available GPEs */

    AcpiCurrentGpeCount += RegisterCount * ACPI_GPE_REGISTER_WIDTH;
    return_ACPI_STATUS (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiEvInitializeGpeBlock
 *
 * PARAMETERS:  GpeDevice           - Handle to the parent GPE block
 *              GpeBlock            - Gpe Block info
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Initialize and enable a GPE block. First find and run any
 *              _PRT methods associated with the block, then enable the
 *              appropriate GPEs.
 *              Note: Assumes namespace is locked.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiEvInitializeGpeBlock (
    ACPI_NAMESPACE_NODE     *GpeDevice,
    ACPI_GPE_BLOCK_INFO     *GpeBlock)
{
    ACPI_STATUS             Status;
    ACPI_GPE_EVENT_INFO     *GpeEventInfo;
    ACPI_GPE_WALK_INFO      GpeInfo;
    UINT32                  WakeGpeCount;
    UINT32                  GpeEnabledCount;
    UINT32                  i;
    UINT32                  j;


    ACPI_FUNCTION_TRACE (EvInitializeGpeBlock);


    /* Ignore a null GPE block (e.g., if no GPE block 1 exists) */

    if (!GpeBlock)
    {
        return_ACPI_STATUS (AE_OK);
    }

    /*
     * Runtime option: Should wake GPEs be enabled at runtime?  The default
     * is no, they should only be enabled just as the machine goes to sleep.
     */
    if (AcpiGbl_LeaveWakeGpesDisabled)
    {
        /*
         * Differentiate runtime vs wake GPEs, via the _PRW control methods.
         * Each GPE that has one or more _PRWs that reference it is by
         * definition a wake GPE and will not be enabled while the machine
         * is running.
         */
        GpeInfo.GpeBlock = GpeBlock;
        GpeInfo.GpeDevice = GpeDevice;

        Status = AcpiNsWalkNamespace (ACPI_TYPE_DEVICE, ACPI_ROOT_OBJECT,
                    ACPI_UINT32_MAX, ACPI_NS_WALK_UNLOCK,
                    AcpiEvMatchPrwAndGpe, NULL, &GpeInfo, NULL);
    }

    /*
     * Enable all GPEs in this block that have these attributes:
     * 1) are "runtime" or "run/wake" GPEs, and
     * 2) have a corresponding _Lxx or _Exx method
     *
     * Any other GPEs within this block must be enabled via the
     * AcpiEnableGpe() external interface.
     */
    WakeGpeCount = 0;
    GpeEnabledCount = 0;

    for (i = 0; i < GpeBlock->RegisterCount; i++)
    {
        for (j = 0; j < 8; j++)
        {
            /* Get the info block for this particular GPE */

            GpeEventInfo = &GpeBlock->EventInfo[((ACPI_SIZE) i *
                ACPI_GPE_REGISTER_WIDTH) + j];

            if (((GpeEventInfo->Flags & ACPI_GPE_DISPATCH_MASK) ==
                    ACPI_GPE_DISPATCH_METHOD) &&
                 (GpeEventInfo->Flags & ACPI_GPE_TYPE_RUNTIME))
            {
                GpeEnabledCount++;
            }

            if (GpeEventInfo->Flags & ACPI_GPE_TYPE_WAKE)
            {
                WakeGpeCount++;
            }
        }
    }

    ACPI_DEBUG_PRINT ((ACPI_DB_INIT,
        "Found %u Wake, Enabled %u Runtime GPEs in this block\n",
        WakeGpeCount, GpeEnabledCount));

    /* Enable all valid runtime GPEs found above */

    Status = AcpiHwEnableRuntimeGpeBlock (NULL, GpeBlock, NULL);
    if (ACPI_FAILURE (Status))
    {
        ACPI_ERROR ((AE_INFO, "Could not enable GPEs in GpeBlock %p",
            GpeBlock));
    }

    return_ACPI_STATUS (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiEvGpeInitialize
 *
 * PARAMETERS:  None
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Initialize the GPE data structures
 *
 ******************************************************************************/

ACPI_STATUS
AcpiEvGpeInitialize (
    void)
{
    UINT32                  RegisterCount0 = 0;
    UINT32                  RegisterCount1 = 0;
    UINT32                  GpeNumberMax = 0;
    ACPI_STATUS             Status;


    ACPI_FUNCTION_TRACE (EvGpeInitialize);


    Status = AcpiUtAcquireMutex (ACPI_MTX_NAMESPACE);
    if (ACPI_FAILURE (Status))
    {
        return_ACPI_STATUS (Status);
    }

    /*
     * Initialize the GPE Block(s) defined in the FADT
     *
     * Why the GPE register block lengths are divided by 2:  From the ACPI
     * Spec, section "General-Purpose Event Registers", we have:
     *
     * "Each register block contains two registers of equal length
     *  GPEx_STS and GPEx_EN (where x is 0 or 1). The length of the
     *  GPE0_STS and GPE0_EN registers is equal to half the GPE0_LEN
     *  The length of the GPE1_STS and GPE1_EN registers is equal to
     *  half the GPE1_LEN. If a generic register block is not supported
     *  then its respective block pointer and block length values in the
     *  FADT table contain zeros. The GPE0_LEN and GPE1_LEN do not need
     *  to be the same size."
     */

    /*
     * Determine the maximum GPE number for this machine.
     *
     * Note: both GPE0 and GPE1 are optional, and either can exist without
     * the other.
     *
     * If EITHER the register length OR the block address are zero, then that
     * particular block is not supported.
     */
    if (AcpiGbl_FADT.Gpe0BlockLength &&
        AcpiGbl_FADT.XGpe0Block.Address)
    {
        /* GPE block 0 exists (has both length and address > 0) */

        RegisterCount0 = (UINT16) (AcpiGbl_FADT.Gpe0BlockLength / 2);

        GpeNumberMax = (RegisterCount0 * ACPI_GPE_REGISTER_WIDTH) - 1;

        /* Install GPE Block 0 */

        Status = AcpiEvCreateGpeBlock (AcpiGbl_FadtGpeDevice,
                    &AcpiGbl_FADT.XGpe0Block, RegisterCount0, 0,
                    AcpiGbl_FADT.SciInterrupt, &AcpiGbl_GpeFadtBlocks[0]);

        if (ACPI_FAILURE (Status))
        {
            ACPI_EXCEPTION ((AE_INFO, Status,
                "Could not create GPE Block 0"));
        }
    }

    if (AcpiGbl_FADT.Gpe1BlockLength &&
        AcpiGbl_FADT.XGpe1Block.Address)
    {
        /* GPE block 1 exists (has both length and address > 0) */

        RegisterCount1 = (UINT16) (AcpiGbl_FADT.Gpe1BlockLength / 2);

        /* Check for GPE0/GPE1 overlap (if both banks exist) */

        if ((RegisterCount0) &&
            (GpeNumberMax >= AcpiGbl_FADT.Gpe1Base))
        {
            ACPI_ERROR ((AE_INFO,
                "GPE0 block (GPE 0 to %d) overlaps the GPE1 block "
                "(GPE %d to %d) - Ignoring GPE1",
                GpeNumberMax, AcpiGbl_FADT.Gpe1Base,
                AcpiGbl_FADT.Gpe1Base +
                ((RegisterCount1 * ACPI_GPE_REGISTER_WIDTH) - 1)));

            /* Ignore GPE1 block by setting the register count to zero */

            RegisterCount1 = 0;
        }
        else
        {
            /* Install GPE Block 1 */

            Status = AcpiEvCreateGpeBlock (AcpiGbl_FadtGpeDevice,
                        &AcpiGbl_FADT.XGpe1Block, RegisterCount1,
                        AcpiGbl_FADT.Gpe1Base,
                        AcpiGbl_FADT.SciInterrupt, &AcpiGbl_GpeFadtBlocks[1]);

            if (ACPI_FAILURE (Status))
            {
                ACPI_EXCEPTION ((AE_INFO, Status,
                    "Could not create GPE Block 1"));
            }

            /*
             * GPE0 and GPE1 do not have to be contiguous in the GPE number
             * space. However, GPE0 always starts at GPE number zero.
             */
            GpeNumberMax = AcpiGbl_FADT.Gpe1Base +
                            ((RegisterCount1 * ACPI_GPE_REGISTER_WIDTH) - 1);
        }
    }

    /* Exit if there are no GPE registers */

    if ((RegisterCount0 + RegisterCount1) == 0)
    {
        /* GPEs are not required by ACPI, this is OK */

        ACPI_DEBUG_PRINT ((ACPI_DB_INIT,
            "There are no GPE blocks defined in the FADT\n"));
        Status = AE_OK;
        goto Cleanup;
    }

    /* Check for Max GPE number out-of-range */

    if (GpeNumberMax > ACPI_GPE_MAX)
    {
        ACPI_ERROR ((AE_INFO,
            "Maximum GPE number from FADT is too large: 0x%X",
            GpeNumberMax));
        Status = AE_BAD_VALUE;
        goto Cleanup;
    }

Cleanup:
    (void) AcpiUtReleaseMutex (ACPI_MTX_NAMESPACE);
    return_ACPI_STATUS (AE_OK);
}


