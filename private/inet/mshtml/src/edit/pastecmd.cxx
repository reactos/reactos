//+------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1998, 1999
//
//  File:       PASTECMD.CXX
//
//  Contents:   Implementation of Paste command.
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

#ifndef _X_PASTECMD_HXX_
#define _X_PASTECMD_HXX_
#include "pastecmd.hxx"
#endif

#ifndef X_SELMAN_HXX_
#define X_SELMAN_HXX_
#include "selman.hxx"
#endif

#ifndef X_SLOAD_HXX_
#define X_SLOAD_HXX_
#include "sload.hxx"
#endif

using namespace EdUtil;

//
// Externs
//

MtDefine(CPasteCommand, EditCommand, "CPasteCommand");


//+---------------------------------------------------------------------------
//
//  CPasteCommand::Paste
//
//----------------------------------------------------------------------------

enum FETCINDEX                          // Keep in sync with g_rgFETC[]
{
    iHTML,                              // HTML (in ANSI)
    iRtfFETC,                           // RTF
#ifndef WIN16
    iUnicodeFETC,                       // Unicode plain text
#endif // !WIN16
    iAnsiFETC,                          // ANSI plain text
//    iFilename,                          // Filename
    iRtfAsTextFETC,                     // Pastes RTF as text
    iFileDescA,                         // FileGroupDescriptor
    iFileDescW,                         // FileGroupDescriptorW
    iFileContents                       // FileContents
};

FORMATETC *
GetFETCs ( int * pnFETC )
{
    static int fInitted = FALSE;

    static FORMATETC rgFETC[] =
    {
        { 0,                 NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL}, // CF_HTML
        { 0,                 NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL}, // CF_RTF
    #ifndef WIN16
        { CF_UNICODETEXT,    NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL},
    #endif
        { CF_TEXT,           NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL},
        { 0,                 NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL}, // CF_RTFASTEXT
        { 0,                 NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL}, // CF_FILEDESCRIPTORA
        { 0,                 NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL}, // CF_FILEDESCRIPTORW
        { 0,                 NULL, DVASPECT_CONTENT,  0, TYMED_HGLOBAL}, // CF_FILECONTENTS
    };

    if (!fInitted)
    {
        fInitted = TRUE;

        Assert( ! rgFETC [ iHTML          ].cfFormat );
        Assert( ! rgFETC [ iRtfFETC       ].cfFormat );
        Assert( ! rgFETC [ iRtfAsTextFETC ].cfFormat );
        Assert( ! rgFETC [ iFileDescA     ].cfFormat );
        Assert( ! rgFETC [ iFileContents  ].cfFormat );

        rgFETC [ iHTML          ].cfFormat = (CLIPFORMAT)RegisterClipboardFormatA( "HTML Format" );
        rgFETC [ iRtfFETC       ].cfFormat = (CLIPFORMAT)RegisterClipboardFormatA( "Rich Text Format" );
        rgFETC [ iRtfAsTextFETC ].cfFormat = (CLIPFORMAT)RegisterClipboardFormatA( "RTF As Text" );
        rgFETC [ iFileDescW     ].cfFormat = (CLIPFORMAT)RegisterClipboardFormat ( CFSTR_FILEDESCRIPTORW );
        rgFETC [ iFileContents  ].cfFormat = (CLIPFORMAT)RegisterClipboardFormat ( CFSTR_FILECONTENTS );
    }

    if (pnFETC)
        *pnFETC= ARRAY_SIZE( rgFETC );

    return rgFETC;
}

extern HGLOBAL TextHGlobalAtoW( HGLOBAL hglobalA );

HRESULT 
CPasteCommand::Paste (
    IMarkupPointer* pStart, IMarkupPointer* pEnd, CSpringLoader * psl, BSTR bstrText /* = NULL */)
{
    HRESULT           hr;
    IMarkupServices * pMarkupServices = GetMarkupServices();
    BOOL              fResult;
    //
    // Delete the range first
    //
    IFC( pMarkupServices->Remove( pStart, pEnd ) );

    //
    // If there is nothing to paste we're done
    //
    if (bstrText == NULL || *bstrText == 0)
    {
        return S_OK;
    }

    //
    // InsertSanitized text checks whether we accept html or not
    // and handles CR LF characters appropriately
    //
    IFC( GetEditor()->InsertSanitizedText( bstrText, pStart, pMarkupServices, psl, FALSE ) );

    // Call the url autodetector
    IFC( pStart->IsRightOf( pEnd, & fResult ) );
    if ( fResult )
    {
        IGNORE_HR( AutoUrl_DetectRange( pMarkupServices, pEnd, pStart, TRUE, pStart ) );
    }
    else
    {
        IGNORE_HR( AutoUrl_DetectRange( pMarkupServices, pStart, pEnd, TRUE, pEnd ) );
    }

    // Collapse the range
    IFC( pStart->IsRightOf( pEnd, & fResult ) );
    if ( fResult )
    {
        IFC( pEnd->MoveToPointer( pStart ) );
    }
    else
    {
        IFC( pStart->MoveToPointer( pEnd ) );
    }

Cleanup:
    RRETURN( hr );
}


#ifdef MERGEFUN
//BUGBUG: this function came from the old LDTE code
//
//+----------------------------------------------------------------------------
//
//  Member:     FilterReservedChars
//
//  Synopsis:   Replaces any characters within our reserved unicode range
//              with question marks.
//
//-----------------------------------------------------------------------------


static void
FilterReservedChars( TCHAR * pStrText )
{
    while (*pStrText)
    {
        if (!IsValidWideChar(*pStrText))
        {
            *pStrText = _T('?');
        }
        pStrText++;
    }
}
#endif

HRESULT 
CPasteCommand::PasteFromClipboard (
    IMarkupPointer *  pPasteStart, 
    IMarkupPointer *  pPasteEnd, 
    IDataObject *     pDataObject,
    CSpringLoader *   psl)
{
    CHTMLEditor         *   pEditor = GetEditor();
    IDataObject * pdoFromClipboard = NULL;
    CLIPFORMAT  cf = 0;
    HGLOBAL     hglobal = NULL;
    HRESULT     hr = DV_E_FORMATETC;
    HRESULT     hr2 = S_OK;
    HGLOBAL     hUnicode = NULL;
    int         i;
    int         nFETC;
    STGMEDIUM   medium = {0, NULL};
    FORMATETC * pfetc;
    LPTSTR      ptext = NULL;
    IHTMLViewServices * pViewServices = pEditor->GetViewServices();
    IHTMLElement    * pElement = NULL;
    IHTMLElement    * pFlowElement = NULL;
    BOOL    fAcceptsHTML, fContainer;
    LPSTR                   pszRtf;
    IMarkupServices     *   pMarkupServices = pEditor->GetMarkupServices();
    IMarkupPointer      *   pRangeStart   = NULL;
    IMarkupPointer      *   pRangeEnd     = NULL;
    IDataObject *           pdoFiltered = NULL;
    IDocHostUIHandler *     pHostUIHandler = NULL;

    //
    // Set up a pair of pointers for URL Autodetection after the insert
    //
    IFC( pMarkupServices->CreateMarkupPointer( &pRangeStart ) );
    IFC( pRangeStart->MoveToPointer( pPasteStart ) );
    IFC( pRangeStart->SetGravity( POINTER_GRAVITY_Left ) );        

    IFC( pMarkupServices->CreateMarkupPointer( &pRangeEnd ) );
    IFC( pRangeEnd->MoveToPointer( pPasteStart ) );
    IFC( pRangeEnd->SetGravity( POINTER_GRAVITY_Right ) );

    pfetc = GetFETCs ( & nFETC );

    Assert( pfetc );

    if (!pDataObject)
    {
        hr2 = OleGetClipboard( & pdoFromClipboard );
        
        if (hr2 != NOERROR)
            return hr2;

        Assert( pdoFromClipboard );

        //
        // See if the host handler wants to give us a massaged data object.
        //
        
        IFC( pViewServices->GetDocHostUIHandler( & pHostUIHandler ) );

        pDataObject = pdoFromClipboard;
        
        if (pHostUIHandler)
        {
            //
            // The host may want to massage the data object to block/add
            // certain formats.
            //
            
            hr = THR( pHostUIHandler->FilterDataObject( pDataObject, & pdoFiltered ) );
            
            if (!hr && pdoFiltered)
            {
                pDataObject = pdoFiltered;
            }
            else
            {
                hr = S_OK;
            }
        }
    }

    if ( pPasteEnd )
    {
        IFC( pMarkupServices->Remove( pPasteStart, pPasteEnd ) );
    }

    //
    // Check if we accept HTML
    //
    IFC( pViewServices->GetFlowElement(pPasteStart, & pFlowElement) );

    if (! pFlowElement)
    {
        //
        // Elements that do not accept HTML, e.g. TextArea, always have a flow layout.
        // If the element does not have a flow layout then it might have been created
        // using the DOM (see bug 42685). Set fAcceptsHTML to true.
        //
        fAcceptsHTML = TRUE;
    }
    else
    {
        IFC( pViewServices->IsContainerElement( pFlowElement, & fContainer, & fAcceptsHTML) );
    }
    
    for( i = 0; i < nFETC; i++, pfetc++ )
    {
        // make sure the format is either 1.) a plain text format
        // if we are in plain text mode or 2.) a rich text format
        // or 3.) matches the requested format.

        if( cf && cf != pfetc->cfFormat )
        {
            continue;
        }

        //
        // If we don't accept HTML and i does not correspond to text format
        // skip it
        //
#ifndef WIN16
        if ( fAcceptsHTML || i == iAnsiFETC || i == iUnicodeFETC )
#else
        if ( fAcceptsHTML || i == iAnsiFETC )                        
#endif // !WIN16
        {
            //
            // make sure the format is available
            //
            if( pDataObject->QueryGetData(pfetc) != NOERROR )
            {
                continue;
            }

            //
            // If we have one of the formats that uses an hglobal get it
            // and lock it.
            //
            if (
#ifndef NO_RTF
                i == iRtfAsTextFETC ||  i == iRtfFETC ||
#endif
                i == iAnsiFETC ||
#ifndef WIN16
                i == iUnicodeFETC ||
#endif // !WIN16
                i == iHTML )
            {
                if( pDataObject->GetData(pfetc, &medium) == NOERROR )
                {
                    Assert(medium.tymed == TYMED_HGLOBAL);

                    hglobal = medium.hGlobal;
                    ptext = (LPTSTR)GlobalLock(hglobal);
                    if( !ptext )
                    {
                        ReleaseStgMedium(&medium);
                        return E_OUTOFMEMORY;
                    }
                }
                else
                {
                    continue;
                }
            }

            switch(i)
            {
            case iHTML:
            {
                // Fire the springloader.
                BOOL fSamePosition = FALSE;
                if (   pPasteStart && psl
                    && (!pPasteEnd || (S_OK == THR(pPasteStart->IsEqualTo(pPasteEnd, &fSamePosition)) && fSamePosition))
                    && S_OK == THR(psl->Fire(pPasteStart))
                    && pPasteEnd)
                {
                    IGNORE_HR(pPasteEnd->MoveToPointer(pPasteStart));
                }

                hr = THR(
                    pViewServices->DoTheDarnPasteHTML(
                        pPasteStart, pPasteEnd, hglobal ) );

                goto Cleanup;
            }
            case iRtfFETC:
            {
                BOOL fEnabled = FALSE;

                IGNORE_HR(pViewServices->IsRtfConverterEnabled(&fEnabled));
                if (!fEnabled)
                    continue;
                
                pszRtf = LPSTR( GlobalLock( hglobal ) );

                if(!pszRtf)
                {
                    hr = E_OUTOFMEMORY;
                    goto Cleanup;
                }

                hr = THR(pViewServices->ConvertRTFToHTML((LPOLESTR)pszRtf, &hglobal));


                if (!hr)
                {
                    //
                    // RTF conversion worked
                    //

                    hr = THR(
                        pViewServices->DoTheDarnPasteHTML(
                            pPasteStart, pPasteEnd, hglobal ) );

                }
                else
                {
                    // RTF conversion failed, try the next format
                    hr = DV_E_FORMATETC;
                    continue;
                }

                goto Cleanup;
            }

            case iRtfAsTextFETC:
            case iAnsiFETC:         // ANSI plain text. If data can be stored

                hUnicode = TextHGlobalAtoW(hglobal);

                if (hUnicode)
                {
                    ptext = (LPTSTR)GlobalLock(hUnicode);
                    if(!ptext)
                    {
                        hr = E_OUTOFMEMORY;
                        goto Cleanup;
                    }

                    //FilterReservedChars(ptext);


                    hr = THR( GetEditor()->InsertSanitizedText( ptext, pPasteStart, pMarkupServices, psl, FALSE ) );
                    
                    IGNORE_HR( AutoUrl_DetectRange( pMarkupServices, pRangeStart, pRangeEnd, TRUE, pRangeEnd ) );
                    
                    GlobalUnlock(hUnicode);
                }

                goto Cleanup;

            case iUnicodeFETC:                          // Unicode plain text

                ptext   = (LPTSTR)GlobalLock(hglobal);
                if(!ptext)
                {
                    hr = E_OUTOFMEMORY;
                    goto Cleanup;
                }

                //FilterReservedChars(ptext);

                hr = THR( GetEditor()->InsertSanitizedText( ptext, pPasteStart, pMarkupServices, psl, FALSE ) );

                IGNORE_HR( AutoUrl_DetectRange( pMarkupServices, pRangeStart, pRangeEnd, TRUE, pRangeEnd ) );

                goto Cleanup;
            }

            //Break out of the for loop
            break;
        }
    }

Cleanup:

    ReleaseInterface( pRangeStart );
    ReleaseInterface( pRangeEnd );

    ReleaseInterface( pElement );
    ReleaseInterface( pFlowElement );

    ReleaseInterface( pdoFiltered );
    ReleaseInterface( pHostUIHandler );

    ReleaseInterface( pdoFromClipboard );

    if (hUnicode)
    {
        GlobalFree(hUnicode);
    }

    //If we used the hglobal unlock it and free it.
    if(hglobal)
    {
        GlobalUnlock(hglobal);
        ReleaseStgMedium(&medium);
    }

    return hr;
}


HRESULT
CPasteCommand::InsertText(OLECHAR         * pchText,
                          long              cch,
                          IMarkupPointer  * pPointerTarget,
                          CSpringLoader   * psl)
{
    // Fire the spring loader.
    if (psl)
    {
        Assert(pPointerTarget);

        IGNORE_HR(psl->Fire(pPointerTarget));
    }

    RRETURN( THR( GetViewServices()->InsertMaximumText( pchText, cch, pPointerTarget ) ) );
}


//+---------------------------------------------------------------------------
//
//  CPasteCommand::Exec
//
//----------------------------------------------------------------------------

HRESULT
CPasteCommand::PrivateExec( 
    DWORD nCmdexecopt,
    VARIANTARG * pvarargIn,
    VARIANTARG * pvarargOut )
{
    HRESULT             hr = S_OK;
    int                 iSegmentCount, i;
    IMarkupPointer *    pStart = NULL;
    IMarkupPointer *    pEnd = NULL;
    SELECTION_TYPE      eSelectionType;
    IHTMLViewServices * pViewServices = GetViewServices();
    IMarkupServices *   pMarkupServices = GetMarkupServices();
    ISegmentList    *   pSegmentList = NULL;
    BOOL                fRet;
    DWORD               code = 0;
    CHTMLEditor     *   pEditor = GetEditor();
    CUndoUnit           undoUnit(pEditor);

    ((IHTMLEditor *) pEditor)->AddRef();    // FireOnCancelableEvent can remove the whole doc

    if (IsSelectionActive())
        return S_OK; // don't paste into an active selection
    
    //
    // Get the segment etc., let's get busy
    //
    IFC( GetSegmentList( &pSegmentList ));
    IFC( pSegmentList->GetSegmentCount( & iSegmentCount, & eSelectionType ) );
    IFC( pMarkupServices->CreateMarkupPointer( & pStart ));
    IFC( pMarkupServices->CreateMarkupPointer( & pEnd ));

    if ( iSegmentCount != 0 )
    {       
        SP_IHTMLElement spElement;
        IFC( undoUnit.Begin(IDS_EDUNDOPASTE) );

        for ( i = 0; i < iSegmentCount; i++ )
        {
            IFC( MovePointersToSegmentHelper(GetViewServices(), pSegmentList, i, &pStart, &pEnd ));

            IFC( IsPastePossible( pStart, pEnd, & fRet ) );
            if (! fRet)
            {
                continue;
            }

            IFC( FindCommonElement(pMarkupServices, GetViewServices(), pStart, pEnd, &spElement ));

            if (! spElement)
            {
                continue;
            }

            IFC( pViewServices->FireCancelableEvent( spElement, DISPID_EVMETH_ONPASTE, DISPID_EVPROP_ONPASTE,
                                                     _T( "paste" ), & fRet));

            if (! fRet)
            {
                continue;
            }

            if (pvarargIn && V_VT(pvarargIn) == VT_BSTR)
            {
                // Paste the passed in bstrText 
                IFC( Paste( pStart, pEnd, GetSpringLoader(), V_BSTR(pvarargIn) ) );
            }
            else
            {
                // Paste from the clipboard
                IFC( PasteFromClipboard( pStart, pEnd, NULL, GetSpringLoader() ) );
            }
        }

        if ( ( eSelectionType != SELECTION_TYPE_Auto) && 
             ( eSelectionType != SELECTION_TYPE_Control ) ) // Control is handled in ExitTree
        {
            pEditor->GetSelectionManager()->EmptySelection();
        }


        if ( eSelectionType != SELECTION_TYPE_Auto )
        {
            //
            // Update selection - go to pStart since it has gravity Right
            //
            IGNORE_HR( pEnd->MoveToPointer( pStart ) );            
            pEditor->Select( pStart, pEnd, SELECTION_TYPE_Caret, &code );
            Assert(code == 0 );
        }
    }

Cleanup:   
    ReleaseInterface((IHTMLEditor *) pEditor);
    ReleaseInterface( pSegmentList );
    ReleaseInterface( pStart );
    ReleaseInterface( pEnd );
    RRETURN ( hr );
}


HRESULT
CPasteCommand::PrivateQueryStatus( 
    OLECMD * pCmd,
    OLECMDTEXT * pcmdtext )
{
    HRESULT             hr = S_OK;
    INT                 iSegmentCount;
    IMarkupPointer *    pStart = NULL;
    IMarkupPointer *    pEnd = NULL;
    IHTMLElement *      pElement = NULL;
    BOOL                fEditable;
    IMarkupServices *   pMarkupServices = GetMarkupServices();
    IHTMLViewServices * pViewServices = GetViewServices();
    ISegmentList *      pSegmentList = NULL;
    SELECTION_TYPE      eSelectionType;
    BOOL fRet;

    pViewServices->AddRef();    // FireOnCancelableEvent can remove the whole doc

    // Status is disabled by default
    pCmd->cmdf = MSOCMDSTATE_DISABLED;

    IFC( GetSegmentList( &pSegmentList ));
    IFC( pSegmentList->GetSegmentCount( & iSegmentCount, &eSelectionType ) );
    if ( iSegmentCount == 0 ) 
        goto Cleanup;

    IFC( GetSegmentPointers(pSegmentList, 0, &pStart, &pEnd) );
    IFC( FindCommonElement( pMarkupServices, GetViewServices(), pStart, pEnd, &pElement ) );
    if (! pElement)
        goto Cleanup;

    IFC( pViewServices->FireCancelableEvent(
            pElement,
            DISPID_EVMETH_ONBEFOREPASTE,
            DISPID_EVPROP_ONBEFOREPASTE,
            _T( "beforepaste" ),
            & fRet) );

    if (! fRet)
    {
        pCmd->cmdf = MSOCMDSTATE_UP;
        goto Cleanup;
    }

    if (eSelectionType != SELECTION_TYPE_Auto && eSelectionType != SELECTION_TYPE_Control)
    {
        IFC( pViewServices->IsEditableElement( pElement, &fEditable ) );
        if (! fEditable)
            goto Cleanup;
    }

    IFC( IsPastePossible( pStart, pEnd, & fRet ) );
    if ( fRet )
    {
        pCmd->cmdf = MSOCMDSTATE_UP; // It's a GO
    }

Cleanup:
    ReleaseInterface( pViewServices );
    ReleaseInterface( pSegmentList );
    ReleaseInterface( pStart );
    ReleaseInterface( pEnd );
    ReleaseInterface( pElement );
    RRETURN(hr);
}


HRESULT
CPasteCommand::IsPastePossible ( IMarkupPointer * pStart,
                                 IMarkupPointer * pEnd,
                                 BOOL * pfResult )
{    
    HRESULT         hr = S_OK;    
    IHTMLElement *  pFlowElement = NULL;
    BOOL            fContainer;
    BOOL            fAcceptsHTML;
    CLIPFORMAT      cf;
    int             nFETC = 0;
    FORMATETC *     pfetc;

    Assert( pfResult );
    *pfResult = FALSE;

    //
    // Cannot paste unless the range is in the same flow layout
    //            
    if (! PointersInSameFlowLayout( pStart, pEnd, & pFlowElement, GetViewServices() ) )
    {
        goto Cleanup;
    }

    if (! pFlowElement)
        goto Cleanup;

    pfetc = GetFETCs ( & nFETC );
    
    //
    // If we don't accept HTML and there is no text on clipboard, it's a no go
    //    
    IFC( GetViewServices()->IsContainerElement( pFlowElement, & fContainer, & fAcceptsHTML ) );
    if ( (! fAcceptsHTML) && 
         (! IsClipboardFormatAvailable( pfetc[iAnsiFETC].cfFormat ) ) )
    {
        goto Cleanup;
    }

    //
    // Make sure that the clipboard has data in it
    //
    while (nFETC--)                 
    {
        cf = pfetc[nFETC].cfFormat;
        if( IsClipboardFormatAvailable(cf) )
        {
            *pfResult = TRUE; // It's a GO
            goto Cleanup;
        }
    }

Cleanup:
    ReleaseInterface( pFlowElement );
    RRETURN( hr );
}
