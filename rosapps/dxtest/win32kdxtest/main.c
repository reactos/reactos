
#include <stdio.h>
/* SDK/DDK/NDK Headers. */
#include <windows.h>
#include <wingdi.h>
#include <winddi.h>
#include <d3dnthal.h>




#include "test.h"

/* which syscall table shall we use WIndows or ReactOS */

/* Windows 2000 sp4 syscall table for win32k */
#include "Windows2000Sp4.h" 

/* Windows syscall code */
#include "Windowsos.h"

/* ReactOS syscall code */
#include "sysreactos.h"


/*
#define DdQueryDirectDrawObject             GdiEntry2

#define DdCreateSurfaceObject               GdiEntry4
#define DdDeleteSurfaceObject               GdiEntry5
#define DdResetVisrgn                       GdiEntry6
#define DdGetDC                             GdiEntry7
#define DdReleaseDC                         GdiEntry8
#define DdCreateDIBSection                  GdiEntry9
#define DdReenableDirectDrawObject          GdiEntry10
#define DdAttachSurface                     GdiEntry11
#define DdUnattachSurface                   GdiEntry12
#define DdQueryDisplaySettingsUniqueness    GdiEntry13
#define DdGetDxHandle                       GdiEntry14
#define DdSetGammaRamp                      GdiEntry15
#define DdSwapTextureHandles                GdiEntry16
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
    
    retValue = sysNtGdiDdCreateDirectDrawObject(NULL);
    testing_noteq(retValue,NULL,fails,"NtGdiDdCreateDirectDrawObject(NULL);\0");

    retValue = sysNtGdiDdCreateDirectDrawObject(hdc);
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

    printf("Start testing of NtGdiDdQueryDirectDrawObject\n");
    
    /* testing NULL */
    retValue = sysNtGdiDdQueryDirectDrawObject( NULL, pHalInfo, 
                                                pCallBackFlags, puD3dCallbacks, 
                                                puD3dDriverData, puD3dBufferCallbacks, 
                                                puD3dTextureFormats, puNumHeaps, 
                                                puvmList, puNumFourCC,
                                                puFourCC);
    testing_noteq(retValue,0,fails,"1. NtGdiDdQueryDirectDrawObject(NULL, ...);\0");
    testing_noteq(pHalInfo,NULL,fails,"2. NtGdiDdQueryDirectDrawObject(NULL, ...);\0");
    testing_noteq(pCallBackFlags,NULL,fails,"3. NtGdiDdQueryDirectDrawObject(NULL, ...);\0");
    testing_noteq(puD3dCallbacks,NULL,fails,"4. NtGdiDdQueryDirectDrawObject(NULL, ...);\0");
    testing_noteq(puD3dDriverData,NULL,fails,"5. NtGdiDdQueryDirectDrawObject(NULL, ...);\0");
    testing_noteq(puD3dBufferCallbacks,NULL,fails,"6. NtGdiDdQueryDirectDrawObject(NULL, ...);\0");
    testing_noteq(puD3dTextureFormats,NULL,fails,"7. NtGdiDdQueryDirectDrawObject(NULL, ...);\0");
    testing_noteq(puNumFourCC,NULL,fails,"8. NtGdiDdQueryDirectDrawObject(NULL, ...);\0");
    testing_noteq(puFourCC,NULL,fails,"9. NtGdiDdQueryDirectDrawObject(NULL, ...);\0");

    retValue = sysNtGdiDdQueryDirectDrawObject( hDirectDrawLocal, pHalInfo, 
                                                pCallBackFlags, puD3dCallbacks, 
                                                puD3dDriverData, puD3dBufferCallbacks, 
                                                puD3dTextureFormats, puNumHeaps, 
                                                puvmList, puNumFourCC,
                                                puFourCC);

    testing_noteq(retValue,0,fails,"1. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, NULL, ...);\0");
    testing_noteq(pHalInfo,NULL,fails,"2. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, NULL, ...);\0");
    testing_noteq(pCallBackFlags,NULL,fails,"3. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, NULL, ...);\0");
    testing_noteq(puD3dCallbacks,NULL,fails,"4. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, NULL, ...);\0");
    testing_noteq(puD3dDriverData,NULL,fails,"5. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, NULL, ...);\0");
    testing_noteq(puD3dBufferCallbacks,NULL,fails,"6. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, NULL, ...);\0");
    testing_noteq(puD3dTextureFormats,NULL,fails,"7. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, NULL, ...);\0");
    testing_noteq(puNumFourCC,NULL,fails,"8. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, NULL, ...);\0");
    testing_noteq(puFourCC,NULL,fails,"9. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, NULL, ...);\0");




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
    
    retValue = sysNtGdiDdDeleteDirectDrawObject(hDirectDrawLocal);
    testing_eq(retValue,FALSE,fails,"NtGdiDdDeleteDirectDrawObject(hDirectDrawLocal);\0");

    retValue = sysNtGdiDdDeleteDirectDrawObject(NULL);
    testing_eq(retValue,TRUE,fails,"NtGdiDdDeleteDirectDrawObject(NULL);\0");

    show_status(fails, "NtGdiDdDeleteDirectDrawObject\0");
}







