//----------------------------------------------------------------------------
//
//  Maintained by: alexa
//
//  Copyright: (C) Microsoft Corporation, 1994-1995.
//
//  File:       repeater.hxx | CRepeaterFrame class declaration.
//
//  Hungarian:
//             hrow - HROW
//             lfr  - CDetailFrame
//             plfr - CDetailFrame *
//
//----------------------------------------------------------------------------

#ifndef _REPEATER_HXX_
#   define _REPEATER_HXX_   1

#   ifndef _BASEFRM_HXX_
#       include "basefrm.hxx"
#   endif

#   ifndef _DATAFRM_HXX_
#       include "datafrm.hxx"
#   endif

#   ifndef _DETAIL_HXX_
#       include "detail.hxx"
#   endif


//----------------------------------------------------------------------------
//  F o r w a r d   c l a s s   d e c l a r a t i o n s.
//----------------------------------------------------------------------------

DEFINE_CLASS(RepeaterFrame);
DEFINE_CLASS(BaseFrameTemplate);
DEFINE_CLASS(DataFrameTemplate);
DEFINE_CLASS(DataFrame);
DEFINE_CLASS(DetailFrame);
DEFINE_CLASS(HeaderFrame);

//----------------------------------------------------------------------------
//  C o n s t a n t s.
//----------------------------------------------------------------------------

// please don't change the values of the following constants
// code is relying on the follwoing values.

const BOOL MOVE_UP   = 0;
const BOOL MOVE_DOWN = 1;

const BOOL POPULATE_UP   = MOVE_UP;
const BOOL POPULATE_DOWN = MOVE_DOWN;

const BOOL ROW  = 0;
const BOOL PAGE = 1;

// scolling type
const BOOL ROW_SET        = 1;
const BOOL DOCUMENT_SPACE = 0;

#define F3REPEATER_REPEAT_TIMER 4

//+---------------------------------------------------------------------------
//
// Class    CRepeaterFrame
//
//----------------------------------------------------------------------------

class CRepeaterFrame: public CBaseFrame
{

friend class CBaseFrameTemplate;
friend class CDataFrameTemplate;
friend class CDataFrame;
friend class CDetailFrame;
friend class CHeaderFrame;

typedef CBaseFrame super;

enum DRAGSELECT_FLAGS
{
    DRAGSELECT_SETCURRENTONLY   = 0x1,
    DRAGSELECT_CTRL_DRAG        = 0x2,
    DRAGSELECT_MOUSE_INSIDE     = 0x4,
    DRAGSELECT_SHIFT_DRAG       = 0x8,
};

    //+-----------------------------------------
    // Creation  API
    //------------------------------------------

private:

    // the only Constructor.
    CRepeaterFrame(CDoc * pDoc,
                   CBaseFrame * pParent,
                   CDetailFrame * pTemplate); // template of section repeated

    // Initialization member function used after
    // using the above constructor.
    HRESULT Init ();

    // Destructor.
    ~CRepeaterFrame();

public:

    virtual CSite::CTBag * GetTBag() { return getOwner()->TBag(); }

    virtual void DataSourceChanged();

    // property updating api
    virtual HRESULT UpdatePropertyChanges(UPDATEPROPS updFlag);

#if DBG == 1
    virtual HRESULT DrawBackground(CFormDrawInfo *pDI);
#endif

    // Populate the rectangle with layout instances.
    HRESULT CreateToFit (IN const CRectl * prclView, IN DWORD dwFlags);

    //+-----------------------------------------
    // UI related API
    //------------------------------------------

    // Scroll from keyboard
    KEYMETHOD(KeyScroll) (long lParam);
    KEYMETHOD(ProcessSpaceKey) (long lReserved);

    //
    // Enumeration method to loop thru children
    //-------------------------------------------------------------------------
	// 	+override : containers
	// 	-call super : no
	// 	-call parent : no
    //  -call children : no
    //-------------------------------------------------------------------------
    virtual CSite * BUGCALL GetNextSiteInTabOrder(
                CSite * pSite, CMessage * pMessage);


    // Check if repeatition is vertical.
    inline BOOL IsVertical ()
        {
            return _fDirection;
        }

    inline BOOL IsCurrent (CBaseFrame *p)
        {
            return p == (CBaseFrame *)_pCurrent;
        }

    inline CDetailFrame * getTemplate ()
        {
            return (CDetailFrame *)_pTemplate;
        }

    inline CDetailFrame *getRepeatedTemplate ()
        {
            return getTemplate();
        }

    virtual CDataFrame * getOwner()
        {
            return (CDataFrame *)_pParent;  // BUGBUG not always true !
        }
    virtual int GetIndex()
        {
            return _iIndex;
        }
    virtual void SetIndex(int index)
        {
            _iIndex = index;
        }

    inline CDetailFrame *getLayoutFrame (unsigned int i)
        {
            if (_arySites.Size())
            {
                Assert ( i < (unsigned int)_arySites.Size());
                return ((CDetailFrame *) FRAME(i));
            }
            else
                return NULL;
        }

    inline unsigned int getFramesCounter ()
        {
            return (unsigned int) ( _arySites.Size() );
        }

    inline unsigned int getTopFrameIdx ()
        {
            Assert(getFramesCounter());
            return 0;
        }

    inline unsigned int getBottomFrameIdx ()
        {
            Assert(getFramesCounter());
            return getFramesCounter () - 1;
        }

    inline BOOL IsAutoSize ()
        {
            return getOwner()->IsAutosize ();
        }

    inline BOOL IsNewRecordShow ()
        {
            return getOwner()->IsNewRecordShow();
        }

    inline BOOL IsListBoxStyle ()
        {
            return getOwner()->IsListBoxStyle();
        }

    // fetch(0) is -1 and fetch(1) is 1.
    inline En_FetchDirection getFetchDirection (BOOL direction)
        { return direction ? en_FetchNext : en_FetchPrev; }


    //+-----------------------------------------
    // OLE DB related API (Chapters, Cursors, etc.)
    //------------------------------------------

    HRESULT GetNextRows (OUT HROW *pRows,
                         IN  ULONG ulFetch,
                         OUT ULONG *pulFetched,
                         CBaseFrame *plfrStart=0);

protected:





    //+-----------------------------------------
    //  Keyboard handling overrides
    //------------------------------------------
public:
    virtual HRESULT BUGCALL HandleMessage(CMessage *pMessage, CSite *pChild);

    virtual HRESULT NextControl(NAVIGATE_DIRECTION Direction,
                                CSite * psCurrent,
                                CSite ** ppsNext,
                                CSite *pSiteCaller,
                                BOOL fStrictInDirection,    //  default = FALSE;
                                int iFirstFrame,            //  default -1
                                int iLastFrame);            //  default -1

    //  timer tick handler
    HRESULT BUGCALL OnTick(UINT id);

    void DeselectListboxLines(void);

protected:

    //+-----------------------------------------
    //  Real estate API (Move, Populate, etc.)
    //------------------------------------------

    // Set as the current row
    HRESULT SetCurrent (CBaseFrame *plfr, DWORD dwFlags = 0);

    // Populate the repeater rectangle with new instances.
    HRESULT Populate (IN    CBaseFrame *plfr,
                      INOUT CRectl& rclRealEstate,
                      IN    long lViewDelta,
                      IN    unsigned int uLimit,
                      IN    DWORD dwFlags,
                      IN    int insertAt = FRAME_NOTFOUND);

    // modify _rcl directly
    virtual void MoveSiteBy(long x, long y);

    // Populate the real estate rectangle (passed as a parameter) with one
    // frame (passed as a parameter). Returns TRUE if after populating the
    // rectangle, the real estate rectangle is not empty.
    HRESULT PopulateWithOneFrame (INOUT CRectl& rclRealEstate,
                                  IN    CRectl& rclView,
                                  INOUT CBaseFrame * pfr,
                                  IN    int iInsertAt,
                                  IN    DWORD dwFlags);

    // Subtract the rectangle from the real estate rectangle and return
    // TRUE if there is more real estate.
    BOOL EnoughRealEstate (INOUT CRectl& rectRealEstate,
                           IN const CRectl& rect);

    virtual HRESULT SetProposed (CSite * pSite, const CRectl * prcl);
    virtual HRESULT GetProposed (CSite * pSite, CRectl * prcl);
    virtual HRESULT CalcProposedPositions ();
    virtual HRESULT ProposedFriendFrames(void);
    virtual HRESULT ProposedDelta(CRectl *rclDelta);
    virtual HRESULT PrepareToScroll (IN int iDirection,
                                     IN UINT uCode,
                                     IN BOOL fKeyboardScrolling,
                                     IN long lPosition,
                                     OUT long *pDelta,
                                     CBaseFrame *plfr = NULL);

    HRESULT FixupPosition (CBaseFrame *plfr, unsigned long lRecordNumber = 0);
    virtual void DrawFeedbackRect (HDC hDC);
    virtual HRESULT UpdatePosRects1 ();

    virtual void GetClientRectl(RECTL *prcl);

    CDetailFrame * HitTestDetailFrames(const CPointl& pt);

    void KillScrollTimer(void);
    void DragSelect(CDataFrame::CTBag * pTBag, CDetailFrame * pdet, CDataFrame * pdfrOwner, DWORD dwFlags = 0);

    //+--------------------------------
    //  Drag & Drop, just for eventfiring...
    //---------------------------------

    virtual HRESULT DragEnter(IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
    virtual HRESULT DragOver(DWORD grfKeyState, POINTL pt, DWORD * pdwEffect);
    virtual HRESULT Drop(IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
    virtual HRESULT DragLeave();


    HRESULT AddToSites(CSite * pSite, DWORD dwOperations, int zOrder);
    void RemoveNotPopulatedSites();

#if OLD_CODE
    HRESULT ScrollBy (
                CBaseFrame *plfr,
                long dxl,
                long dyl,
                fmScrollAction xAction,
                fmScrollAction yAction);
#endif
    virtual HRESULT ScrollBy (
                long dxl,
                long dyl,
                fmScrollAction xAction,
                fmScrollAction yAction);

    //+-----------------------------------------------------------------------
    //  DataDoc selection tree builder methods
    //------------------------------------------------------------------------

    virtual HRESULT SelectSite(CSite * pSite, DWORD dwFlags);

    //+-----------------------------------------
    //  Layout frames caching API.
    //------------------------------------------

    // Cache layout frame.
    void CacheFrame (CBaseFrame *plfr, CBaseFrame *plfrBase=0);
    void RemoveFromCache (CBaseFrame *plfr);

    CBaseFrame* CacheLookUpRow(HROW hrow);


    // Remove all the layout frames from the cach list.
    virtual void Detach ();

    // Get next row, either from Cursor or from the array of cached rows.
    HRESULT GetNextLayoutFrame (IN CBaseFrame *plfrOrigin,
                                OUT CBaseFrame **pplfr,
                                ULONG ulPrefetch=1);

        // Creates new layout frame and cach it.
    HRESULT CreateLayout (HROW hrow, OUT CBaseFrame **plfr, CBaseFrame *plfrBase=0);

    HRESULT CreateNewCurrentLayout (IN  CDataLayerBookmark  &dlBookmark,
                                    IN  long lRecordNumber,
                                    OUT CDetailFrame **pplfr);

    HRESULT CreateLayoutRelativeToVisibleRange (IN  long lPosition,
                                                IN  HROW hrow,
                                                OUT CBaseFrame **pplfr,
                                                OUT int *piInsertAt,
                                                OUT BOOL *pfCreateToFit);
    void MoveToRecycle (CBaseFrame *plfr);

    void FlushCache (void);

    void FlushNotVisibleCache (void);

    void PutCacheFrameInRecycle(void);

#if DBG==1
    void AssertIfCacheInvalid ();
#endif

#if DBG==1 && !defined(PRODUCT_97)
    void CheckFrameSizes(CBaseFrame *p);
#endif

    //+-----------------------------------------
    //  Keyboard handdling.
    //------------------------------------------

    //      Laszlog 03/03/95 :      Keyboard handling
    //HRESULT __stdcall NextRow(void);
    //HRESULT __stdcall PreviousRow(void);
    //HRESULT __stdcall NextRow(void);
    //HRESULT __stdcall PreviousRow(void);

    //  Keyboad lookup helper
    virtual KEY_MAP * GetKeyMap(void);
    virtual int GetKeyMapSize(void);

    virtual CBase::CLASSDESC *GetClassDesc() const { return &s_classdesc;}

private:
    //+------------------------------------------------
    // Snaking (alexa: Hungarian: Kiki tulajdonsagok:-)
    //-------------------------------------------------

    /*

       ( )
        ~ \
           \      /\
            \    /  \     /\
             \__/    \___/  \__/\_/\_
                                     \
    */

    void CalcRowRectangle (CRectl& rclRow, CRectl *prcl);

    // calculate the repeater's rectangle (proposed).
    void CalcProposedRectangle ();

    // calculate the repeater's virtual space rectangle (_rcl)
    virtual void CalcRectangle (BOOL fProposed=FALSE);

    // deleting rows from the cursor
    virtual void DeletingRows (ULONG, const HROW *);

    BOOL IsTopRow (unsigned int index);

    BOOL IsTopRowFirstInSet ();

    BOOL IsBottomRow (unsigned int index);

    BOOL IsBottomRowLastInSet ();

    BOOL IsNoNextRow (CBaseFrame *plfr);

    void SetFirstLastIndexes (unsigned int n);

    BOOL AreInColumn (CBaseFrame *pFrame1, CBaseFrame *pFrame2);

    inline BOOL IsPopulateDown ()
        {
            return _fPopulateDirection == POPULATE_DOWN;
        }

    inline BOOL IsPopulateUp ()
        {
            return _fPopulateDirection == POPULATE_UP;
        }

    inline unsigned int getItemsDown ()
        { return getOwner()->TBag()->_uItems[DOWN]; }
    inline unsigned int getItemsAcross ()
        { return getOwner()->TBag()->_uItems[ACROSS]; }
    inline unsigned int getItemsNonRepDir ()
        {
            CDataFrame::CTBag * pTBag = getOwner()->TBag();
            return pTBag->_uItems[!pTBag->_fDirection];
        }
    inline unsigned int getItemsRepDir ()
        {
            CDataFrame::CTBag * pTBag = getOwner()->TBag();
            return pTBag->_uItems[pTBag->_fDirection];
        }
    inline unsigned int getPadding (BOOL fDirection)
        { return getOwner()->TBag()->_uPadding[fDirection]; }
    inline BOOL getDirection ()
        { return getOwner()->TBag()->_fDirection; }

    HRESULT ReleaseRow(HROW *phrow);


protected:

    //+-----------------------------------------
    // repeater properties
    //------------------------------------------

    CDetailFrame *_pCurrent;

    unsigned int _fDirection: 1;        // Set to 1 if Vertical, 0 if Horizontal.
                                        // the value of _fDirection we always
                                        // get from the owner's template (we
                                        // just cache the flag) for population.
    unsigned int _fPopulateDirection:1;
    unsigned int _fCurrentSelected : 1; // set if only the current row is selected
    unsigned int _fTimerAlive : 1;      //  if we have a scroll-timer around
    unsigned int _fMouseCaptured : 1;
    unsigned int _fComboMouseTracking:1;//  instead of capturing the mouse in the combo box
                                        //  Reason: we must not steal the capture from
                                        //  the wrapper control. In return the wrapper
                                        //  guarantees to feed us the messages

    int          _iIndex;               //  index in parent's arySites

    //+-----------------------------------------
    // computable helper member data
    //------------------------------------------

    // The following propertie is moved to BaseFrame
    // CBaseFrame *_apNode[2];      // Pointers to the begining/end of the
    //                              // double-linked list of layout frames.
    unsigned int _uCached;          // Number of cached layout frames.
    unsigned int _uRepeatInPage;    // In case of vertical repeatition it's
                                    // number of rows visible on the screen,
                                    // In case of horizontal repeatition it's
                                    // number of visible columns.


    //+-----------------------------------------
    //  Keyboard handling data (Laszlog 03/03/95)
    //------------------------------------------
    static KEY_MAP  s_aKeyActions[];
    static int      s_cKeyActions;

    static int s_aiRowOffset[4];    //  static helper array for scrolling calculations

    static CLASSDESC   s_classdesc;
    CRectl          _rclProposeRow; // rcl propose for layouts frames (rows)
};
//----------------------------------------------------------------------------



#endif    // _REPEATER_HXX_

//
//  end of file
//
//----------------------------------------------------------------------------
