/* sample source code for IE4 view extension
 *
 * Copyright Microsoft 1996
 *
 * This file implements the IShellFolderView interface
 */

#include "precomp.h"
#include <mbstring.h>
#include <tchar.h>
#include "atlimpl.cpp"

#define DEFINE_MAIN
#define CPP_FUNCTIONS
#include <crtfree.h>

#define ARRAYSIZE(x) (sizeof(x)/sizeof(x[0]))

CHAR const c_szRegPath[] = REGSTR_THUMBNAILVIEW;
CHAR const c_szAutoExtract[] = "AutoExtract";

UINT g_msgMSWheel = 0;
UINT g_uiShell32 = 0;
BOOL g_bMirroredOS = FALSE;

#define SetWindowBits SHSetWindowBits
#define REGSTR_APPROVED     "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Shell Extensions\\Approved"

void RegisterApprovedExtension( REFCLSID clsid, UINT idDescr );
void UnregisterApprovedExtension( REFCLSID clsid );

////////////////////////////////////////////////////////////////////////////
CThumbnailView::CThumbnailView()
{
    m_pBrowser = NULL;
    m_pCommDlg = NULL;
    m_pDefView = NULL;
    m_pFolder = NULL;
    m_pFolderCB = NULL;
    m_pDropTarget = NULL;
    m_pIcon = NULL;
    m_pDiskCache = NULL;
    m_pImageCache = NULL;

    m_pScheduler = NULL;

    m_hWndParent = NULL;
    m_hWnd = NULL;
    m_hWndListView = NULL;

    m_fTranslateAccel = FALSE;
    m_fExploreMode = FALSE;
    m_fDragStarted = FALSE;

    m_fAutoExtract = FALSE;
    m_fDrawBorder  = TRUE;
    m_fIconStamp = TRUE;

    m_hAccel = NULL;
    m_himlThumbs = NULL;

    m_ulShellRegId = 0;
    m_pidl = NULL;

    RegisterWindowClass();

    m_iXSizeThumbnail = DEFSIZE_THUMBNAIL;
    m_iYSizeThumbnail = DEFSIZE_THUMBNAIL;

    m_hSysLargeImgLst = NULL;
    m_hSysSmallImgLst = NULL;
    m_hpal = NULL;

    InitializeCriticalSection( &m_csAddLock );

    m_hAddList = DPA_Create(5);
    m_hDeleteList = DPA_Create(5);
    m_pidlRename = NULL;
}

////////////////////////////////////////////////////////////////////////////
CThumbnailView::~CThumbnailView()
{
    if ( m_pIcon )
    {
        m_pIcon->Release();
    }

    if ( m_pBrowser )
    {
        m_pBrowser->Release();
    }

    if ( m_pCommDlg )
    {
        m_pCommDlg->Release();
    }

    if ( m_pDefView )
    {
        m_pDefView->Release();
    }

    if ( m_pFolder )
    {
        m_pFolder->Release();
    }

    if ( m_pFolderCB )
    {
        m_pFolderCB->Release();
    }

    if ( m_pScheduler )
    {
        m_pScheduler->Release();
    }

    if ( m_pDropTarget )
    {
        m_pDropTarget->Release();
    }

    if ( m_pDiskCache )
    {
        m_pDiskCache->Release();
    }

    if ( m_pImageCache )
    {
        m_pImageCache->Release();
    }

    if ( m_hWnd )
    {
        DestroyWindow( m_hWnd );
    }

    if ( m_hWndListView )
    {
        DestroyWindow( m_hWndListView );
    }

    if ( m_pidl )
    {
        SHFree( (LPVOID) m_pidl );
    }

    if ( m_hpal )
    {
        DeletePalette( m_hpal );
    }

    if ( m_pidlRename )
    {
        SHFree(m_pidlRename);
    }
    // accelerator tables from resources do not need to be free...

    DeleteCriticalSection( &m_csAddLock );

    if ( m_hAddList )
    {
        for ( int iLoop = DPA_GetPtrCount( m_hAddList ) - 1; iLoop >= 0; iLoop -- )
        {
            SHFree( (LPITEMIDLIST) DPA_GetPtr(m_hAddList, iLoop ));
        }
        DPA_DeleteAllPtrs( m_hAddList );
        DPA_Destroy( m_hAddList );
    }
    if ( m_hDeleteList )
    {
        for ( int iLoop = DPA_GetPtrCount( m_hDeleteList ) - 1; iLoop >= 0; iLoop -- )
        {
            SHFree( (LPITEMIDLIST) DPA_GetPtr(m_hDeleteList, iLoop ));
        }
        DPA_DeleteAllPtrs( m_hDeleteList );
        DPA_Destroy( m_hDeleteList );
    }
}

////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CThumbnailView::SetViewWindowStyle(DWORD dwBits, DWORD dwVal)
{

    SetWindowBits(m_hWndListView, GWL_EXSTYLE, dwBits, dwVal);
    return S_OK;
}

////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CThumbnailView::SetViewWindowBkImage(LPCWSTR pwszImage)
{
    LVBKIMAGEW lvbki;

    if (pwszImage)
    {
        lvbki.ulFlags = LVBKIF_SOURCE_URL | LVBKIF_STYLE_TILE;
        lvbki.pszImage = (LPWSTR)pwszImage;
        ListView_SetBkImageWrapW(m_hWndListView, &lvbki);
    }
    return S_OK;
}

////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CThumbnailView::SetViewWindowColors(COLORREF clrText, COLORREF clrTextBk, COLORREF clrWindow)
{
    ListView_SetBkColor(m_hWndListView, clrWindow);
    ListView_SetTextBkColor(m_hWndListView, clrTextBk);
    ListView_SetTextColor(m_hWndListView, clrText);
    return S_OK;
}

////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CThumbnailView::SetOwnerDetails( IShellFolder * pSF, DWORD lParam )
{
    if ( m_pFolder )
    {
        m_pFolder->Release();
    }

    // remember the folder that we are to be displaying ....
    m_pFolder = pSF;

    if ( m_pFolder )
    {
        m_pFolder->AddRef();
        m_pFolder->QueryInterface( IID_IShellIcon, (LPVOID *) & m_pIcon );
    }

    if ( lParam != 0 )
    {
        // mask off the upper bits, the lowest one is the flag,
        // the upper bits just tell us whether it was set to zero, or not present
        m_fAutoExtract = lParam & 1;
        m_fDrawBorder = !(lParam & TV_NOBORDER);
        m_fIconStamp = !(lParam & TV_NOICONSTAMP);
    }
    else
    {
        // check the registry to see if we have a global setting..

        DWORD lReg = 0;
        DWORD cbSize = sizeof( lReg );
        DWORD dwType;

        LONG lRes = SHRegGetUSValueA( c_szRegPath,
            c_szAutoExtract, &dwType, &lReg, &cbSize, FALSE, NULL, 0 );

        if ( lRes == ERROR_SUCCESS && dwType == REG_DWORD )
        {
            m_fAutoExtract = lReg;
        }
    }

    return ( m_pFolder ? NOERROR : E_INVALIDARG );
}

////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CThumbnailView::IsModal()
{
    return m_fTranslateAccel ? S_OK : S_FALSE;
}
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
CComModule _Module;

BEGIN_OBJECT_MAP( ObjectMap )
    OBJECT_ENTRY( CLSID_ThumbnailViewExt, CThumbnailView )
    OBJECT_ENTRY( CLSID_HtmlThumbnailExtractor, CHtmlThumb )
    OBJECT_ENTRY( CLSID_OfficeGrfxFilterThumbnailExtractor, COfficeThumb )
    OBJECT_ENTRY( CLSID_DocfileThumbnailHandler, CDocFileHandler )
    OBJECT_ENTRY( CLSID_LnkThumbnailDelegator, CLnkThumb )
    OBJECT_ENTRY( CLSID_ShellThumbnailDiskCache, CThumbStore )
    OBJECT_ENTRY( CLSID_ThumbnailScaler, CThumbnailShrinker)
    OBJECT_ENTRY( CLSID_ThumbnailFCNHandler, CThumbnailFCNContainer )
END_OBJECT_MAP( )

HINSTANCE   g_hinstDll; // Make sure it's initialized by the DLL entry code!

/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

// from extract.cpp
extern LPBYTE g_pbCMAP;

extern "C"
BOOL WINAPI LibMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        _Module.Init(ObjectMap, hInstance);
        DisableThreadLibraryCalls(hInstance);
        g_hinstDll = hInstance;

        InitializeCriticalSection( &g_csTNGEN );

        // NT has this already defined, but as we must run same binary on both platforms,
        // then register it ...
        g_msgMSWheel = RegisterWindowMessageA("MSWHEEL_ROLLMSG");
        g_bMirroredOS = IS_MIRRORING_ENABLED();
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        if ( g_pbCMAP )
        {
            LocalFree( g_pbCMAP );
        }

        DeleteCriticalSection( &g_csTNGEN );

        _Module.Term();
    }
    {
        HINSTANCE hinst = GetModuleHandleA("SHELL32.DLL");
        if (hinst)
        {

            // NOTE: GetProcAddress always takes ANSI strings!
            DLLGETVERSIONPROC pfnGetVersion = (DLLGETVERSIONPROC)GetProcAddress(hinst, "DllGetVersion");
            DLLVERSIONINFO dllinfo;

            dllinfo.cbSize = sizeof(DLLVERSIONINFO);
            if (pfnGetVersion && pfnGetVersion(&dllinfo) == NOERROR)
                g_uiShell32 = dllinfo.dwMajorVersion;
        }
    }
    return TRUE;    // ok
}

/////////////////////////////////////////////////////////////////////////////
// Used to determine whether the DLL can be unloaded by OLE

STDAPI DllCanUnloadNow(void)
{
    return (_Module.GetLockCount()==0) ? S_OK : S_FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// Returns a class factory to create an object of the requested type

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    return _Module.GetClassObject(rclsid, riid, ppv);
}

/////////////////////////////////////////////////////////////////////////////
CHAR const c_rgszDocFileExts[][5] =
{
    ".doc",
    ".dot",
    ".xls",
    ".xlt",
    ".obd",
    ".obt",
    ".ppt",
    ".pot",
    ".mic",
    ".mix",
    ".fpx",
    ".mpp"
};

CHAR const c_rgszHtmlExts[][7] =
{
    ".html",
    ".htm",
    ".url",
    ".txt",
    ".mhtml",
    ".mht",
    ".xml",
    ".nws",
    ".eml"
};

// the Trident IImgCtx extractors are now in shdocvw....

// the office graphics filters extensions..
CHAR const c_rgszOffExts[][5] =
{
    ".cdr",
    ".cgm",
    ".drw",
    ".dfx",
    ".emf",
    ".eps",
    ".pcd",
    ".pcx",
    ".pct",
    ".tga",
    ".wpg"
};

CHAR const c_rgszOffTifExts[][5] =
{
    ".tif"
};

CHAR const c_rgszLnkExts[][5] =
{
    ".lnk"
};

CHAR const c_szThumbnailExtract[] = IIDSTR_IExtractImage;
CHAR const c_szDocFileCLSID[] = CLSIDSTR_DocfileThumbnailHandler;
CHAR const c_szBmpCLSID[] = CLSIDSTR_BmpThumbnailExtractor;
CHAR const c_szHtmlCLSID[] = CLSIDSTR_HtmlThumbnailExtractor;
CHAR const c_szOffCLSID[] = CLSIDSTR_OfficeGrfxFilterExtractor;
CHAR const c_szLnkCLSID[] = CLSIDSTR_LnkThumbnailDelegator;

// DllRegisterServer - Adds entries to the system registry

STDAPI DllRegisterServer(void)
{
    HRESULT hr = _Module.RegisterServer();
    if ( FAILED( hr ))
    {
        return hr;
    }

    RegisterApprovedExtension( CLSID_ThumbnailViewExt, IDS_THUMBNAILVIEW_DESC );
    RegisterApprovedExtension( CLSID_HtmlThumbnailExtractor, IDS_HTMLTHUMBEXTRACT_DESC );
    RegisterApprovedExtension( CLSID_OfficeGrfxFilterThumbnailExtractor, IDS_OFCTHUMBEXTRACT_DESC );
    RegisterApprovedExtension( CLSID_DocfileThumbnailHandler, IDS_DOCTHUMBEXTRACT_DESC );
    RegisterApprovedExtension( CLSID_LnkThumbnailDelegator, IDS_LNKTHUMBEXTRACT_DESC );

    hr = RegisterHandler( (LPCSTR) c_rgszHtmlExts,
                          ARRAYSIZE( c_rgszHtmlExts ),
                          7,
                          c_szThumbnailExtract,
                          c_szHtmlCLSID );
    if ( SUCCEEDED( hr ))
    {
        hr = RegisterHandler( (LPCSTR) c_rgszDocFileExts,
                              ARRAYSIZE( c_rgszDocFileExts ),
                              5,
                              c_szThumbnailExtract,
                              c_szDocFileCLSID );
    }

    if ( SUCCEEDED( hr ))
    {
        hr = RegisterHandler( (LPCSTR) c_rgszOffExts,
                              ARRAYSIZE( c_rgszOffExts ),
                              5,
                              c_szThumbnailExtract,
                              c_szOffCLSID );
        if ( !IsOS( OS_NT5 ))
        {
            hr = RegisterHandler( (LPCSTR) c_rgszOffTifExts,
                                  ARRAYSIZE( c_rgszOffTifExts ),
                                  5,
                                  c_szThumbnailExtract,
                                  c_szOffCLSID );
        }
    }
    if ( SUCCEEDED( hr ))
    {
        hr = RegisterHandler( (LPCSTR) c_rgszLnkExts,
                              ARRAYSIZE( c_rgszLnkExts ),
                              5,
                              c_szThumbnailExtract,
                              c_szLnkCLSID );
    }

    if ( SUCCEEDED( hr ))
    {
        // register the autoextract...
        HKEY hkExplorer;
        LONG lRes = RegCreateKeyA( HKEY_LOCAL_MACHINE, c_szRegPath, &hkExplorer );
        if ( lRes == ERROR_SUCCESS )
        {
            DWORD dwDefault = 1;
            lRes = RegSetValueExA( hkExplorer,
                                  c_szAutoExtract,
                                  NULL,
                                  REG_DWORD,
                                  (LPBYTE) &dwDefault,
                                   sizeof( dwDefault ));

        }
        hr = RegisterHTMLExtractor();
    }

    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Adds entries to the system registry

STDAPI DllUnregisterServer(void)
{
    HRESULT hr =_Module.UnregisterServer();
    if ( FAILED( hr ))
    {
        return hr;
    }

    UnregisterApprovedExtension( CLSID_ThumbnailViewExt );
    UnregisterApprovedExtension( CLSID_HtmlThumbnailExtractor );
    UnregisterApprovedExtension( CLSID_OfficeGrfxFilterThumbnailExtractor );
    UnregisterApprovedExtension( CLSID_DocfileThumbnailHandler );
    UnregisterApprovedExtension( CLSID_LnkThumbnailDelegator );

    hr = UnregisterHandler( (LPCSTR) c_rgszHtmlExts,
                            ARRAYSIZE( c_rgszHtmlExts ),
                            7,
                            c_szThumbnailExtract,
                            c_szHtmlCLSID );
    if ( SUCCEEDED( hr ))
    {
        hr = RegisterHTMLExtractor();
    }
    if ( SUCCEEDED( hr ))
    {
        hr = UnregisterHandler( (LPCSTR) c_rgszDocFileExts,
                                ARRAYSIZE( c_rgszDocFileExts ),
                                5,
                                c_szThumbnailExtract,
                                c_szDocFileCLSID );
    }

    if ( SUCCEEDED( hr ))
    {
        hr = UnregisterHandler( (LPCSTR) c_rgszOffExts,
                                ARRAYSIZE( c_rgszOffExts ),
                                5,
                                c_szThumbnailExtract,
                                c_szOffCLSID );
        if ( !IsOS( OS_NT5 ))
        {
            hr = RegisterHandler( (LPCSTR) c_rgszOffTifExts,
                                  ARRAYSIZE( c_rgszOffTifExts ),
                                  5,
                                  c_szThumbnailExtract,
                                  c_szOffCLSID );
        }
    }
    if ( SUCCEEDED( hr ))
    {
        hr = UnregisterHandler( (LPCSTR) c_rgszLnkExts,
                                ARRAYSIZE( c_rgszLnkExts ),
                                5,
                                c_szThumbnailExtract,
                                c_szLnkCLSID );
    }
    SHDeleteKeyA( HKEY_LOCAL_MACHINE, c_szRegPath );

    return hr;
}

void RegisterApprovedExtension( REFCLSID clsid, UINT idDescr )
{
    CHAR szCLSID[40];
    CHAR szDescr[MAX_PATH];

    HRESULT hr = SHStringFromCLSIDA( szCLSID, ARRAYSIZE(szCLSID), clsid );
    if ( SUCCEEDED( hr ))
    {
        HKEY hkey;

        LoadStringA( g_hinstDll, idDescr, szDescr, MAX_PATH );

        LONG lres = RegCreateKeyExA( HKEY_LOCAL_MACHINE,
                                    REGSTR_APPROVED,
                                    0,
                                    NULL,
                                    REG_OPTION_NON_VOLATILE,
                                    KEY_WRITE,
                                    NULL,
                                    &hkey,
                                    NULL );
        if ( lres == ERROR_SUCCESS )
        {
            RegSetValueExA( hkey,
                           szCLSID,
                           0,
                           REG_SZ,
                           (const LPBYTE) szDescr,
                           (lstrlenA( szDescr) + 1) * sizeof( CHAR ));
            RegCloseKey( hkey );
        }
    }
}

void UnregisterApprovedExtension( REFCLSID clsid )
{
    CHAR szCLSID[40];

    HRESULT hr = SHStringFromCLSIDA( szCLSID, ARRAYSIZE(szCLSID), clsid );
    if ( SUCCEEDED( hr ))
    {
        HKEY hkey;

        LONG lres = RegOpenKeyExA( HKEY_LOCAL_MACHINE,
                                  REGSTR_APPROVED,
                                  0,
                                  KEY_WRITE,
                                  &hkey );
        if ( lres == ERROR_SUCCESS )
        {
            RegDeleteValueA( hkey, szCLSID);
            RegCloseKey( hkey );
        }
    }
}

