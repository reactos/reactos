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

#define NDEBUG
#include <debug.h>

static PAGED_LOOKASIDE_LIST gProcessLookasideList;
static LIST_ENTRY gCurIconList;

/* Look up the location of the cursor in the GDIDEVICE structure
 * when all we know is the window station object
 * Actually doesn't use the window station, but should... */
BOOL FASTCALL
IntGetCursorLocation(PWINSTATION_OBJECT WinSta, POINT *loc)
{
   HDC hDC;
   PDC dc;
   GDIDEVICE *GDIDevice;

#if 1
   /* FIXME - get the screen dc from the window station or desktop */
   if (!(hDC = IntGetScreenDC()))
      return FALSE;
#endif

   if (!(dc = DC_LockDc(hDC)))
      return FALSE;
   GDIDevice = (GDIDEVICE *)dc->GDIDevice;
   DC_UnlockDc(dc);

   loc->x = GDIDevice->Pointer.Pos.x;
   loc->y = GDIDevice->Pointer.Pos.y;

   return TRUE;
}


PCURICON_OBJECT FASTCALL UserGetCurIconObject(HCURSOR hCurIcon)
{
   PCURICON_OBJECT CurIcon;

   if (!hCurIcon)
   {
      SetLastWin32Error(ERROR_INVALID_CURSOR_HANDLE);
      return NULL;
   }

   CurIcon = (PCURICON_OBJECT)UserGetObject(gHandleTable, hCurIcon, otCursorIcon);
   if (!CurIcon)
   {
      /* we never set ERROR_INVALID_ICON_HANDLE. lets hope noone ever checks for it */
      SetLastWin32Error(ERROR_INVALID_CURSOR_HANDLE);
      return NULL;
   }

   ASSERT(USER_BODY_TO_HEADER(CurIcon)->RefCount >= 0);
   return CurIcon;
}


#define COLORCURSORS_ALLOWED FALSE
HCURSOR FASTCALL
IntSetCursor(PWINSTATION_OBJECT WinSta, PCURICON_OBJECT NewCursor,
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
   PDC dc;

   CurInfo = IntGetSysCursorInfo(WinSta);
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
      dc = DC_LockDc(Screen);

      if (!dc)
      {
         return Ret;
      }
      dcbmp = dc->w.hBitmap;
      DevInfo = dc->DevInfo;
      DC_UnlockDc(dc);

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
         IntEngMovePointer(SurfObj, -1, -1, &GDIDEV(SurfObj)->Pointer.Exclude);
      }

      GDIDEV(SurfObj)->Pointer.Status = SPS_ACCEPT_NOEXCLUDE;

      CurInfo->CurrentCursorObject = NewCursor; /* i.e. CurrentCursorObject = NULL */
      CurInfo->ShowingCursor = 0;
      BITMAPOBJ_UnlockBitmap(BitmapObj);
      return Ret;
   }

   if (!NewCursor)
   {
      BITMAPOBJ_UnlockBitmap(BitmapObj);
      return Ret;
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
         DPRINT1("SetCursor: The Mask bitmap must have 1BPP!\n");
         BITMAPOBJ_UnlockBitmap(BitmapObj);
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
                  BITMAPOBJ_UnlockBitmap(MaskBmpObj);
                  BITMAPOBJ_UnlockBitmap(BitmapObj);
                  return (HCURSOR)0;
               }
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
      GDIDEV(SurfObj)->Pointer.MovePointer = NULL;
   }
   else
   {
      GDIDEV(SurfObj)->Pointer.MovePointer = GDIDEVFUNCS(SurfObj).MovePointer;
   }

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

   if(GDIDEV(SurfObj)->Pointer.Status == SPS_ERROR)
      DPRINT1("SetCursor: DrvSetPointerShape() returned SPS_ERROR\n");

   return Ret;
}

BOOL FASTCALL
IntSetupCurIconHandles(PWINSTATION_OBJECT WinSta)
{
   ExInitializePagedLookasideList(&gProcessLookasideList,
                                  NULL,
                                  NULL,
                                  0,
                                  sizeof(CURICON_PROCESS),
                                  0,
                                  128);
   InitializeListHead(&gCurIconList);

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
ReferenceCurIconByProcess(PCURICON_OBJECT CurIcon)
{
   PW32PROCESS Win32Process;
   PCURICON_PROCESS Current;

   Win32Process = PsGetCurrentProcessWin32Process();

   LIST_FOR_EACH(Current, &CurIcon->ProcessList, CURICON_PROCESS, ListEntry)
   {
      if (Current->Process == Win32Process)
      {
         /* Already registered for this process */
         return TRUE;
      }
   }

   /* Not registered yet */
   Current = ExAllocateFromPagedLookasideList(&gProcessLookasideList);
   if (NULL == Current)
   {
      return FALSE;
   }
   InsertHeadList(&CurIcon->ProcessList, &Current->ListEntry);
   Current->Process = Win32Process;

   return TRUE;
}

PCURICON_OBJECT FASTCALL
IntFindExistingCurIconObject(PWINSTATION_OBJECT WinSta, HMODULE hModule,
                             HRSRC hRsrc, LONG cx, LONG cy)
{
   PCURICON_OBJECT CurIcon;

   LIST_FOR_EACH(CurIcon, &gCurIconList, CURICON_OBJECT, ListEntry)
   {

      //    if(NT_SUCCESS(ObmReferenceObjectByPointer(Object, otCursorIcon))) //<- huh????
//      ObmReferenceObject(  CurIcon);
//      {
      if((CurIcon->hModule == hModule) && (CurIcon->hRsrc == hRsrc))
      {
         if(cx && ((cx != CurIcon->Size.cx) || (cy != CurIcon->Size.cy)))
         {
//               ObmDereferenceObject(CurIcon);
            continue;
         }
         if (! ReferenceCurIconByProcess(CurIcon))
         {
            return NULL;
         }

         return CurIcon;
      }
//      }
//      ObmDereferenceObject(CurIcon);

   }

   return NULL;
}

PCURICON_OBJECT FASTCALL
IntCreateCurIconHandle(PWINSTATION_OBJECT WinSta)
{
   PCURICON_OBJECT CurIcon;
   HANDLE hCurIcon;

   CurIcon = ObmCreateObject(gHandleTable, &hCurIcon, otCursorIcon, sizeof(CURICON_OBJECT));

   if(!CurIcon)
   {
      SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
      return FALSE;
   }

   CurIcon->Self = hCurIcon;
   InitializeListHead(&CurIcon->ProcessList);

   if (! ReferenceCurIconByProcess(CurIcon))
   {
      DPRINT1("Failed to add process\n");
      ObmDeleteObject(hCurIcon, otCursorIcon);
      ObmDereferenceObject(CurIcon);
      return NULL;
   }

   InsertHeadList(&gCurIconList, &CurIcon->ListEntry);

   ObmDereferenceObject(CurIcon);

   return CurIcon;
}

BOOLEAN FASTCALL
IntDestroyCurIconObject(PWINSTATION_OBJECT WinSta, PCURICON_OBJECT CurIcon, BOOL ProcessCleanup)
{
   PSYSTEM_CURSORINFO CurInfo;
   HBITMAP bmpMask, bmpColor;
   BOOLEAN Ret;
   PCURICON_PROCESS Current = NULL;
   PW32PROCESS W32Process = PsGetCurrentProcessWin32Process();

   /* Private objects can only be destroyed by their own process */
   if (NULL == CurIcon->hModule)
   {
      ASSERT(CurIcon->ProcessList.Flink->Flink == &CurIcon->ProcessList);
      Current = CONTAINING_RECORD(CurIcon->ProcessList.Flink, CURICON_PROCESS, ListEntry);
      if (Current->Process != W32Process)
      {
         DPRINT1("Trying to destroy private icon/cursor of another process\n");
         return FALSE;
      }
   }
   else if (! ProcessCleanup)
   {
      DPRINT("Trying to destroy shared icon/cursor\n");
      return FALSE;
   }

   /* Now find this process in the list of processes referencing this object and
      remove it from that list */
   LIST_FOR_EACH(Current, &CurIcon->ProcessList, CURICON_PROCESS, ListEntry)
   {
      if (Current->Process == W32Process)
      {
         RemoveEntryList(&Current->ListEntry);
         break;
      }
   }

   ExFreeToPagedLookasideList(&gProcessLookasideList, Current);

   /* If there are still processes referencing this object we can't destroy it yet */
   if (! IsListEmpty(&CurIcon->ProcessList))
   {
      return TRUE;
   }


   if (! ProcessCleanup)
   {
      RemoveEntryList(&CurIcon->ListEntry);
   }

   CurInfo = IntGetSysCursorInfo(WinSta);

   if (CurInfo->CurrentCursorObject == CurIcon)
   {
      /* Hide the cursor if we're destroying the current cursor */
      IntSetCursor(WinSta, NULL, TRUE);
   }

   bmpMask = CurIcon->IconInfo.hbmMask;
   bmpColor = CurIcon->IconInfo.hbmColor;

   Ret = ObmDeleteObject(CurIcon->Self, otCursorIcon);

   /* delete bitmaps */
   if(bmpMask)
   {
      GDIOBJ_SetOwnership(GdiHandleTable, bmpMask, PsGetCurrentProcess());
      NtGdiDeleteObject(bmpMask);
   }
   if(bmpColor)
   {
      GDIOBJ_SetOwnership(GdiHandleTable, bmpColor, PsGetCurrentProcess());
      NtGdiDeleteObject(bmpColor);
   }

   return Ret;
}

VOID FASTCALL
IntCleanupCurIcons(struct _EPROCESS *Process, PW32PROCESS Win32Process)
{
   PWINSTATION_OBJECT WinSta;
   PCURICON_OBJECT CurIcon, tmp;
   PCURICON_PROCESS ProcessData;

   WinSta = IntGetWinStaObj();
   if(WinSta == NULL)
   {
      return;
   }

   LIST_FOR_EACH_SAFE(CurIcon, tmp, &gCurIconList, CURICON_OBJECT, ListEntry)
   {
//      ObmReferenceObject(CurIcon);
      //    if(NT_SUCCESS(ObmReferenceObjectByPointer(Object, otCursorIcon)))
      {
         LIST_FOR_EACH(ProcessData, &CurIcon->ProcessList, CURICON_PROCESS, ListEntry)
         {
            if (Win32Process == ProcessData->Process)
            {
               RemoveEntryList(&CurIcon->ListEntry);
               IntDestroyCurIconObject(WinSta, CurIcon, TRUE);
               break;
            }
         }

//         ObmDereferenceObject(Object);
      }


   }

   ObDereferenceObject(WinSta);
}

/*
 * @implemented
 */
HANDLE
STDCALL
NtUserCreateCursorIconHandle(PICONINFO IconInfo OPTIONAL, BOOL Indirect)
{
   PCURICON_OBJECT CurIcon;
   PWINSTATION_OBJECT WinSta;
   PBITMAPOBJ bmp;
   NTSTATUS Status;
   HANDLE Ret;
   DECLARE_RETURN(HANDLE);

   DPRINT("Enter NtUserCreateCursorIconHandle\n");
   UserEnterExclusive();

   WinSta = IntGetWinStaObj();
   if(WinSta == NULL)
   {
      RETURN( (HANDLE)0);
   }

   if (!(CurIcon = IntCreateCurIconHandle(WinSta)))
   {
      SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
      ObDereferenceObject(WinSta);
      RETURN( (HANDLE)0);
   }

   Ret = CurIcon->Self;

   if(IconInfo)
   {
      Status = MmCopyFromCaller(&CurIcon->IconInfo, IconInfo, sizeof(ICONINFO));
      if(NT_SUCCESS(Status))
      {
         if(Indirect)
         {
            CurIcon->IconInfo.hbmMask = BITMAPOBJ_CopyBitmap(CurIcon->IconInfo.hbmMask);
            CurIcon->IconInfo.hbmColor = BITMAPOBJ_CopyBitmap(CurIcon->IconInfo.hbmColor);
         }
         if(CurIcon->IconInfo.hbmColor &&
               (bmp = BITMAPOBJ_LockBitmap(CurIcon->IconInfo.hbmColor)))
         {
            CurIcon->Size.cx = bmp->SurfObj.sizlBitmap.cx;
            CurIcon->Size.cy = bmp->SurfObj.sizlBitmap.cy;
            BITMAPOBJ_UnlockBitmap(bmp);
            GDIOBJ_SetOwnership(GdiHandleTable, CurIcon->IconInfo.hbmColor, NULL);
         }
         if(CurIcon->IconInfo.hbmMask &&
               (bmp = BITMAPOBJ_LockBitmap(CurIcon->IconInfo.hbmMask)))
         {
            if (CurIcon->IconInfo.hbmColor == NULL)
            {
               CurIcon->Size.cx = bmp->SurfObj.sizlBitmap.cx;
               CurIcon->Size.cy = bmp->SurfObj.sizlBitmap.cy / 2;
            }
            BITMAPOBJ_UnlockBitmap(bmp);
            GDIOBJ_SetOwnership(GdiHandleTable, CurIcon->IconInfo.hbmMask, NULL);
         }
      }
      else
      {
         SetLastNtError(Status);
         /* FIXME - Don't exit here */
      }
   }

   ObDereferenceObject(WinSta);
   RETURN( Ret);

CLEANUP:
   DPRINT("Leave NtUserCreateCursorIconHandle, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}

/*
 * @implemented
 */
BOOL
STDCALL
NtUserGetIconInfo(
   HANDLE hCurIcon,
   PICONINFO IconInfo, DWORD UnkOne, DWORD UnkTwo, DWORD UnkThree, DWORD UnkFour )
{
   ICONINFO ii;
   PCURICON_OBJECT CurIcon;
   PWINSTATION_OBJECT WinSta;
   NTSTATUS Status;
   BOOL Ret = FALSE;
   DECLARE_RETURN(BOOL);

   DPRINT("Enter NtUserGetIconInfo\n");
   UserEnterExclusive();

   if(!IconInfo)
   {
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      RETURN(FALSE);
   }

   WinSta = IntGetWinStaObj();
   if(WinSta == NULL)
   {
      RETURN( FALSE);
   }

   if (!(CurIcon = UserGetCurIconObject(hCurIcon)))
   {
      ObDereferenceObject(WinSta);
      RETURN( FALSE);
   }

   RtlCopyMemory(&ii, &CurIcon->IconInfo, sizeof(ICONINFO));

   /* Copy bitmaps */
   ii.hbmMask = BITMAPOBJ_CopyBitmap(ii.hbmMask);
   ii.hbmColor = BITMAPOBJ_CopyBitmap(ii.hbmColor);

   /* Copy fields */
   Status = MmCopyToCaller(IconInfo, &ii, sizeof(ICONINFO));
   if(NT_SUCCESS(Status))
      Ret = TRUE;
   else
      SetLastNtError(Status);

   ObDereferenceObject(WinSta);
   RETURN( Ret);

CLEANUP:
   DPRINT("Leave NtUserGetIconInfo, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}


/*
 * @implemented
 */
BOOL
STDCALL
NtUserGetCursorIconSize(
   HANDLE hCurIcon,
   BOOL *fIcon,
   SIZE *Size)
{
   PCURICON_OBJECT CurIcon;
   PBITMAPOBJ bmp;
   PWINSTATION_OBJECT WinSta;
   NTSTATUS Status;
   BOOL Ret = FALSE;
   SIZE SafeSize;
   DECLARE_RETURN(BOOL);

   DPRINT("Enter NtUserGetCursorIconSize\n");
   UserEnterExclusive();

   WinSta = IntGetWinStaObj();
   if(WinSta == NULL)
   {
      RETURN( FALSE);
   }

   if (!(CurIcon = UserGetCurIconObject(hCurIcon)))
   {
      ObDereferenceObject(WinSta);
      RETURN(FALSE);
   }

   /* Copy fields */
   Status = MmCopyToCaller(fIcon, &CurIcon->IconInfo.fIcon, sizeof(BOOL));
   if(!NT_SUCCESS(Status))
   {
      SetLastNtError(Status);
      goto done;
   }

   bmp = BITMAPOBJ_LockBitmap(CurIcon->IconInfo.hbmColor);
   if(!bmp)
      goto done;

   SafeSize.cx = bmp->SurfObj.sizlBitmap.cx;
   SafeSize.cy = bmp->SurfObj.sizlBitmap.cy;
   Status = MmCopyToCaller(Size, &SafeSize, sizeof(SIZE));
   if(NT_SUCCESS(Status))
      Ret = TRUE;
   else
      SetLastNtError(Status);

   BITMAPOBJ_UnlockBitmap(bmp);

done:
   ObDereferenceObject(WinSta);
   RETURN( Ret);

CLEANUP:
   DPRINT("Leave NtUserGetCursorIconSize, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
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
   PWINSTATION_OBJECT WinSta;
   NTSTATUS Status;
   PCURICON_OBJECT CurIcon;
   HDC hDC;
   DECLARE_RETURN(BOOL);

   DPRINT("Enter NtUserGetCursorInfo\n");
   UserEnterExclusive();

#if 1


   /* FIXME - get the screen dc from the window station or desktop */
   if (!(hDC = IntGetScreenDC()))
   {
      RETURN( FALSE);
   }
#endif

   Status = MmCopyFromCaller(&SafeCi.cbSize, pci, sizeof(DWORD));
   if(!NT_SUCCESS(Status))
   {
      SetLastNtError(Status);
      RETURN( FALSE);
   }

   if(SafeCi.cbSize != sizeof(CURSORINFO))
   {
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      RETURN( FALSE);
   }

   WinSta = IntGetWinStaObj();
   if(WinSta == NULL)
   {
      RETURN( FALSE);
   }

   CurInfo = IntGetSysCursorInfo(WinSta);
   CurIcon = (PCURICON_OBJECT)CurInfo->CurrentCursorObject;

   SafeCi.flags = ((CurInfo->ShowingCursor && CurIcon) ? CURSOR_SHOWING : 0);
   SafeCi.hCursor = (CurIcon ? (HCURSOR)CurIcon->Self : (HCURSOR)0);

   IntGetCursorLocation(WinSta, &SafeCi.ptScreenPos);

   Status = MmCopyToCaller(pci, &SafeCi, sizeof(CURSORINFO));
   if(!NT_SUCCESS(Status))
   {
      ObDereferenceObject(WinSta);
      SetLastNtError(Status);
      RETURN( FALSE);
   }

   ObDereferenceObject(WinSta);
   RETURN( TRUE);

CLEANUP:
   DPRINT("Leave NtUserGetCursorInfo, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
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

   PWINSTATION_OBJECT WinSta;
   PSYSTEM_CURSORINFO CurInfo;
   RECT Rect;
   PWINDOW_OBJECT DesktopWindow = NULL;
   POINT MousePos = {0};
   DECLARE_RETURN(BOOL);

   DPRINT("Enter NtUserClipCursor\n");
   UserEnterExclusive();

   WinSta = IntGetWinStaObj();
   if (WinSta == NULL)
   {
      RETURN( FALSE);
   }

   if (NULL != UnsafeRect && ! NT_SUCCESS(MmCopyFromCaller(&Rect, UnsafeRect, sizeof(RECT))))
   {
      ObDereferenceObject(WinSta);
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      RETURN( FALSE);
   }

   CurInfo = IntGetSysCursorInfo(WinSta);
   IntGetCursorLocation(WinSta, &MousePos);

   if(WinSta->ActiveDesktop)
      DesktopWindow = UserGetWindowObject(WinSta->ActiveDesktop->DesktopWindow);

   if((Rect.right > Rect.left) && (Rect.bottom > Rect.top)
         && DesktopWindow && UnsafeRect != NULL)
   {
      MOUSEINPUT mi;

      CurInfo->CursorClipInfo.IsClipped = TRUE;
      CurInfo->CursorClipInfo.Left = max(Rect.left, DesktopWindow->WindowRect.left);
      CurInfo->CursorClipInfo.Top = max(Rect.top, DesktopWindow->WindowRect.top);
      CurInfo->CursorClipInfo.Right = min(Rect.right - 1, DesktopWindow->WindowRect.right - 1);
      CurInfo->CursorClipInfo.Bottom = min(Rect.bottom - 1, DesktopWindow->WindowRect.bottom - 1);

      mi.dx = MousePos.x;
      mi.dy = MousePos.y;
      mi.mouseData = 0;
      mi.dwFlags = MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE;
      mi.time = 0;
      mi.dwExtraInfo = 0;
      IntMouseInput(&mi);

      RETURN( TRUE);
   }

   CurInfo->CursorClipInfo.IsClipped = FALSE;
   ObDereferenceObject(WinSta);

   RETURN( TRUE);

CLEANUP:
   DPRINT("Leave NtUserClipCursor, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}


/*
 * @implemented
 */
BOOL
STDCALL
NtUserDestroyCursor(
   HANDLE hCurIcon,
   DWORD Unknown)
{
   PWINSTATION_OBJECT WinSta;
   PCURICON_OBJECT CurIcon;
   BOOL ret;
   DECLARE_RETURN(BOOL);

   DPRINT("Enter NtUserDestroyCursorIcon\n");
   UserEnterExclusive();

   WinSta = IntGetWinStaObj();
   if(WinSta == NULL)
   {
      RETURN( FALSE);
   }

   if (!(CurIcon = UserGetCurIconObject(hCurIcon)))
   {
      ObDereferenceObject(WinSta);
      RETURN(FALSE);
   }

   ret = IntDestroyCurIconObject(WinSta, CurIcon, FALSE);

   ObDereferenceObject(WinSta);
   RETURN(ret);

CLEANUP:
   DPRINT("Leave NtUserDestroyCursorIcon, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
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
   PCURICON_OBJECT CurIcon;
   PWINSTATION_OBJECT WinSta;
   HANDLE Ret = (HANDLE)0;
   DECLARE_RETURN(HICON);

   DPRINT("Enter NtUserFindExistingCursorIcon\n");
   UserEnterExclusive();

   WinSta = IntGetWinStaObj();
   if(WinSta == NULL)
   {
      RETURN( Ret);
   }

   CurIcon = IntFindExistingCurIconObject(WinSta, hModule, hRsrc, cx, cy);
   if(CurIcon)
   {
      Ret = CurIcon->Self;

//      IntReleaseCurIconObject(CurIcon);//faxme: is this correct? does IntFindExistingCurIconObject add a ref?
      ObDereferenceObject(WinSta);
      RETURN( Ret);
   }

   SetLastWin32Error(ERROR_INVALID_CURSOR_HANDLE);
   ObDereferenceObject(WinSta);
   RETURN( (HANDLE)0);

CLEANUP:
   DPRINT("Leave NtUserFindExistingCursorIcon, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
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
   PWINSTATION_OBJECT WinSta;
   RECT Rect;
   NTSTATUS Status;
   DECLARE_RETURN(BOOL);

   DPRINT("Enter NtUserGetClipCursor\n");
   UserEnterExclusive();

   if(!lpRect)
      RETURN( FALSE);

   WinSta = IntGetWinStaObj();
   if (WinSta == NULL)
   {
      RETURN( FALSE);
   }

   CurInfo = IntGetSysCursorInfo(WinSta);
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
      Rect.right = UserGetSystemMetrics(SM_CXSCREEN);
      Rect.bottom = UserGetSystemMetrics(SM_CYSCREEN);
   }

   Status = MmCopyToCaller((PRECT)lpRect, &Rect, sizeof(RECT));
   if(!NT_SUCCESS(Status))
   {
      ObDereferenceObject(WinSta);
      SetLastNtError(Status);
      RETURN( FALSE);
   }

   ObDereferenceObject(WinSta);

   RETURN( TRUE);

CLEANUP:
   DPRINT("Leave NtUserGetClipCursor, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}


/*
 * @implemented
 */
HCURSOR
STDCALL
NtUserSetCursor(
   HCURSOR hCursor)
{
   PCURICON_OBJECT CurIcon;
   HICON OldCursor;
   PWINSTATION_OBJECT WinSta;
   DECLARE_RETURN(HCURSOR);

   DPRINT("Enter NtUserSetCursor\n");
   UserEnterExclusive();

   WinSta = IntGetWinStaObj();
   if(WinSta == NULL)
   {
      RETURN(NULL);
   }

   if(!(CurIcon = UserGetCurIconObject(hCursor)))
   {
      ObDereferenceObject(WinSta);
      RETURN(NULL);
   }

   OldCursor = IntSetCursor(WinSta, CurIcon, FALSE);

   ObDereferenceObject(WinSta);

   RETURN(OldCursor);

CLEANUP:
   DPRINT("Leave NtUserSetCursor, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}


/*
 * @implemented
 */
BOOL
STDCALL
NtUserSetCursorContents(
   HANDLE hCurIcon,
   PICONINFO IconInfo)
{
   PCURICON_OBJECT CurIcon;
   PBITMAPOBJ bmp;
   PWINSTATION_OBJECT WinSta;
   NTSTATUS Status;
   BOOL Ret = FALSE;
   DECLARE_RETURN(BOOL);

   DPRINT("Enter NtUserSetCursorContents\n");
   UserEnterExclusive();

   WinSta = IntGetWinStaObj();
   if(WinSta == NULL)
   {
      RETURN( FALSE);
   }

   if (!(CurIcon = UserGetCurIconObject(hCurIcon)))
   {
      ObDereferenceObject(WinSta);
      RETURN(FALSE);
   }

   /* Copy fields */
   Status = MmCopyFromCaller(&CurIcon->IconInfo, IconInfo, sizeof(ICONINFO));
   if(!NT_SUCCESS(Status))
   {
      SetLastNtError(Status);
      goto done;
   }

   bmp = BITMAPOBJ_LockBitmap(CurIcon->IconInfo.hbmColor);
   if(bmp)
   {
      CurIcon->Size.cx = bmp->SurfObj.sizlBitmap.cx;
      CurIcon->Size.cy = bmp->SurfObj.sizlBitmap.cy;
      BITMAPOBJ_UnlockBitmap(bmp);
      GDIOBJ_SetOwnership(GdiHandleTable, CurIcon->IconInfo.hbmColor, NULL);
   }
   else
   {
      bmp = BITMAPOBJ_LockBitmap(CurIcon->IconInfo.hbmMask);
      if(!bmp)
         goto done;

      CurIcon->Size.cx = bmp->SurfObj.sizlBitmap.cx;
      CurIcon->Size.cy = bmp->SurfObj.sizlBitmap.cy / 2;

      BITMAPOBJ_UnlockBitmap(bmp);
      GDIOBJ_SetOwnership(GdiHandleTable, CurIcon->IconInfo.hbmMask, NULL);
   }

   Ret = TRUE;

done:

   ObDereferenceObject(WinSta);
   RETURN( Ret);

CLEANUP:
   DPRINT("Leave NtUserSetCursorContents, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}


/*
 * @implemented
 */
BOOL
STDCALL
NtUserSetCursorIconData(
   HANDLE hCurIcon,
   PBOOL fIcon,
   POINT *Hotspot,
   HMODULE hModule,
   HRSRC hRsrc,
   HRSRC hGroupRsrc)
{
   PCURICON_OBJECT CurIcon;
   PWINSTATION_OBJECT WinSta;
   NTSTATUS Status;
   POINT SafeHotspot;
   BOOL Ret = FALSE;
   DECLARE_RETURN(BOOL);

   DPRINT("Enter NtUserSetCursorIconData\n");
   UserEnterExclusive();

   WinSta = IntGetWinStaObj();
   if(WinSta == NULL)
   {
      RETURN( FALSE);
   }

   if(!(CurIcon = UserGetCurIconObject(hCurIcon)))
   {
      ObDereferenceObject(WinSta);
      RETURN(FALSE);
   }

   CurIcon->hModule = hModule;
   CurIcon->hRsrc = hRsrc;
   CurIcon->hGroupRsrc = hGroupRsrc;

   /* Copy fields */
   if(fIcon)
   {
      Status = MmCopyFromCaller(&CurIcon->IconInfo.fIcon, fIcon, sizeof(BOOL));
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
         CurIcon->IconInfo.xHotspot = SafeHotspot.x;
         CurIcon->IconInfo.yHotspot = SafeHotspot.y;

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
   ObDereferenceObject(WinSta);
   RETURN( Ret);


CLEANUP:
   DPRINT("Leave NtUserSetCursorIconData, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
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


#define STRETCH_CAN_SRCCOPY_ONLY

#ifdef STRETCH_CAN_SRCCOPY_ONLY
void
FASTCALL
DoStretchBlt(HDC DcDest, int XDest, int YDest, int WidthDest, int HeightDest,
             HDC DcSrc, int XSrc, int YSrc, int WidthSrc, int HeightSrc,
             DWORD Rop3, BOOL Color)
{
   HDC DcStretched;
   HBITMAP BitmapStretched;
   HBITMAP OldBitmap;

   if (WidthDest == WidthSrc && HeightDest == HeightSrc)
   {
      NtGdiBitBlt(DcDest, XDest, YDest, WidthDest, HeightDest,
                  DcSrc, XSrc, YSrc, Rop3, 0, 0);
   }
   else if (SRCCOPY == Rop3)
   {
      NtGdiStretchBlt(DcDest, XDest, YDest, WidthDest, HeightDest,
                      DcSrc, XSrc, YSrc, WidthSrc, HeightSrc,
                      Rop3, 0);
   }
   else
   {
      DcStretched = NtGdiCreateCompatibleDC(DcSrc);
      if (NULL == DcStretched)
      {
         DPRINT1("Failed to create compatible DC\n");
         return;
      }
      if (Color)
      {
         BitmapStretched = NtGdiCreateCompatibleBitmap(DcDest, WidthDest,
                                                       HeightDest);
      }
      else
      {
         BitmapStretched = IntGdiCreateBitmap(WidthDest, HeightDest, 1, 1, NULL);
      }
      if (NULL == BitmapStretched)
      {
         NtGdiDeleteObjectApp(DcStretched);
         DPRINT1("Failed to create temporary bitmap\n");
         return;
      }
      OldBitmap = NtGdiSelectObject(DcStretched, BitmapStretched);
      if (NULL == OldBitmap)
      {
         NtGdiDeleteObject(BitmapStretched);
         NtGdiDeleteObjectApp(DcStretched);
         DPRINT1("Failed to create temporary bitmap\n");
         return;
      }
      if (! NtGdiStretchBlt(DcStretched, 0, 0, WidthDest, HeightDest,
                            DcSrc, XSrc, YSrc, WidthSrc, HeightSrc,
                            SRCCOPY, 0) ||
          ! NtGdiBitBlt(DcDest, XDest, YDest, WidthDest, HeightDest,
                        DcStretched, 0, 0, Rop3, 0, 0))
      {
         DPRINT1("Failed to blt\n");
      }
      NtGdiSelectObject(DcStretched, OldBitmap);
      NtGdiDeleteObject(BitmapStretched);
      NtGdiDeleteObjectApp(DcStretched);
   }
}
#else
#define DoStretchBlt(DcDest, XDest, YDest, WidthDest, HeightDest, \
                     DcSrc, XSrc, YSrc, WidthSrc, HeightSrc, Rop3, Color) \
        NtGdiStretchBlt((DcDest), (XDest), (YDest), (WidthDest), (HeightDest), \
                        (DcSrc), (XSrc), (YSrc), (WidthSrc), (HeightSrc), \
                        (Rop3), 0)
#endif /* STRETCH_CAN_SRCCOPY_ONLY */

BOOL
UserDrawIconEx(
   HDC hDc,
   INT xLeft,
   INT yTop,
   PCURICON_OBJECT pIcon,
   INT cxWidth,
   INT cyHeight,
   UINT istepIfAniCur,
   HBRUSH hbrFlickerFreeDraw,
   UINT diFlags)
{
   BOOL Ret = FALSE;
   HBITMAP hbmMask, hbmColor;
   BITMAP bmpMask, bmpColor;
   COLORREF oldFg, oldBg;
   BOOL DoFlickerFree;
   INT nStretchMode;
   SIZE IconSize;

   HDC hdcOff;
   HGDIOBJ hOldOffBrush = 0;
   HGDIOBJ hOldOffBmp = 0;
   HBITMAP hbmOff = 0;
   HDC hdcMem = 0;
   HGDIOBJ hOldMem;
   BOOL bAlpha = FALSE;

   hbmMask = pIcon->IconInfo.hbmMask;
   hbmColor = pIcon->IconInfo.hbmColor;

   if (istepIfAniCur)
      DPRINT1("NtUserDrawIconEx: istepIfAniCur is not supported!\n");

   if (!hbmMask || !IntGdiGetObject(hbmMask, sizeof(BITMAP), &bmpMask))
   {
      return FALSE;
   }

   if (hbmColor && !IntGdiGetObject(hbmColor, sizeof(BITMAP), &bmpColor))
   {
      return FALSE;
   }

   if (hbmColor)
   {
      IconSize.cx = bmpColor.bmWidth;
      IconSize.cy = bmpColor.bmHeight;
   }
   else
   {
      IconSize.cx = bmpMask.bmWidth;
      IconSize.cy = bmpMask.bmHeight / 2;
   }

   if (bmpColor.bmBitsPixel == 32)
   {
      bAlpha = TRUE;
   }

   if (!diFlags)
      diFlags = DI_NORMAL;

   if (!cxWidth)
      cxWidth = ((diFlags & DI_DEFAULTSIZE) ?
         UserGetSystemMetrics(SM_CXICON) : IconSize.cx);

   if (!cyHeight)
      cyHeight = ((diFlags & DI_DEFAULTSIZE) ?
         UserGetSystemMetrics(SM_CYICON) : IconSize.cy);

   DoFlickerFree = (hbrFlickerFreeDraw &&
      (GDI_HANDLE_GET_TYPE(hbrFlickerFreeDraw) == GDI_OBJECT_TYPE_BRUSH));

   if (DoFlickerFree || bAlpha)
   {
      RECT r;
      r.right = cxWidth;
      r.bottom = cyHeight;

      hdcOff = NtGdiCreateCompatibleDC(hDc);
      if (!hdcOff)
      {
         DPRINT1("NtGdiCreateCompatibleDC() failed!\n");
         return FALSE;
      }

      hbmOff = NtGdiCreateCompatibleBitmap(hDc, cxWidth, cyHeight);
      if (!hbmOff)
      {
         DPRINT1("NtGdiCreateCompatibleBitmap() failed!\n");
         goto cleanup;
      }

      hOldOffBmp = NtGdiSelectObject(hdcOff, hbmOff);
      if (!hOldOffBmp)
      {
         DPRINT1("NtGdiSelectObject() failed!\n");
         goto cleanup;
      }

      if (DoFlickerFree)
      {
          hOldOffBrush = NtGdiSelectObject(hdcOff, hbrFlickerFreeDraw);
          if (!hOldOffBrush)
          {
             DPRINT1("NtGdiSelectObject() failed!\n");
             goto cleanup;
          }

          NtGdiPatBlt(hdcOff, 0, 0, r.right, r.bottom, PATCOPY);
      }
   }
   else
       hdcOff = hDc;

   hdcMem = NtGdiCreateCompatibleDC(hDc);
   if (!hdcMem)
   {
      DPRINT1("NtGdiCreateCompatibleDC() failed!\n");
      goto cleanup;
   }

   nStretchMode = NtGdiSetStretchBltMode(hdcOff, STRETCH_DELETESCANS);

   oldFg = NtGdiSetTextColor(hdcOff, RGB(0, 0, 0));
   oldBg = NtGdiSetBkColor(hdcOff, RGB(255, 255, 255));

   if (diFlags & DI_MASK)
   {
      hOldMem = NtGdiSelectObject(hdcMem, hbmMask);
      if (!hOldMem)
      {
         DPRINT("NtGdiSelectObject() failed!\n");
         goto cleanup;
      }

      DoStretchBlt(hdcOff, (DoFlickerFree ? 0 : xLeft),
                   (DoFlickerFree ? 0 : yTop), cxWidth, cyHeight, hdcMem,
                   0, 0, IconSize.cx, IconSize.cy,
                   ((diFlags & DI_IMAGE) ? SRCAND : SRCCOPY), FALSE);

      if (!hbmColor && (bmpMask.bmHeight == 2 * bmpMask.bmWidth)
         && (diFlags & DI_IMAGE))
      {
         DoStretchBlt(hdcOff, (DoFlickerFree ? 0 : xLeft),
                      (DoFlickerFree ? 0 : yTop), cxWidth, cyHeight, hdcMem,
                      0, IconSize.cy, IconSize.cx, IconSize.cy, SRCINVERT,
                      FALSE);

         diFlags &= ~DI_IMAGE;
      }

      NtGdiSelectObject(hdcMem, hOldMem);
   }

   if(diFlags & DI_IMAGE)
   {
      hOldMem = NtGdiSelectObject(hdcMem, (hbmColor ? hbmColor : hbmMask));

      DoStretchBlt(hdcOff, (DoFlickerFree ? 0 : xLeft),
                   (DoFlickerFree ? 0 : yTop), cxWidth, cyHeight, hdcMem,
                   0, (hbmColor ? 0 : IconSize.cy), IconSize.cx, IconSize.cy,
                   ((diFlags & DI_MASK) ? SRCINVERT : SRCCOPY),
                   NULL != hbmColor);

      NtGdiSelectObject(hdcMem, hOldMem);
   }

    if (bAlpha)
    {
        BITMAP bm;
        BITMAPOBJ *BitmapObj = NULL;
        PBYTE pBits = NULL;
        BLENDFUNCTION  BlendFunc;
        BYTE Red, Green, Blue, Alpha;
        DWORD Count = 0;
        INT i, j;

        BitmapObj = BITMAPOBJ_LockBitmap(hbmOff);
        if (BitmapObj == NULL)
        {
            DPRINT1("GDIOBJ_LockObj() failed!\n");
            goto cleanup;
        }
        BITMAP_GetObject(BitmapObj, sizeof(BITMAP), &bm);

        pBits = ExAllocatePoolWithTag(PagedPool, bm.bmWidthBytes * abs(bm.bmHeight), TAG_BITMAP);
        if (pBits == NULL)
        {
            DPRINT1("ExAllocatePoolWithTag() failed!\n");
            BITMAPOBJ_UnlockBitmap(BitmapObj);
            goto cleanup;
        }

        /* get icon bits */
        IntGetBitmapBits(BitmapObj, bm.bmWidthBytes * abs(bm.bmHeight), pBits);

        /* premultiply with the alpha channel value */
        for (i = 0; i < cyHeight; i++)
        {
            for (j = 0; j < cxWidth; j++)
            {
                DWORD OrigPixel = 0;
                DWORD AlphaPixel = 0;

                OrigPixel = *(DWORD *)(pBits + Count);

                Red   = (BYTE)((OrigPixel >>  0) & 0xff);
                Green = (BYTE)((OrigPixel >>  8) & 0xff);
                Blue  = (BYTE)((OrigPixel >> 16) & 0xff);
                Alpha = (BYTE)((OrigPixel >> 24) & 0xff);

                Red = (Red * Alpha) / 0xff;
                Green = (Green * Alpha) / 0xff;
                Blue = (Blue * Alpha) / 0xff;

                AlphaPixel = (DWORD)(Red | (Green << 8) | (Blue << 16) | (Alpha << 24));

                *(DWORD *)(pBits + Count) = AlphaPixel;
                Count += sizeof (DWORD);
            }
        }

        /* set icon bits */
        IntSetBitmapBits(BitmapObj, bm.bmWidthBytes * abs(bm.bmHeight), pBits);
        ExFreePool(pBits);

        BITMAPOBJ_UnlockBitmap(BitmapObj);

        BlendFunc.BlendOp = AC_SRC_OVER;
        BlendFunc.BlendFlags = 0;
        BlendFunc.SourceConstantAlpha = 255;
        BlendFunc.AlphaFormat = AC_SRC_ALPHA;

        NtGdiAlphaBlend(hDc, xLeft, yTop, cxWidth, cyHeight,
                        hdcOff, 0, 0, cxWidth, cyHeight, BlendFunc, 0);
    }
    else if (DoFlickerFree)
    {
        NtGdiBitBlt(hDc, xLeft, yTop, cxWidth,
                    cyHeight, hdcOff, 0, 0, SRCCOPY, 0, 0);
    }

   NtGdiSetTextColor(hdcOff, oldFg);
   NtGdiSetBkColor(hdcOff, oldBg);
   NtGdiSetStretchBltMode(hdcOff, nStretchMode);

   Ret = TRUE;

cleanup:
   if(DoFlickerFree)
   {
      if(hOldOffBmp) NtGdiSelectObject(hdcOff, hOldOffBmp);
      if(hOldOffBrush) NtGdiSelectObject(hdcOff, hOldOffBrush);
      if(hbmOff) NtGdiDeleteObject(hbmOff);
      if(hdcOff) NtGdiDeleteObjectApp(hdcOff);
   }

   if(hdcMem) NtGdiDeleteObjectApp(hdcMem);
   return Ret;
}

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
   int cyHeight,
   UINT istepIfAniCur,
   HBRUSH hbrFlickerFreeDraw,
   UINT diFlags,
   DWORD Unknown0,
   DWORD Unknown1)
{
   PCURICON_OBJECT pIcon;
   BOOL Ret;

   DPRINT("Enter NtUserDrawIconEx\n");
   UserEnterExclusive();

   if(!(pIcon = UserGetCurIconObject(hIcon)))
   {
      DPRINT1("UserGetCurIconObject() failed!\n");
      UserLeave();
      return FALSE;
   }

   Ret = UserDrawIconEx(hdc,
      xLeft,
      yTop,
      pIcon,
      cxWidth,
      cyHeight,
      istepIfAniCur,
      hbrFlickerFreeDraw,
      diFlags);

   UserLeave();
   return Ret;
}
