/******************************************************************************
 *
 * Module Name: utaddress - OpRegion address range check
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
#include "acnamesp.h"


#define _COMPONENT          ACPI_UTILITIES
        ACPI_MODULE_NAME    ("utaddress")


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtAddAddressRange
 *
 * PARAMETERS:  SpaceId             - Address space ID
 *              Address             - OpRegion start address
 *              Length              - OpRegion length
 *              RegionNode          - OpRegion namespace node
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Add the Operation Region address range to the global list.
 *              The only supported Space IDs are Memory and I/O. Called when
 *              the OpRegion address/length operands are fully evaluated.
 *
 * MUTEX:       Locks the namespace
 *
 * NOTE: Because this interface is only called when an OpRegion argument
 * list is evaluated, there cannot be any duplicate RegionNodes.
 * Duplicate Address/Length values are allowed, however, so that multiple
 * address conflicts can be detected.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiUtAddAddressRange (
    ACPI_ADR_SPACE_TYPE     SpaceId,
    ACPI_PHYSICAL_ADDRESS   Address,
    UINT32                  Length,
    ACPI_NAMESPACE_NODE     *RegionNode)
{
    ACPI_ADDRESS_RANGE      *RangeInfo;
    ACPI_STATUS             Status;


    ACPI_FUNCTION_TRACE (UtAddAddressRange);


    if ((SpaceId != ACPI_ADR_SPACE_SYSTEM_MEMORY) &&
        (SpaceId != ACPI_ADR_SPACE_SYSTEM_IO))
    {
        return_ACPI_STATUS (AE_OK);
    }

    /* Allocate/init a new info block, add it to the appropriate list */

    RangeInfo = ACPI_ALLOCATE (sizeof (ACPI_ADDRESS_RANGE));
    if (!RangeInfo)
    {
        return_ACPI_STATUS (AE_NO_MEMORY);
    }

    RangeInfo->StartAddress = Address;
    RangeInfo->EndAddress = (Address + Length - 1);
    RangeInfo->RegionNode = RegionNode;

    Status = AcpiUtAcquireMutex (ACPI_MTX_NAMESPACE);
    if (ACPI_FAILURE (Status))
    {
        ACPI_FREE (RangeInfo);
        return_ACPI_STATUS (Status);
    }

    RangeInfo->Next = AcpiGbl_AddressRangeList[SpaceId];
    AcpiGbl_AddressRangeList[SpaceId] = RangeInfo;

    ACPI_DEBUG_PRINT ((ACPI_DB_NAMES,
        "\nAdded [%4.4s] address range: 0x%8.8X%8.8X-0x%8.8X%8.8X\n",
        AcpiUtGetNodeName (RangeInfo->RegionNode),
        ACPI_FORMAT_UINT64 (Address),
        ACPI_FORMAT_UINT64 (RangeInfo->EndAddress)));

    (void) AcpiUtReleaseMutex (ACPI_MTX_NAMESPACE);
    return_ACPI_STATUS (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtRemoveAddressRange
 *
 * PARAMETERS:  SpaceId             - Address space ID
 *              RegionNode          - OpRegion namespace node
 *
 * RETURN:      None
 *
 * DESCRIPTION: Remove the Operation Region from the global list. The only
 *              supported Space IDs are Memory and I/O. Called when an
 *              OpRegion is deleted.
 *
 * MUTEX:       Assumes the namespace is locked
 *
 ******************************************************************************/

void
AcpiUtRemoveAddressRange (
    ACPI_ADR_SPACE_TYPE     SpaceId,
    ACPI_NAMESPACE_NODE     *RegionNode)
{
    ACPI_ADDRESS_RANGE      *RangeInfo;
    ACPI_ADDRESS_RANGE      *Prev;


    ACPI_FUNCTION_TRACE (UtRemoveAddressRange);


    if ((SpaceId != ACPI_ADR_SPACE_SYSTEM_MEMORY) &&
        (SpaceId != ACPI_ADR_SPACE_SYSTEM_IO))
    {
        return_VOID;
    }

    /* Get the appropriate list head and check the list */

    RangeInfo = Prev = AcpiGbl_AddressRangeList[SpaceId];
    while (RangeInfo)
    {
        if (RangeInfo->RegionNode == RegionNode)
        {
            if (RangeInfo == Prev) /* Found at list head */
            {
                AcpiGbl_AddressRangeList[SpaceId] = RangeInfo->Next;
            }
            else
            {
                Prev->Next = RangeInfo->Next;
            }

            ACPI_DEBUG_PRINT ((ACPI_DB_NAMES,
                "\nRemoved [%4.4s] address range: 0x%8.8X%8.8X-0x%8.8X%8.8X\n",
                AcpiUtGetNodeName (RangeInfo->RegionNode),
                ACPI_FORMAT_UINT64 (RangeInfo->StartAddress),
                ACPI_FORMAT_UINT64 (RangeInfo->EndAddress)));

            ACPI_FREE (RangeInfo);
            return_VOID;
        }

        Prev = RangeInfo;
        RangeInfo = RangeInfo->Next;
    }

    return_VOID;
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtCheckAddressRange
 *
 * PARAMETERS:  SpaceId             - Address space ID
 *              Address             - Start address
 *              Length              - Length of address range
 *              Warn                - TRUE if warning on overlap desired
 *
 * RETURN:      Count of the number of conflicts detected. Zero is always
 *              returned for Space IDs other than Memory or I/O.
 *
 * DESCRIPTION: Check if the input address range overlaps any of the
 *              ASL operation region address ranges. The only supported
 *              Space IDs are Memory and I/O.
 *
 * MUTEX:       Assumes the namespace is locked.
 *
 ******************************************************************************/

UINT32
AcpiUtCheckAddressRange (
    ACPI_ADR_SPACE_TYPE     SpaceId,
    ACPI_PHYSICAL_ADDRESS   Address,
    UINT32                  Length,
    BOOLEAN                 Warn)
{
    ACPI_ADDRESS_RANGE      *RangeInfo;
    ACPI_PHYSICAL_ADDRESS   EndAddress;
    char                    *Pathname;
    UINT32                  OverlapCount = 0;


    ACPI_FUNCTION_TRACE (UtCheckAddressRange);


    if ((SpaceId != ACPI_ADR_SPACE_SYSTEM_MEMORY) &&
        (SpaceId != ACPI_ADR_SPACE_SYSTEM_IO))
    {
        return_UINT32 (0);
    }

    RangeInfo = AcpiGbl_AddressRangeList[SpaceId];
    EndAddress = Address + Length - 1;

    /* Check entire list for all possible conflicts */

    while (RangeInfo)
    {
        /*
         * Check if the requested address/length overlaps this
         * address range. There are four cases to consider:
         *
         * 1) Input address/length is contained completely in the
         *    address range
         * 2) Input address/length overlaps range at the range start
         * 3) Input address/length overlaps range at the range end
         * 4) Input address/length completely encompasses the range
         */
        if ((Address <= RangeInfo->EndAddress) &&
            (EndAddress >= RangeInfo->StartAddress))
        {
            /* Found an address range overlap */

            OverlapCount++;
            if (Warn)   /* Optional warning message */
            {
                Pathname = AcpiNsGetNormalizedPathname (RangeInfo->RegionNode, TRUE);

                ACPI_WARNING ((AE_INFO,
                    "%s range 0x%8.8X%8.8X-0x%8.8X%8.8X conflicts with OpRegion 0x%8.8X%8.8X-0x%8.8X%8.8X (%s)",
                    AcpiUtGetRegionName (SpaceId),
                    ACPI_FORMAT_UINT64 (Address),
                    ACPI_FORMAT_UINT64 (EndAddress),
                    ACPI_FORMAT_UINT64 (RangeInfo->StartAddress),
                    ACPI_FORMAT_UINT64 (RangeInfo->EndAddress),
                    Pathname));
                ACPI_FREE (Pathname);
            }
        }

        RangeInfo = RangeInfo->Next;
    }

    return_UINT32 (OverlapCount);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtDeleteAddressLists
 *
 * PARAMETERS:  None
 *
 * RETURN:      None
 *
 * DESCRIPTION: Delete all global address range lists (called during
 *              subsystem shutdown).
 *
 ******************************************************************************/

void
AcpiUtDeleteAddressLists (
    void)
{
    ACPI_ADDRESS_RANGE      *Next;
    ACPI_ADDRESS_RANGE      *RangeInfo;
    int                     i;


    /* Delete all elements in all address range lists */

    for (i = 0; i < ACPI_ADDRESS_RANGE_MAX; i++)
    {
        Next = AcpiGbl_AddressRangeList[i];

        while (Next)
        {
            RangeInfo = Next;
            Next = RangeInfo->Next;
            ACPI_FREE (RangeInfo);
        }

        AcpiGbl_AddressRangeList[i] = NULL;
    }
}
