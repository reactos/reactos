// adcctl.h : Declaration of the CADCCtl

#ifndef __ADCCTL_H_
#define __ADCCTL_H_

#include "resource.h"       // main symbols

#include "event.h"          // for CEventBroker
#include "iface.h"          // for IARPSimpleProvider
#include <shdispid.h>       // DISPID_ constants

#define NUM_ARP_SIMPLE_PROVIDERS 4

//------------------------------------------------------------------------
//
//  Class:     CADCCtl
//
//  Synopsis:  This is the AppDataControl COM object.
//             It creates a CADCArr object to manage the control's data.
//
//------------------------------------------------------------------------

// CADCCtl
class ATL_NO_VTABLE CADCCtl :
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CADCCtl, &CLSID_ADCCtl>,
    public CComControl<CADCCtl>,
    public CStockPropImpl<CADCCtl, IADCCtl, &IID_IADCCtl, &LIBID_SHAPPMGRLib>,
    public IProvideClassInfo2Impl<&CLSID_ADCCtl, NULL, &LIBID_SHAPPMGRLib>,
    public IPersistStreamInitImpl<CADCCtl>,
    public IPersistStorageImpl<CADCCtl>,        // new
    public IQuickActivateImpl<CADCCtl>,         // new
    public IOleControlImpl<CADCCtl>,
    public IOleObjectImpl<CADCCtl>,
    public IOleInPlaceActiveObjectImpl<CADCCtl>,
    public IViewObjectExImpl<CADCCtl>,
    public IConnectionPointContainerImpl<CADCCtl>,
    public IOleInPlaceObjectWindowlessImpl<CADCCtl>,
    public IPersistPropertyBagImpl<CADCCtl>,         // missing
    public IRunnableObjectImpl<CADCCtl>,             // missing
    public IPropertyNotifySinkCP<CADCCtl>,           // missing
    public IDataObjectImpl<CADCCtl>,            // new
    public ISpecifyPropertyPagesImpl<CADCCtl>   // new
{
public:
    CADCCtl();
    virtual ~CADCCtl();
    
    DECLARE_REGISTRY_RESOURCEID(IDR_ADCCTL)
    DECLARE_NOT_AGGREGATABLE(CADCCtl)

    BEGIN_COM_MAP(CADCCtl) 
        COM_INTERFACE_ENTRY(IADCCtl)
        COM_INTERFACE_ENTRY(IDispatch)          // does order matter?  TDC has IDispatch first
        COM_INTERFACE_ENTRY_IMPL(IViewObjectEx)
        COM_INTERFACE_ENTRY_IMPL_IID(IID_IViewObject2, IViewObjectEx)
        COM_INTERFACE_ENTRY_IMPL_IID(IID_IViewObject, IViewObjectEx)
        COM_INTERFACE_ENTRY_IMPL(IOleInPlaceObjectWindowless)
        COM_INTERFACE_ENTRY_IMPL_IID(IID_IOleInPlaceObject, IOleInPlaceObjectWindowless)
        COM_INTERFACE_ENTRY_IMPL_IID(IID_IOleWindow, IOleInPlaceObjectWindowless)
        COM_INTERFACE_ENTRY_IMPL(IOleInPlaceActiveObject)
        COM_INTERFACE_ENTRY_IMPL(IOleControl)
        COM_INTERFACE_ENTRY_IMPL(IOleObject)
        COM_INTERFACE_ENTRY_IMPL(IQuickActivate)        // new
        COM_INTERFACE_ENTRY_IMPL(IPersistStorage)       // new
        COM_INTERFACE_ENTRY_IMPL(IPersistStreamInit)
        COM_INTERFACE_ENTRY_IMPL(IConnectionPointContainer)
        COM_INTERFACE_ENTRY_IMPL(ISpecifyPropertyPages) // new
        COM_INTERFACE_ENTRY_IMPL(IDataObject)           // new
        COM_INTERFACE_ENTRY(IProvideClassInfo)
        COM_INTERFACE_ENTRY(IProvideClassInfo2)
        COM_INTERFACE_ENTRY_IMPL(IPersistPropertyBag)   // missing
        COM_INTERFACE_ENTRY_IMPL(IRunnableObject)       // missing
    END_COM_MAP()

    BEGIN_PROPERTY_MAP(CADCCtl)
        PROP_ENTRY("Dirty",         DISPID_IADCCTL_DIRTY, CLSID_ADCCtl)
        PROP_ENTRY("Category",      DISPID_IADCCTL_PUBCAT,   CLSID_ADCCtl)
        PROP_ENTRY("ShowPostSetup", DISPID_IADCCTL_SHOWPOSTSETUP, CLSID_ADCCtl)
        PROP_ENTRY("OnDomain",         DISPID_IADCCTL_ONDOMAIN, CLSID_ADCCtl)                
        PROP_ENTRY("DefaultCategory",  DISPID_IADCCTL_DEFAULTCAT, CLSID_ADCCtl)                
    END_PROPERTY_MAP()


    BEGIN_CONNECTION_POINT_MAP(CADCCtl)
        CONNECTION_POINT_ENTRY(IID_IPropertyNotifySink)
    END_CONNECTION_POINT_MAP()


    BEGIN_MSG_MAP(CADCCtl)
        MESSAGE_HANDLER(WM_PAINT, OnPaint)
        MESSAGE_HANDLER(WM_SETFOCUS, OnSetFocus)
        MESSAGE_HANDLER(WM_KILLFOCUS, OnKillFocus)
    END_MSG_MAP()

    // These members and methods expose the IADCCtl interface
    
    // Control methods
    STDMETHOD(IsRestricted) (BSTR bstrPolicy, VARIANT_BOOL * pbRestricted);
    STDMETHOD(Reset)        (BSTR bstrQualifier);
    STDMETHOD(Exec)         (BSTR bstrQualifier, BSTR bstrCmd, LONG nRecord);

    //  Control Properties
    //

    STDMETHOD(get_Dirty)    (VARIANT_BOOL * pbDirty); 
    STDMETHOD(put_Dirty)    (VARIANT_BOOL bDirty);
    STDMETHOD(get_Category) (BSTR * pbstr); 
    STDMETHOD(put_Category) (BSTR bstr);
    STDMETHOD(get_Sort)     (BSTR * pbstr); 
    STDMETHOD(put_Sort)     (BSTR bstr);
    STDMETHOD(get_Forcex86) (VARIANT_BOOL * pbForce); 
    STDMETHOD(put_Forcex86) (VARIANT_BOOL bForce);
    STDMETHOD(get_ShowPostSetup)(VARIANT_BOOL * pbShow);
    STDMETHOD(get_OnDomain) (VARIANT_BOOL * pbOnDomain); 
    STDMETHOD(put_OnDomain) (VARIANT_BOOL bOnDomain);
    STDMETHOD(get_DefaultCategory) (BSTR * pbstr); 

    
    //  Data source notification methods
    STDMETHOD(msDataSourceObject)   (BSTR qualifier, IUnknown **ppUnk);
    STDMETHOD(addDataSourceListener)(IUnknown *pEvent);

    // *** IViewObjectEx ***
    STDMETHOD(GetViewStatus)(DWORD* pdwStatus)
    {
        ATLTRACE(_T("IViewObjectExImpl::GetViewStatus\n"));
        *pdwStatus = VIEWSTATUS_SOLIDBKGND | VIEWSTATUS_OPAQUE;
        return S_OK;
    }

    // *** Overriding ATL functions ***  
    virtual STDMETHODIMP IOleObject_SetClientSite(IOleClientSite *pClientSite);
        
private:

    HRESULT     _CreateMatrixObject(DWORD dwEnum, IARPSimpleProvider ** pparposp);
    HRESULT     _ReleaseMatrixObject(DWORD dwIndex);
    HRESULT     _InitEventBrokers(DataSourceListener * pdsl, BOOL bRecreate);

    // Release all of the matrix objects
    HRESULT     _ReleaseAllMatrixObjects(void);
    HRESULT     _ReleaseAllEventBrokers();
    DWORD       _GetEnumAreaFromQualifier(BSTR bstrQualifier);

    BOOL        _IsMyComputerOnDomain();
    HRESULT     _CheckSecurity(IOleClientSite * pClientSite);
    HRESULT     _KillDatasrcWorkerThread(IARPSimpleProvider * parp);    

    HRESULT     _GetToplevelHWND(void);

    
    IShellAppManager * _psam;               // shell app manager    
    IARPEvent * _rgparpevt[NUM_ARP_SIMPLE_PROVIDERS];
                                            // array of event brokers, each OSP has one cooresponding
    IARPSimpleProvider * _rgparposp[NUM_ARP_SIMPLE_PROVIDERS];
                                            // array of OSP's we carry in this data source object
    
    IOleClientSite * _pclientsite;          // cached client site
    
    CComBSTR    _cbstrCategory;
    CComBSTR    _cbstrSort;
    DWORD       _dwEnum;                    // enumerate which items? (ENUM_*)
    
    BITBOOL     _fInReset: 1;               // TRUE if Reset has already been entered
    BITBOOL     _fDirty : 1;                // TRUE if the recordset is dirty. 
    BITBOOL     _fCategoryChanged: 1;       // TRUE if Category property has changed
    BITBOOL     _fSecure : 1;               // TRUE if we don't have security problem
    BITBOOL     _fOnDomain : 1;             // TRUE if we are running on a machine connected to a domain
    HWND        _hwndTB;                    // Toplevel browser hwnd
};

#endif //__ADCCTL_H_
