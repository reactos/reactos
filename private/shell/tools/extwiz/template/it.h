// $$ClassType$$IT.h : Declaration of the C$$ClassType$$IT

#ifndef __IT_H_
#define __IT_H_

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// C$$ClassType$$IT
class ATL_NO_VTABLE C$$ClassType$$IT : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<C$$ClassType$$IT, &CLSID_$$ClassType$$IT>,
	public IPersistFile, public IQueryInfo
{
        WCHAR _wszFileName[MAX_PATH];
public:
	C$$ClassType$$IT()
	{
	}
    // IPersist methods

    STDMETHODIMP GetClassID(CLSID *pclsid);

    // IPersistFile methods

    STDMETHODIMP IsDirty(void)
        { return E_NOTIMPL; };
    STDMETHODIMP Save(LPCOLESTR pcwszFileName, BOOL bRemember)
        { return E_NOTIMPL; };
    STDMETHODIMP SaveCompleted(LPCOLESTR pcwszFileName)
        { return E_NOTIMPL; };
    STDMETHODIMP Load(LPCOLESTR pcwszFileName, DWORD dwMode);

    STDMETHODIMP GetCurFile(LPOLESTR *ppwszFileName)
        { return E_NOTIMPL; };

    // IQueryInfo methods

    STDMETHODIMP GetInfoTip(DWORD dwFlags, WCHAR **ppwszTip);
    STDMETHODIMP GetInfoFlags(DWORD *pdwFlags);

DECLARE_REGISTRY_RESOURCEID(IDR_SHELLEXTENSIONS)

BEGIN_COM_MAP(C$$ClassType$$IT)
	COM_INTERFACE_ENTRY(IPersistFile)
	COM_INTERFACE_ENTRY(IQueryInfo)
END_COM_MAP()

public:
};

#endif //__$$ClassType$$IT_H_
