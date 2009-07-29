/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         LGPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/eng/engmisc.c
 * PURPOSE:         Miscellaneous Support Routines
 * PROGRAMMERS:     Aleksey Bragin <aleksey@reactos.org>
 *                  Based on gdi/gdiobj.c from Wine:
 *                  Copyright 1993 Alexandre Julliard
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>
#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS **********************************************************/

#define GDI_HEAP_SIZE 0xffe0

PERESOURCE GDI_resource;

/***********************************************************************
 *           GDI_inc_ref_count
 *
 * Increment the reference count of a GDI object.
 */
HGDIOBJ GDI_inc_ref_count( HGDIOBJ handle )
{
    GDIOBJHDR *header;

    if ((header = GDI_GetObjPtr( handle, 0 )))
    {
        header->selcount++;
        GDI_ReleaseObj( handle );
    }
    else handle = 0;

    return handle;
}


/***********************************************************************
 *           GDI_dec_ref_count
 *
 * Decrement the reference count of a GDI object.
 */
BOOL GDI_dec_ref_count( HGDIOBJ handle )
{
    GDIOBJHDR *header;

    if ((header = GDI_GetObjPtr( handle, 0 )))
    {
        ASSERT(header->selcount);
        if (!--header->selcount && header->deleted)
        {
            /* handle delayed DeleteObject*/
            header->deleted = 0;
            GDI_ReleaseObj( handle );
            DPRINT( "executing delayed DeleteObject for %p\n", handle );
            //DeleteObject( handle );
        }
        else GDI_ReleaseObj( handle );
    }
    return header != NULL;
}


/***********************************************************************
 *           GDI_Init
 *
 * GDI initialization.
 */
BOOLEAN GDIOBJ_Init(void)
{
    NTSTATUS Status;

    /* Initialize GDI resource */
    GDI_resource = ExAllocatePoolWithTag(PagedPool, sizeof(ERESOURCE), TAG_GSEM);
    if (!GDI_resource) return FALSE;
    Status = ExInitializeResourceLite(GDI_resource);
    if (!NT_SUCCESS(Status)) return FALSE;

    /* Initialize handle-mapping */
    GDI_InitHandleMapping();

    /* Return success */
    return TRUE;
}

#define FIRST_LARGE_HANDLE 16
#define MAX_LARGE_HANDLES ((GDI_HEAP_SIZE >> 2) - FIRST_LARGE_HANDLE)
static GDIOBJHDR *large_handles[MAX_LARGE_HANDLES];
static int next_large_handle;

/***********************************************************************
 *           alloc_gdi_handle
 *
 * Allocate a GDI handle for an object, which must have been allocated on the process heap.
 */
HGDIOBJ alloc_gdi_handle( GDIOBJHDR *obj, SHORT type )
{
    int i;

    /* initialize the object header */
    obj->type     = type;
    obj->system   = 0;
    obj->deleted  = 0;
    obj->selcount = 0;

    KeEnterCriticalRegion();
    ExAcquireResourceExclusiveLite(GDI_resource, TRUE);
    for (i = next_large_handle + 1; i < MAX_LARGE_HANDLES; i++)
        if (!large_handles[i]) goto found;
    for (i = 0; i <= next_large_handle; i++)
        if (!large_handles[i]) goto found;
    ExReleaseResourceLite(GDI_resource);
    KeLeaveCriticalRegion();
    return 0;

 found:
    large_handles[i] = obj;
    next_large_handle = i;
    ExReleaseResourceLite(GDI_resource);
    KeLeaveCriticalRegion();
    return (HGDIOBJ)(ULONG_PTR)((i + FIRST_LARGE_HANDLE) << 2);
}


/***********************************************************************
 *           free_gdi_handle
 *
 * Free a GDI handle and return a pointer to the object.
 */
void *free_gdi_handle( HGDIOBJ handle )
{
    GDIOBJHDR *object = NULL;
    int i;

    i = ((ULONG_PTR)handle >> 2) - FIRST_LARGE_HANDLE;
    if (i >= 0 && i < MAX_LARGE_HANDLES)
    {
        KeEnterCriticalRegion();
        ExAcquireResourceExclusiveLite(GDI_resource, TRUE);
        object = large_handles[i];
        large_handles[i] = NULL;
        ExReleaseResourceLite(GDI_resource);
        KeLeaveCriticalRegion();
    }
    if (object)
    {
        object->type  = 0;  /* mark it as invalid */
    }
    return object;
}


/***********************************************************************
 *           GDI_GetObjPtr
 *
 * Return a pointer to the GDI object associated to the handle.
 * Return NULL if the object has the wrong magic number.
 * The object must be released with GDI_ReleaseObj.
 */
void *GDI_GetObjPtr( HGDIOBJ handle, SHORT type )
{
    GDIOBJHDR *ptr = NULL;
    int i;

    KeEnterCriticalRegion();
    ExAcquireResourceExclusiveLite(GDI_resource, TRUE);

    i = ((UINT_PTR)handle >> 2) - FIRST_LARGE_HANDLE;
    if (i >= 0 && i < MAX_LARGE_HANDLES)
    {
        ptr = large_handles[i];
        if (ptr && type && ptr->type != type) ptr = NULL;
    }

    if (!ptr)
    {
        DPRINT1( "Invalid handle %p\n", handle );
    }
    //else TRACE("(%p): enter %d\n", handle, GDI_level.crst.RecursionCount);

    ExReleaseResourceLite(GDI_resource);
    KeLeaveCriticalRegion();

    return ptr;
}


/***********************************************************************
 *           GDI_ReleaseObj
 *
 */
void GDI_ReleaseObj( HGDIOBJ handle )
{
    //TRACE("(%p): leave %d\n", handle, GDI_level.crst.RecursionCount);
    //ExReleaseResourceLite(GDI_resource);
    //KeLeaveCriticalRegion();
}

/* Usermode -> kernelmode handle mapping */
LIST_ENTRY HandleMapping;
KSPIN_LOCK HandleMappingLock;

typedef struct _HMAPPING
{
    HGDIOBJ hUser;
    HGDIOBJ hKernel;
    LIST_ENTRY Entry;
} HMAPPING, *PHMAPPING;

VOID NTAPI
GDI_InitHandleMapping()
{
    /* Initialize handles list and a spinlock */
    InitializeListHead(&HandleMapping);
    KeInitializeSpinLock(&HandleMappingLock);
}

VOID NTAPI
GDI_AddHandleMapping(HGDIOBJ hKernel, HGDIOBJ hUser)
{
    PHMAPPING pMapping = ExAllocatePool(NonPagedPool, sizeof(HMAPPING));
    if (!pMapping) return;

    /* Set mapping */
    pMapping->hUser = hUser;
    pMapping->hKernel = hKernel;

    /* Add it to the list */
    ExInterlockedInsertHeadList(&HandleMapping, &pMapping->Entry, &HandleMappingLock);
}

HGDIOBJ NTAPI
GDI_MapUserHandle(HGDIOBJ hUser)
{
    KIRQL OldIrql;
    PLIST_ENTRY Current;
    PHMAPPING Mapping;
    HGDIOBJ Found = 0;

    /* Acquire the lock and check if the list is empty */
    KeAcquireSpinLock(&HandleMappingLock, &OldIrql);

    /* Traverse the list to find our mapping */
    Current = HandleMapping.Flink;
    while(Current != &HandleMapping)
    {
        Mapping = CONTAINING_RECORD(Current, HMAPPING, Entry);

        /* Check if it's our entry */
        if (Mapping->hUser == hUser)
        {
            /* Found it, save it and break out of the loop */
            Found = Mapping->hKernel;
            break;
        }

        /* Advance to the next pair */
        Current = Current->Flink;
    }

    /* Release the lock and return the entry */
    KeReleaseSpinLock(&HandleMappingLock, OldIrql);
    return Found;
}

VOID NTAPI
GDI_RemoveHandleMapping(HGDIOBJ hUser)
{
    KIRQL OldIrql;
    PLIST_ENTRY Current;
    PHMAPPING Mapping;

    /* Acquire the lock and check if the list is empty */
    KeAcquireSpinLock(&HandleMappingLock, &OldIrql);

    /* Traverse the list to find our mapping */
    Current = HandleMapping.Flink;
    while(Current != &HandleMapping)
    {
        Mapping = CONTAINING_RECORD(Current, HMAPPING, Entry);

        /* Check if it's our entry */
        if (Mapping->hUser == hUser)
        {
            /* Remove and free it */
            RemoveEntryList(Current);
            ExFreePool(Mapping);
            break;
        }

        /* Advance to the next pair */
        Current = Current->Flink;
    }

    /* Release the lock and return the entry */
    KeReleaseSpinLock(&HandleMappingLock, OldIrql);
}

