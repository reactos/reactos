/******************************************************************************
 *
 * Module Name: exfield - ACPI AML (p-code) execution - field manipulation
 *
 *****************************************************************************/

/*
 * Copyright (C) 2000 - 2016, Intel Corp.
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
         * This is an SMBus, GSBus or IPMI read. We must create a buffer to
         * hold the data and then directly access the region handler.
         *
         * Note: SMBus and GSBus protocol value is passed in upper 16-bits
         * of Function
         */
        if (ObjDesc->Field.RegionObj->Region.SpaceId ==
            ACPI_ADR_SPACE_SMBUS)
        {
            Length = ACPI_SMBUS_BUFFER_SIZE;
            Function = ACPI_READ | (ObjDesc->Field.Attribute << 16);
        }
        else if (ObjDesc->Field.RegionObj->Region.SpaceId ==
            ACPI_ADR_SPACE_GSBUS)
        {
            AccessorType = ObjDesc->Field.Attribute;
            Length = AcpiExGetSerialAccessLength (
                AccessorType, ObjDesc->Field.AccessLength);

            /*
             * Add additional 2 bytes for the GenericSerialBus data buffer:
             *
             *     Status;    (Byte 0 of the data buffer)
             *     Length;    (Byte 1 of the data buffer)
             *     Data[x-1]: (Bytes 2-x of the arbitrary length data buffer)
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
            ACPI_CAST_PTR (UINT64, BufferDesc->Buffer.Pointer), Function);

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
    Length = (ACPI_SIZE) ACPI_ROUND_BITS_UP_TO_BYTES (
        ObjDesc->Field.BitLength);

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

        Status = AcpiExAccessRegion (
            ObjDesc, 0, (UINT64 *) Buffer, ACPI_READ);

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
         * This is an SMBus, GSBus or IPMI write. We will bypass the entire
         * field mechanism and handoff the buffer directly to the handler.
         * For these address spaces, the buffer is bi-directional; on a
         * write, return data is returned in the same buffer.
         *
         * Source must be a buffer of sufficient size:
         * ACPI_SMBUS_BUFFER_SIZE, ACPI_GSBUS_BUFFER_SIZE, or
         * ACPI_IPMI_BUFFER_SIZE.
         *
         * Note: SMBus and GSBus protocol type is passed in upper 16-bits
         * of Function
         */
        if (SourceDesc->Common.Type != ACPI_TYPE_BUFFER)
        {
            ACPI_ERROR ((AE_INFO,
                "SMBus/IPMI/GenericSerialBus write requires "
                "Buffer, found type %s",
                AcpiUtGetObjectTypeName (SourceDesc)));

            return_ACPI_STATUS (AE_AML_OPERAND_TYPE);
        }

        if (ObjDesc->Field.RegionObj->Region.SpaceId ==
            ACPI_ADR_SPACE_SMBUS)
        {
            Length = ACPI_SMBUS_BUFFER_SIZE;
            Function = ACPI_WRITE | (ObjDesc->Field.Attribute << 16);
        }
        else if (ObjDesc->Field.RegionObj->Region.SpaceId ==
            ACPI_ADR_SPACE_GSBUS)
        {
            AccessorType = ObjDesc->Field.Attribute;
            Length = AcpiExGetSerialAccessLength (
                AccessorType, ObjDesc->Field.AccessLength);

            /*
             * Add additional 2 bytes for the GenericSerialBus data buffer:
             *
             *     Status;    (Byte 0 of the data buffer)
             *     Length;    (Byte 1 of the data buffer)
             *     Data[x-1]: (Bytes 2-x of the arbitrary length data buffer)
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
                "SMBus/IPMI/GenericSerialBus write requires "
                "Buffer of length %u, found length %u",
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
        memcpy (Buffer, SourceDesc->Buffer.Pointer, Length);

        /* Lock entire transaction if requested */

        AcpiExAcquireGlobalLock (ObjDesc->CommonField.FieldFlags);

        /*
         * Perform the write (returns status and perhaps data in the
         * same buffer)
         */
        Status = AcpiExAccessRegion (
            ObjDesc, 0, (UINT64 *) Buffer, Function);
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
            "GPIO FieldWrite [FROM]: (%s:%X), Val %.8X  [TO]: Pin %u Bits %u\n",
            AcpiUtGetTypeName (SourceDesc->Common.Type),
            SourceDesc->Common.Type, (UINT32) SourceDesc->Integer.Value,
            ObjDesc->Field.PinNumberIndex, ObjDesc->Field.BitLength));

        Buffer = &SourceDesc->Integer.Value;

        /* Lock entire transaction if requested */

        AcpiExAcquireGlobalLock (ObjDesc->CommonField.FieldFlags);

        /* Perform the write */

        Status = AcpiExAccessRegion (
            ObjDesc, 0, (UINT64 *) Buffer, ACPI_WRITE);
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
