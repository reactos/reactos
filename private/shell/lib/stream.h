//***   stream.h --
//
// NOTES
//  BUGBUG this inheritance is *backwards*! should have:
//      CRegStrPropBag : CRegStrFS, IPropertyBag
// fix this soon i hope...

class CRegStrPropBag : public IPropertyBag
{
public:
    //*** IUnknown
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppv);
    virtual STDMETHODIMP_(ULONG) AddRef(void);
    virtual STDMETHODIMP_(ULONG) Release(void);

    //*** IPropertyBag
    virtual STDMETHODIMP Read(/*[in]*/ LPCOLESTR pwzPropName,
        /*[inout]*/ VARIANT *pVar, /*[in]*/ IErrorLog *pErrorLog);
    // currently unused (see #if 0 in stream.cpp)
    virtual STDMETHODIMP Write(/*[in]*/ LPCOLESTR pwzPropName,
        /*[in]*/ VARIANT *pVar) { ASSERT(0); return E_NOTIMPL; };

    //*** THISCLASS
    virtual HRESULT Initialize(HKEY hkey, DWORD grfMode);
    HRESULT SetRoot(HKEY hkey, DWORD grfMode);
    HRESULT ChDir(LPCTSTR pszSubKey);
    // fast versions, no OLESTR nonsense
    HRESULT QueryValueStr(LPCTSTR pszName, LPTSTR pszValue, LPDWORD pcbValue);
    HRESULT SetValueStr(LPCTSTR pszName, LPCTSTR pszValue);
    HRESULT SetValueStrEx(LPCTSTR pszName, DWORD dwType, LPCTSTR pszValue);

protected:
    CRegStrPropBag() { _cRef = 1; };
    virtual ~CRegStrPropBag();

    friend CRegStrPropBag *CRegStrPropBag_Create(HKEY hkey, DWORD grfMode);

    int     _cRef;
    HKEY    _hkey;
    int     _grfMode;   // read/write (subset of STGM_* values)
};

HRESULT IPBag_ReadStr(IPropertyBag *pPBag, LPCOLESTR pwzPropName, LPTSTR buf, int cch);
#if 0 // currently unused (see #if 0 in stream.cpp)
HRESULT IPBag_WriteStr(IPropertyBag *pPBag, LPCOLESTR pwzPropName, LPTSTR buf);
#endif
HKEY Reg_CreateOpenKey(HKEY hkey, LPCTSTR pszSubKey, DWORD grfMode);
HKEY Reg_DupKey(HKEY hkey);

class CRegStrFS : public CRegStrPropBag
{
public:
    /*virtual HRESULT Initialize(HKEY hk, DWORD grfMode);*/
    HRESULT QueryValue(LPCTSTR pszName, BYTE *pbData, LPDWORD pcbData);
    HRESULT SetValue(LPCTSTR pszName, DWORD dwType, const BYTE *pbData, DWORD cbData);
    HRESULT DeleteValue(LPCTSTR pszName);
    HRESULT RmDir(LPCTSTR pszName, BOOL fRecurse);

    HKEY GetHkey()  { return _hkey; }

protected:
    friend CRegStrFS *CRegStrFS_Create(HKEY hk, DWORD grfMode);

#if 0
    HKEY _hkeyRoot;
#endif
};       
