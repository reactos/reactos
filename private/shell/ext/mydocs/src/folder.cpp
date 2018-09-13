/*----------------------------------------------------------------------------
/ Title;
/   folder.cpp
/
/ Authors;
/   Rick Turner (ricktu)
/
/ Notes;
/   IShellFolder implementation for My Documents.
/----------------------------------------------------------------------------*/
#include "precomp.hxx"
#pragma hdrstop

//#include "mdguids.h"

/*-----------------------------------------------------------------------------
/ Globals
/----------------------------------------------------------------------------*/

const GUID IID_INeedRealCFSFolder = { 0xABCB3A00L, 0x1B2B, 0x11CF,
                                      { 0xA4, 0x9F, 0x44, 0x45,
                                        0x53, 0x54, 0x00, 0x00
                                      }
                                    };

const GUID CLSID_ShellFSFolder    = { 0xF3364BA0L, 0x65B9, 0x11CE,
                                      { 0xA9, 0xBA, 0x00, 0xAA,
                                        0x00, 0x4A, 0xE8, 0x37
                                      }
                                    };

const GUID IID_IShellFolderViewCB = { 0x2047E320L, 0xF2A9, 0x11CE,
                                     { 0xAE, 0x65, 0x08, 0x00,
                                       0x2B, 0x2E, 0x12, 0x62
                                     }
                                    };

const GUID IID_IShellFolderView   = { 0x37A378C0L, 0xF82D, 0x11CE,
                                      { 0xAE, 0x65, 0x08, 0x00,
                                        0x2B, 0x2E, 0x12, 0x62
                                      }
                                    };

const GUID CLSID_ShellFileDefExt =  { 0x21B22460L, 0x3AEA, 0x1069,
                                      { 0xA2, 0xDC, 0x08, 0x00,
                                        0x2B, 0x30, 0x30, 0x9D
                                       }
                                     };

const GUID CLSID_SharingPage      = { 0xf81e9010L, 0x6ea4, 0x11ce,
                                      { 0xa7, 0xff, 0x00, 0xaa,
                                        0x00, 0x3c, 0xa9, 0xf6
                                       }
                                     };

void RegisterMyDocsClipboardFormats(void);

struct
{
    TCHAR szDetailString[ 50 ];
    UINT  idString;
}
details[] =
{
    TEXT("\0"), 0,
    TEXT("\0"), 0,
    TEXT("\0"), IDS_SPECIAL_TYPE,
    TEXT("\0"), 0,
    TEXT("\0"), 0,
};

HRESULT CreateMyDocsColumns(IShellFolder *psf, IShellFolderView* psfv, REFIID riid, void** ppvOut);

class CMyDocsColumns : public IShellDetails3, CUnknown
{
public:
    // IUnknown
    STDMETHOD(QueryInterface)(REFIID riid, LPVOID* ppvObject);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    // IShellDetails
    STDMETHOD(GetDetailsOf)(LPCITEMIDLIST pidl, UINT iColumn, LPSHELLDETAILS pDetails);
    STDMETHOD(ColumnClick)(UINT iColumn);
    
    // IShellDetails3
    STDMETHOD_(ULONG,GetDefaultSortColumn) ();
    STDMETHOD_(ULONG,GetDefaultDisplayColumn) ();
    STDMETHOD(GetDefaultColumnState)(UINT iColumn, DWORD *pdwState);
    STDMETHOD(GetDetailsEx)(LPCITEMIDLIST pidl, SHCOLUMNID *pscid, VARIANT *pv);
    STDMETHOD(MapNameToSCID)(LPCWSTR pwszName, SHCOLUMNID *pscid);

    CMyDocsColumns(IShellFolder* psf, IShellFolderView* psfv);
    ~CMyDocsColumns();

private:
    BOOL _GetShellDetails3();

    IShellFolder*      m_psf;
    IShellFolderView*  m_psfv;
    IShellDetails*     m_psd;
    IShellDetails3*    m_psd3;
};

CMyDocsColumns::CMyDocsColumns(IShellFolder* psf, IShellFolderView* psfv)
{
    m_psf = psf;
    if (m_psf)
        m_psf->AddRef();
    m_psfv = psfv;
    if (m_psfv)
        m_psfv->AddRef();
    m_psd = NULL;
    m_psd3 = NULL;
}

CMyDocsColumns::~CMyDocsColumns()
{
    if (m_psf)
        m_psf->Release();
    if (m_psfv)
        m_psfv->Release();
    if (m_psd)
        m_psd->Release();
    if (m_psd3)
        m_psd3->Release();
}

#undef CLASS_NAME
#define CLASS_NAME CMyDocsColumns
#include "unknown.inc"

STDMETHODIMP
CMyDocsColumns::QueryInterface( REFIID riid, LPVOID* ppvObject)
{
    MDTraceEnter( TRACE_QI, "CMyDocsColumns::QueryInterface" );
    MDTraceGUID("Interface requested", riid);

    INTERFACES iface[] =
    {
        &IID_IShellDetails,      (IShellDetails *)this,
        &IID_IShellDetails3,     (IShellDetails3 *)this,
    };

    return HandleQueryInterface(riid, ppvObject, iface, ARRAYSIZE(iface));
};

/*-----------------------------------------------------------------------------
/ IShellDetails methods
/----------------------------------------------------------------------------*/

STDMETHODIMP
CMyDocsColumns::GetDetailsOf( LPCITEMIDLIST pidl,
                             UINT iColumn,
                             LPSHELLDETAILS pDetails
                            )
{
    HRESULT hr;
    MDTraceEnter(TRACE_DETAILS, "CMyDocsColumns::GetDetailsOf");

    if (!m_psd)
    {
        hr = m_psf->CreateViewObject( NULL, IID_IShellDetails, (LPVOID *)&m_psd );
        FailGracefully( hr, "couldn't get IShellDetails from shell" );
    }

    if (!pidl)
    {
        hr = m_psd->GetDetailsOf( pidl, iColumn, pDetails );
        FailGracefully( hr, "failed getting column info from shell" );
    }
    else if (MDIsSpecialIDL((LPITEMIDLIST)pidl))
    {
        switch( iColumn + IDS_FILENAME )
        {
        case IDS_FILETYPE:
        case IDS_FILESIZE:
        case IDS_FILEMODIFIED:
        case IDS_FILEATTRIBUTES:
            if ((details[iColumn].szDetailString[0] == 0) &&
                (details[iColumn].idString)
                )
            {
                if ( !LoadString( g_hInstance,
                                  details[iColumn].idString,
                                  details[iColumn].szDetailString,
                                  ARRAYSIZE(details[iColumn].szDetailString)
                                 )
                   )
                {
                    ExitGracefully(hr, E_FAIL, "Failed to loadstring");
                }


            }

            hr = StrRetFromString( &pDetails->str,
                                   details[iColumn].szDetailString
                                  );
            break;

        default:
            hr = E_FAIL;

        }
    }
    else
    {
        hr = m_psd->GetDetailsOf( ILFindLastID(SHIDLFromMDIDL(pidl)),
                                  iColumn,
                                  pDetails
                                 );
    }


exit_gracefully:

    MDTraceLeaveResult(hr);
}

/*---------------------------------------------------------------------------*/
STDMETHODIMP
CMyDocsColumns::ColumnClick(UINT iColumn)
{
    HRESULT hr = NOERROR;

    MDTraceEnter( TRACE_DETAILS, "CMyDocsColumns::ColumnClick" );

    MDTraceAssert( m_psfv );
    if (m_psfv)
        hr = m_psfv->Rearrange( iColumn );


    MDTraceLeaveResult(hr);
}


// IShellDetails3 methods to forward
BOOL CMyDocsColumns::_GetShellDetails3()
{
    if (!m_psd3)
    {
        if (m_psd)
            m_psd->QueryInterface(IID_IShellDetails3, (void **) &m_psd3);
        else
            m_psf->CreateViewObject(NULL, IID_IShellDetails3, (void **)&m_psd3);
    }
    return (m_psd3 != NULL);
}

STDMETHODIMP CMyDocsColumns::GetDefaultColumnState(UINT iColumn, DWORD *pdwState)
{
    if (_GetShellDetails3())
        return m_psd3->GetDefaultColumnState(iColumn, pdwState);
    else
        return E_NOTIMPL;
}

STDMETHODIMP CMyDocsColumns::GetDetailsEx(LPCITEMIDLIST pidl, SHCOLUMNID *pscid, VARIANT *pv)
{
    if (_GetShellDetails3())
        return m_psd3->GetDetailsEx(pidl, pscid, pv);
    else
        return E_NOTIMPL;
}

STDMETHODIMP CMyDocsColumns::MapNameToSCID(LPCWSTR pwszName, SHCOLUMNID *pscid)
{
    if (_GetShellDetails3())
        return m_psd3->MapNameToSCID(pwszName, pscid);
    else
        return E_NOTIMPL;
}

// These two IShellDetails2 methods are never called from defview
ULONG CMyDocsColumns::GetDefaultSortColumn()
{
    if (m_psd3)
        return m_psd3->GetDefaultSortColumn();
    else
        return E_NOTIMPL;
}
ULONG CMyDocsColumns::GetDefaultDisplayColumn()
{
    if (m_psd3)
        return m_psd3->GetDefaultDisplayColumn();
    else
        return E_NOTIMPL;
}

HRESULT CreateMyDocsColumns(IShellFolder *psf, IShellFolderView* psfv, REFIID riid, void** ppvOut)
{
    HRESULT hres = E_INVALIDARG;
    *ppvOut = NULL;

    if (psf && psfv)
    {
        hres = E_OUTOFMEMORY;
        
        CMyDocsColumns* pmdc = new CMyDocsColumns(psf, psfv);

        if (pmdc)
        {
            hres = pmdc->QueryInterface(riid, ppvOut);
            if (FAILED(hres))
                delete pmdc;
        }
    }
    return hres;
}
/*-----------------------------------------------------------------------------
/ Callback functions used by this IShellFolder implementation
/----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
/ _GetKeysForIDL
/ -----------------
/   Returns registry keys associated with the given idlists...
/
/ In:
/   cidl    -> # of idliss in aidl
/   aidl    -> array of idlists
/   cKeys   -> size of aKeys
/   aKeys   -> array to hold retrieved registry keys
/
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/
HRESULT
_GetKeysForIDL( UINT cidl,
                LPCITEMIDLIST *aidl,
                UINT cKeys,
                HKEY *aKeys
               )
{
    HRESULT hr = S_OK;
    UINT i, cspec = 0, last = 0;
    TCHAR szBuffer[ MAX_PATH ];
    LONG lRes;

    MDTraceEnter( TRACE_FOLDER, "_GetKeysForIDL" );

    //
    // zero out the array of registry keys...
    //

    memset( (LPVOID)aKeys, 0, cKeys * sizeof(HKEY) );

    //
    // First, get generic menu keys for all special items...
    //

    lstrcpy( szBuffer, c_szCLSIDFormat );
    lstrcat( szBuffer, TEXT("\\") );
    lstrcat( szBuffer, c_szAllSpecial );

    MDTrace(TEXT("attempting to open: HKEY_CLASSES_ROOT\\%s"), szBuffer );
    lRes = RegOpenKeyEx( HKEY_CLASSES_ROOT, szBuffer, 0, KEY_READ, &aKeys[UIKEY_ALL] );
    if (lRes != NO_ERROR)
        ExitGracefully( hr, E_FAIL, "couldn't open All Items key" );

    //
    // Now, determine how many special items are in the list...
    //

    for (i = 0; i<cidl; i++)
    {
        if (MDIsSpecialIDL((LPITEMIDLIST)aidl[i]))
        {
            cspec++;
            last = i;
        }
    }

    //
    // If there's only one special item, then add it's specific keys to the
    // list...
    //

    if (cspec)
    {
        TCHAR szName[ MAX_PATH ];
        UINT cch = ARRAYSIZE(szName);


        hr = MDGetNameFromIDL( (LPITEMIDLIST)aidl[last], szName, &cch, FALSE );
        if (!SUCCEEDED(hr))
            ExitGracefully( hr, S_OK, "Unable to get name from special idlist" );

        lstrcpy( szBuffer, c_szCLSIDFormat );
        lstrcat( szBuffer, TEXT("\\") );
        lstrcat( szBuffer, c_szSpecificSpecial );
        lstrcat( szBuffer, szName );
        MDTrace(TEXT("attempting to open: HKEY_CLASSES_ROOT\\%s"), szBuffer );
        lRes = RegOpenKeyEx( HKEY_CLASSES_ROOT, szBuffer, 0, KEY_READ, &aKeys[UIKEY_SPECIFIC] );
        if (lRes != NO_ERROR)
            ExitGracefully( hr, S_OK, "couldn't open Specific Items key" );

    }

exit_gracefully:

    MDTraceLeaveResult(hr);

}

/*-----------------------------------------------------------------------------
/ _MergeArrangeMenu
/ -----------------
/   Merge our verbs into the view menu
/
/ In:
/   pInfo -> QCMINFO structure
/
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/
HRESULT
_MergeArrangeMenu( LPQCMINFO pInfo )
{
    MENUITEMINFO mii = { SIZEOF(MENUITEMINFO), MIIM_SUBMENU };
    UINT idCmdFirst = pInfo->idCmdFirst;
    HMENU hMyArrangeMenu;

    MDTraceEnter(TRACE_CALLBACKS, "_MergeArrangeMenu");
    MDTrace(TEXT("pInfo->idCmdFirst %08x"), pInfo->idCmdFirst);

    if ( GetMenuItemInfo(pInfo->hmenu, SFVIDM_MENU_ARRANGE, FALSE, &mii) )
    {
        hMyArrangeMenu = LoadMenu(GLOBAL_HINSTANCE, MAKEINTRESOURCE(IDR_ARRANGE));

        if ( hMyArrangeMenu )
        {
            pInfo->idCmdFirst = Shell_MergeMenus(mii.hSubMenu,
                                                 GetSubMenu(hMyArrangeMenu, 0),
                                                 0,
                                                 pInfo->idCmdFirst, pInfo->idCmdLast,
                                                 0);
            DestroyMenu(hMyArrangeMenu);
        }
    }

    MDTraceLeaveResult(S_OK);
}

/*-----------------------------------------------------------------------------
/ _FolderCFMCallBack
/ ------------------
/   Handles callbacks for the context menu which is displayed when the user
/   right clicks on objects within the view.
/
/ In:
/   psf -> shell folder (our object)
/   hwndView = window handle for a dialog parent
/   pDataObject -> data object for the menu
/   uMsg, wParam, lParam = message specific information
/
/ Out:
/   -
/----------------------------------------------------------------------------*/
HRESULT
CALLBACK
_FolderCFMCallback( LPSHELLFOLDER psf,
                    HWND hwndView,
                    LPDATAOBJECT pDataObject,
                    UINT uMsg,
                    WPARAM wParam,
                    LPARAM lParam
                   )
{
    HRESULT hr = NOERROR;

    MDTraceEnter( TRACE_CALLBACKS, "_FolderCFMCallback" );
    MDTraceMenuMsg( uMsg, wParam, lParam );

    switch ( uMsg )
    {
        case DFM_ADDREF:
            hr = psf->AddRef( );
            break;

        case DFM_RELEASE:
            hr = psf->Release( );
            break;

        case DFM_MERGECONTEXTMENU:
            hr = _MergeArrangeMenu((LPQCMINFO)lParam);
            break;

        case DFM_MAPCOMMANDNAME:
            hr = E_FAIL;
            break;

        case DFM_INVOKECOMMAND:
        {
            UINT idCmd = (UINT)wParam;

            switch ( idCmd )
            {
                case DFM_CMD_PROPERTIES:
                    break;

                case DFM_CMD_RENAME:
                    break;

                case DFM_CMD_DELETE:
                    break;

                case MDVMID_ARRANGEBYNAME:
                case MDVMID_ARRANGEBYDATE:
                case MDVMID_ARRANGEBYSIZE:
                case MDVMID_ARRANGEBYTYPE:
                    ShellFolderView_ReArrange(hwndView, idCmd);
                    break;

                default:
                    hr = S_FALSE;
                    break;
            }

            break;
        }

        default:
            hr = E_NOTIMPL;
            break;
    }

    MDTraceLeaveResult(hr);
}

/*-----------------------------------------------------------------------------
/ CMyDocsFolder
/   This is the My Documents IShellFolder implementation.
/----------------------------------------------------------------------------*/

CMyDocsFolder::CMyDocsFolder( )
{
    MDTraceEnter(TRACE_FOLDER, "CMyDocsFolder::CMyDocsFolder");

    m_path       = NULL;
    m_type       = FOLDER_IS_UNKNOWN;
    m_host       = APP_IS_UNKNOWN;
    m_pidl       = NULL;
    m_pidlReal   = NULL;
    m_psf        = NULL;
    m_punk       = NULL;
    m_psfv       = NULL;
    m_psfvcb     = NULL;
    //m_psd        = NULL;
    //m_psd3       = NULL;
    m_pseGeneral = NULL;
    m_pseSharing = NULL;
    m_HideCmd    = 0;
    m_ext        = EXT_IS_UNKNOWN;

    MDTraceLeave();
}

CMyDocsFolder::~CMyDocsFolder()
{
    LPUNKNOWN punk = NULL;
    HRESULT hr = E_FAIL;

    MDTraceEnter(TRACE_FOLDER, "CMyDocsFolder::~CMyDocsFolder");

    //
    // Artificially bump the refcount so the AddRefing and Releasing
    // doesn't cause the destructor to get called recursively.
    //
    m_cRefCount = 1000;

    hr = QueryInterface(IID_IUnknown, (LPVOID *) &punk);
    if (SUCCEEDED(hr)) {
        RELEASEINNERINTERFACE(punk, m_psf);
    }
    else {
        MDTrace(TEXT("Unable to get IUnknown for MyDocs.  Leaking 1 psf!"));
        punk = NULL;
    } // if

    DoILFree( m_pidl );
    DoILFree( m_pidlReal );
    LocalFreeString( &m_path );
    DoRelease( m_punk );
//    DoRelease( m_psfv );
    DoRelease( m_psfvcb );
//    DoRelease( m_psd );
//    DoRelease( m_psd3 );
    DoRelease( m_pseGeneral );
    DoRelease( m_pseSharing );

    DoRelease(punk);
    m_cRefCount = 0;

    MDTraceLeave();
}

/*---------------------------------------------------------------------------*/

#undef CLASS_NAME
#define CLASS_NAME CMyDocsFolder
#include "unknown.inc"

STDMETHODIMP
CMyDocsFolder::QueryInterface( REFIID riid, LPVOID* ppvObject)
{
    HRESULT hr;
    ITEMIDLIST idlEmpty;

    MDTraceEnter( TRACE_QI, "CMyDocsFolder::QueryInterface" );
    MDTraceGUID("Interface requested", riid);

    INTERFACES iface[] =
    {
        &IID_IShellFolder,       (LPSHELLFOLDER)this,
        &IID_IPersistFolder,     (LPPERSISTFOLDER)this,
        &IID_IShellFolderViewCB, (IShellFolderViewCB *)this,
        &IID_IShellDetails,      (IShellDetails *)this,
        &IID_IShellDetails3,     (IShellDetails3 *)this,
        &IID_IShellExtInit,      (IShellExtInit *)this,
        &IID_IContextMenu,       (IContextMenu *)this,
        &IID_IShellPropSheetExt, (IShellPropSheetExt *)this,
        &IID_IPersistFile,       (IPersistFile *)this,
    };

    //
    // Do the special cases first...
    //

    if (IsEqualIID( riid, IID_IExtractIcon))
    {

        LPITEMIDLIST pidl = NULL;

        // IExtractIcon is used to support our object extraction, we can only cope
        // with extracting a single object at a time.  First however we must
        // build an IDLIST that represents the object we want to extract
        // and get the icon for it.

        if ( (m_type == FOLDER_IS_ROOT) ||
             (m_type == FOLDER_IS_SENDTO)
            )
        {
            idlEmpty.mkid.cb = 0;
            idlEmpty.mkid.abID[0] = 0;

            pidl = &idlEmpty;
        }
        else if (m_psf)
        {
            pidl = m_pidl;
        }

        if (pidl)
        {
            CMyDocsExtractIcon* pExtractIcon = new CMyDocsExtractIcon( m_psf,
                                                                       NULL,
                                                                       pidl,
                                                                       m_type
                                                                      );

            if ( !pExtractIcon )
                ExitGracefully(hr, E_OUTOFMEMORY, "Failed to create CMyDocsExtractIcon");


            *ppvObject = (LPVOID)pExtractIcon;
            pExtractIcon->AddRef();
            ExitGracefully( hr, S_OK, "returning pExtract Icon for IID_IExtractIcon" );
        }
    }
    else if (IsEqualIID( riid, IID_IDropTarget) && m_psf)
    {
        //
        // Need to get IDropTarget for real My Docs folder...(this is for SendTo)
        //
        IShellFolder * psfDesktop = NULL;

        *ppvObject = NULL;
        hr = SHGetDesktopFolder( &psfDesktop );
        FailGracefully( hr, "couldn't get desktop IShellFolder" );

        LPCITEMIDLIST pidl = (LPCITEMIDLIST)ILClone( m_pidlReal );
        hr = psfDesktop->GetUIObjectOf( NULL, 1, &pidl, IID_IDropTarget, NULL, ppvObject );
        DoILFree( pidl );
        DoRelease( psfDesktop );

        FailGracefully( hr, "call to GetUIObjectOf on psfDesktop failed for IID_IDropTarget" );
        ExitGracefully( hr, S_OK, "Got IID_IDropTarget from shell" );
    }
    else if (IsEqualIID( riid, IID_IShellCopyHook))
    {
        CMyDocsCopyHook * pMDCH = new CMyDocsCopyHook();

        if ( !pMDCH )
            ExitGracefully(hr, E_OUTOFMEMORY, "Failed to allocate CMyDocsCopyHook");

        hr = pMDCH->QueryInterface(riid, ppvObject);

        if ( FAILED(hr) )
            delete pMDCH;

        FailGracefully( hr, "Call to CMyDocsCopyHook->QI failed" );
        ExitGracefully( hr, S_OK, "returning CMyDocsCopyHook for IID_IShellCopyHook" );
    }
    else if (IsEqualIID( riid, IID_INeedRealCFSFolder) && m_punk)
    {
        hr = m_punk->QueryInterface( IID_IUnknown, ppvObject );
        FailGracefully( hr, "Couldn't QI m_psf for IID_IUnknown" );
        ExitGracefully( hr, S_OK, "returning m_psf for IID_INeedRealCFSFolder" );
    }

    //
    // Next, try the normal cases...
    //

    hr = HandleQueryInterface(riid, ppvObject, iface, ARRAYSIZE(iface));

    //
    // Lastly, if we haven't succeeded yet, try the shell...
    //

    if (FAILED(hr) && m_punk)
    {
        //
        // Last ditch effort to query the punk for the real shell folder
        // for the given interface...
        //
        hr = m_punk->QueryInterface( riid, ppvObject );
        FailGracefully( hr, "Couldn't QI shell's punk for interface..." );

    }

exit_gracefully:

    MDTraceLeaveResult(hr);

}

/*---------------------------------------------------------------------------
/ BadApps
/ -------
/
/ Table which specifies window class names to watch out for
/
/---------------------------------------------------------------------------*/
const struct
{
    TCHAR szAppName[ 50 ];
    calling_app_type apptype;
}
BadApps[] =
{
    TEXT("bosa_sdm_Mso96"),                         APP_IS_OFFICE97,
    TEXT("bosa_sdm_Microsoft Word 8.0"),            APP_IS_OFFICE97,
    TEXT("bosa_sdm_XL8"),                           APP_IS_OFFICE97,
    TEXT("File Open Message Window"),               APP_IS_OFFICE97,
    TEXT("bosa_sdm_Microsoft Word for Windows 95"), APP_IS_OFFICE95,
    TEXT("bosa_sdm_XL"),                            APP_IS_OFFICE95,
    TEXT("bosa_sdm_Microsoft PowerPoint"),          APP_IS_OFFICE95,
    TEXT("bosa_sdm_Microsoft Access"),              APP_IS_OFFICE95,
    TEXT("bosa_sdm_Open"),                          APP_IS_OFFICE95,
    TEXT("bosa_sdm_Microsoft Office Binder"),       APP_IS_OFFICE95,
    TEXT("Progman"),                                APP_IS_SHELL,
    TEXT("\0"),                                     APP_IS_UNKNOWN,
};


/*---------------------------------------------------------------------------
/ CMyDocsFolder::WhoIsCalling
/ ---------------------------
/
/ Determine who is calling us so that we can do app specific
/ compatibility hacks when needed
/
/---------------------------------------------------------------------------*/
void CMyDocsFolder::WhoIsCalling()
{
    HWND hwnd;
    TCHAR szClass[ MAX_PATH ];
    INT i;

    MDTraceEnter( TRACE_FOLDER, "CMyDocsFolder::WhoIsCalling" );

    // Initialize
    szClass[0] = 0;

    // Check to see if we have the value already...
    if (m_host != APP_IS_UNKNOWN)
        goto exit_gracefully;

    hwnd = GetActiveWindow();
    if (!hwnd)
        goto exit_gracefully;

    if (!GetClassName( hwnd, szClass, ARRAYSIZE(szClass) ))
        goto exit_gracefully;

    MDTrace(TEXT("Class name of active window is -%s-"), szClass );

    // Check for bad applications...
    for (i = 0; BadApps[i].szAppName[0]; i++)
    {
        if (lstrcmp(szClass, BadApps[i].szAppName)==0)
        {
            m_host = BadApps[i].apptype;
            break;
        }
    }

    // If we didn't set m_host above, set it now...
    if (m_host == APP_IS_UNKNOWN)
    {
        MDTrace(TEXT("Setting host to NORMAL"));
        m_host = APP_IS_NORMAL;
    }

exit_gracefully:
#ifdef DEBUG
    if (szClass[0] == 0)
    {
        hwnd = GetActiveWindow();
        if (hwnd) GetClassName( hwnd, szClass, ARRAYSIZE(szClass) );
        MDTrace(TEXT("Class name of active window (%x) is -%s-"), hwnd, szClass );
    }

    if (m_host == APP_IS_SHELL)
    {
        MDTrace(TEXT("Returning host as SHELL"));
    }
    else if (m_host == APP_IS_OFFICE97)
    {
        MDTrace(TEXT("Returning host as OFFICE97"));
    }
    else if (m_host == APP_IS_OFFICE95)
    {
        MDTrace(TEXT("Returning host as OFFICE95"));
    }
    else if (m_host == APP_IS_COREL7)
    {
        MDTrace(TEXT("Returning host as COREL7"));
    }
    else if (m_host == APP_IS_NORMAL)
    {
        MDTrace(TEXT("Returning host as NORMAL"));
    }
    else if (m_host == APP_IS_UNKNOWN)
    {
        MDTrace(TEXT("Returning host as UNKNOWN"));
    }

#endif
    MDTraceLeave();

    return;
}

/*-----------------------------------------------------------------------------
/ IPersist methods
/----------------------------------------------------------------------------*/
STDMETHODIMP
CMyDocsFolder::GetClassID(LPCLSID pClassID)
{
    MDTraceEnter(TRACE_FOLDER, "CMyDocsFolder(IPersist)::GetClassID");

    MDTraceAssert(pClassID);
    *pClassID = CLSID_MyDocumentsExt;

    MDTraceLeaveResult(S_OK);
}

/*-----------------------------------------------------------------------------
/ IPersistFolder methods
/----------------------------------------------------------------------------*/

// Initialization we handle in two ways.  We assume that IPersistFolder::Initialize
// is called with an absolute IDLIST to the start of our name space, further to
// that (eg. when we bind to an object) we call an internal version.
//
// We assume this because we need to have an idea of how much to skip to get
// to the My Documents specific parts of an absolute IDLIST.

STDMETHODIMP
CMyDocsFolder::Initialize(LPCITEMIDLIST pidlStart)
{
    HRESULT hr;

    MDTraceEnter(TRACE_FOLDER, "CMyDocsFolder(IPersistFolder)::Initialize");

    hr = RealInitialize(pidlStart, NULL, NULL );
    FailGracefully(hr, "Failed when calling CMyDocsFolder::RealInitialize");

exit_gracefully:

    MDTraceLeaveResult(hr);
}

/*-----------------------------------------------------------------------------
/ IPersistFile methods
/----------------------------------------------------------------------------*/
STDMETHODIMP
CMyDocsFolder::IsDirty(void)
{
    MDTraceEnter( TRACE_FOLDER, "CMyDocsFolder(IPersistFile)::IsDirty (not implemented)" );
    MDTraceLeaveResult(E_NOTIMPL);
}

/*---------------------------------------------------------------------------*/
STDMETHODIMP
CMyDocsFolder::Load(LPCOLESTR pszFileName, DWORD dwMode)
{
    HRESULT hr = E_FAIL;
    LPITEMIDLIST pidl = NULL;
    BIND_OPTS bo;
    LPSHELLFOLDER psfDesktop = NULL;
    LPBC pbc = NULL;
    ULONG pcchEaten;
    TCHAR szPath[ MAX_PATH ];

    OleStrToStrN( szPath, ARRAYSIZE(szPath), pszFileName, -1 );

    MDTraceEnter( TRACE_FOLDER, "CMyDocsFolder(IPersistFile)::Load" );
    MDTrace(TEXT("pszFileName = %s"), szPath );



    memset( &bo, 0, sizeof(bo) );
    bo.cbStruct = sizeof(bo);
    bo.grfFlags = BIND_JUSTTESTEXISTENCE;

    hr = CreateBindCtx( 0, &pbc );
    FailGracefully( hr, "Unable to create BindCtx" );

    hr = pbc->SetBindOptions( &bo );
    FailGracefully( hr, "Unable to set bind options" );

    hr = SHGetDesktopFolder( &psfDesktop );
    FailGracefully( hr, "Unable to get desktop shell folder" );

    hr = psfDesktop->ParseDisplayName( NULL, pbc, (LPWSTR)pszFileName, &pcchEaten, &pidl, NULL );
    FailGracefully( hr, "Unable to get pidl from shell" );

    hr = RealInitialize( pidl, NULL, NULL );

    //
    // if we failed, check the magic extension.  We might be being called to
    // initialize for the send-to menu...
    //
    if (FAILED(hr))
    {
        INT iLen = lstrlen( szPath );
        INT iLenExt = lstrlen( c_szMyDocsExt );

        if (iLenExt < iLen)
        {
            if (lstrcmpi( &szPath[ iLen - iLenExt ], c_szMyDocsExt ) == 0 )
            {
                //
                // To be doubly sure, check first part of path to see if
                // it's the SENDTO path...
                //

                TCHAR szSendTo[ MAX_PATH ];

                if (SHGetSpecialFolderPath( NULL, szSendTo, CSIDL_SENDTO, FALSE ))
                {
                    INT iLenSendTo = lstrlen( szSendTo );

                    szPath[ iLenSendTo ] = 0;

                    if (lstrcmpi( szPath, szSendTo )==0)
                    {
                        IDREGITEM idri;

                        idri.cb = (sizeof(IDREGITEM) - sizeof(WORD));
                        idri.bFlags = SHID_ROOT_REGITEM;
                        idri.clsid = CLSID_MyDocumentsExt;
                        idri.next = 0;

                        hr = RealInitialize( (LPITEMIDLIST)&idri, 0, 0 );

                        if (SUCCEEDED(hr))
                        {
                            //
                            // Make sure that the file in the SendTo folder
                            // has the most up-to-date name of the desktop
                            // item...
                            //

                            UpdateSendToFile( FALSE );

                        }
                    }
                }

            }
        }
    }

exit_gracefully:

    DoRelease( pbc );
    DoRelease( psfDesktop );
    DoILFree( pidl );
    MDTraceLeaveResult(hr);
}

/*---------------------------------------------------------------------------*/
STDMETHODIMP
CMyDocsFolder::Save(LPCOLESTR pszFileName, BOOL fRemember)
{
    MDTraceEnter( TRACE_FOLDER, "CMyDocsFolder(IPersistFile)::Save (not implemented)" );
    MDTraceLeaveResult(E_NOTIMPL);
}

/*---------------------------------------------------------------------------*/
STDMETHODIMP
CMyDocsFolder::SaveCompleted(LPCOLESTR pszFileName)
{
    MDTraceEnter( TRACE_FOLDER, "CMyDocsFolder(IPersistFile)::SaveCompleted (not implemented)" );
    MDTraceLeaveResult(E_NOTIMPL);
}

/*---------------------------------------------------------------------------*/
STDMETHODIMP
CMyDocsFolder::GetCurFile(LPOLESTR *ppszFileName)
{
    MDTraceEnter( TRACE_FOLDER, "CMyDocsFolder(IPersistFile)::GetCurFile (not implemented)" );
    MDTraceLeaveResult(E_NOTIMPL);
}


/*-----------------------------------------------------------------------------
/ CMyDocsFolder methods
/----------------------------------------------------------------------------*/
STDMETHODIMP
CMyDocsFolder::RealInitialize( LPCITEMIDLIST pidlRoot,
                               LPCITEMIDLIST pidlBindTo,
                               LPTSTR pRootPath
                              )
{
    HRESULT hr = E_FAIL;
    IPersistFolder * ppf        = NULL;
    IShellFolder   * psfDesktop = NULL;
    IUnknown       * punk       = NULL;
    LPBC pbc = NULL;
    TCHAR szPath[ MAX_PATH ];


    MDTraceEnter(TRACE_FOLDER, "CMyDocsFolder::RealInitialize");

    MDTraceAssert(pidlRoot);


    if ( !pidlBindTo )
        m_pidl = ILClone(pidlRoot);
    else
        m_pidl = ILCombine(pidlRoot, pidlBindTo);

    if ( !m_pidl )
        ExitGracefully(hr, E_OUTOFMEMORY, "Failed to create root IDLIST");

    if (IsIDLRootOfNameSpace(ILFindLastID(m_pidl)))
    {
        MDTrace( TEXT("FOLDER_IS_ROOT") );
        m_type = FOLDER_IS_ROOT;
    }


    //
    // If we're the root of the namespace, then we need to get the
    // path to the CSIDL_PERSONAL folder...
    //

    if (m_type == FOLDER_IS_ROOT)
    {
        if (!GetPersonalPath( szPath, FALSE ))
            ExitGracefully( hr, E_FAIL, "Not able to get personal path" );

        hr = LocalAllocString( &m_path, szPath );
        FailGracefully( hr, "Not able to allocate m_path" );
    }
    else if (MDIsSpecialIDL( ILFindLastID(m_pidl) ))
    {
        //
        // Get the real folder path from this special idlist
        //

        MDTrace( TEXT("FOLDER_IS_SPECIAL_ITEM") );
        m_type = FOLDER_IS_SPECIAL_ITEM;
        if (MDGetPathFromIDL( ILFindLastID(m_pidl), szPath, NULL ))
        {
            LocalAllocString( &m_path, szPath );
        }

    }
    else if (pidlBindTo && (MDIsJunctionIDL( ILFindLastID(m_pidl) )))
    {
        MDTrace( TEXT("FOLDER_IS_JUNCTION") );
        m_type = FOLDER_IS_JUNCTION;

        if (MDIsSpecialIDL( (LPITEMIDLIST)pidlBindTo ))
        {
            if (!MDGetFullPathFromIDL( (LPITEMIDLIST)pidlBindTo, szPath, NULL))
                ExitGracefully( hr, E_FAIL, "unable to get full folder path from special IDL" );
        }
        else
        {
            if (!MDGetFullPathFromIDL( (LPITEMIDLIST)pidlBindTo, szPath, pRootPath ))
                ExitGracefully( hr, E_FAIL, "unable to get full folder path from IDL" );
        }

        hr = LocalAllocString( &m_path, szPath );

    }
    else
    {
        TCHAR szPidlPath[ MAX_PATH ];
        //
        // We're being called with a pidl that isn't the regitm for our
        // namespace and that isn't a specail IDL, so most likely it's
        // the path to the shell folder & we've been invoked via the
        // desktop.ini settings...
        //

        if (!MDGetPathFromIDL( (LPITEMIDLIST)pidlRoot, szPidlPath, NULL ))
            ExitGracefully( hr, E_FAIL, "unable to get path from IDL" );


        MDTrace( TEXT("pidlPath = -%s-"), szPidlPath );

        if (!GetPersonalPath( szPath, FALSE ))
            ExitGracefully( hr, E_FAIL, "couldn't get path for CSIDL_PERSONAL" );

        if (lstrcmpi( szPidlPath, szPath) == 0)
        {
            MDTrace( TEXT("FOLDER_IS_ROOT_PATH") );
            m_type = FOLDER_IS_ROOT_PATH;
            hr = LocalAllocString( &m_path, szPath );
            FailGracefully( hr, "Not able to allocate m_path" );
        }
        else
        {
            if (IsWinstone97(szPath) && (lstrcmpi(szPidlPath, szPath)==0))
            {
                //
                // We should treat this like a junction...
                //
                MDTrace( TEXT("FOLDER_IS_JUNCTION (via path & Winstone97 check)") );
                m_type = FOLDER_IS_JUNCTION;
                hr = LocalAllocString( &m_path, szPath );
                FailGracefully( hr, "Not able to allocate m_path" );

            }
            else
            {
                //
                // Last ditch -- we might be being called for a My Docs folder
                // that is not the current My Docs folder.  (i.e., one in another
                // person's profile.  Check for a desktop.ini, etc., that points
                // to us to verify...
                //

                TCHAR szDesktopIni[ MAX_PATH ];
                TCHAR szGUID[ 64 ];

                lstrcpy( szDesktopIni, szPidlPath );
                lstrcat( szDesktopIni, c_szDesktopIni );

                szGUID[0] = 0;
                GetPrivateProfileString( c_szShellInfo,
                                         c_szCLSID2,
                                         c_szConfig,    // anything will work for default
                                         szGUID,
                                         ARRAYSIZE(szGUID),
                                         szDesktopIni
                                        );

                if ( lstrcmpi( szGUID, c_szMyDocsCLSID ) == 0 )
                {
                    MDTrace( TEXT("Being called for an unblessed My Docs folder...") );
                    m_type = FOLDER_IS_UNBLESSED_ROOT_PATH;
                    hr = LocalAllocString( &m_path, szPidlPath );
                    FailGracefully( hr, "Not able to allocate m_path" );
                }
                else
                {
                    ExitGracefully( hr, E_FAIL, "pidl path & personal path are not the same!" );
                }
            }
        }



    }

    if (m_path)
    {
        BIND_OPTS bo;
        WCHAR wszPath[MAX_PATH];
        ULONG pcchEaten;

        memset( &bo, 0, sizeof(bo) );
        bo.cbStruct = sizeof(bo);
        bo.grfFlags = BIND_JUSTTESTEXISTENCE;

        //
        // Test the path first before we try to bind to it...
        // We do this in case it's a UNC or net use'd drive to make sure
        // we don't go recurse trying to connect to a resource that's not
        // there...
        //

        if (!PathFileExists( m_path ))
            ExitGracefully( hr, E_FAIL, "PathFileExists failed!" );

        //
        // Now, try to bind to the real shell folder...
        //

        hr = QueryInterface(IID_IUnknown, (LPVOID *)&punk);
        FailGracefully( hr, "Couldn't get IUnknown for MyDocs!" );

        hr = SHCoCreateInstance( NULL, &CLSID_ShellFSFolder, (IUnknown*)punk, IID_IUnknown, (LPVOID *)&m_punk );
        FailGracefully( hr, "Couldn't get IUnknown for shell folder" );

        hr = SHQueryInnerInterface(
            punk,
            m_punk,
            IID_IShellFolder,
            (LPVOID *) &m_psf
        );
        FailGracefully( hr, "Couldn't QI FSFolder for IID_IShellFolder" );

        hr = SHQueryInnerInterface(
            punk,
            m_punk,
            IID_IPersistFolder,
            (LPVOID *) &ppf
        );
        FailGracefully( hr, "Couldn't get IPersistFolder from FSFolder" );

        MDTrace( TEXT("Folder Path is: %s"), m_path );

        hr = CreateBindCtx( 0, &pbc );
        FailGracefully( hr, "Unable to create BindCtx" );

        hr = pbc->SetBindOptions( &bo );
        FailGracefully( hr, "Unable to set bind options" );

        hr = SHGetDesktopFolder( &psfDesktop );
        FailGracefully( hr, "Unable to get desktop shell folder" );

        SHTCharToUnicode( m_path, wszPath, ARRAYSIZE(wszPath) );

        hr = psfDesktop->ParseDisplayName( NULL, pbc, wszPath, &pcchEaten, &m_pidlReal, NULL );
        FailGracefully( hr, "Unable to get pidl from shell" );

        hr = ppf->Initialize( m_pidlReal );

    }

exit_gracefully:

    RELEASEINNERINTERFACE( punk, ppf );
    DoRelease( pbc );
    DoRelease( psfDesktop );
    DoRelease( punk );

    MDTraceLeaveResult(hr);
}



/*-----------------------------------------------------------------------------
/ IShellFolder methods
/----------------------------------------------------------------------------*/
STDMETHODIMP
CMyDocsFolder::ParseDisplayName( HWND hwndOwner, LPBC pbcReserved,
                                 LPOLESTR pDisplayName, ULONG* pchEaten,
                                 LPITEMIDLIST* ppidl, ULONG *pdwAttributes
                                )
{
    HRESULT hr = E_FAIL;
    TCHAR szName[ MAX_PATH ];

    MDTraceEnter(TRACE_FOLDER, "CMyDocsFolder(IShellFolder)::ParseDisplayName");
#ifdef UNICODE
    MDTrace( TEXT("DisplayName for parsing is: %s"), pDisplayName );
#endif


    if ((m_type == FOLDER_IS_ROOT) || (m_type == FOLDER_IS_ROOT_PATH))
    {
        TCHAR szDispName[ MAX_PATH ];

        GetMyDocumentsDisplayName( szDispName, ARRAYSIZE(szDispName) );
        OleStrToStrN( szName, ARRAYSIZE(szName), pDisplayName, -1 );

        if (szDispName[0])
        {
            if (lstrcmpi( szDispName, szName )==0)
            {
                ITEMIDLIST idlEmpty;

                idlEmpty.mkid.cb = 0;
                idlEmpty.mkid.abID[0] = 0;

                *ppidl = ILClone( &idlEmpty );
                hr = S_OK;
            }

        }
    }

    if ( (hr != S_OK) && ( (m_type == FOLDER_IS_ROOT)      ||
                           (m_type == FOLDER_IS_ROOT_PATH) ||
                           (m_type == FOLDER_IS_SPECIAL_ITEM)
                          )
        )
    {
        CMyDocsEnum * pEnum = NULL;
        LPITEMIDLIST pidl;

        //
        // Get enumerator which will give us a list of special items...
        //

        pEnum = new CMyDocsEnum( NULL, NULL, 0, NULL, TRUE );
        if (!pEnum)
            ExitGracefully( hr, E_FAIL, "Couldn't get pEnum" );

        // Init enumerator (read special items from registry)
        pEnum->DoFirstTimeInitialization();

        OleStrToStrN( szName, ARRAYSIZE(szName), pDisplayName, -1 );

        pidl = pEnum->FindSpecialItem( szName );

        if (pidl)
        {
            //
            // Check to see if this special item has a pidl...
            //

            if (SHIDLFromMDIDL(pidl))
            {
                *ppidl = ILClone(SHIDLFromMDIDL( pidl ));
                hr = S_OK;
            }
            else
            {
                //
                // If the special item doesn't have a pidl, but does have
                // a path store -- then check to see if the path exists
                // and if so, create a pidl to it.
                //

                TCHAR szPath[ MAX_PATH ];
                UINT cch = ARRAYSIZE(szPath);

                hr = MDGetPathInfoFromIDL( pidl, szPath, &cch );
                if (SUCCEEDED(hr))
                {
                    if (PathFileExists( szPath ))
                    {
                        hr = SHILCreateFromPath( szPath, ppidl, NULL );
                    }
                }
            }

        }

        delete pEnum;

    }

    if (hr != S_OK)
    {
        hr = m_psf->ParseDisplayName( hwndOwner,
                                      pbcReserved,
                                      pDisplayName,
                                      pchEaten,
                                      ppidl,
                                      pdwAttributes
                                     );
        FailGracefully( hr, "Call to shell's ParseDisplayName failed" );
    }


exit_gracefully:

    MDTraceLeaveResult(hr);
}

/*---------------------------------------------------------------------------*/
STDMETHODIMP
CMyDocsFolder::EnumObjects( HWND hwndOwner,
                            DWORD grfFlags,
                            LPENUMIDLIST* ppEnumIdList
                           )
{
    HRESULT hr = E_FAIL;
    CMyDocsEnum* pEnum = NULL;
    BOOL bRoot;

    MDTraceEnter(TRACE_FOLDER, "CMyDocsFolder(IShellFolder)::EnumObjects");

    if ( !ppEnumIdList )
        ExitGracefully(hr, E_INVALIDARG, "No return pointer for the IEnumIDList interface");

    bRoot = ((m_type == FOLDER_IS_ROOT) || (m_type == FOLDER_IS_ROOT_PATH));
    pEnum = new CMyDocsEnum( m_psf,
                             hwndOwner,
                             grfFlags,
                             m_pidlReal,
                             bRoot
                            );

    if ( !pEnum )
        ExitGracefully(hr, E_OUTOFMEMORY, "Failed to create the enumerator");

    pEnum->AddRef();
    *ppEnumIdList = (LPENUMIDLIST)pEnum;

    hr = S_OK;

exit_gracefully:

    MDTraceLeaveResult(hr);
}

/*---------------------------------------------------------------------------*/
STDMETHODIMP
CMyDocsFolder::BindToObject( LPCITEMIDLIST pidl,
                             LPBC pbcReserved,
                             REFIID riid,
                             LPVOID* ppvOut
                            )
{
    HRESULT hr = E_FAIL;
    CMyDocsFolder* pMDF = NULL;
//    DWORD dwAttrb;

    MDTraceEnter(TRACE_FOLDER, "CMyDocsFolder(IShellFolder)::BindToObject");
    MDTrace(TEXT("Entry IDLIST is %08x"), pidl);
    MDTraceGUID("Interface being requested", riid);

    if ( !pidl || !ppvOut )
        ExitGracefully(hr, E_INVALIDARG, "Bad parameters for BindToObject");

    //
    // Check the attributes on the item being bound to...
    //

    if (!MDIsSpecialIDL( (LPITEMIDLIST)pidl ))
    {
        MDTrace( TEXT("Calling shell's BindToObject directly") );
        hr = m_psf->BindToObject( pidl, pbcReserved, riid, ppvOut );
        goto exit_gracefully;
    }

    pMDF = new CMyDocsFolder();

    if ( !pMDF )
        ExitGracefully(hr, E_OUTOFMEMORY, "Failed to allocate new CMyDocsFolder");

    pMDF->AddRef();

    hr = pMDF->RealInitialize( m_pidl, pidl, m_path );

    if ( SUCCEEDED(hr) )
        hr = pMDF->QueryInterface(riid, ppvOut);

exit_gracefully:

    DoRelease( pMDF );

    MDTraceLeaveResult(hr);
}

/*---------------------------------------------------------------------------*/
STDMETHODIMP
CMyDocsFolder::BindToStorage( LPCITEMIDLIST pidl,
                              LPBC pbcReserved,
                              REFIID riid,
                              LPVOID* ppvObj
                             )
{
    MDTraceEnter(TRACE_FOLDER, "CMyDocsFolder(IShellFolder)::BindToStorage (Not Implemented)");
    MDTraceLeaveResult(E_NOTIMPL);
}


/*---------------------------------------------------------------------------*/
STDMETHODIMP
CMyDocsFolder::CompareIDs( LPARAM lParam,
                           LPCITEMIDLIST pidl1,
                           LPCITEMIDLIST pidl2
                          )
{
    HRESULT hr = E_FAIL;
    INT iResult = -1;

    MDTraceEnter(TRACE_COMPAREIDS, "CMyDocsFolder(IShellFolder)::CompareIDs");

    MDTrace(TEXT("pidl1 %08x, pidl2 %08x"), pidl1, pidl2);
    MDTrace(TEXT("lParam == %d"), lParam);

    MDTraceAssert(pidl1);
    MDTraceAssert(pidl2);

    TCHAR szPath1[MAX_PATH];
    TCHAR szPath2[MAX_PATH];
    BOOL bSpecial1 = FALSE, bSpecial2 = FALSE;
    UINT cch;

    szPath1[0] = 0; szPath2[0] = 0;

    //
    // Get the names & other generic info...
    //

    if (MDIsSpecialIDL((LPITEMIDLIST)pidl1))
    {
        cch = ARRAYSIZE(szPath1);
        hr = MDGetNameFromIDL( (LPITEMIDLIST)pidl1, szPath1, &cch, TRUE );
        FailGracefully(hr, "couldn't get name from special idlist1" );
        bSpecial1 = TRUE;
    }

    if (MDIsSpecialIDL((LPITEMIDLIST)pidl2))
    {
        cch = ARRAYSIZE(szPath2);
        hr = MDGetNameFromIDL( (LPITEMIDLIST)pidl2, szPath2, &cch, TRUE );
        FailGracefully(hr, "couldn't get name from special idlist2" );
        bSpecial2 = TRUE;
    }


    //
    // This is the same no matter which is our current sort mode...
    //

    if (bSpecial1 && bSpecial2)
    {
        //
        // We know they're both special items, so just sort by name
        //
        iResult = lstrcmpi( szPath1, szPath2 );
    }
    else if (bSpecial1)
    {
        MDTrace(TEXT("pidl1 is special"));
        iResult = -1;
    }
    else if (bSpecial2)
    {
        MDTrace(TEXT("pidl2 is special"));
        iResult = 1;
    }
    else
    {
        //
        // Neither one is a special item, let the shell do the compare
        // for us...
        //

        if (lParam >= FSIDM_SORT_FIRST)
        {
            lParam = FSSortIDToICol(lParam);
        }
        hr = m_psf->CompareIDs( lParam,
                                SHIDLFromMDIDL(pidl1),
                                SHIDLFromMDIDL(pidl2)
                               );
        goto exit_gracefully;
    }

    hr = ResultFromShort(iResult);

exit_gracefully:

    MDTrace(TEXT("Exiting with iResult %d"), ShortFromResult(hr));
    MDTraceLeaveResult(hr);
}

/*---------------------------------------------------------------------------*/
STDMETHODIMP
CMyDocsFolder::CreateViewObject( HWND hwndOwner,
                                 REFIID riid,
                                 LPVOID* ppvOut
                                )
{
    HRESULT hr;

    MDTraceEnter(TRACE_FOLDER, "CMyDocsFolder(IShellFolder)::CreateViewObject");
    MDTraceGUID("View object requested", riid);

    MDTraceAssert(ppvOut);

    *ppvOut = NULL;

    if ( IsEqualIID(riid, IID_IShellDetails) || IsEqualIID(riid, IID_IShellDetails3))
    {
        hr = CreateMyDocsColumns(m_psf, m_psfv, riid, ppvOut);
    }
    else
    {
        MDTrace(TEXT("Getting interface from the shell directly..."));
        hr = m_psf->CreateViewObject( hwndOwner, riid, ppvOut );
        FailGracefully( hr, "shell's CreateViewObject failed" );

        if ( IsEqualIID(riid, IID_IShellView) )
        {
            LPSHELLVIEW psv = (LPSHELLVIEW)*ppvOut;
            IShellFolderView *psfv;

            //
            // Need to poke in our new shell folder view callback...
            //

            if ( SUCCEEDED(psv->QueryInterface( IID_IShellFolderView,
                                                (LPVOID*)&psfv
                                               ))
                )
            {
                IShellFolderViewCB *psfvcbWrap;

                m_psfv = psfv;
                CMyDocsViewCB *psfvcb = new CMyDocsViewCB(psfv, m_pidlReal);
                if (psfvcb)
                {
                    if (SUCCEEDED(psfv->SetCallback(psfvcb, &psfvcbWrap)))
                    {
                        psfvcb->SetRealCB(psfvcbWrap);
                    }
                }
                psfv->Release();
            }

        }
    }

exit_gracefully:

    MDTraceLeaveResult(hr);
}

/*---------------------------------------------------------------------------*/
STDMETHODIMP
CMyDocsFolder::GetAttributesOf( UINT cidl,
                                LPCITEMIDLIST* apidl,
                                ULONG* rgfInOut
                               )
{
    HRESULT hr = S_OK;
    LPITEMIDLIST * apidlShell = NULL;
    ULONG uSpecFlags = 0;
    UINT i;

    MDTraceEnter(TRACE_FOLDER, "CMyDocsFolder(IShellFolder)::GetAttributesOf");
    MDTrace(TEXT("cidl = %d"), cidl);

    if (cidl == 0 || ((cidl == 1) && ((*apidl)->mkid.cb == 0)))
    {
        WhoIsCalling();
        switch( m_host )
        {
            case APP_IS_OFFICE95:
            case APP_IS_OFFICE97:
                *rgfInOut = SFGAO_FOLDER | SFGAO_FILESYSTEM | SFGAO_CANRENAME |
                    SFGAO_HASSUBFOLDER | SFGAO_DROPTARGET | SFGAO_CANLINK;
                break;

            default:
                MDTrace(TEXT("Setting normal attributes"));
                *rgfInOut = SFGAO_FOLDER | SFGAO_FILESYSTEM | SFGAO_CANRENAME |
                    SFGAO_HASSUBFOLDER | SFGAO_FILESYSANCESTOR | SFGAO_CANLINK |
                    SFGAO_DROPTARGET | SFGAO_CANMONIKER
                     | SFGAO_HASPROPSHEET
                    ;

                if ( IsMyDocsHidden() )
                {
                    MDTrace(TEXT("Setting hidden attributes too"));
                    *rgfInOut |= SFGAO_NONENUMERATED;
                }

        }

#ifdef DEBUG
        if (m_path)
        {
            MDTrace( TEXT("Root Folder path is: %s"), m_path );
        }

        if (m_type == FOLDER_IS_ROOT)
        {
            MDTrace(TEXT("FOLDER_IS_ROOT"));
        }
        else if (m_type == FOLDER_IS_ROOT_PATH)
        {
            MDTrace(TEXT("FOLDER_IS_ROOT_PATH"));
        }
#endif
        ExitGracefully( hr, S_OK, "Was called for root folder" );
    }

    if (!cidl || !apidl || !rgfInOut || !m_psf)
        ExitGracefully( hr, E_INVALIDARG, "bad incoming arguments" );

    if ((cidl == 1) && MDIsSpecialIDL( (LPITEMIDLIST)*apidl ) )
    {
        WhoIsCalling();
        switch( m_host )
        {
            case APP_IS_OFFICE95:
            case APP_IS_OFFICE97:
                *rgfInOut &= SFGAO_CANLINK | SFGAO_FILESYSTEM | SFGAO_DROPTARGET | SFGAO_LINK;
                break;

            default:
                *rgfInOut &= SFGAO_CANLINK | SFGAO_HASSUBFOLDER | SFGAO_DROPTARGET |
                             SFGAO_FOLDER | SFGAO_FILESYSANCESTOR;

        }
        ExitGracefully( hr, S_OK, "returning attrib for special item" );
    }


    for( i = 0; i<cidl; i++)
    {
        if (MDIsSpecialIDL( (LPITEMIDLIST)apidl[i] ))
        {
            WhoIsCalling();
            switch( m_host )
            {
                case APP_IS_OFFICE95:
                case APP_IS_OFFICE97:
                    uSpecFlags = SFGAO_CANLINK | SFGAO_FILESYSTEM | SFGAO_HASSUBFOLDER |
                                 SFGAO_DROPTARGET | SFGAO_LINK;
                    break;
                default:
                    uSpecFlags = SFGAO_CANLINK | SFGAO_HASSUBFOLDER | SFGAO_DROPTARGET |
                                 SFGAO_FOLDER | SFGAO_FILESYSANCESTOR;

            }
            break;
        }
    }

    apidlShell = (LPITEMIDLIST *)LocalAlloc( LPTR, sizeof(LPITEMIDLIST) * cidl );
    if (!apidlShell)
        ExitGracefully( hr, E_OUTOFMEMORY, "couldn't allocate apidlShell" );

    hr = MDUnWrapMDIDLists( cidl, (LPITEMIDLIST *)apidl, apidlShell );
    FailGracefully( hr, "Couldn't unwrap idlists" );

    hr = m_psf->GetAttributesOf( cidl, (LPCITEMIDLIST *)apidlShell, rgfInOut );

#if (defined(DEBUG) && defined(SHOW_PATHS))
    if (cidl == 1)
    {
        PrintPath( (LPITEMIDLIST)*apidl );
    }
#endif

    if (SUCCEEDED(hr) && uSpecFlags)
    {
        *rgfInOut &= uSpecFlags;
    }

exit_gracefully:

#if (defined(DEBUG) && defined(SHOW_ATTRIBUTES))
    PrintAttributes(*rgfInOut);
#endif

    if (apidlShell)
        LocalFree( (LPVOID)apidlShell );

    MDTraceLeaveResult(hr);
}

/*---------------------------------------------------------------------------*/
STDMETHODIMP
CMyDocsFolder::GetUIObjectOf( HWND hwndOwner,
                              UINT cidl,
                              LPCITEMIDLIST* aidl,
                              REFIID riid,
                              UINT* prgfReserved,
                              LPVOID* ppvOut
                             )
{
    HRESULT hr = E_NOINTERFACE;
    LPCITEMIDLIST * aidlShell = NULL;
    IShellFolder * psfDesktop = NULL;
    IShellFolder * psf = NULL;
    LPITEMIDLIST pidlLast = NULL;
    LPITEMIDLIST pidlMain = NULL;
    LPITEMIDLIST pidlAlloc = NULL;

    MDTraceEnter(TRACE_FOLDER, "CMyDocsFolder(IShellFolder)::GetUIObjectOf");
    MDTraceGUID("UI object requested", riid);

    MDTraceAssert(cidl > 0);
    MDTraceAssert(aidl);
    MDTraceAssert(ppvOut);

    if ( IsEqualIID(riid, IID_IContextMenu) )
    {

        BOOL fAnyAreSpecial = FALSE;
        UINT i;

        for (i = 0; i<cidl; i++)
        {
            fAnyAreSpecial |= MDIsSpecialIDL((LPITEMIDLIST)aidl[i]);
        }


        if (fAnyAreSpecial)
        {
            HKEY aKeys[ UIKEY_MAX ];

            // IContextMenu requested, therefore let the shell construct one for us,
            // then we can provide verbs on the namespace

            if ( cidl )
            {
                hr = _GetKeysForIDL(cidl, aidl, ARRAYSIZE(aKeys), aKeys);
                FailGracefully(hr, "Failed to get object keys");
            }

            hr = CDefFolderMenu_Create2(m_pidl, hwndOwner,
                                        cidl, aidl,
                                        this,
                                        _FolderCFMCallback,
                                        ARRAYSIZE(aKeys), aKeys,
                                        (LPCONTEXTMENU*)ppvOut);

            FailGracefully(hr, "Failed to create context menu");

        }
        else
        {
            //
            // Map our idlists to shell idlists...
            //

            aidlShell = (LPCITEMIDLIST *)LocalAlloc( LPTR, sizeof(LPCITEMIDLIST) * cidl );
            if (!aidlShell)
                ExitGracefully( hr, E_OUTOFMEMORY, "Unable to allocate aidlShell" );

            hr = MDUnWrapMDIDLists( cidl,
                                    (LPITEMIDLIST *)aidl,
                                    (LPITEMIDLIST *)aidlShell
                                   );
            FailGracefully( hr, "unable to unwrap aidl" );

            //
            // Get shell's view object
            //

            hr = m_psf->GetUIObjectOf( hwndOwner, cidl, aidlShell, riid, prgfReserved, ppvOut );
            FailGracefully( hr, "couldn't get IContextMenu from shell" );

        }

    }
    else if ( IsEqualIID(riid, IID_IDataObject) )
    {

        // IDataObject, this is used to pass selection information to the outside world
        // (both to the shell and IContextMenu handlers)

        CMyDocsDataObject* pDataObject = new CMyDocsDataObject(m_psf, m_pidl, m_pidlReal, cidl, aidl);

        if ( !pDataObject )
            ExitGracefully(hr, E_OUTOFMEMORY, "Failed to create DataObject");

        hr = pDataObject->QueryInterface(riid, ppvOut);

        if ( FAILED(hr) )               // deliberate (don't use ELSE)
            delete pDataObject;

    }
    else if ( IsEqualIID(riid, IID_IExtractIcon) )
    {

        // IExtractIcon is used to support our object extraction, we can only cope
        // with extracting a single object at a time.  First however we must
        // build an IDLIST that represents the object we want to extract
        // and get the icon for it.

        if ( cidl != 1 )
            ExitGracefully(hr, E_FAIL, "Bad number of objects to get icon from");


        CMyDocsExtractIcon* pExtractIcon = new CMyDocsExtractIcon( m_psf,
                                                                   hwndOwner,
                                                                   *aidl,
                                                                   FOLDER_IS_UNKNOWN
                                                                  );

        if ( !pExtractIcon )
            ExitGracefully(hr, E_OUTOFMEMORY, "Failed to create CMyDocsExtractIcon");


        *ppvOut = pExtractIcon;
        pExtractIcon->AddRef();
        hr = S_OK;

    }
    else if ( IsEqualIID(riid, IID_IDropTarget) && (cidl == 1) && MDIsSpecialIDL((LPITEMIDLIST)*aidl))
    {
        LPITEMIDLIST pidlTmp = SHIDLFromMDIDL( *aidl );

        MDTrace(TEXT("Getting IDtopTarget for special item...") );

        if (!pidlTmp)
        {
            TCHAR szPath[ MAX_PATH ];
            UINT cch = ARRAYSIZE( szPath );

            hr = MDGetPathInfoFromIDL( (LPITEMIDLIST)*aidl, szPath, &cch );
            FailGracefully( hr, "There is no shell pidl or path with the item at the moment" );

            if (!PathFileExists( szPath ))
                ExitGracefully( hr, E_FAIL, "The path isn't present right now" );

            hr = SHILCreateFromPath( szPath, &pidlAlloc, NULL );
            FailGracefully( hr, "Couldn't create a pidl on the fly for the path" );

            pidlTmp = pidlAlloc;
        }

        hr = SHGetDesktopFolder( &psfDesktop );
        FailGracefully( hr, "Couldn't get psf for desktop folder" );

        pidlLast = ILClone( ILFindLastID( pidlTmp ) );
        pidlMain = ILClone( pidlTmp );
        ILRemoveLastID( pidlMain );

        hr = psfDesktop->BindToObject( pidlMain,
                                       NULL,
                                       IID_IShellFolder,
                                       (LPVOID *)&psf
                                      );
        FailGracefully( hr, "Couldn't bind to special item object" );

        hr = psf->GetUIObjectOf( hwndOwner,
                                 1,
                                 (LPCITEMIDLIST *)&pidlLast,
                                 IID_IDropTarget,
                                 prgfReserved,
                                 ppvOut
                                );


    }
    else
    {


        //
        // Map our idlists to shell idlists...
        //

        aidlShell = (LPCITEMIDLIST *)LocalAlloc( LPTR, sizeof(LPCITEMIDLIST) * cidl );
        if (!aidlShell)
            ExitGracefully( hr, E_OUTOFMEMORY, "Unable to allocate aidlShell" );

        hr = MDUnWrapMDIDLists( cidl,
                                (LPITEMIDLIST *)aidl,
                                (LPITEMIDLIST *)aidlShell
                               );
        FailGracefully( hr, "unable to unwrap aidl" );


        hr = m_psf->GetUIObjectOf( hwndOwner, cidl, aidlShell, riid, prgfReserved, ppvOut );
        FailGracefully(hr, "UI object not supported by shell");
        MDTrace(TEXT("*!*!*!*!* Getting UI Object from the shell directly *!*!*!*!*") );
    }

exit_gracefully:


    DoRelease( psfDesktop );
    DoRelease( psf );
    DoILFree( pidlMain );
    DoILFree( pidlLast );
    DoILFree( pidlAlloc );
    if (aidlShell)
        LocalFree( aidlShell );

    MDTraceLeaveResult(hr);
}

/*---------------------------------------------------------------------------*/
STDMETHODIMP
CMyDocsFolder::GetDisplayNameOf( LPCITEMIDLIST pidl,
                                 DWORD uFlags,
                                 LPSTRRET pName
                                )
{
    HRESULT hr = E_FAIL;
    LPITEMIDLIST pidlShell = NULL;
    TCHAR szName[ MAX_PATH ];
    UINT cch = ARRAYSIZE(szName);


    MDTraceEnter(TRACE_FOLDER, "CMyDocsFolder(IShellFolder)::GetDisplayNameOf");
    MDTrace(TEXT("uFlags = 0x%0x"), uFlags );

    MDTraceAssert(pidl);
    MDTraceAssert(pName);

    if (!m_psf)
        ExitGracefully( hr, E_FAIL, "m_psf is NULL" );

    //
    // Special check to see if this is an empty pidl & we're
    // trying to get a name for parsing, if this is the case
    // then return the file sys path to the mydocs folder...
    //
    if (ILIsEmpty(pidl))
    {
        LPTSTR pPath = m_path;

        MDTrace(TEXT("It's a request for the folder itself"));
        if (uFlags == SHGDN_INFOLDER)
        {
            pPath = PathFindFileName( m_path );
        }

        MDTrace( TEXT("Returning: %s"), pPath );
        hr = StrRetFromString( pName, pPath );
        FailGracefully( hr, "StrRetFromString failed" );
        MDTraceLeaveResult( S_OK );
    }

    //
    // Is it a special pidl?
    //
    if (MDIsSpecialIDL( (LPITEMIDLIST)pidl ))
    {
        MDTrace(TEXT("It's a special pidl"));
        if (uFlags == SHGDN_FORPARSING)
        {
            // Get the full path
            if (MDGetPathFromIDL( (LPITEMIDLIST)pidl, szName, NULL ))
            {
                hr = S_OK;
            }
        }
        else
        {
            // Just return the name
            hr = MDGetNameFromIDL( (LPITEMIDLIST)pidl, szName, &cch, FALSE );
        }

        if (SUCCEEDED(hr))
        {
            MDTrace( TEXT("Returning: %s"), szName );
            hr = StrRetFromString( pName, szName );
            FailGracefully( hr, "StrRetFromString failed" );
            MDTraceLeaveResult( S_OK );
        }
        else 
        {
            MDTrace( TEXT("MDGetPathFromIDL() or MDGetNameFromIDL() failed.") );
            MDTrace( TEXT("Falling through to let the shell try to handle it.") );

            pidlShell = SHIDLFromMDIDL( pidl );
        } 
    }


    if (!pidlShell) {
        MDTrace( TEXT("pidl is not one of ours") );
        pidlShell = (LPITEMIDLIST)pidl;
    }

#if (defined(DEBUG) && defined(SHOW_PATHS))
    PrintPath( (LPITEMIDLIST)pidl );
#endif


    //
    // Call the shell to do it's thing...
    //

    hr = m_psf->GetDisplayNameOf( pidlShell, uFlags, pName );

    //
    // If it's a STRRET_OFFSET type we need to fix up the offset, as
    // the pidl we gave is embedded in our pidl.
    //

    if ( SUCCEEDED(hr)       &&
         (pidlShell != pidl) &&
         (pName->uType == STRRET_OFFSET)
        )
    {
        pName->uOffset += (((UINT)pidlShell) - ((UINT)pidl));
    }

#if (defined(DEBUG) && defined(SHOW_PATHS))
    if (SUCCEEDED(hr))
    {
        TCHAR szName[ MAX_PATH ];
        StrretToString( pName, (LPITEMIDLIST)pidl, szName, MAX_PATH );
        MDTrace(TEXT("Returning: %s"), szName );
    }
#endif

exit_gracefully:

    MDTraceLeaveResult(hr);
}

/*---------------------------------------------------------------------------*/
STDMETHODIMP
CMyDocsFolder::SetNameOf( HWND hwndOwner,
                          LPCITEMIDLIST pidl,
                          LPCOLESTR pName,
                          DWORD uFlags,
                          LPITEMIDLIST * ppidlOut
                         )
{
    HRESULT hr;

    MDTraceEnter(TRACE_FOLDER, "CMyDocsFolder(IShellFolder)::SetDisplayNameOf");

    if (MDIsSpecialIDL( (LPITEMIDLIST)pidl ))
    {
        hr = E_FAIL;
        FailGracefully( hr, "Can't SetNameOf on special items" );
    }

    hr = m_psf->SetNameOf( hwndOwner,
                           ILFindLastID( SHIDLFromMDIDL( pidl ) ),
                           pName,
                           uFlags,
                           ppidlOut
                          );
    FailGracefully( hr, "call to shell's SetNameOf failed" );

exit_gracefully:

    MDTraceLeaveResult(hr);
}

/*-----------------------------------------------------------------------------
/ IShellExtInit methods
/----------------------------------------------------------------------------*/
STDMETHODIMP
CMyDocsFolder::Initialize( LPCITEMIDLIST pidlFolder,
                           LPDATAOBJECT lpdobj,
                           HKEY hkeyProgID
                          )
{
    HRESULT hr = S_OK;

    MDTraceEnter( TRACE_EXT, "CMyDocsFolder(IShellExtInit)::Initialize" );
    MDTrace(TEXT("pidlFolder = 0x%x, lpdobj = 0x%x, hkeyProgID = 0x%x"), pidlFolder, lpdobj, hkeyProgID );

    m_ext = EXT_IS_UNKNOWN;
    if (pidlFolder)
    {
        if ( IsIDLRootOfNameSpace((LPITEMIDLIST)pidlFolder) ||
             ILIsEmpty( pidlFolder )
           )
        {
            if (IsMyDocsHidden())
            {
                ExitGracefully( hr, E_FAIL, "My Docs is hidden on the desktop" );
            }

            MDTrace(TEXT("IShellExtInit being init'd as ROOT"));
            m_ext = EXT_IS_ROOT;
        }
    }
    else if (lpdobj)
    {
        FORMATETC fmt;
        STGMEDIUM medium;

        RegisterMyDocsClipboardFormats();

        fmt.cfFormat = g_cfShellIDList;
        fmt.ptd = NULL;
        fmt.dwAspect = DVASPECT_CONTENT;
        fmt.lindex = -1;
        fmt.tymed = TYMED_HGLOBAL;

        hr = lpdobj->GetData( &fmt, &medium );
        if (SUCCEEDED(hr))
        {
            if (medium.tymed == TYMED_HGLOBAL)
            {
                LPIDA lpIDA = (LPIDA)medium.hGlobal;
                LPITEMIDLIST pidl;

                pidl = (LPITEMIDLIST)((LPBYTE)lpIDA + lpIDA->aoffset[0]);

                if ((pidl->mkid.cb == 0) && (lpIDA->cidl == 1))
                {
                    pidl = (LPITEMIDLIST)((LPBYTE)lpIDA + lpIDA->aoffset[1]);
                }

                if (IsIDLRootOfNameSpace(pidl))
                {
                    if (IsMyDocsHidden())
                    {
                        ExitGracefully( hr, E_FAIL, "My Docs is hidden on the desktop" );
                    }

                    MDTrace(TEXT("IShellExtInit being init'd as ROOT"));
                    m_ext = EXT_IS_ROOT;
                }
                else if (lpIDA->cidl == 1)
                {
                    pidl = ILCombine( (LPITEMIDLIST)((LPBYTE)lpIDA + lpIDA->aoffset[0]),
                                      (LPITEMIDLIST)((LPBYTE)lpIDA + lpIDA->aoffset[1])
                                     );

                    if (pidl)
                    {
                        TCHAR szPidlPath[ MAX_PATH ];
                        TCHAR szPath[ MAX_PATH ];

                        if (!MDGetPathFromIDL( (LPITEMIDLIST)pidl, szPidlPath, NULL ))
                            ExitGracefully( hr, E_FAIL, "unable to get path from pidl" );


                        MDTrace( TEXT("pidlPath = -%s-"), szPidlPath );

                        if (!GetPersonalPath( szPath, FALSE ))
                            ExitGracefully( hr, E_FAIL, "couldn't get path for CSIDL_PERSONAL" );

                        if (lstrcmpi( szPidlPath, szPath) == 0)
                        {
                            MDTrace( TEXT("IShellExtInit being init'd as ROOT_PATH") );
                            m_ext = EXT_IS_ROOT_PATH;
                        }

                        ILFree( pidl );
                    }
                }
            }

            ReleaseStgMedium( &medium );

        }
        else
        {
            MDTrace( TEXT("Couldn't GetData from lpdobj for IDA, 0x%0x"), hr );
        }

    }

exit_gracefully:

    MDTraceLeaveResult(hr);
}

#ifdef DO_OTHER_PAGES
/*-----------------------------------------------------------------------------
/ _AddGeneralAndSharingPages
/----------------------------------------------------------------------------*/
STDMETHODIMP
CMyDocsFolder::_AddGeneralAndSharingPages( LPFNADDPROPSHEETPAGE lpfnAddPage,
                                           LPARAM lParam
                                          )
{

    HRESULT hr;
    IShellExtInit * psei = NULL;
    CMyDocsDataObject * pMyDocsDataObject = NULL;
    IDataObject * pDataObj = NULL;
    LPITEMIDLIST pidlMyDocs = NULL;
    ITEMIDLIST idlEmpty;
    LPCITEMIDLIST pidl;


    MDTraceEnter( TRACE_EXT, "_AddGeneralAndSharingPages" );

    idlEmpty.mkid.cb = 0;
    idlEmpty.mkid.abID[0] = 0;

    pidl = (LPCITEMIDLIST)&idlEmpty;

    SHGetSpecialFolderLocation( NULL, CSIDL_PERSONAL, &pidlMyDocs );

    pMyDocsDataObject = new CMyDocsDataObject(pidlMyDocs, pidlMyDocs, 1, &pidl);

    if ( !pMyDocsDataObject )
        ExitGracefully(hr, E_OUTOFMEMORY, "Failed to create pDataObject");

    hr = pMyDocsDataObject->QueryInterface( IID_IDataObject, (LPVOID *)&pDataObj );
    if (FAILED(hr))
    {
        delete pMyDocsDataObject;
        FailGracefully( hr, "couldn't QI for IID_IDataObject" );
    }

    DoRelease( m_pseGeneral );
    DoRelease( m_pseSharing );

    // Try to add default general page...
    hr = SHCoCreateInstance( NULL, &CLSID_ShellFileDefExt, NULL, IID_IShellExtInit, (LPVOID *)&psei );
    if (FAILED(hr))
    {
        MDTrace( TEXT("Couldn't SHCoCreateInstance general page (hr = 0x%x)"), hr );
        goto try_sharing_page;
    }

    hr = psei->Initialize( NULL, pDataObj, NULL );
    if (FAILED(hr))
    {
        MDTrace( TEXT("Couldn't Initialize General Page (hr = 0x%x)"), hr );
        goto try_sharing_page;
    }

    hr = psei->QueryInterface( IID_IShellPropSheetExt, (LPVOID *)&m_pseGeneral );
    if (FAILED(hr))
    {
        MDTrace( TEXT("Couldn't QI for IShellPropSheetExt on general page (hr = 0x%x)"), hr );
        goto try_sharing_page;
    }

    hr = m_pseGeneral->AddPages( lpfnAddPage, lParam );
    if (FAILED(hr))
    {
        MDTrace( TEXT("AddPages on general page failed (hr = 0x%x)"), hr );
    }

try_sharing_page:
    // Try to add the sharing page...
    DoRelease( psei );
    hr = SHCoCreateInstance( NULL, &CLSID_SharingPage, NULL, IID_IShellExtInit, (LPVOID *)&psei );
    FailGracefully( hr, "Couldn't SHCoCreateInstance sharing page" );

    hr = psei->Initialize( NULL, pDataObj, NULL );
    FailGracefully( hr, "Couldn't Initialize Sharing Page" );

    hr = psei->QueryInterface( IID_IShellPropSheetExt, (LPVOID *)&m_pseSharing );
    FailGracefully( hr, "Couldn't QI for IShellPropSheetExt on sharing page" );

    hr = m_pseSharing->AddPages( lpfnAddPage, lParam );
    FailGracefully( hr, "AddPages on general page failed" );

exit_gracefully:
    DoRelease( psei );
    DoRelease( pDataObj );
    DoILFree( pidlMyDocs );

    MDTraceLeaveResult(hr);
}
#endif

/*-----------------------------------------------------------------------------
/ IShellPropSheetExt methods
/----------------------------------------------------------------------------*/
STDMETHODIMP
CMyDocsFolder::AddPages( LPFNADDPROPSHEETPAGE lpfnAddPage,
                         LPARAM lParam
                        )
{
    HRESULT hr = E_NOTIMPL;
    PROPSHEETPAGE psp;
    HPROPSHEETPAGE hPage;

    MDTraceEnter( TRACE_EXT, "CMyDocsFolder(IShellPropSheetExt)::AddPages" );

    if (m_ext == EXT_IS_ROOT)
    {
#ifdef DO_OTHER_PAGES
        _AddGeneralAndSharingPages(lpfnAddPage,lParam);
#endif

        psp.dwSize = sizeof(psp);
        psp.dwFlags = PSP_DEFAULT;
        psp.hInstance = g_hInstance;
        psp.pszTemplate = MAKEINTRESOURCE(DLG_TARGET);
        psp.pfnDlgProc = TargetDlgProc;
        psp.lParam = 0;

        hPage = CreatePropertySheetPage( &psp );
        if (!hPage)
            goto exit_gracefully;

        lpfnAddPage( hPage, lParam );

        hr = ResultFromShort(1);  // set first page as default!

    }

exit_gracefully:

    MDTraceLeaveResult(hr);
}

/*---------------------------------------------------------------------------*/
STDMETHODIMP
CMyDocsFolder::ReplacePage( UINT uPageID,
                            LPFNADDPROPSHEETPAGE lpfnReplaceWith,
                            LPARAM lParam
                           )
{
    MDTraceEnter( TRACE_EXT, "CMyDocsFolder(IShellPropSheetExt)::ReplacePage (not implemented)" );
    MDTraceLeaveResult(E_NOTIMPL);
}


/*-----------------------------------------------------------------------------
/ IContextMenu methods
/----------------------------------------------------------------------------*/
STDMETHODIMP
CMyDocsFolder::QueryContextMenu( HMENU hmenu,
                                 UINT indexMenu,
                                 UINT idCmdFirst,
                                 UINT idCmdLast,
                                 UINT uFlags
                                )
{
    HRESULT hr = E_NOTIMPL;
    MENUITEMINFO mii;

    MDTraceEnter( TRACE_EXT, "CMyDocsFolder(IContextMenu)::QueryContextMenu" );

    m_HideCmd = 0;
    m_RestoreCmd = 0;

    if ((m_ext == EXT_IS_ROOT) && !(uFlags & CMF_DEFAULTONLY))
    {
        //
        // Do the context menu items for the desktop item...
        //

        TCHAR szText[ 50 ];
        TCHAR szTemp[ 128 ];  // I'm guessing no item in a menu is going to be bigger than this
        INT i,count;

        if (IsMyDocsHidden())
        {
            ExitGracefully( hr, E_FAIL, "My Docs is hidden on the desktop" );
        }

        // Get the text for the menu item to add...
        if (LoadString( g_hInstance, IDS_REMOVE_STRING, szText, ARRAYSIZE(szText))==0)
            ExitGracefully( hr, E_FAIL, "Couldn't load IDS_REMOVE_STRING" );

        // Make sure it doesn't already exist...
        count = GetMenuItemCount(hmenu);
        if (count == -1)
            ExitGracefully( hr, E_FAIL, "Couldn't GetMenuItemCount" );

        for (i = 0; i < count; i++)
        {
            if (GetMenuString( hmenu, i, szTemp, ARRAYSIZE(szTemp), MF_BYPOSITION ))
            {
                if (lstrcmp( szTemp, szText )==0)
                    ExitGracefully( hr, S_OK, "Menu item already exists..." );
            }
        }

        // add separator
        mii.cbSize = sizeof(mii);
        mii.fMask = MIIM_ID | MIIM_TYPE;
        mii.fType = MFT_SEPARATOR;
        mii.wID = idCmdFirst + ITEM_OFFSET_SEPARATOR;
        InsertMenuItem( hmenu, indexMenu, TRUE, &mii );

        // add remove item
        mii.cbSize = sizeof(mii);
        mii.fMask = MIIM_ID | MIIM_TYPE;
        mii.fType = MFT_STRING;
        mii.wID = idCmdFirst + ITEM_OFFSET_REMOVE;
        m_HideCmd = ITEM_OFFSET_REMOVE;
        mii.dwTypeData = szText;
        mii.cch = lstrlen( szText );
        InsertMenuItem( hmenu, indexMenu+1, TRUE, &mii );
        hr = ResultFromShort( ITEM_OFFSET_REMOVE + 1 );
    }
    else if ( (m_ext == EXT_IS_ROOT_PATH) &&
              (!(uFlags & CMF_DEFAULTONLY)) &&
              IsMyDocsHidden()
             )
    {
        //
        // Do the context menu items for the on-disk item...
        //

        TCHAR szText[ 50 ];
        TCHAR szTemp[ 128 ];  // my guess as to largest possible menu item
        INT i,count;

        // Get the text for the menu item to add...
        if (LoadString( g_hInstance, IDS_RESTORE_STRING, szText, ARRAYSIZE(szText))==0)
            ExitGracefully( hr, E_FAIL, "Couldn't load IDS_RESTORE_STRING" );

        // Make sure it doesn't already exist...
        count = GetMenuItemCount(hmenu);
        if (count == -1)
            ExitGracefully( hr, E_FAIL, "Couldn't GetMenuItemCount" );

        for (i = 0; i < count; i++)
        {
            if (GetMenuString( hmenu, i, szTemp, ARRAYSIZE(szTemp), MF_BYPOSITION ))
            {
                if (lstrcmp( szTemp, szText )==0)
                    ExitGracefully( hr, S_OK, "Menu item already exists..." );
            }
        }

        // add separator
        mii.cbSize = sizeof(mii);
        mii.fMask = MIIM_ID | MIIM_TYPE;
        mii.fType = MFT_SEPARATOR;
        mii.wID = idCmdFirst + ITEM_OFFSET_SEPARATOR;
        InsertMenuItem( hmenu, indexMenu, TRUE, &mii );

        // add restore item
        mii.cbSize = sizeof(mii);
        mii.fMask = MIIM_ID | MIIM_TYPE;
        mii.fType = MFT_STRING;
        mii.wID = idCmdFirst + ITEM_OFFSET_RESTORE;
        m_RestoreCmd = ITEM_OFFSET_RESTORE;
        mii.dwTypeData = szText;
        mii.cch = lstrlen( szText );
        InsertMenuItem( hmenu, indexMenu+1, TRUE, &mii );
        hr = ResultFromShort( ITEM_OFFSET_RESTORE + 1 );
    }

exit_gracefully:

    MDTraceLeaveResult(hr);
}

/*---------------------------------------------------------------------------*/
STDMETHODIMP
CMyDocsFolder::InvokeCommand( LPCMINVOKECOMMANDINFO lpici )
{
    HRESULT hr = S_OK;

    MDTraceEnter( TRACE_EXT, "CMyDocsFolder(QueryContextMenu)::InvokeCommand" );

    MDTrace(TEXT("lpVerb = 0x%x"), lpici->lpVerb );
    if ((HIWORD(lpici->lpVerb)==0) && (LOWORD(lpici->lpVerb)==m_HideCmd))
    {
        MDTrace(TEXT("Remove chosen!"));
        DoRemovePrompt( );
    }
    else if ((HIWORD(lpici->lpVerb)==0) && (LOWORD(lpici->lpVerb)==m_RestoreCmd))
    {
        TCHAR szPath[ MAX_PATH ];

        MDTrace(TEXT("Restore chosen!"));

        //
        // Try to set name to be current folder name...
        //

        if (GetPersonalPath( szPath, FALSE ))
        {
            LPTSTR pName= PathFindFileName( szPath );

            RegSetValue( HKEY_CURRENT_USER,
                         c_szPerUserCLSID,
                         REG_SZ,
                         (LPCTSTR)pName,
                         (lstrlen( pName ) + 1)*sizeof(TCHAR)
                        );
        }

        //
        // Now restore the folder...
        //

        RestoreMyDocsFolder( NULL, NULL, NULL, 0 );

    }

    MDTraceLeaveResult(hr);
}

/*---------------------------------------------------------------------------*/
STDMETHODIMP
CMyDocsFolder::GetCommandString( UINT idCmd,
                                 UINT uType,
                                 UINT * pwReserved,
                                 LPSTR pszName,
                                 UINT cchMax
                                )
{
    HRESULT hr = E_NOTIMPL;

    UINT id = 0;

    MDTraceEnter( TRACE_EXT, "CMyDocsFolder(IContextMenu)::GetCommandString" );

    if ((idCmd == m_HideCmd) && (uType == GCS_HELPTEXT))
    {
        TCHAR szFormat[ 64 ];

        if ( LoadString( g_hInstance,
                         IDS_REMOVE_COMMAND_STRING,
                         szFormat,
                         ARRAYSIZE(szFormat) )
            )
        {
            TCHAR szName[ MAX_PATH ];

            GetMyDocumentsDisplayName( szName, ARRAYSIZE(szName) );

            if ((lstrlen( szName ) + lstrlen( szFormat )) <= (INT)cchMax)
            {
                wsprintf( (LPTSTR)pszName, szFormat, szName );
                hr = S_OK;
            }
            else
            {
                hr = E_INVALIDARG;
            }

        }
    }
    else if ((idCmd == m_RestoreCmd) && (uType == GCS_HELPTEXT))
    {
        TCHAR szFormat[ 64 ];

        if ( LoadString( g_hInstance,
                         IDS_RESTORE_COMMAND_STRING,
                         szFormat,
                         ARRAYSIZE(szFormat) )
            )
        {
            TCHAR szBuffer[ MAX_PATH ];
            LPTSTR pName;

            GetPersonalPath( szBuffer, FALSE );
            pName = PathFindFileName( szBuffer );

            if ((lstrlen( pName ) + lstrlen( szFormat )) <= (INT)cchMax)
            {
                wsprintf( (LPTSTR)pszName, szFormat, pName );
                hr = S_OK;
            }
            else
            {
                hr = E_INVALIDARG;
            }

        }
    }

    MDTraceLeaveResult(hr);
}
