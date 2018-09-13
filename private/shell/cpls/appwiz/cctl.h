// CCtl.h : Declaration of the CARPCtl

#ifndef __CARPCTL_H_
#define __CARPCTL_H_

#include "events.h"
#include "util.h"
#include "mtxarray.h"       // for CAppData
#include "worker.h"         // for IWorkerEvent

// CARPCtl

class ATL_NO_VTABLE CARPCtl : 
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CARPCtl, &CLSID_CARPCtl>,
    public IObjectWithSiteImpl<CARPCtl>,
    public IConnectionPointContainerImpl<CARPCtl>,
    public IObjectSafetyImpl<CARPCtl>,
    public IDispatchImpl<IARPCtl, &IID_IARPCtl, &LIBID_ARPCTLLib>,
    public CProxy_ARPCtlEvents<CARPCtl>,
    public IProvideClassInfo2Impl<&CLSID_CARPCtl, &DIID__ARPCtlEvents, &LIBID_ARPCTLLib>,
    public IWorkerEvent
{
private:
    DWORD   _dwCurrentIndex;
    DWORD   _dwcItems;
    DWORD   _dwEnum;            // One of ENUM_*

    IShellAppManager* _pam;
    CMtxArray2 * _pmtxarray;

    CWorkerThread _workerthread;
    
    BITBOOL _fSecure: 1;

    void    _FreeAppData();
    CAppData * _GetAppData(DWORD iItem);

public:
    CARPCtl();
    ~CARPCtl();
    
    void EnumCallback(CAppData * pcad);

DECLARE_REGISTRY_RESOURCEID(IDR_CARPCTL)
DECLARE_NOT_AGGREGATABLE(CARPCtl)

BEGIN_COM_MAP(CARPCtl)
    COM_INTERFACE_ENTRY(IARPCtl)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY_IMPL(IConnectionPointContainer)
    COM_INTERFACE_ENTRY_IMPL(IObjectWithSite)
    COM_INTERFACE_ENTRY_IMPL(IObjectSafety)
    COM_INTERFACE_ENTRY(IProvideClassInfo)
    COM_INTERFACE_ENTRY(IProvideClassInfo2)
    COM_INTERFACE_ENTRY_IMPL(IConnectionPointContainer)
END_COM_MAP()

BEGIN_CONNECTION_POINT_MAP(CARPCtl)
    CONNECTION_POINT_ENTRY(DIID__ARPCtlEvents)
END_CONNECTION_POINT_MAP()


// IARPCtl
public:
    // *** IWorkerEvent methods ***
    STDMETHOD(FireOnDataReady)  (LONG iRow);
    STDMETHOD(FireOnFinished)   (void);

    STDMETHOD(get_ItemCount)(/*[out, retval]*/ long *pVal);
    STDMETHOD(InitData)(BSTR bstrEnum, DWORD dwSortOrder);
    STDMETHOD(MoveNext)(BOOL* pbool);
    STDMETHOD(MoveFirst)(BOOL* pbool);
    STDMETHOD(MoveTo)(DWORD dwRecNum, BOOL* pbool);
    STDMETHOD(Exec)(BSTR bstrExec);
    STDMETHOD(get_DisplayName)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(get_Version)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(get_Publisher)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(get_ProductID)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(get_RegisteredOwner)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(get_Language)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(get_SupportUrl)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(get_SupportTelephone)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(get_HelpLink)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(get_InstallLocation)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(get_InstallSource)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(get_InstallDate)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(get_RequiredByPolicy)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(get_Contact)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(get_Size)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(get_TimesUsed)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(get_LastUsed)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(get_Capability)(/*[out, retval]*/ long *pVal);
};


#endif //__CARPCTL_H_
