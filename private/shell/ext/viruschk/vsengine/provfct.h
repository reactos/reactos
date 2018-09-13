class CProvider1Factory : IClassFactory
{
   public:
      CProvider1Factory() { cRef = 0; };

      //IUnknown things
      STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
      STDMETHODIMP_(ULONG) AddRef(void);
      STDMETHODIMP_(ULONG) Release(void);
     
      //IClassFactory Things
      STDMETHODIMP CreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppv);
      STDMETHODIMP LockServer(BOOL fLock);

   private:
      UINT cRef;
};

extern CProvider1Factory *pcf;
