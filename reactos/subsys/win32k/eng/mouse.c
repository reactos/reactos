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
/* $Id: mouse.c,v 1.68 2004/05/01 08:55:04 weiden Exp $
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
#include <ddk/winddi.h>
#include <win32k/dc.h>
#include "objects.h"
#include "include/msgqueue.h"
#include "include/object.h"
#include "include/winsta.h"
#include "include/desktop.h"
#include "include/window.h"
#include "include/cursoricon.h"
#include "include/callback.h"
#include "include/palette.h"
#include "include/inteng.h"
#include "include/eng.h"
#include "include/tags.h"
#include <include/mouse.h>
#include <include/input.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

BOOL FASTCALL
IntIsPrimarySurface(PSURFGDI SurfGDI);

VOID FASTCALL
EnableMouse(HDC hDisplayDC)
{
  PDC dc;
  SURFOBJ *SurfObj;
  PSURFGDI SurfGDI;

  if( hDisplayDC && InputWindowStation)
  {
    if(!IntGetWindowStationObject(InputWindowStation))
    {
       InputWindowStation->SystemCursor.Enabled = FALSE;
       return;
    }
    
    dc = DC_LockDc(hDisplayDC);
    SurfObj = (SURFOBJ*)AccessUserObject((ULONG) dc->Surface);
    SurfGDI = (PSURFGDI)AccessInternalObject((ULONG) dc->Surface);
    DC_UnlockDc( hDisplayDC );
    
    /* Move the cursor to the screen center */
    DPRINT("Setting Cursor up at 0x%x, 0x%x\n", SurfObj->sizlBitmap.cx / 2, SurfObj->sizlBitmap.cy / 2);
    ExAcquireFastMutex(&InputWindowStation->SystemCursor.CursorMutex);
    InputWindowStation->SystemCursor.x = SurfObj->sizlBitmap.cx / 2;
    InputWindowStation->SystemCursor.y = SurfObj->sizlBitmap.cy / 2;
    ExReleaseFastMutex(&InputWindowStation->SystemCursor.CursorMutex);
    IntSetCursor(InputWindowStation, NULL, TRUE);
    
    InputWindowStation->SystemCursor.Enabled = (SPS_ACCEPT_EXCLUDE == SurfGDI->PointerStatus ||
                                                SPS_ACCEPT_NOEXCLUDE == SurfGDI->PointerStatus);
    
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

INT FASTCALL
MouseSafetyOnDrawStart(SURFOBJ *SurfObj, PSURFGDI SurfGDI, LONG HazardX1,
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
  if (!IntIsPrimarySurface(SurfGDI) || MouseEnabled == FALSE)
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
      IntLockGDIDriver(SurfGDI);
      SurfGDI->MovePointer(SurfObj, -1, -1, NULL);
      IntUnLockGDIDriver(SurfGDI);
      ExReleaseFastMutex(&CurInfo->CursorMutex);
    }
    
  ObDereferenceObject(InputWindowStation);
  return(TRUE);
}

VOID FASTCALL
SetPointerRect(PSYSTEM_CURSORINFO CurInfo, PRECTL PointerRect)
{
  CurInfo->PointerRectLeft = PointerRect->left;
  CurInfo->PointerRectRight = PointerRect->right;
  CurInfo->PointerRectTop = PointerRect->top;
  CurInfo->PointerRectBottom = PointerRect->bottom;
}

INT FASTCALL
MouseSafetyOnDrawEnd(SURFOBJ *SurfObj, SURFGDI *SurfGDI)
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
  if (!IntIsPrimarySurface(SurfGDI) || MouseEnabled == FALSE)
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
      IntLockGDIDriver(SurfGDI);
      SurfGDI->MovePointer(SurfObj, CurInfo->x, CurInfo->y, &PointerRect);
      IntUnLockGDIDriver(SurfGDI);
      SetPointerRect(CurInfo, &PointerRect);
      CurInfo->SafetySwitch = FALSE;
    }

  ExReleaseFastMutex(&CurInfo->CursorMutex);
  ObDereferenceObject(InputWindowStation);
  return(TRUE);
}

#define ClearMouseInput(mi) \
  mi.dx = 0; \
  mi.dy = 0; \
  mi.mouseData = 0; \
  mi.dwFlags = 0;

#define SendMouseEvent(mi) \
  if(mi.dx != 0 || mi.dy != 0) \
    mi.dwFlags |= MOUSEEVENTF_MOVE; \
  if(mi.dwFlags) \
    IntMouseInput(&mi); \
  ClearMouseInput(mi);

VOID /* STDCALL */
MouseGDICallBack(PMOUSE_INPUT_DATA Data, ULONG InputCount)
{
  PMOUSE_INPUT_DATA mid;
  MOUSEINPUT mi;
  ULONG i;
  
  ClearMouseInput(mi);
  mi.time = 0;
  mi.dwExtraInfo = 0;
  for(i = 0; i < InputCount; i++)
  {
    mid = (Data + i);
    mi.dx += mid->LastX;
    mi.dy += mid->LastY;
    
    if(mid->ButtonFlags)
    {
      if(mid->ButtonFlags & MOUSE_LEFT_BUTTON_DOWN)
      {
        mi.dwFlags |= MOUSEEVENTF_LEFTDOWN;
        SendMouseEvent(mi);
      }
      if(mid->ButtonFlags & MOUSE_LEFT_BUTTON_UP)
      {
        mi.dwFlags |= MOUSEEVENTF_LEFTUP;
        SendMouseEvent(mi);
      }
      if(mid->ButtonFlags & MOUSE_MIDDLE_BUTTON_DOWN)
      {
        mi.dwFlags |= MOUSEEVENTF_MIDDLEDOWN;
        SendMouseEvent(mi);
      }
      if(mid->ButtonFlags & MOUSE_MIDDLE_BUTTON_UP)
      {
        mi.dwFlags |= MOUSEEVENTF_MIDDLEUP;
        SendMouseEvent(mi);
      }
      if(mid->ButtonFlags & MOUSE_RIGHT_BUTTON_DOWN)
      {
        mi.dwFlags |= MOUSEEVENTF_RIGHTDOWN;
        SendMouseEvent(mi);
      }
      if(mid->ButtonFlags & MOUSE_RIGHT_BUTTON_UP)
      {
        mi.dwFlags |= MOUSEEVENTF_RIGHTUP;
        SendMouseEvent(mi);
      }
      if(mid->ButtonFlags & MOUSE_BUTTON_4_DOWN)
      {
        mi.mouseData |= XBUTTON1;
        mi.dwFlags |= MOUSEEVENTF_XDOWN;
        SendMouseEvent(mi);
      }
      if(mid->ButtonFlags & MOUSE_BUTTON_4_UP)
      {
        mi.mouseData |= XBUTTON1;
        mi.dwFlags |= MOUSEEVENTF_XUP;
        SendMouseEvent(mi);
      }
      if(mid->ButtonFlags & MOUSE_BUTTON_5_DOWN)
      {
        mi.mouseData |= XBUTTON2;
        mi.dwFlags |= MOUSEEVENTF_XDOWN;
        SendMouseEvent(mi);
      }
      if(mid->ButtonFlags & MOUSE_BUTTON_5_UP)
      {
        mi.mouseData |= XBUTTON2;
        mi.dwFlags |= MOUSEEVENTF_XUP;
        SendMouseEvent(mi);
      }
      if(mid->ButtonFlags & MOUSE_WHEEL)
      {
        mi.mouseData = mid->ButtonData;
        mi.dwFlags |= MOUSEEVENTF_WHEEL;
        SendMouseEvent(mi);
      }
    }
  }
  
  SendMouseEvent(mi);
}

/* SOFTWARE MOUSE POINTER IMPLEMENTATION **************************************/

VOID FASTCALL
IntHideMousePointer(GDIDEVICE *ppdev, SURFOBJ *DestSurface)
{
   if (ppdev->PointerAttributes.Enable == FALSE)
   {
      return;
   }

   ppdev->PointerAttributes.Enable = FALSE;

   if (ppdev->PointerAttributes.Column + ppdev->PointerHotSpot.x == -1)
   {
      return;
   }

   if (ppdev->PointerSaveSurface != NULL)
   {
      RECTL DestRect;
      POINTL SrcPoint;
      SURFOBJ *SaveSurface;
      SURFOBJ *MaskSurface;

      DestRect.left = max(ppdev->PointerAttributes.Column, 0);
      DestRect.top = max(ppdev->PointerAttributes.Row, 0);
      DestRect.right = min(
         ppdev->PointerAttributes.Column + ppdev->PointerAttributes.Width,
         DestSurface->sizlBitmap.cx);
      DestRect.bottom = min(
         ppdev->PointerAttributes.Row + ppdev->PointerAttributes.Height,
         DestSurface->sizlBitmap.cy);

      SrcPoint.x = max(-ppdev->PointerAttributes.Column, 0);
      SrcPoint.y = max(-ppdev->PointerAttributes.Row, 0);

      SaveSurface = EngLockSurface(ppdev->PointerSaveSurface);
      MaskSurface = EngLockSurface(ppdev->PointerMaskSurface);
      EngBitBlt(DestSurface, SaveSurface, MaskSurface, NULL, NULL,
                &DestRect, &SrcPoint, &SrcPoint, NULL, NULL, SRCCOPY);
      EngUnlockSurface(MaskSurface);
      EngUnlockSurface(SaveSurface);
   }
}

VOID FASTCALL
IntShowMousePointer(GDIDEVICE *ppdev, SURFOBJ *DestSurface)
{
   if (ppdev->PointerAttributes.Enable == TRUE)
   {
      return;
   }

   ppdev->PointerAttributes.Enable = TRUE;

   /*
    * Copy the pixels under the cursor to temporary surface.
    */
   
   if (ppdev->PointerSaveSurface != NULL)
   {
      RECTL DestRect;
      POINTL SrcPoint;
      SURFOBJ *SaveSurface;

      SrcPoint.x = max(ppdev->PointerAttributes.Column, 0);
      SrcPoint.y = max(ppdev->PointerAttributes.Row, 0);

      DestRect.left = SrcPoint.x - ppdev->PointerAttributes.Column;
      DestRect.top = SrcPoint.y - ppdev->PointerAttributes.Row;
      DestRect.right = min(
         ppdev->PointerAttributes.Width,
         DestSurface->sizlBitmap.cx - ppdev->PointerAttributes.Column);
      DestRect.bottom = min(
         ppdev->PointerAttributes.Height,
         DestSurface->sizlBitmap.cy - ppdev->PointerAttributes.Row);

      SaveSurface = EngLockSurface(ppdev->PointerSaveSurface);
      EngBitBlt(SaveSurface, DestSurface, NULL, NULL, NULL,
                &DestRect, &SrcPoint, NULL, NULL, NULL, SRCCOPY);
      EngUnlockSurface(SaveSurface);
   }

   /*
    * Blit the cursor on the screen.
    */

   {
      RECTL DestRect;
      POINTL SrcPoint;
      SURFOBJ *ColorSurf;
      SURFOBJ *MaskSurf;

      DestRect.left = max(ppdev->PointerAttributes.Column, 0);
      DestRect.top = max(ppdev->PointerAttributes.Row, 0);
      DestRect.right = min(
         ppdev->PointerAttributes.Column + ppdev->PointerAttributes.Width,
         DestSurface->sizlBitmap.cx);
      DestRect.bottom = min(
         ppdev->PointerAttributes.Row + ppdev->PointerAttributes.Height,
         DestSurface->sizlBitmap.cy);

      SrcPoint.x = max(-ppdev->PointerAttributes.Column, 0);
      SrcPoint.y = max(-ppdev->PointerAttributes.Row, 0);

      MaskSurf = EngLockSurface(ppdev->PointerMaskSurface);
      if (ppdev->PointerColorSurface != NULL)
      {
         ColorSurf = EngLockSurface(ppdev->PointerColorSurface);
         EngBitBlt(DestSurface, ColorSurf, MaskSurf, NULL, ppdev->PointerXlateObject,
                   &DestRect, &SrcPoint, &SrcPoint, NULL, NULL, 0xAACC);
         EngUnlockSurface(ColorSurf);
      }
      else
      {
         EngBitBlt(DestSurface, MaskSurf, NULL, NULL, ppdev->PointerXlateObject,
                   &DestRect, &SrcPoint, NULL, NULL, NULL, SRCAND);
         SrcPoint.y += ppdev->PointerAttributes.Height;
         EngBitBlt(DestSurface, MaskSurf, NULL, NULL, ppdev->PointerXlateObject,
                   &DestRect, &SrcPoint, NULL, NULL, NULL, SRCINVERT);
      }
      EngUnlockSurface(MaskSurf);
   }
}

/*
 * @implemented
 */

ULONG STDCALL
EngSetPointerShape(
   IN SURFOBJ *pso,
   IN SURFOBJ *psoMask,
   IN SURFOBJ *psoColor,
   IN XLATEOBJ *pxlo,
   IN LONG xHot,
   IN LONG yHot,
   IN LONG x,
   IN LONG y,
   IN RECTL *prcl,
   IN FLONG fl)
{
   GDIDEVICE *ppdev = (GDIDEVICE *)pso->hdev;
   SURFOBJ *TempSurfObj;
   
   IntHideMousePointer(ppdev, pso);

   if (ppdev->PointerColorSurface != NULL)
   {
      /* FIXME: Is this really needed? */
      TempSurfObj = EngLockSurface(ppdev->PointerColorSurface);
      EngFreeMem(TempSurfObj->pvBits);
      TempSurfObj->pvBits = 0;
      EngUnlockSurface(TempSurfObj);

      EngDeleteSurface(ppdev->PointerColorSurface);
      ppdev->PointerMaskSurface = NULL;
   }

   if (ppdev->PointerMaskSurface != NULL)
   {
      /* FIXME: Is this really needed? */
      TempSurfObj = EngLockSurface(ppdev->PointerMaskSurface);
      EngFreeMem(TempSurfObj->pvBits);
      TempSurfObj->pvBits = 0;
      EngUnlockSurface(TempSurfObj);

      EngDeleteSurface(ppdev->PointerMaskSurface);
      ppdev->PointerMaskSurface = NULL;
   }

   if (ppdev->PointerSaveSurface != NULL)
   {
      EngDeleteSurface(ppdev->PointerSaveSurface);
      ppdev->PointerSaveSurface = NULL;
   }

   if (ppdev->PointerXlateObject != NULL)
   {
      EngDeleteXlate(ppdev->PointerXlateObject);
      ppdev->PointerXlateObject = NULL;
   }

   /*
    * See if we are being asked to hide the pointer.
    */

   if (psoMask == NULL)
   {
      return SPS_ACCEPT_NOEXCLUDE;
   }

   ppdev->PointerHotSpot.x = xHot;
   ppdev->PointerHotSpot.y = yHot;

   ppdev->PointerAttributes.Column = x - xHot;
   ppdev->PointerAttributes.Row = y - yHot;
   ppdev->PointerAttributes.Width = psoMask->lDelta << 3;
   ppdev->PointerAttributes.Height = (psoMask->cjBits / psoMask->lDelta) >> 1;

   if (prcl != NULL)
   {
      prcl->left = ppdev->PointerAttributes.Column;
      prcl->top = ppdev->PointerAttributes.Row;
      prcl->right = prcl->left + ppdev->PointerAttributes.Width;
      prcl->bottom = prcl->top + ppdev->PointerAttributes.Height;
   }

   if (psoColor != NULL)
   {
      SIZEL Size;
      PBYTE Bits;

      Size.cx = ppdev->PointerAttributes.Width;
      Size.cy = ppdev->PointerAttributes.Height;
      Bits = EngAllocMem(0, psoColor->cjBits, TAG_MOUSE);
      memcpy(Bits, psoColor->pvBits, psoColor->cjBits);

      ppdev->PointerColorSurface = (HSURF)EngCreateBitmap(Size,
         psoColor->lDelta, psoColor->iBitmapFormat, 0, Bits);
   }
   else
   {
      ppdev->PointerColorSurface = NULL;
   }

   {
      SIZEL Size;
      PBYTE Bits;

      Size.cx = ppdev->PointerAttributes.Width;
      Size.cy = ppdev->PointerAttributes.Height << 1;
      Bits = EngAllocMem(0, psoMask->cjBits, TAG_MOUSE);
      memcpy(Bits, psoMask->pvBits, psoMask->cjBits);

      ppdev->PointerMaskSurface = (HSURF)EngCreateBitmap(Size,
         psoMask->lDelta, psoMask->iBitmapFormat, 0, Bits);
   }

   /*
    * Create and XLATEOBJ that will be used for drawing masks.
    * FIXME: We should get this in pxlo parameter!
    */

   if (pxlo == NULL)
   {
      HPALETTE BWPalette, DestPalette;
      ULONG BWColors[] = {0, 0xFFFFFF};
      PDC Dc;
      PPALGDI PalObj;
      LONG DestMode;

      BWPalette = EngCreatePalette(PAL_INDEXED, sizeof(BWColors) / sizeof(ULONG),
         BWColors, 0, 0, 0);
      Dc = DC_LockDc(IntGetScreenDC());
      DestPalette = Dc->w.hPalette;
      PalObj = PALETTE_LockPalette(DestPalette);
      DestMode = PalObj->Mode;
      PALETTE_UnlockPalette(DestPalette);
      DC_UnlockDc(IntGetScreenDC());
      ppdev->PointerXlateObject = IntEngCreateXlate(DestMode, PAL_INDEXED,
         DestPalette, BWPalette);
      EngDeletePalette(BWPalette);
   }
   else
   {
      ppdev->PointerXlateObject = pxlo;
   }

   /*
    * Create surface for saving the pixels under the cursor.
    */

   {
      SIZEL Size;
      LONG lDelta;

      Size.cx = ppdev->PointerAttributes.Width;
      Size.cy = ppdev->PointerAttributes.Height;

      switch (pso->iBitmapFormat)
      {
         case BMF_1BPP: lDelta = Size.cx >> 3; break;
         case BMF_4BPP: lDelta = Size.cx >> 1; break;
         case BMF_8BPP: lDelta = Size.cx; break;
         case BMF_16BPP: lDelta = Size.cx << 1; break;
         case BMF_24BPP: lDelta = Size.cx * 3; break; 
         case BMF_32BPP: lDelta = Size.cx << 2; break;
      }

      ppdev->PointerSaveSurface = (HSURF)EngCreateBitmap(
         Size, lDelta, pso->iBitmapFormat, BMF_NOZEROINIT, NULL);
   }

   IntShowMousePointer(ppdev, pso);

   return SPS_ACCEPT_EXCLUDE;
}

/*
 * @implemented
 */

VOID STDCALL
EngMovePointer(
   IN SURFOBJ *pso,
   IN LONG x,
   IN LONG y,
   IN RECTL *prcl)
{
   GDIDEVICE *ppdev = (GDIDEVICE *)pso->hdev;

   IntHideMousePointer(ppdev, pso);
   ppdev->PointerAttributes.Column = x - ppdev->PointerHotSpot.x;
   ppdev->PointerAttributes.Row = y - ppdev->PointerHotSpot.y;
   if (x != -1)
   {
      IntShowMousePointer(ppdev, pso);
   }

   if (prcl != NULL)
   {
      prcl->left = ppdev->PointerAttributes.Column;
      prcl->top = ppdev->PointerAttributes.Row;
      prcl->right = prcl->left + ppdev->PointerAttributes.Width;
      prcl->bottom = prcl->top + ppdev->PointerAttributes.Height;
   }
}

/* EOF */
