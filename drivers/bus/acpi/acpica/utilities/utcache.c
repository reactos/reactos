/******************************************************************************
 *
 * Module Name: utcache - local cache allocation routines
 *
 *****************************************************************************/

/******************************************************************************
 *
 * 1. Copyright Notice
 *
 * Some or all of this work - Copyright (c) 1999 - 2009, Intel Corp.
 * All rights reserved.
 *
 * 2. License
 *
 * 2.1. This is your license from Intel Corp. under its intellectual property
 * rights.  You may have additional license terms from the party that provided
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
 * to or modifications of the Original Intel Code.  No other license or right
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
 * and the following Disclaimer and Export Compliance provision.  In addition,
 * Licensee must cause all Covered Code to which Licensee contributes to
 * contain a file documenting the changes Licensee made to create that Covered
 * Code and the date of any change.  Licensee must include in that file the
 * documentation of any changes made by any predecessor Licensee.  Licensee
 * must include a prominent statement that the modification is derived,
 * directly or indirectly, from Original Intel Code.
 *
 * 3.2. Redistribution of Source with no Rights to Further Distribute Source.
 * Redistribution of source code of any substantial portion of the Covered
 * Code or modification without rights to further distribute source must
 * include the following Disclaimer and Export Compliance provision in the
 * documentation and/or other materials provided with distribution.  In
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
 * HERE.  ANY SOFTWARE ORIGINATING FROM INTEL OR DERIVED FROM INTEL SOFTWARE
 * IS PROVIDED "AS IS," AND INTEL WILL NOT PROVIDE ANY SUPPORT,  ASSISTANCE,
 * INSTALLATION, TRAINING OR OTHER SERVICES.  INTEL WILL NOT PROVIDE ANY
 * UPDATES, ENHANCEMENTS OR EXTENSIONS.  INTEL SPECIFICALLY DISCLAIMS ANY
 * IMPLIED WARRANTIES OF MERCHANTABILITY, NONINFRINGEMENT AND FITNESS FOR A
 * PARTICULAR PURPOSE.
 *
 * 4.2. IN NO EVENT SHALL INTEL HAVE ANY LIABILITY TO LICENSEE, ITS LICENSEES
 * OR ANY OTHER THIRD PARTY, FOR ANY LOST PROFITS, LOST DATA, LOSS OF USE OR
 * COSTS OF PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES, OR FOR ANY INDIRECT,
 * SPECIAL OR CONSEQUENTIAL DAMAGES ARISING OUT OF THIS AGREEMENT, UNDER ANY
 * CAUSE OF ACTION OR THEORY OF LIABILITY, AND IRRESPECTIVE OF WHETHER INTEL
 * HAS ADVANCE NOTICE OF THE POSSIBILITY OF SUCH DAMAGES.  THESE LIMITATIONS
 * SHALL APPLY NOTWITHSTANDING THE FAILURE OF THE ESSENTIAL PURPOSE OF ANY
 * LIMITED REMEDY.
 *
 * 4.3. Licensee shall not export, either directly or indirectly, any of this
 * software or system incorporating such software without first obtaining any
 * required license or other approval from the U. S. Department of Commerce or
 * any other agency or department of the United States Government.  In the
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

#define __UTCACHE_C__

#include "acpi.h"
#include "accommon.h"

#define _COMPONENT          ACPI_UTILITIES
        ACPI_MODULE_NAME    ("utcache")


#ifdef ACPI_USE_LOCAL_CACHE
/*******************************************************************************
 *
 * FUNCTION:    AcpiOsCreateCache
 *
 * PARAMETERS:  CacheName       - Ascii name for the cache
 *              ObjectSize      - Size of each cached object
 *              MaxDepth        - Maximum depth of the cache (in objects)
 *              ReturnCache     - Where the new cache object is returned
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Create a cache object
 *
 ******************************************************************************/

ACPI_STATUS
AcpiOsCreateCache (
    char                    *CacheName,
    UINT16                  ObjectSize,
    UINT16                  MaxDepth,
    ACPI_MEMORY_LIST        **ReturnCache)
{
    ACPI_MEMORY_LIST        *Cache;


    ACPI_FUNCTION_ENTRY ();


    if (!CacheName || !ReturnCache || (ObjectSize < 16))
    {
        return (AE_BAD_PARAMETER);
    }

    /* Create the cache object */

    Cache = AcpiOsAllocate (sizeof (ACPI_MEMORY_LIST));
    if (!Cache)
    {
        return (AE_NO_MEMORY);
    }

    /* Populate the cache object and return it */

    ACPI_MEMSET (Cache, 0, sizeof (ACPI_MEMORY_LIST));
    Cache->LinkOffset = 8;
    Cache->ListName   = CacheName;
    Cache->ObjectSize = ObjectSize;
    Cache->MaxDepth   = MaxDepth;

    *ReturnCache = Cache;
    return (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiOsPurgeCache
 *
 * PARAMETERS:  Cache           - Handle to cache object
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Free all objects within the requested cache.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiOsPurgeCache (
    ACPI_MEMORY_LIST        *Cache)
{
    char                    *Next;
    ACPI_STATUS             Status;


    ACPI_FUNCTION_ENTRY ();


    if (!Cache)
    {
        return (AE_BAD_PARAMETER);
    }

    Status = AcpiUtAcquireMutex (ACPI_MTX_CACHES);
    if (ACPI_FAILURE (Status))
    {
        return (Status);
    }

    /* Walk the list of objects in this cache */

    while (Cache->ListHead)
    {
        /* Delete and unlink one cached state object */

        Next = *(ACPI_CAST_INDIRECT_PTR (char,
                    &(((char *) Cache->ListHead)[Cache->LinkOffset])));
        ACPI_FREE (Cache->ListHead);

        Cache->ListHead = Next;
        Cache->CurrentDepth--;
    }

    (void) AcpiUtReleaseMutex (ACPI_MTX_CACHES);
    return (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiOsDeleteCache
 *
 * PARAMETERS:  Cache           - Handle to cache object
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Free all objects within the requested cache and delete the
 *              cache object.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiOsDeleteCache (
    ACPI_MEMORY_LIST        *Cache)
{
    ACPI_STATUS             Status;


    ACPI_FUNCTION_ENTRY ();


   /* Purge all objects in the cache */

    Status = AcpiOsPurgeCache (Cache);
    if (ACPI_FAILURE (Status))
    {
        return (Status);
    }

    /* Now we can delete the cache object */

    AcpiOsFree (Cache);
    return (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiOsReleaseObject
 *
 * PARAMETERS:  Cache       - Handle to cache object
 *              Object      - The object to be released
 *
 * RETURN:      None
 *
 * DESCRIPTION: Release an object to the specified cache.  If cache is full,
 *              the object is deleted.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiOsReleaseObject (
    ACPI_MEMORY_LIST        *Cache,
    void                    *Object)
{
    ACPI_STATUS             Status;


    ACPI_FUNCTION_ENTRY ();


    if (!Cache || !Object)
    {
        return (AE_BAD_PARAMETER);
    }

    /* If cache is full, just free this object */

    if (Cache->CurrentDepth >= Cache->MaxDepth)
    {
        ACPI_FREE (Object);
        ACPI_MEM_TRACKING (Cache->TotalFreed++);
    }

    /* Otherwise put this object back into the cache */

    else
    {
        Status = AcpiUtAcquireMutex (ACPI_MTX_CACHES);
        if (ACPI_FAILURE (Status))
        {
            return (Status);
        }

        /* Mark the object as cached */

        ACPI_MEMSET (Object, 0xCA, Cache->ObjectSize);
        ACPI_SET_DESCRIPTOR_TYPE (Object, ACPI_DESC_TYPE_CACHED);

        /* Put the object at the head of the cache list */

        * (ACPI_CAST_INDIRECT_PTR (char,
            &(((char *) Object)[Cache->LinkOffset]))) = Cache->ListHead;
        Cache->ListHead = Object;
        Cache->CurrentDepth++;

        (void) AcpiUtReleaseMutex (ACPI_MTX_CACHES);
    }

    return (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiOsAcquireObject
 *
 * PARAMETERS:  Cache           - Handle to cache object
 *
 * RETURN:      the acquired object.  NULL on error
 *
 * DESCRIPTION: Get an object from the specified cache.  If cache is empty,
 *              the object is allocated.
 *
 ******************************************************************************/

void *
AcpiOsAcquireObject (
    ACPI_MEMORY_LIST        *Cache)
{
    ACPI_STATUS             Status;
    void                    *Object;


    ACPI_FUNCTION_NAME (OsAcquireObject);


    if (!Cache)
    {
        return (NULL);
    }

    Status = AcpiUtAcquireMutex (ACPI_MTX_CACHES);
    if (ACPI_FAILURE (Status))
    {
        return (NULL);
    }

    ACPI_MEM_TRACKING (Cache->Requests++);

    /* Check the cache first */

    if (Cache->ListHead)
    {
        /* There is an object available, use it */

        Object = Cache->ListHead;
        Cache->ListHead = *(ACPI_CAST_INDIRECT_PTR (char,
                                &(((char *) Object)[Cache->LinkOffset])));

        Cache->CurrentDepth--;

        ACPI_MEM_TRACKING (Cache->Hits++);
        ACPI_DEBUG_PRINT ((ACPI_DB_EXEC,
            "Object %p from %s cache\n", Object, Cache->ListName));

        Status = AcpiUtReleaseMutex (ACPI_MTX_CACHES);
        if (ACPI_FAILURE (Status))
        {
            return (NULL);
        }

        /* Clear (zero) the previously used Object */

        ACPI_MEMSET (Object, 0, Cache->ObjectSize);
    }
    else
    {
        /* The cache is empty, create a new object */

        ACPI_MEM_TRACKING (Cache->TotalAllocated++);

#ifdef ACPI_DBG_TRACK_ALLOCATIONS
        if ((Cache->TotalAllocated - Cache->TotalFreed) > Cache->MaxOccupied)
        {
            Cache->MaxOccupied = Cache->TotalAllocated - Cache->TotalFreed;
        }
#endif

        /* Avoid deadlock with ACPI_ALLOCATE_ZEROED */

        Status = AcpiUtReleaseMutex (ACPI_MTX_CACHES);
        if (ACPI_FAILURE (Status))
        {
            return (NULL);
        }

        Object = ACPI_ALLOCATE_ZEROED (Cache->ObjectSize);
        if (!Object)
        {
            return (NULL);
        }
    }

    return (Object);
}
#endif /* ACPI_USE_LOCAL_CACHE */


