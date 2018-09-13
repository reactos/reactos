//+----------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994, 1995, 1996, 1997, 1998
//
//  File:       notify.hxx
//
//  Contents:   Notification base classes
//
//  Classes:    CNotification, et. al.
//
//-----------------------------------------------------------------------------

#ifndef I_NOTIFY_HXX_
#define I_NOTIFY_HXX_
#pragma INCMSG("--- Beg 'notify.hxx'")

#ifndef X_RECT_HXX_
#define X_RECT_HXX_
#include "rect.hxx"
#endif

#ifndef X_SIZE_HXX_
#define X_SIZE_HXX_
#include "size.hxx"
#endif

#ifndef X_POINT_HXX_
#define X_POINT_HXX_
#include "point.hxx"
#endif

class CElement;
class CNotification;
class CTreeNode;

MtExtern(CNotification)
MtExtern(CDirtyTreeRegion)
MtExtern(CDirtyTextRegion)


//+----------------------------------------------------------------------------
//
//  Enumeration:    NOTIFYTYPE
//
//  Synopsis:       Collection of notification types used in CNotification
//
//-----------------------------------------------------------------------------
enum NOTIFYTYPE
{
    #define  _NOTIFYTYPE_ENUM_
    #include "notifytype.h"

    //
    //  End of notifications
    //

    NTYPE_MAX                               // Last enum
};


//+----------------------------------------------------------------------------
//
//  Enumeration:    NOTIFYFLAGS
//
//  Synopsis:       Collection of notification types used in CNotification
//
//-----------------------------------------------------------------------------
enum NOTIFYFLAGS
{
    //
    //  Target flags
    //

    NFLAGS_SELF              = 0x00000001,  // Send notificaiton to start node
    NFLAGS_ANCESTORS         = 0x00000002,  // Send notification to ancestors
    NFLAGS_DESCENDENTS       = 0x00000004,  // Send notification to descendents
    NFLAGS_TREELEVEL         = 0x00000008,  // Send notification to tree-level listeners

    NFLAGS_TARGETMASK        = 0x0000000F,

    //
    //  Category flags
    //

    NFLAGS_TEXTCHANGE        = 0x00000010,  // Text has changed in some way
    NFLAGS_TREECHANGE        = 0x00000020,  // Tree has changed in some way
    NFLAGS_LAYOUTCHANGE      = 0x00000040,  // Layout may be changed
    NFLAGS_FOR_ACTIVEX       = 0x00000080,  // Send only to ActiveX elements/layouts
    NFLAGS_FOR_LAYOUTS       = 0x00000100,  // Send notification to layout elements
    NFLAGS_FOR_POSITIONED    = 0x00000200,  // Send notificaiton only to positioned elements
    NFLAGS_FOR_WINDOWED      = 0x00000400,  // Send notification only to elements with a window
    NFLAGS_FOR_ALLELEMENTS   = 0x00000800,  // Send to all elements

    NFLAGS_CATEGORYMASK      = 0x00000FF0,

    //
    //  Property flags
    //  (May be part of the notification definition)
    //

    NFLAGS_SENDUNTILHANDLED  = 0x00001000,  // Send the notification until handled
    NFLAGS_LAZYRANGE         = 0x00002000,  // Obtain range information lazily
    NFLAGS_CLEANCHANGE       = 0x00004000,  // Change does not dirty the document (normally they do)
    NFLAGS_CALLSUPERLAST     = 0x00008000,  // Call super after local handling (default is to call it before)
    NFLAGS_DATAISVALID       = 0x00010000,  // Data field is valid (type is notification dependent)
    NFLAGS_SYNCHRONOUSONLY   = 0x00020000,  // Fail if the notification cannot be handled synchronously
    NFLAGS_DONOTBLOCK        = 0x00040000,  // Do not block other notifications while sending this notification
    NFLAGS_CLEARFORMATSELF   = 0x00080000,  // Clear the cached formats on self
    NFLAGS_CLEARFORMAT       = 0x00100000,  // Clear the cached formats of each ancestor/descendent element/layout
    NFLAGS_AUTOONLY          = 0x00200000,  // Skip over layouts which do not contain auto positioned content
    NFLAGS_ZPARENTSONLY      = 0x00400000,  // Skip over z-parents (after notifying them) (that is, do not tunnel into z-parents)
    NFLAGS_SC                = 0x00800000,  // This is a second chance notification
    NFLAGS_SC_AVAILABLE      = 0x01000000,  // There is a second chance notification for this notification

    NFLAGS_PROPERTYMASK      = 0x01FFF000,

    //
    //  Control flags
    //  (Only set at run-time)
    //

    NFLAGS_FORCE             = 0x02000000,  // Force a response
    NFLAGS_SENDENDED         = 0x04000000,  // Discontinue sending the notification
    NFLAGS_DONOTLAYOUT       = 0x08000000,  // Skip layout processing (e.g., do not Enqueue a layout request)
    NFLAGS_SC_REQUESTED      = 0x10000000,  // Second chance requested on this notification
    NFLAGS_PARSER_TEXTCHANGE = 0x20000000,  // Text change requested on this notification and it comes from parser
    NFLAGS_NEEDS_RELEASE     = 0x40000000,  // Contained CElement has been AddRef'd
    NFLAGS_FORWARDED         = 0x80000000,  // Notification is being forwarded from a nested markup

    NFLAGS_CONTROLMASK       = 0x1E000000,


};


//+----------------------------------------------------------------------------
//
//  Class:  CDirtyTreeRegion
//
//-----------------------------------------------------------------------------
class CDirtyTreeRegion
{
public:
    long    _si;
    long    _cElementOld;
    long    _cElementNew;

    CDirtyTreeRegion() { Reset(); }

    BOOL    IsDirty() const { return (_si >= 0); }
    void    Accumulate(CNotification * pnf);
    void    Adjust(CNotification * pnf);
    void    Reset();

protected:
    void    ElementsAdded(CNotification * pnf);
    void    ElementsDeleted(CNotification * pnf);
};


//+----------------------------------------------------------------------------
//
//  Class:  CDirtyTextRegion
//
//-----------------------------------------------------------------------------
class CDirtyTextRegion
{
public:
    long    _cp;
    long    _cchNew;
    long    _cchOld;

    CDirtyTextRegion() { Reset(); }

    BOOL    IsDirty() const { return (_cp >= 0); }
    void    Accumulate(CNotification * pnf, long cpFirst, long cpLast, BOOL fInnerRange);
    void    Adjust(CNotification * pnf, long cpFirst, long cpLast = LONG_MAX);
    void    Reset();

    BOOL    IsBeforeDirty(long cp, long cch) const;
    BOOL    IntersectsDirty(long cp, long cch) const;
    BOOL    IsAfterDirty(long cp, long cch) const;

protected:
    void    TextAdded(long dcp, long cch);
    void    TextDeleted(long dcp, long cch);
    void    TextChanged(long dcp, long cch);
};


//+----------------------------------------------------------------------------
//
//  Class:  CSaveNotifyFlags
//
//-----------------------------------------------------------------------------
class CSaveNotifyFlags
{
    friend class CNotification;

public:
    CSaveNotifyFlags(CNotification * pnf);
    ~CSaveNotifyFlags();

protected:
    CNotification * _pnf;
    DWORD           _grfFlags;
};


//+----------------------------------------------------------------------------
//
//  Class:  CNotification
//
//-----------------------------------------------------------------------------
class CNotification
{
    friend class CMarkup;
    friend class CSaveNotifyFlags;

public:

    //
    //  General Purpose Initialization
    //

    void    Initialize(NOTIFYTYPE  ntype,
                       CElement *  pElement,
                       CTreeNode * pNode = NULL,
                       void *      data = NULL,
                       DWORD       grfFlags = 0);
    void    InitializeSi(NOTIFYTYPE  ntype,
                         long        siElement,
                         long        cElements,
                         CTreeNode * pNode = NULL,
                         void *      data = NULL,
                         DWORD       grfFlags = 0);
    void    InitializeCp(NOTIFYTYPE  ntype,
                         CElement *  pElement,
                         long        cp,
                         long        cch,
                         CTreeNode * pNode = NULL,
                         void *      data = NULL,
                         DWORD       grfFlags = 0);
    void    InitializeCp(NOTIFYTYPE  ntype,
                         long        cp,
                         long        cch,
                         CTreeNode * pNode = NULL,
                         void *      data = NULL,
                         DWORD       grfFlags = 0);
    void    InitializeSiCp(NOTIFYTYPE  ntype,
                           CElement *  pElement,
                           long        siElement,
                           long        cElements,
                           long        cp,
                           long        cch,
                           CTreeNode * pNode = NULL,
                           void *      data = NULL,
                           DWORD       grfFlags = 0);
    void    InitializeSc(CNotification * pNF);

    //
    //  Notification initialization Helpers
    //

    #define  _NOTIFYTYPE_PROTO_
    #include "notifytype.h"

    //
    //  Modification
    //

    void    ChangeTo(NOTIFYTYPE ntype, CElement * pElement, CTreeNode * pNode = NULL, void * data = NULL);

    void    SetFlag(NOTIFYFLAGS f);
    void    ClearFlag(NOTIFYFLAGS f);
    BOOL    IsFlagSet(NOTIFYFLAGS f) const;
    DWORD   LayoutFlags() const;

    void    SetHandler(CElement * pElementHandler);
    BOOL    IsHandled() const;

    void    SetElement(CElement * pElement, BOOL fKeepRange = FALSE);
    void    SetNode(CTreeNode * pNode);

    void    SetElementRange(long siElement, long cElements);
    void    SetTextRange(long cp, long cch);
    void    ClearTextRange();
    void    AddChars(long cch);

    void    SetData(void * pv);
    void    SetData(DWORD dw);
    void    SetData(const CPoint & pt);
    void    SetData(const CSize & size);
    void    SetData(const CRect & rc);
    void    ClearData();

    //
    // Second Chance notifications
    //

    BOOL    IsSecondChance()                    { return IsFlagSet( NFLAGS_SC ); }
    BOOL    IsSecondChanceAvailable()           { return IsFlagSet( NFLAGS_SC_AVAILABLE ); }
    void    SetSecondChanceRequested()          { SetFlag( NFLAGS_SC_REQUESTED ); }
    void    ClearSecondChanceRequested()        { ClearFlag( NFLAGS_SC_REQUESTED ); }
    BOOL    IsSecondChanceRequested()           { return IsFlagSet( NFLAGS_SC_REQUESTED ); }


    //
    //  Data Retrieval
    //

    NOTIFYTYPE  Type() const                    { return _ntype; }
    BOOL        IsType(NOTIFYTYPE ntype) const  { return _ntype == ntype; }

    BOOL        IsTextChange() const        { return IsFlagSet(NFLAGS_TEXTCHANGE); }
    BOOL        IsTreeChange() const        { return IsFlagSet(NFLAGS_TREECHANGE); }
    BOOL        IsLayoutChange() const      { return IsFlagSet(NFLAGS_LAYOUTCHANGE); }
    BOOL        IsForActiveX() const        { return IsFlagSet(NFLAGS_FOR_ACTIVEX); }
    BOOL        IsForLayouts() const        { return IsFlagSet(NFLAGS_FOR_LAYOUTS); }
    BOOL        IsForWindowed() const       { return IsFlagSet(NFLAGS_FOR_WINDOWED); }
    BOOL        IsForPositioned() const     { return IsFlagSet(NFLAGS_FOR_POSITIONED); }
    BOOL        IsForAllElements() const    { return IsFlagSet(NFLAGS_FOR_ALLELEMENTS); }

    BOOL        IsDataValid() const         { return IsFlagSet(NFLAGS_DATAISVALID); }
    void        Data(void ** ppv) const     { *ppv = _pv; }
    void        Data(DWORD * pdw) const     { *pdw = _dw; }
    void        Data(long * pl) const       { *pl  = _l; }
    void        Data(int * pi) const        { *pi  = _i; }
    void        Data(CPoint * ppt) const    { *ppt = _pt; }
    void        Data(CSize * psize) const   { *psize = _size; }
    void        Data(CRect * prc) const     { *prc = _rc; }

    void *          DataAsPtr() const       { return _pv; }
    DWORD           DataAsDWORD() const     { return _dw; }
    long            DataAsLong() const      { return _l; }
    int             DataAsInt() const       { return _i; }
    const CPoint &  DataAsPoint() const     { return (const CPoint &)_pt; }
    const CSize &   DataAsSize() const      { return (const CSize &)_size; }
    const CRect &   DataAsRect() const      { return (const CRect &)_rc; }

    CTreeNode * Node() const            { return _pNode; }
    CElement *  Element() const         { return _pElement; }
    CElement *  Handler() const         { return _pHandler; }

    long        SI() const              { return _siElement; }
    long        CElements() const       { return _cElements; }
    long        ElementsChanged() const;

    BOOL        IsRangeValid() const    { return (_cp >= 0); }
    long        Cp(long cpFirst);
    long        Cch(long cpLast = LONG_MAX);
    long        CchChanged(long cpLast = LONG_MAX);

#if DBG==1
    DWORD       SerialNumber() const    { return _sn; }
    BOOL        IsReceived(DWORD sn) const;
    void        ResetSN();


    LPCTSTR     Name() const;
#endif

protected:

#if DBG==1
    static       DWORD      GetNextSN() { InterlockedIncrement( (long *) &s_snNext ); return s_snNext; }
    static       DWORD      s_snNext;                   // Next serial number
#endif

    static const DWORD      s_aryFlags[NTYPE_MAX];      // NTYPE-to-NFLAGs array

    NOTIFYTYPE      _ntype;     // Notification type

    CTreeNode *     _pNode;     // Node at which to start the notification
    CElement *      _pElement;  // Associated CElement (may be NULL)
    CElement *      _pHandler;  // Last CElement to consume the notification (may be NULL)
    long            _siElement; // Source index of first affected element
    long            _cElements; // Number of affected elements
    long            _cp;        // First affected character (absolute cp)
    long            _cch;       // Number of characters affected

    DWORD           _grfFlags;  // Flags

    union                       // Notification data (type depends on the notification)
    {
        void *      _pv;
        DWORD       _dw;
        long        _l;
        int         _i;
        POINT       _pt;
        SIZE        _size;
        RECT        _rc;
    };

#if DBG==1
public:
    DWORD           _sn;        // Serial number

    union
    {
        DWORD _dwDbgFlags;
        struct
        {
            DWORD   _fNoTextValidate        : 1;   // Don't validate this text change
            DWORD   _fNoElementsValidate    : 1;   // Don't validate this elements change
        };
    };

    BOOL    SendToSelfOnly() const; // used to avoid updating CLayout::_snLast

#endif

protected:

    BOOL            SendTo(NOTIFYFLAGS fFlag) const;
    void            EnsureRange();
};


//+----------------------------------------------------------------------------
//
//  Inlines
//
//-----------------------------------------------------------------------------

inline void
CDirtyTreeRegion::Reset()
{
    _si          = -1;
    _cElementOld = 0;
    _cElementNew = 0;
}

inline void
CDirtyTextRegion::Reset()
{
    _cp      = -1;
    _cchNew  = 0;
    _cchOld  = 0;
}

inline BOOL
CDirtyTextRegion::IsBeforeDirty(
    long    cp,
    long    cch) const
{
    return (    _cp < 0
            ||  (cp + cch) < _cp);
}

inline BOOL
CDirtyTextRegion::IntersectsDirty(
    long    cp,
    long    cch) const
{
    return (    _cp >= 0
            &&  cp  >= _cp
            &&  cp  <  (_cp + _cchNew));
}

inline BOOL
CDirtyTextRegion::IsAfterDirty(
    long    cp,
    long    cch) const
{
    return (    _cp >= 0
            &&  (cp >= (_cp + _cchNew)));
}

inline
CSaveNotifyFlags::CSaveNotifyFlags(
    CNotification * pnf)
{
    Assert(pnf);
    _pnf      = pnf;
    _grfFlags = pnf->_grfFlags;
}

inline
CSaveNotifyFlags::~CSaveNotifyFlags()
{
    _pnf->_grfFlags = _grfFlags;
}

inline void
CNotification::SetFlag(
    NOTIFYFLAGS f)
{
    Assert((f & NFLAGS_TARGETMASK) == 0);
    Assert((f & NFLAGS_CATEGORYMASK)  == 0);

    _grfFlags |= f;
}

inline void
CNotification::ClearFlag(
    NOTIFYFLAGS f)
{
    Assert((f & NFLAGS_TARGETMASK) == 0);
    Assert((f & NFLAGS_CATEGORYMASK)  == 0);

    _grfFlags &= ~f;
}

inline BOOL
CNotification::IsFlagSet(
    NOTIFYFLAGS f) const
{
    return !!(_grfFlags & f);
}

inline void
CNotification::SetElementRange(
    long        siElement,
    long        cElements)
{
    _siElement = siElement;
    _cElements = cElements;
}

inline void
CNotification::SetTextRange(
    long        cp,
    long        cch)
{
    _cp  = cp;
    _cch = cch;

    if (_cp >= 0)
    {
        ClearFlag(NFLAGS_LAZYRANGE);
    }
}

inline void
CNotification::ClearTextRange()
{
    _cp  = -1;
    _cch = -1;

    if (s_aryFlags[_ntype] & NFLAGS_LAZYRANGE)
    {
        SetFlag(NFLAGS_LAZYRANGE);
    }
}

inline void    
CNotification::AddChars(
    long cch)
{
    _cch += cch;
}

inline void
CNotification::SetData(
    void *  pv)
{
    _pv = pv;
    SetFlag(NFLAGS_DATAISVALID);
}

inline void
CNotification::SetData(
    DWORD   dw)
{
    _dw = dw;
    SetFlag(NFLAGS_DATAISVALID);
}

inline void
CNotification::SetData(
    const CPoint &  pt)
{
    _pt = pt;
    SetFlag(NFLAGS_DATAISVALID);
}

inline void
CNotification::SetData(
    const CSize &  size)
{
    _size = size;
    SetFlag(NFLAGS_DATAISVALID);
}

inline void
CNotification::SetData(
    const CRect &  rc)
{
    _rc = rc;
    SetFlag(NFLAGS_DATAISVALID);
}

inline void
CNotification::ClearData()
{
    ClearFlag(NFLAGS_DATAISVALID);
}

#if DBG==1
inline void
CNotification::ResetSN()
{
    _sn = GetNextSN();
}
#endif

inline void
CNotification::Initialize(
    NOTIFYTYPE  ntype,
    CElement *  pElement,
    CTreeNode * pNode,
    void *      data,
    DWORD       grfFlags)
{
    Assert(ntype >=0 && ntype < NTYPE_MAX);
    Assert((grfFlags & NFLAGS_TARGETMASK) == 0);
    Assert((grfFlags & NFLAGS_CATEGORYMASK)  == 0);

    _ntype    = ntype;
    _pv       = data;
    _pNode    = pNode;
    _grfFlags = s_aryFlags[ntype] | grfFlags;
    _pHandler = NULL;
    SetElement(pElement);
    WHEN_DBG(_dwDbgFlags = 0);
    WHEN_DBG(_sn = GetNextSN());
}

inline void
CNotification::InitializeSi(
    NOTIFYTYPE  ntype,
    long        siElement,
    long        cElements,
    CTreeNode * pNode,
    void *      data,
    DWORD       grfFlags)
{
    Assert(ntype >=0 && ntype < NTYPE_MAX);
    Assert((grfFlags & NFLAGS_TARGETMASK) == 0);
    Assert((grfFlags & NFLAGS_CATEGORYMASK)  == 0);

    _ntype    = ntype;
    _pv       = data;
    _pNode    = pNode;
    _grfFlags = s_aryFlags[ntype] | grfFlags;
    _pHandler = NULL;
    _pElement = NULL;
    SetElementRange(siElement, cElements);
    SetTextRange(-1, -1);
    WHEN_DBG(_dwDbgFlags = 0);
    WHEN_DBG(_sn = GetNextSN());
}

inline void
CNotification::InitializeCp(
    NOTIFYTYPE  ntype,
    CElement *  pElement,
    long        cp,
    long        cch,
    CTreeNode * pNode,
    void *      data,
    DWORD       grfFlags)
{
    Assert(ntype >=0 && ntype < NTYPE_MAX);
    Assert((grfFlags & NFLAGS_TARGETMASK) == 0);
    Assert((grfFlags & NFLAGS_CATEGORYMASK)  == 0);

    _ntype    = ntype;
    _pv       = data;
    _pNode    = pNode;
    _grfFlags = s_aryFlags[ntype] | grfFlags;
    _pHandler = NULL;
    _pElement = pElement;
    SetElementRange(-1, -1);
    SetTextRange(cp, cch);
    WHEN_DBG(_dwDbgFlags = 0);
    WHEN_DBG(_sn = GetNextSN());
}

inline void
CNotification::InitializeCp(
    NOTIFYTYPE  ntype,
    long        cp,
    long        cch,
    CTreeNode * pNode,
    void *      data,
    DWORD       grfFlags)
{
    Assert(ntype >=0 && ntype < NTYPE_MAX);
    Assert((grfFlags & NFLAGS_TARGETMASK) == 0);
    Assert((grfFlags & NFLAGS_CATEGORYMASK)  == 0);

    _ntype    = ntype;
    _pv       = data;
    _pNode    = pNode;
    _grfFlags = s_aryFlags[ntype] | grfFlags;
    _pHandler = NULL;
    _pElement = NULL;
    SetElementRange(-1, -1);
    SetTextRange(cp, cch);
    WHEN_DBG(_dwDbgFlags = 0);
    WHEN_DBG(_sn = GetNextSN());
}

inline void
CNotification::InitializeSiCp(
    NOTIFYTYPE  ntype,
    CElement *  pElement,
    long        siElement,
    long        cElements,
    long        cp,
    long        cch,
    CTreeNode * pNode,
    void *      data,
    DWORD       grfFlags)
{
    Assert(ntype >=0 && ntype < NTYPE_MAX);
    Assert((grfFlags & NFLAGS_TARGETMASK) == 0);
    Assert((grfFlags & NFLAGS_CATEGORYMASK)  == 0);

    _ntype    = ntype;
    _pv       = data;
    _pNode    = pNode;
    _grfFlags = s_aryFlags[ntype] | grfFlags;
    _pElement = pElement;
    _pHandler = NULL;
    SetElementRange(siElement, cElements);
    SetTextRange(cp, cch);
    WHEN_DBG(_dwDbgFlags = 0);
    WHEN_DBG(_sn = GetNextSN());
}

inline void
CNotification::InitializeSc(
    CNotification * pNF )
{
    Assert( pNF->_ntype >= 0 && pNF->_ntype < NTYPE_MAX );
    Assert( pNF->IsSecondChanceAvailable() );

    _ntype      = (NOTIFYTYPE)(((int)pNF->_ntype)+1);
    _pv         = pNF->_pv;
    _pNode      = pNF->_pNode;
    _grfFlags   =   s_aryFlags[_ntype] 
                  | (pNF->_grfFlags & ~(NFLAGS_TARGETMASK | NFLAGS_CATEGORYMASK | NFLAGS_SC_REQUESTED));
    _pElement   = pNF->_pElement;
    SetElementRange( pNF->_siElement, pNF->_cElements );
    SetTextRange( pNF->_cp, pNF->_cch );
    WHEN_DBG(_dwDbgFlags = 0);
    WHEN_DBG(_sn = GetNextSN());
}

inline void
CNotification::ChangeTo(
    NOTIFYTYPE  ntype,
    CElement *  pElement,
    CTreeNode * pNode,
    void *      data)
{
    Assert(ntype >=0 && ntype < NTYPE_MAX);
    Assert(pElement);

    //
    //  NOTE: To keep the ordering straight, changing a notification
    //        does not increment the serial number
    //

    _ntype    = ntype;
    _pv       = data;
    _pNode    = pNode;
    _grfFlags = s_aryFlags[ntype];
    _pHandler = NULL;
    SetElement(pElement);
    WHEN_DBG(_dwDbgFlags = 0);
}

inline void
CNotification::SetNode(CTreeNode * pNode)
{
    Assert(pNode);
    _pNode = pNode;
}

inline void
CNotification::SetHandler(CElement * pElementHandler)
{
    Assert(pElementHandler);
    _pHandler = pElementHandler;
}

inline BOOL
CNotification::IsHandled() const
{
    return !!_pHandler;
}

inline long
CNotification::ElementsChanged() const
{
    switch (_ntype)
    {
        case NTYPE_ELEMENTS_ADDED:
            return _cElements;

        case NTYPE_ELEMENTS_DELETED:
            return -_cElements;

        default:
            return 0L;
    }
}

inline long
CNotification::CchChanged(
    long    cpLast)
{
    if (IsFlagSet(NFLAGS_LAZYRANGE))
    {
        EnsureRange();
    }

    switch (_ntype)
    {
        case NTYPE_CHARS_ADDED:
            return min(_cch, cpLast - _cp);

        case NTYPE_CHARS_DELETED:
            return max(-_cch, _cp - cpLast);

        default:
            return 0L;
    }
}

#if DBG==1
inline BOOL
CNotification::IsReceived(DWORD sn) const
{
    return (_sn != (DWORD)-1) && (_sn <= sn);
}
#endif

inline BOOL
CNotification::SendTo(
    NOTIFYFLAGS nflag) const
{
    return IsFlagSet(nflag);
}

#if DBG==1
inline BOOL
CNotification::SendToSelfOnly() const
{
    return (    SendTo(NFLAGS_SELF)
            &&  !SendTo(NFLAGS_ANCESTORS)
            &&  !SendTo(NFLAGS_DESCENDENTS)
            &&  !SendTo(NFLAGS_TREELEVEL)
           );
}
#endif

inline long
CNotification::Cp(
    long    cpFirst)
{
    if (IsFlagSet(NFLAGS_LAZYRANGE))
    {
        EnsureRange();
    }

    return max(_cp, cpFirst);
}

inline long
CNotification::Cch(
    long    cpLast)
{
    if (IsFlagSet(NFLAGS_LAZYRANGE))
    {
        EnsureRange();
    }

    return min(_cch, cpLast - _cp);
}

#define  _NOTIFYTYPE_INLINE_
#include "notifytype.h"

//+----------------------------------------------------------------------------
//
//  General purpose helpers
//
//-----------------------------------------------------------------------------

inline BOOL
IsInvalidationNotification(CNotification * pnf)
{
    return  (  pnf->IsType(NTYPE_CHARS_INVALIDATE)
            || pnf->IsType(NTYPE_ELEMENT_INVALIDATE)
            || pnf->IsType(NTYPE_ELEMENT_INVAL_Z_DESCENDANTS));
}

inline BOOL
IsPositionNotification(CNotification * pnf)
{
    return  pnf->IsType(NTYPE_ELEMENT_ZCHANGE)
        ||  pnf->IsType(NTYPE_ELEMENT_REPOSITION);
}

#pragma INCMSG("--- End 'notify.hxx'")
#else
#pragma INCMSG("*** Dup 'notify.hxx'")
#endif
