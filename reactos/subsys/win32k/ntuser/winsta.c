/*
 *  ReactOS W32 Subsystem
 *  Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003 ReactOS Team
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
 *  $Id: winsta.c,v 1.58 2004/05/01 17:06:55 weiden Exp $
 *
 *  COPYRIGHT:        See COPYING in the top level directory
 *  PROJECT:          ReactOS kernel
 *  PURPOSE:          Window stations
 *  FILE:             subsys/win32k/ntuser/winsta.c
 *  PROGRAMER:        Casper S. Hornstrup (chorns@users.sourceforge.net)
 *  REVISION HISTORY:
 *       06-06-2001  CSH  Created
 *  NOTES:            Exported functions set the Win32 last error value
 *                    on errors. The value can be retrieved with the Win32
 *                    function GetLastError().
 *  TODO:             The process window station is created on
 *                    the first USER32/GDI32 call not related
 *                    to window station/desktop handling
 */

/* INCLUDES ******************************************************************/

#define __WIN32K__
#define NTOS_MODE_KERNEL
#include <ntos.h>
#include <ddk/ntddmou.h>
#include <win32k/win32k.h>
#include <include/winsta.h>
#include <include/desktop.h>
#include <include/object.h>
#include <include/window.h>
#include <include/error.h>
#include <include/cursoricon.h>
#include <include/hotkey.h>
#include <include/color.h>
#include <include/mouse.h>
#include <include/callback.h>
#include <include/guicheck.h>
#include <include/intgdi.h>
#include <include/tags.h>
/* Needed for DIRECTORY_OBJECT */
#include <internal/ob.h>

#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

/* Currently active window station */
PWINSTATION_OBJECT InputWindowStation = NULL;

/* INITALIZATION FUNCTIONS ****************************************************/

NTSTATUS FASTCALL
InitWindowStationImpl(VOID)
{
   OBJECT_ATTRIBUTES ObjectAttributes;
   HANDLE WindowStationsDirectory;
   UNICODE_STRING UnicodeString;
   NTSTATUS Status;
  
   /*
    * Create the '\Windows\WindowStations' directory
    */

   RtlInitUnicodeString(&UnicodeString, WINSTA_ROOT_NAME);
   InitializeObjectAttributes(&ObjectAttributes, &UnicodeString,
      0, NULL, NULL);
   Status = ZwCreateDirectoryObject(&WindowStationsDirectory, 0,
      &ObjectAttributes);
   if (!NT_SUCCESS(Status))
   {
      DPRINT("Could not create \\Windows\\WindowStations directory "
	     "(Status 0x%X)\n", Status);
      return Status;
   }

   return STATUS_SUCCESS;
}

NTSTATUS FASTCALL
CleanupWindowStationImpl(VOID)
{
   return STATUS_SUCCESS;
}

/* PRIVATE FUNCTIONS **********************************************************/

/*
 * IntGetFullWindowStationName
 *
 * Get a full desktop object name from a name specified in 
 * NtUserCreateWindowStation, NtUserOpenWindowStation, NtUserCreateDesktop
 * or NtUserOpenDesktop.
 *
 * Return Value
 *    TRUE on success, FALSE on failure.
 */

BOOL FASTCALL
IntGetFullWindowStationName(
   OUT PUNICODE_STRING FullName,
   IN PUNICODE_STRING WinStaName,
   IN OPTIONAL PUNICODE_STRING DesktopName)
{
   PWCHAR Buffer;

   FullName->Length = WINSTA_ROOT_NAME_LENGTH * sizeof(WCHAR);
   if (WinStaName != NULL)
      FullName->Length += WinStaName->Length + sizeof(WCHAR);
   if (DesktopName != NULL)
      FullName->Length += DesktopName->Length + sizeof(WCHAR);
   FullName->Buffer = ExAllocatePoolWithTag(NonPagedPool, FullName->Length, TAG_STRING);
   if (FullName->Buffer == NULL)
   {
      return FALSE;
   }

   Buffer = FullName->Buffer;
   memcpy(Buffer, WINSTA_ROOT_NAME, WINSTA_ROOT_NAME_LENGTH * sizeof(WCHAR));
   Buffer += WINSTA_ROOT_NAME_LENGTH;
   if (WinStaName != NULL)
   {
      memcpy(Buffer, L"\\", sizeof(WCHAR));
      Buffer ++;
      memcpy(Buffer, WinStaName->Buffer, WinStaName->Length);

      if (DesktopName != NULL)
      {
         Buffer += WinStaName->Length / sizeof(WCHAR);
         memcpy(Buffer, L"\\", sizeof(WCHAR));
         Buffer ++;
         memcpy(Buffer, DesktopName->Buffer, DesktopName->Length);
      }
   }

   return TRUE;
}   

/*
 * IntValidateWindowStationHandle
 *
 * Validates the window station handle.
 *
 * Remarks
 *    If the function succeeds, the handle remains referenced. If the
 *    fucntion fails, last error is set.
 */

NTSTATUS FASTCALL
IntValidateWindowStationHandle(
   HWINSTA WindowStation,
   KPROCESSOR_MODE AccessMode,
   ACCESS_MASK DesiredAccess,
   PWINSTATION_OBJECT *Object)
{
   NTSTATUS Status;
  
   Status = ObReferenceObjectByHandle(
      WindowStation,
      DesiredAccess,
      ExWindowStationObjectType,
      AccessMode,
      (PVOID*)Object,
      NULL);

   if (!NT_SUCCESS(Status))
      SetLastNtError(Status);

   return Status;
}

BOOL FASTCALL
IntGetWindowStationObject(PWINSTATION_OBJECT Object)
{
   NTSTATUS Status;

   Status = ObReferenceObjectByPointer(
      Object,
      KernelMode,
      ExWindowStationObjectType,
      0);
  
   return NT_SUCCESS(Status);
}

BOOL FASTCALL
IntInitializeDesktopGraphics(VOID)
{
  UNICODE_STRING DriverName;
  if (! IntCreatePrimarySurface())
    {
      return FALSE;
    }
  RtlInitUnicodeString(&DriverName, L"DISPLAY");
  ScreenDeviceContext = IntGdiCreateDC(&DriverName, NULL, NULL, NULL);
  if (NULL == ScreenDeviceContext)
    {
      IntDestroyPrimarySurface();
      return FALSE;
    }
  DC_SetOwnership(ScreenDeviceContext, NULL);
  
  EnableMouse(ScreenDeviceContext);

  /* not the best place to load the cursors but it's good for now */
  IntLoadDefaultCursors(FALSE);
  NtUserAcquireOrReleaseInputOwnership(FALSE);

  return TRUE;
}

VOID FASTCALL
IntEndDesktopGraphics(VOID)
{
  NtUserAcquireOrReleaseInputOwnership(TRUE);
  EnableMouse(FALSE);
  if (NULL != ScreenDeviceContext)
    {
      DC_SetOwnership(ScreenDeviceContext, PsGetCurrentProcess());
      NtGdiDeleteDC(ScreenDeviceContext);
      ScreenDeviceContext = NULL;
    }
  IntHideDesktop(IntGetActiveDesktop());
  IntDestroyPrimarySurface();
}

HDC FASTCALL
IntGetScreenDC(VOID)
{
   return ScreenDeviceContext;
}

/* PUBLIC FUNCTIONS ***********************************************************/

/*
 * NtUserCreateWindowStation
 *
 * Creates a new window station.
 *
 * Parameters
 *    lpszWindowStationName
 *       Pointer to a null-terminated string specifying the name of the
 *       window station to be created. Window station names are
 *       case-insensitive and cannot contain backslash characters (\).
 *       Only members of the Administrators group are allowed to specify a
 *       name.
 *
 *    dwDesiredAccess
 *       Requested type of access
 *
 *    lpSecurity
 *       Security descriptor
 *
 *    Unknown3, Unknown4, Unknown5
 *       Unused
 *
 * Return Value
 *    If the function succeeds, the return value is a handle to the newly
 *    created window station. If the specified window station already
 *    exists, the function succeeds and returns a handle to the existing
 *    window station. If the function fails, the return value is NULL.
 *
 * Todo
 *    Correct the prototype to match the Windows one (with 7 parameters
 *    on Windows XP).
 *
 * Status
 *    @implemented
 */

HWINSTA STDCALL
NtUserCreateWindowStation(
   PUNICODE_STRING lpszWindowStationName,
   ACCESS_MASK dwDesiredAccess,
   LPSECURITY_ATTRIBUTES lpSecurity,
   DWORD Unknown3,
   DWORD Unknown4,
   DWORD Unknown5)
{
   UNICODE_STRING WindowStationName;
   PWINSTATION_OBJECT WindowStationObject;
   HWINSTA WindowStation;
   OBJECT_ATTRIBUTES ObjectAttributes;
   NTSTATUS Status;

   /*
    * Generate full window station name
    */

   if (!IntGetFullWindowStationName(&WindowStationName, lpszWindowStationName,
       NULL))
   {
      SetLastNtError(STATUS_INSUFFICIENT_RESOURCES);
      return 0;
   }

   /*
    * Try to open already existing window station
    */
   
   DPRINT("Trying to open window station (%wZ)\n", &WindowStationName);

   /* Initialize ObjectAttributes for the window station object */
   InitializeObjectAttributes(
      &ObjectAttributes,
      &WindowStationName,
      0,
      NULL,
      NULL);

   Status = ObOpenObjectByName(
      &ObjectAttributes,
      ExWindowStationObjectType,
      NULL,
      UserMode,
      dwDesiredAccess,
      NULL,
      (PVOID*)&WindowStation);

   if (NT_SUCCESS(Status))
   {
      DPRINT("Successfully opened window station (%wZ)\n", WindowStationName);
      ExFreePool(WindowStationName.Buffer);
      return (HWINSTA)WindowStation;
   }
  
   /*
    * No existing window station found, try to create new one
    */

   DPRINT("Creating window station (%wZ)\n", &WindowStationName);

   Status = ObCreateObject(
      ExGetPreviousMode(),
      ExWindowStationObjectType,
      &ObjectAttributes,
      ExGetPreviousMode(),
      NULL,
      sizeof(WINSTATION_OBJECT),
      0,
      0,
      (PVOID*)&WindowStationObject);

   if (!NT_SUCCESS(Status))
   {
      DPRINT("Failed creating window station (%wZ)\n", &WindowStationName);
      ExFreePool(WindowStationName.Buffer);
      SetLastNtError(STATUS_INSUFFICIENT_RESOURCES);
      return 0;
   }

   Status = ObInsertObject(
      (PVOID)WindowStationObject,
      NULL,
      STANDARD_RIGHTS_REQUIRED,
      0,
      NULL,
      (PVOID*)&WindowStation);

   if (!NT_SUCCESS(Status))
   {
      DPRINT("Failed creating window station (%wZ)\n", &WindowStationName);
      ExFreePool(WindowStationName.Buffer);
      SetLastNtError(STATUS_INSUFFICIENT_RESOURCES);
      ObDereferenceObject(WindowStationObject);
      return 0;
   }

   /*
    * Initialize the new window station object
    */

   WindowStationObject->HandleTable = ObmCreateHandleTable();
   if (!WindowStationObject->HandleTable)
   {
      DPRINT("Failed creating handle table\n");
      ExFreePool(WindowStationName.Buffer);
      ObDereferenceObject(WindowStationObject);
      SetLastNtError(STATUS_INSUFFICIENT_RESOURCES);
      return 0;
   }
  
   InitHotKeys(WindowStationObject);
  
   ExInitializeFastMutex(&WindowStationObject->SystemCursor.CursorMutex);
   WindowStationObject->SystemCursor.Enabled = FALSE;
   WindowStationObject->SystemCursor.ButtonsDown = 0;
   WindowStationObject->SystemCursor.x = (LONG)0;
   WindowStationObject->SystemCursor.y = (LONG)0;
   WindowStationObject->SystemCursor.CursorClipInfo.IsClipped = FALSE;
   WindowStationObject->SystemCursor.LastBtnDown = 0;
   WindowStationObject->SystemCursor.CurrentCursorObject = NULL;
   WindowStationObject->SystemCursor.ShowingCursor = 0;
  
   /* FIXME: Obtain the following information from the registry */
   WindowStationObject->SystemCursor.SwapButtons = FALSE;
   WindowStationObject->SystemCursor.SafetySwitch = FALSE;
   WindowStationObject->SystemCursor.SafetyRemoveCount = 0;
   WindowStationObject->SystemCursor.DblClickSpeed = 500;
   WindowStationObject->SystemCursor.DblClickWidth = 4;
   WindowStationObject->SystemCursor.DblClickHeight = 4;
  
   if (!IntSetupCurIconHandles(WindowStationObject))
   {
       DPRINT1("Setting up the Cursor/Icon Handle table failed!\n");
       /* FIXME: Complain more loudly? */
   }
  
   DPRINT("Window station successfully created (%wZ)\n", &WindowStationName);
   ExFreePool(WindowStationName.Buffer);

   return WindowStation;
}

/*
 * NtUserOpenWindowStation
 *
 * Opens an existing window station.
 *
 * Parameters
 *    lpszWindowStationName
 *       Name of the existing window station.
 *
 *    dwDesiredAccess
 *       Requested type of access.
 *
 * Return Value
 *    If the function succeeds, the return value is the handle to the
 *    specified window station. If the function fails, the return value
 *    is NULL.
 *
 * Remarks
 *    The returned handle can be closed with NtUserCloseWindowStation.
 *
 * Status
 *    @implemented
 */

HWINSTA STDCALL
NtUserOpenWindowStation(
   PUNICODE_STRING lpszWindowStationName,
   ACCESS_MASK dwDesiredAccess)
{
   UNICODE_STRING WindowStationName;
   HWINSTA WindowStation;
   OBJECT_ATTRIBUTES ObjectAttributes;
   NTSTATUS Status;
  
   /*
    * Generate full window station name
    */

   if (!IntGetFullWindowStationName(&WindowStationName, lpszWindowStationName,
       NULL))
   {
      SetLastNtError(STATUS_INSUFFICIENT_RESOURCES);
      return 0;
   }

   DPRINT("Trying to open window station (%wZ)\n", &WindowStationName);

   /* Initialize ObjectAttributes for the window station object */
   InitializeObjectAttributes(
      &ObjectAttributes,
      &WindowStationName,
      0,
      NULL,
      NULL);

   Status = ObOpenObjectByName(
      &ObjectAttributes,
      ExDesktopObjectType,
      NULL,
      UserMode,
      dwDesiredAccess,
      NULL,
      (PVOID*)&WindowStation);

   if (!NT_SUCCESS(Status))
   {
      SetLastNtError(Status);
      ExFreePool(WindowStationName.Buffer);
      return 0;
   }

   DPRINT("Successfully opened window station (%wZ)\n", &WindowStationName);
   ExFreePool(WindowStationName.Buffer);

   return WindowStation;
}

/*
 * NtUserCloseWindowStation
 *
 * Closes a window station handle.
 *
 * Parameters
 *    hWinSta
 *       Handle to the window station.
 *
 * Return Value
 *    Status
 *
 * Remarks
 *    The window station handle can be created with NtUserCreateWindowStation
 *    or NtUserOpenWindowStation. Attemps to close a handle to the window
 *    station assigned to the calling process will fail.
 *
 * Status
 *    @implemented
 */

BOOL
STDCALL
NtUserCloseWindowStation(
   HWINSTA hWinSta)
{
   PWINSTATION_OBJECT Object;
   NTSTATUS Status;

   DPRINT("About to close window station handle (0x%X)\n", hWinSta);

   Status = IntValidateWindowStationHandle(
      hWinSta,
      KernelMode,
      0,
      &Object);

   if (!NT_SUCCESS(Status))
   {
      DPRINT("Validation of window station handle (0x%X) failed\n", hWinSta);
      return FALSE;
   }

   ObDereferenceObject(Object);

   DPRINT("Closing window station handle (0x%X)\n", hWinSta);

   Status = ZwClose(hWinSta);
   if (!NT_SUCCESS(Status))
   {
      SetLastNtError(Status);
      return FALSE;
   }

   return TRUE;
}

/*
 * NtUserGetObjectInformation
 *
 * The NtUserGetObjectInformation function retrieves information about a
 * window station or desktop object.
 *
 * Parameters
 *    hObj 
 *       Handle to the window station or desktop object for which to
 *       return information. This can be a handle of type HDESK or HWINSTA
 *       (for example, a handle returned by NtUserCreateWindowStation,
 *       NtUserOpenWindowStation, NtUserCreateDesktop, or NtUserOpenDesktop). 
 *
 *    nIndex 
 *       Specifies the object information to be retrieved.
 *
 *    pvInfo 
 *       Pointer to a buffer to receive the object information. 
 *
 *    nLength 
 *       Specifies the size, in bytes, of the buffer pointed to by the
 *       pvInfo parameter. 
 *
 *    lpnLengthNeeded 
 *       Pointer to a variable receiving the number of bytes required to
 *       store the requested information. If this variable's value is
 *       greater than the value of the nLength parameter when the function
 *       returns, the function returns FALSE, and none of the information
 *       is copied to the pvInfo buffer. If the value of the variable pointed
 *       to by lpnLengthNeeded is less than or equal to the value of nLength,
 *       the entire information block is copied. 
 *
 * Return Value
 *    If the function succeeds, the return value is nonzero. If the function
 *    fails, the return value is zero.
 *
 * Status
 *    @unimplemented
 */

BOOL STDCALL
NtUserGetObjectInformation(
   HANDLE hObject,
   DWORD nIndex,
   PVOID pvInformation,
   DWORD nLength,
   PDWORD nLengthNeeded)
{
   SetLastNtError(STATUS_UNSUCCESSFUL);
   return FALSE;
}

/*
 * NtUserSetObjectInformation
 *
 * The NtUserSetObjectInformation function sets information about a
 * window station or desktop object.
 *
 * Parameters
 *    hObj 
 *       Handle to the window station or desktop object for which to set
 *       object information. This value can be a handle of type HDESK or
 *       HWINSTA. 
 *
 *    nIndex 
 *       Specifies the object information to be set.
 *
 *    pvInfo 
 *       Pointer to a buffer containing the object information. 
 *
 *    nLength 
 *       Specifies the size, in bytes, of the information contained in the
 *       buffer pointed to by pvInfo. 
 *
 * Return Value
 *    If the function succeeds, the return value is nonzero. If the function
 *    fails the return value is zero.
 *
 * Status
 *    @unimplemented
 */

BOOL
STDCALL
NtUserSetObjectInformation(
   HANDLE hObject,
   DWORD nIndex,
   PVOID pvInformation,
   DWORD nLength)
{
   /* FIXME: ZwQueryObject */
   /* FIXME: ZwSetInformationObject */
   SetLastNtError(STATUS_UNSUCCESSFUL);
   return FALSE;
}

/*
 * NtUserGetProcessWindowStation
 *
 * Returns a handle to the current process window station.
 *
 * Return Value
 *    If the function succeeds, the return value is handle to the window
 *    station assigned to the current process. If the function fails, the
 *    return value is NULL.
 *
 * Status
 *    @implemented
 */

HWINSTA STDCALL
NtUserGetProcessWindowStation(VOID)
{
   return PROCESS_WINDOW_STATION();
}

/*
 * NtUserSetProcessWindowStation
 *
 * Assigns a window station to the current process.
 *
 * Parameters
 *    hWinSta
 *       Handle to the window station.
 *
 * Return Value
 *    Status
 *
 * Status
 *    @implemented
 */

BOOL STDCALL
NtUserSetProcessWindowStation(HWINSTA hWindowStation)
{
   PWINSTATION_OBJECT Object;
   PW32PROCESS Win32Process;
   NTSTATUS Status;

   DPRINT("About to set process window station with handle (0x%X)\n", 
      hWindowStation);

   Status = IntValidateWindowStationHandle(
      hWindowStation,
      KernelMode,
      0,
      &Object);

   if (!NT_SUCCESS(Status)) 
   {
      DPRINT("Validation of window station handle (0x%X) failed\n", 
         hWindowStation);
      return FALSE;
   }

   Win32Process = PsGetWin32Process();
   if (Win32Process == NULL)
   {
      ObDereferenceObject(Object);
   }
   else
   {
      if (Win32Process->WindowStation != NULL)
         ObDereferenceObject(Win32Process->WindowStation);
      Win32Process->WindowStation = Object;
   }

   SET_PROCESS_WINDOW_STATION(hWindowStation);
  
   DPRINT("IoGetCurrentProcess()->Win32WindowStation 0x%X\n",
      IoGetCurrentProcess()->Win32WindowStation);

   return TRUE;
}

/*
 * NtUserLockWindowStation
 *
 * Locks switching desktops. Only the logon application is allowed to call this function.
 *
 * Status
 *    @implemented
 */

BOOL STDCALL
NtUserLockWindowStation(HWINSTA hWindowStation)
{
   PWINSTATION_OBJECT Object;
   NTSTATUS Status;

   DPRINT("About to set process window station with handle (0x%X)\n", 
      hWindowStation);
   
   if(PsGetWin32Process() != LogonProcess)
   {
     DPRINT1("Unauthorized process attempted to lock the window station!\n");
     SetLastWin32Error(ERROR_ACCESS_DENIED);
     return FALSE;
   }
   
   Status = IntValidateWindowStationHandle(
      hWindowStation,
      KernelMode,
      0,
      &Object);
   if (!NT_SUCCESS(Status)) 
   {
      DPRINT("Validation of window station handle (0x%X) failed\n", 
         hWindowStation);
      SetLastNtError(Status);
      return FALSE;
   }
   
   Object->Flags |= WSS_LOCKED;
   
   ObDereferenceObject(Object);
   return TRUE;
}

/*
 * NtUserUnlockWindowStation
 *
 * Unlocks switching desktops. Only the logon application is allowed to call this function.
 *
 * Status
 *    @implemented
 */

BOOL STDCALL
NtUserUnlockWindowStation(HWINSTA hWindowStation)
{
   PWINSTATION_OBJECT Object;
   NTSTATUS Status;
   BOOL Ret;

   DPRINT("About to set process window station with handle (0x%X)\n", 
      hWindowStation);
   
   if(PsGetWin32Process() != LogonProcess)
   {
     DPRINT1("Unauthorized process attempted to unlock the window station!\n");
     SetLastWin32Error(ERROR_ACCESS_DENIED);
     return FALSE;
   }
   
   Status = IntValidateWindowStationHandle(
      hWindowStation,
      KernelMode,
      0,
      &Object);
   if (!NT_SUCCESS(Status)) 
   {
      DPRINT("Validation of window station handle (0x%X) failed\n", 
         hWindowStation);
      SetLastNtError(Status);
      return FALSE;
   }
   
   Ret = (Object->Flags & WSS_LOCKED) == WSS_LOCKED;
   Object->Flags &= ~WSS_LOCKED;
   
   ObDereferenceObject(Object);
   return Ret;
}

/*
 * NtUserSetWindowStationUser
 *
 * Status
 *    @unimplemented
 */

DWORD STDCALL
NtUserSetWindowStationUser(
   DWORD Unknown0,
   DWORD Unknown1,
   DWORD Unknown2,
   DWORD Unknown3)
{
   UNIMPLEMENTED

   return 0;
}

/*
 * NtUserBuildNameList
 *
 * Function used for enumeration of desktops or window stations.
 *
 * Parameters
 *    hWinSta
 *       For enumeration of window stations this parameter must be set to
 *       zero. Otherwise it's handle for window station.
 *
 *    dwSize
 *       Size of buffer passed by caller.
 *
 *    lpBuffer
 *       Buffer passed by caller. If the function succedes, the buffer is
 *       filled with window station/desktop count (in first DWORD) and
 *       NULL-terminated window station/desktop names.
 *
 *    pRequiredSize
 *       If the function suceedes, this is the number of bytes copied.
 *       Otherwise it's size of buffer needed for function to succeed.
 *
 * Status
 *    @unimplemented
 */

NTSTATUS STDCALL
NtUserBuildNameList(
   HWINSTA hWinSta,
   ULONG dwSize,
   PVOID lpBuffer,
   PULONG pRequiredSize)
{
#if 0
   NTSTATUS Status;
   HANDLE DirectoryHandle;
   ULONG EntryCount = 0;
   UNICODE_STRING DirectoryNameW;
   PWCHAR BufferChar;
   PDIRECTORY_OBJECT DirObj = NULL;
   PLIST_ENTRY CurrentEntry = NULL;
   POBJECT_HEADER CurrentObject = NULL;
	
   /*
    * Generate full window station name
    */

   /* FIXME: Correct this for desktop */
   if (!IntGetFullWindowStationName(&DirectoryNameW, NULL, NULL))
   {
      SetLastNtError(STATUS_INSUFFICIENT_RESOURCES);
      return 0;
   }

   /*
    * Try to open the directory.
    */

   Status = ObReferenceObjectByName(&DirectoryNameW, 0, NULL, DIRECTORY_QUERY,
      ObDirectoryType, UserMode, (PVOID*)&DirObj, NULL);
   if (!NT_SUCCESS(Status))
   {
      ExFreePool(DirectoryNameW.Buffer);
      return Status;
   }

   /*
    * Count the required size of buffer.
    */

   *pRequiredSize = sizeof(DWORD);
   for (CurrentEntry = DirObj->head.Flink; CurrentEntry != &DirObj->head;
        CurrentEntry = CurrentEntry->Flink)
   {
      CurrentObject = CONTAINING_RECORD(CurrentEntry, OBJECT_HEADER, Entry);
      *pRequiredSize += CurrentObject->Name.Length + sizeof(UNICODE_NULL);
      ++EntryCount;
   }

   DPRINT1("Required size: %d Entry count: %d\n", *pRequiredSize, EntryCount);

   /*
    * Check if the supplied buffer is large enought.
    */

   if (*pRequiredSize > dwSize)
   {
      ExFreePool(DirectoryNameW.Buffer);
      ObDereferenceObject(DirectoryHandle);
      return STATUS_BUFFER_TOO_SMALL;
   }

   /*
    * Generate the resulting buffer contents.
    */

   *((DWORD *)lpBuffer) = EntryCount;
   BufferChar = (PWCHAR)((INT_PTR)lpBuffer + 4);
   for (CurrentEntry = DirObj->head.Flink; CurrentEntry != &DirObj->head;
        CurrentEntry = CurrentEntry->Flink)
   {
      CurrentObject = CONTAINING_RECORD(CurrentEntry, OBJECT_HEADER, Entry);
      wcscpy(BufferChar, CurrentObject->Name.Buffer);
      DPRINT1("Name: %s\n", BufferChar);
      BufferChar += (CurrentObject->Name.Length / sizeof(WCHAR)) + 1;
   }

   /*
    * Free any resource.
    */

   ExFreePool(DirectoryNameW.Buffer);
   ObDereferenceObject(DirectoryHandle);

   return STATUS_SUCCESS;
#else
   UNIMPLEMENTED

   return STATUS_NOT_IMPLEMENTED;
#endif
}

/* EOF */
