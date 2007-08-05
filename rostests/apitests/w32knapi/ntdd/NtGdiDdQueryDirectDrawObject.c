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

    return APISTATUS_NORMAL;
}
