#include <stdio.h>
/* SDK/DDK/NDK Headers. */
#include <windows.h>
#include <wingdi.h>
#include <winddi.h>
#include <d3dnthal.h>
#include <dll/directx/d3d8thk.h>
#include "test.h"

extern BOOL dumping_on;

/* my struct */
struct
{
    DWORD pos;
} *mytest;

/*
 * Test see if we can setup DirectDrawObject
 *
 */

/*
 * ToDO
 * 1. add more testcase it is only some, but we do not test for all case
 *    that happen only some
 *
 * 2.Fixed the false alaret for drivers only support 2d dx interface
 *
 * 3. fixed the dumping of d3d struct.
 *
 */

void
test_NtGdiDdQueryDirectDrawObject( HANDLE hDirectDrawLocal)
{
    int fails=0;
    BOOL retValue=FALSE;

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
    DWORD CallBackFlags[4];
    D3DNTHAL_CALLBACKS D3dCallbacks;
    D3DNTHAL_GLOBALDRIVERDATA D3dDriverData;
    DD_D3DBUFCALLBACKS D3dBufferCallbacks;
    DDSURFACEDESC2 D3dTextureFormats[100];
    DWORD NumHeaps = 0;
    VIDEOMEMORY vmList;
    DWORD NumFourCC = 0;
    DWORD FourCC = 0;

    /* clear data */
    memset(&vmList,0,sizeof(VIDEOMEMORY));
    memset(&D3dTextureFormats,0,sizeof(DDSURFACEDESC));
    memset(&D3dBufferCallbacks,0,sizeof(DD_D3DBUFCALLBACKS));
    memset(&D3dDriverData,0,sizeof(D3DNTHAL_GLOBALDRIVERDATA));
    memset(&D3dCallbacks,0,sizeof(D3DNTHAL_CALLBACKS));
    memset(&HalInfo,0,sizeof(DD_HALINFO));
    memset(CallBackFlags,0,sizeof(DWORD)*3);

    printf("Start testing of NtGdiDdQueryDirectDrawObject\n");

    /* testing  OsThunkDdQueryDirectDrawObject( NULL, ....  */
    printf("testing  DdQueryDirectDrawObject( NULL, ....)\n");

    retValue = OsThunkDdQueryDirectDrawObject( NULL, pHalInfo,
                                                pCallBackFlags, puD3dCallbacks,
                                                puD3dDriverData, puD3dBufferCallbacks,
                                                puD3dTextureFormats, puNumHeaps,
                                                puvmList, puNumFourCC,
                                                puFourCC);
    testing_noteq(retValue,FALSE,fails,"1. NtGdiDdQueryDirectDrawObject(NULL, ...);\0");
    testing_noteq(pHalInfo,NULL,fails,"2. NtGdiDdQueryDirectDrawObject(NULL, ...);\0");
    testing_noteq(pCallBackFlags,NULL,fails,"3. NtGdiDdQueryDirectDrawObject(NULL, ...);\0");
    testing_noteq(puD3dCallbacks,NULL,fails,"4. NtGdiDdQueryDirectDrawObject(NULL, ...);\0");
    testing_noteq(puD3dDriverData,NULL,fails,"5. NtGdiDdQueryDirectDrawObject(NULL, ...);\0");
    testing_noteq(puD3dBufferCallbacks,NULL,fails,"6. NtGdiDdQueryDirectDrawObject(NULL, ...);\0");
    testing_noteq(puD3dTextureFormats,NULL,fails,"7. NtGdiDdQueryDirectDrawObject(NULL, ...);\0");
    testing_noteq(puNumFourCC,NULL,fails,"8. NtGdiDdQueryDirectDrawObject(NULL, ...);\0");
    testing_noteq(puFourCC,NULL,fails,"9. NtGdiDdQueryDirectDrawObject(NULL, ...);\0");

    /* testing  OsThunkDdQueryDirectDrawObject( hDirectDrawLocal, NULL, ....  */
    printf("testing  DdQueryDirectDrawObject( hDD, NULL, ....)\n");

    retValue = OsThunkDdQueryDirectDrawObject( hDirectDrawLocal, pHalInfo,
                                                pCallBackFlags, puD3dCallbacks,
                                                puD3dDriverData, puD3dBufferCallbacks,
                                                puD3dTextureFormats, puNumHeaps,
                                                puvmList, puNumFourCC,
                                                puFourCC);

    testing_noteq(retValue,FALSE,fails,"1. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, NULL, ...);\0");
    testing_noteq(pHalInfo,NULL,fails,"2. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, NULL, ...);\0");
    testing_noteq(pCallBackFlags,NULL,fails,"3. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, NULL, ...);\0");
    testing_noteq(puD3dCallbacks,NULL,fails,"4. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, NULL, ...);\0");
    testing_noteq(puD3dDriverData,NULL,fails,"5. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, NULL, ...);\0");
    testing_noteq(puD3dBufferCallbacks,NULL,fails,"6. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, NULL, ...);\0");
    testing_noteq(puD3dTextureFormats,NULL,fails,"7. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, NULL, ...);\0");
    testing_noteq(puNumFourCC,NULL,fails,"8. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, NULL, ...);\0");
    testing_noteq(puFourCC,NULL,fails,"9. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, NULL, ...);\0");

    /* testing  OsThunkDdQueryDirectDrawObject( hDirectDrawLocal, pHalInfo, NULL, ....  */
    printf("testing  DdQueryDirectDrawObject( hDD, pHalInfo, NULL, ....)\n");

    pHalInfo = &HalInfo;
    retValue = OsThunkDdQueryDirectDrawObject( hDirectDrawLocal, pHalInfo,
                                                pCallBackFlags, puD3dCallbacks,
                                                puD3dDriverData, puD3dBufferCallbacks,
                                                puD3dTextureFormats, puNumHeaps,
                                                puvmList, puNumFourCC,
                                                puFourCC);

    testing_noteq(retValue,FALSE,fails,"1. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, NULL, ...);\0");
    testing_eq(pHalInfo,NULL,fails,"2. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, NULL, ...);\0");
    testing_noteq(pCallBackFlags,NULL,fails,"3. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, NULL, ...);\0");
    testing_noteq(puD3dCallbacks,NULL,fails,"4. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, NULL, ...);\0");
    testing_noteq(puD3dDriverData,NULL,fails,"5. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, NULL, ...);\0");
    testing_noteq(puD3dBufferCallbacks,NULL,fails,"6. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, NULL, ...);\0");
    testing_noteq(puD3dTextureFormats,NULL,fails,"7. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal,  pHalInfo, NULL, ...);\0");
    testing_noteq(puNumFourCC,NULL,fails,"8. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, NULL, ...);\0");
    testing_noteq(puFourCC,NULL,fails,"9. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, NULL, ...);\0");

    if ((pHalInfo->dwSize != sizeof(DD_HALINFO)) &&
        (pHalInfo->dwSize != sizeof(DD_HALINFO_V4)))
    {
        printf("10. if this show for NT 2000/XP/2003 ignore it, NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, NULL, ...);\n");
        fails++;
    }

    if (dumping_on == TRUE)
    {
        dump_halinfo(pHalInfo,"NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, NULL, ...)");
    }

    /* testing  OsThunkDdQueryDirectDrawObject( hDirectDrawLocal, pHalInfo, pCallBackFlags, NULL, ....  */
    printf("testing  DdQueryDirectDrawObject( hDD, pHalInfo, pCallBackFlags, NULL, ....)\n");

    pHalInfo = &HalInfo;
    pCallBackFlags = CallBackFlags;

    RtlZeroMemory(pHalInfo,sizeof(DD_HALINFO));

    retValue = OsThunkDdQueryDirectDrawObject( hDirectDrawLocal, pHalInfo,
                                                pCallBackFlags, puD3dCallbacks,
                                                puD3dDriverData, puD3dBufferCallbacks,
                                                puD3dTextureFormats, puNumHeaps,
                                                puvmList, puNumFourCC,
                                                puFourCC);

    testing_noteq(retValue,FALSE,fails,"1. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, pCallBackFlags, NULL, ...);\0");
    testing_eq(pHalInfo,NULL,fails,"2. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, pCallBackFlags, NULL, ...);\0");
    testing_eq(pCallBackFlags,NULL,fails,"3. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, pCallBackFlags, NULL, ...);\0");
    testing_noteq(puD3dCallbacks,NULL,fails,"4. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, pCallBackFlags, NULL, ...);\0");
    testing_noteq(puD3dDriverData,NULL,fails,"5. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, pCallBackFlags, NULL, ...);\0");
    testing_noteq(puD3dBufferCallbacks,NULL,fails,"6. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, pCallBackFlags, NULL, ...);\0");
    testing_noteq(puD3dTextureFormats,NULL,fails,"7. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal,  pHalInfo, pCallBackFlags, NULL, ...);\0");
    testing_noteq(puNumFourCC,NULL,fails,"8. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, pCallBackFlags, NULL, ...);\0");
    testing_noteq(puFourCC,NULL,fails,"9. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, pCallBackFlags, NULL, ...);\0");
    if ((pHalInfo->dwSize != sizeof(DD_HALINFO)) &&
        (pHalInfo->dwSize != sizeof(DD_HALINFO_V4)))
    {
        printf("10. if this show for NT 2000/XP/2003 ignore it, NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, pCallBackFlags, NULL, ...);\n");
        fails++;
    }

    if (dumping_on == TRUE)
    {
        dump_halinfo(pHalInfo,"NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, pCallBackFlags, NULL, ...)");
        dump_CallBackFlags(pCallBackFlags,"NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, pCallBackFlags, NULL, ...)");
    }

    /* testing  OsThunkDdQueryDirectDrawObject( hDirectDrawLocal, pHalInfo, pCallBackFlags, NULL, ....  */
    printf("testing  DdQueryDirectDrawObject( hDD, pHalInfo, pCallBackFlags, puD3dCallbacks, NULL, ....)\n");

    pHalInfo = &HalInfo;
    pCallBackFlags = CallBackFlags;
    puD3dCallbacks = &D3dCallbacks;

    RtlZeroMemory(pHalInfo,sizeof(DD_HALINFO));
    RtlZeroMemory(pCallBackFlags,sizeof(DWORD)*3);

    retValue = OsThunkDdQueryDirectDrawObject( hDirectDrawLocal, pHalInfo,
                                                pCallBackFlags, puD3dCallbacks,
                                                puD3dDriverData, puD3dBufferCallbacks,
                                                puD3dTextureFormats, puNumHeaps,
                                                puvmList, puNumFourCC,
                                                puFourCC);

    testing_noteq(retValue,FALSE,fails,"1. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, pCallBackFlags, puD3dCallbacks, NULL, ...);\0");
    testing_eq(pHalInfo,NULL,fails,"2. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, pCallBackFlags, puD3dCallbacks, NULL, ...);\0");
    testing_eq(pCallBackFlags,NULL,fails,"3. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, pCallBackFlags, puD3dCallbacks, NULL, ...);\0");
    testing_noteq(puD3dCallbacks->dwSize,sizeof(D3DNTHAL_CALLBACKS),fails,"4. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, pCallBackFlags, puD3dCallbacks, NULL, ...);\0");

    testing_noteq(puD3dDriverData,NULL,fails,"5. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, pCallBackFlags, puD3dCallbacks, NULL, ...);\0");
    testing_noteq(puD3dBufferCallbacks,NULL,fails,"6. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, pCallBackFlags, puD3dCallbacks, NULL, ...);\0");
    testing_noteq(puD3dTextureFormats,NULL,fails,"7. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal,  pHalInfo, pCallBackFlags, puD3dCallbacks, NULL, ...);\0");
    testing_noteq(puNumFourCC,NULL,fails,"8. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, pCallBackFlags, puD3dCallbacks, NULL, ...);\0");
    testing_noteq(puFourCC,NULL,fails,"9. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, pCallBackFlags, puD3dCallbacks, NULL, ...);\0");
    if ((pHalInfo->dwSize != sizeof(DD_HALINFO)) &&
        (pHalInfo->dwSize != sizeof(DD_HALINFO_V4)))
    {
        printf("10. if this show for NT 2000/XP/2003 ignore it, NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, pCallBackFlags, puD3dCallbacks, NULL, ...);\n");
        fails++;
    }

    if (dumping_on == TRUE)
    {
        dump_halinfo(pHalInfo,"NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, pCallBackFlags, NULL, ...)");
        dump_CallBackFlags(pCallBackFlags,"NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, pCallBackFlags, NULL, ...)");
        /* FIXME dump puD3dCallbacks */
    }

   /* testing  OsThunkDdQueryDirectDrawObject( hDD, pHalInfo, pCallBackFlags, puD3dCallbacks, puD3dDriverData, NULL, */
    printf("testing  DdQueryDirectDrawObject( hDD, pHalInfo, pCallBackFlags, puD3dCallbacks, puD3dDriverData, NULL, ....)\n");

    pHalInfo = &HalInfo;
    pCallBackFlags = CallBackFlags;
    puD3dCallbacks = &D3dCallbacks;
    puD3dDriverData = &D3dDriverData;

    RtlZeroMemory(pHalInfo,sizeof(DD_HALINFO));
    RtlZeroMemory(pCallBackFlags,sizeof(DWORD)*3);
    RtlZeroMemory(puD3dCallbacks,sizeof(D3DNTHAL_CALLBACKS));

    retValue = OsThunkDdQueryDirectDrawObject( hDirectDrawLocal, pHalInfo,
                                                pCallBackFlags, puD3dCallbacks,
                                                puD3dDriverData, puD3dBufferCallbacks,
                                                puD3dTextureFormats, puNumHeaps,
                                                puvmList, puNumFourCC,
                                                puFourCC);

    testing_noteq(retValue,FALSE,fails,"1. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, pCallBackFlags, puD3dCallbacks, puD3dDriverData, NULL, ...);\0");
    testing_eq(pHalInfo,NULL,fails,"2. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, pCallBackFlags, puD3dCallbacks, puD3dDriverData, NULL, ...);\0");
    testing_eq(pCallBackFlags,NULL,fails,"3. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, pCallBackFlags, puD3dCallbacks, puD3dDriverData, NULL, ...);\0");
    testing_noteq(puD3dCallbacks->dwSize,sizeof(D3DNTHAL_CALLBACKS),fails,"4. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, pCallBackFlags, puD3dCallbacks, puD3dDriverData, NULL, ...);\0");

    testing_noteq(puD3dDriverData->dwSize,sizeof(D3DNTHAL_GLOBALDRIVERDATA),fails,"5. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, pCallBackFlags, puD3dCallbacks, NULL, ...);\0");

    testing_noteq(puD3dBufferCallbacks,NULL,fails,"6. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, pCallBackFlags, puD3dCallbacks, puD3dDriverData, NULL, ...);\0");
    testing_noteq(puD3dTextureFormats,NULL,fails,"7. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal,  pHalInfo, pCallBackFlags, puD3dCallbacks, puD3dDriverData, NULL, ...);\0");
    testing_noteq(puNumFourCC,NULL,fails,"8. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, pCallBackFlags, puD3dCallbacks, puD3dDriverData, NULL, ...);\0");
    testing_noteq(puFourCC,NULL,fails,"9. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, pCallBackFlags, puD3dCallbacks, puD3dDriverData, NULL, ...);\0");
    if ((pHalInfo->dwSize != sizeof(DD_HALINFO)) &&
        (pHalInfo->dwSize != sizeof(DD_HALINFO_V4)))
    {
        printf("10. if this show for NT 2000/XP/2003 ignore it, NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, pCallBackFlags, puD3dCallbacks, puD3dDriverData, NULL, ...);\n");
        fails++;
    }

    if (dumping_on == TRUE)
    {
        dump_halinfo(pHalInfo,"NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, pCallBackFlags, NULL, ...)");
        dump_CallBackFlags(pCallBackFlags,"NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, pCallBackFlags, NULL, ...)");
        /* FIXME dump puD3dCallbacks */
        /* FIXME dump puD3dDriverData */
    }

/* testing  OsThunkDdQueryDirectDrawObject( hDD, pHalInfo, pCallBackFlags, puD3dCallbacks, puD3dDriverData, D3dBufferCallbacks, NULL, */
    printf("testing  DdQueryDirectDrawObject( hDD, pHalInfo, pCallBackFlags, puD3dCallbacks, puD3dDriverData, D3dBufferCallbacks, NULL, ....)\n");

    pHalInfo = &HalInfo;
    pCallBackFlags = CallBackFlags;
    puD3dCallbacks = &D3dCallbacks;
    puD3dDriverData = &D3dDriverData;
    puD3dBufferCallbacks = &D3dBufferCallbacks;

    RtlZeroMemory(pHalInfo,sizeof(DD_HALINFO));
    RtlZeroMemory(pCallBackFlags,sizeof(DWORD)*3);
    RtlZeroMemory(puD3dCallbacks,sizeof(D3DNTHAL_CALLBACKS));
    RtlZeroMemory(puD3dDriverData,sizeof(D3DNTHAL_CALLBACKS));

    retValue = OsThunkDdQueryDirectDrawObject( hDirectDrawLocal, pHalInfo,
                                                pCallBackFlags, puD3dCallbacks,
                                                puD3dDriverData, puD3dBufferCallbacks,
                                                puD3dTextureFormats, puNumHeaps,
                                                puvmList, puNumFourCC,
                                                puFourCC);

    testing_noteq(retValue,FALSE,fails,"1. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, pCallBackFlags, puD3dCallbacks, puD3dDriverData, puD3dBufferCallbacks, NULL, ...);\0");
    testing_eq(pHalInfo,NULL,fails,"2. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, pCallBackFlags, puD3dCallbacks, puD3dDriverData, puD3dBufferCallbacks, NULL, ...);\0");
    testing_eq(pCallBackFlags,NULL,fails,"3. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, pCallBackFlags, puD3dCallbacks, puD3dDriverData, puD3dBufferCallbacks, NULL, ...);\0");
    testing_noteq(puD3dCallbacks->dwSize,sizeof(D3DNTHAL_CALLBACKS),fails,"4. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, pCallBackFlags, puD3dCallbacks, puD3dDriverData, puD3dBufferCallbacks, NULL, ...);\0");

    testing_noteq(puD3dDriverData->dwSize,sizeof(D3DNTHAL_GLOBALDRIVERDATA),fails,"5. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, pCallBackFlags, puD3dCallbacks, puD3dBufferCallbacks, NULL, ...);\0");

    testing_noteq(puD3dTextureFormats,NULL,fails,"6. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal,  pHalInfo, pCallBackFlags, puD3dCallbacks, puD3dDriverData, puD3dBufferCallbacks, NULL, ...);\0");
    testing_noteq(puNumFourCC,NULL,fails,"7. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, pCallBackFlags, puD3dCallbacks, puD3dDriverData, puD3dBufferCallbacks, NULL, ...);\0");
    testing_noteq(puFourCC,NULL,fails,"8. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, pCallBackFlags, puD3dCallbacks, puD3dDriverData, puD3dBufferCallbacks, NULL, ...);\0");
    if ((pHalInfo->dwSize != sizeof(DD_HALINFO)) &&
        (pHalInfo->dwSize != sizeof(DD_HALINFO_V4)))
    {
        printf("9. if this show for NT 2000/XP/2003 ignore it, NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, pCallBackFlags, puD3dCallbacks, puD3dDriverData, puD3dBufferCallbacks, NULL, ...);\n");
        fails++;
    }

    if (puD3dBufferCallbacks)
    {
        testing_noteq(puD3dBufferCallbacks->dwSize,sizeof(DD_D3DBUFCALLBACKS),fails,"11. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, pCallBackFlags, puD3dCallbacks, puD3dDriverData, puD3dBufferCallbacks, NULL...);\0");
    }

    if (dumping_on == TRUE)
    {
        dump_halinfo(pHalInfo,"NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, pCallBackFlags, NULL, ...)");
        dump_CallBackFlags(pCallBackFlags,"NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, pCallBackFlags, NULL, ...)");
        /* FIXME dump puD3dCallbacks */
        /* FIXME dump puD3dDriverData */
        /* FIXME dump D3dBufferCallbacks */

    }

/* testing  OsThunkDdQueryDirectDrawObject( hDD, pHalInfo, pCallBackFlags, puD3dCallbacks, puD3dDriverData, D3dBufferCallbacks, puD3dTextureFormats, NULL, */
    printf("testing  DdQueryDirectDrawObject( hDD, pHalInfo, pCallBackFlags, puD3dCallbacks, puD3dDriverData, D3dBufferCallbacks, puD3dTextureFormats, NULL, ....)\n");

    pHalInfo = &HalInfo;
    pCallBackFlags = CallBackFlags;
    puD3dCallbacks = &D3dCallbacks;
    puD3dDriverData = &D3dDriverData;
    puD3dBufferCallbacks = &D3dBufferCallbacks;

    RtlZeroMemory(pHalInfo,sizeof(DD_HALINFO));
    RtlZeroMemory(pCallBackFlags,sizeof(DWORD)*3);
    RtlZeroMemory(puD3dCallbacks,sizeof(D3DNTHAL_CALLBACKS));
    //RtlZeroMemory(puD3dDriverData,sizeof(D3DNTHAL_CALLBACKS));
    RtlZeroMemory(&D3dBufferCallbacks,sizeof(D3DNTHAL_CALLBACKS));

    if (puD3dDriverData)
    {
        puD3dTextureFormats = malloc (puD3dDriverData->dwNumTextureFormats * sizeof(DDSURFACEDESC2));
        if (!puD3dTextureFormats)
            printf("Waring Out of memory\n");

        RtlZeroMemory(puD3dTextureFormats, puD3dDriverData->dwNumTextureFormats * sizeof(DDSURFACEDESC2));
    }

    retValue = OsThunkDdQueryDirectDrawObject( hDirectDrawLocal, pHalInfo,
                                                pCallBackFlags, puD3dCallbacks,
                                                puD3dDriverData, puD3dBufferCallbacks,
                                                puD3dTextureFormats, puNumHeaps,
                                                puvmList, puNumFourCC,
                                                puFourCC);

    testing_noteq(retValue,FALSE,fails,"1. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, pCallBackFlags, puD3dCallbacks, puD3dDriverData, puD3dBufferCallbacks, puD3dTextureFormats, NULL, ...);\0");
    testing_eq(pHalInfo,NULL,fails,"2. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, pCallBackFlags, puD3dCallbacks, puD3dDriverData, puD3dBufferCallbacks, puD3dTextureFormats, NULL, ...);\0");
    testing_eq(pCallBackFlags,NULL,fails,"3. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, pCallBackFlags, puD3dCallbacks, puD3dDriverData, puD3dBufferCallbacks, puD3dTextureFormats, NULL, ...);\0");
    testing_noteq(puD3dCallbacks->dwSize,sizeof(D3DNTHAL_CALLBACKS),fails,"4. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, pCallBackFlags, puD3dCallbacks, puD3dDriverData, puD3dBufferCallbacks, puD3dTextureFormats, NULL, ...);\0");

    testing_noteq(puD3dDriverData->dwSize,sizeof(D3DNTHAL_GLOBALDRIVERDATA),fails,"5. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, pCallBackFlags, puD3dCallbacks, puD3dBufferCallbacks, puD3dTextureFormats, NULL, ...);\0");

    testing_noteq(puNumFourCC,NULL,fails,"6. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, pCallBackFlags, puD3dCallbacks, puD3dDriverData, puD3dBufferCallbacks, puD3dTextureFormats, NULL, ...);\0");
    testing_noteq(puFourCC,NULL,fails,"7. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, pCallBackFlags, puD3dCallbacks, puD3dDriverData, puD3dBufferCallbacks, puD3dTextureFormats, NULL, ...);\0");
    if ((pHalInfo->dwSize != sizeof(DD_HALINFO)) &&
        (pHalInfo->dwSize != sizeof(DD_HALINFO_V4)))
    {
        printf("8. if this show for NT 2000/XP/2003 ignore it, NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, pCallBackFlags, puD3dCallbacks, puD3dDriverData, puD3dBufferCallbacks, puD3dTextureFormats, NULL, ...);\n");
        fails++;
    }

    if (puD3dBufferCallbacks)
    {
        testing_noteq(puD3dBufferCallbacks->dwSize,sizeof(DD_D3DBUFCALLBACKS),fails,"9. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, pCallBackFlags, puD3dCallbacks, puD3dDriverData, puD3dBufferCallbacks, puD3dTextureFormats, NULL...);\0");
    }

    if (puD3dTextureFormats)
    {
        /* fixme test case for it */
    }

    if (dumping_on == TRUE)
    {
        dump_halinfo(pHalInfo,"NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, pCallBackFlags, NULL, ...)");
        dump_CallBackFlags(pCallBackFlags,"NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, pCallBackFlags, NULL, ...)");
        /* FIXME dump puD3dCallbacks */
        /* FIXME dump puD3dDriverData */
        /* FIXME dump D3dBufferCallbacks */
        /* FIXME dump puD3dTextureFormats */
    }





    if (puD3dTextureFormats)
        free (puD3dTextureFormats);
    show_status(fails, "NtGdiDdQueryDirectDrawObject\0");
}