/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           GDI Clipping Functions
 * FILE:              subsystems/win32/win32k/eng/clip.c
 * PROGRAMER:         Jason Filby
 */

#include <win32k.h>

#define NDEBUG
#include <debug.h>

static __inline int
CompareRightDown(
    const RECTL *r1,
    const RECTL *r2)
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
    const RECTL *r1,
    const RECTL *r2)
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
    const RECTL *r1,
    const RECTL *r2)
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
    const RECTL *r1,
    const RECTL *r2)
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
    const SPAN *Span1,
    const SPAN *Span2)
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

VOID
FASTCALL
IntEngDeleteClipRegion(CLIPOBJ *ClipObj)
{
    EngFreeMem(ObjToGDI(ClipObj, CLIP));
}

CLIPOBJ*
FASTCALL
IntEngCreateClipRegion(ULONG count, PRECTL pRect, PRECTL rcBounds)
{
    CLIPGDI *Clip;

    if(count > 1)
    {
        RECTL *dest;

        Clip = EngAllocMem(0, sizeof(CLIPGDI) + ((count - 1) * sizeof(RECTL)), GDITAG_CLIPOBJ);

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
        Clip = EngAllocMem(0, sizeof(CLIPGDI), GDITAG_CLIPOBJ);

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
CLIPOBJ *
APIENTRY
EngCreateClip(VOID)
{
    CLIPGDI *Clip = EngAllocMem(FL_ZERO_MEMORY, sizeof(CLIPGDI), GDITAG_CLIPOBJ);
    if(Clip != NULL)
    {
        return GDIToObj(Clip, CLIP);
    }

    return NULL;
}

/*
 * @implemented
 */
VOID
APIENTRY
EngDeleteClip(
    _In_ _Post_ptr_invalid_ CLIPOBJ *pco)
{
    EngFreeMem(ObjToGDI(pco, CLIP));
}

/*
 * @implemented
 */
ULONG
APIENTRY
CLIPOBJ_cEnumStart(
    _Inout_ CLIPOBJ *pco,
    _In_ BOOL bAll,
    _In_ ULONG iType,
    _In_ ULONG iDirection,
    _In_ ULONG cMaxRects)
{
    CLIPGDI *ClipGDI = ObjToGDI(pco, CLIP);
    SORTCOMP CompareFunc;

    ClipGDI->EnumPos = 0;
    ClipGDI->EnumMax = (cMaxRects > 0) ? cMaxRects : ClipGDI->EnumRects.c;

    if (CD_ANY != iDirection && ClipGDI->EnumOrder != iDirection)
    {
        switch (iDirection)
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
                DPRINT1("Invalid iDirection %lu\n", iDirection);
                iDirection = ClipGDI->EnumOrder;
                CompareFunc = NULL;
                break;
        }

        if (NULL != CompareFunc)
        {
            EngSort((PBYTE) ClipGDI->EnumRects.arcl, sizeof(RECTL), ClipGDI->EnumRects.c, CompareFunc);
        }

        ClipGDI->EnumOrder = iDirection;
    }

    /* Return the number of rectangles enumerated */
    if ((cMaxRects > 0) && (ClipGDI->EnumRects.c > cMaxRects))
    {
        return 0xFFFFFFFF;
    }

    return ClipGDI->EnumRects.c;
}

/*
 * @implemented
 */
BOOL
APIENTRY
CLIPOBJ_bEnum(
    _In_ CLIPOBJ *pco,
    _In_ ULONG cj,
    _Out_bytecap_(cj) ULONG *pulEnumRects)
{
    RECTL *dest, *src;
    CLIPGDI *ClipGDI = ObjToGDI(pco, CLIP);
    ULONG nCopy, i;
    ENUMRECTS* pERects = (ENUMRECTS*)pulEnumRects;

    // Calculate how many rectangles we should copy
    nCopy = min( ClipGDI->EnumMax - ClipGDI->EnumPos,
            min( ClipGDI->EnumRects.c - ClipGDI->EnumPos,
            (cj - sizeof(ULONG)) / sizeof(RECTL)));

    if(nCopy == 0)
    {
        return FALSE;
    }

    /* Copy rectangles */
    src = ClipGDI->EnumRects.arcl + ClipGDI->EnumPos;
    for(i = 0, dest = pERects->arcl; i < nCopy; i++, dest++, src++)
    {
        *dest = *src;
    }

    pERects->c = nCopy;

    ClipGDI->EnumPos+=nCopy;

    return ClipGDI->EnumPos < ClipGDI->EnumRects.c;
}

/* EOF */
