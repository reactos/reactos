
#include <windows.h>
#include <dciman.h>

CRITICAL_SECTION ddcs;

BOOL
WINAPI
DllMain( HINSTANCE hModule, DWORD ul_reason_for_call, LPVOID reserved )
{
    switch(ul_reason_for_call)
    {
        case DLL_PROCESS_DETACH:
            DeleteCriticalSection( &ddcs );
            break;

        case DLL_PROCESS_ATTACH:
            InitializeCriticalSection( &ddcs );
            break;
    }
    return TRUE;
}

/*++
* @name HDC WINAPI DCIOpenProvider(void)
* @implemented
*
* The function DCIOpenProvider are almost same as CreateDC, it is more simple to use,
* you do not need type which monitor or graphic card you want to use, it will use
*  the current one that the program is running on, so it simple create a hdc, and
*  you can use gdi32 api without any problem

* @return
* Returns a hdc that it have create by gdi32.dll CreateDC
*
* @remarks.
* None
*
*--*/
HDC WINAPI
DCIOpenProvider(void)
{
    HDC hDC;
    if (GetSystemMetrics(SM_CMONITORS) > 1)
    {
        /* FIXME we need add more that one mointor support for DCIOpenProvider, need exaime how */
        SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
        return NULL;
    }
    else
    {
        /* Note Windows using the word "Display" not with big letter to create the hdc
         * MS gdi32 does not care if the Display spell with big or small letters */

        /* Note it only create a hDC handle and send it back. */
        hDC = CreateDCW(L"Display",NULL, NULL, NULL);
    }

    return hDC;
}

/*++
* @name void WINAPI DCICloseProvider(HDC hdc)
* @implemented
*
* Same as DeleteDC, it call indirecly to DeleteDC, it simple free and destory the hdc.

* @return
* None
*
* @remarks.
* None
*
*--*/
void WINAPI
DCICloseProvider(HDC hDC)
{
    /* Note it only delete the hdc */
    DeleteDC(hDC);
}


int WINAPI 
DCICreatePrimary(HDC hdc, LPDCISURFACEINFO *pDciSurfaceInfo)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return DCI_FAIL_UNSUPPORTED;
}



