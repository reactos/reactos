//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       ext.cxx
//
//  Contents:   Shell extension handler for network objects
//
//  Classes:    CNetObj
//
//  History:    26-Sep-95 BruceFo  Created
//
//--------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop

#include <pages.hxx>

#define DONT_WANT_SHELLDEBUG
#include <shsemip.h>

#include "resource.h"
#include "ext.hxx"
#include "util.hxx"

//--------------------------------------------------------------------------
//
// Shell DLL communication
//

typedef
UINT
(WINAPI *SHELLGETNETRESOURCE)(
    HNRES hnres,
    UINT iItem,
    LPNETRESOURCE pnresOut,
    UINT cbMax
    );

HINSTANCE g_hShellLibrary = NULL;
SHELLGETNETRESOURCE g_pFuncGNR = NULL;
UINT g_cfNetResource = 0;

BOOL LoadShellDllEntries(VOID);

//--------------------------------------------------------------------------


/*
 * Helper functions used to communicate with shell32.dll
 */

BOOL LoadShellDllEntries(VOID)
{
    if (g_hShellLibrary)
    {
        return TRUE;
    }

    g_hShellLibrary = LoadLibrary(TEXT("shell32.dll"));
    if (NULL == g_hShellLibrary)
    {
        return FALSE;
    }

    g_pFuncGNR = (SHELLGETNETRESOURCE)GetProcAddress(g_hShellLibrary, (LPSTR)(MAKELONG(SHGetNetResourceORD, 0)));
    if (NULL == g_pFuncGNR)
    {
        return FALSE;
    }

    return TRUE;
}

//+-------------------------------------------------------------------------
//
//  Member:     CNetObj::CNetObj
//
//  Synopsis:   Constructor
//
//  History:    4-Apr-95    BruceFo  Created
//
//--------------------------------------------------------------------------

CNetObj::CNetObj(
    VOID
    )
    :
    _uRefs(0),
    _pDataObject(NULL),
    _hkeyProgID(NULL)
{
    INIT_SIG(CNetObj);

    AddRef(); // give it the correct initial reference count. add to the DLL reference count
}


//+-------------------------------------------------------------------------
//
//  Member:     CNetObj::~CNetObj
//
//  Synopsis:   Destructor
//
//  History:    4-Apr-95    BruceFo  Created
//
//--------------------------------------------------------------------------

CNetObj::~CNetObj()
{
    CHECK_SIG(CNetObj);

    if (_pDataObject)
    {
        _pDataObject->Release();
        _pDataObject = NULL;
    }

    if (_hkeyProgID)
    {
        LONG l = RegCloseKey(_hkeyProgID);
        if (l != ERROR_SUCCESS)
        {
            appDebugOut((DEB_ERROR, "CNetObj::destructor. Error closing registry key, 0x%08lx\n", l));
        }
        _hkeyProgID = NULL;
    }
}


//+-------------------------------------------------------------------------
//
//  Member:     CNetObj::Initialize
//
//  Derivation: IShellExtInit
//
//  Synopsis:   Initialize the shell extension. Stashes away the argument data.
//
//  History:    4-Apr-95    BruceFo  Created
//
//  Notes:      This method can be called more than once.
//
//--------------------------------------------------------------------------

STDMETHODIMP
CNetObj::Initialize(
    LPCITEMIDLIST   pidlFolder,
    LPDATAOBJECT    pDataObject,
    HKEY            hkeyProgID
    )
{
    CHECK_SIG(CNetObj);

    CNetObj::~CNetObj();

    if (!LoadShellDllEntries())
    {
        return E_FAIL;
    }

    // Duplicate the pDataObject pointer
    _pDataObject = pDataObject;
    if (pDataObject)
    {
        pDataObject->AddRef();
    }

    // Duplicate the handle
    if (hkeyProgID)
    {
        LONG l = RegOpenKeyEx(hkeyProgID, NULL, 0L, MAXIMUM_ALLOWED, &_hkeyProgID);
        if (l != ERROR_SUCCESS)
        {
            appDebugOut((DEB_ERROR, "CNetObj::Initialize. Error duplicating registry key, 0x%08lx\n", l));
        }
    }

    return S_OK;
}


//+-------------------------------------------------------------------------
//
//  Member:     CNetObj::AddPages
//
//  Derivation: IShellPropSheetExt
//
//  Synopsis:   (from shlobj.h)
//              "The explorer calls this member function when it finds a
//              registered property sheet extension for a particular type
//              of object. For each additional page, the extension creates
//              a page object by calling CreatePropertySheetPage API and
//              calls lpfnAddPage.
//
//  Arguments:  lpfnAddPage -- Specifies the callback function.
//              lParam -- Specifies the opaque handle to be passed to the
//                        callback function.
//
//  History:    4-Apr-95    BruceFo  Created
//
//--------------------------------------------------------------------------

STDMETHODIMP
CNetObj::AddPages(
    LPFNADDPROPSHEETPAGE lpfnAddPage,
    LPARAM               lParam
    )
{
    CHECK_SIG(CNetObj);

    //
    // Call IDataObject::GetData asking for a g_cfNetResource (i.e., HNRES).
    //
    STGMEDIUM medium;
    FORMATETC fmte =
    {
        g_cfNetResource
            ? g_cfNetResource
            : (g_cfNetResource = RegisterClipboardFormat(CFSTR_NETRESOURCES)),
        NULL,
        DVASPECT_CONTENT,
        -1,
        TYMED_HGLOBAL
    };
    appAssert(NULL != _pDataObject);
    HRESULT hr = _pDataObject->GetData(&fmte, &medium);
    CHECK_HRESULT(hr);
    if (FAILED(hr))
    {
        return hr;
    }

    ///////////////// Now I have a 'medium' to release

    hr = S_OK;

    HNRES hnres = medium.hGlobal;

    // Get number of selected items
    if (NULL != g_pFuncGNR)
    {
        UINT cItems = (*g_pFuncGNR)(hnres, (UINT)-1, NULL, 0);
        if (cItems > 0)
        {
            // Retrieve NETRESOURCE object from clipboard
            LPNETRESOURCE pNetRes = (LPNETRESOURCE)_bufNetResource;
            UINT ret = (*g_pFuncGNR)(hnres, 0, pNetRes, MAX_ONE_RESOURCE); // Get first item
            if (ret == 0)
            {
                // bad hnres?
                appDebugOut((DEB_TRACE, "CNetObj::AddPages. SHGetNetResource returned 0\n"));
                // BUGBUG this is really error
            }
            else if (ret > MAX_ONE_RESOURCE)
            {
                // BUGBUG: Resize the buf and try again
                appDebugOut((DEB_TRACE, "CNetObj::AddPages. buffer too small, needs to be %d\n", ret));
            }
            else
            {
                LPTSTR pszTemplate = NULL;

                if (RESOURCEDISPLAYTYPE_NETWORK == pNetRes->dwDisplayType)
                {
                    pszTemplate = MAKEINTRESOURCE(IDD_NETWORK_SUMMARYINFO);
                }
                else if (RESOURCEDISPLAYTYPE_DOMAIN == pNetRes->dwDisplayType)
                {
                    pszTemplate = MAKEINTRESOURCE(IDD_WRKGRP_SUMMARYINFO);
                }
                else if (RESOURCEDISPLAYTYPE_SERVER == pNetRes->dwDisplayType)
                {
                    pszTemplate = MAKEINTRESOURCE(IDD_SERVER_SUMMARYINFO);
                }
                else if (RESOURCEDISPLAYTYPE_SHARE == pNetRes->dwDisplayType)
                {
                    pszTemplate = MAKEINTRESOURCE(IDD_SHARE_SUMMARYINFO);
                }
                else
                {
                    appDebugOut((DEB_TRACE, "CNetObj::AddPages. Unknown net resource type!\n"));
                }

                //
                //  Create a property sheet page object from a dialog box.
                //

                if (NULL != pszTemplate)
                {
                    hr = FillAndAddPage(
                            lpfnAddPage,
                            lParam,
                            pszTemplate);
                }
            }
        }
        else
        {
            appDebugOut((DEB_TRACE, "CNetObj::AddPages. NO net resources!\n"));
            // BUGBUG this is really error
        }
    }
    else
    {
        appDebugOut((DEB_TRACE, "CNetObj::AddPages. No SHGetNetResource function!\n"));
        // BUGBUG this is really error
    }

    ReleaseStgMedium(&medium);
    return hr;
}

HRESULT
CNetObj::FillAndAddPage(
    LPFNADDPROPSHEETPAGE lpfnAddPage,
    LPARAM lParam,
    LPTSTR pszTemplate
    )
{
    PROPSHEETPAGE psp;
    HPROPSHEETPAGE hpage;

    CPage* pPage = new CPage(this);
    if (NULL == pPage)
    {
        return E_OUTOFMEMORY;
    }

    HRESULT hr = pPage->InitInstance();
    if (FAILED(hr))
    {
        delete pPage;
        return E_OUTOFMEMORY;
    }

    psp.dwSize      = sizeof(psp);  // no extra data.
    psp.dwFlags     = PSP_USEREFPARENT | PSP_USECALLBACK;
    psp.hInstance   = g_hInstance;
    psp.pszTemplate = pszTemplate;
    psp.hIcon       = NULL;
    psp.pszTitle    = NULL;
    psp.pfnDlgProc  = CPage::DlgProcPage;
    psp.pfnCallback = CPage::PageCallback;
    psp.pcRefParent = &g_NonOLEDLLRefs;
    psp.lParam      = (LPARAM)pPage;

    hpage = CreatePropertySheetPage(&psp);
    if (NULL == hpage)
    {
        delete pPage;
        return E_OUTOFMEMORY;
    }

    BOOL fAdded = (*lpfnAddPage)(hpage, lParam);
    if (!fAdded)
    {
        DestroyPropertySheetPage(hpage);
        return E_OUTOFMEMORY;
    }

    return S_OK;
}


//+-------------------------------------------------------------------------
//
//  Member:     CNetObj::ReplacePages
//
//  Derivation: IShellPropSheetExt
//
//  Synopsis:   (From shlobj.h)
//              "The explorer never calls this member of property sheet
//              extensions. The explorer calls this member of control panel
//              extensions, so that they can replace some of default control
//              panel pages (such as a page of mouse control panel)."
//
//  Arguments:  uPageID -- Specifies the page to be replaced.
//              lpfnReplace -- Specifies the callback function.
//              lParam -- Specifies the opaque handle to be passed to the
//                        callback function.
//
//  History:    4-Apr-95    BruceFo  Created
//
//--------------------------------------------------------------------------

STDMETHODIMP
CNetObj::ReplacePage(
    UINT                 uPageID,
    LPFNADDPROPSHEETPAGE lpfnReplaceWith,
    LPARAM               lParam
    )
{
    CHECK_SIG(CNetObj);

    appAssert(!"CNetObj::ReplacePage called, not implemented");
    return E_NOTIMPL;
}


// dummy function to export to get linking to work
HRESULT PropDummyFunction()
{
    return S_OK;
}
