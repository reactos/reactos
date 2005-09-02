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
 */
/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Windows
 * FILE:             subsys/win32k/ntuser/window.c
 * PROGRAMER:        Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISION HISTORY:
 *       06-06-2001  CSH  Created
 */

/* INCLUDES ******************************************************************/

#include <w32k.h>

#define NDEBUG
#include <debug.h>

static WndProcHandle * gWndProcHandlesArray = 0;
static WORD gWndProcHandlesArraySize = 0;
#define WPH_SIZE 0x40 /* the size to add to the WndProcHandle array each time */

/* dialog resources appear to pass this in 16 bits, handle them properly */
#define CW_USEDEFAULT16 (0x8000)

#define POINT_IN_RECT(p, r) (((r.bottom >= p.y) && (r.top <= p.y))&&((r.left <= p.x )&&( r.right >= p.x )))

/* PRIVATE FUNCTIONS **********************************************************/

/*
 * InitWindowImpl
 *
 * Initialize windowing implementation.
 */

NTSTATUS FASTCALL
InitWindowImpl(VOID)
{
   gWndProcHandlesArray = ExAllocatePoolWithTag(PagedPool,WPH_SIZE * sizeof(WndProcHandle), TAG_WINPROCLST);
   gWndProcHandlesArraySize = WPH_SIZE;
   return STATUS_SUCCESS;
}

/*
 * CleanupWindowImpl
 *
 * Cleanup windowing implementation.
 */

NTSTATUS FASTCALL
CleanupWindowImpl(VOID)
{
   ExFreePool(gWndProcHandlesArray);
   gWndProcHandlesArray = 0;
   gWndProcHandlesArraySize = 0;
   return STATUS_SUCCESS;
}

/* HELPER FUNCTIONS ***********************************************************/

/*
 * IntIsWindow
 *
 * The function determines whether the specified window handle identifies
 * an existing window.
 *
 * Parameters
 *    hWnd
 *       Handle to the window to test.
 *
 * Return Value
 *    If the window handle identifies an existing window, the return value
 *    is TRUE. If the window handle does not identify an existing window,
 *    the return value is FALSE.
 */

BOOL FASTCALL
IntIsWindow(HWND hWnd)
{
   PWINDOW_OBJECT Window;

   if (!(Window = UserGetWindowObject(hWnd)))
      return FALSE;

   return TRUE;
}

inline PWINDOW_OBJECT FASTCALL UserGetWindowObject(HWND hWnd)
{
   if (hWnd == NULL)
      return NULL;
   return (PWINDOW_OBJECT)UserGetObject(&gHandleTable, hWnd, otWindow );
}

inline VOID FASTCALL UserFreeWindowObject(PWINDOW_OBJECT Wnd)
{
   UserFreeHandle(&gHandleTable, Wnd->hSelf);
   RtlZeroMemory(Wnd, sizeof(WINDOW_OBJECT) + Wnd->ExtraDataSize);
   ExFreePool(Wnd);
}

PWINDOW_OBJECT FASTCALL UserCreateWindowObject(ULONG bytes)
{
   PWINDOW_OBJECT Wnd;
   HWND hWnd;

   Wnd = (PWINDOW_OBJECT)UserAllocZero(bytes);
   if (!Wnd)
      return NULL;

   hWnd = UserAllocHandle(&gHandleTable, Wnd, otWindow);
   if (!hWnd)
   {
      UserFree(Wnd);
      return NULL;
   }
   Wnd->hSelf = hWnd;
   return Wnd;
}

PWINDOW_OBJECT FASTCALL
UserGetParent(PWINDOW_OBJECT Wnd)
{
   if (Wnd->Style & WS_POPUP)
   {
      return UserGetWindowObject(Wnd->hOwnerWnd);
   }
   else if (Wnd->Style & WS_CHILD)
   {
      return Wnd->ParentWnd;
   }

   return NULL;
}


inline PWINDOW_OBJECT FASTCALL
UserGetOwner(PWINDOW_OBJECT Wnd)
{
   return UserGetWindowObject(Wnd->hOwnerWnd);
}


/*
 * IntWinListChildren
 *
 * Compile a list of all child window handles from given window.
 *
 * Remarks
 *    This function is similar to Wine WIN_ListChildren. The caller
 *    must free the returned list with ExFreePool.
 */

HWND* FASTCALL
IntWinListChildren(PWINDOW_OBJECT Window)
{
   PWINDOW_OBJECT Child;
   HWND *List;
   UINT Index, NumChildren = 0;


   for (Child = Window->FirstChild; Child; Child = Child->NextSibling)
      ++NumChildren;

   List = ExAllocatePoolWithTag(PagedPool, (NumChildren + 1) * sizeof(HWND), TAG_WINLIST);
   if(!List)
   {
      DPRINT1("Failed to allocate memory for children array\n");
      SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
      return NULL;
   }
   for (Child = Window->FirstChild, Index = 0;
         Child != NULL;
         Child = Child->NextSibling, ++Index)
      List[Index] = Child->hSelf;
   List[Index] = NULL;


   return List;
}




/*
 * IntWinListChildren
 *
 * Compile a list of all child window handles from given window.
 *
 * Remarks
 *    This function is similar to Wine WIN_ListChildren. The caller
 *    must free the returned list with ExFreePool.
 */

PWINDOW_OBJECT* FASTCALL
UserListChildWnd(PWINDOW_OBJECT Window)
{
   PWINDOW_OBJECT Child;
   PWINDOW_OBJECT *List;
   UINT Index, NumChildren = 0;


   for (Child = Window->FirstChild; Child; Child = Child->NextSibling)
      ++NumChildren;

   List = ExAllocatePoolWithTag(PagedPool, (NumChildren + 1) * sizeof(PWINDOW_OBJECT), TAG_WINLIST);
   if(!List)
   {
      DPRINT1("Failed to allocate memory for children array\n");
      SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
      return NULL;
   }
   for (Child = Window->FirstChild, Index = 0;
         Child != NULL;
         Child = Child->NextSibling, ++Index)
      List[Index] = Child;
   List[Index] = NULL;

   return List;
}


/***********************************************************************
 *           coUserSendDestroyMsg
 */
static void co_UserSendDestroyMsg(HWND Wnd)
{

   PWINDOW_OBJECT Window, Owner, Parent;
#if 0 /* FIXME */

   GUITHREADINFO info;

   if (GetGUIThreadInfo(GetCurrentThreadId(), &info))
   {
      if (Wnd == info.hwndCaret)
      {
         DestroyCaret();
      }
   }
#endif

   Window = UserGetWindowObject(Wnd);
   if (Window)
   {
      Owner = UserGetOwner(Window);
      if (!Owner)
      {
         Parent = UserGetParent(Window);
         if (!Parent)
            co_UserShellHookNotify(HSHELL_WINDOWDESTROYED, (LPARAM) Wnd);
      }

   }

   /* The window could already be destroyed here */

   /*
    * Send the WM_DESTROY to the window.
    */

   co_UserSendMessage(Wnd, WM_DESTROY, 0, 0);

   /*
    * This WM_DESTROY message can trigger re-entrant calls to DestroyWindow
    * make sure that the window still exists when we come back.
    */
#if 0 /* FIXME */

   if (IsWindow(Wnd))
   {
      HWND* pWndArray;
      int i;

      if (!(pWndArray = WIN_ListChildren( hwnd )))
         return;

      /* start from the end (FIXME: is this needed?) */
      for (i = 0; pWndArray[i]; i++)
         ;

      while (--i >= 0)
      {
         if (IsWindow( pWndArray[i] ))
            WIN_SendDestroyMsg( pWndArray[i] );
      }
      HeapFree(GetProcessHeap(), 0, pWndArray);
   }
   else
   {
      DPRINT("destroyed itself while in WM_DESTROY!\n");
   }
#endif
}

/***********************************************************************
 *           IntDestroyWindow2
 *
 * Destroy storage associated to a window. "Internals" p.358
 */
static LRESULT co_IntDestroyWindowStorage(PWINDOW_OBJECT Window,
                                    PW32PROCESS ProcessData,
                                    PW32THREAD ThreadData)
{
   //  PWINDOW_OBJECT *Children;
   //  PWINDOW_OBJECT *Child;
   HWND* List;
   PWINDOW_OBJECT Child;
   PMENU_OBJECT Menu;
   BOOLEAN BelongsToThreadData;

   ASSERT(Window);

   if(Window->hdr.flags & USER_OBJ_DESTROYING)
   {
      DPRINT("Tried to call coIntDestroyWindow2() twice\n");
      return 0;
   }

   Window->hdr.flags |= USER_OBJ_DESTROYING;
   Window->Flags &= ~WS_VISIBLE;

   /* remove the window already at this point from the thread window list so we
      don't get into trouble when destroying the thread windows while we're still
      in coIntDestroyWindow2() */
   RemoveEntryList(&Window->ThreadListEntry);

   BelongsToThreadData = IntWndBelongsToThread(Window, ThreadData);

   IntDeRegisterShellHookWindow(Window->hSelf);

   /* Send destroy messages */
   co_UserSendDestroyMsg(Window->hSelf);

   /* free child windows */
   List = IntWinListChildren(Window);
   if (List)
   {
      int i;
      for (i=0; List[i]; i++)
      {
         if ((Child = UserGetWindowObject(List[i])))
         {
            if(!IntWndBelongsToThread(Child, ThreadData))
            {
               /* send WM_DESTROY messages to windows not belonging to the same thread */
               //FIXME: co_IntDestroyWindowStorage is never called for this window!!
               co_UserSendDestroyMsg(Child->hSelf);
            }
            else
               co_IntDestroyWindowStorage(Child, ProcessData, ThreadData);

         }
      }
      ExFreePool(List);
   }

   /*
   * Clear the update region to make sure no WM_PAINT messages will be
   * generated for this window while processing the WM_NCDESTROY.
   */
   co_UserRedrawWindow(Window, NULL, 0,
                       RDW_VALIDATE | RDW_NOFRAME | RDW_NOERASE |
                       RDW_NOINTERNALPAINT | RDW_NOCHILDREN);
   if(BelongsToThreadData)
      co_UserSendMessage(Window->hSelf, WM_NCDESTROY, 0, 0);



   /* flush the message queue */
   /* BUGBUG: this derefs the queue and possibly frees it right under us! */
   //  MsqRemoveWindowMessagesFromQueue(Window);

   /* from now on no messages can be sent to this window anymore */
   //FIXME: this flas isnt used anywhere!
   Window->hdr.flags |= USER_OBJ_DESTROYED;
   /* don't remove the WINDOWSTATUS_DESTROYING bit */

   /* reset shell window handles */
   if(ThreadData->Desktop)
   {
      if (Window->hSelf == ThreadData->Desktop->WinSta->ShellWindow)
         ThreadData->Desktop->WinSta->ShellWindow = NULL;

      if (Window->hSelf == ThreadData->Desktop->WinSta->ShellListView)
         ThreadData->Desktop->WinSta->ShellListView = NULL;
   }

   /* Unregister hot keys */
   UnregisterWindowHotKeys (Window);

   /* FIXME: do we need to fake QS_MOUSEMOVE wakebit? */

#if 0 /* FIXME */

   WinPosCheckInternalPos(Window->hSelf);
   if (Window->hSelf == GetCapture())
   {
      ReleaseCapture();
   }

   /* free resources associated with the window */
   TIMER_RemoveWindowTimers(Window->hSelf);
#endif

   if (!(Window->Style & WS_CHILD) && Window->IDMenu
         && (Menu = UserGetMenuObject((HMENU)Window->IDMenu)))
   {
      UserDestroyMenu(Menu, TRUE);
      Window->IDMenu = 0;
   }

   if(Window->SystemMenu
         && (Menu = UserGetMenuObject(Window->SystemMenu)))
   {
      UserDestroyMenu(Menu, TRUE);
      Window->SystemMenu = (HMENU)0;
   }

   DceFreeWindowDCE(Window);    /* Always do this to catch orphaned DCs */
#if 0 /* FIXME */

   WINPROC_FreeProc(Window->winproc, WIN_PROC_WINDOW);
   CLASS_RemoveWindow(Window->Class);
#endif

   IntUnlinkWindow(Window);

   IntDestroyScrollBars(Window);

   /* remove the window from the class object */
   RemoveEntryList(&Window->ClassListEntry);

   /* dereference the class */
   UserDereferenceClass(Window->Class);
   Window->Class = NULL;

   if(Window->WindowRegion)
   {
      NtGdiDeleteObject(Window->WindowRegion);
   }

   RtlFreeUnicodeString(&Window->WindowName);

   /* cleanup timers and queue */
   MsqCleanupWindow(Window);

   UserFreeWindowObject(Window);

   return 0;
}



VOID FASTCALL
IntGetWindowBorderMeasures(PWINDOW_OBJECT WindowObject, UINT *cx, UINT *cy)
{
   DPRINT1("IntGetWindowBorderMeasures, wnd=0x%x, cx=0x%x, cy=0x%x\n", WindowObject,cx,cy);

   if(HAS_DLGFRAME(WindowObject->Style, WindowObject->ExStyle) && !(WindowObject->Style & WS_MINIMIZE))
   {
      *cx = UserGetSystemMetrics(SM_CXDLGFRAME);
      *cy = UserGetSystemMetrics(SM_CYDLGFRAME);
   }
   else
   {
      if(HAS_THICKFRAME(WindowObject->Style, WindowObject->ExStyle)&& !(WindowObject->Style & WS_MINIMIZE))
      {
         *cx = UserGetSystemMetrics(SM_CXFRAME);
         *cy = UserGetSystemMetrics(SM_CYFRAME);
      }
      else if(HAS_THINFRAME(WindowObject->Style, WindowObject->ExStyle))
      {
         *cx = UserGetSystemMetrics(SM_CXBORDER);
         *cy = UserGetSystemMetrics(SM_CYBORDER);
      }
      else
      {
         *cx = *cy = 0;
      }
   }
}

BOOL FASTCALL
IntGetWindowInfo(PWINDOW_OBJECT WindowObject, PWINDOWINFO pwi)
{
   pwi->cbSize = sizeof(WINDOWINFO);
   pwi->rcWindow = WindowObject->WindowRect;
   pwi->rcClient = WindowObject->ClientRect;
   pwi->dwStyle = WindowObject->Style;
   pwi->dwExStyle = WindowObject->ExStyle;
   pwi->dwWindowStatus = (UserGetForegroundWindow() == WindowObject); /* WS_ACTIVECAPTION */
   IntGetWindowBorderMeasures(WindowObject, &pwi->cxWindowBorders, &pwi->cyWindowBorders);
   pwi->atomWindowType = (WindowObject->Class ? WindowObject->Class->Atom : 0);
   pwi->wCreatorVersion = 0x400; /* FIXME - return a real version number */
   return TRUE;
}



/* INTERNAL ******************************************************************/


VOID FASTCALL
DestroyThreadWindows(PW32THREAD WThread)
{
   PLIST_ENTRY Current;
   PWINDOW_OBJECT Wnd;

   while (!IsListEmpty(&WThread->WindowListHead))
   {
      Current = WThread->WindowListHead.Flink;
      Wnd = CONTAINING_RECORD(Current, WINDOW_OBJECT, ThreadListEntry);

      DPRINT1("while destroy wnds, wnd=0x%x, queue=0x%x, wthread=0x%x\n",Wnd,Wnd->Queue,WThread);

      /* window removes itself from the list */
      ASSERT(co_UserDestroyWindow(Wnd));
   }
}


/*!
 * Internal function.
 * Returns client window rectangle relative to the upper-left corner of client area.
 *
 * \note Does not check the validity of the parameters
*/
VOID FASTCALL
IntGetClientRect(PWINDOW_OBJECT WindowObject, PRECT Rect)
{
   ASSERT( WindowObject );
   ASSERT( Rect );

   Rect->left = Rect->top = 0;
   Rect->right = WindowObject->ClientRect.right - WindowObject->ClientRect.left;
   Rect->bottom = WindowObject->ClientRect.bottom - WindowObject->ClientRect.top;
}


#if 0
HWND FASTCALL
IntGetFocusWindow(VOID)
{
   PUSER_MESSAGE_QUEUE Queue;
   PDESKTOP_OBJECT pdo = UserGetActiveDesktop();

   if( !pdo )
      return NULL;

   Queue = pdo->ActiveMessageQueue;

   if (Queue == NULL)
      return(NULL);
   else
      return(Queue->FocusWindow);
}
#endif



//FIXME: Duplicate impl. in umode??
BOOL FASTCALL
UserIsChildWindow(PWINDOW_OBJECT Parent, PWINDOW_OBJECT Child)
{
   PWINDOW_OBJECT Window;//, Old;

   //FIXME: HACK!
   if (!Parent || !Child)
      return FALSE;

   ASSERT(Parent);
   ASSERT(Child);

   Window = Child->ParentWnd;
   while (Window)
   {
      if (Window == Parent)
      {
         return(TRUE);
      }
      //FIXME: wine does not check for this. why should we??
      if(!(Window->Style & WS_CHILD))
      {
         break;
      }
      Window = Window->ParentWnd;
   }

   return(FALSE);
}

/* only use this if Wnd can be NULL!!! If you are sure Wnd is valid, use Wnd->hSelf!!! */
HWND FASTCALL
GetHwndSafe(PWINDOW_OBJECT Wnd)
{
   return Wnd ? Wnd->hSelf : 0;
}

BOOL FASTCALL
UserIsWindowVisible(PWINDOW_OBJECT Wnd)
{
   ASSERT(Wnd);

   while(Wnd)
   {
      if(!(Wnd->Style & WS_CHILD))
      {
         break;
      }
      if(!(Wnd->Style & WS_VISIBLE))
      {
         return FALSE;
      }
      Wnd = Wnd->ParentWnd;
   }

   if(Wnd)
   {
      if(Wnd->Style & WS_VISIBLE)
      {
         return TRUE;
      }
   }

   return FALSE;
}


/* link the window into siblings and parent. children are kept in place. */
VOID FASTCALL
IntLinkWindow(
   PWINDOW_OBJECT Wnd,
   PWINDOW_OBJECT WndParent,
   PWINDOW_OBJECT WndPrevSibling /* set to NULL if top sibling */
)
{
   PWINDOW_OBJECT Parent;

   DPRINT1("Set 0x%x's parent to 0x%x\n",Wnd, WndParent);
   
   Wnd->ParentWnd = WndParent;
   
   if ((Wnd->PrevSibling = WndPrevSibling))
   {
      /* link after WndPrevSibling */
      if ((Wnd->NextSibling = WndPrevSibling->NextSibling))
         Wnd->NextSibling->PrevSibling = Wnd;
      //    else if ((Parent = IntGetWindowObject(Wnd->Parent)))
      else if ((Parent = Wnd->ParentWnd))
      {
         if(Parent->LastChild == WndPrevSibling)
            Parent->LastChild = Wnd;
      }
      Wnd->PrevSibling->NextSibling = Wnd;
   }
   else
   {
      /* link at top */
      Parent = Wnd->ParentWnd;
      if ((Wnd->NextSibling = WndParent->FirstChild))
         Wnd->NextSibling->PrevSibling = Wnd;
      else if (Parent)
      {
         Parent->LastChild = Wnd;
         Parent->FirstChild = Wnd;
         return;
      }
      if(Parent)
      {
         Parent->FirstChild = Wnd;
      }
   }
}


PWINDOW_OBJECT FASTCALL
UserSetOwner(PWINDOW_OBJECT Wnd, PWINDOW_OBJECT NewOwnerWnd OPTIONAL)
{
   PWINDOW_OBJECT PrevOwnerWnd;

   ASSERT(Wnd);

   PrevOwnerWnd = UserGetWindowObject(Wnd->hOwnerWnd);
   
   Wnd->hOwnerWnd = GetHwndSafe(NewOwnerWnd);

   return PrevOwnerWnd;
}


PWINDOW_OBJECT FASTCALL
co_UserSetParent(
   PWINDOW_OBJECT Wnd,
   PWINDOW_OBJECT WndNewParent OPTIONAL
)
{
   PWINDOW_OBJECT WndOldParent, Sibling, InsertAfter, Desktop;
   //HWND hWnd;//, hWndNewParent;
   BOOL WasVisible;
   BOOL MenuChanged;

   ASSERT(Wnd);

   Desktop = UserGetDesktopWindow();
   if (Wnd == Desktop)
   {
      SetLastWin32Error(ERROR_ACCESS_DENIED);
      return(NULL);
   }

   if (!WndNewParent)
   {
      WndNewParent = Desktop;
   }

   //   hWnd = Wnd->Self;
   //   hWndNewParent = WndNewParent->Self;

   /*
    * Windows hides the window first, then shows it again
    * including the WM_SHOWWINDOW messages and all
    */
   WasVisible = co_WinPosShowWindow(Wnd, SW_HIDE);

   /* Validate that window and parent still exist */
   //   if (!IntIsWindow(hWnd) || !IntIsWindow(hWndNewParent))
   //      return NULL;

   /* Window must belong to current process */
   if (QUEUE_2_WTHREAD(Wnd->Queue)->WProcess != PsGetWin32Process())
   {
      return NULL;
   }


   //FIXME: figure out ref counting and parent relationship etc.
   WndOldParent = Wnd->ParentWnd;

   if (WndNewParent != WndOldParent)
   {
      IntUnlinkWindow(Wnd);
      InsertAfter = NULL;
      if (0 == (Wnd->ExStyle & WS_EX_TOPMOST))
      {
         /* Not a TOPMOST window, put after TOPMOSTs of new parent */
         Sibling = WndNewParent->FirstChild;
         while (NULL != Sibling && 0 != (Sibling->ExStyle & WS_EX_TOPMOST))
         {
            InsertAfter = Sibling;
            Sibling = Sibling->NextSibling;
         }
      }
      if (NULL == InsertAfter)
      {
         IntLinkWindow(Wnd, WndNewParent, InsertAfter /*prev sibling*/);
      }
      else
      {
         IntLinkWindow(Wnd, WndNewParent, InsertAfter /*prev sibling*/);
      }

      if (WndNewParent != UserGetDesktopWindow()) /* a child window */
      {
         if (!(Wnd->Style & WS_CHILD))
         {
            //if ( Wnd->Menu ) DestroyMenu ( Wnd->menu );
            UserSetMenu(Wnd, NULL, &MenuChanged);
         }
      }
   }

   /*
    * SetParent additionally needs to make hwnd the top window
    * in the z-order and send the expected WM_WINDOWPOSCHANGING and
    * WM_WINDOWPOSCHANGED notification messages.
    */
   co_WinPosSetWindowPos(Wnd->hSelf, (0 == (Wnd->ExStyle & WS_EX_TOPMOST) ? HWND_TOP : HWND_TOPMOST),
                         0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE
                         | (WasVisible ? SWP_SHOWWINDOW : 0));


   /*
    * FIXME: a WM_MOVE is also generated (in the DefWindowProc handler
    * for WM_WINDOWPOSCHANGED) in Windows, should probably remove SWP_NOMOVE
    */

   /*
    * Validate that the old parent still exist, since it migth have been
    * destroyed during the last callbacks to user-mode
    */
   //   if(WndOldParent)
   //   {
   //     if(!IntIsWindow(WndOldParent->Self))
   //     {
   //       return NULL;
   //     }

   /* don't dereference the window object here, it must be done by the caller
      of IntSetParent() */
   //     return WndOldParent;
   //   }
   //   return NULL;
#if 0
   /* if parent belongs to a different thread, attach the two threads */
   if (/*parent->thread &&*/ WndNewParent->WThread != Wnd->WThread)
   {
      attach_thread_input( Wnd->WThread, WndNewParent->WThread );
   }
#endif
   return WndOldParent;
}

BOOL FASTCALL
UserSetSystemMenu(PWINDOW_OBJECT Window, PMENU_OBJECT Menu)
{
   PMENU_OBJECT OldMenu;
   
   if(Window->SystemMenu)
   {
      OldMenu = UserGetMenuObject(Window->SystemMenu);
      if(OldMenu)
      {
         OldMenu->MenuInfo.Flags &= ~ MF_SYSMENU;
      }
   }

   if(Menu)
   {
      /* FIXME check window style, propably return FALSE ? */
      Window->SystemMenu = Menu->MenuInfo.Self;
      Menu->MenuInfo.Flags |= MF_SYSMENU;
   }
   else
      Window->SystemMenu = (HMENU)0;

   return TRUE;
}


/* unlink the window from siblings and parent. children are kept in place. */
VOID FASTCALL
IntUnlinkWindow(PWINDOW_OBJECT Wnd)
{
   PWINDOW_OBJECT WndParent;

   WndParent = Wnd->ParentWnd;

   if (Wnd->NextSibling)
      Wnd->NextSibling->PrevSibling = Wnd->PrevSibling;
   else if (WndParent && WndParent->LastChild == Wnd)
      WndParent->LastChild = Wnd->PrevSibling;

   if (Wnd->PrevSibling)
      Wnd->PrevSibling->NextSibling = Wnd->NextSibling;
   else if (WndParent && WndParent->FirstChild == Wnd)
      WndParent->FirstChild = Wnd->NextSibling;

   DPRINT1("Set 0x%x's parent to 0x%x\n",Wnd, WndParent);
   Wnd->PrevSibling = Wnd->NextSibling = Wnd->ParentWnd = NULL;
}

BOOL FASTCALL
IntAnyPopup(VOID)
{
   PWINDOW_OBJECT Window, Child;

   if(!(Window = UserGetDesktopWindow()))
   {
      SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
      return FALSE;
   }

   for(Child = Window->FirstChild; Child; Child = Child->NextSibling)
   {
      if(UserGetWindowObject(Child->hOwnerWnd) && Child->Style & WS_VISIBLE)
      {
         /*
          * The desktop has a popup window if one of them has
          * an owner window and is visible
          */
         return TRUE;
      }
   }

   return FALSE;
}

BOOL FASTCALL
IntIsWindowInDestroy(PWINDOW_OBJECT Window)
{
   return (Window->hdr.flags & USER_OBJ_DESTROYING);
}

/* FUNCTIONS *****************************************************************/

/*
 * @unimplemented
 */
DWORD STDCALL
NtUserAlterWindowStyle(DWORD Unknown0,
                       DWORD Unknown1,
                       DWORD Unknown2)
{
   UNIMPLEMENTED

   return(0);
}


/*
 * As best as I can figure, this function is used by EnumWindows,
 * EnumChildWindows, EnumDesktopWindows, & EnumThreadWindows.
 *
 * It's supposed to build a list of HWNDs to return to the caller.
 * We can figure out what kind of list by what parameters are
 * passed to us.
 */
/*
 * @implemented
 */
ULONG
STDCALL
NtUserBuildHwndList(
   HDESK hDesktop,
   HWND hwndParent,
   BOOLEAN bChildren,
   ULONG dwThreadId,
   ULONG lParam,
   HWND* pWnd,
   ULONG nBufSize)
{
   NTSTATUS Status;
   ULONG dwCount = 0;
   DECLARE_RETURN(ULONG);
   /* FIXME handle bChildren */

   DPRINT("Enter NtUserBuildHwndList\n");
   UserEnterExclusive();

   if(hwndParent)
   {
      PWINDOW_OBJECT Window, Child;
      if(!(Window = UserGetWindowObject(hwndParent)))
      {
         SetLastWin32Error(ERROR_INVALID_HANDLE);
         RETURN(0);
      }

      for(Child = Window->FirstChild; Child != NULL; Child = Child->NextSibling)
      {
         if(dwCount++ < nBufSize && pWnd)
         {
            Status = MmCopyToCaller(pWnd++, &Child->hSelf, sizeof(HWND));
            if(!NT_SUCCESS(Status))
            {
               SetLastNtError(Status);
               break;
            }
         }
      }

   }
   else if(dwThreadId)
   {
      PETHREAD Thread;
      PW32THREAD WThread;
      PLIST_ENTRY Current;
      PWINDOW_OBJECT Window;

      Status = PsLookupThreadByThreadId((HANDLE)dwThreadId, &Thread);
      if(!NT_SUCCESS(Status))
      {
         SetLastWin32Error(ERROR_INVALID_PARAMETER);
         RETURN(0);
      }
      if(!(WThread = Thread->Tcb.Win32Thread))
      {
         ObDereferenceObject(Thread);
         DPRINT("Thread is not a GUI Thread!\n");
         SetLastWin32Error(ERROR_INVALID_PARAMETER);
         RETURN(0);
      }

      Current = WThread->WindowListHead.Flink;
      while(Current != &(WThread->WindowListHead))
      {
         Window = CONTAINING_RECORD(Current, WINDOW_OBJECT, ThreadListEntry);
         ASSERT(Window);

         if(dwCount < nBufSize && pWnd)
         {
            Status = MmCopyToCaller(pWnd++, &Window->hSelf, sizeof(HWND));
            if(!NT_SUCCESS(Status))
            {
               SetLastNtError(Status);
               break;
            }
         }
         dwCount++;
         Current = Current->Flink;
      }

      ObDereferenceObject(Thread);
   }
   else
   {
      PDESKTOP_OBJECT Desktop;
      PWINDOW_OBJECT Window, Child;

      if(hDesktop == NULL && !(Desktop = UserGetActiveDesktop()))
      {
         SetLastWin32Error(ERROR_INVALID_HANDLE);
         RETURN(0);
      }

      if(hDesktop)
      {
         Status = IntValidateDesktopHandle(hDesktop,
                                           UserMode,
                                           0,
                                           &Desktop);
         if(!NT_SUCCESS(Status))
         {
            SetLastWin32Error(ERROR_INVALID_HANDLE);
            RETURN(0);
         }
      }
      if(!(Window = UserGetWindowObject(Desktop->DesktopWindow)))
      {
         if(hDesktop)
            ObDereferenceObject(Desktop);
         SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
         RETURN(0);
      }

      for(Child = Window->FirstChild; Child != NULL; Child = Child->NextSibling)
      {
         if(dwCount++ < nBufSize && pWnd)
         {
            Status = MmCopyToCaller(pWnd++, &Child->hSelf, sizeof(HWND));
            if(!NT_SUCCESS(Status))
            {
               SetLastNtError(Status);
               break;
            }
         }
      }

      if(hDesktop)
         ObDereferenceObject(Desktop);
   }

   RETURN(dwCount);

CLEANUP:
   DPRINT("Leave NtUserBuildHwndList, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}


/*
 * @implemented
 */
HWND STDCALL
NtUserChildWindowFromPointEx(HWND hwndParent,
                             LONG x,
                             LONG y,
                             UINT uiFlags)
{
   PWINDOW_OBJECT Parent;
   POINTL Pt;
   HWND Ret;
   HWND *List;
   DECLARE_RETURN(HWND);

   DPRINT("Enter NtUserChildWindowFromPointEx\n");
   UserEnterExclusive();

   if(!(Parent = UserGetWindowObject(hwndParent)))
   {
      SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
      RETURN(NULL);
   }

   Pt.x = x;
   Pt.y = y;

   if(Parent != UserGetDesktopWindow())
   {
      Pt.x += Parent->ClientRect.left;
      Pt.y += Parent->ClientRect.top;
   }

   if(!IntPtInWindow(Parent, Pt.x, Pt.y))
   {
      RETURN(NULL);
   }

   Ret = Parent->hSelf;
   if((List = IntWinListChildren(Parent)))
   {
      int i;
      for(i=0; List[i]; i++)
      {
         PWINDOW_OBJECT Child;
         if((Child = UserGetWindowObject(List[i])))
         {
            if(!(Child->Style & WS_VISIBLE) && (uiFlags & CWP_SKIPINVISIBLE))
            {
               continue;
            }
            if((Child->Style & WS_DISABLED) && (uiFlags & CWP_SKIPDISABLED))
            {
               continue;
            }
            if((Child->ExStyle & WS_EX_TRANSPARENT) && (uiFlags & CWP_SKIPTRANSPARENT))
            {
               continue;
            }
            if(IntPtInWindow(Child, Pt.x, Pt.y))
            {
               Ret = Child->hSelf;
               break;
            }
         }
      }
      ExFreePool(List);
   }

   RETURN(Ret);

CLEANUP:
   DPRINT("Leave NtUserChildWindowFromPointEx, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}


/*
 * calculates the default position of a window
 */
BOOL FASTCALL
IntCalcDefPosSize(PWINDOW_OBJECT Parent, PWINDOW_OBJECT WindowObject, RECT *rc, BOOL IncPos)
{
   SIZE Sz;
   POINT Pos = {0, 0};

   if(Parent != NULL)
   {
      IntGdiIntersectRect(rc, rc, &Parent->ClientRect);

      if(IncPos)
      {
         Pos.x = Parent->TiledCounter * (UserGetSystemMetrics(SM_CXSIZE) + UserGetSystemMetrics(SM_CXFRAME));
         Pos.y = Parent->TiledCounter * (UserGetSystemMetrics(SM_CYSIZE) + UserGetSystemMetrics(SM_CYFRAME));
         if(Pos.x > ((rc->right - rc->left) / 4) ||
               Pos.y > ((rc->bottom - rc->top) / 4))
         {
            /* reset counter and position */
            Pos.x = 0;
            Pos.y = 0;
            Parent->TiledCounter = 0;
         }
         Parent->TiledCounter++;
      }
      Pos.x += rc->left;
      Pos.y += rc->top;
   }
   else
   {
      Pos.x = rc->left;
      Pos.y = rc->top;
   }

   Sz.cx = EngMulDiv(rc->right - rc->left, 3, 4);
   Sz.cy = EngMulDiv(rc->bottom - rc->top, 3, 4);

   rc->left = Pos.x;
   rc->top = Pos.y;
   rc->right = rc->left + Sz.cx;
   rc->bottom = rc->top + Sz.cy;
   return TRUE;
}






/*
 * @implemented
 */
HWND STDCALL
co_UserCreateWindowEx(DWORD dwExStyle,
                      PUNICODE_STRING ClassName,
                      PUNICODE_STRING WindowName,
                      DWORD dwStyle,
                      LONG x,
                      LONG y,
                      LONG nWidth,
                      LONG nHeight,
                      HWND hWndParent,
                      HMENU hMenu,
                      HINSTANCE hInstance,
                      LPVOID lpParam,
                      DWORD dwShowMode,
                      BOOL bUnicodeWindow)
{
   PWNDCLASS_OBJECT ClassObject;
   PWINDOW_OBJECT Wnd;
   PWINDOW_OBJECT ParentWindow, OwnerWindow = NULL;
   HWND ParentWindowHandle;
   PMENU_OBJECT SystemMenu;
   POINT Pos;
   SIZE Size;
#if 0

   POINT MaxSize, MaxPos, MinTrack, MaxTrack;
#else

   POINT MaxPos;
#endif

   CREATESTRUCTW Cs;
   CBT_CREATEWNDW CbtCreate;
   LRESULT Result;
   BOOL MenuChanged;
   BOOL ClassFound;
//   HWND hSelf;
//   BOOL HasOwner;

   DPRINT1("coUserCreateWindowEx W32 thread TID:%d \n", PsGetCurrentThread()->Cid.UniqueThread);

   ASSERT(PsGetWin32Thread());
   ASSERT(PsGetWin32Thread()->Desktop);
   ParentWindowHandle = PsGetWin32Thread()->Desktop->DesktopWindow;

   if (hWndParent == HWND_MESSAGE)
   {
      /*
       * native ole32.OleInitialize uses HWND_MESSAGE to create the
       * message window (style: WS_POPUP|WS_DISABLED)
       */
      DPRINT1("FIXME - Parent is HWND_MESSAGE\n");
   }
   else if (hWndParent)
   {
      if ((dwStyle & (WS_CHILD | WS_POPUP)) == WS_CHILD)
         ParentWindowHandle = hWndParent;
      else
         OwnerWindow = UserGetAncestor(GetWnd(hWndParent), GA_ROOT);
   }
   else if ((dwStyle & (WS_CHILD | WS_POPUP)) == WS_CHILD)
   {
      return (HWND)0;  /* WS_CHILD needs a parent, but WS_POPUP doesn't */
   }

   if (NULL != ParentWindowHandle)
   {
      ParentWindow = UserGetWindowObject(ParentWindowHandle);
   }
   else
   {
      ParentWindow = NULL;
   }

   /* FIXME: parent must belong to the current process */

#if 0
   /* if parent belongs to a different thread, attach the two threads */
   //FIXME: can ParentWindow->WThread be NULL? (wine checks for this)
   if (ParentWindow && /*ParentWindow->WThread &&*/ ParentWindow->WThread != PsGetWin32Thread())
   {
      if (!attach_thread_input( PsGetWin32Thread(), ParentWindow->WThread )) //goto failed;
         return((HWND)0);
   }
#endif
   /* Check the class. */
   ClassFound = ClassReferenceClassByNameOrAtom(&ClassObject, ClassName->Buffer, hInstance);
   if (!ClassFound)
   {
      if (IS_ATOM(ClassName->Buffer))
      {
         DPRINT1("Class 0x%x not found\n", (DWORD_PTR) ClassName->Buffer);
      }
      else
      {
         DPRINT1("Class %wZ not found\n", ClassName);
      }
      SetLastWin32Error(ERROR_CANNOT_FIND_WND_CLASS);
      return((HWND)0);
   }

   /* Check the window station. */
   if (PsGetWin32Thread()->Desktop == NULL)
   {
      UserDereferenceClass(ClassObject);
      DPRINT("Thread is not attached to a desktop! Cannot create window!\n");
      return (HWND)0;
   }

   Wnd = UserCreateWindowObject(sizeof(WINDOW_OBJECT) + ClassObject->cbWndExtra);

   DPRINT("Created object with handle %X\n", Wnd->hSelf);
   if (!Wnd)
   {
      UserDereferenceClass(ClassObject);
      SetLastNtError(STATUS_INSUFFICIENT_RESOURCES);
      return (HWND)0;
   }

   if (NULL == PsGetWin32Thread()->Desktop->DesktopWindow)
   {
      /* If there is no desktop window yet, we must be creating it */
      PsGetWin32Thread()->Desktop->DesktopWindow = Wnd->hSelf;
   }

   /*
    * Fill out the structure describing it.
    */
   Wnd->Class = ClassObject;

   //er dette nødvendig?
   InsertTailList(&ClassObject->ClassWindowsListHead, &Wnd->ClassListEntry);

   Wnd->ExStyle = dwExStyle;
   Wnd->Style = dwStyle & ~WS_VISIBLE;
   DPRINT("1: Style is now %lx\n", Wnd->Style);

   Wnd->SystemMenu = (HMENU)0;
   Wnd->ContextHelpId = 0;
   Wnd->IDMenu = 0;
   Wnd->Instance = hInstance;

   if (0 != (dwStyle & WS_CHILD))
   {
      Wnd->IDMenu = (UINT) hMenu;
   }
   else
   {
      UserSetMenu(Wnd, hMenu, &MenuChanged);
   }

   Wnd->WThread = PsGetWin32Thread();//MessageQueue = UserGetCurrentQueue();

   ASSERT(Wnd->WThread);

   DPRINT1("Set 0x%x's parent to 0x%x\n",Wnd, ParentWindow);
   Wnd->ParentWnd = ParentWindow;

#if 0
    if (!(parent = get_window( req->parent ))) return;
    if (req->owner)
    {
        if (!(owner = get_window( req->owner ))) return;
        if (is_desktop_window(owner)) owner = NULL;
        else if (!is_desktop_window(parent))
        {
            /* an owned window must be created as top-level */
            set_error( STATUS_ACCESS_DENIED );
            return;
        }
    }

#endif

   //if (OwnerWindow) 
   Wnd->hOwnerWnd = GetHwndSafe(OwnerWindow);//->hSelf;
   
//   OwnerWindow = UserGetWindowObject(OwnerWindowHandle)
   
//   Wnd->hOwnerWnd = OwnerWindowHandle;
//   if((OwnerWindow = UserGetWindowObject(OwnerWindowHandle)))
//   {
//      Wnd->hOwnerWnd = OwnerWindowHandle;
//      HasOwner = TRUE;
//   }
//   else
//   {
//      Wnd->hOwnerWnd = NULL;
//      HasOwner = FALSE;
//   }

   Wnd->UserData = 0;

   if ((((DWORD)ClassObject->lpfnWndProcA & 0xFFFF0000) != 0xFFFF0000)
         && (((DWORD)ClassObject->lpfnWndProcW & 0xFFFF0000) != 0xFFFF0000))
   {
      Wnd->Unicode = bUnicodeWindow;
   }
   else
   {
      Wnd->Unicode = ClassObject->Unicode;
   }

   Wnd->WndProcA = ClassObject->lpfnWndProcA;
   Wnd->WndProcW = ClassObject->lpfnWndProcW;
   Wnd->WThread = PsGetWin32Thread();
   Wnd->FirstChild = NULL;
   Wnd->LastChild = NULL;
   Wnd->PrevSibling = NULL;
   Wnd->NextSibling = NULL;
   Wnd->hLastActiveWnd = Wnd->hSelf;

   /* extra window data */
   if (ClassObject->cbWndExtra != 0)
   {
      Wnd->ExtraData = (PCHAR)(Wnd + 1);
      Wnd->ExtraDataSize = ClassObject->cbWndExtra;
      //RtlZeroMemory(WindowObject->ExtraData, WindowObject->ExtraDataSize);
   }
   else
   {
      Wnd->ExtraData = NULL;
      Wnd->ExtraDataSize = 0;
   }

   InitializeListHead(&Wnd->PropListHead);
   InitializeListHead(&Wnd->WndObjListHead); //wtf???

   //  ExInitializeFastMutex(&WindowObject->UpdateLock);

   if (NULL != WindowName->Buffer)
   {
      Wnd->WindowName.MaximumLength = WindowName->MaximumLength;
      Wnd->WindowName.Length = WindowName->Length;
      Wnd->WindowName.Buffer = ExAllocatePoolWithTag(PagedPool, WindowName->MaximumLength,
                               TAG_STRING);
      if (NULL == Wnd->WindowName.Buffer)
      {
         UserDereferenceClass(ClassObject);
         DPRINT1("Failed to allocate mem for window name\n");
         SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
         return NULL;
      }
      RtlCopyMemory(Wnd->WindowName.Buffer, WindowName->Buffer, WindowName->MaximumLength);
   }
   else
   {
      RtlInitUnicodeString(&Wnd->WindowName, NULL);
   }


   /*
    * This has been tested for WS_CHILD | WS_VISIBLE.  It has not been
    * tested for WS_POPUP
    */
   if ((dwExStyle & WS_EX_DLGMODALFRAME) ||
         ((!(dwExStyle & WS_EX_STATICEDGE)) &&
          (dwStyle & (WS_DLGFRAME | WS_THICKFRAME))))
      dwExStyle |= WS_EX_WINDOWEDGE;
   else
      dwExStyle &= ~WS_EX_WINDOWEDGE;

   /* Correct the window style. */
   if (!(dwStyle & WS_CHILD))
   {
      Wnd->Style |= WS_CLIPSIBLINGS;
      DPRINT("3: Style is now %lx\n", Wnd->Style);
      if (!(dwStyle & WS_POPUP))
      {
         Wnd->Style |= WS_CAPTION;
         Wnd->Flags |= WINDOWOBJECT_NEED_SIZE;
         DPRINT("4: Style is now %lx\n", Wnd->Style);
      }
   }

   /* create system menu */
   if((Wnd->Style & WS_SYSMENU) &&
         (Wnd->Style & WS_CAPTION) == WS_CAPTION)
   {
      SystemMenu = co_UserGetSystemMenu(Wnd, TRUE, TRUE);
      if(SystemMenu)
      {
         Wnd->SystemMenu = SystemMenu->MenuInfo.Self;
      }
   }

   /* Insert the window into the thread's window list. */
   InsertTailList (&PsGetWin32Thread()->WindowListHead, &Wnd->ThreadListEntry);

   /* Allocate a DCE for this window. */
   if (dwStyle & CS_OWNDC)
   {
      Wnd->Dce = UserDceAllocDCE(Wnd->hSelf, DCE_WINDOW_DC);
   }
   /* FIXME:  Handle "CS_CLASSDC" */

   Pos.x = x;
   Pos.y = y;
   Size.cx = nWidth;
   Size.cy = nHeight;

   /* call hook */
   Cs.lpCreateParams = lpParam;
   Cs.hInstance = hInstance;
   Cs.hMenu = hMenu;
   Cs.hwndParent = ParentWindowHandle;
   Cs.cx = Size.cx;
   Cs.cy = Size.cy;
   Cs.x = Pos.x;
   Cs.y = Pos.y;
   Cs.style = dwStyle;
   Cs.lpszName = (LPCWSTR) WindowName;
   Cs.lpszClass = (LPCWSTR) ClassName;
   Cs.dwExStyle = dwExStyle;
   CbtCreate.lpcs = &Cs;
   CbtCreate.hwndInsertAfter = HWND_TOP;

   if (co_HOOK_CallHooks(WH_CBT, HCBT_CREATEWND, (WPARAM) Wnd->hSelf, (LPARAM) &CbtCreate))
   {
      /* FIXME - Delete window object and remove it from the thread windows list */
      /* FIXME - delete allocated DCE */

      UserDereferenceClass(ClassObject);

      DPRINT1("CBT-hook returned !0\n");
      return (HWND) NULL;
   }

   x = Cs.x;
   y = Cs.y;
   nWidth = Cs.cx;
   nHeight = Cs.cy;

   /* default positioning for overlapped windows */
   if(!(Wnd->Style & (WS_POPUP | WS_CHILD)))
   {
      RECT rc, WorkArea;
      PRTL_USER_PROCESS_PARAMETERS ProcessParams;
      BOOL CalculatedDefPosSize = FALSE;

      IntGetDesktopWorkArea(Wnd->WThread->Desktop, &WorkArea);

      rc = WorkArea;
      ProcessParams = PsGetCurrentProcess()->Peb->ProcessParameters;

      if(x == CW_USEDEFAULT || x == CW_USEDEFAULT16)
      {
         CalculatedDefPosSize = IntCalcDefPosSize(ParentWindow, Wnd, &rc, TRUE);

         if(ProcessParams->WindowFlags & STARTF_USEPOSITION)
         {
            ProcessParams->WindowFlags &= ~STARTF_USEPOSITION;
            Pos.x = WorkArea.left + ProcessParams->StartingX;
            Pos.y = WorkArea.top + ProcessParams->StartingY;
         }
         else
         {
            Pos.x = rc.left;
            Pos.y = rc.top;
         }

         /* According to wine, the ShowMode is set to y if x == CW_USEDEFAULT(16) and
            y is something else */
         if(y != CW_USEDEFAULT && y != CW_USEDEFAULT16)
         {
            dwShowMode = y;
         }
      }
      if(nWidth == CW_USEDEFAULT || nWidth == CW_USEDEFAULT16)
      {
         if(!CalculatedDefPosSize)
         {
            IntCalcDefPosSize(ParentWindow, Wnd, &rc, FALSE);
         }
         if(ProcessParams->WindowFlags & STARTF_USESIZE)
         {
            ProcessParams->WindowFlags &= ~STARTF_USESIZE;
            Size.cx = ProcessParams->CountX;
            Size.cy = ProcessParams->CountY;
         }
         else
         {
            Size.cx = rc.right - rc.left;
            Size.cy = rc.bottom - rc.top;
         }

         /* move the window if necessary */
         if(Pos.x > rc.left)
            Pos.x = max(rc.left, 0);
         if(Pos.y > rc.top)
            Pos.y = max(rc.top, 0);
      }
   }
   else
   {
      /* if CW_USEDEFAULT(16) is set for non-overlapped windows, both values are set to zero) */
      if(x == CW_USEDEFAULT || x == CW_USEDEFAULT16)
      {
         Pos.x = 0;
         Pos.y = 0;
      }
      if(nWidth == CW_USEDEFAULT || nWidth == CW_USEDEFAULT16)
      {
         Size.cx = 0;
         Size.cy = 0;
      }
   }

   /* Initialize the window dimensions. */
   Wnd->WindowRect.left = Pos.x;
   Wnd->WindowRect.top = Pos.y;
   Wnd->WindowRect.right = Pos.x + Size.cx;
   Wnd->WindowRect.bottom = Pos.y + Size.cy;
   if (0 != (Wnd->Style & WS_CHILD) && ParentWindow)
   {
      IntGdiOffsetRect(&(Wnd->WindowRect), ParentWindow->ClientRect.left,
                       ParentWindow->ClientRect.top);
   }
   Wnd->ClientRect = Wnd->WindowRect;

   /*
    * Get the size and position of the window.
    */
   if ((dwStyle & WS_THICKFRAME) || !(dwStyle & (WS_POPUP | WS_CHILD)))
   {
      POINT MaxSize, MaxPos, MinTrack, MaxTrack;

      /* WinPosGetMinMaxInfo sends the WM_GETMINMAXINFO message */
      co_WinPosGetMinMaxInfo(Wnd, &MaxSize, &MaxPos, &MinTrack,
                             &MaxTrack);

      if (MaxSize.x < nWidth)
         nWidth = MaxSize.x;
      if (MaxSize.y < nHeight)
         nHeight = MaxSize.y;
      if (nWidth < MinTrack.x )
         nWidth = MinTrack.x;
      if (nHeight < MinTrack.y )
         nHeight = MinTrack.y;
      if (nWidth < 0)
         nWidth = 0;
      if (nHeight < 0)
         nHeight = 0;
   }

   Wnd->WindowRect.left = Pos.x;
   Wnd->WindowRect.top = Pos.y;
   Wnd->WindowRect.right = Pos.x + Size.cx;
   Wnd->WindowRect.bottom = Pos.y + Size.cy;
   if (0 != (Wnd->Style & WS_CHILD) && ParentWindow)
   {
      IntGdiOffsetRect(&(Wnd->WindowRect), ParentWindow->ClientRect.left,
                       ParentWindow->ClientRect.top);
   }
   Wnd->ClientRect = Wnd->WindowRect;

   /* FIXME: Initialize the window menu. */

   /* Send a NCCREATE message. */
   Cs.cx = Size.cx;
   Cs.cy = Size.cy;
   Cs.x = Pos.x;
   Cs.y = Pos.y;

   DPRINT("[win32k.window] coUserCreateWindowEx style %d, exstyle %d, parent %d\n", Cs.style, Cs.dwExStyle, Cs.hwndParent);
   DPRINT("coUserCreateWindowEx(): (%d,%d-%d,%d)\n", x, y, nWidth, nHeight);
   DPRINT("coUserCreateWindowEx(): About to send NCCREATE message.\n");
   Result = co_UserSendMessage(Wnd->hSelf, WM_NCCREATE, 0, (LPARAM) &Cs);
   if (!Result)
   {
      /* FIXME: Cleanup. */

      DPRINT("coUserCreateWindowEx(): NCCREATE message failed.\n");
      return((HWND)0);
   }

   /* Calculate the non-client size. */
   MaxPos.x = Wnd->WindowRect.left;
   MaxPos.y = Wnd->WindowRect.top;
   DPRINT("coUserCreateWindowEx(): About to get non-client size.\n");
   /* WinPosGetNonClientSize SENDS THE WM_NCCALCSIZE message */
   Result = co_WinPosGetNonClientSize(Wnd, &Wnd->WindowRect, &Wnd->ClientRect);
   IntGdiOffsetRect(&Wnd->WindowRect,
                    MaxPos.x - Wnd->WindowRect.left,
                    MaxPos.y - Wnd->WindowRect.top);

   if (NULL != ParentWindow)
   {
      /* link the window into the parent's child list */
      if ((dwStyle & (WS_CHILD|WS_MAXIMIZE)) == WS_CHILD)
      {
         PWINDOW_OBJECT PrevSibling;

         PrevSibling = ParentWindow->LastChild;

         /* link window as bottom sibling */
         IntLinkWindow(Wnd, ParentWindow, PrevSibling /*prev sibling*/);
      }
      else
      {
         /* link window as top sibling (but after topmost siblings) */
         PWINDOW_OBJECT InsertAfter, Sibling;
         if (0 == (dwExStyle & WS_EX_TOPMOST))
         {
            InsertAfter = NULL;
            Sibling = ParentWindow->FirstChild;
            while (NULL != Sibling && 0 != (Sibling->ExStyle & WS_EX_TOPMOST))
            {
               InsertAfter = Sibling;
               Sibling = Sibling->NextSibling;
            }
         }
         else
         {
            InsertAfter = NULL;
         }

         IntLinkWindow(Wnd, ParentWindow, InsertAfter /* prev sibling */);
      }
   }

   /* Send the WM_CREATE message. */
   DPRINT("coUserCreateWindowEx(): about to send CREATE message.\n");
   Result = co_UserSendMessage(Wnd->hSelf, WM_CREATE, 0, (LPARAM) &Cs);
   if (Result == (LRESULT)-1)
   {
      /* FIXME: Cleanup. */

      DPRINT("coUserCreateWindowEx(): send CREATE message failed.\n");
      return((HWND)0);
   }

   /* Send move and size messages. */
   if (!(Wnd->Flags & WINDOWOBJECT_NEED_SIZE))
   {
      LONG lParam;

      DPRINT("IntCreateWindow(): About to send WM_SIZE\n");

      if ((Wnd->ClientRect.right - Wnd->ClientRect.left) < 0 ||
            (Wnd->ClientRect.bottom - Wnd->ClientRect.top) < 0)
      {
         DPRINT("Sending bogus WM_SIZE\n");
      }

      lParam = MAKE_LONG(Wnd->ClientRect.right -
                         Wnd->ClientRect.left,
                         Wnd->ClientRect.bottom -
                         Wnd->ClientRect.top);
      co_UserSendMessage(Wnd->hSelf, WM_SIZE, SIZE_RESTORED, lParam);

      DPRINT("IntCreateWindow(): About to send WM_MOVE\n");

      if (0 != (Wnd->Style & WS_CHILD) && ParentWindow)
      {
         lParam = MAKE_LONG(Wnd->ClientRect.left - ParentWindow->ClientRect.left,
                            Wnd->ClientRect.top - ParentWindow->ClientRect.top);
      }
      else
      {
         lParam = MAKE_LONG(Wnd->ClientRect.left,
                            Wnd->ClientRect.top);
      }
      co_UserSendMessage(Wnd->hSelf, WM_MOVE, 0, lParam);

      /* Call WNDOBJ change procs */
      IntEngWindowChanged(Wnd, WOC_RGN_CLIENT);
   }

   /* Show or maybe minimize or maximize the window. */
   if (Wnd->Style & (WS_MINIMIZE | WS_MAXIMIZE))
   {
      RECT NewPos;
      UINT16 SwFlag;

      SwFlag = (Wnd->Style & WS_MINIMIZE) ? SW_MINIMIZE : SW_MAXIMIZE;
      co_WinPosMinMaximize(Wnd, SwFlag, &NewPos);
      SwFlag =
         ((Wnd->Style & WS_CHILD) || UserGetActiveWindow()) ?
         SWP_NOACTIVATE | SWP_NOZORDER | SWP_FRAMECHANGED :
         SWP_NOZORDER | SWP_FRAMECHANGED;
      DPRINT("IntCreateWindow(): About to minimize/maximize\n");
      DPRINT("%d,%d %dx%d\n", NewPos.left, NewPos.top, NewPos.right, NewPos.bottom);
      co_WinPosSetWindowPos(Wnd->hSelf, 0, NewPos.left, NewPos.top,
                            NewPos.right, NewPos.bottom, SwFlag);
   }

   /* Notify the parent window of a new child. */
   if ((Wnd->Style & WS_CHILD) && !(Wnd->ExStyle & WS_EX_NOPARENTNOTIFY) && ParentWindow)
   {
      DPRINT("IntCreateWindow(): About to notify parent\n");
      co_UserSendMessage(ParentWindow->hSelf,
                         WM_PARENTNOTIFY,
                         MAKEWPARAM(WM_CREATE, Wnd->IDMenu),
                         (LPARAM)Wnd->hSelf);
   }

   if (!hWndParent && OwnerWindow /*HasOwner*/)
   {
      DPRINT("Sending CREATED notify\n");
      co_UserShellHookNotify(HSHELL_WINDOWCREATED, (LPARAM)Wnd->hSelf);
   }
   else
   {
      DPRINT("Not sending CREATED notify, %x %d\n", ParentWindow, OwnerWindow != NULL /*HasOwner*/);
   }

   /* Initialize and show the window's scrollbars */
   if (Wnd->Style & WS_VSCROLL)
   {
      co_UserShowScrollBar(Wnd, SB_VERT, TRUE);
   }
   if (Wnd->Style & WS_HSCROLL)
   {
      co_UserShowScrollBar(Wnd, SB_HORZ, TRUE);
   }

   if (dwStyle & WS_VISIBLE)
   {
      DPRINT("IntCreateWindow(): About to show window\n");
      co_WinPosShowWindow(Wnd, dwShowMode);
   }

   DPRINT("IntCreateWindow(): = %X\n", Wnd->hSelf);
   DPRINT("WindowObject->SystemMenu = 0x%x\n", Wnd->SystemMenu);

//   hSelf = Wnd->hSelf;

   return Wnd->hSelf;
}

HWND STDCALL
NtUserCreateWindowEx(DWORD dwExStyle,
                     PUNICODE_STRING UnsafeClassName,
                     PUNICODE_STRING UnsafeWindowName,
                     DWORD dwStyle,
                     LONG x,
                     LONG y,
                     LONG nWidth,
                     LONG nHeight,
                     HWND hWndParent,
                     HMENU hMenu,
                     HINSTANCE hInstance,
                     LPVOID lpParam,
                     DWORD dwShowMode,
                     BOOL bUnicodeWindow)
{
   NTSTATUS Status;
   UNICODE_STRING WindowName;
   UNICODE_STRING ClassName;
   HWND NewWindow;
   DECLARE_RETURN(HWND);

   DPRINT("Enter NtUserCreateWindowEx(): (%d,%d-%d,%d)\n", x, y, nWidth, nHeight);
   UserEnterExclusive();

   /* Get the class name (string or atom) */
   Status = MmCopyFromCaller(&ClassName, UnsafeClassName, sizeof(UNICODE_STRING));
   if (! NT_SUCCESS(Status))
   {
      SetLastNtError(Status);
      RETURN(NULL);
   }
   if (! IS_ATOM(ClassName.Buffer))
   {
      Status = IntSafeCopyUnicodeStringTerminateNULL(&ClassName, UnsafeClassName);
      if (! NT_SUCCESS(Status))
      {
         SetLastNtError(Status);
         RETURN(NULL);
      }
   }

   /* safely copy the window name */
   if (NULL != UnsafeWindowName)
   {
      Status = IntSafeCopyUnicodeString(&WindowName, UnsafeWindowName);
      if (! NT_SUCCESS(Status))
      {
         if (! IS_ATOM(ClassName.Buffer))
         {
            RtlFreeUnicodeString(&ClassName);
         }
         SetLastNtError(Status);
         RETURN(NULL);
      }
   }
   else
   {
      RtlInitUnicodeString(&WindowName, NULL);
   }

   NewWindow = co_UserCreateWindowEx(dwExStyle, &ClassName, &WindowName, dwStyle, x, y, nWidth, nHeight,
                                     hWndParent, hMenu, hInstance, lpParam, dwShowMode, bUnicodeWindow);

   RtlFreeUnicodeString(&WindowName);
   if (! IS_ATOM(ClassName.Buffer))
   {
      RtlFreeUnicodeString(&ClassName);
   }

   RETURN(NewWindow);

CLEANUP:
   DPRINT("Leave NtUserCreateWindowEx, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}

/*
 * @unimplemented
 */
HDWP STDCALL
NtUserDeferWindowPos(HDWP WinPosInfo,
                     HWND Wnd,
                     HWND WndInsertAfter,
                     int x,
                     int y,
                     int cx,
                     int cy,
                     UINT Flags)
{
   UNIMPLEMENTED

   return 0;
}


/*
 * @implemented
 */
BOOLEAN STDCALL
NtUserDestroyWindow(HWND hWnd)
{
   PWINDOW_OBJECT Wnd;
   DECLARE_RETURN(BOOLEAN);

   DPRINT("Enter NtUserDestroyWindow\n");
   UserEnterExclusive();

   if (!(Wnd = UserGetWindowObject(hWnd)))
   {
      SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
      RETURN(FALSE);
   }

   RETURN(co_UserDestroyWindow(Wnd));

CLEANUP:
   DPRINT("Leave NtUserDestroyWindow, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}





BOOLEAN FASTCALL
co_UserDestroyWindow(PWINDOW_OBJECT Wnd)
{
   BOOLEAN isChild;

   ASSERT(Wnd);
   //ASSERT_REFS(Wnd); FIXME: handle destroy in a special way

   /* Check for owner thread and desktop window */
   //FIXME: move this inot NtUserDestroyWindow??

   if ((Wnd->WThread != PsGetWin32Thread()) || IntIsDesktopWindow(Wnd))
   {
      SetLastWin32Error(ERROR_ACCESS_DENIED);
      return(FALSE);
   }

   /* Look whether the focus is within the tree of windows we will
    * be destroying.
    */
   if (!co_WinPosShowWindow(Wnd, SW_HIDE))
   {
      if (UserGetActiveWindow() == Wnd)
      {
         co_WinPosActivateOtherWindow(Wnd);
      }
   }
   //  IntDereferenceMessageQueue(Window->MessageQueue);
   if (Wnd->Queue->Input->hActiveWindow == Wnd->hSelf)
      Wnd->Queue->Input->hActiveWindow = NULL;

   if (Wnd->Queue->Input->hFocusWindow == Wnd->hSelf)
      Wnd->Queue->Input->hFocusWindow = NULL;

   if (Wnd->Queue->Input->hCaptureWindow == Wnd->hSelf)
      Wnd->Queue->Input->hCaptureWindow = NULL;


   /* Call hooks */
#if 0 /* FIXME */

   if (HOOK_CallHooks(WH_CBT, HCBT_DESTROYWND, (WPARAM) hwnd, 0, TRUE))
   {
      return(FALSE);
   }
#endif

   IntEngWindowChanged(Wnd, WOC_DELETE);
   isChild = (0 != (Wnd->Style & WS_CHILD));

#if 0 /* FIXME */

   if (isChild)
   {
      if (! USER_IsExitingThread(GetCurrentThreadId()))
      {
         send_parent_notify(hwnd, WM_DESTROY);
      }
   }
   else if (NULL != GetWindow(hWnd, GW_OWNER))
   {
      HOOK_CallHooks( WH_SHELL, HSHELL_WINDOWDESTROYED, (WPARAM)hwnd, 0L, TRUE );
      /* FIXME: clean up palette - see "Internals" p.352 */
   }
#endif

   if (!IntIsWindow(Wnd->hSelf))
   {
      return(TRUE);
   }

   /* Recursively destroy owned windows */
   if (! isChild)
   {
      for (;;)
      {
         BOOL GotOne = FALSE;
         HWND *List;
         //   HWND *ChildHandle;
         PWINDOW_OBJECT Child, Desktop;

         Desktop = UserGetDesktopWindow();
         List = IntWinListChildren(Desktop);

         if (List)
         {
            int i;
            for (i=0; List[i]; i++)
            {
               Child = UserGetWindowObject(List[i]);
               if (Child == NULL)
                  continue;
               if (UserGetWindowObject(Child->hOwnerWnd) != Wnd)
               {
                  continue;
               }

               if (IntWndBelongsToThread(Child, PsGetWin32Thread()))
               {
                  co_UserDestroyWindow(GetWnd(List[i]));
                  GotOne = TRUE;
                  continue;
               }

               if (Child->hOwnerWnd)//FIXME: why
               {
                  Child->hOwnerWnd = NULL;//what good is this?
               }

            }
            ExFreePool(List);
         }
         if (! GotOne)
         {
            break;
         }
      }
   }

   if (!IntIsWindow(Wnd->hSelf))
   {
      return(TRUE);
   }

   /* Destroy the window storage */
   co_IntDestroyWindowStorage(Wnd, PsGetWin32Process(), PsGetWin32Thread());

   return(TRUE);
}




/*
 * @unimplemented
 */
DWORD
STDCALL
NtUserDrawMenuBarTemp(
   HWND hWnd,
   HDC hDC,
   PRECT hRect,
   HMENU hMenu,
   HFONT hFont)
{
   /* we'll use this function just for caching the menu bar */
   UNIMPLEMENTED
   return 0;
}


/*
 * @unimplemented
 */
DWORD STDCALL
NtUserEndDeferWindowPosEx(DWORD Unknown0,
                          DWORD Unknown1)
{
   UNIMPLEMENTED

   return 0;
}


/*
 * @unimplemented
 */
DWORD STDCALL
NtUserFillWindow(DWORD Unknown0,
                 DWORD Unknown1,
                 DWORD Unknown2,
                 DWORD Unknown3)
{
   UNIMPLEMENTED

   return 0;
}


HWND FASTCALL
IntFindWindow(PWINDOW_OBJECT Parent,
              PWINDOW_OBJECT ChildAfter,
              RTL_ATOM ClassAtom,
              PUNICODE_STRING WindowName)
{
   BOOL CheckWindowName;
   HWND *List, *pWnd;
   HWND Ret = NULL;

   ASSERT(Parent);

   CheckWindowName = (WindowName && (WindowName->Length > 0));

   if((List = IntWinListChildren(Parent)))
   {
      pWnd = List;
      if(ChildAfter)
      {
         /* skip handles before and including ChildAfter */
         while(*pWnd && (*(pWnd++) != ChildAfter->hSelf))
            ;
      }

      /* search children */
      while(*pWnd)
      {
         PWINDOW_OBJECT Child;// = *pWnd;
         if(!(Child = UserGetWindowObject(*(pWnd++))))
         {
            continue;
         }

         /* Do not send WM_GETTEXT messages in the kernel mode version!
            The user mode version however calls GetWindowText() which will
            send WM_GETTEXT messages to windows belonging to its processes */
         if((!CheckWindowName || !RtlCompareUnicodeString(WindowName, &(Child->WindowName), FALSE)) &&
               (!ClassAtom || Child->Class->Atom == ClassAtom))
         {
            Ret = Child->hSelf;
            break;
         }

      }
      ExFreePool(List);
   }

   return Ret;
}

/*
 * FUNCTION:
 *   Searches a window's children for a window with the specified
 *   class and name
 * ARGUMENTS:
 *   hwndParent     = The window whose childs are to be searched.
 *       NULL = desktop
 *       HWND_MESSAGE = message-only windows
 *
 *   hwndChildAfter = Search starts after this child window.
 *       NULL = start from beginning
 *
 *   ucClassName    = Class name to search for
 *       Reguired parameter.
 *
 *   ucWindowName   = Window name
 *       ->Buffer == NULL = don't care
 *
 * RETURNS:
 *   The HWND of the window if it was found, otherwise NULL
 */
/*
 * @implemented
 */
HWND STDCALL
NtUserFindWindowEx(HWND hwndParent,
                   HWND hwndChildAfter,
                   PUNICODE_STRING ucClassName,
                   PUNICODE_STRING ucWindowName)
{
   PWINDOW_OBJECT Parent, ChildAfter;
   UNICODE_STRING ClassName, WindowName;
   NTSTATUS Status;
   HWND Desktop, Ret = NULL;
   RTL_ATOM ClassAtom;
   DECLARE_RETURN(HWND);

   DPRINT("Enter NtUserFindWindowEx\n");
   UserEnterExclusive();

   Desktop = IntGetCurrentThreadDesktopWindow();//FIXME

   if(hwndParent == NULL)
      hwndParent = Desktop;
   /* FIXME
   else if(hwndParent == HWND_MESSAGE)
   {
     hwndParent = IntGetMessageWindow();
   }
   */

   if(!(Parent = UserGetWindowObject(hwndParent)))
   {
      SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
      RETURN( NULL);
   }

   ChildAfter = NULL;
   if(hwndChildAfter && !(ChildAfter = UserGetWindowObject(hwndChildAfter)))
   {
      SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
      RETURN( NULL);
   }

   /* copy the window name */
   Status = IntSafeCopyUnicodeString(&WindowName, ucWindowName);
   if(!NT_SUCCESS(Status))
   {
      SetLastNtError(Status);
      goto Cleanup3;
   }

   /* safely copy the class name */
   Status = MmCopyFromCaller(&ClassName, ucClassName, sizeof(UNICODE_STRING));
   if(!NT_SUCCESS(Status))
   {
      SetLastNtError(Status);
      goto Cleanup2;
   }
   if(ClassName.Length > 0 && ClassName.Buffer)
   {
      WCHAR *buf;
      /* safely copy the class name string (NULL terminated because class-lookup
         depends on it... */
      buf = ExAllocatePoolWithTag(PagedPool, ClassName.Length + sizeof(WCHAR), TAG_STRING);
      if(!buf)
      {
         SetLastWin32Error(STATUS_INSUFFICIENT_RESOURCES);
         goto Cleanup2;
      }
      Status = MmCopyFromCaller(buf, ClassName.Buffer, ClassName.Length);
      if(!NT_SUCCESS(Status))
      {
         ExFreePool(buf);
         SetLastNtError(Status);
         goto Cleanup2;
      }
      ClassName.Buffer = buf;
      /* make sure the string is null-terminated */
      buf += ClassName.Length / sizeof(WCHAR);
      *buf = L'\0';
   }

   /* find the class object */
   if(ClassName.Buffer)
   {
      PWINSTATION_OBJECT WinStaObject;

      if (PsGetWin32Thread()->Desktop == NULL)
      {
         SetLastWin32Error(ERROR_INVALID_HANDLE);
         goto Cleanup;
      }

      WinStaObject = UserGetCurrentWinSta();

      /* fixme: classes are not per winsta, but per session!! use session atom table instead */
      Status = RtlLookupAtomInAtomTable(
                  WinStaObject->AtomTable,
                  ClassName.Buffer,
                  &ClassAtom);

      if (!NT_SUCCESS(Status))
      {
         DPRINT1("Failed to lookup class atom!\n");
         SetLastWin32Error(ERROR_CLASS_DOES_NOT_EXIST);
         goto Cleanup;
      }
   }

   if(Parent->hSelf == Desktop)
   {
      HWND *List, *pWnd;
      PWINDOW_OBJECT TopLevelWindow;
      BOOLEAN CheckWindowName;
      BOOLEAN CheckClassName;
      BOOLEAN WindowMatches;
      BOOLEAN ClassMatches;

      /* windows searches through all top-level windows if the parent is the desktop
         window */

      if((List = IntWinListChildren(Parent)))
      {
         pWnd = List;

         if(ChildAfter)
         {
            /* skip handles before and including ChildAfter */
            while(*pWnd && (*(pWnd++) != ChildAfter->hSelf))
               ;
         }

         CheckWindowName = WindowName.Length > 0;
         CheckClassName = ClassName.Buffer != NULL;

         /* search children */
         while(*pWnd)
         {
            if(!(TopLevelWindow = UserGetWindowObject(*(pWnd++))))
            {
               continue;
            }
            //        TopLevelWindow = *pWnd;

            /* Do not send WM_GETTEXT messages in the kernel mode version!
               The user mode version however calls GetWindowText() which will
               send WM_GETTEXT messages to windows belonging to its processes */
            WindowMatches = !CheckWindowName || RtlEqualUnicodeString(
                               &WindowName, &TopLevelWindow->WindowName, FALSE);
            ClassMatches = !CheckClassName ||
                           ClassAtom == TopLevelWindow->Class->Atom;

            if (WindowMatches && ClassMatches)
            {
               Ret = TopLevelWindow->hSelf;
               break;
            }

            if (IntFindWindow(TopLevelWindow, NULL, ClassAtom, &WindowName))
            {
               /* window returns the handle of the top-level window, in case it found
                  the child window */
               Ret = TopLevelWindow->hSelf;
               break;
            }

         }
         ExFreePool(List);
      }
   }
   else
      Ret = IntFindWindow(Parent, ChildAfter, ClassAtom, &WindowName);

#if 0

   if(Ret == NULL && hwndParent == NULL && hwndChildAfter == NULL)
   {
      /* FIXME - if both hwndParent and hwndChildAfter are NULL, we also should
                 search the message-only windows. Should this also be done if
                 Parent is the desktop window??? */
      PWINDOW_OBJECT MsgWindows;

      if((MsgWindows = UserGetWindowObject(IntGetMessageWindow())))
      {
         Ret = IntFindWindow(MsgWindows, ChildAfter, ClassAtom, &WindowName);
      }
   }
#endif

Cleanup:
   if(ClassName.Length > 0 && ClassName.Buffer)
      ExFreePool(ClassName.Buffer);

Cleanup2:
   RtlFreeUnicodeString(&WindowName);

Cleanup3:

   RETURN( Ret);

CLEANUP:
   DPRINT("Leave NtUserFindWindowEx, ret %i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}


/*
 * @unimplemented
 */
DWORD STDCALL
NtUserFlashWindowEx(DWORD Unknown0)
{
   UNIMPLEMENTED

   return 0;
}


/*
 * @implemented
 */
HWND STDCALL
NtUserGetAncestor(HWND hWnd, UINT Type)
{
   PWINDOW_OBJECT Wnd;
   DECLARE_RETURN(HWND);

   DPRINT("Enter NtUserGetAncestor\n");
   UserEnterExclusive();

   if (!(Wnd = UserGetWindowObject(hWnd)))
   {
      SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
      RETURN(NULL);
   }

   RETURN(GetHwndSafe(UserGetAncestor(Wnd, Type)));

CLEANUP:
   DPRINT("Leave NtUserGetAncestor, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}


/*
 * @implemented
 */
PWINDOW_OBJECT FASTCALL
UserGetAncestor(PWINDOW_OBJECT Wnd, UINT Type)
{
   PWINDOW_OBJECT WndAncestor = NULL, Parent;

   if (IntIsDesktopWindow(Wnd))
   {
      return NULL;
   }

   switch (Type)
   {
      case GA_PARENT:
         {
            WndAncestor = Wnd->ParentWnd;
            break;
         }

      case GA_ROOT:
         {
            PWINDOW_OBJECT tmp;
            WndAncestor = Wnd;
            Parent = NULL;

            for(;;)
            {
               tmp = Parent;
               if(!(Parent = WndAncestor->ParentWnd))
               {
                  break;
               }
               if(IntIsDesktopWindow(Parent))
               {
                  break;
               }
               WndAncestor = Parent;
            }
            break;
         }

      case GA_ROOTOWNER:
         {
            WndAncestor = Wnd;
            for (;;)
            {
               PWINDOW_OBJECT Old;
               Old = WndAncestor;
               Parent = UserGetParent(WndAncestor);
               if (!Parent)
               {
                  break;
               }
               WndAncestor = Parent;
            }
            break;
         }

      default:
         {
            return NULL;
         }
   }

   return WndAncestor;
}



/*!
 * Returns client window rectangle relative to the upper-left corner of client area.
 *
 * \param hWnd window handle.
 * \param Rect pointer to the buffer where the coordinates are returned.
 *
*/
/*
 * @implemented
 */
BOOL STDCALL
NtUserGetClientRect(HWND hWnd, LPRECT Rect)
{
   PWINDOW_OBJECT WindowObject;
   RECT SafeRect;
   DECLARE_RETURN(BOOL);

   DPRINT("Enter NtUserGetClientRect\n");
   UserEnterExclusive();

   if(!(WindowObject = UserGetWindowObject(hWnd)))
   {
      SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
      RETURN(FALSE);
   }

   //FIXME: check retval?
   IntGetClientRect(WindowObject, &SafeRect);

   if(!NT_SUCCESS(MmCopyToCaller(Rect, &SafeRect, sizeof(RECT))))
   {
      RETURN(FALSE);
   }
   RETURN(TRUE);

CLEANUP:
   DPRINT("Leave NtUserGetClientRect, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}


/*
 * @implemented
 */
HWND STDCALL
NtUserGetDesktopWindow()
{
   DECLARE_RETURN(HWND);

   DPRINT("Enter NtUserGetDesktopWindow\n");
   UserEnterExclusive();

   RETURN(GetHwndSafe(UserGetDesktopWindow()));

CLEANUP:
   DPRINT("Leave NtUserGetDesktopWindow, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}


/*
 * @unimplemented
 */
DWORD STDCALL
NtUserGetInternalWindowPos(DWORD Unknown0,
                           DWORD Unknown1,
                           DWORD Unknown2)
{
   UNIMPLEMENTED

   return 0;
}


/*
 * @unimplemented
 */
HWND STDCALL
NtUserGetLastActivePopup(HWND hWnd)
{
   /*
    * This code can't work, because hWndLastPopup member of WINDOW_OBJECT is
    * not changed anywhere.
    * -- Filip, 01/nov/2003
    */
   DECLARE_RETURN(HWND);
   PWINDOW_OBJECT Wnd;
   
   DPRINT("Enter NtUserGetLastActivePopup\n");
   UserEnterExclusive();

   if (!(Wnd = UserGetWindowObject(hWnd)))
   {
      SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
      return NULL;
   }

   RETURN(Wnd->hLastActiveWnd);

CLEANUP:
   DPRINT("Leave NtUserGetLastActivePopup, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;

}

/*
 * NtUserGetParent
 *
 * The NtUserGetParent function retrieves a handle to the specified window's
 * parent or owner.
 *
 * Remarks
 *    Note that, despite its name, this function can return an owner window
 *    instead of a parent window.
 *
 * Status
 *    @implemented
 */

HWND STDCALL
NtUserGetParent(HWND hWnd)
{
   PWINDOW_OBJECT Wnd;
   DECLARE_RETURN(HWND);

   DPRINT("Enter NtUserGetParent\n");
   UserEnterExclusive();

   if (!(Wnd = UserGetWindowObject(hWnd)))
   {
      SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
      RETURN(NULL);
   }

   RETURN(GetHwndSafe(UserGetParent(Wnd)));

CLEANUP:
   DPRINT("Leave NtUserGetParent, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}

/*
 * NtUserSetParent
 *
 * The NtUserSetParent function changes the parent window of the specified
 * child window.
 *
 * Remarks
 *    The new parent window and the child window must belong to the same
 *    application. If the window identified by the hWndChild parameter is
 *    visible, the system performs the appropriate redrawing and repainting.
 *    For compatibility reasons, NtUserSetParent does not modify the WS_CHILD
 *    or WS_POPUP window styles of the window whose parent is being changed.
 *
 * Status
 *    @implemented
 */

HWND STDCALL
NtUserSetParent(HWND hWndChild, HWND hWndNewParent OPTIONAL)
{
   PWINDOW_OBJECT Wnd, WndNewParent = NULL;
   DECLARE_RETURN(HWND);

   DPRINT("Enter NtUserSetParent\n");
   UserEnterExclusive();

   if (IntIsBroadcastHwnd(hWndChild) || IntIsBroadcastHwnd(hWndNewParent))
   {
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      RETURN(NULL);
   }

   if (hWndNewParent)
   {
      if (!(WndNewParent = UserGetWindowObject(hWndNewParent)))
      {
         SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
         RETURN(NULL);
      }
   }

   if (!(Wnd = UserGetWindowObject(hWndChild)))
   {
      SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
      RETURN(NULL);
   }

   RETURN(GetHwndSafe(co_UserSetParent(Wnd, WndNewParent)));

CLEANUP:
   DPRINT("Leave NtUserSetParent, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}

/*
 * NtUserGetShellWindow
 *
 * Returns a handle to shell window that was set by NtUserSetShellWindowEx.
 *
 * Status
 *    @implemented
 */

HWND STDCALL
NtUserGetShellWindow()
{
   DECLARE_RETURN(HWND);

   DPRINT("Enter NtUserGetShellWindow\n");
   UserEnterExclusive();

   RETURN(GetHwndSafe(UserGetShellWindow()));

CLEANUP:
   DPRINT("Leave NtUserGetShellWindow, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}


PWINDOW_OBJECT FASTCALL UserGetShellWindow()
{
   PWINSTATION_OBJECT WinStaObject;
   HWND Ret;

   NTSTATUS Status = IntValidateWindowStationHandle(PsGetCurrentProcess()->Win32WindowStation,
                     KernelMode,
                     0,
                     &WinStaObject);

   if (!NT_SUCCESS(Status))
   {
      SetLastNtError(Status);
      return(NULL);
   }

   Ret = (HWND)WinStaObject->ShellWindow;
   ObDereferenceObject(WinStaObject);
   return(GetWnd(Ret));
}

/*
 * NtUserSetShellWindowEx
 *
 * This is undocumented function to set global shell window. The global
 * shell window has special handling of window position.
 *
 * Status
 *    @implemented
 */

BOOL STDCALL
NtUserSetShellWindowEx(HWND hwndShell, HWND hwndListView)
{
   PWINSTATION_OBJECT WinStaObject;
   DECLARE_RETURN(BOOL);

   DPRINT("Enter NtUserSetShellWindowEx\n");
   UserEnterExclusive();

   NTSTATUS Status = IntValidateWindowStationHandle(PsGetCurrentProcess()->Win32WindowStation,
                     KernelMode,
                     0,
                     &WinStaObject);

   if (!NT_SUCCESS(Status))
   {
      SetLastNtError(Status);
      RETURN(FALSE);
   }

   /*
    * Test if we are permitted to change the shell window.
    */
   if (WinStaObject->ShellWindow)
   {
      ObDereferenceObject(WinStaObject);
      RETURN(FALSE);
   }

   /*
    * Move shell window into background.
    */
   if (hwndListView && hwndListView != hwndShell)
   {
      /*
       * Disabled for now to get Explorer working.
       * -- Filip, 01/nov/2003
       */
#if 0
      WinPosSetWindowPos(hwndListView, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);
#endif

      if (UserGetWindowLong(GetWnd(hwndListView), GWL_EXSTYLE, FALSE) & WS_EX_TOPMOST)
      {
         ObDereferenceObject(WinStaObject);
         RETURN(FALSE);
      }
   }

   if (UserGetWindowLong(GetWnd(hwndShell), GWL_EXSTYLE, FALSE) & WS_EX_TOPMOST)
   {
      ObDereferenceObject(WinStaObject);
      RETURN(FALSE);
   }

   co_WinPosSetWindowPos(hwndShell, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);

   WinStaObject->ShellWindow = hwndShell;
   WinStaObject->ShellListView = hwndListView;

   ObDereferenceObject(WinStaObject);
   RETURN(TRUE);

CLEANUP:
   DPRINT("Leave NtUserSetShellWindowEx, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}



/*
 * NtUserGetWindow
 *
 * The NtUserGetWindow function retrieves a handle to a window that has the
 * specified relationship (Z order or owner) to the specified window.
 *
 * Status
 *    @implemented
 */

HWND STDCALL
NtUserGetWindow(HWND hWnd, UINT Relationship)
{
   PWINDOW_OBJECT Wnd;
   DECLARE_RETURN(HWND);

   DPRINT("Enter NtUserGetWindow\n");
   UserEnterExclusive();

   if (!(Wnd = UserGetWindowObject(hWnd)))
   {
      SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
      RETURN(NULL);
   }

   RETURN(GetHwndSafe(UserGetWindow(Wnd, Relationship)));

CLEANUP:
   DPRINT("Leave NtUserGetWindow, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}




PWINDOW_OBJECT FASTCALL
UserGetWindow(PWINDOW_OBJECT Wnd, UINT Relationship)
{
   PWINDOW_OBJECT WndResult = NULL, Parent;

   ASSERT(Wnd);

   switch (Relationship)
   {
      case GW_HWNDFIRST:
      //FIXME: check if Wnd is desktop
         if((Parent = Wnd->ParentWnd))
         {
            if (Parent->FirstChild)
               WndResult = Parent->FirstChild;
         }
         break;

      case GW_HWNDLAST:
      //FIXME: check if Wnd is desktop
         if((Parent = Wnd->ParentWnd))
         {
            if (Parent->LastChild)
               WndResult = Parent->LastChild;
         }
         break;

      case GW_HWNDNEXT:
      //FIXME: check if Wnd is desktop
         if (Wnd->NextSibling)
            WndResult = Wnd->NextSibling;
         break;

      case GW_HWNDPREV:
      //FIXME: check if Wnd is desktop
         if (Wnd->PrevSibling)
            WndResult = Wnd->PrevSibling;
         break;

      case GW_OWNER:
      //FIXME: check if Wnd is desktop
         if((Parent = UserGetWindowObject(Wnd->hOwnerWnd)))
         {
            WndResult = Parent;
         }
         break;
      case GW_CHILD:
         if (Wnd->FirstChild)
            WndResult = Wnd->FirstChild;
         break;
   }

   return WndResult;
}



/*
 * NtUserGetWindowLong
 *
 * The NtUserGetWindowLong function retrieves information about the specified
 * window. The function also retrieves the 32-bit (long) value at the
 * specified offset into the extra window memory.
 *
 * Status
 *    @implemented
 */

LONG STDCALL
NtUserGetWindowLong(HWND hWnd, DWORD Index, BOOL Ansi)
{
   PWINDOW_OBJECT Wnd;
   DECLARE_RETURN(LONG);

   DPRINT("Enter NtUserGetWindowLong(%x,%d,%d)\n", hWnd, (INT)Index, Ansi);
   UserEnterExclusive();

   if (!(Wnd = UserGetWindowObject(hWnd)))
   {
      SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
      RETURN(0);
   }

   RETURN(UserGetWindowLong(Wnd, Index, Ansi));

CLEANUP:
   DPRINT("Leave NtUserGetWindowLong, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}





LONG FASTCALL
UserGetWindowLong(PWINDOW_OBJECT Wnd, DWORD Index, BOOL Ansi)
{
   PWINDOW_OBJECT Parent;
   LONG Result = 0;

   ASSERT(Wnd);

   /*
    * WndProc is only available to the owner process
    */
   if (GWL_WNDPROC == Index
         && Wnd->WThread->WProcess != PsGetWin32Process())
   {
      SetLastWin32Error(ERROR_ACCESS_DENIED);
      return(0);
   }

   if ((INT)Index >= 0)
   {
      if ((Index + sizeof(LONG)) > Wnd->ExtraDataSize)
      {
         SetLastWin32Error(ERROR_INVALID_PARAMETER);
         return(0);
      }
      Result = *((LONG *)(Wnd->ExtraData + Index));
   }
   else
   {
      switch (Index)
      {
         case GWL_EXSTYLE:
            Result = Wnd->ExStyle;
            break;

         case GWL_STYLE:
            Result = Wnd->Style;
            break;

         case GWL_WNDPROC:
            if (Ansi)
               Result = (LONG) Wnd->WndProcA;
            else
               Result = (LONG) Wnd->WndProcW;
            break;

         case GWL_HINSTANCE:
            Result = (LONG) Wnd->Instance;
            break;

         case GWL_HWNDPARENT:
            Parent = Wnd->ParentWnd;
            if(Parent)
            {
               if (Parent && Parent == UserGetDesktopWindow())
                  Result = (LONG) GetHwndSafe(UserGetWindow(Wnd, GW_OWNER));
               else
                  Result = (LONG) Parent->hSelf;
            }
            break;

         case GWL_ID:
            Result = (LONG) Wnd->IDMenu;
            break;

         case GWL_USERDATA:
            Result = Wnd->UserData;
            break;

         default:
            DPRINT1("UserGetWindowLong(): Unsupported index %d\n", Index);
            SetLastWin32Error(ERROR_INVALID_PARAMETER);
            Result = 0;
            break;
      }
   }

   return(Result);
}




/*
 * NtUserSetWindowLong
 *
 * The NtUserSetWindowLong function changes an attribute of the specified
 * window. The function also sets the 32-bit (long) value at the specified
 * offset into the extra window memory.
 *
 * Status
 *    @implemented
 */

LONG STDCALL
NtUserSetWindowLong(HWND hWnd, DWORD Index, LONG NewValue, BOOL Ansi)
{
   PWINDOW_OBJECT Wnd;
   DECLARE_RETURN(LONG);

   DPRINT("Enter NtUserSetWindowLong\n");
   UserEnterExclusive();

   if (!(Wnd = UserGetWindowObject(hWnd)))
   {
      SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
      RETURN(0);
   }

   RETURN(co_UserSetWindowLong(Wnd, Index, NewValue, Ansi));

CLEANUP:
   DPRINT("Leave NtUserSetWindowLong, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}




/*
 * NtUserSetWindowLong
 *
 * The NtUserSetWindowLong function changes an attribute of the specified
 * window. The function also sets the 32-bit (long) value at the specified
 * offset into the extra window memory.
 *
 * Status
 *    @implemented
 */

LONG FASTCALL
co_UserSetWindowLong(PWINDOW_OBJECT Wnd, DWORD Index, LONG NewValue, BOOL Ansi)
{
   PWINDOW_OBJECT WindowObject, Parent;
   PWINSTATION_OBJECT WindowStation;
   LONG OldValue;
   STYLESTRUCT Style;

   if (Wnd == UserGetDesktopWindow())
   {
      SetLastWin32Error(STATUS_ACCESS_DENIED);
      return(0);
   }

   if ((INT)Index >= 0)
   {
      if ((Index + sizeof(LONG)) > Wnd->ExtraDataSize)
      {
         SetLastWin32Error(ERROR_INVALID_PARAMETER);
         return(0);
      }
      OldValue = *((LONG *)(Wnd->ExtraData + Index));
      *((LONG *)(Wnd->ExtraData + Index)) = NewValue;
   }
   else
   {
      switch (Index)
      {
         case GWL_EXSTYLE:
            OldValue = (LONG) Wnd->ExStyle;
            Style.styleOld = OldValue;
            Style.styleNew = NewValue;

            /*
             * Remove extended window style bit WS_EX_TOPMOST for shell windows.
             */
            WindowStation = Wnd->WThread->Desktop->WinSta;
            if(WindowStation)
            {
               if (Wnd->hSelf == WindowStation->ShellWindow || Wnd->hSelf == WindowStation->ShellListView)
                  Style.styleNew &= ~WS_EX_TOPMOST;
            }

            co_UserSendMessage(Wnd->hSelf, WM_STYLECHANGING, GWL_EXSTYLE, (LPARAM) &Style);
            Wnd->ExStyle = (DWORD)Style.styleNew;
            co_UserSendMessage(Wnd->hSelf, WM_STYLECHANGED, GWL_EXSTYLE, (LPARAM) &Style);
            break;

         case GWL_STYLE:
            OldValue = (LONG) Wnd->Style;
            Style.styleOld = OldValue;
            Style.styleNew = NewValue;
            co_UserSendMessage(Wnd->hSelf, WM_STYLECHANGING, GWL_STYLE, (LPARAM) &Style);
            Wnd->Style = (DWORD)Style.styleNew;
            co_UserSendMessage(Wnd->hSelf, WM_STYLECHANGED, GWL_STYLE, (LPARAM) &Style);
            break;

         case GWL_WNDPROC:
            /* FIXME: should check if window belongs to current process */
            if (Ansi)
            {
               OldValue = (LONG) Wnd->WndProcA;
               Wnd->WndProcA = (WNDPROC) NewValue;
               Wnd->WndProcW = (WNDPROC) IntAddWndProcHandle((WNDPROC)NewValue,FALSE);
               Wnd->Unicode = FALSE;
            }
            else
            {
               OldValue = (LONG) Wnd->WndProcW;
               Wnd->WndProcW = (WNDPROC) NewValue;
               Wnd->WndProcA = (WNDPROC) IntAddWndProcHandle((WNDPROC)NewValue,TRUE);
               Wnd->Unicode = TRUE;
            }
            break;

         case GWL_HINSTANCE:
            OldValue = (LONG) Wnd->Instance;
            Wnd->Instance = (HINSTANCE) NewValue;
            break;

         case GWL_HWNDPARENT:
            Parent = Wnd->ParentWnd;
            if (Parent && (Parent == UserGetDesktopWindow()))
               OldValue = (LONG) GetHwndSafe((PWINDOW_OBJECT) UserSetOwner(Wnd, GetWnd((HWND) NewValue)));
            else
               OldValue = (LONG) co_UserSetParent(Wnd, GetWnd((HWND) NewValue));
            break;

         case GWL_ID:
            OldValue = (LONG) Wnd->IDMenu;
            WindowObject->IDMenu = (UINT) NewValue;
            break;

         case GWL_USERDATA:
            OldValue = Wnd->UserData;
            Wnd->UserData = NewValue;
            break;

         default:
            DPRINT1("NtUserSetWindowLong(): Unsupported index %d\n", Index);
            SetLastWin32Error(ERROR_INVALID_PARAMETER);
            OldValue = 0;
            break;
      }
   }

   return(OldValue);

}



/*
 * NtUserSetWindowWord
 *
 * Legacy function similar to NtUserSetWindowLong.
 *
 * Status
 *    @implemented
 */

WORD STDCALL
NtUserSetWindowWord(HWND hWnd, INT Index, WORD NewValue)
{
   PWINDOW_OBJECT WindowObject;
   WORD OldValue;
   DECLARE_RETURN(WORD);

   DPRINT("Enter NtUserSetWindowWord\n");
   UserEnterExclusive();

   switch (Index)
   {
      case GWL_ID:
      case GWL_HINSTANCE:
      case GWL_HWNDPARENT:
         RETURN(co_UserSetWindowLong(GetWnd(hWnd), Index, (UINT)NewValue, TRUE));
      default:
         if (Index < 0)
         {
            SetLastWin32Error(ERROR_INVALID_INDEX);
            RETURN(0);
         }
   }


   if (!(WindowObject = UserGetWindowObject(hWnd)))
   {
      SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
      RETURN(0);
   }

   if (Index > WindowObject->ExtraDataSize - sizeof(WORD))
   {
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      RETURN(0);
   }

   OldValue = *((WORD *)(WindowObject->ExtraData + Index));
   *((WORD *)(WindowObject->ExtraData + Index)) = NewValue;

   RETURN(OldValue);

CLEANUP:
   DPRINT("Leave NtUserSetWindowWord, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}

/*
 * @implemented
 */
BOOL STDCALL
NtUserGetWindowPlacement(HWND hWnd, WINDOWPLACEMENT *lpwndpl)
{
   PWINDOW_OBJECT Window;
   PINTERNALPOS InternalPos;
   POINT Size;
   WINDOWPLACEMENT Safepl;
   NTSTATUS Status;
   DECLARE_RETURN(BOOL);

   DPRINT("Enter NtUserGetWindowPlacement\n");
   UserEnterExclusive();

   if (!(Window = UserGetWindowObject(hWnd)))
   {
      SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
      RETURN(FALSE);
   }

   Status = MmCopyFromCaller(&Safepl, lpwndpl, sizeof(WINDOWPLACEMENT));
   if(!NT_SUCCESS(Status))
   {
      SetLastNtError(Status);
      RETURN(FALSE);
   }
   if(Safepl.length != sizeof(WINDOWPLACEMENT))
   {
      RETURN(FALSE);
   }

   Safepl.flags = 0;
   Safepl.showCmd = ((Window->Flags & WINDOWOBJECT_RESTOREMAX) ? SW_MAXIMIZE : SW_SHOWNORMAL);

   Size.x = Window->WindowRect.left;
   Size.y = Window->WindowRect.top;
   InternalPos = WinPosInitInternalPos(Window, &Size, &Window->WindowRect);
   if (InternalPos)
   {
      Safepl.rcNormalPosition = InternalPos->NormalRect;
      Safepl.ptMinPosition = InternalPos->IconPos;
      Safepl.ptMaxPosition = InternalPos->MaxPos;
   }
   else
   {
      RETURN(FALSE);
   }

   Status = MmCopyToCaller(lpwndpl, &Safepl, sizeof(WINDOWPLACEMENT));
   if(!NT_SUCCESS(Status))
   {
      SetLastNtError(Status);
      RETURN(FALSE);
   }

   RETURN(TRUE);

CLEANUP:
   DPRINT("Leave NtUserGetWindowPlacement, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}


/*!
 * Return the dimension of the window in the screen coordinates.
 * \param hWnd window handle.
 * \param Rect pointer to the buffer where the coordinates are returned.
*/
/*
 * @implemented
 */
BOOL STDCALL
NtUserGetWindowRect(HWND hWnd, LPRECT Rect)
{
   PWINDOW_OBJECT Wnd;
   NTSTATUS Status;
   DECLARE_RETURN(BOOL);

   DPRINT("Enter NtUserGetWindowRect\n");
   UserEnterExclusive();

   if (!(Wnd = UserGetWindowObject(hWnd)))
   {
      SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
      RETURN(FALSE);
   }
   Status = MmCopyToCaller(Rect, &Wnd->WindowRect, sizeof(RECT));
   if (!NT_SUCCESS(Status))
   {
      SetLastNtError(Status);
      RETURN(FALSE);
   }
   RETURN(TRUE);

CLEANUP:
   DPRINT("Leave NtUserGetWindowRect, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}


/*
 * @implemented
 */
DWORD STDCALL
NtUserGetWindowThreadProcessId(HWND hWnd, LPDWORD UnsafePid)
{
   PWINDOW_OBJECT Wnd;
   DWORD tid, pid;
   DECLARE_RETURN(DWORD);

   DPRINT("Enter NtUserGetWindowThreadProcessId\n");
   UserEnterExclusive();

   if (!(Wnd = UserGetWindowObject(hWnd)))
   {
      SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
      RETURN(0);
   }

   tid = (DWORD)IntGetWndThreadId(Wnd);
   pid = (DWORD)IntGetWndProcessId(Wnd);

   if (UnsafePid)
      MmCopyToCaller(UnsafePid, &pid, sizeof(DWORD));

   RETURN(tid);

CLEANUP:
   DPRINT("Leave NtUserGetWindowThreadProcessId, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}


/*
 * @unimplemented
 */
DWORD STDCALL
NtUserLockWindowUpdate(DWORD Unknown0)
{
   UNIMPLEMENTED

   return 0;
}


/*
 * @implemented
 */
BOOL STDCALL
NtUserMoveWindow(
   HWND hWnd,
   int X,
   int Y,
   int nWidth,
   int nHeight,
   BOOL bRepaint)
{
   return NtUserSetWindowPos(hWnd, 0, X, Y, nWidth, nHeight,
                             (bRepaint ? SWP_NOZORDER | SWP_NOACTIVATE :
                              SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOREDRAW));
}

/*
 QueryWindow based on KJK::Hyperion and James Tabor.
 
 0 = QWUniqueProcessId
 1 = QWUniqueThreadId
 4 = QWIsHung            Implements IsHungAppWindow found
                                by KJK::Hyperion.
 
        9 = QWKillWindow        When I called this with hWnd ==
                                DesktopWindow, it shutdown the system
                                and rebooted.
*/
/*
 * @implemented
 */
DWORD STDCALL
NtUserQueryWindow(HWND hWnd, DWORD Index)
{
   PWINDOW_OBJECT Window;
   DWORD Result;
   DECLARE_RETURN(DWORD);

   DPRINT("Enter NtUserQueryWindow\n");
   UserEnterExclusive();

   if (!(Window = UserGetWindowObject(hWnd)))
   {
      SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
      RETURN(0);
   }

   switch(Index)
   {
      case QUERY_WINDOW_UNIQUE_PROCESS_ID:
         Result = (DWORD)IntGetWndProcessId(Window);
         break;

      case QUERY_WINDOW_UNIQUE_THREAD_ID:
         Result = (DWORD)IntGetWndThreadId(Window);
         break;

      case QUERY_WINDOW_ISHUNG:
         Result = (DWORD)MsqIsHung(Window->Queue);
         break;

      default:
         Result = (DWORD)NULL;
         break;
   }

   RETURN(Result);

CLEANUP:
   DPRINT("Leave NtUserQueryWindow, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}


/*
 * @unimplemented
 */
DWORD STDCALL
NtUserRealChildWindowFromPoint(DWORD Unknown0,
                               DWORD Unknown1,
                               DWORD Unknown2)
{
   UNIMPLEMENTED

   return 0;
}


/*
 * @implemented
 */
UINT STDCALL
NtUserRegisterWindowMessage(PUNICODE_STRING MessageNameUnsafe)
{
   UNICODE_STRING SafeMessageName;
   NTSTATUS Status;
   UINT Ret;
   DECLARE_RETURN(UINT);

   DPRINT("Enter NtUserRegisterWindowMessage\n");
   UserEnterExclusive();

   if(MessageNameUnsafe == NULL)
   {
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      RETURN(0);
   }

   Status = IntSafeCopyUnicodeStringTerminateNULL(&SafeMessageName, MessageNameUnsafe);
   if(!NT_SUCCESS(Status))
   {
      SetLastNtError(Status);
      RETURN(0);
   }

   Ret = (UINT)IntAddAtom(SafeMessageName.Buffer);
   RtlFreeUnicodeString(&SafeMessageName);
   RETURN(Ret);

CLEANUP:
   DPRINT("Leave NtUserRegisterWindowMessage, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}


/*
 * @unimplemented
 */
DWORD STDCALL
NtUserSetImeOwnerWindow(DWORD Unknown0,
                        DWORD Unknown1)
{
   UNIMPLEMENTED

   return 0;
}


/*
 * @unimplemented
 */
DWORD STDCALL
NtUserSetInternalWindowPos(DWORD Unknown0,
                           DWORD Unknown1,
                           DWORD Unknown2,
                           DWORD Unknown3)
{
   UNIMPLEMENTED

   return 0;

}


/*
 * @unimplemented
 */
DWORD STDCALL
NtUserSetLayeredWindowAttributes(DWORD Unknown0,
                                 DWORD Unknown1,
                                 DWORD Unknown2,
                                 DWORD Unknown3)
{
   UNIMPLEMENTED

   return 0;
}


/*
 * @unimplemented
 */
DWORD STDCALL
NtUserSetLogonNotifyWindow(DWORD Unknown0)
{
   UNIMPLEMENTED

   return 0;
}





/*
 * @unimplemented
 */
DWORD STDCALL
NtUserSetWindowFNID(DWORD Unknown0,
                    DWORD Unknown1)
{
   UNIMPLEMENTED

   return 0;
}



/*
 * @implemented
 */
BOOL STDCALL
NtUserSetWindowPlacement(HWND hWnd,
                         WINDOWPLACEMENT *lpwndpl)
{
   PWINDOW_OBJECT Window;
   WINDOWPLACEMENT Safepl;
   NTSTATUS Status;
   DECLARE_RETURN(BOOL);

   DPRINT("Enter NtUserSetWindowPlacement\n");
   UserEnterExclusive();

   if (!(Window = UserGetWindowObject(hWnd)))
   {
      SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
      RETURN(FALSE);
   }
   Status = MmCopyFromCaller(&Safepl, lpwndpl, sizeof(WINDOWPLACEMENT));
   if(!NT_SUCCESS(Status))
   {
      SetLastNtError(Status);
      RETURN(FALSE);
   }
   if(Safepl.length != sizeof(WINDOWPLACEMENT))
   {
      RETURN(FALSE);
   }

   if ((Window->Style & (WS_MAXIMIZE | WS_MINIMIZE)) == 0)
   {
      co_WinPosSetWindowPos(Window->hSelf, NULL,
                            Safepl.rcNormalPosition.left, Safepl.rcNormalPosition.top,
                            Safepl.rcNormalPosition.right - Safepl.rcNormalPosition.left,
                            Safepl.rcNormalPosition.bottom - Safepl.rcNormalPosition.top,
                            SWP_NOZORDER | SWP_NOACTIVATE);
   }

   /* FIXME - change window status */
   co_WinPosShowWindow(Window, Safepl.showCmd);

   if (Window->InternalPos == NULL)
      Window->InternalPos = ExAllocatePoolWithTag(PagedPool, sizeof(INTERNALPOS), TAG_WININTLIST);
   Window->InternalPos->NormalRect = Safepl.rcNormalPosition;
   Window->InternalPos->IconPos = Safepl.ptMinPosition;
   Window->InternalPos->MaxPos = Safepl.ptMaxPosition;

   RETURN(TRUE);

CLEANUP:
   DPRINT("Leave NtUserSetWindowPlacement, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}


/*
 * @implemented
 */
BOOL STDCALL
NtUserSetWindowPos(
   HWND hWnd,
   HWND hWndInsertAfter,
   int X,
   int Y,
   int cx,
   int cy,
   UINT uFlags)
{
   DECLARE_RETURN(BOOL);

   DPRINT("Enter NtUserSetWindowPos\n");
   UserEnterExclusive();

   RETURN(co_WinPosSetWindowPos(hWnd, hWndInsertAfter, X, Y, cx, cy, uFlags));

CLEANUP:
   DPRINT("Leave NtUserSetWindowPos, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}


INT FASTCALL
IntGetWindowRgn(HWND hWnd, HRGN hRgn)
{
   INT Ret;
   PWINDOW_OBJECT WindowObject;
   HRGN VisRgn;
   ROSRGNDATA *pRgn;

   if(!(WindowObject = UserGetWindowObject(hWnd)))
   {
      SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
      return ERROR;
   }
   if(!hRgn)
   {
      return ERROR;
   }

   /* Create a new window region using the window rectangle */
   VisRgn = UnsafeIntCreateRectRgnIndirect(&WindowObject->WindowRect);
   NtGdiOffsetRgn(VisRgn, -WindowObject->WindowRect.left, -WindowObject->WindowRect.top);
   /* if there's a region assigned to the window, combine them both */
   if(WindowObject->WindowRegion && !(WindowObject->Style & WS_MINIMIZE))
      NtGdiCombineRgn(VisRgn, VisRgn, WindowObject->WindowRegion, RGN_AND);
   /* Copy the region into hRgn */
   NtGdiCombineRgn(hRgn, VisRgn, NULL, RGN_COPY);

   if((pRgn = RGNDATA_LockRgn(hRgn)))
   {
      Ret = pRgn->rdh.iType;
      RGNDATA_UnlockRgn(pRgn);
   }
   else
      Ret = ERROR;

   NtGdiDeleteObject(VisRgn);

   return Ret;
}

INT FASTCALL
IntGetWindowRgnBox(HWND hWnd, RECT *Rect)
{
   INT Ret;
   PWINDOW_OBJECT WindowObject;
   HRGN VisRgn;
   ROSRGNDATA *pRgn;

   if(!(WindowObject = UserGetWindowObject(hWnd)))
   {
      SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
      return ERROR;
   }
   if(!Rect)
   {
      return ERROR;
   }

   /* Create a new window region using the window rectangle */
   VisRgn = UnsafeIntCreateRectRgnIndirect(&WindowObject->WindowRect);
   NtGdiOffsetRgn(VisRgn, -WindowObject->WindowRect.left, -WindowObject->WindowRect.top);
   /* if there's a region assigned to the window, combine them both */
   if(WindowObject->WindowRegion && !(WindowObject->Style & WS_MINIMIZE))
      NtGdiCombineRgn(VisRgn, VisRgn, WindowObject->WindowRegion, RGN_AND);

   if((pRgn = RGNDATA_LockRgn(VisRgn)))
   {
      Ret = pRgn->rdh.iType;
      *Rect = pRgn->rdh.rcBound;
      RGNDATA_UnlockRgn(pRgn);
   }
   else
      Ret = ERROR;

   NtGdiDeleteObject(VisRgn);

   return Ret;
}


/*
 * @implemented
 */
INT STDCALL
NtUserSetWindowRgn(
   HWND hWnd,
   HRGN hRgn,
   BOOL bRedraw)
{
   PWINDOW_OBJECT Window;
   DECLARE_RETURN(INT);

   DPRINT("Enter NtUserSetWindowRgn\n");
   UserEnterExclusive();


   if (!(Window = UserGetWindowObject(hWnd)))
   {
      SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
      RETURN(0);
   }

   /* FIXME - Verify if hRgn is a valid handle!!!!
              Propably make this operation thread-safe, but maybe it's not necessary */

   if(Window->WindowRegion)
   {
      /* Delete no longer needed region handle */
      NtGdiDeleteObject(Window->WindowRegion);
   }
   Window->WindowRegion = hRgn;

   /* FIXME - send WM_WINDOWPOSCHANGING and WM_WINDOWPOSCHANGED messages to the window */

   if(bRedraw)
   {
      co_UserRedrawWindow(Window, NULL, NULL, RDW_INVALIDATE);
   }

   RETURN((INT)hRgn);

CLEANUP:
   DPRINT("Leave NtUserSetWindowRgn, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}


/*
 * @implemented
 */
BOOL STDCALL
NtUserShowWindow(HWND hWnd, LONG nCmdShow)
{
   DECLARE_RETURN(BOOL);
   PWINDOW_OBJECT Window;

   DPRINT("Enter NtUserShowWindow\n");
   UserEnterExclusive();

   if (!(Window = UserGetWindowObject(hWnd)))
   {
      SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
      RETURN (FALSE);
   }

   RETURN(co_WinPosShowWindow(Window, nCmdShow));

CLEANUP:
   DPRINT("Leave NtUserShowWindow, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}


/*
 * @unimplemented
 */
DWORD STDCALL
NtUserShowWindowAsync(DWORD Unknown0,
                      DWORD Unknown1)
{
   UNIMPLEMENTED

   return 0;
}


/*
 * @unimplemented
 */
DWORD STDCALL
NtUserUpdateLayeredWindow(DWORD Unknown0,
                          DWORD Unknown1,
                          DWORD Unknown2,
                          DWORD Unknown3,
                          DWORD Unknown4,
                          DWORD Unknown5,
                          DWORD Unknown6,
                          DWORD Unknown7,
                          DWORD Unknown8)
{
   UNIMPLEMENTED

   return 0;
}


/*
 * @implemented
 */
VOID STDCALL
NtUserValidateRect(HWND hWnd, const RECT* Rect)
{
   return (VOID)NtUserRedrawWindow(hWnd, Rect, 0, RDW_VALIDATE | RDW_NOCHILDREN);
}


/*
 *    @implemented
 */
HWND STDCALL
NtUserWindowFromPoint(LONG X, LONG Y)
{
   POINT pt;
   PWINDOW_OBJECT DesktopWindow, Window = NULL;
   DECLARE_RETURN(HWND);

   DPRINT("Enter NtUserWindowFromPoint\n");
   UserEnterExclusive();

   if ((DesktopWindow = UserGetDesktopWindow()))
   {
      USHORT Hit;

      pt.x = X;
      pt.y = Y;

      Hit = co_WinPosWindowFromPoint(DesktopWindow, &PsGetWin32Thread()->Queue, &pt, &Window);
      //FIX: check retval
      
      RETURN(GetHwndSafe(Window));
   }

   RETURN(NULL);

CLEANUP:
   DPRINT("Leave NtUserWindowFromPoint, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}


/*
 * NtUserDefSetText
 *
 * Undocumented function that is called from DefWindowProc to set
 * window text.
 *
 * Status
 *    @implemented
 */

BOOL STDCALL
NtUserDefSetText(HWND hWnd, PUNICODE_STRING WindowText)
{
   PWINDOW_OBJECT Window, Parent, Owner;
   UNICODE_STRING SafeText;
   NTSTATUS Status;
   DECLARE_RETURN(BOOL);

   DPRINT("Enter NtUserDefSetText\n");
   UserEnterExclusive();


   if(!(Window = UserGetWindowObject(hWnd)))
   {
      SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
      RETURN(FALSE);
   }

   if(WindowText)
   {
      Status = IntSafeCopyUnicodeString(&SafeText, WindowText);
      if(!NT_SUCCESS(Status))
      {
         SetLastNtError(Status);
         RETURN(FALSE);
      }
   }
   else
   {
      RtlInitUnicodeString(&SafeText, NULL);
   }

   /* FIXME - do this thread-safe! otherwise one could crash here! */
   RtlFreeUnicodeString(&Window->WindowName);

   Window->WindowName = SafeText;

   /* Send shell notifications */

   Owner = UserGetOwner(Window);
   Parent = UserGetParent(Window);

   if (!Owner && !Parent)
   {
      co_UserShellHookNotify(HSHELL_REDRAW, (LPARAM) hWnd);
   }

   RETURN(TRUE);

CLEANUP:
   DPRINT("Leave NtUserDefSetText, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}

/*
 * NtUserInternalGetWindowText
 *
 * Status
 *    @implemented
 */

INT STDCALL
NtUserInternalGetWindowText(HWND hWnd, LPWSTR lpString, INT nMaxCount)
{
   PWINDOW_OBJECT Window;
   NTSTATUS Status;
   INT Result;
   DECLARE_RETURN(INT);

   DPRINT("Enter NtUserInternalGetWindowText\n");
   UserEnterExclusive();

   if(lpString && (nMaxCount <= 1))
   {
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      RETURN(0);
   }


   if(!(Window = UserGetWindowObject(hWnd)))
   {
      SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
      RETURN(0);
   }

   /* FIXME - do this thread-safe! otherwise one could crash here! */
   Result = Window->WindowName.Length / sizeof(WCHAR);
   if(lpString)
   {
      const WCHAR Terminator = L'\0';
      INT Copy;
      WCHAR *Buffer = (WCHAR*)lpString;

      Copy = min(nMaxCount - 1, Result);
      if(Copy > 0)
      {
         Status = MmCopyToCaller(Buffer, Window->WindowName.Buffer, Copy * sizeof(WCHAR));
         if(!NT_SUCCESS(Status))
         {
            SetLastNtError(Status);
            RETURN(0);
         }
         Buffer += Copy;
      }

      Status = MmCopyToCaller(Buffer, &Terminator, sizeof(WCHAR));
      if(!NT_SUCCESS(Status))
      {
         SetLastNtError(Status);
         RETURN(0);
      }

      Result = Copy;
   }

   RETURN(Result);

CLEANUP:
   DPRINT("Leave NtUserInternalGetWindowText, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}


DWORD STDCALL
NtUserDereferenceWndProcHandle(WNDPROC wpHandle, WndProcHandle *Data)
{
   DECLARE_RETURN(DWORD);

   DPRINT("Enter NtUserDereferenceWndProcHandle\n");
   UserEnterExclusive();

   WndProcHandle Entry;
   if (((DWORD)wpHandle & 0xFFFF0000) == 0xFFFF0000)
   {
      Entry = gWndProcHandlesArray[(DWORD)wpHandle & 0x0000FFFF];
      Data->WindowProc = Entry.WindowProc;
      Data->IsUnicode = Entry.IsUnicode;
      Data->ProcessID = Entry.ProcessID;
      RETURN(TRUE);
   }
   else
   {
      RETURN(FALSE);
   }

   RETURN(FALSE);

CLEANUP:
   DPRINT("Leave NtUserDereferenceWndProcHandle, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}

DWORD
IntAddWndProcHandle(WNDPROC WindowProc, BOOL IsUnicode)
{
   WORD i;
   WORD FreeSpot = 0;
   BOOL found;
   WndProcHandle *OldArray;
   WORD OldArraySize;
   found = FALSE;
   for (i = 0;i < gWndProcHandlesArraySize;i++)
   {
      if (gWndProcHandlesArray[i].WindowProc == NULL)
      {
         FreeSpot = i;
         found = TRUE;
      }
   }
   if (!found)
   {
      OldArray = gWndProcHandlesArray;
      OldArraySize = gWndProcHandlesArraySize;
      gWndProcHandlesArray = ExAllocatePoolWithTag(PagedPool,(OldArraySize + WPH_SIZE) * sizeof(WndProcHandle), TAG_WINPROCLST);
      gWndProcHandlesArraySize = OldArraySize + WPH_SIZE;
      RtlCopyMemory(gWndProcHandlesArray,OldArray,OldArraySize * sizeof(WndProcHandle));
      ExFreePool(OldArray);
      FreeSpot = OldArraySize + 1;
   }
   gWndProcHandlesArray[FreeSpot].WindowProc = WindowProc;
   gWndProcHandlesArray[FreeSpot].IsUnicode = IsUnicode;
   gWndProcHandlesArray[FreeSpot].ProcessID = PsGetCurrentProcessId();
   return FreeSpot + 0xFFFF0000;
}

DWORD
IntRemoveWndProcHandle(WNDPROC Handle)
{
   WORD position;
   position = (DWORD)Handle & 0x0000FFFF;
   if (position > gWndProcHandlesArraySize)
   {
      return FALSE;
   }
   gWndProcHandlesArray[position].WindowProc = NULL;
   gWndProcHandlesArray[position].IsUnicode = FALSE;
   gWndProcHandlesArray[position].ProcessID = NULL;
   return TRUE;
}

DWORD
IntRemoveProcessWndProcHandles(HANDLE ProcessID)
{
   WORD i;
   for (i = 0;i < gWndProcHandlesArraySize;i++)
   {
      if (gWndProcHandlesArray[i].ProcessID == ProcessID)
      {
         gWndProcHandlesArray[i].WindowProc = NULL;
         gWndProcHandlesArray[i].IsUnicode = FALSE;
         gWndProcHandlesArray[i].ProcessID = NULL;
      }
   }
   return TRUE;
}

#define WIN_NEEDS_SHOW_OWNEDPOPUP (0x00000040)

BOOL
FASTCALL
co_UserShowOwnedPopups( HWND hOwner, BOOL fShow )
{
   int count = 0;
   PWINDOW_OBJECT Owner, pWnd;
   HWND *win_array;

   if(!(Owner = UserGetWindowObject(hOwner)))
   {
      SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
      return FALSE;
   }

//   win_array = IntWinListChildren(Owner);
   win_array = IntWinListChildren(UserGetDesktopWindow());

   if (!win_array)
      return TRUE;

   while (win_array[count])
      count++;
      
   while (--count >= 0)
   {
      if (!(pWnd = UserGetWindowObject( win_array[count] )))
         continue;
        
      if (UserGetWindow(pWnd, GW_OWNER ) != Owner)
         continue;

//      if (pWnd == WND_OTHER_PROCESS) continue;


      if (fShow)
      {
         if (pWnd->Flags & WIN_NEEDS_SHOW_OWNEDPOPUP)
         {
            /* In Windows, ShowOwnedPopups(TRUE) generates
             * WM_SHOWWINDOW messages with SW_PARENTOPENING,
             * regardless of the state of the owner
             */
            co_UserSendMessage(win_array[count], WM_SHOWWINDOW, SW_SHOWNORMAL, SW_PARENTOPENING);
            continue;
         }
      }
      else
      {
         if (pWnd->Style & WS_VISIBLE)
         {
            /* In Windows, ShowOwnedPopups(FALSE) generates
             * WM_SHOWWINDOW messages with SW_PARENTCLOSING,
             * regardless of the state of the owner
             */
            co_UserSendMessage(win_array[count], WM_SHOWWINDOW, SW_HIDE, SW_PARENTCLOSING);
            continue;
         }
      }

   }
   ExFreePool( win_array );
   return TRUE;
}


/* EOF */
