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
/* $Id: cursoricon.c,v 1.47 2004/02/10 23:40:01 gvg Exp $ */

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
#include <include/intgdi.h>
#include <include/callback.h>
#include "include/object.h"
#include <internal/safe.h>

#define NDEBUG
#include <win32k/debug1.h>

PCURICON_OBJECT FASTCALL
IntGetCurIconObject(PWINSTATION_OBJECT WinStaObject, HANDLE Handle)
{
  PCURICON_OBJECT Object;
  PUSER_HANDLE_TABLE HandleTable;
  
  HandleTable = (PUSER_HANDLE_TABLE)WinStaObject->SystemCursor.CurIconHandleTable;
  if(!NT_SUCCESS(ObmReferenceObjectByHandle(HandleTable, Handle, otCursorIcon, 
                 (PVOID*)&Object)))
  {
    return FALSE;
  }
  
  return Object;
}

VOID FASTCALL
IntReleaseCurIconObject(PCURICON_OBJECT Object)
{
  ObmDereferenceObject(Object);
}

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

STATIC VOID FASTCALL
SetPointerRect(PSYSTEM_CURSORINFO CurInfo, PRECTL PointerRect)
{
  CurInfo->PointerRectLeft = PointerRect->left;
  CurInfo->PointerRectRight = PointerRect->right;
  CurInfo->PointerRectTop = PointerRect->top;
  CurInfo->PointerRectBottom = PointerRect->bottom;
}

#define COLORCURSORS_ALLOWED FALSE
HCURSOR FASTCALL
IntSetCursor(PWINSTATION_OBJECT WinStaObject, PCURICON_OBJECT NewCursor,
   BOOL ForceChange)
{
   PSURFOBJ SurfObj;
   PSURFGDI SurfGDI;
   SIZEL MouseSize;
   PDEVINFO DevInfo;
   PBITMAPOBJ MaskBmpObj;
   PSYSTEM_CURSORINFO CurInfo;
   PCURICON_OBJECT OldCursor;
   HCURSOR Ret = (HCURSOR)0;
   HBITMAP hMask = (HBITMAP)0, hColor = (HBITMAP)0;
   PSURFOBJ soMask = NULL, soColor = NULL;
   PXLATEOBJ XlateObj = NULL;
   RECTL PointerRect;
  
   CurInfo = &WinStaObject->SystemCursor;
   OldCursor = CurInfo->CurrentCursorObject;
   if (OldCursor)
   {
      Ret = (HCURSOR)OldCursor->Self;
   }
  
   if (!ForceChange && OldCursor == NewCursor)
   {
      return Ret;
   }
  
   {
      /* FIXME use the desktop's HDC instead of using ScreenDeviceContext */
      PDC dc = DC_LockDc(IntGetScreenDC());

      if (!dc)
      {
         return Ret;
      }
  
      SurfObj = (PSURFOBJ)AccessUserObject((ULONG) dc->Surface);
      SurfGDI = (PSURFGDI)AccessInternalObject((ULONG) dc->Surface);
      DevInfo = dc->DevInfo;
      DC_UnlockDc(IntGetScreenDC());
   }
  
   if (!NewCursor && (CurInfo->CurrentCursorObject || ForceChange))
   {
      if (NULL != CurInfo->CurrentCursorObject && CurInfo->ShowingCursor)
      {
         /* Remove the cursor if it was displayed */
         ExAcquireFastMutex(SurfGDI->DriverLock);
         SurfGDI->MovePointer(SurfObj, -1, -1, &PointerRect);
         ExReleaseFastMutex(SurfGDI->DriverLock);
         SetPointerRect(CurInfo, &PointerRect);
      }

      SurfGDI->PointerStatus = SPS_ACCEPT_NOEXCLUDE;

      CurInfo->CurrentCursorObject = NewCursor; /* i.e. CurrentCursorObject = NULL */
      CurInfo->ShowingCursor = 0;
      return Ret;
   }
  
   if (!NewCursor)
   {
      return Ret;
   }

   /* TODO: Fixme. Logic is screwed above */

   ASSERT(NewCursor);
   MaskBmpObj = BITMAPOBJ_LockBitmap(NewCursor->IconInfo.hbmMask);
   if (MaskBmpObj)
   {
      const int maskBpp = MaskBmpObj->bitmap.bmBitsPixel;
      BITMAPOBJ_UnlockBitmap(NewCursor->IconInfo.hbmMask);
      if (maskBpp != 1)
      {
         DbgPrint("SetCursor: The Mask bitmap must have 1BPP!\n");
         return Ret;
      }
      
      if ((DevInfo->flGraphicsCaps2 & GCAPS2_ALPHACURSOR) && 
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
    
    ExAcquireFastMutex(SurfGDI->DriverLock);
    SurfGDI->PointerStatus = SurfGDI->SetPointerShape(SurfObj, soMask, soColor, XlateObj,
                                                      NewCursor->IconInfo.xHotspot,
                                                      NewCursor->IconInfo.yHotspot,
                                                      CurInfo->x, 
                                                      CurInfo->y, 
                                                      &PointerRect,
                                                      SPS_CHANGE);
    ExReleaseFastMutex(SurfGDI->DriverLock);

    if(SurfGDI->PointerStatus == SPS_DECLINE)
    {
      SurfGDI->SetPointerShape = EngSetPointerShape;
      SurfGDI->MovePointer = EngMovePointer;
      SurfGDI->PointerStatus = EngSetPointerShape(
                         SurfObj, soMask, soColor, XlateObj,
                         NewCursor->IconInfo.xHotspot,
                         NewCursor->IconInfo.yHotspot,
                         CurInfo->x, 
                         CurInfo->y, 
                         &PointerRect,
                         SPS_CHANGE);
      DbgPrint("SetCursor: DrvSetPointerShape() returned SPS_DECLINE\n");
    }
    
    SetPointerRect(CurInfo, &PointerRect);
    
    if(hMask)
    {
      EngDeleteSurface(hMask);
    }
    if(hColor)
    {
      EngDeleteSurface(hColor);
    }
    if(XlateObj)
    {
      EngDeleteXlate(XlateObj);
    }
    
    if(SurfGDI->PointerStatus == SPS_ERROR)
      DbgPrint("SetCursor: DrvSetPointerShape() returned SPS_ERROR\n");
  
  return Ret;
}

BOOL FASTCALL
IntSetupCurIconHandles(PWINSTATION_OBJECT WinStaObject)
{
  if((WinStaObject->SystemCursor.CurIconHandleTable = (PVOID)ObmCreateHandleTable()))
  {
    ObmInitializeHandleTable((PUSER_HANDLE_TABLE)WinStaObject->SystemCursor.CurIconHandleTable);
  }
  return (WinStaObject->SystemCursor.CurIconHandleTable != NULL);
}

PCURICON_OBJECT FASTCALL
IntFindExistingCurIconObject(PWINSTATION_OBJECT WinStaObject, HMODULE hModule, 
                             HRSRC hRsrc, LONG cx, LONG cy)
{
  PUSER_HANDLE_TABLE HandleTable;
  PLIST_ENTRY CurrentEntry;
  PUSER_HANDLE_BLOCK Current;
  PCURICON_OBJECT Object;
  ULONG i;
  
  HandleTable = (PUSER_HANDLE_TABLE)WinStaObject->SystemCursor.CurIconHandleTable;
  ExAcquireFastMutex(&HandleTable->ListLock);
  
  CurrentEntry = HandleTable->ListHead.Flink;
  while(CurrentEntry != &HandleTable->ListHead)
  {
    Current = CONTAINING_RECORD(CurrentEntry, USER_HANDLE_BLOCK, ListEntry);
    for(i = 0; i < HANDLE_BLOCK_ENTRIES; i++)
    {
      Object = (PCURICON_OBJECT)Current->Handles[i].ObjectBody;
      if(Object && (Object->hModule == hModule) && (Object->hRsrc == hRsrc))
      {
        if(cx && ((cx != Object->Size.cx) || (cy != Object->Size.cy)))
        {
          continue;
        }
        ObmReferenceObject(Object);
        ExReleaseFastMutex(&HandleTable->ListLock);
        return Object;
      }
    }
    CurrentEntry = CurrentEntry->Flink;
  }
  
  ExReleaseFastMutex(&HandleTable->ListLock);
  return NULL;
}

PCURICON_OBJECT FASTCALL
IntCreateCurIconHandle(PWINSTATION_OBJECT WinStaObject)
{
  PUSER_HANDLE_TABLE HandleTable;
  PCURICON_OBJECT Object;
  HANDLE Handle;
  PW32PROCESS Win32Process;
  
  HandleTable = (PUSER_HANDLE_TABLE)WinStaObject->SystemCursor.CurIconHandleTable;
  
  Object = ObmCreateObject(HandleTable, &Handle, otCursorIcon, sizeof(CURICON_OBJECT));
  
  if(!Object)
  {
    SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
    return FALSE;
  }
  
  Win32Process = PsGetWin32Process();
  
  ExAcquireFastMutex(&Win32Process->CursorIconListLock);
  InsertTailList(&Win32Process->CursorIconListHead, &Object->ListEntry);
  ExReleaseFastMutex(&Win32Process->CursorIconListLock);
  
  Object->Self = Handle;
  Object->Process = PsGetWin32Process();
  
  return Object;
}

BOOL FASTCALL
IntDestroyCurIconObject(PWINSTATION_OBJECT WinStaObject, HANDLE Handle, BOOL RemoveFromProcess)
{
  PUSER_HANDLE_TABLE HandleTable;
  PCURICON_OBJECT Object;
  HBITMAP bmpMask, bmpColor;
  NTSTATUS Status;
  BOOL Ret;
  
  HandleTable = (PUSER_HANDLE_TABLE)WinStaObject->SystemCursor.CurIconHandleTable;
  
  Status = ObmReferenceObjectByHandle(HandleTable, Handle, otCursorIcon, (PVOID*)&Object);
  if(!NT_SUCCESS(Status))
  {
    return FALSE;
  }
  
  if(WinStaObject->SystemCursor.CurrentCursorObject == Object)
  {
    /* Hide the cursor if we're destroying the current cursor */
    IntSetCursor(WinStaObject, NULL, TRUE);
  }
  
  bmpMask = Object->IconInfo.hbmMask;
  bmpColor = Object->IconInfo.hbmColor;
  
  
  if(Object->Process && RemoveFromProcess)
  {
    ExAcquireFastMutex(&Object->Process->CursorIconListLock);
    RemoveEntryList(&Object->ListEntry);
    ExReleaseFastMutex(&Object->Process->CursorIconListLock);
  }
  
  ObmDereferenceObject(Object);
  
  Ret = NT_SUCCESS(ObmCloseHandle(HandleTable, Handle));
  
  /* delete bitmaps */
  if(bmpMask)
    NtGdiDeleteObject(bmpMask);
  if(bmpColor)
    NtGdiDeleteObject(bmpColor);
  
  return Ret;
}

VOID FASTCALL
IntCleanupCurIcons(struct _EPROCESS *Process, PW32PROCESS Win32Process)
{
  PWINSTATION_OBJECT WinStaObject;
  PCURICON_OBJECT Current;
  PLIST_ENTRY CurrentEntry, NextEntry;
  
  if(!(WinStaObject = Win32Process->WindowStation))
    return;
  
  ExAcquireFastMutex(&Win32Process->CursorIconListLock);
  CurrentEntry = Win32Process->CursorIconListHead.Flink;
  while(CurrentEntry != &Win32Process->CursorIconListHead)
  {
    NextEntry = CurrentEntry->Flink;
    Current = CONTAINING_RECORD(CurrentEntry, CURICON_OBJECT, ListEntry);
    RemoveEntryList(&Current->ListEntry);
    IntDestroyCurIconObject(WinStaObject, Current->Self, FALSE);
    CurrentEntry = NextEntry;
  }
  ExReleaseFastMutex(&Win32Process->CursorIconListLock);
}

/*
 * @implemented
 */
HANDLE
STDCALL
NtUserCreateCursorIconHandle(PICONINFO IconInfo, BOOL Indirect)
{
  PCURICON_OBJECT CurIconObject;
  PWINSTATION_OBJECT WinStaObject;
  PBITMAPOBJ bmp;
  NTSTATUS Status;
  HANDLE Ret;
  
  Status = IntValidateWindowStationHandle(PROCESS_WINDOW_STATION(),
				                               KernelMode,
				                               0,
				                               &WinStaObject);
  
  if(!NT_SUCCESS(Status))
  {
    SetLastNtError(Status);
    return (HANDLE)0;
  }
  
  CurIconObject = IntCreateCurIconHandle(WinStaObject);
  if(CurIconObject)
  {
    Ret = CurIconObject->Self;
    
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
          CurIconObject->Size.cx = bmp->bitmap.bmWidth;
          CurIconObject->Size.cy = bmp->bitmap.bmHeight;
          BITMAPOBJ_UnlockBitmap(CurIconObject->IconInfo.hbmColor);
        }
        else
        {
          if(CurIconObject->IconInfo.hbmMask && 
            (bmp = BITMAPOBJ_LockBitmap(CurIconObject->IconInfo.hbmMask)))
          {
            CurIconObject->Size.cx = bmp->bitmap.bmWidth;
            CurIconObject->Size.cy = bmp->bitmap.bmHeight / 2;
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
    
    ObDereferenceObject(WinStaObject);
    return Ret;
  }

  SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
  ObDereferenceObject(WinStaObject);
  return (HANDLE)0;
}

/*
 * @implemented
 */
BOOL
STDCALL
NtUserGetCursorIconInfo(
  HANDLE Handle,
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
  
  CurIconObject = IntGetCurIconObject(WinStaObject, Handle);
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
    
    IntReleaseCurIconObject(CurIconObject);
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
NtUserGetCursorIconSize(
  HANDLE Handle,
  BOOL *fIcon,
  SIZE *Size)
{
  PCURICON_OBJECT CurIconObject;
  PBITMAPOBJ bmp;
  PWINSTATION_OBJECT WinStaObject;
  NTSTATUS Status;
  BOOL Ret = FALSE;
  SIZE SafeSize;
  
  Status = IntValidateWindowStationHandle(PROCESS_WINDOW_STATION(),
				                               KernelMode,
				                               0,
				                               &WinStaObject);
  
  if(!NT_SUCCESS(Status))
  {
    SetLastNtError(Status);
    return FALSE;
  }
  
  CurIconObject = IntGetCurIconObject(WinStaObject, Handle);
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

    SafeSize.cx = bmp->bitmap.bmWidth;
    SafeSize.cy = bmp->bitmap.bmHeight;
    Status = MmCopyToCaller(Size, &SafeSize, sizeof(SIZE));
    if(NT_SUCCESS(Status))
      Ret = TRUE;
    else
      SetLastNtError(Status);
    
    BITMAPOBJ_UnlockBitmap(CurIconObject->IconInfo.hbmColor);
    
    done:
    IntReleaseCurIconObject(CurIconObject);
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
  SafeCi.hCursor = (CursorObject ? (HCURSOR)CursorObject->Self : (HCURSOR)0);
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
NtUserDestroyCursorIcon(
  HANDLE Handle,
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
  
  if(IntDestroyCurIconObject(WinStaObject, Handle, TRUE))
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
HANDLE
STDCALL
NtUserFindExistingCursorIcon(
  HMODULE hModule,
  HRSRC hRsrc,
  LONG cx,
  LONG cy)
{
  PCURICON_OBJECT CurIconObject;
  PWINSTATION_OBJECT WinStaObject;
  NTSTATUS Status;
  HANDLE Ret = (HANDLE)0;
  
  Status = IntValidateWindowStationHandle(PROCESS_WINDOW_STATION(),
				                               KernelMode,
				                               0,
				                               &WinStaObject);
  
  if(!NT_SUCCESS(Status))
  {
    SetLastNtError(Status);
    return Ret;
  }
  
  CurIconObject = IntFindExistingCurIconObject(WinStaObject, hModule, hRsrc, cx, cy);
  if(CurIconObject)
  {
    Ret = CurIconObject->Self;
    
    IntReleaseCurIconObject(CurIconObject);
    ObDereferenceObject(WinStaObject);
    return Ret;
  }
  
  SetLastWin32Error(ERROR_INVALID_CURSOR_HANDLE);
  ObDereferenceObject(WinStaObject);
  return (HANDLE)0;
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
 * @implemented
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
    IntReleaseCurIconObject(CurIconObject);
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
  HANDLE Handle,
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
  
  CurIconObject = IntGetCurIconObject(WinStaObject, Handle);
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
      CurIconObject->Size.cx = bmp->bitmap.bmWidth;
      CurIconObject->Size.cy = bmp->bitmap.bmHeight;
      BITMAPOBJ_UnlockBitmap(CurIconObject->IconInfo.hbmColor);
    }
    else
    {
      bmp = BITMAPOBJ_LockBitmap(CurIconObject->IconInfo.hbmMask);
      if(!bmp)
        goto done;
      
      CurIconObject->Size.cx = bmp->bitmap.bmWidth;
      CurIconObject->Size.cy = bmp->bitmap.bmHeight / 2;
      
      BITMAPOBJ_UnlockBitmap(CurIconObject->IconInfo.hbmMask);
    }
    
    Ret = TRUE;
    
    done:
    IntReleaseCurIconObject(CurIconObject);
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
  HANDLE Handle,
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
  
  CurIconObject = IntGetCurIconObject(WinStaObject, Handle);
  if(CurIconObject)
  {
    CurIconObject->hModule = hModule;
    CurIconObject->hRsrc = hRsrc;
    CurIconObject->hGroupRsrc = hGroupRsrc;
    
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
        
        Ret = TRUE;
      }
      else
        SetLastNtError(Status);
    }
    
    if(!fIcon && !Hotspot)
    {
      Ret = TRUE;
    }
    
    done:
    IntReleaseCurIconObject(CurIconObject);
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
  return FALSE;
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
  HGDIOBJ hOldOffBrush, hOldOffBmp, hOldMem;
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
    IntReleaseCurIconObject(CurIconObject);
    
    if(istepIfAniCur)
      DbgPrint("NtUserDrawIconEx: istepIfAniCur is not supported!\n");
    
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
    ObDereferenceObject(WinStaObject);
    
    return Ret;
  }
  
  SetLastWin32Error(ERROR_INVALID_CURSOR_HANDLE);
  ObDereferenceObject(WinStaObject);
  return FALSE;
}

