#ifdef __cplusplus

#include "capi.h"

// WinVerifyTrust delay load modelled on shell's urlmonp.h

#define DELAY_LOAD_WVT

class CDownload;

class Cwvt
{
    public:
#ifdef DELAY_LOAD_WVT
#define DELAYWVTAPI(_fn, _args, _nargs) \
    HRESULT _fn _args { \
        HRESULT hres = Init(); \
        if (SUCCEEDED(hres)) { \
            hres = _pfn##_fn _nargs; \
        } \
        return hres;    } \
    HRESULT (STDAPICALLTYPE* _pfn##_fn) _args;

    HRESULT     Init(void);
    Cwvt();
    ~Cwvt();

    BOOL    m_fInited;
    HMODULE m_hMod;
#else
#define DELAYWVTAPI(_fn, _args, _nargs) \
    HRESULT _fn _args { \
            HRESULT hr = ::#_fn _nargs; \
            }

#endif

    private:
    DELAYWVTAPI(WinVerifyTrust,
    (HWND hwnd, GUID * ActionID, LPVOID ActionData),
    (hwnd, ActionID, ActionData));

    public:
    HRESULT VerifyTrust(HANDLE hFile, HWND hWnd, PJAVA_TRUST *ppJavaTrust,
                        LPCWSTR szStatusText, 
                        IInternetHostSecurityManager *pHostSecurityManager,
                        LPSTR szFilePath, LPSTR szCatalogFile,
                        CDownload *pdl);

    private:
    BOOL                     m_bHaveWTData;
    WINTRUST_CATALOG_INFO    m_wtCatalogInfo;
};
#endif
