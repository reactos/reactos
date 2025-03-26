/*
 * PROJECT:     shell32
 * LICENSE:     LGPL-2.1+ (https://spdx.org/licenses/LGPL-2.1+)
 * PURPOSE:     IShellDispatch implementation
 * COPYRIGHT:   Copyright 2015-2018 Mark Jansen (mark.jansen@reactos.org)
 */

#ifndef _SHELLDISPATCH_H_
#define _SHELLDISPATCH_H_

#undef ShellExecute

class CShellDispatch:
    public CComCoClass<CShellDispatch, &CLSID_Shell>,
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IDispatchImpl<IShellDispatch4, &IID_IShellDispatch4>,
    public IObjectSafety,
    public IObjectWithSite
{
private:

public:
    CShellDispatch();
    ~CShellDispatch();

    HRESULT Initialize();

    // *** IShellDispatch methods ***
    STDMETHOD(get_Application)(IDispatch **ppid) override;
    STDMETHOD(get_Parent)(IDispatch **ppid) override;
    STDMETHOD(NameSpace)(VARIANT vDir, Folder **ppsdf) override;
    STDMETHOD(BrowseForFolder)(LONG Hwnd, BSTR Title, LONG Options, VARIANT RootFolder, Folder **ppsdf) override;
    STDMETHOD(Windows)(IDispatch **ppid) override;
    STDMETHOD(Open)(VARIANT vDir) override;
    STDMETHOD(Explore)(VARIANT vDir) override;
    STDMETHOD(MinimizeAll)() override;
    STDMETHOD(UndoMinimizeALL)() override;
    STDMETHOD(FileRun)() override;
    STDMETHOD(CascadeWindows)() override;
    STDMETHOD(TileVertically)() override;
    STDMETHOD(TileHorizontally)() override;
    STDMETHOD(ShutdownWindows)() override;
    STDMETHOD(Suspend)() override;
    STDMETHOD(EjectPC)() override;
    STDMETHOD(SetTime)() override;
    STDMETHOD(TrayProperties)() override;
    STDMETHOD(Help)() override;
    STDMETHOD(FindFiles)() override;
    STDMETHOD(FindComputer)() override;
    STDMETHOD(RefreshMenu)() override;
    STDMETHOD(ControlPanelItem)(BSTR szDir) override;

    // *** IShellDispatch2 methods ***
    STDMETHOD(IsRestricted)(BSTR group, BSTR restriction, LONG *value) override;
    STDMETHOD(ShellExecute)(BSTR file, VARIANT args, VARIANT dir, VARIANT op, VARIANT show) override;
    STDMETHOD(FindPrinter)(BSTR name, BSTR location, BSTR model) override;
    STDMETHOD(GetSystemInformation)(BSTR name, VARIANT *ret) override;
    STDMETHOD(ServiceStart)(BSTR service, VARIANT persistent, VARIANT *ret) override;
    STDMETHOD(ServiceStop)(BSTR service, VARIANT persistent, VARIANT *ret) override;
    STDMETHOD(IsServiceRunning)(BSTR service, VARIANT *running) override;
    STDMETHOD(CanStartStopService)(BSTR service, VARIANT *ret) override;
    STDMETHOD(ShowBrowserBar)(BSTR clsid, VARIANT show, VARIANT *ret) override;

    // *** IShellDispatch3 methods ***
    STDMETHOD(AddToRecent)(VARIANT file, BSTR category) override;

    // *** IShellDispatch4 methods ***
    STDMETHOD(WindowsSecurity)() override;
    STDMETHOD(ToggleDesktop)() override;
    STDMETHOD(ExplorerPolicy)(BSTR policy, VARIANT *value) override;
    STDMETHOD(GetSetting)(LONG setting, VARIANT_BOOL *result) override;

    // *** IObjectSafety methods ***
    STDMETHOD(GetInterfaceSafetyOptions)(REFIID riid, DWORD *pdwSupportedOptions, DWORD *pdwEnabledOptions) override;
    STDMETHOD(SetInterfaceSafetyOptions)(REFIID riid, DWORD dwOptionSetMask, DWORD dwEnabledOptions) override;

    // *** IObjectWithSite methods ***
    STDMETHOD(SetSite)(IUnknown *pUnkSite) override;
    STDMETHOD(GetSite)(REFIID riid, PVOID *ppvSite) override;

DECLARE_REGISTRY_RESOURCEID(IDR_SHELL)
DECLARE_NOT_AGGREGATABLE(CShellDispatch)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CShellDispatch)
    COM_INTERFACE_ENTRY_IID(IID_IShellDispatch4, IShellDispatch4)
    COM_INTERFACE_ENTRY_IID(IID_IShellDispatch3, IShellDispatch3)
    COM_INTERFACE_ENTRY_IID(IID_IShellDispatch2, IShellDispatch2)
    COM_INTERFACE_ENTRY_IID(IID_IShellDispatch, IShellDispatch)
    COM_INTERFACE_ENTRY_IID(IID_IDispatch, IDispatch)
    COM_INTERFACE_ENTRY_IID(IID_IObjectSafety, IObjectSafety)
    COM_INTERFACE_ENTRY_IID(IID_IObjectWithSite, IObjectWithSite)
END_COM_MAP()
};

#endif /* _SHELLDISPATCH_H_ */
