/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          NtUserCallXxx call stubs
 * FILE:             subsystem/win32/win32k/ntuser/simplecall.c
 * PROGRAMER:        Ge van Geldorp (ge@gse.nl)
 * REVISION HISTORY:
 *       2008/03/20  Split from misc.c
 */

#include <w32k.h>

#define NDEBUG
#include <debug.h>


/* registered Logon process */
PW32PROCESS LogonProcess = NULL;

BOOL FASTCALL
co_IntRegisterLogonProcess(HANDLE ProcessId, BOOL Register)
{
   PEPROCESS Process;
   NTSTATUS Status;
   CSR_API_MESSAGE Request;

   Status = PsLookupProcessByProcessId(ProcessId,
                                       &Process);
   if (!NT_SUCCESS(Status))
   {
      SetLastWin32Error(RtlNtStatusToDosError(Status));
      return FALSE;
   }

   if (Register)
   {
      /* Register the logon process */
      if (LogonProcess != NULL)
      {
         ObDereferenceObject(Process);
         return FALSE;
      }

      LogonProcess = (PW32PROCESS)Process->Win32Process;
   }
   else
   {
      /* Deregister the logon process */
      if (LogonProcess != (PW32PROCESS)Process->Win32Process)
      {
         ObDereferenceObject(Process);
         return FALSE;
      }

      LogonProcess = NULL;
   }

   ObDereferenceObject(Process);

   Request.Type = MAKE_CSR_API(REGISTER_LOGON_PROCESS, CSR_GUI);
   Request.Data.RegisterLogonProcessRequest.ProcessId = ProcessId;
   Request.Data.RegisterLogonProcessRequest.Register = Register;

   Status = co_CsrNotify(&Request);
   if (! NT_SUCCESS(Status))
   {
      DPRINT1("Failed to register logon process with CSRSS\n");
      return FALSE;
   }

   return TRUE;
}

/*
 * @unimplemented
 */
DWORD
APIENTRY
NtUserCallNoParam(DWORD Routine)
{
   DWORD Result = 0;
   DECLARE_RETURN(DWORD);

   DPRINT("Enter NtUserCallNoParam\n");
   UserEnterExclusive();

   switch(Routine)
   {
      case NOPARAM_ROUTINE_CREATEMENU:
         Result = (DWORD)UserCreateMenu(FALSE);
         break;

      case NOPARAM_ROUTINE_CREATEMENUPOPUP:
         Result = (DWORD)UserCreateMenu(TRUE);
         break;

      case NOPARAM_ROUTINE_DESTROY_CARET:
         Result = (DWORD)co_IntDestroyCaret(PsGetCurrentThread()->Tcb.Win32Thread);
         break;

      case NOPARAM_ROUTINE_INIT_MESSAGE_PUMP:
         Result = (DWORD)IntInitMessagePumpHook();
         break;

      case NOPARAM_ROUTINE_UNINIT_MESSAGE_PUMP:
         Result = (DWORD)IntUninitMessagePumpHook();
         break;

      case NOPARAM_ROUTINE_GETMESSAGEEXTRAINFO:
         Result = (DWORD)MsqGetMessageExtraInfo();
         break;

      case NOPARAM_ROUTINE_ANYPOPUP:
         Result = (DWORD)IntAnyPopup();
         break;

      case NOPARAM_ROUTINE_CSRSS_INITIALIZED:
         Result = (DWORD)CsrInit();
         break;

      case NOPARAM_ROUTINE_MSQCLEARWAKEMASK:
         RETURN( (DWORD)IntMsqClearWakeMask());

      default:
         DPRINT1("Calling invalid routine number 0x%x in NtUserCallNoParam\n", Routine);
         SetLastWin32Error(ERROR_INVALID_PARAMETER);
         break;
   }
   RETURN(Result);

CLEANUP:
   DPRINT("Leave NtUserCallNoParam, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}


/*
 * @implemented
 */
DWORD
APIENTRY
NtUserCallOneParam(
   DWORD Param,
   DWORD Routine)
{
   DECLARE_RETURN(DWORD);

   DPRINT("Enter NtUserCallOneParam\n");

   UserEnterExclusive();

   switch(Routine)
   {
      case ONEPARAM_ROUTINE_SHOWCURSOR:
         RETURN( (DWORD)UserShowCursor((BOOL)Param) );

      case ONEPARAM_ROUTINE_GETDESKTOPMAPPING:
         {
             PW32THREADINFO ti;
             ti = GetW32ThreadInfo();
             if (ti != NULL)
             {
                /* Try convert the pointer to a user mode pointer if the desktop is
                   mapped into the process */
                RETURN((DWORD)DesktopHeapAddressToUser((PVOID)Param));
             }
             else
             {
                RETURN(0);
             }
         }

      case ONEPARAM_ROUTINE_GETMENU:
         {
            PWINDOW_OBJECT Window;
            DWORD Result;

            if(!(Window = UserGetWindowObject((HWND)Param)))
            {
               RETURN( FALSE);
            }

            Result = (DWORD)Window->Wnd->IDMenu;

            RETURN( Result);
         }

      case ONEPARAM_ROUTINE_ISWINDOWUNICODE:
         {
            PWINDOW_OBJECT Window;
            DWORD Result;

            Window = UserGetWindowObject((HWND)Param);
            if(!Window)
            {
               RETURN( FALSE);
            }
            Result = Window->Wnd->Unicode;
            RETURN( Result);
         }

      case ONEPARAM_ROUTINE_WINDOWFROMDC:
         RETURN( (DWORD)IntWindowFromDC((HDC)Param));

      case ONEPARAM_ROUTINE_GETWNDCONTEXTHLPID:
         {
            PWINDOW_OBJECT Window;
            DWORD Result;

            Window = UserGetWindowObject((HWND)Param);
            if(!Window)
            {
               RETURN( FALSE);
            }

            Result = Window->Wnd->ContextHelpId;

            RETURN( Result);
         }

      case ONEPARAM_ROUTINE_SWAPMOUSEBUTTON:
         {
            PWINSTATION_OBJECT WinSta;
            NTSTATUS Status;
            DWORD Result;

            Status = IntValidateWindowStationHandle(PsGetCurrentProcess()->Win32WindowStation,
                                                    KernelMode,
                                                    0,
                                                    &WinSta);
            if (!NT_SUCCESS(Status))
               RETURN( (DWORD)FALSE);

            /* FIXME
            Result = (DWORD)IntSwapMouseButton(WinStaObject, (BOOL)Param); */
            Result = 0;

            ObDereferenceObject(WinSta);
            RETURN( Result);
         }

      case ONEPARAM_ROUTINE_SWITCHCARETSHOWING:
         RETURN( (DWORD)IntSwitchCaretShowing((PVOID)Param));

      case ONEPARAM_ROUTINE_SETCARETBLINKTIME:
         RETURN( (DWORD)IntSetCaretBlinkTime((UINT)Param));

      case ONEPARAM_ROUTINE_GETWINDOWINSTANCE:
         {
            PWINDOW_OBJECT Window;
            DWORD Result;

            if(!(Window = UserGetWindowObject((HWND)Param)))
            {
               RETURN( FALSE);
            }

            Result = (DWORD)Window->Wnd->Instance;
            RETURN( Result);
         }

      case ONEPARAM_ROUTINE_SETMESSAGEEXTRAINFO:
         RETURN( (DWORD)MsqSetMessageExtraInfo((LPARAM)Param));

      case ONEPARAM_ROUTINE_CREATECURICONHANDLE:
         {
            PCURICON_OBJECT CurIcon;
            PWINSTATION_OBJECT WinSta;

            WinSta = IntGetWinStaObj();
            if(WinSta == NULL)
            {
               RETURN(0);
            }

            if (!(CurIcon = IntCreateCurIconHandle(WinSta)))
            {
               SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
               ObDereferenceObject(WinSta);
               RETURN(0);
            }

            ObDereferenceObject(WinSta);
            RETURN((DWORD)CurIcon->Self);
         }

      case ONEPARAM_ROUTINE_GETCURSORPOSITION:
         {
            PWINSTATION_OBJECT WinSta;
            NTSTATUS Status;
            POINT Pos;

            if(!Param)
               RETURN( (DWORD)FALSE);
            Status = IntValidateWindowStationHandle(PsGetCurrentProcess()->Win32WindowStation,
                                                    KernelMode,
                                                    0,
                                                    &WinSta);
            if (!NT_SUCCESS(Status))
               RETURN( (DWORD)FALSE);

            /* FIXME - check if process has WINSTA_READATTRIBUTES */
            IntGetCursorLocation(WinSta, &Pos);

            Status = MmCopyToCaller((PPOINT)Param, &Pos, sizeof(POINT));
            if(!NT_SUCCESS(Status))
            {
               ObDereferenceObject(WinSta);
               SetLastNtError(Status);
               RETURN( FALSE);
            }

            ObDereferenceObject(WinSta);

            RETURN( (DWORD)TRUE);
         }

      case ONEPARAM_ROUTINE_ISWINDOWINDESTROY:
         {
            PWINDOW_OBJECT Window;
            DWORD Result;

            if(!(Window = UserGetWindowObject((HWND)Param)))
            {
               RETURN( FALSE);
            }

            Result = (DWORD)IntIsWindowInDestroy(Window);

            RETURN( Result);
         }

      case ONEPARAM_ROUTINE_ENABLEPROCWNDGHSTING:
         {
            BOOL Enable;
            PW32PROCESS Process = PsGetCurrentProcessWin32Process();

            if(Process != NULL)
            {
               Enable = (BOOL)(Param != 0);

               if(Enable)
               {
                  Process->Flags &= ~W32PF_NOWINDOWGHOSTING;
               }
               else
               {
                  Process->Flags |= W32PF_NOWINDOWGHOSTING;
               }

               RETURN( TRUE);
            }

            RETURN( FALSE);
         }

      case ONEPARAM_ROUTINE_MSQSETWAKEMASK:
         RETURN( (DWORD)IntMsqSetWakeMask(Param));

      case ONEPARAM_ROUTINE_GETKEYBOARDTYPE:
         RETURN( UserGetKeyboardType(Param));

      case ONEPARAM_ROUTINE_GETKEYBOARDLAYOUT:
         RETURN( (DWORD)UserGetKeyboardLayout(Param));

      case ONEPARAM_ROUTINE_REGISTERUSERMODULE:
      {
          PW32THREADINFO ti;

          ti = GetW32ThreadInfo();
          if (ti == NULL)
          {
              DPRINT1("Cannot register user32 module instance!\n");
              SetLastWin32Error(ERROR_INVALID_PARAMETER);
              RETURN(FALSE);
          }

          if (InterlockedCompareExchangePointer(&ti->kpi->hModUser,
                                                (HINSTANCE)Param,
                                                NULL) == NULL)
          {
              RETURN(TRUE);
          }
      }
      case ONEPARAM_ROUTINE_RELEASEDC:
         RETURN (UserReleaseDC(NULL, (HDC) Param, FALSE));

      case ONEPARAM_ROUTINE_REALIZEPALETTE:
         RETURN (UserRealizePalette((HDC) Param));

      case ONEPARAM_ROUTINE_GETQUEUESTATUS:
         RETURN (IntGetQueueStatus((BOOL) Param));

      case ONEPARAM_ROUTINE_ENUMCLIPBOARDFORMATS:
         /* FIXME: Should use UserEnterShared */
         RETURN(IntEnumClipboardFormats(Param));

       case ONEPARAM_ROUTINE_CSRSS_GUICHECK:
          IntUserManualGuiCheck(Param);
          RETURN(TRUE);
   }
   DPRINT1("Calling invalid routine number 0x%x in NtUserCallOneParam(), Param=0x%x\n",
           Routine, Param);
   SetLastWin32Error(ERROR_INVALID_PARAMETER);
   RETURN( 0);

CLEANUP:
   DPRINT("Leave NtUserCallOneParam, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}


/*
 * @implemented
 */
DWORD
APIENTRY
NtUserCallTwoParam(
   DWORD Param1,
   DWORD Param2,
   DWORD Routine)
{
   NTSTATUS Status;
   PWINDOW_OBJECT Window;
   DECLARE_RETURN(DWORD);

   DPRINT("Enter NtUserCallTwoParam\n");
   UserEnterExclusive();

   switch(Routine)
   {
      case TWOPARAM_ROUTINE_GETWINDOWRGNBOX:
         {
            DWORD Ret;
            RECT rcRect;
            Window = UserGetWindowObject((HWND)Param1);
            if (!Window) RETURN(ERROR);

            Ret = (DWORD)IntGetWindowRgnBox(Window, &rcRect);
            Status = MmCopyToCaller((PVOID)Param2, &rcRect, sizeof(RECT));
            if(!NT_SUCCESS(Status))
            {
               SetLastNtError(Status);
               RETURN( ERROR);
            }
            RETURN( Ret);
         }
      case TWOPARAM_ROUTINE_GETWINDOWRGN:
         {
            Window = UserGetWindowObject((HWND)Param1);
            if (!Window) RETURN(ERROR);

            RETURN( (DWORD)IntGetWindowRgn(Window, (HRGN)Param2));
         }
      case TWOPARAM_ROUTINE_SETMENUBARHEIGHT:
         {
            DWORD Ret;
            PMENU_OBJECT MenuObject = IntGetMenuObject((HMENU)Param1);
            if(!MenuObject)
               RETURN( 0);

            if(Param2 > 0)
            {
               Ret = (MenuObject->MenuInfo.Height == (int)Param2);
               MenuObject->MenuInfo.Height = (int)Param2;
            }
            else
               Ret = (DWORD)MenuObject->MenuInfo.Height;
            IntReleaseMenuObject(MenuObject);
            RETURN( Ret);
         }
      case TWOPARAM_ROUTINE_SETMENUITEMRECT:
         {
            BOOL Ret;
            SETMENUITEMRECT smir;
            PMENU_OBJECT MenuObject = IntGetMenuObject((HMENU)Param1);
            if(!MenuObject)
               RETURN( 0);

            if(!NT_SUCCESS(MmCopyFromCaller(&smir, (PVOID)Param2, sizeof(SETMENUITEMRECT))))
            {
               IntReleaseMenuObject(MenuObject);
               RETURN( 0);
            }

            Ret = IntSetMenuItemRect(MenuObject, smir.uItem, smir.fByPosition, &smir.rcRect);

            IntReleaseMenuObject(MenuObject);
            RETURN( (DWORD)Ret);
         }

      case TWOPARAM_ROUTINE_SETGUITHRDHANDLE:
         {
            PUSER_MESSAGE_QUEUE MsgQueue = ((PTHREADINFO)PsGetCurrentThread()->Tcb.Win32Thread)->MessageQueue;

            ASSERT(MsgQueue);
            RETURN( (DWORD)MsqSetStateWindow(MsgQueue, (ULONG)Param1, (HWND)Param2));
         }

      case TWOPARAM_ROUTINE_ENABLEWINDOW:
         UNIMPLEMENTED
         RETURN( 0);

      case TWOPARAM_ROUTINE_SHOWOWNEDPOPUPS:
      {
         Window = UserGetWindowObject((HWND)Param1);
         if (!Window) RETURN(0);

         RETURN( (DWORD)IntShowOwnedPopups(Window, (BOOL) Param2));
      }

      case TWOPARAM_ROUTINE_ROS_UPDATEUISTATE:
      {
          WPARAM wParam;
          Window = UserGetWindowObject((HWND)Param1);
          if (!Window) RETURN(0);

          /* Unpack wParam */
          wParam = MAKEWPARAM((Param2 >> 3) & 0x3,
                              Param2 & (UISF_HIDEFOCUS | UISF_HIDEACCEL | UISF_ACTIVE));

          RETURN( UserUpdateUiState(Window->Wnd, wParam) );
      }

      case TWOPARAM_ROUTINE_SWITCHTOTHISWINDOW:
         UNIMPLEMENTED
         RETURN( 0);

      case TWOPARAM_ROUTINE_SETWNDCONTEXTHLPID:

         if(!(Window = UserGetWindowObject((HWND)Param1)))
         {
            RETURN( (DWORD)FALSE);
         }

         Window->Wnd->ContextHelpId = Param2;

         RETURN( (DWORD)TRUE);

      case TWOPARAM_ROUTINE_SETCARETPOS:
         RETURN( (DWORD)co_IntSetCaretPos((int)Param1, (int)Param2));

      case TWOPARAM_ROUTINE_GETWINDOWINFO:
         {
            WINDOWINFO wi;
            DWORD Ret;

            if(!(Window = UserGetWindowObject((HWND)Param1)))
            {
               RETURN( FALSE);
            }

#if 0
            /*
             * According to WINE, Windows' doesn't check the cbSize field
             */

            Status = MmCopyFromCaller(&wi.cbSize, (PVOID)Param2, sizeof(wi.cbSize));
            if(!NT_SUCCESS(Status))
            {
               SetLastNtError(Status);
               RETURN( FALSE);
            }

            if(wi.cbSize != sizeof(WINDOWINFO))
            {
               SetLastWin32Error(ERROR_INVALID_PARAMETER);
               RETURN( FALSE);
            }
#endif

            if((Ret = (DWORD)IntGetWindowInfo(Window, &wi)))
            {
               Status = MmCopyToCaller((PVOID)Param2, &wi, sizeof(WINDOWINFO));
               if(!NT_SUCCESS(Status))
               {
                  SetLastNtError(Status);
                  RETURN( FALSE);
               }
            }

            RETURN( Ret);
         }

      case TWOPARAM_ROUTINE_REGISTERLOGONPROC:
         RETURN( (DWORD)co_IntRegisterLogonProcess((HANDLE)Param1, (BOOL)Param2));

      case TWOPARAM_ROUTINE_GETSYSCOLORBRUSHES:
      case TWOPARAM_ROUTINE_GETSYSCOLORPENS:
      case TWOPARAM_ROUTINE_GETSYSCOLORS:
         {
            DWORD Ret = 0;
            union
            {
               PVOID Pointer;
               HBRUSH *Brushes;
               HPEN *Pens;
               COLORREF *Colors;
            } Buffer;

            /* FIXME - we should make use of SEH here... */

            Buffer.Pointer = ExAllocatePool(PagedPool, Param2 * sizeof(HANDLE));
            if(Buffer.Pointer != NULL)
            {
               switch(Routine)
               {
                  case TWOPARAM_ROUTINE_GETSYSCOLORBRUSHES:
                     Ret = (DWORD)IntGetSysColorBrushes(Buffer.Brushes, (UINT)Param2);
                     break;
                  case TWOPARAM_ROUTINE_GETSYSCOLORPENS:
                     Ret = (DWORD)IntGetSysColorPens(Buffer.Pens, (UINT)Param2);
                     break;
                  case TWOPARAM_ROUTINE_GETSYSCOLORS:
                     Ret = (DWORD)IntGetSysColors(Buffer.Colors, (UINT)Param2);
                     break;
                  default:
                     Ret = 0;
                     break;
               }

               if(Ret > 0)
               {
                  Status = MmCopyToCaller((PVOID)Param1, Buffer.Pointer, Param2 * sizeof(HANDLE));
                  if(!NT_SUCCESS(Status))
                  {
                     SetLastNtError(Status);
                     Ret = 0;
                  }
               }

               ExFreePool(Buffer.Pointer);
            }
            RETURN( Ret);
         }

      case TWOPARAM_ROUTINE_ROS_REGSYSCLASSES:
      {
          DWORD Ret = 0;
          DWORD Count = Param1;
          PREGISTER_SYSCLASS RegSysClassArray = (PREGISTER_SYSCLASS)Param2;

          if (Count != 0 && RegSysClassArray != NULL)
          {
              _SEH2_TRY
              {
                  ProbeArrayForRead(RegSysClassArray,
                                    sizeof(RegSysClassArray[0]),
                                    Count,
                                    2);

                  Ret = (DWORD)UserRegisterSystemClasses(Count,
                                                         RegSysClassArray);
              }
              _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
              {
                  SetLastNtError(_SEH2_GetExceptionCode());
              }
              _SEH2_END;
          }

          RETURN( Ret);
      }
   }
   DPRINT1("Calling invalid routine number 0x%x in NtUserCallTwoParam(), Param1=0x%x Parm2=0x%x\n",
           Routine, Param1, Param2);
   SetLastWin32Error(ERROR_INVALID_PARAMETER);
   RETURN( 0);

CLEANUP:
   DPRINT("Leave NtUserCallTwoParam, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}


/*
 * @unimplemented
 */
BOOL
APIENTRY
NtUserCallHwndLock(
   HWND hWnd,
   DWORD Routine)
{
   BOOL Ret = 0;
   PWINDOW_OBJECT Window;
   PWINDOW Wnd;
   USER_REFERENCE_ENTRY Ref;
   DECLARE_RETURN(BOOLEAN);

   DPRINT("Enter NtUserCallHwndLock\n");
   UserEnterExclusive();

   if (!(Window = UserGetWindowObject(hWnd)))
   {
      RETURN( FALSE);
   }
   UserRefObjectCo(Window, &Ref);

   Wnd = Window->Wnd;

   /* FIXME: Routine can be 0x53 - 0x5E */
   switch (Routine)
   {
      case HWNDLOCK_ROUTINE_ARRANGEICONICWINDOWS:
         co_WinPosArrangeIconicWindows(Window);
         break;

      case HWNDLOCK_ROUTINE_DRAWMENUBAR:
         {
            PMENU_OBJECT Menu;
            DPRINT("HWNDLOCK_ROUTINE_DRAWMENUBAR\n");
            Ret = FALSE;
            if (!((Wnd->Style & (WS_CHILD | WS_POPUP)) != WS_CHILD))
               break;

            if(!(Menu = UserGetMenuObject((HMENU) Wnd->IDMenu)))
               break;

            Menu->MenuInfo.WndOwner = hWnd;
            Menu->MenuInfo.Height = 0;

            co_WinPosSetWindowPos(Window, 0, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE |
                                  SWP_NOACTIVATE | SWP_NOZORDER | SWP_FRAMECHANGED );

            Ret = TRUE;
            break;
         }

      case HWNDLOCK_ROUTINE_REDRAWFRAME:
         /* FIXME */
         break;

      case HWNDLOCK_ROUTINE_SETFOREGROUNDWINDOW:
         Ret = co_IntSetForegroundWindow(Window);
         break;

      case HWNDLOCK_ROUTINE_UPDATEWINDOW:
         /* FIXME */
         break;
   }

   UserDerefObjectCo(Window);

   RETURN( Ret);

CLEANUP:
   DPRINT("Leave NtUserCallHwndLock, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}

/*
 * @unimplemented
 */
HWND
APIENTRY
NtUserCallHwndOpt(
   HWND hWnd,
   DWORD Routine)
{
   switch (Routine)
   {
      case HWNDOPT_ROUTINE_SETPROGMANWINDOW:
         GetW32ThreadInfo()->Desktop->hProgmanWindow = hWnd;
         break;

      case HWNDOPT_ROUTINE_SETTASKMANWINDOW:
         GetW32ThreadInfo()->Desktop->hTaskManWindow = hWnd;
         break;
   }

   return hWnd;
}

DWORD
APIENTRY
NtUserCallHwnd(
   HWND hWnd,
   DWORD Routine)
{
   switch (Routine)
   {
      case HWND_ROUTINE_REGISTERSHELLHOOKWINDOW:
         if (IntIsWindow(hWnd))
            return IntRegisterShellHookWindow(hWnd);
         return FALSE;
         break;
      case HWND_ROUTINE_DEREGISTERSHELLHOOKWINDOW:
         if (IntIsWindow(hWnd))
            return IntDeRegisterShellHookWindow(hWnd);
         return FALSE;
   }
   UNIMPLEMENTED;

   return 0;
}

DWORD
APIENTRY
NtUserCallHwndParam(
   HWND hWnd,
   DWORD Param,
   DWORD Routine)
{

   switch (Routine)
   {
      case HWNDPARAM_ROUTINE_KILLSYSTEMTIMER:
          return IntKillTimer(hWnd, (UINT_PTR)Param, TRUE);
   }

   UNIMPLEMENTED;

   return 0;
}

DWORD
APIENTRY
NtUserCallHwndParamLock(
   HWND hWnd,
   DWORD Param,
   DWORD Routine)
{
   UNIMPLEMENTED;

   return 0;
}

/* EOF */
