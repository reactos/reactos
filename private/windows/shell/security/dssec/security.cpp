//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       security.cpp
//
//  Invoke the security UI for DS objects
//
//--------------------------------------------------------------------------

#include "pch.h"
#include <dssec.h>


/*-----------------------------------------------------------------------------
/ CDsSecurityClassFactory
/   Class factory for the Security property page and context menu
/----------------------------------------------------------------------------*/

#undef CLASS_NAME
#define CLASS_NAME CDsSecurityClassFactory
#include <unknown.inc>

STDMETHODIMP
CDsSecurityClassFactory::QueryInterface(REFIID riid, LPVOID* ppvObject)
{
    INTERFACES iface[] =
    {
        &IID_IClassFactory, (LPCLASSFACTORY)this,
    };

    return HandleQueryInterface(riid, ppvObject, iface, ARRAYSIZE(iface));
}


/*-----------------------------------------------------------------------------
/ IClassFactory methods
/----------------------------------------------------------------------------*/

STDMETHODIMP
CDsSecurityClassFactory::CreateInstance(LPUNKNOWN punkOuter,
                                        REFIID riid,
                                        LPVOID* ppvObject)
{
    HRESULT hr;
    CDsSecurity* pDsSecurity;

    TraceEnter(TRACE_SECURITY, "CDsSecurityClassFactory::CreateInstance");
    TraceGUID("Interface requested", riid);

    TraceAssert(ppvObject);

    if (punkOuter)
        ExitGracefully(hr, CLASS_E_NOAGGREGATION, "Aggregation is not supported");

    pDsSecurity = new CDsSecurity;

    if (!pDsSecurity)
        ExitGracefully(hr, E_OUTOFMEMORY, "Failed to allocate CDsSecurity");

    hr = pDsSecurity->QueryInterface(riid, ppvObject);

    if (FAILED(hr))
        delete pDsSecurity;

exit_gracefully:

    TraceLeaveResult(hr);
}

/*---------------------------------------------------------------------------*/

STDMETHODIMP
CDsSecurityClassFactory::LockServer(BOOL /*fLock*/)
{
    return E_NOTIMPL;               // not supported
}


/*-----------------------------------------------------------------------------
/ CDsSecurity
/   Security property page and context menu shell extension
/----------------------------------------------------------------------------*/

// Destructor

CDsSecurity::~CDsSecurity()
{
    DoRelease(m_pSI);
}

// IUnknown bits

#undef CLASS_NAME
#define CLASS_NAME CDsSecurity
#include "unknown.inc"

STDMETHODIMP
CDsSecurity::QueryInterface(REFIID riid, LPVOID* ppvObject)
{
    INTERFACES iface[] =
    {
        &IID_IShellExtInit, (LPSHELLEXTINIT)this,
        &IID_IShellPropSheetExt, (LPSHELLPROPSHEETEXT)this,
        &IID_IContextMenu, (LPCONTEXTMENU)this,
    };

    return HandleQueryInterface(riid, ppvObject, iface, ARRAYSIZE(iface));
}


/*----------------------------------------------------------------------------
/ IShellExtInit
/----------------------------------------------------------------------------*/

STDMETHODIMP
CDsSecurity::Initialize(LPCITEMIDLIST /*pIDFolder*/,
                        LPDATAOBJECT pDataObj,
                        HKEY /*hKeyID*/)
{
    HRESULT hr;
    FORMATETC fe = {0, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    STGMEDIUM medObjectNames = {0};
    STGMEDIUM medDisplayOptions = {0};
    LPDSOBJECTNAMES pDsObjects;
    LPWSTR pObjectPath;
    LPWSTR pClass = NULL;
    DWORD dwFlags = 0;
    LPWSTR pszServer = NULL;
    LPWSTR pszUserName = NULL;
    LPWSTR pszPassword = NULL;

    static CLIPFORMAT cfDsObjectNames = 0;
    static CLIPFORMAT cfDsDisplayOptions = 0;

    TraceEnter(TRACE_SECURITY, "CDsSecurity::Initialize");

    DoRelease(m_pSI);

    // Call the data object to get the array of DS names and classes.  This
    // is stored using a private clipboard format - so we must first
    // try and register it.

    if (!cfDsObjectNames)
        cfDsObjectNames = (CLIPFORMAT)RegisterClipboardFormat(CFSTR_DSOBJECTNAMES);

    if (!cfDsObjectNames)
        ExitGracefully(hr, E_FAIL, "Clipboard format failed to register");

    fe.cfFormat = cfDsObjectNames;            // set the clipboard format

    if (!pDataObj)
        ExitGracefully(hr, E_INVALIDARG, "No data object given");

    hr = pDataObj->GetData(&fe, &medObjectNames);
    FailGracefully(hr, "Failed to get selected objects");

    pDsObjects = (LPDSOBJECTNAMES)medObjectNames.hGlobal;
    TraceAssert(pDsObjects);

    if (!(pDsObjects->aObjects[0].dwProviderFlags & DSPROVIDER_ADVANCED))
        ExitGracefully(hr, E_FAIL, "Security page only shown in advanced mode");

    if (1 != pDsObjects->cItems)
        ExitGracefully(hr, E_FAIL, "Multiple selection not supported");

    // Get the object path
    pObjectPath = (LPWSTR)ByteOffset(pDsObjects, pDsObjects->aObjects[0].offsetName);
    Trace((TEXT("Name \"%s\""), pObjectPath));

    // Get the class name
    if (pDsObjects->aObjects[0].offsetClass)
    {
        pClass = (LPWSTR)ByteOffset(pDsObjects, pDsObjects->aObjects[0].offsetClass);
        Trace((TEXT("Class \"%s\""), pClass));
    }

// DSOBJECT_READONLYPAGES is no longer used
#ifdef DSOBJECT_READONLYPAGES
    if (pDsObjects->aObjects[0].dwFlags & DSOBJECT_READONLYPAGES)
        dwFlags = DSSI_READ_ONLY;
#endif

    //
    // Get server name and user credentials from the data object
    //
    if (!cfDsDisplayOptions)
        cfDsDisplayOptions = (CLIPFORMAT)RegisterClipboardFormat(CFSTR_DS_DISPLAY_SPEC_OPTIONS);
    if (cfDsDisplayOptions)
    {
        fe.cfFormat = cfDsDisplayOptions;
        hr = pDataObj->GetData(&fe, &medDisplayOptions);
        if (SUCCEEDED(hr))
        {
            LPDSDISPLAYSPECOPTIONS pDisplayOptions = (LPDSDISPLAYSPECOPTIONS)medDisplayOptions.hGlobal;

            if (pDisplayOptions->dwFlags & DSDSOF_HASUSERANDSERVERINFO)
            {
                if (pDisplayOptions->offsetServer)
                    pszServer = (LPWSTR)ByteOffset(pDisplayOptions, pDisplayOptions->offsetServer);
                if (pDisplayOptions->offsetUserName)
                    pszUserName = (LPWSTR)ByteOffset(pDisplayOptions, pDisplayOptions->offsetUserName);
                if (pDisplayOptions->offsetPassword)
                    pszPassword = (LPWSTR)ByteOffset(pDisplayOptions, pDisplayOptions->offsetPassword);

                Trace((TEXT("Display Options: server = %s; user = %s; pw = %s"),
                      pszServer?pszServer:TEXT("none"),
                      pszUserName?pszUserName:TEXT("none"),
                      pszPassword?pszPassword:TEXT("none")));
            }
        }
    }

    //
    // Create and initialize the ISecurityInformation object.
    //
    hr = DSCreateISecurityInfoObjectEx(pObjectPath,
                                       pClass,
                                       pszServer,
                                       pszUserName,
                                       pszPassword,
                                       dwFlags,
                                       &m_pSI,
                                       NULL,
                                       NULL,
                                       0);

exit_gracefully:

    ReleaseStgMedium(&medDisplayOptions);
    ReleaseStgMedium(&medObjectNames);

    TraceLeaveResult(hr);
}


/*----------------------------------------------------------------------------
/ IShellPropSheetExt
/----------------------------------------------------------------------------*/

STDMETHODIMP
CDsSecurity::AddPages(LPFNADDPROPSHEETPAGE lpfnAddPage,
                      LPARAM               lParam)
{
    HRESULT hr = E_FAIL;

    TraceEnter(TRACE_SECURITY, "CDsSecurity::AddPages");

    if (m_pSI != NULL)
    {
        HPROPSHEETPAGE hPermPage = NULL;

        hr = _CreateSecurityPage(m_pSI, &hPermPage);

        if (NULL != hPermPage && !lpfnAddPage(hPermPage, lParam))
            DestroyPropertySheetPage(hPermPage);
    }

    TraceLeaveResult(hr);
}

/*---------------------------------------------------------------------------*/

STDMETHODIMP
CDsSecurity::ReplacePage(UINT                 /* uPageID */,
                         LPFNADDPROPSHEETPAGE /* lpfnReplaceWith */,
                         LPARAM               /* lParam */)
{
    return E_NOTIMPL;
}


/*----------------------------------------------------------------------------
/ IContextMenu
/----------------------------------------------------------------------------*/

STDMETHODIMP
CDsSecurity::QueryContextMenu(HMENU hMenu,
                              UINT indexMenu,
                              UINT idCmdFirst,
                              UINT /*idCmdLast*/,
                              UINT /*uFlags*/)
{
    TCHAR szBuffer[MAX_PATH];
    MENUITEMINFO mii;
    int idMax = idCmdFirst;

    TraceEnter(TRACE_SECURITY, "CDsSecurity::QueryContextMenu");

    mii.cbSize = sizeof(mii);
    mii.fMask = MIIM_TYPE | MIIM_ID;
    mii.fType = MFT_STRING;
    mii.dwTypeData = szBuffer;
    mii.cch = lstrlen(szBuffer);

    // Merge our verbs into the menu we were given

    LoadString(GLOBAL_HINSTANCE, IDS_SECURITY, szBuffer, ARRAYSIZE(szBuffer));

    mii.wID = idMax++;

    InsertMenuItem(hMenu,
                   indexMenu++,
                   TRUE /*fByPosition*/,
                   &mii);

    TraceLeaveValue(ResultFromShort(idMax - idCmdFirst));
}

/*---------------------------------------------------------------------------*/

STDMETHODIMP
CDsSecurity::InvokeCommand(LPCMINVOKECOMMANDINFO lpcmi)
{
    HRESULT hr = E_FAIL;

    TraceEnter(TRACE_SECURITY, "CDsSecurity::InvokeCommand");

    if (HIWORD(lpcmi->lpVerb) != 0)
        TraceLeaveResult(E_NOTIMPL);

    TraceAssert(LOWORD(lpcmi->lpVerb) == 0);

    // REVIEW If they ever get around to making property pages work
    // for DS objects, we can replace this with
    // ShellExecuteEx(verb=Properties, parameters=Security) [see ..\rshx32\rshx32.cpp]
    if (m_pSI != NULL)
        hr = _EditSecurity(lpcmi->hwnd, m_pSI);

    TraceLeaveResult(hr);
}

/*---------------------------------------------------------------------------*/

STDMETHODIMP
CDsSecurity::GetCommandString(UINT_PTR /*idCmd*/,
                              UINT uFlags,
                              LPUINT /*reserved*/,
                              LPSTR pszName,
                              UINT ccMax)
{
    if (uFlags == GCS_HELPTEXT)
    {
        LoadString(GLOBAL_HINSTANCE, IDS_SECURITYHELP, (LPTSTR)pszName, ccMax);
        return S_OK;
    }

    return E_NOTIMPL;
}
