/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Native driver for dxg implementation
 * FILE:             win32ss/reactx/dxg/ddhmg.c
 * PROGRAMER:        Magnus olsen (magnus@greatlord.com)
 *                   Sebastian Gasiorek (sebastian.gasiorek@reactos.org)
 * REVISION HISTORY:
 *       30/12-2007   Magnus Olsen
 */

#include <dxg_int.h>

/* The DdHmgr manger stuff */
ULONG gcSizeDdHmgr =  1024;
PDD_ENTRY gpentDdHmgr = NULL;

ULONG gcMaxDdHmgr = 0;
PDD_ENTRY gpentDdHmgrLast = NULL;

/* next free ddhmg handle number available to reuse */
ULONG ghFreeDdHmgr = 0;
HSEMAPHORE ghsemHmgr = NULL;

BOOL
FASTCALL
VerifyObjectOwner(PDD_ENTRY pEntry)
{
    DWORD Pid = (DWORD)(DWORD_PTR)PsGetCurrentProcessId() & 0xFFFFFFFC;
    DWORD check = (DWORD)pEntry->Pid & 0xFFFFFFFE;
    return ( (check == Pid) || (!check));
}

/*++
* @name DdHmgCreate
* @implemented
*
* The function DdHmgCreate is used internally in dxg.sys
* It creates all DX kernel objects that are need it for creation of DX objects.
*
* @return
* Return FALSE for failure and TRUE for success in creating the DX object
*
* @remarks.
* Only used internally in dxg.sys
*--*/
BOOL
FASTCALL
DdHmgCreate(VOID)
{
    gpentDdHmgr = EngAllocMem(FL_ZERO_MEMORY, gcSizeDdHmgr * sizeof(DD_ENTRY), TAG_THDD);
    ghFreeDdHmgr = 0;
    gcMaxDdHmgr = 1;

    if (gpentDdHmgr)
    {
        ghsemHmgr = EngCreateSemaphore();

        if (ghsemHmgr)
        {
            gpLockShortDelay = EngAllocMem(FL_ZERO_MEMORY | FL_NONPAGED_MEMORY, sizeof(LARGE_INTEGER), TAG_GINI);

            if (gpLockShortDelay)
            {
                gpLockShortDelay->HighPart = -1;
                return TRUE;
            }

            EngDeleteSemaphore(ghsemHmgr);
            ghsemHmgr = NULL;
        }

        EngFreeMem(gpentDdHmgr);
        gpentDdHmgr = NULL;
    }

    return FALSE;
}

/*++
* @name DdHmgDestroy
* @implemented
*
* The function DdHmgDestroy is used internally in dxg.sys
* It destroys all DX kernel objects
*
* @return
* Always returns true, as a failure here would result in a BSOD.
*
* @remarks.
* Only used internally in dxg.sys
*--*/
BOOL
FASTCALL
DdHmgDestroy(VOID)
{
    gcMaxDdHmgr = 0;
    gcSizeDdHmgr = 0;
    ghFreeDdHmgr = 0;
    gpentDdHmgrLast = NULL;

    if (gpentDdHmgr)
    {
        EngFreeMem(gpentDdHmgr);
        gpentDdHmgr = NULL;
    }

    if (ghsemHmgr)
    {
        EngDeleteSemaphore(ghsemHmgr);
        ghsemHmgr = NULL;
    }

    return TRUE;
}

/*++
* @name DdHmgLock
* @implemented
*
* The function DdHmgLock is used internally in dxg.sys
* It locks a DX kernel object
*
* @param HANDLE DdHandle
* The handle we want locked
*
* @param UCHAR ObjectType
* The type of the object we expected the handle to contain
* value 0 is for ?
* value 1 is for EDD_DIRECTDRAW_LOCAL
* value 2 is for EDD_SURFACE
* value 3 is for ?
* value 4 is for EDD_VIDEOPORT
* value 5 is for EDD_MOTIONCOMP

* @param BOOLEAN LockOwned
* If it needs to call EngAcquireSemaphore or not
*
* @return
* Returns an EDD_* object, or NULL if it fails
*
* @remarks.
* Only used internally in dxg.sys
*--*/
PVOID
FASTCALL
DdHmgLock(HANDLE DdHandle, UCHAR ObjectType, BOOLEAN LockOwned)
{
    DWORD Index = DDHMG_HTOI(DdHandle);

    PDD_ENTRY pEntry = NULL;
    PVOID Object = NULL;

    if ( !LockOwned )
    {
        EngAcquireSemaphore(ghsemHmgr);
    }

    if ( Index < gcMaxDdHmgr )
    {
        pEntry = (PDD_ENTRY)((PLONG)gpentDdHmgr + (sizeof(DD_ENTRY) * Index));

        if ( VerifyObjectOwner(pEntry) )
        {
            if ( ( pEntry->Objt == ObjectType ) &&
                 ( pEntry->FullUnique == (((ULONG)DdHandle >> 21) & 0x7FF) ) &&
                 ( !pEntry->pobj->cExclusiveLock ) )
            {
                InterlockedIncrement((VOID*)&pEntry->pobj->cExclusiveLock);
                pEntry->pobj->Tid = KeGetCurrentThread();
                Object = pEntry->pobj;
            }
        }
    }

    if ( !LockOwned )
    {
        EngReleaseSemaphore(ghsemHmgr);
    }

    return Object;
}

/*++
* @name DdAllocateObject
* @implemented
*
* The function DdAllocateObject is used internally in dxg.sys
* It allocates memory for a DX kernel object
*
* @param UINT32 oSize
* Size of memory to be allocated
* @param UCHAR oType
* Object type
* @param BOOLEAN oZeroMemory
* Zero memory
*
* @remarks.
* Only used internally in dxg.sys
*/
PVOID
FASTCALL
DdAllocateObject(ULONG objSize, UCHAR objType, BOOLEAN objZeroMemory)
{
    PVOID pObject = NULL;

    if (objZeroMemory)
        pObject = EngAllocMem(FL_ZERO_MEMORY, objSize, ((ULONG)objType << 24) + TAG_DH_0);
    else
        pObject = EngAllocMem(0, objSize, ((ULONG)objType << 24) + TAG_DH_0);

    if (!pObject)
    {
        EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
    }

    return pObject;
}

/*++
* @name DdFreeObject
* @implemented
*
* The function DdFreeObject is used internally in dxg.sys
* It frees memory of DX kernel object
*
* @param PVOID pObject
* Object memory to be freed
*
* @remarks.
* Only used internally in dxg.sys
*/
VOID
FASTCALL
DdFreeObject(PVOID pObject)
{
    EngFreeMem(pObject);
}


/*++
* @name DdGetFreeHandle
* @implemented
*
* The function DdGetFreeHandle is used internally in dxg.sys
* It allocates new handle for specified object type
*
* @param UCHAR oType
* Object type
*
* @return
* Returns handle or 0 if it fails.
*
* @remarks.
* Only used internally in dxg.sys
*--*/
HANDLE
FASTCALL
DdGetFreeHandle(UCHAR objType)
{
    PVOID mAllocMem = NULL;
    ULONG mAllocEntries = 0;
    PDD_ENTRY pEntry = NULL;
    ULONG retVal;
    ULONG index;

    // check if memory is allocated
    if (!gpentDdHmgr)
        return 0;

    // check if we reached maximum handle index
    if (gcMaxDdHmgr == DDHMG_HANDLE_LIMIT)
        return 0;

    // check if we have free handle to reuse
    if (ghFreeDdHmgr)
    {
       index = ghFreeDdHmgr;
       pEntry = (PDD_ENTRY)((PLONG)gpentDdHmgr + (sizeof(DD_ENTRY) * index));

       // put next free index to our global variable
       ghFreeDdHmgr = pEntry->NextFree;             

       // build handle 
       pEntry->FullUnique = objType | 8;
       retVal = (pEntry->FullUnique << 21) | index;
       return (HANDLE)retVal;
    }

    // if all pre-allocated memory is already used then allocate more
    if (gcSizeDdHmgr == gcMaxDdHmgr)
    {
        // allocate buffer for next 1024 handles
        mAllocEntries = gcSizeDdHmgr + 1024;
        mAllocMem = EngAllocMem(FL_ZERO_MEMORY, sizeof(DD_ENTRY) * (mAllocEntries), TAG_THDD);
        if (!mAllocMem)
            return 0;

        memmove(&mAllocMem, gpentDdHmgr, sizeof(DD_ENTRY) * gcSizeDdHmgr);
        gcSizeDdHmgr = mAllocEntries;
        gpentDdHmgrLast = gpentDdHmgr;
        EngFreeMem(gpentDdHmgr);
        gpentDdHmgr = mAllocMem;
    }

    pEntry = (PDD_ENTRY)((PLONG)gpentDdHmgr + (sizeof(DD_ENTRY) * gcMaxDdHmgr));

    // build handle 
    pEntry->FullUnique = objType | 8;
    retVal = (pEntry->FullUnique << 21) | gcMaxDdHmgr;
    gcMaxDdHmgr = gcMaxDdHmgr + 1;

    return (HANDLE)retVal;
}

/*++
* @name DdHmgAlloc
* @implemented
*
* The function DdHmgAlloc is used internally in dxg.sys
* It allocates object
*
* @param ULONG objSize
* Size of memory to be allocated
* @param CHAR objType
* Object type
* @param UINT objLock
* Object lock flag
*
* @return
* Handle if object is not locked by objLock
* Object if lock is set in objLock
* 0 if it fails.
*
* @remarks.
* Only used internally in dxg.sys
*--*/
HANDLE
FASTCALL
DdHmgAlloc(ULONG objSize, CHAR objType, BOOLEAN objLock)
{
    PVOID pObject = NULL;
    HANDLE DdHandle = NULL;
    PDD_ENTRY pEntry = NULL;
    DWORD Index;

    pObject = DdAllocateObject(objSize, objType, TRUE);
    if (!pObject)
        return 0;

    EngAcquireSemaphore(ghsemHmgr);

    /* Get next free handle */
    DdHandle = DdGetFreeHandle(objType);

    if (DdHandle)
    {
        Index = DDHMG_HTOI(DdHandle);

        pEntry = (PDD_ENTRY)((PLONG)gpentDdHmgr + (sizeof(DD_ENTRY) * Index));

        pEntry->pobj = pObject;
        pEntry->Objt = objType;

        pEntry->Pid = (HANDLE)(((ULONG)PsGetCurrentProcessId() & 0xFFFFFFFC) | ((ULONG)(pEntry->Pid) & 1));

        if (objLock)
        {
            InterlockedIncrement((VOID*)&pEntry->pobj->cExclusiveLock);
            pEntry->pobj->Tid = KeGetCurrentThread();
        }
        pEntry->pobj->hHmgr = DdHandle;
        
        EngReleaseSemaphore(ghsemHmgr);

        /* Return handle if object not locked */
        if (!objLock)
           return DdHandle;

        return (HANDLE)pEntry;
    }

    EngReleaseSemaphore(ghsemHmgr);
    DdFreeObject(pObject);
    return 0;
}

/*++
* @name DdHmgFree
* @implemented
*
* The function DdHmgFree is used internally in dxg.sys
* It frees DX object and memory allocated to it
*
* @param HANDLE DdHandle
* DX object handle
*
* @remarks.
* Only used internally in dxg.sys
*--*/
VOID
FASTCALL
DdHmgFree(HANDLE DdHandle)
{
    PDD_ENTRY pEntry = NULL;

    DWORD Index = DDHMG_HTOI(DdHandle);

    EngAcquireSemaphore(ghsemHmgr);

    pEntry = (PDD_ENTRY)((PLONG)gpentDdHmgr + (sizeof(DD_ENTRY) * Index));

    // check if we have object that should be freed
    if (pEntry->pobj)
        DdFreeObject(pEntry->pobj);

    pEntry->NextFree = ghFreeDdHmgr;

    // reset process ID
    pEntry->Pid = (HANDLE)((DWORD)pEntry->Pid & 1);
    ghFreeDdHmgr = Index;

    EngReleaseSemaphore(ghsemHmgr);
}
