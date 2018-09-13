/*
 *  @doc INTERNAL
 *
 *  @module QUXCOPY.CXX - Implement the CQuickUnixCopy 
 *
 *      This module implements the methods for the Unix middle button
 *      copy functionality.
 *
 *  Authors: <nl>
 *      David Dawson <nl>
 *
 *  Revisions: <nl>
 */

#ifdef UNIX
#include "headers.hxx"
#include "formkrnl.hxx"
#include "txtdefs.h"
#include "quxcopy.hxx"

// BUGBUG: Should be in mainwin.h
extern "C" void MwSelectionDone(HWND hWndSelectionOwner);


CQuickCopyBuffer g_uxQuickCopyBuffer;


CQuickCopyBuffer::~CQuickCopyBuffer()
{
    ClearSelection();
}

HRESULT 
CQuickCopyBuffer::ClearSelection()
{
    HRESULT hr = S_OK;
    
    if ( _hLastCopiedString ) 
    {
        GlobalFree( _hLastCopiedString );
        _hLastCopiedString = NULL;
    }

    RRETURN (hr);
}


HRESULT
CQuickCopyBuffer::NewTextSelection( CDoc *pDoc )
{
    HRESULT hr = S_OK;

    ClearSelection();
    MwSelectionDone( pDoc->_pInPlace->_hwnd );

    RRETURN( hr );
}


HRESULT 
CQuickCopyBuffer::GetTextSelection(
    HGLOBAL     hszText, 
    BOOL        bUnicode, 
    VARIANTARG *pvarCopiedTextHandle 
    )
{
    HRESULT hr = S_OK;

    if ( hszText == NULL ) {
        // Grab last contents if they exist.
        if ( _hLastCopiedString ) {
            goto Copy;
        }

        hr = S_OK;
        goto Cleanup;
    }

    ClearSelection();

    if (bUnicode) {
        _hLastCopiedString = TextHGlobalWtoA( hszText );
    } else {
        _hLastCopiedString = DuplicateHGlobal( hszText );
    }

Copy:

    if (pvarCopiedTextHandle)
    {
        Assert( VT_EMPTY == V_VT(pvarCopiedTextHandle));
        V_VT( pvarCopiedTextHandle ) = VT_BYREF;
        V_BYREF( pvarCopiedTextHandle ) = DuplicateHGlobal( _hLastCopiedString );
    }

Cleanup:

    RRETURN(hr);
}

#endif // UNIX

