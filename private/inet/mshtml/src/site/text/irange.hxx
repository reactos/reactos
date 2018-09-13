//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1998.
//
//  File:       irange.hxx
//
//  Contents:   Declaration of CAutoRange, an implementation of IHTMLTxtRange
//
//  Classes:    CAutoRange
//
//----------------------------------------------------------------------------

#ifndef I_IRANGE_HXX_
#define I_IRANGE_HXX_


#define _hxx_
#include "range.hdl"

typedef enum {
    MOVERANGE_Both,
    MOVERANGE_Left,
    MOVERANGE_Right
} MOVERANGE;

typedef enum {
    MV_DIR_Left,
    MV_DIR_Right
} MV_DIR;

typedef enum {
    CR_Text,
    CR_NoScope,
    CR_Intrinsic,
    CR_Boundary,
    CR_LineBreak,
    CR_BlockBreak,
    CR_TextSiteBreak,
    CR_TextSiteEnd,
    CR_Failed
} CLING_RESULT;


MtExtern(CAutoRange)


class CAutoRange : public CBase, 
                   public IHTMLTxtRange,
                   public IDispatchEx,
                   public ISegmentList
{
public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CAutoRange))

    CAutoRange ( CMarkup * pMarkup  , CElement * pElemContainer );
    ~CAutoRange();

    BOOL IsCompatibleWith ( IHTMLTxtRange * );
    
    DECLARE_PLAIN_IUNKNOWN ( CAutoRange )
    DECLARE_DERIVED_DISPATCHEx2 ( CBase )
    DECLARE_TEAROFF_TABLE(IOleCommandTarget)

    // Need to support getting the atom table for expando support on IDEX2.
            
    virtual CAtomTable * GetAtomTable ( BOOL * pfExpando = NULL )
    {
        if (pfExpando)
            *pfExpando = GetMarkup()->Doc()->_fExpando;
        
        return & GetMarkup()->Doc()->_AtomTable;
    }

    DECLARE_PRIVATE_QI_FUNCS(CBase)

    // IOleCommandTarget methods
    
    NV_STDMETHOD(QueryStatus) (
            GUID * pguidCmdGroup,
            ULONG cCmds,
            MSOCMD rgCmds[],
            MSOCMDTEXT * pcmdtext);

    NV_STDMETHOD(Exec) (
            GUID * pguidCmdGroup,
            DWORD nCmdID,
            DWORD nCmdexecopt,
            VARIANTARG * pvarargIn,
            VARIANTARG * pvarargOut);

    
    virtual HRESULT CloseErrorInfo ( HRESULT hr );

    #define _CAutoRange_
    #include "range.hdl"

    // ISegmentList methods
    STDMETHOD ( MovePointersToSegment ) ( 
        int iSegmentIndex, 
        IMarkupPointer* pILeft, 
        IMarkupPointer* pIRight ) ;

    STDMETHOD( GetSegmentCount) (
        int* piSegmentCount,
        SELECTION_TYPE * peType );

    STDMETHOD( MoveSegmentToPointers ) ( 
        int iSegmentIndex,
        IMarkupPointer * pILeft, 
        IMarkupPointer * pIRight ) ;
   
    HRESULT GetLeft( IMarkupPointer * pmp );

    HRESULT GetLeft( CMarkupPointer * pmp );

    HRESULT GetRight( IMarkupPointer * pmp );

    HRESULT GetRight( CMarkupPointer * pmp );

    HRESULT GetLeftAndRight( CMarkupPointer * pLeft, CMarkupPointer * pRight );

    HRESULT SetLeft( IMarkupPointer * pmp );

    HRESULT SetRight( IMarkupPointer * pmp );

    HRESULT SetLeftAndRight( IMarkupPointer * pLeft, IMarkupPointer * pRight, BOOL fAdjustPointers = TRUE  );

    CElement * GetContainer()
    {
        return _pElemContainer;
    }

    CTreeNode * LeftNode();

    CTreeNode * RightNode();

    BOOL IsOrphaned ( ) { return !GetMarkup()->Root(); }

    BOOL IsDocLoading ( ) { return GetMarkup()->Doc()->IsLoading(); }
    BOOL IsMarkupLoading ( ) { return ! GetMarkup()->GetLoaded(); }

    CMarkup *GetMarkup() { return _pMarkup; }


    HRESULT     SetSelectionInlineDirection(htmlDir atDir); // BUGBUG: remove this

    virtual CElement *GetCommonContainer();

    virtual CTreeNode *GetCommonNode();
    virtual BOOL OwnedBySingleFlowLayout();
    virtual BOOL SelectionInOneFlowLayout();

    CFlowLayout *GetCommonFlowLayout()
    {
        return GetCommonNode()->GetFlowLayout();
    }

    BOOL FSupportsHTML();

    // Complex Text Helper
    inline BOOL IsDefaultAlignment(BOOL fRTL, htmlBlockAlign atAlign)
    {
        return (fRTL ? atAlign == htmlBlockAlignRight : atAlign == htmlBlockAlignLeft); 
    }
   
    HRESULT SaveHTMLToStream (CStreamWriteBuff * pswb, DWORD dwMode);

    // Set the range to encompass text influenced by given element
    HRESULT SetTextRangeToElement( CElement * pElement );

    // Helper function to fetch a BSTR from an HTML stream in a given mode
    HRESULT GetBstrHelper(BSTR *pbStr, DWORD dwSaveMode, DWORD dwStrWrBuffFlags);

    WHEN_DBG( void DumpTree(); )

    void SetNext( CAutoRange * pNextRange ) 
    { _pNextRange = pNextRange; }

    CAutoRange * GetNext()
    { return _pNextRange; }

protected:
    
    static const CLASSDESC s_classdesc;
    
    virtual const CBase::CLASSDESC *GetClassDesc() const { return & s_classdesc; }

private:

    IMarkupPointer * _pLeft;       // Left boundary of the range
  
    IMarkupPointer * _pRight;      // Right boundary of the range 
    
    CMarkup *       _pMarkup;

    CElement *      _pElemContainer;

    CAutoRange *    _pNextRange;

    CPtrAry<IMarkupPointer *> * _paryAdjacentRangePointers; 
    
    HRESULT GetRangeTopLeft(POINT * pPt, BOOL fScreenCoord = TRUE);
    HRESULT GetRangeBoundingRect(RECT *prc, BOOL fScreenCoord = TRUE);
    HRESULT GetRangeBoundingRects(CDataAry<RECT> * pRects, BOOL fScreenCoord = TRUE);
    HRESULT GetMoveUnitAndType( BSTR bstrUnit, 
                                long Count, 
                                MOVEUNIT_ACTION *pmuAction,
                                htmlUnit * phtmlUnit );


    HRESULT moveRange ( BSTR bstrUnit, long Count, long * pActualCount, int moveWhat );

    HRESULT MoveUnitWithinRange( IMarkupPointer * pPointerToMove, 
                                 MOVEUNIT_ACTION muAction,
                                 long * pnActualCount );

    HRESULT MoveWithinBoundary( IMarkupPointer *pPointerToMove, 
                               MOVEUNIT_ACTION muAction, 
                               IMarkupPointer *pBoundary,
                               BOOL            fLeftBound );

    HRESULT MovePointersToRangeBoundary ( IMarkupPointer ** ppLeftBoundary,
                                         IMarkupPointer ** ppRightBoundary );

    BOOL IsTextInIE4CompatiblePlace( IMarkupPointer * pmpLeft, IMarkupPointer * pmpRight );

    HRESULT CompareRangePointers( IMarkupPointer * pPointerSource, 
                                  IMarkupPointer * pPointerTarget, 
                                  int * piReturn );

    HRESULT MoveCharacter ( IMarkupPointer * pPointerStart,
                            MOVEUNIT_ACTION muAction,
                            IMarkupPointer * pLeftBoundary,
                            IMarkupPointer * pRightBoundary,
                            IMarkupPointer * pJustBefore = NULL );

    HRESULT MoveWord ( IMarkupPointer * pPointerStart,
                       MOVEUNIT_ACTION  muAction,
                       IMarkupPointer * pLeftBoundary,
                       IMarkupPointer * pRightBoundary );

    WHEN_DBG( void SanityCheck ( ); )


    CEditRouter    _EditRouter;

    HRESULT     ValidatePointers();

    BOOL IsPhraseElement( IHTMLElement *pElement );

    HRESULT AdjustLeftIntoEmptyPhrase( IMarkupPointer *pLeft );    

    HRESULT AdjustPointers( IMarkupPointer *pLeft, IMarkupPointer* pRight);

    HRESULT IsRangeEquivalentToSelection(BOOL* pfEquivalent );
    HRESULT KeepRangeLeftToRight();

    BOOL    IsRangeCollapsed();

    CLING_RESULT    ClingToText( IMarkupPointer *    pPointer, 
                                 IMarkupPointer *    pBoundary, 
                                 MV_DIR              eDir );
            
    HRESULT     AdjustForInsert( IMarkupPointer *    pToMovePointer );

    HRESULT     AdjustIntoTextSite( IMarkupPointer *    pPointerToMove,
                                    MV_DIR              Dir,
                                    IMarkupPointer *    pBoundary );

    HRESULT     GetSiteContainer( IMarkupPointer *      pPointer,
                                  IHTMLElement **       ppSite,
                                  BOOL *                pfText );

    HRESULT     GetSiteContainer( IHTMLElement *        pElementStart, 
                                  IHTMLElement **       ppSite,
                                  BOOL *                pfText );

    HRESULT CheckOwnerSiteOrSelection(ULONG cmdID);

    CTreeNode *  GetNode( BOOL fLeft );

    HRESULT InitPointers();
    HRESULT FlipRangePointers();

    BOOL        CheckSecurity(LPCWSTR pszCmdId);

    void        RemoveLookAsideEntry();

    HRESULT     AdjustRangePointerGravity( IMarkupPointer * pRangePointer );

    HRESULT     EnsureAdjacentRangesGravity();

    HRESULT     StoreAdjacentRangePointer( IMarkupPointer * pAdjacentPointer );

    HRESULT     RestoreAdjacentRangesGravity();

    void        ClearAdjacentRangePointers();

};

#endif // _IRANGE_HXX_
