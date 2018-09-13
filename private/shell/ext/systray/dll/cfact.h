class CSysTrayFactory: public IClassFactory
{
public:
    // IUnknown Implementation
    HRESULT __stdcall QueryInterface(REFIID iid, void** ppvObject);
    ULONG __stdcall AddRef(void);
    ULONG __stdcall Release(void);

    // IOleCommandTarget Implementation
    HRESULT __stdcall CreateInstance(IUnknown* pUnkOuter, REFIID iid, void** ppvObject);
    HRESULT __stdcall LockServer(BOOL fLock);

    CSysTrayFactory(BOOL fRunTrayOnConstruct);
    ~CSysTrayFactory();
private:
    // Data
    long m_cRef;
    BOOL m_fRunTrayOnConstruct;
};

