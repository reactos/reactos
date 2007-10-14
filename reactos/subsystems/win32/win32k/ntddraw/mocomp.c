/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Native DirectDraw implementation
 * FILE:             subsys/win32k/ntddraw/d3d.c
 * PROGRAMER:        Magnus olsen (magnus@greatlord.com)
 * REVISION HISTORY:
 *       19/1-2006   Magnus Olsen
 */


#include <w32k.h>
#include <reactos/drivers/directx/dxg.h>

//#define NDEBUG
#include <debug.h>

extern PDRVFN gpDxFuncs;

#define DXG_GET_INDEX_FUNCTION(INDEX, FUNCTION) \
    if (gpDxFuncs) \
    { \
        for (i = 0; i <= DXG_INDEX_DxDdIoctl; i++) \
        { \
            if (gpDxFuncs[i].iFunc == INDEX)  \
            { \
                FUNCTION = (VOID *)gpDxFuncs[i].pfn;  \
                break;  \
            }  \
        } \
    }

typedef DWORD (NTAPI *PGD_DDBEGINMOCOMPFRAME)(HANDLE, PDD_BEGINMOCOMPFRAMEDATA);
typedef HANDLE (NTAPI *PGD_DXDDCREATEMOCOMP)(HANDLE, PDD_CREATEMOCOMPDATA );
typedef DWORD (NTAPI *PGD_DXDDDESTROYMOCOMP)(HANDLE, BOOL);
typedef DWORD (NTAPI *PGD_DXDDENDMOCOMPFRAME)(HANDLE, PDD_ENDMOCOMPFRAMEDATA);
typedef DWORD (NTAPI *PGD_DXDDGETINTERNALMOCOMPINFO)(HANDLE, PDD_GETINTERNALMOCOMPDATA);
typedef DWORD (NTAPI *PGD_DXDDGETMOCOMPBUFFINFO)(HANDLE, PDD_GETMOCOMPCOMPBUFFDATA);
typedef DWORD (NTAPI *PGD_DXDDGETMOCOMPGUIDS)(HANDLE, PDD_GETMOCOMPGUIDSDATA);
typedef DWORD (NTAPI *PGD_DXDDGETMOCOMPFORMATS)(HANDLE, PDD_GETMOCOMPFORMATSDATA);

DWORD
STDCALL
NtGdiDdEndMoCompFrame(HANDLE hMoComp, PDD_ENDMOCOMPFRAMEDATA puEndFrameData)
{

}

DWORD
STDCALL
NtGdiDdGetInternalMoCompInfo(HANDLE hDirectDraw,
                             PDD_GETINTERNALMOCOMPDATA puGetInternalData)
{

}

DWORD STDCALL NtGdiDdGetMoCompBuffInfo(
    HANDLE hDirectDraw,
    PDD_GETMOCOMPCOMPBUFFDATA puGetBuffData)
{

}

DWORD
STDCALL
NtGdiDdGetMoCompFormats(HANDLE hDirectDraw,
                        PDD_GETMOCOMPFORMATSDATA puGetMoCompFormatsData)
{

}


DWORD
STDCALL
NtGdiDdGetMoCompGuids(HANDLE hDirectDraw,
                      PDD_GETMOCOMPGUIDSDATA puGetMoCompGuidsData)
{

}

DWORD
STDCALL NtGdiDdQueryMoCompStatus(HANDLE hMoComp,
                                 PDD_QUERYMOCOMPSTATUSDATA puQueryMoCompStatusData)
{

}


DWORD
STDCALL
NtGdiDdRenderMoComp(HANDLE hMoComp,
                    PDD_RENDERMOCOMPDATA puRenderMoCompData)
{

}

