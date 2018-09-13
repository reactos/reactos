// {68E721D2-CD58-11D0-BD3D-00AA00BC205C}
DEFINE_GUID(CLSID_VirusProvider, 0x68E721D2, 0xCD58, 0x11D0, 0xBD, 0x3D, 0x00, 0xAA, 0x00, 0xBC, 0x20, 0x5C);


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
      STDMETHODIMP DisplayCustomInfo();

      
      STDMETHODIMP GetScannerUrl(LPWSTR *pwszUrl) { return E_NOTIMPL; };
};


      
