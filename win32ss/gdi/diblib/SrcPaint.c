
#include "DibLib.h"

#define __USES_SOURCE 1
#define __USES_PATTERN 0
#define __USES_DEST 1
#define __USES_MASK 0

#define __FUNCTIONNAME SrcPaint

#define _DibDoRop(pBltData, M, D, S, P) pBltData->apfnDoRop[0](D,S,0)

#include "DibLib_AllSrcBPP.h"

VOID
FASTCALL
Dib_SrcPaint(PBLTDATA pBltData)
{
    // TODO: XLATEless same-surface variants
    gapfnSrcPaint[pBltData->siDst.iFormat][pBltData->siSrc.iFormat](pBltData);
}



