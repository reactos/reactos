
#if !defined(__REACTOS__)

    HANDLE sysNtGdiDdCreateDirectDrawObject(HDC hdc)
    {
       INT retValue;
       win_syscall(hdc,retValue, syscallid_NtGdiDdCreateDirectDrawObject);
       return retValue;
    }

    BOOL sysNtGdiDdDeleteDirectDrawObject(HANDLE hDirectDrawLocal)
    {
       INT retValue;
       win_syscall(hDirectDrawLocal,retValue, syscallid_NtGdiDdDeleteDirectDrawObject);
       return retValue;
    }

    BOOL sysNtGdiDdQueryDirectDrawObject( HANDLE hDirectDrawLocal, DD_HALINFO *pHalInfo, 
                                   DWORD *pCallBackFlags, 
                                   LPD3DNTHAL_CALLBACKS puD3dCallbacks, 
                                   LPD3DNTHAL_GLOBALDRIVERDATA puD3dDriverData, 
                                   PDD_D3DBUFCALLBACKS puD3dBufferCallbacks, 
                                   LPDDSURFACEDESC puD3dTextureFormats, 
                                   DWORD *puNumHeaps, VIDEOMEMORY *puvmList, 
                                   DWORD *puNumFourCC, DWORD *puFourCC)
    {
       INT retValue;
       win_syscall(hDirectDrawLocal,retValue, syscallid_NtGdiDdQueryDirectDrawObject);
       return retValue;
       }

#endif
