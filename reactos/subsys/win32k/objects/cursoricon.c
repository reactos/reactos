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
/* $Id: cursoricon.c,v 1.63.2.2 2004/09/12 19:21:08 weiden Exp $ */
#include <w32k.h>

#define COLORCURSORS_ALLOWED FALSE
PCURSOR_OBJECT FASTCALL
IntSetCursor(PCURSOR_OBJECT NewCursor, BOOL ForceChange)
{
   BITMAPOBJ *BitmapObj;
   SURFOBJ *SurfObj;
   PDEVINFO DevInfo;
   PBITMAPOBJ MaskBmpObj = NULL;
   PSYSTEM_CURSORINFO CurInfo;
   PCURSOR_OBJECT OldCursor;
   HBITMAP hColor = (HBITMAP)0;
   HBITMAP hMask = 0;
   SURFOBJ *soMask = NULL, *soColor = NULL;
   XLATEOBJ *XlateObj = NULL;
   RECTL PointerRect;
   HDC Screen;
  
   CurInfo = IntGetSysCursorInfo(PsGetWin32Process()->WindowStation);
   OldCursor = CurInfo->CurrentCursorObject;
   
   if (!ForceChange && OldCursor == NewCursor)
   {
      return OldCursor;
   }
   else
   {
      if(!(Screen = IntGetScreenDC()))
      {
        return NULL;
      }
      /* FIXME use the desktop's HDC instead of using ScreenDeviceContext */
      PDC dc = DC_LockDc(Screen);

      if (!dc)
      {
         return OldCursor;
      }
  
      BitmapObj = BITMAPOBJ_LockBitmap(dc->w.hBitmap);
      /* FIXME - BitmapObj can be NULL!!!!! */
      SurfObj = &BitmapObj->SurfObj;
      DevInfo = dc->DevInfo;
      DC_UnlockDc(dc);
   }
  
   if (!NewCursor && (CurInfo->CurrentCursorObject || ForceChange))
   {
      if (NULL != CurInfo->CurrentCursorObject && CurInfo->ShowingCursor)
      {
         /* Remove the cursor if it was displayed */
         if (GDIDEV(SurfObj)->MovePointer)
           GDIDEV(SurfObj)->MovePointer(SurfObj, -1, -1, &PointerRect);
         SetPointerRect(CurInfo, &PointerRect);
      }
      
      GDIDEV(SurfObj)->PointerStatus = SPS_ACCEPT_NOEXCLUDE;

      CurInfo->CurrentCursorObject = NewCursor; /* i.e. CurrentCursorObject = NULL */
      CurInfo->ShowingCursor = 0;
      BITMAPOBJ_UnlockBitmap(BitmapObj);
      return OldCursor;
   }
  
   if (!NewCursor)
   {
      BITMAPOBJ_UnlockBitmap(BitmapObj);
      return OldCursor;
   }

   /* TODO: Fixme. Logic is screwed above */

   ASSERT(NewCursor);
   MaskBmpObj = BITMAPOBJ_LockBitmap(NewCursor->IconInfo.hbmMask);
   if (MaskBmpObj)
   {
      const int maskBpp = BitsPerFormat(MaskBmpObj->SurfObj.iBitmapFormat);
      BITMAPOBJ_UnlockBitmap(MaskBmpObj);
      if (maskBpp != 1)
      {
         BITMAPOBJ_UnlockBitmap(BitmapObj);
         DPRINT1("SetCursor: The Mask bitmap must have 1BPP!\n");
         return OldCursor;
      }
      
      if ((DevInfo->flGraphicsCaps2 & GCAPS2_ALPHACURSOR) && 
          SurfObj->iBitmapFormat >= BMF_16BPP &&
          SurfObj->iBitmapFormat <= BMF_32BPP &&
          NewCursor->Shadow && COLORCURSORS_ALLOWED)
      {
        /* FIXME - Create a color pointer, only 32bit bitmap, set alpha bits!
                   Do not pass a mask bitmap to DrvSetPointerShape()!
                   Create a XLATEOBJ that describes the colors of the bitmap. */
        DPRINT1("SetCursor: (Colored) alpha cursors are not supported!\n");
      }
      else
      {
        if(NewCursor->IconInfo.hbmColor
           && COLORCURSORS_ALLOWED)
        {
          /* FIXME - Create a color pointer, create only one 32bit bitmap!
                     Do not pass a mask bitmap to DrvSetPointerShape()!
                     Create a XLATEOBJ that describes the colors of the bitmap.
                     (16bit bitmaps are propably allowed) */
          DPRINT1("SetCursor: Cursors with colors are not supported!\n");
        }
        else
        {
          MaskBmpObj = BITMAPOBJ_LockBitmap(NewCursor->IconInfo.hbmMask);
          if(MaskBmpObj)
          {
            RECTL DestRect = {0, 0, MaskBmpObj->SurfObj.sizlBitmap.cx, MaskBmpObj->SurfObj.sizlBitmap.cy};
            POINTL SourcePoint = {0, 0};
            
            /*
             * NOTE: For now we create the cursor in top-down bitmap,
             * because VMware driver rejects it otherwise. This should
             * be fixed later.
             */
            hMask = EngCreateBitmap(
              MaskBmpObj->SurfObj.sizlBitmap, abs(MaskBmpObj->SurfObj.lDelta),
              MaskBmpObj->SurfObj.iBitmapFormat, BMF_TOPDOWN,
              NULL);
            ASSERT(hMask);
            soMask = EngLockSurface((HSURF)hMask);
            EngCopyBits(soMask, &MaskBmpObj->SurfObj, NULL, NULL,
              &DestRect, &SourcePoint);
            BITMAPOBJ_UnlockBitmap(MaskBmpObj);
          }
        }
      }
      CurInfo->ShowingCursor = CURSOR_SHOWING;
      CurInfo->CurrentCursorObject = NewCursor;
    }
    else
    {
      CurInfo->ShowingCursor = 0;
      CurInfo->CurrentCursorObject = NULL;
    }
    
    if (GDIDEVFUNCS(SurfObj).SetPointerShape)
    {
      GDIDEV(SurfObj)->PointerStatus =
        GDIDEVFUNCS(SurfObj).SetPointerShape(SurfObj, soMask, soColor, XlateObj,
                                             NewCursor->IconInfo.xHotspot,
                                             NewCursor->IconInfo.yHotspot,
                                             CurInfo->x, 
                                             CurInfo->y, 
                                             &PointerRect,
                                             SPS_CHANGE);
      DPRINT("SetCursor: DrvSetPointerShape() returned %x\n", GDIDEV(SurfObj)->PointerStatus);
    }
    else
    {
      GDIDEV(SurfObj)->PointerStatus = SPS_DECLINE;
    }

    if(GDIDEV(SurfObj)->PointerStatus == SPS_DECLINE)
    {
      GDIDEV(SurfObj)->PointerStatus = EngSetPointerShape(
                         SurfObj, soMask, soColor, XlateObj,
                         NewCursor->IconInfo.xHotspot,
                         NewCursor->IconInfo.yHotspot,
                         CurInfo->x, 
                         CurInfo->y, 
                         &PointerRect,
                         SPS_CHANGE);
      GDIDEV(SurfObj)->MovePointer = EngMovePointer;
    }
    else
    {
      GDIDEV(SurfObj)->MovePointer = GDIDEVFUNCS(SurfObj).MovePointer;
    }
    
    SetPointerRect(CurInfo, &PointerRect);
    
    BITMAPOBJ_UnlockBitmap(BitmapObj);
    if(hMask)
    {
      EngUnlockSurface(soMask);
      EngDeleteSurface((HSURF)hMask);
    }
    if(hColor)
    {
      EngDeleteSurface((HSURF)hColor);
    }
    if(XlateObj)
    {
      EngDeleteXlate(XlateObj);
    }
    
    if(GDIDEV(SurfObj)->PointerStatus == SPS_ERROR)
    {
      DPRINT1("SetCursor: DrvSetPointerShape() returned SPS_ERROR\n");
    }
  
  return OldCursor;
}

typedef struct _INT_FINDEXISTINGCURSOR
{
  HMODULE hModule;
  HRSRC hRsrc;
  LONG cx;
  LONG cy;
} INT_FINDEXISTINGCURSOR, *PINT_FINDEXISTINGCURSOR;

BOOL FASTCALL
_FindExistingCursorObjectCallBack(PCURSOR_OBJECT CursorObject, PINT_FINDEXISTINGCURSOR FindData)
{
  if(CursorObject->hModule == FindData->hModule && CursorObject->hRsrc == FindData->hRsrc &&
     (FindData->cx == 0 || (FindData->cx == CursorObject->Size.cx && FindData->cy == CursorObject->Size.cy)))
  {
    /* We found the right object */
    return TRUE;
  }
  
  return FALSE;
}

PCURSOR_OBJECT FASTCALL
IntFindExistingCursorObject(HMODULE hModule, HRSRC hRsrc, LONG cx, LONG cy)
{
  INT_FINDEXISTINGCURSOR fec;
  
  fec.hModule = hModule;
  fec.hRsrc = hRsrc;
  fec.cx = cx;
  fec.cy = cy;
  
  return (PCURSOR_OBJECT)ObmEnumHandles(PsGetWin32Process()->WindowStation->HandleTable,
                                        otCURSOR, &fec, (PFNENUMHANDLESPROC)_FindExistingCursorObjectCallBack);
}

PCURSOR_OBJECT FASTCALL
IntCreateCursorObject(HANDLE *Handle)
{
  PCURSOR_OBJECT Object;
  PW32PROCESS Win32Process;
  
  ASSERT(Handle);
  
  Win32Process = PsGetWin32Process();
  
  Object = (PCURSOR_OBJECT)ObmCreateObject(Win32Process->WindowStation->HandleTable, 
                                           Handle, otCURSOR, sizeof(CURSOR_OBJECT));
  
  if(!Object)
  {
    SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
    return FALSE;
  }
  
  IntLockProcessCursorIcons(Win32Process);
  InsertTailList(&Win32Process->CursorIconListHead, &Object->ListEntry);
  IntUnLockProcessCursorIcons(Win32Process);
  
  Object->Handle = *Handle;
  Object->Process = PsGetWin32Process();
  
  return Object;
}

BOOL FASTCALL
IntDestroyCursorObject(PCURSOR_OBJECT Object, BOOL RemoveFromProcess)
{
  PSYSTEM_CURSORINFO CurInfo;
  HBITMAP bmpMask, bmpColor;
  PW32PROCESS Win32Process;
  
  Win32Process = PsGetWin32Process();
  
  ASSERT(Object);
  
  if (Object->Process != Win32Process)
  {
    return FALSE;
  }

  CurInfo = IntGetSysCursorInfo(Win32Process->WindowStation);

  ObmReferenceObject(Object);
  ObmDeleteObject(Win32Process->WindowStation->HandleTable, Object);
  
  if (CurInfo->CurrentCursorObject == Object)
  {
    /* Hide the cursor if we're destroying the current cursor */
    IntSetCursor(NULL, TRUE);
  }
  
  bmpMask = Object->IconInfo.hbmMask;
  bmpColor = Object->IconInfo.hbmColor;

  if (Object->Process && RemoveFromProcess)
  {
    IntLockProcessCursorIcons(Object->Process);
    RemoveEntryList(&Object->ListEntry);
    IntUnLockProcessCursorIcons(Object->Process);
  }
  
  /* delete bitmaps */
  if(bmpMask)
    NtGdiDeleteObject(bmpMask);
  if(bmpColor)
    NtGdiDeleteObject(bmpColor);
  
  ObmDereferenceObject(Object);
  
  return TRUE;
}

VOID FASTCALL
IntCleanupCurIcons(struct _EPROCESS *Process, PW32PROCESS Win32Process)
{
  PWINSTATION_OBJECT WinStaObject;
  PCURSOR_OBJECT Current;
  PLIST_ENTRY CurrentEntry, NextEntry;
  
  if(!(WinStaObject = Win32Process->WindowStation))
    return;
  
  /* FIXME - we have to lock the environment here! */
  
  IntLockProcessCursorIcons(Win32Process);
  CurrentEntry = Win32Process->CursorIconListHead.Flink;
  while(CurrentEntry != &Win32Process->CursorIconListHead)
  {
    NextEntry = CurrentEntry->Flink;
    Current = CONTAINING_RECORD(CurrentEntry, CURSOR_OBJECT, ListEntry);
    RemoveEntryList(&Current->ListEntry);
    IntDestroyCursorObject(Current, FALSE);
    CurrentEntry = NextEntry;
  }
  IntUnLockProcessCursorIcons(Win32Process);
}

VOID FASTCALL
IntGetCursorIconInfo(PCURSOR_OBJECT Cursor, PICONINFO IconInfo)
{
  ASSERT(Cursor);
  
  *IconInfo = Cursor->IconInfo;
  
  IconInfo->hbmMask = BITMAPOBJ_CopyBitmap(Cursor->IconInfo.hbmMask);
  IconInfo->hbmColor = BITMAPOBJ_CopyBitmap(Cursor->IconInfo.hbmColor);
}


BOOL FASTCALL
IntGetCursorIconSize(PCURSOR_OBJECT Cursor,
                     BOOL *fIcon,
                     SIZE *Size)
{
  PBITMAPOBJ bmp;
  HBITMAP hbmp;
  
  ASSERT(Cursor);
  ASSERT(fIcon);
  ASSERT(Size);
  
  *fIcon = Cursor->IconInfo.fIcon;
  
  hbmp = (Cursor->IconInfo.hbmColor != NULL ? Cursor->IconInfo.hbmColor : Cursor->IconInfo.hbmMask);
  if(hbmp == NULL || !(bmp = BITMAPOBJ_LockBitmap(hbmp)))
  {
    DPRINT1("Unable to lock bitmap 0x%x for cursor/icon 0x%x\n", hbmp, Cursor->Handle);
    return FALSE;
  }
  
  Size->cx = bmp->SurfObj.sizlBitmap.cx;
  Size->cy = (hbmp != Cursor->IconInfo.hbmMask ? bmp->SurfObj.sizlBitmap.cy : bmp->SurfObj.sizlBitmap.cy / 2);
  
  BITMAPOBJ_UnlockBitmap(bmp);
  
  return TRUE;
}


BOOL FASTCALL
IntGetCursorInfo(PCURSORINFO pci)
{
  PSYSTEM_CURSORINFO CurInfo;
  PWINSTATION_OBJECT WinSta;
  PCURSOR_OBJECT CursorObject;
  
  if(!(WinSta = PsGetWin32Process()->WindowStation))
  {
    DPRINT1("GetCursorInfo: Process isn't attached to a window station!\n");
    return FALSE;
  }
  
  CurInfo = IntGetSysCursorInfo(WinSta);
  CursorObject = CurInfo->CurrentCursorObject;
  
  pci->flags = ((CurInfo->ShowingCursor && CursorObject != NULL) ? CURSOR_SHOWING : 0);
  pci->hCursor = (CursorObject != NULL ? (HCURSOR)CursorObject->Handle : (HCURSOR)0);
  pci->ptScreenPos.x = CurInfo->x;
  pci->ptScreenPos.y = CurInfo->y;
  
  return TRUE;
}


VOID FASTCALL
IntGetClipCursor(RECT *lpRect)
{
  /* FIXME - check if process has WINSTA_READATTRIBUTES */
  PSYSTEM_CURSORINFO CurInfo;
  
  ASSERT(lpRect);
  
  CurInfo = IntGetSysCursorInfo(PsGetWin32Process()->WindowStation);
  if(CurInfo->CursorClipInfo.IsClipped)
  {
    lpRect->left = CurInfo->CursorClipInfo.Left;
    lpRect->top = CurInfo->CursorClipInfo.Top;
    lpRect->right = CurInfo->CursorClipInfo.Right;
    lpRect->bottom = CurInfo->CursorClipInfo.Bottom;
  }
  else
  {
    lpRect->left = 0;
    lpRect->top = 0;
    lpRect->right = IntGetSystemMetrics(SM_CXSCREEN);
    lpRect->bottom = IntGetSystemMetrics(SM_CYSCREEN);
  }
}


VOID FASTCALL
IntSetCursorIconData(PCURSOR_OBJECT Cursor, BOOL *fIcon, POINT *Hotspot,
                     HMODULE hModule, HRSRC hRsrc, HRSRC hGroupRsrc)
{
  ASSERT(Cursor);
  
  if(fIcon)
  {
    Cursor->IconInfo.fIcon = *fIcon;
  }
  if(Hotspot)
  {
    Cursor->IconInfo.xHotspot = Hotspot->x;
    Cursor->IconInfo.yHotspot = Hotspot->y;
  }
  
  Cursor->hModule = hModule;
  Cursor->hRsrc = hRsrc;
  Cursor->hGroupRsrc = hGroupRsrc;
}


#define CANSTRETCHBLT 0
BOOL FASTCALL
IntDrawIconEx(HDC hdc, int xLeft, int yTop, PCURSOR_OBJECT Cursor, int cxWidth, int cyWidth,
              UINT istepIfAniCur, HBRUSH hbrFlickerFreeDraw, UINT diFlags)
{
  HBITMAP hbmMask, hbmColor;
  BITMAP bmpMask, bmpColor;
  BOOL DoFlickerFree;
  SIZE IconSize;
  COLORREF oldFg, oldBg;
  HDC hdcMem, hdcOff = (HDC)0;
  HBITMAP hbmOff = (HBITMAP)0;
  HGDIOBJ hOldOffBrush = 0, hOldOffBmp = 0, hOldMem;
  BOOL Ret = FALSE;
  #if CANSTRETCHBLT
  INT nStretchMode;
  #endif
  
  ASSERT(Cursor);
  
  hbmMask = Cursor->IconInfo.hbmMask;
  hbmColor = Cursor->IconInfo.hbmColor;
  
  if(istepIfAniCur)
  {
    DPRINT1("NtUserDrawIconEx: istepIfAniCur is not supported!\n");
  }
  
  if(!hbmMask || !IntGdiGetObject(hbmMask, sizeof(BITMAP), &bmpMask))
    goto done;
  
  if(hbmColor && !IntGdiGetObject(hbmColor, sizeof(BITMAP), &bmpColor))
    goto done;
  
  if(hbmColor)
  {
    IconSize.cx = bmpColor.bmWidth;
    IconSize.cy = bmpColor.bmHeight;
  }
  else
  {
    IconSize.cx = bmpMask.bmWidth;
    IconSize.cy = bmpMask.bmHeight / 2;
  }
  
  if(!diFlags)
    diFlags = DI_NORMAL;
  
  if(!cxWidth)
    cxWidth = ((diFlags & DI_DEFAULTSIZE) ? IntGetSystemMetrics(SM_CXICON) : IconSize.cx);
  if(!cyWidth)
    cyWidth = ((diFlags & DI_DEFAULTSIZE) ? IntGetSystemMetrics(SM_CYICON) : IconSize.cy);
  
  DoFlickerFree = (hbrFlickerFreeDraw && (NtGdiGetObjectType(hbrFlickerFreeDraw) == OBJ_BRUSH));
  
  if(DoFlickerFree)
  {
    RECT r;
    r.right = cxWidth;
    r.bottom = cyWidth;
    
    hdcOff = NtGdiCreateCompatableDC(hdc);
    if(!hdcOff)
      goto done;
    
    hbmOff = NtGdiCreateCompatibleBitmap(hdc, cxWidth, cyWidth);
    if(!hbmOff)
    {
      NtGdiDeleteDC(hdcOff);
      goto done;
    }
    hOldOffBrush = NtGdiSelectObject(hdcOff, hbrFlickerFreeDraw);
    hOldOffBmp = NtGdiSelectObject(hdcOff, hbmOff);
    NtGdiPatBlt(hdcOff, 0, 0, r.right, r.bottom, PATCOPY);
    NtGdiSelectObject(hdcOff, hbmOff);
  }
  
  hdcMem = NtGdiCreateCompatableDC(hdc);
  if(!hdcMem)
    goto cleanup;
  
  if(!DoFlickerFree)
    hdcOff = hdc;
  
  #if CANSTRETCHBLT
  nStretchMode = NtGdiSetStretchBltMode(hdcOff, STRETCH_DELETESCANS);
  #endif
  oldFg = NtGdiSetTextColor(hdcOff, RGB(0, 0, 0));
  oldBg = NtGdiSetBkColor(hdcOff, RGB(255, 255, 255));
  
  if(diFlags & DI_MASK)
  {
    hOldMem = NtGdiSelectObject(hdcMem, hbmMask);
    #if CANSTRETCHBLT
    NtGdiStretchBlt(hdcOff, (DoFlickerFree ? 0 : xLeft), (DoFlickerFree ? 0 : yTop),
                    cxWidth, cyWidth, hdcMem, 0, 0, IconSize.cx, IconSize.cy, 
                    ((diFlags & DI_IMAGE) ? SRCAND : SRCCOPY));
    #else
    NtGdiBitBlt(hdcOff, (DoFlickerFree ? 0 : xLeft), (DoFlickerFree ? 0 : yTop),
                cxWidth, cyWidth, hdcMem, 0, 0, ((diFlags & DI_IMAGE) ? SRCAND : SRCCOPY));
    #endif
    if(!hbmColor && (bmpMask.bmHeight == 2 * bmpMask.bmWidth) && (diFlags & DI_IMAGE))
    {
      #if CANSTRETCHBLT
      NtGdiStretchBlt(hdcOff, (DoFlickerFree ? 0 : xLeft), (DoFlickerFree ? 0 : yTop),
                      cxWidth, cyWidth, hdcMem, 0, IconSize.cy, IconSize.cx, IconSize.cy, SRCINVERT);
      #else
      NtGdiBitBlt(hdcOff, (DoFlickerFree ? 0 : xLeft), (DoFlickerFree ? 0 : yTop),
                  cxWidth, cyWidth, hdcMem, 0, IconSize.cy, SRCINVERT);
      #endif
      diFlags &= ~DI_IMAGE;
    }
    NtGdiSelectObject(hdcMem, hOldMem);
  }
  
  if(diFlags & DI_IMAGE)
  {
    hOldMem = NtGdiSelectObject(hdcMem, (hbmColor ? hbmColor : hbmMask));
    #if CANSTRETCHBLT
    NtGdiStretchBlt(hdcOff, (DoFlickerFree ? 0 : xLeft), (DoFlickerFree ? 0 : yTop),
                    cxWidth, cyWidth, hdcMem, 0, (hbmColor ? 0 : IconSize.cy), 
                    IconSize.cx, IconSize.cy, ((diFlags & DI_MASK) ? SRCINVERT : SRCCOPY));
    #else
    NtGdiBitBlt(hdcOff, (DoFlickerFree ? 0 : xLeft), (DoFlickerFree ? 0 : yTop),
                cxWidth, cyWidth, hdcMem, 0, (hbmColor ? 0 : IconSize.cy), 
                ((diFlags & DI_MASK) ? SRCINVERT : SRCCOPY));
    #endif
    NtGdiSelectObject(hdcMem, hOldMem);
  }
  
  if(DoFlickerFree)
    NtGdiBitBlt(hdc, xLeft, yTop, cxWidth, cyWidth, hdcOff, 0, 0, SRCCOPY);
  
  NtGdiSetTextColor(hdcOff, oldFg);
  NtGdiSetBkColor(hdcOff, oldBg);
  #if CANSTRETCHBLT
  SetStretchBltMode(hdcOff, nStretchMode);
  #endif
  
  Ret = TRUE;
  
  cleanup:
  if(DoFlickerFree)
  {
    
    NtGdiSelectObject(hdcOff, hOldOffBmp);
    NtGdiSelectObject(hdcOff, hOldOffBrush);
    NtGdiDeleteObject(hbmOff);
    NtGdiDeleteDC(hdcOff);
  }
  if(hdcMem)
    NtGdiDeleteDC(hdcMem);
  
  done:
  
  return Ret;
}

