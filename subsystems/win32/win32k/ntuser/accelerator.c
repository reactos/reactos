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
/* $Id$
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Window classes
 * FILE:             subsys/win32k/ntuser/class.c
 * PROGRAMER:        Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISION HISTORY:
 *       06-06-2001  CSH  Created
 */

/*
 * Copyright 1993 Martin Ayotte
 * Copyright 1994 Alexandre Julliard
 * Copyright 1997 Morten Welinder
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* INCLUDES ******************************************************************/

#include <w32k.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

NTSTATUS FASTCALL
InitAcceleratorImpl(VOID)
{
   return(STATUS_SUCCESS);
}

NTSTATUS FASTCALL
CleanupAcceleratorImpl(VOID)
{
   return(STATUS_SUCCESS);
}


PACCELERATOR_TABLE FASTCALL UserGetAccelObject(HACCEL hAccel)
{
   PACCELERATOR_TABLE Accel;

   if (!hAccel)
   {
      SetLastWin32Error(ERROR_INVALID_ACCEL_HANDLE);
      return NULL;
   }

   Accel= UserGetObject(gHandleTable, hAccel,  otAccel);
   if (!Accel)
   {
      SetLastWin32Error(ERROR_INVALID_ACCEL_HANDLE);
      return NULL;
   }

   ASSERT(USER_BODY_TO_HEADER(Accel)->RefCount >= 0);

   return Accel;
}


static
BOOLEAN FASTCALL
co_IntTranslateAccelerator(
   PWINDOW_OBJECT Window,
   UINT message,
   WPARAM wParam,
   LPARAM lParam,
   BYTE fVirt,
   WORD key,
   WORD cmd)
{
   UINT mesg = 0;

   ASSERT_REFS_CO(Window);

   DPRINT("IntTranslateAccelerator(hwnd %x, message %x, wParam %x, lParam %x, fVirt %d, key %x, cmd %x)\n",
          Window->hSelf, message, wParam, lParam, fVirt, key, cmd);

   if (wParam != key)
   {
      return FALSE;
   }

   if (message == WM_CHAR)
   {
      if (!(fVirt & FALT) && !(fVirt & FVIRTKEY))
      {
         DPRINT("found accel for WM_CHAR: ('%c')\n", wParam & 0xff);
         goto found;
      }
   }
   else
   {
      if ((fVirt & FVIRTKEY) > 0)
      {
         INT mask = 0;
         DPRINT("found accel for virt_key %04x (scan %04x)\n",
                wParam, 0xff & HIWORD(lParam));

         DPRINT("NtUserGetKeyState(VK_SHIFT) = 0x%x\n",
                UserGetKeyState(VK_SHIFT));
         DPRINT("NtUserGetKeyState(VK_CONTROL) = 0x%x\n",
                UserGetKeyState(VK_CONTROL));
         DPRINT("NtUserGetKeyState(VK_MENU) = 0x%x\n",
                UserGetKeyState(VK_MENU));

         if (UserGetKeyState(VK_SHIFT) & 0x8000)
            mask |= FSHIFT;
         if (UserGetKeyState(VK_CONTROL) & 0x8000)
            mask |= FCONTROL;
         if (UserGetKeyState(VK_MENU) & 0x8000)
            mask |= FALT;
         if (mask == (fVirt & (FSHIFT | FCONTROL | FALT)))
            goto found;
         DPRINT("but incorrect SHIFT/CTRL/ALT-state\n");
      }
      else
      {
         if (!(lParam & 0x01000000))  /* no special_key */
         {
            if ((fVirt & FALT) && (lParam & 0x20000000))
            {                            /* ^^ ALT pressed */
               DPRINT("found accel for Alt-%c\n", wParam & 0xff);
               goto found;
            }
         }
      }
   }

   DPRINT("IntTranslateAccelerator(hwnd %x, message %x, wParam %x, lParam %x, fVirt %d, key %x, cmd %x) = FALSE\n",
          Window->hSelf, message, wParam, lParam, fVirt, key, cmd);

   return FALSE;

found:
   if (message == WM_KEYUP || message == WM_SYSKEYUP)
      mesg = 1;
   else if (IntGetCaptureWindow())
      mesg = 2;
   else if (!IntIsWindowVisible(Window)) /* FIXME: WINE IsWindowEnabled == IntIsWindowVisible? */
      mesg = 3;
   else
   {
#if 0
      HMENU hMenu, hSubMenu, hSysMenu;
      UINT uSysStat = (UINT)-1, uStat = (UINT)-1, nPos;

      hMenu = (UserGetWindowLongW(hWnd, GWL_STYLE) & WS_CHILD) ? 0 : GetMenu(hWnd);
      hSysMenu = get_win_sys_menu(hWnd);

      /* find menu item and ask application to initialize it */
      /* 1. in the system menu */
      hSubMenu = hSysMenu;
      nPos = cmd;
      if(MENU_FindItem(&hSubMenu, &nPos, MF_BYCOMMAND))
      {
         co_IntSendMessage(hWnd, WM_INITMENU, (WPARAM)hSysMenu, 0L);
         if(hSubMenu != hSysMenu)
         {
            nPos = MENU_FindSubMenu(&hSysMenu, hSubMenu);
            TRACE_(accel)("hSysMenu = %p, hSubMenu = %p, nPos = %d\n", hSysMenu, hSubMenu, nPos);
            co_IntSendMessage(hWnd, WM_INITMENUPOPUP, (WPARAM)hSubMenu, MAKELPARAM(nPos, TRUE));
         }
         uSysStat = GetMenuState(GetSubMenu(hSysMenu, 0), cmd, MF_BYCOMMAND);
      }
      else /* 2. in the window's menu */
      {
         hSubMenu = hMenu;
         nPos = cmd;
         if(MENU_FindItem(&hSubMenu, &nPos, MF_BYCOMMAND))
         {
            co_IntSendMessage(hWnd, WM_INITMENU, (WPARAM)hMenu, 0L);
            if(hSubMenu != hMenu)
            {
               nPos = MENU_FindSubMenu(&hMenu, hSubMenu);
               TRACE_(accel)("hMenu = %p, hSubMenu = %p, nPos = %d\n", hMenu, hSubMenu, nPos);
               co_IntSendMessage(hWnd, WM_INITMENUPOPUP, (WPARAM)hSubMenu, MAKELPARAM(nPos, FALSE));
            }
            uStat = GetMenuState(hMenu, cmd, MF_BYCOMMAND);
         }
      }

      if (uSysStat != (UINT)-1)
      {
         if (uSysStat & (MF_DISABLED|MF_GRAYED))
            mesg=4;
         else
            mesg=WM_SYSCOMMAND;
      }
      else
      {
         if (uStat != (UINT)-1)
         {
            if (IsIconic(hWnd))
               mesg=5;
            else
            {
               if (uStat & (MF_DISABLED|MF_GRAYED))
                  mesg=6;
               else
                  mesg=WM_COMMAND;
            }
         }
         else
         {
            mesg=WM_COMMAND;
         }
      }
#else
      DPRINT1("Menu search not implemented\n");
      mesg = WM_COMMAND;
#endif

   }

   if (mesg == WM_COMMAND)
   {
      DPRINT(", sending WM_COMMAND, wParam=%0x\n", 0x10000 | cmd);
      co_IntSendMessage(Window->hSelf, mesg, 0x10000 | cmd, 0L);
   }
   else if (mesg == WM_SYSCOMMAND)
   {
      DPRINT(", sending WM_SYSCOMMAND, wParam=%0x\n", cmd);
      co_IntSendMessage(Window->hSelf, mesg, cmd, 0x00010000L);
   }
   else
   {
      /*  some reasons for NOT sending the WM_{SYS}COMMAND message:
       *   #0: unknown (please report!)
       *   #1: for WM_KEYUP,WM_SYSKEYUP
       *   #2: mouse is captured
       *   #3: window is disabled
       *   #4: it's a disabled system menu option
       *   #5: it's a menu option, but window is iconic
       *   #6: it's a menu option, but disabled
       */
      DPRINT(", but won't send WM_{SYS}COMMAND, reason is #%d\n", mesg);
      if (mesg == 0)
      {
         DPRINT1(" unknown reason - please report!");
      }
   }

   DPRINT("IntTranslateAccelerator(hWnd %x, message %x, wParam %x, lParam %x, fVirt %d, key %x, cmd %x) = TRUE\n",
          Window->hSelf, message, wParam, lParam, fVirt, key, cmd);

   return TRUE;
}


/* SYSCALLS *****************************************************************/


int
STDCALL
NtUserCopyAcceleratorTable(
   HACCEL hAccel,
   LPACCEL Entries,
   int EntriesCount)
{
   PACCELERATOR_TABLE Accel;
   NTSTATUS Status;
   int Ret;
   DECLARE_RETURN(int);

   DPRINT("Enter NtUserCopyAcceleratorTable\n");
   UserEnterShared();

   if (!(Accel = UserGetAccelObject(hAccel)))
   {
      RETURN(0);
   }

   if(Entries)
   {
      Ret = min(EntriesCount, Accel->Count);
      Status = MmCopyToCaller(Entries, Accel->Table, Ret * sizeof(ACCEL));
      if (!NT_SUCCESS(Status))
      {
         SetLastNtError(Status);
         RETURN(0);
      }
   }
   else
   {
      Ret = Accel->Count;
   }

   RETURN(Ret);

CLEANUP:
   DPRINT("Leave NtUserCopyAcceleratorTable, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}

HACCEL
STDCALL
NtUserCreateAcceleratorTable(
   LPACCEL Entries,
   SIZE_T EntriesCount)
{
   PACCELERATOR_TABLE Accel;
   NTSTATUS Status;
   HACCEL hAccel;
   DECLARE_RETURN(HACCEL);

   DPRINT("Enter NtUserCreateAcceleratorTable(Entries %p, EntriesCount %d)\n",
          Entries, EntriesCount);
   UserEnterExclusive();

   if (!Entries || !EntriesCount)
   {
      RETURN( (HACCEL) 0 );
   }

   Accel = UserCreateObject(gHandleTable, (PHANDLE)&hAccel, otAccel, sizeof(ACCELERATOR_TABLE));

   if (Accel == NULL)
   {
      SetLastNtError(STATUS_NO_MEMORY);
      RETURN( (HACCEL) 0 );
   }

   Accel->Count = EntriesCount;
   if (Accel->Count > 0)
   {
      Accel->Table = ExAllocatePoolWithTag(PagedPool, EntriesCount * sizeof(ACCEL), TAG_ACCEL);
      if (Accel->Table == NULL)
      {
         UserDereferenceObject(Accel);
         UserDeleteObject(hAccel, otAccel);
         SetLastNtError(STATUS_NO_MEMORY);
         RETURN( (HACCEL) 0);
      }

      Status = MmCopyFromCaller(Accel->Table, Entries, EntriesCount * sizeof(ACCEL));
      if (!NT_SUCCESS(Status))
      {
         ExFreePool(Accel->Table);
         UserDereferenceObject(Accel);
         UserDeleteObject(hAccel, otAccel);
         SetLastNtError(Status);
         RETURN((HACCEL) 0);
      }
   }

   /* FIXME: Save HandleTable in a list somewhere so we can clean it up again */

   RETURN(hAccel);

CLEANUP:
   DPRINT("Leave NtUserCreateAcceleratorTable(Entries %p, EntriesCount %d) = %x\n",
          Entries, EntriesCount,_ret_);
   UserLeave();
   END_CLEANUP;
}



BOOLEAN
STDCALL
NtUserDestroyAcceleratorTable(
   HACCEL hAccel)
{
   PACCELERATOR_TABLE Accel;
   DECLARE_RETURN(BOOLEAN);

   /* FIXME: If the handle table is from a call to LoadAcceleratorTable, decrement it's
      usage count (and return TRUE).
   FIXME: Destroy only tables created using CreateAcceleratorTable.
    */

   DPRINT("NtUserDestroyAcceleratorTable(Table %x)\n", hAccel);
   UserEnterExclusive();

   if (!(Accel = UserGetAccelObject(hAccel)))
   {
      RETURN( FALSE);
   }

   if (Accel->Table != NULL)
   {
      ExFreePool(Accel->Table);
      Accel->Table = NULL;
   }

   UserDeleteObject(hAccel, otAccel);

   RETURN( TRUE);

CLEANUP:
   DPRINT("Leave NtUserDestroyAcceleratorTable(Table %x) = %i\n", hAccel,_ret_);
   UserLeave();
   END_CLEANUP;
}


int
STDCALL
NtUserTranslateAccelerator(
   HWND hWnd,
   HACCEL hAccel,
   LPMSG Message)
{
   PWINDOW_OBJECT Window = NULL;
   PACCELERATOR_TABLE Accel = NULL;
   ULONG i;
   USER_REFERENCE_ENTRY AccelRef, WindowRef;
   DECLARE_RETURN(int);

   DPRINT("NtUserTranslateAccelerator(hWnd %x, Table %x, Message %p)\n",
          hWnd, hAccel, Message);
   UserEnterShared();

   if (Message == NULL)
   {
      SetLastNtError(STATUS_INVALID_PARAMETER);
      RETURN( 0);
   }

   if ((Message->message != WM_KEYDOWN) &&
         (Message->message != WM_SYSKEYDOWN) &&
         (Message->message != WM_SYSCHAR) &&
         (Message->message != WM_CHAR))
   {
      RETURN( 0);
   }

   if (!(Accel = UserGetAccelObject(hAccel)))
   {
      RETURN( 0);
   }

   UserRefObjectCo(Accel, &AccelRef);

   if (!(Window = UserGetWindowObject(hWnd)))
   {
      RETURN( 0);
   }

   UserRefObjectCo(Window, &WindowRef);


   /* FIXME: Associate AcceleratorTable with the current thread */

   for (i = 0; i < Accel->Count; i++)
   {
      if (co_IntTranslateAccelerator(Window, Message->message, Message->wParam, Message->lParam,
                                     Accel->Table[i].fVirt, Accel->Table[i].key,
                                     Accel->Table[i].cmd))
      {
         DPRINT("NtUserTranslateAccelerator(hWnd %x, Table %x, Message %p) = %i end\n",
                hWnd, hAccel, Message, 1);
         RETURN( 1);
      }
      if (((Accel->Table[i].fVirt & 0x80) > 0))
      {
         break;
      }
   }

   RETURN( 0);

CLEANUP:
   if (Window) UserDerefObjectCo(Window);
   if (Accel) UserDerefObjectCo(Accel);

   DPRINT("NtUserTranslateAccelerator(hWnd %x, Table %x, Message %p) = %i end\n",
          hWnd, hAccel, Message, 0);
   UserLeave();
   END_CLEANUP;
}
