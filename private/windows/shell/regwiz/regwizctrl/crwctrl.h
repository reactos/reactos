// CRegWizCtrl.h : Declaration of the CRegWizCtrl

#ifndef __REGWIZCTRL_H_
#define __REGWIZCTRL_H_

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CRegWizCtrl
class ATL_NO_VTABLE CRegWizCtrl : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CRegWizCtrl, &CLSID_RegWizCtrl>,
	public IObjectWithSiteImpl<CRegWizCtrl>,
	public ISupportErrorInfo,
	public IDispatchImpl<IRegWizCtrl, &IID_IRegWizCtrl, &LIBID_REGWIZCTRLLib>
{
public:
	CRegWizCtrl()
	{
	}

DECLARE_REGISTRY_RESOURCEID(IDR_REGWIZCTRL)

BEGIN_COM_MAP(CRegWizCtrl)
	COM_INTERFACE_ENTRY(IRegWizCtrl)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
	COM_INTERFACE_ENTRY_IMPL(IObjectWithSite)
END_COM_MAP()

// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

// IRegWizCtrl
public:
	STDMETHOD(get_Version)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(TransferOEMRegWizInformation)(BSTR InformationPath);
	STDMETHOD(get_HWID)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_HWID)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_MSID)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_MSID)(/*[in]*/ BSTR newVal);
	STDMETHOD(TransferRegWizInformation)(BSTR  InformationPath);
	STDMETHOD(InvokeRegWizard)(BSTR ProductPath);
	STDMETHOD(get_IsRegistered)(VARIANT_BOOL *pbStatus);
	STDMETHOD(put_IsRegistered)(BSTR ProductPath);
};

#endif //__REGWIZCTRL_H_
