// $$ClassType$$EI.h : Declaration of the C$$ClassType$$EI

#ifndef __$$ClassType$$EI_H_
#define __$$ClassType$$EI_H_

#include "resource.h"       // main symbols
#include "shlobj.h"

/////////////////////////////////////////////////////////////////////////////
// CEI
class ATL_NO_VTABLE C$$ClassType$$EI : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<C$$ClassType$$EI, &CLSID_$$ClassType$$EI>,
    public IPersistFile, public IExtractIcon
{
public:
	C$$ClassType$$EI()
	{
	}
    STDMETHODIMP GetClassID(CLSID* pClassID)
        { return E_NOTIMPL; };

    STDMETHODIMP IsDirty(void)
        { return E_NOTIMPL; };
    
    STDMETHODIMP Load(LPCOLESTR pszFileName,
                      DWORD dwMode);
    
    STDMETHODIMP Save(LPCOLESTR pszFileName,
                      BOOL fRemember)
        { return E_NOTIMPL; };
    
    STDMETHODIMP SaveCompleted(LPCOLESTR pszFileName)
        { return E_NOTIMPL; };
    
    STDMETHODIMP GetCurFile(LPOLESTR*  ppszFileName)
        { return E_NOTIMPL; };

    STDMETHODIMP GetIconLocation( UINT   uFlags,
                             LPTSTR  szIconFile,
                             UINT   cchMax,
                             int   * piIndex,
                             UINT  * pwFlags);

    STDMETHODIMP Extract( LPCTSTR pszFile,
                     UINT   nIconIndex,
                     HICON   *phiconLarge,
                     HICON   *phiconSmall,
                     UINT    nIconSize);

DECLARE_REGISTRY_RESOURCEID(IDR_SHELLEXTENSIONS)

BEGIN_COM_MAP(C$$ClassType$$EI)
	COM_INTERFACE_ENTRY(IExtractIcon)
	COM_INTERFACE_ENTRY(IPersistFile)
	COM_INTERFACE_ENTRY(IPersist)
END_COM_MAP()

public:
};

#endif //__$$ClassType$$EI_H_
