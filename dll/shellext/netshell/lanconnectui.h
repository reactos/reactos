
/// CLASSID
/// {7007ACC5-3202-11D1-AAD2-00805FC1270E}
/// open network properties and wlan properties

typedef enum
{
    NET_TYPE_CLIENT = 1,
    NET_TYPE_SERVICE = 2,
    NET_TYPE_PROTOCOL = 3
} NET_TYPE;

typedef struct
{
    NET_TYPE Type;
    DWORD dwCharacteristics;
    LPWSTR szHelp;
    INetCfgComponent  *pNCfgComp;
    UINT NumPropDialogOpen;
} NET_ITEM, *PNET_ITEM;

class CNetConnectionPropertyUi:
    public CComCoClass<CNetConnectionPropertyUi, &CLSID_LanConnectionUi>,
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public INetConnectionConnectUi,
    public INetConnectionPropertyUi2,
    public INetLanConnectionUiInfo
{
    public:
        CNetConnectionPropertyUi();
        ~CNetConnectionPropertyUi();

        // INetConnectionPropertyUi2
        virtual HRESULT WINAPI AddPages(HWND hwndParent, LPFNADDPROPSHEETPAGE pfnAddPage, LPARAM lParam);
        virtual HRESULT WINAPI GetIcon(DWORD dwSize, HICON *phIcon);

        // INetLanConnectionUiInfo
        virtual HRESULT WINAPI GetDeviceGuid(GUID *pGuid);

        // INetConnectionConnectUi
        virtual HRESULT WINAPI SetConnection(INetConnection* pCon);
        virtual HRESULT WINAPI Connect(HWND hwndParent, DWORD dwFlags);
        virtual HRESULT WINAPI Disconnect(HWND hwndParent, DWORD dwFlags);

    private:
        BOOL GetINetCfgComponent(INetCfg *pNCfg, INetCfgComponent ** pOut);
        VOID EnumComponents(HWND hDlgCtrl, INetCfg *pNCfg, const GUID *CompGuid, UINT Type);
        VOID InitializeLANPropertiesUIDlg(HWND hwndDlg);
        VOID ShowNetworkComponentProperties(HWND hwndDlg);
        BOOL GetDeviceInstanceID(OUT LPOLESTR *DeviceInstanceID);
        static INT_PTR CALLBACK LANPropertiesUIDlg(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

        CComPtr<INetConnection> m_pCon;
        CComPtr<INetCfgLock> m_NCfgLock;
        CComPtr<INetCfg> m_pNCfg;
        NETCON_PROPERTIES * m_pProperties;

    public:
        DECLARE_NO_REGISTRY()
        DECLARE_NOT_AGGREGATABLE(CNetConnectionPropertyUi)
        DECLARE_PROTECT_FINAL_CONSTRUCT()

        BEGIN_COM_MAP(CNetConnectionPropertyUi)
            COM_INTERFACE_ENTRY_IID(IID_INetConnectionConnectUi, INetConnectionConnectUi)
            COM_INTERFACE_ENTRY_IID(IID_INetConnectionPropertyUi, INetConnectionPropertyUi2)
            COM_INTERFACE_ENTRY_IID(IID_INetConnectionPropertyUi2, INetConnectionPropertyUi2)
            COM_INTERFACE_ENTRY_IID(IID_INetLanConnectionUiInfo, INetLanConnectionUiInfo)
        END_COM_MAP()
};
