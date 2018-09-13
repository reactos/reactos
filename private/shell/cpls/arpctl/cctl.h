// CCtl.h : Declaration of the CARPCtl

#ifndef __CARPCTL_H_
#define __CARPCTL_H_

// Globals
    extern HINSTANCE g_hInst;

// CAppData

class CAppData
{
private:
    IInstalledApp* m_pAppIns;
    APPINFODATA m_ai;
    SLOWAPPINFO m_aiSlow;
    
public:
    CAppData(IInstalledApp* pas, APPINFODATA* pai)
    {
        m_pAppIns = pas;
        CopyMemory(&m_ai, pai, sizeof m_ai);
        ZeroMemory(&m_aiSlow, sizeof m_aiSlow);
    }
    ~CAppData()
    {
        m_pAppIns->Release();
    }

    APPINFODATA* GetData() { return &m_ai; }
    HRESULT ReadSlowData()
    {
        if (m_pAppIns)
            return m_pAppIns->GetSlowAppInfo(&m_aiSlow);
        else
            return E_FAIL;
    }
    SLOWAPPINFO* GetSlowData() { return &m_aiSlow; }
};

// CARPCtl
class ATL_NO_VTABLE CARPCtl : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CARPCtl, &CLSID_CARPCtl>,
	public IObjectWithSiteImpl<CARPCtl>,
	public IConnectionPointContainerImpl<CARPCtl>,
    public IObjectSafetyImpl<CARPCtl>,
	public IDispatchImpl<IARPCtl, &IID_IARPCtl, &LIBID_ARPCTLLib>,
    public CProxy_ARPCtlEvents<CARPCtl>,
    public IProvideClassInfo2Impl<&CLSID_CARPCtl, &DIID__ARPCtlEvents, &LIBID_ARPCTLLib>
{
private:
    IShellAppManager* m_pam;
    DWORD m_dwCurrentIndex;
    DWORD m_dwcItems;
    HDPA m_hdpaList;
    BOOL m_fKillWorker;
    HANDLE m_hthreadWorker;
    HWND m_hwndWorker;

    void _FreeAppData();
    void _KillWorkerThread();
    HRESULT _CreateWorkerThread();
    DWORD _ThreadStartProc();
    static DWORD CALLBACK _ThreadStartProcWrapper(LPVOID lpParam);
    LRESULT _WorkerWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK _WorkerWndProcWrapper(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

public:
	CARPCtl();
    void FinalRelease();

DECLARE_REGISTRY_RESOURCEID(IDR_CARPCTL)
DECLARE_NOT_AGGREGATABLE(CARPCtl)

BEGIN_COM_MAP(CARPCtl)
	COM_INTERFACE_ENTRY(IARPCtl)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY_IMPL(IConnectionPointContainer)
	COM_INTERFACE_ENTRY_IMPL(IObjectWithSite)
    COM_INTERFACE_ENTRY_IMPL(IObjectSafety)
    COM_INTERFACE_ENTRY(IProvideClassInfo)
    COM_INTERFACE_ENTRY(IProvideClassInfo2)
    COM_INTERFACE_ENTRY_IMPL(IConnectionPointContainer)
END_COM_MAP()

BEGIN_CONNECTION_POINT_MAP(CARPCtl)
    CONNECTION_POINT_ENTRY(DIID__ARPCtlEvents)
END_CONNECTION_POINT_MAP()


// IARPCtl
public:
	STDMETHOD(get_ItemCount)(/*[out, retval]*/ long *pVal);
	STDMETHOD(InitData)(DWORD dwSortOrder);
	STDMETHOD(MoveNext)(BOOL* pbool);
	STDMETHOD(MoveFirst)(BOOL* pbool);
	STDMETHOD(MoveTo)(DWORD dwRecNum, BOOL* pbool);
	STDMETHOD(get_DisplayName)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(get_Version)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(get_Publisher)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(get_ProductID)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(get_RegisteredOwner)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(get_Language)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(get_SupportUrl)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(get_SupportTelephone)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(get_HelpLink)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(get_InstallLocation)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(get_InstallSource)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(get_InstallDate)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(get_RequiredByPolicy)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(get_Contact)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(get_Size)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(get_TimesUsed)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(get_LastUsed)(/*[out, retval]*/ BSTR *pVal);
};

// Window messages from worker thread that cause actions on the main thread
#define WORKERWIN_EXIT_WINDOW       (WM_USER + 0x0100)
#define WORKERWIN_FIRE_ROW_READY    (WM_USER + 0x0101)  // wParam is row #

// Helper prototypes
STDAPI_(void) FileTimeToDateTimeString(LPFILETIME pft, LPTSTR pszBuf, UINT cchBuf);


#endif //__CARPCTL_H_
