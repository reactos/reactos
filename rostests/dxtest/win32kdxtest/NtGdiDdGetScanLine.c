#include <stdio.h>
/* SDK/DDK/NDK Headers. */
#include <windows.h>
#include <wingdi.h>
#include <winddi.h>
#include <d3dnthal.h>
#include <dll/directx/d3d8thk.h>
#include "test.h"

/*
 * Test see if we can delete a DirectDrawObject from win32k
 *
 */
void
test_NtGdiDdGetScanLine(HANDLE hDirectDrawLocal)
{
    int fails=0;
    BOOL retValue=FALSE;
    DD_GETSCANLINEDATA puGetScanLineData;

    printf("Start testing of NtGdiDdGetScanLine\n");
    RtlZeroMemory(&puGetScanLineData,sizeof(DD_GETSCANLINEDATA));

    retValue = OsThunkDdGetScanLine(NULL,NULL);
    testing_eq(retValue, DDHAL_DRIVER_HANDLED,fails,"1. NtGdiDdGetScanLine(NULL,NULL);\0");

    retValue = OsThunkDdGetScanLine(hDirectDrawLocal,NULL);
    testing_eq(retValue, DDHAL_DRIVER_HANDLED,fails,"2. NtGdiDdGetScanLine(hDirectDrawLocal,NULL);\0");

    puGetScanLineData.ddRVal = DDERR_GENERIC;
    retValue = OsThunkDdGetScanLine(hDirectDrawLocal,&puGetScanLineData);
    testing_eq(retValue,DDHAL_DRIVER_NOTHANDLED,fails,"3. NtGdiDdGetScanLine(hDirectDrawLocal,puGetScanLineData);\0");
    testing_noteq(puGetScanLineData.ddRVal,DD_OK,fails,"4. NtGdiDdGetScanLine(hDirectDrawLocal,puGetScanLineData);\0");
    testing_eq(puGetScanLineData.dwScanLine,0,fails,"4. NtGdiDdGetScanLine(hDirectDrawLocal,puGetScanLineData);\0");


    /* FIXME DDERR_VERTICALBLANKINPROGRESS test */

    show_status(fails, "NtGdiDdGetScanLine\0");
}
