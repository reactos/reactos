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

/*
 * We handle two types of cursors/icons:
 * - Private
 *   Loaded without LR_SHARED flag
 *   Private to a process
 *   Can be deleted by calling NtDestroyCursorIcon()
 *   CurIcon->hModule, CurIcon->hRsrc and CurIcon->hGroupRsrc set to NULL
 * - Shared
 *   Loaded with LR_SHARED flag
 *   Possibly shared by multiple processes
 *   Immune to NtDestroyCursorIcon()
 *   CurIcon->hModule, CurIcon->hRsrc and CurIcon->hGroupRsrc are valid
 * There's a M:N relationship between processes and (shared) cursor/icons.
 * A process can have multiple cursor/icons and a cursor/icon can be used
 * by multiple processes. To keep track of this we keep a list of all
 * cursor/icons (CurIconList) and per cursor/icon we keep a list of
 * CURICON_PROCESS structs starting at CurIcon->ProcessList.
 */

#include <w32k.h>

static PAGED_LOOKASIDE_LIST ProcessLookasideList;
static LIST_ENTRY CurIconList;
static FAST_MUTEX CurIconListLock;

/* Look up the location of the cursor in the GDIDEVICE structure
 * when all we know is the window station object
 * Actually doesn't use the window station, but should... */
BOOL FASTCALL
IntGetCursorLocation(PWINSTATION_OBJECT WinStaObject, POINT *loc)
{
  HDC hDC;
  PDC dc;
  HBITMAP hBitmap;
  BITMAPOBJ *BitmapObj;
  SURFOBJ *SurfObj;

#if 1
  /* FIXME - get the screen dc from the window station or desktop */
  if (!(hDC = IntGetScreenDC()))
    return FALSE;
#endif

  if (!(dc = DC_LockDc(hDC)))
    return FALSE;

  hBitmap = dc->w.hBitmap;
  DC_UnlockDc(hDC);
  if (!(BitmapObj = BITMAPOBJ_LockBitmap(hBitmap)))
    return FALSE;

  SurfObj = &BitmapObj->SurfObj;
  loc->x = GDIDEV(SurfObj)->Pointer.Pos.x;
  loc->y = GDIDEV(SurfObj)->Pointer.Pos.y;

  BITMAPOBJ_UnlockBitmap(hBitmap);
  return TRUE;
}

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
   HBITMAP dcbmp, hColor = (HBITMAP)0;
   HBITMAP hMask = 0;
   SURFOBJ *soMask = NULL, *soColor = NULL;
   XLATEOBJ *XlateObj = NULL;
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
   else
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
      dcbmp = dc->w.hBitmap;
      DevInfo = dc->DevInfo;
      DC_UnlockDc(Screen);

      BitmapObj = BITMAPOBJ_LockBitmap(dcbmp);
      if ( !BitmapObj )
        return (HCURSOR)0;
      SurfObj = &BitmapObj->SurfObj;
      ASSERT(SurfObj);
   }

   if (!NewCursor && (CurInfo->CurrentCursorObject || ForceChange))
   {
      if (NULL != CurInfo->CurrentCursorObject && CurInfo->ShowingCursor)
      {
         /* Remove the cursor if it was displayed */
         if (GDIDEV(SurfObj)->Pointer.MovePointer)
           GDIDEV(SurfObj)->Pointer.MovePointer(SurfObj, -1, -1, &GDIDEV(SurfObj)->Pointer.Exclude);
         else
           EngMovePointer(SurfObj, -1, -1, &GDIDEV(SurfObj)->Pointer.Exclude);
      }

      GDIDEV(SurfObj)->Pointer.Status = SPS_ACCEPT_NOEXCLUDE;

      CurInfo->CurrentCursorObject = NewCursor; /* i.e. CurrentCursorObject = NULL */
      CurInfo->ShowingCursor = 0;
      BITMAPOBJ_UnlockBitmap(dcbmp);
      return Ret;
   }

   if (!NewCursor)
   {
      BITMAPOBJ_UnlockBitmap(dcbmp);
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
         DPRINT1("SetCursor: The Mask bitmap must have 1BPP!\n");
         BITMAPOBJ_UnlockBitmap(dcbmp);
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
            if ( !hMask )
            {
              BITMAPOBJ_UnlockBitmap(NewCursor->IconInfo.hbmMask);
              BITMAPOBJ_UnlockBitmap(dcbmp);
              return (HCURSOR)0;
            }
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
      GDIDEV(SurfObj)->Pointer.Status =
         GDIDEVFUNCS(SurfObj).SetPointerShape(
                                                        SurfObj, soMask, soColor, XlateObj,
                                                        NewCursor->IconInfo.xHotspot,
                                                        NewCursor->IconInfo.yHotspot,
                                                        GDIDEV(SurfObj)->Pointer.Pos.x,
                                                        GDIDEV(SurfObj)->Pointer.Pos.y,
                                                        &(GDIDEV(SurfObj)->Pointer.Exclude),
                                                        SPS_CHANGE);
      DPRINT("SetCursor: DrvSetPointerShape() returned %x\n",
         GDIDEV(SurfObj)->Pointer.Status);
    }
    else
    {
      GDIDEV(SurfObj)->Pointer.Status = SPS_DECLINE;
    }

    if(GDIDEV(SurfObj)->Pointer.Status == SPS_DECLINE)
    {
      GDIDEV(SurfObj)->Pointer.Status = EngSetPointerShape(
                         SurfObj, soMask, soColor, XlateObj,
                         NewCursor->IconInfo.xHotspot,
                         NewCursor->IconInfo.yHotspot,
                         GDIDEV(SurfObj)->Pointer.Pos.x,
                         GDIDEV(SurfObj)->Pointer.Pos.y,
                         &(GDIDEV(SurfObj)->Pointer.Exclude),
                         SPS_CHANGE);
      GDIDEV(SurfObj)->Pointer.MovePointer = EngMovePointer;
    }
    else
    {
      GDIDEV(SurfObj)->Pointer.MovePointer = GDIDEVFUNCS(SurfObj).MovePointer;
    }

    BITMAPOBJ_UnlockBitmap(dcbmp);
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

    if(GDIDEV(SurfObj)->Pointer.Status == SPS_ERROR)
      DPRINT1("SetCursor: DrvSetPointerShape() returned SPS_ERROR\n");

  return Ret;
}

BOOL FASTCALL
IntSetupCurIconHandles(PWINSTATION_OBJECT WinStaObject)
{
  ExInitializePagedLookasideList(&ProcessLookasideList,
				 NULL,
				 NULL,
				 0,
				 sizeof(CURICON_PROCESS),
				 0,
				 128);
  InitializeListHead(&CurIconList);
  ExInitializeFastMutex(&CurIconListLock);

  return TRUE;
}

/*
 * We have to register that this object is in use by the current
 * process. The only way to do that seems to be to walk the list
 * of cursor/icon objects starting at W32Process->CursorIconListHead.
 * If the object is already present in the list, we don't have to do
 * anything, if it's not present we add it and inc the ProcessCount
 * in the object. Having to walk the list kind of sucks, but that's
 * life...
 */
static BOOLEAN FASTCALL
ReferenceCurIconByProcess(PCURICON_OBJECT Object)
{
  PW32PROCESS Win32Process;
  PLIST_ENTRY Search;
  PCURICON_PROCESS Current;

  Win32Process = PsGetWin32Process();

  ExAcquireFastMutex(&Object->Lock);
  Search = Object->ProcessList.Flink;
  while (Search != &Object->ProcessList)
    {
      Current = CONTAINING_RECORD(Search, CURICON_PROCESS, ListEntry);
      if (Current->Process == Win32Process)
        {
          /* Already registered for this process */
          ExReleaseFastMutex(&Object->Lock);
          return TRUE;
        }
      Search = Search->Flink;
    }

  /* Not registered yet */
  Current = ExAllocateFromPagedLookasideList(&ProcessLookasideList);
  if (NULL == Current)
    {
      return FALSE;
    }
  InsertHeadList(&Object->ProcessList, &Current->ListEntry);
  Current->Process = Win32Process;

  ExReleaseFastMutex(&Object->Lock);
  return TRUE;
}

PCURICON_OBJECT FASTCALL
IntFindExistingCurIconObject(PWINSTATION_OBJECT WinStaObject, HMODULE hModule,
                             HRSRC hRsrc, LONG cx, LONG cy)
{
  PLIST_ENTRY CurrentEntry;
  PCURICON_OBJECT Object;

  ExAcquireFastMutex(&CurIconListLock);

  CurrentEntry = CurIconList.Flink;
  while (CurrentEntry != &CurIconList)
  {
    Object = CONTAINING_RECORD(CurrentEntry, CURICON_OBJECT, ListEntry);
    CurrentEntry = CurrentEntry->Flink;
    if(NT_SUCCESS(ObmReferenceObjectByPointer(Object, otCursorIcon)))
    {
      if((Object->hModule == hModule) && (Object->hRsrc == hRsrc))
      {
        if(cx && ((cx != Object->Size.cx) || (cy != Object->Size.cy)))
        {
          ObmDereferenceObject(Object);
          continue;
        }
        if (! ReferenceCurIconByProcess(Object))
        {
          ExReleaseFastMutex(&CurIconListLock);
          return NULL;
        }
        ExReleaseFastMutex(&CurIconListLock);
        return Object;
      }
    }
    ObmDereferenceObject(Object);
  }

  ExReleaseFastMutex(&CurIconListLock);

  return NULL;
}

PCURICON_OBJECT FASTCALL
IntCreateCurIconHandle(PWINSTATION_OBJECT WinStaObject)
{
  PCURICON_OBJECT Object;
  HANDLE Handle;

  Object = ObmCreateObject(WinStaObject->HandleTable, &Handle, otCursorIcon, sizeof(CURICON_OBJECT));

  if(!Object)
  {
    SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
    return FALSE;
  }

  Object->Self = Handle;
  ExInitializeFastMutex(&Object->Lock);
  InitializeListHead(&Object->ProcessList);

  if (! ReferenceCurIconByProcess(Object))
  {
    DPRINT1("Failed to add process\n");
    ObmCloseHandle(WinStaObject->HandleTable, Handle);
    ObmDereferenceObject(Object);
    return NULL;
  }

  ExAcquireFastMutex(&CurIconListLock);
  InsertHeadList(&CurIconList, &Object->ListEntry);
  ExReleaseFastMutex(&CurIconListLock);

  ObmDereferenceObject(Object);

  return Object;
}

BOOLEAN FASTCALL
IntDestroyCurIconObject(PWINSTATION_OBJECT WinStaObject, PCURICON_OBJECT Object, BOOL ProcessCleanup)
{
  PSYSTEM_CURSORINFO CurInfo;
  HBITMAP bmpMask, bmpColor;
  BOOLEAN Ret;
  PLIST_ENTRY Search;
  PCURICON_PROCESS Current = NULL;
  PW32PROCESS W32Process = PsGetWin32Process();

  ExAcquireFastMutex(&Object->Lock);

  /* Private objects can only be destroyed by their own process */
  if (NULL == Object->hModule)
    {
    ASSERT(Object->ProcessList.Flink->Flink == &Object->ProcessList);
    Current = CONTAINING_RECORD(Object->ProcessList.Flink, CURICON_PROCESS, ListEntry);
    if (Current->Process != W32Process)
      {
        ExReleaseFastMutex(&Object->Lock);
        DPRINT1("Trying to destroy private icon/cursor of another process\n");
        return FALSE;
      }
    }
  else if (! ProcessCleanup)
    {
      ExReleaseFastMutex(&Object->Lock);
      DPRINT("Trying to destroy shared icon/cursor\n");
      return FALSE;
    }

  /* Now find this process in the list of processes referencing this object and
     remove it from that list */
  Search = Object->ProcessList.Flink;
  while (Search != &Object->ProcessList)
    {
    Current = CONTAINING_RECORD(Search, CURICON_PROCESS, ListEntry);
    if (Current->Process == W32Process)
      {
      break;
      }
    Search = Search->Flink;
    }
  ASSERT(Search != &Object->ProcessList);
  RemoveEntryList(Search);
  ExFreeToPagedLookasideList(&ProcessLookasideList, Current);

  /* If there are still processes referencing this object we can't destroy it yet */
  if (! IsListEmpty(&Object->ProcessList))
    {
    ExReleaseFastMutex(&Object->Lock);
    return TRUE;
    }

  ExReleaseFastMutex(&Object->Lock);

  if (! ProcessCleanup)
    {
    ExAcquireFastMutex(&CurIconListLock);
    RemoveEntryList(&Object->ListEntry);
    ExReleaseFastMutex(&CurIconListLock);
    }

  CurInfo = IntGetSysCursorInfo(WinStaObject);

  if (CurInfo->CurrentCursorObject == Object)
  {
    /* Hide the cursor if we're destroying the current cursor */
    IntSetCursor(WinStaObject, NULL, TRUE);
  }

  bmpMask = Object->IconInfo.hbmMask;
  bmpColor = Object->IconInfo.hbmColor;

  Ret = NT_SUCCESS(ObmCloseHandle(WinStaObject->HandleTable, Object->Self));

  /* delete bitmaps */
  if(bmpMask)
  {
    GDIOBJ_SetOwnership(bmpMask, PsGetCurrentProcess());
    NtGdiDeleteObject(bmpMask);
  }
  if(bmpColor)
  {
    GDIOBJ_SetOwnership(bmpColor, PsGetCurrentProcess());
    NtGdiDeleteObject(bmpColor);
  }

  return Ret;
}

VOID FASTCALL
IntCleanupCurIcons(struct _EPROCESS *Process, PW32PROCESS Win32Process)
{
  PWINSTATION_OBJECT WinStaObject;
  PLIST_ENTRY CurrentEntry;
  PCURICON_OBJECT Object;
  PLIST_ENTRY ProcessEntry;
  PCURICON_PROCESS ProcessData;

  WinStaObject = IntGetWinStaObj();
  if(WinStaObject == NULL)
  {
    return;
  }

  ExAcquireFastMutex(&CurIconListLock);

  CurrentEntry = CurIconList.Flink;
  while (CurrentEntry != &CurIconList)
  {
    Object = CONTAINING_RECORD(CurrentEntry, CURICON_OBJECT, ListEntry);
    CurrentEntry = CurrentEntry->Flink;
    if(NT_SUCCESS(ObmReferenceObjectByPointer(Object, otCursorIcon)))
      {
      ExAcquireFastMutex(&Object->Lock);
      ProcessEntry = Object->ProcessList.Flink;
      while (ProcessEntry != &Object->ProcessList)
      {
        ProcessData = CONTAINING_RECORD(ProcessEntry, CURICON_PROCESS, ListEntry);
        if (Win32Process == ProcessData->Process)
        {
          ExReleaseFastMutex(&Object->Lock);
          RemoveEntryList(&Object->ListEntry);
          IntDestroyCurIconObject(WinStaObject, Object, TRUE);
          break;
        }
        ProcessEntry = ProcessEntry->Flink;
      }
      if (ProcessEntry == &Object->ProcessList)
      {
        ExReleaseFastMutex(&Object->Lock);
      }
      ObmDereferenceObject(Object);
    }
  }

  ExReleaseFastMutex(&CurIconListLock);
  ObDereferenceObject(WinStaObject);
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

  WinStaObject = IntGetWinStaObj();
  if(WinStaObject == NULL)
  {
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
          GDIOBJ_SetOwnership(CurIconObject->IconInfo.hbmColor, NULL);
        }
        if(CurIconObject->IconInfo.hbmMask &&
          (bmp = BITMAPOBJ_LockBitmap(CurIconObject->IconInfo.hbmMask)))
        {
          if (CurIconObject->IconInfo.hbmColor == NULL)
          {
            CurIconObject->Size.cx = bmp->SurfObj.sizlBitmap.cx;
            CurIconObject->Size.cy = bmp->SurfObj.sizlBitmap.cy / 2;
          }
          BITMAPOBJ_UnlockBitmap(CurIconObject->IconInfo.hbmMask);
          GDIOBJ_SetOwnership(CurIconObject->IconInfo.hbmMask, NULL);
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

  WinStaObject = IntGetWinStaObj();
  if(WinStaObject == NULL)
  {
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

  WinStaObject = IntGetWinStaObj();
  if(WinStaObject == NULL)
  {
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

#if 1
  HDC hDC;

  /* FIXME - get the screen dc from the window station or desktop */
  if (!(hDC = IntGetScreenDC()))
  {
    return FALSE;
  }
#endif

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

  WinStaObject = IntGetWinStaObj();
  if(WinStaObject == NULL)
  {
    return FALSE;
  }

  CurInfo = IntGetSysCursorInfo(WinStaObject);
  CursorObject = (PCURICON_OBJECT)CurInfo->CurrentCursorObject;

  SafeCi.flags = ((CurInfo->ShowingCursor && CursorObject) ? CURSOR_SHOWING : 0);
  SafeCi.hCursor = (CursorObject ? (HCURSOR)CursorObject->Self : (HCURSOR)0);

  IntGetCursorLocation(WinStaObject, &SafeCi.ptScreenPos);

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
  POINT MousePos;

  WinStaObject = IntGetWinStaObj();
  if (WinStaObject == NULL)
  {
    return FALSE;
  }

  if (NULL != UnsafeRect && ! NT_SUCCESS(MmCopyFromCaller(&Rect, UnsafeRect, sizeof(RECT))))
  {
    ObDereferenceObject(WinStaObject);
    SetLastWin32Error(ERROR_INVALID_PARAMETER);
    return FALSE;
  }

  CurInfo = IntGetSysCursorInfo(WinStaObject);
  IntGetCursorLocation(WinStaObject, &MousePos);

  if(WinStaObject->ActiveDesktop)
    DesktopWindow = IntGetWindowObject(WinStaObject->ActiveDesktop->DesktopWindow);

  if((Rect.right > Rect.left) && (Rect.bottom > Rect.top)
     && DesktopWindow && UnsafeRect != NULL)
  {
    MOUSEINPUT mi;

    CurInfo->CursorClipInfo.IsClipped = TRUE;
    CurInfo->CursorClipInfo.Left = max(Rect.left, DesktopWindow->WindowRect.left);
    CurInfo->CursorClipInfo.Top = max(Rect.top, DesktopWindow->WindowRect.top);
    CurInfo->CursorClipInfo.Right = min(Rect.right - 1, DesktopWindow->WindowRect.right - 1);
    CurInfo->CursorClipInfo.Bottom = min(Rect.bottom - 1, DesktopWindow->WindowRect.bottom - 1);
    IntReleaseWindowObject(DesktopWindow);

    mi.dx = MousePos.x;
    mi.dy = MousePos.y;
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
  PCURICON_OBJECT Object;
  NTSTATUS Status;

  WinStaObject = IntGetWinStaObj();
  if(WinStaObject == NULL)
  {
    return FALSE;
  }

  Status = ObmReferenceObjectByHandle(WinStaObject->HandleTable, Handle, otCursorIcon, (PVOID*)&Object);
  if(!NT_SUCCESS(Status))
  {
    ObDereferenceObject(WinStaObject);
    SetLastNtError(Status);
    return FALSE;
  }

  if(IntDestroyCurIconObject(WinStaObject, Object, FALSE))
  {
    ObmDereferenceObject(Object);
    ObDereferenceObject(WinStaObject);
    return TRUE;
  }

  ObmDereferenceObject(Object);
  ObDereferenceObject(WinStaObject);
  SetLastWin32Error(ERROR_INVALID_CURSOR_HANDLE);
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
  HANDLE Ret = (HANDLE)0;

  WinStaObject = IntGetWinStaObj();
  if(WinStaObject == NULL)
  {
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

  WinStaObject = IntGetWinStaObj();
  if (WinStaObject == NULL)
  {
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

  WinStaObject = IntGetWinStaObj();
  if(WinStaObject == NULL)
  {
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

  WinStaObject = IntGetWinStaObj();
  if(WinStaObject == NULL)
  {
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
      GDIOBJ_SetOwnership(CurIconObject->IconInfo.hbmColor, NULL);
    }
    else
    {
      bmp = BITMAPOBJ_LockBitmap(CurIconObject->IconInfo.hbmMask);
      if(!bmp)
        goto done;

      CurIconObject->Size.cx = bmp->SurfObj.sizlBitmap.cx;
      CurIconObject->Size.cy = bmp->SurfObj.sizlBitmap.cy / 2;

      BITMAPOBJ_UnlockBitmap(CurIconObject->IconInfo.hbmMask);
      GDIOBJ_SetOwnership(CurIconObject->IconInfo.hbmMask, NULL);
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

  WinStaObject = IntGetWinStaObj();
  if(WinStaObject == NULL)
  {
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

  WinStaObject = IntGetWinStaObj();
  if(WinStaObject == NULL)
  {
    return FALSE;
  }

  CurIconObject = IntGetCurIconObject(WinStaObject, hIcon);
  if(CurIconObject)
  {
    hbmMask = CurIconObject->IconInfo.hbmMask;
    hbmColor = CurIconObject->IconInfo.hbmColor;
    IntReleaseCurIconObject(CurIconObject);

    if(istepIfAniCur)
      DPRINT1("NtUserDrawIconEx: istepIfAniCur is not supported!\n");

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
    ObDereferenceObject(WinStaObject);

    return Ret;
  }

  SetLastWin32Error(ERROR_INVALID_CURSOR_HANDLE);
  ObDereferenceObject(WinStaObject);
  return FALSE;
}

