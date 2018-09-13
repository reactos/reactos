// DEInsTab.h : Declaration of the CDEInsertTableParam
// Copyright (c)1997-1999 Microsoft Corporation, All Rights Reserved

#ifndef __DEINSERTTABLEPARAM_H_
#define __DEINSERTTABLEPARAM_H_

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CDEInsertTableParam
class ATL_NO_VTABLE CDEInsertTableParam : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CDEInsertTableParam, &CLSID_DEInsertTableParam>,
	public IDispatchImpl<IDEInsertTableParam, &IID_IDEInsertTableParam, &LIBID_DHTMLEDLib>
{
public:
	CDEInsertTableParam();
	~CDEInsertTableParam();

private:

	ULONG	m_nNumRows;
	ULONG	m_nNumCols;
	BSTR	m_bstrTableAttrs;
	BSTR	m_bstrCellAttrs;
	BSTR	m_bstrCaption;

public:

DECLARE_REGISTRY_RESOURCEID(IDR_DEINSERTTABLEPARAM)

BEGIN_COM_MAP(CDEInsertTableParam)
	COM_INTERFACE_ENTRY(IDEInsertTableParam)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// IDEInsertTableParam
public:
	STDMETHOD(get_Caption)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_Caption)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_CellAttrs)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_CellAttrs)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_TableAttrs)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_TableAttrs)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_NumCols)(/*[out, retval]*/ LONG *pVal);
	STDMETHOD(put_NumCols)(/*[in]*/ LONG newVal);
	STDMETHOD(get_NumRows)(/*[out, retval]*/ LONG *pVal);
	STDMETHOD(put_NumRows)(/*[in]*/ LONG newVal);
};

#endif //__DEINSERTTABLEPARAM_H_
