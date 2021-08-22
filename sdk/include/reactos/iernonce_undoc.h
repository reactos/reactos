#ifndef _IERNONCE_UNDOC_H_
#define _IERNONCE_UNDOC_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef VOID(CALLBACK *RUNONCEEX_CALLBACK) (UINT CompleteCnt, UINT TotalCnt, DWORD dwReserved);

VOID WINAPI RunOnceExProcess (_In_ HWND hwnd,
                              _In_ HINSTANCE hInst,
                              _In_ LPCSTR pszCmdLine,
                              _In_ int nCmdShow);
VOID WINAPI InitCallback (_In_ RUNONCEEX_CALLBACK Callback,
                          _In_ BOOL bSilence);

#ifdef __cplusplus
}
#endif

#endif /* _IERNONCE_UNDOC_H_ */
