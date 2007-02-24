

/* All testcase are base how windows 2000 sp4 acting */


#include <stdio.h>
/* SDK/DDK/NDK Headers. */
#include <windows.h>
#include <wingdi.h>
#include <winddi.h>
#include <d3dnthal.h>
#include <dll/directx/d3d8thk.h>
#include "test.h"

/* we using d3d8thk.dll it is doing the real syscall in windows 2000 
 * in ReactOS and Windows XP and higher d3d8thk.dll it linking to
 * gdi32.dll instead doing syscall, gdi32.dll export DdEntry1-56 
 * and doing the syscall direcly. I did forget about it, This 
 * test program are now working on any Windows and ReactOS 
 * that got d3d8thk.dll
 */

int main(int argc, char **argv)
{
    HANDLE hDirectDrawLocal;

    hDirectDrawLocal = test_NtGdiDdCreateDirectDrawObject();

    test_NtGdiDdQueryDirectDrawObject(hDirectDrawLocal);

    test_NtGdiDdDeleteDirectDrawObject(hDirectDrawLocal);
    return 0;
}

/*
 * Test see if we getting a DirectDrawObject from win32k
 *
 */
HANDLE
test_NtGdiDdCreateDirectDrawObject()
{
    HANDLE retValue=0;
    int fails=0;
    HDC hdc=CreateDCW(L"Display",NULL,NULL,NULL);

    printf("Start testing of NtGdiDdCreateDirectDrawObject\n");
    
    retValue = OsThunkDdCreateDirectDrawObject(NULL);
    testing_noteq(retValue,NULL,fails,"NtGdiDdCreateDirectDrawObject(NULL);\0");

    retValue = OsThunkDdCreateDirectDrawObject(hdc);
    testing_eq(retValue,NULL,fails,"NtGdiDdCreateDirectDrawObject(hdc);\0");

    show_status(fails, "NtGdiDdCreateDirectDrawObject\0");

    return retValue;
}

/*
 * Test see if we can setup DirectDrawObject 
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
    DDSURFACEDESC D3dTextureFormats;
    // DWORD NumHeaps = 0;
    VIDEOMEMORY vmList;
    // DWORD NumFourCC = 0;
    //DWORD FourCC = 0;

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
    printf("testing  OsThunkDdQueryDirectDrawObject( NULL, ....)\n");

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
    printf("testing  OsThunkDdQueryDirectDrawObject( hDirectDrawLocal, NULL, ....)\n");

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
    printf("testing  OsThunkDdQueryDirectDrawObject( hDirectDrawLocal, pHalInfo, NULL, ....)\n");

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
        printf("10. if this show for NT 2000/XP/2003 ignore it, NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, NULL, ...);\n");
    }

    /* FIXME dump pHalInfo */

    /* testing  OsThunkDdQueryDirectDrawObject( hDirectDrawLocal, pHalInfo, pCallBackFlags, NULL, ....  */
    printf("testing  OsThunkDdQueryDirectDrawObject( hDirectDrawLocal, pHalInfo, pCallBackFlags, NULL, ....)\n");

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
    testing_noteq(puNumFourCC,NULL,fails,"8. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, NULL, ...);\0");
    testing_noteq(puFourCC,NULL,fails,"9. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, NULL, ...);\0");
    if ((pHalInfo->dwSize != sizeof(DD_HALINFO)) &&
        (pHalInfo->dwSize != sizeof(DD_HALINFO_V4)))
    {
        printf("10. if this show for NT 2000/XP/2003 ignore it, NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, NULL, ...);\n");
    }

    /* FIXME dump pHalInfo */
    /* FIXME dump pCallBackFlags */


    show_status(fails, "NtGdiDdQueryDirectDrawObject\0");
}

/*
 * Test see if we can delete a DirectDrawObject from win32k
 *
 */
void
test_NtGdiDdDeleteDirectDrawObject(HANDLE hDirectDrawLocal)
{
    int fails=0;
    BOOL retValue=FALSE;
    printf("Start testing of NtGdiDdDeleteDirectDrawObject\n");
    
    retValue = OsThunkDdDeleteDirectDrawObject(hDirectDrawLocal);
    testing_eq(retValue,FALSE,fails,"NtGdiDdDeleteDirectDrawObject(hDirectDrawLocal);\0");

    retValue = OsThunkDdDeleteDirectDrawObject(NULL);
    testing_eq(retValue,TRUE,fails,"NtGdiDdDeleteDirectDrawObject(NULL);\0");

    show_status(fails, "NtGdiDdDeleteDirectDrawObject\0");
}







