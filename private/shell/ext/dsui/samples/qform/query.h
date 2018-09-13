#ifndef __simplecom_h
#define __simplecom_h


/*-----------------------------------------------------------------------------
/ CExampleQueryFormClassFactory
/----------------------------------------------------------------------------*/

class CExampleQueryFormClassFactory : public IClassFactory, CUnknown
{
    public:
        // IUnkown
        STDMETHODIMP_(ULONG) AddRef();
        STDMETHODIMP_(ULONG) Release();
        STDMETHODIMP QueryInterface(REFIID riid, LPVOID* ppvObject);

        // IClassFactory
        STDMETHODIMP CreateInstance(IUnknown* pUnkOuter, REFIID riid, LPVOID* ppvObject);
        STDMETHODIMP LockServer(BOOL fLock);
};


/*-----------------------------------------------------------------------------
/ CExampleQueryForm
/----------------------------------------------------------------------------*/

class CExampleQueryForm : public IQueryForm, CUnknown
{
    public:
        // IUnknown
        STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID* ppvObject);
        STDMETHOD_(ULONG, AddRef)(THIS);
        STDMETHOD_(ULONG, Release)(THIS);

        // IQueryForms
        STDMETHOD(Initialize)(THIS_ HKEY hkForm);
        STDMETHOD(AddForms)(THIS_ LPCQADDFORMSPROC pAddFormsProc, LPARAM lParam);
        STDMETHOD(AddPages)(THIS_ LPCQADDPAGESPROC pAddPagesProc, LPARAM lParam);
};


#endif
