/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS ReactX
 * FILE:            dll/directx/d3d9/d3d9_swapchain.h
 * PURPOSE:         Direct3D9's swap chain object
 * PROGRAMERS:      Gregor Gullwi <gbrunmar (dot) ros (at) gmail (dot) com>
 */
#ifndef _D3D9_SWAPCHAIN_H_
#define _D3D9_SWAPCHAIN_H_

#include "d3d9_common.h"
#include <d3d9.h>
#include <ddraw.h>
#include "d3d9_baseobject.h"

struct _Direct3DDevice9_INT;

typedef struct _Direct3DSwapChain9_INT
{
/* 0x0000 */    D3D9BaseObject BaseObject;
/* 0x0020 */    struct IDirect3DSwapChain9Vtbl* lpVtbl;
/* 0x0024 */    DWORD dwUnknown0024;
/* 0x0028 */    struct _D3D9Surface* pPrimarySurface;
/* 0x002c */    struct _D3D9Surface* pExtendedPrimarySurface;
/* 0x0030 */    DWORD dwUnknown0030;
/* 0x0034 */    struct _D3D9DriverSurface** ppBackBufferList;
/* 0x0038 */    DWORD dwNumBackBuffers;
/* 0x003c */    DWORD ChainIndex;
/* 0x0040 */    DWORD AdapterGroupIndex;
/* 0x0044 */    D3DPRESENT_PARAMETERS PresentParameters;
/* 0x007c */    DWORD dwUnknown007c;
/* 0x0080 */    DWORD dwUnknown0080[2];         /* Extended format? */
/* 0x0088 */    struct _D3D9Cursor* pCursor;
/* 0x008c */    DWORD dwUnknown008c[4];
/* 0x009c */    struct _D3D9_Unknown6BC* pUnknown6BC;
/* 0x00a0 */    LPDWORD dwUnknown00a0;
/* 0x00a4 */    DWORD dwUnknown00a4;
/* 0x00a8 */    DWORD dwUnknown00a8;
/* 0x00ac */    DWORD dwWidth;
/* 0x00b0 */    DWORD dwHeight;
/* 0x00b4 */    DWORD dwUnknown00b4;
/* 0x00b8 */    DWORD dwUnknown00b8;
/* 0x00bc */    DWORD dwUnknown00bc;
/* 0x00c0 */    DWORD dwWidth2;
/* 0x00c4 */    DWORD dwHeight2;
/* 0x00c8 */    DWORD dwUnknown00c8;
/* 0x00cc */    DWORD dwUnknown00cc;
/* 0x00d0 */    DWORD dwUnknown00d0;
/* 0x00d4 */    DWORD dwUnknown00d4;
/* 0x00d8 */    DWORD dwUnknown00d8;
/* 0x00dc */    DWORD dwUnknown00dc;
/* 0x00e0 */    DWORD dwDevModeScale;
/* 0x00e4 */    DWORD dwUnknown00e4;
/* 0x00e8 */    DWORD dwUnknown00e8;
/* 0x00ec */    DWORD dwUnknown00ec;
/* 0x00f0 */    DWORD dwUnknown00f0[27];

/* 0x015c */    LPVOID pUnknown015c;
/* 0x0160 */    DWORD dwUnknown0160[4];
/* 0x0170 */    HRESULT hResult;

/* 0x0174 */    DWORD dwUnknown0174[26];

/* 0x01DC */    DWORD dwUnknownFlags;   /* 1 = Show frame rate, 2 = Flip without vsync */
/* 0x01E0 */    BOOL bForceRefreshRate;
/* 0x01E4 */    DWORD dwUnknown01dc[4];

/* 0x01f4 */    D3DSWAPEFFECT SwapEffect;
/* 0x01f8 */    DWORD dwUnknown01f8;
/* 0x01fc */    DWORD dwUnknown01fc;
/* 0x0200 */    D3DGAMMARAMP GammaRamp;
} Direct3DSwapChain9_INT, FAR* LPDIRECT3DSWAPCHAIN9_INT;

LPDIRECT3DSWAPCHAIN9_INT IDirect3DSwapChain9ToImpl(LPDIRECT3DSWAPCHAIN9 iface);
Direct3DSwapChain9_INT* CreateDirect3DSwapChain9(enum REF_TYPE RefType, struct _Direct3DDevice9_INT* pBaseDevice, DWORD ChainIndex);

VOID Direct3DSwapChain9_SetDisplayMode(Direct3DSwapChain9_INT* pThisSwapChain, D3DPRESENT_PARAMETERS* pPresentationParameters);
HRESULT Direct3DSwapChain9_Init(Direct3DSwapChain9_INT* pThisSwapChain, D3DPRESENT_PARAMETERS* pPresentationParameters);
HRESULT Direct3DSwapChain9_Reset(Direct3DSwapChain9_INT* pThisSwapChain, D3DPRESENT_PARAMETERS* pPresentationParameters);
VOID Direct3DSwapChain9_GetGammaRamp(Direct3DSwapChain9_INT* pThisSwapChain, D3DGAMMARAMP* pRamp);

#endif // _D3D9_SWAPCHAIN_H_
