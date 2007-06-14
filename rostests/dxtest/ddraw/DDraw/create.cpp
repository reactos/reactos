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

	TEST ( DirectDraw->SetCooperativeLevel (hwnd, DDSCL_NORMAL) == DD_OK );
	TEST ( DirectDraw->Compact() == DDERR_NOEXCLUSIVEMODE );


	TEST ( DirectDraw->SetCooperativeLevel (NULL, DDSCL_FULLSCREEN | DDSCL_EXCLUSIVE) == DDERR_INVALIDPARAMS );
	TEST ( DirectDraw->SetCooperativeLevel (hwnd, DDSCL_FULLSCREEN) == DDERR_INVALIDPARAMS);
	TEST ( DirectDraw->SetCooperativeLevel (hwnd, DDSCL_NORMAL | DDSCL_ALLOWMODEX) == DDERR_INVALIDPARAMS );
	TEST ( DirectDraw->SetCooperativeLevel ((HWND)0xdeadbeef, DDSCL_NORMAL) == DDERR_INVALIDPARAMS);

	TEST ( DirectDraw->SetCooperativeLevel (hwnd, DDSCL_FULLSCREEN | DDSCL_EXCLUSIVE) == DD_OK);
	TEST ( DirectDraw->Compact() == DD_OK );
	TEST ( DirectDraw->SetCooperativeLevel (hwnd, DDSCL_FULLSCREEN | DDSCL_EXCLUSIVE | DDSCL_ALLOWMODEX) == DD_OK);
    TEST ( DirectDraw->Compact() == DD_OK );
	TEST ( DirectDraw->SetCooperativeLevel (NULL, DDSCL_NORMAL) == DD_OK );
    TEST ( DirectDraw->Compact() == DDERR_NOEXCLUSIVEMODE );
	TEST ( DirectDraw->SetCooperativeLevel (hwnd, DDSCL_NORMAL) == DD_OK );
	TEST ( DirectDraw->Compact() == DDERR_NOEXCLUSIVEMODE );

	TEST ( DirectDraw->TestCooperativeLevel() == DD_OK ); // I do not get what this API does it always seems to return DD_OK

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

BOOL Test_GetDeviceIdentifier (INT* passed, INT* failed)
{
	LPDIRECTDRAW7 DirectDraw;
	DDDEVICEIDENTIFIER2 pDDDI;
	//OLECHAR GuidStr[100];

	/* Preparations */
	if (DirectDrawCreateEx(NULL, (VOID**)&DirectDraw, IID_IDirectDraw7, NULL) != DD_OK)
	{
		printf("ERROR: Failed to set up ddraw\n");
		return FALSE;
	}

	TEST ( DirectDraw->GetDeviceIdentifier(NULL, 0) == DDERR_INVALIDPARAMS );
	TEST ( DirectDraw->GetDeviceIdentifier(NULL, ~DDGDI_GETHOSTIDENTIFIER) == DDERR_INVALIDPARAMS );
	TEST ( DirectDraw->GetDeviceIdentifier(NULL, DDGDI_GETHOSTIDENTIFIER) == DDERR_INVALIDPARAMS );

	memset(&pDDDI,0,sizeof(DDDEVICEIDENTIFIER2));
	TEST ( DirectDraw->GetDeviceIdentifier(&pDDDI, 0) == DD_OK );

/*
    StringFromGUID2(pDDDI.guidDeviceIdentifier, GuidStr, 100);
    printf("1. \n");
    printf("szDriver : %s\n",pDDDI.szDriver);
    printf("szDescription : %s\n",pDDDI.szDescription);
    printf("liDriverVersion : 0x%08x . 0x%08x\n", pDDDI.liDriverVersion.HighPart, pDDDI.liDriverVersion.LowPart);
    printf("dwVendorId : 0x%08x\n",pDDDI.dwVendorId);
    printf("dwDeviceId : 0x%08x\n",pDDDI.dwDeviceId);
    printf("dwSubSysId : 0x%08x\n",pDDDI.dwSubSysId);
    printf("dwRevision : 0x%08x\n",pDDDI.dwRevision);
    printf("guidDeviceIdentifier : %ls\n",GuidStr);
    printf("dwWHQLLevel : 0x%08x\n",pDDDI.dwWHQLLevel);
*/

	memset(&pDDDI,0,sizeof(DDDEVICEIDENTIFIER2));
	TEST ( DirectDraw->GetDeviceIdentifier(&pDDDI, DDGDI_GETHOSTIDENTIFIER) == DD_OK );
	memset(&pDDDI,0,sizeof(DDDEVICEIDENTIFIER2));
	TEST ( DirectDraw->GetDeviceIdentifier(&pDDDI, ~DDGDI_GETHOSTIDENTIFIER) == DDERR_INVALIDPARAMS );

	DirectDraw->Release();

	return TRUE;
}
