/*
 *  ReactOS W32 Subsystem
 *  Copyright (C) 2005 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           GDI DRIVEROBJ Functions
 * FILE:              subsys/win32k/eng/driverobj.c
 * PROGRAMER:         Gregor Anich
 * REVISION HISTORY:
 *                 04/01/2005: Created
 */

#include <w32k.h>

#define NDEBUG
#include <debug.h>

/*!\brief Called when the process is terminated.
 *
 * Calls the free-proc for each existing DRIVEROBJ.
 *
 * \param Process  Pointer to the EPROCESS struct for the process beeing terminated.
 * \param Win32Process  Pointer to the W32PROCESS
 */
VOID FASTCALL
IntEngCleanupDriverObjs(struct _EPROCESS *Process,
                        PW32PROCESS Win32Process)
{
  PDRIVERGDI DrvObjInt;

  IntEngLockProcessDriverObjs(PsGetCurrentProcessWin32Process());
  while (!IsListEmpty(&Win32Process->DriverObjListHead))
    {
      DrvObjInt = CONTAINING_RECORD(Win32Process->DriverObjListHead.Flink,
                                    DRIVERGDI, ListEntry);
      IntEngUnLockProcessDriverObjs(PsGetCurrentProcessWin32Process());
      EngDeleteDriverObj((HDRVOBJ)(&DrvObjInt->DriverObj), TRUE, FALSE);
      IntEngLockProcessDriverObjs(PsGetCurrentProcessWin32Process());
    }
  IntEngUnLockProcessDriverObjs(PsGetCurrentProcessWin32Process());
}


/*
 * @implemented
 */
HDRVOBJ
STDCALL
EngCreateDriverObj(
	IN PVOID        pvObj,
	IN FREEOBJPROC  pFreeObjProc,
	IN HDEV         hdev
	)
{
  PDRIVERGDI DrvObjInt;
  PDRIVEROBJ DrvObjUser;

  /* Create DRIVEROBJ */
  DrvObjInt = EngAllocMem(0, sizeof (DRIVERGDI), TAG_DRIVEROBJ);
  if (DrvObjInt == NULL)
    {
      DPRINT1("Failed to allocate memory for a DRIVERGDI structure!\n");
      return NULL;
    }

  /* fill user object */
  DrvObjUser = GDIToObj(DrvObjInt, DRIVER);
  DrvObjUser->pvObj = pvObj;
  DrvObjUser->pFreeProc = pFreeObjProc;
  DrvObjUser->hdev = hdev;
  DrvObjUser->dhpdev = ((GDIDEVICE*)hdev)->PDev;

  /* fill internal object */
  ExInitializeFastMutex(&DrvObjInt->Lock);
  IntEngLockProcessDriverObjs(PsGetCurrentProcessWin32Process());
  InsertTailList(&PsGetCurrentProcessWin32Process()->DriverObjListHead, &DrvObjInt->ListEntry);
  IntEngUnLockProcessDriverObjs(PsGetCurrentProcessWin32Process());

  return (HDRVOBJ)DrvObjUser;
}


/*
 * @implemented
 */
BOOL
STDCALL
EngDeleteDriverObj(
	IN HDRVOBJ  hdo,
	IN BOOL  bCallBack,
	IN BOOL  bLocked
	)
{
  PDRIVEROBJ DrvObjUser = (PDRIVEROBJ)hdo;
  PDRIVERGDI DrvObjInt = ObjToGDI(DrvObjUser, DRIVER);
  
  /* Make sure the obj is locked */
  if (!bLocked)
    {
      if (!ExTryToAcquireFastMutex(&DrvObjInt->Lock))
        {
          return FALSE;
        }
    }
    
  /* Call the free-proc */
  if (bCallBack)
    {
      if (!DrvObjUser->pFreeProc(DrvObjUser))
        {
          return FALSE;
        }
    }
  
  /* Free the DRIVEROBJ */
  IntEngLockProcessDriverObjs(PsGetCurrentProcessWin32Process());
  RemoveEntryList(&DrvObjInt->ListEntry);
  IntEngUnLockProcessDriverObjs(PsGetCurrentProcessWin32Process());
  EngFreeMem(DrvObjInt);
  
  return TRUE;
}


/*
 * @implemented
 */
PDRIVEROBJ
STDCALL
EngLockDriverObj( IN HDRVOBJ hdo )
{
  PDRIVEROBJ DrvObjUser = (PDRIVEROBJ)hdo;
  PDRIVERGDI DrvObjInt = ObjToGDI(DrvObjUser, DRIVER);
  
  if (!ExTryToAcquireFastMutex(&DrvObjInt->Lock))
    {
      return NULL;
    }

  return DrvObjUser;
}


/*
 * @implemented
 */
BOOL
STDCALL
EngUnlockDriverObj ( IN HDRVOBJ hdo )
{
  PDRIVERGDI DrvObjInt = ObjToGDI((PDRIVEROBJ)hdo, DRIVER);

  ExReleaseFastMutexUnsafeAndLeaveCriticalRegion(&DrvObjInt->Lock);
  return TRUE;
}

/* EOF */

