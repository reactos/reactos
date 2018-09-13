///////////////////////////////////////////////////////////////////////////////
//
// extinit.cpp
//      Explorer Font Folder extension routines
//
//
// History:
//      31 May 95 SteveCat
//          Ported to Windows NT and Unicode, cleaned up
//
//
// NOTE/BUGS
//
//  Copyright (C) 1992-1995 Microsoft Corporation
//
///////////////////////////////////////////////////////////////////////////////

//==========================================================================
//                              Include files
//==========================================================================

#include "priv.h"
#include "globals.h"

#include "extinit.h"
#include "resource.h"
#include "ui.h"
#include "cpanel.h"
#include "fontcl.h"
// #include "fontinfo.h"
#include "fontview.h"

#include "dbutl.h"


#define ARRAYSIZE(a)    (sizeof(a)/sizeof(a[0]))


//------------------------------------------------------------------------

HRESULT MyReleaseStgMedium( LPSTGMEDIUM pmedium )
{
    if( pmedium->pUnkForRelease )
    {
        pmedium->pUnkForRelease->Release( );
    }
    else
    {
        switch( pmedium->tymed )
        {
        case TYMED_HGLOBAL:
            GlobalFree( pmedium->hGlobal );
            break;

        case TYMED_ISTORAGE:
        case TYMED_ISTREAM:
            //
            // hack, the stream and storage overlap eachother in the union
            // so this works for both.
            //

            pmedium->pstm->Release( );
            break;

        default:
            //
            // Assert( 0 );        // unknown type
            // Not fullly implemented.

            MessageBeep( 0 );
                break;
        }
    }

    return NOERROR;
}


//------------------------------------------------------------------------
CShellExtInit::CShellExtInit( )
   :  m_cRef( 0 ),
      m_poData( NULL )
{
    g_cRefThisDll++;
}

CShellExtInit::~CShellExtInit( )
{
    if( m_poData )
    {
        m_poData->Release( );
        m_poData = NULL;
    }

    g_cRefThisDll--;
}

BOOL CShellExtInit::bInit( )
{
    return TRUE;
}

STDMETHODIMP CShellExtInit::QueryInterface( REFIID riid,
                                            LPVOID FAR* ppvObj )
{
    *ppvObj = NULL;

    DEBUGMSG( ( DM_NOEOL | DM_TRACE1,
              TEXT( "FONTEXT: CShellExtInit::QueryInterface called for: " ) ) );

    //
    // Dump out the riid here
    //

    DEBUGREFIID( ( DM_TRACE1, riid ) );

    if( riid == IID_IUnknown )
        *ppvObj = (IUnknown*)( (IShellExtInit *)this );

      // The (IShellExtInit *) ^^^ up there is to disambiguate :) the reference

    else if( riid == IID_IShellExtInit )
        *ppvObj = (IShellExtInit *) this;

    else if( riid == IID_IContextMenu )
        *ppvObj = (IContextMenu*)this;

    else if( riid == IID_IShellPropSheetExt )
        *ppvObj = (IShellPropSheetExt*)this;

    if( *ppvObj )
    {
        ( (LPUNKNOWN)*ppvObj)->AddRef( );
        return NOERROR;
    }

    return( ResultFromScode( E_NOINTERFACE ) );
}


STDMETHODIMP_(ULONG ) CShellExtInit::AddRef( void )
{
    DEBUGMSG( (DM_TRACE1, TEXT( "FONTEXT: CShellExtInit::AddRef called: %d->%d references" ),
              m_cRef, m_cRef + 1 ) );

    return( ++m_cRef );
}


STDMETHODIMP_(ULONG) CShellExtInit::Release( void )
{
    DEBUGMSG( (DM_TRACE1, TEXT( "FONTEXT: CShellExtInit::Release called: %d->%d references" ),
              m_cRef, m_cRef - 1 ) );

    ULONG retval;

    retval = --m_cRef;

    if( !retval ) delete this;

    return( retval );
}


STDMETHODIMP CShellExtInit::Initialize( LPCITEMIDLIST pidlFolder,
                                        LPDATAOBJECT lpdobj,
                                        HKEY hkeyProgID )
{
    if( m_poData )
        m_poData->Release( );

    m_poData = lpdobj;
    m_poData->AddRef( );

    return NOERROR;
}


STDMETHODIMP CShellExtInit::QueryContextMenu( HMENU hmenu,
                                              UINT indexMenu,
                                              UINT idCmdFirst,
                                              UINT idCmdLast,
                                              UINT uFlags )
{
    UINT  nCmd = idCmdFirst;
    TCHAR  szCmd[ 64 ];


    DEBUGMSG( (DM_TRACE2, TEXT( "FONTEXT: QueryContextMenu called with following:" ) ) );
    DEBUGMSG( (DM_TRACE2, TEXT( "         indexMenu:  %d" ), indexMenu ) );
    DEBUGMSG( (DM_TRACE2, TEXT( "         idCmdFirst: %d" ), idCmdFirst ) );
    DEBUGMSG( (DM_TRACE2, TEXT( "         uFlags:     %d" ), uFlags ) );

    LoadString( g_hInst, IDS_EXT_INSTALL, szCmd, ARRAYSIZE( szCmd ) );

    InsertMenu( hmenu, indexMenu++, MF_STRING|MF_BYPOSITION, nCmd++, szCmd );

    return (HRESULT)( 1 );
}

STDMETHODIMP CShellExtInit::InvokeCommand( LPCMINVOKECOMMANDINFO lpici )
{
   HRESULT hr   = ResultFromScode( E_INVALIDARG );
   UINT    nCmd = LOWORD( lpici->lpVerb );

   //
   // We only have one command: Install
   //

   if( !nCmd && m_poData )
   {
        //
        // The fact that we got here is success. The install may or may not work.
        //
        hr = NOERROR;

        InstallDataObject( m_poData, DROPEFFECT_COPY, lpici->hwnd );
   }

   return hr;
}

STDMETHODIMP CShellExtInit::GetCommandString( UINT_PTR idCmd,
                                              UINT    uFlags,
                                              UINT   *pwReserved,
                                              LPSTR   pszName,
                                              UINT    cchMax )
{
    HRESULT  hr = ResultFromScode( E_INVALIDARG );
    UINT  nID;

    if( !idCmd )
    {
        if( uFlags & GCS_HELPTEXT )
            nID = IDS_EXT_INSTALL_HELP;
        else
            nID = IDS_EXT_INSTALL;

        if( uFlags & GCS_UNICODE )
            if( LoadStringW( g_hInst, nID, (LPWSTR) pszName, cchMax ) )
                hr = NOERROR;
        else
            if( LoadStringA( g_hInst, nID, (LPSTR) pszName, cchMax ) )
                hr = NOERROR;
    }

    return hr;
}


//---------------------------------------------------------------------------
//
// FSPage_InitDialog
//
//  This function is called when the dialog procedure receives the
// WM_INITDIALOG message. In this sample code, we simply fill the
// listbox with the list of fully qualified paths to the file and
// directories.
//
//---------------------------------------------------------------------------

void FSPage_InitDialog( HWND hDlg, LPPROPSHEETPAGE psp )
{
    LPDATAOBJECT   poData = (LPDATAOBJECT)psp->lParam;

    FORMATETC fmte = {
                 CF_HDROP,
                 (DVTARGETDEVICE FAR *)NULL,
                 DVASPECT_CONTENT,
                 -1,
                 TYMED_HGLOBAL };

    STGMEDIUM medium;

    HRESULT hres = poData->GetData( &fmte, &medium );


    if( SUCCEEDED( hres ) )
    {
        HDROP        hDrop = (HDROP) medium.hGlobal;
        FONTDESCINFO fdi;
        TCHAR        szAll[ 512 ];

        fdi.dwFlags = FDI_ALL | FDI_VTC;

        ::DragQueryFile( hDrop, 0, fdi.szFile, ARRAYSIZE( fdi.szFile ) );

        if( bIsTrueType( &fdi ) || bIsNewExe( &fdi ) )
        {
            SetDlgItemText( hDlg, stc1, fdi.szDesc );

            //
            // Get the copyright info and put it in the edit control, edt1.
            //

            wsprintf( szAll, TEXT( "%s\r\n\r\n%s\r\n\r\n%s" ), fdi.lpszVersion,
                      fdi.lpszTrademark, fdi.lpszCopyright );

            SetDlgItemText( hDlg, edt1, szAll );

            //
            // Clean up the fdi.
            //

            if( fdi.lpszVersion )
                delete fdi.lpszVersion;

            if( fdi.lpszCopyright )
                delete fdi.lpszCopyright;

            if( fdi.lpszTrademark )
                delete fdi.lpszTrademark;
        }

        MyReleaseStgMedium( &medium );
    }
}


//---------------------------------------------------------------------------
//
// FSPage_DlgProc
//
//  The dialog procedure for the TEXT( "FSPage" ) property sheet page.
//
//---------------------------------------------------------------------------

INT_PTR CALLBACK FSPage_DlgProc( HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam )
{
    LPPROPSHEETPAGE psp = (LPPROPSHEETPAGE)GetWindowLongPtr( hDlg, DWLP_USER );

    switch( uMessage )
    {
    //
    //  When the shell creates a dialog box for a property sheet page,
    // it passes the pointer to the PROPSHEETPAGE data structure as
    // lParam. The dialog procedures of extensions typically store it
    // in the DWL_USER of the dialog box window.
    //
    case WM_INITDIALOG:
        SetWindowLongPtr( hDlg, DWLP_USER, lParam );

        psp = (LPPROPSHEETPAGE)lParam;

        FSPage_InitDialog( hDlg, psp );

        break;

    case WM_DESTROY:
        break;

    case WM_COMMAND:
        break;

    case WM_NOTIFY:
        switch( ( (NMHDR FAR *)lParam)->code )
        {
        case PSN_SETACTIVE:
           break;

        case PSN_APPLY:
           break;

        default:
           break;
        }
        break;

    default:
        return( FALSE );
    }

    return( TRUE );
}

//
//
//
#if 0
void CALLBACK FSPage_ReleasePage( LPPROPSHEETPAGE psp )
{
    GlobalFree( (HDROP)psp->lParam );
}
#endif


STDMETHODIMP CShellExtInit::AddPages( LPFNADDPROPSHEETPAGE lpfnAddPage,
                                      LPARAM lParam )
{
    HRESULT  hr = NOERROR;      // ResultFromScode( E_INVALIDARG );

    if( m_poData )
    {
        //
        // Get an HDROP, if possible.
        //
        FORMATETC fmte = {
                       CF_HDROP,
                       (DVTARGETDEVICE FAR *)NULL,
                       DVASPECT_CONTENT,
                       -1,
                       TYMED_HGLOBAL };

        STGMEDIUM medium;

        hr = m_poData->GetData( &fmte, &medium );

        if( SUCCEEDED( hr ) )
        {
            //
            // Only add a page if there is exactly one font selected.
            //

            HDROP hDrop = (HDROP) medium.hGlobal;
            UINT cnt = ::DragQueryFile( hDrop, (UINT)-1, NULL, 0 );

            if( cnt == 1 )
            {
                PROPSHEETPAGE  psp;
                HPROPSHEETPAGE hpage;

                psp.dwSize      = sizeof( psp );        // no extra data.
                psp.dwFlags     = PSP_USEREFPARENT;
                psp.hInstance   = g_hInst;
                psp.pszTemplate = MAKEINTRESOURCE( ID_DLG_PROPPAGE );
                psp.pfnDlgProc  = FSPage_DlgProc;
                psp.pcRefParent = (UINT *)&g_cRefThisDll;
              // psp.pfnRelease  = FSPage_ReleasePage;
                psp.lParam      = (LPARAM)m_poData;

                hpage = CreatePropertySheetPage( &psp );

                if( hpage )
                {
                    if( !lpfnAddPage( hpage, lParam ) )
                        DestroyPropertySheetPage( hpage );
                }
            }

            MyReleaseStgMedium( &medium );
        }
    }

    return hr;
}


STDMETHODIMP CShellExtInit::ReplacePage( UINT uPageID,
                                         LPFNADDPROPSHEETPAGE lpfnReplaceWith,
                                         LPARAM lParam )
{
    return NOERROR;
}

const TCHAR c_szFileNameMap[] = CFSTR_FILENAMEMAP;       // "FileNameMap"

VOID InstallDataObject( LPDATAOBJECT pdobj,
                        DWORD dwEffect,
                        HWND hWnd,
                        CFontView * poView)
{
    //
    // Get an HDROP, if possible.
    //

    FORMATETC fmte = {
                    CF_HDROP,
                    (DVTARGETDEVICE FAR *)NULL,
                    DVASPECT_CONTENT,
                    -1,
                    TYMED_HGLOBAL };

    STGMEDIUM medium;

    HRESULT hres = pdobj->GetData( &fmte, &medium );

    if( SUCCEEDED( hres ) )
    {
        WaitCursor     cWaiter;           // Starts and stops busy cursor
        STGMEDIUM      mediumNameMap;
        HDROP          hDrop = (HDROP) medium.hGlobal;
        BOOL           bAdded = FALSE;
        FullPathName_t szFile;

        UINT   cfFileNameMap = RegisterClipboardFormat( c_szFileNameMap );
        LPTSTR lpszNameMap   = NULL;

        fmte.cfFormat = (CLIPFORMAT) cfFileNameMap;

        if( pdobj->GetData( &fmte, &mediumNameMap ) == S_OK )
        {
            lpszNameMap = (LPTSTR) GlobalLock( mediumNameMap.hGlobal );
        }

        UINT cnt = ::DragQueryFile( hDrop, (UINT) -1, NULL, 0 );

        for( UINT i = 0; i < cnt; )
        {
            ::DragQueryFile( hDrop, i, szFile, ARRAYSIZE( szFile ) );

            if( poView )
                poView->StatusPush( szFile );

            switch( CPDropInstall( poView->GetViewWindow(),
                                   szFile,
                                   dwEffect,
                                   lpszNameMap,
                                   (int) (cnt - ++i) ) )
            {
            case CPDI_SUCCESS:
                bAdded = TRUE;
                break;

            case CPDI_FAIL:
                break;

            case CPDI_CANCEL:
                i = cnt;
                break;
            }

            if( lpszNameMap && *lpszNameMap )
            {
                lpszNameMap += lstrlen( lpszNameMap ) + 1;
            }
        }

        poView->StatusClear( );

        if( lpszNameMap )
        {
            GlobalUnlock( mediumNameMap.hGlobal );
            MyReleaseStgMedium( &mediumNameMap );
        }


        if( bAdded )
        {
          vCPWinIniFontChange( );

        }

        MyReleaseStgMedium( &medium );

    }
}
