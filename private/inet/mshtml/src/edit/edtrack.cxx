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

#ifndef X_MSHTMLED_HXX_
#define X_MSHTMLED_HXX_
#include "mshtmled.hxx"
#endif

#ifndef X_SELMAN_HXX_
#define X_SELMAN_HXX_
#include "selman.hxx"
#endif

#ifndef X_HTMLED_HXX_
#define X_HTMLED_HXX_
#include "htmled.hxx"
#endif

#ifndef X_EDUTIL_HXX_
#define X_EDUTIL_HXX_
#include "edutil.hxx"
#endif

#ifndef _X_EDCOMMAND_HXX_
#define _X_EDCOMMAND_HXX_
#include "edcmd.hxx"
#endif

#ifndef _X_BLOCKCMD_HXX_
#define _X_BLOCKCMD_HXX_
#include "blockcmd.hxx"
#endif

#ifndef _X_DELCMD_HXX_
#define _X_DELCMD_HXX_
#include "delcmd.hxx"
#endif

#ifndef _X_EDADORN_HXX_
#define _X_EDADORN_HXX_
#include "edadorn.hxx"
#endif

#ifndef _X_EDTRACK_HXX_
#define _X_EDTRACK_HXX_
#include "edtrack.hxx"
#endif

#ifndef _X_IMG_H_
#define _X_IMG_H_
#include "img.h"
#endif

#ifndef _X_INPUTTXT_H_
#define _X_INPUTTXT_H_
#include "inputtxt.h"
#endif

#ifndef X_SLOAD_HXX_
#define X_SLOAD_HXX_
#include "sload.hxx"
#endif

#ifndef _X_IME_HXX_
#define _X_IME_HXX_
#include "ime.hxx"
#endif


using namespace EdUtil;

MtDefine( CEditTracker, Utilities , "CEditTracker" )
MtDefine( CCaretTracker, Utilities , "CCaretTracker" )
MtDefine( CSelectTracker, Utilities , "CSelectionTracker" )
MtDefine( CControlTracker, Utilities , "CControlTracker" )
#ifndef NO_IME
MtDefine( CImeTracker, Utilities , "CImeTracker" )
#endif

DeclareTag(tagSelectionDumpOnCp, "Selection","DumpTreeOnCpRange")

static BOOL DontBackspace( ELEMENT_TAG_ID eTagId );

extern int edWsprintf(LPTSTR pstrOut, LPCTSTR pstrFormat, LPCTSTR pstrParam);

static SIZE         gSizeDragMin;                   // the Size of a Minimum Drag.
int                 g_iDragDelay;                   // The Drag Delay

static char s_achWindows[] = "windows";             //  Localization: Do not localize

static const ACTION_TABLE ActionTable [] = {
                     //  0     1     2       3      4       5     6      7      8      9      10       11      12     13
    {WM_LBUTTONDOWN,  {A_ERR,A_ERR, A_ERR,  A_ERR, A_ERR,  A_ERR,A_ERR, A_ERR, A_ERR, A_ERR, A_10_9,  A_ERR,  A_ERR,  A_IGN}},
    //{WM_RBUTTONDOWN,  {A_ERR,A_ERR, A_ERR,  A_ERR, A_ERR,  A_ERR,A_ERR, A_ERR, A_ERR, A_ERR, A_10_9,  A_ERR,  A_ERR,  A_IGN}},
    {WM_LBUTTONUP,    {A_ERR,A_1_4, A_2_14, A_3_14,A_ERR,  A_5_7,A_6_14,A_ERR, A_8_10,A_9_14,A_ERR,   A_11_14,A_12_14,A_IGN}},
    //{WM_RBUTTONUP,    {A_ERR,A_1_14,A_2_14r,A_3_14,A_ERR,  A_5_7,A_6_14,A_ERR, A_8_10,A_9_14,A_ERR,   A_11_14,A_12_14,A_IGN}},
    {WM_TIMER,        {A_ERR,A_DIS, A_DIS,  A_3_2, A_DIS,  A_DIS,A_6_6, A_DIS, A_DIS, A_DIS, A_DIS,   A_DIS,  A_DIS,  A_IGN}},
    {WM_MOUSEMOVE,    {A_ERR,A_1_2, A_IGN,  A_3_2m,A_4_14m,A_5_6,A_6_6m,A_7_14,A_8_6, A_9_6, A_10_14m,A_11_6, A_12_6, A_IGN}},
    {WM_LBUTTONDBLCLK,{A_ERR,A_ERR, A_ERR,  A_ERR, A_4_8,  A_ERR,A_ERR, A_7_8, A_ERR, A_ERR, A_ERR,   A_ERR,  A_ERR,  A_IGN}},
  //{WM_RBUTTONDBLCLK,{A_ERR,A_ERR, A_ERR,  A_ERR, A_4_8,  A_ERR,A_ERR, A_7_8, A_ERR, A_ERR, A_ERR,   A_ERR,  A_ERR,  A_IGN}},
    {WM_KEYDOWN,      {A_IGN,A_1_15, A_IGN,  A_3_15,A_4_15, A_5_15,A_IGN,A_7_15, A_8_15, A_9_15, A_10_15, A_IGN,  A_12_15,  A_IGN}},
    {0,               {A_ERR,A_IGN, A_IGN,  A_IGN, A_IGN,  A_IGN,A_IGN, A_IGN, A_IGN, A_IGN, A_IGN,   A_IGN,  A_IGN,  A_IGN}}
  //dblclcktimer       A_ERR,A_1_3, A_ERR,  A_ERR, A_4_14t,A_5_11,A_ERR,A_7_14,A_8_12,A_ERR, A_10_14, A_ERR,  A_ERR,  A_ERR
  //this last line is here for documentation purposes only, see the callback Fx for the impl.
};

#define ISC_BUFLENGTH  10

DeclareTag(tagSelectionTrackerState, "Selection", "Selection show tracker state")
DeclareTag(tagSelectionDisableWordSel, "Selection", "Disable Word Selection Model")
DeclareTag(tagSelectionValidateWordPointer, "Selection", "Validate Word Selection")
DeclareTag(tagSelectionValidate, "Selection", "Validate Selection Size in edtrack")
DeclareTag(tagShowScroll,"Selection", "Show scroll pointer into  view");
DeclareTag(tagShowScrollMsgCount,"Selection", "Show scrollMessage Count");

#if DBG == 1

static const LPCTSTR strStartSelection = _T( "    ** Start_Selection");
static const LPCTSTR strEndSelection = _T( "    ** End_Selection");
static const LPCTSTR strWordPointer = _T( "    ** Word");
static const LPCTSTR strTestPointer = _T( "    ** Test");
static const LPCTSTR strPrevTestPointer = _T( "    ** Last Test");
static const LPCTSTR strImeIP = _T( "    ** IME IP");
static const LPCTSTR strImeUncommittedStart = _T( "    ** IME Uncommitted Start");
static const LPCTSTR strImeUncommittedEnd = _T( "    ** IME Uncommitted End");

static int gDebugTestPointerCp = -100;
static int gDebugEndPointerCp = -100;
static int gDebugTestPointerMinCp = -100;
static int gDebugTestPointerMaxCp = -100;
#endif

const DWORD BREAK_CONDITION_BlockScan = BREAK_CONDITION_OMIT_PHRASE - BREAK_CONDITION_Anchor;

//+====================================================================================
//
// Method: CEditTracker
//
// Synopsis: Base Constructor for Trackers.
//
//------------------------------------------------------------------------------------


CEditTracker::CEditTracker(
            CSelectionManager* pManager )
{
    _pManager = pManager;
    _eType = SELECTION_TYPE_None;
    _hwndDoc = NULL;
    _fInFireOnSelectStart = FALSE;
    _fNotAtBOL = TRUE;    
    _fAtLogicalBOL = FALSE;    
    _ulRef = 1;
}

//+====================================================================================
//
// Method: Release
//
// Synopsis: COM-Like release code for tracker
//
//------------------------------------------------------------------------------------

ULONG
CEditTracker::Release()
{
    if ( 0 == --_ulRef )
    {
        delete this;
        return 0;
    }
    return _ulRef;
}

BOOL
CEditTracker::IsListeningForMouseDown()
{
    return FALSE;
}

//+====================================================================================
//
// Method: GetNotAtBOL
//
// Synopsis: Get the 'bol-ness' of the tracker.
//
//------------------------------------------------------------------------------------

BOOL 
CEditTracker::GetNotAtBOL() 
{ 
    if ( _fRecalcBOL )
    {
        CalculateBOL(); 
        _fRecalcBOL = FALSE;
    }   
    return _fNotAtBOL;     
}


Direction 
CEditTracker::GetPointerDirection(CARET_MOVE_UNIT moveDir)
{
    switch (moveDir)
    {
        case CARET_MOVE_LEFT:
        case CARET_MOVE_UP:
        case CARET_MOVE_WORDLEFT:
        case CARET_MOVE_PAGEUP:
        case CARET_MOVE_VIEWSTART:
        case CARET_MOVE_LINESTART:
        case CARET_MOVE_HOME:
        case CARET_MOVE_BLOCKSTART:
            return LEFT;

        case CARET_MOVE_RIGHT:
        case CARET_MOVE_DOWN:
        case CARET_MOVE_WORDRIGHT:
        case CARET_MOVE_PAGEDOWN:
        case CARET_MOVE_VIEWEND:
        case CARET_MOVE_LINEEND:
        case CARET_MOVE_END:
        case CARET_MOVE_BLOCKEND:
            return RIGHT;                
    }
    
    AssertSz(0, "CEditTracker::GetPointerDirection unhandled case");
    return LEFT;
}

BOOL
CCaretTracker::IsListeningForMouseDown()
{
#ifndef NO_IME
    return _pManager->IsIMEComposition() || IsShiftKeyDown() ;
#else
    return IsShiftKeyDown() ;
#endif
}

BOOL
CEditTracker::IsPointerInSelection( IMarkupPointer *pPointer,  POINT * pptGlobal, IHTMLElement* pIElementOver    )
{
    return FALSE;
}


//+====================================================================================
//
// Method: ~CEditTracker
//
// Synopsis: Destructor for Tracker
//
//------------------------------------------------------------------------------------


CEditTracker::~CEditTracker()
{

}

BOOL
CEditTracker::IsActive()
{
    return FALSE;
}


//+====================================================================================
//
// Method: CEditTracker::GetSpringLoader
//
// Synopsis: Accessor for springloader
//
//------------------------------------------------------------------------------------

CSpringLoader *
CEditTracker::GetSpringLoader()
{
    CSpringLoader * psl = NULL;
    CHTMLEditor   * pEditor;

    if (!_pManager)
        goto Cleanup;

    pEditor = _pManager->GetEditor();

    if (!pEditor)
        goto Cleanup;

    psl = pEditor->GetPrimarySpringLoader();

Cleanup:
    return psl;
}


//+====================================================================================
//
// Method: CEditTracker::GetTagIdFromMessage
//
// Synopsis: GetTagIdFromMessage
//
//------------------------------------------------------------------------------------

HRESULT
CEditTracker::GetElementAndTagIdFromMessage(
    SelectionMessage* pMessage,
    IHTMLElement** ppElement, 
    ELEMENT_TAG_ID* peTag ,
    IHTMLDocument2 * pIDoc,
    IHTMLViewServices * pVS, /* = NULL */
    IMarkupServices * pMark /* = NULL*/ )
{
    HRESULT hr = S_OK;
    IHTMLViewServices* pViewServices = NULL;
    IMarkupServices* pMarkup = NULL;
    ELEMENT_TAG_ID eTag = TAGID_NULL;
    Assert( peTag );
    Assert( pMessage );

    if ( ! pVS )
    {
        hr = pIDoc->QueryInterface( IID_IHTMLViewServices, ( void** ) & pViewServices );
        if ( hr )
            goto Cleanup;
    }
    else
        pViewServices = pVS;

    if ( ! pMark )
    {
        hr = pIDoc->QueryInterface( IID_IMarkupServices, ( void** ) & pMarkup );
        if ( hr )
            goto Cleanup;
    }
    else
        pMarkup = pMark;

    hr = THR( pViewServices->GetElementFromCookie( pMessage->elementCookie,  ppElement ) );
    if ( hr )
        goto Cleanup;

    hr = THR( pMarkup->GetElementTagId( *ppElement, & eTag ));
    if ( hr )
        goto Cleanup;

Cleanup:

    *peTag = eTag;

    if ( !pVS )
        ReleaseInterface( pViewServices );

    if ( ! pMark )
        ReleaseInterface( pMarkup );

    RRETURN ( hr );
}

HRESULT
CEditTracker::GetTagIdFromMessage(
    SelectionMessage* pMessage,
    ELEMENT_TAG_ID* peTag ,
    IHTMLDocument2 * pIDoc,
    IHTMLViewServices * pVS,
    IMarkupServices * pMark )
{
    HRESULT hr = S_OK;
    IHTMLViewServices* pViewServices = NULL;
    IMarkupServices* pMarkup = NULL;
    ELEMENT_TAG_ID eTag = TAGID_NULL;

    IHTMLElement * pElement = NULL;
    Assert( peTag );
    Assert( pMessage );

    if ( ! pVS )
    {
        hr = pIDoc->QueryInterface( IID_IHTMLViewServices, ( void** ) & pViewServices );
        if ( hr )
            goto Cleanup;
    }
    else
        pViewServices = pVS;

    if ( ! pMark )
    {
        hr = pIDoc->QueryInterface( IID_IMarkupServices, ( void** ) & pMarkup );
        if ( hr )
            goto Cleanup;
    }
    else
        pMarkup = pMark;

    hr = THR( pViewServices->GetElementFromCookie( pMessage->elementCookie, & pElement ) );
    if ( hr )
        goto Cleanup;

    hr = THR( pMarkup->GetElementTagId(pElement, & eTag ));
    if ( hr )
        goto Cleanup;

Cleanup:

    *peTag = eTag;

    if ( !pVS )
        ReleaseInterface( pViewServices );

    ReleaseInterface( pElement );

    if ( ! pMark )
        ReleaseInterface( pMarkup );

    RRETURN ( hr );
}

BOOL 
CEditTracker::IsTablePart( ELEMENT_TAG_ID eTag )
{
    return ( ( eTag == TAGID_TD ) ||
       ( eTag == TAGID_TR ) ||
       ( eTag == TAGID_TBODY ) || 
       ( eTag == TAGID_TFOOT ) || 
       ( eTag == TAGID_TH ) ||
       ( eTag == TAGID_THEAD ) ||
       ( eTag == TAGID_CAPTION ) ||
       ( eTag == TAGID_TC )  ||        
       ( eTag == TAGID_COL ) || 
       ( eTag == TAGID_COLGROUP )); 
}

VOID
CEditTracker::OnEditFocusChanged()
{
    // do nothing.
}

HRESULT 
CEditTracker::MustDelayBackspaceSpringLoad(CSpringLoader *psl, IMarkupPointer *pPointer, BOOL *pbDelaySpringLoad)
{
    HRESULT         hr;
    CEditPointer    epTest(_pManager->GetEditor());
    DWORD           dwFound;
    
    Assert(psl && pPointer && pbDelaySpringLoad);

    *pbDelaySpringLoad = FALSE;

    // Make sure we don't spring load near an anchor boundary.  If we are at an anchor
    // boundary, spring load after the delete.
    //
    IFR( epTest->MoveToPointer(pPointer) );

    IFR( epTest.Scan(LEFT, BREAK_CONDITION_OMIT_PHRASE, &dwFound) ); // check pre-delete boundary
    if (epTest.CheckFlag(dwFound, BREAK_CONDITION_Text))
        IFR( epTest.Scan(LEFT, BREAK_CONDITION_OMIT_PHRASE, &dwFound) ); // check post-delete boundary

    if (epTest.CheckFlag(dwFound, BREAK_CONDITION_Anchor))
        *pbDelaySpringLoad = TRUE;

    return S_OK;
}

HRESULT
CSelectTracker::Init()
{
    HRESULT hr = S_OK;

    _eType = SELECTION_TYPE_Selection;
    _fEndConstrained = FALSE;
    _fMadeSelection = FALSE;
    _fAddedSegment = FALSE;
    _fInWordSel = FALSE;
    _fReversed = FALSE;
    _fWordPointerSet = FALSE;
    _fWordSelDirection = FALSE;
    _fStartAdjusted = FALSE;
    _lastCaretMove = CARET_MOVE_NONE;
    _fInSelectParagraph = FALSE;
    IFC( GetMarkupServices()->CreateMarkupPointer( & _pStartPointer));
    IFC( GetMarkupServices()->CreateMarkupPointer( & _pEndPointer));
    IFC( GetMarkupServices()->CreateMarkupPointer( & _pTestPointer));
    IFC( GetMarkupServices()->CreateMarkupPointer( & _pWordPointer ));
    IFC( GetMarkupServices()->CreateMarkupPointer( & _pPrevTestPointer));
    IFC( GetViewServices()->GetCurrentSelectionRenderingServices(  & _pSelRenSvc ));

    WHEN_DBG( _ctStartAdjusted = 0 );

#if DBG == 1
    SetDebugName( _pStartPointer, strStartSelection ) ; 
    SetDebugName( _pEndPointer, strEndSelection ); 
    SetDebugName( _pWordPointer, strWordPointer );
    SetDebugName( _pTestPointer, strTestPointer); 
    SetDebugName( _pPrevTestPointer, strPrevTestPointer );    
#endif 

Cleanup:

    return ( hr );

}

HRESULT
CSelectTracker::Init2( 
                        CSelectionManager*      pManager,
                        SelectionMessage *      pMessage, 
                        DWORD*                  pdwFollowUpAction, 
                        TRACKER_NOTIFY *        peTrackerNotify,
                        DWORD                   dwTCFlags)
{
    HRESULT hr = S_OK;

    hr = Init();
    if ( hr )
        goto Cleanup;

    _firstMessage = *pMessage;
    _fState = pManager->IsMessageInSelection(  pMessage ) ? ST_WAIT1 : ST_WAIT2; // marka - we should look to see if we're alredy in a selection here.
    _fDragDrop = ( _fState == ST_WAIT1) ;
    _fLeftButtonStart = ( pMessage->message == WM_LBUTTONDOWN );
    _fShift = FALSE;

#if DBG == 1
    //
    // Make sure we currently don't have a selection on screen.
    //
    if ( _fState == ST_WAIT2 )
    {
        int ctSegment;
        SELECTION_TYPE eSegmentType;
        IGNORE_HR( _pSelRenSvc->GetSegmentCount(& ctSegment,  & eSegmentType ));
        Assert( eSegmentType != SELECTION_TYPE_Selection); 
    }
#endif

    hr = BeginSelection( pMessage, pdwFollowUpAction, peTrackerNotify );

Cleanup:
    return hr;
}


HRESULT
CSelectTracker::Init2( 
                        CSelectionManager*      pManager,
                        IMarkupPointer*         pStart, 
                        IMarkupPointer*         pEnd, 
                        DWORD*                  pdwFollowUpAction, 
                        TRACKER_NOTIFY *        peTrackerNotify,
                        DWORD                   dwTCFlags,
                        CARET_MOVE_UNIT inLastCaretMove )
{
    HRESULT hr                  = S_OK;
    BOOL    fStartFromShiftKey  =  ENSURE_BOOL( dwTCFlags & TRACKER_CREATE_STARTFROMSHIFTKEY );
    BOOL    fNotAtBOL       = ENSURE_BOOL( dwTCFlags & TRACKER_CREATE_NOTATBOL);
    BOOL    fAtLogicalBOL   = ENSURE_BOOL( dwTCFlags & TRACKER_CREATE_ATLOGICALBOL);


    hr = Init();
    if ( hr )
        goto Cleanup;

    _fState = ST_WAIT2;
    _fDragDrop = FALSE;
    _fLeftButtonStart = TRUE;
    _fShift = fStartFromShiftKey ;
    SetLastCaretMove( inLastCaretMove );
    
    ResetSpringLoader(pManager, pStart, pEnd);

    hr = Position( pStart, pEnd, fNotAtBOL, fAtLogicalBOL );

    if (GetSelectionType() == SELECTION_TYPE_Caret)
    {
        POINT pt;

        if (SUCCEEDED(_pManager->GetActiveTracker()->GetLocation(&pt)))
        {
            _curMouseX = pt.x;
        }
    }
        
    
    BecomePassive( peTrackerNotify ); // Our selection is passive if it's been set by TreePointers


    
Cleanup:
    return hr;
}

CSelectTracker::~CSelectTracker()
{
    BecomePassive();
    ClearInterface( & _pStartPointer );
    ClearInterface( & _pEndPointer );
    ClearInterface( & _pTestPointer ); 
    ClearInterface( & _pWordPointer );
    ClearInterface( & _pPrevTestPointer );
    ClearInterface( & _pSelRenSvc );
    ClearInterface( & _pSwapPointer );
}

//+====================================================================================
//
// Method: CalculateBOL
//
// Synopsis: Get our BOL'ness.
//
//------------------------------------------------------------------------------------

VOID
CSelectTracker::CalculateBOL() 
{ 
    BOOL fBetweenLines = FALSE;
    HRESULT hr = S_OK;
    
    hr = THR( GetViewServices()->IsPointerBetweenLines( _pEndPointer, & fBetweenLines));
    if ( ! FAILED(hr))
    {
        _fNotAtBOL = ! fBetweenLines;
        _fAtLogicalBOL = fBetweenLines;
    }
}


void
CSelectTracker::ResetSpringLoader( CSelectionManager* pManager, IMarkupPointer* pStart, IMarkupPointer* pEnd )
{
    CHTMLEditor   * ped = pManager ? pManager->GetEditor() : NULL;
    CSpringLoader * psl = ped ? ped->GetPrimarySpringLoader() : NULL;
    BOOL            fResetSpringLoader = FALSE;

    // Not using a BOOLEAN OR because it bugs PCLint, which thinks
    // that they're evaluated right to left.
    if (!psl)
        goto Cleanup;
    if (!psl->IsSpringLoaded())
        goto Cleanup;

    // Hack for Outlook: Are the pointers only separated by an &nbsp on an empty line.
    fResetSpringLoader = S_OK != psl->CanSpringLoadComposeSettings(pStart, NULL, FALSE, TRUE);
    if (fResetSpringLoader)
        goto Cleanup;

    {
        SP_IMarkupPointer   spmpStartCopy;
        BOOL                fEqual = FALSE;
        MARKUP_CONTEXT_TYPE eContext = CONTEXT_TYPE_None;
        long                cch = 1;
        TCHAR               ch;
        HRESULT             hr;

        fResetSpringLoader = TRUE;

        hr = THR(CopyMarkupPointer(pManager->GetMarkupServices(), pStart, &spmpStartCopy));
        if (hr)
            goto Cleanup;

        hr = THR(spmpStartCopy->Right(TRUE, &eContext, NULL, &cch, &ch));
        if (hr)
            goto Cleanup;

        hr = THR(spmpStartCopy->IsEqualTo(pEnd, &fEqual));
        if (hr)
            goto Cleanup;

        if (   eContext == CONTEXT_TYPE_Text
            && cch == 1
            && ch == WCH_NBSP
            && fEqual )
        {
            fResetSpringLoader = FALSE;
        }
    }

Cleanup:

    if (fResetSpringLoader)
        psl->Reset();
}

HRESULT 
CSelectTracker::AdjustPointersForChar()
{
    HRESULT         hr;
    CEditPointer    epStart(_pManager->GetEditor()), epEnd(_pManager->GetEditor());
    DWORD           eBreakCondition = BREAK_CONDITION_OMIT_PHRASE - BREAK_CONDITION_Anchor;
    CSpringLoader * psl = _pManager->GetEditor()->GetPrimarySpringLoader();

    IFR( epStart->MoveToPointer(_pStartPointer) );
    IFR( epEnd->MoveToPointer(_pEndPointer) );

    IFR( epStart.Scan(RIGHT, eBreakCondition | BREAK_CONDITION_ExitPhrase, NULL) ); // skip phrase elements
    IFR( epStart.Scan(LEFT, eBreakCondition | BREAK_CONDITION_EnterPhrase, NULL) ); // move back
    
    IFR( epEnd.Scan(RIGHT, eBreakCondition, NULL) ); // skip phrase elements
    IFR( epEnd.Scan(LEFT, eBreakCondition, NULL) ); // move back

    IFR( _pStartPointer->MoveToPointer( epStart ));
    IFR( _pEndPointer->MoveToPointer( epEnd ));

    IFR( _pSelRenSvc->MoveSegmentToPointers( 0, _pStartPointer, _pEndPointer, HIGHLIGHT_TYPE_Selected ));

    //
    // Spring load in the right place
    //

    if (psl)
    {
        BOOL fFurtherInStory = GetMoveDirection();

        if (fFurtherInStory)
            IGNORE_HR( psl->SpringLoad(_pStartPointer, SL_TRY_COMPOSE_SETTINGS) );
        else
            IGNORE_HR( psl->SpringLoad(_pEndPointer, SL_TRY_COMPOSE_SETTINGS) );
    }

    return S_OK;
}

BOOL
CSelectTracker::IsListeningForMouseDown()
{
    return ( _fState == ST_WAIT3RDBTNDOWN );
}

VOID
CSelectTracker::EndAndStartCaret(TRACKER_NOTIFY * peTrackerNotify)
{
    Assert( peTrackerNotify );
    _pManager->CopyTempMarkupPointers( _pStartPointer, _pEndPointer );
    *peTrackerNotify = TN_END_TRACKER_POS_CARET;     
}


//+====================================================================================
//
// Method: SetState
//
// Synopsis: Set the State of the tracker. Don't set state if we're in the passive state
//           unless we're explicitly told to do so.
//
//------------------------------------------------------------------------------------

VOID
CSelectTracker::SetState( 
                    SELECT_STATES inState, 
                    BOOL fIgnorePassive /*=FALSE*/)                    
{
    if ( _fState != ST_PASSIVE || fIgnorePassive )
    {
        _fState = inState;
    }
}

HRESULT
CSelectTracker::HandleMessagePrivate(
                SelectionMessage *pMessage,
                DWORD* pdwFollowUpAction,
                TRACKER_NOTIFY * peTrackerNotify )
{
    ACTIONS                Action = A_UNK;
    HRESULT hr = S_OK;
    DWORD followUpAction = FOLLOW_UP_ACTION_None;
    TRACKER_NOTIFY eNotify    = TN_NONE;
    BOOL fHandledDrag = FALSE;
    
    //
    // marka - I've currently written in comments below the stuff I'm not doing as yet,
    // mostly because it involves treenodes and layouts. See txttrack for the extensive notes
    //
    //
    // If Element we hit is a Slave, and we're in a different flow layout
    //  - change the node we hit. This is for trying to select from a input box, outside the text box
    //
    // Else if capture changes, kill the tracker.
    //
    // Else if Currency changes, Stop capture.
    //
    switch (pMessage->message)
    {
        case WM_LBUTTONUP:
            followUpAction |= FOLLOW_UP_ACTION_OnClick; // Allow this to fire an ONClick
            break;
    }
        
    // If we have not already decided what to do with the message ...
    Action = (Action == A_UNK) ? GetAction (pMessage) : Action;

    switch (Action)
    {
    case A_ERR: // Spurious error, bail out please
        if ( _pManager->IsInCapture() )
            ReleaseCapture();
        EndAndStartCaret( & eNotify ); 
        break;

    case A_DIS: // Discard the message
        break;

    case A_IGN: // Do nothing, just ignore the message, let somebody else process it
        hr = S_FALSE;
        
        //
        // the code below is to eat keystrokes while dragging and make sure they dont'
        // get translated into commands - eg. IDM_DELETE.
        //
        if ( ( pMessage->message == WM_KEYDOWN ) && ! IsPassive() )
        {
            hr = S_OK; // eat all keystrokes while dragging.
        }
        break;

    case A_5_7: // ST_WAIT2 & WM_LBUTTONUP
        SetState( ST_WAITBTNDOWN2 );
        //_pManager->StoreLastMessage( pMessage );
        break;

    case A_5_6: // ST_WAIT2 & WM_MOUSEMOVE
        // if the bubblable onselectstart event is cancelled, don't
        // go into DuringSelection State, and cnx the tracker
        if ( ! FireSelectStartMessage( pMessage ) )
        {
            if (  _pManager->IsInCapture()  )
                ReleaseCapture();
            StopTimer();                
            EndAndStartCaret( & eNotify );
        }
        else
        {
            StopTimer();
            GetViewServices()->SetHTMLEditorMouseMoveTimer( );
            DoSelection (pMessage);
            SetState( ST_DOSELECTION ) ;
        }
        break;

    case A_1_4: // ST_WAIT1 & WM_LBUTTONUP
        ClearSelection();
        SetState( ST_WAITBTNDOWN1 ) ;
        break;

    case A_1_2: // ST_WAIT1 & WM_MOUSEMOVE
        {
            if ( _pManager->IsInTimer() )
                StopTimer();
            hr = S_FALSE;
            SetState( ST_DRAGOP );
        }
        break;

    case A_2_14: // ST_DRAGOP & WM_LBUTTONUP
        // hr = SetCaret( pMessage );
        if (  _pManager->IsInCapture()  )
            ReleaseCapture();
            
        EndAndStartCaret( & eNotify );
        break;

    case A_3_14 :  // ST_MAYDRAG & (WM_LBUTTONUP || WM_RBUTTONUP)
    case A_4_14m : // ST_WAITBTNDOWN1 & WM_MOUSEMOVE (was b05)
        // In this case, the cp is *never* updated to the
        // new position. So go and update it. Remember SetCaret
        // also kills any existing selections.
        ClearSelection();
        if (  _pManager->IsInCapture()  )
            ReleaseCapture();

        EndAndStartCaret( & eNotify );
        break;

    case A_3_2: // ST_MAYDRAG & WM_TIMER

        if ( _pManager->IsInTimer() )
            StopTimer();
        // Convert to a move message with correct coordinates
        GetMousePoint(&pMessage->pt);
        if ( IsValidMove( pMessage ) )
        {
            hr = S_FALSE;
            SetState( ST_DRAGOP );
            fHandledDrag = TRUE;
            DoTimerDrag();
        }


        break;

    case A_3_2m: // ST_MAYDRAG & WM_MOUSEMOVE
        {
            if ( _pManager->IsInTimer() )
                StopTimer();
            hr = S_FALSE;
            SetState( ST_DRAGOP );
        }
        break;

    case A_4_8 :// ST_WAITBTNDOWN1 & WM_LBUTTONDBLCLK
    case A_7_8 :// ST_WAITBTNDOWN2 & WM_LBUTTONDBLCLK (was b60)
        if ( _fDragDrop )
            _fDragDrop = FALSE;
            

        StopTimer();
        StartTimer();
        // if the bubblable onselectstart event is cancelled, don't
        // go into DoSelection State, and cnx the tracker
        if (! FireSelectStartMessage( pMessage ) )
        {
            if (  _pManager->IsInCapture()  )
                ReleaseCapture();
            
            EndAndStartCaret( & eNotify );
        }
        else
        {
            hr = DoSelectWord( pMessage );
            SetState( ST_SELECTEDWORD );
#ifdef UNIX
            if (CheckSelectionWasReallyMade())
            {
                if (!_hwndDoc)
                    _pManager->GetEditor()->GetViewServices()->GetViewHWND(&_hwndDoc);
                SendMessage(_hwndDoc, WM_GETTEXTPRIMARY, (WPARAM)&_firstMessage, IDM_CLEARSELECTION);
            }
#endif
        }
        break;

    case A_6_14 :   // ST_DOSELECTION & (WM_LBUTTONUP || WM_RBUTTONUP )
    case A_9_14 :   // ST_SELECTEDPARA & WM_LBUTTONUP (was b90)
    case A_10_14m : // ST_WAIT3RDBTNDOWN & WM_MOUSEMOVE
    case A_12_14 :  // ST_MAYSELECT2 & WM_LBUTTONUP (was c50)

        if (  _pManager->IsInCapture()  )
            ReleaseCapture();
            
        if ( _pManager->IsInTimer() )
            StopTimer();

        if ( Action == A_6_14 )
        {
            hr = DoSelection( pMessage, TRUE );           

#ifdef UNIX // Now we have a selection we can paste using middle button paste.
            if (CheckSelectionWasReallyMade())
            {
                if (!_hwndDoc)
                    _pManager->GetEditor()->GetViewServices()->GetViewHWND(&_hwndDoc);
                SendMessage(_hwndDoc, WM_GETTEXTPRIMARY, (WPARAM)&_firstMessage, IDM_CLEARSELECTION);
            }
#endif
        }

        BecomePassive( & eNotify );
        break;

    case A_6_6: // ST_DOSELECTION & WM_TIMER
        // Convert to correct coordinates
        GetMousePoint( &pMessage->pt, TRUE );
        //else            
        //  fall through

        
    case A_6_6m: // ST_DOSELECTION & WM_MOUSEMOVE

        DoSelection (pMessage);

        // Remain in same state
        break;

    case A_10_9: // ST_WAIT3RDBTNDOWN & WM_LBUTTONDOWN
        if ( _pManager->IsInTimer() )
            StopTimer();
        DoSelectParagraph( pMessage );
        SetState( ST_SELECTEDPARA );
        break;

    case A_11_6: // ST_MAYSELECT1 & WM_MOUSEMOVE (was c40)
        // if the bubblable onselectstart event is cancelled, don't
        // go into DoSelection State, and cnx the tracker
        if ( ! FireSelectStartMessage( pMessage ) )
        {
            if (  _pManager->IsInCapture()  )
                ReleaseCapture();
            
            EndAndStartCaret( &eNotify );
            break;
        }
        // else not canceled so fall through

    case A_8_6 : // ST_SELECTEDWORD & WM_MOUSEMOVE
    case A_12_6: // ST_MAYSELECT2 & WM_MOUSEMOVE (was c60)
        if ( _pManager->IsInTimer() )
            StopTimer();
        DoSelection (pMessage);
        SetState( ST_DOSELECTION );
        break;

    case A_9_6: // ST_SELECTEDPARA & WM_MOUSEMOVE
        DoSelection (pMessage);
        SetState( ST_DOSELECTION );
        break;

    case A_8_10: // ST_SELECTEDWORD & WM_LBUTTONUP
        SetState( ST_WAIT3RDBTNDOWN );
        break;

    case A_1_14  : // ST_WAIT1          & WM_RBUTTONUP
    case A_2_14r : // ST_DRAGOP         & WM_RBUTTONUP
    case A_4_14  : // ST_WAITBTNDOWN1   & WM_KEYDOWN
    case A_7_14  : // ST_WAITBTNDOWN2   & WM_KEYDOWN
                   // ST_WAITBTNDOWN2   & WM_MOUSEMOVE
    case A_10_14 : // ST_WAIT3RDBTNDOWN & WM_KEYDOWN
    case A_11_14 : // ST_MAYSELECT1     & WM_LBUTTONUP (was c30)
        if (  _pManager->IsInCapture()  )
            ReleaseCapture();
            
        if ( _pManager->IsInTimer() )
            StopTimer();

        if ( _fMadeSelection )
        {
            BecomePassive( & eNotify );
        }
        else
        {
            // for a LButtonUp we want to send a click event, which
            // requries an S_OK for PumpMessage to do this.
            hr = (Action==A_11_14) ? S_OK : S_FALSE;            
            EndAndStartCaret( & eNotify );               
        }

        break;


            
    // ST_KEYDOWN
    // Become Passive & Handle the Key. 
    //
    // This is a "psuedo-state" - we really transition to ST_PASSIVE
    // however - we do a HandleKeyDown as well.
    //
    //
    case A_3_15:
    case A_4_15:
    case A_5_15:
    case A_7_15:
    case A_8_15:
    case A_10_15:
    case A_12_15:
    case A_9_15:
    case A_1_15:

        //
        // Ignore the Control Key, as it's used in Drag & Drop or during selection etc.
        //
        if ( pMessage->fCtrl )
            break;
            
        BecomePassive( & eNotify );
        
        hr = HandleKeyDown(
                    pMessage,
                    & followUpAction ,
                    & eNotify );        
        

        break;
        
    default:
        AssertSz (0, "Should never come here!");
        break;
    }
    
    if ( ( _fState == ST_DRAGOP ) && 
         ( ! fHandledDrag ) )
    {
        BecomePassive(); // DO NOT PASS A NOTIFY CODE HERE - IT WILL BREAK DRAG AND DROP
        followUpAction |= FOLLOW_UP_ACTION_DragElement;
    }

    WHEN_DBG( DumpSelectState( pMessage, Action, followUpAction  ) );

    if ( peTrackerNotify )
        *peTrackerNotify = eNotify ;
    if ( ( pdwFollowUpAction ) && ( followUpAction != FOLLOW_UP_ACTION_None ) )
        *pdwFollowUpAction |=  followUpAction;

    RRETURN1 ( hr, S_FALSE );

}

//+====================================================================================
//
// Method: DoTimerDrag
//
// Synopsis: Do a drag from a WM_TIMER message
//
//------------------------------------------------------------------------------------

HRESULT
CSelectTracker::DoTimerDrag( )
{
    HRESULT hr = S_OK;
    IHTMLElement* pIDragElement = NULL;

    IFC(GetViewServices()->CurrentScopeOrSlave(_pStartPointer, & pIDragElement ));
    IFC( GetViewServices()->DragElement( pIDragElement, 0 ));
Cleanup:
    ReleaseInterface( pIDragElement );
    RRETURN ( hr );
}

HRESULT
CSelectTracker::HandleMessage(
    SelectionMessage *pMessage,
    DWORD* pdwFollowUpAction,
    TRACKER_NOTIFY * peTrackerNotify )
{
    HRESULT hr = S_FALSE;
    
    switch( pMessage->message )
    {
        case WM_RBUTTONUP:
        {
            BOOL fRightOfCp;
            BOOL fValidTree;
            //
            // BUGBUG. IE 4.0.1 would allow selection via Right Button click.
            // For now ( beta2) - we will allow transitioning to a caret on RBUTTON_DOWN
            // only if the selection is outside us.
            // Post beta 2 we need to implement all of the IE4 behavior with RBUTTON
            //
            if ( ! _pManager->IsMessageInSelection(  pMessage ) )
            {
                IGNORE_HR( GetViewServices()->MoveMarkupPointerToMessage( _pStartPointer,
                                                                 pMessage,
                                                                 & _fNotAtBOL ,
                                                                 & _fAtLogicalBOL ,
                                                                 & fRightOfCp ,
                                                                 & fValidTree,
                                                                 FALSE,
                                                                 GetEditableElement(),
                                                                 NULL,
                                                                 FALSE  ));

                IGNORE_HR( MoveEndToPointer( _pStartPointer ));
                _pManager->CopyTempMarkupPointers( _pStartPointer, _pEndPointer);

                *peTrackerNotify = TN_END_TRACKER_POS_CARET;
            }
        }
        break;
        
        case WM_LBUTTONUP:
            // we need to fire a click event for things like shift-click
            *pdwFollowUpAction = FOLLOW_UP_ACTION_OnClick;
            // fallthrough

        case WM_LBUTTONDOWN:
        case WM_TIMER:
        case WM_LBUTTONDBLCLK :
        case WM_MOUSEMOVE:
            if ( IsPassive() )
            {
                if ( ( pMessage->message == WM_LBUTTONDOWN) && ( IsShiftKeyDown() ) )
                {
                    IFC(DoSelection( pMessage));
                    BOOL fEqual ;
                    IFC( _pStartPointer->IsEqualTo( _pEndPointer, & fEqual ));
                    if ( fEqual )
                        BecomePassive( peTrackerNotify );
                }  
                Assert( _fState == ST_PASSIVE ); // make sure we're still passive
                
                //
                // If we get here - we're passive. Make sure we're not in capture
                //
                if (  _pManager->IsInCapture()  )
                    ReleaseCapture();
            
                if ( _pManager->IsInTimer() )
                    StopTimer();                  
            }
            else
            {
                hr = HandleMessagePrivate( pMessage, pdwFollowUpAction, peTrackerNotify  );
            }
            break;

        case WM_CHAR:

            hr = THR( HandleChar( pMessage, pdwFollowUpAction, peTrackerNotify  ));
            break;


        case WM_KEYDOWN:
#if DBG == 1    
            if ( pMessage->wParam == VK_F11 )
            {
                DebugBreak();
            }
#endif        
            if ( IsPassive() )      
                hr = THR( HandleKeyDown( pMessage, pdwFollowUpAction, peTrackerNotify  ));
            else
                hr = THR( HandleMessagePrivate( pMessage, pdwFollowUpAction, peTrackerNotify ));
            break;

        case WM_KEYUP:
            if ( IsPassive() )
                hr = THR ( HandleKeyUp ( pMessage, pdwFollowUpAction, peTrackerNotify  ));
            break;

        case WM_KILLFOCUS:
            //
            // A focus change has occured. If we have capture - this is a bad thing.
            // a sample of this is throwing up a dialog from a script.
            //
            if ( ! IsPassive() )
            {            
                if ( _fInFireOnSelectStart )
                {
                    //
                    // If we are in a FireOnSelectStart - gracefully bail.
                    //
                    _fFailFireOnSelectStart = TRUE;
                }
                else
                {
                    BecomePassive(peTrackerNotify);
                }
            }                
            break;

#ifndef NO_IME
        case WM_IME_STARTCOMPOSITION:
            hr = THR( HandleImeStartComposition( pMessage, pdwFollowUpAction, peTrackerNotify ));
            break;

#if defined(IME_RECONVERSION)
        case WM_IME_NOTIFY:
            hr = THR( HandleImeNotify( pMessage, peTrackerNotify ) );
            break;
            
        case WM_IME_REQUEST:            // Reconvert
            hr = THR(HandleImeRequest( pMessage, peTrackerNotify ));
            break;
#endif // IME_RECONVERSION
#endif // !NO_IME
    }
Cleanup:
    RRETURN1( hr, S_FALSE );
}


HRESULT
CSelectTracker::Position(
                IMarkupPointer* pStart,
                IMarkupPointer* pEnd,
                BOOL            fNotAtBOL,
                BOOL            fAtLogicalBOL )
{
    HRESULT hr = S_OK;
    POINTER_GRAVITY eGravity;
    BOOL fAdjust = FALSE;
    HTMLPtrDispInfoRec  pt;
#if DBG==1
    BOOL fPositioned = FALSE;
    hr = _pStartPointer->IsPositioned(& fPositioned );
    Assert( ! fPositioned);
#endif

    _fNotAtBOL = fNotAtBOL;
    _fAtLogicalBOL = fAtLogicalBOL;
    
    //
    // We assume that you can only set a position on a New Tracker.
    //
    Assert( ! _fAddedSegment );

    IFC( _pStartPointer->MoveToPointer( pStart ));
    IFC( MoveEndToPointer( pEnd ));

    //
    // copy gravity - important for commands
    //
    IFC( pStart->Gravity( &eGravity ));            // need to maintain gravity
    IFC(_pStartPointer->SetGravity( eGravity ));
    IFC(pEnd->Gravity( &eGravity ));            // need to maintain gravity
    IFC(_pEndPointer->SetGravity( eGravity ));

    if (SUCCEEDED(GetViewServices()->GetLineInfo(_pEndPointer, _fNotAtBOL, &pt)))
    {
        _curMouseY = pt.lBaseline;
        _curMouseX = pt.lXPosition;
    }

    if ( _fShift )
    {
        IFC( AdjustSelection( & fAdjust ));
    }        
    IFC( ConstrainSelection(TRUE) );
    IFC( _pSelRenSvc->AddSegment( _pStartPointer, _pEndPointer, HIGHLIGHT_TYPE_Selected, & _iSegment ));

    SetMadeSelection( TRUE );
    if ( ! hr )
        _fAddedSegment = TRUE;
    else
        _fAddedSegment = FALSE;
#if DBG == 1
    {
        int iSegmentCount = 0;
        _pSelRenSvc->GetSegmentCount( & iSegmentCount, NULL );
        Assert( iSegmentCount > 0 );
    }
#endif

Cleanup:

    RRETURN ( hr );

}

BOOL
CSelectTracker::IsPassive()
{
    return ( _fState == ST_PASSIVE );
}


VOID
CSelectTracker::BecomePassive(TRACKER_NOTIFY * peTrackerNotify )
{
    SetState( ST_PASSIVE );

    if ( _pManager->IsInTimer() )
        StopTimer();
    if (  _pManager->IsInCapture()  )
        ReleaseCapture();

    if ( !_fInFireOnSelectStart && !CheckSelectionWasReallyMade() )
    {    
        if ( peTrackerNotify  )
        {
            _pManager->CopyTempMarkupPointers( _pStartPointer, _pEndPointer );
            *peTrackerNotify = TN_END_TRACKER_POS_CARET;            
        }        
    }
}

HRESULT
CSelectTracker::Notify(
                TRACKER_NOTIFY inNotify,
                SelectionMessage *pMessage,
                DWORD* pdwFollowUpAction,
                TRACKER_NOTIFY * peTrackerNotify  )
{
    HRESULT hr = S_OK;
    DWORD followUpAction = FOLLOW_UP_ACTION_None;
    
    switch ( inNotify )
    {
        case TN_END_TRACKER_NO_CLEAR:       
            BecomePassive(); 
            break;
            
        case TN_END_TRACKER:
            BecomePassive();          
            if ( pMessage )
            {
                if (! _pManager->IsMessageInSelection( pMessage ))
                    hr = ClearSelection();
            }
            else           
                hr = ClearSelection();   

            break;
            
        case TN_TIMER_TICK:
            hr = OnTimerTick( pMessage, pdwFollowUpAction, peTrackerNotify );
            break;

        default:
            AssertSz(0, "Unexpected Notification");
    }
    if ( followUpAction != FOLLOW_UP_ACTION_None )
    {
        Assert( pdwFollowUpAction );
        *pdwFollowUpAction |= followUpAction;
    }        
        
    RRETURN1( hr, S_FALSE );
}

VOID
CSelectTracker::SetMadeSelection( BOOL fMadeSelection )
{
    if ( ! _fMadeSelection )
    {
        _pManager->TrackerNotify( TN_HIDE_CARET, NULL, NULL) ;
    }
    _fMadeSelection = fMadeSelection;
}


//+====================================================================================
//
// Method: ShouldBeginSelection
//
// Synopsis: We don't want to start selection in Anchors, Images etc.
//
//------------------------------------------------------------------------------------
BOOL
CSelectTracker::ShouldStartTracker(
                CSelectionManager* pManager,
                SelectionMessage* pMessage,
                ELEMENT_TAG_ID eTag,
                IHTMLElement* pIElement,
                IHTMLElement** ppIEditThisElement /*= NULL */
                )
{
    BOOL fShouldStart = FALSE;
    BOOL fNoScope = FALSE;
    IHTMLInputElement* pInputElement = NULL;
    BSTR bstrType = NULL;

    fShouldStart = pManager->GetViewServices()->AllowSelection( pIElement, pMessage ) == S_OK;
    if ( ! fShouldStart )
        goto Cleanup;
        
    fShouldStart = pManager->IsMessageInSelection( pMessage) ;
    if ( fShouldStart )
        goto Cleanup;
        
    IGNORE_HR( pManager->GetViewServices()->IsNoScopeElement( pIElement, & fNoScope ));
    if ( ( fNoScope ) && ( eTag != TAGID_INPUT ) ) // inputs can have selection but are no-scopes
        goto Cleanup;
        
    Assert( !pManager->IsIMEComposition());

    switch ( eTag )
    {
        case TAGID_BUTTON:
        {
            fShouldStart = pManager->IsContextEditable();
        }
        break;
        
        case TAGID_INPUT:
        {
            
            //
            // for input's of type= image, or type=checkbox - we don't want to start a selection 
            // BUGBUG - (krisma) Not sure why this is here. We shouldn't be here if we're an 
            // image or option button, should we? I'll follow up with MarkA.
            //
            if ( ( eTag == TAGID_INPUT ) 
                && S_OK == THR( pIElement->QueryInterface ( IID_IHTMLInputElement, ( void** ) & pInputElement ))
                && S_OK == THR(pInputElement->get_type(&bstrType)))
            {
                if (!_tcsicmp( bstrType, TEXT("image")) || !_tcsicmp(bstrType, TEXT("radio"))
                    || !_tcsicmp(bstrType, TEXT("checkbox")))
                {
                    fShouldStart = FALSE;
                }
                else if ( !_tcsicmp( bstrType, TEXT("button")) && 
                          !pManager->IsContextEditable() )
                {
                    //
                    // disallow selecting in a button if it's not editable.
                    //
                    fShouldStart = FALSE;
                }
                else
                {
                    fShouldStart = TRUE; // allow selection in all inputs - so an Input of READONLY works
                }
                
            }
            break;
        }
        
        default:
            fShouldStart = TRUE;
            break;
    }

Cleanup:
    ReleaseInterface( pInputElement );
    SysFreeString(bstrType);
    return fShouldStart;
}



//+====================================================================================
//
// Method: HandleChar
//
// Synopsis: Delete the Selection, and cause this tracker to end ( & kill us ).
//
//------------------------------------------------------------------------------------

HRESULT
CSelectTracker::HandleChar(
                SelectionMessage *pMessage,
                DWORD* pdwFollowUpAction,
                TRACKER_NOTIFY * peTrackerNotify )
{
    HRESULT         hr = S_OK ;
    SP_IHTMLElement spFlowElement;
    BOOL            bMultiLine = FALSE;
    Assert(IsPassive());
    if ( _pManager->IsContextEditable() && IsPassive() )
    {
        if ( PointersInSameFlowLayout( _pStartPointer, _pEndPointer, NULL , GetViewServices() ))
        {
            if ( pMessage->wParam == VK_ESCAPE )
                goto Cleanup; // BAIL - we don't want to type a character
                
            if (pMessage->wParam == VK_RETURN)
            {
                Assert(_pStartPointer);
                IFR( _pManager->GetViewServices()->GetFlowElement(_pStartPointer, &spFlowElement) );
                if ( spFlowElement )
                {
                    IFR( _pManager->GetViewServices()->IsMultiLineFlowElement( spFlowElement, &bMultiLine) );
                }
                
                if (bMultiLine)
                    *peTrackerNotify =  TN_FIRE_DELETE_REBUBBLE;
                else
                    return S_FALSE;
            }
            else
            {
                // BUGBUG (JohnBed) We should only allow VK_TAB to go through if the caret would
                // have accepted it (after delete, the caret would be placed inside a <PRE>) we
                // should just check both end points of selection to see if they are in a pre
                // for now.
                if( pMessage->wParam >= ' ' || pMessage->wParam == VK_TAB )
                    *peTrackerNotify =  TN_FIRE_DELETE_REBUBBLE;
            }
        }
    }
Cleanup:

    RRETURN1( hr, S_FALSE);
}


HRESULT
CSelectTracker::HandleKeyUp(
                SelectionMessage *pMessage,
                DWORD* pdwFollowUpAction,
                TRACKER_NOTIFY * peTrackerNotify)
{
    HRESULT hr = S_FALSE;
    DWORD followUpAction = FOLLOW_UP_ACTION_None;
    
    if ( ( pMessage->wParam == VK_SHIFT ) && ( _fShift ) )
    {        
        BecomePassive(peTrackerNotify);
        _fShift = FALSE;

        if (  _pManager->IsInCapture()  )
            ReleaseCapture();

        if ( _pManager->IsInTimer() )
            StopTimer();
        hr = S_OK;        
    }

    if ( followUpAction != FOLLOW_UP_ACTION_None )
        *pdwFollowUpAction |= followUpAction;

    RRETURN1( hr, S_FALSE);
}


HRESULT
CSelectTracker::HandleKeyDown(
                SelectionMessage *pMessage,
                DWORD* pdwFollowUpAction,
                TRACKER_NOTIFY * peTrackerNotify)
{
    HRESULT hr = S_FALSE;
    DWORD followUpAction = FOLLOW_UP_ACTION_None;
    IMarkupPointer* pStart = NULL;
    CSpringLoader * psl;
    int iWherePointer = SAME;
    long eHTMLDir = htmlDirLeftToRight;

    Assert( IsPassive());

    if ( _pManager->IsContextEditable() )
    {

        switch(pMessage->wParam)
        {
        
            case VK_BACK:
            case VK_F16:
            {
                if ( _pManager->IsContextEditable() )
                {
                    psl = GetSpringLoader();        
                    if (psl && _pStartPointer)
                    {
                        BOOL fDelaySpringLoad;

                        IFC( MustDelayBackspaceSpringLoad(psl, _pStartPointer, &fDelaySpringLoad) );
                    
                        if (!fDelaySpringLoad)
                            IFC( psl->SpringLoad(_pStartPointer, SL_TRY_COMPOSE_SETTINGS | SL_RESET) );
                    }

                    if (pMessage->fCtrl)
                    {
                        //
                        // BUGBUG - make this use GetMoveDirection - after your checkin
                        //
                        IFC( OldCompare( _pStartPointer, _pEndPointer, & iWherePointer ));
                        if ( iWherePointer == RIGHT )
                        {
                            _pManager->CopyTempMarkupPointers( _pStartPointer, _pEndPointer ) ;
                        }
                        else
                        {
                            _pManager->CopyTempMarkupPointers( _pEndPointer, _pStartPointer );
                        }
                        *peTrackerNotify = TN_END_TRACKER_POS_CARET_REBUBBLE;        
                    }
                    else
                    {
                        if ( CheckSelectionWasReallyMade())
                        {
                            IFC( _pManager->TrackerNotify( TN_FIRE_DELETE, NULL, NULL ) );
                        }
                        else
                        {
                            _pManager->CopyTempMarkupPointers( _pStartPointer, _pEndPointer );
                            *peTrackerNotify = TN_END_TRACKER_POS_CARET_REBUBBLE;        
                        }
                    }                    
                }                        
            }
            break;
            
            case VK_LEFT:
            case VK_RIGHT:   
                IFC( GetViewServices()->GetLineDirection( _pEndPointer , _fNotAtBOL, &eHTMLDir ));
                // don't put a break here. We want to fall through 
            case VK_PRIOR:
            case VK_NEXT:
            case VK_HOME:
            case VK_END:        
            case VK_UP:
            case VK_DOWN:
            {
                if ( IsShiftKeyDown() )
                {
                    _fShift = TRUE;

                    hr = MoveSelection( CCaretTracker::GetMoveDirectionFromMessage( pMessage, (eHTMLDir == htmlDirRightToLeft) ) );

                    //
                    // If the selection pointers collapse to the same point - we will start a caret.
                    //
                    IFC( OldCompare( _pStartPointer, _pEndPointer, & iWherePointer ));
                    if ( iWherePointer == SAME )
                    {
                        IFC( _pManager->CopyTempMarkupPointers( _pStartPointer, _pStartPointer ));
                        ClearSelection();
                        *peTrackerNotify = TN_END_TRACKER_POS_CARET; // this will move the caret to the pointers we just set              
                    }
                }
                else if ( _pManager->IsContextEditable())
                {
                    IFC( GetMarkupServices()->CreateMarkupPointer( &pStart ));
                    IFC( GetCaretStartPoint( CCaretTracker::GetMoveDirectionFromMessage( pMessage, (eHTMLDir == htmlDirRightToLeft) ) , 
                                             pStart ));
                    IFC( _pManager->CopyTempMarkupPointers( pStart, pStart ));
                    
                    *peTrackerNotify = TN_END_TRACKER_POS_CARET; // this will move the caret to the pointers we just set              
                }
            }         
            break;

#ifndef NO_IME
            case VK_KANJI:
                if (   949 == GetKeyboardCodePage()
                    && _pManager->IsContextEditable()
                    && EndPointsInSameFlowLayout())
                {
                    BOOL fEndAfterStart;

                    IFC( _pEndPointer->IsRightOf( _pStartPointer, &fEndAfterStart ) );
                    _pManager->StartHangeulToHanja(peTrackerNotify,
                                                   fEndAfterStart
                                                   ? _pStartPointer
                                                   : _pEndPointer);
                }
                hr = S_OK;
                break;
#endif // !NO_IME
        }
    }

    if ( followUpAction != FOLLOW_UP_ACTION_None )
    {
        *pdwFollowUpAction |= followUpAction;
    }

Cleanup:
    ReleaseInterface( pStart );

    
    return ( hr );
}

#ifndef NO_IME
CSpringLoader *
CIme::GetSpringLoader()
{
    CSpringLoader * psl = NULL;
    CHTMLEditor   * pEditor;

    if (!_pManager)
        goto Cleanup;

    pEditor = _pManager->GetEditor();

    if (!pEditor)
        goto Cleanup;

    psl = pEditor->GetPrimarySpringLoader();

Cleanup:
    return psl;
}

HRESULT
CSelectTracker::HandleImeStartComposition(
    SelectionMessage *pMessage, 
    DWORD* pdwFollowUpAction, 
    TRACKER_NOTIFY * peTrackerNotify )
{
    // We want to kill the current selection.
    // Bubble on up: CSelectTracker -> CCaretTracker -> CImeTracker

    if( PointersInSameFlowLayout( _pStartPointer, _pEndPointer, NULL , GetViewServices() ))
    {
        if ( _pManager->IsContextEditable() )
        {
            *peTrackerNotify = TN_FIRE_DELETE_REBUBBLE;
        }

    }
    
    return S_OK;
}
#endif

//+====================================================================================
//
// Method: GetCaretStartPoint
//
//------------------------------------------------------------------------------------

HRESULT
CSelectTracker::GetCaretStartPoint( 
                                CARET_MOVE_UNIT inCaretMove, 
                                IMarkupPointer* pCopyStart )
{
    HRESULT     hr;
    int         iWherePointer;
    Direction   dir;
    
    //
    // Init pCopyStar to the pointer we need to move
    //

    dir = GetPointerDirection(inCaretMove);
    
    IFC( OldCompare( _pStartPointer, _pEndPointer, & iWherePointer ));
    if ( (iWherePointer == RIGHT && dir == LEFT)
         || (iWherePointer == LEFT && dir == RIGHT) )
    {
        IFC( pCopyStart->MoveToPointer( _pStartPointer ) );
    }    
    else
    {
        IFC( pCopyStart->MoveToPointer( _pEndPointer ) );
    }

    //
    // Adjust the pointer
    //
    
    switch( inCaretMove )
    {
        case CARET_MOVE_LEFT:
        case CARET_MOVE_RIGHT:
            break; // we're done

        case CARET_MOVE_WORDLEFT:
        case CARET_MOVE_WORDRIGHT:
            if (IsAtWordBoundary(pCopyStart))
                break; // we're done

            // fall through

        default:
            //
            // boundary cases are excepted to return failure codes but don't move the pointer
            //
            
            IGNORE_HR( MovePointer(inCaretMove, pCopyStart, &_curMouseX, &_fNotAtBOL, &_fAtLogicalBOL) );
    }
            
Cleanup:
    RRETURN( hr );
}

HRESULT
CSelectTracker::MovePointersToTrueStartAndEnd( 
    IMarkupPointer* pTrueStart, 
    IMarkupPointer* pTrueEnd, 
    BOOL *pfSwap ,
    BOOL *pfEqual /*= NULL*/ )
{
    HRESULT hr = S_OK;
    int iWherePointer = SAME;
    BOOL fSwap = FALSE;
    
    IFC( OldCompare( _pStartPointer, _pEndPointer, & iWherePointer ));
    if ( iWherePointer == RIGHT )
    {
        IFC( pTrueStart->MoveToPointer( _pStartPointer));
        IFC( pTrueEnd->MoveToPointer( _pEndPointer));
    }
    else
    {
        IFC( pTrueStart->MoveToPointer( _pEndPointer));
        IFC( pTrueEnd->MoveToPointer( _pStartPointer));
        fSwap = TRUE;
    }
Cleanup:
    if ( pfSwap )
        *pfSwap = fSwap;
    if ( pfEqual )
        *pfEqual = ( iWherePointer == 0);
        
    RRETURN ( hr );
}

//+====================================================================================
//
// Method: IsBetweenBlocks
//
// Synopsis: Look for the Selection being in the "magic place" at the end of a block.
//
//------------------------------------------------------------------------------------

//
// Ignore BR's !
//
BOOL
CSelectTracker::IsBetweenBlocks( IMarkupPointer* pPointer )
{
    HRESULT hr;
    BOOL fBetween = FALSE;
    DWORD dwBreakCondition = 0;
    
    CEditPointer scanPointer( GetEditor());
    IFC( scanPointer.MoveToPointer( pPointer ));
    IFC( scanPointer.Scan( LEFT, BREAK_CONDITION_BlockScan, & dwBreakCondition ));

    if ( scanPointer.CheckFlag( dwBreakCondition, 
                                BREAK_CONDITION_EnterBlock ) )
    {
        IFC( scanPointer.MoveToPointer( pPointer ));
        IFC( scanPointer.Scan( RIGHT , 
                               BREAK_CONDITION_BlockScan, 
                               & dwBreakCondition ));
                               
        if ( scanPointer.CheckFlag( dwBreakCondition, 
                                    BREAK_CONDITION_EnterBlock ))
        {
            fBetween = TRUE;
        }
    }

Cleanup:
    return fBetween;
}

//+====================================================================================
//
// Method: IsAtEdgeOfTable
//
// Synopsis: Are we at the edge of the table ( ie. by scanning left/right do we hit a TD ?)
//
//------------------------------------------------------------------------------------


BOOL
CSelectTracker::IsAtEdgeOfTable( Direction iDirection, IMarkupPointer* pPointer )
{
    HRESULT hr;
    BOOL fAtEdge = FALSE;
    DWORD dwBreakCondition = 0;
    IHTMLElement* pIElement = NULL;
    ELEMENT_TAG_ID eTag = TAGID_NULL;
    
    CEditPointer scanPointer( GetEditor());
    IFC( scanPointer.MoveToPointer( pPointer ));
    IFC( scanPointer.Scan( iDirection, 
                           BREAK_CONDITION_BlockScan - BREAK_CONDITION_Block , 
                           & dwBreakCondition,
                           & pIElement));

    if ( scanPointer.CheckFlag( dwBreakCondition, 
                                BREAK_CONDITION_ExitSite ) )
    {
        IFC( GetMarkupServices()->GetElementTagId( pIElement, & eTag ));
                               
        if ( IsTablePart( eTag ))
        {
            fAtEdge = TRUE;
        }
    }

Cleanup:
    ReleaseInterface( pIElement );
    
    return fAtEdge;
}

//+====================================================================================
//
// Method: MoveSelection
//
// Synopsis: Adjust a Selection's start/end points based on the CaretMoveUnit ( used
//           for keyboard navigation in shift selection).
//
//------------------------------------------------------------------------------------



HRESULT
CSelectTracker::MoveSelection(  CARET_MOVE_UNIT inCaretMove )
{
    HRESULT             hr = S_OK;
    LONG                curMouseX;
    HTMLPtrDispInfoRec  pt;
    POINT ptLoc;
    BOOL fBetweenBlocks = FALSE;
    //
    // Move end of selection
    //
    GetLocation( &ptLoc);
    curMouseX = ptLoc.x;

    fBetweenBlocks = IsBetweenBlocks( _pEndPointer );

    if ( FAILED( MovePointer(inCaretMove, _pEndPointer, &curMouseX, &_fNotAtBOL, &_fAtLogicalBOL)) &&
        ( ! fBetweenBlocks  || inCaretMove == CARET_MOVE_UP || inCaretMove == CARET_MOVE_DOWN ) )
    {
        //
        // Ignore HR code here - we still want to constrain, adjust and update.
        //
        if (GetPointerDirection(inCaretMove) == RIGHT)
        {
            IGNORE_HR(MovePointer(CARET_MOVE_LINEEND, _pEndPointer, &curMouseX, &_fNotAtBOL, &_fAtLogicalBOL));
        }
        else
        {
            IGNORE_HR(MovePointer(CARET_MOVE_LINESTART, _pEndPointer, &curMouseX, &_fNotAtBOL, &_fAtLogicalBOL));
        }
    }        
    
    //
    // If we were between blocks - and we moved right one character
    // OR we are at the edge of a table - and we moved left/right one character
    // 
    // MovePointer has no moved us to the start of the next line
    // We need to go one more character to the left/right
    //
    if (( fBetweenBlocks && inCaretMove == CARET_MOVE_RIGHT ) ||
        (( inCaretMove == CARET_MOVE_RIGHT || inCaretMove == CARET_MOVE_LEFT ) && 
           IsAtEdgeOfTable( Reverse( GetPointerDirection(inCaretMove )) , _pEndPointer )))
    {
        IFC( MovePointer(inCaretMove, _pEndPointer, &curMouseX, &_fNotAtBOL, &_fAtLogicalBOL));
    }
    
    //
    // Constrain and scroll
    //
    IFC( ConstrainSelection() );
    SetLastCaretMove( inCaretMove );    
    IFC( AdjustSelection(NULL));    
    IFC( ScrollPointerIntoView(_pEndPointer, _fNotAtBOL ) );

    //
    // Update _curMouseX/Y
    //
    IFC( GetViewServices()->GetLineInfo( _pEndPointer , _fNotAtBOL , & pt )); 
    _curMouseY = pt.lBaseline;
    if (curMouseX == CARET_XPOS_UNDEFINED)
        _curMouseX = pt.lXPosition;

    //
    // Update segment list
    //
    IFC( _pSelRenSvc->MoveSegmentToPointers( _iSegment, _pStartPointer, _pEndPointer, HIGHLIGHT_TYPE_Selected ));

Cleanup:
    RRETURN ( hr );
}
    
//+====================================================================================
//
// Method: OnTimerTick
//
// Synopsis: Callback from Trident - for WM_TIMER messages.
//
//------------------------------------------------------------------------------------

HRESULT
CSelectTracker::OnTimerTick(
                SelectionMessage *pMessage,
                DWORD* pdwFollowUpAction,
                TRACKER_NOTIFY * peTrackerNotify )
{
    HRESULT                 hr = S_OK;
    DWORD followUp = FOLLOW_UP_ACTION_None;

    if ( _fInFireOnSelectStart )
        return S_OK; // do nothing for timer ticks when we're in the middle of OnSelectStart
    
    StopTimer();

    switch (_fState)
    {
    case ST_WAIT1:  // === A_1_3
        SetState( ST_MAYDRAG ) ;
        break;

    case ST_WAITBTNDOWN1: // === A_4_14t == A_4_14m
        // In this case, the cp is *never* updated to the
        // new position. So go and update it. Remember SetCaret
        // also kills any existing selections.

        if (  _pManager->IsInCapture()  )
            ReleaseCapture();
        ClearSelection();
        hr = THR( _pManager->CopyTempMarkupPointers( _pStartPointer, _pEndPointer ));
        *peTrackerNotify = TN_END_TRACKER_POS_CARET; // this will move the caret to the pointers we just set              


        break;

    case ST_WAIT2:  // === A_5_11
        SetState( ST_MAYSELECT1 );
        break;

    case ST_WAITBTNDOWN2:   // === A_7_14
    case ST_WAIT3RDBTNDOWN: // === A_10_14

        if (  _pManager->IsInCapture()  )
            ReleaseCapture();


        if (( _fMadeSelection ) && ( CheckSelectionWasReallyMade() ))
        {
            BecomePassive(peTrackerNotify);
        }
        else
        {
            hr = THR( _pManager->CopyTempMarkupPointers( _pStartPointer, _pEndPointer ));
            *peTrackerNotify = TN_END_TRACKER_POS_CARET; // this will move the caret to the pointers we just set              
        }
        break;

    case ST_SELECTEDWORD:   // === A_8_12
        SetState( ST_MAYSELECT2 ) ;
        break;

    case ST_PASSIVE:
        if (  _pManager->IsInCapture()  )
            ReleaseCapture();
        break;
    // ST_START, ST_DRAGOP, ST_MAYDRAG, ST_DOSELECTION,
    // ST_SELECTEDPARA, ST_MAYSELECT1, ST_MAYSELECT2
    default:
        AssertSz (0, "Invalid state & message combination!");
        break;
    }
    if ( pdwFollowUpAction )
        *pdwFollowUpAction |= followUp;

    WHEN_DBG( DumpSelectState( pMessage, A_UNK, followUp , TRUE ) );

    RRETURN(hr);
}

//+=================================================================================================
// Method: Begin Selection
//
// Synopsis: Do the things you want to do at the start of selection
//             This will involve updating the mouse position, and telling the Selection to add a Segment
//
//--------------------------------------------------------------------------------------------------

HRESULT
CSelectTracker::BeginSelection(
    SelectionMessage* pMessage,
    DWORD* pdwFollowUpAction,
    TRACKER_NOTIFY * peTrackNotify )
{
    HRESULT hr = S_OK;
    DWORD followUp = 0;
    BOOL fTestNotAtBOL;
    BOOL fTestAtLogicalBOL;
    BOOL fRightOfCp;
    BOOL fValidTree;

    Assert( _pStartPointer );

#if DBG == 1
    VerifyOkToStartSelection(pMessage);
    _ctScrollMessageIntoView = 0;
#endif
    Assert( pMessage->elementCookie != NULL );
    TraceTag(( tagSelectionTrackerState, "\n---BeginSelection---\nMouse:x:%d,y:%d", pMessage->pt.x, pMessage->pt.y));

    IFC( GetViewServices()->MoveMarkupPointerToMessage( _pStartPointer,
                                                     pMessage,
                                                     & _fNotAtBOL ,
                                                     & _fAtLogicalBOL ,
                                                     & fRightOfCp ,
                                                     & fValidTree,
                                                     FALSE,
                                                     GetEditableElement(),
                                                     NULL,
                                                     FALSE )); // we don't test EOL 

    hr = MoveEndToPointer( _pStartPointer );
    if ( hr )
    {
        AssertSz( 0, "CSelectTracker::BeginSelection: Unable to position EndPointer");
        goto Cleanup;
    }

    //
    // BUG BUG - this is a bit expensive - but here I want to hit test EOL
    // and previously I didn't.
    //
    // Why do we want to hit test EOL here ? Otherwise we'll get bogus changes
    // in direction.
    //

    IFC( GetViewServices()->MoveMarkupPointerToMessage( _pTestPointer,
                                                     pMessage,
                                                     & fTestNotAtBOL ,
                                                     & fTestAtLogicalBOL ,
                                                     & fRightOfCp ,
                                                     & fValidTree,
                                                     FALSE,
                                                     GetEditableElement(),
                                                     NULL,
                                                     TRUE )); // we do test EOL 
    IFC( _pPrevTestPointer->MoveToPointer( _pTestPointer ));
    
    //
    // Set the tree the mouse is in.
    //

    if ( ! _fDragDrop )
    {
        hr = ConstrainSelection( TRUE , & pMessage->pt );    
        if ( ! hr )
            hr = _pSelRenSvc->AddSegment( _pStartPointer, _pEndPointer, HIGHLIGHT_TYPE_Selected, & _iSegment );
        if ( ! hr )
            _fAddedSegment = TRUE;
        else
            _fAddedSegment = FALSE;

#if DBG == 1
        {
            int iSegmentCount = 0;
            _pSelRenSvc->GetSegmentCount( & iSegmentCount, NULL );
            Assert( iSegmentCount > 0 );
        }
#endif

    }
    
    Assert( ! hr );

    _curMouseX = _anchorMouseX = pMessage->pt.x;
    _curMouseY = _anchorMouseY = pMessage->pt.y;

    if ( pMessage->fCtrl && !_fDragDrop ) // ignore control-click for drag drop - they may wnat to copy
    {
        DoSelectParagraph( pMessage );
        SetState( ST_SELECTEDPARA );    
        _fInSelectParagraph = TRUE;
    }
    else
    {
        StartTimer();
    }        

    TakeCapture();
        
Cleanup:
    if( FAILED(hr ))
    {
        if ( peTrackNotify )
            *peTrackNotify = TN_END_TRACKER;

        if ( _pManager->IsInCapture())
            ReleaseCapture();
        if ( _pManager->IsInTimer())
            StopTimer();
    }

    if ( pdwFollowUpAction )
        *pdwFollowUpAction |= followUp;

    RRETURN ( hr );
}
//+====================================================================================
//
// Method: ConstrainSelection
//
// Synopsis: Constrain a Selection between the editable context
//
//------------------------------------------------------------------------------------


HRESULT
CSelectTracker::ConstrainSelection( BOOL fMoveStart /*=fALSE*/, POINT* pptGlobal /*=NULL*/ )
{
    HRESULT hr = S_OK;
    BOOL fDirection = GetMoveDirection();
    ELEMENT_ADJACENCY eAdj = ELEMENT_ADJACENCY_Max;

    if ( fMoveStart )
    {
        hr = ConstrainPointer( _pStartPointer, fDirection );
    }

    if ( !pptGlobal ||
         !_pManager->IsEditContextPositioned() )
    {
        IFC( ConstrainPointer( _pEndPointer, fDirection ));
    }
    else if ( ! _pManager->IsInEditContext( _pEndPointer ))
    {
        //
        // We do this for positioned elements only. Why ? Because where the pointer is in the stream
        // may not necessarily match where it "looks like" it should be.
        //
        // Most of the time we don't get here. MoveMarkupPointer should have adjusted us perfectly already.
        //
        IFC( GetViewServices()->GetAdjacencyForPoint( 
                            _pManager->GetEditableElement(), 
                            *pptGlobal,
                            & eAdj ));                                  
        
        if ( eAdj == ELEM_ADJ_BeforeBegin )
            eAdj = ELEM_ADJ_AfterBegin;
        else if ( eAdj == ELEM_ADJ_AfterEnd )
            eAdj = ELEM_ADJ_BeforeEnd;


        IFC( _pEndPointer->MoveAdjacentToElement( _pManager->GetEditableElement(), eAdj ));

    }
    
Cleanup:

    RRETURN( hr );
    
}

//+====================================================================================
//
// Method: MoveEndToPointer 
//
// Synopsis: Wrapper To MoveToPointer. Allowing better debugging validation.
//
//------------------------------------------------------------------------------------

HRESULT 
CSelectTracker::MoveEndToPointer( IMarkupPointer* pPointer )
{
    HRESULT hr = S_OK;

#if DBG == 1
    int oldEndCp = 0;
    int startCp = 0;
    int endCp = 0;
    
    if ( IsTagEnabled( tagSelectionValidate ))
    {
        oldEndCp = GetCp( _pEndPointer );
        startCp = GetCp( _pStartPointer );
    }
#endif

    hr = THR ( _pEndPointer->MoveToPointer( pPointer ));

#if DBG == 1
    if ( IsTagEnabled( tagSelectionValidate))
    {
        endCp = GetCp( _pEndPointer );
        if ( endCp != 0 && 
             oldEndCp != -1 && 
            ( endCp - startCp == 0 ) && 
            ( oldEndCp - startCp != 0 ) )
        {
            DumpTree(_pEndPointer);
            AssertSz(0,"Selection jumpted to zero");
        }
    }
#endif

    RRETURN ( hr );
}

//+=================================================================================================
// Method: During Selection
//
// Synopsis: Do the things you want to do at the during a selection
//             Tell the selection to extend the current segment.
//
//--------------------------------------------------------------------------------------------------

HRESULT
CSelectTracker::DoSelection(
                            SelectionMessage* pMessage, 
                            BOOL fAdjustSelection /*=FALSE*/)
{
    HRESULT hr = S_OK;
    Assert( _pEndPointer );
    BOOL fRightOfCp;
    BOOL fValidTree;
    BOOL fAdjustedSel = FALSE;
    int iWherePointer = SAME;
    BOOL fNotAtBOL = TRUE;
    BOOL fAtLogicalBOL = FALSE;
    BOOL fValidLayout = FALSE;
    HRESULT hrTestTree;
    BOOL fInEdit;
    
#if DBG==1
    int endCp = 0;
    int startCp = 0;
    int testCp = 0;
    int oldEndCp = GetCp( _pEndPointer );
    int oldStartCp = GetCp( _pStartPointer );
    int newSelSize = 0;
    int oldSelSize = 0;
    int selDelta = 0;
    
#endif

    Assert( ! _fDragDrop );
    
    if (  fAdjustSelection || 
          ( pMessage->pt.x != _curMouseX) ||
          ( pMessage->pt.y != _curMouseY ) ||
          ( pMessage->message == WM_TIMER ) )  // ignore spurious mouse moves.
    {
        hr = GetViewServices()->MoveMarkupPointerToMessage( _pTestPointer,
                                                             pMessage,
                                                             & fNotAtBOL ,
                                                             & fAtLogicalBOL,
                                                             & fRightOfCp ,
                                                             & fValidTree,
                                                             FALSE  , 
                                                             GetEditableElement(),
                                                             & fValidLayout,
                                                             TRUE );
#if DBG ==1
       testCp = GetCp( _pTestPointer );
       if ( testCp == gDebugTestPointerCp ||
          ( testCp >= gDebugTestPointerMinCp && testCp <= gDebugTestPointerMaxCp ) )
       {
            if ( IsTagEnabled(tagSelectionDumpOnCp))
            {
                DumpTree( _pTestPointer );
            }                
       }
#endif

        if ( hr )
        {
            goto Cleanup;
        }

        //
        // Ensure that we are in the edit context, and that fValidTree is set properly.
        //
        hrTestTree = _pManager->IsInEditContext( _pTestPointer, & fInEdit );
        Assert( fInEdit || hrTestTree || fValidTree );
        if ( hrTestTree )
        {
            fValidTree = FALSE;
        }
        
        if ( !fValidTree )
        {
            fAdjustedSel = ConstrainEndToContainer();
            if ( fAdjustedSel )
            {
                SetMadeSelection( TRUE );
                _curMouseX = pMessage->pt.x;
                _curMouseY = pMessage->pt.y;      
            }
        }
        
        if ( fValidTree || !fAdjustedSel )
        {
            if ( !fValidLayout || !fInEdit )
            {
                ELEMENT_TAG_ID eTag = TAGID_NULL;

                GetTagIdFromMessage( pMessage, 
                                     & eTag, 
                                     _pManager->GetDoc(), 
                                     GetViewServices(), GetMarkupServices() );

                if ( !fInEdit )
                {
                    Assert( ! fInEdit && fValidTree );                    
                    fAdjustedSel = TRUE;

                    //
                    // BUGBUG - I don't think I need this special case anymore.
                    // MoveMarkupPointer should magically be fixing this.
                    // but too much risk to change this now
                    // 
                    
                    
                    //
                    // Just move the End to the Test. Constrain Selection will fix up
                    //
                    IFC( _pEndPointer->MoveToPointer( _pTestPointer ));
                }
                else if ( _pManager->IsContextEditable() || 
                          IsJumpOverAtBrowse( eTag ) ) // we only jump over at edit time or you can jump over this thing.
                { 
                    fAdjustedSel = AdjustForSiteSelectable() ;
                }
                
                if ( fAdjustedSel )
                {
                    SetMadeSelection( TRUE );
                    _curMouseX = pMessage->pt.x;
                    _curMouseY = pMessage->pt.y;         
                    IFC( _pPrevTestPointer->MoveToPointer( _pTestPointer ));
                }
            }

            if ( ! fAdjustedSel )
            {
                _fEndConstrained = FALSE;
                IGNORE_HR( OldCompare( _pEndPointer, _pTestPointer, & iWherePointer )); // DO NOT IFC'IZE this. We want to do the constrain for inputs.
                if ( iWherePointer != SAME  )
                {
#if DBG == 1
                    if ( IsTagEnabled( tagSelectionDisableWordSel ))
                    {
                        MoveEndToPointer( _pTestPointer );
                        _fNotAtBOL = fNotAtBOL;
                        _fAtLogicalBOL = fAtLogicalBOL;
                        fAdjustedSel = TRUE;
                    }
                    else
                    {
#endif
                        if (  _fShift || IsShiftKeyDown() )
                        {
                            MoveEndToPointer( _pTestPointer );
                            _fNotAtBOL = fNotAtBOL;
                            _fAtLogicalBOL = fAtLogicalBOL;
                            fAdjustedSel = TRUE;
                            _fInWordSel = FALSE;
                            _fWordPointerSet = FALSE;
                        }                
                        else 
                        {
                            IGNORE_HR( OldCompare( _pPrevTestPointer, _pTestPointer, & iWherePointer));                       
                            BOOL fDirection = (iWherePointer == SAME ) ?
                                                ( _fInWordSel ? !!_fWordSelDirection : GuessDirection( & pMessage->pt )) : // if in word sel. use _fWordSelDirection, otherwise use mouse direction.
                                                ( iWherePointer == RIGHT ) ;
                            //
                            // BUGBUG. We should be checking _fInParagraph here. We've cut this for now.
                            //
                            Assert( fInEdit );
                            IFC( DoWordSelection( pMessage, & fAdjustedSel, fRightOfCp , fDirection ));
                            WHEN_DBG( if ( _fInWordSel ) ValidateWordPointer( _pEndPointer ));

                        } 
#if DBG == 1
                    }
#endif
                    SetMadeSelection( TRUE );
                    _curMouseX = pMessage->pt.x;
                    _curMouseY = pMessage->pt.y;
                    if ( fAdjustedSel )
                        IFC( _pPrevTestPointer->MoveToPointer( _pTestPointer ));
                }
            }
        }
                        
        if ( fAdjustSelection || fAdjustedSel )
        {
            BOOL fDidAdjust = FALSE;
              
            if ( fAdjustSelection )
                IFC( AdjustSelection( & fDidAdjust ));
                
            if ( fDidAdjust || fAdjustedSel )
            {    
                if (!fInEdit && fValidTree)
                {
                    IFC( ConstrainSelection( FALSE, & pMessage->pt ) );           
                }
#if DBG == 1
               endCp = GetCp( _pEndPointer );
               startCp = GetCp( _pStartPointer );
               oldSelSize = oldEndCp - oldStartCp;
               newSelSize = endCp - startCp;
               selDelta  = newSelSize - oldSelSize;
               
               if ( endCp == gDebugEndPointerCp || 
                  ( ( newSelSize == 0 )  && (  oldSelSize != 0  ) ) )
               {
                    if ( IsTagEnabled(tagSelectionDumpOnCp))
                    {        
                        DumpTree( _pEndPointer );
                    }                        
               }
#endif
               if ( _fDoubleClickWord ) // extra funky IE 4/Word behavior
               {
                   CheckSwap();
               }
               
               IFC( _pSelRenSvc->MoveSegmentToPointers( _iSegment, _pStartPointer, _pEndPointer, HIGHLIGHT_TYPE_Selected   ));
               
               ResetSpringLoader(GetSelectionManager(), _pStartPointer, _pEndPointer);


               IFC( ScrollMessageIntoView( pMessage ));  
                                                                                   
            }            
        }
        else if ( pMessage->message == WM_TIMER ) 
        {
           //
           // Always scroll into view on WM_TIMER. Why ? 
           // So we can scroll the mousepoint into view if necessary
           //

           IFC( ScrollMessageIntoView( pMessage ));  
        }        
    }        

    SetLastCaretMove( CARET_MOVE_NONE ); // Reset this to say we didn't have a MouseMove.
Cleanup:

    RRETURN ( hr );
}


//+====================================================================================
//
// Method: ScrollMessageIntoView.
//
// Synopsis: Wrapper to ScrollPointIntoView
//
//------------------------------------------------------------------------------------

HRESULT
CSelectTracker::ScrollMessageIntoView(            
            SelectionMessage* pMessage )
{
    HRESULT hr = S_OK;
    WHEN_DBG( _ctScrollMessageIntoView++);    
    
    IFC( GetViewServices()->ScrollPointIntoView( _pManager->GetEditableElement(), & pMessage->pt));

#if DBG == 1
    TraceTag((tagShowScrollMsgCount, "ScrollMessageIntoView Count:%ld", _ctScrollMessageIntoView ));
#endif 

Cleanup:
    RRETURN ( hr );
    
}


//+====================================================================================
//
// Method: ScrollPointerIntoView
//
// Synopsis: Wrapper to ScrollPointerIntoView. Check if we're in a good place to scroll from
//           if we aren't then look for text - and scroll from there.
//
//------------------------------------------------------------------------------------


HRESULT
CSelectTracker::ScrollPointerIntoView( 
            IMarkupPointer* pScrollPointer, 
            BOOL fNotAtBOL,
            SelectionMessage* pMessage )
{
    HRESULT hr = S_OK;
    BOOL fFoundGoodPlace = FALSE;
    HRESULT hrScroll = S_FALSE ;
    DWORD dwBreak = 0;

    //
    // For WM_TIMER messages - if they are in the window and over empty space
    // we will scroll the point into view.
    //


    if ( pMessage && 
         pMessage->message == WM_TIMER &&
         pMessage->fEmptySpace && 
         IsInWindow( pMessage->pt , TRUE ) &&
         IsPointInEditContextContent( pMessage->pt ) )
    {
        goto Cleanup; // We're done here.                                                         
    }
    
    if ( ! IsInValidPlaceForScroll( pScrollPointer ))
    {

        if ( pMessage && pMessage->fEmptySpace )
        {
            hrScroll = FALSE; // force the scollpoint into view to happen .
        }    
        else
        {
            CEditPointer scanPointer( GetEditor());
            IFC( scanPointer.MoveToPointer( pScrollPointer ));
            IFC( scanPointer.Scan( GetMoveDirection() ? LEFT : RIGHT  ,
                              BREAK_CONDITION_OMIT_PHRASE - BREAK_CONDITION_Block - BREAK_CONDITION_Site,
                              & dwBreak ));
                              
            if ( scanPointer.CheckFlag( dwBreak, BREAK_CONDITION_Text ))
            {
                fFoundGoodPlace =  IsInValidPlaceForScroll( pScrollPointer );
            }

            //
            // Look in other direction.
            //
            if ( ! fFoundGoodPlace )
            {
                IFC( scanPointer.MoveToPointer( pScrollPointer ));
                IFC( scanPointer.Scan( GetMoveDirection() ? RIGHT : LEFT  ,
                                  BREAK_CONDITION_OMIT_PHRASE - BREAK_CONDITION_Block - BREAK_CONDITION_Site,
                                  & dwBreak ));                          
                if ( scanPointer.CheckFlag( dwBreak, BREAK_CONDITION_Text ))
                {
                    fFoundGoodPlace =  IsInValidPlaceForScroll( pScrollPointer );
                }
            }
            
            if ( fFoundGoodPlace )
            {
                hrScroll = THR( GetViewServices()->ScrollPointerIntoView( scanPointer, 
                                                           fNotAtBOL,
                                                           POINTER_SCROLLPIN_Minimal ));
            }
        }
    }
    else
    {
        hrScroll = THR ( GetViewServices()->ScrollPointerIntoView( pScrollPointer , 
                                                       fNotAtBOL,
                                                       POINTER_SCROLLPIN_Minimal ));  
    }

    if ( hrScroll == S_FALSE && pMessage )
    {
        if ( ! IsInWindow( pMessage->pt , TRUE ) && IsPointInEditContextContent( pMessage->pt ) )
        {
            IFC( GetViewServices()->ScrollPointIntoView( _pManager->GetEditableElement(), & pMessage->pt));
        }
    }
        
Cleanup:
    RRETURN( hr );
}

//+====================================================================================
//
// Method: IsPointInEditContext
//
// Synopsis: Sees if a global point is in an Edit Context Content
//
//------------------------------------------------------------------------------------

BOOL
CSelectTracker::IsPointInEditContextContent(POINT ptGlobal)
{
    HRESULT hr;
    RECT contentRect;
    hr = THR( GetViewServices()->GetContentRect( _pManager->GetEditableElement(),
                                            COORD_SYSTEM_GLOBAL,
                                            & contentRect  ));

    if ( ! FAILED( hr ))
    {
        return ::PtInRect( & contentRect, ptGlobal);
    }

    AssertSz(0,"Failure of GetContentRect");

    return FALSE;    
}

//+====================================================================================
//
// Method: IsInValidPlaceForScroll
//
// Synopsis: Are we in a "good" place for scrolling ?
//
//------------------------------------------------------------------------------------

BOOL
CSelectTracker::IsInValidPlaceForScroll(IMarkupPointer* pScrollPointer)
{
    HRESULT hr = S_OK;
    BOOL fGoodPlace = FALSE;
    HTMLPtrDispInfoRec DispInfo;
    
    IHTMLElement* pIElement = NULL;
    IFC( GetViewServices()->GetLineInfo( pScrollPointer,  _fNotAtBOL , & DispInfo ));

    if ( DispInfo.fAligned || DispInfo.fHasNestedRunOwner )
    {
        fGoodPlace = FALSE;
    }
    else
        fGoodPlace = TRUE;

Cleanup:
    ReleaseInterface( pIElement );
    return fGoodPlace;
}

//+====================================================================================
//
// Method: CheckSwap
//
// Synopsis: Check if end is to the left/right of the swap pointer. If it is flip start 
//           and end and flip the swap direction.
//
//
//          This is to implement a funky IE4 / Word behavior. If you begin a word by 
//          double-clicking - the start and end can "flip". You can see this behavior
//          by double clickign on a word - and dragging right/left
//
//------------------------------------------------------------------------------------

HRESULT
CSelectTracker::CheckSwap()
{   
    HRESULT hr = S_OK;
    BOOL fOkSwap;

    Assert( _fDoubleClickWord );
    
    if ( _fSwapDirection )
    {
        IFC( _pEndPointer->IsRightOf( _pSwapPointer, & fOkSwap));
    }
    else
    {
        IFC( _pEndPointer->IsLeftOf( _pSwapPointer, & fOkSwap));
    }

    if ( fOkSwap )
    {
        IFC( _pStartPointer->MoveToPointer( _pSwapPointer ));
        if ( _fSwapDirection )
        {
            IFC( _pEndPointer->MoveToPointer( _pStartPointer));
            IFC( MoveWord( _pEndPointer, MOVEUNIT_NEXTWORDBEGIN ));
            //
            // BUGBUG - this is a hack - until I get the Reverse Word Selection behavior working fully
            //
            IFC( _pWordPointer->MoveToPointer( _pEndPointer ));
            _fInWordSel = TRUE;
            _fWordPointerSet = TRUE;
        }        
        else 
        {
            IFC( MoveWord( _pStartPointer, MOVEUNIT_NEXTWORDBEGIN ));
        }
        
        _fSwapDirection = ! ENSURE_BOOL( _fSwapDirection);
    }

Cleanup:
    RRETURN(hr);
}

//+====================================================================================
//
// Method: DoWordSelection
//
// Synopsis: Do the word selection behavior. Select on word boundaries when in Word Selection
//           mode.
//
//------------------------------------------------------------------------------------

HRESULT
CSelectTracker::DoWordSelection(
                                SelectionMessage* pMessage , 
                                BOOL* pfAdjustedSel, 
                                BOOL fRightOfCp,
                                BOOL fFurtherInStory )
{
    HRESULT hr = S_OK;
    int iWherePointer = SAME;
    BOOL fStartWordSel = FALSE;
    BOOL fNeedsAdjust;
    MARKUP_CONTEXT_TYPE eContext = CONTEXT_TYPE_None;
    
    Assert( pfAdjustedSel );
#if DBG == 1
    int endCp = 0;
    int oldEndCp = GetCp( _pEndPointer );
    int oldStartCp = GetCp( _pStartPointer );
    int startCp = oldStartCp;
#endif


    if ( _fInWordSel )
    {
        if ( !!_fWordSelDirection != fFurtherInStory ) // only look at changes in direction that are not in Y
        {
          // We only pay attention to changes in direction in the scope of the previous
            // word. Hence moves up or down by a line - that are changes in direction won't
            // snap us out of word select mode.
            //
            CEditPointer prevWordPointer ( GetEditor() );
            CEditPointer nextWordPointer ( GetEditor() );
            IFC( prevWordPointer.MoveToPointer( _pPrevTestPointer ));
            IFC( nextWordPointer.MoveToPointer( _pPrevTestPointer ));
            IFC( MoveWord( prevWordPointer, MOVEUNIT_PREVWORDBEGIN ));
            IFC( MoveWord( nextWordPointer, MOVEUNIT_NEXTWORDBEGIN ));
            BOOL fBetween = FALSE;
            
            IFC( _pTestPointer->IsRightOfOrEqualTo( prevWordPointer, & fBetween ));
            if ( fBetween )
            {
                IFC( _pTestPointer->IsLeftOfOrEqualTo( nextWordPointer, & fBetween ));    // CTL_E_INCOMPATIBLE will bail - but this is ok              
            }

            if ( fBetween )
            {
                _fInWordSel = FALSE;
                WHEN_DBG( if ( _fInWordSel ) ValidateWordPointer( _pEndPointer));
                IFC( MoveEndToPointer ( _pTestPointer ));
                _fWordPointerSet = FALSE;
                *pfAdjustedSel = TRUE;
                _fReversed = TRUE;
                _fReversedDirection = fFurtherInStory;
            }
        }

        if ( _fInWordSel ) // if we're still in word sel
        {
            //
            // We're in the same direction, we can move words.
            //
                
            IFC( OldCompare( _pWordPointer, _pTestPointer, & iWherePointer ));
            fNeedsAdjust = ( fFurtherInStory ) ?
                            ( iWherePointer == RIGHT ) :
                            ( iWherePointer == LEFT ) ;

            if ( fNeedsAdjust )
            {
                BOOL fAlreadyAtWord = IsAtWordBoundary( _pTestPointer );
                IFC( _pWordPointer->MoveToPointer( _pTestPointer ));                     
                if ( ! fAlreadyAtWord )
                {
                    //
                    // Use GetMoveDirection() instead of fFurtherInStory here.
                    //
                    IFC( MoveWord( _pWordPointer, GetMoveDirection() ? 
                                                  MOVEUNIT_NEXTWORDBEGIN : 
                                                  MOVEUNIT_PREVWORDBEGIN   ));                                                                                                    
                    IFC( ConstrainPointer( _pWordPointer, fFurtherInStory ));                   
                }

                WHEN_DBG( ValidateWordPointer(_pWordPointer) );
                IFC( MoveEndToPointer( _pWordPointer ));                    
                *pfAdjustedSel = TRUE;
            }               
            WHEN_DBG( if ( _fInWordSel ) ValidateWordPointer( _pEndPointer ));            

        }
    }
    else 
    {
        BOOL fTryToGetBackIn = TRUE;
        
        if ( _fReversed && 
            ( fFurtherInStory == ENSURE_BOOL( _fReversedDirection )) )
        {
            //
            // Word behavior is to stay out of word selection mode
            // once you've reversed - until you are past your start point.
            //

            if ( ! fFurtherInStory)
            {
                IFC( _pTestPointer->IsLeftOf( _pStartPointer, & fTryToGetBackIn ));
            }
            else
            {
                IFC( _pTestPointer->IsRightOf( _pStartPointer, & fTryToGetBackIn ));
            }
        }

        if ( fTryToGetBackIn )
        {
            BOOL fWordPointerSet = FALSE;
            IFC( SetWordPointer( _pWordPointer, fFurtherInStory, FALSE , & fStartWordSel , &fWordPointerSet) );
            _fWordPointerSet = fWordPointerSet;
            
            if ( _fWordPointerSet )
            {            
                if ( ! fStartWordSel )
                {
                    WHEN_DBG( ValidateWordPointer(_pWordPointer));        
                    IFC( OldCompare( _pWordPointer, _pTestPointer, & iWherePointer ));

                    switch( iWherePointer )
                    {
                        case LEFT:
                            fStartWordSel = !fFurtherInStory ;
                            break;
                            
                        case RIGHT:
                            fStartWordSel = fFurtherInStory;
                            break;

                        //
                        // Ok to start a word selection - if we are adjacent to a block
                        //
                        case SAME:
                        {
                            DWORD dwBreakCondition = 0;
                            fStartWordSel = IsAdjacentToBlock( _pWordPointer, fFurtherInStory ? RIGHT: LEFT , & dwBreakCondition  ) ;
                            //
                            // Ok to start word selection if you hit the end of the edit context.
                            //
                            if ( CEditPointer::CheckFlag( dwBreakCondition, BREAK_CONDITION_Boundary ))
                            {
                                fStartWordSel = TRUE;
                            }
                        }                        
                    }
                }
                
                if ( fStartWordSel )
                {
                    _fInWordSel = TRUE;   
                    WHEN_DBG( ValidateWordPointer( _pWordPointer ));

                    if ( ! _fReversed )
                    {
                        //
                        // The first time we jump into word sel mode, we set the word pointer off of StartPointer.
                        // This is valid for the purposes of determining if we went past the boundary - and for
                        // determining where to set the Start Pointer to.
                        // It is also valid to set the end off of this if you moved over this pointer 
                        //
                        // However there are some cases (like starting a selection in the empty space of tables)
                        // Where this causes us to move the end to the start - when it should be moved to test
                        // So what we do here is always set the end off of test.
                        //
                        
                        CEditPointer tempPointer( GetEditor());
                        IFC( tempPointer.MoveToPointer( _pTestPointer));
                        IFC( SetWordPointer( tempPointer, fFurtherInStory , TRUE ));
                        IFC( MoveEndToPointer( tempPointer ));
                    }
                    else
                        IFC( MoveEndToPointer( _pWordPointer ));
#if DBG == 1
                    endCp == GetCp(_pEndPointer );
                    if (( endCp - startCp == 0 ) && ( oldEndCp - oldStartCp != 0 ) )
                    {
    // BUGBUG: wrap a trace tag around this           
    //                    DumpTree( _pEndPointer );
                    }
#endif                
                    WHEN_DBG( if ( _fInWordSel ) ValidateWordPointer( _pEndPointer ));
                    
                    _fWordPointerSet = FALSE;

                    
                    if ( ! ENSURE_BOOL(_fStartAdjusted) )
                    {
                        //
                        // Check to see if the Start is already at a word boundary
                        // we don't make the adjustment if the context to the left/right of start 
                        // is not text !
                        //

                        if ( fFurtherInStory )
                        {
                            IFC( GetViewServices()->LeftOrSlave(_pStartPointer, FALSE, & eContext, NULL, NULL, NULL ));   
                        }
                        else
                        {
                            IFC( GetViewServices()->RightOrSlave(_pStartPointer, FALSE, & eContext, NULL, NULL, NULL ));                    
                        }

                        if ( eContext == CONTEXT_TYPE_Text )
                        {
                            BOOL fAlreadyAtStartOfWord = IsAtWordBoundary( _pStartPointer );
                            if ( ! fAlreadyAtStartOfWord )
                            {
                                IFC( MoveWord( _pStartPointer, fFurtherInStory ? 
                                                              MOVEUNIT_PREVWORDBEGIN  :  
                                                              MOVEUNIT_NEXTWORDBEGIN ));

                                ConstrainPointer( _pStartPointer, fFurtherInStory );                                                          
                            }
                        }
                        _fStartAdjusted = TRUE;
                        
                        WHEN_DBG( _ctStartAdjusted++);
                        Assert( _ctStartAdjusted == 1);
                    }
                    _fStartAdjusted = TRUE;
                    _fWordSelDirection = fFurtherInStory;
                    
                    *pfAdjustedSel = TRUE;
                }
                else
                {
                    IFC( MoveEndToPointer( _pTestPointer ));
                    *pfAdjustedSel = TRUE;
                }
                WHEN_DBG( if ( _fInWordSel ) ValidateWordPointer( _pEndPointer));
            }
        }
        else
        {
            IFC( MoveEndToPointer( _pTestPointer ));
            *pfAdjustedSel = TRUE;
        }
    }

    _fLastFurtherInStory = fFurtherInStory;
    
Cleanup:

    RRETURN ( hr );
}

//+====================================================================================
//
// Method: SetWordPointer
//
// Synopsis: Set the Word Pointer if it's not set already.
//
// Parameters:
//      fSetFromGivenPOinter - if false - we use either _pStartPointer, or _pPrevTestPointer to determine
//                                what to set the WOrd Pointer off of.
//                                if true we use where the Pointer given already is - to set the word pointer
//
//------------------------------------------------------------------------------------

HRESULT
CSelectTracker::SetWordPointer( 
                                IMarkupPointer* pPointerToSet, 
                                BOOL fFurtherInStory, 
                                BOOL fSetFromGivenPointer, /* = FALSE*/
                                BOOL* pfStartWordSel, /* = NULL*/
                                BOOL* pfWordPointerSet /*= NULL*/ )
{
    BOOL fStartWordSel = FALSE;
    BOOL fAtWord = FALSE;
    BOOL fBlockBetween = FALSE;
    BOOL fWordPointerSet = FALSE;
    
    HRESULT hr = S_OK;
    //
    // The first time we drop into word selection mode - we use the start pointer
    // from then on - we go off of the _testPointer
    // We know if this is the first time if we haven't reversed yet
    //
    BOOL fAtStart = FALSE;
    BOOL fAtEnd = FALSE;
    
    if ( ! fSetFromGivenPointer )
    {
        if ( ! _fReversed )
        {
            IsAtWordBoundary( _pStartPointer, &fAtStart, &fAtEnd, TRUE  );
            //
            // You may be at a word boundary. But it may not be the "right" word boundary.
            // Hence if you start a selection at the beginning of a word and select right
            // the "right" word boundary is the end of the word - not where you started !
            //
            fAtWord = fFurtherInStory  ? fAtEnd : fAtStart ; 

        }
        else
        {
            fAtWord = IsAtWordBoundary( _pPrevTestPointer, & fAtStart, &fAtEnd );
        }

        if ( ! _fReversed )
        {
            IFC( pPointerToSet->MoveToPointer( _pStartPointer ));                   
        }
        else
        {
            IFC( pPointerToSet->MoveToPointer( _pPrevTestPointer ));   
        }        
    }
    else
    {
#if DBG == 1    
        BOOL fPositioned;
        IGNORE_HR( pPointerToSet->IsPositioned( & fPositioned));
        Assert( fPositioned );
#endif         
        fAtWord = IsAtWordBoundary( pPointerToSet, & fAtStart, &fAtEnd );        
    }
    


    //
    // We move the word pointer, if start was not already at a word.
    //
    if ( !fAtWord ) 
    {          
        IFC( MoveWord( pPointerToSet, fFurtherInStory ? 
                                      MOVEUNIT_NEXTWORDBEGIN : 
                                      MOVEUNIT_PREVWORDBEGIN ));
        IFC( ConstrainPointer( pPointerToSet, fFurtherInStory ));         

        //
        // Compensate for moving past the end of a line.
        //
        // We look for this by seeing if the Test is to the left of a block (for the fFurtherInStory case)
        // and the word is to the right of a block.
        //
        fBlockBetween = IsAdjacentToBlock( pPointerToSet, fFurtherInStory ? LEFT: RIGHT ) &&
                        IsAdjacentToBlock( _pTestPointer, fFurtherInStory ? RIGHT : LEFT );

        if ( fBlockBetween ) 
        {
            //
            // We moved past line end. Compensate.
            // 

            IFC( pPointerToSet->MoveToPointer( _pTestPointer ));   
            fStartWordSel = TRUE;
            fWordPointerSet = TRUE;
        }
        else
        {                   
            fWordPointerSet = IsAtWordBoundary( pPointerToSet );
        }                         
    }    
    else
    {
        //
        // We know we're at a word boundary.
        //
        fWordPointerSet = TRUE;
    }

    if ( pfStartWordSel )
        *pfStartWordSel = fStartWordSel;

    if ( pfWordPointerSet )
        *pfWordPointerSet = fWordPointerSet;
        
Cleanup:    
    RRETURN( hr );
}

#if DBG == 1

//+====================================================================================
//
// Method: ValidateWordPointer
//
// Synopsis: Do some checks to see the Word Pointer is at a valid place
//
//------------------------------------------------------------------------------------

VOID
CSelectTracker::ValidateWordPointer(IMarkupPointer* pPointer)
{
    if ( IsTagEnabled ( tagSelectionValidateWordPointer ))
    {
        ELEMENT_TAG_ID eTag = TAGID_NULL;
        IHTMLElement* pIElement = NULL;
        BOOL fAtWord = FALSE;
        //
        // Check to see we're not in the root or other garbage
        //
        IGNORE_HR ( GetViewServices()->CurrentScopeOrSlave(pPointer, & pIElement ));
        IGNORE_HR( GetMarkupServices()->GetElementTagId( pIElement, & eTag ));
        Assert( eTag != TAGID_NULL );

        fAtWord = IsAtWordBoundary( pPointer );
        AssertSz( fAtWord, "Pointer is not at a word");
        ReleaseInterface( pIElement );
    }        
}

#endif

//+====================================================================================
//
// Method: GetStartSelectionForSpringLoader
//
// Synopsis: Seek out Font Tags to fall into for the spring loader.
//
//------------------------------------------------------------------------------------


HRESULT
CSelectTracker::GetStartSelectionForSpringLoader( IMarkupPointer* pPointerStart)
{
    HRESULT hr = S_OK;
    MARKUP_CONTEXT_TYPE eRightContext = CONTEXT_TYPE_None;
    BOOL fHitText = FALSE;
    IMarkupPointer* pBoundary = NULL;
    
    BOOL fFurtherInStory = GetMoveDirection();
    if ( fFurtherInStory )
    {
        IFC( pPointerStart->MoveToPointer( _pStartPointer ));
        pBoundary = _pEndPointer;
    }
    else
    {
        IFC( pPointerStart->MoveToPointer( _pEndPointer ));
        pBoundary = _pStartPointer;
    }
    IFC( GetViewServices()->RightOrSlave(pPointerStart, FALSE, & eRightContext, NULL, NULL, NULL));

    //
    // Don't do anything if we're already in Text
    //
    if ( eRightContext != CONTEXT_TYPE_Text )
    {
        IFC( MovePointerToText(
                    GetMarkupServices(),
                    GetViewServices(),
                    pPointerStart,
                    RIGHT,
                    pBoundary,
                    &fHitText,
                    FALSE ) );
    }
Cleanup:
    RRETURN ( hr );
}

//+====================================================================================
//
// Method: IsAdjacentToBlock
//
// Synopsis: Scan in a given direction - looking for a block.
//
//------------------------------------------------------------------------------------


BOOL 
CSelectTracker::IsAdjacentToBlock( 
                    IMarkupPointer* pPointer,
                    Direction iDirection,
                    DWORD* pdwBreakCondition)
{
    BOOL fHasBlock = FALSE ;
    MARKUP_CONTEXT_TYPE eContext = CONTEXT_TYPE_None;
    HRESULT hr;
    CEditPointer pEditPointer ( _pManager->GetEditor() ) ;
    DWORD dwBreakCondition = 0;

    Assert( iDirection == LEFT || iDirection == RIGHT );
    
    if ( iDirection == LEFT )
    {
        IFC( pPointer->Left( FALSE, &eContext, NULL, NULL, NULL ));  
    }
    else
    {
        IFC( pPointer->Right( FALSE, &eContext, NULL, NULL, NULL ));  
    }
    if ( eContext == CONTEXT_TYPE_Text )
        goto Cleanup;

    IFC( pEditPointer.MoveToPointer( pPointer ));                                        
    if ( iDirection == LEFT )
    {
        IFC( pEditPointer.SetBoundary( _pManager->GetStartEditContext() , pPointer  ));
    }
    else
    {
        IFC( pEditPointer.SetBoundary( pPointer, _pManager->GetEndEditContext() ));
    }

    //
    // Scan for the End of the Boundary
    //
    IGNORE_HR( pEditPointer.Scan( 
                                    iDirection , 
                                     BREAK_CONDITION_Text           | 
                                     BREAK_CONDITION_NoScopeSite    | 
                                     BREAK_CONDITION_NoScopeBlock   |
                                     BREAK_CONDITION_Site           | 
                                     BREAK_CONDITION_Block          |
                                     BREAK_CONDITION_BlockPhrase    |
                                     BREAK_CONDITION_Phrase         |
                                     BREAK_CONDITION_Anchor         |
                                     BREAK_CONDITION_Control ,
                                     & dwBreakCondition,
                                     NULL,
                                     NULL,
                                     NULL ));

    if ( pEditPointer.CheckFlag( dwBreakCondition, BREAK_CONDITION_Block | 
                                                    BREAK_CONDITION_Site |
                                                    BREAK_CONDITION_Control |
                                                    BREAK_CONDITION_NoScopeBlock ) )
    {
        fHasBlock = TRUE;
    }
    
Cleanup:
    if ( pdwBreakCondition )
        *pdwBreakCondition = dwBreakCondition;
        
    return fHasBlock;
}



                
//+====================================================================================
//
// Method: IsAtWordBoundary
//
// Synopsis: Check to see if we're already at the start of a word boundary
//
//------------------------------------------------------------------------------------


BOOL 
CSelectTracker::IsAtWordBoundary( 
                IMarkupPointer* pPointer, 
                BOOL * pfAtStart, /*= NULL */
                BOOL *pfAtEnd /*= NULL */,
                BOOL fAlwaysTestIfAtEnd /* = FALSE*/ )
{
    HRESULT hr = S_OK;
    IMarkupPointer* pStartTest = NULL;
    BOOL fAtStart = FALSE;
    BOOL fAtEnd = FALSE;
    
    IFC( GetMarkupServices()->CreateMarkupPointer(& pStartTest ));
    IFC( pStartTest->MoveToPointer( pPointer ));

    //
    // BUGBUG marka - we need to look for both the start of word, and the end
    // of word case. I do this by moving the pointer and comparing.
    //  this seems slow - but we'll try it this way and see.
    //
    
    //
    // Start of Word Case
    //

    //
    // We don't call MoveWord here - as we really want to go to NextWordEnd.
    //
    IFC( MoveWord( pStartTest, MOVEUNIT_NEXTWORDBEGIN ));
    IFC( MoveWord( pStartTest, MOVEUNIT_PREVWORDBEGIN ));
    IFC( pStartTest->IsEqualTo( pPointer, & fAtStart ));
        
    //
    // End of Word case
    //
    if ( !fAtStart || fAlwaysTestIfAtEnd )
    {
        IFC( pStartTest->MoveToPointer( pPointer ));
        IFC( MoveWord( pStartTest, MOVEUNIT_PREVWORDBEGIN ));
        IFC( MoveWord( pStartTest, MOVEUNIT_NEXTWORDBEGIN ));
        IFC( pStartTest->IsEqualTo( pPointer, & fAtEnd ));        
    }


Cleanup:
    AssertSz(!FAILED(hr), "Failure in IsAtWordBoundary");
    ReleaseInterface( pStartTest );

    if ( pfAtStart )
        *pfAtStart = fAtStart;
    if ( pfAtEnd )
        *pfAtEnd = fAtEnd;
        
    return ( fAtStart || fAtEnd );
}

       

//+====================================================================================
//
// Method: ScanForEnterBlock
//
// Synopsis: Look in the given direction for an EnterBlock. If you hit anything but a Block
//           on the way terminate !
//
//           If you do find an enter block - you move the given MarkupPointer - to just before the
//           enterblock. Otherwise you don't touch the pointer.
//------------------------------------------------------------------------------------

HRESULT
CSelectTracker::ScanForEnterBlock( 
            Direction iDirection, 
            IMarkupPointer* pPointer, 
            BOOL* pfFoundEnterBlock, 
            DWORD* pdwBreakCondition)
{
    HRESULT hr = S_OK;
    
    CEditPointer scanPointer( _pManager->GetEditor());
    DWORD dwBreakCondition = 0;
    BOOL fFoundEnterBlock = FALSE;

    IFC( scanPointer.MoveToPointer( pPointer ));
    IFC( scanPointer.SetBoundary( _pManager->GetStartEditContext(), _pManager->GetEndEditContext()));
    IFC( scanPointer.Scan( iDirection, 
                      BREAK_CONDITION_BlockScan ,
                      & dwBreakCondition ));

    while ( scanPointer.CheckFlag( dwBreakCondition, BREAK_CONDITION_Block | BREAK_CONDITION_NoScopeBlock ))
    {
        if ( scanPointer.CheckFlag( dwBreakCondition, BREAK_CONDITION_EnterBlock | BREAK_CONDITION_NoScopeBlock))
        {
            fFoundEnterBlock = TRUE;
            break;
        }

        IFC( scanPointer.Scan(  iDirection, 
                                BREAK_CONDITION_BlockScan,
                                & dwBreakCondition ));        
    }

    if ( fFoundEnterBlock )
    {
        //
        // Move the Pointer to the block - and backup over where you were.
        //
        IFC( pPointer->MoveToPointer( scanPointer ));
        if ( iDirection == RIGHT )
        {
            IFC( pPointer->Left( TRUE, NULL, NULL, NULL, NULL ));
        }
        else
        {
            IFC( pPointer->Right( TRUE, NULL, NULL, NULL, NULL ));
        }
    }

    
Cleanup:
    if ( pdwBreakCondition )
        *pdwBreakCondition = dwBreakCondition;
    if ( pfFoundEnterBlock )
        *pfFoundEnterBlock = fFoundEnterBlock;

    RRETURN( hr );
}

HRESULT
CSelectTracker::ScanForLastExitBlock( 
            Direction iDirection, 
            IMarkupPointer* pPointer, 
            DWORD* pdwBreakCondition )
{
    RRETURN( ScanForLastEnterOrExitBlock( iDirection,
                                          pPointer,
                                          BREAK_CONDITION_ExitBlock | BREAK_CONDITION_NoScopeBlock, 
                                          pdwBreakCondition ));
}


HRESULT
CSelectTracker::ScanForLastEnterBlock( 
            Direction iDirection, 
            IMarkupPointer* pPointer, 
            DWORD* pdwBreakCondition )
{
    RRETURN( ScanForLastEnterOrExitBlock( iDirection,
                                          pPointer,
                                          BREAK_CONDITION_EnterBlock | BREAK_CONDITION_NoScopeBlock, 
                                          pdwBreakCondition ));
}

//+====================================================================================
//
// Method: ScanForLastEnterOrExitBlock
//
// Synopsis: While you have ExitBlock - keep scanning. Return the markup pointer after the last
//           exit block.
//
//           Although this routine has a very big similarity to ScanForEnterBlock - I have
//           split it into two routines as it's less error prone.
//------------------------------------------------------------------------------------

HRESULT
CSelectTracker::ScanForLastEnterOrExitBlock( 
            Direction iDirection, 
            IMarkupPointer* pPointer, 
            DWORD dwTerminateCondition ,
            DWORD* pdwBreakCondition )
{
    HRESULT hr = S_OK;
    
    CEditPointer scanPointer( _pManager->GetEditor());
    DWORD dwBreakCondition = 0;

    IFC( scanPointer.MoveToPointer( pPointer ));
    IFC( scanPointer.SetBoundary( _pManager->GetStartEditContext(), _pManager->GetEndEditContext()));

    IFC( scanPointer.Scan( iDirection, 
                      BREAK_CONDITION_BlockScan ,
                      & dwBreakCondition ));

    while ( scanPointer.CheckFlag( dwBreakCondition, dwTerminateCondition ))
    {
        IFC( scanPointer.Scan( iDirection, 
                          BREAK_CONDITION_BlockScan ,
                          & dwBreakCondition ));
    }

    if ( ! scanPointer.CheckFlag( dwBreakCondition, BREAK_CONDITION_Boundary)  )
    {
        //
        // Go back to where you were.
        //
        IFC( scanPointer.Scan( Reverse( iDirection ) , 
                          BREAK_CONDITION_BlockScan ,
                          & dwBreakCondition ));
    }

    IFC( pPointer->MoveToPointer( scanPointer ));
    
Cleanup:
    if ( pdwBreakCondition )
        *pdwBreakCondition = dwBreakCondition;

    RRETURN( hr );
}

//+====================================================================================
//
// Method: IsValidForAdjust
//
// Synopsis: It is invalid to adjust selection into the space between certain tags
//           that are block tags (like lists). T
//
//------------------------------------------------------------------------------------

BOOL
CSelectTracker::IsValidForAdjust( ELEMENT_TAG_ID eTag )
{
    switch( eTag )
    {
        case TAGID_LI:
        case TAGID_OL:
        case TAGID_UL:
        case TAGID_DL:
        case TAGID_DT:
            return FALSE;

        default:
            return TRUE;
    }
}

//+====================================================================================
//
// Method: AdjustSelection
//
// Synopsis: Adjust the Selection to wholly encompass blocks. We do this if the End point is
//           outside a block, or we started from the shift key.
//
//------------------------------------------------------------------------------------


HRESULT
CSelectTracker::AdjustSelection(BOOL* pfAdjustedSel )
{
    HRESULT hr = S_OK;
    SP_IMarkupPointer spEnd;
    SP_IMarkupPointer spStart;
    BOOL fEqual = FALSE;
    MARKUP_CONTEXT_TYPE eContext = CONTEXT_TYPE_None;
    MARKUP_CONTEXT_TYPE eOtherContext = CONTEXT_TYPE_None;
    BOOL fAdjustedSelStart = FALSE;
    BOOL fAdjustedSelEnd = FALSE;
    BOOL fSwap = FALSE;
    BOOL fAdjustedFlow = FALSE;
    BOOL fPointerSet = FALSE;
    IHTMLElement* pIElement = NULL;
    IHTMLElement* pIOtherElement = NULL;
    IHTMLElement* pIEditableElement = NULL;
    BOOL fBlock = FALSE;
    IObjectIdentity*  pIIdent = NULL;
    IObjectIdentity* pIEditIdent = NULL;
    HRESULT hrEqual = S_OK;
    HRESULT hrEqualEdit = S_OK;
    HRESULT hrEqualEdit2 = S_OK;
    ELEMENT_TAG_ID eTag1 = TAGID_NULL;
    ELEMENT_TAG_ID eTag2 = TAGID_NULL;
    DWORD dwBreakCondition = 0;
    CEditPointer endPointer( _pManager->GetEditor());
    CEditPointer startPointer( _pManager->GetEditor());
    BOOL fAtEndOfLine = FALSE;
    
    IFC(GetMarkupServices()->CreateMarkupPointer( & spStart ));
    IFC(GetMarkupServices()->CreateMarkupPointer( & spEnd ));
    
    //
    // Find the "True" Start & End
    //
    IFC( MovePointersToTrueStartAndEnd( spStart, spEnd, & fSwap , & fEqual ));

    if (!_fShift ||  _lastCaretMove != CARET_MOVE_LINEEND )
    {
        //
        // If your end/start point is at the start of a block - adjust it in so it's in the
        // "magic place" between blocks.
        // ie. if we have <p>...</p><p><- End Selection
        // move it to;
        //                <p>...</p><-End <p>
        //
        if ( ! fEqual )
        {
            IFC( endPointer.MoveToPointer( spEnd ));
            IFC( endPointer.SetBoundary( _pManager->GetStartEditContext(), _pManager->GetEndEditContext()));                
            IFC( endPointer.Scan( LEFT,
                             BREAK_CONDITION_BlockScan ,
                             & dwBreakCondition, 
                             & pIElement));

                
            if ( endPointer.CheckFlag( dwBreakCondition,
                                      BREAK_CONDITION_ExitBlock ) 
                 && pIElement ) // possible to be null for a return in a PRE ( NoScopeBlock breaks on hard returns).
            {
                Assert( pIElement );
                //
                // Don't adjust for certain tags - like LI
                //
                IFC( GetMarkupServices()->GetElementTagId( pIElement, & eTag1 ));
                ClearInterface( & pIElement );

                if ( IsValidForAdjust( eTag1 ) )
                {
                    // If we found an exit block - continue looking to the left to see if
                    // you enter a block. Only if you do find an Enter Block and nothing else
                    // do you have to move to the special place.
                    //
                    IFC( ScanForEnterBlock( LEFT, endPointer, & fBlock ));
                    if ( fBlock )
                        fPointerSet = TRUE;
                }
            }
        }
    }
    else  // we only adjust out - if the last move was an End.
    {
        Assert( _lastCaretMove == CARET_MOVE_LINEEND );
        //
        // For Shift Key Selection - we adjust if the start and end are not at text.
        //

        fBlock = TRUE; // Hacky but safe.
        
        endPointer.MoveToPointer( spEnd );
        endPointer.SetBoundary( _pManager->GetStartEditContext(), _pManager->GetEndEditContext());                
        endPointer.Scan( RIGHT,
                         BREAK_CONDITION_BlockScan ,
                         & dwBreakCondition );

        if ( endPointer.CheckFlag( dwBreakCondition, 
                                            BREAK_CONDITION_ExitBlock | 
                                            BREAK_CONDITION_NoScopeBlock |
                                            BREAK_CONDITION_Boundary ))
        {
            //
            // If we found an exit block - continue looking to the right to see if
            // you enter a block. Only if you do find an Enter Block and nothing else
            // do you have to move to the special place.
            //
            IFC( ScanForLastExitBlock( RIGHT , endPointer ));
            fPointerSet = TRUE;
        }
    }

    //
    // Adjust the Start  pointer for insert to move into text.
    //
    //
    // We can't use the _fAtBOL flag here - it's only meaningful for the 
    // end point of selection
    //

    IFC( GetViewServices()->IsPointerBetweenLines( spStart , & fAtEndOfLine ));
    IFC( AdjustPointerForInsert( spStart, ! fAtEndOfLine, RIGHT, RIGHT ));
    fAdjustedSelStart = TRUE;
    
    //
    // Set the end pointer, here we do set the BOLness 
    //
    if ( fBlock )
    {
        if ( fPointerSet )
        {
            IFC( spEnd->MoveToPointer( endPointer ));            
        }
        fAdjustedSelEnd = TRUE;
    }        

    if ( _fShift && ( _lastCaretMove == CARET_MOVE_UP || _lastCaretMove == CARET_MOVE_DOWN ) )
    {
        IFC( GetViewServices()->IsPointerBetweenLines( spEnd, & fAtEndOfLine ));
        if ( fAtEndOfLine )
        {
            SetNotAtBOL( FALSE );
            SetAtLogicalBOL( TRUE );
        }
    }

    //
    // Now check the "FlowElements"  - ie layouts, the start and end are in match.
    //
    ClearInterface( & pIElement );
    ClearInterface( & pIOtherElement);
    IFC( GetViewServices()->GetFlowElement( spStart, & pIElement ));
    IFC( GetViewServices()->GetFlowElement( spEnd, & pIOtherElement ));

    if ( pIElement && pIOtherElement )
    {
        IFC( _pManager->GetEditableElement( & pIEditableElement ));
        IFC( pIEditableElement->QueryInterface( IID_IObjectIdentity, (void**) & pIEditIdent ));
        IFC( pIElement->QueryInterface(IID_IObjectIdentity, (void**) & pIIdent ));
        hrEqual  = pIIdent->IsEqualObject( pIOtherElement );
        hrEqualEdit = pIEditIdent->IsEqualObject( pIElement );
        hrEqualEdit2 = pIEditIdent->IsEqualObject( pIOtherElement);
        IFC( GetMarkupServices()->GetElementTagId( pIElement, &eTag1 ));
        IFC( GetMarkupServices()->GetElementTagId( pIOtherElement, &eTag2 ));
        
        //
        // We only do this adjustment if the two points are not in the same flow layout
        // AND one of the points is in the edit context.
        //
        if ( ( hrEqual != S_OK ) && 
             ( ! IsTablePart( eTag1 ) ) && // Explicitly don't adjust across TD's.
             ( ! IsTablePart( eTag2 ) ) &&
             (( hrEqualEdit == S_OK ) || ( hrEqualEdit2 == S_OK )))
        {
            //
            // The Start and End are in different flow layouts, 
            // if the context to the left and right is ExitScope - we see if we can go left and right
            // and fix this.
            //
            IFC( GetViewServices()->LeftOrSlave(spStart, FALSE, & eContext, NULL,NULL, NULL ));
            IFC( GetViewServices()->RightOrSlave(spEnd, FALSE, & eOtherContext, NULL, NULL, NULL));
            if ( ( eContext == CONTEXT_TYPE_ExitScope ) || 
                 ( eOtherContext == CONTEXT_TYPE_ExitScope ) )
            {
                if ( eContext == CONTEXT_TYPE_ExitScope )
                {
                    IFC( MovePointerOutOfScope( 
                                            GetMarkupServices(), 
                                            GetViewServices(),
                                            spStart,
                                            LEFT,
                                            _pManager->GetStartEditContext() ,
                                            &fAdjustedFlow,
                                            FALSE,
                                            TRUE,
                                            FALSE , FALSE )); // Don't stop at Layout or Intrinsics

                    fAdjustedSelStart = ( fAdjustedSelStart || fAdjustedFlow );                                            
                }
                if ( eOtherContext == CONTEXT_TYPE_ExitScope)
                {
                    IFC( MovePointerOutOfScope( 
                                            GetMarkupServices(), 
                                            GetViewServices(),
                                            spEnd,
                                            RIGHT,
                                            _pManager->GetEndEditContext() ,
                                            &fAdjustedFlow,
                                            FALSE,
                                            TRUE,
                                            FALSE, FALSE )); // Don't stop at layout 
                                            
                    fAdjustedSelEnd = ( fAdjustedSelEnd || fAdjustedFlow );                                            
                }
            }                    
        }
        else if ( hrEqual != S_OK  && 
                  IsTablePart(eTag1) && 
                  hrEqualEdit2 == S_OK &&
                  !( _fShift && _lastCaretMove != CARET_MOVE_NONE )) // don't do this for key-nav
            {
            //
            // Special case tables ( sigh). We started in a table cell, and ended in the EditContext.
            // If the End Can be moved back - through EnterBlocks only - then do it.
            // 
            //
            // BUGBUG - we could try something fancier here - like see whether the start pointer's layout
            // encloses the end pointer - and try to get away from the table special casing. However this
            // isn't that bad - tables being the only flow layout that aren't the edit context
            // means we always end up doing some "specialness" for them.
            //
            CEditPointer scanPointer ( _pManager->GetEditor());
            DWORD dwBreakCondition = 0;
            scanPointer.MoveToPointer( spEnd );
            scanPointer.Scan( LEFT,
                              BREAK_CONDITION_BlockScan ,
                              & dwBreakCondition );

            if ( scanPointer.CheckFlag( dwBreakCondition, BREAK_CONDITION_EnterBlock))
            {
                ScanForLastEnterBlock( LEFT, scanPointer, NULL);
                fAdjustedSelEnd = TRUE;
                IFC( spEnd->MoveToPointer( scanPointer ));
            }                                  
        }        
    }

    
    //
    // Now adjust the context to the adjusted start and end
    //
    if ( fAdjustedSelStart || fAdjustedSelEnd )
    {
        if ( ! fSwap )
        {
            if ( fAdjustedSelStart )
                IFC( _pStartPointer->MoveToPointer( spStart ));
            if ( fAdjustedSelEnd )
                IFC( MoveEndToPointer( spEnd ));
        }
        else
        {
            if ( fAdjustedSelStart )
                IFC( MoveEndToPointer( spStart ));
            if ( fAdjustedSelEnd )
                IFC( _pStartPointer->MoveToPointer( spEnd ));
        }
    }        
    

    //
    // marka - explicitly set the pointer gravity - based on the direction of the selection
    // 
    if ( ! fSwap )
    {
        IFC( _pStartPointer->SetGravity( POINTER_GRAVITY_Right ));
        IFC( _pEndPointer->SetGravity( POINTER_GRAVITY_Left ));
    }
    else
    {
        IFC( _pEndPointer->SetGravity( POINTER_GRAVITY_Right ));
        IFC( _pStartPointer->SetGravity( POINTER_GRAVITY_Left ));  
    } 
    
    if ( pfAdjustedSel )
        *pfAdjustedSel = fAdjustedSelStart || fAdjustedSelEnd ;
Cleanup:
    ReleaseInterface( pIEditIdent );
    ReleaseInterface( pIEditableElement );
    ReleaseInterface( pIIdent );
    ReleaseInterface( pIElement );
    ReleaseInterface( pIOtherElement );
    RRETURN( hr );
}

//+====================================================================================
//
// Method: GetMoveDirection
//
// Synopsis: Get the direction in which selection is occuring.
//
//  Return True - if we're selecting further in the story (From L to R in a LTR layout).
//         False - if we're selecting earlier in the story ( From R to L in a LTR layout).
//
// BUGBUG - we're breaking RTL-ness here if the pointers are in the same place.
//
//------------------------------------------------------------------------------------

BOOL
CSelectTracker::GetMoveDirection()
{
    BOOL fFurtherInStory = FALSE;
    int wherePointer = SAME;
    HRESULT hr = S_OK;
    
    hr = THR ( OldCompare( _pStartPointer, _pEndPointer, & wherePointer));
    if (( wherePointer == SAME ) || ( hr == E_INVALIDARG ))
    {
        //
        // BUGBUG does this break under RTL ?
        //
        POINT ptGuess;
        ptGuess.x = _curMouseX;
        ptGuess.y = _curMouseY; 
        fFurtherInStory = GuessDirection( & ptGuess );
    }
    else
        fFurtherInStory = ( wherePointer == RIGHT ) ? TRUE : FALSE;

    return fFurtherInStory;
}

//+====================================================================================
//
// Method: GetDirection
//
// Synopsis: our _pTestPointer is equal to our previous test. So we use the Point
//           to guess the direction. The "guess" is actually ok once there is some delta
//           from _anchorMouseX and _anchorMouseY
//
//------------------------------------------------------------------------------------


BOOL
CSelectTracker::GuessDirection(POINT* ppt)
{
    if ( ppt->x != _anchorMouseX )
    {
        return ppt->x >= _anchorMouseX ;    // did we go past _anchorMouseX ?
    }
    else if ( ppt->y != _anchorMouseX)
    {
        return ppt->y >= _anchorMouseY;  // did we go past _anchorMouseY ?
    }

    return TRUE ; // we're really and truly in the same place. Default to TRUE.
    
}

//+====================================================================================
//
// Method: AdjustForSiteSelectable.
//
// Synopsis: We just moved the end point into a different flow layout. Check to see if 
//           the thing we moved into is Site Selectable. If it is - Jump Over it, using the 
//           direction in which we are moving.
//
//           Return - TRUE - if we adjusted the selection
//                    FALSE if we didn't make any adjustments. Normal selection processing
//                          should occur.
//
//------------------------------------------------------------------------------------

BOOL
CSelectTracker::AdjustForSiteSelectable()
{
    HRESULT hr = S_OK;
    BOOL fAdjustedSelection = FALSE;
    IHTMLElement* pIInThisElement = NULL;
    IHTMLElement* pIFlowElement = NULL;
    
    IFC( GetViewServices()->CurrentScopeOrSlave(_pTestPointer, & pIInThisElement ));
    if (  _pManager->GetEditor()->IsElementSiteSelectable( pIInThisElement ) == S_OK )
    {
        Assert( _pManager->IsInEditContext(_pTestPointer ) );
            
        IFC( GetViewServices()->GetFlowElement( _pTestPointer, & pIFlowElement ));
        if ( !pIFlowElement )
            goto Cleanup;
            
        if ( GetMoveDirection() ) // Further in Story
        {
            hr = THR( _pEndPointer->MoveAdjacentToElement( pIFlowElement, ELEM_ADJ_AfterEnd));
        }
        else
        {
            hr = THR( _pEndPointer->MoveAdjacentToElement( pIFlowElement, ELEM_ADJ_BeforeBegin));
        }
        fAdjustedSelection = TRUE;
    }
    
Cleanup:
    ReleaseInterface( pIInThisElement );
    ReleaseInterface( pIFlowElement);
    return ( fAdjustedSelection );
}

//+====================================================================================
//
// Method: ConstrainEndToContainer
//
// Synopsis: Verifies that the given MarkupPointer is in the same tree as when we started.
//           If it is not - we then position the EndPointer.
//
//           Return TRUE - if we may still reposition the pointer
//                  FALSE - otherwise
//------------------------------------------------------------------------------------

BOOL
CSelectTracker::ConstrainEndToContainer( )
{
    HRESULT hr = S_OK;
    IHTMLElement* pIElement = NULL;
    IHTMLElement* pISelectElement = NULL;
    BOOL repositionSegment  = FALSE;
    BOOL moveToStart = FALSE;
    HRESULT hrContextContainsTest = S_FALSE;
    IMarkupContainer* pIContainerTest = NULL;
    IMarkupContainer* pIContainerContext = NULL;

    IFC( _pTestPointer->GetContainer( & pIContainerTest));
    IFC( _pEndPointer->GetContainer(& pIContainerContext ));

    if (! pIContainerTest || ! pIContainerContext )
    {
        AssertSz(0,"No Markup Container");
        goto Cleanup;
    }
    //
    // Compare MarkupContainers to see if the Context contains the Test Pointer. If it does,
    // then we can jump over the Element. If it doesn't contain it we constrain to the container we're
    // in.
    //

    hrContextContainsTest = GetViewServices()->IsContainedBy( pIContainerContext, pIContainerTest);  
    if( hrContextContainsTest == E_INVALIDARG )  // In the same tree - just reposition 
    {
        if ( ! _fInWordSel )
        {
            IFC( MoveEndToPointer( _pTestPointer ));
            repositionSegment = TRUE;
        }            
        else
            repositionSegment = FALSE;        
    }
    else if ( hrContextContainsTest == S_OK ) // context contains the markup - so jump over it.
    {
        IFC( GetViewServices()->CurrentScopeOrSlave(_pTestPointer, & pIElement ));    
        IFC( GetViewServices()->GetElementForSelection( pIElement, & pISelectElement));
        if ( GetMoveDirection() ) // Further in Story
        {
            hr = THR( _pEndPointer->MoveAdjacentToElement( pISelectElement, ELEM_ADJ_AfterEnd));
        }
        else
        {
            hr = THR( _pEndPointer->MoveAdjacentToElement( pISelectElement, ELEM_ADJ_BeforeBegin));
        }
        //
        // Move Test to End to make fValidTree check ok
        //
        IFC( _pTestPointer->MoveToPointer( _pEndPointer ));        
        repositionSegment = TRUE;
    }
    else // We don't contain the markup we're in - just sit tight in the markup we're in.
    {

        //
        // BUGBUG - I don't think I need this code anymore in fact it scares me !
        // investigate sometime
        //
        if ( ! _fEndConstrained )
        {
            moveToStart = ! GetMoveDirection();

            if ( moveToStart )
            {
                hr = THR( MoveEndToPointer( _pManager->GetStartEditContext()));
            }
            else
            {
                hr = THR( MoveEndToPointer( _pManager->GetEndEditContext()));
            }
            repositionSegment = TRUE;
            _fEndConstrained = TRUE;
            
            //
            // Move Test to End to make fValidTree check ok
            //
            IFC( _pTestPointer->MoveToPointer( _pEndPointer ));
        }
        else
            repositionSegment = FALSE;
            
    }

    if ( repositionSegment )
    {
        IFC( ScrollPointerIntoView( _pEndPointer, _fNotAtBOL  ));
    }
    
Cleanup:

    ReleaseInterface( pIElement );
    ReleaseInterface( pISelectElement );
    ReleaseInterface( pIContainerContext );
    ReleaseInterface( pIContainerTest );
    
    return ( repositionSegment );
}



//+---------------------------------------------------------------------------
//
//  Member:     CTextSelectTracker::GetAction
//
//  Synopsis:   Get the action to be taken, given the state we are in.
//
//----------------------------------------------------------------------------
ACTIONS
CSelectTracker::GetAction(SelectionMessage *pMessage)
{
    unsigned int LastEntry = sizeof (ActionTable) / sizeof (ActionTable[0]);
    unsigned int i;
    ACTIONS Action = A_ERR;

    Assert (_fState <= ST_STOP);

    // Discard any spurious mouse move messages
    if ((WM_MOUSEMOVE == pMessage->message) && (!IsValidMove (pMessage)))
    {
        Action = A_DIS;
    }
    else
    {
        // Lookup the state-message tabl to find the appropriate action
        // ActionTable[LastEntry - 1]._iJMessage = pMessage->message;
        for (i = 0; i < LastEntry; i++)
        {
            if ( (ActionTable[i]._iJMessage == pMessage->message) || ( i == LastEntry ) )
            {
                Action = ActionTable[i]._aAction[_fState];
                break;
            }
        }
    }
    return (Action);
}

//+===============================================================================
//
// Method: FireSelectStartMessage..
//
// Synopsis - Fire On Select Start back to the element and return the result.
//
//--------------------------------------------------------------------------------
HRESULT
CEditTracker::FireSelectStartMessage(SelectionMessage* pMessage)
{
    HRESULT hr = S_OK;
    HRESULT fireValue = S_FALSE;
    IHTMLElement* pICurElement = NULL;
    
    IFC( GetElementFromMessage( _pManager, pMessage, & pICurElement ));
    if ( pICurElement )
    {
        _fInFireOnSelectStart = TRUE;
        fireValue =  GetViewServices()->FireOnSelectStart( pICurElement );
        _fInFireOnSelectStart = FALSE;
    }        
Cleanup:
    ReleaseInterface( pICurElement);
    if ( _fFailFireOnSelectStart )
    {
        fireValue = S_OK ; // This looks wrong but isn't by default FireOnSelectStart return S_FALSE
    }
    return ( fireValue );
}

//+====================================================================================
//
// Method: fire On Before EditFocus
//
// Synopsis:
//
//------------------------------------------------------------------------------------


BOOL
CEditTracker::FireOnBeforeEditFocus( IHTMLElement* pIElement )
{
    BOOL fRet = FALSE;
    
    IGNORE_HR( GetViewServices()->FireOnBeforeEditFocus( pIElement , & fRet ));

    return fRet;
}    

//+---------------------------------------------------------------------------
//
//  Member:     CSelectionManager::GetMousePoint
//
//  Synopsis:   Get the x,y of the mouse by looking at the global cursor Pos.
//
//----------------------------------------------------------------------------
void
CEditTracker::GetMousePoint(POINT *ppt, BOOL fDoScreenToClient /* = TRUE */)
{
    HRESULT hr = S_OK;
    
    if ( ! _hwndDoc )
    {
        hr = THR( _pManager->GetEditor()->GetViewServices()->GetViewHWND( &_hwndDoc ));
        AssertSz( ! FAILED( hr ) , "GetViewHWND In CSelectTracker Failed" );
    }
    
    GetCursorPos(ppt);
    if ( fDoScreenToClient )
        ScreenToClient( _hwndDoc, ppt);
}

//+====================================================================================
//
// Method: Is MessageInWindow
//
// Synopsis: See if a given message is in a window. Assumed that the messages' pt has
//           already been converted to Screen Coords.
//
//------------------------------------------------------------------------------------

BOOL 
CEditTracker::IsMessageInWindow( SelectionMessage* peMessage )
{
    POINT globalPt;
    HRESULT hr ;
    
    globalPt = peMessage->pt;
    
    if ( ! _hwndDoc )
    {
       hr = THR( _pManager->GetEditor()->GetViewServices()->GetViewHWND( &_hwndDoc ));
       AssertSz( ! FAILED(hr) , "GetViewHWND In CSelectTracker Failed" );
    }

    ::ClientToScreen( _hwndDoc , & globalPt );

    return ( IsInWindow( globalPt ));
    
}

//+---------------------------------------------------------------------------
//
//  Member:     CSelectionManager::GetMousePoint
//
//  Synopsis:   Check to see if the GLOBAL point is in the window.
//
//----------------------------------------------------------------------------
BOOL
CEditTracker::IsInWindow(POINT pt , BOOL fClientToScreen /* = FALSE*/ )
{
    HRESULT hr = S_OK;

    if ( ! _hwndDoc )
    {
        hr = THR( GetViewServices()->GetViewHWND( &_hwndDoc ));
        AssertSz( ! FAILED( hr ) , "GetViewHWND In CSelectTracker Failed" );
    }

    return EdUtil::IsInWindow(_hwndDoc, pt, fClientToScreen );

}

//+---------------------------------------------------------------------------
//
//  Member:     CEditTracker::GetLocation
//
//----------------------------------------------------------------------------
HRESULT 
CEditTracker::GetLocation(POINT *pPoint)
{
    AssertSz(0, "GetLocation not implemented for tracker");
    return E_NOTIMPL;
}



//+====================================================================================
//
// Method: DoSelectWord
//
// Synopsis: Select a word at the given point.
//
//------------------------------------------------------------------------------------


HRESULT
CSelectTracker::DoSelectWord( SelectionMessage* pMessage )
{
    HRESULT hr = S_OK;
    BOOL fAtStart = FALSE;
    BOOL fAtEnd = FALSE;
    IMarkupPointer* pTest = NULL;
    IHTMLElement * pIFlowElement = NULL;
    BOOL fEnabled = FALSE;
    IHTMLInputElement* pIInputElement = NULL;
    BSTR bstrType = NULL;
    BOOL fWasPassword = FALSE;
    IHTMLElement* pIElement = NULL;
    CSpringLoader *psl = GetSpringLoader();

    // Currently, we reset for empty selections.  We may want to change this at some point.
    if (psl)
        psl->Reset();
    
    IFC( GetViewServices()->GetFlowElement( _pStartPointer, & pIFlowElement )); 
    if ( ! pIFlowElement )
        goto Cleanup;
    IFC( GetViewServices()->IsEnabled( pIFlowElement, & fEnabled ));

    if ( fEnabled )
    {    
        //
        // Special case to look to see if we are in a Password.
        // If we are - we don't want to do a word select - just select the entire contents of the password
        // Why ? Because we might then be revealing the presence of a space in the password ( gasp !)
        //
        if ( _pManager->GetEditableTagId() == TAGID_INPUT )
        {
            //
            // Get the Master.
            //
            IFC( GetViewServices()->GetElementForSelection( _pManager->GetEditableElement(),
                                                          & pIElement));
                                                          
            IFC( pIElement->QueryInterface ( 
                                            IID_IHTMLInputElement, 
                                            ( void** ) & pIInputElement ));
                                
            IFC(pIInputElement->get_type(&bstrType));
            
            if (!_tcsicmp( bstrType, TEXT("password")))
            {
                IFC( _pStartPointer->MoveToPointer( _pManager->GetStartEditContext()));
                IFC( _pEndPointer->MoveToPointer( _pManager->GetEndEditContext()));
                fWasPassword = TRUE;
            }
        }

        if ( ! fWasPassword )
        {

            ClearInterface( & _pSwapPointer);
            IFC( GetMarkupServices()->CreateMarkupPointer( & _pSwapPointer));
#if DBG == 1
            SetDebugName( _pSwapPointer, _T("Swap Pointer"));
#endif

            ClearInterface( & pIFlowElement );
            IFC( GetMarkupServices()->CreateMarkupPointer( & pTest ));
            IFC( pTest->MoveToPointer( _pStartPointer ));

            //
            // Move Start
            //
            IsAtWordBoundary( pTest, & fAtStart, NULL );
            if ( ! fAtStart )
            {
                IFC( MoveWord(  pTest, MOVEUNIT_PREVWORDBEGIN));          
            }       
            //
            // See if we're in the same flow layout
            //
            if ( PointersInSameFlowLayout( _pStartPointer, pTest, NULL, GetViewServices() ))
            {           
                IFC( _pStartPointer->MoveToPointer( pTest ));
            }
            else
            {
                //
                // We know the Start went into another layout. If we go left - we will find that
                // layout start. We position ourselves at the start of the layout boundary.
                //
                
                DWORD dwBreak = 0;
                CEditPointer scanPointer( _pManager->GetEditor());
                scanPointer.MoveToPointer( _pTestPointer );
                scanPointer.Scan( LEFT,
                                  BREAK_CONDITION_Site,
                                  &dwBreak);

                Assert( scanPointer.CheckFlag(  dwBreak, BREAK_CONDITION_Site  ));                                    
                
                IFC( _pStartPointer->MoveToPointer( scanPointer ));
                IFC( _pStartPointer->Right( TRUE, NULL, NULL, NULL, NULL ));
            }

            //
            // Move End
            //
            IFC( pTest->MoveToPointer( _pEndPointer ));
            IsAtWordBoundary( pTest, NULL, & fAtEnd );
            if ( ! fAtEnd )
            {
                IFC( MoveWord( pTest, MOVEUNIT_NEXTWORDBEGIN ));
            }      
            //
            // See if we're in the same flow layout
            //            
            if ( PointersInSameFlowLayout( _pEndPointer, pTest, NULL, GetViewServices() ))
            {
                //
                // See if we moved pass a block boundary.
                //
                CEditPointer endPointer( _pManager->GetEditor());
                DWORD dwBreakCondition = 0;
                
                endPointer.MoveToPointer( pTest );

                if ( ! fAtEnd )
                {
                    endPointer.SetBoundary( 
                            _pManager->GetStartEditContext(), 
                            _pManager->GetEndEditContext());                
                    endPointer.Scan( LEFT,
                                     BREAK_CONDITION_Text | 
                                     BREAK_CONDITION_Block |
                                     BREAK_CONDITION_NoScopeBlock | 
                                     BREAK_CONDITION_Site ,
                                     & dwBreakCondition);

                    WHEN_DBG( SetDebugName( endPointer, _T(" Word Block Scanner")));
                    BOOL fHitBlock =   endPointer.CheckFlag( dwBreakCondition, BREAK_CONDITION_Block | BREAK_CONDITION_NoScopeBlock  );
                    
                    if ( fHitBlock )
                    {
                        //
                        // We found an Exit Block. Instead of doing a move word - scan
                        // from the end looking for a block boundary.
                        //
                        endPointer.MoveToPointer( _pEndPointer );
                        endPointer.Scan( RIGHT,
                                            BREAK_CONDITION_Block |
                                            BREAK_CONDITION_Site |
                                            BREAK_CONDITION_NoScopeBlock | 
                                            BREAK_CONDITION_Boundary,
                                            & dwBreakCondition );
                                            
                        if ( ! endPointer.CheckFlag( dwBreakCondition, BREAK_CONDITION_Boundary ))
                        {
                            endPointer.Scan( LEFT,
                                                BREAK_CONDITION_Block |
                                                BREAK_CONDITION_Site |
                                                BREAK_CONDITION_NoScopeBlock | 
                                                BREAK_CONDITION_Boundary,
                                                & dwBreakCondition );
                        }                        
                        hr = THR( _pEndPointer->MoveToPointer( endPointer ));                                       
                    }
                    else
                    {
                        Assert( ! fHitBlock ); // I know this looks funny. The debugger gets confused on the above so looks like it steps into here.
                        hr = THR( _pEndPointer->MoveToPointer( pTest ));
                    }                    
                }
            }
            else
            {
                //
                // We know the End went into another layout. If we go right  we will find that
                // layout boundary - we position ourselves at the start of the layout boundary.
                //
                
                DWORD dwBreak = 0;
                CEditPointer scanPointer( _pManager->GetEditor());
                scanPointer.MoveToPointer( _pTestPointer );
                scanPointer.Scan( RIGHT,
                                  BREAK_CONDITION_Site,
                                  &dwBreak);

                Assert( scanPointer.CheckFlag( dwBreak, BREAK_CONDITION_Site ));                                  

                IFC( _pEndPointer->MoveToPointer( scanPointer ));                
                IFC( _pEndPointer->Left( TRUE, NULL, NULL, NULL, NULL ));

            }
        }
    }
    else
    {
        IFC( _pStartPointer->MoveAdjacentToElement( pIFlowElement, ELEM_ADJ_BeforeBegin ));
        IFC( _pEndPointer->MoveAdjacentToElement( pIFlowElement, ELEM_ADJ_AfterEnd));        
    }

    if ( hr )
        goto Cleanup;
        
    IFC( ConstrainSelection( TRUE, & pMessage->pt)); 
    IFC( AdjustSelection( NULL ));
    if ( _fAddedSegment )
    {    
        IFC( _pSelRenSvc->MoveSegmentToPointers( _iSegment, _pStartPointer, _pEndPointer, HIGHLIGHT_TYPE_Selected  ));
    }
    else
    {

        hr = THR(  _pSelRenSvc->AddSegment( _pStartPointer, _pEndPointer, HIGHLIGHT_TYPE_Selected, & _iSegment ) );    

        if ( ! hr )
            _fAddedSegment = TRUE;
        else
            _fAddedSegment = FALSE;
    }

    if ( !fWasPassword && fEnabled )
    {
        IFC( _pSwapPointer->MoveToPointer( _pStartPointer));
        _fDoubleClickWord = TRUE;
        _fSwapDirection = FALSE;

        //
        // If we got here by double-click - we're in word sel mode.
        //
        _fInWordSel = TRUE;
        IFC( _pWordPointer->MoveToPointer( _pEndPointer ));   
        _fWordPointerSet = TRUE;
        
    }    
    
    SetMadeSelection( TRUE);
Cleanup:
    SysFreeString(bstrType);
    ReleaseInterface( pIElement );
    ReleaseInterface( pIInputElement );
    ReleaseInterface( pIFlowElement );
    ReleaseInterface( pTest );
    RRETURN( hr );
}


//+====================================================================================
//
// Method: DoSelectParagraph
//
// Synopsis: Select a paragraph at the given point.
//
//------------------------------------------------------------------------------------


HRESULT
CSelectTracker::DoSelectParagraph( SelectionMessage* pMessage )
{
    HRESULT hr = S_OK;
    IHTMLElement* pIElement  = NULL;
    DWORD breakOn = 0 ;
    ELEMENT_TAG_ID eTag = TAGID_NULL;
    CSpringLoader   *psl = GetSpringLoader();
    
    CEditPointer startPointer ( _pManager->GetEditor() , _pStartPointer );
    CEditPointer endPointer ( _pManager->GetEditor(), _pEndPointer );

    if (psl)
        psl->Reset();
    
    startPointer.SetBoundary( _pManager->GetStartEditContext(), 
                                _pManager->GetEndEditContext() );
    endPointer.SetBoundary( _pManager->GetStartEditContext(), 
                                _pManager->GetEndEditContext() );

    IFC( startPointer.Scan(
                        LEFT,
                        BREAK_CONDITION_Block ,
                        & breakOn,
                        & pIElement ));
    //
    // Go back again to reposition the Start to the Start of the Block Break
    //
    if ( startPointer.CheckFlag( breakOn, BREAK_CONDITION_Block ))
    {
        //
        // Table is a block, so we have to specialcase skipping over them - as IE4 did.
        // BUGBUG - it'd be nice to specify in the ScanPointer that we want to skip tables as blocks
        // but will leave that for now.
        //
        IFC( GetMarkupServices()->GetElementTagId( pIElement, & eTag ));
        while ( eTag == TAGID_TABLE && startPointer.CheckFlag( breakOn, BREAK_CONDITION_Block ))
        {
            IFC( startPointer.MoveAdjacentToElement( pIElement, ELEM_ADJ_BeforeBegin ));
            
            ClearInterface( & pIElement );
            IFC( startPointer.Scan(
                                LEFT,
                                BREAK_CONDITION_Block ,
                                & breakOn,
                                & pIElement ));
            IFC( GetMarkupServices()->GetElementTagId( pIElement, & eTag ));
        }

        if ( startPointer.CheckFlag( breakOn, BREAK_CONDITION_Block ))
        {
            IFC( startPointer.Scan(
                                RIGHT,
                                BREAK_CONDITION_Block | BREAK_CONDITION_Site ,
                                & breakOn ));
        }                            
    }

    ClearInterface( & pIElement );
    IFC( endPointer.Scan( 
                     RIGHT,
                     BREAK_CONDITION_Block | BREAK_CONDITION_Site ,
                     & breakOn,
                     & pIElement ));

    if ( endPointer.CheckFlag( breakOn, BREAK_CONDITION_Site ))
    {
        IFC( GetMarkupServices()->GetElementTagId( pIElement, & eTag ));
        while ( eTag == TAGID_TABLE && endPointer.CheckFlag( breakOn, BREAK_CONDITION_Block ))
        {
            IFC( endPointer.MoveAdjacentToElement( pIElement, ELEM_ADJ_AfterEnd ));
            
            ClearInterface( & pIElement );
            IFC( endPointer.Scan(
                                RIGHT,
                                BREAK_CONDITION_Block | BREAK_CONDITION_Site ,
                                & breakOn,
                                & pIElement ));
            IFC( GetMarkupServices()->GetElementTagId( pIElement, & eTag ));
        }
        
        if ( endPointer.CheckFlag( breakOn, BREAK_CONDITION_Site ))
        {
            IFC( endPointer.Scan(
                            LEFT,
                            BREAK_CONDITION_Block | BREAK_CONDITION_Site ,
                            & breakOn ));
        }                            
    }
    
    IFC( ConstrainSelection( TRUE, & pMessage->pt ));
    IFC( AdjustSelection( NULL ));
    if ( _fAddedSegment )
    {    
        IFC( _pSelRenSvc->MoveSegmentToPointers( _iSegment, _pStartPointer, _pEndPointer, HIGHLIGHT_TYPE_Selected  ));
    }
    else
    {

        hr = THR(  _pSelRenSvc->AddSegment( _pStartPointer, _pEndPointer, HIGHLIGHT_TYPE_Selected, & _iSegment ) );    

        if ( ! hr )
            _fAddedSegment = TRUE;
        else
            _fAddedSegment = FALSE;
    }
    
#if 0
    //
    // I have decided not to scroll into view here. IE 4 didn't do this - and on long paragraphs
    // it looks ugly. What they clicked on is already in view - so this is probably ok.
    //
    IFC( GetViewServices()->ScrollPointerIntoView( _pEndPointer, _fNotAtBOL, POINTER_SCROLLPIN_Minimal ));
#endif    

    SetMadeSelection( FALSE );
Cleanup:    
    ReleaseInterface( pIElement );
    
    RRETURN ( hr );
}


//+==========================================================================================
//
//  Method: IsValidMove
//
// Synopsis: Check the Mouse has moved by at least _sizeDragMin - to be considered a "valid move"
//
//----------------------------------------------------------------------------------------------------
BOOL
CControlTracker::IsValidMove (SelectionMessage *pMessage)
{
    return ((abs(pMessage->pt.x - _startMouseX ) > gSizeDragMin.cx) ||
        (abs(pMessage->pt.y - _startMouseY) > gSizeDragMin.cy)) ;
}

//+==========================================================================================
//
//  Method: IsValidMove
//
// Synopsis: Check the Mouse has moved by at least _sizeDragMin - to be considered a "valid move"
//
//----------------------------------------------------------------------------------------------------
BOOL
CSelectTracker::IsValidMove (SelectionMessage *pMessage)
{
    return ((abs(pMessage->pt.x - _anchorMouseX ) > gSizeDragMin.cx) ||
        (abs(pMessage->pt.y - _anchorMouseY) > gSizeDragMin.cy)) ;
}

//+====================================================================================
//
// Method: InitMetrics
//
// Synopsis: Iniitalize any static vars for the SelectionManager
//
//------------------------------------------------------------------------------------

VOID
CSelectTracker::InitMetrics()
{
#ifdef WIN16
    // bugbug: (Stevepro) Isn't there an ini file setting for this in win16 that is used
    //         for ole drag-drop?
    //
    // Width and height, in pixels, of a rectangle centered on a drag point
    // to allow for limited movement of the mouse pointer before a drag operation
    // begins. This allows the user to click and release the mouse button easily
    // without unintentionally starting a drag operation
    //
    gSizeDragMin.cx = 3;
    gSizeDragMin.cy = 3;
#else
    gSizeDragMin.cx = GetSystemMetrics(SM_CXDRAG);
    gSizeDragMin.cy = GetSystemMetrics(SM_CYDRAG);
    g_iDragDelay = GetProfileIntA(s_achWindows, "DragDelay", 20);
#endif
}


//+====================================================================================
//
// Method: VeriyOkToStartSelection
//
// Synopsis: Verify that it is ok to start the selection now. If not we will Assert
//
//          The Selection manager should have inially called ShouldBeginSelection
//          before starting a given tracker. This method checks that this is so.
//
//------------------------------------------------------------------------------------

#if DBG == 1

VOID
CSelectTracker::VerifyOkToStartSelection( SelectionMessage * pMessage )
{

    HRESULT hr = S_OK;
    ELEMENT_TAG_ID eTag = TAGID_NULL;
    IHTMLElement* pElement = NULL;

    if ( _fState == ST_WAIT2 ) // it is always ok to start a selection in the middle of a selection
    {
        //
        // Examine the Context of the thing we started dragging in.
        //


        hr = THR( GetViewServices()->GetElementFromCookie( pMessage->elementCookie, & pElement ) );
        if ( hr )
            goto Cleanup;

        hr = THR( GetMarkupServices()->GetElementTagId(pElement, & eTag));
        if ( hr )
            goto Cleanup;

        Assert( ShouldStartTracker( _pManager, pMessage, eTag , pElement ) );
    }
Cleanup:

    ReleaseInterface( pElement );

}

#endif


HRESULT
CCaretTracker::Init2( 
                        CSelectionManager*      pManager,
                        SelectionMessage *      pMessage, 
                        DWORD*                  pdwFollowUpAction, 
                        TRACKER_NOTIFY *        peTrackerNotify,
                        DWORD                   dwTCFlags)
{    
    HRESULT hr = S_OK;
    
    Init();


    if ( pMessage )
    {
        hr = PositionCaretFromMessage( pMessage );
    }

    SetCaretShouldBeVisible( ShouldCaretBeVisible() );
    
#if DBG == 1
    BOOL fVisible = FALSE;
    TraceTag(( tagSelectionTrackerState, "\n---Start Caret Tracker--- Visible:%d\n",fVisible ));
#endif   

    return hr;
}


HRESULT
CCaretTracker::Init2(
                                CSelectionManager*      pManager,
                                IMarkupPointer*         pStart, 
                                IMarkupPointer*         pEnd, 
                                DWORD*                  pdwFollowUpAction, 
                                TRACKER_NOTIFY *        peTrackerNotify,
                                DWORD                   dwTCFlags,
                                CARET_MOVE_UNIT inLastCaretMove )
{
    HRESULT hr              = S_OK;
    BOOL    fNotAtBOL       = ENSURE_BOOL( dwTCFlags & TRACKER_CREATE_NOTATBOL);
    BOOL    fAtLogicalBOL   = ENSURE_BOOL( dwTCFlags & TRACKER_CREATE_ATLOGICALBOL);
    
    Init();

    _fNotAtBOL = fNotAtBOL; 
    _fAtLogicalBOL = fAtLogicalBOL;

    BOOL fStartPositioned = FALSE;
    BOOL fEndPositioned = FALSE;

    hr = THR( pStart->IsPositioned( & fStartPositioned ));
    if (!hr) hr = THR( pEnd->IsPositioned( & fEndPositioned) );

    if ( ! fStartPositioned || ! fEndPositioned )
    {
        _fValidPosition = FALSE;
    }
    
    if ( _fValidPosition )
    {
        hr = PositionCaretAt( pStart, _fNotAtBOL, _fAtLogicalBOL, CARET_DIRECTION_INDETERMINATE, POSCARETOPT_None, ADJPTROPT_None );
    }
    
    SetCaretShouldBeVisible( ShouldCaretBeVisible() );
    
#if DBG == 1
    int caretStart = GetCp( pStart) ;
    BOOL fVisible = FALSE;
    TraceTag(( tagSelectionTrackerState, "\n---Start Caret Tracker--- Cp: %d Visible:%d\n",caretStart, fVisible ));
#endif    

    return hr;
}

VOID
CCaretTracker::Init()
{
    _eType = SELECTION_TYPE_Caret;
    _fValidPosition = TRUE;
    _fNotAtBOL = FALSE;
    _fAtLogicalBOL = TRUE;
    _fCaretShouldBeVisible = TRUE;
}


CCaretTracker::~CCaretTracker()
{

}

HRESULT 
CCaretTracker::GetLocation(POINT *pPoint)
{
    HRESULT hr;
    
    SP_IHTMLCaret spCaret;

    IFR( GetViewServices()->GetCaret(&spCaret) );
    IFR( spCaret->GetLocation(pPoint, TRUE) );

    return S_OK;    
}

HRESULT
CCaretTracker::PositionCaretFromMessage(
                SelectionMessage* pMessage )
{
    IMarkupPointer * pPointer = NULL;

    ELEMENT_TAG_ID eTag = TAGID_NULL;
    BOOL fRightOfCp = FALSE;
    BOOL fValidTree = FALSE;

    HRESULT hr = S_OK;

    if ( pMessage  &&  pMessage->elementCookie )
    {
        //
        // We may require the caret to become visible again, but we should only
        // move on button down
        //
        if ( ( pMessage->message == WM_LBUTTONDOWN ) || 
             ( pMessage->message == WM_LBUTTONUP ) ||
             ( pMessage->message == WM_RBUTTONUP ) 
#ifdef UNIX
                || (pMessage->message == WM_MBUTTONDOWN)
#endif
            )
        {
            hr = THR( _pManager->GetMarkupServices()->CreateMarkupPointer( & pPointer));
            if ( hr )
                goto Cleanup;
            
            hr = _pManager->GetViewServices()->MoveMarkupPointerToMessage(  pPointer,
                                                                            pMessage,
                                                                            & _fNotAtBOL,
                                                                            & _fAtLogicalBOL,
                                                                            & fRightOfCp,
                                                                            & fValidTree,
                                                                            FALSE, 
                                                                            GetEditableElement(),
                                                                            NULL,
                                                                            FALSE );
            if( hr ) 
            {
                _fValidPosition = FALSE;
                goto Cleanup;
            }

            hr = PositionCaretAt( pPointer, _fNotAtBOL, _fAtLogicalBOL, fRightOfCp ? CARET_DIRECTION_BACKWARD : CARET_DIRECTION_FORWARD, POSCARETOPT_None, ADJPTROPT_None );
            if ( hr )
                goto Cleanup;
        }
        else
        {
            SetCaretShouldBeVisible ( ShouldCaretBeVisible() );
        }
        
        hr = GetTagIdFromMessage( pMessage, &eTag, _pManager->GetDoc(), _pManager->GetViewServices(), _pManager->GetMarkupServices() );
        if( hr )
            goto Cleanup;
    }    

Cleanup:
    ReleaseInterface( pPointer );
    RRETURN ( hr );
}

HRESULT
CCaretTracker::Position(
                IMarkupPointer* pPointer ,
                IMarkupPointer* pEnd,
                BOOL            fNotAtBOL,
                BOOL            fAtLogicalBOL)
{
    // Since we don't know where we are coming from, assume that we are at the EOL
    HRESULT hr = THR( PositionCaretAt( pPointer, fNotAtBOL, fAtLogicalBOL, CARET_DIRECTION_INDETERMINATE, POSCARETOPT_None, ADJPTROPT_None )); 
    
    RRETURN ( hr );
}

//+====================================================================================
//
// Method: OnEditFocusChanged
//
// Synopsis: Change the Visibility of the caret - based on the Edit Focus
//
//------------------------------------------------------------------------------------


VOID
CCaretTracker::OnEditFocusChanged()
{
    SetCaretShouldBeVisible( ShouldCaretBeVisible() );
}


//+====================================================================================
//
// Method: ConstrainPointer
//
// Synopsis: Check to see that the caret is in the edit context of the manager.
//           IF it isn't position appropriately.
//          If the caret has gone before the Start, postion at start of context
//          If the caret has gone after the end, position at end of context
//
//------------------------------------------------------------------------------------


HRESULT
CEditTracker::ConstrainPointer( IMarkupPointer* pPointer, BOOL fDirection)
{
    HRESULT hr = S_OK;
    IMarkupPointer* pPointerLimit = NULL;
    BOOL fAfterStart = FALSE;
    BOOL fBeforeEnd = FALSE;
        
    Assert(pPointer);

    if ( ! _pManager->IsInEditContext( pPointer ))
    {
        hr = THR( _pManager->IsAfterStart( pPointer, &fAfterStart ));
        if ( ( hr == CTL_E_INCOMPATIBLEPOINTERS ) || !fAfterStart )
        {
            if ( hr == CTL_E_INCOMPATIBLEPOINTERS )
            {
                //
                // Assume we're in a different tree. Based on the direction - we move
                // to the appropriate limit.
                // 
                if ( !fDirection ) // earlier in story
                {
                    pPointerLimit = _pManager->GetStartEditContext();
                }
                else
                {
                    pPointerLimit = _pManager->GetEndEditContext();
                }
            }
            else
            {            
                pPointerLimit = _pManager->GetStartEditContext();
            }        
            hr = THR( pPointer->MoveToPointer( pPointerLimit));
            goto Cleanup;
        }

        hr = THR( _pManager->IsBeforeEnd( pPointer, &fBeforeEnd ));
        if (( hr == CTL_E_INCOMPATIBLEPOINTERS ) || !fBeforeEnd )
        {
            if ( hr == CTL_E_INCOMPATIBLEPOINTERS )
            {
                //
                // Assume we're in a different tree. Based on the direction - we move
                // to the appropriate limit.
                // 
                if ( ! fDirection ) // earlier in story
                {
                    pPointerLimit = _pManager->GetStartEditContext();
                }
                else
                {
                    pPointerLimit = _pManager->GetEndEditContext();
                }
            }
            else
            {            
                pPointerLimit = _pManager->GetEndEditContext(); 
            }   
            hr = THR( pPointer->MoveToPointer( pPointerLimit));             
        }            
    }
Cleanup:
    RRETURN ( hr );
}

//+====================================================================================
//
// Method: AdjustForDeletion
//
// Synopsis: The World has been destroyed around us. Reposition ourselves.
//
//------------------------------------------------------------------------------------


BOOL
CCaretTracker::AdjustForDeletion( IMarkupPointer* pPointer )
{
    BOOL fLogicalBOLBefore = _fAtLogicalBOL;
    
    //
    // BUGBUG (johnbed) why is this hardcoded instead of passing _fAtLogicalBOL, and 
    // why do we set it back after we are done? This is unexpected use and deserves
    // at least a comment explaining what is going on here.
    //
    
    PositionCaretAt( pPointer, _fNotAtBOL, TRUE, CARET_DIRECTION_INDETERMINATE, POSCARETOPT_None, ADJPTROPT_None );

    _fAtLogicalBOL = fLogicalBOLBefore;                    
                    
    return FALSE;
}

//+====================================================================================
//
// Method: AdjsutForDeletion
//
// Synopsis: The Flow Layout we're in has been deleted
//
//  Return - TRUE - if we want to EmptySelection after we've been called
//           FALSE - if we don't want the manager to do anything.
//------------------------------------------------------------------------------------

BOOL
CEditTracker::AdjustForDeletion(IMarkupPointer * pPointer )
{
    return TRUE;
}

//+====================================================================================
//
// Method:      Position Caret At
//
// Synopsis:    Wrapper to place the Caret at a given TreePointer.
//
// WARNING:     You should normally use PositionCaretFromMessage
//
//------------------------------------------------------------------------------------

HRESULT
CCaretTracker::PositionCaretAt( 
    IMarkupPointer *        pPointer, 
    BOOL                    fNotAtBOL,
    BOOL                    fAtLogicalBOL,
    CARET_DIRECTION         eDir             /* = CARET_DIRECTION_INDETERMINATE */,
    DWORD                   fPositionOptions /* = POSCARETOPT_None */,
    DWORD                   dwAdjustOptions  /* = ADJPTROPT_None */   )
{
    HRESULT             hr = S_OK;
    CSpringLoader *     psl;
    SP_IHTMLCaret       spCaret;
    IHTMLViewServices * pvs = _pManager->GetViewServices();
    BOOL                fResetSpringLoader = FALSE;
    BOOL                fOutsideUrl = ! CheckFlag( dwAdjustOptions , ADJPTROPT_AdjustIntoURL );
    
    _fNotAtBOL =        fNotAtBOL;
    _fAtLogicalBOL =    fAtLogicalBOL;
    
    if( _pManager->HaveTypedSinceLastUrlDetect() )
    {
        IGNORE_HR( UrlAutodetectCurrentWord( NULL ) );
        _pManager->SetHaveTypedSinceLastUrlDetect( FALSE );
    }
    
    psl = GetSpringLoader();
    if (psl)
        fResetSpringLoader = !psl->IsSpringLoadedAt(pPointer);

    _lScreenXPosForVertMove = CARET_XPOS_UNDEFINED;    
    IFC( pvs->GetCaret( &spCaret ));
    
    if( ! CheckFlag( fPositionOptions, POSCARETOPT_DoNotAdjust ))
    {
        IFC( AdjustPointerForInsert( pPointer, _fNotAtBOL, _fAtLogicalBOL ? RIGHT : LEFT, _fAtLogicalBOL ? RIGHT : LEFT, dwAdjustOptions ));
    }
    SetCaretShouldBeVisible( ShouldCaretBeVisible() );
    IFC( spCaret->MoveCaretToPointerEx( pPointer, _fNotAtBOL, _fCaretShouldBeVisible , TRUE, eDir ));

    // Reset the spring loader.
    if (psl)
    {
        if (fOutsideUrl && !fResetSpringLoader && psl->IsSpringLoaded())
        {
            CEditPointer epTest(_pManager->GetEditor());
            DWORD        dwFound;

            IFC( epTest->MoveToPointer(pPointer) );
            IFC( epTest.Scan(LEFT, BREAK_CONDITION_OMIT_PHRASE, &dwFound) );

            fResetSpringLoader = epTest.CheckFlag(dwFound, BREAK_CONDITION_EnterAnchor);
        }
        else if (fResetSpringLoader)
        {
            fResetSpringLoader = !psl->IsSpringLoadedAt(pPointer);
        }

        IGNORE_HR(psl->SpringLoadComposeSettings(pPointer, fResetSpringLoader, TRUE));
    }
    
Cleanup:
    RRETURN( hr );
}

//+====================================================================================
//
// Method: CCaretTracker.HandleMessage
//
// Synopsis: Look for Keystrokes and such.
//
//------------------------------------------------------------------------------------


HRESULT
CCaretTracker::HandleMessage(
    SelectionMessage *pMessage,
    DWORD* pdwFollowUpAction,
    TRACKER_NOTIFY * peTrackerNotify )
{

    HRESULT hr = S_FALSE;
    IHTMLElement* pIElement = NULL;
    IHTMLElement* pIEditElement = NULL;
    IObjectIdentity * pIdent = NULL;

    if ( pMessage->message == WM_LBUTTONUP )
    {
        *pdwFollowUpAction |= FOLLOW_UP_ACTION_OnClick;
    }
    
    if ( _fValidPosition )
    {
        switch( pMessage->message )
        {
            case WM_LBUTTONDOWN:
            case WM_LBUTTONDBLCLK :
            case WM_RBUTTONUP:
#ifdef UNIX
            case WM_MBUTTONDOWN:
#endif
            
                hr = THR ( HandleMouseMessage( pMessage, pdwFollowUpAction, peTrackerNotify  ));
#ifdef UNIX
                if ( pMessage->message == WM_MBUTTONDOWN )
                    hr = S_FALSE;
#endif
                if (( pMessage->message == WM_RBUTTONUP ) || 
                    (!_pManager->IsContextEditable() ))
                    hr = S_FALSE;
                break;

            case WM_CHAR:

                hr = THR( HandleChar( pMessage, pdwFollowUpAction, peTrackerNotify ));
                break;


            case WM_KEYDOWN:
                hr = THR( HandleKeyDown( pMessage, pdwFollowUpAction, peTrackerNotify ));
                break;

            case WM_KEYUP:
                hr = THR( HandleKeyUp( pMessage, pdwFollowUpAction, peTrackerNotify ));
                break;

            case WM_INPUTLANGCHANGE:
                hr = THR( HandleInputLangChange() );
                break;
        }
    }
    else
    {
        //
        // The Caret is hidden inside a No-Scope or an Element that we can't go inside 
        // ( eg. Image )
        // If we clicked on the same element as our context - don't do anything
        //
        // If we didn't process the mouse down.
        //
        if ( ( pMessage->message == WM_LBUTTONDOWN ) || 
             (pMessage->message == WM_LBUTTONDBLCLK )  )
        {             
            IFC( GetElementFromMessage( _pManager, pMessage, & pIElement ) );
            IFC( pIElement->QueryInterface(IID_IObjectIdentity, (LPVOID *)&pIdent));        
            IFC( _pManager->GetEditableElement( & pIEditElement ));

            if (pIdent->IsEqualObject(pIEditElement) != S_OK)
            {
                hr = THR ( HandleMouseMessage( pMessage, pdwFollowUpAction, peTrackerNotify  ));
            }
            else
                hr = S_FALSE; // Not our event !
        }

    }

Cleanup:
    ReleaseInterface( pIElement );
    ReleaseInterface( pIEditElement );
    ReleaseInterface( pIdent );
    RRETURN1( hr, S_FALSE );

}


//+====================================================================================
//
// Method: HandleMouseMessage
//
// Synopsis: When we get a mouse message, we move the Caret, and end ourselves.
//
//------------------------------------------------------------------------------------

HRESULT
CCaretTracker::HandleMouseMessage(
        SelectionMessage *pMessage,
        DWORD* pdwFollowUpAction,
        TRACKER_NOTIFY * peTrackerNotify )
{
    HRESULT hr = S_OK;
    BOOL fRightOfCp = FALSE;
    SP_IMarkupPointer spStart;
    SP_IMarkupPointer spEnd;
    SP_IHTMLCaret spCaret;

#ifndef NO_IME
    if ( _pManager->IsIMEComposition())
    {
        hr = THR( _pManager->HandleImeMessage( pMessage, pdwFollowUpAction, peTrackerNotify ) );
    }
    else
#endif // NO_IME

    if ( ! IsShiftKeyDown() )
    {
        hr = PositionCaretFromMessage( pMessage );

        if ( peTrackerNotify && 
             pMessage->message != WM_RBUTTONUP )
            *peTrackerNotify = TN_END_TRACKER_RESELECT;  // Give it to a Selection Tracker
    }
    else
    {
        if( FireSelectStartMessage( pMessage ) )
        {
            IFC( GetMarkupServices()->CreateMarkupPointer( & spStart ));
            IFC( GetMarkupServices()->CreateMarkupPointer( & spEnd ));
            IFC( GetViewServices()->GetCaret( & spCaret ));
            
            if( ShouldCaretBeVisible() )
            {
                IFC( spCaret->Show( FALSE ) );            
            }
            
            IFC( spCaret->MovePointerToCaret( spStart ));

            IFC( GetViewServices()->MoveMarkupPointerToMessage( spEnd,
                                                                pMessage,
                                                                &_fNotAtBOL,
                                                                &_fAtLogicalBOL,
                                                                &fRightOfCp,
                                                                NULL,
                                                                FALSE,
                                                                GetEditableElement(),
                                                                NULL ,
                                                                TRUE )); // Hit test eol is true
            _pManager->CopyTempMarkupPointers( spStart, spEnd );
            *peTrackerNotify = TN_END_TRACKER_POS_SELECT;
        }        
    }

Cleanup:
    RRETURN1( hr, S_FALSE );
}

//+----------------------------------------------------------------------------
//
//  Function:   UrlAutoDetectCurrentWord
//
//  Synopsis:   Performs URL autodetection on the current word (ie, around
//      or just prior to the caret). 
//      Note: Autodetection is not always triggered by a character (can
//      be by caret movement, etc.).  In this case, pChar should be NULL,
//      and some rules are different.
//
//  Arguments:  [pChar]     The character entered that triggered autodetction.
//
//  Returns:    HRESULT     S_OK if everything's cool, otherwise error
//
//-----------------------------------------------------------------------------
HRESULT
CCaretTracker::UrlAutodetectCurrentWord( OLECHAR *pChar )
{
    HRESULT                 hr              = S_OK;
    IHTMLViewServices   *   pvs             = _pManager->GetViewServices();
    IHTMLCaret          *   pc              = NULL;
    IHTMLElement        *   pElement        = NULL;
    IHTMLElement        *   pAnchor         = NULL;
    IMarkupServices     *   pms             = _pManager->GetMarkupServices();
    IMarkupPointer      *   pmp             = NULL;
    IMarkupPointer      *   pLeft           = NULL;
    BOOL                    fFound          = FALSE;
    BOOL                    fInsideLink     = FALSE;
    BOOL                    fOutsideLink    = FALSE;
    BOOL                    fLimit          = FALSE;
    BOOL                    fLeftOf         = FALSE;

    if( IsContextEditable() )
    {
        hr = THR( pvs->GetCaret( &pc ) );
        if( hr )
            goto Cleanup;

        hr = THR( pms->CreateMarkupPointer( &pmp ) );
        if( hr )
            goto Cleanup;

        hr = THR( pms->CreateMarkupPointer( &pLeft ) );
        if( hr )
            goto Cleanup;

        hr = THR( pc->MovePointerToCaret( pmp ) );
        if( hr )
            goto Cleanup;

        if( pChar )
        {    
            hr = THR( pmp->MoveUnit( MOVEUNIT_PREVCHAR ) );
            if( hr )
                goto Cleanup;

            if( VK_RETURN == *pChar )
            {
                pChar = NULL;
            }
            else 
            {
                fLimit = ( _T('"') == *pChar ||
                           _T('>') == *pChar );

                if( fLimit )
                {
                    // If we're in an anchor, don't limit because of a quote.
                    IFC( pmp->CurrentScope( &pElement ) );

                    IFC( FindTagAbove( pms, pElement, TAGID_A, &pAnchor ) );
                    fLimit = !pAnchor;
                }
            }
        }

        // Position left to our current position
        hr = THR( pLeft->MoveToPointer( pmp ) );
        if( hr ) 
            goto Cleanup;

        // Fire off the autodetection
        // TODO: turn fInsideLink/fOutsideLink into enum [ashrafm]
        hr = THR( AutoUrl_DetectCurrentWord( pms, pmp, pChar, &fInsideLink, &fOutsideLink, fLimit ? pmp : NULL, pLeft, &fFound ) );
        if( hr )
            goto Cleanup;

        if (fInsideLink || fOutsideLink)
        {
            CEditPointer epAdjust(_pManager->GetEditor());
            DWORD        dwSearch = BREAK_CONDITION_OMIT_PHRASE;
            DWORD        dwFound;
            Direction    eTextDir;
            
            Assert(!fInsideLink || !fOutsideLink);
            
            IFC( pc->MovePointerToCaret( epAdjust ) );
            eTextDir = fInsideLink ? LEFT : RIGHT;

            IFC( epAdjust.Scan(eTextDir, dwSearch - BREAK_CONDITION_Anchor, &dwFound) );
            if (epAdjust.CheckFlag(dwFound, dwSearch))
            {
                IFC( epAdjust.Scan(Reverse(eTextDir), dwSearch, &dwFound) );
            }
            
            _fNotAtBOL = TRUE;
            _fAtLogicalBOL = FALSE;
            hr = THR( pc->MoveCaretToPointer( epAdjust, _fNotAtBOL, TRUE, CARET_DIRECTION_INDETERMINATE ));
            if( hr )
                goto Cleanup;
        }
        
        if( _pManager->HaveTypedSinceLastUrlDetect() )
        {
            // If we didn't find a URL, pLeft wasn't moved, so do our best
            // to avoid autodetecting any more than we have to - go back
            // one word.
            if( !fFound )
            {
                hr = THR( pLeft->MoveUnit( MOVEUNIT_PREVWORDEND ) );
                if( hr )
                    goto Cleanup;
            }

            hr = THR( _pManager->GetPtrBeganTyping()->IsLeftOf( pLeft, &fLeftOf ) );
            if( hr )
                goto Cleanup;

            // If there's still stuff we haven't detected
            if( fLeftOf )
            {
                hr = THR( AutoUrl_DetectRange( pms, _pManager->GetPtrBeganTyping(), pLeft ) );
                if( hr )
                    goto Cleanup;
            }
        }
    }
            
Cleanup:

    ReleaseInterface( pc );
    ReleaseInterface( pmp );
    ReleaseInterface( pElement );
    ReleaseInterface( pAnchor );
    ReleaseInterface( pLeft );

    RRETURN( hr );
}


BOOL
CCaretTracker::IsContextEditable()
{

#if 0 // BUBBUG: This breaks HTMLDialogs due to their wacky select code
    BOOL fOut = FALSE;
    HRESULT hr = S_OK;
    SP_IHTMLCaret spCaret;
    
    IFC( _pManager->GetViewServices()->GetCaret( & spCaret ));
    IFC( spCaret->IsVisible( & fOut ));
    fOut = fOut && _pManager->IsContextEditable();
Cleanup:
    return fOut;
#else

    return _pManager->IsContextEditable() && _fCaretShouldBeVisible ;

#endif

}


//+====================================================================================
//
// Method: HandleChar
//
// Synopsis: Delete the Selection, and cause this tracker to end ( & kill us ).
//
//------------------------------------------------------------------------------------

HRESULT
CCaretTracker::HandleChar(
                SelectionMessage *pMessage,
                DWORD* pdwFollowUpAction,
                TRACKER_NOTIFY * peTrackerNotify )
{
    HRESULT     hr = S_FALSE;
    CUndoUnit   undoUnit(_pManager->GetEditor());
    IHTMLElement * pIElement = NULL;
    IHTMLInputElement* pIInputElement = NULL;
    BSTR bstrType = NULL;
    
    // Char codes we DON'T handle go here
    switch( pMessage->wParam )
    {
        case VK_BACK:
        case VK_F16:
            hr = S_OK;
            goto Cleanup;
            
        case VK_ESCAPE:
        {
            hr = S_FALSE;
            goto Cleanup;
        }            
    }

    if( pMessage->wParam < ' ' && pMessage->wParam != VK_TAB && pMessage->wParam != VK_RETURN )
    {
        hr = S_FALSE;
        goto Cleanup;
    }

    IFC( undoUnit.Begin(_pManager->GetOverwriteMode() ? IDS_EDUNDOOVERWRITE : IDS_EDUNDOTYPING) );
    
    if ( IsContextEditable() )
    {
        SP_IHTMLCaret pc;
        OLECHAR t ;
        BOOL fOverWrite = _pManager->GetOverwriteMode();
        IFC( _pManager->GetViewServices()->GetCaret( &pc ));
            
        t = (OLECHAR)pMessage->wParam;

        switch (pMessage->wParam)
        {
            case VK_TAB:
            {
                
                if( IsCaretInPre( pc ))
                {
                    if( fOverWrite )
                    {
                        SP_IMarkupPointer   spPos;
                        IFC( _pManager->GetMarkupServices()->CreateMarkupPointer( & spPos ));
                        IFC( pc->MovePointerToCaret( spPos ));
                        IFC( DeleteNextChar( spPos ));
                    }

                    t = 9;
                    IFC( InsertText( &t, 1, pc ));                    
                }
                else
                {
                    hr = S_FALSE;
                }

                break;
            }

            case VK_RETURN:
            {
                IGNORE_HR( UrlAutodetectCurrentWord( &t ) );
                _pManager->SetHaveTypedSinceLastUrlDetect( FALSE );
                IFC( HandleEnter(pc, pMessage->fShift, pMessage->fCtrl) );
                // Autodetect on space, return, or anyof the character in the string.

                break;
            }
            
            case VK_SPACE:
            {
                IFC( HandleSpace( (OLECHAR) pMessage->wParam ));
                break;
            }
            
            default:
            {
                BOOL fAccept = TRUE;

                if( fOverWrite )
                {
                    IFC( DeleteNextChar( NULL ));
                }

                if(_pManager->HasActiveISC())
                {
                    SP_IMarkupPointer   spPos;
                    MARKUP_CONTEXT_TYPE eCtxt;
                    LONG cch = ISC_BUFLENGTH;
                    OLECHAR aryISCBuffer[ISC_BUFLENGTH];
                    BOOL fPassword = FALSE;

                    IFC( _pManager->GetMarkupServices()->CreateMarkupPointer( & spPos ));
                    IFC( pc->MovePointerToCaret( spPos ));
                    IFC( GetViewServices()->LeftOrSlave(spPos, TRUE, & eCtxt , NULL , &cch , aryISCBuffer ));

                    // paulnel: if we are editing a password field we want to allow any character combination
                    if ( _pManager->GetEditableTagId() == TAGID_INPUT )
                    {
                        //
                        // Get the Master. Due to the Input/Text Slave architecture we have to get the master
                        // element this way.
                        IFC( GetViewServices()->GetElementForSelection( _pManager->GetEditableElement(),
                                                          & pIElement));
                                                          
                        IFC( pIElement->QueryInterface (IID_IHTMLInputElement, 
                                                        ( void** ) &pIInputElement ));
                                
                        IFC(pIInputElement->get_type(&bstrType));
            
                        if (!_tcsicmp( bstrType, TEXT("password")))
                        {
                            fPassword = TRUE;
                        }
                    }

                    if(!fPassword)
                        fAccept = _pManager->GetISCList()->CheckInputSequence(aryISCBuffer, cch, t);
                }

                if( fAccept )
                    IFC( InsertText( &t, 1, pc ));
                    
                break;
            }
        }
    }

Cleanup:
    ReleaseInterface( pIInputElement );
    ReleaseInterface( pIElement );
    SysFreeString(bstrType);

    return hr ;
}

//+====================================================================================
//
// Method: HandleSpace
//
// Synopsis: Handle conversion of spaces to NBSP.
//
//------------------------------------------------------------------------------------



HRESULT
CCaretTracker::HandleSpace( OLECHAR t )
{
    HRESULT hr = S_OK;
    SP_IMarkupPointer   spPos;
    SP_IHTMLCaret pc;

    BOOL fOverWrite = _pManager->GetOverwriteMode();
    IFC( _pManager->GetViewServices()->GetCaret( &pc ));

    if( fOverWrite )
    {
        IFC( DeleteNextChar( NULL ));
    }

    //
    // Only insert nbsp if the container can accept html and isn't in a PRE tag
    //
    if( _pManager->CanContextAcceptHTML() && ! IsCaretInPre( pc ) )
    {
        CEditPointer   spPos( _pManager->GetEditor() );
        DWORD dwSearch = BREAK_CONDITION_OMIT_PHRASE;
        DWORD dwFound = BREAK_CONDITION_None;                    
        OLECHAR chTest;

        IFC( pc->MovePointerToCaret( spPos ));
        IFC( spPos.Scan( LEFT , dwSearch , &dwFound , NULL , NULL , & chTest , NULL ));
        
        if( spPos.CheckFlag( dwFound, BREAK_CONDITION_Text ) && chTest == 32 )
        {
            //
            // there is a space to the left of us, delete it and insert
            // "&nbsp "
            //
            
            SP_IMarkupPointer spCaret;
            OLECHAR t[2];
            t[0] = 160;
            t[1] = 32;
            
            IFC( _pManager->GetMarkupServices()->CreateMarkupPointer( & spCaret ));
            IFC( pc->MovePointerToCaret( spCaret ));
            IFC( _pManager->GetMarkupServices()->Remove( spCaret, spPos ));
            IFC( InsertText( t, 2, pc ));                    

            goto Cleanup;
            
        }
        else if( spPos.CheckFlag( dwFound, BREAK_CONDITION_TEXT - BREAK_CONDITION_Text ) ||
                 spPos.CheckFlag( dwFound, BREAK_CONDITION_Site )                        ||
                 spPos.CheckFlag( dwFound, BREAK_CONDITION_Block ))

        {
            //
            // There is an image, control, block or site to the left of us
            // so we should insert an &nbsp
            //

            t = 160;
        }
        
#if 0
        // This breaks the case where you have "a b", place the caret before the space
        // and type. What IE4 does is convert any inserted nbsp's back to a space when
        // you type the next character. This, however, is also bad. If the user actually
        // typed "anbsp;b" on purpose, IE4 converts it to "a b". A fair compromise is to
        // just have the user type an additional space to get the desired white space.
        
        else
        {
            //
            // plain text or nbsp to my left, look right. If there is a space to my right,
            // insert an nbsp.
            //
            
            IFC( pc->MovePointerToCaret( spPos ));
            dwFound = BREAK_CONDITION_None;
            IFC( spPos.Scan( RIGHT , dwSearch , &dwFound , NULL , NULL , & chTest , NULL ));

            if( spPos.CheckFlag( dwFound, BREAK_CONDITION_Text ) && chTest == 32 )
            {
                // there is a space to our right, we should insert an nbsp

                t = 160;
            }
            
        }
#endif 0

    }

    IFC( InsertText( &t, 1, pc ));

Cleanup:
    RRETURN( hr );
}
           
BOOL CCaretTracker::IsCaretInPre( IHTMLCaret * pCaret )
{
    HRESULT hr = S_OK;
    HTMLCharFormatData stFormatData;
    stFormatData.fPre = FALSE;
    SP_IMarkupPointer spPointer;
    IFC( _pManager->GetMarkupServices()->CreateMarkupPointer( & spPointer ));
    IFC( pCaret->MovePointerToCaret( spPointer ));
    IFC( _pManager->GetViewServices()->GetCharFormatInfo( spPointer, CHAR_FORMAT_ParaFormat, &stFormatData ));

Cleanup:
    return stFormatData.fPre;
    
}

HRESULT 
CCaretTracker::SetNotAtBOL(BOOL fNotAtBOL)
{
    HRESULT         hr;
    SP_IHTMLCaret   spCaret;

    IFR( CEditTracker::SetNotAtBOL(fNotAtBOL) );
    IFR( GetViewServices()->GetCaret(&spCaret) );
    if (spCaret != NULL)
    {
        IFR( spCaret->SetNotAtBOL(fNotAtBOL) );
    }

    return S_OK;
}

HRESULT
CCaretTracker::DeleteNextChar(
    IMarkupPointer *    pPos )
{
    HRESULT hr = S_OK;
    LONG cch;
    OLECHAR chTest = 0;
    MARKUP_CONTEXT_TYPE eCtxt;
    SP_IMarkupPointer spEnd;
    SP_IHTMLElement spElement;
    BOOL fDone = FALSE;
    
    IFC( _pManager->GetMarkupServices()->CreateMarkupPointer( & spEnd ));

    if( pPos )
    {
        IFC( spEnd->MoveToPointer( pPos ));
    }
    else
    {
        SP_IHTMLCaret pc;
        IFC( _pManager->GetViewServices()->GetCaret( & pc ));
        IFC( pc->MovePointerToCaret( spEnd ));
    }

    while( ! fDone )
    {
        cch = 1;
        IFC( GetViewServices()->RightOrSlave(spEnd, TRUE, & eCtxt , & spElement , & cch , & chTest ));

        switch( eCtxt )
        {
            case CONTEXT_TYPE_Text:
                if( cch == 1 )
                {
                    //
                    // If we hit a \r, we essentially hit a block break - we are done
                    //
                    
                    if( chTest != '\r' )
                    {                    
                        //
                        // Passed over a character, back up, move a markup pointer to 
                        // the start, and jump the end to the next cluster end point.
                        //
                        SP_IMarkupPointer spStart;
                        
                        
                        IFC( GetViewServices()->LeftOrSlave(spEnd, TRUE, & eCtxt , NULL , & cch , & chTest ));
                        Assert( eCtxt == CONTEXT_TYPE_Text );
                        IFC( _pManager->GetMarkupServices()->CreateMarkupPointer( & spStart ));
                        IFC( spStart->MoveToPointer( spEnd ));
                        IFC( spEnd->MoveUnit( MOVEUNIT_NEXTCLUSTEREND )); 
                        IFC( _pManager->GetMarkupServices()->Remove( spStart, spEnd ));
                    }
                    
                    fDone = TRUE;
                }
                break;
                
            case CONTEXT_TYPE_EnterScope:
            case CONTEXT_TYPE_ExitScope:
            {
                BOOL fLayout = FALSE;
                IFC( _pManager->GetViewServices()->IsLayoutElement( spElement, & fLayout ));

                if( ! fLayout )
                {
                    IFC( _pManager->GetViewServices()->IsBlockElement( spElement, &fLayout ));
                }
                
                fDone = fLayout;    // we are done if we hit a layout...
                break;
            }

            default:
                break;
        }
    }

Cleanup:
    RRETURN( hr );
}

HRESULT 
CCaretTracker::IsQuotedURL(IHTMLElement *pAnchorElement)
{
    HRESULT         hr;
    CEditPointer    ep(GetEditor());
    TCHAR           ch = 0;
    DWORD           dwFound;

    IFC( ep->MoveAdjacentToElement(pAnchorElement, ELEM_ADJ_BeforeBegin) );
    IFC( ep.Scan(LEFT, BREAK_CONDITION_OMIT_PHRASE, &dwFound, NULL, NULL, &ch) );

    hr = S_FALSE; // not quoted
    if (ep.CheckFlag(dwFound, BREAK_CONDITION_Text) && (ch == '"' || ch == '<'))
    {
        hr = S_OK;
    }    

Cleanup:
    RRETURN1(hr, S_FALSE);
}


HRESULT
CCaretTracker::InsertText( 
    OLECHAR    *    pText,
    LONG            lLen,
    IHTMLCaret *    pc)
{
    HRESULT             hr = S_OK;
    CSpringLoader       * psl = NULL;
    SP_IMarkupPointer   spStartPosition;
    SP_IMarkupPointer   spCaretPosition;
    CUndoUnit           undoUnit(_pManager->GetEditor());
    
    IFC( undoUnit.Begin(IDS_EDUNDOTYPING) );

    if( !_pManager->HaveTypedSinceLastUrlDetect() )
    {
        pc->MovePointerToCaret( _pManager->GetPtrBeganTyping() );
    }

    // Get the spring loaded state
    psl = GetSpringLoader();

    // If whitespace, make sure we fall out of the current URL (if any)
    if (pText && (lLen >= 1 || lLen == -1) && IsWhiteSpace(*pText) && pc)
    {
        CEditPointer ep(GetEditor());
        DWORD        dwFound;
        BOOL         fNotAtBOL = FALSE;

        // A failure here should not abort the text insertion
        hr = THR( pc->MovePointerToCaret(ep) );            
        if (SUCCEEDED(hr))
        {
            SP_IHTMLElement spElement; 
            
            hr = THR( ep.Scan(RIGHT, BREAK_CONDITION_OMIT_PHRASE, &dwFound, &spElement) );
            if (SUCCEEDED(hr) 
                && ep.CheckFlag(dwFound, BREAK_CONDITION_ExitAnchor)
                && IsQuotedURL(spElement) == S_FALSE)
            {
                if (SUCCEEDED(pc->GetNotAtBOL(&fNotAtBOL)))
                {
                    hr = THR( pc->MoveCaretToPointer(ep, fNotAtBOL, FALSE /* fScrollIntoView */, CARET_DIRECTION_INDETERMINATE) );

                    // Can't trust spring loaded space from inside the anchor,
                    // so reset
                    if (SUCCEEDED(hr) && psl)
                        psl->Reset();
                }
            }
        }
    }

    // Fire the spring loader at the caret position.
    if (psl && psl->IsSpringLoaded())
    {
        IFC( GetMarkupServices()->CreateMarkupPointer( & spStartPosition ));
        IFC( pc->MovePointerToCaret( spStartPosition) );        

        IFC( pc->InsertText( pText , lLen ) );
    
        IFC( GetMarkupServices()->CreateMarkupPointer( & spCaretPosition ));
        IFC( pc->MovePointerToCaret( spCaretPosition ));
        IGNORE_HR(psl->Fire( spStartPosition, spCaretPosition, FALSE ));
    }
    else
    {
        IFC( pc->InsertText( pText , lLen ) );
    }

    _fNotAtBOL = TRUE;
    _fAtLogicalBOL = FALSE;
    

    // Autodetect on space, return, or anyof the character in the string.
    if( _T('(') == pText[0]
        || _T(')') == pText[0]
        || VK_TAB == (DWORD) pText[0]
        || VK_SPACE == (DWORD) pText[0] )
    {
        IGNORE_HR( UrlAutodetectCurrentWord( pText ) );
        _pManager->SetHaveTypedSinceLastUrlDetect( FALSE );
    }
    else if (_T('"') == pText[0]
             || _T('>') == pText[0])
    {
        CEditPointer ep(_pManager->GetEditor());
        DWORD        dwFound;
        
        // Before we autodetect again on a quote, make sure we don't have an url to the left of us.
        // This behavior will allow the user to add quotes around a url without them being deleted.

        IFC( pc->MovePointerToCaret(ep) );
        IFC( ep.Scan(LEFT, BREAK_CONDITION_OMIT_PHRASE, &dwFound) ); // skip quote
        Assert(ep.CheckFlag(dwFound, BREAK_CONDITION_Text)); 

        IFC( ep.Scan(LEFT, BREAK_CONDITION_OMIT_PHRASE, &dwFound) ); // find out what is left of the quote

        if (!ep.CheckFlag(dwFound, BREAK_CONDITION_EnterAnchor))
        {
            IGNORE_HR( UrlAutodetectCurrentWord( pText ) );
            _pManager->SetHaveTypedSinceLastUrlDetect( FALSE );
        }        
    }
    else
    {
        _pManager->SetHaveTypedSinceLastUrlDetect( TRUE );
    }
    
Cleanup:
    RRETURN( hr );
}


HRESULT
CCaretTracker::HandleKeyDown(
                SelectionMessage *pMessage,
                DWORD* pdwFollowUpAction,
                TRACKER_NOTIFY * peTrackerNotify)
{
    CSpringLoader  * psl;
    HRESULT          hr = S_FALSE;
    IHTMLElement   * pIEditElement = NULL;
    IMarkupPointer * pStartCaret = NULL;
    IMarkupPointer * pEndCaret  = NULL;
    IHTMLElement   * pISelectElement = NULL;
    long             eHTMLDir = htmlDirLeftToRight;
    BOOL             fHadTyped;

    _fShiftCapture = FALSE;
    
    if ( IsContextEditable() )
    {
        switch(pMessage->wParam)
        {
            case VK_BACK:
            case VK_F16:
            {
                // tell the caret which way it is moving in the logical string
                SP_IHTMLCaret       spCaret;
                SP_IMarkupPointer   spPointer;
                BOOL                fDelaySpringLoad = FALSE;

                IFC( _pManager->GetMarkupServices()->CreateMarkupPointer( &spPointer ));
                
                IFC( _pManager->GetViewServices()->GetCaret( &spCaret ));
    
                IFC( spCaret->MovePointerToCaret( spPointer ));

                psl = GetSpringLoader();        
                if (psl)
                {
                    IFC( MustDelayBackspaceSpringLoad(psl, spPointer, &fDelaySpringLoad) );
                    
                    if (!fDelaySpringLoad)
                        IFC( psl->SpringLoad(spPointer, SL_ADJUST_FOR_INSERT_LEFT | SL_TRY_COMPOSE_SETTINGS | SL_RESET) );
                }

                IFC( _pManager->GetEditor()->DeleteCharacter( spPointer , TRUE, pMessage->fCtrl, _pManager->GetStartEditContext() ));

                if (psl && fDelaySpringLoad)
                {
                    IFC( psl->SpringLoad(spPointer, SL_ADJUST_FOR_INSERT_LEFT | SL_TRY_COMPOSE_SETTINGS | SL_RESET) );
                }
                
                // F16 (forward DELETE) does not move the caret, hence, we use the same flags. However,
                // Backspace defaults the the beginning of the line.
                
                if( pMessage->wParam == VK_BACK )
                {
                    _fNotAtBOL = FALSE;
                    _fAtLogicalBOL = TRUE;
                }
                
                // Disable auto-detect during backspace
                fHadTyped = _pManager->HaveTypedSinceLastUrlDetect();
                _pManager->SetHaveTypedSinceLastUrlDetect( FALSE );
                IFC( PositionCaretAt( spPointer, _fNotAtBOL , _fAtLogicalBOL , CARET_DIRECTION_FORWARD , POSCARETOPT_None, ADJPTROPT_AdjustIntoURL ));

                if( !fHadTyped )
                {
                    _pManager->GetPtrBeganTyping()->MoveToPointer( spPointer );
                }
                _pManager->SetHaveTypedSinceLastUrlDetect( TRUE );

                break;
            }


            case VK_ESCAPE:
            {
                //
                // If we're in a UI Active control - we site select it.
                //
                if ( _pManager->HasFocusAdorner())
                {
                    //
                    // We create 2 pointers, move them around the control, Position the Temp Markup Pointers
                    // and then Site Select the Control
                    //
                    IFC( GetMarkupServices()->CreateMarkupPointer( & pStartCaret ));
                    IFC( GetMarkupServices()->CreateMarkupPointer( & pEndCaret ));
                    IFC( _pManager->GetEditableElement( & pIEditElement));
                    IFC( GetViewServices()->GetElementForSelection( pIEditElement, &pISelectElement ));
                    IFC( pStartCaret->MoveAdjacentToElement( pISelectElement, ELEM_ADJ_BeforeBegin ));
                    IFC( pEndCaret->MoveAdjacentToElement( pISelectElement, ELEM_ADJ_AfterEnd ));
                    _pManager->CopyTempMarkupPointers( pStartCaret, pEndCaret );
                    
                    _pManager->SetPendingTrackerNotify( TRUE, TN_END_TRACKER_POS_CONTROL );
                    IFC( GetViewServices()->MakeParentCurrent( pISelectElement ));
                    //
                    // BUGBUG from here on - this control is dead.
                    //
                    hr = S_OK;
                    goto Cleanup;
                }
            }            
            break;
                
            case VK_INSERT: // we should handle this in keydown - sets the appropriate flag
            {
                BOOL newORMode = ! ( _pManager->GetOverwriteMode());
                
                if( pMessage->fShift || pMessage->fCtrl || pMessage->fAlt )
                {
                    hr = S_FALSE;
                    break;
                }
                
                hr = S_OK;
                _pManager->SetOverwriteMode( newORMode );
                break;
            }

            case VK_TAB:
            {
                //
                // BUGBUG: This is a hack. We have to do this because Trident never gives us
                // the VK_TAB as a WM_CHAR message.
                //
                if( pMessage->fShift || pMessage->fCtrl || pMessage->fAlt )
                {
                    hr = S_FALSE;
                    break;
                }

                hr = THR( HandleChar( pMessage, pdwFollowUpAction, peTrackerNotify ));
                break;
            }

#ifndef NO_IME
            case VK_KANJI:
                if (   949 == GetKeyboardCodePage()
                    && _pManager->IsContextEditable())
                {
                    THR(_pManager->StartHangeulToHanja(peTrackerNotify));
                }
                hr = S_OK;
                break;
#endif // !NO_IME

                // only get line direction if the left or right key are used 
                // this is a performance enhancement 
            case VK_LEFT:
            case VK_RIGHT:
            {
                SP_IHTMLCaret       spCaret;
                SP_IMarkupPointer   spPointer;
                HRESULT             hr2 = S_FALSE;

                hr2 = THR( GetMarkupServices()->CreateMarkupPointer( &spPointer ));
                if (hr2)
                    goto Cleanup;

                hr2 = THR( GetViewServices()->GetCaret( &spCaret ));
                if (hr2)
                    goto Cleanup;

                hr2 = THR( spCaret->MovePointerToCaret( spPointer ));
                if (hr2)
                    goto Cleanup;

                hr2 = THR( GetViewServices()->GetLineDirection( spPointer, _fNotAtBOL, & eHTMLDir ));
                if (hr2)
                    goto Cleanup;
            }
            // we don't want a break here. Fall through to default 
            default:
            {
                CARET_MOVE_UNIT cmu = GetMoveDirectionFromMessage( pMessage, (eHTMLDir == htmlDirRightToLeft) );
                
                if( cmu != CARET_MOVE_NONE )
                {
                    if( IsShiftKeyDown() )
                    {
                        IFC( _pManager->GetEditableElement( &pIEditElement ));
                        if ( GetViewServices()->FireOnSelectStart( pIEditElement ) )                        
                            *peTrackerNotify = TN_END_TRACKER_SHIFT_START;
                    }
                    else
                    {
                        SP_IMarkupPointer spPointer;
                        IFC( _pManager->GetMarkupServices()->CreateMarkupPointer( &spPointer ));
                        IFC( MoveCaret( cmu, spPointer, TRUE ));
                    }
                }
                break;
            }
        }

        if(pMessage->wParam == VK_SHIFT && pMessage->fCtrl)
            _fShiftCapture = TRUE;
    }


Cleanup:
    ReleaseInterface( pStartCaret );
    ReleaseInterface( pEndCaret );
    ReleaseInterface( pIEditElement );
    ReleaseInterface( pISelectElement );

    return( hr );
}


CARET_MOVE_UNIT
CCaretTracker::GetMoveDirectionFromMessage( SelectionMessage* pMessage, BOOL fRightToLeft )
{   

    switch( pMessage->wParam)
    {
        case VK_LEFT:
            if(!fRightToLeft)
            {
                if( pMessage->fCtrl )
                    return CARET_MOVE_WORDLEFT;
                else
                    return CARET_MOVE_LEFT;
            }
            else
            {
                if( pMessage->fCtrl )
                    return CARET_MOVE_WORDRIGHT;
                else
                    return CARET_MOVE_RIGHT;
            }
            break;

        case VK_RIGHT:
            if(!fRightToLeft)
            {
                if( pMessage->fCtrl )
                    return CARET_MOVE_WORDRIGHT;
                else
                    return CARET_MOVE_RIGHT;
            }
            else
            {
                if( pMessage->fCtrl )
                    return CARET_MOVE_WORDLEFT;
                else
                    return CARET_MOVE_LEFT;
            }
            break;

        case VK_UP:
            if( pMessage->fCtrl )
                return CARET_MOVE_BLOCKSTART;
            else
                return CARET_MOVE_UP;
            break;

        case VK_DOWN:
            if( pMessage->fCtrl )
                return CARET_MOVE_BLOCKEND;
            else
                return CARET_MOVE_DOWN;
            break;
        
        case VK_PRIOR:
            if( pMessage->fCtrl )
                return CARET_MOVE_VIEWSTART;
            else
                return CARET_MOVE_PAGEUP;
            break;

        case VK_NEXT:
            if( pMessage->fCtrl )
                return CARET_MOVE_VIEWEND;
            else
                return CARET_MOVE_PAGEDOWN;
            break;

        case VK_HOME:
            if( pMessage->fCtrl )
                return CARET_MOVE_HOME;
            else
                return CARET_MOVE_LINESTART;
            break;

        case VK_END:
            if( pMessage->fCtrl )
                return CARET_MOVE_END;
            else
                return CARET_MOVE_LINEEND;
            break;
    }
    return CARET_MOVE_NONE;
} 

#define OEM_SCAN_RIGHTSHIFT 0x36

HRESULT
CCaretTracker::HandleKeyUp(
                SelectionMessage *pMessage,
                DWORD* pdwFollowUpAction,
                TRACKER_NOTIFY * peTrackerNotify)
{
    HRESULT hr = S_FALSE;
    IHTMLElement* pIEditElement = NULL;
    
    // We have potentially had a direction change message passed to us
    if(pMessage->wParam == VK_SHIFT && pMessage->fCtrl && _fShiftCapture)
    {
        _fShiftCapture = FALSE;
    
        // 1. Find out if the system is a right to left keyboard installed
        BOOL fBidiEnabled;
        IFC( GetViewServices()->IsBidiEnabled(&fBidiEnabled) );

        if(fBidiEnabled)
        {
            // 2. if we have right to left system capabilities, set the direction as appropriate.
            //    If we are editable we want to change the block direction of the current element
            if ( _pManager->IsContextEditable() )
            {
                // if the right shift key is coming up we are going to be changing to right to left
                // if the left shift key is coming up we are going to be changing to left to right
                long eHTMLDir = (LOBYTE(HIWORD(pMessage->lParam)) == OEM_SCAN_RIGHTSHIFT) 
                                ? htmlDirRightToLeft
                                : htmlDirLeftToRight;

                IFC( _pManager->GetEditableElement( & pIEditElement) );
                IFC( GetViewServices()->SetElementBlockDirection(pIEditElement, eHTMLDir) );
            }
        }
    }

Cleanup:
    ReleaseInterface( pIEditElement );
    return hr;
}



//+====================================================================================
//
// Method: HandleEnter
//
// Synopsis: Handle the enter key
//
//------------------------------------------------------------------------------------

HRESULT
CCaretTracker::HandleEnter( IHTMLCaret * pCaret, BOOL fShift, BOOL fCtrl )
{
    HRESULT             hr = S_FALSE;
    IMarkupServices *   pMarkupServices = _pManager->GetMarkupServices();
    IHTMLViewServices * pViewServices = _pManager->GetViewServices();   
    SP_IMarkupPointer   spPos;
    SP_IMarkupPointer   spNewPos;
    SP_IHTMLElement     spFlowElement;
    BOOL                bContainer, bHTML, bMultiLine = FALSE;
    SP_IHTMLElement     spElement;
    CSpringLoader     * pSpringLoader = NULL;

    //
    // Position a worker pointer at the current caret location. Then
    // check to see if we are in a multi-line flow element.
    //

    IFC( pMarkupServices->CreateMarkupPointer( &spPos ));
    IFC( pCaret->MovePointerToCaret( spPos ));
    IFC( pViewServices->GetFlowElement( spPos, &spFlowElement ));
    if ( ! spFlowElement )
    {
        hr = E_FAIL;
        goto Cleanup;
    }

#ifndef NO_IME
    if (_pManager->IsIMEComposition())
    {
        TRACKER_NOTIFY eTrackerNotify = TN_NONE;

        _pManager->TerminateIMEComposition(TERMINATE_NORMAL, &eTrackerNotify);
        AssertSz(eTrackerNotify == TN_NONE || eTrackerNotify == TN_END_TRACKER_CREATE_CARET, "Dropped notify");
    }
#endif // NO_IME

    IFC( pViewServices->IsMultiLineFlowElement( spFlowElement, &bMultiLine ));
    if (!bMultiLine)
    {
        hr = S_FALSE;
        goto Cleanup;
    }

    //
    // Check to see if we can contain HTML. If not, just insert a \r.
    //

    IFC( pViewServices->IsContainerElement( spFlowElement, &bContainer, &bHTML ));

    if (!bHTML)
    {
        IFC( spPos->SetGravity( POINTER_GRAVITY_Right ));
        IFC( pCaret->InsertText(_T("\r"),1));

        spNewPos = spPos;
    }
    else
    {
        if( fShift )
        {
            IFC( pMarkupServices->CreateElement( TAGID_BR, NULL, &spElement ));
            IFC( InsertElement(pMarkupServices, spElement, spPos, spPos ));
            IFC( pMarkupServices->CreateMarkupPointer( &spNewPos ));
            IFC( spNewPos->MoveAdjacentToElement( spElement, ELEM_ADJ_AfterEnd ));
        }
        else if( ShouldEnterExitList( spPos, &spElement ))
        {
            CBlockPointer   bpListItem(_pManager->GetEditor());
            CBlockPointer   bpParent(_pManager->GetEditor());
            
            IFC( bpListItem.MoveTo(spElement) );
            Assert(bpListItem.GetType() == NT_ListItem);

            IFC( bpParent.MoveTo(&bpListItem) );
            IFC( bpParent.MoveToParent() );

            if (hr == S_FALSE
                || bpParent.GetType() == NT_Container
                || bpParent.GetType() == NT_BlockLayout
                || bpParent.GetType() == NT_FlowLayout)
            {
                ELEMENT_TAG_ID tagId = TAGID_DIV;

                // Can't float through a container or a block layout
                // so just morph to the default block element
                IFC( CGetBlockFmtCommand::GetDefaultBlockTag(GetMarkupServices(), &tagId) );
                IFC( bpListItem.Morph(&bpListItem, tagId) );
            }
            else
            {
                IFC( bpListItem.FloatUp(&bpListItem, TRUE) );
            }

            if (bpListItem.GetType() == NT_Block)
            {
                IFC( bpListItem.FlattenNode() );
            }
            goto Cleanup;            
        }
        else
        {
            pSpringLoader = GetSpringLoader();
            IFC( _pManager->GetEditor()->HandleEnter( spPos, &spNewPos, pSpringLoader ));
        }    
    }

    //
    // Move the caret to the new location.
    // We know we are going right and are visible, if ambiguious, 
    // we are at the beginning of the inserted line
    //

    _fNotAtBOL = FALSE;
    _fAtLogicalBOL = TRUE;
    
    IFC( ConstrainPointer( spNewPos ));
    IFC( pCaret->MoveCaretToPointer( spNewPos, _fNotAtBOL, TRUE, CARET_DIRECTION_INDETERMINATE ));

    if (pSpringLoader && spNewPos)
        pSpringLoader->Reposition(spNewPos);

Cleanup:

    RRETURN1(hr, S_FALSE);
}


//+====================================================================================
//
// Method: HandleEnter
//
// Synopsis: Handle the enter key
//
//------------------------------------------------------------------------------------
BOOL    
CCaretTracker::ShouldEnterExitList(IMarkupPointer *pPosition, IHTMLElement **ppElement)
{
    HRESULT                 hr;
    SP_IHTMLElement         spElement;
    SP_IHTMLElement         spListItem;
    SP_IMarkupPointer       spStart;
    SP_IMarkupPointer       spEnd;
    CBlockPointer           bpChild(_pManager->GetEditor());

    *ppElement = NULL;
    
    //
    // Are we in a list item scope?
    //

    IFC( GetViewServices()->CurrentScopeOrSlave(pPosition, &spElement) );
    if (spElement == NULL)
        goto Cleanup;

    IFC( EdUtil::FindListItem(GetMarkupServices(), spElement, &spListItem) );
    if (spListItem == NULL)
        goto Cleanup;

    //
    // Is the list item empty?  For the list item to be empty, the block tree must have
    // branching factor exactly equal to one from the list item to an empty text node below.
    //

    IFC( bpChild.MoveTo(spElement) );

    for (;;)
    {        
        IFC( bpChild.MoveToFirstChild() );

        IFC( bpChild.MoveToSibling(RIGHT) );
        if (hr != S_FALSE)
            return FALSE; // has siblings so not empty
            
        if (bpChild.GetType() == NT_Text)
            break; // we have our text node
        
        if (bpChild.GetType() != NT_Block)
            return FALSE; // if not a block, then not empty            
    }

    //
    // Is the scope empty?
    //
    
    IFC( GetMarkupServices()->CreateMarkupPointer(&spEnd) );
    IFC( bpChild.MovePointerTo(spEnd, ELEM_ADJ_BeforeEnd) );
    
    IFC( GetMarkupServices()->CreateMarkupPointer(&spStart) );
    IFC( bpChild.MovePointerTo(spStart, ELEM_ADJ_AfterBegin) );

    if (!DoesSegmentContainText( GetMarkupServices(), GetViewServices(), spStart, spEnd))
    {
        *ppElement = spListItem;
        (*ppElement)->AddRef();

        return TRUE;
    }

Cleanup:
    return FALSE;
}

//+====================================================================================
//
// Method: HandleInputLangChange
//
// Synopsis: Update the screen caret to reflect change in keyboard language
//
//------------------------------------------------------------------------------------

HRESULT
CCaretTracker::HandleInputLangChange()
{
    HRESULT hr = S_FALSE;
    IHTMLViewServices * pvs = _pManager->GetViewServices();
    IMarkupServices*    pms = _pManager->GetMarkupServices();
    SP_IHTMLCaret       spCaret;
    SP_IMarkupPointer   spPointer;
    

    if ( IsContextEditable() )
    {
        IFC( pms->CreateMarkupPointer( &spPointer  ));                        
        IFC( pvs->GetCaret( &spCaret ));
        spCaret->UpdateCaret();
        hr = S_OK;
    }

    // Something seems to be missing here...
Cleanup:
    RRETURN1( hr, S_FALSE );
}


//+====================================================================================
//
//  Method: MoveCaret
//
//  Synopsis: Moves the pointer like it were a caret - by the amount given for Caret 
//  Navigation
//
//  Parameters: inMove      -   The direction to move the caret
//              pPointer    -   A markup pointer pointing to the new location of the
//                              caret after moving
//              fMoveCaret  -   Actually move the caret or just compute the new
//                              location of the caret
//
//------------------------------------------------------------------------------------

HRESULT
CEditTracker::MovePointer(
    CARET_MOVE_UNIT     inMove, 
    IMarkupPointer      *pPointer,
    LONG                *plXPosForMove,
    BOOL                *pfNotAtBOL,
    BOOL                *pfAtLogicalBOL,
    Direction           *peMvDir)
{
    IHTMLViewServices *pvs = _pManager->GetViewServices();

    SP_IHTMLElement spTargetElement;
    SP_IHTMLElement spElement;

    HRESULT hr = S_OK;
    Direction eMvDir = LEFT;
    Direction eBlockAdj = SAME;

    if (peMvDir)
        *peMvDir = eMvDir; // init for error case

    POINT pt;
    pt.x = 0;
    pt.y = 0;

    IFC( _pManager->GetEditableElement( &spTargetElement ));

    if( inMove != CARET_MOVE_UP && inMove != CARET_MOVE_DOWN )
        *plXPosForMove = CARET_XPOS_UNDEFINED;

    // Initialize

    eMvDir = LEFT;
    
    switch( inMove )
    {
        case CARET_MOVE_RIGHT:
            eMvDir = RIGHT;
            // fall through
            
        case CARET_MOVE_LEFT:            
        {   
            CEditPointer tPointer( _pManager->GetEditor() );
            IFC( tPointer->MoveToPointer( pPointer ));
            IFC( tPointer.MoveCharacter( eMvDir , pfNotAtBOL, pfAtLogicalBOL ));
            IFC( pPointer->MoveToPointer( tPointer ));

            break;
        }
        
        case CARET_MOVE_WORDRIGHT:
            eMvDir = RIGHT;
            // fall through

        case CARET_MOVE_WORDLEFT:
        {
            CEditPointer tPointer( _pManager->GetEditor() );
            IFC( tPointer->MoveToPointer( pPointer ));
            IFC( tPointer.MoveWord( eMvDir , pfNotAtBOL, pfAtLogicalBOL ));
            IFC( pPointer->MoveToPointer( tPointer ));
            
            break;
        }

        case CARET_MOVE_UP:            
            if( CARET_XPOS_UNDEFINED == *plXPosForMove )
            {
                IFC( GetLocation( &pt ));
                *plXPosForMove = pt.x;
            }
            
            eMvDir = LEFT;
            IFC( pvs->MoveMarkupPointer( pPointer, LAYOUT_MOVE_UNIT_PreviousLine, *plXPosForMove, pfNotAtBOL, pfAtLogicalBOL ));
            break;

        case CARET_MOVE_DOWN:
            if( CARET_XPOS_UNDEFINED == *plXPosForMove )
            {
                IFC( GetLocation( &pt ));
                 *plXPosForMove = pt.x;
            }
            eMvDir = RIGHT;
            IFC( pvs->MoveMarkupPointer( pPointer, LAYOUT_MOVE_UNIT_NextLine, *plXPosForMove, pfNotAtBOL, pfAtLogicalBOL ));            
            break;

        case CARET_MOVE_LINESTART:            
            IFC( GetLocation( &pt ));
            IFC( pvs->MoveMarkupPointer( pPointer, LAYOUT_MOVE_UNIT_CurrentLineStart, pt.x, pfNotAtBOL, pfAtLogicalBOL ));
            eMvDir = LEFT;            
            break;

        case CARET_MOVE_LINEEND:
            IFC( GetLocation( &pt ));
            IFC( pvs->MoveMarkupPointer( pPointer, LAYOUT_MOVE_UNIT_CurrentLineEnd, pt.x, pfNotAtBOL, pfAtLogicalBOL ));
            eMvDir = RIGHT;
            break;

        case CARET_MOVE_VIEWEND:
            eMvDir = RIGHT;
            // fall through
            
        case CARET_MOVE_VIEWSTART:
        {
            // Get the bounding rect of the edit context, offset by several pixels, and do
            // a global hit test.

            POINT ptGlobal;
            RECT rcGlobalClient;
            IFC( pvs->GetClientRect( spTargetElement, COORD_SYSTEM_GLOBAL, & rcGlobalClient ));

            if( inMove == CARET_MOVE_VIEWSTART )
            {
                ptGlobal.x = rcGlobalClient.left+1;
                ptGlobal.y = rcGlobalClient.top+10;
            }
            else
            {
                ptGlobal.x = rcGlobalClient.right-1;
                ptGlobal.y = rcGlobalClient.bottom-10;
            }

            IFC( pvs->MoveMarkupPointerToPointEx( ptGlobal, pPointer, TRUE, pfNotAtBOL, pfAtLogicalBOL, NULL, FALSE ));
            break;
        }

        case CARET_MOVE_PAGEUP:
        case CARET_MOVE_PAGEDOWN:
        {
            SP_IHTMLElement spScroller;
            POINT ptCaretLocation;
            POINT ptScrollDelta;
            RECT windowRect;
            HWND hwnd;
            POINT ptWindow;

            ptScrollDelta.x = 0;
            ptScrollDelta.y = 0;
            ptCaretLocation.x = 0;
            ptCaretLocation.y = 0;

            IFC( pvs->GetScrollingElement( pPointer, spTargetElement, &spScroller ));
            if( spScroller == NULL )
            {
                // can't scroll this element, just go to the start or end of it
                spScroller = spTargetElement;
                goto CantScroll;    // we are done
            }

            // Get me the caret's position in global coord's
            IFC( GetLocation( &ptCaretLocation )); 
            ptWindow = ptCaretLocation;
            
            // Constrain this point to the visible window
            IFC( GetViewServices()->GetViewHWND( &hwnd ));            
            ::GetWindowRect( hwnd, & windowRect );
            ::ClientToScreen( hwnd, &ptWindow );

            if( ptWindow.x < windowRect.left )
                ptCaretLocation.x = ptCaretLocation.x + ( windowRect.left - ptWindow.x ) + 5;

            if( ptWindow.x > windowRect.right )
                ptCaretLocation.x = ptCaretLocation.x - ( ptWindow.x - windowRect.right ) - 5;

            if( ptWindow.y < windowRect.top )
                ptCaretLocation.y = ptCaretLocation.y + ( windowRect.top - ptWindow.y ) + 5;

            if( ptWindow.y > windowRect.bottom )
                ptCaretLocation.y = ptCaretLocation.y - ( ptWindow.y - windowRect.bottom ) - 5;

            // Scroll
            hr = THR( pvs->ScrollElement( spScroller,
                                     875 * (( inMove == CARET_MOVE_PAGEDOWN ) ? 1 : -1 ),
                                     &ptScrollDelta ));

CantScroll:
            // Did we scroll a visible amount? If not, just move to the top or bottom of the element
            if( abs( ptScrollDelta.x ) < 7 && abs( ptScrollDelta.y ) < 7 )
            {   
                // like other keynav - default to bol if unknown
                
                *pfNotAtBOL = FALSE;
                *pfAtLogicalBOL = TRUE;

                // we didn't scroll anywhere, go to the start or end of the current editable element
                if( inMove == CARET_MOVE_PAGEDOWN )
                {
                    IFC( pPointer->MoveAdjacentToElement( spScroller, ELEM_ADJ_BeforeEnd ));
                }
                else
                {
                    IFC( pPointer->MoveAdjacentToElement( spScroller, ELEM_ADJ_AfterBegin ));
                }
            }
            else
            {   
                // we scrolled. figure out what element we hit. if we hit an image or control that isn't
                // our edit context, we want to move adjacent to that element's start or end. if we 
                // didn't, we want to to a global hit test at our adjusted caret location to relocate
                // the pointer.
                
                SP_IHTMLElement spElement;
                BOOL fSameElement = FALSE;
                IObjectIdentity * pIdent;
                ELEMENT_TAG_ID eTagId;
                IFC( pvs->GetTopElement( ptCaretLocation, &spElement ));
                BOOL fShouldHitTest = TRUE;

                if( spElement )
                {
                    IFC( spElement->QueryInterface(IID_IObjectIdentity, (LPVOID *)&pIdent));      
                    fSameElement = ( pIdent->IsEqualObject ( spScroller ) == S_OK );
                    pIdent->Release();
                    IFC( GetMarkupServices()->GetElementTagId( spElement , &eTagId ));
                
                    if( ! fSameElement && ( IsIntrinsic( GetMarkupServices(), spElement ) || ( eTagId == TAGID_IMG )))
                    {
                        IFC( pPointer->MoveAdjacentToElement( spElement, inMove == CARET_MOVE_PAGEDOWN ? ELEM_ADJ_AfterEnd : ELEM_ADJ_BeforeBegin ));
                        fShouldHitTest = FALSE;
                    }
                }

                if( fShouldHitTest )
                {
                    IFC( pvs->MoveMarkupPointerToPointEx( ptCaretLocation, pPointer, TRUE, pfNotAtBOL, pfAtLogicalBOL, NULL, FALSE ));
                }
            }
            
            break;
        }
        
        case CARET_MOVE_HOME:
            *pfNotAtBOL = FALSE;
            *pfAtLogicalBOL = TRUE;
            IFC( pPointer->MoveAdjacentToElement( spTargetElement , ELEM_ADJ_AfterBegin ));
            eMvDir = LEFT;
            break;

        case CARET_MOVE_END:
            *pfNotAtBOL = TRUE;
            *pfAtLogicalBOL = FALSE;
            IFC( pPointer->MoveAdjacentToElement( spTargetElement , ELEM_ADJ_BeforeEnd ));
            eMvDir = RIGHT;
            break;

        case CARET_MOVE_BLOCKSTART:            
        {
            // First, move one char to the left
            IFC( MovePointer(   CARET_MOVE_LEFT, 
                                pPointer,
                                plXPosForMove,
                                pfNotAtBOL,
                                pfAtLogicalBOL,
                                peMvDir ));

            *pfNotAtBOL = FALSE;
            *pfAtLogicalBOL = TRUE;

            // now, find the block limit - if i'm at the block start, i'll stay there
            IFC( FindBlockLimit( GetMarkupServices(), GetViewServices(), LEFT, pPointer, NULL, NULL, FALSE, TRUE  ));
            
            eMvDir = LEFT;
            break;
        }
        
        case CARET_MOVE_BLOCKEND:
        {
            *pfNotAtBOL = FALSE;
            *pfAtLogicalBOL = TRUE;            
            CEditPointer epPos( _pManager->GetEditor() );
            epPos.SetBoundary( _pManager->GetStartEditContext(), _pManager->GetEndEditContext());
            
            IFC( epPos->MoveToPointer( pPointer ));

            // Move to the end of the current block
            IFC( FindBlockLimit(GetMarkupServices(), GetViewServices(), RIGHT, epPos, NULL, NULL, FALSE, TRUE));

            // Scan right until I enter the next block
            IFC( epPos.Scan( RIGHT, BREAK_CONDITION_EnterBlock ));
            IFC( pPointer->MoveToPointer( epPos ));
            
            eMvDir = RIGHT;
            break;
        }            
    }
    
    // Properly constrain the pointer
    {
        if( eBlockAdj == SAME )
        {
            eBlockAdj = *pfAtLogicalBOL ? RIGHT : LEFT;
        }

        //
        // Check to see if we have moved into a different site selectable object.
        // If this happens - we jump over the site
        //
        if ( GetEditor()->IsInDifferentEditableSite( pPointer ))
        {
            SP_IHTMLElement spFlow;
            IFC( GetViewServices()->GetFlowElement( pPointer, & spFlow));
            
            if (GetPointerDirection( inMove ) == RIGHT)
            {
                IFC( pPointer->MoveAdjacentToElement( spFlow, ELEM_ADJ_AfterEnd));
            }
            else
            {
                IFC( pPointer->MoveAdjacentToElement( spFlow, ELEM_ADJ_BeforeBegin));
            }
        }

        //
        // HACKHACK (johnbed)
        // Now the pointer is in the right place. Problem: fNotAtBOL may be wrong if the line is empty.
        // Easy check: if fNotAtBOL == TRUE, make sure there isn't a block phrase to the left of us. <BR>
        // and \r are never swallowed, and being to the right of one of them with fNotAtBOL==TRUE will 
        // make us render in a bad place.
        // 
        // The right fix is to create a line-aware CEditPointer subclass that encapsulates moving in
        // a line aware way and to use these pointers instead of raw markup pointers.
        //

        IFC( ConstrainPointer(pPointer, 
                               ( GetPointerDirection( inMove ) == RIGHT ))); // don't rely on AdjPointer for this.
        
        IFC( AdjustPointerForInsert( pPointer , *pfNotAtBOL, eBlockAdj, *pfAtLogicalBOL ? RIGHT : LEFT, 
                                     inMove != CARET_MOVE_LEFT && inMove != CARET_MOVE_RIGHT) );

        if( *pfNotAtBOL )
        {
            CEditPointer epScan( GetEditor() );
            DWORD dwSearch = BREAK_CONDITION_OMIT_PHRASE - BREAK_CONDITION_Anchor; // anchors are just phrase elements to me here
            DWORD dwFound = BREAK_CONDITION_None;
            IFC( epScan->MoveToPointer( pPointer ));
            IFC( epScan.Scan( LEFT, dwSearch, &dwFound ));

            if( CheckFlag( dwFound, BREAK_CONDITION_NoScopeBlock ))
            {
                *pfNotAtBOL = FALSE;
            }
        }
    }
            
Cleanup:
    if (peMvDir)
        *peMvDir = eMvDir; 

    RRETURN( hr );
}

HRESULT
CCaretTracker::MoveCaret(
    CARET_MOVE_UNIT     inMove, 
    IMarkupPointer*     pPointer,
    BOOL                fMoveCaret)
{
    HRESULT             hr;    
    SP_IHTMLCaret       spCaret;
    IHTMLViewServices   *pvs = GetViewServices();
    Direction           eMvDir;
    CSpringLoader       * psl = NULL;

    IFC( pvs->GetCaret( &spCaret ));
    IFC( spCaret->MovePointerToCaret( pPointer ));

    if( _pManager->HaveTypedSinceLastUrlDetect() )
    {
        IGNORE_HR( UrlAutodetectCurrentWord( NULL ) );
        _pManager->SetHaveTypedSinceLastUrlDetect( FALSE );
    }

    IFC( MovePointer(inMove, pPointer, &_lScreenXPosForVertMove, &_fNotAtBOL, &_fAtLogicalBOL, &eMvDir));

    if ( fMoveCaret )
    {
        BOOL fShouldIScroll = ! ( inMove == CARET_MOVE_PAGEUP || inMove == CARET_MOVE_PAGEDOWN );
        //Assert( _pManager->IsInEditContext( pPointer ));
        if( ! _pManager->IsInEditContext( pPointer ))
            goto Cleanup;

#ifndef NO_IME
        if (_pManager->IsIMEComposition())
        {
            TRACKER_NOTIFY eTrackerNotify = TN_NONE;

            _pManager->TerminateIMEComposition(TERMINATE_NORMAL, &eTrackerNotify);
            AssertSz(eTrackerNotify == TN_NONE || eTrackerNotify == TN_END_TRACKER_CREATE_CARET, "Dropped notify");
        }
#endif // NO_IME

        IFC( spCaret->MoveCaretToPointer( pPointer, _fNotAtBOL, fShouldIScroll, CARET_DIRECTION_INDETERMINATE ));
        
        // Reset the spring loader.
        psl = GetSpringLoader();
        if (psl)
        {
            IGNORE_HR(psl->SpringLoadComposeSettings(pPointer, TRUE));
        }
    }
  

Cleanup:
    RRETURN( hr );
}

//+====================================================================================
//
// Method: SetCaretVisible.
//
// Synopsis: Set's the Caret's visiblity.
//
//------------------------------------------------------------------------------------

HRESULT
CCaretTracker::SetCaretVisible( IHTMLDocument2* pIDoc, BOOL fVisible )
{
    HRESULT hr = S_OK ;
    SP_IHTMLCaret spc;
    IHTMLViewServices * pvs = NULL;
    
    IFC( pIDoc->QueryInterface( IID_IHTMLViewServices, (void **) & pvs ));
    IFC( pvs->GetCaret( &spc ));

    if( fVisible )
        hr = spc->Show( FALSE );
    else
        hr = spc->Hide();

Cleanup:
    ReleaseInterface( pvs );
    RRETURN( hr );
}




//+====================================================================================
//
// Method:GetCaretPointer
//
// Synopsis: Utility Routine to get the MarkupPointer at the Caret
//
//------------------------------------------------------------------------------------

HRESULT
CCaretTracker::GetCaretPointer( IMarkupPointer ** ppMarkup )
{
    HRESULT hr = S_OK;

    IHTMLViewServices *pvs = _pManager->GetViewServices();
    IMarkupServices* pms = _pManager->GetMarkupServices();

    SP_IHTMLCaret  spCaret;
    
    IFC( pms->CreateMarkupPointer( ppMarkup  ));
    IFC( pvs->GetCaret( &spCaret ));
    IFC( spCaret->MovePointerToCaret( *ppMarkup ));

Cleanup:
    RRETURN( hr );
}


HRESULT
CEditTracker::AdjustPointerForInsert( 
    IMarkupPointer * pWhereIThinkIAm, 
    BOOL             fNotAtBOL,
    Direction        inBlockcDir, 
    Direction        inTextDir, 
    DWORD            dwOptions /* = NULL */ )
{
    HRESULT hr = S_OK;
    SP_IMarkupPointer spLeftEdge;
    SP_IMarkupPointer spRightEdge;    
    IFC( _pManager->GetMarkupServices()->CreateMarkupPointer( &spLeftEdge ));
    IFC( _pManager->GetMarkupServices()->CreateMarkupPointer( &spRightEdge ));
    IFC( _pManager->MovePointersToContext( spLeftEdge, spRightEdge ));
    IFC( _pManager->GetEditor()->AdjustPointer( pWhereIThinkIAm, fNotAtBOL, inBlockcDir, inTextDir, spLeftEdge, spRightEdge, dwOptions ));

Cleanup:
    RRETURN( hr );
}



HRESULT
CCaretTracker::Notify(
        TRACKER_NOTIFY inNotify,
        SelectionMessage *pMessage,
        DWORD* pdwFollowUpAction,
        TRACKER_NOTIFY * peTrackerNotify  )
{
    HRESULT hr = S_OK;

    switch ( inNotify )
    {
        case TN_END_TRACKER:
        case TN_END_TRACKER_NO_CLEAR:
            // If we're ending the tracker, then autodetect, but only
            // if the reason for ending the tracker was NOT an IME composition.
            if (_pManager->HaveTypedSinceLastUrlDetect() && 
                ( !pMessage || 
                   pMessage->message < WM_IME_STARTCOMPOSITION ||
                   pMessage->message > WM_IME_COMPOSITION ) )
            {
                UrlAutodetectCurrentWord(NULL);
                _pManager->SetHaveTypedSinceLastUrlDetect( FALSE );
            }
            
            if ( ( pMessage ) && ( _fValidPosition ) )
                PositionCaretFromMessage( pMessage );

            TraceTag(( tagSelectionTrackerState, "\n---End Caret Tracker--- "));

            break;
    }
    RRETURN ( hr );
}

//+====================================================================================
//
// Method: ShouldBeginSelection
//
// Synopsis: We don't want to start selection in Anchors, Images etc.
//
//------------------------------------------------------------------------------------
BOOL
CCaretTracker::ShouldStartTracker(
                CSelectionManager* pManager,
                SelectionMessage* pMessage,
                ELEMENT_TAG_ID eTag,
                IHTMLElement* pIElement,
                IHTMLElement** ppIEditThisElement /*= NULL */
 )
{
    return TRUE;
}


VOID 
CCaretTracker::SetCaretShouldBeVisible( BOOL fVisible )
{
    _fCaretShouldBeVisible = fVisible;
    SetCaretVisible( _pManager->GetDoc(), _fCaretShouldBeVisible );
}



BOOL
CCaretTracker::ShouldCaretBeVisible()
{
    IHTMLElement* pIElement = NULL;
    IHTMLInputElement* pIInputElement = NULL;
    HRESULT hr = S_OK;
    BSTR bstrType = NULL;
    BOOL fShouldBeVisible = FALSE;

    ELEMENT_TAG_ID eTag = _pManager->GetEditableTagId();

    if ( _pManager->IsContextEditable() )
    {
        switch( eTag )
        {
            case TAGID_IMG:
            case TAGID_OBJECT:
            case TAGID_APPLET:
            case TAGID_SELECT:
                fShouldBeVisible = FALSE;
                break;

            case TAGID_INPUT:
            {
                //
                // for input's of type= image, or type=button - we don't want to make the caret visible.
                //
               IFC( GetViewServices()->GetElementForSelection( GetEditableElement(), &pIElement));
               
                if ( S_OK == THR( pIElement->QueryInterface ( IID_IHTMLInputElement, 
                                                             ( void** ) & pIInputElement ))
                    && S_OK == THR(pIInputElement->get_type(&bstrType)))
                {                    
                    if (!_tcsicmp( bstrType, TEXT("image")) || !_tcsicmp(bstrType, TEXT("radio"))
                        || !_tcsicmp(bstrType, TEXT("checkbox")))
                    {
                        fShouldBeVisible = FALSE;
                    }                
                    else
                        fShouldBeVisible = TRUE;
                }
                else
                    fShouldBeVisible = TRUE;
            }
            break;

            default:
                fShouldBeVisible = TRUE;
        }
    }
    //
    // See if we can have a visible caret - via the FireOnBeforeEditFocus event.
    //
    if ( fShouldBeVisible )
    {
        fShouldBeVisible = _pManager->CanHaveEditFocus();
    }
    
Cleanup:
    ReleaseInterface( pIInputElement);
    ReleaseInterface( pIElement );
    SysFreeString(bstrType);

    return ( fShouldBeVisible);
}

//+====================================================================================
//
// Method: DontBackspace
//
// Synopsis: Dont Backspace when you're at the start of these
//
// Typical example is - don't backspace from inside an input.
//
//------------------------------------------------------------------------------------


BOOL
DontBackspace( ELEMENT_TAG_ID eTagId )
{
    switch( eTagId )
    {
        case TAGID_INPUT:
        case TAGID_BODY:
        case TAGID_TD:
        case TAGID_TR:
        case TAGID_TABLE:

            return TRUE;

        default:
            return FALSE;

    }
}


//+====================================================================================
//
// Method: ClearSelection
//
// Synopsis: Clear the current ISelectionRenderingServices
//
//------------------------------------------------------------------------------------

HRESULT
CSelectTracker::ClearSelection()
{
    HRESULT hr = S_OK;
    IHTMLViewServices * pViewServices = NULL;
    ISelectionRenderingServices* pSelRenSvc = NULL;
    IHTMLDocument2* pIDoc = _pManager->GetDoc();

    hr = THR ( pIDoc->QueryInterface( IID_IHTMLViewServices, ( void**) & pViewServices ));
    if ( hr )
        goto Cleanup;
    hr = THR( pViewServices->GetCurrentSelectionRenderingServices( & pSelRenSvc));
    if ( hr )
        goto Cleanup;

    hr = pSelRenSvc->ClearSegments( TRUE );

    //
    // BUGBUG - marka - is there an OM thing to fire here ?
    //


Cleanup:
    ReleaseInterface( pViewServices );
    ReleaseInterface( pSelRenSvc );

    RRETURN ( hr );
}

BOOL
CSelectTracker::IsPointerInSelection( IMarkupPointer* pPointer,  POINT * pptGlobal, IHTMLElement* pIElementOver )
{
    HRESULT hr = S_OK;
    BOOL fWithin = FALSE;
    int iWherePointer = SAME;
    IMarkupPointer* pTrueStart = NULL;
    IMarkupPointer* pTrueEnd = NULL;
    IMarkupContainer* pIContainer1 = NULL;
    IMarkupContainer* pIContainer2 = NULL;
    

    IFC( OldCompare( _pStartPointer, _pEndPointer, & iWherePointer ));
    if ( iWherePointer != LEFT )
    {
        ReplaceInterface( & pTrueStart, _pStartPointer );
        ReplaceInterface( & pTrueEnd, _pEndPointer );
    }
    else
    {
        ReplaceInterface( & pTrueStart, _pEndPointer );
        ReplaceInterface( & pTrueEnd, _pStartPointer );
    }

    //
    // If the pointers aren't in equivalent containers, adjust them so they are.
    //
    IFC( pPointer->GetContainer( & pIContainer1 ));
    IFC( _pStartPointer->GetContainer( & pIContainer2));
    if (! GetEditor()->EqualContainers( pIContainer1 , pIContainer2 ))
    {
        IFC( GetEditor()->MovePointersToEqualContainers( pPointer, _pStartPointer ));
    }
    
    IFC( OldCompare( pTrueStart, pPointer, & iWherePointer ));

    if ( iWherePointer != LEFT )
    {
        IFC( OldCompare( pTrueEnd, pPointer, & iWherePointer));
        
        fWithin = ( iWherePointer != RIGHT ) ;
    }
    
Cleanup:
    ReleaseInterface( pTrueStart );
    ReleaseInterface( pTrueEnd );
    ReleaseInterface( pIContainer1 );
    ReleaseInterface( pIContainer2 );
    
    return ( fWithin );
}


BOOL
CControlTracker::IsPointerInSelection( IMarkupPointer* pPointer,  POINT * pptGlobal, IHTMLElement* pIElementOver )
{
    //
    // We now do this work - by seeing if the given point is in the adorner 
    //  OR it's the same element we're over ( to handle the glyph case)
    // We do this - as this routine is called to handle mouse overs. At the end of the document
    // hit testing will put the pointers at the edge of the control - making us think the mouse is 
    // inside the site selected object when it really isn't.
    // 
    
    BOOL fWithin =  _pGrabAdorner->IsInAdorner( *pptGlobal ) || 
                           IsSameElementAsControl( pIElementOver );
                               
#if 0
    int iWherePointer = SAME;
    IMarkupPointer* pTrueStart = NULL;
    IMarkupPointer* pTrueEnd = NULL;
    IHTMLElement* pISelectableElement = NULL;
    IHTMLElement* pICurElement = NULL;
    
    IFC( GetMarkupServices()->CreateMarkupPointer( & pTrueStart));
    IFC( GetMarkupServices()->CreateMarkupPointer( & pTrueEnd ));
    IFC( pTrueStart->MoveAdjacentToElement( GetControlElement(), ELEM_ADJ_BeforeBegin));
    IFC( pTrueEnd->MoveAdjacentToElement( GetControlElement(), ELEM_ADJ_AfterEnd ));
    
    hr = THR( OldCompare( pTrueStart, pPointer, & iWherePointer ));
    if ( hr == CTL_E_INCOMPATIBLEPOINTERS ) 
    {
        //
        // BUGBUG special case for different trees to make inputs work.
        //
        IFC( GetViewServices()->CurrentScopeOrSlave(pPointer, &pICurElement ));
        IFC( GetViewServices()->GetElementForSelection( pICurElement, & pISelectableElement ));
        IFC( pPointer->MoveAdjacentToElement( pISelectableElement, ELEM_ADJ_BeforeBegin ));
        IFC( OldCompare( pTrueStart, pPointer, & iWherePointer ));
    }

    if ( iWherePointer != LEFT )
    {
        IFC( OldCompare( pTrueEnd, pPointer, & iWherePointer));        
        fWithin = ( iWherePointer != RIGHT ) ;
    }
    ReleaseInterface( pICurElement);
    ReleaseInterface( pISelectableElement );
    ReleaseInterface( pTrueStart );
    ReleaseInterface( pTrueEnd );
#endif     
    return ( fWithin );
}
//+====================================================================================
//
// Method: CheckSelectionWasReallyMade
//
// Synopsis: Verify that a Selection Was really made, ie start and End aren't in same point
//
//------------------------------------------------------------------------------------


BOOL
CSelectTracker::CheckSelectionWasReallyMade()
{
    BOOL fEqual = TRUE;

    IGNORE_HR( _pStartPointer->IsEqualTo(_pEndPointer, &fEqual) );
             
    return !fEqual;
}

HRESULT
CControlTracker::Init2( 
                        CSelectionManager*      pManager,
                        SelectionMessage *      pMessage, 
                        DWORD*                  pdwFollowUpAction, 
                        TRACKER_NOTIFY *        peTrackerNotify,
                        DWORD                   dwTCFlags)
{
    HRESULT         hr              = S_OK;
    IHTMLElement *  pIElement       = NULL;
    ELEMENT_TAG_ID  eTag            = TAGID_NULL;
    CSpringLoader * psl             = GetSpringLoader();
    BOOL            fGoActive       = dwTCFlags & TRACKER_CREATE_GOACTIVE;
    BOOL            fActiveOnMove   = dwTCFlags & TRACKER_CREATE_ACTIVEONMOVE;
    
    Assert( pMessage );
    Assert( pMessage->elementCookie );
    Assert( ! pManager->HadGrabHandles() );
    Assert( pManager->IsParentEditable());

    Init();

    // Reset the spring loader
    if (psl)
        psl->Reset();

    _ulNextEvent = EdUtil::NextEventTime(g_iDragDelay);
    
    hr = THR ( GetElementFromMessage( _pManager, pMessage, & pIElement ));
    if ( hr )
        goto Cleanup;
        
    hr = THR( GetTagIdFromMessage( pMessage, & eTag, _pManager->GetDoc(), NULL, NULL  ));
    if ( hr )
        goto Cleanup;

    hr = THR( SetControlElement( eTag, pIElement ));
    if ( hr )
        goto Cleanup;


#if DBG == 1
    VerifyOkToStartControlTracker( pMessage );
#endif

        _startMouseX = pMessage->pt.x;
        _startMouseY = pMessage->pt.y;            
 

        hr = Select( FALSE );

        if ( peTrackerNotify )
            *peTrackerNotify = TN_HIDE_CARET; // Hide the Caret.

        _pManager->TrackerNotify( TN_KILL_ADORNER, NULL, NULL );
        
        if ( fActiveOnMove )
            *pdwFollowUpAction |= FOLLOW_UP_ACTION_DragElement; // we still use the hokey code here 
                                                                // as we only start dragging once the constructor has exited
        
        if ( ( fGoActive ) && ( ! fActiveOnMove ))
        {
            _pGrabAdorner->SetNotifyManagerOnPositionSet( TRUE );
            _fMouseUp = TRUE;
        }
        else
            _fMouseUp = FALSE;               

Cleanup:
    ReleaseInterface( pIElement );
    return hr;
}

//+====================================================================================
//
// Method: ShouldGoUIActiveOnFirstClick
//
// Synopsis: See if the element we clicked on should go UI Active Immediately. This happens
//          
//
//------------------------------------------------------------------------------------

BOOL
CControlTracker::ShouldGoUIActiveOnFirstClick(
                                                IHTMLElement* pIElement, 
                                                ELEMENT_TAG_ID eTag)
{
    ELEMENT_TAG_ID eControlTag = TAGID_NULL;

    IGNORE_HR( GetMarkupServices()->GetElementTagId( GetControlElement(), & eControlTag));
    if (( eControlTag == TAGID_TABLE ) && ( eTag != TAGID_TABLE ))
            return TRUE;

    return FALSE;
}

VOID
CControlTracker::BecomeActiveOnFirstMove(
                SelectionMessage* pMessage )
{
    BecomeActive( pMessage, FALSE );
    _fMouseUp = TRUE;
}
                

HRESULT
CControlTracker::Init2( 
                        CSelectionManager*      pManager,
                        IMarkupPointer*         pStart, 
                        IMarkupPointer*         pEnd, 
                        DWORD*                  pdwFollowUpAction, 
                        TRACKER_NOTIFY *        peTrackerNotify,
                        DWORD                   dwTCFlags,
                        CARET_MOVE_UNIT         inLastCaretMove )
{
    HRESULT         hr  = S_OK;
    CSpringLoader * psl = GetSpringLoader();
    
    Init();

    // Reset the spring loader
    if (psl)
        psl->Reset();

    _pManager->TrackerNotify( TN_KILL_ADORNER, NULL, NULL ); // Whack any UI Active border on the Manager
    
    IFC( Position( pStart, pEnd ));    
    IFC( Select( TRUE ) );
    
    if ( peTrackerNotify )
        *peTrackerNotify = TN_HIDE_CARET ; // Hide the Caret.

    _fMouseUp = TRUE;

Cleanup:    
    if ( FAILED( hr ))
    {
        *peTrackerNotify = TN_END_TRACKER ;
    }
    _startMouseX = 0; // Don't set to -1 - as we've been created via OM.
    _startMouseY = 0;
    //
    // BUGBUG - does it make sense to become active here ?
    // lets' assume not.
    //

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     SetControlElement
//
//  Synopsis: At the start of the tracker, do some work to find out the element we're
//            drawing grab handles around, and assign it to _pIControlElement.
//
//----------------------------------------------------------------------------

HRESULT
CControlTracker::SetControlElement( 
                                    ELEMENT_TAG_ID eTag, 
                                    IHTMLElement* pIElement)
{
    HRESULT hr = S_OK;

    Verify(IsElementSiteSelectable( _pManager,  eTag, pIElement, NULL , & _pIControlElement  ));

    RRETURN( hr );
}

//+====================================================================================
//
// Method: IsSameElementAsControl
//
// Synopsis: We said we should start a tracker. Make sure it's not the same elemnet
//           as the existing ControlElemnet
//
//------------------------------------------------------------------------------------


BOOL
CControlTracker::IsSameElementAsControl(         
        IHTMLElement* pIElement )
{
    BOOL fSame = FALSE;
    IObjectIdentity * pIdent;
    HRESULT hr = S_OK;

    IFC( _pIControlElement->QueryInterface(IID_IObjectIdentity, (LPVOID *)&pIdent));  
    
    fSame = ( pIdent->IsEqualObject ( pIElement ) == S_OK );
            
Cleanup:

    ReleaseInterface( pIdent );

    return fSame;
}

//+====================================================================================
//
// Method: IsSameElementAsControl
//
// Synopsis: Check to see if this message is under the same element as the control
//
//------------------------------------------------------------------------------------


BOOL
CControlTracker::IsSameElementAsControl( SelectionMessage* pMessage )
{
    IHTMLElement* pIElement = NULL;
    HRESULT hr;
    BOOL fSame = FALSE;
    
    IFC( GetElementFromMessage( _pManager, pMessage, & pIElement ));

    fSame = IsSameElementAsControl( pIElement );

Cleanup:
    ReleaseInterface( pIElement );

    return fSame;
}   

VOID
CControlTracker::Init()
{
    _eType = SELECTION_TYPE_Control;
    _fPassive = TRUE;
    _fPendingUIActive = FALSE;
    Assert( _pIControlElement == NULL );
    Assert( _pGrabAdorner == NULL );
}

CControlTracker::~CControlTracker()
{
   ReleaseInterface( _pIControlElement );
   BecomePassive();
   //
   // We don't need to Unselect anything here - if we're being deleted by a doc
   // shut down, the Selection Arrays should already be gone.
   //
   DestroyAdorner();
   
}

HRESULT
CControlTracker::CreateAdorner(BOOL fScrollIntoView)
{
    HRESULT hr = S_OK;

    Assert( ! _pGrabAdorner );
    BOOL fLocked = IsElementLocked();
    
    //
    // For Control Selection - it is possible to select something at browse time
    // but not show any grab handles.
    //
    if ( _pManager->IsParentEditable() )
    {
        _pGrabAdorner = new CGrabHandleAdorner( _pIControlElement , _pManager->GetDoc(), fLocked );
    }
    else
    {
        _pGrabAdorner = new CSelectedControlAdorner( _pIControlElement , _pManager->GetDoc(), fLocked );
    }
    if ( ! _pGrabAdorner )
        goto Error;

    _pGrabAdorner->SetManager( _pManager );
    _pGrabAdorner->AddRef();

    IFC( _pGrabAdorner->CreateAdorner() );   

    if ( fScrollIntoView )
    {
        IFC( _pGrabAdorner->ScrollIntoView());
    }        

Cleanup:    

    RRETURN ( hr );
Error:
    return E_OUTOFMEMORY;
}

VOID
CControlTracker::DestroyAdorner()
{
    if ( _pGrabAdorner )
    {
        _pGrabAdorner->DestroyAdorner();
        _pGrabAdorner->Release();
        _pGrabAdorner = NULL;
    }
}

//+====================================================================================
//
// Method: IsElementLocked
//
// Synopsis: Is the Control Element Locked ?
//
//------------------------------------------------------------------------------------

BOOL
CControlTracker::IsElementLocked()
{
    HRESULT hr = S_OK;
    BOOL fLocked = FALSE;
    
    hr = THR( _pManager->GetViewServices()->IsElementLocked( _pIControlElement , & fLocked ));

    return fLocked;
}

//+====================================================================================
//
// Method: BecomePassive
//
// Synopsis:We are in a 'passive' state - where we have control handles drawn around us
//          but are not actively tracking mouse moves for resizes.
//
//------------------------------------------------------------------------------------

VOID
CControlTracker::BecomePassive()
{
    _fPassive = TRUE;
}


//+====================================================================================
//
// Method: BecomeActive
//
// Synopsis: We are actively dragging a control handle, we are in an "Active State".
//
//------------------------------------------------------------------------------------


VOID
CControlTracker::BecomeActive(SelectionMessage* peMessage, BOOL fInMove )
{
    _fPassive = FALSE;
     HWND   myHwnd;
     
     _startMoveX = peMessage->pt.x;
     _startMoveY = peMessage->pt.y;
     
     IGNORE_HR( _pManager->GetEditor()->GetViewServices()->GetViewHWND( &myHwnd ));

     TakeCapture();
     _pGrabAdorner->BeginResize(peMessage->pt, myHwnd ) ;

#if 0     
    // Move Code commented out for IE 5.
    _fInMove = fInMove;

     if ( ! _fInMove )
     {
        _pGrabAdorner->BeginResize(peMessage->pt, myHwnd ) ;
     }
     else
     {
        if ( _fDragElementDifferent )
        {
            RECT dragRect;
            IGNORE_HR( GetViewServices()->GetElementDragBounds( _pIDragElement, & dragRect ));
            _pGrabAdorner->BeginMove(peMessage->pt, myHwnd, & dragRect ) ;
        }
        else
            _pGrabAdorner->BeginMove(peMessage->pt, myHwnd ) ;
     }
#endif 

}

//+====================================================================================
//
// Method:Select
//
// Synopsis: Our Element has become selected. Add it to the selection rendering service
//
//------------------------------------------------------------------------------------

HRESULT
CControlTracker::Select(BOOL fScrollIntoView )
{
    HRESULT hr = S_OK;
    IHTMLDocument2* pIDoc = _pManager->GetDoc();
    IHTMLViewServices * pViewServices = NULL;
    ISelectionRenderingServices* pSelRenSvc = NULL;

    int cSegments = 0;

    hr = THR( pIDoc->QueryInterface( IID_IHTMLViewServices, ( void**) & pViewServices ));
    if ( hr )
        goto Cleanup;

    hr = THR( pViewServices->GetCurrentSelectionRenderingServices( & pSelRenSvc ));
    if ( hr )
        goto Cleanup;

    hr = THR( pSelRenSvc->AddElementSegment( _pIControlElement, & cSegments ));
    if ( hr )
        goto Cleanup;

    Assert( cSegments == 0 ); // No MultipleSelection yet.

    hr = THR( CreateAdorner( fScrollIntoView ) ) ;

Cleanup:
    ReleaseInterface( pViewServices );
    ReleaseInterface( pSelRenSvc );


    RRETURN ( hr );
}

//+====================================================================================
//
// Method: Unselect
//
// Synopsis: Remove our element from being selected. ASSUME ONLY ONE ELEMENT
//
//------------------------------------------------------------------------------------


HRESULT
CControlTracker::UnSelect()
{
    HRESULT hr = S_OK;
    IHTMLDocument2* pIDoc = _pManager->GetDoc();
    IHTMLViewServices * pViewServices = NULL;
    ISelectionRenderingServices* pSelRenSvc = NULL;

    hr = THR( pIDoc->QueryInterface( IID_IHTMLViewServices, ( void**) & pViewServices ));
    if ( hr )
        goto Cleanup;

    hr = THR( pViewServices->GetCurrentSelectionRenderingServices( & pSelRenSvc ));
    if ( hr )
        goto Cleanup;

    hr = THR( pSelRenSvc->ClearElementSegments());

    DestroyAdorner();

Cleanup:
    ReleaseInterface( pViewServices );
    ReleaseInterface( pSelRenSvc );


    RRETURN ( hr );
}
//+====================================================================================
//
// Method: HandleMouseUp
//
// Synopsis: Handle Mouse Up message. Per VID we now do UI Activation here.
//
//------------------------------------------------------------------------------------

HRESULT
CControlTracker::HandleMouseUp(
        SelectionMessage *pMessage,
        DWORD* pdwFollowUpAction,
        TRACKER_NOTIFY * peTrackerNotify )
{
    HRESULT hr = S_FALSE;
    TRACKER_NOTIFY eNotify = TN_NONE;
    DWORD followUpAction = FOLLOW_UP_ACTION_None;    
    ELEMENT_TAG_ID eTag = TAGID_NULL;
    IMarkupPointer* pTemp = NULL;
    BOOL fNotAtBOL  = FALSE;
    BOOL fAtLogicalBOL = FALSE;
    BOOL fRightOfCp = FALSE;
    BOOL fValidTree = FALSE;
    
    Assert( _pGrabAdorner );

    _fMouseUp = TRUE;
            
    if ( IsActive() )
    {
        CommitResize( pMessage );
        BecomePassive();
        
        //
        // Commit move code would go here if we supported drag move
        //
    }
    else if ( pMessage->message == WM_LBUTTONUP || 
              pMessage->message == WM_LBUTTONDBLCLK )
    {
        if( _fPendingUIActive )
        {
            _fPendingUIActive = FALSE;
            IFC( GetMarkupServices()->GetElementTagId( GetControlElement() , & eTag ));

            if ( pMessage->message == WM_LBUTTONUP )
            {
                followUpAction |= FOLLOW_UP_ACTION_OnClick; // Set the OnClick handler.
            }
            else
            {
                //
                // We double clicked. The first click was the click that site selected us
                // so we never cached the mouse down. We fake everything by caching the mouse
                // down and then setting the next message
                //
                // POST IE5 we will bypass this hackery by making the control trakcer a true state
                // machine.
                //
                if ( eTag != TAGID_TABLE )
                {
                    pMessage->message = WM_LBUTTONDOWN;
                    _pManager->SetDrillIn( TRUE, pMessage);             
                    pMessage->message = WM_LBUTTONDBLCLK;
                }
            }
            if ( eTag != TAGID_TABLE )
            {
                _pManager->SetNextMessage( pMessage );
                IGNORE_HR( BecomeUIActive( pMessage ) );
            }                
            else
            {
                IFC( GetMarkupServices()->CreateMarkupPointer( & pTemp ));
                IFC( GetViewServices()->MoveMarkupPointerToMessage( pTemp ,
                                                 pMessage,
                                                 & fNotAtBOL ,
                                                 & fAtLogicalBOL,
                                                 & fRightOfCp ,
                                                 & fValidTree,
                                                 TRUE,
                                                 GetEditableElement(),
                                                 NULL ,
                                                 TRUE )); 
                _pManager->CopyTempMarkupPointers( pTemp , pTemp  );
                eNotify = TN_END_TRACKER_POS_CARET;
            }
            //
            // BUGBUG - from this point on - this tracker is dead. Become Current will kill it.
            //
            hr = S_FALSE;
         }
         else
         {
            //
            // This is for the first site-selecting click. We eat it.
            //
             hr = S_OK; // eat the event.
         }            
    }
    if ( peTrackerNotify )
        *peTrackerNotify = eNotify;
    if ( pdwFollowUpAction )
        *pdwFollowUpAction |= followUpAction;
Cleanup:
    ReleaseInterface( pTemp );
    RRETURN1 ( hr , S_FALSE );
}

//+====================================================================================
//
// Method: IsMessageOverControl
//
// Synopsis: Is the given message over the control ?
//
//------------------------------------------------------------------------------------


BOOL 
CControlTracker::IsMessageOverControl( SelectionMessage * pMessage )
{
    Assert( _pGrabAdorner && pMessage );
    return ( _pGrabAdorner->IsInAdorner( pMessage->pt ));
}

HRESULT
CControlTracker::HandleMouseDown(
        SelectionMessage *pMessage,
        DWORD* pdwFollowUpAction,
        TRACKER_NOTIFY * peTrackerNotify )
{
    HRESULT hr = S_FALSE;
    ELEMENT_TAG_ID eTag = TAGID_NULL;
    TRACKER_NOTIFY eNotify = TN_NONE;
    DWORD followUpAction = FOLLOW_UP_ACTION_None;
    BOOL fSameElement;



    if ( IsActive() && pMessage->message == WM_RBUTTONDOWN )
    {
        //
        // While resizing they click on R-Button. Stop resizing - become passive, and let trident do the rest.
        //
        CommitResize( pMessage );
        BecomePassive();

        hr = S_FALSE;
        goto Cleanup;
    }
    
    if ( _pGrabAdorner->IsEditable() )
    {  

        BOOL fLocked = IsElementLocked();
        
        if ( ! fLocked && ( _pGrabAdorner->IsInResizeHandle( pMessage->pt )))
        {
            BecomeActive(pMessage, FALSE );
        }
        else if ( ! fLocked && ( _pGrabAdorner->IsInMoveArea( pMessage->pt )))
        {
            if (  pMessage->message == WM_LBUTTONDOWN )  
            {   
                hr = S_OK;
            }
            else
            {
                //
                //  we want to return S_FALSE here to allow the bringing up of a CONTEXTMENU
                //  we will also allow the drag to be handled by a MouseMove
                //  

                hr = S_FALSE;              

            }                
            _fIgnoreTimeElapsed = TRUE;
            _fMouseUp = FALSE; // Get handled by Mouse Move Dragging
            _startMouseX = pMessage->pt.x;
            _startMouseY = pMessage->pt.y; 
        }
        else
        {
            fSameElement = _pGrabAdorner->IsInAdorner( pMessage->pt ) || 
                           IsSameElementAsControl( pMessage );

            if ( fSameElement )
            {
                Assert( _pGrabAdorner );

                if ( pMessage->message == WM_LBUTTONDOWN )
                {
                    //                        
                    // Here we will go UI Active.
                    // Only allow going UI Active on LBUTTONDOWN. 
                    //
                    IFC( GetMarkupServices()->GetElementTagId( GetControlElement() , & eTag ));
                    hr = S_FALSE; // hr is set to S_OK in the IFC macro
                    if ( ShouldClickInsideGoActive( eTag ) || eTag == TAGID_TABLE )
                    {
                        //
                        // For tables the editable element is getting focus. Otherwise the control
                        // is getting focus.
                        //
                        if ( FireOnBeforeEditFocus( eTag != TAGID_TABLE  ? 
                                                                _pIControlElement :
                                                                _pManager->GetEditableElement() ) )
                        {
                            if ( eTag != TAGID_TABLE )
                                _pManager->SetDrillIn( TRUE, pMessage); // tell the manager we're drilling in                    
                            //
                            // BUGBUG - from this point on - this tracker is dead. Become Current will kill it.
                            //                                 
                            _fPendingUIActive = TRUE;
                        }
                        else
                        {
                            _fMouseUp = FALSE;
                        }
                        hr = S_OK; // take the event - stop flow layout from taking capture etc.
                        _startMouseX = pMessage->pt.x;
                        _startMouseY = pMessage->pt.y; 
                    }
                    else
                    {
                        _fMouseUp = FALSE;
                        AssertSz( ! _fGotRButtonDown, "Setting mouse up when we got rbuttondown");                        
                        eNotify = TN_HIDE_CARET;
                    }
                }
                else
                {
                    //
                    // For RBUTTONDOWN - return S_FALSE - allowing CONTEXT_MENU to come up.
                    //
                    _fMouseUp = FALSE;
                    AssertSz( ! _fGotRButtonDown, "Setting mouse up when we got rbuttondown");                    
                    _fIgnoreTimeElapsed = TRUE; // to allow dragging on RBUTTON DOWN
                    _startMouseX = pMessage->pt.x;
                    _startMouseY = pMessage->pt.y;                      
                    hr = S_FALSE;
                }
            }
            else
                eNotify = TN_END_TRACKER;
        }
    }

    if ( peTrackerNotify )
        *peTrackerNotify = eNotify;
    if ( pdwFollowUpAction )
        *pdwFollowUpAction |= followUpAction;

Cleanup:
    RRETURN1 ( hr , S_FALSE );
}

              
//+====================================================================================
//
// Method: HandleKeyDown
//
// Synopsis: Trap ESC to go deactive
//
//------------------------------------------------------------------------------------

HRESULT
CControlTracker::HandleKeyDown(
                SelectionMessage *pMessage,
                DWORD* pdwFollowUpAction,
                TRACKER_NOTIFY * peTrackerNotify )
{
    HRESULT hr = S_FALSE;
    IMarkupPointer* pControlStart = NULL;
    IMarkupPointer* pControlEnd = NULL;

    if ( ! IsActive() )
    {
        switch( pMessage->wParam )
        {        

            case VK_ESCAPE:
            {
                //
                // We are Site Selected. Hitting escape transitions to a caret before the control
                //
                IFC( GetMarkupServices()->CreateMarkupPointer( & pControlStart ));
                IFC( GetMarkupServices()->CreateMarkupPointer( & pControlEnd ));
                IFC( pControlStart->MoveAdjacentToElement( _pIControlElement, ELEM_ADJ_BeforeBegin ));
                IFC( pControlEnd->MoveAdjacentToElement( _pIControlElement, ELEM_ADJ_BeforeBegin ));
                _pManager->CopyTempMarkupPointers( pControlStart, pControlEnd );
                *peTrackerNotify = TN_END_TRACKER_POS_CARET;

                hr = S_OK;
                
            }
            break;
        }
    }
    
Cleanup:    
    ReleaseInterface( pControlStart );
    ReleaseInterface( pControlEnd );
    RRETURN1 ( hr, S_FALSE );
}

//+====================================================================================
//
// Method: HandleChar
//
// Synopsis: Trap Enter and ESC to go active/ deactive
//
//------------------------------------------------------------------------------------

HRESULT
CControlTracker::HandleChar(
                SelectionMessage *pMessage,
                DWORD* pdwFollowUpAction,
                TRACKER_NOTIFY * peTrackerNotify )
{
    HRESULT hr = S_FALSE;
    
    switch( pMessage->wParam )
    {        

        case VK_RETURN:
        {
            if ( ! IsActive())
            {
                ELEMENT_TAG_ID eTag = TAGID_NULL;
                IFC( GetMarkupServices()->GetElementTagId( GetControlElement(), & eTag ));
                if ( ShouldClickInsideGoActive( eTag )  )
                {
                    if ( FireOnBeforeEditFocus( _pIControlElement ) )
                    {
                        _pManager->SetDrillIn( TRUE , NULL );
                        IGNORE_HR( BecomeUIActive( pMessage ) ) ;
                    }
                }                    
                hr = S_OK;
            }
        }
        break;
    }

Cleanup:    
    RRETURN1 ( hr, S_FALSE );
}

HRESULT
CControlTracker::BecomeUIActive(  SelectionMessage *pMessage )
{
    HRESULT hr = S_OK;
    ELEMENT_TAG_ID eTag = TAGID_NULL;
    BOOL fOldActiveControl = _fActiveControl;

    IFC( GetMarkupServices()->GetElementTagId( GetControlElement(), & eTag ));
    _fActiveControl = ( eTag == TAGID_OBJECT );

    if (S_OK != GetViewServices()->BecomeCurrent( GetControlElement(), pMessage ))
    {
        _fActiveControl = fOldActiveControl;
    }
    


Cleanup:    
    RRETURN ( hr );
}

HRESULT
CControlTracker::HandleMessage(
    SelectionMessage *pMessage,
    DWORD* pdwFollowUpAction,
    TRACKER_NOTIFY * peTrackerNotify )
{
    HRESULT hr = S_FALSE;
    Assert( pMessage );
    TRACKER_NOTIFY eNotify = TN_NONE;
    DWORD followUpAction = FOLLOW_UP_ACTION_None;
    
    if ( _fActiveControl )
        goto Cleanup; // Bail!!
        
    switch( pMessage->message )
    {
        case WM_CHAR:
        {
            hr = HandleChar( pMessage, & followUpAction, & eNotify );
        }
        break;

        case WM_KEYDOWN:
        {
            hr = HandleKeyDown( pMessage, & followUpAction, & eNotify );
        }
        break;
        
        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
        {
            hr = HandleMouseDown( pMessage, & followUpAction, & eNotify  );
        }
        break;

        case WM_MOUSEMOVE:
        {     
            if ( IsActive() )
                DoResizeDrag( pMessage, & followUpAction );
            else
            {
                if ( !_fMouseUp || _fPendingUIActive ) // If Mouse Has been released after first click do this.
                {
                    Assert( _startMouseX != -1 && _startMouseY != - 1);
                    
                    if ( !IsValidMove(pMessage) ||
                        ( !_fIgnoreTimeElapsed && !EdUtil::IsTimePassed(_ulNextEvent) ) )
                    {
                        break;
                    }
                    _ulNextEvent = 0;
                    _fIgnoreTimeElapsed = FALSE;
                    //
                    // Do the Element Drag.
                    //
                    if ( _fPendingUIActive )
                    {
                        _fPendingUIActive = FALSE;
                        _pManager->SetDrillIn( FALSE, NULL );
                    }
                    DoDrag( pMessage ); 
                    hr = S_OK;
                }
            }
        }
        break;

        case WM_LBUTTONDBLCLK:
        {
            if ( ! GetControlElement() )
            {
                hr = S_FALSE;
                break;
            }                
            //
            // BUGBUG - instead of doing this here as well as HandleMouseDown - make a separate
            // method for this check
            //
            ELEMENT_TAG_ID eTag = TAGID_NULL;
            IFC( GetMarkupServices()->GetElementTagId( GetControlElement() , & eTag ));            
            if ( !_pGrabAdorner->IsInResizeHandle( pMessage->pt ) && 
                 !_pGrabAdorner->IsInMoveArea( pMessage->pt ) && 
                 ShouldClickInsideGoActive( eTag ) )  
            {                 
                _fPendingUIActive = TRUE;  
                //
                // we then fallthru and treat the double click as a mouse up.
                //                
            }                
            else
                break;

        }
        case WM_LBUTTONUP:
        case WM_RBUTTONUP:
        {
            hr = HandleMouseUp( pMessage, & followUpAction, & eNotify  );
        }
        break;

        case WM_CONTEXTMENU:
        {
            // We must have gotten a RButtonUp.
            //
            _fMouseUp = TRUE;
        }
        break;
        
        case WM_IME_STARTCOMPOSITION:
            hr = S_FALSE; // denied.
            break;
    }

    if ( peTrackerNotify )
        *peTrackerNotify = eNotify;
    if ( pdwFollowUpAction )
        *pdwFollowUpAction |= followUpAction;

Cleanup:
    RRETURN1 ( hr , S_FALSE );
}


//+====================================================================================
//
// Method: DoDrag
//
// Synopsis: Let Trident do it's dragging. If we fail - set _fMouseUP so we don't begin 
//           dragging again.
//
//------------------------------------------------------------------------------------


HRESULT
CControlTracker::DoDrag( SelectionMessage* peMessage )
{
    DWORD dwKeyState = 0;
    HRESULT hr = S_OK;
    CSelectionManager* pManager = _pManager;
    CControlTracker* pTracker = this;
    
    if (peMessage->fCtrl)
        dwKeyState |= FCONTROL;
    if (peMessage->fShift)
        dwKeyState |= FSHIFT;
    if (peMessage->fAlt)
        dwKeyState |= FALT;

    hr = THR( GetViewServices()->DragElement( _pIControlElement, dwKeyState ));

    if ( hr == S_FALSE || 
         pManager->HasSameTracker(pTracker))
    {
        //
        // The Drag failed. So we set _fMouseUp to true to stop an Infinite number of drags
        // for every mouse move.
        //
        _fMouseUp = TRUE; 
    }

    RRETURN1( hr , S_FALSE );
}

HRESULT
CControlTracker::DoResizeDrag(
                                SelectionMessage* pMessage, 
                                DWORD* pFollowUpAction )
{
    HRESULT hr = S_OK;

    _pGrabAdorner->DuringResize( pMessage->pt );

    //
    // Move code commetned out for IE5.
    //
#if 0
    if ( ! _fInMove )
    {
         _pGrabAdorner->DuringResize( pMessage->pt );
    }
    else
    {
        if ( pMessage->elementCookie != NULL )
        {
            RECT newRC ;
            _pGrabAdorner->CalcRectMove(  & newRC, pMessage->pt );
            if ( ! HostDrawDragFeedback( & newRC ))
            {
                _pGrabAdorner->DuringMove( pMessage->pt, FALSE, & newRC );
            }
        }
        else
        {
            //
            // We have started dragging outside the window, 
            // tear down the positioning and start a drag
            //

            CRect   NewRect ;

            _pGrabAdorner->CalcRectMove( & NewRect , pMessage->pt);
            BOOL fHostDraw = HostDrawDragFeedback(& NewRect);        

            _pGrabAdorner->EndMove( 
                                    pMessage->pt, 
                                    NULL , 
                                    ! fHostDraw ,
                                    &NewRect);

            IGNORE_HR( GetViewServices()->HTMLEditorReleaseCapture() );
            BecomePassive();
            //
            // Tell the host to erase the rect, and it's ending.
            //
            _eDragResult = DRAG_RESULT_OUTSIDE_WINDOW;
            DRAG_ACTION eDragAction;

            hr = THR( _pManager->GetDragEditHost()->EndDrag( _eDragResult, 
                                                              & eDragAction,
                                                              & NewRect,
                                                              pMessage->pt ));
                                                              
            IGNORE_HR( GetViewServices()->DragElement(GetControlElement(), 0));
        }
    }
#endif     
    RRETURN1( hr, S_FALSE );
}



HRESULT
CControlTracker::Notify(
        TRACKER_NOTIFY inNotify,
        SelectionMessage *pMessage,
        DWORD* pdwFollowUpAction,
        TRACKER_NOTIFY * peTrackerNotify  )
{
    HRESULT hr = S_OK;

    switch ( inNotify )
    {
        case TN_END_TRACKER_NO_CLEAR:     
        {
            BecomePassive();
        }
        break;
        
        case TN_END_TRACKER:
        {
            BecomePassive();
            UnSelect();
        }
        break;
    }

    RRETURN ( hr );
}


//+====================================================================================
//
// Method: Position
//
// Synopsis: Given two markup pointers, select the element they adorn.
//
//------------------------------------------------------------------------------------

HRESULT
CControlTracker::Position(
        IMarkupPointer* pStart,
        IMarkupPointer* pEnd,
        BOOL            fNotAtBOL,
        BOOL            fAtLogicalBOL)
{
    HRESULT hr = S_OK;

    IHTMLElement* pIElement = NULL ;
    ELEMENT_TAG_ID eTag = TAGID_NULL;
    IMarkupServices * pMarkup = NULL;
    MARKUP_CONTEXT_TYPE eContext = CONTEXT_TYPE_None;
    CControlTracker::HOW_SELECTED eHow =  CControlTracker::HS_NONE ;    
    
    hr = THR( _pManager->GetDoc()->QueryInterface( IID_IMarkupServices, (void**) & pMarkup));
    if ( hr )
        goto Cleanup;

    //
    // Assumed that if we get here - it's from a control range, and the pointers are around
    // the element we want to select.
    //
    hr = THR( GetViewServices()->RightOrSlave(pStart, FALSE, & eContext, & pIElement, NULL, NULL ));
    if ( hr )
        goto Cleanup;
    AssertSz( pIElement, "CControlTracker - expected to find an element");
    AssertSz( eContext != CONTEXT_TYPE_Text, "Did not expect to find text");
    
    hr = THR( pMarkup->GetElementTagId( pIElement , & eTag ));
    if ( hr )
        goto Cleanup;
        
      
    //
    // Verify that this object CAN be site selected
    //
    IGNORE_HR( IsElementSiteSelectable( _pManager,  eTag, pIElement, &eHow ));       
    if( eHow == HS_NONE )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }
           
    hr = THR( SetControlElement( eTag, pIElement ));
    
    
Cleanup:    
    ReleaseInterface ( pMarkup );
    ReleaseInterface ( pIElement );
    
    RRETURN1( hr, S_FALSE );
}


//+====================================================================================
//
// Method:ShouldClickInsideGoActive
//
// Synopsis: Certain elements allow grab handles - but don't go UI Active, and handles
//           don't go away when you click inside. eg. Image.
//
//------------------------------------------------------------------------------------

BOOL
CControlTracker::ShouldClickInsideGoActive( ELEMENT_TAG_ID eTag)
{
    switch( eTag )
    {
        case TAGID_IMG:
        case TAGID_TABLE:
        case TAGID_HR:
            return FALSE;

        default:
            return TRUE;
    }
}

//+====================================================================================
//
// Method: IsSiteSelectable
//
// Synopsis: Is this element site selectable or not.
//
//------------------------------------------------------------------------------------


BOOL 
CControlTracker::IsSiteSelectable( ELEMENT_TAG_ID eTag )
{
    switch ( eTag )
    {
        case TAGID_BUTTON:
        case TAGID_INPUT:
        case TAGID_OBJECT:
        case TAGID_MARQUEE:
//        case TAGID_HTMLAREA:
        case TAGID_TEXTAREA:
        case TAGID_IMG:
        case TAGID_APPLET:
        case TAGID_TABLE:
        case TAGID_SELECT:
        case TAGID_HR:
        case TAGID_OPTION:    
        case TAGID_IFRAME:
        case TAGID_LEGEND:
        
            return TRUE;

        default:

            return FALSE;
    }    
}

//+====================================================================================
//
// Method:  Is Jumpover AtBrowse
// Synopsis: If we start a selection outside one of these
//              Do we want to Jump Over this tag at browse time ?
//
//
// Note that this is almost the same as IsSiteSelectable EXCEPT for Marquee
//
//------------------------------------------------------------------------------------

BOOL 
CSelectTracker::IsJumpOverAtBrowse( ELEMENT_TAG_ID eTag )
{
    switch ( eTag )
    {
        case TAGID_BUTTON:
        case TAGID_INPUT:
        case TAGID_OBJECT:
        case TAGID_TEXTAREA:
        case TAGID_IMG:
        case TAGID_APPLET:
        case TAGID_SELECT:
        case TAGID_HR:
        case TAGID_OPTION:    
        case TAGID_IFRAME:
        case TAGID_LEGEND:
        
            return TRUE;

        default:

            return FALSE;
    }    
}

BOOL
CControlTracker::ShouldStartTracker(
        CSelectionManager* pManager,
        SelectionMessage* pMessage,
        ELEMENT_TAG_ID eTag,
        IHTMLElement* pIElement,
        IHTMLElement** ppIEditThisElement /*= NULL */
 )
{
    BOOL fShouldStart = FALSE;
    //
    // Check to see if the Element we want to site select is already in a selection
    //
    if ( pManager->GetSelectionType() == SELECTION_TYPE_Selection )
    {
        if ( pManager->IsMessageInSelection( pMessage ))
        {
            fShouldStart = FALSE;
            goto Cleanup;
        }
    }

    fShouldStart = IsElementSiteSelectable( pManager, eTag, pIElement, NULL , ppIEditThisElement);

Cleanup:

    return fShouldStart;
}

//+====================================================================================
//
// Method: IsThisElementSiteSelectable
//
// Synopsis: Check to see if this particular element is SiteSelectable or not.
//
//------------------------------------------------------------------------------------


BOOL
CControlTracker::IsThisElementSiteSelectable( 
                        CSelectionManager * pManager,
                        ELEMENT_TAG_ID eTag, 
                        IHTMLElement* pIElement)
{
    BOOL fSiteSelectable = FALSE;

    fSiteSelectable = IsSiteSelectable( eTag );

    if ( ! fSiteSelectable && ! IsTablePart( eTag ) && eTag != ETAG_BODY )
    {
        //
         // Do some work to see if the Element is positioned or sized (has width/height)
         //


        fSiteSelectable =  IsElementPositioned( pIElement );
        if ( ! fSiteSelectable )
        {
            IGNORE_HR( pManager->GetViewServices()->IsElementSized( pIElement, & fSiteSelectable));
        }            
    }           

    return fSiteSelectable;
}


//+====================================================================================
//
// Method: IsElementSiteSelectable
//
// Synopsis: Do the work of figuring out whether we should have a control tracker,
//           and why we're doing it.
//
// Split up from ShouldStartTracker, as FindSelectedElement requires this routine as well
//
//------------------------------------------------------------------------------------

BOOL
CControlTracker::IsElementSiteSelectable( 
                        CSelectionManager* pManager,
                        ELEMENT_TAG_ID eTag, 
                        IHTMLElement* pIElement,
                        CControlTracker::HOW_SELECTED *peHowSelected,
                        IHTMLElement** ppIWeWouldSelectThisElement /* = NULL */)
{
    HRESULT hr = S_OK;
    BOOL fShouldStart = FALSE;
    CControlTracker::HOW_SELECTED eHow =  CControlTracker::HS_NONE ;

    IHTMLElement* pIOuterElement = NULL;
    ELEMENT_TAG_ID eOuterTag = TAGID_NULL;    
    IMarkupPointer* pTempPointer = NULL;

    //
    // BUGBUG - there is much ambiguity here.
    // Normally it's fine to just check the etag of the element that was 
    // clicked on (obtained from the message), and if the element has size or position.
    //
    //
    // However if the element we clicked on isn't site selectable - we find the Edit Context, that we would have
    // and see if it is site selectable.
    //
    
    
    fShouldStart = IsThisElementSiteSelectable( pManager, eTag, pIElement );    
    if ( ! fShouldStart )
    {          
        IFC( pManager->GetMarkupServices()->CreateMarkupPointer( & pTempPointer ));
        hr = THR( pTempPointer->MoveAdjacentToElement( pIElement, ELEM_ADJ_AfterBegin));
        if ( hr )
        {
            hr = THR( pTempPointer->MoveAdjacentToElement( pIElement, ELEM_ADJ_BeforeBegin ));
        }            
        IFC( pManager->GetViewServices()->GetFlowElement( pTempPointer, & pIOuterElement ));
        if ( pIOuterElement )
        {                    
            IFC( pManager->GetMarkupServices()->GetElementTagId( pIOuterElement, & eOuterTag));
            fShouldStart = IsThisElementSiteSelectable( pManager, eOuterTag, pIOuterElement );
            if ( fShouldStart )
            {
                if ( ppIWeWouldSelectThisElement )
                    ReplaceInterface( ppIWeWouldSelectThisElement, pIOuterElement );            
                eHow =  CControlTracker::HS_OUTER_ELEMENT;
            }                
        }
    }   
    else
    {
        if ( ppIWeWouldSelectThisElement )
            ReplaceInterface( ppIWeWouldSelectThisElement, pIElement );
        eHow =  CControlTracker::HS_FROM_ELEMENT;
    }        
        
Cleanup:
    ReleaseInterface( pTempPointer );
    ReleaseInterface( pIOuterElement );        
    if ( peHowSelected )
        *peHowSelected = eHow;
    
    return fShouldStart;
    
}


HRESULT
CEditTracker::GetElementFromMessage(
    CSelectionManager* pManager,
    SelectionMessage* peMessage,
    IHTMLElement** ppElement )
{
    HRESULT hr = S_OK;

    if ( peMessage->elementCookie )
    {
        hr = THR( pManager->GetViewServices()->GetElementFromCookie( peMessage->elementCookie, ppElement ));
    }

    RRETURN ( hr );
}


#if DBG == 1

VOID
CControlTracker::VerifyOkToStartControlTracker( SelectionMessage * pMessage )
{

    HRESULT hr = S_OK;

    IMarkupServices* pMarkup = NULL;
    IHTMLViewServices* pViewServices = NULL;
    ELEMENT_TAG_ID eTag = TAGID_NULL;
    IHTMLElement* pElement = NULL;
    IHTMLDocument2* pIDoc = NULL;


    //
    // Examine the Context of the thing we started dragging in.
    //
    pIDoc = _pManager->GetDoc();
    hr = THR( pIDoc->QueryInterface( IID_IMarkupServices, (void**) & pMarkup ));
    if ( hr )
        goto Cleanup;

    hr = THR( pIDoc->QueryInterface( IID_IHTMLViewServices, (void**) & pViewServices ));
    if ( hr )
        goto Cleanup;

    Assert( pMessage->elementCookie );

    hr = THR( pViewServices->GetElementFromCookie( pMessage->elementCookie, & pElement ) );
    if ( hr )
        goto Cleanup;

    hr = THR( pMarkup->GetElementTagId(pElement, & eTag));
    if ( hr )
        goto Cleanup;

    Assert( ShouldStartTracker( _pManager, pMessage, eTag, pElement ) );

Cleanup:
    AssertSz( ! hr , "Unexpected ResultCode in VerifyOkToStartControlTracker");
    ReleaseInterface( pMarkup ) ;
    ReleaseInterface( pViewServices );
    ReleaseInterface( pElement );

}

#endif

//+====================================================================================
//
// Method: CommitResize
//
// Synopsis: Commit the Resize Change to the element
//
//------------------------------------------------------------------------------------

HRESULT
CControlTracker::CommitResize(SelectionMessage* peMessage)
{
    HRESULT hr = S_OK;
    IHTMLStyle * pStyle = NULL;
    VARIANT vtSizeChange;
    VARIANT vtLeft;
    VARIANT vtTop;
    RECT    newRect;
    POINT   point;
    IHTMLElement* pIEditElement = NULL;
    CUndoUnit undoUnit(_pManager->GetEditor());

    ReleaseCapture();

#ifdef NEVER    
    //
    // marka we don't bother unless we moved by at least the threshold.
    //
    if ((abs(peMessage->pt.x - _startMoveX ) > gSizeDragMin.cx) ||
        (abs(peMessage->pt.y - _startMoveY) > gSizeDragMin.cy)) 
#endif

    //
    // marka - per brettt - dont have a threshold - just don't resize if the point didn't move.
    //

    _pGrabAdorner->EndResize( peMessage->pt, & newRect );

    if (( peMessage->pt.x != _startMoveX ) || 
        (peMessage->pt.y != _startMoveY)  )
    {    
        VariantInit( & vtSizeChange );
        VariantInit( & vtTop );
        VariantInit( & vtLeft );

        hr = THR( _pIControlElement->get_style( & pStyle ));
        if ( hr )
            goto Cleanup;

        hr = THR( pStyle->get_left( & vtLeft ));
        if ( hr )
            goto Cleanup;

        hr = THR( pStyle->get_top( & vtTop ));
        if ( hr )
            goto Cleanup;

        Assert ( V_VT( & vtTop ) == VT_BSTR && V_VT( & vtLeft ) == VT_BSTR );

        IGNORE_HR( undoUnit.Begin(IDS_EDUNDORESIZE) );

        //
        // Per Access PM we are going to set Left and Top on resize
        // only if the Left *and* the Top attributes are set
        //    
        if ( V_BSTR( & vtLeft ) != NULL &&
             V_BSTR( & vtTop )  != NULL ) 
        {
            //
            // We must convert left and top to parent relative coordinates 
            //
            point.x = newRect.left;
            point.y = newRect.top;

            IGNORE_HR( GetViewServices()->TransformPoint( & point, 
                                                          COORD_SYSTEM_GLOBAL, 
                                                          COORD_SYSTEM_PARENT, 
                                                          _pIControlElement ));


            V_VT( & vtTop ) = VT_I4;
            V_I4( & vtTop ) = point.y;
            hr = THR( pStyle->put_top( vtTop ));
            if ( hr )
                goto Cleanup;        

            V_VT( & vtLeft ) = VT_I4;
            V_I4( & vtLeft ) = point.x;
            hr = THR( pStyle->put_left( vtLeft ));
            if ( hr )
                goto Cleanup;        
        }    

        V_VT( & vtSizeChange ) = VT_I4;
        V_I4( & vtSizeChange ) = abs( newRect.right - newRect.left ) ;

        hr = THR( pStyle->put_width( vtSizeChange ));
        if ( hr )
            goto Cleanup;

        V_VT( & vtSizeChange ) = VT_I4;
        V_I4( & vtSizeChange ) = abs( newRect.bottom - newRect.top ) ;

        hr = THR( pStyle->put_height( vtSizeChange ));
        if ( hr )
            goto Cleanup;
    }

    
Cleanup:
    VariantClear( & vtSizeChange );
    VariantClear( & vtTop );
    VariantClear( & vtLeft );
    ReleaseInterface( pIEditElement );
    ReleaseInterface( pStyle );

    RRETURN ( hr );
}


#if DBG == 1

void
CSelectTracker::StateToString(TCHAR* pAryMsg, SELECT_STATES inState )
{
    switch ( inState )
    {
    case ST_START:
        edWsprintf( pAryMsg , _T("%s"), _T("ST_START"));
        break;

    case ST_WAIT1:
        edWsprintf( pAryMsg , _T("%s"), _T("ST_WAIT1"));
        break;

    case ST_DRAGOP:
        edWsprintf( pAryMsg , _T("%s"), _T("ST_DRAGOP"));
        break;

    case ST_MAYDRAG:
        edWsprintf( pAryMsg , _T("%s"), _T("ST_MAYDRAG"));
        break;

    case ST_WAITBTNDOWN1:
        edWsprintf( pAryMsg , _T("%s"), _T("ST_WAITBTNDOWN1"));
        break;

    case ST_WAIT2:
        edWsprintf( pAryMsg , _T("%s"), _T("ST_WAIT2"));
        break;

    case ST_DOSELECTION:
        edWsprintf( pAryMsg , _T("%s"), _T("ST_DOSELECTION"));
        break;

    case ST_WAITBTNDOWN2:
        edWsprintf( pAryMsg , _T("%s"), _T("ST_WAITBTNDOWN2"));
        break;

    case ST_SELECTEDWORD:
        edWsprintf( pAryMsg , _T("%s"), _T("ST_SELECTEDWORD"));
        break;

    case ST_SELECTEDPARA:
        edWsprintf( pAryMsg , _T("%s"), _T("ST_SELECTEDPARA"));
        break;

    case ST_WAIT3RDBTNDOWN:
        edWsprintf( pAryMsg , _T("%s"), _T("ST_WAIT3RDBTNDOWN"));
        break;

    case ST_MAYSELECT1:
        edWsprintf( pAryMsg , _T("%s"), _T("ST_WAIT3RDBTNDOWN"));
        break;

    case ST_MAYSELECT2:
        edWsprintf( pAryMsg , _T("%s"), _T("ST_WAIT3RDBTNDOWN"));
        break;

    case ST_STOP :
        edWsprintf( pAryMsg , _T("%s"), _T("ST_WAIT3RDBTNDOWN"));
        break;

    case ST_PASSIVE :
        edWsprintf( pAryMsg , _T("%s"), _T("ST_WAIT3RDBTNDOWN"));
        break;

    default:
        AssertSz(0,"Unknown State");
    }
}


void
CSelectTracker::ActionToString(TCHAR* pAryMsg, ACTIONS inState )
{
    switch ( inState )
    {
     
    case A_UNK:
        edWsprintf( pAryMsg , _T("%s"), _T("A_UNK"));
        break;

    case A_ERR:
        edWsprintf( pAryMsg , _T("%s"), _T("A_ERR"));
        break;

    case A_DIS:
        edWsprintf( pAryMsg , _T("%s"), _T("A_DIS"));
        break;

    case A_IGN:
        edWsprintf( pAryMsg , _T("%s"), _T("A_IGN"));
        break;

    case A_1_2:
        edWsprintf( pAryMsg , _T("%s"), _T("A_1_2"));
        break;

    case A_1_4:
        edWsprintf( pAryMsg , _T("%s"), _T("A_1_4"));
        break;

    case A_1_14:
        edWsprintf( pAryMsg , _T("%s"), _T("A_1_14"));
        break;

    case A_2_14:
        edWsprintf( pAryMsg , _T("%s"), _T("A_2_14"));
        break;

    case A_2_14r:
        edWsprintf( pAryMsg , _T("%s"), _T("A_2_14r"));
        break;

    case A_3_2:
        edWsprintf( pAryMsg , _T("%s"), _T("A_3_2"));
        break;

    case A_3_2m:
        edWsprintf( pAryMsg , _T("%s"), _T("A_3_2m"));
        break;

    case A_3_14:
        edWsprintf( pAryMsg , _T("%s"), _T("A_3_14"));
        break;

    case A_4_8:
        edWsprintf( pAryMsg , _T("%s"), _T("A_4_8"));
        break;

    case A_4_14 :
        edWsprintf( pAryMsg , _T("%s"), _T("A_4_14"));
        break;

    case A_4_14m :
        edWsprintf( pAryMsg , _T("%s"), _T("A_4_14m"));
        break;
        
    case A_5_6 :
        edWsprintf( pAryMsg , _T("%s"), _T("A_5_6"));
        break;
    case A_5_7 :
        edWsprintf( pAryMsg , _T("%s"), _T("A_5_7"));
        break;
    case A_6_6 :
        edWsprintf( pAryMsg , _T("%s"), _T("A_6_6"));
        break;
    case A_6_6m :
        edWsprintf( pAryMsg , _T("%s"), _T("A_6_6m"));
        break;
    case A_6_14 :
        edWsprintf( pAryMsg , _T("%s"), _T("A_6_14"));
        break;
    case A_7_8 :
        edWsprintf( pAryMsg , _T("%s"), _T("A_7_8"));
        break;
    case A_7_14 :
        edWsprintf( pAryMsg , _T("%s"), _T("A_7_14"));
        break;
        
    case A_8_6 :
        edWsprintf( pAryMsg , _T("%s"), _T("A_8_6"));
        break;
    case A_8_10 :
        edWsprintf( pAryMsg , _T("%s"), _T("A_8_10"));
        break;
    case A_9_6 :
        edWsprintf( pAryMsg , _T("%s"), _T("A_9_6"));
        break;
    case A_9_14 :
        edWsprintf( pAryMsg , _T("%s"), _T("A_9_14"));
        break;
    case A_10_9 :
        edWsprintf( pAryMsg , _T("%s"), _T("A_10_9"));
        break;
    case A_10_14 :
        edWsprintf( pAryMsg , _T("%s"), _T("A_10_14"));
        break;       
    case A_10_14m :
        edWsprintf( pAryMsg , _T("%s"), _T("A_10_14m"));
        break; 
    case A_11_6 :
        edWsprintf( pAryMsg , _T("%s"), _T("A_11_6"));
        break; 
    case A_11_14 :
        edWsprintf( pAryMsg , _T("%s"), _T("A_11_14"));
        break; 
    case A_12_6 :
        edWsprintf( pAryMsg , _T("%s"), _T("A_12_6"));
        break; 
    case A_12_14 :
        edWsprintf( pAryMsg , _T("%s"), _T("A_12_14"));
        break; 

    
    case A_1_15 :
        edWsprintf( pAryMsg , _T("%s"), _T("A_1_15"));
        break;
        
    case A_4_15:
        edWsprintf( pAryMsg , _T("%s"), _T("A_4_15"));
        break;
    
    case A_3_15 :
        edWsprintf( pAryMsg , _T("%s"), _T("A_3_15"));
        break;
        
    case A_5_15 :
        edWsprintf( pAryMsg , _T("%s"), _T("A_5_15"));
        break; 
        
    case A_7_15 :
        edWsprintf( pAryMsg , _T("%s"), _T("A_7_15"));
        break; 

    case A_8_15 :
        edWsprintf( pAryMsg , _T("%s"), _T("A_8_15"));
        break; 

    case A_9_15 :
        edWsprintf( pAryMsg , _T("%s"), _T("A_9_15"));
        break; 
        
    case A_10_15 :
        edWsprintf( pAryMsg , _T("%s"), _T("A_10_15"));
        break; 

    case A_12_15 :
        edWsprintf( pAryMsg , _T("%s"), _T("A_12_15"));
        break; 
        
    default:
        AssertSz(0,"Unknown State");
    }
}

void
CSelectTracker::DumpSelectState( 
                                SelectionMessage* peMessage, 
                                ACTIONS inAction, 
                                DWORD dwAction, 
                                BOOL fInTimer /*=FALSE*/ )
{
    if ( IsTagEnabled(tagSelectionTrackerState))
    {
        TCHAR sSelectState[20];
        TCHAR sMessageType[20];
        TCHAR sAction[20];
        
        StateToString( sSelectState, _fState );
        if ( peMessage )
            MessageTypeToString( sMessageType, peMessage->message );

        if ( ! fInTimer )
            ActionToString( sAction, inAction);

        if ( ! fInTimer )
        {
            TraceTag(( tagSelectionTrackerState, "Tracker State: %ls Action:%ls Message:%ls pt:%d,%d FollowUpAction:%d",
                        sSelectState,sAction, sMessageType, peMessage->pt.x, peMessage->pt.y, dwAction));
        }
        else
        {
            TraceTag(( tagSelectionTrackerState, "\nOnTimerTick. Tracker State: %ls FollowUpAction:%d",
                        sSelectState,sAction, dwAction));
        }
    }
}

#endif


BOOL
CSelectTracker::IsActive()
{
    return _fActive;
}
     
VOID 
CSelectTracker::StartTimer()
{
    HRESULT hr = S_OK;
    //
    // Now possible for the manager to have released the tracker
    // and created a new one. Instead of touching the timers when we shouldn't
    // Don't do anything if we no longer belong to the manager.
    //
    if ( _pManager->GetActiveTracker() == this )
    {               
        Assert( ! _pManager->IsInTimer() );
        TraceTag(( tagSelectionTrackerState, "Starting Timer"));  
        hr = THR( GetViewServices()->StartHTMLEditorDblClickTimer());
        if ( hr )
            AssertSz(0, "Error starting timer");
        else
        {
            _pManager->SetInTimer( TRUE);
        }        
    }   
    
}

VOID 
CSelectTracker::StopTimer()
{
    HRESULT hr = S_OK;

    //
    // Now possible for the manager to have released the tracker
    // and created a new one. Instead of touching the timers when we shouldn't
    // Don't do anything if we no longer belong to the manager.
    //
    if ( _pManager->GetActiveTracker() == this )
    {       
        Assert( _pManager->IsInTimer() );

        TraceTag(( tagSelectionTrackerState, "Stopping Timer"));    

        hr = THR( GetViewServices()->StopHTMLEditorDblClickTimer());

        //
        // If a Fire On Select Start failed - it is valid for the timer to fail
        // the Doc may have unloaded.
        //
#if DBG == 1
        if ( ! _fFailFireOnSelectStart )
        {
            AssertSz( hr == S_OK || hr == S_FALSE  , "Error stopping timer");
        }
#endif     
        _pManager->SetInTimer( FALSE );               
    }
}

HRESULT 
CSelectTracker::GetLocation(POINT *pPoint)
{
    HRESULT             hr;
    HTMLPtrDispInfoRec  pt;
    IHTMLElement* pIFlowElement;
    IFR( GetViewServices()->GetLineInfo(_pEndPointer, _fNotAtBOL , &pt) );
    pPoint->y = pt.lBaseline;
    pPoint->x = pt.lXPosition;         
    IFC( GetViewServices()->GetFlowElement( _pEndPointer, & pIFlowElement ));
    if ( ! pIFlowElement )
    {
        IFC( _pManager->GetEditableElement( & pIFlowElement));
    }
    
    IFC( GetViewServices()->TransformPoint( pPoint, COORD_SYSTEM_CONTENT, COORD_SYSTEM_GLOBAL, pIFlowElement ));
    
Cleanup:    
    ReleaseInterface( pIFlowElement );
    
    return S_OK;    
}

BOOL
CSelectTracker::EndPointsInSameFlowLayout()
{
    return PointersInSameFlowLayout( _pStartPointer, _pEndPointer, NULL , GetViewServices() );
}


#if !defined(NO_IME) && defined(IME_RECONVERSION)
/*
 *  HRESULT HandleImeRequest 
 *
 *  @func
 *      Initiates an IME composition string edit on IMR_RECONVERTSTRING.
 *  @comm
 *      Called from the message loop to handle WM_IME_REQUEST with
 *      IMR_RECONVERTSTRING.
 *      This is a glue routine into the IME object hierarchy.
 *
 *  @devnote
 *
 *  @rdesc
 *      HRESULT-S_FALSE for DefWindowProc processing, S_OK if not.
 */

HRESULT
CSelectTracker::HandleImeRequest(
    SelectionMessage* pMessage,
    TRACKER_NOTIFY * peTrackerNotify )
{
    LRESULT lResult = 0;

    if (   (   pMessage->wParam == IMR_RECONVERTSTRING
            || pMessage->wParam == IMR_CONFIRMRECONVERTSTRING)
        && _pManager->IsContextEditable()
        && IsPassive()
        && PointersInSameFlowLayout( _pStartPointer, _pEndPointer, NULL , GetViewServices() ))
    {
        HRESULT hr;
        const BOOL fUnicode = IsWindowUnicode(pMessage->hwnd);
        long cch = 0;
        long cbReconvert;
        long cb = 0;
        UINT uiKeyboardCodePage = 0;
        TCHAR ach[MAX_RECONVERSION_SIZE];

        IFC( ScoopTextForReconversion( MAX_RECONVERSION_SIZE, &cch, ach ) );

        if (cch == 0) // selection is required.
            goto Cleanup;

        if (fUnicode)
        {
            cbReconvert = sizeof( RECONVERTSTRING ) + (cch + 1) * sizeof(WCHAR);
        }
        else
        {
            uiKeyboardCodePage = GetKeyboardCodePage();
            cb = WideCharToMultiByte( uiKeyboardCodePage, 0, ach, cch, NULL, 0, NULL, NULL);
            cbReconvert = sizeof( RECONVERTSTRING ) + (cb + 1);
        }

        lResult = cbReconvert;

        if (   pMessage->wParam == IMR_RECONVERTSTRING
            && pMessage->lParam)
        {
            //
            // Populate the RECONVERTSTRING structure
            // We're doing a simple reconversion
            //

            WHEN_DBG( RECONVERTSTRING * prs = (RECONVERTSTRING *)pMessage->lParam );

            ((RECONVERTSTRING *)pMessage->lParam)->dwSize = cbReconvert;
            ((RECONVERTSTRING *)pMessage->lParam)->dwVersion = 0;
            ((RECONVERTSTRING *)pMessage->lParam)->dwStrLen =
            ((RECONVERTSTRING *)pMessage->lParam)->dwCompStrLen =
            ((RECONVERTSTRING *)pMessage->lParam)->dwTargetStrLen = fUnicode ? cch : cb;
            ((RECONVERTSTRING *)pMessage->lParam)->dwStrOffset = sizeof(RECONVERTSTRING);
            ((RECONVERTSTRING *)pMessage->lParam)->dwCompStrOffset =
            ((RECONVERTSTRING *)pMessage->lParam)->dwTargetStrOffset = 0;

            if (fUnicode)
            {
                WCHAR * pch = (WCHAR *)((BYTE *)((RECONVERTSTRING *)pMessage->lParam) + sizeof(RECONVERTSTRING));

                memcpy(pch, ach, cch * sizeof(WCHAR));
                pch[cch] = 0;
            }
            else
            {
                char * pb = (char *)((BYTE *)((RECONVERTSTRING *)pMessage->lParam) + sizeof(RECONVERTSTRING));

                WideCharToMultiByte( uiKeyboardCodePage, 0, ach, cch, pb, cb, NULL, NULL);
                pb[cb] = 0;
            }
        }
    }

Cleanup:

    pMessage->lResult = lResult;
    
    return S_OK;
}

HRESULT
CSelectTracker::ScoopTextForReconversion(
    LONG cchMax,        // IN   - max chars to scoop
    LONG * pcch,        // OUT  - number of chars scooped
    TCHAR * pch )       // OUT  - chars (buffer must be at least cchMax large)
{
    HRESULT hr = S_OK;
    CEditPointer epPointer(_pManager->GetEditor());
    DWORD dwSearch = BREAK_CONDITION_Text;
    DWORD dwFound = BREAK_CONDITION_None;
    const TCHAR * pchStart = pch;
    const TCHAR * pchEnd = pch + cchMax;

    Assert( cchMax > 0 );

    IFC( epPointer->MoveToPointer(_pStartPointer) );
    IFC( epPointer.SetBoundary( _pManager->GetStartEditContext() , _pManager->GetEndEditContext() ));
    IFC( epPointer.Constrain() );

    // 
    // Scoot begin pointer to the beginning of the non-white text
    //

    IFC( epPointer.Scan( RIGHT, dwSearch, &dwFound, NULL, NULL, NULL, SCAN_OPTION_SkipWhitespace | SCAN_OPTION_SkipNBSP ) );

    if ( !epPointer.CheckFlag(dwFound, BREAK_CONDITION_TEXT) )
        goto Cleanup;

    dwFound = BREAK_CONDITION_None;
    IFC( epPointer.Scan( LEFT, dwSearch, &dwFound ) );
    IFC( _pStartPointer->MoveToPointer(epPointer) ); // set the start

    //
    // Scoop up the text
    // 

    while (pch < pchEnd)
    {
        BOOL fLeftOfEnd;
        
        dwFound = BREAK_CONDITION_None;

        IFC( epPointer.Scan( RIGHT, dwSearch, &dwFound, NULL, NULL, pch ) );

        if (!epPointer.CheckFlag(dwFound, BREAK_CONDITION_TEXT) )
            break;

        // NOTE (cthrash) WCH_NBSP is not native to any Far East codepage.
        // Here we simply convert to space, thus prevent the IME from getting confused.

        if (*pch == WCH_NBSP)
        {
            *pch = L' ';
        }

        pch++;

        IFC( epPointer->IsLeftOf( _pEndPointer, &fLeftOfEnd ));

        if (!fLeftOfEnd)
            break;
    }

    IFC( _pEndPointer->MoveToPointer(epPointer) ); // set the end
    IFC( _pSelRenSvc->MoveSegmentToPointers( 0, _pStartPointer, _pEndPointer, HIGHLIGHT_TYPE_Selected ));

Cleanup:

    *pcch = pch - pchStart;

    RRETURN(hr);
}

#endif // IME_RECONVERSION

VOID 
CEditTracker::TakeCapture()
{
    HRESULT hr = S_OK;
    //
    // Now possible for the manager to have released the tracker
    // and created a new one. Instead of touching the timers when we shouldn't
    // Don't do anything if we no longer belong to the manager.
    //
    if ( _pManager->GetActiveTracker() == this )
    {       
        Assert( ! _pManager->IsInCapture() );
        
        hr = THR( GetViewServices()->HTMLEditorTakeCapture());
        if ( hr )
            AssertSz(0, "Error taking capture");
        else
        {
            _pManager->SetInCapture( TRUE );
        }        
    }       
}

VOID 
CEditTracker::ReleaseCapture()
{
    HRESULT hr = S_OK;

    //
    // Now possible for the manager to have released the tracker
    // and created a new one. Instead of touching the timers when we shouldn't
    // Don't do anything if we no longer belong to the manager.
    //
    if ( _pManager->GetActiveTracker() == this )
    {       
        Assert( _pManager->IsInCapture() );
        
        hr = THR( GetViewServices()->HTMLEditorReleaseCapture());
        if ( hr )
            AssertSz(0, "Error releasing capture");
        else    
        {
            _pManager->SetInCapture( FALSE );   
        }        
    }
}

#ifndef NO_IME

HRESULT
CIme::Init()
{
    HRESULT hr;
    IHTMLDocument2* pIDocument = _pManager->GetDoc();
    IMarkupServices * pMarkupServices = NULL;
    IHTMLViewServices * pViewServices = NULL;
    IHTMLCaret * pCaret = NULL;
    IHTMLElement * pCurElement = NULL ;
    extern CODEPAGE GetKeyboardCodePage();

    //_eType = SELECTION_TYPE_IME;
    //_fCaretVisible = TRUE;

    hr = THR( pIDocument->QueryInterface(IID_IMarkupServices, (void**)&pMarkupServices ));
    if (hr)
        goto Cleanup;

    hr = THR( pIDocument->QueryInterface( IID_IHTMLViewServices, (void **)&pViewServices ) );
    if (hr)
        goto Cleanup;

    hr = THR( pMarkupServices->CreateMarkupPointer( &_pmpInsertionPoint ));
    if (hr)
        goto Cleanup;

    WHEN_DBG( _pManager->SetDebugName( _pmpInsertionPoint, strImeIP ) );

    hr = THR( pMarkupServices->CreateMarkupPointer( &_pmpStartUncommitted ));
    if (hr)
        goto Cleanup;

    WHEN_DBG( _pManager->SetDebugName( _pmpStartUncommitted, strImeUncommittedStart ) );

    hr = THR( pMarkupServices->CreateMarkupPointer( &_pmpEndUncommitted ));
    if (hr)
        goto Cleanup;

    WHEN_DBG( _pManager->SetDebugName( _pmpEndUncommitted, strImeUncommittedEnd ) );

    hr = THR( pViewServices->GetCaret( &pCaret ));
    if (hr)
        goto Cleanup;

    hr = THR( pCaret->MovePointerToCaret( _pmpInsertionPoint ));
    if (hr)
       goto Cleanup;

    hr = THR( _pmpInsertionPoint->SetGravity( POINTER_GRAVITY_Left ) );
    if (hr)
        goto Cleanup;
    
    hr = THR( pViewServices->GetFlowElement( _pmpInsertionPoint, &pCurElement ));
    if (hr)
        goto Cleanup;

    // BUGBUG (cthrash) Need to determine IME properties from pCurElement, such
    // as IME mode, password, etc.
    //
    // BUGBUG (marka) when and if we do write this code - note that pCurElement can be NULL
    // if ( pCurElement )

    hr = THR( pViewServices->GetCurrentSelectionRenderingServices(  & _pSelRenSvc ) );
    if (hr)
        goto Cleanup;

Cleanup:

    ReleaseInterface( pCurElement );
    ReleaseInterface( pCaret );
    ReleaseInterface( pViewServices );
    ReleaseInterface( pMarkupServices ) ;

    RRETURN(hr);
}

void
CIme::Deinit()
{
    ClearHighlightSegments();

    ClearInterface( &_pmpInsertionPoint );
    ClearInterface( &_pmpStartUncommitted );
    ClearInterface( &_pmpEndUncommitted );
    ClearInterface( &_pSelRenSvc );
}

HRESULT
CSelectionManager::HandleImeMessage(
    SelectionMessage *pMessage,
    DWORD* pdwFollowUpAction,
    TRACKER_NOTIFY * peTrackerNotify )
{
    HRESULT hr = S_FALSE;

    switch (pMessage->message)
    {
        case WM_IME_STARTCOMPOSITION:   // IME input is being kicked off
            hr = THR(StartCompositionGlue( FALSE, peTrackerNotify ));
            break;

        case WM_IME_COMPOSITION:        // State of user's input has changed
            hr = THR(CompositionStringGlue( pMessage->lParam, peTrackerNotify ));
            break;

        case WM_IME_ENDCOMPOSITION:     // User has OK'd IME conversions
            hr = THR(EndCompositionGlue( peTrackerNotify ));
            break;

        case WM_IME_NOTIFY:             // Candidate window state change info, etc.
            hr = THR(NotifyGlue( pMessage->wParam, pMessage->lParam, peTrackerNotify ));
            break;

        case WM_IME_COMPOSITIONFULL:    // Level 2 comp string about to overflow.
            hr = THR(CompositionFullGlue( peTrackerNotify ));
            break;

        case WM_IME_CHAR:               // 2 byte character, usually FE.
            if (IsIMEComposition())
            {
                hr = _pIme->IgnoreIMECharMsg() ? S_OK : S_FALSE;
            }
            else
            {
                hr = _fIgnoreImeCharMsg ? S_OK : S_FALSE;
            }
            break;

        case WM_LBUTTONDOWN:
        case WM_KILLFOCUS:
            TerminateIMEComposition( TERMINATE_NORMAL, peTrackerNotify );
            break;

        case WM_KEYDOWN:
            if (   !IsIMEComposition()
                && pMessage->wParam == VK_KANJI)
            {
                hr = THR(StartHangeulToHanja(peTrackerNotify));
            }
            break;

#if !defined(NO_IME) && defined(IME_RECONVERSION)
        case WM_IME_REQUEST:
            if (pMessage->wParam == IMR_RECONVERTSTRING)
            {
                // user hits a reconversion key or click on a reconversion button
                // only IME_RECONVERTSTRING request is handled
            }
            break;
#endif // IME_RECONVERSION
    }

    RRETURN1(hr,S_FALSE);
}

HRESULT
CIme::SetCaretVisible( IHTMLDocument2* pIDoc, BOOL fVisible )
{
    HRESULT hr = S_OK ;
    SP_IHTMLCaret spc;
    IHTMLViewServices * pvs = NULL;

    IFC( pIDoc->QueryInterface( IID_IHTMLViewServices, (void **) & pvs ));
    IFC( pvs->GetCaret( &spc ));

    if( fVisible )
        hr = spc->Show( FALSE );
    else
        hr = spc->Hide();

Cleanup:
    ReleaseInterface( pvs );
    RRETURN( hr );
}

HRESULT
CIme::ReplaceRange(
    TCHAR * pch,
    LONG cch,
    BOOL fShowCaret /* = TRUE */,
    LONG ichCaret   /* = -1 */,
    BOOL fMoveIP    /* FALSE */)
{
    IMarkupServices * pTreeServices = NULL;
    IHTMLViewServices * pViewServices = NULL;
    IHTMLCaret * pCaret = NULL;
    BOOL fPositioned;
    HRESULT hr = S_FALSE;
    CUndoUnit undoUnit(_pManager->GetEditor());
    CCaretTracker * pCaretTracker = NULL;

    if (!OK(GetCaretTracker( &pCaretTracker ) ))
        goto Cleanup;

    if (! pCaretTracker)
        goto Cleanup;

    IFC( ClearHighlightSegments() );
    
    hr = THR( _pManager->GetDoc()->QueryInterface( IID_IMarkupServices, (void**) & pTreeServices));
    if (hr)
        goto Cleanup;

    //
    // First nuke the old range, if we have one.
    //

    hr = THR( _pmpStartUncommitted->IsPositioned( &fPositioned ) );
    if (hr)
        goto Cleanup;

    IFC( undoUnit.Begin(IDS_EDUNDOTYPING) );

    if (fPositioned)
    {
        //
        // Memorize the formatting just before we nuke the old range.
        //

        CSpringLoader * psl = GetSpringLoader();
        if (psl)
        {
            IGNORE_HR(psl->SpringLoad(_pmpStartUncommitted, SL_ADJUST_FOR_INSERT_RIGHT));
        }

        //
        // Now remove.
        //

        hr = THR( pTreeServices->Remove( _pmpStartUncommitted, _pmpEndUncommitted ) );
        if (hr)
            goto Cleanup;
    }

    //
    // Now add the new text
    //

    if (cch)
    {
        hr = THR( _pManager->GetDoc()->QueryInterface( IID_IHTMLViewServices, (void **)&pViewServices ) );
        if (hr)
            goto Cleanup;

        hr = THR( pViewServices->GetCaret( &pCaret ));
        if (hr)
            goto Cleanup;

        // This places the caret *before* the IP

        IFC( pCaret->MoveCaretToPointer( _pmpInsertionPoint, TRUE, TRUE, CARET_DIRECTION_INDETERMINATE ));

        IFC( AdjustUncommittedRangeAroundInsertionPoint() );
        
        // In goes the text.  Call through CCaretTracker

        IFC( pCaretTracker->InsertText( pch, -1, pCaret ) );

        IFC( ClingUncommittedRangeToText() );

        if (fMoveIP)
        {
            IFC( pCaret->MovePointerToCaret( _pmpInsertionPoint ) );

            IFC( AdjustUncommittedRangeAroundInsertionPoint() );

            pCaretTracker->SetCaretShouldBeVisible( TRUE );
        }
        else if (ichCaret != -1)
        {
            AssertSz( ichCaret >= 0 && ichCaret <= cch,
                      "IME caret should be within the text.");
            
            if (ichCaret < cch)
            {
                CEditPointer epForCaret(_pManager->GetEditor());
                DWORD dwSearch = BREAK_CONDITION_Text;
                DWORD dwFound;

                IFC( epForCaret->MoveToPointer( _pmpInsertionPoint ));

                while (ichCaret--)
                {
                    hr = THR( epForCaret.Scan(RIGHT, dwSearch, &dwFound) );
                    if (hr)
                        goto Cleanup;

                    if (!epForCaret.CheckFlag(dwFound, dwSearch))
                        break;
                }

                IFC( pCaret->MoveCaretToPointer( epForCaret, TRUE, TRUE, CARET_DIRECTION_INDETERMINATE ));

                // BUGBUG (cthrash) Ideally, we want to ensure the whole of the
                // currently converted range to show.  This would require some
                // trickly logic to keep the display from shifting annoyingly.
                // So for now, we only ensure the beginning of it is showing.
                
                IFC( GetViewServices()->ScrollPointerIntoView(epForCaret, _fNotAtBOL, POINTER_SCROLLPIN_Minimal) );
            }
            else
            {
                IFC( GetViewServices()->ScrollPointerIntoView(_pmpEndUncommitted, _fNotAtBOL, POINTER_SCROLLPIN_Minimal) );
            }

            pCaretTracker->SetCaretShouldBeVisible( fShowCaret );
        }
    }
    else
    {
        pCaretTracker->SetNotAtBOL( FALSE );
    }

Cleanup:

    if (pCaretTracker)
        pCaretTracker->Release();

    ReleaseInterface( pCaret );
    ReleaseInterface( pViewServices );
    ReleaseInterface( pTreeServices );

    RRETURN1(hr, S_FALSE);
}

HRESULT
CIme::AdjustUncommittedRangeAroundInsertionPoint()
{
    HRESULT hr;
    
    // This places the pointer *before* the IP

    IFC( _pmpStartUncommitted->MoveToPointer( _pmpInsertionPoint ) );

    IFC( _pmpStartUncommitted->SetGravity( POINTER_GRAVITY_Left) );

    // This places the pointer *after* the IP (and therefore *after* the caret.)

    IFC( _pmpEndUncommitted->MoveToPointer( _pmpInsertionPoint ) );

    IFC( _pmpEndUncommitted->SetGravity( POINTER_GRAVITY_Right) );

Cleanup:

    RRETURN(hr);
}

HRESULT
CIme::ClingUncommittedRangeToText()
{
    HRESULT         hr;
    DWORD           dwSearch = BREAK_CONDITION_OMIT_PHRASE;
    DWORD           dwFound;
    CEditPointer    epTest(_pManager->GetEditor());

    // Cling to text on the left
    
    IFR( epTest->MoveToPointer(_pmpStartUncommitted) );
    IFR( epTest.Scan(RIGHT, dwSearch, &dwFound) )
    if (epTest.CheckFlag(dwFound, BREAK_CONDITION_TEXT))
    {
        IFR( epTest.Scan(LEFT, dwSearch, &dwFound) ); // restore position
        IFR( _pmpStartUncommitted->MoveToPointer(epTest) );
    }

    // Cling to text on the right
    IFR( epTest->MoveToPointer(_pmpEndUncommitted) );
    IFR( epTest.Scan(LEFT, dwSearch, &dwFound) );
    if (epTest.CheckFlag(dwFound, BREAK_CONDITION_TEXT))
    {
        IFR( epTest.Scan(RIGHT, dwSearch, &dwFound) ); // restore position
        IFR( _pmpEndUncommitted->MoveToPointer(epTest) );
    }

    return S_OK;    
}

#endif

//
// The below were to do internal move of 2-D postioned elemetns. This has been cut from IE5 
//
//
//
#if 0

//+====================================================================================
//
// Method: BeginDrag
//
// Synopsis: Decide what action to take at the start of a Drag Operation
//
// Returns
//  DRAG_ACTION_OLE_DRAG        - Do a 'normal' OLE Drag/Drop
//  DRAG_ACTION_2D_POSITION     - Do a '2D-Position'
//
//------------------------------------------------------------------------------------

BOOL
CControlTracker::BeginDrag(DRAG_ACTION *peDragAction)
{
    HRESULT hr = S_OK;
    BOOL fOkToDrag = TRUE;
    DRAG_ACTION eDragAction = DRAG_ACTION_OLE_DRAG;
    IHTMLElement* pIDragElement = GetControlElement();
    pIDragElement->AddRef();    
    IObjectIdentity * pIObj1 = NULL;
    IObjectIdentity * pIObj2 = NULL;
    HRESULT hEqual = S_OK;
    
    if ( _fUseHostDrag )
    {
        //
        // We give the Drag Edit Host a pointer to an IHTMLElement that is ref-counted
        // if it chooses to it can change this pointer using Replace Interface.
        //

        hr = THR( _pManager->GetDragEditHost()->BeginDrag( &pIDragElement, 
                                                           & eDragAction ));
        if ( hr == S_FALSE )
        {
            fOkToDrag = FALSE;
            goto Cleanup;
        }
        
        //
        // See if the element we drag is different
        //
        if ( pIDragElement != GetControlElement() )
        {
            IFC( pIDragElement->QueryInterface( IID_IObjectIdentity, ( void**) & pIObj1));
            IFC( _pIControlElement->QueryInterface(IID_IObjectIdentity, ( void**) & pIObj2 ));
            hEqual = pIObj1->IsEqualObject( pIObj2 );

            if ( hEqual != S_OK )
            {
                _fDragElementDifferent = TRUE;
                ReplaceInterface( & _pIDragElement, pIDragElement );
            }
            else
            {
                _fDragElementDifferent = FALSE;
                ReplaceInterface( & _pIDragElement, _pIControlElement );                     
            }                                
        }
        else
        {
            _fDragElementDifferent = FALSE ;            
            ReplaceInterface( & _pIDragElement, _pIControlElement );            
        }            
    }


Cleanup:
    ReleaseInterface( pIObj1 );
    ReleaseInterface( pIObj2 );
    ReleaseInterface( pIDragElement );
    *peDragAction = eDragAction;
    return fOkToDrag;
}

//+====================================================================================
//
// Method: ShouldChangeCursor
//
// Synopsis: Callback to allow a Host to draw their own cursor.
//
//------------------------------------------------------------------------------------
BOOL 
CControlTracker::ShouldChangeCursor( LPCTSTR inCursorId )
{
    HRESULT hr = S_OK;
    
    if ( _fUseHostDrag )
    {
        hr = THR( _pManager->GetDragEditHost()->ChangeCursor( inCursorId ));
        if ( FAILED(hr) )
            return FALSE;
        else
            return TRUE;
    }
    else
        return TRUE;

}

//+====================================================================================
//
// Method: EndDrag
//
// Synopsis: Decide what action to take at the end of an OLE/Drag/Drop operation
//
//           We should only get here if we decide decided to do a 2D-Position here
//
//     Return TRUE - if it's ok to do the drag.
//
//------------------------------------------------------------------------------------

BOOL
CControlTracker::EndDrag( POINT pt, DRAG_ACTION *peAction, CRect* pEndDragRect )
{
    HRESULT hr = S_OK;
    DRAG_ACTION eDragAction = DRAG_ACTION_2D_POSITION;
    BOOL fEndResult = TRUE;
    RECT rc = * pEndDragRect;
    Assert( _fUseHostDrag );

    _eDragResult = DRAG_RESULT_SUCCESS;
    hr = THR( _pManager->GetDragEditHost()->EndDrag( _eDragResult, 
                                                      & eDragAction,
                                                      &rc ,
                                                      pt ));
                                                  
    AssertSz( eDragAction != DRAG_ACTION_OLE_DRAG, "OLE Drag is not allowed at EndDrag");                                                  
    
    if ( FAILED( hr ) )
    {
        fEndResult =  FALSE;
    }
    *peAction = eDragAction;
    
    return fEndResult;

}

//+====================================================================================
//
// Method: HostDrawDragFeedback
//
// Synopsis: Provide Callback for Host to Draw Drag Feedback.
//
//           Return TRUE if host drew the feedback
//                  FALSE if host did not draw the feedback ( and we should )
//------------------------------------------------------------------------------------


BOOL
CControlTracker::HostDrawDragFeedback(RECT* pRect )
{
    Assert( _fUseHostDrag );
    HRESULT hr = S_OK;
    DRAG_ACTION eDragAction = DRAG_ACTION_2D_POSITION;
    hr = _pManager->GetDragEditHost()->DrawDragFeedback( &eDragAction, pRect );
    AssertSz( eDragAction == DRAG_ACTION_2D_POSITION, "Other drag codes not allowed during draw drag feedback");
    if ( FAILED( hr ))
        return TRUE;
    else
        return FALSE;
}   
//+====================================================================================
//
// Method: CommitMove
//
// Synopsis:Commit the Move to the element
//
//------------------------------------------------------------------------------------

HRESULT
CControlTracker::CommitMove(SelectionMessage* peMessage, TRACKER_NOTIFY * peTrackerNotify )
{
    HRESULT hr = S_OK;

    IHTMLDocument2 * pDoc = _pManager->GetDoc();
    IHTMLStyle * pStyle = NULL;
    IHTMLStyle2 * pStyle2 = NULL;
    DRAG_ACTION eAction = DRAG_ACTION_2D_POSITION;
    VARIANT vtPosChange;
    POINT newLocation;
    VariantInit( & vtPosChange );
    BOOL fGoAhead = FALSE;
    CRect   NewRect ;
    _pGrabAdorner->CalcRectMove( & NewRect , peMessage->pt);
    BSTR positionStyle;
    CUndoUnit undoUnit(_pManager->GetEditor());

    positionStyle = SysAllocString( _T("absolute"));
    if (positionStyle == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    ReleaseCapture();
    
    Assert( _fUseHostDrag );
    fGoAhead = EndDrag( peMessage->pt, & eAction, & NewRect );
    BOOL fHostDraw;
    
    if ( fGoAhead )
    {
        switch ( eAction )
        {
            case DRAG_ACTION_2D_POSITION :
            {    

                fHostDraw = HostDrawDragFeedback(& NewRect);    
                   
                _pGrabAdorner->EndMove( 
                                        peMessage->pt  , 
                                        & newLocation, 
                                        ! fHostDraw ,
                                        &NewRect);


                //
                // Transform the point into the coords of the parent.
                //
                POINT point;
                point.x = newLocation.x; 
                point.y = newLocation.y;
                
                IGNORE_HR( GetViewServices()->TransformPoint( & point, 
                                                              COORD_SYSTEM_GLOBAL, 
                                                              COORD_SYSTEM_PARENT, 
                                                              _pIDragElement ) );    
                //
                // Allow the Host to "snap" the new location.
                //
                IFC( _pManager->GetDragEditHost()->SnapPoint( & point ));
                
                IFC( _pIDragElement->get_style( & pStyle ));
                IFC( pStyle->QueryInterface( IID_IHTMLStyle2, (void**) & pStyle2));


                IGNORE_HR( undoUnit.Begin(IDS_EDUNDOMOVE) );

                IFC( pStyle2->put_position( positionStyle ));

                V_VT( & vtPosChange ) = VT_I4;
                V_I4( & vtPosChange ) = point.x ;

                IFC( pStyle->put_left( vtPosChange ));


                V_VT( & vtPosChange ) = VT_I4;
                V_I4( & vtPosChange ) = point.y ;

                IFC( pStyle->put_top( vtPosChange ));

                _pGrabAdorner->UpdateAdorner();
            }
            break;
        
            case DRAG_ACTION_PASTE_IN_FLOW:
            {
                fHostDraw = HostDrawDragFeedback(& NewRect);    
                   
                _pGrabAdorner->EndMove( 
                                        peMessage->pt  , 
                                        & newLocation, 
                                        ! fHostDraw ,
                                        &NewRect);
                                        
                DoPasteInFlow( peMessage, peTrackerNotify );
            }
            break;
        }
    }        
Cleanup:
    SysFreeString( positionStyle );

    ReleaseInterface( pStyle2);
    ReleaseInterface( pStyle );

    ClearInterface( & _pIDragElement );
    _fDragElementDifferent = FALSE;
    Assert( _pIDragElement == NULL );
    RRETURN ( hr );
}


//+====================================================================================
//
// Method: DoInternalDrag
//
// Synopsis: We started a 2D Position Drag, the host changed it's mind and wants a 2D-Drag-Drop
//           We simulate this by doing a cut of the control and a repaste.
//
//           BUGBUG - we assume the drag ends in trident. We also assume that it's valid to
//           do a cut & paste here. We don't do a 'real' OLE-Drag-Drop, so any assumptiions
//           underlying pieces may have made about this may break.
//
//------------------------------------------------------------------------------------


HRESULT
CControlTracker::DoPasteInFlow( SelectionMessage* peMessage, TRACKER_NOTIFY *peTrackerNotify )
{
    IOleCommandTarget* pTarget = NULL;
    HRESULT hr = S_OK;
    IHTMLTxtRange* pRange = NULL;
    IHTMLBodyElement* pIBody = NULL;
    IHTMLDocument2* pIDoc2 = NULL;
    IMarkupPointer* pStart = NULL;
    IMarkupPointer* pEnd = NULL;
    GUID theGUID = CGID_MSHTML;
    BOOL fNotAtBOL = FALSE;
    BOOL fAtLogicalBOL = TRUE;
    BOOL fRightOfCp = FALSE;
    BOOL fValidTree = FALSE;    
    IMarkupPointer* pPointer = NULL;
    IMarkupPointer* pLeftBoundary = NULL ;
    IMarkupPointer* pRightBoundary = NULL;
    IHTMLElement* pICurElement = NULL;
    MARKUP_CONTEXT_TYPE eContext = CONTEXT_TYPE_None;
    int iWherePointer = 0;
    BOOL fFound = FALSE;
    IHTMLStyle* pStyle = NULL;
    IHTMLStyle2 * pStyle2 = NULL;
    IHTMLElement* pIBodyElement = NULL;
    BSTR positionStyle;

    positionStyle = SysAllocString( _T("static"));
    if (positionStyle == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    //
    // Position the pointer to where we will do the paste in flow, move to pointers around it.
    //
    IFC( GetMarkupServices()->CreateMarkupPointer( & pPointer ));
    IFC( GetMarkupServices()->CreateMarkupPointer( & pLeftBoundary ));
    IFC( GetMarkupServices()->CreateMarkupPointer( & pRightBoundary ));
    
    IFC( GetViewServices()->MoveMarkupPointerToMessage( pPointer,
                                                         peMessage,
                                                         & fNotAtBOL,
                                                         & fAtLogicalBOL,
                                                         & fRightOfCp ,
                                                         & fValidTree,
                                                         TRUE, 
                                                         GetEditableElement(), 
                                                         NULL )) ;
    IFC( pLeftBoundary->MoveToPointer( pPointer ));
    IFC( pRightBoundary->MoveToPointer( pPointer));
    IFC( pLeftBoundary->SetGravity( POINTER_GRAVITY_Left ));
    IFC( pRightBoundary->SetGravity( POINTER_GRAVITY_Right ));

    //
    // Create a Text Range, move it around the element we're doing a paste in flow for.
    //
    IFC( _pManager->GetDoc()->QueryInterface( IID_IHTMLDocument2 , (void**) & pIDoc2));
    IFC( pIDoc2->get_body( & pIBodyElement ));
    IFC( pIBodyElement->QueryInterface( IID_IHTMLBodyElement, (void**) & pIBody));
    IFC( pIBody->createTextRange( & pRange ));

    IFC( GetMarkupServices()->CreateMarkupPointer( & pStart ));
    IFC( GetMarkupServices()->CreateMarkupPointer( & pEnd ));
    IFC( pStart->MoveAdjacentToElement( _pIDragElement, ELEM_ADJ_BeforeBegin ));
    IFC( pEnd->MoveAdjacentToElement( _pIDragElement, ELEM_ADJ_AfterEnd ));

    IFC( GetMarkupServices()->MoveRangeToPointers( pStart, pEnd, pRange ));


    //
    // Make the element be "in-flow"
    //
    IFC( _pIDragElement->get_style( & pStyle ));
    IFC( pStyle->QueryInterface( IID_IHTMLStyle2, (void**) & pStyle2));
    IFC( pStyle2->put_position( positionStyle ));

    _pManager->SetIgnoreExitTree( TRUE ); // tell the manager to please not whack us.
    IFC( pRange->QueryInterface( IID_IOleCommandTarget, (void**) & pTarget ));
    IFC( pTarget->Exec( &theGUID , IDM_CUT, 0, NULL, NULL  ));
    _pManager->SetIgnoreExitTree( FALSE );                

    //
    // Paste the Element
    //
    IFC( _pManager->GetEditor()->PasteFromClipboard( pPointer, NULL, NULL ));

    //
    // Now find an element equivalent to the one we pasted
    //
    IFC( pPointer->MoveToPointer( pLeftBoundary));


    for(;;)
    {
        IFC( GetViewServices()->RightOrSlave(pPointer, FALSE, & eContext, & pICurElement, NULL, NULL));
        IFC( OldCompare( pPointer, pRightBoundary, & iWherePointer));

        if ( iWherePointer == LEFT )
        {       
            break;
        }                    
        if ( ( eContext == CONTEXT_TYPE_Text ) || ( eContext == CONTEXT_TYPE_None ) )
        {
            AssertSz( ( eContext != CONTEXT_TYPE_Text), "Did not expect to hit text");
            break;
        }
        
        if ( pICurElement && ( EquivalentElements( GetMarkupServices(), pICurElement, _pIDragElement)) )
        {
            fFound = TRUE;
            break;
        }                        
        
        hr = THR( GetViewServices()->RightOrSlave(pPointer, TRUE, NULL, NULL, NULL, NULL));
        if ( hr )
            goto Cleanup;          
            
        ClearInterface( & pICurElement );
    }

    if ( fFound )
    {
        IFC( pLeftBoundary->MoveAdjacentToElement( pICurElement, ELEM_ADJ_BeforeBegin ));
        IFC( pRightBoundary->MoveAdjacentToElement( pICurElement, ELEM_ADJ_AfterEnd));
    }

    _pManager->CopyTempMarkupPointers( pLeftBoundary, pRightBoundary );
    Assert( peTrackerNotify );
    if ( fFound )
        *peTrackerNotify = TN_END_TRACKER_POS_CONTROL;
    else
        *peTrackerNotify = TN_END_TRACKER_POS_SELECT;
                  
Cleanup:
    SysFreeString( positionStyle );
    ReleaseInterface( pIBodyElement );
    ReleaseInterface( pStyle );
    ReleaseInterface( pStyle2 );
    ReleaseInterface( pICurElement );
    ReleaseInterface( pIDoc2 );
    ReleaseInterface( pIBody );
    ReleaseInterface( pTarget );
    ReleaseInterface( pRange );
    ReleaseInterface( pStart );
    ReleaseInterface( pEnd );
    ReleaseInterface( pPointer );
    ReleaseInterface( pLeftBoundary );
    ReleaseInterface( pRightBoundary );
 
    RRETURN ( hr );
}
#endif 

