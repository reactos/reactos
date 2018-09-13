// {E489FDC0-BD6C-11CF-AAFA-00AA00B6015C}
DEFINE_GUID(CLSID_VirusScanner1, 0xE489FDC0L, 0xBD6C, 0x11CF, 0xAA, 0xFA, 0x00, 0xAA, 0x00, 0xB6, 0x01, 0x5C);


class CVirusProvider1 : public IVirusScanEngine
{
   private:
      DWORD cObjRef;
      IUnknown *punkOuter;

   public:
      CVirusProvider1(IUnknown *punkOut, IUnknown **punkRet);

      //IUnknown members
      STDMETHODIMP QueryInterface(REFIID riid, LPVOID FAR* ppvObj);
      STDMETHODIMP_(ULONG) AddRef(void);
      STDMETHODIMP_(ULONG) Release(void);


      /* IVirusScanEngine methods */
      STDMETHODIMP ScanForVirus(STGMEDIUM *pstgMedium, DWORD dwFlags, LPVIRUSINFO pvrsinfo);
      STDMETHODIMP GetScannerUrl(LPWSTR *pwszUrl) { return E_NOTIMPL; };
};

      
