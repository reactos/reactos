//+------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1998
//
//  File:       SELMAN.CXX
//
//  Contents:   Implementation of ISelectionManager interface inisde of mshtmled.dll
//
//  Classes:    CMshtmlEd
//
//  History:    06-10-98 - marka - created
//
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

#ifndef _X_EDCMD_HXX_
#define _X_EDCMD_HXX_
#include "edcmd.hxx"
#endif

#ifndef _X_DELCMD_HXX_
#define _X_DELCMD_HXX_
#include "delcmd.hxx"
#endif

#ifndef _X_BLOCKCMD_HXX_
#define _X_BLOCKCMD_HXX_
#include "blockcmd.hxx"
#endif

#ifndef X_EDTRACK_HXX_
#define X_EDTRACK_HXX_
#include "edtrack.hxx"
#endif

#ifndef _X_EDADORN_HXX_
#define _X_EDADORN_HXX_
#include "edadorn.hxx"
#endif

#ifndef X_IME_HXX_
#define X_IME_HXX_
#include "ime.hxx"
#endif

using namespace EdUtil;

MtDefine(CSelectionManager, Utilities, "Selection Manager")
DeclareTag(tagSelectionMgrState, "Selection", "Selection show manager state")
DeclareTag(tagSelectionMgrNotify, "Selection", "SelectionManager show TrackerNotify")
ExternTag( tagSelectionTrackerState );


extern int edWsprintf(LPTSTR pstrOut, LPCTSTR pstrFormat, LPCTSTR pstrParam);
#if DBG == 1

static const LPCTSTR strStartContext = _T( "   ** Start_Edit_Context");
static const LPCTSTR strEndContext = _T( "   ** End_Edit_Context");
static const LPCTSTR strBeganTyping = _T(" BeganTyping");

#endif

CSelectionManager::CSelectionManager( CHTMLEditor * pEd )
    :   _pEd( pEd )
{
    HRESULT hr = S_OK;

    Init();
    _pActiveTracker = NULL;    // The currently active tracker.
    _pAdorner= NULL;
    _pStartContext = NULL;
    _pEndContext = NULL;
    _pISCList = NULL;
    _fInitSequenceChecker = FALSE;
    _fIgnoreSetEditContext = FALSE;
    _fDontChangeTrackers = FALSE;
    _fHadGrabHandles = FALSE;
    _fDrillIn = FALSE;
    _fLastMessageValid = FALSE;
    _fNoScope = FALSE;
    _pStartTemp = NULL;
    _pEndTemp = NULL;
    _fInTimer = FALSE;
    _fInCapture = FALSE;
    _fContextAcceptsHTML = FALSE;
    _fPendingUndo = FALSE;
    Assert( _pIEditableElement == NULL );
    //
    // TODO (marka) Move all this stuff into a public Initialize() method. Passing
    // pointers to HRESULTS in constructors is bad practice (seltrack), even worse
    // here is completely ignoring the HRESULT of memory allocating routines.
    //

    hr = THR( _pEd->GetMarkupServices()->CreateMarkupPointer(& _pStartContext ));
    if( hr )
        goto Cleanup;

    IGNORE_HR( _pStartContext->SetGravity( POINTER_GRAVITY_Left ) );

    hr = THR( _pEd->GetMarkupServices()->CreateMarkupPointer( &_pEndContext ));
    if( hr )
        goto Cleanup;

    IGNORE_HR( _pEndContext->SetGravity( POINTER_GRAVITY_Right ) );

    hr = THR( _pEd->GetMarkupServices()->CreateMarkupPointer( &_pPointerBeganTyping ) );
    if( hr )
        goto Cleanup;

#if DBG == 1
    SetDebugName( _pStartContext,strStartContext );
    SetDebugName( _pEndContext,strEndContext );
    SetDebugName( _pPointerBeganTyping,strBeganTyping );
#endif

Cleanup:
    // done
    return;
}

CSelectionManager::~CSelectionManager()
{
    ReleaseInterface( _pStartContext );
    ReleaseInterface( _pEndContext );
    ReleaseInterface( _pStartTemp );
    ReleaseInterface( _pEndTemp );
    ReleaseInterface( _pIEditableElement );
    ReleaseInterface( _pPointerBeganTyping );
    if(_pISCList)
        delete _pISCList;

    DWORD dwFollowUpAction = 0; // BUGBUG: This needs to be passed back to the doc [marka]
    EndCurrentTracker( &dwFollowUpAction, NULL, FALSE, FALSE ); // End without clearing the selection if any
    AssertSz(dwFollowUpAction == 0, "Dropping follow up action");

    Assert(!_pIme);

    DestroyAdorner();
}

VOID
CSelectionManager::Init()
{
    _pendingFollowUp = FOLLOW_UP_ACTION_None;
    _fContextEditable = FALSE;
    _fParentEditable = FALSE;
    _lastStringId = UINT(-1);
    _eContextTagId = TAGID_NULL ;
    _fNoScope = FALSE;
    _fIgnoreExitTree = FALSE;
    _fEditFocusAllowed = TRUE;
    _fPositionedSet = FALSE;
}

//+=====================================================================
// Method: HandleMessage
//
// Synopsis: Handle a UI Message passed from Trident to us.
//
//----------------------------------------------------------------------
#define OEM_SCAN_RIGHTSHIFT 0x36

HRESULT
CSelectionManager::HandleMessage(
    SelectionMessage* pSelectionMessage,
    DWORD * pdwFollowUpAction )
{
    HRESULT hr = S_FALSE ;
    TRACKER_NOTIFY  eTrackerNotify = TN_NONE;
    BOOL fStarted = FALSE;
    Assert( _pEd->GetDoc() );
    HWND hwndDoc;
    CEditTracker* pTracker = _pActiveTracker;

    if ( pTracker )
        pTracker->AddRef();

    //
    // We may not have an Active Tracker if the Document was torn down.
    //

    if( ( _pActiveTracker ) ||
        ( pSelectionMessage->message == WM_LBUTTONDOWN ) ||
        ( pSelectionMessage->message == WM_RBUTTONDOWN ) ||
        ( pSelectionMessage->message == WM_KEYDOWN )     ||
        ( pSelectionMessage->message == WM_KEYUP )       
#ifdef UNIX // LAST
        || (pSelectionMessage->message == WM_MBUTTONDOWN)
#endif
        )
    {
        switch ( pSelectionMessage->message )
        {
            case WM_RBUTTONDOWN:
            case WM_LBUTTONDOWN:
#ifdef UNIX
            case WM_MBUTTONDOWN:
#endif
                if ( ( ! _pActiveTracker ) ||
                    ( ! _pActiveTracker->IsListeningForMouseDown()) )
                {
                    IFC( GetViewServices()->GetViewHWND( & hwndDoc ));                   
                    if ( ! IsInWindow( hwndDoc, pSelectionMessage->pt , TRUE))
                    {
                        hr = S_FALSE;
                        goto Cleanup; // Ignore mouse down messages that aren't in Trident's Window or in the Content of the EditContext
                    }                        
                    hr = ShouldChangeTracker( pSelectionMessage, pdwFollowUpAction, & fStarted );
                    if ( !fStarted && _pActiveTracker )
                        hr = _pActiveTracker->HandleMessage( pSelectionMessage, pdwFollowUpAction, & eTrackerNotify  );
                }
                else
                 {
                    if ( pSelectionMessage->message != WM_RBUTTONDOWN )
                        hr = _pActiveTracker->HandleMessage( pSelectionMessage, pdwFollowUpAction, & eTrackerNotify  );
                }

#ifdef UNIX
                if (pSelectionMessage->message == WM_MBUTTONDOWN)
                    hr = S_FALSE;
#endif
                break;

            case WM_RBUTTONUP:
            case WM_LBUTTONUP:
            case WM_TIMER:
            case WM_LBUTTONDBLCLK :
            case WM_MOUSEMOVE:
            case WM_KEYDOWN:
            case WM_KEYUP:
            case WM_KILLFOCUS:
            case WM_CONTEXTMENU:

#if DBG == 1
            if ( pSelectionMessage->message == WM_KEYDOWN && 
                 pSelectionMessage->wParam == VK_F12 )
                    DebugBreak();
#endif                    
                if ( _pActiveTracker )
                {
                    hr = _pActiveTracker->HandleMessage( pSelectionMessage, pdwFollowUpAction, & eTrackerNotify );

                    AssertSz(    pSelectionMessage->message != WM_RBUTTONUP
                              || hr == S_FALSE,
                              "Expected hr to be S_FALSE for RBUTTON_UP" );
                }
                else // we have a keydown or keyup scenareo for possible change of document direction
                {
                    if( pSelectionMessage->message == WM_KEYDOWN)
                    {
                        _fShiftCaptured = FALSE;
                        if(pSelectionMessage->fCtrl && 
                           pSelectionMessage->wParam == VK_SHIFT)
                        {
                            _fShiftCaptured = TRUE;
                        }
                    }
                    else if(_fShiftCaptured && 
                            pSelectionMessage->message == WM_KEYUP &&
                            pSelectionMessage->fCtrl && 
                            pSelectionMessage->wParam == VK_SHIFT)
                    {
                        _fShiftCaptured = FALSE;
                        // 1. Find out if the system is a right to left keyboard installed
                        BOOL fBidiEnabled;
                        IFC( GetViewServices()->IsBidiEnabled(&fBidiEnabled) );

                        if(fBidiEnabled)
                        {
                            // if the right shift key is coming up we are going to be changing to right to left
                            // if the left shift key is coming up we are going to be changing to left to right
                            long eHTMLDir = (LOBYTE(HIWORD(pSelectionMessage->lParam)) == OEM_SCAN_RIGHTSHIFT) 
                                            ? htmlDirRightToLeft
                                            : htmlDirLeftToRight;

                            IFC( GetViewServices()->SetDocDirection(eHTMLDir) );
                        }
                    }
                        
                }
                break;

#ifndef NO_IME
            case WM_IME_STARTCOMPOSITION:
            case WM_IME_COMPOSITION:        // State of user's input has changed
            case WM_IME_ENDCOMPOSITION:     // User has OK'd IME conversions
            case WM_IME_NOTIFY:             // Candidate window state change info, etc.
            case WM_IME_COMPOSITIONFULL:    // Level 2 comp string about to overflow.
            case WM_IME_CHAR:               // 2 byte character, usually FE.
#ifdef IME_RECONVERSION
            case WM_IME_REQUEST:
#endif // IME_RECONVERSION
                hr = THR( HandleImeMessage( pSelectionMessage, pdwFollowUpAction, & eTrackerNotify ) );
                break;
#endif // NO_IME


            case WM_CAPTURECHANGED:
                hr = HandleCaptureChanged( pSelectionMessage );
                break;

            case WM_CHAR: 

                if ( _pActiveTracker && pSelectionMessage->lParam != 0  )
                {
                    hr = _pActiveTracker->HandleMessage( pSelectionMessage, pdwFollowUpAction, & eTrackerNotify );
                }
                break;

            case WM_INPUTLANGCHANGE:
                // Update the Input Sequence Checker (Thai, Hindi, Vietnamese, etc.)
                LCID lcidCurrent = LOWORD(pSelectionMessage->lParam);
                // PaulNel - If ISC is not initialized, this is a good place to do it.
                if(!_fInitSequenceChecker)
                {
                    CreateISCList();
                }
                if(_pISCList)
                    _pISCList->SetActive(lcidCurrent);
                if ( _pActiveTracker )
                {
                    hr = _pActiveTracker->HandleMessage( pSelectionMessage, pdwFollowUpAction, & eTrackerNotify  );
                }                    
                break;

        }

        if ( eTrackerNotify != TN_NONE )
        {
            HRESULT hrNotify = THR(TrackerNotify( eTrackerNotify, pSelectionMessage, pdwFollowUpAction ));

            if (eTrackerNotify == TN_END_TRACKER_START_IME ||
                eTrackerNotify == TN_END_TRACKER_POS_CARET_REBUBBLE )
            {
                hr = hrNotify;  // NB (cthrash) This needs to bubble up for proper IME functionality.
            }
        }
    }

Cleanup:

    if ( pTracker )
        pTracker->Release();


    return ( hr );
}


HRESULT
CSelectionManager::SetEditContextPrivate(
                                    BOOL fEditable,
                                    BOOL fSetSelection,
                                    BOOL fParentEditable,
                                    IMarkupPointer* pStart,
                                    IMarkupPointer* pEnd,
                                    BOOL fNoScope,
                                    BOOL fCreateTracker
                                          )

{
    Assert( GetDoc() );
    HRESULT hr = S_OK;

    BOOL fSameElement = FALSE;
    BOOL fPositioned = FALSE;
    //
    // Certain tracker actions ( eg. making an element absolute positioned ) - do a become current
    // on an element. As far as ed is concerned we want to ignore this bogus change in currency.
    //
    if (( ! _pActiveTracker ) ||
        ( _pActiveTracker && ( ! _pActiveTracker->IsActive() ) ) )
    {
        //
        // Check to see if we're setting the edit context again on teh same element
        // if we are - don't touch the _fGrabHandles flag. It's used to make sure
        // we don't startup a ControlTracker again.
        //

        if ( IsSameEditContext( pStart, pEnd, & fPositioned ) && 
            (!!fEditable) == (!!_fContextEditable) && 
            ( ENSURE_BOOL(_fNoScope) == fNoScope ) )
        {
            fSameElement = TRUE;
        }
        else
        {
            if ( !_fDrillIn )
                _fHadGrabHandles = FALSE;
        }


        if (( ! fPositioned ) || ( ! fSameElement ) )
        {
            IFC( _pStartContext->MoveToPointer( pStart ));
            IFC( _pEndContext->MoveToPointer( pEnd ));
            _fNoScope = fNoScope; // Why ? We need to set this to ensure edit context is set correctly.
            
            //
            // Set the Editable Element
            //
            
            ClearInterface( & _pIEditableElement );
            IFC( GetEditableElement( & _pIEditableElement ));
        }

        SELECTION_TYPE myType = fSetSelection ? SELECTION_TYPE_Selection : SELECTION_TYPE_Caret;
        SELECTION_TYPE currentType = GetSelectionType();

        if ( (( ! fSameElement ) && ( fCreateTracker) ) ||
             ( ! _pActiveTracker ) || // we may have lost a tracker on a lose focus
             ( _fDrillIn ) )  // always bounce trackers on Drilling In.
        {
            _fContextEditable = fEditable;
            _fParentEditable = fParentEditable ;
            _eContextTagId = TAGID_NULL;
            _fNoScope = fNoScope;
            _fPositionedSet = FALSE;
            
            if ( ! IsDontChangeTrackers() )
            {
                if ( ! fNoScope ) // Dont create a tracker if we're in a no-scope element.
                {
                    DWORD dwFollowUpAction = 0; // BUGBUG: This needs to be passed back to the doc [marka]

                    //
                    // See if we already had a caret in this edit context.
                    // if so - create the tracker there instead of just at the start of the edit context
                    //
                    if ( myType == SELECTION_TYPE_Caret && 
                         IsCaretAlreadyWithinContext())
                    {
                        SP_IHTMLCaret spCaret;
                        CEditPointer edPointer( GetEditor());
                        IFC( GetViewServices()->GetCaret(&spCaret) );
                        IFC( spCaret->MovePointerToCaret( edPointer ));

                        hr = CreateTrackerForType( &dwFollowUpAction, myType  , edPointer, edPointer );
                    }
                    else
                    {
                        hr = CreateTrackerForType( &dwFollowUpAction, myType  , pStart, pEnd);
                    }
                    AssertSz(dwFollowUpAction == 0, "Dropping follow up action");
                }
                else
                {
                    CCaretTracker::SetCaretVisible( GetDoc(), FALSE );
                }
                if ( _pAdorner )
                    DestroyAdorner();
            }
        }
        else if( fSameElement && myType == currentType == SELECTION_TYPE_Caret )
        {
            //
            // we didn't change trackers and the current tracker is the caret
            // so ensure that the caret is properly visible
            //

            CCaretTracker * pTracker = (CCaretTracker * ) _pActiveTracker;
            pTracker->SetCaretShouldBeVisible( pTracker->ShouldCaretBeVisible() );
        }

        if ( _fDrillIn )
            _fDrillIn = FALSE;
    }

    EnsureAdornment();
    
    if (!fSameElement)
    {
        SP_IHTMLElement spElement;
        SP_IHTMLElement spContainer;
        BOOL            fHTML;

        _fContextAcceptsHTML = FALSE; // false by default

        IFC( GetEditableElement(&spElement) );
        if (spElement != NULL)
        {
            IFC( FindContainer(GetMarkupServices(), spElement, &spContainer) );
            if (spContainer != NULL)
            {
                IFC( GetViewServices()->IsContainerElement(spContainer, NULL, &fHTML) );
                _fContextAcceptsHTML = fHTML;
            }
        }        
        if (!_fContextAcceptsHTML && _pActiveTracker)
        {
            CSpringLoader *psl = _pActiveTracker->GetSpringLoader();

            if (psl)
                psl->Reset();
        }

    }


    if ( _fLastMessageValid )
    {
        if (( _pAdorner ) && ( GetEditableTagId() == ETAG_OBJECT ))
        {
            //
            // If we are a UI Active Object - return S_FALSE and BAIL.
            //
            hr = S_FALSE;
            goto Cleanup;
        }
        //
        // Rebubble the last message.
        //
        DWORD followUp = 0;
        _fLastMessageValid = FALSE;
        IGNORE_HR( HandleMessage( & _lastMessage, &followUp ) );
        AssertSz( ( followUp == 0 ), "Unexpected followUp Code");

        if ( _fNextMessageValid )
        {
            followUp = 0;
            _fNextMessageValid = FALSE;
            IGNORE_HR( HandleMessage( & _nextMessage, &followUp ) );

            //
            // BUGBUG - we get the Onclick code here - which we will drop.
            // This is ok - becasue the OnClick code is set in CControlTracker::HandleMouseUp 
            // where we really had a valid MouseUp message.
            //
            AssertSz( ( followUp == 0 || followUp == 32 ), "Unexpected followUp Code");
        }
    }

    if ( _fPendingTrackerNotify )
    {
        DWORD followUp = 0;
        _fPendingTrackerNotify = FALSE ;

        TraceTag(( tagSelectionTrackerState, "\n---SetEditContextPrivate: About to start a caret--- "));

        TrackerNotify(
                _pendingTrackerNotify ,
                NULL,
                & followUp );

                
        AssertSz( ( followUp == 0 ), "Unexpected followUp Code");      
    }
Cleanup:

    RRETURN ( hr );
}

//+====================================================================================
//
// Method: IsCaretAlreadyWithinContext
//
// Synopsis: Look and see if the physical Caret is already within the edit context
//           This is a check to see if we previously had the focus - and we can just restore
//           the caret to where we were
//          
//------------------------------------------------------------------------------------

BOOL
CSelectionManager::IsCaretAlreadyWithinContext()
{
    HRESULT hr ;
    CEditPointer edCaret( GetEditor());   
    SP_IHTMLCaret spCaret;
    BOOL fInEdit = FALSE;

    IFC( GetViewServices()->GetCaret( & spCaret ));    
    IFC( spCaret->MovePointerToCaret( edCaret ));
    IFC( IsInEditContext( edCaret, & fInEdit ));   

    //
    // BUGBUG - nice to assert here if the caret is already visible if it's in the context
    // it gets tripped in dialogs.js. It looks like input.select() is calling becomeCurrent
    // unnecessarily. Investigate ripping it out eventually.
    //
Cleanup:

    return  fInEdit ;
}


//+====================================================================================
//
// Method: EnsureAdornment
//
// Synopsis: Ensure that if it's valid for us to have a UI-Active border - that we have one.
//
//------------------------------------------------------------------------------------


HRESULT
CSelectionManager::EnsureAdornment()
{
    if ( ShouldElementShowUIActiveBorder() &&
         !_pAdorner &&
         GetSelectionType() != SELECTION_TYPE_Control &&
         !IsDontChangeTrackers() )
    {
        _fHadGrabHandles = TRUE;
        IGNORE_HR( CreateAdorner() );
    }

    RRETURN( S_OK );
}

//+====================================================================================
//
// Method: SetEditContext
//
// Synopsis: Sets the "edit context". Called by trident when an Editable
//           element is made current.
//
//------------------------------------------------------------------------------------

HRESULT
CSelectionManager::SetEditContext(
                                    BOOL fEditable,
                                    BOOL fSetSelection,
                                    BOOL fParentEditable,
                                    IMarkupPointer* pStart,
                                    IMarkupPointer* pEnd,
                                    BOOL fNoScope )
{
    RRETURN ( SetEditContextPrivate( fEditable, fSetSelection, fParentEditable, pStart, pEnd, fNoScope, TRUE ));
}

//+====================================================================================
//
// Method: EmptySelection
//
// Synopsis: An empty selection method for commands and such to call.
//
//------------------------------------------------------------------------------------


HRESULT
CSelectionManager::EmptySelection( BOOL fChangeTrackerAndSetRange /*=TRUE*/)
{
    HRESULT hr = S_OK;

    DWORD actionCode = FOLLOW_UP_ACTION_None;

    hr = THR( EmptySelection( & actionCode, FALSE, fChangeTrackerAndSetRange ) );

    if ( actionCode != FOLLOW_UP_ACTION_None )
    {
        AssertSz(0, "unexecpected action code !");
        _pendingFollowUp = actionCode;
    }

    RRETURN ( hr );
}


HRESULT
CSelectionManager::EmptySelection(
    DWORD *         pdwFollowUpAction,
    BOOL            fHideCaret /*= FALSE */,
    BOOL            fChangeTrackerAndSetRange /*=TRUE*/) // If called from the OM Selection, it may want to hide the caret
{
    HRESULT hr = S_OK;
    IMarkupPointer* pPointer = NULL ;
    IMarkupPointer* pPointer2 = NULL;
    ISegmentList * pSegmentList = NULL;
    ISelectionRenderingServices * pSelRenSvc = NULL;
    int iSegmentCount = 0;
    SELECTION_TYPE eType = SELECTION_TYPE_None;


    IFC( GetViewServices()->GetCurrentSelectionSegmentList( & pSegmentList ));

    //
    // We will always position the Caret on Selection End. The Caret will decide
    // whehter it's visible
    //
    IFC( GetMarkupServices()->CreateMarkupPointer( & pPointer ));
    IFC( GetMarkupServices()->CreateMarkupPointer( & pPointer2 ));

    IFC( pSegmentList->GetSegmentCount( & iSegmentCount, & eType ));
    if ( iSegmentCount > 0 )
    {
        hr = THR( pSegmentList->MovePointersToSegment ( 0, pPointer, pPointer2));
        AssertSz( hr == 0 , "Unable to position pointers");
    }

    //
    // What we do here is we clear all text selection segments - BEFORE calling the trackers
    // this is done as we can crash during invalidation on deletion of segments
    //
    IFC( GetViewServices()->GetCurrentSelectionRenderingServices( & pSelRenSvc ));
    IFC( pSelRenSvc->ClearSegments ( TRUE ));

    if ( fChangeTrackerAndSetRange )
    {
        //
        // After the delete the BOL'ness maybe invalid. Force a recalc.
        //
        if(_pActiveTracker)
            _pActiveTracker->SetRecalculateBOL( TRUE);    

        IFC( CreateTrackerForType( pdwFollowUpAction, SELECTION_TYPE_Caret, pPointer, pPointer2 ) );
    }
    
    if( fHideCaret )
    {
        if ( fChangeTrackerAndSetRange )
        {
            Assert( GetSelectionType() == SELECTION_TYPE_Caret );
            DYNCAST( CCaretTracker, _pActiveTracker )->SetCaretShouldBeVisible( FALSE );            
        }
        else
            CCaretTracker::SetCaretVisible( GetDoc(), FALSE);
    }

Cleanup:
    ReleaseInterface( pPointer );
    ReleaseInterface( pPointer2 );
    ReleaseInterface( pSegmentList );
    ReleaseInterface( pSelRenSvc );
    RRETURN ( hr );
}

void
CSelectionManager::SetPendingFollowUpCode ( DWORD pendingFollowUp )
{
    _pendingFollowUp = pendingFollowUp;
}

//+====================================================================================
//
// Method: OnTimerTick
//
// Synopsis: Callback from Trident - for WM_TIMER messages. Route these to the tracker (if any).
//
//------------------------------------------------------------------------------------

HRESULT
CSelectionManager::OnTimerTick(DWORD* pdwFollowUpActionCode )
{
    HRESULT hr = S_OK;

    TRACKER_NOTIFY eNotify = TN_NONE;

    if ( _pendingFollowUp == FOLLOW_UP_ACTION_None )
    {
        if ( _pActiveTracker )
        {
            hr = _pActiveTracker->Notify( TN_TIMER_TICK, NULL, pdwFollowUpActionCode, & eNotify );

            if ( eNotify != TN_NONE)
                hr = TrackerNotify( eNotify, NULL, pdwFollowUpActionCode );
        }
        else
        {
            GetViewServices()->StopHTMLEditorDblClickTimer();
        }
    }
    else
    {
        //
        // BUGBUG - we don't need this anymore. Get rid of all this HOKEY stuff.
        //
        //

        // What is this here for ? It allows the Tracker to Request a state change to Caret
        // and kill itself while setting a pending follow up code - thats sent back to triden
        // on the next tick.
        //
        TraceTag((tagSelectionMgrState, "CSelectionManager::OnTimerTick - not invoking OnTimer - passing code back to Trident:%d",_pendingFollowUp ));

        *pdwFollowUpActionCode = _pendingFollowUp;
        TraceTag((tagSelectionMgrState, "SelectionManager::OnTimerTick FollowUpCode of :%d sent to trident OnTimerTick",_pendingFollowUp ));

        _pendingFollowUp = 0;
        hr = S_OK ;
    }

    RRETURN1 ( hr, S_FALSE );
}




//+====================================================================================
//
// Method: HandleChar
//
// Synopsis:
//
//------------------------------------------------------------------------------------

HRESULT
CSelectionManager::HandleCaptureChanged( SelectionMessage * pMessage )
{
    HRESULT hr = S_OK;

    //
    // We should do a state change here. Have to understand how the capture changes.
    // we need to either kill the selection, or transition to the caret
    //
    TraceTag((tagSelectionMgrState, "SelectionManager - HandleCapture." ));

    RRETURN ( hr );
}


//+====================================================================================
//
// Method: DeleteSelection
//
// Synopsis: Do the "brain dead" deletion of the Selection by firing IDM_DELETE
//
//------------------------------------------------------------------------------------

HRESULT
CSelectionManager::DeleteSelection(BOOL fAdjustPointersBeforeDeletion )
{
    HRESULT hr = S_OK;
    Assert( _fContextEditable );

    IHTMLViewServices * pViewServices = GetViewServices();
    IMarkupServices * pMarkupServices = GetMarkupServices();
    ISegmentList * pSegmentList = NULL;
    int iSegmentCount = 0;
    SELECTION_TYPE eSelectionType;
    IMarkupPointer* pStart = NULL;
    IMarkupPointer* pEnd = NULL;
    int i ;
    CUndoUnit undoUnit(GetEditor());

    //
    // Do the prep work
    //
    IFC( pViewServices->GetCurrentSelectionSegmentList( &pSegmentList ));
    IFC( pSegmentList->GetSegmentCount( & iSegmentCount, & eSelectionType ) );
    Assert( ( iSegmentCount != 0 ) && ( eSelectionType == SELECTION_TYPE_Selection) );

    IFC( pMarkupServices->CreateMarkupPointer( & pStart ) );
    IFC( pMarkupServices->CreateMarkupPointer( & pEnd ) );

    IFC( undoUnit.Begin(IDS_EDUNDOTEXTDELETE) );

    //
    // Delete the segments
    //

    for ( i = 0; i < iSegmentCount; i++ )
    {
        IFC(MovePointersToSegmentHelper(GetViewServices(), pSegmentList, i, &pStart, &pEnd));

        //
        // Cannot delete or cut unless the range is in the same flow layout
        //
        if ( PointersInSameFlowLayout( pStart, pEnd, NULL, pViewServices ) )
        {
            IFC( GetEditor()->Delete( pStart, pEnd, fAdjustPointersBeforeDeletion ));
        }
    }

    IFC( EmptySelection() );

Cleanup:
        
    ReleaseInterface( pSegmentList );
    ReleaseInterface( pStart );
    ReleaseInterface( pEnd );
    RRETURN( hr );
}

IHTMLDocument2*
CSelectionManager::GetDoc()
{
    return _pEd->GetDoc();
}


//+====================================================================================
//
// Method:
//
// Synopsis: An Event has occurred, which may require changing of the current tracker,
//           poll all your trackers, and see if any require a change.
//
//           If any trackers require changing, end them, and start the new tracker.
//
//------------------------------------------------------------------------------------


HRESULT
CSelectionManager::ShouldChangeTracker(
    SelectionMessage *pMessage ,
    DWORD * pdwFollowUpAction,
    BOOL *pfStarted)
{
    ELEMENT_TAG_ID      eTag;
    TRACKER_NOTIFY      eNotify                 = TN_NONE ;
    HRESULT             hr                      = S_OK;
    BOOL                fGoActive               = FALSE;
    BOOL                fInMove                 = FALSE;
    BOOL                fInResize               = FALSE;
    BOOL                fStarted                = FALSE;
    IHTMLElement*       pIElement               = NULL;
    IHTMLElement*       pIEditableElement       = NULL;
    IHTMLElement*       pISelectThisElement     = NULL;
    HRESULT             hrEqual                 = S_FALSE;
    BOOL fOverSiteSelectedTable = FALSE;    
    DWORD               dwTCFlags;
    //
    // This handles Shift Clicking to extend the selection
    //
    if ( ( GetSelectionType() == SELECTION_TYPE_Selection )
         && IsShiftKeyDown() )
    {
        goto Cleanup;
    }


    IFC ( CEditTracker::GetElementAndTagIdFromMessage(
                                                                pMessage,
                                                                & pIElement,
                                                                & eTag,
                                                                GetDoc(),
                                                                GetViewServices() ,
                                                                GetMarkupServices() ) );
    //
    // If we have an adorner, and we clicked on a resize or a move area
    // handle - we need to end it, and tell the control tracker to go active immediately
    //

    if ( pMessage && _pAdorner )
    {
        fInResize = _pAdorner->IsInResizeHandle( pMessage->pt, NULL );
        if ( ! fInResize )
            fInMove = _pAdorner->IsInMoveArea( pMessage->pt);

        if ( fInMove || fInResize )
        {
            DestroyAdorner();
            _fHadGrabHandles = FALSE;
            fGoActive = TRUE;

            //
            // We are transitioning from a UI Active tracker, to begin a move/resize
            // If we have a text selection - we whack it - or else we will end up dragging
            // the text (and mess-up access which uses the type of selection)
            // IE 5 Bug 39974
            //
            if ( GetSelectionType() == SELECTION_TYPE_Selection )
                EmptySelection(FALSE);
        }
    }

    //
    // We special case mouse clicks over site selected tables. The code below could almost cover
    // the general over a site selected control test - except for glyphs. !
    //

    if (  _pActiveTracker  && ( _pActiveTracker->GetSelectionType() == SELECTION_TYPE_Control ) )
    {
        ELEMENT_TAG_ID eControlTag;
        CControlTracker* pControlTrack = DYNCAST( CControlTracker, _pActiveTracker );
        IFC( GetMarkupServices()->GetElementTagId( pControlTrack->GetControlElement(), & eControlTag))
        if ( eControlTag == TAGID_TABLE )
        {
            fOverSiteSelectedTable = pControlTrack->IsMessageOverControl(pMessage);
        }
    }

    if ( fOverSiteSelectedTable )
    {
        fStarted = FALSE;
        goto Cleanup;
    }
    
    if ( IsParentEditable() &&
        ( CControlTracker::ShouldStartTracker( this, pMessage, eTag, pIElement, & pISelectThisElement )) )
    {
        //
        // BUGBUG - this is kind of messy, and loses the "general-ness" of having abstract trackers
        //

        //
        // If we alerady have a Control Tracker, only create a new one if you click on a different element
        //
        BOOL fShouldStart = TRUE;
        if  ( _pActiveTracker  && ( _pActiveTracker->GetSelectionType() == SELECTION_TYPE_Control ) )
        {
            CControlTracker* pControlTrack = DYNCAST( CControlTracker, _pActiveTracker );
            fShouldStart = ! pControlTrack->IsSameElementAsControl( pISelectThisElement ) ;
            if ( ! fShouldStart )
            {
                //
                // If we already have a control tracker, and we don't want to start another one.
                // Bail.
                //
                fStarted = FALSE;
                goto Cleanup;
            }
        }

        //
        // If we still want to create a control tracker - ensure that it's not the same
        // as the Editable Element - IF we _fHadGrabHandles.
        //
        IFC( GetEditableElement( & pIEditableElement));
        if ( fShouldStart && _fHadGrabHandles )
        {
            hrEqual = GetViewServices()->IsEqualElement( pIEditableElement, pISelectThisElement );

            fShouldStart = ( hrEqual == S_FALSE ) || !HasFocusAdorner() ;
            if ( fShouldStart )
            {
                //
                // _fGrabHandles indicates that a control had grab handles. We now reset it
                // as we're changing the element that has/will have grab handles
                //
                _fHadGrabHandles = FALSE;
            }
            //else
            //{
            //
            // We clicked in a UI Active Control. We don't want to make a Site Selection
            // so we go on to make a Normal Selection.
            //
            //}
        }

        if ( fShouldStart )
        {
            if ( fGoActive )
            {
                //
                // if fGoActive means we are transitioning from a UI Active tracker
                // to a SiteSelected tracker, and will allow dragging/resizing.
                // We need to force a currency change here ( important for OLE Controls )
                // BUT We don't want to whack any trackers on the set edit context
                //
                SetDontChangeTrackers( TRUE );

                IGNORE_HR( GetViewServices()->MakeParentCurrent( pIEditableElement  ));
                SetDontChangeTrackers( FALSE );

                if ( !fInMove  )
                {
                    //
                    // More specialness for going from UI-Active to a live resizing site selection
                    // we need to listed for the AdornerPositioned Notification from the tracker
                    // to then tell the tracker to to go "live"
                    //
                    _fPendingAdornerPosition = TRUE;
                    _lastMessage = *pMessage;
                }
            }

            EndCurrentTracker( pdwFollowUpAction, pMessage );
            _pActiveTracker = new CControlTracker( this );
            if (!_pActiveTracker)
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }
            dwTCFlags = 0;
            if (fInMove)
            {
                dwTCFlags |= TRACKER_CREATE_ACTIVEONMOVE;
            }
            if (fGoActive)
            {
                dwTCFlags |= TRACKER_CREATE_GOACTIVE;
            }
            hr = _pActiveTracker->Init2( this, pMessage, pdwFollowUpAction, & eNotify, dwTCFlags );

            fStarted = TRUE;
        }
    }

    if ( ! fStarted )
    {
        if ( CSelectTracker::ShouldStartTracker ( this, pMessage, eTag, pIElement  ) &&
                ( pMessage->message != WM_RBUTTONDOWN  ) &&
                     (  CControlTracker::IsThisElementSiteSelectable( 
                            this ,
                            eTag, 
                            pIElement ) && 
                        _pActiveTracker && 
                        GetSelectionType() == SELECTION_TYPE_Selection ? 
                            _pActiveTracker->FireOnBeforeEditFocus( pIElement ) :  // If we're over somehting site selectable - fire the event on that
                            FireOnBeforeEditFocus() ) )
        {
            EndCurrentTracker( pdwFollowUpAction, pMessage );
#ifdef UNIX
            if (pMessage->message != WM_MBUTTONDOWN)
#endif
            _pActiveTracker = new CSelectTracker( this );
            if (!_pActiveTracker)
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }
            hr = _pActiveTracker->Init2(this, pMessage, pdwFollowUpAction, & eNotify);
            fStarted = TRUE;

        }
    }

    if ( !_pActiveTracker  && !_fNoScope )
    {
        _pActiveTracker = new CCaretTracker( this );
        if (!_pActiveTracker)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
        hr = _pActiveTracker->Init2(this, pMessage, pdwFollowUpAction, & eNotify);
        fStarted = TRUE;

    }

    if ( eNotify != TN_NONE )
        hr = TrackerNotify( eNotify, pMessage, pdwFollowUpAction );

Cleanup:
    if ( pfStarted )
        *pfStarted = fStarted;

    ReleaseInterface( pISelectThisElement);
    ReleaseInterface( pIElement );
    ReleaseInterface( pIEditableElement );
    RRETURN1( hr, S_FALSE );

}

HRESULT
CSelectionManager::CreateTrackerForType(
                DWORD* pdwFollowUpAction,
                SELECTION_TYPE eType ,
                IMarkupPointer* pStart,
                IMarkupPointer* pEnd,
                DWORD dwTCFlagsIn,
                CARET_MOVE_UNIT inLastCaretMove,
                BOOL fSetTCFromActiveTracker /*= TRUE*/)
{
    HRESULT         hr              = S_OK;
    TRACKER_NOTIFY  eNotify         = TN_NONE ;
    DWORD           dwTCFlags = dwTCFlagsIn ;

    if ( fSetTCFromActiveTracker )
        SetTCForActiveTrackerBOL( & dwTCFlags );
    
    //
    // BUGBUG - this will have to change - if we get to doing CUSTOM trackers
    //

    EndCurrentTracker( pdwFollowUpAction, NULL, TRUE );
    switch ( eType )
    {
        case SELECTION_TYPE_Caret:
            Assert ( ! _fNoScope );
            _pActiveTracker = new CCaretTracker( this );
            break;

        case SELECTION_TYPE_Selection:
            Assert( ! _fNoScope );
            _pActiveTracker = new CSelectTracker( this );
            break;

        case SELECTION_TYPE_Control:
            _pActiveTracker = new CControlTracker( this );
            break;
    }
    if (!_pActiveTracker)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = _pActiveTracker->Init2(this, pStart, pEnd, NULL , & eNotify, dwTCFlags, inLastCaretMove );

    if ( eNotify != TN_NONE )
        IGNORE_HR( TrackerNotify( eNotify, NULL, NULL  ));

Cleanup:
    RRETURN ( hr );
}

//+====================================================================================
//
// Method: SetTCForActiveTrackerBOL
//
// Synopsis: Examine the current active tracker - and grab it's BOL/Logical BOL
//           Set these to TrackerCreation Codes 
//
//------------------------------------------------------------------------------------

VOID
CSelectionManager::SetTCForActiveTrackerBOL( DWORD* pdw )
{
    BOOL            fNotAtBOL       = TRUE;
    BOOL            fAtLogicalBOL   = FALSE;

    Assert( pdw );
    
    if( _pActiveTracker )
    {
        fNotAtBOL = _pActiveTracker->GetNotAtBOL();
        fAtLogicalBOL = _pActiveTracker->GetAtLogicalBOL();
    }

    if (fNotAtBOL)
    {
        *pdw |= TRACKER_CREATE_NOTATBOL;
    }
    
    if (fAtLogicalBOL)
    {
        *pdw |= TRACKER_CREATE_ATLOGICALBOL;
    }
}

//+====================================================================================
//
// Method: StartSelectionFromShift
//
// Synopsis:
//
//------------------------------------------------------------------------------------

HRESULT
CSelectionManager::StartSelectionFromShift(
                SelectionMessage *pMessage,
                DWORD* pdwFollowUpAction,
                TRACKER_NOTIFY * peNotify )
{
    HRESULT hr = S_OK;

    IMarkupPointer* pStartCaret = NULL;
    IMarkupPointer* pEndCaret = NULL;
    long eHTMLDir = htmlDirLeftToRight;
    SP_IHTMLCaret spCaret;
    DWORD dwCode = TRACKER_CREATE_STARTFROMSHIFTKEY;
    CARET_MOVE_UNIT eCaretMove = CARET_MOVE_NONE;
    POINT ptGlobal;
    Assert( GetSelectionType() == SELECTION_TYPE_Caret );
    CCaretTracker* pCaretTracker = DYNCAST( CCaretTracker, _pActiveTracker );
    BOOL fNotAtBOL = pCaretTracker->GetNotAtBOL();
    BOOL fAtLogicalBOL = pCaretTracker->GetAtLogicalBOL();

    IFC( GetMarkupServices()->CreateMarkupPointer( & pStartCaret ));
    IFC( GetMarkupServices()->CreateMarkupPointer( & pEndCaret ));
    IFC( GetViewServices()->GetCaret( &spCaret ));
    IFC( spCaret->MovePointerToCaret( pStartCaret ));
    IFC( spCaret->MovePointerToCaret( pEndCaret ));
    IFC( spCaret->GetNotAtBOL( &fNotAtBOL ));
    fAtLogicalBOL = ! fNotAtBOL;
    IFC( spCaret->GetLocation( & ptGlobal , fNotAtBOL ));
    IFC( GetViewServices()->GetLineDirection( pStartCaret, FALSE, &eHTMLDir ));
    eCaretMove = CCaretTracker::GetMoveDirectionFromMessage( pMessage, (eHTMLDir == htmlDirRightToLeft));
 
    if ( FAILED( pCaretTracker->MovePointer( eCaretMove, pEndCaret, & ptGlobal.x, &fNotAtBOL, &fAtLogicalBOL, NULL )))
    {
        //
        // BUGBUG - better to explicitly trap for the fail condition of no next layout ?
        //
        if ( pCaretTracker->GetPointerDirection(eCaretMove) == RIGHT)
        {
            IGNORE_HR(  pCaretTracker->MovePointer( CARET_MOVE_LINEEND, pEndCaret, & ptGlobal.x, &fNotAtBOL, &fAtLogicalBOL, NULL ));
        }
        else
        {
            IGNORE_HR(  pCaretTracker->MovePointer( CARET_MOVE_LINESTART, pEndCaret, & ptGlobal.x, &fNotAtBOL, &fAtLogicalBOL, NULL ));     
        }    
    }

    if (fNotAtBOL)
    {
        dwCode |= TRACKER_CREATE_NOTATBOL;
    }
    
    if (fAtLogicalBOL)
    {
        dwCode |= TRACKER_CREATE_ATLOGICALBOL;
    }

    IFC( GetViewServices()->ScrollPointerIntoView( pEndCaret, fNotAtBOL, POINTER_SCROLLPIN_Minimal ));

    CreateTrackerForType(   pdwFollowUpAction,
                            SELECTION_TYPE_Selection,
                            pStartCaret,
                            pEndCaret,
                            dwCode,
                            eCaretMove,
                            FALSE );

Cleanup:
    ReleaseInterface( pStartCaret );
    ReleaseInterface( pEndCaret );
    RRETURN1( hr, S_FALSE );
}


//+====================================================================================
//
// Method: Notify
//
// Synopsis: This is the internal notify - used between the manager and the trackers.
//              Something has happened that the SelectionManager needs to know about
//
//------------------------------------------------------------------------------------


HRESULT
CSelectionManager::TrackerNotify(
    TRACKER_NOTIFY inNotify,
    SelectionMessage *pMessage,
    DWORD* pdwFollowUpAction )
{
    CSpringLoader * psl;
    HRESULT         hr                      = S_OK;
    IMarkupPointer* pPointerSpringLoader    = NULL;
    BOOL            fStarted                = FALSE;
    TRACKER_NOTIFY  eNotify                 = TN_NONE;

    WHEN_DBG( DumpNotify( eNotify, TRUE));
    switch ( inNotify )
    {
        case TN_KILL_ADORNER:
        {
            DestroyAdorner();
        }
        break;

        case TN_END_TRACKER_RESELECT:
        {
            EndCurrentTracker(pdwFollowUpAction, pMessage);
            ShouldChangeTracker( pMessage, pdwFollowUpAction, & fStarted );
        }
        break;

        case TN_END_TRACKER_POS_CONTROL:
        {
#if DBG ==1
            Assert( _pStartTemp && _pEndTemp);
            BOOL fPositioned = FALSE;
            IGNORE_HR( _pStartTemp->IsPositioned( & fPositioned));
            Assert( fPositioned );
            IGNORE_HR( _pEndTemp->IsPositioned( & fPositioned));
            Assert( fPositioned );
#endif

            if ( _pStartTemp && _pEndTemp )
            {
                CreateTrackerForType( 
                                pdwFollowUpAction ,
                                SELECTION_TYPE_Control,
                                _pStartTemp,
                                _pEndTemp );
            }
            
            if (( eNotify != TN_END_TRACKER_POS_CARET ) && 
                ( eNotify != TN_END_TRACKER_POS_CARET_REBUBBLE ) && 
                ( eNotify != TN_END_TRACKER_POS_SELECT ) )
               ReleaseTempMarkupPointers();
        }
        break;

        case TN_END_TRACKER_POS_CARET:
        case TN_END_TRACKER_POS_CARET_REBUBBLE:
        {
#if DBG ==1
            Assert( _pStartTemp && _pEndTemp);
            BOOL fPositioned = FALSE;
            IGNORE_HR( _pStartTemp->IsPositioned( & fPositioned));
            Assert( fPositioned );
            IGNORE_HR( _pEndTemp->IsPositioned( & fPositioned));
            Assert( fPositioned );
#endif

            if ( _pStartTemp && _pEndTemp )
            {
                CreateTrackerForType( 
                    pdwFollowUpAction ,
                    SELECTION_TYPE_Caret,
                    _pStartTemp,
                    _pEndTemp );
            }
            
            if (( eNotify != TN_END_TRACKER_POS_CONTROL ) && 
                ( eNotify != TN_END_TRACKER_POS_SELECT ) )
               ReleaseTempMarkupPointers(); 

            if (    inNotify == TN_END_TRACKER_POS_CARET_REBUBBLE

                // BUGBUG (MohanB) This should be cleaned up by the Edit team
                 || (pMessage && pMessage->message == WM_KEYDOWN && pMessage->wParam == VK_ESCAPE))
            {
                hr = THR( HandleMessage( pMessage, pdwFollowUpAction ));
            }

            EnsureAdornment();
        }
        break;

        case TN_END_TRACKER_POS_SELECT:
        {
#if DBG ==1
            Assert( _pStartTemp && _pEndTemp);
            BOOL fPositioned = FALSE;
            IGNORE_HR( _pStartTemp->IsPositioned( & fPositioned));
            Assert( fPositioned );
            IGNORE_HR( _pEndTemp->IsPositioned( & fPositioned));
            Assert( fPositioned );
#endif
            DWORD dwCode = TRACKER_CREATE_STARTFROMSHIFTKEY; // BUGBUG - correct for all the places POS_SELECT is used.
            if ( _pStartTemp && _pEndTemp )
            {
                CreateTrackerForType( 
                                pdwFollowUpAction,
                                SELECTION_TYPE_Selection,
                                _pStartTemp,
                                _pEndTemp,
                                dwCode );
            }
            
            if (( eNotify != TN_END_TRACKER_POS_CARET ) && 
                ( eNotify != TN_END_TRACKER_POS_CARET_REBUBBLE ) && 
                ( eNotify != TN_END_TRACKER_POS_CONTROL ) )
               ReleaseTempMarkupPointers();
        }
        break;

        case TN_END_TRACKER_SHIFT_START:
        {
            hr = StartSelectionFromShift( pMessage, pdwFollowUpAction, & eNotify );
        }
        break;

        //
        // Only use this if you know what you're doing.
        //
        // BUGBUG - There may be places where we should create a CaretTracker here.
        //
        case TN_END_TRACKER:
        {
            EndCurrentTracker(pdwFollowUpAction, pMessage) ;
        }
        break;

        case TN_END_TRACKER_CREATE_CARET:
        {
            EndCurrentTracker(pdwFollowUpAction, pMessage) ;
            _pActiveTracker = new CCaretTracker( this );
            if (!_pActiveTracker)
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }
            hr = _pActiveTracker->Init2( this, pMessage, pdwFollowUpAction, & eNotify );
        }
        break;

        case TN_END_TRACKER_REBUBBLE_ADORN:
        {
            //
            // This works as this is used for commands where there's already a tracker.
            // eg. Selection has got a char - fires a delete - delete kills selection,
            // we bubble to here, and then we fire the char to the CCaretTracker.
            //
            //
            if ( inNotify == TN_END_TRACKER_REBUBBLE_ADORN )
            {
                if ( _pAdorner )
                    DestroyAdorner();

                if ( ShouldElementShowUIActiveBorder() )
                {
                    IGNORE_HR( CreateAdorner() );
                }
            }
        }
        break;

        case TN_HIDE_CARET:
            //
            // This is sort of weird. The Caret has ended already. But it may
            // be visible. We now make sure it isn't.
            //
            CCaretTracker::SetCaretVisible( GetDoc(), FALSE );
            break;


        case TN_FIRE_DELETE:

            // Springload formats at start of selection (Word behavior).
            if (_pActiveTracker && _pActiveTracker->GetSelectionType() == SELECTION_TYPE_Selection)
            {
                CSelectTracker * pSelectTrack = DYNCAST( CSelectTracker, _pActiveTracker );

                psl = _pEd->GetPrimarySpringLoader();
                if (psl && pSelectTrack->GetStartSelection())
                    IFC( psl->SpringLoad( pSelectTrack->GetStartSelection(), SL_ADJUST_FOR_INSERT_LEFT | SL_TRY_COMPOSE_SETTINGS | SL_RESET ));
            }
            IFC( DeleteSelection( TRUE ));
            if ( IsPendingUndo())
            {
                IFC( GetMarkupServices()->EndUndoUnit());
                SetPendingUndo( FALSE );
            }
            break;

            //
            // Used for "typing into" a selection.
            //
        case TN_FIRE_DELETE_REBUBBLE:
            {
                CUndoUnit           undoUnit(GetEditor());
                SP_IMarkupPointer   spSelectionStart;
                
                IFC( undoUnit.Begin(IDS_EDUNDOTYPING));

                // Springload formats at start of selection (Word behavior).
                if (_pActiveTracker && _pActiveTracker->GetSelectionType() == SELECTION_TYPE_Selection)
                {
                    CSelectTracker  * pSelectTrack = DYNCAST( CSelectTracker, _pActiveTracker );
                    IFC( pSelectTrack->AdjustPointersForChar() );
                    IFC( CopyMarkupPointer(GetMarkupServices(), pSelectTrack->GetStartSelection(), &spSelectionStart) );
                }


                IFC( DeleteSelection( FALSE ));

                // Make sure we move into an adjacent url
                Assert( GetSelectionType() == SELECTION_TYPE_Caret && spSelectionStart != NULL);
                if (GetSelectionType() == SELECTION_TYPE_Caret && spSelectionStart != NULL)
                {
                    CEditPointer        epTest(GetEditor());
                    DWORD               dwSearch = DWORD(BREAK_CONDITION_ANYTHING);
                    DWORD               dwFound;
                    CCaretTracker       *pCaretTracker = DYNCAST(CCaretTracker, _pActiveTracker);
                    
                    IFC( epTest->MoveToPointer(spSelectionStart) );

                    IFC( epTest.Scan(LEFT, dwSearch, &dwFound) );
                    if (!epTest.CheckFlag(dwFound, BREAK_CONDITION_EnterAnchor))
                    {
                        IFC( epTest->MoveToPointer(spSelectionStart) );
                        IFC( epTest.Scan(RIGHT, dwSearch, &dwFound) );
                    }

                    BOOL fBetweenLines = FALSE;                        
                    

                    if (epTest.CheckFlag(dwFound, BREAK_CONDITION_EnterAnchor))
                    {
                        IFC( GetViewServices()->IsPointerBetweenLines( epTest, & fBetweenLines));                    
                        IFC( pCaretTracker->PositionCaretAt( epTest, ! fBetweenLines, fBetweenLines, CARET_DIRECTION_INDETERMINATE, POSCARETOPT_None, ADJPTROPT_None ));
                    }                   
                    else 
                    {
                        IFC( GetViewServices()->IsPointerBetweenLines( spSelectionStart, & fBetweenLines));                    
                        IFC( pCaretTracker->PositionCaretAt( 
                                spSelectionStart, 
                                !fBetweenLines, 
                                fBetweenLines , 
                                CARET_DIRECTION_INDETERMINATE, 
                                fBetweenLines ? POSCARETOPT_None : POSCARETOPT_DoNotAdjust, 
                                ADJPTROPT_DontExitPhrase | ADJPTROPT_AdjustIntoURL));
                    }
                }

                // We need to stay 
                
                hr = THR( HandleMessage( pMessage, pdwFollowUpAction ));
            }
            break;

        default:
            _pActiveTracker->Notify( inNotify, pMessage, pdwFollowUpAction, & eNotify );

    }
Cleanup:
    ReleaseInterface( pPointerSpringLoader );
    WHEN_DBG( DumpNotify( eNotify, FALSE ));
    //
    // BUGBUG - make sure we don't get into recursion here.
    //
    if ( eNotify != TN_NONE )
        hr = TrackerNotify( eNotify, pMessage, pdwFollowUpAction );


    RRETURN1( hr , S_FALSE );
}


//+====================================================================================
//
// Method: EndCurrentTracker
//
// Synopsis: End the current tracker. - telling it it has ended.
//
//           If we're told to hide the caret, and the tracker was a caret , then we hide it.
//
//------------------------------------------------------------------------------------


VOID
CSelectionManager::EndCurrentTracker(   DWORD *pdwFollowUpAction,
                                        SelectionMessage* pMessage ,
                                        BOOL fHideCaret,
                                        BOOL fClear
                                        )
{
    SELECTION_TYPE myType = GetSelectionType();

    if ( _pActiveTracker )
    {
        CEditTracker * pTracker = _pActiveTracker;

        if ( _fInTimer && GetSelectionType() == SELECTION_TYPE_Selection )
        {
            DYNCAST( CSelectTracker, _pActiveTracker )->StopTimer();
        }

        if ( _fInCapture )
        {
            Assert( _pActiveTracker );
            _pActiveTracker->ReleaseCapture();
        }
  
        _pActiveTracker = NULL;

        if ( fClear )
            pTracker->Notify( TN_END_TRACKER , pMessage , pdwFollowUpAction , NULL  );
        else
            pTracker->Notify( TN_END_TRACKER_NO_CLEAR, pMessage, pdwFollowUpAction, NULL );

        pTracker->Release();
    }
    if ( ( fHideCaret ) && ( myType == SELECTION_TYPE_Caret ))
    {
        CCaretTracker::SetCaretVisible( GetDoc() , FALSE );
    }

    //
    // After we kill ANY tracker, we better not have capture or a timer.
    //
    Assert( !_fInTimer );
    Assert( !_fInCapture );
}

SELECTION_TYPE
CSelectionManager::GetSelectionType ( )
{

    if ( _pActiveTracker )
        return _pActiveTracker->GetSelectionType();
    else
        return SELECTION_TYPE_None;

}

HRESULT
CSelectionManager::GetSelectionType (
    SELECTION_TYPE * peSelectionType )
{
    HRESULT hr = S_OK;

    Assert( peSelectionType );

    if ( peSelectionType )
    {
        *peSelectionType = GetSelectionType();
    }
    else
        hr = E_INVALIDARG;

    RRETURN ( hr );
}


//+====================================================================================
//
// Method: Notify
//
// Synopsis: This is the 'external' notify - used by Trident to tell the selection manager
//           that 'something' has happened that it should do soemthing about.
//
//------------------------------------------------------------------------------------


HRESULT
CSelectionManager::Notify(
    SELECTION_NOTIFICATION eSelectionNotification,
    IUnknown* pUnknown,
    DWORD* pdwFollowUpActionFlag,
    DWORD dword )
{
    HRESULT hr = S_OK;

    switch( eSelectionNotification )
    {
        case SELECT_NOTIFY_CARET_IN_CONTEXT:
            hr = CaretInContext(pdwFollowUpActionFlag);
            break;

        case SELECT_NOTIFY_TIMER_TICK:
            hr = OnTimerTick( pdwFollowUpActionFlag );
            break;

        case SELECT_NOTIFY_EMPTY_SELECTION:
            hr = EmptySelection( pdwFollowUpActionFlag , TRUE );
            break;

        case SELECT_NOTIFY_DESTROY_SELECTION:
            if ( ! IsDontChangeTrackers() )
            {
                hr = DestroySelection(pdwFollowUpActionFlag);
            }
            break;

        case SELECT_NOTIFY_EXIT_TREE:
            {
                if ( ! IsIgnoreExitTree() )
                    hr = ExitTree( pdwFollowUpActionFlag, pUnknown );

            }
            break;

        case SELECT_NOTIFY_DOC_ENDED:
        case SELECT_NOTIFY_DESTROY_ALL_SELECTION:       

            if ( _pActiveTracker )
            {
                if ( ! _pActiveTracker->IsInFireOnSelectStart() )
                    EndCurrentTracker( pdwFollowUpActionFlag, 
                                        NULL, 
                                        (eSelectionNotification == SELECT_NOTIFY_LOSE_FOCUS_FRAME ) ? TRUE : FALSE, 
                                        TRUE );  
                else
                {
                    //
                    // We're unloading during firing of OnSelectStart
                    // We want to not kill the tracker now - but fail the OnSelectStart
                    // resulting in the Tracker dieing gracefully.
                    //
                    _pActiveTracker->SetFailFireOnSelectStart( TRUE );
                }
            }                                
            if ( eSelectionNotification == SELECT_NOTIFY_DOC_ENDED )
            {
                SetDrillIn( FALSE, NULL);
                SetNextMessage( NULL );
            }
            break;
            
        case SELECT_NOTIFY_LOSE_FOCUS_FRAME: 
            //
            // Selection is being made in another frame/instance of trident
            // we just kill ourselves.
            //
            hr = LoseFocusFrame( pdwFollowUpActionFlag, dword );
            
            break;

        case SELECT_NOTIFY_LOSE_FOCUS:
            _fLastMessageValid = FALSE;
            hr = LoseFocus();
            break;

        case SELECT_NOTIFY_DISABLE_IME:
            hr = TerminateIMEComposition(TERMINATE_NORMAL, NULL, pdwFollowUpActionFlag );
            break;
    }

    RRETURN1 ( hr, S_FALSE );
}



//+====================================================================================
//
// Method: LoseFocusFrame
//
// Synopsis: Do some work to end the current tracker, and to create/destroy adorners
//           for Iframes
//
//------------------------------------------------------------------------------------

HRESULT
CSelectionManager::LoseFocusFrame(DWORD* pdwFollowUpAction, DWORD selType )
{
    HRESULT hr = S_OK;
    
    EndCurrentTracker(pdwFollowUpAction, NULL, TRUE , TRUE );

    //
    // Do some extra work for IFrames.
    //
    if ( selType == START_CONTROL_SELECTION)
    {
        if ( _pAdorner && GetEditableTagId() == TAGID_IFRAME  )      
            DestroyAdorner();
    }
    else if ( selType == START_TEXT_SELECTION )
    {
        //
        // I'm in a UIActive control - that doesn't have an adorner
        // Special case Adorner Creation for Iframes. Why ? because the above would
        // have destroyed it
        //
        // Only if this is an Iframe - can we guess that it's ok to create the adorner.
        // otherwise the adorner should be already created around the element that's we're making
        // the selection in.
        // Why all this bizarreitude ? Because IFrames are in one document - and their inner 
        // document is in another
        // 
        if ( !_pAdorner && 
             ShouldElementShowUIActiveBorder()  && 
             GetEditableTagId() == TAGID_IFRAME  )
        {
            hr = THR( CreateAdorner() ) ;
            _pAdorner->InvalidateAdorner() ;
        }
    }

    RRETURN( hr );
}

//+====================================================================================
//
// Method: ExitTree
//
// Synopsis: An element with the _fAdorned bit set has left the tree. Check to see
//           if we have a Site Selection for it - and tear it down if necessary.
//
//------------------------------------------------------------------------------------

HRESULT
CSelectionManager::ExitTree( DWORD * pdw, IUnknown* pUnknown)
{
    HRESULT hr = S_OK;
    IHTMLElement* pIElement = NULL;
    IMarkupPointer* pPointer = NULL;
    IMarkupPointer* pPointer2 = NULL;
    CEditPointer startPointer( GetEditor() );
    CEditPointer endPointer( GetEditor())  ;
    ISelectionRenderingServices* pSelRenSvc = NULL;
    CControlTracker* pControlTrack = NULL;
    ELEMENT_TAG_ID eTag = TAGID_NULL;
    IHTMLElement* pIOuterElement = NULL;
    IHTMLElement* pISiteSelectThisElement = NULL;
    SELECTION_TYPE eSelType = SELECTION_TYPE_None ;
    IHTMLElement* pIParentElement = NULL;
    SELECTION_TYPE eType = GetSelectionType();
    int ctSegment = 0;
    
    if ( eType == SELECTION_TYPE_Control )
    {
        IFC( pUnknown->QueryInterface( IID_IHTMLElement, (void**) & pIElement ));
        IFC( GetMarkupServices()->GetElementTagId( pIElement, & eTag ));

        pControlTrack = DYNCAST( CControlTracker, _pActiveTracker );
        if ( pControlTrack->IsSameElementAsControl( pIElement ) )
        {
           IFC( GetMarkupServices()->CreateMarkupPointer( & pPointer ));
           IFC( GetMarkupServices()->CreateMarkupPointer( & pPointer2 ));        

           eSelType = SELECTION_TYPE_Caret;
           IFC( pPointer->MoveAdjacentToElement( pIElement , ELEM_ADJ_BeforeBegin ));
           IFC( pPointer2->MoveAdjacentToElement( pIElement , ELEM_ADJ_BeforeBegin ));

           hr = THR( CreateTrackerForType( pdw, eSelType, pPointer, pPointer2 ) );
        }
    }
    else
    {
        if ( _pActiveTracker )
        {
            IFC( GetViewServices()->GetCurrentSelectionRenderingServices( & pSelRenSvc ));
            IFC( pSelRenSvc->GetSegmentCount( &ctSegment, & eSelType ));

            if ( eSelType == SELECTION_TYPE_Selection || eSelType == SELECTION_TYPE_Caret )
            {
                //
                // A layout is leaving the tree. Does this completely contain selection ?
                //
                IFC( pUnknown->QueryInterface( IID_IHTMLElement, (void**) & pIElement ));
                IFC( GetMarkupServices()->CreateMarkupPointer( & pPointer ));
                IFC( GetMarkupServices()->CreateMarkupPointer( & pPointer2 )); 
                IFC( pPointer->MoveAdjacentToElement( pIElement, ELEM_ADJ_AfterBegin ));
                IFC( pPointer2->MoveAdjacentToElement( pIElement, ELEM_ADJ_BeforeEnd));

                IFC( pSelRenSvc->MovePointersToSegment(0, startPointer, endPointer ));

                if ( startPointer.Between( pPointer, pPointer2) && 
                     endPointer.Between( pPointer, pPointer2))
                {
                    IFC( pPointer->MoveAdjacentToElement( pIElement, ELEM_ADJ_AfterEnd));
                    if ( _pActiveTracker &&  _pActiveTracker->AdjustForDeletion( pPointer ) )
                    {
                        DWORD dwFollowUpAction = 0; // BUGBUG: This needs to be passed back to the doc [marka]
                        EndCurrentTracker( &dwFollowUpAction, NULL, TRUE, TRUE ); // End without clearing the selection if any
                        AssertSz(dwFollowUpAction == 0, "Dropping follow up action");
                    }                
                }
            }
        }
    }

Cleanup:
    ReleaseInterface( pIParentElement);
    ReleaseInterface( pISiteSelectThisElement);
    ReleaseInterface( pIOuterElement );
    ReleaseInterface( pIElement );
    ReleaseInterface( pPointer );
    ReleaseInterface( pPointer2 );
    ReleaseInterface( pSelRenSvc );
    RRETURN ( hr );
}


HRESULT
CSelectionManager::LoseFocus()
{
    HRESULT             hr = S_OK;
    SP_IHTMLCaret       spCaret;
    SP_IMarkupPointer   spLeft;

    IFC( GetViewServices()->GetCaret( & spCaret ));
    IFC( spCaret->LoseFocus());

    TerminateIMEComposition(TERMINATE_NORMAL);

    if( _pActiveTracker && _pActiveTracker->GetSelectionType() == SELECTION_TYPE_Caret )
    {
        IFC( GetMarkupServices()->CreateMarkupPointer( & spLeft ));
        IFC( spCaret->MovePointerToCaret( spLeft ));
        IGNORE_HR( AutoUrl_DetectCurrentWord( GetMarkupServices(), spLeft, NULL, NULL ));
    }
    
Cleanup:
    RRETURN( hr );
}



//+====================================================================================
//
// Method: CaretInContext
//
// Synopsis: Position a Caret at the start of the edit context. Used to do somehting
//           meaningful on junk, eg. calling select() on a control range that is empty
//
//------------------------------------------------------------------------------------


HRESULT
CSelectionManager::CaretInContext( DWORD* pdwFollowUpActionFlag )
{
    HRESULT hr = S_OK;

    hr = THR ( CreateTrackerForType(
                                    pdwFollowUpActionFlag,
                                    SELECTION_TYPE_Caret,
                                    _pStartContext,
                                    _pStartContext ) );

    RRETURN( hr );
}


HRESULT
CSelectionManager::DestroySelection( DWORD *pdwFollowUpActionCode)
{
    HRESULT hr = S_OK;
    IHTMLViewServices * pVS = NULL;
    ISelectionRenderingServices* pSelRenSvc = NULL;

    //
    // Ignore potential BOGUS currency changes (like from moving a control)
    //

    if ( _pActiveTracker && (  _pActiveTracker->IsActive() ))
        goto Cleanup;

    EndCurrentTracker(pdwFollowUpActionCode,  NULL, TRUE, TRUE);
    DestroyAdorner();

    //
    // Destroy any and all selections
    //

    pVS = _pEd->GetViewServices();

    hr = THR( pVS->GetCurrentSelectionRenderingServices( & pSelRenSvc ));
    if ( hr )
        goto Cleanup;

    hr = THR( pSelRenSvc->ClearSegments(TRUE));
    if ( hr )
        goto Cleanup;

    hr = THR( pSelRenSvc->ClearElementSegments());
    if ( hr )
        goto Cleanup;

Cleanup:
    ReleaseInterface( pSelRenSvc );

    RRETURN( hr );
}


//+====================================================================================
//
// Method:ShouldElementBeAdorned
//
// Synopsis: Is it valid to show a UI Active border around this element ? 
//
//------------------------------------------------------------------------------------


BOOL 
CSelectionManager::IsElementUIActivatable()
{
    if ( _fParentEditable )
    {
        return IsElementUIActivatable( GetEditableElement());
    }
    else
        return FALSE;        
}

//+====================================================================================
//
// Method:IsElementUIActivatable
//
// Synopsis: Is it valid to show a UI Active border around this element ? 
//
//------------------------------------------------------------------------------------


BOOL 
CSelectionManager::IsElementUIActivatable(IHTMLElement* pIElement)
{
    HRESULT hr = S_OK;

    BOOL fShouldBeActive = FALSE;
    ELEMENT_TAG_ID eTag = TAGID_NULL;

    IFC( GetMarkupServices()->GetElementTagId( pIElement, & eTag ));

    switch( eTag )
    {
        case TAGID_INPUT:
        case TAGID_BUTTON:
//        case TAGID_HTMLAREA:
        case TAGID_TEXTAREA:
        case TAGID_DIV:
        case TAGID_SPAN:   // Divs and Spans - because if these have the Edit COntext - they must be current.
        case TAGID_LEGEND:
        case TAGID_MARQUEE:
        case TAGID_IFRAME:
        case TAGID_SELECT:
            fShouldBeActive = TRUE;
            break;

        case TAGID_BODY:
        case TAGID_TABLE:
            goto Cleanup; // skip the positioning checks here for these common cases

        case TAGID_OBJECT:
        {
            IFC( GetViewServices()->ShouldObjectHaveBorder( pIElement, & fShouldBeActive ));
        }
        break;
    }
    
    if ( ! fShouldBeActive )
    {
        fShouldBeActive =  IsElementPositioned( pIElement );
        if ( ! fShouldBeActive )
        {
            IFC( GetViewServices()->IsElementSized( pIElement, & fShouldBeActive ));
        }                        
    }

Cleanup:

    return fShouldBeActive;
}


//+====================================================================================
//
// Method:ShouldElementBeAdorned
//
// Synopsis: Should there be a UI-Active border around the current element that has focus
//
//------------------------------------------------------------------------------------

BOOL
CSelectionManager::ShouldElementShowUIActiveBorder()
{
    BOOL fActivatable = IsElementUIActivatable( );

    BOOL fOkEditFocus = FireOnBeforeEditFocus();
    
    return fActivatable && fOkEditFocus ;
}


//+====================================================================================
//
// Method:CreateAdorner
//
// Synopsis: Create a UI Active Adorner for this type of tracker.
//
//------------------------------------------------------------------------------------


HRESULT
CSelectionManager::CreateAdorner()
{
    HRESULT hr = S_OK;
    Assert( ! _pAdorner );
    IHTMLElement * pIElement = NULL;
    BOOL fLocked;

    AssertSz( _fEditFocusAllowed , " UI Activation of Control not allowed" );
    
    hr = GetEditableElement( & pIElement );
    if ( hr )
        goto Cleanup;

    IFC( GetViewServices()->IsElementLocked( pIElement, & fLocked ));
    _pAdorner = new CActiveControlAdorner( pIElement , _pEd->GetDoc() , fLocked );
    if ( ! _pAdorner )
        goto Error;

    _pAdorner->SetManager( this );
    _pAdorner->AddRef();

    hr = _pAdorner->CreateAdorner() ;

Cleanup:
    ReleaseInterface( pIElement );
    RRETURN ( hr );
Error:
    return E_OUTOFMEMORY;
}


//+====================================================================================
//
// Method:DestroyAdorner
//
// Synopsis: Destroy the adorner associated with this tracker
//
//------------------------------------------------------------------------------------


VOID
CSelectionManager::DestroyAdorner()
{
    if ( _pAdorner )
    {
        _pAdorner->DestroyAdorner();
        _pAdorner->Release();
        _pAdorner = NULL;
    }
}


BOOL
CSelectionManager::HasFocusAdorner()
{
    return ( _pAdorner != NULL );
}

#if DBG == 1


void
TrackerNotifyToString(TCHAR* pAryMsg, TRACKER_NOTIFY eNotify )
{
    switch( eNotify )
    {
    case TN_NONE:
        edWsprintf( pAryMsg, _T("%s"), _T("TN_NONE"));
        break;

    case TN_BECOME_PASSIVE:
        edWsprintf( pAryMsg, _T("%s"), _T("TN_BECOME_PASSIVE"));
        break;

    case TN_END_TRACKER:
        edWsprintf( pAryMsg, _T("%s"), _T("TN_END_TRACKER"));
        break;

    case TN_END_TRACKER_NO_CLEAR:
        edWsprintf( pAryMsg, _T("%s"), _T("TN_END_TRACKER_NO_CLEAR") );
        break;
        
    case TN_END_TRACKER_SHIFT_START:
        edWsprintf( pAryMsg, _T("%s"), _T("TN_END_TRACKER_SHIFT_START") );
        break;
        
    case TN_HIDE_CARET:
        edWsprintf( pAryMsg, _T("%s"), _T("TN_HIDE_CARET"));
        break;

    case TN_END_SELECTION:
        edWsprintf( pAryMsg, _T("%s"), _T("TN_END_SELECTION"));
        break;

    case TN_TIMER_TICK:
        edWsprintf( pAryMsg, _T("%s"), _T("TN_TIMER_TICK"));
        break;

    case TN_FIRE_DELETE:
        edWsprintf( pAryMsg, _T("%s"), _T("TN_FIRE_DELETE") );
        break;
        
    case TN_FIRE_DELETE_REBUBBLE:
        edWsprintf( pAryMsg, _T("%s"), _T("TN_FIRE_DELETE_REBUBBLE") );
        break;
        
    case TN_KILL_ADORNER:
        edWsprintf( pAryMsg, _T("%s"), _T("TN_KILL_ADORNER") );
        break;
        
    case TN_END_TRACKER_RESELECT:
        edWsprintf( pAryMsg, _T("%s"), _T("TN_END_TRACKER_RESELECT"));
        break;

    case TN_END_TRACKER_REBUBBLE_ADORN:
        edWsprintf( pAryMsg, _T("%s"), _T("TN_END_TRACKER_REBUBBLE_ADORN") );
        break;
        
    case TN_END_TRACKER_RESELECT_REBUBBLE:
        edWsprintf( pAryMsg, _T("%s"), _T("TN_END_TRACKER_RESELECT_REBUBBLE"));
        break;

    case TN_END_TRACKER_CREATE_CARET:
        edWsprintf( pAryMsg, _T("%s"), _T("TN_END_TRACKER_CREATE_CARET"));
        break;

    case TN_REBUBBLE:
        edWsprintf( pAryMsg, _T("%s"), _T("TN_REBUBBLE") );
        break;
        
    case TN_END_TRACKER_OBJECT_ACTIVE:
        edWsprintf( pAryMsg, _T("%s"), _T("TN_END_TRACKER_OBJECT_ACTIVE") );
        break;
        
    case TN_END_TRACKER_POS_SELECT:
        edWsprintf( pAryMsg, _T("%s"), _T("TN_END_TRACKER_POS_SELECT") );
        break;

    case TN_END_TRACKER_POS_CONTROL:
        edWsprintf( pAryMsg, _T("%s"), _T("TN_END_TRACKER_POS_CONTROL") );
        break;

    case TN_END_TRACKER_POS_CARET_REBUBBLE:
        edWsprintf( pAryMsg, _T("%s"), _T("ND_TRACKER_POS_CARET_REBUBBLE") );
        break;
            
    case TN_END_TRACKER_START_IME:
        edWsprintf( pAryMsg, _T("%s"), _T("TN_END_TRACKER_START_IME") );
        break;
        
    default:
        edWsprintf( pAryMsg, _T("%s"), _T("UNKNOWN NOTIFY"));
        break;

    }

}

void
SelectionTypeToString( TCHAR* pAryMsg, SELECTION_TYPE eType )
{
    switch ( eType )
    {
        case SELECTION_TYPE_None:
            edWsprintf( pAryMsg, _T("%s"), _T("SELECTION_TYPE_None"));
            break;

        case SELECTION_TYPE_Caret:
            edWsprintf( pAryMsg, _T("%s"), _T("SELECTION_TYPE_Caret"));
            break;

        case SELECTION_TYPE_Selection:
            edWsprintf( pAryMsg, _T("%s"), _T("SELECTION_TYPE_Selection"));
            break;

        case SELECTION_TYPE_Control:
            edWsprintf( pAryMsg, _T("%s"), _T("SELECTION_TYPE_Control"));
            break;

        case SELECTION_TYPE_Auto:
            edWsprintf( pAryMsg, _T("%s"), _T("SELECTION_TYPE_Auto"));
            break;

        case SELECTION_TYPE_IME:
            edWsprintf( pAryMsg, _T("%s"), _T("SELECTION_TYPE_IME"));
            break;

        default:
            edWsprintf( pAryMsg, _T("%s"), _T("Unknown Type"));
            break;
    }
}

void
CSelectionManager::DumpNotify(TRACKER_NOTIFY eNotify, BOOL fBefore )
{
    TCHAR sNotifyState[30];
    TCHAR sTrackerType[20];

    if ( IsTagEnabled( tagSelectionMgrNotify ))
    {
        TrackerNotifyToString( sNotifyState, eNotify );
        SelectionTypeToString( sTrackerType, GetSelectionType() );


        if ( fBefore )
        {
            TraceTag(( tagSelectionMgrNotify, "Before Tracker Notify: %ls ActiveTrackerType:%ls", sNotifyState, sTrackerType));
        }
        else
            TraceTag(( tagSelectionMgrNotify, "After Tracker Notify: %ls ActiveTrackerType:%ls \n", sNotifyState, sTrackerType));
    }
}

#endif


ELEMENT_TAG_ID
CSelectionManager::GetEditableTagId()
{
    IHTMLElement* pIElement = NULL;

    HRESULT hr = S_OK;

    if ( _eContextTagId == TAGID_NULL )
    {

        hr = THR( GetEditableElement( & pIElement ));
        if ( hr)
            goto Cleanup;

        hr = THR( _pEd->GetMarkupServices()->GetElementTagId( pIElement, & _eContextTagId ));
    }
Cleanup:

    ReleaseInterface( pIElement);

    return ( _eContextTagId );
}

BOOL 
CSelectionManager::IsEditContextPositioned()
{
    if (! _fPositionedSet )
    {
        _fPositioned = IsElementPositioned( GetEditableElement() );
        _fPositionedSet = TRUE;
    }

    return ( _fPositioned );
}

BOOL
CSelectionManager::IsEditContextSet()
{
    BOOL fPositioned = FALSE;
    IGNORE_HR( _pStartContext->IsPositioned( & fPositioned ));

    if ( fPositioned )
        IGNORE_HR( _pEndContext->IsPositioned( & fPositioned ));

    return fPositioned;

}


HRESULT
CSelectionManager::MovePointersToContext(
    IMarkupPointer *        pLeftEdge,
    IMarkupPointer *        pRightEdge )
{
    HRESULT hr = S_OK;
    IFR( pLeftEdge->MoveToPointer( _pStartContext ));
    IFR( pRightEdge->MoveToPointer( _pEndContext ));
    RRETURN( hr );
}


//+====================================================================================
//
// Method: IsInEditContext
//
// Synopsis: Check to see that the given markup pointer is within the Edit Context
//
//------------------------------------------------------------------------------------


BOOL
CSelectionManager::IsInEditContext( IMarkupPointer* pPointer)
{
    BOOL fInside = FALSE;
    
    IsInEditContext( pPointer, & fInside );
    
    return fInside;
}

//+====================================================================================
//
// Method: IsInEditContext
//
// Synopsis: Check to see that the given markup pointer is within the Edit Context
//
//------------------------------------------------------------------------------------


HRESULT
CSelectionManager::IsInEditContext( IMarkupPointer* pPointer, BOOL * pfInEdit )
{
    HRESULT hr = S_OK;
    BOOL fInside = FALSE;
    BOOL fResult = FALSE;
    
#if DBG == 1
    BOOL fPositioned = FALSE;
    IGNORE_HR( _pStartContext->IsPositioned( & fPositioned ));
    Assert( fPositioned );
    IGNORE_HR( _pEndContext->IsPositioned( & fPositioned  ));
    Assert( fPositioned );
#endif

    IFC( pPointer->IsLeftOf( _pStartContext, & fResult ));
        
    if ( fResult )
        goto Cleanup;

    IFC( pPointer->IsRightOf( _pEndContext, & fResult ));
    
    if ( fResult )
        goto Cleanup;

    fInside = TRUE;

Cleanup:
    if ( pfInEdit )
        *pfInEdit = fInside;

    return ( hr );
}

//+====================================================================================
//
// Method: IsAfterStart
//
// Synopsis: Check to see if this markup pointer is After the Start ( ie to the Right )
//
//------------------------------------------------------------------------------------


HRESULT
CSelectionManager::IsAfterStart( IMarkupPointer* pPointer, BOOL * pfAfterStart )
{
    HRESULT hr = S_OK;
    Assert( pfAfterStart );

    BOOL fAfterStart = FALSE;

#if DBG == 1
    BOOL fPositioned = FALSE;
    IGNORE_HR( _pStartContext->IsPositioned( & fPositioned ));
    Assert( fPositioned );
#endif
    int iWherePointer = SAME;

    hr = THR( OldCompare( _pStartContext, pPointer, & iWherePointer  ));
    if ( hr )
    {
        //AssertSz(0, "Unable To Compare Pointers - Are they in the same tree?");
        goto Cleanup;
    }

    fAfterStart =  ( iWherePointer != LEFT );

Cleanup:
    *pfAfterStart = fAfterStart;
    RRETURN( hr );
}

//+====================================================================================
//
// Method: IsBeforeEnd
//
// Synopsis: Check to see if this markup pointer is Before the End ( ie to the Left )
//
//------------------------------------------------------------------------------------

HRESULT
CSelectionManager::IsBeforeEnd( IMarkupPointer* pPointer, BOOL *pfBeforeEnd )
{
    HRESULT hr = S_OK;
    BOOL fBeforeEnd = FALSE;

#if DBG == 1
    BOOL fPositioned = FALSE;
    IGNORE_HR( _pEndContext->IsPositioned( & fPositioned ));
    Assert( fPositioned );
#endif
    int iWherePointer = SAME;

    hr = THR( OldCompare( _pEndContext, pPointer, & iWherePointer  ));
    if ( hr )
    {
        //AssertSz(0, "Unable To Compare Pointers - Are they in the same tree?");
        goto Cleanup;
    }

    fBeforeEnd =  ( iWherePointer != RIGHT );

Cleanup:
    *pfBeforeEnd = fBeforeEnd;

    RRETURN( hr );
}

//+====================================================================================
//
// Method: IsSameEditContext
//
// Synopsis: Compare two given markup pointers to see if they represent a context
//           change from the current markup context
//
//------------------------------------------------------------------------------------


BOOL
CSelectionManager::IsSameEditContext(
                                        IMarkupPointer* pPointerStart,
                                        IMarkupPointer* pPointerEnd,
                                        BOOL * pfPositioned )
{
    HRESULT hr = S_OK;
    BOOL fSame = FALSE;
    BOOL fPositioned = FALSE;

    hr = THR( _pStartContext->IsPositioned( & fPositioned));
    if ( hr)
        goto Cleanup;

    if ( ! fPositioned )
        goto Cleanup;

    hr = THR( _pEndContext->IsPositioned( & fPositioned));
    if ( hr)
        goto Cleanup;

    if ( ! fPositioned )
        goto Cleanup;

    hr = THR( _pStartContext->IsEqualTo( pPointerStart, & fSame ));
    if ( hr )
        goto Cleanup;
    if ( ! fSame )
        goto Cleanup;

    hr = THR( _pEndContext->IsEqualTo( pPointerEnd, & fSame ));
    if ( hr )
        goto Cleanup;

Cleanup:
    if ( pfPositioned )
        *pfPositioned = fPositioned;

    return ( fSame );
}

//+====================================================================================
//
// Method: GetEditableElement
//
// Synopsis: Return an IHTMLElement of the current editing context
//
//------------------------------------------------------------------------------------

HRESULT
CSelectionManager::GetEditableElement( IHTMLElement** ppElement)
{
    Assert( _pStartContext );
    
    BOOL fPositioned = FALSE;
    IGNORE_HR( _pStartContext->IsPositioned( & fPositioned));

    if ( fPositioned )
    {
        if ( ! _fNoScope )
            RRETURN( GetViewServices()->CurrentScopeOrSlave(_pStartContext, ppElement ));
        else
        {
            RRETURN ( GetViewServices()->RightOrSlave(_pStartContext, FALSE, FALSE, ppElement, NULL, NULL ) );
        }
    }
    return S_FALSE;
}

//+====================================================================================
//
// Method: GetEditableElement
//
// Synopsis: GetEditable Element as an Accessor
//
//------------------------------------------------------------------------------------

IHTMLElement* 
CSelectionManager::GetEditableElement()
{
    if ( ! _pIEditableElement)
        GetEditableElement( & _pIEditableElement );
        
    return _pIEditableElement;
}
//+====================================================================================
//
// Method: SetDrillIn
//
// Synopsis: Tell the Manager we're "drilling in". Used so we know whether to set
//           or reset the _fHadGrabHandles Flag ( further used to ensure we don't create
//           a Site Selection again on an object that's UI Active).
//
//------------------------------------------------------------------------------------


void
CSelectionManager::SetDrillIn(BOOL fDrillIn, SelectionMessage* pLastMessage)
{
    _fDrillIn = fDrillIn;
    _fHadGrabHandles = fDrillIn;
    if ( pLastMessage )
    {
       _lastMessage = * pLastMessage;
       _fLastMessageValid = TRUE;
    }
    else
    {
        _fLastMessageValid = FALSE;
    }
}



//+====================================================================================
//
// Method: SetNextMessage
//
// Synopsis: Allow caching of 2 messages ( first set in SetDrillIn). Currently this
//           is used in storing WM_LBUTOONUP on going UI Active
//
//------------------------------------------------------------------------------------


void
CSelectionManager::SetNextMessage(SelectionMessage* pNextMessage)
{
    if ( pNextMessage )
    {
       _nextMessage = * pNextMessage;
       _fNextMessageValid = TRUE;
    }
    else
    {
        _fNextMessageValid = FALSE;
    }
}

VOID
CSelectionManager::StoreLastMessage( SelectionMessage* pLastMessage )
{
    Assert( pLastMessage );
    Assert( ! _fLastMessageValid );
    _lastMessage = * pLastMessage;
    _fLastMessageValid = TRUE;
}

//+====================================================================================
//
// Method: Select All
//
// Synopsis: Do the work required by a select all.
//
//          If ISegmentList is not null it is either a TextRange or a Control Range, and we
//          select around this segment
//
//          Else we do a Select All on the Current Edit Context.
//
//------------------------------------------------------------------------------------


HRESULT
CSelectionManager::SelectAll(ISegmentList * pSegmentList, BOOL *pfDidSelectAll)
{
    HRESULT hr = S_OK;
    IMarkupPointer* pStart = NULL;
    IMarkupPointer* pEnd = NULL;
    IHTMLElement* pIEditableElement = NULL;
    IHTMLElement* pIOuterElement = NULL;
    int iWherePointer = SAME;
    SELECTION_TYPE desiredType = SELECTION_TYPE_Selection;
    int ctSegment;

    Assert( pSegmentList );

    if ( pSegmentList )
    {
        hr = THR( pSegmentList->GetSegmentCount( & ctSegment, & desiredType ));
    }

    if ( desiredType == SELECTION_TYPE_Auto )
    {

        IFC( _pEd->GetMarkupServices()->CreateMarkupPointer( & pStart));
        IFC( _pEd->GetMarkupServices()->CreateMarkupPointer( & pEnd ));
        IFC( pSegmentList->MovePointersToSegment( 0, pStart, pEnd ));

        IFC( pSegmentList->GetSegmentCount( & ctSegment, & desiredType ));
        if ( desiredType == SELECTION_TYPE_Auto )
        {
            hr = THR( OldCompare( pStart, pEnd, & iWherePointer));
            if ( hr )
                goto Cleanup;

            if ( iWherePointer == SAME )
                desiredType = SELECTION_TYPE_Caret;
            else
                desiredType = SELECTION_TYPE_Selection;
        }

        //
        // BUGBUG - we should pass a selection code here !
        //
        hr = THR( GetViewServices()->EnsureEditContext( pStart ));
        if ( hr )
            goto Cleanup;
            
        hr = Select( pStart, pEnd , desiredType, NULL, pfDidSelectAll );

    }
    else
    {
        BOOL fStartPositioned = FALSE;
        BOOL fEndPositioned = FALSE;

        IGNORE_HR( _pStartContext->IsPositioned( & fStartPositioned ));
        IGNORE_HR( _pEndContext->IsPositioned( & fEndPositioned ));
        desiredType = SELECTION_TYPE_Selection;

        //
        // If we're not editable, or we're UI Active we select the edit context
        //
        if ( ( !IsParentEditable() || HasFocusAdorner() )
                && fStartPositioned && fEndPositioned )
        {
           hr = Select( _pStartContext, _pEndContext , desiredType , NULL , pfDidSelectAll );
        }
        else
        {
            //
            // Select the "outermost editable element"
            //
            IFC( _pEd->GetMarkupServices()->CreateMarkupPointer( & pStart));
            IFC( _pEd->GetMarkupServices()->CreateMarkupPointer( & pEnd ));
            IGNORE_HR( GetEditableElement( & pIEditableElement)); // pIEditableElement may be FALSE
            IFC( GetViewServices()->GetOuterMostEditableElement( pIEditableElement , & pIOuterElement ));
            
            IFC( pStart->MoveAdjacentToElement( pIOuterElement, ELEM_ADJ_AfterBegin));
            IFC( pEnd->MoveAdjacentToElement( pIOuterElement, ELEM_ADJ_BeforeEnd ));

            IFC( GetViewServices()->EnsureEditContext( pStart ));
#if DBG == 1
            BOOL fEqual = FALSE;
            IGNORE_HR( _pStartContext->IsEqualTo( pStart, & fEqual ));
            AssertSz( fEqual , "Pointers not same - did Ensure Edit Context work ?");
#endif 
            hr = Select( pStart, pEnd , desiredType, NULL, pfDidSelectAll );
        }
    }

Cleanup:
    ReleaseInterface( pStart );
    ReleaseInterface( pEnd );
    ReleaseInterface( pIEditableElement);
    ReleaseInterface( pIOuterElement );

    RRETURN ( hr );
}

HRESULT
CSelectionManager::Select(
                            IMarkupPointer* pStart,
                            IMarkupPointer * pEnd,
                            SELECTION_TYPE eType,
                            DWORD* pdwFollowUpAction /* = NULL */ ,
                            BOOL* pfDidSelection /* = NULL*/)
{
    HRESULT hr = S_OK;
    BOOL fPositioned;
    Assert( pStart && pEnd );
    IHTMLElement* pIEditElement = NULL;
    BOOL fOkToSelect = TRUE;
    BOOL fDidSelection = FALSE;
    BOOL fBetweenLines = FALSE;
    BOOL fNotAtBOL = TRUE;
    BOOL fAtLogicalBOL = FALSE;

    IFC( pStart->IsPositioned(  & fPositioned ));

    if ( ! fPositioned )
    {
        AssertSz( 0, "Select - Start Pointer is NOT positioned!");
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    IFC( pEnd->IsPositioned(  & fPositioned  ));
    if ( ! fPositioned )
    {
        AssertSz( 0, "Select - End Pointer is NOT positioned!");
        hr = E_INVALIDARG;
        goto Cleanup;
    }
    if ( eType == SELECTION_TYPE_Selection)
    {
        hr = THR( GetViewServices()->CurrentScopeOrSlave(pStart, & pIEditElement ));
        fOkToSelect = GetViewServices()->FireOnSelectStart( pIEditElement) ;
    }

    if ( fOkToSelect )
    {
        //
        // Verify that the pointers we're been given are in the Edit Context.
        //
        AssertSz( IsInEditContext(pStart), "Start Pointer is NOT in Edit Context");
        AssertSz( IsInEditContext(pEnd), "End Pointer is NOT in Edit Context");

        // Transition trackers
        DWORD dwTCCode = 0;

        // If caret tracker is both source and destination, and the caret did not move, do nothing
        if (eType == SELECTION_TYPE_Caret )
        {
            fBetweenLines = FALSE;
            IFC( GetViewServices()->IsPointerBetweenLines( pStart, & fBetweenLines ));

            fNotAtBOL = ! fBetweenLines ;
            fAtLogicalBOL = fBetweenLines ;

            if( GetSelectionType() == SELECTION_TYPE_Caret )
            {
                BOOL                fEqual;
                SP_IMarkupPointer   spCaret;
                CCaretTracker       *pCaretTracker = DYNCAST(CCaretTracker, _pActiveTracker);

                if (pCaretTracker)
                {
                    BOOL fCaretNotAtBOL = pCaretTracker->GetNotAtBOL();
                    BOOL fCaretAtLogBOL = pCaretTracker->GetAtLogicalBOL();
                    IFC( pCaretTracker->GetCaretPointer(&spCaret) );
                    
                    if (spCaret != NULL )
                    {
                        IFC( spCaret->IsEqualTo(pStart, &fEqual) );

                        if (fEqual)
                        {
                            IFC( spCaret->IsEqualTo(pEnd, &fEqual) );

                            if (fEqual)
                            {
                                if( fNotAtBOL == fCaretNotAtBOL && fAtLogicalBOL == fCaretAtLogBOL )
                                {
                                    goto Cleanup; // done, nothing to do
                                }
                                else
                                {
                                    IFC( pCaretTracker->PositionCaretAt( spCaret, fNotAtBOL, fAtLogicalBOL, CARET_DIRECTION_INDETERMINATE, POSCARETOPT_None, ADJPTROPT_None ));
                                }
                            }
                        }           
                    }
                }
            }
            else
            {
                if ( fNotAtBOL)
                {
                    dwTCCode |= TRACKER_CREATE_NOTATBOL;
                }
                
                if (fAtLogicalBOL)
                {
                    dwTCCode |= TRACKER_CREATE_ATLOGICALBOL;
                }
            }
        }
        
        EndCurrentTracker(pdwFollowUpAction,  NULL, TRUE, TRUE);

        hr = CreateTrackerForType ( 
                                    pdwFollowUpAction, 
                                    eType, 
                                    pStart, 
                                    pEnd, 
                                    dwTCCode, 
                                    CARET_MOVE_NONE, 
                                    FALSE );
        if ( hr == S_OK )
            fDidSelection = TRUE;
    }

Cleanup:
    ReleaseInterface(pIEditElement);
    if ( pfDidSelection )
        *pfDidSelection = fDidSelection;
    RRETURN ( hr );

}

HRESULT
CSelectionManager::CopyTempMarkupPointers( IMarkupPointer* pStart, IMarkupPointer* pEnd )
{
    HRESULT hr = S_OK;

    Assert( pStart && pEnd );
#if DBG == 1
    BOOL fPositioned = FALSE;
    IGNORE_HR( pStart->IsPositioned( & fPositioned) );
    Assert( fPositioned );
    IGNORE_HR( pEnd->IsPositioned( & fPositioned ));
    Assert( fPositioned );
#endif

    if ( ! _pStartTemp )
    {
        IFC( GetMarkupServices()->CreateMarkupPointer( & _pStartTemp ));
    }

    if ( ! _pEndTemp )
    {
        IFC( GetMarkupServices()->CreateMarkupPointer( & _pEndTemp ));
    }
    
    IFC( _pStartTemp->MoveToPointer( pStart ) ) ;
    IFC( _pEndTemp->MoveToPointer( pEnd ));

Cleanup:
    RRETURN ( hr );
}

void
CSelectionManager::ReleaseTempMarkupPointers()
{
    ClearInterface( & _pStartTemp );
    ClearInterface( & _pEndTemp );
}


//+====================================================================================
//
// Method: IsElementSiteSelected
//
// Synopsis: See if the given element is Site Selected
//
//------------------------------------------------------------------------------------
HRESULT
CSelectionManager::IsElementSiteSelected( IHTMLElement* pIElement)
{
    HRESULT hr = S_FALSE;
    BOOL fSame = FALSE;

    if ( _pActiveTracker && GetSelectionType() == SELECTION_TYPE_Control )
    {
        CControlTracker* pControlTrack = DYNCAST( CControlTracker, _pActiveTracker );
        fSame = pControlTrack->IsSameElementAsControl( pIElement ) ;
        if ( fSame )
            hr = S_OK;
    }
        
    RRETURN1 ( hr , S_FALSE);
}



//+====================================================================================
//
// Method: IsPointerInSelection
//
// Synopsis: Check to see if the given pointer is in a Selection
//
//------------------------------------------------------------------------------------


HRESULT
CSelectionManager::IsPointerInSelection(
                                         IMarkupPointer* pIPointer ,
                                         BOOL* pfPointInSelection,
                                         POINT * pptGlobal,
                                         IHTMLElement* pIElementOver            )
{
    BOOL fInSelection = FALSE;


    if ( _pActiveTracker )
    {
        fInSelection = _pActiveTracker->IsPointerInSelection( pIPointer, pptGlobal, pIElementOver );
    }
    else
        fInSelection = FALSE;

    if ( pfPointInSelection )
        *pfPointInSelection = fInSelection;


    RRETURN ( S_OK );
}

VOID
CSelectionManager::AdornerPositionSet()
{
    Assert( _fPendingAdornerPosition );
    DYNCAST( CControlTracker, _pActiveTracker)->BecomeActiveOnFirstMove( & _lastMessage );
    _fPendingAdornerPosition = FALSE;
}

VOID 
CSelectionManager::SetPendingTrackerNotify( 
                BOOL fPendingTrackerNotify, 
                TRACKER_NOTIFY eNotify )
{
    Assert( ! _fPendingTrackerNotify );
    _fPendingTrackerNotify =  fPendingTrackerNotify;
    _pendingTrackerNotify = eNotify;
}


//+===================================================================================
// Method: IsMessageInSelection
//
// Synopsis: Check to see if the given message is in the current SelectionRenderingServices
//
//------------------------------------------------------------------------------------
BOOL
CSelectionManager::IsMessageInSelection( SelectionMessage* pMessage)
{
    HRESULT hr = S_OK;

    IMarkupPointer* pPointerMsg = NULL;
    IMarkupPointer* pPointerStartSel = NULL;
    IMarkupPointer* pPointerEndSel = NULL;
    ISegmentList * pSegmentList = NULL;

    SELECTION_TYPE eSegmentType = SELECTION_TYPE_None;
    int ctSegment = 0;
    BOOL fNotAtBOL = FALSE;
    BOOL fAtLogicalBOL = TRUE;
    BOOL fRightOfCp = FALSE;
    BOOL fValidTree = FALSE;
    BOOL fIsInSelection = FALSE;
    int wherePointer = SAME;
    BOOL fEqual = FALSE;
    HTMLPtrDispInfoRec DispInfo;
    IMarkupContainer* pIContainer1 = NULL;
    IMarkupContainer* pIContainer2 = NULL;
    
    hr = THR( GetViewServices()->GetCurrentSelectionSegmentList( & pSegmentList ));
    if ( hr )
        goto Cleanup;

    hr = THR( pSegmentList->GetSegmentCount(& ctSegment,  & eSegmentType ));
    if ( hr )
        goto Cleanup;

    Assert( ctSegment <= 1 ); // do we do an ole/drag drop for multiple selections ?

    if ( ( ctSegment == 1 ) &&
         (eSegmentType == SELECTION_TYPE_Selection ) )
    {
        Assert( eSegmentType == SELECTION_TYPE_Selection );

        hr = THR( GetMarkupServices()->CreateMarkupPointer( & pPointerMsg ));
        if ( hr )
            goto Cleanup;

        hr = THR( GetMarkupServices()->CreateMarkupPointer( & pPointerStartSel ));
        if ( hr )
            goto Cleanup;

        hr = THR( GetMarkupServices()->CreateMarkupPointer( & pPointerEndSel));
        if ( hr )
            goto Cleanup;

        hr = THR( pSegmentList->MovePointersToSegment( 0, pPointerStartSel, pPointerEndSel ));
        if ( hr )
            goto Cleanup;

        hr = THR( pPointerStartSel->IsEqualTo( pPointerEndSel, & fEqual ));
        if ( hr )
            goto Cleanup;

        if ( ! fEqual )
        {


            hr = THR( GetViewServices()->MoveMarkupPointerToMessage( pPointerMsg,
                                                       pMessage,
                                                       &fNotAtBOL,
                                                       &fAtLogicalBOL,
                                                       &fRightOfCp,
                                                       &fValidTree,
                                                       FALSE,
                                                       GetEditableElement(),
                                                       NULL,
                                                       FALSE ));
            if ( hr )
                goto Cleanup;

            //
            // See if we're in different containers. If we are - we adjust.
            //
            IFC( pPointerMsg->GetContainer( & pIContainer1 ));
            IFC( pPointerStartSel->GetContainer( & pIContainer2 ));
            if ( ! GetEditor()->EqualContainers( pIContainer1, pIContainer2))
            {
                //
                // IFC is ok - if we can't be adjusted into the same containers , we will
                // return S_FALSE - and we will bail.
                //
                IFC( GetEditor()->MovePointersToEqualContainers( pPointerMsg,pPointerStartSel ));
            }
            
            hr = THR( OldCompare( pPointerStartSel, pPointerMsg, & wherePointer));
            if ( hr )
                goto Cleanup;

            if ( wherePointer == LEFT )
                goto Cleanup;

            if ( wherePointer == SAME )
            {
                //
                // Verify we are truly to the left of the point. As at EOL/BOL the fRightOfCp
                // test above may be insufficient
                //
                IFC( GetViewServices()->GetLineInfo( pPointerMsg, fNotAtBOL, & DispInfo ));
                if ( DispInfo.lXPosition > pMessage->ptContent.x )
                    goto Cleanup;
            }                    

            hr = THR( OldCompare( pPointerEndSel, pPointerMsg, & wherePointer ));
            if ( hr )
                goto Cleanup;

            if ( wherePointer == RIGHT )              
                goto Cleanup;

            if ( wherePointer == SAME )
            {
                //
                // Verify we are truly to the right of the point. As at EOL/BOL the fRightOfCp
                // test above may be insufficient
                //            
                IFC( GetViewServices()->GetLineInfo( pPointerMsg, fNotAtBOL, & DispInfo ));
                if ( DispInfo.lXPosition < pMessage->ptContent.x )
                    goto Cleanup;
            }                    


            //
            //  We're between the start and end - ergo we're inside
            //
            fIsInSelection = TRUE;
        }
    }


Cleanup:
    ReleaseInterface( pPointerMsg );
    ReleaseInterface( pPointerStartSel );
    ReleaseInterface( pPointerEndSel );
    ReleaseInterface( pSegmentList );
    ReleaseInterface( pIContainer1 );
    ReleaseInterface( pIContainer2 );
    
    return fIsInSelection;
}


//+====================================================================================
//
// Method: IsOkToEditContents
//
// Synopsis: Fires the OnBeforeXXXX Event back to Trident to see if its' ok to UI-Activate,
//           and place a caret inside a given control.
//
// RETURN: TRUE - if it's ok to go ahead and UI Activate
//         FALSE - if it's not ok to UI Activate 
//------------------------------------------------------------------------------------



BOOL
CSelectionManager::FireOnBeforeEditFocus()
{
    HRESULT hr = S_OK;
    BOOL fRet = FALSE;
    
    BOOL fEditFocusBefore = _fEditFocusAllowed;

    hr = THR( GetViewServices()->FireOnBeforeEditFocus( GetEditableElement() , & fRet ));
    
    _fEditFocusAllowed = fRet ;
    
    //
    // We have changed states - so we need to enable/disable the UI Active border
    // Note that by default _fUIActivate == TRUE - so this will not fire on SetEditContextPrivate
    // for the first time.
    //
    if ( _fEditFocusAllowed != fEditFocusBefore )
    {
        if ( _fEditFocusAllowed && IsElementUIActivatable())
        {
            hr = THR( CreateAdorner());
        }
        else if ( !_fEditFocusAllowed && _pAdorner )
        {
            DestroyAdorner();
        }
        if ( _pActiveTracker )
            _pActiveTracker->OnEditFocusChanged();
    }

    return fRet;
       
}


//+====================================================================================
//
// Method: IsInEditContextClientRect
//
// Synopsis: Is the given point in the content area of the Editable Element ?
//
//------------------------------------------------------------------------------------


HRESULT
CSelectionManager::IsInEditableClientRect( POINT ptGlobal )
{
    HRESULT hr = S_OK;
    IHTMLElement* pIOuterMost = NULL;
    
    RECT rectGlobal;
    IFC( GetViewServices()->GetOuterMostEditableElement( GetEditableElement(), & pIOuterMost ));   
    IFC( GetViewServices()->GetClientRect( pIOuterMost , COORD_SYSTEM_GLOBAL, & rectGlobal ));
    
    hr =  ::PtInRect( & rectGlobal, ptGlobal ) ? S_OK : S_FALSE;
    
Cleanup:
    ReleaseInterface( pIOuterMost );
    RRETURN1( hr, S_FALSE );
}


#if DBG == 1

void 
CSelectionManager::DumpTree( IMarkupPointer* pPointer )
{
    Assert( GetEditDebugServices()) ; // possible for this to be null if a retail mshtml creates a debug mshtmled
    if ( GetEditDebugServices() )
    {
        IGNORE_HR( GetEditDebugServices()->DumpTree( pPointer ));
    }        
}

long
CSelectionManager::GetCp( IMarkupPointer* pPointer )
{
    long cp = 0;

    Assert( GetEditDebugServices()) ; // possible for this to be null if a retail mshtml creates a debug mshtmled
    
    if ( GetEditDebugServices() )
    {
        IGNORE_HR( GetEditDebugServices()->GetCp( pPointer, & cp));
    }
    return cp;
    
}

void 
CSelectionManager::SetDebugName( IMarkupPointer* pPointer, LPCTSTR strDebugName )
{
    Assert( GetEditDebugServices()) ; // possible for this to be null if a retail mshtml creates a debug mshtmled

    if ( GetEditDebugServices())
    {
        IGNORE_HR( GetEditDebugServices()->SetDebugName( pPointer, strDebugName));
    }        
}

#endif
