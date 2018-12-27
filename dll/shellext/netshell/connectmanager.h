
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
        virtual HRESULT WINAPI EnumConnections(NETCONMGR_ENUM_FLAGS Flags, IEnumNetConnection **ppEnum);

        // IEnumNetConnection
        virtual HRESULT WINAPI Next(ULONG celt, INetConnection **rgelt, ULONG *pceltFetched);
        virtual HRESULT WINAPI Skip(ULONG celt);
        virtual HRESULT WINAPI Reset();
        virtual HRESULT WINAPI Clone(IEnumNetConnection **ppenum);

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
        HRESULT WINAPI Connect();
        HRESULT WINAPI Disconnect();
        HRESULT WINAPI Delete();
        HRESULT WINAPI Duplicate(LPCWSTR pszwDuplicateName, INetConnection **ppCon);
        HRESULT WINAPI GetProperties(NETCON_PROPERTIES **ppProps);
        HRESULT WINAPI GetUiObjectClassId(CLSID *pclsid);
        HRESULT WINAPI Rename(LPCWSTR pszwDuplicateName);

        DECLARE_NOT_AGGREGATABLE(CNetConnection)
        DECLARE_PROTECT_FINAL_CONSTRUCT()

        BEGIN_COM_MAP(CNetConnection)
            COM_INTERFACE_ENTRY_IID(IID_INetConnection, INetConnection)
        END_COM_MAP()
};

BOOL GetAdapterIndexFromNetCfgInstanceId(PIP_ADAPTER_INFO pAdapterInfo, LPWSTR szNetCfg, PDWORD pIndex);
HRESULT WINAPI CNetConnectionManager_CreateInstance(REFIID riid, LPVOID * ppv);

