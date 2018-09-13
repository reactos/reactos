#ifndef _CDLINFO_INCLUDED
#define _CDLINFO_INCLUDED

class CCodeDownloadInfo : public ICodeDownloadInfo
{
    public:
        // IUnknown methods
        STDMETHODIMP QueryInterface(REFIID iid, void **ppvObj);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);
        
        // ICodeDownloadInfo methods
        STDMETHODIMP GetCodeBase(LPWSTR *szCodeBase);
        STDMETHODIMP SetCodeBase(LPCWSTR szCodeBase);

        STDMETHODIMP GetClassID(CLSID *clsid);
        STDMETHODIMP SetClassID(CLSID clsid);

        STDMETHODIMP SetMajorVersion(ULONG ulVersion);
        STDMETHODIMP GetMajorVersion(ULONG *pulVersion);
        
        STDMETHODIMP SetMinorVersion(ULONG ulVersion);
        STDMETHODIMP GetMinorVersion(ULONG *pulVersion);

    public:
        CCodeDownloadInfo();
        virtual ~CCodeDownloadInfo();

    private:
        LPWSTR                  _szCodeBase;
        DWORD                   _cRefs;
        ULONG                   _ulMajorVersion;
        ULONG                   _ulMinorVersion;
        CLSID                   _clsid;
};

#endif
