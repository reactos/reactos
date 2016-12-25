/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for NtGdiDdCreateDirectDrawObject
 * PROGRAMMERS:
 */

#include <win32nt.h>

/* Note : OsThunkDdQueryDirectDrawObject is the usermode name of NtGdiDdQueryDirectDrawObject
 *        it lives in d3d8thk.dll and in windows 2000 it doing syscall direcly to win32k.sus
 *        in windows xp and higher it call to gdi32.dll to DdEntry41 and it doing the syscall
 */
START_TEST(NtGdiDdQueryDirectDrawObject)
{
    HANDLE  hDirectDraw = NULL;
    DD_HALINFO *pHalInfo = NULL;
    DWORD *pCallBackFlags = NULL;
    LPD3DNTHAL_CALLBACKS puD3dCallbacks = NULL;
    LPD3DNTHAL_GLOBALDRIVERDATA puD3dDriverData = NULL;
    PDD_D3DBUFCALLBACKS puD3dBufferCallbacks = NULL;
    LPDDSURFACEDESC puD3dTextureFormats = NULL;
    DWORD *puNumHeaps = NULL;
    VIDEOMEMORY *puvmList = NULL;
    DWORD *puNumFourCC = NULL;
    DWORD *puFourCC = NULL;

    DD_HALINFO HalInfo;
    DD_HALINFO oldHalInfo;
    DWORD CallBackFlags[4];

    D3DNTHAL_CALLBACKS D3dCallbacks;
    D3DNTHAL_CALLBACKS oldD3dCallbacks;

    D3DNTHAL_GLOBALDRIVERDATA D3dDriverData;
    D3DNTHAL_GLOBALDRIVERDATA oldD3dDriverData;

    DD_D3DBUFCALLBACKS D3dBufferCallbacks;
    DD_D3DBUFCALLBACKS oldD3dBufferCallbacks;

    DDSURFACEDESC2 D3dTextureFormats[100];
    DWORD NumHeaps = 0;
    VIDEOMEMORY vmList;
    //DWORD NumFourCC = 0;
    //DWORD FourCC = 0;
    DEVMODE devmode;
    HDC hdc;

    DWORD dwTextureCounter = 0;
    DDSURFACEDESC *myDesc = NULL;

    /* clear data */
    memset(&vmList,0,sizeof(VIDEOMEMORY));
    memset(&D3dTextureFormats,0,sizeof(DDSURFACEDESC));
    memset(&D3dBufferCallbacks,0,sizeof(DD_D3DBUFCALLBACKS));
    memset(&D3dDriverData,0,sizeof(D3DNTHAL_GLOBALDRIVERDATA));
    memset(&D3dCallbacks,0,sizeof(D3DNTHAL_CALLBACKS));
    memset(&HalInfo,0,sizeof(DD_HALINFO));
    memset(CallBackFlags,0,sizeof(DWORD)*3);

    /* Get current display mode */
    EnumDisplaySettingsA(NULL, ENUM_CURRENT_SETTINGS, &devmode);

    /* Create hdc that we can use */
    hdc = CreateDCW(L"DISPLAY",NULL,NULL,NULL);
    ASSERT(hdc != NULL);


    /* Create ReactX handle */
    hDirectDraw = (HANDLE) NtGdiDdCreateDirectDrawObject(hdc);
    ASSERT(hDirectDraw != NULL);

    /* Start Test ReactX NtGdiDdQueryDirectDrawObject function */

    /* testing  OsThunkDdQueryDirectDrawObject( NULL, ....  */
    RTEST(NtGdiDdQueryDirectDrawObject( NULL, pHalInfo,
                                        pCallBackFlags, puD3dCallbacks,
                                        puD3dDriverData, puD3dBufferCallbacks,
                                        puD3dTextureFormats, puNumHeaps,
                                        puvmList, puNumFourCC,
                                        puFourCC) == FALSE);

    RTEST(pHalInfo == NULL);
    RTEST(pCallBackFlags == NULL);
    RTEST(puD3dCallbacks == NULL);
    RTEST(puD3dDriverData == NULL);
    RTEST(puD3dBufferCallbacks == NULL);
    RTEST(puD3dTextureFormats == NULL);
    RTEST(puNumFourCC == NULL);
    RTEST(puFourCC == NULL);
    RTEST(puNumHeaps == NULL);
    RTEST(puvmList == NULL);

    /* testing  NtGdiDdQueryDirectDrawObject( hDirectDrawLocal, NULL, ....  */
    RTEST(NtGdiDdQueryDirectDrawObject( hDirectDraw, pHalInfo,
                                        pCallBackFlags, puD3dCallbacks,
                                        puD3dDriverData, puD3dBufferCallbacks,
                                        puD3dTextureFormats, puNumHeaps,
                                        puvmList, puNumFourCC,
                                        puFourCC) == FALSE);

    RTEST(pHalInfo == NULL);
    RTEST(pCallBackFlags == NULL);
    RTEST(puD3dCallbacks == NULL);
    RTEST(puD3dDriverData == NULL);
    RTEST(puD3dBufferCallbacks == NULL);
    RTEST(puD3dTextureFormats == NULL);
    RTEST(puNumFourCC == NULL);
    RTEST(puFourCC == NULL);
    RTEST(puNumHeaps == NULL);
    RTEST(puvmList == NULL);

    /* testing  NtGdiDdQueryDirectDrawObject( hDirectDrawLocal, pHalInfo, NULL, ....  */
    pHalInfo = &HalInfo;
    RTEST(NtGdiDdQueryDirectDrawObject( hDirectDraw, pHalInfo,
                                        pCallBackFlags, puD3dCallbacks,
                                        puD3dDriverData, puD3dBufferCallbacks,
                                        puD3dTextureFormats, puNumHeaps,
                                        puvmList, puNumFourCC,
                                        puFourCC)== FALSE);

    RTEST(pHalInfo != NULL);
    ASSERT(pHalInfo != NULL);

    RTEST(pCallBackFlags == NULL);
    RTEST(puD3dCallbacks == NULL);
    RTEST(puD3dDriverData == NULL);
    RTEST(puD3dBufferCallbacks == NULL);
    RTEST(puD3dTextureFormats == NULL);
    RTEST(puNumFourCC == NULL);
    RTEST(puFourCC == NULL);
    RTEST(puNumHeaps == NULL);
    RTEST(puvmList == NULL);

    if ((pHalInfo->dwSize != sizeof(DD_HALINFO)) &&
        (pHalInfo->dwSize != sizeof(DD_HALINFO_V4)))
    {
        RTEST(pHalInfo->dwSize != sizeof(DD_HALINFO));
        ASSERT(pHalInfo->dwSize != sizeof(DD_HALINFO));
    }

    if (pHalInfo->dwSize == sizeof(DD_HALINFO))
    {
        /*the offset, in bytes, to primary surface in the display memory  */
        /* some graphic card like  sis 760 GX, Nvida GF7900GS does not set any offset at all */
        // RTEST(pHalInfo->vmiData.fpPrimary != 0 );

        /* unsuse always 0 */
        RTEST(pHalInfo->vmiData.dwFlags == 0 );

        /* Check the res */
        RTEST(pHalInfo->vmiData.dwDisplayWidth == devmode.dmPelsWidth );
        RTEST(pHalInfo->vmiData.dwDisplayHeight == devmode.dmPelsHeight );

        /*  This can never be test for it is who big the line is after it been align displayPitch */
        RTEST(pHalInfo->vmiData.lDisplayPitch != 0);

        RTEST(pHalInfo->vmiData.ddpfDisplay.dwSize == sizeof(DDPIXELFORMAT) );
        ASSERT(pHalInfo->vmiData.ddpfDisplay.dwSize == sizeof(DDPIXELFORMAT));

        /* We can not check if it DDPF_RGB flags been set for primary surface
         * for it can be DDPF_PALETTEINDEXED1,DDPF_PALETTEINDEXED2,DDPF_PALETTEINDEXED4,DDPF_PALETTEINDEXED8, DDPF_PALETTEINDEXEDTO8, DDPF_RGB, DDPF_YUV
         */
        RTEST( (pHalInfo->vmiData.ddpfDisplay.dwFlags & (DDPF_PALETTEINDEXED1 | DDPF_PALETTEINDEXED2 | DDPF_PALETTEINDEXED4 |
                                                         DDPF_PALETTEINDEXED8 | DDPF_PALETTEINDEXEDTO8 | DDPF_RGB | DDPF_YUV)) != 0);


        /* No fourcc are use on primary screen */
        RTEST(pHalInfo->vmiData.ddpfDisplay.dwFourCC == 0 );

        /* Count RGB Bits 8/16/24/32 */
        RTEST(pHalInfo->vmiData.ddpfDisplay.dwRGBBitCount == devmode.dmBitsPerPel );

        /* The rgb mask can not be detected in user mode, for it can be 15Bpp convert to 16Bpp mode, so we have no way detect correct mask
         * But the mask can never be Zero
         */
        RTEST(pHalInfo->vmiData.ddpfDisplay.dwRBitMask  !=  0 );
        RTEST(pHalInfo->vmiData.ddpfDisplay.dwGBitMask !=  0 );
        RTEST(pHalInfo->vmiData.ddpfDisplay.dwBBitMask != 0 );

        /* primary never set the alpha blend mask */
        RTEST(pHalInfo->vmiData.ddpfDisplay.dwRGBAlphaBitMask ==  0 );

        /* This can not be test at usermode it is each hardware drv that fill in it,
         * only way to found them is to use this call  */
        // pHalInfo->vmiData->dwOffscreenAlign
        // pHalInfo->vmiData->dwOverlayAlign
        // pHalInfo->vmiData->dwTextureAlign
        // pHalInfo->vmiData->dwZBufferAlign
        // pHalInfo->vmiData->dwAlphaAlign

        /* the primary display address */
        RTEST( ( (DWORD)pHalInfo->vmiData.pvPrimary & (~0x80000000)) != 0 );

        /* test see if we got back the pvmList here
         * acording msdn vmiData.dwNumHeaps and vmiData.pvmList
         * exists for _VIDEOMEMORYINFO but they do not, it reviews
         * in ddk and wdk and own testcase
         */
         // RTEST(pHalInfo->vmiData.dwNumHeaps  != 0 );
         // RTEST(pHalInfo->vmiData.pvmList  != 0 );

        /* Test see if we got any hardware acclartions for 2d or 3d, this always fill in
         * that mean we found a bugi drv and dx does not work on this drv
         */

        /* the SIS 760 GX will never fill it in, it is a bugi drv */
        RTEST(pHalInfo->ddCaps.dwSize == sizeof(DDCORECAPS));

        /* Testing see if we got any hw support for
         * This test can fail on video card that does not support 2d/overlay/3d
         */
        RTEST( pHalInfo->ddCaps.dwCaps != 0);
        RTEST( pHalInfo->ddCaps.ddsCaps.dwCaps != 0);

        /* This flags is obsolete and should not be used by the driver */
        RTEST( pHalInfo->ddCaps.dwFXAlphaCaps == 0);


        /* basic dx 2 is found if this flags not set
         * if this fail we do not have a dx driver install acodring ms, some version of windows it
         * is okay this fail and drv does then only support basic dx
         *
         */
        if (pHalInfo->dwFlags != 0)
        {
            RTEST( (pHalInfo->dwFlags & (DDHALINFO_GETDRIVERINFOSET | DDHALINFO_GETDRIVERINFO2)) != 0 );
            RTEST( ( (DWORD)pHalInfo->GetDriverInfo & 0x80000000) != 0 );
            ASSERT( ((DWORD)pHalInfo->GetDriverInfo & 0x80000000) != 0 );
        }

        /* point to kmode direcly to the graphic drv, the drv is kmode and it is kmode address we getting back*/


       /* the pHalInfo->ddCaps.ddsCaps.dwCaps & DDSCAPS_3DDEVICE will be ignore, only way detect it proper follow code,
        * this will be fill in of all drv, it is not only for 3d stuff, this always fill by win32k.sys or dxg.sys depns
        * if it windows 2000 or windows xp/2003
        *
        * point to kmode direcly to the win32k.sys, win32k.sys is kmode and it is kmode address we getting back
        */
        RTEST( ( (DWORD)pHalInfo->lpD3DGlobalDriverData & (~0x80000000)) != 0 );
        RTEST( ( (DWORD)pHalInfo->lpD3DHALCallbacks & (~0x80000000)) != 0 );
        RTEST( ( (DWORD)pHalInfo->lpD3DHALCallbacks & (~0x80000000)) != 0 );
    }

    /* Backup DD_HALINFO so we do not need resting it */
    RtlCopyMemory(&oldHalInfo, &HalInfo, sizeof(DD_HALINFO));

    /* testing  NtGdiDdQueryDirectDrawObject( hDirectDrawLocal, pHalInfo, pCallBackFlags, NULL, ....  */
    pHalInfo = &HalInfo;
    pCallBackFlags = CallBackFlags;
    RtlZeroMemory(pHalInfo,sizeof(DD_HALINFO));

    RTEST(NtGdiDdQueryDirectDrawObject( hDirectDraw, pHalInfo,
                                        pCallBackFlags, puD3dCallbacks,
                                        puD3dDriverData, puD3dBufferCallbacks,
                                        puD3dTextureFormats, puNumHeaps,
                                        puvmList, puNumFourCC,
                                        puFourCC)== FALSE);
    RTEST(pHalInfo != NULL);
    ASSERT(pHalInfo != NULL);

    RTEST(pCallBackFlags != NULL);
    ASSERT(pCallBackFlags != NULL);

    RTEST(puD3dCallbacks == NULL);
    RTEST(puD3dDriverData == NULL);
    RTEST(puD3dBufferCallbacks == NULL);
    RTEST(puD3dTextureFormats == NULL);
    RTEST(puNumFourCC == NULL);
    RTEST(puFourCC == NULL);
    RTEST(puNumHeaps == NULL);
    RTEST(puvmList == NULL);

    /* We do not retesting DD_HALINFO, instead we compare it */
    RTEST(memcmp(&oldHalInfo, pHalInfo, sizeof(DD_HALINFO)) == 0);

    /* Rember on some nivida drv the pCallBackFlags will not be set even they api exists in the drv
     * known workaround is to check if the drv really return a kmode pointer for the drv functions
     * we want to use.
     */
    RTEST(pCallBackFlags[0] != 0);
    RTEST(pCallBackFlags[1] != 0);
    RTEST(pCallBackFlags[2] == 0);

    /* testing  NtGdiDdQueryDirectDrawObject( hDirectDrawLocal, pHalInfo, pCallBackFlags, D3dCallbacks, NULL, ....  */
    pHalInfo = &HalInfo;
    pCallBackFlags = CallBackFlags;
    puD3dCallbacks = &D3dCallbacks;

    RtlZeroMemory(pHalInfo,sizeof(DD_HALINFO));
    RtlZeroMemory(pCallBackFlags,sizeof(DWORD)*3);

    RTEST(NtGdiDdQueryDirectDrawObject( hDirectDraw, pHalInfo,
                                        pCallBackFlags, puD3dCallbacks,
                                        puD3dDriverData, puD3dBufferCallbacks,
                                        puD3dTextureFormats, puNumHeaps,
                                        puvmList, puNumFourCC,
                                        puFourCC)== FALSE);
    RTEST(pHalInfo != NULL);
    ASSERT(pHalInfo != NULL);

    RTEST(pCallBackFlags != NULL);
    ASSERT(pCallBackFlags != NULL);

    /* rember puD3dCallbacks shall never return NULL */
    RTEST(puD3dCallbacks != NULL);
    ASSERT(puD3dCallbacks != NULL);

    /* the pHalInfo->ddCaps.ddsCaps.dwCaps & DDSCAPS_3DDEVICE will be ignore, only way detect it proper follow code,
     * this will be fill in of all drv, it is not only for 3d stuff, this always fill by win32k.sys or dxg.sys depns
     * if it windows 2000 or windows xp/2003
     */
    RTEST(puD3dCallbacks->dwSize == sizeof(D3DNTHAL_CALLBACKS));

    /* Nivda like GF7900GS will not follow ms design rule here,
     * ContextDestroyAll must alwyas be NULL for it is not longer inuse in windows 2000 and higher
     */
    RTEST(puD3dCallbacks->ContextDestroyAll == NULL);

    /* Nivda like GF7900GS will not follow ms design rule here,
     * SceneCapture must alwyas be NULL for it is not longer inuse in windows 2000 and higher
     */
    RTEST(puD3dCallbacks->SceneCapture  == NULL);
    RTEST(puD3dCallbacks->dwReserved10 == 0);
    RTEST(puD3dCallbacks->dwReserved11 == 0);
    RTEST(puD3dCallbacks->dwReserved22 == 0);
    RTEST(puD3dCallbacks->dwReserved23 == 0);
    RTEST(puD3dCallbacks->dwReserved == 0);
    RTEST(puD3dCallbacks->TextureCreate  == NULL);
    RTEST(puD3dCallbacks->TextureDestroy  == NULL);
    RTEST(puD3dCallbacks->TextureSwap  == NULL);
    RTEST(puD3dCallbacks->TextureGetSurf  == NULL);
    RTEST(puD3dCallbacks->dwReserved12 == 0);
    RTEST(puD3dCallbacks->dwReserved13 == 0);
    RTEST(puD3dCallbacks->dwReserved14 == 0);
    RTEST(puD3dCallbacks->dwReserved15 == 0);
    RTEST(puD3dCallbacks->dwReserved16 == 0);
    RTEST(puD3dCallbacks->dwReserved17 == 0);
    RTEST(puD3dCallbacks->dwReserved18 == 0);
    RTEST(puD3dCallbacks->dwReserved19 == 0);
    RTEST(puD3dCallbacks->dwReserved20 == 0);
    RTEST(puD3dCallbacks->dwReserved21 == 0);
    RTEST(puD3dCallbacks->dwReserved24 == 0);
    RTEST(puD3dCallbacks->dwReserved0 == 0);
    RTEST(puD3dCallbacks->dwReserved1 == 0);
    RTEST(puD3dCallbacks->dwReserved2 == 0);
    RTEST(puD3dCallbacks->dwReserved3 == 0);
    RTEST(puD3dCallbacks->dwReserved4 == 0);
    RTEST(puD3dCallbacks->dwReserved5 == 0);
    RTEST(puD3dCallbacks->dwReserved6 == 0);
    RTEST(puD3dCallbacks->dwReserved7 == 0);
    RTEST(puD3dCallbacks->dwReserved8 == 0);
    RTEST(puD3dCallbacks->dwReserved9 == 0);

    /* how detect puD3dCallbacks->ContextCreate and puD3dCallbacks->ContextDestroy shall be set for bugi drv like nivda ? */
    /* pointer direcly to the graphic drv, it is kmode pointer */
    // RTEST( ( (DWORD)puD3dCallbacks->ContextCreate & (~0x80000000)) != 0 );
    // RTEST( ( (DWORD)puD3dCallbacks->ContextDestroy & (~0x80000000)) != 0 );

    RTEST(puD3dDriverData == NULL);
    RTEST(puD3dBufferCallbacks == NULL);
    RTEST(puD3dTextureFormats == NULL);
    RTEST(puNumFourCC == NULL);
    RTEST(puFourCC == NULL);
    RTEST(puNumHeaps == NULL);
    RTEST(puvmList == NULL);

    /* We do not retesting DD_HALINFO, instead we compare it */
    RTEST(memcmp(&oldHalInfo, pHalInfo, sizeof(DD_HALINFO)) == 0);
    RTEST(pCallBackFlags[0] != 0);
    RTEST(pCallBackFlags[1] != 0);
    RTEST(pCallBackFlags[2] == 0);

    /* Backup D3DNTHAL_CALLBACKS so we do not need resting it */
    RtlCopyMemory(&oldD3dCallbacks, &D3dCallbacks, sizeof(D3DNTHAL_CALLBACKS));


/* testing  NtGdiDdQueryDirectDrawObject( hDD, pHalInfo, pCallBackFlags, puD3dCallbacks, puD3dDriverData, NULL, */
    pHalInfo = &HalInfo;
    pCallBackFlags = CallBackFlags;
    puD3dCallbacks = &D3dCallbacks;
    puD3dDriverData = &D3dDriverData;

    RtlZeroMemory(pHalInfo,sizeof(DD_HALINFO));
    RtlZeroMemory(pCallBackFlags,sizeof(DWORD)*3);
    RtlZeroMemory(puD3dCallbacks,sizeof(D3DNTHAL_CALLBACKS));

    RTEST(NtGdiDdQueryDirectDrawObject( hDirectDraw, pHalInfo,
                                        pCallBackFlags, puD3dCallbacks,
                                        puD3dDriverData, puD3dBufferCallbacks,
                                        puD3dTextureFormats, puNumHeaps,
                                        puvmList, puNumFourCC,
                                        puFourCC)== FALSE);
    RTEST(pHalInfo != NULL);
    ASSERT(pHalInfo != NULL);

    RTEST(pCallBackFlags != NULL);
    ASSERT(pCallBackFlags != NULL);

    RTEST(puD3dCallbacks != NULL);
    ASSERT(puD3dCallbacks != NULL);

    RTEST(puD3dDriverData != NULL);
    ASSERT(puD3dDriverData != NULL);

    RTEST(puD3dBufferCallbacks == NULL);
    RTEST(puD3dTextureFormats == NULL);
    RTEST(puNumFourCC == NULL);
    RTEST(puFourCC == NULL);
    RTEST(puNumHeaps == NULL);
    RTEST(puvmList == NULL);

    /* We retesting pCallBackFlags  */
    RTEST(pCallBackFlags[0] != 0);
    RTEST(pCallBackFlags[1] != 0);
    RTEST(pCallBackFlags[2] == 0);

    /* We do not retesting instead we compare it */
    RTEST(memcmp(&oldHalInfo, pHalInfo, sizeof(DD_HALINFO)) == 0);
    RTEST(memcmp(&oldD3dCallbacks, puD3dCallbacks, sizeof(D3DNTHAL_CALLBACKS)) == 0);

    /* start test of puD3dDriverData */

    RTEST(puD3dDriverData->dwSize == sizeof(D3DNTHAL_GLOBALDRIVERDATA));
    RTEST(puD3dDriverData->hwCaps.dwSize == sizeof(D3DNTHALDEVICEDESC_V1));
    RTEST(puD3dDriverData->hwCaps.dtcTransformCaps.dwSize == sizeof(D3DTRANSFORMCAPS));
    RTEST(puD3dDriverData->hwCaps.dlcLightingCaps.dwSize == sizeof(D3DLIGHTINGCAPS));
    RTEST(puD3dDriverData->hwCaps.dpcLineCaps.dwSize == sizeof(D3DPRIMCAPS));
    RTEST(puD3dDriverData->hwCaps.dpcTriCaps.dwSize  == sizeof(D3DPRIMCAPS));
    RTEST(puD3dDriverData->hwCaps.dwMaxBufferSize == 0);
    RTEST(puD3dDriverData->hwCaps.dwMaxVertexCount == 0);

    /* Backup D3DHAL_GLOBALDRIVERDATA so we do not need resting it */
    RtlCopyMemory(&oldD3dDriverData, &D3dDriverData, sizeof(D3DNTHAL_GLOBALDRIVERDATA));

/* testing  NtGdiDdQueryDirectDrawObject( hDD, pHalInfo, pCallBackFlags, puD3dCallbacks, puD3dDriverData, puD3dBufferCallbacks, NULL, */
    pHalInfo = &HalInfo;
    pCallBackFlags = CallBackFlags;
    puD3dCallbacks = &D3dCallbacks;
    puD3dDriverData = &D3dDriverData;
    puD3dBufferCallbacks = &D3dBufferCallbacks;

    RtlZeroMemory(pHalInfo,sizeof(DD_HALINFO));
    RtlZeroMemory(pCallBackFlags,sizeof(DWORD)*3);
    RtlZeroMemory(puD3dCallbacks,sizeof(D3DNTHAL_CALLBACKS));
    RtlZeroMemory(puD3dDriverData,sizeof(D3DNTHAL_CALLBACKS));

    RTEST(NtGdiDdQueryDirectDrawObject( hDirectDraw, pHalInfo,
                                        pCallBackFlags, puD3dCallbacks,
                                        puD3dDriverData, puD3dBufferCallbacks,
                                        puD3dTextureFormats, puNumHeaps,
                                        puvmList, puNumFourCC,
                                        puFourCC)== FALSE);
    RTEST(pHalInfo != NULL);
    RTEST(pCallBackFlags != NULL);

    if (pHalInfo->ddCaps.ddsCaps.dwCaps  & DDSCAPS_3DDEVICE )
    {
        RTEST(puD3dCallbacks != NULL);
        RTEST(puD3dDriverData != NULL);
        RTEST(puD3dBufferCallbacks != NULL);
    }

    RTEST(pHalInfo != NULL);
    ASSERT(pHalInfo != NULL);

    RTEST(pCallBackFlags != NULL);
    ASSERT(pCallBackFlags != NULL);

    RTEST(puD3dCallbacks != NULL);
    ASSERT(puD3dCallbacks != NULL);

    RTEST(puD3dDriverData != NULL);
    ASSERT(puD3dDriverData != NULL);

    RTEST(puD3dBufferCallbacks != NULL);
    ASSERT(puD3dDriverData != NULL);

    RTEST(puD3dTextureFormats == NULL);
    RTEST(puNumFourCC == NULL);
    RTEST(puFourCC == NULL);
    RTEST(puNumHeaps == NULL);
    RTEST(puvmList == NULL);

    /* We retesting the flags */
    RTEST(pCallBackFlags[0] != 0);
    RTEST(pCallBackFlags[1] != 0);
    RTEST(pCallBackFlags[2] == 0);

    /* We do not retesting instead we compare it */
    RTEST(memcmp(&oldHalInfo, pHalInfo, sizeof(DD_HALINFO)) == 0);
    RTEST(memcmp(&oldD3dCallbacks, puD3dCallbacks, sizeof(D3DNTHAL_CALLBACKS)) == 0);
    RTEST(memcmp(&oldD3dDriverData, puD3dDriverData, sizeof(D3DNTHAL_GLOBALDRIVERDATA)) == 0);

    /* start test of puD3dBufferCallbacks */
    RTEST(puD3dBufferCallbacks->dwSize == sizeof(DD_D3DBUFCALLBACKS));
    if (puD3dBufferCallbacks->dwFlags & DDHAL_D3DBUFCB32_CANCREATED3DBUF)
    {
        /* point to kmode direcly to the graphic drv, the drv is kmode and it is kmode address we getting back*/
        RTEST( ( (DWORD)puD3dBufferCallbacks->CanCreateD3DBuffer & (~0x80000000)) != 0 );
    }
    else
    {
        RTEST( puD3dBufferCallbacks->CanCreateD3DBuffer == NULL);
    }

    if (puD3dBufferCallbacks->dwFlags & DDHAL_D3DBUFCB32_CREATED3DBUF)
    {
        /* point to kmode direcly to the graphic drv, the drv is kmode and it is kmode address we getting back*/
        RTEST( ( (DWORD)puD3dBufferCallbacks->CreateD3DBuffer & (~0x80000000)) != 0 );
    }
    else
    {
        RTEST( puD3dBufferCallbacks->CreateD3DBuffer == NULL);
    }

    if (puD3dBufferCallbacks->dwFlags & DDHAL_D3DBUFCB32_DESTROYD3DBUF)
    {
        /* point to kmode direcly to the graphic drv, the drv is kmode and it is kmode address we getting back*/
        RTEST( ( (DWORD)puD3dBufferCallbacks->DestroyD3DBuffer & (~0x80000000)) != 0 );
    }
    else
    {
        RTEST( puD3dBufferCallbacks->DestroyD3DBuffer == NULL);
    }

    if (puD3dBufferCallbacks->dwFlags & DDHAL_D3DBUFCB32_LOCKD3DBUF)
    {
        /* point to kmode direcly to the graphic drv, the drv is kmode and it is kmode address we getting back*/
        RTEST( ( (DWORD)puD3dBufferCallbacks->LockD3DBuffer & (~0x80000000)) != 0 );
    }
    else
    {
        RTEST( puD3dBufferCallbacks->LockD3DBuffer == NULL);
    }

    if (puD3dBufferCallbacks->dwFlags & DDHAL_D3DBUFCB32_UNLOCKD3DBUF)
    {
        /* point to kmode direcly to the graphic drv, the drv is kmode and it is kmode address we getting back*/
        RTEST( ( (DWORD)puD3dBufferCallbacks->UnlockD3DBuffer & (~0x80000000)) != 0 );
    }
    else
    {
        RTEST( puD3dBufferCallbacks->UnlockD3DBuffer == NULL);
    }

    /* Backup DD_D3DBUFCALLBACKS so we do not need resting it */
    RtlCopyMemory(&oldD3dBufferCallbacks, &D3dBufferCallbacks, sizeof(DD_D3DBUFCALLBACKS));


/* testing  NtGdiDdQueryDirectDrawObject( hDD, pHalInfo, pCallBackFlags, puD3dCallbacks, puD3dDriverData, puD3dBufferCallbacks, puD3dTextureFormats, NULL, */
    pHalInfo = &HalInfo;
    pCallBackFlags = CallBackFlags;
    puD3dCallbacks = &D3dCallbacks;
    puD3dDriverData = &D3dDriverData;
    puD3dBufferCallbacks = &D3dBufferCallbacks;

    /* It is forbein to return a  DDSURFACEDESC2 it should always be DDSURFACEDESC
        This is only for detected bad drivers that does not follow the rules, if they
        does not follow tthe rules, not everthing being copy then in gdi32.dll
        gdi32.dll always assume it is DDSURFACEDESC size
    */
    if (puD3dDriverData->dwNumTextureFormats != 0)
    {
        puD3dTextureFormats = malloc (puD3dDriverData->dwNumTextureFormats * sizeof(DDSURFACEDESC2));
    }

    RtlZeroMemory(pHalInfo,sizeof(DD_HALINFO));
    RtlZeroMemory(pCallBackFlags,sizeof(DWORD)*3);
    RtlZeroMemory(puD3dCallbacks,sizeof(D3DNTHAL_CALLBACKS));
    RtlZeroMemory(puD3dDriverData,sizeof(D3DNTHAL_GLOBALDRIVERDATA));
    RtlZeroMemory(&D3dBufferCallbacks,sizeof(DD_D3DBUFCALLBACKS));

    RTEST(NtGdiDdQueryDirectDrawObject( hDirectDraw, pHalInfo,
                                        pCallBackFlags, puD3dCallbacks,
                                        puD3dDriverData, puD3dBufferCallbacks,
                                        puD3dTextureFormats, puNumHeaps,
                                        puvmList, puNumFourCC,
                                        puFourCC)== FALSE);

    RTEST(pHalInfo != NULL);
    ASSERT(pHalInfo != NULL);

    RTEST(pCallBackFlags != NULL);
    ASSERT(pCallBackFlags != NULL);

    RTEST(puD3dCallbacks != NULL);
    ASSERT(puD3dCallbacks != NULL);

    RTEST(puD3dDriverData != NULL);
    ASSERT(puD3dDriverData != NULL);

    RTEST(puD3dBufferCallbacks != NULL);
    ASSERT(puD3dDriverData != NULL);

    RTEST(puD3dTextureFormats != NULL);
    ASSERT(puD3dTextureFormats != NULL);

    RTEST(puNumFourCC == NULL);
    RTEST(puFourCC == NULL);
    RTEST(puNumHeaps == NULL);
    RTEST(puvmList == NULL);

    /* We retesting the flags */
    RTEST(pCallBackFlags[0] != 0);
    RTEST(pCallBackFlags[1] != 0);
    RTEST(pCallBackFlags[2] == 0);

    /* We do not retesting instead we compare it */
    RTEST(memcmp(&oldHalInfo, pHalInfo, sizeof(DD_HALINFO)) == 0);
    RTEST(memcmp(&oldD3dCallbacks, puD3dCallbacks, sizeof(D3DNTHAL_CALLBACKS)) == 0);
    RTEST(memcmp(&oldD3dDriverData, puD3dDriverData, sizeof(D3DNTHAL_GLOBALDRIVERDATA)) == 0);
    RTEST(memcmp(&oldD3dBufferCallbacks, puD3dBufferCallbacks, sizeof(DD_D3DBUFCALLBACKS)) == 0);

    /* start test of dwNumTextureFormats */
    if (puD3dDriverData->dwNumTextureFormats != 0)
    {
        myDesc = puD3dTextureFormats;
        for (dwTextureCounter=0;dwTextureCounter<puD3dDriverData->dwNumTextureFormats;dwTextureCounter++)
        {
            RTEST(myDesc->dwSize == sizeof(DDSURFACEDESC));
            ASSERT(myDesc->dwSize == sizeof(DDSURFACEDESC));

            RTEST( (myDesc->dwFlags & (~(DDSD_CAPS|DDSD_PIXELFORMAT))) == 0);
            RTEST(myDesc->dwHeight == 0);
            RTEST(myDesc->dwWidth == 0);
            RTEST(myDesc->dwLinearSize == 0);
            RTEST(myDesc->dwBackBufferCount == 0);
            RTEST(myDesc->dwZBufferBitDepth == 0);
            RTEST(myDesc->dwAlphaBitDepth == 0);
            RTEST(myDesc->dwReserved == 0);
            RTEST(myDesc->lpSurface == 0);
            RTEST(myDesc->ddckCKDestOverlay.dwColorSpaceLowValue == 0);
            RTEST(myDesc->ddckCKDestOverlay.dwColorSpaceHighValue == 0);
            RTEST(myDesc->ddckCKDestBlt.dwColorSpaceLowValue == 0);
            RTEST(myDesc->ddckCKDestBlt.dwColorSpaceHighValue == 0);
            RTEST(myDesc->ddckCKSrcOverlay.dwColorSpaceLowValue == 0);
            RTEST(myDesc->ddckCKSrcOverlay.dwColorSpaceHighValue == 0);
            RTEST(myDesc->ddckCKSrcBlt.dwColorSpaceLowValue == 0);
            RTEST(myDesc->ddckCKSrcBlt.dwColorSpaceHighValue == 0);
            RTEST(myDesc->ddpfPixelFormat.dwSize == sizeof(DDPIXELFORMAT));
            RTEST(myDesc->ddpfPixelFormat.dwFlags != 0);
            if (myDesc->ddpfPixelFormat.dwFlags & DDPF_FOURCC)
            {
                RTEST(myDesc->ddpfPixelFormat.dwFourCC != 0);
            }
            RTEST(myDesc->ddsCaps.dwCaps == DDSCAPS_TEXTURE);

            myDesc = (DDSURFACEDESC *) (((DWORD) myDesc) + sizeof(DDSURFACEDESC));
        }
    }


    /* testing  NtGdiDdQueryDirectDrawObject( hDD, pHalInfo, pCallBackFlags, puD3dCallbacks, puD3dDriverData, puD3dBufferCallbacks, puD3dTextureFormats, puNumHeaps, NULL, */
    pHalInfo = &HalInfo;
    pCallBackFlags = CallBackFlags;
    puD3dCallbacks = &D3dCallbacks;
    puD3dDriverData = &D3dDriverData;
    puD3dBufferCallbacks = &D3dBufferCallbacks;
    puNumHeaps = &NumHeaps;

    if (puD3dDriverData->dwNumTextureFormats != 0)
    {
        RtlZeroMemory(puD3dTextureFormats, puD3dDriverData->dwNumTextureFormats * sizeof(DDSURFACEDESC2));
    }
    RtlZeroMemory(pHalInfo,sizeof(DD_HALINFO));
    RtlZeroMemory(pCallBackFlags,sizeof(DWORD)*3);
    RtlZeroMemory(puD3dCallbacks,sizeof(D3DNTHAL_CALLBACKS));
    RtlZeroMemory(puD3dDriverData,sizeof(D3DNTHAL_GLOBALDRIVERDATA));
    RtlZeroMemory(&D3dBufferCallbacks,sizeof(DD_D3DBUFCALLBACKS));

    RTEST(NtGdiDdQueryDirectDrawObject( hDirectDraw, pHalInfo,
                                        pCallBackFlags, puD3dCallbacks,
                                        puD3dDriverData, puD3dBufferCallbacks,
                                        puD3dTextureFormats, puNumHeaps,
                                        puvmList, puNumFourCC,
                                        puFourCC)== FALSE);

    RTEST(pHalInfo != NULL);
    ASSERT(pHalInfo != NULL);

    RTEST(pCallBackFlags != NULL);
    ASSERT(pCallBackFlags != NULL);

    RTEST(puD3dCallbacks != NULL);
    ASSERT(puD3dCallbacks != NULL);

    RTEST(puD3dDriverData != NULL);
    ASSERT(puD3dDriverData != NULL);

    RTEST(puD3dBufferCallbacks != NULL);
    ASSERT(puD3dDriverData != NULL);

    RTEST(puD3dTextureFormats != NULL);
    ASSERT(puD3dTextureFormats != NULL);

    RTEST(puNumHeaps != NULL);
    ASSERT(puNumHeaps != NULL);
    RTEST(NumHeaps == 0);

    RTEST(puNumFourCC == NULL);
    RTEST(puFourCC == NULL);

    RTEST(puvmList == NULL);

    /* We retesting the flags */
    RTEST(pCallBackFlags[0] != 0);
    RTEST(pCallBackFlags[1] != 0);
    RTEST(pCallBackFlags[2] == 0);

    /* We do not retesting instead we compare it */
    RTEST(memcmp(&oldHalInfo, pHalInfo, sizeof(DD_HALINFO)) == 0);
    RTEST(memcmp(&oldD3dCallbacks, puD3dCallbacks, sizeof(D3DNTHAL_CALLBACKS)) == 0);
    RTEST(memcmp(&oldD3dDriverData, puD3dDriverData, sizeof(D3DNTHAL_GLOBALDRIVERDATA)) == 0);
    RTEST(memcmp(&oldD3dBufferCallbacks, puD3dBufferCallbacks, sizeof(DD_D3DBUFCALLBACKS)) == 0);
    /* we skip resting texture */


    /* testing  NtGdiDdQueryDirectDrawObject( hDD, pHalInfo, pCallBackFlags, puD3dCallbacks, puD3dDriverData, puD3dBufferCallbacks, puD3dTextureFormats, puNumHeaps, puvmList, NULL, */
    pHalInfo = &HalInfo;
    pCallBackFlags = CallBackFlags;
    puD3dCallbacks = &D3dCallbacks;
    puD3dDriverData = &D3dDriverData;
    puD3dBufferCallbacks = &D3dBufferCallbacks;
    puNumHeaps = &NumHeaps;
    puvmList = &vmList;

    if (puD3dDriverData->dwNumTextureFormats != 0)
    {
        RtlZeroMemory(puD3dTextureFormats, puD3dDriverData->dwNumTextureFormats * sizeof(DDSURFACEDESC2));
    }
    RtlZeroMemory(pHalInfo,sizeof(DD_HALINFO));
    RtlZeroMemory(pCallBackFlags,sizeof(DWORD)*3);
    RtlZeroMemory(puD3dCallbacks,sizeof(D3DNTHAL_CALLBACKS));
    RtlZeroMemory(puD3dDriverData,sizeof(D3DNTHAL_GLOBALDRIVERDATA));
    RtlZeroMemory(&D3dBufferCallbacks,sizeof(DD_D3DBUFCALLBACKS));

    RTEST(NtGdiDdQueryDirectDrawObject( hDirectDraw, pHalInfo,
                                        pCallBackFlags, puD3dCallbacks,
                                        puD3dDriverData, puD3dBufferCallbacks,
                                        puD3dTextureFormats, puNumHeaps,
                                        puvmList, puNumFourCC,
                                        puFourCC)== FALSE);

    RTEST(pHalInfo != NULL);
    ASSERT(pHalInfo != NULL);

    RTEST(pCallBackFlags != NULL);
    ASSERT(pCallBackFlags != NULL);

    RTEST(puD3dCallbacks != NULL);
    ASSERT(puD3dCallbacks != NULL);

    RTEST(puD3dDriverData != NULL);
    ASSERT(puD3dDriverData != NULL);

    RTEST(puD3dBufferCallbacks != NULL);
    ASSERT(puD3dDriverData != NULL);

    RTEST(puD3dTextureFormats != NULL);
    ASSERT(puD3dTextureFormats != NULL);

    RTEST(puNumHeaps != NULL);
    ASSERT(puNumHeaps != NULL);
    RTEST(NumHeaps == 0);

    RTEST(puvmList != NULL);

    RTEST(puNumFourCC == NULL);
    RTEST(puFourCC == NULL);



    /* We retesting the flags */
    RTEST(pCallBackFlags[0] != 0);
    RTEST(pCallBackFlags[1] != 0);
    RTEST(pCallBackFlags[2] == 0);

    /* We do not retesting instead we compare it */
    RTEST(memcmp(&oldHalInfo, pHalInfo, sizeof(DD_HALINFO)) == 0);
    RTEST(memcmp(&oldD3dCallbacks, puD3dCallbacks, sizeof(D3DNTHAL_CALLBACKS)) == 0);
    RTEST(memcmp(&oldD3dDriverData, puD3dDriverData, sizeof(D3DNTHAL_GLOBALDRIVERDATA)) == 0);
    RTEST(memcmp(&oldD3dBufferCallbacks, puD3dBufferCallbacks, sizeof(DD_D3DBUFCALLBACKS)) == 0);
    /* we skip resting texture */

    /* Todo
    * adding test for
    * puNumFourCC
    * puFourCC
    */

    /* Cleanup ReactX setup */
    DeleteDC(hdc);
    NtGdiDdDeleteDirectDrawObject(hDirectDraw);
}
