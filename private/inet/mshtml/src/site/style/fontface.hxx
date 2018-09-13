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

#ifndef I_FONTFACE_HXX_
#define I_FONTFACE_HXX_
#pragma INCMSG("--- Beg 'fontface.hxx'")

#define _hxx_
#include "fontface.hdl"

class CStyleSheet;
class CDwnChan;
class CBitsCtx;

MtExtern(CFontFace)

class CFontFace : public CBase
{
    DECLARE_CLASS_TYPES(CFontFace, CBase)
    
public:

    DECLARE_TEAROFF_TABLE(IDispatchEx)
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CFontFace))

    CFontFace( CStyleSheet *pParentStyleSheet, LPCTSTR pcszFaceName );
    ~CFontFace();

    HRESULT SetProperty( const TCHAR *pcszPropName, const TCHAR *pcszValue );
    HRESULT StartDownload( void );
    HRESULT StartFontDownload( void );

public:     // Accessors
    inline LPCTSTR GetFriendlyName( void ) { return _pszFaceName; };
    inline LPCTSTR GetInstalledName( void ) { return _pszInstalledName; };
    inline BOOL IsInstalled( void ) { return _fFontLoadSucceeded; };
    inline const CStyleSheet *ParentStyleSheet( void ) { return _pParentStyleSheet; };
    inline const LPCTSTR GetSrc( void ) {
            LPCTSTR pcsz = NULL;
            if ( _pAA )
                _pAA->FindString ( DISPID_A_FONTFACESRC, &pcsz );
            return pcsz; }

    void   StopDownloads();
    
    // for faulting (JIT) in t2embed
    NV_DECLARE_ONCALL_METHOD(FaultInT2, faultint2, (DWORD_PTR));

protected:  // functions

#ifndef NO_FONT_DOWNLOAD

    void SetBitsCtx(CBitsCtx * pBitsCtx);
    void OnDwnChan(CDwnChan * pDwnChan);
    static void CALLBACK OnDwnChanCallback(void * pvObj, void * pvArg)
        { ((CFontFace *)pvArg)->OnDwnChan((CDwnChan *)pvObj); }
    HRESULT InstallFont( LPCTSTR pcszPathname );
    void CookupInstalledName( void * p );

#endif

protected:  // data
    TCHAR *_pszFaceName;
    TCHAR _pszInstalledName[ LF_FACESIZE ];

    //CAttrArray  *_pAA;
    CStyleSheet *_pParentStyleSheet;
    CBitsCtx    *_pBitsCtx;                // The bitsctx used to download this font
    HANDLE       _hEmbeddedFont;   // The handle of the embedded font.  (This is NOT an HFONT!! It's a handle to an internal data structure.)
    BOOL         _fFontLoadSucceeded;
    BOOL         _fFontDownloadInterrupted;
    DWORD        _dwStyleCookie;

public:
    struct CLASSDESC
    {
        CBase::CLASSDESC _classdescBase;
        void*_apfnTearOff;
    };

    #define _CFontFace_
    #include "fontface.hdl"

    DECLARE_CLASSDESC_MEMBERS;
};

#pragma INCMSG("--- End 'fontface.hxx'")
#else
#pragma INCMSG("*** Dup 'fontface.hxx'")
#endif
