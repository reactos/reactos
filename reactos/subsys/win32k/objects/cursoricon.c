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
/* $Id: cursoricon.c,v 1.61 2004/07/04 11:18:56 navaraf Exp $ */
#include <w32k.h>

PCURICON_OBJECT FASTCALL
IntGetCurIconObject(PWINSTATION_OBJECT WinStaObject, HANDLE Handle)
{
   PCURICON_OBJECT Object;
   NTSTATUS Status;

   Status = ObmReferenceObjectByHandle(WinStaObject->HandleTable,
      Handle, otCursorIcon, (PVOID*)&Object);
   if (!NT_SUCCESS(Status))
   {
      return NULL;
   }
   return Object;
}

#define COLORCURSORS_ALLOWED FALSE
HCURSOR FASTCALL
IntSetCursor(PWINSTATION_OBJECT WinStaObject, PCURICON_OBJECT NewCursor,
   BOOL ForceChange)
{
   BITMAPOBJ *BitmapObj;
   SURFOBJ *SurfObj;
   PDEVINFO DevInfo;
   PBITMAPOBJ MaskBmpObj = NULL;
   PSYSTEM_CURSORINFO CurInfo;
   PCURICON_OBJECT OldCursor;
   HCURSOR Ret = (HCURSOR)0;
   HBITMAP hColor = (HBITMAP)0;
   HBITMAP hMask = 0;
   SURFOBJ *soMask = NULL, *soColor = NULL;
   XLATEOBJ *XlateObj = NULL;
   RECTL PointerRect;
   HDC Screen;
  
   CurInfo = IntGetSysCursorInfo(WinStaObject);
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
      if(!(Screen = IntGetScreenDC()))
      {
        return (HCURSOR)0;
      }
      /* FIXME use the desktop's HDC instead of using ScreenDeviceContext */
      PDC dc = DC_LockDc(Screen);

      if (!dc)
      {
         return Ret;
      }
  
      BitmapObj = BITMAPOBJ_LockBitmap(dc->w.hBitmap);
      SurfObj = &BitmapObj->SurfObj;
      DevInfo = dc->DevInfo;
      DC_UnlockDc(Screen);
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
      BITMAPOBJ_UnlockBitmap(SurfObj->hsurf);
      return Ret;
   }
  
   if (!NewCursor)
   {
      BITMAPOBJ_UnlockBitmap(SurfObj->hsurf);
      return Ret;
   }

   /* TODO: Fixme. Logic is screwed above */

   ASSERT(NewCursor);
   MaskBmpObj = BITMAPOBJ_LockBitmap(NewCursor->IconInfo.hbmMask);
   if (MaskBmpObj)
   {
      const int maskBpp = BitsPerFormat(MaskBmpObj->SurfObj.iBitmapFormat);
      BITMAPOBJ_UnlockBitmap(NewCursor->IconInfo.hbmMask);
      if (maskBpp != 1)
      {
         DbgPrint("SetCursor: The Mask bitmap must have 1BPP!\n");
         BITMAPOBJ_UnlockBitmap(SurfObj->hsurf);
         return Ret;
      }
      
      if ((DevInfo->flGraphicsCaps2 & GCAPS2_ALPHACURSOR) && 
          SurfObj->iBitmapFormat >= BMF_16BPP &&
          SurfObj->iBitmapFormat <= BMF_32BPP &&
          NewCursor->Shadow && COLORCURSORS_ALLOWED)
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
    
    if (GDIDEVFUNCS(SurfObj).SetPointerShape)
    {
      GDIDEV(SurfObj)->PointerStatus =
         GDIDEVFUNCS(SurfObj).SetPointerShape(
                                                        SurfObj, soMask, soColor, XlateObj,
                                                        NewCursor->IconInfo.xHotspot,
                                                        NewCursor->IconInfo.yHotspot,
                                                        CurInfo->x, 
                                                        CurInfo->y, 
                                                        &PointerRect,
                                                        SPS_CHANGE);
      DPRINT("SetCursor: DrvSetPointerShape() returned %x\n",
         GDIDEV(SurfObj)->PointerStatus);
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
    
    BITMAPOBJ_UnlockBitmap(SurfObj->hsurf);
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
      DbgPrint("SetCursor: DrvSetPointerShape() returned SPS_ERROR\n");
  
  return Ret;
}

BOOL FASTCALL
IntSetupCurIconHandles(PWINSTATION_OBJECT WinStaObject)
{
  return TRUE;
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
  
  HandleTable = (PUSER_HANDLE_TABLE)WinStaObject->HandleTable;
  ObmpLockHandleTable(HandleTable);
  
  CurrentEntry = HandleTable->ListHead.Flink;
  while(CurrentEntry != &HandleTable->ListHead)
  {
    Current = CONTAINING_RECORD(CurrentEntry, USER_HANDLE_BLOCK, ListEntry);
    for(i = 0; i < HANDLE_BLOCK_ENTRIES; i++)
    {
      Object = (PCURICON_OBJECT)Current->Handles[i].ObjectBody;
      if(Object && (ObmReferenceObjectByPointer(Object, otCursorIcon) == STATUS_SUCCESS))
      {
        if((Object->hModule == hModule) && (Object->hRsrc == hRsrc))
        {
          if(cx && ((cx != Object->Size.cx) || (cy != Object->Size.cy)))
          {
	    ObmDereferenceObject(Object);
	    continue;
          }
          ObmpUnlockHandleTable(HandleTable);
          return Object;
        }
        ObmDereferenceObject(Object);
      }
    }
    CurrentEntry = CurrentEntry->Flink;
  }
  
  ObmpUnlockHandleTable(HandleTable);
  return NULL;
}

PCURICON_OBJECT FASTCALL
IntCreateCurIconHandle(PWINSTATION_OBJECT WinStaObject)
{
  PCURICON_OBJECT Object;
  HANDLE Handle;
  PW32PROCESS Win32Process;
  
  Object = ObmCreateObject(WinStaObject->HandleTable, &Handle, otCursorIcon, sizeof(CURICON_OBJECT));
  
  if(!Object)
  {
    SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
    return FALSE;
  }
  
  Win32Process = PsGetWin32Process();
  
  IntLockProcessCursorIcons(Win32Process);
  InsertTailList(&Win32Process->CursorIconListHead, &Object->ListEntry);
  IntUnLockProcessCursorIcons(Win32Process);
  
  Object->Self = Handle;
  Object->Process = PsGetWin32Process();
  
  return Object;
}

BOOL FASTCALL
IntDestroyCurIconObject(PWINSTATION_OBJECT WinStaObject, HANDLE Handle, BOOL RemoveFromProcess)
{
  PSYSTEM_CURSORINFO CurInfo;
  PCURICON_OBJECT Object;
  HBITMAP bmpMask, bmpColor;
  NTSTATUS Status;
  BOOL Ret;
  
  Status = ObmReferenceObjectByHandle(WinStaObject->HandleTable, Handle, otCursorIcon, (PVOID*)&Object);
  if(!NT_SUCCESS(Status))
  {
    return FALSE;
  }
  
  if (Object->Process != PsGetWin32Process())
  {
    ObmDereferenceObject(Object);
    return FALSE;
  }

  CurInfo = IntGetSysCursorInfo(WinStaObject);

  if (CurInfo->CurrentCursorObject == Object)
  {
    /* Hide the cursor if we're destroying the current cursor */
    IntSetCursor(WinStaObject, NULL, TRUE);
  }
  
  bmpMask = Object->IconInfo.hbmMask;
  bmpColor = Object->IconInfo.hbmColor;

  if (Object->Process && RemoveFromProcess)
  {
    IntLockProcessCursorIcons(Object->Process);
    RemoveEntryList(&Object->ListEntry);
    IntUnLockProcessCursorIcons(Object->Process);
  }
  
  Ret = NT_SUCCESS(ObmCloseHandle(WinStaObject->HandleTable, Handle));
  
  /* delete bitmaps */
  if(bmpMask)
    NtGdiDeleteObject(bmpMask);
  if(bmpColor)
    NtGdiDeleteObject(bmpColor);

  ObmDereferenceObject(Object);
  
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
  
  IntLockProcessCursorIcons(Win32Process);
  CurrentEntry = Win32Process->CursorIconListHead.Flink;
  while(CurrentEntry != &Win32Process->CursorIconListHead)
  {
    NextEntry = CurrentEntry->Flink;
    Current = CONTAINING_RECORD(CurrentEntry, CURICON_OBJECT, ListEntry);
    RemoveEntryList(&Current->ListEntry);
    IntDestroyCurIconObject(WinStaObject, Current->Self, FALSE);
    CurrentEntry = NextEntry;
  }
  IntUnLockProcessCursorIcons(Win32Process);
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
          CurIconObject->IconInfo.hbmMask = BITMAPOBJ_CopyBitmap(CurIconObject->IconInfo.hbmMask);
          CurIconObject->IconInfo.hbmColor = BITMAPOBJ_CopyBitmap(CurIconObject->IconInfo.hbmColor);
        }
        if(CurIconObject->IconInfo.hbmColor && 
          (bmp = BITMAPOBJ_LockBitmap(CurIconObject->IconInfo.hbmColor)))
        {
          CurIconObject->Size.cx = bmp->SurfObj.sizlBitmap.cx;
          CurIconObject->Size.cy = bmp->SurfObj.sizlBitmap.cy;
          BITMAPOBJ_UnlockBitmap(CurIconObject->IconInfo.hbmColor);
        }
        else
        {
          if(CurIconObject->IconInfo.hbmMask && 
            (bmp = BITMAPOBJ_LockBitmap(CurIconObject->IconInfo.hbmMask)))
          {
            CurIconObject->Size.cx = bmp->SurfObj.sizlBitmap.cx;
            CurIconObject->Size.cy = bmp->SurfObj.sizlBitmap.cy / 2;
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
      ii.hbmMask = BITMAPOBJ_CopyBitmap(ii.hbmMask);
      ii.hbmColor = BITMAPOBJ_CopyBitmap(ii.hbmColor);      
      
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

    SafeSize.cx = bmp->SurfObj.sizlBitmap.cx;
    SafeSize.cy = bmp->SurfObj.sizlBitmap.cy;
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
  
  CurInfo = IntGetSysCursorInfo(WinStaObject);
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
  
  CurInfo = IntGetSysCursorInfo(WinStaObject);
  if(WinStaObject->ActiveDesktop)
    DesktopWindow = IntGetWindowObject(WinStaObject->ActiveDesktop->DesktopWindow);
  
  if((Rect.right > Rect.left) && (Rect.bottom > Rect.top)
     && DesktopWindow)
  {
    MOUSEINPUT mi;
    
    CurInfo->CursorClipInfo.IsClipped = TRUE;
    CurInfo->CursorClipInfo.Left = max(Rect.left, DesktopWindow->WindowRect.left);
    CurInfo->CursorClipInfo.Top = max(Rect.top, DesktopWindow->WindowRect.top);
    CurInfo->CursorClipInfo.Right = min(Rect.right - 1, DesktopWindow->WindowRect.right - 1);
    CurInfo->CursorClipInfo.Bottom = min(Rect.bottom - 1, DesktopWindow->WindowRect.bottom - 1);
    IntReleaseWindowObject(DesktopWindow);
    
    mi.dx = CurInfo->x;
    mi.dy = CurInfo->y;
    mi.mouseData = 0;
    mi.dwFlags = MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE;
    mi.time = 0;
    mi.dwExtraInfo = 0;
    IntMouseInput(&mi);
    
    return TRUE;
  }
  
  CurInfo->CursorClipInfo.IsClipped = FALSE;
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
HICON
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
  PSYSTEM_CURSORINFO CurInfo;
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
  
  CurInfo = IntGetSysCursorInfo(WinStaObject);
  if(CurInfo->CursorClipInfo.IsClipped)
  {
    Rect.left = CurInfo->CursorClipInfo.Left;
    Rect.top = CurInfo->CursorClipInfo.Top;
    Rect.right = CurInfo->CursorClipInfo.Right;
    Rect.bottom = CurInfo->CursorClipInfo.Bottom;
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
      CurIconObject->Size.cx = bmp->SurfObj.sizlBitmap.cx;
      CurIconObject->Size.cy = bmp->SurfObj.sizlBitmap.cy;
      BITMAPOBJ_UnlockBitmap(CurIconObject->IconInfo.hbmColor);
    }
    else
    {
      bmp = BITMAPOBJ_LockBitmap(CurIconObject->IconInfo.hbmMask);
      if(!bmp)
        goto done;
      
      CurIconObject->Size.cx = bmp->SurfObj.sizlBitmap.cx;
      CurIconObject->Size.cy = bmp->SurfObj.sizlBitmap.cy / 2;
      
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
  HGDIOBJ hOldOffBrush = 0, hOldOffBmp = 0, hOldMem;
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

