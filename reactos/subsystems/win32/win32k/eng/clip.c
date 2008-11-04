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
/* $Id$
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

#define NDEBUG
#include <debug.h>

static __inline int
CompareRightDown(
    const PRECT r1,
    const PRECT r2)
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

static __inline int
CompareRightUp(
    const PRECT r1,
    const PRECT r2)
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

static __inline int
CompareLeftDown(
    const PRECT r1,
    const PRECT r2)
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

static __inline int
CompareLeftUp(
    const PRECT r1,
    const PRECT r2)
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

static __inline int
CompareSpans(
    const PSPAN Span1,
    const PSPAN Span2)
{
    int Cmp;

    if (Span1->Y < Span2->Y)
    {
        Cmp = -1;
    }
    else if (Span2->Y < Span1->Y)
    {
        Cmp = +1;
    }
    else
    {
        if (Span1->X < Span2->X)
        {
            Cmp = -1;
        }
        else if (Span2->X < Span1->X)
        {
            Cmp = +1;
        }
        else
        {
            Cmp = 0;
        }
    }

    return Cmp;
}

VOID FASTCALL
IntEngDeleteClipRegion(CLIPOBJ *ClipObj)
{
    EngFreeMem(ObjToGDI(ClipObj, CLIP));
}

CLIPOBJ* FASTCALL
IntEngCreateClipRegion(ULONG count, PRECTL pRect, PRECTL rcBounds)
{
    CLIPGDI *Clip;

    if(count > 1)
    {
        RECTL *dest;

        Clip = EngAllocMem(0, sizeof(CLIPGDI) + ((count - 1) * sizeof(RECTL)), TAG_CLIPOBJ);

        if(Clip != NULL)
        {
            Clip->EnumRects.c = count;
            Clip->EnumOrder = CD_ANY;
            for(dest = Clip->EnumRects.arcl;count > 0; count--, dest++, pRect++)
            {
                *dest = *pRect;
            }

            Clip->ClipObj.iDComplexity = DC_COMPLEX;
            Clip->ClipObj.iFComplexity = ((Clip->EnumRects.c <= 4) ? FC_RECT4 : FC_COMPLEX);
            Clip->ClipObj.iMode = TC_RECTANGLES;
            Clip->ClipObj.rclBounds = *rcBounds;

            return GDIToObj(Clip, CLIP);
        }
    }
    else
    {
        Clip = EngAllocMem(0, sizeof(CLIPGDI), TAG_CLIPOBJ);

        if(Clip != NULL)
        {
            Clip->EnumRects.c = 1;
            Clip->EnumOrder = CD_ANY;
            Clip->EnumRects.arcl[0] = *rcBounds;

            Clip->ClipObj.iDComplexity = (((rcBounds->top == rcBounds->bottom) &&
                                         (rcBounds->left == rcBounds->right))
                                         ? DC_TRIVIAL : DC_RECT);

            Clip->ClipObj.iFComplexity = FC_RECT;
            Clip->ClipObj.iMode = TC_RECTANGLES;
            Clip->ClipObj.rclBounds = *rcBounds;

            return GDIToObj(Clip, CLIP);
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
    CLIPGDI *Clip = EngAllocMem(FL_ZERO_MEMORY, sizeof(CLIPGDI), TAG_CLIPOBJ);
    if(Clip != NULL)
    {
        return GDIToObj(Clip, CLIP);
    }

    return NULL;
}

/*
 * @implemented
 */
VOID STDCALL
EngDeleteClip(CLIPOBJ *ClipRegion)
{
    EngFreeMem(ObjToGDI(ClipRegion, CLIP));
}

/*
 * @implemented
 */
ULONG STDCALL
CLIPOBJ_cEnumStart(
    IN CLIPOBJ* ClipObj,
    IN BOOL ShouldDoAll,
    IN ULONG ClipType,
    IN ULONG BuildOrder,
    IN ULONG MaxRects)
{
    CLIPGDI *ClipGDI = ObjToGDI(ClipObj, CLIP);
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
            EngSort((PBYTE) ClipGDI->EnumRects.arcl, sizeof(RECTL), ClipGDI->EnumRects.c, CompareFunc);
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
CLIPOBJ_bEnum(
    IN CLIPOBJ* ClipObj,
    IN ULONG ObjSize,
    OUT ULONG *EnumRects)
{
    RECTL *dest, *src;
    CLIPGDI *ClipGDI = ObjToGDI(ClipObj, CLIP);
    ULONG nCopy, i;
    ENUMRECTS* pERects = (ENUMRECTS*)EnumRects;

    //calculate how many rectangles we should copy
    nCopy = min( ClipGDI->EnumMax - ClipGDI->EnumPos,
            min( ClipGDI->EnumRects.c - ClipGDI->EnumPos,
            (ObjSize - sizeof(ULONG)) / sizeof(RECTL)));

    if(nCopy == 0)
    {
        return FALSE;
    }

    /* copy rectangles */
    src = ClipGDI->EnumRects.arcl + ClipGDI->EnumPos;
    for(i = 0, dest = pERects->arcl; i < nCopy; i++, dest++, src++)
    {
        *dest = *src;
    }

    pERects->c = nCopy;

    ClipGDI->EnumPos+=nCopy;

    return ClipGDI->EnumPos < ClipGDI->EnumRects.c;
}

BOOLEAN FASTCALL
ClipobjToSpans(
    PSPAN *Spans,
    UINT *Count,
    CLIPOBJ *ClipRegion,
    PRECTL Boundary)
{
    BOOL EnumMore;
    UINT i, NewCount;
    RECT_ENUM RectEnum;
    PSPAN NewSpans;
    RECTL *Rect;

    ASSERT(Boundary->top <= Boundary->bottom && Boundary->left <= Boundary->right);

    *Spans = NULL;
    if (NULL == ClipRegion || DC_TRIVIAL == ClipRegion->iDComplexity)
    {
        *Count = Boundary->bottom - Boundary->top;
        if (0 != *Count)
        {
            *Spans = ExAllocatePoolWithTag(PagedPool, *Count * sizeof(SPAN), TAG_CLIP);
            if (NULL == *Spans)
            {
                *Count = 0;
                return FALSE;
            }
            for (i = 0; i < Boundary->bottom - Boundary->top; i++)
            {
                (*Spans)[i].X = Boundary->left;
                (*Spans)[i].Y = Boundary->top + i;
                (*Spans)[i].Width = Boundary->right - Boundary->left;
            }
        }
      return TRUE;
    }

    *Count = 0;
    CLIPOBJ_cEnumStart(ClipRegion, FALSE, CT_RECTANGLES, CD_ANY, 0);
    do
    {
        EnumMore = CLIPOBJ_bEnum(ClipRegion, (ULONG) sizeof(RECT_ENUM), (PVOID) &RectEnum);

        NewCount = *Count;
        for (i = 0; i < RectEnum.c; i++)
        {
            NewCount += RectEnum.arcl[i].bottom - RectEnum.arcl[i].top;
        }
        if (NewCount != *Count)
        {
            NewSpans = ExAllocatePoolWithTag(PagedPool, NewCount * sizeof(SPAN), TAG_CLIP);
            if (NULL == NewSpans)
            {
                if (NULL != *Spans)
                {
                    ExFreePoolWithTag(*Spans, TAG_CLIP);
                    *Spans = NULL;
                }
                *Count = 0;
                return FALSE;
            }
            if (0 != *Count)
            {
                PSPAN dest, src;
                UINT i = *Count;
                for(dest = NewSpans, src = *Spans;i > 0; i--)
                {
                    *dest++ = *src++;
                }
                ExFreePoolWithTag(*Spans, TAG_CLIP);
            }
            *Spans = NewSpans;
        }
        for (Rect = RectEnum.arcl; Rect < RectEnum.arcl + RectEnum.c; Rect++)
        {
            for (i = 0; i < Rect->bottom - Rect->top; i++)
            {
                (*Spans)[*Count].X = Rect->left;
                (*Spans)[*Count].Y = Rect->top + i;
                (*Spans)[*Count].Width = Rect->right - Rect->left;
                (*Count)++;
            }
        }
        ASSERT(*Count == NewCount);
    }
    while (EnumMore);

    if (0 != *Count)
    {
        EngSort((PBYTE) *Spans, sizeof(SPAN), *Count, (SORTCOMP) CompareSpans);
    }

    return TRUE;
}

/* EOF */
