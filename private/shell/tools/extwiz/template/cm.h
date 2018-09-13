// $$ClassType$$CM.h : Declaration of the C$$ClassType$$CM

#ifndef __$$ClassType$$CM_H_
#define __$$ClassType$$CM_H_

#include "resource.h"       // main symbols
#include "shlobj.h"

/////////////////////////////////////////////////////////////////////////////
// CCM
class ATL_NO_VTABLE C$$ClassType$$CM : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<C$$ClassType$$CM, &CLSID_$$ClassType$$CM>,
	public IContextMenu3,
    public IShellExtInit
{

public:
	C$$ClassType$$CM()
	{
	}

    STDMETHODIMP Initialize ( LPCITEMIDLIST pidlFolder,
                         LPDATAOBJECT lpdobj, 
                         HKEY hkeyProgID);

    STDMETHODIMP QueryContextMenu( HMENU hmenu,
                                UINT indexMenu,
                                UINT idCmdFirst,
                                UINT idCmdLast,
                                UINT uFlags);

    STDMETHODIMP InvokeCommand(LPCMINVOKECOMMANDINFO lpici);

    STDMETHODIMP GetCommandString( UINT idCmd,
                                UINT uType,
                                UINT* pwReserved,
                                LPSTR pszName,
                                UINT cchMax);

    STDMETHODIMP HandleMenuMsg(UINT uMsg,
                         WPARAM wParam,
                         LPARAM lParam)
    {   return HandleMenuMsg2(uMsg, wParam, lParam, NULL);  }


    STDMETHODIMP HandleMenuMsg2(UINT uMsg,
                             WPARAM wParam,
                             LPARAM lParam,
                             LRESULT* plResult);


DECLARE_REGISTRY_RESOURCEID(IDR_SHELLEXTENSIONS)

BEGIN_COM_MAP(C$$ClassType$$CM)
	COM_INTERFACE_ENTRY(IShellExtInit)
	COM_INTERFACE_ENTRY(IContextMenu)
	COM_INTERFACE_ENTRY(IContextMenu2)
	COM_INTERFACE_ENTRY(IContextMenu3)
END_COM_MAP()

public:
};

#endif //__$$ClassType$$CM_H_
