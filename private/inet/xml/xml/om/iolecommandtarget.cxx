/*
 * @(#)IOleCommandTarget.cxx 1.0 11/18/97
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. * 
 */
#include "core.hxx"
#pragma hdrstop

#include "xml/om/iolecommandtarget.hxx"

HRESULT STDMETHODCALLTYPE IOleCommandTargetWrapper::QueryStatus( 
            /* [unique][in] */ const GUID __RPC_FAR *pguidCmdGroup,
            /* [in] */ ULONG cCmds,
            /* [out][in][size_is] */ OLECMD __RPC_FAR prgCmds[  ],
            /* [unique][out][in] */ OLECMDTEXT __RPC_FAR *pCmdText)
{
    STACK_ENTRY_WRAPPED;
    MutexLock lock(_pMutex);
    HRESULT hr = S_OK;

    TRY
    {
        getWrapped()->queryStatus(pguidCmdGroup, cCmds, prgCmds, pCmdText);
    }
    CATCH
    {
        hr = ERESULTINFO;
    }
    ENDTRY

    return hr;
}

HRESULT STDMETHODCALLTYPE IOleCommandTargetWrapper::Exec( 
            /* [unique][in] */ const GUID __RPC_FAR *pguidCmdGroup,
            /* [in] */ DWORD nCmdID,
            /* [in] */ DWORD nCmdexecopt,
            /* [unique][in] */ VARIANT __RPC_FAR *pvaIn,
            /* [unique][out][in] */ VARIANT __RPC_FAR *pvaOut)
{
    STACK_ENTRY_WRAPPED;

//  Locking is special for the abort case.  
//  It is the same as XMLDOMDocument::abort.
//    MutexLock lock(_pMutex);

    HRESULT hr = S_OK;
    TRY
    {
        // calls abort.
        getWrapped()->exec(pguidCmdGroup, nCmdID, nCmdexecopt, pvaIn, pvaOut);
    }
    CATCH
    {
        hr = ERESULTINFO;
    }
    ENDTRY

    return hr;
}

