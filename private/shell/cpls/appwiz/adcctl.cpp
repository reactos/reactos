// adcctl.cpp : Implementation of CADCCtl
#include "priv.h"

// Do not build this file if on Win9X or NT4
#ifndef DOWNLEVEL_PLATFORM

#include "adcctl.h"
#include "shdguid.h"
#include "shguidp.h"
#include "util.h"       // for InstallAppFromFloppy...
#include "datasrc.h"    // for CDataSrc_CreateInstance
#include "dump.h"

EXTERN_C BOOL bForceX86Env;

// Declare the private GUIDs
#include "initguid.h"
#include "iface.h"
#include "mshtml.h"


/*-------------------------------------------------------------------------
Purpose: Returns TRUE if the bstr is an empty string.
*/
inline BOOL IsEmptyBSTR(BSTR bstr)
{
    return bstr == NULL || bstr[0] == 0;
}


//---------------------------------------------------------------------------
//   
//---------------------------------------------------------------------------


BOOL CADCCtl::_IsMyComputerOnDomain()
{
    // NOTE: assume it's on the domain 
    BOOL bRet = TRUE;
#if WINNT
    LPWSTR pszDomain;
    NETSETUP_JOIN_STATUS nsjs;
    
    if (NERR_Success == NetGetJoinInformation(NULL, &pszDomain, &nsjs))
    {
        if (nsjs != NetSetupDomainName)
            bRet = FALSE;
        NetApiBufferFree(pszDomain);
    }
#endif
    return bRet;
}

// constructor
CADCCtl::CADCCtl()
{

    TraceMsg(TF_OBJLIFE, "(Ctl) creating");
    
    // This object cannot be stack-allocated
    ASSERT(NULL == _hwndTB);
    
    ASSERT(FALSE == _fInReset);
    ASSERT(FALSE == _fSecure);

    // We do this so comctl32.dll won't fail to create the DATETIME_PICKER
    // control in case we do "Add Later"
    INITCOMMONCONTROLSEX icex = {0};
    icex.dwSize = sizeof(icex);
    icex.dwICC = ICC_DATE_CLASSES;
    InitCommonControlsEx(&icex);

    _fOnDomain = _IsMyComputerOnDomain();
        
    // Set default sort values
    _cbstrSort = L"displayname";
}


// destructor
CADCCtl::~CADCCtl()
{
    TraceMsg(TF_OBJLIFE, "(Ctl) destroying");

    ATOMICRELEASE(_psam);
    
    _ReleaseAllMatrixObjects();

    _ReleaseAllEventBrokers();
}



//------------------------------------------------------------------------
//
//  These set/get methods implement the control's properties,
//  copying values to and from class members.  They perform no
//  other processing apart from argument validation.
//
//------------------------------------------------------------------------

STDMETHODIMP CADCCtl::get_Dirty(VARIANT_BOOL * pbDirty)
{
    *pbDirty = _fDirty ? VARIANT_TRUE : VARIANT_FALSE;
    return  S_OK;
}

STDMETHODIMP CADCCtl::put_Dirty(VARIANT_BOOL bDirty)
{
    // to its current value.
    _fDirty = (bDirty == VARIANT_TRUE) ? TRUE : FALSE;
    return S_OK;
}

STDMETHODIMP CADCCtl::get_Category(BSTR* pbstr)
{
    *pbstr = _cbstrCategory.Copy();
    return *pbstr ? S_OK : E_OUTOFMEMORY;
}

STDMETHODIMP CADCCtl::put_Category(BSTR bstr)
{
    if (NULL == (LPWSTR)_cbstrCategory || 0 != StrCmpIW(bstr, _cbstrCategory))
    {
        _cbstrCategory = bstr;
        _fCategoryChanged = TRUE;
    }
    return S_OK;
}

STDMETHODIMP CADCCtl::get_DefaultCategory(BSTR* pbstr)
{
    WCHAR sz[64];

    ARPGetPolicyString(L"DefaultCategory", sz, SIZECHARS(sz));

    CComBSTR bstr(sz);
    *pbstr = bstr.Copy();
    return *pbstr ? S_OK : E_OUTOFMEMORY;
}

STDMETHODIMP CADCCtl::get_Sort(BSTR* pbstr)
{
    *pbstr = _cbstrSort.Copy();
    return *pbstr ? S_OK : E_OUTOFMEMORY;
}

STDMETHODIMP CADCCtl::put_Sort(BSTR bstr)
{
    _cbstrSort = bstr;
    return S_OK;
}

STDMETHODIMP CADCCtl::get_Forcex86(VARIANT_BOOL * pbForce)
{
    *pbForce = VARIANT_FALSE;
#ifdef WX86
    *pbForce = bForceX86Env ? VARIANT_TRUE : VARIANT_FALSE;
#endif
    return S_OK;
}

STDMETHODIMP CADCCtl::put_Forcex86(VARIANT_BOOL bForce)
{
#ifdef WX86
    bForceX86Env = (bForce == VARIANT_TRUE) ? TRUE : FALSE;
#else
    // nobody should be calling this if we are not under WX86
    ASSERT(0);
#endif
    return S_OK;
}

//  Property: ShowPostSetup
//      Returns TRUE if the Post Setup Page should be shown.

STDMETHODIMP CADCCtl::get_ShowPostSetup(VARIANT_BOOL * pbShow)
{
    // Only show the page if it is needed
    if (COCSetupEnum::s_OCSetupNeeded())
        *pbShow = VARIANT_TRUE;
    else
        *pbShow = VARIANT_FALSE;
        
    return S_OK;
}

STDMETHODIMP CADCCtl::get_OnDomain(VARIANT_BOOL * pbOnDomain)
{
    *pbOnDomain = _fOnDomain ? VARIANT_TRUE : VARIANT_FALSE;
    
    return  S_OK;
}

STDMETHODIMP CADCCtl::put_OnDomain(VARIANT_BOOL bOnDomain)
{
    // to its current value.
    _fOnDomain = (bOnDomain == VARIANT_TRUE) ? TRUE : FALSE;
    return S_OK;
}


/*-------------------------------------------------------------------------
Purpose: Creates the data source object.  Also initiates the data enumeration.
*/
HRESULT CADCCtl::_CreateMatrixObject(DWORD dwEnum, IARPSimpleProvider ** pparposp)
{
    HRESULT hres;
    IARPSimpleProvider * parposp = NULL;

    TraceMsg(TF_CTL, "(Ctl) creating matrix object");

    // Security check must pass before we can provide a DSO. 
    if (!_fSecure)
    {
        TraceMsg(TF_WARNING, "(Ctl) Security blocked creating DSO");
        hres = E_FAIL;
    }
    else
    {
        hres = THR(CDataSrc_CreateInstance(IID_IARPSimpleProvider, (LPVOID *)&parposp));
        if (SUCCEEDED(hres))
        {
            if (_psam == NULL)
            {
                hres = THR(CoCreateInstance(CLSID_ShellAppManager, NULL, CLSCTX_INPROC_SERVER, 
                                                        IID_IShellAppManager, (LPVOID *)&_psam));
            }
            
            if (_psam)
            {
                ASSERT(_rgparpevt[dwEnum]);
                
                hres = THR(parposp->Initialize(_psam, _rgparpevt[dwEnum], dwEnum));
                if (SUCCEEDED(hres))
                {
                    parposp->SetSortCriteria(_cbstrSort);
                    parposp->SetFilter(_cbstrCategory);
                }
            }

            if (FAILED(hres))
            {
                parposp->Release();
                parposp = NULL;
            }
        }
    }

    *pparposp = parposp;
    
    return hres;
}

/*-------------------------------------------------------------------------
Purpose: Helper method to kill the worker thread
*/
HRESULT CADCCtl::_KillDatasrcWorkerThread(IARPSimpleProvider * parp)
{
    HRESULT hres = S_OK;

    if (parp)
    {
        IARPWorker * pDatasrcWorker;
        
        hres = parp->QueryInterface(IID_IARPWorker, (LPVOID *)&pDatasrcWorker);
        if (SUCCEEDED(hres))
        {
            hres = pDatasrcWorker->KillWT();
            pDatasrcWorker->Release();
        }
    }
    return hres;
}

HRESULT CADCCtl::_ReleaseMatrixObject(DWORD dwIndex)
{
    _fDirty = FALSE;
    _fCategoryChanged = FALSE;

    if (_rgparposp[dwIndex])
    {
        _KillDatasrcWorkerThread(_rgparposp[dwIndex]);
        ATOMICRELEASE(_rgparposp[dwIndex]);

        if (_rgparpevt[dwIndex])
            _rgparpevt[dwIndex]->SetOSPListener(NULL);
    }
    
    return S_OK;
}

/*-------------------------------------------------------------------------
Purpose: Release the control's embedded matrix object.

         Terminates any pending data download.
*/
HRESULT CADCCtl::_ReleaseAllMatrixObjects(void)
{
    int i;
    
    for (i = 0; i < ARRAYSIZE(_rgparposp); i++)
        _ReleaseMatrixObject(i);

    return S_OK;
}


const static struct {
    DWORD dwEnum;
    LPWSTR pszAreaText;
} c_rgEnumAreas[] = {
    {ENUM_INSTALLED,      L"Remove"},
    {ENUM_PUBLISHED,      L"Add"},
    {ENUM_CATEGORIES,     L"Categories"},
    {ENUM_OCSETUP,        L"ocsetup"},
};

/*-------------------------------------------------------------------------
Purpose: Release all the event broker objects.          
*/
HRESULT CADCCtl::_ReleaseAllEventBrokers()
{
    HRESULT hres = S_OK;

    int i;
    for (i = 0; i < NUM_ARP_SIMPLE_PROVIDERS; i++)
    {
        // Release our previous Event Broker.
        ATOMICRELEASE(_rgparpevt[i]);
    }

    return hres;
}

/*-------------------------------------------------------------------------
Purpose: Initialize the event broker object.  This function will create
         if it doesn't exist already.

         bRecreate - if TRUE, the broker object will be released and recreated.
         
*/
HRESULT CADCCtl::_InitEventBrokers(DataSourceListener * pdsl, BOOL bRecreate)
{
    HRESULT hres = S_OK;

    TraceMsg(TF_CTL, "(Ctl) initializing event broker");
    
    int i;
    for (i = 0; i < NUM_ARP_SIMPLE_PROVIDERS; i++)
    {
        if (bRecreate)
        {
            // Release our previous Event Broker.
            ATOMICRELEASE(_rgparpevt[i]);

            //  Create a new event broker for each DSO
            hres = CARPEvent_CreateInstance(IID_IARPEvent, (LPVOID *)&_rgparpevt[i], c_rgEnumAreas[i].pszAreaText);
            if (FAILED(hres))
                break;
        }

        if (_rgparpevt[i])
        {
            // Set the DataSourceListener for the event broker.
            _rgparpevt[i]->SetDataSourceListener(pdsl);
        }

    }

    return hres;
}


/*--------------------------------------------------------------------------
Purpose: Matches the bstrQualifiers passed in to the Enum Areas we have
*/

DWORD CADCCtl::_GetEnumAreaFromQualifier(BSTR bstrQualifier)
{
    DWORD dwEnum = ENUM_UNKNOWN;
    int i;

    for (i = 0; i < ARRAYSIZE(c_rgEnumAreas); i++)
    {
        // (Trident databinding treats qualifiers case-sensitively, so we
        // should too.)
        if (0 == StrCmpW(bstrQualifier, c_rgEnumAreas[i].pszAreaText))
        {
            dwEnum = c_rgEnumAreas[i].dwEnum;
            break;
        }
    }   
    
    return dwEnum; 
}

/*-------------------------------------------------------------------------
Purpose: Creates an object that supports the OLEDBSimpleProvider interface.
         We call this the OSP. This object is the main workhorse that 
         provides data back to the consumer (Trident's databinding agent).

         MSHTML calls this method to obtain the object.  It always passes
         an empty BSTR for bstrQualifier.

         This function can return S_OK and a NULL *ppunk!  It actually is
         required to do this if the OSP doesn't have any data yet.  Then,
         once the OSP has some data, it should fire READYSTATE_COMPLETE and
         dataSetMemberChanged, which will cause this method to be called
         again.

         Our implementation immediately enumerates and fills the OSP before
         returning, so we always hand back an OSP if we return S_OK.
         
*/
STDMETHODIMP CADCCtl::msDataSourceObject(BSTR bstrQualifier, IUnknown **ppunk)
{
    HRESULT hres = E_FAIL;
    *ppunk = NULL;                      // NULL in case of failure

    // Let the event brokers initialize even before we have real data
    if (NULL == _rgparpevt[ENUM_OCSETUP])
        hres = _InitEventBrokers(NULL, TRUE);

    // Check the last event broker to determine if they were created correctly
    if (_rgparpevt[ENUM_OCSETUP])
    {
        DWORD dwEnum = _GetEnumAreaFromQualifier(bstrQualifier);

        TraceMsg(TF_CTL, "(Ctl) msDataSourceObject called for %d", dwEnum);

        if (dwEnum != ENUM_UNKNOWN)
        {
            // Hand back a data source object
            if (NULL == _rgparposp[dwEnum])
            {
                hres = THR(_CreateMatrixObject(dwEnum, &_rgparposp[dwEnum]));
                if (SUCCEEDED(hres))
                {
                    // Tell the OSP to enumerate the items.
                    hres = THR(_rgparposp[dwEnum]->EnumerateItemsAsync());
                    if (FAILED(hres))
                        _ReleaseMatrixObject(dwEnum);
                }
            }
            else
            {
                // Recalculate all the data. 
                hres = _rgparposp[dwEnum]->Recalculate();
                if (SUCCEEDED(hres))
                {
                    // fetch OLEDBSimpleProvider interface pointer and cache it
                    hres = THR(_rgparposp[dwEnum]->QueryInterface(IID_OLEDBSimpleProvider, (void **)ppunk));
                }
            }
        }
    }
    
    if (FAILED(hres))
        TraceMsg(TF_ERROR, "(Ctl) msDataSourceObject failed %s", Dbg_GetHRESULT(hres));
    else
        TraceMsg(TF_CTL, "(Ctl) msDataSourceObject returned %s and %#lx", Dbg_GetHRESULT(hres), *ppunk);

    return hres;
}


const IID IID_IDATASRCListener = {0x3050f380,0x98b5,0x11cf,{0xbb,0x82,0x00,0xaa,0x00,0xbd,0xce,0x0b}};
const IID IID_DataSourceListener = {0x7c0ffab2,0xcd84,0x11d0,{0x94,0x9a,0x00,0xa0,0xc9,0x11,0x10,0xed}};

//------------------------------------------------------------------------
//
//  Method:    addDataSourceListener()
//
//  Synopsis:  Sets the COM object which should receive notification
//             events.
//
//  Arguments: pEvent        Pointer to COM object to receive notification
//                           events, or NULL if no notifications to be sent.
//
//  Returns:   S_OK upon success.
//             Error code upon failure.
//
//------------------------------------------------------------------------

STDMETHODIMP CADCCtl::addDataSourceListener(IUnknown *punkListener)
{
    HRESULT hres = E_FAIL;

    TraceMsg(TF_CTL, "(Ctl) addDataSourceListener called.  Listener is %#lx", punkListener);

    ASSERT(IS_VALID_CODE_PTR(punkListener, IUnknown));
    
    DataSourceListener * pdsl;
    
    // Make sure this is the interface we expect
    hres = punkListener->QueryInterface(IID_DataSourceListener, (void **)&pdsl);
    if (SUCCEEDED(hres))
    {
        _InitEventBrokers(pdsl, FALSE);
        pdsl->Release();
    }
    
    return hres;
}

const TCHAR c_szStubWindowClass[] = TEXT("Add/Remove Stub Window");

HWND _CreateTransparentStubWindow(HWND hwndParent)
{
    WNDCLASS wc;
    RECT rc = {0};
    if (hwndParent)
    {
        RECT rcParent = {0};
        GetWindowRect(hwndParent, &rcParent);
        rc.left = (rcParent.left + RECTWIDTH(rcParent)) / 2;
        rc.top = (rcParent.top + RECTHEIGHT(rcParent)) / 2;
    }
    else
    {
        rc.left = CW_USEDEFAULT;
        rc.top = CW_USEDEFAULT;
    }
        
    if (!GetClassInfo(HINST_THISDLL, c_szStubWindowClass, &wc))
    {
        wc.style         = 0;
        wc.lpfnWndProc   = DefWindowProc;
        wc.cbClsExtra    = 0;
        wc.cbWndExtra    = SIZEOF(DWORD) * 2;
        wc.hInstance     = HINST_THISDLL;
        wc.hIcon         = NULL;
        wc.hCursor       = LoadCursor (NULL, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)GetStockObject (WHITE_BRUSH);
        wc.lpszMenuName  = NULL;
        wc.lpszClassName = c_szStubWindowClass;

        RegisterClass(&wc);
    }

    // WS_EX_APPWINDOW makes this show up in ALT+TAB, but not the tray.
        
    return CreateWindowEx(WS_EX_TRANSPARENT, c_szStubWindowClass, TEXT(""), WS_POPUP | WS_VISIBLE, rc.left,
                          rc.top, 1, 1, hwndParent, NULL, HINST_THISDLL, NULL);
}


/*-------------------------------------------------------------------------
Purpose: Retreive the top level HWND for our HTA host from the clientsite 
         
*/
HRESULT CADCCtl::_GetToplevelHWND()
{
    HRESULT hres = E_FAIL;
    if (_pclientsite)
    {
        IOleInPlaceSite * pops = NULL;

        if (SUCCEEDED(_pclientsite->QueryInterface(IID_IOleInPlaceSite, (void **)&pops)))
        {
            pops->GetWindow(&_hwndTB);
            pops->Release();
            // Do we have a valid window?
            if (_hwndTB)
            {
                // Yes, then go up the hwnd chain to find the top level window
                HWND hwndTmp = NULL;
                while (hwndTmp = ::GetParent(_hwndTB))
                    _hwndTB = hwndTmp;

                hres = S_OK;
            }                
        }
    }

    return hres;
}

/*-------------------------------------------------------------------------
Purpose: Invoke a specific command.  The list of commands are specified
         in the APPCMD enumerated type.

         This method can be called via script.
         
*/
STDMETHODIMP CADCCtl::Exec(BSTR bstrQualifier, BSTR bstrCmd, LONG nRecord)
{
    SetErrorInfo(0, NULL);
    TraceMsg(TF_CTL, "(Ctl) Command (%ls, %d) called", bstrCmd, nRecord);
    
    // security check must pass before we could exec anything. 
    if (!_fSecure)
    {
        TraceMsg(TF_CTL, "(Ctl) Security blocked Exec call");
        return S_OK;        // scripting methods cannot return failure
    }

    // We should always get passed a legal qualifier
    DWORD dwEnum = _GetEnumAreaFromQualifier(bstrQualifier);
    RIP(dwEnum != ENUM_UNKNOWN);
    if (dwEnum == ENUM_UNKNOWN)
        return S_OK;
    
    const static struct {
        LPCWSTR pszCmd;
        APPCMD  appcmd;
    } s_rgCmds[] = {
        { L"install",           APPCMD_INSTALL },
        { L"uninstall",         APPCMD_UNINSTALL },
        { L"modify",            APPCMD_MODIFY },
        { L"upgrade",           APPCMD_UPGRADE },
        { L"repair",            APPCMD_REPAIR },
        { L"generic install",   APPCMD_GENERICINSTALL },
        { L"ntoptions",         APPCMD_NTOPTIONS },
        { L"winupdate",         APPCMD_WINUPDATE },
        { L"addlater",          APPCMD_ADDLATER },
    };

    int i;
    APPCMD appcmd = APPCMD_UNKNOWN;

    for (i = 0; i < ARRAYSIZE(s_rgCmds); i++)
    {
        if (0 == StrCmpIW(bstrCmd, s_rgCmds[i].pszCmd))
        {
            appcmd = s_rgCmds[i].appcmd;
            break;
        }
    }

    HWND hwndStub = NULL;

    if (_hwndTB == NULL)
        _GetToplevelHWND();
    
    if (_hwndTB)
    {
        hwndStub = _CreateTransparentStubWindow(_hwndTB);
        ::EnableWindow(_hwndTB, FALSE);
        ::SetActiveWindow(hwndStub);
    }        

    switch (appcmd)
    {
        case APPCMD_INSTALL:
        case APPCMD_UNINSTALL:
        case APPCMD_MODIFY:
        case APPCMD_UPGRADE:
        case APPCMD_REPAIR:
        case APPCMD_ADDLATER:
            if (_rgparposp[dwEnum])
                _rgparposp[dwEnum]->DoCommand(hwndStub, appcmd, nRecord);
            break;
                    
        case APPCMD_GENERICINSTALL:
            InstallAppFromFloppyOrCDROM(_hwndTB);
            break;

        case APPCMD_NTOPTIONS:
        {
            // command to invoke and OCMgr
            // "sysocmgr /x /i:%systemroot%\system32\sysoc.inf"
            TCHAR szInf[MAX_PATH];
            if (GetSystemDirectory(szInf, ARRAYSIZE(szInf)) && PathCombine(szInf, szInf, TEXT("sysoc.inf")))
            {
                TCHAR szParam[MAX_PATH];
                wsprintf(szParam, TEXT("/i:%s"), szInf);
                ShellExecute(NULL, NULL, TEXT("sysocmgr"), szParam, NULL, SW_SHOWDEFAULT);
            }
        }
        break;

        case APPCMD_WINUPDATE:
        {
            TCHAR szUrl[512];

            if (0 < LoadString(g_hinst, IDS_WINUPD_URL, szUrl, SIZECHARS(szUrl)))
            {
                ShellExecute(NULL, NULL, TEXT("wupdmgr.exe"), szUrl, NULL, SW_SHOWDEFAULT);
            }
        }
            break;

        case APPCMD_UNKNOWN:
            TraceMsg(TF_ERROR, "(Ctl) Received invalid appcmd %ls", bstrCmd);
            break;

        default:
            ASSERTMSG(0, "You forgot to add %d to the command list above", appcmd);
            break;
    }

    if (_hwndTB)
    {
        ::EnableWindow(_hwndTB, TRUE);
        if (hwndStub)
            ::DestroyWindow(hwndStub);
        ::SetForegroundWindow(_hwndTB);
    }
    return S_OK;
}


/*-------------------------------------------------------------------------
Purpose: Provide an interface to the policies stored in the registry
         to the poor scripting languages

         This method can be called via script.
         
*/
STDMETHODIMP CADCCtl::IsRestricted(BSTR bstrPolicy, VARIANT_BOOL * pbRestricted)
{
    RIP(pbRestricted);
    
    *pbRestricted = ARPGetRestricted(bstrPolicy) ? VARIANT_TRUE : VARIANT_FALSE;

    return S_OK;
}

/*-------------------------------------------------------------------------
Purpose: Reset the control's filter and sort criteria

         This method can be called via script.
         
*/
STDMETHODIMP CADCCtl::Reset(BSTR bstrQualifier)
{
    HRESULT hres = E_FAIL;
    DWORD dwEnum = _GetEnumAreaFromQualifier(bstrQualifier);

    // We should always get a legal qualifier
    RIP(dwEnum != ENUM_UNKNOWN);
    if (dwEnum == ENUM_UNKNOWN)
        return S_OK;
    
    // security check must pass before we could exec anything. 
    if (!_fSecure)
    {
        TraceMsg(TF_CTL, "(Ctl) Security blocked Reset call");
        return S_OK;        // scripting methods cannot return failure
    }

    TraceMsg(TF_CTL, "(Ctl) Reset called");

    // Infinite recursive calls to Reset can occur if script code calls reset
    // from within the datasetchanged event.  This isn't a good idea.
    if ( !_fInReset )
    {
        _fInReset = TRUE;   // prevent reentrancy

        // Did the EnumArea change OR
        // did the category change for these published apps?
        if (_fDirty || ((ENUM_PUBLISHED == dwEnum) && _fCategoryChanged))
        {
            // Yes; release the matrix object and recreate the event broker
            _ReleaseMatrixObject(dwEnum);
            
            // Make sure if we call Reset() right away now, we don't re-download
            // the data.
            _fDirty = FALSE;
            _fCategoryChanged = FALSE;

            // Create a new matrix object and read the new data into it
            hres = THR(_CreateMatrixObject(dwEnum, &_rgparposp[dwEnum]));
            if (SUCCEEDED(hres))
            {
                // Tell the OSP to enumerate the items.
                hres = THR(_rgparposp[dwEnum]->EnumerateItemsAsync());
                if (FAILED(hres))
                    ATOMICRELEASE(_rgparposp[dwEnum]);

             }
        }
        else if (_rgparposp[dwEnum])
        {
            // No; simply re-apply the sort and filter criteria
            hres = S_OK;

            // Did the sort criteria change since the last sort?
            if (S_OK == _rgparposp[dwEnum]->SetSortCriteria(_cbstrSort))
            {
                // Yes

                // Create a new matrix object and transfer the contents of the 
                // existing object to that.  We must do this because Trident's
                // databinding expects to get a different object from msDataSourceObject
                // when it queries for another dataset.
                IARPSimpleProvider * parposp;

                hres = THR(_CreateMatrixObject(dwEnum, &parposp));
                if (SUCCEEDED(hres))
                {
                    // Transferring the data is much faster than recreating it...
                    hres = THR(parposp->TransferData(_rgparposp[dwEnum]));
                    if (SUCCEEDED(hres))
                    {
                        // Release the old datasource and remember the new one
                        _ReleaseMatrixObject(dwEnum);
                        _rgparposp[dwEnum] = parposp;

                        // Now apply the sort on the new dataset
                        hres = _rgparposp[dwEnum]->Sort();
                    }
                }
            }

        }

        _fInReset = FALSE;
    }

    // reset should always return S_OK, otherwise we get scrip error.
    return S_OK;
}

HRESULT CADCCtl::_CheckSecurity(IOleClientSite * pClientSite)
{
    IOleContainer * poc;
    if (SUCCEEDED(pClientSite->GetContainer(&poc)))
    {
        IHTMLDocument2 * phd;
        if (SUCCEEDED(poc->QueryInterface(IID_IHTMLDocument2, (void **)&phd)))
        {
            IHTMLWindow2 * phw;
            if (SUCCEEDED(phd->get_parentWindow(&phw)))
            {
                IHTMLWindow2 * phwTop;
                if (SUCCEEDED(phw->get_top(&phwTop)))
                {
                    IHTMLLocation * phl;
                    if (SUCCEEDED(phwTop->get_location(&phl)))
                    {
                        BSTR bstrHref;
                        if (SUCCEEDED(phl->get_href(&bstrHref)))
                        {
                            ASSERT(IS_VALID_STRING_PTRW(bstrHref, -1));
                            WCHAR szResURL[] = L"res://appwiz.cpl/default.hta";
                            DWORD cchUrl = lstrlen(szResURL);
                            if (!StrCmpNIW(bstrHref, szResURL, cchUrl))
                            {
                                _fSecure = TRUE;
                            }

                            SysFreeString(bstrHref);   
                        }
                        phl->Release();
                    }
                    phwTop->Release();
                }
                phw->Release();
            }

            phd->Release();
        }
        poc->Release();
    }

    return S_OK;
}


/*-------------------------------------------------------------------------
Purpose: IOleObject::SetClientSite
   For security reasons, on top of ATL's implementation of this, we need to
   check our top level browser's URL location, it must be our official URL
   namely "res://appwiz.cpl/frm_*.htm"
*/
STDMETHODIMP CADCCtl::IOleObject_SetClientSite(IOleClientSite *pClientSite)
{
    HRESULT hres = S_OK;
    
    // Has the site already been set?
    if (pClientSite != _pclientsite)
    {
        ATOMICRELEASE(_pclientsite);
        if (pClientSite)
        {
            // No; check some things out and cache them

            _hwndTB = NULL;

            _pclientsite = pClientSite;
            if (_pclientsite)
                _pclientsite->AddRef();

            _fSecure = FALSE;
            if (g_dwPrototype & PF_NOSECURITYCHECK)
            {
                _fSecure = TRUE;
            }
            else
                _CheckSecurity(_pclientsite);
        }

        hres = CComControlBase::IOleObject_SetClientSite(pClientSite);
    }


    return hres;
}

#endif //DOWNLEVEL_PLATFORM
