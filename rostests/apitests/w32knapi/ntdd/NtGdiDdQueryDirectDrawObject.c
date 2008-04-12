
INT
Test_NtGdiDdQueryDirectDrawObject(PTESTINFO pti)
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
    D3DNTHAL_GLOBALDRIVERDATA D3dDriverData;
    DD_D3DBUFCALLBACKS D3dBufferCallbacks;
    DDSURFACEDESC2 D3dTextureFormats[100];
    //DWORD NumHeaps = 0;
    VIDEOMEMORY vmList;
    //DWORD NumFourCC = 0;
    //DWORD FourCC = 0;
    DEVMODE devmode;
    HDC hdc;


    /* clear data */
    memset(&vmList,0,sizeof(VIDEOMEMORY));
    memset(&D3dTextureFormats,0,sizeof(DDSURFACEDESC));
    memset(&D3dBufferCallbacks,0,sizeof(DD_D3DBUFCALLBACKS));
    memset(&D3dDriverData,0,sizeof(D3DNTHAL_GLOBALDRIVERDATA));
    memset(&D3dCallbacks,0,sizeof(D3DNTHAL_CALLBACKS));
    memset(&HalInfo,0,sizeof(DD_HALINFO));
    memset(CallBackFlags,0,sizeof(DWORD)*3);



    /* Get currenet display mode */
    EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &devmode);

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

    /* testing  OsThunkDdQueryDirectDrawObject( hDirectDrawLocal, pHalInfo, NULL, ....  */
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
        RTEST(pHalInfo->vmiData.fpPrimary != 0 );

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

        /* test see if it in kmode memory or not, t */
        RTEST(pHalInfo->vmiData.pvPrimary != 0 );
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
        RTEST(pHalInfo->ddCaps.dwSize == sizeof(DDCORECAPS));        

        /* Testing see if we got any hw support for
         * This test can fail on video card that does not support 2d/overlay/3d
         */
        RTEST( pHalInfo->ddCaps.dwCaps != 0);
        RTEST( pHalInfo->ddCaps.ddsCaps.dwCaps != 0);

        /* if this fail we do not have a dx driver install acodring ms, some version of windows it
         * is okay this fail and drv does then only support basic dx
         */
        RTEST( (pHalInfo->dwFlags & (DDHALINFO_GETDRIVERINFOSET | DDHALINFO_GETDRIVERINFO2)) != 0 );

        if (pHalInfo->ddCaps.ddsCaps.dwCaps  & DDSCAPS_3DDEVICE )
        {
            RTEST( pHalInfo->lpD3DGlobalDriverData != 0);
            RTEST( pHalInfo->lpD3DHALCallbacks != 0);
            RTEST( pHalInfo->lpD3DBufCallbacks != 0);
        }
    }


/* Next Start 2 */
    RtlCopyMemory(&oldHalInfo, &HalInfo, sizeof(DD_HALINFO));

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
    RTEST(pCallBackFlags != NULL);
    RTEST(puD3dCallbacks == NULL);
    RTEST(puD3dDriverData == NULL);
    RTEST(puD3dBufferCallbacks == NULL);
    RTEST(puD3dTextureFormats == NULL);
    RTEST(puNumFourCC == NULL);
    RTEST(puFourCC == NULL);
    ASSERT(pHalInfo != NULL);
    RTEST(puNumHeaps == NULL);
    RTEST(puvmList == NULL);

    /* We do not retesting DD_HALINFO, instead we compare it */
    RTEST(memcmp(&oldHalInfo, pHalInfo, sizeof(DD_HALINFO)) == 0);
    RTEST(pCallBackFlags[0] != 0);
    RTEST(pCallBackFlags[1] != 0);

    /* NT4 this will fail */
    RTEST(pCallBackFlags[2] == 0);

/* Next Start 3 */
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
    RTEST(pCallBackFlags != NULL);

    if (pHalInfo->ddCaps.ddsCaps.dwCaps  & DDSCAPS_3DDEVICE )
    {
        RTEST(puD3dCallbacks != NULL);
    }

    RTEST(puD3dDriverData == NULL);
    RTEST(puD3dBufferCallbacks == NULL);
    RTEST(puD3dTextureFormats == NULL);
    RTEST(puNumFourCC == NULL);
    RTEST(puFourCC == NULL);
    RTEST(puNumHeaps == NULL);
    RTEST(puvmList == NULL);
    ASSERT(pHalInfo != NULL);

    /* We do not retesting DD_HALINFO, instead we compare it */
    RTEST(memcmp(&oldHalInfo, pHalInfo, sizeof(DD_HALINFO)) == 0);
    RTEST(pCallBackFlags[0] != 0);
    RTEST(pCallBackFlags[1] != 0);

    /* NT4 this will fail */
    RTEST(pCallBackFlags[2] == 0);

/* Next Start 4 */
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
    RTEST(pCallBackFlags != NULL);

    if (pHalInfo->ddCaps.ddsCaps.dwCaps  & DDSCAPS_3DDEVICE )
    {
        RTEST(puD3dCallbacks != NULL);
        RTEST(puD3dDriverData != NULL);
    }

    RTEST(puD3dBufferCallbacks == NULL);
    RTEST(puD3dTextureFormats == NULL);
    RTEST(puNumFourCC == NULL);
    RTEST(puFourCC == NULL);
    RTEST(puNumHeaps == NULL);
    RTEST(puvmList == NULL);
    ASSERT(pHalInfo != NULL);

    /* We do not retesting DD_HALINFO, instead we compare it */
    RTEST(memcmp(&oldHalInfo, pHalInfo, sizeof(DD_HALINFO)) == 0);
    RTEST(pCallBackFlags[0] != 0);
    RTEST(pCallBackFlags[1] != 0);

    /* NT4 this will fail */
    RTEST(pCallBackFlags[2] == 0);

/* Next Start 5 */
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


    RTEST(puD3dTextureFormats == NULL);
    RTEST(puNumFourCC == NULL);
    RTEST(puFourCC == NULL);
    RTEST(puNumHeaps == NULL);
    RTEST(puvmList == NULL);
    ASSERT(pHalInfo != NULL);

    /* We do not retesting DD_HALINFO, instead we compare it */
    RTEST(memcmp(&oldHalInfo, pHalInfo, sizeof(DD_HALINFO)) == 0);
    RTEST(pCallBackFlags[0] != 0);
    RTEST(pCallBackFlags[1] != 0);

    /* NT4 this will fail */
    RTEST(pCallBackFlags[2] == 0);

/* Next Start 6 */
    pHalInfo = &HalInfo;
    pCallBackFlags = CallBackFlags;
    puD3dCallbacks = &D3dCallbacks;
    puD3dDriverData = &D3dDriverData;
    puD3dBufferCallbacks = &D3dBufferCallbacks;
    
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
    RTEST(pCallBackFlags != NULL);

    if (pHalInfo->ddCaps.ddsCaps.dwCaps  & DDSCAPS_3DDEVICE )
    {
        RTEST(puD3dCallbacks != NULL);
        RTEST(puD3dDriverData != NULL);
        RTEST(puD3dBufferCallbacks != NULL);
    }

    RTEST(puD3dTextureFormats == NULL);
    RTEST(puNumFourCC == NULL);
    RTEST(puFourCC == NULL);
    RTEST(puNumHeaps == NULL);
    RTEST(puvmList == NULL);
    ASSERT(pHalInfo != NULL);

    /* We do not retesting DD_HALINFO, instead we compare it */
    RTEST(memcmp(&oldHalInfo, pHalInfo, sizeof(DD_HALINFO)) == 0);
    RTEST(pCallBackFlags[0] != 0);
    RTEST(pCallBackFlags[1] != 0);

    /* NT4 this will fail */
    RTEST(pCallBackFlags[2] == 0);

/* Next Start 7 */
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
    RTEST(pCallBackFlags != NULL);

    if (pHalInfo->ddCaps.ddsCaps.dwCaps  & DDSCAPS_3DDEVICE )
    {
        RTEST(puD3dCallbacks != NULL);
        RTEST(puD3dDriverData != NULL);
    RTEST(puD3dBufferCallbacks != NULL);
    if (puD3dDriverData->dwNumTextureFormats != 0)
    {
            /* FIXME add a better test for texture */
            RTEST(puD3dTextureFormats != NULL);
        }
    }

    RTEST(puNumFourCC == NULL);
    RTEST(puFourCC == NULL);
    RTEST(puNumHeaps == NULL);
    RTEST(puvmList == NULL);
    ASSERT(pHalInfo != NULL);

    /* We do not retesting DD_HALINFO, instead we compare it */
    RTEST(memcmp(&oldHalInfo, pHalInfo, sizeof(DD_HALINFO)) == 0);
    RTEST(pCallBackFlags[0] != 0);
    RTEST(pCallBackFlags[1] != 0);

    /* NT4 this will fail */
    RTEST(pCallBackFlags[2] == 0);



    /* Todo
    * adding test for
    * puD3dCallbacks
    * puD3dDriverData
    * puD3dBufferCallbacks
    * puNumFourCC
    * puFourCC
    * puNumHeaps
    * puvmList
    */

    /* Cleanup ReactX setup */
    DeleteDC(hdc);
    Syscall(L"NtGdiDdDeleteDirectDrawObject", 1, &hDirectDraw);

    return APISTATUS_NORMAL;
}
