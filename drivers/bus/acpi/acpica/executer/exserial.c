/******************************************************************************
 *
 * Module Name: exserial - FieldUnit support for serial address spaces
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
#include "acdispat.h"
#include "acinterp.h"
#include "amlcode.h"


#define _COMPONENT          ACPI_EXECUTER
        ACPI_MODULE_NAME    ("exserial")


/*******************************************************************************
 *
 * FUNCTION:    AcpiExReadGpio
 *
 * PARAMETERS:  ObjDesc             - The named field to read
 *              Buffer              - Where the return data is returned
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Read from a named field that references a Generic Serial Bus
 *              field
 *
 ******************************************************************************/

ACPI_STATUS
AcpiExReadGpio (
    ACPI_OPERAND_OBJECT     *ObjDesc,
    void                    *Buffer)
{
    ACPI_STATUS             Status;


    ACPI_FUNCTION_TRACE_PTR (ExReadGpio, ObjDesc);


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

    /* Perform the read */

    Status = AcpiExAccessRegion (
        ObjDesc, 0, (UINT64 *) Buffer, ACPI_READ);

    AcpiExReleaseGlobalLock (ObjDesc->CommonField.FieldFlags);
    return_ACPI_STATUS (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiExWriteGpio
 *
 * PARAMETERS:  SourceDesc          - Contains data to write. Expect to be
 *                                    an Integer object.
 *              ObjDesc             - The named field
 *              ResultDesc          - Where the return value is returned, if any
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Write to a named field that references a General Purpose I/O
 *              field.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiExWriteGpio (
    ACPI_OPERAND_OBJECT     *SourceDesc,
    ACPI_OPERAND_OBJECT     *ObjDesc,
    ACPI_OPERAND_OBJECT     **ReturnBuffer)
{
    ACPI_STATUS             Status;
    void                    *Buffer;


    ACPI_FUNCTION_TRACE_PTR (ExWriteGpio, ObjDesc);


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
        "GPIO FieldWrite [FROM]: (%s:%X), Value %.8X  [TO]: Pin %u Bits %u\n",
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


/*******************************************************************************
 *
 * FUNCTION:    AcpiExReadSerialBus
 *
 * PARAMETERS:  ObjDesc             - The named field to read
 *              ReturnBuffer        - Where the return value is returned, if any
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Read from a named field that references a serial bus
 *              (SMBus, IPMI, or GSBus).
 *
 ******************************************************************************/

ACPI_STATUS
AcpiExReadSerialBus (
    ACPI_OPERAND_OBJECT     *ObjDesc,
    ACPI_OPERAND_OBJECT     **ReturnBuffer)
{
    ACPI_STATUS             Status;
    UINT32                  BufferLength;
    ACPI_OPERAND_OBJECT     *BufferDesc;
    UINT32                  Function;
    UINT16                  AccessorType;


    ACPI_FUNCTION_TRACE_PTR (ExReadSerialBus, ObjDesc);


    /*
     * This is an SMBus, GSBus or IPMI read. We must create a buffer to
     * hold the data and then directly access the region handler.
     *
     * Note: SMBus and GSBus protocol value is passed in upper 16-bits
     * of Function
     *
     * Common buffer format:
     *     Status;    (Byte 0 of the data buffer)
     *     Length;    (Byte 1 of the data buffer)
     *     Data[x-1]: (Bytes 2-x of the arbitrary length data buffer)
     */
    switch (ObjDesc->Field.RegionObj->Region.SpaceId)
    {
    case ACPI_ADR_SPACE_SMBUS:

        BufferLength = ACPI_SMBUS_BUFFER_SIZE;
        Function = ACPI_READ | (ObjDesc->Field.Attribute << 16);
        break;

    case ACPI_ADR_SPACE_IPMI:

        BufferLength = ACPI_IPMI_BUFFER_SIZE;
        Function = ACPI_READ;
        break;

    case ACPI_ADR_SPACE_GSBUS:

        AccessorType = ObjDesc->Field.Attribute;
        if (AccessorType == AML_FIELD_ATTRIB_RAW_PROCESS_BYTES)
        {
            ACPI_ERROR ((AE_INFO,
                "Invalid direct read using bidirectional write-then-read protocol"));

            return_ACPI_STATUS (AE_AML_PROTOCOL);
        }

        Status = AcpiExGetProtocolBufferLength (AccessorType, &BufferLength);
        if (ACPI_FAILURE (Status))
        {
            ACPI_ERROR ((AE_INFO,
                "Invalid protocol ID for GSBus: 0x%4.4X", AccessorType));

            return_ACPI_STATUS (Status);
        }

        /* Add header length to get the full size of the buffer */

        BufferLength += ACPI_SERIAL_HEADER_SIZE;
        Function = ACPI_READ | (AccessorType << 16);
        break;

    case ACPI_ADR_SPACE_PLATFORM_RT:

        BufferLength = ACPI_PRM_INPUT_BUFFER_SIZE;
        Function = ACPI_READ;
        break;

    default:
        return_ACPI_STATUS (AE_AML_INVALID_SPACE_ID);
    }

    /* Create the local transfer buffer that is returned to the caller */

    BufferDesc = AcpiUtCreateBufferObject (BufferLength);
    if (!BufferDesc)
    {
        return_ACPI_STATUS (AE_NO_MEMORY);
    }

    /* Lock entire transaction if requested */

    AcpiExAcquireGlobalLock (ObjDesc->CommonField.FieldFlags);

    /* Call the region handler for the write-then-read */

    Status = AcpiExAccessRegion (ObjDesc, 0,
        ACPI_CAST_PTR (UINT64, BufferDesc->Buffer.Pointer), Function);
    AcpiExReleaseGlobalLock (ObjDesc->CommonField.FieldFlags);

    *ReturnBuffer = BufferDesc;
    return_ACPI_STATUS (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiExWriteSerialBus
 *
 * PARAMETERS:  SourceDesc          - Contains data to write
 *              ObjDesc             - The named field
 *              ReturnBuffer        - Where the return value is returned, if any
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Write to a named field that references a serial bus
 *              (SMBus, IPMI, GSBus).
 *
 ******************************************************************************/

ACPI_STATUS
AcpiExWriteSerialBus (
    ACPI_OPERAND_OBJECT     *SourceDesc,
    ACPI_OPERAND_OBJECT     *ObjDesc,
    ACPI_OPERAND_OBJECT     **ReturnBuffer)
{
    ACPI_STATUS             Status;
    UINT32                  BufferLength;
    UINT32                  DataLength;
    void                    *Buffer;
    ACPI_OPERAND_OBJECT     *BufferDesc;
    UINT32                  Function;
    UINT16                  AccessorType;


    ACPI_FUNCTION_TRACE_PTR (ExWriteSerialBus, ObjDesc);


    /*
     * This is an SMBus, GSBus or IPMI write. We will bypass the entire
     * field mechanism and handoff the buffer directly to the handler.
     * For these address spaces, the buffer is bidirectional; on a
     * write, return data is returned in the same buffer.
     *
     * Source must be a buffer of sufficient size, these are fixed size:
     * ACPI_SMBUS_BUFFER_SIZE, or ACPI_IPMI_BUFFER_SIZE.
     *
     * Note: SMBus and GSBus protocol type is passed in upper 16-bits
     * of Function
     *
     * Common buffer format:
     *     Status;    (Byte 0 of the data buffer)
     *     Length;    (Byte 1 of the data buffer)
     *     Data[x-1]: (Bytes 2-x of the arbitrary length data buffer)
     */
    if (SourceDesc->Common.Type != ACPI_TYPE_BUFFER)
    {
        ACPI_ERROR ((AE_INFO,
            "SMBus/IPMI/GenericSerialBus write requires "
            "Buffer, found type %s",
            AcpiUtGetObjectTypeName (SourceDesc)));

        return_ACPI_STATUS (AE_AML_OPERAND_TYPE);
    }

    switch (ObjDesc->Field.RegionObj->Region.SpaceId)
    {
    case ACPI_ADR_SPACE_SMBUS:

        BufferLength = ACPI_SMBUS_BUFFER_SIZE;
        Function = ACPI_WRITE | (ObjDesc->Field.Attribute << 16);
        break;

    case ACPI_ADR_SPACE_IPMI:

        BufferLength = ACPI_IPMI_BUFFER_SIZE;
        Function = ACPI_WRITE;
        break;

    case ACPI_ADR_SPACE_GSBUS:

        AccessorType = ObjDesc->Field.Attribute;
        Status = AcpiExGetProtocolBufferLength (AccessorType, &BufferLength);
        if (ACPI_FAILURE (Status))
        {
            ACPI_ERROR ((AE_INFO,
                "Invalid protocol ID for GSBus: 0x%4.4X", AccessorType));

            return_ACPI_STATUS (Status);
        }

        /* Add header length to get the full size of the buffer */

        BufferLength += ACPI_SERIAL_HEADER_SIZE;
        Function = ACPI_WRITE | (AccessorType << 16);
        break;

    case ACPI_ADR_SPACE_PLATFORM_RT:

        BufferLength = ACPI_PRM_INPUT_BUFFER_SIZE;
        Function = ACPI_WRITE;
        break;

    default:
        return_ACPI_STATUS (AE_AML_INVALID_SPACE_ID);
    }

    /* Create the transfer/bidirectional/return buffer */

    BufferDesc = AcpiUtCreateBufferObject (BufferLength);
    if (!BufferDesc)
    {
        return_ACPI_STATUS (AE_NO_MEMORY);
    }

    /* Copy the input buffer data to the transfer buffer */

    Buffer = BufferDesc->Buffer.Pointer;
    DataLength = (BufferLength < SourceDesc->Buffer.Length ?
        BufferLength : SourceDesc->Buffer.Length);
    memcpy (Buffer, SourceDesc->Buffer.Pointer, DataLength);

    /* Lock entire transaction if requested */

    AcpiExAcquireGlobalLock (ObjDesc->CommonField.FieldFlags);

    /*
     * Perform the write (returns status and perhaps data in the
     * same buffer)
     */
    Status = AcpiExAccessRegion (
        ObjDesc, 0, (UINT64 *) Buffer, Function);
    AcpiExReleaseGlobalLock (ObjDesc->CommonField.FieldFlags);

    *ReturnBuffer = BufferDesc;
    return_ACPI_STATUS (Status);
}
