//+---------------------------------------------------------------------------
//
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 1994-1997
//
//  File:       FontFace.cxx
//
//  Contents:   Support for Cascading Style Sheets "@font-face"
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_FONTFACE_HXX_
#define X_FONTFACE_HXX_
#include "fontface.hxx"
#endif

#ifndef X_STYLE_HXX_
#define X_STYLE_HXX_
#include "style.hxx"
#endif

#ifndef X_SHEETS_HXX_
#define X_SHEETS_HXX_
#include "sheets.hxx"
#endif

#ifndef X_ELEMENT_HXX_
#define X_ELEMENT_HXX_
#include "element.hxx"
#endif

#ifndef X_T2EMBAPI_H_
#define X_T2EMBAPI_H_
#include "t2embapi.h"
#endif

#ifndef X_T2EMWRAP_HXX_
#define X_T2EMWRAP_HXX_
#include "t2emwrap.hxx"
#endif

#ifndef X_DOWNLOAD_HXX_
#define X_DOWNLOAD_HXX_
#include "download.hxx"
#endif

#ifndef X_ASSOC_HXX_
#define X_ASSOC_HXX_
#include "assoc.hxx"
#endif

#define _cxx_
#include "fontface.hdl"

MtDefine(CFontFace, StyleSheets, "CFontFace")
MtDefine(CFontFaceName, StyleSheets, "CFontFace::_pszFaceName")
MtDefine(CFontFaceRefStr, StyleSheets, "CFontFace::InstallFont (temp)")

const CFontFace::CLASSDESC CFontFace::s_classdesc =
{
    {
        &CLSID_HTMLStyleFontFace,            // _pclsid
        0,                                   // _idrBase
#ifndef NO_PROPERTY_PAGE
        NULL,                                // _apClsidPages
#endif // NO_PROPERTY_PAGE
        NULL,                                // _pcpi
        0,                                   // _dwFlags
        &IID_IHTMLStyleFontFace,             // _piidDispinterface
        &s_apHdlDescs                        // _apHdlDesc
    },
    (void *)s_apfnpdIHTMLStyleFontFace                 // _apfnTearOff
};


CFontFace::CFontFace(
    CStyleSheet *pParentStyleSheet,
    LPCTSTR pcszFaceName )
    : _pParentStyleSheet( pParentStyleSheet )
{
    if(_pParentStyleSheet)
        _pParentStyleSheet->PrivateAddRef();

    if ( pcszFaceName )
    {
        _pszFaceName = (TCHAR *)MemAlloc(Mt(CFontFaceName), (_tcslen( pcszFaceName ) + 1)*sizeof(TCHAR) );
        if ( _pszFaceName )
            _tcscpy( _pszFaceName, pcszFaceName );
    }
    else
        _pszFaceName = NULL;
    *_pszInstalledName = 0; // Terminate the string
    _hEmbeddedFont = NULL;
    _fFontLoadSucceeded = FALSE;
    _fFontDownloadInterrupted = FALSE;
    _pBitsCtx = NULL;
    _pAA = NULL;
    _dwStyleCookie = 0;
}

CFontFace::~CFontFace()
{
    if ( _pszFaceName )
      MemFree( _pszFaceName ); //free
    if ( _pAA )
        delete( _pAA );
    _pAA = NULL;

#ifndef NO_FONT_DOWNLOAD
    GWKillMethodCall(this, ONCALL_METHOD(CFontFace, FaultInT2, faultinusp), 0);
    if ( _hEmbeddedFont )
    {
        ULONG ulStatus;

        if ( E_NONE != T2DeleteEmbeddedFont( _hEmbeddedFont, 0, &ulStatus ) )
        {
            Assert( "Couldn't unload downloaded font!" );
        }
    }

    SetBitsCtx(NULL);
#endif

    if(_pParentStyleSheet)
        _pParentStyleSheet->PrivateRelease();

#ifdef IF_THIS_MOVES_INTO_FREE
    _pszFaceName = NULL;
    *_pszInstalledName = 0;
    _pParentStyleSheet = NULL;
    _hEmbeddedFont = NULL;
#endif
}

HRESULT CFontFace::SetProperty( const TCHAR *pcszPropName, const TCHAR *pcszValue )
{
    HRESULT hr = S_OK;

    if ( !_tcslen( pcszPropName ) )
        return S_FALSE;

    const PROPERTYDESC * found = FindPropDescForName ( pcszPropName );

    if ( !StrCmpIC( pcszPropName, _T( "font-family" ) ) )
    {
        if ( _pszFaceName )
          MemFree( _pszFaceName ); //free
        _pszFaceName = (TCHAR *)MemAlloc(Mt(CFontFaceName), (_tcslen( pcszValue ) + 1)*sizeof(TCHAR) );
        if ( _pszFaceName )
        {
            _tcscpy( _pszFaceName, pcszValue );
            TCHAR *pszFace = _pszFaceName;
            while ( *pszFace && ( *pszFace != _T(',') ) )
                pszFace++;
            if ( *pszFace == _T(',') )
            {
                *pszFace-- = _T('\0');
                while ( ( pszFace >= _pszFaceName ) && _istspace( *pszFace ) )
                    *pszFace-- = _T('\0');
            }
        }
        goto Cleanup;
    }

    if ( found && ( found->pfnHandleProperty ) )
    {
        // Try and parse attribute
#ifdef WIN16
        hr = THR ( (found->pfnHandleProperty)((PROPERTYDESC *)found,
            HANDLEPROP_SETHTML, /*BUGBUG: This opcode may be incorrect when we expose these through the OM. */
            (CVoid *)pcszValue, NULL, (CVoid *)&_pAA ) );
#else
        hr = THR ( CALL_METHOD( found, found->pfnHandleProperty, (
            HANDLEPROP_SETHTML, /*BUGBUG: This opcode may be incorrect when we expose these through the OM. */
            (CVoid *)pcszValue, NULL, (CVoid *)&_pAA ) ));
#endif
        if (!hr)
        {
            // got a match
            goto Cleanup;
        }
    }

    // BUGBUG: Need to handle expandos - either here, or (more preferably) in a generic fashion in the CAtBlockHandler code.

Cleanup:
    RRETURN1(hr, S_FALSE);
}

#ifdef NO_FONT_DOWNLOAD
HRESULT CFontFace::StartDownload( void )
{
    return S_OK;
}

#else // NO_FONT_DOWNLOAD

static inline void FillClassSpec(uCLSSPEC *pclasspec)
{
    pclasspec->tyspec = TYSPEC_FILENAME;
    pclasspec->tagged_union.pFileName = _T("{630b1da0-b465-11d1-9948-00c04f98bbc9}");
}

HRESULT CFontFace::StartDownload( void )
{
    CDoc *pDoc = _pParentStyleSheet->GetElement()->Doc();
    QUERYCONTEXT qcNotUsed;
    uCLSSPEC classpec;
    LPCTSTR  pcszURL = NULL;
    HRESULT hr = E_FAIL;
    extern DYNPROC g_dynprocT2EmbedLoadFont;
    //
    // Do we have a source URL from which to download?
    //
    if ( !_pAA || !_pAA->FindString ( DISPID_A_FONTFACESRC, &pcszURL ) )
        goto Cleanup;   // No SRC!
    
    //
    // First check if t2embed has already on the system.  Note that the user
    // may have independently loaded t2embed.dll, in which case we just use
    // that version.  Otherwise we would require a cabinet containing other
    // features the user may not need.
    //

    hr = THR_NOTRACE(LoadProcedure(&g_dynprocT2EmbedLoadFont));
    if (FAILED(hr))
    {
        FillClassSpec(&classpec);
        hr = THR(FaultInIEFeatureHelper(pDoc->GetHWND(), &classpec, &qcNotUsed, FIEF_FLAG_PEEK) );
    }
    
    //
    // If it has already been downloaded, then ask t2 to bring in the font.
    //
    if (hr == S_OK)
    {
        hr = THR(StartFontDownload());
    }
    else if (hr == HRESULT_FROM_WIN32(ERROR_PRODUCT_UNINSTALLED))
    {
        hr = GWPostMethodCall(this, ONCALL_METHOD(CFontFace, FaultInT2, faultinusp), 0, FALSE, "CFontFace::FaultInT2");
    }
    else
        hr = S_FALSE;

Cleanup:    
    RRETURN2(hr, S_FALSE, E_FAIL);
    }

void CFontFace::FaultInT2(DWORD_PTR dwContext)
{
    CDoc *pDoc = _pParentStyleSheet->GetElement()->Doc();
    uCLSSPEC classpec;
    HRESULT hr = S_OK;
    ULONG   cDie;
    
    FillClassSpec(&classpec);

    _fFontDownloadInterrupted = FALSE;
    cDie = pDoc->_cDie;
    pDoc->SubAddRef();
    PrivateAddRef();
    
    hr = THR(FaultInIEFeatureHelper(pDoc->GetHWND(), &classpec, NULL, 0));

    //
    // Irrespective of what Fault* returns we have to restart rendering! However
    // if the document cycled thru a UnloadContents then we are not interested
    // either enabling rendering or in starting font download.
    //
    if (cDie == pDoc->_cDie)
    {
        //
        // If the style element was blown away (say thru OM timer event) while
        // we were downloading (only the element was blown away -- the doc was
        // left intact and hence was not caught in the check for cDie)
        // then do not do anything since its pointless!
        //
        if (!_fFontDownloadInterrupted)
        {
            //
            // If we successfully downloaded the t2 dll, then we should proceed with
            // the download of the font.
            //
            if (S_OK == hr)
            {
                hr = StartFontDownload();
            }
        }
    }
    
    // And yes, don't leak anything :-)
    PrivateRelease();
    pDoc->SubRelease();
}

HRESULT CFontFace::StartFontDownload( void )
{
    CBitsCtx   *pBitsCtx = NULL;
    HRESULT     hr = E_FAIL;
    LPCTSTR     pcszURL = NULL;
    CElement   *pElement = _pParentStyleSheet->GetElement();
    BOOL        fAllow;
    TCHAR      *pchFontSrcURL = NULL;
    CDoc       *pDoc = pElement->Doc();

    if (!_pAA || !_pAA->FindString ( DISPID_A_FONTFACESRC, &pcszURL ) )
        goto Cleanup;
    
    _fFontLoadSucceeded = FALSE;

    hr = THR(pDoc->ProcessURLAction(
            URLACTION_HTML_FONT_DOWNLOAD,
            &fAllow));
    if (hr)
        RRETURN(hr);

    if (!fAllow)
        RRETURN(E_ACCESSDENIED);

    if (_pParentStyleSheet->_achAbsoluteHref)
    {
        hr = ExpandUrlWithBaseUrl(_pParentStyleSheet->_achAbsoluteHref,
                                  pcszURL,
                                  &pchFontSrcURL);
        if (hr)
            goto Cleanup;
    }

    // Kick off the download of the imported sheet
    hr = THR(_pParentStyleSheet->GetDocument()->NewDwnCtx(DWNCTX_FILE,
                _pParentStyleSheet->_achAbsoluteHref ? pchFontSrcURL : pcszURL,
                pElement,
                (CDwnCtx **)&pBitsCtx));
    if ( hr == S_OK )
    {
        // For rendering purposes, having an @imported sheet pending is just like having
        // a linked sheet pending.
        pDoc->EnterStylesheetDownload(&_dwStyleCookie);
        
        // Give ownership of bitsctx to the newly created (empty) stylesheet, since it's
        // the one that will need to be filled in by the @import'ed sheet.
        SetBitsCtx( pBitsCtx );
    }

    if (pBitsCtx)
        pBitsCtx->Release();

Cleanup:
    if (pchFontSrcURL)
        MemFreeString(pchFontSrcURL);
    return hr;
}

//*********************************************************************
//  CFontFace::SetBitsCtx()
//  Sets ownership and callback information for a BitsCtx.  A FontFace
//  will have a non-NULL BitsCtx if it's being downloaded.
//*********************************************************************
void CFontFace::SetBitsCtx( CBitsCtx * pBitsCtx )
{
    CElement *pElement = _pParentStyleSheet->GetElement();

    if (_pBitsCtx)
        _pBitsCtx->Release();

    _pBitsCtx = pBitsCtx;

    if (pBitsCtx)
    {
        CDoc *    pDoc = pElement->Doc();

        pBitsCtx->AddRef();

        // If the bits are here then just call OnDwnChan.
        // This will start handling the font (e.g., installing it).

        if (pBitsCtx->GetState() & (DWNLOAD_COMPLETE | DWNLOAD_ERROR | DWNLOAD_STOPPED))
            OnDwnChan(pBitsCtx);
        else
        {
            pBitsCtx->SetProgSink(pDoc->GetProgSink());
            pBitsCtx->SetCallback(OnDwnChanCallback, this);
            pBitsCtx->SelectChanges(DWNCHG_COMPLETE, 0, TRUE);
        }
    }
}

//*********************************************************************
//  CFontFace::OnChan()
//  Callback used by BitsCtx once it's downloaded our font file.
//*********************************************************************
void CFontFace::OnDwnChan(CDwnChan * pDwnChan)
{
    ULONG ulState = _pBitsCtx->GetState();
    CMarkup *pMarkup = _pParentStyleSheet->GetElement()->GetMarkup();
    CDoc *pDoc;

    Assert(pMarkup);
    pDoc = pMarkup->Doc();

    if (ulState & (DWNLOAD_COMPLETE | DWNLOAD_ERROR | DWNLOAD_STOPPED))
    {
        pDoc->LeaveStylesheetDownload(&_dwStyleCookie);
        _pBitsCtx->SetProgSink(NULL); // detach download from document's load progress
    }

    if (ulState & DWNLOAD_COMPLETE)
    {
        LPTSTR szFile = NULL;
        if ( S_OK == _pBitsCtx->GetFile(&szFile) )
        {
            // If unsecure download, may need to remove lock icon on Doc
            pDoc->OnSubDownloadSecFlags(_pBitsCtx->GetUrl(), _pBitsCtx->GetSecFlags());
            
            Assert( szFile );       // GetFile should always yield a valid string if it rets S_OK

            if ( S_OK == InstallFont( szFile ) )
            {
                // (this is not always a stable moment)
                IGNORE_HR( pMarkup->OnCssChange( /*fStable = */ FALSE, /* fRecomputePeers = */FALSE) );
            }
            else
                pDoc->Invalidate(); // So we'll update the screen.
            MemFreeString( szFile );
        }
    }
    else
    {
        pDoc->Invalidate(); // So we'll update the screen.
        TraceTag((tagError, "CFontFace::OnChan bitsctx failed to complete!"));
    }
}

unsigned long __cdecl FontReadCallback( void *pvFile, void *pvData, const unsigned long ulBytes )
{
    HFILE hFile = (HFILE)(DWORD_PTR)pvFile;
    return ( _lread( hFile, pvData, ulBytes ) );
}

//*********************************************************************
//  CFontFace::InstallFont()
//      Method used to load up the font embedding DLL and attempt to
//  install the font.
//*********************************************************************
HRESULT CFontFace::InstallFont( LPCTSTR pcszPathname )
{
    HRESULT hr = S_FALSE;
    HANDLE hFile = CreateFile( pcszPathname, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL );

    hr = S_FALSE;   // Still haven't loaded the font.
    if ( hFile != INVALID_HANDLE_VALUE)
    {
        TTLOADINFO ttli;
        CDoc *pDoc = _pParentStyleSheet->GetElement()->Doc();
        Assert( pDoc != NULL );
        TCHAR *pchBaseUrl = NULL;

        LPCTSTR pcszDocUrl = ( pDoc->IsPrintDoc() && (S_OK == pDoc->GetBaseUrl( &pchBaseUrl ))) ? pchBaseUrl : pDoc->_cstrUrl;
        
        CookupInstalledName(pDoc);

        ttli.usStructSize = sizeof(TTLOADINFO);

        hr = THR(MemAllocString(Mt(CFontFaceRefStr), pcszDocUrl, &ttli.pusRefStr ) );
        if ( hr == S_OK )
        {
            hr = S_FALSE;   // Still haven't finished.
            ttli.usRefStrSize = _tcslen( pcszDocUrl ) + 1;
            ULONG ulPrivStatus, ulStatus;
            char acMacName[ LF_FACESIZE * 2 + 1 ];

            WideCharToMultiByte( CP_ACP, 0, _pszInstalledName, -1, acMacName, LF_FACESIZE*2, NULL, NULL );

            switch ( T2LoadEmbeddedFont( &_hEmbeddedFont, TTLOAD_PRIVATE, &ulPrivStatus, LICENSE_DEFAULT, &ulStatus,
                                                        FontReadCallback, (void *)hFile, _pszInstalledName,
                                                        acMacName, &ttli ) )
            {
            case E_FONTNAMEALREADYEXISTS:   // We must have already downloaded this font for this document.
                _hEmbeddedFont = NULL;
                // Intentional fall-through
            case E_NONE:
                // Whoo-hoo!  The font has been loaded!
                _fFontLoadSucceeded = TRUE;
                hr = S_OK;
            }
            MemFreeString( ttli.pusRefStr );
        }
        CloseHandle( hFile );
    }
    RRETURN1( hr, S_FALSE );
}

//+----------------------------------------------------------------------------
//
//  Function:   CFontFace::CookupInstalledName
//
//  Synopsis:   Populate CFontFace::_pszInstalledName
//
//  Note:       Emperically we have determined that AddFontResource will fail
//              if the internal name is too long.  For these systems, we'll use
//              the old IE4-style internal name, which will fail if we have a
//              non-ASCII Unicode value.  Most fonts have an ASCII external
//              name, so this isn't usually a problem.
//
//-----------------------------------------------------------------------------

#define SIZEOF_COLOR 7 // syntax for <c>: #rrggbb

void
CFontFace::CookupInstalledName( void * p )
{
    TCHAR szTmpPtr[ 20 ];   // We use the document ptr as a hash on the installed name, so
                            // font-faces can be named the same across pages without conflict.
                            // We also use the current process ID to avoid naming conflicts
                            // across processes - although this is highly improbably, it is
                            // possible.


    if (g_dwPlatformID == VER_PLATFORM_WIN32_WINDOWS && g_dwPlatformVersion == 0x00040000)
    {
        const size_t cch = LF_FACESIZE - SIZEOF_COLOR - SIZEOF_COLOR - 1;
        _tcsncpy( _pszInstalledName, _pszFaceName, cch );
        _pszInstalledName[ cch ] = _T('\0');

        // Add the document pointer.

        Format( 0, szTmpPtr, 19, _T("<0c>"), p );
        _tcscat( _pszInstalledName, szTmpPtr );
    }
    else
    {
        DWORD dwFontNameHash;

        //
        // Embedded fonts are saved under a generated name which is a hash of a bunch of things.
        // First, there is a hash of the original font-name itself.  Then there is 
        // the location of the pDoc object (in Hex), and then the process Id.  The latter
        // two are added to prevent different pages from sharing fonts, as this would be a security
        // flaw.  The font name is hashed so that if two fonts differ only at the end, we still
        // register them under different names.
        //

        // BUGBUG (paulpark) Eventually, this code will break since we are relying on a hash.
        // If two (different) font-names hash to the same value, this will break, and we will 
        // display something in an incorrect font.

        dwFontNameHash = HashString( _pszFaceName, _tcslen(_pszFaceName), 0 );
        Format( 0, szTmpPtr, 19, _T("<0x>"), dwFontNameHash );
        _tcscpy( _pszInstalledName, szTmpPtr );

        // Run it twice to get a (sort-of) 64 bit hash.

        dwFontNameHash = HashString( _pszFaceName, _tcslen(_pszFaceName), dwFontNameHash );
        Format( 0, szTmpPtr, 19, _T("<0x>"), dwFontNameHash );
        _tcscat( _pszInstalledName, szTmpPtr );
        // Add the document pointer.

        Format( 0, szTmpPtr, 19, _T("<0x>"), p );
        _tcscat( _pszInstalledName, szTmpPtr );
    }

    // Add the process ID just to be safe.

    Format( 0, szTmpPtr, 19, _T("<0c>"), GetCurrentProcessId() );
    _tcscat( _pszInstalledName, szTmpPtr );
}

void
CFontFace::StopDownloads()
{
    GWKillMethodCall(this, ONCALL_METHOD(CFontFace, FaultInT2, faultinusp), 0);
    _fFontDownloadInterrupted = TRUE;
}

#endif // !NO_FONT_DOWNLOAD
