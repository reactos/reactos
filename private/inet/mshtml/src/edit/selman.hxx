//+---------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1998.
//
//  Class:      CSelectionManager
//
//  Contents:   CSelectionManager Class
//
//              A CSelectionManager - translates GUI gestures, into a meaningful
//              "selection" on screen.
//
//  History:   05/09/98  marka Created
//----------------------------------------------------------------------------

#ifndef _SELMAN_HXX_
#define _SELMAN_HXX_

#ifndef X_RESOURCE_H_
#define X_RESOURCE_H
#include "resource.h"    
#endif

#ifndef X_HTMLED_HXX_
#define X_HTMLED_HXX_
#include "htmled.hxx"
#endif

#ifndef X_ISCSA_H_
#define X_ISCSA_H_
#include "iscsa.h"
#endif


class CEditTracker;
class CActiveControlAdorner;
class CHTMLEditor;
class CSelectTracker;
#ifndef NO_IME
class CIme;
#endif

enum TRACKER_NOTIFY;

enum CARET_MOVE_UNIT
{
    CARET_MOVE_NONE,
    CARET_MOVE_LEFT,
    CARET_MOVE_RIGHT,
    CARET_MOVE_UP,
    CARET_MOVE_DOWN,
    CARET_MOVE_WORDLEFT,
    CARET_MOVE_WORDRIGHT,
    CARET_MOVE_PAGEUP,
    CARET_MOVE_PAGEDOWN,
    CARET_MOVE_VIEWSTART,
    CARET_MOVE_VIEWEND,
    CARET_MOVE_LINESTART,
    CARET_MOVE_LINEEND,
    CARET_MOVE_HOME,
    CARET_MOVE_END,
    CARET_MOVE_BLOCKSTART,
    CARET_MOVE_BLOCKEND,
};


MtExtern(CSelectionManager)
MtExtern(CAryISCData_pv)

#define MAX_UNDOTITLE   64
#define MAX_ISC_COUNT    8

DECLARE_CStackDataAry(CAryISCData, ISCDATA, MAX_ISC_COUNT, Mt(Mem), Mt(CAryISCData_pv));

class CISCList
{
public:
    CISCList();
    ~CISCList();

    int FillList();
    IInputSequenceChecker* SetActive(LCID);
    BOOL CheckInputSequence(LPTSTR pszISCBuffer, long ich, WCHAR chTest);
    BOOL HasActiveISC() { return (_pISCCurrent != NULL); }
    int GetCount() { return _nISCCount; }

private:
    HRESULT Add(LCID lcidISC, IInputSequenceChecker* pISC);
    IInputSequenceChecker* Find(LCID lcidISC);

    CAryISCData _aryInstalledISC;
    int _nISCCount;
    LCID _lcidCurrent;
    IInputSequenceChecker* _pISCCurrent;

};


//
// Valid States are
//
// SM_NONE - SM_CARET -> SM_ACTIVESELECTION -> SM_SELECTION -> SM_ACTIVESELECTION || SM_CARET
//
enum SELMGR_STATE
{
    SM_NONE,                            // default state
    SM_NOSTATE_CHANGE,                  // no State change. Used for CSelectTracker::HandleMessage
    SM_STOP_TRACKER,                    // Used to request killing the tracker. Next State is either CARET or SELECTION
    SM_CARET,                           // Caret is visible.
    SM_ACTIVESELECTION ,                // Processing Selection. Caret is Invisible.
    SM_SELECTION,                       // Not processing Selection - but we have a selection
    SM_CONTROL_SELECTION ,              // A "control" is selected
    SM_ACTIVE_CONTROL_SELECTION ,       // A "control" is being resized ( click on a control handle happened)
    SM_STOP_CONTROL_SELECTION ,         // Transitioning out of Control Selection. Next State should be SM_ACTIVE_SELECTION
    SM_STOP_ACTIVE_CONTROL_SELECTION    // Transitioning from an active control. Next state is Control Selection
};

void 
SelectionTypeToString( TCHAR* pAryMsg, SELECTION_TYPE eType );




class CSelectionManager 
{
    public:
        // Constructor
        
        CSelectionManager( CHTMLEditor * pEd ); // { _pEd = pEd; }
        virtual ~CSelectionManager();

        DECLARE_MEMCLEAR_NEW_DELETE(Mt(CSelectionManager));

        // methods dispatched by IHTMLEditor
        
        HRESULT HandleMessage( 
            SelectionMessage* pSelectionMessage ,
            DWORD* pdwFollowUpAction )  ;
            
        HRESULT SetEditContext(
            BOOL fEditable ,
            BOOL fSetSelection,
            BOOL fParentEditable,
            IMarkupPointer* pStartPointer,
            IMarkupPointer* pEndPointer,
            BOOL fNoScope ); 

        HRESULT GetSelectionType(
            SELECTION_TYPE * eSelectionType );

        HRESULT Notify(
            SELECTION_NOTIFICATION eSelectionNotification,
            IUnknown *pUnknown, 
            DWORD* pdwFollowUpActionFlag,
            DWORD dword ) ;

        STDMETHOD ( IsPointerInSelection ) (
            IMarkupPointer* pMarkupPointer ,
            BOOL *  pfPointInSelection,
            POINT * pptGlobal,
            IHTMLElement* pIElementOver             );

        HRESULT IsElementSiteSelectable( IHTMLElement* pIElement );

        HRESULT IsElementSiteSelected( IHTMLElement* pIElement );
        
        CHTMLEditor * GetEditor() { return _pEd; }

        IHTMLDocument2* GetDoc() ;

        IHTMLViewServices* GetViewServices() ;

        IMarkupServices * GetMarkupServices(); 

#if DBG == 1
        IEditDebugServices* GetEditDebugServices();
        void DumpTree( IMarkupPointer* pPointer );
        long GetCp( IMarkupPointer* pPointer );
        void SetDebugName( IMarkupPointer* pPointer, LPCTSTR strDebugName );
#endif

        HRESULT EnsureAdornment();
        
        BOOL IsContextEditable();
        
        BOOL CanContextAcceptHTML();
        
        CEditTracker * GetActiveTracker() { return _pActiveTracker; }

        BOOL IsParentEditable();
        
        HRESULT TrackerNotify(
            TRACKER_NOTIFY inNotify, 
            SelectionMessage *pMessage, 
            DWORD* pdwFollowUpAction );

        SELECTION_TYPE GetSelectionType ( );

        ELEMENT_TAG_ID GetEditableTagId();

        void* GetEditableCookie();

        BOOL HasFocusAdorner();

        VOID SetDrillIn( BOOL inGrabHandles, SelectionMessage* pLastMessage = NULL );

        VOID SetNextMessage( SelectionMessage* pNextMessage );
        
        BOOL IsMessageInSelection( SelectionMessage * pSelectionMessage) ;

        BOOL HadGrabHandles();

        IHTMLElement* GetEditableElement( );
        
        HRESULT GetEditableElement(IHTMLElement** ppElement );

        void SetIgnoreEditContext(BOOL fSetIgnoreEdit);

        BOOL IsIgnoreEditContext();

        void SetPendingFollowUpCode ( DWORD pendingFollowUp );

        HRESULT MovePointersToContext( IMarkupPointer * pLeftEdge, IMarkupPointer * pRightEdge );

        // Depricate these next 2 methods - not COM safe!
        
        IMarkupPointer* GetStartEditContext();

        IMarkupPointer* GetEndEditContext();

        BOOL IsInEditContext( IMarkupPointer* pPointer);

        HRESULT IsInEditContext( IMarkupPointer* pPointer, BOOL* pfInEdit);

        HRESULT IsBeforeEnd( IMarkupPointer* pPointer, BOOL* pfBeforeEnd );

        HRESULT IsAfterStart( IMarkupPointer* pPointer, BOOL* pfAfterStart );
        
        HRESULT SelectAll(ISegmentList* pSegmentList, BOOL * pfDidSelectAll);

        BOOL IsEditContextSet();

        BOOL IsEditContextPositioned();
        
        HRESULT Select( 
                        IMarkupPointer* pStart, 
                        IMarkupPointer * pEnd, 
                        SELECTION_TYPE eType,
                        DWORD *pdwFollowUpAction = NULL,
                        BOOL *pfDidSelection = FALSE );

        VOID StoreLastMessage( SelectionMessage* pLastMessage );

        HRESULT StartSelectionFromShift(
                SelectionMessage *pMessage, 
                DWORD* pdwFollowUpAction,
                TRACKER_NOTIFY *peNotify );

        HRESULT CopyTempMarkupPointers( IMarkupPointer* pStart, IMarkupPointer* pEnd );                

        VOID SetInCapture(BOOL fInCapture)
        {
            _fInCapture = fInCapture;
        }       

        VOID SetInTimer(BOOL fInTimer )
        {
            _fInTimer = fInTimer;
        }

        BOOL IsInCapture()
        {
            return _fInCapture;
        }

        BOOL IsInTimer()
        {
            return _fInTimer;
        }

        
        HRESULT EmptySelection(BOOL fChangeTrackerAndSetRange = TRUE);
        

        BOOL GetOverwriteMode()
        {
            return _fOverwriteMode;
        }

        VOID SetOverwriteMode(BOOL fOverwriteMode)
        {
            _fOverwriteMode = !!fOverwriteMode;
        }

        VOID SetIgnoreExitTree( BOOL fIgnoreExitTree )
        {
            _fIgnoreExitTree = fIgnoreExitTree;
        }

        BOOL IsIgnoreExitTree()
        {
            return _fIgnoreExitTree;
        }

        VOID AdornerPositionSet();

        VOID SetDontChangeTrackers(BOOL fDontChangeTrackers)
        {
            _fDontChangeTrackers = fDontChangeTrackers;
        }

        BOOL IsDontChangeTrackers()
        {
            return _fDontChangeTrackers;
        }

        VOID SetPendingUndo( BOOL fPendingUndo )
        {
            _fPendingUndo = fPendingUndo;
        }

        BOOL IsPendingUndo()
        {
            return _fPendingUndo;
        }

        VOID SetPendingTrackerNotify( BOOL fPendingTrackerNotify, TRACKER_NOTIFY eNotify );

        BOOL IsPendingTrackerNotify()
        {
            return _fPendingTrackerNotify;
        }

        void CreateISCList()
        {
            _pISCList = new CISCList;
            if(_pISCList != NULL && _pISCList->GetCount() == 0)
            {
                delete _pISCList;
                _pISCList = NULL;
            }
            _fInitSequenceChecker = TRUE;
        }

        BOOL HasActiveISC()
        {
            BOOL fHasActiveISC = FALSE;

            // The first time we come in here we will attempt to load
            // ISC. This gives us a lazy load.
            if(!_fInitSequenceChecker)
            {
                CreateISCList();
            }

            if(_pISCList)
                fHasActiveISC = _pISCList->HasActiveISC();

            return fHasActiveISC;
        }

        CISCList* GetISCList()
        {
            return _pISCList;
        }

        VOID SetHadGrabHandles( BOOL fHadGrabHandles) 
        {
            _fHadGrabHandles = fHadGrabHandles;
        }

        BOOL HasSameTracker(CEditTracker* pEditTracker)
        {
            return ( _pActiveTracker == pEditTracker );
        }

        HRESULT IsInEditableClientRect( POINT ptGlobal);
        
        BOOL CanHaveEditFocus()
        {
            return _fEditFocusAllowed;
        }            

        BOOL IsElementUIActivatable( IHTMLElement* pIElement);

        IMarkupPointer * GetPtrBeganTyping()
        {
            return _pPointerBeganTyping;
        }

        BOOL HaveTypedSinceLastUrlDetect()
        {
            return _fHaveTypedSinceLastUrlDetect;
        }

        VOID SetHaveTypedSinceLastUrlDetect( BOOL fHaveTyped )
        {
            _fHaveTypedSinceLastUrlDetect = fHaveTyped;
        }
        

    private:

        BOOL IsCaretAlreadyWithinContext();
        
        BOOL FireOnBeforeEditFocus();

        VOID SetTCForActiveTrackerBOL( DWORD* dword );
        //
        // Methods
        //
        VOID Init();

        BOOL IsSameEditContext( 
                                    IMarkupPointer* pPointerStart, 
                                    IMarkupPointer* pPointerEnd, 
                                    BOOL * pfPositioned );
                                    
        HRESULT DestroySelection(DWORD* pdwFollowUpCode );

        HRESULT CaretInContext( DWORD* pdwFollowUpActionFlag );

        HRESULT ExitTree( DWORD * pdwFollowUpCode, IUnknown* pIElement);
        
        VOID EndTracker();                                  // called by tracker when its done

        HRESULT LoseFocus();

        HRESULT LoseFocusFrame(DWORD* pdwFollowUpAction, DWORD selType );

        HRESULT SetEditContextPrivate( 
                                    BOOL fEditable, 
                                    BOOL fSetSelection,
                                    BOOL fParentEditable,
                                    IMarkupPointer* pStart,
                                    IMarkupPointer* pEnd,
                                    BOOL fNoScope,
                                    BOOL fCreateTracker );


                     
        HRESULT CreateTrackerForType( 
                    DWORD* pdwFollowUpCode, 
                    SELECTION_TYPE eType , 
                    IMarkupPointer* pStart, 
                    IMarkupPointer* pEnd ,
                    DWORD dwTCFlags = 0,
                    CARET_MOVE_UNIT inLastCaretMove = CARET_MOVE_NONE, 
                    BOOL fSetTCFromActiveTracker = TRUE );

        HRESULT ShouldChangeTracker(    SelectionMessage *pMessage ,
                                        DWORD * pdwFollowUpAction,
                                        BOOL * pfStarted);
        
        VOID EndCurrentTracker( DWORD* pdwFollowUpCode, SelectionMessage* pMessage = NULL , BOOL fHideCaret = FALSE, BOOL fClear = TRUE);
        
        HRESULT HandleCaptureChanged( SelectionMessage * pMessage );

        HRESULT DeleteSelection( BOOL fAdjustPointersBeforeDeletion );

        HRESULT OnTimerTick( DWORD * pdwFollowUpActionCode );

        HRESULT EmptySelection( DWORD * pdwFollowUpActionCode, BOOL fHideCaret = FALSE, BOOL fChangeTrackerAndSetRange = TRUE );

        BOOL ShouldElementShowUIActiveBorder();

        BOOL IsElementUIActivatable();
        
        HRESULT CreateAdorner();

        void DestroyAdorner();

#ifndef NO_IME
    public:
        BOOL     IsIMEComposition( BOOL fProtectedOK = TRUE );
        BOOL     IsImeCancelComplete() { return _fImeCancelComplete; }
        BOOL     IsImeAlwaysNotify() { return _fImeAlwaysNotify; }
        CODEPAGE KeyboardCodePage() { return _codepageKeyboard; }
        static BOOL IsOnNT() { if (s_dwPlatformId == DWORD(-1)) CheckVersion(); return s_dwPlatformId == VER_PLATFORM_WIN32_NT; }
        static void CheckVersion();
        HIMC     ImmGetContext();
        void     ImmReleaseContext( HIMC himc );
        HRESULT  HandleImeComposition( SelectionMessage *pMessage );
        HRESULT  HandleImeMessage( SelectionMessage *pMessage,
                                   DWORD* pdwFollowUpAction,
                                   TRACKER_NOTIFY * peTrackerNotify );
        void     CheckDestroyIME(TRACKER_NOTIFY * peTrackerNotify);
        BOOL     IsImeEditable() { return TRUE; } // BUGBUG
        HRESULT  StartHangeulToHanja(TRACKER_NOTIFY * peTrackerNotify, IMarkupPointer * pPointer = NULL );
        HRESULT  TerminateIMEComposition( DWORD dwMode, TRACKER_NOTIFY * peTrackerNotify = NULL, DWORD * pdwFollowUpActionFlag = NULL );

        //
        // Glue functions to create/access/destroy the embedded CIme object
        //

        HRESULT  StartCompositionGlue( BOOL IsProtected, TRACKER_NOTIFY * peTrackerNotify );
        HRESULT  CompositionStringGlue( const LPARAM lparam, TRACKER_NOTIFY * peTrackerNotify );
        HRESULT  EndCompositionGlue(TRACKER_NOTIFY * peTrackerNotify);
        HRESULT  NotifyGlue( const WPARAM wparam, const LPARAM lparam, TRACKER_NOTIFY * peTrackerNotify );
        HRESULT  CompositionFullGlue( TRACKER_NOTIFY * peTrackerNotify );
        HRESULT  PostIMECharGlue( TRACKER_NOTIFY * peTrackerNotify );
#endif
        
#if DBG == 1
        void DumpNotify(TRACKER_NOTIFY eNotify, BOOL fBefore);

#endif
       

        void ReleaseTempMarkupPointers();
        
        //
        // Instance Vars
        //
        

        CHTMLEditor*                    _pEd;                // The editor


        IMarkupPointer*                 _pStartContext;

        IMarkupPointer*                 _pEndContext;

        IMarkupPointer*                 _pStartTemp;

        IMarkupPointer*                 _pEndTemp;

        IMarkupPointer *                _pPointerBeganTyping;   // Points to where we last began typing

        IHTMLElement*                   _pIEditableElement;     // The Editable Element 
        
        SelectionMessage                _lastMessage;

        SelectionMessage                _nextMessage;           // The Message after the last.

        ELEMENT_TAG_ID                  _eContextTagId;
        
        BOOL                            _fContextEditable:1;    // The Editable ness of the Edit Context
        
        BOOL                            _fContextAcceptsHTML:1;    // The Editable ness of the Edit Context

        BOOL                            _fParentEditable:1;     // Editableness of parent

        BOOL                            _fHadGrabHandles:1;     // has this control had grab handles ?

        BOOL                            _fDrillIn:1;            // is this context change due to a "drill in"
        
        BOOL                            _fIgnoreSetEditContext:1; // if true - ignore SetEditContext calls

        BOOL                            _fLastMessageValid:1;

        BOOL                            _fNextMessageValid:1; // Bubble this message after the last
        
        BOOL                            _fNoScope:1;

        BOOL                            _fInCapture:1;

        BOOL                            _fInTimer:1;
        
        BOOL                            _fOverwriteMode:1;

        BOOL                            _fIgnoreExitTree:1;     // Special mode to ignore exit tree notifications

        BOOL                            _fDontChangeTrackers:1; // Special mode to not change the trackers.

        BOOL                            _fPendingAdornerPosition:1; // We're expecting a notification from an adorner telling us when it's been positioned 

        BOOL                            _fPendingUndo:1;

        BOOL                            _fPendingTrackerNotify:1; // we have a pending Tracker Notification

        BOOL                            _fEditFocusAllowed:1;         // Set if the FireOnBeforeX event fails.

        BOOL                            _fPositionedSet:1;  // Has the Positioned Bit been set on this element 

        BOOL                            _fPositioned:1;     // Is this element positioned ?

        BOOL                            _fShiftCaptured:1;  // Was the shift key captured for direction change?

        BOOL                            _fHaveTypedSinceLastUrlDetect:1;    // Have typed since last URL autodetect

        BOOL                            _fInitSequenceChecker:1;            // Has the sequence checker been initialized?

        DWORD                           _pendingFollowUp;

        TRACKER_NOTIFY                  _pendingTrackerNotify;  // A pending Tracker Notify Code
        
        CEditTracker*                   _pActiveTracker;        // The currently active tracker.

        CActiveControlAdorner           * _pAdorner;              // The current active focus-adorner

        unsigned int                    _lastStringId;            // The last string we loaded.

        CISCList*                       _pISCList;              // the list of Input Sequence Checkers that is loaded

#ifndef NO_IME
        CIme*                           _pIme;

        BOOL                            _fImeCancelComplete;  // completed cancellation

        BOOL                            _fImeAlwaysNotify;

        BOOL                            _fIgnoreImeCharMsg;

        CODEPAGE                        _codepageKeyboard;

        static DWORD                    s_dwPlatformId;
#endif
};

inline BOOL
CSelectionManager::IsContextEditable()
{
    return _fContextEditable;
}

inline BOOL
CSelectionManager::CanContextAcceptHTML()
{
    return _fContextAcceptsHTML;
}

inline BOOL
CSelectionManager::IsParentEditable()
{
    return _fParentEditable;
}


inline IMarkupPointer* 
CSelectionManager::GetStartEditContext()
{
    return _pStartContext;
}

inline IMarkupPointer* 
CSelectionManager::GetEndEditContext()
{
    return _pEndContext;
}      

inline BOOL
CSelectionManager::HadGrabHandles()
{
   return _fHadGrabHandles ;
}

inline void 
CSelectionManager::SetIgnoreEditContext(BOOL fSetIgnoreEdit)
{
    _fIgnoreSetEditContext = fSetIgnoreEdit;
}

inline BOOL 
CSelectionManager::IsIgnoreEditContext()
{
    return _fIgnoreSetEditContext;
}


inline IHTMLViewServices* 
CSelectionManager::GetViewServices() 
{ 
    return _pEd->GetViewServices(); 
}

inline IMarkupServices * 
CSelectionManager::GetMarkupServices() 
{ 
    return _pEd->GetMarkupServices(); 
}

#if DBG == 1
inline IEditDebugServices*
CSelectionManager::GetEditDebugServices()
{
    return _pEd->GetEditDebugServices();
}
#endif

#endif //_SELMAN_HXX_
