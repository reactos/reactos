

/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Native driver for dxg implementation
 * FILE:             drivers/directx/dxg/ddhmg.c
 * PROGRAMER:        Magnus olsen (magnus@greatlord.com)
 * REVISION HISTORY:
 *       30/12-2007   Magnus Olsen
 */

#include <dxg_int.h>

/* The DdHmgr manger stuff */
ULONG gcSizeDdHmgr =  64 * sizeof(DD_ENTRY);
PDD_ENTRY gpentDdHmgr = NULL;

ULONG gcMaxDdHmgr = 0;
PDD_ENTRY gpentDdHmgrLast = NULL;

HANDLE ghFreeDdHmgr = 0;
HSEMAPHORE ghsemHmgr = NULL;

BOOL
FASTCALL
VerifyObjectOwner(PDD_ENTRY pEntry)
{
    DWORD Pid = (DWORD) PsGetCurrentProcessId() & 0xFFFFFFFC;
    DWORD check = pEntry->ObjectOwner.ulObj & 0xFFFFFFFE;
    return ( (check == Pid) || (!check));
}

/*++
* @name DdHmgCreate
* @implemented
*
* The function DdHmgCreate is internal use in dxg.sys
* It Create all DX kernel object that is need it, for create DX object.
*
* @return
* return FALSE for fail, return TRUE for sussess create DX object
*
* @remarks.
* Only use internal in dxg.sys
*--*/
BOOL
FASTCALL
DdHmgCreate()
{
    gpentDdHmgr = EngAllocMem(FL_ZERO_MEMORY, gcSizeDdHmgr, TAG_THDD);
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
* The function DdHmgDestroy is internal use in dxg.sys
* It destore all DX kernel object
*
* @return
* return FALSE for fail or noting to destore, return TRUE for sussess destore all dx object
*
* @remarks.
* Only use internal in dxg.sys
*--*/
BOOL
FASTCALL
DdHmgDestroy()
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
* The function DdHmgLock is internal use in dxg.sys
* it lock a Dx kernel object
*
* @param HANDLE DdHandle
* The handle we want lock
*
* @param UCHAR ObjectType
* The type of the object we expected the handle contain
* value 0 is for getting ?
* value 1 is for getting EDD_DIRECTDRAW_LOCAL
* value 2 is for getting EDD_SURFACE
* value 3 is for getting ?
* value 4 is for getting EDD_VIDEOPORT
* value 5 is for getting EDD_MOTIONCOMP

* @param BOOLEAN LockOwned
* if it need be EngAcquireSemaphore or not
*
* @return
* return a EDD_* object, or NULL depnes if it success or not.
*
* @remarks.
* Only use internal in dxg.sys
*--*/
PVOID
FASTCALL
DdHmgLock( HANDLE DdHandle, UCHAR ObjectType,  BOOLEAN LockOwned)
{

    DWORD Index = (DWORD)DdHandle & 0x1FFFFF;
    PDD_ENTRY pEntry = NULL;
    PVOID Object = NULL;

    if ( !LockOwned )
    {
        EngAcquireSemaphore(ghsemHmgr);
    }

    if ( Index < gcMaxDdHmgr )
    {
        pEntry = (PDD_ENTRY)((PBYTE)gpentDdHmgr + (sizeof(DD_ENTRY) * Index));
        if ( VerifyObjectOwner(pEntry) )
        {
            if ( (pEntry->Objt == ObjectType ) &&
                 (pEntry->FullUnique == (((DWORD)DdHandle >> 21) & 0x7FF) ) &&
                 (pEntry->pobj->cExclusiveLock == 0) &&
                 (pEntry->pobj->Tid == PsGetCurrentThread()))
               {
                    InterlockedIncrement(&pEntry->pobj->cExclusiveLock);
                    pEntry->pobj->Tid = PsGetCurrentThread();
                    Object = pEntry->pobj;
               }
        }
    }

    if ( !LockOwned )
    {
        EngDeleteSemaphore(ghsemHmgr);
    }

    return Object;
}
