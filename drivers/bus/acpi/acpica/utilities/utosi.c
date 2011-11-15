/******************************************************************************
 *
 * Module Name: utosi - Support for the _OSI predefined control method
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

#define __UTOSI_C__

#include "acpi.h"
#include "accommon.h"


#define _COMPONENT          ACPI_UTILITIES
        ACPI_MODULE_NAME    ("utosi")

/*
 * Strings supported by the _OSI predefined control method (which is
 * implemented internally within this module.)
 *
 * March 2009: Removed "Linux" as this host no longer wants to respond true
 * for this string. Basically, the only safe OS strings are windows-related
 * and in many or most cases represent the only test path within the
 * BIOS-provided ASL code.
 *
 * The last element of each entry is used to track the newest version of
 * Windows that the BIOS has requested.
 */
static ACPI_INTERFACE_INFO    AcpiDefaultSupportedInterfaces[] =
{
    /* Operating System Vendor Strings */

    {"Windows 2000",        NULL, 0, ACPI_OSI_WIN_2000},         /* Windows 2000 */
    {"Windows 2001",        NULL, 0, ACPI_OSI_WIN_XP},           /* Windows XP */
    {"Windows 2001 SP1",    NULL, 0, ACPI_OSI_WIN_XP_SP1},       /* Windows XP SP1 */
    {"Windows 2001.1",      NULL, 0, ACPI_OSI_WINSRV_2003},      /* Windows Server 2003 */
    {"Windows 2001 SP2",    NULL, 0, ACPI_OSI_WIN_XP_SP2},       /* Windows XP SP2 */
    {"Windows 2001.1 SP1",  NULL, 0, ACPI_OSI_WINSRV_2003_SP1},  /* Windows Server 2003 SP1 - Added 03/2006 */
    {"Windows 2006",        NULL, 0, ACPI_OSI_WIN_VISTA},        /* Windows Vista - Added 03/2006 */
    {"Windows 2006.1",      NULL, 0, ACPI_OSI_WINSRV_2008},      /* Windows Server 2008 - Added 09/2009 */
    {"Windows 2006 SP1",    NULL, 0, ACPI_OSI_WIN_VISTA_SP1},    /* Windows Vista SP1 - Added 09/2009 */
    {"Windows 2006 SP2",    NULL, 0, ACPI_OSI_WIN_VISTA_SP2},    /* Windows Vista SP2 - Added 09/2010 */
    {"Windows 2009",        NULL, 0, ACPI_OSI_WIN_7},            /* Windows 7 and Server 2008 R2 - Added 09/2009 */

    /* Feature Group Strings */

    {"Extended Address Space Descriptor", NULL, 0, 0}

    /*
     * All "optional" feature group strings (features that are implemented
     * by the host) should be dynamically added by the host via
     * AcpiInstallInterface and should not be manually added here.
     *
     * Examples of optional feature group strings:
     *
     * "Module Device"
     * "Processor Device"
     * "3.0 Thermal Model"
     * "3.0 _SCP Extensions"
     * "Processor Aggregator Device"
     */
};


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtInitializeInterfaces
 *
 * PARAMETERS:  None
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Initialize the global _OSI supported interfaces list
 *
 ******************************************************************************/

ACPI_STATUS
AcpiUtInitializeInterfaces (
    void)
{
    UINT32                  i;


    (void) AcpiOsAcquireMutex (AcpiGbl_OsiMutex, ACPI_WAIT_FOREVER);
    AcpiGbl_SupportedInterfaces = AcpiDefaultSupportedInterfaces;

    /* Link the static list of supported interfaces */

    for (i = 0; i < (ACPI_ARRAY_LENGTH (AcpiDefaultSupportedInterfaces) - 1); i++)
    {
        AcpiDefaultSupportedInterfaces[i].Next =
            &AcpiDefaultSupportedInterfaces[(ACPI_SIZE) i + 1];
    }

    AcpiOsReleaseMutex (AcpiGbl_OsiMutex);
    return (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtInterfaceTerminate
 *
 * PARAMETERS:  None
 *
 * RETURN:      None
 *
 * DESCRIPTION: Delete all interfaces in the global list. Sets
 *              AcpiGbl_SupportedInterfaces to NULL.
 *
 ******************************************************************************/

void
AcpiUtInterfaceTerminate (
    void)
{
    ACPI_INTERFACE_INFO     *NextInterface;


    (void) AcpiOsAcquireMutex (AcpiGbl_OsiMutex, ACPI_WAIT_FOREVER);
    NextInterface = AcpiGbl_SupportedInterfaces;

    while (NextInterface)
    {
        AcpiGbl_SupportedInterfaces = NextInterface->Next;

        /* Only interfaces added at runtime can be freed */

        if (NextInterface->Flags & ACPI_OSI_DYNAMIC)
        {
            ACPI_FREE (NextInterface->Name);
            ACPI_FREE (NextInterface);
        }

        NextInterface = AcpiGbl_SupportedInterfaces;
    }

    AcpiOsReleaseMutex (AcpiGbl_OsiMutex);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtInstallInterface
 *
 * PARAMETERS:  InterfaceName       - The interface to install
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Install the interface into the global interface list.
 *              Caller MUST hold AcpiGbl_OsiMutex
 *
 ******************************************************************************/

ACPI_STATUS
AcpiUtInstallInterface (
    ACPI_STRING             InterfaceName)
{
    ACPI_INTERFACE_INFO     *InterfaceInfo;


    /* Allocate info block and space for the name string */

    InterfaceInfo = ACPI_ALLOCATE_ZEROED (sizeof (ACPI_INTERFACE_INFO));
    if (!InterfaceInfo)
    {
        return (AE_NO_MEMORY);
    }

    InterfaceInfo->Name = ACPI_ALLOCATE_ZEROED (ACPI_STRLEN (InterfaceName) + 1);
    if (!InterfaceInfo->Name)
    {
        ACPI_FREE (InterfaceInfo);
        return (AE_NO_MEMORY);
    }

    /* Initialize new info and insert at the head of the global list */

    ACPI_STRCPY (InterfaceInfo->Name, InterfaceName);
    InterfaceInfo->Flags = ACPI_OSI_DYNAMIC;
    InterfaceInfo->Next = AcpiGbl_SupportedInterfaces;

    AcpiGbl_SupportedInterfaces = InterfaceInfo;
    return (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtRemoveInterface
 *
 * PARAMETERS:  InterfaceName       - The interface to remove
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Remove the interface from the global interface list.
 *              Caller MUST hold AcpiGbl_OsiMutex
 *
 ******************************************************************************/

ACPI_STATUS
AcpiUtRemoveInterface (
    ACPI_STRING             InterfaceName)
{
    ACPI_INTERFACE_INFO     *PreviousInterface;
    ACPI_INTERFACE_INFO     *NextInterface;


    PreviousInterface = NextInterface = AcpiGbl_SupportedInterfaces;
    while (NextInterface)
    {
        if (!ACPI_STRCMP (InterfaceName, NextInterface->Name))
        {
            /* Found: name is in either the static list or was added at runtime */

            if (NextInterface->Flags & ACPI_OSI_DYNAMIC)
            {
                /* Interface was added dynamically, remove and free it */

                if (PreviousInterface == NextInterface)
                {
                    AcpiGbl_SupportedInterfaces = NextInterface->Next;
                }
                else
                {
                    PreviousInterface->Next = NextInterface->Next;
                }

                ACPI_FREE (NextInterface->Name);
                ACPI_FREE (NextInterface);
            }
            else
            {
                /*
                 * Interface is in static list. If marked invalid, then it
                 * does not actually exist. Else, mark it invalid.
                 */
                if (NextInterface->Flags & ACPI_OSI_INVALID)
                {
                    return (AE_NOT_EXIST);
                }

                NextInterface->Flags |= ACPI_OSI_INVALID;
            }

            return (AE_OK);
        }

        PreviousInterface = NextInterface;
        NextInterface = NextInterface->Next;
    }

    /* Interface was not found */

    return (AE_NOT_EXIST);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtGetInterface
 *
 * PARAMETERS:  InterfaceName       - The interface to find
 *
 * RETURN:      ACPI_INTERFACE_INFO if found. NULL if not found.
 *
 * DESCRIPTION: Search for the specified interface name in the global list.
 *              Caller MUST hold AcpiGbl_OsiMutex
 *
 ******************************************************************************/

ACPI_INTERFACE_INFO *
AcpiUtGetInterface (
    ACPI_STRING             InterfaceName)
{
    ACPI_INTERFACE_INFO     *NextInterface;


    NextInterface = AcpiGbl_SupportedInterfaces;
    while (NextInterface)
    {
        if (!ACPI_STRCMP (InterfaceName, NextInterface->Name))
        {
            return (NextInterface);
        }

        NextInterface = NextInterface->Next;
    }

    return (NULL);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtOsiImplementation
 *
 * PARAMETERS:  WalkState           - Current walk state
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Implementation of the _OSI predefined control method. When
 *              an invocation of _OSI is encountered in the system AML,
 *              control is transferred to this function.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiUtOsiImplementation (
    ACPI_WALK_STATE         *WalkState)
{
    ACPI_OPERAND_OBJECT     *StringDesc;
    ACPI_OPERAND_OBJECT     *ReturnDesc;
    ACPI_INTERFACE_INFO     *InterfaceInfo;
    ACPI_INTERFACE_HANDLER  InterfaceHandler;
    UINT32                  ReturnValue;


    ACPI_FUNCTION_TRACE (UtOsiImplementation);


    /* Validate the string input argument (from the AML caller) */

    StringDesc = WalkState->Arguments[0].Object;
    if (!StringDesc ||
        (StringDesc->Common.Type != ACPI_TYPE_STRING))
    {
        return_ACPI_STATUS (AE_TYPE);
    }

    /* Create a return object */

    ReturnDesc = AcpiUtCreateInternalObject (ACPI_TYPE_INTEGER);
    if (!ReturnDesc)
    {
        return_ACPI_STATUS (AE_NO_MEMORY);
    }

    /* Default return value is 0, NOT SUPPORTED */

    ReturnValue = 0;
    (void) AcpiOsAcquireMutex (AcpiGbl_OsiMutex, ACPI_WAIT_FOREVER);

    /* Lookup the interface in the global _OSI list */

    InterfaceInfo = AcpiUtGetInterface (StringDesc->String.Pointer);
    if (InterfaceInfo &&
        !(InterfaceInfo->Flags & ACPI_OSI_INVALID))
    {
        /*
         * The interface is supported.
         * Update the OsiData if necessary. We keep track of the latest
         * version of Windows that has been requested by the BIOS.
         */
        if (InterfaceInfo->Value > AcpiGbl_OsiData)
        {
            AcpiGbl_OsiData = InterfaceInfo->Value;
        }

        ReturnValue = ACPI_UINT32_MAX;
    }

    AcpiOsReleaseMutex (AcpiGbl_OsiMutex);

    /*
     * Invoke an optional _OSI interface handler. The host OS may wish
     * to do some interface-specific handling. For example, warn about
     * certain interfaces or override the true/false support value.
     */
    InterfaceHandler = AcpiGbl_InterfaceHandler;
    if (InterfaceHandler)
    {
        ReturnValue = InterfaceHandler (
            StringDesc->String.Pointer, ReturnValue);
    }

    ACPI_DEBUG_PRINT_RAW ((ACPI_DB_INFO,
        "ACPI: BIOS _OSI(\"%s\") is %ssupported\n",
        StringDesc->String.Pointer, ReturnValue == 0 ? "not " : ""));

    /* Complete the return object */

    ReturnDesc->Integer.Value = ReturnValue;
    WalkState->ReturnDesc = ReturnDesc;
    return_ACPI_STATUS (AE_OK);
}
