//+------------------------------------------------------------------------
//
//  File:       util.cxx
//  Contents:   general utility methods
//
//  History:    06-017-98 ashrafm - created
//
//-------------------------------------------------------------------------

#ifndef _EDUTIL_HXX_
#define _EDUTIL_HXX_ 1

class CSpringLoader;
class CSelectionManager;
class CHTMLEditor;

// TODO: this const should be in mshtml.dll [ashrafm]
const INT NUM_TAGS = TAGID_COUNT;

//
// Define Direction
// NOTE: This can't be an ENUM because it conclicts with another typdef
//
typedef INT Direction;
const INT LEFT = 1;
const INT SAME = 0;
const INT RIGHT = -1;

Direction Reverse( Direction iDir );

#define BREAK_NONE            0x0
#define BREAK_BLOCK_BREAK     0x1
#define BREAK_SITE_BREAK      0x2
#define BREAK_SITE_END        0x4

//
// DLL  hInstance
//

extern HINSTANCE g_hInstance;

extern HINSTANCE g_hEditLibInstance;

//
// Check bitfield utility
//

static inline BOOL CheckFlag( DWORD dwField, DWORD dwTest )
{
    return( 0 != ((DWORD) dwField & dwTest ));
}

static inline DWORD ClearFlag( DWORD dwField, DWORD dwFlagToClear )
{
    return(( dwField ^ dwFlagToClear ) & dwField );
}

//
// Generic BitField implementation
//

template <INT iBitFieldSize>
class CBitField 
{
public:
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CBitField))

// TODO: this should be a static const with initializer but this
// currently produces an internal compiler error [ashrafm]
#define BITFIELD_ARRAYSIZE (iBitFieldSize/8 \
                            + (((iBitFieldSize % 8) == 0)?0:1))

    CBitField() 
    {
        for (INT i = 0; i < BITFIELD_ARRAYSIZE; i++)
        {
            _aBitField[i] = '\0';
        }
    }
    
    BOOL Test(USHORT iBitFieldIndex)
    {
        Assert( (iBitFieldIndex / 8) <= BITFIELD_ARRAYSIZE );
        return ( _aBitField[ iBitFieldIndex / 8 ] & (1 <<  ( iBitFieldIndex % 8 ) ) );
    }

    VOID Set(USHORT iBitFieldIndex)
    {
        Assert( (iBitFieldIndex / 8) <= BITFIELD_ARRAYSIZE );
        _aBitField[iBitFieldIndex / 8] |= ( 1 << (iBitFieldIndex % 8) );
    }

    VOID Clear(USHORT iBitFieldIndex)
    {
        Assert( (iBitFieldIndex / 8) <= BITFIELD_ARRAYSIZE );
        _aBitField[iBitFieldIndex / 8] &= ~( 1 << (iBitFieldIndex % 8) );           
    }

private:
    static const INT _iArraySize;   
    BYTE  _aBitField[BITFIELD_ARRAYSIZE]; 
#undef BITFIELD_ARRAYSIZE
};

//
// Tag bitfield
//
typedef CBitField<NUM_TAGS> CTagBitField;

//
// CBreakContainer - used by segment iterator to manage break points
//

class CBreakContainer
{
public:
    // CBreakContainer mask type
    enum Mask
    {
        BreakOnNone  = 0x0,    
        BreakOnStart = 0x1,    
        BreakOnEnd   = 0x2,
        BreakOnBoth  = (BreakOnStart | BreakOnEnd)
    };

    VOID Set(ELEMENT_TAG_ID tagId, Mask mask);
    VOID Clear(ELEMENT_TAG_ID tagId, Mask mask);
    BOOL Test(ELEMENT_TAG_ID tagId, Mask mask);
    
private:
    CTagBitField bitFieldStart;
    CTagBitField bitFieldEnd;
};



//
// Segment Iterators
//

class CSegmentListIter
{
public:
    CSegmentListIter();
    ~CSegmentListIter();

    HRESULT Init(IMarkupServices *pMarkupServices, IHTMLViewServices *pViewServices, ISegmentList *pSegmentList);

    //
    // Call Next to get the next segment.  Returns S_FALSE if last segment
    //
    
    HRESULT Next(IMarkupPointer **ppLeft, IMarkupPointer **ppRight);
    
private:
    INT             _iSegmentCount;
    INT             _iCurrentSegment;
    IMarkupPointer  *_pLeft, *_pRight;
    ISegmentList    *_pSegmentList;
    IHTMLViewServices *_pViewServices;
};


enum BREAK_CONDITION
{
    BREAK_CONDITION_None            =    0x0,           // Nothing at all
    BREAK_CONDITION_Text            =    0x1,           // Characters
    BREAK_CONDITION_NoScope         =    0x2,           // Script tags, form tags, comments
    BREAK_CONDITION_NoScopeSite     =    0x4,           // Images, inputs, etc.
    BREAK_CONDITION_NoScopeBlock    =    0x8,           // BR's, '\r'...
    
    //
    // These are arranged in pairs, enter first, exit double the enter
    //
    
    BREAK_CONDITION_EnterSite       =    0x10,   
    BREAK_CONDITION_ExitSite        =    0x20,
    BREAK_CONDITION_Site            =    BREAK_CONDITION_EnterSite | BREAK_CONDITION_ExitSite,
    
    BREAK_CONDITION_EnterTextSite   =    0x40,
    BREAK_CONDITION_ExitTextSite    =    0x80,
    BREAK_CONDITION_TextSite        =    BREAK_CONDITION_EnterTextSite | BREAK_CONDITION_ExitTextSite,

    BREAK_CONDITION_EnterBlock      =    0x100,
    BREAK_CONDITION_ExitBlock       =    0x200,
    BREAK_CONDITION_Block           =    BREAK_CONDITION_EnterBlock | BREAK_CONDITION_ExitBlock,

    BREAK_CONDITION_EnterControl    =    0x400,
    BREAK_CONDITION_ExitControl     =    0x800,
    BREAK_CONDITION_Control         =    BREAK_CONDITION_EnterControl | BREAK_CONDITION_ExitControl,

    BREAK_CONDITION_EnterPhrase     =    0x1000,
    BREAK_CONDITION_ExitPhrase      =    0x2000,
    BREAK_CONDITION_Phrase          =    BREAK_CONDITION_EnterPhrase | BREAK_CONDITION_ExitPhrase,

    BREAK_CONDITION_EnterAnchor     =    0x4000,
    BREAK_CONDITION_ExitAnchor      =    0x8000,
    BREAK_CONDITION_Anchor          =    BREAK_CONDITION_EnterAnchor | BREAK_CONDITION_ExitAnchor,

    BREAK_CONDITION_EnterBlockPhrase =   0x10000,
    BREAK_CONDITION_ExitBlockPhrase =    0x20000,
    BREAK_CONDITION_BlockPhrase     =    BREAK_CONDITION_EnterBlockPhrase | BREAK_CONDITION_ExitBlockPhrase,

    //
    // Error and boundary flags
    //
    
    BREAK_CONDITION_Error           =    0x40000,    
    BREAK_CONDITION_Boundary        =    0x80000,      // This is set by default, use as an out condition
    
    //
    // Useful Macro flags
    //
    
    BREAK_CONDITION_OMIT_PHRASE     =    BREAK_CONDITION_Text           | 
                                         BREAK_CONDITION_NoScopeSite    | 
                                         BREAK_CONDITION_NoScopeBlock   |
                                         BREAK_CONDITION_Site           | 
                                         BREAK_CONDITION_Block          |
                                         BREAK_CONDITION_Anchor         |
                                         BREAK_CONDITION_BlockPhrase    |
                                         BREAK_CONDITION_Control,

    BREAK_CONDITION_TEXT            =    BREAK_CONDITION_Text           | 
                                         BREAK_CONDITION_NoScopeSite    | 
                                         BREAK_CONDITION_BlockPhrase    |
                                         BREAK_CONDITION_Anchor         |
                                         BREAK_CONDITION_EnterControl,
                                          
    BREAK_CONDITION_ANYTHING        =    0x0FFFFFFF
};

enum SCAN_OPTION
{
    SCAN_OPTION_None            = 0x0,
    SCAN_OPTION_SkipControls    = 0x1,
    SCAN_OPTION_SkipWhitespace  = 0x2,
    SCAN_OPTION_ChunkifyText    = 0x4,
    SCAN_OPTION_SkipNBSP	= 0x8
};

#define E_HITBOUNDARY 0x8000FFFA


class CEditPointer : public IMarkupPointer
{

    //////////////////////////////////////////////////////////////////
    // Constructor/Destructor
    //////////////////////////////////////////////////////////////////
    public:

        CEditPointer(
            CHTMLEditor *           pEd,
            IMarkupPointer *        pPointer = NULL );
            
        CEditPointer(
            const CEditPointer&     lp );

        virtual ~CEditPointer();


        // Hide the default constructor
    protected:

        CEditPointer();


    
    //////////////////////////////////////////////////////////////////
    // Operator Overloads
    //////////////////////////////////////////////////////////////////
    public:

        operator IMarkupPointer*() 
        {
            return (IMarkupPointer*)_pPointer;
        }

        IMarkupPointer& operator*() 
        {
            Assert(_pPointer!=NULL); 
            return *_pPointer; 
        }

        IMarkupPointer** operator&() 
        {
            ReleaseInterface( _pPointer ); 
            return &_pPointer; 
        } // NOTE: different than ATL

        IMarkupPointer* operator->() 
        {
            Assert( _pPointer!=NULL );
            return _pPointer;
        }

        IMarkupPointer* operator=( 
            IMarkupPointer*         lp)
        { 
            ReplaceInterface( &_pPointer, lp); 
            return (IMarkupPointer*) lp; 
        }

        IMarkupPointer* operator=(
            const CEditPointer&     lp)
        {
            _pEd = lp._pEd;
            ReplaceInterface(&_pPointer, lp._pPointer);
            ReplaceInterface(&_pLeftBoundary, lp._pLeftBoundary );
            ReplaceInterface(&_pRightBoundary, lp._pRightBoundary );
            _fBound = lp._fBound;
            return (IMarkupPointer*)lp._pPointer;
        }
        
        BOOL operator!()
        {
            return (_pPointer == NULL) ? TRUE : FALSE;
        }


    //////////////////////////////////////////////////////////////////
    // CEditPointer Methods
    //////////////////////////////////////////////////////////////////
    public:
    
        HRESULT SetBoundary(                                    // Sets the boundary pointers for this pointer; scan will not move outside pointers specified
            IMarkupPointer *        pLeftBoundary,              //      [in]     Left boundary, may be null
            IMarkupPointer *        pRightBoundary );           //      [in]     Right boundary, may be null

        HRESULT SetBoundaryForDirection(                        // Sets a single pointer boundary in a particular direction
            Direction               eDir,                       //      [in]     Direction
            IMarkupPointer*         pBoundary );                //      [in]     Boundary Pointer
    
        HRESULT ClearBoundary();                                // Eliminates bondary of the pointer, allows unconstrained scanning

        BOOL IsWithinBoundary();                                // Returns TRUE if the pointer is within both boundaries

        BOOL IsWithinBoundary(                                  // Returns TRUE if the pointer is within boundary in the given direction
            Direction               inDir );                    //      [in]     Direction of travel to check

        HRESULT Constrain();                                    // Returns S_OK if pointer is within bounds after constrain (if there are any)

        virtual HRESULT Scan(                                   // Scan in the given direction
            Direction               eDir,                       //      [in]     Direction of travel
            DWORD                   eBreakCondition,            //      [in]     The condition I should break on; also returns what we actually broke on
            DWORD *                 peBreakCondition = NULL,    //      [out]    returns what we actually broke on
            IHTMLElement **         ppElement = NULL,           //      [out]    Element passed over when I terminated, if any
            ELEMENT_TAG_ID *        peTagId = NULL,             //      [out]    Tag of element above
            TCHAR *                 pChar = NULL,               //      [out]    Character that I passed over, if any
            DWORD                   dwScanOptions = SCAN_OPTION_None);             
            
        HRESULT Move(                                           // Directional Wrapper for Left or Right
            Direction               eDir,                       //      [in]     Direction of travel
            BOOL                    fMove,                      //      [in]     Should we actually move the pointer
            MARKUP_CONTEXT_TYPE*    pContext,                   //      [out]    Context change
            IHTMLElement**          ppElement,                  //      [out]    Element we pass over
            long*                   pcch,                       //      [in,out] number of characters to read back
            OLECHAR*                pchText );                  //      [out]    characters

        HRESULT IsEqualTo( 
            IMarkupPointer *        pPointer,                   //      [in]     Pointer to compare "this" to
            DWORD                   dwIgnoreBreaks,             //      [in]     Break conditions to ignore during compare
            BOOL *                  pfEqual );                  //      [out]    Equal?

        HRESULT IsLeftOfOrEqualTo( 
            IMarkupPointer *        pPointer,                   //      [in]     Pointer to compare "this" to
            DWORD                   dwIgnoreBreaks,             //      [in]     Break conditions to ignore during compare
            BOOL *                  pfEqual );                  //      [out]    Equal?

        HRESULT IsRightOfOrEqualTo( 
            IMarkupPointer *        pPointer,                   //      [in]     Pointer to compare "this" to
            DWORD                   dwIgnoreBreaks,             //      [in]     Break conditions to ignore during compare
            BOOL *                  pfEqual );                  //      [out]    Equal?

        HRESULT MoveWord(                                       // Move a word left or right
            Direction               eDir,                       //      [in]     Direction to move
            BOOL *                  pfNotAtBOL      =NULL,      //      [out]    What line am I on after move? (optional)
            BOOL *                  pfAtLogicalBOL  =NULL );    //      [out]    Am I at the lbol after move? (optional)

        HRESULT MoveCharacter(                                  // Move a character left or right
            Direction               eDir,                       //      [in]     Direction to move
            BOOL *                  pfNotAtBOL      =NULL,      //      [out]    What line am I on after move? (optional)
            BOOL *                  pfAtLogicalBOL  =NULL );    //      [out]    Am I at the lbol after move? (optional)

        static BOOL CheckFlag( DWORD dwBreakResults, DWORD dwTest )
        {
            return( 0 != ((DWORD) dwBreakResults & dwTest ));
        }
        
        BOOL Between( IMarkupPointer* pStart, IMarkupPointer* pEnd );

        BOOL IsInDifferentEditableSite();
        
#if DBG == 1
        void DumpTree()                                         // Inline debugging aid to dump the tree
        {
            Assert( _pPointer );

            IMarkupContainer * pTree = NULL;
            IGNORE_HR( _pPointer->GetContainer( & pTree ));
            extern void dt( IUnknown * );
            dt( pTree );
            ReleaseInterface( pTree );
        }
#endif // DBG == 1


    //////////////////////////////////////////////////////////////////
    // CEditPoitner Private Methods
    //////////////////////////////////////////////////////////////////
    protected:
    
        BOOL IsPointerInLeftBoundary();                         // Returns TRUE if the pointer is to the right of or equal to the left boundary if there is one
        BOOL IsPointerInRightBoundary();                        // Returns TRUE if the pointer is to the left of or equal to the right boundary if there is one
        HRESULT MoveUnit(                                       // Move a character left or right
            Direction               eDir,                       //      [in]     Direction to move
            MOVEUNIT_ACTION         eUnit,                      //      [in]     Type of move unit to use
            BOOL *                  pfNotAtBOL      =NULL,      //      [out]    What line am I on after move? (optional)
            BOOL *                  pfAtLogicalBOL  =NULL );    //      [out]    Am I at the lbol after move? (optional)


    //////////////////////////////////////////////////////////////////
    // Instance Variables
    //////////////////////////////////////////////////////////////////
    private:

        CHTMLEditor *               _pEd;                       // Editor
        IMarkupPointer *            _pPointer;                  // MarkupPointer we work on
        IMarkupPointer *            _pLeftBoundary;             // Left Boundary of Iterator
        IMarkupPointer *            _pRightBoundary;            // Right Boundary of Iterator
        BOOL                        _fBound;                    // Is this pointer constrained?
    


    //////////////////////////////////////////////////////////////////
    // IUnknown Methods
    //////////////////////////////////////////////////////////////////
    public:

        STDMETHODIMP_(ULONG) AddRef( void )
        {
            Assert( _pPointer );
            return _pPointer->AddRef();
        }

        STDMETHODIMP_(ULONG) Release( void ) 
        {
            Assert( _pPointer );
            return _pPointer->Release();
        }

        STDMETHODIMP QueryInterface(
            REFIID              iid, 
            LPVOID *            ppv )
        {
            Assert( _pPointer );
            return _pPointer->QueryInterface( iid, ppv );
        }


    //////////////////////////////////////////////////////////////////
    // IMarkupPointer Methods
    //////////////////////////////////////////////////////////////////
    public:

        HRESULT STDMETHODCALLTYPE OwningDoc(
                /* [out] */ IHTMLDocument2** ppDoc )
        {
            Assert( _pPointer );
            return _pPointer->OwningDoc( ppDoc );
        }

        HRESULT STDMETHODCALLTYPE Gravity(
                /* [out] */ POINTER_GRAVITY* pGravity )
        {
            Assert( _pPointer );
            return _pPointer->Gravity( pGravity );
        }


        HRESULT STDMETHODCALLTYPE SetGravity(
                /* [in] */ POINTER_GRAVITY Gravity)
        {
            Assert( _pPointer );
            return _pPointer->SetGravity( Gravity );
        }

        HRESULT STDMETHODCALLTYPE Cling(
                /* [out] */ BOOL* pfCling)
        {
            Assert( _pPointer );
            return _pPointer->Cling( pfCling );
        }

        HRESULT STDMETHODCALLTYPE SetCling(
                /* [in] */ BOOL fCLing)
        {
            Assert( _pPointer );
            return _pPointer->SetCling( fCLing );
        }

        HRESULT STDMETHODCALLTYPE Unposition( )
        {
            Assert( _pPointer );
            return _pPointer->Unposition();
        }

        HRESULT STDMETHODCALLTYPE IsPositioned(
                /* [out] */ BOOL* pfPositioned)
        {
            Assert( _pPointer );
            return _pPointer->IsPositioned( pfPositioned );
        }

        HRESULT STDMETHODCALLTYPE GetContainer(
                /* [out] */ IMarkupContainer** ppContainer)
        {
            Assert( ppContainer );
            return _pPointer->GetContainer( ppContainer );
        }

        HRESULT STDMETHODCALLTYPE MoveAdjacentToElement(
                /* [in] */ IHTMLElement* pElement,
                /* [in] */ ELEMENT_ADJACENCY eAdj)
        {
            Assert( _pPointer );
            return _pPointer->MoveAdjacentToElement( pElement, eAdj );
        }

        HRESULT STDMETHODCALLTYPE MoveToPointer(
                /* [in] */ IMarkupPointer* pPointer)
        {
            Assert( _pPointer );
            return _pPointer->MoveToPointer( pPointer );
        }

        HRESULT STDMETHODCALLTYPE MoveToContainer(
                /* [in] */ IMarkupContainer* pContainer,
                /* [in] */ BOOL fAtStart)
        {
            Assert( _pPointer );
            return _pPointer->MoveToContainer( pContainer, fAtStart );
        }

        HRESULT STDMETHODCALLTYPE Left(
                /* [in] */ BOOL fMove,
                /* [out] */ MARKUP_CONTEXT_TYPE* pContext,
                /* [out] */ IHTMLElement** ppElement,
                /* [in, out] */ long* pcch,
                /* [out] */ OLECHAR* pchText)
        {
            Assert( _pPointer );
            return _pPointer->Left( fMove, pContext, ppElement, pcch, pchText );
        }                

        HRESULT STDMETHODCALLTYPE Right(
                /* [in] */ BOOL fMove,
                /* [out] */ MARKUP_CONTEXT_TYPE* pContext,
                /* [out] */ IHTMLElement** ppElement,
                /* [in, out] */ long* pcch,
                /* [out] */ OLECHAR* pchText)
        {
            Assert( _pPointer );
            return _pPointer->Right( fMove, pContext, ppElement, pcch, pchText );
        }                

        HRESULT STDMETHODCALLTYPE CurrentScope(
                /* [out] */ IHTMLElement** ppElemCurrent)
        {
            Assert( _pPointer );
            return _pPointer->CurrentScope( ppElemCurrent );
        }

        HRESULT STDMETHODCALLTYPE IsLeftOf( IMarkupPointer * pIPointerThat, BOOL * pfResult )
            { return _pPointer->IsLeftOf( pIPointerThat, pfResult ); }

        HRESULT STDMETHODCALLTYPE IsRightOf( IMarkupPointer * pIPointerThat, BOOL * pfResult )
            { return _pPointer->IsRightOf( pIPointerThat, pfResult ); }

        HRESULT STDMETHODCALLTYPE IsLeftOfOrEqualTo( IMarkupPointer * pIPointerThat, BOOL * pfResult )
            { return _pPointer->IsLeftOfOrEqualTo( pIPointerThat, pfResult ); }

        HRESULT STDMETHODCALLTYPE IsRightOfOrEqualTo( IMarkupPointer * pIPointerThat, BOOL * pfResult )
            { return _pPointer->IsRightOfOrEqualTo( pIPointerThat, pfResult ); }

        HRESULT STDMETHODCALLTYPE IsEqualTo(
                /* [in] */ IMarkupPointer* pPointerThat,
                /* [out] */ BOOL* pfAreEqual)
        {
            Assert( _pPointer );
            return _pPointer->IsEqualTo( pPointerThat, pfAreEqual );
        }                

        HRESULT STDMETHODCALLTYPE MoveUnit(
                /* [in] */ MOVEUNIT_ACTION muAction)
        {
            Assert( _pPointer );
            return _pPointer->MoveUnit( muAction );
        }                

        HRESULT STDMETHODCALLTYPE FindText(
                /* [in] */ OLECHAR* pchFindText,
                /* [in] */ DWORD dwFlags,
                /* [in] */ IMarkupPointer* pIEndMatch,
                /* [in] */ IMarkupPointer* pIEndSearch)
        {
            Assert( _pPointer );
            return _pPointer->FindText( pchFindText, dwFlags, pIEndMatch, pIEndSearch );
        }                
};


#ifdef NEVER
class CPointerPairSegmentList : public ISegmentList 
{
public:
    
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CPointerPairSegmentList));

    CPointerPairSegmentList( IMarkupServices* pMarkupServices, IMarkupPointer* pStart, IMarkupPointer* pEnd );

    CPointerPairSegmentList( IMarkupServices* pMarkupServices, IHTMLElement* pIElement );

    ~CPointerPairSegmentList();

    // --------------------------------------------------
    // IUnknown Interface
    // --------------------------------------------------

    STDMETHODIMP_(ULONG)
    AddRef( void ) ;

    STDMETHODIMP_(ULONG)
    Release( void ) ;

    STDMETHODIMP
    QueryInterface(
        REFIID              iid, 
        LPVOID *            ppv ) ;

    //
    // ISegmentList
    //
    STDMETHOD ( MovePointersToSegment ) ( 
        int iSegmentIndex, 
        IMarkupPointer* pILeft, 
        IMarkupPointer* pIRight ) ;

    STDMETHOD( GetSegmentCount) (
        int* piSegmentCount,
        SELECTION_TYPE * peType );

    private:
        HRESULT Init( IMarkupServices * pMarkupServices );
        
        IMarkupServices* _pMarkupServices;
        IMarkupPointer* _pStart;
        IMarkupPointer* _pEnd;
};
#endif

struct COMPOSE_SETTINGS
{
    int       _fComposeSettings;
    int       _fBold;
    int       _fItalic;
    int       _fUnderline;
    int       _fSuperscript;
    int       _fSubscript;
    int       _lSize;
    OLE_COLOR _color;
    OLE_COLOR _colorBg;
    CVariant  _varFont;
    CVariant  _varSpanClass;
    int       _fUseOutsideSpan;
};

class CUndoUnit
{
public:    
    CUndoUnit(CHTMLEditor *pEd);
    ~CUndoUnit();
    
    HRESULT Begin(UINT uiStringId);

private:
    CHTMLEditor *_pEd;
    BOOL        _fEndUndo;
};

class CStringCache
{
public:
    CStringCache(UINT uiStart, UINT uiEnd);
    ~CStringCache();

    TCHAR *GetString(UINT uiString);

private:
    UINT _uiStart;
    UINT _uiEnd;

    struct CacheEntry
    {
        TCHAR *pchString;
    };

    CacheEntry *_pCache;
};

//////////////////////////////////////////////////////////////////////////////////
//      ****************   DO NOT ADD METHODS TO EDUTIL   ***************
//
//  ALL PUBLIC UTILITIES SHOULD BE MOVED INTO THE CHTMLEDITOR OBJECT.  IF STATIC
//  ROUTINES NEED ACCESS TO THESE UTILITIES, THEY MUST TAKE A CHTMLEDITOR AS
//  A PARAMETER.
//
//  --- JOHNBED 09/16/98 ----
//////////////////////////////////////////////////////////////////////////////////

namespace EdUtil
{

    //
    // General markup services helpers
    //

    HRESULT CopyMarkupPointer(
        IMarkupServices *pMarkupServices, 
        IMarkupPointer  *pSource, 
        IMarkupPointer  **ppDest );

    //
    // These helpers where taken from mshtml.dll
    //

    BOOL VariantCompareColor(VARIANT * pvarColor1, VARIANT * pvarColor2);
    BOOL VariantCompareFontName(VARIANT * pvarColor1, VARIANT * pvarColor2);
    BOOL VariantCompareFontSize(VARIANT * pvarColor1, VARIANT * pvarColor2);
    BOOL VariantCompareBSTRS(VARIANT * pvar1, VARIANT * pvar2);
    BOOL VariantCompareFontSize(VARIANT * pvarSize1, VARIANT * pvarSize2);

    HRESULT ConvertRGBToOLEColor(VARIANT *pvarargIn);
    HRESULT ConvertOLEColorToRGB(VARIANT *pvarargIn);

    HRESULT FormsAllocStringW(LPCWSTR pch, BSTR * pBSTR);
#if defined(_MAC) || defined(WIN16)
    HRESULT FormsAllocStringA(LPCSTR pch, BSTR * pBSTR);
#ifndef WIN16
    HRESULT FormsAllocStringA(LPCWSTR pwch, BSTR * pBSTR);
#endif // !WIN16
#endif //_MAC

    int ConvertHtmlSizeToTwips(int nHtmlSize);
    int ConvertTwipsToHtmlSize(int nFontSize);

    BOOL IsWhiteSpace(TCHAR ch);


    //
    // Generally useful util functions
    //

    HRESULT MovePointersToSegmentHelper(
        IHTMLViewServices* pViewServices,
        ISegmentList*    pSegmentList,
        INT              iSegmentIndex, 
        IMarkupPointer** ppIStart, 
        IMarkupPointer** ppIEnd,
        BOOL             fCheckPtrOrder = TRUE,
        BOOL             fCheckCurScope = TRUE );
 
    BOOL IsListContainer(ELEMENT_TAG_ID tagId);
    BOOL IsListItem(ELEMENT_TAG_ID tagId);

    HRESULT FindCommonElement( 
        IMarkupServices *pMarkupServices,
        IHTMLViewServices *pViewServices,
        IMarkupPointer  *pStart, 
        IMarkupPointer  *pEnd,
        IHTMLElement    **ppElement );

    HRESULT FindBlockElement( 
        IMarkupServices  *pMarkupServices,
        IHTMLElement     *pElement, 
        IHTMLElement     **pBlockElement );

   HRESULT ReplaceElement( 
        IMarkupServices *pMarkupServices,
        IHTMLElement    *pOldElement,
        IHTMLElement    *pNewElement,
        IMarkupPointer  *pStart = NULL,
        IMarkupPointer  *pEnd = NULL);
                            
    HRESULT CopyAttributes( IHTMLViewServices *pViewSrv, IHTMLElement * pSrcElement, IHTMLElement * pDestElement, BOOL fCopyId);

    HRESULT MovePointerOutOfScope( 
        IMarkupServices *   pMarkupServices,
        IHTMLViewServices * pViewServices,
        IMarkupPointer *    pPointer, 
        Direction           eDirection,
        IMarkupPointer *    pBoundary,
        BOOL *              pfAdjustedPointer,
        BOOL                fStopAtBlockOnExitScope = TRUE,
        BOOL                fStopAtBlockOnEnterScope = TRUE ,
        BOOL                fStopAtLayout = TRUE,
        BOOL                fStopAtIntrinscic = TRUE );
        
    HRESULT MovePointerToText( 
        IMarkupServices *   pMarkupServices,
        IHTMLViewServices * pViewServices,
        IMarkupPointer *    pPointer, 
        Direction           eDirection,
        IMarkupPointer *    pBoundary,
        BOOL *              pfHitText,
        BOOL fStopAtBlock = TRUE);
    
    HRESULT ExpandToWord(
        IMarkupServices   * pMarkupServices,
        IHTMLViewServices * pViewServices, 
        IMarkupPointer    * pmpStart,
        IMarkupPointer    * pmpEnd);

    BOOL    PointersInSameFlowLayout( IMarkupPointer * pStart, 
                                      IMarkupPointer * pEnd, 
                                      IHTMLElement ** ppFlowElement,
                                      IHTMLViewServices* pViewServices );

    HRESULT    FindBlockLimit(
                                IMarkupServices*    pMarkupServices, 
                                IHTMLViewServices*  pViewServices,
                                Direction           direction, 
                                IMarkupPointer      *pPointer, 
                                IHTMLElement        **ppElement, 
                                MARKUP_CONTEXT_TYPE *pContext,
                                BOOL                fExpanded,
                                BOOL                fLeaveInside = FALSE ,
                                BOOL                fCanCrossLayout = FALSE );

    HRESULT BlockMove(
                                IMarkupServices         * pMarkupServices,
                                IHTMLViewServices       * pViewServices,
                                IMarkupPointer          *pMarkupPointer, 
                                Direction               direction, 
                                BOOL                    fMove,
                                MARKUP_CONTEXT_TYPE *   pContext,
                                IHTMLElement * *        ppElement);

    HRESULT BlockMoveBack(
                            IMarkupServices*        pMarkupServices,
                            IHTMLViewServices*      pViewServices,
                            IMarkupPointer          *pMarkupPointer, 
                            Direction               direction, 
                            BOOL                    fMove,
                            MARKUP_CONTEXT_TYPE *   pContext,
                            IHTMLElement * *        ppElement);

    BOOL DoesSegmentContainText( IMarkupServices * pMarkupServices, IHTMLViewServices *pViewServices, IMarkupPointer *pStart, IMarkupPointer *pEnd);

    BOOL IsBlockCommandLimit( IMarkupServices* pMarkupServices, IHTMLViewServices* pViewServices, IHTMLElement *pElement, MARKUP_CONTEXT_TYPE context) ;

    BOOL IsElementPositioned(IHTMLElement* pElement);

    BOOL IsElementSized(IHTMLElement* pElement);
    
    BOOL IsIntrinsic( IMarkupServices* pMarkupServices, IHTMLElement* pIHTMLElement );
    
#if DBG == 1    
    void MessageTypeToString( TCHAR* pAryMsg, UINT message );
#endif

    HRESULT MoveAdjacentToElementHelper(IMarkupPointer *pMarkupPointer, IHTMLElement *pElement, ELEMENT_ADJACENCY elemAdj);

    BOOL IsShiftKeyDown();

    BOOL IsControlKeyDown();

    BOOL IsArrowKey( SelectionMessage* pMessage)    ;
    
    HRESULT FindListContainer( IMarkupServices *pMarkupServices,
                               IHTMLElement    *pElement,
                               IHTMLElement    **pListContainer );

    HRESULT FindListItem( IMarkupServices *pMarkupServices,
                          IHTMLElement    *pElement,
                          IHTMLElement    **pListContainer );

    HRESULT FindTagAbove( IMarkupServices *pMarkupServices,
                          IHTMLElement    *pElement,
                          ELEMENT_TAG_ID  tagIdGoal,
                          IHTMLElement    **ppElement,
                          BOOL            fStopAtLayout = FALSE);

    HRESULT FindContainer( IMarkupServices *pMarkupServices,
                           IHTMLElement    *pElement,
                           IHTMLElement    **pContainer );

    HRESULT InsertBlockElement(
        IMarkupServices *pMarkupServices,
        IHTMLElement    *pElement, 
        IMarkupPointer  *pStart, 
        IMarkupPointer  *pEnd,
        IMarkupPointer  *pCaret);

    //
    // Some helper functions from wutils.cxx
    //
    ULONG NextEventTime(ULONG ulDelta);        

    BOOL IsTimePassed(ULONG ulTime);        

    HRESULT GetEditResourceLibrary(
        HINSTANCE       *hResourceLibrary);

    BOOL SameElements(
        IHTMLElement *      pElement1,
        IHTMLElement *      pElement2 );

    BOOL EquivalentElements( IMarkupServices* pMarkupServices, IHTMLElement* pIElement1, IHTMLElement* pIElement2 );        

    HRESULT InsertElement(IMarkupServices *pMarkupServices, IHTMLElement *pElement, IMarkupPointer *pStart, IMarkupPointer *pEnd);

    HRESULT NeedsNewBlock(IMarkupServices *pMarkupServices, IMarkupPointer *pLeft, IMarkupPointer *pRight, BOOL *pfNeedsBlock);

    BOOL IsInWindow( HWND hwnd, POINT pt , BOOL fConvertToScreen = FALSE );
    
}; // namespace EdUtil

    HRESULT AutoUrl_DetectCurrentWord( IMarkupServices *pMS, IMarkupPointer *pWord, OLECHAR * pChar, BOOL *pfInsideLink, BOOL *pfOutsideLink = NULL, IMarkupPointer *pLimit = NULL, IMarkupPointer *pLeft = NULL, BOOL *pfFound = NULL );
    HRESULT AutoUrl_DetectRange( IMarkupServices *pMS, IMarkupPointer *pStart, IMarkupPointer *pEnd, BOOL fValidate = TRUE, IMarkupPointer *pLimit = NULL );
    HRESULT AutoUrl_ShouldUpdateAnchorText( OLECHAR * pstrHref, OLECHAR * pstrAnchorText, BOOL * pfResult );


//
// Define smart pointers
//

template <class T>
class CSmartPtr
{
public:
    typedef T _PtrClass;
    CSmartPtr() {p=NULL;}
    CSmartPtr(T* lp)
    {
        if ((p = lp) != NULL)
            p->AddRef();
    }
    CSmartPtr(const CSmartPtr<T>& lp)
    {
        if ((p = lp.p) != NULL)
            p->AddRef();
    }
    ~CSmartPtr() {if (p) p->Release();}
    void Release() {if (p) p->Release(); p=NULL;}
    operator T*() {return (T*)p;}
    T& operator*() {Assert(p!=NULL); return *p; }
    T** operator&() { Release(); return &p; } // NOTE: different than ATL
    T* operator->() { Assert(p!=NULL); return p; }
    T* operator=(T* lp){ReplaceInterface(&p, lp); return (T*)lp;}
    T* operator=(const CSmartPtr<T>& lp)
    {
        ReplaceInterface(&p, lp.p); return (T*)lp.p;
    }
    BOOL operator!(){return (p == NULL) ? TRUE : FALSE;}
    T* p;
};

#define DefineSmartPointer(pointerType) typedef CSmartPtr<pointerType> SP_ ## pointerType

DefineSmartPointer(IHTMLElement);
DefineSmartPointer(IHTMLElement2);
DefineSmartPointer(IHTMLDocument3);
DefineSmartPointer(IHTMLStyle);
DefineSmartPointer(IMarkupPointer);
DefineSmartPointer(IObjectIdentity);
DefineSmartPointer(ISegmentList);
DefineSmartPointer(IHTMLCaret);
DefineSmartPointer(ISelectionRenderingServices);    
DefineSmartPointer(IOleCommandTarget);
//
// Error macros
//

#define IFR(expr) {hr = THR(expr); if (FAILED(hr)) RRETURN1(hr, S_FALSE);}
#define IFC(expr) {hr = THR(expr); if (FAILED(hr)) goto Cleanup;}


#endif

