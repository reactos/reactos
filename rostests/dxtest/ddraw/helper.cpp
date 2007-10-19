#include "ddrawtest.h"

LONG WINAPI BasicWindowProc (HWND hwnd, UINT message, UINT wParam, LONG lParam)
{
	switch (message)
	{
		case WM_DESTROY:
		{
			PostQuitMessage (0);
			return 0;
		} break;
	}

	return DefWindowProc (hwnd, message, wParam, lParam);
}

HWND CreateBasicWindow (VOID)
{
	WNDCLASS wndclass = {0};
	wndclass.lpfnWndProc   = BasicWindowProc;
	wndclass.hInstance     = GetModuleHandle(NULL);
	wndclass.lpszClassName = "DDrawTest";
	RegisterClass(&wndclass);

	return CreateWindow("DDrawTest", "ReactOS DirectDraw Test", WS_POPUP, 0, 0, 10, 10, NULL, NULL, GetModuleHandle(NULL), NULL);
}

BOOL CreateSurface(LPDIRECTDRAWSURFACE7* pSurface)
{
	LPDIRECTDRAW7 DirectDraw;
	LPDIRECTDRAWSURFACE7 Surface;
    HWND hwnd;

	// Create DDraw Object
	if (DirectDrawCreateEx(NULL, (VOID**)&DirectDraw, IID_IDirectDraw7, NULL) != DD_OK)
	{
		printf("ERROR: Failed to set up ddraw\n");
		return FALSE;
	}

	if(!( hwnd = CreateBasicWindow() ))
	{
		printf("ERROR: Failed to create window\n");
		DirectDraw->Release();
		return FALSE;
	}

	if (DirectDraw->SetCooperativeLevel (hwnd, DDSCL_NORMAL) != DD_OK)
	{
		printf("ERROR: Could not set cooperative level\n");
		DirectDraw->Release();
		return 0;
	}

    // Creat Surface
	DDSURFACEDESC2 Desc = { 0 };
	Desc.dwHeight = 200;
	Desc.dwWidth = 200;
    Desc.dwSize = sizeof (DDSURFACEDESC2);
    Desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
	Desc.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;

    if(DirectDraw->CreateSurface(&Desc, &Surface, NULL) != DD_OK)
    {
        printf("ERROR: Faild to create Surface\n");
        return FALSE;
    }

    DirectDraw->Release();

    *pSurface = Surface;
    return TRUE;
}
