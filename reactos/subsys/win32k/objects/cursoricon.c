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
/* $Id: cursoricon.c,v 1.29 2003/12/08 22:51:11 gvg Exp $ */

#undef WIN32_LEAN_AND_MEAN

#include <ddk/ntddk.h>
#include <ddk/ntddmou.h>
#include <win32k/win32k.h>
#include <windows.h>
#include <stdlib.h>
#include <win32k/bitmaps.h>
#include <win32k/dc.h>
#include <include/winsta.h>
#include <include/desktop.h>
#include <include/error.h>
#include <include/mouse.h>
#include <include/window.h>
#include <include/cursoricon.h>
#include <include/inteng.h>
#include <include/surface.h>
#include <include/palette.h>
#include <include/eng.h>
#include <include/callback.h>
#include "include/object.h"
#include <internal/safe.h>

#define NDEBUG
#include <win32k/debug1.h>

HBITMAP FASTCALL
IntCopyBitmap(HBITMAP bmp)
{
  PBITMAPOBJ src;
  HBITMAP ret = (HBITMAP)0;
  
  if(!bmp)
    return (HBITMAP)0;
  
  src = BITMAPOBJ_LockBitmap(bmp);
  if(src)
  {
    ret = NtGdiCreateBitmap(src->bitmap.bmWidth, src->bitmap.bmHeight, src->bitmap.bmPlanes, 
                            src->bitmap.bmBitsPixel, src->bitmap.bmBits);
    BITMAPOBJ_UnlockBitmap(bmp);
  }
  
  return ret;
}

#define COLORCURSORS_ALLOWED FALSE
HCURSOR FASTCALL
IntSetCursor(PWINSTATION_OBJECT WinStaObject, PCURICON_OBJECT NewCursor, BOOL ForceChange)
{
  PDC dc;
  PSURFOBJ SurfObj;
  PSURFGDI SurfGDI;
  SIZEL MouseSize;
  RECTL MouseRect;
  PDEVINFO DevInfo;
  PBITMAPOBJ MaskBmpObj;
  PSYSTEM_CURSORINFO CurInfo;
  PCURICON_OBJECT OldCursor;
  HCURSOR Ret = (HCURSOR)0;
  HBITMAP hMask = (HBITMAP)0, hColor = (HBITMAP)0;
  PSURFOBJ soMask = NULL, soColor = NULL;
  PXLATEOBJ XlateObj = NULL;
  
  CurInfo = &WinStaObject->SystemCursor;
  OldCursor = CurInfo->CurrentCursorObject;
  if(OldCursor)
    Ret = (HCURSOR)OldCursor->Handle;
  
  if(!ForceChange && (OldCursor == NewCursor))
    goto done;
  
  /* FIXME use the desktop's HDC instead of using ScreenDeviceContext */
  dc = DC_LockDc(IntGetScreenDC());
  if(!dc)
    goto done;
  
  SurfObj = (PSURFOBJ)AccessUserObject((ULONG) dc->Surface);
  SurfGDI = (PSURFGDI)AccessInternalObject((ULONG) dc->Surface);
  DevInfo = dc->DevInfo;
  DC_UnlockDc(IntGetScreenDC());
  
  if(!NewCursor && (CurInfo->CurrentCursorObject || ForceChange))
  {
    SurfGDI->PointerStatus = SurfGDI->SetPointerShape(SurfObj, NULL, NULL, NULL,
                                                      0,
                                                      0,
                                                      CurInfo->x, 
                                                      CurInfo->y, 
                                                      &MouseRect,
                                                      SPS_CHANGE);
    
    CurInfo->CurrentCursorObject = NewCursor;
    CurInfo->ShowingCursor = 0;
    goto done;
  }
  
  if(NewCursor || ForceChange)
  {
    MaskBmpObj = BITMAPOBJ_LockBitmap(NewCursor->IconInfo.hbmMask);
    if(MaskBmpObj)
    {
      if(MaskBmpObj->bitmap.bmBitsPixel != 1)
      {
        DbgPrint("SetCursor: The Mask bitmap must have 1BPP!\n");
        BITMAPOBJ_UnlockBitmap(NewCursor->IconInfo.hbmMask);
        goto done;
      }
      
      BITMAPOBJ_UnlockBitmap(NewCursor->IconInfo.hbmMask);
      
      if((DevInfo->flGraphicsCaps2 & GCAPS2_ALPHACURSOR) && 
         (SurfGDI->BitsPerPixel >= 16) && NewCursor->Shadow
         && COLORCURSORS_ALLOWED)
      {
        /* FIXME - Create a color pointer, only 32bit bitmap, set alpha bits!
                   Do not pass a mask bitmap to DrvSetPointerShape()!
                   Create a XLATEOBJ that describes the colors of the bitmap. */
        DbgPrint("SetCursor: (Colored) alpha cursors are not supported!\n");
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
          DbgPrint("SetCursor: Cursors with colors are not supported!\n");
        }
        else
        {
          MaskBmpObj = BITMAPOBJ_LockBitmap(NewCursor->IconInfo.hbmMask);
          if(MaskBmpObj)
          {
            MouseSize.cx = MaskBmpObj->bitmap.bmWidth;
            MouseSize.cy = MaskBmpObj->bitmap.bmHeight;
            hMask = EngCreateBitmap(MouseSize, 4, BMF_1BPP, BMF_TOPDOWN, MaskBmpObj->bitmap.bmBits);
            soMask = (PSURFOBJ)AccessUserObject((ULONG)hMask);
            BITMAPOBJ_UnlockBitmap(NewCursor->IconInfo.hbmMask);
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
    
    SurfGDI->PointerStatus = SurfGDI->SetPointerShape(SurfObj, soMask, soColor, XlateObj,
                                                      NewCursor->IconInfo.xHotspot,
                                                      NewCursor->IconInfo.yHotspot,
                                                      CurInfo->x, 
                                                      CurInfo->y, 
                                                      &MouseRect,
                                                      SPS_CHANGE);
    
    if(hMask)
      EngDeleteSurface(hMask);
    if(hColor)
      EngDeleteSurface(hColor);
    if(XlateObj)
      EngDeleteXlate(XlateObj);
    
    if(SurfGDI->PointerStatus == SPS_DECLINE)
      DbgPrint("SetCursor: DrvSetPointerShape() returned SPS_DECLINE\n");
    else if(SurfGDI->PointerStatus == SPS_ERROR)
      DbgPrint("SetCursor: DrvSetPointerShape() returned SPS_ERROR\n");
  }
  
  done:
  return Ret;
}

BOOL FASTCALL
IntSetupCurIconHandles(PWINSTATION_OBJECT WinStaObject)
{
  PCURICONS Cursors;
  
  Cursors = &WinStaObject->SystemCursor.CurIcons;
  
  ExInitializeFastMutex(&Cursors->LockHandles);
  
  Cursors->Handles = ExAllocatePool(NonPagedPool, 2 * MAXCURICONHANDLES * sizeof(PCURICON_OBJECT));
  if(Cursors->Handles)
  {
    RtlZeroMemory(Cursors->Handles, 2 * MAXCURICONHANDLES * sizeof(PCURICON_OBJECT));
    Cursors->Objects = (PVOID)(Cursors->Handles + MAXCURICONHANDLES);
  }
  else
    Cursors->Objects = NULL;
  Cursors->Count = 0;
  
  return (Cursors->Handles != NULL);
}

PVOID FASTCALL
IntFindFreeHandleSlot(PCURICONS Cursors, UINT *Index)
{
  UINT i;
  PVOID *CurIconObject = Cursors->Handles;
  
  if(Cursors->Count >= MAXCURICONHANDLES)
  {
    return NULL;
  }
  
  for(i = 0; i <= MAXCURICONHANDLES; i++)
  {
    CurIconObject++;
    if(*CurIconObject == NULL)
    {
      *Index = i;
      return CurIconObject;
    }
  }
  
  return NULL;
}

PCURICON_OBJECT FASTCALL
IntFindByHandle(PCURICONS Cursors, HICON hIcon)
{
  UINT i, c = 0;
  PVOID *CurIconObject, *Objects;
  
  if(!hIcon)
    return NULL;
  
  CurIconObject = Cursors->Handles;
  Objects = Cursors->Objects;
  
  for(i = 0; i <= MAXCURICONHANDLES; i++)
  {
    CurIconObject++;
    if(*CurIconObject == hIcon)
    {
      return (PCURICON_OBJECT)(*(Objects + i));
    }
    if(*CurIconObject)
    {
      /* no more handles */
      if(++c >= Cursors->Count)
        return NULL;
    }
  }
  
  return NULL;
}

PCURICON_OBJECT FASTCALL
IntGetCurIconObject(PWINSTATION_OBJECT WinStaObject, HICON hIcon)
{
  PCURICONS Cursors;
  PCURICON_OBJECT CurIconObject = NULL;

  Cursors = &WinStaObject->SystemCursor.CurIcons;
  
  ExAcquireFastMutex(&Cursors->LockHandles);
  CurIconObject = IntFindByHandle(Cursors, hIcon);
  
  if(!CurIconObject)
    ExReleaseFastMutex(&Cursors->LockHandles);
  
  return CurIconObject;  
}

PCURICON_OBJECT FASTCALL
IntFindExistingCurIconObject(PWINSTATION_OBJECT WinStaObject, HMODULE hModule, 
                             HRSRC hRsrc)
{
  UINT i, c = 0;
  PVOID *Objects;
  PCURICON_OBJECT Obj;
  PCURICONS Cursors = &WinStaObject->SystemCursor.CurIcons;
  
  Objects = Cursors->Objects;
  
  ExAcquireFastMutex(&Cursors->LockHandles);
  
  for(i = 0; i <= MAXCURICONHANDLES; i++)
  {
    Obj = (PCURICON_OBJECT)(*Objects);
    if(Obj)
    {
      if((Obj->hModule == hModule) && (Obj->hRsrc == hRsrc))
      {
        return Obj;
      }
      /* no more handles */
      if(++c > Cursors->Count)
      {
        ExReleaseFastMutex(&Cursors->LockHandles);
        return NULL;
      }
    }
    Objects++;
  }
  
  ExReleaseFastMutex(&Cursors->LockHandles);
  return NULL;
}

VOID FASTCALL
IntReleaseCurIconObject(PWINSTATION_OBJECT WinStaObject)
{
  ExReleaseFastMutex(&WinStaObject->SystemCursor.CurIcons.LockHandles);
}

PCURICON_OBJECT FASTCALL
IntCreateCurIconHandle(PWINSTATION_OBJECT WinStaObject)
{
  PCURICONS Cursors;
  PVOID *Handle, *Objects;
  UINT i;
  PCURICON_OBJECT CurIconObject = NULL;
  
  Cursors = &WinStaObject->SystemCursor.CurIcons;
  
  ExAcquireFastMutex(&Cursors->LockHandles);
  
  Handle = IntFindFreeHandleSlot(Cursors, &i);
  if(!Handle)
  {
    SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
    return NULL;
  }
  
  /* create a new handle */
  CurIconObject = ExAllocatePool(NonPagedPool, sizeof(CURICON_OBJECT));
  if(!CurIconObject)
  {
    SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
    return NULL;
  }
  RtlZeroMemory(CurIconObject, sizeof(CURICON_OBJECT));
  
  CurIconObject->Handle = (HICON)(i + 1);
  
  CurIconObject->Process = PsGetWin32Process();
  
  Objects = Cursors->Objects;
  
  *Handle = (PVOID)CurIconObject->Handle;
  *(Objects + i) = (PVOID)CurIconObject;
  
  Cursors->Count++;

  return CurIconObject;
}

BOOL FASTCALL
IntDestroyCurIconObject(PWINSTATION_OBJECT WinStaObject, HCURSOR hCursor)
{
  UINT i, c = 0;
  PVOID *CurIconObject, *Objects;
  PCURICON_OBJECT Cursor;
  PSYSTEM_CURSORINFO CurInfo;
  
  if(!hCursor)
    return FALSE;
  
  CurInfo = &WinStaObject->SystemCursor;
  ExAcquireFastMutex(&CurInfo->CurIcons.LockHandles);
  
  CurIconObject = CurInfo->CurIcons.Handles;
  Objects = CurInfo->CurIcons.Objects;
  
  for(i = 0; i <= MAXCURICONHANDLES; i++)
  {
    CurIconObject++;
    if(*CurIconObject == hCursor)
    {
      Cursor = (PCURICON_OBJECT)(*(Objects + i));
      if(CurInfo->CurrentCursorObject == Cursor)
      {
        IntSetCursor(WinStaObject, NULL, FALSE);
        CurInfo->CurrentCursorObject = NULL;
      }
      
      /* remove from table */
      *CurIconObject = NULL;
      *(Objects + i) = NULL;
      CurInfo->CurIcons.Count--;
      
      /* free bitmaps */
      if(Cursor->IconInfo.hbmMask)
        NtGdiDeleteObject(Cursor->IconInfo.hbmMask);
      if(Cursor->IconInfo.hbmColor)
        NtGdiDeleteObject(Cursor->IconInfo.hbmColor);
      
      /* free object */
      ExFreePool(Cursor);
      
      ExReleaseFastMutex(&CurInfo->CurIcons.LockHandles);
      return TRUE;
    }
    if(*CurIconObject)
    {
      /* no more handles */
      if(++c >= CurInfo->CurIcons.Count)
      {
        ExReleaseFastMutex(&CurInfo->CurIcons.LockHandles);
        return FALSE;
      }
    }
  }
  
  ExReleaseFastMutex(&CurInfo->CurIcons.LockHandles);
  return FALSE;
}

/*
 * @implemented
 */
HICON
STDCALL
NtUserCreateCursorIconHandle(PICONINFO IconInfo, BOOL Indirect)
{
  PCURICON_OBJECT CurIconObject;
  PWINSTATION_OBJECT WinStaObject;
  PBITMAPOBJ bmp;
  NTSTATUS Status;
  HICON Ret;
  
  Status = IntValidateWindowStationHandle(PROCESS_WINDOW_STATION(),
				                               KernelMode,
				                               0,
				                               &WinStaObject);
  
  if(!NT_SUCCESS(Status))
  {
    SetLastNtError(Status);
    return (HICON)0;
  }
  
  CurIconObject = IntCreateCurIconHandle(WinStaObject);
  if(CurIconObject)
  {
    Ret = CurIconObject->Handle;
    
    if(IconInfo)
    {
      Status = MmCopyFromCaller(&CurIconObject->IconInfo, IconInfo, sizeof(ICONINFO));
      if(NT_SUCCESS(Status))
      {
        if(Indirect)
        {
          CurIconObject->IconInfo.hbmMask = IntCopyBitmap(CurIconObject->IconInfo.hbmMask);
          CurIconObject->IconInfo.hbmColor = IntCopyBitmap(CurIconObject->IconInfo.hbmColor);
        }
        if(CurIconObject->IconInfo.hbmColor && 
          (bmp = BITMAPOBJ_LockBitmap(CurIconObject->IconInfo.hbmColor)))
        {
          CurIconObject->Size.cx = bmp->size.cx;
          CurIconObject->Size.cy = bmp->size.cy;
          BITMAPOBJ_UnlockBitmap(CurIconObject->IconInfo.hbmColor);
        }
        else
        {
          if(CurIconObject->IconInfo.hbmMask && 
            (bmp = BITMAPOBJ_LockBitmap(CurIconObject->IconInfo.hbmMask)))
          {
            CurIconObject->Size.cx = bmp->size.cx;
            CurIconObject->Size.cy = bmp->size.cy / 2;
            BITMAPOBJ_UnlockBitmap(CurIconObject->IconInfo.hbmMask);
          }
        }
      }
      else
      {
        SetLastNtError(Status);
        /* FIXME - Don't exit here */
      }
    }
    
    IntReleaseCurIconObject(WinStaObject);
    ObDereferenceObject(WinStaObject);
    return Ret;
  }

  SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
  ObDereferenceObject(WinStaObject);
  return (HICON)0;
}

/*
 * @implemented
 */
BOOL
STDCALL
NtUserGetIconInfo(
  HICON hIcon,
  PICONINFO IconInfo)
{
  ICONINFO ii;
  PCURICON_OBJECT CurIconObject;
  PWINSTATION_OBJECT WinStaObject;
  NTSTATUS Status;
  BOOL Ret = FALSE;
  
  Status = IntValidateWindowStationHandle(PROCESS_WINDOW_STATION(),
				                               KernelMode,
				                               0,
				                               &WinStaObject);
  
  if(!NT_SUCCESS(Status))
  {
    SetLastNtError(Status);
    return FALSE;
  }
  
  CurIconObject = IntGetCurIconObject(WinStaObject, hIcon);
  if(CurIconObject)
  {
    if(IconInfo)
    {
      RtlCopyMemory(&ii, &CurIconObject->IconInfo, sizeof(ICONINFO));
      
      /* Copy bitmaps */
      ii.hbmMask = IntCopyBitmap(ii.hbmMask);
      ii.hbmColor = IntCopyBitmap(ii.hbmColor);      
      
      /* Copy fields */
      Status = MmCopyToCaller(IconInfo, &ii, sizeof(ICONINFO));
      if(NT_SUCCESS(Status))
        Ret = TRUE;
      else
        SetLastNtError(Status);
    }
    else
    {
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
    }
    
    IntReleaseCurIconObject(WinStaObject);
    ObDereferenceObject(WinStaObject);
    return Ret;
  }

  SetLastWin32Error(ERROR_INVALID_CURSOR_HANDLE);
  ObDereferenceObject(WinStaObject);
  return FALSE;
}


/*
 * @implemented
 */
BOOL
STDCALL
NtUserGetIconSize(
  HICON hIcon,
  BOOL *fIcon,
  SIZE *Size)
{
  PCURICON_OBJECT CurIconObject;
  PBITMAPOBJ bmp;
  PWINSTATION_OBJECT WinStaObject;
  NTSTATUS Status;
  BOOL Ret = FALSE;
  
  Status = IntValidateWindowStationHandle(PROCESS_WINDOW_STATION(),
				                               KernelMode,
				                               0,
				                               &WinStaObject);
  
  if(!NT_SUCCESS(Status))
  {
    SetLastNtError(Status);
    return FALSE;
  }
  
  CurIconObject = IntGetCurIconObject(WinStaObject, hIcon);
  if(CurIconObject)
  {
    /* Copy fields */
    Status = MmCopyToCaller(fIcon, &CurIconObject->IconInfo.fIcon, sizeof(BOOL));
    if(!NT_SUCCESS(Status))
    {
      SetLastNtError(Status);
      goto done;
    }
    
    bmp = BITMAPOBJ_LockBitmap(CurIconObject->IconInfo.hbmColor);
    if(!bmp)
      goto done;

    Status = MmCopyToCaller(Size, &bmp->size, sizeof(SIZE));
    if(NT_SUCCESS(Status))
      Ret = TRUE;
    else
      SetLastNtError(Status);
    
    BITMAPOBJ_UnlockBitmap(CurIconObject->IconInfo.hbmColor);
    
    done:
    IntReleaseCurIconObject(WinStaObject);
    ObDereferenceObject(WinStaObject);
    return Ret;
  }

  SetLastWin32Error(ERROR_INVALID_CURSOR_HANDLE);
  ObDereferenceObject(WinStaObject);
  return FALSE;
}


/*
 * @unimplemented
 */
DWORD
STDCALL
NtUserGetCursorFrameInfo(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3)
{
  UNIMPLEMENTED

  return 0;
}


/*
 * @implemented
 */
BOOL
STDCALL
NtUserGetCursorInfo(
  PCURSORINFO pci)
{
  CURSORINFO SafeCi;
  PSYSTEM_CURSORINFO CurInfo;
  PWINSTATION_OBJECT WinStaObject;
  NTSTATUS Status;
  PCURICON_OBJECT CursorObject;
  
  Status = MmCopyFromCaller(&SafeCi.cbSize, pci, sizeof(DWORD));
  if(!NT_SUCCESS(Status))
  {
    SetLastNtError(Status);
    return FALSE;
  }
  
  if(SafeCi.cbSize != sizeof(CURSORINFO))
  {
    SetLastWin32Error(ERROR_INVALID_PARAMETER);
    return FALSE;
  }
  
  Status = IntValidateWindowStationHandle(PROCESS_WINDOW_STATION(),
				                               KernelMode,
				                               0,
				                               &WinStaObject);
  
  if(!NT_SUCCESS(Status))
  {
    SetLastNtError(Status);
    return FALSE;
  }
  
  CurInfo = &WinStaObject->SystemCursor;
  CursorObject = (PCURICON_OBJECT)CurInfo->CurrentCursorObject;
  
  SafeCi.flags = ((CurInfo->ShowingCursor && CursorObject) ? CURSOR_SHOWING : 0);
  SafeCi.hCursor = (CursorObject ? (HCURSOR)CursorObject->Handle : (HCURSOR)0);
  SafeCi.ptScreenPos.x = CurInfo->x;
  SafeCi.ptScreenPos.y = CurInfo->y;
  
  Status = MmCopyToCaller(pci, &SafeCi, sizeof(CURSORINFO));
  if(!NT_SUCCESS(Status))
  {
    ObDereferenceObject(WinStaObject);
    SetLastNtError(Status);
    return FALSE;
  }
  
  ObDereferenceObject(WinStaObject);
  return TRUE;
}


/*
 * @implemented
 */
BOOL
STDCALL
NtUserClipCursor(
  RECT *UnsafeRect)
{
  /* FIXME - check if process has WINSTA_WRITEATTRIBUTES */
  
  PWINSTATION_OBJECT WinStaObject;
  PSYSTEM_CURSORINFO CurInfo;
  RECT Rect;
  PWINDOW_OBJECT DesktopWindow = NULL;

  NTSTATUS Status = IntValidateWindowStationHandle(PROCESS_WINDOW_STATION(),
				       KernelMode,
				       0,
				       &WinStaObject);
  if (!NT_SUCCESS(Status))
  {
    DPRINT1("Validation of window station handle (0x%X) failed\n",
      PROCESS_WINDOW_STATION());
    SetLastNtError(Status);
    return FALSE;
  }

  if (NULL != UnsafeRect && ! NT_SUCCESS(MmCopyFromCaller(&Rect, UnsafeRect, sizeof(RECT))))
  {
    ObDereferenceObject(WinStaObject);
    SetLastWin32Error(ERROR_INVALID_PARAMETER);
    return FALSE;
  }
  
  CurInfo = &WinStaObject->SystemCursor;
  if(WinStaObject->ActiveDesktop)
    DesktopWindow = IntGetWindowObject(WinStaObject->ActiveDesktop->DesktopWindow);
  
  if((Rect.right > Rect.left) && (Rect.bottom > Rect.top)
     && DesktopWindow)
  {
    CurInfo->CursorClipInfo.IsClipped = TRUE;
    CurInfo->CursorClipInfo.Left = max(Rect.left, DesktopWindow->WindowRect.left);
    CurInfo->CursorClipInfo.Top = max(Rect.top, DesktopWindow->WindowRect.top);
    CurInfo->CursorClipInfo.Right = min(Rect.right - 1, DesktopWindow->WindowRect.right - 1);
    CurInfo->CursorClipInfo.Bottom = min(Rect.bottom - 1, DesktopWindow->WindowRect.bottom - 1);
    IntReleaseWindowObject(DesktopWindow);
    
    MouseMoveCursor(CurInfo->x, CurInfo->y);  
  }
  else
    WinStaObject->SystemCursor.CursorClipInfo.IsClipped = FALSE;
    
  ObDereferenceObject(WinStaObject);
  
  return TRUE;
}


/*
 * @implemented
 */
BOOL
STDCALL
NtUserDestroyCursor(
  HCURSOR hCursor,
  DWORD Unknown)
{
  PWINSTATION_OBJECT WinStaObject;
  NTSTATUS Status;
  
  Status = IntValidateWindowStationHandle(PROCESS_WINDOW_STATION(),
				                               KernelMode,
				                               0,
				                               &WinStaObject);
  
  if(!NT_SUCCESS(Status))
  {
    SetLastNtError(Status);
    return FALSE;
  }
  
  if(IntDestroyCurIconObject(WinStaObject, hCursor))
  {
    ObDereferenceObject(WinStaObject);
    return TRUE;
  }

  SetLastWin32Error(ERROR_INVALID_CURSOR_HANDLE);
  ObDereferenceObject(WinStaObject);
  return FALSE;
}


/*
 * @implemented
 */
HICON
STDCALL
NtUserFindExistingCursorIcon(
  HMODULE hModule,
  HRSRC hRsrc)
{
  PCURICON_OBJECT CurIconObject;
  PWINSTATION_OBJECT WinStaObject;
  NTSTATUS Status;
  HICON Ret = (HICON)0;
  
  Status = IntValidateWindowStationHandle(PROCESS_WINDOW_STATION(),
				                               KernelMode,
				                               0,
				                               &WinStaObject);
  
  if(!NT_SUCCESS(Status))
  {
    SetLastNtError(Status);
    return Ret;
  }
  
  CurIconObject = IntFindExistingCurIconObject(WinStaObject, hModule, hRsrc);
  if(CurIconObject)
  {
    Ret = CurIconObject->Handle;
    
    IntReleaseCurIconObject(WinStaObject);
    ObDereferenceObject(WinStaObject);
    return Ret;
  }
  
  SetLastWin32Error(ERROR_INVALID_CURSOR_HANDLE);
  ObDereferenceObject(WinStaObject);
  return (HICON)0;
}


/*
 * @implemented
 */
BOOL
STDCALL
NtUserGetClipCursor(
  RECT *lpRect)
{
  /* FIXME - check if process has WINSTA_READATTRIBUTES */
  
  PWINSTATION_OBJECT WinStaObject;
  RECT Rect;
  NTSTATUS Status;
  
  if(!lpRect)
    return FALSE;

  Status = IntValidateWindowStationHandle(PROCESS_WINDOW_STATION(),
	       KernelMode,
	       0,
	       &WinStaObject);
  if (!NT_SUCCESS(Status))
  {
    DPRINT("Validation of window station handle (0x%X) failed\n",
      PROCESS_WINDOW_STATION());
    SetLastNtError(Status);
    return FALSE;
  }
  
  if(WinStaObject->SystemCursor.CursorClipInfo.IsClipped)
  {
    Rect.left = WinStaObject->SystemCursor.CursorClipInfo.Left;
    Rect.top = WinStaObject->SystemCursor.CursorClipInfo.Top;
    Rect.right = WinStaObject->SystemCursor.CursorClipInfo.Right;
    Rect.bottom = WinStaObject->SystemCursor.CursorClipInfo.Bottom;
  }
  else
  {
    Rect.left = 0;
    Rect.top = 0;
    Rect.right = NtUserGetSystemMetrics(SM_CXSCREEN);
    Rect.bottom = NtUserGetSystemMetrics(SM_CYSCREEN);
  }
  
  Status = MmCopyToCaller((PRECT)lpRect, &Rect, sizeof(RECT));
  if(!NT_SUCCESS(Status))
  {
    ObDereferenceObject(WinStaObject);
    SetLastNtError(Status);
    return FALSE;
  }
    
  ObDereferenceObject(WinStaObject);
  
  return TRUE;
}


/*
 * @unimplemented
 */
HCURSOR
STDCALL
NtUserSetCursor(
  HCURSOR hCursor)
{
  PCURICON_OBJECT CurIconObject;
  HICON OldCursor = (HCURSOR)0;
  PWINSTATION_OBJECT WinStaObject;
  NTSTATUS Status;
  
  Status = IntValidateWindowStationHandle(PROCESS_WINDOW_STATION(),
				                               KernelMode,
				                               0,
				                               &WinStaObject);
  if(!NT_SUCCESS(Status))
  {
    SetLastNtError(Status);
    return (HCURSOR)0;
  }
  
  CurIconObject = IntGetCurIconObject(WinStaObject, hCursor);
  if(CurIconObject)
  {
    OldCursor = IntSetCursor(WinStaObject, CurIconObject, FALSE);
    IntReleaseCurIconObject(WinStaObject);
  }
  else
    SetLastWin32Error(ERROR_INVALID_CURSOR_HANDLE);
  
  ObDereferenceObject(WinStaObject);
  return OldCursor;
}


/*
 * @implemented
 */
BOOL
STDCALL
NtUserSetCursorIconContents(
  HCURSOR hCursor,
  PICONINFO IconInfo)
{
  PCURICON_OBJECT CurIconObject;
  PBITMAPOBJ bmp;
  PWINSTATION_OBJECT WinStaObject;
  NTSTATUS Status;
  BOOL Ret = FALSE;
  
  Status = IntValidateWindowStationHandle(PROCESS_WINDOW_STATION(),
				                               KernelMode,
				                               0,
				                               &WinStaObject);
  
  if(!NT_SUCCESS(Status))
  {
    SetLastNtError(Status);
    return FALSE;
  }
  
  CurIconObject = IntGetCurIconObject(WinStaObject, (HICON)hCursor);
  if(CurIconObject)
  {
    /* Copy fields */
    Status = MmCopyFromCaller(&CurIconObject->IconInfo, IconInfo, sizeof(ICONINFO));
    if(!NT_SUCCESS(Status))
    {
      SetLastNtError(Status);
      goto done;
    }
    
    bmp = BITMAPOBJ_LockBitmap(CurIconObject->IconInfo.hbmColor);
    if(bmp)
    {
      CurIconObject->Size.cx = bmp->size.cx;
      CurIconObject->Size.cy = bmp->size.cy;
      BITMAPOBJ_UnlockBitmap(CurIconObject->IconInfo.hbmColor);
    }
    else
    {
      bmp = BITMAPOBJ_LockBitmap(CurIconObject->IconInfo.hbmMask);
      if(!bmp)
        goto done;
      
      CurIconObject->Size.cx = bmp->size.cx;
      CurIconObject->Size.cy = bmp->size.cy / 2;
      
      BITMAPOBJ_UnlockBitmap(CurIconObject->IconInfo.hbmMask);
    }
    
    Ret = TRUE;
    
    done:
    IntReleaseCurIconObject(WinStaObject);
    ObDereferenceObject(WinStaObject);
    return Ret;
  }

  SetLastWin32Error(ERROR_INVALID_CURSOR_HANDLE);
  ObDereferenceObject(WinStaObject);
  return FALSE;
}


/*
 * @implemented
 */
BOOL
STDCALL
NtUserSetCursorIconData(
  HICON hIcon,
  PBOOL fIcon,
  POINT *Hotspot,
  HMODULE hModule,
  HRSRC hRsrc,
  HRSRC hGroupRsrc)
{
  PCURICON_OBJECT CurIconObject;
  PWINSTATION_OBJECT WinStaObject;
  NTSTATUS Status;
  POINT SafeHotspot;
  BOOL Ret = FALSE;
  
  Status = IntValidateWindowStationHandle(PROCESS_WINDOW_STATION(),
				                               KernelMode,
				                               0,
				                               &WinStaObject);
  
  if(!NT_SUCCESS(Status))
  {
    SetLastNtError(Status);
    return FALSE;
  }
  
  CurIconObject = IntGetCurIconObject(WinStaObject, hIcon);
  if(CurIconObject)
  {
    /* Copy fields */
    if(fIcon)
    {
      Status = MmCopyFromCaller(&CurIconObject->IconInfo.fIcon, fIcon, sizeof(BOOL));
      if(!NT_SUCCESS(Status))
      {
        SetLastNtError(Status);
        goto done;
      }
    }
    else
    {
      if(!Hotspot)
        Ret = TRUE;
    }
    
    if(Hotspot)
    {
      Status = MmCopyFromCaller(&SafeHotspot, Hotspot, sizeof(POINT));
      if(NT_SUCCESS(Status))
      {
        CurIconObject->IconInfo.xHotspot = SafeHotspot.x;
        CurIconObject->IconInfo.yHotspot = SafeHotspot.y;
        
        CurIconObject->hModule = hModule;
        CurIconObject->hRsrc = hRsrc;
        CurIconObject->hGroupRsrc = hGroupRsrc;
        
        Ret = TRUE;
      }
      else
        SetLastNtError(Status);
    }
    
    if(!fIcon && !Hotspot)
    {
      CurIconObject->hModule = hModule;
      CurIconObject->hRsrc = hRsrc;
      CurIconObject->hGroupRsrc = hGroupRsrc;
      Ret = TRUE;
    }
    
    done:
    IntReleaseCurIconObject(WinStaObject);
    ObDereferenceObject(WinStaObject);
    return Ret;
  }

  SetLastWin32Error(ERROR_INVALID_CURSOR_HANDLE);
  ObDereferenceObject(WinStaObject);
  return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
NtUserSetSystemCursor(
  HCURSOR hcur,
  DWORD id)
{
  BOOL res = FALSE;

  return res;
}


#define CANSTRETCHBLT 0
/*
 * @implemented
 */
BOOL
STDCALL
NtUserDrawIconEx(
  HDC hdc,
  int xLeft,
  int yTop,
  HICON hIcon,
  int cxWidth,
  int cyWidth,
  UINT istepIfAniCur,
  HBRUSH hbrFlickerFreeDraw,
  UINT diFlags,
  DWORD Unknown0,
  DWORD Unknown1)
{
  PCURICON_OBJECT CurIconObject;
  PWINSTATION_OBJECT WinStaObject;
  NTSTATUS Status;
  HBITMAP hbmMask, hbmColor;
  BITMAP bmpMask, bmpColor;
  BOOL DoFlickerFree;
  SIZE IconSize;
  COLORREF oldFg, oldBg;
  HDC hdcMem, hdcOff = (HDC)0;
  HBITMAP hbmOff = (HBITMAP)0;
  HGDIOBJ hOldOff, hOldMem;
  BOOL Ret = FALSE;
  #if CANSTRETCHBLT
  INT nStretchMode;
  #endif
  
  Status = IntValidateWindowStationHandle(PROCESS_WINDOW_STATION(),
				                               KernelMode,
				                               0,
				                               &WinStaObject);
  
  if(!NT_SUCCESS(Status))
  {
    SetLastNtError(Status);
    return FALSE;
  }
  
  CurIconObject = IntGetCurIconObject(WinStaObject, hIcon);
  if(CurIconObject)
  {
    hbmMask = CurIconObject->IconInfo.hbmMask;
    hbmColor = CurIconObject->IconInfo.hbmColor;
    IntReleaseCurIconObject(WinStaObject);
    
    if(istepIfAniCur)
      DbgPrint("NtUserDrawIconEx: istepIfAniCur is not supported!\n");
    
    if(!hbmMask || !NtGdiGetObject(hbmMask, sizeof(BITMAP), &bmpMask))
      goto done;
    
    if(hbmColor && !NtGdiGetObject(hbmColor, sizeof(BITMAP), &bmpColor))
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
      cxWidth = ((diFlags & DI_DEFAULTSIZE) ? NtUserGetSystemMetrics(SM_CXICON) : IconSize.cx);
    if(!cyWidth)
      cyWidth = ((diFlags & DI_DEFAULTSIZE) ? NtUserGetSystemMetrics(SM_CYICON) : IconSize.cy);
    
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
      hOldOff = NtGdiSelectObject(hdcOff, hbrFlickerFreeDraw);
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
    
    cleanup:
    if(DoFlickerFree)
    {
      
      NtGdiSelectObject(hdcOff, hOldOff);
      NtGdiDeleteObject(hbmOff);
      NtGdiDeleteDC(hdcOff);
    }
    if(hdcMem)
      NtGdiDeleteDC(hdcMem);
    
    done:
    ObDereferenceObject(WinStaObject);
    
    return Ret;
  }
  
  SetLastWin32Error(ERROR_INVALID_CURSOR_HANDLE);
  ObDereferenceObject(WinStaObject);
  return FALSE;
}

