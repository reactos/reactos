/* $Id$
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Misc User funcs
 * FILE:             subsys/win32k/ntuser/misc.c
 * PROGRAMER:        Ge van Geldorp (ge@gse.nl)
 * REVISION HISTORY:
 *       2003/05/22  Created
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
STDCALL
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
STDCALL
NtUserCallOneParam(
   DWORD Param,
   DWORD Routine)
{
   DECLARE_RETURN(DWORD);
   PDC dc;

   DPRINT("Enter NtUserCallOneParam\n");


   if (Routine == ONEPARAM_ROUTINE_SHOWCURSOR)
   {
      PWINSTATION_OBJECT WinSta = PsGetCurrentThreadWin32Thread()->Desktop->WindowStation;
      PSYSTEM_CURSORINFO CurInfo;

      HDC Screen;
      HBITMAP dcbmp;
      SURFOBJ *SurfObj;
      BITMAPOBJ *BitmapObj;
      GDIDEVICE *ppdev;
      GDIPOINTER *pgp;
      int showpointer=0;

      if(!(Screen = IntGetScreenDC()))
      {
        return showpointer; /* No mouse */
      }

      dc = DC_LockDc(Screen);

      if (!dc)
      {
        return showpointer; /* No mouse */
      }

      dcbmp = dc->w.hBitmap;
      DC_UnlockDc(dc);

      BitmapObj = BITMAPOBJ_LockBitmap(dcbmp);
      if ( !BitmapObj )
      {
         BITMAPOBJ_UnlockBitmap(BitmapObj);
         return showpointer; /* No Mouse */
      }

      SurfObj = &BitmapObj->SurfObj;
      if (SurfObj == NULL)
      {
        BITMAPOBJ_UnlockBitmap(BitmapObj);
        return showpointer; /* No mouse */
      }

      ppdev = GDIDEV(SurfObj);

      if(ppdev == NULL)
      {
        BITMAPOBJ_UnlockBitmap(BitmapObj);
        return showpointer; /* No mouse */
      }

      pgp = &ppdev->Pointer;

      CurInfo = IntGetSysCursorInfo(WinSta);

      if (Param == FALSE)
      {
          pgp->ShowPointer--;
          showpointer = pgp->ShowPointer;

          if (showpointer >= 0)
          {
             //ppdev->SafetyRemoveCount = 1;
             //ppdev->SafetyRemoveLevel = 1;
             EngMovePointer(SurfObj,-1,-1,NULL);
             CurInfo->ShowingCursor = 0;
           }

       }
       else
       {
          pgp->ShowPointer++;
          showpointer = pgp->ShowPointer;

          /* Show Cursor */
          if (showpointer < 0)
          {
             //ppdev->SafetyRemoveCount = 0;
             //ppdev->SafetyRemoveLevel = 0;
             EngMovePointer(SurfObj,-1,-1,NULL);
             CurInfo->ShowingCursor = CURSOR_SHOWING;
          }
       }

       BITMAPOBJ_UnlockBitmap(BitmapObj);
       return showpointer;
       }


   UserEnterExclusive();

   switch(Routine)
   {
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

            Result = Window->ContextHelpId;

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
/*
      case ONEPARAM_ROUTINE_ENUMCLIPBOARDFORMATS:
         RETURN( (DWORD)NtUserEnumClipboardFormats((UINT)Param));
*/
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
STDCALL
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
      case TWOPARAM_ROUTINE_SETDCPENCOLOR:
         {
            RETURN( (DWORD)IntSetDCColor((HDC)Param1, OBJ_PEN, (COLORREF)Param2));
         }
      case TWOPARAM_ROUTINE_SETDCBRUSHCOLOR:
         {
            RETURN( (DWORD)IntSetDCColor((HDC)Param1, OBJ_BRUSH, (COLORREF)Param2));
         }
      case TWOPARAM_ROUTINE_GETDCCOLOR:
         {
            RETURN( (DWORD)IntGetDCColor((HDC)Param1, (ULONG)Param2));
         }
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
            PUSER_MESSAGE_QUEUE MsgQueue = ((PW32THREAD)PsGetCurrentThread()->Tcb.Win32Thread)->MessageQueue;

            ASSERT(MsgQueue);
            RETURN( (DWORD)MsqSetStateWindow(MsgQueue, (ULONG)Param1, (HWND)Param2));
         }

      case TWOPARAM_ROUTINE_ENABLEWINDOW:
         UNIMPLEMENTED
         RETURN( 0);

      case TWOPARAM_ROUTINE_UNKNOWN:
         UNIMPLEMENTED
         RETURN( 0);

      case TWOPARAM_ROUTINE_SHOWOWNEDPOPUPS:
      {
         Window = UserGetWindowObject((HWND)Param1);
         if (!Window) RETURN(0);

         RETURN( (DWORD)IntShowOwnedPopups(Window, (BOOL) Param2));
      }

      case TWOPARAM_ROUTINE_ROS_SHOWWINDOW:
         {
#define WIN_NEEDS_SHOW_OWNEDPOPUP (0x00000040)
            DPRINT1("ROS_SHOWWINDOW\n");

            if (!(Window = UserGetWindowObject((HWND)Param1)))
            {
               RETURN( 1 );
            }

            if (Param2)
            {
               if (!(Window->Flags & WIN_NEEDS_SHOW_OWNEDPOPUP))
               {
                  RETURN( -1 );
               }
               Window->Flags &= ~WIN_NEEDS_SHOW_OWNEDPOPUP;
            }
            else
               Window->Flags |= WIN_NEEDS_SHOW_OWNEDPOPUP;

            DPRINT1("ROS_SHOWWINDOW ---> 0x%x\n",Window->Flags);
            RETURN( 0 );
         }

      case TWOPARAM_ROUTINE_SWITCHTOTHISWINDOW:
         UNIMPLEMENTED
         RETURN( 0);

      case TWOPARAM_ROUTINE_SETWNDCONTEXTHLPID:

         if(!(Window = UserGetWindowObject((HWND)Param1)))
         {
            RETURN( (DWORD)FALSE);
         }

         Window->ContextHelpId = Param2;

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

      case TWOPARAM_ROUTINE_SETSYSCOLORS:
         {
            DWORD Ret = 0;
            PVOID Buffer;
            struct
            {
               INT *Elements;
               COLORREF *Colors;
            }
            ChangeSysColors;

            /* FIXME - we should make use of SEH here... */

            Status = MmCopyFromCaller(&ChangeSysColors, (PVOID)Param1, sizeof(ChangeSysColors));
            if(!NT_SUCCESS(Status))
            {
               SetLastNtError(Status);
               RETURN( 0);
            }

            Buffer = ExAllocatePool(PagedPool, (Param2 * sizeof(INT)) + (Param2 * sizeof(COLORREF)));
            if(Buffer != NULL)
            {
               INT *Elements = (INT*)Buffer;
               COLORREF *Colors = (COLORREF*)Buffer + Param2;

               Status = MmCopyFromCaller(Elements, ChangeSysColors.Elements, Param2 * sizeof(INT));
               if(NT_SUCCESS(Status))
               {
                  Status = MmCopyFromCaller(Colors, ChangeSysColors.Colors, Param2 * sizeof(COLORREF));
                  if(NT_SUCCESS(Status))
                  {
                     Ret = (DWORD)IntSetSysColors((UINT)Param2, Elements, Colors);
                  }
                  else
                     SetLastNtError(Status);
               }
               else
                  SetLastNtError(Status);

               ExFreePool(Buffer);
            }


            RETURN( Ret);
         }

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
              _SEH_TRY
              {
                  ProbeArrayForRead(RegSysClassArray,
                                    sizeof(RegSysClassArray[0]),
                                    Count,
                                    2);

                  Ret = (DWORD)UserRegisterSystemClasses(Count,
                                                         RegSysClassArray);
              }
              _SEH_HANDLE
              {
                  SetLastNtError(_SEH_GetExceptionCode());
              }
              _SEH_END;
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
STDCALL
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
STDCALL
NtUserCallHwndOpt(
   HWND Param,
   DWORD Routine)
{
   switch (Routine)
   {
      case HWNDOPT_ROUTINE_SETPROGMANWINDOW:
         GetW32ThreadInfo()->Desktop->hProgmanWindow = Param;
         break;

      case HWNDOPT_ROUTINE_SETTASKMANWINDOW:
         GetW32ThreadInfo()->Desktop->hTaskManWindow = Param;
         break;
   }

   return Param;
}

/*
 * @unimplemented
 */
DWORD STDCALL
NtUserGetThreadState(
   DWORD Routine)
{
   DECLARE_RETURN(DWORD);

   DPRINT("Enter NtUserGetThreadState\n");
   if (Routine != THREADSTATE_GETTHREADINFO)
   {
       UserEnterShared();
   }
   else
   {
       UserEnterExclusive();
   }

   switch (Routine)
   {
      case THREADSTATE_GETTHREADINFO:
         GetW32ThreadInfo();
         RETURN(0);

      case THREADSTATE_FOCUSWINDOW:
         RETURN( (DWORD)IntGetThreadFocusWindow());
      case THREADSTATE_PROGMANWINDOW:
         RETURN( (DWORD)GetW32ThreadInfo()->Desktop->hProgmanWindow);
      case THREADSTATE_TASKMANWINDOW:
         RETURN( (DWORD)GetW32ThreadInfo()->Desktop->hTaskManWindow);
   }
   RETURN( 0);

CLEANUP:
   DPRINT("Leave NtUserGetThreadState, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}

VOID FASTCALL
IntGetFontMetricSetting(LPWSTR lpValueName, PLOGFONTW font)
{
   RTL_QUERY_REGISTRY_TABLE QueryTable[2];
   NTSTATUS Status;
   /* Firefox 1.0.7 depends on the lfHeight value being negative */
   static LOGFONTW DefaultFont = {
                                    -11, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
                                    0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS,
                                    L"MS Sans Serif"
                                 };

   RtlZeroMemory(&QueryTable, sizeof(QueryTable));

   QueryTable[0].Name = lpValueName;
   QueryTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT | RTL_QUERY_REGISTRY_REQUIRED;
   QueryTable[0].EntryContext = font;

   Status = RtlQueryRegistryValues(
               RTL_REGISTRY_USER,
               L"Control Panel\\Desktop\\WindowMetrics",
               QueryTable,
               NULL,
               NULL);

   if (!NT_SUCCESS(Status))
   {
      RtlCopyMemory(font, &DefaultFont, sizeof(LOGFONTW));
   }
}


ULONG FASTCALL
IntSystemParametersInfo(
   UINT uiAction,
   UINT uiParam,
   PVOID pvParam,
   UINT fWinIni)
{
   PWINSTATION_OBJECT WinStaObject;
   NTSTATUS Status;
   BOOL bChanged = FALSE;

   static BOOL bInitialized = FALSE;
   static LOGFONTW IconFont;
   static NONCLIENTMETRICSW pMetrics;
   static MINIMIZEDMETRICS MinimizedMetrics;
   static BOOL GradientCaptions = TRUE;
   static UINT FocusBorderHeight = 1;
   static UINT FocusBorderWidth = 1;
   static ANIMATIONINFO anim;

   if (!bInitialized)
   {
      RtlZeroMemory(&IconFont, sizeof(LOGFONTW));
      RtlZeroMemory(&pMetrics, sizeof(NONCLIENTMETRICSW));

      IntGetFontMetricSetting(L"CaptionFont", &pMetrics.lfCaptionFont);
      IntGetFontMetricSetting(L"SmCaptionFont", &pMetrics.lfSmCaptionFont);
      IntGetFontMetricSetting(L"MenuFont", &pMetrics.lfMenuFont);
      IntGetFontMetricSetting(L"StatusFont", &pMetrics.lfStatusFont);
      IntGetFontMetricSetting(L"MessageFont", &pMetrics.lfMessageFont);
      IntGetFontMetricSetting(L"IconFont", &IconFont);

      pMetrics.iBorderWidth = 1;
      pMetrics.iScrollWidth = UserGetSystemMetrics(SM_CXVSCROLL);
      pMetrics.iScrollHeight = UserGetSystemMetrics(SM_CYHSCROLL);
      pMetrics.iCaptionWidth = UserGetSystemMetrics(SM_CXSIZE);
      pMetrics.iCaptionHeight = UserGetSystemMetrics(SM_CYSIZE);
      pMetrics.iSmCaptionWidth = UserGetSystemMetrics(SM_CXSMSIZE);
      pMetrics.iSmCaptionHeight = UserGetSystemMetrics(SM_CYSMSIZE);
      pMetrics.iMenuWidth = UserGetSystemMetrics(SM_CXMENUSIZE);
      pMetrics.iMenuHeight = UserGetSystemMetrics(SM_CYMENUSIZE);
      pMetrics.cbSize = sizeof(NONCLIENTMETRICSW);

      MinimizedMetrics.cbSize = sizeof(MINIMIZEDMETRICS);
      MinimizedMetrics.iWidth = UserGetSystemMetrics(SM_CXMINIMIZED);
      MinimizedMetrics.iHorzGap = UserGetSystemMetrics(SM_CXMINSPACING);
      MinimizedMetrics.iVertGap = UserGetSystemMetrics(SM_CYMINSPACING);
      MinimizedMetrics.iArrange = ARW_HIDE;

      bInitialized = TRUE;
   }

   switch(uiAction)
   {
     case SPI_GETDRAGFULLWINDOWS:
           /* FIXME: Implement this, don't just return constant */
           *(PBOOL)pvParam = FALSE;
           break;



      case SPI_GETKEYBOARDCUES:
      case SPI_SETDOUBLECLKWIDTH:
      case SPI_SETDOUBLECLKHEIGHT:
      case SPI_SETDOUBLECLICKTIME:
      case SPI_SETDESKWALLPAPER:
      case SPI_SETSCREENSAVERRUNNING:
      case SPI_SETSCREENSAVETIMEOUT:
      case SPI_SETFLATMENU:
      case SPI_SETMOUSEHOVERTIME:
      case SPI_SETMOUSEHOVERWIDTH:
      case SPI_SETMOUSEHOVERHEIGHT:
      case SPI_SETMOUSE:
      case SPI_SETMOUSESPEED:
      case SPI_SETMOUSEBUTTONSWAP:
         /* We will change something, so set the flag here */
         bChanged = TRUE;
      case SPI_GETDESKWALLPAPER:
      case SPI_GETWHEELSCROLLLINES:
      case SPI_GETWHEELSCROLLCHARS:
      case SPI_GETSCREENSAVERRUNNING:
      case SPI_GETSCREENSAVETIMEOUT:
      case SPI_GETSCREENSAVEACTIVE:
      case SPI_GETFLATMENU:
      case SPI_GETMOUSEHOVERTIME:
      case SPI_GETMOUSEHOVERWIDTH:
      case SPI_GETMOUSEHOVERHEIGHT:
      case SPI_GETMOUSE:
      case SPI_GETMOUSESPEED:
         {
            PSYSTEM_CURSORINFO CurInfo;

            Status = IntValidateWindowStationHandle(PsGetCurrentProcess()->Win32WindowStation,
                                                    KernelMode,
                                                    0,
                                                    &WinStaObject);
            if(!NT_SUCCESS(Status))
            {
               SetLastNtError(Status);
               return (DWORD)FALSE;
            }

            switch(uiAction)
            {
               case SPI_GETKEYBOARDCUES:
                  ASSERT(pvParam);
                  *((BOOL*)pvParam) = TRUE;
               case SPI_GETFLATMENU:
                  ASSERT(pvParam);
                  *((UINT*)pvParam) = WinStaObject->FlatMenu;
                  break;
               case SPI_SETFLATMENU:
                  WinStaObject->FlatMenu = (BOOL)pvParam;
                  break;
               case SPI_GETSCREENSAVETIMEOUT:
                   ASSERT(pvParam);
                   *((UINT*)pvParam) = WinStaObject->ScreenSaverTimeOut;
               break;
               case SPI_SETSCREENSAVETIMEOUT:
                  WinStaObject->ScreenSaverTimeOut = uiParam;
                  break;
               case SPI_GETSCREENSAVERRUNNING:
                  if (pvParam != NULL) *((BOOL*)pvParam) = WinStaObject->ScreenSaverRunning;
                  break;
               case SPI_SETSCREENSAVERRUNNING:
                  if (pvParam != NULL) *((BOOL*)pvParam) = WinStaObject->ScreenSaverRunning;
                  WinStaObject->ScreenSaverRunning = uiParam;
                  break;
               case SPI_SETSCREENSAVEACTIVE:
                  WinStaObject->ScreenSaverActive = uiParam;
                  break;
               case SPI_GETSCREENSAVEACTIVE:
                  if (pvParam != NULL) *((BOOL*)pvParam) = WinStaObject->ScreenSaverActive;
                  break;
               case SPI_GETWHEELSCROLLLINES:
                  ASSERT(pvParam);
                  CurInfo = IntGetSysCursorInfo(WinStaObject);
                  *((UINT*)pvParam) = CurInfo->WheelScroLines;
                  /* FIXME add this value to scroll list as scroll value ?? */
                  break;
               case SPI_GETWHEELSCROLLCHARS:
                  ASSERT(pvParam);
                  CurInfo = IntGetSysCursorInfo(WinStaObject);
                  *((UINT*)pvParam) = CurInfo->WheelScroChars;
                  // FIXME add this value to scroll list as scroll value ??
                  break;
               case SPI_SETDOUBLECLKWIDTH:
                  CurInfo = IntGetSysCursorInfo(WinStaObject);
                  /* FIXME limit the maximum value? */
                  CurInfo->DblClickWidth = uiParam;
                  break;
               case SPI_SETDOUBLECLKHEIGHT:
                  CurInfo = IntGetSysCursorInfo(WinStaObject);
                  /* FIXME limit the maximum value? */
                  CurInfo->DblClickHeight = uiParam;
                  break;
               case SPI_SETDOUBLECLICKTIME:
                  CurInfo = IntGetSysCursorInfo(WinStaObject);
                  /* FIXME limit the maximum time to 1000 ms? */
                  CurInfo->DblClickSpeed = uiParam;
                  break;
               case SPI_GETMOUSEHOVERTIME:
                   CurInfo = IntGetSysCursorInfo(WinStaObject);
                   *((UINT*)pvParam) = CurInfo->MouseHoverTime;
                   break;
               case SPI_SETMOUSEHOVERTIME:
                   /* see http://msdn2.microsoft.com/en-us/library/ms724947.aspx
                    * copy text from it, if some agument why xp and 2003 behovir diffent
                    * only if they do not have SP install
                    * " Windows Server 2003 and Windows XP: The operating system does not
                    *   enforce the use of USER_TIMER_MAXIMUM and USER_TIMER_MINIMUM until
                    *   Windows Server 2003 SP1 and Windows XP SP2 "
                    */
                  CurInfo = IntGetSysCursorInfo(WinStaObject);
                  CurInfo->MouseHoverTime = uiParam;
                  if(CurInfo->MouseHoverTime < USER_TIMER_MINIMUM)
                  {
                      CurInfo->MouseHoverTime = USER_TIMER_MINIMUM;
                  }
                  if(CurInfo->MouseHoverTime > USER_TIMER_MAXIMUM)
                  {
                      CurInfo->MouseHoverTime = USER_TIMER_MAXIMUM;
                  }

                  break;
               case SPI_GETMOUSEHOVERWIDTH:
                   CurInfo = IntGetSysCursorInfo(WinStaObject);
                   *(PUINT)pvParam = CurInfo->MouseHoverWidth;
                   break;
               case SPI_GETMOUSEHOVERHEIGHT:
                   CurInfo = IntGetSysCursorInfo(WinStaObject);
                   *(PUINT)pvParam = CurInfo->MouseHoverHeight;
                   break;
               case SPI_SETMOUSEHOVERWIDTH:
                  CurInfo = IntGetSysCursorInfo(WinStaObject);
                  CurInfo->MouseHoverWidth = uiParam;
                  break;
               case SPI_SETMOUSEHOVERHEIGHT:
                   CurInfo = IntGetSysCursorInfo(WinStaObject);
                   CurInfo->MouseHoverHeight = uiParam;
                   break;
               case SPI_SETMOUSEBUTTONSWAP:
                   CurInfo = IntGetSysCursorInfo(WinStaObject);
                   CurInfo->SwapButtons = uiParam;
                   break;
               case SPI_SETMOUSE:
                   CurInfo = IntGetSysCursorInfo(WinStaObject);
                   CurInfo->CursorAccelerationInfo = *(PCURSORACCELERATION_INFO)pvParam;
                   break;
               case SPI_GETMOUSE:
                   CurInfo = IntGetSysCursorInfo(WinStaObject);
                   *(PCURSORACCELERATION_INFO)pvParam = CurInfo->CursorAccelerationInfo;
                   break;
               case SPI_SETMOUSESPEED:
                   CurInfo = IntGetSysCursorInfo(WinStaObject);
                   CurInfo->MouseSpeed = (UINT)pvParam;
                   /* Limit value to 1...20 range */
                   if(CurInfo->MouseSpeed < 1)
                   {
                       CurInfo->MouseSpeed = 1;
                   }
                   else if(CurInfo->MouseSpeed > 20)
                   {
                       CurInfo->MouseSpeed = 20;
                   }
                   break;
               case SPI_GETMOUSESPEED:
                   CurInfo = IntGetSysCursorInfo(WinStaObject);
                   *(PUINT)pvParam = CurInfo->MouseSpeed;
                   break;
               case SPI_SETDESKWALLPAPER:
                  {
                     /* This function expects different parameters than the user mode version!

                        We let the user mode code load the bitmap, it passed the handle to
                        the bitmap. We'll change it's ownership to system and replace it with
                        the current wallpaper bitmap */
                     HBITMAP hOldBitmap, hNewBitmap;
                     UNICODE_STRING Key = RTL_CONSTANT_STRING(L"Control Panel\\Desktop");
                     UNICODE_STRING Tile = RTL_CONSTANT_STRING(L"TileWallpaper");
                     UNICODE_STRING Style = RTL_CONSTANT_STRING(L"WallpaperStyle");
                     UNICODE_STRING KeyPath;
                     OBJECT_ATTRIBUTES KeyAttributes;
                     OBJECT_ATTRIBUTES ObjectAttributes;
                     NTSTATUS Status;
                     HANDLE CurrentUserKey = NULL;
                     HANDLE KeyHandle = NULL;
                     PKEY_VALUE_PARTIAL_INFORMATION KeyValuePartialInfo;
                     ULONG Length = 0;
                     ULONG ResLength = 0;
                     ULONG TileNum = 0;
                     ULONG StyleNum = 0;
                     ASSERT(pvParam);

                     hNewBitmap = *(HBITMAP*)pvParam;
                     if(hNewBitmap != NULL)
                     {
                        BITMAPOBJ *bmp;
                        /* try to get the size of the wallpaper */
                        if(!(bmp = BITMAPOBJ_LockBitmap(hNewBitmap)))
                        {
                           ObDereferenceObject(WinStaObject);
                           return FALSE;
                        }
                        WinStaObject->cxWallpaper = bmp->SurfObj.sizlBitmap.cx;
                        WinStaObject->cyWallpaper = bmp->SurfObj.sizlBitmap.cy;

                        BITMAPOBJ_UnlockBitmap(bmp);

                        /* change the bitmap's ownership */
                        GDIOBJ_SetOwnership(GdiHandleTable, hNewBitmap, NULL);
                     }
                     hOldBitmap = (HBITMAP)InterlockedExchange((LONG*)&WinStaObject->hbmWallpaper, (LONG)hNewBitmap);
                     if(hOldBitmap != NULL)
                     {
                        /* delete the old wallpaper */
                        NtGdiDeleteObject(hOldBitmap);
                     }

                     /* Set the style */
                     /*default value is center */
                     WinStaObject->WallpaperMode = wmCenter;

                     /* Get a handle to the current users settings */
                     RtlFormatCurrentUserKeyPath(&KeyPath);
                     InitializeObjectAttributes(&ObjectAttributes,&KeyPath,OBJ_CASE_INSENSITIVE,NULL,NULL);
                     ZwOpenKey(&CurrentUserKey, KEY_READ, &ObjectAttributes);
                     RtlFreeUnicodeString(&KeyPath);

                     /* open up the settings to read the values */
                     InitializeObjectAttributes(&KeyAttributes, &Key, OBJ_CASE_INSENSITIVE,
                              CurrentUserKey, NULL);
                     ZwOpenKey(&KeyHandle, KEY_READ, &KeyAttributes);
                     ZwClose(CurrentUserKey);

                     /* read the tile value in the registry */
                     Status = ZwQueryValueKey(KeyHandle, &Tile, KeyValuePartialInformation,
                                              0, 0, &ResLength);

                     /* fall back to .DEFAULT if we didnt find values */
                     if(Status == STATUS_INVALID_HANDLE)
                     {
                        RtlInitUnicodeString (&KeyPath,L"\\Registry\\User\\.Default\\Control Panel\\Desktop");
                        InitializeObjectAttributes(&KeyAttributes, &KeyPath, OBJ_CASE_INSENSITIVE,
                                                   NULL, NULL);
                        ZwOpenKey(&KeyHandle, KEY_READ, &KeyAttributes);
                        ZwQueryValueKey(KeyHandle, &Tile, KeyValuePartialInformation,
                                        0, 0, &ResLength);
                     }

                     ResLength += sizeof(KEY_VALUE_PARTIAL_INFORMATION);
                     KeyValuePartialInfo = ExAllocatePoolWithTag(PagedPool, ResLength, TAG_STRING);
                     Length = ResLength;

                     if(!KeyValuePartialInfo)
                     {
                        NtClose(KeyHandle);
                        return FALSE;
                     }

                     Status = ZwQueryValueKey(KeyHandle, &Tile, KeyValuePartialInformation,
                                              (PVOID)KeyValuePartialInfo, Length, &ResLength);
                     if(!NT_SUCCESS(Status) || (KeyValuePartialInfo->Type != REG_SZ))
                     {
                        ZwClose(KeyHandle);
                        ExFreePool(KeyValuePartialInfo);
                        return FALSE;
                     }

                     Tile.Length = KeyValuePartialInfo->DataLength;
                     Tile.MaximumLength = KeyValuePartialInfo->DataLength;
                     Tile.Buffer = (PWSTR)KeyValuePartialInfo->Data;

                     Status = RtlUnicodeStringToInteger(&Tile, 0, &TileNum);
                     if(!NT_SUCCESS(Status))
                     {
                        TileNum = 0;
                     }
                     ExFreePool(KeyValuePartialInfo);

                     /* start over again and look for the style*/
                     ResLength = 0;
                     Status = ZwQueryValueKey(KeyHandle, &Style, KeyValuePartialInformation,
                                              0, 0, &ResLength);

                     ResLength += sizeof(KEY_VALUE_PARTIAL_INFORMATION);
                     KeyValuePartialInfo = ExAllocatePoolWithTag(PagedPool, ResLength, TAG_STRING);
                     Length = ResLength;

                     if(!KeyValuePartialInfo)
                     {
                        ZwClose(KeyHandle);
                        return FALSE;
                     }

                     Status = ZwQueryValueKey(KeyHandle, &Style, KeyValuePartialInformation,
                                              (PVOID)KeyValuePartialInfo, Length, &ResLength);
                     if(!NT_SUCCESS(Status) || (KeyValuePartialInfo->Type != REG_SZ))
                     {
                        ZwClose(KeyHandle);
                        ExFreePool(KeyValuePartialInfo);
                        return FALSE;
                     }

                     Style.Length = KeyValuePartialInfo->DataLength;
                     Style.MaximumLength = KeyValuePartialInfo->DataLength;
                     Style.Buffer = (PWSTR)KeyValuePartialInfo->Data;

                     Status = RtlUnicodeStringToInteger(&Style, 0, &StyleNum);
                     if(!NT_SUCCESS(Status))
                     {
                        StyleNum = 0;
                     }
                     ExFreePool(KeyValuePartialInfo);

                     /* Check the values we found in the registry */
                     if(TileNum && !StyleNum)
                     {
                        WinStaObject->WallpaperMode = wmTile;
                     }
                     else if(!TileNum && StyleNum == 2)
                     {
                        WinStaObject->WallpaperMode = wmStretch;
                     }

                     ZwClose(KeyHandle);
                     break;
                  }
               case SPI_GETDESKWALLPAPER:
                  /* This function expects different parameters than the user mode version!
                     We basically return the current wallpaper handle - if any. The user
                     mode version should load the string from the registry and return it
                     without calling this function */

                  ASSERT(pvParam);
                  *(HBITMAP*)pvParam = (HBITMAP)WinStaObject->hbmWallpaper;
                  break;
            }

            /* FIXME save the value to the registry */

            ObDereferenceObject(WinStaObject);
            break;
         }
      case SPI_SETWORKAREA:
         {
            RECT *rc;
            PDESKTOP_OBJECT Desktop = PsGetCurrentThreadWin32Thread()->Desktop;

            if(!Desktop)
            {
               /* FIXME - Set last error */
               return FALSE;
            }

            ASSERT(pvParam);
            rc = (RECT*)pvParam;
            Desktop->WorkArea = *rc;
            bChanged = TRUE;

            break;
         }
      case SPI_GETWORKAREA:
         {
            PDESKTOP_OBJECT Desktop = PsGetCurrentThreadWin32Thread()->Desktop;

            if(!Desktop)
            {
               /* FIXME - Set last error */
               return FALSE;
            }

            ASSERT(pvParam);
            IntGetDesktopWorkArea(Desktop, (PRECT)pvParam);

            break;
         }
      case SPI_SETGRADIENTCAPTIONS:
         {
            GradientCaptions = (pvParam != NULL);
            /* FIXME - should be checked if the color depth is higher than 8bpp? */
            bChanged = TRUE;
            break;
         }
      case SPI_GETGRADIENTCAPTIONS:
         {
            HDC hDC;
            BOOL Ret = GradientCaptions;

            hDC = IntGetScreenDC();
            if(!hDC)
            {
               return FALSE;
            }
            Ret = (NtGdiGetDeviceCaps(hDC, BITSPIXEL) > 8) && Ret;

            ASSERT(pvParam);
            *((PBOOL)pvParam) = Ret;
            break;
         }
      case SPI_SETFONTSMOOTHING:
         {
            IntEnableFontRendering(uiParam != 0);
            bChanged = TRUE;
            break;
         }
      case SPI_GETFONTSMOOTHING:
         {
            ASSERT(pvParam);
            *((BOOL*)pvParam) = IntIsFontRenderingEnabled();
            break;
         }
      case SPI_GETICONTITLELOGFONT:
         {
            ASSERT(pvParam);
            *((LOGFONTW*)pvParam) = IconFont;
            break;
         }
      case SPI_GETNONCLIENTMETRICS:
         {
            ASSERT(pvParam);
            *((NONCLIENTMETRICSW*)pvParam) = pMetrics;
            break;
         }
      case SPI_GETANIMATION:
         {
            ASSERT(pvParam);
            *(( ANIMATIONINFO*)pvParam) = anim;
            break;
         }
      case SPI_SETANIMATION:
         {
            ASSERT(pvParam);
            anim = *((ANIMATIONINFO*)pvParam);
            bChanged = TRUE;
         }
      case SPI_SETNONCLIENTMETRICS:
         {
            ASSERT(pvParam);
            pMetrics = *((NONCLIENTMETRICSW*)pvParam);
            bChanged = TRUE;
            break;
         }
      case SPI_GETMINIMIZEDMETRICS:
         {
            ASSERT(pvParam);
            *((MINIMIZEDMETRICS*)pvParam) = MinimizedMetrics;
            break;
         }
      case SPI_SETMINIMIZEDMETRICS:
         {
            ASSERT(pvParam);
            MinimizedMetrics = *((MINIMIZEDMETRICS*)pvParam);
            bChanged = TRUE;
            break;
         }
      case SPI_GETFOCUSBORDERHEIGHT:
         {
            ASSERT(pvParam);
            *((UINT*)pvParam) = FocusBorderHeight;
            break;
         }
      case SPI_GETFOCUSBORDERWIDTH:
         {
            ASSERT(pvParam);
            *((UINT*)pvParam) = FocusBorderWidth;
            break;
         }
      case SPI_SETFOCUSBORDERHEIGHT:
         {
            FocusBorderHeight = (UINT)pvParam;
            bChanged = TRUE;
            break;
         }
      case SPI_SETFOCUSBORDERWIDTH:
         {
            FocusBorderWidth = (UINT)pvParam;
            bChanged = TRUE;
            break;
         }

      default:
         {
             DPRINT1("FIXME: Unsupported SPI Action 0x%x (uiParam: 0x%x, pvParam: 0x%x, fWinIni: 0x%x)\n",
                    uiAction, uiParam, pvParam, fWinIni);
            return FALSE;
         }
   }
   /* Did we change something ? */
   if (bChanged)
   {
      /* Shall we send a WM_SETTINGCHANGE message ? */
      if (fWinIni & (SPIF_UPDATEINIFILE | SPIF_SENDCHANGE))
      {
         /* Broadcast WM_SETTINGCHANGE to all toplevel windows */
         /* FIXME: lParam should be pointer to a string containing the reg key */
         UserPostMessage(HWND_BROADCAST, WM_SETTINGCHANGE, (WPARAM)uiAction, 0);
      }
   }
   return TRUE;
}

static BOOL
UserSystemParametersInfo_StructSet(
    UINT uiAction,
    UINT uiParam,
    PVOID pvParam,
    UINT fWinIni,
    PVOID pBuffer, /* private kmode buffer */
    UINT cbSize    /* size of buffer and expected size usermode data, pointed by pvParam  */
    )
{
    NTSTATUS Status = STATUS_SUCCESS;

    DPRINT("UserSystemParametersInfo_StructSet SPI Action 0x%x (uiParam: 0x%x, fWinIni: 0x%x)\n",
        uiAction, uiParam, fWinIni);

    _SEH_TRY
    {
        ProbeForRead(pvParam, cbSize, 1);
        RtlCopyMemory(pBuffer,pvParam,cbSize);
    }
    _SEH_HANDLE
    {
        Status = _SEH_GetExceptionCode();
    }
    _SEH_END;
    if(!NT_SUCCESS(Status))
    {
        SetLastNtError(Status);
        return( FALSE);
    }
    if(*(PUINT)pBuffer != cbSize)
    {
        SetLastWin32Error(ERROR_INVALID_PARAMETER);
        return( FALSE);
    }
    return IntSystemParametersInfo(uiAction, uiParam, pBuffer, fWinIni);
}

static BOOL
UserSystemParametersInfo_StructGet(
    UINT uiAction,
    UINT uiParam,
    PVOID pvParam,
    UINT fWinIni,
    PVOID pBuffer, /* private kmode buffer */
    UINT cbSize    /* size of buffer and expected size usermode data, pointed by pvParam  */
    )
{
    NTSTATUS Status = STATUS_SUCCESS;

    DPRINT("UserSystemParametersInfo_StructGet SPI Action 0x%x (uiParam: 0x%x, fWinIni: 0x%x)\n",uiAction,  uiParam, fWinIni);

    _SEH_TRY
    {
        ProbeForRead(pvParam, cbSize, 1);
        /* Copy only first UINT describing structure size*/
        *((PUINT)pBuffer) = *((PUINT)pvParam);
    }
    _SEH_HANDLE
    {
        Status = _SEH_GetExceptionCode();
    }
    _SEH_END;
    if(!NT_SUCCESS(Status))
    {
        SetLastNtError(Status);
        return( FALSE);
    }
    if(*((PUINT)pBuffer) != cbSize)
    {
        SetLastWin32Error(ERROR_INVALID_PARAMETER);
        return( FALSE);
    }
    if(!IntSystemParametersInfo(uiAction, uiParam, pBuffer, fWinIni))
    {
        return( FALSE);
    }
    _SEH_TRY
    {
        ProbeForWrite(pvParam,  cbSize, 1);
        RtlCopyMemory(pvParam,pBuffer,cbSize);
    }
    _SEH_HANDLE
    {
        Status = _SEH_GetExceptionCode();
    }
    _SEH_END;
    if(!NT_SUCCESS(Status))
    {
        SetLastNtError(Status);
        return( FALSE);
    }
    return( TRUE);
}

/*
 * @implemented
 */
BOOL FASTCALL
UserSystemParametersInfo(
   UINT uiAction,
   UINT uiParam,
   PVOID pvParam,
   UINT fWinIni)
{
   NTSTATUS Status = STATUS_SUCCESS;

   /* FIXME: Support Windows Vista SPI Actions */

   switch(uiAction)
   {
#if 1 /* only for 32bit applications */
      case SPI_SETLOWPOWERACTIVE:
      case SPI_SETLOWPOWERTIMEOUT:
      case SPI_SETPOWEROFFACTIVE:
      case SPI_SETPOWEROFFTIMEOUT:
#endif
      case SPI_SETICONS:
      case SPI_SETSCREENSAVETIMEOUT:
      case SPI_SETSCREENSAVEACTIVE:
      case SPI_SETDOUBLECLKWIDTH:
      case SPI_SETDOUBLECLKHEIGHT:
      case SPI_SETDOUBLECLICKTIME:
      case SPI_SETFONTSMOOTHING:
      case SPI_SETMOUSEHOVERTIME:
      case SPI_SETMOUSEHOVERWIDTH:
      case SPI_SETMOUSEHOVERHEIGHT:
      case SPI_SETMOUSETRAILS:
      case SPI_SETSNAPTODEFBUTTON:
      case SPI_SETBEEP:
      case SPI_SETBLOCKSENDINPUTRESETS:
      case SPI_SETKEYBOARDDELAY:
      case SPI_SETKEYBOARDPREF:
      case SPI_SETKEYBOARDSPEED:
      case SPI_SETMOUSEBUTTONSWAP:
      case SPI_SETWHEELSCROLLLINES:
      case SPI_SETMENUSHOWDELAY:
      case SPI_SETMENUDROPALIGNMENT:
      case SPI_SETICONTITLEWRAP:
      case SPI_SETCURSORS:
      case SPI_SETDESKPATTERN:
      case SPI_SETBORDER:
      case SPI_SETDRAGHEIGHT:
      case SPI_SETDRAGWIDTH:
      case SPI_SETSHOWIMEUI:
      case SPI_SETSCREENREADER:
      case SPI_SETSHOWSOUNDS:
                return IntSystemParametersInfo(uiAction, uiParam, NULL, fWinIni);

      /* NOTICE: from the IntSystemParametersInfo implementation it uses pvParam */
      case SPI_SETSCREENSAVERRUNNING:
      case SPI_SETFLATMENU:
      case SPI_SETMOUSESPEED:
      case SPI_SETGRADIENTCAPTIONS:
      case SPI_SETFOCUSBORDERHEIGHT:
      case SPI_SETFOCUSBORDERWIDTH:
      case SPI_SETLANGTOGGLE:
      case SPI_SETMENUFADE:
      case SPI_SETDROPSHADOW:
      case SPI_SETACTIVEWINDOWTRACKING:
      case SPI_SETACTIVEWNDTRKZORDER:
      case SPI_SETACTIVEWNDTRKTIMEOUT:
      case SPI_SETCARETWIDTH:
      case SPI_SETFOREGROUNDFLASHCOUNT:
      case SPI_SETFOREGROUNDLOCKTIMEOUT:
      case SPI_SETFONTSMOOTHINGORIENTATION:
      case SPI_SETFONTSMOOTHINGTYPE:
      case SPI_SETFONTSMOOTHINGCONTRAST:
      case SPI_SETKEYBOARDCUES:
      case SPI_SETCOMBOBOXANIMATION:
      case SPI_SETCURSORSHADOW:
      case SPI_SETHOTTRACKING:
      case SPI_SETLISTBOXSMOOTHSCROLLING:
      case SPI_SETMENUANIMATION:
      case SPI_SETSELECTIONFADE:
      case SPI_SETTOOLTIPANIMATION:
      case SPI_SETTOOLTIPFADE:
      case SPI_SETUIEFFECTS:
      case SPI_SETMOUSECLICKLOCK:
      case SPI_SETMOUSESONAR:
      case SPI_SETMOUSEVANISH:
            return IntSystemParametersInfo(uiAction, 0, pvParam, fWinIni);

      /* Get SPI msg here  */

#if 1 /* only for 32bit applications */
      case SPI_GETLOWPOWERACTIVE:
      case SPI_GETLOWPOWERTIMEOUT:
      case SPI_GETPOWEROFFACTIVE:
      case SPI_GETPOWEROFFTIMEOUT:
#endif

      case SPI_GETSCREENSAVERRUNNING:
      case SPI_GETSCREENSAVETIMEOUT:
      case SPI_GETSCREENSAVEACTIVE:
      case SPI_GETKEYBOARDCUES:
      case SPI_GETFONTSMOOTHING:
      case SPI_GETGRADIENTCAPTIONS:
      case SPI_GETFOCUSBORDERHEIGHT:
      case SPI_GETFOCUSBORDERWIDTH:
      case SPI_GETWHEELSCROLLLINES:
      case SPI_GETWHEELSCROLLCHARS:
      case SPI_GETFLATMENU:
      case SPI_GETMOUSEHOVERHEIGHT:
      case SPI_GETMOUSEHOVERWIDTH:
      case SPI_GETMOUSEHOVERTIME:
      case SPI_GETMOUSESPEED:
      case SPI_GETMOUSETRAILS:
      case SPI_GETSNAPTODEFBUTTON:
      case SPI_GETBEEP:
      case SPI_GETBLOCKSENDINPUTRESETS:
      case SPI_GETKEYBOARDDELAY:
      case SPI_GETKEYBOARDPREF:
      case SPI_GETMENUDROPALIGNMENT:
      case SPI_GETMENUFADE:
      case SPI_GETMENUSHOWDELAY:
      case SPI_GETICONTITLEWRAP:
      case SPI_GETDROPSHADOW:
      case SPI_GETFONTSMOOTHINGCONTRAST:
      case SPI_GETFONTSMOOTHINGORIENTATION:
      case SPI_GETFONTSMOOTHINGTYPE:
      case SPI_GETACTIVEWINDOWTRACKING:
      case SPI_GETACTIVEWNDTRKZORDER:
      case SPI_GETBORDER:
      case SPI_GETCOMBOBOXANIMATION:
      case SPI_GETCURSORSHADOW:
      case SPI_GETHOTTRACKING:
      case SPI_GETLISTBOXSMOOTHSCROLLING:
      case SPI_GETMENUANIMATION:
      case SPI_GETSELECTIONFADE:
      case SPI_GETTOOLTIPANIMATION:
      case SPI_GETTOOLTIPFADE:
      case SPI_GETUIEFFECTS:
      case SPI_GETMOUSECLICKLOCK:
      case SPI_GETMOUSECLICKLOCKTIME:
      case SPI_GETMOUSESONAR:
      case SPI_GETMOUSEVANISH:
      case SPI_GETSCREENREADER:
      case SPI_GETSHOWSOUNDS:
            {
                /* pvParam is PINT,PUINT or PBOOL */
                UINT Ret;
                if(!IntSystemParametersInfo(uiAction, uiParam, &Ret, fWinIni))
                {
                    return( FALSE);
                }
                _SEH_TRY
                {
                    ProbeForWrite(pvParam, sizeof(UINT ), 1);
                    *(PUINT)pvParam = Ret;
                }
                _SEH_HANDLE
                {
                    Status = _SEH_GetExceptionCode();
                }
                _SEH_END;
                if(!NT_SUCCESS(Status))
                {
                    SetLastNtError(Status);
                    return( FALSE);
                }
                return( TRUE);
            }

      case SPI_GETACTIVEWNDTRKTIMEOUT:
      case SPI_GETKEYBOARDSPEED:
      case SPI_GETCARETWIDTH:
      case SPI_GETDRAGFULLWINDOWS:
      case SPI_GETFOREGROUNDFLASHCOUNT:
      case SPI_GETFOREGROUNDLOCKTIMEOUT:
            {
                /* pvParam is PDWORD */
                DWORD Ret;
                if(!IntSystemParametersInfo(uiAction, uiParam, &Ret, fWinIni))
                {
                    return( FALSE);
                }
                _SEH_TRY
                {
                    ProbeForWrite(pvParam, sizeof(DWORD ), 1);
                    *(PDWORD)pvParam = Ret;
                }
                _SEH_HANDLE
                {
                    Status = _SEH_GetExceptionCode();
                }
                _SEH_END;
                if(!NT_SUCCESS(Status))
                {
                    SetLastNtError(Status);
                    return( FALSE);
                }
                return( TRUE);
            }
      case SPI_GETICONMETRICS:
            {
                ICONMETRICSW Buffer;
                return UserSystemParametersInfo_StructGet(uiAction, uiParam, pvParam, fWinIni, &Buffer,sizeof(Buffer));
            }
      case SPI_SETICONMETRICS:
            {
                ICONMETRICSW Buffer;
                return UserSystemParametersInfo_StructSet(uiAction, uiParam, pvParam, fWinIni, &Buffer,sizeof(Buffer));
            }
      case SPI_GETMINIMIZEDMETRICS:
            {
                MINIMIZEDMETRICS Buffer;
                return UserSystemParametersInfo_StructGet(uiAction, uiParam, pvParam, fWinIni,&Buffer,sizeof(Buffer));
            }
            case SPI_SETMINIMIZEDMETRICS:
            {
                MINIMIZEDMETRICS Buffer;
                return UserSystemParametersInfo_StructSet(uiAction, uiParam, pvParam, fWinIni,&Buffer,sizeof(Buffer));
            }
      case SPI_GETNONCLIENTMETRICS:
          {
              NONCLIENTMETRICSW Buffer;
              return UserSystemParametersInfo_StructGet(uiAction, uiParam, pvParam, fWinIni, &Buffer,sizeof(Buffer));
          }
      case SPI_SETNONCLIENTMETRICS:
          {
              NONCLIENTMETRICSW Buffer;
              return UserSystemParametersInfo_StructSet(uiAction, uiParam, pvParam, fWinIni, &Buffer,sizeof(Buffer));
          }
      case SPI_GETANIMATION:
          {
              ANIMATIONINFO Buffer;
              return UserSystemParametersInfo_StructGet(uiAction, uiParam, pvParam, fWinIni, &Buffer,sizeof(Buffer));
          }
      case SPI_SETANIMATION:
          {
              ANIMATIONINFO Buffer;
              return UserSystemParametersInfo_StructSet(uiAction, uiParam, pvParam, fWinIni, &Buffer,sizeof(Buffer));
          }
      case SPI_GETACCESSTIMEOUT:
          {
              ACCESSTIMEOUT Buffer;
              return UserSystemParametersInfo_StructGet(uiAction, uiParam, pvParam, fWinIni, &Buffer,sizeof(Buffer));
          }
      case SPI_SETACCESSTIMEOUT:
          {
              ACCESSTIMEOUT Buffer;
              return UserSystemParametersInfo_StructSet(uiAction, uiParam, pvParam, fWinIni, &Buffer,sizeof(Buffer));
          }
      case SPI_GETFILTERKEYS:
          {
              FILTERKEYS Buffer;
              return UserSystemParametersInfo_StructGet(uiAction, uiParam, pvParam, fWinIni,
                  &Buffer,sizeof(Buffer));
          }
      case SPI_SETFILTERKEYS:
          {
              FILTERKEYS Buffer;
              return UserSystemParametersInfo_StructSet(uiAction, uiParam, pvParam, fWinIni,
                  &Buffer,sizeof(Buffer));
          }
      case SPI_GETSTICKYKEYS:
          {
              STICKYKEYS Buffer;
              return UserSystemParametersInfo_StructGet(uiAction, uiParam, pvParam, fWinIni, &Buffer,sizeof(Buffer));
          }
      case SPI_SETSTICKYKEYS:
          {
              STICKYKEYS Buffer;
              return UserSystemParametersInfo_StructSet(uiAction, uiParam, pvParam, fWinIni, &Buffer,sizeof(Buffer));
          }
      case SPI_GETTOGGLEKEYS:
          {
              TOGGLEKEYS Buffer;
              return UserSystemParametersInfo_StructGet(uiAction, uiParam, pvParam, fWinIni, &Buffer,sizeof(Buffer));
          }
      case SPI_SETTOGGLEKEYS:
          {
              TOGGLEKEYS Buffer;
              return UserSystemParametersInfo_StructSet(uiAction, uiParam, pvParam, fWinIni, &Buffer,sizeof(Buffer));
          }
      case SPI_SETWORKAREA:
          {
              RECT rc;
              _SEH_TRY
              {
                  ProbeForRead(pvParam, sizeof( RECT ), 1);
                  RtlCopyMemory(&rc,pvParam,sizeof(RECT));
              }
              _SEH_HANDLE
              {
                  Status = _SEH_GetExceptionCode();
              }
              _SEH_END;
              if(!NT_SUCCESS(Status))
              {
                  SetLastNtError(Status);
                  return( FALSE);
              }
              return IntSystemParametersInfo(uiAction, uiParam, &rc, fWinIni);
          }
      case SPI_GETWORKAREA:
          {
              RECT rc;
              if(!IntSystemParametersInfo(uiAction, uiParam, &rc, fWinIni))
              {
                  return( FALSE);
              }
              _SEH_TRY
              {
                  ProbeForWrite(pvParam,  sizeof( RECT ), 1);
                  RtlCopyMemory(pvParam,&rc,sizeof(RECT));
              }
              _SEH_HANDLE
              {
                  Status = _SEH_GetExceptionCode();
              }
              _SEH_END;
              if(!NT_SUCCESS(Status))
              {
                  SetLastNtError(Status);
                  return( FALSE);
              }
              return( TRUE);
          }
      case SPI_SETMOUSE:
          {
              CURSORACCELERATION_INFO CursorAccelerationInfo;
              _SEH_TRY
              {
                  ProbeForRead(pvParam, sizeof( CURSORACCELERATION_INFO ), 1);
                  RtlCopyMemory(&CursorAccelerationInfo,pvParam,sizeof(CURSORACCELERATION_INFO));
              }
              _SEH_HANDLE
              {
                  Status = _SEH_GetExceptionCode();
              }
              _SEH_END;
              if(!NT_SUCCESS(Status))
              {
                  SetLastNtError(Status);
                  return( FALSE);
              }
              return IntSystemParametersInfo(uiAction, uiParam, &CursorAccelerationInfo, fWinIni);
          }
      case SPI_GETMOUSE:
          {
              CURSORACCELERATION_INFO CursorAccelerationInfo;
              if(!IntSystemParametersInfo(uiAction, uiParam, &CursorAccelerationInfo, fWinIni))
              {
                  return( FALSE);
              }
              _SEH_TRY
              {
                  ProbeForWrite(pvParam,  sizeof( CURSORACCELERATION_INFO ), 1);
                  RtlCopyMemory(pvParam,&CursorAccelerationInfo,sizeof(CURSORACCELERATION_INFO));
              }
              _SEH_HANDLE
              {
                  Status = _SEH_GetExceptionCode();
              }
              _SEH_END;
              if(!NT_SUCCESS(Status))
              {
                  SetLastNtError(Status);
                  return( FALSE);
              }
              return( TRUE);
          }
      case SPI_SETICONTITLELOGFONT:
          {
              LOGFONTW LogFont;
              _SEH_TRY
              {
                  ProbeForRead(pvParam, sizeof( LOGFONTW ), 1);
                  RtlCopyMemory(&LogFont,pvParam,sizeof(LOGFONTW));
              }
              _SEH_HANDLE
              {
                  Status = _SEH_GetExceptionCode();
              }
              _SEH_END;
              if(!NT_SUCCESS(Status))
              {
                  SetLastNtError(Status);
                  return( FALSE);
              }
              return IntSystemParametersInfo(uiAction, uiParam, &LogFont, fWinIni);
          }
      case SPI_GETICONTITLELOGFONT:
          {
              LOGFONTW LogFont;
              if(!IntSystemParametersInfo(uiAction, uiParam, &LogFont, fWinIni))
              {
                  return( FALSE);
              }
              _SEH_TRY
              {
                  ProbeForWrite(pvParam,  sizeof( LOGFONTW ), 1);
                  RtlCopyMemory(pvParam,&LogFont,sizeof(LOGFONTW));
              }
              _SEH_HANDLE
              {
                  Status = _SEH_GetExceptionCode();
              }
              _SEH_END;
              if(!NT_SUCCESS(Status))
              {
                  SetLastNtError(Status);
                  return( FALSE);
              }
              return( TRUE);
          }
      case SPI_ICONVERTICALSPACING:
      case SPI_ICONHORIZONTALSPACING:
          {
              UINT Ret;
              if(!IntSystemParametersInfo(uiAction, uiParam, &Ret, fWinIni))
              {
                  return( FALSE);
              }
              if(NULL != pvParam)
              {
                  _SEH_TRY
                  {
                      ProbeForWrite(pvParam, sizeof(UINT ), 1);
                      *(PUINT)pvParam = Ret;
                  }
                  _SEH_HANDLE
                  {
                      Status = _SEH_GetExceptionCode();
                  }
                  _SEH_END;
                  if(!NT_SUCCESS(Status))
                  {
                      SetLastNtError(Status);
                      return( FALSE);
                  }
              }
              return( TRUE);
          }
      case SPI_SETDEFAULTINPUTLANG:
      case SPI_SETDESKWALLPAPER:
          /* !!! As opposed to the user mode version this version accepts a handle to the bitmap! */
          {
              HANDLE Handle;
              _SEH_TRY
              {
                  ProbeForRead(pvParam, sizeof( HANDLE ), 1);
                  Handle = *(PHANDLE)pvParam;
              }
              _SEH_HANDLE
              {
                  Status = _SEH_GetExceptionCode();
              }
              _SEH_END;
              if(!NT_SUCCESS(Status))
              {
                  SetLastNtError(Status);
                  return( FALSE);
              }
              return IntSystemParametersInfo(uiAction, uiParam, &Handle, fWinIni);
          }
      case SPI_GETDEFAULTINPUTLANG:
      case SPI_GETDESKWALLPAPER:
          {
              HANDLE Handle;
              if(!IntSystemParametersInfo(uiAction, uiParam, &Handle, fWinIni))
              {
                  return( FALSE);
              }
              _SEH_TRY
              {
                  ProbeForWrite(pvParam,  sizeof( HANDLE ), 1);
                  *(PHANDLE)pvParam = Handle;
                  RtlCopyMemory(pvParam,&Handle,sizeof(HANDLE));
              }
              _SEH_HANDLE
              {
                  Status = _SEH_GetExceptionCode();
              }
              _SEH_END;
              if(!NT_SUCCESS(Status))
              {
                  SetLastNtError(Status);
                  return( FALSE);
              }
              return( TRUE);
          }
      case SPI_GETHIGHCONTRAST:
      case SPI_SETHIGHCONTRAST:
      case SPI_GETSOUNDSENTRY:
      case SPI_SETSOUNDSENTRY:
          {
              /* FIXME: Support this accessibility SPI actions */
              DPRINT1("FIXME: Unsupported SPI Code: %lx \n",uiAction );
              break;
          }
      default :
            {
                SetLastNtError(ERROR_INVALID_PARAMETER);
                DPRINT1("Invalid SPI Code: %lx \n",uiAction );
                break;
            }
   }
   return( FALSE);
}



/*
 * @implemented
 */
BOOL
STDCALL
NtUserSystemParametersInfo(
   UINT uiAction,
   UINT uiParam,
   PVOID pvParam,
   UINT fWinIni)
{
   DECLARE_RETURN(BOOLEAN);

   DPRINT("Enter NtUserSystemParametersInfo\n");
   UserEnterExclusive();

   RETURN( UserSystemParametersInfo(uiAction, uiParam, pvParam, fWinIni));

CLEANUP:
   DPRINT("Leave NtUserSystemParametersInfo, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}



UINT
STDCALL
NtUserGetDoubleClickTime(VOID)
{
   UINT Result;
   NTSTATUS Status;
   PWINSTATION_OBJECT WinStaObject;
   PSYSTEM_CURSORINFO CurInfo;
   DECLARE_RETURN(UINT);

   DPRINT("Enter NtUserGetDoubleClickTime\n");
   UserEnterShared();

   Status = IntValidateWindowStationHandle(PsGetCurrentProcess()->Win32WindowStation,
                                           KernelMode,
                                           0,
                                           &WinStaObject);
   if (!NT_SUCCESS(Status))
      RETURN( (DWORD)FALSE);

   CurInfo = IntGetSysCursorInfo(WinStaObject);
   Result = CurInfo->DblClickSpeed;

   ObDereferenceObject(WinStaObject);
   RETURN( Result);

CLEANUP:
   DPRINT("Leave NtUserGetDoubleClickTime, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}

BOOL
STDCALL
NtUserGetGUIThreadInfo(
   DWORD idThread, /* if NULL use foreground thread */
   LPGUITHREADINFO lpgui)
{
   NTSTATUS Status;
   PTHRDCARETINFO CaretInfo;
   GUITHREADINFO SafeGui;
   PDESKTOP_OBJECT Desktop;
   PUSER_MESSAGE_QUEUE MsgQueue;
   PETHREAD Thread = NULL;
   DECLARE_RETURN(BOOLEAN);

   DPRINT("Enter NtUserGetGUIThreadInfo\n");
   UserEnterShared();

   Status = MmCopyFromCaller(&SafeGui, lpgui, sizeof(DWORD));
   if(!NT_SUCCESS(Status))
   {
      SetLastNtError(Status);
      RETURN( FALSE);
   }

   if(SafeGui.cbSize != sizeof(GUITHREADINFO))
   {
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      RETURN( FALSE);
   }

   if(idThread)
   {
      Status = PsLookupThreadByThreadId((HANDLE)idThread, &Thread);
      if(!NT_SUCCESS(Status))
      {
         SetLastWin32Error(ERROR_ACCESS_DENIED);
         RETURN( FALSE);
      }
      Desktop = ((PW32THREAD)Thread->Tcb.Win32Thread)->Desktop;
   }
   else
   {
      /* get the foreground thread */
      PW32THREAD W32Thread = (PW32THREAD)PsGetCurrentThread()->Tcb.Win32Thread;
      Desktop = W32Thread->Desktop;
      if(Desktop)
      {
         MsgQueue = Desktop->ActiveMessageQueue;
         if(MsgQueue)
         {
            Thread = MsgQueue->Thread;
         }
      }
   }

   if(!Thread || !Desktop)
   {
      if(idThread && Thread)
         ObDereferenceObject(Thread);
      SetLastWin32Error(ERROR_ACCESS_DENIED);
      RETURN( FALSE);
   }

   MsgQueue = (PUSER_MESSAGE_QUEUE)Desktop->ActiveMessageQueue;
   CaretInfo = MsgQueue->CaretInfo;

   SafeGui.flags = (CaretInfo->Visible ? GUI_CARETBLINKING : 0);
   if(MsgQueue->MenuOwner)
      SafeGui.flags |= GUI_INMENUMODE | MsgQueue->MenuState;
   if(MsgQueue->MoveSize)
      SafeGui.flags |= GUI_INMOVESIZE;

   /* FIXME add flag GUI_16BITTASK */

   SafeGui.hwndActive = MsgQueue->ActiveWindow;
   SafeGui.hwndFocus = MsgQueue->FocusWindow;
   SafeGui.hwndCapture = MsgQueue->CaptureWindow;
   SafeGui.hwndMenuOwner = MsgQueue->MenuOwner;
   SafeGui.hwndMoveSize = MsgQueue->MoveSize;
   SafeGui.hwndCaret = CaretInfo->hWnd;

   SafeGui.rcCaret.left = CaretInfo->Pos.x;
   SafeGui.rcCaret.top = CaretInfo->Pos.y;
   SafeGui.rcCaret.right = SafeGui.rcCaret.left + CaretInfo->Size.cx;
   SafeGui.rcCaret.bottom = SafeGui.rcCaret.top + CaretInfo->Size.cy;

   if(idThread)
      ObDereferenceObject(Thread);

   Status = MmCopyToCaller(lpgui, &SafeGui, sizeof(GUITHREADINFO));
   if(!NT_SUCCESS(Status))
   {
      SetLastNtError(Status);
      RETURN( FALSE);
   }

   RETURN( TRUE);

CLEANUP:
   DPRINT("Leave NtUserGetGUIThreadInfo, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}


DWORD
STDCALL
NtUserGetGuiResources(
   HANDLE hProcess,
   DWORD uiFlags)
{
   PEPROCESS Process;
   PW32PROCESS W32Process;
   NTSTATUS Status;
   DWORD Ret = 0;
   DECLARE_RETURN(DWORD);

   DPRINT("Enter NtUserGetGuiResources\n");
   UserEnterShared();

   Status = ObReferenceObjectByHandle(hProcess,
                                      PROCESS_QUERY_INFORMATION,
                                      PsProcessType,
                                      ExGetPreviousMode(),
                                      (PVOID*)&Process,
                                      NULL);

   if(!NT_SUCCESS(Status))
   {
      SetLastNtError(Status);
      RETURN( 0);
   }

   W32Process = (PW32PROCESS)Process->Win32Process;
   if(!W32Process)
   {
      ObDereferenceObject(Process);
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      RETURN( 0);
   }

   switch(uiFlags)
   {
      case GR_GDIOBJECTS:
         {
            Ret = (DWORD)W32Process->GDIObjects;
            break;
         }
      case GR_USEROBJECTS:
         {
            Ret = (DWORD)W32Process->UserObjects;
            break;
         }
      default:
         {
            SetLastWin32Error(ERROR_INVALID_PARAMETER);
            break;
         }
   }

   ObDereferenceObject(Process);

   RETURN( Ret);

CLEANUP:
   DPRINT("Leave NtUserGetGuiResources, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}

NTSTATUS FASTCALL
IntSafeCopyUnicodeString(PUNICODE_STRING Dest,
                         PUNICODE_STRING Source)
{
   NTSTATUS Status;
   PWSTR Src;

   Status = MmCopyFromCaller(Dest, Source, sizeof(UNICODE_STRING));
   if(!NT_SUCCESS(Status))
   {
      return Status;
   }

   if(Dest->Length > 0x4000)
   {
      return STATUS_UNSUCCESSFUL;
   }

   Src = Dest->Buffer;
   Dest->Buffer = NULL;

   if(Dest->Length > 0 && Src)
   {
      Dest->MaximumLength = Dest->Length;
      Dest->Buffer = ExAllocatePoolWithTag(PagedPool, Dest->MaximumLength, TAG_STRING);
      if(!Dest->Buffer)
      {
         return STATUS_NO_MEMORY;
      }

      Status = MmCopyFromCaller(Dest->Buffer, Src, Dest->Length);
      if(!NT_SUCCESS(Status))
      {
         ExFreePool(Dest->Buffer);
         Dest->Buffer = NULL;
         return Status;
      }


      return STATUS_SUCCESS;
   }

   /* string is empty */
   return STATUS_SUCCESS;
}

NTSTATUS FASTCALL
IntSafeCopyUnicodeStringTerminateNULL(PUNICODE_STRING Dest,
                                      PUNICODE_STRING Source)
{
   NTSTATUS Status;
   PWSTR Src;

   Status = MmCopyFromCaller(Dest, Source, sizeof(UNICODE_STRING));
   if(!NT_SUCCESS(Status))
   {
      return Status;
   }

   if(Dest->Length > 0x4000)
   {
      return STATUS_UNSUCCESSFUL;
   }

   Src = Dest->Buffer;
   Dest->Buffer = NULL;

   if(Dest->Length > 0 && Src)
   {
      Dest->MaximumLength = Dest->Length + sizeof(WCHAR);
      Dest->Buffer = ExAllocatePoolWithTag(PagedPool, Dest->MaximumLength, TAG_STRING);
      if(!Dest->Buffer)
      {
         return STATUS_NO_MEMORY;
      }

      Status = MmCopyFromCaller(Dest->Buffer, Src, Dest->Length);
      if(!NT_SUCCESS(Status))
      {
         ExFreePool(Dest->Buffer);
         Dest->Buffer = NULL;
         return Status;
      }

      /* make sure the string is null-terminated */
      Src = (PWSTR)((PBYTE)Dest->Buffer + Dest->Length);
      *Src = L'\0';

      return STATUS_SUCCESS;
   }

   /* string is empty */
   return STATUS_SUCCESS;
}

NTSTATUS FASTCALL
IntUnicodeStringToNULLTerminated(PWSTR *Dest, PUNICODE_STRING Src)
{
   if (Src->Length + sizeof(WCHAR) <= Src->MaximumLength
         && L'\0' == Src->Buffer[Src->Length / sizeof(WCHAR)])
   {
      /* The unicode_string is already nul terminated. Just reuse it. */
      *Dest = Src->Buffer;
      return STATUS_SUCCESS;
   }

   *Dest = ExAllocatePoolWithTag(PagedPool, Src->Length + sizeof(WCHAR), TAG_STRING);
   if (NULL == *Dest)
   {
      return STATUS_NO_MEMORY;
   }
   RtlCopyMemory(*Dest, Src->Buffer, Src->Length);
   (*Dest)[Src->Length / 2] = L'\0';

   return STATUS_SUCCESS;
}

void FASTCALL
IntFreeNULLTerminatedFromUnicodeString(PWSTR NullTerminated, PUNICODE_STRING UnicodeString)
{
   if (NullTerminated != UnicodeString->Buffer)
   {
      ExFreePool(NullTerminated);
   }
}

BOOL STDCALL
NtUserUpdatePerUserSystemParameters(
   DWORD dwReserved,
   BOOL bEnable)
{
   BOOL Result = TRUE;
   DECLARE_RETURN(BOOLEAN);

   DPRINT("Enter NtUserUpdatePerUserSystemParameters\n");
   UserEnterExclusive();

   Result &= IntDesktopUpdatePerUserSettings(bEnable);
   RETURN( Result);

CLEANUP:
   DPRINT("Leave NtUserUpdatePerUserSystemParameters, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}

PW32PROCESSINFO
GetW32ProcessInfo(VOID)
{
    PW32PROCESSINFO pi;
    PW32PROCESS W32Process = PsGetCurrentProcessWin32Process();

    if (W32Process == NULL)
    {
        /* FIXME - temporary hack for system threads... */
        return NULL;
    }

    if (W32Process->ProcessInfo == NULL)
    {
        pi = UserHeapAlloc(sizeof(W32PROCESSINFO));
        if (pi != NULL)
        {
            RtlZeroMemory(pi,
                          sizeof(W32PROCESSINFO));

            /* initialize it */
            pi->UserHandleTable = gHandleTable;
            pi->hUserHeap = W32Process->HeapMappings.KernelMapping;
            pi->UserHeapDelta = (ULONG_PTR)W32Process->HeapMappings.KernelMapping -
                                (ULONG_PTR)W32Process->HeapMappings.UserMapping;

            if (InterlockedCompareExchangePointer(&W32Process->ProcessInfo,
                                                  pi,
                                                  NULL) != NULL)
            {
                UserHeapFree(pi);
            }
        }
        else
        {
            SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
        }
    }

    return W32Process->ProcessInfo;
}

PW32THREADINFO
GetW32ThreadInfo(VOID)
{
    PTEB Teb;
    PW32THREADINFO ti;
    PW32THREAD W32Thread = PsGetCurrentThreadWin32Thread();

    if (W32Thread == NULL)
    {
        /* FIXME - temporary hack for system threads... */
        return NULL;
    }

    /* allocate a W32THREAD structure if neccessary */
    if (W32Thread->ThreadInfo == NULL)
    {
        ti = UserHeapAlloc(sizeof(W32THREADINFO));
        if (ti != NULL)
        {
            RtlZeroMemory(ti,
                          sizeof(W32THREADINFO));

            /* initialize it */
            ti->kpi = GetW32ProcessInfo();
            ti->pi = UserHeapAddressToUser(ti->kpi);
            ti->Hooks = W32Thread->Hooks;
            if (W32Thread->Desktop != NULL)
            {
                ti->Desktop = W32Thread->Desktop->DesktopInfo;
                ti->DesktopHeapBase = W32Thread->Desktop->DesktopInfo->hKernelHeap;
                ti->DesktopHeapLimit = W32Thread->Desktop->DesktopInfo->HeapLimit;
                ti->DesktopHeapDelta = DesktopHeapGetUserDelta();
            }
            else
            {
                ti->Desktop = NULL;
                ti->DesktopHeapBase = NULL;
                ti->DesktopHeapLimit = 0;
                ti->DesktopHeapDelta = 0;
            }

            W32Thread->ThreadInfo = ti;
            /* update the TEB */
            Teb = NtCurrentTeb();
            _SEH_TRY
            {
                ProbeForWrite(Teb,
                              sizeof(TEB),
                              sizeof(ULONG));

                Teb->Win32ThreadInfo = UserHeapAddressToUser(W32Thread->ThreadInfo);
            }
            _SEH_HANDLE
            {
                SetLastNtError(_SEH_GetExceptionCode());
            }
            _SEH_END;
        }
        else
        {
            SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
        }
    }

    return W32Thread->ThreadInfo;
}


/* EOF */
