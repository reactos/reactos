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


static BOOL Setup = FALSE;

/* FUNCTIONS *****************************************************************/

BOOL
FASTCALL
InitMetrics(VOID)
{
  INT Index;
  NTSTATUS Status;
  PWINSTATION_OBJECT WinStaObject;
  ULONG Width = 640, Height = 480;

  for (Index = 0; Index < SM_CMETRICS; Index++)
  {
      switch (Index)
      {
       case SM_CXSCREEN:
         {
            HDC ScreenDCHandle;
            PDC ScreenDC;

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
            gpsi->SystemMetrics[Index] = Width;
            break;
         }
       case SM_CYSCREEN:
          gpsi->SystemMetrics[Index] = Height;
          break;
       case SM_ARRANGE:
          gpsi->SystemMetrics[Index] = 8;
          break;
       case SM_CLEANBOOT:
          gpsi->SystemMetrics[Index] = 0;
          break;
       case SM_CMOUSEBUTTONS:
          gpsi->SystemMetrics[Index] = 2;
          break;
       case SM_CXBORDER:
       case SM_CYBORDER:
          gpsi->SystemMetrics[Index] = 1;
          break;
       case SM_CXCURSOR:
       case SM_CYCURSOR:
          gpsi->SystemMetrics[Index] = 32;
          break;
       case SM_CXDLGFRAME:
       case SM_CYDLGFRAME:
          gpsi->SystemMetrics[Index] = 3;
          break;
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
               gpsi->SystemMetrics[Index] = 0xFFFFFFFF;
               break;

            CurInfo = IntGetSysCursorInfo(WinStaObject);
            switch(Index)
            {
               case SM_CXDOUBLECLK:
                  gpsi->SystemMetrics[Index] = CurInfo->DblClickWidth;
                  break;
               case SM_CYDOUBLECLK:
                  gpsi->SystemMetrics[Index] = CurInfo->DblClickWidth;
                  break;
               case SM_SWAPBUTTON:
                  gpsi->SystemMetrics[Index] = CurInfo->SwapButtons;
                  break;
            }

            ObDereferenceObject(WinStaObject);
            break;
          }
       case SM_CXDRAG:
       case SM_CYDRAG:
          gpsi->SystemMetrics[Index] = 2;
          break;
       case SM_CXEDGE:
       case SM_CYEDGE:
          gpsi->SystemMetrics[Index] = 2;
          break;
       case SM_CXFRAME:
       case SM_CYFRAME:
          gpsi->SystemMetrics[Index] = 4;
          break;
       case SM_CXFULLSCREEN:
          /* FIXME: shouldn't we take borders etc into account??? */
          gpsi->SystemMetrics[Index] = gpsi->SystemMetrics[SM_CXSCREEN];
          break;
       case SM_CYFULLSCREEN:
          gpsi->SystemMetrics[Index] = gpsi->SystemMetrics[SM_CYSCREEN];
          break;
       case SM_CXHSCROLL:
       case SM_CYHSCROLL:
          gpsi->SystemMetrics[Index] = 16;
          break;
       case SM_CYVTHUMB:
       case SM_CXHTHUMB:
          gpsi->SystemMetrics[Index] = 16;
          break;
        case SM_CXICON:
        case SM_CYICON:
          gpsi->SystemMetrics[Index] = 32;
          break;
       case SM_CXICONSPACING:
       case SM_CYICONSPACING:
          gpsi->SystemMetrics[Index] = 64;
          break;
       case SM_CXMAXIMIZED:
     /* This seems to be 8 pixels greater than the screen width */
          gpsi->SystemMetrics[Index] = gpsi->SystemMetrics[SM_CXSCREEN] + 8;
          break;
       case SM_CYMAXIMIZED:
     /* This seems to be 20 pixels less than the screen height, taskbar maybe? */
          gpsi->SystemMetrics[Index] = gpsi->SystemMetrics[SM_CYSCREEN] - 20;
          break;
       case SM_CXMAXTRACK:
          gpsi->SystemMetrics[Index] = gpsi->SystemMetrics[SM_CYSCREEN] + 12;
          break;
       case SM_CYMAXTRACK:
          gpsi->SystemMetrics[Index] = gpsi->SystemMetrics[SM_CYSCREEN] + 12;
          break;
       case SM_CXMENUCHECK:
       case SM_CYMENUCHECK:
          gpsi->SystemMetrics[Index] = 13;
          break;
       case SM_CXMENUSIZE:
       case SM_CYMENUSIZE:
          gpsi->SystemMetrics[Index] = 18;
          break;
       case SM_CXMIN:
          gpsi->SystemMetrics[Index] = 112;
          break;
       case SM_CYMIN:
          gpsi->SystemMetrics[Index] = 27;
          break;
       case SM_CXMINIMIZED:
          gpsi->SystemMetrics[Index] = 160;
          break;
       case SM_CYMINIMIZED:
          gpsi->SystemMetrics[Index] = 24;
          break;
       case SM_CXMINSPACING:
          gpsi->SystemMetrics[Index] = 160;
          break;
       case SM_CYMINSPACING:
          gpsi->SystemMetrics[Index] = 24;
          break;
       case SM_CXMINTRACK:
          gpsi->SystemMetrics[Index] = 112;
          break;
       case SM_CYMINTRACK:
          gpsi->SystemMetrics[Index] = 27;
          break;
       case SM_CXSIZE:
       case SM_CYSIZE:
          gpsi->SystemMetrics[Index] = 18;
          break;
       case SM_CXSMICON:
       case SM_CYSMICON:
          gpsi->SystemMetrics[Index] = 16;
          break;
       case SM_CXSMSIZE:
          gpsi->SystemMetrics[Index] = 12;
          break;
       case SM_CYSMSIZE:
          gpsi->SystemMetrics[Index] = 14;
          break;
       case SM_CXVSCROLL:
       case SM_CYVSCROLL:
          gpsi->SystemMetrics[Index] = 16;
          break;
       case SM_CYCAPTION:
          gpsi->SystemMetrics[Index] = 19;
          break;
       case SM_CYKANJIWINDOW:
          gpsi->SystemMetrics[Index] = 0;
          break;
       case SM_CYMENU:
          gpsi->SystemMetrics[Index] = 19;
          break;
       case SM_CYSMCAPTION:
          gpsi->SystemMetrics[Index] = 15;
          break;
       case SM_DBCSENABLED:
       case SM_DEBUG:
       case SM_MENUDROPALIGNMENT:
       case SM_MIDEASTENABLED:
          gpsi->SystemMetrics[Index] = 0;
          break;
       case SM_MOUSEPRESENT:
          gpsi->SystemMetrics[Index] = 1;
          break;
       case SM_NETWORK:
          gpsi->SystemMetrics[Index] = 3;
          break;
       case SM_PENWINDOWS:
       case SM_SECURE:
       case SM_SHOWSOUNDS:
       case SM_SLOWMACHINE:
          gpsi->SystemMetrics[Index] = 0;
          break;
       case SM_CMONITORS:
          gpsi->SystemMetrics[Index] = 1;
          break;
       case SM_REMOTESESSION:
          gpsi->SystemMetrics[Index] = 0;
          break;
       default:
          gpsi->SystemMetrics[Index] = 0xFFFFFFFF;
      }
  }
  gpsi->SRVINFO_Flags |= SRVINFO_METRICS;
  Setup = TRUE;
  return TRUE;
}

ULONG FASTCALL
UserGetSystemMetrics(ULONG Index)
{
   NTSTATUS Status;
   PWINSTATION_OBJECT WinStaObject;
   ULONG Width, Height, Result;

//  DPRINT1("UserGetSystemMetrics -> %d\n",Index);

  if (Index >= SM_CMETRICS)
  {
     DPRINT1("UserGetSystemMetrics() called with invalid index %d\n", Index);
     return 0;
  }

  if (gpsi && Setup)
     return gpsi->SystemMetrics[Index];
  else
  {
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
         InitMetrics();
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
}

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
