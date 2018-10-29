/******************************************************************************
 *
 * Module Name: nspredef - Validation of ACPI predefined methods and objects
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

#define ACPI_CREATE_PREDEFINED_TABLE

#include "acpi.h"
#include "accommon.h"
#include "acnamesp.h"
#include "acpredef.h"


#define _COMPONENT          ACPI_NAMESPACE
        ACPI_MODULE_NAME    ("nspredef")


/*******************************************************************************
 *
 * This module validates predefined ACPI objects that appear in the namespace,
 * at the time they are evaluated (via AcpiEvaluateObject). The purpose of this
 * validation is to detect problems with BIOS-exposed predefined ACPI objects
 * before the results are returned to the ACPI-related drivers.
 *
 * There are several areas that are validated:
 *
 *  1) The number of input arguments as defined by the method/object in the
 *     ASL is validated against the ACPI specification.
 *  2) The type of the return object (if any) is validated against the ACPI
 *     specification.
 *  3) For returned package objects, the count of package elements is
 *     validated, as well as the type of each package element. Nested
 *     packages are supported.
 *
 * For any problems found, a warning message is issued.
 *
 ******************************************************************************/


/* Local prototypes */

static ACPI_STATUS
AcpiNsCheckReference (
    ACPI_EVALUATE_INFO          *Info,
    ACPI_OPERAND_OBJECT         *ReturnObject);

static UINT32
AcpiNsGetBitmappedType (
    ACPI_OPERAND_OBJECT         *ReturnObject);


/*******************************************************************************
 *
 * FUNCTION:    AcpiNsCheckReturnValue
 *
 * PARAMETERS:  Node            - Namespace node for the method/object
 *              Info            - Method execution information block
 *              UserParamCount  - Number of parameters actually passed
 *              ReturnStatus    - Status from the object evaluation
 *              ReturnObjectPtr - Pointer to the object returned from the
 *                                evaluation of a method or object
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Check the value returned from a predefined name.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiNsCheckReturnValue (
    ACPI_NAMESPACE_NODE         *Node,
    ACPI_EVALUATE_INFO          *Info,
    UINT32                      UserParamCount,
    ACPI_STATUS                 ReturnStatus,
    ACPI_OPERAND_OBJECT         **ReturnObjectPtr)
{
    ACPI_STATUS                 Status;
    const ACPI_PREDEFINED_INFO  *Predefined;


    /* If not a predefined name, we cannot validate the return object */

    Predefined = Info->Predefined;
    if (!Predefined)
    {
        return (AE_OK);
    }

    /*
     * If the method failed or did not actually return an object, we cannot
     * validate the return object
     */
    if ((ReturnStatus != AE_OK) &&
        (ReturnStatus != AE_CTRL_RETURN_VALUE))
    {
        return (AE_OK);
    }

    /*
     * Return value validation and possible repair.
     *
     * 1) Don't perform return value validation/repair if this feature
     * has been disabled via a global option.
     *
     * 2) We have a return value, but if one wasn't expected, just exit,
     * this is not a problem. For example, if the "Implicit Return"
     * feature is enabled, methods will always return a value.
     *
     * 3) If the return value can be of any type, then we cannot perform
     * any validation, just exit.
     */
    if (AcpiGbl_DisableAutoRepair ||
        (!Predefined->Info.ExpectedBtypes) ||
        (Predefined->Info.ExpectedBtypes == ACPI_RTYPE_ALL))
    {
        return (AE_OK);
    }

    /*
     * Check that the type of the main return object is what is expected
     * for this predefined name
     */
    Status = AcpiNsCheckObjectType (Info, ReturnObjectPtr,
        Predefined->Info.ExpectedBtypes, ACPI_NOT_PACKAGE_ELEMENT);
    if (ACPI_FAILURE (Status))
    {
        goto Exit;
    }

    /*
     *
     * 4) If there is no return value and it is optional, just return
     * AE_OK (_WAK).
     */
    if (!(*ReturnObjectPtr))
    {
        goto Exit;
    }

    /*
     * For returned Package objects, check the type of all sub-objects.
     * Note: Package may have been newly created by call above.
     */
    if ((*ReturnObjectPtr)->Common.Type == ACPI_TYPE_PACKAGE)
    {
        Info->ParentPackage = *ReturnObjectPtr;
        Status = AcpiNsCheckPackage (Info, ReturnObjectPtr);
        if (ACPI_FAILURE (Status))
        {
            /* We might be able to fix some errors */

            if ((Status != AE_AML_OPERAND_TYPE) &&
                (Status != AE_AML_OPERAND_VALUE))
            {
                goto Exit;
            }
        }
    }

    /*
     * The return object was OK, or it was successfully repaired above.
     * Now make some additional checks such as verifying that package
     * objects are sorted correctly (if required) or buffer objects have
     * the correct data width (bytes vs. dwords). These repairs are
     * performed on a per-name basis, i.e., the code is specific to
     * particular predefined names.
     */
    Status = AcpiNsComplexRepairs (Info, Node, Status, ReturnObjectPtr);

Exit:
    /*
     * If the object validation failed or if we successfully repaired one
     * or more objects, mark the parent node to suppress further warning
     * messages during the next evaluation of the same method/object.
     */
    if (ACPI_FAILURE (Status) ||
       (Info->ReturnFlags & ACPI_OBJECT_REPAIRED))
    {
        Node->Flags |= ANOBJ_EVALUATED;
    }

    return (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiNsCheckObjectType
 *
 * PARAMETERS:  Info            - Method execution information block
 *              ReturnObjectPtr - Pointer to the object returned from the
 *                                evaluation of a method or object
 *              ExpectedBtypes  - Bitmap of expected return type(s)
 *              PackageIndex    - Index of object within parent package (if
 *                                applicable - ACPI_NOT_PACKAGE_ELEMENT
 *                                otherwise)
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Check the type of the return object against the expected object
 *              type(s). Use of Btype allows multiple expected object types.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiNsCheckObjectType (
    ACPI_EVALUATE_INFO          *Info,
    ACPI_OPERAND_OBJECT         **ReturnObjectPtr,
    UINT32                      ExpectedBtypes,
    UINT32                      PackageIndex)
{
    ACPI_OPERAND_OBJECT         *ReturnObject = *ReturnObjectPtr;
    ACPI_STATUS                 Status = AE_OK;
    char                        TypeBuffer[96]; /* Room for 10 types */


    /* A Namespace node should not get here, but make sure */

    if (ReturnObject &&
        ACPI_GET_DESCRIPTOR_TYPE (ReturnObject) == ACPI_DESC_TYPE_NAMED)
    {
        ACPI_WARN_PREDEFINED ((AE_INFO, Info->FullPathname, Info->NodeFlags,
            "Invalid return type - Found a Namespace node [%4.4s] type %s",
            ReturnObject->Node.Name.Ascii,
            AcpiUtGetTypeName (ReturnObject->Node.Type)));
        return (AE_AML_OPERAND_TYPE);
    }

    /*
     * Convert the object type (ACPI_TYPE_xxx) to a bitmapped object type.
     * The bitmapped type allows multiple possible return types.
     *
     * Note, the cases below must handle all of the possible types returned
     * from all of the predefined names (including elements of returned
     * packages)
     */
    Info->ReturnBtype = AcpiNsGetBitmappedType (ReturnObject);
    if (Info->ReturnBtype == ACPI_RTYPE_ANY)
    {
        /* Not one of the supported objects, must be incorrect */
        goto TypeErrorExit;
    }

    /* For reference objects, check that the reference type is correct */

    if ((Info->ReturnBtype & ExpectedBtypes) == ACPI_RTYPE_REFERENCE)
    {
        Status = AcpiNsCheckReference (Info, ReturnObject);
        return (Status);
    }

    /* Attempt simple repair of the returned object if necessary */

    Status = AcpiNsSimpleRepair (Info, ExpectedBtypes,
        PackageIndex, ReturnObjectPtr);
    if (ACPI_SUCCESS (Status))
    {
        return (AE_OK); /* Successful repair */
    }


TypeErrorExit:

    /* Create a string with all expected types for this predefined object */

    AcpiUtGetExpectedReturnTypes (TypeBuffer, ExpectedBtypes);

    if (!ReturnObject)
    {
        ACPI_WARN_PREDEFINED ((AE_INFO, Info->FullPathname, Info->NodeFlags,
            "Expected return object of type %s",
            TypeBuffer));
    }
    else if (PackageIndex == ACPI_NOT_PACKAGE_ELEMENT)
    {
        ACPI_WARN_PREDEFINED ((AE_INFO, Info->FullPathname, Info->NodeFlags,
            "Return type mismatch - found %s, expected %s",
            AcpiUtGetObjectTypeName (ReturnObject), TypeBuffer));
    }
    else
    {
        ACPI_WARN_PREDEFINED ((AE_INFO, Info->FullPathname, Info->NodeFlags,
            "Return Package type mismatch at index %u - "
            "found %s, expected %s", PackageIndex,
            AcpiUtGetObjectTypeName (ReturnObject), TypeBuffer));
    }

    return (AE_AML_OPERAND_TYPE);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiNsCheckReference
 *
 * PARAMETERS:  Info            - Method execution information block
 *              ReturnObject    - Object returned from the evaluation of a
 *                                method or object
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Check a returned reference object for the correct reference
 *              type. The only reference type that can be returned from a
 *              predefined method is a named reference. All others are invalid.
 *
 ******************************************************************************/

static ACPI_STATUS
AcpiNsCheckReference (
    ACPI_EVALUATE_INFO          *Info,
    ACPI_OPERAND_OBJECT         *ReturnObject)
{

    /*
     * Check the reference object for the correct reference type (opcode).
     * The only type of reference that can be converted to an ACPI_OBJECT is
     * a reference to a named object (reference class: NAME)
     */
    if (ReturnObject->Reference.Class == ACPI_REFCLASS_NAME)
    {
        return (AE_OK);
    }

    ACPI_WARN_PREDEFINED ((AE_INFO, Info->FullPathname, Info->NodeFlags,
        "Return type mismatch - unexpected reference object type [%s] %2.2X",
        AcpiUtGetReferenceName (ReturnObject),
        ReturnObject->Reference.Class));

    return (AE_AML_OPERAND_TYPE);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiNsGetBitmappedType
 *
 * PARAMETERS:  ReturnObject    - Object returned from method/obj evaluation
 *
 * RETURN:      Object return type. ACPI_RTYPE_ANY indicates that the object
 *              type is not supported. ACPI_RTYPE_NONE indicates that no
 *              object was returned (ReturnObject is NULL).
 *
 * DESCRIPTION: Convert object type into a bitmapped object return type.
 *
 ******************************************************************************/

static UINT32
AcpiNsGetBitmappedType (
    ACPI_OPERAND_OBJECT         *ReturnObject)
{
    UINT32                      ReturnBtype;


    if (!ReturnObject)
    {
        return (ACPI_RTYPE_NONE);
    }

    /* Map ACPI_OBJECT_TYPE to internal bitmapped type */

    switch (ReturnObject->Common.Type)
    {
    case ACPI_TYPE_INTEGER:

        ReturnBtype = ACPI_RTYPE_INTEGER;
        break;

    case ACPI_TYPE_BUFFER:

        ReturnBtype = ACPI_RTYPE_BUFFER;
        break;

    case ACPI_TYPE_STRING:

        ReturnBtype = ACPI_RTYPE_STRING;
        break;

    case ACPI_TYPE_PACKAGE:

        ReturnBtype = ACPI_RTYPE_PACKAGE;
        break;

    case ACPI_TYPE_LOCAL_REFERENCE:

        ReturnBtype = ACPI_RTYPE_REFERENCE;
        break;

    default:

        /* Not one of the supported objects, must be incorrect */

        ReturnBtype = ACPI_RTYPE_ANY;
        break;
    }

    return (ReturnBtype);
}
