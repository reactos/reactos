/*
 *  COPYRIGHT:        See COPYING in the top level directory
 *  PROJECT:          ReactOS Win32k subsystem
 *  PURPOSE:          Desktops
 *  FILE:             subsystems/win32/win32k/ntuser/desktop.c
 *  PROGRAMER:        Casper S. Hornstrup (chorns@users.sourceforge.net)
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>
DBG_DEFAULT_CHANNEL(UserDesktop);

#include <reactos/buildno.h>

static NTSTATUS
UserInitializeDesktop(PDESKTOP pdesk, PUNICODE_STRING DesktopName, PWINSTATION_OBJECT pwinsta);

static NTSTATUS
IntMapDesktopView(IN PDESKTOP pdesk);

static NTSTATUS
IntUnmapDesktopView(IN PDESKTOP pdesk);

static VOID
IntFreeDesktopHeap(IN PDESKTOP pdesk);

/* GLOBALS *******************************************************************/

/* These can be changed via csrss startup, these are defaults */
DWORD gdwDesktopSectionSize = 512;
DWORD gdwNOIOSectionSize    = 128; // A guess, for one or more of the first three system desktops.

/* Currently active desktop */
PDESKTOP gpdeskInputDesktop = NULL;
HDC ScreenDeviceContext = NULL;
PTHREADINFO gptiDesktopThread = NULL;
HCURSOR gDesktopCursor = NULL;

/* OBJECT CALLBACKS **********************************************************/

NTSTATUS
APIENTRY
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
    UNICODE_STRING DesktopName;
    PBOOLEAN pContext = (PBOOLEAN) Context;

    if(pContext)
        *pContext = FALSE;

    /* Set the list pointers and loop the window station */
    ListHead = &WinStaObject->DesktopListHead;
    NextEntry = ListHead->Flink;
    while (NextEntry != ListHead)
    {
        /* Get the current desktop */
        Desktop = CONTAINING_RECORD(NextEntry, DESKTOP, ListEntry);

        /* Get the desktop name */
        ASSERT(Desktop->pDeskInfo != NULL);
        RtlInitUnicodeString(&DesktopName, Desktop->pDeskInfo->szDesktopName);

        /* Compare the name */
        if (RtlEqualUnicodeString(RemainingName,
                                  &DesktopName,
                                  (Attributes & OBJ_CASE_INSENSITIVE) != 0))
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
                            (PVOID*)&Desktop);
    if (!NT_SUCCESS(Status)) return Status;

    /* Initialize the desktop */
    Status = UserInitializeDesktop(Desktop, RemainingName, WinStaObject);
    if (!NT_SUCCESS(Status))
    {
        ObDereferenceObject(Desktop);
        return Status;
    }

    /* Set the desktop object and return success */
    *Object = Desktop;
    *pContext = TRUE;
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
IntDesktopObjectDelete(
    _In_ PVOID Parameters)
{
    PWIN32_DELETEMETHOD_PARAMETERS DeleteParameters = Parameters;
   PDESKTOP pdesk = (PDESKTOP)DeleteParameters->Object;

   TRACE("Deleting desktop object 0x%p\n", pdesk);

   ASSERT(pdesk->pDeskInfo->spwnd->spwndChild == NULL);

   if (pdesk->pDeskInfo->spwnd)
       co_UserDestroyWindow(pdesk->pDeskInfo->spwnd);

   if (pdesk->spwndMessage)
       co_UserDestroyWindow(pdesk->spwndMessage);

   /* Remove the desktop from the window station's list of associcated desktops */
   RemoveEntryList(&pdesk->ListEntry);

   /* Free the heap */
   IntFreeDesktopHeap(pdesk);
   return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
IntDesktopOkToClose(
    _In_ PVOID Parameters)
{
    PWIN32_OKAYTOCLOSEMETHOD_PARAMETERS OkToCloseParameters = Parameters;
    PTHREADINFO pti = PsGetCurrentThreadWin32Thread();

    if( pti == NULL)
    {
        /* This happens when we leak desktop handles */
        return STATUS_SUCCESS;
    }

    /* Do not allow the current desktop or the initial desktop to be closed */
    if( OkToCloseParameters->Handle == pti->ppi->hdeskStartup ||
        OkToCloseParameters->Handle == pti->hdesk)
    {
        return STATUS_ACCESS_DENIED;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
IntDesktopObjectOpen(
    _In_ PVOID Parameters)
{
    PWIN32_OPENMETHOD_PARAMETERS OpenParameters = Parameters;
    PPROCESSINFO ppi = PsGetProcessWin32Process(OpenParameters->Process);
    if (ppi == NULL)
        return STATUS_SUCCESS;

    return IntMapDesktopView((PDESKTOP)OpenParameters->Object);
}

NTSTATUS
NTAPI
IntDesktopObjectClose(
    _In_ PVOID Parameters)
{
    PWIN32_CLOSEMETHOD_PARAMETERS CloseParameters = Parameters;
    PPROCESSINFO ppi = PsGetProcessWin32Process(CloseParameters->Process);
    if (ppi == NULL)
    {
        /* This happens when the process leaks desktop handles.
         * At this point the PPROCESSINFO is already destroyed */
         return STATUS_SUCCESS;
    }

    return IntUnmapDesktopView((PDESKTOP)CloseParameters->Object);
}


/* PRIVATE FUNCTIONS **********************************************************/

INIT_FUNCTION
NTSTATUS
NTAPI
InitDesktopImpl(VOID)
{
    GENERIC_MAPPING IntDesktopMapping = { DESKTOP_READ,
                                          DESKTOP_WRITE,
                                          DESKTOP_EXECUTE,
                                          DESKTOP_ALL_ACCESS};

    /* Set Desktop Object Attributes */
    ExDesktopObjectType->TypeInfo.DefaultNonPagedPoolCharge = sizeof(DESKTOP);
    ExDesktopObjectType->TypeInfo.GenericMapping = IntDesktopMapping;
    ExDesktopObjectType->TypeInfo.ValidAccessMask = DESKTOP_ALL_ACCESS;
    return STATUS_SUCCESS;
}

static int GetSystemVersionString(LPWSTR buffer)
{
    int len;
#if 0 // Disabled until versioning in win32k gets correctly implemented (hbelusca).
    RTL_OSVERSIONINFOEXW versionInfo;

    versionInfo.dwOSVersionInfoSize = sizeof(RTL_OSVERSIONINFOEXW);

    if (!NT_SUCCESS(RtlGetVersion((PRTL_OSVERSIONINFOW)&versionInfo)))
        return 0;

    if (versionInfo.dwMajorVersion <= 4)
        len = swprintf(buffer,
                       L"ReactOS Version %lu.%lu %s Build %lu",
                       versionInfo.dwMajorVersion, versionInfo.dwMinorVersion,
                       versionInfo.szCSDVersion, versionInfo.dwBuildNumber & 0xFFFF);
    else
        len = swprintf(buffer,
                       L"ReactOS %s (Build %lu)",
                       versionInfo.szCSDVersion, versionInfo.dwBuildNumber & 0xFFFF);
#else
    len = swprintf(buffer, L"ReactOS Version %S %S", KERNEL_VERSION_STR, KERNEL_VERSION_BUILD_STR);
#endif

   return len;
}


NTSTATUS FASTCALL
IntParseDesktopPath(PEPROCESS Process,
                    PUNICODE_STRING DesktopPath,
                    HWINSTA *hWinSta,
                    HDESK *hDesktop)
{
   OBJECT_ATTRIBUTES ObjectAttributes;
   UNICODE_STRING ObjectName;
   NTSTATUS Status;
   WCHAR wstrWinstaFullName[MAX_PATH], *pwstrWinsta = NULL, *pwstrDesktop = NULL;

   ASSERT(hWinSta);
   ASSERT(hDesktop);
   ASSERT(DesktopPath);

   *hWinSta = NULL;
   *hDesktop = NULL;

   if(DesktopPath->Buffer != NULL && DesktopPath->Length > sizeof(WCHAR))
   {
      /*
       * Parse the desktop path string which can be in the form "WinSta\Desktop"
       * or just "Desktop". In latter case WinSta0 will be used.
       */

      pwstrDesktop = wcschr(DesktopPath->Buffer, L'\\');
      if(pwstrDesktop != NULL)
      {
          *pwstrDesktop = 0;
          pwstrDesktop++;
          pwstrWinsta = DesktopPath->Buffer;
      }
      else
      {
          pwstrDesktop = DesktopPath->Buffer;
          pwstrWinsta = NULL;
      }

      TRACE("IntParseDesktopPath pwstrWinsta:%S pwstrDesktop:%S\n", pwstrWinsta, pwstrDesktop);
   }

#if 0
   /* Search the process handle table for (inherited) window station
      handles, use a more appropriate one than WinSta0 if possible. */
   if (!ObFindHandleForObject(Process,
                              NULL,
                              ExWindowStationObjectType,
                              NULL,
                              (PHANDLE)hWinSta))
#endif
   {
       /* We had no luck searching for opened handles, use WinSta0 now */
       if(!pwstrWinsta)
           pwstrWinsta = L"WinSta0";
   }

#if 0
   /* Search the process handle table for (inherited) desktop
      handles, use a more appropriate one than Default if possible. */
   if (!ObFindHandleForObject(Process,
                              NULL,
                              ExDesktopObjectType,
                              NULL,
                              (PHANDLE)hDesktop))
#endif
   {
       /* We had no luck searching for opened handles, use Desktop now */
       if(!pwstrDesktop)
           pwstrDesktop = L"Default";
   }

   if(*hWinSta == NULL)
   {
       swprintf(wstrWinstaFullName, L"%wZ\\%ws", &gustrWindowStationsDir, pwstrWinsta);
       RtlInitUnicodeString( &ObjectName, wstrWinstaFullName);

       TRACE("parsed initial winsta: %wZ\n", &ObjectName);

      /* Open the window station */
      InitializeObjectAttributes(&ObjectAttributes,
                                 &ObjectName,
                                 OBJ_CASE_INSENSITIVE,
                                 NULL,
                                 NULL);

      Status = ObOpenObjectByName(&ObjectAttributes,
                                  ExWindowStationObjectType,
                                  KernelMode,
                                  NULL,
                                  WINSTA_ACCESS_ALL,
                                  NULL,
                                  (HANDLE*)hWinSta);

      if(!NT_SUCCESS(Status))
      {
         SetLastNtError(Status);
         ERR("Failed to reference window station %wZ PID: --!\n", &ObjectName );
         return Status;
      }
   }

   if(*hDesktop == NULL)
   {
      RtlInitUnicodeString(&ObjectName, pwstrDesktop);

      TRACE("parsed initial desktop: %wZ\n", &ObjectName);

      /* Open the desktop object */
      InitializeObjectAttributes(&ObjectAttributes,
                                 &ObjectName,
                                 OBJ_CASE_INSENSITIVE,
                                 *hWinSta,
                                 NULL);

      Status = ObOpenObjectByName(&ObjectAttributes,
                                  ExDesktopObjectType,
                                  KernelMode,
                                  NULL,
                                  DESKTOP_ALL_ACCESS,
                                  NULL,
                                  (HANDLE*)hDesktop);

      if(!NT_SUCCESS(Status))
      {
         *hDesktop = NULL;
         NtClose(*hWinSta);
         *hWinSta = NULL;
         SetLastNtError(Status);
         ERR("Failed to reference desktop %wZ PID: --!\n", &ObjectName);
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

   TRACE("IntValidateDesktopHandle: handle:0x%p obj:0x%p access:0x%x Status:0x%lx\n",
            Desktop, *Object, DesiredAccess, Status);

   if (!NT_SUCCESS(Status))
      SetLastNtError(Status);

   return Status;
}

PDESKTOP FASTCALL
IntGetActiveDesktop(VOID)
{
   return gpdeskInputDesktop;
}

/*
 * Returns or creates a handle to the desktop object
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
         /* Unable to create a handle */
         ERR("Unable to create a desktop handle\n");
         return NULL;
      }
   }
   else
   {
       TRACE("Got handle: %p\n", Ret);
   }

   return Ret;
}

PUSER_MESSAGE_QUEUE FASTCALL
IntGetFocusMessageQueue(VOID)
{
   PDESKTOP pdo = IntGetActiveDesktop();
   if (!pdo)
   {
      TRACE("No active desktop\n");
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
      TRACE("No active desktop\n");
      return;
   }
   if(NewQueue != NULL)
   {
      if(NewQueue->Desktop != NULL)
      {
         TRACE("Message Queue already attached to another desktop!\n");
         return;
      }
      IntReferenceMessageQueue(NewQueue);
      (void)InterlockedExchangePointer((PVOID*)&NewQueue->Desktop, pdo);
   }
   Old = (PUSER_MESSAGE_QUEUE)InterlockedExchangePointer((PVOID*)&pdo->ActiveMessageQueue, NewQueue);
   if(Old != NULL)
   {
      (void)InterlockedExchangePointer((PVOID*)&Old->Desktop, 0);
      gpqForegroundPrev = Old;
      IntDereferenceMessageQueue(Old);
   }
   // Only one Q can have active foreground even when there are more than one desktop.
   if (NewQueue)
   {
      gpqForeground = pdo->ActiveMessageQueue;
   }
   else
   {
      gpqForeground = NULL;
      ERR("ptiLastInput is CLEARED!!\n");
      ptiLastInput = NULL; // ReactOS hacks,,,, should check for process death.
   }
}

PWND FASTCALL
IntGetThreadDesktopWindow(PTHREADINFO pti)
{
   if (!pti) pti = PsGetCurrentThreadWin32Thread();
   if (pti->pDeskInfo) return pti->pDeskInfo->spwnd;
   return NULL;
}

PWND FASTCALL co_GetDesktopWindow(PWND pWnd)
{
   if (pWnd->head.rpdesk &&
       pWnd->head.rpdesk->pDeskInfo)
      return pWnd->head.rpdesk->pDeskInfo->spwnd;
   return NULL;
}

HWND FASTCALL IntGetDesktopWindow(VOID)
{
   PDESKTOP pdo = IntGetActiveDesktop();
   if (!pdo)
   {
      TRACE("No active desktop\n");
      return NULL;
   }
   return pdo->DesktopWindow;
}

PWND FASTCALL UserGetDesktopWindow(VOID)
{
   PDESKTOP pdo = IntGetActiveDesktop();

   if (!pdo)
   {
      TRACE("No active desktop\n");
      return NULL;
   }
   // return pdo->pDeskInfo->spwnd;
   return UserGetWindowObject(pdo->DesktopWindow);
}

HWND FASTCALL IntGetMessageWindow(VOID)
{
   PDESKTOP pdo = IntGetActiveDesktop();

   if (!pdo)
   {
      TRACE("No active desktop\n");
      return NULL;
   }
   return pdo->spwndMessage->head.h;
}

PWND FASTCALL UserGetMessageWindow(VOID)
{
   PDESKTOP pdo = IntGetActiveDesktop();

   if (!pdo)
   {
      TRACE("No active desktop\n");
      return NULL;
   }
   return pdo->spwndMessage;
}

HWND FASTCALL IntGetCurrentThreadDesktopWindow(VOID)
{
   PTHREADINFO pti = PsGetCurrentThreadWin32Thread();
   PDESKTOP pdo = pti->rpdesk;
   if (NULL == pdo)
   {
      ERR("Thread doesn't have a desktop\n");
      return NULL;
   }
   return pdo->DesktopWindow;
}

/* PUBLIC FUNCTIONS ***********************************************************/

BOOL FASTCALL
DesktopWindowProc(PWND Wnd, UINT Msg, WPARAM wParam, LPARAM lParam, LRESULT *lResult)
{
   PAINTSTRUCT Ps;
   ULONG Value;
   //ERR("DesktopWindowProc\n");

   *lResult = 0;

   switch (Msg)
   {
      case WM_NCCREATE:
         if (!Wnd->fnid)
         {
            Wnd->fnid = FNID_DESKTOP;
         }
         *lResult = (LRESULT)TRUE;
         return TRUE;

      case WM_CREATE:
         Value = HandleToULong(PsGetCurrentProcessId());
         // Save Process ID
         co_UserSetWindowLong(UserHMGetHandle(Wnd), DT_GWL_PROCESSID, Value, FALSE);
         Value = HandleToULong(PsGetCurrentThreadId());
         // Save Thread ID
         co_UserSetWindowLong(UserHMGetHandle(Wnd), DT_GWL_THREADID, Value, FALSE);
      case WM_CLOSE:
         return TRUE;

      case WM_DISPLAYCHANGE:
         co_WinPosSetWindowPos(Wnd, 0, 0, 0, LOWORD(lParam), HIWORD(lParam), SWP_NOZORDER | SWP_NOACTIVATE);
         return TRUE;

      case WM_ERASEBKGND:
         IntPaintDesktop((HDC)wParam);
         *lResult = 1;
         return TRUE;

      case WM_PAINT:
      {
         if (IntBeginPaint(Wnd, &Ps))
         {
            IntEndPaint(Wnd, &Ps);
         }
         return TRUE;
      }
      case WM_SYSCOLORCHANGE:
         co_UserRedrawWindow(Wnd, NULL, NULL, RDW_INVALIDATE|RDW_ERASE|RDW_ALLCHILDREN);
         return TRUE;

      case WM_SETCURSOR:
      {
          PCURICON_OBJECT pcurOld, pcurNew;
          pcurNew = UserGetCurIconObject(gDesktopCursor);
          if (!pcurNew)
          {
              return TRUE;
          }

          pcurNew->CURSORF_flags |= CURSORF_CURRENT;
          pcurOld = UserSetCursor(pcurNew, FALSE);
          if (pcurOld)
          {
               pcurOld->CURSORF_flags &= ~CURSORF_CURRENT;
               UserDereferenceObject(pcurOld);
          }
          return TRUE;
      }

      case WM_WINDOWPOSCHANGING:
      {
          PWINDOWPOS pWindowPos = (PWINDOWPOS)lParam;
          if((pWindowPos->flags & SWP_SHOWWINDOW) != 0)
          {
              HDESK hdesk = IntGetDesktopObjectHandle(gpdeskInputDesktop);
              IntSetThreadDesktop(hdesk, FALSE);
          }
          break;
      }
      default:
          TRACE("DWP calling IDWP Msg %d\n",Msg);
          *lResult = IntDefWindowProc(Wnd, Msg, wParam, lParam, FALSE);
   }
   return TRUE; /* We are done. Do not do any callbacks to user mode */
}

BOOL FASTCALL
UserMessageWindowProc(PWND pwnd, UINT Msg, WPARAM wParam, LPARAM lParam, LRESULT *lResult)
{
    *lResult = 0;

    switch(Msg)
    {
    case WM_NCCREATE:
        pwnd->fnid |= FNID_MESSAGEWND;
        *lResult = (LRESULT)TRUE;
        break;
    case WM_DESTROY:
        pwnd->fnid |= FNID_DESTROY;
        break;
    default:
        ERR("UMWP calling IDWP\n");
        *lResult = IntDefWindowProc(pwnd, Msg, wParam, lParam, FALSE);
    }

    return TRUE; /* We are done. Do not do any callbacks to user mode */
}

VOID NTAPI DesktopThreadMain()
{
    BOOL Ret;
    MSG Msg;

    gptiDesktopThread = PsGetCurrentThreadWin32Thread();

    UserEnterExclusive();

    /* Register system classes. This thread does not belong to any desktop so the
       classes will be allocated from the shared heap */
    UserRegisterSystemClasses();

    while (TRUE)
    {
        Ret = co_IntGetPeekMessage(&Msg, 0, 0, 0, PM_REMOVE, TRUE);
        if (Ret)
        {
            IntDispatchMessage(&Msg);
        }
    }

    UserLeave();
}

HDC FASTCALL
UserGetDesktopDC(ULONG DcType, BOOL EmptyDC, BOOL ValidatehWnd)
{
    PWND DesktopObject = 0;
    HDC DesktopHDC = 0;

    if (DcType == DC_TYPE_DIRECT)
    {
        DesktopObject = UserGetDesktopWindow();
        DesktopHDC = (HDC)UserGetWindowDC(DesktopObject);
    }
    else
    {
        PMONITOR pMonitor = UserGetPrimaryMonitor();
        DesktopHDC = IntGdiCreateDisplayDC(pMonitor->hDev, DcType, EmptyDC);
    }

    return DesktopHDC;
}

VOID APIENTRY
UserRedrawDesktop()
{
    PWND Window = NULL;
    PREGION Rgn;

    Window = UserGetDesktopWindow();
    Rgn = IntSysCreateRectpRgnIndirect(&Window->rcWindow);

    IntInvalidateWindows( Window,
                             Rgn,
                       RDW_FRAME |
                       RDW_ERASE |
                  RDW_INVALIDATE |
                 RDW_ALLCHILDREN);

    REGION_Delete(Rgn);
}


NTSTATUS FASTCALL
co_IntShowDesktop(PDESKTOP Desktop, ULONG Width, ULONG Height, BOOL bRedraw)
{
   PWND pwnd = Desktop->pDeskInfo->spwnd;
   UINT flags = SWP_NOACTIVATE|SWP_NOZORDER|SWP_SHOWWINDOW;
   ASSERT(pwnd);

   if(!bRedraw)
       flags |= SWP_NOREDRAW;

   co_WinPosSetWindowPos(pwnd, NULL, 0, 0, Width, Height, flags);

   if(bRedraw)
       co_UserRedrawWindow( pwnd, NULL, 0, RDW_UPDATENOW | RDW_ALLCHILDREN | RDW_INVALIDATE );

   return STATUS_SUCCESS;
}

NTSTATUS FASTCALL
IntHideDesktop(PDESKTOP Desktop)
{
   PWND DesktopWnd;

   DesktopWnd = IntGetWindowObject(Desktop->DesktopWindow);
   if (! DesktopWnd)
   {
      return ERROR_INVALID_WINDOW_HANDLE;
   }
   DesktopWnd->style &= ~WS_VISIBLE;

   return STATUS_SUCCESS;
}

static
HWND* FASTCALL
UserBuildShellHookHwndList(PDESKTOP Desktop)
{
   ULONG entries=0;
   PLIST_ENTRY ListEntry;
   PSHELL_HOOK_WINDOW Current;
   HWND* list;

   /* FIXME: If we save nb elements in desktop, we don't have to loop to find nb entries */
   ListEntry = Desktop->ShellHookWindows.Flink;
   while (ListEntry != &Desktop->ShellHookWindows)
   {
      ListEntry = ListEntry->Flink;
      entries++;
   }

   if (!entries) return NULL;

   list = ExAllocatePoolWithTag(PagedPool, sizeof(HWND) * (entries + 1), USERTAG_WINDOWLIST); /* alloc one extra for nullterm */
   if (list)
   {
      HWND* cursor = list;

      ListEntry = Desktop->ShellHookWindows.Flink;
      while (ListEntry != &Desktop->ShellHookWindows)
      {
         Current = CONTAINING_RECORD(ListEntry, SHELL_HOOK_WINDOW, ListEntry);
         ListEntry = ListEntry->Flink;
         *cursor++ = Current->hWnd;
      }

      *cursor = NULL; /* Nullterm list */
   }

   return list;
}

/*
 * Send the Message to the windows registered for ShellHook
 * notifications. The lParam contents depend on the Message. See
 * MSDN for more details (RegisterShellHookWindow)
 */
VOID co_IntShellHookNotify(WPARAM Message, WPARAM wParam, LPARAM lParam)
{
   PDESKTOP Desktop = IntGetActiveDesktop();
   HWND* HwndList;

   if (!gpsi->uiShellMsg)
   {
      gpsi->uiShellMsg = IntAddAtom(L"SHELLHOOK");

      TRACE("MsgType = %x\n", gpsi->uiShellMsg);
      if (!gpsi->uiShellMsg)
         ERR("LastError: %x\n", EngGetLastError());
   }

   if (!Desktop)
   {
      TRACE("IntShellHookNotify: No desktop!\n");
      return;
   }

   // Allow other devices have a shot at foreground.
   if (Message == HSHELL_APPCOMMAND) ptiLastInput = NULL;

   // FIXME: System Tray Support.

   HwndList = UserBuildShellHookHwndList(Desktop);
   if (HwndList)
   {
      HWND* cursor = HwndList;

      for (; *cursor; cursor++)
      {
         TRACE("Sending notify\n");
         UserPostMessage(*cursor,
                          gpsi->uiShellMsg,
                          Message,
                         (Message == HSHELL_LANGUAGE ? lParam : (LPARAM)wParam) );
/*         co_IntPostOrSendMessage(*cursor,
                                 gpsi->uiShellMsg,
                                 Message,
                                 (Message == HSHELL_LANGUAGE ? lParam : (LPARAM)wParam) );*/
      }

      ExFreePoolWithTag(HwndList, USERTAG_WINDOWLIST);
   }

   if (ISITHOOKED(WH_SHELL))
   {
      co_HOOK_CallHooks(WH_SHELL, Message, wParam, lParam);
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
   PDESKTOP Desktop = pti->rpdesk;
   PSHELL_HOOK_WINDOW Entry;

   TRACE("IntRegisterShellHookWindow\n");

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
   PDESKTOP Desktop = pti->rpdesk;
   PLIST_ENTRY ListEntry;
   PSHELL_HOOK_WINDOW Current;

   ListEntry = Desktop->ShellHookWindows.Flink;
   while (ListEntry != &Desktop->ShellHookWindows)
   {
      Current = CONTAINING_RECORD(ListEntry, SHELL_HOOK_WINDOW, ListEntry);
      ListEntry = ListEntry->Flink;
      if (Current->hWnd == hWnd)
      {
         RemoveEntryList(&Current->ListEntry);
         ExFreePoolWithTag(Current, TAG_WINSTA);
         return TRUE;
      }
   }

   return FALSE;
}

static VOID
IntFreeDesktopHeap(IN OUT PDESKTOP Desktop)
{
    /* FIXME: Disable until unmapping works in mm */
#if 0
    if (Desktop->pheapDesktop != NULL)
    {
        MmUnmapViewInSessionSpace(Desktop->pheapDesktop);
        Desktop->pheapDesktop = NULL;
    }

    if (Desktop->hsectionDesktop != NULL)
    {
        ObDereferenceObject(Desktop->hsectionDesktop);
        Desktop->hsectionDesktop = NULL;
    }
#endif
}

BOOL FASTCALL
IntPaintDesktop(HDC hDC)
{
    RECTL Rect;
    HBRUSH DesktopBrush, PreviousBrush;
    HWND hWndDesktop;
    BOOL doPatBlt = TRUE;
    PWND WndDesktop;
    static WCHAR s_wszSafeMode[] = L"Safe Mode";
    int len;
    COLORREF color_old;
    UINT align_old;
    int mode_old;

    if (GdiGetClipBox(hDC, &Rect) == ERROR)
        return FALSE;

    hWndDesktop = IntGetDesktopWindow(); // rpdesk->DesktopWindow;

    WndDesktop = UserGetWindowObject(hWndDesktop); // rpdesk->pDeskInfo->spwnd;
    if (!WndDesktop)
        return FALSE;

    if (!UserGetSystemMetrics(SM_CLEANBOOT))
    {
        DesktopBrush = (HBRUSH)WndDesktop->pcls->hbrBackground;

        /*
         * Paint desktop background
         */
        if (gspv.hbmWallpaper != NULL)
        {
            SIZE sz;
            int x, y;
            HDC hWallpaperDC;

            sz.cx = WndDesktop->rcWindow.right - WndDesktop->rcWindow.left;
            sz.cy = WndDesktop->rcWindow.bottom - WndDesktop->rcWindow.top;

            if (gspv.WallpaperMode == wmStretch ||
                gspv.WallpaperMode == wmTile)
            {
                x = 0;
                y = 0;
            }
            else
            {
                /* Find the upper left corner, can be negative if the bitmap is bigger then the screen */
                x = (sz.cx / 2) - (gspv.cxWallpaper / 2);
                y = (sz.cy / 2) - (gspv.cyWallpaper / 2);
            }

            hWallpaperDC = NtGdiCreateCompatibleDC(hDC);
            if(hWallpaperDC != NULL)
            {
                HBITMAP hOldBitmap;

                /* Fill in the area that the bitmap is not going to cover */
                if (x > 0 || y > 0)
                {
                   /* FIXME: Clip out the bitmap
                      can be replaced with "NtGdiPatBlt(hDC, x, y, WinSta->cxWallpaper, WinSta->cyWallpaper, PATCOPY | DSTINVERT);"
                      once we support DSTINVERT */
                  PreviousBrush = NtGdiSelectBrush(hDC, DesktopBrush);
                  NtGdiPatBlt(hDC, Rect.left, Rect.top, Rect.right, Rect.bottom, PATCOPY);
                  NtGdiSelectBrush(hDC, PreviousBrush);
                }

                /* Do not fill the background after it is painted no matter the size of the picture */
                doPatBlt = FALSE;

                hOldBitmap = NtGdiSelectBitmap(hWallpaperDC, gspv.hbmWallpaper);

                if (gspv.WallpaperMode == wmStretch)
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
                                        gspv.cxWallpaper,
                                        gspv.cyWallpaper,
                                        SRCCOPY,
                                        0);
                }
                else if (gspv.WallpaperMode == wmTile)
                {
                    /* Paint the bitmap across the screen then down */
                    for(y = 0; y < Rect.bottom; y += gspv.cyWallpaper)
                    {
                        for(x = 0; x < Rect.right; x += gspv.cxWallpaper)
                        {
                            NtGdiBitBlt(hDC,
                                        x,
                                        y,
                                        gspv.cxWallpaper,
                                        gspv.cyWallpaper,
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
                                gspv.cxWallpaper,
                                gspv.cyWallpaper,
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
    else
    {
        /* Black desktop background in Safe Mode */
        DesktopBrush = StockObjects[BLACK_BRUSH];
    }
    /* Background is set to none, clear the screen */
    if (doPatBlt)
    {
        PreviousBrush = NtGdiSelectBrush(hDC, DesktopBrush);
        NtGdiPatBlt(hDC, Rect.left, Rect.top, Rect.right, Rect.bottom, PATCOPY);
        NtGdiSelectBrush(hDC, PreviousBrush);
    }

    /*
     * Display system version on the desktop background
     */

    if (g_PaintDesktopVersion || UserGetSystemMetrics(SM_CLEANBOOT))
    {
        static WCHAR s_wszVersion[256] = {0};
        RECTL rect;

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

            if(!UserGetSystemMetrics(SM_CLEANBOOT))
            {
                GreExtTextOutW(hDC, rect.right - 16, rect.bottom - 48, 0, NULL, s_wszVersion, len, NULL, 0);
            }
            else
            {
                /* Safe Mode */

                /* Version information text in top center */
                IntGdiSetTextAlign(hDC, TA_CENTER | TA_TOP);
                GreExtTextOutW(hDC, (rect.right + rect.left)/2, rect.top + 3, 0, NULL, s_wszVersion, len, NULL, 0);

                /* Safe Mode text in corners */
                len = wcslen(s_wszSafeMode);
                IntGdiSetTextAlign(hDC, TA_LEFT | TA_TOP);
                GreExtTextOutW(hDC, rect.left, rect.top + 3, 0, NULL, s_wszSafeMode, len, NULL, 0);
                IntGdiSetTextAlign(hDC, TA_RIGHT | TA_TOP);
                GreExtTextOutW(hDC, rect.right, rect.top + 3, 0, NULL, s_wszSafeMode, len, NULL, 0);
                IntGdiSetTextAlign(hDC, TA_LEFT | TA_BASELINE);
                GreExtTextOutW(hDC, rect.left, rect.bottom - 5, 0, NULL, s_wszSafeMode, len, NULL, 0);
                IntGdiSetTextAlign(hDC, TA_RIGHT | TA_BASELINE);
                GreExtTextOutW(hDC, rect.right, rect.bottom - 5, 0, NULL, s_wszSafeMode, len, NULL, 0);
            }

            IntGdiSetBkMode(hDC, mode_old);
            IntGdiSetTextAlign(hDC, align_old);
            IntGdiSetTextColor(hDC, color_old);
        }
    }
    return TRUE;
}

static NTSTATUS
UserInitializeDesktop(PDESKTOP pdesk, PUNICODE_STRING DesktopName, PWINSTATION_OBJECT pwinsta)
{
    PVOID DesktopHeapSystemBase = NULL;
    ULONG_PTR HeapSize = gdwDesktopSectionSize * 1024;
    SIZE_T DesktopInfoSize;
    ULONG i;

    TRACE("UserInitializeDesktop desktop 0x%p with name %wZ\n", pdesk, DesktopName);

    RtlZeroMemory(pdesk, sizeof(DESKTOP));

    /* Link the desktop with the parent window station */
    pdesk->rpwinstaParent = pwinsta;
    InsertTailList(&pwinsta->DesktopListHead, &pdesk->ListEntry);

    /* Create the desktop heap */
    pdesk->hsectionDesktop = NULL;
    pdesk->pheapDesktop = UserCreateHeap(&pdesk->hsectionDesktop,
                                         &DesktopHeapSystemBase,
                                         HeapSize);
   if (pdesk->pheapDesktop == NULL)
   {
       ERR("Failed to create desktop heap!\n");
       return STATUS_NO_MEMORY;
   }

   /* Create DESKTOPINFO */
   DesktopInfoSize = sizeof(DESKTOPINFO) + DesktopName->Length + sizeof(WCHAR);
   pdesk->pDeskInfo = RtlAllocateHeap(pdesk->pheapDesktop,
                                      HEAP_NO_SERIALIZE | HEAP_ZERO_MEMORY,
                                      DesktopInfoSize);
   if (pdesk->pDeskInfo == NULL)
   {
       ERR("Failed to create the DESKTOP structure!\n");
       return STATUS_NO_MEMORY;
   }

   /* Initialize the DESKTOPINFO */
   pdesk->pDeskInfo->pvDesktopBase = DesktopHeapSystemBase;
   pdesk->pDeskInfo->pvDesktopLimit = (PVOID)((ULONG_PTR)DesktopHeapSystemBase + HeapSize);
   RtlCopyMemory(pdesk->pDeskInfo->szDesktopName,
                 DesktopName->Buffer,
                 DesktopName->Length + sizeof(WCHAR));
   for (i = 0; i < NB_HOOKS; i++)
   {
      InitializeListHead(&pdesk->pDeskInfo->aphkStart[i]);
   }

   InitializeListHead(&pdesk->ShellHookWindows);
   InitializeListHead(&pdesk->PtiList);

   return STATUS_SUCCESS;
}

/* SYSCALLS *******************************************************************/

/*
 * NtUserCreateDesktop
 *
 * Creates a new desktop.
 *
 * Parameters
 *    poaAttribs
 *       Object Attributes.
 *
 *    lpszDesktopDevice
 *       Name of the device.
 *
 *    pDeviceMode
 *       Device Mode.
 *
 *    dwFlags
 *       Interaction flags.
 *
 *    dwDesiredAccess
 *       Requested type of access.
 *
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

HDESK APIENTRY
NtUserCreateDesktop(
   POBJECT_ATTRIBUTES ObjectAttributes,
   PUNICODE_STRING lpszDesktopDevice,
   LPDEVMODEW lpdmw,
   DWORD dwFlags,
   ACCESS_MASK dwDesiredAccess)
{
   PDESKTOP pdesk = NULL;
   NTSTATUS Status = STATUS_SUCCESS;
   HDESK hdesk;
   BOOLEAN Context = FALSE;
   UNICODE_STRING ClassName;
   LARGE_STRING WindowName;
   BOOL NoHooks = FALSE;
   PWND pWnd = NULL;
   CREATESTRUCTW Cs;
   PTHREADINFO ptiCurrent;
   PCLS pcls;

   DECLARE_RETURN(HDESK);

   TRACE("Enter NtUserCreateDesktop\n");
   UserEnterExclusive();

   ptiCurrent = PsGetCurrentThreadWin32Thread();
   ASSERT(ptiCurrent);
   ASSERT(gptiDesktopThread);

   /* Turn off hooks when calling any CreateWindowEx from inside win32k. */
   NoHooks = (ptiCurrent->TIF_flags & TIF_DISABLEHOOKS);
   ptiCurrent->TIF_flags |= TIF_DISABLEHOOKS;
   ptiCurrent->pClientInfo->dwTIFlags = ptiCurrent->TIF_flags;

   /*
    * Try to open already existing desktop
    */
   Status = ObOpenObjectByName(
               ObjectAttributes,
               ExDesktopObjectType,
               UserMode,
               NULL,
               dwDesiredAccess,
               (PVOID)&Context,
               (HANDLE*)&hdesk);
   if (!NT_SUCCESS(Status))
   {
      ERR("ObOpenObjectByName failed to open/create desktop\n");
      SetLastNtError(Status);
      RETURN(NULL);
   }

   /* In case the object was not created (eg if it existed), return now */
   if (Context == FALSE)
   {
       TRACE("NtUserCreateDesktop opened desktop %wZ\n", ObjectAttributes->ObjectName);
       RETURN( hdesk);
   }

   /* Reference the desktop */
   Status = ObReferenceObjectByHandle(hdesk,
                                      0,
                                      ExDesktopObjectType,
                                      KernelMode,
                                      (PVOID*)&pdesk,
                                      NULL);
   if (!NT_SUCCESS(Status))
   {
       ERR("Failed to reference desktop object\n");
       SetLastNtError(Status);
       RETURN(NULL);
   }

   /* Get the desktop window class. The thread desktop does not belong to any desktop
    * so the classes created there (including the desktop class) are allocated in the shared heap
    * It would cause problems if we used a class that belongs to the caller
    */
   ClassName.Buffer = WC_DESKTOP;
   ClassName.Length = 0;
   pcls = IntGetAndReferenceClass(&ClassName, 0, TRUE);
   if (pcls == NULL)
   {
      ASSERT(FALSE);
      RETURN(NULL);
   }

   RtlZeroMemory(&WindowName, sizeof(WindowName));
   RtlZeroMemory(&Cs, sizeof(Cs));
   Cs.x = UserGetSystemMetrics(SM_XVIRTUALSCREEN),
   Cs.y = UserGetSystemMetrics(SM_YVIRTUALSCREEN),
   Cs.cx = UserGetSystemMetrics(SM_CXVIRTUALSCREEN),
   Cs.cy = UserGetSystemMetrics(SM_CYVIRTUALSCREEN),
   Cs.style = WS_POPUP|WS_CLIPCHILDREN;
   Cs.hInstance = hModClient; // hModuleWin; // Server side winproc!
   Cs.lpszName = (LPCWSTR) &WindowName;
   Cs.lpszClass = (LPCWSTR) &ClassName;

   /* Use IntCreateWindow instead of co_UserCreateWindowEx cause the later expects a thread with a desktop */
   pWnd = IntCreateWindow(&Cs, &WindowName, pcls, NULL, NULL, NULL, pdesk);
   if (pWnd == NULL)
   {
      ERR("Failed to create desktop window for the new desktop\n");
      RETURN(NULL);
   }

   pdesk->dwSessionId = PsGetCurrentProcessSessionId();
   pdesk->DesktopWindow = pWnd->head.h;
   pdesk->pDeskInfo->spwnd = pWnd;
   pWnd->fnid = FNID_DESKTOP;

   ClassName.Buffer = MAKEINTATOM(gpsi->atomSysClass[ICLS_HWNDMESSAGE]);
   ClassName.Length = 0;
   pcls = IntGetAndReferenceClass(&ClassName, 0, TRUE);
   if (pcls == NULL)
   {
      ASSERT(FALSE);
      RETURN(NULL);
   }

   RtlZeroMemory(&WindowName, sizeof(WindowName));
   RtlZeroMemory(&Cs, sizeof(Cs));
   Cs.cx = Cs.cy = 100;
   Cs.style = WS_POPUP|WS_CLIPCHILDREN;
   Cs.hInstance = hModClient; // hModuleWin; // Server side winproc!
   Cs.lpszName = (LPCWSTR) &WindowName;
   Cs.lpszClass = (LPCWSTR) &ClassName;
   pWnd = IntCreateWindow(&Cs, &WindowName, pcls, NULL, NULL, NULL, pdesk);
   if (pWnd == NULL)
   {
      ERR("Failed to create message window for the new desktop\n");
      RETURN(NULL);
   }

   pdesk->spwndMessage = pWnd;
   pWnd->fnid = FNID_MESSAGEWND;

   /* Now,,,
      if !(WinStaObject->Flags & WSF_NOIO) is (not set) for desktop input output mode (see wiki)
      Create Tooltip. Saved in DesktopObject->spwndTooltip.
      Tooltip dwExStyle: WS_EX_TOOLWINDOW|WS_EX_TOPMOST
      hWndParent are spwndMessage. Use hModuleWin for server side winproc!
      The rest is same as message window.
      http://msdn.microsoft.com/en-us/library/bb760250(VS.85).aspx
   */
   RETURN( hdesk);

CLEANUP:
   if (pdesk != NULL)
   {
       ObDereferenceObject(pdesk);
   }
   if (_ret_ == NULL && hdesk != NULL)
   {
      ObCloseHandle(hdesk, UserMode);
   }
   if (!NoHooks)
   {
       ptiCurrent->TIF_flags &= ~TIF_DISABLEHOOKS;
       ptiCurrent->pClientInfo->dwTIFlags = ptiCurrent->TIF_flags;
   }
   TRACE("Leave NtUserCreateDesktop, ret=%p\n",_ret_);
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

HDESK APIENTRY
NtUserOpenDesktop(
   POBJECT_ATTRIBUTES ObjectAttributes,
   DWORD dwFlags,
   ACCESS_MASK dwDesiredAccess)
{
   NTSTATUS Status;
   HDESK Desktop;

   Status = ObOpenObjectByName(
               ObjectAttributes,
               ExDesktopObjectType,
               UserMode,
               NULL,
               dwDesiredAccess,
               NULL,
               (HANDLE*)&Desktop);

   if (!NT_SUCCESS(Status))
   {
      ERR("Failed to open desktop\n");
      SetLastNtError(Status);
      return 0;
   }

   TRACE("Opened desktop %S with handle 0x%p\n", ObjectAttributes->ObjectName->Buffer, Desktop);

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

HDESK APIENTRY
NtUserOpenInputDesktop(
   DWORD dwFlags,
   BOOL fInherit,
   ACCESS_MASK dwDesiredAccess)
{
   NTSTATUS Status;
   HDESK hdesk = NULL;
   ULONG HandleAttributes = 0;

   UserEnterExclusive();
   TRACE("Enter NtUserOpenInputDesktop gpdeskInputDesktop 0x%p\n",gpdeskInputDesktop);

   if(fInherit) HandleAttributes = OBJ_INHERIT;

   /* Create a new handle to the object */
   Status = ObOpenObjectByPointer(
               gpdeskInputDesktop,
               HandleAttributes,
               NULL,
               dwDesiredAccess,
               ExDesktopObjectType,
               UserMode,
               (PHANDLE)&hdesk);

   if (!NT_SUCCESS(Status))
   {
       ERR("Failed to open input desktop object\n");
       SetLastNtError(Status);
   }

   TRACE("NtUserOpenInputDesktop returning 0x%p\n",hdesk);
   UserLeave();
   return hdesk;
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

BOOL APIENTRY
NtUserCloseDesktop(HDESK hDesktop)
{
   PDESKTOP pdesk;
   NTSTATUS Status;
   DECLARE_RETURN(BOOL);

   TRACE("NtUserCloseDesktop called (0x%p)\n", hDesktop);
   UserEnterExclusive();

   if( hDesktop == gptiCurrent->hdesk || hDesktop == gptiCurrent->ppi->hdeskStartup)
   {
       ERR("Attempted to close thread desktop\n");
       EngSetLastError(ERROR_BUSY);
       RETURN(FALSE);
   }

   Status = IntValidateDesktopHandle( hDesktop, UserMode, 0, &pdesk);
   if (!NT_SUCCESS(Status))
   {
      ERR("Validation of desktop handle (0x%p) failed\n", hDesktop);
      RETURN(FALSE);
   }

   ObDereferenceObject(pdesk);

   Status = ZwClose(hDesktop);
   if (!NT_SUCCESS(Status))
   {
      ERR("Failed to close desktop handle 0x%p\n", hDesktop);
      SetLastNtError(Status);
      RETURN(FALSE);
   }

   RETURN(TRUE);

CLEANUP:
   TRACE("Leave NtUserCloseDesktop, ret=%i\n",_ret_);
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

BOOL APIENTRY
NtUserPaintDesktop(HDC hDC)
{
   BOOL Ret;
   UserEnterExclusive();
   TRACE("Enter NtUserPaintDesktop\n");
   Ret = IntPaintDesktop(hDC);
   TRACE("Leave NtUserPaintDesktop, ret=%i\n",Ret);
   UserLeave();
   return Ret;
}

/*
 * NtUserResolveDesktop
 *
 * The NtUserResolveDesktop function retrieves handles to the desktop and
 * the window station specified by the desktop path string.
 *
 * Parameters
 *    ProcessHandle
 *       Handle to a user process.
 *
 *    DesktopPath
 *       The desktop path string.
 *
 * Return Value
 *    Handle to the desktop (direct return value) and
 *    handle to the associated window station (by pointer).
 *    NULL in case of failure.
 *
 * Remarks
 *    Callable by CSRSS only.
 *
 * Status
 *    @implemented
 */

HDESK
APIENTRY
NtUserResolveDesktop(
    IN HANDLE ProcessHandle,
    IN PUNICODE_STRING DesktopPath,
    DWORD dwUnknown,
    OUT HWINSTA* phWinSta)
{
    NTSTATUS Status;
    PEPROCESS Process = NULL;
    HWINSTA hWinSta = NULL;
    HDESK hDesktop  = NULL;

    /* Allow only the Console Server to perform this operation (via CSRSS) */
    if (PsGetCurrentProcess() != gpepCSRSS)
        return NULL;

    /* Get the process object the user handle was referencing */
    Status = ObReferenceObjectByHandle(ProcessHandle,
                                       PROCESS_QUERY_INFORMATION,
                                       *PsProcessType,
                                       UserMode,
                                       (PVOID*)&Process,
                                       NULL);
    if (!NT_SUCCESS(Status)) return NULL;

    // UserEnterShared();

    _SEH2_TRY
    {
        UNICODE_STRING CapturedDesktopPath;

        /* Capture the user desktop path string */
        Status = IntSafeCopyUnicodeStringTerminateNULL(&CapturedDesktopPath,
                                                       DesktopPath);
        if (!NT_SUCCESS(Status)) _SEH2_YIELD(goto Quit);

        /* Call the internal function */
        Status = IntParseDesktopPath(Process,
                                     &CapturedDesktopPath,
                                     &hWinSta,
                                     &hDesktop);
        if (!NT_SUCCESS(Status))
        {
            ERR("IntParseDesktopPath failed, Status = 0x%08lx\n", Status);
            hWinSta  = NULL;
            hDesktop = NULL;
        }

        /* Return the window station handle */
        *phWinSta = hWinSta;

        /* Free the captured string */
        if (CapturedDesktopPath.Buffer)
            ExFreePoolWithTag(CapturedDesktopPath.Buffer, TAG_STRING);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

Quit:
    // UserLeave();

    /* Dereference the process object */
    ObDereferenceObject(Process);

    /* Return the desktop handle */
    return hDesktop;
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

BOOL APIENTRY
NtUserSwitchDesktop(HDESK hdesk)
{
   PDESKTOP pdesk;
   NTSTATUS Status;
   BOOL bRedrawDesktop;
   DECLARE_RETURN(BOOL);

   UserEnterExclusive();
   TRACE("Enter NtUserSwitchDesktop(0x%p)\n", hdesk);

   Status = IntValidateDesktopHandle( hdesk, UserMode, 0, &pdesk);
   if (!NT_SUCCESS(Status))
   {
      ERR("Validation of desktop handle (0x%p) failed\n", hdesk);
      RETURN(FALSE);
   }

   if (PsGetCurrentProcessSessionId() != pdesk->rpwinstaParent->dwSessionId)
   {
      ERR("NtUserSwitchDesktop called for a desktop of a different session\n");
      RETURN(FALSE);
   }

   if(pdesk == gpdeskInputDesktop)
   {
       WARN("NtUserSwitchDesktop called for active desktop\n");
       RETURN(TRUE);
   }

   /*
    * Don't allow applications switch the desktop if it's locked, unless the caller
    * is the logon application itself
    */
   if((pdesk->rpwinstaParent->Flags & WSS_LOCKED) &&
      gpidLogon != PsGetCurrentProcessId())
   {
      ObDereferenceObject(pdesk);
      ERR("Switching desktop 0x%p denied because the window station is locked!\n", hdesk);
      RETURN(FALSE);
   }

   if(pdesk->rpwinstaParent != InputWindowStation)
   {
      ObDereferenceObject(pdesk);
      ERR("Switching desktop 0x%p denied because desktop doesn't belong to the interactive winsta!\n", hdesk);
      RETURN(FALSE);
   }

   /* FIXME: Fail if the process is associated with a secured
             desktop such as Winlogon or Screen-Saver */
   /* FIXME: Connect to input device */

   TRACE("Switching from desktop 0x%p to 0x%p\n", gpdeskInputDesktop, pdesk);

   bRedrawDesktop = FALSE;

   /* The first time SwitchDesktop is called, gpdeskInputDesktop is NULL */
   if(gpdeskInputDesktop != NULL)
   {
       if((gpdeskInputDesktop->pDeskInfo->spwnd->style & WS_VISIBLE) == WS_VISIBLE)
           bRedrawDesktop = TRUE;

       /* Hide the previous desktop window */
       IntHideDesktop(gpdeskInputDesktop);
   }

   /* Set the active desktop in the desktop's window station. */
   InputWindowStation->ActiveDesktop = pdesk;

   /* Set the global state. */
   gpdeskInputDesktop = pdesk;

   /* Show the new desktop window */
   co_IntShowDesktop(pdesk, UserGetSystemMetrics(SM_CXSCREEN), UserGetSystemMetrics(SM_CYSCREEN), bRedrawDesktop);

   TRACE("SwitchDesktop gpdeskInputDesktop 0x%p\n",gpdeskInputDesktop);
   ObDereferenceObject(pdesk);

   RETURN(TRUE);

CLEANUP:
   TRACE("Leave NtUserSwitchDesktop, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}

/*
 * NtUserGetThreadDesktop
 *
 * Status
 *    @implemented
 */

HDESK APIENTRY
NtUserGetThreadDesktop(DWORD dwThreadId, DWORD Unknown1)
{
   NTSTATUS Status;
   PETHREAD Thread;
   PDESKTOP DesktopObject;
   HDESK Ret, hThreadDesktop;
   OBJECT_HANDLE_INFORMATION HandleInformation;
   DECLARE_RETURN(HDESK);

   UserEnterExclusive();
   TRACE("Enter NtUserGetThreadDesktop\n");

   if(!dwThreadId)
   {
      EngSetLastError(ERROR_INVALID_PARAMETER);
      RETURN(0);
   }

   Status = PsLookupThreadByThreadId((HANDLE)(DWORD_PTR)dwThreadId, &Thread);
   if(!NT_SUCCESS(Status))
   {
      EngSetLastError(ERROR_INVALID_PARAMETER);
      RETURN(0);
   }

   if(Thread->ThreadsProcess == PsGetCurrentProcess())
   {
      /* Just return the handle, we queried the desktop handle of a thread running
         in the same context */
      Ret = ((PTHREADINFO)Thread->Tcb.Win32Thread)->hdesk;
      ObDereferenceObject(Thread);
      RETURN(Ret);
   }

   /* Get the desktop handle and the desktop of the thread */
   if(!(hThreadDesktop = ((PTHREADINFO)Thread->Tcb.Win32Thread)->hdesk) ||
         !(DesktopObject = ((PTHREADINFO)Thread->Tcb.Win32Thread)->rpdesk))
   {
      ObDereferenceObject(Thread);
      ERR("Desktop information of thread 0x%x broken!?\n", dwThreadId);
      RETURN(NULL);
   }

   /* We could just use DesktopObject instead of looking up the handle, but latter
      may be a bit safer (e.g. when the desktop is being destroyed */
   /* Switch into the context of the thread we're trying to get the desktop from,
      so we can use the handle */
   KeAttachProcess(&Thread->ThreadsProcess->Pcb);
   Status = ObReferenceObjectByHandle(hThreadDesktop,
                                      GENERIC_ALL,
                                      ExDesktopObjectType,
                                      UserMode,
                                      (PVOID*)&DesktopObject,
                                      &HandleInformation);
   KeDetachProcess();

   /* The handle couldn't be found, there's nothing to get... */
   if(!NT_SUCCESS(Status))
   {
      ObDereferenceObject(Thread);
      RETURN(NULL);
   }

   /* Lookup our handle table if we can find a handle to the desktop object,
      if not, create one */
   Ret = IntGetDesktopObjectHandle(DesktopObject);

   /* All done, we got a valid handle to the desktop */
   ObDereferenceObject(DesktopObject);
   ObDereferenceObject(Thread);
   RETURN(Ret);

CLEANUP:
   TRACE("Leave NtUserGetThreadDesktop, ret=%p\n",_ret_);
   UserLeave();
   END_CLEANUP;
}

static NTSTATUS
IntUnmapDesktopView(IN PDESKTOP pdesk)
{
    PPROCESSINFO ppi;
    PW32HEAP_USER_MAPPING HeapMapping, *PrevLink;
    NTSTATUS Status = STATUS_SUCCESS;

    TRACE("IntUnmapDesktopView called for desktop object %p\n", pdesk);

    ppi = PsGetCurrentProcessWin32Process();

    /*
     * Unmap if we're the last thread using the desktop.
     * Start the search at the next mapping: skip the first entry
     * as it must be the global user heap mapping.
     */
    PrevLink = &ppi->HeapMappings.Next;
    HeapMapping = *PrevLink;
    while (HeapMapping != NULL)
    {
        if (HeapMapping->KernelMapping == (PVOID)pdesk->pheapDesktop)
        {
            if (--HeapMapping->Count == 0)
            {
                *PrevLink = HeapMapping->Next;

                TRACE("ppi 0x%p unmapped heap of desktop 0x%p\n", ppi, pdesk);
                Status = MmUnmapViewOfSection(PsGetCurrentProcess(),
                                              HeapMapping->UserMapping);

                ObDereferenceObject(pdesk);

                UserHeapFree(HeapMapping);
                break;
            }
        }

        PrevLink = &HeapMapping->Next;
        HeapMapping = HeapMapping->Next;
    }

    return Status;
}

static NTSTATUS
IntMapDesktopView(IN PDESKTOP pdesk)
{
    PPROCESSINFO ppi;
    PW32HEAP_USER_MAPPING HeapMapping, *PrevLink;
    PVOID UserBase = NULL;
    SIZE_T ViewSize = 0;
    LARGE_INTEGER Offset;
    NTSTATUS Status;

    TRACE("IntMapDesktopView called for desktop object 0x%p\n", pdesk);

    ppi = PsGetCurrentProcessWin32Process();

    /*
     * Find out if another thread already mapped the desktop heap.
     * Start the search at the next mapping: skip the first entry
     * as it must be the global user heap mapping.
     */
    PrevLink = &ppi->HeapMappings.Next;
    HeapMapping = *PrevLink;
    while (HeapMapping != NULL)
    {
        if (HeapMapping->KernelMapping == (PVOID)pdesk->pheapDesktop)
        {
            HeapMapping->Count++;
            return STATUS_SUCCESS;
        }

        PrevLink = &HeapMapping->Next;
        HeapMapping = HeapMapping->Next;
    }

    /* We're the first, map the heap */
    Offset.QuadPart = 0;
    Status = MmMapViewOfSection(pdesk->hsectionDesktop,
                                PsGetCurrentProcess(),
                                &UserBase,
                                0,
                                0,
                                &Offset,
                                &ViewSize,
                                ViewUnmap,
                                SEC_NO_CHANGE,
                                PAGE_EXECUTE_READ); /* Would prefer PAGE_READONLY, but thanks to RTL heaps... */
    if (!NT_SUCCESS(Status))
    {
        ERR("Failed to map desktop\n");
        return Status;
    }

    TRACE("ppi 0x%p mapped heap of desktop 0x%p\n", ppi, pdesk);

    /* Add the mapping */
    HeapMapping = UserHeapAlloc(sizeof(*HeapMapping));
    if (HeapMapping == NULL)
    {
        MmUnmapViewOfSection(PsGetCurrentProcess(), UserBase);
        ERR("UserHeapAlloc() failed!\n");
        return STATUS_NO_MEMORY;
    }

    HeapMapping->Next = NULL;
    HeapMapping->KernelMapping = (PVOID)pdesk->pheapDesktop;
    HeapMapping->UserMapping = UserBase;
    HeapMapping->Limit = ViewSize;
    HeapMapping->Count = 1;
    *PrevLink = HeapMapping;

    ObReferenceObject(pdesk);

    return STATUS_SUCCESS;
}

BOOL
IntSetThreadDesktop(IN HDESK hDesktop,
                    IN BOOL FreeOnFailure)
{
    PDESKTOP pdesk = NULL, pdeskOld;
    HDESK hdeskOld;
    PTHREADINFO pti;
    NTSTATUS Status;
    PCLIENTTHREADINFO pctiOld, pctiNew = NULL;
    PCLIENTINFO pci;

    ASSERT(NtCurrentTeb());

    TRACE("IntSetThreadDesktop hDesktop:0x%p, FOF:%i\n",hDesktop, FreeOnFailure);

    pti = PsGetCurrentThreadWin32Thread();
    pci = pti->pClientInfo;

    /* If the caller gave us a desktop, ensure it is valid */
    if(hDesktop != NULL)
    {
        /* Validate the new desktop. */
        Status = IntValidateDesktopHandle( hDesktop, UserMode, 0, &pdesk);
        if (!NT_SUCCESS(Status))
        {
            ERR("Validation of desktop handle (0x%p) failed\n", hDesktop);
            return FALSE;
        }

        if (pti->rpdesk == pdesk)
        {
            /* Nothing to do */
            ObDereferenceObject(pdesk);
            return TRUE;
        }
    }

    /* Make sure that we don't own any window in the current desktop */
    if (!IsListEmpty(&pti->WindowListHead))
    {
        if(pdesk)
            ObDereferenceObject(pdesk);
        ERR("Attempted to change thread desktop although the thread has windows!\n");
        EngSetLastError(ERROR_BUSY);
        return FALSE;
    }

    /* Desktop is being re-set so clear out foreground. */
    if (pti->rpdesk != pdesk && pti->MessageQueue == gpqForeground)
    {
       // Like above, there shouldn't be any windows, hooks or anything active on this threads desktop!
       IntSetFocusMessageQueue(NULL);
    }

    /* Before doing the switch, map the new desktop heap and allocate the new pcti */
    if(pdesk != NULL)
    {
        Status = IntMapDesktopView(pdesk);
        if (!NT_SUCCESS(Status))
        {
            ERR("Failed to map desktop heap!\n");
            ObDereferenceObject(pdesk);
            SetLastNtError(Status);
            return FALSE;
        }

        pctiNew = DesktopHeapAlloc( pdesk, sizeof(CLIENTTHREADINFO));
        if(pctiNew == NULL)
        {
            ERR("Failed to allocate new pcti\n");
            IntUnmapDesktopView(pdesk);
            ObDereferenceObject(pdesk);
            EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return FALSE;
        }
    }

    /* free all classes or move them to the shared heap */
    if(pti->rpdesk != NULL)
    {
        if(!IntCheckProcessDesktopClasses(pti->rpdesk, FreeOnFailure))
        {
            ERR("Failed to move process classes to shared heap!\n");
            if(pdesk)
            {
                DesktopHeapFree(pdesk, pctiNew);
                IntUnmapDesktopView(pdesk);
                ObDereferenceObject(pdesk);
            }
            return FALSE;
        }
    }

    pdeskOld = pti->rpdesk;
    hdeskOld = pti->hdesk;
    if (pti->pcti != &pti->cti)
       pctiOld = pti->pcti;
    else
       pctiOld = NULL;

    /* do the switch */
    if(pdesk != NULL)
    {
        pti->rpdesk = pdesk;
        pti->hdesk = hDesktop;
        pti->pDeskInfo = pti->rpdesk->pDeskInfo;
        pti->pcti = pctiNew;

        pci->ulClientDelta = DesktopHeapGetUserDelta();
        pci->pDeskInfo = (PVOID)((ULONG_PTR)pti->pDeskInfo - pci->ulClientDelta);
        pci->pClientThreadInfo = (PVOID)((ULONG_PTR)pti->pcti - pci->ulClientDelta);

        /* initialize the new pcti */
        if(pctiOld != NULL)
        {
            RtlCopyMemory(pctiNew, pctiOld, sizeof(CLIENTTHREADINFO));
        }
        else
        {
            RtlZeroMemory(pctiNew, sizeof(CLIENTTHREADINFO));
            pci->fsHooks = pti->fsHooks;
            pci->dwTIFlags = pti->TIF_flags;
        }
    }
    else
    {
        pti->rpdesk = NULL;
        pti->hdesk = NULL;
        pti->pDeskInfo = NULL;
        pti->pcti = &pti->cti; // Always point inside so there will be no crash when posting or sending msg's!
        pci->ulClientDelta = 0;
        pci->pDeskInfo = NULL;
        pci->pClientThreadInfo = NULL;
    }

    /* clean up the old desktop */
    if(pdeskOld != NULL)
    {
        RemoveEntryList(&pti->PtiLink);
        if (pctiOld) DesktopHeapFree(pdeskOld, pctiOld);
        IntUnmapDesktopView(pdeskOld);
        ObDereferenceObject(pdeskOld);
        ZwClose(hdeskOld);
    }

    if(pdesk)
    {
        InsertTailList(&pdesk->PtiList, &pti->PtiLink);
    }

    TRACE("IntSetThreadDesktop: pti 0x%p ppi 0x%p switched from object 0x%p to 0x%p\n", pti, pti->ppi, pdeskOld, pdesk);

    return TRUE;
}

/*
 * NtUserSetThreadDesktop
 *
 * Status
 *    @implemented
 */

BOOL APIENTRY
NtUserSetThreadDesktop(HDESK hDesktop)
{
   BOOL ret = FALSE;

   UserEnterExclusive();

   // FIXME: IntSetThreadDesktop validates the desktop handle, it should happen
   // here too and set the NT error level. Q. Is it necessary to have the validation
   // in IntSetThreadDesktop? Is it needed there too?
   if (hDesktop || (!hDesktop && PsGetCurrentProcess() == gpepCSRSS))
      ret = IntSetThreadDesktop(hDesktop, FALSE);

   UserLeave();

   return ret;
}

/* EOF */
