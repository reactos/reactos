/******************************************************************************
 *
 * Module Name: nsrepair2 - Repair for objects returned by specific
 *                          predefined methods
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

#define __NSREPAIR2_C__

#include "acpi.h"
#include "accommon.h"
#include "acnamesp.h"

#define _COMPONENT          ACPI_NAMESPACE
        ACPI_MODULE_NAME    ("nsrepair2")


/*
 * Information structure and handler for ACPI predefined names that can
 * be repaired on a per-name basis.
 */
typedef
ACPI_STATUS (*ACPI_REPAIR_FUNCTION) (
    ACPI_PREDEFINED_DATA    *Data,
    ACPI_OPERAND_OBJECT     **ReturnObjectPtr);

typedef struct acpi_repair_info
{
    char                    Name[ACPI_NAME_SIZE];
    ACPI_REPAIR_FUNCTION    RepairFunction;

} ACPI_REPAIR_INFO;


/* Local prototypes */

static const ACPI_REPAIR_INFO *
AcpiNsMatchRepairableName (
    ACPI_NAMESPACE_NODE     *Node);

static ACPI_STATUS
AcpiNsRepair_ALR (
    ACPI_PREDEFINED_DATA    *Data,
    ACPI_OPERAND_OBJECT     **ReturnObjectPtr);

static ACPI_STATUS
AcpiNsRepair_CID (
    ACPI_PREDEFINED_DATA    *Data,
    ACPI_OPERAND_OBJECT     **ReturnObjectPtr);

static ACPI_STATUS
AcpiNsRepair_FDE (
    ACPI_PREDEFINED_DATA    *Data,
    ACPI_OPERAND_OBJECT     **ReturnObjectPtr);

static ACPI_STATUS
AcpiNsRepair_HID (
    ACPI_PREDEFINED_DATA    *Data,
    ACPI_OPERAND_OBJECT     **ReturnObjectPtr);

static ACPI_STATUS
AcpiNsRepair_PSS (
    ACPI_PREDEFINED_DATA    *Data,
    ACPI_OPERAND_OBJECT     **ReturnObjectPtr);

static ACPI_STATUS
AcpiNsRepair_TSS (
    ACPI_PREDEFINED_DATA    *Data,
    ACPI_OPERAND_OBJECT     **ReturnObjectPtr);

static ACPI_STATUS
AcpiNsCheckSortedList (
    ACPI_PREDEFINED_DATA    *Data,
    ACPI_OPERAND_OBJECT     *ReturnObject,
    UINT32                  ExpectedCount,
    UINT32                  SortIndex,
    UINT8                   SortDirection,
    char                    *SortKeyName);

static void
AcpiNsSortList (
    ACPI_OPERAND_OBJECT     **Elements,
    UINT32                  Count,
    UINT32                  Index,
    UINT8                   SortDirection);

/* Values for SortDirection above */

#define ACPI_SORT_ASCENDING     0
#define ACPI_SORT_DESCENDING    1


/*
 * This table contains the names of the predefined methods for which we can
 * perform more complex repairs.
 *
 * As necessary:
 *
 * _ALR: Sort the list ascending by AmbientIlluminance
 * _CID: Strings: uppercase all, remove any leading asterisk
 * _FDE: Convert Buffer of BYTEs to a Buffer of DWORDs
 * _GTM: Convert Buffer of BYTEs to a Buffer of DWORDs
 * _HID: Strings: uppercase all, remove any leading asterisk
 * _PSS: Sort the list descending by Power
 * _TSS: Sort the list descending by Power
 *
 * Names that must be packages, but cannot be sorted:
 *
 * _BCL: Values are tied to the Package index where they appear, and cannot
 * be moved or sorted. These index values are used for _BQC and _BCM.
 * However, we can fix the case where a buffer is returned, by converting
 * it to a Package of integers.
 */
static const ACPI_REPAIR_INFO       AcpiNsRepairableNames[] =
{
    {"_ALR", AcpiNsRepair_ALR},
    {"_CID", AcpiNsRepair_CID},
    {"_FDE", AcpiNsRepair_FDE},
    {"_GTM", AcpiNsRepair_FDE},     /* _GTM has same repair as _FDE */
    {"_HID", AcpiNsRepair_HID},
    {"_PSS", AcpiNsRepair_PSS},
    {"_TSS", AcpiNsRepair_TSS},
    {{0,0,0,0}, NULL}               /* Table terminator */
};


#define ACPI_FDE_FIELD_COUNT        5
#define ACPI_FDE_BYTE_BUFFER_SIZE   5
#define ACPI_FDE_DWORD_BUFFER_SIZE  (ACPI_FDE_FIELD_COUNT * sizeof (UINT32))


/******************************************************************************
 *
 * FUNCTION:    AcpiNsComplexRepairs
 *
 * PARAMETERS:  Data                - Pointer to validation data structure
 *              Node                - Namespace node for the method/object
 *              ValidateStatus      - Original status of earlier validation
 *              ReturnObjectPtr     - Pointer to the object returned from the
 *                                    evaluation of a method or object
 *
 * RETURN:      Status. AE_OK if repair was successful. If name is not
 *              matched, ValidateStatus is returned.
 *
 * DESCRIPTION: Attempt to repair/convert a return object of a type that was
 *              not expected.
 *
 *****************************************************************************/

ACPI_STATUS
AcpiNsComplexRepairs (
    ACPI_PREDEFINED_DATA    *Data,
    ACPI_NAMESPACE_NODE     *Node,
    ACPI_STATUS             ValidateStatus,
    ACPI_OPERAND_OBJECT     **ReturnObjectPtr)
{
    const ACPI_REPAIR_INFO  *Predefined;
    ACPI_STATUS             Status;


    /* Check if this name is in the list of repairable names */

    Predefined = AcpiNsMatchRepairableName (Node);
    if (!Predefined)
    {
        return (ValidateStatus);
    }

    Status = Predefined->RepairFunction (Data, ReturnObjectPtr);
    return (Status);
}


/******************************************************************************
 *
 * FUNCTION:    AcpiNsMatchRepairableName
 *
 * PARAMETERS:  Node                - Namespace node for the method/object
 *
 * RETURN:      Pointer to entry in repair table. NULL indicates not found.
 *
 * DESCRIPTION: Check an object name against the repairable object list.
 *
 *****************************************************************************/

static const ACPI_REPAIR_INFO *
AcpiNsMatchRepairableName (
    ACPI_NAMESPACE_NODE     *Node)
{
    const ACPI_REPAIR_INFO  *ThisName;


    /* Search info table for a repairable predefined method/object name */

    ThisName = AcpiNsRepairableNames;
    while (ThisName->RepairFunction)
    {
        if (ACPI_COMPARE_NAME (Node->Name.Ascii, ThisName->Name))
        {
            return (ThisName);
        }
        ThisName++;
    }

    return (NULL); /* Not found */
}


/******************************************************************************
 *
 * FUNCTION:    AcpiNsRepair_ALR
 *
 * PARAMETERS:  Data                - Pointer to validation data structure
 *              ReturnObjectPtr     - Pointer to the object returned from the
 *                                    evaluation of a method or object
 *
 * RETURN:      Status. AE_OK if object is OK or was repaired successfully
 *
 * DESCRIPTION: Repair for the _ALR object. If necessary, sort the object list
 *              ascending by the ambient illuminance values.
 *
 *****************************************************************************/

static ACPI_STATUS
AcpiNsRepair_ALR (
    ACPI_PREDEFINED_DATA    *Data,
    ACPI_OPERAND_OBJECT     **ReturnObjectPtr)
{
    ACPI_OPERAND_OBJECT     *ReturnObject = *ReturnObjectPtr;
    ACPI_STATUS             Status;


    Status = AcpiNsCheckSortedList (Data, ReturnObject, 2, 1,
                ACPI_SORT_ASCENDING, "AmbientIlluminance");

    return (Status);
}


/******************************************************************************
 *
 * FUNCTION:    AcpiNsRepair_FDE
 *
 * PARAMETERS:  Data                - Pointer to validation data structure
 *              ReturnObjectPtr     - Pointer to the object returned from the
 *                                    evaluation of a method or object
 *
 * RETURN:      Status. AE_OK if object is OK or was repaired successfully
 *
 * DESCRIPTION: Repair for the _FDE and _GTM objects. The expected return
 *              value is a Buffer of 5 DWORDs. This function repairs a common
 *              problem where the return value is a Buffer of BYTEs, not
 *              DWORDs.
 *
 *****************************************************************************/

static ACPI_STATUS
AcpiNsRepair_FDE (
    ACPI_PREDEFINED_DATA    *Data,
    ACPI_OPERAND_OBJECT     **ReturnObjectPtr)
{
    ACPI_OPERAND_OBJECT     *ReturnObject = *ReturnObjectPtr;
    ACPI_OPERAND_OBJECT     *BufferObject;
    UINT8                   *ByteBuffer;
    UINT32                  *DwordBuffer;
    UINT32                  i;


    ACPI_FUNCTION_NAME (NsRepair_FDE);


    switch (ReturnObject->Common.Type)
    {
    case ACPI_TYPE_BUFFER:

        /* This is the expected type. Length should be (at least) 5 DWORDs */

        if (ReturnObject->Buffer.Length >= ACPI_FDE_DWORD_BUFFER_SIZE)
        {
            return (AE_OK);
        }

        /* We can only repair if we have exactly 5 BYTEs */

        if (ReturnObject->Buffer.Length != ACPI_FDE_BYTE_BUFFER_SIZE)
        {
            ACPI_WARN_PREDEFINED ((AE_INFO, Data->Pathname, Data->NodeFlags,
                "Incorrect return buffer length %u, expected %u",
                ReturnObject->Buffer.Length, ACPI_FDE_DWORD_BUFFER_SIZE));

            return (AE_AML_OPERAND_TYPE);
        }

        /* Create the new (larger) buffer object */

        BufferObject = AcpiUtCreateBufferObject (ACPI_FDE_DWORD_BUFFER_SIZE);
        if (!BufferObject)
        {
            return (AE_NO_MEMORY);
        }

        /* Expand each byte to a DWORD */

        ByteBuffer = ReturnObject->Buffer.Pointer;
        DwordBuffer = ACPI_CAST_PTR (UINT32, BufferObject->Buffer.Pointer);

        for (i = 0; i < ACPI_FDE_FIELD_COUNT; i++)
        {
            *DwordBuffer = (UINT32) *ByteBuffer;
            DwordBuffer++;
            ByteBuffer++;
        }

        ACPI_DEBUG_PRINT ((ACPI_DB_REPAIR,
            "%s Expanded Byte Buffer to expected DWord Buffer\n",
            Data->Pathname));
        break;

    default:
        return (AE_AML_OPERAND_TYPE);
    }

    /* Delete the original return object, return the new buffer object */

    AcpiUtRemoveReference (ReturnObject);
    *ReturnObjectPtr = BufferObject;

    Data->Flags |= ACPI_OBJECT_REPAIRED;
    return (AE_OK);
}


/******************************************************************************
 *
 * FUNCTION:    AcpiNsRepair_CID
 *
 * PARAMETERS:  Data                - Pointer to validation data structure
 *              ReturnObjectPtr     - Pointer to the object returned from the
 *                                    evaluation of a method or object
 *
 * RETURN:      Status. AE_OK if object is OK or was repaired successfully
 *
 * DESCRIPTION: Repair for the _CID object. If a string, ensure that all
 *              letters are uppercase and that there is no leading asterisk.
 *              If a Package, ensure same for all string elements.
 *
 *****************************************************************************/

static ACPI_STATUS
AcpiNsRepair_CID (
    ACPI_PREDEFINED_DATA    *Data,
    ACPI_OPERAND_OBJECT     **ReturnObjectPtr)
{
    ACPI_STATUS             Status;
    ACPI_OPERAND_OBJECT     *ReturnObject = *ReturnObjectPtr;
    ACPI_OPERAND_OBJECT     **ElementPtr;
    ACPI_OPERAND_OBJECT     *OriginalElement;
    UINT16                  OriginalRefCount;
    UINT32                  i;


    /* Check for _CID as a simple string */

    if (ReturnObject->Common.Type == ACPI_TYPE_STRING)
    {
        Status = AcpiNsRepair_HID (Data, ReturnObjectPtr);
        return (Status);
    }

    /* Exit if not a Package */

    if (ReturnObject->Common.Type != ACPI_TYPE_PACKAGE)
    {
        return (AE_OK);
    }

    /* Examine each element of the _CID package */

    ElementPtr = ReturnObject->Package.Elements;
    for (i = 0; i < ReturnObject->Package.Count; i++)
    {
        OriginalElement = *ElementPtr;
        OriginalRefCount = OriginalElement->Common.ReferenceCount;

        Status = AcpiNsRepair_HID (Data, ElementPtr);
        if (ACPI_FAILURE (Status))
        {
            return (Status);
        }

        /* Take care with reference counts */

        if (OriginalElement != *ElementPtr)
        {
            /* Element was replaced */

            (*ElementPtr)->Common.ReferenceCount =
                OriginalRefCount;

            AcpiUtRemoveReference (OriginalElement);
        }

        ElementPtr++;
    }

    return (AE_OK);
}


/******************************************************************************
 *
 * FUNCTION:    AcpiNsRepair_HID
 *
 * PARAMETERS:  Data                - Pointer to validation data structure
 *              ReturnObjectPtr     - Pointer to the object returned from the
 *                                    evaluation of a method or object
 *
 * RETURN:      Status. AE_OK if object is OK or was repaired successfully
 *
 * DESCRIPTION: Repair for the _HID object. If a string, ensure that all
 *              letters are uppercase and that there is no leading asterisk.
 *
 *****************************************************************************/

static ACPI_STATUS
AcpiNsRepair_HID (
    ACPI_PREDEFINED_DATA    *Data,
    ACPI_OPERAND_OBJECT     **ReturnObjectPtr)
{
    ACPI_OPERAND_OBJECT     *ReturnObject = *ReturnObjectPtr;
    ACPI_OPERAND_OBJECT     *NewString;
    char                    *Source;
    char                    *Dest;


    ACPI_FUNCTION_NAME (NsRepair_HID);


    /* We only care about string _HID objects (not integers) */

    if (ReturnObject->Common.Type != ACPI_TYPE_STRING)
    {
        return (AE_OK);
    }

    if (ReturnObject->String.Length == 0)
    {
        ACPI_WARN_PREDEFINED ((AE_INFO, Data->Pathname, Data->NodeFlags,
            "Invalid zero-length _HID or _CID string"));

        /* Return AE_OK anyway, let driver handle it */

        Data->Flags |= ACPI_OBJECT_REPAIRED;
        return (AE_OK);
    }

    /* It is simplest to always create a new string object */

    NewString = AcpiUtCreateStringObject (ReturnObject->String.Length);
    if (!NewString)
    {
        return (AE_NO_MEMORY);
    }

    /*
     * Remove a leading asterisk if present. For some unknown reason, there
     * are many machines in the field that contains IDs like this.
     *
     * Examples: "*PNP0C03", "*ACPI0003"
     */
    Source = ReturnObject->String.Pointer;
    if (*Source == '*')
    {
        Source++;
        NewString->String.Length--;

        ACPI_DEBUG_PRINT ((ACPI_DB_REPAIR,
            "%s: Removed invalid leading asterisk\n", Data->Pathname));
    }

    /*
     * Copy and uppercase the string. From the ACPI specification:
     *
     * A valid PNP ID must be of the form "AAA####" where A is an uppercase
     * letter and # is a hex digit. A valid ACPI ID must be of the form
     * "ACPI####" where # is a hex digit.
     */
    for (Dest = NewString->String.Pointer; *Source; Dest++, Source++)
    {
        *Dest = (char) ACPI_TOUPPER (*Source);
    }

    AcpiUtRemoveReference (ReturnObject);
    *ReturnObjectPtr = NewString;
    return (AE_OK);
}


/******************************************************************************
 *
 * FUNCTION:    AcpiNsRepair_TSS
 *
 * PARAMETERS:  Data                - Pointer to validation data structure
 *              ReturnObjectPtr     - Pointer to the object returned from the
 *                                    evaluation of a method or object
 *
 * RETURN:      Status. AE_OK if object is OK or was repaired successfully
 *
 * DESCRIPTION: Repair for the _TSS object. If necessary, sort the object list
 *              descending by the power dissipation values.
 *
 *****************************************************************************/

static ACPI_STATUS
AcpiNsRepair_TSS (
    ACPI_PREDEFINED_DATA    *Data,
    ACPI_OPERAND_OBJECT     **ReturnObjectPtr)
{
    ACPI_OPERAND_OBJECT     *ReturnObject = *ReturnObjectPtr;
    ACPI_STATUS             Status;
    ACPI_NAMESPACE_NODE     *Node;


    /*
     * We can only sort the _TSS return package if there is no _PSS in the
     * same scope. This is because if _PSS is present, the ACPI specification
     * dictates that the _TSS Power Dissipation field is to be ignored, and
     * therefore some BIOSs leave garbage values in the _TSS Power field(s).
     * In this case, it is best to just return the _TSS package as-is.
     * (May, 2011)
     */
    Status = AcpiNsGetNode (Data->Node, "^_PSS", ACPI_NS_NO_UPSEARCH, &Node);
    if (ACPI_SUCCESS (Status))
    {
        return (AE_OK);
    }

    Status = AcpiNsCheckSortedList (Data, ReturnObject, 5, 1,
                ACPI_SORT_DESCENDING, "PowerDissipation");

    return (Status);
}


/******************************************************************************
 *
 * FUNCTION:    AcpiNsRepair_PSS
 *
 * PARAMETERS:  Data                - Pointer to validation data structure
 *              ReturnObjectPtr     - Pointer to the object returned from the
 *                                    evaluation of a method or object
 *
 * RETURN:      Status. AE_OK if object is OK or was repaired successfully
 *
 * DESCRIPTION: Repair for the _PSS object. If necessary, sort the object list
 *              by the CPU frequencies. Check that the power dissipation values
 *              are all proportional to CPU frequency (i.e., sorting by
 *              frequency should be the same as sorting by power.)
 *
 *****************************************************************************/

static ACPI_STATUS
AcpiNsRepair_PSS (
    ACPI_PREDEFINED_DATA    *Data,
    ACPI_OPERAND_OBJECT     **ReturnObjectPtr)
{
    ACPI_OPERAND_OBJECT     *ReturnObject = *ReturnObjectPtr;
    ACPI_OPERAND_OBJECT     **OuterElements;
    UINT32                  OuterElementCount;
    ACPI_OPERAND_OBJECT     **Elements;
    ACPI_OPERAND_OBJECT     *ObjDesc;
    UINT32                  PreviousValue;
    ACPI_STATUS             Status;
    UINT32                  i;


    /*
     * Entries (sub-packages) in the _PSS Package must be sorted by power
     * dissipation, in descending order. If it appears that the list is
     * incorrectly sorted, sort it. We sort by CpuFrequency, since this
     * should be proportional to the power.
     */
    Status =AcpiNsCheckSortedList (Data, ReturnObject, 6, 0,
                ACPI_SORT_DESCENDING, "CpuFrequency");
    if (ACPI_FAILURE (Status))
    {
        return (Status);
    }

    /*
     * We now know the list is correctly sorted by CPU frequency. Check if
     * the power dissipation values are proportional.
     */
    PreviousValue = ACPI_UINT32_MAX;
    OuterElements = ReturnObject->Package.Elements;
    OuterElementCount = ReturnObject->Package.Count;

    for (i = 0; i < OuterElementCount; i++)
    {
        Elements = (*OuterElements)->Package.Elements;
        ObjDesc = Elements[1]; /* Index1 = PowerDissipation */

        if ((UINT32) ObjDesc->Integer.Value > PreviousValue)
        {
            ACPI_WARN_PREDEFINED ((AE_INFO, Data->Pathname, Data->NodeFlags,
                "SubPackage[%u,%u] - suspicious power dissipation values",
                i-1, i));
        }

        PreviousValue = (UINT32) ObjDesc->Integer.Value;
        OuterElements++;
    }

    return (AE_OK);
}


/******************************************************************************
 *
 * FUNCTION:    AcpiNsCheckSortedList
 *
 * PARAMETERS:  Data                - Pointer to validation data structure
 *              ReturnObject        - Pointer to the top-level returned object
 *              ExpectedCount       - Minimum length of each sub-package
 *              SortIndex           - Sub-package entry to sort on
 *              SortDirection       - Ascending or descending
 *              SortKeyName         - Name of the SortIndex field
 *
 * RETURN:      Status. AE_OK if the list is valid and is sorted correctly or
 *              has been repaired by sorting the list.
 *
 * DESCRIPTION: Check if the package list is valid and sorted correctly by the
 *              SortIndex. If not, then sort the list.
 *
 *****************************************************************************/

static ACPI_STATUS
AcpiNsCheckSortedList (
    ACPI_PREDEFINED_DATA    *Data,
    ACPI_OPERAND_OBJECT     *ReturnObject,
    UINT32                  ExpectedCount,
    UINT32                  SortIndex,
    UINT8                   SortDirection,
    char                    *SortKeyName)
{
    UINT32                  OuterElementCount;
    ACPI_OPERAND_OBJECT     **OuterElements;
    ACPI_OPERAND_OBJECT     **Elements;
    ACPI_OPERAND_OBJECT     *ObjDesc;
    UINT32                  i;
    UINT32                  PreviousValue;


    ACPI_FUNCTION_NAME (NsCheckSortedList);


    /* The top-level object must be a package */

    if (ReturnObject->Common.Type != ACPI_TYPE_PACKAGE)
    {
        return (AE_AML_OPERAND_TYPE);
    }

    /*
     * NOTE: assumes list of sub-packages contains no NULL elements.
     * Any NULL elements should have been removed by earlier call
     * to AcpiNsRemoveNullElements.
     */
    OuterElements = ReturnObject->Package.Elements;
    OuterElementCount = ReturnObject->Package.Count;
    if (!OuterElementCount)
    {
        return (AE_AML_PACKAGE_LIMIT);
    }

    PreviousValue = 0;
    if (SortDirection == ACPI_SORT_DESCENDING)
    {
        PreviousValue = ACPI_UINT32_MAX;
    }

    /* Examine each subpackage */

    for (i = 0; i < OuterElementCount; i++)
    {
        /* Each element of the top-level package must also be a package */

        if ((*OuterElements)->Common.Type != ACPI_TYPE_PACKAGE)
        {
            return (AE_AML_OPERAND_TYPE);
        }

        /* Each sub-package must have the minimum length */

        if ((*OuterElements)->Package.Count < ExpectedCount)
        {
            return (AE_AML_PACKAGE_LIMIT);
        }

        Elements = (*OuterElements)->Package.Elements;
        ObjDesc = Elements[SortIndex];

        if (ObjDesc->Common.Type != ACPI_TYPE_INTEGER)
        {
            return (AE_AML_OPERAND_TYPE);
        }

        /*
         * The list must be sorted in the specified order. If we detect a
         * discrepancy, sort the entire list.
         */
        if (((SortDirection == ACPI_SORT_ASCENDING) &&
                (ObjDesc->Integer.Value < PreviousValue)) ||
            ((SortDirection == ACPI_SORT_DESCENDING) &&
                (ObjDesc->Integer.Value > PreviousValue)))
        {
            AcpiNsSortList (ReturnObject->Package.Elements,
                OuterElementCount, SortIndex, SortDirection);

            Data->Flags |= ACPI_OBJECT_REPAIRED;

            ACPI_DEBUG_PRINT ((ACPI_DB_REPAIR,
                "%s: Repaired unsorted list - now sorted by %s\n",
                Data->Pathname, SortKeyName));
            return (AE_OK);
        }

        PreviousValue = (UINT32) ObjDesc->Integer.Value;
        OuterElements++;
    }

    return (AE_OK);
}


/******************************************************************************
 *
 * FUNCTION:    AcpiNsSortList
 *
 * PARAMETERS:  Elements            - Package object element list
 *              Count               - Element count for above
 *              Index               - Sort by which package element
 *              SortDirection       - Ascending or Descending sort
 *
 * RETURN:      None
 *
 * DESCRIPTION: Sort the objects that are in a package element list.
 *
 * NOTE: Assumes that all NULL elements have been removed from the package,
 *       and that all elements have been verified to be of type Integer.
 *
 *****************************************************************************/

static void
AcpiNsSortList (
    ACPI_OPERAND_OBJECT     **Elements,
    UINT32                  Count,
    UINT32                  Index,
    UINT8                   SortDirection)
{
    ACPI_OPERAND_OBJECT     *ObjDesc1;
    ACPI_OPERAND_OBJECT     *ObjDesc2;
    ACPI_OPERAND_OBJECT     *TempObj;
    UINT32                  i;
    UINT32                  j;


    /* Simple bubble sort */

    for (i = 1; i < Count; i++)
    {
        for (j = (Count - 1); j >= i; j--)
        {
            ObjDesc1 = Elements[j-1]->Package.Elements[Index];
            ObjDesc2 = Elements[j]->Package.Elements[Index];

            if (((SortDirection == ACPI_SORT_ASCENDING) &&
                    (ObjDesc1->Integer.Value > ObjDesc2->Integer.Value)) ||

                ((SortDirection == ACPI_SORT_DESCENDING) &&
                    (ObjDesc1->Integer.Value < ObjDesc2->Integer.Value)))
            {
                TempObj = Elements[j-1];
                Elements[j-1] = Elements[j];
                Elements[j] = TempObj;
            }
        }
    }
}
