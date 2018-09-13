//=================================================
//
//  File : httpwfh.h
//
//  purpose : definition of the Cwfolders class
//
//=================================================
// Chad Lindhorst, 1998

#ifndef __HTTPWFH_H_
#define __HTTPWFH_H_

#include <mshtmhst.h>

#include "iextag.h"         // for IID_Iwfolders... etc.
#include "resource.h"       // main symbols
#include <shlobj.h>         // needed to define pidls
#include "oledb.h"

// This is used to fix the max size for strings loaded from the
// string table.
#define MAX_LOADSTRING MAX_PATH

// This is the longest URL we should send to office.  (anything longer
// will get an error dialog.)  (Should be MAX_PATH - talk to office...)
#define MAX_WEB_FOLDER_LENGTH            100

// This is the guid we use for active setup....
static const GUID CLSID_IOD = 
{ 0x73fa19d0, 0x2d75, 0x11d2, { 0x99, 0x5d, 0x00, 0xc0, 0x4f, 0x98, 0xbb, 0xc9 } };

// Just to look nice.
#define BAILOUT(HR)             {hr=HR; goto cleanup;}
#define FAILONBAD_HR(HR)        {if (FAILED(HR)) BAILOUT(HR);}

// These typedefs and statics are for the target frame creation code taken
// out of shdocvw.  
typedef enum _TARGET_TYPE {
TARGET_FRAMENAME,
TARGET_SELF,
TARGET_PARENT,
TARGET_BLANK,
TARGET_TOP,
TARGET_MAIN,
TARGET_SEARCH
} TARGET_TYPE;

typedef struct _TARGETENTRY {
    TARGET_TYPE targetType;
    const WCHAR *pTargetValue;
} TARGETENTRY;

static const TARGETENTRY targetTable[] =
{
    {TARGET_SELF, L"_self"},
    {TARGET_PARENT, L"_parent"},
    {TARGET_BLANK, L"_blank"},
    {TARGET_TOP, L"_top"},
    {TARGET_MAIN, L"_main"},
    {TARGET_SEARCH, L"_search"},
    {TARGET_SELF, NULL}
};

// Custom Window Messages
#define WM_WEBFOLDER_NAV                   WM_USER + 2000
#define WM_WEBFOLDER_CANCEL                WM_WEBFOLDER_NAV + 1
#define WM_WEBFOLDER_DONE                  WM_WEBFOLDER_NAV + 2
#define WM_WEBFOLDER_INIT                  WM_WEBFOLDER_NAV + 3

// Used to keep the state of the message window.  They are void * to cram into
// the window properties.
#define STATUS_READY        (void *) 1
#define STATUS_CANCELED     (void *) 2

// Window Property Names
#define __INFO              L"__WFOLDER_INFO"
#define __CANCEL            L"__WFOLDER_CANCEL"

// Name of the class of windows that handles all the messages from ParseDisplayName
#define WFOLDERSWNDCLASS  L"WebFolderSilentMessageHandlerWindowClass"

// These values help the various parts of this program know what is going on.
// They are to be used in ONE DIRECTION ONLY because they are often carried
// to different threads, and the variable they are used with is NOT 
// synchronized.
#define READY_WORKING            0
#define READY_INITIALIZED        1
#define READY_CANCEL             10
#define READY_DONE               11

// These are my different UI codes.  You pass one (or all) of these to NavigateInternal
// to change what UI gets seen.
#define USE_NO_UI                0
#define USE_ERROR_BOXES          1
#define USE_FAILED_QUESTION      2
#define USE_WEB_PAGE_UI          4

#define USE_ALL_UI               USE_ERROR_BOXES | USE_FAILED_QUESTION | USE_WEB_PAGE_UI

//+------------------------------------------------------------------------
//
//  Class:      Cwfolders
//
//  Synopsis:   Implements a behavior which allows the browser to
//              navigate to a folder view of a given URL.  The most
//              important method here is Navigate.
//
//-------------------------------------------------------------------------

class ATL_NO_VTABLE Cwfolders : 
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<Cwfolders,&CLSID_wfolders>,
    public IDispatchImpl<Iwfolders, &IID_Iwfolders, &LIBID_IEXTagLib>,
    public IObjectSafetyImpl<Cwfolders>,
    public IElementBehavior
{
// METHODS
// -------

public:
    Cwfolders();

    ~Cwfolders();

DECLARE_REGISTRY_RESOURCEID(IDR_WFOLDERS)

BEGIN_COM_MAP(Cwfolders) 
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(Iwfolders)
    COM_INTERFACE_ENTRY(IElementBehavior)
    COM_INTERFACE_ENTRY_IMPL(IObjectSafety)
END_COM_MAP()

    // Iwfolders
    STDMETHOD(navigate)(BSTR bstrUrl, BSTR * pbstrRetVal);
    STDMETHOD(navigateFrame)(BSTR bstrUrl, BSTR bstrTargetFrame, /*BSTR bstrProtocol,*/ BSTR * pbstrRetVal);
    STDMETHOD(navigateNoSite)(BSTR bstrUrl, BSTR bstrTargetFrame, /*BSTR bstrProtocol,*/
                              DWORD dwhwnd, IUnknown* punk);

    // IObjectSafety
    STDMETHOD(SetInterfaceSafetyOptions)(REFIID riid,
              DWORD dwSupportedOptions, DWORD dwEnabledOptions);

    // IElementBehavior
    HRESULT STDMETHODCALLTYPE Init (IElementBehaviorSite __RPC_FAR *pBehaviorSite);
    HRESULT STDMETHODCALLTYPE Notify (LONG lEvent, VARIANT __RPC_FAR *pVar);
    STDMETHOD(Detach)() { return S_OK; };


private:
    // The function that actually does all the navigation.
    // all other (including exported) functions call this one.
    STDMETHOD(navigateInternal)(BSTR bstrUrl, BSTR bstrTargetFrame, /*BSTR bstrProtocol,*/ int iUI);

    // show UI for four different kinds of errors.
    void ShowNavigationFailedQuestion (BSTR bstrUrl, BSTR bstrTargetFrame);
    void ShowError (HWND hWnd, unsigned int IDSmessage, unsigned int IDStitle, BSTR bstrUrl);
    void ShowNavigationFailed (HWND hWnd, BSTR bstrUrl, BSTR bstrTargetFrame,
                               WCHAR * wszResource);

// STATIC METHODS
// --------------

public:
    // message handler for my window (the "connecting..." dialog)
    static INT_PTR CALLBACK WaitDlgProc (HWND hDlg, UINT message, 
                               WPARAM wParam, LPARAM lParam);
    // message handler for a silent, invisible message window that
    // stays around to listen for ParseDisplayName to finish
    static INT_PTR CALLBACK NavMessageProc (HWND hDlg, UINT message, WPARAM wParam, 
                                  LPARAM lParam);

private:
    // These two (unexported) functions were taken from the shdocvw code.
    static TARGET_TYPE ParseTargetType(LPCOLESTR pszTarget);
    static HRESULT CreateTargetFrame(LPCOLESTR pszTargetName, LPUNKNOWN *ppunk);

    // Some helper functions for pidl stuffing
    static HRESULT InitVARIANTFromPidl(LPVARIANT pvar, LPITEMIDLIST pidl);
    static LPSAFEARRAY MakeSafeArrayFromData(LPBYTE pData, DWORD cbData);
    static UINT ILGetSize(LPITEMIDLIST pidl);

    // My thread proc
    static DWORD WINAPI RunParseDisplayName (LPVOID pArguments);

    // General helper functions (destined for utils.cxx?)
    static HRESULT NavToPidl (LPITEMIDLIST pidl, BSTR bstrTargetFrame, 
        IWebBrowser2 * pwb);

    // Some code mostly supplied by Chris Guzak that gets a pidl from 
    // an url.
    static HRESULT CreateWebFolderIDList(BSTR bstrUrl, LPITEMIDLIST *ppidl, HWND hwnd);
    static void SetScriptErrorMessage (HRESULT hr, BSTR * pbstr);

// DATA MEMBERS
// ------------

public:
    IWebBrowser2 * m_pwb;
    HWND m_hwndOwner;
    IElementBehaviorSite *m_pSite;
};

// Using this to pass arguments to my child thread.
class CThreadArgs
{
public:
    CThreadArgs()
    {
        m_bstrUrl = NULL;
    }
    ~CThreadArgs()
    {
        SysFreeString (m_bstrUrl);
    }

    // holds the URL ask office for a pidl with
    BSTR m_bstrUrl;
    // holds the hwnd of the message window, once
    // it is created and initialized.  (0 otherwise)
    HWND m_hwndMessage;
    // holds the hwnd of the dialog window, once
    // it is created and initialized.  (0 otherwise)
    HWND m_hwndDialog;
    // holds the HRESULT returned by office
    HRESULT m_hrReady;
    // holds the pidl returned by office 
    // (this actually only carries the pidl between the
    // message window and the navigateInternal call...
    // the pidl gets from the PDN thread to the message
    // window through the WM_WEBFOLDER_NAV message)
    LPITEMIDLIST m_pidl;
    // Holds the status of the message window
    int m_imsgStatus;
    // Holds the status of the PDN thread
    int m_ipdnStatus;
};

#endif //__HTTPWFH_H_
