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
/* $Id: mouse.c,v 1.33 2003/08/25 00:28:22 weiden Exp $
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
#include <win32k/win32k.h>
#include <win32k/dc.h>
#include "objects.h"
#include "include/msgqueue.h"
#include "include/object.h"
#include "include/winsta.h"
#include <include/mouse.h>

#define NDEBUG
#include <debug.h>


#define GETSYSCURSOR(x) ((x) - OCR_NORMAL)

/* GLOBALS *******************************************************************/

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

BOOL FASTCALL
CheckClipCursor(LONG *x, LONG *y, PSYSTEM_CURSORINFO CurInfo)
{
  if(CurInfo->CursorClipInfo.IsClipped)
  {
    if(*x > CurInfo->CursorClipInfo.Right)
      *x = CurInfo->CursorClipInfo.Right;
    if(*x < CurInfo->CursorClipInfo.Left)
      *x = CurInfo->CursorClipInfo.Left;
    if(*y > CurInfo->CursorClipInfo.Bottom)
      *y = CurInfo->CursorClipInfo.Bottom;
    if(*y < CurInfo->CursorClipInfo.Top)
      *y = CurInfo->CursorClipInfo.Top;
    return TRUE;
  }
  return TRUE;
}

INT STDCALL
MouseSafetyOnDrawStart(PSURFOBJ SurfObj, PSURFGDI SurfGDI, LONG HazardX1,
		       LONG HazardY1, LONG HazardX2, LONG HazardY2)
/*
 * FUNCTION: Notify the mouse driver that drawing is about to begin in
 * a rectangle on a particular surface.
 */
{
  RECTL MouseRect;
  LONG tmp;
  PSYSTEM_CURSORINFO CurInfo;
  PSYSCURSOR SysCursor;
  BOOL MouseEnabled = FALSE;


  /* Mouse is not allowed to move if GDI is busy drawing */
   
  if(IntGetWindowStationObject(InputWindowStation))
  {
    CurInfo = &InputWindowStation->SystemCursor;
    
    SysCursor = &CurInfo->SystemCursors[CurInfo->CurrentCursor];
    MouseEnabled = CurInfo->Enabled && SysCursor->hCursor;
  }
  else
    return FALSE;
    
  CurInfo->SafetySwitch2 = TRUE;
    
  if (SurfObj == NULL)
    {
      ObDereferenceObject(InputWindowStation);
      return(FALSE);
    }


  if (SurfObj->iType != STYPE_DEVICE || MouseEnabled == FALSE)
    {
      ObDereferenceObject(InputWindowStation);
      return(FALSE);
    }

  if (SPS_ACCEPT_NOEXCLUDE == PointerStatus)
    {
      /* Hardware cursor, no need to remove it */
      ObDereferenceObject(InputWindowStation);
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

  if (((CurInfo->x + SysCursor->cx) >= HazardX1)  && (CurInfo->x <= HazardX2) &&
      ((CurInfo->y + SysCursor->cy) >= HazardY1) && (CurInfo->y <= HazardY2))
    {
      /* Mouse is not allowed to move if GDI is busy drawing */
      ExAcquireFastMutexUnsafe(&CurInfo->CursorMutex);
      CurInfo->SafetySwitch = TRUE;
      SurfGDI->MovePointer(SurfObj, -1, -1, &MouseRect);
      ExReleaseFastMutexUnsafe(&CurInfo->CursorMutex);
    }
    
  ObDereferenceObject(InputWindowStation);
  return(TRUE);
}

INT FASTCALL
MouseSafetyOnDrawEnd(PSURFOBJ SurfObj, PSURFGDI SurfGDI)
/*
 * FUNCTION: Notify the mouse driver that drawing has finished on a surface.
 */
{
  RECTL MouseRect;
  PSYSTEM_CURSORINFO CurInfo;
  PSYSCURSOR SysCursor;
  BOOL MouseEnabled = FALSE;
    
  if(IntGetWindowStationObject(InputWindowStation))
  {
    CurInfo = &InputWindowStation->SystemCursor;
  }
  else
    return FALSE;
    
  if(SurfObj == NULL)
  {
    CurInfo->SafetySwitch2 = FALSE;
    ObDereferenceObject(InputWindowStation);
    return FALSE;
  }
  
  SysCursor = &CurInfo->SystemCursors[CurInfo->CurrentCursor];
  MouseEnabled = CurInfo->Enabled && SysCursor->hCursor;

  if (SurfObj->iType != STYPE_DEVICE || MouseEnabled == FALSE)
    {
      CurInfo->SafetySwitch2 = FALSE;
      ObDereferenceObject(InputWindowStation);
      return(FALSE);
    }

  if (SPS_ACCEPT_NOEXCLUDE == PointerStatus)
    {
      /* Hardware cursor, it wasn't removed so need to restore it */
      CurInfo->SafetySwitch2 = FALSE;
      ObDereferenceObject(InputWindowStation);
      return(FALSE);
    }

  if (CurInfo->SafetySwitch)
    {
      ExAcquireFastMutexUnsafe(&CurInfo->CursorMutex);
      SurfGDI->MovePointer(SurfObj, CurInfo->x, CurInfo->y, &MouseRect);
      CurInfo->SafetySwitch = FALSE;
      ExReleaseFastMutexUnsafe(&CurInfo->CursorMutex);
    }
  
  CurInfo->SafetySwitch2 = FALSE;
  ObDereferenceObject(InputWindowStation);
  return(TRUE);
}

BOOL FASTCALL
MouseMoveCursor(LONG X, LONG Y)
{
  HDC hDC;
  PDC dc;
  RECTL MouseRect;
  BOOL res = FALSE;
  PSURFOBJ SurfObj;
  PSURFGDI SurfGDI;
  PSYSTEM_CURSORINFO CurInfo;
  MSG Msg;
  LARGE_INTEGER LargeTickCount;
  ULONG TickCount;
  static ULONG ButtonsDown = 0;
  
  hDC = IntGetScreenDC();
  
  if(!hDC || !InputWindowStation)
    return FALSE;
  
  if(IntGetWindowStationObject(InputWindowStation))
  {
    dc = DC_LockDc(hDC);
    SurfObj = (PSURFOBJ)AccessUserObject((ULONG) dc->Surface);
    SurfGDI = (PSURFGDI)AccessInternalObject((ULONG) dc->Surface);
    DC_UnlockDc( hDC );
    CurInfo = &InputWindowStation->SystemCursor;
    CheckClipCursor(&X, &Y, CurInfo);
    if((X != CurInfo->x) || (Y != CurInfo->y))
    {
      /* send MOUSEMOVE message */
      KeQueryTickCount(&LargeTickCount);
      TickCount = LargeTickCount.u.LowPart;
      Msg.wParam = ButtonsDown;
      Msg.lParam = MAKELPARAM(X, Y);
      Msg.message = WM_MOUSEMOVE;
      Msg.time = TickCount;
      Msg.pt.x = X;
      Msg.pt.y = Y;
      MsqInsertSystemMessage(&Msg);
      /* move cursor */
      CurInfo->x = X;
      CurInfo->y = Y;
      if(!CurInfo->SafetySwitch && !CurInfo->SafetySwitch2 && CurInfo->Enabled)
      {
        ExAcquireFastMutexUnsafe(&CurInfo->CursorMutex);
        SurfGDI->MovePointer(SurfObj, CurInfo->x, CurInfo->y, &MouseRect);
        ExReleaseFastMutexUnsafe(&CurInfo->CursorMutex);
      }
      res = TRUE;
    }
        
    ObDereferenceObject(InputWindowStation);
    return res;
  }
  else
    return FALSE;
}

VOID /* STDCALL */
MouseGDICallBack(PMOUSE_INPUT_DATA Data, ULONG InputCount)
/*
 * FUNCTION: Call by the mouse driver when input events occur.
 */
{
  ULONG i;
  PSYSTEM_CURSORINFO CurInfo;
  PSYSCURSOR SysCursor;
  BOOL MouseEnabled = FALSE;
  LONG mouse_ox, mouse_oy;
  LONG mouse_cx = 0, mouse_cy = 0;
  HDC hDC;
  PDC dc;
  PSURFOBJ SurfObj;
  PSURFGDI SurfGDI;
  RECTL MouseRect;
  MSG Msg;
  LARGE_INTEGER LargeTickCount;
  ULONG TickCount;
  static ULONG ButtonsDown = 0;
  
  hDC = IntGetScreenDC();
  
  if(!hDC || !InputWindowStation)
    return;

  if(IntGetWindowStationObject(InputWindowStation))
  {
    CurInfo = &InputWindowStation->SystemCursor;
    SysCursor = &CurInfo->SystemCursors[CurInfo->CurrentCursor];
    MouseEnabled = CurInfo->Enabled;
    mouse_ox = CurInfo->x;
    mouse_oy = CurInfo->y;
  }
  else
    return;

  KeQueryTickCount(&LargeTickCount);
  TickCount = LargeTickCount.u.LowPart;

  dc = DC_LockDc(hDC);
  SurfObj = (PSURFOBJ)AccessUserObject((ULONG) dc->Surface);
  SurfGDI = (PSURFGDI)AccessInternalObject((ULONG) dc->Surface);
  DC_UnlockDc( hDC );

  /* Compile the total mouse movement change and dispatch button events. */
  for (i = 0; i < InputCount; i++)
  {
    mouse_cx += Data[i].LastX;
    mouse_cy += Data[i].LastY;

    Msg.wParam = ButtonsDown;
    Msg.lParam = MAKELPARAM(CurInfo->x + mouse_cx, CurInfo->y + mouse_cy);
    Msg.message = WM_MOUSEMOVE;
    Msg.time = TickCount;
    Msg.pt.x = CurInfo->x + mouse_cx;
    Msg.pt.y = CurInfo->y + mouse_cy;
    
    CheckClipCursor(&Msg.pt.x, &Msg.pt.y, CurInfo);
    
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
    CurInfo->x += mouse_cx;
    CurInfo->y += mouse_cy;

    CurInfo->x = max(CurInfo->x, 0);
    CurInfo->y = max(CurInfo->y, 0);
    CurInfo->x = min(CurInfo->x, SurfObj->sizlBitmap.cx - 20);
    CurInfo->y = min(CurInfo->y, SurfObj->sizlBitmap.cy - 20);
    
    CheckClipCursor(&CurInfo->x, &CurInfo->y, CurInfo);
    
    if (!CurInfo->SafetySwitch && !CurInfo->SafetySwitch2 &&
        ((mouse_ox != CurInfo->x) || (mouse_oy != CurInfo->y)))
    {
      ExAcquireFastMutexUnsafe(&CurInfo->CursorMutex);
      SurfGDI->MovePointer(SurfObj, CurInfo->x, CurInfo->y, &MouseRect);
      ExReleaseFastMutexUnsafe(&CurInfo->CursorMutex);
    }
  }

  ObDereferenceObject(InputWindowStation);
}

VOID FASTCALL
EnableMouse(HDC hDisplayDC)
{
  PDC dc;
  PSURFOBJ SurfObj;
  PSURFGDI SurfGDI;
  HBITMAP hMouseSurf;
  PSURFOBJ MouseSurf;
  SIZEL MouseSize;
  RECTL MouseRect;
  PSYSTEM_CURSORINFO CurInfo;
  PSYSCURSOR SysCursor;

  if( hDisplayDC && InputWindowStation)
  {
    if(!IntGetWindowStationObject(InputWindowStation))
    {
       InputWindowStation->SystemCursor.Enabled = FALSE;
       return;
    }
    
    CurInfo = &InputWindowStation->SystemCursor;
    SysCursor = &CurInfo->SystemCursors[CurInfo->CurrentCursor];
    
    dc = DC_LockDc(hDisplayDC);
    SurfObj = (PSURFOBJ)AccessUserObject((ULONG) dc->Surface);
    SurfGDI = (PSURFGDI)AccessInternalObject((ULONG) dc->Surface);
    DC_UnlockDc( hDisplayDC );
    
    /* Tell the display driver to set the pointer shape. */
#if 1
    CurInfo->x = SurfObj->sizlBitmap.cx / 2;
    CurInfo->y = SurfObj->sizlBitmap.cy / 2;
#else
    CurInfo->x = 320;
    CurInfo->y = 240;
#endif

    /* Create the default mouse cursor. */
    MouseSize.cx = SysCursor->cx;
    MouseSize.cy = SysCursor->cy * 2;
    hMouseSurf = EngCreateBitmap(MouseSize, 4, BMF_1BPP, BMF_TOPDOWN, DefaultCursor);
    MouseSurf = (PSURFOBJ)AccessUserObject((ULONG) hMouseSurf);

    DbgPrint("Setting Cursor up at 0x%x, 0x%x\n", CurInfo->x, CurInfo->y);
    CheckClipCursor(&CurInfo->x, 
                    &CurInfo->y,
                    CurInfo);

    PointerStatus = SurfGDI->SetPointerShape(SurfObj, MouseSurf, NULL, NULL,
                                             0, 0, 
                                             CurInfo->x, 
                                             CurInfo->y, 
                                             &MouseRect,
                                             SPS_CHANGE);

    InputWindowStation->SystemCursor.Enabled = (SPS_ACCEPT_EXCLUDE == PointerStatus ||
                                                SPS_ACCEPT_NOEXCLUDE == PointerStatus);

    EngDeleteSurface(hMouseSurf);
    ObDereferenceObject(InputWindowStation);
    
  }
  else
  {
    if(IntGetWindowStationObject(InputWindowStation))
    {
       InputWindowStation->SystemCursor.Enabled = FALSE;
       InputWindowStation->SystemCursor.CursorClipInfo.IsClipped = FALSE;
	   ObDereferenceObject(InputWindowStation);
       return;
    }
  }
}
/* EOF */
