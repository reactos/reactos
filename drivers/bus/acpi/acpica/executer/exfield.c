/******************************************************************************
 *
 * Module Name: exfield - AML execution - FieldUnit read/write
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
#include "acdispat.h"
#include "acinterp.h"
#include "amlcode.h"


#define _COMPONENT          ACPI_EXECUTER
        ACPI_MODULE_NAME    ("exfield")


/*
 * This table maps the various Attrib protocols to the byte transfer
 * length. Used for the generic serial bus.
 */
#define ACPI_INVALID_PROTOCOL_ID        0x80
#define ACPI_MAX_PROTOCOL_ID            0x0F

static const UINT8      AcpiProtocolLengths[] =
{
    ACPI_INVALID_PROTOCOL_ID,   /* 0 - reserved */
    ACPI_INVALID_PROTOCOL_ID,   /* 1 - reserved */
    0x00,                       /* 2 - ATTRIB_QUICK */
    ACPI_INVALID_PROTOCOL_ID,   /* 3 - reserved */
    0x01,                       /* 4 - ATTRIB_SEND_RECEIVE */
    ACPI_INVALID_PROTOCOL_ID,   /* 5 - reserved */
    0x01,                       /* 6 - ATTRIB_BYTE */
    ACPI_INVALID_PROTOCOL_ID,   /* 7 - reserved */
    0x02,                       /* 8 - ATTRIB_WORD */
    ACPI_INVALID_PROTOCOL_ID,   /* 9 - reserved */
    0xFF,                       /* A - ATTRIB_BLOCK  */
    0xFF,                       /* B - ATTRIB_BYTES */
    0x02,                       /* C - ATTRIB_PROCESS_CALL */
    0xFF,                       /* D - ATTRIB_BLOCK_PROCESS_CALL */
    0xFF,                       /* E - ATTRIB_RAW_BYTES */
    0xFF                        /* F - ATTRIB_RAW_PROCESS_BYTES */
};

#define PCC_MASTER_SUBSPACE     3

/*
 * The following macros determine a given offset is a COMD field.
 * According to the specification, generic subspaces (types 0-2) contains a
 * 2-byte COMD field at offset 4 and master subspaces (type 3) contains a 4-byte
 * COMD field starting at offset 12.
 */
#define GENERIC_SUBSPACE_COMMAND(a)     (4 == a || a == 5)
#define MASTER_SUBSPACE_COMMAND(a)      (12 <= a && a <= 15)


/*******************************************************************************
 *
 * FUNCTION:    AcpiExGetProtocolBufferLength
 *
 * PARAMETERS:  ProtocolId      - The type of the protocol indicated by region
 *                                field access attributes
 *              ReturnLength    - Where the protocol byte transfer length is
 *                                returned
 *
 * RETURN:      Status and decoded byte transfer length
 *
 * DESCRIPTION: This routine returns the length of the GenericSerialBus
 *              protocol bytes
 *
 ******************************************************************************/

ACPI_STATUS
AcpiExGetProtocolBufferLength (
    UINT32                  ProtocolId,
    UINT32                  *ReturnLength)
{

    if ((ProtocolId > ACPI_MAX_PROTOCOL_ID) ||
        (AcpiProtocolLengths[ProtocolId] == ACPI_INVALID_PROTOCOL_ID))
    {
        ACPI_ERROR ((AE_INFO,
            "Invalid Field/AccessAs protocol ID: 0x%4.4X", ProtocolId));

        return (AE_AML_PROTOCOL);
    }

    *ReturnLength = AcpiProtocolLengths[ProtocolId];
    return (AE_OK);
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
 *              Buffer, depending on the size of the field and whether if a
 *              field is created by the CreateField() operator.
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
    void                    *Buffer;
    UINT32                  BufferLength;


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
        /* SMBus, GSBus, IPMI serial */

        Status = AcpiExReadSerialBus (ObjDesc, RetBufferDesc);
        return_ACPI_STATUS (Status);
    }

    /*
     * Allocate a buffer for the contents of the field.
     *
     * If the field is larger than the current integer width, create
     * a BUFFER to hold it. Otherwise, use an INTEGER. This allows
     * the use of arithmetic operators on the returned value if the
     * field size is equal or smaller than an Integer.
     *
     * However, all buffer fields created by CreateField operator needs to
     * remain as a buffer to match other AML interpreter implementations.
     *
     * Note: Field.length is in bits.
     */
    BufferLength = (ACPI_SIZE) ACPI_ROUND_BITS_UP_TO_BYTES (
        ObjDesc->Field.BitLength);

    if (BufferLength > AcpiGbl_IntegerByteWidth ||
        (ObjDesc->Common.Type == ACPI_TYPE_BUFFER_FIELD &&
        ObjDesc->BufferField.IsCreateField))
    {
        /* Field is too large for an Integer, create a Buffer instead */

        BufferDesc = AcpiUtCreateBufferObject (BufferLength);
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

        BufferLength = AcpiGbl_IntegerByteWidth;
        Buffer = &BufferDesc->Integer.Value;
    }

    if ((ObjDesc->Common.Type == ACPI_TYPE_LOCAL_REGION_FIELD) &&
        (ObjDesc->Field.RegionObj->Region.SpaceId == ACPI_ADR_SPACE_GPIO))
    {
        /* General Purpose I/O */

        Status = AcpiExReadGpio (ObjDesc, Buffer);
        goto Exit;
    }
    else if ((ObjDesc->Common.Type == ACPI_TYPE_LOCAL_REGION_FIELD) &&
        (ObjDesc->Field.RegionObj->Region.SpaceId == ACPI_ADR_SPACE_PLATFORM_COMM))
    {
        /*
         * Reading from a PCC field unit does not require the handler because
         * it only requires reading from the InternalPccBuffer.
         */
        ACPI_DEBUG_PRINT ((ACPI_DB_BFIELD,
            "PCC FieldRead bits %u\n", ObjDesc->Field.BitLength));

        memcpy (Buffer, ObjDesc->Field.RegionObj->Field.InternalPccBuffer +
        ObjDesc->Field.BaseByteOffset, (ACPI_SIZE) ACPI_ROUND_BITS_UP_TO_BYTES (
            ObjDesc->Field.BitLength));

        *RetBufferDesc = BufferDesc;
        return AE_OK;
    }

    ACPI_DEBUG_PRINT ((ACPI_DB_BFIELD,
        "FieldRead [TO]:   Obj %p, Type %X, Buf %p, ByteLen %X\n",
        ObjDesc, ObjDesc->Common.Type, Buffer, BufferLength));
    ACPI_DEBUG_PRINT ((ACPI_DB_BFIELD,
        "FieldRead [FROM]: BitLen %X, BitOff %X, ByteOff %X\n",
        ObjDesc->CommonField.BitLength,
        ObjDesc->CommonField.StartFieldBitOffset,
        ObjDesc->CommonField.BaseByteOffset));

    /* Lock entire transaction if requested */

    AcpiExAcquireGlobalLock (ObjDesc->CommonField.FieldFlags);

    /* Read from the field */

    Status = AcpiExExtractFromField (ObjDesc, Buffer, BufferLength);
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
    UINT32                  BufferLength;
    UINT32                  DataLength;
    void                    *Buffer;


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
        (ObjDesc->Field.RegionObj->Region.SpaceId == ACPI_ADR_SPACE_GPIO))
    {
        /* General Purpose I/O */

        Status = AcpiExWriteGpio (SourceDesc, ObjDesc, ResultDesc);
        return_ACPI_STATUS (Status);
    }
    else if ((ObjDesc->Common.Type == ACPI_TYPE_LOCAL_REGION_FIELD) &&
        (ObjDesc->Field.RegionObj->Region.SpaceId == ACPI_ADR_SPACE_SMBUS ||
         ObjDesc->Field.RegionObj->Region.SpaceId == ACPI_ADR_SPACE_GSBUS ||
         ObjDesc->Field.RegionObj->Region.SpaceId == ACPI_ADR_SPACE_IPMI))
    {
        /* SMBus, GSBus, IPMI serial */

        Status = AcpiExWriteSerialBus (SourceDesc, ObjDesc, ResultDesc);
        return_ACPI_STATUS (Status);
    }
    else if ((ObjDesc->Common.Type == ACPI_TYPE_LOCAL_REGION_FIELD) &&
             (ObjDesc->Field.RegionObj->Region.SpaceId == ACPI_ADR_SPACE_PLATFORM_COMM))
    {
        /*
         * According to the spec a write to the COMD field will invoke the
         * region handler. Otherwise, write to the PccInternal buffer. This
         * implementation will use the offsets specified rather than the name
         * of the field. This is considered safer because some firmware tools
         * are known to obfiscate named objects.
         */
        DataLength = (ACPI_SIZE) ACPI_ROUND_BITS_UP_TO_BYTES (
            ObjDesc->Field.BitLength);
        memcpy (ObjDesc->Field.RegionObj->Field.InternalPccBuffer +
            ObjDesc->Field.BaseByteOffset,
            SourceDesc->Buffer.Pointer, DataLength);

        if ((ObjDesc->Field.RegionObj->Region.Address == PCC_MASTER_SUBSPACE &&
           MASTER_SUBSPACE_COMMAND (ObjDesc->Field.BaseByteOffset)) ||
           GENERIC_SUBSPACE_COMMAND (ObjDesc->Field.BaseByteOffset))
        {
            /* Perform the write */

            ACPI_DEBUG_PRINT ((ACPI_DB_BFIELD,
                "PCC COMD field has been written. Invoking PCC handler now.\n"));

            Status = AcpiExAccessRegion (
                ObjDesc, 0, (UINT64 *) ObjDesc->Field.RegionObj->Field.InternalPccBuffer,
                ACPI_WRITE);
            return_ACPI_STATUS (Status);
        }
        return (AE_OK);
    }


    /* Get a pointer to the data to be written */

    switch (SourceDesc->Common.Type)
    {
    case ACPI_TYPE_INTEGER:

        Buffer = &SourceDesc->Integer.Value;
        BufferLength = sizeof (SourceDesc->Integer.Value);
        break;

    case ACPI_TYPE_BUFFER:

        Buffer = SourceDesc->Buffer.Pointer;
        BufferLength = SourceDesc->Buffer.Length;
        break;

    case ACPI_TYPE_STRING:

        Buffer = SourceDesc->String.Pointer;
        BufferLength = SourceDesc->String.Length;
        break;

    default:
        return_ACPI_STATUS (AE_AML_OPERAND_TYPE);
    }

    ACPI_DEBUG_PRINT ((ACPI_DB_BFIELD,
        "FieldWrite [FROM]: Obj %p (%s:%X), Buf %p, ByteLen %X\n",
        SourceDesc, AcpiUtGetTypeName (SourceDesc->Common.Type),
        SourceDesc->Common.Type, Buffer, BufferLength));

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

    Status = AcpiExInsertIntoField (ObjDesc, Buffer, BufferLength);
    AcpiExReleaseGlobalLock (ObjDesc->CommonField.FieldFlags);
    return_ACPI_STATUS (Status);
}
