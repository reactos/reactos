/*
 * Implementation of DCIMAN32 - DCI Manager
 */

#include <precomp.h>

BOOL
WINAPI
DllMain( HINSTANCE inst, DWORD reason, LPVOID reserved )
{
    return TRUE;
}


/*
 * @unimplemented
 */
DCMAIN32SDKAPI
DCIRVAL
WINAPI
DCIBeginAccess(
    LPDCISURFACEINFO pdci,
    int x,
    int y,
    int dx,
    int dy)
{
    UNIMPLEMENTED
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return DCI_FAIL_UNSUPPORTED;
}


/*
 * @implemented
 */
DCMAIN32SDKAPI
HDC
WINAPI
DCIOpenProvider()
{
    HDC hdc = NULL;

    if ( (GetSystemMetrics(SM_CMONITORS) == 1) ||
         (GetSystemMetrics(SM_CMONITORS) == 0) )
    {
        hdc = CreateDCW(L"DISPLAY",0,0,0);
    }

    return hdc;
}

/*
 * @implemented
 */
DCMAIN32SDKAPI
void 
WINAPI 
DCICloseProvider(HDC hdc)
{
    DeleteDC(hdc);
}

/*
 * @implemented
 * Note : DCICreateOffscreen always return  DCI_FAIL_UNSUPPORTED what every it u try todo 
 */
DCMAIN32SDKAPI
int
WINAPI
DCICreateOffscreen(
    HDC hdc,
    DWORD dwCompression,
    DWORD dwRedMask,
    DWORD dwGreenMask,
    DWORD dwBlueMask,
    DWORD dwWidth,
    DWORD dwHeight,
    DWORD dwDCICaps,
    DWORD dwBitCount,
    LPDCIOFFSCREEN FAR *lplpSurface)
{
    return DCI_FAIL_UNSUPPORTED;
}

/*
 * @implemented
 * Note : DCICreateOverlay always return  DCI_FAIL_UNSUPPORTED what every it u try todo 
 */
DCMAIN32SDKAPI
int
WINAPI
DCICreateOverlay(
    HDC hdc,
    LPVOID lpOffscreenSurf,
    LPDCIOVERLAY FAR *lplpSurface)
{
    return DCI_FAIL_UNSUPPORTED;
}


/*
 * @unimplemented
 */
DCMAIN32SDKAPI
int
WINAPI
DCICreatePrimary(
    HDC hdc,
    LPDCISURFACEINFO FAR *lplpSurface)
{

    UNIMPLEMENTED
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return DCI_FAIL_UNSUPPORTED;
}

/*
 * @unimplemented
 */
DCMAIN32SDKAPI
void
WINAPI DCIDestroy(LPDCISURFACEINFO pdci)
{
    UNIMPLEMENTED
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
}

/*
 * @implemented
 * Note : DCIDraw always return  DCI_FAIL_UNSUPPORTED what every it u try todo 
 */
DCMAIN32SDKAPI
DCIRVAL
WINAPI
DCIDraw(LPDCIOFFSCREEN pdci)
{
    return DCI_FAIL_UNSUPPORTED;
}

/*
 * @unimplemented
 */
DCMAIN32SDKAPI
void
WINAPI
DCIEndAccess(LPDCISURFACEINFO pdci)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
}

/*
 * @implemented
 * Note : DCIEnum always return  DCI_FAIL_UNSUPPORTED what every it u try todo 
 */
DCMAIN32SDKAPI
int
WINAPI
DCIEnum(
    HDC hdc,
    LPRECT lprDst,
    LPRECT lprSrc,
    LPVOID lpFnCallback,
    LPVOID lpContext)
{
    return DCI_FAIL_UNSUPPORTED;
}

/*
 * @implemented
 * Note : DCIEnum always return  DCI_FAIL_UNSUPPORTED what every it u try todo 
 */
DCMAIN32SDKAPI
DCIRVAL
WINAPI
DCISetClipList(
    LPDCIOFFSCREEN pdci,
    LPRGNDATA prd)
{
    return DCI_FAIL_UNSUPPORTED;
}

/*
 * @unimplemented
 * Note : DCIEnum always return  DCI_FAIL_UNSUPPORTED what every it u try todo 
 */
DCMAIN32SDKAPI
DCIRVAL
WINAPI
DCISetSrcDestClip(
    LPDCIOFFSCREEN pdci,
    LPRECT srcrc,
    LPRECT destrc,
    LPRGNDATA prd )
{
    return DCI_FAIL_UNSUPPORTED;
}

/*
 * @unimplemented
 */
DCMAIN32SDKAPI
DWORD
WINAPI
GetDCRegionData(
    HDC hdc,
    DWORD size,
    LPRGNDATA prd)
{
    UNIMPLEMENTED
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
DCMAIN32SDKAPI
DWORD
WINAPI
GetWindowRegionData(
    HWND hwnd,
    DWORD size,
    LPRGNDATA prd)
{
    UNIMPLEMENTED
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
DCMAIN32SDKAPI
void
WINAPI
WinWatchClose(HWINWATCH hWW)
{
    UNIMPLEMENTED
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
}

/*
 * @unimplemented
 */
DCMAIN32SDKAPI
BOOL
WINAPI
WinWatchDidStatusChange(HWINWATCH hWW)
{
    UNIMPLEMENTED
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
DCMAIN32SDKAPI
UINT
WINAPI
WinWatchGetClipList(
    HWINWATCH hWW,
    LPRECT prc,
    UINT size,
    LPRGNDATA prd)
{
    UNIMPLEMENTED
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
DCMAIN32SDKAPI
BOOL
WINAPI
WinWatchNotify(
    HWINWATCH hWW,
    WINWATCHNOTIFYPROC NotifyCallback,
    LPARAM NotifyParam )
{
    UNIMPLEMENTED
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
DCMAIN32SDKAPI
HWINWATCH
WINAPI
WinWatchOpen(HWND hwnd)
{
    UNIMPLEMENTED
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


