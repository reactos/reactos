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
/* $Id: window.c,v 1.11 2004/05/10 17:07:17 blight Exp $
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

  DPRINT1("Calling WNDOBJCHANGEPROC (0x%x), Changed = 0x%x\n",
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

  DPRINT("EngCreateWnd: WNDOBJCHANGEPROC = 0x%x, Flags = 0x%x\n", pfn, fl);

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

  /* HACKHACKHACK */
  IntEngWndChanged(WndObjUser, WOC_RGN_CLIENT);

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
  
  DPRINT("EngDeleteWnd\n");

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
  DPRINT("WNDOBJ_bEnum\n");
  return CLIPOBJ_bEnum(WndObjInt->ClientClipObj, cj, pul);
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
  DPRINT("WNDOBJ_cEnumStart\n");
  /* FIXME: Should we enumerate all rectangles or not? */
  return CLIPOBJ_cEnumStart(WndObjInt->ClientClipObj, FALSE, iType, iDirection, cLimit);
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
  pwo->pvConsumer = pvConsumer;
}

/* EOF */

