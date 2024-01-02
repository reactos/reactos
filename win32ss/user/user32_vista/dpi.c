#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H
#define NTOS_MODE_USER
#include <windef.h>
#include <winuser.h>

/*
 * @stub
*/
UINT WINAPI GetDpiForSystem()
{
    return USER_DEFAULT_SCREEN_DPI;
}

/*
 * @stub
*/
UINT WINAPI GetDpiForWindow(_In_ HWND hWnd)
{
    UNREFERENCED_PARAMETER(hWnd);
    return GetDpiForSystem();
}
