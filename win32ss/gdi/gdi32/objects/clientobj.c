/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS WIN32 subsystem
 * PURPOSE:         Support functions for GDI client objects
 * PROGRAMMER:      Timo Kreuzer (timo.kreuzer@reactos.org)
 */

#include <precomp.h>

CRITICAL_SECTION gcsClientObjLinks;

typedef struct _CLIENTOBJLINK
{
    struct _CLIENTOBJLINK *pcolNext;
    HGDIOBJ hobj;
    PVOID pvObj;
} CLIENTOBJLINK, *PCLIENTOBJLINK;

PCLIENTOBJLINK gapcolHashTable[127];

HGDIOBJ
WINAPI
GdiInsertClientObj(
    _In_ PVOID pvObject,
    _In_ GDILOOBJTYPE eObjType)
{
    PCLIENTOBJLINK pcol;
    ULONG iHashIndex;
    HGDIOBJ hobj;

    /* Call win32k to create a client object handle */
    hobj = NtGdiCreateClientObj(eObjType);
    if (hobj == NULL)
    {
        return NULL;
    }

    /* Calculate the hash index */
    iHashIndex = (ULONG_PTR)hobj % _countof(gapcolHashTable);

    /* Allocate a link structure */
    pcol = HeapAlloc(GetProcessHeap(), 0, sizeof(*pcol));
    if (pcol == NULL)
    {
        NtGdiDeleteClientObj(hobj);
        return NULL;
    }

    /* Setup the link structure */
    pcol->hobj = hobj;
    pcol->pvObj = pvObject;

    /* Enter the critical section */
    EnterCriticalSection(&gcsClientObjLinks);

    /* Insert the link structure */
    pcol->pcolNext = gapcolHashTable[iHashIndex];
    gapcolHashTable[iHashIndex] = pcol;

    /* Leave the critical section */
    LeaveCriticalSection(&gcsClientObjLinks);

    return hobj;
}

PVOID
WINAPI
GdiGetClientObject(
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
GdiRemoveClientObject(
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
