/////////////////////////////////////////////////////////////////////////////
// SAVER.H
//
// Declaration of CActiveScreenSaver
//
// History:
//
// Author   Date        Description
// ------   ----        -----------
// jaym     08/26/96    Created
/////////////////////////////////////////////////////////////////////////////
#ifndef __SAVER_H__
#define __SAVER_H__

#include <objsafe.h>        // For IObjectSafety interface
#include "if\actsaver.h"
#include "preview.h"        // For CPreviewWindow class
#include "sswnd.h"          // For CScreenSaverWindow class

#include "resource.h"

/////////////////////////////////////////////////////////////////////////////
// SScreenSaverInfo
/////////////////////////////////////////////////////////////////////////////
struct SScreenSaverInfo
{
    DWORD           dwFeatureFlags;
    VARIANT_BOOL    bNavigateOnClick;
    DWORD           dwChannelTime;
    DWORD           dwRestartTime;
    VARIANT_BOOL    bPlaySounds;
};
    // Screen saver settings data (contained in the registry)

/////////////////////////////////////////////////////////////////////////////
// Defaults
/////////////////////////////////////////////////////////////////////////////
#define DEFAULT_FEATURE_FLAGS           0
#define DEFAULT_NAVIGATE_ON_CLICK       VARIANT_TRUE
#define DEFAULT_CHANNEL_TIME            30              // seconds
#define MIN_CHANNEL_TIME_SECONDS        10              // seconds
#define MAX_CHANNEL_TIME_SECONDS        300             // seconds
#define DEFAULT_RESTART_TIME_MINUTES    360             // minutes
#define DEFAULT_PLAY_SOUNDS             VARIANT_FALSE

/////////////////////////////////////////////////////////////////////////////
// CActiveScreenSaver
/////////////////////////////////////////////////////////////////////////////
class CActiveScreenSaver : 
    public CComDualImpl<IScreenSaver, &IID_IScreenSaver, &LIBID_ACTSAVERLib>, 
    public CComDualImpl<IScreenSaverConfig, &IID_IScreenSaverConfig, &LIBID_ACTSAVERLib>,
    public ISupportErrorInfo,
    public IObjectSafety,
    public CComObjectRoot,
    public CComCoClass<CActiveScreenSaver, &CLSID_CActiveScreenSaver>
{
// Construction/Destruction
public:
    CActiveScreenSaver();
    virtual ~CActiveScreenSaver();

    DECLARE_SINGLE_NOT_AGGREGATABLE(CActiveScreenSaver) 
    DECLARE_STATIC_REGISTRY_RESOURCEID(IDR_SCREENSAVER)

// Data
public:
    HIMAGELIST  m_hImgList;
    DLGPROC     m_pfnOldGeneralPSDlgProc;
        // Valid only when in IScreenSaverConfig::ShowDialog()

protected:
    long                    m_lMode;
    VARIANT_BOOL            m_bRunning;
    BSTR                    m_bstrCurrentURL;

    CPreviewWindow *        m_pPreviewWnd;
    CScreenSaverWindow *    m_pSaverWnd;

    SScreenSaverInfo *      m_pSSInfo;

private:
    HWND CreatePreviewWindow(HWND hwndParent);
    HRESULT ReadSSInfo();
    void WriteSSInfo();

// Interfaces
public:
    BEGIN_COM_MAP(CActiveScreenSaver)
        COM_INTERFACE_ENTRY2(IDispatch, IScreenSaver)
        COM_INTERFACE_ENTRY(IScreenSaver)
        COM_INTERFACE_ENTRY(IScreenSaverConfig)
        COM_INTERFACE_ENTRY(IProvideClassInfo2)
        COM_INTERFACE_ENTRY(IProvideClassInfo)
        COM_INTERFACE_ENTRY(ISupportErrorInfo)
        COM_INTERFACE_ENTRY(IObjectSafety)
    END_COM_MAP()

    // IScreenSaver
    STDMETHOD(get_Mode)         (long * plMode);
    STDMETHOD(put_Mode)         (long lMode);
    STDMETHOD(get_Running)      (VARIANT_BOOL * pbRunning);
    STDMETHOD(put_Running)      (VARIANT_BOOL bRunning);
    STDMETHOD(get_Config)       (IScreenSaverConfig ** ppSSConfig);
    STDMETHOD(get_CurrentURL)   (BSTR * pbstrCurrentURL);
    STDMETHOD(put_CurrentURL)   (BSTR bstrCurrentURL);
    STDMETHOD(Run)              (HWND hwndParent);

    // IScreenSaverConfig
    STDMETHOD(get_Features)         (DWORD * pdwFeatureFlags);
    STDMETHOD(put_Features)         (DWORD dwFeatureFlags);
    STDMETHOD(get_NavigateOnClick)  (VARIANT_BOOL * pbNavOnClick);
    STDMETHOD(put_NavigateOnClick)  (VARIANT_BOOL bNavOnClick);
    STDMETHOD(get_ChannelTime)      (int * pnChannelTime);
    STDMETHOD(put_ChannelTime)      (int nChannelTime);
    STDMETHOD(get_RestartTime)      (DWORD * pdwRestartTime);
    STDMETHOD(put_RestartTime)      (DWORD dwRestartTime);
    STDMETHOD(get_PlaySounds)       (VARIANT_BOOL * bMuteSounds);
    STDMETHOD(put_PlaySounds)       (VARIANT_BOOL pbMuteSounds);
    STDMETHOD(ShowDialog)           (HWND hwndParent);
    STDMETHOD(Apply)                ();

    // ISupportsErrorInfo
    STDMETHOD(InterfaceSupportsErrorInfo)   (REFIID riid);

    // IObjectSafety
    STDMETHOD(GetInterfaceSafetyOptions)    (REFIID riid, DWORD * pdwSupportedOptions, DWORD * pdwEnabledOptions);
    STDMETHOD(SetInterfaceSafetyOptions)    (REFIID riid, DWORD dwOptionsSetMask, DWORD dwEnabledOptions);
};

/////////////////////////////////////////////////////////////////////////////
// Helper functions
/////////////////////////////////////////////////////////////////////////////
int myatoi(LPSTR pch);

#endif  // __SAVER_H__
