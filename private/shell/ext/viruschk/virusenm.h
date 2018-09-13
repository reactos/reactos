class CEnumVirusProviders : IEnumVirusProviders
{
   private:
      UINT uNext;
      UINT uNumItems;
      IUnknown *pIUnk;

      CLSID **ppclsid;

   public:
      CEnumVirusProviders(IUnknown *punk, UINT uElem, CLSID *);
      ~CEnumVirusProviders();

      /* IEnumVirusProviders methods */
      STDMETHODIMP Next(DWORD celt, CLSID *pelt, DWORD *pdw);
      STDMETHODIMP Skip(DWORD celt);
      STDMETHODIMP Reset() PURE;
      STDMETHODIMP Clone(IEnumVirusProviders **ppenum);
}