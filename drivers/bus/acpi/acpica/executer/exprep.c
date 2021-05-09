/******************************************************************************
 *
 * Module Name: exprep - ACPI AML field prep utilities
 *
 *****************************************************************************/

/*
 * Copyright (C) 2000 - 2020, Intel Corp.
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
#include "amlcode.h"
#include "acnamesp.h"
#include "acdispat.h"


#define _COMPONENT          ACPI_EXECUTER
        ACPI_MODULE_NAME    ("exprep")

/* Local prototypes */

static UINT32
AcpiExDecodeFieldAccess (
    ACPI_OPERAND_OBJECT     *ObjDesc,
    UINT8                   FieldFlags,
    UINT32                  *ReturnByteAlignment);


#ifdef ACPI_UNDER_DEVELOPMENT

static UINT32
AcpiExGenerateAccess (
    UINT32                  FieldBitOffset,
    UINT32                  FieldBitLength,
    UINT32                  RegionLength);


/*******************************************************************************
 *
 * FUNCTION:    AcpiExGenerateAccess
 *
 * PARAMETERS:  FieldBitOffset      - Start of field within parent region/buffer
 *              FieldBitLength      - Length of field in bits
 *              RegionLength        - Length of parent in bytes
 *
 * RETURN:      Field granularity (8, 16, 32 or 64) and
 *              ByteAlignment (1, 2, 3, or 4)
 *
 * DESCRIPTION: Generate an optimal access width for fields defined with the
 *              AnyAcc keyword.
 *
 * NOTE: Need to have the RegionLength in order to check for boundary
 *       conditions (end-of-region). However, the RegionLength is a deferred
 *       operation. Therefore, to complete this implementation, the generation
 *       of this access width must be deferred until the region length has
 *       been evaluated.
 *
 ******************************************************************************/

static UINT32
AcpiExGenerateAccess (
    UINT32                  FieldBitOffset,
    UINT32                  FieldBitLength,
    UINT32                  RegionLength)
{
    UINT32                  FieldByteLength;
    UINT32                  FieldByteOffset;
    UINT32                  FieldByteEndOffset;
    UINT32                  AccessByteWidth;
    UINT32                  FieldStartOffset;
    UINT32                  FieldEndOffset;
    UINT32                  MinimumAccessWidth = 0xFFFFFFFF;
    UINT32                  MinimumAccesses = 0xFFFFFFFF;
    UINT32                  Accesses;


    ACPI_FUNCTION_TRACE (ExGenerateAccess);


    /* Round Field start offset and length to "minimal" byte boundaries */

    FieldByteOffset = ACPI_DIV_8 (
        ACPI_ROUND_DOWN (FieldBitOffset, 8));

    FieldByteEndOffset = ACPI_DIV_8 (
        ACPI_ROUND_UP (FieldBitLength + FieldBitOffset, 8));

    FieldByteLength = FieldByteEndOffset - FieldByteOffset;

    ACPI_DEBUG_PRINT ((ACPI_DB_BFIELD,
        "Bit length %u, Bit offset %u\n",
        FieldBitLength, FieldBitOffset));

    ACPI_DEBUG_PRINT ((ACPI_DB_BFIELD,
        "Byte Length %u, Byte Offset %u, End Offset %u\n",
        FieldByteLength, FieldByteOffset, FieldByteEndOffset));

    /*
     * Iterative search for the maximum access width that is both aligned
     * and does not go beyond the end of the region
     *
     * Start at ByteAcc and work upwards to QwordAcc max. (1,2,4,8 bytes)
     */
    for (AccessByteWidth = 1; AccessByteWidth <= 8; AccessByteWidth <<= 1)
    {
        /*
         * 1) Round end offset up to next access boundary and make sure that
         *    this does not go beyond the end of the parent region.
         * 2) When the Access width is greater than the FieldByteLength, we
         *    are done. (This does not optimize for the perfectly aligned
         *    case yet).
         */
        if (ACPI_ROUND_UP (FieldByteEndOffset, AccessByteWidth) <=
            RegionLength)
        {
            FieldStartOffset =
                ACPI_ROUND_DOWN (FieldByteOffset, AccessByteWidth) /
                AccessByteWidth;

            FieldEndOffset =
                ACPI_ROUND_UP ((FieldByteLength + FieldByteOffset),
                    AccessByteWidth) / AccessByteWidth;

            Accesses = FieldEndOffset - FieldStartOffset;

            ACPI_DEBUG_PRINT ((ACPI_DB_BFIELD,
                "AccessWidth %u end is within region\n", AccessByteWidth));

            ACPI_DEBUG_PRINT ((ACPI_DB_BFIELD,
                "Field Start %u, Field End %u -- requires %u accesses\n",
                FieldStartOffset, FieldEndOffset, Accesses));

            /* Single access is optimal */

            if (Accesses <= 1)
            {
                ACPI_DEBUG_PRINT ((ACPI_DB_BFIELD,
                    "Entire field can be accessed "
                    "with one operation of size %u\n",
                    AccessByteWidth));
                return_VALUE (AccessByteWidth);
            }

            /*
             * Fits in the region, but requires more than one read/write.
             * try the next wider access on next iteration
             */
            if (Accesses < MinimumAccesses)
            {
                MinimumAccesses = Accesses;
                MinimumAccessWidth = AccessByteWidth;
            }
        }
        else
        {
            ACPI_DEBUG_PRINT ((ACPI_DB_BFIELD,
                "AccessWidth %u end is NOT within region\n",
                AccessByteWidth));
            if (AccessByteWidth == 1)
            {
                ACPI_DEBUG_PRINT ((ACPI_DB_BFIELD,
                    "Field goes beyond end-of-region!\n"));

                /* Field does not fit in the region at all */

                return_VALUE (0);
            }

            /*
             * This width goes beyond the end-of-region, back off to
             * previous access
             */
            ACPI_DEBUG_PRINT ((ACPI_DB_BFIELD,
                "Backing off to previous optimal access width of %u\n",
                MinimumAccessWidth));
            return_VALUE (MinimumAccessWidth);
        }
    }

    /*
     * Could not read/write field with one operation,
     * just use max access width
     */
    ACPI_DEBUG_PRINT ((ACPI_DB_BFIELD,
        "Cannot access field in one operation, using width 8\n"));

    return_VALUE (8);
}
#endif /* ACPI_UNDER_DEVELOPMENT */


/*******************************************************************************
 *
 * FUNCTION:    AcpiExDecodeFieldAccess
 *
 * PARAMETERS:  ObjDesc             - Field object
 *              FieldFlags          - Encoded fieldflags (contains access bits)
 *              ReturnByteAlignment - Where the byte alignment is returned
 *
 * RETURN:      Field granularity (8, 16, 32 or 64) and
 *              ByteAlignment (1, 2, 3, or 4)
 *
 * DESCRIPTION: Decode the AccessType bits of a field definition.
 *
 ******************************************************************************/

static UINT32
AcpiExDecodeFieldAccess (
    ACPI_OPERAND_OBJECT     *ObjDesc,
    UINT8                   FieldFlags,
    UINT32                  *ReturnByteAlignment)
{
    UINT32                  Access;
    UINT32                  ByteAlignment;
    UINT32                  BitLength;


    ACPI_FUNCTION_TRACE (ExDecodeFieldAccess);


    Access = (FieldFlags & AML_FIELD_ACCESS_TYPE_MASK);

    switch (Access)
    {
    case AML_FIELD_ACCESS_ANY:

#ifdef ACPI_UNDER_DEVELOPMENT
        ByteAlignment =
            AcpiExGenerateAccess (ObjDesc->CommonField.StartFieldBitOffset,
                ObjDesc->CommonField.BitLength,
                0xFFFFFFFF /* Temp until we pass RegionLength as parameter */);
        BitLength = ByteAlignment * 8;
#endif

        ByteAlignment = 1;
        BitLength = 8;
        break;

    case AML_FIELD_ACCESS_BYTE:
    case AML_FIELD_ACCESS_BUFFER:   /* ACPI 2.0 (SMBus Buffer) */

        ByteAlignment = 1;
        BitLength     = 8;
        break;

    case AML_FIELD_ACCESS_WORD:

        ByteAlignment = 2;
        BitLength     = 16;
        break;

    case AML_FIELD_ACCESS_DWORD:

        ByteAlignment = 4;
        BitLength     = 32;
        break;

    case AML_FIELD_ACCESS_QWORD:    /* ACPI 2.0 */

        ByteAlignment = 8;
        BitLength     = 64;
        break;

    default:

        /* Invalid field access type */

        ACPI_ERROR ((AE_INFO,
            "Unknown field access type 0x%X",
            Access));

        return_UINT32 (0);
    }

    if (ObjDesc->Common.Type == ACPI_TYPE_BUFFER_FIELD)
    {
        /*
         * BufferField access can be on any byte boundary, so the
         * ByteAlignment is always 1 byte -- regardless of any ByteAlignment
         * implied by the field access type.
         */
        ByteAlignment = 1;
    }

    *ReturnByteAlignment = ByteAlignment;
    return_UINT32 (BitLength);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiExPrepCommonFieldObject
 *
 * PARAMETERS:  ObjDesc             - The field object
 *              FieldFlags          - Access, LockRule, and UpdateRule.
 *                                    The format of a FieldFlag is described
 *                                    in the ACPI specification
 *              FieldAttribute      - Special attributes (not used)
 *              FieldBitPosition    - Field start position
 *              FieldBitLength      - Field length in number of bits
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Initialize the areas of the field object that are common
 *              to the various types of fields. Note: This is very "sensitive"
 *              code because we are solving the general case for field
 *              alignment.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiExPrepCommonFieldObject (
    ACPI_OPERAND_OBJECT     *ObjDesc,
    UINT8                   FieldFlags,
    UINT8                   FieldAttribute,
    UINT32                  FieldBitPosition,
    UINT32                  FieldBitLength)
{
    UINT32                  AccessBitWidth;
    UINT32                  ByteAlignment;
    UINT32                  NearestByteAddress;


    ACPI_FUNCTION_TRACE (ExPrepCommonFieldObject);


    /*
     * Note: the structure being initialized is the
     * ACPI_COMMON_FIELD_INFO;  No structure fields outside of the common
     * area are initialized by this procedure.
     */
    ObjDesc->CommonField.FieldFlags = FieldFlags;
    ObjDesc->CommonField.Attribute  = FieldAttribute;
    ObjDesc->CommonField.BitLength  = FieldBitLength;

    /*
     * Decode the access type so we can compute offsets. The access type gives
     * two pieces of information - the width of each field access and the
     * necessary ByteAlignment (address granularity) of the access.
     *
     * For AnyAcc, the AccessBitWidth is the largest width that is both
     * necessary and possible in an attempt to access the whole field in one
     * I/O operation. However, for AnyAcc, the ByteAlignment is always one
     * byte.
     *
     * For all Buffer Fields, the ByteAlignment is always one byte.
     *
     * For all other access types (Byte, Word, Dword, Qword), the Bitwidth is
     * the same (equivalent) as the ByteAlignment.
     */
    AccessBitWidth = AcpiExDecodeFieldAccess (
        ObjDesc, FieldFlags, &ByteAlignment);
    if (!AccessBitWidth)
    {
        return_ACPI_STATUS (AE_AML_OPERAND_VALUE);
    }

    /* Setup width (access granularity) fields (values are: 1, 2, 4, 8) */

    ObjDesc->CommonField.AccessByteWidth = (UINT8)
        ACPI_DIV_8 (AccessBitWidth);

    /*
     * BaseByteOffset is the address of the start of the field within the
     * region. It is the byte address of the first *datum* (field-width data
     * unit) of the field. (i.e., the first datum that contains at least the
     * first *bit* of the field.)
     *
     * Note: ByteAlignment is always either equal to the AccessBitWidth or 8
     * (Byte access), and it defines the addressing granularity of the parent
     * region or buffer.
     */
    NearestByteAddress =
        ACPI_ROUND_BITS_DOWN_TO_BYTES (FieldBitPosition);
    ObjDesc->CommonField.BaseByteOffset = (UINT32)
        ACPI_ROUND_DOWN (NearestByteAddress, ByteAlignment);

    /*
     * StartFieldBitOffset is the offset of the first bit of the field within
     * a field datum.
     */
    ObjDesc->CommonField.StartFieldBitOffset = (UINT8)
        (FieldBitPosition - ACPI_MUL_8 (ObjDesc->CommonField.BaseByteOffset));

    return_ACPI_STATUS (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiExPrepFieldValue
 *
 * PARAMETERS:  Info    - Contains all field creation info
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Construct an object of type ACPI_OPERAND_OBJECT with a
 *              subtype of DefField and connect it to the parent Node.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiExPrepFieldValue (
    ACPI_CREATE_FIELD_INFO  *Info)
{
    ACPI_OPERAND_OBJECT     *ObjDesc;
    ACPI_OPERAND_OBJECT     *SecondDesc = NULL;
    ACPI_STATUS             Status;
    UINT32                  AccessByteWidth;
    UINT32                  Type;


    ACPI_FUNCTION_TRACE (ExPrepFieldValue);


    /* Parameter validation */

    if (Info->FieldType != ACPI_TYPE_LOCAL_INDEX_FIELD)
    {
        if (!Info->RegionNode)
        {
            ACPI_ERROR ((AE_INFO, "Null RegionNode"));
            return_ACPI_STATUS (AE_AML_NO_OPERAND);
        }

        Type = AcpiNsGetType (Info->RegionNode);
        if (Type != ACPI_TYPE_REGION)
        {
            ACPI_ERROR ((AE_INFO, "Needed Region, found type 0x%X (%s)",
                Type, AcpiUtGetTypeName (Type)));

            return_ACPI_STATUS (AE_AML_OPERAND_TYPE);
        }
    }

    /* Allocate a new field object */

    ObjDesc = AcpiUtCreateInternalObject (Info->FieldType);
    if (!ObjDesc)
    {
        return_ACPI_STATUS (AE_NO_MEMORY);
    }

    /* Initialize areas of the object that are common to all fields */

    ObjDesc->CommonField.Node = Info->FieldNode;
    Status = AcpiExPrepCommonFieldObject (ObjDesc,
        Info->FieldFlags, Info->Attribute,
        Info->FieldBitPosition, Info->FieldBitLength);
    if (ACPI_FAILURE (Status))
    {
        AcpiUtDeleteObjectDesc (ObjDesc);
        return_ACPI_STATUS (Status);
    }

    /* Initialize areas of the object that are specific to the field type */

    switch (Info->FieldType)
    {
    case ACPI_TYPE_LOCAL_REGION_FIELD:

        ObjDesc->Field.RegionObj = AcpiNsGetAttachedObject (Info->RegionNode);

        /* Fields specific to GenericSerialBus fields */

        ObjDesc->Field.AccessLength = Info->AccessLength;

        if (Info->ConnectionNode)
        {
            SecondDesc = Info->ConnectionNode->Object;
            if (!(SecondDesc->Common.Flags & AOPOBJ_DATA_VALID))
            {
                Status = AcpiDsGetBufferArguments (SecondDesc);
                if (ACPI_FAILURE (Status))
                {
                    AcpiUtDeleteObjectDesc (ObjDesc);
                    return_ACPI_STATUS (Status);
                }
            }

            ObjDesc->Field.ResourceBuffer =
                SecondDesc->Buffer.Pointer;
            ObjDesc->Field.ResourceLength =
                (UINT16) SecondDesc->Buffer.Length;
        }
        else if (Info->ResourceBuffer)
        {
            ObjDesc->Field.ResourceBuffer = Info->ResourceBuffer;
            ObjDesc->Field.ResourceLength = Info->ResourceLength;
        }

        ObjDesc->Field.PinNumberIndex = Info->PinNumberIndex;

        /* Allow full data read from EC address space */

        if ((ObjDesc->Field.RegionObj->Region.SpaceId == ACPI_ADR_SPACE_EC) &&
            (ObjDesc->CommonField.BitLength > 8))
        {
            AccessByteWidth = ACPI_ROUND_BITS_UP_TO_BYTES (
                ObjDesc->CommonField.BitLength);

            /* Maximum byte width supported is 255 */

            if (AccessByteWidth < 256)
            {
                ObjDesc->CommonField.AccessByteWidth =
                    (UINT8) AccessByteWidth;
            }
        }

        ACPI_DEBUG_PRINT ((ACPI_DB_BFIELD,
            "RegionField: BitOff %X, Off %X, Gran %X, Region %p\n",
            ObjDesc->Field.StartFieldBitOffset,
            ObjDesc->Field.BaseByteOffset,
            ObjDesc->Field.AccessByteWidth,
            ObjDesc->Field.RegionObj));
        break;

    case ACPI_TYPE_LOCAL_BANK_FIELD:

        ObjDesc->BankField.Value = Info->BankValue;
        ObjDesc->BankField.RegionObj =
            AcpiNsGetAttachedObject (Info->RegionNode);
        ObjDesc->BankField.BankObj =
            AcpiNsGetAttachedObject (Info->RegisterNode);

        /* An additional reference for the attached objects */

        AcpiUtAddReference (ObjDesc->BankField.RegionObj);
        AcpiUtAddReference (ObjDesc->BankField.BankObj);

        ACPI_DEBUG_PRINT ((ACPI_DB_BFIELD,
            "Bank Field: BitOff %X, Off %X, Gran %X, Region %p, BankReg %p\n",
            ObjDesc->BankField.StartFieldBitOffset,
            ObjDesc->BankField.BaseByteOffset,
            ObjDesc->Field.AccessByteWidth,
            ObjDesc->BankField.RegionObj,
            ObjDesc->BankField.BankObj));

        /*
         * Remember location in AML stream of the field unit
         * opcode and operands -- since the BankValue
         * operands must be evaluated.
         */
        SecondDesc = ObjDesc->Common.NextObject;
        SecondDesc->Extra.AmlStart = ACPI_CAST_PTR (ACPI_PARSE_OBJECT,
            Info->DataRegisterNode)->Named.Data;
        SecondDesc->Extra.AmlLength = ACPI_CAST_PTR (ACPI_PARSE_OBJECT,
            Info->DataRegisterNode)->Named.Length;

        break;

    case ACPI_TYPE_LOCAL_INDEX_FIELD:

        /* Get the Index and Data registers */

        ObjDesc->IndexField.IndexObj =
            AcpiNsGetAttachedObject (Info->RegisterNode);
        ObjDesc->IndexField.DataObj =
            AcpiNsGetAttachedObject (Info->DataRegisterNode);

        if (!ObjDesc->IndexField.DataObj || !ObjDesc->IndexField.IndexObj)
        {
            ACPI_ERROR ((AE_INFO, "Null Index Object during field prep"));
            AcpiUtDeleteObjectDesc (ObjDesc);
            return_ACPI_STATUS (AE_AML_INTERNAL);
        }

        /* An additional reference for the attached objects */

        AcpiUtAddReference (ObjDesc->IndexField.DataObj);
        AcpiUtAddReference (ObjDesc->IndexField.IndexObj);

        /*
         * April 2006: Changed to match MS behavior
         *
         * The value written to the Index register is the byte offset of the
         * target field in units of the granularity of the IndexField
         *
         * Previously, the value was calculated as an index in terms of the
         * width of the Data register, as below:
         *
         *      ObjDesc->IndexField.Value = (UINT32)
         *          (Info->FieldBitPosition / ACPI_MUL_8 (
         *              ObjDesc->Field.AccessByteWidth));
         *
         * February 2006: Tried value as a byte offset:
         *      ObjDesc->IndexField.Value = (UINT32)
         *          ACPI_DIV_8 (Info->FieldBitPosition);
         */
        ObjDesc->IndexField.Value = (UINT32) ACPI_ROUND_DOWN (
            ACPI_DIV_8 (Info->FieldBitPosition),
            ObjDesc->IndexField.AccessByteWidth);

        ACPI_DEBUG_PRINT ((ACPI_DB_BFIELD,
            "IndexField: BitOff %X, Off %X, Value %X, "
            "Gran %X, Index %p, Data %p\n",
            ObjDesc->IndexField.StartFieldBitOffset,
            ObjDesc->IndexField.BaseByteOffset,
            ObjDesc->IndexField.Value,
            ObjDesc->Field.AccessByteWidth,
            ObjDesc->IndexField.IndexObj,
            ObjDesc->IndexField.DataObj));
        break;

    default:

        /* No other types should get here */

        break;
    }

    /*
     * Store the constructed descriptor (ObjDesc) into the parent Node,
     * preserving the current type of that NamedObj.
     */
    Status = AcpiNsAttachObject (
        Info->FieldNode, ObjDesc, AcpiNsGetType (Info->FieldNode));

    ACPI_DEBUG_PRINT ((ACPI_DB_BFIELD,
        "Set NamedObj %p [%4.4s], ObjDesc %p\n",
        Info->FieldNode, AcpiUtGetNodeName (Info->FieldNode), ObjDesc));

    /* Remove local reference to the object */

    AcpiUtRemoveReference (ObjDesc);
    return_ACPI_STATUS (Status);
}
