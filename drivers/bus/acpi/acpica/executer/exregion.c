/******************************************************************************
 *
 * Module Name: exregion - ACPI default OpRegion (address space) handlers
 *
 *****************************************************************************/

/*
 * Copyright (C) 2000 - 2017, Intel Corp.
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


#define _COMPONENT          ACPI_EXECUTER
        ACPI_MODULE_NAME    ("exregion")


/*******************************************************************************
 *
 * FUNCTION:    AcpiExSystemMemorySpaceHandler
 *
 * PARAMETERS:  Function            - Read or Write operation
 *              Address             - Where in the space to read or write
 *              BitWidth            - Field width in bits (8, 16, or 32)
 *              Value               - Pointer to in or out value
 *              HandlerContext      - Pointer to Handler's context
 *              RegionContext       - Pointer to context specific to the
 *                                    accessed region
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Handler for the System Memory address space (Op Region)
 *
 ******************************************************************************/

ACPI_STATUS
AcpiExSystemMemorySpaceHandler (
    UINT32                  Function,
    ACPI_PHYSICAL_ADDRESS   Address,
    UINT32                  BitWidth,
    UINT64                  *Value,
    void                    *HandlerContext,
    void                    *RegionContext)
{
    ACPI_STATUS             Status = AE_OK;
    void                    *LogicalAddrPtr = NULL;
    ACPI_MEM_SPACE_CONTEXT  *MemInfo = RegionContext;
    UINT32                  Length;
    ACPI_SIZE               MapLength;
    ACPI_SIZE               PageBoundaryMapLength;
#ifdef ACPI_MISALIGNMENT_NOT_SUPPORTED
    UINT32                  Remainder;
#endif


    ACPI_FUNCTION_TRACE (ExSystemMemorySpaceHandler);


    /* Validate and translate the bit width */

    switch (BitWidth)
    {
    case 8:

        Length = 1;
        break;

    case 16:

        Length = 2;
        break;

    case 32:

        Length = 4;
        break;

    case 64:

        Length = 8;
        break;

    default:

        ACPI_ERROR ((AE_INFO, "Invalid SystemMemory width %u",
            BitWidth));
        return_ACPI_STATUS (AE_AML_OPERAND_VALUE);
    }

#ifdef ACPI_MISALIGNMENT_NOT_SUPPORTED
    /*
     * Hardware does not support non-aligned data transfers, we must verify
     * the request.
     */
    (void) AcpiUtShortDivide ((UINT64) Address, Length, NULL, &Remainder);
    if (Remainder != 0)
    {
        return_ACPI_STATUS (AE_AML_ALIGNMENT);
    }
#endif

    /*
     * Does the request fit into the cached memory mapping?
     * Is 1) Address below the current mapping? OR
     *    2) Address beyond the current mapping?
     */
    if ((Address < MemInfo->MappedPhysicalAddress) ||
        (((UINT64) Address + Length) >
            ((UINT64)
            MemInfo->MappedPhysicalAddress + MemInfo->MappedLength)))
    {
        /*
         * The request cannot be resolved by the current memory mapping;
         * Delete the existing mapping and create a new one.
         */
        if (MemInfo->MappedLength)
        {
            /* Valid mapping, delete it */

            AcpiOsUnmapMemory (MemInfo->MappedLogicalAddress,
                MemInfo->MappedLength);
        }

        /*
         * October 2009: Attempt to map from the requested address to the
         * end of the region. However, we will never map more than one
         * page, nor will we cross a page boundary.
         */
        MapLength = (ACPI_SIZE)
            ((MemInfo->Address + MemInfo->Length) - Address);

        /*
         * If mapping the entire remaining portion of the region will cross
         * a page boundary, just map up to the page boundary, do not cross.
         * On some systems, crossing a page boundary while mapping regions
         * can cause warnings if the pages have different attributes
         * due to resource management.
         *
         * This has the added benefit of constraining a single mapping to
         * one page, which is similar to the original code that used a 4k
         * maximum window.
         */
        PageBoundaryMapLength = (ACPI_SIZE)
            (ACPI_ROUND_UP (Address, ACPI_DEFAULT_PAGE_SIZE) - Address);
        if (PageBoundaryMapLength == 0)
        {
            PageBoundaryMapLength = ACPI_DEFAULT_PAGE_SIZE;
        }

        if (MapLength > PageBoundaryMapLength)
        {
            MapLength = PageBoundaryMapLength;
        }

        /* Create a new mapping starting at the address given */

        MemInfo->MappedLogicalAddress = AcpiOsMapMemory (Address, MapLength);
        if (!MemInfo->MappedLogicalAddress)
        {
            ACPI_ERROR ((AE_INFO,
                "Could not map memory at 0x%8.8X%8.8X, size %u",
                ACPI_FORMAT_UINT64 (Address), (UINT32) MapLength));
            MemInfo->MappedLength = 0;
            return_ACPI_STATUS (AE_NO_MEMORY);
        }

        /* Save the physical address and mapping size */

        MemInfo->MappedPhysicalAddress = Address;
        MemInfo->MappedLength = MapLength;
    }

    /*
     * Generate a logical pointer corresponding to the address we want to
     * access
     */
    LogicalAddrPtr = MemInfo->MappedLogicalAddress +
        ((UINT64) Address - (UINT64) MemInfo->MappedPhysicalAddress);

    ACPI_DEBUG_PRINT ((ACPI_DB_INFO,
        "System-Memory (width %u) R/W %u Address=%8.8X%8.8X\n",
        BitWidth, Function, ACPI_FORMAT_UINT64 (Address)));

    /*
     * Perform the memory read or write
     *
     * Note: For machines that do not support non-aligned transfers, the target
     * address was checked for alignment above. We do not attempt to break the
     * transfer up into smaller (byte-size) chunks because the AML specifically
     * asked for a transfer width that the hardware may require.
     */
    switch (Function)
    {
    case ACPI_READ:

        *Value = 0;
        switch (BitWidth)
        {
        case 8:

            *Value = (UINT64) ACPI_GET8 (LogicalAddrPtr);
            break;

        case 16:

            *Value = (UINT64) ACPI_GET16 (LogicalAddrPtr);
            break;

        case 32:

            *Value = (UINT64) ACPI_GET32 (LogicalAddrPtr);
            break;

        case 64:

            *Value = (UINT64) ACPI_GET64 (LogicalAddrPtr);
            break;

        default:

            /* BitWidth was already validated */

            break;
        }
        break;

    case ACPI_WRITE:

        switch (BitWidth)
        {
        case 8:

            ACPI_SET8 (LogicalAddrPtr, *Value);
            break;

        case 16:

            ACPI_SET16 (LogicalAddrPtr, *Value);
            break;

        case 32:

            ACPI_SET32 (LogicalAddrPtr, *Value);
            break;

        case 64:

            ACPI_SET64 (LogicalAddrPtr, *Value);
            break;

        default:

            /* BitWidth was already validated */

            break;
        }
        break;

    default:

        Status = AE_BAD_PARAMETER;
        break;
    }

    return_ACPI_STATUS (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiExSystemIoSpaceHandler
 *
 * PARAMETERS:  Function            - Read or Write operation
 *              Address             - Where in the space to read or write
 *              BitWidth            - Field width in bits (8, 16, or 32)
 *              Value               - Pointer to in or out value
 *              HandlerContext      - Pointer to Handler's context
 *              RegionContext       - Pointer to context specific to the
 *                                    accessed region
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Handler for the System IO address space (Op Region)
 *
 ******************************************************************************/

ACPI_STATUS
AcpiExSystemIoSpaceHandler (
    UINT32                  Function,
    ACPI_PHYSICAL_ADDRESS   Address,
    UINT32                  BitWidth,
    UINT64                  *Value,
    void                    *HandlerContext,
    void                    *RegionContext)
{
    ACPI_STATUS             Status = AE_OK;
    UINT32                  Value32;


    ACPI_FUNCTION_TRACE (ExSystemIoSpaceHandler);


    ACPI_DEBUG_PRINT ((ACPI_DB_INFO,
        "System-IO (width %u) R/W %u Address=%8.8X%8.8X\n",
        BitWidth, Function, ACPI_FORMAT_UINT64 (Address)));

    /* Decode the function parameter */

    switch (Function)
    {
    case ACPI_READ:

        Status = AcpiHwReadPort ((ACPI_IO_ADDRESS) Address,
                    &Value32, BitWidth);
        *Value = Value32;
        break;

    case ACPI_WRITE:

        Status = AcpiHwWritePort ((ACPI_IO_ADDRESS) Address,
                    (UINT32) *Value, BitWidth);
        break;

    default:

        Status = AE_BAD_PARAMETER;
        break;
    }

    return_ACPI_STATUS (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiExPciConfigSpaceHandler
 *
 * PARAMETERS:  Function            - Read or Write operation
 *              Address             - Where in the space to read or write
 *              BitWidth            - Field width in bits (8, 16, or 32)
 *              Value               - Pointer to in or out value
 *              HandlerContext      - Pointer to Handler's context
 *              RegionContext       - Pointer to context specific to the
 *                                    accessed region
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Handler for the PCI Config address space (Op Region)
 *
 ******************************************************************************/

ACPI_STATUS
AcpiExPciConfigSpaceHandler (
    UINT32                  Function,
    ACPI_PHYSICAL_ADDRESS   Address,
    UINT32                  BitWidth,
    UINT64                  *Value,
    void                    *HandlerContext,
    void                    *RegionContext)
{
    ACPI_STATUS             Status = AE_OK;
    ACPI_PCI_ID             *PciId;
    UINT16                  PciRegister;


    ACPI_FUNCTION_TRACE (ExPciConfigSpaceHandler);


    /*
     *  The arguments to AcpiOs(Read|Write)PciConfiguration are:
     *
     *  PciSegment  is the PCI bus segment range 0-31
     *  PciBus      is the PCI bus number range 0-255
     *  PciDevice   is the PCI device number range 0-31
     *  PciFunction is the PCI device function number
     *  PciRegister is the Config space register range 0-255 bytes
     *
     *  Value - input value for write, output address for read
     *
     */
    PciId       = (ACPI_PCI_ID *) RegionContext;
    PciRegister = (UINT16) (UINT32) Address;

    ACPI_DEBUG_PRINT ((ACPI_DB_INFO,
        "Pci-Config %u (%u) Seg(%04x) Bus(%04x) "
        "Dev(%04x) Func(%04x) Reg(%04x)\n",
        Function, BitWidth, PciId->Segment, PciId->Bus, PciId->Device,
        PciId->Function, PciRegister));

    switch (Function)
    {
    case ACPI_READ:

        *Value = 0;
        Status = AcpiOsReadPciConfiguration (
            PciId, PciRegister, Value, BitWidth);
        break;

    case ACPI_WRITE:

        Status = AcpiOsWritePciConfiguration (
            PciId, PciRegister, *Value, BitWidth);
        break;

    default:

        Status = AE_BAD_PARAMETER;
        break;
    }

    return_ACPI_STATUS (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiExCmosSpaceHandler
 *
 * PARAMETERS:  Function            - Read or Write operation
 *              Address             - Where in the space to read or write
 *              BitWidth            - Field width in bits (8, 16, or 32)
 *              Value               - Pointer to in or out value
 *              HandlerContext      - Pointer to Handler's context
 *              RegionContext       - Pointer to context specific to the
 *                                    accessed region
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Handler for the CMOS address space (Op Region)
 *
 ******************************************************************************/

ACPI_STATUS
AcpiExCmosSpaceHandler (
    UINT32                  Function,
    ACPI_PHYSICAL_ADDRESS   Address,
    UINT32                  BitWidth,
    UINT64                  *Value,
    void                    *HandlerContext,
    void                    *RegionContext)
{
    ACPI_STATUS             Status = AE_OK;


    ACPI_FUNCTION_TRACE (ExCmosSpaceHandler);


    return_ACPI_STATUS (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiExPciBarSpaceHandler
 *
 * PARAMETERS:  Function            - Read or Write operation
 *              Address             - Where in the space to read or write
 *              BitWidth            - Field width in bits (8, 16, or 32)
 *              Value               - Pointer to in or out value
 *              HandlerContext      - Pointer to Handler's context
 *              RegionContext       - Pointer to context specific to the
 *                                    accessed region
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Handler for the PCI BarTarget address space (Op Region)
 *
 ******************************************************************************/

ACPI_STATUS
AcpiExPciBarSpaceHandler (
    UINT32                  Function,
    ACPI_PHYSICAL_ADDRESS   Address,
    UINT32                  BitWidth,
    UINT64                  *Value,
    void                    *HandlerContext,
    void                    *RegionContext)
{
    ACPI_STATUS             Status = AE_OK;


    ACPI_FUNCTION_TRACE (ExPciBarSpaceHandler);


    return_ACPI_STATUS (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiExDataTableSpaceHandler
 *
 * PARAMETERS:  Function            - Read or Write operation
 *              Address             - Where in the space to read or write
 *              BitWidth            - Field width in bits (8, 16, or 32)
 *              Value               - Pointer to in or out value
 *              HandlerContext      - Pointer to Handler's context
 *              RegionContext       - Pointer to context specific to the
 *                                    accessed region
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Handler for the Data Table address space (Op Region)
 *
 ******************************************************************************/

ACPI_STATUS
AcpiExDataTableSpaceHandler (
    UINT32                  Function,
    ACPI_PHYSICAL_ADDRESS   Address,
    UINT32                  BitWidth,
    UINT64                  *Value,
    void                    *HandlerContext,
    void                    *RegionContext)
{
    ACPI_FUNCTION_TRACE (ExDataTableSpaceHandler);


    /*
     * Perform the memory read or write. The BitWidth was already
     * validated.
     */
    switch (Function)
    {
    case ACPI_READ:

        memcpy (ACPI_CAST_PTR (char, Value), ACPI_PHYSADDR_TO_PTR (Address),
            ACPI_DIV_8 (BitWidth));
        break;

    case ACPI_WRITE:

        memcpy (ACPI_PHYSADDR_TO_PTR (Address), ACPI_CAST_PTR (char, Value),
            ACPI_DIV_8 (BitWidth));
        break;

    default:

        return_ACPI_STATUS (AE_BAD_PARAMETER);
    }

    return_ACPI_STATUS (AE_OK);
}
