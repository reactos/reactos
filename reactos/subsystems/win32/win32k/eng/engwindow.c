/*
 *  ReactOS W32 Subsystem
 *  Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003, 2004 ReactOS Team
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
/* $Id$
 *
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           GDI WNDOBJ Functions
 * FILE:              subsys/win32k/eng/window.c
 * PROGRAMER:         Gregor Anich
 * REVISION HISTORY:
 *                 16/11/2004: Created
 */

/* TODO: Check how the WNDOBJ implementation should behave with a driver on windows.

   Simple! Use Prop's!
 */

#include <w32k.h>

#define NDEBUG
#include <debug.h>

/*
 * Calls the WNDOBJCHANGEPROC of the given WNDOBJ
 */
VOID
FASTCALL
IntEngWndCallChangeProc(
  IN WNDOBJ *pwo,
  IN FLONG   flChanged)
{
  WNDGDI *WndObjInt = ObjToGDI(pwo, WND);

  if (WndObjInt->ChangeProc == NULL)
    {
      return;
    }

  /* check flags of the WNDOBJ */
  flChanged &= WndObjInt->Flags;
  if (flChanged == 0)
    {
      return;
    }

  /* Call the WNDOBJCHANGEPROC */
  if (flChanged == WOC_CHANGED)
    {
      pwo = NULL;
    }

  DPRINT("Calling WNDOBJCHANGEPROC (0x%x), Changed = 0x%x\n",
         WndObjInt->ChangeProc, flChanged);
  WndObjInt->ChangeProc(pwo, flChanged);
}

/*
 * Fills the CLIPOBJ and client rect of the WNDOBJ with the data from the given WINDOW_OBJECT
 */
BOOLEAN
FASTCALL
IntEngWndUpdateClipObj(
  WNDGDI *WndObjInt,
  PWINDOW_OBJECT Window)
{
  HRGN hVisRgn;
  PROSRGNDATA visRgn;
  CLIPOBJ *ClipObj = NULL;
  CLIPOBJ *OldClipObj;

  hVisRgn = VIS_ComputeVisibleRegion(Window, TRUE, TRUE, TRUE);
  if (hVisRgn != NULL)
  {
    NtGdiOffsetRgn(hVisRgn, Window->Wnd->ClientRect.left, Window->Wnd->ClientRect.top);
    visRgn = REGION_LockRgn(hVisRgn);
    if (visRgn != NULL)
    {
      if (visRgn->rdh.nCount > 0)
      {
        ClipObj = IntEngCreateClipRegion(visRgn->rdh.nCount, visRgn->Buffer,
                                         &visRgn->rdh.rcBound);
        DPRINT("Created visible region with %d rects\n", visRgn->rdh.nCount);
        DPRINT("  BoundingRect: %d, %d  %d, %d\n",
               visRgn->rdh.rcBound.left, visRgn->rdh.rcBound.top,
               visRgn->rdh.rcBound.right, visRgn->rdh.rcBound.bottom);
        {
          INT i;
          for (i = 0; i < visRgn->rdh.nCount; i++)
          {
            DPRINT("  Rect #%d: %d,%d  %d,%d\n", i+1,
                   visRgn->Buffer[i].left, visRgn->Buffer[i].top,
                   visRgn->Buffer[i].right, visRgn->Buffer[i].bottom);
          }
        }
      }
      REGION_UnlockRgn(visRgn);
    }
    else
    {
      DPRINT1("Warning: Couldn't lock visible region of window DC\n");
    }
  }
  else
  {
    DPRINT1("Warning: VIS_ComputeVisibleRegion failed!\n");
  }

  if (ClipObj == NULL)
  {
    /* Fall back to client rect */
    ClipObj = IntEngCreateClipRegion(1, &Window->Wnd->ClientRect,
                                     &Window->Wnd->ClientRect);
  }

  if (ClipObj == NULL)
  {
    DPRINT1("Warning: IntEngCreateClipRegion() failed!\n");
    return FALSE;
  }

  RtlCopyMemory(&WndObjInt->WndObj.coClient, ClipObj, sizeof (CLIPOBJ));
  RtlCopyMemory(&WndObjInt->WndObj.rclClient, &Window->Wnd->ClientRect, sizeof (RECT));
  OldClipObj = InterlockedExchangePointer((PVOID*)&WndObjInt->ClientClipObj, ClipObj);
  if (OldClipObj != NULL)
    IntEngDeleteClipRegion(OldClipObj);

  return TRUE;
}

/*
 * Updates all WNDOBJs of the given WINDOW_OBJECT and calls the change-procs.
 */
VOID
FASTCALL
IntEngWindowChanged(
  PWINDOW_OBJECT  Window,
  FLONG           flChanged)
{
  PLIST_ENTRY CurrentEntry;
  WNDGDI *Current;

  ASSERT_IRQL_LESS_OR_EQUAL(PASSIVE_LEVEL);

  CurrentEntry = Window->WndObjListHead.Flink;
  while (CurrentEntry != &Window->WndObjListHead)
    {
      Current = CONTAINING_RECORD(CurrentEntry, WNDGDI, ListEntry);

      if (Current->WndObj.pvConsumer != NULL)
        {
          /* Update the WNDOBJ */
          switch (flChanged)
            {
            case WOC_RGN_CLIENT:
              /* Update the clipobj and client rect of the WNDOBJ */
              IntEngWndUpdateClipObj(Current, Window);
              break;

            case WOC_DELETE:
              /* FIXME: Should the WNDOBJs be deleted by win32k or by the driver? */
              break;
            }

          /* Call the change proc */
          IntEngWndCallChangeProc(&Current->WndObj, flChanged);

          /* HACK: Send WOC_CHANGED after WOC_RGN_CLIENT */
          if (flChanged == WOC_RGN_CLIENT)
            {
              IntEngWndCallChangeProc(&Current->WndObj, WOC_CHANGED);
            }

          CurrentEntry = CurrentEntry->Flink;
        }
    }

}

/*
 * @implemented
 */
WNDOBJ*
APIENTRY
EngCreateWnd(
  SURFOBJ          *pso,
  HWND              hWnd,
  WNDOBJCHANGEPROC  pfn,
  FLONG             fl,
  int               iPixelFormat)
{
  WNDGDI *WndObjInt = NULL;
  WNDOBJ *WndObjUser = NULL;
  PWINDOW_OBJECT Window;
  BOOL calledFromUser;
  DECLARE_RETURN(WNDOBJ*);

  DPRINT("EngCreateWnd: pso = 0x%x, hwnd = 0x%x, pfn = 0x%x, fl = 0x%x, pixfmt = %d\n",
         pso, hWnd, pfn, fl, iPixelFormat);

  calledFromUser = UserIsEntered();
  if (!calledFromUser){
     UserEnterShared();
  }

  /* Get window object */
  Window = UserGetWindowObject(hWnd);
  if (Window == NULL)
    {
      RETURN( NULL);
    }

  /* Create WNDOBJ */
  WndObjInt = EngAllocMem(0, sizeof (WNDGDI), TAG_WNDOBJ);
  if (WndObjInt == NULL)
    {
      DPRINT1("Failed to allocate memory for a WND structure!\n");
      RETURN( NULL);
    }

  /* Fill the clipobj */
  WndObjInt->ClientClipObj = NULL;
  if (!IntEngWndUpdateClipObj(WndObjInt, Window))
    {
      EngFreeMem(WndObjInt);
      RETURN( NULL);
    }

  /* Fill user object */
  WndObjUser = GDIToObj(WndObjInt, WND);
  WndObjUser->psoOwner = pso;
  WndObjUser->pvConsumer = NULL;

  /* Fill internal object */
  WndObjInt->Hwnd = hWnd;
  WndObjInt->ChangeProc = pfn;
  WndObjInt->Flags = fl;
  WndObjInt->PixelFormat = iPixelFormat;

  /* associate object with window */
  InsertTailList(&Window->WndObjListHead, &WndObjInt->ListEntry);

  DPRINT("EngCreateWnd: SUCCESS!\n");

  RETURN( WndObjUser);

CLEANUP:

  if (!calledFromUser){
    UserLeave();
  }

  END_CLEANUP;
}


/*
 * @implemented
 */
VOID
APIENTRY
EngDeleteWnd(
  IN WNDOBJ *pwo)
{
  WNDGDI *WndObjInt = ObjToGDI(pwo, WND);
  PWINDOW_OBJECT Window;
  BOOL calledFromUser;

  DPRINT("EngDeleteWnd: pwo = 0x%x\n", pwo);

  calledFromUser = UserIsEntered();
  if (!calledFromUser){
     UserEnterExclusive();
  }

  /* Get window object */
  Window = UserGetWindowObject(WndObjInt->Hwnd);
  if (Window == NULL)
    {
      DPRINT1("Warning: Couldnt get window object for WndObjInt->Hwnd!!!\n");
      RemoveEntryList(&WndObjInt->ListEntry);
    }
  else
    {
      /* Remove object from window */
      RemoveEntryList(&WndObjInt->ListEntry);
    }

  if (!calledFromUser){
     UserLeave();
  }

  /* Free resources */
  IntEngDeleteClipRegion(WndObjInt->ClientClipObj);
  EngFreeMem(WndObjInt);
}


/*
 * @implemented
 */
BOOL
APIENTRY
WNDOBJ_bEnum(
  IN WNDOBJ  *pwo,
  IN ULONG  cj,
  OUT ULONG  *pul)
{
  WNDGDI *WndObjInt = ObjToGDI(pwo, WND);
  BOOL Ret;

  DPRINT("WNDOBJ_bEnum: pwo = 0x%x, cj = %d, pul = 0x%x\n", pwo, cj, pul);
  Ret = CLIPOBJ_bEnum(WndObjInt->ClientClipObj, cj, pul);

  DPRINT("WNDOBJ_bEnum: Returning %s\n", Ret ? "True" : "False");
  return Ret;
}


/*
 * @implemented
 */
ULONG
APIENTRY
WNDOBJ_cEnumStart(
  IN WNDOBJ  *pwo,
  IN ULONG  iType,
  IN ULONG  iDirection,
  IN ULONG  cLimit)
{
  WNDGDI *WndObjInt = ObjToGDI(pwo, WND);
  ULONG Ret;

  DPRINT("WNDOBJ_cEnumStart: pwo = 0x%x, iType = %d, iDirection = %d, cLimit = %d\n",
         pwo, iType, iDirection, cLimit);

  /* FIXME: Should we enumerate all rectangles or not? */
  Ret = CLIPOBJ_cEnumStart(WndObjInt->ClientClipObj, FALSE, iType, iDirection, cLimit);

  DPRINT("WNDOBJ_cEnumStart: Returning 0x%x\n", Ret);
  return Ret;
}


/*
 * @implemented
 */
VOID
APIENTRY
WNDOBJ_vSetConsumer(
  IN WNDOBJ  *pwo,
  IN PVOID  pvConsumer)
{
  BOOL Hack;

  DPRINT("WNDOBJ_vSetConsumer: pwo = 0x%x, pvConsumer = 0x%x\n", pwo, pvConsumer);

  Hack = (pwo->pvConsumer == NULL);
  pwo->pvConsumer = pvConsumer;

  /* HACKHACKHACK
   *
   * MSDN says that the WNDOBJCHANGEPROC will be called with the most recent state
   * when a WNDOBJ is created - we do it here because most drivers will need pvConsumer
   * in the callback to identify the WNDOBJ I think.
   *
   *  - blight
   */
  if (Hack)
    {
      IntEngWndCallChangeProc(pwo, WOC_RGN_CLIENT);
      IntEngWndCallChangeProc(pwo, WOC_CHANGED);
      IntEngWndCallChangeProc(pwo, WOC_DRAWN);
    }
}

/* EOF */

