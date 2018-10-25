/******************************************************************************
 *
 * Module Name: uttrack - Memory allocation tracking routines (debug only)
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

/*
 * These procedures are used for tracking memory leaks in the subsystem, and
 * they get compiled out when the ACPI_DBG_TRACK_ALLOCATIONS is not set.
 *
 * Each memory allocation is tracked via a doubly linked list. Each
 * element contains the caller's component, module name, function name, and
 * line number. AcpiUtAllocate and AcpiUtAllocateZeroed call
 * AcpiUtTrackAllocation to add an element to the list; deletion
 * occurs in the body of AcpiUtFree.
 */

#include "acpi.h"
#include "accommon.h"

#ifdef ACPI_DBG_TRACK_ALLOCATIONS

#define _COMPONENT          ACPI_UTILITIES
        ACPI_MODULE_NAME    ("uttrack")


/* Local prototypes */

static ACPI_DEBUG_MEM_BLOCK *
AcpiUtFindAllocation (
    ACPI_DEBUG_MEM_BLOCK    *Allocation);

static ACPI_STATUS
AcpiUtTrackAllocation (
    ACPI_DEBUG_MEM_BLOCK    *Address,
    ACPI_SIZE               Size,
    UINT8                   AllocType,
    UINT32                  Component,
    const char              *Module,
    UINT32                  Line);

static ACPI_STATUS
AcpiUtRemoveAllocation (
    ACPI_DEBUG_MEM_BLOCK    *Address,
    UINT32                  Component,
    const char              *Module,
    UINT32                  Line);


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtCreateList
 *
 * PARAMETERS:  CacheName       - Ascii name for the cache
 *              ObjectSize      - Size of each cached object
 *              ReturnCache     - Where the new cache object is returned
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Create a local memory list for tracking purposed
 *
 ******************************************************************************/

ACPI_STATUS
AcpiUtCreateList (
    const char              *ListName,
    UINT16                  ObjectSize,
    ACPI_MEMORY_LIST        **ReturnCache)
{
    ACPI_MEMORY_LIST        *Cache;


    Cache = AcpiOsAllocateZeroed (sizeof (ACPI_MEMORY_LIST));
    if (!Cache)
    {
        return (AE_NO_MEMORY);
    }

    Cache->ListName = ListName;
    Cache->ObjectSize = ObjectSize;

    *ReturnCache = Cache;
    return (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtAllocateAndTrack
 *
 * PARAMETERS:  Size                - Size of the allocation
 *              Component           - Component type of caller
 *              Module              - Source file name of caller
 *              Line                - Line number of caller
 *
 * RETURN:      Address of the allocated memory on success, NULL on failure.
 *
 * DESCRIPTION: The subsystem's equivalent of malloc.
 *
 ******************************************************************************/

void *
AcpiUtAllocateAndTrack (
    ACPI_SIZE               Size,
    UINT32                  Component,
    const char              *Module,
    UINT32                  Line)
{
    ACPI_DEBUG_MEM_BLOCK    *Allocation;
    ACPI_STATUS             Status;


    /* Check for an inadvertent size of zero bytes */

    if (!Size)
    {
        ACPI_WARNING ((Module, Line,
            "Attempt to allocate zero bytes, allocating 1 byte"));
        Size = 1;
    }

    Allocation = AcpiOsAllocate (Size + sizeof (ACPI_DEBUG_MEM_HEADER));
    if (!Allocation)
    {
        /* Report allocation error */

        ACPI_WARNING ((Module, Line,
            "Could not allocate size %u", (UINT32) Size));

        return (NULL);
    }

    Status = AcpiUtTrackAllocation (
        Allocation, Size, ACPI_MEM_MALLOC, Component, Module, Line);
    if (ACPI_FAILURE (Status))
    {
        AcpiOsFree (Allocation);
        return (NULL);
    }

    AcpiGbl_GlobalList->TotalAllocated++;
    AcpiGbl_GlobalList->TotalSize += (UINT32) Size;
    AcpiGbl_GlobalList->CurrentTotalSize += (UINT32) Size;

    if (AcpiGbl_GlobalList->CurrentTotalSize >
        AcpiGbl_GlobalList->MaxOccupied)
    {
        AcpiGbl_GlobalList->MaxOccupied =
            AcpiGbl_GlobalList->CurrentTotalSize;
    }

    return ((void *) &Allocation->UserSpace);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtAllocateZeroedAndTrack
 *
 * PARAMETERS:  Size                - Size of the allocation
 *              Component           - Component type of caller
 *              Module              - Source file name of caller
 *              Line                - Line number of caller
 *
 * RETURN:      Address of the allocated memory on success, NULL on failure.
 *
 * DESCRIPTION: Subsystem equivalent of calloc.
 *
 ******************************************************************************/

void *
AcpiUtAllocateZeroedAndTrack (
    ACPI_SIZE               Size,
    UINT32                  Component,
    const char              *Module,
    UINT32                  Line)
{
    ACPI_DEBUG_MEM_BLOCK    *Allocation;
    ACPI_STATUS             Status;


    /* Check for an inadvertent size of zero bytes */

    if (!Size)
    {
        ACPI_WARNING ((Module, Line,
            "Attempt to allocate zero bytes, allocating 1 byte"));
        Size = 1;
    }

    Allocation = AcpiOsAllocateZeroed (
        Size + sizeof (ACPI_DEBUG_MEM_HEADER));
    if (!Allocation)
    {
        /* Report allocation error */

        ACPI_ERROR ((Module, Line,
            "Could not allocate size %u", (UINT32) Size));
        return (NULL);
    }

    Status = AcpiUtTrackAllocation (Allocation, Size,
        ACPI_MEM_CALLOC, Component, Module, Line);
    if (ACPI_FAILURE (Status))
    {
        AcpiOsFree (Allocation);
        return (NULL);
    }

    AcpiGbl_GlobalList->TotalAllocated++;
    AcpiGbl_GlobalList->TotalSize += (UINT32) Size;
    AcpiGbl_GlobalList->CurrentTotalSize += (UINT32) Size;

    if (AcpiGbl_GlobalList->CurrentTotalSize >
        AcpiGbl_GlobalList->MaxOccupied)
    {
        AcpiGbl_GlobalList->MaxOccupied =
            AcpiGbl_GlobalList->CurrentTotalSize;
    }

    return ((void *) &Allocation->UserSpace);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtFreeAndTrack
 *
 * PARAMETERS:  Allocation          - Address of the memory to deallocate
 *              Component           - Component type of caller
 *              Module              - Source file name of caller
 *              Line                - Line number of caller
 *
 * RETURN:      None
 *
 * DESCRIPTION: Frees the memory at Allocation
 *
 ******************************************************************************/

void
AcpiUtFreeAndTrack (
    void                    *Allocation,
    UINT32                  Component,
    const char              *Module,
    UINT32                  Line)
{
    ACPI_DEBUG_MEM_BLOCK    *DebugBlock;
    ACPI_STATUS             Status;


    ACPI_FUNCTION_TRACE_PTR (UtFree, Allocation);


    if (NULL == Allocation)
    {
        ACPI_ERROR ((Module, Line,
            "Attempt to delete a NULL address"));

        return_VOID;
    }

    DebugBlock = ACPI_CAST_PTR (ACPI_DEBUG_MEM_BLOCK,
        (((char *) Allocation) - sizeof (ACPI_DEBUG_MEM_HEADER)));

    AcpiGbl_GlobalList->TotalFreed++;
    AcpiGbl_GlobalList->CurrentTotalSize -= DebugBlock->Size;

    Status = AcpiUtRemoveAllocation (DebugBlock, Component, Module, Line);
    if (ACPI_FAILURE (Status))
    {
        ACPI_EXCEPTION ((AE_INFO, Status, "Could not free memory"));
    }

    AcpiOsFree (DebugBlock);
    ACPI_DEBUG_PRINT ((ACPI_DB_ALLOCATIONS, "%p freed (block %p)\n",
        Allocation, DebugBlock));
    return_VOID;
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtFindAllocation
 *
 * PARAMETERS:  Allocation              - Address of allocated memory
 *
 * RETURN:      Three cases:
 *              1) List is empty, NULL is returned.
 *              2) Element was found. Returns Allocation parameter.
 *              3) Element was not found. Returns position where it should be
 *                  inserted into the list.
 *
 * DESCRIPTION: Searches for an element in the global allocation tracking list.
 *              If the element is not found, returns the location within the
 *              list where the element should be inserted.
 *
 *              Note: The list is ordered by larger-to-smaller addresses.
 *
 *              This global list is used to detect memory leaks in ACPICA as
 *              well as other issues such as an attempt to release the same
 *              internal object more than once. Although expensive as far
 *              as cpu time, this list is much more helpful for finding these
 *              types of issues than using memory leak detectors outside of
 *              the ACPICA code.
 *
 ******************************************************************************/

static ACPI_DEBUG_MEM_BLOCK *
AcpiUtFindAllocation (
    ACPI_DEBUG_MEM_BLOCK    *Allocation)
{
    ACPI_DEBUG_MEM_BLOCK    *Element;


    Element = AcpiGbl_GlobalList->ListHead;
    if (!Element)
    {
        return (NULL);
    }

    /*
     * Search for the address.
     *
     * Note: List is ordered by larger-to-smaller addresses, on the
     * assumption that a new allocation usually has a larger address
     * than previous allocations.
     */
    while (Element > Allocation)
    {
        /* Check for end-of-list */

        if (!Element->Next)
        {
            return (Element);
        }

        Element = Element->Next;
    }

    if (Element == Allocation)
    {
        return (Element);
    }

    return (Element->Previous);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtTrackAllocation
 *
 * PARAMETERS:  Allocation          - Address of allocated memory
 *              Size                - Size of the allocation
 *              AllocType           - MEM_MALLOC or MEM_CALLOC
 *              Component           - Component type of caller
 *              Module              - Source file name of caller
 *              Line                - Line number of caller
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Inserts an element into the global allocation tracking list.
 *
 ******************************************************************************/

static ACPI_STATUS
AcpiUtTrackAllocation (
    ACPI_DEBUG_MEM_BLOCK    *Allocation,
    ACPI_SIZE               Size,
    UINT8                   AllocType,
    UINT32                  Component,
    const char              *Module,
    UINT32                  Line)
{
    ACPI_MEMORY_LIST        *MemList;
    ACPI_DEBUG_MEM_BLOCK    *Element;
    ACPI_STATUS             Status = AE_OK;


    ACPI_FUNCTION_TRACE_PTR (UtTrackAllocation, Allocation);


    if (AcpiGbl_DisableMemTracking)
    {
        return_ACPI_STATUS (AE_OK);
    }

    MemList = AcpiGbl_GlobalList;
    Status = AcpiUtAcquireMutex (ACPI_MTX_MEMORY);
    if (ACPI_FAILURE (Status))
    {
        return_ACPI_STATUS (Status);
    }

    /*
     * Search the global list for this address to make sure it is not
     * already present. This will catch several kinds of problems.
     */
    Element = AcpiUtFindAllocation (Allocation);
    if (Element == Allocation)
    {
        ACPI_ERROR ((AE_INFO,
            "UtTrackAllocation: Allocation (%p) already present in global list!",
            Allocation));
        goto UnlockAndExit;
    }

    /* Fill in the instance data */

    Allocation->Size = (UINT32) Size;
    Allocation->AllocType = AllocType;
    Allocation->Component = Component;
    Allocation->Line = Line;

    AcpiUtSafeStrncpy (Allocation->Module, (char *) Module, ACPI_MAX_MODULE_NAME);

    if (!Element)
    {
        /* Insert at list head */

        if (MemList->ListHead)
        {
            ((ACPI_DEBUG_MEM_BLOCK *)(MemList->ListHead))->Previous =
                Allocation;
        }

        Allocation->Next = MemList->ListHead;
        Allocation->Previous = NULL;

        MemList->ListHead = Allocation;
    }
    else
    {
        /* Insert after element */

        Allocation->Next = Element->Next;
        Allocation->Previous = Element;

        if (Element->Next)
        {
            (Element->Next)->Previous = Allocation;
        }

        Element->Next = Allocation;
    }


UnlockAndExit:
    Status = AcpiUtReleaseMutex (ACPI_MTX_MEMORY);
    return_ACPI_STATUS (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtRemoveAllocation
 *
 * PARAMETERS:  Allocation          - Address of allocated memory
 *              Component           - Component type of caller
 *              Module              - Source file name of caller
 *              Line                - Line number of caller
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Deletes an element from the global allocation tracking list.
 *
 ******************************************************************************/

static ACPI_STATUS
AcpiUtRemoveAllocation (
    ACPI_DEBUG_MEM_BLOCK    *Allocation,
    UINT32                  Component,
    const char              *Module,
    UINT32                  Line)
{
    ACPI_MEMORY_LIST        *MemList;
    ACPI_STATUS             Status;


    ACPI_FUNCTION_NAME (UtRemoveAllocation);


    if (AcpiGbl_DisableMemTracking)
    {
        return (AE_OK);
    }

    MemList = AcpiGbl_GlobalList;
    if (NULL == MemList->ListHead)
    {
        /* No allocations! */

        ACPI_ERROR ((Module, Line,
            "Empty allocation list, nothing to free!"));

        return (AE_OK);
    }

    Status = AcpiUtAcquireMutex (ACPI_MTX_MEMORY);
    if (ACPI_FAILURE (Status))
    {
        return (Status);
    }

    /* Unlink */

    if (Allocation->Previous)
    {
        (Allocation->Previous)->Next = Allocation->Next;
    }
    else
    {
        MemList->ListHead = Allocation->Next;
    }

    if (Allocation->Next)
    {
        (Allocation->Next)->Previous = Allocation->Previous;
    }

    ACPI_DEBUG_PRINT ((ACPI_DB_ALLOCATIONS, "Freeing %p, size 0%X\n",
        &Allocation->UserSpace, Allocation->Size));

    /* Mark the segment as deleted */

    memset (&Allocation->UserSpace, 0xEA, Allocation->Size);

    Status = AcpiUtReleaseMutex (ACPI_MTX_MEMORY);
    return (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtDumpAllocationInfo
 *
 * PARAMETERS:  None
 *
 * RETURN:      None
 *
 * DESCRIPTION: Print some info about the outstanding allocations.
 *
 ******************************************************************************/

void
AcpiUtDumpAllocationInfo (
    void)
{
/*
    ACPI_MEMORY_LIST        *MemList;
*/

    ACPI_FUNCTION_TRACE (UtDumpAllocationInfo);

/*
    ACPI_DEBUG_PRINT (TRACE_ALLOCATIONS | TRACE_TABLES,
        ("%30s: %4d (%3d Kb)\n", "Current allocations",
        MemList->CurrentCount,
        ROUND_UP_TO_1K (MemList->CurrentSize)));

    ACPI_DEBUG_PRINT (TRACE_ALLOCATIONS | TRACE_TABLES,
        ("%30s: %4d (%3d Kb)\n", "Max concurrent allocations",
        MemList->MaxConcurrentCount,
        ROUND_UP_TO_1K (MemList->MaxConcurrentSize)));


    ACPI_DEBUG_PRINT (TRACE_ALLOCATIONS | TRACE_TABLES,
        ("%30s: %4d (%3d Kb)\n", "Total (all) internal objects",
        RunningObjectCount,
        ROUND_UP_TO_1K (RunningObjectSize)));

    ACPI_DEBUG_PRINT (TRACE_ALLOCATIONS | TRACE_TABLES,
        ("%30s: %4d (%3d Kb)\n", "Total (all) allocations",
        RunningAllocCount,
        ROUND_UP_TO_1K (RunningAllocSize)));


    ACPI_DEBUG_PRINT (TRACE_ALLOCATIONS | TRACE_TABLES,
        ("%30s: %4d (%3d Kb)\n", "Current Nodes",
        AcpiGbl_CurrentNodeCount,
        ROUND_UP_TO_1K (AcpiGbl_CurrentNodeSize)));

    ACPI_DEBUG_PRINT (TRACE_ALLOCATIONS | TRACE_TABLES,
        ("%30s: %4d (%3d Kb)\n", "Max Nodes",
        AcpiGbl_MaxConcurrentNodeCount,
        ROUND_UP_TO_1K ((AcpiGbl_MaxConcurrentNodeCount *
            sizeof (ACPI_NAMESPACE_NODE)))));
*/
    return_VOID;
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtDumpAllocations
 *
 * PARAMETERS:  Component           - Component(s) to dump info for.
 *              Module              - Module to dump info for. NULL means all.
 *
 * RETURN:      None
 *
 * DESCRIPTION: Print a list of all outstanding allocations.
 *
 ******************************************************************************/

void
AcpiUtDumpAllocations (
    UINT32                  Component,
    const char              *Module)
{
    ACPI_DEBUG_MEM_BLOCK    *Element;
    ACPI_DESCRIPTOR         *Descriptor;
    UINT32                  NumOutstanding = 0;
    UINT8                   DescriptorType;


    ACPI_FUNCTION_TRACE (UtDumpAllocations);


    if (AcpiGbl_DisableMemTracking)
    {
        return_VOID;
    }

    /*
     * Walk the allocation list.
     */
    if (ACPI_FAILURE (AcpiUtAcquireMutex (ACPI_MTX_MEMORY)))
    {
        return_VOID;
    }

    if (!AcpiGbl_GlobalList)
    {
        goto Exit;
    }

    Element = AcpiGbl_GlobalList->ListHead;
    while (Element)
    {
        if ((Element->Component & Component) &&
            ((Module == NULL) || (0 == strcmp (Module, Element->Module))))
        {
            Descriptor = ACPI_CAST_PTR (
                ACPI_DESCRIPTOR, &Element->UserSpace);

            if (Element->Size < sizeof (ACPI_COMMON_DESCRIPTOR))
            {
                AcpiOsPrintf ("%p Length 0x%04X %9.9s-%4.4u "
                    "[Not a Descriptor - too small]\n",
                    Descriptor, Element->Size, Element->Module,
                    Element->Line);
            }
            else
            {
                /* Ignore allocated objects that are in a cache */

                if (ACPI_GET_DESCRIPTOR_TYPE (Descriptor) !=
                    ACPI_DESC_TYPE_CACHED)
                {
                    AcpiOsPrintf ("%p Length 0x%04X %9.9s-%4.4u [%s] ",
                        Descriptor, Element->Size, Element->Module,
                        Element->Line, AcpiUtGetDescriptorName (Descriptor));

                    /* Validate the descriptor type using Type field and length */

                    DescriptorType = 0; /* Not a valid descriptor type */

                    switch (ACPI_GET_DESCRIPTOR_TYPE (Descriptor))
                    {
                    case ACPI_DESC_TYPE_OPERAND:

                        if (Element->Size == sizeof (ACPI_OPERAND_OBJECT))
                        {
                            DescriptorType = ACPI_DESC_TYPE_OPERAND;
                        }
                        break;

                    case ACPI_DESC_TYPE_PARSER:

                        if (Element->Size == sizeof (ACPI_PARSE_OBJECT))
                        {
                            DescriptorType = ACPI_DESC_TYPE_PARSER;
                        }
                        break;

                    case ACPI_DESC_TYPE_NAMED:

                        if (Element->Size == sizeof (ACPI_NAMESPACE_NODE))
                        {
                            DescriptorType = ACPI_DESC_TYPE_NAMED;
                        }
                        break;

                    default:

                        break;
                    }

                    /* Display additional info for the major descriptor types */

                    switch (DescriptorType)
                    {
                    case ACPI_DESC_TYPE_OPERAND:

                        AcpiOsPrintf ("%12.12s  RefCount 0x%04X\n",
                            AcpiUtGetTypeName (Descriptor->Object.Common.Type),
                            Descriptor->Object.Common.ReferenceCount);
                        break;

                    case ACPI_DESC_TYPE_PARSER:

                        AcpiOsPrintf ("AmlOpcode 0x%04hX\n",
                            Descriptor->Op.Asl.AmlOpcode);
                        break;

                    case ACPI_DESC_TYPE_NAMED:

                        AcpiOsPrintf ("%4.4s\n",
                            AcpiUtGetNodeName (&Descriptor->Node));
                        break;

                    default:

                        AcpiOsPrintf ( "\n");
                        break;
                    }
                }
            }

            NumOutstanding++;
        }

        Element = Element->Next;
    }

Exit:
    (void) AcpiUtReleaseMutex (ACPI_MTX_MEMORY);

    /* Print summary */

    if (!NumOutstanding)
    {
        ACPI_INFO (("No outstanding allocations"));
    }
    else
    {
        ACPI_ERROR ((AE_INFO, "%u (0x%X) Outstanding cache allocations",
            NumOutstanding, NumOutstanding));
    }

    return_VOID;
}

#endif  /* ACPI_DBG_TRACK_ALLOCATIONS */
