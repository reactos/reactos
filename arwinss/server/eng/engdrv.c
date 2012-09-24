/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/eng/engdrv.c
 * PURPOSE:         Driver Support Routines
 * PROGRAMMERS:     Stefan Ginsberg (stefan__100__@hotmail.com)
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>
#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS **********************************************************/

HDRVOBJ
APIENTRY
EngCreateDriverObj(PVOID pvObj,
    FREEOBJPROC  pFreeObjProc,
    HDEV  hdev)
{
    UNIMPLEMENTED;
	return NULL;
}

BOOL
APIENTRY
EngDeleteDriverObj(IN HDRVOBJ  hdo,
                   IN BOOL  bCallBack,
                   IN BOOL  bLocked)
{
    UNIMPLEMENTED;
	return FALSE;
}

PWSTR
APIENTRY
EngGetDriverName(IN HDEV hDev)
{
    UNIMPLEMENTED;
	return NULL;
}

DRIVEROBJ*
APIENTRY
EngLockDriverObj(IN HDRVOBJ hDriverObj)
{
    UNIMPLEMENTED;
	return NULL;
}

BOOL
APIENTRY
EngUnlockDriverObj(IN HDRVOBJ hDriverObj)
{
    UNIMPLEMENTED;
	return FALSE;
}
