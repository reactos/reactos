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

BOOL FASTCALL
IntGetCursorLocation(PWINSTATION_OBJECT WinSta, POINT *loc)
{
   loc->x = gpsi->ptCursor.x;
   loc->y = gpsi->ptCursor.y;

   return TRUE;
}

/* This function creates a reference for the object! */
PCURICON_OBJECT FASTCALL UserGetCurIconObject(HCURSOR hCurIcon)
{
   PCURICON_OBJECT CurIcon;

   if (!hCurIcon)
   {
      SetLastWin32Error(ERROR_INVALID_CURSOR_HANDLE);
      return NULL;
   }

   CurIcon = (PCURICON_OBJECT)UserReferenceObjectByHandle(hCurIcon, otCursorIcon);
   if (!CurIcon)
   {
      /* we never set ERROR_INVALID_ICON_HANDLE. lets hope noone ever checks for it */
      SetLastWin32Error(ERROR_INVALID_CURSOR_HANDLE);
      return NULL;
   }

   ASSERT(USER_BODY_TO_HEADER(CurIcon)->RefCount >= 1);
   return CurIcon;
}


#define COLORCURSORS_ALLOWED FALSE
HCURSOR FASTCALL
IntSetCursor(PWINSTATION_OBJECT WinSta, PCURICON_OBJECT NewCursor,
             BOOL ForceChange)
{
   SURFACE *psurf;
   SURFOBJ *pso;
   PDEVINFO DevInfo;
   PSURFACE MaskBmpObj = NULL;
   PSYSTEM_CURSORINFO CurInfo;
   PCURICON_OBJECT OldCursor;
   HCURSOR Ret = (HCURSOR)0;
   HBITMAP dcbmp;
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
   dcbmp = dc->rosdc.hBitmap;
   DevInfo = (PDEVINFO)&dc->ppdev->DevInfo;
   DC_UnlockDc(dc);

   psurf = SURFACE_LockSurface(dcbmp);
   if (!psurf)
      return (HCURSOR)0;
   pso = &psurf->SurfObj;

   if (!NewCursor)
   {
      if (CurInfo->CurrentCursorObject || ForceChange)
      {
         if (CurInfo->CurrentCursorObject)
         {
            UserDereferenceObject(CurInfo->CurrentCursorObject);
            if (CurInfo->ShowingCursor)
            {
                DPRINT1("Removing pointer!\n");
               /* Remove the cursor if it was displayed */
               IntEngMovePointer(pso, -1, -1, &GDIDEV(pso)->Pointer.Exclude);
            }
         }

         GDIDEV(pso)->Pointer.Status = SPS_ACCEPT_NOEXCLUDE;

         CurInfo->CurrentCursorObject = NewCursor; /* i.e. CurrentCursorObject = NULL */
         CurInfo->ShowingCursor = 0;
      }

      SURFACE_UnlockSurface(psurf);
      return Ret;
   }

   /* TODO: Fixme. Logic is screwed above */

   MaskBmpObj = SURFACE_LockSurface(NewCursor->IconInfo.hbmMask);
   if (MaskBmpObj)
   {
      const int maskBpp = BitsPerFormat(MaskBmpObj->SurfObj.iBitmapFormat);
      SURFACE_UnlockSurface(MaskBmpObj);
      if (maskBpp != 1)
      {
         DPRINT1("SetCursor: The Mask bitmap must have 1BPP!\n");
         SURFACE_UnlockSurface(psurf);
         return Ret;
      }

      if ((DevInfo->flGraphicsCaps2 & GCAPS2_ALPHACURSOR) &&
            pso->iBitmapFormat >= BMF_16BPP &&
            pso->iBitmapFormat <= BMF_32BPP &&
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
            MaskBmpObj = SURFACE_LockSurface(NewCursor->IconInfo.hbmMask);
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
                  SURFACE_UnlockSurface(MaskBmpObj);
                  SURFACE_UnlockSurface(psurf);
                  return (HCURSOR)0;
               }
               soMask = EngLockSurface((HSURF)hMask);
               EngCopyBits(soMask, &MaskBmpObj->SurfObj, NULL, NULL,
                           &DestRect, &SourcePoint);
               SURFACE_UnlockSurface(MaskBmpObj);
            }
         }
      }
      CurInfo->ShowingCursor = CURSOR_SHOWING;
      CurInfo->CurrentCursorObject = NewCursor;
      UserReferenceObject(NewCursor);
   }
   else
   {
      CurInfo->ShowingCursor = 0;
      CurInfo->CurrentCursorObject = NULL;
   }

   /* OldCursor is not in use anymore */
   if (OldCursor)
   {
      UserDereferenceObject(OldCursor);
   }

   if (GDIDEVFUNCS(pso).SetPointerShape)
   {
      GDIDEV(pso)->Pointer.Status =
         GDIDEVFUNCS(pso).SetPointerShape(
            pso, soMask, soColor, XlateObj,
            NewCursor->IconInfo.xHotspot,
            NewCursor->IconInfo.yHotspot,
            gpsi->ptCursor.x,
            gpsi->ptCursor.y,
            &(GDIDEV(pso)->Pointer.Exclude),
            SPS_CHANGE);
      DPRINT("SetCursor: DrvSetPointerShape() returned %x\n",
             GDIDEV(pso)->Pointer.Status);
   }
   else
   {
      GDIDEV(pso)->Pointer.Status = SPS_DECLINE;
   }

   if(GDIDEV(pso)->Pointer.Status == SPS_DECLINE)
   {
      GDIDEV(pso)->Pointer.Status = EngSetPointerShape(
                                           pso, soMask, soColor, XlateObj,
                                           NewCursor->IconInfo.xHotspot,
                                           NewCursor->IconInfo.yHotspot,
                                           gpsi->ptCursor.x,
                                           gpsi->ptCursor.y,
                                           &(GDIDEV(pso)->Pointer.Exclude),
                                           SPS_CHANGE);
      GDIDEV(pso)->Pointer.MovePointer = NULL;
   }
   else
   {
      GDIDEV(pso)->Pointer.MovePointer = GDIDEVFUNCS(pso).MovePointer;
   }

   SURFACE_UnlockSurface(psurf);
   if(hMask)
   {
      EngUnlockSurface(soMask);
      EngDeleteSurface((HSURF)hMask);
   }
   if(XlateObj)
   {
      EngDeleteXlate(XlateObj);
   }

   if(GDIDEV(pso)->Pointer.Status == SPS_ERROR)
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
                                  TAG_DIB,
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

      //    if(NT_SUCCESS(UserReferenceObjectByPointer(Object, otCursorIcon))) //<- huh????
//      UserReferenceObject(  CurIcon);
//      {
      if((CurIcon->hModule == hModule) && (CurIcon->hRsrc == hRsrc))
      {
         if(cx && ((cx != CurIcon->Size.cx) || (cy != CurIcon->Size.cy)))
         {
//               UserDereferenceObject(CurIcon);
            continue;
         }
         if (! ReferenceCurIconByProcess(CurIcon))
         {
            return NULL;
         }

         return CurIcon;
      }
//      }
//      UserDereferenceObject(CurIcon);

   }

   return NULL;
}

PCURICON_OBJECT FASTCALL
IntCreateCurIconHandle(PWINSTATION_OBJECT WinSta)
{
   PCURICON_OBJECT CurIcon;
   HANDLE hCurIcon;

   CurIcon = UserCreateObject(gHandleTable, &hCurIcon, otCursorIcon, sizeof(CURICON_OBJECT));

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
      UserDeleteObject(hCurIcon, otCursorIcon);
      UserDereferenceObject(CurIcon);
      return NULL;
   }

   InsertHeadList(&gCurIconList, &CurIcon->ListEntry);

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

   /* delete bitmaps */
   if(bmpMask)
   {
      GDIOBJ_SetOwnership(bmpMask, PsGetCurrentProcess());
      NtGdiDeleteObject(bmpMask);
      CurIcon->IconInfo.hbmMask = NULL;
   }
   if(bmpColor)
   {
      GDIOBJ_SetOwnership(bmpColor, PsGetCurrentProcess());
      NtGdiDeleteObject(bmpColor);
      CurIcon->IconInfo.hbmColor = NULL;
   }

   /* We were given a pointer, no need to keep the reference anylonger! */
   UserDereferenceObject(CurIcon);
   Ret = UserDeleteObject(CurIcon->Self, otCursorIcon);

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
      UserReferenceObject(CurIcon);
      //    if(NT_SUCCESS(UserReferenceObjectByPointer(Object, otCursorIcon)))
      {
         LIST_FOR_EACH(ProcessData, &CurIcon->ProcessList, CURICON_PROCESS, ListEntry)
         {
            if (Win32Process == ProcessData->Process)
            {
               RemoveEntryList(&CurIcon->ListEntry);
               IntDestroyCurIconObject(WinSta, CurIcon, TRUE);
               CurIcon = NULL;
               break;
            }
         }

//         UserDereferenceObject(Object);
      }

      if (CurIcon)
      {
         UserDereferenceObject(CurIcon);
      }
   }

   ObDereferenceObject(WinSta);
}

/*
 * @implemented
 */
HANDLE
APIENTRY
NtUserCreateCursorIconHandle(PICONINFO IconInfo OPTIONAL, BOOL Indirect)
{
   PCURICON_OBJECT CurIcon;
   PWINSTATION_OBJECT WinSta;
   PSURFACE psurfBmp;
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
            CurIcon->IconInfo.hbmMask = BITMAP_CopyBitmap(CurIcon->IconInfo.hbmMask);
            CurIcon->IconInfo.hbmColor = BITMAP_CopyBitmap(CurIcon->IconInfo.hbmColor);
         }
         if(CurIcon->IconInfo.hbmColor &&
               (psurfBmp = SURFACE_LockSurface(CurIcon->IconInfo.hbmColor)))
         {
            CurIcon->Size.cx = psurfBmp->SurfObj.sizlBitmap.cx;
            CurIcon->Size.cy = psurfBmp->SurfObj.sizlBitmap.cy;
            SURFACE_UnlockSurface(psurfBmp);
            GDIOBJ_SetOwnership(CurIcon->IconInfo.hbmColor, NULL);
         }
         if(CurIcon->IconInfo.hbmMask &&
               (psurfBmp = SURFACE_LockSurface(CurIcon->IconInfo.hbmMask)))
         {
            if (CurIcon->IconInfo.hbmColor == NULL)
            {
               CurIcon->Size.cx = psurfBmp->SurfObj.sizlBitmap.cx;
               CurIcon->Size.cy = psurfBmp->SurfObj.sizlBitmap.cy / 2;
            }
            SURFACE_UnlockSurface(psurfBmp);
            GDIOBJ_SetOwnership(CurIcon->IconInfo.hbmMask, NULL);
         }
      }
      else
      {
         SetLastNtError(Status);
         /* FIXME - Don't exit here */
      }
   }

   UserDereferenceObject(CurIcon);
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
APIENTRY
NtUserGetIconInfo(
   HANDLE hCurIcon,
   PICONINFO IconInfo,
   PUNICODE_STRING lpInstName, // optional
   PUNICODE_STRING lpResName,  // optional
   LPDWORD pbpp,               // optional
   BOOL bInternal)
{
   ICONINFO ii;
   PCURICON_OBJECT CurIcon;
   PWINSTATION_OBJECT WinSta;
   NTSTATUS Status = STATUS_SUCCESS;
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
   ii.hbmMask = BITMAP_CopyBitmap(CurIcon->IconInfo.hbmMask);
   ii.hbmColor = BITMAP_CopyBitmap(CurIcon->IconInfo.hbmColor);

   /* Copy fields */
   _SEH2_TRY
   {
       ProbeForWrite(IconInfo, sizeof(ICONINFO), 1);
       RtlCopyMemory(IconInfo, &ii, sizeof(ICONINFO));

       if (pbpp)
       {
           PSURFACE psurfBmp;
           int colorBpp = 0;

           ProbeForWrite(pbpp, sizeof(DWORD), 1);

           psurfBmp = SURFACE_LockSurface(CurIcon->IconInfo.hbmColor);
           if (psurfBmp)
           {
               colorBpp = BitsPerFormat(psurfBmp->SurfObj.iBitmapFormat);
               SURFACE_UnlockSurface(psurfBmp);
           }

           RtlCopyMemory(pbpp, &colorBpp, sizeof(DWORD));
       }

   }
   _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
   {
       Status = _SEH2_GetExceptionCode();
   }
   _SEH2_END

   if (NT_SUCCESS(Status))
      Ret = TRUE;
   else
      SetLastNtError(Status);

   UserDereferenceObject(CurIcon);
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
APIENTRY
NtUserGetIconSize(
    HANDLE hCurIcon,
    UINT istepIfAniCur,
    PLONG plcx,       // &size.cx
    PLONG plcy)       // &size.cy
{
   PCURICON_OBJECT CurIcon;
   NTSTATUS Status = STATUS_SUCCESS;
   BOOL bRet = FALSE;

   DPRINT("Enter NtUserGetIconSize\n");
   UserEnterExclusive();

   if (!(CurIcon = UserGetCurIconObject(hCurIcon)))
   {
      goto cleanup;
   }

   _SEH2_TRY
   {
       ProbeForWrite(plcx, sizeof(LONG), 1);
       RtlCopyMemory(plcx, &CurIcon->Size.cx, sizeof(LONG));
       ProbeForWrite(plcy, sizeof(LONG), 1);
       RtlCopyMemory(plcy, &CurIcon->Size.cy, sizeof(LONG));
   }
   _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
   {
       Status = _SEH2_GetExceptionCode();
   }
   _SEH2_END

   if(NT_SUCCESS(Status))
      bRet = TRUE;
   else
      SetLastNtError(Status); // maybe not, test this

   UserDereferenceObject(CurIcon);

cleanup:
   DPRINT("Leave NtUserGetIconSize, ret=%i\n", bRet);
   UserLeave();
   return bRet;
}


/*
 * @unimplemented
 */
DWORD
APIENTRY
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
APIENTRY
NtUserGetCursorInfo(
   PCURSORINFO pci)
{
   CURSORINFO SafeCi;
   PSYSTEM_CURSORINFO CurInfo;
   PWINSTATION_OBJECT WinSta;
   NTSTATUS Status = STATUS_SUCCESS;
   PCURICON_OBJECT CurIcon;
   BOOL Ret = FALSE;
   DECLARE_RETURN(BOOL);

   DPRINT("Enter NtUserGetCursorInfo\n");
   UserEnterExclusive();

   WinSta = IntGetWinStaObj();
   if (WinSta == NULL)
   {
      RETURN( FALSE);
   }

   CurInfo = IntGetSysCursorInfo(WinSta);
   CurIcon = (PCURICON_OBJECT)CurInfo->CurrentCursorObject;

   SafeCi.cbSize = sizeof(CURSORINFO);
   SafeCi.flags = ((CurInfo->ShowingCursor && CurIcon) ? CURSOR_SHOWING : 0);
   SafeCi.hCursor = (CurIcon ? (HCURSOR)CurIcon->Self : (HCURSOR)0);

   IntGetCursorLocation(WinSta, &SafeCi.ptScreenPos);

   _SEH2_TRY
   {
      if (pci->cbSize == sizeof(CURSORINFO))
      {
         ProbeForWrite(pci, sizeof(CURSORINFO), 1);
         RtlCopyMemory(pci, &SafeCi, sizeof(CURSORINFO));
         Ret = TRUE;
      }
      else
      {
         SetLastWin32Error(ERROR_INVALID_PARAMETER);
      }
   }
   _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
   {
      Status = _SEH2_GetExceptionCode();
   }
   _SEH2_END;
   if (!NT_SUCCESS(Status))
   {
      SetLastNtError(Status);
   }

   ObDereferenceObject(WinSta);
   RETURN(Ret);

CLEANUP:
   DPRINT("Leave NtUserGetCursorInfo, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}


/*
 * @implemented
 */
BOOL
APIENTRY
NtUserClipCursor(
   RECTL *UnsafeRect)
{
   /* FIXME - check if process has WINSTA_WRITEATTRIBUTES */

   PWINSTATION_OBJECT WinSta;
   PSYSTEM_CURSORINFO CurInfo;
   RECTL Rect;
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
      CurInfo->CursorClipInfo.Left = max(Rect.left, DesktopWindow->Wnd->WindowRect.left);
      CurInfo->CursorClipInfo.Top = max(Rect.top, DesktopWindow->Wnd->WindowRect.top);
      CurInfo->CursorClipInfo.Right = min(Rect.right - 1, DesktopWindow->Wnd->WindowRect.right - 1);
      CurInfo->CursorClipInfo.Bottom = min(Rect.bottom - 1, DesktopWindow->Wnd->WindowRect.bottom - 1);

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
APIENTRY
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
   /* Note: IntDestroyCurIconObject will remove our reference for us! */

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
APIENTRY
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
APIENTRY
NtUserGetClipCursor(
   RECTL *lpRect)
{
   /* FIXME - check if process has WINSTA_READATTRIBUTES */
   PSYSTEM_CURSORINFO CurInfo;
   PWINSTATION_OBJECT WinSta;
   RECTL Rect;
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

   Status = MmCopyToCaller(lpRect, &Rect, sizeof(RECT));
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
APIENTRY
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

   if(hCursor)
   {
      if(!(CurIcon = UserGetCurIconObject(hCursor)))
      {
         ObDereferenceObject(WinSta);
         RETURN(NULL);
      }
   }
   else
   {
      CurIcon = NULL;
   }

   OldCursor = IntSetCursor(WinSta, CurIcon, FALSE);

   if(CurIcon)
   {
      UserDereferenceObject(CurIcon);
   }
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
APIENTRY
NtUserSetCursorContents(
   HANDLE hCurIcon,
   PICONINFO UnsafeIconInfo)
{
   PCURICON_OBJECT CurIcon;
   ICONINFO IconInfo;
   PSURFACE psurfBmp;
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
   Status = MmCopyFromCaller(&IconInfo, UnsafeIconInfo, sizeof(ICONINFO));
   if(!NT_SUCCESS(Status))
   {
      SetLastNtError(Status);
      goto done;
   }

   /* Delete old bitmaps */
   if (CurIcon->IconInfo.hbmColor != IconInfo.hbmColor)
   {
      NtGdiDeleteObject(CurIcon->IconInfo.hbmColor);
   }
   if (CurIcon->IconInfo.hbmMask != IconInfo.hbmMask)
   {
      NtGdiDeleteObject(CurIcon->IconInfo.hbmMask);
   }

   /* Copy new IconInfo field */
   CurIcon->IconInfo = IconInfo;

   psurfBmp = SURFACE_LockSurface(CurIcon->IconInfo.hbmColor);
   if(psurfBmp)
   {
      CurIcon->Size.cx = psurfBmp->SurfObj.sizlBitmap.cx;
      CurIcon->Size.cy = psurfBmp->SurfObj.sizlBitmap.cy;
      SURFACE_UnlockSurface(psurfBmp);
      GDIOBJ_SetOwnership(CurIcon->IconInfo.hbmColor, NULL);
   }
   else
   {
      psurfBmp = SURFACE_LockSurface(CurIcon->IconInfo.hbmMask);
      if(!psurfBmp)
         goto done;

      CurIcon->Size.cx = psurfBmp->SurfObj.sizlBitmap.cx;
      CurIcon->Size.cy = psurfBmp->SurfObj.sizlBitmap.cy / 2;

      SURFACE_UnlockSurface(psurfBmp);
      GDIOBJ_SetOwnership(CurIcon->IconInfo.hbmMask, NULL);
   }

   Ret = TRUE;

done:

   if (CurIcon)
   {
      UserDereferenceObject(CurIcon);
   }
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
#if 0
BOOL
APIENTRY
NtUserSetCursorIconData(
  HANDLE Handle,
  HMODULE hModule,
  PUNICODE_STRING pstrResName,
  PICONINFO pIconInfo)
{
   PCURICON_OBJECT CurIcon;
   PWINSTATION_OBJECT WinSta;
   PSURFACE psurfBmp;
   NTSTATUS Status = STATUS_SUCCESS;
   BOOL Ret = FALSE;
   DECLARE_RETURN(BOOL);

   DPRINT("Enter NtUserSetCursorIconData\n");
   UserEnterExclusive();

   WinSta = IntGetWinStaObj();
   if(WinSta == NULL)
   {
      RETURN( FALSE);
   }

   if(!(CurIcon = UserGetCurIconObject(Handle)))
   {
      ObDereferenceObject(WinSta);
      RETURN(FALSE);
   }

   CurIcon->hModule = hModule;
   CurIcon->hRsrc = NULL; //hRsrc;
   CurIcon->hGroupRsrc = NULL; //hGroupRsrc;

   _SEH2_TRY
   {
       ProbeForRead(pIconInfo, sizeof(ICONINFO), 1);
       RtlCopyMemory(&CurIcon->IconInfo, pIconInfo, sizeof(ICONINFO));

       CurIcon->IconInfo.hbmMask = BITMAP_CopyBitmap(pIconInfo->hbmMask);
       CurIcon->IconInfo.hbmColor = BITMAP_CopyBitmap(pIconInfo->hbmColor);

       if (CurIcon->IconInfo.hbmColor)
       {
           if ((psurfBmp = SURFACE_LockSurface(CurIcon->IconInfo.hbmColor)))
           {
               CurIcon->Size.cx = psurfBmp->SurfObj.sizlBitmap.cx;
               CurIcon->Size.cy = psurfBmp->SurfObj.sizlBitmap.cy;
               SURFACE_UnlockSurface(psurfBmp);
               GDIOBJ_SetOwnership(GdiHandleTable, CurIcon->IconInfo.hbmMask, NULL);
           }
       }
       if (CurIcon->IconInfo.hbmMask)
       {
           if (CurIcon->IconInfo.hbmColor == NULL)
           {
               if ((psurfBmp = SURFACE_LockSurface(CurIcon->IconInfo.hbmMask)))
               {
                   CurIcon->Size.cx = psurfBmp->SurfObj.sizlBitmap.cx;
                   CurIcon->Size.cy = psurfBmp->SurfObj.sizlBitmap.cy;
                   SURFACE_UnlockSurface(psurfBmp);
               }
           }
           GDIOBJ_SetOwnership(GdiHandleTable, CurIcon->IconInfo.hbmMask, NULL);
       }
   }
   _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
   {
        Status = _SEH2_GetExceptionCode();
   }
   _SEH2_END

   if(!NT_SUCCESS(Status))
      SetLastNtError(Status);
   else
      Ret = TRUE;

   UserDereferenceObject(CurIcon);
   ObDereferenceObject(WinSta);
   RETURN( Ret);

CLEANUP:
   DPRINT("Leave NtUserSetCursorIconData, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}
#else
BOOL
APIENTRY
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
   UserDereferenceObject(CurIcon);
   ObDereferenceObject(WinSta);
   RETURN( Ret);


CLEANUP:
   DPRINT("Leave NtUserSetCursorIconData, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}
#endif

/*
 * @unimplemented
 */
BOOL
APIENTRY
NtUserSetSystemCursor(
   HCURSOR hcur,
   DWORD id)
{
   return FALSE;
}


/* FIXME: ReactOS specific hack */
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

   if (!hbmMask || !IntGdiGetObject(hbmMask, sizeof(BITMAP), (PVOID)&bmpMask))
   {
      return FALSE;
   }

   if (hbmColor && !IntGdiGetObject(hbmColor, sizeof(BITMAP), (PVOID)&bmpColor))
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

   /* NtGdiCreateCompatibleBitmap will create a monochrome bitmap
      when cxWidth or cyHeight is 0 */
   if ((bmpColor.bmBitsPixel == 32) && (cxWidth != 0) && (cyHeight != 0))
   {
      SURFACE *psurfOff = NULL;
      PFN_DIB_GetPixel fnSource_GetPixel = NULL;
      INT x, y;

      //Find alpha into icon
      psurfOff = SURFACE_LockSurface(hbmColor ? hbmColor : hbmMask);
      if (psurfOff)
      {
         fnSource_GetPixel = DibFunctionsForBitmapFormat[psurfOff->SurfObj.iBitmapFormat].DIB_GetPixel;
         if (fnSource_GetPixel)
         {
            for (x = 0; x < psurfOff->SurfObj.sizlBitmap.cx; x++)
            {
               for (y = 0; y < psurfOff->SurfObj.sizlBitmap.cy; y++)
               {
                  bAlpha = ((BYTE)(fnSource_GetPixel(&psurfOff->SurfObj, x, y) >> 24) & 0xff);
                  if (bAlpha) 
                     break;
               }
               if (bAlpha) 
                  break;
            }
         }
         SURFACE_UnlockSurface(psurfOff);
      }
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
      RECTL r;
      BITMAP bm;
      SURFACE *psurfOff = NULL;

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

      /* make sure we have a 32 bit offscreen bitmap
        otherwise we can't do alpha blending */
      psurfOff = SURFACE_LockSurface(hbmOff);
      if (psurfOff == NULL)
      {
          DPRINT1("BITMAPOBJ_LockBitmap() failed!\n");
          goto cleanup;
      }
      BITMAP_GetObject(psurfOff, sizeof(BITMAP), (PVOID)&bm);

      if (bm.bmBitsPixel != 32)
        bAlpha = FALSE;

      SURFACE_UnlockSurface(psurfOff);

      hOldOffBmp = NtGdiSelectBitmap(hdcOff, hbmOff);
      if (!hOldOffBmp)
      {
         DPRINT1("NtGdiSelectBitmap() failed!\n");
         goto cleanup;
      }

      if (DoFlickerFree)
      {
          hOldOffBrush = NtGdiSelectBrush(hdcOff, hbrFlickerFreeDraw);
          if (!hOldOffBrush)
          {
             DPRINT1("NtGdiSelectBrush() failed!\n");
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

   oldFg = IntGdiSetTextColor(hdcOff, RGB(0, 0, 0));
   oldBg = IntGdiSetBkColor(hdcOff, RGB(255, 255, 255));

   if (diFlags & DI_MASK)
   {
      hOldMem = NtGdiSelectBitmap(hdcMem, hbmMask);
      if (!hOldMem)
      {
         DPRINT("NtGdiSelectBitmap() failed!\n");
         goto cleanup;
      }

      NtGdiStretchBlt(hdcOff,
                   (DoFlickerFree || bAlpha ? 0 : xLeft),
                   (DoFlickerFree || bAlpha ? 0 : yTop), 
                   cxWidth,
                   cyHeight,
                   hdcMem,
                   0,
                   0,
                   IconSize.cx,
                   IconSize.cy,
                   ((diFlags & DI_IMAGE) ? SRCAND : SRCCOPY),
                   0);

      NtGdiSelectBitmap(hdcMem, hOldMem);
   }

   if(diFlags & DI_IMAGE)
   {
      hOldMem = NtGdiSelectBitmap(hdcMem, (hbmColor ? hbmColor : hbmMask));

      NtGdiStretchBlt(hdcOff, 
                   (DoFlickerFree || bAlpha ? 0 : xLeft),
                   (DoFlickerFree || bAlpha ? 0 : yTop),
                   cxWidth,
                   cyHeight,
                   hdcMem,
                   0,
                   (hbmColor ? 0 : IconSize.cy),
                   IconSize.cx,
                   IconSize.cy,
                   ((diFlags & DI_MASK) ? SRCINVERT : SRCCOPY),
                   0);

      NtGdiSelectBitmap(hdcMem, hOldMem);
   }

    if (bAlpha)
    {
        BITMAP bm;
        SURFACE *psurfOff = NULL;
        PBYTE pBits = NULL;
        BLENDFUNCTION BlendFunc;
        DWORD Pixel;
        BYTE Red, Green, Blue, Alpha;
        DWORD Count = 0;
        INT i, j;

        psurfOff = SURFACE_LockSurface(hbmOff);
        if (psurfOff == NULL)
        {
            DPRINT1("BITMAPOBJ_LockBitmap() failed!\n");
            goto cleanup;
        }
        BITMAP_GetObject(psurfOff, sizeof(BITMAP), (PVOID)&bm);

        pBits = ExAllocatePoolWithTag(PagedPool, bm.bmWidthBytes * abs(bm.bmHeight), TAG_BITMAP);
        if (pBits == NULL)
        {
            DPRINT1("ExAllocatePoolWithTag() failed!\n");
            SURFACE_UnlockSurface(psurfOff);
            goto cleanup;
        }

        /* get icon bits */
        IntGetBitmapBits(psurfOff, bm.bmWidthBytes * abs(bm.bmHeight), pBits);

        /* premultiply with the alpha channel value */
        for (i = 0; i < cyHeight; i++)
        {
            for (j = 0; j < cxWidth; j++)
            {
                Pixel = *(DWORD *)(pBits + Count);

                Alpha = ((BYTE)(Pixel >> 24) & 0xff);

                Red   = (((BYTE)(Pixel >>  0)) * Alpha) / 0xff;
                Green = (((BYTE)(Pixel >>  8)) * Alpha) / 0xff;
                Blue  = (((BYTE)(Pixel >> 16)) * Alpha) / 0xff;

                *(DWORD *)(pBits + Count) = (DWORD)(Red | (Green << 8) | (Blue << 16) | (Alpha << 24));

                Count += sizeof (DWORD);
            }
        }

        /* set icon bits */
        IntSetBitmapBits(psurfOff, bm.bmWidthBytes * abs(bm.bmHeight), pBits);
        ExFreePoolWithTag(pBits, TAG_BITMAP);

        SURFACE_UnlockSurface(psurfOff);

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

   IntGdiSetTextColor(hdcOff, oldFg);
   IntGdiSetBkColor(hdcOff, oldBg);

   Ret = TRUE;

cleanup:
   if(DoFlickerFree || bAlpha)
   {
      if(hOldOffBmp) NtGdiSelectBitmap(hdcOff, hOldOffBmp);
      if(hOldOffBrush) NtGdiSelectBrush(hdcOff, hOldOffBrush);
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
APIENTRY
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

   UserDereferenceObject(pIcon);

   UserLeave();
   return Ret;
}

/* Called from NtUserCallOneParam with Routine ONEPARAM_ROUTINE_SHOWCURSOR
 * User32 macro NtUserShowCursor */
int
APIENTRY
UserShowCursor(BOOL bShow)
{
    PTHREADINFO pti = PsGetCurrentThreadWin32Thread();
    PWINSTATION_OBJECT WinSta = pti->Desktop->WindowStation;
    PSYSTEM_CURSORINFO CurInfo;

    HDC Screen;
    PDC dc;
    HBITMAP hbmpDc;
    SURFOBJ *SurfObj;
    SURFACE *psurfDc;
    PDEVOBJ *ppdev;
    GDIPOINTER *pgp;
    int showpointer=0;

    if(!(Screen = IntGetScreenDC()))
    {
        return showpointer; /* No mouse */
    }

    dc = DC_LockDc(Screen);

    if (!dc)
    {
        return showpointer; /* No mouse */
    }

    hbmpDc = dc->rosdc.hBitmap;
    DC_UnlockDc(dc);

    psurfDc = SURFACE_LockSurface(hbmpDc);
    if ( !psurfDc )
    {
        return showpointer; /* No Mouse */
    }

    SurfObj = &psurfDc->SurfObj;
    if (SurfObj == NULL)
    {
        SURFACE_UnlockSurface(psurfDc);
        return showpointer; /* No mouse */
    }

    ppdev = GDIDEV(SurfObj);

    if(ppdev == NULL)
    {
        SURFACE_UnlockSurface(psurfDc);
        return showpointer; /* No mouse */
    }

    pgp = &ppdev->Pointer;

    CurInfo = IntGetSysCursorInfo(WinSta);

    if (bShow == FALSE)
    {
        pgp->ShowPointer--;
        showpointer = pgp->ShowPointer;

        if (showpointer >= 0)
        {
            //ppdev->SafetyRemoveCount = 1;
            //ppdev->SafetyRemoveLevel = 1;
            EngMovePointer(SurfObj,-1,-1,NULL);
            CurInfo->ShowingCursor = 0;
        }

    }
    else
    {
        pgp->ShowPointer++;
        showpointer = pgp->ShowPointer;

        /* Show Cursor */
        if (showpointer < 0)
        {
            //ppdev->SafetyRemoveCount = 0;
            //ppdev->SafetyRemoveLevel = 0;
            EngMovePointer(SurfObj,-1,-1,NULL);
            CurInfo->ShowingCursor = CURSOR_SHOWING;
        }
    }

    SURFACE_UnlockSurface(psurfDc);
    return showpointer;
}
