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
/* $Id: clip.c,v 1.20 2004/05/10 17:07:17 weiden Exp $
 * 
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           GDI Clipping Functions
 * FILE:              subsys/win32k/eng/clip.c
 * PROGRAMER:         Jason Filby
 * REVISION HISTORY:
 *                 21/8/1999: Created
 */
#include <w32k.h>

VOID STDCALL IntEngDeleteClipRegion(CLIPOBJ *ClipObj)
{
  HCLIP HClip      = AccessHandleFromUserObject(ClipObj);
  FreeGDIHandle(HClip);
}

CLIPOBJ* STDCALL
IntEngCreateClipRegion(ULONG count, PRECTL pRect, PRECTL rcBounds)
{
  HCLIP hClip;
  CLIPGDI* clipInt;
  CLIPOBJ* clipUser;

  DPRINT("IntEngCreateClipRegion count: %d\n", count);
  if (1 < count)
    {
      hClip = (HCLIP) CreateGDIHandle(sizeof(CLIPGDI) + count * sizeof(RECTL),
                                      sizeof(CLIPOBJ));

      if (hClip)
	{
	  clipInt = (CLIPGDI *) AccessInternalObject(hClip);
	  RtlCopyMemory(clipInt->EnumRects.arcl, pRect, count * sizeof(RECTL));
	  clipInt->EnumRects.c = count;
	  clipInt->EnumOrder = CD_ANY;

	  clipUser = (CLIPOBJ *) AccessUserObject(hClip);
	  ASSERT(NULL != clipUser);

	  clipUser->iDComplexity = DC_COMPLEX;
	  clipUser->iFComplexity = (count <= 4) ? FC_RECT4: FC_COMPLEX;
	  clipUser->iMode = TC_RECTANGLES;
	  RtlCopyMemory(&(clipUser->rclBounds), rcBounds, sizeof(RECTL));

	  return clipUser;
	}
    }
  else
    {
      hClip = (HCLIP) CreateGDIHandle(sizeof(CLIPGDI),
	                              sizeof(CLIPOBJ));
      if (hClip)
	{
	  clipInt = (CLIPGDI *) AccessInternalObject(hClip);
	  RtlCopyMemory(clipInt->EnumRects.arcl, rcBounds, sizeof(RECTL));
	  clipInt->EnumRects.c = 1;
	  clipInt->EnumOrder = CD_ANY;

	  clipUser = (CLIPOBJ *) AccessUserObject(hClip);
	  ASSERT(NULL != clipUser);

	  clipUser->iDComplexity = ((rcBounds->top==rcBounds->bottom)
	                            && (rcBounds->left==rcBounds->right))
	                           ? DC_TRIVIAL : DC_RECT;
	  clipUser->iFComplexity = FC_RECT;
	  clipUser->iMode = TC_RECTANGLES;
	  DPRINT("IntEngCreateClipRegion: iDComplexity: %d\n", clipUser->iDComplexity);
	  RtlCopyMemory(&(clipUser->rclBounds), rcBounds, sizeof(RECTL));
	  return clipUser;
	}
    }

  return NULL;
}

/*
 * @implemented
 */
CLIPOBJ * STDCALL
EngCreateClip(VOID)
{
  return EngAllocMem(FL_ZERO_MEMORY, sizeof(CLIPOBJ), 0);
}

/*
 * @implemented
 */
VOID STDCALL
EngDeleteClip(CLIPOBJ *ClipRegion)
{
  EngFreeMem(ClipRegion);
}

static int 
CompareRightDown(const PRECT r1, const PRECT r2)
{
  int Cmp;

  if (r1->top < r2->top)
    {
      Cmp = -1;
    }
  else if (r2->top < r1->top)
    {
      Cmp = +1;
    }
  else 
    {
      ASSERT(r1->bottom == r2->bottom);
      if (r1->left < r2->left)
	{
	  Cmp = -1;
	}
      else if (r2->left < r1->left)
	{
	  Cmp = +1;
	}
      else
	{
	  ASSERT(r1->right == r2->right);
	  Cmp = 0;
	}
    }

  return Cmp;
}

static int 
CompareRightUp(const PRECT r1, const PRECT r2)
{
  int Cmp;

  if (r1->bottom < r2->bottom)
    {
      Cmp = +1;
    }
  else if (r2->bottom < r1->bottom)
    {
      Cmp = -1;
    }
  else 
    {
      ASSERT(r1->top == r2->top);
      if (r1->left < r2->left)
	{
	  Cmp = -1;
	}
      else if (r2->left < r1->left)
	{
	  Cmp = +1;
	}
      else
	{
	  ASSERT(r1->right == r2->right);
	  Cmp = 0;
	}
    }

  return Cmp;
}

static int 
CompareLeftDown(const PRECT r1, const PRECT r2)
{
  int Cmp;

  if (r1->top < r2->top)
    {
      Cmp = -1;
    }
  else if (r2->top < r1->top)
    {
      Cmp = +1;
    }
  else 
    {
      ASSERT(r1->bottom == r2->bottom);
      if (r1->right < r2->right)
	{
	  Cmp = +1;
	}
      else if (r2->right < r1->right)
	{
	  Cmp = -1;
	}
      else
	{
	  ASSERT(r1->left == r2->left);
	  Cmp = 0;
	}
    }

  return Cmp;
}

static int 
CompareLeftUp(const PRECT r1, const PRECT r2)
{
  int Cmp;

  if (r1->bottom < r2->bottom)
    {
      Cmp = +1;
    }
  else if (r2->bottom < r1->bottom)
    {
      Cmp = -1;
    }
  else 
    {
      ASSERT(r1->top == r2->top);
      if (r1->right < r2->right)
	{
	  Cmp = +1;
	}
      else if (r2->right < r1->right)
	{
	  Cmp = -1;
	}
      else
	{
	  ASSERT(r1->left == r2->left);
	  Cmp = 0;
	}
    }

  return Cmp;
}

/*
 * @implemented
 */
ULONG STDCALL
CLIPOBJ_cEnumStart(IN CLIPOBJ* ClipObj,
		   IN BOOL ShouldDoAll,
		   IN ULONG ClipType,
		   IN ULONG BuildOrder,
		   IN ULONG MaxRects)
{
  CLIPGDI *ClipGDI = (CLIPGDI*)AccessInternalObjectFromUserObject(ClipObj);
  SORTCOMP CompareFunc;

  ClipGDI->EnumPos = 0;
  ClipGDI->EnumMax = (MaxRects > 0) ? MaxRects : ClipGDI->EnumRects.c;

  if (CD_ANY != BuildOrder && ClipGDI->EnumOrder != BuildOrder)
    {
      switch (BuildOrder)
	{
	case CD_RIGHTDOWN:
	  CompareFunc = (SORTCOMP) CompareRightDown;
	  break;
	case CD_RIGHTUP:
	  CompareFunc = (SORTCOMP) CompareRightUp;
	  break;
	case CD_LEFTDOWN:
	  CompareFunc = (SORTCOMP) CompareLeftDown;
	  break;
	case CD_LEFTUP:
	  CompareFunc = (SORTCOMP) CompareLeftUp;
	  break;
	default:
	  DPRINT1("Invalid BuildOrder %d\n", BuildOrder);
	  BuildOrder = ClipGDI->EnumOrder;
	  CompareFunc = NULL;
	  break;
	}

      if (NULL != CompareFunc)
	{
	  EngSort((PBYTE) ClipGDI->EnumRects.arcl, sizeof(RECTL), ClipGDI->EnumRects.c,
	          CompareFunc);
	}

      ClipGDI->EnumOrder = BuildOrder;
    }

  /* Return the number of rectangles enumerated */
  if ((MaxRects > 0) && (ClipGDI->EnumRects.c > MaxRects))
    {
      return 0xFFFFFFFF;
    }

  return ClipGDI->EnumRects.c;
}

/*
 * @implemented
 */
BOOL STDCALL
CLIPOBJ_bEnum(IN CLIPOBJ* ClipObj,
	      IN ULONG ObjSize,
	      OUT ULONG *EnumRects)
{
  CLIPGDI *ClipGDI = (CLIPGDI*)AccessInternalObjectFromUserObject(ClipObj);
  ULONG nCopy;
  ENUMRECTS* pERects = (ENUMRECTS*)EnumRects;

  //calculate how many rectangles we should copy
  nCopy = min( ClipGDI->EnumMax - ClipGDI->EnumPos,
               min( ClipGDI->EnumRects.c - ClipGDI->EnumPos,
                    (ObjSize - sizeof(ULONG)) / sizeof(RECTL)));
  if(nCopy == 0)
  {
    return FALSE;
  }
  RtlCopyMemory( pERects->arcl, ClipGDI->EnumRects.arcl + ClipGDI->EnumPos,
                 nCopy * sizeof(RECTL) );
  pERects->c = nCopy;

  ClipGDI->EnumPos+=nCopy;

  return ClipGDI->EnumPos < ClipGDI->EnumRects.c;
}

/* EOF */
