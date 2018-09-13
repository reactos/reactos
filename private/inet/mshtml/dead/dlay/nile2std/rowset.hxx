//+----------------------------------------------------------------------------
// rowset.hxx:
//              CImpIRowset
//
// Copyright: (c) 1994-1995, Microsoft Corporation
// Information contained herein is Proprietary and Confidential.
//
// Contents: this file contains the above refered to definitions.
//
//  History:
//  07/18/95    TerryLu     Creation of Nile provider implementation.
//  07/24/95    TedSmith    Converted to multiple inheritance.
//  07/28/95    TerryLu     Removed pUnkOuter references (CImpIRowset will not
//                          support aggregation for now.  Change ACCESSOR to
//                          InternalAccessor and other minor cleanup.
//  08/14/95    TerryLu     Added CBase derivation.
//

#ifndef _ROWSET_HXX_
#define _ROWSET_HXX_

#include <oledb.h>
#include <oledberr.h>

#ifndef __std_h__
#include <simpdata.h>
#include "MStdWrap.h"
#endif  // __std_h__

#ifndef _DBLLINK_HXX_
#   include <dbllink.hxx>
#endif  // _DBLLINK_HXX_

#include "cdbase.hxx"
#include "objcache.hxx"

#include "rowhndl.hxx"

#include "..\include\oledbtmp.hxx"

#if DBG == 1
extern TAG tagNileRowsetProvider;
#endif

// BUGBUG: We are hoping Nile defines this
const int DBCOMPAREOPS_NOTPARTIALEQ = 0x100;

//+----------------------------------------------------------------------------
//
//  Class CSTDColumnInfo
//
//  Purpose:
//      Subset of DBCOLUMNINFO structure used to cache some of its data.
//

class CSTDColumnInfo
{
public:
    DBTYPE dwType;
    ULONG cbMaxLength;
    DBCOLUMNFLAGS dwFlags;
};


//+----------------------------------------------------------------------------
//
//  Class CDBProperties
//
//  Purpose:
//      Maintenance of properties and their values
//

class CDBProperties
{
public:
    CDBProperties();
    ~CDBProperties();
    DBPROPSET*			GetPropertySet(const GUID& guid) const;
    HRESULT             CopyPropertySet(const GUID& guid, DBPROPSET* pPropSetDst) const;
    HRESULT             CopyPropertySet(ULONG iPropSet, DBPROPSET* pPropSetDst) const
        { return CopyPropertySet(_aPropSets[iPropSet].guidPropertySet, pPropSetDst); }
    const DBPROP*		GetProperty(const GUID& guid, DBPROPID id) const;
    HRESULT				SetProperty(const GUID& guid, const DBPROP& prop);
    ULONG               GetNPropSets() const { return _cPropSets; }
private:
	ULONG		_cPropSets;			// number of property sets
	DBPROPSET*	_aPropSets;			// array of property sets
	
	NO_COPY(CDBProperties);
};


//+----------------------------------------------------------------------------
//
//  Class CImpIRowset
//
//  Purpose:
//      Nile IRowset provider
//
//  Currently contains:
//      IRowset:
//          AddRefRows
//          GetData
//          GetNextRows
//          ReleaseChapter
//          ReleaseRows
//          RestartPosition
//      IRowsetInfo:
//          GetProperties
//          GetReferencedRowset - not done
//          GetSpecification
//      IRowsetLocate:
//          Compare
//          GetRowsAt
//          GetRowsByBookmark - not done
//          Hash - not done
//      IAccessor:
//          CreateAccessor
//          GetBindings
//          ReleaseAccessor
//      IColumnsInfo:
//          GetColumnInfo
//          MapColumnIDs
//      IRowsetChange:
//          SetData
//          DeleteRows
//      IRowsetScroll:
//          GetApproximatePosition
//          GetRowsAtRatio
//      IRowsetExactScroll:
//          GetExactPosition
//      IRowsetNewRow:
//          SetNewData
//      IRowsetNewRowAfter:
//          SetNewDataAfter
//      IRowsetIdentity:
//          IsSameRow
//      IRowsetFind:            -- removed due to spec drift cfranks 8 May 1997
//          GetRowsByValues
//      IConvertType:
//          CanConvert
//      INile2STDFastBookmark:
//          GetNileI4Bookmark
//          IsClonedHRowDeleted
//          IsDeleteInProgress 
//
//
//
//  NOTE:  To add a new interface:
//
//           1)  Add the new interface as a public base class of CImpIRowset.
//               Add the most derived interface in a chain only.  e.g.
//               IRowsetExactScroll represents the
//               IRowsetExactScroll::IRowsetScroll::IRowsetLocate::IRowset
//               chain.
//           2)  Add ALL methods of that interface to CImpIRowset.
//           3)  In CImpIRowset::QueryInterface (in rowset.cxx) add a new TEST
//               line.  (You'll understand when you get there.)
//           4)  It would be nice if you would update the comment above for
//               other's quick reference.  Please keep the "- not done"'s
//               correct as well.
//           5)  I suspect it would be helpful to keep IRowsetExactScroll first.
//               No other ordering matters, except that you need to be very
//               carefull to rebuild everything when you change this.
//


class CImpIRowset : public CBase, public IRowsetExactScroll, public IRowsetInfo,
                    public IAccessor, public IColumnsInfo, public IRowsetChange,
                    public IRowsetNewRowAfter,
#ifdef NEVER
                    public IRowsetFind,
#endif
                    public IRowsetIdentity, public IConvertType,
                    public IRowsetAsynch
{
    CImpIRowset();
    void *          operator new(size_t cb) { return MemAllocClear(cb); }
public:
    static HRESULT CreateRowset(LPSIMPLETABULARDATA pSTD, IUnknown **ppUnk);

    //
    // IUnknown members
    //
    STDMETHOD(PrivateQueryInterface)(REFIID riid, void ** ppv);
    DECLARE_FORMS_DELEGATING_IUNKNOWN(CImpIRowset)

    //
    // CBase members
    //
    virtual const CBase::CLASSDESC *GetClassDesc() const;
    void Passivate();

    //  IRowset members
    STDMETHODIMP            AddRefRows(ULONG, const HROW [  ], ULONG[],
                                        DBROWSTATUS [  ]);
    STDMETHODIMP            GetData(HROW, HACCESSOR, void *);
    STDMETHODIMP            GetNextRows(HCHAPTER, LONG, LONG, ULONG*, HROW**);
    STDMETHODIMP            ReleaseRows(ULONG, const HROW[], DBROWOPTIONS[],
                                        ULONG[], DBROWSTATUS[]);
    STDMETHODIMP            RestartPosition(HCHAPTER);

    // IRowsetInfo members
    STDMETHODIMP            GetProperties(const ULONG, 
                                            const DBPROPIDSET[  ],
                                            ULONG *,
                                            DBPROPSET  **);
    STDMETHODIMP            GetReferencedRowset(ULONG, REFIID, IUnknown**);
    STDMETHODIMP            GetSpecification(REFIID, IUnknown**);
    

    //  IAccessor members
    STDMETHODIMP            AddRefAccessor(HACCESSOR hAccessor, ULONG *pcRefCount);
    STDMETHODIMP			CreateAccessor(DBACCESSORFLAGS, ULONG,
    									const DBBINDING[], ULONG, HACCESSOR*,
    									DBBINDSTATUS[]);
    STDMETHODIMP            GetBindings(HACCESSOR, DBACCESSORFLAGS*,
                                        ULONG*, DBBINDING** );
    STDMETHODIMP            ReleaseAccessor(HACCESSOR hAccessor, ULONG *pcRefCount);
    
    //  IColumnsInfo members
    STDMETHODIMP            GetColumnInfo(ULONG*, DBCOLUMNINFO**, OLECHAR**);
    STDMETHODIMP            MapColumnIDs(ULONG, const DBID[], ULONG[]);

    //  IRowsetChange members
    STDMETHODIMP            DeleteRows(HCHAPTER, ULONG, const HROW[],
                                       DBROWSTATUS[]);
    STDMETHODIMP            SetData(HROW, HACCESSOR, void*);
    STDMETHODIMP            InsertRow (HCHAPTER    hChapter,
                                       HACCESSOR   hAccessor,
                                       void       *pData,
                                       HROW       *phRow );

    //  IRowsetLocate members
    STDMETHODIMP            Compare(HCHAPTER hChapter,
                                    ULONG cbBookmark1, const BYTE *pBookmark1,
                                    ULONG cbBookmark2, const BYTE *pBookmark2,
                                    DBCOMPARE *pdwComparison );
    STDMETHODIMP            GetRowsAt(HWATCHREGION hRegion,
                                      HCHAPTER hChapter,
                                      ULONG cbBookmark,  const BYTE *pBookmark,
                                      LONG  lRowsOffset,     LONG  cRows,
                                      ULONG *pcRowsObtained, HROW **pahRows );
    STDMETHODIMP            GetRowsByBookmark(HCHAPTER    hChapter,
                                              ULONG       cRows,
                                              const ULONG acbBookmarks[],
                                              const BYTE *apBookmarks[],
                                              HROW        ahRows[],
                                              DBROWSTATUS aRowStatus[] );
    STDMETHODIMP            Hash(HCHAPTER    hChapter,
                                 ULONG       cBookmarks, const ULONG acbBookmarks[],
                                 const BYTE *apBookmarks[],
                                 DWORD       aHashedValues[],
                                 DBROWSTATUS aBookmarkStatus[] );

    //  IRowsetScroll members
    STDMETHODIMP            GetApproximatePosition(HCHAPTER    hChapter,
                                                   ULONG       cbBookmark,
                                                   const BYTE *pBookmark,
                                                   ULONG      *pulPosition,
                                                   ULONG      *pcRows );
    STDMETHODIMP            GetRowsAtRatio(HWATCHREGION hRegion,
                                           HCHAPTER    hChapter,
                                           ULONG       ulNumerator,
                                           ULONG       ulDenominator,
                                           LONG        cRows,
                                           ULONG      *pcRowsObtained,
                                           HROW      **pahRows );

    //  IRowsetExactScroll members
    STDMETHODIMP            GetExactPosition(HCHAPTER    hChapter,
                                             ULONG       cbBookmark,
                                             const BYTE *pBookmark,
                                             ULONG      *pulPosition,
                                             ULONG      *pcRows );

    //  IRowsetNewRowAfter member
    STDMETHODIMP            SetNewDataAfter(HCHAPTER    hChapter,
                                            ULONG       cbBookmarkPrevious,
                                            const BYTE *pBookmarkPrevious,
                                            HACCESSOR   hAccessor,
                                            BYTE       *pData,
                                            HROW       *phRow );

    //  IRowsetIdentity member
    STDMETHODIMP            IsSameRow(HROW  hThisRow,
                                      HROW  hThatRow);

#ifdef NEVER
    //  IRowsetFind member
    STDMETHODIMP            GetRowsByValues(HCHAPTER      hChapter,
                                            ULONG         cbBookmark,
                                            const BYTE   *pBookmark,
                                            LONG          lRowsOffset,
                                            ULONG         cValues,
                                            const ULONG   aColumns[],
                                            const DBTYPE  aValueTypes[],
                                            const BYTE   *aValues[],
                                            const DBCOMPAREOPS aCompareOps[],
                                            LONG          cRows,
                                            ULONG        *pcRowsObtained,
                                            HROW        **pahRows );
#endif

    //  IRowsetAsynch members
    STDMETHODIMP            RatioFinished(ULONG         *pulDenominator,
                                          ULONG         *pulNumerator,
                                          ULONG         *pcRows,
                                          BOOL          *pfNewRows);
        
    STDMETHODIMP            Stop( void);

    // IConnectionPointContainer members
    STDMETHODIMP            EnumConnectionPoints(
                                                IEnumConnectionPoints **ppEnum);
    STDMETHODIMP            FindConnectionPoint(REFIID riid,
                                                IConnectionPoint ** ppCP);
    // IConvertType members
    STDMETHODIMP            CanConvert(DBTYPE wFromType,
                                       DBTYPE wToType,
                                       DBCONVERTFLAGS dwConvertFlags);

    // INile2STDFastBookmark members
    virtual ULONG           GetNileI4Bookmark(HROW hRow);
    virtual BOOL            IsClonedHRowDeleted(HROW hRow);
    virtual BOOL            IsDeleteInProgress();

    // Event firing helper functions:
#if defined(PRODUCT_97)
    HRESULT     FireChapterEvent(HCHAPTER hChapter, DBREASON eReason);
#endif
    HRESULT     FireFieldEvent(HROW hRow, ULONG cColumns, ULONG aColumns[],
                               DBREASON eReason, DBEVENTPHASE ePhase);
    HRESULT     FireRowEvent(ULONG cRows, const HROW rghRows[],
                             DBREASON eReason, DBEVENTPHASE ePhase);
    HRESULT     FireRowsetEvent(DBREASON eReason, DBEVENTPHASE ePhase);

    // Misc. help functions.
    HRESULT     Init(ISimpleTabularData *pSTD);
    HRESULT     CacheMetaData();
    IUnknown   *getpIUnknown() const;

protected:
    static const CBase::CLASSDESC       s_classdesc;// IRowset classDesc
    static const CONNECTION_POINT_INFO  s_acpi[];   // IRowsetNotify conn. pt.
    
    //
    // embedded class for ISimpleTabularDataEvents sink
    //
    class MySTDEvents: public ISimpleTabularDataEvents
    {
        public:
            //  IUnknown members
            DECLARE_FORMS_SUBOBJECT_IUNKNOWN(CImpIRowset);

            //  ISimpleTabularDataEvents members
            STDMETHODIMP    CellChanged(LONG iRow, LONG iColumn);
            STDMETHODIMP    DeletedRows(LONG iRow, LONG cRows);
            STDMETHODIMP    InsertedRows(LONG iRow, LONG cRows);
            STDMETHODIMP    DeletedColumns(LONG iColumn, LONG cColumns);
            STDMETHODIMP    InsertedColumns(LONG iColumn, LONG cColumns);
            STDMETHODIMP    AsyncRowsArrived(LONG iRow, LONG cRows);
            STDMETHODIMP    PopulationComplete();
        private:
            STDMETHODIMP    InsertedRowsHelper(LONG iRow, LONG cRows, DBREASON eReason);
    } _STDEvents;
    friend class MySTDEvents;

private:
    // Internal accessor structure
    struct AccessorFormat: public CIntrusiveDblLinkedListNode
    {
        void* operator new(size_t, void* p) { return p; }
        AccessorFormat(ULONG RefCount, DBACCESSORFLAGS AccFlags, ULONG Bindings):
            cRefCount(RefCount), dwAccFlags(AccFlags), cBindings(Bindings) {}
        
        ULONG     cRefCount;
        DBACCESSORFLAGS  dwAccFlags;
        ULONG     cBindings;
        DBBINDING aBindings[1];
    };
    TIntrusiveDblLinkedList<AccessorFormat> _dblAccessors;  // active accessors

    // Information needed to set or get data and coerce it to the accessor type
    // from the database type.
    struct XferInfo
    {
        DWORD       dwDBType;           // Database type
        DWORD       dwAccType;          // Accessor type
        void        *pData;             // Location of data to set or get
        ULONG       *pulXferLength;     // Number of bytes transfered
        ULONG       ulDataMaxLength;    // Size of pData in bytes
        DWORD       *pdwStatus;         // Status of transfer
    };

    // New scheme for mapping hRows and Bookmarks to STD rows
    CRowMap     _rowmap;                // an embedded CRowMap
    ULONG       _GetNextCursor;         // for iRowSet:GetNextRows

    // IRowset state
    ISimpleTabularData      *_pSTD;         // Simple tabular data provider
    CSTDColumnInfo          *_astdcolinfo;  // Cached ColumnInfo array
    ULONG                   _cSTDRows;      // rows in STD
    ULONG                   _cSTDCols;      // columns in STD
    ULONG                   _cRowsVirtual;  // number of virtual rows;
                                            //  > _cSTDRows if del in progress
    BOOL                    _fPopulationComplete; // async row population done?

	// Properties
	CDBProperties			_dbpProperties;	// properties

    // special handling for delete in progress
    // Unfortunately, ISimpleTabularData does not provide for any notification
    //  before delete or insert takes place.  (Excel is unable to generate such
    //  notification.)  So, at least for deletion, we simulate Nile pre- and
    //  post- notifications.  During pre-delete notifications, we act, to the
    //  greatest extent possible, as if no deletion has yet taken place.
    //  Bookmarks refer to the same row-contents they would have referenced
    //  before the notification, as do HROWs.  HROWs corresponding to the rows
    //  being deleted may be used to fetch the old bookmarks, but not fetch the
    //  old data (which the STD has already discarded).  Bookmarks referring
    //  to the actual rows being deleted will behave as if the rows have already
    //  been deleted though.
    //
    // In support of this behavior, we keep track of delete notifications in
    //  progress.  The data structure in theory support nested deletions, but in
    //  fact, the code only handle one delete-in-progress at a time.
    class CDeleteInProgress
    {
    public:
        class CDeleteInProgressNode
        {
        public:
            ULONG _index;                   // STD index: 1st row being deleted
            ULONG _cRows;                   // number rows being deleted.
            CDeleteInProgressNode *_pDipn;  // allows for re-entrancy, although
                                            //    we only detect it and fail
                                            //    certain operations.

            CDeleteInProgressNode(ULONG index,
                                    ULONG cRows,
                                    CDeleteInProgress &rDip)
                : _index(index), _cRows(cRows), _pDipn(rDip._pDipn)
            {
                Assert(_cRows != 0);
                rDip._pDipn = this;
            }

            ~CDeleteInProgressNode()
            {
                Assert("DeleteInProgressNode not unlinked" && _cRows == 0);
            }

        };

        CDeleteInProgressNode *_pDipn;  // head of linked-list of nested delete
                                        //  notfication info; most recently
                                        //  entered notification is at head of
                                        //  list.

        CDeleteInProgress() : _pDipn(NULL) {}

        void Unlink(CDeleteInProgressNode &rDipn)
        {
            Assert(_pDipn == &rDipn);
            _pDipn = rDipn._pDipn;
#if DBG == 1
            rDipn._cRows = 0;
#endif
        }

        BOOL FDeleteInProgress()    { return _pDipn != NULL; }

        ~CDeleteInProgress()
        {
            Assert("Destruction w/delete in progress" && !FDeleteInProgress());
        }

        HRESULT IndexToHRowNumber(ULONG index, ULONG &rulRow);
        HRESULT HRowNumberToIndex(ULONG ulRow, BOOL fDeleted,
                                ULONG &rindex, BOOL fAdjustBackward = FALSE);
    };

    CDeleteInProgress _Dip;

    // Notification reentrancy stuff
#   define DBEVENTPHASE_MAX ((DWORD) DBEVENTPHASE_DIDEVENT)
#   define DBREASON_MAX     ((DWORD) DBREASON_POPULATION_STOPPED) // for now
    typedef unsigned char   NOTIFY_STATE;

    NOTIFY_STATE           _aNotifyInProgress[DBREASON_MAX + 1];

    inline BOOL FAnyPhaseInProgress(DBREASON reason)
    {
        Assert(reason <= DBREASON_MAX);
        return _aNotifyInProgress[reason] != 0;
    };
    inline NOTIFY_STATE EnterNotify(DBREASON reason, DBEVENTPHASE phase)
    {
        NOTIFY_STATE ns = _aNotifyInProgress[reason];
        Assert(reason <= DBREASON_MAX);
        Assert(phase <= DBEVENTPHASE_MAX);

        _aNotifyInProgress[reason] |= (1 << phase);
        return (ns);
    };
    inline void LeaveNotify(DBREASON reason, DBEVENTPHASE, NOTIFY_STATE ns)
    {
        Assert(reason <= DBREASON_MAX);
        _aNotifyInProgress[reason] = ns;
    };


    // Local helper routines
    static void FastVariantInit (VARIANTARG *pVar);

    enum DATA_DIRECTION {DataFromProvider, DataToProvider};
    HRESULT DataToWSTR (DATA_DIRECTION dDirection,
                        ULONG uRow, ULONG uCol, XferInfo & xferData);
    void    DataCoerce_STR (ULONG uRow, ULONG uCol,
                            BOOL fBSTR, XferInfo & xferData );
    HRESULT DataCoerce (DATA_DIRECTION dDirection,
                        ULONG uRow, ULONG uCol, XferInfo & xferData);

    static HRESULT WhineAboutChapters(HCHAPTER hChapter);
    HRESULT Bookmark2HRowNumber(ULONG cbBookmark, const BYTE *pBookmark,
                                    ULONG &rulRow);
    HRESULT GenerateHRowsFromHRowNumber(ULONG ulFirstRow,
                                       LONG lRowsOffset, LONG cRows,
                                       ULONG *pcRowsObtained, HROW **pahRows );
    HRESULT Index2HROWQuiet(ULONG ulIndex, HROW &rhrow);
    HRESULT HRowNumber2HROWQuiet(ULONG ulIndex, HROW &rhrow);

#if defined(PRODUCT_97)
    HRESULT Index2HROW(ULONG ulIndex, HROW &rhrow);
#else
    HRESULT Index2HROW(ULONG ulIndex, HROW &rhrow)
    {
        return Index2HROWQuiet(ulIndex, rhrow);
    }
#endif
    
    HRESULT HROW2Index(HROW hRow, ULONG &rulIndex,
                        ULONG *pulIndexOld = NULL,
                        BOOL *pfDeleting = NULL);

    static ULONG   ColToDBColIndex(ULONG ulColId);

    HRESULT ReleaseRowsQuiet(ULONG cRows, const HROW ahRow[],
                            ULONG aRefCounts[] = NULL,
                            DBROWSTATUS aRowStatus[] = NULL);

    static void LogErrors(DBROWSTATUS aRowStatus[], ULONG iFirstRow,
    						ULONG cErrorRows, DBROWSTATUS dbrs);

#if DBG == 1
    static void DbgCheckOverlap (const DBBINDING &currBinding);
#endif // DBG == 1

    ~CImpIRowset(); // Clients use Release not delete.
};


//+-----------------------------------------------------------------------
//
//  Member:    WhineAboutChapters (private member)
//
//  Synopsis:  Whines if we get a chapter
//
//  Arguments: hChapter     chapter handle
//
//  Returns:   S_OK             if no chapter passed in
//             DB_E_BADCHAPTER  if chapter passed in
//

inline HRESULT
CImpIRowset::WhineAboutChapters(HCHAPTER hChapter)
{
    // two lines to avoid lev 4 compiler warning
    HRESULT hr = hChapter? DB_E_BADCHAPTER : S_OK;
    return hr;
}

//+---------------------------------------------------------------------------
//  Member:     FastVariantInit (private member)
//
//  Synopsis:   Quickly initialize a VARIANT, VariantInit sets the type to
//              VT_EMPTY and then zeros the entire VARIANT, this routine simply
//              sets the type to VT_EMPTY.
//
//  Arguments:  pVar        VARIANT or VARIANTARG structure to initialize
//
//  Returns:    None
//

inline void
CImpIRowset::FastVariantInit (VARIANTARG *pVar)
{
    Assert(pVar);

    pVar->vt = VT_EMPTY;
}


//+---------------------------------------------------------------------------
//  Member:     ColToDBColIndx (private member)
//
//  Synopsis:   Quickly computes column id to index within a DBColumnInfo
//              structure.  With current Nile spec, bookmarks are at
//              at the beginning,and this routine does nothing.
//
//  Arguments:  ulColId     Column ID to return index into DBColumnInfo.
//
//  Returns:    None
//

inline ULONG
CImpIRowset::ColToDBColIndex (ULONG ulColId)
{
    return ulColId;
}



//+---------------------------------------------------------------------------
//  Member:     getpIUnknown (public member)
//
//  Synopsis:   Quickly returns the real pUnk (watch out there are many fakes)
//

inline IUnknown *
CImpIRowset::getpIUnknown() const
{
    return (IUnknown *)(IPrivateUnknown *)this;
}

#endif  //_ROWSET_HXX_
