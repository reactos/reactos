#ifndef __io_h
#define __io_h


/*-----------------------------------------------------------------------------
/ CDsPersistQuery
/----------------------------------------------------------------------------*/

class CDsPersistQuery : public IPersistQuery, CUnknown
{
    private:
        TCHAR m_szFilename[MAX_PATH];

    public:
        CDsPersistQuery(LPCTSTR pFilename);;

        // IUnknown
        STDMETHOD(QueryInterface)(REFIID riid, LPVOID* ppvObject);
        STDMETHOD_(ULONG, AddRef)();
        STDMETHOD_(ULONG, Release)();

        // IPersist
        STDMETHOD(GetClassID)(THIS_ CLSID* pClassID);

        // IPersistQuery
        STDMETHOD(WriteString)(THIS_ LPCTSTR pSection, LPCTSTR pValueName, LPCTSTR pValue);
        STDMETHOD(ReadString)(THIS_ LPCTSTR pSection, LPCTSTR pValueName, LPTSTR pBuffer, INT cchBuffer);
        STDMETHOD(WriteInt)(THIS_ LPCTSTR pSection, LPCTSTR pValueName, INT value);
        STDMETHOD(ReadInt)(THIS_ LPCTSTR pSection, LPCTSTR pValueName, LPINT pValue);
        STDMETHOD(WriteStruct)(THIS_ LPCTSTR pSection, LPCTSTR pValueName, LPVOID pStruct, DWORD cbStruct);
        STDMETHOD(ReadStruct)(THIS_ LPCTSTR pSection, LPCTSTR pValueName, LPVOID pStruct, DWORD cbStruct);
        STDMETHOD(Clear)(THIS);
};


#endif
