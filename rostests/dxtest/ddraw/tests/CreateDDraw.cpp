#include "ddrawtest.h"

HWND CreateBasicWindow (VOID);

BOOL Test_CreateDDraw (INT* passed, INT* failed)
{
	LPDIRECTDRAW7 DirectDraw;
	IDirectDraw* DirectDraw2;

	/*** FIXME: Test first parameter using EnumDisplayDrivers  ***/

	TEST (DirectDrawCreateEx(NULL, (VOID**)&DirectDraw, IID_IDirectDraw7, (IUnknown*)0xdeadbeef) == CLASS_E_NOAGGREGATION);
	TEST (DirectDrawCreateEx(NULL, (VOID**)&DirectDraw, IID_IDirectDraw4, NULL) == DDERR_INVALIDPARAMS);
	TEST (DirectDrawCreateEx(NULL, NULL, IID_IDirectDraw7, NULL) == DDERR_INVALIDPARAMS); 
	TEST (DirectDrawCreateEx(NULL, (VOID**)&DirectDraw, IID_IDirectDraw7, NULL) == DD_OK);
	TEST (DirectDrawCreate(NULL ,&DirectDraw2, NULL) == DD_OK);

	return TRUE;
}

BOOL Test_SetCooperativeLevel (INT* passed, INT* failed)
{
	HWND hwnd; 
	LPDIRECTDRAW7 DirectDraw;
	
	/* Preparations */
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
	
	/* The Test */
	TEST ( DirectDraw->SetCooperativeLevel (NULL, DDSCL_FULLSCREEN) == DDERR_INVALIDPARAMS );
	TEST ( DirectDraw->SetCooperativeLevel (NULL, DDSCL_FULLSCREEN | DDSCL_EXCLUSIVE) == DDERR_INVALIDPARAMS );
	TEST ( DirectDraw->SetCooperativeLevel (hwnd, DDSCL_FULLSCREEN) == DDERR_INVALIDPARAMS);
	TEST ( DirectDraw->SetCooperativeLevel (hwnd, DDSCL_NORMAL | DDSCL_ALLOWMODEX) == DDERR_INVALIDPARAMS );
	TEST ( DirectDraw->SetCooperativeLevel ((HWND)0xdeadbeef, DDSCL_NORMAL) == DDERR_INVALIDPARAMS);

	TEST ( DirectDraw->SetCooperativeLevel (hwnd, DDSCL_FULLSCREEN | DDSCL_EXCLUSIVE) == DD_OK);
	TEST ( DirectDraw->SetCooperativeLevel (hwnd, DDSCL_FULLSCREEN | DDSCL_EXCLUSIVE | DDSCL_ALLOWMODEX) == DD_OK);
	TEST ( DirectDraw->SetCooperativeLevel (NULL, DDSCL_NORMAL) == DD_OK );
	TEST ( DirectDraw->SetCooperativeLevel (hwnd, DDSCL_NORMAL) == DD_OK );

	DirectDraw->Release();

	return TRUE;
}

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
