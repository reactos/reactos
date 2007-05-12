
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
test_NtGdiDdCanCreateSurface(HANDLE hDirectDrawLocal)
{
    int fails=0;
    BOOL retValue=FALSE;
    DDHAL_CANCREATESURFACEDATA pCanCreateSurface;
    DDSURFACEDESC2 desc;
    
    RtlZeroMemory(&pCanCreateSurface,sizeof(DDHAL_CANCREATESURFACEDATA));
    RtlZeroMemory(&desc,sizeof(DDSURFACEDESC2));
    
    /* crash in windows 2000 */
    retValue = OsThunkDdCanCreateSurface(NULL,NULL);
    testing_eq(retValue, DDHAL_DRIVER_HANDLED,fails,"1. NtGdiDdCanCreateSurface(NULL,NULL);\0");

    retValue = OsThunkDdCanCreateSurface(hDirectDrawLocal,NULL);
    testing_eq(retValue, DDHAL_DRIVER_HANDLED,fails,"2. NtGdiDdCanCreateSurface(hDirectDrawLocal,NULL);\0");

    retValue = OsThunkDdCanCreateSurface(hDirectDrawLocal,(PDD_CANCREATESURFACEDATA)&pCanCreateSurface);
    testing_eq(retValue, DDHAL_DRIVER_HANDLED,fails,"3. NtGdiDdCanCreateSurface(hDirectDrawLocal,pCanCreateSurface);\0");

    pCanCreateSurface.lpDDSurfaceDesc = &desc;
    desc.dwSize = sizeof(DDSURFACEDESC2);

    retValue = OsThunkDdCanCreateSurface(hDirectDrawLocal,(PDD_CANCREATESURFACEDATA)&pCanCreateSurface);
    testing_eq(retValue, DDHAL_DRIVER_HANDLED,fails,"4. NtGdiDdCanCreateSurface(hDirectDrawLocal,pCanCreateSurface);\0");

}


