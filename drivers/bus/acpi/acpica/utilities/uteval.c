/******************************************************************************
 *
 * Module Name: uteval - Object evaluation
 *
 *****************************************************************************/

/*
 * Copyright (C) 2000 - 2021, Intel Corp.
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
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
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
#include "acnamesp.h"


#define _COMPONENT          ACPI_UTILITIES
        ACPI_MODULE_NAME    ("uteval")


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtEvaluateObject
 *
 * PARAMETERS:  PrefixNode          - Starting node
 *              Path                - Path to object from starting node
 *              ExpectedReturnTypes - Bitmap of allowed return types
 *              ReturnDesc          - Where a return value is stored
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Evaluates a namespace object and verifies the type of the
 *              return object. Common code that simplifies accessing objects
 *              that have required return objects of fixed types.
 *
 *              NOTE: Internal function, no parameter validation
 *
 ******************************************************************************/

ACPI_STATUS
AcpiUtEvaluateObject (
    ACPI_NAMESPACE_NODE     *PrefixNode,
    const char              *Path,
    UINT32                  ExpectedReturnBtypes,
    ACPI_OPERAND_OBJECT     **ReturnDesc)
{
    ACPI_EVALUATE_INFO      *Info;
    ACPI_STATUS             Status;
    UINT32                  ReturnBtype;


    ACPI_FUNCTION_TRACE (UtEvaluateObject);


    /* Allocate the evaluation information block */

    Info = ACPI_ALLOCATE_ZEROED (sizeof (ACPI_EVALUATE_INFO));
    if (!Info)
    {
        return_ACPI_STATUS (AE_NO_MEMORY);
    }

    Info->PrefixNode = PrefixNode;
    Info->RelativePathname = Path;

    /* Evaluate the object/method */

    Status = AcpiNsEvaluate (Info);
    if (ACPI_FAILURE (Status))
    {
        if (Status == AE_NOT_FOUND)
        {
            ACPI_DEBUG_PRINT ((ACPI_DB_EXEC, "[%4.4s.%s] was not found\n",
                AcpiUtGetNodeName (PrefixNode), Path));
        }
        else
        {
            ACPI_ERROR_METHOD ("Method execution failed",
                PrefixNode, Path, Status);
        }

        goto Cleanup;
    }

    /* Did we get a return object? */

    if (!Info->ReturnObject)
    {
        if (ExpectedReturnBtypes)
        {
            ACPI_ERROR_METHOD ("No object was returned from",
                PrefixNode, Path, AE_NOT_EXIST);

            Status = AE_NOT_EXIST;
        }

        goto Cleanup;
    }

    /* Map the return object type to the bitmapped type */

    switch ((Info->ReturnObject)->Common.Type)
    {
    case ACPI_TYPE_INTEGER:

        ReturnBtype = ACPI_BTYPE_INTEGER;
        break;

    case ACPI_TYPE_BUFFER:

        ReturnBtype = ACPI_BTYPE_BUFFER;
        break;

    case ACPI_TYPE_STRING:

        ReturnBtype = ACPI_BTYPE_STRING;
        break;

    case ACPI_TYPE_PACKAGE:

        ReturnBtype = ACPI_BTYPE_PACKAGE;
        break;

    default:

        ReturnBtype = 0;
        break;
    }

    if ((AcpiGbl_EnableInterpreterSlack) &&
        (!ExpectedReturnBtypes))
    {
        /*
         * We received a return object, but one was not expected. This can
         * happen frequently if the "implicit return" feature is enabled.
         * Just delete the return object and return AE_OK.
         */
        AcpiUtRemoveReference (Info->ReturnObject);
        goto Cleanup;
    }

    /* Is the return object one of the expected types? */

    if (!(ExpectedReturnBtypes & ReturnBtype))
    {
        ACPI_ERROR_METHOD ("Return object type is incorrect",
            PrefixNode, Path, AE_TYPE);

        ACPI_ERROR ((AE_INFO,
            "Type returned from %s was incorrect: %s, expected Btypes: 0x%X",
            Path, AcpiUtGetObjectTypeName (Info->ReturnObject),
            ExpectedReturnBtypes));

        /* On error exit, we must delete the return object */

        AcpiUtRemoveReference (Info->ReturnObject);
        Status = AE_TYPE;
        goto Cleanup;
    }

    /* Object type is OK, return it */

    *ReturnDesc = Info->ReturnObject;

Cleanup:
    ACPI_FREE (Info);
    return_ACPI_STATUS (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtEvaluateNumericObject
 *
 * PARAMETERS:  ObjectName          - Object name to be evaluated
 *              DeviceNode          - Node for the device
 *              Value               - Where the value is returned
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Evaluates a numeric namespace object for a selected device
 *              and stores result in *Value.
 *
 *              NOTE: Internal function, no parameter validation
 *
 ******************************************************************************/

ACPI_STATUS
AcpiUtEvaluateNumericObject (
    const char              *ObjectName,
    ACPI_NAMESPACE_NODE     *DeviceNode,
    UINT64                  *Value)
{
    ACPI_OPERAND_OBJECT     *ObjDesc;
    ACPI_STATUS             Status;


    ACPI_FUNCTION_TRACE (UtEvaluateNumericObject);


    Status = AcpiUtEvaluateObject (DeviceNode, ObjectName,
        ACPI_BTYPE_INTEGER, &ObjDesc);
    if (ACPI_FAILURE (Status))
    {
        return_ACPI_STATUS (Status);
    }

    /* Get the returned Integer */

    *Value = ObjDesc->Integer.Value;

    /* On exit, we must delete the return object */

    AcpiUtRemoveReference (ObjDesc);
    return_ACPI_STATUS (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtExecute_STA
 *
 * PARAMETERS:  DeviceNode          - Node for the device
 *              Flags               - Where the status flags are returned
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Executes _STA for selected device and stores results in
 *              *Flags. If _STA does not exist, then the device is assumed
 *              to be present/functional/enabled (as per the ACPI spec).
 *
 *              NOTE: Internal function, no parameter validation
 *
 ******************************************************************************/

ACPI_STATUS
AcpiUtExecute_STA (
    ACPI_NAMESPACE_NODE     *DeviceNode,
    UINT32                  *Flags)
{
    ACPI_OPERAND_OBJECT     *ObjDesc;
    ACPI_STATUS             Status;


    ACPI_FUNCTION_TRACE (UtExecute_STA);


    Status = AcpiUtEvaluateObject (DeviceNode, METHOD_NAME__STA,
        ACPI_BTYPE_INTEGER, &ObjDesc);
    if (ACPI_FAILURE (Status))
    {
        if (AE_NOT_FOUND == Status)
        {
            /*
             * if _STA does not exist, then (as per the ACPI specification),
             * the returned flags will indicate that the device is present,
             * functional, and enabled.
             */
            ACPI_DEBUG_PRINT ((ACPI_DB_EXEC,
                "_STA on %4.4s was not found, assuming device is present\n",
                AcpiUtGetNodeName (DeviceNode)));

            *Flags = ACPI_UINT32_MAX;
            Status = AE_OK;
        }

        return_ACPI_STATUS (Status);
    }

    /* Extract the status flags */

    *Flags = (UINT32) ObjDesc->Integer.Value;

    /* On exit, we must delete the return object */

    AcpiUtRemoveReference (ObjDesc);
    return_ACPI_STATUS (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtExecutePowerMethods
 *
 * PARAMETERS:  DeviceNode          - Node for the device
 *              MethodNames         - Array of power method names
 *              MethodCount         - Number of methods to execute
 *              OutValues           - Where the power method values are returned
 *
 * RETURN:      Status, OutValues
 *
 * DESCRIPTION: Executes the specified power methods for the device and returns
 *              the result(s).
 *
 *              NOTE: Internal function, no parameter validation
 *
 ******************************************************************************/

ACPI_STATUS
AcpiUtExecutePowerMethods (
    ACPI_NAMESPACE_NODE     *DeviceNode,
    const char              **MethodNames,
    UINT8                   MethodCount,
    UINT8                   *OutValues)
{
    ACPI_OPERAND_OBJECT     *ObjDesc;
    ACPI_STATUS             Status;
    ACPI_STATUS             FinalStatus = AE_NOT_FOUND;
    UINT32                  i;


    ACPI_FUNCTION_TRACE (UtExecutePowerMethods);


    for (i = 0; i < MethodCount; i++)
    {
        /*
         * Execute the power method (_SxD or _SxW). The only allowable
         * return type is an Integer.
         */
        Status = AcpiUtEvaluateObject (DeviceNode,
            ACPI_CAST_PTR (char, MethodNames[i]),
            ACPI_BTYPE_INTEGER, &ObjDesc);
        if (ACPI_SUCCESS (Status))
        {
            OutValues[i] = (UINT8) ObjDesc->Integer.Value;

            /* Delete the return object */

            AcpiUtRemoveReference (ObjDesc);
            FinalStatus = AE_OK;            /* At least one value is valid */
            continue;
        }

        OutValues[i] = ACPI_UINT8_MAX;
        if (Status == AE_NOT_FOUND)
        {
            continue; /* Ignore if not found */
        }

        ACPI_DEBUG_PRINT ((ACPI_DB_EXEC, "Failed %s on Device %4.4s, %s\n",
            ACPI_CAST_PTR (char, MethodNames[i]),
            AcpiUtGetNodeName (DeviceNode), AcpiFormatException (Status)));
    }

    return_ACPI_STATUS (FinalStatus);
}
