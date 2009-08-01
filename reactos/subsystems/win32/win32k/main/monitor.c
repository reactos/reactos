/*
 *  ReactOS W32 Subsystem
 *  Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003, 2004 ReactOS Team
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
 *
 *  COPYRIGHT:        See COPYING in the top level directory
 *  PROJECT:          ReactOS kernel
 *  PURPOSE:          Monitor support
 *  FILE:             subsys/win32k/ntuser/monitor.c
 *  PROGRAMER:        Anich Gregor (blight@blight.eu.org)
 *  REVISION HISTORY:
 *       26-02-2004  Created
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>
#include "object.h"
#include "user.h"
#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

/* list of monitors */
static PMONITOR gMonitorList = NULL;

/* PRIVATE FUNCTIONS **********************************************************/

/* IntCreateMonitorObject
 *
 * Creates a MONITOR struct
 *
 * Return value
 *   If the function succeeds a pointer to a MONITOR struct is returned. On failure
 *   NULL is returned.
 */
static
PMONITOR
IntCreateMonitorObject()
{
   PMONITOR Monitor;

   Monitor = ExAllocatePool( PagedPool, sizeof (MONITOR) );
   if (Monitor == NULL)
   {
      return NULL;
   }

   RtlZeroMemory(Monitor, sizeof (MONITOR));
   Monitor->Handle = (HANDLE)alloc_user_handle(Monitor, USER_MONITOR);

   ExInitializeFastMutex(&Monitor->Lock);

   return Monitor;
}

/* IntDestroyMonitorObject
 *
 * Destroys a MONITOR struct
 * You have to be the owner of the monitors lock to safely destroy it.
 *
 * Arguments
 *
 *   pMonitor
 *      Pointer to the MONITOR struct which shall be deleted
 */
static
void
IntDestroyMonitorObject(IN PMONITOR pMonitor)
{
   RtlFreeUnicodeString(&pMonitor->DeviceName);
   free_user_handle((user_handle_t)pMonitor->Handle);
   //UserDereferenceObject(pMonitor);
}


PMONITOR FASTCALL
UserGetMonitorObject(IN HMONITOR hMonitor)
{
   PMONITOR Monitor;

   if (!hMonitor)
   {
      SetLastWin32Error(ERROR_INVALID_MONITOR_HANDLE);
      return NULL;
   }

   Monitor = (PMONITOR)get_user_object((user_handle_t)hMonitor, USER_MONITOR);
   if (!Monitor)
   {
      SetLastWin32Error(ERROR_INVALID_MONITOR_HANDLE);
      return NULL;
   }

   return Monitor;
}


/* IntAttachMonitor
 *
 * Creates a new MONITOR struct and appends it to the list of monitors.
 *
 * Arguments
 *
 *   pGdiDevice     Pointer to the PDEVOBJ onto which the monitor was attached
 *   DisplayNumber  Display Number (starting with 0)
 *
 * Return value
 *   Returns a NTSTATUS
 */
NTSTATUS
AttachMonitor(IN PDEVOBJ *pGdiDevice,
                 IN ULONG DisplayNumber)
{
   PMONITOR Monitor;
   WCHAR Buffer[CCHDEVICENAME];

   DPRINT("Attaching monitor...\n");

   /* create new monitor object */
   Monitor = IntCreateMonitorObject();
   if (Monitor == NULL)
   {
      DPRINT1("Couldnt create monitor object\n");
      return STATUS_INSUFFICIENT_RESOURCES;
   }

   _snwprintf(Buffer, CCHDEVICENAME, L"\\\\.\\DISPLAY%d", DisplayNumber + 1);
   if (!RtlCreateUnicodeString(&Monitor->DeviceName, Buffer))
   {
      DPRINT1("Couldn't duplicate monitor name!\n");
      //UserDereferenceObject(Monitor);
      //UserDeleteObject(Monitor->Handle, Monitor);
	  free_user_handle((user_handle_t)Monitor->Handle);
      return STATUS_INSUFFICIENT_RESOURCES;
   }

   Monitor->GdiDevice = pGdiDevice;
   if (gMonitorList == NULL)
   {
      DPRINT("Primary monitor is beeing attached\n");
      Monitor->IsPrimary = TRUE;
      gMonitorList = Monitor;
   }
   else
   {
      PMONITOR p;
      DPRINT("Additional monitor is beeing attached\n");
      for (p = gMonitorList; p->Next != NULL; p = p->Next)
         ;
      {
         p->Next = Monitor;
      }
      Monitor->Prev = p;
   }

   return STATUS_SUCCESS;
}

/* IntDetachMonitor
 *
 * Deletes a MONITOR struct and removes it from the list of monitors.
 *
 * Arguments
 *
 *   pGdiDevice  Pointer to the PDEVOBJ from which the monitor was detached
 *
 * Return value
 *   Returns a NTSTATUS
 */
NTSTATUS
DetachMonitor(IN PDEVOBJ *pGdiDevice)
{
   PMONITOR Monitor;

   for (Monitor = gMonitorList; Monitor != NULL; Monitor = Monitor->Next)
   {
      if (Monitor->GdiDevice == pGdiDevice)
         break;
   }

   if (Monitor == NULL)
   {
      /* no monitor for given device found */
      return STATUS_INVALID_PARAMETER;
   }

   if (Monitor->IsPrimary && (Monitor->Next != NULL || Monitor->Prev != NULL))
   {
      PMONITOR NewPrimaryMonitor = (Monitor->Prev != NULL) ? (Monitor->Prev) : (Monitor->Next);

      ExEnterCriticalRegionAndAcquireFastMutexUnsafe(&NewPrimaryMonitor->Lock);
      NewPrimaryMonitor->IsPrimary = TRUE;
      ExReleaseFastMutexUnsafeAndLeaveCriticalRegion(&NewPrimaryMonitor->Lock);
   }

   if (gMonitorList == Monitor)
   {
      gMonitorList = Monitor->Next;
      if (Monitor->Next != NULL)
         Monitor->Next->Prev = NULL;
   }
   else
   {
      Monitor->Prev->Next = Monitor->Next;
      if (Monitor->Next != NULL)
         Monitor->Next->Prev = Monitor->Prev;
   }

   IntDestroyMonitorObject(Monitor);

   return STATUS_SUCCESS;
}

/* PUBLIC FUNCTIONS ***********************************************************/

/* RosUserEnumDisplayMonitors
 *
 * Fills hMonitorList with handles to all available monitors and their rectangles
 *
 */
INT
APIENTRY
RosUserEnumDisplayMonitors(
   OPTIONAL OUT HMONITOR *hMonitorList,
   OPTIONAL OUT PRECTL monitorRectList,
   OPTIONAL IN DWORD listSize)
{
   INT numMonitors = 0,i =0;
   HMONITOR *safeHMonitorList = NULL;
   PMONITOR Monitor;
   PRECTL safeRectList = NULL;
   NTSTATUS Status = STATUS_SUCCESS;

	DPRINT("RosUserEnumDisplayMonitors called\n");

   /* get monitors count */
   for (Monitor = gMonitorList; Monitor != NULL; Monitor = Monitor->Next)
   {
	   numMonitors++;
   }

   if (numMonitors == 0 || listSize == 0 || (hMonitorList == NULL && monitorRectList == NULL))
   {
      return numMonitors;
   }

      safeHMonitorList = ExAllocatePool(PagedPool, sizeof (HMONITOR) * listSize);
      if (safeHMonitorList == NULL)
      {
         /* FIXME: SetLastWin32Error? */
		  DPRINT1("safeHMonitorList == NULL\n");
         return -1;
      }

      safeRectList = ExAllocatePool(PagedPool, sizeof (RECT) * listSize);
      if (safeRectList == NULL)
      {
         ExFreePool(safeHMonitorList);
         /* FIXME: SetLastWin32Error? */
		 DPRINT1("safeRectList == NULL\n");
         return -1;
      }

   for (Monitor = gMonitorList; Monitor != NULL; Monitor = Monitor->Next)
   {
		safeHMonitorList[i] = Monitor->Handle;
		safeRectList[i].left = 0; /* FIXME: get origin */
		safeRectList[i].top = 0; /* FIXME: get origin */
		safeRectList[i].right = safeRectList->left + Monitor->GdiDevice->GDIInfo.ulHorzRes;
		safeRectList[i].bottom = safeRectList->top + Monitor->GdiDevice->GDIInfo.ulVertRes;
	
		i++;
   }

   /* output result */
   if (hMonitorList != NULL)
   {
		_SEH2_TRY
		{
			ProbeForWrite(hMonitorList, sizeof (HMONITOR) * listSize, 1);
			RtlCopyMemory(hMonitorList,safeHMonitorList,sizeof (HMONITOR) * listSize);
		}
		_SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
		{
			Status = _SEH2_GetExceptionCode();
		}
		_SEH2_END;

      if (!NT_SUCCESS(Status))
      {
         ExFreePool(safeHMonitorList);
         SetLastNtError(Status);
         return -1;
      }
   }
   if (monitorRectList != NULL)
   {
	   	_SEH2_TRY
		{
			ProbeForWrite(monitorRectList, sizeof (RECT) * listSize, 1);
			RtlCopyMemory(monitorRectList,safeRectList,sizeof (RECT) * listSize);
		}
		_SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
		{
			Status = _SEH2_GetExceptionCode();
		}
		_SEH2_END;

      ExFreePool(safeRectList);
      if (!NT_SUCCESS(Status))
      {
         SetLastNtError(Status);
         return -1;
      }
   }

   return numMonitors;
}

/* RosUserGetMonitorInfo
 *
 * Retrieves information about a given monitor
 *
 * Arguments
 *
 *   hMonitor
 *      Handle to a monitor for which to get information
 *
 *   pMonitorInfo
 *      Pointer to a MONITORINFO struct which is filled with the information.
 *      The cbSize member must be set to sizeof(MONITORINFO) or
 *      sizeof(MONITORINFOEX). Even if set to sizeof(MONITORINFOEX) only parts
 *      from MONITORINFO will be filled.
 *
 *   pDevice
 *      Pointer to a UNICODE_STRING which will recieve the device's name. The
 *      length should be CCHDEVICENAME
 *      Can be NULL
 *
 * Return value
 *   TRUE on success; FALSE on failure (calls SetLastNtError())
 *
 */
BOOL
APIENTRY
RosUserGetMonitorInfo(
   IN HMONITOR hMonitor,
   OUT LPMONITORINFO pMonitorInfo)
{
   PMONITOR Monitor;
   MONITORINFOEXW MonitorInfo;
   NTSTATUS Status = STATUS_SUCCESS;

   DPRINT("Enter NtUserGetMonitorInfo\n");
   //FIXME: lock

   /* get monitor object */
   if (!(Monitor = UserGetMonitorObject(hMonitor)))
   {
      DPRINT1("Couldnt find monitor 0x%lx\n", hMonitor);
      return FALSE;
   }

   if(pMonitorInfo == NULL)
   {
      SetLastNtError(STATUS_INVALID_PARAMETER);
      return FALSE;
   }

   /* get size of pMonitorInfo */
   _SEH2_TRY
    {
        ProbeForRead(&pMonitorInfo->cbSize, sizeof (MonitorInfo.cbSize), 1);
        RtlCopyMemory(&MonitorInfo.cbSize, &pMonitorInfo->cbSize, sizeof (MonitorInfo.cbSize));
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

   if (!NT_SUCCESS(Status))
   {
      SetLastNtError(Status);
      return FALSE;
   }
   if ((MonitorInfo.cbSize != sizeof (MONITORINFO)) &&
         (MonitorInfo.cbSize != sizeof (MONITORINFOEXW)))
   {
      SetLastNtError(STATUS_INVALID_PARAMETER);
      return FALSE;
   }

   /* fill monitor info */
   MonitorInfo.rcMonitor.left = 0; /* FIXME: get origin */
   MonitorInfo.rcMonitor.top = 0; /* FIXME: get origin */
   MonitorInfo.rcMonitor.right = MonitorInfo.rcMonitor.left + Monitor->GdiDevice->GDIInfo.ulHorzRes;
   MonitorInfo.rcMonitor.bottom = MonitorInfo.rcMonitor.top + Monitor->GdiDevice->GDIInfo.ulVertRes;
   MonitorInfo.rcWork = MonitorInfo.rcMonitor; /* FIXME: use DEVMODE panning to calculate work area? */
   MonitorInfo.dwFlags = 0;

   if (Monitor->IsPrimary)
      MonitorInfo.dwFlags |= MONITORINFOF_PRIMARY;

   /* fill device name */
   if (MonitorInfo.cbSize == sizeof (MONITORINFOEXW))
   {
      WCHAR nul = L'\0';
      INT len = Monitor->DeviceName.Length;
      if (len >= CCHDEVICENAME * sizeof (WCHAR))
         len = (CCHDEVICENAME - 1) * sizeof (WCHAR);

      memcpy(MonitorInfo.szDevice, Monitor->DeviceName.Buffer, len);
      memcpy(MonitorInfo.szDevice + (len / sizeof (WCHAR)), &nul, sizeof (WCHAR));
   }

   /* output data */
	   	_SEH2_TRY
		{
			ProbeForWrite(pMonitorInfo, MonitorInfo.cbSize, 1);
			RtlCopyMemory(pMonitorInfo,&MonitorInfo,MonitorInfo.cbSize);
		}
		_SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
		{
			Status = _SEH2_GetExceptionCode();
		}
		_SEH2_END;

   if (!NT_SUCCESS(Status))
   {
      SetLastNtError(Status);
      return FALSE;
   }

   //FIXME: unlock

   DPRINT("GetMonitorInfo: success\n");

   return TRUE;


}
