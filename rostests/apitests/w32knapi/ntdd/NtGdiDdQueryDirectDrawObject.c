#include "../w32knapi.h"

W32KAPI
BOOL STDCALL 
NtGdiDdQueryDirectDrawObject(
    HANDLE hDirectDrawLocal,
    DD_HALINFO  *pHalInfo,
    DWORD *pCallBackFlags,
    LPD3DNTHAL_CALLBACKS puD3dCallbacks,
    LPD3DNTHAL_GLOBALDRIVERDATA puD3dDriverData,
    PDD_D3DBUFCALLBACKS puD3dBufferCallbacks,
    LPDDSURFACEDESC puD3dTextureFormats,
    DWORD *puNumHeaps,
    VIDEOMEMORY *puvmList,
    DWORD *puNumFourCC,
    DWORD *puFourCC
)
{
	return (BOOL)Syscall(L"NtGdiDdQueryDirectDrawObject", 11, &hDirectDrawLocal);
}

INT
Test_NtGdiDdQueryDirectDrawObject(PTESTINFO pti)
{
	HANDLE  hDirectDraw = NULL;
	DD_HALINFO *pHalInfo = NULL;
	DWORD *pCallBackFlags = NULL;
	LPD3DNTHAL_CALLBACKS puD3dCallbacks = NULL;
	LPD3DNTHAL_GLOBALDRIVERDATA puD3dDriverData = NULL;
	PDD_D3DBUFCALLBACKS puD3dBufferCallbacks = NULL;
	LPDDSURFACEDESC puD3dTextureFormats = NULL;
	DWORD *puNumHeaps = NULL;
	VIDEOMEMORY *puvmList = NULL;
	DWORD *puNumFourCC = NULL;
	DWORD *puFourCC = NULL;

	DD_HALINFO HalInfo;
	DWORD CallBackFlags[4];
	D3DNTHAL_CALLBACKS D3dCallbacks;
	D3DNTHAL_GLOBALDRIVERDATA D3dDriverData;
	DD_D3DBUFCALLBACKS D3dBufferCallbacks;
	DDSURFACEDESC2 D3dTextureFormats[100];
	//DWORD NumHeaps = 0;
	VIDEOMEMORY vmList;
	//DWORD NumFourCC = 0;
	//DWORD FourCC = 0;

	/* clear data */
	memset(&vmList,0,sizeof(VIDEOMEMORY));
	memset(&D3dTextureFormats,0,sizeof(DDSURFACEDESC));
	memset(&D3dBufferCallbacks,0,sizeof(DD_D3DBUFCALLBACKS));
	memset(&D3dDriverData,0,sizeof(D3DNTHAL_GLOBALDRIVERDATA));
	memset(&D3dCallbacks,0,sizeof(D3DNTHAL_CALLBACKS));
	memset(&HalInfo,0,sizeof(DD_HALINFO));
	memset(CallBackFlags,0,sizeof(DWORD)*3);

	HDC hdc = CreateDCW(L"DISPLAY",NULL,NULL,NULL);
	ASSERT1(hdc != NULL);

	hDirectDraw = (HANDLE) Syscall(L"NtGdiDdCreateDirectDrawObject", 1, &hdc);
	ASSERT1(hDirectDraw != NULL);

	/* Test ReactX */
	hDirectDraw = (HANDLE) Syscall(L"NtGdiDdCreateDirectDrawObject", 1, &hdc);

	RTEST(NtGdiDdQueryDirectDrawObject( NULL, pHalInfo, 
										pCallBackFlags, puD3dCallbacks, 
										puD3dDriverData, puD3dBufferCallbacks, 
										puD3dTextureFormats, puNumHeaps, 
										puvmList, puNumFourCC,
										puFourCC) == FALSE);

	RTEST(pHalInfo == NULL);
	RTEST(pCallBackFlags == NULL);
	RTEST(puD3dCallbacks == NULL);
	RTEST(puD3dDriverData == NULL);
	RTEST(puD3dBufferCallbacks == NULL);
	RTEST(puD3dTextureFormats == NULL);
	RTEST(puNumFourCC == NULL);
	RTEST(puFourCC == NULL);

	RTEST(NtGdiDdQueryDirectDrawObject( hDirectDraw, pHalInfo, 
										pCallBackFlags, puD3dCallbacks, 
										puD3dDriverData, puD3dBufferCallbacks, 
										puD3dTextureFormats, puNumHeaps, 
										puvmList, puNumFourCC,
										puFourCC) == FALSE);

	RTEST(pHalInfo == NULL);
	RTEST(pCallBackFlags == NULL);
	RTEST(puD3dCallbacks == NULL);
	RTEST(puD3dDriverData == NULL);
	RTEST(puD3dBufferCallbacks == NULL);
	RTEST(puD3dTextureFormats == NULL);
	RTEST(puNumFourCC == NULL);
	RTEST(puFourCC == NULL);

	pHalInfo = &HalInfo;
	RTEST(NtGdiDdQueryDirectDrawObject( hDirectDraw, pHalInfo, 
										pCallBackFlags, puD3dCallbacks, 
										puD3dDriverData, puD3dBufferCallbacks, 
										puD3dTextureFormats, puNumHeaps, 
										puvmList, puNumFourCC,
										puFourCC)== FALSE);
	RTEST(pHalInfo != NULL);
	RTEST(pCallBackFlags == NULL);
	RTEST(puD3dCallbacks == NULL);
	RTEST(puD3dDriverData == NULL);
	RTEST(puD3dBufferCallbacks == NULL);
	RTEST(puD3dTextureFormats == NULL);
	RTEST(puNumFourCC == NULL);
	RTEST(puFourCC == NULL);
	ASSERT1(pHalInfo != NULL);

	/* Cleanup ReactX setup */
	DeleteDC(hdc);
	Syscall(L"NtGdiDdDeleteDirectDrawObject", 1, &hDirectDraw);

	return APISTATUS_NORMAL;
}
