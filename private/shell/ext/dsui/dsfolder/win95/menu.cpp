/*----------------------------------------------------------------------------
/ Title;
/   menu.cpp
/
/ Authors;
/   David De Vorchik (daviddv)
/
/ Notes;
/   This code implements the context menu handler for the down level 
/   clients, its is based on the defcm code, but is must simplier, it
/   only allows certain static verbs and supports some of the
/   basic callback functions.
/
/   We originally explored calling the original defcm code from here, but
/   it has become so intertwined with the shell architecture (suprise)
/   that this works easier.
/----------------------------------------------------------------------------*/
#include "pch.h"
#include "dlshell.h"
#include "shguidp.h"
#pragma hdrstop


/*-----------------------------------------------------------------------------
/ Locals & helper functions
/----------------------------------------------------------------------------*/

#define CM_MAX_HKEY 16

class CMyDefContextMenu : public CUnknown, IContextMenu
{
    private:
        LPCITEMIDLIST _pidlFolder;
        HWND _hwndOwner;
        IDataObject* _pDataObject;
        LPSHELLFOLDER _psf;
        LPFNDFMCALLBACK _lpfn;
        UINT _nKeys;
        HKEY _ahKeys[CM_MAX_HKEY];

        HDXA _hdxa;

        UINT _idFirst;              // base of all verbs
        UINT _idStdMax;             // std edit verbs (cut, copy, paste etc)
        UINT _idFolderMax;          // verbs added by the folder
        UINT _idVerbMax;            // extensions loaded

        HRESULT _invokeVerb(UINT idCmd, LPCMINVOKECOMMANDINFO pCMIEx);
        HRESULT _invokeViewVerb(UINT idCmd, LPCMINVOKECOMMANDINFO pCMIEx);
        HRESULT _cmdstrViewVerb(UINT uID, UINT uType, UINT FAR* pReserved, LPSTR pszName, UINT cchMax);

    public:
        CMyDefContextMenu(LPCITEMIDLIST pidlFolder, HWND hwndOwner,
                          IDataObject* pDataObject,
                          LPSHELLFOLDER psf, LPFNDFMCALLBACK lpfn,
                          UINT nKeys, const HKEY *ahkeyClsKeys);
        ~CMyDefContextMenu();

        // IUnknown
        STDMETHOD(QueryInterface)(REFIID riid, LPVOID* ppvObject);
        STDMETHOD_(ULONG, AddRef)();
        STDMETHOD_(ULONG, Release)();

        // IContextMenu
        STDMETHODIMP QueryContextMenu(HMENU hMenu, UINT uIndex, UINT uIDFirst, UINT uIDLast, UINT uFlags);
        STDMETHODIMP InvokeCommand(LPCMINVOKECOMMANDINFO pCMI);
        STDMETHODIMP GetCommandString(UINT uID, UINT uType, UINT FAR* pReserved, LPSTR pszName, UINT cchMax);
};

struct
{
    UINT idDFMCmd;
    UINT idDefCmd;
    UINT idHelp;
    LPTSTR pVerb;
}
cmd_map[] =
{
    DFM_CMD_LINK,       SHARED_FILE_LINK,       IDS_LINK,       TEXT("link"),
    DFM_CMD_PROPERTIES, SHARED_FILE_PROPERTIES, IDS_PROPERTIES, TEXT("properties"),
};


/*-----------------------------------------------------------------------------
/ MyDefFolderMenuCreate
/----------------------------------------------------------------------------*/

EXTERN_C HRESULT MyDefFolderMenu(LPCITEMIDLIST pidlFolder, 
                                 HWND hwndOwner, 
                                 UINT cidl, LPCITEMIDLIST * apidl,
                                 LPSHELLFOLDER psf, LPFNDFMCALLBACK lpfn,
                                 UINT nKeys, const HKEY *ahkeyClsKeys,
                                 LPCONTEXTMENU *ppcm)
{   
    HRESULT hr;
    CMyDefContextMenu* pContextMenu = NULL;
    IDataObject* pDataObject = NULL;
   
    TraceEnter(TRACE_MYCONTEXTMENU, "MyDefFolderMenu");

    IDLData_InitializeClipboardFormats();

    if ( cidl )
    {
        hr = psf->GetUIObjectOf(hwndOwner, cidl, apidl, IID_IDataObject, NULL, (void**)&pDataObject);
        FailGracefully(hr, "Failed to construct the IDataObject");
    }

    pContextMenu = new CMyDefContextMenu(pidlFolder, hwndOwner, pDataObject, psf, lpfn, nKeys, ahkeyClsKeys);
    TraceAssert(pContextMenu);

    if ( !pContextMenu )
        ExitGracefully(hr, E_OUTOFMEMORY, "Failed to allocate context menu");

    hr = pContextMenu->QueryInterface(IID_IContextMenu, (void**)ppcm);

exit_gracefully:

    DoRelease(pContextMenu);
    DoRelease(pDataObject);

    TraceLeaveResult(hr);
}


/*-----------------------------------------------------------------------------
/ CMyDefContextMenu
/----------------------------------------------------------------------------*/

CMyDefContextMenu::CMyDefContextMenu(LPCITEMIDLIST pidlFolder, HWND hwndOwner, IDataObject* pDataObject,
                                         LPSHELLFOLDER psf, LPFNDFMCALLBACK lpfn, UINT nKeys, const HKEY *ahkeyClsKeys) :
    _pidlFolder(pidlFolder),
    _hwndOwner(hwndOwner),
    _pDataObject(pDataObject),
    _psf(psf),
    _lpfn(lpfn),
    _nKeys(0),
    _hdxa(NULL),
    _idFirst(0),
    _idStdMax(0),
    _idFolderMax(0),
    _idVerbMax(0)
{
    UINT n;

    TraceEnter(TRACE_MYCONTEXTMENU, "CMyDefContextMenu::CMyDefContextMenu");

    // take ownership of the objects we are given, and ensure our refcount > 0

    if ( _psf )
        _psf->AddRef();

    if ( _pDataObject )
        _pDataObject->AddRef();

    this->AddRef();

    // we are passed an array of HKEYs, so lets ensure we take copies of those
    // by re-opening them.

    Trace(TEXT("Copying %d keys to private array"), nKeys);

    for ( n = 0 ; (n < nKeys) && (n < CM_MAX_HKEY) ; n++ )
    {
        if ( ahkeyClsKeys[n] )
        {
            if ( RegOpenKeyEx(ahkeyClsKeys[n], NULL, 0L, MAXIMUM_ALLOWED, &_ahKeys[_nKeys]) == ERROR_SUCCESS )
                _nKeys++;
        }
    }

    Trace(TEXT("_nKeys is now %d"), _nKeys);

    TraceLeave();
}

CMyDefContextMenu::~CMyDefContextMenu()
{
    UINT n;

    TraceEnter(TRACE_MYCONTEXTMENU, "CMyDefContextMenu::~CMyDefContextMenu");

    DoRelease(_psf);
    DoRelease(_pDataObject);
    HDXA_Destroy(_hdxa);

    for ( n = 0 ; n < _nKeys ; n++ )
        RegCloseKey(_ahKeys[n]);

    TraceLeave();
}

// IUnknown

#undef CLASS_NAME
#define CLASS_NAME CMyDefContextMenu
#include "unknown.inc"

STDMETHODIMP CMyDefContextMenu::QueryInterface(REFIID riid, LPVOID* ppvObject)
{
    INTERFACES iface[]=
    {
        &IID_IContextMenu, (LPCONTEXTMENU)this,
    };

    return HandleQueryInterface(riid, ppvObject, iface, ARRAYSIZE(iface));
}


/*-----------------------------------------------------------------------------
/ IContextMenu
/----------------------------------------------------------------------------*/

STDMETHODIMP CMyDefContextMenu::QueryContextMenu(HMENU hMenu, UINT uIndex, UINT uIDFirst, UINT uIDLast, UINT uFlags)
{
    HRESULT hr;
    HDCA hdca = NULL;
    QCMINFO qcm = { hMenu, uIndex, uIDFirst, uIDLast };
    UINT n;

    TraceEnter(TRACE_MYCONTEXTMENU, "CMyDefContextMenu::QueryContextMenu");

    // initialize the menu ready for us to insert the items we have, load the
    // base menu and merge that, this has some ID's which we don't want to lose

    _idFirst = uIDFirst;

    if ( _pDataObject && !(uFlags & (CMF_VERBSONLY | CMF_DVFILE)) )
    {
        HMENU hSelMenu = LoadMenu(GLOBAL_HINSTANCE, MAKEINTRESOURCE(IDR_SELECTION_MENU));
        TraceAssert(hSelMenu);

        if ( hSelMenu )
        {
            qcm.idCmdFirst = Shell_MergeMenus(hMenu, GetSubMenu(hSelMenu, 0),  
                                              uIndex, uIDFirst, uIDLast,  
                                              MM_ADDSEPARATOR | MM_SUBMENUSHAVEIDS | MM_DONTREMOVESEPS);
            DestroyMenu(hSelMenu);
        }
    }

    _idStdMax = qcm.idCmdFirst - _idFirst;

    if ( _lpfn )
        _lpfn(_psf, _hwndOwner, _pDataObject, DFM_MERGECONTEXTMENU, uFlags, (LPARAM)&qcm);

    _idFolderMax = qcm.idCmdFirst - _idFirst;

    // add the verbs if required

    if ( !(uFlags & CMF_NOVERBS) )
    {
        MENUITEMINFO mii = { 0 };

        // ensure that we have a DXA and DCA to store the information about the extensions 
        // we are going to invoke/load.

        _hdxa = HDXA_Create();
        TraceAssert(_hdxa);

        hdca = DCA_Create();
        TraceAssert(hdca);

        if ( !_hdxa || !hdca )
            ExitGracefully(hr, E_OUTOFMEMORY, "Failed to create the HDXA");
        
        // put a seperator in at before the verbs (if needed)

        mii.cbSize = SIZEOF(mii);
        mii.fMask = MIIM_TYPE;
        mii.fType = MFT_SEPARATOR;              // to avoid ramdom result.

        if ( GetMenuItemInfo(hMenu, uIndex, TRUE, &mii) && !(mii.fType & MFT_SEPARATOR) )
            InsertMenu(hMenu, uIndex, MF_BYPOSITION | MF_SEPARATOR, (UINT)-1, NULL);

        // now get the list of extensions and merge those into the menu, CLSID_ShellFileDefExt
        // if the default extenion handler which handles the "shell" subkey.

        DCA_AddItem(hdca, CLSID_ShellFileDefExt);

        for ( n = 0; n < _nKeys; n++ )
            DCA_AddItemsFromKey(hdca, _ahKeys[n], STRREG_SHEX_MENUHANDLER);

        qcm.idCmdFirst = HDXA_AppendMenuItems2(_hdxa, _pDataObject, 
                                                _nKeys, _ahKeys, 
                                                _pidlFolder,
                                                &qcm, uFlags, hdca, NULL);
        // set the default menu item

        if ( _pDataObject && !(uFlags & CMF_NODEFAULT) &&
                  GetMenuDefaultItem(hMenu, MF_BYPOSITION, 0) == -1)
        {
            UINT idStatic;

            // we are about to set the default menu id, give the callback a chance
            // to override and set one of the static entries instead of the
            // first entry in the menu.

            if ( _lpfn && SUCCEEDED(_lpfn(_psf, _hwndOwner, _pDataObject, DFM_GETDEFSTATICID,  0, (LPARAM)&idStatic)))
            {
                for ( int i = 0; i < ARRAYSIZE(cmd_map); i++ )
                {
                    if ( idStatic == cmd_map[i].idDFMCmd )
                    {
                        SetMenuDefaultItem(hMenu, uIDFirst+cmd_map[i].idDefCmd, MF_BYCOMMAND);
                        break;
                    }
                }
            }

            if ( GetMenuDefaultItem(hMenu, MF_BYPOSITION, 0) == -1 )
                SetMenuDefaultItem(hMenu, MF_BYPOSITION, 0);
        }
    }

    _idVerbMax = qcm.idCmdFirst - _idFirst;

    Trace(TEXT("%d items added to the menu"), qcm.idCmdFirst - _idFirst);
    hr = ResultFromShort(qcm.idCmdFirst - _idFirst);

exit_gracefully:

    if ( hdca )
        DCA_Destroy(hdca);

    TraceLeaveResult(hr);
}

//-----------------------------------------------------------------------------

//
// Invoke verbs from an extension object, pass the verb string if the HIWORD
// is non-zero.
//

HRESULT CMyDefContextMenu::_invokeVerb(UINT idCmd, LPCMINVOKECOMMANDINFO pCMI)
{
    HRESULT hr;
    CMINVOKECOMMANDINFOEX ici;

    TraceEnter(TRACE_MYCONTEXTMENU, "CMyDefContextMenu::__invokeVerb");

    CopyInvokeInfo(&ici, pCMI);

    if ( !HIWORD(pCMI->lpVerb) )
        ici.lpVerb = (LPSTR)MAKEINTRESOURCE(idCmd);

    if ( IDCMD_PROCESSED != HDXA_LetHandlerProcessCommand(_hdxa, &ici) )
        ExitGracefully(hr, E_INVALIDARG, "Failed to execute verb via the _hdxa function");            

    hr = S_OK;

exit_gracefully:

    TraceLeaveResult(hr);
}

//
// Handle either default verbs or the ones which should be handled by the
// folder cb.
//

HRESULT CMyDefContextMenu::_invokeViewVerb(UINT idCmd, LPCMINVOKECOMMANDINFO pCMI)
{
    HRESULT hr;
    DFMICS dfmics;
    LPARAM lParam = (LPARAM)pCMI->lpParameters;

    TraceEnter(TRACE_MYCONTEXTMENU, "CMyDefContextMenu::_invokeViewVerb");

    // remap some verbs as required, and set lParam as needed for calling the
    // non-EX version of the callback.  lParam == the parameters already,
    // so most cases are already covered.

    switch ( idCmd )
    {
        case DFM_CMD_LINK:
        {
            lParam = (LPARAM)pCMI->lpDirectory;
            break;
        }
    }

    // call the callback, if they don't support the EX reason code then fall
    // back to the real one.

    dfmics.cbSize = SIZEOF(dfmics);
    dfmics.fMask = pCMI->fMask;
    dfmics.lParam = lParam;
    dfmics.idCmdFirst = _idFirst;
    dfmics.idDefMax = _idStdMax;

// HACK alert: (dli) This is a hack for the property pages to show up right at
// the POINT where they were activated. 

    if ( (pCMI->cbSize >= SIZEOF(CMINVOKECOMMANDINFOEX)) &&
            (idCmd == DFM_CMD_PROPERTIES) && 
                (pCMI->fMask & CMIC_MASK_PTINVOKE) 
                    && _pDataObject )
    {
        LPCMINVOKECOMMANDINFOEX pCMIEx = (LPCMINVOKECOMMANDINFOEX)pCMI;
        LPPOINT ppt;

        ppt = (LPPOINT)GlobalAlloc(GPTR, SIZEOF(POINT));
        TraceAssert(ppt);            

        if ( ppt )
        {
            *ppt = pCMIEx->ptInvoke;
            if (SUCCEEDED(DataObj_SetGlobal(_pDataObject, g_cfOFFSETS, ppt)) )
            {
                TraceMsg("Failed to set POINT into Dataobject, so free'ing structure");
                GlobalFree(ppt);
            }
        }
    }

    // try the callback, calling DFM_INVOKECOMMANDEX with a structure, if thats not supported
    // then fall back to using the original method with a lParam == a internesting parameter.

    hr = _lpfn(_psf, _hwndOwner, _pDataObject, DFM_INVOKECOMMANDEX, idCmd, (LPARAM)&dfmics);

    if ( hr == E_NOTIMPL )
    {
        TraceMsg("DFM_INVOKECOMMANDEX not supported, trying non-EX variant");
        hr = _lpfn(_psf, _hwndOwner, _pDataObject, DFM_INVOKECOMMAND, idCmd, lParam);
    }

    // if we get a S_FALSE back the handler didn't take the message so we must attempt
    // to invoke it instead, so lets do our default handling.

    if ( hr == S_FALSE )
    {
        if ( !_pDataObject )
            ExitGracefully(hr, E_INVALIDARG, "No IDataObject to invoke using");

        switch ( idCmd )
        {
// BUGBUG: daviddv - we only do DFM_CMD_LINK as thats all thats needed currently.            
            case DFM_CMD_LINK:
            {
                SHCreateLinks(pCMI->hwnd, NULL, _pDataObject, 
                              lParam ? SHCL_USETEMPLATE | SHCL_USEDESKTOP : SHCL_USETEMPLATE, 
                              NULL);
                break;
            }

            default:
                ExitGracefully(hr, E_INVALIDARG, "Ignorning the DFM_CMD not handled by the folderCB");
                
        }   

        hr = S_OK;          // success
    }
   
exit_gracefully:

    TraceLeaveResult(hr);
}
    
STDMETHODIMP CMyDefContextMenu::InvokeCommand(LPCMINVOKECOMMANDINFO pCMI)
{
    HRESULT hr;
    UINT idCmd;
    INT i;

    TraceEnter(TRACE_MYCONTEXTMENU, "CMyDefContextMenu::InvokeCommand");

    // is this a verb string or something we can just invoke as a command ID?
    // unfortunately we need to handle the command string case for properties
    // and link creation from the toolbar.

    if ( HIWORD(pCMI->lpVerb) )
    {
        LPCTSTR pVerb = pCMI->lpVerb;
        TraceAssert(pVerb);

        // check our table of command strings, if we get a hit then lets invoke the folder
        // verb based on it.

        for ( i = 0 ; i < ARRAYSIZE(cmd_map) ; i++ )
        {
            if ( !lstrcmpi(pVerb, cmd_map[i].pVerb) )
            {
                TraceMsg("Invoking a std verb from its command string");
                hr = _invokeViewVerb(cmd_map[i].idDFMCmd, pCMI);
                goto exit_gracefully;
            }
        }

        // check the callback, if it wants it then invoke from there

        if ( *pVerb && _lpfn )
        {
            if ( SUCCEEDED(_lpfn(_psf, _hwndOwner, _pDataObject, DFM_MAPCOMMANDNAME, (WPARAM)&idCmd, (LPARAM)pVerb)) )
            {
                TraceMsg("Invoking a folder verb from its command string (after cb)");
                hr = _invokeViewVerb(idCmd, pCMI);
                goto exit_gracefully;
            }
        }

        // looks like its a verb we need to invoke, so call an extension

        hr = _invokeVerb(0, pCMI);
        goto exit_gracefully;
    }
    else
    {
        idCmd = LOWORD(pCMI->lpVerb);

        if ( idCmd < _idStdMax )
        {
            // its in the range of the standard verbs so lets look and
            // see if we can convert it to an internal command ID
            // and invoked accordingly.

            for ( i = 0 ; i < ARRAYSIZE(cmd_map) ; i++ )
            {
                if ( idCmd == cmd_map[i].idDefCmd )
                {
                    TraceMsg("Invoking a std verb");
                    hr = _invokeViewVerb(cmd_map[i].idDFMCmd, pCMI);
                    goto exit_gracefully;
                }
            }

            ExitGracefully(hr, E_INVALIDARG, "Didn't map to a standard verb");
        }
        else if ( idCmd < _idFolderMax )
        {
            TraceMsg("Invoking a folder added verb");
            hr = _invokeViewVerb(idCmd-_idStdMax, pCMI);
            goto exit_gracefully;
        }
        else if (idCmd < _idVerbMax )
        {
            TraceMsg("Invoking an extension verb");
            hr = _invokeVerb(idCmd - _idFolderMax, pCMI);
            FailGracefully(hr, "Failed to invoke the verb");
        }
        else
        {
            ExitGracefully(hr, E_INVALIDARG, "Out of range idCMD, ignoring");
        }
    }

    hr = S_OK;

exit_gracefully:

    TraceLeaveResult(hr);
}

//-----------------------------------------------------------------------------

HRESULT CMyDefContextMenu::_cmdstrViewVerb(UINT uID, UINT uType, UINT FAR* pReserved, LPSTR pszName, UINT cchMax)
{
    HRESULT hr;

    TraceEnter(TRACE_MYCONTEXTMENU, "CMyDefContextMenu::_cmdstrViewVerb");

    if ( !_lpfn )
        ExitGracefully(hr, E_INVALIDARG, "No folder callback to call");

    switch ( uType )
    {
        case GCS_HELPTEXTA:
            hr = _lpfn(_psf, _hwndOwner, _pDataObject, DFM_GETHELPTEXT, (WPARAM)MAKELONG(uID, cchMax), (LPARAM)pszName);
            break;

        case GCS_HELPTEXTW:
            hr = _lpfn(_psf, _hwndOwner, _pDataObject, DFM_GETHELPTEXTW, (WPARAM)MAKELONG(uID, cchMax), (LPARAM)pszName);
            break;

        case GCS_VALIDATEA:
        case GCS_VALIDATEW:
            hr = _lpfn(_psf, _hwndOwner, _pDataObject, DFM_VALIDATECMD, uID, 0);
            break;

        default:
            ExitGracefully(hr, E_INVALIDARG, "Unexpected reason code");
    }

exit_gracefully:

    TraceLeaveResult(hr);                      
}

STDMETHODIMP CMyDefContextMenu::GetCommandString(UINT uID, UINT uType, UINT FAR* pReserved, LPSTR pszName, UINT cchMax)
{
    HRESULT hr;
    INT i;

    TraceEnter(TRACE_MYCONTEXTMENU, "CMyDefContextMenu::GetCommandString");

    // HIWORD is non-zero then this is a verb string, so lets try and handle it

    if ( HIWORD(uID) )
    {
        // see if the extension code handles it first (it will barf if not)

        hr = HDXA_GetCommandString(_hdxa, uID, uType, pReserved, pszName, cchMax);
        if ( SUCCEEDED(hr) )
        {
            TraceMsg("Handled by extension handler");
            goto exit_gracefully;
        }

        // it was not handled by an extension so lets see if we can find it in our table
        // of verbs, and then pass it down to the folder CB.

        for ( i = 0 ; i < ARRAYSIZE(cmd_map) ; i++ )
        {
            if ( !lstrcmpi((LPTSTR)uID, cmd_map[i].pVerb) )
            {
                hr = _cmdstrViewVerb(cmd_map[i].idDFMCmd, uType, pReserved, pszName, cchMax);
                goto exit_gracefully;
            }
        }

        ExitGracefully(hr, E_INVALIDARG, "Bad verb string given");        
    }
    else
    {
        // the HIWORD was zereo, therefore the loword contains the verb ID
        // we need to invoke from.

        if ( uID < _idStdMax )
        {
            // convert the command ID to an index in the map table, this we can
            // then use for returning strings to the caller.

            for ( i = 0 ; i < ARRAYSIZE(cmd_map) ; i++ )
            {
                if ( cmd_map[i].idDefCmd == uID )
                    break;
            }

            if ( i >= ARRAYSIZE(cmd_map) )
                ExitGracefully(hr, E_INVALIDARG, "Failed to look up the command");

            // we have an index so lets do act on the type of message we rx'd.

            switch ( uType )
            {
                case GCS_HELPTEXT:
                {
                    if ( !LoadString(GLOBAL_HINSTANCE, cmd_map[i].idHelp, pszName, cchMax) )
                        ExitGracefully(hr, E_OUTOFMEMORY, "Failed when loading the string");

                    break;
                }
    
                case GCS_VERB:
                {
                    StrCmpN(pszName, cmd_map[i].pVerb, cchMax);
                    break;
                }
    
                case GCS_VALIDATEA:
                case GCS_VALIDATEW:
                default:
                {
                    ExitGracefully(hr, E_NOTIMPL, "Not implemented code for std verb");
                }
            }            
        } 
        else if ( uID < _idFolderMax )
        {
            hr = _cmdstrViewVerb(uID-_idStdMax, uType, pReserved, pszName, cchMax);
            goto exit_gracefully;
        }
        else if ( uID < _idVerbMax )
        {
            hr = HDXA_GetCommandString(_hdxa, uID-_idFolderMax, uType, pReserved, pszName, cchMax);
            goto exit_gracefully;
        }
        else
        {
            ExitGracefully(hr, E_INVALIDARG, "Command ID is invalid");
        }
    }

    hr = S_OK;

exit_gracefully:

    TraceLeaveResult(hr);
}
