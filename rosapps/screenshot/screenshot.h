#include <windows.h>
#include <tchar.h>

typedef struct _SCREENSHOT
{
    HWND hSelf;
    HDC hDC;
    HBITMAP hBitmap;
    LPBITMAPINFO lpbi;
    LPVOID lpvBits;
    INT Width;
    INT Height;
} SCREENSHOT, *PSCREENSHOT;

//INT WINAPI GetScreenshot(VOID);
