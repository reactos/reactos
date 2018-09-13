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

#ifndef _X_COPYCMD_HXX_
#define _X_COPYCMD_HXX_
#include "copycmd.hxx"
#endif

using namespace EdUtil;

//
// Externs
//

MtDefine(CCopyCommand, EditCommand, "CCopyCommand");

HRESULT
CCopyCommand::PrivateExec( 
    DWORD nCmdexecopt,
    VARIANTARG * pvarargIn,
    VARIANTARG * pvarargOut )
{
    HRESULT         hr = S_OK;
    INT             iSegmentCount;
    ISegmentList *  pSegmentList = NULL;
    SELECTION_TYPE  eSelectionType;
    IMarkupPointer* pStart = NULL;
    IMarkupPointer* pEnd = NULL;
    CHTMLEditor *   pEditor = GetEditor();
    IHTMLElement *  pElement = NULL;
    BOOL            fRet;
    IMarkupServices * pMarkupServices = GetMarkupServices();

    //
    // Do the prep work
    //
    ((IHTMLEditor *) pEditor)->AddRef();    // FireOnCancelableEvent can remove the whole doc

    IFC( GetSegmentList( &pSegmentList ));

    IFC( pSegmentList->GetSegmentCount( & iSegmentCount, & eSelectionType ) );

    // 
    // If there is no segments we're done, it's a nogo
    //
    if (iSegmentCount == 0)
        goto Cleanup;

    //
    // Fire the canceable event
    //
    IFC( pMarkupServices->CreateMarkupPointer( & pStart ) );
    
    IFC( pMarkupServices->CreateMarkupPointer( & pEnd ) );

    IFC( MovePointersToSegmentHelper(GetViewServices(), pSegmentList, 0, & pStart, & pEnd) );

    IFC( FindCommonElement( pMarkupServices, GetViewServices(), pStart, pEnd, & pElement ) );

    hr = THR( GetViewServices()->FireCancelableEvent(
                pElement,
                DISPID_EVMETH_ONCOPY,
                DISPID_EVPROP_ONCOPY,
                _T("copy"),
                &fRet) );
    
    if (hr)
        goto Cleanup;

    if (!fRet)
        goto Cleanup;

    //
    // Do the actual copy
    //
#ifndef UNIX
    IFC( GetViewServices()->SaveSegmentsToClipboard( pSegmentList ) );
#else
    IFC( GetViewServices()->SaveSegmentsToClipboard( pSegmentList, pvarargOut ) );
#endif

Cleanup:
    ReleaseInterface( pElement );
    ReleaseInterface((IHTMLEditor *) pEditor);
    ReleaseInterface(pSegmentList);
    ReleaseInterface(pStart);
    ReleaseInterface(pEnd);

    RRETURN(hr);
}


HRESULT 
CCopyCommand::PrivateQueryStatus( OLECMD * pCmd,
                     OLECMDTEXT * pcmdtext )
{
    HRESULT           hr = S_OK;
    INT               iSegmentCount;

    IMarkupServices * pMarkupServices = GetMarkupServices();
    IHTMLViewServices * pViewServices = GetViewServices();
    ISegmentList *  pSegmentList = NULL;
    SELECTION_TYPE  eSelectionType;
    IMarkupPointer * pStart = NULL;
    IMarkupPointer * pEnd = NULL;
    IHTMLElement * pElement = NULL;
    BOOL fRet;

    //
    // Status is disabled by default
    //
    pCmd->cmdf = MSOCMDSTATE_DISABLED;

    // 
    // Get the segment list and selection type
    //
    hr = THR( GetSegmentList( &pSegmentList ));
    if( hr )
        goto Cleanup;

    hr = THR( pSegmentList->GetSegmentCount( & iSegmentCount, & eSelectionType ) );
    if (FAILED(hr))
        goto Cleanup;

    // 
    // If there is no segments we're done, it's a nogo
    //
    if (iSegmentCount == 0)
        goto Cleanup;

    hr = THR(GetSegmentPointers(pSegmentList, 0, &pStart, &pEnd));
    if (hr)
        goto Cleanup;

    hr = THR(FindCommonElement(pMarkupServices, GetViewServices(), pStart, pEnd, &pElement));
    if (hr)
        goto Cleanup;

    hr = THR(pViewServices->FireCancelableEvent(
        pElement,
        DISPID_EVMETH_ONBEFORECOPY,
        DISPID_EVPROP_ONBEFORECOPY,
        _T("beforecopy"),
        &fRet));
    if (hr)
        goto Cleanup;

    if (!fRet)
    {
        pCmd->cmdf = MSOCMDSTATE_UP; 
        goto Cleanup;
    }

    //
    // If there is a selection, copy is enabled
    //
    if (eSelectionType != SELECTION_TYPE_Caret )
    {
        pCmd->cmdf = MSOCMDSTATE_UP;
    }

Cleanup:
    ReleaseInterface( pSegmentList );
    ReleaseInterface(pStart);
    ReleaseInterface(pEnd);
    ReleaseInterface(pElement);
    RRETURN(hr);
}

