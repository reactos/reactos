/*
 *  ReactOS kernel
 *  Copyright (C) 1998, 1999, 2000, 2001 ReactOS Team
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
/* $Id: mouse.c,v 1.14 2002/10/31 00:03:30 dwelch Exp $
 *
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Mouse
 * FILE:             subsys/win32k/eng/mouse.c
 * PROGRAMER:        Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISION HISTORY:
 *       06-06-2001  CSH  Created
 */
/* INCLUDES ******************************************************************/

#include <windows.h>
#include <ddk/ntddk.h>
#include <win32k/dc.h>
#include "../../drivers/input/include/mouse.h"
#include "objects.h"
#include "include/msgqueue.h"

/* GLOBALS *******************************************************************/

static BOOLEAN SafetySwitch = FALSE;
static BOOLEAN SafetySwitch2 = FALSE;
static BOOLEAN MouseEnabled = FALSE;
static LONG mouse_x, mouse_y;
static UINT mouse_width = 0, mouse_height = 0;

static UCHAR DefaultCursor[256] = {
  0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 
  0x00, 0xC0, 0x00, 0x00, 
  0x00, 0xC0, 0x00, 0x00, 
  0x01, 0x80, 0x00, 0x00, 
  0x01, 0x80, 0x00, 0x00, 
  0x03, 0x00, 0x00, 0x00, 
  0x43, 0x00, 0x00, 0x00, 
  0x66, 0x00, 0x00, 0x00, 
  0x76, 0x00, 0x00, 0x00, 
  0x7E, 0x00, 0x00, 0x00, 
  0x7F, 0xC0, 0x00, 0x00, 
  0x7F, 0x80, 0x00, 0x00, 
  0x7F, 0x00, 0x00, 0x00, 
  0x7E, 0x00, 0x00, 0x00, 
  0x7C, 0x00, 0x00, 0x00, 
  0x78, 0x00, 0x00, 0x00, 
  0x70, 0x00, 0x00, 0x00, 
  0x60, 0x00, 0x00, 0x00, 
  0x40, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 

  0xFF, 0xFF, 0xFF, 0xFF, 
  0xFF, 0xFF, 0xFF, 0xFF, 
  0xFF, 0xFF, 0xFF, 0xFF, 
  0xFF, 0xFF, 0xFF, 0xFF, 
  0xFF, 0xFF, 0xFF, 0xFF, 
  0xFF, 0xFF, 0xFF, 0xFF, 
  0xFF, 0xFF, 0xFF, 0xFF, 
  0xFF, 0xFF, 0xFF, 0xFF, 
  0xFF, 0xFF, 0xFF, 0xFF, 
  0xFF, 0xFF, 0xFF, 0xFF, 
  0xFF, 0xFF, 0xFF, 0xFF, 
  0xFF, 0xFF, 0xFF, 0xFF, 
  0xFF, 0x3F, 0xFF, 0xFF, 
  0xFE, 0x1F, 0xFF, 0xFF, 
  0xFE, 0x1F, 0xFF, 0xFF, 
  0xFC, 0x3F, 0xFF, 0xFF, 
  0x7C, 0x3F, 0xFF, 0xFF, 
  0x38, 0x7F, 0xFF, 0xFF, 
  0x18, 0x7F, 0xFF, 0xFF, 
  0x00, 0xFF, 0xFF, 0xFF, 
  0x00, 0xFF, 0xFF, 0xFF, 
  0x00, 0x0F, 0xFF, 0xFF, 
  0x00, 0x1F, 0xFF, 0xFF, 
  0x00, 0x3F, 0xFF, 0xFF, 
  0x00, 0x7F, 0xFF, 0xFF, 
  0x00, 0xFF, 0xFF, 0xFF, 
  0x01, 0xFF, 0xFF, 0xFF, 
  0x03, 0xFF, 0xFF, 0xFF, 
  0x07, 0xFF, 0xFF, 0xFF, 
  0x0F, 0xFF, 0xFF, 0xFF, 
  0x1F, 0xFF, 0xFF, 0xFF, 
  0x3F, 0xFF, 0xFF, 0xFF};

/* FUNCTIONS *****************************************************************/

INT 
MouseSafetyOnDrawStart(PSURFOBJ SurfObj, PSURFGDI SurfGDI, LONG HazardX1, 
		       LONG HazardY1, LONG HazardX2, LONG HazardY2)
/*
 * FUNCTION: Notify the mouse driver that drawing is about to begin in
 * a rectangle on a particular surface.
 */
{
  RECTL MouseRect;
  LONG tmp;

  if (SurfObj == NULL) 
    {
      return(FALSE);
    }

  if (SurfObj->iType != STYPE_DEVICE || MouseEnabled == FALSE) 
    {
      return(FALSE);
    }

  if (HazardX1 > HazardX2) 
    { 
      tmp = HazardX2; HazardX2 = HazardX1; HazardX1 = tmp; 
    }
  if (HazardY1 > HazardY2) 
    { 
      tmp = HazardY2; HazardY2 = HazardY1; HazardY1 = tmp; 
    }

  if ((mouse_x + mouse_width >= HazardX1)  && (mouse_x <= HazardX2) &&
      (mouse_y + mouse_height >= HazardY1) && (mouse_y <= HazardY2))
    {
      SurfGDI->MovePointer(SurfObj, -1, -1, &MouseRect);
      SafetySwitch = TRUE;
    }

  /* Mouse is not allowed to move if GDI is busy drawing */
  SafetySwitch2 = TRUE;

  return(TRUE);
}

INT 
MouseSafetyOnDrawEnd(PSURFOBJ SurfObj, PSURFGDI SurfGDI)
/*
 * FUNCTION: Notify the mouse driver that drawing has finished on a surface.
 */
{
  RECTL MouseRect;

  if (SurfObj == NULL) 
    {
      return(FALSE);
    }

  if (SurfObj->iType != STYPE_DEVICE || MouseEnabled == FALSE) 
    {
      return(FALSE);
    }

  if (SafetySwitch)
    {
      SurfGDI->MovePointer(SurfObj, mouse_x, mouse_y, &MouseRect);
      SafetySwitch = FALSE;
    }
  
  SafetySwitch2 = FALSE;

  return(TRUE);
}

VOID
MouseGDICallBack(PMOUSE_INPUT_DATA Data, ULONG InputCount)
/*
 * FUNCTION: Call by the mouse driver when input events occur.
 */
{
  ULONG i;
  LONG mouse_cx = 0, mouse_cy = 0;
  HDC hDC = W32kGetScreenDC();
  PDC dc;
  PSURFOBJ SurfObj;
  PSURFGDI SurfGDI;
  RECTL MouseRect; 
  MSG Msg;
  ULONG j;
  LARGE_INTEGER LargeTickCount;
  ULONG TickCount;
  static ULONG ButtonsDown = 0;
  const UINT MouseButtonDownMessage[3] = 
    {WM_RBUTTONDOWN, WM_MBUTTONDOWN, WM_LBUTTONDOWN};
  const UINT MouseButtonUpMessage[3] =
    {WM_RBUTTONUP, WM_MBUTTONUP, WM_LBUTTONUP};
  const ULONG MouseButtonFlag[3] = {MK_RBUTTON, MK_MBUTTON, MK_LBUTTON};

  KeQueryTickCount(&LargeTickCount);
  TickCount = LargeTickCount.u.LowPart;

  if (hDC == 0)
    {
      return;
    }

  dc = DC_HandleToPtr(hDC);
  SurfObj = (PSURFOBJ)AccessUserObject(dc->Surface);
  SurfGDI = (PSURFGDI)AccessInternalObject(dc->Surface);
  
  /* Compile the total mouse movement change and dispatch button events. */
  for (i = 0; i < InputCount; i++)
    {
      mouse_cx += Data[i].LastX;
      mouse_cy += Data[i].LastY;

      Msg.wParam = ButtonsDown;
      Msg.lParam = MAKELPARAM(mouse_x + mouse_cx, mouse_y + mouse_cy);
      Msg.message = WM_MOUSEMOVE;
      Msg.time = TickCount;
      Msg.pt.x = mouse_x + mouse_cx;
      Msg.pt.y = mouse_y + mouse_cy;
      MsqInsertSystemMessage(&Msg);
      
      for (j = 0; j < 3; j++)
	{
	  ULONG Flag = MouseButtonFlag[j];
	  if (Data[i].ButtonData & (1 << j) && !(ButtonsDown & Flag))
	    {
	      ButtonsDown |= Flag;

	      Msg.wParam = ButtonsDown;
	      Msg.message = MouseButtonDownMessage[j];
	      MsqInsertSystemMessage(&Msg);
	    }
	  if (!(Data[i].ButtonData & (1 << j)) && (ButtonsDown & Flag))
	    {	      
	      ButtonsDown &= ~Flag;	   

	      Msg.wParam = ButtonsDown;
	      Msg.message = MouseButtonUpMessage[j];
	      MsqInsertSystemMessage(&Msg);
	    }
	}
    }

  /* If the mouse moved then move the pointer. */
  if (mouse_cx != 0 || mouse_cy != 0)
    {     
      mouse_x += mouse_cx;
      mouse_y += mouse_cy;      

      mouse_x = max(mouse_x, 0);
      mouse_y = max(mouse_y, 0);
      mouse_x = min(mouse_x, 620);
      mouse_y = min(mouse_y, 460);

      if (SafetySwitch == FALSE && SafetySwitch2 == FALSE)
	{
	  SurfGDI->MovePointer(SurfObj, mouse_x, mouse_y, &MouseRect); 
	}
    }
}

VOID 
EnableMouse(HDC hDisplayDC)
{
  PDC dc = DC_HandleToPtr(hDisplayDC);
  PSURFOBJ SurfObj = (PSURFOBJ)AccessUserObject(dc->Surface);
  PSURFGDI SurfGDI = (PSURFGDI)AccessInternalObject(dc->Surface);
  HBITMAP hMouseSurf;
  PSURFOBJ MouseSurf;
  SIZEL MouseSize;
  RECTL MouseRect;

  /* Create the default mouse cursor. */
  mouse_width = 32;
  mouse_height = 32;
  MouseSize.cx = 32;
  MouseSize.cy = 64;
  hMouseSurf = EngCreateBitmap(MouseSize, 4, BMF_1BPP, 0, DefaultCursor);
  MouseSurf = (PSURFOBJ)AccessUserObject(hMouseSurf);

  /* Tell the display driver to set the pointer shape. */
  SurfGDI->SetPointerShape(SurfObj, MouseSurf, NULL, NULL, 0, 0, 320, 240, 
			   &MouseRect, 0);

  mouse_x = 320;
  mouse_y = 240;
  MouseEnabled = TRUE;
}

