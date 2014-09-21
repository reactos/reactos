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
    DWORD Pid = (DWORD)(DWORD_PTR)PsGetCurrentProcessId() & 0xFFFFFFFC;
    DWORD check = pEntry->ObjectOwner.ulObj & 0xFFFFFFFE;
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

    DWORD Index = (DWORD)(DWORD_PTR)DdHandle & 0x1FFFFF;
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
            /* FIXME
            if ( (pEntry->Objt == ObjectType ) &&
                 (pEntry->FullUnique == (((DWORD)DdHandle >> 21) & 0x7FF) ) &&
                 (pEntry->pobj->cExclusiveLock == 0) &&
                 (pEntry->pobj->Tid == PsGetCurrentThread()))
               {
                    InterlockedIncrement(&pEntry->pobj->cExclusiveLock);
                    pEntry->pobj->Tid = PsGetCurrentThread();
                    Object = pEntry->pobj;
               }
           */
        }
    }

    if ( !LockOwned )
    {
        EngDeleteSemaphore(ghsemHmgr);
    }

    return Object;
}
