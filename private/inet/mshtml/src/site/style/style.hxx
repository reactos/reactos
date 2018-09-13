//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       css.cxx
//
//  Contents:   Support for Cascading Style Sheets.. including:
//
//              CStyleTag
//              CStyle
//
//----------------------------------------------------------------------------

#ifndef I_STYLE_HXX_
#define I_STYLE_HXX_
#pragma INCMSG("--- Beg 'style.hxx'")

#ifndef X_BUFFER_HXX_
#define X_BUFFER_HXX_
#include "buffer.hxx"
#endif

#ifndef X_CODEPAGE_H_
#define X_CODEPAGE_H_
#include <codepage.h>
#endif

#define _hxx_
#include "style.hdl"

class CFilterArray;
class CStyleSheet;
class CAtBlockHandler;
class CStyleRule;
class CStyleSelector;
class CNamespace;

//+---------------------------------------------------------------------------
//
//  Class:      CStyle (Style Sheets)
//
//   Note:      Even though methods exist for this structure to be cached, we
//              are currently not doing so.
//
//----------------------------------------------------------------------------
class CElement;

MtExtern(CStyle)

class CStyle : public CBase
{
    DECLARE_CLASS_TYPES(CStyle, CBase)
private:
    WHEN_DBG(enum {eCookie=0x13000031};)
    WHEN_DBG(DWORD _dwCookie;)

    CElement         *_pElem;
    DISPID          _dispIDAA; // DISPID of Attr Array on _pElem

protected:
    DWORD           _dwFlags;
    
    union
    {
        WORD _wflags;
        struct
        {
            BOOL        _fSeparateFromElem: 1;
        };
    };

public:

    IDispatch *    _pStyleSource;

    enum STYLEFLAGS
    {
        STYLE_MASKPROPERTYCHANGES   =   0x1,
        STYLE_SEPARATEFROMELEM      =   0x2,
        STYLE_NOCLEARCACHES         =   0x4,    // BUGBUG: (anandra) HACK ALERT
    };
    
    CStyle(CElement *pElem, DISPID dispID, DWORD dwFlags);
    ~CStyle();

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CStyle))

    // Support for sub-objects created through pdl's
    // CStyle & Celement implement this differently
    // In the future this may return a NULL ptr!!
    CElement *GetElementPtr ( void ) { return _pElem; }
    void ClearElementPtr (void) { _pElem = NULL; }

    //Data access
    HRESULT PrivatizeProperties(CVoid **ppv) {return S_OK;}
    void    MaskPropertyChanges(BOOL fMask);

    //Parsing methods
    HRESULT BackgroundToStream(IStream * pIStream) const;
    HRESULT OnPropertyChange(DISPID dispid, DWORD dwFlags);

    DECLARE_TEAROFF_TABLE(IServiceProvider)
    // IServiceProvider methods
    NV_DECLARE_TEAROFF_METHOD(QueryService, queryservice, (REFGUID guidService, REFIID iid, LPVOID * ppv));

    DECLARE_TEAROFF_TABLE(IRecalcProperty)
    // IRecalcProperty methods
    NV_DECLARE_TEAROFF_METHOD(GetCanonicalProperty, getcanonicalproperty, (DISPID dispid, IUnknown **ppUnk, DISPID *pdispid));

    //CBase methods
    DECLARE_TEAROFF_METHOD(PrivateQueryInterface, privatequeryinterface, (REFIID iid, void ** ppv));
    HRESULT QueryInterface(REFIID iid, void **ppv){return PrivateQueryInterface(iid, ppv);}

    NV_DECLARE_TEAROFF_METHOD_(ULONG, PrivateAddRef, privateaddref, ());
    NV_DECLARE_TEAROFF_METHOD_(ULONG, PrivateRelease, privaterelease, ());

    NV_DECLARE_TEAROFF_METHOD(InvokeEx, invokeex, (
        DISPID              id,
        LCID                lcid,
        WORD                wFlags,
        DISPPARAMS *        pdp,
        VARIANT *           pvarRes,
        EXCEPINFO *         pei,
        IServiceProvider *  pSrvProvider));

    virtual CAtomTable * GetAtomTable ( BOOL *pfExpando = NULL )
        { return _pElem->GetAtomTable(pfExpando); }        

    BOOL    TestFlag  (DWORD dwFlags) const { return 0 != (_dwFlags & dwFlags); };
    void    SetFlag   (DWORD dwFlags) { _dwFlags |= dwFlags; };
    void    ClearFlag (DWORD dwFlags) { _dwFlags &= ~dwFlags; };
    
    HRESULT getValueHelper(long *plValue, DWORD dwFlags, const PROPERTYDESC *pPropertyDesc);
    HRESULT putValueHelper(long lValue, DWORD dwFlags, DISPID dispid);
    HRESULT getfloatHelper(float *pfValue, DWORD dwFlags, const PROPERTYDESC *pPropertyDesc);
    HRESULT putfloatHelper(float fValue, DWORD dwFlags, DISPID dispid);

    // recalc expression support
    BOOL DeleteCSSExpression(DISPID dispid);
    STDMETHOD(removeExpression)(BSTR bstrPropertyName, VARIANT_BOOL *pfSuccess);
    STDMETHOD(setExpression)(BSTR strPropertyName, BSTR strExpression, BSTR strLanguage);
    STDMETHOD(getExpression)(BSTR strPropertyName, VARIANT *pvExpression);

    // misc

    BOOL    NeedToDelegateGet(DISPID dispid);
    HRESULT       DelegateGet(DISPID dispid, VARTYPE varType, void * pv);

    void Passivate();
    struct CLASSDESC
    {
        CBase::CLASSDESC _classdescBase;
        void *_apfnTearOff;
    };

    // Helper for HDL file
    virtual CAttrArray **GetAttrArray ( void ) const;

    NV_DECLARE_TEAROFF_METHOD(put_textDecorationNone, PUT_textDecorationNone, (VARIANT_BOOL v));
    NV_DECLARE_TEAROFF_METHOD(get_textDecorationNone, GET_textDecorationNone, (VARIANT_BOOL * p));
    NV_DECLARE_TEAROFF_METHOD(put_textDecorationUnderline, PUT_textDecorationUnderline, (VARIANT_BOOL v));
    NV_DECLARE_TEAROFF_METHOD(get_textDecorationUnderline, GET_textDecorationUnderline, (VARIANT_BOOL * p));
    NV_DECLARE_TEAROFF_METHOD(put_textDecorationOverline, PUT_textDecorationOverline, (VARIANT_BOOL v));
    NV_DECLARE_TEAROFF_METHOD(get_textDecorationOverline, GET_textDecorationOverline, (VARIANT_BOOL * p));
    NV_DECLARE_TEAROFF_METHOD(put_textDecorationLineThrough, PUT_textDecorationLineThrough, (VARIANT_BOOL v));
    NV_DECLARE_TEAROFF_METHOD(get_textDecorationLineThrough, GET_textDecorationLineThrough, (VARIANT_BOOL * p));
    NV_DECLARE_TEAROFF_METHOD(put_textDecorationBlink, PUT_textDecorationBlink, (VARIANT_BOOL v));
    NV_DECLARE_TEAROFF_METHOD(get_textDecorationBlink, GET_textDecorationBlink, (VARIANT_BOOL * p));

    // Override all property gets/puts so that the invtablepropdesc points to
    // our local functions to give us a chance to pass the pAA of CStyleRule and
    // not CRuleStyle.
    NV_DECLARE_TEAROFF_METHOD(put_StyleComponent, PUT_StyleComponent, (BSTR v));
    NV_DECLARE_TEAROFF_METHOD(put_Url, PUT_Url, (BSTR v));
    NV_DECLARE_TEAROFF_METHOD(put_String, PUT_String, (BSTR v));
    NV_DECLARE_TEAROFF_METHOD(put_Short, PUT_Short, (short v));
    NV_DECLARE_TEAROFF_METHOD(put_Long, PUT_Long, (long v));
    NV_DECLARE_TEAROFF_METHOD(put_Bool, PUT_Bool, (VARIANT_BOOL v));
    NV_DECLARE_TEAROFF_METHOD(put_Variant, PUT_Variant, (VARIANT v));
    NV_DECLARE_TEAROFF_METHOD(put_DataEvent, PUT_DataEvent, (VARIANT v));
    NV_DECLARE_TEAROFF_METHOD(get_Url, GET_Url, (BSTR *p));
    NV_DECLARE_TEAROFF_METHOD(get_StyleComponent, GET_StyleComponent, (BSTR *p));
    NV_DECLARE_TEAROFF_METHOD(get_Property, GET_Property, (void *p));

#ifndef NO_EDIT
    virtual IOleUndoManager * UndoManager(void) { return _pElem->UndoManager(); }
    virtual BOOL QueryCreateUndo(BOOL fRequiresParent, BOOL fDirtyChange = TRUE) 
        { return _pElem->QueryCreateUndo( fRequiresParent, fDirtyChange ); }
#endif // NO_EDIT

    #define _CStyle_
    #include "style.hdl"

protected:
    DECLARE_CLASSDESC_MEMBERS;
};

//---------------------------------------------------------------------
//  Class Declaration:  CCSSParser
//      The CCSSParser class implements a parser for data in the
//  Cascading Style Sheets format.
//---------------------------------------------------------------------

MtExtern(CCSSParser)

typedef enum ePARSERTYPE { eStylesheetDefinition, eSingleStyle }; // Is this a stylesheet, or just

class CCSSParser {
public:

    DECLARE_MEMALLOC_NEW_DELETE(Mt(CCSSParser))

    // Standard constructor and destructor
#ifdef XMV_PARSE
    // Standard constructor and destructor
    CCSSParser( CStyleSheet *pStyleSheet,
                CAttrArray **ppPropertyArray = NULL,
                BOOL fXMLGeneric = FALSE,
                ePARSERTYPE eType = eStylesheetDefinition,
                const HDLDESC *pHDLDesc = &CStyle::s_apHdlDescs,
                CBase * pBaseObj = NULL,
                DWORD dwOpcode = HANDLEPROP_SETHTML );
#else
    // Standard constructor and destructor
    CCSSParser( CStyleSheet *pStyleSheet,
                CAttrArray **ppPropertyArray = NULL,
                ePARSERTYPE eType = eStylesheetDefinition,
                const HDLDESC *pHDLDesc = &CStyle::s_apHdlDescs,
                CBase * pBaseObj = NULL,
                DWORD dwOpcode = HANDLEPROP_SETHTML );
#endif
    ~CCSSParser();

    CStr _cstrLookForName;              // Name to look for if we're sub-parsing
    CStr _cstrLookForValue;             // If we match - stored here

    ePARSERTYPE _eDefType;

    // Shortcut for loading from files.
    HRESULT LoadFromFile( LPCTSTR szFilename, CODEPAGE codepage );
    HRESULT LoadFromStream( IStream * pStream, CODEPAGE codepage );
    CODEPAGE CheckForCharset( TCHAR * pch, ULONG cch );

    // Prepare to accept data.
    void Open();

    // Write a block of data to the parser.
    ULONG Write( const TCHAR *pData, ULONG ulLen );

    // Signal the end of data.
    void Close();

    void GetParentElement(CElement ** ppElement);

    HRESULT SetStyleProperty( BOOL fImportant = FALSE );

    HRESULT SetExpression(DISPID dispid, WORD wMaxstrlen = 0, TCHAR chOld = 0);    // Called by the CSS Parser

    inline CStyleSheet *GetStyleSheet( void ) { return _pStyleSheet; };

    void SetDefaultNamespace(LPCTSTR pchNamespace);

protected:
    // Used internally to finish off a single style declaration - when a new
    // declaration begins, or at EOD.
    HRESULT EndStyleDefinition( void );

    // At block handling support
    HRESULT BeginAtBlock( void );
    HRESULT EndAtBlock( void );

    typedef enum {
        AT_DEFAULT,
        AT_PAGE,
        AT_FONTFACE,
        AT_MEDIA,
        AT_CHARSET,
        AT_UNKNOWN
    } EAtBlockType;

    enum {
        ATNESTDEPTH = 8
    };

    CAtBlockHandler *_sAtBlock[ ATNESTDEPTH ];  // blocktype-specific handler.

public:
    int _iAtBlockNestLevel;			// The current at-block nesting.  Inits to 0.
    // The states the parser can be in.
    typedef enum {
        stPreElemName,      // Before any element name is read
        stAtToken,          // While parsing an "@token", e.g. "@import"
        stAtPreContents,    // After the @token, while in whitespace
        stAtContents,       // While reading the contents after the @token
        stAtUnknownContents,// While reading the contents after the @unknown
        stInElemName,       // While reading element name
        stPreProperty,      // After reading selector, before property name
        stInProperty,       // While reading property name
        stPreEquals,        // After reading property name, before reading ':'
        stPostEquals,       // After reading ':', before reading value
        stInValue,          // While reading value
        stPostValue,        // After reading value (only used in conjunction with "!important")
        stInDblQuote,       // While reading double-quoted value
        stInSnglQuote,      // While reading single-quoted value
        stBeginComment,     // After '/', could be a comment
        stInComment,        // After "/*", we're in a comment
        stEndComment,       // After "/*...*", could be the "*/" that ends the comment
        stInImportant,      // While we're parsing the string "! important"
        stIgnore            // We're in a "junk" state, e.g. a STYLE attribute with '}'
    } EStyleParserState;


protected:
    EStyleParserState _eState;      // The current state
    EStyleParserState _eLastState;  // The last state before we went into a comment, etc.

                                                            // a single style?
    CStyleRule          *_pCurrRule;            // The rule we're currently parsing
    CAttrArray         **_ppCurrProperties;    // The properties in the style group we're parsing
    CStyleSelector      *_pCurrSelector;        // The selector for the style we're parsing
    CStyleSelector      *_pSiblings;            // Siblings for this selector (other selectors to which
                                                // this style applies)

    CBuffer              _cbufPropertyName;     // The name of the current property
    CBuffer              _cbufBuffer;           // A temporary buffer to store data while parsing
    CBuffer              _cbufImportant;        // A buffer to store the string "!important" while we're parsing,
                                                // so we can recover if it's "!impertinent" or something.
    int                  _iImportantText;       // The offset of the beginning of the actual text in _cbufImportant.

    CStyleSheet         *_pStyleSheet;  // The stylesheet we're building from parsing the data.
    const HDLDESC       *_pHDLDesc;
    CBase               *_pBaseObj;
    DWORD                _dwOpcode;
    CNamespace          *_pDefaultNamespace;

    unsigned            _cExpressionsRule;      // The number of expressions in the current rule

    CODEPAGE            _codepage;
#ifdef XMV_PARSE
    BOOL                _fXMLGeneric;           // non-qualified selectors turn into generic tags
#endif
};

size_t ValidStyleUrl(TCHAR *pch);
HRESULT RemoveStyleUrlFromStr(TCHAR **pch);  // Removes "url(" if present from the string


// Style Property Helpers
HRESULT WriteStyleToBSTR( CBase *pObject, CAttrArray *pAA, BSTR *pbstr, BOOL fQuote );

HRESULT WriteBackgroundStyleToBSTR( CAttrArray *pAA, BSTR *pbstr );
HRESULT WriteFontToBSTR( CAttrArray *pAA, BSTR *pbstr );
HRESULT WriteLayoutGridToBSTR( CAttrArray *pAA, BSTR *pbstr );
HRESULT WriteTextDecorationToBSTR( CAttrArray *pAA, BSTR *pbstr );
HRESULT WriteTextAutospaceToBSTR( CAttrArray *pAA, BSTR *pbstr );
void WriteTextAutospaceFromLongToBSTR( LONG lTextAutospace, BSTR *pbstr, BOOL fWriteNone );
HRESULT WriteBorderToBSTR( CAttrArray *pAA, BSTR *pbstr );
HRESULT WriteExpandedPropertyToBSTR( DWORD dispid, CAttrArray *pAA, BSTR *pbstr );
HRESULT WriteListStyleToBSTR( CAttrArray *pAA, BSTR *pbstr );
HRESULT WriteBorderSidePropertyToBSTR( DWORD dispid, CAttrArray *pAA, BSTR *pbstr );
HRESULT WriteBackgroundPositionToBSTR( CAttrArray *pAA, BSTR *pbstr );
HRESULT WriteClipToBSTR( CAttrArray *pAA, BSTR *pbstr );

// Style Property Sub-parsers
HRESULT ParseBackgroundProperty( CAttrArray **ppAA, CBase *pObject, DWORD dwOpCode, LPCTSTR pcszBGString, BOOL bValidate );
HRESULT ParseFontProperty( CAttrArray **ppAA, CBase *pObject, DWORD dwOpCode, LPCTSTR pcszFontString );
HRESULT ParseLayoutGridProperty( CAttrArray **ppAA, CBase *pObject, DWORD dwOpCode, LPCTSTR pcszGridString );
HRESULT ParseExpandProperty( CAttrArray **ppAA, CBase *pObject, DWORD dwOpCode, LPCTSTR pcszBGString, DWORD dwDispId, BOOL fIsMeasurements );
HRESULT ParseBorderProperty( CAttrArray **ppAA, CBase *pObject, DWORD dwOpCode, LPCTSTR pcszBorderString );
HRESULT ParseAndExpandBorderSideProperty( CAttrArray **ppAA, CBase *pObject, DWORD dwOpCode, LPCTSTR pcszBorderString, DWORD dwDispId );
HRESULT ParseTextDecorationProperty( CAttrArray **ppAA, LPCTSTR pcszTextDecoration, WORD wFlags );
HRESULT ParseTextAutospaceProperty( CAttrArray **ppAA, LPCTSTR pcszTextAutospace, WORD wFlags );
HRESULT ParseListStyleProperty( CAttrArray **ppAA, CBase *pObject, DWORD dwOpCode, LPCTSTR pcszListStyle );
HRESULT ParseBackgroundPositionProperty( CAttrArray **ppAA, CBase *pObject, DWORD dwOpCode, LPCTSTR pcszBackgroundPosition );
HRESULT ParseClipProperty( CAttrArray **ppAA, CBase *pObject, DWORD dwOpCode, LPCTSTR pcszClip );
HRESULT ParseFilterProperty( LPCTSTR pcszFilter, CFilterArray * pFA, CElement * pElem );

typedef enum {
    sysfont_caption = 0,
    sysfont_icon = 1,
    sysfont_menu = 2,
    sysfont_messagebox = 3,
    sysfont_smallcaption = 4,
    sysfont_statusbar = 5,
    sysfont_non_system = -1
} Esysfont;

Esysfont FindSystemFontByName( const TCHAR * szString );

#define TD_NONE         0x01
#define TD_UNDERLINE    0x02
#define TD_OVERLINE     0x04
#define TD_LINETHROUGH  0x08
#define TD_BLINK        0x10

enum TEXTAUTOSPACETYPE
{
    TEXTAUTOSPACE_NONE         = 0,
    TEXTAUTOSPACE_NUMERIC      = 1,
    TEXTAUTOSPACE_ALPHA        = 2,
    TEXTAUTOSPACE_SPACE        = 4,
    TEXTAUTOSPACE_PARENTHESIS  = 8
};

#pragma INCMSG("--- End 'style.hxx'")
#else
#pragma INCMSG("*** Dup 'style.hxx'")
#endif
