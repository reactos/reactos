
#if __USES_SOLID_BRUSH
#undef __USES_PATTERN
#define __USES_PATTERN 0
#endif

#define _DibFunction __DIB_FUNCTION_NAME(__FUNCTIONNAME, _SOURCE_BPP, _DEST_BPP)
#define _ReadPixel(bpp, pj, jShift) __PASTE(_ReadPixel_, bpp)(pj, jShift)
#define _WritePixel(pj, jShift, c) __PASTE(_WritePixel_, _DEST_BPP)(pj, jShift, c)
#define _NextPixel(bpp, ppj, pjShift) __PASTE(_NextPixel_, bpp)(ppj, pjShift)
#define _SHIFT(bpp, x) __PASTE(_SHIFT_, bpp)(x)
#define _CALCSHIFT(bpp, pshift, x) __PASTE(_CALCSHIFT_, bpp)(pshift, x)

#if (__PASTE(_DibFunction, _manual) != 1)

VOID
FASTCALL
_DibFunction(PBLTDATA pBltData)
{
    ULONG cRows, cLines, ulDest;
    PBYTE pjDest, pjDestBase;
    _SHIFT(_DEST_BPP, BYTE jDstShift;)
#if __USES_MASK
    PBYTE pjMask, pjMaskBase;
    BYTE jMaskBit, jMskShift;
#endif
#if __USES_SOURCE
    PBYTE pjSource, pjSrcBase;
    ULONG ulSource;
    _SHIFT(_SOURCE_BPP, BYTE jSrcShift;)
#endif
#if __USES_PATTERN
    PBYTE pjPattern, pjPatBase;
    ULONG ulPattern, cPatRows, cPatLines;
    _SHIFT(_DEST_BPP, BYTE jPatShift;)
#endif
#if __USES_SOLID_BRUSH
    ULONG ulPattern = pBltData->ulSolidColor;
#endif

#if __USES_MASK
    pjMaskBase = pBltData->siMsk.pjBase;
#endif
#if __USES_PATTERN
    pjPatBase = pBltData->siPat.pjBase;
    pjPatBase += pBltData->siPat.ptOrig.y * pBltData->siPat.lDelta;
    cPatLines = pBltData->ulPatHeight - pBltData->siPat.ptOrig.y;
#endif
    pjDestBase = pBltData->siDst.pjBase;
#if __USES_SOURCE
    pjSrcBase = pBltData->siSrc.pjBase;
#endif

    /* Loop all lines */
    cLines = pBltData->ulHeight;
    while (cLines--)
    {
        /* Set current bit pointers and shifts */
        pjDest = pjDestBase;
        _CALCSHIFT(_DEST_BPP, &jDstShift, pBltData->siDst.ptOrig.x);
#if __USES_SOURCE
        pjSource = pjSrcBase;
        _CALCSHIFT(_SOURCE_BPP, &jSrcShift, pBltData->siSrc.ptOrig.x);
#endif
#if __USES_MASK
        pjMask = pjMaskBase;
        _CALCSHIFT_1(&jMskShift, pBltData->siMsk.ptOrig.x);
#endif
#if __USES_PATTERN
        pjPattern = pjPatBase + pBltData->siPat.ptOrig.x * _DEST_BPP / 8;
        _CALCSHIFT(_DEST_BPP, &jPatShift, pBltData->siPat.ptOrig.x);
        cPatRows = pBltData->ulPatWidth - pBltData->siPat.ptOrig.x;
#endif

        /* Loop all rows */
        cRows = pBltData->ulWidth;
        while (cRows--)
        {
#if __USES_MASK
            /* Read the mask color and go to the next mask pixel */
            jMaskBit = _ReadPixel_1(pjMask, jMskShift);
            _NextPixel(1, &pjMask, &jMskShift);
#endif
#if __USES_PATTERN
            /* Read the pattern color and go to the next pattern pixel */
            ulPattern = _ReadPixel(_DEST_BPP, pjPattern, jPatShift);
            _NextPixel(_DEST_BPP, &pjPattern, &jPatShift);

            /* Check if this was the last pixel in the pattern */
            if (--cPatRows == 0)
            {
                /* Restart pattern from x = 0 */
                pjPattern = pjPatBase;
                _SHIFT(_DEST_BPP, jPatShift = (_DEST_BPP == 1) ? 7 : 4;)
                cPatRows = pBltData->ulPatWidth;
            }
#endif
#if __USES_SOURCE
            /* Read the pattern color, xlate it and go to the next pixel */
            ulSource = _ReadPixel(_SOURCE_BPP, pjSource, jSrcShift);
            ulSource = _DibXlate(pBltData, ulSource);
            _NextPixel(_SOURCE_BPP, &pjSource, &jSrcShift);
#endif
#if __USES_DEST
            ulDest = _ReadPixel(_DEST_BPP, pjDest, jDstShift);
#endif
            /* Apply the ROP operation on the colors */
            ulDest = _DibDoRop(pBltData, jMaskBit, ulDest, ulSource, ulPattern);

            /* Write the pixel and go to the next dest pixel */
            _WritePixel(pjDest, jDstShift, ulDest);
            _NextPixel(_DEST_BPP, &pjDest, &jDstShift);
        }

        pjDestBase += pBltData->siDst.cjAdvanceY;
#if __USES_SOURCE
        pjSrcBase += pBltData->siSrc.cjAdvanceY;
#endif
#if __USES_PATTERN
        /* Go to the next pattern line */
        pjPatBase += pBltData->siPat.cjAdvanceY;

        /* Check if this was the last line in the pattern */
        if (--cPatLines == 0)
        {
            /* Restart pattern from y = 0 */
            pjPatBase = pBltData->siPat.pjBase;
            cPatLines = pBltData->ulPatHeight;
        }
#endif
    }
}

#endif // manual

#undef _DibFunction
#undef __FUNCTIONNAME2
