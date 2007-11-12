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
 * FILE:             subsys/win32k/ntuser/metric.c
 * PROGRAMER:        Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISION HISTORY:
 *       06-06-2001  CSH  Created
 */

/* INCLUDES ******************************************************************/

#include <w32k.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

/* FIXME:  Alot of thse values should NOT be hardcoded but they are */
ULONG FASTCALL
UserGetSystemMetrics(ULONG Index)
{
   NTSTATUS Status;
   PWINSTATION_OBJECT WinStaObject;
   ULONG Width, Height, Result;

   Result = 0;
   switch (Index)
   {
      case SM_ARRANGE:
         return(8);
      case SM_CLEANBOOT:
         return(0);
      case SM_CMOUSEBUTTONS:
         return(2);
      case SM_CXBORDER:
      case SM_CYBORDER:
         return(1);
      case SM_CXCURSOR:
      case SM_CYCURSOR:
         return(32);
      case SM_CXDLGFRAME:
      case SM_CYDLGFRAME:
         return(3);
      case SM_CXDOUBLECLK:
      case SM_CYDOUBLECLK:
      case SM_SWAPBUTTON:
         {
            PSYSTEM_CURSORINFO CurInfo;
            Status = IntValidateWindowStationHandle(PsGetCurrentProcess()->Win32WindowStation,
                                                    KernelMode,
                                                    0,
                                                    &WinStaObject);
            if (!NT_SUCCESS(Status))
               return 0xFFFFFFFF;

            CurInfo = IntGetSysCursorInfo(WinStaObject);
            switch(Index)
            {
               case SM_CXDOUBLECLK:
                  Result = CurInfo->DblClickWidth;
                  break;
               case SM_CYDOUBLECLK:
                  Result = CurInfo->DblClickWidth;
                  break;
               case SM_SWAPBUTTON:
                  Result = (UINT)CurInfo->SwapButtons;
                  break;
            }

            ObDereferenceObject(WinStaObject);
            return Result;
         }

      case SM_CXDRAG:
      case SM_CYDRAG:
         return(2);
      case SM_CXEDGE:
      case SM_CYEDGE:
         return(2);
      case SM_CXFRAME:
      case SM_CYFRAME:
         return(4);
      case SM_CXFULLSCREEN:
         /* FIXME: shouldn't we take borders etc into account??? */
         return UserGetSystemMetrics(SM_CXSCREEN);
      case SM_CYFULLSCREEN:
         return UserGetSystemMetrics(SM_CYSCREEN);
      case SM_CXHSCROLL:
      case SM_CYHSCROLL:
         return(16);
      case SM_CYVTHUMB:
      case SM_CXHTHUMB:
         return(16);
      case SM_CXICON:
      case SM_CYICON:
         return(32);
      case SM_CXICONSPACING:
      case SM_CYICONSPACING:
         return(64);
      case SM_CXMAXIMIZED:
         return(UserGetSystemMetrics(SM_CXSCREEN) + 8); /* This seems to be 8
                                                                   pixels greater than
                                                                   the screen width */
      case SM_CYMAXIMIZED:
         return(UserGetSystemMetrics(SM_CYSCREEN) - 20); /* This seems to be 20
                                                                    pixels less than
                                                                    the screen height,
                                                                    taskbar maybe? */
      case SM_CXMAXTRACK:
         return(UserGetSystemMetrics(SM_CYSCREEN) + 12);
      case SM_CYMAXTRACK:
         return(UserGetSystemMetrics(SM_CYSCREEN) + 12);
      case SM_CXMENUCHECK:
      case SM_CYMENUCHECK:
         return(13);
      case SM_CXMENUSIZE:
      case SM_CYMENUSIZE:
         return(18);
      case SM_CXMIN:
         return(112);
      case SM_CYMIN:
         return(27);
      case SM_CXMINIMIZED:
         return(160);
      case SM_CYMINIMIZED:
         return(24);
      case SM_CXMINSPACING:
         return(160);
      case SM_CYMINSPACING:
         return(24);
      case SM_CXMINTRACK:
         return(112);
      case SM_CYMINTRACK:
         return(27);
      case SM_CXSCREEN:
      case SM_CYSCREEN:
         {
            HDC ScreenDCHandle;
            PDC ScreenDC;

            Width = 640;
            Height = 480;
            ScreenDCHandle = IntGdiCreateDC(NULL, NULL, NULL, NULL, TRUE);
            if (NULL != ScreenDCHandle)
            {
               ScreenDC = DC_LockDc(ScreenDCHandle);
               if (NULL != ScreenDC)
               {
                  Width = ((PGDIDEVICE)ScreenDC->pPDev)->GDIInfo.ulHorzRes;
                  Height = ((PGDIDEVICE)ScreenDC->pPDev)->GDIInfo.ulVertRes;
                  DC_UnlockDc(ScreenDC);
               }
               NtGdiDeleteObjectApp(ScreenDCHandle);
            }
            return SM_CXSCREEN == Index ? Width : Height;
         }
      case SM_CXSIZE:
      case SM_CYSIZE:
         return(18);
      case SM_CXSMICON:
      case SM_CYSMICON:
         return(16);
      case SM_CXSMSIZE:
         return(12);
      case SM_CYSMSIZE:
         return(14);
      case SM_CXVSCROLL:
      case SM_CYVSCROLL:
         return(16);
      case SM_CYCAPTION:
         return(19);
      case SM_CYKANJIWINDOW:
         return 0;
      case SM_CYMENU:
         return(19);
      case SM_CYSMCAPTION:
         return(15);
      case SM_DBCSENABLED:
      case SM_DEBUG:
      case SM_MENUDROPALIGNMENT:
      case SM_MIDEASTENABLED:
         return(0);
      case SM_MOUSEPRESENT:
         return(1);
      case SM_NETWORK:
         return(3);
      case SM_PENWINDOWS:
      case SM_SECURE:
      case SM_SHOWSOUNDS:
      case SM_SLOWMACHINE:
         return(0);
      case SM_CMONITORS:
         return(1);
      case SM_REMOTESESSION:
         return (0);

      default:
         return(0xFFFFFFFF);
   }
}



/* FIXME:  Alot of thse values should NOT be hardcoded but they are */
ULONG STDCALL
NtUserGetSystemMetrics(ULONG Index)
{
   DECLARE_RETURN(ULONG);

   DPRINT("Enter NtUserGetSystemMetrics\n");
   UserEnterShared();

   RETURN(UserGetSystemMetrics(Index));

CLEANUP:
   DPRINT("Leave NtUserGetSystemMetrics, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}
/* EOF */
