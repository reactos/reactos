extern const CLSID CLSID_SnapIn;    // In-Proc server GUID
extern const CLSID CLSID_Extension; // In-Proc server GUID
extern const CLSID CLSID_About;

//
// INTERNAL
//

struct INTERNAL
{
    INTERNAL() { m_type = CCT_UNINITIALIZED; m_cookie = -1;};
    ~INTERNAL() {}

    DATA_OBJECT_TYPES   m_type;     // What context is the data object.
    MMC_COOKIE          m_cookie;   // What object the cookie represents
    CLSID               m_clsid;    // Class ID of who created this data object

    INTERNAL & operator=(const INTERNAL& rhs)
    {
        if (&rhs == this)
            return *this;

        m_type = rhs.m_type;
        m_cookie = rhs.m_cookie;

        return *this;
    }
};

INTERNAL *   ExtractInternalFormat(LPDATAOBJECT lpDataObject);
GUID *       ExtractNodeType(LPDATAOBJECT lpDataObject);
CLSID *      ExtractClassID(LPDATAOBJECT lpDataObject);

#define FREE_INTERNAL(pInternal) \
    do { if (pInternal != NULL) \
        GlobalFree(pInternal); } \
    while(0);

//
// CComponentData class
//

class CComponentData:
    public IComponentData,
    public CComObjectRoot
{
BEGIN_COM_MAP(CComponentData)
    COM_INTERFACE_ENTRY(IComponentData)
END_COM_MAP()

    friend class CSnapIn;
    friend class CDataObject;

    CComponentData();
    ~CComponentData();

protected:
    ULONG                m_cRef;
    LPGPEINFORMATION     m_pGPTInformation; // GPT Editor's interface
    HWND                 m_hwndFrame;       // Main window handle

public:
    virtual const CLSID& GetCoClassID() = 0;

public:
    // IUnknown methods
    STDMETHODIMP         QueryInterface(REFIID, LPVOID FAR *);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    //
    // Implemented IComponentData methods
    //

    STDMETHODIMP         CreateComponent(LPCOMPONENT* ppComponent);
    STDMETHODIMP         QueryDataObject(MMC_COOKIE cookie, DATA_OBJECT_TYPES type, LPDATAOBJECT* ppDataObject);
    STDMETHODIMP         Initialize(LPUNKNOWN pUnknown);
    STDMETHODIMP         Notify(LPDATAOBJECT lpDataObject, MMC_NOTIFY_TYPE event, LPARAM arg, LPARAM param);
    STDMETHODIMP         Destroy(void);
    STDMETHODIMP         GetDisplayInfo(LPSCOPEDATAITEM pItem);

    //
    // Unimplemented IComponentData methods
    //

    STDMETHOD(CompareObjects)(LPDATAOBJECT lpDataObjectA, LPDATAOBJECT lpDataObjectB)
    { return E_NOTIMPL; };

private:
    CFolder* GetFolder(void);
    void CreateFolder(LPDATAOBJECT lpDataObject);
    void EnumerateScopePane(LPDATAOBJECT lpDataObject, HSCOPEITEM pParent);

    LPCONSOLENAMESPACE  m_pScope;
    CFolder* m_Folder;

};


class CComponentDataPrimary : public CComponentData,
    public CComCoClass<CComponentDataPrimary, &CLSID_SnapIn>
{
public:
    DECLARE_REGISTRY(CSnapin, _T("PwrAdmin.Snapin.1"), _T("PwrAdmin.Snapin"), IDS_SNAPIN_DESC, THREADFLAGS_BOTH)
    virtual const CLSID & GetCoClassID(){ return CLSID_SnapIn; }
};

class CComponentDataExtension : public CComponentData,
    public CComCoClass<CComponentDataExtension, &CLSID_Extension>
{
public:
    DECLARE_REGISTRY(CSnapin, _T("PwrAdmin.Extension.1"), _T("PwrAdmin.Extension"), IDS_SNAPIN_DESC, THREADFLAGS_BOTH)
    virtual const CLSID & GetCoClassID(){ return CLSID_Extension; }
};


//
// SnapIn class
//

class CSnapIn:
    public IComponent,
    public CComObjectRoot
{

BEGIN_COM_MAP(CSnapIn)
    COM_INTERFACE_ENTRY(IComponent)
END_COM_MAP()

    friend class CDataObject;

protected:
    ULONG               m_cRef;
    LPCONSOLE           m_pConsole;     // Console's IFrame interface
    LPCOMPONENTDATA     m_pComponentData;
    LPRESULTDATA        m_pResult;      // Result pane's interface
    LPHEADERCTRL        m_pHeader;      // Result pane's header control interface
    LPIMAGELIST         m_pImageResult; // Result pane's image list interface
    LPCONSOLEVERB       m_pConsoleVerb; // pointer the console verb
    CComponentData      *m_pcd;
    LONG                m_lViewMode;    // View mode


// Header titles
    WCHAR m_description[255];
    WCHAR m_column1[20];      // Name

    WCHAR m_TitleSystem[40];
    WCHAR m_TitleMonitor[40];

    HRESULTITEM m_itemSystem;
    HRESULTITEM m_itemMonitor;

public:
    CSnapIn(CComponentData *pComponent);
    ~CSnapIn();

    //
    // IUnknown methods
    //

    STDMETHODIMP         QueryInterface(REFIID, LPVOID FAR *);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    //
    // Implemented IComponent methods
    //

    STDMETHODIMP        Initialize(LPCONSOLE);
    STDMETHODIMP        Destroy(MMC_COOKIE);
    STDMETHODIMP        Notify(LPDATAOBJECT, MMC_NOTIFY_TYPE, LPARAM, LPARAM);
    STDMETHODIMP        GetDisplayInfo(LPRESULTDATAITEM);
    STDMETHODIMP        QueryDataObject(MMC_COOKIE, DATA_OBJECT_TYPES, LPDATAOBJECT *);

    //
    // Unimplemented IComponent methods
    //

    STDMETHOD(GetResultViewType)(MMC_COOKIE, LPOLESTR*, long*)
    { return S_FALSE; };

    STDMETHOD(CompareObjects)(LPDATAOBJECT, LPDATAOBJECT)
    { return E_NOTIMPL; };
    //
    // Helpers
public:
    void SetIComponentData(CComponentData* pData);

private:
    //
    // Internal methods
    //

    STDMETHODIMP_(void) DisplayPropertyPage(long ItemId);
    static INT_PTR CALLBACK SystemDlgProc(HWND hWnd, UINT uMsg,
                                       WPARAM wParam, LPARAM lParam);
    static INT_PTR CALLBACK MonitorDlgProc(HWND hWnd, UINT uMsg,
                                       WPARAM wParam, LPARAM lParam);
};

inline void CSnapIn::SetIComponentData(CComponentData* pData)
{
    HRESULT hr;

    hr = pData->QueryInterface(IID_IComponentData, reinterpret_cast<void**>(&m_pComponentData));

}
