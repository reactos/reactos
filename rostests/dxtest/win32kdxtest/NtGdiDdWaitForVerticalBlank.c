#include <stdio.h>
/* SDK/DDK/NDK Headers. */
#include <windows.h>
#include <wingdi.h>
#include <winddi.h>
#include <d3dnthal.h>
#include <dll/directx/d3d8thk.h>

#include <ddrawi.h>
#include "test.h"

/*
 * Test see if we can delete a DirectDrawObject from win32k
 *
 */
void
test_NtGdiDdWaitForVerticalBlank(HANDLE hDirectDrawLocal)
{
    int fails=0;
    BOOL retValue=FALSE;
    DDHAL_WAITFORVERTICALBLANKDATA pDdWaitForVerticalBlankData;
    
    RtlZeroMemory(&pDdWaitForVerticalBlankData,sizeof(DDHAL_WAITFORVERTICALBLANKDATA));

    retValue = OsThunkDdWaitForVerticalBlank(NULL,NULL);
    testing_eq(retValue, DDHAL_DRIVER_HANDLED,fails,"1. NtGdiDdWaitForVerticalBlank(NULL,NULL);\0");

    retValue = OsThunkDdWaitForVerticalBlank(hDirectDrawLocal,NULL);
    testing_eq(retValue, DDHAL_DRIVER_HANDLED,fails,"2. NtGdiDdWaitForVerticalBlank(hDirectDrawLocal,NULL);\0");

    retValue = OsThunkDdWaitForVerticalBlank(hDirectDrawLocal,(PDD_WAITFORVERTICALBLANKDATA)&pDdWaitForVerticalBlankData);
    testing_eq(retValue, DDHAL_DRIVER_HANDLED,fails,"3. NtGdiDdWaitForVerticalBlank(hDirectDrawLocal,NULL);\0");
    testing_eq(pDdWaitForVerticalBlankData.ddRVal, DD_OK,fails,"4. NtGdiDdWaitForVerticalBlank(hDirectDrawLocal,NULL);\0");

    RtlZeroMemory(&pDdWaitForVerticalBlankData,sizeof(DDHAL_WAITFORVERTICALBLANKDATA));
    pDdWaitForVerticalBlankData.dwFlags = DDWAITVB_I_TESTVB;
    retValue = OsThunkDdWaitForVerticalBlank(hDirectDrawLocal,(PDD_WAITFORVERTICALBLANKDATA)&pDdWaitForVerticalBlankData);

    testing_eq(retValue, DDHAL_DRIVER_NOTHANDLED,fails,"5. NtGdiDdWaitForVerticalBlank(hDirectDrawLocal,NULL);\0");
    testing_noteq(pDdWaitForVerticalBlankData.ddRVal, DD_OK,fails,"6. NtGdiDdWaitForVerticalBlank(hDirectDrawLocal,NULL);\0");

    retValue = OsThunkDdWaitForVerticalBlank(hDirectDrawLocal,(PDD_WAITFORVERTICALBLANKDATA)&pDdWaitForVerticalBlankData);

    show_status(fails, "NtGdiDdWaitForVerticalBlank\0");
}

