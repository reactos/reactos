/*
 * COPYRIGHT:         GPL, see COPYING in the top level directory
 * PROJECT:           ReactOS win32 kernel mode sunsystem
 * PURPOSE:           GDI DRIVEROBJ Functions
 * FILE:              subsystems/win32k/eng/driverobj.c
 * PROGRAMER:         Timo Kreuzer
 */

/** Includes ******************************************************************/

#include <win32k.h>

#define NDEBUG
#include <debug.h>


/** Internal interface ********************************************************/

/*!
 * \brief DRIVEROBJ cleanup function
 */
VOID
NTAPI
DRIVEROBJ_vCleanup(PVOID pObject)
{
    PEDRIVEROBJ pedo = pObject;
    FREEOBJPROC pFreeProc;

    pFreeProc = pedo->drvobj.pFreeProc;
    if (pFreeProc)
    {
        NT_VERIFY(pFreeProc(&pedo->drvobj));
    }
}

/** Public interface **********************************************************/

_Must_inspect_result_
HDRVOBJ
APIENTRY
EngCreateDriverObj(
    _In_ PVOID       pvObj,
    _In_opt_ FREEOBJPROC pFreeObjProc,
    _In_ HDEV        hdev)
{
    PEDRIVEROBJ pedo;
    HDRVOBJ hdo;
    PDEVOBJ *ppdev = (PDEVOBJ*)hdev;

    /* Allocate a new DRIVEROBJ */
    pedo = DRIVEROBJ_AllocObjectWithHandle();
    if (!pedo)
    {
        return NULL;
    }
    hdo = pedo->baseobj.hHmgr;

    /* Fill in fields */
    pedo->drvobj.pvObj = pvObj;
    pedo->drvobj.pFreeProc = pFreeObjProc;
    pedo->drvobj.hdev = hdev;
    pedo->drvobj.dhpdev = ppdev->dhpdev;

    /* Unlock the object */
    DRIVEROBJ_UnlockObject(pedo);

    /* Return the handle */
    return hdo;
}


BOOL
APIENTRY
EngDeleteDriverObj(
    _In_ _Post_ptr_invalid_ HDRVOBJ hdo,
    _In_ BOOL    bCallBack,
    _In_ BOOL    bLocked)
{
    PEDRIVEROBJ pedo;

    /* Lock the object */
    pedo = DRIVEROBJ_TryLockObject(hdo);
    if (!pedo)
    {
        return FALSE;
    }

    /* Manually call cleanup callback */
    if (bCallBack)
    {
        if (!pedo->drvobj.pFreeProc(&pedo->drvobj))
        {
            /* Callback failed */
            DRIVEROBJ_UnlockObject(pedo);
            return FALSE;
        }
    }

    /* Prevent cleanup callback from being called again */
    pedo->drvobj.pFreeProc = NULL;

    /* Unlock if the caller indicates it is locked */
    if (bLocked)
        DRIVEROBJ_UnlockObject(pedo);

    /* Now delete the object */
    GDIOBJ_vDeleteObject(&pedo->baseobj);
    return TRUE;
}


PDRIVEROBJ
APIENTRY
EngLockDriverObj(
    _In_ HDRVOBJ hdo)
{
    PEDRIVEROBJ pedo;

    /* Lock the object */
    pedo = DRIVEROBJ_TryLockObject(hdo);

    /* Return pointer to the DRIVEROBJ structure */
    return &pedo->drvobj;
}


BOOL
APIENTRY
EngUnlockDriverObj(
    _In_ _Post_ptr_invalid_ HDRVOBJ hdo)
{
    PEDRIVEROBJ pedo;
    ULONG cLocks;

    /* First lock to get a pointer to the object */
    pedo = DRIVEROBJ_TryLockObject(hdo);
    if(!pedo)
    {
        /* Object could not be locked, fail. */
        return FALSE;
    }

    /* Unlock object */
    cLocks = pedo->baseobj.cExclusiveLock;
    DRIVEROBJ_UnlockObject(pedo);

    /* Check if we still hold a lock */
    if (cLocks < 2)
    {
        /* Object wasn't locked before, fail. */
        return FALSE;
    }

    /* Unlock again */
    DRIVEROBJ_UnlockObject(pedo);

    /* Success */
    return TRUE;
}

