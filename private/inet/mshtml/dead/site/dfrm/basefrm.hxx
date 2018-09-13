//+---------------------------------------------------------------------------
//
//  Maintained by alexa.
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994-1995.
//
//  File:       basefrm.hxx
//
//  Contents:   definition of CBaseFrame
//
//  Classes     CBaseFrame
//
//----------------------------------------------------------------------------

#ifndef _BASEFRM_HXX_
#   define _BASEFRM_HXX_   1

#   ifndef _CSTR_HXX_
#       include "cstr.hxx"
#   endif

#   ifndef _KEYTABLE_HXX_
#       include "keytable.hxx"
#   endif

#   ifndef _SELECTION_HXX_
#       include "ddselect.hxx"
#   endif

#   ifndef _BINDING_HXX_
#       include "binding.hxx"
#   endif

#   include "propchg.hxx"

#   ifndef _2DSITE_HXX_
#       include "..\2dsite\2dsite.hxx"
#   endif

#include "selectr.hxx"

#define _hxx_
#include "basefrm.hdl"

/*----------------------------------------------------------------------------

                    DataDoc Class Hierarchy
                    -----------------------

CBaseFrame
|
|___CDetailFrame
|       |_____________CDetailFrameTemplate
|___CHeaderFrame
|
|___CRepeaterFrame
|
|___CDataFrame
    |______________________CDataFrameTemplate


----------------------------------------------------------------------------*/


//----------------------------------------------------------------------------
//  F O R W A R D    C L A S S   D E C L A R A T I O N S
//----------------------------------------------------------------------------

class CDataFrame;

//----------------------------------------------------------------------------
//  C O N S T A N T S
//----------------------------------------------------------------------------

const UINT FIRSTFRAME = 0;
const UINT LASTFRAME = 32000; // temp until we get a better list object !
const UINT FRAME_NOTFOUND = (UINT)(-1);

const BOOL REPEATED_LAYOUT = 1;
const BOOL STATIC_LAYOUT = 0;

const int DETAIL_LAYOUT = 0;
const int HEADER_LAYOUT = 1;
const int FOOTER_LAYOUT = 2;

const int NROFDATAFRAMEMETHODS = 111;

const int PREV_NODE = 0;
const int NEXT_NODE = 1;
const int BEGIN_NODE = 0;
const int END_NODE = 1;

const int FITRECORDS = -1;
const int FITPARENT = -2;

const unsigned int KB_SCROLLING         = 1000;
const unsigned int KB_LINEUP            = KB_SCROLLING + SB_LINEUP;
const unsigned int KB_LINEDOWN          = KB_SCROLLING + SB_LINEDOWN;
const unsigned int KB_PAGEUP            = KB_SCROLLING + SB_PAGEUP;
const unsigned int KB_PAGEDOWN          = KB_SCROLLING + SB_PAGEDOWN;
const unsigned int KB_TOP               = KB_SCROLLING + SB_TOP;
const unsigned int KB_BOTTOM            = KB_SCROLLING + SB_BOTTOM;
const unsigned int SB_SCROLLINTOVIEW    = SB_PAGEUP + 500;
const unsigned int SB_INSERTAT          = 2000;

//----------------------------------------------------------------------------
//  M A C R O S
//----------------------------------------------------------------------------

#define ABSOLUTE_COORDINATES    1

// Access subframe in the CParentSite class.
#define FRAME(i)      ( _arySites[i] )
#define FRAMESCOUNTER ( _arySites.Size() )

//----------------------------------------------------------------------------
//  H a n d y   Enums
//----------------------------------------------------------------------------
const DWORD en_MoveSomewhere =0;
const DWORD en_MoveTopOfCursor=1;
const DWORD en_MoveBottomOfCursor=2;

enum SETCURRENT_FLAGS
{
    SETCURRENT_NOINVALIDATE = 0x0001,
    SETCURRENT_NOFIREEVENT  = 0x0002,
};

//----------------------------------------------------------------------------
//  C L A S S   D E C L A R A T I O N S
//----------------------------------------------------------------------------


//+---------------------------------------------------------------------------
//
//  Class:      CBaseFrame
//
//  Purpose:    Base frame for all other frames (header/detail/footer/repeater/data)
//
//----------------------------------------------------------------------------

class CBaseFrame : public C2DSite
{

    typedef C2DSite super;

friend class CDataFrame;
friend class CRootDataFrame;
friend class CRepeaterFrame;
friend class CDetailFrame;


public:

    //+-----------------------------------------------------------------------
    //  IUnknown methods
    //------------------------------------------------------------------------

    STDMETHOD(PrivateQueryInterface)(REFIID riid, void ** ppv);
    DECLARE_FORMS_DELEGATING_IUNKNOWN(CBaseFrame)

//    DECLARE_TEAROFF_TABLE(IDataFrame)

    // frankman: due to the tearoff table implementation
    // none of the IDataFrame methods need to be virtual

    NV_STDMETHOD(GetTypeInfo)(UINT itinfo, ULONG lcid, ITypeInfo ** ppTI);
    NV_STDMETHOD(GetTypeInfoCount)(UINT * pctinfo);
    NV_STDMETHOD(GetIDsOfNames)(
            REFIID riid,
            LPTSTR * rgsz,
            UINT c,
            LCID lcid,
            DISPID * rgdispid);
    NV_STDMETHOD(Invoke) (
            DISPID          dispidMember,
            REFIID          riid,
            LCID            lcid,
            WORD            wFlags,
            DISPPARAMS *    pdispparams,
            VARIANT *       pvarResult,
            EXCEPINFO *     pexcepinfo,
            UINT *          puArgErr);

    //+-----------------------------------------------------------------------
    //  IControl
    //------------------------------------------------------------------------


    STDMETHOD(SetHeight) (long Height);
    STDMETHOD(SetLeft) (long Left);
    STDMETHOD(GetRowSource) (BSTR * pbstrRowSource);
    STDMETHOD(SetRowSource) (LPTSTR bstrRowSource);
    STDMETHOD(SetTop) (long Top);
    STDMETHOD(SetWidth) (long Width);

    // Methods

    STDMETHOD(Move) (long Left, long Top, long Width, long Height);


    //+-----------------------------------------------------------------------
    //  IDataFrame, generated property methods
    //------------------------------------------------------------------------

    #define _CBaseFrame_
    #include "basefrm.hdl"
/*
    STDMETHOD(GetControls) (IControls ** ppControls);       // overwritten in CDetailFrame
    NV_STDMETHOD(GetBackColor) (OLE_COLOR * pcolorBackColor);
    NV_STDMETHOD(SetBackColor) (OLE_COLOR colorBackColor);
    NV_STDMETHOD(GetForeColor) (OLE_COLOR * pcolorForeColor);
    NV_STDMETHOD(SetForeColor) (OLE_COLOR colorForeColor);
    NV_STDMETHOD(GetMouseIcon) (IDispatch * * ppDisp);
    NV_STDMETHOD(SetMouseIcon) (IDispatch * pDisp);
    NV_STDMETHOD(SetMouseIconByRef) (IDispatch * pDisp);
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



#if PRODUCT_97
    //#if DBG==1
    NV_STDMETHOD(GetRunTestcode) (int *piTestcode);
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
    NV_STDMETHOD(SetOutlineCollapseIconByRef) (IDispatch * hOutlineCollapseIcon);
    NV_STDMETHOD(GetOutlineExpandIcon) (IDispatch * * phOutlineExpandIcon);
    NV_STDMETHOD(SetOutlineExpandIcon) (IDispatch * hOutlineExpandIcon);
    NV_STDMETHOD(SetOutlineExpandIconByRef) (IDispatch * hOutlineExpandIcon);
    NV_STDMETHOD(GetOutlineLeafIcon) (IDispatch * * phOutlineLeafIcon);
    NV_STDMETHOD(SetOutlineLeafIcon) (IDispatch * hOutlineLeafIcon);
    NV_STDMETHOD(SetOutlineLeafIconByRef) (IDispatch * hOutlineLeafIcon);
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
    NV_STDMETHOD(GetRecordSelector) (IControl ** ppRecordSelector);
    NV_STDMETHOD(SetRecordSelector) (IControl * pRecordSelector);
    NV_STDMETHOD(GetRecordState)   (int * piRecordState);
    NV_STDMETHOD(SetRecordState)   (int iRecordState);
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
*/

    //+-----------------------------------------------------------------------
    //  Core methods override
    //------------------------------------------------------------------------

    virtual HRESULT Notify(SITE_NOTIFICATION, DWORD);

    // Move/Resize/Positioning
    virtual HRESULT Move (RECTL *prcl, DWORD dwFlags = 0);

    // modify _rcl directly
    virtual void MoveSiteBy(long x, long y);

    //  Helpers to hook up the site to the form's keypress pump

    virtual HRESULT InsertNewSite(
            REFCLSID clsid,
            LPCTSTR pstrName,
            const RECTL * prcl,
            DWORD dwOperations,
            CSite ** ppSite);

    //+-----------------------------------------------------------------------
    //  Data access
    //------------------------------------------------------------------------

    virtual HRESULT GetData (CDataLayerAccessor * dla, LPVOID lpv) { return E_FAIL; }   // for now...
    virtual HRESULT SetData (CDataLayerAccessor * dla, LPVOID lpv) { return E_FAIL; }
    virtual HRESULT GetRecordNumber (ULONG *pulRecordNumber);
    virtual BOOL    EnsureHRow ()   {return TRUE; }
    virtual HRESULT RefreshData();
    virtual void DataSourceChanged();


    //+-----------------------------------------------------------------------
    //  Keyboard handling overrides
    //------------------------------------------------------------------------

    //
    // Handle messages by bubbling out or in when necessary
    //-------------------------------------------------------------------------
        //      +override : when additional handling needed beyond default
        //      +call super : when not handled
        //      +call parent : when not handled
    //  +call children : for special events
    //-------------------------------------------------------------------------
    virtual HRESULT BUGCALL HandleMessage(CMessage * pMessage, CSite * pChild);

    // global event firing mechanism, deals with all WindowMsg related events...
    void    FireKeyboardEvent(UINT msg, int *pInfo);
    void    FireMouseEvent(UINT msg, int x, int y, WORD wParam);

    HRESULT FireDragEvent(fmDragState fmdragstate, DWORD grfKeystate, POINTL pt, DWORD *pdwEffect, VARIANT_BOOL *pfCancel);
    HRESULT FireDropEvent(DWORD grfKeystate, POINTL pt, DWORD *pdwEffect, VARIANT_BOOL *pfCancel);




    HRESULT DoKeyAction(UINT vk, BOOL fDown, UINT flags);
    virtual KEY_MAP * GetKeyMap(void);
    virtual int GetKeyMapSize(void);
    KEY_MAP * FindKeyHandler(UINT vk, BOOL fDown, UINT flags);


    KEYMETHOD(RotateControl) (long lDirection);
    KEYMETHOD(ProcessEnterKey) (long lDirection);

    virtual HRESULT NavigateToNextControl(NAVIGATE_DIRECTION);
    virtual HRESULT NextControl(NAVIGATE_DIRECTION Direction,
                                CSite * psCurrent,
                                CSite ** ppsNext,
                                CSite *pSiteCaller,
                                BOOL fStrictInDirection = FALSE,    //  default = FALSE
                                int iFirstFrame = -1,               //  default -1
                                int iLastFrame = -1);               //  default -1

    // Helper methods

    BOOL AreMyChildren(int c, CSite ** ppSite);
    UINT Find(CSite * pSite) { return _arySites.Find(pSite); }

    inline CBaseFrame * getTemplate ()
    {
        return _pTemplate;
    }


    //+-----------------------------------------------------------------------
    //  Selection API
    //+-----------------------------------------------------------------------

    void SetSelectionStates(DWORD dwFlags = SS_SETSELECTIONSTATE)
    {
        IGNORE_HR(SelectSite(this, dwFlags));
    }

    void ResetSelectionPointer(void)
    {
        _pSelectionElement = NULL;
    }

    inline void SetSelectionRoot(CArySelector * pselRoot)
    {
        Assert(_fOwnTBag);  //  to assure this is a template thing
        _pSelectionRoot = pselRoot;
    }

    inline CArySelector * GetSelectionRoot(void)
    {
        Assert(_fOwnTBag);  //  to assure this is a template thing
        return _pSelectionRoot;
    }

    virtual int getIndex(void);
    virtual void SetIndex(int i) { TBag()->_iIndex = i; };

    void RemoveFromSelection(void);

    virtual CSite * CreateSite(ELEMENT_TAG type);
    HRESULT AddToSites(CSite * pSite, DWORD dwOperations);
    HRESULT DeleteSite(CSite * pSite, DWORD dwFlags);

    virtual HRESULT RemoveRelated(CSite *pSite, DWORD dwFlags) { return S_OK; }

    virtual ELEMENT_TAG SiteTypeFromCLSID(REFCLSID clsid);

    BOOL    IsPopulated(void)           { return _fIsPopulated;};
    void    SetPopulated(BOOL fStatus)  { _fIsPopulated = fStatus;};
    BOOL    IsInSiteList(void)          { return _fInSiteList;};
    void    SetInSiteList(BOOL fStatus) { _fInSiteList = fStatus;};

    inline BOOL IsDirtyBelow ()         { return _fDirtyBelow; }
    inline void SetDirtyBelow(BOOL f)   { _fDirtyBelow = f; }

    inline BOOL IsAnythingDirty ()      { return getTemplate()->_fIsDirtyRectangle ||
                                          getTemplate()->IsDirtyBelow() ||
                                          _fIsDirtyRectangle;
                                        }
    //
    // positioning API
    //


    // this method is overwritten on the detail frame
    //  and on the repeater
    virtual HRESULT MoveToRow(HROW hrow, DWORD dwFlags=0) { return E_NOTIMPL;};
    virtual HRESULT DeleteRow(DWORD dwFlags=0) { return E_NOTIMPL; };
    virtual HROW    getHRow(void) { Assert(FALSE && "getHRow should not be called on baseframe"); return 0;};

    // CalcProposed Helper
    virtual HRESULT ProposedFriendFrames(void) {_fMark1 = TRUE; return S_OK;};
    virtual HRESULT ProposedDelta(CRectl *rclDelta) { _fMark1 = TRUE; return S_OK;};
    virtual HRESULT MoveToProposed(DWORD dwFlags)
    { _fProposedSet1 = FALSE; return super::MoveToProposed(dwFlags); }

    //+-----------------------------------------------------------------------
    //
    //  CTBag (template bag)
    //
    //------------------------------------------------------------------------

    class CBaseFrameTBag : public CSite::CTBag
    {
    public:

        CBaseFrameTBag() { _iIndex = -1; }

        ~CBaseFrameTBag() {}

        void * operator new(size_t cb) { return MemAllocClear(cb); }

        int             _iIndex;
        CPropertyChange _propertyChanges;
    };

    typedef CBaseFrameTBag CTBag;

    CTBag * TBag() { return (CTBag *)GetTBag(); }


    CSite * GetTemplate(void) { return _pTemplate; }

    virtual CDataFrame * getOwner() = 0;        // owner of this frame

    HRESULT OnDataChange (DISPID dispid, BOOL fInvalidate = TRUE);
    HRESULT OnDataChangeCloseError(DISPID dispid, BOOL fInvalidate = TRUE)
    {
        RRETURN(CloseErrorInfo(OnDataChange(dispid, fInvalidate)));
    }

    HRESULT PropagateChange(DISPID);

    // gets common hrs and puts the EPART_ERROR text on the error object
    BOOL ConstructErrorInfo(HRESULT hr, int ids=-1);



    //+-----------------------------------------------------------------------
    // Instance Creation/Population API
    //------------------------------------------------------------------------

    // calculate the repeater's virtual space rectangle (_rcl)
    virtual void CalcRectangle (BOOL fProposed=FALSE) {};

    // deleting rows from the cursor
    virtual void DeletingRows (ULONG, const HROW *) {};

protected:

    // Protected methods
    virtual void SetPropertyFromTemplate(DISPID dispid);
    // Build instance created by CreateInstance method (template method)
    // BuildInstance is an Overriden method from CSite.
    HRESULT BuildInstance (CBaseFrame * pNewInstance, CCreateInfo * pcinfo);

    // Populate the view rectangle.
    HRESULT CreateToFit (const CRectl * prclView, DWORD dwFlags);

    // Populate subframes in the view rectangle.
    HRESULT CreateToFitSubFrames (const CRectl * prclView, DWORD dwFlags);

    virtual HRESULT PrepareToScroll (IN int iDirection,
                                     IN UINT uCode,
                                     IN BOOL fKeyboardScrolling,
                                     IN long lPosition,
                                     OUT long *pDelta,
                                     CBaseFrame *plfr) { return S_OK; }

    virtual HRESULT UpdatePosRects1()   { return S_OK; }

    BOOL IsAutosizeHeight ();
    BOOL IsAutosizeWidth ();
    BOOL IsAutosizeDir (int iDirection);
    BOOL IsAutosize ();

protected:
    // property set/get helper methods, needed && implemented mostly in frmprops.cxx
    inline HRESULT GetInt(int *pInt, int iValue)
    {
        return (GetLong((long*)pInt, (long) iValue));
    }
    inline HRESULT GetInt(unsigned int *puInt, unsigned int uiValue)
    {
        return (GetInt((int*)puInt, (int) uiValue));
    }

    HRESULT SetInt(int *pInt, int iValue, int iRangeStart, int iRangeEnd, DISPID dispid);
    inline HRESULT SetInt(int *pInt, int iValue, DISPID dispid)
    {
        return (SetLong((long*)pInt, (long)iValue, dispid));
    };

    HRESULT GetBool(VARIANT_BOOL *pf, BOOL flag);

    HRESULT GetColor(OLE_COLOR *pOlecolor, OLE_COLOR olecolor);
    HRESULT SetColor(OLE_COLOR *pOlecolor, OLE_COLOR olecolor, DISPID dispid);

    HRESULT GetLong(long *pLong, long l);
    HRESULT SetLong(long *pLong, long l, DISPID dispid);

    HRESULT GetBstr(BSTR *pbstr, CStr &cstr);
    HRESULT SetBstr(LPTSTR lp, CStr &cstr, DISPID dispid);

    inline HRESULT GetIcon(IDispatch **pIcon, IDispatch * icon)
    {
        return (GetLong((long*)pIcon, (long) icon));
    }
    inline HRESULT SetIcon(IDispatch **pIcon, IDispatch *icon, DISPID dispid)
    {
        return SetLong((long*)pIcon, (long)icon, dispid);
    }



public:
    OLE_COLOR           _colorFore;
    OLE_COLOR           _colorBack;

protected:

    CBaseFrame * _pTemplate;

    unsigned            _fEnabled : 1;          // frame enabled
    unsigned            _fDataDirty : 1;        // data dirty in record
    unsigned            _fCurrent : 1;    // record is current
    unsigned            _fLockRecord : 1;       // record is locked
    unsigned            _fNewData : 1;          // newly created record
    unsigned            _fIsPopulated :1;       // is cached or not
    unsigned            _fInSiteList : 1;       // is in sitelist or not
    unsigned            _fDirtyBelow : 1;       // frame has a dirty frame in
    unsigned            _fPaintBackground : 1;  // detailframe background shows through
    unsigned            _fSelectionChanged :1;  // selection state has changed in the repeater
    unsigned            _fIsFooter : 1;
    unsigned            _iMouseClickType:2;     // Mouse click type (0 = Single, 1 = Double, 2 = Canceled Double)
    unsigned            _fProposedSet1 : 1;     // Proposed flag is set tot true when SetProposed is called, reset on MoveToProposed

    // Set the layout framel to be current in the repeater (update record
    // selector).
    HRESULT SetCurrent (BOOL fCurrent);

    BOOL IsCurrent () { return _fCurrent; }

    //+-----------------------------------------------------------------------
    // Protected member data
    //------------------------------------------------------------------------

    CBaseFrame          *_apNode[2];    // Pointer to a previous, next frame in
                                        // the repeater cache.

    //+-----------------------------------------
    // Selection related member data
    //------------------------------------------

    //  This is a placeholder for the selection tree pointers in DataFrame
    //  and in DetailFrame. Both of them need a single pointer
    //  but of a different kind. We have an unnamed union here to reuse the space.
    union
    {
        SelectUnit *    _pSelectionElement;     //  This is for DetailFrame
        CArySelector *  _parySelectedQualifiers;//  This is for DataFrame
        CArySelector *  _pSelectionRoot;        //  This is for the root dataframe
                                                //      template
    };

    static CInvokeElem  s_Invokes[NROFDATAFRAMEMETHODS];
#ifdef NEVER
#if DBG==1
    static CCheckInvokeTable s_ccheck;
#endif
#endif  // NEVER

    static const CLSID * s_aclsidPages[];

    //+-----------------------------------------
    // Keyboard handling related member data
    //------------------------------------------

    static KEY_MAP  s_aKeyActions[];
    static int      s_cKeyActions;

private:

    //+-----------------------------------------------------------------------
    //  Constructors, destructors, initialization, passivation.
    //------------------------------------------------------------------------

    CBaseFrame(CDoc * pDoc, CSite * pParent);

    // for normal creation (no cloning)
    void * operator new(size_t cb) { return MemAllocClear(cb); }

    // Cloning Constructor of CBaseFrame from the CBaseFrameTemplate.
    CBaseFrame(CDoc * pDoc, CSite * pParent, CBaseFrame * pTemplate);

        // allocate size and memcpy it from original to create clone
    void * operator new (size_t s, CBaseFrame * pOriginal);

protected:
    // Initialization member function used after
    // using the above constructor.
    HRESULT InitInstance ();

    static const PROPERTYDESC * const s_appropdescs[];
};

#endif _BASEFRM_HXX_



