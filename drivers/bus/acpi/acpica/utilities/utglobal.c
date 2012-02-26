/******************************************************************************
 *
 * Module Name: utglobal - Global variables for the ACPI subsystem
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

#define __UTGLOBAL_C__
#define DEFINE_ACPI_GLOBALS

#include "acpi.h"
#include "accommon.h"

#define _COMPONENT          ACPI_UTILITIES
        ACPI_MODULE_NAME    ("utglobal")


/*******************************************************************************
 *
 * Static global variable initialization.
 *
 ******************************************************************************/

/*
 * We want the debug switches statically initialized so they
 * are already set when the debugger is entered.
 */

/* Debug switch - level and trace mask */

#ifdef ACPI_DEBUG_OUTPUT
UINT32                      AcpiDbgLevel = ACPI_DEBUG_DEFAULT;
#else
UINT32                      AcpiDbgLevel = ACPI_NORMAL_DEFAULT;
#endif

/* Debug switch - layer (component) mask */

UINT32                      AcpiDbgLayer = ACPI_COMPONENT_DEFAULT;
UINT32                      AcpiGbl_NestingLevel = 0;

/* Debugger globals */

BOOLEAN                     AcpiGbl_DbTerminateThreads = FALSE;
BOOLEAN                     AcpiGbl_AbortMethod = FALSE;
BOOLEAN                     AcpiGbl_MethodExecuting = FALSE;

/* System flags */

UINT32                      AcpiGbl_StartupFlags = 0;

/* System starts uninitialized */

BOOLEAN                     AcpiGbl_Shutdown = TRUE;

const char                  *AcpiGbl_SleepStateNames[ACPI_S_STATE_COUNT] =
{
    "\\_S0_",
    "\\_S1_",
    "\\_S2_",
    "\\_S3_",
    "\\_S4_",
    "\\_S5_"
};

const char                  *AcpiGbl_LowestDstateNames[ACPI_NUM_SxW_METHODS] =
{
    "_S0W",
    "_S1W",
    "_S2W",
    "_S3W",
    "_S4W"
};

const char                  *AcpiGbl_HighestDstateNames[ACPI_NUM_SxD_METHODS] =
{
    "_S1D",
    "_S2D",
    "_S3D",
    "_S4D"
};


/*******************************************************************************
 *
 * Namespace globals
 *
 ******************************************************************************/

/*
 * Predefined ACPI Names (Built-in to the Interpreter)
 *
 * NOTES:
 * 1) _SB_ is defined to be a device to allow \_SB_._INI to be run
 *    during the initialization sequence.
 * 2) _TZ_ is defined to be a thermal zone in order to allow ASL code to
 *    perform a Notify() operation on it. 09/2010: Changed to type Device.
 *    This still allows notifies, but does not confuse host code that
 *    searches for valid ThermalZone objects.
 */
const ACPI_PREDEFINED_NAMES     AcpiGbl_PreDefinedNames[] =
{
    {"_GPE",    ACPI_TYPE_LOCAL_SCOPE,      NULL},
    {"_PR_",    ACPI_TYPE_LOCAL_SCOPE,      NULL},
    {"_SB_",    ACPI_TYPE_DEVICE,           NULL},
    {"_SI_",    ACPI_TYPE_LOCAL_SCOPE,      NULL},
    {"_TZ_",    ACPI_TYPE_DEVICE,           NULL},
    {"_REV",    ACPI_TYPE_INTEGER,          (char *) ACPI_CA_SUPPORT_LEVEL},
    {"_OS_",    ACPI_TYPE_STRING,           ACPI_OS_NAME},
    {"_GL_",    ACPI_TYPE_MUTEX,            (char *) 1},

#if !defined (ACPI_NO_METHOD_EXECUTION) || defined (ACPI_CONSTANT_EVAL_ONLY)
    {"_OSI",    ACPI_TYPE_METHOD,           (char *) 1},
#endif

    /* Table terminator */

    {NULL,      ACPI_TYPE_ANY,              NULL}
};


/******************************************************************************
 *
 * Event and Hardware globals
 *
 ******************************************************************************/

ACPI_BIT_REGISTER_INFO      AcpiGbl_BitRegisterInfo[ACPI_NUM_BITREG] =
{
    /* Name                                     Parent Register             Register Bit Position                   Register Bit Mask       */

    /* ACPI_BITREG_TIMER_STATUS         */   {ACPI_REGISTER_PM1_STATUS,   ACPI_BITPOSITION_TIMER_STATUS,          ACPI_BITMASK_TIMER_STATUS},
    /* ACPI_BITREG_BUS_MASTER_STATUS    */   {ACPI_REGISTER_PM1_STATUS,   ACPI_BITPOSITION_BUS_MASTER_STATUS,     ACPI_BITMASK_BUS_MASTER_STATUS},
    /* ACPI_BITREG_GLOBAL_LOCK_STATUS   */   {ACPI_REGISTER_PM1_STATUS,   ACPI_BITPOSITION_GLOBAL_LOCK_STATUS,    ACPI_BITMASK_GLOBAL_LOCK_STATUS},
    /* ACPI_BITREG_POWER_BUTTON_STATUS  */   {ACPI_REGISTER_PM1_STATUS,   ACPI_BITPOSITION_POWER_BUTTON_STATUS,   ACPI_BITMASK_POWER_BUTTON_STATUS},
    /* ACPI_BITREG_SLEEP_BUTTON_STATUS  */   {ACPI_REGISTER_PM1_STATUS,   ACPI_BITPOSITION_SLEEP_BUTTON_STATUS,   ACPI_BITMASK_SLEEP_BUTTON_STATUS},
    /* ACPI_BITREG_RT_CLOCK_STATUS      */   {ACPI_REGISTER_PM1_STATUS,   ACPI_BITPOSITION_RT_CLOCK_STATUS,       ACPI_BITMASK_RT_CLOCK_STATUS},
    /* ACPI_BITREG_WAKE_STATUS          */   {ACPI_REGISTER_PM1_STATUS,   ACPI_BITPOSITION_WAKE_STATUS,           ACPI_BITMASK_WAKE_STATUS},
    /* ACPI_BITREG_PCIEXP_WAKE_STATUS   */   {ACPI_REGISTER_PM1_STATUS,   ACPI_BITPOSITION_PCIEXP_WAKE_STATUS,    ACPI_BITMASK_PCIEXP_WAKE_STATUS},

    /* ACPI_BITREG_TIMER_ENABLE         */   {ACPI_REGISTER_PM1_ENABLE,   ACPI_BITPOSITION_TIMER_ENABLE,          ACPI_BITMASK_TIMER_ENABLE},
    /* ACPI_BITREG_GLOBAL_LOCK_ENABLE   */   {ACPI_REGISTER_PM1_ENABLE,   ACPI_BITPOSITION_GLOBAL_LOCK_ENABLE,    ACPI_BITMASK_GLOBAL_LOCK_ENABLE},
    /* ACPI_BITREG_POWER_BUTTON_ENABLE  */   {ACPI_REGISTER_PM1_ENABLE,   ACPI_BITPOSITION_POWER_BUTTON_ENABLE,   ACPI_BITMASK_POWER_BUTTON_ENABLE},
    /* ACPI_BITREG_SLEEP_BUTTON_ENABLE  */   {ACPI_REGISTER_PM1_ENABLE,   ACPI_BITPOSITION_SLEEP_BUTTON_ENABLE,   ACPI_BITMASK_SLEEP_BUTTON_ENABLE},
    /* ACPI_BITREG_RT_CLOCK_ENABLE      */   {ACPI_REGISTER_PM1_ENABLE,   ACPI_BITPOSITION_RT_CLOCK_ENABLE,       ACPI_BITMASK_RT_CLOCK_ENABLE},
    /* ACPI_BITREG_PCIEXP_WAKE_DISABLE  */   {ACPI_REGISTER_PM1_ENABLE,   ACPI_BITPOSITION_PCIEXP_WAKE_DISABLE,   ACPI_BITMASK_PCIEXP_WAKE_DISABLE},

    /* ACPI_BITREG_SCI_ENABLE           */   {ACPI_REGISTER_PM1_CONTROL,  ACPI_BITPOSITION_SCI_ENABLE,            ACPI_BITMASK_SCI_ENABLE},
    /* ACPI_BITREG_BUS_MASTER_RLD       */   {ACPI_REGISTER_PM1_CONTROL,  ACPI_BITPOSITION_BUS_MASTER_RLD,        ACPI_BITMASK_BUS_MASTER_RLD},
    /* ACPI_BITREG_GLOBAL_LOCK_RELEASE  */   {ACPI_REGISTER_PM1_CONTROL,  ACPI_BITPOSITION_GLOBAL_LOCK_RELEASE,   ACPI_BITMASK_GLOBAL_LOCK_RELEASE},
    /* ACPI_BITREG_SLEEP_TYPE           */   {ACPI_REGISTER_PM1_CONTROL,  ACPI_BITPOSITION_SLEEP_TYPE,            ACPI_BITMASK_SLEEP_TYPE},
    /* ACPI_BITREG_SLEEP_ENABLE         */   {ACPI_REGISTER_PM1_CONTROL,  ACPI_BITPOSITION_SLEEP_ENABLE,          ACPI_BITMASK_SLEEP_ENABLE},

    /* ACPI_BITREG_ARB_DIS              */   {ACPI_REGISTER_PM2_CONTROL,  ACPI_BITPOSITION_ARB_DISABLE,           ACPI_BITMASK_ARB_DISABLE}
};


ACPI_FIXED_EVENT_INFO       AcpiGbl_FixedEventInfo[ACPI_NUM_FIXED_EVENTS] =
{
    /* ACPI_EVENT_PMTIMER       */  {ACPI_BITREG_TIMER_STATUS,          ACPI_BITREG_TIMER_ENABLE,        ACPI_BITMASK_TIMER_STATUS,          ACPI_BITMASK_TIMER_ENABLE},
    /* ACPI_EVENT_GLOBAL        */  {ACPI_BITREG_GLOBAL_LOCK_STATUS,    ACPI_BITREG_GLOBAL_LOCK_ENABLE,  ACPI_BITMASK_GLOBAL_LOCK_STATUS,    ACPI_BITMASK_GLOBAL_LOCK_ENABLE},
    /* ACPI_EVENT_POWER_BUTTON  */  {ACPI_BITREG_POWER_BUTTON_STATUS,   ACPI_BITREG_POWER_BUTTON_ENABLE, ACPI_BITMASK_POWER_BUTTON_STATUS,   ACPI_BITMASK_POWER_BUTTON_ENABLE},
    /* ACPI_EVENT_SLEEP_BUTTON  */  {ACPI_BITREG_SLEEP_BUTTON_STATUS,   ACPI_BITREG_SLEEP_BUTTON_ENABLE, ACPI_BITMASK_SLEEP_BUTTON_STATUS,   ACPI_BITMASK_SLEEP_BUTTON_ENABLE},
    /* ACPI_EVENT_RTC           */  {ACPI_BITREG_RT_CLOCK_STATUS,       ACPI_BITREG_RT_CLOCK_ENABLE,     ACPI_BITMASK_RT_CLOCK_STATUS,       ACPI_BITMASK_RT_CLOCK_ENABLE},
};


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtInitGlobals
 *
 * PARAMETERS:  None
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Init ACPICA globals.  All globals that require specific
 *              initialization should be initialized here!
 *
 ******************************************************************************/

ACPI_STATUS
AcpiUtInitGlobals (
    void)
{
    ACPI_STATUS             Status;
    UINT32                  i;


    ACPI_FUNCTION_TRACE (UtInitGlobals);


    /* Create all memory caches */

    Status = AcpiUtCreateCaches ();
    if (ACPI_FAILURE (Status))
    {
        return_ACPI_STATUS (Status);
    }

    /* Mutex locked flags */

    for (i = 0; i < ACPI_NUM_MUTEX; i++)
    {
        AcpiGbl_MutexInfo[i].Mutex          = NULL;
        AcpiGbl_MutexInfo[i].ThreadId       = ACPI_MUTEX_NOT_ACQUIRED;
        AcpiGbl_MutexInfo[i].UseCount       = 0;
    }

    for (i = 0; i < ACPI_NUM_OWNERID_MASKS; i++)
    {
        AcpiGbl_OwnerIdMask[i]              = 0;
    }

    /* Last OwnerID is never valid */

    AcpiGbl_OwnerIdMask[ACPI_NUM_OWNERID_MASKS - 1] = 0x80000000;

    /* Event counters */

    AcpiMethodCount                     = 0;
    AcpiSciCount                        = 0;
    AcpiGpeCount                        = 0;

    for (i = 0; i < ACPI_NUM_FIXED_EVENTS; i++)
    {
        AcpiFixedEventCount[i]              = 0;
    }

    /* GPE support */

    AcpiGbl_AllGpesInitialized          = FALSE;
    AcpiGbl_GpeXruptListHead            = NULL;
    AcpiGbl_GpeFadtBlocks[0]            = NULL;
    AcpiGbl_GpeFadtBlocks[1]            = NULL;
    AcpiCurrentGpeCount                 = 0;

    /* Global handlers */

    AcpiGbl_SystemNotify.Handler        = NULL;
    AcpiGbl_DeviceNotify.Handler        = NULL;
    AcpiGbl_ExceptionHandler            = NULL;
    AcpiGbl_InitHandler                 = NULL;
    AcpiGbl_TableHandler                = NULL;
    AcpiGbl_InterfaceHandler            = NULL;
    AcpiGbl_GlobalEventHandler          = NULL;

    /* Global Lock support */

    AcpiGbl_GlobalLockSemaphore         = NULL;
    AcpiGbl_GlobalLockMutex             = NULL;
    AcpiGbl_GlobalLockAcquired          = FALSE;
    AcpiGbl_GlobalLockHandle            = 0;
    AcpiGbl_GlobalLockPresent           = FALSE;

    /* Miscellaneous variables */

    AcpiGbl_DSDT                        = NULL;
    AcpiGbl_CmSingleStep                = FALSE;
    AcpiGbl_DbTerminateThreads          = FALSE;
    AcpiGbl_Shutdown                    = FALSE;
    AcpiGbl_NsLookupCount               = 0;
    AcpiGbl_PsFindCount                 = 0;
    AcpiGbl_AcpiHardwarePresent         = TRUE;
    AcpiGbl_LastOwnerIdIndex            = 0;
    AcpiGbl_NextOwnerIdOffset           = 0;
    AcpiGbl_TraceMethodName             = 0;
    AcpiGbl_TraceDbgLevel               = 0;
    AcpiGbl_TraceDbgLayer               = 0;
    AcpiGbl_DebuggerConfiguration       = DEBUGGER_THREADING;
    AcpiGbl_DbOutputFlags               = ACPI_DB_CONSOLE_OUTPUT;
    AcpiGbl_OsiData                     = 0;
    AcpiGbl_OsiMutex                    = NULL;
    AcpiGbl_RegMethodsExecuted          = FALSE;

    /* Hardware oriented */

    AcpiGbl_EventsInitialized           = FALSE;
    AcpiGbl_SystemAwakeAndRunning       = TRUE;

    /* Namespace */

    AcpiGbl_ModuleCodeList              = NULL;
    AcpiGbl_RootNode                    = NULL;
    AcpiGbl_RootNodeStruct.Name.Integer = ACPI_ROOT_NAME;
    AcpiGbl_RootNodeStruct.DescriptorType = ACPI_DESC_TYPE_NAMED;
    AcpiGbl_RootNodeStruct.Type         = ACPI_TYPE_DEVICE;
    AcpiGbl_RootNodeStruct.Parent       = NULL;
    AcpiGbl_RootNodeStruct.Child        = NULL;
    AcpiGbl_RootNodeStruct.Peer         = NULL;
    AcpiGbl_RootNodeStruct.Object       = NULL;


#ifdef ACPI_DISASSEMBLER
    AcpiGbl_ExternalList                = NULL;
#endif

#ifdef ACPI_DEBUG_OUTPUT
    AcpiGbl_LowestStackPointer          = ACPI_CAST_PTR (ACPI_SIZE, ACPI_SIZE_MAX);
#endif

#ifdef ACPI_DBG_TRACK_ALLOCATIONS
    AcpiGbl_DisplayFinalMemStats        = FALSE;
    AcpiGbl_DisableMemTracking          = FALSE;
#endif

    return_ACPI_STATUS (AE_OK);
}

/* Public globals */

ACPI_EXPORT_SYMBOL (AcpiGbl_FADT)
ACPI_EXPORT_SYMBOL (AcpiDbgLevel)
ACPI_EXPORT_SYMBOL (AcpiDbgLayer)
ACPI_EXPORT_SYMBOL (AcpiGpeCount)
ACPI_EXPORT_SYMBOL (AcpiCurrentGpeCount)
