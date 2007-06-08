#include "ddrawtest.h"

HWND CreateBasicWindow (VOID);

BOOL Test_CreateDDraw (INT* passed, INT* failed)
{
	LPDIRECTDRAW7 DirectDraw;
	IDirectDraw* DirectDraw2;

	/*** FIXME: Test first parameter using EnumDisplayDrivers  ***/
	DirectDrawCreateEx(NULL, (VOID**)&DirectDraw, IID_IDirectDraw7, NULL);

	TEST (DirectDrawCreateEx(NULL, (VOID**)&DirectDraw, IID_IDirectDraw7, (IUnknown*)0xdeadbeef) == CLASS_E_NOAGGREGATION);
	TEST (DirectDrawCreateEx(NULL, (VOID**)&DirectDraw, IID_IDirectDraw4, NULL) == DDERR_INVALIDPARAMS);
	TEST (DirectDrawCreateEx(NULL, NULL, IID_IDirectDraw7, NULL) == DDERR_INVALIDPARAMS); 

	DirectDraw = NULL;
	TEST (DirectDrawCreateEx(NULL, (VOID**)&DirectDraw, IID_IDirectDraw7, NULL) == DD_OK);
	if(DirectDraw)
	{
		TEST (DirectDraw->Initialize(NULL) == DDERR_ALREADYINITIALIZED);
		TEST (DirectDraw->Release() == 0);
	}

	DirectDraw2 = NULL;
	TEST (DirectDrawCreate(NULL ,&DirectDraw2, NULL) == DD_OK);
	if(DirectDraw2)
	{
		TEST (DirectDraw2->QueryInterface(IID_IDirectDraw7, (PVOID*)&DirectDraw) == 0);
		TEST (DirectDraw2->AddRef() == 2);
		TEST (DirectDraw->AddRef() == 2);
		TEST (DirectDraw->Release() == 1);
		TEST (DirectDraw->Release() == 0);
		TEST (DirectDraw2->Release() == 1);
		TEST (DirectDraw2->Release() == 0);
	}

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
	TEST ( DirectDraw->SetCooperativeLevel (hwnd, DDSCL_FULLSCREEN) == DDERR_INVALIDPARAMS );
	TEST ( DirectDraw->SetCooperativeLevel (NULL, DDSCL_FULLSCREEN | DDSCL_EXCLUSIVE) == DDERR_INVALIDPARAMS );
	TEST ( DirectDraw->SetCooperativeLevel (hwnd, DDSCL_FULLSCREEN) == DDERR_INVALIDPARAMS);
	TEST ( DirectDraw->SetCooperativeLevel (hwnd, DDSCL_NORMAL | DDSCL_ALLOWMODEX) == DDERR_INVALIDPARAMS );
	TEST ( DirectDraw->SetCooperativeLevel ((HWND)0xdeadbeef, DDSCL_NORMAL) == DDERR_INVALIDPARAMS);

	TEST ( DirectDraw->SetCooperativeLevel (hwnd, DDSCL_FULLSCREEN | DDSCL_EXCLUSIVE) == DD_OK);
	TEST ( DirectDraw->Compact() == DD_OK );
	TEST ( DirectDraw->SetCooperativeLevel (hwnd, DDSCL_FULLSCREEN | DDSCL_EXCLUSIVE | DDSCL_ALLOWMODEX) == DD_OK);
	TEST ( DirectDraw->SetCooperativeLevel (NULL, DDSCL_NORMAL) == DD_OK );
	TEST ( DirectDraw->SetCooperativeLevel (hwnd, DDSCL_NORMAL) == DD_OK );
	TEST ( DirectDraw->Compact() == DDERR_NOEXCLUSIVEMODE );

	TEST ( DirectDraw->TestCooperativeLevel() == DD_OK ); // I do not get what this API does it always seems to return DD_OK

	DirectDraw->Release();

	return TRUE;
}

BOOL Test_GetAvailableVidMem (INT* passed, INT* failed)
{
	LPDIRECTDRAW7 DirectDraw;

	/* Preparations */
	if (DirectDrawCreateEx(NULL, (VOID**)&DirectDraw, IID_IDirectDraw7, NULL) != DD_OK)
	{
		printf("ERROR: Failed to set up ddraw\n");
		return FALSE;
	}

	/* Here we go */
	DWORD Total, Free;
	DDSCAPS2 Caps = { 0 };

	TEST (DirectDraw->GetAvailableVidMem(&Caps, NULL, NULL) == DDERR_INVALIDPARAMS);
	TEST (DirectDraw->GetAvailableVidMem(NULL, &Total, &Free) == DDERR_INVALIDPARAMS);
	TEST (DirectDraw->GetAvailableVidMem(&Caps, &Total, &Free) == DD_OK);


    /* testing caps */
	memset(&Caps,0,sizeof(DDSCAPS2));
	Caps.dwCaps = DDSCAPS_RESERVED1;
	TEST (DirectDraw->GetAvailableVidMem(&Caps, &Total, &Free) == DD_OK );

	memset(&Caps,0,sizeof(DDSCAPS2));
	Caps.dwCaps = DDSCAPS_ALPHA;
	TEST (DirectDraw->GetAvailableVidMem(&Caps, &Total, &Free) == DD_OK );

	memset(&Caps,0,sizeof(DDSCAPS2));
	Caps.dwCaps = DDSCAPS_BACKBUFFER;
	TEST (DirectDraw->GetAvailableVidMem(&Caps, &Total, &Free) == DDERR_INVALIDPARAMS );

	memset(&Caps,0,sizeof(DDSCAPS2));
	Caps.dwCaps = DDSCAPS_COMPLEX;
	TEST (DirectDraw->GetAvailableVidMem(&Caps, &Total, &Free) == DDERR_INVALIDPARAMS );

	memset(&Caps,0,sizeof(DDSCAPS2));
	Caps.dwCaps = DDSCAPS_FLIP;
	TEST (DirectDraw->GetAvailableVidMem(&Caps, &Total, &Free) == DDERR_INVALIDPARAMS );

	memset(&Caps,0,sizeof(DDSCAPS2));
	Caps.dwCaps = DDSCAPS_FRONTBUFFER;
	TEST (DirectDraw->GetAvailableVidMem(&Caps, &Total, &Free) == DDERR_INVALIDPARAMS );

	memset(&Caps,0,sizeof(DDSCAPS2));
	Caps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
	TEST (DirectDraw->GetAvailableVidMem(&Caps, &Total, &Free) == DD_OK );

	memset(&Caps,0,sizeof(DDSCAPS2));
	Caps.dwCaps = DDSCAPS_OVERLAY;
	TEST (DirectDraw->GetAvailableVidMem(&Caps, &Total, &Free) == DD_OK );

	memset(&Caps,0,sizeof(DDSCAPS2));
	Caps.dwCaps = DDSCAPS_PALETTE;
	TEST (DirectDraw->GetAvailableVidMem(&Caps, &Total, &Free) == DDERR_INVALIDPARAMS );

	memset(&Caps,0,sizeof(DDSCAPS2));
	Caps.dwCaps = DDSCAPS_PRIMARYSURFACE;
	TEST (DirectDraw->GetAvailableVidMem(&Caps, &Total, &Free) == DD_OK );

	memset(&Caps,0,sizeof(DDSCAPS2));
	Caps.dwCaps = DDSCAPS_RESERVED3;
	TEST (DirectDraw->GetAvailableVidMem(&Caps, &Total, &Free) == DD_OK );

	memset(&Caps,0,sizeof(DDSCAPS2));
	Caps.dwCaps = DDSCAPS_SYSTEMMEMORY;
	TEST (DirectDraw->GetAvailableVidMem(&Caps, &Total, &Free) == DDERR_INVALIDPARAMS );

	memset(&Caps,0,sizeof(DDSCAPS2));
	Caps.dwCaps = DDSCAPS_TEXTURE;
	TEST (DirectDraw->GetAvailableVidMem(&Caps, &Total, &Free) == DD_OK );

	memset(&Caps,0,sizeof(DDSCAPS2));
	Caps.dwCaps = DDSCAPS_3DDEVICE;
	TEST (DirectDraw->GetAvailableVidMem(&Caps, &Total, &Free) == DD_OK );

	memset(&Caps,0,sizeof(DDSCAPS2));
	Caps.dwCaps = DDSCAPS_VIDEOMEMORY;
	TEST (DirectDraw->GetAvailableVidMem(&Caps, &Total, &Free) == DD_OK );

	memset(&Caps,0,sizeof(DDSCAPS2));
	Caps.dwCaps = DDSCAPS_VISIBLE;
	TEST (DirectDraw->GetAvailableVidMem(&Caps, &Total, &Free) == DDERR_INVALIDPARAMS );

	memset(&Caps,0,sizeof(DDSCAPS2));
	Caps.dwCaps = DDSCAPS_WRITEONLY;
	TEST (DirectDraw->GetAvailableVidMem(&Caps, &Total, &Free) == DDERR_INVALIDPARAMS );

	memset(&Caps,0,sizeof(DDSCAPS2));
	Caps.dwCaps = DDSCAPS_ZBUFFER;
	TEST (DirectDraw->GetAvailableVidMem(&Caps, &Total, &Free) == DD_OK );

	memset(&Caps,0,sizeof(DDSCAPS2));
	Caps.dwCaps = DDSCAPS_OWNDC;
	TEST (DirectDraw->GetAvailableVidMem(&Caps, &Total, &Free) == DDERR_INVALIDPARAMS );

	memset(&Caps,0,sizeof(DDSCAPS2));
	Caps.dwCaps = DDSCAPS_LIVEVIDEO;
	TEST (DirectDraw->GetAvailableVidMem(&Caps, &Total, &Free) == DD_OK );

	memset(&Caps,0,sizeof(DDSCAPS2));
	Caps.dwCaps = DDSCAPS_HWCODEC;
	TEST (DirectDraw->GetAvailableVidMem(&Caps, &Total, &Free) == DD_OK );

	memset(&Caps,0,sizeof(DDSCAPS2));
	Caps.dwCaps = DDSCAPS_MODEX;
	TEST (DirectDraw->GetAvailableVidMem(&Caps, &Total, &Free) == DD_OK );

	memset(&Caps,0,sizeof(DDSCAPS2));
	Caps.dwCaps = DDSCAPS_MIPMAP;
	TEST (DirectDraw->GetAvailableVidMem(&Caps, &Total, &Free) == DD_OK );

	memset(&Caps,0,sizeof(DDSCAPS2));
	Caps.dwCaps = DDSCAPS_RESERVED2;
	TEST (DirectDraw->GetAvailableVidMem(&Caps, &Total, &Free) == DD_OK );

	memset(&Caps,0,sizeof(DDSCAPS2));
	Caps.dwCaps = DDSCAPS_ALLOCONLOAD;
	TEST (DirectDraw->GetAvailableVidMem(&Caps, &Total, &Free) == DD_OK );

	memset(&Caps,0,sizeof(DDSCAPS2));
	Caps.dwCaps = DDSCAPS_VIDEOPORT;
	TEST (DirectDraw->GetAvailableVidMem(&Caps, &Total, &Free) == DD_OK );

	memset(&Caps,0,sizeof(DDSCAPS2));
	Caps.dwCaps = DDSCAPS_LOCALVIDMEM;
	TEST (DirectDraw->GetAvailableVidMem(&Caps, &Total, &Free) == DD_OK);

	memset(&Caps,0,sizeof(DDSCAPS2));
	Caps.dwCaps = DDSCAPS_NONLOCALVIDMEM;
	TEST (DirectDraw->GetAvailableVidMem(&Caps, &Total, &Free) == DD_OK);

	memset(&Caps,0,sizeof(DDSCAPS2));
	Caps.dwCaps2 = 0x01;
	TEST (DirectDraw->GetAvailableVidMem(&Caps, &Total, &Free) == DDERR_INVALIDCAPS);

	memset(&Caps,0,sizeof(DDSCAPS2));
	Caps.dwCaps2 = DDSCAPS2_RESERVED4;
	TEST (DirectDraw->GetAvailableVidMem(&Caps, &Total, &Free) == DD_OK);

	memset(&Caps,0,sizeof(DDSCAPS2));
	Caps.dwCaps2 = DDSCAPS2_HINTDYNAMIC;
	TEST (DirectDraw->GetAvailableVidMem(&Caps, &Total, &Free) == DD_OK);

	memset(&Caps,0,sizeof(DDSCAPS2));
	Caps.dwCaps2 = DDSCAPS2_HINTSTATIC;
	TEST (DirectDraw->GetAvailableVidMem(&Caps, &Total, &Free) == DD_OK);

	memset(&Caps,0,sizeof(DDSCAPS2));
	Caps.dwCaps2 = DDSCAPS2_TEXTUREMANAGE;
	TEST (DirectDraw->GetAvailableVidMem(&Caps, &Total, &Free) == DD_OK);

	memset(&Caps,0,sizeof(DDSCAPS2));
	Caps.dwCaps2 = DDSCAPS2_RESERVED1;
	TEST (DirectDraw->GetAvailableVidMem(&Caps, &Total, &Free) == DD_OK);

	memset(&Caps,0,sizeof(DDSCAPS2));
	Caps.dwCaps2 = DDSCAPS2_RESERVED2;
	TEST (DirectDraw->GetAvailableVidMem(&Caps, &Total, &Free) == DD_OK);

	memset(&Caps,0,sizeof(DDSCAPS2));
	Caps.dwCaps2 = DDSCAPS2_OPAQUE;
	TEST (DirectDraw->GetAvailableVidMem(&Caps, &Total, &Free) == DD_OK);

	memset(&Caps,0,sizeof(DDSCAPS2));
	Caps.dwCaps2 = DDSCAPS2_HINTANTIALIASING;
	TEST (DirectDraw->GetAvailableVidMem(&Caps, &Total, &Free) == DD_OK);

	memset(&Caps,0,sizeof(DDSCAPS2));
	Caps.dwCaps2 = DDSCAPS2_CUBEMAP;
	TEST (DirectDraw->GetAvailableVidMem(&Caps, &Total, &Free) == DD_OK);

	memset(&Caps,0,sizeof(DDSCAPS2));
	Caps.dwCaps2 = DDSCAPS2_CUBEMAP_POSITIVEX;
	TEST (DirectDraw->GetAvailableVidMem(&Caps, &Total, &Free) == DD_OK);

	memset(&Caps,0,sizeof(DDSCAPS2));
	Caps.dwCaps2 = DDSCAPS2_CUBEMAP_NEGATIVEX;
	TEST (DirectDraw->GetAvailableVidMem(&Caps, &Total, &Free) == DD_OK);

	memset(&Caps,0,sizeof(DDSCAPS2));
	Caps.dwCaps2 = DDSCAPS2_CUBEMAP_POSITIVEY;
	TEST (DirectDraw->GetAvailableVidMem(&Caps, &Total, &Free) == DD_OK);

	memset(&Caps,0,sizeof(DDSCAPS2));
	Caps.dwCaps2 = DDSCAPS2_CUBEMAP_NEGATIVEY;
	TEST (DirectDraw->GetAvailableVidMem(&Caps, &Total, &Free) == DD_OK);

	memset(&Caps,0,sizeof(DDSCAPS2));
	Caps.dwCaps2 = DDSCAPS2_CUBEMAP_POSITIVEZ;
	TEST (DirectDraw->GetAvailableVidMem(&Caps, &Total, &Free) == DD_OK);

	memset(&Caps,0,sizeof(DDSCAPS2));
	Caps.dwCaps2 = DDSCAPS2_CUBEMAP_NEGATIVEZ;
	TEST (DirectDraw->GetAvailableVidMem(&Caps, &Total, &Free) == DD_OK);

	memset(&Caps,0,sizeof(DDSCAPS2));
	Caps.dwCaps2 = DDSCAPS2_MIPMAPSUBLEVEL;
	TEST (DirectDraw->GetAvailableVidMem(&Caps, &Total, &Free) == DD_OK);

	memset(&Caps,0,sizeof(DDSCAPS2));
	Caps.dwCaps2 = DDSCAPS2_D3DTEXTUREMANAGE;
	TEST (DirectDraw->GetAvailableVidMem(&Caps, &Total, &Free) == DD_OK);

	memset(&Caps,0,sizeof(DDSCAPS2));
	Caps.dwCaps2 = DDSCAPS2_DONOTPERSIST;
	TEST (DirectDraw->GetAvailableVidMem(&Caps, &Total, &Free) == DD_OK);

	memset(&Caps,0,sizeof(DDSCAPS2));
	Caps.dwCaps2 = DDSCAPS2_STEREOSURFACELEFT;
	TEST (DirectDraw->GetAvailableVidMem(&Caps, &Total, &Free) == DD_OK);

	memset(&Caps,0,sizeof(DDSCAPS2));
	Caps.dwCaps2 = DDSCAPS2_VOLUME;
	TEST (DirectDraw->GetAvailableVidMem(&Caps, &Total, &Free) == DD_OK);

	memset(&Caps,0,sizeof(DDSCAPS2));
	Caps.dwCaps2 = DDSCAPS2_NOTUSERLOCKABLE;
	TEST (DirectDraw->GetAvailableVidMem(&Caps, &Total, &Free) == DD_OK);

	memset(&Caps,0,sizeof(DDSCAPS2));
	Caps.dwCaps2 = DDSCAPS2_POINTS;
	TEST (DirectDraw->GetAvailableVidMem(&Caps, &Total, &Free) == DD_OK);

	memset(&Caps,0,sizeof(DDSCAPS2));
	Caps.dwCaps2 =  DDSCAPS2_RTPATCHES;
	TEST (DirectDraw->GetAvailableVidMem(&Caps, &Total, &Free) == DD_OK);

	memset(&Caps,0,sizeof(DDSCAPS2));
	Caps.dwCaps2 = DDSCAPS2_NPATCHES;
	TEST (DirectDraw->GetAvailableVidMem(&Caps, &Total, &Free) == DD_OK);

	memset(&Caps,0,sizeof(DDSCAPS2));
	Caps.dwCaps2 = DDSCAPS2_RESERVED3;
	TEST (DirectDraw->GetAvailableVidMem(&Caps, &Total, &Free) == DD_OK);

	memset(&Caps,0,sizeof(DDSCAPS2));
	Caps.dwCaps2 = DDSCAPS2_DISCARDBACKBUFFER;
	TEST (DirectDraw->GetAvailableVidMem(&Caps, &Total, &Free) == DD_OK);

	memset(&Caps,0,sizeof(DDSCAPS2));
	Caps.dwCaps2 = DDSCAPS2_ENABLEALPHACHANNEL;
	TEST (DirectDraw->GetAvailableVidMem(&Caps, &Total, &Free) == DD_OK);

	memset(&Caps,0,sizeof(DDSCAPS2));
	Caps.dwCaps2 = DDSCAPS2_EXTENDEDFORMATPRIMARY;
	TEST (DirectDraw->GetAvailableVidMem(&Caps, &Total, &Free) == DD_OK);

	memset(&Caps,0,sizeof(DDSCAPS2));
	Caps.dwCaps2 = DDSCAPS2_ADDITIONALPRIMARY;
	TEST (DirectDraw->GetAvailableVidMem(&Caps, &Total, &Free) == DD_OK);

	memset(&Caps,0,sizeof(DDSCAPS2));
	Caps.dwCaps3 = ~(DDSCAPS3_MULTISAMPLE_QUALITY_MASK | DDSCAPS3_MULTISAMPLE_MASK | DDSCAPS3_RESERVED1 | DDSCAPS3_RESERVED2 | DDSCAPS3_LIGHTWEIGHTMIPMAP | DDSCAPS3_AUTOGENMIPMAP | DDSCAPS3_DMAP);
	TEST (DirectDraw->GetAvailableVidMem(&Caps, &Total, &Free) == DDERR_INVALIDCAPS);
	
	memset(&Caps,0,sizeof(DDSCAPS2));
	Caps.dwCaps3 = (DDSCAPS3_MULTISAMPLE_QUALITY_MASK | DDSCAPS3_MULTISAMPLE_MASK | DDSCAPS3_RESERVED1 | DDSCAPS3_RESERVED2 | DDSCAPS3_LIGHTWEIGHTMIPMAP | DDSCAPS3_AUTOGENMIPMAP | DDSCAPS3_DMAP);
	TEST (DirectDraw->GetAvailableVidMem(&Caps, &Total, &Free) == DD_OK);

	memset(&Caps,0,sizeof(DDSCAPS2));
	Caps.dwCaps4 = 1;
	TEST (DirectDraw->GetAvailableVidMem(&Caps, &Total, &Free) == DDERR_INVALIDCAPS );

	DirectDraw->Release();

	return TRUE;
}

BOOL Test_GetFourCCCodes (INT* passed, INT* failed)
{
	LPDIRECTDRAW7 DirectDraw;

	/* Preparations */
	if (DirectDrawCreateEx(NULL, (VOID**)&DirectDraw, IID_IDirectDraw7, NULL) != DD_OK)
	{
		printf("ERROR: Failed to set up ddraw\n");
		return FALSE;
	}

	/* Here we go */
	DWORD dwNumCodes, *lpCodes;
	TEST (DirectDraw->GetFourCCCodes(NULL, (PDWORD)0xdeadbeef) == DDERR_INVALIDPARAMS);

	TEST (DirectDraw->GetFourCCCodes(&dwNumCodes, NULL) == DD_OK && dwNumCodes);
	lpCodes = (PDWORD)HeapAlloc(GetProcessHeap(), 0, sizeof(DWORD)*dwNumCodes);
	*lpCodes = 0;
	TEST ( DirectDraw->GetFourCCCodes(NULL, lpCodes) == DDERR_INVALIDPARAMS );
	TEST (DirectDraw->GetFourCCCodes(&dwNumCodes, lpCodes) == DD_OK && *lpCodes );

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
