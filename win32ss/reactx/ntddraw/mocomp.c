/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Native DirectDraw implementation
 * FILE:             win32ss/reactx/ntddraw/mocomp.c
 * PROGRAMER:        Magnus Olsen (magnus@greatlord.com)
 * REVISION HISTORY:
 *       19/1-2006   Magnus Olsen
 */

#include <win32k.h>

// #define NDEBUG
#include <debug.h>

/************************************************************************/
/* NtGdiDdBeginMoCompFrame                                              */
/************************************************************************/
DWORD
APIENTRY
NtGdiDdBeginMoCompFrame(HANDLE hMoComp,
                        PDD_BEGINMOCOMPFRAMEDATA puBeginFrameData)
{
    PGD_DDBEGINMOCOMPFRAME pfnDdBeginMoCompFrame = (PGD_DDBEGINMOCOMPFRAME)gpDxFuncs[DXG_INDEX_DxDdBeginMoCompFrame].pfn;

    if (pfnDdBeginMoCompFrame == NULL)
    {
        DPRINT1("Warning: no pfnDdBeginMoCompFrame\n");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT("Calling dxg.sys pfnDdBeginMoCompFrame\n");
    return pfnDdBeginMoCompFrame(hMoComp,puBeginFrameData);
}

/************************************************************************/
/* NtGdiDdCreateMoComp                                                  */
/************************************************************************/
HANDLE
APIENTRY
NtGdiDdCreateMoComp(HANDLE hDirectDraw, PDD_CREATEMOCOMPDATA puCreateMoCompData)
{
    PGD_DXDDCREATEMOCOMP pfnDdCreateMoComp = (PGD_DXDDCREATEMOCOMP)gpDxFuncs[DXG_INDEX_DxDdCreateMoComp].pfn;

    if (pfnDdCreateMoComp == NULL)
    {
        DPRINT1("Warning: no pfnDdCreateMoComp\n");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT("Calling dxg.sys pfnDdCreateMoComp\n");
    return pfnDdCreateMoComp(hDirectDraw, puCreateMoCompData);
}

/************************************************************************/
/* NtGdiDdDestroyMoComp                                                 */
/************************************************************************/
DWORD
APIENTRY
NtGdiDdDestroyMoComp(HANDLE hMoComp,
                     PDD_DESTROYMOCOMPDATA puBeginFrameData)
{
    PGD_DXDDDESTROYMOCOMP pfnDdDestroyMoComp =
        (PGD_DXDDDESTROYMOCOMP)gpDxFuncs[DXG_INDEX_DxDdDestroyMoComp].pfn;

    if (pfnDdDestroyMoComp == NULL)
    {
        DPRINT1("Warning: no pfnDdDestroyMoComp\n");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT("Calling dxg.sys pfnDdDestroyMoComp\n");
    return pfnDdDestroyMoComp(hMoComp, puBeginFrameData);
}

/************************************************************************/
/* NtGdiDdEndMoCompFrame                                                */
/************************************************************************/
DWORD
APIENTRY
NtGdiDdEndMoCompFrame(HANDLE hMoComp,
                      PDD_ENDMOCOMPFRAMEDATA puEndFrameData)
{
    PGD_DXDDENDMOCOMPFRAME pfnDdEndMoCompFrame = (PGD_DXDDENDMOCOMPFRAME)gpDxFuncs[DXG_INDEX_DxDdEndMoCompFrame].pfn;

    if (pfnDdEndMoCompFrame == NULL)
    {
        DPRINT1("Warning: no pfnDdEndMoCompFrame\n");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT("Calling dxg.sys pfnDdEndMoCompFrame\n");
    return pfnDdEndMoCompFrame(hMoComp, puEndFrameData);
}

/************************************************************************/
/* NtGdiDdGetInternalMoCompInfo                                         */
/************************************************************************/
DWORD
APIENTRY
NtGdiDdGetInternalMoCompInfo(HANDLE hDirectDraw,
                             PDD_GETINTERNALMOCOMPDATA puGetInternalData)
{
    PGD_DXDDGETINTERNALMOCOMPINFO pfnDdGetInternalMoCompInfo = (PGD_DXDDGETINTERNALMOCOMPINFO)gpDxFuncs[DXG_INDEX_DxDdGetInternalMoCompInfo].pfn;

    if (pfnDdGetInternalMoCompInfo == NULL)
    {
        DPRINT1("Warning: no pfnDdGetInternalMoCompInfo\n");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT("Calling dxg.sys pfnDdGetInternalMoCompInfo\n");
    return pfnDdGetInternalMoCompInfo(hDirectDraw, puGetInternalData);
}

/************************************************************************/
/* NtGdiDdGetMoCompBuffInfo                                             */
/************************************************************************/
DWORD
APIENTRY
NtGdiDdGetMoCompBuffInfo(HANDLE hDirectDraw,
                         PDD_GETMOCOMPCOMPBUFFDATA puGetBuffData)
{
    PGD_DXDDGETMOCOMPBUFFINFO pfnDdGetMoCompBuffInfo = (PGD_DXDDGETMOCOMPBUFFINFO)gpDxFuncs[DXG_INDEX_DxDdGetMoCompBuffInfo].pfn;

    if (pfnDdGetMoCompBuffInfo == NULL)
    {
        DPRINT1("Warning: no pfnDdGetMoCompBuffInfo\n");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT("Calling dxg.sys pfnDdGetMoCompBuffInfo\n");
    return pfnDdGetMoCompBuffInfo(hDirectDraw, puGetBuffData);
}

/************************************************************************/
/* NtGdiDdGetMoCompFormats                                              */
/************************************************************************/
DWORD
APIENTRY
NtGdiDdGetMoCompFormats(HANDLE hDirectDraw,
                        PDD_GETMOCOMPFORMATSDATA puGetMoCompFormatsData)
{
    PGD_DXDDGETMOCOMPFORMATS pfnDdGetMoCompFormats = (PGD_DXDDGETMOCOMPFORMATS)gpDxFuncs[DXG_INDEX_DxDdGetMoCompFormats].pfn;

    if (pfnDdGetMoCompFormats == NULL)
    {
        DPRINT1("Warning: no pfnDdGetMoCompFormats\n");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT("Calling dxg.sys pfnDdGetMoCompFormats\n");
    return pfnDdGetMoCompFormats(hDirectDraw, puGetMoCompFormatsData);
}

/************************************************************************/
/* NtGdiDdGetMoCompGuids                                                */
/************************************************************************/
DWORD
APIENTRY
NtGdiDdGetMoCompGuids(HANDLE hDirectDraw,
                      PDD_GETMOCOMPGUIDSDATA puGetMoCompGuidsData)
{
    PGD_DXDDGETMOCOMPGUIDS pfnDdGetMoCompGuids = (PGD_DXDDGETMOCOMPGUIDS)gpDxFuncs[DXG_INDEX_DxDdGetMoCompGuids].pfn;

    if (pfnDdGetMoCompGuids == NULL)
    {
        DPRINT1("Warning: no pfnDdGetMoCompGuids\n");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT("Calling dxg.sys pfnDdGetMoCompGuids\n");
    return pfnDdGetMoCompGuids(hDirectDraw, puGetMoCompGuidsData);
}

/************************************************************************/
/* NtGdiDdQueryMoCompStatus                                             */
/************************************************************************/
DWORD
APIENTRY
NtGdiDdQueryMoCompStatus(HANDLE hMoComp,
                         PDD_QUERYMOCOMPSTATUSDATA puQueryMoCompStatusData)
{
    PGD_DXDDQUERYMOCOMPSTATUS pfnDdQueryMoCompStatus = (PGD_DXDDQUERYMOCOMPSTATUS)gpDxFuncs[DXG_INDEX_DxDdQueryMoCompStatus].pfn;

    if (pfnDdQueryMoCompStatus == NULL)
    {
        DPRINT1("Warning: no pfnDdQueryMoCompStatus\n");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT("Calling dxg.sys pfnDdQueryMoCompStatus\n");
    return pfnDdQueryMoCompStatus(hMoComp, puQueryMoCompStatusData);
}

/************************************************************************/
/* NtGdiDdRenderMoComp                                                  */
/************************************************************************/
DWORD
APIENTRY
NtGdiDdRenderMoComp(HANDLE hMoComp,
                    PDD_RENDERMOCOMPDATA puRenderMoCompData)
{
    PGD_DXDDRENDERMOCOMP pfnDdRenderMoComp = (PGD_DXDDRENDERMOCOMP)gpDxFuncs[DXG_INDEX_DxDdRenderMoComp].pfn;

    if (pfnDdRenderMoComp == NULL)
    {
        DPRINT1("Warning: no pfnDdRenderMoComp\n");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT("Calling dxg.sys pfnDdRenderMoComp\n");
    return pfnDdRenderMoComp(hMoComp, puRenderMoCompData);
}
