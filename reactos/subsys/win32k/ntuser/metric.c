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
/* $Id: metric.c,v 1.21.12.2 2004/09/14 01:00:44 weiden Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Window classes
 * FILE:             subsys/win32k/ntuser/metric.c
 * PROGRAMER:        Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISION HISTORY:
 *       06-06-2001  CSH  Created
 */
#include <w32k.h>

#define NDEBUG
#include <debug.h>


INT INTERNAL_CALL
IntGetSystemMetrics(INT nIndex)
{
  PW32PROCESS W32Process = PsGetWin32Process();
  PWINSTATION_OBJECT WinStaObject;
  
  WinStaObject = (W32Process != NULL ? W32Process->WindowStation : NULL);
#ifdef DBG
  if(WinStaObject == NULL)
  {
    DPRINT1("GetSystemMetrics: The Window Station for this process is inaccessible!\n");
  }
#endif
  
  switch (nIndex)
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
      if(WinStaObject == NULL)
      {
	return 0;
      }
      
      CurInfo = IntGetSysCursorInfo(WinStaObject);
      ASSERT(CurInfo);
      
      switch(nIndex)
      {
        case SM_CXDOUBLECLK:
          return CurInfo->DblClickWidth;
        case SM_CYDOUBLECLK:
          return CurInfo->DblClickWidth;
        case SM_SWAPBUTTON:
          return (UINT)CurInfo->SwapButtons;
      }
      return 0;
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
      return IntGetSystemMetrics(SM_CXSCREEN);
    case SM_CYFULLSCREEN:
      return IntGetSystemMetrics(SM_CYSCREEN);
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
      return(IntGetSystemMetrics(SM_CXSCREEN) + 8); /* This seems to be 8
                                                       pixels greater than
                                                       the screen width */
    case SM_CYMAXIMIZED:
      return(IntGetSystemMetrics(SM_CYSCREEN) - 20); /* This seems to be 20
                                                        pixels less than 
                                                        the screen height, 
                                                        taskbar maybe? */
    case SM_CXMAXTRACK:
      return(IntGetSystemMetrics(SM_CYSCREEN) + 12);
    case SM_CYMAXTRACK:
      return(IntGetSystemMetrics(SM_CYSCREEN) + 12);
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
      ULONG Width, Height;
      PWINDOW_OBJECT DesktopWindow;
      
      /* FIXME - make sure we have the lock */
      DesktopWindow = IntGetDesktopWindow();
      if (NULL != DesktopWindow)
	  {
	    Width = DesktopWindow->WindowRect.right;
	    Height = DesktopWindow->WindowRect.bottom;
	  }
      else
	  {
	    Width = 640;
	    Height = 480;
	  }
      return (SM_CXSCREEN == nIndex ? Width : Height);
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
  }
  
  return 0xFFFFFFFF;
}

