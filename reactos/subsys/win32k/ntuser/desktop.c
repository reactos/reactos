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
 *  $Id: desktop.c,v 1.11 2004/05/01 16:43:15 weiden Exp $
 *
 *  COPYRIGHT:        See COPYING in the top level directory
 *  PROJECT:          ReactOS kernel
 *  PURPOSE:          Desktops
 *  FILE:             subsys/win32k/ntuser/desktop.c
 *  PROGRAMER:        Casper S. Hornstrup (chorns@users.sourceforge.net)
 *  REVISION HISTORY:
 *       06-06-2001  CSH  Created
 */

/* INCLUDES ******************************************************************/

#define __WIN32K__
#include <ddk/ntddmou.h>
#include <win32k/win32k.h>
#include <csrss/csrss.h>
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

#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

/* Currently active desktop */
PDESKTOP_OBJECT InputDesktop = NULL;
HDESK InputDesktopHandle = NULL; 
HDC ScreenDeviceContext = NULL;

/* INITALIZATION FUNCTIONS ****************************************************/

NTSTATUS FASTCALL
InitDesktopImpl(VOID)
{
  return STATUS_SUCCESS;
}

NTSTATUS FASTCALL
CleanupDesktopImpl(VOID)
{
  return STATUS_SUCCESS;
}

/* PRIVATE FUNCTIONS **********************************************************/

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

PRECT FASTCALL
IntGetDesktopWorkArea(PDESKTOP_OBJECT Desktop)
{
  PRECT Ret;
  
  Ret = &Desktop->WorkArea;
  if((Ret->right == -1) && ScreenDeviceContext)
  {
    PDC dc;
    SURFOBJ *SurfObj;
    dc = DC_LockDc(ScreenDeviceContext);
    SurfObj = (SURFOBJ*)AccessUserObject((ULONG) dc->Surface);
    if(SurfObj)
    {
      Ret->right = SurfObj->sizlBitmap.cx;
      Ret->bottom = SurfObj->sizlBitmap.cy;
    }
    DC_UnlockDc(ScreenDeviceContext);
  }
  
  return Ret;
}

PDESKTOP_OBJECT FASTCALL
IntGetActiveDesktop(VOID)
{
   return InputDesktop;
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

HWND FASTCALL IntGetDesktopWindow(VOID)
{
   PDESKTOP_OBJECT pdo = IntGetActiveDesktop();
   if (!pdo)
   {
      DPRINT("No active desktop\n");
      return NULL;
   }
  return pdo->DesktopWindow;
}

/* PUBLIC FUNCTIONS ***********************************************************/


static NTSTATUS FASTCALL
NotifyCsrss(PCSRSS_API_REQUEST Request, PCSRSS_API_REPLY Reply)
{
  NTSTATUS Status;
  UNICODE_STRING PortName;
  ULONG ConnectInfoLength;
  static HANDLE WindowsApiPort = NULL;

  RtlInitUnicodeString(&PortName, L"\\Windows\\ApiPort");
  ConnectInfoLength = 0;
  Status = ZwConnectPort(&WindowsApiPort,
                         &PortName,
                         NULL,
                         NULL,
                         NULL,
                         NULL,
                         NULL,
                         &ConnectInfoLength);
  if (! NT_SUCCESS(Status))
    {
      return Status;
    }

   Request->Header.DataSize = sizeof(CSRSS_API_REQUEST) - LPC_MESSAGE_BASE_SIZE;
   Request->Header.MessageSize = sizeof(CSRSS_API_REQUEST);
   
   Status = ZwRequestWaitReplyPort(WindowsApiPort,
				   &Request->Header,
				   &Reply->Header);
   if (! NT_SUCCESS(Status) || ! NT_SUCCESS(Status = Reply->Status))
     {
       ZwClose(WindowsApiPort);
       return Status;
     }

//  ZwClose(WindowsApiPort);

  return STATUS_SUCCESS;
}

NTSTATUS FASTCALL
IntShowDesktop(PDESKTOP_OBJECT Desktop, ULONG Width, ULONG Height)
{
  CSRSS_API_REQUEST Request;
  CSRSS_API_REPLY Reply;

  Request.Type = CSRSS_SHOW_DESKTOP;
  Request.Data.ShowDesktopRequest.DesktopWindow = Desktop->DesktopWindow;
  Request.Data.ShowDesktopRequest.Width = Width;
  Request.Data.ShowDesktopRequest.Height = Height;

  return NotifyCsrss(&Request, &Reply);
}

NTSTATUS FASTCALL
IntHideDesktop(PDESKTOP_OBJECT Desktop)
{
#if 0
  CSRSS_API_REQUEST Request;
  CSRSS_API_REPLY Reply;

  Request.Type = CSRSS_HIDE_DESKTOP;
  Request.Data.HideDesktopRequest.DesktopWindow = Desktop->DesktopWindow;

  return NotifyCsrss(&Request, &Reply);
#else
  PWINDOW_OBJECT DesktopWindow;

  DesktopWindow = IntGetWindowObject(Desktop->DesktopWindow);
  if (! DesktopWindow)
    {
      return ERROR_INVALID_WINDOW_HANDLE;
    }
  DesktopWindow->Style &= ~WS_VISIBLE;

  return STATUS_SUCCESS;
#endif
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
  CSRSS_API_REQUEST Request;
  CSRSS_API_REPLY Reply;

  Status = IntValidateWindowStationHandle(
    hWindowStation,
    KernelMode,
    0,
    &WinStaObject);

  if (! NT_SUCCESS(Status))
    {
      DPRINT1("Failed validation of window station handle (0x%X)\n", 
        hWindowStation);
      SetLastNtError(Status);
      return NULL;
    }
  
  if (! IntGetFullWindowStationName(&DesktopName, &WinStaObject->Name,
          lpszDesktopName))
    {
      SetLastNtError(STATUS_INSUFFICIENT_RESOURCES);
      ObDereferenceObject(WinStaObject);
      return NULL;
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
    (HANDLE*)&Desktop);

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

  if (! NT_SUCCESS(Status))
    {
      DPRINT1("Failed creating desktop (%wZ)\n", &DesktopName);
      ExFreePool(DesktopName.Buffer);
      SetLastNtError(STATUS_UNSUCCESSFUL);
      return NULL;
    }
  
  // init desktop area
  DesktopObject->WorkArea.left = 0;
  DesktopObject->WorkArea.top = 0;
  DesktopObject->WorkArea.right = -1;
  DesktopObject->WorkArea.bottom = -1;
  IntGetDesktopWorkArea(DesktopObject);

  /* Initialize some local (to win32k) desktop state. */
  DesktopObject->ActiveMessageQueue = NULL;

  Status = ObInsertObject(
    (PVOID)DesktopObject,
    NULL,
    STANDARD_RIGHTS_REQUIRED,
    0,
    NULL,
    (HANDLE*)&Desktop);

  DesktopObject->Self = (HANDLE)Desktop;
  
  ObDereferenceObject(DesktopObject);
  ExFreePool(DesktopName.Buffer);

  if (! NT_SUCCESS(Status))
    {
      DPRINT1("Failed to create desktop handle\n");
      SetLastNtError(Status);
      return NULL;
    }

  Request.Type = CSRSS_CREATE_DESKTOP;
  memcpy(Request.Data.CreateDesktopRequest.DesktopName, lpszDesktopName->Buffer,
         lpszDesktopName->Length);
  Request.Data.CreateDesktopRequest.DesktopName[lpszDesktopName->Length / sizeof(WCHAR)] = L'\0';

  Status = NotifyCsrss(&Request, &Reply);
  if (! NT_SUCCESS(Status))
    {
      DPRINT1("Failed to notify CSRSS about new desktop\n");
      ZwClose(Desktop);
      SetLastNtError(Status);
      return NULL;
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
      (HANDLE*)&Desktop);

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
      InputDesktopHandle,
      UserMode,
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
      (HANDLE*)&Desktop);

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
      UserMode,
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
  RECT Rect;
  HBRUSH DesktopBrush, PreviousBrush;
  HWND hWndDesktop;

  IntGdiGetClipBox(hDC, &Rect);

  hWndDesktop = IntGetDesktopWindow();
  DesktopBrush = (HBRUSH)NtUserGetClassLong(hWndDesktop, GCL_HBRBACKGROUND, FALSE);

  /*
   * Paint desktop background
   */

  PreviousBrush = NtGdiSelectObject(hDC, DesktopBrush);
  NtGdiPatBlt(hDC, Rect.left, Rect.top, Rect.right, Rect.bottom, PATCOPY);
  NtGdiSelectObject(hDC, PreviousBrush);

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
      UserMode,
      0,
      &DesktopObject);

   if (!NT_SUCCESS(Status)) 
   {
      DPRINT("Validation of desktop handle (0x%X) failed\n", hDesktop);
      return FALSE;
   }
   
   /*
    * Don't allow applications switch the desktop if it's locked, unless the caller
    * is the logon application itself
    */
   if((DesktopObject->WindowStation->Flags & WSS_LOCKED) &&
      LogonProcess != NULL && LogonProcess != PsGetWin32Process())
   {
      ObDereferenceObject(DesktopObject);
      DPRINT1("Switching desktop 0x%x denied because the work station is locked!\n", hDesktop);
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
 *    @implemented
 */

HDESK STDCALL
NtUserGetThreadDesktop(DWORD dwThreadId, DWORD Unknown1)
{
  PDESKTOP_OBJECT Desktop;
  NTSTATUS Status;
  PETHREAD Thread;
  HDESK Ret;
  
  if(!dwThreadId)
  {
    SetLastWin32Error(ERROR_INVALID_PARAMETER);
    return 0;
  }
  
  Status = PsLookupThreadByThreadId((PVOID)dwThreadId, &Thread);
  if(!NT_SUCCESS(Status))
  {
    SetLastWin32Error(ERROR_INVALID_PARAMETER);
    return 0;
  }
  
  Desktop = Thread->Win32Thread->Desktop;
  
  if(Desktop)
  {
    Status = ObReferenceObjectByPointer(Desktop,
                                        0,
                                        ExDesktopObjectType,
                                        UserMode);
    if(!NT_SUCCESS(Status))
    {
      SetLastNtError(Status);
      return 0;
    }
    
    Ret = (HDESK)Desktop->Self;
    
    ObDereferenceObject(Desktop);
    return Ret;
  }
  
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
      UserMode,
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

/* EOF */
