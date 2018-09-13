class CVirusFactory : IClassFactory
{
   public:
      CVirusFactory() { cRef = 0; };

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

extern CVirusFactory *pcf;
