#include "priv.h"
#include <wintrust.h>
#include "wvtp.h"

#define WINTRUST TEXT("wintrust.dll")

#ifdef DELAY_LOAD_WVT

#ifndef _WVTP_NOCODE_
Cwvt::Cwvt()
{
    m_fInited = FALSE;
}

Cwvt::~Cwvt()
{
    if (m_fInited) {
        FreeLibrary(m_hMod);
    }
}

HRESULT 
Cwvt::Init(void)
{

    if (m_fInited) {
        return S_OK;
    }

    m_hMod = LoadLibrary( WINTRUST );

    if (NULL == m_hMod) {
        return (HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND));
    }


#define CHECKAPI(_fn) \
    *(FARPROC*)&(_pfn##_fn) = GetProcAddress(m_hMod, #_fn); \
    if (!(_pfn##_fn)) { \
        FreeLibrary(m_hMod); \
        return (HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND)); \
    }

    CHECKAPI(WinVerifyTrust);

    m_fInited = TRUE;
    return S_OK;
}


#endif // _WVTP_NOCODE_
#endif // DELAY_LOAD_WVT

#define REGSTR_PATH_INFODEL_REST     TEXT("Software\\Policies\\Microsoft\\Internet Explorer\\Infodelivery\\Restrictions")
#define REGVAL_UI_REST        TEXT("NoWinVerifyTrustUI")

BOOL
IsUIRestricted()
{

    HKEY hkeyRest = 0;
    BOOL bUIRest = FALSE;
    DWORD dwValue = 0;
    DWORD dwLen = sizeof(DWORD);

    // per-machine UI off policy
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGSTR_PATH_INFODEL_REST, 0, KEY_READ, &hkeyRest) == ERROR_SUCCESS) {

        if (RegQueryValueEx( hkeyRest, REGVAL_UI_REST, NULL, NULL,
                      (LPBYTE)&dwValue, &dwLen) == ERROR_SUCCESS && dwValue)
            bUIRest = TRUE;

        RegCloseKey(hkeyRest);
    }

    return bUIRest;
}

// BUGBUG: move these to corpolicy.h in iedev\inc!!!
// {D41E4F1F-A407-11d1-8BC9-00C04FA30A41}
#define COR_POLICY_LOCKDOWN_CHECK \
{ 0xd41e4f1f, 0xa407, 0x11d1, {0x8b, 0xc9, 0x0, 0xc0, 0x4f, 0xa3, 0xa, 0x41 } }

//--------------------------------------------------------------------
// For COR_POLICY_LOCKDOWN_CHECK:
// -----------------------------

// Structure to pass into WVT
typedef struct _COR_LOCKDOWN {
    DWORD                 cbSize;          // Size of policy provider
    DWORD                 flag;            // reserved
    BOOL                  fAllPublishers;  // Trust all publishers or just ones in the trusted data base
} COR_LOCKDOWN, *PCOR_LOCKDOWN;


HRESULT Cwvt::VerifyTrust(HANDLE hFile, HWND hWnd, LPCWSTR szStatusText) 
{
    WINTRUST_DATA           sWTD;
    WINTRUST_FILE_INFO      sWTFI;

    GUID gV2 = COR_POLICY_LOCKDOWN_CHECK;
    COR_LOCKDOWN  sCorPolicy;

    HRESULT hr = S_OK;

    memset(&sCorPolicy, 0, sizeof(COR_LOCKDOWN));
    
    sCorPolicy.cbSize = sizeof(COR_LOCKDOWN);

    if ( (hWnd == INVALID_HANDLE_VALUE) || IsUIRestricted())
        sCorPolicy.fAllPublishers = FALSE; // lockdown to only trusted pubs
    else
        sCorPolicy.fAllPublishers = TRUE; // regular behavior
    
    // Set up the winverify provider structures
    memset(&sWTD, 0x00, sizeof(WINTRUST_DATA));
    memset(&sWTFI, 0x00, sizeof(WINTRUST_FILE_INFO));
        
    sWTFI.cbStruct      = sizeof(WINTRUST_FILE_INFO);
    sWTFI.hFile         = hFile;
    sWTFI.pcwszFilePath = szStatusText;

    sWTD.cbStruct       = sizeof(WINTRUST_DATA);
    sWTD.pPolicyCallbackData = &sCorPolicy; // Add in the cor trust information!!
    sWTD.dwUIChoice     = WTD_UI_ALL;        // No bad UI is overridden in COR TRUST provider

    sWTD.dwUnionChoice  = WTD_CHOICE_FILE;
    sWTD.pFile          = &sWTFI;
        
    hr = WinVerifyTrust(hWnd, &gV2, &sWTD);
        
    // BUGBUG: this works around a wvt bug that returns 0x57 (success) when
    // you hit No to an usigned control
    if (SUCCEEDED(hr) && hr != S_OK) {
        hr = TRUST_E_FAIL;
    }

    return hr;
}
