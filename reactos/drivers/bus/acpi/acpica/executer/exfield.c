/******************************************************************************
 *
 * Module Name: exfield - ACPI AML (p-code) execution - field manipulation
 *
 *****************************************************************************/

/******************************************************************************
 *
 * 1. Copyright Notice
 *
 * Some or all of this work - Copyright (c) 1999 - 2015, Intel Corp.
 * All rights reserved.
 *
 * 2. License
 *
 * 2.1. This is your license from Intel Corp. under its intellectual property
 * rights. You may have additional license terms from the party that provided
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
 * to or modifications of the Original Intel Code. No other license or right
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
 * and the following Disclaimer and Export Compliance provision. In addition,
 * Licensee must cause all Covered Code to which Licensee contributes to
 * contain a file documenting the changes Licensee made to create that Covered
 * Code and the date of any change. Licensee must include in that file the
 * documentation of any changes made by any predecessor Licensee. Licensee
 * must include a prominent statement that the modification is derived,
 * directly or indirectly, from Original Intel Code.
 *
 * 3.2. Redistribution of Source with no Rights to Further Distribute Source.
 * Redistribution of source code of any substantial portion of the Covered
 * Code or modification without rights to further distribute source must
 * include the following Disclaimer and Export Compliance provision in the
 * documentation and/or other materials provided with distribution. In
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
 * HERE. ANY SOFTWARE ORIGINATING FROM INTEL OR DERIVED FROM INTEL SOFTWARE
 * IS PROVIDED "AS IS," AND INTEL WILL NOT PROVIDE ANY SUPPORT, ASSISTANCE,
 * INSTALLATION, TRAINING OR OTHER SERVICES. INTEL WILL NOT PROVIDE ANY
 * UPDATES, ENHANCEMENTS OR EXTENSIONS. INTEL SPECIFICALLY DISCLAIMS ANY
 * IMPLIED WARRANTIES OF MERCHANTABILITY, NONINFRINGEMENT AND FITNESS FOR A
 * PARTICULAR PURPOSE.
 *
 * 4.2. IN NO EVENT SHALL INTEL HAVE ANY LIABILITY TO LICENSEE, ITS LICENSEES
 * OR ANY OTHER THIRD PARTY, FOR ANY LOST PROFITS, LOST DATA, LOSS OF USE OR
 * COSTS OF PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES, OR FOR ANY INDIRECT,
 * SPECIAL OR CONSEQUENTIAL DAMAGES ARISING OUT OF THIS AGREEMENT, UNDER ANY
 * CAUSE OF ACTION OR THEORY OF LIABILITY, AND IRRESPECTIVE OF WHETHER INTEL
 * HAS ADVANCE NOTICE OF THE POSSIBILITY OF SUCH DAMAGES. THESE LIMITATIONS
 * SHALL APPLY NOTWITHSTANDING THE FAILURE OF THE ESSENTIAL PURPOSE OF ANY
 * LIMITED REMEDY.
 *
 * 4.3. Licensee shall not export, either directly or indirectly, any of this
 * software or system incorporating such software without first obtaining any
 * required license or other approval from the U. S. Department of Commerce or
 * any other agency or department of the United States Government. In the
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
#include "acdispat.h"
#include "acinterp.h"
#include "amlcode.h"


#define _COMPONENT          ACPI_EXECUTER
        ACPI_MODULE_NAME    ("exfield")

/* Local prototypes */

static UINT32
AcpiExGetSerialAccessLength (
    UINT32                  AccessorType,
    UINT32                  AccessLength);


/*******************************************************************************
 *
 * FUNCTION:    AcpiExGetSerialAccessLength
 *
 * PARAMETERS:  AccessorType    - The type of the protocol indicated by region
 *                                field access attributes
 *              AccessLength    - The access length of the region field
 *
 * RETURN:      Decoded access length
 *
 * DESCRIPTION: This routine returns the length of the GenericSerialBus
 *              protocol bytes
 *
 ******************************************************************************/

static UINT32
AcpiExGetSerialAccessLength (
    UINT32                  AccessorType,
    UINT32                  AccessLength)
{
    UINT32                  Length;


    switch (AccessorType)
    {
    case AML_FIELD_ATTRIB_QUICK:

        Length = 0;
        break;

    case AML_FIELD_ATTRIB_SEND_RCV:
    case AML_FIELD_ATTRIB_BYTE:

        Length = 1;
        break;

    case AML_FIELD_ATTRIB_WORD:
    case AML_FIELD_ATTRIB_WORD_CALL:

        Length = 2;
        break;

    case AML_FIELD_ATTRIB_MULTIBYTE:
    case AML_FIELD_ATTRIB_RAW_BYTES:
    case AML_FIELD_ATTRIB_RAW_PROCESS:

        Length = AccessLength;
        break;

    case AML_FIELD_ATTRIB_BLOCK:
    case AML_FIELD_ATTRIB_BLOCK_CALL:
    default:

        Length = ACPI_GSBUS_BUFFER_SIZE - 2;
        break;
    }

    return (Length);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiExReadDataFromField
 *
 * PARAMETERS:  WalkState           - Current execution state
 *              ObjDesc             - The named field
 *              RetBufferDesc       - Where the return data object is stored
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Read from a named field. Returns either an Integer or a
 *              Buffer, depending on the size of the field.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiExReadDataFromField (
    ACPI_WALK_STATE         *WalkState,
    ACPI_OPERAND_OBJECT     *ObjDesc,
    ACPI_OPERAND_OBJECT     **RetBufferDesc)
{
    ACPI_STATUS             Status;
    ACPI_OPERAND_OBJECT     *BufferDesc;
    ACPI_SIZE               Length;
    void                    *Buffer;
    UINT32                  Function;
    UINT16                  AccessorType;


    ACPI_FUNCTION_TRACE_PTR (ExReadDataFromField, ObjDesc);


    /* Parameter validation */

    if (!ObjDesc)
    {
        return_ACPI_STATUS (AE_AML_NO_OPERAND);
    }
    if (!RetBufferDesc)
    {
        return_ACPI_STATUS (AE_BAD_PARAMETER);
    }

    if (ObjDesc->Common.Type == ACPI_TYPE_BUFFER_FIELD)
    {
        /*
         * If the BufferField arguments have not been previously evaluated,
         * evaluate them now and save the results.
         */
        if (!(ObjDesc->Common.Flags & AOPOBJ_DATA_VALID))
        {
            Status = AcpiDsGetBufferFieldArguments (ObjDesc);
            if (ACPI_FAILURE (Status))
            {
                return_ACPI_STATUS (Status);
            }
        }
    }
    else if ((ObjDesc->Common.Type == ACPI_TYPE_LOCAL_REGION_FIELD) &&
             (ObjDesc->Field.RegionObj->Region.SpaceId == ACPI_ADR_SPACE_SMBUS ||
              ObjDesc->Field.RegionObj->Region.SpaceId == ACPI_ADR_SPACE_GSBUS ||
              ObjDesc->Field.RegionObj->Region.SpaceId == ACPI_ADR_SPACE_IPMI))
    {
        /*
         * This is an SMBus, GSBus or IPMI read. We must create a buffer to hold
         * the data and then directly access the region handler.
         *
         * Note: SMBus and GSBus protocol value is passed in upper 16-bits of Function
         */
        if (ObjDesc->Field.RegionObj->Region.SpaceId == ACPI_ADR_SPACE_SMBUS)
        {
            Length = ACPI_SMBUS_BUFFER_SIZE;
            Function = ACPI_READ | (ObjDesc->Field.Attribute << 16);
        }
        else if (ObjDesc->Field.RegionObj->Region.SpaceId == ACPI_ADR_SPACE_GSBUS)
        {
            AccessorType = ObjDesc->Field.Attribute;
            Length = AcpiExGetSerialAccessLength (AccessorType,
                ObjDesc->Field.AccessLength);

            /*
             * Add additional 2 bytes for the GenericSerialBus data buffer:
             *
             *     Status;      (Byte 0 of the data buffer)
             *     Length;      (Byte 1 of the data buffer)
             *     Data[x-1];   (Bytes 2-x of the arbitrary length data buffer)
             */
            Length += 2;
            Function = ACPI_READ | (AccessorType << 16);
        }
        else /* IPMI */
        {
            Length = ACPI_IPMI_BUFFER_SIZE;
            Function = ACPI_READ;
        }

        BufferDesc = AcpiUtCreateBufferObject (Length);
        if (!BufferDesc)
        {
            return_ACPI_STATUS (AE_NO_MEMORY);
        }

        /* Lock entire transaction if requested */

        AcpiExAcquireGlobalLock (ObjDesc->CommonField.FieldFlags);

        /* Call the region handler for the read */

        Status = AcpiExAccessRegion (ObjDesc, 0,
                    ACPI_CAST_PTR (UINT64, BufferDesc->Buffer.Pointer),
                    Function);
        AcpiExReleaseGlobalLock (ObjDesc->CommonField.FieldFlags);
        goto Exit;
    }

    /*
     * Allocate a buffer for the contents of the field.
     *
     * If the field is larger than the current integer width, create
     * a BUFFER to hold it. Otherwise, use an INTEGER. This allows
     * the use of arithmetic operators on the returned value if the
     * field size is equal or smaller than an Integer.
     *
     * Note: Field.length is in bits.
     */
    Length = (ACPI_SIZE) ACPI_ROUND_BITS_UP_TO_BYTES (ObjDesc->Field.BitLength);
    if (Length > AcpiGbl_IntegerByteWidth)
    {
        /* Field is too large for an Integer, create a Buffer instead */

        BufferDesc = AcpiUtCreateBufferObject (Length);
        if (!BufferDesc)
        {
            return_ACPI_STATUS (AE_NO_MEMORY);
        }
        Buffer = BufferDesc->Buffer.Pointer;
    }
    else
    {
        /* Field will fit within an Integer (normal case) */

        BufferDesc = AcpiUtCreateIntegerObject ((UINT64) 0);
        if (!BufferDesc)
        {
            return_ACPI_STATUS (AE_NO_MEMORY);
        }

        Length = AcpiGbl_IntegerByteWidth;
        Buffer = &BufferDesc->Integer.Value;
    }

    if ((ObjDesc->Common.Type == ACPI_TYPE_LOCAL_REGION_FIELD) &&
        (ObjDesc->Field.RegionObj->Region.SpaceId == ACPI_ADR_SPACE_GPIO))
    {
        /*
         * For GPIO (GeneralPurposeIo), the Address will be the bit offset
         * from the previous Connection() operator, making it effectively a
         * pin number index. The BitLength is the length of the field, which
         * is thus the number of pins.
         */
        ACPI_DEBUG_PRINT ((ACPI_DB_BFIELD,
            "GPIO FieldRead [FROM]:  Pin %u Bits %u\n",
            ObjDesc->Field.PinNumberIndex, ObjDesc->Field.BitLength));

        /* Lock entire transaction if requested */

        AcpiExAcquireGlobalLock (ObjDesc->CommonField.FieldFlags);

        /* Perform the write */

        Status = AcpiExAccessRegion (ObjDesc, 0,
                    (UINT64 *) Buffer, ACPI_READ);
        AcpiExReleaseGlobalLock (ObjDesc->CommonField.FieldFlags);
        if (ACPI_FAILURE (Status))
        {
            AcpiUtRemoveReference (BufferDesc);
        }
        else
        {
            *RetBufferDesc = BufferDesc;
        }
        return_ACPI_STATUS (Status);
    }

    ACPI_DEBUG_PRINT ((ACPI_DB_BFIELD,
        "FieldRead [TO]:   Obj %p, Type %X, Buf %p, ByteLen %X\n",
        ObjDesc, ObjDesc->Common.Type, Buffer, (UINT32) Length));
    ACPI_DEBUG_PRINT ((ACPI_DB_BFIELD,
        "FieldRead [FROM]: BitLen %X, BitOff %X, ByteOff %X\n",
        ObjDesc->CommonField.BitLength,
        ObjDesc->CommonField.StartFieldBitOffset,
        ObjDesc->CommonField.BaseByteOffset));

    /* Lock entire transaction if requested */

    AcpiExAcquireGlobalLock (ObjDesc->CommonField.FieldFlags);

    /* Read from the field */

    Status = AcpiExExtractFromField (ObjDesc, Buffer, (UINT32) Length);
    AcpiExReleaseGlobalLock (ObjDesc->CommonField.FieldFlags);


Exit:
    if (ACPI_FAILURE (Status))
    {
        AcpiUtRemoveReference (BufferDesc);
    }
    else
    {
        *RetBufferDesc = BufferDesc;
    }

    return_ACPI_STATUS (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiExWriteDataToField
 *
 * PARAMETERS:  SourceDesc          - Contains data to write
 *              ObjDesc             - The named field
 *              ResultDesc          - Where the return value is returned, if any
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Write to a named field
 *
 ******************************************************************************/

ACPI_STATUS
AcpiExWriteDataToField (
    ACPI_OPERAND_OBJECT     *SourceDesc,
    ACPI_OPERAND_OBJECT     *ObjDesc,
    ACPI_OPERAND_OBJECT     **ResultDesc)
{
    ACPI_STATUS             Status;
    UINT32                  Length;
    void                    *Buffer;
    ACPI_OPERAND_OBJECT     *BufferDesc;
    UINT32                  Function;
    UINT16                  AccessorType;


    ACPI_FUNCTION_TRACE_PTR (ExWriteDataToField, ObjDesc);


    /* Parameter validation */

    if (!SourceDesc || !ObjDesc)
    {
        return_ACPI_STATUS (AE_AML_NO_OPERAND);
    }

    if (ObjDesc->Common.Type == ACPI_TYPE_BUFFER_FIELD)
    {
        /*
         * If the BufferField arguments have not been previously evaluated,
         * evaluate them now and save the results.
         */
        if (!(ObjDesc->Common.Flags & AOPOBJ_DATA_VALID))
        {
            Status = AcpiDsGetBufferFieldArguments (ObjDesc);
            if (ACPI_FAILURE (Status))
            {
                return_ACPI_STATUS (Status);
            }
        }
    }
    else if ((ObjDesc->Common.Type == ACPI_TYPE_LOCAL_REGION_FIELD) &&
             (ObjDesc->Field.RegionObj->Region.SpaceId == ACPI_ADR_SPACE_SMBUS ||
              ObjDesc->Field.RegionObj->Region.SpaceId == ACPI_ADR_SPACE_GSBUS ||
              ObjDesc->Field.RegionObj->Region.SpaceId == ACPI_ADR_SPACE_IPMI))
    {
        /*
         * This is an SMBus, GSBus or IPMI write. We will bypass the entire field
         * mechanism and handoff the buffer directly to the handler. For
         * these address spaces, the buffer is bi-directional; on a write,
         * return data is returned in the same buffer.
         *
         * Source must be a buffer of sufficient size:
         * ACPI_SMBUS_BUFFER_SIZE, ACPI_GSBUS_BUFFER_SIZE, or ACPI_IPMI_BUFFER_SIZE.
         *
         * Note: SMBus and GSBus protocol type is passed in upper 16-bits of Function
         */
        if (SourceDesc->Common.Type != ACPI_TYPE_BUFFER)
        {
            ACPI_ERROR ((AE_INFO,
                "SMBus/IPMI/GenericSerialBus write requires Buffer, found type %s",
                AcpiUtGetObjectTypeName (SourceDesc)));

            return_ACPI_STATUS (AE_AML_OPERAND_TYPE);
        }

        if (ObjDesc->Field.RegionObj->Region.SpaceId == ACPI_ADR_SPACE_SMBUS)
        {
            Length = ACPI_SMBUS_BUFFER_SIZE;
            Function = ACPI_WRITE | (ObjDesc->Field.Attribute << 16);
        }
        else if (ObjDesc->Field.RegionObj->Region.SpaceId == ACPI_ADR_SPACE_GSBUS)
        {
            AccessorType = ObjDesc->Field.Attribute;
            Length = AcpiExGetSerialAccessLength (AccessorType,
                ObjDesc->Field.AccessLength);

            /*
             * Add additional 2 bytes for the GenericSerialBus data buffer:
             *
             *     Status;      (Byte 0 of the data buffer)
             *     Length;      (Byte 1 of the data buffer)
             *     Data[x-1];   (Bytes 2-x of the arbitrary length data buffer)
             */
            Length += 2;
            Function = ACPI_WRITE | (AccessorType << 16);
        }
        else /* IPMI */
        {
            Length = ACPI_IPMI_BUFFER_SIZE;
            Function = ACPI_WRITE;
        }

        if (SourceDesc->Buffer.Length < Length)
        {
            ACPI_ERROR ((AE_INFO,
                "SMBus/IPMI/GenericSerialBus write requires Buffer of length %u, found length %u",
                Length, SourceDesc->Buffer.Length));

            return_ACPI_STATUS (AE_AML_BUFFER_LIMIT);
        }

        /* Create the bi-directional buffer */

        BufferDesc = AcpiUtCreateBufferObject (Length);
        if (!BufferDesc)
        {
            return_ACPI_STATUS (AE_NO_MEMORY);
        }

        Buffer = BufferDesc->Buffer.Pointer;
        ACPI_MEMCPY (Buffer, SourceDesc->Buffer.Pointer, Length);

        /* Lock entire transaction if requested */

        AcpiExAcquireGlobalLock (ObjDesc->CommonField.FieldFlags);

        /*
         * Perform the write (returns status and perhaps data in the
         * same buffer)
         */
        Status = AcpiExAccessRegion (ObjDesc, 0,
                    (UINT64 *) Buffer, Function);
        AcpiExReleaseGlobalLock (ObjDesc->CommonField.FieldFlags);

        *ResultDesc = BufferDesc;
        return_ACPI_STATUS (Status);
    }
    else if ((ObjDesc->Common.Type == ACPI_TYPE_LOCAL_REGION_FIELD) &&
             (ObjDesc->Field.RegionObj->Region.SpaceId == ACPI_ADR_SPACE_GPIO))
    {
        /*
         * For GPIO (GeneralPurposeIo), we will bypass the entire field
         * mechanism and handoff the bit address and bit width directly to
         * the handler. The Address will be the bit offset
         * from the previous Connection() operator, making it effectively a
         * pin number index. The BitLength is the length of the field, which
         * is thus the number of pins.
         */
        if (SourceDesc->Common.Type != ACPI_TYPE_INTEGER)
        {
            return_ACPI_STATUS (AE_AML_OPERAND_TYPE);
        }

        ACPI_DEBUG_PRINT ((ACPI_DB_BFIELD,
            "GPIO FieldWrite [FROM]: (%s:%X), Val %.8X  [TO]:  Pin %u Bits %u\n",
            AcpiUtGetTypeName (SourceDesc->Common.Type),
            SourceDesc->Common.Type, (UINT32) SourceDesc->Integer.Value,
            ObjDesc->Field.PinNumberIndex, ObjDesc->Field.BitLength));

        Buffer = &SourceDesc->Integer.Value;

        /* Lock entire transaction if requested */

        AcpiExAcquireGlobalLock (ObjDesc->CommonField.FieldFlags);

        /* Perform the write */

        Status = AcpiExAccessRegion (ObjDesc, 0,
                    (UINT64 *) Buffer, ACPI_WRITE);
        AcpiExReleaseGlobalLock (ObjDesc->CommonField.FieldFlags);
        return_ACPI_STATUS (Status);
    }

    /* Get a pointer to the data to be written */

    switch (SourceDesc->Common.Type)
    {
    case ACPI_TYPE_INTEGER:

        Buffer = &SourceDesc->Integer.Value;
        Length = sizeof (SourceDesc->Integer.Value);
        break;

    case ACPI_TYPE_BUFFER:

        Buffer = SourceDesc->Buffer.Pointer;
        Length = SourceDesc->Buffer.Length;
        break;

    case ACPI_TYPE_STRING:

        Buffer = SourceDesc->String.Pointer;
        Length = SourceDesc->String.Length;
        break;

    default:

        return_ACPI_STATUS (AE_AML_OPERAND_TYPE);
    }

    ACPI_DEBUG_PRINT ((ACPI_DB_BFIELD,
        "FieldWrite [FROM]: Obj %p (%s:%X), Buf %p, ByteLen %X\n",
        SourceDesc, AcpiUtGetTypeName (SourceDesc->Common.Type),
        SourceDesc->Common.Type, Buffer, Length));

    ACPI_DEBUG_PRINT ((ACPI_DB_BFIELD,
        "FieldWrite [TO]:   Obj %p (%s:%X), BitLen %X, BitOff %X, ByteOff %X\n",
        ObjDesc, AcpiUtGetTypeName (ObjDesc->Common.Type),
        ObjDesc->Common.Type,
        ObjDesc->CommonField.BitLength,
        ObjDesc->CommonField.StartFieldBitOffset,
        ObjDesc->CommonField.BaseByteOffset));

    /* Lock entire transaction if requested */

    AcpiExAcquireGlobalLock (ObjDesc->CommonField.FieldFlags);

    /* Write to the field */

    Status = AcpiExInsertIntoField (ObjDesc, Buffer, Length);
    AcpiExReleaseGlobalLock (ObjDesc->CommonField.FieldFlags);

    return_ACPI_STATUS (Status);
}
