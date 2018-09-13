
//+---------------------------------------------------------------------------
//
//  Maintained by: alexa
//
//  Copyright: (C) Microsoft Corporation, 1994-1995.
//
//  File:       detail.hxx
//
//  Contents:   this file contains the CDataFrame class definition.
//
//----------------------------------------------------------------------------

#ifndef _DETAIL_HXX_
#   define _DETAIL_HXX_ 1

#   ifndef _BASEFRM_HXX_
#       include "basefrm.hxx"
#   endif

#   ifndef _DATAFRM_HXX_
#       include "datafrm.hxx"
#   endif

#   ifndef _BINDING_HXX_
#       include "binding.hxx"
#   endif

//----------------------------------------------------------------------------
//  F O R W A R D    C L A S S   D E C L A R A T I O N S
//----------------------------------------------------------------------------

#ifdef PRODUCT_97
class CMatrix;
#endif
class CDetailFrameTemplate;







//+---------------------------------------------------------------------------
//
//  Class:      CDetailFrame
//
//  Purpose:    Frame class holding controls or DataFrames.
//
//----------------------------------------------------------------------------

class CDetailFrame: public CBaseFrame//, public CBindSource
{
typedef CBaseFrame super;

friend class CRepeaterFrame;
friend class CDataFrame;

public:
    //+-----------------------------------------------------------------------
    //  Template construction
    //------------------------------------------------------------------------

    // Template constructor.
    CDetailFrame(CDoc * pDoc, CSite * pParent);

    // allocate size and memcpy it from original to create clone
    // operator new for the instance construction.
    void * operator new(size_t cb) { return MemAllocClear(cb); }

    // Template initialization (inheretedd from CBaseFrame).
    // HRESULT Init ();

    // Template/instance detach.
    void Detach ();

    virtual void PaintSelectionFeedback(CFormDrawInfo *pDI, RECT *prc, DWORD dwSelInfo);
    virtual HRESULT BUGCALL HandleMessage(CMessage *pMessage, CSite *pChild);

    //+-----------------------------------------------------------------------
    //  (Public) Instance creation API
    //------------------------------------------------------------------------

    // We have to override pure virtual for PULL model creation.
    HRESULT CreateInstance (CDoc * pDoc,
                            CSite * pParent,
                            CSite **ppFrame,
                            CCreateInfo * pcinfo)
                            { Assert (FALSE); RRETURN (E_FAIL); }

    // The following create instance is not virtual and is intruduced for the
    // PUSH model creation (the HROW is pushed to the instance).
    HRESULT CreateInstance (CDoc * pDoc,
                            CSite * pParent,
                            OUT CBaseFrame **ppFrame,
                            HROW hrow);

    virtual HRESULT UpdatePosRect1 ();

    virtual HRESULT ScrollBy(
                        long dxl,
                        long dyl,
                        fmScrollAction xAction,
                        fmScrollAction yAction);

    //+-----------------------------------------------------------------------
    //  Generic functions
    //------------------------------------------------------------------------

    //  Core methods overridden
    virtual HRESULT Notify(SITE_NOTIFICATION, DWORD);
    virtual HRESULT DrawBackground(CFormDrawInfo *pDI);
    virtual HRESULT Draw(CFormDrawInfo *);
    virtual void GetClientRectl(RECTL *prcl);
    virtual HTC HitTestPointl(
        POINTL ptl,
        RECTL * prclClip,
        CSite ** ppSite,
        CMessage *pMessage,
        DWORD dwFlags);

    virtual void OptimizeBackgroundPainting(void);

    // get the template.
    inline CDetailFrameTemplate * getTemplate()
        {
            return (CDetailFrameTemplate *) _pTemplate;
        }

    // owner of this frame
    CDataFrame * getOwner()     { return ((CBaseFrame*)_pParent)->getOwner(); }

    // Check if detail frame is repeated vertically.
    inline BOOL IsVertical ()   { return getOwner()->IsVertical(); }

    virtual BOOL IsVisible(void);      // TRUE if this control is visible

    // property updating api
    virtual HRESULT UpdatePropertyChanges(UPDATEPROPS updFlag=UpdatePropsPrepareTemplates);



    HRESULT AddToSites(CSite * pSite, DWORD dwOperations);
    HRESULT DeleteSite(CSite * pSite, DWORD dwFlags);


    #ifdef PRODUCT_97
    virtual HRESULT SetProposed (CSite * pSite, const CRectl * prcl);
    virtual HRESULT GetProposed (CSite * pSite, CRectl * prcl);
    virtual HRESULT CalcControlPositions(DWORD dwFlags);
    virtual void DrawFeedbackRect (HDC hDC);
    void DirtyMatrix();
    HRESULT EnsureMatrix();
    virtual HRESULT Move (RECTL *prcl, DWORD dwFlags = 0);
    #endif


    virtual HRESULT CalcProposedPositions ();

    virtual HRESULT MoveToProposed (DWORD dwFlags);

    #if defined(PRODUCT_97)
    virtual HRESULT ProposedFriendFrames(void);
    #endif
    virtual HRESULT ProposedDelta(CRectl *rclDelta);


    //
    //  recycling api
    //
    int  MoveToRecycle(CBaseFrame *pdfr);
    void EmptyRecycle();

    #if DBG==1
    ULONG   _ulRecycleCounter;
    ULONG   _ulRecycleCreation;
    #endif


    ////////////////////////////////////////////////////////
    // A couple of overloaded XObj properties, they get
    //  redirected to the dataframe holding the detail
    //      !! this has to be done for header/footer as well
    ////////////////////////////////////////////////////////


    STDMETHOD(GetTabIndex) (short * TabIndex) { return getOwner()->GetTabIndex(TabIndex); }
    STDMETHOD(SetTabIndex) (short TabIndex) { return getOwner()->SetTabIndex(TabIndex); }
    STDMETHOD(GetTabStop) (VARIANT_BOOL * pfTabStop) { return getOwner()->GetTabStop(pfTabStop); }
    STDMETHOD(SetTabStop) (VARIANT_BOOL fTabStop) { return getOwner()->SetTabStop(fTabStop); }
    STDMETHOD(GetVisible) (VARIANT_BOOL * pfVisible) { return getOwner()->GetVisible(pfVisible); }
    STDMETHOD(SetVisible) (VARIANT_BOOL fVisible) { return getOwner()->SetVisible(fVisible); }

    //  DetailFrame property overrides


    //+-----------------------------------------
    // CBindSource methods API
    //------------------------------------------

    // note: look at the comment on _hrow.
    virtual HRESULT GetData (CDataLayerAccessor * dla, LPVOID lpv);
    virtual HRESULT SetData (CDataLayerAccessor * dla, LPVOID lpv);

    //
    // positioning API
    //

    virtual HRESULT MoveToRow(HROW hrow, DWORD dwFlags=0);
    HRESULT RefreshData(void);          // used to refresh controls
    virtual HRESULT DeleteRow(DWORD dwFlags=0) { return E_NOTIMPL; }
    virtual HROW    getHRow(void) { return _hrow; }


    //+--------------------------------
    //  Drag & Drop
    //---------------------------------

    #if 0 //ndef PRODUCT_97

    virtual HRESULT DragEnter(IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
    virtual HRESULT DragOver(DWORD grfKeyState, POINTL pt, DWORD * pdwEffect);
    virtual HRESULT Drop(IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
    virtual HRESULT DragLeave();
    virtual void    DragHide();
    virtual void DrawDragFeedback(void);

    #endif


    //+--------------------------------
    //  DataDoc selection tree builder methods
    //---------------------------------

    virtual HRESULT SelectSite(CSite * pSite, DWORD dwFlags);
    virtual HRESULT SetSelected(BOOL fSelected);

    void SetSelectionState (CSite * pSite);
    HRESULT SelectSubtree(CSite * pSite, DWORD dwFlags);
    HRESULT AddToSelectionTree(DWORD dwFlags);
    HRESULT DeselectSubtree(CSite * pSite, DWORD dwFlags);
    HRESULT RemoveFromSelectionTree(DWORD dwFlags);

    void SetSelectionElement(SelectUnit * pSelectUnit)
        {
            _pSelectionElement = pSelectUnit;
        }
    SelectUnit * GetSelectionElement(void)
        {
            return _pSelectionElement;
        }

    //  Return the qualifier / bookmark of the layout's row
    virtual HRESULT GetQualifier (QUALIFIER * q);
    virtual HRESULT GetRecordNumber (ULONG *pulRecordNumber);
    virtual BOOL    EnsureHRow ();


    //+-----------------------------------------
    // Listbox helper
    //------------------------------------------

    void SuppressControlBorders(BOOL fSuppress);

    //+-----------------------------------------------------------------------
    //
    //  CTBag (template bag)
    //
    //------------------------------------------------------------------------

    class CDetailFrameTBag : public CBaseFrame::CBaseFrameTBag
    {
    public:
        #ifdef PRODUCT_97
        CDetailFrameTBag() { Assert (_pMatrix == NULL); }
        #endif

         // CDetailFrame::Detach should release the matrix
        ~CDetailFrameTBag() { }

        void * operator new(size_t cb) { return MemAllocClear(cb); }
        #ifdef PRODUCT_97
        CMatrix *       _pMatrix;
        #endif
    };

    typedef CDetailFrameTBag CTBag;

    inline CTBag * TBag(); // { return (CTBag *)GetTBag(); }

protected:

    //+-----------------------------------------------------------------------
    //  (Protected) Instance creation API
    //------------------------------------------------------------------------

    // Instance constructor.
    CDetailFrame(CDoc *pDoc, CSite * pParent, CDetailFrame * pTemplate);

    // operator new for the template construction.
    void * operator new (size_t s, CDetailFrame * pOriginal);

        // Creation of the controls (all the subframes).
    HRESULT BuildInstance (CDetailFrame * pNewInstance);


    //+-----------------------------------------
    // OLE DB related member data
    //------------------------------------------

    HROW            _hrow;  // Layout instance current row.
                            // note: thus field is useless on the template,
                            // but this way we can reuse the code for the footer
                            // without introducing a special CDetailFarameXBag;

};


/*   BUGBUG rgardner removed CControls
class CDetailFrameControls : public CControls
{
    DECLARE_FORMS_SUBOBJECT_IUNKNOWN(CDetailFrameTemplate)

protected:

    virtual CSite *   ParentSite(void);
}; */


class CDetailFrameTemplate : public CDetailFrame
{
typedef CDetailFrame super;

friend class CDetailFrameControls;
friend class CDetailFrame;

public:
    CDetailFrameTemplate(CDoc * pDoc, CSite * pParent) : super(pDoc, pParent)
        {
            _fPaintBackground = 1;
        }

    virtual CSite::CTBag * GetTBag() { return &_TBag; }

    virtual CBase::CLASSDESC *GetClassDesc() const { return &s_classdesc;}

    virtual CPtrAry<CSite *> *  SitesCollection();

protected:

    CTBag _TBag;

    CPtrAry<CSite *>        _arySitesCollection;
    //BUGBUG rgardner remove Controls
    //CDetailFrameControls    _Controls;

    static PROP_DESC    s_apropdesc[];
    static CLASSDESC   s_classdesc;

};


inline CPtrAry<CSite *> *
CDetailFrameTemplate::SitesCollection( )
{
    return &_arySitesCollection;
}



class CDetailFrameInstance : public CDetailFrame
{
typedef CDetailFrame super;

friend class CRepeaterFrame;
friend class CDetailFrame;

public:
    virtual CSite::CTBag * GetTBag() { return getTemplate()->GetTBag(); }
    virtual CBase::CLASSDESC *GetClassDesc() const { return &s_classdesc;}

protected:

    // Cloning Constructor of CBaseFrame from the CBaseFrameTemplate.
    CDetailFrameInstance(CDoc * pDoc, CSite * pParent, CDetailFrame * pTemplate);

    ~CDetailFrameInstance ();

    //+-----------------------------------------
    // Selection related member data
    //------------------------------------------

    //  @todo: to be moved back here with virtual accessors

    //SelectUnit     *_pSelectionElement;
    //CArySelector   *_pDummySelectorList;

    static CLASSDESC   s_classdesc;
};


inline CDetailFrame::CTBag * CDetailFrame::TBag()
{
    Assert((CTBag *)GetTBag() == &((CDetailFrameTemplate *)_pTemplate)->_TBag);
    return &((CDetailFrameTemplate *)_pTemplate)->_TBag;
}

#endif _DETAIL_HXX
