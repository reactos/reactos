#if !defined(__thunk_h) && defined(UNICODE)
#define __thunk_h


/*-----------------------------------------------------------------------------
/ CPersistQueryA2W
/----------------------------------------------------------------------------*/

class CPersistQueryA2W : public IPersistQueryA, CUnknown
{
    private:
        IPersistQueryW* m_pPersistQuery;

    public:
        CPersistQueryA2W(IPersistQueryW* pPersistQuery);
        ~CPersistQueryA2W();

        // IUnknown
        STDMETHOD(QueryInterface)(REFIID riid, LPVOID* ppvObject);
        STDMETHOD_(ULONG, AddRef)();
        STDMETHOD_(ULONG, Release)();

        // IPersist
        STDMETHOD(GetClassID)(THIS_ CLSID* pClassID);

        // IPersistQuery
        STDMETHOD(WriteString)(THIS_ LPCSTR pSection, LPCSTR pValueName, LPCSTR pValue);
        STDMETHOD(ReadString)(THIS_ LPCSTR pSection, LPCSTR pValueName, LPSTR pBuffer, INT cchBuffer);
        STDMETHOD(WriteInt)(THIS_ LPCSTR pSection, LPCSTR pValueName, INT value);
        STDMETHOD(ReadInt)(THIS_ LPCSTR pSection, LPCSTR pValueName, LPINT pValue);
        STDMETHOD(WriteStruct)(THIS_ LPCSTR pSection, LPCSTR pValueName, LPVOID pStruct, DWORD cbStruct);
        STDMETHOD(ReadStruct)(THIS_ LPCSTR pSection, LPCSTR pValueName, LPVOID pStruct, DWORD cbStruct);
        STDMETHOD(Clear)(THIS);
};


/*-----------------------------------------------------------------------------
/ CPersistQueryW2A
/----------------------------------------------------------------------------*/

class CPersistQueryW2A : public IPersistQueryW, CUnknown
{
    private:
        IPersistQueryA* m_pPersistQuery;

    public:
        CPersistQueryW2A(IPersistQueryA* pPersistQuery);
        ~CPersistQueryW2A();

        // IUnknown
        STDMETHOD(QueryInterface)(REFIID riid, LPVOID* ppvObject);
        STDMETHOD_(ULONG, AddRef)();
        STDMETHOD_(ULONG, Release)();

        // IPersist
        STDMETHOD(GetClassID)(THIS_ CLSID* pClassID);

        // IPersistQuery
        STDMETHOD(WriteString)(THIS_ LPCWSTR pSection, LPCWSTR pValueName, LPCWSTR pValue);
        STDMETHOD(ReadString)(THIS_ LPCWSTR pSection, LPCWSTR pValueName, LPWSTR pBuffer, INT cchBuffer);
        STDMETHOD(WriteInt)(THIS_ LPCWSTR pSection, LPCWSTR pValueName, INT value);
        STDMETHOD(ReadInt)(THIS_ LPCWSTR pSection, LPCWSTR pValueName, LPINT pValue);
        STDMETHOD(WriteStruct)(THIS_ LPCWSTR pSection, LPCWSTR pValueName, LPVOID pStruct, DWORD cbStruct);
        STDMETHOD(ReadStruct)(THIS_ LPCWSTR pSection, LPCWSTR pValueName, LPVOID pStruct, DWORD cbStruct);
        STDMETHOD(Clear)(THIS);
};


#endif
