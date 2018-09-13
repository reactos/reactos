//+---------------------------------------------------------------------------
//
//  Microsoft Internet Explorer
//  Copyright (C) Microsoft Corporation, 1998
//
//  File:       dmembmgr.hxx
//
//  Contents:   CDataMemberMgr and related classes.
//
//----------------------------------------------------------------------------

#ifndef I_DMEMBMGR_HXX_
#define I_DMEMBMGR_HXX_
#pragma INCMSG("--- Beg 'dmembmgr.hxx'")

#ifndef X_MSDATSRC_H_
#define X_MSDATSRC_H_
#pragma INCMSG("--- Beg <msdatsrc.h>")
#include <msdatsrc.h>
#pragma INCMSG("--- End <msdatsrc.h>")
#endif

#ifndef X_DSLISTEN_H_
#define X_DSLISTEN_H_
#pragma INCMSG("--- Beg <dslisten.h>")
#include <dslisten.h>
#pragma INCMSG("--- End <dslisten.h>")
#endif

#define PUNKDB_UNKNOWN ((IUnknown *) -1)

// forward references
interface DataSource;
interface DataSourceListener;
interface ISimpleDataConverter;
class CDataSourceProvider;
class COleSite;
class CScriptElement;
class CGenericElement;
class CDoc;

MtExtern(CDataMemberMgr);
MtExtern(CDataMemberMgr_aryDataMember_pv);

class CDataMemberMgr: public DataSourceListener, public IDATASRCListener
{
public:
    static HRESULT Create(CElement *pElement, CDataMemberMgr **ppMgr);
    HRESULT GetDataSourceProvider(BSTR bstrMember, CDataSourceProvider **ppdsp);
    HRESULT GetDataBindingInterface(BSTR bstrMember, IUnknown **ppunkDataBinding);
    HRESULT ChangeDataBindingInterface(BSTR bstrMember, BOOL fDataAvail);
    void    EnsureDataEvents();
    static void EnsureDataEventsFor(CBase *pBase, DISPID dispid);

    CDoc *  Doc() { return _pDoc; }
    CElement * GetOwner() { return _pElementOwner; }
    void    Detach();
    void    DetachDataProviders();
    void    Notify(CNotification *pnf);
    HRESULT IsReady();
    BOOL    IsDataProvider() { return GetDataProviderType() > DPT_NOTAPROVIDER; }
    DISPID  GetDatabindingDispid() { return _dispidDataBinding; }
    HRESULT GetTridentAsOSP(CDoc **ppDocOSP);
    ISimpleDataConverter * GetSDC() { return _pSDC; }

    HRESULT namedRecordset(BSTR bstrDatamember,
                               VARIANT *pvarHierarchy,
                               IDispatch **pRecordSet);

    // IUnknown methods
    STDMETHOD(QueryInterface)(REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG,AddRef)(void);
    STDMETHOD_(ULONG,Release)(void);

    // DataSourceListener methods
    HRESULT STDMETHODCALLTYPE dataMemberChanged(BSTR bstrDM);
    HRESULT STDMETHODCALLTYPE dataMemberAdded(BSTR bstrDM);
    HRESULT STDMETHODCALLTYPE dataMemberRemoved(BSTR bstrDM);

    // IDATASRCListener methods
    HRESULT STDMETHODCALLTYPE datasrcChanged(BSTR bstrQualifier, BOOL fDataAvail);

private:
    enum ELEMENT_TYPE { ET_OLESITE, ET_SCRIPT, ET_XML };
    enum DATA_PROVIDER_TYPE { DPT_UNKNOWN=0, DPT_NOTAPROVIDER,
                    DPT_COM, DPT_JAVA, DPT_PROPERTY, DPT_ICURSOR, DPT_DATASOURCE };
    struct CDataMemberRecord
    {
        BSTR                _bstrName;          // name of data member
        CDataSourceProvider *_pdspProvider;     // data interface provider
        IUnknown *          _punkDataBinding;   // databinding interface
    };
    
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CDataMemberMgr));
    CDataMemberMgr(ELEMENT_TYPE et, CElement *pElement);
    ~CDataMemberMgr();
    
    CDataMemberRecord * GetDataMemberRecord(BSTR bstrName);
    CDataMemberRecord * AddDataMemberRecord(BSTR bstrName);
    void    FindDataProviderType();
    DATA_PROVIDER_TYPE GetDataProviderType();
    HRESULT EnsureDataBindingInterface(BSTR bstrMember);
    BOOL    IllegalCall(DWORD dwFlags);

    ULONG               _ulRefs;            // reference count
    DATA_PROVIDER_TYPE  _dpt;               // what kind of DSO am I
    DataSource *        _pDataSource;       // many DSOs use IDataSource
    CDataAry<CDataMemberRecord> _aryDataMember; // info about data members
    DISPID              _dispidDataBinding; // 

    CDoc *              _pDoc;              // containing document
    ELEMENT_TYPE        _et;                // what type of element I act for
    union {                                 // the element I act for
        COleSite *      _pOleSite;          //   ET_OLESITE
        CScriptElement *_pScript;           //   ET_SCRIPT
        CGenericElement *_pXML;             //   ET_XML
    };
    CElement *          _pElementOwner;     //   generic - any of the above

    ISimpleDataConverter *_pSDC;            // some DSOs use ISimpleDataConverter
};

#pragma INCMSG("--- End 'dmembmgr.hxx'")
#else
#pragma INCMSG("*** Dup 'dmembmgr.hxx'")
#endif

