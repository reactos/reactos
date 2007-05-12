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

