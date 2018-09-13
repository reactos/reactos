#ifndef I_SHEETS_HXX_
#define I_SHEETS_HXX_
#pragma INCMSG("--- Beg 'sheets.hxx'")

#define _hxx_
#include "sheet.hdl"

#define _hxx_
#include "sheetcol.hdl"

class CStyleSheetRuleArray;
class CFontFace;

MtExtern(CStyleSheet)
MtExtern(CStyleSheet_apRulesList_pv)
MtExtern(CStyleSheetArray)
MtExtern(CStyleSheetArray_aStyleSheets_pv)
MtExtern(CStyleSheetArray_apFontFaces_pv)
MtExtern(CStyleRule)
MtExtern(CStyleRuleArray)
MtExtern(CStyleRuleArray_pv)
MtExtern(CNamespace)


// Use w/ CStyleRule::_dwSpecificity
#define SPECIFICITY_ID               0x00010000
#define SPECIFICITY_CLASS            0x00000100
#define SPECIFICITY_PSEUDOCLASS      0x00000100
#define SPECIFICITY_TAG              0x00000001
#define SPECIFICITY_PSEUDOELEMENT    0x00000001

// Use w/ CStyleRule::_dwFlags
#define STYLERULEFLAG_DISABLED       0x00000001     // SS has just been turned off by user
#define STYLERULEFLAG_OUTOFTREE      0x00000002     // SS is no longer attached to document
#define STYLERULEFLAG_NORENDER       (STYLERULEFLAG_DISABLED|STYLERULEFLAG_OUTOFTREE)

// NOTE:  The STYLERULEFLAG_* flags are OR'ed together with the MEDIA_* flags 
// declared below, be careful not to cause conflicts.

enum EMediaType {
    MEDIA_NotSet        = 0x0000,
    MEDIA_Aural         = 0x0010,
    MEDIA_Braille       = 0x0020,
    MEDIA_Embossed      = 0x0040,
    MEDIA_Handheld      = 0x0080,
    MEDIA_Print         = 0x0100,
    MEDIA_Projection    = 0x0200,
    MEDIA_Screen        = 0x0400,
    MEDIA_Tty           = 0x0800,
    MEDIA_Tv            = 0x1000,
    MEDIA_Unknown       = 0x2000,
    MEDIA_All           = 0x1FF0,   // Does not include unknown
    MEDIA_Bits          = 0x3FF0,
};



#define MEDIATYPE(dw)   (dw&MEDIA_Bits)


// Use w/ CStyleSheet::ChangeStatus()
// If you find it necessary to OR several of these flags together, create a
// descriptive #define for the combination for readability.
#define CS_ENABLERULES               0x00000001     // enable vs. disable
#define CS_CLEARRULES                0x00000002     // zero ids?
#define CS_PATCHRULES                0x00000004     // patch ids of succeeding rules?
#define CS_DETACHRULES               (CS_CLEARRULES|CS_PATCHRULES)  // use when a SS is leaving the document


class CStyle;    // defined in style.hxx
class CBitsCtx;
class CDwnChan;

//defined in rulescol.hxx
class CStyleSheetRule;

// Classes defined in this file:
class CStyleSheet;
class CStyleSelector;
class CStyleRule;
class CStyleSheetArray;
class CStyleRuleArray;
class CStyleClassIDCache;
class CNamespace;

typedef enum {
    pclassNone,
    pclassActive,
    pclassVisited,
    pclassHover,
    pclassLink
} EPseudoclass;

// If CStyleID changes to support deeper nesting, make sure this changes!
#define MAX_IMPORT_NESTING 4
#define MAX_SHEETS_PER_LEVEL 31
#define MAX_RULES_PER_SHEET 4095

#define LEVEL1_MASK 0xF8000000
#define LEVEL2_MASK 0x07C00000
#define LEVEL3_MASK 0x003E0000
#define LEVEL4_MASK 0x0001F000
#define RULE_MASK   0x00000FFF

class CStyleID
{
public:
    CStyleID(const DWORD id = 0) { _dwID = id; }
    CStyleID(const unsigned long l1, const unsigned long l2,
                        const unsigned long l3, const unsigned long l4, const unsigned long r);
    CStyleID(const CStyleID & id) { _dwID = id._dwID; }
    
    operator DWORD() const { return _dwID; }
    CStyleID & operator =(const CStyleID & src) { _dwID = src._dwID; return *this; }
    CStyleID & operator =(const DWORD src) { _dwID = src; return *this; }

    void SetLevel(const unsigned long level, const unsigned long value);
    unsigned long GetLevel(const unsigned long level) const;
    unsigned long FindNestingLevel() const { unsigned long l = MAX_IMPORT_NESTING;
                                      while ( l && !GetLevel(l) ) --l;
                                      return l; }

    void SetRule(const unsigned long rule)
        { Assert( "Maximum of 4095 rules per stylesheet!" && rule <= MAX_RULES_PER_SHEET );
          _dwID &= ~RULE_MASK; _dwID |= (rule & RULE_MASK); }
    unsigned int GetRule() const { return (_dwID & RULE_MASK); }

    unsigned int GetSheet() const { return (_dwID & ~RULE_MASK); }

private:
    DWORD _dwID;
};

//+---------------------------------------------------------------------------
//
// CStyleRule
//
//----------------------------------------------------------------------------
class CStyleRule {
    friend class CStyleSheet;
    friend class CStyleSheetArray;
    friend class CStyleRuleArray;

public:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CStyleRule))
    CStyleRule( CStyleSelector *pSelector );
    ~CStyleRule();

    void Free( void );
    inline void Detach( void ) { _pSelector = NULL; _dwSpecificity = 0;
                                 _paaStyleProperties = NULL;  };
    void SetSelector( CStyleSelector *pSelector );
    inline CStyleSelector * const GetSelector() { return _pSelector; };
    inline BOOL MediaTypeMatches( EMediaType emedia ) { return !!( emedia & MEDIATYPE(_dwFlags) ); };
    CAttrArray *_paaStyleProperties;

    HRESULT GetString( CBase *pBase, CStr *pResult );
    inline CStyleID GetID() { return _sidRule; };
    const CNamespace * GetNamespace() const;

    DWORD GetLastAtMediaTypeBits() { return _dwAtMediaTypeFlags; }

    HRESULT GetMediaString(DWORD dwCurMedia, CBufferedStr *pstrMediaString);

protected:
    CStyleSelector *_pSelector;
    DWORD _dwSpecificity;
    CStyleID _sidRule;
    DWORD _dwFlags;                 // media types and other settings
    DWORD _dwAtMediaTypeFlags;      // Media types that came from the @media blocks
};

//+---------------------------------------------------------------------------
//
// CStyleRuleArray.
//
//----------------------------------------------------------------------------
class CStyleRuleArray : public CPtrAry<CStyleRule *>
{
public:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CStyleRuleArray))
     CStyleRuleArray() : CPtrAry<CStyleRule *>(Mt(CStyleRuleArray_pv)) {};
    ~CStyleRuleArray() { Assert( "Must call Free() before destructor!" && (Size() == 0));};

    void Free( void );

    HRESULT InsertStyleRule( CStyleRule *pRule, BOOL fDefeatPrevious );
};


typedef enum 
{
    CSSPARSESTATUS_NOTSTARTED,
    CSSPARSESTATUS_PARSING,
    CSSPARSESTATUS_DONE
} CSSPARSESTATUS;


//+---------------------------------------------------------------------------
//
// CStyleSheet
//
//----------------------------------------------------------------------------

class CStyleSheet : public CBase
{
    DECLARE_CLASS_TYPES(CStyleSheet, CBase)
    
    friend class CStyleSheetArray;
    
public:     // functions

    DECLARE_MEMALLOC_NEW_DELETE(Mt(CStyleSheet))

    CStyleSheet( CElement *pParentElem, CStyleSheetArray * const pSSAContainer);
    virtual ~CStyleSheet();

    // IUnknown
    // We support our own interfaces, and maintain our own ref-count.  Further, any
    // ref on the stylesheet automatically adds a subref to the stylesheet's HTML element.
    STDMETHOD(QueryInterface) (REFIID iid, LPVOID * ppv)
        { return PrivateQueryInterface(iid, ppv); };
    STDMETHOD_(ULONG, AddRef) (void) { return PrivateAddRef(); };
    STDMETHOD_(ULONG, Release) (void) { return PrivateRelease(); };

    STDMETHOD(PrivateQueryInterface)(REFIID, void **);
    STDMETHOD_(ULONG, PrivateAddRef) (void);
    STDMETHOD_(ULONG, PrivateRelease) (void);

    // Need our own classdesc to support tearoffs (IHTMLStyleSheet)
    struct CLASSDESC
    {
        CBase::CLASSDESC _classdescBase;
        void *_apfnTearOff;
    };
    const CLASSDESC * ElementDesc() const { return (const CLASSDESC *) BaseDesc(); }

    // CBase overrides
    void Passivate();
    virtual CAtomTable * GetAtomTable ( BOOL *pfExpando = NULL )
        { Assert(_pParentElement); return _pParentElement->GetAtomTable(pfExpando); }        

    // CStyleSheet-specific functions
    HRESULT AddStyleRule( CStyleRule *pRule, BOOL fDefeatPrevious = TRUE, long lIdx = -1 );
    HRESULT AddImportedStyleSheet( TCHAR *pszURL, long lPos = -1, long *plNewPos = NULL );
    HRESULT ChangeStatus( DWORD dwAction, BOOL fForceRender, BOOL *pFound );
    inline HRESULT SetMediaType( DWORD emedia, BOOL fForceRender ) {
        return CStyleSheet::ChangeStatus( _fDisabled ? emedia : (CS_ENABLERULES|emedia), fForceRender, NULL ); };
    HRESULT LoadFromURL( const TCHAR *pszURL, BOOL fAutoEnable = FALSE );
    void PatchID( unsigned long ulLevel, unsigned long ulValue, BOOL fRecursiveCall );
    void ChangeID(CStyleID const idNew);
    void CheckImportStatus( void );
    void StopDownloads( BOOL fReleaseBitsCtx );
    HRESULT InsertExistingRules( void );
    void    ChangeContainer(CStyleSheetArray * _pSSANewContainer);

    inline CElement *GetParentElement( void ) { return _pParentElement; };
    inline void DisconnectFromParentSS( void ) { if ( _pParentStyleSheet ) _pParentStyleSheet = this; };
    inline BOOL IsDisconnectedFromParentSS( void ) { return ( _pParentStyleSheet == this ? TRUE : FALSE ); };
    inline BOOL IsAnImport( void ) { return (_pParentStyleSheet ? TRUE : FALSE); };

    void Fire_onerror();
    HRESULT SetReadyState( long readyState );

    HRESULT GetString( CStr *pResult );
    CStyleSheetArray *GetRootContainer( void );
    CStyleSheetArray *GetSSAContainer( void ) { return _pSSAContainer; }

    CDoc *GetDocument( void );
    CMarkup *GetMarkup( void );
    inline CElement *GetElement( void ) { return _pParentElement; };
    inline int GetNumRules( void ) { return _apRulesList.Size(); };
    HRESULT GetOMRule( long lIdx, IHTMLStyleSheetRule **ppSSRule );
    CStyleRule *GetRule( ELEMENT_TAG eTag, CStyleID ruleID );

    EMediaType GetMediaTypeValue(void) { return _eMediaType; }
    EMediaType  GetLastAtMediaTypeValue(void) { return _eLastAtMediaType; }
    void SetMediaTypeValue(EMediaType eMediaType) { _eMediaType = eMediaType; }
    void SetLastAtMediaTypeValue(EMediaType eMediaType)  { _eLastAtMediaType = eMediaType; }
 

#ifndef NO_EDIT
    virtual IOleUndoManager * UndoManager(void) { return _pParentElement->UndoManager(); }
    virtual BOOL QueryCreateUndo(BOOL fRequiresParent, BOOL fDirtyChange = TRUE) 
        { return _pParentElement->QueryCreateUndo( fRequiresParent, fDirtyChange ); }
#endif // NO_EDIT

    #define _CStyleSheet_
    #include "sheet.hdl"

public:     
    // data
    CStyleID         _sidSheet;                // "Rules" field will be empty (nesting depths only)
    CSSPARSESTATUS   _eParsingStatus;          // Have we finished parsing the sheet?
    TCHAR *          _achAbsoluteHref;         // absolute HREF

protected:
    DECLARE_CLASSDESC_MEMBERS; 

protected:  // functions
    void Free( void );
    void SetBitsCtx(CBitsCtx * pBitsCtx);
    void OnDwnChan(CDwnChan * pDwnChan);
    static void CALLBACK OnDwnChanCallback(void * pvObj, void * pvArg)
        { ((CStyleSheet *)pvArg)->OnDwnChan((CDwnChan *)pvObj); }

#ifdef XMV_PARSE
    BOOL IsXML(void);
#endif

protected:  // data
    CElement         *_pParentElement;          // ALWAYS points to a valid element, but may be stale if
                                                // the stylesheet has been orphaned.  E.g. if a script
                                                // var holds a linked SS, but the SS has been removed from
                                                // the document (the link href changed), this will still
                                                // point to the link elem.  This allows the omission of all
                                                // sorts of patchup code to deal with possible null parents.

    CStyleSheet      *_pParentStyleSheet;       // Non-NULL indicates we're an import.
                                                // If _pParentStyleSheet == this, it means we _were_ an
                                                // import and our parent has been removed from the OM
                                                // collection(s).
    CStyleSheetArray *_pImportedStyleSheets;    // Ptrs to SS that were imported in this SS.
    CStyleSheetArray * _pSSAContainer;          // The SSA that will contain this SS's rules.
    
    // Private data structure for our internal list of rules
    class CRuleEntry {
    public:
        CStyleID    GetStyleID()
            { return _pRule->GetID(); }
    public:
        ELEMENT_TAG _eTag;
        CStyleSheetRule *pAutomationRule;
        CStyleRule     *_pRule;                  // The Rule object that is also in the StyleSheetArray.  We
                                                // keep this pointers to allow the Style Sheet to be moved to 
                                                // another Markup without needing to re-parse the style sheet and
                                                // create new rules objects.


    };
    
    DECLARE_CDataAry(CAryRulesList, CRuleEntry, Mt(Mem), Mt(CStyleSheet_apRulesList_pv))
    CAryRulesList    _apRulesList;              // Source order list of rules (by selector & ID) that
                                                // this stylesheet has added.

    BOOL             _fDisabled : 1;            // Is this stylesheet disabled?
    BOOL             _fComplete : 1;            // Has OnDwnChan been called yet?
    EMediaType       _eMediaType;               // media type for the linked stylsheet &ed with effected @media media
    EMediaType       _eLastAtMediaType;         // Media type from the last @media block (used for serialization)
    
    CBitsCtx         *_pBitsCtx;                // The bitsctx used to download this SS if it was @imported
    DWORD            _dwStyleCookie;
    DWORD            _dwScriptCookie;
    CStr             _cstrImportHref;           // If we're an import, this is our HREF

    long             _nExpectedImports;         // After parsing this sheet, this tells us how many @imports we encountered  
    long             _nCompletedImports;        // Number of imported SS that have finished d/ling.
    CStyleSheetRuleArray   *_pOMRulesArray;     // The Automation array (if needed).
};


//+---------------------------------------------------------------------------
//
// CNamespace
//
// BUGBUG (alexz, artakka)  as we removed any support for urns and @namespace,
//                          this class is no longer necessary and CStr can be
//                          used instead
//
//----------------------------------------------------------------------------

MtExtern(CNamespace)

class CNamespace
{
public:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CNamespace))

    CNamespace() {}
    CNamespace(const CNamespace & nmsp) 
    {
        _strNamespace.Set(nmsp._strNamespace); 
    }
    
    const CNamespace& operator=(const CNamespace & nmsp);

    // Returns the name  of the namespace, or HTML if empty
    inline LPCTSTR GetNamespaceString() {if(IsEmpty()) return _T("HTML"); else return _strNamespace;}

    // Parses given string and stores into member variables depending on the type
    HRESULT SetNamespace(LPCTSTR pchStr);

    // Sets directly the short name 
    inline void SetShortName(LPCTSTR pchStr)
    {
        _strNamespace.Set(pchStr);
    }

    // Sets directly the short name 
    inline void SetShortName(LPCTSTR pchStr, UINT nNum)
    {
        _strNamespace.Set(pchStr, nNum);
    }

    // Returns TRUE if the namespace is empty
    inline BOOL IsEmpty()  const { return (_strNamespace.Length() == 0); }
    // Returns TRUE if the namspaces  are equal
    BOOL IsEqual(const CNamespace * pNmsp) const;


protected:
    CStr    _strNamespace;      // contains either unique name (e.g. URL) or short name
};


//+---------------------------------------------------------------------------
//
// CStyleSelector
//
//----------------------------------------------------------------------------

MtExtern(CStyleSelector)

class CStyleSelector {
    friend class CStyleSheet;
    friend class CStyleSheetArray;
    friend class CStyleRuleArray;

public:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CStyleSelector))
#ifdef XMV_PARSE
    CStyleSelector( const TCHAR *pcszSelectorString, CStyleSelector *pParent, 
                        BOOL fXMLGeneric = FALSE);
#else
    CStyleSelector( const TCHAR *pcszSelectorString, CStyleSelector *pParent);
#endif
    ~CStyleSelector();

#ifdef XMV_PARSE
    HRESULT Init( const TCHAR *pcszSelectorString, BOOL fXMLGeneric);
#else
    HRESULT Init( const TCHAR *pcszSelectorString);
#endif
    DWORD GetSpecificity( void );

    BOOL Match( CTreeNode * pNode, CStyleClassIDCache *pCIDCache, EPseudoclass *pePseudoclass = NULL );

    inline HRESULT SetSibling( CStyleSelector *pSibling )
        { if ( _pSibling ) THR(S_FALSE); _pSibling = pSibling; return S_OK; }
    inline CStyleSelector *GetSibling( void ) { return _pSibling; }

    inline EPseudoclass Pseudoclass( void ) { return _ePseudoclass; };

    HRESULT GetString( CStr *pResult );

    const CNamespace * GetNamespace() { return &_Nmsp; }

protected:
    BOOL MatchSimple( CElement *pElement, CStyleClassIDCache *pCIDCache, int iCacheSlot, EPseudoclass *pePseudoclass );
    
    ELEMENT_TAG _eElementType;
    CStr _cstrClass;
    CStr _cstrID;
    EPseudoclass _ePseudoclass;
    enum {
        pelemNone,
        pelemFirstLetter,
        pelemFirstLine,
        pelemUnknown
    } _ePseudoElement;
    CStyleSelector *_pParent;
    CStyleSelector *_pSibling;
    CStr            _cstrTagName;
    CNamespace      _Nmsp;
};

//+---------------------------------------------------------------------------
//
// CStyleSheetArray.
//
//----------------------------------------------------------------------------
class CStyleSheetArray : public CBase
{
    DECLARE_CLASS_TYPES(CStyleSheetArray, CBase)

    friend class CStyleSheet;
    
public:

    DECLARE_MEMALLOC_NEW_DELETE(Mt(CStyleSheetArray))

    CStyleSheetArray( CBase * const pOwner, CStyleSheetArray *pRuleManager, CStyleID const sidParent );
    virtual ~CStyleSheetArray();

    enum ERulesShiftAction {
        ruleRemove,
        ruleInsert
    };
    // IUnknown
    // We support our own interfaces, but all ref-counting is delegated via
    // sub- calls to our owner.  We maintain NO REF-COUNT information internally.
    // (the CBase impl. of ref-counting is never accessed except for destruction).
    STDMETHOD(QueryInterface) (REFIID iid, LPVOID * ppv)
        { return PrivateQueryInterface(iid, ppv); }
    STDMETHOD_(ULONG, AddRef) (void)
        { return PrivateAddRef(); }
    STDMETHOD_(ULONG, Release) (void)
        { return PrivateRelease(); }

    DECLARE_TEAROFF_METHOD(PrivateQueryInterface, privatequeryinterface, (REFIID, void **));
    DECLARE_TEAROFF_METHOD_(ULONG, PrivateAddRef, privateaddref, (void))
    { return (_pOwner ? _pOwner->SubAddRef() : S_OK); }
    DECLARE_TEAROFF_METHOD_(ULONG, PrivateRelease, privaterelease, (void))
    { return (_pOwner ? _pOwner->SubRelease() : S_OK); }

    // Need our own classdesc to support tearoffs (IHTMLStyleSheetsCollection)
    struct CLASSDESC
    {
        CBase::CLASSDESC _classdescBase;
        void *_apfnTearOff;
    };
    const CLASSDESC * ElementDesc() const { return (const CLASSDESC *) BaseDesc(); }

    // IDispatchEx overrides (overriding CBase impl)
    HRESULT STDMETHODCALLTYPE GetDispID(BSTR bstrName, DWORD grfdex, DISPID *pid);

    HRESULT STDMETHODCALLTYPE InvokeEx(
        DISPID dispidMember,
        LCID lcid,
        WORD wFlags,
        DISPPARAMS * pdispparams,
        VARIANT * pvarResult,
        EXCEPINFO * pexcepinfo,
        IServiceProvider *pSrvProvider);

    HRESULT STDMETHODCALLTYPE GetNextDispID(DWORD grfdex,
                                            DISPID id,
                                            DISPID *prgid);

    HRESULT STDMETHODCALLTYPE GetMemberName(DISPID id,
                                            BSTR *pbstrName);
    // CBase override
    virtual CAtomTable * GetAtomTable ( BOOL *pfExpando = NULL )
        { return _pSSARuleManager->_pOwner->GetAtomTable(pfExpando); }        

    // Cleanup helper
    void Free( void );

    // CStyleSheetArray methods
    HRESULT CreateNewStyleSheet( CElement *pParentElem, CStyleSheet **ppStyleSheet,
                                 long lPos = -1, long *plNewPos = NULL );
    HRESULT AddStyleSheet ( CStyleSheet *pStyleSheet, long lPos = -1, long *plNewPos = NULL );
    HRESULT ReleaseStyleSheet( CStyleSheet * pStyleSheet, BOOL fForceRender );
    HRESULT AddStyleRule( CStyleRule *pRule, BOOL fDefeatPrevious );
    HRESULT Apply( CStyleInfo *pStyleInfo, ApplyPassType passType = APPLY_All, EMediaType eMediaType = MEDIA_All, BOOL *pfContainsImportant = NULL );
    BOOL TestForPseudoclassEffect( CElement *pElem, BOOL fVisited, BOOL fActive,
                                   BOOL fOldVisited, BOOL fOldActive );
    void ChangeRulesStatus( DWORD dwAction, BOOL fForceRender, BOOL *pFound, CStyleID sidSheet);
    void ShiftRules( ERulesShiftAction eAction, BOOL fForceRender, BOOL *pContainsChangedRule, CStyleID sidRule );
    
    int Size( void ) const { return _aStyleSheets.Size(); };
    CStyleSheet * Get( long lIndex );
    CStyleRule * const GetRule( ELEMENT_TAG eTag, CStyleID ruleID );

    CStyleSheet * SheetInRule(CStyleRule *pRule)
    { Assert(pRule); return FindStyleSheetForSID(pRule->GetID()); }

    #define _CStyleSheetArray_
    #include "sheetcol.hdl"

public: // data
    CStyleID _sidForOurSheets;              // Base ID for stylesheets contained in this array
    BOOL _fInvalid;                         // Set to TRUE if constructor fails
    CStr _cstrUserStylesheet;               // File name of current user style sheet from option settings

protected:
    DECLARE_CLASSDESC_MEMBERS;

    CStyleSheet *FindStyleSheetForSID(CStyleID sidTarget);

private:
    CStyleSheetArray() : _pOwner(NULL) { Assert("Not implemented; must give params!" && FALSE ); };  // disallow use of default constructor
    BOOL IsOrdinalSSDispID( DISPID dispidMember );
    BOOL IsNamedSSDispID( DISPID dispidMember );
    long FindSSByHTMLID( LPCTSTR pszID, BOOL fCaseSensitive );

    // Internal helper overload of item method (see pdl as well)
    HRESULT item(long lIndex, IHTMLStyleSheet** ppHTMLStyleSheet);

protected:
    CStyleSheetArray * _pSSARuleManager;        // The stylesheet array that manages storage for our rules.
                                                //  Only one SSA should manage storage -- the root SSA belonging
                                                //  to the doc.  All others must delegate to it.
private:
    DECLARE_CDataAry(CAryStyleSheets, CStyleSheet *, Mt(Mem), Mt(CStyleSheetArray_aStyleSheets_pv))
    CAryStyleSheets _aStyleSheets;              // Stylesheets held by this collection
    CStyleRuleArray  * _pRulesArrays;           // All rules from all stylesheets in this collection.
    CBase * const _pOwner;                  // CBase-derived obj whose ref-count controls our lifetime.
                                            //  For the top-level SSA, point at the document.
                                            //  For imports SSA, point at the importing SS.
    unsigned long _Level;                    // Nesting level of the stylesheets contained in this array (1-4, not zero-based)


public:
    DECLARE_CPtrAry(CAryFontFaces, CFontFace *, Mt(Mem), Mt(CStyleSheetArray_apFontFaces_pv))
    CAryFontFaces _apFontFaces;    // Downloadable fonts held by this collection.
};

// Unlikely to be a valid ptr value, and even if it is, we still work.
#define UNKNOWN_CLASS_OR_ID     ((LPTSTR)(-1))

MtExtern(CStyleClassIDCache)

class CStyleClassIDCache
{
public:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CStyleClassIDCache))

    enum { MAX_SLOT_INDEX = 3 };
    inline void PutClass( LPCTSTR pszClass, int slot )
        { Assert(slot >= 0); if (slot > MAX_SLOT_INDEX) return; _aCacheEntry[slot]._pszClass = pszClass; }
    inline void PutID( LPCTSTR pszID, int slot )
        { Assert(slot >= 0); if (slot > MAX_SLOT_INDEX) return; _aCacheEntry[slot]._pszID = pszID; }
    inline LPCTSTR GetClass( int slot )
        { Assert(slot >= 0); return ( (slot > MAX_SLOT_INDEX) ? UNKNOWN_CLASS_OR_ID : _aCacheEntry[slot]._pszClass); }
    inline LPCTSTR GetID( int slot )
        { Assert(slot >= 0); return ( (slot > MAX_SLOT_INDEX) ? UNKNOWN_CLASS_OR_ID : _aCacheEntry[slot]._pszID); }
private:  
    struct CCacheEntry
    {
        CCacheEntry() { _pszClass = _pszID = UNKNOWN_CLASS_OR_ID; }
        LPCTSTR _pszClass;
        LPCTSTR _pszID;
    } _aCacheEntry[MAX_SLOT_INDEX+1];
};

MtExtern(CachedStyleSheet)

class CachedStyleSheet
{
public:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CachedStyleSheet))
    CachedStyleSheet (CStyleSheetArray *pssa) : _pCachedSS(NULL), _pssa(pssa), _pRule(NULL)
      {  }
    ~CachedStyleSheet ()
      {  }

    void PrepareForCache (CStyleRule *pRule);	
    LPTSTR GetBaseURL(void);

private:
    CStyleSheetArray   *_pssa;
    unsigned int        _uCachedSheet;
    CStyleSheet        *_pCachedSS;
    CStyleRule		   *_pRule;
};


DWORD TranslateMediaTypeString( LPCTSTR pcszMedia );

#pragma INCMSG("--- End 'sheets.hxx'")
#else
#pragma INCMSG("*** Dup 'sheets.hxx'")
#endif
