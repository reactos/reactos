
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
 *  $Id$
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

#include <w32k.h>

#define NDEBUG
#include <debug.h>
#define TRACE DPRINT
#define WARN DPRINT1
#define ERR DPRINT1

static
VOID
IntFreeDesktopHeap(
    IN OUT PDESKTOP Desktop
);

/* GLOBALS *******************************************************************/

/* Currently active desktop */
PDESKTOP InputDesktop = NULL;
HDESK InputDesktopHandle = NULL;
HDC ScreenDeviceContext = NULL;
BOOL g_PaintDesktopVersion = FALSE;

GENERIC_MAPPING IntDesktopMapping =
{
      STANDARD_RIGHTS_READ     | DESKTOP_ENUMERATE       |
                                 DESKTOP_READOBJECTS,
      STANDARD_RIGHTS_WRITE    | DESKTOP_CREATEMENU      |
                                 DESKTOP_CREATEWINDOW    |
                                 DESKTOP_HOOKCONTROL     |
                                 DESKTOP_JOURNALPLAYBACK |
                                 DESKTOP_JOURNALRECORD   |
                                 DESKTOP_WRITEOBJECTS,
      STANDARD_RIGHTS_EXECUTE  | DESKTOP_SWITCHDESKTOP,
      STANDARD_RIGHTS_REQUIRED | DESKTOP_CREATEMENU      |
                                 DESKTOP_CREATEWINDOW    |
                                 DESKTOP_ENUMERATE       |
                                 DESKTOP_HOOKCONTROL     |
                                 DESKTOP_JOURNALPLAYBACK |
                                 DESKTOP_JOURNALRECORD   |
                                 DESKTOP_READOBJECTS     |
                                 DESKTOP_SWITCHDESKTOP   |
                                 DESKTOP_WRITEOBJECTS
};

/* OBJECT CALLBACKS **********************************************************/

NTSTATUS
NTAPI
IntDesktopObjectParse(IN PVOID ParseObject,
                      IN PVOID ObjectType,
                      IN OUT PACCESS_STATE AccessState,
                      IN KPROCESSOR_MODE AccessMode,
                      IN ULONG Attributes,
                      IN OUT PUNICODE_STRING CompleteName,
                      IN OUT PUNICODE_STRING RemainingName,
                      IN OUT PVOID Context OPTIONAL,
                      IN PSECURITY_QUALITY_OF_SERVICE SecurityQos OPTIONAL,
                      OUT PVOID *Object)
{
    NTSTATUS Status;
    PDESKTOP Desktop;
    OBJECT_ATTRIBUTES ObjectAttributes;
    PLIST_ENTRY NextEntry, ListHead;
    PWINSTATION_OBJECT WinStaObject = (PWINSTATION_OBJECT)ParseObject;
    PUNICODE_STRING DesktopName;

    /* Set the list pointers and loop the window station */
    ListHead = &WinStaObject->DesktopListHead;
    NextEntry = ListHead->Flink;
    while (NextEntry != ListHead)
    {
        /* Get the current desktop */
        Desktop = CONTAINING_RECORD(NextEntry, DESKTOP, ListEntry);

        /* Get its name */
        DesktopName = GET_DESKTOP_NAME(Desktop);
        if (DesktopName)
        {
            /* Compare the name */
            if (RtlEqualUnicodeString(RemainingName,
                                      DesktopName,
                                      (Attributes & OBJ_CASE_INSENSITIVE)))
            {
                /* We found a match. Did this come from a create? */
                if (Context)
                {
                    /* Unless OPEN_IF was given, fail with an error */
                    if (!(Attributes & OBJ_OPENIF))
                    {
                        /* Name collision */
                        return STATUS_OBJECT_NAME_COLLISION;
                    }
                    else
                    {
                        /* Otherwise, return with a warning only */
                        Status = STATUS_OBJECT_NAME_EXISTS;
                    }
                }
                else
                {
                    /* This was a real open, so this is OK */
                    Status = STATUS_SUCCESS;
                }

                /* Reference the desktop and return it */
                ObReferenceObject(Desktop);
                *Object = Desktop;
                return Status;
            }
        }

        /* Go to the next desktop */
        NextEntry = NextEntry->Flink;
    }

    /* If we got here but this isn't a create, then fail */
    if (!Context) return STATUS_OBJECT_NAME_NOT_FOUND;

    /* Create the desktop object */
    InitializeObjectAttributes(&ObjectAttributes, RemainingName, 0, NULL, NULL);
    Status = ObCreateObject(KernelMode,
                            ExDesktopObjectType,
                            &ObjectAttributes,
                            KernelMode,
                            NULL,
                            sizeof(DESKTOP),
                            0,
                            0,
                            (PVOID)&Desktop);
    if (!NT_SUCCESS(Status)) return Status;

    /* Initialize shell hook window list and set the parent */
    InitializeListHead(&Desktop->ShellHookWindows);
    Desktop->WindowStation = (PWINSTATION_OBJECT)ParseObject;

    /* Put the desktop on the window station's list of associated desktops */
    InsertTailList(&Desktop->WindowStation->DesktopListHead,
                   &Desktop->ListEntry);

    /* Set the desktop object and return success */
    *Object = Desktop;
    return STATUS_SUCCESS;
}

VOID STDCALL
IntDesktopObjectDelete(PWIN32_DELETEMETHOD_PARAMETERS Parameters)
{
   PDESKTOP Desktop = (PDESKTOP)Parameters->Object;

   DPRINT("Deleting desktop (0x%X)\n", Desktop);

   /* Remove the desktop from the window station's list of associcated desktops */
   RemoveEntryList(&Desktop->ListEntry);

   IntFreeDesktopHeap(Desktop);
}

/* PRIVATE FUNCTIONS **********************************************************/

NTSTATUS
FASTCALL
InitDesktopImpl(VOID)
{
    /* Set Desktop Object Attributes */
    ExDesktopObjectType->TypeInfo.DefaultNonPagedPoolCharge = sizeof(DESKTOP);
    ExDesktopObjectType->TypeInfo.GenericMapping = IntDesktopMapping;
    return STATUS_SUCCESS;
}

NTSTATUS
FASTCALL
CleanupDesktopImpl(VOID)
{
    return STATUS_SUCCESS;
}

static int GetSystemVersionString(LPWSTR buffer)
{
   RTL_OSVERSIONINFOEXW versionInfo;
   int len;

   versionInfo.dwOSVersionInfoSize = sizeof(RTL_OSVERSIONINFOEXW);

   if (!NT_SUCCESS(RtlGetVersion((PRTL_OSVERSIONINFOW)&versionInfo)))
      return 0;

   if (versionInfo.dwMajorVersion <= 4)
      len = swprintf(buffer,
                     L"ReactOS Version %d.%d %s Build %d",
                     versionInfo.dwMajorVersion, versionInfo.dwMinorVersion,
                     versionInfo.szCSDVersion, versionInfo.dwBuildNumber&0xFFFF);
   else
      len = swprintf(buffer,
                     L"ReactOS %s (Build %d)",
                     versionInfo.szCSDVersion, versionInfo.dwBuildNumber&0xFFFF);

   return len;
}


NTSTATUS FASTCALL
IntParseDesktopPath(PEPROCESS Process,
                    PUNICODE_STRING DesktopPath,
                    HWINSTA *hWinSta,
                    HDESK *hDesktop)
{
   OBJECT_ATTRIBUTES ObjectAttributes;
   UNICODE_STRING WinSta, Desktop, FullName;
   BOOL DesktopPresent = FALSE;
   BOOL WinStaPresent = FALSE;
   NTSTATUS Status;

   ASSERT(hWinSta);

   *hWinSta = NULL;

   if(hDesktop != NULL)
   {
      *hDesktop = NULL;
   }

   RtlInitUnicodeString(&WinSta, NULL);
   RtlInitUnicodeString(&Desktop, NULL);

   if(DesktopPath != NULL && DesktopPath->Buffer != NULL && DesktopPath->Length > sizeof(WCHAR))
   {
      PWCHAR c = DesktopPath->Buffer;
      USHORT wl = 0;
      USHORT l = DesktopPath->Length;

      /*
       * Parse the desktop path string which can be in the form "WinSta\Desktop"
       * or just "Desktop". In latter case WinSta0 will be used.
       */

      while(l > 0)
      {
         if(*c == L'\\')
         {
            wl = (ULONG_PTR)c - (ULONG_PTR)DesktopPath->Buffer;
            break;
         }
         l -= sizeof(WCHAR);
         c++;
      }

      if(wl > 0)
      {
         WinSta.Length = wl;
         WinSta.MaximumLength = wl + sizeof(WCHAR);
         WinSta.Buffer = DesktopPath->Buffer;

         WinStaPresent = TRUE;
         c++;
      }

      Desktop.Length = DesktopPath->Length - wl;
      if(wl > 0)
      {
         Desktop.Length -= sizeof(WCHAR);
      }
      if(Desktop.Length > 0)
      {
         Desktop.MaximumLength = Desktop.Length + sizeof(WCHAR);
         Desktop.Buffer = ((wl > 0) ? c : DesktopPath->Buffer);
         DesktopPresent = TRUE;
      }
   }

   if(!WinStaPresent)
   {
#if 0
      /* search the process handle table for (inherited) window station
         handles, use a more appropriate one than WinSta0 if possible. */
      if (!ObFindHandleForObject(Process,
                                 NULL,
                                 ExWindowStationObjectType,
                                 NULL,
                                 (PHANDLE)hWinSta))
#endif
      {
            /* we had no luck searching for opened handles, use WinSta0 now */
            RtlInitUnicodeString(&WinSta, L"WinSta0");
      }
   }

   if(!DesktopPresent && hDesktop != NULL)
   {
#if 0
      /* search the process handle table for (inherited) desktop
         handles, use a more appropriate one than Default if possible. */
      if (!ObFindHandleForObject(Process,
                                 NULL,
                                 ExDesktopObjectType,
                                 NULL,
                                 (PHANDLE)hDesktop))
#endif
      {
         /* we had no luck searching for opened handles, use Desktop now */
         RtlInitUnicodeString(&Desktop, L"Default");
      }
   }

   if(*hWinSta == NULL)
   {
      if(!IntGetFullWindowStationName(&FullName, &WinSta, NULL))
      {
         return STATUS_INSUFFICIENT_RESOURCES;
      }

      /* open the window station */
      InitializeObjectAttributes(&ObjectAttributes,
                                 &FullName,
                                 OBJ_CASE_INSENSITIVE,
                                 NULL,
                                 NULL);

      Status = ObOpenObjectByName(&ObjectAttributes,
                                  ExWindowStationObjectType,
                                  KernelMode,
                                  NULL,
                                  0,
                                  NULL,
                                  (HANDLE*)hWinSta);

      ExFreePoolWithTag(FullName.Buffer, TAG_STRING);

      if(!NT_SUCCESS(Status))
      {
         SetLastNtError(Status);
         DPRINT("Failed to reference window station %wZ PID: %d!\n", &WinSta, PsGetCurrentProcessId());
         return Status;
      }
   }

   if(hDesktop != NULL && *hDesktop == NULL)
   {
      if(!IntGetFullWindowStationName(&FullName, &WinSta, &Desktop))
      {
         NtClose(*hWinSta);
         *hWinSta = NULL;
         return STATUS_INSUFFICIENT_RESOURCES;
      }

      /* open the desktop object */
      InitializeObjectAttributes(&ObjectAttributes,
                                 &FullName,
                                 OBJ_CASE_INSENSITIVE,
                                 NULL,
                                 NULL);

      Status = ObOpenObjectByName(&ObjectAttributes,
                                  ExDesktopObjectType,
                                  KernelMode,
                                  NULL,
                                  0,
                                  NULL,
                                  (HANDLE*)hDesktop);

      ExFreePoolWithTag(FullName.Buffer, TAG_STRING);

      if(!NT_SUCCESS(Status))
      {
         *hDesktop = NULL;
         NtClose(*hWinSta);
         *hWinSta = NULL;
         SetLastNtError(Status);
         DPRINT("Failed to reference desktop %wZ PID: %d!\n", &Desktop, PsGetCurrentProcessId());
         return Status;
      }
   }

   return STATUS_SUCCESS;
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
   PDESKTOP *Object)
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

VOID FASTCALL
IntGetDesktopWorkArea(PDESKTOP Desktop, PRECT Rect)
{
   PRECT Ret;

   ASSERT(Desktop);

   Ret = &Desktop->WorkArea;
   if((Ret->right == -1) && ScreenDeviceContext)
   {
      PDC dc;
      BITMAPOBJ *BitmapObj;
      dc = DC_LockDc(ScreenDeviceContext);
      /* FIXME - Handle dc == NULL!!!! */
      BitmapObj = BITMAPOBJ_LockBitmap(dc->w.hBitmap);
      if(BitmapObj)
      {
         Ret->right = BitmapObj->SurfObj.sizlBitmap.cx;
         Ret->bottom = BitmapObj->SurfObj.sizlBitmap.cy;
         BITMAPOBJ_UnlockBitmap(BitmapObj);
      }
      DC_UnlockDc(dc);
   }

   if(Rect)
   {
      *Rect = *Ret;
   }
}

PDESKTOP FASTCALL
IntGetActiveDesktop(VOID)
{
   return InputDesktop;
}

/*
 * returns or creates a handle to the desktop object
 */
HDESK FASTCALL
IntGetDesktopObjectHandle(PDESKTOP DesktopObject)
{
   NTSTATUS Status;
   HDESK Ret;

   ASSERT(DesktopObject);

   if (!ObFindHandleForObject(PsGetCurrentProcess(),
                              DesktopObject,
                              ExDesktopObjectType,
                              NULL,
                              (PHANDLE)&Ret))
   {
      Status = ObOpenObjectByPointer(DesktopObject,
                                     0,
                                     NULL,
                                     0,
                                     ExDesktopObjectType,
                                     UserMode,
                                     (PHANDLE)&Ret);
      if(!NT_SUCCESS(Status))
      {
         /* unable to create a handle */
         DPRINT1("Unable to create a desktop handle\n");
         return NULL;
      }
   }
   else
   {
       DPRINT1("Got handle: %lx\n", Ret);
   }

   return Ret;
}

PUSER_MESSAGE_QUEUE FASTCALL
IntGetFocusMessageQueue(VOID)
{
   PDESKTOP pdo = IntGetActiveDesktop();
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
   PUSER_MESSAGE_QUEUE Old;
   PDESKTOP pdo = IntGetActiveDesktop();
   if (!pdo)
   {
      DPRINT("No active desktop\n");
      return;
   }
   if(NewQueue != NULL)
   {
      if(NewQueue->Desktop != NULL)
      {
         DPRINT("Message Queue already attached to another desktop!\n");
         return;
      }
      IntReferenceMessageQueue(NewQueue);
      (void)InterlockedExchangePointer((PVOID*)&NewQueue->Desktop, pdo);
   }
   Old = (PUSER_MESSAGE_QUEUE)InterlockedExchangePointer((PVOID*)&pdo->ActiveMessageQueue, NewQueue);
   if(Old != NULL)
   {
      (void)InterlockedExchangePointer((PVOID*)&Old->Desktop, 0);
      IntDereferenceMessageQueue(Old);
   }
}

HWND FASTCALL IntGetDesktopWindow(VOID)
{
   PDESKTOP pdo = IntGetActiveDesktop();
   if (!pdo)
   {
      DPRINT("No active desktop\n");
      return NULL;
   }
   return pdo->DesktopWindow;
}

PWINDOW_OBJECT FASTCALL UserGetDesktopWindow(VOID)
{
   PDESKTOP pdo = IntGetActiveDesktop();

   if (!pdo)
   {
      DPRINT("No active desktop\n");
      return NULL;
   }

   return UserGetWindowObject(pdo->DesktopWindow);
}


HWND FASTCALL IntGetCurrentThreadDesktopWindow(VOID)
{
   PTHREADINFO pti = PsGetCurrentThreadWin32Thread();
   PDESKTOP pdo = pti->Desktop;
   if (NULL == pdo)
   {
      DPRINT1("Thread doesn't have a desktop\n");
      return NULL;
   }
   return pdo->DesktopWindow;
}

BOOL FASTCALL IntDesktopUpdatePerUserSettings(BOOL bEnable)
{
   if (bEnable)
   {
      RTL_QUERY_REGISTRY_TABLE QueryTable[2];
      NTSTATUS Status;

      RtlZeroMemory(QueryTable, sizeof(QueryTable));

      QueryTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT;
      QueryTable[0].Name = L"PaintDesktopVersion";
      QueryTable[0].EntryContext = &g_PaintDesktopVersion;

      /* Query the "PaintDesktopVersion" flag in the "Control Panel\Desktop" key */
      Status = RtlQueryRegistryValues(RTL_REGISTRY_USER,
                                      L"Control Panel\\Desktop",
                                      QueryTable, NULL, NULL);
      if (!NT_SUCCESS(Status))
      {
         DPRINT1("RtlQueryRegistryValues failed for PaintDesktopVersion (%x)\n",
                 Status);
         g_PaintDesktopVersion = FALSE;
         return FALSE;
      }

      DPRINT("PaintDesktopVersion = %d\n", g_PaintDesktopVersion);

      return TRUE;
   }
   else
   {
      g_PaintDesktopVersion = FALSE;
      return TRUE;
   }
}

/* PUBLIC FUNCTIONS ***********************************************************/

VOID APIENTRY
UserRedrawDesktop()
{
    PWINDOW_OBJECT Window = NULL;

    UserEnterExclusive();

    Window = UserGetDesktopWindow();

    IntInvalidateWindows( Window,
            Window->UpdateRegion,
                       RDW_FRAME |
                       RDW_ERASE |
                  RDW_INVALIDATE |
                 RDW_ALLCHILDREN);
    UserLeave();
}


NTSTATUS FASTCALL
co_IntShowDesktop(PDESKTOP Desktop, ULONG Width, ULONG Height)
{
   CSR_API_MESSAGE Request;

   Request.Type = MAKE_CSR_API(SHOW_DESKTOP, CSR_GUI);
   Request.Data.ShowDesktopRequest.DesktopWindow = Desktop->DesktopWindow;
   Request.Data.ShowDesktopRequest.Width = Width;
   Request.Data.ShowDesktopRequest.Height = Height;

   return co_CsrNotify(&Request);
}

NTSTATUS FASTCALL
IntHideDesktop(PDESKTOP Desktop)
{
#if 0
   CSRSS_API_REQUEST Request;
   CSRSS_API_REPLY Reply;

   Request.Type = CSRSS_HIDE_DESKTOP;
   Request.Data.HideDesktopRequest.DesktopWindow = Desktop->DesktopWindow;

   return NotifyCsrss(&Request, &Reply);
#else

   PWINDOW_OBJECT DesktopWindow;
   PWINDOW DesktopWnd;

   DesktopWindow = IntGetWindowObject(Desktop->DesktopWindow);
   if (! DesktopWindow)
   {
      return ERROR_INVALID_WINDOW_HANDLE;
   }
   DesktopWnd = DesktopWindow->Wnd;
   DesktopWnd->Style &= ~WS_VISIBLE;

   return STATUS_SUCCESS;
#endif
}




static
HWND* FASTCALL
UserBuildShellHookHwndList(PDESKTOP Desktop)
{
   ULONG entries=0;
   PSHELL_HOOK_WINDOW Current;
   HWND* list;

   /* fixme: if we save nb elements in desktop, we dont have to loop to find nb entries */
   LIST_FOR_EACH(Current, &Desktop->ShellHookWindows, SHELL_HOOK_WINDOW, ListEntry)
      entries++;

   if (!entries) return NULL;

   list = ExAllocatePool(PagedPool, sizeof(HWND) * (entries + 1)); /* alloc one extra for nullterm */
   if (list)
   {
      HWND* cursor = list;

      LIST_FOR_EACH(Current, &Desktop->ShellHookWindows, SHELL_HOOK_WINDOW, ListEntry)
         *cursor++ = Current->hWnd;

      *cursor = NULL; /* nullterm list */
   }

   return list;
}

/*
 * Send the Message to the windows registered for ShellHook
 * notifications. The lParam contents depend on the Message. See
 * MSDN for more details (RegisterShellHookWindow)
 */
VOID co_IntShellHookNotify(WPARAM Message, LPARAM lParam)
{
   PDESKTOP Desktop = IntGetActiveDesktop();
   HWND* HwndList;

   static UINT MsgType = 0;

   if (!MsgType)
   {

      /* Too bad, this doesn't work.*/
#if 0
      UNICODE_STRING Str;
      RtlInitUnicodeString(&Str, L"SHELLHOOK");
      MsgType = UserRegisterWindowMessage(&Str);
#endif

      MsgType = IntAddAtom(L"SHELLHOOK");

      DPRINT("MsgType = %x\n", MsgType);
      if (!MsgType)
         DPRINT1("LastError: %x\n", GetLastNtError());
   }

   if (!Desktop)
   {
      DPRINT("IntShellHookNotify: No desktop!\n");
      return;
   }

   HwndList = UserBuildShellHookHwndList(Desktop);
   if (HwndList)
   {
      HWND* cursor = HwndList;

      for (; *cursor; cursor++)
      {
         DPRINT("Sending notify\n");
         co_IntPostOrSendMessage(*cursor,
                                 MsgType,
                                 Message,
                                 lParam);
      }

      ExFreePool(HwndList);
   }

}

/*
 * Add the window to the ShellHookWindows list. The windows
 * on that list get notifications that are important to shell
 * type applications.
 *
 * TODO: Validate the window? I'm not sure if sending these messages to
 * an unsuspecting application that is not your own is a nice thing to do.
 */
BOOL IntRegisterShellHookWindow(HWND hWnd)
{
   PTHREADINFO pti = PsGetCurrentThreadWin32Thread();
   PDESKTOP Desktop = pti->Desktop;
   PSHELL_HOOK_WINDOW Entry;

   DPRINT("IntRegisterShellHookWindow\n");

   /* First deregister the window, so we can be sure it's never twice in the
    * list.
    */
   IntDeRegisterShellHookWindow(hWnd);

   Entry = ExAllocatePoolWithTag(PagedPool,
                                 sizeof(SHELL_HOOK_WINDOW),
                                 TAG_WINSTA);

   if (!Entry)
      return FALSE;

   Entry->hWnd = hWnd;

   InsertTailList(&Desktop->ShellHookWindows, &Entry->ListEntry);

   return TRUE;
}

/*
 * Remove the window from the ShellHookWindows list. The windows
 * on that list get notifications that are important to shell
 * type applications.
 */
BOOL IntDeRegisterShellHookWindow(HWND hWnd)
{
   PTHREADINFO pti = PsGetCurrentThreadWin32Thread();
   PDESKTOP Desktop = pti->Desktop;
   PSHELL_HOOK_WINDOW Current;

   LIST_FOR_EACH(Current, &Desktop->ShellHookWindows, SHELL_HOOK_WINDOW, ListEntry)
   {
      if (Current->hWnd == hWnd)
      {
         RemoveEntryList(&Current->ListEntry);
         ExFreePool(Current);
         return TRUE;
      }
   }

   return FALSE;
}

static VOID
IntFreeDesktopHeap(IN OUT PDESKTOP Desktop)
{
    if (Desktop->DesktopHeapSection != NULL)
    {
        ObDereferenceObject(Desktop->DesktopHeapSection);
        Desktop->DesktopHeapSection = NULL;
    }
}
/* SYSCALLS *******************************************************************/


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
   PDESKTOP DesktopObject;
   UNICODE_STRING DesktopName;
   NTSTATUS Status;
   HDESK Desktop;
   CSR_API_MESSAGE Request;
   PVOID DesktopHeapSystemBase = NULL;
   SIZE_T DesktopInfoSize;
   UNICODE_STRING SafeDesktopName;
   ULONG DummyContext;
   ULONG_PTR HeapSize = 4 * 1024 * 1024; /* FIXME */
   DECLARE_RETURN(HDESK);


   DPRINT("Enter NtUserCreateDesktop: %wZ\n", lpszDesktopName);
   UserEnterExclusive();

   Status = IntValidateWindowStationHandle(
               hWindowStation,
               KernelMode,
               0, /* FIXME - WINSTA_CREATEDESKTOP */
               &WinStaObject);

   if (! NT_SUCCESS(Status))
   {
      DPRINT1("Failed validation of window station handle (0x%X), cannot create desktop %wZ\n",
              hWindowStation, lpszDesktopName);
      SetLastNtError(Status);
      RETURN( NULL);
   }
   if(lpszDesktopName != NULL)
   {
      Status = IntSafeCopyUnicodeString(&SafeDesktopName, lpszDesktopName);
      if(!NT_SUCCESS(Status))
      {
         SetLastNtError(Status);
         RETURN( NULL);
	  }
   }
   else
   {
      RtlInitUnicodeString(&SafeDesktopName, NULL);
   }

   if (! IntGetFullWindowStationName(&DesktopName, &WinStaObject->Name,
                                     &SafeDesktopName))
   {
      SetLastNtError(STATUS_INSUFFICIENT_RESOURCES);
      ObDereferenceObject(WinStaObject);
      if (lpszDesktopName)
         ExFreePoolWithTag(SafeDesktopName.Buffer, TAG_STRING);
      RETURN( NULL);
   }
   if (lpszDesktopName)
      ExFreePoolWithTag(SafeDesktopName.Buffer, TAG_STRING);
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
               KernelMode,
               NULL,
               dwDesiredAccess,
               (PVOID)&DummyContext,
               (HANDLE*)&Desktop);
   if (!NT_SUCCESS(Status)) RETURN(NULL);
   if (Status == STATUS_OBJECT_NAME_EXISTS)
   {
      ExFreePoolWithTag(DesktopName.Buffer, TAG_STRING);
      RETURN( Desktop);
   }

   /* Reference the desktop */
   Status = ObReferenceObjectByHandle(Desktop,
                                      0,
                                      ExDesktopObjectType,
                                      KernelMode,
                                      (PVOID)&DesktopObject,
                                      NULL);
   if (!NT_SUCCESS(Status)) RETURN(NULL);

   DesktopObject->DesktopHeapSection = NULL;
   DesktopObject->pheapDesktop = UserCreateHeap(&DesktopObject->DesktopHeapSection,
                                                &DesktopHeapSystemBase,
                                                HeapSize);
   if (DesktopObject->pheapDesktop == NULL)
   {
       ObDereferenceObject(DesktopObject);
       DPRINT1("Failed to create desktop heap!\n");
       RETURN(NULL);
   }

   DesktopInfoSize = FIELD_OFFSET(DESKTOPINFO,
                                  szDesktopName[(lpszDesktopName->Length / sizeof(WCHAR)) + 1]);

   DesktopObject->DesktopInfo = RtlAllocateHeap(DesktopObject->pheapDesktop,
                                                HEAP_NO_SERIALIZE,
                                                DesktopInfoSize);

   if (DesktopObject->DesktopInfo == NULL)
   {
       ObDereferenceObject(DesktopObject);
       DPRINT1("Failed to create the DESKTOP structure!\n");
       RETURN(NULL);
   }

   RtlZeroMemory(DesktopObject->DesktopInfo,
                 DesktopInfoSize);

   DesktopObject->DesktopInfo->pvDesktopBase = DesktopHeapSystemBase;
   DesktopObject->DesktopInfo->pvDesktopLimit = (PVOID)((ULONG_PTR)DesktopHeapSystemBase + HeapSize);
   RtlCopyMemory(DesktopObject->DesktopInfo->szDesktopName,
                 lpszDesktopName->Buffer,
                 lpszDesktopName->Length);

   // init desktop area
   DesktopObject->WorkArea.left = 0;
   DesktopObject->WorkArea.top = 0;
   DesktopObject->WorkArea.right = -1;
   DesktopObject->WorkArea.bottom = -1;
   IntGetDesktopWorkArea(DesktopObject, NULL);

   /* Initialize some local (to win32k) desktop state. */
   InitializeListHead(&DesktopObject->PtiList);
   DesktopObject->ActiveMessageQueue = NULL;
   ExFreePoolWithTag(DesktopName.Buffer, TAG_STRING);

   if (! NT_SUCCESS(Status))
   {
      DPRINT1("Failed to create desktop handle\n");
      SetLastNtError(Status);
      RETURN( NULL);
   }

   /*
    * Create a handle for CSRSS and notify CSRSS
    */
   Request.Type = MAKE_CSR_API(CREATE_DESKTOP, CSR_GUI);
   Status = CsrInsertObject(Desktop,
                            GENERIC_ALL,
                            (HANDLE*)&Request.Data.CreateDesktopRequest.DesktopHandle);
   if (! NT_SUCCESS(Status))
   {
      DPRINT1("Failed to create desktop handle for CSRSS\n");
      ZwClose(Desktop);
      SetLastNtError(Status);
      RETURN( NULL);
   }

   Status = co_CsrNotify(&Request);
   if (! NT_SUCCESS(Status))
   {
      CsrCloseHandle(Request.Data.CreateDesktopRequest.DesktopHandle);
      DPRINT1("Failed to notify CSRSS about new desktop\n");
      ZwClose(Desktop);
      SetLastNtError(Status);
      RETURN( NULL);
   }

   RETURN( Desktop);

CLEANUP:
   DPRINT("Leave NtUserCreateDesktop, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
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
   HWINSTA WinSta;
   PWINSTATION_OBJECT WinStaObject;
   UNICODE_STRING DesktopName;
   UNICODE_STRING SafeDesktopName;
   NTSTATUS Status;
   HDESK Desktop;
   BOOL Result;
   DECLARE_RETURN(HDESK);

   DPRINT("Enter NtUserOpenDesktop: %wZ\n", lpszDesktopName);
   UserEnterExclusive();

   /*
    * Validate the window station handle and compose the fully
    * qualified desktop name
    */

   WinSta = UserGetProcessWindowStation();
   Status = IntValidateWindowStationHandle(
               WinSta,
               KernelMode,
               0,
               &WinStaObject);

   if (!NT_SUCCESS(Status))
   {
      DPRINT1("Failed validation of window station handle (0x%X)\n", WinSta);
      SetLastNtError(Status);
      RETURN( 0);
   }

   if(lpszDesktopName != NULL)
   {
      Status = IntSafeCopyUnicodeString(&SafeDesktopName, lpszDesktopName);
      if(!NT_SUCCESS(Status))
      {
         SetLastNtError(Status);
         RETURN( NULL);
	  }
   }
   else
   {
      RtlInitUnicodeString(&SafeDesktopName, NULL);
   }

   Result = IntGetFullWindowStationName(&DesktopName, &WinStaObject->Name,
                                        &SafeDesktopName);

   if (lpszDesktopName)
      ExFreePoolWithTag(SafeDesktopName.Buffer, TAG_STRING);
   ObDereferenceObject(WinStaObject);


   if (!Result)
   {
      SetLastNtError(STATUS_INSUFFICIENT_RESOURCES);
      RETURN( 0);
   }


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
               KernelMode,
               NULL,
               dwDesiredAccess,
               NULL,
               (HANDLE*)&Desktop);

   if (!NT_SUCCESS(Status))
   {
      SetLastNtError(Status);
      ExFreePool(DesktopName.Buffer);
      RETURN( 0);
   }

   DPRINT("Successfully opened desktop (%wZ)\n", &DesktopName);
   ExFreePool(DesktopName.Buffer);

   RETURN( Desktop);

CLEANUP:
   DPRINT("Leave NtUserOpenDesktop, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
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
   PDESKTOP Object;
   NTSTATUS Status;
   HDESK Desktop;
   DECLARE_RETURN(HDESK);

   DPRINT("Enter NtUserOpenInputDesktop\n");
   UserEnterExclusive();

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
      RETURN((HDESK)0);
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
      RETURN((HDESK)Desktop);
   }

   SetLastNtError(Status);
   RETURN((HDESK)0);

CLEANUP:
   DPRINT("Leave NtUserOpenInputDesktop, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
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
   PDESKTOP Object;
   NTSTATUS Status;
   DECLARE_RETURN(BOOL);

   DPRINT("Enter NtUserCloseDesktop\n");
   UserEnterExclusive();

   DPRINT("About to close desktop handle (0x%X)\n", hDesktop);

   Status = IntValidateDesktopHandle(
               hDesktop,
               UserMode,
               0,
               &Object);

   if (!NT_SUCCESS(Status))
   {
      DPRINT("Validation of desktop handle (0x%X) failed\n", hDesktop);
      RETURN(FALSE);
   }

   ObDereferenceObject(Object);

   DPRINT("Closing desktop handle (0x%X)\n", hDesktop);

   Status = ZwClose(hDesktop);
   if (!NT_SUCCESS(Status))
   {
      SetLastNtError(Status);
      RETURN(FALSE);
   }

   RETURN(TRUE);

CLEANUP:
   DPRINT("Leave NtUserCloseDesktop, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}




/*
 * NtUserPaintDesktop
 *
 * The NtUserPaintDesktop function fills the clipping region in the
 * specified device context with the desktop pattern or wallpaper. The
 * function is provided primarily for shell desktops.
 *
 * Parameters
 *    hDC
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
   BOOL doPatBlt = TRUE;
   PWINDOW_OBJECT WndDesktop;
   int len;
   COLORREF color_old;
   UINT align_old;
   int mode_old;
   PTHREADINFO pti = PsGetCurrentThreadWin32Thread();
   PWINSTATION_OBJECT WinSta = pti->Desktop->WindowStation;
   DECLARE_RETURN(BOOL);

   UserEnterExclusive();
   DPRINT("Enter NtUserPaintDesktop\n");

   GdiGetClipBox(hDC, &Rect);

   hWndDesktop = IntGetDesktopWindow();

   WndDesktop = UserGetWindowObject(hWndDesktop);
   if (!WndDesktop)
   {
      RETURN(FALSE);
   }

   DesktopBrush = (HBRUSH)UserGetClassLongPtr(WndDesktop->Wnd->Class, GCL_HBRBACKGROUND, FALSE);


   /*
    * Paint desktop background
    */

   if (WinSta->hbmWallpaper != NULL)
   {
      PWINDOW_OBJECT DeskWin;

      DeskWin = UserGetWindowObject(hWndDesktop);

      if (DeskWin)
      {
         SIZE sz;
         int x, y;
         HDC hWallpaperDC;

         sz.cx = DeskWin->Wnd->WindowRect.right - DeskWin->Wnd->WindowRect.left;
         sz.cy = DeskWin->Wnd->WindowRect.bottom - DeskWin->Wnd->WindowRect.top;

         if (WinSta->WallpaperMode == wmStretch ||
             WinSta->WallpaperMode == wmTile)
         {
            x = 0;
            y = 0;
         }
         else
         {
            /* Find the upper left corner, can be negtive if the bitmap is bigger then the screen */
            x = (sz.cx / 2) - (WinSta->cxWallpaper / 2);
            y = (sz.cy / 2) - (WinSta->cyWallpaper / 2);
         }

         hWallpaperDC = NtGdiCreateCompatibleDC(hDC);
         if(hWallpaperDC != NULL)
         {
            HBITMAP hOldBitmap;

            /* fill in the area that the bitmap is not going to cover */
            if (x > 0 || y > 0)
            {
               /* FIXME - clip out the bitmap
                                            can be replaced with "NtGdiPatBlt(hDC, x, y, WinSta->cxWallpaper, WinSta->cyWallpaper, PATCOPY | DSTINVERT);"
                                            once we support DSTINVERT */
              PreviousBrush = NtGdiSelectBrush(hDC, DesktopBrush);
              NtGdiPatBlt(hDC, Rect.left, Rect.top, Rect.right, Rect.bottom, PATCOPY);
              NtGdiSelectBrush(hDC, PreviousBrush);
            }

            /*Do not fill the background after it is painted no matter the size of the picture */
            doPatBlt = FALSE;

            hOldBitmap = NtGdiSelectBitmap(hWallpaperDC, WinSta->hbmWallpaper);

            if (WinSta->WallpaperMode == wmStretch)
            {
                if(Rect.right && Rect.bottom)
	                NtGdiStretchBlt(hDC,
                                x,
                                y,
                                sz.cx,
                                sz.cy,
                                hWallpaperDC,
                                0,
                                0,
                                WinSta->cxWallpaper,
                                WinSta->cyWallpaper,
                                SRCCOPY,
                                0);

            }
            else if (WinSta->WallpaperMode == wmTile)
            {
                /* paint the bitmap across the screen then down */
                for(y = 0; y < Rect.bottom; y += WinSta->cyWallpaper)
                {
                    for(x = 0; x < Rect.right; x += WinSta->cxWallpaper)
                    {
                        NtGdiBitBlt(hDC,
                                    x,
                                    y,
                                    WinSta->cxWallpaper,
                                    WinSta->cyWallpaper,
                                    hWallpaperDC,
                                    0,
                                    0,
                                    SRCCOPY,
                                    0,
                                    0);
                    }
                }
            }
            else
            {
                NtGdiBitBlt(hDC,
                            x,
                            y,
                            WinSta->cxWallpaper,
                            WinSta->cyWallpaper,
                            hWallpaperDC,
                            0,
                            0,
                            SRCCOPY,
                            0,
                            0);
            }
            NtGdiSelectBitmap(hWallpaperDC, hOldBitmap);
            NtGdiDeleteObjectApp(hWallpaperDC);
         }
      }
   }

   /* Back ground is set to none, clear the screen */
   if (doPatBlt)
   {
      PreviousBrush = NtGdiSelectBrush(hDC, DesktopBrush);
      NtGdiPatBlt(hDC, Rect.left, Rect.top, Rect.right, Rect.bottom, PATCOPY);
      NtGdiSelectBrush(hDC, PreviousBrush);
   }

   /*
    * Display system version on the desktop background
    */

   if (g_PaintDesktopVersion)
   {
      static WCHAR s_wszVersion[256] = {0};
      RECT rect;

      if (*s_wszVersion)
      {
         len = wcslen(s_wszVersion);
      }
      else
      {
         len = GetSystemVersionString(s_wszVersion);
      }

      if (len)
      {
         if (!UserSystemParametersInfo(SPI_GETWORKAREA, 0, &rect, 0))
         {
            rect.right = UserGetSystemMetrics(SM_CXSCREEN);
            rect.bottom = UserGetSystemMetrics(SM_CYSCREEN);
         }

         color_old = IntGdiSetTextColor(hDC, RGB(255,255,255));
         align_old = IntGdiSetTextAlign(hDC, TA_RIGHT);
         mode_old = IntGdiSetBkMode(hDC, TRANSPARENT);

         NtGdiExtTextOutW(hDC, rect.right-16, rect.bottom-48, 0, NULL, s_wszVersion, len, NULL, 0);

         IntGdiSetBkMode(hDC, mode_old);
         IntGdiSetTextAlign(hDC, align_old);
         IntGdiSetTextColor(hDC, color_old);
      }
   }

   RETURN(TRUE);

CLEANUP:
   DPRINT("Leave NtUserPaintDesktop, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
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
   PDESKTOP DesktopObject;
   NTSTATUS Status;
   DECLARE_RETURN(BOOL);

   UserEnterExclusive();
   DPRINT("Enter NtUserSwitchDesktop\n");

   DPRINT("About to switch desktop (0x%X)\n", hDesktop);

   Status = IntValidateDesktopHandle(
               hDesktop,
               UserMode,
               0,
               &DesktopObject);

   if (!NT_SUCCESS(Status))
   {
      DPRINT("Validation of desktop handle (0x%X) failed\n", hDesktop);
      RETURN(FALSE);
   }

   /*
    * Don't allow applications switch the desktop if it's locked, unless the caller
    * is the logon application itself
    */
   if((DesktopObject->WindowStation->Flags & WSS_LOCKED) &&
         LogonProcess != NULL && LogonProcess != PsGetCurrentProcessWin32Process())
   {
      ObDereferenceObject(DesktopObject);
      DPRINT1("Switching desktop 0x%x denied because the work station is locked!\n", hDesktop);
      RETURN(FALSE);
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

   RETURN(TRUE);

CLEANUP:
   DPRINT("Leave NtUserSwitchDesktop, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
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
   NTSTATUS Status;
   PETHREAD Thread;
   PDESKTOP DesktopObject;
   HDESK Ret, hThreadDesktop;
   OBJECT_HANDLE_INFORMATION HandleInformation;
   DECLARE_RETURN(HDESK);

   UserEnterExclusive();
   DPRINT("Enter NtUserGetThreadDesktop\n");

   if(!dwThreadId)
   {
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      RETURN(0);
   }

   Status = PsLookupThreadByThreadId((HANDLE)(DWORD_PTR)dwThreadId, &Thread);
   if(!NT_SUCCESS(Status))
   {
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      RETURN(0);
   }

   if(Thread->ThreadsProcess == PsGetCurrentProcess())
   {
      /* just return the handle, we queried the desktop handle of a thread running
         in the same context */
      Ret = ((PTHREADINFO)Thread->Tcb.Win32Thread)->hDesktop;
      ObDereferenceObject(Thread);
      RETURN(Ret);
   }

   /* get the desktop handle and the desktop of the thread */
   if(!(hThreadDesktop = ((PTHREADINFO)Thread->Tcb.Win32Thread)->hDesktop) ||
         !(DesktopObject = ((PTHREADINFO)Thread->Tcb.Win32Thread)->Desktop))
   {
      ObDereferenceObject(Thread);
      DPRINT1("Desktop information of thread 0x%x broken!?\n", dwThreadId);
      RETURN(NULL);
   }

   /* we could just use DesktopObject instead of looking up the handle, but latter
      may be a bit safer (e.g. when the desktop is being destroyed */
   /* switch into the context of the thread we're trying to get the desktop from,
      so we can use the handle */
   KeAttachProcess(&Thread->ThreadsProcess->Pcb);
   Status = ObReferenceObjectByHandle(hThreadDesktop,
                                      GENERIC_ALL,
                                      ExDesktopObjectType,
                                      UserMode,
                                      (PVOID*)&DesktopObject,
                                      &HandleInformation);
   KeDetachProcess();

   /* the handle couldn't be found, there's nothing to get... */
   if(!NT_SUCCESS(Status))
   {
      ObDereferenceObject(Thread);
      RETURN(NULL);
   }

   /* lookup our handle table if we can find a handle to the desktop object,
      if not, create one */
   Ret = IntGetDesktopObjectHandle(DesktopObject);

   /* all done, we got a valid handle to the desktop */
   ObDereferenceObject(DesktopObject);
   ObDereferenceObject(Thread);
   RETURN(Ret);

CLEANUP:
   DPRINT("Leave NtUserGetThreadDesktop, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}

static NTSTATUS
IntUnmapDesktopView(IN PDESKTOP DesktopObject)
{
    PW32THREADINFO ti;
    PW32PROCESS CurrentWin32Process;
    PW32HEAP_USER_MAPPING HeapMapping, *PrevLink;
    NTSTATUS Status = STATUS_SUCCESS;

    TRACE("DO %p\n");

    CurrentWin32Process = PsGetCurrentProcessWin32Process();
    PrevLink = &CurrentWin32Process->HeapMappings.Next;

    /* unmap if we're the last thread using the desktop */
    HeapMapping = *PrevLink;
    while (HeapMapping != NULL)
    {
        if (HeapMapping->KernelMapping == (PVOID)DesktopObject->pheapDesktop)
        {
            if (--HeapMapping->Count == 0)
            {
                *PrevLink = HeapMapping->Next;

                Status = MmUnmapViewOfSection(PsGetCurrentProcess(),
                                              HeapMapping->UserMapping);

                ObDereferenceObject(DesktopObject);

                UserHeapFree(HeapMapping);
                break;
            }
        }

        PrevLink = &HeapMapping->Next;
        HeapMapping = HeapMapping->Next;
    }

    ti = GetW32ThreadInfo();
    if (ti != NULL)
    {
        if (ti->Desktop == DesktopObject->DesktopInfo)
        {
            ti->Desktop = NULL;
        }
    }
    GetWin32ClientInfo()->ulClientDelta = 0;

    return Status;
}

static NTSTATUS
IntMapDesktopView(IN PDESKTOP DesktopObject)
{
    PW32THREADINFO ti;
    PW32PROCESS CurrentWin32Process;
    PW32HEAP_USER_MAPPING HeapMapping, *PrevLink;
    PVOID UserBase = NULL;
    SIZE_T ViewSize = 0;
    LARGE_INTEGER Offset;
    NTSTATUS Status;

    CurrentWin32Process = PsGetCurrentProcessWin32Process();
    PrevLink = &CurrentWin32Process->HeapMappings.Next;

    /* find out if another thread already mapped the desktop heap */
    HeapMapping = *PrevLink;
    while (HeapMapping != NULL)
    {
        if (HeapMapping->KernelMapping == (PVOID)DesktopObject->pheapDesktop)
        {
            HeapMapping->Count++;
            return STATUS_SUCCESS;
        }

        PrevLink = &HeapMapping->Next;
        HeapMapping = HeapMapping->Next;
    }

    /* we're the first, map the heap */
    DPRINT("Noone mapped the desktop heap %p yet, so - map it!\n", DesktopObject->pheapDesktop);
    Offset.QuadPart = 0;
    Status = MmMapViewOfSection(DesktopObject->DesktopHeapSection,
                                PsGetCurrentProcess(),
                                &UserBase,
                                0,
                                0,
                                &Offset,
                                &ViewSize,
                                ViewUnmap,
                                SEC_NO_CHANGE,
                                PAGE_EXECUTE_READ); /* would prefer PAGE_READONLY, but thanks to RTL heaps... */
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to map desktop\n");
        return Status;
    }

    /* add the mapping */
    HeapMapping = UserHeapAlloc(sizeof(W32HEAP_USER_MAPPING));
    if (HeapMapping == NULL)
    {
        MmUnmapViewOfSection(PsGetCurrentProcess(),
                             UserBase);
        DPRINT1("UserHeapAlloc() failed!\n");
        return STATUS_NO_MEMORY;
    }

    HeapMapping->Next = NULL;
    HeapMapping->KernelMapping = (PVOID)DesktopObject->pheapDesktop;
    HeapMapping->UserMapping = UserBase;
    HeapMapping->Limit = ViewSize;
    HeapMapping->Count = 1;
    *PrevLink = HeapMapping;

    ObReferenceObject(DesktopObject);

    /* create a W32THREADINFO structure if not already done, or update it */
    ti = GetW32ThreadInfo();
    if (ti != NULL)
    {
        if (ti->Desktop == NULL)
        {
            ti->Desktop = DesktopObject->DesktopInfo;
        }
    }
    GetWin32ClientInfo()->ulClientDelta = DesktopHeapGetUserDelta();

    return STATUS_SUCCESS;
}

BOOL
IntSetThreadDesktop(IN PDESKTOP DesktopObject,
                    IN BOOL FreeOnFailure)
{
    PDESKTOP OldDesktop;
    PTHREADINFO W32Thread;
    NTSTATUS Status;
    BOOL MapHeap;

    DPRINT("IntSetThreadDesktop() DO=%p, FOF=%d\n", DesktopObject, FreeOnFailure);
    MapHeap = (PsGetCurrentProcess() != PsInitialSystemProcess);
    W32Thread = PsGetCurrentThreadWin32Thread();

    if (W32Thread->Desktop != DesktopObject)
    {
        OldDesktop = W32Thread->Desktop;

        if (!IsListEmpty(&W32Thread->WindowListHead))
        {
            DPRINT1("Attempted to change thread desktop although the thread has windows!\n");
            SetLastWin32Error(ERROR_BUSY);
            return FALSE;
        }

        W32Thread->Desktop = DesktopObject;

        if (MapHeap && DesktopObject != NULL)
        {
            Status = IntMapDesktopView(DesktopObject);
            if (!NT_SUCCESS(Status))
            {
                SetLastNtError(Status);
                return FALSE;
            }
        }

        if (W32Thread->Desktop == NULL)
        {
            PW32THREADINFO ti = GetW32ThreadInfo();
            if (ti != NULL)
            {
                ti->Desktop = NULL;
            }
        }

        /* Hack for system threads */
        if (NtCurrentTeb())
        {
            PCLIENTINFO pci = GetWin32ClientInfo();
            pci->ulClientDelta = DesktopHeapGetUserDelta();
            if (DesktopObject)
            {
                pci->pDeskInfo = (PVOID)((ULONG_PTR)DesktopObject->DesktopInfo - pci->ulClientDelta);
            }
        }

        if (OldDesktop != NULL &&
            !IntCheckProcessDesktopClasses(OldDesktop,
                                           FreeOnFailure))
        {
            DPRINT1("Failed to move process classes to shared heap!\n");

            /* failed to move desktop classes to the shared heap,
               unmap the view and return the error */
            if (MapHeap && DesktopObject != NULL)
                IntUnmapDesktopView(DesktopObject);

            return FALSE;
        }

        /* Remove the thread from the old desktop's list */
        RemoveEntryList(&W32Thread->PtiLink);

        if (DesktopObject != NULL)
        {
            ObReferenceObject(DesktopObject);
            /* Insert into new desktop's list */
            InsertTailList(&DesktopObject->PtiList, &W32Thread->PtiLink);
        }

        if (OldDesktop != NULL)
        {
            if (MapHeap)
            {
                IntUnmapDesktopView(OldDesktop);
            }

            ObDereferenceObject(OldDesktop);
        }
    }

    return TRUE;
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
   PDESKTOP DesktopObject;
   NTSTATUS Status;
   DECLARE_RETURN(BOOL);

   UserEnterExclusive();
   DPRINT("Enter NtUserSetThreadDesktop\n");

   /* Validate the new desktop. */
   Status = IntValidateDesktopHandle(
               hDesktop,
               UserMode,
               0,
               &DesktopObject);

   if (!NT_SUCCESS(Status))
   {
      DPRINT("Validation of desktop handle (0x%X) failed\n", hDesktop);
      RETURN(FALSE);
   }

   /* FIXME: Should check here to see if the thread has any windows. */

   if (!IntSetThreadDesktop(DesktopObject,
                            FALSE))
   {
       RETURN(FALSE);
   }

   RETURN(TRUE);

CLEANUP:
   DPRINT("Leave NtUserSetThreadDesktop, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}

/* EOF */
