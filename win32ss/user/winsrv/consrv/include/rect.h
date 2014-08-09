/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Console Server DLL
 * FILE:            consrv/include/rect.h
 * PURPOSE:         Rectangle helper functions
 * PROGRAMMERS:     Gé van Geldorp
 *                  Jeffrey Morlan
 */

#pragma once

#define ConioInitRect(Rect, top, left, bottom, right) \
do {    \
    ((Rect)->Top) = top;    \
    ((Rect)->Left) = left;  \
    ((Rect)->Bottom) = bottom;  \
    ((Rect)->Right) = right;    \
} while (0)
#define ConioIsRectEmpty(Rect) \
    (((Rect)->Left > (Rect)->Right) || ((Rect)->Top > (Rect)->Bottom))

#define ConioRectHeight(Rect) \
    (((Rect)->Top) > ((Rect)->Bottom) ? 0 : ((Rect)->Bottom) - ((Rect)->Top) + 1)
#define ConioRectWidth(Rect) \
    (((Rect)->Left) > ((Rect)->Right) ? 0 : ((Rect)->Right) - ((Rect)->Left) + 1)


static __inline BOOLEAN
ConioGetIntersection(OUT PSMALL_RECT Intersection,
                     IN PSMALL_RECT Rect1,
                     IN PSMALL_RECT Rect2)
{
    if ( ConioIsRectEmpty(Rect1) ||
         ConioIsRectEmpty(Rect2) ||
        (Rect1->Top  > Rect2->Bottom) ||
        (Rect1->Left > Rect2->Right)  ||
        (Rect1->Bottom < Rect2->Top)  ||
        (Rect1->Right  < Rect2->Left) )
    {
        /* The rectangles do not intersect */
        ConioInitRect(Intersection, 0, -1, 0, -1);
        return FALSE;
    }

    ConioInitRect(Intersection,
                  max(Rect1->Top   , Rect2->Top   ),
                  max(Rect1->Left  , Rect2->Left  ),
                  min(Rect1->Bottom, Rect2->Bottom),
                  min(Rect1->Right , Rect2->Right ));

    return TRUE;
}

static __inline BOOLEAN
ConioGetUnion(OUT PSMALL_RECT Union,
              IN PSMALL_RECT Rect1,
              IN PSMALL_RECT Rect2)
{
    if (ConioIsRectEmpty(Rect1))
    {
        if (ConioIsRectEmpty(Rect2))
        {
            ConioInitRect(Union, 0, -1, 0, -1);
            return FALSE;
        }
        else
        {
            *Union = *Rect2;
        }
    }
    else if (ConioIsRectEmpty(Rect2))
    {
        *Union = *Rect1;
    }
    else
    {
        ConioInitRect(Union,
                      min(Rect1->Top   , Rect2->Top   ),
                      min(Rect1->Left  , Rect2->Left  ),
                      max(Rect1->Bottom, Rect2->Bottom),
                      max(Rect1->Right , Rect2->Right ));
    }

    return TRUE;
}
