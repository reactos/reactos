/*
 * COPYRIGHT:         GPL, see COPYING in the top level directory
 * PROJECT:           ReactOS win32 kernel mode sunsystem
 * PURPOSE:           GDI DRIVEROBJ Functions
 * FILE:              subsystems/win32k/eng/driverobj.c
 * PROGRAMER:         Timo Kreuzer
 */

/** Includes ******************************************************************/

#include <w32k.h>

#define NDEBUG
#include <debug.h>


/** Internal interface ********************************************************/

/*!
 * \brief DRIVEROBJ cleanup function
 */
BOOL INTERNAL_CALL
DRIVEROBJ_Cleanup(PVOID pObject)
{
    PEDRIVEROBJ pedo = pObject;
    FREEOBJPROC pFreeProc;

    pFreeProc = pedo->drvobj.pFreeProc;
    if (pFreeProc)
    {
        return pFreeProc(pedo->drvobj.pvObj);
    }

    return TRUE;
}

/** Public interface **********************************************************/

HDRVOBJ
APIENTRY
EngCreateDriverObj(
	IN PVOID       pvObj,
	IN FREEOBJPROC pFreeObjProc,
	IN HDEV        hdev)
{
    PEDRIVEROBJ pedo;
    HDRVOBJ hdo;
    GDIDEVICE *ppdev = (GDIDEVICE*)hdev;

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
    pedo->drvobj.dhpdev = ppdev->hPDev;

    /* Unlock the object */
    DRIVEROBJ_UnlockObject(pedo);

    /* Return the handle */
    return hdo;
}


BOOL
APIENTRY
EngDeleteDriverObj(
	IN HDRVOBJ hdo,
	IN BOOL    bCallBack,
	IN BOOL    bLocked)
{
    PEDRIVEROBJ pedo;

    /* Lock the object */
    pedo = DRIVEROBJ_LockObject(hdo);
    if (!pedo)
    {
        return FALSE;
    }

    /* Manually call cleanup callback */
    if (bCallBack)
    {
        if (!pedo->drvobj.pFreeProc(pedo->drvobj.pvObj))
        {
            /* Callback failed */
            DRIVEROBJ_UnlockObject(pedo);
            return FALSE;
        }
    }

    /* Prevent cleanup callback from being called again */
    pedo->drvobj.pFreeProc = NULL;

    /* NOTE: We don't care about the bLocked param, as our handle manager
       allows freeing the object, while we hold any number of locks. */

    /* Free the object */
    return DRIVEROBJ_FreeObjectByHandle(hdo);
}


PDRIVEROBJ
APIENTRY
EngLockDriverObj(
    IN HDRVOBJ hdo)
{
    PEDRIVEROBJ pedo;

    /* Lock the object */
    pedo = DRIVEROBJ_LockObject(hdo);

    /* Return pointer to the DRIVEROBJ structure */
    return &pedo->drvobj;
}


BOOL
APIENTRY
EngUnlockDriverObj(
    IN HDRVOBJ hdo)
{
    PEDRIVEROBJ pedo;

    /* First lock to get a pointer to the object */
    pedo = DRIVEROBJ_LockObject(hdo);
    if(!pedo)
    {
        /* Object could not be locked, fail. */
        return FALSE;
    }

    /* Unlock object */
    DRIVEROBJ_UnlockObject(pedo);

    /* Check if we still hold a lock */
    if (pedo->baseobj.cExclusiveLock < 1)
    {
        /* Object wasn't locked before, fail. */
        return FALSE;
    }

    /* Unlock again */
    DRIVEROBJ_UnlockObject(pedo);

    /* Success */
    return TRUE;
}

