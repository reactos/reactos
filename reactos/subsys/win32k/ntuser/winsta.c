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
 *  $Id: winsta.c,v 1.47 2003/11/25 22:06:31 gvg Exp $
 *
 *  COPYRIGHT:        See COPYING in the top level directory
 *  PROJECT:          ReactOS kernel
 *  PURPOSE:          Window stations and Desktops
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
#include <include/object.h>
#include <include/window.h>
#include <include/error.h>
#include <include/cursoricon.h>
#include <include/hotkey.h>
#include <include/color.h>
#include <include/mouse.h>
#include <include/callback.h>
#include <include/guicheck.h>

#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

/* Currently active window station */
PWINSTATION_OBJECT InputWindowStation = NULL;
/* Currently active desktop */
STATIC PDESKTOP_OBJECT InputDesktop = NULL;
STATIC HDESK InputDesktopHandle = NULL; 

LRESULT CALLBACK
IntDesktopWindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

STATIC PWNDCLASS_OBJECT DesktopWindowClass;
HDC ScreenDeviceContext = NULL;

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

NTSTATUS FASTCALL
InitDesktopImpl(VOID)
{
   WNDCLASSEXW wcx;
   
   /* 
    * Create the desktop window class
    */
   wcx.style = 0;
   wcx.lpfnWndProc = IntDesktopWindowProc;
   wcx.cbClsExtra = wcx.cbWndExtra = 0;
   wcx.hInstance = wcx.hIcon = wcx.hCursor = NULL;
   wcx.hbrBackground = NULL;
   wcx.lpszMenuName = NULL;
   wcx.lpszClassName = L"DesktopWindowClass";
   DesktopWindowClass = IntCreateClass(&wcx, TRUE, IntDesktopWindowProc,
      (RTL_ATOM)32880);
  
   return STATUS_SUCCESS;
}

NTSTATUS FASTCALL
CleanupDesktopImpl(VOID)
{
   /* FIXME: Unregister the desktop window class */

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

   FullName->Length = (WINSTA_ROOT_NAME_LENGTH + 1) * sizeof(WCHAR) +
      WinStaName->Length;
   if (DesktopName != NULL)
      FullName->Length += DesktopName->Length + sizeof(WCHAR);
   FullName->Buffer = ExAllocatePool(NonPagedPool, FullName->Length);
   if (FullName->Buffer == NULL)
   {
      return FALSE;
   }

   Buffer = FullName->Buffer;
   memcpy(Buffer, WINSTA_ROOT_NAME, WINSTA_ROOT_NAME_LENGTH * sizeof(WCHAR));
   Buffer += WINSTA_ROOT_NAME_LENGTH;
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

LRESULT CALLBACK
IntDesktopWindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
   switch (msg)
   {
      case WM_CREATE:
         return 0;

      case WM_NCCREATE:
         return 1;

      case WM_ERASEBKGND:
         return NtUserPaintDesktop((HDC)wParam);

      default:
         return 0;
   }
}

/*
 * IntValidateDesktopHandle
 *
 * Validates the desktop handle.
 *
 * Remarks
 *    If the function succeeds, the handle remains referenced. If the
 *    fucntion fails, last error is set.
 */

NTSTATUS FASTCALL
IntValidateDesktopHandle(
   HDESK Desktop,
   KPROCESSOR_MODE AccessMode,
   ACCESS_MASK DesiredAccess,
   PDESKTOP_OBJECT *Object)
{
   NTSTATUS Status;

   Status = ObReferenceObjectByHandle(
      Desktop,
      DesiredAccess,
      ExDesktopObjectType,
      AccessMode,
      (PVOID*)Object,
      NULL);

   if (!NT_SUCCESS(Status))
      SetLastNtError(Status);

   return Status;
}

BOOL FASTCALL
IntInitializeDesktopGraphics(VOID)
{
  if (! IntCreatePrimarySurface())
    {
      return FALSE;
    }
  ScreenDeviceContext = NtGdiCreateDC(L"DISPLAY", NULL, NULL, NULL);
  if (NULL == ScreenDeviceContext)
    {
      IntDestroyPrimarySurface();
      return FALSE;
    }
  GDIOBJ_MarkObjectGlobal(ScreenDeviceContext);
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
      GDIOBJ_UnmarkObjectGlobal(ScreenDeviceContext);
      NtGdiDeleteDC(ScreenDeviceContext);
      ScreenDeviceContext = NULL;
    }
  IntDestroyPrimarySurface();
}

HDC FASTCALL
IntGetScreenDC(VOID)
{
   return ScreenDeviceContext;
}

PDESKTOP_OBJECT FASTCALL
IntGetActiveDesktop(VOID)
{
   return InputDesktop;
}

PWINDOW_OBJECT FASTCALL
IntGetCaptureWindow(VOID)
{
   PDESKTOP_OBJECT pdo = IntGetActiveDesktop();
   if (!pdo)
   {
      DPRINT("No active desktop\n");
      return NULL;
   }
   return pdo->CaptureWindow;
}

VOID FASTCALL
IntSetCaptureWindow(PWINDOW_OBJECT Window)
{
   PDESKTOP_OBJECT pdo = IntGetActiveDesktop();
   if (!pdo)
   {
      DPRINT("No active desktop\n");
      return;
   }
   pdo->CaptureWindow = Window;
}

PUSER_MESSAGE_QUEUE FASTCALL
IntGetFocusMessageQueue(VOID)
{
   PDESKTOP_OBJECT pdo = IntGetActiveDesktop();
   if (!pdo)
   {
      DPRINT("No active desktop\n");
      return(NULL);
   }
   return (PUSER_MESSAGE_QUEUE)pdo->ActiveMessageQueue;
}

VOID FASTCALL
IntSetFocusMessageQueue(PUSER_MESSAGE_QUEUE NewQueue)
{
   PDESKTOP_OBJECT pdo = IntGetActiveDesktop();
   if (!pdo)
   {
      DPRINT("No active desktop\n");
      return;
   }
   pdo->ActiveMessageQueue = NewQueue;
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
      &WindowStation);

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
      &WindowStation);

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
   WindowStationObject->SystemCursor.SafetySwitch2 = TRUE;
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
      &WindowStation);


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
 * Status
 *    @unimplemented
 */

BOOL STDCALL
NtUserLockWindowStation(HWINSTA hWindowStation)
{
   UNIMPLEMENTED

   return 0;
}

/*
 * NtUserUnlockWindowStation
 *
 * Status
 *    @unimplemented
 */

BOOL STDCALL
NtUserUnlockWindowStation(HWINSTA hWindowStation)
{
   UNIMPLEMENTED

   return FALSE;
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
 * NtUserCreateDesktop
 *
 * Creates a new desktop.
 *
 * Parameters
 *    lpszDesktopName
 *       Name of the new desktop.
 *
 *    dwFlags
 *       Interaction flags.
 *
 *    dwDesiredAccess
 *       Requested type of access.
 *
 *    lpSecurity
 *       Security descriptor.
 *
 *    hWindowStation
 *       Handle to window station on which to create the desktop.
 *
 * Return Value
 *    If the function succeeds, the return value is a handle to the newly
 *    created desktop. If the specified desktop already exists, the function
 *    succeeds and returns a handle to the existing desktop. When you are
 *    finished using the handle, call the CloseDesktop function to close it.
 *    If the function fails, the return value is NULL.
 *
 * Status
 *    @implemented
 */

HDESK STDCALL
NtUserCreateDesktop(
   PUNICODE_STRING lpszDesktopName,
   DWORD dwFlags,
   ACCESS_MASK dwDesiredAccess,
   LPSECURITY_ATTRIBUTES lpSecurity,
   HWINSTA hWindowStation)
{
   OBJECT_ATTRIBUTES ObjectAttributes;
   PWINSTATION_OBJECT WinStaObject;
   PDESKTOP_OBJECT DesktopObject;
   UNICODE_STRING DesktopName;
   NTSTATUS Status;
   HDESK Desktop;

   Status = IntValidateWindowStationHandle(
      hWindowStation,
      KernelMode,
      0,
      &WinStaObject);

   if (!NT_SUCCESS(Status))
   {
      DPRINT("Failed validation of window station handle (0x%X)\n", 
         hWindowStation);
      SetLastNtError(Status);
      return 0;
   }
  
   if (!IntGetFullWindowStationName(&DesktopName, &WinStaObject->Name,
       lpszDesktopName))
   {
      SetLastNtError(STATUS_INSUFFICIENT_RESOURCES);
      ObDereferenceObject(WinStaObject);
      return 0;
   }

   ObDereferenceObject(WinStaObject);

   /*
    * Try to open already existing desktop
    */

   DPRINT("Trying to open desktop (%wZ)\n", &DesktopName);

   /* Initialize ObjectAttributes for the desktop object */
   InitializeObjectAttributes(
      &ObjectAttributes,
      &DesktopName,
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
      &Desktop);

   if (NT_SUCCESS(Status))
   {
      DPRINT("Successfully opened desktop (%wZ)\n", &DesktopName);
      ExFreePool(DesktopName.Buffer);
      return Desktop;
   }

   /*
    * No existing desktop found, try to create new one
    */

   Status = ObCreateObject(
      ExGetPreviousMode(),
      ExDesktopObjectType,
      &ObjectAttributes,
      ExGetPreviousMode(),
      NULL,
      sizeof(DESKTOP_OBJECT),
      0,
      0,
      (PVOID*)&DesktopObject);

   if (!NT_SUCCESS(Status))
   {
      DPRINT("Failed creating desktop (%wZ)\n", &DesktopName);
      ExFreePool(DesktopName.Buffer);
      SetLastNtError(STATUS_UNSUCCESSFUL);
      return((HDESK)0);
   }
  
   /* FIXME: Set correct dimensions. */
   DesktopObject->WorkArea.left = 0;
   DesktopObject->WorkArea.top = 0;
   DesktopObject->WorkArea.right = 640;
   DesktopObject->WorkArea.bottom = 480;

   /* Initialize some local (to win32k) desktop state. */
   DesktopObject->ActiveMessageQueue = NULL;  
   DesktopObject->DesktopWindow = IntCreateDesktopWindow(
      DesktopObject->WindowStation,
      DesktopWindowClass,
      DesktopObject->WorkArea.right,
      DesktopObject->WorkArea.bottom);

   DPRINT("Created Desktop Window: %08x\n", DesktopObject->DesktopWindow);

   Status = ObInsertObject(
      (PVOID)DesktopObject,
      NULL,
      STANDARD_RIGHTS_REQUIRED,
      0,
      NULL,
      &Desktop);

   ObDereferenceObject(DesktopObject);
   ExFreePool(DesktopName.Buffer);

   if (!NT_SUCCESS(Status))
   {
      DPRINT("Failed to create desktop handle\n");
      SetLastNtError(STATUS_UNSUCCESSFUL);
      return 0;
   }

   return Desktop;
}

/*
 * NtUserOpenDesktop
 *
 * Opens an existing desktop.
 *
 * Parameters
 *    lpszDesktopName
 *       Name of the existing desktop.
 *
 *    dwFlags
 *       Interaction flags.
 *
 *    dwDesiredAccess
 *       Requested type of access.
 *
 * Return Value
 *    Handle to the desktop or zero on failure.
 *
 * Status
 *    @implemented
 */

HDESK STDCALL
NtUserOpenDesktop(
   PUNICODE_STRING lpszDesktopName,
   DWORD dwFlags,
   ACCESS_MASK dwDesiredAccess)
{
   OBJECT_ATTRIBUTES ObjectAttributes;
   PWINSTATION_OBJECT WinStaObject;
   UNICODE_STRING DesktopName;
   NTSTATUS Status;
   HDESK Desktop;

   /*
    * Validate the window station handle and compose the fully
    * qualified desktop name
    */

   Status = IntValidateWindowStationHandle(
      PROCESS_WINDOW_STATION(),
      KernelMode,
      0,
      &WinStaObject);

   if (!NT_SUCCESS(Status))
   {
      DPRINT("Failed validation of window station handle (0x%X)\n",
         PROCESS_WINDOW_STATION());
      SetLastNtError(Status);
      return 0;
   }

   if (!IntGetFullWindowStationName(&DesktopName, &WinStaObject->Name,
       lpszDesktopName))
   {
      SetLastNtError(STATUS_INSUFFICIENT_RESOURCES);
      ObDereferenceObject(WinStaObject);
      return 0;
   }
 
   ObDereferenceObject(WinStaObject);

   DPRINT("Trying to open desktop station (%wZ)\n", &DesktopName);

   /* Initialize ObjectAttributes for the desktop object */
   InitializeObjectAttributes(
      &ObjectAttributes,
      &DesktopName,
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
      &Desktop);

   if (!NT_SUCCESS(Status))
   {
      SetLastNtError(Status);
      ExFreePool(DesktopName.Buffer);
      return 0;
   }

   DPRINT("Successfully opened desktop (%wZ)\n", &DesktopName);
   ExFreePool(DesktopName.Buffer);

   return Desktop;
}

/*
 * NtUserOpenInputDesktop
 *
 * Opens the input (interactive) desktop.
 *
 * Parameters
 *    dwFlags
 *       Interaction flags.
 *
 *    fInherit
 *       Inheritance option.
 *
 *    dwDesiredAccess
 *       Requested type of access.
 * 
 * Return Value
 *    Handle to the input desktop or zero on failure.
 *
 * Status
 *    @implemented
 */

HDESK STDCALL
NtUserOpenInputDesktop(
   DWORD dwFlags,
   BOOL fInherit,
   ACCESS_MASK dwDesiredAccess)
{
   PDESKTOP_OBJECT Object;
   NTSTATUS Status;
   HDESK Desktop;

   DPRINT("About to open input desktop\n");

   /* Get a pointer to the desktop object */

   Status = IntValidateDesktopHandle(
      InputDesktop,
      KernelMode,
      0,
      &Object);

   if (!NT_SUCCESS(Status))
   {
      DPRINT("Validation of input desktop handle (0x%X) failed\n", InputDesktop);
      return (HDESK)0;
   }

   /* Create a new handle to the object */

   Status = ObOpenObjectByPointer(
      Object,
      0,
      NULL,
      dwDesiredAccess,
      ExDesktopObjectType,
      UserMode,
      &Desktop);

   ObDereferenceObject(Object);

   if (NT_SUCCESS(Status))
   {
      DPRINT("Successfully opened input desktop\n");
      return (HDESK)Desktop;
   }

   SetLastNtError(Status);
   return (HDESK)0;
}

/*
 * NtUserCloseDesktop
 *
 * Closes a desktop handle.
 *
 * Parameters
 *    hDesktop
 *       Handle to the desktop.
 *
 * Return Value
 *   Status
 *
 * Remarks
 *   The desktop handle can be created with NtUserCreateDesktop or
 *   NtUserOpenDesktop. This function will fail if any thread in the calling
 *   process is using the specified desktop handle or if the handle refers
 *   to the initial desktop of the calling process.
 *
 * Status
 *    @implemented
 */

BOOL STDCALL
NtUserCloseDesktop(HDESK hDesktop)
{
   PDESKTOP_OBJECT Object;
   NTSTATUS Status;

   DPRINT("About to close desktop handle (0x%X)\n", hDesktop);

   Status = IntValidateDesktopHandle(
      hDesktop,
      KernelMode,
      0,
      &Object);

   if (!NT_SUCCESS(Status))
   {
      DPRINT("Validation of desktop handle (0x%X) failed\n", hDesktop);
      return FALSE;
   }

   ObDereferenceObject(Object);

   DPRINT("Closing desktop handle (0x%X)\n", hDesktop);

   Status = ZwClose(hDesktop);
   if (!NT_SUCCESS(Status))
   {
      SetLastNtError(Status);
      return FALSE;
   }

   return TRUE;
}

/*
 * NtUserPaintDesktop
 *
 * The NtUserPaintDesktop function fills the clipping region in the
 * specified device context with the desktop pattern or wallpaper. The
 * function is provided primarily for shell desktops.
 *
 * Parameters
 *    hdc 
 *       Handle to the device context. 
 *
 * Status
 *    @implemented
 */

BOOL STDCALL
NtUserPaintDesktop(HDC hDC)
{
   HWND hwnd = IntGetDesktopWindow();

   /*
    * Check for an owning thread, otherwise don't paint anything
    * (non-desktop mode)
    */

   if (NtUserGetWindowThreadProcessId(hwnd, NULL))
   {
      RECT Rect;
      HBRUSH PreviousBrush;

      NtUserGetClientRect(hwnd, &Rect);

      /*
       * Paint desktop background
       */

      PreviousBrush = NtGdiSelectObject(hDC, NtGdiGetSysColorBrush(COLOR_BACKGROUND));
      NtGdiPatBlt(hDC, Rect.left, Rect.top, Rect.right, Rect.bottom, PATCOPY);
      NtGdiSelectObject(hDC, PreviousBrush);
   }

   return TRUE;
}

/*
 * NtUserSwitchDesktop
 *
 * Sets the current input (interactive) desktop.
 *
 * Parameters
 *    hDesktop
 *       Handle to desktop.
 *
 * Return Value
 *    Status
 *
 * Status
 *    @unimplemented
 */

BOOL STDCALL
NtUserSwitchDesktop(HDESK hDesktop)
{
   PDESKTOP_OBJECT DesktopObject;
   NTSTATUS Status;

   DPRINT("About to switch desktop (0x%X)\n", hDesktop);

   Status = IntValidateDesktopHandle(
      hDesktop,
      KernelMode,
      0,
      &DesktopObject);

   if (!NT_SUCCESS(Status)) 
   {
      DPRINT("Validation of desktop handle (0x%X) failed\n", hDesktop);
      return FALSE;
   }

   /* FIXME: Fail if the desktop belong to an invisible window station */
   /* FIXME: Fail if the process is associated with a secured
             desktop such as Winlogon or Screen-Saver */
   /* FIXME: Connect to input device */

   /* Set the active desktop in the desktop's window station. */
   DesktopObject->WindowStation->ActiveDesktop = DesktopObject;

   /* Set the global state. */
   InputDesktop = DesktopObject;
   InputDesktopHandle = hDesktop;
   InputWindowStation = DesktopObject->WindowStation;

   ObDereferenceObject(DesktopObject);

   return TRUE;
}

/*
 * NtUserResolveDesktopForWOW
 *
 * Status
 *    @unimplemented
 */

DWORD STDCALL
NtUserResolveDesktopForWOW(DWORD Unknown0)
{
   UNIMPLEMENTED
   return 0;
}

/*
 * NtUserGetThreadDesktop
 *
 * Status
 *    @unimplemented
 */

HDESK STDCALL
NtUserGetThreadDesktop(DWORD dwThreadId, DWORD Unknown1)
{
   UNIMPLEMENTED
   return 0;
}

/*
 * NtUserSetThreadDesktop
 *
 * Status
 *    @implemented
 */

BOOL STDCALL
NtUserSetThreadDesktop(HDESK hDesktop)
{
   PDESKTOP_OBJECT DesktopObject;
   NTSTATUS Status;

   /* Validate the new desktop. */
   Status = IntValidateDesktopHandle(
      hDesktop,
      KernelMode,
      0,
      &DesktopObject);

   if (!NT_SUCCESS(Status)) 
   {
      DPRINT("Validation of desktop handle (0x%X) failed\n", hDesktop);
      return FALSE;
   }

   /* Check for setting the same desktop as before. */
   if (DesktopObject == PsGetWin32Thread()->Desktop)
   {
      ObDereferenceObject(DesktopObject);
      return TRUE;
   }

   /* FIXME: Should check here to see if the thread has any windows. */

   if (PsGetWin32Thread()->Desktop != NULL)
   {
      ObDereferenceObject(PsGetWin32Thread()->Desktop);
   }

   PsGetWin32Thread()->Desktop = DesktopObject;

   return TRUE;
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
   UNIMPLEMENTED

   return STATUS_UNSUCCESSFUL;
}

/* EOF */
