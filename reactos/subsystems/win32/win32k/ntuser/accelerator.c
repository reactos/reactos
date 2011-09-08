/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Window accelerator
 * FILE:             subsystems/win32/win32k/ntuser/accelerator.c
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>

DBG_DEFAULT_CHANNEL(UserAccel);

UINT FASTCALL IntFindSubMenu(HMENU *hMenu, HMENU hSubTarget );
HMENU FASTCALL IntGetSubMenu( HMENU hMenu, int nPos);
UINT FASTCALL IntGetMenuState( HMENU hMenu, UINT uId, UINT uFlags);

/* FUNCTIONS *****************************************************************/

PACCELERATOR_TABLE FASTCALL UserGetAccelObject(HACCEL hAccel)
{
   PACCELERATOR_TABLE Accel;

   if (!hAccel)
   {
      EngSetLastError(ERROR_INVALID_ACCEL_HANDLE);
      return NULL;
   }

   Accel= UserGetObject(gHandleTable, hAccel,  otAccel);
   if (!Accel)
   {
      EngSetLastError(ERROR_INVALID_ACCEL_HANDLE);
      return NULL;
   }

   return Accel;
}


static
BOOLEAN FASTCALL
co_IntTranslateAccelerator(
   PWND Window,
   UINT message,
   WPARAM wParam,
   LPARAM lParam,
   BYTE fVirt,
   WORD key,
   WORD cmd)
{
   INT mask = 0;
   UINT mesg = 0;
   HWND hWnd;

   ASSERT_REFS_CO(Window);

   hWnd = Window->head.h;

   TRACE("IntTranslateAccelerator(hwnd %x, message %x, wParam %x, lParam %x, fVirt %d, key %x, cmd %x)\n",
          Window->head.h, message, wParam, lParam, fVirt, key, cmd);

   if (wParam != key)
   {
      return FALSE;
   }

   TRACE("NtUserGetKeyState(VK_CONTROL) = 0x%x\n",UserGetKeyState(VK_CONTROL));
   TRACE("NtUserGetKeyState(VK_MENU) = 0x%x\n",UserGetKeyState(VK_MENU));
   TRACE("NtUserGetKeyState(VK_SHIFT) = 0x%x\n",UserGetKeyState(VK_SHIFT));

   if (UserGetKeyState(VK_CONTROL) & 0x8000) mask |= FCONTROL;
   if (UserGetKeyState(VK_MENU) & 0x8000) mask |= FALT;
   if (UserGetKeyState(VK_SHIFT) & 0x8000) mask |= FSHIFT;
   TRACE("Mask 0x%x\n",mask);

   if (message == WM_CHAR || message == WM_SYSCHAR)
   {
      if ( !(fVirt & FVIRTKEY) && (mask & FALT) == (fVirt & FALT) )
      {
         TRACE("found accel for WM_CHAR: ('%c')\n", LOWORD(wParam) & 0xff);
         goto found;
      }
   }
   else
   {
      if (fVirt & FVIRTKEY)
      {
         TRACE("found accel for virt_key %04x (scan %04x)\n",
                 wParam, 0xff & HIWORD(lParam));

         if (mask == (fVirt & (FSHIFT | FCONTROL | FALT))) goto found;
         TRACE("but incorrect SHIFT/CTRL/ALT-state mask %x fVirt %x\n",mask,fVirt);
      }
      else
      {
         if (!(lParam & 0x01000000))  /* no special_key */
         {
            if ((fVirt & FALT) && (lParam & 0x20000000))
            {                            /* ^^ ALT pressed */
               TRACE("found accel for Alt-%c\n", LOWORD(wParam) & 0xff);
               goto found;
            }
         }
      }
   }

   TRACE("IntTranslateAccelerator(hwnd %x, message %x, wParam %x, lParam %x, fVirt %d, key %x, cmd %x) = FALSE\n",
          Window->head.h, message, wParam, lParam, fVirt, key, cmd);

   return FALSE;

found:
   if (message == WM_KEYUP || message == WM_SYSKEYUP)
      mesg = 1;
   else
   {
      HMENU hMenu, hSubMenu, hSysMenu;
      UINT uSysStat = (UINT)-1, uStat = (UINT)-1, nPos;
      PMENU_OBJECT MenuObject, SubMenu;
      PMENU_ITEM MenuItem;

      hMenu = (Window->style & WS_CHILD) ? 0 : (HMENU)Window->IDMenu;
      hSysMenu = Window->SystemMenu;
      MenuObject = IntGetMenuObject(Window->SystemMenu);

      /* find menu item and ask application to initialize it */
      /* 1. in the system menu */
      hSubMenu = hSysMenu;
      if (MenuObject)
      {
         nPos = IntGetMenuItemByFlag( MenuObject,
                                      cmd,
                                      MF_BYCOMMAND,
                                     &SubMenu,
                                     &MenuItem,
                                      NULL);

         if (MenuItem && (nPos != (UINT)-1))
         {
            hSubMenu = MenuItem->hSubMenu;

            if (IntGetCaptureWindow())
                mesg = 2;
            if (Window->style & WS_DISABLED)
                mesg = 3;
            else
            {                                               
               co_IntSendMessage(hWnd, WM_INITMENU, (WPARAM)hSysMenu, 0L);
               if (hSubMenu != hSysMenu)
               {
                  nPos = IntFindSubMenu(&hSysMenu, hSubMenu);
                  TRACE("hSysMenu = %p, hSubMenu = %p, nPos = %d\n", hSysMenu, hSubMenu, nPos);
                  co_IntSendMessage(hWnd, WM_INITMENUPOPUP, (WPARAM)hSubMenu, MAKELPARAM(nPos, TRUE));
               }
            uSysStat = IntGetMenuState(IntGetSubMenu(hSysMenu, 0), cmd, MF_BYCOMMAND);
            }
         }
         else /* 2. in the window's menu */
         {
            MenuObject = IntGetMenuObject(hMenu);
            hSubMenu = hMenu;
            if (MenuObject)
            {
               nPos = IntGetMenuItemByFlag( MenuObject,
                                            cmd,
                                            MF_BYCOMMAND,
                                           &SubMenu,
                                           &MenuItem,
                                            NULL);

               if (MenuItem && (nPos != (UINT)-1))
               {
                  if (IntGetCaptureWindow())
                      mesg = 2;
                  if (Window->style & WS_DISABLED)
                      mesg = 3;
                  else
                  {
                     co_IntSendMessage(hWnd, WM_INITMENU, (WPARAM)hMenu, 0L);
                     if (hSubMenu != hMenu)
                     {
                        nPos = IntFindSubMenu(&hMenu, hSubMenu);
                        TRACE("hMenu = %p, hSubMenu = %p, nPos = %d\n", hMenu, hSubMenu, nPos);
                        co_IntSendMessage(hWnd, WM_INITMENUPOPUP, (WPARAM)hSubMenu, MAKELPARAM(nPos, FALSE));
                     }
                     uStat = IntGetMenuState(hMenu, cmd, MF_BYCOMMAND);
                  }
               }
            }
         }
      }
      if (mesg == 0)
      {
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
               if (Window->style & WS_MINIMIZE)
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
      }
   }

   if (mesg == WM_COMMAND)
   {
      TRACE(", sending WM_COMMAND, wParam=%0x\n", 0x10000 | cmd);
      co_IntSendMessage(Window->head.h, mesg, 0x10000 | cmd, 0L);
   }
   else if (mesg == WM_SYSCOMMAND)
   {
      TRACE(", sending WM_SYSCOMMAND, wParam=%0x\n", cmd);
      co_IntSendMessage(Window->head.h, mesg, cmd, 0x00010000L);
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
      ERR(", but won't send WM_{SYS}COMMAND, reason is #%d\n", mesg);
      if (mesg == 0)
      {
         ERR(" unknown reason - please report!");
      }
   }

   TRACE("IntTranslateAccelerator(hWnd %x, message %x, wParam %x, lParam %x, fVirt %d, key %x, cmd %x) = TRUE\n",
          Window->head.h, message, wParam, lParam, fVirt, key, cmd);

   return TRUE;
}


/* SYSCALLS *****************************************************************/


int
APIENTRY
NtUserCopyAcceleratorTable(
   HACCEL hAccel,
   LPACCEL Entries,
   int EntriesCount)
{
   PACCELERATOR_TABLE Accel;
   int Ret;
   DECLARE_RETURN(int);

   TRACE("Enter NtUserCopyAcceleratorTable\n");
   UserEnterShared();

   Accel = UserGetAccelObject(hAccel);
   if (!Accel)
   {
       EngSetLastError(ERROR_INVALID_ACCEL_HANDLE);
       RETURN(0);
   }

   /* If Entries is NULL return table size */
   if (!Entries)
   {
      RETURN(Accel->Count);
   }

   /* Don't overrun */
   if (Accel->Count < EntriesCount)
       EntriesCount = Accel->Count;

   Ret = 0;

   _SEH2_TRY
   {
       ProbeForWrite(Entries, EntriesCount*sizeof(Entries[0]), 4);

       for (Ret = 0; Ret < EntriesCount; Ret++)
       {
           Entries[Ret].fVirt = Accel->Table[Ret].fVirt;
           Entries[Ret].key = Accel->Table[Ret].key;
           Entries[Ret].cmd = Accel->Table[Ret].cmd;
       }
   }
   _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
   {
      SetLastNtError(_SEH2_GetExceptionCode());
      Ret = 0;
   }
   _SEH2_END;

   RETURN(Ret);

CLEANUP:
   TRACE("Leave NtUserCopyAcceleratorTable, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}

HACCEL
APIENTRY
NtUserCreateAcceleratorTable(
   LPACCEL Entries,
   SIZE_T EntriesCount)
{
   PACCELERATOR_TABLE Accel;
   HACCEL hAccel;
   INT Index;
   NTSTATUS Status = STATUS_SUCCESS;
   DECLARE_RETURN(HACCEL);

   TRACE("Enter NtUserCreateAcceleratorTable(Entries %p, EntriesCount %d)\n",
          Entries, EntriesCount);
   UserEnterExclusive();

   if (!Entries || EntriesCount <= 0)
   {
      SetLastNtError(STATUS_INVALID_PARAMETER);
      RETURN( (HACCEL) NULL );
   }

   Accel = UserCreateObject(gHandleTable, NULL, (PHANDLE)&hAccel, otAccel, sizeof(ACCELERATOR_TABLE));

   if (Accel == NULL)
   {
      SetLastNtError(STATUS_NO_MEMORY);
      RETURN( (HACCEL) NULL );
   }

   Accel->Count = EntriesCount;
   Accel->Table = ExAllocatePoolWithTag(PagedPool, EntriesCount * sizeof(ACCEL), USERTAG_ACCEL);
   if (Accel->Table == NULL)
   {
       UserDereferenceObject(Accel);
       UserDeleteObject(hAccel, otAccel);
       SetLastNtError(STATUS_NO_MEMORY);
       RETURN( (HACCEL) NULL);
   }

   _SEH2_TRY
   {
       ProbeForRead(Entries, EntriesCount * sizeof(ACCEL), 4);

       for (Index = 0; Index < EntriesCount; Index++)
       {
           Accel->Table[Index].fVirt = Entries[Index].fVirt & 0x7F;
           if(Accel->Table[Index].fVirt & FVIRTKEY)
           {
               Accel->Table[Index].key = Entries[Index].key;
           }
           else
           {
               RtlMultiByteToUnicodeN(&Accel->Table[Index].key,
                                      sizeof(WCHAR),
                                      NULL,
                                      (PCSTR)&Entries[Index].key,
                                      sizeof(CHAR));
           }

           Accel->Table[Index].cmd = Entries[Index].cmd;
       }
   }
   _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
   {
       Status = _SEH2_GetExceptionCode();
   }
   _SEH2_END;

   if (!NT_SUCCESS(Status))
   {
       ExFreePoolWithTag(Accel->Table, USERTAG_ACCEL);
       UserDereferenceObject(Accel);
       UserDeleteObject(hAccel, otAccel);
       SetLastNtError(Status);
       RETURN( (HACCEL) NULL);
   }

   /* FIXME: Save HandleTable in a list somewhere so we can clean it up again */

   RETURN(hAccel);

CLEANUP:
   TRACE("Leave NtUserCreateAcceleratorTable(Entries %p, EntriesCount %d) = %x\n",
          Entries, EntriesCount,_ret_);
   UserLeave();
   END_CLEANUP;
}

BOOLEAN
APIENTRY
NtUserDestroyAcceleratorTable(
   HACCEL hAccel)
{
   PACCELERATOR_TABLE Accel;
   DECLARE_RETURN(BOOLEAN);

   /* FIXME: If the handle table is from a call to LoadAcceleratorTable, decrement it's
      usage count (and return TRUE).
   FIXME: Destroy only tables created using CreateAcceleratorTable.
    */

   TRACE("NtUserDestroyAcceleratorTable(Table %x)\n", hAccel);
   UserEnterExclusive();

   if (!(Accel = UserGetAccelObject(hAccel)))
   {
      RETURN( FALSE);
   }

   if (Accel->Table != NULL)
   {
      ExFreePoolWithTag(Accel->Table, USERTAG_ACCEL);
      Accel->Table = NULL;
   }

   UserDeleteObject(hAccel, otAccel);

   RETURN( TRUE);

CLEANUP:
   TRACE("Leave NtUserDestroyAcceleratorTable(Table %x) = %i\n", hAccel,_ret_);
   UserLeave();
   END_CLEANUP;
}

int
APIENTRY
NtUserTranslateAccelerator(
   HWND hWnd,
   HACCEL hAccel,
   LPMSG pUnsafeMessage)
{
   PWND Window = NULL;
   PACCELERATOR_TABLE Accel = NULL;
   ULONG i;
   MSG Message;
   USER_REFERENCE_ENTRY AccelRef, WindowRef;
   DECLARE_RETURN(int);

   TRACE("NtUserTranslateAccelerator(hWnd %x, Table %x, Message %p)\n",
          hWnd, hAccel, pUnsafeMessage);
   UserEnterShared();

   if (pUnsafeMessage == NULL)
   {
      SetLastNtError(STATUS_INVALID_PARAMETER);
      RETURN( 0);
   }

   _SEH2_TRY
   {
       ProbeForRead(pUnsafeMessage, sizeof(MSG), 4);
       RtlCopyMemory(&Message, pUnsafeMessage, sizeof(MSG));
   }
   _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
   {
      SetLastNtError(_SEH2_GetExceptionCode());
      _SEH2_YIELD(RETURN( 0));
   }
   _SEH2_END;

   if ((Message.message != WM_KEYDOWN) &&
         (Message.message != WM_SYSKEYDOWN) &&
         (Message.message != WM_SYSCHAR) &&
         (Message.message != WM_CHAR))
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
      if (co_IntTranslateAccelerator(Window, Message.message, Message.wParam, Message.lParam,
                                     Accel->Table[i].fVirt, Accel->Table[i].key,
                                     Accel->Table[i].cmd))
      {
         TRACE("NtUserTranslateAccelerator(hWnd %x, Table %x, Message %p) = %i end\n",
                hWnd, hAccel, pUnsafeMessage, 1);
         RETURN( 1);
      }
   }

   RETURN( 0);

CLEANUP:
   if (Window) UserDerefObjectCo(Window);
   if (Accel) UserDerefObjectCo(Accel);

   TRACE("NtUserTranslateAccelerator(hWnd %x, Table %x, Message %p) = %i end\n",
          hWnd, hAccel, pUnsafeMessage, 0);
   UserLeave();
   END_CLEANUP;
}
