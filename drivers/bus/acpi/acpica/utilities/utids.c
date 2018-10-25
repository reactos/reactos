/******************************************************************************
 *
 * Module Name: utids - support for device IDs - HID, UID, CID, SUB, CLS
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

#include "acpi.h"
#include "accommon.h"
#include "acinterp.h"


#define _COMPONENT          ACPI_UTILITIES
        ACPI_MODULE_NAME    ("utids")


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtExecute_HID
 *
 * PARAMETERS:  DeviceNode          - Node for the device
 *              ReturnId            - Where the string HID is returned
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Executes the _HID control method that returns the hardware
 *              ID of the device. The HID is either an 32-bit encoded EISAID
 *              Integer or a String. A string is always returned. An EISAID
 *              is converted to a string.
 *
 *              NOTE: Internal function, no parameter validation
 *
 ******************************************************************************/

ACPI_STATUS
AcpiUtExecute_HID (
    ACPI_NAMESPACE_NODE     *DeviceNode,
    ACPI_PNP_DEVICE_ID      **ReturnId)
{
    ACPI_OPERAND_OBJECT     *ObjDesc;
    ACPI_PNP_DEVICE_ID      *Hid;
    UINT32                  Length;
    ACPI_STATUS             Status;


    ACPI_FUNCTION_TRACE (UtExecute_HID);


    Status = AcpiUtEvaluateObject (DeviceNode, METHOD_NAME__HID,
        ACPI_BTYPE_INTEGER | ACPI_BTYPE_STRING, &ObjDesc);
    if (ACPI_FAILURE (Status))
    {
        return_ACPI_STATUS (Status);
    }

    /* Get the size of the String to be returned, includes null terminator */

    if (ObjDesc->Common.Type == ACPI_TYPE_INTEGER)
    {
        Length = ACPI_EISAID_STRING_SIZE;
    }
    else
    {
        Length = ObjDesc->String.Length + 1;
    }

    /* Allocate a buffer for the HID */

    Hid = ACPI_ALLOCATE_ZEROED (
        sizeof (ACPI_PNP_DEVICE_ID) + (ACPI_SIZE) Length);
    if (!Hid)
    {
        Status = AE_NO_MEMORY;
        goto Cleanup;
    }

    /* Area for the string starts after PNP_DEVICE_ID struct */

    Hid->String = ACPI_ADD_PTR (char, Hid, sizeof (ACPI_PNP_DEVICE_ID));

    /* Convert EISAID to a string or simply copy existing string */

    if (ObjDesc->Common.Type == ACPI_TYPE_INTEGER)
    {
        AcpiExEisaIdToString (Hid->String, ObjDesc->Integer.Value);
    }
    else
    {
        strcpy (Hid->String, ObjDesc->String.Pointer);
    }

    Hid->Length = Length;
    *ReturnId = Hid;


Cleanup:

    /* On exit, we must delete the return object */

    AcpiUtRemoveReference (ObjDesc);
    return_ACPI_STATUS (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtExecute_UID
 *
 * PARAMETERS:  DeviceNode          - Node for the device
 *              ReturnId            - Where the string UID is returned
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Executes the _UID control method that returns the unique
 *              ID of the device. The UID is either a 64-bit Integer (NOT an
 *              EISAID) or a string. Always returns a string. A 64-bit integer
 *              is converted to a decimal string.
 *
 *              NOTE: Internal function, no parameter validation
 *
 ******************************************************************************/

ACPI_STATUS
AcpiUtExecute_UID (
    ACPI_NAMESPACE_NODE     *DeviceNode,
    ACPI_PNP_DEVICE_ID      **ReturnId)
{
    ACPI_OPERAND_OBJECT     *ObjDesc;
    ACPI_PNP_DEVICE_ID      *Uid;
    UINT32                  Length;
    ACPI_STATUS             Status;


    ACPI_FUNCTION_TRACE (UtExecute_UID);


    Status = AcpiUtEvaluateObject (DeviceNode, METHOD_NAME__UID,
        ACPI_BTYPE_INTEGER | ACPI_BTYPE_STRING, &ObjDesc);
    if (ACPI_FAILURE (Status))
    {
        return_ACPI_STATUS (Status);
    }

    /* Get the size of the String to be returned, includes null terminator */

    if (ObjDesc->Common.Type == ACPI_TYPE_INTEGER)
    {
        Length = ACPI_MAX64_DECIMAL_DIGITS + 1;
    }
    else
    {
        Length = ObjDesc->String.Length + 1;
    }

    /* Allocate a buffer for the UID */

    Uid = ACPI_ALLOCATE_ZEROED (
        sizeof (ACPI_PNP_DEVICE_ID) + (ACPI_SIZE) Length);
    if (!Uid)
    {
        Status = AE_NO_MEMORY;
        goto Cleanup;
    }

    /* Area for the string starts after PNP_DEVICE_ID struct */

    Uid->String = ACPI_ADD_PTR (char, Uid, sizeof (ACPI_PNP_DEVICE_ID));

    /* Convert an Integer to string, or just copy an existing string */

    if (ObjDesc->Common.Type == ACPI_TYPE_INTEGER)
    {
        AcpiExIntegerToString (Uid->String, ObjDesc->Integer.Value);
    }
    else
    {
        strcpy (Uid->String, ObjDesc->String.Pointer);
    }

    Uid->Length = Length;
    *ReturnId = Uid;


Cleanup:

    /* On exit, we must delete the return object */

    AcpiUtRemoveReference (ObjDesc);
    return_ACPI_STATUS (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtExecute_CID
 *
 * PARAMETERS:  DeviceNode          - Node for the device
 *              ReturnCidList       - Where the CID list is returned
 *
 * RETURN:      Status, list of CID strings
 *
 * DESCRIPTION: Executes the _CID control method that returns one or more
 *              compatible hardware IDs for the device.
 *
 *              NOTE: Internal function, no parameter validation
 *
 * A _CID method can return either a single compatible ID or a package of
 * compatible IDs. Each compatible ID can be one of the following:
 * 1) Integer (32 bit compressed EISA ID) or
 * 2) String (PCI ID format, e.g. "PCI\VEN_vvvv&DEV_dddd&SUBSYS_ssssssss")
 *
 * The Integer CIDs are converted to string format by this function.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiUtExecute_CID (
    ACPI_NAMESPACE_NODE     *DeviceNode,
    ACPI_PNP_DEVICE_ID_LIST **ReturnCidList)
{
    ACPI_OPERAND_OBJECT     **CidObjects;
    ACPI_OPERAND_OBJECT     *ObjDesc;
    ACPI_PNP_DEVICE_ID_LIST *CidList;
    char                    *NextIdString;
    UINT32                  StringAreaSize;
    UINT32                  Length;
    UINT32                  CidListSize;
    ACPI_STATUS             Status;
    UINT32                  Count;
    UINT32                  i;


    ACPI_FUNCTION_TRACE (UtExecute_CID);


    /* Evaluate the _CID method for this device */

    Status = AcpiUtEvaluateObject (DeviceNode, METHOD_NAME__CID,
        ACPI_BTYPE_INTEGER | ACPI_BTYPE_STRING | ACPI_BTYPE_PACKAGE,
        &ObjDesc);
    if (ACPI_FAILURE (Status))
    {
        return_ACPI_STATUS (Status);
    }

    /*
     * Get the count and size of the returned _CIDs. _CID can return either
     * a Package of Integers/Strings or a single Integer or String.
     * Note: This section also validates that all CID elements are of the
     * correct type (Integer or String).
     */
    if (ObjDesc->Common.Type == ACPI_TYPE_PACKAGE)
    {
        Count = ObjDesc->Package.Count;
        CidObjects = ObjDesc->Package.Elements;
    }
    else /* Single Integer or String CID */
    {
        Count = 1;
        CidObjects = &ObjDesc;
    }

    StringAreaSize = 0;
    for (i = 0; i < Count; i++)
    {
        /* String lengths include null terminator */

        switch (CidObjects[i]->Common.Type)
        {
        case ACPI_TYPE_INTEGER:

            StringAreaSize += ACPI_EISAID_STRING_SIZE;
            break;

        case ACPI_TYPE_STRING:

            StringAreaSize += CidObjects[i]->String.Length + 1;
            break;

        default:

            Status = AE_TYPE;
            goto Cleanup;
        }
    }

    /*
     * Now that we know the length of the CIDs, allocate return buffer:
     * 1) Size of the base structure +
     * 2) Size of the CID PNP_DEVICE_ID array +
     * 3) Size of the actual CID strings
     */
    CidListSize = sizeof (ACPI_PNP_DEVICE_ID_LIST) +
        ((Count - 1) * sizeof (ACPI_PNP_DEVICE_ID)) +
        StringAreaSize;

    CidList = ACPI_ALLOCATE_ZEROED (CidListSize);
    if (!CidList)
    {
        Status = AE_NO_MEMORY;
        goto Cleanup;
    }

    /* Area for CID strings starts after the CID PNP_DEVICE_ID array */

    NextIdString = ACPI_CAST_PTR (char, CidList->Ids) +
        ((ACPI_SIZE) Count * sizeof (ACPI_PNP_DEVICE_ID));

    /* Copy/convert the CIDs to the return buffer */

    for (i = 0; i < Count; i++)
    {
        if (CidObjects[i]->Common.Type == ACPI_TYPE_INTEGER)
        {
            /* Convert the Integer (EISAID) CID to a string */

            AcpiExEisaIdToString (
                NextIdString, CidObjects[i]->Integer.Value);
            Length = ACPI_EISAID_STRING_SIZE;
        }
        else /* ACPI_TYPE_STRING */
        {
            /* Copy the String CID from the returned object */

            strcpy (NextIdString, CidObjects[i]->String.Pointer);
            Length = CidObjects[i]->String.Length + 1;
        }

        CidList->Ids[i].String = NextIdString;
        CidList->Ids[i].Length = Length;
        NextIdString += Length;
    }

    /* Finish the CID list */

    CidList->Count = Count;
    CidList->ListSize = CidListSize;
    *ReturnCidList = CidList;


Cleanup:

    /* On exit, we must delete the _CID return object */

    AcpiUtRemoveReference (ObjDesc);
    return_ACPI_STATUS (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtExecute_CLS
 *
 * PARAMETERS:  DeviceNode          - Node for the device
 *              ReturnId            - Where the _CLS is returned
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Executes the _CLS control method that returns PCI-defined
 *              class code of the device. The _CLS value is always a package
 *              containing PCI class information as a list of integers.
 *              The returned string has format "BBSSPP", where:
 *                BB = Base-class code
 *                SS = Sub-class code
 *                PP = Programming Interface code
 *
 ******************************************************************************/

ACPI_STATUS
AcpiUtExecute_CLS (
    ACPI_NAMESPACE_NODE     *DeviceNode,
    ACPI_PNP_DEVICE_ID      **ReturnId)
{
    ACPI_OPERAND_OBJECT     *ObjDesc;
    ACPI_OPERAND_OBJECT     **ClsObjects;
    UINT32                  Count;
    ACPI_PNP_DEVICE_ID      *Cls;
    UINT32                  Length;
    ACPI_STATUS             Status;
    UINT8                   ClassCode[3] = {0, 0, 0};


    ACPI_FUNCTION_TRACE (UtExecute_CLS);


    Status = AcpiUtEvaluateObject (DeviceNode, METHOD_NAME__CLS,
        ACPI_BTYPE_PACKAGE, &ObjDesc);
    if (ACPI_FAILURE (Status))
    {
        return_ACPI_STATUS (Status);
    }

    /* Get the size of the String to be returned, includes null terminator */

    Length = ACPI_PCICLS_STRING_SIZE;
    ClsObjects = ObjDesc->Package.Elements;
    Count = ObjDesc->Package.Count;

    if (ObjDesc->Common.Type == ACPI_TYPE_PACKAGE)
    {
        if (Count > 0 && ClsObjects[0]->Common.Type == ACPI_TYPE_INTEGER)
        {
            ClassCode[0] = (UINT8) ClsObjects[0]->Integer.Value;
        }
        if (Count > 1 && ClsObjects[1]->Common.Type == ACPI_TYPE_INTEGER)
        {
            ClassCode[1] = (UINT8) ClsObjects[1]->Integer.Value;
        }
        if (Count > 2 && ClsObjects[2]->Common.Type == ACPI_TYPE_INTEGER)
        {
            ClassCode[2] = (UINT8) ClsObjects[2]->Integer.Value;
        }
    }

    /* Allocate a buffer for the CLS */

    Cls = ACPI_ALLOCATE_ZEROED (
        sizeof (ACPI_PNP_DEVICE_ID) + (ACPI_SIZE) Length);
    if (!Cls)
    {
        Status = AE_NO_MEMORY;
        goto Cleanup;
    }

    /* Area for the string starts after PNP_DEVICE_ID struct */

    Cls->String = ACPI_ADD_PTR (char, Cls, sizeof (ACPI_PNP_DEVICE_ID));

    /* Simply copy existing string */

    AcpiExPciClsToString (Cls->String, ClassCode);
    Cls->Length = Length;
    *ReturnId = Cls;


Cleanup:

    /* On exit, we must delete the return object */

    AcpiUtRemoveReference (ObjDesc);
    return_ACPI_STATUS (Status);
}
