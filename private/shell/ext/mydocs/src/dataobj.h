#ifndef __dataobj_h
#define __dataobj_h


/*-----------------------------------------------------------------------------
/ Registered clipboard formats
/----------------------------------------------------------------------------*/

#define MDCF_SHELLIDLIST            0
#define MDCF_SHELLDESCRIPTORSA      1
#define MDCF_SHELLDESCRIPTORSW      2
#define MDCF_SHELLOFFSETS           3
#define MDCF_MAX                    3

extern UINT g_clipboardFormats[MDCF_MAX];

#define g_cfShellIDList          g_clipboardFormats[MDCF_SHELLIDLIST]
#define g_cfFileDescriptorsA     g_clipboardFormats[MDCF_SHELLDESCRIPTORSA]
#define g_cfFileDescriptorsW     g_clipboardFormats[MDCF_SHELLDESCRIPTORSW]
#define g_cfOffsets              g_clipboardFormats[MDCF_SHELLOFFSETS]

void RegisterMyDocsClipboardFormats(void);


/*-----------------------------------------------------------------------------
/ CMyDocsDataObject
/----------------------------------------------------------------------------*/

class CMyDocsDataObject : public IDataObject, CUnknown
{
    private:
        LPCITEMIDLIST m_pidlRoot;
        LPCITEMIDLIST m_pidlShellRoot;
        HDPA          m_hdpaSpecial;
        HDPA          m_hdpaShell;
        LPTSTR        m_rootPath;
        STGMEDIUM     m_OffsetsMedium;
        STGMEDIUM *   m_pOffsetsMedium;
        IDataObject * m_pdo;

    public:
        CMyDocsDataObject(IShellFolder * psf, LPCITEMIDLIST pidlRoot, LPCITEMIDLIST pidlReal, INT cidl, LPCITEMIDLIST* aidl);
        ~CMyDocsDataObject();

        // IUnknown
        STDMETHOD(QueryInterface)(REFIID riid, LPVOID* ppvObject);
        STDMETHOD_(ULONG, AddRef)();
        STDMETHOD_(ULONG, Release)();

        // IDataObject
        STDMETHODIMP GetData(FORMATETC *pformatetcIn, STGMEDIUM *pmedium);
        STDMETHODIMP GetDataHere(FORMATETC *pformatetc, STGMEDIUM *pmedium);
        STDMETHODIMP QueryGetData(FORMATETC *pformatetc);
        STDMETHODIMP GetCanonicalFormatEtc(FORMATETC *pformatectIn, FORMATETC *pformatetcOut);
        STDMETHODIMP SetData(FORMATETC *pformatetc, STGMEDIUM *pmedium, BOOL fRelease);
        STDMETHODIMP EnumFormatEtc(DWORD dwDirection, IEnumFORMATETC **ppenumFormatEtc);
        STDMETHODIMP DAdvise(FORMATETC *pformatetc, DWORD advf, IAdviseSink *pAdvSink, DWORD *pdwConnection);
        STDMETHODIMP DUnadvise(DWORD dwConnection);
        STDMETHODIMP EnumDAdvise(IEnumSTATDATA **ppenumAdvise);
};


/*-----------------------------------------------------------------------------
/ CMyDocsEnumFormatETC
/----------------------------------------------------------------------------*/

class CMyDocsEnumFormatETC : public IEnumFORMATETC, CUnknown
{
    private:
        INT m_index;
        IEnumFORMATETC * m_pefe;

    public:
        CMyDocsEnumFormatETC( IDataObject * pdo );
        ~CMyDocsEnumFormatETC();

        // IUnknown
        STDMETHOD(QueryInterface)(REFIID riid, LPVOID* ppvObject);
        STDMETHOD_(ULONG, AddRef)();
        STDMETHOD_(ULONG, Release)();

        // IEnumIDList
        STDMETHODIMP Next(ULONG celt, FORMATETC* rgelt, ULONG* pceltFetched);
        STDMETHODIMP Skip(ULONG celt);
        STDMETHODIMP Reset();
        STDMETHODIMP Clone(LPENUMFORMATETC* ppenum);
};


#endif
