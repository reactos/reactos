
typedef struct tagINetConnectionItem
{
    struct tagINetConnectionItem * Next;
    DWORD dwAdapterIndex;
    NETCON_PROPERTIES    Props;
} INetConnectionItem, *PINetConnectionItem;

class CNetConnectionManager:
    public CComCoClass<CNetConnectionManager, &CLSID_ConnectionManager>,
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public INetConnectionManager,
    public IEnumNetConnection
{
    public:
        CNetConnectionManager();
        HRESULT Initialize();
        HRESULT EnumerateINetConnections();

        // INetConnectionManager
        STDMETHOD(EnumConnections)(NETCONMGR_ENUM_FLAGS Flags, IEnumNetConnection **ppEnum) override;

        // IEnumNetConnection
        STDMETHOD(Next)(ULONG celt, INetConnection **rgelt, ULONG *pceltFetched) override;
        STDMETHOD(Skip)(ULONG celt) override;
        STDMETHOD(Reset)() override;
        STDMETHOD(Clone)(IEnumNetConnection **ppenum) override;

    private:
        PINetConnectionItem m_pHead;
        PINetConnectionItem m_pCurrent;

    public:
        DECLARE_NO_REGISTRY()
        DECLARE_NOT_AGGREGATABLE(CNetConnectionManager)
        DECLARE_PROTECT_FINAL_CONSTRUCT()

        BEGIN_COM_MAP(CNetConnectionManager)
            COM_INTERFACE_ENTRY_IID(IID_INetConnectionManager, INetConnectionManager)
            COM_INTERFACE_ENTRY_IID(IID_IEnumNetConnection, IEnumNetConnection)
        END_COM_MAP()
};

class CNetConnection:
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public INetConnection
{
    private:
        NETCON_PROPERTIES m_Props;
        DWORD m_dwAdapterIndex;

    public:
       ~CNetConnection();
        HRESULT WINAPI Initialize(PINetConnectionItem pItem);

        // INetConnection
        STDMETHOD(Connect)() override;
        STDMETHOD(Disconnect)() override;
        STDMETHOD(Delete)() override;
        STDMETHOD(Duplicate)(LPCWSTR pszwDuplicateName, INetConnection **ppCon) override;
        STDMETHOD(GetProperties)(NETCON_PROPERTIES **ppProps) override;
        STDMETHOD(GetUiObjectClassId)(CLSID *pclsid) override;
        STDMETHOD(Rename)(LPCWSTR pszwDuplicateName) override;

        DECLARE_NOT_AGGREGATABLE(CNetConnection)
        DECLARE_PROTECT_FINAL_CONSTRUCT()

        BEGIN_COM_MAP(CNetConnection)
            COM_INTERFACE_ENTRY_IID(IID_INetConnection, INetConnection)
        END_COM_MAP()
};

BOOL GetAdapterIndexFromNetCfgInstanceId(PIP_ADAPTER_INFO pAdapterInfo, LPWSTR szNetCfg, PDWORD pIndex);
HRESULT WINAPI CNetConnectionManager_CreateInstance(REFIID riid, LPVOID * ppv);

