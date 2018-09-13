//+------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1998, 1999
//
//  File:       CUTCMD.CXX
//
//  Contents:   Implementation of Cut command.
//
//  History:    07-14-98 - raminh - created
//
//-------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_HTMLED_HXX_
#define X_HTMLED_HXX_
#include "htmled.hxx"
#endif

#ifndef X_EDUTIL_HXX_
#define X_EDUTIL_HXX_
#include "edutil.hxx"
#endif

#ifndef _X_EDCMD_HXX_
#define _X_EDCMD_HXX_
#include "edcmd.hxx"
#endif

#ifndef _X_DELCMD_HXX_
#define _X_DELCMD_HXX_
#include "delcmd.hxx"
#endif

#ifndef _X_CUTCMD_HXX_
#define _X_CUTCMD_HXX_
#include "cutcmd.hxx"
#endif

#ifndef X_SELMAN_HXX_
#define X_SELMAN_HXX_
#include "selman.hxx"
#endif

using namespace EdUtil;

//
// Externs
//

MtDefine(CCutCommand, EditCommand, "CCutCommand");

//+---------------------------------------------------------------------------
//
//  CCutCommand::Exec
//
//----------------------------------------------------------------------------

HRESULT
CCutCommand::PrivateExec( 
    DWORD nCmdexecopt,
    VARIANTARG * pvarargIn,
    VARIANTARG * pvarargOut )
{
    HRESULT         hr = S_OK;
    IHTMLElement *  pElement = NULL;
    int             iSegmentCount;
    IMarkupPointer  * pStart = NULL;
    IMarkupPointer  * pEnd = NULL;
    SELECTION_TYPE  eSelectionType;
    IMarkupServices * pMarkupServices = GetMarkupServices();
    ISegmentList *  pSegmentList = NULL;
    BOOL            fRet;
    BOOL            fNotRange = TRUE;
    CUndoUnit       undoUnit(GetEditor());
    CHTMLEditor *   pEditor = GetEditor();

    ((IHTMLEditor *) pEditor)->AddRef();    // FireOnCancelableEvent can remove the whole doc

    //
    // Do the prep work
    //
    IFC( GetSegmentList( &pSegmentList ));    
    IFC( pSegmentList->GetSegmentCount( & iSegmentCount, & eSelectionType ) );           

    //
    // Cut is allowed iff we have a non-empty segment
    //    
    if ( eSelectionType == SELECTION_TYPE_Caret 
         || iSegmentCount == 0 )
    {
        goto Cleanup;
    }
    IFC( pMarkupServices->CreateMarkupPointer( & pStart ) );
    IFC( pMarkupServices->CreateMarkupPointer( & pEnd ) );   
    IFC( MovePointersToSegmentHelper( GetViewServices(), pSegmentList, 0, & pStart, & pEnd ) );
    IFC( pStart->IsEqualTo( pEnd, & fRet ) );
    if ( fRet )
    {
        goto Cleanup;
    }

    //
    // Cannot delete or cut unless the range is in the same flow layout
    //
    if (! PointersInSameFlowLayout( pStart, pEnd, NULL, GetViewServices() ) )
    {
        goto Cleanup;
    }

    //
    // Now Handle the cut 
    //
    IFC( undoUnit.Begin(IDS_EDUNDOCUT) );

    IFC(FindCommonElement(pMarkupServices, GetViewServices(), pStart, pEnd, &pElement));

    if (! pElement)
        goto Cleanup;

    IFC( GetViewServices()->FireCancelableEvent( 
            pElement,
            DISPID_EVMETH_ONCUT,
            DISPID_EVPROP_ONCUT,
            _T( "cut" ),
            & fRet ) );

    if (! fRet)
    {
        goto Cleanup;
    }

#ifndef UNIX
    IFC( GetViewServices()->SaveSegmentsToClipboard( pSegmentList ) );
#else
    IFC( GetViewServices()->SaveSegmentsToClipboard( pSegmentList, NULL ) );
#endif

    fNotRange = ( eSelectionType != SELECTION_TYPE_Auto && eSelectionType != SELECTION_TYPE_Control );
    IFC( pEditor->Delete( pStart, pEnd, fNotRange ) );

    if ( eSelectionType == SELECTION_TYPE_Selection) 
    {
        pEditor->GetSelectionManager()->EmptySelection();
    }

Cleanup:   
    ReleaseInterface( (IHTMLEditor *) pEditor );
    ReleaseInterface( pSegmentList );
    ReleaseInterface( pStart );
    ReleaseInterface( pEnd );
    ReleaseInterface( pElement );
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  CCutCommand::QueryStatus
//
//----------------------------------------------------------------------------

HRESULT
CCutCommand::PrivateQueryStatus( 
    OLECMD * pCmd,
    OLECMDTEXT * pcmdtext )
{
    HRESULT             hr = S_OK;
    INT                 iSegmentCount;
    IMarkupPointer *    pStart = NULL;
    IMarkupPointer *    pEnd = NULL;
    IHTMLElement   *    pElement = NULL;
    BOOL                fEditable;
    IMarkupServices   * pMarkupServices = GetMarkupServices();
    IHTMLViewServices * pViewServices = GetViewServices();
    ISegmentList      * pSegmentList = NULL;
    SELECTION_TYPE      eSelectionType;
    BOOL                fRet;

    pViewServices->AddRef();    // FireOnCancelableEvent can remove the whole doc

    // 
    // Status is disabled by default
    //
    pCmd->cmdf = MSOCMDSTATE_DISABLED;

    //
    // Get Segment list and selection type
    //
    IFC( GetSegmentList( &pSegmentList ));
    IFC( pSegmentList->GetSegmentCount( & iSegmentCount, & eSelectionType ) );

    //
    // Cut is allowed iff we have a non-empty segment
    //
    if ( eSelectionType == SELECTION_TYPE_Caret 
         || iSegmentCount == 0 )
    {
        goto Cleanup;
    }
    IFC( GetSegmentPointers(pSegmentList, 0, & pStart, & pEnd) );
    IFC( pStart->IsEqualTo( pEnd, & fRet ) );
    if ( fRet )
    {
        goto Cleanup;
    }

    //
    // Fire cancelable event
    //
    IFC( FindCommonElement(pMarkupServices, GetViewServices(), pStart, pEnd, & pElement) );
    if (! pElement) 
        goto Cleanup;

    IFC( pViewServices->FireCancelableEvent(
            pElement, 
            DISPID_EVMETH_ONBEFORECUT,
            DISPID_EVPROP_ONBEFORECUT,
            _T( "beforecut" ),
            & fRet) );

    if (! fRet)
    {
        pCmd->cmdf = MSOCMDSTATE_UP; 
        goto Cleanup;
    }

    if (eSelectionType != SELECTION_TYPE_Auto && eSelectionType != SELECTION_TYPE_Control)
    {
        IFC( pViewServices->IsEditableElement(pElement, & fEditable) );
        if ( (! fEditable) )
            goto Cleanup;
    }

    //
    // Cannot delete or cut unless the range is in the same flow layout
    //
    if ( PointersInSameFlowLayout( pStart, pEnd, NULL, pViewServices ) )
    {
        pCmd->cmdf = MSOCMDSTATE_UP; 
    }

Cleanup:
    ReleaseInterface(pViewServices);
    ReleaseInterface(pSegmentList);
    ReleaseInterface(pStart);
    ReleaseInterface(pEnd);
    ReleaseInterface(pElement);
    RRETURN(hr);
}

