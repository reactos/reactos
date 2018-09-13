//+------------------------------------------------------------------------
//
//  File:       Selrensv
//
//  Contents:   Implementation of SelectionRenderingServices - put in TxtEdit
//      
//  History:    06/04/98 marka - created
//
//-------------------------------------------------------------------------

#ifndef _SELRENSV_HXX_
#define _SELRENSV_HXX_ 1

MtExtern( CSelectionRenderingServiceProvider )

struct PointerSegment
{
    CMarkupPointer* _pStart;
    CMarkupPointer* _pEnd;
    HIGHLIGHT_TYPE _HighlightType;
    int _cpStart;
    int _cpEnd;
    BOOL _fFiredSelectionNotify; // Has the SelectionNotification been fired on this segment
};

struct HighlightSegment
{
    int     _cpStart;
    int     _cpEnd;
    DWORD   _dwHighlightType;
};

#define WM_BEGINSELECTION       (WM_USER + 1201) // Posted by Selection to say we've begun a selection
#define START_TEXT_SELECTION    1
#define START_CONTROL_SELECTION 2

class CElementAdorner;

class CSelectionRenderingServiceProvider  
{
public:    
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CSelectionRenderingServiceProvider))
#if DBG == 1
    CSelectionRenderingServiceProvider(CDoc * pDoc, CMarkup* pMarkup );
#else
    CSelectionRenderingServiceProvider(CDoc * pDoc);
#endif

    ~CSelectionRenderingServiceProvider();
    //
    // ISelectionRenderServices Interface 
    //
    STDMETHOD ( AddSegment ) ( 
        IMarkupPointer* pStart, 
        IMarkupPointer* pEnd,
        HIGHLIGHT_TYPE HighlightType,
        int * iSegmentIndex )  ;
        
    STDMETHOD ( AddElementSegment ) ( 
        IHTMLElement* pElement ,
        int * iSegmentIndex )  ;
        
    STDMETHOD ( MovePointersToSegment ) ( 
        int iSegmentIndex, 
        IMarkupPointer* pStart, 
        IMarkupPointer* pEnd ) ;

    STDMETHOD( GetElementSegment ) ( 
        int iSegmentIndex, 
        IHTMLElement** ppElement ) ;
        
    STDMETHOD( MoveSegmentToPointers ) ( 
        int iSegmentIndex,
        IMarkupPointer* pStart, 
        IMarkupPointer* pEnd,
        HIGHLIGHT_TYPE HighlightType )  ;
        
    STDMETHOD( SetElementSegment ) ( 
        int iSegmentIndex,
        IHTMLElement* pElement )  ;

    STDMETHOD( GetSegmentCount) (
        int* piSegmentCount,
        SELECTION_TYPE * peType);

    STDMETHOD( ClearSegment ) (
        int iSegmentIndex,
        BOOL fInvalidate );

    STDMETHOD ( ClearSegments ) ( BOOL fInvalidate );

    STDMETHOD ( ClearElementSegments ) () ;

    VOID GetSelectionChunksForLayout( CFlowLayout* pFlowLayout, CPtrAry<HighlightSegment*> *paryHighlight, int* pCpMin, int * pCpMax );

    HRESULT InvalidateSegment( CMarkupPointer* pStart, CMarkupPointer *pEnd, CMarkupPointer* pNewStart, CMarkupPointer* pNewEnd , BOOL fSelected, BOOL fFireOM = TRUE );
    
    HRESULT UpdateSegment( CMarkupPointer* pOldStart, CMarkupPointer* pOldEnd,CMarkupPointer* pNewStart, CMarkupPointer* pNewEnd);

    BOOL IsElementSelected( CElement* pElement );

    CElement* GetSelectedElement(int iElementIndex);

    HRESULT GetFlattenedSelection( int iSegmentIndex, int & cpStart, int & cpEnd, SELECTION_TYPE&  eType )    ;

    VOID HideSelection();

    VOID ShowSelection();

    VOID InvalidateSelection(BOOL fSelectionOn, BOOL fFireOM );
    
private:

    VOID ConstructSelectionRenderCache();

    BOOL IsLayoutCompletelyEnclosed( CLayout* pLayout, CMarkupPointer* pStart, CMarkupPointer* pEnd );
    
    CDoc* _pDoc;
    
    CPtrAry<PointerSegment*> * _parySegment;    // Array of all segments
    
    CPtrAry<CElement*> * _paryElementSegment; // Array of Element Segments.

    HRESULT NotifyBeginSelection( WPARAM wParam );
    
    long    _lContentsVersion; // Use this to compare when we need to invalidate

    BOOL        _fSelectionVisible:1;           // Is Selection Visible ?
    BOOL        _fPendingInvalidate:1;          // Pending Invalidation.
    BOOL        _fPendingFireOM:1;              // Value for fFireOM on Pending Inval.
    BOOL        _fInUpdateSegment:1;            // Are we updating the segments.
    
#if DBG == 1
    CMarkup    * _pMarkup;
    long         _cchLast;     // Count of Chars in Selection
    void DumpSegments();
#endif    
};

#endif

