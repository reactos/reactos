//+---------------------------------------------------------------------------
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       element.hxx
//
//  Contents:   CElement class
//
//----------------------------------------------------------------------------

#ifndef I_ELEMENT_HXX_
#define I_ELEMENT_HXX_
#pragma INCMSG("--- Beg 'element.hxx'")

#ifdef _MAC // to get notifytype enum
#ifndef I_NOTIFY_HXX_
#include "notify.hxx"
#endif
#endif

#ifndef X_FCACHE_HXX_
#define X_FCACHE_HXX_
#include "fcache.hxx"
#endif

#ifndef X_HTMTAGS_HXX_
#define X_HTMTAGS_HXX_
#include "htmtags.hxx"
#endif

#ifndef X_RECT_HXX_
#define X_RECT_HXX_
#include "rect.hxx"
#endif

#ifndef X_OLECTL_H_
#define X_OLECTL_H_
#pragma INCMSG("--- Beg <olectl.h>")
#include <olectl.h>
#pragma INCMSG("--- End <olectl.h>")
#endif

#define _hxx_
#include "element.hdl"

#define _hxx_
#include "unknown.hdl"

#define DEFAULT_ATTR_SIZE 128
#define MAX_PERSIST_ID_LENGTH 256

class CFormDrawInfo;
class CPostData;
class CFormElement;
class CLayout;
class CFlowLayout;
class CTableLayout;
class CFrameSetlayout;
class CBorderInfo;
class CStreamWriteBuff;
class CImgCtx;
class CSelection;
class CDoc;
class CPropertyBag;
class CCSSFilterSite;
class CFilterArray;
class CSite;
class CTxtSite;
class CStyle;
class CTreePos;
class CTreeDataPos;
class CTreeNode;
class CMarkupPointer;
class CCurrentStyle;
class CLabelElement;
class CShape;
class CRootElement;
class CNotification;
class CPeerHolder;
class CPeerMgr;
class CMarkup;
class CDOMChildrenCollection;
class CDOMTextNode;
class CDispNode;
class CRecordInstance;
class CAccElement;
class CFilter;
class CDataMemberMgr;
class CGlyphRenderInfoType;
class CRequest;

enum PEERTASK;

interface IHTMLPersistData;
interface INamedPropertyBag;

interface IHTMLPersistData;
interface INamedPropertyBag;

struct DBMEMBERS;
struct DBSPEC;
enum FOCUS_DIRECTION;

interface IMarkupPointer;

MtExtern(CElement)
MtExtern(CElementCLock)
MtExtern(CElementFactory)
MtExtern(CFormatInfo)
MtExtern(CUnknownElement)
MtExtern(CSiteDrawList)
MtExtern(CSiteDrawList_pv)
MtExtern(CMessage)
MtExtern(CTreePos)
MtExtern(CTreeDataPos)
MtExtern(CTreeNode)
MtExtern(CTreeNodeCLock)

//+------------------------------------------------------------------------
//
//  Macro: DECLARE_LAYOUT_FNS, IMPLEMENT_LAYOUT_FNS
//
//  Basic layout-related functions that need to be implemented by all
//  CElement-derived classes which have their own layout classes.
//
//-------------------------------------------------------------------------
#define DECLARE_LAYOUT_FNS(clsLayout)      \
    virtual HRESULT CreateLayout();        \
    clsLayout* Layout();


// NOTE: (jbeda) GetLayoutPtr returns null if there is no layout
// there is no need to have two if checks in Layout.  However,
// in debug mode we want to have the DYNCAST so there are two
// different versions of IMPLEMENT_LAYOUT_FNS
#if DBG==1
#define IMPLEMENT_LAYOUT_FNS(clsElem, clsLayout)    \
    HRESULT clsElem::CreateLayout()                 \
    {                                               \
        Assert(!HasLayoutPtr());                    \
        CLayout * pLayout = new clsLayout(this);    \
        if (!pLayout) return(E_OUTOFMEMORY);        \
        SetLayoutPtr(pLayout);                      \
        return(S_OK);                               \
    }                                               \
    clsLayout* clsElem::Layout()                    \
    {                                               \
        return HasLayout() ? DYNCAST(clsLayout, GetLayoutPtr()) : NULL;     \
    }
#else
#define IMPLEMENT_LAYOUT_FNS(clsElem, clsLayout)    \
    HRESULT clsElem::CreateLayout()                 \
    {                                               \
        Assert(!HasLayoutPtr());                    \
        CLayout * pLayout = new clsLayout(this);    \
        if (!pLayout) return(E_OUTOFMEMORY);        \
        SetLayoutPtr(pLayout);                      \
        return(S_OK);                               \
    }                                               \
    clsLayout* clsElem::Layout()                    \
    {                                               \
        return (clsLayout*)GetLayoutPtr();          \
    }
#endif

struct COnCommandExecParams {
    inline COnCommandExecParams()
    {
        memset(this, 0, sizeof(COnCommandExecParams));
    }

    GUID *       pguidCmdGroup;
    DWORD        nCmdID;
    DWORD        nCmdexecopt;
    VARIANTARG * pvarargIn;
    VARIANTARG * pvarargOut;
};

// Used by NTYPE_QUERYFOCUSSABLE and NTYPE_QUERYTABBABLE
struct CQueryFocus
{
    long    _lSubDivision;
    BOOL    _fRetVal;
};

// Used by NTYPE_SETTINGFOCUS and NTYPE_SETFOCUS
struct CSetFocus
{
    CMessage *  _pMessage;
    long        _lSubDivision;
    HRESULT     _hr;
};

enum FOCUSSABILITY
{
    // Used by CElement::GetDefaultFocussability()
    // We may need to add more states, but we will think hard before adding them.
    //
    // All four modes are allowed for browse mode.
    // Only the first and the last (FOCUSSABILTY_NEVER and FOCUSSABILTY_TABBABLE)
    // are allowed for design mode.

    FOCUSSABILITY_NEVER         = 0,    // never focussable (and hence never tabbable)
    FOCUSSABILITY_MAYBE         = 1,    // possible to become focussable and tabbable
    FOCUSSABILITY_FOCUSSABLE    = 2,    // focussable by default, possible to become tabbable
    FOCUSSABILITY_TABBABLE      = 3,    // tabbable (and hence focussable) by default
};

//+---------------------------------------------------------------------------
//
//  Flag values for CELEMENT::OnPropertyChange
//
//----------------------------------------------------------------------------

enum ELEMCHNG_FLAG
{
    ELEMCHNG_INLINESTYLE_PROPERTY   = FORMCHNG_LAST << 1,
    ELEMCHNG_CLEARCACHES            = FORMCHNG_LAST << 2,
    ELEMCHNG_SITEREDRAW             = FORMCHNG_LAST << 3,
    ELEMCHNG_REMEASURECONTENTS      = FORMCHNG_LAST << 4,
    ELEMCHNG_CLEARFF                = FORMCHNG_LAST << 5,
    ELEMCHNG_UPDATECOLLECTION       = FORMCHNG_LAST << 6,
    ELEMCHNG_SITEPOSITION           = FORMCHNG_LAST << 7,
    ELEMCHNG_RESIZENONSITESONLY     = FORMCHNG_LAST << 8,
    ELEMCHNG_SIZECHANGED            = FORMCHNG_LAST << 9,
    ELEMCHNG_REMEASUREINPARENT      = FORMCHNG_LAST << 10,
    ELEMCHNG_ACCESSIBILITY          = FORMCHNG_LAST << 11,
    ELEMCHNG_REMEASUREALLCONTENTS   = FORMCHNG_LAST << 12,
    ELEMCHNG_LAST                   = FORMCHNG_LAST << 12
};

#define FEEDBACKRECTSIZE        1
#define GRABSIZE                7
#define HITTESTSIZE             5

//+------------------------------------------------------------------------
//
//  Literals:   ENTERTREE types
//
//-------------------------------------------------------------------------

enum ENTERTREE_TYPE
{
    ENTERTREE_MOVE  = 0x1 << 0,         // [IN] This is the second half of a move opertaion
    ENTERTREE_PARSE = 0x1 << 1          // [IN] This is happening during parse
};

//+------------------------------------------------------------------------
//
//  Literals:   EXITTREE types
//
//-------------------------------------------------------------------------

enum EXITTREE_TYPE
{
    EXITTREE_MOVE               = 0x1 << 0,     // [IN] This is the first half of a move operation
    EXITTREE_DESTROY            = 0x1 << 1,     // [IN] The entire markup is being shut down -- 
                                                //      element is not allowed to talk to other elements
    EXITTREE_PASSIVATEPENDING   = 0x1 << 2,     // [IN] The tree has the last ref on this element
    EXITTREE_DELAYRELEASENEEDED = 0x1 << 3      // [OUT]Element talks to outside world on final release
                                                //      so release should be delayed
};

//+------------------------------------------------------------------------
//
//  Function:   TagPreservationType
//
//  Synopsis:   Describes how white space is handled in a tag
//
//-------------------------------------------------------------------------

enum WSPT
{
    WSPT_PRESERVE,
    WSPT_COLLAPSE,
    WSPT_NEITHER
};

WSPT TagPreservationType ( ELEMENT_TAG );

FOCUSSABILITY GetDefaultFocussability( ELEMENT_TAG );

// Border sides.  These are used to make loops of common set/get operations on border properties.
enum {
    BORDER_TOP = 0,
    BORDER_RIGHT = 1,
    BORDER_BOTTOM = 2,
    BORDER_LEFT = 3
};


//
// Persistance enums.  Used to distinguish in TryPeerPersist.
//

enum PERSIST_TYPE
{
    FAVORITES_SAVE,
    FAVORITES_LOAD,
    XTAG_HISTORY_SAVE,
    XTAG_HISTORY_LOAD
};

//
// CInit2Context flags
//
enum
{
    INIT2FLAG_EXECUTE = 0x01
};

//
// A success code which implies that the action was successful, but
// somehow incomplete.  This is used in InitAttrBag if some attribute
// was not fully loaded.  This is also the first hresult value after
// the reserved ole values.
//

#define S_INCOMPLETE    MAKE_HRESULT(SEVERITY_SUCCESS, FACILITY_ITF, 0x201)

// Defining USE_SPLAYTREE_THREADS makes NextTreePos() and PreviousTreePos()
// very fast, at the expense of two more pointers per node.  In debug builds,
// we always maintain the threads, as a check that the maintenance code works.

//#define USE_SPLAYTREE_THREADS
#if DBG==1 || defined(USE_SPLAYTREE_THREADS)
#  define MAINTAIN_SPLAYTREE_THREADS
#endif

#define BOOLFLAG(f, dwFlag)  ((DWORD)(-(LONG)!!(f)) & (dwFlag))

// helper
void
TransformSlaveToMaster(CTreeNode ** ppNode);

//+---------------------------------------------------------------------------
//
//  Class:      CTreePos (Position In Tree)
//
//----------------------------------------------------------------------------

class CTreePos
{
    friend class CMarkup;
    friend class CTreePosGap;
    friend class CTreeNode;
    friend class CHtmRootParseCtx;
    friend class CSpliceTreeEngine;
    friend class CMarkupUndoUnit;
    
public:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CTreePos));

    WHEN_DBG( BOOL IsSplayValid(CTreePos *ptpTotal) const; )

    // TreePos come in various flavors:
    //  Uninit  this node is uninitialized
    //  Node    marks begin or end of a CTreeNode's scope
    //  Text    holds a bunch of text (formerly known as CElementRun)
    //  Pointer implements an IMarkupPointer
    // Be sure the bit field _eType (below) is big enough for all the flavors
    enum EType { Uninit=0x0, NodeBeg=0x1, NodeEnd=0x2, Text=0x4, Pointer=0x8 };

    // cast to CTreeDataPos
    CTreeDataPos * DataThis();
    const CTreeDataPos * DataThis() const;

    // accessors
    EType   Type() const { return (EType)(GetFlags() & TPF_ETYPE_MASK); }
    void    SetType(EType etype)  { Assert(etype <= Pointer); WHEN_DBG(_eTypeDbg = etype); SetFlags((GetFlags() & ~TPF_ETYPE_MASK) | (etype)); }
    BOOL    IsUninit() const { return !TestFlag(NodeBeg|NodeEnd|Text|Pointer); }
    BOOL    IsNode() const { return TestFlag(NodeBeg|NodeEnd); }
    BOOL    IsText() const { return TestFlag(Text); }
    BOOL    IsPointer() const { return TestFlag(Pointer); }
    BOOL    IsDataPos() const { return TestFlag(TPF_DATA_POS); }
    BOOL    IsData2Pos() const { Assert( !IsNode() ); return TestFlag( TPF_DATA2_POS ); }
    BOOL    IsBeginNode() const { return TestFlag(NodeBeg); }
    BOOL    IsEndNode() const { return TestFlag(NodeEnd); }
    BOOL    IsEdgeScope() const { Assert( IsNode() ); return TestFlag(TPF_EDGE); }
    BOOL    IsBeginElementScope() const { return IsBeginNode() && IsEdgeScope(); }
    BOOL    IsEndElementScope() const { return IsEndNode() && IsEdgeScope(); }
    BOOL    IsBeginElementScope(CElement *pElem);
    BOOL    IsEndElementScope(CElement *pElem);
    
    CMarkup * GetMarkup();
    BOOL      IsInMarkup ( CMarkup * pMarkup ) { return GetMarkup() == pMarkup; }

    //
    // The following are logical comparison operations (two pos are equal
    // when separated by only pointers or empty text positions).
    //
    
    int  InternalCompare ( CTreePos * ptpThat );

    CTreeNode * Branch() const;         // Only valid to call on NodePoses
    CTreeNode * GetBranch() const;      // Can be called on any pos, may be expensive

    CMarkupPointer * MarkupPointer() const;
    void             SetMarkupPointer ( CMarkupPointer * );

    // GetInterNode finds the node with direct influence
    // over the position directly after this CTreePos.
    CTreeNode * GetInterNode() const;

    long    Cch() const;
    long    Sid() const;

    BOOL    HasTextID() const { return IsText() && TestFlag(TPF_DATA2_POS); }
    long    TextID() const;

    long    GetCElements() const { return IsBeginElementScope() ? 1 : 0; }
    long    SourceIndex();

    long    GetCch() const { return IsNode() ? 1 : IsText() ? Cch() : 0; }

    long    GetCp( WHEN_DBG(BOOL fNoTrace=FALSE) );
    WHEN_DBG( long GetCp_NoTrace() { return GetCp(TRUE); } )

    int     Gravity ( ) const;
    void    SetGravity ( BOOL fRight );
    int     Cling ( ) const;
    void    SetCling ( BOOL fStick );


    // modifiers
    HRESULT ReplaceNode(CTreeNode *);
    void    SetScopeFlags(BOOL fEdge);
    void    ChangeCch(long cchDelta);

    // navigation
#if defined(USE_SPLAYTREE_THREADS)
    CTreePos *  NextTreePos() { return _ptpThreadRight; }
    CTreePos *  PreviousTreePos() { return _ptpThreadLeft; }
#else
    CTreePos *  NextTreePos();
    CTreePos *  PreviousTreePos();
#endif


    CTreePos *  NextValidInterRPos()
        { return NextTreePos()->FindLegalPosition(FALSE); }
    CTreePos *  NextValidInterLPos()
        { CTreePos *ptp = FindLegalPosition(FALSE);
          return ptp ? ptp->NextTreePos() : NULL; }

    CTreePos *  PreviousValidInterRPos()
        { CTreePos *ptp = FindLegalPosition(TRUE);
          return ptp ? ptp->PreviousTreePos() : NULL; }
    CTreePos *  PreviousValidInterLPos()
        { return PreviousTreePos()->FindLegalPosition(TRUE); }

    BOOL        IsValidInterRPos()
        { return IsLegalPosition(FALSE); }
    BOOL        IsValidInterLPos()
        { return IsLegalPosition(TRUE); }

    CTreePos * NextValidNonPtrInterLPos();
    CTreePos * PreviousValidNonPtrInterLPos();

    CTreePos *  NextNonPtrTreePos();
    CTreePos *  PreviousNonPtrTreePos();

    // tree pointer support
    // BUGBUG deprecated.  use CTreePosGap::IsValid()
    static BOOL IsLegalPosition(CTreePos *ptpLeft, CTreePos *ptpRight);
    BOOL        IsLegalPosition(BOOL fBefore)
        { return fBefore ? IsLegalPosition(PreviousTreePos(), this)
                         : IsLegalPosition(this, NextTreePos()); }
    CTreePos *  FindLegalPosition(BOOL fBefore);

    // misc
    BOOL ShowTreePos(CGlyphRenderInfoType *pRenderInfo=NULL);
    
protected:
    void    InitSublist();
    CTreePos *  Parent() const;
    CTreePos *  LeftChild() const;
    CTreePos *  RightChild() const;
    CTreePos *  LeftmostDescendant() const;
    CTreePos *  RightmostDescendant() const;
    void    GetChildren(CTreePos **ppLeft, CTreePos **ppRight) const;
    HRESULT Remove();
    void    Splay();
    void    RotateUp(CTreePos *p, CTreePos *g);
    void    ReplaceChild(CTreePos *pOld, CTreePos *pNew);
    void    RemoveChild(CTreePos *pOld);
    void    ReplaceOrRemoveChild(CTreePos *pOld, CTreePos *pNew);
    WHEN_DBG( long Depth() const; )
    WHEN_DBG( CMarkup * Owner() const { return _pOwner; } )

    // constructors (for use only by CMarkup and CTreeNode)
    CTreePos() {}

private:
    // distributed order-statistic fields
    DWORD       _cElemLeftAndFlags;  // number of elements that begin in my left subtree
    DWORD       _cchLeft;       // number of characters in my left subtree
    // structure fields (to maintain the splay tree)
    CTreePos *  _pFirstChild;   // pointer to my leftmost child
    CTreePos *  _pNext;         // pointer to right sibling or parent

#if defined(MAINTAIN_SPLAYTREE_THREADS)
    CTreePos *  _ptpThreadLeft;     // previous tree pos
    CTreePos *  _ptpThreadRight;    // next tree pos
    CTreePos *  LeftThread() const { return _ptpThreadLeft; }
    CTreePos *  RightThread() const { return _ptpThreadRight; }
    void        SetLeftThread(CTreePos *ptp) { _ptpThreadLeft = ptp; }
    void        SetRightThread(CTreePos *ptp) { _ptpThreadRight = ptp; }
#define CTREEPOS_THREADS_SIZE   (2 * sizeof(CTreePos *))
#else
#define CTREEPOS_THREADS_SIZE   (0)
#endif

    enum {
        TPF_ETYPE_MASK      = 0x0F,
        TPF_LEFT_CHILD      = 0x10,
        TPF_LAST_CHILD      = 0x20,
        TPF_EDGE            = 0x40,
        TPF_DATA2_POS       = 0x40,
        TPF_DATA_POS        = 0x80,
        TPF_FLAGS_MASK      = 0xFF,
        TPF_FLAGS_SHIFT     = 8
    };

    DWORD   GetFlags() const                { return(_cElemLeftAndFlags); }
    void    SetFlags(DWORD dwFlags)         { _cElemLeftAndFlags = dwFlags; }
    BOOL    TestFlag(DWORD dwFlag) const    { return(!!(GetFlags() & dwFlag)); }
    void    SetFlag(DWORD dwFlag)           { SetFlags(GetFlags() | dwFlag); }
    void    ClearFlag(DWORD dwFlag)         { SetFlags(GetFlags() & ~(dwFlag)); }

    long    GetElemLeft() const             { return((long)(_cElemLeftAndFlags >> TPF_FLAGS_SHIFT)); }
    void    SetElemLeft(DWORD cElem)        { _cElemLeftAndFlags = (_cElemLeftAndFlags & TPF_FLAGS_MASK) | (DWORD)(cElem << TPF_FLAGS_SHIFT); }
    void    AdjElemLeft(long cDelta)        { _cElemLeftAndFlags += cDelta << TPF_FLAGS_SHIFT; }
    BOOL    IsLeftChild() const             { return(TestFlag(TPF_LEFT_CHILD)); }
    BOOL    IsLastChild() const             { return(TestFlag(TPF_LAST_CHILD)); }
    void    MarkLeft()                      { SetFlag(TPF_LEFT_CHILD); }
    void    MarkRight()                     { ClearFlag(TPF_LEFT_CHILD); }
    void    MarkLeft(BOOL fLeft)            { SetFlags((GetFlags() & ~TPF_LEFT_CHILD) | BOOLFLAG(fLeft, TPF_LEFT_CHILD)); }
    void    MarkFirst()                     { ClearFlag(TPF_LAST_CHILD); }
    void    MarkLast()                      { SetFlag(TPF_LAST_CHILD); }
    void    MarkLast(BOOL fLast)            { SetFlags((GetFlags() & ~TPF_LAST_CHILD) | BOOLFLAG(fLast, TPF_LAST_CHILD)); }

    void        SetFirstChild(CTreePos *ptp)    { _pFirstChild = ptp; }
    void        SetNext(CTreePos *ptp)          { _pNext = ptp; }
    CTreePos *  FirstChild() const              { return(_pFirstChild); }
    CTreePos *  Next() const                    { return(_pNext); }

    // support for CTreePosGap
    CTreeNode * SearchBranchForElement(CElement *pElement, BOOL fLeft);

    // count encapsulation
    enum    ECountFlags { TP_LEFT=0x1, TP_DIRECT=0x2, TP_BOTH=0x3 };

    struct SCounts
    {
        DWORD   _cch;
        DWORD   _cElem;
        void    Clear();
        void    Increase( const CTreePos *ptp );    // TP_DIRECT is implied
        BOOL    IsNonzero();
    };

    void    ClearCounts();
    void    IncreaseCounts(const CTreePos *ptp, unsigned fFlags);
    void    IncreaseCounts(const SCounts * pCounts );
    void    DecreaseCounts(const CTreePos *ptp, unsigned fFlags);
    BOOL    HasNonzeroCounts(unsigned fFlags);

    BOOL LogicallyEqual ( CTreePos * ptpRight );

#if DBG==1
    CMarkup *   _pOwner;
    EType       _eTypeDbg;
    long        _cGapsAttached;
    CTreePos(BOOL) { ClearCounts(); }
    BOOL    EqualCounts(const CTreePos* ptp) const;
    void    AttachGap() { ++ _cGapsAttached; }
    void    DetachGap() { Assert(_cGapsAttached>0); -- _cGapsAttached; }
#define CTREEPOS_DBG_SIZE   (sizeof(CMarkup *) + sizeof(long) + sizeof(long))
#else
#define CTREEPOS_DBG_SIZE   (0)
#endif

public:
#if DBG == 1 || defined(DUMPTREE)
    int     _nSerialNumber;
    int     _nPad;
    void    SetSN();

    int     SN() { return _nSerialNumber; }

    static int s_NextSerialNumber;
#define CTREEPOS_SN_SIZE    (sizeof(int) + sizeof(int))
#else
#define CTREEPOS_SN_SIZE    (0)
#endif

    NO_COPY(CTreePos);

};

//
// If the compiler barfs on the next statement, it means that the size of the CTreePos structure
// has grown beyond allowable limits.  You cannot increase the size of this structure without
// approval from the Trident development manager.
//

COMPILE_TIME_ASSERT(CTreePos, (2*sizeof(DWORD)) + (2*sizeof(CTreePos *)) + CTREEPOS_DBG_SIZE + CTREEPOS_SN_SIZE + CTREEPOS_THREADS_SIZE);

//+---------------------------------------------------------------------------
//
// CTreeDataPos
//
//----------------------------------------------------------------------------

struct DATAPOSTEXT
{
    unsigned long   _cch:26;       // [Text] number of characters I own directly
    unsigned long   _sid:6;        // [Text] the script id for this run
    // This member will only be valid if the TPF_DATA2_POS flag is turned
    // on.  Otherwise, assume that _lTextID is 0.
    long            _lTextID;      // [Text] Text ID for DOM text nodes
};

struct DATAPOSPOINTER
{
    DWORD_PTR       _dwPointerAndGravityAndCling;
                                    // [Pointer] my CMarkupPointer and Gravity
};

class CTreeDataPos : public CTreePos
{
    friend class CTreePos;
    friend class CMarkup;
    friend class CMarkupUndoUnit;
    friend class CHtmRootParseCtx;

public:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CTreeDataPos));

protected:

    union {
        DATAPOSTEXT     t;
        DATAPOSPOINTER  p;
    };

private:
    CTreeDataPos() {}
    NO_COPY(CTreeDataPos);
};

#define TREEDATA2SIZE (sizeof(CTreePos) + 8)

#ifdef _WIN64
#define TREEDATA1SIZE (sizeof(CTreePos) + 8)
#else
#define TREEDATA1SIZE (sizeof(CTreePos) + 4)
#endif

COMPILE_TIME_ASSERT(CTreeDataPos, TREEDATA2SIZE);

//+---------------------------------------------------------------------------
//
// CTreeNode
//
//----------------------------------------------------------------------------

class NOVTABLE CTreeNode : public CVoid
{
    friend class CTreePos;

    DECLARE_CLASS_TYPES(CTreeNode, CVoid)

public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CTreeNode))

    CTreeNode(CTreeNode *pParent, CElement *pElement = NULL);
    WHEN_DBG( ~CTreeNode() );

    // Use this to get an interface to the element/node

    HRESULT GetElementInterface ( REFIID riid, void * * ppUnk );

    // NOTE: these functions may look like IUnknown functions
    //       but don't make that mistake.  They are here to
    //       manage creation of the tearoff to handle all
    //       external references.

    // These functions should not be called!
    NV_DECLARE_TEAROFF_METHOD(
        GetInterface, getinterface, (
            REFIID riid,
            LPVOID * ppv ) ) ;
    NV_DECLARE_TEAROFF_METHOD_( ULONG,
        PutRef, putref, () ) ;
    NV_DECLARE_TEAROFF_METHOD_( ULONG,
        RemoveRef, removeref, () ) ;

    // These functions are to be used to keep a node/element
    // combination alive while it may leave the tree.
    // BEWARE: this may cause the creation of a tearoff.
    ULONG   NodeAddRef();
    ULONG   NodeRelease();

    // These functions manage the _fInMarkup bit
    void    PrivateEnterTree();
    void    PrivateExitTree();
    void    PrivateMakeDead();
    void    PrivateMarkupRelease();

    void        SetElement(CElement *pElement);
    void        SetParent(CTreeNode *pNodeParent);

    // Element access and structure methods
    CElement*   Element() { return _pElement; }
    CElement*   SafeElement() { return this ? _pElement : NULL; }

    CTreePos*   GetBeginPos() { return &_tpBegin; }
    CTreePos*   GetEndPos()   { return &_tpEnd;   }

    // Context chain access
    BOOL        IsFirstBranch() { return _tpBegin.IsEdgeScope(); }
    BOOL        IsLastBranch() { return _tpEnd.IsEdgeScope(); }
    CTreeNode * NextBranch();
    CTreeNode * PreviousBranch();

    CDoc *      Doc();

    BOOL            IsInMarkup()    { return _fInMarkup; }
    BOOL            IsDead()        { return ! _fInMarkup; }
    CRootElement *  IsRoot();
    CMarkup *       GetMarkup();
    CRootElement *  MarkupRoot();

    // Does the element that this node points to have currency?
    BOOL HasCurrency();

    BOOL        IsContainer();
    CTreeNode * GetContainerBranch();
    CElement  * GetContainer()
        { return GetContainerBranch()->SafeElement(); }

    BOOL        SupportsHtml();

    CTreeNode * Parent()
        { return _pNodeParent; }

    CTreeNode * Ancestor (ELEMENT_TAG etag);
    CTreeNode * Ancestor (ELEMENT_TAG *arytag);

    CElement *  ZParent()
        { return ZParentBranch()->SafeElement(); }
    CTreeNode * ZParentBranch();

    CElement *     RenderParent()
        { return RenderParentBranch()->SafeElement(); }
    CTreeNode * RenderParentBranch();

    CElement *  ClipParent()
        { return ClipParentBranch()->SafeElement(); }
    CTreeNode * ClipParentBranch();

    CElement *  ScrollingParent()
        { return ScrollingParentBranch()->SafeElement(); }
    CTreeNode * ScrollingParentBranch();

    inline ELEMENT_TAG Tag()   { return (ELEMENT_TAG)_etag; }

    ELEMENT_TAG TagType() {
        switch (_etag)
        {
        case ETAG_GENERIC_LITERAL:
        case ETAG_GENERIC_BUILTIN:
            return ETAG_GENERIC;
        default:
            return (ELEMENT_TAG)_etag;
        }
    }

    CTreeNode * GetFirstCommonAncestor(CTreeNode * pNode, CElement* pEltStop);
    CTreeNode * GetFirstCommonBlockOrLayoutAncestor(CTreeNode * pNodeTwo, CElement* pEltStop);
    CTreeNode * GetFirstCommonAncestorNode(CTreeNode * pNodeTwo, CElement* pEltStop);

    CTreeNode * SearchBranchForPureBlockElement(CFlowLayout *);
    CTreeNode * SearchBranchToFlowLayoutForTag(ELEMENT_TAG etag);
    CTreeNode * SearchBranchToRootForTag(ELEMENT_TAG etag);
    CTreeNode * SearchBranchToRootForScope(CElement * pElementFindMe);
    BOOL        SearchBranchToRootForNode(CTreeNode * pNodeFindMe);

    CTreeNode * GetCurrentRelativeNode(CElement * pElementFL);


    // CAUTION CAUTION CAUTION CAUTION CAUTION CAUTION CAUTION CAUTION CAUTION
    // CAUTION CAUTION CAUTION CAUTION CAUTION CAUTION CAUTION CAUTION CAUTION
    // (please read the comments below)
    //
    // The layout attached to the current element may not be accurate, when a
    // property changes, current element can gain/lose layoutness. When an
    // element gains/loses layoutness, its layout is created/destroyed lazily.
    //
    // So, for the following functions "cur" means return the layout currently
    // associated with the layout which may not be accurate. "Updated" means
    // compute the state and return the accurate information.
    //
    // Note: Calling "Updated" function may cause the formats to be computed.
    //
    // If there is any confusion please talk to (srinib/lylec/brendand)
    //
    inline  CLayout   * GetCurLayout();
    inline  BOOL        HasLayout();

    CLayout   * GetCurNearestLayout();
    CTreeNode * GetCurNearestLayoutNode();
    CElement  * GetCurNearestLayoutElement();

    CLayout   * GetCurParentLayout();
    CTreeNode * GetCurParentLayoutNode();
    CElement  * GetCurParentLayoutElement();

    //
    // CAUTION CAUTION CAUTION CAUTION CAUTION CAUTION CAUTION CAUTION CAUTION
    // CAUTION CAUTION CAUTION CAUTION CAUTION CAUTION CAUTION CAUTION CAUTION
    // (please read the comments above)
    //
    //
    // the following get functions may create the layout if it is not
    // created yet.
    //
    inline  CLayout   * GetUpdatedLayout();     // checks for NeedsLayout()
    inline  CLayout   * GetUpdatedLayoutPtr();  // Call this if NeedsLayout() is already called
    inline  BOOL        NeedsLayout();

    CLayout   * GetUpdatedNearestLayout();
    CTreeNode * GetUpdatedNearestLayoutNode();
    CElement  * GetUpdatedNearestLayoutElement();

    CLayout   * GetUpdatedParentLayout();
    CTreeNode * GetUpdatedParentLayoutNode();
    CElement  * GetUpdatedParentLayoutElement();

    // BUGBUG - these functions should go, we should not need
    // to know if the element has flowlayout.
    CFlowLayout   * GetFlowLayout();
    CTreeNode     * GetFlowLayoutNode();
    CElement      * GetFlowLayoutElement();
    CFlowLayout   * HasFlowLayout();
    CTableLayout  * HasTableLayout();
    //
    // Filters
    //
    BOOL            HasFilterPtr();

    // Helper methods
    htmlBlockAlign      GetParagraphAlign ( BOOL fOuter );
    htmlControlAlign    GetSiteAlign();

    BOOL IsInlinedElement ( );

    BOOL IsPositionStatic ( void );
    BOOL IsPositioned ( void );
    BOOL IsAbsolute ( stylePosition st );
    BOOL IsAbsolute ( void );

    BOOL IsAligned();

    // IsRelative() tells you if the specific element has had a CSS position
    // property set on it ( by examining _fRelative and _bPositionType on the
    // FF).  It will NOT tell you if something is relative because one of its
    // ancestors is relative; that information is stored in the CF, and can be
    // had via IsInheritingRelativeness()
    
    BOOL IsRelative ( stylePosition st );
    BOOL IsRelative ( void );
    BOOL IsInheritingRelativeness ( void );
    
    BOOL IsScrollingParent ( void );
    BOOL IsClipParent ( void );
    BOOL IsZParent ( void );
    BOOL IsDisplayNone( void );
    BOOL IsVisibilityHidden( void );

    //
    // Depth is defined to be 1 plus the count of parents above this element
    //

    int Depth() const;

    //
    // Format info functions
    //

    HRESULT         CacheNewFormats(CFormatInfo * pCFI);

    void                 EnsureFormats();
    BOOL                 IsCharFormatValid()    { return _iCF >= 0; }
    BOOL                 IsParaFormatValid()    { return _iPF >= 0; }
    BOOL                 IsFancyFormatValid()   { return _iFF >= 0; }
    const CCharFormat *  GetCharFormat()  { return(_iCF >= 0 ? ::GetCharFormatEx(_iCF) : GetCharFormatHelper()); }
    const CParaFormat *  GetParaFormat()  { return(_iPF >= 0 ? ::GetParaFormatEx(_iPF) : GetParaFormatHelper()); }
    const CFancyFormat * GetFancyFormat() { return(_iFF >= 0 ? ::GetFancyFormatEx(_iFF) : GetFancyFormatHelper()); }
    const CCharFormat *  GetCharFormatHelper();
    const CParaFormat *  GetParaFormatHelper();
    const CFancyFormat * GetFancyFormatHelper();
    long GetCharFormatIndex()  { return(_iCF >= 0 ? _iCF : GetCharFormatIndexHelper()); }
    long GetParaFormatIndex()  { return(_iPF >= 0 ? _iPF : GetParaFormatIndexHelper()); }
    long GetFancyFormatIndex() { return(_iFF >= 0 ? _iFF : GetFancyFormatIndexHelper()); }
    long GetCharFormatIndexHelper();
    long GetParaFormatIndexHelper();
    long GetFancyFormatIndexHelper();

    long            GetFontHeightInTwips ( CUnitValue * pCuv );
    void            GetRelTopLeft(CElement * pElementFL, CParentInfo * ppi,
                        long * pxOffset, long * pyOffset);

    // These GetCascaded methods are taken from style.hdl where they were
    // originally generated by the PDL parser.
    CColorValue        GetCascadedbackgroundColor();
    CColorValue        GetCascadedcolor();
    CUnitValue         GetCascadedletterSpacing();
    styleTextTransform GetCascadedtextTransform();
    CUnitValue         GetCascadedpaddingTop();
    CUnitValue         GetCascadedpaddingRight();
    CUnitValue         GetCascadedpaddingBottom();
    CUnitValue         GetCascadedpaddingLeft();
    CColorValue        GetCascadedborderTopColor();
    CColorValue        GetCascadedborderRightColor();
    CColorValue        GetCascadedborderBottomColor();
    CColorValue        GetCascadedborderLeftColor();
    styleBorderStyle   GetCascadedborderTopStyle();
    styleBorderStyle   GetCascadedborderRightStyle();
    styleBorderStyle   GetCascadedborderBottomStyle();
    styleBorderStyle   GetCascadedborderLeftStyle();
    CUnitValue         GetCascadedborderTopWidth();
    CUnitValue         GetCascadedborderRightWidth();
    CUnitValue         GetCascadedborderBottomWidth();
    CUnitValue         GetCascadedborderLeftWidth();
    CUnitValue         GetCascadedwidth();
    CUnitValue         GetCascadedheight();
    CUnitValue         GetCascadedtop();
    CUnitValue         GetCascadedbottom();
    CUnitValue         GetCascadedleft();
    CUnitValue         GetCascadedright();
    styleDataRepeat    GetCascadeddataRepeat();
    styleOverflow      GetCascadedoverflowX();
    styleOverflow      GetCascadedoverflowY();
    styleOverflow      GetCascadedoverflow();
    styleStyleFloat    GetCascadedfloat();
    stylePosition      GetCascadedposition();
    long               GetCascadedzIndex();
    CUnitValue         GetCascadedclipTop();
    CUnitValue         GetCascadedclipLeft();
    CUnitValue         GetCascadedclipRight();
    CUnitValue         GetCascadedclipBottom();
    BOOL               GetCascadedtableLayout();    // fixed - 1, auto - 0
    BOOL               GetCascadedborderCollapse(); // collapse - 1, separate - 0
    BOOL               GetCascadedborderOverride();
    WORD               GetCascadedfontWeight();
    WORD               GetCascadedfontHeight();
    CUnitValue         GetCascadedbackgroundPositionX();
    CUnitValue         GetCascadedbackgroundPositionY();
    BOOL               GetCascadedbackgroundRepeatX();
    BOOL               GetCascadedbackgroundRepeatY();
    htmlBlockAlign     GetCascadedblockAlign();
    styleVisibility    GetCascadedvisibility();
    styleDisplay       GetCascadeddisplay();
    BOOL               GetCascadedunderline();
    styleAccelerator   GetCascadedaccelerator();
    BOOL               GetCascadedoverline();
    BOOL               GetCascadedstrikeOut();
    CUnitValue         GetCascadedlineHeight();
    CUnitValue         GetCascadedtextIndent();
    BOOL               GetCascadedsubscript();
    BOOL               GetCascadedsuperscript();
    BOOL               GetCascadedbackgroundAttachmentFixed();
    styleListStyleType GetCascadedlistStyleType();
    styleListStylePosition GetCascadedlistStylePosition();
    long               GetCascadedlistImageCookie();
    stylePageBreak     GetCascadedpageBreakBefore();
    stylePageBreak     GetCascadedpageBreakAfter();
    const TCHAR      * GetCascadedfontFaceName();
    const TCHAR      * GetCascadedfontFamilyName();
    BOOL               GetCascadedfontItalic();
    long               GetCascadedbackgroundImageCookie();
    BOOL               GetCascadedclearLeft();
    BOOL               GetCascadedclearRight();
    styleCursor        GetCascadedcursor();
    styleTableLayout   GetCascadedtableLayoutEnum();
    styleBorderCollapse  GetCascadedborderCollapseEnum();
    styleDir           GetCascadedBlockDirection();
    styleDir           GetCascadeddirection();
    styleBidi          GetCascadedunicodeBidi();
    styleLayoutGridMode GetCascadedlayoutGridMode();
    styleLayoutGridType GetCascadedlayoutGridType();
    CUnitValue         GetCascadedlayoutGridLine();
    CUnitValue         GetCascadedlayoutGridChar();
    LONG               GetCascadedtextAutospace();
    styleWordBreak     GetCascadedwordBreak();
    styleLineBreak     GetCascadedlineBreak();
    styleTextJustify   GetCascadedtextJustify();
    styleTextJustifyTrim  GetCascadedtextJustifyTrim();
    CUnitValue         GetCascadedmarginTop();
    CUnitValue         GetCascadedmarginRight();
    CUnitValue         GetCascadedmarginBottom();
    CUnitValue         GetCascadedmarginLeft();
    CUnitValue         GetCascadedtextKashida();

    //
    // Ref helpers
    //    Right now these just drop right through to the element
    //
    static void ReplacePtr      ( CTreeNode ** ppNodelhs, CTreeNode * pNoderhs );
    static void SetPtr          ( CTreeNode ** ppNodelhs, CTreeNode * pNoderhs );
    static void ClearPtr        ( CTreeNode ** ppNodelhs );
    static void StealPtrSet     ( CTreeNode ** ppNodelhs, CTreeNode * pNoderhs );
    static void StealPtrReplace ( CTreeNode ** ppNodelhs, CTreeNode * pNoderhs );
    static void ReleasePtr      ( CTreeNode *  pNode );

    //
    // Other helpers
    //

    void VoidCachedInfo();
    void VoidCachedNodeInfo();
    void VoidFancyFormat();


    //
    // Helpers for contained CTreePos's
    //
    CTreePos *InitBeginPos(BOOL fEdge)
    {
        _tpBegin.SetFlags(  (_tpBegin.GetFlags() & ~(CTreePos::TPF_ETYPE_MASK|CTreePos::TPF_DATA_POS|CTreePos::TPF_EDGE))
                         |  CTreePos::NodeBeg
                         |  BOOLFLAG(fEdge, CTreePos::TPF_EDGE));
        WHEN_DBG( _tpBegin._eTypeDbg = CTreePos::NodeBeg );
        #if DBG == 1 || defined(DUMPTREE)
        _tpBegin.SetSN();
        #endif
        return &_tpBegin;
    }

    CTreePos *InitEndPos(BOOL fEdge)
    {
        _tpEnd.SetFlags(    (_tpEnd.GetFlags() & ~(CTreePos::TPF_ETYPE_MASK|CTreePos::TPF_DATA_POS|CTreePos::TPF_EDGE))
                        |   CTreePos::NodeEnd
                        |   BOOLFLAG(fEdge, CTreePos::TPF_EDGE));
        WHEN_DBG( _tpEnd._eTypeDbg = CTreePos::NodeEnd );
        #if DBG == 1 || defined(DUMPTREE)
        _tpEnd.SetSN();
        #endif
        return &_tpEnd;
    }

    //+-----------------------------------------------------------------------
    //
    //  CTreeNode::CLock
    //
    //------------------------------------------------------------------------

    class CLock
    {
    public:
        DECLARE_MEMALLOC_NEW_DELETE(Mt(CTreeNodeCLock))
        CLock(CTreeNode *pNode);
        ~CLock();

    private:
        CTreeNode *     _pNode;
    };

    //
    //
    // Lookaside pointers
    //

    enum
    {
        LOOKASIDE_PRIMARYTEAROFF        = 0,
        LOOKASIDE_CURRENTSTYLE          = 1,
        LOOKASIDE_NODE_NUMBER           = 2
        // *** There are only 2 bits reserved in the node.
        // *** if you add more lookasides you have to make sure 
        // *** that you make room for those bits.
    };

    BOOL            HasLookasidePtr(int iPtr)                   { return(_fHasLookasidePtr & (1 << iPtr)); }
    void *          GetLookasidePtr(int iPtr);
    HRESULT         SetLookasidePtr(int iPtr, void * pv);
    void *          DelLookasidePtr(int iPtr);

    // Primary Tearoff pointer management
    BOOL        HasPrimaryTearoff()                             { return(HasLookasidePtr(LOOKASIDE_PRIMARYTEAROFF)); }
    IUnknown *  GetPrimaryTearoff()                             { return((IUnknown *)GetLookasidePtr(LOOKASIDE_PRIMARYTEAROFF)); }
    HRESULT     SetPrimaryTearoff( IUnknown * pTearoff )        { return(SetLookasidePtr(LOOKASIDE_PRIMARYTEAROFF, pTearoff)); }
    IUnknown *  DelPrimaryTearoff()                             { return((IUnknown *)DelLookasidePtr(LOOKASIDE_PRIMARYTEAROFF)); }

    // CCurrentStyle pointer management
    BOOL            HasCurrentStyle()                                   { return(HasLookasidePtr(LOOKASIDE_CURRENTSTYLE)); }
    CCurrentStyle * GetCurrentStyle()                                   { return((CCurrentStyle *)GetLookasidePtr(LOOKASIDE_CURRENTSTYLE)); }
    HRESULT         SetCurrentStyle( CCurrentStyle * pCurrentStyle )    { return(SetLookasidePtr(LOOKASIDE_CURRENTSTYLE, pCurrentStyle)); }
    CCurrentStyle * DelCurrentStyle()                                   { return((CCurrentStyle *)DelLookasidePtr(LOOKASIDE_CURRENTSTYLE)); }

#if DBG==1
    union
    {
        void *              _apLookAside[LOOKASIDE_NODE_NUMBER];
        struct
        {
            IUnknown *      _pPrimaryTearoffDbg;
            CCurrentStyle * _pCurrentStyleDbg;
        };
    };
    ELEMENT_TAG             _etagDbg;
    DWORD                   _dwPad;
    #define CTREENODE_DBG_SIZE  (sizeof(void*)*2 + sizeof(ELEMENT_TAG) + sizeof(DWORD))
#else
    #define CTREENODE_DBG_SIZE  (0)
#endif

    //
    // Class Data
    //
    CElement *  _pElement;                   // The element for this node
    CTreeNode * _pNodeParent;                // The parent in the CTreeNode tree

    // DWORD 1
    BYTE        _etag;                              // 0-7:     element tag
    BYTE        _fFirstCommonAncestorNode   : 1;    // 8:       for finding common ancestor
    BYTE        _fInMarkup                  : 1;    // 9:       this node is in a markup and shouldn't die
    BYTE        _fInMarkupDestruction       : 1;    // 10:      Used by CMarkup::DestroySplayTree
    BYTE        _fHasLookasidePtr           : 2;    // 11-12    Lookaside flags
    BYTE        _fBlockNess                 : 1;    // 13:      Cached from format -- valid if _iFF != -1
    BYTE        _fHasLayout                 : 1;    // 14:      Cached from format -- valid if _iFF != -1
    BYTE        _fUnused                    : 1;    // 15:      Unused

    SHORT       _iPF;                               // 16-31:   Paragraph Format

    // DWORD 2
    SHORT       _iCF;                               // 0-15:    Char Format
    SHORT       _iFF;                               // 16-31:   Fancy Format

protected:
    // Use GetBeginPos() or GetEndPos() to get at these members
    CTreePos    _tpBegin;                    // The begin CTreePos for this node
    CTreePos    _tpEnd;                      // The end CTreePos for this node
    
public:
#if DBG == 1 || defined(DUMPTREE)
    union
    {
        struct
        {
            BOOL        _fInDestructor              : 1;    // The node is shutting down
            BOOL        _fSettingTearoff            : 1;    // In the process of setting the primary tearoff
            BOOL        _fDeadPending               : 1;    // Going to kill this node.  Used in splice
        };
        DWORD _dwDbgFlags;
    };

    const int _nSerialNumber;

    int SN ( ) const { return _nSerialNumber; }

    static int s_NextSerialNumber;
#define CTREENODE_DUMPTREE_SIZE  (sizeof(int) + sizeof(DWORD))
#else
#define CTREENODE_DUMPTREE_SIZE  (0)
#endif // DBG

    // STATIC MEMBERS

    DECLARE_TEAROFF_TABLE_NAMED(s_apfnNodeVTable)
private:

    NO_COPY(CTreeNode);
};

//
// If the compiler barfs on the next statement, it means that the size of the CTreeNode structure
// has grown beyond allowable limits.  You cannot increase the size of this structure without
// approval from the Trident development manager.
//
// Actually, this could also fire if you have shrunk the size of CTreeNode.  If that is the case,
// change the assert and pat yourself on the back.
//

COMPILE_TIME_ASSERT(CTreeNode, 2*sizeof(void*)+2*sizeof(DWORD) + 2 * sizeof(CTreePos) + CTREENODE_DBG_SIZE + CTREENODE_DUMPTREE_SIZE);

//+----------------------------------------------------------------------------
//
//  Class:      CMessage
//
//-----------------------------------------------------------------------------

DWORD FormsGetKeyState();   // KeyState helper

class HITTESTRESULTS
{
public:
    HITTESTRESULTS() { memset(this, 0, sizeof(HITTESTRESULTS)); };

    BOOL        _fPseudoHit;    // set to true if pseudohit (i.e. NOT text hit)
    BOOL        _fWantArrow;
    BOOL        _fRightOfCp;
    LONG        _cpHit;
    LONG        _iliHit;
    LONG        _ichHit;
    LONG        _cchPreChars;
};

struct FOCUS_ITEM
{
    CElement *  pElement;
    long        lTabIndex;
    long        lSubDivision;
};

class CMessage : public MSG
{
    void CommonCtor();
    NO_COPY(CMessage);
public:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CMessage))
    CMessage(const CMessage *pMessage);
    CMessage(const MSG * pmsg);
    CMessage(
        HWND hwnd,
        UINT msg,
        WPARAM wParam,
        LPARAM lParam);
    CMessage() {CommonCtor();}
    ~CMessage();

    void SetNodeHit(CTreeNode * pNodeHit);
    void SetNodeClk(CTreeNode * pNodeClk);

    BOOL IsContentPointValid() const
         {
             return (pDispNode != NULL);
         }
    void SetContentPoint(const CPoint & pt, CDispNode * pDispNodeContent)
         {
             ptContent = pt;
             pDispNode = pDispNodeContent;
         }
    void SetContentPoint(const POINT & pt, CDispNode * pDispNodeContent)
         {
             ptContent = pt;
             pDispNode = pDispNodeContent;
         }

    CPoint      pt;                     // Global  coordinates
    CPoint      ptContent;              // Content coordinates
    CDispNode * pDispNode;              // CDispNode associated with ptContent
    DWORD       dwKeyState;
    CTreeNode * pNodeHit;
    CTreeNode * pNodeClk;
    DWORD       dwClkData;              // Used to pass in any data for DoClick().
    HTC         htc;
    LRESULT     lresult;
    HITTESTRESULTS resultsHitTest;
    long        lSubDivision;

    unsigned     fNeedTranslate:1;      // TRUE if Trident should manually
                                        // call TranslateMessage, raid 44891
    unsigned     fRepeated:1;
    unsigned     fEventsFired:1;        // because we can have situation when FireStdEventsOnMessage
                                        // called twice with the same pMessage, this bit is used to
                                        // prevent firing same events 2nd time
    unsigned     fSelectionHMCalled:1;  // set once the selection has had a shot at the mess. perf.
    unsigned     fStopForward:1;        // prevent CElement::HandleMessage()
};


// fn ptr for mouse capture
#ifdef WIN16
typedef HRESULT (BUGCALL *PFN_VOID_MOUSECAPTURE)(CVoid *, CMessage *pMessage);

#define NV_DECLARE_MOUSECAPTURE_METHOD(fn, FN, args)\
        static HRESULT BUGCALL FN args;\
        HRESULT BUGCALL fn args

#define DECLARE_MOUSECAPTURE_METHOD(fn, FN, args)\
        static HRESULT BUGCALL FN args;\
        virtual HRESULT BUGCALL fn args

#define MOUSECAPTURE_METHOD(klass, fn, FN)\
    (PFN_VOID_MOUSECAPTURE)&klass::FN

#else

typedef HRESULT (BUGCALL CVoid::*PFN_VOID_MOUSECAPTURE)(CMessage *pMessage);

#define MOUSECAPTURE_METHOD(klass, fn, FN)\
    (PFN_VOID_MOUSECAPTURE)&klass::fn

#define NV_DECLARE_MOUSECAPTURE_METHOD(fn, FN, args)\
        HRESULT BUGCALL fn args

#define DECLARE_MOUSECAPTURE_METHOD(fn, FN, args)\
        virtual HRESULT BUGCALL fn args

#endif // ndef WIN16


//+----------------------------------------------------------------------------
//
//  Class:      CStyleInfo
//
//
//-----------------------------------------------------------------------------

class CStyleInfo
{
public:
    CStyleInfo() {}
    CStyleInfo(CTreeNode * pNodeContext) { _pNodeContext = pNodeContext; }
    CTreeNode * _pNodeContext;
};

//+----------------------------------------------------------------------------
//
//  Class:      CBehaviorInfo
//
//
//-----------------------------------------------------------------------------

class CBehaviorInfo : public CStyleInfo
{
public:
    CBehaviorInfo(CTreeNode * pNodeContext) : CStyleInfo (pNodeContext)
    {
    }
    ~CBehaviorInfo()
    {
        _acstrBehaviorUrls.Free();
    }

    CAtomTable  _acstrBehaviorUrls;       // urls of behaviors
};

typedef enum  {
    ComputeFormatsType_Normal = 0x0,
    ComputeFormatsType_GetValue = 0x1,
    ComputeFormatsType_GetInheritedValue = -1,  // 0xFFFFFFFF
    ComputeFormatsType_ForceDefaultValue = 0x2,
}  COMPUTEFORMATSTYPE;


//+----------------------------------------------------------------------------
//
//  Class:      CFormatInfo
//
//   Note:      This class exists for optimization purposes only.
//              This is so we only have to pass 1 object on the stack
//              while we are applying format.
//
//   WARNING    This class is allocated on the stack so you cannot count on
//              the new operator to clear things out for you.
//
//-----------------------------------------------------------------------------

class CFormatInfo : public CStyleInfo
{
private:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(Mem))
    CFormatInfo();
public:

    void            Reset()                  { memset(this, 0, offsetof(CFormatInfo, _icfSrc)); }
    void            Cleanup();
    CAttrArray *    GetAAExpando();
    void            PrepareCharFormatHelper();
    void            PrepareParaFormatHelper();
    void            PrepareFancyFormatHelper();
    HRESULT         ProcessImgUrl(CElement * pElem, LPCTSTR lpszUrl, DISPID dispID, LONG * plCookie, BOOL fHasLayout);

#if DBG==1
    void            UnprepareForDebug();
    void            PrepareCharFormat();
    void            PrepareParaFormat();
    void            PrepareFancyFormat();
    CCharFormat &   _cf();
    CParaFormat &   _pf();
    CFancyFormat &  _ff();
#else
    void            UnprepareForDebug()      {}
    void            PrepareCharFormat()      { if (!_fPreparedCharFormat)  PrepareCharFormatHelper(); }
    void            PrepareParaFormat()      { if (!_fPreparedParaFormat)  PrepareParaFormatHelper(); }
    void            PrepareFancyFormat()     { if (!_fPreparedFancyFormat) PrepareFancyFormatHelper(); }
    CCharFormat &   _cf()                    { return(_cfDst); }
    CParaFormat &   _pf()                    { return(_pfDst); }
    CFancyFormat &  _ff()                    { return(_ffDst); }
#endif

    // Data Members

#if DBG==1
    BOOL                    _fPreparedCharFormatDebug;  // To detect failure to call PrepareCharFormat
    BOOL                    _fPreparedParaFormatDebug;  // To detect failure to call PrepareParaFormat
    BOOL                    _fPreparedFancyFormatDebug; // To detect failure to call PrepareFancyFormat
#endif

    unsigned                _fPreparedCharFormat    :1; //  0
    unsigned                _fPreparedParaFormat    :1; //  1
    unsigned                _fPreparedFancyFormat   :1; //  2
    unsigned                _fPreparedAAExpando     :1; //  3
    unsigned                _fHasImageUrl           :1; //  4
    unsigned                _fHasBgColor            :1; //  5
    unsigned                _fNotInUse              :1; //  6
    unsigned                _fAlwaysUseMyColors     :1; //  7
    unsigned                _fAlwaysUseMyFontSize   :1; //  8
    unsigned                _fAlwaysUseMyFontFace   :1; //  9
    unsigned                _fHasFilters            :1; // 10
    unsigned                _fVisibilityHidden      :1; // 11
    unsigned                _fDisplayNone           :1; // 12
    unsigned                _fRelative              :1; // 13
    unsigned                _uTextJustify           :3; // 14,15,16
    unsigned                _uTextJustifyTrim       :2; // 17,18
    unsigned                _fPadBord               :1; // 19
    unsigned                _fHasBgImage            :1; // 20
    unsigned                _fNoBreak               :1; // 21
    unsigned                _fCtrlAlignLast         :1; // 22
    unsigned                _fPre                   :1; // 23
    unsigned                _fInclEOLWhite          :1; // 24
    unsigned                _fHasExpandos           :1; // 25
    unsigned                _fBidiEmbed             :1; // 26
    unsigned                _fBidiOverride          :1; // 27
    
    BYTE                    _bBlockAlign;               // Alignment set by DISPID_BLOCKALIGN
    BYTE                    _bControlAlign;             // Alignment set by DISPID_CONTROLALIGN
    BYTE                    _bCtrlBlockAlign;           // For elements with TAGDESC_OWNLINE, they also set the block alignment.
                                                        // Combined with _fCtrlAlignLast we use this to figure
                                                        // out what the correct value for the block alignment is.
    CUnitValue              _cuvTextIndent;
    CUnitValue              _cuvTextKashida;
    CAttrArray *            _pAAExpando;                // AA for style expandos
    CStr                    _cstrBgImgUrl;              // URL for background image
    CStr                    _cstrLiImgUrl;              // URL for <LI> image
    CStr                    _cstrFilters;               // New filters string
    

    // ^^^^^ All of the above fields are cleared by Reset() ^^^^^

    LONG                    _icfSrc;                    // _icf being inherited
    LONG                    _ipfSrc;                    // _ipf being inherited
    LONG                    _iffSrc;                    // _iff being inherited
    const CCharFormat *     _pcfSrc;                    // Original CCharFormat being inherited
    const CParaFormat *     _ppfSrc;                    // Original CParaFormat being inherited
    const CFancyFormat *    _pffSrc;                    // Original CFancyFormat being inherited
    const CCharFormat *     _pcf;                       // Same as _pcfSrc until _cf is prepared
    const CParaFormat *     _ppf;                       // Same as _ppfSrc until _pf is prepared
    const CFancyFormat *    _pff;                       // Same as _pffSrc until _ff is prepared

    // We can call ComputeFormats to get a style attribute that affects given element
    // _eExtraValues is uesd to request the special mode
    COMPUTEFORMATSTYPE      _eExtraValues;              // If not ComputeFormatsType_Normal next members are used
    DISPID                  _dispIDExtra;               // DISPID of the value requested
    VARIANT               * _pvarExtraValue;            // Returned value. Type depends on _dispIDExtra

    // We can pass in a style object from which to force values.  That is, no inheritance,
    // or cascading.  Just jam the values from this style obj into the formatinfo.  
    CStyle *                _pStyleForce;               // Style object that's forced in.  

private:

    CCharFormat             _cfDst;
    CParaFormat             _pfDst;
    CFancyFormat            _ffDst;
    CAttrArray              _AAExpando;

};


// fn ptr for visiting elements in groups (radiobuttons for now)

#ifdef WIN16
typedef HRESULT (BUGCALL *PFN_VISIT)(void *, DWORD_PTR dw);

#define VISIT_METHOD(kls, FN, fn)\
    (PFN_VISIT)&kls::fn

#define NV_DECLARE_VISIT_METHOD(FN, fn, args)\
    static HRESULT BUGCALL fn args;\
    HRESULT BUGCALL FN args;
#else

typedef HRESULT (BUGCALL CElement::*PFN_VISIT)(DWORD_PTR dw);

#define VISIT_METHOD(kls, FN, fn)\
    (PFN_VISIT)&kls::FN

#define NV_DECLARE_VISIT_METHOD(FN, fn, args)\
    HRESULT BUGCALL FN args;

#endif

#if defined(DYNAMICARRAY_NOTSUPPORTED)
#ifndef WIN16
#define CELEMENT_ACCELLIST_SIZE    30
#else
#define CELEMENT_ACCELLIST_SIZE    29
#endif

template <int n>
struct ACCEL_LIST_DYNAMIC
{
    const CElement::ACCEL_LIST * pnext; // Next ACCEL_LIST to search, NULL=last
    ACCEL aaccel[n];
    operator const CElement::ACCEL_LIST& () const
        { return *(CElement::ACCEL_LIST *) this; }
    CElement::ACCEL_LIST* operator & () const
        { return  (CElement::ACCEL_LIST *) this; }
};
#endif

//+----------------------------------------------------------------------------
//
//  Class:      CElement (element)
//
//   Note:      Derivation and virtual overload should be used to
//              distinguish different nodes.
//
//-----------------------------------------------------------------------------

EXTERN_C const GUID CLSID_CInlineStylePropertyPage;

class CElement : public CBase
{
    DECLARE_CLASS_TYPES(CElement, CBase)

    friend class CDBindMethods;
    friend class CLayout;
    friend class CFlowLayout;
    friend class CFilter;

private:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(Mem))

public:

    CElement (ELEMENT_TAG etag, CDoc *pDoc);

    virtual ~ CElement ( );

    CDoc * Doc () const { return GetDocPtr(); }

    //
    // creating thunks with AddRef and Release set to peer holder, if present
    //

    HRESULT CreateTearOffThunk(
        void *      pvObject1,
        const void * apfn1,
        IUnknown *  pUnkOuter,
        void **     ppvThunk,
        void *      appropdescsInVtblOrder = NULL);

    HRESULT CreateTearOffThunk(
        void*       pvObject1,
        const void * apfn1,
        IUnknown *  pUnkOuter,
        void **     ppvThunk,
        void *      pvObject2,
        void *      apfn2,
        DWORD       dwMask,
        const IID * const * apIID,
        void *      appropdescsInVtblOrder = NULL);

    //
    // IDispatchEx
    //

    NV_DECLARE_TEAROFF_METHOD(GetDispID, getdispid, (
        BSTR        bstrName,
        DWORD       grfdex,
        DISPID *    pdispid));

    NV_DECLARE_TEAROFF_METHOD(GetNextDispID, getnextdispid, (
        DWORD       grfdex,
        DISPID      dispid,
        DISPID *    pdispid));

    NV_DECLARE_TEAROFF_METHOD(GetMemberName, getmembername, (
        DISPID      dispid,
        BSTR *      pbstrName));

    //
    // IProvideMultipleClassInfo
    //

    NV_DECLARE_TEAROFF_METHOD(GetMultiTypeInfoCount, getmultitypeinfocount, (ULONG *pcti));
    NV_DECLARE_TEAROFF_METHOD(GetInfoOfIndex, getinfoofindex, (
            ULONG iti,
            DWORD dwFlags,
            ITypeInfo** pptiCoClass,
            DWORD* pdwTIFlags,
            ULONG* pcdispidReserved,
            IID* piidPrimary,
            IID* piidSource));

    CAttrArray **GetAttrArray ( ) const
    {
        return CBase::GetAttrArray();
    }
    void SetAttrArray (CAttrArray *pAA)
    {
        CBase::SetAttrArray(pAA);
    }

    HRESULT GetExpandoDispID(BSTR bstrName, DISPID *pid, DWORD grfdex);

    // *********************************************
    //
    // ENUMERATIONS, CLASSES, & STRUCTS
    //
    // *********************************************

    enum ELEMENTDESC_FLAG
    {
        ELEMENTDESC_DONTINHERITSTYLE= (BASEDESC_LAST << 1),   // Do not inherit style from parent
        ELEMENTDESC_NOANCESTORCLICK = (BASEDESC_LAST << 2),   // We don't want our ancestors to fire clicks
        ELEMENTDESC_NOTIFYENDPARSE  = (BASEDESC_LAST << 3),   // We want to be notified when we're parsed

        ELEMENTDESC_OLESITE         = (BASEDESC_LAST << 4), // class derived from COleSite
        ELEMENTDESC_OMREADONLY      = (BASEDESC_LAST << 5), // element's value can not be accessed through OM (input/file)
        ELEMENTDESC_ALLOWSCROLLING  = (BASEDESC_LAST << 6), // allow scrolling
        ELEMENTDESC_HASDEFDESCENT   = (BASEDESC_LAST << 7), // use 4 pixels descent for default vertical alignment
        ELEMENTDESC_BODY            = (BASEDESC_LAST << 8), // class is used the BODY element
        ELEMENTDESC_TEXTSITE        = (BASEDESC_LAST << 9), // class derived from CTxtSite
        ELEMENTDESC_ANCHOROUT       = (BASEDESC_LAST <<10), // draw anchor border outside site/inside
        ELEMENTDESC_SHOWTWS         = (BASEDESC_LAST <<11), // show trailing whitespaces
        ELEMENTDESC_XTAG            = (BASEDESC_LAST <<12), // an xtag - derived from CXElement
        ELEMENTDESC_NOOFFSETCTX     = (BASEDESC_LAST <<13), // Shift-F10 context menu shows on top-left (not offset)
        ELEMENTDESC_CARETINS_SL     = (BASEDESC_LAST <<14), // Dont select site after inserting site
        ELEMENTDESC_CARETINS_DL     = (BASEDESC_LAST <<15), //  show caret (SameLine or DifferntLine)
        ELEMENTDESC_NOSELECT        = (BASEDESC_LAST <<16), // do not select site in edit mode
        ELEMENTDESC_TABLECELL       = (BASEDESC_LAST <<17), // site is a table cell. Also implies do not
                                 // word break before sites.
        ELEMENTDESC_VPADDING        = (BASEDESC_LAST <<18), // add a pixel vertical padding for this site
        ELEMENTDESC_EXBORDRINMOV    = (BASEDESC_LAST <<19), // Exclude the border in CSite::Move
        ELEMENTDESC_DEFAULT         = (BASEDESC_LAST <<20), // acts like a default site to receive return key
        ELEMENTDESC_CANCEL          = (BASEDESC_LAST <<21), // acts like a cancel site to receive esc key
        ELEMENTDESC_NOBKGRDRECALC   = (BASEDESC_LAST <<22), // Dis-allow background recalc
        ELEMENTDESC_NOPCTRESIZE     = (BASEDESC_LAST <<23), // Don't force resize due to percentage height/width
        ELEMENTDESC_NOLAYOUT        = (BASEDESC_LAST <<24), // element's like script and comment etc. cannot have layout
        ELEMENTDESC_NEVERSCROLL     = (BASEDESC_LAST <<26), // element can scroll
        ELEMENTDESC_CANSCROLL       = (BASEDESC_LAST <<27), // element can scroll
        ELEMENTDESC_LAST            = ELEMENTDESC_CANSCROLL,
        ELEMENTDESC_MAX             = LONG_MAX                              // Force enum to DWORD on Macintosh.
    };

    // IDispatchEx methods
    NV_DECLARE_TEAROFF_METHOD(ContextThunk_Invoke, invoke, (
            DISPID dispidMember,
            REFIID riid,
            LCID lcid,
            WORD wFlags,
            DISPPARAMS * pdispparams,
            VARIANT * pvarResult,
            EXCEPINFO * pexcepinfo,
            UINT * puArgErr));

    NV_DECLARE_TEAROFF_METHOD(ContextThunk_InvokeEx, invokeex, (
            DISPID id,
            LCID lcid,
            WORD wFlags,
            DISPPARAMS *pdp,
            VARIANT *pvarRes,
            EXCEPINFO *pei,
            IServiceProvider *pSrvProvider));

    STDMETHOD(ContextInvokeEx)(
            DISPID id,
            LCID lcid,
            WORD wFlags,
            DISPPARAMS *pdp,
            VARIANT *pvarRes,
            EXCEPINFO *pei,
            IServiceProvider *pSrvProvider,
            IUnknown *pUnkContext);


    NV_DECLARE_TEAROFF_METHOD(GetNameSpaceParent, getnamespaceparent, (IUnknown **ppunk));

    NV_DECLARE_TEAROFF_METHOD(IsEqualObject, isequalobject, (IUnknown *ppunk));


    // ******************************************
    //
    // Virtual overrides
    //
    // ******************************************

    virtual void Passivate ( );

    DECLARE_PLAIN_IUNKNOWN(CElement)

    STDMETHOD(PrivateQueryInterface)(REFIID, void **);
    STDMETHOD_(ULONG, PrivateAddRef)();
    STDMETHOD_(ULONG, PrivateRelease)();

    void    PrivateEnterTree();
    void    PrivateExitTree( CMarkup * pMarkupOld );

    // Message helpers
    HRESULT __cdecl ShowMessage(
                int   * pnResult,
                DWORD dwFlags,
                DWORD dwHelpContext,
                UINT  idsMessage, ...);

    HRESULT ShowMessageV(
                int   * pnResult,
                DWORD   dwFlags,
                DWORD   dwHelpContext,
                UINT    idsMessage,
                void  * pvArgs);

    HRESULT ShowLastErrorInfo(HRESULT hr, int iIDSDefault=0);

    HRESULT ShowHelp(TCHAR * szHelpFile, DWORD dwData, UINT uCmd, POINT pt);


#ifndef NO_EDIT
    virtual IOleUndoManager * UndoManager(void);

    virtual BOOL QueryCreateUndo(BOOL fRequiresParent, BOOL fDirtyChange = TRUE);
#endif // NO_EDIT

    virtual HRESULT OnPropertyChange ( DISPID dispid, DWORD dwFlags );

    virtual HRESULT GetNaturalExtent(DWORD dwExtentMode, SIZEL *psizel) { return E_FAIL; }

    typedef enum
    {
        GETINFO_ISCOMPLETED,    // Has loading of an element completed
        GETINFO_HISTORYCODE     // Code used to validate history
    } GETINFO;

    virtual DWORD GetInfo(GETINFO gi);

    DWORD HistoryCode() { return GetInfo(GETINFO_HISTORYCODE); }

    NV_DECLARE_TEAROFF_METHOD(get_tabIndex, GET_tabIndex, (short *));

    // these datafld, datasrc, and dataformatas properties are declared
    //  baseimplementation in the pdl; we need prototypes here
    NV_DECLARE_TEAROFF_METHOD(put_dataFld, PUT_dataFld, (BSTR v));
    NV_DECLARE_TEAROFF_METHOD(get_dataFld, GET_dataFld, (BSTR*p));
    NV_DECLARE_TEAROFF_METHOD(put_dataSrc, PUT_dataSrc, (BSTR v));
    NV_DECLARE_TEAROFF_METHOD(get_dataSrc, GET_dataSrc, (BSTR*p));
    NV_DECLARE_TEAROFF_METHOD(put_dataFormatAs, PUT_dataFormatAs, (BSTR v));
    NV_DECLARE_TEAROFF_METHOD(get_dataFormatAs, GET_dataFormatAs, (BSTR*p));

    // The dir property is declared baseimplementation in the pdl.
    // We need prototype here
    NV_DECLARE_TEAROFF_METHOD(put_dir, PUT_dir, (BSTR v));
    NV_DECLARE_TEAROFF_METHOD(get_dir, GET_dir, (BSTR*p));

    // these delegaters implement redirection to the window object
    NV_DECLARE_TEAROFF_METHOD(get_onload, GET_onload, (VARIANT*));
    NV_DECLARE_TEAROFF_METHOD(put_onload, PUT_onload, (VARIANT));
    NV_DECLARE_TEAROFF_METHOD(get_onunload, GET_onunload, (VARIANT*));
    NV_DECLARE_TEAROFF_METHOD(put_onunload, PUT_onunload, (VARIANT));
    NV_DECLARE_TEAROFF_METHOD(get_onfocus, GET_onfocus, (VARIANT*));
    NV_DECLARE_TEAROFF_METHOD(put_onfocus, PUT_onfocus, (VARIANT));
    NV_DECLARE_TEAROFF_METHOD(get_onblur, GET_onblur, (VARIANT*));
    NV_DECLARE_TEAROFF_METHOD(put_onblur, PUT_onblur, (VARIANT));
    NV_DECLARE_TEAROFF_METHOD(get_onbeforeunload, GET_onbeforeunload, (VARIANT*));
    NV_DECLARE_TEAROFF_METHOD(put_onbeforeunload, PUT_onbeforeunload, (VARIANT));
    NV_DECLARE_TEAROFF_METHOD(get_onhelp, GET_onhelp, (VARIANT*));
    NV_DECLARE_TEAROFF_METHOD(put_onhelp, PUT_onhelp, (VARIANT));

    NV_DECLARE_TEAROFF_METHOD(get_onscroll, GET_onscroll, (VARIANT*));
    NV_DECLARE_TEAROFF_METHOD(put_onscroll, PUT_onscroll, (VARIANT));
    NV_DECLARE_TEAROFF_METHOD(get_onresize, GET_onresize, (VARIANT*));
    NV_DECLARE_TEAROFF_METHOD(put_onresize, PUT_onresize, (VARIANT));

    NV_DECLARE_TEAROFF_METHOD(get_onbeforeprint, GET_onbeforeprint, (VARIANT*));
    NV_DECLARE_TEAROFF_METHOD(put_onbeforeprint, PUT_onbeforeprint, (VARIANT));
    NV_DECLARE_TEAROFF_METHOD(get_onafterprint, GET_onafterprint, (VARIANT*));
    NV_DECLARE_TEAROFF_METHOD(put_onafterprint, PUT_onafterprint, (VARIANT));

    // non-abstract getters for tagName and scopeName. See element.pdl
    NV_DECLARE_TEAROFF_METHOD(GettagName, gettagname, (BSTR*));
    NV_DECLARE_TEAROFF_METHOD(GetscopeName, getscopename, (BSTR*));

    // IServiceProvider methods
    NV_DECLARE_TEAROFF_METHOD(QueryService, queryservice, (REFGUID guidService, REFIID iid, LPVOID * ppv));

    // IRecalcProperty methods
    NV_DECLARE_TEAROFF_METHOD(GetCanonicalProperty, getcanonicalproperty, (DISPID dispid, IUnknown **ppUnk, DISPID *pdispid));

    virtual CAtomTable * GetAtomTable ( BOOL *pfExpando = NULL );

    //
    // init / deinit methods
    //

    class CInit2Context
    {
    public:
        CInit2Context(CHtmTag * pht, CMarkup * pTargetMarkup, DWORD dwFlags) :
          _pTargetMarkup(pTargetMarkup),
          _pht(pht),
          _dwFlags(dwFlags)
          {
          };

        CInit2Context(CHtmTag * pht, CMarkup * pTargetMarkup) :
          _pTargetMarkup(pTargetMarkup),
          _pht(pht),
          _dwFlags(0)
          {
          };


        CHtmTag *   _pht;
        CMarkup *   _pTargetMarkup;
        DWORD       _dwFlags;
    };

    virtual HRESULT Init();
    virtual HRESULT Init2(CInit2Context * pContext);

            HRESULT InitAttrBag (CHtmTag * pht);
            HRESULT MergeAttrBag(CHtmTag * pht);

    virtual void    Notify (CNotification * pnf);

    HRESULT         EnterTree();
    void            ExitTree( DWORD dwExitFlags );

    //
    // other
    //

    CBase *GetOmWindow ( void );

    //
    // Get the Base Object that owns the attr array for a given property
    // Allows us to re-direct properties to another objects storage
    //
    CBase *GetBaseObjectFor ( DISPID dispID );

    HRESULT ConnectInlineEventHandler(
        DISPID      dispid,
        DISPID      dispidCode,
        ULONG       uOffset,
        ULONG       uLine,
        BOOL        fStandard,
        LPCTSTR *   ppchLanguageCached = NULL);

    //
    // Pass the call to the form.
    //-------------------------------------------------------------------------
    //  +override : special process
    //  +call super : first
    //  -call parent : no
    //  -call children : no
    //-------------------------------------------------------------------------
    virtual HRESULT CloseErrorInfo ( HRESULT hr );

    //
    // Scrolling methods
    //

    // Scroll this element into view
    //-------------------------------------------------------------------------
    virtual HRESULT ScrollIntoView (SCROLLPIN spVert = SP_MINIMAL,
                                    SCROLLPIN spHorz = SP_MINIMAL,
                                    BOOL fScrollBits = TRUE);

    HRESULT DeferScrollIntoView(
        SCROLLPIN spVert = SP_MINIMAL, SCROLLPIN spHorz = SP_MINIMAL );

    NV_DECLARE_ONCALL_METHOD(DeferredScrollIntoView, deferredscrollintoview, (DWORD_PTR dwParam));

    //
    // Relative element (non-site) helper methods
    //
    virtual HTC  HitTestPoint(
                     CMessage *pMessage,
                     CTreeNode ** ppNodeElement,
                     DWORD dwFlags);

    BOOL CanHandleMessage()
    {
        return (HasLayout()) ? (IsEnabled() && IsVisible(TRUE)) : (TRUE);
    }

    // BUGCALL required when passing fn through generic ptr
    virtual HRESULT BUGCALL HandleMessage(CMessage * pMessage);

    HRESULT BUGCALL HandleCaptureMessage(CMessage * pMessage)
        { return HandleMessage(pMessage); }

#ifdef WIN16
    static HRESULT BUGCALL handlecapturemessage(CElement * pObj, CMessage * pMessage)
        { return pObj->HandleCaptureMessage(pMessage); }
#endif

    //
    // marka these are to be DELETED !!
    //
    BOOL DisallowSelection();

    // set the state of the IME.
    HRESULT SetImeState();
    HRESULT ComputeExtraFormat(
        DISPID dispID,
        BOOL fInherits,
        CTreeNode * pTreeNode,
        VARIANT *pVarReturn);

    // DoClick() is called by click(). It is also called internally in response
    // to a mouse click by user.
    // DoClick() fires the click event and then calls ClickAction() if the event
    // is not cancelled.
    // Derived classes can override ClickAction() to provide specific functionality.
    virtual HRESULT DoClick(CMessage * pMessage = NULL, CTreeNode *pNodeContext = NULL,
        BOOL fFromLabel = FALSE);
    virtual HRESULT ClickAction(CMessage * pMessage);

    virtual HRESULT ShowTooltip(CMessage *pmsg, POINT pt);

    HRESULT SetCursorStyle(LPCTSTR pstrIDC, CTreeNode *pNodeContext = NULL);

    void    Invalidate();

    HRESULT OnCssChange(BOOL fStable, BOOL fRecomputePeers);

    // Element rect and element invalidate support

    enum GERC_FLAGS {
        GERC_ONALINE = 1,
        GERC_CLIPPED = 2
    };

    void    GetElementRegion(CDataAry<RECT> * paryRects, RECT * prcBound = NULL, DWORD dwFlags = 0);
    HRESULT GetElementRc(RECT     *prc,
                         DWORD     dwFlags,
                         POINT    *ppt = NULL);

    // these helper functions return in container coordinates
    void    GetBoundingSize(SIZE & sz);
    HRESULT GetBoundingRect(CRect * pRect, DWORD dwFlags = 0);
    HRESULT GetElementTopLeft(POINT & pt);

    // helper to return the actual background color
    COLORREF GetInheritedBackgroundColor(CTreeNode * pNodeContext = NULL );
    HRESULT  GetInheritedBackgroundColorValue(CColorValue *pVal,
                                         CTreeNode * pNodeContext = NULL );
    virtual HRESULT GetColors(CColorInfo * pCI);
    COLORREF GetBackgroundColor()
    {
        CTreeNode * pNodeContext = GetFirstBranch();
        CTreeNode * pNodeParent  = pNodeContext->Parent()
                                 ? pNodeContext->Parent() : pNodeContext;

        return pNodeParent->Element()->GetInheritedBackgroundColor(pNodeParent);
    }

    //
    //  Persistence Helpers
    //-------------------------------------------------------------------------------------------
    HRESULT DoPersistFavorite(IHTMLPersistData *pIPersist, void * pvNotify, PERSIST_TYPE ptype);
    HRESULT DoPersistHistorySave(IHTMLPersistData *pIPersist, void * pvNotify);
    HRESULT DoPersistHistoryLoad(IHTMLPersistData *pIPersist, void * pvNotify);
    HRESULT GetPersistenceCache( IXMLDOMDocument **ppXMLDoc );
    BOOL    PersistAccessAllowed(INamedPropertyBag * pINPB);
    BSTR    GetPersistID( BSTR bstrParentName=NULL );
    HRESULT TryPeerPersist(PERSIST_TYPE ptype, void * pvNotify);
    HRESULT TryPeerSnapshotSave (IUnknown * pDesignDoc);
    IHTMLPersistData * GetPeerPersist();

    //
    // Events related stuff
    //--------------------------------------------------------------------------------------
    inline BOOL    ShouldFireEvents() { return _fEventListenerPresent; }
    inline void    SetEventsShouldFire() 
        { _fEventListenerPresent = TRUE;  }
    BOOL    FireCancelableEvent   ( DISPID dispidMethod, DISPID dispidProp, LPCTSTR pchEventType, BYTE * pbTypes, ... );
    BOOL    BubbleCancelableEvent ( CTreeNode * pNodeContext, long lSubDivision, DISPID dispidMethod, DISPID dispidProp, LPCTSTR pchEventType, BYTE * pbTypes, ... );
    HRESULT FireEventHelper       ( DISPID dispidMethod, DISPID dispidProp, BYTE * pbTypes,  ...);
    HRESULT FireEvent             ( DISPID dispidMethod, DISPID dispidProp, LPCTSTR pchEventType, BYTE * pbTypes, ... );
    HRESULT BubbleEvent           ( CTreeNode * pNodeContext, long lSubDivision, DISPID dispidMethod, DISPID dispidProp, LPCTSTR pchEventType, BYTE * pbTypes, ... );
    HRESULT BubbleEventHelper     ( CTreeNode * pNodeContext, long lSubDivision, DISPID dispidMethod, DISPID dispidProp, BOOL fRaisedByPeer, VARIANT *pvb, BYTE * pbTypes, ...);
    virtual HRESULT DoSubDivisionEvents(long lSubDivision, DISPID dispidMethod, DISPID dispidProp, VARIANT *pvb, BYTE * pbTypes, ...);
    HRESULT FireStdEventOnMessage ( CTreeNode * pNodeContext,
                                    CMessage * pMessage,
                                    CTreeNode * pNodeBeginBubbleWith = NULL,
                                    CTreeNode * pNodeEvent = NULL);
    BOOL FireStdEvent_KeyDown     ( CTreeNode * pNodeContext, CMessage *pMessage, int *piKeyCode, short shift );
    BOOL FireStdEvent_KeyUp       ( CTreeNode * pNodeContext, CMessage *pMessage, int *piKeyCode, short shift );
    BOOL FireStdEvent_KeyPress    ( CTreeNode * pNodeContext, CMessage *pMessage, int *piKeyCode);
    BOOL FireStdEvent_MouseHelper (CTreeNode *  pNodeContext,
                                   CMessage *pMessage,
                                   DISPID       dispidMethod,
                                   DISPID       dispidProp,
                                   short        button,
                                   short        shift,
                                   float        x,
                                   float        y,
                                   CTreeNode *  pNodeFrom = NULL,
                                   CTreeNode *  pNodeTo   = NULL,
                                   CTreeNode *  pNodeBeginBubbleWith = NULL,
                                   CTreeNode * pNodeEvent = NULL);

    void    Fire_onpropertychange(LPCTSTR strPropName);
    void    Fire_onscroll();
    HRESULT Fire_PropertyChangeHelper(DISPID dispid, DWORD dwFlags);

    void BUGCALL Fire_onfocus(DWORD_PTR dwContext);
    void BUGCALL Fire_onblur(DWORD_PTR dwContext);

    BOOL DragElement(CLayout *                  pFlowLayout,
                     DWORD                      dwStateKey,
                     IUniformResourceLocator *  pUrlToDrag,
                     long                       lSubDivision);

    BOOL Fire_ondragHelper(long lSubDivision,
                           DISPID dispidEvent,
                           DISPID dispidProp,
                           LPCTSTR pchType,
                           DWORD * pdwDropEffect);
    void Fire_ondragend(long lSubDivision, DWORD dwDropEffect);


    // onData* event firing helper..
    HRESULT Fire_ondatasetX(LPTSTR pchQualifier, long lReason, DISPID dispidEvent,
                            DISPID dispidProp);

    // Associated image context helpers
    HRESULT GetImageUrlCookie(LPCTSTR lpszURL, long *plCtxCookie);
    HRESULT AddImgCtx(DISPID dispID, LONG lCookie);
    void    ReleaseImageCtxts();
    void    DeleteImageCtx(DISPID dispid);

    // copy the common attributes from a given element
    HRESULT MergeAttributes(CElement * pElement, BOOL fOMCall = TRUE, BOOL fCopyID = FALSE);

#ifndef NO_DATABINDING
    //
    // Data binding methods.
    //
    virtual const CDBindMethods * GetDBindMethods ()     {return NULL; }

    HRESULT         AttachDataBindings ( );
    HRESULT         CreateDatabindRequest(LONG id, DBSPEC *pdbs = NULL);
    void            DetachDataBinding (LONG id);
    void            DetachDataBindings ( );

    HRESULT         EnsureDBMembers();
    DBMEMBERS *     GetDBMembers()    { return(GetDataBindPtr()); }
    HRESULT         FindDatabindContext(LPCTSTR strDataSrc, LPCTSTR strDataFld,
                            CElement **ppElementOuter, CElement **ppElementRepeat,
                            LPCTSTR *pstrTail);

    CDataMemberMgr * GetDataMemberManager();
    HRESULT         EnsureDataMemberManager();

    BOOL            IsDataProvider();

    // hooks for databinding from CSite, to be overridden by derived classes
    //
    virtual CElement * GetElementDataBound();
    virtual HRESULT SaveDataIfChanged(LONG id, BOOL fLoud = FALSE, BOOL fForceIsCurrent=FALSE);
#endif // ndef NO_DATABINDING

    //
    // Internal events
    //

    HRESULT EnsureFormatCacheChange ( DWORD dwFlags);
    BOOL    IsFormatCacheValid();

    //
    // Element Tree methods
    //

    BOOL IsAligned();
    BOOL IsContainer();
    BOOL IsNoScope();
    BOOL IsBlockElement();
    BOOL IsBlockTag();
    BOOL IsOwnLineElement(CFlowLayout *pFlowLayoutContext);
    BOOL IsRunOwner() { return _fOwnsRuns; }
    BOOL BreaksLine();
    BOOL IsTagAndBlock (ELEMENT_TAG   eTag) { return Tag() == eTag && IsBlockElement(); }
    BOOL IsFlagAndBlock(TAGDESC_FLAGS flag) { return HasFlag(flag) && IsBlockElement(); }
    
    HRESULT ClearRunCaches(DWORD dwFlags);

    //-------------------------------------------------------------------------
    //
    // Tree related functions
    //
    //-------------------------------------------------------------------------

    // DOM helpers
    HRESULT GetDOMInsertPosition ( CElement *pRefElement, CDOMTextNode *pRefTextNode, CMarkupPointer *pmkpPtr );
    CDOMChildrenCollection *EnsureDOMChildrenCollectionPtr();
    HRESULT DOMWalkChildren ( long lWantItem, long *plCount, IDispatch **ppDispItem );
    // XOM Helpers
    HRESULT GetMarkupPtrRange(CMarkupPointer *pPtrStart, CMarkupPointer *pPtrEnd, BOOL fEnsureMarkup = FALSE);

    //
    // Get the first branch for this element
    //

    CTreeNode * GetFirstBranch ( ) const { return __pNodeFirstBranch; }
    CTreeNode * GetLastBranch ( );
    BOOL IsOverlapped();

    //
    // Get the CTreePos extent of this element
    //

    void        GetTreeExtent ( CTreePos ** pptpStart, CTreePos ** pptpEnd );

    BOOL        IsOverflowFrame();

    //-------------------------------------------------------------------------
    //
    // Layout related functions
    //
    //-------------------------------------------------------------------------

private:
            BOOL        HasLayoutLazy();    // should only be called by HasLayout
    inline  CLayout   * GetLayoutLazy();
    //
    // Create the layout object to be associated with the current element
    //
    virtual HRESULT CreateLayout();

public:

    //
    // CAUTION CAUTION CAUTION CAUTION CAUTION CAUTION CAUTION CAUTION CAUTION
    // CAUTION CAUTION CAUTION CAUTION CAUTION CAUTION CAUTION CAUTION CAUTION
    // (please read the comments below)
    //
    // The layout attached to the current element may not be accurate, when a
    // property changes, current element can gain/lose layoutness. When an
    // element gains/loses layoutness, its layout is created/destroyed lazily.
    //
    // So, for the following functions "cur" means return the layout currently
    // associated with the layout which may not be accurate. "Updated" means
    // compute the state and return the accurate information.
    //
    // Note: Calling "Updated" function may cause the formats to be computed.
    //
    // If there is any confusion please talk to (srinib/lylec/brendand)
    //
    CLayout   * GetCurLayout()      { return GetLayoutPtr(); }
    BOOL        HasLayout()         { return !!HasLayoutPtr(); }

    CLayout   * GetCurNearestLayout();
    CTreeNode * GetCurNearestLayoutNode();
    CElement  * GetCurNearestLayoutElement();

    CLayout   * GetCurParentLayout();
    CTreeNode * GetCurParentLayoutNode();
    CElement  * GetCurParentLayoutElement();

    //
    // CAUTION CAUTION CAUTION CAUTION CAUTION CAUTION CAUTION CAUTION CAUTION
    // CAUTION CAUTION CAUTION CAUTION CAUTION CAUTION CAUTION CAUTION CAUTION
    // (please read the comments above)
    //
    //
    // the following get functions may create the layout if it is not
    // created yet.
    //
    inline  CLayout   * GetUpdatedLayout();     // checks for NeedsLayout()
    inline  CLayout   * GetUpdatedLayoutPtr();  // Call this if NeedsLayout() is already called
    inline  BOOL        NeedsLayout();

    CLayout   * GetUpdatedNearestLayout();
    CTreeNode * GetUpdatedNearestLayoutNode();
    CElement  * GetUpdatedNearestLayoutElement();

    CLayout   * GetUpdatedParentLayout();
    CTreeNode * GetUpdatedParentLayoutNode();
    CElement  * GetUpdatedParentLayoutElement();


    void        DestroyLayout();

    // this functions return GetFirstBranch()->Parent()->Ancestor(etag)->Element(); safely
    // returns NULL if the element is not in the tree, or it doesn't have a parent, or no such ansectors in the tree, etc.
    CElement  * GetParentAncestorSafe(ELEMENT_TAG etag) const;
    CElement  * GetParentAncestorSafe(ELEMENT_TAG *arytag) const;

    // BUGBUG - these functions should go, we should not need
    // to know if the element has flowlayout.
    CFlowLayout   * GetFlowLayout();
    CTreeNode     * GetFlowLayoutNode();
    CElement      * GetFlowLayoutElement();
    CFlowLayout   * HasFlowLayout();
    CTableLayout  * HasTableLayout();

    //
    // Notification helpers
    //
    void    InvalidateElement(DWORD grfFlags = 0);
    void    MinMaxElement(DWORD grfFlags = 0);
    void    ResizeElement(DWORD grfFlags = 0);
    void    RemeasureElement(DWORD grfFlags = 0);
    void    RemeasureInParentContext(DWORD grfFlags = 0);
    void    RepositionElement(DWORD grfFlags = 0);
    void    ZChangeElement(DWORD grfFlags = 0, CPoint * ppt = NULL);

    void    SendNotification(enum NOTIFYTYPE ntype, DWORD grfFlags = 0, void * pvData = 0);
    void    SendNotification(enum NOTIFYTYPE ntype, void * pvData)
            {
                SendNotification(ntype, 0, pvData);
            }
    void    SendNotification(CNotification *pNF);

    long    GetSourceIndex ( void );

    long    CompareZOrder(CElement * pElement);

    //
    // Mark an element's layout (if any) dirty
    //
    void    DirtyLayout(DWORD grfLayout = 0);
    BOOL    OpenView();

    BOOL    HasPercentBgImg();
    //
    // cp and run related helper functions
    //
    long GetFirstCp();
    long GetLastCp();
    long GetElementCch();
    long GetFirstAndLastCp(long * pcpFirst, long * pcpLast);

    //
    // get the border information related to the element
    //

    virtual DWORD GetBorderInfo(CDocInfo * pdci, CBorderInfo *pbi, BOOL fAll = FALSE);

    HRESULT GetRange(long * pcp, long * pcch);
    HRESULT GetPlainTextInScope(CStr * pstrText);

    virtual HRESULT Save ( CStreamWriteBuff * pStreamWrBuff, BOOL fEnd );
            HRESULT WriteTag ( CStreamWriteBuff * pStreamWrBuff, BOOL fEnd, BOOL fForce = FALSE );
    virtual HRESULT SaveAttributes ( CStreamWriteBuff * pStreamWrBuff, BOOL *pfAny = NULL );
    HRESULT SaveAttributes ( IPropertyBag * pPropBag, BOOL fSaveBlankAttributes = TRUE );
    HRESULT SaveAttribute (
        CStreamWriteBuff *      pStreamWrBuff,
        LPTSTR                  pchName,
        LPTSTR                  pchValue,
        const PROPERTYDESC *    pPropDesc = NULL,
        CBase *                 pBaseObj = NULL,
        BOOL                    fEqualSpaces = TRUE,
        BOOL                    fAlwaysQuote = FALSE);

    ELEMENT_TAG Tag ( ) const { return (ELEMENT_TAG) _etag; }
    inline ELEMENT_TAG TagType( ) const {
        switch (_etag)
        {
        case ETAG_GENERIC_LITERAL:
        case ETAG_GENERIC_BUILTIN:
            return ETAG_GENERIC;
        default:
            return (ELEMENT_TAG) _etag;
        }
    }

    
#ifndef VSTUDIO7
    virtual const TCHAR * TagName ();
    virtual const TCHAR * Namespace();
#else
    // Set the tagname and namespace of the element.
    HRESULT SetTagNameAndScope(CHtmTag *pht);
    const TCHAR * TagName ();
    const TCHAR * Namespace();
#endif //VSTUDIO7

    const TCHAR * NamespaceHtml();
    
    BOOL CanContain ( ELEMENT_TAG etag ) const;

    // Support for sub-objects created through pdl's
    // CStyle & Celement implement this differently

    CElement * GetElementPtr ( ) { return this; }

    BOOL CanShow();

    BOOL HasFlag (enum TAGDESC_FLAGS) const;

    static void ReplacePtr ( CElement ** pplhs, CElement * prhs );
    static void ReplacePtrSub ( CElement ** pplhs, CElement * prhs );
    static void SetPtr     ( CElement ** pplhs, CElement * prhs );
    static void ClearPtr   ( CElement ** pplhs );
    static void StealPtrSet     ( CElement ** pplhs, CElement * prhs );
    static void StealPtrReplace ( CElement ** pplhs, CElement * prhs );
    static void ReleasePtr      ( CElement *  pElement );

    // Write unknown attr set
    HRESULT SaveUnknown ( CStreamWriteBuff * pStreamWrBuff, BOOL *pfAny = NULL );
    HRESULT SaveUnknown ( IPropertyBag * pPropBag, BOOL fSaveBlankAttributes = TRUE );

    // Helpers
    BOOL IsProperlyContained ( );
    BOOL IsNamed ( ) const { return !!_fIsNamed; }

    // CSS Extension object (Filters)
	void    ComputeFilterFormat(CFormatInfo *pCFI);
    HRESULT ApplyFilterCollection();
    STDMETHODIMP AddExtension( TCHAR *name, CPropertyBag *pBag, CCSSFilterSite *pSite );

    //
    // comparison
    //

    LPCTSTR NameOrIDOfParentForm ( );


    // Property bag management

//    virtual void *  GetPropMemberPtr(const BASICPROPPARAMS * pbpp, const void * pvParams );

    // Helper for name or ID
    LPCTSTR GetIdentifier ( void );
    HRESULT GetUniqueIdentifier (CStr * pcstr, BOOL fSetWhenCreated = FALSE, BOOL *pfDidCreate = NULL );
    LPCTSTR GetAAname() const;
    LPCTSTR GetAAsubmitname() const;

    HRESULT AddAllScriptlets(TCHAR * pchExposedName);

    //
    // Paste helpers
    //

    enum Where {
        Inside,
        Outside,
        BeforeBegin,
        AfterBegin,
        BeforeEnd,
        AfterEnd
    };

    HRESULT Inject ( Where, BOOL fIsHtml, LPTSTR pStr, long cch );

    virtual HRESULT PasteClipboard() { return S_OK; }

    HRESULT InsertAdjacent ( Where where, CElement * pElementInsert );

    HRESULT RemoveOuter ( );

    // Helper to get the specified text under the element -- invokes saver.
    HRESULT GetText(BSTR * pbstr, DWORD dwStm);

    // Another helper for databinding
    HRESULT GetBstrFromElement ( BOOL fHTML, BSTR * pbstr );

    // Helper for Pdl-generated method impl's that depend on design/run time
    BOOL IsDesignMode();
    // CBase function used for implementing ISpecifyPropertyPages. Most
    // people should use the non-virtual function IsDesignMode() directly.
    virtual BOOL DesignMode() { return IsDesignMode(); }

    //
    // Collection Management helpers
    //
    NV_DECLARE_PROPERTY_METHOD(GetIDHelper, GETIDHelper, (CStr * pf));
    NV_DECLARE_PROPERTY_METHOD(SetIDHelper, SETIDHelper, (CStr * pf));
    NV_DECLARE_PROPERTY_METHOD(GetnameHelper, GETNAMEHelper, (CStr * pf));
    NV_DECLARE_PROPERTY_METHOD(SetnameHelper, SETNAMEHelper, (CStr * pf));
    LPCTSTR         GetAAid() const; 
    void            InvalidateCollection ( long lIndex );
    NV_STDMETHOD    (removeAttribute) (BSTR strPropertyName, LONG lFlags, VARIANT_BOOL *pfSuccess);
    HRESULT         SetUniqueNameHelper ( LPCTSTR szUniqueName );
    HRESULT         SetIdentifierHelper ( LPCTSTR lpszValue, DISPID dspIDThis, DISPID dspOther1, DISPID dspOther2 );
    void            OnEnterExitInvalidateCollections(BOOL);
        void                    DoElementNameChangeCollections(void);

    //
    // Clone - make a duplicate new element
    //
    // !! returns an element with no node!
    //
    virtual HRESULT Clone(CElement **ppElementClone, CDoc *pDoc);
                  
    //
    // behaviors support
    //

    HRESULT         ProcessPeerTask(PEERTASK task);
    HRESULT         OnPeerListChange();

    BOOL            HasIdentityPeer();
    BOOL            NeedsIdentityPeer();
    HRESULT         EnsureIdentityPeer(BOOL fForceCreate = FALSE);
    CPeerHolder *   GetIdentityPeerHolder();

    CPeerHolder *   GetRenderPeerHolder();
    BOOL            HasRenderPeerHolder();

#ifdef VSTUDIO7
    BOOL            NeedsVStudioPeer();
#endif //VSTUDIO7

    //
    // Debug helpers
    //

#if DBG==1 || defined(DUMPTREE)
    int SN () const { return _nSerialNumber; }
#endif // DBG

    void ComputeHorzBorderAndPadding(CCalcInfo * pci, CTreeNode * pNodeContext, CElement * pTxtSite,
                    LONG * pxBorderLeft, LONG *pxPaddingLeft,
                    LONG * pxBorderRight, LONG *pxPaddingRight);

    HRESULT SetDim(DISPID                    dispID,
                   float                     fValue,
                   CUnitValue::UNITVALUETYPE uvt,
                   long                      lDimOf,
                   CAttrArray **             ppAA,
                   BOOL                      fInlineStyle,
                   BOOL *                    pfChanged);

    HRESULT StealAttributes(CElement *pElementVictim);
    HRESULT MergeAttributesInternal(IHTMLElement *pIHTMLElementMergeThis, BOOL fOMCall = TRUE, BOOL fCopyID = FALSE);

    virtual HRESULT ComputeFormats(CFormatInfo * pCFI, CTreeNode * pNodeTarget);
    virtual HRESULT ApplyDefaultFormat (CFormatInfo * pCFI);
    BOOL    ElementNeedsLayout(CFormatInfo * pCFI);
    BOOL    DetermineBlockness(CFormatInfo * pCFI);
    HRESULT ApplyInnerOuterFormats(CFormatInfo * pCFI);
    CImgCtx *GetBgImgCtx();

    // Access Key Handling Helper Functions

    FOCUS_ITEM  GetMnemonicTarget();
    HRESULT HandleMnemonic(CMessage * pmsg, BOOL fDoClick,  BOOL * pfYieldFailed = NULL);
    HRESULT GotMnemonic(CMessage * pMessage);
    HRESULT LostMnemonic(CMessage * pMessage);  
    BOOL    MatchAccessKey(CMessage * pmsg);
    HRESULT OnTabIndexChange();
    
    //
    // Styles
    //
    HRESULT GetStyleObject(CStyle **ppStyle);
    CAttrArray *  GetInLineStyleAttrArray ();
#ifdef VSTUDIO7
    void FlushPeerProperties ();
#endif
    CAttrArray ** CreateStyleAttrArray ( DISPID );

    BOOL HasInLineStyles(void);
    BOOL HasClassOrID(void);

    CStyle * GetInLineStylePtr();
    CStyle * GetRuntimeStylePtr();
    CFilterArray *GetFilterCollectionPtr ();

    // recalc expression support (overrides from CBase)
    STDMETHOD(removeExpression)(BSTR bstrPropertyName, VARIANT_BOOL *pfSuccess);
    STDMETHOD(setExpression)(BSTR strPropertyName, BSTR strExpression, BSTR strLanguage);
    STDMETHOD(getExpression)(BSTR strPropertyName, VARIANT *pvExpression);

    // Helpers for abstract name properties implemented on derived elements
    DECLARE_TEAROFF_METHOD(put_name , PUT_name ,  (BSTR v));
    DECLARE_TEAROFF_METHOD(get_name , GET_name ,  (BSTR * p));

    htmlBlockAlign      GetParagraphAlign ( BOOL fOuter );
    htmlControlAlign    GetSiteAlign();

    BOOL IsInlinedElement ( );

    BOOL IsPositionStatic ( void );
    BOOL IsPositioned ( void );
    BOOL IsAbsolute ( void );
    BOOL IsRelative ( void );
    BOOL IsInheritingRelativeness ( void );
    BOOL IsScrollingParent ( void );
    BOOL IsClipParent ( void );
    BOOL IsZParent ( void );
    BOOL IsLocked();
    BOOL IsDisplayNone();
    BOOL HasPageBreakBefore();
    BOOL HasPageBreakAfter();
    BOOL IsVisibilityHidden ();

    //
    // Reset functionallity
    // Returns S_OK if successful and E_NOTIMPL if not applicable
    virtual HRESULT DoReset(void) { return E_NOTIMPL; }

    // Submit -- used to get the pieces for constructing the submit string
    // returns S_FALSE for elemetns that do not participate in the submit string
    virtual HRESULT GetSubmitInfo(CPostData * pSubmitData) { return S_FALSE; }

    // Get control's window, does control have window?
    virtual HWND GetHwnd() { return NULL; }
    //

    // Take the capture.
    //
    void TakeCapture(BOOL fTake);

    BOOL HasCapture();

    // IMsoCommandTarget methods

    HRESULT STDMETHODCALLTYPE QueryStatus(
        GUID * pguidCmdGroup,
        ULONG cCmds,
        MSOCMD rgCmds[],
        MSOCMDTEXT * pcmdtext);
    HRESULT STDMETHODCALLTYPE Exec(
        GUID * pguidCmdGroup,
        DWORD nCmdID,
        DWORD nCmdexecopt,
        VARIANTARG * pvarargIn,
        VARIANTARG * pvarargOut);

    BOOL    IsEditable(BOOL fCheckContainerOnly = FALSE);
    BOOL    IsEditableSlow();

    HRESULT EnsureInMarkup();

    // Currency / UI-Activity
    //

    // Does this element (or its master) have currency?
    BOOL HasCurrency();

    virtual HRESULT RequestYieldCurrency(BOOL fForce);

    // Relinquishes currency
    virtual HRESULT YieldCurrency(CElement *pElemNew);

    // Relinquishes UI
    virtual void YieldUI(CElement *pElemNew);

    // Forces UI upon an element
    virtual HRESULT BecomeUIActive();

    BOOL NoUIActivate(); // tell us if element can be UIActivate

    BOOL            IsFocussable(long lSubDivision);
    BOOL            IsTabbable(long lSubDivision);

    HRESULT PreBecomeCurrent(long lSubDivision, CMessage *pMessage);
    HRESULT BecomeCurrentFailed(long lSubDivision, CMessage *pMessage);
    HRESULT PostBecomeCurrent(CMessage *pMessage);

    HRESULT BecomeCurrent(
                    long lSubDivision,
                    BOOL *pfYieldFailed = NULL,
                    CMessage *pMessage = NULL,
                    BOOL fTakeFocus = FALSE);

    HRESULT BubbleBecomeCurrent(
                    long lSubDivision,
                    BOOL *pfYieldFailed = NULL,
                    CMessage *pMessage = NULL,
                    BOOL fTakeFocus = FALSE);

    CElement *GetFocusBlurFireTarget(long lSubDiv);

    HRESULT focusHelper(long lSubDivision);

    virtual HRESULT GetFocusShape(long lSubDivision, CDocInfo * pdci, CShape ** ppShape);

    // Forces Currency and uiactivity upon an element
    HRESULT BecomeCurrentAndActive(CMessage * pmsg = NULL, long lSubDivision = 0, BOOL fTakeFocus = FALSE, 
                                    BOOL * pfYieldFailed = NULL);
    HRESULT BubbleBecomeCurrentAndActive(CMessage * pmsg = NULL, BOOL fTakeFocus = FALSE);

    virtual HRESULT GetSubDivisionCount(long *pc);
    virtual HRESULT GetSubDivisionTabs(long *pTabs, long c);
    virtual HRESULT SubDivisionFromPt(POINT pt, long *plSub);

    // Find an element with the given set of SITE_FLAG's set
    CElement * FindDefaultElem(BOOL fDefault, BOOL fFull = FALSE);

    // set the default element in a form or in the doc
    void    SetDefaultElem(BOOL fFindNew = FALSE);

    HRESULT GetNextSubdivision(FOCUS_DIRECTION dir, long lSubDivision, long *plSubNew);

    // Public Helper Methods
    //
    //
    CFormElement *  GetParentForm();
    CLabelElement * GetLabel() const;


    virtual BOOL IsEnabled();

    virtual BOOL IsValid() { return TRUE; }

            BOOL    IsVisible  ( BOOL fCheckParent );
            BOOL    IsParent(CElement *pElement);       // Is pElement a parent of this element?

    virtual HRESULT GetControlInfo(CONTROLINFO * pCI) { return E_FAIL; }
    virtual BOOL    OnMenuEvent(int id, UINT code)    { return FALSE; }
    HRESULT BUGCALL OnCommand(int id, HWND hwndCtrl, UINT codeNotify)
            { return S_OK; }
    HRESULT OnContextMenu(int x, int y, int id);
#ifndef NO_MENU
    HRESULT OnInitMenuPopup(HMENU hmenu, int item, BOOL fSystemMenu);
    HRESULT OnMenuSelect(UINT uItem, UINT fuFlags, HMENU hmenu);
#endif // NO_MENU

    // Helper for translating keystrokes into commands
    //
    HRESULT PerformTA(CMessage *pMessage);

    DWORD GetCommandID(LPMSG lpmsg);

    CImgCtx * GetNearestBgImgCtx();

    // Helper for parsing filter name=value pairs and storing in a property bag
    HRESULT            ParseFilterNameValuePair(LPTSTR pchNameValue, CPropertyBag **ppPropBag);

    //+---------------------------------------------------------------------------
    //
    //  Flag values for CElement::CLock
    //
    //----------------------------------------------------------------------------

    enum ELEMENTLOCK_FLAG
    {
        ELEMENTLOCK_NONE            = 0,
        ELEMENTLOCK_CLICK           = 1 <<  0,
        ELEMENTLOCK_PROCESSPOSITION = 1 <<  1,
        ELEMENTLOCK_PROCESSADORNERS = 1 <<  2,
        ELEMENTLOCK_DELETE          = 1 <<  3,
        ELEMENTLOCK_FOCUS           = 1 <<  4,
        ELEMENTLOCK_CHANGE          = 1 <<  5,
        ELEMENTLOCK_UPDATE          = 1 <<  6,
        ELEMENTLOCK_SIZING          = 1 <<  7,
        ELEMENTLOCK_COMPUTEFORMATS  = 1 <<  8,
        ELEMENTLOCK_QUERYSTATUS     = 1 <<  9,
        ELEMENTLOCK_BLUR            = 1 << 10,
        ELEMENTLOCK_RECALC          = 1 << 11,
        ELEMENTLOCK_BLOCKCALC       = 1 << 12,
        ELEMENTLOCK_ATTACHPEER      = 1 << 13,
        ELEMENTLOCK_PROCESSREQUESTS = 1 << 14,
        ELEMENTLOCK_PROCESSMEASURE  = 1 << 15,
        ELEMENTLOCK_LAST            = 1 << 15,

        // don't add anymore flags, we only have 16 bits
    };




    //+-----------------------------------------------------------------------
    //
    //  CElement::CLock
    //
    //------------------------------------------------------------------------

    class CLock
    {
    public:
        DECLARE_MEMALLOC_NEW_DELETE(Mt(CElementCLock))
        CLock(CElement *pElement, ELEMENTLOCK_FLAG enumLockFlags = ELEMENTLOCK_NONE);
        ~CLock();

    private:
        CElement *  _pElement;
        WORD        _wLockFlags;
    };

    BOOL    TestLock(ELEMENTLOCK_FLAG enumLockFlags)
                { return _wLockFlags & ((WORD)enumLockFlags); }

    BOOL    TestClassFlag(ELEMENTDESC_FLAG dwFlag) const
                { return ElementDesc()->_classdescBase._dwFlags & dwFlag; }

    inline BOOL WantEndParseNotification()
                { return TestClassFlag(CElement::ELEMENTDESC_NOTIFYENDPARSE) || HasPeerHolder(); }

    // BUGBUG: we should have a general notification mechanism to tell what
    // elements are listening to which notifications
    BOOL WantTextChangeNotifications();

#ifndef NO_EDIT
    //
    // Undo support.
    //

    HRESULT CreateUndoAttrValueSimpleChange( DISPID             dispid,
                                             VARIANT &          varProp,
                                             BOOL               fInlineStyle,
                                             CAttrValue::AATYPE aaType );

    HRESULT CreateUndoPropChangeNotification( DISPID dispid,
                                              DWORD  dwFlags,
                                              BOOL   fPlaceHolder );

    // Identifies block tags that have their own character formats.
    // This is used to control where we spring load fonts to insert
    // formatting and where we just leave the spring loading to happen.
    inline BOOL IsCharFormatBlockTag(ELEMENT_TAG etag)
    {
        // If we've just cleared the formatting,
        // skip over any block elements that have some
        // character formatting associated with them.
        return (etag >= ETAG_H1 && etag <= ETAG_H6 ||
                etag == ETAG_PRE || etag == ETAG_BLOCKQUOTE);
    }
    inline BOOL IsCharFormatBlock() {return IsCharFormatBlockTag(Tag());}

#endif // NO_EDIT

    //+-----------------------------------------------------------------------
    //
    //  CLASSDESC (class descriptor)
    //
    //------------------------------------------------------------------------

    class ACCELS
    {
        public:

        ACCELS (ACCELS * pSuper, WORD wAccels);
        ~ACCELS ();
        HRESULT EnsureResources();
        HRESULT LoadAccelTable ();
        DWORD   GetCommandID (LPMSG pmsg);

        ACCELS *    _pSuper;

        BOOL        _fResourcesLoaded;

        WORD        _wAccels;
        LPACCEL     _pAccels;
        int         _cAccels;
    };

    struct CLASSDESC
    {
        CBase::CLASSDESC _classdescBase;
        void *_apfnTearOff;

        BOOL TestFlag(ELEMENTDESC_FLAG dw) const { return (_classdescBase._dwFlags & dw) != 0; }

        // move from CSite::CLASSDESC
        //
        ACCELS *            _pAccelsDesign;
        ACCELS *            _pAccelsRun;
    };

    const CLASSDESC * ElementDesc() const
    {
        return (const CLASSDESC *) BaseDesc();
    }

public:
    //
    // Lookaside pointers
    //

    enum
    {
        LOOKASIDE_FILTER            = 0,
        LOOKASIDE_DATABIND          = 1,
        LOOKASIDE_PEER              = 2,
        LOOKASIDE_PEERMGR           = 3,
        LOOKASIDE_ACCESSIBLE        = 4,
        LOOKASIDE_SLAVEMARKUP       = 5,
        LOOKASIDE_REQUEST           = 6,
        LOOKASIDE_ELEMENT_NUMBER    = 7

        // *** There are only 7 bits reserved in the element.
        // *** if you add more lookasides you have to make sure 
        // *** that you make room for those bits.
    };

private:
    BOOL            HasLookasidePtr(int iPtr)                   { return(_fHasLookasidePtr & (1 << iPtr)); }
    void *          GetLookasidePtr(int iPtr);
    HRESULT         SetLookasidePtr(int iPtr, void * pv);
    void *          DelLookasidePtr(int iPtr);

public:

    BOOL            HasLayoutPtr() const                        { return _fHasLayoutPtr; }
    CLayout *       GetLayoutPtr() const;
    void            SetLayoutPtr( CLayout * pLayout );
    CLayout *       DelLayoutPtr();

    BOOL            IsInMarkup() const                          { return _fHasMarkupPtr; }
    BOOL            HasMarkupPtr() const                        { return _fHasMarkupPtr; }
    CMarkup *       GetMarkupPtr() const;
    void            SetMarkupPtr( CMarkup * pMarkup );
    void            DelMarkupPtr();

    CRootElement *  IsRoot()                                    { return Tag() == ETAG_ROOT ? (CRootElement*)this : NULL; }
    CMarkup *       GetMarkup() const                           { return GetMarkupPtr(); }
    BOOL            IsInPrimaryMarkup ( ) const;
    BOOL            IsInThisMarkup ( CMarkup* pMarkup ) const;
    CRootElement *  MarkupRoot();

    CElement *      MarkupMaster() const;
    CMarkup *       SlaveMarkup() const                           { return ((CElement *)this)->GetSlaveMarkupPtr(); }
    CElement *      FireEventWith();

    CDoc *          GetDocPtr() const;

    BOOL            HasSlaveMarkupPtr()                           { return(HasLookasidePtr(LOOKASIDE_SLAVEMARKUP)); }
    CMarkup *       GetSlaveMarkupPtr()                           { return((CMarkup *)GetLookasidePtr(LOOKASIDE_SLAVEMARKUP)); }
    HRESULT         SetSlaveMarkupPtr(CMarkup * pSlaveMarkup)     { return(SetLookasidePtr(LOOKASIDE_SLAVEMARKUP, pSlaveMarkup)); }
    CMarkup *       DelSlaveMarkupPtr()                           { return((CMarkup *)DelLookasidePtr(LOOKASIDE_SLAVEMARKUP)); }

    BOOL            HasRequestPtr()                             { return(HasLookasidePtr(LOOKASIDE_REQUEST)); }
    CRequest *      GetRequestPtr()                             { return((CRequest *)GetLookasidePtr(LOOKASIDE_REQUEST)); }
    HRESULT         SetRequestPtr( CRequest * pRequest )        { return(SetLookasidePtr(LOOKASIDE_REQUEST, pRequest)); }
    CRequest *      DelRequestPtr()                             { return((CRequest *)DelLookasidePtr(LOOKASIDE_REQUEST)); }

    BOOL            HasFilterPtr()                              { return(HasLookasidePtr(LOOKASIDE_FILTER)); }
    CFilter *       GetFilterPtr()                              { return((CFilter *)GetLookasidePtr(LOOKASIDE_FILTER)); }
    HRESULT         SetFilterPtr(CFilter * pFilter)             { return(SetLookasidePtr(LOOKASIDE_FILTER, pFilter)); }
    CFilter *       DelFilterPtr()                              { return((CFilter *)DelLookasidePtr(LOOKASIDE_FILTER)); }

    BOOL            HasDataBindPtr()                            { return(HasLookasidePtr(LOOKASIDE_DATABIND)); }
    DBMEMBERS *     GetDataBindPtr()                            { return((DBMEMBERS *)GetLookasidePtr(LOOKASIDE_DATABIND)); }
    HRESULT         SetDataBindPtr(DBMEMBERS * pDBMembers)      { return(SetLookasidePtr(LOOKASIDE_DATABIND, pDBMembers));  }
    DBMEMBERS *     DelDataBindPtr()                            { return((DBMEMBERS *)DelLookasidePtr(LOOKASIDE_DATABIND)); }

    BOOL            HasPeerHolder()                             { return(HasLookasidePtr(LOOKASIDE_PEER)); }
    CPeerHolder *   GetPeerHolder()                             { return((CPeerHolder *)GetLookasidePtr(LOOKASIDE_PEER)); }
    HRESULT         SetPeerHolder(CPeerHolder * pPeerHolder)    { return(SetLookasidePtr(LOOKASIDE_PEER, pPeerHolder)); }
    CPeerHolder *   DelPeerHolder()                             { return((CPeerHolder *)DelLookasidePtr(LOOKASIDE_PEER)); }

    BOOL            HasPeerMgr()                                { return(HasLookasidePtr(LOOKASIDE_PEERMGR)); }
    CPeerMgr *      GetPeerMgr()                                { return((CPeerMgr *)GetLookasidePtr(LOOKASIDE_PEERMGR)); }
    HRESULT         SetPeerMgr(CPeerMgr * pPeerMgr)             { return(SetLookasidePtr(LOOKASIDE_PEERMGR, pPeerMgr)); }
    CPeerMgr *      DelPeerMgr()                                { return((CPeerMgr*)DelLookasidePtr(LOOKASIDE_PEERMGR)); }

    void            IncPeerDownloads();
    void            DecPeerDownloads();

    BOOL            HasPeerWithUrn(LPCTSTR Urn);
    HRESULT         ApplyBehaviorCss(CBehaviorInfo * pInfo);


    HRESULT         PutUrnAtom(LONG atom);
    HRESULT         GetUrnAtom(LONG * pAtom);
    HRESULT         GetUrn(LPTSTR * ppchUrn);

    BOOL            HasAccObjPtr()                              { return(HasLookasidePtr(LOOKASIDE_ACCESSIBLE)); }
    CAccElement *   GetAccObjPtr()                              { return((CAccElement *)GetLookasidePtr(LOOKASIDE_ACCESSIBLE)); }
    HRESULT         SetAccObjPtr(CAccElement * pAccElem)        { return(SetLookasidePtr(LOOKASIDE_ACCESSIBLE, pAccElem)); }
    CAccElement *   DelAccObjPtr()                              { return((CAccElement *)DelLookasidePtr(LOOKASIDE_ACCESSIBLE)); }
    CAccElement *   CreateAccObj();

    long GetReadyState();
    virtual void OnReadyStateChange();

    // **************************************
    //
    // MEMBER DATA
    //
    // **************************************

#if DBG==1 || defined(DUMPTREE) || defined(OBJCNTCHK)
private:
    ELEMENT_TAG             _etagDbg;
public:
    int                     _nSerialNumber;
    DWORD                   _dwObjCnt;
    DWORD                   _dwPad1;
    #define CELEMENT_DBG1_SIZE   (sizeof(ELEMENT_TAG) + sizeof(int) + (2*sizeof(DWORD)))
#else
    #define CELEMENT_DBG1_SIZE   (0)
#endif

#if DBG==1
private:
    CDoc *                  _pDocDbg;
    CMarkup *               _pMarkupDbg;
    CLayout *               _pLayoutDbg;
    union
    {
        void *              _apLookAside[LOOKASIDE_ELEMENT_NUMBER];
        struct
        {
            CFilter *       _pFilterDbg;
            DBMEMBERS *     _pDBMembersDbg;
            CPeerHolder *   _pPeerHolderDbg;
            CPeerMgr *      _pPeerMgrDbg;
            CAccElement *   _pAccElementDbg;
            CMarkup *       _pSlaveMarkupDbg;
            CRequest *      _pRequestDbg;
        };
    };

public:
    DWORD                   _fPassivatePending : 1;     // Debug bit to be used with EXITTREE_PASSIVATEPENDING
    DWORD                   _fDelayRelease     : 1;     // Debug bit to be used with EXITTREE_DELAYRELEASENEEDED
    DWORD                   _dwPad2;
    
    #define CELEMENT_DBG2_SIZE (10*sizeof(void *) + 2*sizeof(DWORD))
#else
    #define CELEMENT_DBG2_SIZE (0)
#endif

    //
    // The element is the head of a linked list of important structures.  If the element has layout,
    // then the __pvChain member points to that layout.  If not and if the element is in a tree then
    // then the __pvChain member points to the ped that it is in.  Otherwise __pvChain points to the
    // document.
    //

private:

    void *                  __pvChain;

public:

    CTreeNode *             __pNodeFirstBranch;

    // First DWORD of bits

    ELEMENT_TAG _etag                    : 8; //  0- 7 element tag
    unsigned _fHasLookasidePtr           : 7; //  8-14 TRUE if lookaside table has pointer
    unsigned _fIsNamed                   : 1; // 15 set if element has a name or ID attribute
    unsigned _wLockFlags                 :16; // 16-31 Element lock flags for preventing recursion

    //
    // Second DWORD of bits
    //
    // Note that the _fMark1 and _fMark2 bits are only safe to use in routines which can *guarantee*
    // their processing will not be interrupted. If there is a chance that a message loop can cause other,
    // unrelated code, to execute, these bits could get reset before the original caller is finished.
    //

    unsigned _fHasMarkupPtr              : 1; //  0 TRUE if element has a Markup pointer
    unsigned _fHasLayoutPtr              : 1; //  1 TRUE if element has layout ptr
    unsigned _fHasPendingFilterTask      : 1; //  2 TRUE if there is a pending filter task (see EnsureView)
    unsigned _fHasPendingRecalcTask      : 1; //  3 TRUE if there is a pending recalc task (see EnsureView)
    unsigned _fLayoutAlwaysValid         : 1; //  4 TRUE if element is a site or never has layout
    unsigned _fOwnsRuns                  : 1; //  5 TRUE if element owns the runs underneath it
    unsigned _fInheritFF                 : 1; //  6 TRUE if element to inherit site and fancy format
    unsigned _fBreakOnEmpty              : 1; //  7 this element breaks a line even is it own no text
    unsigned _fUnused2                   : 4; //  8-11
    unsigned _fDefinitelyNoBorders       : 1; // 12 There are no borders on this element
    unsigned _fHasTabIndex               : 1; // 13 Has a tabindex associated with this element. Look in doc _aryTabIndexInfo
    unsigned _fHasImage                  : 1; // 14 has at least one image context
    unsigned _fResizeOnImageChange       : 1; // 15 need to force resize when image changes
    unsigned _fExplicitEndTag            : 1; // 16 element had a close tag (for P)
    unsigned _fSynthesized               : 1; // 17 FALSE (default) if user created, TRUE if synthesized
    unsigned _fUnused3                   : 1; // 18 Unused bit
    unsigned _fActsLikeButton            : 1; // 19 does element act like a push button?
    unsigned _fEditAtBrowse              : 1; // 20 to TestClassFlag(SITEDESC_EDITATBROWSE) in init
    unsigned _fSite                      : 1; // 21 element with layout by default
    unsigned _fDefault                   : 1; // 22 Is this the default "ok" button/control
    unsigned _fCancel                    : 1; // 23 Is this the default "cancel" button/control
    unsigned _fHasPendingEvent           : 1; // 24 A posted event for element is pending
    unsigned _fEventListenerPresent      : 1; // 25 Someone has asked for a connection point/ or set an event handler
    unsigned _fHasFilterCollectionPtr    : 1; // 26 FilterCollectionPtr has been added to _pAA
    unsigned _fExittreePending           : 1; // 27 There is a pending Exittree notification for this element
    unsigned _fFirstCommonAncestor       : 1; // 28 Used in GetFirstCommonAncestor - don't touch!
    unsigned _fMark1                     : 1; // 29 Random mark
    unsigned _fMark2                     : 1; // 30 Another random mark
    unsigned _fHasStyleExpressions       : 1; // 31 There are style expressions on this element

    //
    // STATIC DATA
    //

    //  Default property page list for elements that don't have their own.
    //  This gives them the allpage by default.

    static const CLSID * s_apclsidPages[];
    static ACCELS s_AccelsElementDesign;
    static ACCELS s_AccelsElementRun;

    // Style methods

    #include "style.hdl"

    // IHTMLElement methods

    #define _CElement_
    #include "element.hdl"

    DECLARE_TEAROFF_TABLE(IServiceProvider)
    DECLARE_TEAROFF_TABLE(IProvideMultipleClassInfo)
    DECLARE_TEAROFF_TABLE(IRecalcProperty)

private:

    NO_COPY(CElement);

};

//
// If the compiler barfs on the next statement, it means that the size of the CElement structure
// has grown beyond allowable limits.  You cannot increase the size of this structure without
// approval from the Trident development manager.
//

COMPILE_TIME_ASSERT(CElement, CBASE_SIZE + (2*sizeof(void*)) + (2*sizeof(DWORD)) + CELEMENT_DBG1_SIZE + CELEMENT_DBG2_SIZE);

BOOL SameScope ( CTreeNode * pNode1, const CElement * pElement2 );
BOOL SameScope ( const CElement * pElement1, CTreeNode * pNode2 );
BOOL SameScope ( CTreeNode * pNode1, CTreeNode * pNode2 );

inline BOOL DifferentScope ( CTreeNode * pNode1, const CElement * pElement2 )
{
    return ! SameScope( pNode1, pElement2 );
}
inline BOOL DifferentScope ( const CElement * pElement1, CTreeNode * pNode2 )
{
    return ! SameScope( pElement1, pNode2 );
}
inline BOOL DifferentScope ( CTreeNode * pNode1, CTreeNode * pNode2 )
{
    return ! SameScope( pNode1, pNode2 );
}

//+---------------------------------------------------------------------------
//
// Inline CTreePos members
//
//----------------------------------------------------------------------------

inline BOOL
CTreePos::IsBeginElementScope(CElement *pElem)
{
    return IsBeginElementScope() && Branch()->Element() == pElem;
}

inline BOOL
CTreePos::IsEndElementScope(CElement *pElem)
{
    return IsEndElementScope() && Branch()->Element() == pElem;
}

inline CTreeDataPos *
CTreePos::DataThis()
{
    Assert( IsDataPos() );
    return (CTreeDataPos *) this;
}

inline const CTreeDataPos *
CTreePos::DataThis() const
{
    Assert( IsDataPos() );
    return (const CTreeDataPos *) this;
}

inline CTreeNode *
CTreePos::Branch() const
{
    AssertSz( !IsDataPos() && IsNode(), "CTreePos::Branch called on non-node pos" );
    return(CONTAINING_RECORD((this + !!TestFlag(NodeBeg)), CTreeNode, _tpEnd));
}

inline CMarkupPointer *
CTreePos::MarkupPointer() const
{
    Assert( IsPointer() && IsDataPos() );
    return (CMarkupPointer *) (DataThis()->p._dwPointerAndGravityAndCling & ~3);
}

inline void
CTreePos::SetMarkupPointer ( CMarkupPointer * pmp )
{
    Assert( IsPointer() && IsDataPos() );
    Assert( (DWORD_PTR( pmp ) & 0x1) == 0 );
    DataThis()->p._dwPointerAndGravityAndCling = (DataThis()->p._dwPointerAndGravityAndCling & 1) | (DWORD_PTR)(pmp);
}

inline long
CTreePos::Cch() const
{
    Assert( IsText() && IsDataPos() );
    return DataThis()->t._cch;
}

inline long
CTreePos::Sid() const
{
    Assert( IsText() && IsDataPos() );
    return DataThis()->t._sid;
}

inline long
CTreePos::TextID() const
{
    Assert( IsText() && IsDataPos() );
    return HasTextID() ? DataThis()->t._lTextID : 0;
}

#if DBG == 1 || defined(DUMPTREE)
inline void
CTreePos::SetSN()
{
    _nSerialNumber = s_NextSerialNumber++;
}
#endif

//+---------------------------------------------------------------------------
//
// Inline CTreeNode members
//
//----------------------------------------------------------------------------

inline HRESULT
CTreeNode::GetElementInterface (
    REFIID riid, void * * ppVoid )
{
    CElement * pElement = Element();

    Assert( pElement );

    if (pElement->GetFirstBranch() == this)
        return pElement->QueryInterface( riid, ppVoid );
    else
        return GetInterface( riid, ppVoid );
}

inline CRootElement *
CTreeNode::IsRoot()
{
    Assert( !IsDead() );
    return Element()->IsRoot();
}

inline CMarkup *
CTreeNode::GetMarkup()
{
    Assert( !IsDead() );
    return Element()->GetMarkup();
}

inline CRootElement *
CTreeNode::MarkupRoot()
{
    Assert( !IsDead() );
    return Element()->MarkupRoot();
}

inline void
CTreeNode::PrivateEnterTree()
{
    Assert( !_fInMarkup );
    _fInMarkup = TRUE;
}

inline void
CTreeNode::PrivateExitTree()
{
    PrivateMakeDead();
    PrivateMarkupRelease();
}

inline void    
CTreeNode::PrivateMakeDead()
{
    Assert( _fInMarkup );
    _fInMarkup = FALSE;

    VoidCachedNodeInfo();

    SetParent(NULL);
}

inline void    
CTreeNode::PrivateMarkupRelease()
{
    if (!HasPrimaryTearoff())
    {
        WHEN_DBG( _fInDestructor = TRUE );
        delete this;
    }
}

inline CColorValue
CTreeNode::GetCascadedcolor()
{
    return (CColorValue) CTreeNode::GetCharFormat()->_ccvTextColor;
}

inline CUnitValue
CTreeNode::GetCascadedletterSpacing()
{
    return (CUnitValue) CTreeNode::GetCharFormat()->_cuvLetterSpacing;
}

inline styleTextTransform
CTreeNode::GetCascadedtextTransform()
{
    return (styleTextTransform) CTreeNode::GetCharFormat()->_bTextTransform;
}

inline CUnitValue
CTreeNode::GetCascadedpaddingTop()
{
    return (CUnitValue) CTreeNode::GetFancyFormat()->_cuvPaddingTop;
}

inline CUnitValue
CTreeNode::GetCascadedpaddingRight()
{
    return (CUnitValue) CTreeNode::GetFancyFormat()->_cuvPaddingRight;
}

inline CUnitValue
CTreeNode::GetCascadedpaddingBottom()
{
    return (CUnitValue) CTreeNode::GetFancyFormat()->_cuvPaddingBottom;
}

inline CUnitValue
CTreeNode::GetCascadedpaddingLeft()
{
    return (CUnitValue) CTreeNode::GetFancyFormat()->_cuvPaddingLeft;
}

inline CColorValue
CTreeNode::GetCascadedborderTopColor()
{
    return (CColorValue) CTreeNode::GetFancyFormat()->_ccvBorderColors[BORDER_TOP];
}

inline CColorValue
CTreeNode::GetCascadedborderRightColor()
{
    return (CColorValue) CTreeNode::GetFancyFormat()->_ccvBorderColors[BORDER_RIGHT];
}

inline CColorValue
CTreeNode::GetCascadedborderBottomColor()
{
    return (CColorValue) CTreeNode::GetFancyFormat()->_ccvBorderColors[BORDER_BOTTOM];
}

inline CColorValue
CTreeNode::GetCascadedborderLeftColor()
{
    return (CColorValue) CTreeNode::GetFancyFormat()->_ccvBorderColors[BORDER_LEFT];
}



inline CUnitValue
CTreeNode::GetCascadedborderTopWidth()
{
    return (CUnitValue) CTreeNode::GetFancyFormat()->_cuvBorderWidths[BORDER_TOP];
}

inline CUnitValue
CTreeNode::GetCascadedborderRightWidth()
{
    return (CUnitValue) CTreeNode::GetFancyFormat()->_cuvBorderWidths[BORDER_RIGHT];
}

inline CUnitValue
CTreeNode::GetCascadedborderBottomWidth()
{
    return (CUnitValue) CTreeNode::GetFancyFormat()->_cuvBorderWidths[BORDER_BOTTOM];
}

inline CUnitValue
CTreeNode::GetCascadedborderLeftWidth()
{
    return (CUnitValue) CTreeNode::GetFancyFormat()->_cuvBorderWidths[BORDER_LEFT];
}

inline CUnitValue
CTreeNode::GetCascadedwidth()
{
    return (CUnitValue) CTreeNode::GetFancyFormat()->_cuvWidth;
}

inline CUnitValue
CTreeNode::GetCascadedheight()
{
    return (CUnitValue) CTreeNode::GetFancyFormat()->_cuvHeight;
}

inline CUnitValue
CTreeNode::GetCascadedtop()
{
    return (CUnitValue) CTreeNode::GetFancyFormat()->_cuvTop;
}

inline CUnitValue
CTreeNode::GetCascadedbottom()
{
    return (CUnitValue) CTreeNode::GetFancyFormat()->_cuvBottom;
}

inline CUnitValue
CTreeNode::GetCascadedleft()
{
    return (CUnitValue) CTreeNode::GetFancyFormat()->_cuvLeft;
}

inline CUnitValue
CTreeNode::GetCascadedright()
{
    return (CUnitValue) CTreeNode::GetFancyFormat()->_cuvRight;
}

inline styleOverflow
CTreeNode::GetCascadedoverflowX()
{
    return (styleOverflow)CTreeNode::GetFancyFormat()->_bOverflowX;
}

inline styleOverflow
CTreeNode::GetCascadedoverflowY()
{
    return (styleOverflow)CTreeNode::GetFancyFormat()->_bOverflowY;
}

inline styleOverflow
CTreeNode::GetCascadedoverflow()
{
    const CFancyFormat * pFF = CTreeNode::GetFancyFormat();
    return (styleOverflow)(pFF->_bOverflowX > pFF->_bOverflowY
                                ? pFF->_bOverflowX
                                : pFF->_bOverflowY);
}

inline styleStyleFloat
CTreeNode::GetCascadedfloat()
{
    return (styleStyleFloat)CTreeNode::GetFancyFormat()->_bStyleFloat;
}

inline stylePosition
CTreeNode::GetCascadedposition()
{
    return (stylePosition) CTreeNode::GetFancyFormat()->_bPositionType;
}

inline long
CTreeNode::GetCascadedzIndex()
{
    return (long) CTreeNode::GetFancyFormat()->_lZIndex;
}

inline CUnitValue
CTreeNode::GetCascadedclipTop()
{
    return CTreeNode::GetFancyFormat()->_cuvClipTop;
}

inline CUnitValue
CTreeNode::GetCascadedclipLeft()
{
    return CTreeNode::GetFancyFormat()->_cuvClipLeft;
}

inline CUnitValue
CTreeNode::GetCascadedclipRight()
{
    return CTreeNode::GetFancyFormat()->_cuvClipRight;
}

inline CUnitValue
CTreeNode::GetCascadedclipBottom()
{
    return CTreeNode::GetFancyFormat()->_cuvClipBottom;
}

inline BOOL
CTreeNode::GetCascadedtableLayout()
{
    return (BOOL) CTreeNode::GetFancyFormat()->_bTableLayout;
}

inline BOOL
CTreeNode::GetCascadedborderCollapse()
{
    return (BOOL) CTreeNode::GetFancyFormat()->_bBorderCollapse;
}

inline BOOL
CTreeNode::GetCascadedborderOverride()
{
    return (BOOL) CTreeNode::GetFancyFormat()->_fOverrideTablewideBorderDefault;
}

inline WORD
CTreeNode::GetCascadedfontWeight()
{
    return GetCharFormat()->_wWeight;
}


inline WORD
CTreeNode::GetCascadedfontHeight()
{
    return  GetCharFormat()->_yHeight;
}

inline CUnitValue
CTreeNode::GetCascadedbackgroundPositionX()
{
    return  CTreeNode::GetFancyFormat()->_cuvBgPosX;
}

inline CUnitValue
CTreeNode::GetCascadedbackgroundPositionY()
{
    return  CTreeNode::GetFancyFormat()->_cuvBgPosY;
}

inline BOOL
CTreeNode::GetCascadedbackgroundRepeatX()
{
    return  (BOOL)CTreeNode::GetFancyFormat()->_fBgRepeatX;
}

inline BOOL
CTreeNode::GetCascadedbackgroundRepeatY()
{
    return  (BOOL)CTreeNode::GetFancyFormat()->_fBgRepeatY;
}

inline htmlBlockAlign
CTreeNode::GetCascadedblockAlign()
{
    return (htmlBlockAlign)CTreeNode::GetParaFormat()->_bBlockAlignInner;
}

inline styleVisibility
CTreeNode::GetCascadedvisibility()
{
    return  (styleVisibility)GetFancyFormat()->_bVisibility;
}

inline styleDisplay
CTreeNode::GetCascadeddisplay()
{
    return  (styleDisplay)GetFancyFormat()->_bDisplay;
}

inline BOOL
CTreeNode::GetCascadedunderline()
{
    return  (BOOL)GetCharFormat()->_fUnderline;
}

inline styleAccelerator
CTreeNode::GetCascadedaccelerator()
{
    return (styleAccelerator) GetCharFormat()->_fAccelerator;
}

inline BOOL
CTreeNode::GetCascadedoverline()
{
    return  (BOOL)GetCharFormat()->_fOverline;
}

inline BOOL
CTreeNode::GetCascadedstrikeOut()
{
    return  (BOOL)GetCharFormat()->_fStrikeOut;
}

inline CUnitValue
CTreeNode::GetCascadedlineHeight()
{
    return  GetCharFormat()->_cuvLineHeight;
}

inline CUnitValue
CTreeNode::GetCascadedtextIndent()
{
    return  CTreeNode::GetParaFormat()->_cuvTextIndent;
}

inline BOOL
CTreeNode::GetCascadedsubscript()
{
    return  (BOOL)GetCharFormat()->_fSubscript;
}

inline BOOL
CTreeNode::GetCascadedsuperscript()
{
    return  (BOOL)GetCharFormat()->_fSuperscript;
}

inline BOOL
CTreeNode::GetCascadedbackgroundAttachmentFixed()
{
    return  (BOOL)CTreeNode::GetFancyFormat()->_fBgFixed;
}

inline styleListStyleType
CTreeNode::GetCascadedlistStyleType()
{
    styleListStyleType slt;

    slt = (styleListStyleType)CTreeNode::GetFancyFormat()->_ListType;
    if(slt == styleListStyleTypeNotSet)
        slt = GetParaFormat()->_cListing.GetStyle();
    return slt;
}

inline styleListStylePosition
CTreeNode::GetCascadedlistStylePosition()
{
    return (styleListStylePosition)GetParaFormat()->_bListPosition;
}

inline long
CTreeNode::GetCascadedlistImageCookie()
{
    return GetParaFormat()->_lImgCookie;
}

inline stylePageBreak
CTreeNode::GetCascadedpageBreakBefore()
{
    return  (stylePageBreak)((CTreeNode::GetFancyFormat()->_bPageBreaks) & 0x0f);
}

inline stylePageBreak
CTreeNode::GetCascadedpageBreakAfter()
{
    return  (stylePageBreak)(((CTreeNode::GetFancyFormat()->_bPageBreaks) & 0xf0)>>4);
}

inline const TCHAR *
CTreeNode::GetCascadedfontFaceName()
{
    return  GetCharFormat()->GetFaceName();
}

inline const TCHAR *
CTreeNode::GetCascadedfontFamilyName()
{
    return  GetCharFormat()->GetFamilyName();
}


inline BOOL
CTreeNode::GetCascadedfontItalic()
{
    return  (BOOL)(GetCharFormat()->_fItalic);
}

inline long
CTreeNode::GetCascadedbackgroundImageCookie()
{
    return CTreeNode::GetFancyFormat()->_lImgCtxCookie;
}

inline styleCursor
CTreeNode::GetCascadedcursor()
{
    return  (styleCursor)(GetCharFormat()->_bCursorIdx);
}

inline styleTableLayout
CTreeNode::GetCascadedtableLayoutEnum()
{
    return GetCascadedtableLayout() ? styleTableLayoutFixed : styleTableLayoutAuto;

}

inline styleBorderCollapse
CTreeNode::GetCascadedborderCollapseEnum()
{
    return GetCascadedborderCollapse() ? styleBorderCollapseCollapse : styleBorderCollapseSeparate;
}

// This is used during layout to get the block level direction for the node.
inline styleDir
CTreeNode::GetCascadedBlockDirection()
{
  return (styleDir)(GetParaFormat()->_fRTLInner ? styleDirRightToLeft : styleDirLeftToRight);
}

// This is used for OM support to get the direction of any node
inline styleDir
CTreeNode::GetCascadeddirection()
{
  return (styleDir)(GetCharFormat()->_fRTL ? styleDirRightToLeft : styleDirLeftToRight);
}
inline styleBidi
CTreeNode::GetCascadedunicodeBidi()
{
  return (styleBidi)(GetCharFormat()->_fBidiEmbed ? (GetCharFormat()->_fBidiOverride ? styleBidiOverride : styleBidiEmbed) : styleBidiNormal);
}

inline styleLayoutGridMode
CTreeNode::GetCascadedlayoutGridMode()
{
    styleLayoutGridMode mode = GetCharFormat()->GetLayoutGridMode(TRUE);
    return (mode != styleLayoutGridModeNotSet) ? mode : styleLayoutGridModeBoth;
}

inline styleLayoutGridType
CTreeNode::GetCascadedlayoutGridType()
{
    styleLayoutGridType type = GetCharFormat()->GetLayoutGridType(TRUE);
    return (type != styleLayoutGridTypeNotSet) ? type : styleLayoutGridTypeLoose;
}

inline CUnitValue
CTreeNode::GetCascadedlayoutGridLine()
{
/*    if (GetCharFormat()->HasLineGridSizeOverrriden())
    {
        // return overriden format
    }
*/    return GetParaFormat()->GetLineGridSize(TRUE);
}

inline CUnitValue
CTreeNode::GetCascadedlayoutGridChar()
{
/*    if (GetCharFormat()->HasCharGridSizeOverrriden())
    {
        // return overriden format
    }
*/    return GetParaFormat()->GetCharGridSize(TRUE);
}

inline LONG
CTreeNode::GetCascadedtextAutospace()
{
    return GetCharFormat()->_fTextAutospace;
};

inline styleWordBreak
CTreeNode::GetCascadedwordBreak()
{
    return GetParaFormat()->_fWordBreak == styleWordBreakNotSet
           ? styleWordBreakNormal
           : (styleWordBreak)GetParaFormat()->_fWordBreak;
}

inline styleLineBreak
CTreeNode::GetCascadedlineBreak()
{
    return GetCharFormat()->_fLineBreakStrict ? styleLineBreakStrict : styleLineBreakNormal;
}

inline styleTextJustify
CTreeNode::GetCascadedtextJustify()
{
    return styleTextJustify(CTreeNode::GetParaFormat()->_uTextJustify);
}

inline styleTextJustifyTrim
CTreeNode::GetCascadedtextJustifyTrim()
{
    return styleTextJustifyTrim(CTreeNode::GetParaFormat()->_uTextJustifyTrim);
}

inline CUnitValue
CTreeNode::GetCascadedmarginTop()
{
    return (CUnitValue) CTreeNode::GetFancyFormat()->_cuvMarginTop;
}

inline CUnitValue
CTreeNode::GetCascadedmarginRight()
{
    return (CUnitValue) CTreeNode::GetFancyFormat()->_cuvMarginRight;
}

inline CUnitValue
CTreeNode::GetCascadedmarginBottom()
{
    return (CUnitValue) CTreeNode::GetFancyFormat()->_cuvMarginBottom;
}

inline CUnitValue
CTreeNode::GetCascadedmarginLeft()
{
    return (CUnitValue) CTreeNode::GetFancyFormat()->_cuvMarginLeft;
}

inline CUnitValue
CTreeNode::GetCascadedtextKashida()
{
    return CTreeNode::GetParaFormat()->_cuvTextKashida;
}

//
// -------------------------------
// Positioning inline methods
// -------------------------------
//
inline BOOL
CTreeNode::IsPositioned ( void )
{
    return GetFancyFormat()->_fPositioned;
}

inline BOOL
CTreeNode::IsPositionStatic ( void )
{
    return !GetFancyFormat()->_fPositioned;
}

inline BOOL
CTreeNode::IsAbsolute( stylePosition st )
{
    return (Tag() == ETAG_ROOT || Tag() == ETAG_BODY || st == stylePositionabsolute);
}

inline BOOL
CTreeNode::IsAbsolute( void )
{
    return IsAbsolute(GetCascadedposition());
}

inline BOOL
CTreeNode::IsAligned( void )
{
    return GetFancyFormat()->IsAligned();
}

// See description of these in class declaration

inline BOOL
CTreeNode::IsRelative( stylePosition st )
{
    return (st == stylePositionrelative);
}

inline BOOL
CTreeNode::IsRelative( void )
{
    return IsRelative(GetCascadedposition());
}

inline BOOL
CTreeNode::IsInheritingRelativeness( void )
{
    return ( GetCharFormat()->_fRelative );
}

// Returns TRUE if we're the body or a scrolling DIV or SPAN
inline BOOL
CTreeNode::IsScrollingParent( void)
{
    return GetFancyFormat()->_fScrollingParent;
}

inline BOOL
CTreeNode::IsClipParent( void )
{
    const CFancyFormat * pFF = GetFancyFormat();

    return IsAbsolute(stylePosition(pFF->_bPositionType)) || pFF->_fScrollingParent;
}

inline BOOL
CTreeNode::IsZParent( void )
{
    return GetFancyFormat()->_fZParent;
}

//+---------------------------------------------------------------------------
//
// Inline CElement members
//
//----------------------------------------------------------------------------

//
// -------------------------------
// Positioning inline methods
// -------------------------------
//

inline BOOL
CElement::IsAligned( void)
{
    return GetFirstBranch()->IsAligned();
}

inline BOOL
CElement::IsPositioned ( void )
{
    return GetFirstBranch()->IsPositioned();
}

inline BOOL
CElement::IsPositionStatic ( void )
{
    return GetFirstBranch()->IsPositionStatic();
}

inline BOOL
CElement::IsAbsolute( void )
{
    return GetFirstBranch()->IsAbsolute();
}

inline BOOL
CElement::IsRelative( void )
{
    return GetFirstBranch()->IsRelative();
}

inline BOOL
CElement::IsInheritingRelativeness( void )
{
    return GetFirstBranch()->IsInheritingRelativeness();
}

// Returns TRUE if we're the body or a scrolling DIV or SPAN
inline BOOL
CElement::IsScrollingParent( void )
{
    return GetFirstBranch()->IsScrollingParent();
}

inline BOOL
CElement::IsClipParent( void )
{
    return GetFirstBranch()->IsClipParent();
}

inline BOOL
CElement::IsZParent( void )
{
    return GetFirstBranch()->IsZParent();
}

inline BOOL
CElement::HasInLineStyles ( void )
{
    CAttrArray *pStyleAA = GetInLineStyleAttrArray();
    if ( pStyleAA && pStyleAA->Size() )
        return TRUE;
    else
        return FALSE;
}

inline BOOL
CElement::HasClassOrID ( void )
{
    return GetAAclassName() || GetAAid();
}

inline CStyle *
CElement::GetInLineStylePtr ( )
{
    CStyle *pStyleInLine = NULL;
    GetPointerAt ( FindAAIndex ( DISPID_INTERNAL_CSTYLEPTRCACHE,CAttrValue::AA_Internal ),
        (void **)&pStyleInLine );
    return pStyleInLine;
}

inline CStyle *
CElement::GetRuntimeStylePtr ( )
{
    CStyle *pStyle = NULL;
    GetPointerAt ( FindAAIndex ( DISPID_INTERNAL_CRUNTIMESTYLEPTRCACHE,
                                 CAttrValue::AA_Internal ),
                   (void **)&pStyle );
    return pStyle;
}

inline CFilterArray *
CElement::GetFilterCollectionPtr ()
{
    CFilterArray *pFA = NULL;
    GetPointerAt ( FindAAIndex ( DISPID_INTERNAL_FILTERPTRCACHE,
                                 CAttrValue::AA_Internal ), (void **)&pFA );
    return pFA;
}

inline BOOL
CTreeNode::IsDisplayNone()
{
    return (BOOL)GetCharFormat()->IsDisplayNone();
}

inline BOOL
CElement::IsDisplayNone()
{
    CTreeNode * pNode = GetFirstBranch();

    return pNode ? pNode->IsDisplayNone() : TRUE;
}

inline BOOL
CTreeNode::IsVisibilityHidden()
{
    return (BOOL)GetCharFormat()->IsVisibilityHidden();
}

inline BOOL
CElement::IsVisibilityHidden()
{
    CTreeNode * pNode = GetFirstBranch();

    return pNode ? pNode->IsVisibilityHidden() : TRUE;
}

inline BOOL
CElement::IsInlinedElement()
{
    // return TRUE for non-sites
    return GetFirstBranch()->IsInlinedElement();
}

inline BOOL
CElement::IsContainer ( )
{
    return HasFlag( TAGDESC_CONTAINER );
}

inline BOOL
CTreeNode::IsContainer ( )
{
    return Element()->IsContainer();
}

inline BOOL
CTreeNode::SupportsHtml ( )
{
    CElement * pContainer = GetContainer();

    return ( pContainer && pContainer->HasFlag( TAGDESC_ACCEPTHTML ) );
}

inline void
CTreeNode::SetElement(CElement *pElement)
{
    _pElement =  pElement;
    if (pElement)
    {
        _etag = pElement->_etag;
        WHEN_DBG(_etagDbg = (ELEMENT_TAG)_etag);
    }
}

inline void
CTreeNode::SetParent(CTreeNode *pNodeParent)
{
    _pNodeParent = pNodeParent;
}

inline CDoc *
CTreeNode::Doc()
{
    Assert( _pElement );
    return _pElement->Doc();
}

inline
CTreeNode::CLock::CLock(CTreeNode *pNode)
{
    Assert(pNode);
    _pNode = pNode;
    pNode->NodeAddRef();
}

inline
CTreeNode::CLock::~CLock()
{
    _pNode->NodeRelease();
}

//+----------------------------------------------------------------------------
//
//  Function:   GetCascadedAlign
//
//  Synopsis:   Returns the cascaded alignment for an element.
//              Normally this would be generated by the pdlparse.exe, but the
//              this was non-normal due the multiple alignment types and the
//              way the measurer and renderer expect alignment.
//
//              see cfpf.cxx, CascadeAlign for more details
//
//-----------------------------------------------------------------------------

htmlBlockAlign
inline CElement::GetParagraphAlign ( BOOL fOuter )
{
    return GetFirstBranch()->GetParagraphAlign(fOuter);
}

htmlBlockAlign
inline CTreeNode::GetParagraphAlign ( BOOL fOuter )
{
    return htmlBlockAlign( GetParaFormat()->GetBlockAlign(!fOuter) );
}

inline htmlControlAlign
CElement::GetSiteAlign()
{
    return GetFirstBranch()->GetSiteAlign();
}

inline htmlControlAlign
CTreeNode::GetSiteAlign()
{
    return htmlControlAlign(GetFancyFormat()->_bControlAlign);
}

//+---------------------------------------------------------------------------
//
//  Member:     CElement::GetElementCch
//
//  Synopsis:   Get the number of charactersinfluenced by this element
//
//  Returns:    LONG        - no of characters, 0 if the element is not in the
//                            the tree.
//
//----------------------------------------------------------------------------

inline long
CElement::GetElementCch()
{
    long cpStart, cpFinish;

    return GetFirstAndLastCp(&cpStart, &cpFinish);
}

inline BOOL
CElement::IsNoScope ( )
{
    return HasFlag( TAGDESC_TEXTLESS );
}

// N.B. (CARLED) in order for body.onfoo = new Function("") to work you
// need to add the BodyAliasForWindow flag in the PDL files for each of
// these properties
inline CBase *CElement::GetBaseObjectFor ( DISPID dispID )
{
    return ( ( ETAG_BODY == Tag() || ETAG_FRAMESET == Tag() ) &&
       ( dispID == DISPID_EVPROP_ONLOAD ||
       dispID == DISPID_EVPROP_ONUNLOAD ||
       dispID == DISPID_EVPROP_ONRESIZE ||
       dispID == DISPID_EVPROP_ONSCROLL ||
       dispID == DISPID_EVPROP_ONBEFOREUNLOAD ||
       dispID == DISPID_EVPROP_ONHELP ||
       dispID == DISPID_EVPROP_ONBLUR ||
       dispID == DISPID_EVPROP_ONFOCUS ||
       dispID == DISPID_EVPROP_ONBEFOREPRINT ||
       dispID == DISPID_EVPROP_ONAFTERPRINT ) ) ?
       GetOmWindow() : this;
}

// Does the element that this node points to have currency?
inline BOOL
CTreeNode::HasCurrency()
{
    return (this && Element()->HasCurrency());
}

// CTreeNode - layout related functions
inline BOOL
CTreeNode::HasLayout()
{
    return Element()->HasLayout();
}

inline BOOL
CTreeNode::NeedsLayout()
{
    return Element()->NeedsLayout();
}

inline CFlowLayout *
CTreeNode::HasFlowLayout()
{
    return Element()->HasFlowLayout();
}

inline CLayout *
CTreeNode::GetUpdatedLayout()
{
    return Element()->GetUpdatedLayout();
}

inline CLayout *
CTreeNode::GetUpdatedLayoutPtr()
{
    return Element()->GetUpdatedLayoutPtr();
}

inline CLayout *
CTreeNode::GetCurLayout()
{
    return Element()->GetCurLayout();
}

inline CLayout *
CTreeNode::GetCurNearestLayout()
{
    if(this)
    {
        CLayout * pLayout = GetCurLayout();

        return (pLayout ? pLayout : GetCurParentLayout());
    }
    else
        return NULL;
}

inline CLayout *
CTreeNode::GetUpdatedNearestLayout()
{
    if(this)
    {
        CLayout * pLayout = GetUpdatedLayout();

        return (pLayout ? pLayout : GetUpdatedParentLayout());
    }
    else
        return NULL;
}

inline CTreeNode *
CTreeNode::GetCurNearestLayoutNode()
{
    return (HasLayout() ? this : GetCurParentLayoutNode());
}

inline CTreeNode *
CTreeNode::GetUpdatedNearestLayoutNode()
{
    return (NeedsLayout() ? this : GetUpdatedParentLayoutNode());
}

inline CElement *
CTreeNode::GetCurNearestLayoutElement()
{
    return (HasLayout() ? Element() : GetCurParentLayoutElement());
}
inline CElement *
CTreeNode::GetUpdatedNearestLayoutElement()
{
    return (NeedsLayout() ? Element() : GetUpdatedParentLayoutElement());
}

inline CLayout *
CTreeNode::GetCurParentLayout()
{
    CTreeNode * pTreeNode = GetCurParentLayoutNode();
    return  pTreeNode ? pTreeNode->GetCurLayout() : NULL;
}

inline CLayout *
CTreeNode::GetUpdatedParentLayout()
{
    CTreeNode * pTreeNode = GetUpdatedParentLayoutNode();
    return  pTreeNode ? pTreeNode->GetUpdatedLayoutPtr() : NULL;
}

inline CElement *
CTreeNode::GetCurParentLayoutElement()
{
    return GetCurParentLayoutNode()->SafeElement();
}

inline CElement *
CTreeNode::GetUpdatedParentLayoutElement()
{
    return GetUpdatedParentLayoutNode()->SafeElement();
}

inline CElement *
CTreeNode::GetFlowLayoutElement()
{
    return GetFlowLayoutNode()->SafeElement();
}

inline BOOL
CTreeNode::HasFilterPtr()
{
    return Element()->HasFilterPtr();
}


//
// CElement - layout related functions
//

inline CLayout *       
CElement::GetLayoutPtr() const
{
    Assert( _pLayoutDbg == ( HasLayoutPtr() ? (CLayout*) __pvChain : NULL ) );
    return HasLayoutPtr() ? (CLayout*) __pvChain : NULL;
}

inline BOOL
CElement::NeedsLayout()
{
    if (_fLayoutAlwaysValid)
        return HasLayoutPtr();

    CTreeNode * pNode = GetFirstBranch();
    if (pNode && pNode->_iFF != -1)
        return pNode->_fHasLayout;

    return HasLayoutLazy();
}

inline CLayout *
CElement::GetUpdatedLayout()
{
    return NeedsLayout()
            ? GetUpdatedLayoutPtr()
            : NULL;
}

inline CLayout *
CElement::GetUpdatedLayoutPtr()
{
    Assert(NeedsLayout());
    return HasLayoutPtr()
                ? GetLayoutPtr()
                : GetLayoutLazy();
}

inline CLayout *
CElement::GetLayoutLazy()
{
    Assert (!HasLayoutPtr());
    Assert (GetFirstBranch()->GetFancyFormat()->_fHasLayout == GetFirstBranch()->_fHasLayout);

    CreateLayout();

    return GetLayoutPtr();
}

inline CLayout *
CElement::GetCurNearestLayout()
{
    CLayout * pLayout = GetCurLayout();

    return (pLayout ? pLayout : GetCurParentLayout());
}

inline CLayout *
CElement::GetUpdatedNearestLayout()
{
    CLayout * pLayout = GetUpdatedLayout();

    return (pLayout ? pLayout : GetUpdatedParentLayout());
}

inline CTreeNode *
CElement::GetCurNearestLayoutNode()
{
    return (HasLayout() ? GetFirstBranch() : GetCurParentLayoutNode());
}

inline CTreeNode *
CElement::GetUpdatedNearestLayoutNode()
{
    return (NeedsLayout() ? GetFirstBranch() : GetUpdatedParentLayoutNode());
}

inline CElement *
CElement::GetCurNearestLayoutElement()
{
    return (HasLayout() ? this : GetCurParentLayoutElement());
}

inline CElement *
CElement::GetUpdatedNearestLayoutElement()
{
    return (NeedsLayout() ? this : GetUpdatedParentLayoutElement());
}

inline CFlowLayout *
CElement::GetFlowLayout()
{
    if (!GetFirstBranch())
        return NULL;

    return GetFirstBranch()->GetFlowLayout();
}

inline CTreeNode *
CElement::GetFlowLayoutNode()
{
    if (!GetFirstBranch())
        return NULL;

    return GetFirstBranch()->GetFlowLayoutNode();
}

inline CElement *
CElement::GetFlowLayoutElement()
{
    if (!GetFirstBranch())
        return NULL;

    return GetFirstBranch()->GetFlowLayoutElement();
}

inline CLayout *
CElement::GetCurParentLayout()
{
    if (!GetFirstBranch())
        return NULL;

    return GetFirstBranch()->GetCurParentLayout();
}

inline CLayout *
CElement::GetUpdatedParentLayout()
{
    if (!GetFirstBranch())
        return NULL;

    return GetFirstBranch()->GetUpdatedParentLayout();
}

inline CTreeNode *
CElement::GetCurParentLayoutNode()
{
    if (!GetFirstBranch())
        return NULL;

    return GetFirstBranch()->GetCurParentLayoutNode();
}

inline CTreeNode *
CElement::GetUpdatedParentLayoutNode()
{
    if (!GetFirstBranch())
        return NULL;

    return GetFirstBranch()->GetUpdatedParentLayoutNode();
}

inline CElement *
CElement::GetCurParentLayoutElement()
{
    if (!GetFirstBranch())
        return NULL;

    return GetFirstBranch()->GetCurParentLayoutElement();
}

inline CElement *
CElement::GetUpdatedParentLayoutElement()
{
    Assert(GetFirstBranch());

    return GetFirstBranch()->GetCurParentLayoutElement();
}

inline LPCTSTR CElement::GetAAid() const 
{
    LPCTSTR v;

    if ( !IsNamed() )
        return NULL;

    CAttrArray::FindString( *GetAttrArray(), &s_propdescCElementid.a, &v);
    return *(LPCTSTR*)&v;
}

//+---------------------------------------------------------------------------
//
// CUnknownElement
//
//----------------------------------------------------------------------------

class CUnknownElement : public CElement
{
    DECLARE_CLASS_TYPES(CUnknownElement, CElement)

public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CUnknownElement))

    CUnknownElement ( CHtmTag * pht, CDoc *pDoc );

    virtual ~ CUnknownElement() { }

    virtual HRESULT Init2(CInit2Context * pContext);

    static HRESULT CreateElement(CHtmTag *pht,
                              CDoc *pDoc, CElement **ppElementResult);

#ifdef VSTUDIO7
    // Set the tagname and namespace of the element.
    HRESULT InternalSetTagName(CHtmTag *pht);
#else
    virtual const TCHAR *TagName()
    {
        return _cstrTagName;
    }
#endif //VSTUDIO7
    
    #define _CUnknownElement_
    #include "unknown.hdl"

    DECLARE_CLASSDESC_MEMBERS;

private:
#ifndef VSTUDIO7
    CStr _cstrTagName;
#endif //VSTUDIO7
    NO_COPY(CUnknownElement);
};

// Element Factory - Base Class for creating new elements
// provides support for JScript "new" operator
// Derived class must have a method with dispid=DISPID_VALUE
// that will be invoked when "new" is specified.

#ifdef WIN16
#define DECLARE_DERIVED_DISPATCH_NO_INVOKE(cls)                         \
    DECLARE_TEAROFF_METHOD(GetTypeInfo, gettypeinfo,                    \
            (UINT itinfo, ULONG lcid, ITypeInfo ** pptinfo))                \
        { return cls::GetTypeInfo(itinfo, lcid, pptinfo); }                 \
    DECLARE_TEAROFF_METHOD(GetTypeInfoCount, gettypeinfocount,          \
            (UINT * pctinfo))                                               \
        { return cls::GetTypeInfoCount(pctinfo); }                          \
    DECLARE_TEAROFF_METHOD(GetIDsOfNames, getidsofnames,                \
                            (REFIID riid,                                   \
                             LPOLESTR * rgszNames,                          \
                             UINT cNames,                                   \
                             LCID lcid,                                     \
                             DISPID * rgdispid))                            \
        { return cls::GetIDsOfNames(                                        \
                            riid,                                           \
                            rgszNames,                                      \
                            cNames,                                         \
                            lcid,                                           \
                            rgdispid); }                                    \

#endif

class CElementFactory : public CBase
{
    DECLARE_CLASS_TYPES(CElementFactory, CBase)
public:
#ifndef WIN16
    DECLARE_TEAROFF_TABLE(IDispatchEx)
#endif // ndef WIN16

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CElementFactory))

    CElementFactory(){};
    ~CElementFactory(){};
    CDoc *_pDoc;
    //IDispatch methods
    DECLARE_PRIVATE_QI_FUNCS(CBase)
    HRESULT QueryInterface(REFIID iid, void **ppv){return PrivateQueryInterface(iid, ppv);}

    //IDispatchEx methods
    DECLARE_TEAROFF_METHOD(InvokeEx, invokeex, (DISPID dispidMember,
                          LCID lcid,
                          WORD wFlags,
                          DISPPARAMS * pdispparams,
                          VARIANT * pvarResult,
                          EXCEPINFO * pexcepinfo,
                          IServiceProvider *pSrvProvider));

    struct CLASSDESC
    {
        CBase::CLASSDESC _classdescBase;
        void *_apfnTearOff;
    };

};

HRESULT StoreLineAndOffsetInfo(
    CBase *      pBaseObj,
    DISPID       dispid,
    ULONG        uLine,
    ULONG        uOffset);

HRESULT GetLineAndOffsetInfo(
    CAttrArray * pAA,
    DISPID       dispid,
    ULONG *      puLine,
    ULONG *      puOffset);

#pragma INCMSG("--- End 'element.hxx'")
#else
#pragma INCMSG("*** Dup 'element.hxx'")
#endif
