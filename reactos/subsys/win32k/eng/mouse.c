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
/* $Id: mouse.c,v 1.22 2003/03/28 15:27:47 gvg Exp $
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
#include <ddk/ntddmou.h>
#include <win32k/dc.h>
#include "objects.h"
#include "include/msgqueue.h"
#include "include/object.h"
#include "include/winsta.h"

#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

static BOOLEAN SafetySwitch = FALSE;
static BOOLEAN SafetySwitch2 = FALSE;
static BOOLEAN MouseEnabled = FALSE;
static LONG mouse_x, mouse_y;
static UINT mouse_width = 0, mouse_height = 0;
static ULONG PointerStatus;

static UCHAR DefaultCursor[256] = {
  0x3F, 0xFF, 0xFF, 0xFF,
  0x1F, 0xFF, 0xFF, 0xFF,
  0x0F, 0xFF, 0xFF, 0xFF,
  0x07, 0xFF, 0xFF, 0xFF,
  0x03, 0xFF, 0xFF, 0xFF,
  0x01, 0xFF, 0xFF, 0xFF,
  0x00, 0xFF, 0xFF, 0xFF,
  0x00, 0x7F, 0xFF, 0xFF,
  0x00, 0x3F, 0xFF, 0xFF,
  0x00, 0x1F, 0xFF, 0xFF,
  0x00, 0x0F, 0xFF, 0xFF,
  0x00, 0xFF, 0xFF, 0xFF,
  0x00, 0xFF, 0xFF, 0xFF,
  0x18, 0x7F, 0xFF, 0xFF,
  0x38, 0x7F, 0xFF, 0xFF,
  0x7C, 0x3F, 0xFF, 0xFF,
  0xFC, 0x3F, 0xFF, 0xFF,
  0xFE, 0x1F, 0xFF, 0xFF,
  0xFE, 0x1F, 0xFF, 0xFF,
  0xFF, 0x3F, 0xFF, 0xFF,
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

  0x00, 0x00, 0x00, 0x00,
  0x40, 0x00, 0x00, 0x00,
  0x60, 0x00, 0x00, 0x00,
  0x70, 0x00, 0x00, 0x00,
  0x78, 0x00, 0x00, 0x00,
  0x7C, 0x00, 0x00, 0x00,
  0x7E, 0x00, 0x00, 0x00,
  0x7F, 0x00, 0x00, 0x00,
  0x7F, 0x80, 0x00, 0x00,
  0x7F, 0xC0, 0x00, 0x00,
  0x7E, 0x00, 0x00, 0x00,
  0x76, 0x00, 0x00, 0x00,
  0x76, 0x00, 0x00, 0x00,
  0x43, 0x00, 0x00, 0x00,
  0x03, 0x00, 0x00, 0x00,
  0x01, 0x80, 0x00, 0x00,
  0x01, 0x80, 0x00, 0x00,
  0x00, 0xC0, 0x00, 0x00,
  0x00, 0xC0, 0x00, 0x00,
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
  0x00, 0x00, 0x00, 0x00};

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


  /* Mouse is not allowed to move if GDI is busy drawing */
  SafetySwitch2 = TRUE;

  if (SurfObj == NULL)
    {
      return(FALSE);
    }

  if (SurfObj->iType != STYPE_DEVICE || MouseEnabled == FALSE)
    {
      return(FALSE);
    }

  if (SPS_ACCEPT_NOEXCLUDE == PointerStatus)
    {
      /* Hardware cursor, no need to remove it */
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

  if (((mouse_x + mouse_width) >= HazardX1)  && (mouse_x <= HazardX2) &&
      ((mouse_y + mouse_height) >= HazardY1) && (mouse_y <= HazardY2))
    {
      SafetySwitch = TRUE;
      SurfGDI->MovePointer(SurfObj, -1, -1, &MouseRect);
    }

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
      SafetySwitch2 = FALSE;
      return(FALSE);
    }

  if (SurfObj->iType != STYPE_DEVICE || MouseEnabled == FALSE)
    {
      SafetySwitch2 = FALSE;
      return(FALSE);
    }

  if (SPS_ACCEPT_NOEXCLUDE == PointerStatus)
    {
      /* Hardware cursor, it wasn't removed so need to restore it */
      SafetySwitch2 = FALSE;
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
  LARGE_INTEGER LargeTickCount;
  ULONG TickCount;
  static ULONG ButtonsDown = 0;

  KeQueryTickCount(&LargeTickCount);
  TickCount = LargeTickCount.u.LowPart;

  if (hDC == 0)
  {
    return;
  }

  dc = DC_HandleToPtr(hDC);
  SurfObj = (PSURFOBJ)AccessUserObject((ULONG) dc->Surface);
  SurfGDI = (PSURFGDI)AccessInternalObject((ULONG) dc->Surface);
  DC_ReleasePtr( hDC );

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
    if ((0 != Data[i].LastX) || (0 != Data[i].LastY))
    {
      MsqInsertSystemMessage(&Msg);
    }

    if (Data[i].ButtonFlags != 0)
    {
      if ((Data[i].ButtonFlags & MOUSE_LEFT_BUTTON_DOWN) > 0)
      {
      	Msg.wParam  = MK_LBUTTON;
        Msg.message = WM_LBUTTONDOWN;
      }
      if ((Data[i].ButtonFlags & MOUSE_MIDDLE_BUTTON_DOWN) > 0)
      {
      	Msg.wParam  = MK_MBUTTON;
        Msg.message = WM_MBUTTONDOWN;
      }
      if ((Data[i].ButtonFlags & MOUSE_RIGHT_BUTTON_DOWN) > 0)
      {
      	Msg.wParam  = MK_RBUTTON;
        Msg.message = WM_RBUTTONDOWN;
      }

      if ((Data[i].ButtonFlags & MOUSE_LEFT_BUTTON_UP) > 0)
      {
      	Msg.wParam  = MK_LBUTTON;
        Msg.message = WM_LBUTTONUP;
      }
      if ((Data[i].ButtonFlags & MOUSE_MIDDLE_BUTTON_UP) > 0)
      {
      	Msg.wParam  = MK_MBUTTON;
        Msg.message = WM_MBUTTONUP;
      }
      if ((Data[i].ButtonFlags & MOUSE_RIGHT_BUTTON_UP) > 0)
      {
      	Msg.wParam  = MK_RBUTTON;
        Msg.message = WM_RBUTTONUP;
      }

      MsqInsertSystemMessage(&Msg);
    }
  }

  /* If the mouse moved then move the pointer. */
  if ((mouse_cx != 0 || mouse_cy != 0) && MouseEnabled)
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
  PDC dc;
  PSURFOBJ SurfObj;
  PSURFGDI SurfGDI;
  HBITMAP hMouseSurf;
  PSURFOBJ MouseSurf;
  SIZEL MouseSize;
  RECTL MouseRect;

  if( hDisplayDC )
  {
    dc = DC_HandleToPtr(hDisplayDC);
    SurfObj = (PSURFOBJ)AccessUserObject((ULONG) dc->Surface);
    SurfGDI = (PSURFGDI)AccessInternalObject((ULONG) dc->Surface);
    DC_ReleasePtr( hDisplayDC );

    /* Create the default mouse cursor. */
    mouse_width = 32;
    mouse_height = 32;
    MouseSize.cx = 32;
    MouseSize.cy = 64;
    hMouseSurf = EngCreateBitmap(MouseSize, 4, BMF_1BPP, BMF_TOPDOWN, DefaultCursor);
    MouseSurf = (PSURFOBJ)AccessUserObject((ULONG) hMouseSurf);

    /* Tell the display driver to set the pointer shape. */
#if 0
    mouse_x = SurfObj->sizlBitmap.cx / 2;
    mouse_y = SurfObj->sizlBitmap.cy / 2;
#else
    mouse_x = 320;
    mouse_y = 240;
#endif
    PointerStatus = SurfGDI->SetPointerShape(SurfObj, MouseSurf, NULL, NULL,
                                             0, 0, mouse_x, mouse_y, &MouseRect,
                                             SPS_CHANGE);

    MouseEnabled = (SPS_ACCEPT_EXCLUDE == PointerStatus ||
                    SPS_ACCEPT_NOEXCLUDE == PointerStatus);

    EngDeleteSurface(hMouseSurf);
  }
  else
  {
    MouseEnabled = FALSE;
  }
}

