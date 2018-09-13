#include "headers.hxx"

#ifndef X_HTMLED_HXX_
#define X_HTMLED_HXX_
#include "htmled.hxx"
#endif

#ifndef X_EDUTIL_HXX_
#define X_EDUTIL_HXX_
#include "edutil.hxx"
#endif

#ifndef X_EDCMD_HXX_
#define X_EDCMD_HXX_
#include "edcmd.hxx"
#endif

#ifndef X_SELCMD_HXX_
#define X_SELCMD_HXX_
#include "selcmd.hxx"
#endif

#ifndef X_SELMAN_HXX_
#define X_SELMAN_HXX_
#include "selman.hxx"
#endif

//
// Externs
//
MtDefine(CSelectAllCommand, EditCommand, "CSelectAllCommand");
MtDefine(CClearSelectionCommand, EditCommand, "CClearSelectionCommand");


////////////////////////////////////////////////////////////////////////////////
// CSelectAllCommand
////////////////////////////////////////////////////////////////////////////////

HRESULT 
CSelectAllCommand::PrivateExec( 
    DWORD nCmdexecopt,
    VARIANTARG * pvarargIn,
    VARIANTARG * pvarargOut )
{
    HRESULT             hr;
    SP_ISegmentList     spSegmentList;
    SP_IMarkupPointer   spStart;
    SP_IMarkupPointer   spEnd;
    INT                 iSegmentCount;
    SELECTION_TYPE      eSelType;
    BOOL                fSelectedAll = FALSE;
    
    IFR( GetSegmentList( &spSegmentList ));
            
    //
    // NOTE: This code is required for select all after range.pastehtml
    //       so that it auto-detects any existing url's.  This is
    //       a BVT case.  See bug 40009. [ashrafm]
    //

    if ( GetEditor()->GetSelectionManager()->IsParentEditable() )
    {
        IFR( spSegmentList->GetSegmentCount( & iSegmentCount, &eSelType ) );

        if( eSelType == SELECTION_TYPE_Caret )
        {
            IFR( GetMarkupServices()->CreateMarkupPointer( & spStart ) );
            IFR( GetMarkupServices()->CreateMarkupPointer( & spEnd ) );

            IFR( spSegmentList->MovePointersToSegment ( 0, spStart, spEnd ) );
            IGNORE_HR( AutoUrl_DetectCurrentWord( GetMarkupServices(), spStart, NULL, NULL ));
        }
    }

    IFR( GetEditor()->GetSelectionManager()->SelectAll( spSegmentList, & fSelectedAll ));   

    return S_OK;
}

HRESULT 
CSelectAllCommand::PrivateQueryStatus( OLECMD * pcmd,
                     OLECMDTEXT * pcmdtext )
{
    pcmd->cmdf = MSOCMDSTATE_UP;

    return S_OK;
}

////////////////////////////////////////////////////////////////////////////////
// CClearSelectionCommand
////////////////////////////////////////////////////////////////////////////////

HRESULT 
CClearSelectionCommand::PrivateExec( 
    DWORD nCmdexecopt,
    VARIANTARG * pvarargIn,
    VARIANTARG * pvarargOut )
{
    IGNORE_HR( GetEditor()->GetSelectionManager()->EmptySelection( FALSE ) );

    return S_OK;
}

HRESULT 
CClearSelectionCommand::PrivateQueryStatus( OLECMD * pcmd,
                     OLECMDTEXT * pcmdtext )
{
    pcmd->cmdf = MSOCMDSTATE_UP;

    return S_OK;
}


