//+------------------------------------------------------------------------
//
//  File:       Edtrack.cxx
//
//  Contents:   Edit Trackers
//
//  Contents:   CEditTracker, CCaretTracker, CSelectTracker
//       
//  History:    07-12-98 marka - created
//
//-------------------------------------------------------------------------

#ifndef _EDTRACK_HXX_
#define _EDTRACK_HXX_ 1

#ifndef X_RESOURCE_H_
#define X_RESOURCE_H
#include "resource.h"    
#endif

#ifndef X_SELMAN_HXX_
#define X_SELMAN_HXX_
#include "selman.hxx"
#endif

#ifndef X_EDUTIL_HXX_
#define X_EDUTIL_HXX_
#include "edutil.hxx"
#endif

#ifndef _X_EDADORN_HXX_
#define _X_EDADORN_HXX_
#include "edadorn.hxx"
#endif

MtExtern( CEditTracker )
MtExtern( CCaretTracker )
MtExtern( CSelectTracker )
MtExtern( CControlTracker )

class CGrabHandleAdorner;
class CActiveControlAdorner;
class CSpringLoader;
class CHTMLEditor;

#define CARET_XPOS_UNDEFINED -9999

//
// An enumeration of the type of notifications we expect to recieve.
//

enum TRACKER_NOTIFY
{
    TN_NONE,
    TN_BECOME_PASSIVE,
    TN_END_TRACKER,
    TN_END_TRACKER_NO_CLEAR,
    TN_END_TRACKER_SHIFT_START,
    TN_HIDE_CARET,
    TN_END_SELECTION,
    TN_TIMER_TICK,
    TN_FIRE_DELETE,
    TN_FIRE_DELETE_REBUBBLE,
    TN_KILL_ADORNER,
    TN_END_TRACKER_RESELECT,
    TN_END_TRACKER_REBUBBLE_ADORN,
    TN_END_TRACKER_RESELECT_REBUBBLE,
    TN_END_TRACKER_CREATE_CARET,
    TN_REBUBBLE,
    TN_END_TRACKER_OBJECT_ACTIVE,
    TN_END_TRACKER_POS_SELECT,    
    TN_END_TRACKER_POS_CONTROL,
    TN_END_TRACKER_POS_CARET,
    TN_END_TRACKER_POS_CARET_REBUBBLE,
    TN_END_TRACKER_START_IME,
    TN_DISABLE_IME
};

enum TRACKER_CREATE_FLAGS
{
    TRACKER_CREATE_GOACTIVE             = 0x01,
    TRACKER_CREATE_ACTIVEONMOVE         = 0x02,
    TRACKER_CREATE_NOTATBOL             = 0x04,
    TRACKER_CREATE_ATLOGICALBOL         = 0x08,
    TRACKER_CREATE_STARTFROMSHIFTKEY    = 0x10
};

//
// marka - the Tracker is a State Machine, the states you can transition between
// are managed by the SelectTracker::GetAction() call - that describes the transitions between states
//
// You need to see the diagram of the states to make sense of this ( ask SujalP, CarlEd or me ).
//

//
// The states the select tracker can be in
//
enum SELECT_STATES {
    ST_START,           //  0
    ST_WAIT1,           //  1
    ST_DRAGOP,          //  2
    ST_MAYDRAG,         //  3
    ST_WAITBTNDOWN1,    //  4
    ST_WAIT2,           //  5
    ST_DOSELECTION,     //  6
    ST_WAITBTNDOWN2,    //  7
    ST_SELECTEDWORD,    //  8
    ST_SELECTEDPARA,    //  9
    ST_WAIT3RDBTNDOWN,  // 10
    ST_MAYSELECT1,      // 11
    ST_MAYSELECT2,      // 12
    ST_STOP,            // 13
    ST_PASSIVE,         // 14 
    ST_KEYDOWN          // 15 // A 'psuedo-state' as we just become passive anyway.    
} ;



// The various actions to be taken upon receipt of message
enum ACTIONS {
     A_UNK, A_ERR, A_DIS, A_IGN,
  // A_From_To              old name(s)
     A_1_2,                // A_A50
     A_1_4,                // A_A30
     A_1_14,               // A_B50
     A_2_14,               // A_A60,  due to leftbutton up
     A_2_14r,              // A_b50   due to rightbutton up,
     A_3_2,                // A_A90   due to timer
     A_3_2m,               // A_b00   due to move
     A_3_14,               // A_A80
     A_4_8,                // A_b10
     A_4_14,               // A_C20
     A_4_14m,              // A_A80, b05  mousemove + click
     A_5_6,                // A_A20
     A_5_7,                // A_A00
     A_6_6,                // A_B30
     A_6_6m,               // A_B40   due to move
     A_6_14,               // A_B20
     A_7_8,                // A_B10. B60
     A_7_14,               // A_C20, B50
     A_8_6,                // A_B80
     A_8_10,               // A_C10
     A_9_6,                // A_C00
     A_9_14,               // A_B20, b90
     A_10_9,               // A_B70
     A_10_14,              // A_C20     via char
     A_10_14m,             ///A_B20     via mouseMove
     A_11_6,               // A_B80, c40
     A_11_14,              // A_B50, c30
     A_12_6,               // A_B80, c60
     A_12_14,              // A_B20, c50
     A_1_15,
     A_3_15,
     A_4_15,
     A_5_15,
     A_7_15, 
     A_8_15,
     A_9_15,
     A_10_15,
     A_12_15
} ;

typedef struct {
    UINT _iJMessage ;               // The message
    ACTIONS _aAction[ST_STOP+1] ;   // The actions to take on this message for
                                    // various states
} ACTION_TABLE ;

#if DBG == 1
void TrackerNotifyToString(TCHAR* pAryMsg, TRACKER_NOTIFY eNotify );

#endif

//
// A CEditTracker is the abstract base class from which all trackers are derived.
//
class CEditTracker
{

public:

    DECLARE_MEMALLOC_NEW_DELETE(Mt(CEditTracker))

    CEditTracker( CSelectionManager* pManager );
                

    STDMETHODIMP_(ULONG)    AddRef(void)    { return ++ _ulRef ; }
    STDMETHODIMP_(ULONG)    Release(void) ;
    
    virtual HRESULT Init2( 
                            CSelectionManager*      pManager,
                            SelectionMessage *      pMessage, 
                            DWORD*                  pdwFollowUpAction, 
                            TRACKER_NOTIFY *        peTrackerNotify,
                            DWORD                   dwTCFlags = 0) = 0;

    virtual HRESULT Init2( 
                            CSelectionManager*      pManager,
                            IMarkupPointer*         pStart, 
                            IMarkupPointer*         pEnd, 
                            DWORD*                  pdwFollowUpAction, 
                            TRACKER_NOTIFY *        peTrackerNotify,
                            DWORD                   dwTCFlags = 0,
                            CARET_MOVE_UNIT inLastCaretMove  = CARET_MOVE_NONE ) = 0;

    static BOOL ShouldStartTracker( 
                CSelectionManager* pManager,
                SelectionMessage* pMessage,
                ELEMENT_TAG_ID eTag,
                IHTMLElement* pIElement,
                IHTMLElement** ppIEditThisElement = NULL )  ;
    
    virtual HRESULT HandleMessage( 
                SelectionMessage *pMessage, 
                DWORD* pdwFollowUpAction, 
                TRACKER_NOTIFY * peTrackerNotify ) = 0 ;

    virtual HRESULT Notify(
                TRACKER_NOTIFY inNotify, 
                SelectionMessage *pMessage, 
                DWORD* pdwFollowUpAction, 
                TRACKER_NOTIFY * peTrackerNotify  ) = 0 ;

    virtual HRESULT Position( 
                IMarkupPointer* pStart,
                IMarkupPointer* pEnd,
                BOOL fNotAtBOL = TRUE,
                BOOL fAtLogicalBOL = FALSE) = 0;

    virtual HRESULT GetLocation(POINT *pPoint);

    virtual VOID OnEditFocusChanged();
    
    HRESULT static GetTagIdFromMessage( 
                SelectionMessage* pMessage, 
                ELEMENT_TAG_ID* peTag , 
                IHTMLDocument2 * pIDoc = NULL,
                IHTMLViewServices * pVS = NULL ,
                IMarkupServices* pMarkup = NULL );

    HRESULT static GetElementAndTagIdFromMessage(
                        SelectionMessage* pMessage,
                        IHTMLElement** ppElement, 
                        ELEMENT_TAG_ID* peTag ,
                        IHTMLDocument2 * pIDoc ,
                        IHTMLViewServices * pVS = NULL ,
                        IMarkupServices * pMark = NULL  );
                        
    HRESULT static GetElementFromMessage( 
        CSelectionManager* pManager, 
        SelectionMessage* peMessage, 
        IHTMLElement** ppElement );

    BOOL static IsTablePart( ELEMENT_TAG_ID eTag );

    HRESULT ConstrainPointer( IMarkupPointer* pPointer, BOOL fDirection = TRUE );

    VOID SetSelectionType(SELECTION_TYPE eType)
    {
        _eType = eType;
    }
    
    SELECTION_TYPE GetSelectionType()
    {
        return _eType;
    }

    IMarkupServices* GetMarkupServices()
    {
        return _pManager->GetMarkupServices();
    }

    IHTMLElement* GetEditableElement()
    {
        return _pManager->GetEditableElement();
    }
    
    IHTMLViewServices* GetViewServices()
    {
        return _pManager->GetViewServices();
    }

    CSelectionManager *GetSelectionManager()
    {
        return _pManager;
    }

    CHTMLEditor* GetEditor()
    {
        return _pManager->GetEditor();
    }

#if DBG == 1
    IEditDebugServices* GetEditDebugServices()
    {
        return _pManager->GetEditDebugServices();        
    }

    void DumpTree( IMarkupPointer* pPointer )
    {
        _pManager->DumpTree( pPointer);
    }

    long GetCp( IMarkupPointer* pPointer )
    {
        return _pManager->GetCp(pPointer);
    }

    void SetDebugName( IMarkupPointer* pPointer, LPCTSTR strDebugName )
    {
        _pManager->SetDebugName( pPointer, strDebugName );
    }
#endif

    virtual BOOL IsActive(); // is this tracker type - "ACTIVE ?"

    virtual BOOL IsListeningForMouseDown();

    BOOL IsInFireOnSelectStart() { return _fInFireOnSelectStart ; }

    VOID SetFailFireOnSelectStart( BOOL fFail )
    {
        _fFailFireOnSelectStart = fFail;
    }
    CSpringLoader * GetSpringLoader();


    BOOL IsInWindow(POINT pt, BOOL fDoClientToScreen = FALSE );

    VOID GetMousePoint(POINT *ppt, BOOL fDoScreenToClient = TRUE );

    BOOL IsMessageInWindow( SelectionMessage* peMessage );
    
    HRESULT FireSelectStartMessage(SelectionMessage* pMessage);

    BOOL FireOnBeforeEditFocus( IHTMLElement* pIElement);
    
    virtual BOOL IsPointerInSelection( IMarkupPointer* pStart ,  POINT * pptGlobal, IHTMLElement* pIElementOver    );

    VOID TakeCapture();
    VOID ReleaseCapture();

    Direction static GetPointerDirection(CARET_MOVE_UNIT moveDir);
 
    HRESULT MovePointer(  
        CARET_MOVE_UNIT         inMove, 
        IMarkupPointer*         pPointer,
        LONG                    *plXPosForMove,
        BOOL                    *pfNotAtBOL,
        BOOL                    *pfAtLogicalBOL,
        Direction*              peMvDir = NULL);

    VOID SetRecalculateBOL(BOOL fRecalcBOL) { _fRecalcBOL = fRecalcBOL;}

    VOID virtual CalculateBOL() { return ; }
    
    BOOL virtual GetNotAtBOL();
    
    virtual HRESULT SetNotAtBOL(BOOL fNotAtBOL) {_fNotAtBOL = fNotAtBOL; return S_OK;}

    HRESULT AdjustPointerForInsert( 
        IMarkupPointer * pWhereIThinkIAm, 
        BOOL fNotAtBOL,
        Direction inblockdir = LEFT,
        Direction intextdir = LEFT,
        DWORD dwOptions = ADJPTROPT_None );

    BOOL GetAtLogicalBOL() { return _fAtLogicalBOL; }
    HRESULT SetAtLogicalBOL(BOOL fAtLogicalBOL) {_fAtLogicalBOL= fAtLogicalBOL; return S_OK;}

    //
    // The flow layout we're in has been deleted.
    //
    virtual BOOL AdjustForDeletion(IMarkupPointer* pPointer );

protected:

    virtual ~CEditTracker(); // destructor is now private tracker cannot be destroyed directly.

    HRESULT MustDelayBackspaceSpringLoad(CSpringLoader *psl, IMarkupPointer *pPointer, BOOL *pbDelaySpringLoad);



    CSelectionManager*              _pManager;                  // The driver driving us.
    SELECTION_TYPE                  _eType;                     // The type of selection this tracker tracks
    HWND                            _hwndDoc;

    BOOL                            _fNotAtBOL;
    BOOL                            _fAtLogicalBOL;
    
    BOOL                            _fInFireOnSelectStart:1;    // Flag to say we're in Fire OnSelectStart
    BOOL                            _fFailFireOnSelectStart:1;  // Force Failure of OnSelectStart
    BOOL                            _fRecalcBOL:1;
    BOOL                            _fShiftCapture:1;           // Is the shift key captured for toggling direction?

    LONG                            _ulRef;                     // Ref-Count
};


enum POSCARETOPTION
{
    POSCARETOPT_None                = 0,
    POSCARETOPT_DoNotAdjust         = 1
};

class CCaretTracker : public CEditTracker
{
    friend class CSelectionManager;
#ifndef NO_IME
    friend class CIme;
#endif
    
    public:

        DECLARE_MEMCLEAR_NEW_DELETE(Mt(CCaretTracker));
        
        DECLARE_CLASS_TYPES(CCaretTracker, CEditTracker);
        CCaretTracker( CSelectionManager* pManager ) : super (pManager) {}
        virtual HRESULT Init2( 
                                CSelectionManager*      pManager,
                                SelectionMessage *      pMessage, 
                                DWORD*                  pdwFollowUpAction, 
                                TRACKER_NOTIFY *        peTrackerNotify,
                                DWORD                   dwTCFlags = 0);

        virtual HRESULT Init2( 
                                CSelectionManager*      pManager,
                                IMarkupPointer*         pStart, 
                                IMarkupPointer*         pEnd, 
                                DWORD*                  pdwFollowUpAction, 
                                TRACKER_NOTIFY *        peTrackerNotify,
                                DWORD                   dwTCFlags = 0,
                                CARET_MOVE_UNIT inLastCaretMove = CARET_MOVE_NONE );

        VOID Init();
        
        
        HRESULT HandleMessage( 
                SelectionMessage *  pMessage, 
                DWORD*              pdwFollowUpAction, 
                TRACKER_NOTIFY *    peTrackerNotify );

        HRESULT Notify(
                TRACKER_NOTIFY      inNotify, 
                SelectionMessage *  pMessage, 
                DWORD*              pdwFollowUpAction, 
                TRACKER_NOTIFY *    peTrackerNotify  );


        HRESULT Position( 
                IMarkupPointer*     pStart,
                IMarkupPointer*     pEnd,
                BOOL                fNotAtBOL = TRUE,
                BOOL                fAtLogicalBOL = FALSE);
                
        HRESULT PositionCaretAt ( 
                IMarkupPointer *    pPointer, 
                BOOL                fNotAtBOL,
                BOOL                fAtLogicalBOL,
                CARET_DIRECTION     eDir              = CARET_DIRECTION_INDETERMINATE,
                DWORD               dwPositionOptions = POSCARETOPT_None, 
                DWORD               dwAdjustOptions   = ADJPTROPT_None );
                
        static BOOL ShouldStartTracker( 
                CSelectionManager* pManager,
                SelectionMessage* pMessage,
                ELEMENT_TAG_ID eTag ,
                IHTMLElement* pIElement,
                IHTMLElement** ppIEditThisElement = NULL   ) ;
                
        static HRESULT SetCaretVisible( 
            IHTMLDocument2*         pIDoc, 
            BOOL                    fVisible );

        static CARET_MOVE_UNIT GetMoveDirectionFromMessage( 
            SelectionMessage*       pMessage,
            BOOL                    fRightToLeft);

        HRESULT MoveCaret(  
            CARET_MOVE_UNIT         inMove, 
            IMarkupPointer*         pPointer,
            BOOL                    fMoveCaret );

        BOOL IsListeningForMouseDown();

        HRESULT GetLocation(POINT *pPoint);

        VOID OnEditFocusChanged();

        BOOL AdjustForDeletion(IMarkupPointer* pPointer );

        HRESULT HandleSpace( OLECHAR t);

        VOID SetCaretShouldBeVisible( BOOL fVisible );

    protected:
        ~CCaretTracker();

    private:
                          
        HRESULT PositionCaretFromMessage( SelectionMessage * pMessage );
        
        HRESULT Backspace( SelectionMessage* pMessage, BOOL fDirection );
        
        HRESULT DeleteNextChar( IMarkupPointer * pPos );

        HRESULT GetCaretPointer( IMarkupPointer** ppPointer );

        HRESULT HandleMouseMessage( 
                SelectionMessage *pMessage, 
                DWORD* pdwFollowUpAction, 
                TRACKER_NOTIFY * peTrackerNotify );
                
        HRESULT HandleChar( 
                SelectionMessage *pMessage, 
                DWORD* pdwFollowUpAction, 
                TRACKER_NOTIFY * peTrackerNotify );

        HRESULT InsertText( 
                OLECHAR    *    pText,
                LONG            lLen,
                IHTMLCaret *    pc );

        HRESULT HandleKeyUp( 
                SelectionMessage *pMessage, 
                DWORD* pdwFollowUpAction, 
                TRACKER_NOTIFY * peTrackerNotify );

        HRESULT HandleKeyDown( 
                SelectionMessage *pMessage, 
                DWORD* pdwFollowUpAction, 
                TRACKER_NOTIFY * peTrackerNotify );

        BOOL    ShouldEnterExitList(IMarkupPointer *pPosition, IHTMLElement **ppElement);
        
        HRESULT HandleEnter(IHTMLCaret *pc, BOOL fShift, BOOL fCtrl);

        HRESULT HandleInputLangChange();

        BOOL ShouldCaretBeVisible();


        
        HRESULT CreateAdorner();

        VOID DestroyAdorner();

        HRESULT UrlAutodetectCurrentWord( OLECHAR * pChar );

        BOOL    IsContextEditable();
        
        BOOL    IsCaretInPre( IHTMLCaret * pCaret );        

        HRESULT SetNotAtBOL(BOOL fNotAtBOL);

        HRESULT IsQuotedURL(IHTMLElement *pAnchorElement);

        LONG                            _lScreenXPosForVertMove;    // Cached position of last vertical move
        BOOL                            _fValidPosition:1;          // THe caret cannot position its start and end. Happens for eg. Trying to place pointer in an input, or a checkbox.
        BOOL                            _fHaveTypedSinceLastUrlDetect:1;
        BOOL                            _fCaretShouldBeVisible:1;           // Is the Caret Visible ?
};


class CSelectTracker: public CEditTracker
{
    public:

        DECLARE_MEMCLEAR_NEW_DELETE(Mt(CSelectTracker));
        
        DECLARE_CLASS_TYPES(CSelectTracker, CEditTracker);
        CSelectTracker( CSelectionManager* pManager ) : super (pManager) {}
        virtual HRESULT Init2( 
                                CSelectionManager*      pManager,
                                SelectionMessage *      pMessage, 
                                DWORD*                  pdwFollowUpAction, 
                                TRACKER_NOTIFY *        peTrackerNotify,
                                DWORD                   dwTCFlags = 0);

        virtual HRESULT Init2( 
                                CSelectionManager*      pManager,
                                IMarkupPointer*         pStart, 
                                IMarkupPointer*         pEnd, 
                                DWORD*                  pdwFollowUpAction, 
                                TRACKER_NOTIFY *        peTrackerNotify,
                                DWORD                   dwTCFlags = 0,
                                CARET_MOVE_UNIT inLastCaretMove = CARET_MOVE_NONE );

        HRESULT Init();
        


        virtual VOID CalculateBOL() ;

        HRESULT HandleMessage( 
                SelectionMessage *pMessage, 
                DWORD* pdwFollowUpAction, 
                TRACKER_NOTIFY * peTrackerNotify );

                
        HRESULT Notify(
                TRACKER_NOTIFY inNotify, 
                SelectionMessage *pMessage, 
                DWORD* pdwFollowUpAction, 
                TRACKER_NOTIFY * peTrackerNotify  ) ;               

        static BOOL ShouldStartTracker( 
                CSelectionManager* pManager,
                SelectionMessage* pMessage,
                ELEMENT_TAG_ID eTag,
                IHTMLElement* pIElement,
                IHTMLElement** ppIEditThisElement = NULL ) ;

        HRESULT Position( 
                IMarkupPointer* pStart,
                IMarkupPointer* pEnd,
                BOOL            fNotAtBOL = TRUE,
                BOOL            fAtLogicalBOL = FALSE);        

        VOID VerifyOkToStartSelection( SelectionMessage * pMessage );

        BOOL GetMadeSelection();

        IMarkupPointer* GetStartSelection();

        IMarkupPointer* GetEndSelection();

        HRESULT GetStartSelectionForSpringLoader( IMarkupPointer* pPointer );
        
        static VOID InitMetrics();    

        BOOL IsActive();

        HRESULT MoveSelection( CARET_MOVE_UNIT inCaretMove );

        HRESULT GetCaretStartPoint( 
                                CARET_MOVE_UNIT inCaretMove, 
                                IMarkupPointer* pCopyStart );

        BOOL IsListeningForMouseDown();


        HRESULT DoTimerDrag( );     

        BOOL IsPointerInSelection( IMarkupPointer* pStart ,  POINT * pptGlobal, IHTMLElement* pIElementOver    );

        VOID SetLastCaretMove( CARET_MOVE_UNIT inCaretMove )
        {
            _lastCaretMove = inCaretMove;
        }
        
        HRESULT GetLocation(POINT *pPoint);

        HRESULT AdjustPointersForChar();

        BOOL EndPointsInSameFlowLayout();

        BOOL IsPassive();

        VOID StopTimer();

    protected:
        ~CSelectTracker();

    private:

        BOOL IsJumpOverAtBrowse( ELEMENT_TAG_ID eTag );
        
        BOOL IsInValidPlaceForScroll(IMarkupPointer* pScrollPointer );

        HRESULT ScrollMessageIntoView( SelectionMessage* pMessage );
        
        HRESULT ScrollPointerIntoView( IMarkupPointer* pScrollPointer, BOOL fNotAtBOL, SelectionMessage* pMessage = NULL );        

        BOOL IsPointInEditContextContent(POINT ptGlobal);
        
        VOID SetState(SELECT_STATES inState, BOOL fIgnorePassive = FALSE );

        BOOL IsBetweenBlocks( IMarkupPointer* pPointer );
        
        BOOL IsAtEdgeOfTable( Direction iDirection, IMarkupPointer* pPointer );
        
        HRESULT MoveWord( IMarkupPointer* pPointer,
                           MOVEUNIT_ACTION  muAction );
        VOID StartTimer();

        
        HRESULT ConstrainSelection( BOOL fConstrainStart = FALSE, POINT* pptGlobal = NULL  );
                
        HRESULT AdjustSelection(BOOL * pfAdjustedSel);
        
        HRESULT ScanForEnterBlock( 
                    Direction iDirection, 
                    IMarkupPointer* pPointer, 
                    BOOL* pfFoundEnterBlock, 
                    DWORD *pdwBreakCondition = NULL );

        HRESULT ScanForLastExitBlock( 
                    Direction iDirection, 
                    IMarkupPointer* pPointer, 
                    DWORD* pdwBreakCondition = NULL );

        HRESULT ScanForLastEnterBlock( 
                    Direction iDirection, 
                    IMarkupPointer* pPointer, 
                    DWORD* pdwBreakCondition  = NULL );


        HRESULT ScanForLastEnterOrExitBlock( 
                    Direction iDirection, 
                    IMarkupPointer* pPointer, 
                    DWORD dwTerminateCondtion ,
                    DWORD* pdwBreakCondition = NULL );
            
        BOOL IsValidForAdjust( ELEMENT_TAG_ID eTag );
        
        HRESULT HandleMessagePrivate( 
                SelectionMessage *pMessage, 
                DWORD* pdwFollowUpAction, 
                TRACKER_NOTIFY * peTrackerNotify );
                
        HRESULT HandleChar( 
                SelectionMessage *pMessage, 
                DWORD* pdwFollowUpAction, 
                TRACKER_NOTIFY * peTrackerNotify );

        HRESULT HandleKeyUp( 
                SelectionMessage *pMessage, 
                DWORD* pdwFollowUpAction, 
                TRACKER_NOTIFY * peTrackerNotify );

        HRESULT HandleKeyDown( 
                SelectionMessage *pMessage, 
                DWORD* pdwFollowUpAction, 
                TRACKER_NOTIFY * peTrackerNotify );

        HRESULT OnTimerTick(                 
                SelectionMessage *pMessage, 
                DWORD* pdwFollowUpAction, 
                TRACKER_NOTIFY * peTrackerNotify   );        
                
        VOID BecomePassive( TRACKER_NOTIFY * peTrackerNotify = NULL  );

        HRESULT ClearSelection();
        
        EndActiveTrack();
        
        //
        // Methods
        //

        
        ACTIONS GetAction ( SelectionMessage *pMessage) ;

        BOOL IsValidMove (SelectionMessage *pMessage);
        
        HRESULT BeginSelection(SelectionMessage* pMessage, DWORD* pdwFollowUpAction, TRACKER_NOTIFY * peTrackerNotify );

        HRESULT DoSelection(SelectionMessage* pMessage, BOOL fAdjustSelection = FALSE );
        
        HRESULT DoWordSelection(SelectionMessage* pMessage, BOOL* pfAdjustedSel, BOOL fRightOfCp, BOOL fFurtherInStory);

        HRESULT SetWordPointer( IMarkupPointer* pPointerToSet, BOOL fFurtherInStory, BOOL fSetFromGivenPointer = FALSE, BOOL* pfStartWordSel = NULL , BOOL *pfWordPointerSet = NULL  );
        
        HRESULT DoSelectWord( SelectionMessage* pMessage );

        HRESULT DoSelectParagraph( SelectionMessage* pMessage );       

        BOOL CheckSelectionWasReallyMade();

        BOOL ConstrainEndToContainer( );

        BOOL AdjustForSiteSelectable();
        
        VOID SetMadeSelection( BOOL fMadeSelection );

        BOOL GetMoveDirection();

        BOOL GuessDirection(POINT* ppt);
        
        HRESULT MovePointersToTrueStartAndEnd(IMarkupPointer* pTrueStart, IMarkupPointer* pTrueEnd, BOOL *pfSwap = NULL , BOOL *pfEqual = NULL);
        
        BOOL IsAtWordBoundary( IMarkupPointer* pPointer, BOOL * pfAtStart = NULL , BOOL* pfAtEnd = NULL, BOOL fAlwaysTestIfAtEnd = FALSE );
        
        BOOL IsAdjacentToBlock( 
                            IMarkupPointer* pPointer,
                            Direction iDirection,
                            DWORD *pdwBreakCondition = NULL );
                            
        void EndAndStartCaret( TRACKER_NOTIFY * peTrackerNotify );

        void ResetSpringLoader( CSelectionManager* pManager, IMarkupPointer* pStart, IMarkupPointer* pEnd );

#if DBG != 1
        inline 
#endif
        HRESULT MoveEndToPointer( IMarkupPointer* pPointer );

#ifndef NO_IME
        //
        // IME Reconversion support
        //
        
        HRESULT HandleImeStartComposition( SelectionMessage *pMessage, DWORD* pdwFollowUpAction, TRACKER_NOTIFY * peTrackerNotify );
#if defined(IME_RECONVERSION)
        HRESULT HandleImeNotify( SelectionMessage * pMessage, TRACKER_NOTIFY * peTrackerNotify );
        HRESULT HandleImeRequest( SelectionMessage * pMessage, TRACKER_NOTIFY * peTrackerNotify );
        HRESULT ScoopTextForReconversion( LONG cchMax, LONG * pcch, TCHAR * pch );
#endif // IME_RECONVERSION
#endif // !NO_IME

#if DBG == 1

        void DumpSelectState(SelectionMessage* peMessage,  ACTIONS inAction,DWORD followUp, BOOL fInTimer = FALSE ) ;

        void StateToString(TCHAR* pAryMsg, SELECT_STATES inState );
        
        void ActionToString(TCHAR* pAryMsg, ACTIONS inState );

        void ValidateWordPointer(IMarkupPointer* pPointer );
        
#endif
        HRESULT CheckSwap( );
        
        //
        // Instance Variables.
        //
        

        LONG _anchorMouseX;                                              // The X-coord of Mouse Anchor

        LONG _anchorMouseY;                                            // The Y-coord of Mouse Anchor

        LONG _curMouseX;                                                // Current Mouse X-coord

        LONG _curMouseY;                                                // Current Mouse Y-coord.

        SelectionMessage            _firstMessage;                  // The first message we rec'd.
        
        IMarkupPointer *            _pStartPointer;

        IMarkupPointer *            _pEndPointer;

        IMarkupPointer *            _pTestPointer;

        IMarkupPointer *            _pWordPointer; 

        IMarkupPointer *            _pPrevTestPointer;

        IMarkupPointer *            _pSwapPointer; 
        
        ISelectionRenderingServices *_pSelRenSvc; 

        BOOL                        _fActive:1;                     // Are we actively tracking things ?
        
        BOOL                        _fLeftButtonStart:1;            // Did thsi selection start with the Left MouseButton Down ?     

        BOOL                        _fEndConstrained:1;             // Has the End been constrained ?

        BOOL                        _fMadeSelection:1;

        BOOL                        _fShift:1;                      // The Shift Key has been used to extend/create a selection

        BOOL                        _fAddedSegment:1;               // Used for debugging. TRUE if we successfully added a SEGMENT.

        BOOL                        _fDragDrop:1;                   // Tracker is Dragging.

        BOOL                        _fInWordSel:1;                  // In Word Selection Mode

        BOOL                        _fLastFurtherInStory:1;         // Was the last time we called DoSelectWord - were we selecting further in the story  ?

        BOOL                        _fReversed:1;                   // Have we flipped out of word-sel by reversing direction ?

        BOOL                        _fReversedDirection:1;          // The direction we're moving when we flipped out of word selection mode.
        
        BOOL                        _fWordPointerSet:1;             // Has the WordPointer been set ?
        
        BOOL                        _fWordSelDirection:1;           // The direction in which words are being selected

        BOOL                        _fStartAdjusted:1;              // Has the Start been adjusted for Word Selection

        BOOL                        _fInSelectParagraph:1;          // When in this mode - we always select paragraphs.

        BOOL                        _fDoubleClickWord:1;            // We double clicked on a word to begin selection 

        BOOL                        _fSwapDirection:1;         // The swap direction.
        
        SELECT_STATES               _fState;                        // Stores the current state of the state machine
        
        int                         _iSegment ;                     // The current segment we're managing.

        CARET_MOVE_UNIT             _lastCaretMove;
#if DBG == 1
        int                         _ctStartAdjusted;           // Debug count for the amount of times we've adjusted the start
        unsigned long               _ctScrollMessageIntoView;   // Debug count of ScrollMessage Into View.
#endif

};



class CControlTracker : public CEditTracker
{
    public:

        DECLARE_MEMCLEAR_NEW_DELETE(Mt(CControlTracker));
        
        DECLARE_CLASS_TYPES(CControlTracker, CEditTracker);
        CControlTracker( CSelectionManager* pManager ) : super (pManager) {}
        virtual HRESULT Init2( 
                                CSelectionManager*      pManager,
                                SelectionMessage *      pMessage, 
                                DWORD*                  pdwFollowUpAction, 
                                TRACKER_NOTIFY *        peTrackerNotify,
                                DWORD                   dwTCFlags = 0);

        virtual HRESULT Init2( 
                                CSelectionManager*      pManager,
                                IMarkupPointer*         pStart, 
                                IMarkupPointer*         pEnd, 
                                DWORD*                  pdwFollowUpAction, 
                                TRACKER_NOTIFY *        peTrackerNotify,
                                DWORD                   dwTCFlags = 0,
                                CARET_MOVE_UNIT inLastCaretMove = CARET_MOVE_NONE);

        VOID Init();                
        

        BOOL HasAdorner();
        
        HRESULT HandleMessage( 
                SelectionMessage *pMessage, 
                DWORD* pdwFollowUpAction, 
                TRACKER_NOTIFY * peTrackerNotify );
                
        HRESULT HandleMouseDown( 
                SelectionMessage *pMessage, 
                DWORD* pdwFollowUpAction, 
                TRACKER_NOTIFY * peTrackerNotify );

        HRESULT HandleMouseUp(
                SelectionMessage *pMessage,
                DWORD* pdwFollowUpAction,
                TRACKER_NOTIFY * peTrackerNotify );

  
                     
        HRESULT Notify(
                TRACKER_NOTIFY inNotify, 
                SelectionMessage *pMessage, 
                DWORD* pdwFollowUpAction, 
                TRACKER_NOTIFY * peTrackerNotify  );

        HRESULT Position( 
                IMarkupPointer* pStart,
                IMarkupPointer* pEnd,
                BOOL            fNotAtBOL = TRUE,
                BOOL            fAtLogicalBOL = FALSE);
                
        static BOOL ShouldStartTracker( 
                CSelectionManager* pManager,
                SelectionMessage* pMessage,
                ELEMENT_TAG_ID eTag,
                IHTMLElement* pIElement,
                IHTMLElement** ppIEditThisElement = NULL ) ;

        BOOL IsActive();

        IHTMLElement* GetControlElement() {  return _pIControlElement; }
        
        BOOL IsSameElementAsControl(         
                IHTMLElement* pIElement );

        BOOL IsSameElementAsControl( SelectionMessage* pMessage );
        
        enum HOW_SELECTED {
            HS_NONE,
            HS_FROM_ELEMENT, 
            HS_OUTER_ELEMENT
        } ;
        
        static BOOL IsElementSiteSelectable( 
                    CSelectionManager* pManager,
                    ELEMENT_TAG_ID eTag, 
                    IHTMLElement* pIElement,
                    CControlTracker::HOW_SELECTED *peHowSelected,
                    IHTMLElement** ppIFindElement = NULL );    

        static BOOL IsThisElementSiteSelectable(
                    CSelectionManager* pSelectionManager,
                    ELEMENT_TAG_ID eTag, 
                    IHTMLElement* pIElement );
                    
        BOOL IsPointerInSelection( IMarkupPointer* pStart, POINT * pptGlobal, IHTMLElement* pIElementOver             );

        VOID BecomeActiveOnFirstMove( SelectionMessage* pMessage );

        BOOL ShouldGoUIActiveOnFirstClick( IHTMLElement* pIHitElement, ELEMENT_TAG_ID eHitTag);

        BOOL IsMessageOverControl( SelectionMessage * pMessage );

        static BOOL IsSiteSelectable( ELEMENT_TAG_ID eTag );

        protected:
            ~CControlTracker();

        private:
            
            BOOL IsElementLocked();
            
            HRESULT   DoDrag(SelectionMessage* peMessage );
                        
            HRESULT SetControlElement( 
                                    ELEMENT_TAG_ID eTag, 
                                    IHTMLElement* pIElement)    ;
                                    
            HRESULT CreateAdorner(BOOL fScrollIntoView  );

            VOID DestroyAdorner();
            
            VOID    BecomePassive();

            VOID    BecomeActive(SelectionMessage* peMessage, BOOL fInMove = TRUE );
            
            VOID    VerifyOkToStartControlTracker( SelectionMessage * pMessage );

            HRESULT Select(BOOL fScrollIntoView);

            HRESULT UnSelect();

            BOOL IsInControlHandle( SelectionMessage* peMessage );

            BOOL IsPassive();
                
            BOOL ShouldClickInsideGoActive( ELEMENT_TAG_ID eTag);

            BOOL IsValidMove (SelectionMessage *pMessage);


#if 0

            //
            // Drag & Drop Control Methods;
            //
            BOOL BeginDrag(DRAG_ACTION *peDragAction);

            BOOL EndDrag( POINT pt , DRAG_ACTION *peDragAction, CRect* pEndDragRect );

            BOOL ShouldChangeCursor( LPCTSTR inCursorId );

            BOOL HostDrawDragFeedback(RECT* pRect);
            HRESULT DoPasteInFlow( SelectionMessage* peMessage, TRACKER_NOTIFY * peNotify  );
            HRESULT CommitMove(SelectionMessage * peMessage, TRACKER_NOTIFY * peNotify );
#endif             
            HRESULT CommitResize(SelectionMessage * peMessage);


            
            HRESULT DoResizeDrag( SelectionMessage* peMessage, DWORD* pFollowUpAction );

            HRESULT BecomeUIActive( SelectionMessage *pMessage);

            HRESULT HandleChar(
                SelectionMessage *pMessage,
                DWORD* pdwFollowUpAction,
                TRACKER_NOTIFY * peTrackerNotify );

            HRESULT HandleKeyDown(
                SelectionMessage *pMessage,
                DWORD* pdwFollowUpAction,
                TRACKER_NOTIFY * peTrackerNotify );
                
            //
            // Instance Variables
            //
            
            BOOL             _fPassive:1;

#if 0
            BOOL             _fInMove:1;
            BOOL             _fUseHostDrag:1;
            BOOL             _fDragElementDifferent:1; // The element we're dragging is 'different' from the control element
#endif
            BOOL             _fMouseUp:1;

            BOOL             _fActiveControl:1; // We adorn a UI Active control. Don't take events

            BOOL             _fIgnoreTimeElapsed:1;    // Bit to say to Ignore Time since event elapsed

            BOOL             _fPendingUIActive:1; // Becoming UI Active is pending.
            
            IHTMLElement*    _pIControlElement; // Element we are actively editing.

            int _startMouseX;                   // The X-coord of Mouse Anchor

            int _startMouseY;                   // The Y-coord of Mouse Anchor

            int _startMoveX;

            int _startMoveY;

            
            CGrabHandleAdorner * _pGrabAdorner; // The Adorner

            ULONG   _ulNextEvent ;              // Ignore Mouse Moves until this much time has passed

#if DBG == 1
            BOOL                          _fGotRButtonDown;
#endif

};

//
//
// Inlines
//
//

inline BOOL 
CSelectTracker::GetMadeSelection()
{
    return _fMadeSelection;
}

inline IMarkupPointer* 
CSelectTracker::GetStartSelection()
{
    return _pStartPointer;
}

inline IMarkupPointer*
CSelectTracker::GetEndSelection()
{
    return _pEndPointer;
}

inline BOOL
CControlTracker::IsActive()
{
    return( ! _fPassive );
}

inline BOOL
CControlTracker::IsPassive()
{
    return( _fPassive );
}


//+====================================================================================
//
// Method: MoveWord
//
// Synopsis: Wrapper to ViewServices MoveWord
//
//------------------------------------------------------------------------------------

inline HRESULT
CSelectTracker::MoveWord( IMarkupPointer* pPointer,
                          MOVEUNIT_ACTION  muAction)
{
    HRESULT hr = THR( GetViewServices()->MoveWord( pPointer,
                                          muAction,
                                          _pManager->GetStartEditContext(),
                                          _pManager->GetEndEditContext() ));

    AssertSz( !hr, "Failure in move word");
    
    RRETURN( hr ) ;    
}

#endif

