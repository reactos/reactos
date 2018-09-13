//+=================================================================
// File: Seltrack.hxx
//
//  Contents: CSelDragDropSrcInfo 
//
//  This object is used for OLE Drag Drop of a Selection.
//
// BUGBUG - this should live inside of mshtmled.dll, and is waiting for the
// saver to be exposed to allow us to do this. For now, it will live in Trident.
//
// All of the code that was in the layout - that had intimate knowledge of the selection,
// and this object is to be moved inside of this object.
//
//  History: 07-27-98 - marka - Created
//
//------------------------------------------------------------------
#ifndef _SELDRAG_HXX_
#define _SELDRAG_HXX_ 1

MtExtern(CSelDragDropSrcInfo)
MtExtern(CDropTargetInfo)

class CSelDragDropSrcInfo;
class CBaseBag; 

//
// This object is used to wing
// the cursor around, as you drag.
//

class CDropTargetInfo
{
public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CDropTargetInfo))
    
    CDropTargetInfo(CLayout * pLayout, CDoc* pDoc, POINT pt );

    ~CDropTargetInfo();
    
    void Init();
    
    void UpdateDragFeedback(CLayout * pLayout, POINT pt, CSelDragDropSrcInfo * pInfo, BOOL fPositionLastToTest=FALSE );

    void DrawDragFeedback();

    HRESULT Drop (  
                    CLayout*        pLayout, 
                    IDataObject *   pDataObj,
                    DWORD           grfKeyState,
                    POINT           ptScreen,
                    DWORD *         pdwEffect);

    HRESULT PasteDataObject( 
                    IDataObject * pDO,
                    IMarkupPointer* pInsertionPoint,
                    IMarkupPointer* pFinalStart,
                    IMarkupPointer* pFinalEnd );
private:

    BOOL EquivalentElements( IHTMLElement* pIElement1, IHTMLElement* pIElement2 );

    IMarkupPointer* _pPointer;
    IMarkupPointer* _pTestPointer;
    BOOL            _fPointerPositioned;
    BOOL            _fFurtherInStory;       // Are we moving further in the story ?
    BOOL            _fAtLogicalBOL;         // BOL
    BOOL            _fNotAtBOL;
    IHTMLCaret*   _pCaret;
    CDoc    *     _pDoc;    
};

class CSelDragDropSrcInfo : public CDragDropSrcInfo,
                            public ISegmentList,
                            public IDataObject,
                            public IDropSource
{
public:
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CSelDragDropSrcInfo))
    DECLARE_FORMS_STANDARD_IUNKNOWN(CSelDragDropSrcInfo);

    CSelDragDropSrcInfo(CDoc* pDoc, CElement* pElement );

    VOID Init(CElement* pElement);
    


    IMarkupPointer*           _pStart;

    IMarkupPointer*           _pEnd;

    SELECTION_TYPE            _eSelType;
    
    // BUGBUG - I'd like to make this IHTMLDocument2 based
    // so we can easily move it into mshtmled. However, any method
    // to do "doc" things eg get the current segment list - is likely to be based
    // on methods already existing in the Editor. So we'll just stub out those methods internally
    //
    CDoc*                   _pDoc; 
    
    CBaseBag *  _pBag;  // Data object for the selection being dragged

    BOOL IsInsideSelection(IMarkupPointer* pTestPointer );

    HRESULT IsValidDrop(IMarkupPointer* pPastePointer);
    
    BOOL IsInSameFlow() { return _fInSameFlow ; }
    
    HRESULT GetDataObjectAndDropSource ( IDataObject **  ppDO,
                                         IDropSource **  ppDS );

    HRESULT PostDragDelete();   

    HRESULT PostDragSelect();
    
    //---------------------------------        
    // ISegmentList methods
    //---------------------------------
    
    STDMETHOD ( MovePointersToSegment ) ( 
        int iSegmentIndex, 
        IMarkupPointer * pStart, 
        IMarkupPointer * pEnd ) ;

    STDMETHOD( GetSegmentCount ) (
        int* piSegmentCount,
        SELECTION_TYPE * peType );

    //---------------------------------------------------
    //
    // IDataObject
    //
    //--------------------------------------------------
    STDMETHODIMP DAdvise( FORMATETC FAR* pFormatetc,
            DWORD advf,
            LPADVISESINK pAdvSink,
            DWORD FAR* pdwConnection) ;

    STDMETHODIMP DUnadvise( DWORD dwConnection);

    STDMETHODIMP EnumDAdvise( LPENUMSTATDATA FAR* ppenumAdvise);

    STDMETHODIMP EnumFormatEtc(
                DWORD dwDirection,
                LPENUMFORMATETC FAR* ppenumFormatEtc);

    STDMETHODIMP GetCanonicalFormatEtc(
                LPFORMATETC pformatetc,
                LPFORMATETC pformatetcOut);

    STDMETHODIMP GetData(LPFORMATETC pformatetcIn, LPSTGMEDIUM pmedium );

    STDMETHODIMP GetDataHere(LPFORMATETC pformatetc, LPSTGMEDIUM pmedium);

    STDMETHODIMP QueryGetData(LPFORMATETC pformatetc );
    
    STDMETHODIMP SetData(LPFORMATETC pformatetc, STGMEDIUM FAR * pmedium, BOOL fRelease);

    //---------------------------------------------------
    //
    // IDropSource
    //
    //--------------------------------------------------
    
    STDMETHOD(QueryContinueDrag) (BOOL fEscapePressed, DWORD grfKeyState);
    STDMETHOD(GiveFeedback) (DWORD dwEffect);

private:
    ~CSelDragDropSrcInfo();
    HRESULT GetSegmentList(ISegmentList** ppSegmentList ) ;
    HRESULT GetMarkup( IMarkupServices** ppMarkup);

    HRESULT SetInSameFlow();
    
    BOOL _fInSameFlow; // Are the pointers in the same flow layout ?
};

#endif
