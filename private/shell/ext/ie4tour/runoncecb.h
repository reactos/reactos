// RunOnceCheckBox.h : Declaration of the CRunOnceCheckBox

#ifndef __RUNONCECHECKBOX_H_
#define __RUNONCECHECKBOX_H_

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CRunOnceCheckBox
class ATL_NO_VTABLE CRunOnceCheckBox : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CRunOnceCheckBox, &CLSID_RunOnceCheckBox>,
	public IObjectWithSiteImpl<CRunOnceCheckBox>,
	public IDispatchImpl<IRunOnceCheckBox, &IID_IRunOnceCheckBox, &LIBID_RUNONCELib>,
    public IObjectSafetyImpl<CRunOnceCheckBox>
{
public:
	CRunOnceCheckBox()
	{
	}

DECLARE_REGISTRY_RESOURCEID(IDR_RUNONCECHECKBOX)

BEGIN_COM_MAP(CRunOnceCheckBox)
	COM_INTERFACE_ENTRY(IRunOnceCheckBox)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY_IMPL(IObjectWithSite)
    COM_INTERFACE_ENTRY_IMPL(IObjectSafety)
END_COM_MAP()

// IRunOnceCheckBox
public:
	STDMETHOD(get_ShowIE4State)(/*[out, retval]*/ BOOL *pVal);
	STDMETHOD(put_ShowIE4State)(/*[in]*/ BOOL newVal);
};

#endif //__RUNONCECHECKBOX_H_
