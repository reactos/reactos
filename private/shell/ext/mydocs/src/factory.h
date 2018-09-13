#ifndef __factory_h
#define __factory_h


/*-----------------------------------------------------------------------------
/ CMyDocsClassFactory
/----------------------------------------------------------------------------*/

class CMyDocsClassFactory : public IClassFactory, CUnknown
{
    private:
    public:
        CMyDocsClassFactory();

        // IUnkown
        STDMETHODIMP_(ULONG) AddRef();
        STDMETHODIMP_(ULONG) Release();
        STDMETHODIMP         QueryInterface(REFIID riid, LPVOID* ppvObject);

        // IClassFactory
        STDMETHODIMP CreateInstance(IUnknown* pOuter, REFIID riid, LPVOID* ppvObject);
        STDMETHODIMP LockServer(BOOL fLock);
};


#endif
