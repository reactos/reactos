//----------------------------------------------------------------------------
//
//  Maintained by: istvanc
//
//  Microsoft DataDocs
//  Copyright (C) Microsoft Corporation, 1994-1995
//
//  File:       src\doc\datadoc\datafrm.cxx
//
//  Contents:   Implementation of the data frame.
//
//  Classes:    CRootDataFrame
//
//  Functions:  None.
//
//----------------------------------------------------------------------------

#include "headers.hxx"
#include "dfrm.hxx"

#include "rootfrm.hxx"

DeclareTag(tagRootDataFrame,"src\\ddoc\\datadoc\\rootfrm.cxx","RootDataFrame");
extern TAG tagPropose;


#if PRODUCT_97
PROP_DESC CDataFrameTemplate::s_apropdesc[] =
{
    PROP_MEMBER(CSTRING,
                CDataFrameTemplate,
                _TBag._cstrName,
                NULL,
                "Name",
                PROP_DESC_NOBYTESWAP)
    PROP_MEMBER(CSTRING,
                CDataFrameTemplate,
                _IBag._cstrTag,
                NULL,
                "Tag",
                PROP_DESC_NOBYTESWAP)
    PROP_MEMBER(LONG,
                CDataFrameTemplate,
                _TBag._ID,
                0,
                "ID",
                PROP_DESC_BYTESWAPLONG)
    PROP_MEMBER(LONG,
                CDataFrameTemplate,
                _dwHelpContextID,
                0,
                "HelpContextID",
                PROP_DESC_BYTESWAPLONG)
 //   PROP_MEMBER(LONG,
 //               CDataFrameTemplate,
 //               _ulBitFlags,
 //               SITE_FLAG_DEFAULTVALUE,
 //               "BitFlags",
 //               PROP_DESC_BYTESWAPLONG)
    PROP_VARARG(LONG,
                sizeof(DWORD),
                0,
                "ObjectStreamSize",
                PROP_DESC_BYTESWAPLONG)
    PROP_MEMBER(SHORT,
                CDataFrameTemplate,
                _TBag._usTabIndex,
                (ULONG)-1L,
                "TabIndex",
                PROP_DESC_BYTESWAPSHORT)
    PROP_MEMBER(USERDEFINED,
                CDataFrameTemplate,
                _rcl,
                NULL,
                "SizeAndPosition",
                PROP_DESC_BYTESWAPRECTL)
    PROP_MEMBER(SHORT,
                CDataFrameTemplate,
                _TBag._usGroupID,
                0,
                "GroupID" ,
                PROP_DESC_BYTESWAPSHORT)

    // the above stuff is copied from the CSite implementation
    // now we first save the unnamed struct, this includes all bitfields

    // BUGBUG -- This needs to be fixed to not use bitfields
    PROP_CUSTOM(WPI_USERDEFINED,
        offsetof(CDataFrameTemplate, _TBag._pMousePicture) -    // starting point of variable after
        (offsetof(CDataFrameTemplate, _TBag._pOutlineCollapsePicture)+sizeof(((CDataFrameTemplate*)0)->_TBag._pOutlineCollapsePicture)), // endpoint of variable before
        (offsetof(CDataFrameTemplate, _TBag._pOutlineCollapsePicture)+sizeof(((CDataFrameTemplate*)0)->_TBag._pOutlineCollapsePicture)), // endpoint of variable before
        NULL,
        "Layout::BitFieldStruct",
        PROP_DESC_NOBYTESWAP)

    PROP_NOPERSIST(CSTRING, sizeof(CStr), NULL)   // Old ControlSource
    // iff none of the four properties "Database", "LinkChildFields",
    // "LinkMasterFields" and "RowSource" have a non-null value we don't need a
    // CDataLayer (DLAS) in CSite::TBag.  So we get these four using vararg's.
    // sizeof a pointer, offset immaterial
    PROP_VARARG(CSTRING,
                sizeof(CStr),
                0,
                "Database",
                PROP_DESC_NOBYTESWAP)
    PROP_MEMBER(CSTRING,
                CDataFrameTemplate,
                _TBag._cstrFooter,
                NULL,
                "Footer",
                PROP_DESC_NOBYTESWAP)
    PROP_MEMBER(CSTRING,
                CDataFrameTemplate,
                _TBag._cstrHeader,
                NULL,
                "Header",
                PROP_DESC_NOBYTESWAP)
    // See "Database" above...
    // sizeof a pointer, offset immaterial
    PROP_VARARG(CSTRING,
                sizeof(CStr),
                NULL,
                "LinkChildFields",
                PROP_DESC_NOBYTESWAP)
    // See "Database" above...
    // sizeof a pointer, offset immaterial
    PROP_VARARG(CSTRING,
                sizeof(CStr),
                NULL,
                "LinkMasterFields",
                PROP_DESC_NOBYTESWAP)
    PROP_MEMBER(CSTRING,
                CDataFrameTemplate,
                _TBag._cstrOpenArgs,
                NULL,
                "OpenArgs" ,
                PROP_DESC_NOBYTESWAP)
    // See "Database" above...
    // sizeof a pointer, offset immaterial
    PROP_VARARG(CSTRING,
                sizeof(CStr),
                NULL,
                "RowSource",
                PROP_DESC_NOBYTESWAP)
    PROP_MEMBER(LONG,
                CDataFrameTemplate,
                _TBag._colorAlternateBackColor,
                NULL,
                "AlternateBackColor",
                PROP_DESC_BYTESWAPLONG)
    PROP_MEMBER(LONG,
                CDataFrameTemplate,
                _TBag._colorDirtyDataColor,
                NULL,
                "DirtyDataColor",
                PROP_DESC_BYTESWAPLONG)

    PROP_MEMBER(LONG,
                CDataFrameTemplate,
                _TBag._colorNormalDataColor,
                NULL,
                "NormalDataColor",
                PROP_DESC_BYTESWAPLONG)

    PROP_MEMBER(LONG,
                CDataFrameTemplate,
                _TBag._colorReadOnlyDataColor,
                NULL,
                "ReadOnlyDataColor",
                PROP_DESC_BYTESWAPLONG)
    PROP_MEMBER(LONG,
                CDataFrameTemplate,
                _TBag._colorSelectedControlBackCol,
                NULL,
                "SelectedControlBackCol" ,
                PROP_DESC_BYTESWAPLONG)
    PROP_MEMBER(BYTE,
                CDataFrameTemplate,
                _TBag._iAllowUpdating,
                NULL,
                "AllowUpdating",
                PROP_DESC_NOBYTESWAP)
    PROP_MEMBER(BYTE,
                CDataFrameTemplate,
                _TBag._iAlternateInterval,
                NULL,
                "AlternateInterval",
                PROP_DESC_NOBYTESWAP)
    PROP_NOPERSIST(BYTE, sizeof(int), NULL)   // Old RowSpacing
    PROP_MEMBER(BYTE,
                CDataFrameTemplate,
                _TBag._iCommitSync,
                NULL,
                "CommitSync",
                PROP_DESC_NOBYTESWAP)

    PROP_MEMBER(BYTE,
                CDataFrameTemplate,
                _TBag._iCommitWhen,
                NULL,
                "CommitWhen",
                PROP_DESC_NOBYTESWAP)

    PROP_MEMBER(BYTE,
                CDataFrameTemplate,
                _TBag._iCommitWhat,
                NULL,
                "CommitWhat",
                PROP_DESC_NOBYTESWAP)

    PROP_MEMBER(BYTE,
                CDataFrameTemplate,
                _TBag._iDateGrouping,
                NULL,
                "DateGrouping",
                PROP_DESC_NOBYTESWAP)

    PROP_MEMBER(BYTE,
                CDataFrameTemplate,
                _TBag._iDefaultEditing,
                NULL,
                "DefaultEditing",
                PROP_DESC_NOBYTESWAP)

    PROP_MEMBER(BYTE,
                CDataFrameTemplate,
                _TBag._iDisplayWhen,
                NULL,
                "DisplayWhen",
                PROP_DESC_NOBYTESWAP)

    PROP_MEMBER(BYTE,
                CDataFrameTemplate,
                _TBag._iForceNewPage,
                NULL,
                "ForceNewPage",
                PROP_DESC_NOBYTESWAP)

    PROP_MEMBER(BYTE,
                CDataFrameTemplate,
                _TBag._iGroupInterval,
                NULL,
                "GroupInterval",
                PROP_DESC_NOBYTESWAP)

    PROP_MEMBER(BYTE,
                CDataFrameTemplate,
                _TBag._iGroupOn,
                NULL,
                "GroupOn",
                PROP_DESC_NOBYTESWAP)

    PROP_NOPERSIST(BYTE, sizeof(int), NULL)   // Old ItemsDown

    PROP_MEMBER(BYTE,
                CDataFrameTemplate,
                _TBag._iKeepTogether,
                NULL,
                "KeepTogether",
                PROP_DESC_NOBYTESWAP)

    PROP_MEMBER(BYTE,
                CDataFrameTemplate,
                _TBag._iLayoutForPrint,
                NULL,
                "LayoutForPrint",
                PROP_DESC_NOBYTESWAP)

    PROP_MEMBER(BYTE,
                CDataFrameTemplate,
                _TBag._iMousePointer,
                NULL,
                "MousePointer",
                PROP_DESC_NOBYTESWAP)

    PROP_MEMBER(BYTE,
                CDataFrameTemplate,
                _TBag._iNewRowOrCol,
                NULL,
                "NewRowOrCol",
                PROP_DESC_NOBYTESWAP)

    PROP_MEMBER(BYTE,
                CDataFrameTemplate,
                _TBag._iOutline,
                NULL,
                "Outline",
                PROP_DESC_NOBYTESWAP)

    PROP_MEMBER(BYTE,
                CDataFrameTemplate,
                _TBag._iOutlinePrint,
                NULL,
                "OutlinePrint",
                PROP_DESC_NOBYTESWAP)

    PROP_MEMBER(BYTE,
                CDataFrameTemplate,
                _TBag._iOutlineShowWhen,
                NULL,
                "OutlineShowWhen",
                PROP_DESC_NOBYTESWAP)

    PROP_MEMBER(BYTE,
                CDataFrameTemplate,
                _TBag._iParentPosition,
                NULL,
                "ParentPosition",
                PROP_DESC_NOBYTESWAP)

    PROP_MEMBER(BYTE,
                CDataFrameTemplate,
                _TBag._iRecordLocks,
                NULL,
                "RecordLocks" ,
                PROP_DESC_NOBYTESWAP)

    PROP_MEMBER(BYTE,
                CDataFrameTemplate,
                _TBag._iRecordPosition,
                NULL,
                "RecordPosition",
                PROP_DESC_NOBYTESWAP)

    PROP_NOPERSIST(BYTE, sizeof(LONG), NULL)   // old _iRecordSelectors

    PROP_MEMBER(BYTE,
                CDataFrameTemplate,
                _TBag._iRecordSourceSync,
                NULL,
                "RecordSourceSync",
                PROP_DESC_NOBYTESWAP)

    PROP_MEMBER(BYTE,
                CDataFrameTemplate,
                _TBag._iRecordSourceType,
                NULL,
                "RecordSourceType",
                PROP_DESC_NOBYTESWAP)

    PROP_MEMBER(BYTE,
                CDataFrameTemplate,
                _TBag._iRequeryWhen,
                NULL,
                "RequeryWhen",
                PROP_DESC_NOBYTESWAP)

    PROP_MEMBER(BYTE,
                CDataFrameTemplate,
                _TBag._iScrollbars,
                NULL,
                "Scrollbars",
                PROP_DESC_NOBYTESWAP)

    PROP_MEMBER(BYTE,
                CDataFrameTemplate,
                _TBag._iShow3D,
                NULL,
                "Show3D" ,
                PROP_DESC_NOBYTESWAP)

    PROP_MEMBER(BYTE,
                CDataFrameTemplate,
                _TBag._iShowGridLines,
                NULL,
                "ShowGridLines",
                PROP_DESC_NOBYTESWAP)

    PROP_MEMBER(BYTE,
                CDataFrameTemplate,
                _TBag._iShowWhen,
                NULL,
                "ShowWhen",
                PROP_DESC_NOBYTESWAP)

    PROP_MEMBER(BYTE,
                CDataFrameTemplate,
                _TBag._iSnakingDirection,
                NULL,
                "SnakingDirection" ,
                PROP_DESC_NOBYTESWAP)

    PROP_MEMBER(BYTE,
                CDataFrameTemplate,
                _TBag._iSpecialEffect,
                NULL,
                "SpecialEffect" ,
                PROP_DESC_NOBYTESWAP)

    PROP_MEMBER(BYTE,
                CDataFrameTemplate,
                _TBag._iViewMode,
                NULL,
                "ViewMode" ,
                PROP_DESC_NOBYTESWAP)

    PROP_MEMBER(LONG,
                CDataFrameTemplate,
                _TBag._lFormatCount,
                NULL,
                "FormatCount" ,
                PROP_DESC_BYTESWAPLONG)

    PROP_MEMBER(LONG,
                CDataFrameTemplate,
                _TBag._lPrintCount,
                NULL,
                "PrintCount",
                PROP_DESC_BYTESWAPLONG)

    PROP_MEMBER(LONG,
                CDataFrameTemplate,
                _TBag._lRecordCount,
                NULL,
                "RecordCount",
                PROP_DESC_BYTESWAPLONG)

    PROP_NOPERSIST(LONG, sizeof(LONG), NULL)   // old _lScrollHeight

    PROP_MEMBER(LONG,
                CDataFrameTemplate,
                _TBag._lScrollLeft,
                NULL,
                "ScrollLeft" ,
                PROP_DESC_BYTESWAPLONG)

    PROP_MEMBER(LONG,
                CDataFrameTemplate,
                _TBag._lScrollTop,
                NULL,
                "ScrollTop",
                PROP_DESC_BYTESWAPLONG)

    PROP_NOPERSIST(LONG, sizeof(LONG), NULL)   // old _lScrollWidth

    PROP_VARARG(LONG,
                sizeof(LONG),
                0,
                "Detail_ID" ,
                PROP_DESC_BYTESWAPLONG)

    // + RowSpacing
    PROP_MEMBER(USERDEFINED,
                CDataFrameTemplate,
                _TBag._uPadding,
                NULL,
                "ColumnSpacing",
                PROP_DESC_BYTESWAP2INTS)

    // + ItemsDown
    PROP_MEMBER(USERDEFINED,
                CDataFrameTemplate,
                _TBag._uItems,
                NULL,
                "ItemsAcross",
                PROP_DESC_BYTESWAP2INTS)

    // + new flags in _ulBitflags
    PROP_MEMBER(LONG,
                CDataFrameTemplate,
                _TBag._ulBitflags,
                NULL,
                "Layout::BitFlags" ,
                PROP_DESC_BYTESWAPLONG)

    PROP_VARARG(LONG,
                sizeof(LONG),
                0,
                "Header_ID",
                PROP_DESC_BYTESWAPLONG)

    PROP_VARARG(LONG,
                sizeof(LONG),
                0,
                "Footer_ID",
                PROP_DESC_BYTESWAPLONG)

    PROP_NOPERSIST(LONG, sizeof(long), NULL)   // Old WhatsThisHelpID

    PROP_MEMBER(CSTRING,
                CDataFrameTemplate,
                _TBag._cstrControlTipText,
                NULL,
                "ControlTipText",
                PROP_DESC_NOBYTESWAP)

    PROP_MEMBER(USERDEFINED,
                CDataFrameTemplate,
                _TBag._lMinRows,
                NULL,
                "MinRows" ,
                PROP_DESC_BYTESWAPLONG)

    PROP_MEMBER(USERDEFINED,
                CDataFrameTemplate,
                _TBag._lMaxRows,
                NULL,
                "MaxRows" ,
                PROP_DESC_BYTESWAPLONG)

    PROP_MEMBER(USERDEFINED,
                CDataFrameTemplate,
                _TBag._lMinCols,
                NULL,
                "MinCols" ,
                PROP_DESC_BYTESWAPLONG)

    PROP_MEMBER(USERDEFINED,
                CDataFrameTemplate,
                _TBag._lMaxCols,
                NULL,
                "MaxCols",
                PROP_DESC_BYTESWAPLONG)


    PROP_NOPERSIST(CSTRING, sizeof(CStr), NULL)   //  Old StatusBarText
};

#endif PRODUCT_97

CONNECTION_POINT_INFO   CDataFrame::s_acpi[] =
{
    CPI_PNS(CDataFrame)
    CPI_DISPATCH(CDataFrame, DataFrameEvents)
};


CSite::CLASSDESC CDataFrameTemplate::s_classdesc =
{
    {
        {
            &CLSID_CDataFrame,              // _pclsid
            IDR_BASE_FRAME,                 // _idrBase
#if PRODUCT_97
            s_aclsidPages,                  // _apClsidPages
#else
            NULL,
#endif
            ARRAY_SIZE(s_acpi),             // _ccp
            s_acpi,                         // _pcpi
#if PRODUCT_97
            ARRAY_SIZE(s_apropdesc),        // _cpropdesc
            s_apropdesc,                    // _ppropdesc
#else
            0,
            0,
#endif
            SITEDESC_PARENT |              // _dwFlags
            SITEDESC_BASEFRAME |
            SITEDESC_DATAFRAME |
            SITEDESC_TEMPLATE,
            &IID_IDataFrame,                // _piidDispinterface
            0,                              // _lAccRole
            &s_PropCats,                    // _pPropCats;
            NULL
        },
        s_apfnIControlElement,                 // _pfnTearOff
    },
    ETAG_DATAFRAME,                       // _st
};



CSite::CLASSDESC CDataFrameInstance::s_classdesc =
{
    {
        {
            &CLSID_CDataFrame,              // _pclsid
            IDR_BASE_FRAME,                 // _idrBase
#if PRODUCT_97
            s_aclsidPages,                  // _apClsidPages
#else
            NULL,
#endif
            ARRAY_SIZE(s_acpi),             // _ccp
            s_acpi,                         // _pcpi
            0,                              // _cpropdesc
            NULL,                           // _ppropdesc
            SITEDESC_PARENT |               // dwFlags
            SITEDESC_BASEFRAME |
            SITEDESC_DATAFRAME,
            &IID_IDataFrame,                // _piidDispinterface
            0,                              // _lAccRole
            &CSite::s_PropCats,             // _pPropCats;
            NULL
        },
        NULL,                              // _pfnTearOff
    },
    ETAG_DATAFRAME,                       // _st
};


const CLSID * CRootDataFrame::s_apclsidPages[] =
{
    &CLSID_CCDGenericPropertyPage,
    NULL
};

CSite::CLASSDESC CRootDataFrame::s_classdesc =
{
    {
        {
            &CLSID_CDataFrame,              // _pclsid
            IDR_BASE_FRAME,                 // _idrBase
            s_apclsidPages,                 // _apClsidPages
            ARRAY_SIZE(s_acpi),             // _ccp
            s_acpi,                         // _pcpi
#if PRODUCT_97
            ARRAY_SIZE(super::s_apropdesc), // _cpropdesc
            super::s_apropdesc,             // _ppropdesc
#else
            0,
            0,
#endif PRODUCT_97
            SITEDESC_PARENT |              // _dwFlags
            SITEDESC_BASEFRAME |
            SITEDESC_DATAFRAME |
            SITEDESC_TEMPLATE,
            &IID_IDataFrame,                // _piidDispinterface
            0,                              // _lAccRole
            &CSite::s_PropCats,             // _pPropCats;
            NULL
        },
        s_apfnIControlElement,                 // _pfnTearOff
    },
    ETAG_ROOTDATAFRAME,                   // _st
};




CSite *
CreateRootDataFrame(CSite * pParent)
{
    return new CRootDataFrame(pParent->_pDoc, pParent);
}




CRootDataFrame::CRootDataFrame(CDoc * pDoc, CSite * pParent)
    : super(pDoc, pParent)
{
#if PRODUCT_97
    Assert(_pBindSource == NULL);
#endif

}





//+---------------------------------------------------------------------------
//
//  Member:     Detach
//
//  Synopsis:   Detaches the layout template from the template hierarchy.
//
//----------------------------------------------------------------------------

void
CRootDataFrame::Detach ()
{
    TraceTag((tagRootDataFrame, "CRootDataFrame::Detach"));

    Assert(_fOwnTBag);

    //  Discard the selection tree
    delete _pSelectionRoot;
    _pSelectionRoot = NULL;

    // we have to remove the instances first
    DeleteInstances();
    super::Detach();

}

//+---------------------------------------------------------------------------
//
//  Member:     CreateInstance
//
//  Synopsis:   Create and instance of the template
//
//  Arguments:  pDataDoc    Form
//              pParent     parent instance
//              ppFrame     pointer to frame to be returned
//              info        create info
//
//
//  Returns:    Returns S_OK if everything is fine,
//
//----------------------------------------------------------------------------

HRESULT CRootDataFrame::CreateInstance (
    CDoc * pDoc,
    CSite * pParent,
    OUT CSite * *ppFrame,
    CCreateInfo * pcinfo)
{
    TraceTag((tagRootDataFrame, "CDataFrame::CreateInstance"));


    HRESULT hr;

#ifdef LATER
    //
    // We can't do this check yet because we're stripping the strings from
    // the PROP_DESCs. At the point where they're added back in then we can
    // re-enable this check.
    //
#if DBG == 1 && (PRODUCT_97)
    //
    // BUGBUG -- This may not be where we want to do this check.
    //
    LPTSTR aszKnownDiff[] = { _T("Position"),  _T("CLSIDCacheIndex"),
                              _T("RowSource"), _T("ControlSource"), NULL };
    // BUGBUG: I don't know iff using aszKnownDiff is the right way to handle
    //         the RowSource and ControlSource properties which only exist
    //         on COleSiteConcrete (not COleSite nor CDataFrameTemplate.)
    //         - TedSmith

    AssertPropDescs("CSite",
                    COleSiteConcrete::s_classdesc._classdescBase._ppropdesc,
                    COleSiteConcrete::s_classdesc._classdescBase._cpropdesc,
                    "CDataFrameTemplate",
                    CDataFrameTemplate::s_classdesc._classdescBase._ppropdesc,
                    CDataFrameTemplate::s_classdesc._classdescBase._cpropdesc,
                    aszKnownDiff
                   );
#endif
#endif


    // here we just use this template as an instance...

    CDataFrame * pdfr = this;


// BUGBUG - The following code caused a memory leak with the morphable data control.
/// InitInstance ended up calling CBase::Init and wiping out the property notify sink
// connection between thr MDC and the dataframe.I showed to Istvan and he agrees that
// the rootsite shouldn't be cloned.  Someone should revisit this code.  <DavidSch>

#ifdef NEVER
    hr = pdfr->InitInstance ();
    if (hr)
        goto Cleanup;
#endif

    hr = BuildInstance (pdfr, pcinfo);
    if (hr)
        goto Cleanup;

Cleanup:

    RRETURN (hr);
}


//+---------------------------------------------------------------------------
//
//  Member      CRootDataFrame::BuildDetail
//
//  Synopsis    Build detail section in instance
//              recursive (not directly, it doesn't call itself, but it calls
//              create, which will call Build).
//
//  Arguments   pNewInstance        the newly (just) created instance.
//              info                Procreation info structure, contains binding
//                                  information.
//
//  Returns     HRESULT
//
//  NOTE        this pointer here is always a TEMPLATE.
//
//----------------------------------------------------------------------------

HRESULT
CRootDataFrame::BuildDetail (CDataFrame *pNewInstance, CCreateInfo * pcinfo)
{
    TraceTag((tagRootDataFrame,"CDataFrame::BuildDetail "));
    HRESULT hr = S_OK;

    Assert(this == getTemplate() && _fOwnTBag);
    Assert(pNewInstance);

    if (IsRepeated())
    {
        // hide detail section
        _pDetail->CBaseFrame::SetVisible(FALSE);    // BUGBUG we have to do the reverse when repeated changes
        // create repeater for detail
        hr = super::BuildDetail(pNewInstance, pcinfo);
    }
    else
    {
        hr = _pDetail->BuildInstance(_pDetail, pcinfo);
    }

    RRETURN(hr);
}



//+---------------------------------------------------------------------------
//
//  Member      CDataFrame::InitSubFrameInstance
//
//  Synopsis    Creates and hooks up the instancee for a subframe
//              Detail, Header, Footer - the rootframe just assigns
//              the templates to do that
//
//  Arguments   pNewInstance  The new dataframe instance
//              pTemplate       the template to create an instance from
//
//  Returns     nothing
//
//  NOTE        this pointer here is always a TEMPLATE.
//
//----------------------------------------------------------------------------
HRESULT CRootDataFrame::InitSubFrameInstance(CBaseFrame **ppNewInstance, CBaseFrame *pTemplate, CCreateInfo * pcinfo)
{

    pTemplate->CBaseFrame::SetVisible(TRUE);    // BUGBUG we have to do the reverse when repeated changes
    *ppNewInstance = pTemplate;

    return S_OK;
}



//+---------------------------------------------------------------------------
//
//  Member      DeleteInstances
//
//  Synopsis    Remove and delete all instances from the object, reverts
//              back to the original (right after instantiation) state
//
//----------------------------------------------------------------------------

void
CRootDataFrame::DeleteInstances()
{
    Assert(_pDetail);
    if (_pDetail && !_pDetail->TestClassFlag(SITEDESC_TEMPLATE))
    {
        Assert(IsRepeated());

        ClearSelection(FALSE);
        TBag()->_suAnchor.Clear();

        CBaseFrame * pTemp   = _pDetail;
        _pDetail = _pDetail->getTemplate();
        DeleteSite(pTemp, DELSITE_NOTABADJUST | DELSITE_NOREGENERATE);
        Assert(_pDetail->TestClassFlag(SITEDESC_DETAILFRAME));
        Assert(_pDetail->TestClassFlag(SITEDESC_TEMPLATE));
        ((CDetailFrame *)_pDetail)->EmptyRecycle();   // BUGBUG we don't always want to do this !
    }
    if (_pHeader && !_pHeader->TestClassFlag(SITEDESC_TEMPLATE))
    {
        CBaseFrame * pTemp   = _pHeader;
        _pHeader = _pHeader->getTemplate();
        DeleteSite(pTemp, DELSITE_NOTABADJUST | DELSITE_NOREGENERATE);
        Assert(_pHeader->TestClassFlag(SITEDESC_HEADERFRAME));
        Assert(_pHeader->TestClassFlag(SITEDESC_TEMPLATE));
        ((CHeaderFrame *)_pHeader)->EmptyRecycle();   // BUGBUG we don't always want to do this !
    }
    #if defined(PRODUCT_97)
    if (_pFooter && !_pFooter->TestClassFlag(SITEDESC_TEMPLATE))
    {
        CBaseFrame * pTemp   = _pFooter;
        _pFooter = _pFooter->getTemplate();
        DeleteSite(pTemp, DELSITE_NOTABADJUST | DELSITE_NOREGENERATE);
        Assert(_pFooter->TestClassFlag(SITEDESC_HEADERFRAME));
        Assert(_pFooter->TestClassFlag(SITEDESC_TEMPLATE));
        ((CHeaderFrame *)_pFooter)->EmptyRecycle();   // BUGBUG we don't always want to do this !
    }
    #endif
}




//+---------------------------------------------------------------------------
//
//  Member:     Generate
//
//  Synopsis:   Generate frame instances.
//
//  Purpose:    Generate method is only called for the root layout.
//              This is the ONLY method that DataDoc should call to generate
//              all the instances.
//              This routine should pick up the root of the selection tree.
//
//  Arguments:  rclBound            generate frames in this bound rectangle.
//              fAfterLoad          TRUE if called directly after loading
//              BUGBUG: remove that flag, useless
//
//  Returns:    Returns S_OK if everything is fine,
//              E_INVALIDARG if parameters are NULL.
//
//----------------------------------------------------------------------------

HRESULT
CRootDataFrame::Generate (IN CRectl& rclBound, BOOL fAfterLoad)
{
    TraceTag((tagRootDataFrame, "CRootDataFrame::Generate"));



    HRESULT     hr = S_OK;
    CCreateInfo info;       // default contructor sets _pBindSource, _bindId to 0.

    if (!fAfterLoad)
    {
        DeleteInstances();
    }

    hr = CreateInstance (_pDoc, _pParent, NULL, &info);

    if (SUCCEEDED(hr) && (IsAutoSizeBelow () || IsRepeaterBelow ()))
    {
        CRectl rclView;
        // since it could be that we are in deferred update mode let's check
        // if we want to call CalcProposedPositions
#if TEST
        GetProposed(this, &rclView);
        if (_fProposedSet && rclView.IsRectEmpty())
#else
        if (_fProposedSet)
#endif
        {
            CalcProposedPositions();
            MoveToProposed(SITEMOVE_POPULATEDIR | SITEMOVE_NOFIREEVENT | SITEMOVE_NOINVALIDATE);
        }
        GetClientRectl (&rclView);      // BUGBUG: just for now.
        hr = CreateToFit (&rclView, SITEMOVE_POPULATEDIR | SITEMOVE_NOINVALIDATE);
        Invalidate(NULL, 0);
        if (SUCCEEDED(hr))
        {
            hr = S_OK;
        }
#if NEVER
  //    BUGBUG: We'll need to
  //    1. send a deferselect event
  //    2. select the dataframe instead of the detail.
        if (!hr)
        {
            PostMessage(
                    _pDoc->_pInPlace->_hwnd,
                    WM_DEFERSELECT,
                    0,
                    (LPARAM) _pDetail->TBag()->_ID);
        }
#endif
    }

    CHECKPROPOSEDFLAG(_pDoc);

    RRETURN(hr);
}
//-+ End of Method-------------------------------------------------------------




//+---------------------------------------------------------------------------
//
//  Function:   ClearSelection
//
//  Synopsis:   discards the selection tree rooted at the object's
//              parent RootFrame
//
//  Arguments:  pSite:  the frame it is called from. Used to locate
//                      the root of the dataframe hierarchy
//
//              fResetInstances:    TRUE if the instance back-pointers
//                                  have to be reset immediately
//---------------------------------------------------------------------------
void
CRootDataFrame::ClearSelection(BOOL fResetInstances)
{
    if (_pSelectionRoot)
    {
        delete _pSelectionRoot;
        _pSelectionRoot = NULL;

        // remove back pointers from instances

        //  BUGBUG: This needs to be replaced with an optimized SelectSite()
        if ( fResetInstances && _TBag._eListBoxStyle &&
            _pDetail->TestClassFlag(SITEDESC_REPEATER) )
        {
            ((CRepeaterFrame*)_pDetail)->DeselectListboxLines();
        }
    }
}



#if DBG == 1
//+-------------------------------------------------------------------------
//
//  Method:     CRootDataFrame::SetProposed
//
//  Synopsis:   Deferred Move to this rectangle
//
//  Returns:    HRESULT
//
//--------------------------------------------------------------------------

HRESULT
CRootDataFrame::SetProposed(CSite * pSite, const CRectl * prcl)
{
    Assert(prcl);

    TraceTag((tagPropose, "%ls/%d CRootDataFrame::SetProposed %d,%d,%d,%d to %d,%d,%d,%d",
        TBag()->_cstrName, TBag()->_ID,
        prcl->left - _rcl.left, prcl->top - _rcl.top, prcl->right - _rcl.right, prcl->bottom - _rcl.bottom,
        prcl->left, prcl->top, prcl->right, prcl->bottom));

    return super::SetProposed(pSite, prcl);
}

//+-------------------------------------------------------------------------
//
//  Method:     CRootDataFrame::GetProposed
//
//  Synopsis:   Deferred Move to this rectangle
//
//  Returns:    HRESULT
//
//--------------------------------------------------------------------------

HRESULT
CRootDataFrame::GetProposed(CSite * pSite, CRectl * prcl)
{
    HRESULT hr;

    Assert(prcl);

    hr = super::GetProposed(pSite, prcl);

    TraceTag((tagPropose, "%ls/%d CRootDataFrame::GetProposed %d,%d,%d,%d to %d,%d,%d,%d",
        TBag()->_cstrName, TBag()->_ID,
        prcl->left - _rcl.left, prcl->top - _rcl.top, prcl->right - _rcl.right, prcl->bottom - _rcl.bottom,
        prcl->left, prcl->top, prcl->right, prcl->bottom));

    RRETURN(hr);
}

#endif DBG






//+---------------------------------------------------------------------------
//
//  Member:     CDataFrame:::UpdatePropertyChanges, public
//
//  Synopsis:   Walks over the dataframe and it's children.
//              if the template is dirty, updates the properies on the instances
//
//  Returns:    HRESULT
//
//
//----------------------------------------------------------------------------
HRESULT CRootDataFrame::UpdatePropertyChanges(UPDATEPROPS updFlag)
{
    HRESULT hr= S_OK;
    CRectl rclForm(0, 0, _pDoc->_sizel.cx, _pDoc->_sizel.cy);

    TraceTag((tagRootDataFrame, "CRootDataFrame::UpdatePropertyChanges"));

    Assert(updFlag == UpdatePropsPrepareTemplates);

    Assert(_fOwnTBag);



    if (!_pDoc->_fInstancePropagating)
    {
        _pDoc->_fInstancePropagating = TRUE;
        hr = super::UpdatePropertyChanges(UpdatePropsPrepareTemplates);
        if (hr)
            goto Cleanup;

        // now we got to walk the TBag chains and clear the propertychange bags
        if (TBag()->_propertyChanges.NeedRegenerate())
        {
            // first empty the change bags
            if (TBag()->_propertyChanges.IsDataSourceModified())
            {
                DataSourceChanged();
                hr = super::UpdatePropertyChanges(UpdatePropsPrepareDataBase);
                if (hr)
                    goto Cleanup;
            }

            hr = super::UpdatePropertyChanges(UpdatePropsEmptyBags);
            if (hr)
                goto Cleanup;

            hr = Generate(rclForm);
            if (hr)
                goto Cleanup;

        }
        else if (TBag()->_propertyChanges.NeedCreateToFit())
        {
            hr = super::UpdatePropertyChanges(UpdatePropsEmptyBags);

            if (SUCCEEDED(hr))
            {
                CRectl rclBound(0, 0, _pDoc->_sizel.cx, _pDoc->_sizel.cy);
                rclBound += _rcl.TopLeft(); // BUGBUG: repeater would go to top left...
                rclBound.bottom -= _rcl.top;
                rclBound.right  -= _rcl.left;
                hr = CalcProposedPositions();
                if (hr)
                    goto Cleanup;

                hr = MoveToProposed(SITEMOVE_NOFIREEVENT | SITEMOVE_NOINVALIDATE);
                if (hr)
                    goto Cleanup;

                hr = CreateToFit (&rclBound, SITEMOVE_POPULATEDIR);
            }
        }
        else
        {
            // just empty the change bags
            hr = super::UpdatePropertyChanges(UpdatePropsEmptyBags);
        }

        hr = super::UpdatePropertyChanges(UpdatePropsCloseAction);
        _pDoc->_fInstancePropagating = FALSE;
    }

    CHECKPROPOSEDFLAG(_pDoc);

Cleanup:
    // only reset the calculation flag if we are not in
    // defered property update...
    if (!_pDoc->_fDeferedPropertyUpdate)
        TBag()->_fMinMaxPropertyChanged = FALSE;
    if (hr)
    {
        PutErrorInfoText(EPART_ACTION, IDS_ACT_UPDATEPROPS);
    }

    RRETURN(hr);
}
//-+ End of Method-------------------------------------------------------------



//+---------------------------------------------------------------
//
//  Member:     CDetailFrame::Notify
//
//  Synopsis:   Handle notification
//
//---------------------------------------------------------------
HRESULT
CRootDataFrame::Notify(SITE_NOTIFICATION sn, DWORD dw)
{
    HRESULT hr = S_OK;

#if defined(PRODUCT_97)
    switch (sn)
    {
    case SN_AFTERLOAD:
        hr = AfterLoad();
        hr = super::Notify(sn, dw);
        break;

    default:
        hr = THR(super::Notify(sn, dw));
    }
#endif

    RRETURN1(hr, S_FALSE);
}


#if PRODUCT_97
//+---------------------------------------------------------------------------
//
//  Member:     CRootDataFrame::AfterLoad
//
//  Synopsis:   Fix things up after loading the form.
//
//----------------------------------------------------------------------------

HRESULT
CRootDataFrame::AfterLoad(DWORD dw)
{
    HRESULT hr = S_OK;

    CRectl rclForm;

    TraceTag((tagRootDataFrame, "CRootDataFrame::UpdatePropertyChanges"));

    Assert(_fOwnTBag);

    IGNORE_HR(_pDetail->GetTemplate()->AfterLoad(dw));
    if (_pHeader)
    {
        IGNORE_HR(_pHeader->AfterLoad(dw));
    }
    if (_pFooter)
    {
        IGNORE_HR(_pFooter->AfterLoad(dw));
    }

    _pDoc->_fInstancePropagating = TRUE;
    hr = super::UpdatePropertyChanges(UpdatePropsPrepareTemplates);
    if (hr)
        goto Error;

       // first empty the change bags
    hr = super::UpdatePropertyChanges(UpdatePropsPrepareDataBase);
    if (hr)
        goto Error;

    hr = super::UpdatePropertyChanges(UpdatePropsEmptyBags);
    if (hr)
        goto Error;

    // I don't need this rectangle to be initialized
    //  in case of Generate(..,, TRUE) the rectangle
    //  is not used.

    hr = Generate(rclForm, TRUE);
    if (hr)
        goto Error;

    _pDoc->_fInstancePropagating = FALSE;

    hr = super::UpdatePropertyChanges(UpdatePropsCloseAction);
    if (hr)
        goto Error;

Cleanup:

    return super::AfterLoad(dw);

Error:
    ConstructErrorInfo(hr,IDS_ACT_DDOCAFTERLOAD);
    goto Cleanup;
}
//-+ End of Method-------------------------------------------------------------

#endif // PRODUCT_97

