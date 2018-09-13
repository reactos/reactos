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

#ifndef X_OLEDBERR_H_
#define X_OLEDBERR_H_
#pragma INCMSG("--- Beg <oledberr.h>")
#include <oledberr.h>
#pragma INCMSG("--- End <oledberr.h>")
#endif

#ifndef X_SIMPDATA_H_
#define X_SIMPDATA_H_
#pragma INCMSG("--- Beg <simpdata.h>")
#include <simpdata.h>
#pragma INCMSG("--- End <simpdata.h>")
#endif

#ifndef X_MSTDWRAP_H_
#define X_MSTDWRAP_H_
#include "mstdwrap.h"
#endif

#ifndef X_DBLLINK_HXX_
#define X_DBLLINK_HXX_
#include "dbllink.hxx"
#endif

#ifndef X_CDBASE_HXX_
#define X_CDBASE_HXX_
#include "cdbase.hxx"
#endif

#ifndef X_ROWHNDL_HXX_
#define X_ROWHNDL_HXX_
#include "rowhndl.hxx"
#endif

ExternTag(tagNileRowsetProvider);
ExternTag(tagOSPRowDelta);

// BUGBUG: We are hoping Nile defines this
const int DBCOMPAREOPS_NOTPARTIALEQ = 0x100;

// Initial value of various cursors for IRowset::GetNextRows,
// IRowsetFind::FindNextRows.  Use a value in the first 16 that
// we don't think OLE-DB is going to use soon
#define DBBMK_INITIAL 14

class CChapRowset;                      // forward reference

//+----------------------------------------------------------------------------
//
//  Class CSTDColumnInfo
//
//  Purpose:
//      Subset of DBCOLUMNINFO structure used to cache some of its data,
//      and to hold the reference to child rowsets!
//

class CSTDColumnInfo
{
public:
    DBTYPE dwType;
    ULONG cbMaxLength;
    DBCOLUMNFLAGS dwFlags;
    BSTR bstrName;
    // For chapter columns, reference to child rowset..
    CChapRowset *pChapRowset;
};


//+----------------------------------------------------------------------------
//
//  Class CDBProperties
//
//  Purpose:
//      Maintenance of properties and their values
//

MtExtern(CDBProperties)

class CDBProperties
{
public:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CDBProperties))
    CDBProperties();
    ~CDBProperties();
    DBPROPSET*  GetPropertySet(const GUID& guid) const;
    HRESULT     CopyPropertySet(const GUID& guid, DBPROPSET* pPropSetDst) const;
    HRESULT     CopyPropertySet(ULONG iPropSet, DBPROPSET* pPropSetDst) const
        { return CopyPropertySet(_aPropSets[iPropSet].guidPropertySet, pPropSetDst); }
    const DBPROP* GetProperty(const GUID& guid, DBPROPID id) const;
    HRESULT     SetProperty(const GUID& guid, const DBPROP& prop);
    ULONG       GetNPropSets() const { return _cPropSets; }
private:
    ULONG       _cPropSets;         // number of property sets
    DBPROPSET*  _aPropSets;         // array of property sets

    NO_COPY(CDBProperties);
};

class CImpIRowset;                      // forward references

// The PCOSPData class exists only because we can't use a COSPData * as the parameter
// of a template class (i.e. we want a CRowArray<COSPData *>).

class PCOSPData
{
public:
    COSPData* _pCOSPData;
    PCOSPData(COSPData *pCOSPData) { _pCOSPData = pCOSPData; }
    PCOSPData& operator=(COSPData* p) { _pCOSPData = p;  return *this; }
    COSPData * operator->() {return _pCOSPData;}

#ifdef UNIX
    operator HCHAPTER() { return (HCHAPTER) _pCOSPData; }
#else
    operator const HCHAPTER() { return (HCHAPTER) _pCOSPData; }
#endif

    operator COSPData*() { return _pCOSPData; }
    static COSPData* Invalid() { return NULL; }
};

//+----------------------------------------------------------------------------
//
//  Class Cospdata
//
//  Purpose:
//     Holds all information that is per-OSP (OLEDBSimpleProvider object,
//     formerly known as STD -- Simple Tabular Data).
//

MtExtern(COSPData)

class COSPData : public IUnknown
{
private:
    ULONG _ulRefs, _ulAllRefs;
    
public:

    DECLARE_MEMALLOC_NEW_DELETE(Mt(COSPData))

    class CSTDEvents : public OLEDBSimpleProviderListener
    {
     public:
        //
        // methods for ISimpleTabularDataEvents sink
        //

        //  IUnknown members
        STDMETHODIMP QueryInterface (REFIID iid, void **ppv);

        STDMETHOD_(ULONG, AddRef)()
        {
            return CONTAINING_RECORD(this, COSPData, _STDEvents)->SubAddRef();
        }

        STDMETHOD_(ULONG, Release)()
        {
            return CONTAINING_RECORD(this, COSPData, _STDEvents)->SubRelease();
        }

        //  ISimpleTabularDataEvents members
        STDMETHODIMP aboutToChangeCell(DBROWCOUNT iRow, DB_LORDINAL iColumn);
        STDMETHODIMP cellChanged(DBROWCOUNT iRow, DB_LORDINAL iColumn);
        STDMETHODIMP aboutToDeleteRows(DBROWCOUNT iRow, DBROWCOUNT iColumn);
        STDMETHODIMP deletedRows(DBROWCOUNT iRow, DBROWCOUNT cRows);
        STDMETHODIMP aboutToInsertRows(DBROWCOUNT iRow, DBROWCOUNT cRows);
        STDMETHODIMP insertedRows(DBROWCOUNT iRow, DBROWCOUNT cRows);
        STDMETHODIMP rowsAvailable(DBROWCOUNT iRow, DBROWCOUNT cRows);
        STDMETHODIMP transferComplete(OSPXFER xfer);

        // We have a private enum for the various InsertRowsHelper cases.
        // (see the big comment just before the event firing stuff).
        enum INSERT_REASONS
        {
            ROWS_ADDED,
            ROWS_INSERTED,
            ROWS_ASYNCHINSERTED
        };

        STDMETHODIMP CellChangedHelper(DBROWCOUNT iRow, DB_LORDINAL iColumn, BOOL fBefore);
        STDMETHODIMP DeleteRowsHelper(DBROWCOUNT iRow, DBROWCOUNT cRows, BOOL fBefore);
        STDMETHODIMP InsertRowsHelper(DBROWCOUNT iRow, DBROWCOUNT cRows,
                                    INSERT_REASONS eReason,
                                    BOOL fBefore);
    } _STDEvents;
    friend class CSTDEvents;

    COSPData()
    {
        _ulRefs = 1;
        _ulAllRefs = 1;
    }

    ~COSPData()
    {
        Assert(_ulRefs && "Destructor called before Passivate!");
    };

    void Passivate()
    {
        // Disallow any more notifications through here.
        IGNORE_HR(_pSTD->removeOLEDBSimpleProviderListener(
                                (OLEDBSimpleProviderListener *)&_STDEvents));
        _pRowset = NULL;

        _pSTD->Release();               // release our STD
    }

    ULONG SubAddRef() { return ++ _ulAllRefs; }
    ULONG SubRelease()
    {
        if (--_ulAllRefs == 0)
        {
            _ulRefs = ULREF_IN_DESTRUCTOR;
            _ulAllRefs = ULREF_IN_DESTRUCTOR;
            delete this;
            return 0;
        }
        return _ulAllRefs;
    }

    // IUknown methods
    STDMETHOD(QueryInterface) (REFIID iid, void **ppv);
    STDMETHOD_(ULONG, AddRef)()
    {
        SubAddRef();
        return ++_ulRefs;
    }
    STDMETHOD_(ULONG, Release)();                 // in rowset.cxx

    COSPData * Invalid () { return NULL; }

    // We need a quick backpointer to the rowset that owns us for dispatching
    // notifications, etc.  For simplicity (to avoid circular references)
    // we do not refcount this pointer.  When a rowset is passivated all
    // COSPDatas that it owns are freed.
    CImpIRowset             *_pRowset;

    // The OSP we own.
    OLEDBSimpleProvider     *_pSTD;

    // The Row # To hRow map is per OSP, so it lives here.
    // Note that hRowToRow map is consolidated across all OSPs in a Rowset
    CRowArray<ChRow> _mapIndex2hRow;

    ULONG       _iGetNextCursor;        // for IRowSet:GetNextRows
    ULONG       _iFindRowsCursor;       // for IRowsetFind::FindNextRows
    ULONG       _cSTDRows;              // rows in STD
    ULONG       _cSTDCols;              // columns in STD
    BOOL        _fPopulationComplete;   // async row population done?

    HRESULT Init(OLEDBSimpleProvider *pSTD, CImpIRowset * pRowset);

    OLEDBSimpleProvider *GetpOSP()
    {
        return _pSTD;
    }

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
//          GetReferencedRowset
//          GetSpecification
//      IRowsetLocate:
//          Compare
//          GetRowsAt
//          GetRowsByBookmark - not done
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

MtExtern(CImpIRowset)

class CImpIRowset : public CBase, public IRowsetExactScroll, public IRowsetInfo,
                    public IAccessor, public IColumnsInfo, public IRowsetChange,
                    public IRowsetNewRowAfter,
                    public IRowsetFind,
                    public IRowsetIdentity, public IConvertType,
                    public IChapteredRowset, public IRowsetChapterMember,
                    public IDBAsynchStatus
{
    friend class COSPData;
    friend class COSPData::CSTDEvents;

protected:
    CImpIRowset();
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CImpIRowset))
public:
    static HRESULT CreateRowset(CImpIRowset *pRowset,
                                LPOLEDBSimpleProvider pSTD, IUnknown **ppUnk);

    //
    // IUnknown members
    //
    DECLARE_PRIVATE_QI_FUNCS(CBase)
    DECLARE_AGGREGATED_IUNKNOWN(CImpIRowset)

    IUnknown * PunkOuter() { return _pUnkOuter; }

    //
    // CBase members
    //
    virtual const CBase::CLASSDESC *GetClassDesc() const;
    void Passivate();

    //  IRowset members
    STDMETHODIMP            AddRefRows(DBCOUNTITEM cRows, const HROW rghRows [  ], DBREFCOUNT rgRefCounts[],
                                        DBROWSTATUS rgRowStatus [  ]);
    STDMETHODIMP            GetData(HROW hRow, HACCESSOR hAccessor, void *pData);
    STDMETHODIMP            GetNextRows(HCHAPTER hReserved, DBROWOFFSET lRowsOffset, DBROWCOUNT cRows, DBCOUNTITEM *pcRowsObtained, HROW**prghRows);
    STDMETHODIMP            ReleaseRows(DBCOUNTITEM cRows, const HROW rghRows[], DBROWOPTIONS rgRefOptions[],
                                        DBREFCOUNT rgRefCounts[], DBROWSTATUS rgRowStatus[]);
    STDMETHODIMP            RestartPosition(HCHAPTER);

    // IRowsetInfo members
    STDMETHODIMP            GetProperties(const ULONG cPropertyIDSets, 
                                            const DBPROPIDSET rgPropertyIDSets[  ],
                                            ULONG *pcPropertySEts,
                                            DBPROPSET  ** prgPropertySets);
    STDMETHODIMP            GetReferencedRowset(DBORDINAL iOrdinal, REFIID riid, IUnknown**);
    STDMETHODIMP            GetSpecification(REFIID, IUnknown**);
    

    //  IAccessor members
    STDMETHODIMP            AddRefAccessor(HACCESSOR hAccessor, DBREFCOUNT *pcRefCount);
    STDMETHODIMP            CreateAccessor(DBACCESSORFLAGS dwAccessorFlags, DBCOUNTITEM cBindings,
                                        const DBBINDING rgBindings[], DBLENGTH cbRowSize, HACCESSOR* phAccessor,
                                        DBBINDSTATUS rgStatus[]);
    STDMETHODIMP            GetBindings(HACCESSOR, DBACCESSORFLAGS*,
                                        DBCOUNTITEM*, DBBINDING** );
    STDMETHODIMP            ReleaseAccessor(HACCESSOR hAccessor, DBREFCOUNT *pcRefCount);
    
    //  IColumnsInfo members
    STDMETHODIMP            GetColumnInfo(DBORDINAL*, DBCOLUMNINFO**, OLECHAR**);
    STDMETHODIMP            MapColumnIDs(DBORDINAL cColumnIDs, const DBID rgColumnIDs[], DBORDINAL rgColumns[]);

    //  IRowsetChange members
    STDMETHODIMP            DeleteRows(HCHAPTER hReserved, DBCOUNTITEM cRows, const HROW rghRows[],
                                       DBROWSTATUS rgRowStatus[]);
    STDMETHODIMP            SetData(HROW, HACCESSOR, void*);
    STDMETHODIMP            InsertRow (HCHAPTER    hChapter,
                                       HACCESSOR   hAccessor,
                                       void       *pData,
                                       HROW       *phRow );

    //  IRowsetLocate members
    STDMETHODIMP            Compare(HCHAPTER hChapter,
                                    DBBKMARK cbBookmark1, const BYTE *pBookmark1,
                                    DBBKMARK cbBookmark2, const BYTE *pBookmark2,
                                    DBCOMPARE *pdwComparison );
    STDMETHODIMP            GetRowsAt(HWATCHREGION hRegion,
                                      HCHAPTER hChapter,
                                      DBBKMARK cbBookmark,  const BYTE *pBookmark,
                                      DBROWOFFSET  lRowsOffset,     DBROWCOUNT  cRows,
                                      DBCOUNTITEM *pcRowsObtained, HROW **pahRows );
    STDMETHODIMP            GetRowsByBookmark(HCHAPTER    hChapter,
                                              DBCOUNTITEM cRows,
                                              const DBBKMARK acbBookmarks[],
                                              const BYTE *apBookmarks[],
                                              HROW        ahRows[],
                                              DBROWSTATUS aRowStatus[] );
    STDMETHODIMP            Hash(HCHAPTER    hChapter,
                                 DBBKMARK    cBookmarks, const DBBKMARK acbBookmarks[],
                                 const BYTE *apBookmarks[],
                                 DBHASHVALUE aHashedValues[],
                                 DBROWSTATUS aBookmarkStatus[] );


    //  IRowsetScroll members
    STDMETHODIMP            GetApproximatePosition(HCHAPTER    hChapter,
                                                   DBBKMARK    cbBookmark,
                                                   const BYTE *pBookmark,
                                                   DBCOUNTITEM *pulPosition,
                                                   DBCOUNTITEM *pcRows );
    STDMETHODIMP            GetRowsAtRatio(HWATCHREGION hRegion,
                                           HCHAPTER    hChapter,
                                           DBCOUNTITEM ulNumerator,
                                           DBCOUNTITEM ulDenominator,
                                           DBROWCOUNT  cRows,
                                           DBCOUNTITEM *pcRowsObtained,
                                           HROW      **pahRows );

    //  IRowsetExactScroll members
    STDMETHODIMP            GetExactPosition(HCHAPTER    hChapter,
                                             DBBKMARK    cbBookmark,
                                             const BYTE *pBookmark,
                                             DBCOUNTITEM *pulPosition,
                                             DBCOUNTITEM *pcRows );

    //  IRowsetNewRowAfter member
    STDMETHODIMP            SetNewDataAfter(HCHAPTER    hChapter,
                                            DBBKMARK    cbBookmarkPrevious,
                                            const BYTE *pBookmarkPrevious,
                                            HACCESSOR   hAccessor,
                                            BYTE       *pData,
                                            HROW       *phRow );

    //  IRowsetIdentity member
    STDMETHODIMP            IsSameRow(HROW  hThisRow,
                                      HROW  hThatRow);


    //  IRowsetFind member
    STDMETHODIMP            FindNextRow(HCHAPTER hChapter,
                                         HACCESSOR hAccessor,
                                         void * pValue,
                                         DBCOMPAREOP CompareOp,
                                         DBBKMARK cbBookmark,
                                         const BYTE *pBookmark,
                                         DBROWOFFSET lRowsOffset,
                                         DBROWCOUNT cRows,
                                         DBCOUNTITEM *pcRowsObtained,
                                         HROW **prghRows);

    //  IDBAsynchStatus members
    STDMETHODIMP            Abort(HCHAPTER  hChapter,
                                  DBASYNCHOP ulOperation);

    STDMETHODIMP            GetStatus(HCHAPTER  hChapter,
                                      DBASYNCHOP ulOperation,
                                      DBCOUNTITEM *pulProgress,
                                      DBCOUNTITEM *pulProgressMax,
                                      DBASYNCHPHASE *pulStatusCode,
                                      LPOLESTR *ppwszStatusText);

    // IConvertType members
    STDMETHODIMP            CanConvert(DBTYPE wFromType,
                                       DBTYPE wToType,
                                       DBCONVERTFLAGS dwConvertFlags);

    // IChapteredRowset members
    STDMETHODIMP            AddRefChapter(HCHAPTER hChapter,
                                          DBREFCOUNT *pcRefCount);

    STDMETHODIMP            ReleaseChapter(HCHAPTER hChapter,
                                           DBREFCOUNT *pcRefCount);

    // IRowsetChapterMember members
    STDMETHODIMP            IsRowInChapter(HCHAPTER hChapter,
                                           HROW hrow);

    // Event firing helper functions:
#if defined(PRODUCT_97)
    HRESULT     FireChapterEvent(HCHAPTER hChapter, DBREASON eReason);
#endif
    HRESULT     FireFieldEvent(HROW hRow, DBORDINAL cColumns, DBORDINAL aColumns[],
                               DBREASON eReason, DBEVENTPHASE ePhase);
    HRESULT     FireRowEvent(DBCOUNTITEM cRows, const HROW rghRows[],
                             DBREASON eReason, DBEVENTPHASE ePhase);
    HRESULT     FireRowsetEvent(DBREASON eReason, DBEVENTPHASE ePhase);
    HRESULT     FireAsynchOnProgress(DBCOUNTITEM ulProgress, DBCOUNTITEM ulProgressMax,
                                     DBASYNCHPHASE ulStatusCode);
    HRESULT     FireAsynchOnStop(HRESULT hrStatus);

    // Misc. help functions.
    virtual HRESULT Init(OLEDBSimpleProvider *pSTD);
    HRESULT     CacheMetaData();
    IUnknown   *getpIUnknown() const;
    
protected:
    static const CBase::CLASSDESC       s_classdesc;// IRowset classDesc
    static const CONNECTION_POINT_INFO  s_acpi[];   // IRowsetNotify conn. pt, &
                                                    // IDBAsynchNotify conn. pt.

    // Pure virtual method to get us quickly to the right COSPData.
    virtual     COSPData *GetpOSPData(HCHAPTER hChapter) = 0;

    virtual HCHAPTER    HChapterFromOSPData(COSPData *pOSPData)
    {
        return DB_NULL_HCHAPTER;
    }

    virtual     OLEDBSimpleProvider *GetpMetaOSP() = 0;
    OLEDBSimpleProvider *GetpOSP(HCHAPTER hChapter)
    {
        return GetpOSPData(hChapter)->GetpOSP();
    }

    // Internal accessor structure
    struct AccessorFormat: public CIntrusiveDblLinkedListNode
    {
        void* operator new(size_t uCompilerSize, ULONG cBindings)
            { return ::new BYTE[uCompilerSize + (cBindings-1)*sizeof(DBBINDING)]; }

        AccessorFormat(ULONG RefCount, DBACCESSORFLAGS AccFlags, ULONG Bindings,
                        const DBBINDING rgBindings[]):
                cRefCount(RefCount), dwAccFlags(AccFlags), cBindings(Bindings)
                { // Copy the client's bindings into the ACCESSOR.
                    memcpy(aBindings, rgBindings, cBindings * sizeof(DBBINDING) );
                }
        
        ULONG     cRefCount;
        DBACCESSORFLAGS  dwAccFlags;
        ULONG     cBindings;
        DBBINDING aBindings[1];
    };
    TIntrusiveDblLinkedListOfStruct<AccessorFormat> _dblAccessors;  // active accessors
    CRowArray<CIndex>  _maphRow2Index;   // Handle table (entries are CRow's)
    LONG               _NextH2R;        // Index of next free _mapHandle2Row

    // For iterating through ChRow's.
    ChRow   FirsthRef(HCHAPTER hChapter);
    ChRow   NexthRef(HCHAPTER hChapter, ChRow hRef);
    BOOL    ValidhRef(ChRow hRef);

    virtual HCHAPTER ChapterFromHRow(ChRow href)
    {
        return (HCHAPTER)(_maphRow2Index.GetElem(href.DeRef()).GetpChap());
    }

    virtual int     IndexFromHRow(ChRow href)
    {
            return _maphRow2Index.GetElem(href.DeRef()).Row();
    }

    virtual BOOL    FhRowDeleted(ChRow href)
    {
        return _maphRow2Index.GetElem(href.DeRef()).FDeleted();
    }

    // InsertRows returns FALSE for out of memory
    HRESULT InsertRows(ULONG Row,       // row idx to delete (0 is before 1st)
                       int crows,       // # of rows to delete
                       COSPData *pOSP);
    // DeleteRows has no failure modes
    void    DeleteRows(ULONG Row,       // row idx to delete (0 is first row)
                       int crows,       // # of rows to delete
                       COSPData *pOSP);

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

    CSTDColumnInfo  *_astdcolinfo;      // Cached ColumnInfo array
    ULONG            _cCols;            // number of columns in Rowset
    ULONG            _cRows;            // number of total rows in rowset

    // Properties
    CDBProperties    _dbpProperties;    // properties

    HRESULT EnsureReferencedRowset(HCHAPTER hchapter, ULONG uStdRow, ULONG icol,
                                    OLEDBSimpleProvider **ppOSP);
    HRESULT GetChapterData(XferInfo &xfrData, CSTDColumnInfo &stdColInfo,
                           DBBINDING &currBinding, HCHAPTER hChapter,
                           HROW hrow, ULONG uStdRow, ULONG icol);

    HRESULT IndexToHRowNumber(HCHAPTER hChapter, ULONG index, ULONG &rulRow);

    IUnknown *              _pUnkOuter;

    // Notification reentrancy stuff
#   define DBEVENTPHASE_MAX ((DWORD) DBEVENTPHASE_DIDEVENT)
#   define DBREASON_MAX     ((DWORD) DBREASON_ROW_ASYNCHINSERT) // for now
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
    HRESULT DataToWSTR (DATA_DIRECTION dDirection, HCHAPTER hChapter,
                        ULONG uRow, ULONG uCol, XferInfo & xferData);
    void    DataCoerce_STR (HCHAPTER hChapter, ULONG uRow, ULONG uCol,
                            BOOL fBSTR, XferInfo & xferData );
    HRESULT DataCoerce (DATA_DIRECTION dDirection, HCHAPTER hChapter,
                        ULONG uRow, ULONG uCol, XferInfo & xferData);

    static HRESULT WhineAboutChapters(HCHAPTER hChapter);
    HRESULT Bookmark2HRowNumber(HCHAPTER hChapter, DBBKMARK cbBookmark,
                                const BYTE *pBookmark, DBCOUNTITEM &rulRow);
    HRESULT GenerateHRowsFromHRowNumber(HCHAPTER hChapter, DBCOUNTITEM ulFirstRow,
                                       DBROWOFFSET lRowsOffset, DBROWCOUNT cRows,
                                       DBCOUNTITEM *pcRowsObtained, HROW **pahRows );
    HRESULT HRowNumber2HROWQuiet(HCHAPTER hChapter, DBCOUNTITEM ulIndex, HROW &rhrow);

    ChRow   HRowFromIndex(HCHAPTER hChapter, DBCOUNTITEM row);

    HRESULT Index2HROW(HCHAPTER hChapter, DBCOUNTITEM ulIndex, HROW &rhrow);

    HRESULT HROW2Index(HROW hRow, DBCOUNTITEM &rulIndex);

    static DBORDINAL   ColToDBColIndex(DBORDINAL ulColId);

    HRESULT ReleaseRowsQuiet(DBCOUNTITEM cRows, const HROW ahRow[],
                            DBREFCOUNT aRefCounts[] = NULL,
                            DBROWSTATUS aRowStatus[] = NULL);

    static void LogErrors(DBROWSTATUS aRowStatus[], DBCOUNTITEM iFirstRow,
                            DBCOUNTITEM cErrorRows, DBROWSTATUS dbrs);

#if DBG == 1
    static void DbgCheckOverlap (const DBBINDING &currBinding);
#endif // DBG == 1

    ~CImpIRowset(); // Clients use Release not delete.
};

MtExtern(CTopRowset)

class CTopRowset : public CImpIRowset
{
    typedef CImpIRowset super;

    COSPData    *_pOSPData;
    COSPData    *GetpOSPData(HCHAPTER hChapter)
    {
        return(_pOSPData);
    }
    OLEDBSimpleProvider *GetpMetaOSP()    
    {
        return(_pOSPData->GetpOSP());
    }
    ~CTopRowset();                      // really only for debug

public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CTopRowset))

    void Passivate();
    HRESULT Init(OLEDBSimpleProvider *pSTD);
    static HRESULT CreateRowset(LPOLEDBSimpleProvider pSTD, IUnknown **ppUnk);

};

MtExtern(CChapRowset)

class CChapRowset : public CImpIRowset
{
    typedef CImpIRowset super;
    OLEDBSimpleProvider *_pMetaOSP;
    CRowArray<PCOSPData> _aryOSPData;

    COSPData * GetpOSPData(HCHAPTER hChapter)
    {
        return (COSPData *)hChapter;
    }

    OLEDBSimpleProvider *GetpMetaOSP()    
    {
        return(_pMetaOSP);
    }

    virtual HCHAPTER HChapterFromOSPData(COSPData *pOSPData)
    {
        return (HCHAPTER)pOSPData;
    }

public:
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CChapRowset))
    void Passivate();
    HRESULT Init(OLEDBSimpleProvider *pSTD) { return super::Init(pSTD); } // for now.

    HRESULT EnsureHChapter(HROW hrow,
                           OLEDBSimpleProvider *, HCHAPTER *);
    static HRESULT CreateRowset(LPOLEDBSimpleProvider pSTD, IUnknown **ppUnk);
};


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

inline DBORDINAL
CImpIRowset::ColToDBColIndex (DBORDINAL ulColId)
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
