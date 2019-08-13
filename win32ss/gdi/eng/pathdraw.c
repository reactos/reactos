/*
 * PROJECT:     ReactOS kernel
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Path drawing API.
 * COPYRIGHT:   Copyright 2019 Katayama Hirofumi MZ.
 */

#include <win32k.h>
#undef XFORMOBJ

#define NDEBUG
#include <debug.h>

BOOL
APIENTRY
EngStrokePath(
    IN SURFOBJ  *pso,
    IN PATHOBJ  *ppo,
    IN CLIPOBJ  *pco,
    IN XFORMOBJ  *pxo,
    IN BRUSHOBJ  *pbo,
    IN POINTL  *pptlBrushOrg,
    IN LINEATTRS  *plineattrs,
    IN MIX  mix)
{
    // www.osr.com/ddk/graphics/gdifncs_4yaw.htm
    UNIMPLEMENTED;
    return FALSE;
}

/*
 * @unimplemented
 */
BOOL
APIENTRY
EngFillPath(
    IN SURFOBJ   *pso,
    IN PATHOBJ   *ppo,
    IN CLIPOBJ   *pco,
    IN BRUSHOBJ  *pbo,
    IN POINTL    *pptlBrushOrg,
    IN MIX        mix,
    IN FLONG      flOptions)
{
    // www.osr.com/ddk/graphics/gdifncs_9pyf.htm
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
EngStrokeAndFillPath(
    IN SURFOBJ  *pso,
    IN PATHOBJ  *ppo,
    IN CLIPOBJ  *pco,
    IN XFORMOBJ  *pxo,
    IN BRUSHOBJ  *pboStroke,
    IN LINEATTRS  *plineattrs,
    IN BRUSHOBJ  *pboFill,
    IN POINTL  *pptlBrushOrg,
    IN MIX  mixFill,
    IN FLONG  flOptions)
{
    // www.osr.com/ddk/graphics/gdifncs_2xwn.htm
    UNIMPLEMENTED;
    return FALSE;
}

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiEngStrokePath(
    _In_ SURFOBJ *pso,
    _In_ PATHOBJ *ppo,
    _In_ CLIPOBJ *pco,
    _In_ XFORMOBJ *pxo,
    _In_ BRUSHOBJ *pbo,
    _In_ POINTL *pptlBrushOrg,
    _In_ LINEATTRS *plineattrs,
    _In_ MIX mix)
{
    BOOL ret = FALSE;
    POINT ptlBrushOrg;
    LINEATTRS lineattrs;

    _SEH2_TRY
    {
        ProbeForRead(pptlBrushOrg, sizeof(*pptlBrushOrg), 1);
        ptlBrushOrg = *pptlBrushOrg;

        ProbeForRead(plineattrs, sizeof(*plineattrs), 1);
        lineattrs = *plineattrs;

        ret = EngStrokePath(pso, ppo, pco, pxo, pbo, &ptlBrushOrg, &lineattrs, mix);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        SetLastNtError(_SEH2_GetExceptionCode());
    }
    _SEH2_END;

    return ret;
}

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiEngFillPath(
    _In_ SURFOBJ *pso,
    _In_ PATHOBJ *ppo,
    _In_ CLIPOBJ *pco,
    _In_ BRUSHOBJ *pbo,
    _In_ POINTL *pptlBrushOrg,
    _In_ MIX mix,
    _In_ FLONG flOptions)
{
    BOOL ret = FALSE;
    POINT ptlBrushOrg;

    _SEH2_TRY
    {
        ProbeForRead(pptlBrushOrg, sizeof(*pptlBrushOrg), 1);
        ptlBrushOrg = *pptlBrushOrg;

        ret = EngFillPath(pso, ppo, pco, pbo, &ptlBrushOrg, mix, flOptions);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        SetLastNtError(_SEH2_GetExceptionCode());
    }
    _SEH2_END;

    return ret;
}

__kernel_entry
W32KAPI
BOOL
APIENTRY
NtGdiEngStrokeAndFillPath(
    _In_ SURFOBJ *pso,
    _In_ PATHOBJ *ppo,
    _In_ CLIPOBJ *pco,
    _In_ XFORMOBJ *pxo,
    _In_ BRUSHOBJ *pboStroke,
    _In_ LINEATTRS *plineattrs,
    _In_ BRUSHOBJ *pboFill,
    _In_ POINTL *pptlBrushOrg,
    _In_ MIX mix,
    _In_ FLONG flOptions)
{
    BOOL ret = FALSE;
    POINT ptlBrushOrg;
    LINEATTRS lineattrs;

    _SEH2_TRY
    {
        ProbeForRead(pptlBrushOrg, sizeof(*pptlBrushOrg), 1);
        ptlBrushOrg = *pptlBrushOrg;

        ProbeForRead(plineattrs, sizeof(*plineattrs), 1);
        lineattrs = *plineattrs;

        ret = EngStrokeAndFillPath(pso, ppo, pco, pxo, pboStroke, &lineattrs, pboFill,
                                   &ptlBrushOrg, mix, flOptions);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        SetLastNtError(_SEH2_GetExceptionCode());
    }
    _SEH2_END;

    return ret;
}

/* EOF */
