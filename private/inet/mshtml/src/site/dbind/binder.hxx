//+----------------------------------------------------------------------------
// binder.hxx:
//          CCurrentRecordInstance
//          CDataSourceProvider
//          CDataSourceBinder
//
// Copyright: (c) 1994-1995, Microsoft Corporation
// All rights Reserved.
// Information contained herein is Proprietary and Confidential.
//
// Contents: this file contains the above refered to definitions.
//

#ifndef I_BINDER_HXX_
#define I_BINDER_HXX_
#pragma INCMSG("--- Beg 'binder.hxx'")

MtExtern(CCurrentRecordInstance)
MtExtern(CDataSourceProvider)
MtExtern(CDataSourceProvider_aryAdvisees_pv)
MtExtern(CDataSourceBinder)

#ifndef X_DLCURSOR_HXX_
#define X_DLCURSOR_HXX_
#include "dlcursor.hxx"
#endif

#ifndef X_OLEDB_H_
#define X_OLEDB_H_
#pragma INCMSG("--- Beg <oledb.h>")
#include <oledb.h>
#pragma INCMSG("--- End <oledb.h>")
#endif

// forward references
interface _ADORecordset;
interface Recordset15;
interface IRowPositionChange;
interface OLEDBSimpleProvider;
interface IHTMLBookmarkCollection;
interface IHTMLElementCollection;
class CRecordInstance;
class CDataLayerCursorEvents;
class CDataBindProgress;
class CDataBindTask;

//+----------------------------------------------------------------------------
//
// Forward declarations
//
class CElement;
struct DBSPEC;
class CDoc;
interface ICursor;
interface IVBDSC;
class CDataLayerCursor;
class CDataLayerBookmark;
class COleSite;
interface ISimpleTabularData;
class CDataSourceProvider;
class CDataSourceBinder;
class CXferThunk;
class CDataMemberMgr;

//+----------------------------------------------------------------------------
//
//  Class CCurrentRecordInstance
//
//  Purpose:
//      (a) hold a CRecordInstance attached to the current row of a rowset.
//          Data consumers can bind to this instance.
//      (b) sink IRowPositionChange notifications from the rowset, and
//          propagate the changes to all consumers bound to the instance.

EXTERN_C const IID IID_ICurrentRecordInstance;

class CCurrentRecordInstance: public IRowPositionChange, public CDataLayerCursorEvents
{
public:
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CCurrentRecordInstance))
    // creation/destruction
    CCurrentRecordInstance();
    HRESULT Init(CDataSourceProvider *pProvider, IRowPosition *pRowPos);
    HRESULT InitChapter(HCHAPTER hChapter);
    HRESULT InitPosition(BOOL fFireRowEnter=FALSE);
    HRESULT Detach();
    
    // CurrentRecordInstance methods
    HRESULT GetCurrentRecordInstance(CRecordInstance **ppRecInstance);
    
    // IUnknown methods
    STDMETHODIMP_(ULONG)    AddRef() { return ++_ulRefCount; }
    STDMETHODIMP_(ULONG)    Release();
    STDMETHODIMP            QueryInterface(REFIID riid, void **ppv);
    
    // IRowPositionChanged methods
    STDMETHODIMP OnRowPositionChange(DBREASON eReason, DBEVENTPHASE ePhase,
                                BOOL fCantDeny);

    // CDataLayerCursorEvents methods
    HRESULT AllChanged();
    HRESULT RowsChanged(DBCOUNTITEM cRows, const HROW *ahRows);
    HRESULT FieldsChanged(HROW hRow, DBORDINAL cColumns, DBORDINAL aColumns[]);
    HRESULT RowsInserted(DBCOUNTITEM cRows, const HROW *ahRows);
    HRESULT DeletingRows(DBCOUNTITEM cRows, const HROW *ahRows);
    HRESULT RowsDeleted(DBCOUNTITEM cRows, const HROW *ahRows);
    HRESULT DeleteCancelled(DBCOUNTITEM cRows, const HROW *ahRows);
    HRESULT RowsAdded();
    HRESULT PopulationComplete();

    // cursor access
    CDataLayerCursor *GetDLCursor() { return _pDLC; }
    
private:
    HRESULT Passivate();
    HRESULT InitCurrentRow();
    
    ULONG                   _ulRefCount;
    IRowPosition            *_pRowPos;      // my row position
    CRecordInstance         *_priCurrent;   // current instance
    CDataSourceProvider     *_pProvider;    // owning provider
    HROW                    _hrow;          // current HROW
    HCHAPTER                _hChapter;      // current HCHAPTER
    CDataLayerCursor        *_pDLC;         // needed to acquire/release HROW
    CDataLayerBookmark      _dlbDeleted;    // bookmark for deleted current row 
    // state of my connection
    IConnectionPoint    *_pCP;
    DWORD               _dwAdviseCookie;
};


//+----------------------------------------------------------------------------
//
//  Class CDataSourceProvider (abstract)
//  concrete derived classes are defined elsewhere
//
//  Purpose:
//      Hand out interfaces on behalf of data source controls
//
//  Created by sambent
//
//  Hungarian: dsp, pdsp
//

class CDataSourceProvider         // abstract provider
{
public:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CDataSourceProvider))
    static  HRESULT Create(CDataMemberMgr *pDMembMgr, CDoc *pDoc, BSTR bstrMember,
                            CDataSourceProvider **ppdsp);
    ULONG   AddRef() { return ++ _ulRefCount; }
    ULONG   Release();
    virtual void    Detach();
    virtual HRESULT QueryDataInterface(REFIID riid, void **ppv) = 0;
    HRESULT AdviseDataProviderEvents(CDataSourceBinder *pdsb);
    HRESULT UnadviseDataProviderEvents(CDataSourceBinder *pdsb);
    virtual HRESULT ReplaceProvider(CDataSourceProvider *pdspNewProvider);
    virtual HRESULT GetSubProvider(CDataSourceProvider **ppdsp,
                                    LPCTSTR bstrName, HROW hrow=0) = 0;
    virtual CXferThunk* GetXT(LPCTSTR strColumn, CVarType vtDest)
                { return NULL; }

    // stops async download
    virtual HRESULT Stop() { return S_OK; } // default implementation

    // hierarchy
    virtual HCHAPTER GetChapter() { return DB_NULL_HCHAPTER; }

    // events
    virtual HRESULT FireDataEvent(LPCTSTR pchType,
                            DISPID dispidEvent, DISPID dispidProp,
                            BOOL *pfCancelable = NULL,
                            long lReason = 0)
                { return S_OK; }
    virtual HRESULT FireDelayedRowEnter() { return S_OK; }
    virtual HRESULT get_bookmarks(IHTMLBookmarkCollection* *) { return E_FAIL; }
    virtual HRESULT get_recordset(IDispatch* *) { return E_FAIL; }
    virtual HRESULT get_dataFld(BSTR *) { return E_FAIL; }
    HRESULT LoadBoundElementCollection(CCollectionCache *pCollectionCache,
                                       long lIndex);
    
    LPTSTR DataMember() { return _cstrDataMember; }
    
protected:
    CDataSourceProvider(): _ulRefCount(1) {}
    virtual ~CDataSourceProvider() {}
    
    CStr                _cstrDataMember;
    CDataMemberMgr *    _pDataMemberMgr;
    CDoc *              _pDoc;
    
private:
    NO_COPY(CDataSourceProvider);
    
    ULONG   _ulRefCount;
    
    DECLARE_CPtrAry(CAryAdvisees, CDataSourceBinder *, Mt(Mem), Mt(CDataSourceProvider_aryAdvisees_pv))
    CAryAdvisees _aryAdvisees;
};


enum DBINDER_OPERATION {
        BINDEROP_BIND,
        BINDEROP_UNBIND,
        BINDEROP_REBIND,
        BINDEROP_ENSURE_DATA_EVENTS,
        DBINDER_OPERATION_Last_Enum
};

//+----------------------------------------------------------------------------
//
//  Class CDataSourceBinder
//
//  Purpose:
//      Binds a data consumer to its data source
//
//  Created by sambent
//
//  Hungarian: dsb, pdsb
//

class CDataSourceBinder
{
public:
    class CConsumer: public IUnknown    // abstract consumer
    {
    public:
        CConsumer(CDataSourceBinder *pBinder)
            : _pBinder(pBinder),
              _ulRefs(1)
            {}
            
        static  HRESULT Create(CDataSourceBinder *pBinder,
                               CConsumer **ppConsumer);
        virtual HRESULT Bind() = 0;
        virtual HRESULT UnBind() = 0;
        virtual HRESULT GetDLCursor(CDataLayerCursor **ppdlcCursor) { return E_FAIL; }
        virtual HRESULT GetICursor(ICursor **ppCursor) { return E_FAIL; }
        virtual HRESULT GetDataSrcAndFld(BSTR *, BSTR *) { return E_FAIL; }
        virtual void    FireOnDataReady(BOOL fReady) = 0;
        virtual void    Passivate() {}
        
        // IUnknown members
        ULONG STDMETHODCALLTYPE AddRef() { return ++_ulRefs; }
        ULONG STDMETHODCALLTYPE Release();
        HRESULT STDMETHODCALLTYPE QueryInterface(REFIID refiid, LPVOID *ppv) { return E_NOINTERFACE; }
    protected:
        CDataSourceProvider*  GetProvider() { return _pBinder->GetProvider(); }
        CElement *       GetElementConsumer()
            { return _pBinder->GetElementConsumer(); }
        LONG             IdConsumer()   { return _pBinder->IdConsumer(); }
        void             SubstituteProvider(CDataSourceProvider *pProviderNew)
            { _pBinder->SubstituteProvider(pProviderNew); }
            
        virtual ~CConsumer() {}
        CDataSourceBinder   *_pBinder;  // binder who owns me
    private:
        ULONG               _ulRefs;    // reference count
    };
    
    friend class CConsumer;
    
public:
    // creation / destruction
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CDataSourceBinder))
    CDataSourceBinder(CElement *pel, LONG id, DBINDER_OPERATION dbop=BINDEROP_BIND) :
            _pelConsumer(pel), _idConsumer(id), _dbop(dbop) {}
    HRESULT Passivate();

    // requests for service
    HRESULT Register(const DBSPEC *pdbs);

    HRESULT GetDLCursor(CDataLayerCursor **ppdlcCursor)
            { return _pConsumer->GetDLCursor(ppdlcCursor); }
    HRESULT GetICursor(ICursor **ppCursor)
            { return _pConsumer ? _pConsumer->GetICursor(ppCursor) : E_FAIL; }
    HRESULT GetDataSrcAndFld(BSTR *pbstrDataSrc, BSTR *pbstrDataFld)
            { return _pConsumer->GetDataSrcAndFld(pbstrDataSrc, pbstrDataFld); }

    // notifications from provider
    HRESULT ReplaceProvider(CDataSourceProvider *pdspNewProvider);
                        // provider is being replaced by a new one
    HRESULT DetachBinding();

    // public member access
    LONG    IdConsumer()                { return _idConsumer; }
    void    SetReady(BOOL fReady);
    CDataSourceProvider*  GetProvider() { return _pProvider; }
    CElement* GetElementConsumer()      { return _pelConsumer; }

protected:
    // private helper methods
    HRESULT Bind();
    HRESULT UnBind();
    HRESULT ReBind();
    HRESULT EnsureDataEvents();
    const CStr& GetProviderName() const { return _cstrProviderName; }
    HRESULT TryToBind();
    HRESULT FindProviderByName(CElement **ppelProvider);
    void    SubstituteProvider(CDataSourceProvider *pProviderNew);

    // communication
    CDataBindTask* GetDataBindTask();
    
private:
    // consumer/provider
    CElement            *_pelConsumer;
    LONG                _idConsumer;
    CConsumer           *_pConsumer;
    CDataSourceProvider *_pProvider;
    CStr                _cstrProviderName;
    CStr                _cstrDataMember;
    CElement            *_pelProvider;

    // outside world
    CDoc                *_pDoc;         // for name resolution

    // list of deferred bindings
    friend class CDataBindTask;
    CDataSourceBinder   *_pdsbNext;
    DBINDER_OPERATION   _dbop;
    unsigned            _fOnTaskList:1;     // true if I'm on a task list
    unsigned            _fNotReady:1;       // consumer is not ready to bind
    unsigned            _fNoProvider:1;     // couldn't find the provider
    unsigned            _fBinding:1;        // for re-entrancy test
    unsigned            _fAbort:1;          // re-entrant call killed binder

    NO_COPY(CDataSourceBinder);
#if DBG==1
    ~CDataSourceBinder();
#endif
};


////////////////////////////////////////////////////////////////////////
//      Helper functions

void    ReleaseChapterAndRow(HCHAPTER hchapter, HROW hrow, IRowPosition *pRowPos);

#pragma INCMSG("--- End 'binder.hxx'")
#else
#pragma INCMSG("*** Dup 'binder.hxx'")
#endif
