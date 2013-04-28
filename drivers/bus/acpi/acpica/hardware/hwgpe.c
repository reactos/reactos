
/******************************************************************************
 *
 * Module Name: hwgpe - Low level GPE enable/disable/clear functions
 *
 *****************************************************************************/

/******************************************************************************
 *
 * 1. Copyright Notice
 *
 * Some or all of this work - Copyright (c) 1999 - 2011, Intel Corp.
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

#define _COMPONENT          ACPI_HARDWARE
        ACPI_MODULE_NAME    ("hwgpe")

/* Local prototypes */

static ACPI_STATUS
AcpiHwEnableWakeupGpeBlock (
    ACPI_GPE_XRUPT_INFO     *GpeXruptInfo,
    ACPI_GPE_BLOCK_INFO     *GpeBlock,
    void                    *Context);


/******************************************************************************
 *
 * FUNCTION:    AcpiHwGetGpeRegisterBit
 *
 * PARAMETERS:  GpeEventInfo        - Info block for the GPE
 *              GpeRegisterInfo     - Info block for the GPE register
 *
 * RETURN:      Register mask with a one in the GPE bit position
 *
 * DESCRIPTION: Compute the register mask for this GPE. One bit is set in the
 *              correct position for the input GPE.
 *
 ******************************************************************************/

UINT32
AcpiHwGetGpeRegisterBit (
    ACPI_GPE_EVENT_INFO     *GpeEventInfo,
    ACPI_GPE_REGISTER_INFO  *GpeRegisterInfo)
{

    return ((UINT32) 1 <<
        (GpeEventInfo->GpeNumber - GpeRegisterInfo->BaseGpeNumber));
}


/******************************************************************************
 *
 * FUNCTION:    AcpiHwLowSetGpe
 *
 * PARAMETERS:  GpeEventInfo        - Info block for the GPE to be disabled
 *              Action              - Enable or disable
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Enable or disable a single GPE in the parent enable register.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiHwLowSetGpe (
    ACPI_GPE_EVENT_INFO     *GpeEventInfo,
    UINT32                  Action)
{
    ACPI_GPE_REGISTER_INFO  *GpeRegisterInfo;
    ACPI_STATUS             Status;
    UINT32                  EnableMask;
    UINT32                  RegisterBit;


    ACPI_FUNCTION_ENTRY ();


    /* Get the info block for the entire GPE register */

    GpeRegisterInfo = GpeEventInfo->RegisterInfo;
    if (!GpeRegisterInfo)
    {
        return (AE_NOT_EXIST);
    }

    /* Get current value of the enable register that contains this GPE */

    Status = AcpiHwRead (&EnableMask, &GpeRegisterInfo->EnableAddress);
    if (ACPI_FAILURE (Status))
    {
        return (Status);
    }

    /* Set or clear just the bit that corresponds to this GPE */

    RegisterBit = AcpiHwGetGpeRegisterBit (GpeEventInfo, GpeRegisterInfo);
    switch (Action)
    {
    case ACPI_GPE_CONDITIONAL_ENABLE:

        /* Only enable if the EnableForRun bit is set */

        if (!(RegisterBit & GpeRegisterInfo->EnableForRun))
        {
            return (AE_BAD_PARAMETER);
        }

        /*lint -fallthrough */

    case ACPI_GPE_ENABLE:
        ACPI_SET_BIT (EnableMask, RegisterBit);
        break;

    case ACPI_GPE_DISABLE:
        ACPI_CLEAR_BIT (EnableMask, RegisterBit);
        break;

    default:
        ACPI_ERROR ((AE_INFO, "Invalid GPE Action, %u\n", Action));
        return (AE_BAD_PARAMETER);
    }

    /* Write the updated enable mask */

    Status = AcpiHwWrite (EnableMask, &GpeRegisterInfo->EnableAddress);
    return (Status);
}


/******************************************************************************
 *
 * FUNCTION:    AcpiHwClearGpe
 *
 * PARAMETERS:  GpeEventInfo        - Info block for the GPE to be cleared
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Clear the status bit for a single GPE.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiHwClearGpe (
    ACPI_GPE_EVENT_INFO     *GpeEventInfo)
{
    ACPI_GPE_REGISTER_INFO  *GpeRegisterInfo;
    ACPI_STATUS             Status;
    UINT32                  RegisterBit;


    ACPI_FUNCTION_ENTRY ();

    /* Get the info block for the entire GPE register */

    GpeRegisterInfo = GpeEventInfo->RegisterInfo;
    if (!GpeRegisterInfo)
    {
        return (AE_NOT_EXIST);
    }

    /*
     * Write a one to the appropriate bit in the status register to
     * clear this GPE.
     */
    RegisterBit = AcpiHwGetGpeRegisterBit (GpeEventInfo, GpeRegisterInfo);

    Status = AcpiHwWrite (RegisterBit,
                    &GpeRegisterInfo->StatusAddress);

    return (Status);
}


/******************************************************************************
 *
 * FUNCTION:    AcpiHwGetGpeStatus
 *
 * PARAMETERS:  GpeEventInfo        - Info block for the GPE to queried
 *              EventStatus         - Where the GPE status is returned
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Return the status of a single GPE.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiHwGetGpeStatus (
    ACPI_GPE_EVENT_INFO     *GpeEventInfo,
    ACPI_EVENT_STATUS       *EventStatus)
{
    UINT32                  InByte;
    UINT32                  RegisterBit;
    ACPI_GPE_REGISTER_INFO  *GpeRegisterInfo;
    ACPI_EVENT_STATUS       LocalEventStatus = 0;
    ACPI_STATUS             Status;


    ACPI_FUNCTION_ENTRY ();


    if (!EventStatus)
    {
        return (AE_BAD_PARAMETER);
    }

    /* Get the info block for the entire GPE register */

    GpeRegisterInfo = GpeEventInfo->RegisterInfo;

    /* Get the register bitmask for this GPE */

    RegisterBit = AcpiHwGetGpeRegisterBit (GpeEventInfo, GpeRegisterInfo);

    /* GPE currently enabled? (enabled for runtime?) */

    if (RegisterBit & GpeRegisterInfo->EnableForRun)
    {
        LocalEventStatus |= ACPI_EVENT_FLAG_ENABLED;
    }

    /* GPE enabled for wake? */

    if (RegisterBit & GpeRegisterInfo->EnableForWake)
    {
        LocalEventStatus |= ACPI_EVENT_FLAG_WAKE_ENABLED;
    }

    /* GPE currently active (status bit == 1)? */

    Status = AcpiHwRead (&InByte, &GpeRegisterInfo->StatusAddress);
    if (ACPI_FAILURE (Status))
    {
        return (Status);
    }

    if (RegisterBit & InByte)
    {
        LocalEventStatus |= ACPI_EVENT_FLAG_SET;
    }

    /* Set return value */

    (*EventStatus) = LocalEventStatus;
    return (AE_OK);
}


/******************************************************************************
 *
 * FUNCTION:    AcpiHwDisableGpeBlock
 *
 * PARAMETERS:  GpeXruptInfo        - GPE Interrupt info
 *              GpeBlock            - Gpe Block info
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Disable all GPEs within a single GPE block
 *
 ******************************************************************************/

ACPI_STATUS
AcpiHwDisableGpeBlock (
    ACPI_GPE_XRUPT_INFO     *GpeXruptInfo,
    ACPI_GPE_BLOCK_INFO     *GpeBlock,
    void                    *Context)
{
    UINT32                  i;
    ACPI_STATUS             Status;


    /* Examine each GPE Register within the block */

    for (i = 0; i < GpeBlock->RegisterCount; i++)
    {
        /* Disable all GPEs in this register */

        Status = AcpiHwWrite (0x00, &GpeBlock->RegisterInfo[i].EnableAddress);
        if (ACPI_FAILURE (Status))
        {
            return (Status);
        }
    }

    return (AE_OK);
}


/******************************************************************************
 *
 * FUNCTION:    AcpiHwClearGpeBlock
 *
 * PARAMETERS:  GpeXruptInfo        - GPE Interrupt info
 *              GpeBlock            - Gpe Block info
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Clear status bits for all GPEs within a single GPE block
 *
 ******************************************************************************/

ACPI_STATUS
AcpiHwClearGpeBlock (
    ACPI_GPE_XRUPT_INFO     *GpeXruptInfo,
    ACPI_GPE_BLOCK_INFO     *GpeBlock,
    void                    *Context)
{
    UINT32                  i;
    ACPI_STATUS             Status;


    /* Examine each GPE Register within the block */

    for (i = 0; i < GpeBlock->RegisterCount; i++)
    {
        /* Clear status on all GPEs in this register */

        Status = AcpiHwWrite (0xFF, &GpeBlock->RegisterInfo[i].StatusAddress);
        if (ACPI_FAILURE (Status))
        {
            return (Status);
        }
    }

    return (AE_OK);
}


/******************************************************************************
 *
 * FUNCTION:    AcpiHwEnableRuntimeGpeBlock
 *
 * PARAMETERS:  GpeXruptInfo        - GPE Interrupt info
 *              GpeBlock            - Gpe Block info
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Enable all "runtime" GPEs within a single GPE block. Includes
 *              combination wake/run GPEs.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiHwEnableRuntimeGpeBlock (
    ACPI_GPE_XRUPT_INFO     *GpeXruptInfo,
    ACPI_GPE_BLOCK_INFO     *GpeBlock,
    void                    *Context)
{
    UINT32                  i;
    ACPI_STATUS             Status;


    /* NOTE: assumes that all GPEs are currently disabled */

    /* Examine each GPE Register within the block */

    for (i = 0; i < GpeBlock->RegisterCount; i++)
    {
        if (!GpeBlock->RegisterInfo[i].EnableForRun)
        {
            continue;
        }

        /* Enable all "runtime" GPEs in this register */

        Status = AcpiHwWrite (GpeBlock->RegisterInfo[i].EnableForRun,
                    &GpeBlock->RegisterInfo[i].EnableAddress);
        if (ACPI_FAILURE (Status))
        {
            return (Status);
        }
    }

    return (AE_OK);
}


/******************************************************************************
 *
 * FUNCTION:    AcpiHwEnableWakeupGpeBlock
 *
 * PARAMETERS:  GpeXruptInfo        - GPE Interrupt info
 *              GpeBlock            - Gpe Block info
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Enable all "wake" GPEs within a single GPE block. Includes
 *              combination wake/run GPEs.
 *
 ******************************************************************************/

static ACPI_STATUS
AcpiHwEnableWakeupGpeBlock (
    ACPI_GPE_XRUPT_INFO     *GpeXruptInfo,
    ACPI_GPE_BLOCK_INFO     *GpeBlock,
    void                    *Context)
{
    UINT32                  i;
    ACPI_STATUS             Status;


    /* Examine each GPE Register within the block */

    for (i = 0; i < GpeBlock->RegisterCount; i++)
    {
        if (!GpeBlock->RegisterInfo[i].EnableForWake)
        {
            continue;
        }

        /* Enable all "wake" GPEs in this register */

        Status = AcpiHwWrite (GpeBlock->RegisterInfo[i].EnableForWake,
                    &GpeBlock->RegisterInfo[i].EnableAddress);
        if (ACPI_FAILURE (Status))
        {
            return (Status);
        }
    }

    return (AE_OK);
}


/******************************************************************************
 *
 * FUNCTION:    AcpiHwDisableAllGpes
 *
 * PARAMETERS:  None
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Disable and clear all GPEs in all GPE blocks
 *
 ******************************************************************************/

ACPI_STATUS
AcpiHwDisableAllGpes (
    void)
{
    ACPI_STATUS             Status;


    ACPI_FUNCTION_TRACE (HwDisableAllGpes);


    Status = AcpiEvWalkGpeList (AcpiHwDisableGpeBlock, NULL);
    Status = AcpiEvWalkGpeList (AcpiHwClearGpeBlock, NULL);
    return_ACPI_STATUS (Status);
}


/******************************************************************************
 *
 * FUNCTION:    AcpiHwEnableAllRuntimeGpes
 *
 * PARAMETERS:  None
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Enable all "runtime" GPEs, in all GPE blocks
 *
 ******************************************************************************/

ACPI_STATUS
AcpiHwEnableAllRuntimeGpes (
    void)
{
    ACPI_STATUS             Status;


    ACPI_FUNCTION_TRACE (HwEnableAllRuntimeGpes);


    Status = AcpiEvWalkGpeList (AcpiHwEnableRuntimeGpeBlock, NULL);
    return_ACPI_STATUS (Status);
}


/******************************************************************************
 *
 * FUNCTION:    AcpiHwEnableAllWakeupGpes
 *
 * PARAMETERS:  None
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Enable all "wakeup" GPEs, in all GPE blocks
 *
 ******************************************************************************/

ACPI_STATUS
AcpiHwEnableAllWakeupGpes (
    void)
{
    ACPI_STATUS             Status;


    ACPI_FUNCTION_TRACE (HwEnableAllWakeupGpes);


    Status = AcpiEvWalkGpeList (AcpiHwEnableWakeupGpeBlock, NULL);
    return_ACPI_STATUS (Status);
}

