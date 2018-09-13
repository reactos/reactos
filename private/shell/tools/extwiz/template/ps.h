// $$ClassType$$PS.h : Declaration of the C$$ClassType$$PS

#ifndef __$$ClassType$$PS_H_
#define __$$ClassType$$PS_H_

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// C$$ClassType$$PS
class ATL_NO_VTABLE C$$ClassType$$PS : 
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<C$$ClassType$$PS, &CLSID_$$ClassType$$PS>,
    public IShellPropSheetExt, public IShellExtInit
{
public:
    C$$ClassType$$PS()
    {
    }
    STDMETHODIMP Initialize (LPCITEMIDLIST pidlFolder, LPDATAOBJECT lpdobj, 
                         HKEY hkeyProgID);

    STDMETHODIMP AddPages(LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam);
    STDMETHODIMP ReplacePage(UINT uPageID, LPFNADDPROPSHEETPAGE lpfnReplaceWith, LPARAM lParam);

DECLARE_REGISTRY_RESOURCEID(IDR_SHELLEXTENSIONS)

BEGIN_COM_MAP(C$$ClassType$$PS)
    COM_INTERFACE_ENTRY(IShellPropSheetExt)
    COM_INTERFACE_ENTRY(IShellExtInit)
END_COM_MAP()

public:
};

#endif //__$$ClassType$$PS_H_
