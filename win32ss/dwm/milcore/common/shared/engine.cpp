// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


/**************************************************************************\
*

*
* Abstract:
*
*   Contains miscellaneous engine helper functions.
*
*
\**************************************************************************/

#include "precomp.hpp"

// turn on warning tags by default (DBG builds).
DeclareTagEx(tagMILWarning, "MIL", "MIL Warning output", TRUE);
DeclareTag(tagMILVerbose, "MIL", "MIL Verbose output");

MtDefine(MILImaging, Mem, "MIL Imaging objects");
MtDefine(MIL, Mem, "MIL Common objects");


HINSTANCE g_DllInstance;

/**************************************************************************
*
* Function Description:
*
*   IntersectRect
*   This routine produces the intersection of two source rectangles. It
*   returns TRUE if there is an intersection, FALSE if the intersection
*   is empty. If there is no intersection, prcDst is zeroed.
*
*
**************************************************************************/
template<typename T>
BOOL IntersectRectT(
    __out_ecount(1) T *prcDst,
    __in_ecount(1) const T *prcSrc1,
    __in_ecount(1) const T *prcSrc2
    )
{
    Assert(prcDst);
    Assert(prcSrc1);
    Assert(prcSrc2);

    // we want normalized rects here
    Assert(prcSrc1->Width >= 0);
    Assert(prcSrc2->Width >= 0);
    Assert(prcSrc1->Height >= 0);
    Assert(prcSrc2->Height >= 0);

    //
    // Since prcSrc1 or prclSrc2 may reference the same memory as prclDst
    // don't write results until they won't have a chance of being picked up
    // from either source rect.
    //

    INT Right = min(prcSrc1->X + prcSrc1->Width, prcSrc2->X + prcSrc2->Width);

    prcDst->X  = max(prcSrc1->X, prcSrc2->X);
    prcDst->Width = -prcDst->X + Right;

    // check for empty rect

    if (prcDst->Width > 0)
    {
        INT Bottom = min(prcSrc1->Y + prcSrc1->Height, prcSrc2->Y + prcSrc2->Height);

        prcDst->Y = max(prcSrc1->Y, prcSrc2->Y);
        prcDst->Height = -prcDst->Y + Bottom;

        // check for empty rect

        if (prcDst->Height > 0)
        {
            // Postcondition

            Assert(prcDst->Width >= 0);
            Assert(prcDst->Height >= 0);

            return TRUE;        // not empty
        }
    }

    // empty rect

    GpMemset(prcDst, 0, sizeof(T));

    return FALSE;
}

template
BOOL
IntersectRectT<WICRect>(
    __out_ecount(1) WICRect *prcDst,
    __in_ecount(1) const WICRect *prcSrc1,
    __in_ecount(1) const WICRect *prcSrc2
    );

template
BOOL
IntersectRectT<MilPointAndSizeL>(
    __out_ecount(1) MilPointAndSizeL *prcDst,
    __in_ecount(1) const MilPointAndSizeL *prcSrc1,
    __in_ecount(1) const MilPointAndSizeL *prcSrc2
    );



