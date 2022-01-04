/* All testcase are base how windows 2000 sp4 acting */


#include <stdio.h>
#include <windows.h>
#include <wingdi.h>
#include <winddi.h>
#include <d3dnthal.h>

#include "test.h"

/* this flag are only set for dx5 and lower but we need check see if we getting it back from win32k or not */
#define D3DLIGHTCAPS_GLSPOT             0x00000010

/* this flag are only set for dx6 and lower but we need check see if we getting it back from win32k or not */
#define D3DLIGHTCAPS_PARALLELPOINT      0x00000008

#define checkflag(dwflag,dwvalue,text) \
        if (dwflag & dwvalue) \
        { \
            if (count!=0) \
            { \
                printf("| "); \
            } \
            dwflag = (ULONG)dwflag - (ULONG)dwvalue; \
            printf("%s ",text); \
            count++; \
        }

#define endcheckflag(dwflag,text) \
    if (count==0) \
        printf("0x%08lx\n", (ULONG) dwflag);\
    else \
        printf("\n");\
    if (flag != 0) \
        printf("undoc value in %s flags value %08lx\n",text, (ULONG) dwflag);



void
dump_CallBackFlags(DWORD *pCallBackFlags, char *text)
{
    UINT flag;
    INT count;

    printf("dumping the CallBackFlags from %s\n",text);
    printf("pCallBackFlags[0] : ");

    flag=pCallBackFlags[0];
    count=0;
    checkflag(flag,DDHAL_CB32_CANCREATESURFACE,"DDHAL_CB32_CANCREATESURFACE");
    checkflag(flag,DDHAL_CB32_CREATEPALETTE,"DDHAL_CB32_CREATEPALETTE");
    checkflag(flag,DDHAL_CB32_CREATESURFACE,"DDHAL_CB32_CREATESURFACE");
    checkflag(flag,DDHAL_CB32_GETSCANLINE,"DDHAL_CB32_GETSCANLINE");
    checkflag(flag,DDHAL_CB32_MAPMEMORY,"DDHAL_CB32_MAPMEMORY");
    checkflag(flag,DDHAL_CB32_SETCOLORKEY,"DDHAL_CB32_SETCOLORKEY");
    checkflag(flag,DDHAL_CB32_SETMODE,"DDHAL_CB32_SETMODE");
    checkflag(flag,DDHAL_CB32_WAITFORVERTICALBLANK,"DDHAL_CB32_WAITFORVERTICALBLANK");
    endcheckflag(flag,"pCallBackFlags[0]");

    /* SURFACE callback */
    printf("pCallBackFlags[1] : ");
    flag = pCallBackFlags[1];
    count = 0;
    checkflag(flag,DDHAL_SURFCB32_ADDATTACHEDSURFACE,"DDHAL_SURFCB32_ADDATTACHEDSURFACE");
    checkflag(flag,DDHAL_SURFCB32_BLT,"DDHAL_SURFCB32_BLT");
    checkflag(flag,DDHAL_SURFCB32_DESTROYSURFACE,"DDHAL_SURFCB32_DESTROYSURFACE");
    checkflag(flag,DDHAL_SURFCB32_FLIP,"DDHAL_SURFCB32_FLIP");
    checkflag(flag,DDHAL_SURFCB32_GETBLTSTATUS,"DDHAL_SURFCB32_GETBLTSTATUS");
    checkflag(flag,DDHAL_SURFCB32_GETFLIPSTATUS,"DDHAL_SURFCB32_GETFLIPSTATUS");
    checkflag(flag,DDHAL_SURFCB32_LOCK,"DDHAL_SURFCB32_LOCK");
    checkflag(flag,DDHAL_SURFCB32_RESERVED4,"DDHAL_SURFCB32_RESERVED4");
    checkflag(flag,DDHAL_SURFCB32_SETCLIPLIST,"DDHAL_SURFCB32_SETCLIPLIST");
    checkflag(flag,DDHAL_SURFCB32_SETCOLORKEY,"DDHAL_SURFCB32_SETCOLORKEY");
    checkflag(flag,DDHAL_SURFCB32_SETOVERLAYPOSITION,"DDHAL_SURFCB32_SETOVERLAYPOSITION");
    checkflag(flag,DDHAL_SURFCB32_SETPALETTE,"DDHAL_SURFCB32_SETPALETTE");
    checkflag(flag,DDHAL_SURFCB32_UNLOCK,"DDHAL_SURFCB32_UNLOCK");
    checkflag(flag,DDHAL_SURFCB32_UPDATEOVERLAY,"DDHAL_SURFCB32_UPDATEOVERLAY");
    endcheckflag(flag,"pCallBackFlags[1]");


    /* palleted */
    printf("pCallBackFlags[2] : ");
    flag = pCallBackFlags[2];
    count = 0;
    checkflag(flag,DDHAL_PALCB32_DESTROYPALETTE,"DDHAL_PALCB32_DESTROYPALETTE");
    checkflag(flag,DDHAL_PALCB32_SETENTRIES,"DDHAL_PALCB32_SETENTRIES");
    endcheckflag(flag,"pCallBackFlags[2]");
}

void
dump_halinfo(DD_HALINFO *pHalInfo, char *text)
{
    printf("dumping the DD_HALINFO from %s\n",text);

    if (pHalInfo->dwSize == sizeof(DD_HALINFO_V4))
    {
        DD_HALINFO_V4 *pHalInfo4 = (DD_HALINFO_V4 *) pHalInfo;
        int t;

        printf("DD_HALINFO Version NT4 found \n");
        printf(" pHalInfo4->dwSize                                  : 0x%08lx\n",(long)pHalInfo4->dwSize);
        printf(" pHalInfo4->vmiData->fpPrimary                      : 0x%08lx\n",(long)pHalInfo4->vmiData.fpPrimary);
        printf(" pHalInfo4->vmiData->dwFlags                        : 0x%08lx\n",(long)pHalInfo4->vmiData.dwFlags);
        printf(" pHalInfo4->vmiData->dwDisplayWidth                 : 0x%08lx\n",(long)pHalInfo4->vmiData.dwDisplayWidth);
        printf(" pHalInfo4->vmiData->dwDisplayHeight                : 0x%08lx\n",(long)pHalInfo4->vmiData.dwDisplayHeight);
        printf(" pHalInfo4->vmiData->lDisplayPitch                  : 0x%08lx\n",(long)pHalInfo4->vmiData.lDisplayPitch);

        printf(" pHalInfo4->vmiData->ddpfDisplay.dwSize             : 0x%08lx\n",(long)pHalInfo4->vmiData.ddpfDisplay.dwSize);
        printf(" pHalInfo4->vmiData->ddpfDisplay.dwFlags            : 0x%08lx\n",(long)pHalInfo4->vmiData.ddpfDisplay.dwFlags);
        printf(" pHalInfo4->vmiData->ddpfDisplay.dwFourCC           : 0x%08lx\n",(long)pHalInfo4->vmiData.ddpfDisplay.dwFourCC);
        printf(" pHalInfo4->vmiData->ddpfDisplay.dwRGBBitCount      : 0x%08lx\n",(long)pHalInfo4->vmiData.ddpfDisplay.dwRGBBitCount);
        printf(" pHalInfo4->vmiData->ddpfDisplay.dwRBitMask         : 0x%08lx\n",(long)pHalInfo4->vmiData.ddpfDisplay.dwRBitMask);
        printf(" pHalInfo4->vmiData->ddpfDisplay.dwGBitMask         : 0x%08lx\n",(long)pHalInfo4->vmiData.ddpfDisplay.dwGBitMask);
        printf(" pHalInfo4->vmiData->ddpfDisplay.dwBBitMask         : 0x%08lx\n",(long)pHalInfo4->vmiData.ddpfDisplay.dwBBitMask);
        printf(" pHalInfo4->vmiData->ddpfDisplay.dwRGBAlphaBitMask  : 0x%08lx\n",(long)pHalInfo4->vmiData.ddpfDisplay.dwRGBAlphaBitMask);

        printf(" pHalInfo4->vmiData->dwOffscreenAlign               : 0x%08lx\n",(long)pHalInfo4->vmiData.dwOffscreenAlign);
        printf(" pHalInfo4->vmiData->dwOverlayAlign                 : 0x%08lx\n",(long)pHalInfo4->vmiData.dwOverlayAlign);
        printf(" pHalInfo4->vmiData->dwTextureAlign                 : 0x%08lx\n",(long)pHalInfo4->vmiData.dwTextureAlign);
        printf(" pHalInfo4->vmiData->dwZBufferAlign                 : 0x%08lx\n",(long)pHalInfo4->vmiData.dwZBufferAlign);
        printf(" pHalInfo4->vmiData->dwAlphaAlign                   : 0x%08lx\n",(long)pHalInfo4->vmiData.dwAlphaAlign);
        printf(" pHalInfo4->vmiData->pvPrimary                      : 0x%08lx\n",(long)pHalInfo4->vmiData.pvPrimary);

        printf(" pHalInfo4->ddCaps.dwSize                           : 0x%08lx\n",pHalInfo4->ddCaps.dwSize);
        printf(" pHalInfo4->ddCaps.dwCaps                           : 0x%08lx\n",pHalInfo4->ddCaps.dwCaps);
        printf(" pHalInfo4->ddCaps.dwCaps2                          : 0x%08lx\n",pHalInfo4->ddCaps.dwCaps2);
        printf(" pHalInfo4->ddCaps.dwCKeyCaps                       : 0x%08lx\n",pHalInfo4->ddCaps.dwCKeyCaps);
        printf(" pHalInfo4->ddCaps.dwFXCaps                         : 0x%08lx\n",pHalInfo4->ddCaps.dwFXCaps);
        printf(" pHalInfo4->ddCaps.dwFXAlphaCaps                    : 0x%08lx\n",pHalInfo4->ddCaps.dwFXAlphaCaps);
        printf(" pHalInfo4->ddCaps.dwPalCaps                        : 0x%08lx\n",pHalInfo4->ddCaps.dwPalCaps);
        printf(" pHalInfo4->ddCaps.dwSVCaps                         : 0x%08lx\n",pHalInfo4->ddCaps.dwSVCaps);
        printf(" pHalInfo4->ddCaps.dwAlphaBltConstBitDepths         : 0x%08lx\n",pHalInfo4->ddCaps.dwAlphaBltConstBitDepths);
        printf(" pHalInfo4->ddCaps.dwAlphaBltPixelBitDepths         : 0x%08lx\n",pHalInfo4->ddCaps.dwAlphaBltPixelBitDepths);
        printf(" pHalInfo4->ddCaps.dwAlphaBltSurfaceBitDepths       : 0x%08lx\n",pHalInfo4->ddCaps.dwAlphaBltSurfaceBitDepths);
        printf(" pHalInfo4->ddCaps.dwAlphaOverlayConstBitDepths     : 0x%08lx\n",pHalInfo4->ddCaps.dwAlphaOverlayConstBitDepths);
        printf(" pHalInfo4->ddCaps.dwAlphaOverlayPixelBitDepths     : 0x%08lx\n",pHalInfo4->ddCaps.dwAlphaOverlayPixelBitDepths);
        printf(" pHalInfo4->ddCaps.dwAlphaOverlaySurfaceBitDepths   : 0x%08lx\n",pHalInfo4->ddCaps.dwAlphaOverlaySurfaceBitDepths);
        printf(" pHalInfo4->ddCaps.dwZBufferBitDepths               : 0x%08lx\n",pHalInfo4->ddCaps.dwZBufferBitDepths);
        printf(" pHalInfo4->ddCaps.dwVidMemTotal                    : 0x%08lx\n",pHalInfo4->ddCaps.dwVidMemTotal);
        printf(" pHalInfo4->ddCaps.dwVidMemFree                     : 0x%08lx\n",pHalInfo4->ddCaps.dwVidMemFree);
        printf(" pHalInfo4->ddCaps.dwMaxVisibleOverlays             : 0x%08lx\n",pHalInfo4->ddCaps.dwMaxVisibleOverlays);
        printf(" pHalInfo4->ddCaps.dwCurrVisibleOverlays            : 0x%08lx\n",pHalInfo4->ddCaps.dwCurrVisibleOverlays);
        printf(" pHalInfo4->ddCaps.dwNumFourCCCodes                 : 0x%08lx\n",pHalInfo4->ddCaps.dwNumFourCCCodes);
        printf(" pHalInfo4->ddCaps.dwAlignBoundarySrc               : 0x%08lx\n",pHalInfo4->ddCaps.dwAlignBoundarySrc);
        printf(" pHalInfo4->ddCaps.dwAlignSizeSrc                   : 0x%08lx\n",pHalInfo4->ddCaps.dwAlignSizeSrc);
        printf(" pHalInfo4->ddCaps.dwAlignBoundaryDes               : 0x%08lx\n",pHalInfo4->ddCaps.dwAlignBoundaryDest);
        printf(" pHalInfo4->ddCaps.dwAlignSizeDest                  : 0x%08lx\n",pHalInfo4->ddCaps.dwAlignSizeDest);
        printf(" pHalInfo4->ddCaps.dwAlignStrideAlign               : 0x%08lx\n",pHalInfo4->ddCaps.dwAlignStrideAlign);
        for (t=0;t<DD_ROP_SPACE;t++)
        {
        printf(" pHalInfo4->ddCaps.dwRops[0x%04x]                   : 0x%08lx\n",t,pHalInfo4->ddCaps.dwRops[t]);
        }
        printf(" pHalInfo4->ddCaps.ddsCaps.dwCaps                   : 0x%08lx\n",pHalInfo4->ddCaps.ddsCaps.dwCaps);
        printf(" pHalInfo4->ddCaps.dwMinOverlayStretch              : 0x%08lx\n",pHalInfo4->ddCaps.dwMinOverlayStretch);
        printf(" pHalInfo4->ddCaps.dwMaxOverlayStretch              : 0x%08lx\n",pHalInfo4->ddCaps.dwMaxOverlayStretch);
        printf(" pHalInfo4->ddCaps.dwMinLiveVideoStretch            : 0x%08lx\n",pHalInfo4->ddCaps.dwMinLiveVideoStretch);
        printf(" pHalInfo4->ddCaps.dwMaxLiveVideoStretch            : 0x%08lx\n",pHalInfo4->ddCaps.dwMaxLiveVideoStretch);
        printf(" pHalInfo4->ddCaps.dwMinHwCodecStretch              : 0x%08lx\n",pHalInfo4->ddCaps.dwMinHwCodecStretch);
        printf(" pHalInfo4->ddCaps.dwMaxHwCodecStretch              : 0x%08lx\n",pHalInfo4->ddCaps.dwMaxHwCodecStretch);
        printf(" pHalInfo4->ddCaps.dwReserved1                      : 0x%08lx\n",pHalInfo4->ddCaps.dwReserved1);
        printf(" pHalInfo4->ddCaps.dwReserved2                      : 0x%08lx\n",pHalInfo4->ddCaps.dwReserved2);
        printf(" pHalInfo4->ddCaps.dwReserved3                      : 0x%08lx\n",pHalInfo4->ddCaps.dwReserved3);
        printf(" pHalInfo4->ddCaps.dwSVBCaps                        : 0x%08lx\n",pHalInfo4->ddCaps.dwSVBCaps);
        printf(" pHalInfo4->ddCaps.dwSVBCKeyCaps                    : 0x%08lx\n",pHalInfo4->ddCaps.dwSVBCKeyCaps);
        printf(" pHalInfo4->ddCaps.dwSVBFXCaps                      : 0x%08lx\n",pHalInfo4->ddCaps.dwSVBFXCaps);
        for (t=0;t<DD_ROP_SPACE;t++)
        {
        printf(" pHalInfo4->ddCaps.dwSVBRops[0x%04x]                : 0x%08lx\n",t,pHalInfo4->ddCaps.dwSVBRops[t]);
        }
        printf(" pHalInfo4->ddCaps.dwVSBCaps                        : 0x%08lx\n",pHalInfo4->ddCaps.dwVSBCaps);
        printf(" pHalInfo4->ddCaps.dwVSBCKeyCaps                    : 0x%08lx\n",pHalInfo4->ddCaps.dwVSBCKeyCaps);
        printf(" pHalInfo4->ddCaps.dwVSBFXCaps                      : 0x%08lx\n",pHalInfo4->ddCaps.dwVSBFXCaps);
        for (t=0;t<DD_ROP_SPACE;t++)
        {
        printf(" pHalInfo4->ddCaps.dwVSBRops[0x%04x]                : 0x%08lx\n",t,pHalInfo4->ddCaps.dwVSBRops[t]);
        }
        printf(" pHalInfo4->ddCaps.dwSSBCaps                        : 0x%08lx\n",pHalInfo4->ddCaps.dwSSBCaps);
        printf(" pHalInfo4->ddCaps.dwSSBCKeyCa                      : 0x%08lx\n",pHalInfo4->ddCaps.dwSSBCKeyCaps);
        printf(" pHalInfo4->ddCaps.dwSSBFXCaps                      : 0x%08lx\n",pHalInfo4->ddCaps.dwSSBFXCaps);
        for (t=0;t<DD_ROP_SPACE;t++)
        {
        printf(" pHalInfo4->ddCaps.dwSSBRops[0x%04x]                : 0x%08lx\n",t,pHalInfo4->ddCaps.dwSSBRops[t]);
        }

        printf(" pHalInfo4->ddCaps.dwMaxVideoPorts                  : 0x%08lx\n",pHalInfo4->ddCaps.dwMaxVideoPorts);
        printf(" pHalInfo4->ddCaps.dwCurrVideoPorts                 : 0x%08lx\n",pHalInfo4->ddCaps.dwCurrVideoPorts);
        printf(" pHalInfo4->ddCaps.dwSVBCaps2                       : 0x%08lx\n",pHalInfo4->ddCaps.dwSVBCaps2);


        printf(" pHalInfo4->GetDriverInfo                           : 0x%08lx\n",(long)pHalInfo4->GetDriverInfo);
        printf(" pHalInfo4->dwFlags                                 : 0x%08lx\n",(long)pHalInfo4->dwFlags);

    }
    else if (pHalInfo->dwSize == sizeof(DD_HALINFO))
    {
        int t;
        UINT flag;
        INT count=0;

        printf("DD_HALINFO Version NT 2000/XP/2003 found \n");
        printf(" pHalInfo->dwSize                                  : 0x%08lx\n",(long)pHalInfo->dwSize);

        printf(" pHalInfo->vmiData->fpPrimary                      : 0x%08lx\n",(long)pHalInfo->vmiData.fpPrimary);
        printf(" pHalInfo->vmiData->dwFlags                        : 0x%08lx\n",(long)pHalInfo->vmiData.dwFlags);
        printf(" pHalInfo->vmiData->dwDisplayWidth                 : 0x%08lx\n",(long)pHalInfo->vmiData.dwDisplayWidth);
        printf(" pHalInfo->vmiData->dwDisplayHeight                : 0x%08lx\n",(long)pHalInfo->vmiData.dwDisplayHeight);
        printf(" pHalInfo->vmiData->lDisplayPitch                  : 0x%08lx\n",(long)pHalInfo->vmiData.lDisplayPitch);

        printf(" pHalInfo->vmiData->ddpfDisplay.dwSize             : 0x%08lx\n",(long)pHalInfo->vmiData.ddpfDisplay.dwSize);
        printf(" pHalInfo->vmiData->ddpfDisplay.dwFlags            : 0x%08lx\n",(long)pHalInfo->vmiData.ddpfDisplay.dwFlags);
        printf(" pHalInfo->vmiData->ddpfDisplay.dwFourCC           : 0x%08lx\n",(long)pHalInfo->vmiData.ddpfDisplay.dwFourCC);
        printf(" pHalInfo->vmiData->ddpfDisplay.dwRGBBitCount      : 0x%08lx\n",(long)pHalInfo->vmiData.ddpfDisplay.dwRGBBitCount);
        printf(" pHalInfo->vmiData->ddpfDisplay.dwRBitMask         : 0x%08lx\n",(long)pHalInfo->vmiData.ddpfDisplay.dwRBitMask);
        printf(" pHalInfo->vmiData->ddpfDisplay.dwGBitMask         : 0x%08lx\n",(long)pHalInfo->vmiData.ddpfDisplay.dwGBitMask);
        printf(" pHalInfo->vmiData->ddpfDisplay.dwBBitMask         : 0x%08lx\n",(long)pHalInfo->vmiData.ddpfDisplay.dwBBitMask);
        printf(" pHalInfo->vmiData->ddpfDisplay.dwRGBAlphaBitMask  : 0x%08lx\n",(long)pHalInfo->vmiData.ddpfDisplay.dwRGBAlphaBitMask);


        printf(" pHalInfo->vmiData->dwOffscreenAlign               : 0x%08lx\n",(long)pHalInfo->vmiData.dwOffscreenAlign);
        printf(" pHalInfo->vmiData->dwOverlayAlign                 : 0x%08lx\n",(long)pHalInfo->vmiData.dwOverlayAlign);
        printf(" pHalInfo->vmiData->dwTextureAlign                 : 0x%08lx\n",(long)pHalInfo->vmiData.dwTextureAlign);
        printf(" pHalInfo->vmiData->dwZBufferAlign                 : 0x%08lx\n",(long)pHalInfo->vmiData.dwZBufferAlign);
        printf(" pHalInfo->vmiData->dwAlphaAlign                   : 0x%08lx\n",(long)pHalInfo->vmiData.dwAlphaAlign);
        printf(" pHalInfo->vmiData->pvPrimary                      : 0x%08lx\n",(long)pHalInfo->vmiData.pvPrimary);

        printf(" pHalInfo->ddCaps.dwSize                           : 0x%08lx\n",pHalInfo->ddCaps.dwSize);
        printf(" pHalInfo->ddCaps.dwCaps                           : ");
        flag = pHalInfo->ddCaps.dwCaps;
        count = 0;
        checkflag(flag,DDCAPS_3D,"DDCAPS_3D");
        checkflag(flag,DDCAPS_ALIGNBOUNDARYDEST,"DDCAPS_ALIGNBOUNDARYDEST");
        checkflag(flag,DDCAPS_ALIGNBOUNDARYSRC,"DDCAPS_ALIGNBOUNDARYSRC");
        checkflag(flag,DDCAPS_ALIGNSIZEDEST,"DDCAPS_ALIGNSIZEDEST");
        checkflag(flag,DDCAPS_ALIGNSIZESRC,"DDCAPS_ALIGNSIZESRC");
        checkflag(flag,DDCAPS_ALIGNSTRIDE,"DDCAPS_ALIGNSTRIDE");
        checkflag(flag,DDCAPS_ALPHA,"DDCAPS_ALPHA");
        checkflag(flag,DDCAPS_BANKSWITCHED,"DDCAPS_BANKSWITCHED");
        checkflag(flag,DDCAPS_BLT,"DDCAPS_BLT");
        checkflag(flag,DDCAPS_BLTCOLORFILL,"DDCAPS_BLTCOLORFILL");
        checkflag(flag,DDCAPS_BLTDEPTHFILL,"DDCAPS_BLTDEPTHFILL");
        checkflag(flag,DDCAPS_BLTFOURCC,"DDCAPS_BLTFOURCC");
        checkflag(flag,DDCAPS_BLTQUEUE,"DDCAPS_BLTQUEUE");
        checkflag(flag,DDCAPS_BLTSTRETCH,"DDCAPS_BLTSTRETCH");
        checkflag(flag,DDCAPS_CANBLTSYSMEM,"DDCAPS_CANBLTSYSMEM");
        checkflag(flag,DDCAPS_CANCLIP,"DDCAPS_CANCLIP");
        checkflag(flag,DDCAPS_CANCLIPSTRETCHED,"DDCAPS_CANCLIPSTRETCHED");
        checkflag(flag,DDCAPS_COLORKEY,"DDCAPS_COLORKEY");
        checkflag(flag,DDCAPS_COLORKEYHWASSIST,"DDCAPS_COLORKEYHWASSIST");
        checkflag(flag,DDCAPS_GDI,"DDCAPS_GDI");
        checkflag(flag,DDCAPS_NOHARDWARE,"DDCAPS_NOHARDWARE");
        checkflag(flag,DDCAPS_OVERLAY,"DDCAPS_OVERLAY");
        checkflag(flag,DDCAPS_OVERLAYCANTCLIP,"DDCAPS_OVERLAYCANTCLIP");
        checkflag(flag,DDCAPS_OVERLAYFOURCC,"DDCAPS_OVERLAYFOURCC");
        checkflag(flag,DDCAPS_OVERLAYSTRETCH,"DDCAPS_OVERLAYSTRETCH");
        checkflag(flag,DDCAPS_PALETTE,"DDCAPS_PALETTE");
        checkflag(flag,DDCAPS_PALETTEVSYNC,"DDCAPS_PALETTEVSYNC");
        checkflag(flag,DDCAPS_READSCANLINE,"DDCAPS_READSCANLINE");
        checkflag(flag,DDCAPS_STEREOVIEW,"DDCAPS_STEREOVIEW");
        checkflag(flag,DDCAPS_VBI,"DDCAPS_VBI");
        checkflag(flag,DDCAPS_ZBLTS,"DDCAPS_ZBLTS");
        checkflag(flag,DDCAPS_ZOVERLAYS,"DDCAPS_ZOVERLAYS");
        endcheckflag(flag,"pHalInfo->ddCaps.dwCaps");

        printf(" pHalInfo->ddCaps.dwCaps2                          : ");
        flag = pHalInfo->ddCaps.dwCaps2;
        count = 0;
        checkflag(flag,DDCAPS2_AUTOFLIPOVERLAY,"DDCAPS2_AUTOFLIPOVERLAY");
        checkflag(flag,DDCAPS2_CANAUTOGENMIPMAP,"DDCAPS2_CANAUTOGENMIPMAP");
        checkflag(flag,DDCAPS2_CANBOBHARDWARE,"DDCAPS2_CANBOBHARDWARE");
        checkflag(flag,DDCAPS2_CANBOBINTERLEAVED,"DDCAPS2_CANBOBINTERLEAVED");
        checkflag(flag,DDCAPS2_CANBOBNONINTERLEAVED,"DDCAPS2_CANBOBNONINTERLEAVED");
        checkflag(flag,DDCAPS2_CANCALIBRATEGAMMA,"DDCAPS2_CANCALIBRATEGAMMA");
        checkflag(flag,DDCAPS2_CANDROPZ16BIT,"DDCAPS2_CANDROPZ16BIT");
        checkflag(flag,DDCAPS2_CANFLIPODDEVEN,"DDCAPS2_CANFLIPODDEVEN");
        checkflag(flag,DDCAPS2_CANMANAGERESOURCE,"DDCAPS2_CANMANAGERESOURCE");
        checkflag(flag,DDCAPS2_CANMANAGETEXTURE,"DDCAPS2_CANMANAGETEXTURE");

        checkflag(flag,DDCAPS2_CANRENDERWINDOWED,"DDCAPS2_CANRENDERWINDOWED");
        checkflag(flag,DDCAPS2_CERTIFIED,"DDCAPS2_CERTIFIED");
        checkflag(flag,DDCAPS2_COLORCONTROLOVERLAY,"DDCAPS2_COLORCONTROLOVERLAY");
        checkflag(flag,DDCAPS2_COLORCONTROLPRIMARY,"DDCAPS2_COLORCONTROLPRIMARY");
        checkflag(flag,DDCAPS2_COPYFOURCC,"DDCAPS2_COPYFOURCC");
        checkflag(flag,DDCAPS2_FLIPINTERVAL,"DDCAPS2_FLIPINTERVAL");
        checkflag(flag,DDCAPS2_FLIPNOVSYNC,"DDCAPS2_FLIPNOVSYNC");
        checkflag(flag,DDCAPS2_NO2DDURING3DSCENE,"DDCAPS2_NO2DDURING3DSCENE");
        checkflag(flag,DDCAPS2_NONLOCALVIDMEM,"DDCAPS2_NONLOCALVIDMEM");
        checkflag(flag,DDCAPS2_NONLOCALVIDMEMCAPS,"DDCAPS2_NONLOCALVIDMEMCAPS");
        checkflag(flag,DDCAPS2_NOPAGELOCKREQUIRED,"DDCAPS2_NOPAGELOCKREQUIRED");
        checkflag(flag,DDCAPS2_PRIMARYGAMMA,"DDCAPS2_PRIMARYGAMMA");
        checkflag(flag,DDCAPS2_VIDEOPORT,"DDCAPS2_VIDEOPORT");
        checkflag(flag,DDCAPS2_WIDESURFACES,"DDCAPS2_WIDESURFACES");
        endcheckflag(flag,"pHalInfo->ddCaps.dwCaps2");

        printf(" pHalInfo->ddCaps.dwCKeyCaps                       : ");
        flag = pHalInfo->ddCaps.dwCKeyCaps;
        count = 0;
        checkflag(flag,DDCKEYCAPS_DESTBLT,"DDCKEYCAPS_DESTBLT");
        checkflag(flag,DDCKEYCAPS_DESTBLTCLRSPACE,"DDCKEYCAPS_DESTBLTCLRSPACE");
        checkflag(flag,DDCKEYCAPS_DESTBLTCLRSPACEYUV,"DDCKEYCAPS_DESTBLTCLRSPACEYUV");
        checkflag(flag,DDCKEYCAPS_DESTBLTYUV,"DDCKEYCAPS_DESTBLTYUV");
        checkflag(flag,DDCKEYCAPS_DESTOVERLAY,"DDCKEYCAPS_DESTOVERLAY");
        checkflag(flag,DDCKEYCAPS_DESTOVERLAYCLRSPACE,"DDCKEYCAPS_DESTOVERLAYCLRSPACE");
        checkflag(flag,DDCKEYCAPS_DESTOVERLAYCLRSPACEYUV,"DDCKEYCAPS_DESTOVERLAYCLRSPACEYUV");
        checkflag(flag,DDCKEYCAPS_DESTOVERLAYONEACTIVE,"DDCKEYCAPS_DESTOVERLAYONEACTIVE");
        checkflag(flag,DDCKEYCAPS_DESTOVERLAYYUV,"DDCKEYCAPS_DESTOVERLAYYUV");
        checkflag(flag,DDCKEYCAPS_NOCOSTOVERLAY,"DDCKEYCAPS_NOCOSTOVERLAY");
        checkflag(flag,DDCKEYCAPS_SRCBLT,"DDCKEYCAPS_SRCBLT");
        checkflag(flag,DDCKEYCAPS_SRCBLTCLRSPACE,"DDCKEYCAPS_SRCBLTCLRSPACE");
        checkflag(flag,DDCKEYCAPS_SRCBLTCLRSPACEYUV,"DDCKEYCAPS_SRCBLTCLRSPACEYUV");
        checkflag(flag,DDCKEYCAPS_SRCBLTYUV,"DDCKEYCAPS_SRCBLTYUV");
        checkflag(flag,DDCKEYCAPS_SRCOVERLAY,"DDCKEYCAPS_SRCOVERLAY");
        checkflag(flag,DDCKEYCAPS_SRCOVERLAYCLRSPACE,"DDCKEYCAPS_SRCOVERLAYCLRSPACE");
        checkflag(flag,DDCKEYCAPS_SRCOVERLAYCLRSPACEYUV,"DDCKEYCAPS_SRCOVERLAYCLRSPACEYUV");
        checkflag(flag,DDCKEYCAPS_SRCOVERLAYONEACTIVE,"DDCKEYCAPS_SRCOVERLAYONEACTIVE");
        checkflag(flag,DDCKEYCAPS_SRCOVERLAYYUV,"DDCKEYCAPS_SRCOVERLAYYUV");
        endcheckflag(flag,"pHalInfo->ddCaps.dwCKeyCaps");

        printf(" pHalInfo->ddCaps.dwFXCaps                         : ");
        flag = pHalInfo->ddCaps.dwFXCaps;
        count = 0;
        checkflag(flag,DDFXCAPS_BLTARITHSTRETCHY,"DDFXCAPS_BLTARITHSTRETCHY");
        checkflag(flag,DDFXCAPS_BLTARITHSTRETCHYN,"DDFXCAPS_BLTARITHSTRETCHYN");
        checkflag(flag,DDFXCAPS_BLTMIRRORLEFTRIGHT,"DDFXCAPS_BLTMIRRORLEFTRIGHT");
        checkflag(flag,DDFXCAPS_BLTMIRRORUPDOWN,"DDFXCAPS_BLTMIRRORUPDOWN");
        checkflag(flag,DDFXCAPS_BLTROTATION,"DDFXCAPS_BLTROTATION");
        checkflag(flag,DDFXCAPS_BLTROTATION90,"DDFXCAPS_BLTROTATION90");
        checkflag(flag,DDFXCAPS_BLTSHRINKX,"DDFXCAPS_BLTSHRINKX");
        checkflag(flag,DDFXCAPS_BLTSHRINKXN,"DDFXCAPS_BLTSHRINKXN");
        checkflag(flag,DDFXCAPS_BLTSHRINKY,"DDFXCAPS_BLTSHRINKY");
        checkflag(flag,DDFXCAPS_BLTSHRINKYN,"DDFXCAPS_BLTSHRINKYN");
        checkflag(flag,DDFXCAPS_BLTSTRETCHX,"DDFXCAPS_BLTSTRETCHX");
        checkflag(flag,DDFXCAPS_BLTSTRETCHXN,"DDFXCAPS_BLTSTRETCHXN");
        checkflag(flag,DDFXCAPS_BLTSTRETCHY,"DDFXCAPS_BLTSTRETCHY");
        checkflag(flag,DDFXCAPS_BLTSTRETCHYN,"DDFXCAPS_BLTSTRETCHYN");
        checkflag(flag,DDFXCAPS_OVERLAYARITHSTRETCHY,"DDFXCAPS_OVERLAYARITHSTRETCHY");
        checkflag(flag,DDFXCAPS_OVERLAYARITHSTRETCHYN,"DDFXCAPS_OVERLAYARITHSTRETCHYN");
        checkflag(flag,DDFXCAPS_OVERLAYMIRRORLEFTRIGHT,"DDFXCAPS_OVERLAYMIRRORLEFTRIGHT");
        checkflag(flag,DDFXCAPS_OVERLAYMIRRORUPDOWN,"DDFXCAPS_OVERLAYMIRRORUPDOWN");
        checkflag(flag,DDFXCAPS_OVERLAYSHRINKX,"DDFXCAPS_OVERLAYSHRINKX");
        checkflag(flag,DDFXCAPS_OVERLAYSHRINKXN,"DDFXCAPS_OVERLAYSHRINKXN");
        checkflag(flag,DDFXCAPS_OVERLAYSHRINKY,"DDFXCAPS_OVERLAYSHRINKY");
        checkflag(flag,DDFXCAPS_OVERLAYSHRINKYN,"DDFXCAPS_OVERLAYSHRINKYN");
        checkflag(flag,DDFXCAPS_OVERLAYSTRETCHX,"DDFXCAPS_OVERLAYSTRETCHX");
        checkflag(flag,DDFXCAPS_OVERLAYSTRETCHX,"DDFXCAPS_OVERLAYSTRETCHX");
        checkflag(flag,DDFXCAPS_OVERLAYSTRETCHY,"DDFXCAPS_OVERLAYSTRETCHY");
        checkflag(flag,DDFXCAPS_OVERLAYSTRETCHYN,"DDFXCAPS_OVERLAYSTRETCHYN");
        endcheckflag(flag,"pHalInfo->ddCaps.dwFXCaps");

        printf(" pHalInfo->ddCaps.dwFXAlphaCaps                    : 0x%08lx\n",pHalInfo->ddCaps.dwFXAlphaCaps);
        printf(" pHalInfo->ddCaps.dwPalCaps                        : 0x%08lx\n",pHalInfo->ddCaps.dwPalCaps);

        printf(" pHalInfo->ddCaps.dwSVCaps                         : ");
        flag = pHalInfo->ddCaps.dwSVCaps;
        count = 0;
        checkflag(flag,DDSVCAPS_ENIGMA,"DDSVCAPS_ENIGMA");
        checkflag(flag,DDSVCAPS_FLICKER,"DDSVCAPS_FLICKER");
        checkflag(flag,DDSVCAPS_REDBLUE,"DDSVCAPS_REDBLUE");
        checkflag(flag,DDSVCAPS_SPLIT,"DDSVCAPS_SPLIT");
        endcheckflag(flag,"pHalInfo->ddCaps.dwSVCaps");

        printf(" pHalInfo->ddCaps.dwAlphaBltConstBitDepths         : 0x%08lx\n",pHalInfo->ddCaps.dwAlphaBltConstBitDepths);
        printf(" pHalInfo->ddCaps.dwAlphaBltPixelBitDepths         : 0x%08lx\n",pHalInfo->ddCaps.dwAlphaBltPixelBitDepths);
        printf(" pHalInfo->ddCaps.dwAlphaBltSurfaceBitDepths       : 0x%08lx\n",pHalInfo->ddCaps.dwAlphaBltSurfaceBitDepths);
        printf(" pHalInfo->ddCaps.dwAlphaOverlayConstBitDepths     : 0x%08lx\n",pHalInfo->ddCaps.dwAlphaOverlayConstBitDepths);
        printf(" pHalInfo->ddCaps.dwAlphaOverlayPixelBitDepths     : 0x%08lx\n",pHalInfo->ddCaps.dwAlphaOverlayPixelBitDepths);
        printf(" pHalInfo->ddCaps.dwAlphaOverlaySurfaceBitDepths   : 0x%08lx\n",pHalInfo->ddCaps.dwAlphaOverlaySurfaceBitDepths);
        printf(" pHalInfo->ddCaps.dwZBufferBitDepths               : 0x%08lx\n",pHalInfo->ddCaps.dwZBufferBitDepths);
        printf(" pHalInfo->ddCaps.dwVidMemTotal                    : 0x%08lx\n",pHalInfo->ddCaps.dwVidMemTotal);
        printf(" pHalInfo->ddCaps.dwVidMemFree                     : 0x%08lx\n",pHalInfo->ddCaps.dwVidMemFree);
        printf(" pHalInfo->ddCaps.dwMaxVisibleOverlays             : 0x%08lx\n",pHalInfo->ddCaps.dwMaxVisibleOverlays);
        printf(" pHalInfo->ddCaps.dwCurrVisibleOverlays            : 0x%08lx\n",pHalInfo->ddCaps.dwCurrVisibleOverlays);
        printf(" pHalInfo->ddCaps.dwNumFourCCCodes                 : 0x%08lx\n",pHalInfo->ddCaps.dwNumFourCCCodes);
        printf(" pHalInfo->ddCaps.dwAlignBoundarySrc               : 0x%08lx\n",pHalInfo->ddCaps.dwAlignBoundarySrc);
        printf(" pHalInfo->ddCaps.dwAlignSizeSrc                   : 0x%08lx\n",pHalInfo->ddCaps.dwAlignSizeSrc);
        printf(" pHalInfo->ddCaps.dwAlignBoundaryDes               : 0x%08lx\n",pHalInfo->ddCaps.dwAlignBoundaryDest);
        printf(" pHalInfo->ddCaps.dwAlignSizeDest                  : 0x%08lx\n",pHalInfo->ddCaps.dwAlignSizeDest);
        printf(" pHalInfo->ddCaps.dwAlignStrideAlign               : 0x%08lx\n",pHalInfo->ddCaps.dwAlignStrideAlign);
        for (t=0;t<DD_ROP_SPACE;t++)
        {
        printf(" pHalInfo->ddCaps.dwRops[0x%04x]                   : 0x%08lx\n",t,pHalInfo->ddCaps.dwRops[t]);
        }
        printf(" pHalInfo->ddCaps.ddsCaps.dwCaps                   : ");
        flag = pHalInfo->ddCaps.ddsCaps.dwCaps;
        count = 0;
        checkflag(flag,DDSCAPS_3DDEVICE,"DDSCAPS_3DDEVICE");
        checkflag(flag,DDSCAPS_ALLOCONLOAD,"DDSCAPS_ALLOCONLOAD");
        checkflag(flag,DDSCAPS_ALPHA,"DDSCAPS_ALPHA");
        checkflag(flag,DDSCAPS_BACKBUFFER,"DDSCAPS_BACKBUFFER");
        checkflag(flag,DDSCAPS_COMPLEX,"DDSCAPS_COMPLEX");
        checkflag(flag,DDSCAPS_EXECUTEBUFFER,"DDSCAPS_EXECUTEBUFFER");
        checkflag(flag,DDSCAPS_FLIP,"DDSCAPS_FLIP");
        checkflag(flag,DDSCAPS_FRONTBUFFER,"DDSCAPS_FRONTBUFFER");
        checkflag(flag,DDSCAPS_HWCODEC,"DDSCAPS_HWCODEC");
        checkflag(flag,DDSCAPS_LIVEVIDEO,"DDSCAPS_LIVEVIDEO");
        checkflag(flag,DDSCAPS_LOCALVIDMEM,"DDSCAPS_LOCALVIDMEM");
        checkflag(flag,DDSCAPS_MIPMAP,"DDSCAPS_MIPMAP");
        checkflag(flag,DDSCAPS_MODEX,"DDSCAPS_MODEX");
        checkflag(flag,DDSCAPS_NONLOCALVIDMEM,"DDSCAPS_NONLOCALVIDMEM");
        checkflag(flag,DDSCAPS_OFFSCREENPLAIN,"DDSCAPS_OFFSCREENPLAIN");
        checkflag(flag,DDSCAPS_OVERLAY,"DDSCAPS_OVERLAY");
        checkflag(flag,DDSCAPS_OPTIMIZED,"DDSCAPS_OPTIMIZED");
        checkflag(flag,DDSCAPS_OWNDC,"DDSCAPS_OWNDC");
        checkflag(flag,DDSCAPS_PALETTE,"DDSCAPS_PALETTE");
        checkflag(flag,DDSCAPS_PRIMARYSURFACE,"DDSCAPS_PRIMARYSURFACE");
        checkflag(flag,DDSCAPS_PRIMARYSURFACELEFT,"DDSCAPS_PRIMARYSURFACELEFT");
        checkflag(flag,DDSCAPS_STANDARDVGAMODE,"DDSCAPS_STANDARDVGAMODE");
        checkflag(flag,DDSCAPS_SYSTEMMEMORY,"DDSCAPS_SYSTEMMEMORY");
        checkflag(flag,DDSCAPS_TEXTURE,"DDSCAPS_TEXTURE");
        checkflag(flag,DDSCAPS_VIDEOMEMORY,"DDSCAPS_VIDEOMEMORY");
        checkflag(flag,DDSCAPS_VIDEOPORT,"DDSCAPS_VIDEOPORT");
        checkflag(flag,DDSCAPS_VISIBLE,"DDSCAPS_VISIBLE");
        checkflag(flag,DDSCAPS_WRITEONLY,"DDSCAPS_WRITEONLY");
        checkflag(flag,DDSCAPS_ZBUFFER,"DDSCAPS_ZBUFFER");
        endcheckflag(flag,"pHalInfo->ddCaps.ddsCaps.dwCaps");

        printf(" pHalInfo->ddCaps.dwMinOverlayStretch              : 0x%08lx\n",pHalInfo->ddCaps.dwMinOverlayStretch);
        printf(" pHalInfo->ddCaps.dwMaxOverlayStretch              : 0x%08lx\n",pHalInfo->ddCaps.dwMaxOverlayStretch);
        printf(" pHalInfo->ddCaps.dwMinLiveVideoStretch            : 0x%08lx\n",pHalInfo->ddCaps.dwMinLiveVideoStretch);
        printf(" pHalInfo->ddCaps.dwMaxLiveVideoStretch            : 0x%08lx\n",pHalInfo->ddCaps.dwMaxLiveVideoStretch);
        printf(" pHalInfo->ddCaps.dwMinHwCodecStretch              : 0x%08lx\n",pHalInfo->ddCaps.dwMinHwCodecStretch);
        printf(" pHalInfo->ddCaps.dwMaxHwCodecStretch              : 0x%08lx\n",pHalInfo->ddCaps.dwMaxHwCodecStretch);
        printf(" pHalInfo->ddCaps.dwReserved1                      : 0x%08lx\n",pHalInfo->ddCaps.dwReserved1);
        printf(" pHalInfo->ddCaps.dwReserved2                      : 0x%08lx\n",pHalInfo->ddCaps.dwReserved2);
        printf(" pHalInfo->ddCaps.dwReserved3                      : 0x%08lx\n",pHalInfo->ddCaps.dwReserved3);

        printf(" pHalInfo->ddCaps.dwSVBCaps                        : ");
        flag = pHalInfo->ddCaps.dwSVBCaps;
        count = 0;
        checkflag(flag,DDCAPS_3D,"DDCAPS_3D");
        checkflag(flag,DDCAPS_ALIGNBOUNDARYDEST,"DDCAPS_ALIGNBOUNDARYDEST");
        checkflag(flag,DDCAPS_ALIGNBOUNDARYSRC,"DDCAPS_ALIGNBOUNDARYSRC");
        checkflag(flag,DDCAPS_ALIGNSIZEDEST,"DDCAPS_ALIGNSIZEDEST");
        checkflag(flag,DDCAPS_ALIGNSIZESRC,"DDCAPS_ALIGNSIZESRC");
        checkflag(flag,DDCAPS_ALIGNSTRIDE,"DDCAPS_ALIGNSTRIDE");
        checkflag(flag,DDCAPS_ALPHA,"DDCAPS_ALPHA");
        checkflag(flag,DDCAPS_BANKSWITCHED,"DDCAPS_BANKSWITCHED");
        checkflag(flag,DDCAPS_BLT,"DDCAPS_BLT");
        checkflag(flag,DDCAPS_BLTCOLORFILL,"DDCAPS_BLTCOLORFILL");
        checkflag(flag,DDCAPS_BLTDEPTHFILL,"DDCAPS_BLTDEPTHFILL");
        checkflag(flag,DDCAPS_BLTFOURCC,"DDCAPS_BLTFOURCC");
        checkflag(flag,DDCAPS_BLTQUEUE,"DDCAPS_BLTQUEUE");
        checkflag(flag,DDCAPS_BLTSTRETCH,"DDCAPS_BLTSTRETCH");
        checkflag(flag,DDCAPS_CANBLTSYSMEM,"DDCAPS_CANBLTSYSMEM");
        checkflag(flag,DDCAPS_CANCLIP,"DDCAPS_CANCLIP");
        checkflag(flag,DDCAPS_CANCLIPSTRETCHED,"DDCAPS_CANCLIPSTRETCHED");
        checkflag(flag,DDCAPS_COLORKEY,"DDCAPS_COLORKEY");
        checkflag(flag,DDCAPS_COLORKEYHWASSIST,"DDCAPS_COLORKEYHWASSIST");
        checkflag(flag,DDCAPS_GDI,"DDCAPS_GDI");
        checkflag(flag,DDCAPS_NOHARDWARE,"DDCAPS_NOHARDWARE");
        checkflag(flag,DDCAPS_OVERLAY,"DDCAPS_OVERLAY");
        checkflag(flag,DDCAPS_OVERLAYCANTCLIP,"DDCAPS_OVERLAYCANTCLIP");
        checkflag(flag,DDCAPS_OVERLAYFOURCC,"DDCAPS_OVERLAYFOURCC");
        checkflag(flag,DDCAPS_OVERLAYSTRETCH,"DDCAPS_OVERLAYSTRETCH");
        checkflag(flag,DDCAPS_PALETTE,"DDCAPS_PALETTE");
        checkflag(flag,DDCAPS_PALETTEVSYNC,"DDCAPS_PALETTEVSYNC");
        checkflag(flag,DDCAPS_READSCANLINE,"DDCAPS_READSCANLINE");
        checkflag(flag,DDCAPS_STEREOVIEW,"DDCAPS_STEREOVIEW");
        checkflag(flag,DDCAPS_VBI,"DDCAPS_VBI");
        checkflag(flag,DDCAPS_ZBLTS,"DDCAPS_ZBLTS");
        checkflag(flag,DDCAPS_ZOVERLAYS,"DDCAPS_ZOVERLAYS");
        endcheckflag(flag,"pHalInfo->ddCaps.dwSVBCaps");

        printf(" pHalInfo->ddCaps.dwSVBCKeyCaps                    : 0x%08lx\n",pHalInfo->ddCaps.dwSVBCKeyCaps);
        printf(" pHalInfo->ddCaps.dwSVBFXCaps                      : 0x%08lx\n",pHalInfo->ddCaps.dwSVBFXCaps);
        for (t=0;t<DD_ROP_SPACE;t++)
        {
        printf(" pHalInfo->ddCaps.dwSVBRops[0x%04x]                : 0x%08lx\n",t,pHalInfo->ddCaps.dwSVBRops[t]);
        }

        printf(" pHalInfo->ddCaps.dwVSBCaps                        : ");
        flag = pHalInfo->ddCaps.dwVSBCaps;
        count = 0;
        checkflag(flag,DDCAPS_3D,"DDCAPS_3D");
        checkflag(flag,DDCAPS_ALIGNBOUNDARYDEST,"DDCAPS_ALIGNBOUNDARYDEST");
        checkflag(flag,DDCAPS_ALIGNBOUNDARYSRC,"DDCAPS_ALIGNBOUNDARYSRC");
        checkflag(flag,DDCAPS_ALIGNSIZEDEST,"DDCAPS_ALIGNSIZEDEST");
        checkflag(flag,DDCAPS_ALIGNSIZESRC,"DDCAPS_ALIGNSIZESRC");
        checkflag(flag,DDCAPS_ALIGNSTRIDE,"DDCAPS_ALIGNSTRIDE");
        checkflag(flag,DDCAPS_ALPHA,"DDCAPS_ALPHA");
        checkflag(flag,DDCAPS_BANKSWITCHED,"DDCAPS_BANKSWITCHED");
        checkflag(flag,DDCAPS_BLT,"DDCAPS_BLT");
        checkflag(flag,DDCAPS_BLTCOLORFILL,"DDCAPS_BLTCOLORFILL");
        checkflag(flag,DDCAPS_BLTDEPTHFILL,"DDCAPS_BLTDEPTHFILL");
        checkflag(flag,DDCAPS_BLTFOURCC,"DDCAPS_BLTFOURCC");
        checkflag(flag,DDCAPS_BLTQUEUE,"DDCAPS_BLTQUEUE");
        checkflag(flag,DDCAPS_BLTSTRETCH,"DDCAPS_BLTSTRETCH");
        checkflag(flag,DDCAPS_CANBLTSYSMEM,"DDCAPS_CANBLTSYSMEM");
        checkflag(flag,DDCAPS_CANCLIP,"DDCAPS_CANCLIP");
        checkflag(flag,DDCAPS_CANCLIPSTRETCHED,"DDCAPS_CANCLIPSTRETCHED");
        checkflag(flag,DDCAPS_COLORKEY,"DDCAPS_COLORKEY");
        checkflag(flag,DDCAPS_COLORKEYHWASSIST,"DDCAPS_COLORKEYHWASSIST");
        checkflag(flag,DDCAPS_GDI,"DDCAPS_GDI");
        checkflag(flag,DDCAPS_NOHARDWARE,"DDCAPS_NOHARDWARE");
        checkflag(flag,DDCAPS_OVERLAY,"DDCAPS_OVERLAY");
        checkflag(flag,DDCAPS_OVERLAYCANTCLIP,"DDCAPS_OVERLAYCANTCLIP");
        checkflag(flag,DDCAPS_OVERLAYFOURCC,"DDCAPS_OVERLAYFOURCC");
        checkflag(flag,DDCAPS_OVERLAYSTRETCH,"DDCAPS_OVERLAYSTRETCH");
        checkflag(flag,DDCAPS_PALETTE,"DDCAPS_PALETTE");
        checkflag(flag,DDCAPS_PALETTEVSYNC,"DDCAPS_PALETTEVSYNC");
        checkflag(flag,DDCAPS_READSCANLINE,"DDCAPS_READSCANLINE");
        checkflag(flag,DDCAPS_STEREOVIEW,"DDCAPS_STEREOVIEW");
        checkflag(flag,DDCAPS_VBI,"DDCAPS_VBI");
        checkflag(flag,DDCAPS_ZBLTS,"DDCAPS_ZBLTS");
        checkflag(flag,DDCAPS_ZOVERLAYS,"DDCAPS_ZOVERLAYS");
        endcheckflag(flag,"pHalInfo->ddCaps.dwVSBCaps");

        printf(" pHalInfo->ddCaps.dwVSBCKeyCaps                    : 0x%08lx\n",pHalInfo->ddCaps.dwVSBCKeyCaps);
        printf(" pHalInfo->ddCaps.dwVSBFXCaps                      : 0x%08lx\n",pHalInfo->ddCaps.dwVSBFXCaps);
        for (t=0;t<DD_ROP_SPACE;t++)
        {
        printf(" pHalInfo->ddCaps.dwVSBRops[0x%04x]                : 0x%08lx\n",t,pHalInfo->ddCaps.dwVSBRops[t]);
        }
        printf(" pHalInfo->ddCaps.dwSSBCaps                        : 0x%08lx\n",pHalInfo->ddCaps.dwSSBCaps);
        printf(" pHalInfo->ddCaps.dwSSBCKeyCa                      : 0x%08lx\n",pHalInfo->ddCaps.dwSSBCKeyCaps);
        printf(" pHalInfo->ddCaps.dwSSBFXCaps                      : 0x%08lx\n",pHalInfo->ddCaps.dwSSBFXCaps);
        for (t=0;t<DD_ROP_SPACE;t++)
        {
        printf(" pHalInfo->ddCaps.dwSSBRops[0x%04x]                : 0x%08lx\n",t,pHalInfo->ddCaps.dwSSBRops[t]);
        }

        printf(" pHalInfo->GetDriverInfo                           : 0x%08lx\n",(long)pHalInfo->GetDriverInfo);
        printf(" pHalInfo->dwFlags                                 : ");

        flag = pHalInfo->dwFlags;
        count = 0;
        checkflag(flag,DDHALINFO_ISPRIMARYDISPLAY,"DDHALINFO_ISPRIMARYDISPLAY");
        checkflag(flag,DDHALINFO_MODEXILLEGAL,"DDHALINFO_MODEXILLEGAL");
        checkflag(flag,DDHALINFO_GETDRIVERINFOSET,"DDHALINFO_GETDRIVERINFOSET");
        checkflag(flag,DDHALINFO_GETDRIVERINFO2,"DDHALINFO_GETDRIVERINFO2");
        endcheckflag(flag,"pHalInfo->dwFlags");

        printf(" pHalInfo->lpD3DGlobalDriverData                   : 0x%08lx\n",(long)pHalInfo->lpD3DGlobalDriverData);
        printf(" pHalInfo->lpD3DHALCallbacks                       : 0x%08lx\n",(long)pHalInfo->lpD3DHALCallbacks);
        printf(" pHalInfo->lpD3DBufCallbacks                       : 0x%08lx\n",(long)pHalInfo->lpD3DBufCallbacks);
    }
    else
    {
        if (pHalInfo->dwSize !=0)
        {
            printf("unkonwn dwSize DD_HALINFO : the size found is 0x%08lx\n",pHalInfo->dwSize);
        }
        else
        {
            printf("none pHalInfo from the driver 0x%08lx\n",pHalInfo->dwSize);
        }
    }
}


void
dump_D3dCallbacks(D3DNTHAL_CALLBACKS *puD3dCallbacks, char *text)
{
    printf("dumping the D3DNTHAL_CALLBACKS from %s\n",text);
    if (puD3dCallbacks->dwSize ==  sizeof(D3DNTHAL_CALLBACKS))
    {
        printf(" puD3dCallbacks->dwSize                                         : 0x%08lx\n",(long)puD3dCallbacks->dwSize);
        printf(" puD3dCallbacks->ContextCreate                                  : 0x%08lx\n",(long)puD3dCallbacks->ContextCreate);
        printf(" puD3dCallbacks->ContextDestroy                                 : 0x%08lx\n",(long)puD3dCallbacks->ContextDestroy);
        printf(" puD3dCallbacks->ContextDestroyAll                              : 0x%08lx\n",(long)puD3dCallbacks->ContextDestroyAll);
        printf(" puD3dCallbacks->SceneCapture                                   : 0x%08lx\n",(long)puD3dCallbacks->SceneCapture);
        printf(" puD3dCallbacks->dwReserved10                                   : 0x%08lx\n",(long)puD3dCallbacks->dwReserved10);
        printf(" puD3dCallbacks->dwReserved11                                   : 0x%08lx\n",(long)puD3dCallbacks->dwReserved11);
        printf(" puD3dCallbacks->dwReserved22                                   : 0x%08lx\n",(long)puD3dCallbacks->dwReserved22);
        printf(" puD3dCallbacks->dwReserved23                                   : 0x%08lx\n",(long)puD3dCallbacks->dwReserved23);
        printf(" puD3dCallbacks->dwReserved                                     : 0x%08lx\n",(long)puD3dCallbacks->dwReserved);
        printf(" puD3dCallbacks->TextureCreate                                  : 0x%08lx\n",(long)puD3dCallbacks->TextureCreate);
        printf(" puD3dCallbacks->TextureDestroy                                 : 0x%08lx\n",(long)puD3dCallbacks->TextureDestroy);
        printf(" puD3dCallbacks->TextureSwap                                    : 0x%08lx\n",(long)puD3dCallbacks->TextureSwap);
        printf(" puD3dCallbacks->TextureGetSurf                                 : 0x%08lx\n",(long)puD3dCallbacks->TextureGetSurf);
        printf(" puD3dCallbacks->dwReserved12                                   : 0x%08lx\n",(long)puD3dCallbacks->dwReserved12);
        printf(" puD3dCallbacks->dwReserved13                                   : 0x%08lx\n",(long)puD3dCallbacks->dwReserved13);
        printf(" puD3dCallbacks->dwReserved14                                   : 0x%08lx\n",(long)puD3dCallbacks->dwReserved14);
        printf(" puD3dCallbacks->dwReserved15                                   : 0x%08lx\n",(long)puD3dCallbacks->dwReserved15);
        printf(" puD3dCallbacks->dwReserved16                                   : 0x%08lx\n",(long)puD3dCallbacks->dwReserved16);
        printf(" puD3dCallbacks->dwReserved17                                   : 0x%08lx\n",(long)puD3dCallbacks->dwReserved17);
        printf(" puD3dCallbacks->dwReserved18                                   : 0x%08lx\n",(long)puD3dCallbacks->dwReserved18);
        printf(" puD3dCallbacks->dwReserved19                                   : 0x%08lx\n",(long)puD3dCallbacks->dwReserved19);
        printf(" puD3dCallbacks->dwReserved20                                   : 0x%08lx\n",(long)puD3dCallbacks->dwReserved20);
        printf(" puD3dCallbacks->dwReserved21                                   : 0x%08lx\n",(long)puD3dCallbacks->dwReserved21);
        printf(" puD3dCallbacks->dwReserved24                                   : 0x%08lx\n",(long)puD3dCallbacks->dwReserved24);
        printf(" puD3dCallbacks->dwReserved0                                    : 0x%08lx\n",(long)puD3dCallbacks->dwReserved0);
        printf(" puD3dCallbacks->dwReserved1                                    : 0x%08lx\n",(long)puD3dCallbacks->dwReserved1);
        printf(" puD3dCallbacks->dwReserved2                                    : 0x%08lx\n",(long)puD3dCallbacks->dwReserved2);
        printf(" puD3dCallbacks->dwReserved3                                    : 0x%08lx\n",(long)puD3dCallbacks->dwReserved3);
        printf(" puD3dCallbacks->dwReserved4                                    : 0x%08lx\n",(long)puD3dCallbacks->dwReserved4);
        printf(" puD3dCallbacks->dwReserved5                                    : 0x%08lx\n",(long)puD3dCallbacks->dwReserved5);
        printf(" puD3dCallbacks->dwReserved6                                    : 0x%08lx\n",(long)puD3dCallbacks->dwReserved6);
        printf(" puD3dCallbacks->dwReserved7                                    : 0x%08lx\n",(long)puD3dCallbacks->dwReserved7);
        printf(" puD3dCallbacks->dwReserved8                                    : 0x%08lx\n",(long)puD3dCallbacks->dwReserved8);
        printf(" puD3dCallbacks->dwReserved9                                    : 0x%08lx\n",(long)puD3dCallbacks->dwReserved9);
    }
     else
    {
        printf("none puD3dCallbacks from the driver 0x%08lx\n",puD3dCallbacks->dwSize);
    }
}


void
dump_D3dDriverData(D3DNTHAL_GLOBALDRIVERDATA *puD3dDriverData, char *text)
{
    UINT flag = 0;
    INT count = 0;

    printf("dumping the D3DNTHAL_GLOBALDRIVERDATA from %s\n",text);
    if (puD3dDriverData->dwSize ==  sizeof(D3DNTHAL_GLOBALDRIVERDATA))
    {
        printf(" puD3dDriverData->dwSize                                        : 0x%08lx\n",(long)puD3dDriverData->dwSize);
        if (puD3dDriverData->hwCaps.dwSize == sizeof(D3DNTHALDEVICEDESC_V1))
        {
            printf(" puD3dDriverData->hwCaps.dwSize                                 : 0x%08lx\n",(long)puD3dDriverData->hwCaps.dwSize);
            printf(" puD3dDriverData->hwCaps.dwFlags                                : ");

            flag = puD3dDriverData->hwCaps.dwFlags;
            checkflag(flag,D3DDD_BCLIPPING,"D3DDD_BCLIPPING");
            checkflag(flag,D3DDD_COLORMODEL,"D3DDD_COLORMODEL");
            checkflag(flag,D3DDD_DEVCAPS,"D3DDD_DEVCAPS");
            checkflag(flag,D3DDD_DEVICERENDERBITDEPTH,"D3DDD_DEVICERENDERBITDEPTH");
            checkflag(flag,D3DDD_DEVICEZBUFFERBITDEPTH,"D3DDD_DEVICEZBUFFERBITDEPTH");
            checkflag(flag,D3DDD_LIGHTINGCAPS,"D3DDD_LIGHTINGCAPS");
            checkflag(flag,D3DDD_LINECAPS,"D3DDD_LINECAPS");
            checkflag(flag,D3DDD_MAXBUFFERSIZE,"D3DDD_MAXBUFFERSIZE");
            checkflag(flag,D3DDD_MAXVERTEXCOUNT,"D3DDD_MAXVERTEXCOUNT");
            checkflag(flag,D3DDD_TRANSFORMCAPS,"D3DDD_TRANSFORMCAPS");
            checkflag(flag,D3DDD_TRICAPS,"D3DDD_TRICAPS");
            endcheckflag(flag,"puD3dDriverData->hwCaps.dwFlags ");

            printf(" puD3dDriverData->hwCaps.dcmColorModel                          : 0x%08lx\n",(long)puD3dDriverData->hwCaps.dcmColorModel);
            printf(" puD3dDriverData->hwCaps.dwDevCaps                              : ");

            count = 0;
            flag = puD3dDriverData->hwCaps.dwDevCaps;
            checkflag(flag,D3DDEVCAPS_CANBLTSYSTONONLOCAL,"D3DDEVCAPS_CANBLTSYSTONONLOCAL");
            checkflag(flag,D3DDEVCAPS_CANRENDERAFTERFLIP,"D3DDEVCAPS_CANRENDERAFTERFLIP");
            checkflag(flag,D3DDEVCAPS_DRAWPRIMITIVES2,"D3DDEVCAPS_DRAWPRIMITIVES2");
            checkflag(flag,D3DDEVCAPS_DRAWPRIMITIVES2EX,"D3DDEVCAPS_DRAWPRIMITIVES2EX");
            checkflag(flag,D3DDEVCAPS_DRAWPRIMTLVERTEX,"D3DDEVCAPS_DRAWPRIMTLVERTEX");
            checkflag(flag,D3DDEVCAPS_EXECUTESYSTEMMEMORY,"D3DDEVCAPS_EXECUTESYSTEMMEMORY");
            checkflag(flag,D3DDEVCAPS_EXECUTEVIDEOMEMORY,"D3DDEVCAPS_EXECUTEVIDEOMEMORY");
            checkflag(flag,D3DDEVCAPS_FLOATTLVERTEX,"D3DDEVCAPS_FLOATTLVERTEX");
            checkflag(flag,D3DDEVCAPS_HWRASTERIZATION,"D3DDEVCAPS_HWRASTERIZATION");
            checkflag(flag,D3DDEVCAPS_HWTRANSFORMANDLIGHT,"D3DDEVCAPS_HWTRANSFORMANDLIGHT");
            checkflag(flag,D3DDEVCAPS_SEPARATETEXTUREMEMORIES,"D3DDEVCAPS_SEPARATETEXTUREMEMORIES");
            checkflag(flag,D3DDEVCAPS_SORTDECREASINGZ,"D3DDEVCAPS_SORTDECREASINGZ");
            checkflag(flag,D3DDEVCAPS_SORTEXACT,"D3DDEVCAPS_SORTEXACT");
            checkflag(flag,D3DDEVCAPS_SORTINCREASINGZ,"D3DDEVCAPS_SORTINCREASINGZ");
            // not in ddk or dxsdk I have but it is msdn checkflag(flag,D3DDEVCAPS_TEXTURENONLOCALVIDEOMEMORY,"D3DDEVCAPS_TEXTURENONLOCALVIDEOMEMORY");
            checkflag(flag,D3DDEVCAPS_TLVERTEXSYSTEMMEMORY,"D3DDEVCAPS_TLVERTEXSYSTEMMEMORY");
            checkflag(flag,D3DDEVCAPS_TLVERTEXVIDEOMEMORY,"D3DDEVCAPS_TLVERTEXVIDEOMEMORY");
            checkflag(flag,D3DDEVCAPS_TEXTURESYSTEMMEMORY,"D3DDEVCAPS_TEXTURESYSTEMMEMORY");
            checkflag(flag,D3DDEVCAPS_TEXTUREVIDEOMEMORY,"D3DDEVCAPS_TEXTUREVIDEOMEMORY");
            endcheckflag(flag,"puD3dDriverData->hwCaps.dwDevCaps");

            if (puD3dDriverData->hwCaps.dtcTransformCaps.dwSize == sizeof(D3DTRANSFORMCAPS))
            {
                printf(" puD3dDriverData->hwCaps.dtcTransformCaps.dwSize                : 0x%08lx\n",(long) puD3dDriverData->hwCaps.dtcTransformCaps.dwSize);
                printf(" puD3dDriverData->hwCaps.dtcTransformCaps.dwCaps                : ");

                count = 0;
                flag = puD3dDriverData->hwCaps.dtcTransformCaps.dwCaps;
                checkflag(flag,D3DTRANSFORMCAPS_CLIP,"D3DTRANSFORMCAPS_CLIP");
                endcheckflag(flag,"puD3dDriverData->hwCaps.dtcTransformCaps.dwCaps");
            }
            else
            {
                printf("none puD3dDriverData->hwCaps.dtcTransformCaps.dwSize from the driver 0x%08lx\n",puD3dDriverData->hwCaps.dtcTransformCaps.dwSize);
            }

            if (puD3dDriverData->hwCaps.dlcLightingCaps.dwSize == sizeof(D3DLIGHTINGCAPS))
            {
                printf(" puD3dDriverData->hwCaps.dlcLightingCaps.dwSize                 : 0x%08lx\n",(long)puD3dDriverData->hwCaps.dlcLightingCaps.dwSize);
                printf(" puD3dDriverData->hwCaps.dlcLightingCaps.dwCaps                 : ");

                count = 0;
                flag = puD3dDriverData->hwCaps.dlcLightingCaps.dwCaps;

                checkflag(flag,D3DLIGHTCAPS_DIRECTIONAL,"D3DLIGHTCAPS_DIRECTIONAL");
                checkflag(flag,D3DLIGHTCAPS_GLSPOT,"D3DLIGHTCAPS_GLSPOT");
                checkflag(flag,D3DLIGHTCAPS_PARALLELPOINT,"D3DLIGHTCAPS_PARALLELPOINT");
                checkflag(flag,D3DLIGHTCAPS_POINT,"D3DLIGHTCAPS_POINT");
                checkflag(flag,D3DLIGHTCAPS_SPOT,"D3DLIGHTCAPS_SPOT");
                endcheckflag(flag,"puD3dDriverData->hwCaps.dlcLightingCaps.dwCaps");

                printf(" puD3dDriverData->hwCaps.dlcLightingCaps.dwLightingModel        : ");

                count = 0;
                flag = puD3dDriverData->hwCaps.dlcLightingCaps.dwLightingModel;

                checkflag(flag,D3DLIGHTINGMODEL_MONO,"D3DLIGHTINGMODEL_MONO");
                checkflag(flag,D3DLIGHTINGMODEL_RGB,"D3DLIGHTINGMODEL_RGB");
                endcheckflag(flag,"puD3dDriverData->hwCaps.dlcLightingCaps.dwLightingModel");

                printf(" puD3dDriverData->hwCaps.dlcLightingCaps.dwNumLights            : 0x%08lx\n",(long)puD3dDriverData->hwCaps.dlcLightingCaps.dwNumLights);
            }
            else
            {
                printf("none puD3dDriverData->hwCaps.dlcLightingCaps.dwSize from the driver 0x%08lx\n",puD3dDriverData->hwCaps.dlcLightingCaps.dwSize);
            }


            if (puD3dDriverData->hwCaps.dpcLineCaps.dwSize == sizeof(D3DPRIMCAPS))
            {
                printf(" puD3dDriverData->hwCaps.dpcLineCaps.dwSize                     : 0x%08lx\n",(long)puD3dDriverData->hwCaps.dpcLineCaps.dwSize);

                printf(" puD3dDriverData->hwCaps.dpcLineCaps.dwMiscCaps                 : ");
                count = 0;
                flag = puD3dDriverData->hwCaps.dpcLineCaps.dwMiscCaps;
                checkflag(flag,D3DPMISCCAPS_CONFORMANT,"D3DPMISCCAPS_CONFORMANT");
                checkflag(flag,D3DPMISCCAPS_CULLCCW,"D3DPMISCCAPS_CULLCCW");
                checkflag(flag,D3DPMISCCAPS_CULLCW,"D3DPMISCCAPS_CULLCW");
                checkflag(flag,D3DPMISCCAPS_CULLNONE,"D3DPMISCCAPS_CULLNONE");
                checkflag(flag,D3DPMISCCAPS_LINEPATTERNREP,"D3DPMISCCAPS_LINEPATTERNREP");
                checkflag(flag,D3DPMISCCAPS_MASKPLANES,"D3DPMISCCAPS_MASKPLANES");
                checkflag(flag,D3DPMISCCAPS_MASKZ,"D3DPMISCCAPS_MASKZ");
                endcheckflag(flag,"puD3dDriverData->hwCaps.dpcLineCaps.dwMiscCaps");

                printf(" puD3dDriverData->hwCaps.dpcLineCaps.dwRasterCaps               : ");
                count = 0;
                flag = puD3dDriverData->hwCaps.dpcLineCaps.dwRasterCaps;
                checkflag(flag,D3DPRASTERCAPS_ANISOTROPY,"D3DPRASTERCAPS_ANISOTROPY");
                checkflag(flag,D3DPRASTERCAPS_ANTIALIASEDGES,"D3DPRASTERCAPS_ANTIALIASEDGES");
                checkflag(flag,D3DPRASTERCAPS_ANTIALIASSORTDEPENDENT,"D3DPRASTERCAPS_ANTIALIASSORTDEPENDENT");
                checkflag(flag,D3DPRASTERCAPS_ANTIALIASSORTINDEPENDENT,"D3DPRASTERCAPS_ANTIALIASSORTINDEPENDENT");
                checkflag(flag,D3DPRASTERCAPS_DITHER,"D3DPRASTERCAPS_DITHER");
                checkflag(flag,D3DPRASTERCAPS_FOGRANGE,"D3DPRASTERCAPS_FOGRANGE");
                checkflag(flag,D3DPRASTERCAPS_FOGTABLE,"D3DPRASTERCAPS_FOGTABLE");
                checkflag(flag,D3DPRASTERCAPS_FOGVERTEX,"D3DPRASTERCAPS_FOGVERTEX");
                checkflag(flag,D3DPRASTERCAPS_PAT,"D3DPRASTERCAPS_PAT");
                checkflag(flag,D3DPRASTERCAPS_ROP2,"D3DPRASTERCAPS_ROP2");
                checkflag(flag,D3DPRASTERCAPS_STIPPLE,"D3DPRASTERCAPS_STIPPLE");
                checkflag(flag,D3DPRASTERCAPS_SUBPIXEL,"D3DPRASTERCAPS_SUBPIXEL");
                checkflag(flag,D3DPRASTERCAPS_SUBPIXELX,"D3DPRASTERCAPS_SUBPIXELX");
                checkflag(flag,D3DPRASTERCAPS_TRANSLUCENTSORTINDEPENDENT,"D3DPRASTERCAPS_TRANSLUCENTSORTINDEPENDENT");
                checkflag(flag,D3DPRASTERCAPS_WBUFFER,"D3DPRASTERCAPS_WBUFFER");
                checkflag(flag,D3DPRASTERCAPS_WFOG,"D3DPRASTERCAPS_WFOG");
                checkflag(flag,D3DPRASTERCAPS_XOR,"D3DPRASTERCAPS_XOR");
                checkflag(flag,D3DPRASTERCAPS_ZBIAS,"D3DPRASTERCAPS_ZBIAS");
                checkflag(flag,D3DPRASTERCAPS_ZBUFFERLESSHSR,"D3DPRASTERCAPS_ZBUFFERLESSHSR");
                checkflag(flag,D3DPRASTERCAPS_ZFOG,"D3DPRASTERCAPS_ZFOG");
                checkflag(flag,D3DPRASTERCAPS_ZTEST,"D3DPRASTERCAPS_ZTEST");
                endcheckflag(flag,"puD3dDriverData->hwCaps.dpcLineCaps.dwRasterCaps");

                printf(" puD3dDriverData->hwCaps.dpcLineCaps.dwZCmpCaps                 : ");
                count = 0;
                flag = puD3dDriverData->hwCaps.dpcLineCaps.dwZCmpCaps;
                checkflag(flag,D3DPCMPCAPS_ALWAYS,"D3DPCMPCAPS_ALWAYS");
                checkflag(flag,D3DPCMPCAPS_EQUAL,"D3DPCMPCAPS_EQUAL");
                checkflag(flag,D3DPCMPCAPS_GREATER,"D3DPCMPCAPS_GREATER");
                checkflag(flag,D3DPCMPCAPS_GREATEREQUAL,"D3DPCMPCAPS_GREATEREQUAL");
                checkflag(flag,D3DPCMPCAPS_LESS,"D3DPCMPCAPS_LESS");
                checkflag(flag,D3DPCMPCAPS_LESSEQUAL,"D3DPCMPCAPS_LESSEQUAL");
                checkflag(flag,D3DPCMPCAPS_NEVER,"D3DPCMPCAPS_NEVER");
                checkflag(flag,D3DPCMPCAPS_NOTEQUAL,"D3DPCMPCAPS_NOTEQUAL");
                endcheckflag(flag,"puD3dDriverData->hwCaps.dpcLineCaps.dwZCmpCaps ");

                printf(" puD3dDriverData->hwCaps.dpcLineCaps.dwSrcBlendCaps             : ");
                count = 0;
                flag = puD3dDriverData->hwCaps.dpcLineCaps.dwSrcBlendCaps;
                checkflag(flag,D3DPBLENDCAPS_BOTHINVSRCALPHA,"D3DPBLENDCAPS_BOTHINVSRCALPHA");
                checkflag(flag,D3DPBLENDCAPS_BOTHSRCALPHA,"D3DPBLENDCAPS_BOTHSRCALPHA");
                checkflag(flag,D3DPBLENDCAPS_DESTALPHA,"D3DPBLENDCAPS_DESTALPHA");
                checkflag(flag,D3DPBLENDCAPS_DESTCOLOR,"D3DPBLENDCAPS_DESTCOLOR");
                checkflag(flag,D3DPBLENDCAPS_INVDESTALPHA,"D3DPBLENDCAPS_INVDESTALPHA");
                checkflag(flag,D3DPBLENDCAPS_INVDESTCOLOR,"D3DPBLENDCAPS_INVDESTCOLOR");
                checkflag(flag,D3DPBLENDCAPS_INVSRCALPHA,"D3DPBLENDCAPS_INVSRCALPHA");
                checkflag(flag,D3DPBLENDCAPS_INVSRCCOLOR,"D3DPBLENDCAPS_INVSRCCOLOR");
                checkflag(flag,D3DPBLENDCAPS_ONE,"D3DPBLENDCAPS_ONE");
                checkflag(flag,D3DPBLENDCAPS_SRCALPHA,"D3DPBLENDCAPS_SRCALPHA");
                checkflag(flag,D3DPBLENDCAPS_SRCALPHASAT,"D3DPBLENDCAPS_SRCALPHASAT");
                checkflag(flag,D3DPBLENDCAPS_SRCCOLOR,"D3DPBLENDCAPS_SRCCOLOR");
                checkflag(flag,D3DPBLENDCAPS_ZERO,"D3DPBLENDCAPS_ZERO");
                endcheckflag(flag,"puD3dDriverData->hwCaps.dpcLineCaps.dwSrcBlendCaps ");

                printf(" puD3dDriverData->hwCaps.dpcLineCaps.dwDestBlendCaps            : ");
                count = 0;
                flag = puD3dDriverData->hwCaps.dpcLineCaps.dwDestBlendCaps;
                checkflag(flag,D3DPBLENDCAPS_BOTHINVSRCALPHA,"D3DPBLENDCAPS_BOTHINVSRCALPHA");
                checkflag(flag,D3DPBLENDCAPS_BOTHSRCALPHA,"D3DPBLENDCAPS_BOTHSRCALPHA");
                checkflag(flag,D3DPBLENDCAPS_DESTALPHA,"D3DPBLENDCAPS_DESTALPHA");
                checkflag(flag,D3DPBLENDCAPS_DESTCOLOR,"D3DPBLENDCAPS_DESTCOLOR");
                checkflag(flag,D3DPBLENDCAPS_INVDESTALPHA,"D3DPBLENDCAPS_INVDESTALPHA");
                checkflag(flag,D3DPBLENDCAPS_INVDESTCOLOR,"D3DPBLENDCAPS_INVDESTCOLOR");
                checkflag(flag,D3DPBLENDCAPS_INVSRCALPHA,"D3DPBLENDCAPS_INVSRCALPHA");
                checkflag(flag,D3DPBLENDCAPS_INVSRCCOLOR,"D3DPBLENDCAPS_INVSRCCOLOR");
                checkflag(flag,D3DPBLENDCAPS_ONE,"D3DPBLENDCAPS_ONE");
                checkflag(flag,D3DPBLENDCAPS_SRCALPHA,"D3DPBLENDCAPS_SRCALPHA");
                checkflag(flag,D3DPBLENDCAPS_SRCALPHASAT,"D3DPBLENDCAPS_SRCALPHASAT");
                checkflag(flag,D3DPBLENDCAPS_SRCCOLOR,"D3DPBLENDCAPS_SRCCOLOR");
                checkflag(flag,D3DPBLENDCAPS_ZERO,"D3DPBLENDCAPS_ZERO");
                endcheckflag(flag,"puD3dDriverData->hwCaps.dpcLineCaps.dwDestBlendCaps ");

                printf(" puD3dDriverData->hwCaps.dpcLineCaps.dwAlphaCmpCaps             : ");
                count = 0;
                flag = puD3dDriverData->hwCaps.dpcLineCaps.dwAlphaCmpCaps;
                checkflag(flag,D3DPCMPCAPS_ALWAYS,"D3DPCMPCAPS_ALWAYS");
                checkflag(flag,D3DPCMPCAPS_EQUAL,"D3DPCMPCAPS_EQUAL");
                checkflag(flag,D3DPCMPCAPS_GREATER,"D3DPCMPCAPS_GREATER");
                checkflag(flag,D3DPCMPCAPS_GREATEREQUAL,"D3DPCMPCAPS_GREATEREQUAL");
                checkflag(flag,D3DPCMPCAPS_LESS,"D3DPCMPCAPS_LESS");
                checkflag(flag,D3DPCMPCAPS_LESSEQUAL,"D3DPCMPCAPS_LESSEQUAL");
                checkflag(flag,D3DPCMPCAPS_NEVER,"D3DPCMPCAPS_NEVER");
                checkflag(flag,D3DPCMPCAPS_NOTEQUAL,"D3DPCMPCAPS_NOTEQUAL");
                endcheckflag(flag,"puD3dDriverData->hwCaps.dpcLineCaps.dwAlphaCmpCaps ");

                printf(" puD3dDriverData->hwCaps.dpcLineCaps.dwShadeCaps                : ");
                count = 0;
                flag = puD3dDriverData->hwCaps.dpcLineCaps.dwShadeCaps;
                checkflag(flag,D3DPSHADECAPS_ALPHAFLATBLEND,"D3DPSHADECAPS_ALPHAFLATBLEND");
                checkflag(flag,D3DPSHADECAPS_ALPHAFLATSTIPPLED,"D3DPSHADECAPS_ALPHAFLATSTIPPLED");
                checkflag(flag,D3DPSHADECAPS_ALPHAGOURAUDBLEND,"D3DPSHADECAPS_ALPHAGOURAUDBLEND");
                checkflag(flag,D3DPSHADECAPS_ALPHAGOURAUDSTIPPLED,"D3DPSHADECAPS_ALPHAGOURAUDSTIPPLED");
                checkflag(flag,D3DPSHADECAPS_ALPHAPHONGBLEND,"D3DPSHADECAPS_ALPHAPHONGBLEND");
                checkflag(flag,D3DPSHADECAPS_ALPHAPHONGSTIPPLED,"D3DPSHADECAPS_ALPHAPHONGSTIPPLED");
                checkflag(flag,D3DPSHADECAPS_COLORFLATMONO,"D3DPSHADECAPS_COLORFLATMONO");
                checkflag(flag,D3DPSHADECAPS_COLORFLATRGB,"D3DPSHADECAPS_COLORFLATRGB");
                checkflag(flag,D3DPSHADECAPS_COLORGOURAUDMONO,"D3DPSHADECAPS_COLORGOURAUDMONO");
                checkflag(flag,D3DPSHADECAPS_COLORGOURAUDRGB,"D3DPSHADECAPS_COLORGOURAUDRGB");
                checkflag(flag,D3DPSHADECAPS_COLORPHONGMONO,"D3DPSHADECAPS_COLORPHONGMONO");
                checkflag(flag,D3DPSHADECAPS_COLORPHONGRGB,"D3DPSHADECAPS_COLORPHONGRGB");
                checkflag(flag,D3DPSHADECAPS_FOGFLAT,"D3DPSHADECAPS_FOGFLAT");
                checkflag(flag,D3DPSHADECAPS_FOGGOURAUD,"D3DPSHADECAPS_FOGGOURAUD");
                checkflag(flag,D3DPSHADECAPS_FOGPHONG,"D3DPSHADECAPS_FOGPHONG");
                checkflag(flag,D3DPSHADECAPS_SPECULARFLATMONO,"D3DPSHADECAPS_SPECULARFLATMONO");
                checkflag(flag,D3DPSHADECAPS_SPECULARFLATRGB,"D3DPSHADECAPS_SPECULARFLATRGB");
                checkflag(flag,D3DPSHADECAPS_SPECULARGOURAUDMONO,"D3DPSHADECAPS_SPECULARGOURAUDMONO");
                checkflag(flag,D3DPSHADECAPS_SPECULARGOURAUDRGB,"D3DPSHADECAPS_SPECULARGOURAUDRGB");
                checkflag(flag,D3DPSHADECAPS_SPECULARPHONGMONO,"D3DPSHADECAPS_SPECULARPHONGMONO");
                checkflag(flag,D3DPSHADECAPS_SPECULARPHONGRGB,"D3DPSHADECAPS_SPECULARPHONGRGB");
                endcheckflag(flag,"puD3dDriverData->hwCaps.dpcLineCaps.dwShadeCaps ");

                printf(" puD3dDriverData->hwCaps.dpcLineCaps.dwTextureCaps              : ");
                count = 0;
                flag = puD3dDriverData->hwCaps.dpcLineCaps.dwTextureCaps;
                checkflag(flag,D3DPTEXTURECAPS_ALPHA,"D3DPTEXTURECAPS_ALPHA");
                checkflag(flag,D3DPTEXTURECAPS_ALPHAPALETTE,"D3DPTEXTURECAPS_ALPHAPALETTE");
                checkflag(flag,D3DPTEXTURECAPS_BORDER,"D3DPTEXTURECAPS_BORDER");
                checkflag(flag,D3DPTEXTURECAPS_COLORKEYBLEND,"D3DPTEXTURECAPS_COLORKEYBLEND");
                checkflag(flag,D3DPTEXTURECAPS_CUBEMAP,"D3DPTEXTURECAPS_CUBEMAP");
                checkflag(flag,D3DPTEXTURECAPS_PERSPECTIVE,"D3DPTEXTURECAPS_PERSPECTIVE");
                checkflag(flag,D3DPTEXTURECAPS_POW2,"D3DPTEXTURECAPS_POW2");
                checkflag(flag,D3DPTEXTURECAPS_PROJECTED,"D3DPTEXTURECAPS_PROJECTED");
                checkflag(flag,D3DPTEXTURECAPS_NONPOW2CONDITIONAL,"D3DPTEXTURECAPS_NONPOW2CONDITIONAL");
                checkflag(flag,D3DPTEXTURECAPS_SQUAREONLY,"D3DPTEXTURECAPS_SQUAREONLY");
                // not in ddk or dxsdk but it is in msdn checkflag(flag,D3DPTEXTURECAPS_TEXREPEATNOTSCALESBYSIZE,"D3DPTEXTURECAPS_TEXREPEATNOTSCALESBYSIZE");
                // not in ddk or dxsdk but it is in msdn checkflag(flag,D3DPTEXTURECAPS_TEXTURETRANSFORM,"D3DPTEXTURECAPS_TEXTURETRANSFORM");
                checkflag(flag,D3DPTEXTURECAPS_TRANSPARENCY,"D3DPTEXTURECAPS_TRANSPARENCY");
                endcheckflag(flag,"puD3dDriverData->hwCaps.dpcLineCaps.dwTextureCaps ");

                printf(" puD3dDriverData->hwCaps.dpcLineCaps.dwTextureFilterCaps        : ");
                count = 0;
                flag = puD3dDriverData->hwCaps.dpcLineCaps.dwTextureFilterCaps;
                checkflag(flag,D3DPTFILTERCAPS_LINEAR,"D3DPTFILTERCAPS_LINEAR");
                checkflag(flag,D3DPTFILTERCAPS_LINEARMIPLINEAR,"D3DPTFILTERCAPS_LINEARMIPLINEAR");
                checkflag(flag,D3DPTFILTERCAPS_LINEARMIPNEAREST,"D3DPTFILTERCAPS_LINEARMIPNEAREST");
                checkflag(flag,D3DPTFILTERCAPS_MAGFAFLATCUBIC,"D3DPTFILTERCAPS_MAGFAFLATCUBIC");
                checkflag(flag,D3DPTFILTERCAPS_MAGFANISOTROPIC,"D3DPTFILTERCAPS_MAGFANISOTROPIC");
                checkflag(flag,D3DPTFILTERCAPS_MAGFGAUSSIANCUBIC,"D3DPTFILTERCAPS_MAGFGAUSSIANCUBIC");
                checkflag(flag,D3DPTFILTERCAPS_MAGFLINEAR,"D3DPTFILTERCAPS_MAGFLINEAR");
                checkflag(flag,D3DPTFILTERCAPS_MAGFPOINT,"D3DPTFILTERCAPS_MAGFPOINT");
                checkflag(flag,D3DPTFILTERCAPS_MINFANISOTROPIC,"D3DPTFILTERCAPS_MINFANISOTROPIC");
                checkflag(flag,D3DPTFILTERCAPS_MINFLINEAR,"D3DPTFILTERCAPS_MINFLINEAR");
                checkflag(flag,D3DPTFILTERCAPS_MINFPOINT,"D3DPTFILTERCAPS_MINFPOINT");
                checkflag(flag,D3DPTFILTERCAPS_MIPFLINEAR,"D3DPTFILTERCAPS_MIPFLINEAR");
                checkflag(flag,D3DPTFILTERCAPS_MIPFPOINT,"D3DPTFILTERCAPS_MIPFPOINT");
                checkflag(flag,D3DPTFILTERCAPS_MIPLINEAR,"D3DPTFILTERCAPS_MIPLINEAR");
                checkflag(flag,D3DPTFILTERCAPS_MIPNEAREST,"D3DPTFILTERCAPS_MIPNEAREST");
                checkflag(flag,D3DPTFILTERCAPS_NEAREST,"D3DPTFILTERCAPS_NEAREST");
                endcheckflag(flag,"puD3dDriverData->hwCaps.dpcLineCaps.dwTextureFilterCaps ");

                printf(" puD3dDriverData->hwCaps.dpcLineCaps.dwTextureBlendCaps         : ");
                count = 0;
                flag = puD3dDriverData->hwCaps.dpcLineCaps.dwTextureBlendCaps;
                checkflag(flag,D3DPTBLENDCAPS_ADD,"D3DPTBLENDCAPS_ADD");
                checkflag(flag,D3DPTBLENDCAPS_COPY,"D3DPTBLENDCAPS_COPY");
                checkflag(flag,D3DPTBLENDCAPS_DECAL,"D3DPTBLENDCAPS_DECAL");
                checkflag(flag,D3DPTBLENDCAPS_DECALALPHA,"D3DPTBLENDCAPS_DECALALPHA");
                checkflag(flag,D3DPTBLENDCAPS_DECALMASK,"D3DPTBLENDCAPS_DECALMASK");
                checkflag(flag,D3DPTBLENDCAPS_MODULATE,"D3DPTBLENDCAPS_MODULATE");
                checkflag(flag,D3DPTBLENDCAPS_MODULATEALPHA,"D3DPTBLENDCAPS_MODULATEALPHA");
                checkflag(flag,D3DPTBLENDCAPS_MODULATEMASK,"D3DPTBLENDCAPS_MODULATEMASK");
                endcheckflag(flag,"puD3dDriverData->hwCaps.dpcLineCaps.dwTextureBlendCaps ");

                printf(" puD3dDriverData->hwCaps.dpcLineCaps.dwTextureAddressCaps       : ");
                count = 0;
                flag = puD3dDriverData->hwCaps.dpcLineCaps.dwTextureAddressCaps;
                checkflag(flag,D3DPTADDRESSCAPS_BORDER,"D3DPTADDRESSCAPS_BORDER");
                checkflag(flag,D3DPTADDRESSCAPS_CLAMP,"D3DPTADDRESSCAPS_CLAMP");
                checkflag(flag,D3DPTADDRESSCAPS_INDEPENDENTUV,"D3DPTADDRESSCAPS_INDEPENDENTUV");
                checkflag(flag,D3DPTADDRESSCAPS_MIRROR,"D3DPTADDRESSCAPS_MIRROR");
                checkflag(flag,D3DPTADDRESSCAPS_WRAP,"D3DPTADDRESSCAPS_WRAP");
                endcheckflag(flag,"puD3dDriverData->hwCaps.dpcLineCaps.dwTextureAddressCaps ");

                printf(" puD3dDriverData->hwCaps.dpcLineCaps.dwStippleWidth             : 0x%08lx\n",(long)puD3dDriverData->hwCaps.dpcLineCaps.dwStippleWidth);
                printf(" puD3dDriverData->hwCaps.dpcLineCaps.dwStippleHeight            : 0x%08lx\n",(long)puD3dDriverData->hwCaps.dpcLineCaps.dwStippleHeight);
            }
            else
            {
                printf("none puD3dDriverData->hwCaps.dpcLineCaps.dwSize from the driver 0x%08lx\n",puD3dDriverData->hwCaps.dpcLineCaps.dwSize);
            }

            if (puD3dDriverData->hwCaps.dpcTriCaps.dwSize == sizeof(D3DPRIMCAPS))
            {
                printf(" puD3dDriverData->hwCaps.dpcTriCaps.dwSize                      : 0x%08lx\n",(long)puD3dDriverData->hwCaps.dpcTriCaps.dwSize);

                printf(" puD3dDriverData->hwCaps.dpcTriCaps.dwMiscCaps                  : ");
                count = 0;
                flag = puD3dDriverData->hwCaps.dpcTriCaps.dwMiscCaps;
                checkflag(flag,D3DPMISCCAPS_CONFORMANT,"D3DPMISCCAPS_CONFORMANT");
                checkflag(flag,D3DPMISCCAPS_CULLCCW,"D3DPMISCCAPS_CULLCCW");
                checkflag(flag,D3DPMISCCAPS_CULLCW,"D3DPMISCCAPS_CULLCW");
                checkflag(flag,D3DPMISCCAPS_CULLNONE,"D3DPMISCCAPS_CULLNONE");
                checkflag(flag,D3DPMISCCAPS_LINEPATTERNREP,"D3DPMISCCAPS_LINEPATTERNREP");
                checkflag(flag,D3DPMISCCAPS_MASKPLANES,"D3DPMISCCAPS_MASKPLANES");
                checkflag(flag,D3DPMISCCAPS_MASKZ,"D3DPMISCCAPS_MASKZ");
                endcheckflag(flag,"puD3dDriverData->hwCaps.dpcTriCaps.dwMiscCaps");

                printf(" puD3dDriverData->hwCaps.dpcTriCaps.dwRasterCaps                : ");
                count = 0;
                flag = puD3dDriverData->hwCaps.dpcTriCaps.dwRasterCaps;
                checkflag(flag,D3DPRASTERCAPS_ANISOTROPY,"D3DPRASTERCAPS_ANISOTROPY");
                checkflag(flag,D3DPRASTERCAPS_ANTIALIASEDGES,"D3DPRASTERCAPS_ANTIALIASEDGES");
                checkflag(flag,D3DPRASTERCAPS_ANTIALIASSORTDEPENDENT,"D3DPRASTERCAPS_ANTIALIASSORTDEPENDENT");
                checkflag(flag,D3DPRASTERCAPS_ANTIALIASSORTINDEPENDENT,"D3DPRASTERCAPS_ANTIALIASSORTINDEPENDENT");
                checkflag(flag,D3DPRASTERCAPS_DITHER,"D3DPRASTERCAPS_DITHER");
                checkflag(flag,D3DPRASTERCAPS_FOGRANGE,"D3DPRASTERCAPS_FOGRANGE");
                checkflag(flag,D3DPRASTERCAPS_FOGTABLE,"D3DPRASTERCAPS_FOGTABLE");
                checkflag(flag,D3DPRASTERCAPS_FOGVERTEX,"D3DPRASTERCAPS_FOGVERTEX");
                checkflag(flag,D3DPRASTERCAPS_PAT,"D3DPRASTERCAPS_PAT");
                checkflag(flag,D3DPRASTERCAPS_ROP2,"D3DPRASTERCAPS_ROP2");
                checkflag(flag,D3DPRASTERCAPS_STIPPLE,"D3DPRASTERCAPS_STIPPLE");
                checkflag(flag,D3DPRASTERCAPS_SUBPIXEL,"D3DPRASTERCAPS_SUBPIXEL");
                checkflag(flag,D3DPRASTERCAPS_SUBPIXELX,"D3DPRASTERCAPS_SUBPIXELX");
                checkflag(flag,D3DPRASTERCAPS_TRANSLUCENTSORTINDEPENDENT,"D3DPRASTERCAPS_TRANSLUCENTSORTINDEPENDENT");
                checkflag(flag,D3DPRASTERCAPS_WBUFFER,"D3DPRASTERCAPS_WBUFFER");
                checkflag(flag,D3DPRASTERCAPS_WFOG,"D3DPRASTERCAPS_WFOG");
                checkflag(flag,D3DPRASTERCAPS_XOR,"D3DPRASTERCAPS_XOR");
                checkflag(flag,D3DPRASTERCAPS_ZBIAS,"D3DPRASTERCAPS_ZBIAS");
                checkflag(flag,D3DPRASTERCAPS_ZBUFFERLESSHSR,"D3DPRASTERCAPS_ZBUFFERLESSHSR");
                checkflag(flag,D3DPRASTERCAPS_ZFOG,"D3DPRASTERCAPS_ZFOG");
                checkflag(flag,D3DPRASTERCAPS_ZTEST,"D3DPRASTERCAPS_ZTEST");
                endcheckflag(flag,"puD3dDriverData->hwCaps.dpcTriCaps.dwRasterCaps");

                printf(" puD3dDriverData->hwCaps.dpcTriCaps.dwZCmpCaps                  : ");
                count = 0;
                flag = puD3dDriverData->hwCaps.dpcTriCaps.dwZCmpCaps;
                checkflag(flag,D3DPCMPCAPS_ALWAYS,"D3DPCMPCAPS_ALWAYS");
                checkflag(flag,D3DPCMPCAPS_EQUAL,"D3DPCMPCAPS_EQUAL");
                checkflag(flag,D3DPCMPCAPS_GREATER,"D3DPCMPCAPS_GREATER");
                checkflag(flag,D3DPCMPCAPS_GREATEREQUAL,"D3DPCMPCAPS_GREATEREQUAL");
                checkflag(flag,D3DPCMPCAPS_LESS,"D3DPCMPCAPS_LESS");
                checkflag(flag,D3DPCMPCAPS_LESSEQUAL,"D3DPCMPCAPS_LESSEQUAL");
                checkflag(flag,D3DPCMPCAPS_NEVER,"D3DPCMPCAPS_NEVER");
                checkflag(flag,D3DPCMPCAPS_NOTEQUAL,"D3DPCMPCAPS_NOTEQUAL");
                endcheckflag(flag,"puD3dDriverData->hwCaps.dpcTriCaps.dwZCmpCaps ");

                printf(" puD3dDriverData->hwCaps.dpcTriCaps.dwSrcBlendCaps              : ");
                count = 0;
                flag = puD3dDriverData->hwCaps.dpcTriCaps.dwSrcBlendCaps;
                checkflag(flag,D3DPBLENDCAPS_BOTHINVSRCALPHA,"D3DPBLENDCAPS_BOTHINVSRCALPHA");
                checkflag(flag,D3DPBLENDCAPS_BOTHSRCALPHA,"D3DPBLENDCAPS_BOTHSRCALPHA");
                checkflag(flag,D3DPBLENDCAPS_DESTALPHA,"D3DPBLENDCAPS_DESTALPHA");
                checkflag(flag,D3DPBLENDCAPS_DESTCOLOR,"D3DPBLENDCAPS_DESTCOLOR");
                checkflag(flag,D3DPBLENDCAPS_INVDESTALPHA,"D3DPBLENDCAPS_INVDESTALPHA");
                checkflag(flag,D3DPBLENDCAPS_INVDESTCOLOR,"D3DPBLENDCAPS_INVDESTCOLOR");
                checkflag(flag,D3DPBLENDCAPS_INVSRCALPHA,"D3DPBLENDCAPS_INVSRCALPHA");
                checkflag(flag,D3DPBLENDCAPS_INVSRCCOLOR,"D3DPBLENDCAPS_INVSRCCOLOR");
                checkflag(flag,D3DPBLENDCAPS_ONE,"D3DPBLENDCAPS_ONE");
                checkflag(flag,D3DPBLENDCAPS_SRCALPHA,"D3DPBLENDCAPS_SRCALPHA");
                checkflag(flag,D3DPBLENDCAPS_SRCALPHASAT,"D3DPBLENDCAPS_SRCALPHASAT");
                checkflag(flag,D3DPBLENDCAPS_SRCCOLOR,"D3DPBLENDCAPS_SRCCOLOR");
                checkflag(flag,D3DPBLENDCAPS_ZERO,"D3DPBLENDCAPS_ZERO");
                endcheckflag(flag,"puD3dDriverData->hwCaps.dpcTriCaps.dwSrcBlendCaps ");

                printf(" puD3dDriverData->hwCaps.dpcTriCaps.dwDestBlendCaps             : ");
                count = 0;
                flag = puD3dDriverData->hwCaps.dpcTriCaps.dwDestBlendCaps;
                checkflag(flag,D3DPBLENDCAPS_BOTHINVSRCALPHA,"D3DPBLENDCAPS_BOTHINVSRCALPHA");
                checkflag(flag,D3DPBLENDCAPS_BOTHSRCALPHA,"D3DPBLENDCAPS_BOTHSRCALPHA");
                checkflag(flag,D3DPBLENDCAPS_DESTALPHA,"D3DPBLENDCAPS_DESTALPHA");
                checkflag(flag,D3DPBLENDCAPS_DESTCOLOR,"D3DPBLENDCAPS_DESTCOLOR");
                checkflag(flag,D3DPBLENDCAPS_INVDESTALPHA,"D3DPBLENDCAPS_INVDESTALPHA");
                checkflag(flag,D3DPBLENDCAPS_INVDESTCOLOR,"D3DPBLENDCAPS_INVDESTCOLOR");
                checkflag(flag,D3DPBLENDCAPS_INVSRCALPHA,"D3DPBLENDCAPS_INVSRCALPHA");
                checkflag(flag,D3DPBLENDCAPS_INVSRCCOLOR,"D3DPBLENDCAPS_INVSRCCOLOR");
                checkflag(flag,D3DPBLENDCAPS_ONE,"D3DPBLENDCAPS_ONE");
                checkflag(flag,D3DPBLENDCAPS_SRCALPHA,"D3DPBLENDCAPS_SRCALPHA");
                checkflag(flag,D3DPBLENDCAPS_SRCALPHASAT,"D3DPBLENDCAPS_SRCALPHASAT");
                checkflag(flag,D3DPBLENDCAPS_SRCCOLOR,"D3DPBLENDCAPS_SRCCOLOR");
                checkflag(flag,D3DPBLENDCAPS_ZERO,"D3DPBLENDCAPS_ZERO");
                endcheckflag(flag,"puD3dDriverData->hwCaps.dpcTriCaps.dwDestBlendCaps ");

                printf(" puD3dDriverData->hwCaps.dpcTriCaps.dwAlphaCmpCaps              : ");
                count = 0;
                flag = puD3dDriverData->hwCaps.dpcTriCaps.dwAlphaCmpCaps;
                checkflag(flag,D3DPCMPCAPS_ALWAYS,"D3DPCMPCAPS_ALWAYS");
                checkflag(flag,D3DPCMPCAPS_EQUAL,"D3DPCMPCAPS_EQUAL");
                checkflag(flag,D3DPCMPCAPS_GREATER,"D3DPCMPCAPS_GREATER");
                checkflag(flag,D3DPCMPCAPS_GREATEREQUAL,"D3DPCMPCAPS_GREATEREQUAL");
                checkflag(flag,D3DPCMPCAPS_LESS,"D3DPCMPCAPS_LESS");
                checkflag(flag,D3DPCMPCAPS_LESSEQUAL,"D3DPCMPCAPS_LESSEQUAL");
                checkflag(flag,D3DPCMPCAPS_NEVER,"D3DPCMPCAPS_NEVER");
                checkflag(flag,D3DPCMPCAPS_NOTEQUAL,"D3DPCMPCAPS_NOTEQUAL");
                endcheckflag(flag,"puD3dDriverData->hwCaps.dpcTriCaps.dwAlphaCmpCaps ");

                printf(" puD3dDriverData->hwCaps.dpcTriCaps.dwShadeCaps                 : ");
                count = 0;
                flag = puD3dDriverData->hwCaps.dpcTriCaps.dwShadeCaps;
                checkflag(flag,D3DPSHADECAPS_ALPHAFLATBLEND,"D3DPSHADECAPS_ALPHAFLATBLEND");
                checkflag(flag,D3DPSHADECAPS_ALPHAFLATSTIPPLED,"D3DPSHADECAPS_ALPHAFLATSTIPPLED");
                checkflag(flag,D3DPSHADECAPS_ALPHAGOURAUDBLEND,"D3DPSHADECAPS_ALPHAGOURAUDBLEND");
                checkflag(flag,D3DPSHADECAPS_ALPHAGOURAUDSTIPPLED,"D3DPSHADECAPS_ALPHAGOURAUDSTIPPLED");
                checkflag(flag,D3DPSHADECAPS_ALPHAPHONGBLEND,"D3DPSHADECAPS_ALPHAPHONGBLEND");
                checkflag(flag,D3DPSHADECAPS_ALPHAPHONGSTIPPLED,"D3DPSHADECAPS_ALPHAPHONGSTIPPLED");
                checkflag(flag,D3DPSHADECAPS_COLORFLATMONO,"D3DPSHADECAPS_COLORFLATMONO");
                checkflag(flag,D3DPSHADECAPS_COLORFLATRGB,"D3DPSHADECAPS_COLORFLATRGB");
                checkflag(flag,D3DPSHADECAPS_COLORGOURAUDMONO,"D3DPSHADECAPS_COLORGOURAUDMONO");
                checkflag(flag,D3DPSHADECAPS_COLORGOURAUDRGB,"D3DPSHADECAPS_COLORGOURAUDRGB");
                checkflag(flag,D3DPSHADECAPS_COLORPHONGMONO,"D3DPSHADECAPS_COLORPHONGMONO");
                checkflag(flag,D3DPSHADECAPS_COLORPHONGRGB,"D3DPSHADECAPS_COLORPHONGRGB");
                checkflag(flag,D3DPSHADECAPS_FOGFLAT,"D3DPSHADECAPS_FOGFLAT");
                checkflag(flag,D3DPSHADECAPS_FOGGOURAUD,"D3DPSHADECAPS_FOGGOURAUD");
                checkflag(flag,D3DPSHADECAPS_FOGPHONG,"D3DPSHADECAPS_FOGPHONG");
                checkflag(flag,D3DPSHADECAPS_SPECULARFLATMONO,"D3DPSHADECAPS_SPECULARFLATMONO");
                checkflag(flag,D3DPSHADECAPS_SPECULARFLATRGB,"D3DPSHADECAPS_SPECULARFLATRGB");
                checkflag(flag,D3DPSHADECAPS_SPECULARGOURAUDMONO,"D3DPSHADECAPS_SPECULARGOURAUDMONO");
                checkflag(flag,D3DPSHADECAPS_SPECULARGOURAUDRGB,"D3DPSHADECAPS_SPECULARGOURAUDRGB");
                checkflag(flag,D3DPSHADECAPS_SPECULARPHONGMONO,"D3DPSHADECAPS_SPECULARPHONGMONO");
                checkflag(flag,D3DPSHADECAPS_SPECULARPHONGRGB,"D3DPSHADECAPS_SPECULARPHONGRGB");
                endcheckflag(flag,"puD3dDriverData->hwCaps.dpcTriCaps.dwShadeCaps ");

                printf(" puD3dDriverData->hwCaps.dpcTriCaps.dwTextureCaps              : ");
                count = 0;
                flag = puD3dDriverData->hwCaps.dpcTriCaps.dwTextureCaps;
                checkflag(flag,D3DPTEXTURECAPS_ALPHA,"D3DPTEXTURECAPS_ALPHA");
                checkflag(flag,D3DPTEXTURECAPS_ALPHAPALETTE,"D3DPTEXTURECAPS_ALPHAPALETTE");
                checkflag(flag,D3DPTEXTURECAPS_BORDER,"D3DPTEXTURECAPS_BORDER");
                checkflag(flag,D3DPTEXTURECAPS_COLORKEYBLEND,"D3DPTEXTURECAPS_COLORKEYBLEND");
                checkflag(flag,D3DPTEXTURECAPS_CUBEMAP,"D3DPTEXTURECAPS_CUBEMAP");
                checkflag(flag,D3DPTEXTURECAPS_PERSPECTIVE,"D3DPTEXTURECAPS_PERSPECTIVE");
                checkflag(flag,D3DPTEXTURECAPS_POW2,"D3DPTEXTURECAPS_POW2");
                checkflag(flag,D3DPTEXTURECAPS_PROJECTED,"D3DPTEXTURECAPS_PROJECTED");
                checkflag(flag,D3DPTEXTURECAPS_NONPOW2CONDITIONAL,"D3DPTEXTURECAPS_NONPOW2CONDITIONAL");
                checkflag(flag,D3DPTEXTURECAPS_SQUAREONLY,"D3DPTEXTURECAPS_SQUAREONLY");
                //not in ddk or dxsdk but it is in msdn  checkflag(flag,D3DPTEXTURECAPS_TEXREPEATNOTSCALESBYSIZE,"D3DPTEXTURECAPS_TEXREPEATNOTSCALESBYSIZE");
                //not in ddk or dxsdk but it is in msdn  checkflag(flag,D3DPTEXTURECAPS_TEXTURETRANSFORM,"D3DPTEXTURECAPS_TEXTURETRANSFORM");
                checkflag(flag,D3DPTEXTURECAPS_TRANSPARENCY,"D3DPTEXTURECAPS_TRANSPARENCY");
                endcheckflag(flag,"puD3dDriverData->hwCaps.dpcTriCaps.dwTextureCaps ");

                printf(" puD3dDriverData->hwCaps.dpcTriCaps.dwTextureFilterCaps         : ");
                count = 0;
                flag = puD3dDriverData->hwCaps.dpcTriCaps.dwTextureFilterCaps;
                checkflag(flag,D3DPTFILTERCAPS_LINEAR,"D3DPTFILTERCAPS_LINEAR");
                checkflag(flag,D3DPTFILTERCAPS_LINEARMIPLINEAR,"D3DPTFILTERCAPS_LINEARMIPLINEAR");
                checkflag(flag,D3DPTFILTERCAPS_LINEARMIPNEAREST,"D3DPTFILTERCAPS_LINEARMIPNEAREST");
                checkflag(flag,D3DPTFILTERCAPS_MAGFAFLATCUBIC,"D3DPTFILTERCAPS_MAGFAFLATCUBIC");
                checkflag(flag,D3DPTFILTERCAPS_MAGFANISOTROPIC,"D3DPTFILTERCAPS_MAGFANISOTROPIC");
                checkflag(flag,D3DPTFILTERCAPS_MAGFGAUSSIANCUBIC,"D3DPTFILTERCAPS_MAGFGAUSSIANCUBIC");
                checkflag(flag,D3DPTFILTERCAPS_MAGFLINEAR,"D3DPTFILTERCAPS_MAGFLINEAR");
                checkflag(flag,D3DPTFILTERCAPS_MAGFPOINT,"D3DPTFILTERCAPS_MAGFPOINT");
                checkflag(flag,D3DPTFILTERCAPS_MINFANISOTROPIC,"D3DPTFILTERCAPS_MINFANISOTROPIC");
                checkflag(flag,D3DPTFILTERCAPS_MINFLINEAR,"D3DPTFILTERCAPS_MINFLINEAR");
                checkflag(flag,D3DPTFILTERCAPS_MINFPOINT,"D3DPTFILTERCAPS_MINFPOINT");
                checkflag(flag,D3DPTFILTERCAPS_MIPFLINEAR,"D3DPTFILTERCAPS_MIPFLINEAR");
                checkflag(flag,D3DPTFILTERCAPS_MIPFPOINT,"D3DPTFILTERCAPS_MIPFPOINT");
                checkflag(flag,D3DPTFILTERCAPS_MIPLINEAR,"D3DPTFILTERCAPS_MIPLINEAR");
                checkflag(flag,D3DPTFILTERCAPS_MIPNEAREST,"D3DPTFILTERCAPS_MIPNEAREST");
                checkflag(flag,D3DPTFILTERCAPS_NEAREST,"D3DPTFILTERCAPS_NEAREST");
                endcheckflag(flag,"puD3dDriverData->hwCaps.dpcTriCaps.dwTextureFilterCaps ");

                printf(" puD3dDriverData->hwCaps.dpcTriCaps.dwTextureBlendCaps          : ");
                count = 0;
                flag = puD3dDriverData->hwCaps.dpcTriCaps.dwTextureBlendCaps;
                checkflag(flag,D3DPTBLENDCAPS_ADD,"D3DPTBLENDCAPS_ADD");
                checkflag(flag,D3DPTBLENDCAPS_COPY,"D3DPTBLENDCAPS_COPY");
                checkflag(flag,D3DPTBLENDCAPS_DECAL,"D3DPTBLENDCAPS_DECAL");
                checkflag(flag,D3DPTBLENDCAPS_DECALALPHA,"D3DPTBLENDCAPS_DECALALPHA");
                checkflag(flag,D3DPTBLENDCAPS_DECALMASK,"D3DPTBLENDCAPS_DECALMASK");
                checkflag(flag,D3DPTBLENDCAPS_MODULATE,"D3DPTBLENDCAPS_MODULATE");
                checkflag(flag,D3DPTBLENDCAPS_MODULATEALPHA,"D3DPTBLENDCAPS_MODULATEALPHA");
                checkflag(flag,D3DPTBLENDCAPS_MODULATEMASK,"D3DPTBLENDCAPS_MODULATEMASK");
                endcheckflag(flag,"puD3dDriverData->hwCaps.dpcTriCaps.dwTextureBlendCaps ");

                printf(" puD3dDriverData->hwCaps.dpcTriCaps.dwTextureAddressCaps        : ");
                count = 0;
                flag = puD3dDriverData->hwCaps.dpcTriCaps.dwTextureAddressCaps;
                checkflag(flag,D3DPTADDRESSCAPS_BORDER,"D3DPTADDRESSCAPS_BORDER");
                checkflag(flag,D3DPTADDRESSCAPS_CLAMP,"D3DPTADDRESSCAPS_CLAMP");
                checkflag(flag,D3DPTADDRESSCAPS_INDEPENDENTUV,"D3DPTADDRESSCAPS_INDEPENDENTUV");
                checkflag(flag,D3DPTADDRESSCAPS_MIRROR,"D3DPTADDRESSCAPS_MIRROR");
                checkflag(flag,D3DPTADDRESSCAPS_WRAP,"D3DPTADDRESSCAPS_WRAP");
                endcheckflag(flag,"puD3dDriverData->hwCaps.dpcTriCaps.dwTextureAddressCaps ");

                printf(" puD3dDriverData->hwCaps.dpcTriCaps.dwStippleWidth              : 0x%08lx\n",(long)puD3dDriverData->hwCaps.dpcTriCaps.dwStippleWidth);
                printf(" puD3dDriverData->hwCaps.dpcTriCaps.dwStippleHeight             : 0x%08lx\n",(long)puD3dDriverData->hwCaps.dpcTriCaps.dwStippleHeight);
            }
            else
            {
                printf("none puD3dDriverData->hwCaps.dpcTriCaps.dwSize from the driver 0x%08lx\n",puD3dDriverData->hwCaps.dpcTriCaps.dwSize);
            }

            printf(" puD3dDriverData->hwCaps.dwDeviceRenderBitDepth                 : 0x%08lx\n",(long)puD3dDriverData->hwCaps.dwDeviceRenderBitDepth);
            printf(" puD3dDriverData->hwCaps.dwDeviceZBufferBitDepth                : 0x%08lx\n",(long)puD3dDriverData->hwCaps.dwDeviceZBufferBitDepth);
            printf(" puD3dDriverData->hwCaps.dwMaxBufferSize                        : 0x%08lx\n",(long)puD3dDriverData->hwCaps.dwMaxBufferSize);
            printf(" puD3dDriverData->hwCaps.dwMaxVertexCount                       : 0x%08lx\n",(long)puD3dDriverData->hwCaps.dwMaxVertexCount);
        }
        else
        {
            printf("none puD3dDriverData->hwCaps.dwSize from the driver 0x%08lx\n",puD3dDriverData->hwCaps.dwSize);
        }

        printf(" puD3dDriverData->dwNumVertices                                 : 0x%08lx\n",(long)puD3dDriverData->dwNumVertices);
        printf(" puD3dDriverData->dwNumClipVertices                             : 0x%08lx\n",(long)puD3dDriverData->dwNumClipVertices);
        printf(" puD3dDriverData->dwNumTextureFormats                           : 0x%08lx\n",(long)puD3dDriverData->dwNumTextureFormats);
        printf(" puD3dDriverData->lpTextureFormats                              : 0x%08lx\n",(long)puD3dDriverData->lpTextureFormats);
        printf(" puD3dDriverData->lpTextureFormats                              : 0x%08lx\n",(long)puD3dDriverData->lpTextureFormats);
    }
    else
    {
        printf("none puD3dDriverData from the driver 0x%08lx\n",puD3dDriverData->dwSize);
    }
}


void
dump_D3dBufferCallbacks(DD_D3DBUFCALLBACKS *puD3dBufferCallbacks, char *text)
{
    int count = 0;
    DWORD flag = 0;

    printf("dumping the DD_D3DBUFCALLBACKS from %s\n",text);

    if (puD3dBufferCallbacks->dwSize == sizeof(DD_D3DBUFCALLBACKS))
    {
        printf(" puD3dBufferCallbacks->dwSize                                   : 0x%08lx\n",(long)puD3dBufferCallbacks->dwSize);
        printf(" puD3dBufferCallbacks->dwFlags                                  : ");

        /* rember this flags are not in msdn only in ms ddk */
        count = 0;
        flag = puD3dBufferCallbacks->dwFlags;
        checkflag(flag,DDHAL_D3DBUFCB32_CANCREATED3DBUF,"DDHAL_D3DBUFCB32_CANCREATED3DBUF");
        checkflag(flag,DDHAL_D3DBUFCB32_CREATED3DBUF,"DDHAL_D3DBUFCB32_CREATED3DBUF");

        checkflag(flag,DDHAL_D3DBUFCB32_DESTROYD3DBUF,"DDHAL_D3DBUFCB32_DESTROYD3DBUF");

        checkflag(flag,DDHAL_D3DBUFCB32_LOCKD3DBUF,"DDHAL_D3DBUFCB32_LOCKD3DBUF");
        checkflag(flag,DDHAL_D3DBUFCB32_UNLOCKD3DBUF,"DDHAL_D3DBUFCB32_UNLOCKD3DBUF");
        endcheckflag(flag,"puD3dBufferCallbacks->dwFlags");

        printf(" puD3dBufferCallbacks->CanCreateD3DBuffer                       : 0x%08lx\n",(long)puD3dBufferCallbacks->CanCreateD3DBuffer);
        printf(" puD3dBufferCallbacks->CreateD3DBuffer                          : 0x%08lx\n",(long)puD3dBufferCallbacks->CreateD3DBuffer);
        printf(" puD3dBufferCallbacks->DestroyD3DBuffer                         : 0x%08lx\n",(long)puD3dBufferCallbacks->DestroyD3DBuffer);
        printf(" puD3dBufferCallbacks->LockD3DBuffer                            : 0x%08lx\n",(long)puD3dBufferCallbacks->LockD3DBuffer);
        printf(" puD3dBufferCallbacks->UnlockD3DBuffer                          : 0x%08lx\n",(long)puD3dBufferCallbacks->UnlockD3DBuffer);
    }
    else
    {
        printf("none puD3dBufferCallbacks from the driver 0x%08lx\n",puD3dBufferCallbacks->dwSize);
    }

}

void
dump_D3dTextureFormats(DDSURFACEDESC *puD3dTextureFormats, int dwNum, char *text)
{
    int t=0;
    int count = 0;
    DWORD flag = 0;
    DDSURFACEDESC * myTextureFormats = puD3dTextureFormats;

    printf("dumping the DDSURFACEDESC/DDSURFACEDESC2 from %s\n",text);

    for (t=0;t<dwNum;t++)
    {
        printf("Show %d of %d DDSURFACEDESC\n",t+1,dwNum);
        if (myTextureFormats->dwSize == sizeof(DDSURFACEDESC))
        {
            printf(" puD3dTextureFormats->dwSize                                    : 0x%08lx\n",(long)myTextureFormats->dwSize);

            printf(" puD3dTextureFormats->dwFlags                                   : ");
            count = 0;
            flag = myTextureFormats->dwFlags;
            checkflag(flag,DDSD_ALPHABITDEPTH,"DDSD_ALPHABITDEPTH");
            checkflag(flag,DDSD_BACKBUFFERCOUNT ,"DDSD_BACKBUFFERCOUNT");
            checkflag(flag,DDSD_CAPS,"DDSD_CAPS ");
            checkflag(flag,DDSD_CKDESTBLT,"DDSD_CKDESTBLT");
            checkflag(flag,DDSD_CKDESTOVERLAY,"DDSD_CKDESTOVERLAY");
            checkflag(flag,DDSD_CKSRCBLT,"DDSD_CKSRCBLT");
            checkflag(flag,DDSD_CKSRCOVERLAY,"DDSD_CKSRCOVERLAY");
            checkflag(flag,DDSD_HEIGHT,"DDSD_HEIGHT");
            checkflag(flag,DDSD_LINEARSIZE,"DDSD_LINEARSIZE");
            checkflag(flag,DDSD_LPSURFACE,"DDSD_LPSURFACE");
            checkflag(flag,DDSD_MIPMAPCOUNT,"DDSD_MIPMAPCOUNT");
            checkflag(flag,DDSD_PITCH,"DDSD_PITCH");
            checkflag(flag,DDSD_PIXELFORMAT,"DDSD_PIXELFORMAT");
            checkflag(flag,DDSD_REFRESHRATE,"DDSD_REFRESHRATE");
            checkflag(flag,DDSD_WIDTH,"DDSD_WIDTH");
            checkflag(flag,DDSD_ZBUFFERBITDEPTH,"DDSD_ZBUFFERBITDEPTH");
            endcheckflag(flag,"puD3dTextureFormats->dwFlags");

            printf(" puD3dTextureFormats->dwHeight                                  : 0x%08lx\n",(long)myTextureFormats->dwHeight);
            printf(" puD3dTextureFormats->dwWidth                                   : 0x%08lx\n",(long)myTextureFormats->dwWidth);
            printf(" puD3dTextureFormats->dwLinearSize                              : 0x%08lx\n",(long)myTextureFormats->dwLinearSize);
            printf(" puD3dTextureFormats->dwBackBufferCount                         : 0x%08lx\n",(long)myTextureFormats->dwBackBufferCount);
            printf(" puD3dTextureFormats->dwZBufferBitDepth                         : 0x%08lx\n",(long)myTextureFormats->dwZBufferBitDepth);
            printf(" puD3dTextureFormats->dwAlphaBitDepth                           : 0x%08lx\n",(long)myTextureFormats->dwAlphaBitDepth);
            printf(" puD3dTextureFormats->dwReserved                                : 0x%08lx\n",(long)myTextureFormats->dwReserved);
            printf(" puD3dTextureFormats->lpSurface                                 : 0x%08lx\n",(long)myTextureFormats->lpSurface);
            printf(" puD3dTextureFormats->ddckCKDestOverlay.dwColorSpaceLowValue    : 0x%08lx\n",(long)myTextureFormats->ddckCKDestOverlay.dwColorSpaceLowValue);
            printf(" puD3dTextureFormats->ddckCKDestOverlay.dwColorSpaceHighValue   : 0x%08lx\n",(long)myTextureFormats->ddckCKDestOverlay.dwColorSpaceHighValue);
            printf(" puD3dTextureFormats->ddckCKDestBlt.dwColorSpaceLowValue        : 0x%08lx\n",(long)myTextureFormats->ddckCKDestBlt.dwColorSpaceLowValue);
            printf(" puD3dTextureFormats->ddckCKDestBlt                             : 0x%08lx\n",(long)myTextureFormats->ddckCKDestBlt.dwColorSpaceHighValue);
            printf(" puD3dTextureFormats->ddckCKSrcOverlay.dwColorSpaceLowValue     : 0x%08lx\n",(long)myTextureFormats->ddckCKSrcOverlay.dwColorSpaceLowValue);
            printf(" puD3dTextureFormats->ddckCKSrcOverlay.dwColorSpaceHighValue    : 0x%08lx\n",(long)myTextureFormats->ddckCKSrcOverlay.dwColorSpaceHighValue);
            printf(" puD3dTextureFormats->ddckCKSrcBlt.dwColorSpaceLowValue         : 0x%08lx\n",(long)myTextureFormats->ddckCKSrcBlt.dwColorSpaceLowValue);
            printf(" puD3dTextureFormats->ddckCKSrcBlt.dwColorSpaceHighValue        : 0x%08lx\n",(long)myTextureFormats->ddckCKSrcBlt.dwColorSpaceHighValue);

         // DDPIXELFORMAT
            printf(" puD3dTextureFormats->ddpfPixelFormat.dwSize                    : 0x%08lx\n",(long)myTextureFormats->ddpfPixelFormat.dwSize);
            if (puD3dTextureFormats->ddpfPixelFormat.dwSize == sizeof(DDPIXELFORMAT))
            {
                printf(" puD3dTextureFormats->ddpfPixelFormat.dwFlags                   : ");
                count = 0;
                flag = myTextureFormats->ddpfPixelFormat.dwFlags;
                checkflag(flag,DDPF_ALPHA,"DDPF_ALPHA");
                checkflag(flag,DDPF_ALPHAPIXELS ,"DDPF_ALPHAPIXELS");
                checkflag(flag,DDPF_ALPHAPREMULT,"DDPF_ALPHAPREMULT");
                checkflag(flag,DDPF_BUMPLUMINANCE ,"DDPF_BUMPLUMINANCE");
                checkflag(flag,DDPF_BUMPDUDV,"DDPF_BUMPDUDV");
                checkflag(flag,DDPF_COMPRESSED,"DDPF_COMPRESSED");
                checkflag(flag,DDPF_FOURCC,"DDPF_FOURCC");
                checkflag(flag,DDPF_LUMINANCE,"DDPF_LUMINANCE");
                checkflag(flag,DDPF_PALETTEINDEXED1,"DDPF_PALETTEINDEXED1");
                checkflag(flag,DDPF_PALETTEINDEXED2,"DDPF_PALETTEINDEXED2");
                checkflag(flag,DDPF_PALETTEINDEXED4,"DDPF_PALETTEINDEXED4");
                checkflag(flag,DDPF_PALETTEINDEXED8,"DDPF_PALETTEINDEXED8");
                checkflag(flag,DDPF_PALETTEINDEXEDTO8,"DDPF_PALETTEINDEXEDTO8");
                checkflag(flag,DDPF_RGB ,"DDPF_RGB");
                checkflag(flag,DDPF_RGBTOYUV,"DDPF_RGBTOYUV");
                checkflag(flag,DDPF_STENCILBUFFER,"DDPF_STENCILBUFFER");
                checkflag(flag,DDPF_YUV,"DDPF_YUV");
                checkflag(flag,DDPF_ZBUFFER,"DDPF_ZBUFFER");
                checkflag(flag,DDPF_ZPIXELS,"DDPF_ZPIXELS");
                endcheckflag(flag,"puD3dTextureFormats->ddpfPixelFormat.dwFlags");


                if (myTextureFormats->ddpfPixelFormat.dwFlags & DDPF_FOURCC)
                {
                    printf(" puD3dTextureFormats->ddpfPixelFormat.dwFourCC                  : ");
                    switch(myTextureFormats->ddpfPixelFormat.dwFourCC)
                    {
                        case MAKEFOURCC('A','U','R','2'):
                            printf("AUR2\n");
                            break;

                        case MAKEFOURCC('A','U','R','A'):
                            printf("AURA\n");
                            break;

                        case MAKEFOURCC('C','H','A','M'):
                            printf("CHAM\n");
                            break;

                        case MAKEFOURCC('C','V','I','D'):
                            printf("CVID\n");
                            break;

                        case MAKEFOURCC('D','X','T','1'):
                            printf("DXT1\n");
                            break;

                        case MAKEFOURCC('D','X','T','2'):
                            printf("DXT2\n");
                            break;

                        case MAKEFOURCC('D','X','T','3'):
                            printf("DXT3\n");
                            break;

                        case MAKEFOURCC('D','X','T','4'):
                            printf("DXT4\n");
                            break;

                        case MAKEFOURCC('D','X','T','5'):
                            printf("DXT5\n");
                            break;

                        case MAKEFOURCC('F','V','F','1'):
                            printf("FVF1\n");
                            break;

                        case MAKEFOURCC('I','F','0','9'):
                            printf("IF09\n");
                            break;

                        case MAKEFOURCC('I','V','3','1'):
                            printf("IV31\n");
                            break;

                        case MAKEFOURCC('J','P','E','G'):
                            printf("JPEG\n");
                            break;

                        case MAKEFOURCC('M','J','P','G'):
                            printf("MJPG\n");
                            break;

                        case MAKEFOURCC('M','R','L','E'):
                            printf("MRLE\n");
                            break;

                        case MAKEFOURCC('M','S','V','C'):
                            printf("MSVC\n");
                            break;

                        case MAKEFOURCC('P','H','M','O'):
                            printf("PHMO\n");
                            break;

                        case MAKEFOURCC('R','T','2','1'):
                            printf("RT21\n");
                            break;

                        case MAKEFOURCC('U','L','T','I'):
                            printf("ULTI\n");
                            break;

                        case MAKEFOURCC('V','4','2','2'):
                            printf("V422\n");
                            break;

                        case MAKEFOURCC('V','6','5','5'):
                            printf("V655\n");
                            break;

                        case MAKEFOURCC('V','D','C','T'):
                            printf("VDCT\n");
                            break;

                        case MAKEFOURCC('V','I','D','S'):
                            printf("VIDS\n");
                            break;

                        case MAKEFOURCC('Y','U','9','2'):
                            printf("YU92\n");
                            break;

                        case MAKEFOURCC('Y','U','V','8'):
                            printf("YUV8\n");
                            break;

                        case MAKEFOURCC('Y','U','V','9'):
                            printf("YUV9\n");
                            break;

                        case MAKEFOURCC('Y','U','Y','V'):
                            printf("YUYV\n");
                            break;

                        case MAKEFOURCC('Z','P','E','G'):
                            printf("ZPEG\n");
                            break;

                        default:
                            printf("0x%08lx\n",(long)myTextureFormats->ddpfPixelFormat.dwFourCC);
                            break;
                    }
                }
                else
                {
                    printf(" puD3dTextureFormats->ddpfPixelFormat.dwFourCC                  : 0x%08lx\n",(long)myTextureFormats->ddpfPixelFormat.dwFourCC);
                }
                printf(" puD3dTextureFormats->ddpfPixelFormat.dwRGBBitCount             : 0x%08lx\n",(long)myTextureFormats->ddpfPixelFormat.dwRGBBitCount);
                printf(" puD3dTextureFormats->ddpfPixelFormat.dwRBitMask                : 0x%08lx\n",(long)myTextureFormats->ddpfPixelFormat.dwRBitMask);
                printf(" puD3dTextureFormats->ddpfPixelFormat.dwGBitMask                : 0x%08lx\n",(long)myTextureFormats->ddpfPixelFormat.dwGBitMask);
                printf(" puD3dTextureFormats->ddpfPixelFormat.dwBBitMask                : 0x%08lx\n",(long)myTextureFormats->ddpfPixelFormat.dwBBitMask);
                printf(" puD3dTextureFormats->ddpfPixelFormat.dwRGBAlphaBitMask         : 0x%08lx\n",(long)myTextureFormats->ddpfPixelFormat.dwRGBAlphaBitMask);
            }
            else
            {
                printf("none uD3dTextureFormats->ddpfPixelFormat from the driver 0x%08lx\n",myTextureFormats->ddpfPixelFormat.dwSize);
            }

            printf(" puD3dTextureFormats->ddsCaps.dwCaps                            : ");
            count = 0;
            flag = myTextureFormats->ddsCaps.dwCaps;
            // not in use longer acoing msdn checkflag(flag,DDSCAPS_3D,"DDSCAPS_3D");
            checkflag(flag,DDSCAPS_3DDEVICE  ,"DDSCAPS_3DDEVICE");
            checkflag(flag,DDSCAPS_ALLOCONLOAD,"DDSCAPS_ALLOCONLOAD ");
            checkflag(flag,DDSCAPS_ALPHA,"DDSCAPS_ALPHA");
            checkflag(flag,DDSCAPS_BACKBUFFER,"DDSCAPS_BACKBUFFER");
            checkflag(flag,DDSCAPS_FLIP,"DDSCAPS_FLIP");
            checkflag(flag,DDSCAPS_FRONTBUFFER,"DDSCAPS_FRONTBUFFER");
            checkflag(flag,DDSCAPS_HWCODEC,"DDSCAPS_HWCODEC");
            checkflag(flag,DDSCAPS_LIVEVIDEO ,"DDSCAPS_LIVEVIDEO");
            checkflag(flag,DDSCAPS_LOCALVIDMEM,"DDSCAPS_LOCALVIDMEM");
            checkflag(flag,DDSCAPS_MIPMAP,"DDSCAPS_MIPMAP");
            checkflag(flag,DDSCAPS_MODEX,"DDSCAPS_MODEX");
            checkflag(flag,DDSCAPS_NONLOCALVIDMEM,"DDSCAPS_NONLOCALVIDMEM");
            checkflag(flag,DDSCAPS_OFFSCREENPLAIN,"DDSCAPS_OFFSCREENPLAIN");
            checkflag(flag,DDSCAPS_OPTIMIZED,"DDSCAPS_OPTIMIZED");
            checkflag(flag,DDSCAPS_OVERLAY,"DDSCAPS_OVERLAY");
            checkflag(flag,DDSCAPS_OWNDC,"DDSCAPS_OWNDC");
            checkflag(flag,DDSCAPS_PALETTE,"DDSCAPS_PALETTE");
            checkflag(flag,DDSCAPS_PRIMARYSURFACE ,"DDSCAPS_PRIMARYSURFACE");
            checkflag(flag,DDSCAPS_STANDARDVGAMODE,"DDSCAPS_STANDARDVGAMODE");
            checkflag(flag,DDSCAPS_SYSTEMMEMORY,"DDSCAPS_SYSTEMMEMORY");
            checkflag(flag,DDSCAPS_TEXTURE,"DDSCAPS_TEXTURE");
            checkflag(flag,DDSCAPS_VIDEOMEMORY,"DDSCAPS_VIDEOMEMORY");
            checkflag(flag,DDSCAPS_VIDEOPORT,"DDSCAPS_VIDEOPORT");
            checkflag(flag,DDSCAPS_VISIBLE,"DDSCAPS_VISIBLE");
            checkflag(flag,DDSCAPS_WRITEONLY ,"DDSCAPS_WRITEONLY");
            checkflag(flag,DDSCAPS_ZBUFFER,"DDSCAPS_ZBUFFER");
            endcheckflag(flag,"puD3dTextureFormats->ddsCaps.dwCaps");


           myTextureFormats = (DDSURFACEDESC *) (((DWORD) myTextureFormats) + sizeof(DDSURFACEDESC));
        }
        else
        {
            printf("error this should not happen : puD3dTextureFormats from the driver 0x%08lx\n",myTextureFormats->dwSize);
            break;
        }
    }











}
