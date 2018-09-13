#include "pch.h"
#pragma hdrstop

#ifndef WINNT
#include <msprintx.h>       // Win9x printing support.
#endif


/*----------------------------------------------------------------------------
/ Static data for mapping verbs to intersting information
/----------------------------------------------------------------------------*/

// 
// Menu item stored in the DSA to map from external IDs to internal
//

typedef struct
{
    INT    iMenuItem;               // index into menu_items array
} MENUITEM, * LPMENUITEM;

// 
// This table maps classes to verbs that should be added to the menu
// we then add menu item data structures as required.
//

#define MENUCMD_INITITEM      0x0001    // called per menu item
#define MENUCMD_INVOKE        0x0002    // called to invoke the command

//
// Handlers
//

typedef struct
{
    DWORD dwFlags;
    HDPA hdpaSelection;
    LPWSTR pszUserName;
    LPWSTR pszPassword;
} VERBINFO, * LPVERBINFO;
                                                                               
typedef HRESULT (*LPMENUITEMCB)(UINT uCmd, HWND hWnd, HMENU hMenu, LPARAM uID, LPVERBINFO pvi, UINT uFlags);

HRESULT _UserVerbCB(UINT uCmd, HWND hWnd, HMENU hMenu, LPARAM uID, LPVERBINFO pvi, UINT uFlags);
HRESULT _VolumeVerbCB(UINT uCmd, HWND hWnd, HMENU hMenu, LPARAM uID, LPVERBINFO pvi, UINT uFlags);
HRESULT _ComputerVerbCB(UINT uCmd, HWND hWnd, HMENU hMenu, LPARAM uID, LPVERBINFO pvi, UINT uFlags);
HRESULT _PrinterVerbCB(UINT uCmd, HWND hWnd, HMENU hMenu, LPARAM uID, LPVERBINFO pvi, UINT uFlags);

struct
{
    BOOL fNotValidInWAB:1;          // =1 => verb is NOT valid when invoked from WAB
    LPWSTR pObjectClass;            // class name
    UINT uID;                       // name to add for verb   
    UINT idsHelp;                   // help text for this verb
    LPMENUITEMCB pItemCB;           // menu item callback
}   
menu_items[] =
{
    0, L"user",        IDC_USER_OPENHOMEPAGE,      IDS_USER_OPENHOMEPAGE,  _UserVerbCB,
    1, L"user",        IDC_USER_MAILTO,            IDS_USER_MAILTO,        _UserVerbCB,
    0, L"contact",     IDC_USER_OPENHOMEPAGE,      IDS_USER_OPENHOMEPAGE,  _UserVerbCB, 
    1, L"contact",     IDC_USER_MAILTO,            IDS_USER_MAILTO,        _UserVerbCB,
    1, L"group",       IDC_USER_MAILTO,            IDS_USER_MAILTO,        _UserVerbCB, 
    0, L"volume",      IDC_VOLUME_OPEN,            IDS_VOLUME_OPEN,        _VolumeVerbCB,
    0, L"volume",      IDC_VOLUME_EXPLORE,         IDS_VOLUME_EXPLORE,     _VolumeVerbCB,
    0, L"volume",      IDC_VOLUME_FIND,            IDS_VOLUME_FIND,        _VolumeVerbCB, 
    0, L"volume",      IDC_VOLUME_MAPNETDRIVE,     IDS_VOLUME_MAPNETDRIVE, _VolumeVerbCB,
    0, L"computer",    IDC_COMPUTER_MANAGE,        IDS_COMPUTER_MANAGE,    _ComputerVerbCB,
    0, L"printQueue",  IDC_PRINTER_INSTALL,        IDS_PRINTER_INSTALL,    _PrinterVerbCB,
#ifdef WINNT
    0, L"printQueue",  IDC_PRINTER_OPEN,           IDS_PRINTER_OPEN,       _PrinterVerbCB,
#endif
};

//
// Our class for implementing the standard verbs
// 

class CDsVerbs : public IShellExtInit, IContextMenu, CUnknown
{
    private:
        IDataObject* _pDataObject;
        HDSA _hdsaItems;               // entry per verb on menu
        VERBINFO _vi;

    // 
    // This public data is used by the verb handlers, they are passed a CDsVerbs*
    // as one of their parameters, so using this we then allow them to store what 
    // they need in here.
    //

    public:
        CDsVerbs();
        ~CDsVerbs();

        // IUnknown members
        STDMETHODIMP_(ULONG) AddRef();
        STDMETHODIMP_(ULONG) Release();
        STDMETHODIMP QueryInterface( REFIID, LPVOID FAR* );

        // IShellExtInit
        STDMETHODIMP Initialize(LPCITEMIDLIST pIDFolder, LPDATAOBJECT pDataObj, HKEY hKeyID );

        // IContextMenu
        STDMETHODIMP QueryContextMenu(HMENU hMenu, UINT uIndex, UINT uIDFirst, UINT uIDLast, UINT uFlags);
        STDMETHODIMP InvokeCommand(LPCMINVOKECOMMANDINFO pCMI);
        STDMETHODIMP GetCommandString(UINT_PTR uID, UINT uFlags, UINT FAR* reserved, LPSTR pName, UINT ccMax);

    private:
        VOID FreeMenuStateData(VOID);
};


/*----------------------------------------------------------------------------
/ SHStartNetConnectionDialog not supported on older shell32, therefore lets
/ implement it here and call the original SHNetConnectionDialog API that
/ the existing implementation does.
/----------------------------------------------------------------------------*/

#if DOWNLEVEL_SHELL

typedef struct
{
    HWND    hwnd;
    TCHAR   szRemoteName[MAX_PATH];
    BOOL    fRemoteName;
    DWORD   dwType;
    DWORD   dwThreadId;
} SHNETCONNECT;

DWORD CALLBACK _StartNetConnect(LPVOID ptd)
{
    SHNETCONNECT *pshnc = (SHNETCONNECT*)ptd;
    HWND    hwnd = pshnc->hwnd;
    DWORD   dwType = pshnc->dwType;
    BOOL    fRemoteName = pshnc->fRemoteName;
    TCHAR   szRemoteName[MAX_PATH];
    LPTSTR  lpszRemoteName = NULL;

    if (fRemoteName)
    {
        lstrcpy(szRemoteName,pshnc->szRemoteName);
        lpszRemoteName = szRemoteName;
    }

    LocalFree(pshnc);

    SHNetConnectionDialog(hwnd, lpszRemoteName, dwType);
    SHChangeNotifyHandleEvents();

    return 0;
}

STDAPI MySHStartNetConnectionDialog(HWND    hwnd, 
                                    LPCTSTR pszRemoteName,       OPTIONAL
                                    DWORD   dwType)
{
    DWORD dwThreadId;
    HANDLE  hThread;
    SHNETCONNECT *pshnc = (SHNETCONNECT *)LocalAlloc(LPTR, SIZEOF(SHNETCONNECT));
    if (!pshnc)
        return E_OUTOFMEMORY;

    pshnc->hwnd = hwnd;
    pshnc->dwType = dwType;
    pshnc->dwThreadId = GetCurrentThreadId();
    if (pszRemoteName)
    {
        pshnc->fRemoteName = TRUE;
        lstrcpyn(pshnc->szRemoteName, pszRemoteName, ARRAYSIZE(pshnc->szRemoteName));
    }
    else
        pshnc->fRemoteName = FALSE;

    hThread = CreateThread(NULL, 0, _StartNetConnect, pshnc, 0, &dwThreadId);

    if (hThread) {
        CloseHandle(hThread);
        return S_OK;
    } else {
        LocalFree((HLOCAL)pshnc);
        return E_UNEXPECTED;
    }
}

#ifdef SHStartNetConnectionDialog
#undef SHStartNetConnectionDialog
#endif

#define SHStartNetConnectionDialog MySHStartNetConnectionDialog

#endif // DOWNLEVEL_SHELL


/*----------------------------------------------------------------------------
/ CDsVerbs implementation
/----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
/ IUnknown
/----------------------------------------------------------------------------*/

CDsVerbs::CDsVerbs() :
    _pDataObject(NULL),
    _hdsaItems(NULL)
{
    _vi.dwFlags = 0;
    _vi.hdpaSelection = NULL;
    _vi.pszUserName = NULL;
    _vi.pszPassword = NULL;
}

CDsVerbs::~CDsVerbs()
{
    DoRelease(_pDataObject);

    FreeMenuStateData();

    LocalFreeStringW(&_vi.pszUserName);
    LocalFreeStringW(&_vi.pszPassword);
}

// IUnknown bits

#undef CLASS_NAME
#define CLASS_NAME CDsVerbs
#include "unknown.inc"

STDMETHODIMP CDsVerbs::QueryInterface(REFIID riid, LPVOID* ppvObject)
{
    INTERFACES iface[] =
    {
        &IID_IShellExtInit, (LPSHELLEXTINIT)this,
        &IID_IContextMenu, (LPCONTEXTMENU)this,
    };

    return HandleQueryInterface(riid, ppvObject, iface, ARRAYSIZE(iface));
}

//
// handle create instance
//

STDAPI CDsVerbs_CreateInstance(IUnknown* punkOuter, IUnknown** ppunk, LPCOBJECTINFO poi)
{
    CDsVerbs *pdv = new CDsVerbs();
    if ( !pdv )
        return E_OUTOFMEMORY;

    HRESULT hres = pdv->QueryInterface(IID_IUnknown, (void **)ppunk);
    pdv->Release();
    return hres;
}


/*----------------------------------------------------------------------------
/ IShellExtInit
/----------------------------------------------------------------------------*/

STDMETHODIMP CDsVerbs::Initialize(LPCITEMIDLIST pIDFolder, LPDATAOBJECT pDataObject, HKEY hKeyID)
{
    HRESULT hr;

    TraceEnter(TRACE_VERBS, "CDsVerbs::Initialize");

    // take a copy of the IDataObject if we are given one

    if ( !pDataObject )
        ExitGracefully(hr, E_FAIL, "No IDataObject to interact with");

    DoRelease(_pDataObject);

    _pDataObject = pDataObject;
    _pDataObject->AddRef();

    hr = S_OK;                          // sucess

exit_gracefully:

    TraceLeaveResult(hr);
}


/*----------------------------------------------------------------------------
/ IContextMenu
/----------------------------------------------------------------------------*/

STDMETHODIMP CDsVerbs::QueryContextMenu(HMENU hMenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags)
{
    HRESULT hr;
    FORMATETC fmte = {(CLIPFORMAT)0, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    STGMEDIUM mediumDsObjects = { TYMED_NULL };
    STGMEDIUM mediumDispSpecOptions = { TYMED_NULL };
    LPDSOBJECTNAMES pDsObjectNames;
    LPDSDISPLAYSPECOPTIONS pDispSpecOptions;
    MENUITEM item;
    INT i, iVerb;
    TCHAR szBuffer[MAX_PATH];
    BOOL fInWAB = FALSE;
    USES_CONVERSION;

    TraceEnter(TRACE_VERBS, "CDsVerbs::QueryContextMenu");

    FreeMenuStateData();

    // Get the selection from the IDataObject we have been given.  This structure
    // contains the object class, ADsPath and other information.

    if ( !_pDataObject )
        ExitGracefully(hr, E_FAIL, "No IDataObject to use");    

    fmte.cfFormat = g_cfDsObjectNames;    
    hr = _pDataObject->GetData(&fmte, &mediumDsObjects);
    FailGracefully(hr, "Failed to get the DSOBJECTNAMES from IDataObject");

    pDsObjectNames = (LPDSOBJECTNAMES)mediumDsObjects.hGlobal;
    TraceAssert(pDsObjectNames);

    fmte.cfFormat = g_cfDsDispSpecOptions;    
    if ( SUCCEEDED(_pDataObject->GetData(&fmte, &mediumDispSpecOptions)) )
    {
        pDispSpecOptions = (LPDSDISPLAYSPECOPTIONS)mediumDispSpecOptions.hGlobal;
        TraceAssert(pDispSpecOptions);

        TraceMsg("Retrieved the CF_DISPSPECOPTIONS from the IDataObject");

        fInWAB = (pDispSpecOptions->dwFlags & DSDSOF_INVOKEDFROMWAB) == DSDSOF_INVOKEDFROMWAB;
        Trace(TEXT("Invoked from WAB == %d"), fInWAB);

        // copy credential and other information for the verbs to invoke with

        _vi.dwFlags = pDispSpecOptions->dwFlags;

        if ( _vi.dwFlags & DSDSOF_HASUSERANDSERVERINFO )
        {
            TraceMsg("Copying user and credential information from clipboard block");

            if ( pDispSpecOptions->offsetUserName )
            {
                LPWSTR pszUserName = (LPWSTR)ByteOffset(pDispSpecOptions, pDispSpecOptions->offsetUserName);
                Trace(TEXT("pszUserName: %s"), W2T(pszUserName));

                hr = LocalAllocStringW(&_vi.pszUserName, pszUserName);
                FailGracefully(hr, "Failed to copy the user name");
            }

            if ( pDispSpecOptions->offsetPassword )
            {
                LPWSTR pszPassword = (LPWSTR)ByteOffset(pDispSpecOptions, pDispSpecOptions->offsetPassword);
                Trace(TEXT("pszPassword: %s"), W2T(pszPassword));

                hr = LocalAllocStringW(&_vi.pszPassword, pszPassword);
                FailGracefully(hr, "Failed to copy the password");
            }
        }
    }

    // Take the first item of the selection, compare all the objects in the
    // rest of the DSOBJECTNAMES, all those who have the same class.

    _hdsaItems = DSA_Create(SIZEOF(MENUITEM), 4);
    TraceAssert(_hdsaItems);

    _vi.hdpaSelection = DPA_Create(4);
    TraceAssert(_vi.hdpaSelection);

    if ( !_vi.hdpaSelection || !_hdsaItems )
        ExitGracefully(hr, E_OUTOFMEMORY, "Failed to allocate the selection DPA");
        
    for ( i = 0 ; i < (INT)pDsObjectNames->cItems ; i++ )
    {
        LPCWSTR pObjectClass0 = (LPCWSTR)ByteOffset(pDsObjectNames, pDsObjectNames->aObjects[0].offsetClass);
        LPWSTR pPath = (LPWSTR)ByteOffset(pDsObjectNames, pDsObjectNames->aObjects[i].offsetName);
        LPCWSTR pObjectClass = (LPCWSTR)ByteOffset(pDsObjectNames, pDsObjectNames->aObjects[i].offsetClass);

        Trace(TEXT("ADsPath of object %d is %s"), i, W2CT(pPath));
        Trace(TEXT("objectClass of object %d is %s"), i, W2CT(pObjectClass));

        if ( !StrCmpW(pObjectClass0, pObjectClass) )
        {
            Trace(TEXT("Adding item %d to the selection DPA"), i);

            hr = StringDPA_AppendStringW(_vi.hdpaSelection, pPath, NULL);
            FailGracefully(hr, "Failed to copy selection to selection DPA");
        }
    }

    // Walk the list of menu items, lets see which ones we need to add to the
    // menu.

    if ( DPA_GetPtrCount(_vi.hdpaSelection) )
    {
        LPCWSTR pObjectClass0 = (LPCWSTR)ByteOffset(pDsObjectNames, pDsObjectNames->aObjects[0].offsetClass);

        for ( i = 0 ; i < ARRAYSIZE(menu_items); i++ )
        {
            if ( menu_items[i].fNotValidInWAB && fInWAB )
            {
                TraceMsg("Skipping verb not valid for WAB");
                continue;
            }

            if ( !StrCmpW(pObjectClass0, menu_items[i].pObjectClass ) )  
            {
                Trace(TEXT("Adding the verb at index %d to the menu"), i);

                // now fill in the MENUITEM structure and add it to the DSA list,
                // then add the menu item itself, calling the callback so it can
                // enable/disable itself.

                item.iMenuItem = i;

                iVerb = DSA_AppendItem(_hdsaItems, &item);
                TraceAssert(iVerb != -1);

                if ( iVerb != -1 )
                {
                    Trace(TEXT("iVerb is %d"), iVerb);
    
                    LoadString(GLOBAL_HINSTANCE, menu_items[i].uID, szBuffer, ARRAYSIZE(szBuffer));
                    InsertMenu(hMenu, iVerb+indexMenu, MF_BYPOSITION|MF_STRING, iVerb+idCmdFirst, szBuffer);

                    menu_items[i].pItemCB(MENUCMD_INITITEM,
                                          NULL,
                                          hMenu, 
                                          MAKELPARAM(menu_items[i].uID, iVerb+idCmdFirst),
                                          &_vi,
                                          uFlags);
                }
            } 
        }
    }
   
    hr = S_OK;

exit_gracefully:

    if ( SUCCEEDED(hr) )
    {
        Trace(TEXT("%d items added by QueryContextMenu"), DSA_GetItemCount(_hdsaItems));
        hr = ResultFromShort(DSA_GetItemCount(_hdsaItems));
    }

    ReleaseStgMedium(&mediumDsObjects);
    ReleaseStgMedium(&mediumDispSpecOptions);

    TraceLeaveResult(hr);
}

/*---------------------------------------------------------------------------*/

STDMETHODIMP CDsVerbs::InvokeCommand(LPCMINVOKECOMMANDINFO lpcmi)
{
    HRESULT hr;
    UINT uID = LOWORD(lpcmi->lpVerb);
    LPMENUITEM pMenuItem;

    TraceEnter(TRACE_VERBS, "CDsVerbs::InvokeCommand");

    // Dreference the menu item to get the index's into both the item list and the
    // menu table.  With both of these we can then invoke the command.

    Trace(TEXT("uID %d (DSA contains %d)"), uID, DSA_GetItemCount(_hdsaItems));

    if ( !_hdsaItems )
        ExitGracefully(hr, E_UNEXPECTED, "No _hdasItems");

    pMenuItem = (LPMENUITEM)DSA_GetItemPtr(_hdsaItems, (UINT)uID);
    TraceAssert(pMenuItem);

    if ( !pMenuItem || !menu_items[pMenuItem->iMenuItem].pItemCB )
        ExitGracefully(hr, E_UNEXPECTED, "Failed because pItem == NULL");

    hr = menu_items[pMenuItem->iMenuItem].pItemCB(MENUCMD_INVOKE,
                                                  lpcmi->hwnd,
                                                  NULL, 
                                                  MAKELPARAM(menu_items[pMenuItem->iMenuItem].uID, 0),
                                                  &_vi,
                                                  0);
exit_gracefully:

    TraceLeaveResult(S_OK);
}

/*---------------------------------------------------------------------------*/

STDMETHODIMP CDsVerbs::GetCommandString(UINT_PTR uID, UINT uFlags, UINT FAR* reserved, LPSTR pszName, UINT ccMax)
{
    HRESULT hr = E_NOTIMPL;
    INT cc;

    TraceEnter(TRACE_VERBS, "CDsVerbs::GetCommandString");

    if ( _hdsaItems )
    {
        LPMENUITEM pMenuItem = (LPMENUITEM)DSA_GetItemPtr(_hdsaItems, (INT)uID);
        TraceAssert(pMenuItem);

        if ( !pMenuItem )
            ExitGracefully(hr, E_FAIL, "Failed to get menu item");

        if ( uFlags == GCS_HELPTEXT )
        {
            // Get the menu item and look up the resource for this verb
            // and return it into the callers buffer.
            
            if ( !LoadString(GLOBAL_HINSTANCE, menu_items[pMenuItem->iMenuItem].idsHelp, (LPTSTR)pszName, ccMax) ) 
                ExitGracefully(hr, E_FAIL, "Failed to load string for help text");
        }
        else
        {
            ExitGracefully(hr, E_FAIL, "Failed to get command string");
        }
    }

    hr = S_OK;

exit_gracefully:

    TraceLeaveResult(hr);
}


/*-----------------------------------------------------------------------------
/ CDsVerbs::FreeMenuStateData
/ ---------------------------
/   Release the verb state data for the CDsVerbs class, this can be called
/   (and is) during the destructor and during the context menu construction
/   to ensure a consistent state.
/
/ In:
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/

VOID CDsVerbs::FreeMenuStateData(VOID)
{
    TraceEnter(TRACE_VERBS, "CDsVerbs::FreeMenuStateData");

    if ( _hdsaItems )
    {
        DSA_Destroy(_hdsaItems);
        _hdsaItems = NULL;
    }

    StringDPA_Destroy(&_vi.hdpaSelection);
    LocalFreeStringW(&_vi.pszUserName);
    LocalFreeStringW(&_vi.pszPassword);

    TraceLeave();
}


/*----------------------------------------------------------------------------
/ User object verbs
/----------------------------------------------------------------------------*/

VOID _BuildMailToLine(LPTSTR pBuffer, UINT* pLen, HDPA hdpaMailTo)
{
    INT i;
    
    TraceEnter(TRACE_VERBS, "_BuildMailToLine");

    *pLen = 0;
    PutStringElement(pBuffer, pLen, TEXT("mailto:"));

    for ( i = 0 ; i < DPA_GetPtrCount(hdpaMailTo); i++ )
    {
        LPTSTR pMailTo = (LPTSTR)DPA_GetPtr(hdpaMailTo, i);
        TraceAssert(pMailTo);

        Trace(TEXT("Sending mail to:"), pMailTo);

        if ( i )
        {
            TraceMsg("Adding sepereator");
            PutStringElement(pBuffer, pLen, TEXT(";"));
        }

        PutStringElement(pBuffer, pLen, pMailTo);
    }

    TraceLeave();
}

HRESULT _UserVerbCB(UINT uCmd, HWND hWnd, HMENU hMenu, LPARAM uID, LPVERBINFO pvi, UINT uFlags)
{
    HRESULT hr;
    HDPA hdpaMailTo = NULL;
    LPTSTR pURL = NULL;
    LPTSTR pMailTo = NULL;
    UINT cchMailTo = 0;
    IADs* pDsObject = NULL;
    VARIANT variant;
    SHELLEXECUTEINFO sei = { 0 };
    INT i;
    USES_CONVERSION;
    DECLAREWAITCURSOR;

    TraceEnter(TRACE_VERBS, "_UserVerbCB");

    VariantInit(&variant);

    switch ( uCmd )
    {
        case MENUCMD_INITITEM:
        {
            // if this is a map network drive/find volume verb then lets ensure we only handle
            // a single selection.

            switch ( LOWORD(uID) )
            {
                case IDC_USER_OPENHOMEPAGE:
                {
                    if ( DPA_GetPtrCount(pvi->hdpaSelection) != 1 )
                    {
                        TraceMsg("Disabling as selection > 1");
                        EnableMenuItem(hMenu, HIWORD(uID), MF_BYCOMMAND|MF_GRAYED);
                    }

                    break;
                }
            }

            break;
        }

        case MENUCMD_INVOKE:
        {
            // if we have a selection and the user has picked a verb then we
            // need to get the UNC"s from the objects we are trying to invoke,
            // therefore lets build a DPA containing them.

            SetWaitCursor();

            for ( i = 0 ; i < DPA_GetPtrCount(pvi->hdpaSelection); i++ )
            {
                LPWSTR pPath = (LPWSTR)DPA_GetPtr(pvi->hdpaSelection, i);
                TraceAssert(pPath);

                DoRelease(pDsObject);
                VariantClear(&variant);

                Trace(TEXT("Binding to %s"), W2T(pPath));

                if ( FAILED(ADsOpenObject(pPath, pvi->pszUserName, pvi->pszPassword, 
                                          (pvi->dwFlags & DSDSOF_SIMPLEAUTHENTICATE) ? 0:ADS_SECURE_AUTHENTICATION, 
                                          IID_IADs, (LPVOID*)&pDsObject)) )
                {
                    TraceMsg("Failed to bind to the object");
                    continue;
                }

                if ( LOWORD(uID) == IDC_USER_OPENHOMEPAGE ) 
                {
                    // get the web address of the object and store it, this should
                    // only happen once.                 

                    if ( FAILED(pDsObject->Get(L"wWWHomePage", &variant)) )
                        continue;

                    if ( V_VT(&variant) == VT_BSTR )
                    {
                        Trace(TEXT("Storing URL "), W2T(V_BSTR(&variant)));

                        hr = LocalAllocStringW2T(&pURL, V_BSTR(&variant));
                        FailGracefully(hr, "Failed to store the URL");
                    }                   
                }
                else
                {
                    // ensure we have a DPA for storing the mail addresses of the
                    // objects we are invoked on.

                    if ( !hdpaMailTo )
                    {
                        hdpaMailTo = DPA_Create(4);
                        TraceAssert(hdpaMailTo);

                        if ( !hdpaMailTo )
                            ExitGracefully(hr, E_OUTOFMEMORY, "Failed to create the DPA for mail addresses");
                    }

                    if ( FAILED(pDsObject->Get(L"mail", &variant)) )
                        continue;

                    if ( V_VT(&variant) == VT_BSTR )
                    {
                        Trace(TEXT("Adding mail address %s to DPA"), W2T(V_BSTR(&variant)));
                        StringDPA_AppendString(hdpaMailTo, W2T(V_BSTR(&variant)), NULL);
                    }
                }
            }

            // now process the argument list that we have built.

            ResetWaitCursor();

            sei.cbSize = SIZEOF(sei);
            sei.hwnd = hWnd;
            sei.nShow = SW_SHOWNORMAL;

            switch ( LOWORD(uID) )
            {
                case IDC_USER_OPENHOMEPAGE:
                {
                    // if we have a URL then lets pass it to shell execute,
                    // otherwise report the failure to the user.

                    if ( !pURL )
                    {
                        FormatMsgBox(hWnd, GLOBAL_HINSTANCE, IDS_TITLE, IDS_ERR_NOHOMEPAGE, MB_OK|MB_ICONERROR);
                        ExitGracefully(hr, E_FAIL, "No URL defined");
                    }
 
                    Trace(TEXT("Executing URL %s"), pURL);
                    sei.lpFile = pURL;

                    if ( !ShellExecuteEx(&sei) )
                        ExitGracefully(hr, E_UNEXPECTED, "Failed in ShellExecuteEx");

                    break;
                }

                case IDC_USER_MAILTO:
                {
                    // If every single bind operation failed above,
                    // hdpaMailTo didn't get defined, and we'll fault
                    // if we try to use it.
                    if (hdpaMailTo)
                    {
                        // build a command line we can use for the mail to verb.

                        if ( DPA_GetPtrCount(hdpaMailTo) <= 0 )
                        {
                            FormatMsgBox(hWnd, GLOBAL_HINSTANCE, IDS_TITLE, IDS_ERR_NOMAILADDR, MB_OK|MB_ICONERROR);
                            ExitGracefully(hr, E_FAIL, "No mail addresses defined");
                        }

                        _BuildMailToLine(NULL, &cchMailTo, hdpaMailTo);

                        hr = LocalAllocStringLen(&pMailTo, cchMailTo);
                        FailGracefully(hr, "Failed to allocate mailto line:");

                        _BuildMailToLine(pMailTo, &cchMailTo, hdpaMailTo);
                
                        Trace(TEXT("Executing: %s"), pMailTo);
                        sei.lpFile = pMailTo;

                        if ( !ShellExecuteEx(&sei) )
                            ExitGracefully(hr, E_UNEXPECTED, "Failed in ShellExecuteEx");

                    }
                    else
                    {
                        //BUGBUG:  We need an error message, here
//                        FormatMsgBox(hWnd, GLOBAL_HINSTANCE, IDS_TITLE, IDS_ERR_NOMAILADDR, MB_OK|MB_ICONERROR);
                        ExitGracefully(hr, E_FAIL, "hdpaMailTo never initialized!");
                    }
                    break;
                }
            }
        }
    }

    hr = S_OK;                  // success

exit_gracefully:

    DoRelease(pDsObject);
    VariantClear(&variant);

    LocalFreeString(&pURL);

    StringDPA_Destroy(&hdpaMailTo);
    LocalFreeString(&pMailTo);

    ResetWaitCursor();

    TraceLeaveResult(hr);
}


/*----------------------------------------------------------------------------
/ Volume object verbs
/----------------------------------------------------------------------------*/

HRESULT _VolumeVerbCB(UINT uCmd, HWND hWnd, HMENU hMenu, LPARAM uID, LPVERBINFO pvi, UINT uFlags)
{
    HRESULT hr;
    HDPA hdpaUNC = NULL;
    IADs* pDsObject = NULL;
    VARIANT variant;
    INT i;
    LPITEMIDLIST pidl;
    USES_CONVERSION;
    DECLAREWAITCURSOR;

    TraceEnter(TRACE_VERBS, "_VolumeVerbCB");

    VariantInit(&variant);

    switch ( uCmd )
    {
        case MENUCMD_INITITEM:
        {
            // if this is a map network drive/find volume verb then lets ensure we only handle
            // a single selection.

            switch ( LOWORD(uID) )
            {
                case IDC_VOLUME_FIND:
                case IDC_VOLUME_MAPNETDRIVE:
                {
                    if ( DPA_GetPtrCount(pvi->hdpaSelection) != 1 )
                    {
                        TraceMsg("Disabling as selection > 1");
                        EnableMenuItem(hMenu, HIWORD(uID), MF_BYCOMMAND|MF_GRAYED);
                    }

                    // we remove the find verb if we the restrictions apply to remove it.

                    if ( LOWORD(uID) == IDC_VOLUME_FIND )
                    {
                        if ( SHRestricted(REST_NOFIND) )
                        {
                            TraceMsg("Restriction says 'no find', so deleting the find verb");
                            DeleteMenu(hMenu, HIWORD(uID), MF_BYCOMMAND);
                        }
                    }

                    break;
                }

                case IDC_VOLUME_OPEN:
                {
                    if ( !(uFlags & CMF_EXPLORE) )
                    {
                        TraceMsg("Not exploring, so making open the default verb");
                        SetMenuDefaultItem(hMenu, HIWORD(uID), MF_BYCOMMAND);
                    }

                    break;
                }

                case IDC_VOLUME_EXPLORE:
                {
                    if ( uFlags & CMF_EXPLORE )
                    {
                        TraceMsg("Exploring so making explore the default verb");
                        SetMenuDefaultItem(hMenu, HIWORD(uID), MF_BYCOMMAND);
                    }

                    break;
                }
            }

            break;
        }

        case MENUCMD_INVOKE:
        {
            // if we have a selection and the user has picked a verb then we
            // need to get the UNC"s from the objects we are trying to invoke,
            // therefore lets build a DPA containing them.

            SetWaitCursor();

            hdpaUNC = DPA_Create(4);
            TraceAssert(hdpaUNC);

            if ( !hdpaUNC )
                ExitGracefully(hr, E_OUTOFMEMORY, "Failed to get UNC DPA");

            for ( i = 0 ; i < DPA_GetPtrCount(pvi->hdpaSelection); i++ )
            {
                LPWSTR pPath = (LPWSTR)DPA_GetPtr(pvi->hdpaSelection, i);
                TraceAssert(pPath);

                DoRelease(pDsObject);
                VariantClear(&variant);

                Trace(TEXT("Binding to %s"), W2T(pPath));

                if ( FAILED(ADsOpenObject(pPath, pvi->pszUserName, pvi->pszPassword, 
                                          (pvi->dwFlags & DSDSOF_SIMPLEAUTHENTICATE) ? 0:ADS_SECURE_AUTHENTICATION, 
                                          IID_IADs, (LPVOID*)&pDsObject)) )        
                {
                    TraceMsg("Failed to bind to the object");
                    continue;
                }

                if ( FAILED(pDsObject->Get(L"uNCName", &variant)) )
                    continue;
                
                if ( V_VT(&variant) == VT_BSTR )
                {
                    Trace(TEXT("Adding UNC %s to DPA"), W2T(V_BSTR(&variant)));
                    StringDPA_AppendString(hdpaUNC, W2T(V_BSTR(&variant)), NULL);
                }
            }

            ResetWaitCursor();

            // we now have the selection stored in the DPA, so lets invoke the command
            // by walking the list of UNC's and calling the relevant invoke logic.

            Trace(TEXT("UNC DPA contains %d entries"), DPA_GetPtrCount(hdpaUNC));

            if ( !DPA_GetPtrCount(hdpaUNC) )
            {
                FormatMsgBox(hWnd, GLOBAL_HINSTANCE, IDS_TITLE, IDS_ERR_NOUNC, MB_OK|MB_ICONERROR);
                ExitGracefully(hr, E_FAIL, "No UNC paths defined");
            }

            for ( i = 0 ; i < DPA_GetPtrCount(hdpaUNC); i++ )
            {
                LPTSTR pUNC = (LPTSTR)DPA_GetPtr(hdpaUNC, i);
                TraceAssert(pUNC);

                Trace(TEXT("pUNC is %s"), pUNC);

                switch ( LOWORD(uID) )
                {
                    // explore and open we pass onto the shell.

                    case IDC_VOLUME_OPEN:
                    case IDC_VOLUME_EXPLORE:
                    {
                        SHELLEXECUTEINFO sei = { 0 };       // clears the structure

                        TraceMsg("Trying to open/explore to UNC");

                        sei.cbSize = SIZEOF(sei);
                        sei.hwnd = hWnd;
                        sei.lpFile = pUNC;
                        sei.nShow = SW_SHOWNORMAL;

                        if ( uID == IDC_VOLUME_EXPLORE )
                            sei.lpVerb = TEXT("explore");

                        ShellExecuteEx(&sei);
                        break;
                    }

                    // find we show the find UI by building an ITEMIDLIST for the UNC we
                    // have and then call the shells find UI.

                    case IDC_VOLUME_FIND:
                    {
                        TraceMsg("Invoking find on the UNC");

                        if ( SUCCEEDED(SHILCreateFromPath(pUNC, &pidl, NULL)) )
                        {
                            SHFindFiles(pidl, NULL);
                            ILFree(pidl);
                        }

                        break;
                    }

                    // lets get a net connection from SHStartNetConnection...

                    case IDC_VOLUME_MAPNETDRIVE:
                    {
                        Trace(TEXT("Invoking Map Network Drive for: %s"), pUNC);
                        SHStartNetConnectionDialog(hWnd, pUNC, RESOURCETYPE_DISK);
                        break;
                    }

                    default:
                    {
                        TraceAssert(FALSE);
                        ExitGracefully(hr, E_UNEXPECTED, "Failed to invoke, bad uID");
                    }
                }
            }
        }
    }

    hr = S_OK;                  // success

exit_gracefully:

    DoRelease(pDsObject);
    VariantClear(&variant);

    StringDPA_Destroy(&hdpaUNC);

    ResetWaitCursor();

    TraceLeaveResult(hr);
}


/*----------------------------------------------------------------------------
/ Computer object verbs
/----------------------------------------------------------------------------*/

HRESULT _ComputerVerbCB(UINT uCmd, HWND hWnd, HMENU hMenu, LPARAM uID, LPVERBINFO pvi, UINT uFlags)
{
    HRESULT hr;
    INT i;
    IADs * pDsObject = NULL;
    LPTSTR pArguments = NULL;
    LPTSTR pComputer = NULL;
    TCHAR szBuffer[MAX_PATH];
    USES_CONVERSION;
    DECLAREWAITCURSOR = GetCursor();

    TraceEnter(TRACE_VERBS, "_ComputerVerbCB");

    if ( LOWORD(uID) != IDC_COMPUTER_MANAGE )
        ExitGracefully(hr, E_INVALIDARG, "Not computer manange, so bailing");

    switch ( uCmd )
    {
        case MENUCMD_INITITEM:
        {
            if ( DPA_GetPtrCount(pvi->hdpaSelection) != 1 )
            {
                TraceMsg("Selection is != 1, so disabling verb");
                EnableMenuItem(hMenu, HIWORD(uID), MF_BYCOMMAND|MF_GRAYED);
            }

            break;
        }

        case MENUCMD_INVOKE:
        {
            LPWSTR pPath = (LPWSTR)DPA_GetPtr(pvi->hdpaSelection, 0);       // selection always 0
            TraceAssert(pPath);

            hr = ADsOpenObject(pPath, pvi->pszUserName, pvi->pszPassword, 
                              (pvi->dwFlags & DSDSOF_SIMPLEAUTHENTICATE) ? 0:ADS_SECURE_AUTHENTICATION, 
                              IID_IADs, (LPVOID*)&pDsObject);

            FailGracefully(hr, "Failed to bind to computer object");
    
            VARIANT vNetAddr;
            hr = pDsObject->Get(L"dNSHostName", &vNetAddr);
            
            if (SUCCEEDED(hr)) {
              hr = LocalAllocString (&pComputer, W2T(vNetAddr.bstrVal));
              FailGracefully(hr, "Failed to copy computer address somewhere interesting");
            } else {
              if (hr == E_ADS_PROPERTY_NOT_FOUND) {
                hr = pDsObject->Get(L"sAMAccountName", &vNetAddr);
                if (SUCCEEDED(hr)) {
                  hr = LocalAllocString(&pComputer, W2T (vNetAddr.bstrVal));
                  FailGracefully(hr, "Failed to copy SAM account name somewhere interesting");
                  
                  // To make the computer name useful we must remove the trailing dollar if
                  // there is one.  Therefore scan to the end of the string and nuke the
                  // last character.
                  
                  INT i = lstrlen(pComputer);
                  TraceAssert(i > 1);
                  
                  if ( (i > 1) && (pComputer[i-1] == TEXT('$')) )
                    {
                      pComputer[i-1] = TEXT('\0');
                      Trace(TEXT("Fixed computer name: %s"), pComputer);
                    }
                  
                } else 
                  FailGracefully (hr, "Failed to find a usable machine address");
              }
            }
            hr = FormatMsgResource(&pArguments, g_hInstance, IDS_COMPUTER_MANAGECMD, pComputer);
            FailGracefully(hr, "Failed to format MMC cmd line");
            
            ExpandEnvironmentStrings(pArguments, szBuffer, ARRAYSIZE(szBuffer));
            Trace(TEXT("MMC cmd line: mmc.exe %s"), szBuffer);

            ResetWaitCursor();

            ShellExecute(NULL, NULL, TEXT("mmc.exe"), szBuffer,  NULL, SW_SHOWNORMAL);
        }
    }

    hr = S_OK;                  // success

exit_gracefully:

    DoRelease(pDsObject);

    LocalFreeString (&pComputer);

    TraceLeaveResult(hr);
}


/*----------------------------------------------------------------------------
/ printQueue object verb implementations
/----------------------------------------------------------------------------*/

#ifdef WINNT

#define PRINT_FMT            TEXT("printui.dll,PrintUIEntry /n \"%s\" ")
#define PRINT_SWITCH_OPEN    TEXT("/o ")
#define PRINT_SWITCH_INSTALL TEXT("/in ")

#else

// code to invoke the printer installing on Win9x.

DWORD WINAPI _InvokePrinterThread(PVOID pThreadData)
{
    LPTSTR              pPrinterUNC = (LPTSTR)pThreadData;
    HRESULT             hr;
    DWORD               cchBuffer;
    HINSTANCE           hLib;
    PRINTERSETUPPROC32  pfnPrinterSetup;

    TraceEnter(TRACE_VERBS, "_InvokePrinterInstallVerb");
    TraceAssert(pPrinterUNC);

    //
    // Load the win9x printer installation dll.  We don't link to this directly 
    // because msprint2.dll is an 9x binary only and it brings in other  
    // config manager dlls.
    //
    hLib = LoadLibrary(TEXT("MSPRINT2.DLL"));
    if( !hLib )
    {
        ExitGracefully(hr, E_FAIL, "_InvokePrinterInstallVerb could not load MSPRINT2.DLL");
    }

    //
    // Get the setup procedure entry point.
    //
    pfnPrinterSetup = (PRINTERSETUPPROC32)GetProcAddress(hLib, "PrinterSetup32");
    if (!pfnPrinterSetup)
    {
        ExitGracefully(hr, E_FAIL, "_InvokePrinterInstallVerb could get PrinterSetup32 proc address");
    }

    //
    // Tell the printer setup code to install this printer connection.  Note on win9x printer
    // connections are really just local printers with redirected ports, similar to a masq
    // printer on NT.
    //
    cchBuffer = lstrlen(pPrinterUNC) + 1;
    if( !pfnPrinterSetup(NULL, MSP_NETPRINTER, (WORD)cchBuffer, (LPBYTE)pPrinterUNC, (LPWORD)&cchBuffer) )   
    {
        ExitGracefully(hr, E_FAIL, "_InvokePrinterInstallVerb could not install printer.");
    }

    hr = S_OK;

exit_gracefully:

    if( hLib )
    {
        FreeLibrary(hLib);
    }

    LocalFreeString(&pPrinterUNC);
    InterlockedDecrement(&GLOBAL_REFCOUNT);
    TraceLeave();
    ExitThread(0);
    return 0;
}

#endif

BOOL _PrinterCheckRestrictions(HWND hwnd, RESTRICTIONS rest)
{
    if ( SHRestricted(rest) )
    {
        FormatMsgBox(hwnd, GLOBAL_HINSTANCE, IDS_RESTRICTIONSTITLE, IDS_RESTRICTIONS, MB_OK|MB_ICONERROR);
        return TRUE;
    }
    return FALSE;
}

HRESULT _PrinterRunDLLCountAtSymbols( LPCTSTR pszPrinterName, UINT *puCount )
{
    HRESULT hr = E_FAIL;

    if( pszPrinterName && puCount )
    {
        *puCount = 0;

        while( *pszPrinterName )
        {
            if( TEXT('@') == *pszPrinterName++ )
            {
                (*puCount) ++;
            }
        }

        hr = S_OK;
    }

    return hr;
}

HRESULT _PrinterRunDLLFormatAtSymbols( LPTSTR pszBuffer, UINT uBufSize, LPCTSTR pszPrinterName )
{
    HRESULT hr = E_FAIL;

    if( pszPrinterName && pszBuffer && uBufSize )
    {
        // the buffer end - where we will put the zero terminator
        LPTSTR  pszBufEnd = pszBuffer + uBufSize - 1;

        // format the printer name quoting the @ symbols
        while( *pszPrinterName )
        {
            if( TEXT('@') == *pszPrinterName )
            {
                // check the buffer size
                if( (pszBuffer+1) >= pszBufEnd )
                    break; // not enough space

                // we have space in the buffer
                *pszBuffer++ = TEXT('\\');
                *pszBuffer++ = *pszPrinterName++;
            }
            else
            {
                // check the buffer size
                if( pszBuffer >= pszBufEnd )
                    break; // not enough space

                // we have space in the buffer
                *pszBuffer++ = *pszPrinterName++;
            }
        }

        if( 0 == *pszPrinterName )
        {
            // the buffer is long enough
            hr = S_OK;
        }
        else
        {
            // we hit the insufficent buffer error
            hr = HRESULT_FROM_WIN32( ERROR_INSUFFICIENT_BUFFER );
        }

        // put the zero terminator
        *pszBuffer = 0;
    }

    return hr;
}

HRESULT _PrinterVerbCB(UINT uCmd, HWND hWnd, HMENU hMenu, LPARAM uID, LPVERBINFO pvi, UINT uFlags)
{
    HRESULT hr;
    IADs* pDsObject = NULL;
#ifdef WINNT
    LPTSTR pPrinterUNC = NULL;
    LPTSTR pBuffer = NULL;
    LPTSTR pPrinterName = NULL;
    UINT uAtSymbolsCount, uBufSize;
#else
    LPTSTR pPrinterServer = NULL;
    LPTSTR pPrinterShare = NULL;
    LPTSTR pThreadCmd = NULL;
    UINT cchThreadCmd = 0;
    DWORD dwThreadID;
    HANDLE hThread;
#endif
    INT i;
    VARIANT variant;
    USES_CONVERSION;
    DECLAREWAITCURSOR;

    TraceEnter(TRACE_VERBS, "_ComputerVerbCB");

    VariantInit(&variant);

    switch ( uCmd )
    {
        case MENUCMD_INITITEM:
        {
            // printers want the open verb as their default.
            if ( LOWORD(uID) == IDC_PRINTER_INSTALL )
            {
                TraceMsg("Install should be the default verb for printQueue objects");
                SetMenuDefaultItem(hMenu, HIWORD(uID), MF_BYCOMMAND);
            }

            // printer verbs only work on a single selection.
            if ( DPA_GetPtrCount(pvi->hdpaSelection) != 1 )
            {
                TraceMsg("Selection is != 1, so disabling verb");
                EnableMenuItem(hMenu, HIWORD(uID), MF_BYCOMMAND|MF_GRAYED);
            }

            break;
        }

        case MENUCMD_INVOKE:
        {
            LPWSTR pPath = (LPWSTR)DPA_GetPtr(pvi->hdpaSelection, 0);       // selection always 0
            TraceAssert(pPath);

            SetWaitCursor();

            hr = ADsOpenObject(pPath, pvi->pszUserName, pvi->pszPassword, 
                              (pvi->dwFlags & DSDSOF_SIMPLEAUTHENTICATE) ? 0:ADS_SECURE_AUTHENTICATION, 
                              IID_IADs, (LPVOID*)&pDsObject);

            FailGracefully(hr, "Failed to get pDsObject");

#ifdef WINNT
            // for Windows NT we can grab the UNC name and build a command line
            // we invoke the printUI dll using.  

            hr = pDsObject->Get(L"uNCName", &variant);
            FailGracefully(hr, "Failed to get UNC from the printer object");

            if ( V_VT(&variant) != VT_BSTR )
                ExitGracefully(hr, E_FAIL, "UNC is not a BSTR - whats with that?");

            hr = LocalAllocStringW2T(&pPrinterUNC, W2T(V_BSTR(&variant)));
            FailGracefully(hr, "Failed to copy the printerUNC");

            Trace(TEXT("printQueue object UNC: %s"), pPrinterUNC);

            hr = _PrinterRunDLLCountAtSymbols(pPrinterUNC, &uAtSymbolsCount);
            FailGracefully(hr, "Failed to count the @ symbols");

            uBufSize = lstrlen(pPrinterUNC) + uAtSymbolsCount + 1;
            hr = LocalAllocStringLen(&pPrinterName,  uBufSize);
            FailGracefully(hr, "Failed to copy the printerName");

            hr = _PrinterRunDLLFormatAtSymbols(pPrinterName, uBufSize, pPrinterUNC);
            FailGracefully(hr, "Failed to format printerName @ symbols ");

            // allocate the format buffer.
            hr = LocalAllocStringLen(&pBuffer, lstrlen(PRINT_FMT) + 
                                               lstrlen(PRINT_SWITCH_OPEN) + 
                                               lstrlen(PRINT_SWITCH_INSTALL) +
                                               lstrlen(pPrinterName) + 1 );
                                               
            FailGracefully(hr, "Failed to allocate format buffer");

            // now format the line...

            wsprintf(pBuffer, PRINT_FMT, pPrinterName);

            switch ( LOWORD(uID) )
            {
                case IDC_PRINTER_OPEN:
                    StrCat(pBuffer, PRINT_SWITCH_OPEN);
                    break;

                case IDC_PRINTER_INSTALL:
                    StrCat(pBuffer, PRINT_SWITCH_INSTALL);
                    break;
            }

            ResetWaitCursor();

            BOOL bRunCommand = TRUE;
            if( IDC_PRINTER_INSTALL == LOWORD(uID) && _PrinterCheckRestrictions(hWnd, REST_NOPRINTERADD) )
                bRunCommand = FALSE;

            if( bRunCommand )
            {
                Trace(TEXT("Invoking: rundll32.exe %s"), pBuffer);
                ShellExecute(NULL, NULL, TEXT("rundll32.exe"), pBuffer,  NULL, SW_SHOWNORMAL);
            }
#else
            // On Win9x we need to cope with the old printing sub-system so
            // we get the server and share and then call into that UI to
            // get the correct UI.

            hr = pDsObject->Get(L"shortServerName", &variant);
            FailGracefully(hr, "Failed to get server name from the printer object");

            if ( V_VT(&variant) != VT_BSTR )
                ExitGracefully(hr, E_FAIL, "Server name is not a BSTR - whats with that?");

            hr = LocalAllocStringW2T(&pPrinterServer, V_BSTR(&variant));
            FailGracefully(hr, "Failed to copy the printer server");

            VariantClear(&variant);

            hr = pDsObject->Get(L"printShareName", &variant);
            FailGracefully(hr, "Failed to get share name from the printer object");

            if ( V_VT(&variant) != VT_BSTR )
                ExitGracefully(hr, E_FAIL, "Share name is not a BSTR - whats with that?");

            hr = LocalAllocStringW2T(&pPrinterShare, V_BSTR(&variant));
            FailGracefully(hr, "Failed to copy the printer share name");

            Trace(TEXT("Printer server: %s, share: %s"), pPrinterServer, pPrinterShare);

            // now format a suitable command line we can pass into the thread
            // which will launch the print UI.

            cchThreadCmd = 3;           // '\\'...'\'...

            PutStringElement(NULL, &cchThreadCmd, pPrinterServer);
            PutStringElement(NULL, &cchThreadCmd, pPrinterShare);

            hr = LocalAllocStringLen(&pThreadCmd, cchThreadCmd);
            FailGracefully(hr, "Failed to create full share name buffer");

            PutStringElement(pThreadCmd, NULL, TEXT("\\\\"));
            PutStringElement(pThreadCmd, NULL, pPrinterServer);
            PutStringElement(pThreadCmd, NULL, TEXT("\\"));
            PutStringElement(pThreadCmd, NULL, pPrinterShare);

            Trace(TEXT("Command string to pass to install printer thread: %s"), pThreadCmd);
            
            // now spin off a thread which can display the UI for installing 
            // the printer.

            ResetWaitCursor();
            InterlockedIncrement(&GLOBAL_REFCOUNT);     // unloading would be bad!

            hThread = CreateThread(NULL, 0, _InvokePrinterThread, (LPVOID)pThreadCmd, 0, &dwThreadID);
            TraceAssert(hThread);

            if( !hThread )
            {
                LocalFreeString(&pThreadCmd);
                InterlockedDecrement(&GLOBAL_REFCOUNT);
                ExitGracefully(hr, E_FAIL, "Failed to create thread and install printer");
            }
           
            CloseHandle(hThread);
#endif
        }
    }

    hr = S_OK;                  // success

exit_gracefully:

    VariantClear(&variant);

    DoRelease(pDsObject);

#ifdef WINNT    
    LocalFreeString(&pPrinterUNC);
    LocalFreeString(&pPrinterName);
    LocalFreeString(&pBuffer);
#else
    LocalFreeString(&pPrinterServer);
    LocalFreeString(&pPrinterShare);
#endif

    ResetWaitCursor();

    TraceLeaveResult(hr);
}
