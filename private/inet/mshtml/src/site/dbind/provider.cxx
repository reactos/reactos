//+---------------------------------------------------------------------------
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 1996-1997
//  All rights Reserved.
//  Information contained herein is Proprietary and Confidential.
//
//  Contents:   Data Source Provider objects
//
//  Classes:    CDataSourceProvider (abstract)
//                  CAdaptingProvider
//                      CRowsetProvider
//                      CSTDProvider
//                      CCursorProvider
//                  CNullProvider
//              CDataSetEvents (used by provider classes)
//
//  History:    10/1/96     (sambent) created

// The concrete classes derived from CDataSourceProvider are
// declared in this file, which makes them unknown to the rest of the world.
// The function CDataSourceProvider::Create acts as a
// factory -- creating the appropriate concrete provider.
// 
// To support a new type of provider, derive a new class from CDataSourceProvider,
// and add code to CDataSourceProvider::Create to create an instance.

// The CDataSourceProvider class (and classes derived from it), are
// designed to be owned by a CObjectElement associated with a data-source
// control.  This allows the Trident object model to expose an ADO
// interface on suitable controls -- the OM request for ADO translates into
// a QDI (QueryDataInterface) on the element's provider object.
// A CDataSourceBinder object also holds a reference to a provider, usually (but
// not always) one that it shares with a control site.
// 
// Some words about refcounting.  The provider object owns (and refcounts)
// references into the control.  The owning element also owns references
// into the control, as well as a reference to the provider.  When the
// owning element is through with its control, its Detach method releases
// the provider, which in turn releases its own references to the control.
// Releasing in Detach solves any circular reference problems.
// 
// When a provider is shared by a control site and a binder, it doesn't die
// until both references go away.  This keeps the control alive even after
// its site has died as long as it's still in use for databinding.
// 
// Providers also snap in adapters between a control's native interface and
// the interface desired by a data-bound element (or the object model).  A
// provider remembers adapted interfaces it has handed out, so that it can
// hand out the same interface if asked again.  Thus, several
// ICursor-consuming elements can be bound to single STD control, and the
// provider arranges that they all share the same ICursor it has created by
// interposing the Viaduct II (cursor-to-rowset) and Nile2Std
// (rowset-to-STD) adapters.

#include <headers.hxx>

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include <formkrnl.hxx>     // for safetylevel in safety.hxx (via olesite.hxx)
#endif

#ifndef X_OLESITE_HXX_
#define X_OLESITE_HXX_
#include <olesite.hxx>
#endif

#ifndef X_ROWSET_HXX_
#define X_ROWSET_HXX_
#include <rowset.hxx>
#endif

#ifndef X_FORMSARY_HXX_
#define X_FORMSARY_HXX_
#include <formsary.hxx>
#endif

#ifndef X_EVNTPRM_HXX_
#define X_EVNTPRM_HXX_
#include <evntprm.hxx>
#endif

#ifndef X_BINDER_HXX_
#define X_BINDER_HXX_
#include "binder.hxx"
#endif

#ifndef X_DBTASK_HXX_
#define X_DBTASK_HXX_
#include "dbtask.hxx"
#endif

#ifndef X_DBINDING_HXX_
#define X_DBINDING_HXX_
#include "dbinding.hxx"
#endif

#ifndef X_DMEMBMGR_HXX_
#define X_DMEMBMGR_HXX_
#include "dmembmgr.hxx"       // for CDataMemberMgr
#endif

#ifndef X_SIMPDC_H_
#define X_SIMPDC_H_
#include "simpdc.h"
#endif

#ifndef X_BMKCOLL_HXX_
#define X_BMKCOLL_HXX_
#include "bmkcoll.hxx"
#endif

#ifndef X_ADO_ADOID_H_
#define X_ADO_ADOID_H_
#include <adoid.h>
#endif

#ifndef X_ADO_ADOINT_H_
#define X_ADO_ADOINT_H_
#include <adoint.h>
#endif

#ifndef X_ADO_ADOSECUR_H_
#define X_ADO_ADOSECUR_H_
#include <adosecur.h>
#endif

#define VD_INCLUDE_ROWPOSITION 

#ifndef X_VBCURSOR_OCDB_H_
#define X_VBCURSOR_OCDB_H_
#include <vbcursor/ocdb.h>
#endif

#ifndef X_VBCURSOR_OCDBID_H_
#define X_VBCURSOR_OCDBID_H_
#include <vbcursor/ocdbid.h>
#endif

#ifndef X_VBCURSOR_VBDSC_H_
#define X_VBCURSOR_VBDSC_H_
#include <vbcursor/vbdsc.h>
#endif

#ifndef X_VBCURSOR_OLEBIND_H_
#define X_VBCURSOR_OLEBIND_H_
#include <vbcursor/olebind.h>
#endif

#ifndef X_VBCURSOR_MSC2R_H_
#define X_VBCURSOR_MSC2R_H_
#include <vbcursor/msc2r.h>                      // viaduct-i  ole include
#endif

#ifndef X_VBCURSOR_MSR2C_H_
#define X_VBCURSOR_MSR2C_H_
#include <vbcursor/msr2c.h>                      // viaduct-ii ole include
#endif

#ifndef X_OBJSAFE_H_
#define X_OBJSAFE_H_
#include "objsafe.h"
#endif

typedef Recordset15 IADORecordset;    // beats me why ADO doesn't use I...
typedef ADORecordsetConstruction IADORecordsetConstruction;

#ifdef UNIX
#include "unixposition.hxx"
#else
   EXTERN_C const CLSID CLSID_CRowPosition;
#endif

class CAdaptingProvider;

DeclareTag(tagRowEvents, "RowEvents", "Show row events");
MtDefine(CDataSetEvents, DataBind, "CDataSetEvents")
MtDefine(CNullProvider, DataBind, "CNullProvider")
MtDefine(CAdaptingProvider, DataBind, "CAdaptingProvider")
MtDefine(CAdaptingProvider_aryCR_pv, CAdaptingProvider, "CAdaptingProvider::_aryCR::_pv")
MtDefine(CAdaptingProvider_aryPR_pv, CAdaptingProvider, "CAdaptingProvider::_aryPR::_pv")
MtDefine(CAdaptingProvider_aryAR_pv, CAdaptingProvider, "CAdaptingProvider::_aryAR::_pv")
MtDefine(CAdaptingProvider_aryXT_pv, CAdaptingProvider, "CAdaptingProvider::_aryXT::_pv");


/////////////////////////////////////////////////////////////////////////////
/////                       Helper functions                            /////
/////////////////////////////////////////////////////////////////////////////

void
ReleaseChapterAndRow(HCHAPTER hchapter, HROW hrow, IRowPosition *pRowPos)
{
    IRowset *pRowset = NULL;
    IChapteredRowset *pChapRowset = NULL;
    
    if (pRowPos && (hrow != DB_NULL_HROW || hchapter != DB_NULL_HCHAPTER))
    {
        pRowPos->GetRowset(IID_IRowset, (IUnknown **)&pRowset);
        AssertSz(pRowset, "Can't get rowset from rowpos");

        // release the hrow
        if (hrow != DB_NULL_HROW)
        {
            pRowset->ReleaseRows(1, &hrow, NULL, NULL, NULL);
        }

        // release the hchapter
        if (hchapter != DB_NULL_HCHAPTER)
        {
            pRowset->QueryInterface(IID_IChapteredRowset, (void**)&pChapRowset);
            if (pChapRowset)
            {
                pChapRowset->ReleaseChapter(hchapter, NULL);
                ReleaseInterface(pChapRowset);
            }
        }

        ReleaseInterface(pRowset);
    }
}

/////-------------------------------------------------------------------/////
///// CDataSetEvents.  Listens for rowset notifications on behalf of    /////
///// provider, and fires data events                                   /////
/////-------------------------------------------------------------------/////

class CDataSetEvents: public IRowPositionChange, public IDBAsynchNotify,
                      public IRowsetNotify,
                      public IOleClientSite, public IServiceProvider
{
    friend class CAdaptingProvider;
public:
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CDataSetEvents))
    // creation/destruction
    CDataSetEvents(): _ulRefs(1) {}
    ~CDataSetEvents();
    HRESULT Init(CDataMemberMgr *pDMembMgr, IRowset *pRowset, IRowPosition *pRowPos,
                    CAdaptingProvider *pProvider);
    HRESULT Detach();

    // watch out for callback on wrong thread
    HRESULT CheckCallbackThread();
    
    // IRowPositionChanged methods
    STDMETHODIMP OnRowPositionChange(DBREASON eReason, DBEVENTPHASE ePhase,
                                BOOL fCantDeny);

    //  IDBAsynchNotify methods
    virtual HRESULT STDMETHODCALLTYPE OnLowResource( 
            /* [in] */ DB_DWRESERVE dwReserved);
        
    virtual HRESULT STDMETHODCALLTYPE OnProgress( 
            /* [in] */ HCHAPTER hChapter,
            /* [in] */ DBASYNCHOP ulOperation,
            /* [in] */ DBCOUNTITEM ulProgress,
            /* [in] */ DBCOUNTITEM ulProgressMax,
            /* [in] */ DBASYNCHPHASE ulStatusCode,
            /* [in] */ LPOLESTR pwszStatusText);
        
    virtual HRESULT STDMETHODCALLTYPE OnStop( 
            /* [in] */ HCHAPTER hChapter,
            /* [in] */ DBASYNCHOP ulOperation,
            /* [in] */ HRESULT hrStatus,
            /* [in] */ LPOLESTR pwszStatusText);

    // IRowsetNotify methods
    virtual HRESULT STDMETHODCALLTYPE OnFieldChange( 
        /* [in] */ IRowset *pRowset,
        /* [in] */ HROW hRow,
        /* [in] */ DBORDINAL cColumns,
        /* [size_is][in] */ DBORDINAL rgColumns[  ],
        /* [in] */ DBREASON eReason,
        /* [in] */ DBEVENTPHASE ePhase,
        /* [in] */ BOOL fCantDeny);
    
    virtual HRESULT STDMETHODCALLTYPE OnRowChange( 
        /* [in] */ IRowset *pRowset,
        /* [in] */ DBCOUNTITEM cRows,
        /* [size_is][in] */ const HROW rghRows[  ],
        /* [in] */ DBREASON eReason,
        /* [in] */ DBEVENTPHASE ePhase,
        /* [in] */ BOOL fCantDeny);
    
    virtual HRESULT STDMETHODCALLTYPE OnRowsetChange( 
        /* [in] */ IRowset *pRowset,
        /* [in] */ DBREASON eReason,
        /* [in] */ DBEVENTPHASE ePhase,
        /* [in] */ BOOL fCantDeny);

    // IOleClientSite methods
    HRESULT STDMETHODCALLTYPE SaveObject();
    HRESULT STDMETHODCALLTYPE GetMoniker(DWORD dwAssign,
            DWORD dwWhichMoniker,
            LPMONIKER FAR* ppmk);
    HRESULT STDMETHODCALLTYPE GetContainer(LPOLECONTAINER FAR* ppContainer);
    HRESULT STDMETHODCALLTYPE ShowObject();
    HRESULT STDMETHODCALLTYPE OnShowWindow(BOOL fShow);
    HRESULT STDMETHODCALLTYPE RequestNewObjectLayout();
    
    //  IServiceProvider methods
    HRESULT STDMETHODCALLTYPE QueryService(REFGUID guidService, REFIID iid, void ** ppv);

    // IUnknown methods
    STDMETHODIMP_(ULONG)    AddRef() { return ++_ulRefs; }
    STDMETHODIMP_(ULONG)    Release();
    STDMETHODIMP            QueryInterface(REFIID riid, void **ppv);

    // accessors
    CDataMemberMgr *  GetDataMemberManager() const { return _pDMembMgr; }
    CElement *      GetOwner() { Assert(_pDMembMgr); return _pDMembMgr->GetOwner(); }
    CDoc *          Doc();
    
private:
    HRESULT Fire_ondatasetcomplete(long lReason);
    ULONG           _ulRefs;            // reference count
    CDataMemberMgr  *_pDMembMgr;        // my data manager.  Fire events on its owner.
    IRowset         *_pRowset;          // the Rowset I'm listening to
    IRowPosition    *_pRowPos;          // the RowPosition I'm listening to
    IConnectionPoint *_pCPRowPos;       // Connection point for listening on RowPos
    IConnectionPoint *_pCPDBAsynch;       // Connection point for listening on Rowset
    IConnectionPoint *_pCPRowset;       // Connection point for listening on Rowset
    DWORD           _dwAdviseCookieRowPos; // saved from Advise, so I can Unadvise
    DWORD           _dwAdviseCookieDBAsynch; // saved from Advise, so I can Unadvise
    DWORD           _dwAdviseCookieRowset; // saved from Advise, so I can Unadvise
    CAdaptingProvider *_pProvider;      // the provider who owns me
    const HROW      *_rghRows;          // HROWs affected by change
    DBCOUNTITEM     _cRows;             // number of HROWs affected by change
    DBORDINAL      *_rgColumns;        // columns affected by change
    DBORDINAL       _cColumns;          // number of columns affected by change
    unsigned        _fCompleteFired:1;  // iff ondatasetcomplete already fired

    void SetEventData(const HROW *rghRows, DBCOUNTITEM cRows,
                      DBORDINAL *rgColumns, DBORDINAL cColumns)
    {
        _rghRows = rghRows;
        _cRows = cRows;
        _rgColumns = rgColumns;
        _cColumns = cColumns;
    }
    void SetEventData(const HROW *rghRows, DBCOUNTITEM cRows)
    {
        _rghRows = rghRows;
        _cRows = cRows;
    }
    void ClearEventData()
    {
        _rghRows = NULL;
        _cRows = 0;
        _rgColumns = NULL;
        _cColumns = 0;
    }
};


/////-------------------------------------------------------------------/////
/////                        Adapting provider                          /////
///// This abstract class is the base class for many of the concrete    /////
///// providers defined below.  It provides common behavior (code       /////
///// sharing), principally the management of interface adapters.       /////
/////-------------------------------------------------------------------/////

class CAdaptingProvider: public CDataSourceProvider
{
    typedef CDataSourceProvider super;
    friend class CDataSetEvents;
public:
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CAdaptingProvider))
    CAdaptingProvider() :
        _aryCR(Mt(CAdaptingProvider_aryCR_pv)),
        _aryPR(Mt(CAdaptingProvider_aryPR_pv)),
        _aryAR(Mt(CAdaptingProvider_aryAR_pv)),
        _aryXT(Mt(CAdaptingProvider_aryXT_pv))
        {}
    virtual ~CAdaptingProvider() { ReleaseAll(); }
    virtual void    Detach();
    virtual HRESULT QueryDataInterface(REFIID riid, void **ppv);
    virtual HRESULT ReplaceProvider(CDataSourceProvider *pdspNewProvider);
    virtual HRESULT Stop();
    virtual CXferThunk* GetXT(LPCTSTR strColumn, CVarType vtDest);

    // hierarchy
    virtual HRESULT GetSubProvider(CDataSourceProvider **ppdsp,
                        LPCTSTR bstrName, HROW hrow=0);
    virtual HCHAPTER GetChapter() { return _hChapter; }
    BOOL    IsTopLevel() const { return (_pProviderParent == NULL); }
    HRESULT ChangeChapter(HCHAPTER hChapterNew, HROW hRowNew);

    // events
    virtual HRESULT FireDataEvent(LPCTSTR pchType,
                            DISPID dispidEvent, DISPID dispidProp,
                            BOOL *pfCancelable = NULL,
                            long lReason = 0);
    virtual HRESULT FireDelayedRowEnter();
    virtual HRESULT get_bookmarks(IHTMLBookmarkCollection* *);
    virtual HRESULT get_recordset(IDispatch* *);
    virtual HRESULT get_dataFld(BSTR *);

    // accessors
    CDoc *  Doc() { return _pDoc; }
    IOleClientSite *    GetClientSite() { return (IOleClientSite*) _pRowEvents; }
    
    // helpers for plugging in interface adapters
    HRESULT AdaptRowsetOnRowPosition(IRowset** ppRowset, IRowPosition *pRowPos);
    HRESULT AdaptRowsetOnSTD(IRowset** ppRowset, OLEDBSimpleProvider *pSTD);
    HRESULT AdaptCursorOnRowset(ICursor** ppCursor, IRowset *pRowset);
    HRESULT AdaptCursorOnRowPosition(ICursor** ppCursor, IRowPosition *pRowPos);
    HRESULT AdaptRowPositionOnRowset(IRowPosition** ppRowPos, IRowset *pRowset);
    HRESULT AdaptADOOnRowset(IADORecordset** ppADO, IRowset *pRowset);
    HRESULT AdaptADOOnRowPos(IADORecordset** ppADO, IRowPosition *pRowPos);
    HRESULT AdaptRowsetOnCursor(IRowset** ppRowset, ICursor *pCursor);
    HRESULT SetADOClientSite(IADORecordset *pADO, IOleClientSite *pClientSite);

    // initialization
    HRESULT Init(CDataMemberMgr *pdmm, HCHAPTER hChapter=DB_NULL_HCHAPTER,
                 CAdaptingProvider *pProviderParent=NULL)
    {
        HRESULT hr;

        _pDoc = pdmm->Doc();
        _hChapter = hChapter;
        _pProviderParent = pProviderParent;

        // if I'm tied to a chapter, addref it
        if (_hChapter != DB_NULL_HCHAPTER && EnsureRowset())
        {
            IChapteredRowset *pChapRowset = NULL;

            if (S_OK == _pRowset->QueryInterface(IID_IChapteredRowset,
                        reinterpret_cast<void**>(&pChapRowset)))
            {
                IGNORE_HR(pChapRowset->AddRefChapter(_hChapter, NULL));
            }

            ReleaseInterface(pChapRowset);
        }

        // We're about to do things that fire events.  The event handlers
        // might run script that destroys the provider (e.g. ondatasetcomplete
        // handler resets the DSO's URL or query as in bug 57231).  So
        // stabilize to avoid dying during this kind of reentrancy.

        AddRef();
        
        // The order of the next three lines is critical.  We must ensure
        // the CRI before hooking up notifications, because SetRowChangedEventTarget
        // can (and often does) fire ondatasetchanged and ondatasetcomplete.
        // If the handlers for these events touch the rowset and cause
        // IRowsetNotify notificaitons, we need the DLCursor in place, and it's
        // owned by the CRI.  (See bug 59405.)
        //
        // However, we also need to hook up notifications before setting the
        // initial position of the CRI.  This is because setting CRI position
        // causes onrowenter to fire, and it's the CDataSetEvents that does
        // the firing.  (See bug 58708.)
        
        // always hook up CRI, so that we get the "adjust current record after
        // it's deleted" effect, even if there are no current-record-bound elements.
        EnsureCRI();

        // listen for notifications
        hr = SetRowChangedEventTarget(pdmm);

        // set the CRI to its initial position
        if (_pCRI)
            Doc()->GetDataBindTask()->InitCurrentRecord(_pCRI);

        Release();
        return hr;
    }

    // create from IRowPosition
    CAdaptingProvider(IRowPosition *pRowPos, HROW hrow): _pRowPos(pRowPos),
        _aryCR(Mt(CAdaptingProvider_aryCR_pv)),
        _aryPR(Mt(CAdaptingProvider_aryPR_pv)),
        _aryAR(Mt(CAdaptingProvider_aryAR_pv)),
        _aryXT(Mt(CAdaptingProvider_aryXT_pv))
    {
        _pRowPos->AddRef();
        _fNeedRowEnter =  (hrow != DB_NULL_HROW);
    }

    // create from IRowset
    CAdaptingProvider(IRowset *pRowset): _pRowset(pRowset),
        _aryCR(Mt(CAdaptingProvider_aryCR_pv)),
        _aryPR(Mt(CAdaptingProvider_aryPR_pv)),
        _aryAR(Mt(CAdaptingProvider_aryAR_pv)),
        _aryXT(Mt(CAdaptingProvider_aryXT_pv))
    {
        _pRowset->AddRef();
    }

    // create from ISimpleTabularData
    CAdaptingProvider(OLEDBSimpleProvider *pSTD): _pSTD(pSTD),
        _aryCR(Mt(CAdaptingProvider_aryCR_pv)),
        _aryPR(Mt(CAdaptingProvider_aryPR_pv)),
        _aryAR(Mt(CAdaptingProvider_aryAR_pv)),
        _aryXT(Mt(CAdaptingProvider_aryXT_pv))
    {
        _pSTD->AddRef();
    }

    // create from IVDBDC (ICursor generator)
    CAdaptingProvider(IVBDSC *pVBDSC) :
        _aryCR(Mt(CAdaptingProvider_aryCR_pv)),
        _aryPR(Mt(CAdaptingProvider_aryPR_pv)),
        _aryAR(Mt(CAdaptingProvider_aryAR_pv)),
        _aryXT(Mt(CAdaptingProvider_aryXT_pv))
    {
        IGNORE_HR(pVBDSC->CreateCursor(&_pCursor));
    }

protected:
    CDataSetEvents      *_pRowEvents; // sink for RowChanged events
    OLEDBSimpleProvider *_pSTD;     // ISTD interface
    IRowset             *_pRowset;  // IRowset interface
    IRowPosition        *_pRowPos;  // IRowPosition interface
    ICursor             *_pCursor;  // ICursor interface
    IADORecordset       *_pADO;     // ADO interface
    CCurrentRecordInstance  *_pCRI; // Current record interface
    CAdaptingProvider   *_pProviderParent;  // my parent provider (hierarchy)
    ISimpleDataConverter *_pSDC;    // DSO's simple data converter (if any)

    struct CXTRecord {          // map <field, type> to <XT>
        BSTR                bstrField;
        CVarType            vtDest;
        CXferThunk *        pXT;
    };

    // support for hierarchical rowsets
    struct CChapterRecord {     // map <hrow, field> to <chapter>
        HROW                hrow;
        BSTR                bstrField;
        HCHAPTER            hChapter;
    };
    
    struct CProviderRecord {    // map <field, chapter> to <provider>
        BSTR                bstrField;
        HCHAPTER            hChapter;
        CDataSourceProvider *pdsp;
    };

    struct CAccessorRecord {    // map <field> to <accessor, rowset>
        BSTR        bstrField;
        HACCESSOR   hAccessor;
        IRowset *   pRowset;
        DB_LORDINAL ulOrdinal;  // internal use only
    };

    CDataAry<CXTRecord>         _aryXT;
    CDataAry<CChapterRecord>    _aryCR;
    CDataAry<CProviderRecord>   _aryPR;
    CDataAry<CAccessorRecord>   _aryAR;
    HCHAPTER                    _hChapter;  // restrict to this chapter
    
    unsigned        _fNeedRowEnter:1;   // iff we need to fire onrowenter
    unsigned        _fRowEnterOK:1;     // iff we can fire onrowenter now

    // hierarchy methods
    HRESULT GetChapterFromField(HROW hrow, BSTR bstrField, HCHAPTER *phChapter);
    HRESULT GetProviderFromField(BSTR bstrField, HROW hrow,
                                CDataSourceProvider **ppdsp);
    HRESULT GetAccessorAndRowsetForField(BSTR bstrField,
                                HACCESSOR *phAccessor, IRowset **ppRowset);
    HRESULT GetAccessorForField(BSTR bstrField, HACCESSOR *phAccessor)
            { return GetAccessorAndRowsetForField(bstrField, phAccessor, NULL); }
    HRESULT GetRowsetForField(BSTR bstrField, IRowset **ppRowset)
            { return GetAccessorAndRowsetForField(bstrField, NULL, ppRowset); }
    HRESULT InitializeProviderRecord(CProviderRecord *pPR,
                        HCHAPTER hChapter, BSTR bstrField);
    HRESULT InitializeAccessorRecord(CAccessorRecord *pAR, BSTR bstrField);
    BOOL    EnsureAccessorInAccessorRecord(CAccessorRecord *pAR);
    BOOL    EnsureRowsetInAccessorRecord(CAccessorRecord *pAR);
    static HRESULT CreateSubProvider(CAdaptingProvider *pProviderParent,
                                    IRowset *pChildRowset, IRowPosition *pChildRowPos,
                                    CDataMemberMgr *pdmm,
                                    HCHAPTER hChapter, HROW hrow,
                                    CDataSourceProvider **ppdsp);
    void    RemoveSubProvider(CDataSourceProvider *pdsp);
    HRESULT UpdateProviderRecords(HCHAPTER hChapterOld, CDataSourceProvider *pdspOld,
                                HCHAPTER hChapterNew, CDataSourceProvider *pdspNew);
    
    // helper methods
    HRESULT SetRowChangedEventTarget(CDataMemberMgr *pdmm);
    void    ReleaseAll();
    CDataLayerCursor * GetDLCursor() { Assert(_pCRI); return _pCRI->GetDLCursor(); }
    BOOL    IsSameRow(HROW hrow1, HROW hrow2)
                { return GetDLCursor()->IsSameRow(hrow1, hrow2); }
private:
    // helper methods to generate adapters, if necessary
    BOOL    EnsureSTD();
    BOOL    EnsureRowset();
    BOOL    EnsureRowsetPosition();
    BOOL    EnsureCursor();
    BOOL    EnsureADO();
    BOOL    EnsureCRI();
};


inline CDoc *
CDataSetEvents::Doc()  { Assert(_pDMembMgr); return _pDMembMgr->Doc(); }

//+-------------------------------------------------------------------------
// Member:      Init (CDataSetEvents, public)
//
// Synopsis:    listen to the given IRowPosition, and remember the OleSite

HRESULT
CDataSetEvents::Init(CDataMemberMgr *pdmm, IRowset *pRowset, IRowPosition *pRowPos,
                    CAdaptingProvider *pProvider)
{
    Assert(pdmm && pProvider);

    HRESULT hr = S_OK, hrFire = S_OK;
    IConnectionPointContainer *pCPC = 0;
    
    _pProvider = pProvider;

    // We can be called with a NULL pRowPos, but not with a NULL pRowset.
    Assert(pRowset);

    // hold on to the RowSet (let go in Detach)
    _pRowset = pRowset;
    _pRowset->AddRef();

    // hold on to the data member manager
    _pDMembMgr = pdmm;
    _pDMembMgr->AddRef();

    // sink notifications from the RowPosition, if we have one
    if (pRowPos)
    {
        // hold on to the RowPosition (let go in Detach)
        _pRowPos = pRowPos;
        _pRowPos->AddRef();

        if (S_OK == _pRowPos->QueryInterface(IID_IConnectionPointContainer,
                                           (void **)&pCPC )
           &&
           S_OK == pCPC->FindConnectionPoint(IID_IRowPositionChange, &_pCPRowPos))
        {
            if (FAILED(_pCPRowPos->Advise((IRowPositionChange *)this, &_dwAdviseCookieRowPos)))
                ClearInterface(&_pCPRowPos);
        }
    }

    ClearInterface(&pCPC);             // re-use connection point container

    // We fire the script event from all the way down here, because
    // if we waited until after we hooked up the Rowset event sink,
    // the Rowset might cause ondataavailable or ondatasetcomplete
    // to be fired before we fired datasetchanged.

    // Fire datasetchanged to tell script code we just got a new
    // dataset.  Note this fires for the initial dataset and for
    // any subsequent datasets.
    if (_pProvider->IsTopLevel())
    {
        hrFire = pProvider->FireDataEvent(_T("datasetchanged"),
                                        DISPID_EVMETH_ONDATASETCHANGED,
                                        DISPID_EVPROP_ONDATASETCHANGED);
    }

    // sink notifications from the Rowset
    if (S_OK == pRowset->QueryInterface(IID_IConnectionPointContainer,
                                        (void **)&pCPC ))
    {
        if (S_OK == pCPC->FindConnectionPoint(IID_IDBAsynchNotify, &_pCPDBAsynch))
        {
            if (FAILED(_pCPDBAsynch->Advise((IDBAsynchNotify *)this, &_dwAdviseCookieDBAsynch)))
            {
                ClearInterface(&_pCPDBAsynch);
            }
        }
        
        if (S_OK == pCPC->FindConnectionPoint(IID_IRowsetNotify, &_pCPRowset))
        {
            if (FAILED(_pCPRowset->Advise((IRowsetNotify *)this, &_dwAdviseCookieRowset)))
            {
                ClearInterface(&_pCPRowset);
            }
        }
    }

    // The Rowset may or may not have fired IDBAsynchNotify::OnStop, during,
    // or since, our Advise call.  If it did, we're done.  If not, then we
    // want to ask the rowset if it was complete, in which case we can fire
    // it for it.
    if (!_fCompleteFired)
    {
        DBASYNCHPHASE ulStatusCode = DBASYNCHPHASE_COMPLETE;
        IDBAsynchStatus *pAsynchStatus;
        DBCOUNTITEM ulProgress, ulProgressMax;

        if (_pRowset &&
            S_OK == _pRowset->QueryInterface(IID_IDBAsynchStatus,
                                             (void **)&pAsynchStatus))
        {
            // Get the Rowset's ulStatusCode
            // (We don't care about ulProgress & ulProgressMax, but I was afraid
            // some OLE-DB providers might crash if we passed in NULL).
            pAsynchStatus->GetStatus(_pProvider->GetChapter(), DBASYNCHOP_OPEN,
                                     &ulProgress, &ulProgressMax,
                                     &ulStatusCode, NULL);

            ClearInterface(&pAsynchStatus);
        }

        if (DBASYNCHPHASE_COMPLETE == ulStatusCode)
        {
            hr = Fire_ondatasetcomplete(0);
        }
    }

    ClearInterface(&pCPC);

    // If we got no other errors, but the Fire OnDataSetChanged event returned
    // an hr, then use that for our hr.  It probably means some wise-ass script
    // code triggered an ondatasetchanged event in inside the ondatasetchanged
    // handler, and blew the stack.  Checking this hr should stop it.
    if (S_OK == hr)
        hr = hrFire;

    return hr;
}


//+-------------------------------------------------------------------------
// Member:      destructor (CDataSetEvents, public)
//
// Synopsis:    stop listening to my RowPosition, release my OleSite

CDataSetEvents::~CDataSetEvents()
{
    // let go of my data member manager
    if (_pDMembMgr)
    {
        _pDMembMgr->Release();
        _pDMembMgr = NULL;
    }
}


//+-------------------------------------------------------------------------
// Member:      Detach (CDataSetEvents, public)
//
// Synopsis:    stop listening to my RowPosition, release my OleSite

HRESULT
CDataSetEvents::Detach()
{
    HRESULT hr = S_OK;

    // stop listening for events
    if (_pCPRowPos)
    {
        IGNORE_HR(_pCPRowPos->Unadvise(_dwAdviseCookieRowPos));
        ClearInterface(&_pCPRowPos);
    }

    // stop listening for events
    if (_pCPDBAsynch)
    {
        IGNORE_HR(_pCPDBAsynch->Unadvise(_dwAdviseCookieDBAsynch));
        ClearInterface(&_pCPDBAsynch);
    }
    if (_pCPRowset)
    {
        IGNORE_HR(_pCPRowset->Unadvise(_dwAdviseCookieRowset));
        ClearInterface(&_pCPRowset);
    }

    // let go of the Rowset
    ClearInterface(&_pRowset);    
    
    // let go of the RowPosition
    ClearInterface(&_pRowPos);

    // let go of my provider
    if (_pProvider)
    {
        _pProvider = NULL;
    }
    
    return hr;
}



//+-------------------------------------------------------------------------
// Member:      OnRowPositionChange (IRowPositionChange, public)
//
// Synopsis:    translate IRowPosition notifications into
//              onRowExit/Enter events on my OLE site.

HRESULT
CDataSetEvents::OnRowPositionChange(DBREASON eReason, DBEVENTPHASE ePhase,
                                        BOOL fCantDeny)
{
    HCHAPTER hchapter = DB_NULL_HCHAPTER;
    HROW hrow = DB_NULL_HROW;
    DBPOSITIONFLAGS dwPositionFlags;
    HRESULT hr = CheckCallbackThread();
    BOOL fCancelled;

    _pProvider->AddRef();               // stabilize

    if (hr)
        goto Cleanup;

    switch (ePhase)
    {
    case DBEVENTPHASE_ABOUTTODO:        // position is changing, fire onRowExit
        _pRowPos->GetRowPosition(&hchapter, &hrow, &dwPositionFlags);
        if (hrow != DB_NULL_HROW)
        {
            TraceTag((tagRowEvents, "%p -> onRowExit", _pDMembMgr->GetOwner()));
            
            _pProvider->FireDataEvent(_T("rowexit"),
                            DISPID_EVMETH_ONROWEXIT, DISPID_EVPROP_ONROWEXIT,
                            &fCancelled);
            if (fCancelled)
            {
                hr = S_FALSE;
            }
        }
        break;
        
    case DBEVENTPHASE_SYNCHAFTER:
        switch (eReason)
        {
        case DBREASON_ROWPOSITION_CHAPTERCHANGED:
            // the provider is effectively a tearoff rowset on a specific
            // chapter, so when the chapter changes we need to replace
            // the provider
            Assert(_pProvider);
            _pRowPos->GetRowPosition(&hchapter, &hrow, &dwPositionFlags);
            IGNORE_HR(_pProvider->ChangeChapter(hchapter, hrow));
            break;

        default:
            break;
        }
        break;
    
    case DBEVENTPHASE_DIDEVENT:         // position changed, fire onRowEnter
        switch (eReason)
        {
        case DBREASON_ROWPOSITION_CHANGED:
            if (_pProvider->_fRowEnterOK)
            {
                TraceTag((tagRowEvents, "%p -> onRowEnter", _pDMembMgr->GetOwner()));
                _pProvider->FireDataEvent(_T("rowenter"),
                                    DISPID_EVMETH_ONROWENTER,
                                    DISPID_EVPROP_ONROWENTER);
            }
            else
            {
                _pProvider->_fNeedRowEnter = TRUE;
            }
            break;

        default:
            break;
        }
        break;
    }
    
Cleanup:
    // if we acquired an hrow or hchapter from the row position, release it now
    ReleaseChapterAndRow(hchapter, hrow, _pRowPos);
    
    _pProvider->Release();
    
    RRETURN1(hr, S_FALSE);
}

//  IDBAsynchNotify methods
HRESULT STDMETHODCALLTYPE
CDataSetEvents::OnLowResource( 
            /* [in] */ DB_DWRESERVE dwReserved)
{
    return E_NOTIMPL;
}

HRESULT
CDataSetEvents::Fire_ondatasetcomplete(long lReason)
{
    HRESULT hr = S_OK;
    if (_pProvider && !_fCompleteFired)
    {
        _fCompleteFired = TRUE;

        // only fire for top-level provider
        if (_pProvider->IsTopLevel())
        {
            hr = _pProvider->FireDataEvent(_T("datasetcomplete"),
                                        DISPID_EVMETH_ONDATASETCOMPLETE,
                                        DISPID_EVPROP_ONDATASETCOMPLETE,
                                        NULL, lReason);
        }
    }    
    return hr;
}

HRESULT STDMETHODCALLTYPE
CDataSetEvents::OnProgress( 
            /* [in] */ HCHAPTER hChapter,
            /* [in] */ DBASYNCHOP ulOperation,
            /* [in] */ DBCOUNTITEM ulProgress,
            /* [in] */ DBCOUNTITEM ulProgressMax,
            /* [in] */ DBASYNCHPHASE ulStatusCode,
            /* [in] */ LPOLESTR pwszStatusText)
{
    HRESULT hr = CheckCallbackThread();
 
    if (hr)
        goto Cleanup;
        
    if (_pProvider && ulStatusCode==DBASYNCHPHASE_POPULATION && !_fCompleteFired)
    {
        // good place to fire script event
        hr = _pProvider->FireDataEvent(_T("dataavailable"),
                                        DISPID_EVMETH_ONDATAAVAILABLE,
                                        DISPID_EVPROP_ONDATAAVAILABLE);
    }

Cleanup:
    return S_OK;                        // only allowed OLEDB return here
 }

STDMETHODIMP
CDataSetEvents:: OnStop( 
            /* [in] */ HCHAPTER hChapter,
            /* [in] */ ULONG ulOperation,
            /* [in] */ HRESULT hrStatus,
            /* [in] */ LPOLESTR pwszStatusText)
{
    long lReason;
    HRESULT hr = CheckCallbackThread();

    if (hr)
        goto Cleanup;
        

    switch (hrStatus)
    {
        case DB_E_CANCELED:
            lReason = 1;                // Abort
            break;

        case E_FAIL:
            lReason = 2;                // Error
            break;

        case S_OK:
        default:
            lReason = 0;
            break;
    }

    Fire_ondatasetcomplete(lReason);

Cleanup:
    RRETURN(hr);
}


// IRowsetNotify methods

STDMETHODIMP
CDataSetEvents::OnFieldChange( 
        /* [in] */ IRowset *pRowset,
        /* [in] */ HROW hRow,
        /* [in] */ DBORDINAL cColumns,
        /* [size_is][in] */ DBORDINAL rgColumns[  ],
        /* [in] */ DBREASON eReason,
        /* [in] */ DBEVENTPHASE ePhase,
        /* [in] */ BOOL fCantDeny)
{
    HRESULT hr;
    DBROWCOUNT cRowsGood;
    const HROW *pRowsGood;

    switch (eReason)
    {
    case DBREASON_COLUMN_SET:
        switch (ePhase)
        {
        case DBEVENTPHASE_ABOUTTODO:
            // first make sure the row belongs to my chapter
            hr = _pProvider->GetDLCursor()->FilterRowsToChapter(1, &hRow,
                                                    &cRowsGood, &pRowsGood);
            if (!hr && cRowsGood != 0)
            {
                Assert(pRowsGood == &hRow); // no allocation for only one row
                SetEventData(&hRow, 1, rgColumns, cColumns);
                
                _pProvider->FireDataEvent(_T("cellchange"),
                                DISPID_EVMETH_ONCELLCHANGE, DISPID_EVPROP_ONCELLCHANGE);

                ClearEventData();
            }
            hr = S_OK;
            break;
        default:
            hr = DB_S_UNWANTEDPHASE;
            break;
        }
        break;

    default:
        hr = DB_S_UNWANTEDREASON;
        break;
    }
    
    return hr;
}

    
STDMETHODIMP
CDataSetEvents::OnRowChange( 
        /* [in] */ IRowset *pRowset,
        /* [in] */ DBCOUNTITEM cRows,
        /* [size_is][in] */ const HROW rghRows[  ],
        /* [in] */ DBREASON eReason,
        /* [in] */ DBEVENTPHASE ePhase,
        /* [in] */ BOOL fCantDeny)
{
    HRESULT hr = S_OK;
    DBROWCOUNT cRowsGood;
    const HROW *pRowsGood = rghRows;

    // ignore rows that don't belong to my chapter
    hr = _pProvider->GetDLCursor()->FilterRowsToChapter(cRows, rghRows,
                                                    &cRowsGood, &pRowsGood);
    if (hr || cRowsGood == 0)
        goto Cleanup;

    switch (eReason)
    {
    case DBREASON_ROW_DELETE:
    case DBREASON_ROW_UNDOINSERT:
        switch (ePhase)
        {
        case DBEVENTPHASE_ABOUTTODO:
            SetEventData(pRowsGood, cRowsGood);
            
            _pProvider->FireDataEvent(_T("rowsdelete"),
                            DISPID_EVMETH_ONROWSDELETE, DISPID_EVPROP_ONROWSDELETE);

            ClearEventData();
            hr = S_OK;
            break;
        default:
            hr = DB_S_UNWANTEDPHASE;
            break;
        }
        break;
    
    case DBREASON_ROW_INSERT:
    case DBREASON_ROW_ASYNCHINSERT:
    case DBREASON_ROW_UNDODELETE:
        switch (ePhase)
        {
        case DBEVENTPHASE_DIDEVENT:
            SetEventData(pRowsGood, cRowsGood);
            
            _pProvider->FireDataEvent(_T("rowsinserted"),
                            DISPID_EVMETH_ONROWSINSERTED, DISPID_EVPROP_ONROWSINSERTED);

            ClearEventData();
            hr = S_OK;
            break;
        default:
            hr = DB_S_UNWANTEDPHASE;
            break;
        }
        break;

    default:
        hr = DB_S_UNWANTEDREASON;
        break;
    }

Cleanup:
    if (pRowsGood != rghRows)
        delete const_cast<HROW *>(pRowsGood);
    
    return hr;
}

    
STDMETHODIMP
CDataSetEvents::OnRowsetChange( 
        /* [in] */ IRowset *pRowset,
        /* [in] */ DBREASON eReason,
        /* [in] */ DBEVENTPHASE ePhase,
        /* [in] */ BOOL fCantDeny)
{
    return DB_S_UNWANTEDREASON;
}


/////////////      IOleClientSite methods     ////////////////////////////
// We only need these so we can give ADO a client site (so it can determine
// its security behavior).  But ADO only calls GetContainer, to get to the
// Doc, and QI(ServiceProvider).

STDMETHODIMP
CDataSetEvents::SaveObject()
{ return E_NOTIMPL; }

STDMETHODIMP
CDataSetEvents::GetMoniker(DWORD dwAssign,
        DWORD dwWhichMoniker,
        LPMONIKER FAR* ppmk)
{ return E_NOTIMPL; }

STDMETHODIMP
CDataSetEvents::GetContainer(LPOLECONTAINER FAR* ppContainer)
{
    HRESULT hr;
    hr = THR(Doc()->QueryInterface(IID_IOleContainer, (void **)ppContainer));
    RRETURN(hr);
}

STDMETHODIMP
CDataSetEvents::ShowObject()
{ return E_NOTIMPL; }

STDMETHODIMP
CDataSetEvents::OnShowWindow(BOOL fShow)
{ return E_NOTIMPL; }

STDMETHODIMP
CDataSetEvents::RequestNewObjectLayout()
{ return E_NOTIMPL; }


/////////////      IServiceProvider methods     ////////////////////////////
// We delegate all requests (from ADO) right to the Doc

STDMETHODIMP
CDataSetEvents::QueryService(REFGUID guidService, REFIID iid, void ** ppv)
{
    RRETURN(Doc()->QueryService(guidService, iid, ppv));
}


HRESULT
CDataSetEvents::CheckCallbackThread()
{
    HRESULT hr = S_OK;

    if (_pDMembMgr
        && _pDMembMgr->Doc()
        && _pDMembMgr->Doc()->_dwTID != GetCurrentThreadId() )
    {
        Assert(!"OLEDB callback on wrong thread");
        hr = E_UNEXPECTED;
    }

    RRETURN(hr);
}

//+-------------------------------------------------------------------------
// Member:      Release (public, IUnknown)
//
// Synopsis:    decrease refcount, die if 0
//
// Returns:     new refcount

ULONG
CDataSetEvents::Release()
{
    ULONG ulRefs = --_ulRefs;
    if (ulRefs == 0)
        delete this;
    return ulRefs;
}


//+-------------------------------------------------------------------------
// Member:      Query Interface (public, IUnknown)
//
// Synopsis:    return desired interface pointer
//
// Arguments:   riid        IID of desired interface
//              ppv         where to store the pointer
//
// Returns:     HRESULT

HRESULT
CDataSetEvents::QueryInterface(REFIID riid, void **ppv)
{
    HRESULT hr;
    IUnknown *punkReturn = 0;

    // check for bad arguments
    if (!ppv)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    // look for interfaces I support
    if (IsEqualIID(riid, IID_IUnknown))
    {
        // We inherit IUnknown from both IRowPositionChange & IDBAsynchNotify,
        // We can arbitrarily return the IUknown from one of them..
        punkReturn = (IRowPositionChange *)this;
    }
    else if (IsEqualIID(riid, IID_IRowPositionChange))
    {
        punkReturn = (IRowPositionChange *)this;
    }
    else if (IsEqualIID(riid, IID_IRowsetNotify))
    {
        punkReturn = (IRowsetNotify *)this;
    }
    else if (IsEqualIID(riid, IID_IDBAsynchNotify))
    {
        punkReturn = (IDBAsynchNotify *)this;
    }
    else if (IsEqualIID(riid, IID_IOleClientSite))
    {
        punkReturn = (IOleClientSite *)this;
    }
    else if (IsEqualIID(riid, IID_IServiceProvider))
    {
        punkReturn = (IServiceProvider *)this;
    }
    else
    {
        hr = E_NOINTERFACE;
        goto Cleanup;
    }
    punkReturn->AddRef();          // return addref'd copy
    *ppv = punkReturn;
    hr = S_OK;

Cleanup:
    return hr;        
}


/////////////////////////////////////////////////////////////////////////////
/////                       Helper functions                            /////
/////////////////////////////////////////////////////////////////////////////

//+-------------------------------------------------------------------------
// Function:    Adapt Rowset On RowPosition (static helper)
//
// Synopsis:    point pRowset at a rowset obtained from the RowPosition object.
//              If pRowset already points at an adapter, don't change it.
//
// Arguments:   ppRowset    [out] where to store adapter
//              pSTD        [in] pointer to ISTD

HRESULT
CAdaptingProvider::AdaptRowsetOnRowPosition(IRowset** ppRowset, IRowPosition *pRowPos)
{
    HRESULT hr = S_OK;
    Assert("RowPos expected" && pRowPos);

    // if adapter already exists, nothing to do
    if (*ppRowset)
        goto Cleanup;

    // get its IRowset interface
    hr = pRowPos->GetRowset(IID_IRowset, (IUnknown**)ppRowset);
    if (hr)
        goto Cleanup;
    Assert(*ppRowset);

Cleanup:
    return hr;
}


//+-------------------------------------------------------------------------
// Function:    Adapt Rowset On STD (static helper)
//
// Synopsis:    point pRowset at an object that adapts IRowset to OLEDBSimpleProvider.
//              If pRowset already points at an adapter, don't change it.
//
// Arguments:   ppRowset    [out] where to store adapter
//              pOSP        [in] pointer to OLEDBSimpleProvider

#if defined(USE_PW_DLL)
HRESULT
CAdaptingProvider::AdaptRowsetOnSTD(IRowset** ppRowset, OLEDBSimpleProvider *pOSP)
{
    HRESULT hr = S_OK;
    IUnknown *pUnkRowset = 0;
    Assert("OSP expected" && pOSP);
    static const CLSID clsidPW = {0xdfc8bdc0, 0xe378, 0x11d0, 0x9b, 0x30, 0x00, 0x80, 0xc7, 0xe9, 0xfe, 0x95};
    static const GUID DBPROPSET_PWROWSET = {0xe6e478db,0xf226,0x11d0,{0x94,0xee,0x0,0xc0,0x4f,0xb6,0x6a,0x50}};
    const DWORD PWPROP_OSPVALUE = 2;
    DBID dbidTable;
    DBPROPSET rgdbpropset[1];
    DBPROP rgprop[1];
    IDBInitialize *pIDBInitialize = NULL;
    IDBCreateSession *pIDBCreateSession = NULL;
    IOpenRowset *pIOpenRowset = NULL;

    // if adapter already exists, nothing to do
    if (*ppRowset)
        goto Cleanup;

    // Instantiate the OSP->Rowset adapter
    hr = CoCreateInstance(clsidPW,
                            NULL,
                            CLSCTX_ALL,
                            IID_IDBInitialize,
                            (void **)&pIDBInitialize);
    if (hr)
        goto Cleanup;

    hr = pIDBInitialize->Initialize();
    if (hr)
        goto Cleanup;

    hr = pIDBInitialize->QueryInterface(IID_IDBCreateSession,
                                        (void **)&pIDBCreateSession);
    if (hr)
        goto Cleanup;

    hr = pIDBCreateSession->CreateSession(NULL, IID_IOpenRowset,
                                            (IUnknown **)&pIOpenRowset);
    if (hr)
        goto Cleanup;

    // Set OSPVALUE property
    rgdbpropset[0].guidPropertySet = DBPROPSET_PWROWSET;
    rgdbpropset[0].cProperties = 1;
    rgdbpropset[0].rgProperties = &rgprop[0];

    rgprop[0].dwPropertyID = PWPROP_OSPVALUE;
    rgprop[0].dwOptions = DBPROPOPTIONS_REQUIRED;
    rgprop[0].dwStatus = DBPROPSTATUS_OK;
    // rgprop[0].colid = DB_NULLID;
    memset(&rgprop[0].colid, 0, sizeof(rgprop[0].colid));
    VariantInit(&(rgprop[0].vValue));
    V_VT(&(rgprop[0].vValue)) = VT_UNKNOWN;
    V_UNKNOWN(&(rgprop[0].vValue)) = (IUnknown *)pOSP;

    dbidTable.eKind = DBKIND_NAME;
    dbidTable.uName.pwszName = NULL;

    hr = pIOpenRowset->OpenRowset(NULL,
                                    &dbidTable,
                                    NULL,
                                    IID_IRowset,
                                    1,
                                    rgdbpropset,
                                    (IUnknown **)ppRowset);
    if (hr)
        goto Cleanup;

Cleanup:
    ReleaseInterface(pIDBInitialize);
    ReleaseInterface(pIDBCreateSession);
    ReleaseInterface(pIOpenRowset);

    return hr;
}
#else // USE_PW_DLL
HRESULT
CAdaptingProvider::AdaptRowsetOnSTD(IRowset** ppRowset, OLEDBSimpleProvider *pOSP)
{
    HRESULT hr = S_OK;
    IUnknown *pUnkRowset = 0;
    Assert("OSP expected" && pOSP);

    // if adapter already exists, nothing to do
    if (*ppRowset)
        goto Cleanup;

    hr = CTopRowset::CreateRowset(pOSP, &pUnkRowset);
    if (hr)
        goto Cleanup;

    // get its IRowset interface
    hr = pUnkRowset->QueryInterface(IID_IRowset, (void**)ppRowset);
    if (hr)
        goto Cleanup;
    Assert(*ppRowset);

Cleanup:
    ReleaseInterface(pUnkRowset);
    return hr;
}
#endif USE_PW_DLL


//+-------------------------------------------------------------------------
// Function:    Adapt Cursor On Rowset (static helper)
//
// Synopsis:    point pCursor at an object that adapts ICursor to IRowset.
//              If pCursor already points at an adapter, don't change it.
//
// Arguments:   ppCursor    [out] where to store adapter
//              pRowset     [in] pointer to IRowset

HRESULT
CAdaptingProvider::AdaptCursorOnRowset(ICursor** ppCursor, IRowset *pRowset)
{
    HRESULT hr = S_OK;
    Assert("rowset expected" && pRowset);
    ICursorFromRowset *pIcfr = 0;

    // if adapter already exists, nothing to do
    if (*ppCursor)
        goto Cleanup;

    // create an adapter
    hr = CoCreateInstance(CLSID_CCursorFromRowset, NULL,
                          CLSCTX_INPROC, IID_ICursorFromRowset,
                          (void **)&pIcfr);
    if (hr) 
        goto Cleanup;
        
    // get its ICursor interface
    hr = THR(pIcfr->GetCursor(pRowset, ppCursor, g_lcidUserDefault));
    if (hr)
        goto Cleanup;

Cleanup:
    ReleaseInterface(pIcfr);
    return hr;
}


//+-------------------------------------------------------------------------
// Function:    Adapt Cursor On Row Position (static helper)
//
// Synopsis:    point pCursor at an object that adapts ICursor to IRowset.
//              If pCursor already points at an adapter, don't change it.
//
// Arguments:   ppCursor    [out] where to store adapter
//              pRowset     [in] pointer to IRowset

HRESULT
CAdaptingProvider::AdaptCursorOnRowPosition(ICursor** ppCursor, IRowPosition *pRowPos)
{
    HRESULT hr = S_OK;
#ifdef VD_INCLUDE_ROWPOSITION
    Assert("rowposition expected" && pRowPos);
    ICursorFromRowPosition *pIcfr = 0;

    // if adapter already exists, nothing to do
    if (*ppCursor)
        goto Cleanup;

    // create an adapter
    hr = CoCreateInstance(CLSID_CCursorFromRowset, NULL,
                          CLSCTX_INPROC, IID_ICursorFromRowPosition,
                          (void **)&pIcfr);
    if (hr) 
        goto Cleanup;
        
    // get its ICursor interface
    hr = THR(pIcfr->GetCursor(pRowPos, ppCursor, g_lcidUserDefault));
    if (hr)
        goto Cleanup;

Cleanup:
    ReleaseInterface(pIcfr);
#endif // VD_INCLUDE_ROWPOSITION
    return hr;
}


//+-------------------------------------------------------------------------
// Function:    GetADOConstructor (static helper)
//
// Synopsis:    point pADO at an object that adapts IADORecordset to IRowset.
//              point pADOConstructor at its constructor interface
//
// Arguments:   ppADO               [out] where to store adapter
//              ppADOConstructor    [out] where to store constructor

static HRESULT
GetADOConstructor(IADORecordset **ppADO,
                  IADORecordsetConstruction **ppADOConstructor,
                  CAdaptingProvider *pProvider)
{
    HRESULT hr;
    IADOSecurity *pADOSecurity = NULL;
    IADORecordset *pADO = NULL;
    IObjectSafety *pObjSafety = NULL;
    static const IID IID_IADOSecurity = {0x782d16ae, 0x905f, 0x11d1, 0xac, 0x38, 0x0, 0xc0, 0x4f, 0xc2, 0x9f, 0x8f};
    
    // create an adapter
    hr = CoCreateInstance(CLSID_CADORecordset, NULL,
                          CLSCTX_INPROC, IID_IADORecordset15,
                          (void **)&pADO);
    if (hr) 
        goto Cleanup;

    // tell it to act safely
    if (pProvider)
    {
        // ADO 2.1 uses ClientSite / ObjectSafety.  Try that first.
        
        // give ADO a client site, so it can post security dialogs
        hr = pProvider->SetADOClientSite(pADO, pProvider->GetClientSite());

        // now tell it to act safely
        if (!hr)
            hr = pADO->QueryInterface(IID_IObjectSafety, (void**)&pObjSafety);
        if (!hr && pObjSafety)
        {
            // make it safe to init on IPersistPropertyBag
            IGNORE_HR(pObjSafety->SetInterfaceSafetyOptions(IID_IPersistPropertyBag,
                                INTERFACESAFE_FOR_UNTRUSTED_DATA,
                                INTERFACESAFE_FOR_UNTRUSTED_DATA));

            // make it safe to script on IDispatchEx or IDispatch
            hr = pObjSafety->SetInterfaceSafetyOptions(IID_IDispatchEx,
                                INTERFACESAFE_FOR_UNTRUSTED_CALLER,
                                INTERFACESAFE_FOR_UNTRUSTED_CALLER);
            if (hr)
                hr = pObjSafety->SetInterfaceSafetyOptions(IID_IDispatch,
                                INTERFACESAFE_FOR_UNTRUSTED_CALLER,
                                INTERFACESAFE_FOR_UNTRUSTED_CALLER);

            ReleaseInterface(pObjSafety);

            hr = S_OK;      // we tried our best.  If that failed, just carry on.
        }

        // If that didn't work, maybe it's because ADO 1.5 is installed.
        // So try the old private interface approach.
        if (hr && S_OK == pADO->QueryInterface(IID_IADOSecurity, (void**)&pADOSecurity))
        {
            BSTR bstrURL = NULL;

            hr = pADOSecurity->SetSafe(TRUE);
            
            if (!hr)
                hr = pProvider->Doc()->get_URL(&bstrURL);

            if (!hr)
                hr = pADOSecurity->SetURL(bstrURL);

            FormsFreeString(bstrURL);
            ReleaseInterface(pADOSecurity);

            if (hr)
            {
                ReleaseInterface(pADO);
                goto Cleanup;
            }
        }
    }

    *ppADO = pADO;
    
    // get its IADORecordsetConstruction interface
    hr = pADO->QueryInterface(IID_IADORecordsetConstruction,
                                (void **)ppADOConstructor);

Cleanup:
    return hr;
}


//+-------------------------------------------------------------------------
// Function:    Adapt ADO On Rowset (static helper)
//
// Synopsis:    point pADO at an object that adapts IADORecordset to IRowset.
//              If pADO already points at an adapter, don't change it.
//
// Arguments:   ppADO       [out] where to store adapter
//              pRowset     [in] pointer to IRowset

HRESULT
CAdaptingProvider::AdaptADOOnRowset(IADORecordset** ppADO, IRowset *pRowset)
{
    HRESULT hr = S_OK;
    IADORecordset *pADO = 0;
    IADORecordsetConstruction *pAdoConstructor = 0;
    Assert("rowset expected" && pRowset);
    
    // if adapter already exists, nothing to do
    if (*ppADO)
        goto Cleanup;

    // create an adapter
    hr = GetADOConstructor(&pADO, &pAdoConstructor, this);
    if (hr)
        goto Cleanup;

    // return addref'd result to caller
    pADO->AddRef();
    *ppADO = pADO;
        
    // give it the rowset.  This may fire onrowenter, which might ask for
    // event.recordset, so we must set *ppADO first.
    hr = THR(pAdoConstructor->put_Rowset(pRowset));
    if (hr)
        goto Cleanup;

Cleanup:
    ReleaseInterface(pADO);
    ReleaseInterface(pAdoConstructor);
    return hr;
}

//+-------------------------------------------------------------------------
// Function:    Adapt ADO On RowPosition (static helper)
//
// Synopsis:    point pADO at an object that adapts IADORecordset to IRowset.
//              If pADO already points at an adapter, don't change it.
//
// Arguments:   ppADO       [out] where to store adapter
//              pRowset     [in] pointer to IRowset

HRESULT
CAdaptingProvider::AdaptADOOnRowPos(IADORecordset** ppADO, IRowPosition *pRowPos)
{
    HRESULT hr = S_OK;
    IADORecordset *pADO = 0;
    IADORecordsetConstruction *pAdoConstructor = 0;
    Assert("rowpos expected" && pRowPos);
    
    // if adapter already exists, nothing to do
    if (*ppADO)
        goto Cleanup;

    // create an adapter
    hr = GetADOConstructor(&pADO, &pAdoConstructor, this);
    if (hr)
        goto Cleanup;

    // return addref'd result to caller
    pADO->AddRef();
    *ppADO = pADO;
        
    // give it the row position.  This may fire onrowenter, which might ask for
    // event.recordset, so we must set *ppADO first.
    hr = THR(pAdoConstructor->put_RowPosition(pRowPos));
    if (hr)
        goto Cleanup;

Cleanup:
    ReleaseInterface(pADO);
    ReleaseInterface(pAdoConstructor);
    return hr;
}

EXTERN_C const CLSID CLSID_CRowPosition;

//+-------------------------------------------------------------------------
// Function:    Adapt RowPosition On Rowset (static helper)
//
// Synopsis:    point pRowPos at an object that adapts IRowPosition to IRowset.
//              If pRowPos already points at an adapter, don't change it.
//
// Arguments:   ppRowPos    [out] where to store adapter
//              pRowset     [in] pointer to IRowset

HRESULT
CAdaptingProvider::AdaptRowPositionOnRowset(IRowPosition** ppRowPos, IRowset *pRowset)
{
    IRowPosition *pRowPos = NULL;
    HRESULT hr = S_OK;
    Assert("rowset expected" && pRowset);
    
    // if adapter already exists, nothing to do
    if (*ppRowPos)
        goto Cleanup;

    // create an adapter
    hr = CoCreateInstance(CLSID_CRowPosition, NULL,
                          CLSCTX_INPROC, IID_IRowPosition,
                          (void **)&pRowPos);
    if (hr)
    {
#ifndef UNIX
        // Win32 doesn't have a built-in CRowPosition anymore
        goto Cleanup;
#else
        pRowPos = new CRowPosition();
        if (!pRowPos)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
#endif
    }


    hr = pRowPos->Initialize(pRowset);
    if (hr)
    {
        ReleaseInterface(pRowPos);
        goto Cleanup;
    }
        
Cleanup:
    if (!hr && pRowPos)
        *ppRowPos = pRowPos;
    return hr;
}


//+-------------------------------------------------------------------------
// Function:    Adapt Rowset On Cursor (static helper)
//
// Synopsis:    point pRowset at an object that adapts IRowset to ICursor.
//              If pRowset already points at an adapter, don't change it.
//
// Arguments:   ppRowset    [out] where to store adapter
//              pCursor     [in] pointer to ICursor

HRESULT
CAdaptingProvider::AdaptRowsetOnCursor(IRowset** ppRowset, ICursor *pCursor)
{
    HRESULT hr = S_OK;
//    IRowsetFromCursor *pIrfc = 0;
    Assert("cursor expected" && pCursor);

    // if adapter already exists, nothing to do
    if (*ppRowset)
        goto Cleanup;
    
    // BUGBUG Disable Viaduct Phase I.  The Viaduct-I DLL uses OLE-DB M6.1, but
    // we're at M11.  So it would be a bad idea to hook up to it.
#if !defined(GetRidOfThisWhenViaductConvertsToOleDBM11)
    hr = E_NOTIMPL;
#else // Viaduct-I is enabled

    // create an adapter
    hr = CoCreateInstance(CLSID_CRowsetFromCursor, NULL,
                          CLSCTX_INPROC, IID_IRowsetFromCursor,
                          (void **)&pIrfc);
    if (hr) 
        goto Cleanup;
        
    // get its IRowset interface
    hr = THR(pIrfc->GetRowset(pCursor, ppRowset, g_lcidUserDefault));
#endif // disable Viaduct-I

Cleanup:
//     ReleaseInterface(pIrfc);
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
/////                       Provider Classes                            /////
/////////////////////////////////////////////////////////////////////////////

/////-------------------------------------------------------------------/////
///// null provider.  Used when binding fails.  Always returns error.   /////
/////-------------------------------------------------------------------/////

class CNullProvider: public CDataSourceProvider
{
    typedef CDataSourceProvider super;
public:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CNullProvider))
    CNullProvider(CDoc *pDoc): _pADO(0) { _pDoc = pDoc; }
    virtual ~CNullProvider() { ReleaseInterface(_pADO); }
    virtual HRESULT QueryDataInterface(REFIID riid, void **ppv);
    virtual HRESULT GetSubProvider(CDataSourceProvider **ppdsp,
                LPCTSTR bstrName, HROW hrow=0) { *ppdsp = this;  AddRef(); return S_OK; }
private:
    IADORecordset *_pADO;
};


//+-------------------------------------------------------------------------
// Member:      Query Data Interface (CNullProvider, public)
//
// Synopsis:    return addref'd interface pointer.  NullProvider supports
//              QDI for ADO, and returns an empty ADO;  this allows scripts
//              to limp along even when there's no real provider.
//              All other requests return E_NOINTERFACE.
//
//              ** The behavior described above also deprives script authors **
//              ** of a reasonable why of telling whether or not they have   **
//              ** a valid recordset, since foo.recordset == null will return**
//              ** false.  I'm going to try deprecating this and see what    **
//              ** happens -- cfranks 2 August 1997                          **
//
// Arguments:   riid        desired interface
//              ppvUnk      where to store interface pointer
//
// Returns:     HRESULT

HRESULT
CNullProvider::QueryDataInterface(REFIID riid, void **ppvUnk)
{
    HRESULT hr = E_NOINTERFACE;     // assume the worst

    if (IsEqualIID(riid, IID_IADORecordset15))
    {
        if (!_pADO)
        {
            IADORecordset *pADO;
            IADORecordsetConstruction *pADOC;

            hr = GetADOConstructor(&pADO, &pADOC, NULL);
            if (hr)
                goto Cleanup;

            _pADO = pADO;
            ReleaseInterface(pADOC);
        }

        if (_pADO)
        {
            _pADO->AddRef();
            *ppvUnk = _pADO;
            hr = S_OK;
        }
    }
Cleanup:

    return hr;
}


//+-------------------------------------------------------------------------
// Member:      Detach (CDataSourceProvider, public)
//
// Synopsis:    Detach my resources

void
CAdaptingProvider::Detach()
{
    super::Detach();
}


//+-------------------------------------------------------------------------
// Member:      SetRowChangedEventTarget (CAdaptingProvider, protected)
//
// Synopsis:    wire up a listener to my IRowPosition, so I can fire
//              onRowExit/Enter to the given OLE site, and wire up
//              a listener to my IDBAsynchNotify, so we can get transfer
//              notifications.

HRESULT
CAdaptingProvider::SetRowChangedEventTarget(CDataMemberMgr *pdmm)
{
    HRESULT hr = S_OK;
    Assert(pdmm);

    // cache the SimpleDataConverter, if any
    _pSDC = pdmm->GetSDC();
    if (_pSDC)
        _pSDC->AddRef();

    // Try to make sure we have both a row position & a rowset, but ultimately
    // proceed if we have either one.  (Note that Ensuring a RowsetPosition will
    // ensure a Rowset, but not visa versa so the order here is important.)
    if (EnsureRowsetPosition() || EnsureRowset())
    {
        _pRowEvents = new CDataSetEvents;
        if (_pRowEvents)
        {
            hr = _pRowEvents->Init(pdmm, _pRowset, _pRowPos, this);
        }
    }
    return hr;
}

//+-------------------------------------------------------------------------
// Member:      ReleaseAll (CAdaptingProvider, protected)
//
// Synopsis:    release my resources

void
CAdaptingProvider::ReleaseAll()
{
    int i;
    CXTRecord       *pXR;
    CChapterRecord  *pCR;
    CProviderRecord *pPR;
    CAccessorRecord *pAR;
    CDataMemberMgr  *pdmm = NULL;
    
    if (_pRowEvents)
    {
        pdmm = _pRowEvents->GetDataMemberManager();
        if (pdmm)
        {
            pdmm->AddRef();       // keep control alive through shutdown
        }
        _pRowEvents->Detach();
    }
    ClearInterface((IUnknown**)&_pRowEvents);

    // release the XferThunk cache
    for (i=_aryXT.Size(), pXR=_aryXT;  i>0;  --i, ++pXR)
    {
        // release field name
        FormsFreeString(pXR->bstrField);

        // release XT
        if (pXR->pXT)
            pXR->pXT->Release();
    }
    _aryXT.DeleteAll();

    // release current record instance
    if (_pCRI)
        _pCRI->Detach();
    ClearInterface(&_pCRI);

    // release chapter table
    for (i=_aryCR.Size(), pCR=_aryCR;  i>0;  --i, ++pCR)
    {
        // release hrow
        Assert(_pRowset);
        IGNORE_HR(_pRowset->ReleaseRows(1, &pCR->hrow, NULL, NULL, NULL));

        // release field name
        FormsFreeString(pCR->bstrField);
    }
    _aryCR.DeleteAll();
    
    // release provider table
    for (i=_aryPR.Size(), pPR=_aryPR;  i>0;  --i, ++pPR)
    {
        FormsFreeString(pPR->bstrField);
        if (pPR->pdsp)
            pPR->pdsp->Release();
    }
    _aryPR.DeleteAll();

    // release accessor/rowset table
    if (_aryAR.Size() > 0)
    {
        IAccessor *pAccessor = NULL;

        Assert(_pRowset && "hierarchy without a rowset?");
        
        _pRowset->QueryInterface(IID_IAccessor, reinterpret_cast<void**>(&pAccessor));
            
        for (i=_aryAR.Size(), pAR=_aryAR;  i>0;  --i, ++pAR)
        {
            FormsFreeString(pAR->bstrField);
            if (pAccessor && pAR->hAccessor)
                pAccessor->ReleaseAccessor(pAR->hAccessor, NULL);

            ReleaseInterface(pAR->pRowset);
        }
        _aryAR.DeleteAll();

        ReleaseInterface(pAccessor);
    }

    if (_hChapter != DB_NULL_HCHAPTER)
    {
        Assert(_pRowset && "hierarchy without a rowset?");
        IChapteredRowset *pChapRowset = NULL;
        HRESULT hr = _pRowset->QueryInterface(IID_IChapteredRowset,
                        reinterpret_cast<void**>(&pChapRowset));

        if (!hr)
        {
            pChapRowset->ReleaseChapter(_hChapter, NULL);
        }

        _hChapter = DB_NULL_HCHAPTER;
        ReleaseInterface(pChapRowset);
    }

    // Bug 70754 illustrates that we cannot change ADO's client site.  We may
    // have handed the ADO recordset out to script or an application, and
    // it should continue to function with security.
    // if (_pADO)
    //     SetADOClientSite(_pADO, NULL);
    
    ClearInterface(&_pSDC);
    ClearInterface(&_pCursor);
    ClearInterface(&_pRowset);
    ClearInterface(&_pRowPos);
    ClearInterface(&_pSTD);
    ClearInterface(&_pADO);
    if (pdmm)
        pdmm->Release();
}


//+-------------------------------------------------------------------------
// Member:      Ensure STD (private)
//
// Synopsis:    Try to acquire a valid ISTD interface
//
// Arguments:   none
//
// Returns:     true    _pSTD is valid
//              false   _pSTD is invalid

BOOL
CAdaptingProvider::EnsureSTD()
{
    return !!_pSTD;
}


//+-------------------------------------------------------------------------
// Member:      Ensure Rowset (private)
//
// Synopsis:    Try to acquire a valid IRowset interface
//
// Arguments:   none
//
// Returns:     true    _pRowset is valid
//              false   _pRowset is invalid

BOOL
CAdaptingProvider::EnsureRowset()
{
    if (_pRowPos)
        AdaptRowsetOnRowPosition(&_pRowset, _pRowPos);
    else if (_pSTD)
        AdaptRowsetOnSTD(&_pRowset, _pSTD);
    else if (_pCursor)
        AdaptRowsetOnCursor(&_pRowset, _pCursor);
    return !!_pRowset;
}


//+-------------------------------------------------------------------------
// Member:      Ensure Rowset Position (private)
//
// Synopsis:    Try to acquire a valid IRowPosition interface
//
// Arguments:   none
//
// Returns:     true    _pRowPos is valid
//              false   _pRowPos is invalid

BOOL
CAdaptingProvider::EnsureRowsetPosition()
{
    if (EnsureRowset())
        AdaptRowPositionOnRowset(&_pRowPos, _pRowset);
    return !!_pRowPos;
}


//+-------------------------------------------------------------------------
// Member:      Ensure Cursor (private)
//
// Synopsis:    Try to acquire a valid ICursor interface
//
// Arguments:   none
//
// Returns:     true    _pCursor is valid
//              false   _pCursor is invalid

BOOL
CAdaptingProvider::EnsureCursor()
{
    if (!_pCursor)
    {
#ifdef VD_INCLUDE_ROWPOSITION
        if (EnsureRowsetPosition())
            AdaptCursorOnRowPosition(&_pCursor, _pRowPos);
        else
#endif
        if (EnsureRowset())
            AdaptCursorOnRowset(&_pCursor, _pRowset);
    }
    return !!_pCursor;
}


//+-------------------------------------------------------------------------
// Member:      Ensure ADO (private)
//
// Synopsis:    Try to acquire a valid IADORecordset interface
//
// Arguments:   none
//
// Returns:     true    _pADO is valid
//              false   _pADO is invalid

BOOL
CAdaptingProvider::EnsureADO()
{
    if (_pADO)
        goto Cleanup;
    
    if (EnsureRowsetPosition())
    {
        if (_pCRI)
            _pCRI->InitPosition();      // in case it hasn't been done yet
        AdaptADOOnRowPos(&_pADO, _pRowPos);
    }
    else
    if (EnsureRowset())
        AdaptADOOnRowset(&_pADO, _pRowset);

Cleanup:
    return !!_pADO;
}


//+-------------------------------------------------------------------------
// Member:      SetADOClientSite (public)
//
// Synopsis:    Tell ADO to use the given client site
//
// Arguments:   pADO            ADO object
//              pClientSite     client site
//
// Returns:     HRESULT

HRESULT
CAdaptingProvider::SetADOClientSite(IADORecordset *pADO, IOleClientSite *pClientSite)
{
    HRESULT hr;
    IObjectWithSite *pObjSite = NULL;

    hr = pADO->QueryInterface(IID_IObjectWithSite, (void **)&pObjSite);
    if (OK(hr) && pObjSite)
    {
        hr = pObjSite->SetSite(pClientSite);
        ReleaseInterface(pObjSite);
        goto Cleanup;
    }
    hr = S_OK;
    
Cleanup:
    RRETURN(hr);
}


//+-------------------------------------------------------------------------
// Member:      Ensure CRI (private)
//
// Synopsis:    Try to acquire a valid ICurrentRecordInstance interface
//
// Arguments:   none
//
// Returns:     true    _pCRI is valid
//              false   _pCRI is invalid

BOOL
CAdaptingProvider::EnsureCRI()
{
    HRESULT hr = S_OK;
    
    if (_pCRI)                      // already exists, nothing to do
        goto Cleanup;

    EnsureRowsetPosition();         // need a valid RowPosition

    // get a CRI object
    _pCRI = new CCurrentRecordInstance;
    if (!_pCRI)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    // tell it to track the RowPosition
    hr = _pCRI->Init(this, _pRowPos);
    if (!hr)
        hr = _pCRI->InitChapter(_hChapter);
    if (hr)
    {
        ClearInterface(&_pCRI);
        goto Cleanup;
    }

Cleanup:
    return !!_pCRI;
}


//+-------------------------------------------------------------------------
// Member:      Query Data Interface (CAdaptingProvider, public)
//
// Synopsis:    return addref'd interface pointer
//
// Arguments:   riid        desired interface
//              ppvUnk      where to store interface pointer
//
// Returns:     HRESULT

#define TEST(intf, EnsureFn, _pIntf) \
    if (IsEqualIID(riid, intf))     \
    {                               \
        if (EnsureFn())             \
        {                           \
            _pIntf->AddRef();       \
            *ppvUnk = _pIntf;       \
            hr = S_OK;              \
        }                           \
    }

HRESULT
CAdaptingProvider::QueryDataInterface(REFIID riid, void **ppvUnk)
{
    HRESULT hr = E_NOINTERFACE;     // assume the worst

            TEST(IID_IRowset,               EnsureRowset,           _pRowset)
    else    TEST(IID_ICurrentRecordInstance,EnsureCRI,              _pCRI)
    else    TEST(IID_IADORecordset15,       EnsureADO,              _pADO)
    else    TEST(IID_IRowPosition,          EnsureRowsetPosition,   _pRowPos)
    else    TEST(IID_ICursor,               EnsureCursor,           _pCursor)
    else    TEST(IID_OLEDBSimpleProvider,   EnsureSTD,              _pSTD)

    return hr;
}

#undef TEST


//+-------------------------------------------------------------------------
// Member:      Stop
//
// Synopsis:    Stops any asynchronous download that may be in progress
//
// Arguments:   
//
// Returns:     HRESULT

HRESULT
CAdaptingProvider::Stop()
{
    HRESULT hr=S_OK;
    IDBAsynchStatus * pRowsetAsynch;

    if (_pRowset)
    {
        hr = _pRowset->QueryInterface(IID_IDBAsynchStatus, (void **)&pRowsetAsynch);
        if (SUCCEEDED(hr) && pRowsetAsynch)
        {
            hr = pRowsetAsynch->Abort(NULL, DBASYNCHOP_OPEN);
            pRowsetAsynch->Release();
        }
    }

    return hr;
}

//+-------------------------------------------------------------------------
// Member:      ReplaceProvider (CAdaptingProvider, public)
//
// Synopsis:    tear down current-record bindings before going away
//
// Arguments:   pdspNewProvider        new provider
//
// Returns:     HRESULT


HRESULT
CAdaptingProvider::ReplaceProvider(CDataSourceProvider *pdspNewProvider)
{
    HRESULT hr;
    int i;
    CProviderRecord *pPR;
    
    // notify everyone attached to me that I'm changing
    hr = super::ReplaceProvider(pdspNewProvider);

    // notify all my child providers that their clients should rebind
    for (i=_aryPR.Size(), pPR=_aryPR;  i > 0;  --i, ++pPR)
    {
        if (pPR->pdsp)
        {
            HRESULT hrT;

            hrT = pPR->pdsp->ReplaceProvider(NULL);
            if (!hr)
                hr = hrT;               // report first error
        }
    }
    
    return hr;
}


//+------------------------------------------------------------------------
//
//  Member:     GetXT
//
//  Synopsis:   Return the CXferThunk used to get/set data from the given
//              field using the given type.  Create one, if necessary, and
//              cache it in _aryXT.
//
//  Returns:    XT

CXferThunk *
CAdaptingProvider::GetXT(LPCTSTR strColumn, CVarType vtDest)
{
    CXferThunk *pXT = NULL;
    int i;
    CXTRecord *pXR;

    // look up <field, type> in the table
    for (i=_aryXT.Size(), pXR=_aryXT;  i>0;  --i, ++pXR)
    {
        if (0 == FormsStringCmp(strColumn, pXR->bstrField) &&
            vtDest.IsEqual(pXR->vtDest))
        {
            break;
        }
    }

    // if it's there, just return the XT
    if (i != 0)
    {
        pXT = pXR->pXT;
    }

    // if it's not, create and cache a new XT
    else
    {
        ISimpleDataConverter *pSDC;

        pSDC = vtDest.FLocalized() ? Doc()->GetSimpleDataConverter() : _pSDC;

        // create a cache entry
        pXR = _aryXT.Append();
        if (pXR == NULL)
            goto Cleanup;

        // create an XT
        CXferThunk::Create(GetDLCursor(), strColumn, vtDest, pSDC, &pXT);

        // fill in the cache entry
        FormsAllocString(strColumn, &pXR->bstrField);
        pXR->vtDest = vtDest;
        pXR->pXT = pXT;
    }

Cleanup:
    return pXT;
}



//+-------------------------------------------------------------------------
// Member:      GetSubProvider (CDataSourceProvider, public)
//
// Synopsis:    return a sub-provider of a hierarchical provider
//
// Arguments:   ppdsp       where to store the answer
//              bstrName    path through hierarchy (e.g. "orders.details")
//              hrow        hrow of interest
//
// Description: This function is called in two circumstances.
//              (1) during nested table expansion, inner tables need a
//              sub-provider corresponding to the HROW of the enclosing table
//              row.  In this case hrow is non-null, and bstrName is a single
//              component denoting the field that holds the chapter handle.
//              (2) script and current-record bound elements need the sub-
//              provider obtained by walking down the tree following the
//              current row position of each provider.  In this case, hrow is
//              null, and bstrName can have many components separated by dots.
//
// Returns:     HRESULT

HRESULT
CAdaptingProvider::GetSubProvider(CDataSourceProvider **ppdsp,
                                    LPCTSTR bstrName, HROW hrow)
{
    HRESULT hr = S_OK;
    BSTR bstrHead = NULL;
    BSTR bstrTail = NULL;
    HCHAPTER hchapter = DB_NULL_HCHAPTER;
    HROW hrowCurr = DB_NULL_HROW;

    // bookkeeping to get started
    Assert(ppdsp);
    *ppdsp = NULL;
    FormsSplitFirstComponent(bstrName, &bstrHead, &bstrTail);

    // if the name is empty, return myself
    if (FormsStringLen(bstrHead) == 0)
    {
        AddRef();
        *ppdsp = this;
    }

    // if an HROW is given, use sub-provider corresponding to chapter (case 1)
    else if (hrow != DB_NULL_HROW)
    {
        hr = GetProviderFromField(bstrHead, hrow, ppdsp);
    }

    // otherwise, follow current position down the tree
    else if (EnsureRowsetPosition())
    {
        DBPOSITIONFLAGS dwFlags;
        CDataSourceProvider *pSubProvider = NULL;

        Assert(_pCRI);
        _pCRI->InitPosition();      // make sure _pRowPos has valid position
        
        // get the current hrow
        hr = _pRowPos->GetRowPosition(&hchapter, &hrowCurr, &dwFlags);
        if (!hr && hrowCurr == DB_NULL_HROW)
            hr = DB_E_BADROWHANDLE;

        // get next level sub-provider
        if (!hr)
            hr = GetProviderFromField(bstrHead, hrowCurr, &pSubProvider);

        // if something failed, return a null provider (so script gets
        // a zombie ADO recordset)
        if (hr)
        {
            *ppdsp = new CNullProvider(Doc());
            hr = (*ppdsp) ? S_OK : E_OUTOFMEMORY;
            goto Cleanup;
        }

        // continue search at next level
        Assert (pSubProvider);

        hr = pSubProvider->GetSubProvider(ppdsp, bstrTail);
        pSubProvider->Release();
    }

    // without a row position object, we're helpless
    else
    {
        hr = E_FAIL;
    }

Cleanup:
    ReleaseChapterAndRow(hchapter, hrowCurr, _pRowPos);
    FormsFreeString(bstrHead);
    FormsFreeString(bstrTail);
    RRETURN(hr);
}


//+-------------------------------------------------------------------------
// Member:      GetChapterFromField (CAdaptingProvider, private)
//
// Synopsis:    find the chapter handle in the given hrow's field.
//
// Flame:       You might think this could be done by doing GetData
//              on the given HROW and field.  But it seems that OLE-DB
//              allows providers to generate chapter handles dynamically,
//              returning a new handle each time you call GetData.  This
//              isn't specifically mentioned in the OLE-DB spce (v2.0), but
//              I called the OLE-DB spec guru (12-May-98) and he attested
//              that this behavior is permitted.
//              Furthermore, at least one provider (the Shape Provider)
//              works this way.  So we have to fetch the handle once and
//              cache it.
//
// Arguments:   hrow        HROW holding desired chapter value
//              bstrField   name of chapter-valued field
//              phChapter   where to return the answer
//
// Returns:     HRESULT


HRESULT
CAdaptingProvider::GetChapterFromField(HROW hrow, BSTR bstrField,
                                        HCHAPTER *phChapter)
{
    HRESULT hr = S_OK;
    int i;
    CChapterRecord *pCR;
    
    // look up <hrow, field> in chapter table
    for (i=_aryCR.Size(), pCR=_aryCR; i>0;  --i, ++pCR)
    {
        if (IsSameRow(pCR->hrow, hrow) &&
            FormsStringCmp(pCR->bstrField, bstrField) == 0)
        {
            break;
        }
    }

    // if it's not there, fetch the handle and cache it in the table
    if (i == 0)
    {
        HACCESSOR hAccessor;
        HCHAPTER hChapter;

        // fetch the chapter handle from the rowset
        hr = GetAccessorForField(bstrField, &hAccessor);
        if (hr)
            goto Cleanup;

        hr = _pRowset->GetData(hrow, hAccessor, &hChapter);
        if (hr)
            goto Cleanup;

        // add an entry to the cache
        pCR = _aryCR.Append();
        if (pCR)
        {
            _pRowset->AddRefRows(1, &hrow, NULL, NULL);
            pCR->hrow = hrow;
            FormsAllocString(bstrField, &pCR->bstrField);
            pCR->hChapter = hChapter;
        }
        else
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
    }

    // return the desired chapter handle
    *phChapter = pCR->hChapter;

Cleanup:
    RRETURN(hr);
}


//+-------------------------------------------------------------------------
// Member:      GetProviderFromField (CAdaptingProvider, private)
//
// Synopsis:    find the provider associated with the chapter value in the
//              given hrow's field.
//
// Arguments:   bstrField   name of chapter-valued field
//              hrow        HROW holding desired chapter value
//              ppdsp       where to return the answer
//
// Returns:     HRESULT


HRESULT
CAdaptingProvider::GetProviderFromField(BSTR bstrField, HROW hrow,
                                        CDataSourceProvider **ppdsp)
{
    HRESULT hr;
    HCHAPTER hChapter;
    int i;
    CProviderRecord *pPR;

    Assert(_pRowset);
    *ppdsp = NULL;

    hr = GetChapterFromField(hrow, bstrField, &hChapter);
    if (hr)
        goto Cleanup;

    // look up <field, chapter> in my cache
    for (i=_aryPR.Size(), pPR=_aryPR;  i > 0;  --i, ++pPR)
    {
        if (pPR->hChapter == hChapter &&
            FormsStringCmp(pPR->bstrField, bstrField) == 0)
        {
            break;
        }
    }

    // if it wasn't there, create the desired provider and add it to the cache
    if (i == 0)
    {
        pPR = _aryPR.Append();
        if (!pPR)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        hr = InitializeProviderRecord(pPR, hChapter, bstrField);
        if (hr)
            goto Cleanup;
    }

    // return the desired result
    *ppdsp = pPR->pdsp;
    if (*ppdsp)
        (*ppdsp)->AddRef();

Cleanup:
    RRETURN(hr);
}


//+-------------------------------------------------------------------------
// Member:      GetAccessorAndRowsetForField (CAdaptingProvider, private)
//
// Synopsis:    find the accessor for the chapter-valued column
//
// Arguments:   bstrField   name of chapter-valued field
//              phAccessor  where to store accessor
//              ppRowset    where to store field's referenced rowset
//
// Returns:     HRESULT


HRESULT
CAdaptingProvider::GetAccessorAndRowsetForField(BSTR bstrField,
                                        HACCESSOR *phAccessor, IRowset **ppRowset)
{
    HRESULT hr = S_OK;
    int i;
    CAccessorRecord *pAR;

    // prepare for failure
    if (phAccessor)
        *phAccessor = NULL;
    if (ppRowset)
        *ppRowset = NULL;

    // look up the field in the cache
    for (i=_aryAR.Size(), pAR=_aryAR;  i>0;  --i, ++pAR)
    {
        if (FormsStringCmp(bstrField, pAR->bstrField) == 0)
            break;
    }

    // if not in the cache, add it
    if (i == 0)
    {
        pAR = _aryAR.Append();
        if (!pAR)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        hr = InitializeAccessorRecord(pAR, bstrField);
        if (hr)
            goto Cleanup;
    }

    // return the desired results
    if (phAccessor)
    {
        if (EnsureAccessorInAccessorRecord(pAR))
        {
            *phAccessor = pAR->hAccessor;
        }
        else
        {
            hr = E_FAIL;
        }
    }
    
    if (ppRowset)
    {
        if (EnsureRowsetInAccessorRecord(pAR))
        {
            *ppRowset = pAR->pRowset;
        }
        else
        {
            hr = E_FAIL;
        }
    }
    
Cleanup:
    RRETURN(hr);
}


//+-------------------------------------------------------------------------
// Member:      InitializeProviderRecord (CAdaptingProvider, private)
//
// Synopsis:    initialize an entry in the _aryPR array that maps
//              <fieldname, chapter> to <provider>.
//
// Arguments:   pPR         pointer to entry that needs initializing
//              hChapter    chapter handle
//              bstrField   name of chapter-valued field
//
// Returns:     HRESULT


HRESULT
CAdaptingProvider::InitializeProviderRecord(CProviderRecord *pPR,
                        HCHAPTER hChapter, BSTR bstrField)
{
    HRESULT hr;
    IRowsetInfo *pRowsetInfo = NULL;
    IRowset *pChildRowset = NULL;

    // fill in entries with null (in case of error)
    FormsAllocString(bstrField, &pPR->bstrField);
    pPR->hChapter = hChapter;
    pPR->pdsp = NULL;

    // get the rowset to which the chapter applies
    hr = GetRowsetForField(bstrField, &pChildRowset);
    if (hr)
        goto Cleanup;

    // create a provider and stick it into the table
    hr = CreateSubProvider(this, pChildRowset, NULL, _pRowEvents->GetDataMemberManager(),
                            hChapter, DB_NULL_HROW, &pPR->pdsp);
    if (hr)
        goto Cleanup;

Cleanup:
    ReleaseInterface(pRowsetInfo);

    RRETURN(hr);
}


//+-------------------------------------------------------------------------
// Member:      InitializeAccessorRecord (CAdaptingProvider, private)
//
// Synopsis:    initialize an entry in the _aryAR array that maps
//              <fieldname> to <accessor, ordinal>.
//
// Arguments:   pAR         pointer to entry that needs initializing
//              bstrField   name of chapter-valued field
//
// Returns:     HRESULT


HRESULT
CAdaptingProvider::InitializeAccessorRecord(CAccessorRecord *pAR, BSTR bstrField)
{
    HRESULT hr;
    IColumnsInfo *pColumnsInfo = NULL;
    DBORDINAL cColumns;
    DBCOLUMNINFO *aColumnInfo = NULL;
    OLECHAR *pStringsBuffer = NULL;
    ULONG i;

    FormsAllocString(bstrField, &pAR->bstrField);
    pAR->hAccessor = 0;
    pAR->pRowset = NULL;
    pAR->ulOrdinal = DB_INVALIDCOLUMN;

    // get the column info
    hr = _pRowset->QueryInterface(IID_IColumnsInfo, reinterpret_cast<void**>(&pColumnsInfo));
    if (hr)
        goto Cleanup;

    hr = pColumnsInfo->GetColumnInfo(&cColumns, &aColumnInfo, &pStringsBuffer);
    if (hr)
        goto Cleanup;

    // look up the desired column, make sure it's chapter-valued
    hr = E_FAIL;        // in case search fails
    for (i=0; i<cColumns; ++i)
    {
        if (FormsStringICmp(bstrField, aColumnInfo[i].pwszName) == 0)
            break;
    }
    if (i == cColumns)
        goto Cleanup;
    if ( !(aColumnInfo[i].dwFlags & DBCOLUMNFLAGS_ISCHAPTER) )
        goto Cleanup;

    pAR->ulOrdinal = i;
    hr = S_OK;

Cleanup:
    CoTaskMemFree(aColumnInfo);
    CoTaskMemFree(pStringsBuffer);
    ReleaseInterface(pColumnsInfo);

    RRETURN(hr);
}


//+-------------------------------------------------------------------------
// Member:      EnsureAccessorInAccessorRecord (CAdaptingProvider, private)
//
// Synopsis:    make sure the accessor field is filled in
//
// Arguments:   pAR         the accessor record of interest
//
// Returns:     TRUE        if accessor field has a valid accessor

BOOL
CAdaptingProvider::EnsureAccessorInAccessorRecord(CAccessorRecord *pAR)
{
    HRESULT hr = S_OK;
    IAccessor *pAccessor = NULL;
    DBBINDING dbBinding;
    DBBINDSTATUS dbBindStatus;

    if (pAR->hAccessor != 0 || pAR->ulOrdinal == DB_INVALIDCOLUMN)
        goto Cleanup;
    
    hr = _pRowset->QueryInterface(IID_IAccessor, reinterpret_cast<void**>(&pAccessor));
    if (hr)
        goto Cleanup;

    dbBinding.iOrdinal = pAR->ulOrdinal;
    dbBinding.obValue = 0;
    dbBinding.dwPart = DBPART_VALUE;
    dbBinding.wType = DBTYPE_HCHAPTER;

    dbBinding.pTypeInfo = 0;
    dbBinding.pBindExt = 0;
    dbBinding.dwMemOwner = DBMEMOWNER_CLIENTOWNED;
    dbBinding.dwFlags = 0;
    
    hr = pAccessor->CreateAccessor(DBACCESSOR_ROWDATA, 1, &dbBinding, 0,
                                    &pAR->hAccessor, &dbBindStatus);
    if (hr)
        goto Cleanup;

Cleanup:
    ReleaseInterface(pAccessor);
    return (pAR->hAccessor != 0);
}


//+-------------------------------------------------------------------------
// Member:      EnsureRowsetInAccessorRecord (CAdaptingProvider, private)
//
// Synopsis:    make sure the rowset field is filled in
//
// Arguments:   pAR         the accessor record of interest
//
// Returns:     TRUE        if rowset field has a valid rowset

BOOL
CAdaptingProvider::EnsureRowsetInAccessorRecord(CAccessorRecord *pAR)
{
    HRESULT hr = S_OK;
    IRowsetInfo *pRowsetInfo = NULL;

    if (pAR->pRowset != NULL || pAR->ulOrdinal == DB_INVALIDCOLUMN)
        goto Cleanup;
    
    hr = _pRowset->QueryInterface(IID_IRowsetInfo, reinterpret_cast<void**>(&pRowsetInfo));
    if (hr)
        goto Cleanup;

    hr = pRowsetInfo->GetReferencedRowset(pAR->ulOrdinal, IID_IRowset,
                                    reinterpret_cast<IUnknown**>(&pAR->pRowset));
    if (hr)
        goto Cleanup;

Cleanup:
    ReleaseInterface(pRowsetInfo);
    return (pAR->pRowset != NULL);
}


//+-------------------------------------------------------------------------
// Member:      Create (CAdaptingProvider, static)
//
// Synopsis:    create a sub-provider for a child rowset (for hierarchy)
//
// Arguments:   pChildRowset    child rowset on which to base provider
//              hChapter        chapter to restrict provider
//              ppdsp           where to store the result
//
// Returns:     HRESULT

HRESULT
CAdaptingProvider::CreateSubProvider(CAdaptingProvider *pProviderParent,
                                    IRowset *pChildRowset,
                                    IRowPosition *pChildRowPos,
                                    CDataMemberMgr *pdmm,
                                    HCHAPTER hChapter, HROW hrow,
                                    CDataSourceProvider **ppdsp)
{
    HRESULT hr = S_OK;
    CAdaptingProvider *pProvider;

    if (pChildRowPos)
    {
        pProvider = new CAdaptingProvider(pChildRowPos, hrow);
    }
    else
    {
        Assert(pChildRowset);
        pProvider = new CAdaptingProvider(pChildRowset);
    }
    
    if (pProvider)
    {
        *ppdsp = pProvider;
        pProvider->Init(pdmm, hChapter, pProviderParent);
    }
    else
        hr = E_OUTOFMEMORY;

    RRETURN(hr);
}


//+-------------------------------------------------------------------------
// Member:      RemoveSubProvider (CAdaptingProvider, private)
//
// Synopsis:    remove a sub-provider from my tables
//
// Arguments:   pdsp            sub-provider to remove
//
// Returns:     nothing

void
CAdaptingProvider::RemoveSubProvider(CDataSourceProvider *pdsp)
{
    int i;
    CProviderRecord *pPR;
    Assert(pdsp);

    // look up <field, chapter> in my cache
    for (i=_aryPR.Size()-1, pPR=_aryPR+i;  i >= 0;  --i, --pPR)
    {
        if (pPR->pdsp == pdsp)
        {
            FormsFreeString(pPR->bstrField);
            pPR->pdsp->Release();
            _aryPR.Delete(i);
        }
    }
}


//+-------------------------------------------------------------------------
// Member:      UpdateProviderRecords (CAdaptingProvider, private)
//
// Synopsis:    Replace old <chapter, provider> by new one in my tables
//
// Returns:     HRESULT

HRESULT
CAdaptingProvider::UpdateProviderRecords(HCHAPTER hChapterOld, CDataSourceProvider *pdspOld,
                                HCHAPTER hChapterNew, CDataSourceProvider *pdspNew)
{
    int i, j;
    CProviderRecord *pPR;
    CChapterRecord *pCR;

    // find record for old provider
    for (i=_aryPR.Size()-1, pPR=_aryPR+i;  i >= 0;  --i, --pPR)
    {
        if (pPR->pdsp == pdspOld)
        {
            // it should correspond to the old chapter
            Assert(pPR->hChapter == hChapterOld);

            // replace it with the new chapter and provider
            pPR->hChapter = hChapterNew;
            pdspNew->AddRef();
            pdspOld->Release();
            pPR->pdsp = pdspNew;

            // update the chapter table
            for (j=_aryCR.Size()-1, pCR=_aryCR+j;  j>=0;  --j, --pCR)
            {
                if (pCR->hChapter == hChapterOld &&
                    FormsStringCmp(pPR->bstrField, pCR->bstrField) == 0)
                {
                    IRowset *pChildRowset = NULL;
                    if (S_OK == GetRowsetForField(pPR->bstrField, &pChildRowset)
                        && pChildRowset)
                    {
                        IChapteredRowset *pChapRowset = NULL;

                        if (S_OK == pChildRowset->QueryInterface(IID_IChapteredRowset,
                                                                 (void**)&pChapRowset)
                                && pChapRowset)
                        {
                            IGNORE_HR(pChapRowset->AddRefChapter(hChapterNew, NULL));
                            IGNORE_HR(pChapRowset->ReleaseChapter(pCR->hChapter, NULL));
                        }

                        ReleaseInterface(pChapRowset);
                    }
                    
                    pCR->hChapter = hChapterNew;
                }
            }
        }
    }

    RRETURN(S_OK);
}


//+-------------------------------------------------------------------------
// Member:      ChangeChapter (CAdaptingProvider, private)
//
// Synopsis:    change the chapter I get my data from
//
// Returns:     HRESULT

HRESULT
CAdaptingProvider::ChangeChapter(HCHAPTER hChapterNew, HROW hrowNew)
{
    HRESULT hr = S_OK;
    CDataSourceProvider *pProviderNew = NULL;

    AddRef();               // stabilize during this routine
    
    if (hChapterNew == _hChapter)
        goto Cleanup;
    
    if (_pProviderParent)   // Child provider.
    {
        // create a new provider tied to the new chapter
        hr = CreateSubProvider(_pProviderParent, NULL, _pRowPos,
                                _pRowEvents->GetDataMemberManager(),
                                hChapterNew, hrowNew, &pProviderNew);
        
        // tell my parent to update her records
        if (!hr)
            hr = _pProviderParent->UpdateProviderRecords(_hChapter, this,
                            hChapterNew, pProviderNew);

        // tell my clients to use the new provider
        if (!hr)
            hr = ReplaceProvider(pProviderNew);
    }

    else                    // Top-level provider
    {
        // change my chapter
        IChapteredRowset *pChapRowset = NULL;

        if (S_OK == _pRowset->QueryInterface(IID_IChapteredRowset,
                                             (void**)&pChapRowset)
                     && pChapRowset)
        {
            IGNORE_HR(pChapRowset->AddRefChapter(hChapterNew, NULL));
            IGNORE_HR(pChapRowset->ReleaseChapter(_hChapter, NULL));
            ReleaseInterface(pChapRowset);
        }

        // tell my clients to rebind
        hr = ReplaceProvider(this);
        
        _hChapter = hChapterNew;

        _fNeedRowEnter = (hrowNew != DB_NULL_HROW);
        _fRowEnterOK = FALSE;
        
        IGNORE_HR(_pCRI->InitChapter(_hChapter));
        Doc()->GetDataBindTask()->InitCurrentRecord(_pCRI);
    }
    
Cleanup:
    if (pProviderNew)
    {
        pProviderNew->Release();
    }
    Release();      // don't use (this) any more
    RRETURN(hr);
}


//+----------------------------------------------------------------------------
//
//  Member:     FireDelayedRowEnter
//
//  Synopsis:   Fire onrowenter now, if there was reason to fire it during
//              startup and we haven't fired it yet.
//
//  Returns:    HRESULT

HRESULT
CAdaptingProvider::FireDelayedRowEnter()
{
    if (_fNeedRowEnter && _pRowEvents)
    {
        TraceTag((tagRowEvents, "%p -> onRowEnter", _pRowEvents->GetOwner()));
        FireDataEvent(_T("rowenter"),
                            DISPID_EVMETH_ONROWENTER,
                            DISPID_EVPROP_ONROWENTER);

    }
    
    _fNeedRowEnter = FALSE;
    _fRowEnterOK = TRUE;
    return S_OK;
}


//+----------------------------------------------------------------------------
//
//  Member:     FireDataEvent
//
//  Synopsis:   Fire one of the events we synthesize on data source objects:
//              datasetchanged, dataavail, datasetcomplete, rowenter, rowexit,
//              rowsinserted, rowsdelete, cellchange
//
//  Returns:    HRESULT

HRESULT
CAdaptingProvider::FireDataEvent(LPCTSTR pchType,
                                DISPID dispidEvent, DISPID dispidProp,
                                BOOL *pfCancelled /* NULL */,
                                long lReason /* 0 */)
{
    HRESULT         hr = S_OK;

    // if there's no _pRowEvents, there's no use trying
    if (_pRowEvents)
    {
        CDataMemberMgr * pdmm = _pRowEvents->GetDataMemberManager();
        CTreeNode *     pNodeContext = pdmm->GetOwner()->GetFirstBranch();
        CDoc *          pDoc = pdmm->Doc();
        EVENTPARAM      param(pDoc, TRUE);
        CDoc::CLock     Lock(pDoc);
        CVariant        vb, *pVariant=NULL;
        CDoc *          pDocOSP;
    
        if (pfCancelled)
            pVariant = &vb;

        // set event parameters
        param.SetNodeAndCalcCoordinates(pNodeContext);
        param.SetType(pchType);
        param.SetQualifier(_cstrDataMember);
        param._lReason      = lReason;
        param.pProvider     = this;

        AddRef();       // stabilize during event

        // if we're firing onrowexit, make sure we've enabled firing
        // onrowenter as well
        if (dispidEvent == DISPID_EVMETH_ONROWEXIT && !_fRowEnterOK)
        {
            FireDelayedRowEnter();
        }
        
        // fire the event
        if (S_OK == pdmm->GetTridentAsOSP(&pDocOSP))
        {   // Trident-as-OSP (note, pDocOSP is *not* refcounted)
            hr = THR(pDocOSP->FireEventHelper(
                            dispidEvent,
                            dispidProp,
                            (BYTE*) VTS_NONE));
        }
        else
        {   // normal DSO
            hr =  THR(pdmm->GetOwner()->BubbleEventHelper(
                        pNodeContext,
                        0,
                        dispidEvent,
                        dispidProp,
                        FALSE,
                        pVariant,
                        (BYTE *) VTS_NONE));
        }

        // see if it was cancelled
        if (pfCancelled)
        {
            BOOL fRet = TRUE;
            
            if ( V_VT(&vb) != VT_EMPTY )
                fRet = (V_VT(&vb) == VT_BOOL) ? V_BOOL(&vb)==VB_TRUE : TRUE;

            fRet = fRet && !param.IsCancelled();

            *pfCancelled = !fRet;
        }

        Release();
    }
    
    return hr;
}


//+----------------------------------------------------------------------------
//
//  Member:     get_bookmarks
//
//  Synopsis:   return a collection of ADO bookmarks corresponding to the
//              hrows affected by the current data change event
//
//  Returns:    HRESULT

HRESULT 
CAdaptingProvider::get_bookmarks(IHTMLBookmarkCollection **ppBookmarkCollection)
{
    HRESULT hr;
    CBookmarkCollection *pBmkColl = NULL;

    if (ppBookmarkCollection == NULL)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    // create a collection
    pBmkColl = new CBookmarkCollection;
    if (pBmkColl == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    Assert(_pCRI);
    _pCRI->InitPosition();      // make sure _pRowPos has valid position

    // fill it with ADO bookmarks
    hr = pBmkColl->Init(_pRowEvents->_rghRows, _pRowEvents->_cRows, _pADO);
    if (hr)
        goto Cleanup;

    // return the answer
    hr = pBmkColl->QueryInterface(IID_IHTMLBookmarkCollection,
                                  (void**)ppBookmarkCollection);

Cleanup:
    ReleaseInterface(pBmkColl);
    RRETURN(hr);
}


//+----------------------------------------------------------------------------
//
//  Member:     get_recordset
//
//  Synopsis:   return the ADO recordset for this provider's dataset
//
//  Returns:    HRESULT

HRESULT 
CAdaptingProvider::get_recordset(IDispatch **ppRecordset)
{
    return QueryDataInterface(IID_IADORecordset15, (void**)ppRecordset);
}


//+----------------------------------------------------------------------------
//
//  Member:     get_dataFld
//
//  Synopsis:   return the name of the data field changed in the current
//              oncellchange event
//
//  Returns:    HRESULT

HRESULT 
CAdaptingProvider::get_dataFld(BSTR *pbstrDataFld)
{
    HRESULT hr;
    
    if (pbstrDataFld)
    {
        if (_pRowEvents->_cColumns > 0)
        {
            LPCTSTR pchDataFld = NULL;

            hr = GetDLCursor()->GetColumnNameFromNumber(_pRowEvents->_rgColumns[0],
                                                        &pchDataFld);
            if (!hr)
                hr = FormsAllocString(pchDataFld, pbstrDataFld);
        }
        else
        {
            *pbstrDataFld = NULL;
            hr = S_OK;
        }
    }
    else
        hr = E_POINTER;

    RRETURN(hr);
}


/////-------------------------------------------------------------------/////
/////                   CDataSourceProvider methods                     /////
/////-------------------------------------------------------------------/////

//+-------------------------------------------------------------------------
// Member:      Create (CDataSourceProvider, static, public)
//
// Synopsis:    Factory method - creates an element's interface provider
//
// Arguments:   pelProvider     underlying element
//              ppdsp           where to put pointer to created provider object
//
// Returns:     S_OK            it worked
//              E_OUTOFMEMORY   amnesia

HRESULT
CDataSourceProvider::Create(CDataMemberMgr *pDMembMgr, CDoc *pDoc,
                            BSTR bstrDataMember, CDataSourceProvider **ppdsp)
{
    Assert(ppdsp);
    
    HRESULT hr = S_OK;
    IUnknown *punk = NULL;
    CAdaptingProvider * pAdaptingProvider = NULL;
    OLEDBSimpleProvider *pSTD;
    IRowset *pRowset;
    IVBDSC *pVBDSC;
    IRowPosition *pRowPos = NULL;
    HCHAPTER hChapter = DB_NULL_HCHAPTER;
    HROW hrow = DB_NULL_HROW;

    *ppdsp = NULL;                      // assume failure

    // get the databinding interface
    if (pDMembMgr)
    {
        // Check for NULL punk instead of HRESULT..
        IGNORE_HR(pDMembMgr->GetDataBindingInterface(bstrDataMember, &punk));
    }

    // match it to the ones we know about
    if (punk == NULL)
        *ppdsp = new CNullProvider(pDoc);
    else if (S_OK == punk->QueryInterface(IID_IRowPosition, (void**)&pRowPos) && pRowPos)
    {
        pRowPos->GetRowPosition(&hChapter, &hrow, NULL);
        pAdaptingProvider = new CAdaptingProvider(pRowPos, hrow);
    }
    else if (S_OK == punk->QueryInterface(IID_IRowset, (void**)&pRowset) && pRowset)
    {
        pAdaptingProvider = new CAdaptingProvider(pRowset);
        ReleaseInterface(pRowset);
    }
    else if (S_OK == punk->QueryInterface(IID_OLEDBSimpleProvider, (void**)&pSTD) && pSTD)
    {
        pAdaptingProvider = new CAdaptingProvider(pSTD);
        ReleaseInterface(pSTD);
    }
    else if (S_OK == punk->QueryInterface(IID_IVBDSC, (void**)&pVBDSC) && pVBDSC)
    {
        pAdaptingProvider = new CAdaptingProvider(pVBDSC);
        ReleaseInterface(pVBDSC);
    }
    else
        *ppdsp = new CNullProvider(pDoc);

    // return the answer (this must be done here;  see below)
    if (pAdaptingProvider)
    {
        *ppdsp = pAdaptingProvider;
    }

    // tell the new provider which data member he's attached to
    if (*ppdsp)
    {
        (*ppdsp)->_cstrDataMember.Set(bstrDataMember);
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    if (pAdaptingProvider)
    {
        // It's very important that *ppdsp be set before we call the Init routine,
        // because the Init may fire script events that could attempt to reference
        // the recordset, which would cause us to arrive here recursively unless
        // our caller's _pdspProvider has already been set.
        hr = pAdaptingProvider->Init(pDMembMgr, hChapter);
    }

    // release the chapter, if we got one
    ReleaseChapterAndRow(hChapter, hrow, pRowPos);
        
    ReleaseInterface(pRowPos);
    ReleaseInterface(punk);
    return hr;
}


//+-------------------------------------------------------------------------
// Member:      Release (CDataSourceProvider, public)
//
// Synopsis:    decrement refcount, and if it's 0 delete myself
//
// Returns:     new refcount

ULONG
CDataSourceProvider::Release()
{
    ULONG ulRefCount = --_ulRefCount;
    if (ulRefCount == 0)
        delete this;
    return ulRefCount;        
}


//+-------------------------------------------------------------------------
// Member:      Detach (CDataSourceProvider, public)
//
// Synopsis:    Detach my resources

void
CDataSourceProvider::Detach()
{
    ReplaceProvider(NULL);
    _aryAdvisees.DeleteAll();
}


//+-------------------------------------------------------------------------
// Member:      AdviseDataProviderEvents (CDataSourceProvider, public)
//
// Synopsis:    add a new advisee
//
// Returns:     HRESULT

HRESULT
CDataSourceProvider::AdviseDataProviderEvents(CDataSourceBinder *pdsb)
{
    HRESULT hr = S_OK;
    const int iEmpty = _aryAdvisees.Find(NULL);

    if (iEmpty != -1)
    {
        _aryAdvisees[iEmpty] = pdsb;
    }
    else
    {
        hr = THR(_aryAdvisees.Append(pdsb));
        if (hr)
            goto Cleanup;
    }
    
Cleanup:
    RRETURN(hr);
}


//+-------------------------------------------------------------------------
// Member:      UnadviseDataProviderEvents (CDataSourceProvider, public)
//
// Synopsis:    remove an advisee
//
// Returns:     HRESULT

HRESULT
CDataSourceProvider::UnadviseDataProviderEvents(CDataSourceBinder *pdsb)
{
    HRESULT hr = S_OK;
    const int iIndex = _aryAdvisees.Find(pdsb);

    if (iIndex >= 0)
    {
        _aryAdvisees[iIndex] = NULL;
    }
    else
    {
        hr = E_UNEXPECTED;
    }

    RRETURN(hr);
}


//+-------------------------------------------------------------------------
// Member:      ReplaceProvider (CDataSourceProvider, public)
//
// Synopsis:    I'm about to be replaced by a new provider.  Inform my advisees.
//
// Returns:     HRESULT

HRESULT
CDataSourceProvider::ReplaceProvider(CDataSourceProvider *pdspNewProvider)
{
    HRESULT hr = S_OK;
    int k;

    for (k=0; k<_aryAdvisees.Size(); ++k)
    {
        if (_aryAdvisees[k])
            _aryAdvisees[k]->ReplaceProvider(pdspNewProvider);
    }

    RRETURN(hr);
}


//+----------------------------------------------------------------------------
//
//  Member:     LoadBoundElementCollection
//
//  Synopsis:   load a collection with the elements bound to my dataset
//
//  Returns:    HRESULT

HRESULT 
CDataSourceProvider::LoadBoundElementCollection(CCollectionCache *pCollectionCache,
                                                long lIndex)
{
    HRESULT hr = S_OK;
    CDataSourceBinder **ppdsb;
    int i;
    
    for (ppdsb=_aryAdvisees, i=_aryAdvisees.Size();  i>0;  ++ppdsb, --i)
    {
        CElement *pBoundElement = (*ppdsb)->GetElementConsumer();
        
        hr = THR(pCollectionCache->SetIntoAry(lIndex, pBoundElement ));
        if (hr)
            goto Cleanup;
    }

Cleanup:
    RRETURN(hr);
}

