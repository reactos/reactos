//+---------------------------------------------------------------------------
//
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 1994-1997
//
//  File:       AtBlocks.cxx
//
//  Contents:   Support for Cascading Style Sheets "atblocks" - e.g., "@page" and "@media" definitions.
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

#ifndef X_ATBLOCKS_HXX_
#define X_ATBLOCKS_HXX_
#include "atblocks.hxx"
#endif

#ifndef X_CBUFSTR_HXX_
#define X_CBUFSTR_HXX_
#include "cbufstr.hxx"
#endif

MtDefine(CAtPage, StyleSheets, "CAtPage")
MtDefine(CAtMedia, StyleSheets, "CAtMedia")
MtDefine(CAtFontFace, StyleSheets, "CAtFontFace")
MtDefine(CAtNamespace, StyleSheets, "CAtNamespace")
MtDefine(CAtUnknown, StyleSheets, "CAtUnknown")
MtDefine(CAtUnknownInfo, StyleSheets, "CAtUnknownInfo")

DeclareTag(tagCSSAtBlocks, "Stylesheets", "Dump '@' blocks")



EMediaType CSSMediaTypeFromName(LPCTSTR szMediaName)
{
    if(!szMediaName || !(*szMediaName))
        return MEDIA_NotSet;

    for(int i = 0; i < ARRAY_SIZE(cssMediaTypeTable); i++)
    {
        if(_tcsiequal(szMediaName, cssMediaTypeTable[i]._szName))
            return cssMediaTypeTable[i]._mediaType;
    }

    return MEDIA_Unknown;
}


//+----------------------------------------------------------------------------
//
//  Class:  CAtPage
//
//-----------------------------------------------------------------------------

CAtPage::CAtPage( CCSSParser *pParser ) : CAtBlockHandler( pParser )
{
}

CAtPage::~CAtPage()
{
}

HRESULT CAtPage::SetProperty( LPCTSTR pszName, LPCTSTR pszValue, BOOL fImportant )
{
#if DBG==1
    if (IsTagEnabled(tagCSSAtBlocks))
    {
        CStr cstr;
        cstr.Set( _T("AtPage::SetProperty( \"") );
        cstr.Append( pszName );
        cstr.Append( _T("\", \"") );
        cstr.Append( pszValue );
        cstr.Append( fImportant ? _T("\" ) (important)\r\n") : _T("\" )\r\n") );
        OutputDebugString( cstr );
    }
#endif
    return S_FALSE;
}

HRESULT CAtPage::EndStyleRule( CStyleRule *pRule)
{
#if DBG==1
    if (IsTagEnabled(tagCSSAtBlocks))
    {
        OutputDebugStringA("AtPage::EndStyleRule()\r\n" );
    }
#endif
    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Class:  CAtMedia
//
//-----------------------------------------------------------------------------

CAtMedia::CAtMedia( CCSSParser *pParser, LPCTSTR pszName, CStyleSheet *pStyleSheet) 
                : CAtBlockHandler( pParser )
{
    AssertSz(pszName && *pszName, "Media type is empty");
    Assert(pParser);
 
    _ePrevMediaType = _ePrevAtMediaType = MEDIA_NotSet;

    _pStyleSheet = pStyleSheet;

    _dwFlags = ATBLOCKFLAGS_MULTIPLERULES;
    if(pStyleSheet)
    {
        EMediaType eMediaType = (EMediaType)TranslateMediaTypeString(pszName);
        if(eMediaType != MEDIA_NotSet)
        {
            _ePrevMediaType   = pStyleSheet->GetMediaTypeValue();
            _ePrevAtMediaType = pStyleSheet->GetLastAtMediaTypeValue();
            pStyleSheet->SetMediaTypeValue( (EMediaType)(eMediaType & _ePrevMediaType) );
            // Save the last media type for serializaton purposed
            if(_ePrevAtMediaType != MEDIA_NotSet)
            {
                eMediaType = (EMediaType)(eMediaType & _ePrevAtMediaType);
                if(eMediaType == MEDIA_NotSet)
                    eMediaType = MEDIA_Unknown;
            }
            pStyleSheet->SetLastAtMediaTypeValue(eMediaType);
        }
    }

}

CAtMedia::~CAtMedia()
{
    if(_pStyleSheet)
    {
        // Restore the saved previous at block media types
         _pStyleSheet->SetMediaTypeValue(_ePrevMediaType);
         // Restore the previous applied media type value (& ed value)
         _pStyleSheet->SetLastAtMediaTypeValue(_ePrevAtMediaType);
    }
}

HRESULT CAtMedia::SetProperty( LPCTSTR pszName, LPCTSTR pszValue, BOOL fImportant )
{
#if DBG==1
    if (IsTagEnabled(tagCSSAtBlocks))
    {
        CStr cstr;
        cstr.Set( _T("AtMedia::SetProperty( \"") );
        cstr.Append( pszName );
        cstr.Append( _T("\", \"") );
        cstr.Append( pszValue );
        cstr.Append( fImportant ? _T("\" ) (important)\r\n") : _T("\" )\r\n") );
        OutputDebugString( cstr );
    }
#endif
    return S_OK;
}

HRESULT CAtMedia::EndStyleRule( CStyleRule *pRule)
{
#if DBG==1
    if (IsTagEnabled(tagCSSAtBlocks))
    {
        OutputDebugStringA("AtMedia::EndStyleRule()\n");
    }
#endif
    
    return S_OK;
}



//+----------------------------------------------------------------------------
//
//  Class:  CAtFontFace
//
//-----------------------------------------------------------------------------

CAtFontFace::CAtFontFace( CCSSParser *pParser, LPCTSTR pszName ) : CAtBlockHandler( pParser )
{
    Assert( pParser && pParser->GetStyleSheet() );
    
    _pFontFace = new CFontFace( pParser->GetStyleSheet(), pszName );
    if ( _pFontFace )
        pParser->GetStyleSheet()->GetRootContainer()->_apFontFaces.Append( _pFontFace );
}

CAtFontFace::~CAtFontFace()
{
}

HRESULT CAtFontFace::SetProperty( LPCTSTR pszName, LPCTSTR pszValue, BOOL fImportant )
{
    IGNORE_HR(_pFontFace->SetProperty( pszName, pszValue ));

    // S_FALSE means no further processing is needed

    return S_FALSE;
}

HRESULT CAtFontFace::EndStyleRule( CStyleRule *pRule)
{
    HRESULT hr = _pFontFace->StartDownload();

    return hr;
}


//+----------------------------------------------------------------------------
//
//  Class:  CAtUnknown
//
//-----------------------------------------------------------------------------

CAtUnknown::CAtUnknown(CCSSParser *pParser, LPCTSTR pszName, 
                            LPCTSTR pszSelector, CStyleSheet *pStyleSheet) 
                : CAtBlockHandler( pParser )
{
    Assert(pParser);
 
    _pStyleSheet = pStyleSheet;

    _dwFlags = ATBLOCKFLAGS_MULTIPLERULES;

    _pBlockInfo = new CAtUnknownInfo;

    if(_pBlockInfo)
    {
        _pBlockInfo->_cstrUnknownBlockName.Set(pszName);
        _pBlockInfo->_cstrUnknownBlockSelector.Set(pszSelector);
    }
}

CAtUnknown::~CAtUnknown()
{    
    delete _pBlockInfo;
}

HRESULT CAtUnknown::SetProperty( LPCTSTR pszName, LPCTSTR pszValue, BOOL fImportant )
{
#if DBG==1
    if (IsTagEnabled(tagCSSAtBlocks))
    {
        CStr cstr;
        cstr.Set( _T("AtUnknown::SetProperty( \"") );
        cstr.Append( pszName );
        cstr.Append( _T("\", \"") );
        cstr.Append( pszValue );
        cstr.Append( fImportant ? _T("\" ) (important)\r\n") : _T("\" )\r\n") );
        OutputDebugString( cstr );
    }
#endif
    return S_OK;
}

HRESULT CAtUnknown::EndStyleRule(CStyleRule *pRule)
{
#if DBG==1
    if (IsTagEnabled(tagCSSAtBlocks))
    {
        OutputDebugStringA("CAtUnknown::EndStyleRule()\n");
    }
#endif

    
    return S_FALSE;
}
