
//+---------------------------------------------------------------------------
//
//  Maintained by: alexa
//
//  Copyright: (C) Microsoft Corporation, 1994-1995.
//
//  File:       datafrm.hxx
//
//  Contents:   this file contains the CDataFrame class definition.
//
//----------------------------------------------------------------------------

#ifndef _DATAFRM_HXX_
#   define _DATAFRM_HXX_ 1

#   ifndef _BASEFRM_HXX_
#       include "basefrm.hxx"
#   endif

#   ifndef _BINDING_HXX_
#       include "binding.hxx"
#   endif

#include    "propchg.hxx"

#ifndef INOUT
#   define INOUT
#endif

#define _hxx_
#include "datafrm.hdl"

//----------------------------------------------------------------------------
//  F O R W A R D    C L A S S   D E C L A R A T I O N S
//----------------------------------------------------------------------------

class CDataFrameTemplate;
class COleDataSite;
class CRootDataFrame;
//--------------------------------------------------------------------------//
// Constants                                                                //
//--------------------------------------------------------------------------//

const unsigned int NO_LIMIT         = (unsigned int) -1;
const unsigned int UNLIMITED        = NO_LIMIT;
const unsigned int CURRENT_LAYOUT   = FRAME_NOTFOUND;

const int ACROSS      = edgeHorizontal;
const int DOWN        = edgeVertical;
const int ACROSS_DOWN = ACROSS + 2;
const int DOWN_ACROSS = DOWN + 2;


const int AT_INPLACE = 0;
const int AT_BEGINNING = 1;
const int AT_END = 2;

#define VERTICAL(dir)   ( 0 == (dir & 0x2))
#define HORIZONTAL(dir) ( 0 != (dir & 0x2))

const int DIRECTION     = 0x1;
const int ORIENTATION   = 0x2;

#define RECORD_SELECTOR_SIZE 13
#define RECORD_SELECTOR_CLEARANCE 2

enum En_FetchDirection
{
    en_FetchPrev = -1,      // essentially this is only offset stuff
    en_FetchNext = 1
};

//--------------------------------------------------------------------------//
// Typedef section.                                                         //
//--------------------------------------------------------------------------//

enum ESnaking
{
    SNK_Across     = ACROSS,
    SNK_Down       = DOWN,
    SNK_AcrossDown = ACROSS_DOWN,
    SNK_DownAcross = DOWN_ACROSS
};


enum EScrollBars
{
    NOSCROLLBARS = 0,
    HORSCROLLBAR = 1,
    VERSCROLLBAR = 2,
    BOTHSCROLLBARS = 3
};

//+---------------------------------------------------------------------------
//
//  Class:      CDataFrame
//
//  Purpose:    Frame class holding caption/header/detail/footer/VCR/scrollers
//
//----------------------------------------------------------------------------

class CDataFrame: public CBaseFrame, public IDataFrameExpert
{
typedef CBaseFrame super;

friend class CRepeaterFrame;
friend class CDetailFrame;
friend class CHeaderFrame;

public:
    //
    // IUnknown
    //

    STDMETHOD(PrivateQueryInterface)(REFIID riid, void ** ppv);
    DECLARE_FORMS_DELEGATING_IUNKNOWN(CDataFrame)

    //
    // Reference counting
    //

    ULONG SubAddRef()       { return CBase::SubAddRef(); }
    ULONG SubRelease()      { return CBase::SubRelease();}
    ULONG GetObjectRefs()   { return CBase::GetObjectRefs(); }
    ULONG GetRefs()         { return CBase::GetRefs();   }

    //+-----------------------------------------------------------------------
    //  Constructors, Destructors, Creation API
    //------------------------------------------------------------------------

    CDataFrame(CDoc * pDoc, CSite * pParent);

    HRESULT InitNew();
    HRESULT InitSubFrameTemplate(CBaseFrame **pNewTemplate, SITEDESC_FLAG sf, BOOL fFooter=FALSE);
    virtual HRESULT InitSubFrameInstance(CBaseFrame **pNewInstance, CBaseFrame *pTemplate, CCreateInfo * pcinfo);

    void * operator new(size_t cb) { return MemAllocClear(cb); }

    void Detach ();

    inline CDataFrameTemplate * getTemplate()
    {
        return (CDataFrameTemplate *)_pTemplate;
    }

    inline CBaseFrame *getDetail() { Assert(_pDetail); return _pDetail; }
    inline CBaseFrame *getHeader() { return _pHeader; }
    #if defined(PRODUCT_97)
    inline CBaseFrame *getFooter() { return _pFooter; }
    #endif

    virtual CDataFrame * getOwner() { return this; }

    virtual HRESULT Draw(CFormDrawInfo *pDI);
    virtual HRESULT DrawBackground(CFormDrawInfo *pDI);
    virtual HRESULT BUGCALL HandleMessage(CMessage *pMessage, CSite *pChild);
    STDMETHOD(OnUIActivate) (void);
    STDMETHOD(OnUIDeactivate) (BOOL fUndoable);


    //+-----------------------------------------------------------------------

    // Template functionality (Instance Creation/Population API)
    //------------------------------------------------------------------------

    // Creation methods:  (one of the main purposes of the template is to
    // create an instances).

    HRESULT CreateInstance (CDoc * pDoc,
                               CSite * pParent,
                               CSite **ppFrame,
                               CCreateInfo * pcinfo);

    HRESULT BuildInstance (CDataFrame * pNewInstance, CCreateInfo * pcinfo);
    virtual HRESULT BuildDetail (CDataFrame * pNewInstance, CCreateInfo * pcinfo);

    virtual HRESULT Generate (IN CRectl& boundRect);

public:
    //
    //  IDataFrameExpert methods for use by the Morphable Data Control
    //

    //  The following methods are there for ListBox creation/maintainance

    STDMETHOD(CreateListbox)(IUnknown       *pRowSet,
                             OLE_COLOR      olecolorBack,
                             OLE_COLOR      olecolorFore,
                             IDispatch      *pFontObject,
                             fmListBoxStyles listStyle,
                             fmScrollBars   scrollbarStyle,
                             int            iColumnCount,
                             int            iListRows,
                             VARIANT_BOOL   fIntegralHeight,
                             VARIANT_BOOL   fColumnHeads,
                             int           *piColumnWidths,
                             int            c,
                             DWORD          dwCookie);

    STDMETHOD(SetControlProperty)(DISPID dispid, VARIANT *pVariant);
    STDMETHOD(ComputeExtent) (int   *piColumnWidths,
                              int   cb,
                              void  *  pTransform,    // BUGBUG - Type is CTransform
                              SIZEL *pszlProposed,
                              SIZEL *pszlComputed);

protected:
    // listbox helper methods
    HRESULT SetColumnHeads( VARIANT_BOOL fColumnHeads);
    HRESULT SetupVisuals( OLE_COLOR olecolorBack,  OLE_COLOR olecolorFore, DWORD dwCookie);
    HRESULT SetupRowset( IUnknown *pRowSet);
    HRESULT CreateDetailFromCursor( int iColumnCount,  int *piColumnWidths,  int cb);
    HRESULT CreateHeaderFromCursor(void);
    HRESULT CalculateColumnSize (CTransform * pTransform,
                                    long lRowWidth,
                                    long *plCalculatedWidth,
                                    int  *piColumnWidths,
                                    int c);


    HRESULT CalcSizeFromProperties(OUT CRectl   *prclDataFrame,
                                   IN  CRectl   *pszlHeader,
                                   IN  CSizel   *pszlDetailTemplate,
                                   IN  long     lMaxRows,
                                   IN  long     lMinRows);

    HRESULT CalcPropertiesFromSize (IN  CRectl  *prclDataFrame,
                                    IN  CRectl  *prclHeader,
                                    IN  CSizel  *pszlDetailTemplate,
                                    OUT long *pMaxRows,
                                    OUT long *pMinRows);

#ifdef PRODUCT_97

    void CalcProposedDetailSection (INOUT CRectl& rclDetailSection);
    void AdjustProposed (CRectl& rclPropose, CRectl& rclDetailSection);

#endif // PRODUCT_97

    void SubScrollbars (CRectl* prcl, unsigned fScrollbars);
    void SetProposedDetailSection ();

    HRESULT ComputeNaturalExtent(COleDataSite *pods, long * plHeightMax);

    virtual HRESULT GetNaturalExtent(DWORD dwAspect, LONG lindex,
        DVTARGETDEVICE * ptd, HDC hicTargetDev,
        DVEXTENTINFO * pExtentInfo, LPSIZEL psizel);

public:
    // zooming helper methods

    HRESULT AlignTemplatesToPixels(CTransform * pTransform);


public:

    virtual HRESULT SetProposed (CSite * pSite, const CRectl * prcl);
    virtual HRESULT GetProposed (CSite * pSite, CRectl * prcl);
    virtual HRESULT CalcProposedPositions ();
    virtual HRESULT MoveToProposed(DWORD dwFlags);

    HRESULT OnListBoxStyleChange(fmListBoxStyles eNewListBoxStyle);
    void RetireRecordSelector(void);

    // Populate the rectangle with layout instances. virtual
    virtual HRESULT CreateToFit (IN const CRectl * prclView, IN DWORD dwFlags);

    #if defined(PRODUCT_97)
    virtual void UpdateObjectRects(RECT *prcClip);
    #endif

public:
    //+-----------------------------------------------------------------------
    // OLE DB related API
    //------------------------------------------------------------------------

#if defined(PRODUCT_97)
    HRESULT SetRowsetSource();
#endif // defined(PRODUCT_97)

    // databinding hooks overriden from CSite
    virtual HRESULT RefreshData();
    virtual void DataSourceChanged();

    // Get DataLayer Cursor, returns non-refcounted cursor for now.
    HRESULT LazyGetDataLayerCursor(CDataLayerCursor **ppDataLayerCursor);

    // Get DataLayerAccessorSource, returns non-refcounted DLAS.
    HRESULT LazyGetDLAccessorSource(CDataLayerAccessorSource **ppDLASOut);

    BOOL HasDLAccessorSource() { return TBag()->_dl.HasDLAS(); }

    HRESULT GetNextRows (OUT HROW *pRows,
                         IN  ULONG ulFetch,
                         OUT ULONG *pulFetched,
                         En_FetchDirection iFetch,
                         HROW hRowCurrent);

    HRESULT GetRowAt (OUT HROW *pHRow, ULONG ulPosition);

    HRESULT GetFirstRow (HROW *pRow);

    HRESULT GetLastRow (HROW *pRow);

    HRESULT AddRefHRow(HROW *pHRow);

    // Check if hrow is still valid and change it to the next if not
    HRESULT FixupRow(CDetailFrame * pRow);

    #if DBG==1
    virtual ELEMENT_TAG SiteTypeFromCLSID(REFCLSID clsid);
    #endif

    virtual HRESULT InsertNewSite(
            REFCLSID clsid,
            LPCTSTR pstrName,
            const RECTL * prcl,
            DWORD dwOperations,
            CSite ** ppSite);

    virtual HRESULT InsertedAt(
            CSite * pParent,
            REFCLSID clsid,
            LPCTSTR pstrName,
            const RECTL * prcl,
            DWORD dwOperations);

    virtual HRESULT DragOver(DWORD grfKeyState, POINTL pt, DWORD * pdwEffect);

    BOOL    IsPrimaryDirection(NAVIGATE_DIRECTION Direction);

    HRESULT RemoveRelated(CSite *pSite, DWORD dwFlags);

    HRESULT GetNumberOfRecords (ULONG * pulRows);

    //+-----------------------------------------------------------------------
    //  DataDoc selection tree builder methods
    //------------------------------------------------------------------------

    virtual HRESULT SelectSite(CSite * pSite, DWORD dwFlags);

    void SetSelectionState (CDetailFrame *pNewInstance);
    HRESULT SelectSubtree(SelectUnit * pSelector, DWORD dwFlags);
    HRESULT AddToSelectionTree(DWORD dwFlags);
    HRESULT DeselectSubtree(SelectUnit * pSelector, DWORD dwFlags);
    HRESULT RemoveFromSelectionTree(DWORD dwFlags);

    SelectUnit * getAnchor()
    {
        return &TBag()->_suAnchor;
    }
    BOOL IsAnchored()
    {
        return getAnchor()->qStart.IsValid();
    }

    inline CArySelector * GetSelectedQualifiers(void)
        {
            return _parySelectedQualifiers;
        }
    inline void SetSelectedQualifiers(CArySelector * p)
        {
            _parySelectedQualifiers = p;
        }

    HRESULT SetAnchorSite(CSite * pSite);

    HRESULT SetCurrent (CBaseFrame *plfr, DWORD dwFlags = 0);

    virtual HRESULT UpdatePropertyChanges(UPDATEPROPS updFlag=UpdatePropsPrepareTemplates);

    //+-----------------------------------------------------------------------
    //  IDataFrame, generated property methods
    //------------------------------------------------------------------------

    NV_STDMETHOD(GetMouseIcon) (IDispatch **ppDisp);
    NV_STDMETHOD(SetMouseIcon) (IDispatch *pDisp);
    NV_STDMETHOD(GetMousePointer) (fmMousePointer * pMousePointer);
    NV_STDMETHOD(SetMousePointer) (fmMousePointer MousePointer);
    NV_STDMETHOD(GetShow3D) (int * piShow3D);
    NV_STDMETHOD(SetShow3D) (int iShow3D);
    NV_STDMETHOD(GetEnabled)(VARIANT_BOOL *);
    NV_STDMETHOD(SetEnabled)(VARIANT_BOOL);
    NV_STDMETHOD(GetDeferedPropertyUpdate) (VARIANT_BOOL *);
    NV_STDMETHOD(SetDeferedPropertyUpdate) (VARIANT_BOOL);
    NV_STDMETHOD(GetShowHeaders) (VARIANT_BOOL *pfShowHeaders);
    NV_STDMETHOD(SetShowHeaders) (VARIANT_BOOL fShowHeaders);
    NV_STDMETHOD(SetListBoxStyle) (fmListBoxStyles eListBoxStyle);
    NV_STDMETHOD(GetListBoxStyle) (fmListBoxStyles * peListBoxStyle);
    NV_STDMETHOD(SetMultiSelect) (fmMultiSelect eMultiSelect);
    NV_STDMETHOD(GetMultiSelect) (fmMultiSelect * peMultiSelect);
    NV_STDMETHOD(SetListStyle) (fmListStyle eListStyle);
    NV_STDMETHOD(GetListStyle) (fmListStyle * peListStyle);
    NV_STDMETHOD(GetAutosizeStyle) (fmEnAutoSize *enAutoSize);
    NV_STDMETHOD(SetAutosizeStyle) (fmEnAutoSize enAutoSize);
    NV_STDMETHOD(GetListIndex) (long * plListIndex);
    NV_STDMETHOD(SetListIndex) (long    lListIndex);
    NV_STDMETHOD(GetListItemSelected) (long lItemIndex, VARIANT_BOOL *pfSelected);
    NV_STDMETHOD(SetListItemSelected) (long lItemIndex, VARIANT_BOOL fSelected);
    NV_STDMETHOD(GetTopIndex) (long * plTopIndex);
    NV_STDMETHOD(SetTopIndex) (long    lTopIndex);
    NV_STDMETHOD(GetVisibleCount) (long * pVisibleCount);
    NV_STDMETHOD(GetMinRows) (long * plMinRows);
    NV_STDMETHOD(SetMinRows) (long lMinRows);
    NV_STDMETHOD(GetMaxRows) (long * plMaxRows);
    NV_STDMETHOD(SetMaxRows) (long lMaxRows);
    NV_STDMETHOD(GetScrollBars) (fmScrollBars * piScrollBars);
    NV_STDMETHOD(SetScrollBars) (fmScrollBars iScrollBars);
    NV_STDMETHOD(SetRowSource) (LPTSTR lpwstrRowSource);
    NV_STDMETHOD(GetRowSource) (BSTR * pbstrRowSource);
#ifdef PRODUCT_97
    //#if DBG==1
    NV_STDMETHOD(SetRunTestcode) (int iTestcode);
    //#endif
    NV_STDMETHOD(GetDetail) (IDataFrame ** ppDetail);
    NV_STDMETHOD(GetDatabase) (BSTR * pbstrDatabase);
    NV_STDMETHOD(SetDatabase) (LPTSTR lpwstrDatabase);
    NV_STDMETHOD(GetActiveInstance) (IDispatch ** ppdispActiveInstance);
    NV_STDMETHOD(GetAllowAdditions) (VARIANT_BOOL * pfAllowAdditions);
    NV_STDMETHOD(SetAllowAdditions) (VARIANT_BOOL fAllowAdditions);
    NV_STDMETHOD(GetAllowDeletions) (VARIANT_BOOL * pfAllowDeletions);
    NV_STDMETHOD(SetAllowDeletions) (VARIANT_BOOL fAllowDeletions);
    NV_STDMETHOD(GetAllowEditing) (VARIANT_BOOL * pfAllowEditing);
    NV_STDMETHOD(SetAllowEditing) (VARIANT_BOOL fAllowEditing);
    NV_STDMETHOD(GetAllowFilters) (VARIANT_BOOL * pfAllowFilters);
    NV_STDMETHOD(SetAllowFilters) (VARIANT_BOOL fAllowFilters);
    NV_STDMETHOD(GetAllowUpdating) (VARIANT_BOOL * piAllowUpdating);
    NV_STDMETHOD(SetAllowUpdating) (VARIANT_BOOL iAllowUpdating);
    NV_STDMETHOD(GetAlternateBackColor) (OLE_COLOR * pcolorAlternateBackColor);
    NV_STDMETHOD(SetAlternateBackColor) (OLE_COLOR colorAlternateBackColor);
    NV_STDMETHOD(GetAlternateInterval) (int * piAlternateInterval);
    NV_STDMETHOD(SetAlternateInterval) (int iAlternateInterval);
    NV_STDMETHOD(GetAutoEject) (VARIANT_BOOL * pfAutoEject);
    NV_STDMETHOD(SetAutoEject) (VARIANT_BOOL fAutoEject);
    NV_STDMETHOD(GetColumnSpacing) (int * piColumnSpacing);
    NV_STDMETHOD(SetColumnSpacing) (int iColumnSpacing);
    NV_STDMETHOD(GetRowSpacing) (int * piRowSpacing);
    NV_STDMETHOD(SetRowSpacing) (int iRowSpacing);
    NV_STDMETHOD(GetCommitSync) (int * piCommitSync);
    NV_STDMETHOD(SetCommitSync) (int iCommitSync);
    NV_STDMETHOD(GetCommitWhat) (int * piCommitWhat);
    NV_STDMETHOD(SetCommitWhat) (int iCommitWhat);
    NV_STDMETHOD(GetCommitWhen) (int * piCommitWhen);
    NV_STDMETHOD(SetCommitWhen) (int iCommitWhen);
    NV_STDMETHOD(GetContinousForm) (VARIANT_BOOL * pfContinousForm);
    NV_STDMETHOD(SetContinousForm) (VARIANT_BOOL fContinousForm);
    NV_STDMETHOD(GetDataOnly) (VARIANT_BOOL * pfDataOnly);
    NV_STDMETHOD(SetDataOnly) (VARIANT_BOOL fDataOnly);
    NV_STDMETHOD(GetDateGrouping) (int * piDateGrouping);
    NV_STDMETHOD(SetDateGrouping) (int iDateGrouping);
    NV_STDMETHOD(GetDefaultEditing) (int * piDefaultEditing);
    NV_STDMETHOD(SetDefaultEditing) (int iDefaultEditing);
    NV_STDMETHOD(GetFooter) (IDataFrame ** ppDataFrame);
    NV_STDMETHOD(GetHeader) (IDataFrame ** ppDataFrame);
    NV_STDMETHOD(GetDirty) (VARIANT_BOOL * pfDirty);
    NV_STDMETHOD(SetDirty) (VARIANT_BOOL fDirty);
    NV_STDMETHOD(GetDirtyDataColor) (OLE_COLOR * pcolorDirtyDataColor);
    NV_STDMETHOD(SetDirtyDataColor) (OLE_COLOR colorDirtyDataColor);
    NV_STDMETHOD(GetDirtyPencil) (VARIANT_BOOL * pfDirtyPencil);
    NV_STDMETHOD(SetDirtyPencil) (VARIANT_BOOL fDirtyPencil);
    NV_STDMETHOD(GetDisplayWhen) (int * piDisplayWhen);
    NV_STDMETHOD(SetDisplayWhen) (int iDisplayWhen);
    NV_STDMETHOD(GetFooterText) (BSTR * pbstrFooter);
    NV_STDMETHOD(SetFooterText) (LPTSTR bstrFooter);
    NV_STDMETHOD(GetForceNewPage) (int * piForceNewPage);
    NV_STDMETHOD(SetForceNewPage) (int iForceNewPage);
    NV_STDMETHOD(GetFormatCount) (long * plFormatCount);
    NV_STDMETHOD(SetFormatCount) (long lFormatCount);
    NV_STDMETHOD(GetFullIntervals) (VARIANT_BOOL * pfFullIntervals);
    NV_STDMETHOD(SetFullIntervals) (VARIANT_BOOL fFullIntervals);
    NV_STDMETHOD(GetGroupInterval) (int * piGroupInterval);
    NV_STDMETHOD(SetGroupInterval) (int iGroupInterval);
    NV_STDMETHOD(GetGroupOn) (int * piGroupOn);
    NV_STDMETHOD(SetGroupOn) (int iGroupOn);
    NV_STDMETHOD(GetHeaderText) (BSTR * pbstrHeader);
    NV_STDMETHOD(SetHeaderText) (LPTSTR bstrHeader);
    NV_STDMETHOD(GetHideDuplicates) (VARIANT_BOOL * pfHideDuplicates);
    NV_STDMETHOD(SetHideDuplicates) (VARIANT_BOOL fHideDuplicates);
    NV_STDMETHOD(GetItemsAcross) (int * piItemsAcross);
    NV_STDMETHOD(SetItemsAcross) (int iItemsAcross);
    NV_STDMETHOD(GetKeepTogether) (int * piKeepTogether);
    NV_STDMETHOD(SetKeepTogether) (int iKeepTogether);
    NV_STDMETHOD(GetLayoutForPrint) (int * piLayoutForPrint);
    NV_STDMETHOD(SetLayoutForPrint) (int iLayoutForPrint);
    NV_STDMETHOD(GetLockRecord) (VARIANT_BOOL * pfLockRecord);
    NV_STDMETHOD(SetLockRecord) (VARIANT_BOOL fLockRecord);
    NV_STDMETHOD(GetMoveLayout) (VARIANT_BOOL * pfMoveLayout);
    NV_STDMETHOD(SetMoveLayout) (VARIANT_BOOL fMoveLayout);
    NV_STDMETHOD(GetNavigationButtons) (VARIANT_BOOL * pfNavigationButtons);
    NV_STDMETHOD(SetNavigationButtons) (VARIANT_BOOL fNavigationButtons);
    NV_STDMETHOD(GetNewRowOrCol) (int * piNewRowOrCol);
    NV_STDMETHOD(SetNewRowOrCol) (int iNewRowOrCol);
    NV_STDMETHOD(GetNewStar) (VARIANT_BOOL * pfNewStar);
    NV_STDMETHOD(SetNewStar) (VARIANT_BOOL fNewStar);
    NV_STDMETHOD(GetNextRecord) (VARIANT_BOOL * pfNextRecord);
    NV_STDMETHOD(SetNextRecord) (VARIANT_BOOL fNextRecord);
    NV_STDMETHOD(GetNormalDataColor) (OLE_COLOR * pcolorNormalDataColor);
    NV_STDMETHOD(SetNormalDataColor) (OLE_COLOR colorNormalDataColor);
    NV_STDMETHOD(GetOneFooterPerPage) (VARIANT_BOOL * pfOneFooterPerPage);
    NV_STDMETHOD(SetOneFooterPerPage) (VARIANT_BOOL fOneFooterPerPage);
    NV_STDMETHOD(GetOneHeaderPerPage) (VARIANT_BOOL * pfOneHeaderPerPage);
    NV_STDMETHOD(SetOneHeaderPerPage) (VARIANT_BOOL fOneHeaderPerPage);
    NV_STDMETHOD(GetOpenArgs) (BSTR * pbstrOpenArgs);
    NV_STDMETHOD(SetOpenArgs) (LPTSTR bstrOpenArgs);
    NV_STDMETHOD(GetOutline) (int * piOutline);
    NV_STDMETHOD(SetOutline) (int iOutline);
    NV_STDMETHOD(GetOutlineCollapseIcon) (IDispatch * * phOutlineCollapseIcon);
    NV_STDMETHOD(SetOutlineCollapseIcon) (IDispatch * hOutlineCollapseIcon);
    NV_STDMETHOD(GetOutlineExpandIcon) (IDispatch * * phOutlineExpandIcon);
    NV_STDMETHOD(SetOutlineExpandIcon) (IDispatch * hOutlineExpandIcon);
    NV_STDMETHOD(GetOutlineLeafIcon) (IDispatch * * phOutlineLeafIcon);
    NV_STDMETHOD(SetOutlineLeafIcon) (IDispatch * hOutlineLeafIcon);
    NV_STDMETHOD(GetOutlinePrint) (int * piOutlinePrint);
    NV_STDMETHOD(SetOutlinePrint) (int iOutlinePrint);
    NV_STDMETHOD(GetOutlineShowWhen) (int * piOutlineShowWhen);
    NV_STDMETHOD(SetOutlineShowWhen) (int iOutlineShowWhen);
    NV_STDMETHOD(GetParentPosition) (int * piParentPosition);
    NV_STDMETHOD(SetParentPosition) (int iParentPosition);
    NV_STDMETHOD(GetPrintCount) (long * plPrintCount);
    NV_STDMETHOD(SetPrintCount) (long lPrintCount);
    NV_STDMETHOD(GetPrintSection) (VARIANT_BOOL * pfPrintSection);
    NV_STDMETHOD(SetPrintSection) (VARIANT_BOOL fPrintSection);
    NV_STDMETHOD(GetQBFAutoExecute) (VARIANT_BOOL * pfQBFAutoExecute);
    NV_STDMETHOD(SetQBFAutoExecute) (VARIANT_BOOL fQBFAutoExecute);
    NV_STDMETHOD(GetQBFAvailable) (VARIANT_BOOL * pfQBFAvailable);
    NV_STDMETHOD(SetQBFAvailable) (VARIANT_BOOL fQBFAvailable);
    NV_STDMETHOD(GetQBFMode) (VARIANT_BOOL * pfQBFMode);
    NV_STDMETHOD(SetQBFMode) (VARIANT_BOOL fQBFMode);
    NV_STDMETHOD(GetQBFShowData) (VARIANT_BOOL * pfQBFShowData);
    NV_STDMETHOD(SetQBFShowData) (VARIANT_BOOL fQBFShowData);
    NV_STDMETHOD(GetReadOnlyDataColor) (OLE_COLOR * pcolorReadOnlyDataColor);
    NV_STDMETHOD(SetReadOnlyDataColor) (OLE_COLOR colorReadOnlyDataColor);
    NV_STDMETHOD(GetRecordCount) (long * plRecordCount);
    NV_STDMETHOD(SetRecordCount) (long lRecordCount);
    NV_STDMETHOD(GetRecordLocks) (int * piRecordLocks);
    NV_STDMETHOD(SetRecordLocks) (int iRecordLocks);
    NV_STDMETHOD(GetRecordPosition) (int * piRecordPosition);
    NV_STDMETHOD(SetRecordPosition) (int iRecordPosition);
    NV_STDMETHOD(GetRecordSelectors) (VARIANT_BOOL * pfRecordSelectors);
    NV_STDMETHOD(SetRecordSelectors) (VARIANT_BOOL fRecordSelectors);
    NV_STDMETHOD(GetRecordset) (IDispatch ** ppdispRecordset);
    NV_STDMETHOD(GetRecordsetClone) (IDispatch ** ppdispRecordsetClone);
    NV_STDMETHOD(GetRecordSourceSample) (VARIANT_BOOL * pfRecordSourceSample);
    NV_STDMETHOD(SetRecordSourceSample) (VARIANT_BOOL fRecordSourceSample);
    NV_STDMETHOD(GetRecordSourceSync) (int * piRecordSourceSync);
    NV_STDMETHOD(SetRecordSourceSync) (int iRecordSourceSync);
    NV_STDMETHOD(GetRecordSourceType) (int * piRecordSourceType);
    NV_STDMETHOD(SetRecordSourceType) (int iRecordSourceType);
    NV_STDMETHOD(GetRepeat) (VARIANT_BOOL * pfRepeat);
    NV_STDMETHOD(SetRepeat) (VARIANT_BOOL fRepeat);
    NV_STDMETHOD(GetRequeryWhen) (int * piRequeryWhen);
    NV_STDMETHOD(SetRequeryWhen) (int iRequeryWhen);
    NV_STDMETHOD(GetResetPages) (VARIANT_BOOL * pfResetPages);
    NV_STDMETHOD(SetResetPages) (VARIANT_BOOL fResetPages);
    NV_STDMETHOD(GetScrollHeight) (long * plScrollHeight);
    NV_STDMETHOD(GetScrollLeft) (long * plScrollLeft);
    NV_STDMETHOD(SetScrollLeft) (long lScrollLeft);
    NV_STDMETHOD(GetScrollTop) (long * plScrollTop);
    NV_STDMETHOD(SetScrollTop) (long lScrollTop);
    NV_STDMETHOD(GetScrollWidth) (long * plScrollWidth);
    NV_STDMETHOD(GetSelectedControlBackCol) (OLE_COLOR *);
    NV_STDMETHOD(SetSelectedControlBackCol) (OLE_COLOR);
    NV_STDMETHOD(GetShowGridLines) (int * piShowGridLines);
    NV_STDMETHOD(SetShowGridLines) (int iShowGridLines);
    NV_STDMETHOD(GetShowNewRowAtBottom) (VARIANT_BOOL * pfShowNewRowAtBottom);
    NV_STDMETHOD(SetShowNewRowAtBottom) (VARIANT_BOOL fShowNewRowAtBottom);
    NV_STDMETHOD(GetShowWhen) (int * piShowWhen);
    NV_STDMETHOD(SetShowWhen) (int iShowWhen);
    NV_STDMETHOD(GetSnakingDirection) (int * piDirection);
    NV_STDMETHOD(SetSnakingDirection) (int iDirection);
    NV_STDMETHOD(GetSpecialEffect) (int * piSpecialEffect);
    NV_STDMETHOD(SetSpecialEffect) (int iSpecialEffect);
    NV_STDMETHOD(GetViewMode) (int * piViewMode);
    NV_STDMETHOD(SetViewMode) (int iViewMode);
    NV_STDMETHOD(GetLinkMasterFields) (BSTR * pbstrLinkMasterFields);
    NV_STDMETHOD(SetLinkMasterFields) (LPTSTR lpwstrLinkMasterFields);
    NV_STDMETHOD(GetLinkChildFields) (BSTR * pbstrLinkChildFields);
    NV_STDMETHOD(SetLinkChildFields) (LPTSTR lpwstrLinkChildFields);
    NV_STDMETHOD(GetDataDirty) (VARIANT_BOOL * pfDataDirty);
    NV_STDMETHOD(SetDataDirty) (VARIANT_BOOL fDataDirty);
    NV_STDMETHOD(GetLayoutCurrent) (VARIANT_BOOL * pfLayoutCurrent);
    NV_STDMETHOD(SetLayoutCurrent) (VARIANT_BOOL fLayoutCurrent);
    NV_STDMETHOD(GetNewData) (VARIANT_BOOL * pfNewData);
    NV_STDMETHOD(SetNewData) (VARIANT_BOOL fNewData);
    NV_STDMETHOD(GetRecordSelector) (IControlElement ** ppRecordSelector);
    NV_STDMETHOD(SetRecordSelector) (IControlElement * pRecordSelector);
    NV_STDMETHOD(GetArrowEnterFieldBehavior) (int * piArrowEnterFieldBehavior);
    NV_STDMETHOD(SetArrowEnterFieldBehavior) (int    iArrowEnterFieldBehavior);
    NV_STDMETHOD(GetArrowKeyBehavior) (int * piArrowKeyBehavior);
    NV_STDMETHOD(SetArrowKeyBehavior) (int    iArrowKeyBehavior);
    NV_STDMETHOD(GetArrowKeys) (int * piArrowKeys);
    NV_STDMETHOD(SetArrowKeys) (int    iArrowKeys);
    NV_STDMETHOD(GetDynamicTabOrder) (VARIANT_BOOL * pfDynamicTabOrder);
    NV_STDMETHOD(SetDynamicTabOrder) (VARIANT_BOOL    fDynamicTabOrder);
    NV_STDMETHOD(GetMoveAfterEnter) (int * piMoveAfterEnter);
    NV_STDMETHOD(SetMoveAfterEnter) (int   piMoveAfterEnter);
    NV_STDMETHOD(GetTabEnterFieldBehavior) (int * piTabEnterFieldBehavior);
    NV_STDMETHOD(SetTabEnterFieldBehavior) (int    iTabEnterFieldBehavior);
    NV_STDMETHOD(GetTabOut) (int * piTabOut);
    NV_STDMETHOD(SetTabOut) (int    iTabOut);
    NV_STDMETHOD(GetShowFooters) (VARIANT_BOOL *pfShowFooters);
    NV_STDMETHOD(SetShowFooters) (VARIANT_BOOL fShowFooters);

    NV_STDMETHOD(GetDirection) (fmRepeatDirection * piDirection);
    NV_STDMETHOD(SetDirection) (fmRepeatDirection iDirection);
    NV_STDMETHOD(GetMinCols) (long * plMinCols);
    NV_STDMETHOD(SetMinCols) (long lMinCols);
    NV_STDMETHOD(GetMaxCols) (long * plMaxCols);
    NV_STDMETHOD(SetMaxCols) (long lMaxCols);
    NV_STDMETHOD(GetNewRecordShow) (VARIANT_BOOL *pfNewRecordShow);
    NV_STDMETHOD(SetNewRecordShow) (VARIANT_BOOL fNewRecordShow);
#endif // Product_97



    //+-----------------------------------------------------------------------
    // Snaking API
    //------------------------------------------------------------------------
    STDMETHOD(GetSnakingType) (unsigned int *pSnakingDirection);
    STDMETHOD(SetSnakingType) (unsigned int direction,unsigned int uItemsAcross,unsigned int uItemsDown);




    // Scroll with keyboard, lReserved is KB_LINEUP to KB_BOTTOM
    KEYMETHOD(KeyScroll)       (long lReserved);
    KEYMETHOD(ProcessSpaceKey) (long lReserved);

    // insert new row at (INPLACE)
    KEYMETHOD(InsertNewRecordAt) (long lLocation);

    // Delete current row
    KEYMETHOD(DeleteCurrentRow) (long lReserved);

    inline  BOOL IsRepeated ()
        {
            return TBag()->_fRepeated;
        }
    HRESULT GetRepeated (OUT BOOL *pRepeated)
        {
            Assert (pRepeated);
            *pRepeated = TBag()->_fRepeated;
            return S_OK;
        }
    HRESULT SetRepeated (BOOL repeated)
        {
            TBag()->_fRepeated = repeated;
            return S_OK;
        }

    inline BOOL getDirection ()
        { return TBag()->_fDirection; }

    inline  BOOL ShowHeaders() {return TBag()->_fShowHeaders;}
    #if defined(PRODUCT_97)
    inline  BOOL ShowFooters() {return TBag()->_fShowFooters;}
    #endif

    inline BOOL IsNewRecordShow ()
        {
            return TBag()->_fNewRecordShow;
        }


protected:

    //+-----------------------------------------
    // Record selector helper
    //------------------------------------------

    HRESULT CreateRecordSelector(CRectl * prcl);

    //+-----------------------------------------
    // Keyboard handling related member data
    //------------------------------------------

    static KEY_MAP  s_aKeyActions[];
    static int      s_cKeyActions;

    //  Keyboad lookup helper
    virtual KEY_MAP * GetKeyMap(void);
    virtual int GetKeyMapSize(void);


public:
    KEYMETHOD(Generate) (long lReserved);

protected:
    // helper methods
    HRESULT ForwardPropertyChanges(UPDATEPROPS updFlag);

    HRESULT RecordNumberFromSelector(SelectUnit * psu, ULONG * pulRecordNumber);
    HRESULT SelectorFromRecordNumber(ULONG ulRecordNumber, SelectUnit * psu);
    void    CalcRowPosition (ULONG ulRecordNumber,
                             OUT long *plRowPosition,
                             OUT long *plPositionInRow);
    HRESULT GetRecordNumber (HROW hRow, ULONG *pulRecordNumber);


    HRESULT ReplaceQualifierWithRelativeRow(QUALIFIER * pqToModify, QUALIFIER * pqBase, int iRowOffset);

    BOOL GetScrollbarRectl (int iDirection, CRectl *prcl);
    void GetDetailRegion (CRectl *prclDetail);       // not counting scrollbars
    void GetDetailSectionRectl (CRectl *prclDetail); // with scrollbars
    void GetScrollableRectl (CRectl *prcl);

    //+-----------------------------------------------------------------------
    // Persistance code for the template part
    //------------------------------------------------------------------------

public:

    #if defined(PRODUCT_97)
    virtual HRESULT WriteProps(IStream * pStm, ULONG * pulObjSize);

    virtual HRESULT ReadProps(USHORT  usPropsVer,
                              USHORT  usFormVer,
                              BYTE *  pb,
                              USHORT  cb,
                              ULONG * pulObjSize);
    #endif


    //+-----------------------------------------------------------------------
    // Scrolling API
    //------------------------------------------------------------------------

    inline BOOL IsScrollBar (int iDirection)
    {
        CRectl  rclDetailView;
        GetDetailRegion (&rclDetailView);
        Assert (_pDetail);
        return IsScrollBar (iDirection, &rclDetailView, &_pDetail->_rcl);
    }

    BOOL IsScrollBar (int iDirection, CRectl* prclDetailView, CRectl* prclDetail);

    BOOL IsVerticalScrollBar ()
        {
            return IsScrollBar (1);
        }

    BOOL IsHorizontalScrollBar ()
        {
            return IsScrollBar (0);
        }


    virtual BOOL GetScrollbarRect (int iDirection, RECT *prc);

    virtual BOOL GetScrollInfo (int iDirection,
                                long *plPosition,
                                long *plVisible,
                                long *plSize);

    // ScrollBar events handler
    virtual HRESULT BUGCALL OnScroll (int iDirection, UINT uCode, long lPosition);

    virtual HRESULT ScrollBy (
                        long dxl,
                        long dyl,
                        fmScrollAction xAction,
                        fmScrollAction yAction);

    virtual void GetClientRectl(RECTL *prcl);

    virtual HTC  HitTestPointl(
            POINTL ptl,
            RECTL * prclClip,
            CSite ** ppSite,
            CMessage *pMessage,
            DWORD dwFlags);

    HRESULT UpdatePosRects1();

    long    GetScrollPos (int iDirection);

    BOOL    ScrollByPropsChange ();

private:
    void    ScrollRegion (const CRectl& rcl, long dxl, long dyl);
    void    InvalidateCurrent();

public:

    //+-----------------------------------------------------------------------
    //
    //  CDLSink Sink datalayer (cursor) events
    //
    //------------------------------------------------------------------------

    class CDLSink : public CDataLayerAccessorSourceEvents
    {
    public:
        //
        //  CDataLayerAccessorSourceEvents methods
        //

        HRESULT AllChanged();
        HRESULT RowsChanged(ULONG cRows, const HROW *ahRows);
        HRESULT FieldChanged(HROW hRow, ULONG iColumn);
        HRESULT RowsInserted(ULONG cRows, const HROW *ahRows);
        HRESULT DeletingRows(ULONG cRows, const HROW *ahRows);
        HRESULT RowsDeleted(ULONG cRows, const HROW *ahRows);
#if defined(VIADUCT)
        HRESULT OnNileError(HRESULT hr, BOOL fSupportsErrorInfo);
#endif // defined(VIADUCT)

        // helper methods

        CDataFrameTemplate *MyDataFrame();
        HRESULT CreateToFit();
    };

    //+-----------------------------------------------------------------------
    //
    //  CTBag (template bag)
    //
    //------------------------------------------------------------------------

    class CDataFrameTBag : public CBaseFrame::CBaseFrameTBag               // BUGBUG why can't it be CTBag ?
    {
    public:
        CDataFrameTBag();
        ~CDataFrameTBag();

        CRectl  _rclProposeDetail;
        CRectl  _rclProposeFooter;
        CRectl  _rclProposeHeader;
        CRectl  _rclProposeRepeater;


        // for normal creation (no cloning)
//        void * operator new(size_t cb) { return MemAllocClear(cb); }

        //+-----------------------------------------
        //  Extended multiselect anchor point
        //------------------------------------------
        SelectUnit  _suAnchor;              // anchor point for shift selection

        //+-----------------------------------------
        // Properties for the repeater
        //------------------------------------------

        unsigned int    _uPadding[2];       // the horizontal/vertical padding
        unsigned int    _uItems[2];         // For snaking ,number of items Across
                                            // (columns), n of items Down (rows).
        //+-----------------------------------------
        // generated properties
        //------------------------------------------
        // do not move the icons encapsulating the unnamed struct !!!
        IPicture                        *_pOutlineCollapsePicture;
        struct
        {
        unsigned                        _fDirection : 1;
        unsigned                        _fAllowAdditions : 1;
        unsigned                        _fAllowDeletions : 1;
        unsigned                        _fAllowEditing : 1;
        unsigned                        _fAllowFilters : 1;
        unsigned                        _fAutoEject : 1;
        unsigned                        _fContinousForm : 1;
        unsigned                        _fDataOnly : 1;
        unsigned                        _fDirty : 1;
        unsigned                        _fDirtyPencil : 1;
        unsigned                        _fFullIntervals : 1;
        unsigned                        _fHideDuplicates : 1;
        unsigned                        _fMoveLayout : 1;
        unsigned                        _fNavigationButtons : 1;
        unsigned                        _fNewStar : 1;
        unsigned                        _fNextRecord : 1;
        unsigned                        _fOneFooterPerPage : 1;
        unsigned                        _fOneHeaderPerPage : 1;
        unsigned                        _fPrintSection : 1;
        unsigned                        _fQBFAutoExecute : 1;
        unsigned                        _fQBFAvailable : 1;
        unsigned                        _fQBFMode : 1;
        unsigned                        _fQBFShowData : 1;
        unsigned                        _fRecordSourceSample : 1;
        unsigned                        _fResetPages : 1;
        unsigned                        _fShowNewRowAtBottom : 1;
        unsigned                        _fRepeated : 1;
        unsigned                        _fNewRecordShow : 1;
        unsigned                        _fSetMinMax:1;      // just a working field
        unsigned                        _fMinMaxPropertyChanged:1; // working field for resize logic
        };   // end unnamed struct
        IPicture                        * _pMousePicture;


        CStr                            _cstrFooter;
        CStr                            _cstrHeader;
        CStr                            _cstrOpenArgs;
        IPicture                        * _pOutlineExpandPicture;
        IPicture                        * _pOutlineLeafPicture;
        OLE_COLOR                       _colorAlternateBackColor;
        OLE_COLOR                       _colorDirtyDataColor;
        OLE_COLOR                       _colorNormalDataColor;
        OLE_COLOR                       _colorReadOnlyDataColor;
        OLE_COLOR                       _colorSelectedControlBackCol;
        int                             _iAllowUpdating;
        int                             _iAlternateInterval;
        int                             _iCommitSync;
        int                             _iCommitWhat;
        int                             _iCommitWhen;
        int                             _iDateGrouping;
        int                             _iDefaultEditing;
        int                             _iDisplayWhen;
        int                             _iForceNewPage;
        int                             _iGroupInterval;
        int                             _iGroupOn;
        int                             _iKeepTogether;
        int                             _iLayoutForPrint;
        int                             _iMousePointer;
        int                             _iNewRowOrCol;
        int                             _iOutline;
        int                             _iOutlinePrint;
        int                             _iOutlineShowWhen;
        int                             _iParentPosition;
        int                             _iRecordLocks;
        int                             _iRecordPosition;
        int                             _iRecordSourceSync;
        int                             _iRecordSourceType;
        int                             _iRequeryWhen;
        fmScrollBars                    _iScrollbars;
        int                             _iShow3D;
        int                             _iShowGridLines;
        int                             _iShowWhen;
        int                             _iSnakingDirection;
        int                             _iSpecialEffect;
        int                             _iViewMode;
        long                            _lFormatCount;
        long                            _lPrintCount;
        long                            _lRecordCount;
        long                            _lScrollLeft;
        long                            _lScrollTop;
        union
        {
            DWORD                       _ulBitflags;
            struct
            {
                unsigned                _fForceArrowsToTabs : 1;
                unsigned                _fArrowStaysInFrame : 1;
                unsigned                _uArrowEnterFieldBehavior : 2;
                unsigned                _fArrowKeyBehavior : 1;
                unsigned                _fDynamicTabOrder : 1;
                unsigned                _uMoveAfterEnter : 2;
                unsigned                _uTabEnterFieldBehavior : 2;
                unsigned                _uTabOut : 2;
                unsigned                _fShowHeaders:1;
                unsigned                _fShowFooters:1;
                unsigned                _eListBoxStyle : 2;
                unsigned                _eMultiSelect : 2;
                unsigned                _eListStyle : 2;
                unsigned                _enAutosize : 2;
                unsigned                _fRecordSelectors : 1;
                unsigned                _fNoCursorEvents : 1;   // just a working field
                unsigned                _fRefreshRows : 1;      // indicates rows changed
            };
        };
        // fmEnAutoSize                      _enAutosize;
        long                            _lMinRows;
        long                            _lMaxRows;
        long                            _lMinCols;
        long                            _lMaxCols;
        COleDataSite *                  _pctrlRecordSelector;

        CDLSink _dlSink;    // event sink for cursor notifications
    };

    typedef CDataFrameTBag CTBag;

    inline CTBag * TBag();  // { return (CTBag *)GetTBag(); }

    inline BOOL IsVertical () { return  TBag()->_fDirection; }

    inline BOOL IsListBoxStyle () { return TBag()->_eListBoxStyle; }

#if !defined(PRODUCT_97)
    const CDataLayerChapter& getGroupID() const { return CDataLayerChapter::TheNull; }
#else  // !defined(PRODUCT_97)
    // allow access to the chapter
    virtual const CDataLayerChapter& getGroupID() const = 0;
#endif // !defined(PRODUCT_97)

protected:

    CDataFrame(CDoc * pDoc, CSite * pParent, CDataFrame * pTemplate);

    // allocate size and memcpy it from original to create clone
    void * operator new (size_t s, CDataFrame * pOriginal);

#if defined(PRODUCT_97)
    // fetch the chapter from the cursor at specified hRow
    HRESULT GetGroupID(HROW, CDataLayerChapter *);
#endif // defined(PRODUCT_97)

    CBaseFrame      *   _pDetail;
    CBaseFrame      *   _pHeader;

#if defined(PRODUCT_97)
    CBaseFrame      *   _pBindSource;
    CBaseFrame      *   _pCaption;
    CBaseFrame      *   _pFooter;
    CDataLayerChapter   _groupID;
#endif // defined(PRODUCT_97)

    //+-----------------------------------------
    // Selection
    //------------------------------------------

    unsigned    _fPixelScrollingDisable: 1; // default is FALSE; helper flag
    unsigned    _fScrolling: 1;             // default is FALSE; TRUE means we are
                                            // inside ScrollRect
    unsigned    _fSubtractCurrent: 1;       // scratch flag for ScrollRegion
#ifdef PRODUCT_97

    unsigned    _fProposedOutsideScrollbars : 2;    // : 1 - TRUE if horizontal scrollbar is outside
                                                    // : 1 - TRUE if vertical scrollbar is outside
    unsigned    _fOutsideScrollbars : 2;            // : 1 - TRUE if horizontal scrollbar is outside
                                                    // : 1 - TRUE if vertical scrollbar is outside
#endif

    static CONNECTION_POINT_INFO    s_acpi[];
};

/* BUGBUG rgardner remover CControls
class CDataFrameControls : public CControls
{
    DECLARE_FORMS_SUBOBJECT_IUNKNOWN(CDataFrameTemplate)

protected:

    virtual CSite *   ParentSite(void);
};
*/


class CDataFrameTemplate : public CDataFrame
{
typedef CDataFrame super;

friend class CDataFrameControls;
friend class CDataFrame;
friend class CDataFrame::CDLSink;

public:
    CDataFrameTemplate(CDoc * pDoc, CSite * pParent) : super(pDoc, pParent) {}

#if defined(PRODUCT_97)
    const CDataLayerChapter& getGroupID() const { Assert(FALSE); return CDataLayerChapter::TheNull; }
#endif // defined(PRODUCT_97)

    virtual CSite::CTBag * GetTBag() { return &_TBag; }

    virtual CBase::CLASSDESC *GetClassDesc() const { return &s_classdesc;}

    virtual CPtrAry<CSite *> *  SitesCollection();

protected:
    CTBag _TBag;

    CPtrAry<CSite *>    _arySitesCollection;
    // BUGBUG rgardner removed Controls
    //CDataFrameControls  _Controls;

    static CLASSDESC                s_classdesc;
    static PROP_DESC                s_apropdesc[];
};


inline CPtrAry<CSite *> *
CDataFrameTemplate::SitesCollection( )
{
    return &_arySitesCollection;
}



class CDataFrameInstance : public CDataFrame
{
typedef CDataFrame super;

friend class CRepeaterFrame;
friend class CDataFrame;

public:

    virtual CSite::CTBag * GetTBag() { return getTemplate()->GetTBag(); }

    virtual CBase::CLASSDESC *GetClassDesc() const { return &s_classdesc;}

protected:

    // Cloning Constructor of CBaseFrame from the CBaseFrameTemplate.
    CDataFrameInstance(CDoc * pDoc, CSite * pParent, CDataFrame * pTemplate);

    ~CDataFrameInstance ();
#if defined(PRODUCT_97)
    const CDataLayerChapter& getGroupID() const
    {
        return _groupID;
    };
#endif // defined(PRODUCT_97)


    static CLASSDESC   s_classdesc;
};


inline CDataFrameTemplate *
CDataFrame::CDLSink::MyDataFrame()
{
    return CONTAINING_RECORD(this, CDataFrameTemplate, _TBag._dlSink);
};



// helper function to find the root frame
CRootDataFrame * RootFrame(CSite *pSite);

inline CDataFrame::CTBag * CDataFrame::TBag()
{
    Assert((CTBag *)GetTBag() == &((CDataFrameTemplate *)_pTemplate)->_TBag);
    return &((CDataFrameTemplate *)_pTemplate)->_TBag;
}


#if DBG==1
#define CHECKPROPOSEDFLAG(p)        CheckProposedFlag(p)
void    CheckProposedFlag(CDoc * pDoc);
#else
#define CHECKPROPOSEDFLAG(p)        {}
#endif


#endif  _DATAFRM_HXX_



