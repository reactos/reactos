/////////////////////////////////////////////////////////////////////////////
// SSWND.H
//
// Definition of CScreenSaverWindow
//
// History:
//
// Author   Date        Description
// ------   ----        -----------
// jaym     08/26/96    Created
// jaym     02/25/97    Changed navigation method to use 
/////////////////////////////////////////////////////////////////////////////
#ifndef __SSWND_H__
#define __SSWND_H__

class CScreenSaverWindow;

#include <hlink.h>
#include <mshtmdid.h>
#include <servprov.h>
#include "if\actsaver.h"
#include "wnd.h"
#include "toolbar.h"
#include "pidllist.h"
#include "bsc.h"

/////////////////////////////////////////////////////////////////////////////
// Typedefs and Constants
/////////////////////////////////////////////////////////////////////////////
#define CURRENT_CHANNEL_NONE    (-1)

typedef BOOL (FAR PASCAL * VERIFYPWDPROC)   (HWND);
typedef HIMC (FAR PASCAL * IMMASSOCPROC)    (HWND, HIMC);

/////////////////////////////////////////////////////////////////////////////
// Private messages
/////////////////////////////////////////////////////////////////////////////
#define WM_ABORTNAVIGATE    (WM_USER+1)
#define WM_DEATHTOSAVER     (WM_USER+2)

/////////////////////////////////////////////////////////////////////////////
// CScreenSaverWindow
/////////////////////////////////////////////////////////////////////////////
class CScreenSaverWindow :  public CWindow,
                            public IServiceProvider,
                            public IOleClientSite,
                            public IOleInPlaceSite,
                            public IOleInPlaceFrame,
                            public IOleContainer,
                            public IOleCommandTarget,
                            public IDispatch
{
friend CWindow;

// Construction/destruction
public:
    CScreenSaverWindow();
    virtual ~CScreenSaverWindow();

    virtual BOOL Create(const RECT & rect, HWND hwndParent, IScreenSaver * pScreenSaver);

    BOOL ALTMouseMode()
    {
        VARIANT_BOOL bNavigateOnClick;

        ASSERT(m_pSSConfig != NULL);
        EVAL(SUCCEEDED(m_pSSConfig->get_NavigateOnClick(&bNavigateOnClick)));

        return (bNavigateOnClick == VARIANT_FALSE);
    }

    void GetFeatures(DWORD * pdwFeatures)
    {
        ASSERT(m_pSSConfig != NULL);
        EVAL(SUCCEEDED(m_pSSConfig->get_Features(pdwFeatures)));
    }

    BOOL ModelessEnabled()
    { return m_bModelessEnabled; }

    void Quit(BOOL bIgnorePassword = FALSE)
    {
        if (bIgnorePassword || VerifyPassword())
        {
            SaveState();
            PostMessage(WM_DEATHTOSAVER); 
        }
    }

    void ShowControls(BOOL bShow);
    void ShowPropertiesDlg(HWND hwndParent);
    BOOL UserIsInteractive();

    BOOL IsOffline()
    {
        VARIANT_BOOL bOffline;
        EVAL(SUCCEEDED(m_pWebBrowser->get_Offline(&bOffline)));
        return (bOffline == VARIANT_TRUE);
    }

    void GoOffline()
    {
        EVAL(SUCCEEDED(m_pWebBrowser->put_Offline(VARIANT_TRUE)));
    }

    void OnChannelChangeTimer();

// Data
protected:
    int                     m_cRef;

    CToolbarWindow *        m_pToolbar;

    BOOL                    m_bModelessEnabled;
    BOOL                    m_bMouseClicked;
    BOOL                    m_bScrollbarVisible;

    IScreenSaver *          m_pScreenSaver;
    IScreenSaverConfig *    m_pSSConfig;

    UINT_PTR                m_idChangeTimer;
    UINT_PTR                m_idControlsTimer;
    UINT_PTR                m_idClickCheckTimer;
    UINT_PTR                m_idReloadTimer;

    CPIDLList *             m_pPIDLList;
    long                    m_lCurrChannel;
    LPITEMIDLIST            m_pidlDefault;

    HWND                    m_hwndContainer;

    IUnknown *              m_pUnkBrowser;
    IWebBrowser2 *          m_pWebBrowser;
    IOleObject *            m_pOleObject;
    IHlinkFrame *           m_pHlinkFrame;

    IDispatch *             m_pHTMLDocument;

    DWORD                   m_dwWebBrowserEvents;
    DWORD                   m_dwWebBrowserEvents2;
    DWORD                   m_dwHTMLDocumentEvents;

    CSSNavigateBSC *        m_pBindStatusCallback;

    HMODULE                 m_hPasswordDLL;
    VERIFYPWDPROC           m_pfnVerifyPassword;
        // Password verification

    HIMC                    m_hPrevBrowserIMC;
    LONG                    m_lVerifingPassword;
        // IME window

// Overrides
protected:
    virtual void OnDestroy();
    virtual BOOL OnCreate(CREATESTRUCT * pcs);
    virtual BOOL OnEraseBkgnd(HDC hDC);
    virtual void OnTimer(UINT nIDTimer);
    virtual LRESULT OnClose() { return 0; }

    virtual LRESULT OnUserMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    virtual HRESULT OnNewWindow(DISPPARAMS *pDispParams, VARIANT *pVarResult);

    virtual HRESULT OnBeforeNavigate(DISPPARAMS * pDispParams, VARIANT * pVarResult);
    virtual HRESULT OnDocumentComplete(BSTR bstrURL);
    virtual HRESULT OnTitleChange(BSTR bstrTitle);

// Implementation
private:
    BOOL    CreateWebBrowser(const RECT & rect);
    HRESULT GetHTMLBodyElement(IHTMLBodyElement ** ppHTMLBodyElement);

    long    FindNextChannel(long lChannelStart);
    HRESULT DisplayCurrentChannel();
    HRESULT DisplayChannel(long lChannel);
    HRESULT DisplayNextChannel();
    HRESULT NavigateToPIDL(LPITEMIDLIST pidl);
    HRESULT NavigateToBSTR(BSTR bstrURL);
    HRESULT NavigateToDefault();

    HRESULT SetConnectedState(BSTR bstrURL);
    HRESULT IsURLAvailable(BSTR bstrURL);

    void    ResetChangeTimer(BOOL bExtendTime = FALSE);

    BOOL    LaunchBrowser(CString & strURL);

    void    InitChannelList();
    void    LoadState();
    void    SaveState();

    void    LoadPasswordDLL();
    void    UnloadPasswordDLL();
    BOOL    VerifyPassword();
        // Manage password verification.

    void    SetHooks();
    void    ReleaseHooks();
        // Manage Keyboard and Mouse hooks.

// Interfaces
public:
    // IUnknown
    STDMETHOD_(ULONG, AddRef)   (THIS);
    STDMETHOD_(ULONG, Release)  (THIS);
    STDMETHOD(QueryInterface)   (THIS_ REFIID riid, LPVOID * ppvObj);

    // IServiceProvider
    STDMETHOD(QueryService)     (THIS_ REFGUID guidService, REFIID riid, LPVOID* ppvObj);

    // IOleClientSite
    STDMETHOD(SaveObject)               (THIS);
    STDMETHOD(GetMoniker)               (THIS_ DWORD, DWORD, IMoniker **);
    STDMETHOD(GetContainer)             (THIS_ IOleContainer **);
    STDMETHOD(ShowObject)               (THIS);
    STDMETHOD(OnShowWindow)             (THIS_ BOOL fShow);
    STDMETHOD(RequestNewObjectLayout)   (THIS);

    // IOleWindow
    STDMETHOD(GetWindow)            (THIS_ HWND * lphwnd);
    STDMETHOD(ContextSensitiveHelp) (THIS_ BOOL fEnterMode);

    // IOleInPlaceSite (also IOleWindow)
    STDMETHOD(CanInPlaceActivate)   (THIS);
    STDMETHOD(OnInPlaceActivate)    (THIS);
    STDMETHOD(OnUIActivate)         (THIS);
    STDMETHOD(GetWindowContext)     (THIS_ IOleInPlaceFrame **ppFrame, IOleInPlaceUIWindow **ppDoc, LPRECT lprcPosRect, LPRECT lprcClipRect, LPOLEINPLACEFRAMEINFO lpFrameInfo);
    STDMETHOD(Scroll)               (THIS_ SIZE scrollExtant);
    STDMETHOD(OnUIDeactivate)       (THIS_ BOOL fUndoable);
    STDMETHOD(OnInPlaceDeactivate)  (THIS);
    STDMETHOD(DiscardUndoState)     (THIS);
    STDMETHOD(DeactivateAndUndo)    (THIS);
    STDMETHOD(OnPosRectChange)      (THIS_ LPCRECT lprcPosRect);

    // IOleInPlaceUIWindow (also IOleWindow)
    STDMETHOD(GetBorder)            (THIS_ LPRECT lprectBorder);
    STDMETHOD(RequestBorderSpace)   (THIS_ LPCBORDERWIDTHS pborderwidths);
    STDMETHOD(SetBorderSpace)       (THIS_ LPCBORDERWIDTHS pborderwidths);
    STDMETHOD(SetActiveObject)      (THIS_ IOleInPlaceActiveObject *pActiveObject, LPCOLESTR pszObjName);

    // IOleInPlaceFrame (also IOleInPlaceUIWindow)
    STDMETHOD(InsertMenus)          (THIS_ HMENU hmenuShared, LPOLEMENUGROUPWIDTHS lpMenuWidths);
    STDMETHOD(SetMenu)              (THIS_ HMENU hmenuShared, HOLEMENU holemenu, HWND hwndActiveObject);
    STDMETHOD(RemoveMenus)          (THIS_ HMENU hmenuShared);
    STDMETHOD(SetStatusText)        (THIS_ LPCOLESTR pszStatusText);
    STDMETHOD(EnableModeless)       (THIS_ BOOL fEnable);
    STDMETHOD(TranslateAccelerator) (THIS_ LPMSG lpmsg, WORD wID);

    // IParseDisplayName
    STDMETHOD(ParseDisplayName) (THIS_ IBindCtx * pbc, LPOLESTR pszDisplayName, ULONG * pchEaten, IMoniker ** ppmkOut);

    // IOleContainer (also IParseDisplayName)
    STDMETHOD(EnumObjects)      (THIS_ DWORD grfFlags, IEnumUnknown ** ppEnum);
    STDMETHOD(LockContainer)    (THIS_ BOOL fLock);

    // IOleCommandTarget
    STDMETHOD(QueryStatus)  (THIS_ const GUID * pguidCmdGroup, ULONG cCmds, OLECMD prgCmds[], OLECMDTEXT * pCmdText);
    STDMETHOD(Exec)         (THIS_ const GUID * pguidCmdGroup, DWORD nCmdID, DWORD nCmdExecOpt, VARIANTARG * pvaIn, VARIANTARG * pvaOut);

    // IDispatch
    STDMETHOD(GetTypeInfoCount) (THIS_ UINT * pctInfo);
    STDMETHOD(GetTypeInfo)      (THIS_ UINT iTInfo, LCID lcid, ITypeInfo ** ppTInfo);
    STDMETHOD(GetIDsOfNames)    (THIS_ REFIID riid, OLECHAR ** rgszNames, UINT cNames, LCID lcid, DISPID * rgDispId);
    STDMETHOD(Invoke)           (THIS_ DISPID dispIdMember, REFIID riid, LCID lcid, USHORT wFlags, DISPPARAMS * pDispParams, VARIANT * pVarResult, EXCEPINFO * pExcepInfo, UINT * puArgErr);
};

/////////////////////////////////////////////////////////////////////////////
// Hook functions
/////////////////////////////////////////////////////////////////////////////
LRESULT CALLBACK KeyboardHookProc(int nCode, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK MouseHookProc(int nCode, WPARAM wParam, LPARAM lParam);

#endif  // __SSWND_H__
