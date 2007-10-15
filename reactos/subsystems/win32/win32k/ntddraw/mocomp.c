/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Native DirectDraw implementation
 * FILE:             subsys/win32k/ntddraw/mocomp.c
 * PROGRAMER:        Magnus olsen (magnus@greatlord.com)
 * REVISION HISTORY:
 *       19/1-2006   Magnus Olsen
 */


#include <w32k.h>
#include <debug.h>

/************************************************************************/
/* NtGdiDdBeginMoCompFrame                                              */
/************************************************************************/
DWORD
STDCALL
NtGdiDdBeginMoCompFrame(HANDLE hMoComp,
                        PDD_BEGINMOCOMPFRAMEDATA puBeginFrameData)
{
    PGD_DDBEGINMOCOMPFRAME pfnDdBeginMoCompFrame = NULL;
    INT i;

    DXG_GET_INDEX_FUNCTION(DXG_INDEX_DxDdBeginMoCompFrame, pfnDdBeginMoCompFrame);

    if (pfnDdBeginMoCompFrame == NULL)
    {
        DPRINT1("Warring no pfnDdBeginMoCompFrame");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling on dxg.sys DdBeginMoCompFrame");
    return pfnDdBeginMoCompFrame(hMoComp,puBeginFrameData);
}

/************************************************************************/
/* NtGdiDdCreateMoComp                                                  */
/************************************************************************/
HANDLE
STDCALL
NtGdiDdCreateMoComp(HANDLE hDirectDraw, PDD_CREATEMOCOMPDATA puCreateMoCompData)
{
    PGD_DXDDCREATEMOCOMP pfnDdCreateMoComp = NULL;
    INT i;

    DXG_GET_INDEX_FUNCTION(DXG_INDEX_DxDdCreateMoComp, pfnDdCreateMoComp);

    if (pfnDdCreateMoComp == NULL)
    {
        DPRINT1("Warring no pfnDdCreateMoComp");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling on dxg.sys DdCreateMoComp");
    return pfnDdCreateMoComp(hDirectDraw, puCreateMoCompData);
}

/************************************************************************/
/* NtGdiDdDestroyMoComp                                                 */
/************************************************************************/
DWORD 
STDCALL
NtGdiDdDestroyMoComp(HANDLE hMoComp,
                     PDD_DESTROYMOCOMPDATA puBeginFrameData)
{
    PGD_DXDDDESTROYMOCOMP pfnDxDdDestroyMoComp = NULL;
    INT i;

    DXG_GET_INDEX_FUNCTION(DXG_INDEX_DxDdDestroyMoComp, pfnDxDdDestroyMoComp);

    if (pfnDxDdDestroyMoComp == NULL)
    {
        DPRINT1("Warring no pfnDxDdDestroyMoComp");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling on dxg.sys DxDdDestroyMoComp");
    return pfnDxDdDestroyMoComp(hMoComp, puBeginFrameData);
}

/************************************************************************/
/* NtGdiDdEndMoCompFrame                                                */
/************************************************************************/
DWORD
STDCALL
NtGdiDdEndMoCompFrame(HANDLE hMoComp,
                      PDD_ENDMOCOMPFRAMEDATA puEndFrameData)
{
    PGD_DXDDENDMOCOMPFRAME pfnDdEndMoCompFrame = NULL;
    INT i;

    DXG_GET_INDEX_FUNCTION(DXG_INDEX_DxDdEndMoCompFrame, pfnDdEndMoCompFrame);

    if (pfnDdEndMoCompFrame == NULL)
    {
        DPRINT1("Warring no pfnDdEndMoCompFrame");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling on dxg.sys pfnDdEndMoCompFrame");
    return pfnDdEndMoCompFrame(hMoComp, puEndFrameData);
}

/************************************************************************/
/* NtGdiDdGetInternalMoCompInfo                                         */
/************************************************************************/
DWORD
STDCALL
NtGdiDdGetInternalMoCompInfo(HANDLE hDirectDraw,
                             PDD_GETINTERNALMOCOMPDATA puGetInternalData)
{
    PGD_DXDDGETINTERNALMOCOMPINFO pfnDdGetInternalMoCompInfo = NULL;
    INT i;

    DXG_GET_INDEX_FUNCTION(DXG_INDEX_DxDdGetInternalMoCompInfo, pfnDdGetInternalMoCompInfo);

    if (pfnDdGetInternalMoCompInfo == NULL)
    {
        DPRINT1("Warring no pfnDdGetInternalMoCompInfo");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling on dxg.sys pfnDdGetInternalMoCompInfo");
    return pfnDdGetInternalMoCompInfo(hDirectDraw, puGetInternalData);
}


/************************************************************************/
/* NtGdiDdGetMoCompBuffInfo                                             */
/************************************************************************/
DWORD
STDCALL
NtGdiDdGetMoCompBuffInfo(HANDLE hDirectDraw,
                         PDD_GETMOCOMPCOMPBUFFDATA puGetBuffData)
{
    PGD_DXDDGETMOCOMPBUFFINFO pfnDdGetMoCompBuffInfo = NULL;
    INT i;

    DXG_GET_INDEX_FUNCTION(DXG_INDEX_DxDdGetMoCompBuffInfo, pfnDdGetMoCompBuffInfo);

    if (pfnDdGetMoCompBuffInfo == NULL)
    {
        DPRINT1("Warring no pfnDdGetMoCompBuffInfo");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling on dxg.sys pfnDdGetMoCompBuffInfo");
    return pfnDdGetMoCompBuffInfo(hDirectDraw, puGetBuffData);
}

/************************************************************************/
/* NtGdiDdGetMoCompFormats                                              */
/************************************************************************/
DWORD
STDCALL
NtGdiDdGetMoCompFormats(HANDLE hDirectDraw,
                        PDD_GETMOCOMPFORMATSDATA puGetMoCompFormatsData)
{
    PGD_DXDDGETMOCOMPFORMATS pfnDdGetMoCompFormats = NULL;
    INT i;

    DXG_GET_INDEX_FUNCTION(DXG_INDEX_DxDdGetMoCompFormats, pfnDdGetMoCompFormats);

    if (pfnDdGetMoCompFormats == NULL)
    {
        DPRINT1("Warring no pfnDdGetMoCompFormats");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling on dxg.sys pfnDdGetMoCompFormats");
    return pfnDdGetMoCompFormats(hDirectDraw, puGetMoCompFormatsData);
}


/************************************************************************/
/* NtGdiDdGetMoCompGuids                                                */
/************************************************************************/
DWORD
STDCALL
NtGdiDdGetMoCompGuids(HANDLE hDirectDraw,
                      PDD_GETMOCOMPGUIDSDATA puGetMoCompGuidsData)
{
    PGD_DXDDGETMOCOMPGUIDS pfnDdGetMoCompGuids = NULL;
    INT i;

    DXG_GET_INDEX_FUNCTION(DXG_INDEX_DxDdGetMoCompGuids, pfnDdGetMoCompGuids);

    if (pfnDdGetMoCompGuids == NULL)
    {
        DPRINT1("Warring no pfnDdGetMoCompGuids");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling on dxg.sys pfnDdGetMoCompGuids");
    return pfnDdGetMoCompGuids(hDirectDraw, puGetMoCompGuidsData);
}



/************************************************************************/
/* NtGdiDdQueryMoCompStatus                                             */
/************************************************************************/
DWORD
STDCALL
NtGdiDdQueryMoCompStatus(HANDLE hMoComp,
                         PDD_QUERYMOCOMPSTATUSDATA puQueryMoCompStatusData)
{
    PGD_DXDDQUERYMOCOMPSTATUS pfnDdQueryMoCompStatus = NULL;
    INT i;

    DXG_GET_INDEX_FUNCTION(DXG_INDEX_DxDdQueryMoCompStatus, pfnDdQueryMoCompStatus);

    if (pfnDdQueryMoCompStatus == NULL)
    {
        DPRINT1("Warring no pfnDdQueryMoCompStatus");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling on dxg.sys pfnDdQueryMoCompStatus");
    return pfnDdQueryMoCompStatus(hMoComp, puQueryMoCompStatusData);
}


/************************************************************************/
/* NtGdiDdRenderMoComp                                                  */
/************************************************************************/
DWORD
STDCALL
NtGdiDdRenderMoComp(HANDLE hMoComp,
                    PDD_RENDERMOCOMPDATA puRenderMoCompData)
{
    PGD_DXDDRENDERMOCOMP pfnDdRenderMoComp = NULL;
    INT i;

    DXG_GET_INDEX_FUNCTION(DXG_INDEX_DxDdRenderMoComp, pfnDdRenderMoComp);

    if (pfnDdRenderMoComp == NULL)
    {
        DPRINT1("Warring no pfnDdRenderMoComp");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling on dxg.sys pfnDdRenderMoComp");
    return pfnDdRenderMoComp(hMoComp, puRenderMoCompData);
}



