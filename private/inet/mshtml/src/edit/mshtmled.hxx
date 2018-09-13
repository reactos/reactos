//+---------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1998.
//
//  Class:      CMshtmlEd
//
//  Contents:   Definition of CMshtmlEd interfaces. This class is used to 
//                dispatch IOleCommandTarget commands to Tridnet range objects.
//
//  History:    7-Jan-98   raminh  Created
//----------------------------------------------------------------------------
#ifndef _MSHTMLED_HXX_
#define _MSHTMLED_HXX_ 1

#ifndef X_RESOURCE_H_
#define X_RESOURCE_H
#include "resource.h"    
#endif

#ifndef X_SLOAD_HXX_
#define X_SLOAD_HXX_
#include "sload.hxx"
#endif

class CSpringLoader;
class CHTMLEditor;


MtExtern(CMshtmlEd)

class CMshtmlEd : 
    public IOleCommandTarget
{
public:

    CMshtmlEd( CHTMLEditor * pEd ) ;
    ~CMshtmlEd();

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CMshtmlEd))

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


    // --------------------------------------------------
    // IOleCommandTarget methods
    // --------------------------------------------------

    STDMETHOD (QueryStatus)(
        const GUID * pguidCmdGroup,
        ULONG cCmds,
        OLECMD rgCmds[],
        OLECMDTEXT * pcmdtext);

    STDMETHOD (Exec)(
        const GUID * pguidCmdGroup,
        DWORD nCmdID,
        DWORD nCmdexecopt,
        VARIANTARG * pvarargIn,
        VARIANTARG * pvarargOut);


    HRESULT Initialize( IUnknown * pContext );

    CSpringLoader * GetSpringLoader() { return &_sl; }
    CHTMLEditor   * GetEditor()       { return _pEd; }
    HRESULT         GetSegmentList( ISegmentList** ppSegmentList ); 

private:
    CMshtmlEd() { }            // Protect the default constructor
    
    BOOL IsDialogCommand(DWORD nCmdexecopt, DWORD nCmdId, VARIANT *pvarargIn);

#if 0
    HRESULT 
    ControlRangeQueryStatus( 
        UINT          cCmds,
        OLECMD        rgCmds[],
        OLECMDTEXT *  pcmdtext);

    HRESULT
    ControlRangeExec(
        DWORD                    cmdId,
        DWORD                    nCmdexecopt,
        VARIANTARG *             pvarargIn,
        VARIANTARG *             pvarargOut);
#endif // if 0

    LONG            _cRef;
    CHTMLEditor   * _pEd;      // The editor that we work for
    IUnknown      * _pContext; // The segment list context of this command router
    CSpringLoader   _sl;       // The spring loader                                        
};

#endif //_MSHTMLED_HXX_
