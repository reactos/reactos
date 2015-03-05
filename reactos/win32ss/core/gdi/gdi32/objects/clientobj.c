/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS WIN32 subsystem
 * PURPOSE:         Support functions for GDI client objects
 * PROGRAMMER:      Timo Kreuzer (timo.kreuzer@reactos.org)
 */

#include <precomp.h>

CRITICAL_SECTION gcsClientObjLinks;
ULONG gcClientObj;

typedef struct _CLIENTOBJLINK
{
    struct _CLIENTOBJLINK *pcolNext;
    HGDIOBJ hobj;
    PVOID pvObj;
} CLIENTOBJLINK, *PCLIENTOBJLINK;

PCLIENTOBJLINK gapcolHashTable[127];

BOOL
WINAPI
GdiCreateClientObjLink(
    _In_ HGDIOBJ hobj,
    _In_ PVOID pvObject)
{
    PCLIENTOBJLINK pcol;
    ULONG iHashIndex;

    /* Allocate a link structure */
    pcol = HeapAlloc(GetProcessHeap(), 0, sizeof(*pcol));
    if (pcol == NULL)
    {
        return FALSE;
    }

    /* Setup the link structure */
    pcol->hobj = hobj;
    pcol->pvObj = pvObject;

    /* Calculate the hash index */
    iHashIndex = (ULONG_PTR)hobj % _countof(gapcolHashTable);

    /* Enter the critical section */
    EnterCriticalSection(&gcsClientObjLinks);

    /* Insert the link structure */
    pcol->pcolNext = gapcolHashTable[iHashIndex];
    gapcolHashTable[iHashIndex] = pcol;
    gcClientObj++;

    /* Leave the critical section */
    LeaveCriticalSection(&gcsClientObjLinks);

    return TRUE;
}

PVOID
WINAPI
GdiGetClientObjLink(
    _In_ HGDIOBJ hobj)
{
    ULONG iHashIndex;
    PCLIENTOBJLINK pcol;
    PVOID pvObject = NULL;

    /* Calculate the hash index */
    iHashIndex = (ULONG_PTR)hobj % _countof(gapcolHashTable);

    /* Enter the critical section */
    EnterCriticalSection(&gcsClientObjLinks);

    /* Loop the link entries in this hash bucket */
    pcol = gapcolHashTable[iHashIndex];
    while (pcol != NULL)
    {
        /* Check if this is the object we want */
        if (pcol->hobj == hobj)
        {
            /* Get the object pointer and bail out */
            pvObject = pcol->pvObj;
            break;
        }

        /* Go to the next entry */
        pcol = pcol->pcolNext;
    }

    /* Leave the critical section */
    LeaveCriticalSection(&gcsClientObjLinks);

    return pvObject;
}

PVOID
WINAPI
GdiRemoveClientObjLink(
    _In_ HGDIOBJ hobj)
{
    PCLIENTOBJLINK pcol, *ppcol;
    ULONG iHashIndex;
    PVOID pvObject = NULL;

    /* Calculate the hash index */
    iHashIndex = (ULONG_PTR)hobj % _countof(gapcolHashTable);

    /* Enter the critical section */
    EnterCriticalSection(&gcsClientObjLinks);

    /* Loop the link entries in this hash bucket */
    ppcol = &gapcolHashTable[iHashIndex];
    while (*ppcol != NULL)
    {
        /* Get the current client object link */
        pcol = *ppcol;

        /* Check if this is the one we want */
        if (pcol->hobj == hobj)
        {
            /* Update the link pointer, removing this link */
            *ppcol = pcol->pcolNext;
            gcClientObj--;

            /* Get the object pointer */
            pvObject = pcol->pvObj;

            /* Free the link structure */
            HeapFree(GetProcessHeap(), 0, pcol);

            /* We're done */
            break;
        }

        /* Go to the next link pointer */
        ppcol = &(pcol->pcolNext);
    }

    /* Leave the critical section */
    LeaveCriticalSection(&gcsClientObjLinks);

    /* Return the object pointer, or NULL if we did not find it */
    return pvObject;
}

HGDIOBJ
WINAPI
GdiCreateClientObj(
    _In_ PVOID pvObject,
    _In_ GDILOOBJTYPE eObjType)
{
    HGDIOBJ hobj;

    /* Call win32k to create a client object handle */
    hobj = NtGdiCreateClientObj(eObjType);
    if (hobj == NULL)
    {
        return NULL;
    }

    /* Create the client object link */
    if (!GdiCreateClientObjLink(hobj, pvObject))
    {
        NtGdiDeleteClientObj(hobj);
        return NULL;
    }

    return hobj;
}

PVOID
WINAPI
GdiDeleteClientObj(
    _In_ HGDIOBJ hobj)
{
    PVOID pvObject;

    /* Remove the client object link */
    pvObject = GdiRemoveClientObjLink(hobj);
    if (pvObject == NULL)
    {
        return NULL;
    }

    /* Call win32k to delete the handle */
    if (!NtGdiDeleteClientObj(hobj))
    {
        ASSERT(FALSE);
    }

    return pvObject;
}
