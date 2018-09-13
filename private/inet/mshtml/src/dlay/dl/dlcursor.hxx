//+----------------------------------------------------------------------------
// Internal
//   DLCursor.HXX   class definition for CDataLayerCursor
//
// Copyright: (c) 1994-1996, Microsoft Corporation
// All rights Reserved.
// Information contained herein is Proprietary and Confidential.
//
// Contents:
//      this header defines the basic functinoallity needed to communicate with
//      a cursor
//      related header files are: templmgr.hxx
//
//

#ifndef I_DLCURSOR_HXX_
#define I_DLCURSOR_HXX_
#pragma INCMSG("--- Beg 'dlcursor.hxx'")

#ifndef X_NDUMP_HXX_
#define X_NDUMP_HXX_
#include "ndump.hxx"
#endif

#ifndef X_DBLLINK_HXX_
#define X_DBLLINK_HXX_
#include "dbllink.hxx"
#endif

#ifndef X_OLEDB_H_
#define X_OLEDB_H_
#pragma INCMSG("--- Beg <oledb.h>")
#include <oledb.h>
#pragma INCMSG("--- End <oledb.h>")
#endif

#ifndef X_OLEDBTRI_H_
#define X_OLEDBTRI_H_
#pragma INCMSG("--- Beg <oledbtri.h>")
#include <oledbtri.h>
#pragma INCMSG("--- End <oledbtri.h>")
#endif

// Class forwarding declaration macro.
#define DEFINE_CLASS(T) class C##T; typedef C##T* PC##T;

DEFINE_CLASS(DataLayerBookmark);
DEFINE_CLASS(DataLayerChapter);
DEFINE_CLASS(DataLayerCursor);


//+----------------------------------------------------------------------------
//
// CDataLayerCursorEvents - Event sink used for detecting changes to the
//                                  underlying data of a rowset.
//
//    cRows == 0 and ahRows == NULL imply that all rows were affected
//

class CDataLayerCursorEvents
{
public:
    virtual HRESULT AllChanged() = 0;
    virtual HRESULT RowsChanged(DBCOUNTITEM cRows, const HROW *ahRows) = 0;
    virtual HRESULT FieldsChanged(HROW hRow, DBORDINAL cColumns, DBORDINAL aColumns[]) = 0;
    virtual HRESULT RowsInserted(DBCOUNTITEM cRows, const HROW *ahRows) = 0;
    virtual HRESULT DeletingRows(DBCOUNTITEM cRows, const HROW *ahRows) = 0;
    virtual HRESULT RowsDeleted(DBCOUNTITEM cRows, const HROW *ahRows) = 0;
    virtual HRESULT DeleteCancelled(DBCOUNTITEM cRows, const HROW *ahRows) = 0;
    virtual HRESULT OnNileError(HRESULT hr, BOOL fSupportsErrorInfo);
    virtual HRESULT RowsAdded() = 0;
    virtual HRESULT PopulationComplete() = 0;
};



//+----------------------------------------------------------------------------
//
//  Class CDataLayerBookmarkHelper
//
//  Purpose:
//      Only used for CDataLayerBookmark's
//      The real interfaces which users should see are there.
//
//  This class supports very inexpensive value based semantics.  It is small
//      and reference counts, so assignments and copy construction are cheap.
//
//  Created by tedsmith
//

class CDataLayerBookmarkHelperRep; // Private representation type

class CDataLayerBookmarkHelper
{
    friend class CDataLayerBookmark;
    friend class CDataLayerBookmarkHelperRep;

private: // Keep others from using this class
    CDataLayerBookmarkHelper() : _pRep(0) {}
    CDataLayerBookmarkHelper(const CDataLayerBookmarkHelper &);

    // Iff out of memory the resultant Bookmark/Chapter is null (use IsNull!)

    // For using DBBMK_FIRST and DBBMK_LAST, for example.
    CDataLayerBookmarkHelper(const BYTE &dbbmk);

    CDataLayerBookmarkHelper(CDataLayerCursor &, const DBVECTOR &);
    CDataLayerBookmarkHelper(CDataLayerCursor &, const ULONG &);

#if DBG == 1
    ~CDataLayerBookmarkHelper();
    void Trace(LPSTR pstr);
#endif
    CDataLayerBookmarkHelper &operator=(const CDataLayerBookmarkHelper &);
    BOOL   IsNull()        const { return !_pRep; }
    BOOL   IsDBBMK_FIRST() const;

    BOOL   IsDBBMK_LAST()  const;
    const BYTE *getDataPointer() const;
    size_t getDataSize()         const;
    HROW   getHRow()             const;
    HCHAPTER getHChapter()       const;

    CDataLayerBookmarkHelperRep *_pRep;
    void Unlink();

public:
    DEBUG_METHODS
};



//+----------------------------------------------------------------------------
//
//  Class CDataLayerBookmark
//
//  Purpose:
//
//  This class supports very inexpensive value based semantics.  It is small
//      and reference counts, so assignments and copy construction are cheap.
//
//  Created by tedsmith
//
//  Hungarian: dlb, pdlb
//

MtExtern(CDataLayerBookmark)

class CDataLayerBookmark : public CDataLayerBookmarkHelper
{
typedef CDataLayerBookmarkHelper super;

public:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CDataLayerBookmark))
    CDataLayerBookmark() : super() {}

    CDataLayerBookmark(const CDataLayerBookmark &rdlb) : super(rdlb) {}

    // Iff we run out of memory the resultant Bookmark is null (use IsNull!)

    CDataLayerBookmark(CDataLayerCursor &, const DBVECTOR &);
    CDataLayerBookmark(CDataLayerCursor &, const ULONG &);

    ~CDataLayerBookmark()        { Passivate(); }
    BOOL   IsNull()        const { return super::IsNull(); }
    BOOL   IsDBBMK_FIRST() const { return super::IsDBBMK_FIRST(); }
    BOOL   IsDBBMK_LAST()  const { return super::IsDBBMK_LAST(); }
    size_t getDataSize()   const { return super::getDataSize(); }
    const BYTE *getDataPointer() const { return super::getDataPointer(); }

    void Passivate()
    {
        if (!super::IsNull())
        {
            *this = CDataLayerBookmark::TheNull;
        }
    }

    CDataLayerBookmark &operator=(const CDataLayerBookmark &rdlb)
    {
        super::operator=(rdlb);
        return *this;
    }

    BOOL operator==(const CDataLayerBookmark &rdlb) const;

    BOOL operator!=(const CDataLayerBookmark &rdlb) const
    {
        return !operator==(rdlb);
    }

    BOOL operator<(const CDataLayerBookmark &rdlb) const;

    static const CDataLayerBookmark TheNull;
    static const CDataLayerBookmark TheFirst;
    static const CDataLayerBookmark TheLast;

    DEBUG_METHODS

private:
    // Callers should use TheFirst or TheLast, rather than creating a new
    //   fixed bookmark.
    CDataLayerBookmark(const BYTE &);
};


inline CDataLayerBookmark::CDataLayerBookmark(CDataLayerCursor &rdlc,
                                              const DBVECTOR &rdbv ) :
    super(rdlc, rdbv) {}


inline CDataLayerBookmark::CDataLayerBookmark(CDataLayerCursor &rdlc,
                                              const ULONG &rul ) :
    super(rdlc, rul) {}


inline CDataLayerBookmark::CDataLayerBookmark(const BYTE &dbbmk) :
    super(dbbmk) {}


///////////////////////////////////////////////////////////////////////////////
//
//  CDataLayerNotify    subobject of CDataLayerCursor that implements
//                      notification interfaces.  This is needed to
//                      avoid circular reference counts.

MtExtern(CDataLayerNotify);

class CDataLayerNotify : public IRowsetNotify, IDBAsynchNotify
{
    friend class CDataLayerCursor;

public:
    //  IUnknown members
    STDMETHODIMP            QueryInterface (REFIID riid, LPVOID *ppv);
    STDMETHODIMP_(ULONG)    AddRef ();
    STDMETHODIMP_(ULONG)    Release ();

    //  IRowsetNotify members
    virtual HRESULT STDMETHODCALLTYPE OnFieldChange(
        IRowset *pRowset,
        HROW hRow,
        DBORDINAL cColumns,
        DBORDINAL rgColumns[  ],
        DBREASON eReason,
        DBEVENTPHASE ePhase,
        BOOL fCantDeny);

    virtual HRESULT STDMETHODCALLTYPE OnRowChange(
        IRowset *pRowset,
        DBCOUNTITEM cRows,
        const HROW rghRows[  ],
        DBREASON eReason,
        DBEVENTPHASE ePhase,
        BOOL fCantDeny);

    virtual HRESULT STDMETHODCALLTYPE OnRowsetChange(
        IRowset *pRowset,
        DBREASON eReason,
        DBEVENTPHASE ePhase,
        BOOL fCantDeny);

    // IDBAsynch members
    virtual HRESULT STDMETHODCALLTYPE OnLowResource( 
            /* [in] */ DB_DWRESERVE dwReserved);
        
    virtual HRESULT STDMETHODCALLTYPE OnProgress( 
            /* [in] */ HCHAPTER hChapter,
            /* [in] */ DBASYNCHOP ulOperation,
            /* [in] */ DBCOUNTITEM ulProgress,
            /* [in] */ DBCOUNTITEM ulProgressMax,
            /* [in] */ DBASYNCHPHASE eAsynchPhase,
            /* [in] */ LPOLESTR pwszStatusText);
        
    virtual HRESULT STDMETHODCALLTYPE OnStop( 
            /* [in] */ HCHAPTER hChapter,
            /* [in] */ ULONG ulOperation,
            /* [in] */ HRESULT hrStatus,
            /* [in] */ LPOLESTR pwszStatusText);

private:
    CDataLayerNotify(CDataLayerCursorEvents *pDLCEvents, DWORD dwTID):
        _pDLCEvents(pDLCEvents),
        _dwTID(dwTID)
        { }
    
    CDataLayerCursor * MyDLC();

    // Check that all callbacks are proper apartment model
    HRESULT CheckCallbackThread();

    // ptr to an interface on which we can fire notification events:
    CDataLayerCursorEvents  *_pDLCEvents;
    DWORD                    _dwTID;    // thread ID on which we're created
};


///////////////////////////////////////////////////////////////////////////////
//
//  CDataLayerCursor   basic cover class for OLEDB cursors
//
//  Purpose:
//
//
//  created by frankman
//      11/02/94
//
//  Hungarian:
//      oleC, poleC
//

MtExtern(CDataLayerCursor);

class CDataLayerCursor
{
    friend class CDataLayerNotify;
    friend class CDataLayerSearch;
    friend class CDataLayerBookmarkHelperRep;
    friend class CDataLayerBookmarkHelper;
    friend class CDataLayerBookmark;
    friend class CTableConsumer;
    friend class CCurrentRecordInstance;

private:
    // constructor, destructor, operators are private
    // (We should only exist in a CDataLayerCursorEvents,
    // CTableConsumer, or CCurrentRecordInstance)
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CDataLayerCursor))
    CDataLayerCursor(CDataLayerCursorEvents *pDLCEvents);
#if DBG == 1
    ~CDataLayerCursor();
#endif
    HRESULT Init(IUnknown *pUnkRowset, HCHAPTER hchapter); // Initialization call
    HRESULT CacheColumnInfo(IUnknown *pUnkRowset);
    HRESULT FetchRowsetIdentity();      // helper function for Init
    void    Passivate();       // Clean up cursor (back to pre-Init state)
public:
    BOOL    IsActive() const { return !!_pRowsetLocate; }
    BOOL    IsComplete() const { return !!_fComplete; }

        // This enum is used as a bitfield mask.
        // e.g. if (Cursor->GetCapabilities() & RCOrdinalIndex) ...
    enum RowsetCapabilities
    {
        RCScrollable   = 0x00000001l, // Supports approximate positioning
        RCOrdinalIndex = 0x00000002l, // Supports exact positioning
        RCAsynchronous = 0x00000004l // "HROWs come and go at random"
    };

    RowsetCapabilities GetCapabilities() const
    {
        return (RowsetCapabilities)_rcCapabilities;
    }

    BOOL IsSameRow(HROW hrow1, HROW hrow2) const
    {
        // note that unlike NILE, we return TRUE for two NULLs.
        // datadocs relies on this behavior.
        BOOL fRet = hrow1 == hrow2 ||
                    (_pRowsetIdentity &&
                     !_pRowsetIdentity->IsSameRow(hrow1, hrow2) );
        return fRet;
    }

    // public operations - manipulating HROWS

    //  There are two sets of interfaces, Bookmark oriented and ordinal.
    //  There are implementation dependant inneficiencies when using the
    //    wrong interfaces.  Use GetCapabilities to see if ordinal access
    //    is supported.

        // Creates and returns a bookmark coresponding to the given row
    HRESULT CreateBookmark(HROW hRow, CDataLayerBookmark *pdlb );

        // Creates and returns a new hRow just after the specified bookmark
        // BUGBUG:  DBBMK_LAST (for append) is the only bookmark supported
        //          with DIRT now (everything works with STD)
    HRESULT NewRowAfter(const CDataLayerBookmark &rdlb, HROW *pHRow);

    HRESULT GetRowsAt(const CDataLayerBookmark &rdlb,
                      DBROWOFFSET iOffset, DBROWCOUNT iRows, DBCOUNTITEM *puFetchedRows,
                      HROW *pHRows );

    HRESULT GetRowAt(const CDataLayerBookmark &rdlb, HROW *pHRow);

    void AddRefRows(int ulcb, HROW *pHRows);
    void ReleaseRows(int ulcb, HROW *pHRows);

    HRESULT GetData(HROW hRow, HACCESSOR hAccessor, void * pData,
                    BOOL fOriginal = FALSE);
    HRESULT SetData(HROW hRow, HACCESSOR hAccessor, void * pData);

    HRESULT Update();
    HRESULT Update(HROW);
    HRESULT Undo();
    HRESULT Undo(HROW);
    HRESULT GetRowStatus(HROW, LONG *);

    HRESULT DeleteRows(size_t uRows, HROW *pHRow);

    HRESULT DeleteAllRows();

    // NULL's are accepted and may be more efficient
    HRESULT GetPositionAndSize(HROW hRow,
                               DBCOUNTITEM *puPosition, DBCOUNTITEM *puChapterSize );
    HRESULT GetPositionAndSize(const CDataLayerBookmark &rdlb,
                               DBCOUNTITEM *puPosition, DBCOUNTITEM *puChapterSize );
    HRESULT GetSize(DBCOUNTITEM *puChapterSize);

    // column information functions
    struct ColumnInfo                // subset of DBCOLUMNINFO to be cached
    {
        LPTSTR pwszName;
        ULONG iNumber;
        DBTYPE dwType;
        ULONG cbMaxLength;
        DBCOLUMNFLAGS dwFlags;
    };

    HRESULT GetPColumnInfo(DBORDINAL ulColumnNum, const ColumnInfo **ppColumnInfo);
    HRESULT GetColumnCount(DBORDINAL &cColumns);
    HRESULT GetColumnNumberFromName(LPCTSTR pstrName, DBORDINAL &ulColumnNum);
    HRESULT GetColumnNameFromNumber(DBORDINAL ulColumnNum, LPCTSTR *ppstrName);

    // Accessor wrappers
    HRESULT CreateAccessor(HACCESSOR &rhAccessor, DBACCESSORFLAGS dwAccFlags,
                           const DBBINDING rgBindings[], int ulcb );
    void    ReleaseAccessor(HACCESSOR &rhAccessor);

    void    SetDLCEvents(CDataLayerCursorEvents *pDLCEvents)
                { _DLNotify._pDLCEvents = pDLCEvents; }

    //  reference counting
    STDMETHODIMP_(ULONG)    AddRef ();
    STDMETHODIMP_(ULONG)    Release ();
    ULONG                   SubAddRef();
    ULONG                   SubRelease();

    // Check that all callbacks are proper apartment model
    HRESULT CheckCallbackThread() { return _DLNotify.CheckCallbackThread(); }

    // Notification helpers
    HRESULT FilterRowsToChapter(
        DBROWCOUNT cRows,
        const HROW rghRows[  ],
        DBROWCOUNT *pcRows,
        const HROW **prghRows
        );
    
    DEBUG_METHODS

private:
    ULONG                    _ulRefs;
    ULONG                    _ulAllRefs;
    CDataLayerNotify         _DLNotify;         // sub-object for notifications
    
    IAccessor               *_pAccessor;
    IColumnsInfo            *_pColumnsInfo;
    IRowsetChange           *_pRowsetChange;
    IRowsetExactScroll      *_pRowsetExactScroll;
    IRowsetLocate           *_pRowsetLocate;
    IRowsetNewRowAfter      *_pRowsetNewRowAfter;
    IRowsetFind             *_pRowsetFind;
    IRowsetScroll           *_pRowsetScroll;
    IRowsetUpdate           *_pRowsetUpdate;
    IDBAsynchStatus         *_pAsynchStatus;
    ISupportErrorInfo       *_pSupportErrorInfo;
    IRowsetIdentity         *_pRowsetIdentity;   // only set if not natural ID
    IRowsetChapterMember    *_pRowsetChapterMember;
    DBORDINAL                _cColumns;
    ColumnInfo              *_adlColInfo;
    TCHAR                   *_cStrBlk;
    DBROWCOUNT               _uChapterSize;         // Cached rowset size
    HACCESSOR                _hBookmarkAccessor;
    HACCESSOR                _hNullAccessor;        // For creating empty rows
    int                      _rcCapabilities;
    IConnectionPoint        *_pcpRowsetNotify;   // For IRowsetNotify
    IConnectionPoint        *_pcpDBAsynchNotify;  // For IDBAsynchNotify
    DWORD                    _wAdviseCookieRowsetNotify;
    DWORD                    _wAdviseCookieDBAsynchNotify;
    unsigned                 _fNewRowsSinceLastAsynchRatioFinishedCall : 1;
    unsigned                 _fFixedSizedBookmark                      : 1;
    unsigned                 _fDeleteAllInProgress                     : 1;
    unsigned                 _fComplete                                : 1;

    // For hierarchy, restrict all references to this chapter
    HCHAPTER                _hChapter;

    HRESULT InitBookmarkAccessor();
    HRESULT HandleError(REFIID, HRESULT);
    NO_COPY(CDataLayerCursor);

};

// wrapping macros for Nile method calls, handle error notifications
#define DL_THR(intf, call)  THR(HandleError(IID_I##intf, _p##intf->call))
#define DL_VERIFY_OK(intf, call)    \
                            Verify(!HandleError(IID_I##intf, _p##intf->call))

// inlines

inline CDataLayerCursor *
CDataLayerNotify::MyDLC()
{
    return CONTAINING_RECORD(this, CDataLayerCursor, _DLNotify);
}

///////////////////////////////////////////////////////////////////////////////

#endif  // _DLCURSOR_HXX_
