//+------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1998.
//
//  File:       MSHTMLED.CXX
//
//  Contents:   Implementation of Mshtml Editing Component
//
//  Classes:    CMshtmlEd
//
//  History:    7-Jan-98   raminh  Created
//             12-Mar-98   raminh  Converted over to use ATL
//-------------------------------------------------------------------------
#include "headers.hxx"

#ifndef X_STDAFX_H_
#define X_STDAFX_H_
#include "stdafx.h"
#endif

#ifndef X_OptsHold_H_
#define X_OptsHold_H_
#include "optshold.h"
#endif

#ifndef X_COREGUID_H_
#define X_COREGUID_H_
#include "coreguid.h"
#endif

#ifndef X_MSHTMHST_H_
#define X_MSHTMHST_H_
#include "mshtmhst.h"
#endif

#ifndef _X_HTMLED_HXX_
#define _X_HTMLED_HXX_
#include "htmled.hxx"
#endif

#ifndef _X_EDUTIL_HXX_
#define _X_EDUTIL_HXX_
#include "edutil.hxx"
#endif

#ifndef _X_EDCMD_HXX_
#define _X_EDCMD_HXX_
#include "edcmd.hxx"
#endif

#ifndef _X_BLOCKCMD_HXX_
#define _X_BLOCKCMD_HXX_
#include "blockcmd.hxx"
#endif

#ifndef _X_CHARCMD_HXX_
#define _X_CHARCMD_HXX_
#include "charcmd.hxx"
#endif
#ifndef _X_INSCMD_HXX_
#define _X_INSCMD_HXX_
#include "inscmd.hxx"
#endif
#ifndef _X_DELCMD_HXX_
#define _X_DELCMD_HXX_
#include "delcmd.hxx"
#endif
#ifndef _X_DLGCMD_HXX_
#define _X_DLGCMD_HXX_
#include "dlgcmd.hxx"
#endif

#ifndef X_MSHTMLED_HXX_
#define X_MSHTMLED_HXX_
#include "mshtmled.hxx"
#endif

MtDefine(CMshtmlEd, Utilities, "CMshtmlEd")

extern HRESULT      InsertObject (UINT cmdID, LPTSTR pstrParam, IHTMLTxtRange * pRange, HWND hwnd);
extern HRESULT      ShowEditDialog(UINT idm, VARIANT * pvarExecArgIn, HWND hwndParent, VARIANT * pvarArgReturn);

extern "C" const GUID CGID_EditStateCommands;

DeclareTag(tagEditingTrackQueryStatusFailures, "Edit", "Track query status failures")

//+---------------------------------------------------------------------------
//
//  CMshtmlEd Constructor
//
//----------------------------------------------------------------------------
CMshtmlEd::CMshtmlEd( CHTMLEditor * pEd )
                        : _sl(this)
{ 
    _pEd = pEd;
    _pContext = NULL;
}


CMshtmlEd::~CMshtmlEd()
{
//  delete _psl;
}


HRESULT
CMshtmlEd::Initialize( IUnknown * pContext )
{
    _pContext = pContext;
    return(S_OK);
}    


//////////////////////////////////////////////////////////////////////////
//
//  Public Interface CCaret::IUnknown's Implementation
//
//////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(ULONG)
CMshtmlEd::AddRef( void )
{
    return( ++_cRef );
}


STDMETHODIMP_(ULONG)
CMshtmlEd::Release( void )
{
    --_cRef;

    if( 0 == _cRef )
    {
        delete this;
        return 0;
    }

    return _cRef;
}


STDMETHODIMP
CMshtmlEd::QueryInterface(
    REFIID              iid, 
    LPVOID *            ppv )
{
    if (!ppv)
        RRETURN(E_INVALIDARG);

    *ppv = NULL;
    
    if( iid == IID_IUnknown )
    {
        *ppv = static_cast< IUnknown * >( this );
    }
    else if( iid == IID_IOleCommandTarget )
    {
        *ppv = static_cast< IOleCommandTarget * >( this );
    }

    if (*ppv)
    {
        ((IUnknown *)*ppv)->AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
    
}

BOOL
CMshtmlEd::IsDialogCommand(DWORD nCmdexecopt, DWORD nCmdID, VARIANT *pvarargIn)
{
    BOOL bResult = FALSE;
    
    if (nCmdID == IDM_HYPERLINK)
    {
        bResult = (pvarargIn == NULL) || (nCmdexecopt != OLECMDEXECOPT_DONTPROMPTUSER);
    }
    else if (nCmdID == IDM_IMAGE || nCmdID == IDM_FONT)
    {
        bResult = (nCmdexecopt != OLECMDEXECOPT_DONTPROMPTUSER);
    }
    
    return bResult;
}


//+---------------------------------------------------------------------------
//
//  CMshtmlEd IOleCommandTarget Implementation for Exec() 
//
//----------------------------------------------------------------------------
STDMETHODIMP
CMshtmlEd::Exec( const GUID *       pguidCmdGroup,
                       DWORD        nCmdID,
                       DWORD        nCmdexecopt,
                       VARIANTARG * pvarargIn,
                       VARIANTARG * pvarargOut)
{
    HRESULT hr = OLECMDERR_E_NOTSUPPORTED;
    CCommand* theCommand = NULL;

    // ShowHelp is not implemented 
    if(nCmdexecopt == OLECMDEXECOPT_SHOWHELP)
        goto Cleanup;

    Assert( *pguidCmdGroup == CGID_MSHTML );
    Assert( _pEd );
    Assert( _pEd->GetDoc() );
    Assert( _pContext );

#if 0
    hr = THR(ControlRangeExec(nCmdID, nCmdexecopt, pvarargIn, pvarargOut));
    if (hr != S_FALSE)
        goto Cleanup;
#endif
    if (IsDialogCommand(nCmdexecopt, nCmdID, pvarargIn) )
    {
        // Special case for image or hyperlink or font dialogs
        theCommand = _pEd->GetCommandTable()->Get( ~nCmdID );    
    }
    else
    {
        theCommand = _pEd->GetCommandTable()->Get( nCmdID );
    }

    if ( theCommand )
    {
        hr = theCommand->Exec( nCmdexecopt, pvarargIn, pvarargOut, this );
    }
    else
    {
        hr = OLECMDERR_E_NOTSUPPORTED;
    }

Cleanup:
    RRETURN( hr );
}

//+---------------------------------------------------------------------------
//
//  CMshtmlEd IOleCommandTarget Implementation for QueryStatus()
//
//  Note: QueryStatus() is still being handled by Trident
//----------------------------------------------------------------------------
STDMETHODIMP
CMshtmlEd::QueryStatus(
        const GUID * pguidCmdGroup,
        ULONG cCmds,
        OLECMD rgCmds[],
        OLECMDTEXT * pcmdtext)
{
    HRESULT             hr = OLECMDERR_E_NOTSUPPORTED ;
    CCommand  *         theCommand = NULL;
    OLECMD  *           pCmd = &rgCmds[0];

    Assert( *pguidCmdGroup == CGID_MSHTML );
    Assert( _pEd );
    Assert( _pEd->GetDoc() );
    Assert( _pContext );

#if 0
    hr = THR(ControlRangeQueryStatus(cCmds, rgCmds, pcmdtext));
    if (hr != S_FALSE)
        goto Cleanup;
#endif        
    // BUGBUG: The dialog commands are hacked with strange tagId's.  So, for now we just
    // make sure the right command gets the query status [ashrafm]
    
    if (pCmd->cmdID == IDM_FONT)
    {
        theCommand = _pEd->GetCommandTable()->Get( ~(pCmd->cmdID)  );
    }
    else
    {
        theCommand = _pEd->GetCommandTable()->Get( pCmd->cmdID  );
    }
    
    if (theCommand )
    {
        hr = theCommand->QueryStatus( pCmd, pcmdtext, this );                      
    }
    else 
    {
        hr = OLECMDERR_E_NOTSUPPORTED;
    }

#if DBG==1
    if (IsTagEnabled(tagEditingTrackQueryStatusFailures))
    {
        if (FAILED(hr))
        {
            CHAR szBuf[1000];

            wsprintfA(szBuf, "CMshtmlEd::QueryStatus failed: nCmdId=%d", pCmd->cmdID);
            AssertSz(0, szBuf);
        }
    }
#endif

    RRETURN ( hr ) ;

}


HRESULT 
CMshtmlEd::GetSegmentList( 
    ISegmentList**      ppSegmentList ) 
{ 
    HRESULT hr = S_OK;
    ISegmentList * pOut = NULL;
    hr = THR( _pContext->QueryInterface( IID_ISegmentList , (void **) &pOut ));
    *ppSegmentList = pOut;
    RRETURN( hr );
}

#if 0

HRESULT 
CMshtmlEd::ControlRangeExec( 
    DWORD                    cmdId,
    DWORD                    nCmdexecopt,
    VARIANTARG *             pvarargIn,
    VARIANTARG *             pvarargOut)
{
    HRESULT                 hr;
    SP_ISegmentList         spSegmentList;
    SELECTION_TYPE          eSelectionType;
    INT                     iSegmentCount;
    INT                     i;
    SP_IHTMLElement         spElement;        
    SP_IOleCommandTarget    spCommandTarget;
    OLECMD                  cmd;

    cmd.cmdf = 0;
   
    IFC( ControlRangeQueryStatus(1,  &cmd, NULL) );
    if (!(cmd.cmdf & MSOCMDF_SUPPORTED))
        return S_FALSE;
    
    IFC( GetSegmentList(&spSegmentList) );
    IFC( spSegmentList->GetSegmentCount(&iSegmentCount, &eSelectionType) );

    //
    // For control range/site selection - give the element a shot first
    //

    if (eSelectionType == SELECTION_TYPE_Control)
    {
        for (i = 0; i < iSegmentCount; i += 1)
        {
            IFC( GetEditor()->GetSegmentElement(spSegmentList, i, &spElement) );

            // Give the element the first shot at handling the command
            IFC( spElement->QueryInterface(IID_IOleCommandTarget, (LPVOID *)&spCommandTarget) );
            IFC( spCommandTarget->Exec(&CGID_MSHTML, cmdId, nCmdexecopt, pvarargIn, pvarargOut) );
        }

        return S_OK;
    }

Cleanup:
    return S_FALSE;
}    

HRESULT 
CMshtmlEd::ControlRangeQueryStatus( 
    UINT          cCmds,
    OLECMD        rgCmds[],
    OLECMDTEXT *  pcmdtext)
{
    HRESULT                 hr;
    SP_ISegmentList         spSegmentList;
    SELECTION_TYPE          eSelectionType;
    INT                     iSegmentCount;
    INT                     i;
    SP_IHTMLElement         spElement;        
    SP_IOleCommandTarget    spCommandTarget;

    IFC( GetSegmentList(&spSegmentList) );
    IFC( spSegmentList->GetSegmentCount(&iSegmentCount, &eSelectionType) );

    //
    // For control range/site selection - give the element a shot first
    //

    if (eSelectionType == SELECTION_TYPE_Control)
    {
        for (i = 0; i < iSegmentCount; i += 1)
        {
            IFC( GetEditor()->GetSegmentElement(spSegmentList, i, &spElement) );

            // Give the element the first shot at handling the command
            IFC( spElement->QueryInterface(IID_IOleCommandTarget, (LPVOID *)&spCommandTarget) );
            IFC( spCommandTarget->QueryStatus(&CGID_MSHTML, cCmds, rgCmds, pcmdtext) ); // BUGBUG: this is changing the value of pCmd->cmdID
            
            for (UINT j = 0; j < cCmds; j++)
            {
                if (!(rgCmds[j].cmdf & MSOCMDF_SUPPORTED))
                    return S_FALSE;
            }                    
        }

        return S_OK;
    }

Cleanup:
    return S_FALSE;
}    

#endif // if 0

