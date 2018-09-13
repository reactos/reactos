//=============================================================
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1998
//
//  File:       ctlrange.hxx
//
//  Contents:   Implementation of the Control Range class.
//
//  Classes:    CAutoTxtSiteRange
//
//=============================================================

#ifndef I_CTLRANGE_HXX_
#define I_CTLRANGE_HXX_
#pragma INCMSG("--- Beg 'ctlrange.hxx'")

#ifndef X_LAYOUT_HXX_
#define X_LAYOUT_HXX_
#include "layout.hxx"
#endif

#define _hxx_               // interface defns
#include "siterang.hdl"

class CLayout;

MtExtern(CAutoTxtSiteRange)

//+----------------------------------------------------------------------------
//
//   Class : CAutoTxtSiteRange
//
//   Synopsis :
//
//+----------------------------------------------------------------------------


class CAutoTxtSiteRange : public  CBase,
                          public  IHTMLControlRange,
                          public  IDispatchEx,
                          public  ISegmentList
{
public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CAutoTxtSiteRange))

    // CTOR
    CAutoTxtSiteRange(CElement * pElementOwner);
    ~CAutoTxtSiteRange();
    
    // IUnknown and IDispatch
    DECLARE_PLAIN_IUNKNOWN(CAutoTxtSiteRange);
    DECLARE_DERIVED_DISPATCHEx2(CBase);
    DECLARE_PRIVATE_QI_FUNCS(CBase)
    DECLARE_TEAROFF_TABLE(IOleCommandTarget)

    // Need to support getting the atom table for expando support on IDEX2.
    virtual CAtomTable * GetAtomTable (BOOL *pfExpando = NULL)
        { if (pfExpando)
            {
                *pfExpando = _pElementOwner->Doc()->_fExpando;
            }
          return &_pElementOwner->Doc()->_AtomTable; }

    // ICommandTarget methods

    STDMETHOD(QueryStatus) (
            GUID * pguidCmdGroup,
            ULONG cCmds,
            MSOCMD rgCmds[],
            MSOCMDTEXT * pcmdtext);

    STDMETHOD(Exec) (
            GUID * pguidCmdGroup,
            DWORD nCmdID,
            DWORD nCmdexecopt,
            VARIANTARG * pvarargIn,
            VARIANTARG * pvarargOut);

    // ISegmentList methods
    STDMETHOD ( MovePointersToSegment ) ( 
        int iSegmentIndex, 
        IMarkupPointer * pStart, 
        IMarkupPointer * pEnd ) ;

    STDMETHOD( GetSegmentCount ) (
        int* piSegmentCount,
        SELECTION_TYPE * peType );

    // Automation Interface members
    #define _CAutoTxtSiteRange_
    #include "siterang.hdl"

    virtual HRESULT CloseErrorInfo(HRESULT hr);

    HRESULT AddElement( CElement* pElement );
    
    DECLARE_CLASSDESC_MEMBERS;

private:
    VOID QueryStatusSitesNeeded(MSOCMD *pCmd, INT cSitesNeeded);

    BOOL IsLoading ( ) { return _pElementOwner->Doc()->IsLoading(); }
    
    
    CEditRouter         _EditRouter;        // Edit Router to MshtmlEd
    CElement *          _pElementOwner;     // Site that owns this selection.
    CPtrAry<CElement*>  _aryElements;       // Array of Elements

};

#pragma INCMSG("--- End 'txtsrang.hxx'")
#else
#pragma INCMSG("*** Dup 'txtsrang.hxx'")
#endif
