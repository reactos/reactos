/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Native DirectDraw implementation
 * FILE:             subsystems/win32/win32k/ntddraw/mocomp.c
 * PROGRAMER:        Magnus Olsen (magnus@greatlord.com)
 * REVISION HISTORY:
 *       19/1-2006   Magnus Olsen
 */


#include <win32k.h>
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
        DPRINT1("Warning: no pfnDdBeginMoCompFrame");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling dxg.sys DdBeginMoCompFrame");
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
        DPRINT1("Warning: no pfnDdCreateMoComp");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling dxg.sys DdCreateMoComp");
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
    PGD_DXDDDESTROYMOCOMP pfnDxDdDestroyMoComp = (PGD_DXDDDESTROYMOCOMP)gpDxFuncs[DXG_INDEX_DxDdDestroyMoComp].pfn;

    if (pfnDxDdDestroyMoComp == NULL)
    {
        DPRINT1("Warning: no pfnDxDdDestroyMoComp");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling dxg.sys DxDdDestroyMoComp");
    return pfnDxDdDestroyMoComp(hMoComp, puBeginFrameData);
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
        DPRINT1("Warning: no pfnDdEndMoCompFrame");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling dxg.sys pfnDdEndMoCompFrame");
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
        DPRINT1("Warning: no pfnDdGetInternalMoCompInfo");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling dxg.sys pfnDdGetInternalMoCompInfo");
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
        DPRINT1("Warning: no pfnDdGetMoCompBuffInfo");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling dxg.sys pfnDdGetMoCompBuffInfo");
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
        DPRINT1("Warning: no pfnDdGetMoCompFormats");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling dxg.sys pfnDdGetMoCompFormats");
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
        DPRINT1("Warning: no pfnDdGetMoCompGuids");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling dxg.sys pfnDdGetMoCompGuids");
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
        DPRINT1("Warning: no pfnDdQueryMoCompStatus");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling dxg.sys pfnDdQueryMoCompStatus");
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
        DPRINT1("Warning: no pfnDdRenderMoComp");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling dxg.sys pfnDdRenderMoComp");
    return pfnDdRenderMoComp(hMoComp, puRenderMoCompData);
}

