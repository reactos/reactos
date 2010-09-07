/******************************************************************************
 *
 * Module Name: utxface - External interfaces for "global" ACPI functions
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


#define __UTXFACE_C__

#include "acpi.h"
#include "accommon.h"
#include "acevents.h"
#include "acnamesp.h"
#include "acdebug.h"
#include "actables.h"

#define _COMPONENT          ACPI_UTILITIES
        ACPI_MODULE_NAME    ("utxface")


#ifndef ACPI_ASL_COMPILER

/*******************************************************************************
 *
 * FUNCTION:    AcpiInitializeSubsystem
 *
 * PARAMETERS:  None
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Initializes all global variables.  This is the first function
 *              called, so any early initialization belongs here.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiInitializeSubsystem (
    void)
{
    ACPI_STATUS             Status;


    ACPI_FUNCTION_TRACE (AcpiInitializeSubsystem);


    AcpiGbl_StartupFlags = ACPI_SUBSYSTEM_INITIALIZE;
    ACPI_DEBUG_EXEC (AcpiUtInitStackPtrTrace ());

    /* Initialize the OS-Dependent layer */

    Status = AcpiOsInitialize ();
    if (ACPI_FAILURE (Status))
    {
        ACPI_EXCEPTION ((AE_INFO, Status, "During OSL initialization"));
        return_ACPI_STATUS (Status);
    }

    /* Initialize all globals used by the subsystem */

    Status = AcpiUtInitGlobals ();
    if (ACPI_FAILURE (Status))
    {
        ACPI_EXCEPTION ((AE_INFO, Status, "During initialization of globals"));
        return_ACPI_STATUS (Status);
    }

    /* Create the default mutex objects */

    Status = AcpiUtMutexInitialize ();
    if (ACPI_FAILURE (Status))
    {
        ACPI_EXCEPTION ((AE_INFO, Status, "During Global Mutex creation"));
        return_ACPI_STATUS (Status);
    }

    /*
     * Initialize the namespace manager and
     * the root of the namespace tree
     */
    Status = AcpiNsRootInitialize ();
    if (ACPI_FAILURE (Status))
    {
        ACPI_EXCEPTION ((AE_INFO, Status, "During Namespace initialization"));
        return_ACPI_STATUS (Status);
    }

    /* If configured, initialize the AML debugger */

    ACPI_DEBUGGER_EXEC (Status = AcpiDbInitialize ());
    return_ACPI_STATUS (Status);
}

ACPI_EXPORT_SYMBOL (AcpiInitializeSubsystem)


/*******************************************************************************
 *
 * FUNCTION:    AcpiEnableSubsystem
 *
 * PARAMETERS:  Flags           - Init/enable Options
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Completes the subsystem initialization including hardware.
 *              Puts system into ACPI mode if it isn't already.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiEnableSubsystem (
    UINT32                  Flags)
{
    ACPI_STATUS             Status = AE_OK;


    ACPI_FUNCTION_TRACE (AcpiEnableSubsystem);


    /* Enable ACPI mode */

    if (!(Flags & ACPI_NO_ACPI_ENABLE))
    {
        ACPI_DEBUG_PRINT ((ACPI_DB_EXEC, "[Init] Going into ACPI mode\n"));

        AcpiGbl_OriginalMode = AcpiHwGetMode();

        Status = AcpiEnable ();
        if (ACPI_FAILURE (Status))
        {
            ACPI_WARNING ((AE_INFO, "AcpiEnable failed"));
            return_ACPI_STATUS (Status);
        }
    }

    /*
     * Obtain a permanent mapping for the FACS. This is required for the
     * Global Lock and the Firmware Waking Vector
     */
    Status = AcpiTbInitializeFacs ();
    if (ACPI_FAILURE (Status))
    {
        ACPI_WARNING ((AE_INFO, "Could not map the FACS table"));
        return_ACPI_STATUS (Status);
    }

    /*
     * Install the default OpRegion handlers.  These are installed unless
     * other handlers have already been installed via the
     * InstallAddressSpaceHandler interface.
     */
    if (!(Flags & ACPI_NO_ADDRESS_SPACE_INIT))
    {
        ACPI_DEBUG_PRINT ((ACPI_DB_EXEC,
            "[Init] Installing default address space handlers\n"));

        Status = AcpiEvInstallRegionHandlers ();
        if (ACPI_FAILURE (Status))
        {
            return_ACPI_STATUS (Status);
        }
    }

    /*
     * Initialize ACPI Event handling (Fixed and General Purpose)
     *
     * Note1: We must have the hardware and events initialized before we can
     * execute any control methods safely. Any control method can require
     * ACPI hardware support, so the hardware must be fully initialized before
     * any method execution!
     *
     * Note2: Fixed events are initialized and enabled here. GPEs are
     * initialized, but cannot be enabled until after the hardware is
     * completely initialized (SCI and GlobalLock activated) and the various
     * initialization control methods are run (_REG, _STA, _INI) on the
     * entire namespace.
     */
    if (!(Flags & ACPI_NO_EVENT_INIT))
    {
        ACPI_DEBUG_PRINT ((ACPI_DB_EXEC,
            "[Init] Initializing ACPI events\n"));

        Status = AcpiEvInitializeEvents ();
        if (ACPI_FAILURE (Status))
        {
            return_ACPI_STATUS (Status);
        }
    }

    /*
     * Install the SCI handler and Global Lock handler. This completes the
     * hardware initialization.
     */
    if (!(Flags & ACPI_NO_HANDLER_INIT))
    {
        ACPI_DEBUG_PRINT ((ACPI_DB_EXEC,
            "[Init] Installing SCI/GL handlers\n"));

        Status = AcpiEvInstallXruptHandlers ();
        if (ACPI_FAILURE (Status))
        {
            return_ACPI_STATUS (Status);
        }
    }

    return_ACPI_STATUS (Status);
}

ACPI_EXPORT_SYMBOL (AcpiEnableSubsystem)


/*******************************************************************************
 *
 * FUNCTION:    AcpiInitializeObjects
 *
 * PARAMETERS:  Flags           - Init/enable Options
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Completes namespace initialization by initializing device
 *              objects and executing AML code for Regions, buffers, etc.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiInitializeObjects (
    UINT32                  Flags)
{
    ACPI_STATUS             Status = AE_OK;


    ACPI_FUNCTION_TRACE (AcpiInitializeObjects);


    /*
     * Run all _REG methods
     *
     * Note: Any objects accessed by the _REG methods will be automatically
     * initialized, even if they contain executable AML (see the call to
     * AcpiNsInitializeObjects below).
     */
    if (!(Flags & ACPI_NO_ADDRESS_SPACE_INIT))
    {
        ACPI_DEBUG_PRINT ((ACPI_DB_EXEC,
            "[Init] Executing _REG OpRegion methods\n"));

        Status = AcpiEvInitializeOpRegions ();
        if (ACPI_FAILURE (Status))
        {
            return_ACPI_STATUS (Status);
        }
    }

    /*
     * Execute any module-level code that was detected during the table load
     * phase. Although illegal since ACPI 2.0, there are many machines that
     * contain this type of code. Each block of detected executable AML code
     * outside of any control method is wrapped with a temporary control
     * method object and placed on a global list. The methods on this list
     * are executed below.
     */
    AcpiNsExecModuleCodeList ();

    /*
     * Initialize the objects that remain uninitialized. This runs the
     * executable AML that may be part of the declaration of these objects:
     * OperationRegions, BufferFields, Buffers, and Packages.
     */
    if (!(Flags & ACPI_NO_OBJECT_INIT))
    {
        ACPI_DEBUG_PRINT ((ACPI_DB_EXEC,
            "[Init] Completing Initialization of ACPI Objects\n"));

        Status = AcpiNsInitializeObjects ();
        if (ACPI_FAILURE (Status))
        {
            return_ACPI_STATUS (Status);
        }
    }

    /*
     * Initialize all device objects in the namespace. This runs the device
     * _STA and _INI methods.
     */
    if (!(Flags & ACPI_NO_DEVICE_INIT))
    {
        ACPI_DEBUG_PRINT ((ACPI_DB_EXEC,
            "[Init] Initializing ACPI Devices\n"));

        Status = AcpiNsInitializeDevices ();
        if (ACPI_FAILURE (Status))
        {
            return_ACPI_STATUS (Status);
        }
    }

    /*
     * Initialize the GPE blocks defined in the FADT (GPE block 0 and 1).
     * The runtime GPEs are enabled here.
     *
     * This is where the _PRW methods are executed for the GPEs. These
     * methods can only be executed after the SCI and Global Lock handlers are
     * installed and initialized.
     *
     * GPEs can only be enabled after the _REG, _STA, and _INI methods have
     * been run. This ensures that all Operation Regions and all Devices have
     * been initialized and are ready.
     */
    if (!(Flags & ACPI_NO_EVENT_INIT))
    {
        Status = AcpiEvInstallFadtGpes ();
        if (ACPI_FAILURE (Status))
        {
            return (Status);
        }
    }

    /*
     * Empty the caches (delete the cached objects) on the assumption that
     * the table load filled them up more than they will be at runtime --
     * thus wasting non-paged memory.
     */
    Status = AcpiPurgeCachedObjects ();

    AcpiGbl_StartupFlags |= ACPI_INITIALIZED_OK;
    return_ACPI_STATUS (Status);
}

ACPI_EXPORT_SYMBOL (AcpiInitializeObjects)


#endif

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

ACPI_STATUS
AcpiTerminate (
    void)
{
    ACPI_STATUS         Status;


    ACPI_FUNCTION_TRACE (AcpiTerminate);


    /* Just exit if subsystem is already shutdown */

    if (AcpiGbl_Shutdown)
    {
        ACPI_ERROR ((AE_INFO, "ACPI Subsystem is already terminated"));
        return_ACPI_STATUS (AE_OK);
    }

    /* Subsystem appears active, go ahead and shut it down */

    AcpiGbl_Shutdown = TRUE;
    AcpiGbl_StartupFlags = 0;
    ACPI_DEBUG_PRINT ((ACPI_DB_INFO, "Shutting down ACPI Subsystem\n"));

    /* Terminate the AML Debugger if present */

    ACPI_DEBUGGER_EXEC (AcpiGbl_DbTerminateThreads = TRUE);

    /* Shutdown and free all resources */

    AcpiUtSubsystemShutdown ();

    /* Free the mutex objects */

    AcpiUtMutexTerminate ();


#ifdef ACPI_DEBUGGER

    /* Shut down the debugger */

    AcpiDbTerminate ();
#endif

    /* Now we can shutdown the OS-dependent layer */

    Status = AcpiOsTerminate ();
    return_ACPI_STATUS (Status);
}

ACPI_EXPORT_SYMBOL (AcpiTerminate)


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
 *              state of the ACPI subsystem.  It will return system information
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

    ACPI_MEMCPY (Stats->FixedEventCount, AcpiFixedEventCount,
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
    return AE_OK;
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

#endif /* ACPI_ASL_COMPILER */

