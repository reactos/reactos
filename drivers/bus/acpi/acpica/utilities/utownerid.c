/*******************************************************************************
 *
 * Module Name: utownerid - Support for Table/Method Owner IDs
 *
 ******************************************************************************/

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
        ACPI_MODULE_NAME    ("utownerid")


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtAllocateOwnerId
 *
 * PARAMETERS:  OwnerId         - Where the new owner ID is returned
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Allocate a table or method owner ID. The owner ID is used to
 *              track objects created by the table or method, to be deleted
 *              when the method exits or the table is unloaded.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiUtAllocateOwnerId (
    ACPI_OWNER_ID           *OwnerId)
{
    UINT32                  i;
    UINT32                  j;
    UINT32                  k;
    ACPI_STATUS             Status;


    ACPI_FUNCTION_TRACE (UtAllocateOwnerId);


    /* Guard against multiple allocations of ID to the same location */

    if (*OwnerId)
    {
        ACPI_ERROR ((AE_INFO,
            "Owner ID [0x%3.3X] already exists", *OwnerId));
        return_ACPI_STATUS (AE_ALREADY_EXISTS);
    }

    /* Mutex for the global ID mask */

    Status = AcpiUtAcquireMutex (ACPI_MTX_CACHES);
    if (ACPI_FAILURE (Status))
    {
        return_ACPI_STATUS (Status);
    }

    /*
     * Find a free owner ID, cycle through all possible IDs on repeated
     * allocations. (ACPI_NUM_OWNERID_MASKS + 1) because first index
     * may have to be scanned twice.
     */
    for (i = 0, j = AcpiGbl_LastOwnerIdIndex;
         i < (ACPI_NUM_OWNERID_MASKS + 1);
         i++, j++)
    {
        if (j >= ACPI_NUM_OWNERID_MASKS)
        {
            j = 0;  /* Wraparound to start of mask array */
        }

        for (k = AcpiGbl_NextOwnerIdOffset; k < 32; k++)
        {
            if (AcpiGbl_OwnerIdMask[j] == ACPI_UINT32_MAX)
            {
                /* There are no free IDs in this mask */

                break;
            }

            /*
             * Note: the UINT32 cast ensures that 1 is stored as a unsigned
             * integer. Omitting the cast may result in 1 being stored as an
             * int. Some compilers or runtime error detection may flag this as
             * an error.
             */
            if (!(AcpiGbl_OwnerIdMask[j] & ((UINT32) 1 << k)))
            {
                /*
                 * Found a free ID. The actual ID is the bit index plus one,
                 * making zero an invalid Owner ID. Save this as the last ID
                 * allocated and update the global ID mask.
                 */
                AcpiGbl_OwnerIdMask[j] |= ((UINT32) 1 << k);

                AcpiGbl_LastOwnerIdIndex = (UINT8) j;
                AcpiGbl_NextOwnerIdOffset = (UINT8) (k + 1);

                /*
                 * Construct encoded ID from the index and bit position
                 *
                 * Note: Last [j].k (bit 4095) is never used and is marked
                 * permanently allocated (prevents +1 overflow)
                 */
                *OwnerId = (ACPI_OWNER_ID) ((k + 1) + ACPI_MUL_32 (j));

                ACPI_DEBUG_PRINT ((ACPI_DB_VALUES,
                    "Allocated OwnerId: 0x%3.3X\n", (unsigned int) *OwnerId));
                goto Exit;
            }
        }

        AcpiGbl_NextOwnerIdOffset = 0;
    }

    /*
     * All OwnerIds have been allocated. This typically should
     * not happen since the IDs are reused after deallocation. The IDs are
     * allocated upon table load (one per table) and method execution, and
     * they are released when a table is unloaded or a method completes
     * execution.
     *
     * If this error happens, there may be very deep nesting of invoked
     * control methods, or there may be a bug where the IDs are not released.
     */
    Status = AE_OWNER_ID_LIMIT;
    ACPI_ERROR ((AE_INFO,
        "Could not allocate new OwnerId (4095 max), AE_OWNER_ID_LIMIT"));

Exit:
    (void) AcpiUtReleaseMutex (ACPI_MTX_CACHES);
    return_ACPI_STATUS (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtReleaseOwnerId
 *
 * PARAMETERS:  OwnerIdPtr          - Pointer to a previously allocated OwnerID
 *
 * RETURN:      None. No error is returned because we are either exiting a
 *              control method or unloading a table. Either way, we would
 *              ignore any error anyway.
 *
 * DESCRIPTION: Release a table or method owner ID. Valid IDs are 1 - 255
 *
 ******************************************************************************/

void
AcpiUtReleaseOwnerId (
    ACPI_OWNER_ID           *OwnerIdPtr)
{
    ACPI_OWNER_ID           OwnerId = *OwnerIdPtr;
    ACPI_STATUS             Status;
    UINT32                  Index;
    UINT32                  Bit;


    ACPI_FUNCTION_TRACE_U32 (UtReleaseOwnerId, OwnerId);


    /* Always clear the input OwnerId (zero is an invalid ID) */

    *OwnerIdPtr = 0;

    /* Zero is not a valid OwnerID */

    if (OwnerId == 0)
    {
        ACPI_ERROR ((AE_INFO, "Invalid OwnerId: 0x%3.3X", OwnerId));
        return_VOID;
    }

    /* Mutex for the global ID mask */

    Status = AcpiUtAcquireMutex (ACPI_MTX_CACHES);
    if (ACPI_FAILURE (Status))
    {
        return_VOID;
    }

    /* Normalize the ID to zero */

    OwnerId--;

    /* Decode ID to index/offset pair */

    Index = ACPI_DIV_32 (OwnerId);
    Bit = (UINT32) 1 << ACPI_MOD_32 (OwnerId);

    /* Free the owner ID only if it is valid */

    if (AcpiGbl_OwnerIdMask[Index] & Bit)
    {
        AcpiGbl_OwnerIdMask[Index] ^= Bit;
    }
    else
    {
        ACPI_ERROR ((AE_INFO,
            "Attempted release of non-allocated OwnerId: 0x%3.3X", OwnerId + 1));
    }

    (void) AcpiUtReleaseMutex (ACPI_MTX_CACHES);
    return_VOID;
}
