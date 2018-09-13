
//
// CDataObject class
//

class CDataObject :
    public IDataObject,
    public CComObjectRoot
{
    friend class CSnapIn;

DECLARE_NOT_AGGREGATABLE(CDataObject)
BEGIN_COM_MAP(CDataObject)
        COM_INTERFACE_ENTRY(IDataObject)
END_COM_MAP()

protected:

    //
    // Reference count
    //

    ULONG                  m_cRef;

public:
    //
    // Clipboard formats that are required by the console
    //

    static unsigned int    m_cfNodeType;
    static unsigned int    m_cfNodeTypeString;
    static unsigned int    m_cfDisplayName;
    static unsigned int    m_cfCoClass;
    static unsigned int    m_cfInternal;


public:
    CDataObject();
    ~CDataObject();


    //
    // IUnknown methods
    //

    STDMETHODIMP         QueryInterface(REFIID, LPVOID FAR *);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();


    //
    // Implemented IDataObject methods
    //

    STDMETHOD(GetDataHere)(LPFORMATETC lpFormatetc, LPSTGMEDIUM lpMedium);


    //
    // Unimplemented IDataObject methods
    //

    STDMETHOD(GetData)(LPFORMATETC lpFormatetcIn, LPSTGMEDIUM lpMedium)
    { return E_NOTIMPL; };

    STDMETHOD(EnumFormatEtc)(DWORD dwDirection, LPENUMFORMATETC* ppEnumFormatEtc)
    { return E_NOTIMPL; };

    STDMETHOD(QueryGetData)(LPFORMATETC lpFormatetc)
    { return E_NOTIMPL; };

    STDMETHOD(GetCanonicalFormatEtc)(LPFORMATETC lpFormatetcIn, LPFORMATETC lpFormatetcOut)
    { return E_NOTIMPL; };

    STDMETHOD(SetData)(LPFORMATETC lpFormatetc, LPSTGMEDIUM lpMedium, BOOL bRelease)
    { return E_NOTIMPL; };

    STDMETHOD(DAdvise)(LPFORMATETC lpFormatetc, DWORD advf,
                LPADVISESINK pAdvSink, LPDWORD pdwConnection)
    { return E_NOTIMPL; };

    STDMETHOD(DUnadvise)(DWORD dwConnection)
    { return E_NOTIMPL; };

    STDMETHOD(EnumDAdvise)(LPENUMSTATDATA* ppEnumAdvise)
    { return E_NOTIMPL; };

// Implementation
public:
    void SetType(DATA_OBJECT_TYPES type) { m_internal.m_type = type;  }
    void SetCookie(MMC_COOKIE cookie) { m_internal.m_cookie = cookie; }
    void SetClsid(const CLSID& clsid) { m_internal.m_clsid = clsid; }

private:
    HRESULT CreateNodeTypeData(LPSTGMEDIUM lpMedium);
    HRESULT CreateNodeTypeStringData(LPSTGMEDIUM lpMedium);
    HRESULT CreateDisplayName(LPSTGMEDIUM lpMedium);
    HRESULT CreateInternal(LPSTGMEDIUM lpMedium);
    HRESULT CreateCoClassID(LPSTGMEDIUM lpMedium);

    HRESULT Create(LPVOID pBuffer, INT len, LPSTGMEDIUM lpMedium);

    INTERNAL m_internal;
};
