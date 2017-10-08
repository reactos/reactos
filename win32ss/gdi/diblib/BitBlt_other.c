
#include "DibLib.h"

extern PFN_DIBFUNCTION gapfnBitBlt_PATCOPY_Solid[];

VOID
FASTCALL
Dib_BitBlt_BLACKNESS(PBLTDATA pBltData)
{
    /* Pass it to the colorfil function */
    pBltData->ulSolidColor = XLATEOBJ_iXlate(pBltData->pxlo, 0);
    gapfnBitBlt_PATCOPY_Solid[pBltData->siDst.iFormat](pBltData);
}

VOID
FASTCALL
Dib_BitBlt_WHITENESS(PBLTDATA pBltData)
{
    /* Pass it to the colorfil function */
    pBltData->ulSolidColor = XLATEOBJ_iXlate(pBltData->pxlo, 0xFFFFFF);
    gapfnBitBlt_PATCOPY_Solid[pBltData->siDst.iFormat](pBltData);
}

VOID
FASTCALL
Dib_BitBlt_NOOP(PBLTDATA pBltData)
{
    UNREFERENCED_PARAMETER(pBltData);
}
