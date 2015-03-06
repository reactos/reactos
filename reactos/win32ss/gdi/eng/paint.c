/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           GDI Driver Paint Functions
 * FILE:              subsys/win32k/eng/paint.c
 * PROGRAMERS:        Timo Kreuzer (timo.kreuzer@reactos.org)
 *                    Jason Filby
 */

#include <win32k.h>

#define NDEBUG
#include <debug.h>

const BYTE gajRop2ToRop3[16] =
{
    0x00, //  1: R2_BLACK        0
    0x05, //  2: R2_NOTMERGEPEN  DPon
    0x0A, //  3: R2_MASKNOTPEN   DPna
    0x0F, //  4: R2_NOTCOPYPEN   Pn
    0x50, //  5: R2_MASKPENNOT   PDna
    0x55, //  6: R2_NOT          Dn
    0x5A, //  7: R2_XORPEN       DPx
    0x5F, //  8: R2_NOTMASKPEN   DPan
    0xA0, //  9: R2_MASKPEN      DPa
    0xA5, // 10: R2_NOTXORPEN    PDxn
    0xAA, // 11: R2_NOP          D
    0xAF, // 12: R2_MERGENOTPEN  DPno
    0xF0, // 13: R2_COPYPEN      P
    0xF5, // 14: R2_MERGEPENNOT  PDno
    0xFA, // 15: R2_MERGEPEN     DPo
    0xFF, // 16: R2_WHITE        1
};

BOOL APIENTRY FillSolid(SURFOBJ *pso, PRECTL pRect, ULONG iColor)
{
  LONG y;
  ULONG LineWidth;

  ASSERT(pso);
  ASSERT(pRect);
  LineWidth  = pRect->right - pRect->left;
  DPRINT(" LineWidth: %lu, top: %ld, bottom: %ld\n", LineWidth, pRect->top, pRect->bottom);
  for (y = pRect->top; y < pRect->bottom; y++)
  {
    DibFunctionsForBitmapFormat[pso->iBitmapFormat].DIB_HLine(
      pso, pRect->left, pRect->right, y, iColor);
  }
  return TRUE;
}

BOOL
APIENTRY
EngPaint(
    _In_ SURFOBJ *pso,
    _In_ CLIPOBJ *pco,
    _In_ BRUSHOBJ *pbo,
    _In_ POINTL *pptlBrushOrg,
    _In_ __in_data_source(USER_MODE) MIX mix)
{
    ROP4 rop4;

    /* Convert the MIX, consisting of 2 ROP2 codes into a ROP4 */
    rop4 = MIX_TO_ROP4(mix);

    /* Sanity check */
    NT_ASSERT(!ROP4_USES_SOURCE(rop4));

    /* Forward the call to Eng/DrvBitBlt */
    return IntEngBitBlt(pso,
                        NULL,
                        NULL,
                        pco,
                        NULL,
                        &pco->rclBounds,
                        NULL,
                        NULL,
                        pbo,
                        pptlBrushOrg,
                        rop4);
}

BOOL
APIENTRY
IntEngPaint(
    _In_ SURFOBJ *pso,
    _In_ CLIPOBJ *pco,
    _In_ BRUSHOBJ *pbo,
    _In_ POINTL *pptlBrushOrg,
    _In_ __in_data_source(USER_MODE) MIX mix)
{
    SURFACE *psurf = CONTAINING_RECORD(pso, SURFACE, SurfObj);

    /* Is the surface's Paint function hooked? */
    if ((pso->iType != STYPE_BITMAP) && (psurf->flags & HOOK_PAINT))
    {
        /* Call the driver's DrvPaint */
        return GDIDEVFUNCS(pso).Paint(pso, pco, pbo, pptlBrushOrg, mix);
    }

    return EngPaint(pso, pco, pbo, pptlBrushOrg, mix);
}

/* EOF */
