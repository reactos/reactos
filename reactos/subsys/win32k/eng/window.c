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
#include <w32k.h>

/*
 * Calls the WNDOBJCHANGEPROC of the given WNDOBJ
 */
VOID
FASTCALL
IntEngWndChanged(
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
 * @implemented
 */
WNDOBJ*
STDCALL
EngCreateWnd(
	SURFOBJ          *pso,
	HWND              hwnd,
	WNDOBJCHANGEPROC  pfn,
	FLONG             fl,
	int               iPixelFormat
	)
{
  WNDGDI *WndObjInt = NULL;
  WNDOBJ *WndObjUser = NULL;
  PWINDOW_OBJECT Window;
  CLIPOBJ *ClientClipObj;

  DPRINT("EngCreateWnd: pso = 0x%x, hwnd = 0x%x, pfn = 0x%x, fl = 0x%x, pixfmt = %d\n",
         pso, hwnd, pfn, fl, iPixelFormat);

  /* Get window object */
  Window = IntGetWindowObject(hwnd);
  if (Window == NULL)
    {
      return NULL;
    }

  /* Create WNDOBJ */
  WndObjInt = EngAllocMem(0, sizeof (WNDGDI), TAG_WNDOBJ);
  if (WndObjInt == NULL)
    {
      IntReleaseWindowObject(Window);
      DPRINT1("Failed to allocate memory for a WND structure!\n");
      return NULL;
    }

  ClientClipObj = IntEngCreateClipRegion(1, (PRECTL)&Window->ClientRect,
                                         (PRECTL)&Window->ClientRect);
  if (ClientClipObj == NULL)
    {
      IntReleaseWindowObject(Window);
      EngFreeMem(WndObjInt);
      return NULL;
    }

  /* fill user object */
  WndObjUser = GDIToObj(WndObjInt, WND);
  WndObjUser->psoOwner = pso;
  WndObjUser->pvConsumer = NULL;
  RtlCopyMemory(&WndObjUser->rclClient, &Window->ClientRect, sizeof (RECT));
  RtlCopyMemory(&WndObjUser->coClient, ClientClipObj, sizeof (CLIPOBJ));

  /* fill internal object */
  WndObjInt->ChangeProc = pfn;
  WndObjInt->Flags = fl;
  WndObjInt->PixelFormat = iPixelFormat;
  WndObjInt->ClientClipObj = ClientClipObj;

  /* release resources */
  IntReleaseWindowObject(Window);

  DPRINT("EngCreateWnd: SUCCESS!\n");
  
  return WndObjUser;
}


/*
 * @implemented
 */
VOID
STDCALL
EngDeleteWnd ( IN WNDOBJ *pwo )
{
  WNDGDI *WndObjInt = ObjToGDI(pwo, WND);
  
  DPRINT("EngDeleteWnd: pwo = 0x%x\n", pwo);

  IntEngDeleteClipRegion(WndObjInt->ClientClipObj);
  EngFreeMem(WndObjInt);
}


/*
 * @implemented
 */
BOOL
STDCALL
WNDOBJ_bEnum(
	IN WNDOBJ  *pwo,
	IN ULONG  cj,
	OUT ULONG  *pul
	)
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
STDCALL
WNDOBJ_cEnumStart(
	IN WNDOBJ  *pwo,
	IN ULONG  iType,
	IN ULONG  iDirection,
	IN ULONG  cLimit
	)
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
STDCALL
WNDOBJ_vSetConsumer(
	IN WNDOBJ  *pwo,
	IN PVOID  pvConsumer
	)
{
  BOOL Hack;

  DPRINT("WNDOBJ_vSetConsumer: pwo = 0x%x, pvConsumer = 0x%x\n", pwo, pvConsumer);
  
  Hack = (pwo->pvConsumer == NULL);
  pwo->pvConsumer = pvConsumer;

  /* HACKHACKHACK */
  if (Hack)
    {
      IntEngWndChanged(pwo, WOC_RGN_CLIENT);
      IntEngWndChanged(pwo, WOC_CHANGED);
      IntEngWndChanged(pwo, WOC_DRAWN);
    }
}

/* EOF */

