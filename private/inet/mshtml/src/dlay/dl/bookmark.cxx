//+---------------------------------------------------------------------------
//
//  Maintained by: Jerry, Terry and Ted
//
//  Microsoft DataDocs
//  Copyright (C) Microsoft Corporation, 1994-1995
//  All rights Reserved.
//  Information contained herein is Proprietary and Confidential.
//
//  File:       ddoc\dl\bookmark.cxx
//
//  Contents:   Data Layer Bookmark/Chapter helper object
//
//  Classes:    CDataLayerBookmarkHelper
//
//  Functions:  None.
//

#include <dlaypch.hxx>

#ifndef X_DLCURSOR_HXX_
#define X_DLCURSOR_HXX_
#include "dlcursor.hxx"
#endif

DeclareTag(tagDataLayerBookmarkHelper, "DataLayerBookmark's",
           "Nile (OLE DB) bookmarks/chapters" );

MtDefine(CDataLayerBookmarkHelperRep, DataBind, "CDataLayerBookmarkHelperRep")

//+---------------------------------------------------------------------------
//
//  CDataLayerBookmarkHelperRep is the refcounted object which holds the real
//  bookmark.  Note:  This is not a OLE refcount!  Iff the Bookmark/Chapter
//  is null the pointer to this rep (_pRep) is null.  Iff the Bookmark/Chapter
//  is a predefined DBBMK_* iSize is eSimple and _dbbmk holds that byte (use
//  IsSimple() to check for this.)  Iff iSize is eFabricated then this bookmark
//  is a fabricated bookmark for use by an DataLayerCursor (use IsFabricated().)
//  _pDataLayerCursor is only used for Nile bookmarks (i.e. IsVarying, IsULONG
//  and IsHROW.)  Be sure to look at IsValidObjects routines at the bottom of
//  this file for this and other representation invariants.
//

class CDataLayerBookmarkHelperRep
{
public:
    unsigned long           _uRefCount;
    CDataLayerCursor *const _pDataLayerCursor;
    const size_t            _iSize;
    enum
        {
        eSimple = -1,
        eULONG = -2
        };
    BOOL IsSimple()      const { return _iSize == eSimple; }


    BOOL IsULONG()       const { return _iSize == eULONG; }
    BOOL IsVarying()     const { return int(_iSize) >= 0; }
    BOOL IsHCHAPTER()    const { return FALSE; }
    BOOL IsFabricated()  const { return FALSE; }
    union // NOTE:  This must be the last field in this structure! (see new).
    {
        BYTE                      _abData[1];
        ULONG                     _uData;
        HROW                      _hRow;
    };
    CDataLayerBookmarkHelperRep(CDataLayerCursor *const, const size_t iSize,
        const void *pvData );
    CDataLayerBookmarkHelperRep(CDataLayerCursor *const, const ULONG &);
    // overload new && delete to enable caching of 
    // bookmark creation (frankman 1/23/96)
    void *operator new (size_t uCompilerSize, size_t uDataSize);
    void *operator new (size_t);
    void operator delete( void* p );

    // two fixed bookmarks
    static CDataLayerBookmarkHelperRep TheFirst;
    static CDataLayerBookmarkHelperRep TheLast;

private:
//    NO_COPY(CDataLayerBookmarkHelperRep);
    CDataLayerBookmarkHelperRep(const BYTE &dbbmk ) :
        _uRefCount(1), _pDataLayerCursor(NULL), _iSize((const size_t)eSimple)
    {
        _abData[0] = dbbmk;
    }

public:
};

typedef CDataLayerBookmarkHelperRep CRep;

//+---------------------------------------------------------------------------
//
//  Member:     Constructor - Create a simple, "predefined" bookmark
//

CDataLayerBookmarkHelper::CDataLayerBookmarkHelper(const BYTE &dbbmk)
{
    TraceTag((tagDataLayerBookmarkHelper,
              "CDataLayerBookmarkHelper::constructor(%p{%u})", this, dbbmk ));

    if (dbbmk == DBBMK_FIRST)
    {
        _pRep = &CDataLayerBookmarkHelperRep::TheFirst;
    }
    else
    {
        Assert(dbbmk == DBBMK_LAST);
        _pRep = &CDataLayerBookmarkHelperRep::TheLast;
    }
    IS_VALID(this);
}

// Initialize statics

// as good a place as any to define these
extern const BYTE g_bBmkLast = DBBMK_LAST;

#ifdef _MAC
CRep CRep::TheFirst((BYTE) DBBMK_FIRST);
CRep CRep::TheLast((BYTE) DBBMK_LAST);
#else
CRep CRep::TheFirst = CRep((BYTE) DBBMK_FIRST);
CRep CRep::TheLast = CRep((BYTE) DBBMK_LAST);
#endif
const CDataLayerBookmark CDataLayerBookmark::TheNull = CDataLayerBookmark();
const CDataLayerBookmark CDataLayerBookmark::TheFirst = CDataLayerBookmark((BYTE) DBBMK_FIRST);
const CDataLayerBookmark CDataLayerBookmark::TheLast = CDataLayerBookmark((BYTE) DBBMK_LAST);


CDataLayerBookmarkHelperRep::CDataLayerBookmarkHelperRep(
    CDataLayerCursor *const pDataLayerCursor,
    const size_t iSize, const void *pvData ) :
    _uRefCount(1), _pDataLayerCursor(pDataLayerCursor), _iSize(iSize)
{
    memcpy(_abData, pvData, iSize);
}
CDataLayerBookmarkHelperRep::CDataLayerBookmarkHelperRep(
    CDataLayerCursor *const pDataLayerCursor, const ULONG &rul ) :
    _uRefCount(1), _pDataLayerCursor(pDataLayerCursor), _iSize((const size_t)eULONG)
{
    _uData = rul;
}


inline void *
CDataLayerBookmarkHelperRep::operator new(size_t uCompilerSize,
                                          size_t uDataSize)
                                          
{
    return ::new(Mt(CDataLayerBookmarkHelperRep)) BYTE[uCompilerSize + uDataSize - sizeof(void*)];
}

inline void *
CDataLayerBookmarkHelperRep::operator new(size_t uCompilerSize)
                                          
{
    return ::new(Mt(CDataLayerBookmarkHelperRep)) BYTE[uCompilerSize];
}


inline void 
CDataLayerBookmarkHelperRep::operator delete (void *p)
{
    ::delete p;
}

//+---------------------------------------------------------------------------
//
//  getDataPointer and getDataSize are used when useing a bookmark/chapter with
//  Nile.  They decode the internal rep and return appropriate info.
//

const BYTE *
CDataLayerBookmarkHelper::getDataPointer() const
{
    const BYTE *ret = IsNull() ? NULL
                               :
#if defined(PRODUCT_97)
                                            _pRep->IsFabricated() ? &g_bBmkLast :
#endif // defined(PRODUCT_97)               
                                            _pRep->_abData;
    return ret;
}


size_t
CDataLayerBookmarkHelper::getDataSize() const
{
    size_t ret = IsNull() ? 0 :
#if defined(PRODUCT_97)
                 _pRep->IsFabricated ||
#endif // defined(PRODUCT_97)
                 _pRep->IsSimple() ? 1 :
                 _pRep->IsVarying() ? _pRep->_iSize :
                sizeof(ULONG);
    return ret;
}

#if defined(PRODUCT_97)
HCHAPTER
CDataLayerBookmarkHelper::getHChapter() const
{
    if (IsNull())
    {
        return NULL;
    }
    Assert(_pRep->IsHCHAPTER());
    return _pRep->_hChapter;
}
#endif

// BUGBUG: should be in-line now, but outside world doesn't know about CRep
BOOL
CDataLayerBookmarkHelper::IsDBBMK_FIRST() const
{
    // we only allow the world to have one Rep for the for DBBMK_FIRST
    Assert((_pRep == &CRep::TheFirst)
        == (_pRep && _pRep->IsSimple() && _pRep->_abData[0] == DBBMK_FIRST) );
    return _pRep == &CRep::TheFirst;
}


// BUGBUG: should be in-line now, but outside world doesn't know about CRep
BOOL
CDataLayerBookmarkHelper::IsDBBMK_LAST() const
{
    // we only allow the world to have one Rep for the for DBBMK_LAST
    Assert((_pRep == &CRep::TheLast)
        == (_pRep && _pRep->IsSimple() && _pRep->_abData[0] == DBBMK_LAST) );
    return _pRep == &CRep::TheLast;
}

#if defined(PRODUCT_97)
const CDataLayerBookmarkHelper &
CDataLayerBookmarkHelper::getdlbh() const
{
    Assert(!IsNull());
    return _pRep->_rdlb;
}
#endif // defined(PRODUCT_97)


#if DBG == 1
//+---------------------------------------------------------------------------
//
//  Trace is used for TraceTag because decoding the rep on the fly is not easy.
//

void
CDataLayerBookmarkHelper::Trace(LPSTR pstr)
{
    if (IsNull())
    {
        TraceTag((tagDataLayerBookmarkHelper,
                  "CDataLayerBookmarkHelper::%s(%p{0})", pstr, this ));
    }
#if defined(PRODUCT_97)
    else if (_pRep->IsFabricated())
    {
        TraceTag((tagDataLayerBookmarkHelper,
                  "CDataLayerBookmarkHelper::%s(%p{%p, %u, %p, %p})",
                  pstr, this, _pRep, _pRep->_uRefCount,
                  _pRep->_pDataLayerCursor,
                  _pRep->_pDataLayerNewBookmark ));
    }
#endif // defined(PRODUCT_97)
    else
    {
        TraceTag((tagDataLayerBookmarkHelper,
                  "CDataLayerBookmarkHelper::%s(%p{%p, %u, %u, %p})",
                  pstr, this, _pRep, _pRep->_uRefCount, _pRep->_iSize,
                  _pRep->_pDataLayerCursor ));
    }
}
#define TRACE(x) Trace(x)
#else
#define TRACE(x) 0
#endif



//+---------------------------------------------------------------------------
//
//  Member:     Copy Constructor
//

CDataLayerBookmarkHelper::CDataLayerBookmarkHelper(
    const CDataLayerBookmarkHelper &rdlb ) : _pRep(rdlb._pRep)
{
    TRACE("constructor");

    IS_VALID(this);

    if (!IsNull())
    {
        _pRep->_uRefCount += 1;
    }
}




//+---------------------------------------------------------------------------
//
//  Member:     Constructor - construct from a DBVECTOR
//
//  Iff we run out of memory the resultant Bookmark/Chapter is null (use IsNull)
//

CDataLayerBookmarkHelper::CDataLayerBookmarkHelper(
    CDataLayerCursor &rDataLayerCursor, const DBVECTOR &rdbv )
{
    TraceTag((tagDataLayerBookmarkHelper,
              "CDataLayerBookmarkHelper::constructor(%p{%p, %u})",
              this, &rDataLayerCursor, rdbv.size ));

    _pRep = new (rdbv.size) CRep(&rDataLayerCursor, rdbv.size, rdbv.ptr);
    CoTaskMemFree(rdbv.ptr);
    IS_VALID(this);
}


//+---------------------------------------------------------------------------
//
//  Member:     Constructor - construct from a ULONG
//
//  Iff we run out of memory the resultant Bookmark/Chapter is null (use IsNull)
//

CDataLayerBookmarkHelper::CDataLayerBookmarkHelper(
    CDataLayerCursor &rDataLayerCursor, const ULONG &rul )
{
    TraceTag((tagDataLayerBookmarkHelper,
              "CDataLayerBookmarkHelper::constructor(%p{%p, %u})",
              this, &rDataLayerCursor, rul ));

    _pRep = new CRep(&rDataLayerCursor, rul);
    IS_VALID(this);
}


#if defined(PRODUCT_97)
CDataLayerBookmarkHelper::CDataLayerBookmarkHelper(
    const CDataLayerBookmarkHelper &rdlb,
    CDataLayerCursor &rDataLayerCursor, const DBVECTOR &rdbv )
{
    TraceTag((tagDataLayerBookmarkHelper,
              "CDataLayerBookmarkHelper::constructor(%p{%p, %u})",
              this, &rDataLayerCursor, rdbv.size ));

    _pRep = new (rdbv.size) CRep(rdlb, &rDataLayerCursor,
                                 rdbv.size, rdbv.ptr );
    CoTaskMemFree(rdbv.ptr);
    IS_VALID(this);
}



CDataLayerBookmarkHelper::CDataLayerBookmarkHelper(
    const CDataLayerBookmarkHelper &rdlb,
    CDataLayerCursor &rDataLayerCursor, const ULONG &rul )
{
    TraceTag((tagDataLayerBookmarkHelper,
              "CDataLayerBookmarkHelper::constructor(%p{%p, %u})",
              this, &rDataLayerCursor, rul ));

    _pRep = new CRep(rdlb, &rDataLayerCursor, rul);
    IS_VALID(this);
}



//+---------------------------------------------------------------------------
//
//  Constructor - construct from a DataLayerCursor and some helper stuff
//
//  Iff we run out of memory the resultant Bookmark/Chapter is null (use IsNull)
//  For CDataLayerCursor to store helper stuff in a bookmark/chapter like object
//

CDataLayerBookmarkHelper::CDataLayerBookmarkHelper(
    const CDataLayerBookmarkHelper &rdlb,
    CDataLayerNewBookmark *pDataLayerNewBookmark )
{
    TraceTag((tagDataLayerBookmarkHelper,
              "CDataLayerBookmarkHelper::constructor(%p{%p})",
              this, pDataLayerNewBookmark ));

    _pRep = new CRep(rdlb, pDataLayerNewBookmark);
    IS_VALID(this);
}



CDataLayerBookmarkHelper::CDataLayerBookmarkHelper(
    CDataLayerNewBookmark *pDataLayerNewBookmark )
{
    TraceTag((tagDataLayerBookmarkHelper,
              "CDataLayerBookmarkHelper::constructor(%p{%p})",
              this, pDataLayerNewBookmark ));

    _pRep = new CRep(pDataLayerNewBookmark);
    IS_VALID(this);
}



//+---------------------------------------------------------------------------
//
//  getFabricated fetches DataLayerCursor's private data from this bookmark
//  iff IsFabricated.  (In fact, conversly, IsFabricated calls us to see if
//  there is any data!)
//

CDataLayerNewBookmark *
CDataLayerBookmarkHelper::getFabricated() const
{
    CDataLayerNewBookmark *ret = IsNull() || !_pRep->IsFabricated() ?
                                    NULL : _pRep->_pDataLayerNewBookmark;
    return ret;
}
#endif // defined(PRODUCT_97)



#if DBG == 1
//+---------------------------------------------------------------------------
//
//  Member:     Destructor
//

CDataLayerBookmarkHelper::~CDataLayerBookmarkHelper()
{
    TRACE("destructor");

    IS_VALID(this);

    Assert("Passivate wasn't called before destructor" &&
           (IsNull()) );
}
#endif



//+---------------------------------------------------------------------------
//
//  Member:     Unlink
//
//  Decrement ref count and release if gone.
//

void
CDataLayerBookmarkHelper::Unlink()
{
    TRACE("Unlink");

    IS_VALID(this);

    if (!IsNull())
    {
        _pRep->_uRefCount -= 1;
        if (!_pRep->_uRefCount)
        {
            if (!_pRep->IsSimple())
            {
                delete _pRep;
                _pRep = 0;
            }
        }
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     Assignment operator
//

CDataLayerBookmarkHelper &
CDataLayerBookmarkHelper::operator=(const CDataLayerBookmarkHelper &rdlb)
{
    TraceTag((tagDataLayerBookmarkHelper,
              "CDataLayerBookmarkHelper::%p = %p", this, &rdlb ));

    IS_VALID(this);
    if (_pRep != rdlb._pRep)
    {
        Unlink();
        _pRep = rdlb._pRep;
        if (!IsNull())
        {
            _pRep->_uRefCount += 1;
        }
    }
    return *this;
}


//+---------------------------------------------------------------------------
//
//  Member:     operator==
//
//  Synopsis:   Compare two bookmarks
//

BOOL
CDataLayerBookmark::operator==(const CDataLayerBookmark &rdlb) const
{
    TraceTag((tagDataLayerBookmarkHelper,
              "CDataLayerBookmark::%p == %p", this, &rdlb ));

    IS_VALID((CDataLayerBookmark*)this);
    IS_VALID((CDataLayerBookmark*)&rdlb);

    if (_pRep == rdlb._pRep)
    {
        return TRUE;
    }
    if (IsNull() || rdlb.IsNull())
    {
        return FALSE;
    }

    if (_pRep->IsVarying() != rdlb._pRep->IsVarying())
    {
        return (FALSE);
    }
    if (!_pRep->IsVarying())
    {
        if (_pRep->_iSize != rdlb._pRep->_iSize)
        {
            return FALSE;
        }
        if (_pRep->IsULONG())
        {
            goto RowsetCompare;
        }

        Assert(_pRep->IsSimple());      // last case must be simple..
        // we only have two static bookmarks, and we already checked if
        //  the two _pReps matched!
        Assert(_pRep->_abData[0] != rdlb._pRep->_abData[0]);
        return FALSE;
    }
    else
    {
RowsetCompare:
        Assert(_pRep->_pDataLayerCursor == rdlb._pRep->_pDataLayerCursor);
        DBCOMPARE dbc;
        return !_pRep->_pDataLayerCursor->_pRowsetLocate->Compare(
                   _pRep->_pDataLayerCursor->_hChapter,
                   getDataSize(), getDataPointer(),
                   rdlb.getDataSize(), rdlb.getDataPointer(), &dbc ) &&
               dbc == DBCOMPARE_EQ;
    }
}



//+---------------------------------------------------------------------------
//
//  Member:     operator<
//
//  Synopsis:   Compare two bookmarks
//

BOOL
CDataLayerBookmark::operator<(const CDataLayerBookmark &rdlb) const
{
    TraceTag((tagDataLayerBookmarkHelper,
              "CDataLayerBookmark::%p < %p", this, &rdlb ));

    IS_VALID((CDataLayerBookmark*)this);
    IS_VALID((CDataLayerBookmark*)&rdlb);

    if (_pRep == rdlb._pRep || rdlb.IsNull())
    {
        return FALSE;
    }
    if (IsNull())
    {
        return TRUE;
    }
    if (_pRep->IsSimple() || rdlb._pRep->IsSimple())
    {
        return !rdlb._pRep->IsSimple() ||
               _pRep->_abData[0] < rdlb._pRep->_abData[0];
    }

    Assert((_pRep->IsVarying() || _pRep->IsULONG()) &&
           (rdlb._pRep->IsVarying() || rdlb._pRep->IsULONG()) );

    Assert(_pRep->_pDataLayerCursor == rdlb._pRep->_pDataLayerCursor);

    const BYTE *pbBookmarkData1 = getDataPointer();
    size_t      cbBookmarkSize1 = getDataSize();
    const BYTE *pbBookmarkData2 = rdlb.getDataPointer();
    size_t      cbBookmarkSize2 = rdlb.getDataSize();
    DBCOMPARE   dbc;

    if (_pRep->_pDataLayerCursor->_pRowsetLocate->Compare(
            _pRep->_pDataLayerCursor->_hChapter,
            cbBookmarkSize1, pbBookmarkData1,
            cbBookmarkSize2, pbBookmarkData2, &dbc ) ||
        dbc == DBCOMPARE_NE || dbc == DBCOMPARE_NOTCOMPARABLE )
    {
        int cmp = memcmp(pbBookmarkData1, pbBookmarkData2,
                         min(cbBookmarkSize1, cbBookmarkSize2) );
        return cmp < 0 || (cmp == 0 && cbBookmarkSize1 < cbBookmarkSize2);
    }
    else
    {
        Assert(dbc == DBCOMPARE_EQ || dbc == DBCOMPARE_LT ||
               dbc == DBCOMPARE_GT );
        return (dbc == DBCOMPARE_LT);
    }
}


#if DBG == 1
//+---------------------------------------------------------------------------
//
//  Member:     IsValidObject
//
//  Synopsis:   Validation method, called by macro IS_VALID(p)
//

BOOL
CDataLayerBookmarkHelper::IsValidObject()
{
    if (!IsNull())
    {
        Assert("_pRep must be exactly one type" &&
               (_pRep->IsSimple()  + _pRep->IsFabricated() +
                _pRep->IsVarying() + _pRep->IsULONG() == 1 ) );
        Assert("Nile and HROW Bookmarks (only) should have cursors" &&
               ((_pRep->IsVarying() || _pRep->IsULONG()) ==
                !!_pRep->_pDataLayerCursor ) );
        Assert("Only two fixed Reps for Simple bookmarks" &&
                (_pRep == &CRep::TheFirst || _pRep == &CRep::TheLast)
                    == _pRep->IsSimple() );
        Assert("Cursors must be active" &&
               (!_pRep->_pDataLayerCursor ||
                _pRep->_pDataLayerCursor->IsActive() ) );
        Assert("Refcounts must not underflow" &&
               (_pRep->_uRefCount != (unsigned long)-1l) );
    }
    return TRUE;
}


BOOL
CDataLayerBookmark::IsValidObject()
{
    super::IsValidObject();
    return TRUE;
}


char s_achCDataLayerBookmarkHelper[] = "CDataLayerBookmarkHelper";
char s_achCDataLayerBookmark[]       = "CDataLayerBookmark";

//+---------------------------------------------------------------------------
//
//  Member:     Dump
//
//  Synopsis:   Dump function, called by macro DUMP(p,dc)
//

void
CDataLayerBookmarkHelper::Dump(CDumpContext&)
{
    TraceTag((tagDataLayerBookmarkHelper, "CDataLayerBookmarkHelper::Dump(%p)",
              this ));

    IS_VALID(this);
}


void
CDataLayerBookmark::Dump(CDumpContext& dc)
{
    TraceTag((tagDataLayerBookmarkHelper, "CDataLayerBookmark::Dump(%p)",
              this ));

    super::Dump(dc);
}


//+---------------------------------------------------------------------------
//
//  Member:     GetClassName
//
//  Synopsis:   GetClassName function. (virtual)
//

char *
CDataLayerBookmarkHelper::GetClassName()
{
    return s_achCDataLayerBookmarkHelper;
}


char *
CDataLayerBookmark::GetClassName()
{
    return s_achCDataLayerBookmark;
}

#endif // DBG == 1
