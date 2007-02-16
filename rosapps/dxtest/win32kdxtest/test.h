

HANDLE test_NtGdiDdCreateDirectDrawObject();
void test_NtGdiDdDeleteDirectDrawObject(HANDLE hDirectDrawLocal);
void test_NtGdiDdQueryDirectDrawObject( HANDLE hDirectDrawLocal);



HANDLE sysNtGdiDdCreateDirectDrawObject(HDC hdc);
BOOL sysNtGdiDdDeleteDirectDrawObject( HANDLE hDirectDrawLocal);
BOOL sysNtGdiDdQueryDirectDrawObject( HANDLE hDirectDrawLocal, DD_HALINFO *pHalInfo, 
                                   DWORD *pCallBackFlags, 
                                   LPD3DNTHAL_CALLBACKS puD3dCallbacks, 
                                   LPD3DNTHAL_GLOBALDRIVERDATA puD3dDriverData, 
                                   PDD_D3DBUFCALLBACKS puD3dBufferCallbacks, 
                                   LPDDSURFACEDESC puD3dTextureFormats, 
                                   DWORD *puNumHeaps, VIDEOMEMORY *puvmList, 
                                   DWORD *puNumFourCC, DWORD *puFourCC);

HANDLE NtGdiDdCreateDirectDrawObject(HDC hdc);
BOOL NtGdiDdDeleteDirectDrawObject( HANDLE hDirectDrawLocal);

BOOL NtGdiDdQueryDirectDrawObject( HANDLE hDirectDrawLocal, DD_HALINFO *pHalInfo, 
                                   DWORD *pCallBackFlags, 
                                   LPD3DNTHAL_CALLBACKS puD3dCallbacks, 
                                   LPD3DNTHAL_GLOBALDRIVERDATA puD3dDriverData, 
                                   PDD_D3DBUFCALLBACKS puD3dBufferCallbacks, 
                                   LPDDSURFACEDESC puD3dTextureFormats, 
                                   DWORD *puNumHeaps, VIDEOMEMORY *puvmList, 
                                   DWORD *puNumFourCC, DWORD *puFourCC);



#define testing_eq(input,value,counter,text) \
        if (input == value) \
        { \
            counter++; \
            printf("FAIL ret=%s, %d != %d )\n",text,(int)input,(int)value); \
        }


#define testing_noteq(input,value,counter,text) \
        if (input != value) \
        { \
            counter++; \
            printf("FAIL ret=%s, %d == %d )\n",text,(int)input,(int)value); \
        }


#define show_status(counter, text) \
        if (counter == 0) \
        { \
            printf("End testing of %s Status : ok\n\n",text); \
        } \
        else \
        { \
            printf("End testing of %s Status : fail\n\n",text); \
        }


#if !defined(__REACTOS__)

#define win_syscall(inValue,outValue,syscallid) \
                    __asm { mov     eax, syscallid }; \
                    __asm { lea     edx, [inValue] }; \
                    __asm { int     0x2E }; \
                    __asm { mov outValue,eax};



#endif
