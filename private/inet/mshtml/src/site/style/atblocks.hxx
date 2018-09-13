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

#ifndef I_ATBLOCKS_HXX_
#define I_ATBLOCKS_HXX_
#pragma INCMSG("--- Beg 'atblocks.hxx'")

MtExtern(CAtPage)
MtExtern(CAtMedia)
MtExtern(CAtFontFace)
MtExtern(CAtNamespace)
MtExtern(CAtUnknown)
MtExtern(CAtUnknownInfo)

class CCSSParser;
class CStyleRule;
class CFontFace;

EMediaType CSSMediaTypeFromName(LPCTSTR szMediaName);
LPCTSTR CSSNameFromMediaType(EMediaType eMediaType);


// This bit shows that the block can have more then 1 rules
// like @media { foo {rule1}; bar {rule2} }
#define ATBLOCKFLAGS_MULTIPLERULES      1   


class CAtBlockHandler {
private:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(Mem))
public:
    CAtBlockHandler( CCSSParser *pParser ) { _pParser = pParser; _dwFlags = 0;};
    virtual ~CAtBlockHandler() {};
    virtual HRESULT SetProperty( LPCTSTR pcszName, LPCTSTR pcszValue, BOOL fImportant ) = 0;
    virtual HRESULT EndStyleRule( CStyleRule *pRule) = 0;
    virtual CCSSParser::EStyleParserState GetNewParserState() { return CCSSParser::stPreProperty;};
    BOOL    IsFlagSet(DWORD dwVal)  { return !!(dwVal & _dwFlags); }

protected:
    DWORD _dwFlags;
    CCSSParser *_pParser;
};

class CAtPage : public CAtBlockHandler {
public:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CAtPage))
    CAtPage( CCSSParser *pParser );
    virtual ~CAtPage();

    virtual HRESULT SetProperty( LPCTSTR pcszName, LPCTSTR pcszValue, BOOL fImportant );
    virtual HRESULT EndStyleRule( CStyleRule *pRule);
};

class CAtMedia : public CAtBlockHandler {
public:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CAtMedia))
    CAtMedia( CCSSParser *pParser, LPCTSTR pcszName, CStyleSheet *pStyleSheet);
    virtual ~CAtMedia();

    virtual HRESULT SetProperty( LPCTSTR pcszName, LPCTSTR pcszValue, BOOL fImportant );
    virtual HRESULT EndStyleRule( CStyleRule *pRule);
    virtual CCSSParser::EStyleParserState GetNewParserState() { return CCSSParser::stPreElemName;};

protected:
    void SetMediaType(LPCTSTR pchName);

    CStyleSheet * _pStyleSheet;         // Save the style sheet
    
    EMediaType  _ePrevMediaType;        // Media type from the style sheet before current @media block
    EMediaType  _ePrevAtMediaType;      // Media type from the upper level nesting media block
};

class CAtFontFace : public CAtBlockHandler {
public:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CAtFontFace))
    CAtFontFace( CCSSParser *pParser, LPCTSTR pcszName );
    virtual ~CAtFontFace();

    virtual HRESULT SetProperty( LPCTSTR pcszName, LPCTSTR pcszValue, BOOL fImportant );
    virtual HRESULT EndStyleRule( CStyleRule *pRule);

protected:
    CFontFace *_pFontFace;
};


class CAtUnknownInfo
{
public:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CAtUnknownInfo));
    CStr _cstrUnknownBlockName;
    CStr _cstrUnknownBlockSelector;
    CStr _cstrUnknownBlockBody;
};


class CAtUnknown : public CAtBlockHandler {
public:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CAtUnknown))
    CAtUnknown(CCSSParser *pParser, LPCTSTR pszName, LPCTSTR pszSelector, CStyleSheet *pStyleSheet);
    virtual ~CAtUnknown();

    virtual HRESULT SetProperty( LPCTSTR pcszName, LPCTSTR pcszValue, BOOL fImportant );
    virtual HRESULT EndStyleRule( CStyleRule *pRule);
    virtual CCSSParser::EStyleParserState GetNewParserState() { return CCSSParser::stAtUnknownContents;};

protected:

    CStyleSheet     * _pStyleSheet;         // Save the style sheet
    CAtUnknownInfo  * _pBlockInfo; 
};





typedef struct
{
    EMediaType _mediaType;
    LPCTSTR    _szName;
} CSSMediaTypeTableTYPE;

static CSSMediaTypeTableTYPE cssMediaTypeTable[] =
{
    // Make sure to put most frequently used ones near the top
    {MEDIA_All, _T("All")},
    {MEDIA_Print, _T("Print")},
    {MEDIA_Screen, _T("Screen")},
    {MEDIA_Tv, _T("Tv")},
    {MEDIA_Unknown, _T("Unknown")},
    {MEDIA_Aural, _T("Aural")},
    {MEDIA_Braille, _T("Braille")},
    {MEDIA_Embossed, _T("Embossed")},
    {MEDIA_Handheld, _T("Handheld")},
    {MEDIA_Projection, _T("Projection")},
    {MEDIA_Tty, _T("Tty")}
};

#pragma INCMSG("--- End 'atblocks.hxx'")
#else
#pragma INCMSG("*** Dup 'atblocks.hxx'")
#endif
