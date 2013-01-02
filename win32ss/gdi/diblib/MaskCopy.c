
#include "DibLib.h"

extern PFN_DIBFUNCTION gapfnBitBlt_SRCCOPY[7][7];
extern PFN_DIBFUNCTION gapfnBitBlt_SRCINVERT[7][7];

VOID
FASTCALL
Dib_MaskCopy(PBLTDATA pBltData)
{
    pBltData->siSrc = pBltData->siMsk;

    /* Create an XLATEOBJ */
    pBltData->pxlo = 0;// FIXME: use 1bpp -> destbpp

    /* 4 possibilities... */
    if (pBltData->rop4 == MAKEROP4(BLACKNESS, WHITENESS))
    {
        gapfnBitBlt_SRCCOPY[pBltData->siDst.iFormat][pBltData->siSrc.iFormat](pBltData);
    }
    else if (pBltData->rop4 == MAKEROP4(WHITENESS, BLACKNESS))
    {
        gapfnBitBlt_SRCINVERT[pBltData->siDst.iFormat][pBltData->siSrc.iFormat](pBltData);
    }
    else if (pBltData->rop4 == MAKEROP4(BLACKNESS, BLACKNESS))
    {
        Dib_BitBlt_BLACKNESS(pBltData);
    }
    else // if (pBltData->rop4 == MAKEROP4(WHITENESS, WHITENESS))
    {
        Dib_BitBlt_WHITENESS(pBltData);
    }
}


