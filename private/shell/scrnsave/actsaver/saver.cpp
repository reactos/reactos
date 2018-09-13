/////////////////////////////////////////////////////////////////////////////
// SAVER.CPP
//
// Implementation of CActiveScreenSaver
//
// History:
//
// Author   Date        Description
// ------   ----        -----------
// jaym     08/26/96    Created
// jaym     05/04/97    Removed Pause functionality
// jaym     06/05/97    Added Mode (changed PreviewMode)
/////////////////////////////////////////////////////////////////////////////
#include "precomp.h"
#include "regini.h"
#include "preview.h"
#include "sswnd.h"
#include "saver.h"

/////////////////////////////////////////////////////////////////////////////
// External variables
/////////////////////////////////////////////////////////////////////////////
extern TCHAR g_szRegSubKey[];   // From ACTSAVER.CPP

/////////////////////////////////////////////////////////////////////////////
// Module variables
/////////////////////////////////////////////////////////////////////////////
#pragma data_seg(DATASEG_READONLY)
static const TCHAR s_szFeatureFlags[]   = TEXT("Features");
static const TCHAR s_szNavOnClick[]     = TEXT("Navigate On Click");
static const TCHAR s_szChannelTime[]    = TEXT("Display Time");
static const TCHAR s_szRestartTime[]    = TEXT("Restart Time");
static const TCHAR s_szPlaySounds[]     = TEXT("Play Sounds");
    // Registry load/save settings
#pragma data_seg()

/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////
static const IID * g_argiidInterfaces[] = 
{
    &IID_IScreenSaver,
    &IID_IScreenSaverConfig,
};

/////////////////////////////////////////////////////////////////////////////
// CActiveScreenSaver
/////////////////////////////////////////////////////////////////////////////
IMPLEMENT_SINGLE_NOT_AGGREGATABLE(CActiveScreenSaver)

CActiveScreenSaver::CActiveScreenSaver
(
)
{
    m_hImgList = NULL;
    m_pfnOldGeneralPSDlgProc = NULL;
        // Valid only if in IScreenSaverConfig::ShowDialog()

    m_lMode = SSMODE_NORMAL;

    m_bRunning = FALSE;
    m_bstrCurrentURL = NULL;

    m_pPreviewWnd = NULL;
    m_pSaverWnd = NULL;

    m_pSSInfo = NULL;
}

CActiveScreenSaver::~CActiveScreenSaver
(
)
{
    ASSERT(!m_bRunning);

    if (m_pSSInfo != NULL)
        delete m_pSSInfo;

    if (m_pSaverWnd != NULL)
        delete m_pSaverWnd;

    if (m_pPreviewWnd != NULL)
        delete m_pPreviewWnd;

    if (m_bstrCurrentURL != NULL)
        SysFreeString(m_bstrCurrentURL);

    if (m_hImgList != NULL)
         EVAL(ImageList_Destroy(m_hImgList));

    ASSERT(m_pfnOldGeneralPSDlgProc == NULL);

    CLEANUP_SINGLE_NOT_AGGRAGATABLE(CActiveScreenSaver);
}

/////////////////////////////////////////////////////////////////////////////
// CActiveScreenSaver::ReadSSInfo
/////////////////////////////////////////////////////////////////////////////
HRESULT CActiveScreenSaver::ReadSSInfo
(
)
{

    // If this is the first time, create the struct.
    if (m_pSSInfo == NULL)
        m_pSSInfo = new SScreenSaverInfo;

    if (m_pSSInfo == NULL)
        return E_OUTOFMEMORY;

    // Read the feature flags.
    m_pSSInfo->dwFeatureFlags = ReadRegDWORD(   HKEY_CURRENT_USER,
                                                g_szRegSubKey,
                                                s_szFeatureFlags,
                                                DEFAULT_FEATURE_FLAGS);

    // Navigate on Click
    m_pSSInfo->bNavigateOnClick = (ReadRegDWORD(HKEY_CURRENT_USER,
                                                g_szRegSubKey,
                                                s_szNavOnClick,
                                                DEFAULT_NAVIGATE_ON_CLICK)
                                        ? VARIANT_TRUE
                                        : VARIANT_FALSE);

    // Channel Time
    m_pSSInfo->dwChannelTime = ReadRegDWORD(HKEY_CURRENT_USER,
                                            g_szRegSubKey,
                                            s_szChannelTime,
                                            DEFAULT_CHANNEL_TIME);

    // Restart Time
    m_pSSInfo->dwRestartTime = ReadRegDWORD(HKEY_CURRENT_USER,
                                            g_szRegSubKey,
                                            s_szRestartTime,
                                            DEFAULT_RESTART_TIME_MINUTES);
    // Convert minutes to ms
    m_pSSInfo->dwRestartTime *= (60*1000);

    // Play sounds - 0 Mute 1 = No mute
    m_pSSInfo->bPlaySounds = (ReadRegDWORD( HKEY_CURRENT_USER,
                                            g_szRegSubKey,
                                            s_szPlaySounds,
                                            DEFAULT_PLAY_SOUNDS)
                                    ? VARIANT_TRUE
                                    : VARIANT_FALSE);

    return S_OK;

}

/////////////////////////////////////////////////////////////////////////////
// CActiveScreenSaver::WriteSSInfo
/////////////////////////////////////////////////////////////////////////////
void CActiveScreenSaver::WriteSSInfo
(
)
{
    DWORD dwVal;

    // Navigate on Click
    dwVal = ((m_pSSInfo->bNavigateOnClick == VARIANT_TRUE) ? TRUE : FALSE);
    WriteRegValue(  HKEY_CURRENT_USER,
                    g_szRegSubKey,
                    s_szNavOnClick,
                    REG_DWORD,
                    (BYTE *)&dwVal,
                    SIZEOF(DWORD));

    // Channel Time
    WriteRegValue(  HKEY_CURRENT_USER,
                    g_szRegSubKey,
                    s_szChannelTime,
                    REG_DWORD,
                    (BYTE *)&m_pSSInfo->dwChannelTime,
                    SIZEOF(DWORD));

    // Play sounds - 0 Mute 1 = No mute
    dwVal = ((m_pSSInfo->bPlaySounds == VARIANT_TRUE) ? TRUE : FALSE);
    WriteRegValue(  HKEY_CURRENT_USER,
                    g_szRegSubKey,
                    s_szPlaySounds,
                    REG_DWORD,
                    (BYTE *)&dwVal,
                    SIZEOF(DWORD));
}

/////////////////////////////////////////////////////////////////////////////
// IScreenSaver
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// CActiveScreenSaver::get_Mode
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CActiveScreenSaver::get_Mode
(
    long * plMode
)
{
    *plMode = m_lMode;
    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// CActiveScreenSaver::put_Mode
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CActiveScreenSaver::put_Mode
(
    long lMode
)
{
    m_lMode = lMode;
    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// CActiveScreenSaver::get_Running
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CActiveScreenSaver::get_Running
(
    VARIANT_BOOL * pbRunning
)
{
    *pbRunning = m_bRunning;
    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// CActiveScreenSaver::put_Running
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CActiveScreenSaver::put_Running
(
    VARIANT_BOOL bRunning
)
{
    m_bRunning = bRunning;
    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// CActiveScreenSaver::get_Config
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CActiveScreenSaver::get_Config
(
    IScreenSaverConfig ** ppSSConfig
)
{
    return _InternalQueryInterface(IID_IScreenSaverConfig, (void **)ppSSConfig);
}

/////////////////////////////////////////////////////////////////////////////
// CActiveScreenSaver::get_CurrentURL
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CActiveScreenSaver::get_CurrentURL
(
    BSTR * pbstrCurrentURL
)
{
    if (m_bstrCurrentURL != NULL)
    {
        *pbstrCurrentURL = SysAllocString(m_bstrCurrentURL);

        if (NULL == *pbstrCurrentURL)
            return E_OUTOFMEMORY;
    }
    else
        *pbstrCurrentURL = NULL;

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// CActiveScreenSaver::put_CurrentURL
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CActiveScreenSaver::put_CurrentURL
(
    BSTR bstrCurrentURL
)
{

    if (m_bstrCurrentURL != NULL)
    {
        SysFreeString(m_bstrCurrentURL);
        m_bstrCurrentURL = NULL;
    }

    if (bstrCurrentURL != NULL)
    {
        m_bstrCurrentURL = SysAllocString(bstrCurrentURL);

        if (NULL == m_bstrCurrentURL)
            return E_OUTOFMEMORY;
    }

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// CActiveScreenSaver::Run
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CActiveScreenSaver::Run
(
    HWND hwndParent
)
{
    RECT rectWindow;
    HRESULT hr;

    if (m_bRunning)
        return S_OK;

    if (m_lMode == SSMODE_PREVIEW)
    {
        if (hwndParent != NULL)
            GetClientRect(hwndParent, &rectWindow);
        else
            GetWindowRect(GetDesktopWindow(), &rectWindow);

        m_pPreviewWnd = new CPreviewWindow;

        if (m_pPreviewWnd == NULL)
            return E_OUTOFMEMORY;

        m_bRunning = (VARIANT_BOOL)m_pPreviewWnd->Create(rectWindow, hwndParent);
    }
    else
    {
        // Read the screen saver information from the registry.
        hr = ReadSSInfo();
        if (FAILED(hr))
            return E_OUTOFMEMORY;

        // Create and display the screen saver window.
        m_pSaverWnd = new CScreenSaverWindow;

        if (NULL == m_pSaverWnd)
            return E_OUTOFMEMORY;

        // Make sure we support multiple monitors.
        HDC hDC = GetDC(NULL);

        if (NULL == hDC)
            return E_OUTOFMEMORY;

        GetClipBox(hDC, &rectWindow);
        ReleaseDC(NULL, hDC);

        InternalAddRef();
        m_bRunning = (VARIANT_BOOL)m_pSaverWnd->Create(rectWindow, NULL, this);
        InternalRelease();

    }

    // Don't return from this function until the window closes.
    MSG msg;
    while (m_bRunning && GetMessage(&msg, 0, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// ISupportsErrorInfo
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// CActiveScreenSaver::InterfaceSupportsErrorInfo
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CActiveScreenSaver::InterfaceSupportsErrorInfo
(
    REFIID riid
)
{
    for (int i = 0; i < ARRAYSIZE(g_argiidInterfaces); i++)
    {
        if (InlineIsEqualGUID(*g_argiidInterfaces[i], riid))
            return S_OK;
    }

    return S_FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// IObjectSafety interface
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// CActiveScreenSaver::GetInterfaceSafetyOptions
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CActiveScreenSaver::GetInterfaceSafetyOptions
(
    REFIID  riid,
    DWORD * pdwSupportedOptions,
    DWORD * pdwEnabledOptions
)
{
    HRESULT hrResult = S_OK;

    for (;;)
    {
        if (InlineIsEqualGUID(riid, IID_IDispatch))
            break;

        for (int i = 0; i < ARRAYSIZE(g_argiidInterfaces); i++)
        {
            if (InlineIsEqualGUID(*g_argiidInterfaces[i], riid))
            {
                *pdwSupportedOptions = INTERFACESAFE_FOR_UNTRUSTED_CALLER | INTERFACESAFE_FOR_UNTRUSTED_DATA;
                *pdwEnabledOptions = INTERFACESAFE_FOR_UNTRUSTED_CALLER | INTERFACESAFE_FOR_UNTRUSTED_DATA;

                return S_OK;
            }
        }

        hrResult = E_NOINTERFACE;
        break;
    }

    return hrResult;
}

/////////////////////////////////////////////////////////////////////////////
// CActiveScreenSaver::SetInterfaceSafetyOptions
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CActiveScreenSaver::SetInterfaceSafetyOptions
(
    REFIID  riid,
    DWORD   dwOptionSetMask,
    DWORD   dwEnabledOptions
)
{
    HRESULT hrResult = S_OK;

    for (;;)
    {
        if (InlineIsEqualGUID(riid, IID_IDispatch))
            break;

        for (int i = 0; i < ARRAYSIZE(g_argiidInterfaces); i++)
        {
            if (InlineIsEqualGUID(*g_argiidInterfaces[i], riid))
                return S_OK;
        }

        hrResult = E_NOINTERFACE;
        break;
    }

    return hrResult;
}

/////////////////////////////////////////////////////////////////////////////
// Helper functions
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// myatoi
/////////////////////////////////////////////////////////////////////////////
int myatoi
(
    LPSTR pch
)
{
    int i = 0;

    // REVIEW: Handle negative numbers? [jaym]

    while (*pch >= '0' && *pch <= '9')
        i = i*10 + *pch++ - '0';

    return i;
}
