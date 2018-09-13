#include "pch.h"
#include "wab.h"
#pragma hdrstop


/*-----------------------------------------------------------------------------
/ Misc data
/----------------------------------------------------------------------------*/

//
//  CDsPropertyPages is used to display the property pages, context menus etc
//

class CDsPropertyPages : public IWABExtInit, IShellExtInit, IContextMenu, IShellPropSheetExt, IObjectWithSite, CUnknown
{
    private:
        IUnknown* _punkSite;
        IDataObject* _pDataObject;
        HDSA         _hdsaMenuItems;               

        SHORT AddMenuItem(HMENU hMenu, LPWSTR pMenuReference, UINT index, UINT uIDFirst, UINT uIDLast, UINT uFlags);

    public:
        CDsPropertyPages();
        ~CDsPropertyPages();

        // IUnknown members
        STDMETHODIMP_(ULONG) AddRef();
        STDMETHODIMP_(ULONG) Release();
        STDMETHODIMP QueryInterface( REFIID, LPVOID FAR* );

        // IShellExtInit
        STDMETHODIMP Initialize(LPCITEMIDLIST pIDFolder, LPDATAOBJECT pDataObj, HKEY hKeyID);

        // IWABExtInit
        STDMETHODIMP Initialize(LPWABEXTDISPLAY pWED);

        // IShellPropSheetExt
        STDMETHODIMP AddPages(LPFNADDPROPSHEETPAGE pAddPageProc, LPARAM lParam);
        STDMETHODIMP ReplacePage(UINT uPageID, LPFNADDPROPSHEETPAGE pReplacePageFunc, LPARAM lParam);

        // IContextMenu
        STDMETHODIMP QueryContextMenu(HMENU hMenu, UINT uIndex, UINT uIDFirst, UINT uIDLast, UINT uFlags);
        STDMETHODIMP InvokeCommand(LPCMINVOKECOMMANDINFO pCMI);
        STDMETHODIMP GetCommandString(UINT_PTR uID, UINT uFlags, UINT FAR* reserved, LPSTR pName, UINT ccMax);

        // IObjectWithSite
        STDMETHODIMP SetSite(IUnknown* punk);
        STDMETHODIMP GetSite(REFIID riid, void **ppv);
};


//
// To handle the conversion from a IWABExtInit to an IShellExtInit we must
// provide an IDataObject implementation that supports this.  This doesn't need
// to be too public, therefore lets define it here.
//

class CWABDataObject : public IDataObject, CUnknown
{
    private:
        LPWSTR _pPath;
        IADs* _pDsObject;

    public:
        CWABDataObject(LPWSTR pDN);
        ~CWABDataObject();

        // IUnknown
        STDMETHOD(QueryInterface)(REFIID riid, LPVOID* ppvObject);
        STDMETHOD_(ULONG, AddRef)();
        STDMETHOD_(ULONG, Release)();

        // IDataObject
        STDMETHODIMP GetData(FORMATETC *pformatetcIn, STGMEDIUM *pmedium);
        STDMETHODIMP GetDataHere(FORMATETC *pformatetc, STGMEDIUM *pmedium);
        STDMETHODIMP QueryGetData(FORMATETC *pformatetc);
        STDMETHODIMP GetCanonicalFormatEtc(FORMATETC *pformatectIn, FORMATETC *pformatetcOut);
        STDMETHODIMP SetData(FORMATETC *pformatetc, STGMEDIUM *pmedium, BOOL fRelease);
        STDMETHODIMP EnumFormatEtc(DWORD dwDirection, IEnumFORMATETC **ppenumFormatEtc);
        STDMETHODIMP DAdvise(FORMATETC *pformatetc, DWORD advf, IAdviseSink *pAdvSink, DWORD *pdwConnection);
        STDMETHODIMP DUnadvise(DWORD dwConnection);
        STDMETHODIMP EnumDAdvise(IEnumSTATDATA **ppenumAdvise);
};


//
// clipboard formats exposed
//

CLIPFORMAT g_cfDsObjectNames = 0;
CLIPFORMAT g_cfDsDispSpecOptions = 0;


//
// Having extracted the menu item handler list from the cache we then
// convert it DSA made of the following items.  For
//

typedef struct
{
    INT           cAdded;                   // number of verbs added
    IContextMenu* pContextMenu;             // IContextMenu handler interface / = NULL
    LPTSTR        pCaption;                 // Display text for the command, used for the help text
    LPTSTR        pCommand;                 // Command line passed to shell execute
} DSMENUITEM, * LPDSMENUITEM;



/*----------------------------------------------------------------------------
/ Helper functions
/----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
/ _FreeMenuItem
/ -------------
/   Tidy up a DSMENUITEM structure, releasing all memory, interfaces etc.
/
/ In:
/   pItem -> item to be released
/
/ Out:
/   VOID
/----------------------------------------------------------------------------*/

VOID _FreeMenuItem(LPDSMENUITEM pItem)
{
    TraceEnter(TRACE_UI, "_FreeMenuItem");

    DoRelease(pItem->pContextMenu);
    LocalFreeString(&pItem->pCaption);
    LocalFreeString(&pItem->pCommand);

    TraceLeave();
}

//
// Helper for DSA destruction
//

INT _FreeMenuItemCB(LPVOID pVoid, LPVOID pData)
{
    LPDSMENUITEM pItem = (LPDSMENUITEM)pVoid;
    TraceAssert(pItem);

    TraceEnter(TRACE_UI, "_FreeMenuItemCB");

    _FreeMenuItem(pItem);

    TraceLeaveValue(TRUE);
}


/*----------------------------------------------------------------------------
/ CDsPropertyPages implementation
/----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
/ IUnknown
/----------------------------------------------------------------------------*/

CDsPropertyPages::CDsPropertyPages()
{
    _punkSite = NULL;
    _pDataObject = NULL;
    _hdsaMenuItems = NULL;
}

CDsPropertyPages::~CDsPropertyPages()
{
    DoRelease(_punkSite);
    DoRelease(_pDataObject);

    if ( _hdsaMenuItems )
        DSA_DestroyCallback(_hdsaMenuItems, _FreeMenuItemCB, NULL);
}

// IUnknown bits

#undef CLASS_NAME
#define CLASS_NAME CDsPropertyPages
#include "unknown.inc"

STDMETHODIMP CDsPropertyPages::QueryInterface(REFIID riid, LPVOID* ppvObject)
{
    INTERFACES iface[] =
    {
        &IID_IShellExtInit, (LPSHELLEXTINIT)this,
        &IID_IShellPropSheetExt, (LPSHELLPROPSHEETEXT)this,
        &IID_IContextMenu, (LPCONTEXTMENU)this,
        &IID_IWABExtInit, (IWABExtInit*)this,
        &IID_IObjectWithSite, (IObjectWithSite*)this,
    };

    return HandleQueryInterface(riid, ppvObject, iface, ARRAYSIZE(iface));
}

//
// handle create instance
//

STDAPI CDsPropertyPages_CreateInstance(IUnknown* punkOuter, IUnknown** ppunk, LPCOBJECTINFO poi)
{
    CDsPropertyPages *pdpp = new CDsPropertyPages();
    if ( !pdpp )
        return E_OUTOFMEMORY;

    HRESULT hres = pdpp->QueryInterface(IID_IUnknown, (void **)ppunk);
    pdpp->Release();
    return hres;
}


/*-----------------------------------------------------------------------------
/ CDsPropertyPages::AddMenuItem
/ -----------------------------
/   This object maintains a DSA containing the currently active menu item list,
/   this adds a menu item to that list and also merges with the specified
/   hMenu.  We are given a string which reperesnets the menu to add, this
/   can either be a GUID, or "display text,command" which we then parse
/   and make a suitable entry for.
/
/   The DSA reflects the items that we add and contains the IContextMenu
/   handler iface pointers for the things we drag in.
/
/ In:
/   hMenu = menu to merge into
/   pMenuReference -> string defining item to add
/   index = index to insert the item at
/   uIDFirst, uIDLast, uFlags = IContextMenu::QueryContextMenu parameters
/
/ Out:
/   SHORT = the number of items merged
/----------------------------------------------------------------------------*/
SHORT CDsPropertyPages::AddMenuItem(HMENU hMenu, LPWSTR pMenuReference, UINT index, UINT uIDFirst, UINT uIDLast, UINT uFlags)
{
    HRESULT hres;
    GUID guid;
    WCHAR szCaption[MAX_PATH];
    WCHAR szCommand[MAX_PATH];
    DSMENUITEM item;
    IShellExtInit* pShellExtInit = NULL;
    IObjectWithSite *pows = NULL;
    USES_CONVERSION;

    TraceEnter(TRACE_UI, "CDsPropertyPages::AddMenuItem");

    // initialize the item structure we are going to keep, then try and crack the
    // item information we have been given

    if ( !hMenu )
        ExitGracefully(hres, E_INVALIDARG, "Bad arguments to _AddMenuItem");

    item.cAdded = 0;
    item.pContextMenu = NULL;
    item.pCaption = NULL;
    item.pCommand = NULL;

    if ( GetGUIDFromStringW(pMenuReference, &guid) )
    {
        // its a GUID, therefore lets pull in the Win32 extension that provides it, and allow it
        // to add in its verbs.  We then hang onto the IContextMenu interface so that we can
        // pass further requests to it (InvokeCommand, GetCommandString).

        hres = CoCreateInstance(guid, NULL, CLSCTX_INPROC_SERVER, IID_IContextMenu, (LPVOID*)&item.pContextMenu);
        FailGracefully(hres, "Failed to get IContextMenu from the GUID");

        if ( _punkSite && 
                SUCCEEDED(item.pContextMenu->QueryInterface(IID_IObjectWithSite,(void**)&pows)) )
        {
            hres = pows->SetSite(_punkSite);
            FailGracefully(hres, "Failed to ::SetSite on the extension object");
        }

        if ( SUCCEEDED(item.pContextMenu->QueryInterface(IID_IShellExtInit, (LPVOID*)&pShellExtInit)) )
        {
            hres = pShellExtInit->Initialize(NULL, _pDataObject, NULL);
            FailGracefully(hres, "Failed when calling IShellExtInit::Initialize");
        }

        hres = item.pContextMenu->QueryContextMenu(hMenu, index, uIDFirst, uIDLast, uFlags);
        FailGracefully(hres, "Failed when calling QueryContextMenu");

        item.cAdded = ShortFromResult(hres);
    }
    else
    {
        // its not a GUID therefore lets pull apart the string we have, it should
        // consist of the display text for the menu item, and then a command to pass
        // to ShellExecute.

        Trace(TEXT("Parsing: %s"), W2T(pMenuReference));

        if ( SUCCEEDED(GetStringElementW(pMenuReference, 0, szCaption, ARRAYSIZE(szCaption))) && 
             SUCCEEDED(GetStringElementW(pMenuReference, 1, szCommand, ARRAYSIZE(szCommand))) )
        {
            hres = LocalAllocStringW2T(&item.pCaption, szCaption);
            FailGracefully(hres, "Failed to add 'prompt' to structure");

            hres = LocalAllocStringW2T(&item.pCommand, szCommand);
            FailGracefully(hres, "Failed to add 'command' to structure");

            Trace(TEXT("uID: %08x, Caption: %s, Command: %s"), 
                            uIDFirst, item.pCaption, item.pCommand);

            if ( !InsertMenu(hMenu, index, MF_BYPOSITION|MF_STRING, uIDFirst, item.pCaption) )
               ExitGracefully(hres, E_FAIL, "Failed to add the menu item to hMenu");

            item.cAdded = 1;
        }
    }
    
    hres = S_OK;              // success

exit_gracefully:
    
    if ( SUCCEEDED(hres) )
    {
        if ( -1 == DSA_AppendItem(_hdsaMenuItems, &item) )
            ExitGracefully(hres, E_FAIL, "Failed to add the item to the DSA");
    }
    else
    {
        _FreeMenuItem(&item);           // make sure we tidy up
    }

    DoRelease(pows);
    DoRelease(pShellExtInit);

    TraceLeaveValue((SHORT)item.cAdded);
}


/*----------------------------------------------------------------------------
/ IShellExtInit
/----------------------------------------------------------------------------*/

STDMETHODIMP CDsPropertyPages::Initialize(LPCITEMIDLIST pIDFolder, LPDATAOBJECT pDataObj, HKEY hKeyID)
{
    HRESULT hres;

    TraceEnter(TRACE_UI, "CDsPropertyPages::Initialize (IShellExtInit)");

    // Release the previous data object and then pick up the new one that
    // we are going to be using.

    DoRelease(_pDataObject);

    if ( !pDataObj )
        ExitGracefully(hres, E_INVALIDARG, "Failed because we don't have a data object");

    pDataObj->AddRef();
    _pDataObject = pDataObj;

    // Check that we have the clipboard format correctly registered so that we
    // can collect a DSOBJECTNAMES structure

    if ( !g_cfDsObjectNames )
    {
        g_cfDsObjectNames = (CLIPFORMAT)RegisterClipboardFormat(CFSTR_DSOBJECTNAMES);
        g_cfDsDispSpecOptions = (CLIPFORMAT)RegisterClipboardFormat(CFSTR_DSDISPLAYSPECOPTIONS);

        if ( !g_cfDsObjectNames || !g_cfDsDispSpecOptions )
        {
            ExitGracefully(hres, E_FAIL, "No clipboard form registered");
        }
    }

    hres = S_OK;              // success

exit_gracefully:

    TraceLeaveResult(hres);
}


/*----------------------------------------------------------------------------
/ IWABExtInit
/----------------------------------------------------------------------------*/

#define WAB_PREFIX     L"ldap:///"
#define CCH_WAB_PREFIX 8

STDMETHODIMP CDsPropertyPages::Initialize(LPWABEXTDISPLAY pWED)
{
    HRESULT hres;
    WCHAR szDecodedURL[INTERNET_MAX_URL_LENGTH];
    LPWSTR pszDecodedURL = szDecodedURL;
    INT cchDecodedURL;
    DWORD dwLen = ARRAYSIZE(szDecodedURL);
    IDataObject* pDataObject = NULL;
    LPWSTR pszPath = NULL;
    LPWSTR pURL = (LPWSTR)pWED->lpsz;
    INT i;
    USES_CONVERSION;

    TraceEnter(TRACE_UI, "CDsPropertyPages::Initialize (IWABExtInit)");

    if ( !(pWED->ulFlags & WAB_DISPLAY_ISNTDS) )
        ExitGracefully(hres, E_FAIL, "The URL is not from NTDS, therefore ignoring");

    if ( !pURL )
        ExitGracefully(hres, E_FAIL, "URL pointer is NULL");

    Trace(TEXT("LDAP URL is: %s"), W2T(pURL));

    //
    // we must now convert from a RFC LDAP URL to something that ADSI can handle, because
    // although they both have the LDAP scheme they don't really mean the same thing.
    //
    // WAB will pass us an encoded URL, this we need to decode, strip the scheme name and
    // then remove the tripple slash,
    //
    // eg: "LDAP:///dn%20dn" becomes, "LDAP://dn dn"
    //

    hres = UrlUnescapeW(pURL, szDecodedURL, &dwLen, 0);
    FailGracefully(hres, "Failed to convert URL to decoded format");

    Trace(TEXT("Decoded URL is: %s"), W2T(szDecodedURL));

    pszDecodedURL += CCH_WAB_PREFIX;         // skip the LDAP:///

    //
    // now tail the URL removing all trailing slashes from it
    //

    for ( cchDecodedURL = lstrlenW(pszDecodedURL); 
                (cchDecodedURL > 0) && (pszDecodedURL[cchDecodedURL] == L'/'); 
                    cchDecodedURL-- )
    {
        pszDecodedURL[cchDecodedURL] = L'\0';
    }
    
    if ( !cchDecodedURL )
        ExitGracefully(hres, E_UNEXPECTED, "URL is now NULL");

    //
    // so we have a DN, so lets allocate a IDataObject using it so that we
    // can pass this into the real initialize method for shell extensions.
    //

    Trace(TEXT("DN from the LDAP URL we were given: %s"), W2T(pszDecodedURL));

    pDataObject = new CWABDataObject(pszDecodedURL);
    TraceAssert(pDataObject);

    if ( !pDataObject )
        ExitGracefully(hres, E_OUTOFMEMORY, "Failed to allocate the data object");

    hres = Initialize(NULL, pDataObject, NULL);
    FailGracefully(hres, "Failed to initialize with the IDataObject");

    // hres = S_OK;           // success

exit_gracefully:

    DoRelease(pDataObject);

    TraceLeaveResult(hres);
}





/*----------------------------------------------------------------------------
/ IShellPropSheetExt
/----------------------------------------------------------------------------*/

HRESULT TabCollector_Collect(IUnknown *punkSite, IDataObject* pDataObject, LPFNADDPROPSHEETPAGE pAddPageProc, LPARAM lParam);

STDMETHODIMP CDsPropertyPages::AddPages(LPFNADDPROPSHEETPAGE pAddPageProc, LPARAM lParam)
{
    HRESULT hres;
    
    TraceEnter(TRACE_UI, "CDsPropertyPages::AddPages");

    hres = TabCollector_Collect(_punkSite, _pDataObject, pAddPageProc, lParam);
    FailGracefully(hres, "Failed when calling the collector");

    //hres = S_OK;              // success

exit_gracefully:

    TraceLeaveResult(hres);
}

/*---------------------------------------------------------------------------*/

STDMETHODIMP CDsPropertyPages::ReplacePage(UINT uPageID, LPFNADDPROPSHEETPAGE lpfnReplaceWith, LPARAM lParam)
{
    TraceEnter(TRACE_UI, "CDsPropertyPages::ReplacePage");
    TraceLeaveResult(E_NOTIMPL);
}


/*----------------------------------------------------------------------------
/ IContextMenu
/----------------------------------------------------------------------------*/

STDMETHODIMP CDsPropertyPages::QueryContextMenu(HMENU hMenu, UINT index, UINT uIDFirst, UINT uIDLast, UINT uFlags)
{
    HRESULT hres;
    STGMEDIUM medium = { TYMED_NULL };
    FORMATETC fmte = {g_cfDsObjectNames, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    LPDSOBJECTNAMES pDsObjectNames;
    LPWSTR pPath;
    LPWSTR pObjectClass;
    CLASSCACHEGETINFO ccgi = { 0 };
    LPCLASSCACHEENTRY pCacheEntry = NULL;
    INT i;
    INT cAdded = 0;
    USES_CONVERSION;
    
    TraceEnter(TRACE_UI, "CDsPropertyPages::QueryContextMenu");

    if ( !hMenu || !_pDataObject )
        ExitGracefully(hres, E_FAIL, "Either no IDataObject or no hMenu");

    // Get the bits of information we need from the data object, we are not
    // interested in a attributePrefix, therefore we skip that bit
    // and then look up the menu list in the cache.

    hres = _pDataObject->GetData(&fmte, &medium);
    FailGracefully(hres, "Failed to GetData using CF_DSOBJECTNAMES");

    pDsObjectNames = (LPDSOBJECTNAMES)medium.hGlobal;

    if ( pDsObjectNames->cItems < 1 )
        ExitGracefully(hres, E_FAIL, "Not enough objects in DSOBJECTNAMES structure");

    pPath = (LPWSTR)ByteOffset(pDsObjectNames, pDsObjectNames->aObjects[0].offsetName);
    pObjectClass = (LPWSTR)ByteOffset(pDsObjectNames, pDsObjectNames->aObjects[0].offsetClass);

    // fill the CLASSCACHEGETINFO record so we can cache the information from the
    // display specifiers.

    ccgi.dwFlags = CLASSCACHE_CONTEXTMENUS;
    ccgi.pPath = pPath;
    ccgi.pObjectClass = pObjectClass;
    ccgi.pDataObject = _pDataObject;

    hres = GetServerAndCredentails(&ccgi);
    FailGracefully(hres, "Failed to get the server name");

    hres = GetAttributePrefix(&ccgi.pAttributePrefix, _pDataObject);
    FailGracefully(hres, "Failed to get attributePrefix");

    Trace(TEXT("Class: %s; Attribute Prefix: %s; Server: %s"), 
                W2T(pObjectClass), W2T(ccgi.pAttributePrefix), ccgi.pServer ? W2T(ccgi.pServer):TEXT("<none>"));

    hres = ClassCache_GetClassInfo(&ccgi, &pCacheEntry);
    FailGracefully(hres, "Failed to get page list (via the cache)");

    // did we get a menu list?  If so lets pull it a part and generate a DSA
    // which lists the menu items we are going to be displaying.   

    if ( (pCacheEntry->dwCached & CLASSCACHE_CONTEXTMENUS) && pCacheEntry->hdsaMenuHandlers )
    {
        if ( _hdsaMenuItems )
            DSA_DestroyCallback(_hdsaMenuItems, _FreeMenuItemCB, NULL);

        _hdsaMenuItems = DSA_Create(SIZEOF(DSMENUITEM), 4);

        if ( !_hdsaMenuItems )
            ExitGracefully(hres, E_OUTOFMEMORY, "Failed to construct DSA for menu items");

        for ( i = DSA_GetItemCount(pCacheEntry->hdsaMenuHandlers) ; --i >= 0 ; )
        {
            LPDSMENUHANDLER pHandlerItem = (LPDSMENUHANDLER)DSA_GetItemPtr(pCacheEntry->hdsaMenuHandlers, i);
            TraceAssert(pHandlerItem);

            cAdded += AddMenuItem(hMenu, pHandlerItem->pMenuReference,
                                        index, uIDFirst+cAdded, uIDLast, uFlags);
        }
    }

    hres = S_OK;              // success

exit_gracefully:

    LocalFreeStringW(&ccgi.pAttributePrefix);
    LocalFreeStringW(&ccgi.pUserName);
    LocalFreeStringW(&ccgi.pPassword);
    LocalFreeStringW(&ccgi.pServer);

    ClassCache_ReleaseClassInfo(&pCacheEntry);
    ReleaseStgMedium(&medium);

    TraceLeaveResult(ResultFromShort(cAdded));
}

/*---------------------------------------------------------------------------*/

STDMETHODIMP CDsPropertyPages::InvokeCommand(LPCMINVOKECOMMANDINFO pCMI)
{
    HRESULT hres;
    STGMEDIUM medium = { TYMED_NULL };
    FORMATETC fmte = {g_cfDsObjectNames, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    LPTSTR pArguments = NULL;
    LPDSOBJECTNAMES pDsObjectNames;
    LPTSTR pPath;
    LPWSTR pObjectClass;
    DWORD object;
    INT i, id; 
    USES_CONVERSION;
    
    TraceEnter(TRACE_UI, "CDsPropertyPages::InvokeCommand");

    // Walk the DSA until we find an item in it that contains the range of
    // items we are looking for, this will either involve invoking the
    // command (via IContextMenu::InvokeCommand) or calling ShellExecute
    // for the objects in the selection.

    if ( HIWORD(pCMI->lpVerb) )
        ExitGracefully(hres, E_FAIL, "Bad lpVerb value for this handler");

    if ( !_hdsaMenuItems )
        ExitGracefully(hres, E_INVALIDARG, "No menu item DSA");

    for ( id = LOWORD(pCMI->lpVerb), i = 0 ; i < DSA_GetItemCount(_hdsaMenuItems) ; i++ )
    {
        LPDSMENUITEM pItem = (LPDSMENUITEM)DSA_GetItemPtr(_hdsaMenuItems, i);
        TraceAssert(pItem);

        Trace(TEXT("id %08x, cAdded %d"), id, pItem->cAdded);
        
        if ( id < pItem->cAdded )
        {
            if ( pItem->pContextMenu )
            {
                CMINVOKECOMMANDINFO cmi = *pCMI;
                cmi.lpVerb = (LPCSTR)id;

                Trace(TEXT("Calling IContextMenu iface with ID %d"), id);

                hres = pItem->pContextMenu->InvokeCommand(&cmi);
                FailGracefully(hres, "Failed when calling context menu handler (InvokeCommand)");
            }
            else
            {
                // the command is not serviced via an IContextMenu handler, therefore lets for
                // each object in the IDataObject call the command passing the arguments of
                // the ADsPath and the class.

                hres = _pDataObject->GetData(&fmte, &medium);
                FailGracefully(hres, "Failed to GetData using CF_DSOBJECTNAMES");

                pDsObjectNames = (LPDSOBJECTNAMES)medium.hGlobal;

                if ( pDsObjectNames->cItems < 1 )
                    ExitGracefully(hres, E_FAIL, "Not enough objects in DSOBJECTNAMES structure");

                Trace(TEXT("Calling ShellExecute for ID %d (%s)"), id, pItem->pCommand);

                for ( object = 0 ; object < pDsObjectNames->cItems ; object++ )
                {
                    pPath = W2T((LPWSTR)ByteOffset(pDsObjectNames, pDsObjectNames->aObjects[object].offsetName));
                    pObjectClass = (LPWSTR)ByteOffset(pDsObjectNames, pDsObjectNames->aObjects[object].offsetClass);

                    hres = LocalAllocStringLen(&pArguments, lstrlen(pPath)+lstrlenW(pObjectClass)+5);    // nb: +5 for space and quotes
                    FailGracefully(hres, "Failed to allocate buffer for arguments");

                    //
                    // does the object path have a space?  if so then lets wrap it in quotes
                    //

                    if ( StrChr(pPath, TEXT(' ')) )
                    {
                        StrCpy(pArguments, TEXT("\""));
                        StrCat(pArguments, pPath);
                        StrCat(pArguments, TEXT("\""));
                    }
                    else
                    {
                        StrCpy(pArguments, pPath);
                    }

                    StrCat(pArguments, TEXT(" "));
                    StrCat(pArguments, W2T(pObjectClass));

                    Trace(TEXT("Executing: %s"), pItem->pCommand);
                    Trace(TEXT("Arguments: %s"), pArguments);

                    ShellExecute(NULL, NULL, pItem->pCommand, pArguments, NULL, SW_SHOWNORMAL);
                    LocalFreeString(&pArguments);                    
                }
            }

            break;
        }

        id -= pItem->cAdded;
    }

    hres = (i < DSA_GetItemCount(_hdsaMenuItems)) ? S_OK:E_FAIL;

exit_gracefully:

    LocalFreeString(&pArguments);
    ReleaseStgMedium(&medium);

    TraceLeaveResult(hres);
}

/*---------------------------------------------------------------------------*/

STDMETHODIMP CDsPropertyPages::GetCommandString(UINT_PTR uID, UINT uFlags, UINT FAR* reserved, LPSTR pName, UINT ccNameMax)
{
    HRESULT hres;
    INT i;
    INT id = (INT)uID;

    TraceEnter(TRACE_UI, "CDsPropertyPages::GetCommandString");

    // Walk down the list of the menu items looking for one that matches the
    // item we are trying get the command string from.  If it is an IContextMenu
    // handler then we must call down to that.

    if ( !_hdsaMenuItems )
        ExitGracefully(hres, E_INVALIDARG, "No menu item DSA");

    for ( i = 0 ; i < DSA_GetItemCount(_hdsaMenuItems) ; i++ )
    {
        LPDSMENUITEM pItem = (LPDSMENUITEM)DSA_GetItemPtr(_hdsaMenuItems, i);
        TraceAssert(pItem);

        Trace(TEXT("id %08x, cAdded %d"), id, pItem->cAdded);
        
        if ( id < pItem->cAdded )
        {
            if ( pItem->pContextMenu )
            {
                hres = pItem->pContextMenu->GetCommandString(id, uFlags, reserved, pName, ccNameMax);
                FailGracefully(hres, "Failed when calling context menu handler (GetCommandString)");
            }
            else
            {
                if ( uFlags != GCS_HELPTEXT )
                    ExitGracefully(hres, E_FAIL, "We only respond to GCS_HELPTEXT");

                Trace(TEXT("GCS_HELPTEXT returns for non-IContextMenu item: %s"), pItem->pCaption);
                lstrcpyn((LPTSTR)pName, pItem->pCaption, ccNameMax);               
            }

            break;
        }

        id -= pItem->cAdded;
    }        

    hres = (i < DSA_GetItemCount(_hdsaMenuItems)) ? S_OK:E_FAIL;

exit_gracefully:

    TraceLeaveResult(hres);
}


/*----------------------------------------------------------------------------
/ IObjectWithSite
/----------------------------------------------------------------------------*/

STDMETHODIMP CDsPropertyPages::SetSite(IUnknown* punk)
{
    HRESULT hres = S_OK;

    TraceEnter(TRACE_UI, "CDsPropertyPages::SetSite");

    DoRelease(_punkSite);

    if ( punk )
    {
        TraceMsg("QIing for IUnknown from the site object");

        hres = punk->QueryInterface(IID_IUnknown, (void **)&_punkSite);
        FailGracefully(hres, "Failed to get IUnknown from the site object");
    }

exit_gracefully:

    TraceLeaveResult(hres);
}

/*---------------------------------------------------------------------------*/

STDMETHODIMP CDsPropertyPages::GetSite(REFIID riid, void **ppv)
{
    HRESULT hres;
    
    TraceEnter(TRACE_UI, "CDsPropertyPages::GetSite");

    if ( !_punkSite )
        ExitGracefully(hres, E_NOINTERFACE, "No site to QI from");

    hres = _punkSite->QueryInterface(riid, ppv);
    FailGracefully(hres, "QI failed on the site unknown object");

exit_gracefully:

    TraceLeaveResult(hres);
}


/*-----------------------------------------------------------------------------
/ CWABDataObject
/----------------------------------------------------------------------------*/

CWABDataObject::CWABDataObject(LPWSTR pDN)
{
    USES_CONVERSION;

    TraceEnter(TRACE_WAB, "CWABDataObject::CWABDataObject");
    
    TraceAssert(_pPath==NULL);
    TraceAssert(_pDsObject==NULL);

    if ( SUCCEEDED(LocalAllocStringLenW(&_pPath, lstrlenW(pDN)+7)) )
    {
        StrCpyW(_pPath, L"LDAP://");
        StrCatW(_pPath, pDN);
        Trace(TEXT("DN converted to an ADSI path: %s"), W2T(_pPath));
    }

    TraceLeave();
}

CWABDataObject::~CWABDataObject()
{
    TraceEnter(TRACE_WAB, "CWABDataObject::~CWABDataObject");

    LocalFreeStringW(&_pPath);
    DoRelease(_pDsObject);

    TraceLeave();
}

// IUnknown

#undef CLASS_NAME
#define CLASS_NAME CWABDataObject
#include "unknown.inc"

STDMETHODIMP CWABDataObject::QueryInterface(REFIID riid, LPVOID* ppvObject)
{
    INTERFACES iface[]=
    {
        &IID_IDataObject, (LPDATAOBJECT)this,
    };

    return HandleQueryInterface(riid, ppvObject, iface, ARRAYSIZE(iface));
}


/*-----------------------------------------------------------------------------
/ IDataObject methods
/----------------------------------------------------------------------------*/

STDMETHODIMP CWABDataObject::GetData(FORMATETC* pFmt, STGMEDIUM* pMedium)
{
    HRESULT hres;
    BSTR bstrObjectClass = NULL;
    DWORD cbStruct = SIZEOF(DSOBJECTNAMES);
    DWORD offset = SIZEOF(DSOBJECTNAMES);
    LPDSOBJECTNAMES pDsObjectNames = NULL;
    CLASSCACHEGETINFO ccgi = { 0 };
    CLASSCACHEENTRY *pcce = NULL;
    USES_CONVERSION;

    TraceEnter(TRACE_WAB, "CWABDataObject::GetData");

    if ( !g_cfDsObjectNames )
        ExitGracefully(hres, E_FAIL, "g_cfDsObjectNames == NULL, therefore GetData cannot work");

    if ( !_pPath )
        ExitGracefully(hres, E_FAIL, "No _pPath set in data object");

    if ( pFmt->cfFormat == g_cfDsObjectNames )
    {
        // do we have the ADsObject that represents this path yet?  If not then
        // lets grab it, but only do that once otherwise we will continually hit
        // the wire.

        if ( !_pDsObject )
        {
            Trace(TEXT("Caching IADs for %s"), W2T(_pPath));
            hres = ADsOpenObject(_pPath, NULL, NULL, ADS_SECURE_AUTHENTICATION, IID_IADs, (LPVOID*)&_pDsObject);
            FailGracefully(hres, "Failed to get IADs for ADsPath we have");
        }

        // lets allocate a storage medium, put in the only object we have
        // and then return that to the caller.

        hres = _pDsObject->get_Class(&bstrObjectClass);
        FailGracefully(hres, "Failed to get the class of the object");

        // we have the information we need so lets allocate the storage medium and 
        // return the DSOBJECTNAMES structure to the caller.

        cbStruct += StringByteSizeW(_pPath);
        cbStruct += StringByteSizeW(bstrObjectClass);

        hres = AllocStorageMedium(pFmt, pMedium, cbStruct, (LPVOID*)&pDsObjectNames);
        FailGracefully(hres, "Failed to allocate storage medium");

        pDsObjectNames->clsidNamespace = CLSID_MicrosoftDS;
        pDsObjectNames->cItems = 1;

        pDsObjectNames->aObjects[0].dwFlags = 0;

        // check to see if the object is a container, if it is then set the attributes
        // accordingly.

        ccgi.dwFlags = CLASSCACHE_CONTAINER|CLASSCACHE_TREATASLEAF;
        ccgi.pPath = _pPath;
        ccgi.pObjectClass = bstrObjectClass;

        hres = ClassCache_GetClassInfo(&ccgi, &pcce);
        if ( SUCCEEDED(hres) )
        {
            if ( _IsClassContainer(pcce, FALSE) ) 
            {
                TraceMsg("Flagging the object as a container");
                pDsObjectNames->aObjects[0].dwFlags |= DSOBJECT_ISCONTAINER;
            }
            
            ClassCache_ReleaseClassInfo(&pcce);
        }

        pDsObjectNames->aObjects[0].dwProviderFlags = 0;

        pDsObjectNames->aObjects[0].offsetName = offset;
        StringByteCopyW(pDsObjectNames, offset, _pPath);
        offset += StringByteSizeW(_pPath);

        pDsObjectNames->aObjects[0].offsetClass = offset;
        StringByteCopyW(pDsObjectNames, offset, bstrObjectClass);
        offset += StringByteSizeW(bstrObjectClass);
    }
    else if ( pFmt->cfFormat == g_cfDsDispSpecOptions )
    {
        PDSDISPLAYSPECOPTIONS pOptions;
        DWORD cbSize = SIZEOF(DSDISPLAYSPECOPTIONS)+StringByteSizeW(DS_PROP_SHELL_PREFIX);

        // return the display spec options so we can indicate that WAB is involved
        // in the menus.

        hres = AllocStorageMedium(pFmt, pMedium, cbSize, (LPVOID*)&pOptions);
        FailGracefully(hres, "Failed to allocate the storage medium");

        pOptions->dwSize = cbSize;
        pOptions->dwFlags = DSDSOF_INVOKEDFROMWAB;                      // invoked from WAB however
        pOptions->offsetAttribPrefix = SIZEOF(DSDISPLAYSPECOPTIONS);
        StringByteCopyW(pOptions, pOptions->offsetAttribPrefix, DS_PROP_SHELL_PREFIX);
    }
    else 
    {
        ExitGracefully(hres, DV_E_FORMATETC, "Bad format passed to GetData");
    }

    hres = S_OK;              // success

exit_gracefully:

    if ( FAILED(hres) )
        ReleaseStgMedium(pMedium);

    SysFreeString(bstrObjectClass);

    TraceLeaveResult(hres);
}

/*---------------------------------------------------------------------------*/

STDMETHODIMP CWABDataObject::GetDataHere(FORMATETC* pFmt, STGMEDIUM* pMedium)
{
    TraceEnter(TRACE_WAB, "CWABDataObject::GetDataHere");
    TraceLeaveResult(E_NOTIMPL);
}

/*---------------------------------------------------------------------------*/

STDMETHODIMP CWABDataObject::QueryGetData(FORMATETC* pFmt)
{
    TraceEnter(TRACE_WAB, "CWABDataObject::QueryGetData");
    TraceLeaveResult(E_NOTIMPL);
}

/*---------------------------------------------------------------------------*/

STDMETHODIMP CWABDataObject::GetCanonicalFormatEtc(FORMATETC* pFmtIn, FORMATETC *pFmtOut)
{
    TraceEnter(TRACE_WAB, "CWABDataObject::GetCanonicalFormatEtc");
    TraceLeaveResult(E_NOTIMPL);
}

/*---------------------------------------------------------------------------*/

STDMETHODIMP CWABDataObject::SetData(FORMATETC* pFormatEtc, STGMEDIUM* pMedium, BOOL fRelease)
{
    TraceEnter(TRACE_WAB, "CWABDataObject::SetData");
    TraceLeaveResult(E_NOTIMPL);
}

/*---------------------------------------------------------------------------*/

STDMETHODIMP CWABDataObject::EnumFormatEtc(DWORD dwDirection, IEnumFORMATETC** ppEnumFormatEtc)
{
    TraceEnter(TRACE_WAB, "CWABDataObject::EnumFormatEtc");
    TraceLeaveResult(E_NOTIMPL);
}

/*---------------------------------------------------------------------------*/

STDMETHODIMP CWABDataObject::DAdvise(FORMATETC* pFormatEtc, DWORD advf, IAdviseSink* pAdvSink, DWORD* pdwConnection)
{
    TraceEnter(TRACE_WAB, "CWABDataObject::DAdvise");
    TraceLeaveResult(E_NOTIMPL);
}

/*---------------------------------------------------------------------------*/

STDMETHODIMP CWABDataObject::DUnadvise(DWORD dwConnection)
{
    TraceEnter(TRACE_WAB, "CWABDataObject::DUnadvise");
    TraceLeaveResult(E_NOTIMPL);
}

/*---------------------------------------------------------------------------*/

STDMETHODIMP CWABDataObject::EnumDAdvise(IEnumSTATDATA** ppenumAdvise)
{
    TraceEnter(TRACE_WAB, "CWABDataObject::EnumDAdvise");
    TraceLeaveResult(E_NOTIMPL);
}
