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
    virtual HRESULT STDMETHODCALLTYPE get_Application(IDispatch **ppid);
    virtual HRESULT STDMETHODCALLTYPE get_Parent(IDispatch **ppid);
    virtual HRESULT STDMETHODCALLTYPE NameSpace(VARIANT vDir, Folder **ppsdf);
    virtual HRESULT STDMETHODCALLTYPE BrowseForFolder(LONG Hwnd, BSTR Title, LONG Options, VARIANT RootFolder, Folder **ppsdf);
    virtual HRESULT STDMETHODCALLTYPE Windows(IDispatch **ppid);
    virtual HRESULT STDMETHODCALLTYPE Open(VARIANT vDir);
    virtual HRESULT STDMETHODCALLTYPE Explore(VARIANT vDir);
    virtual HRESULT STDMETHODCALLTYPE MinimizeAll();
    virtual HRESULT STDMETHODCALLTYPE UndoMinimizeALL();
    virtual HRESULT STDMETHODCALLTYPE FileRun();
    virtual HRESULT STDMETHODCALLTYPE CascadeWindows();
    virtual HRESULT STDMETHODCALLTYPE TileVertically();
    virtual HRESULT STDMETHODCALLTYPE TileHorizontally();
    virtual HRESULT STDMETHODCALLTYPE ShutdownWindows();
    virtual HRESULT STDMETHODCALLTYPE Suspend();
    virtual HRESULT STDMETHODCALLTYPE EjectPC();
    virtual HRESULT STDMETHODCALLTYPE SetTime();
    virtual HRESULT STDMETHODCALLTYPE TrayProperties();
    virtual HRESULT STDMETHODCALLTYPE Help();
    virtual HRESULT STDMETHODCALLTYPE FindFiles();
    virtual HRESULT STDMETHODCALLTYPE FindComputer();
    virtual HRESULT STDMETHODCALLTYPE RefreshMenu();
    virtual HRESULT STDMETHODCALLTYPE ControlPanelItem(BSTR szDir);

    // *** IShellDispatch2 methods ***
    virtual HRESULT STDMETHODCALLTYPE IsRestricted(BSTR group, BSTR restriction, LONG *value);
    virtual HRESULT STDMETHODCALLTYPE ShellExecute(BSTR file, VARIANT args, VARIANT dir, VARIANT op, VARIANT show);
    virtual HRESULT STDMETHODCALLTYPE FindPrinter(BSTR name, BSTR location, BSTR model);
    virtual HRESULT STDMETHODCALLTYPE GetSystemInformation(BSTR name, VARIANT *ret);
    virtual HRESULT STDMETHODCALLTYPE ServiceStart(BSTR service, VARIANT persistent, VARIANT *ret);
    virtual HRESULT STDMETHODCALLTYPE ServiceStop(BSTR service, VARIANT persistent, VARIANT *ret);
    virtual HRESULT STDMETHODCALLTYPE IsServiceRunning(BSTR service, VARIANT *running);
    virtual HRESULT STDMETHODCALLTYPE CanStartStopService(BSTR service, VARIANT *ret);
    virtual HRESULT STDMETHODCALLTYPE ShowBrowserBar(BSTR clsid, VARIANT show, VARIANT *ret);

    // *** IShellDispatch3 methods ***
    virtual HRESULT STDMETHODCALLTYPE AddToRecent(VARIANT file, BSTR category);

    // *** IShellDispatch4 methods ***
    virtual HRESULT STDMETHODCALLTYPE WindowsSecurity();
    virtual HRESULT STDMETHODCALLTYPE ToggleDesktop();
    virtual HRESULT STDMETHODCALLTYPE ExplorerPolicy(BSTR policy, VARIANT *value);
    virtual HRESULT STDMETHODCALLTYPE GetSetting(LONG setting, VARIANT_BOOL *result);

    // *** IObjectSafety methods ***
    virtual HRESULT STDMETHODCALLTYPE GetInterfaceSafetyOptions(REFIID riid, DWORD *pdwSupportedOptions, DWORD *pdwEnabledOptions);
    virtual HRESULT STDMETHODCALLTYPE SetInterfaceSafetyOptions(REFIID riid, DWORD dwOptionSetMask, DWORD dwEnabledOptions);

    // *** IObjectWithSite methods ***
    virtual HRESULT STDMETHODCALLTYPE SetSite(IUnknown *pUnkSite);
    virtual HRESULT STDMETHODCALLTYPE GetSite(REFIID riid, PVOID *ppvSite);


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
