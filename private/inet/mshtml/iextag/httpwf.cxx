//=========================================================================
//
//  File : httpwf.cxx
//
//  purpose : implementation of the Cwfolders class
//
//=========================================================================
// Chad Lindhorst, 1998

#include "headers.h"
#include "httpwfh.h"
#include "utils.hxx"
// This define keeps shsemip.h from redefining Assert
#define DONT_WANT_SHELLDEBUG     
#include <shsemip.h>
#include "iextag.h"
#include "htiface.h"  // for ITargetFrame
#include "msdaipper.h"  // only needed for IPP_E_SERVERTYPE_NOT_SUPPORTED
// This #define was from oledberr.h, but only shows up with some versions
// of OLEDB.  (OLEDBVER = 0x210)  Hopefully this is less destructive than
// setting OLEDBVER.
#define DB_E_TIMEOUT                     ((HRESULT)0x80040E97L)
#define DB_E_CANNOTCONNECT               ((HRESULT)0x80040E96L)
#include "oledberr.h"

// ========================================================================
// Cwfolders
// ========================================================================

//+------------------------------------------------------------------------
//
//  Members:    Cwfolders::Cwfolders
//              Cwfolders::~Cwfolders
//
//  Synopsis:   Constructor/destructor
//
//-------------------------------------------------------------------------

Cwfolders::Cwfolders() 
{
    m_pSite = NULL;
    m_pwb = NULL;
}

Cwfolders::~Cwfolders() 
{
    ReleaseInterface (m_pwb);
    ReleaseInterface (m_pSite);
}

// ========================================================================
// Iwfolders
// ========================================================================

//+------------------------------------------------------------------------
//
//  Member:     Cwfolders::navigate
//
//  Synopsis:   Navigates the browser to a folder view of the URL passed 
//              in.  The function requires that m_hwndOwner and m_pwb are
//              both inited. (done in Cwfolders::Init)
//
//-------------------------------------------------------------------------

STDMETHODIMP
Cwfolders::navigate(BSTR bstrUrl, BSTR * pbstrRetVal)
{
    HRESULT hr = navigateInternal(bstrUrl, NULL, /*NULL,*/ USE_NO_UI);
    SetScriptErrorMessage (hr, pbstrRetVal);
    return S_OK;
}

//+------------------------------------------------------------------------
//
//  Member:     Cwfolders::navigateFrame
//
//  Synopsis:   Navigates the browser to a folder view of the URL passed 
//              in.  This version also allows for the folder view to be
//              pointed to a specific frame, as indicated by bstrTargetFrame.
//              The Protocol parameter can be either "WEC","DAV", or "any"
//              the latter meaning that the protocol doesn't matter.
//              Protocol selection is currently a BUGBUG.  This requires
//              the m_hwndOwner and m_pwb members to be inited.
//
//-------------------------------------------------------------------------

STDMETHODIMP 
Cwfolders::navigateFrame(BSTR bstrUrl, BSTR bstrTargetFrame, /*BSTR bstrProtocol,*/
                         BSTR * pbstrRetVal) 
{
    HRESULT hr = navigateInternal(bstrUrl, bstrTargetFrame, /*bstrProtocol,*/ USE_NO_UI);
    SetScriptErrorMessage (hr, pbstrRetVal);
    return S_OK;
}

//+------------------------------------------------------------------------
//
//  Member:     Cwfolders::navigateNoSite
//
//  Synopsis:   This does a folder navigation but doesn't require the behavior
//              to be started in the normal way.  (you can just call this
//              with the punk.  That punk should be to an object with an
//              IWebBrowser2 interface.)  
//
//-------------------------------------------------------------------------

STDMETHODIMP
Cwfolders::navigateNoSite (BSTR bstrUrl, BSTR bstrTargetFrame, /*BSTR bstrProtocol,*/
               DWORD dwhwnd, IUnknown* punk)
{
    HRESULT hr = E_FAIL;

    if ((!(m_hwndOwner = (HWND)LongToHandle(dwhwnd))) || !punk)
    {
        goto done;
    }

    // We should release any old members here
    if (m_pwb)
    {
        ReleaseInterface (m_pwb);
        m_pwb = NULL;
    }

    hr = punk->QueryInterface(IID_IWebBrowser2, (void **) &m_pwb);
    if (FAILED (hr))
        m_pwb = NULL;
    
    if (m_pwb)
        hr = navigateInternal(bstrUrl, bstrTargetFrame, /*bstrProtocol,*/ USE_ALL_UI);
    else
        hr = E_FAIL;

done:
    return hr;
}

// ========================================================================
// IElementBehaviorSite
// ========================================================================

//+------------------------------------------------------------------------
//
//  Member:     Cwfolders::Init
//
//  Synopsis:   Called when this code is initialized as a behavior.  This
//              sets up the m_pwb, m_pSite, and m_hwndOwner members as 
//              well.
//
//-------------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE 
Cwfolders::Init (IElementBehaviorSite __RPC_FAR *pBehaviorSite) 
{
    HRESULT hr = E_INVALIDARG;

    if (pBehaviorSite != NULL)
    {
        m_pSite = pBehaviorSite;
        m_pSite->AddRef();
    
        if (m_pwb)
        {
            ReleaseInterface(m_pwb);
            m_pwb = NULL;
        }

        // gets browser window handle (for ui)
        hr = GetClientSiteWindow(m_pSite, &m_hwndOwner);
        if (SUCCEEDED(hr))
            // Get the browser
            hr = IUnknown_QueryService(m_pSite, SID_SWebBrowserApp, 
                                       IID_IWebBrowser2, (LPVOID *) &m_pwb);     
    }
    return hr;  
}

//+------------------------------------------------------------------------
//
//  Member:     Cwfolders::Notify
//
//  Synopsis:   Not really used, but needed by the interface...
//
//-------------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE 
Cwfolders::Notify (LONG lEvent, VARIANT __RPC_FAR *pVar) 
{
    return S_OK;    
}

// ========================================================================
// IObjectSafety
// ========================================================================

//+------------------------------------------------------------------------
//
//  Member:     Cwfolders::SetInterfaceSafetyOptions
//
//  Synopsis:   Prevents those pesky security dialogs from popping up.
//              We are ALWAYS secure.
//
//-------------------------------------------------------------------------

STDMETHODIMP 
Cwfolders::SetInterfaceSafetyOptions (REFIID riid,
    DWORD dwSupportedOptions, DWORD dwEnabledOptions) 
{
    if (riid == IID_Iwfolders) 
    {
        // Since this object is always safe to use, no matter the input,
        // this returns S_OK all the time.
        return S_OK;
    }
    return IObjectSafetyImpl<Cwfolders>::SetInterfaceSafetyOptions (
        riid, dwSupportedOptions, dwEnabledOptions);
}

// ========================================================================
// Internal helper functions
// ========================================================================

//+------------------------------------------------------------------------
//
//  Member:     Cwfolders::navigateInternal
//
//  Synopsis:   All the other navigate functions call this one... this is
//              the one that does everything.  It first creates a thread
//              which will run ParseDisplayName from office.  Then, it 
//              makes a window that will recieve messages from the new 
//              thread.  Finally, it spends it's time in a DialogBox
//              call.  The normal flow is like this:
//
//  PDN Thread ---------WM_WEBFOLDER_NAV----------> Message Window
//  (PDN thread dies)
//  Message Window -----WM_WEBFOLDER_DONE---------> Dialog Window
//  (dialog and message windows close, this function returns)
//
//              The dialog we put up has a cancel button though....
//
//  (user hits cancel)
//  Dialog Window ------WM_WEBFOLDER_CANCEL-------> MessageWindow
//  MessageWindow ------WM_WEBFOLDER_DONE---------> DialogWindow
//  (dialog window closes, this function returns)
//  PDN Thread ---------WM_WEBFOLDER_NAV----------> MessageWindow
//  (PDN thread dies and message window closes)
//
//              The argsThreadArgs local variable in this function is
//              used to pass information between this function, the
//              message window, the dialog window, and the PDN thread.
//              Be careful that this variable isn't used by more than one
//              piece of code at once!!
//
//-------------------------------------------------------------------------

STDMETHODIMP
Cwfolders::navigateInternal(BSTR bstrUrl, BSTR bstrTargetFrame, /*BSTR bstrProtocol,*/ int iUI) 
{
    HRESULT hr = E_FAIL;
    DWORD dwScheme;

    // This is what is used to pass all sorts of stuff between the different
    // windows and threads below.
    CThreadArgs argsThreadArgs;

    DWORD dwThread = 0;
    void * pThread = NULL;

    HWND hwndMessage = 0;

    WCHAR wszClassName [] = WFOLDERSWNDCLASS;

    uCLSSPEC classpec;
    URL_COMPONENTS urlcomp;
    VARIANT vTarget;
    LPITEMIDLIST pidl = NULL;

    // sanity check.
    if (!bstrUrl)
        BAILOUT (E_INVALIDARG);

    // Can't be longer than MAX_WEB_FOLDER_LENGTH
    if (SysStringLen(bstrUrl) > MAX_WEB_FOLDER_LENGTH)
    {
        if (iUI & USE_ERROR_BOXES)
            ShowError (m_hwndOwner, IDS_ERRORURLTOOLONG, 
                       IDS_ERRORURLTOOLONGTITLE, NULL); 
        BAILOUT (E_INVALIDARG);
    }

    /*
    // Check and make sure the protocol makes sense.
    if (bstrProtocol && (! (0==StrCmpICW(L"wec", bstrProtocol) ||
                            0==StrCmpICW(L"dav", bstrProtocol) ||
                            0==StrCmpICW(L"any", bstrProtocol))))
    {
        BAILOUT (E_INVALIDARG);
    }
    */

    // if the url is ftp, then we are just going to assume that Office
    // can't handle it.  We save the user the fun of the 10 minute IOD
    memset (&urlcomp, 0, sizeof(urlcomp));
    urlcomp.dwStructSize = sizeof(urlcomp);
    urlcomp.dwHostNameLength = 1;
    urlcomp.dwUrlPathLength = 1;
    urlcomp.dwExtraInfoLength = 1;

    if (!InternetCrackUrlW(bstrUrl, 0, 0, &urlcomp))
        BAILOUT(E_INVALIDARG);

    /*
    if (urlcomp.nScheme != INTERNET_SCHEME_HTTP &&
        urlcomp.nScheme != INTERNET_SCHEME_HTTPS &&
        bstrProtocol && (0==StrCmpICW(L"dav", bstrProtocol) ||
                         0==StrCmpICW(L"wec", bstrProtocol)))
    {
        BAILOUT(E_INVALIDARG);
    }
    */
    
    if (urlcomp.nScheme == INTERNET_SCHEME_FTP ||
        urlcomp.nScheme == INTERNET_SCHEME_FILE)
    {
        if (iUI & USE_FAILED_QUESTION)
        {
            ShowNavigationFailedQuestion (bstrUrl, bstrTargetFrame); 
            goto done;
        }
        else BAILOUT(IPP_E_SERVERTYPE_NOT_SUPPORTED);
        
        //vTarget.vt = VT_BSTR;
        //vTarget.bstrVal = bstrTargetFrame;

        //m_pwb->Navigate (bstrUrl, NULL, &vTarget, NULL, NULL);
        //BAILOUT(S_OK);
    }
    

    // JIT install Office NSE.
    // - note, do not call JIT on NT5 since it is not supported
    OSVERSIONINFO osVersionInfo;
    osVersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    if (GetVersionEx(&osVersionInfo) &&
        (osVersionInfo.dwPlatformId == VER_PLATFORM_WIN32_NT ) &&
        (osVersionInfo.dwMajorVersion > 4)   )
    {
      // this is NT5, no need to call JIT
    }
    else
    {
        classpec.tyspec=TYSPEC_CLSID;
        classpec.tagged_union.clsid=CLSID_IOD;    
        if (FAILED(FaultInIEFeature(m_hwndOwner, &classpec, NULL, 0)))
            BAILOUT(S_FALSE);  // this is where the script error code "CANCEL" originates
    }
    
/****************************** DANPOZ    ************************
    // Give office an URL and ask for a pidl back.  Do that in a 
    // seperate thread.  
    argsThreadArgs.m_hwndDialog = 0;     // must be done in case
                                       // the message thread needs to
                                       // find out if the dialog is open
    argsThreadArgs.m_bstrUrl = SysAllocString (bstrUrl);
    argsThreadArgs.m_pidl = NULL;
    argsThreadArgs.m_hrReady = E_FAIL;
    argsThreadArgs.m_ipdnStatus = READY_WORKING;
    argsThreadArgs.m_imsgStatus = READY_WORKING;


    // make message window      
    WNDCLASSEX wcex;
        
    wcex.cbSize         = sizeof(WNDCLASSEX); 
    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = (WNDPROC)NavMessageProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = g_hInst;
    wcex.hIcon          = NULL;
    wcex.hCursor        = NULL;
    wcex.hbrBackground  = NULL;
    wcex.lpszMenuName   = NULL;
    wcex.lpszClassName  = wszClassName;
    wcex.hIconSm        = NULL;

    RegisterClassEx(&wcex);   

    argsThreadArgs.m_hwndMessage = CreateWindow(wszClassName, NULL, NULL,
        0, 0, 0, 0, NULL, NULL, g_hInst, NULL);        
    if (argsThreadArgs.m_hwndMessage == NULL)
        BAILOUT (E_FAIL);
    SendMessage (argsThreadArgs.m_hwndMessage, WM_WEBFOLDER_INIT, 0, (LPARAM) &argsThreadArgs);

    // run Office's ParseDisplayName in a different thread.  It will be
    // sending messages to the argsThreadArgs.hwndMessage.  It copies all
    // information from that structure it needs first, then sets a flag
    // which allows this procedure to deallocate the memory.
    pThread = CreateThread (NULL, 0, RunParseDisplayName, &argsThreadArgs, 0, &dwThread);    

    if (pThread == NULL)
    {
        DestroyWindow (hwndMessage);
        BAILOUT (E_FAIL);
    }

    // Wait until RunParseDisplayName makes copies of the data it needs before
    // deleting it. (which would happen if the function returns)
    while (argsThreadArgs.m_ipdnStatus == READY_WORKING)
        Sleep(0);

    // our fancy dialog (with animation!)
    DialogBoxParam (g_hInst, MAKEINTRESOURCE (IDD_WEBFOLDER_SEARCH), m_hwndOwner, 
        (DLGPROC) WaitDlgProc, (LPARAM) &argsThreadArgs);

    SetFocus (m_hwndOwner);
    
    if (argsThreadArgs.m_imsgStatus == READY_DONE)  //user didn't hit cancel

    {
        hr = argsThreadArgs.m_hrReady;
******************************** DANPOZ *****************************/

/******************************** DANPOZ *****************************/
        hr = CreateWebFolderIDList( bstrUrl, &pidl, m_hwndOwner);
/**********************************************************************/

        switch (hr)
        {
            case S_OK:
                // navigate to the pidl returned by office
                // ---------------------------------------

                // the message handling window packs the pidl it got from
                // office (through WM_WEBFOLDER_NAV) into the .pidl of
                // argsThreadArgs.
                /******** ************** DANPOZ *********************
                hr = NavToPidl (argsThreadArgs.m_pidl, bstrTargetFrame, m_pwb);
                *****************************************************/
                /*********************** DANPOZ NEW ***************/
                hr = NavToPidl (pidl, bstrTargetFrame, m_pwb);
                /*********************************  ***************/
                if (FAILED (hr))
                {
                    hr = E_FAIL;
                    if (iUI & USE_WEB_PAGE_UI)
                        ShowNavigationFailed (m_hwndOwner, bstrUrl, bstrTargetFrame,
                                              L"res://shdoclc.dll/http_gen.htm");
                    goto done;
                }
                else
                    hr = S_OK;
                break;
            case E_OUTOFMEMORY: //handled lower, under cleanup
                break;
            case MK_E_CONNECTMANUALLY: //returned to us if we sent Office an ftp or file url.  Shouldn't
                                       // happen at all (we filter those.) unless called via script
            case IPP_E_SERVERTYPE_NOT_SUPPORTED:
                // The server doesn't support our extensions, but does 
                // exist
                if (iUI & USE_FAILED_QUESTION)
                {
                    ShowNavigationFailedQuestion (bstrUrl, bstrTargetFrame); 
                    goto done;
                }
                break;
            case DB_SEC_E_PERMISSIONDENIED:
                // authentication failed
                if (iUI & USE_WEB_PAGE_UI)
                    ShowNavigationFailed (m_hwndOwner, bstrUrl, bstrTargetFrame,
                                          L"res://shdoclc.dll/http_403.htm");
                break;
            case STG_E_ACCESSDENIED:
            case STG_E_SHAREVIOLATION:
            case IPP_E_OFFLINE:
                hr = IPP_E_OFFLINE;
                if (iUI & USE_ERROR_BOXES)
                    ShowError (m_hwndOwner, IDS_ERROROFFLINE, 
                               IDS_ERROROFFLINETITLE, NULL); 
                break;
            case DB_E_TIMEOUT:
                // bind procedure timed out
                if (iUI & USE_WEB_PAGE_UI)
                    ShowNavigationFailed (m_hwndOwner, bstrUrl, bstrTargetFrame,
                                          L"res://shdoclc.dll/dnserror.htm");
                break;
            case ERROR_MOD_NOT_FOUND:
            case 0x8007007e: // this value is coming back.  VC 6 looks it up like
                             // ERROR_MOD_NOT_FOUND which is really what happened
                             // (office wasn't there)  But ERROR_MOD_NOT_FOUND 
                             // has a different number. 
                // This gets called when the IOD passes, but the namespace
                // extension isn't there.  This should never get called.
                hr = E_FAIL;
                break;
            case MK_E_SYNTAX:
            case MK_E_NOOBJECT:
            case MK_E_UNAVAILABLE:
            case MK_E_NOSTORAGE:
            case E_INVALIDARG:     
                //hr = MK_E_NOOBJECT;
                //if (iUI & USE_WEB_PAGE_UI)
                //    ShowNavigationFailed (m_hwndOwner, bstrUrl, bstrTargetFrame,
                //                          L"res://shdoclc.dll/dnserror.htm");
                //break;

                //BUGBUG - hack follows for bug 47127 - We "should" catch most real
                // invalid args in the param checking at the beginning of this method,
                // but forcing hr to IPP_E_SERVERTYPE_NOT_SUPPORTED makes it impossible to
                // detect a real invalid arg situation that we missed above (i.e. targetframe)
                hr = IPP_E_SERVERTYPE_NOT_SUPPORTED; //Force correct error code for script

                //Fall through as per bug 43338
            default:
#ifdef NONB2_HACK                
                hr = E_FAIL;
                // Something went wrong, we know not what.  Give a relatively
                // general error page.
                if (iUI & USE_WEB_PAGE_UI)
                    ShowNavigationFailed (m_hwndOwner, bstrUrl, bstrTargetFrame,
                                          L"res://shdoclc.dll/http_gen.htm");
                goto done;
                break;
#else
                // For beta 2 (bug 43338) we're going to 
                if (iUI & USE_FAILED_QUESTION)
                {
                    ShowNavigationFailedQuestion (bstrUrl, bstrTargetFrame); 
                    goto done;
                }
                break;
#endif
        }
/****************** DANPOZ ***************************
    }
    else
        hr = S_FALSE;
******************************************************/

cleanup:
    if (hr == E_OUTOFMEMORY)
        if (iUI & USE_ERROR_BOXES)
           ShowError (m_hwndOwner, IDS_ERROROUTOFMEMORY, 
                      IDS_ERROROUTOFMEMORYTITLE, NULL); 
    if (hr == E_FAIL)
        if (iUI & USE_ERROR_BOXES)
            ShowError (m_hwndOwner, IDS_ERRORINTERNAL, 
                       IDS_ERRORINTTITLE, NULL); 
    if (hr == E_INVALIDARG)
        if (iUI & USE_WEB_PAGE_UI)
            ShowNavigationFailed (m_hwndOwner, bstrUrl, bstrTargetFrame,
                                  L"res://shdoclc.dll/http_gen.htm");   

done:
    return hr;
}

//+------------------------------------------------------------------------
//
//  Member:     Cwfolders::ShowNavigationFailedQuestion
//
//  Synopsis:   This is what we want to call when we can't get a folder 
//              view of the resource, but we might be able to get a file
//              view (normal).  This asks the user if he/she wants us to
//              try.  Navigates to the target frame.
//
//-------------------------------------------------------------------------

void 
Cwfolders::ShowNavigationFailedQuestion (BSTR bstrUrl, BSTR bstrTargetFrame)
{
    VARIANT vEMPTY;
    VariantInit (&vEMPTY);
    VARIANT * PVAREMPTY = &vEMPTY;
        
    WCHAR wszMessage [MAX_LOADSTRING+1];
    WCHAR wszTitle   [MAX_LOADSTRING+1];

    LoadString(g_hInst, IDS_ERRORBADSERVER, wszMessage, ARRAYSIZE(wszMessage));
    LoadString(g_hInst, IDS_ERRORBADSERVERTITLE, wszTitle, ARRAYSIZE(wszTitle));
    WCHAR * wsErrorMessage = new WCHAR [MAX_LOADSTRING+wcslen(bstrUrl)+1];
    if (!wsErrorMessage)
    {
        ShowError (m_hwndOwner, IDS_ERROROUTOFMEMORY, 
                   IDS_ERROROUTOFMEMORYTITLE, NULL); 
        delete[] wsErrorMessage;
        return;
    }
    wnsprintf (wsErrorMessage, MAX_LOADSTRING+wcslen(bstrUrl), wszMessage, bstrUrl);
    int iButton = MessageBox (m_hwndOwner, 
                              wsErrorMessage,
                              wszTitle,
                              MB_YESNO | MB_ICONQUESTION);
    delete[] wsErrorMessage;
    if (iButton == IDYES)
    {
        VARIANT vTarget;
        vTarget.vt = VT_BSTR;
        vTarget.bstrVal = bstrTargetFrame;

        m_pwb->Navigate (bstrUrl, NULL, &vTarget, NULL, NULL);
    }
}

//+------------------------------------------------------------------------
//
//  Member:     Cwfolders::NavToPidl
//
//  Synopsis:   Comparatively low level helper function that navigates
//              a given pidl (from office) to the target frame given.
//              The web browser 2 arg does not have to refer to the
//              correct frame.
//
//-------------------------------------------------------------------------

HRESULT 
Cwfolders::NavToPidl (LPITEMIDLIST pidl, BSTR bstrTargetFrame, IWebBrowser2 * pwb)
{
    VARIANT vPidl;
    HRESULT hr;
    IWebBrowser2 * pwbf = NULL;
    ITargetFrame * ptf = NULL;
    IUnknown * punknown = NULL;
    VARIANT vEMPTY;
    VariantInit (&vEMPTY);
    VARIANT * PVAREMPTY  = &vEMPTY;

    hr = InitVARIANTFromPidl(&vPidl, pidl);
    if (FAILED(hr))
        return hr;

    // This is here to change frames, or to make a new one as the case my
    if (bstrTargetFrame && (0!=StrCmpICW(L"", bstrTargetFrame)))
    {
        hr = pwb->QueryInterface(IID_ITargetFrame, (void **)&ptf);     
        FAILONBAD_HR(hr);
        hr = ptf->FindFrame (bstrTargetFrame, (IUnknown *) pwb, 0, &punknown);
        if (FAILED (hr))
        {
            hr = CreateTargetFrame (bstrTargetFrame, &punknown);
        }
        FAILONBAD_HR(hr);
        hr = punknown->QueryInterface(IID_IWebBrowser2, (void **)&pwbf);
        FAILONBAD_HR(hr);
    }
    else
    {
        pwbf = pwb;
    }

    // the actual navigation!
    hr = pwbf->put_Visible (VARIANT_TRUE);
    if (SUCCEEDED (hr))
    {
        pwbf->Stop();
        hr = pwbf->Navigate2(&vPidl, PVAREMPTY, PVAREMPTY, PVAREMPTY, PVAREMPTY);
    }
cleanup:
    ReleaseInterface (ptf);
    if (pwb != pwbf)
        ReleaseInterface (pwbf);
    ReleaseInterface (punknown);

    return hr;
}

//+------------------------------------------------------------------------
//
//  Member:     Cwfolders::RunParseDisplayName
//
//  Synopsis:   Runs ParseDisplayName.  This is abstracted to use as a
//              seperate thread.
//
//-------------------------------------------------------------------------

DWORD WINAPI 
Cwfolders::RunParseDisplayName (LPVOID pArguments)
{
    HRESULT hr = E_FAIL;

    CThreadArgs *pInfo = (CThreadArgs *)pArguments;
    LPITEMIDLIST pidl = NULL;
    HWND hwndMessage = pInfo->m_hwndMessage;
    BSTR bstrUrl = SysAllocString (pInfo->m_bstrUrl);

    // This will signal that pInfo is safe to be released (we have all the info 
    // we need out of it)  pInfo should NEVER be referenced in this function
    // after this value is set.
    pInfo->m_ipdnStatus = READY_INITIALIZED;

    if (!bstrUrl)
    {
        PostMessage (hwndMessage, WM_WEBFOLDER_NAV, 0, (LPARAM) E_OUTOFMEMORY);
        goto cleanup;
    }

    // calls the appropriate ParseDisplayName
    /******************DANPOZ*********************
    hr = CreateWebFolderIDList(bstrUrl, &pidl);
    **********************************************/
    // because of some OLE restrictions, this call must be PostMesage,
    // not SendMessage.
    PostMessage (hwndMessage, WM_WEBFOLDER_NAV, (WPARAM) pidl, (LPARAM) hr);
    
cleanup:
    SysFreeString (bstrUrl);
    return hr;
}

//+------------------------------------------------------------------------
//
//  NAME:         Cwfolders::WaitDlgProc
//
//  SYNOPSIS:     This is the handler for our little dialog.  It will get
//                a WM_WEBFOLDER_DONE message when it should die.
//
//-------------------------------------------------------------------------

INT_PTR CALLBACK 
Cwfolders::WaitDlgProc (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    CThreadArgs * pInfo = NULL;
    HWND hAnimation;
    HWND hStatic;

    switch (message)
    {
    case WM_COMMAND:
        if (LOWORD(wParam) == IDCANCEL) 
        {
            pInfo = (CThreadArgs *) GetProp (hDlg, __INFO);
            if (!pInfo)
                return E_FAIL;
            PostMessage (pInfo->m_hwndMessage, WM_WEBFOLDER_CANCEL, 0, 0);
        }   
        return TRUE;

    case WM_INITDIALOG:
        if (!lParam)
            return E_INVALIDARG;
        pInfo = (CThreadArgs *) lParam;
        // if this is true then there is no message window (PDN thread closed
        // it before we could even start the wait dialog.)  Just close the
        // dialog because everything is already done.
        if (pInfo->m_imsgStatus == READY_DONE)
            EndDialog (hDlg, 0);
        else
        {
            if (!SetProp (hDlg, __INFO, (void *)lParam))
                return E_FAIL;
            // Change our text string to something with the URL in it.
            // If this section fails for any reason, the dialog will keep
            // the generic "IE is looking for your folder" message.
            {
                WCHAR wsMessage [MAX_LOADSTRING+1]; 
                WCHAR *wsStatus = new WCHAR[wcslen(pInfo->m_bstrUrl)+1+MAX_LOADSTRING];
                if (wsStatus)
                {
                    LoadString ( g_hInst, IDS_WEBFOLDER_FIND, wsMessage, ARRAYSIZE(wsMessage) );

                    DWORD dwResultSize;
                    WCHAR wsResult [INTERNET_MAX_URL_LENGTH]; 

                    if (SUCCEEDED (CoInternetParseUrl(pInfo->m_bstrUrl, PARSE_DOMAIN, 
                         0, wsResult, ARRAYSIZE(wsResult), &dwResultSize, 0)))
                    {
                        wnsprintf (wsStatus, wcslen(pInfo->m_bstrUrl)+MAX_LOADSTRING, 
                            wsMessage, wsResult);
                        hStatic = GetDlgItem (hDlg, IDC_WEBFOLDER_MESSAGE);
                        if (hStatic)
                            SendMessage (hStatic, WM_SETTEXT, 0, (LPARAM) wsStatus);
                    }
                    
                    delete[] wsStatus;
                }
            }
            // At this point, we are ready to get messages from the
            // message window, so let it know our hwnd.
            pInfo->m_hwndDialog = hDlg;
            hAnimation = GetDlgItem (hDlg, IDC_WEBFOLDER_ANIMATE);
            if (hAnimation)
                Animate_Open(hAnimation, MAKEINTRESOURCE(IDA_ISEARCH));
        }
        return S_OK;

    case WM_WEBFOLDER_DONE:
        // The message window is closing us.
        EndDialog (hDlg, 0);
        break;

    case WM_DESTROY:
        hAnimation = GetDlgItem (hDlg, IDC_WEBFOLDER_ANIMATE);
        if (hAnimation)
        {
            Animate_Close(hAnimation);
        }        
        RemoveProp (hDlg, __INFO);
        break;
    }
    return DefWindowProc(hDlg, message, wParam, lParam);
}

//+------------------------------------------------------------------------
//
//  NAME:         Cwfolders::NavMessageProc
//
//  SYNOPSIS:     This is the message handler for the ParseDisplayName call.
//
//-------------------------------------------------------------------------

INT_PTR CALLBACK 
Cwfolders::NavMessageProc (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    CThreadArgs * pInfo = NULL;
    HANDLE hCancel;
    HWND hwndDialog;

    switch (message)
    {
    case WM_CREATE:
        if (!SetProp (hDlg, __CANCEL, STATUS_READY))
            return E_FAIL;
        return S_OK;

    case WM_WEBFOLDER_INIT:
        if (!lParam)
            return E_INVALIDARG;
        pInfo = (CThreadArgs *) lParam;
        if (!SetProp (hDlg, __INFO, (void *)lParam))
            return E_FAIL;
        pInfo->m_hwndMessage = hDlg;
        break;
    
    case WM_WEBFOLDER_CANCEL:
        pInfo = (CThreadArgs *) GetProp (hDlg, __INFO);
        if (!pInfo)
            return E_FAIL;
        if (!SetProp (hDlg, __CANCEL, STATUS_CANCELED))
            return E_FAIL;
        hwndDialog = pInfo->m_hwndDialog;
        // let the navigateInternal call know that we are canceled.
        pInfo->m_imsgStatus = READY_CANCEL;
        if (hwndDialog)
            SendMessage (hwndDialog, WM_WEBFOLDER_DONE, 0, 0);
        return S_OK;

    case WM_WEBFOLDER_NAV:
        hCancel = GetProp (hDlg, __CANCEL);
        if (!hCancel)
            return E_FAIL;
        if (hCancel != STATUS_CANCELED)
        {
            // we wait to get pInfo because if we canceled before, it
            // doesn't exist anymore!
            pInfo = (CThreadArgs *) GetProp (hDlg, __INFO);
            if (!pInfo)
                return E_FAIL;

            pInfo->m_pidl = (LPITEMIDLIST) wParam;
            pInfo->m_hrReady = (HRESULT)lParam;
            hwndDialog = pInfo->m_hwndDialog;
            pInfo->m_imsgStatus = READY_DONE;
            if (hwndDialog)
                SendMessage (hwndDialog, WM_WEBFOLDER_DONE, 0, 0);
        }
        DestroyWindow (hDlg);
        return TRUE;

    case WM_DESTROY:
        RemoveProp (hDlg, __CANCEL);
        RemoveProp (hDlg, __INFO);
        break;
    }
    return DefWindowProc(hDlg, message, wParam, lParam);
}

//+------------------------------------------------------------------------
//
//  Member:     Cwfolders::ShowError
//
//  Synopsis:   Throws up a message box.  The two unsinged ints point to
//              entries in the string table.  If the bstr is set, it will
//              be placed in the %s location in the string table.  
//
//-------------------------------------------------------------------------

void 
Cwfolders::ShowError (HWND hwnd, unsigned int IDSmessage, 
                      unsigned int IDStitle, BSTR bstrUrl)
{
    WCHAR wszMessage [MAX_LOADSTRING+1];
    WCHAR wszTitle   [MAX_LOADSTRING+1];

    LoadString(g_hInst, IDSmessage, wszMessage, ARRAYSIZE(wszMessage));
    LoadString(g_hInst, IDStitle, wszTitle, ARRAYSIZE(wszTitle));
    if (bstrUrl)
    {
        WCHAR * wsErrorMessage = new WCHAR [MAX_LOADSTRING+wcslen(bstrUrl)+1];
        if (!wsErrorMessage)
        {
            ShowError (hwnd, IDS_ERROROUTOFMEMORY, IDS_ERROROUTOFMEMORYTITLE, NULL); 
            return;
        }
        wnsprintf (wsErrorMessage, MAX_LOADSTRING+wcslen(bstrUrl), wszMessage, bstrUrl);
        MessageBox (hwnd, wsErrorMessage, wszTitle, MB_OK | MB_ICONERROR);
        delete[] wsErrorMessage;
    }
    else
        MessageBox (hwnd, wszMessage, wszTitle, MB_OK | MB_ICONERROR);
}
    
//+------------------------------------------------------------------------
//
//  Member:     Cwfolders::ShowNavigationFailed
//
//  Synopsis:   This guy gets called when there is no chance we can show
//              the requested resource in ANY view.  For instance, if the
//              DNS lookup fails.
//
//-------------------------------------------------------------------------

void 
Cwfolders::ShowNavigationFailed (HWND hWnd, BSTR bstrUrl, BSTR bstrTargetFrame,
                                 WCHAR * wszResource)
{
    VARIANT vTarget;
    vTarget.vt = VT_BSTR;

    // +1 for \0 and +1 for '#'
    WCHAR * wszResUrl = new WCHAR [wcslen(bstrUrl) + wcslen(wszResource) + 2];
    
    if (wszResUrl)
    {
        // This is how to hack a different URL into the address bar when
        // showing an error page without having the address of the error
        // page in there too.
        wszResUrl [0] = NULL;
        wcscat (wszResUrl, wszResource);
        wcscat (wszResUrl, L"#");
        wcscat (wszResUrl, bstrUrl);

        vTarget.bstrVal = bstrTargetFrame;
        m_pwb->Navigate (wszResUrl, NULL, &vTarget, NULL, NULL);
        delete [] wszResUrl;
    }
    else
    {
        // damn, the memory allocation failed.  Oh well, so the url in
        // the address bar is wrong.  
        vTarget.bstrVal = bstrTargetFrame;
        m_pwb->Navigate (wszResource, NULL, &vTarget, NULL, NULL);
    }
}
     
//+------------------------------------------------------------------------
//
//  Member:     Cwfolders::CreateWebFolderIDList
//
//  Synopsis:   Asks office nse for a pidl, given an URL.  This was written
//              by Chris Guzak to be compatible with NT 5, which puts the 
//              office nse in a different place from NT 4, 9x.
//
//-------------------------------------------------------------------------

HRESULT 
//Cwfolders::CreateWebFolderIDList(BSTR bstrUrl, LPITEMIDLIST *ppidl)
Cwfolders::CreateWebFolderIDList(BSTR bstrUrl, LPITEMIDLIST *ppidl, HWND hwnd)
{
    IShellFolder *psf = NULL;
    IShellFolder *psfb = NULL;

    HRESULT hr = SHGetDesktopFolder(&psf);
    if (SUCCEEDED(hr))
    {
        IBindCtx *pbc;
        hr = CreateBindCtx(0, &pbc);
        if (SUCCEEDED(hr))
        {
            BIND_OPTS bo;
            memset(&bo, 0, sizeof(bo));
            bo.cbStruct = sizeof(bo);
            bo.grfFlags = BIND_JUSTTESTEXISTENCE;
            bo.grfMode = STGM_CREATE;
            pbc->SetBindOptions(&bo);
            
            // CLSID_MyComputer CLSID_WebFolders
            WCHAR wszPath[] = L"::{20D04FE0-3AEA-1069-A2D8-08002B30309D}\\::{BDEADF00-C265-11D0-BCED-00A0C90AB50F}";

            LPITEMIDLIST pidlo = NULL;
            LPITEMIDLIST pidlurl = NULL;
            ULONG chEaten = 0;
            ULONG ulAttr = SFGAO_FOLDER;

            // first PDN gets the Office shell folder pidl, then bind to it, next PDN tries for a pidl
            // to our specific URL
            if (SUCCEEDED(hr = psf->ParseDisplayName(NULL, pbc, wszPath, &chEaten, &pidlo, &ulAttr)) ) 
            {
                if( SUCCEEDED(hr = psf->BindToObject (pidlo, NULL, IID_IShellFolder, (void **) &psfb)) )
                { 
                    if( SUCCEEDED(hr = psfb->ParseDisplayName(hwnd, pbc, bstrUrl, &chEaten, &pidlurl, &ulAttr)))
                    {
                        if (!(ulAttr & SFGAO_FOLDER))
                        {
                            // fancy footwork to navigate to the parent folder if one exists
                            WCHAR * wszParentUrl = new WCHAR [1+SysStringLen(bstrUrl)];
                            if (wszParentUrl)
                            {
                                DWORD dwBuffSize = SysStringLen(bstrUrl);
                                // get name of parent folder
                                if (InternetCombineUrlW (bstrUrl, L"", wszParentUrl, &dwBuffSize, ICU_NO_ENCODE))
                                {
                                    ulAttr = SFGAO_FOLDER;
                                    hr = psfb->ParseDisplayName(NULL, pbc, wszParentUrl, &chEaten, &pidlurl, &ulAttr);
                                    if (SUCCEEDED(hr))
                                    {   
                                        if (!(ulAttr & SFGAO_FOLDER))
                                        hr = MK_E_NOSTORAGE;
                                    }
                                }
                                else
                                {
                                    switch (GetLastError()) 
                                    {
                                        case ERROR_BAD_PATHNAME:
                                            hr = MK_E_NOSTORAGE;
                                            break;
                                        case ERROR_INSUFFICIENT_BUFFER:
                                            hr = E_OUTOFMEMORY;
                                            break;
                                        case ERROR_INTERNET_INVALID_URL:
                                            hr = E_INVALIDARG;
                                            break;
                                        default:
                                            hr = E_FAIL;
                                    }
                                }
                                delete [] wszParentUrl;
                            }
                            else
                                hr = E_OUTOFMEMORY;
                        }
                
                        if (pidlurl && pidlo)
                            (*ppidl) = ILCombine (pidlo, pidlurl);

                        ILFree(pidlurl);

                    } // psfb->ParseDisplayName

                } // psf->BindToObject

                ILFree(pidlo);
            } // psf->ParseDisplayName

            pbc->Release();

        } // CreateBindCtx 
    }

    ReleaseInterface (psf);
    ReleaseInterface (psfb);    
    return hr;
}

// ========================================================================
// The following three functions were taken from an SDK example.
// They are here to facilitate the packing of a PIDL into a 
// VARIANT so that explorer will accept it for an argument to
// Navigate2.
// ========================================================================

//+------------------------------------------------------------------------
//
//  Member:     Cwfolders::InitVARIANTFromPidl
//
//  Synopsis:   Packs a pidl into a VARIANT
//
//-------------------------------------------------------------------------

HRESULT 
Cwfolders::InitVARIANTFromPidl(LPVARIANT pvar, LPITEMIDLIST pidl)   
{
    if (!pidl || !pvar) 
    { 
        return E_POINTER;      
    }
    // Get the size of the pidl and allocate a SAFEARRAY of
    // equivalent size      
    UINT cb = ILGetSize(pidl);
    LPSAFEARRAY psa = MakeSafeArrayFromData((LPBYTE)pidl, cb);      
    if (!psa) 
    { 
        VariantInit(pvar);
        return E_OUTOFMEMORY;  
    }
    pvar->vt = VT_ARRAY|VT_UI1;  
    pvar->parray = psa;
    return NOERROR;  
}

//+------------------------------------------------------------------------
//
//  Member:     Cwfolders::MakeSafeArrayFromData
//
//  Synopsis:   Allocates a SAFEARRAY of cbData size and packs pData into
//              it.
//
//-------------------------------------------------------------------------

LPSAFEARRAY 
Cwfolders::MakeSafeArrayFromData(LPBYTE pData, DWORD cbData)   
{
    LPSAFEARRAY psa;      

    if (!pData || 0 == cbData) 
    {
        return NULL;  // nothing to do
    }
    // create a one-dimensional safe array of BYTEs
    psa = SafeArrayCreateVector(VT_UI1, 0, cbData);      
    if (psa) 
    {
        // copy data into the area in safe array reserved for data
        // Note we party directly on the pointer instead of using locking/
        // unlocking functions.  Since we just created this and no one
        // else could possibly know about it or be using it, this is okay.
        memcpy(psa->pvData,pData,cbData);      
    }      
    
    return psa;   
}

//+------------------------------------------------------------------------
//
//  Member:     Cwfolders::ILGetSize
//
//  Synopsis:   Gets the size of the PIDL by walking the item id list
//
//-------------------------------------------------------------------------

UINT 
Cwfolders::ILGetSize(LPITEMIDLIST pidl) 
{
    UINT cbTotal = 0;
    if (pidl) 
    {
        cbTotal += sizeof(pidl->mkid.cb); // Null terminator
        while (pidl->mkid.cb) 
        {
            cbTotal += pidl->mkid.cb;
            pidl = _ILNext(pidl);
        }
    }
    return cbTotal;
}

// ========================================================================
// These two functions live in shdocvw.  They are not, unfortunately,
// exported, so they appear here as well.
// ========================================================================

//+------------------------------------------------------------------------
//
//    NAME:       Cwfolders::ParseTargetType
//
//    SYNOPSIS:   Maps pszTarget into a target class.
//
//    IMPLEMENTATION:
//    Treats unknown MAGIC targets as _self
//
//-------------------------------------------------------------------------

TARGET_TYPE 
Cwfolders::ParseTargetType(LPCOLESTR pszTarget)
{
    const TARGETENTRY *pEntry = targetTable;

    if (pszTarget[0] != '_') return TARGET_FRAMENAME;
    while (pEntry->pTargetValue)
    {
        if (!StrCmpW(pszTarget, pEntry->pTargetValue)) return pEntry->targetType;
        pEntry++;
    }
    //  Treat unknown MAGIC targets as regular frame name! <<for NETSCAPE compatibility>>
    return TARGET_FRAMENAME;

}

//+------------------------------------------------------------------------
//
//  NAME:         Cwfolders::CreateTargetFrame
//
//  SYNOPSIS:     Creates a new window, if pszTargetName is not special
//                target, names it pszTargetName.  returns IUnknown for
//                the object that implements ITargetFrame,IHlinkFrame and
//                IWebBrowserApp.
//
//-------------------------------------------------------------------------

HRESULT 
Cwfolders::CreateTargetFrame(LPCOLESTR pszTargetName, LPUNKNOWN /*IN,OUT*/ *ppunk)
{
    LPTARGETFRAME2 ptgfWindowFrame;
    HRESULT hr = S_OK;

    //  Launch a new window, set it's frame name to pszTargetName
    //  return it's IUnknown. If the new window is passed to us,
    //  just set the target name.

    if (NULL == *ppunk)
    {
#ifndef  UNIX
        hr = CoCreateInstance(CLSID_InternetExplorer, NULL,
                              CLSCTX_LOCAL_SERVER, IID_IUnknown, (LPVOID*)ppunk);
#else
        hr = CoCreateInternetExplorer( IID_IUnknown, 
                                       CLSCTX_LOCAL_SERVER | CLSCTX_INPROC_SERVER,
                                       (LPVOID*) ppunk );
#endif
    }

    if (SUCCEEDED(hr))
    {
        //  Don't set frame name if target is special or missing
        if (pszTargetName && ParseTargetType(pszTargetName) == TARGET_FRAMENAME)
        {
            HRESULT hrLocal;
            hrLocal = (*ppunk)->QueryInterface(IID_ITargetFrame2, (LPVOID *)&ptgfWindowFrame);
            if (SUCCEEDED(hrLocal))
            {
                ptgfWindowFrame->SetFrameName(pszTargetName);
                ptgfWindowFrame->Release();
            }
        }

        // Even if we don't set the frame name, we still want to return
        // success, otherwise we'd have a blank window hanging around.
    }

    return hr;
}

//+------------------------------------------------------------------------
//
//  NAME:         Cwfolders::SetScriptErrorMessage
//
//  SYNOPSIS:     This converts the hresult we would have returned from
//                navigateInternal to a string to pass back to scripts
//
//-------------------------------------------------------------------------

void
Cwfolders::SetScriptErrorMessage (HRESULT hr, BSTR * pbstr)
{
    // These strings are what the scripting language recieves
    // as our return value to navigate and navigateFrame
    // The hr in the switch is the return value of NavigateInternal
    switch (hr)
    {
    case S_OK:
        *pbstr = SysAllocString (L"OK");
        break;
    case IPP_E_SERVERTYPE_NOT_SUPPORTED:
        *pbstr = SysAllocString (L"PROTOCOL_NOT_SUPPORTED");
        break;
    case MK_E_NOOBJECT:
    case DB_E_CANNOTCONNECT:
        *pbstr = SysAllocString (L"LOCATION_DOES_NOT_EXIST");
        break;
    case DB_SEC_E_PERMISSIONDENIED:
        *pbstr = SysAllocString (L"PERMISSION_DENIED");
        break;
    case E_INVALIDARG:
        *pbstr = SysAllocString (L"INVALIDARG");
        break;
    case IPP_E_OFFLINE:
        *pbstr = SysAllocString (L"OFFLINE");
        break;
    case E_OUTOFMEMORY:
        *pbstr = SysAllocString (L"OUTOFMEMORY");
        break;
    case DB_E_TIMEOUT:
        *pbstr = SysAllocString (L"TIMEOUT");
        break;
    case S_FALSE:
        *pbstr = SysAllocString (L"CANCEL");
        break;
    case MK_E_CONNECTMANUALLY: //returned to us if we sent Office an ftp or file url
        *pbstr = SysAllocString (L"PROTOCOL_NOT_SUPPORTED");
        break;
    default:
        *pbstr = SysAllocString (L"FAILED");
        break;
    }
}
