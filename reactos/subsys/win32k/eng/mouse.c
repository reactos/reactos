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
/* $Id: mouse.c,v 1.56 2004/01/17 15:20:25 navaraf Exp $
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
#include "include/desktop.h"
#include "include/window.h"
#include "include/cursoricon.h"
#include "include/callback.h"
#include <include/mouse.h>

#define NDEBUG
#include <debug.h>

#define GETSYSCURSOR(x) ((x) - OCR_NORMAL)

/* FUNCTIONS *****************************************************************/

BOOL FASTCALL
IntCheckClipCursor(LONG *x, LONG *y, PSYSTEM_CURSORINFO CurInfo)
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

BOOL FASTCALL
IntSwapMouseButton(PWINSTATION_OBJECT WinStaObject, BOOL Swap)
{
  BOOL res = WinStaObject->SystemCursor.SwapButtons;
  WinStaObject->SystemCursor.SwapButtons = Swap;
  return res;
}

INT STDCALL
MouseSafetyOnDrawStart(PSURFOBJ SurfObj, PSURFGDI SurfGDI, LONG HazardX1,
		       LONG HazardY1, LONG HazardX2, LONG HazardY2)
/*
 * FUNCTION: Notify the mouse driver that drawing is about to begin in
 * a rectangle on a particular surface.
 */
{
  LONG tmp;
  PSYSTEM_CURSORINFO CurInfo;
  BOOL MouseEnabled = FALSE;
  PCURICON_OBJECT Cursor;


  /* Mouse is not allowed to move if GDI is busy drawing */
   
  if(IntGetWindowStationObject(InputWindowStation))
  {
    CurInfo = &InputWindowStation->SystemCursor;
    
    MouseEnabled = CurInfo->Enabled && CurInfo->ShowingCursor;
  }
  else
    return FALSE;
    
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

  if (SPS_ACCEPT_NOEXCLUDE == SurfGDI->PointerStatus)
    {
      /* Hardware cursor, no need to remove it */
      ObDereferenceObject(InputWindowStation);
      return(FALSE);
    }
  
  if(!(Cursor = CurInfo->CurrentCursorObject))
  {
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

  if (CurInfo->PointerRectRight >= HazardX1
      && CurInfo->PointerRectLeft <= HazardX2
      && CurInfo->PointerRectBottom  >= HazardY1
      && CurInfo->PointerRectTop <= HazardY2)
    {
      /* Mouse is not allowed to move if GDI is busy drawing */
      ExAcquireFastMutex(&CurInfo->CursorMutex);
      if (0 != CurInfo->SafetyRemoveCount++)
        {
          /* Was already removed */
          ExReleaseFastMutex(&CurInfo->CursorMutex);
          ObDereferenceObject(InputWindowStation);
          return FALSE;
        }
      CurInfo->SafetySwitch = TRUE;
      ExAcquireFastMutex(SurfGDI->DriverLock);
      SurfGDI->MovePointer(SurfObj, -1, -1, NULL);
      ExReleaseFastMutex(SurfGDI->DriverLock);
      ExReleaseFastMutex(&CurInfo->CursorMutex);
    }
    
  ObDereferenceObject(InputWindowStation);
  return(TRUE);
}

STATIC VOID FASTCALL
SetPointerRect(PSYSTEM_CURSORINFO CurInfo, PRECTL PointerRect)
{
  CurInfo->PointerRectLeft = PointerRect->left;
  CurInfo->PointerRectRight = PointerRect->right;
  CurInfo->PointerRectTop = PointerRect->top;
  CurInfo->PointerRectBottom = PointerRect->bottom;
}

INT FASTCALL
MouseSafetyOnDrawEnd(PSURFOBJ SurfObj, PSURFGDI SurfGDI)
/*
 * FUNCTION: Notify the mouse driver that drawing has finished on a surface.
 */
{
  PSYSTEM_CURSORINFO CurInfo;
  BOOL MouseEnabled = FALSE;
  RECTL PointerRect;
    
  if(IntGetWindowStationObject(InputWindowStation))
  {
    CurInfo = &InputWindowStation->SystemCursor;
  }
  else
    return FALSE;
    
  ExAcquireFastMutex(&CurInfo->CursorMutex);
  if(SurfObj == NULL)
  {
    ExReleaseFastMutex(&CurInfo->CursorMutex);
    ObDereferenceObject(InputWindowStation);
    return FALSE;
  }
  
  MouseEnabled = CurInfo->Enabled && CurInfo->ShowingCursor;

  if (SurfObj->iType != STYPE_DEVICE || MouseEnabled == FALSE)
    {
      ExReleaseFastMutex(&CurInfo->CursorMutex);
      ObDereferenceObject(InputWindowStation);
      return(FALSE);
    }

  if (SPS_ACCEPT_NOEXCLUDE == SurfGDI->PointerStatus)
    {
      /* Hardware cursor, it wasn't removed so need to restore it */
      ExReleaseFastMutex(&CurInfo->CursorMutex);
      ObDereferenceObject(InputWindowStation);
      return(FALSE);
    }
  
  if (CurInfo->SafetySwitch)
    {
      if (1 < CurInfo->SafetyRemoveCount--)
        {
          /* Someone else removed it too, let them restore it */
          ExReleaseFastMutex(&CurInfo->CursorMutex);
          ObDereferenceObject(InputWindowStation);
          return FALSE;
        }
      ExAcquireFastMutex(SurfGDI->DriverLock);
      SurfGDI->MovePointer(SurfObj, CurInfo->x, CurInfo->y, &PointerRect);
      ExReleaseFastMutex(SurfGDI->DriverLock);
      SetPointerRect(CurInfo, &PointerRect);
      CurInfo->SafetySwitch = FALSE;
    }

  ExReleaseFastMutex(&CurInfo->CursorMutex);
  ObDereferenceObject(InputWindowStation);
  return(TRUE);
}

BOOL FASTCALL
MouseMoveCursor(LONG X, LONG Y)
{
  HDC hDC;
  PDC dc;
  BOOL res = FALSE;
  PSURFOBJ SurfObj;
  PSURFGDI SurfGDI;
  PSYSTEM_CURSORINFO CurInfo;
  MSG Msg;
  LARGE_INTEGER LargeTickCount;
  ULONG TickCount;
  RECTL PointerRect;
  
  if(!InputWindowStation)
    return FALSE;
  
  if(IntGetWindowStationObject(InputWindowStation))
  {
    CurInfo = &InputWindowStation->SystemCursor;
    if(!CurInfo->Enabled)
    {
      ObDereferenceObject(InputWindowStation);
      return FALSE;
    }
    hDC = IntGetScreenDC();
    if(!hDC)
    {
      ObDereferenceObject(InputWindowStation);
      return FALSE;
    }
    dc = DC_LockDc(hDC);
    SurfObj = (PSURFOBJ)AccessUserObject((ULONG) dc->Surface);
    SurfGDI = (PSURFGDI)AccessInternalObject((ULONG) dc->Surface);
    DC_UnlockDc( hDC );
    IntCheckClipCursor(&X, &Y, CurInfo);
    if((X != CurInfo->x) || (Y != CurInfo->y))
    {
      /* move cursor */
      CurInfo->x = X;
      CurInfo->y = Y;
      if(CurInfo->Enabled)
      {
        ExAcquireFastMutex(&CurInfo->CursorMutex);
        ExAcquireFastMutex(SurfGDI->DriverLock);
        SurfGDI->MovePointer(SurfObj, CurInfo->x, CurInfo->y, &PointerRect);
        ExReleaseFastMutex(SurfGDI->DriverLock);
        SetPointerRect(CurInfo, &PointerRect);
        ExReleaseFastMutex(&CurInfo->CursorMutex);
      }
      /* send MOUSEMOVE message */
      KeQueryTickCount(&LargeTickCount);
      TickCount = LargeTickCount.u.LowPart;
      Msg.wParam = CurInfo->ButtonsDown;
      Msg.lParam = MAKELPARAM(X, Y);
      Msg.message = WM_MOUSEMOVE;
      Msg.time = TickCount;
      Msg.pt.x = X;
      Msg.pt.y = Y;
      MsqInsertSystemMessage(&Msg, TRUE);
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
  BOOL MouseEnabled = FALSE;
  BOOL Moved = FALSE;
  LONG mouse_ox, mouse_oy;
  LONG mouse_cx = 0, mouse_cy = 0;
  LONG dScroll = 0;
  HDC hDC;
  PDC dc;
  PSURFOBJ SurfObj;
  PSURFGDI SurfGDI;
  MSG Msg;
  RECTL PointerRect;
  
  hDC = IntGetScreenDC();
  
  if(!hDC || !InputWindowStation)
    return;

  if(IntGetWindowStationObject(InputWindowStation))
  {
    CurInfo = &InputWindowStation->SystemCursor;
    MouseEnabled = CurInfo->Enabled;
    if(!MouseEnabled)
    {
      ObDereferenceObject(InputWindowStation);
      return;
    }
    mouse_ox = CurInfo->x;
    mouse_oy = CurInfo->y;
  }
  else
    return;

  dc = DC_LockDc(hDC);
  SurfObj = (PSURFOBJ)AccessUserObject((ULONG) dc->Surface);
  SurfGDI = (PSURFGDI)AccessInternalObject((ULONG) dc->Surface);
  DC_UnlockDc( hDC );

  /* Compile the total mouse movement change and dispatch button events. */
  for (i = 0; i < InputCount; i++)
  {
    mouse_cx += Data[i].LastX;
    mouse_cy += Data[i].LastY;
    
    CurInfo->x += Data[i].LastX;
    CurInfo->y += Data[i].LastY;
    
    CurInfo->x = max(CurInfo->x, 0);
    CurInfo->y = max(CurInfo->y, 0);
    CurInfo->x = min(CurInfo->x, SurfObj->sizlBitmap.cx - 1);
    CurInfo->y = min(CurInfo->y, SurfObj->sizlBitmap.cy - 1);
    
    IntCheckClipCursor(&CurInfo->x, &CurInfo->y, CurInfo);

    Msg.wParam = CurInfo->ButtonsDown;
    Msg.lParam = MAKELPARAM(CurInfo->x, CurInfo->y);
    Msg.message = WM_MOUSEMOVE;
    Msg.pt.x = CurInfo->x;
    Msg.pt.y = CurInfo->y;
    
    if (Data[i].ButtonFlags != 0)
    {
      if ((Data[i].ButtonFlags & MOUSE_LEFT_BUTTON_DOWN) > 0)
      {
      	CurInfo->ButtonsDown |= (CurInfo->SwapButtons ? MK_RBUTTON : MK_LBUTTON);
        Msg.message = (CurInfo->SwapButtons ? WM_RBUTTONDOWN : WM_LBUTTONDOWN);
      }
      if ((Data[i].ButtonFlags & MOUSE_MIDDLE_BUTTON_DOWN) > 0)
      {
      	CurInfo->ButtonsDown |= MK_MBUTTON;
        Msg.message = WM_MBUTTONDOWN;
      }
      if ((Data[i].ButtonFlags & MOUSE_RIGHT_BUTTON_DOWN) > 0)
      {
      	CurInfo->ButtonsDown |= (CurInfo->SwapButtons ? MK_LBUTTON : MK_RBUTTON);
        Msg.message = (CurInfo->SwapButtons ? WM_LBUTTONDOWN : WM_RBUTTONDOWN);
      }
      
      if ((Data[i].ButtonFlags & MOUSE_BUTTON_4_DOWN) > 0)
      {
      	CurInfo->ButtonsDown |= MK_XBUTTON1;
        Msg.message = WM_XBUTTONDOWN;
      }
      if ((Data[i].ButtonFlags & MOUSE_BUTTON_5_DOWN) > 0)
      {
      	CurInfo->ButtonsDown |= MK_XBUTTON2;
        Msg.message = WM_XBUTTONDOWN;
      }

      if ((Data[i].ButtonFlags & MOUSE_LEFT_BUTTON_UP) > 0)
      {
      	CurInfo->ButtonsDown &= (CurInfo->SwapButtons ? ~MK_RBUTTON : ~MK_LBUTTON);
        Msg.message = (CurInfo->SwapButtons ? WM_RBUTTONUP : WM_LBUTTONUP);
      }
      if ((Data[i].ButtonFlags & MOUSE_MIDDLE_BUTTON_UP) > 0)
      {
      	CurInfo->ButtonsDown &= ~MK_MBUTTON;
        Msg.message = WM_MBUTTONUP;
      }
      if ((Data[i].ButtonFlags & MOUSE_RIGHT_BUTTON_UP) > 0)
      {
      	CurInfo->ButtonsDown &= (CurInfo->SwapButtons ? ~MK_LBUTTON : ~MK_RBUTTON);
        Msg.message = (CurInfo->SwapButtons ? WM_LBUTTONUP : WM_RBUTTONUP);
      }
      if ((Data[i].ButtonFlags & MOUSE_BUTTON_4_UP) > 0)
      {
      	CurInfo->ButtonsDown &= ~MK_XBUTTON1;
        Msg.message = WM_XBUTTONUP;
      }
      if ((Data[i].ButtonFlags & MOUSE_BUTTON_5_UP) > 0)
      {
      	CurInfo->ButtonsDown &= ~MK_XBUTTON2;
        Msg.message = WM_XBUTTONUP;
      }
      if ((Data[i].ButtonFlags & MOUSE_WHEEL) > 0)
      {
        dScroll += (LONG)Data[i].ButtonData;
      }
      
      if (Data[i].ButtonFlags != MOUSE_WHEEL)
      {
        Moved = (0 != mouse_cx) || (0 != mouse_cy);
        if(Moved && MouseEnabled)
        {
          if (!CurInfo->SafetySwitch && 0 == CurInfo->SafetyRemoveCount &&
              ((mouse_ox != CurInfo->x) || (mouse_oy != CurInfo->y)))
          {
            ExAcquireFastMutex(&CurInfo->CursorMutex);
            ExAcquireFastMutex(SurfGDI->DriverLock);
            SurfGDI->MovePointer(SurfObj, CurInfo->x, CurInfo->y, &PointerRect);
            ExReleaseFastMutex(SurfGDI->DriverLock);
            SetPointerRect(CurInfo, &PointerRect);
            ExReleaseFastMutex(&CurInfo->CursorMutex);
            mouse_cx = 0;
            mouse_cy = 0;
          }
        }
        
        Msg.wParam = CurInfo->ButtonsDown;
        MsqInsertSystemMessage(&Msg, FALSE);
      }
    }
  }

  /* If the mouse moved then move the pointer. */
  if ((mouse_cx != 0 || mouse_cy != 0) && MouseEnabled)
  {
    Msg.wParam = CurInfo->ButtonsDown;
    Msg.message = WM_MOUSEMOVE;
    Msg.pt.x = CurInfo->x;
    Msg.pt.y = CurInfo->y;
    Msg.lParam = MAKELPARAM(CurInfo->x, CurInfo->y);
    MsqInsertSystemMessage(&Msg, TRUE);
    
    if (!CurInfo->SafetySwitch && 0 == CurInfo->SafetyRemoveCount &&
        ((mouse_ox != CurInfo->x) || (mouse_oy != CurInfo->y)))
    {
      ExAcquireFastMutex(&CurInfo->CursorMutex);
      ExAcquireFastMutex(SurfGDI->DriverLock);
      SurfGDI->MovePointer(SurfObj, CurInfo->x, CurInfo->y, &PointerRect);
      ExReleaseFastMutex(SurfGDI->DriverLock);
      SetPointerRect(CurInfo, &PointerRect);
      ExReleaseFastMutex(&CurInfo->CursorMutex);
    }
  }
  
  /* send WM_MOUSEWHEEL message */
  if(dScroll && MouseEnabled)
  {
    Msg.message = WM_MOUSEWHEEL;
    Msg.wParam = MAKEWPARAM(CurInfo->ButtonsDown, dScroll);
    Msg.lParam = MAKELPARAM(CurInfo->x, CurInfo->y);
    Msg.pt.x = CurInfo->x;
    Msg.pt.y = CurInfo->y;
    MsqInsertSystemMessage(&Msg, FALSE);
  }

  ObDereferenceObject(InputWindowStation);
}

VOID FASTCALL
EnableMouse(HDC hDisplayDC)
{
  PDC dc;
  PSURFOBJ SurfObj;
  PSURFGDI SurfGDI;

  if( hDisplayDC && InputWindowStation)
  {
    if(!IntGetWindowStationObject(InputWindowStation))
    {
       InputWindowStation->SystemCursor.Enabled = FALSE;
       return;
    }
    
    dc = DC_LockDc(hDisplayDC);
    SurfObj = (PSURFOBJ)AccessUserObject((ULONG) dc->Surface);
    SurfGDI = (PSURFGDI)AccessInternalObject((ULONG) dc->Surface);
    DC_UnlockDc( hDisplayDC );
    
    IntSetCursor(InputWindowStation, NULL, TRUE);
    
    InputWindowStation->SystemCursor.Enabled = (SPS_ACCEPT_EXCLUDE == SurfGDI->PointerStatus ||
                                                SPS_ACCEPT_NOEXCLUDE == SurfGDI->PointerStatus);
    
    /* Move the cursor to the screen center */
    DPRINT("Setting Cursor up at 0x%x, 0x%x\n", SurfObj->sizlBitmap.cx / 2, SurfObj->sizlBitmap.cy / 2);
    MouseMoveCursor(SurfObj->sizlBitmap.cx / 2, SurfObj->sizlBitmap.cy / 2);

    ObDereferenceObject(InputWindowStation);
  }
  else
  {
    if(IntGetWindowStationObject(InputWindowStation))
    {
       IntSetCursor(InputWindowStation, NULL, TRUE);
       InputWindowStation->SystemCursor.Enabled = FALSE;
       InputWindowStation->SystemCursor.CursorClipInfo.IsClipped = FALSE;
	   ObDereferenceObject(InputWindowStation);
       return;
    }
  }
}

ULONG
STDCALL
EngSetPointerShape(
	IN SURFOBJ  *pso,
	IN SURFOBJ  *psoMask,
	IN SURFOBJ  *psoColor,
	IN XLATEOBJ  *pxlo,
	IN LONG  xHot,
	IN LONG  yHot,
	IN LONG  x,
	IN LONG  y,
	IN RECTL  *prcl,
	IN FLONG  fl
	)
{
  // www.osr.com/ddk/graphics/gdifncs_1y5j.htm
  UNIMPLEMENTED;
  return 0;
}

/*
 * @unimplemented
 */
VOID
STDCALL
EngMovePointer(
	IN SURFOBJ  *pso,
	IN LONG      x,
	IN LONG      y,
	IN RECTL    *prcl
	)
{
  // www.osr.com/ddk/graphics/gdifncs_8wfb.htm
  UNIMPLEMENTED;
}

/* EOF */
