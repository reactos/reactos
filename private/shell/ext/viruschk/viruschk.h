#ifndef _VIRUSCHK_H_
#define _VIRUSCHK_H_

class CVirusScanProvider
{
    public:
        CVirusScanProvider();
        ~CVirusScanProvider();

        CLSID  clsid;
        LPWSTR pwszDescription;
        DWORD  dwFlags;
        IVirusScanEngine *pvs;

        CVirusScanProvider *nextProv;
};

class CVirusCheck : public IVirusScanner, public IRegisterVirusScanEngine
{
   private:
      DWORD m_cObjRef;
      IUnknown *m_punkOuter;            // aggregation currently unsupported

      UINT m_uNumProviders;

      CVirusScanProvider *m_provList;   // Head of list of virus scan providers
 
   public:
      CVirusCheck(IUnknown *punkOut, IUnknown **punkRet);
      ~CVirusCheck();

      void LoadProviders();
      void FillDefaultVirusInfo(LPVIRUSINFO pvrsinfo, CVirusScanProvider *vsp, DWORD dwFlags);
      HRESULT DoVirusFoundDefaultUI(HWND hwnd, LPVIRUSINFO pvrsinfo, LPWSTR pwszDesc, DWORD dwFlags);
      DWORD GetScannerEngineFlags(DWORD dwFlags);
      
      //IUnknown members
      STDMETHODIMP QueryInterface(REFIID riid, LPVOID FAR* ppvObj);
      STDMETHODIMP_(ULONG) AddRef(void);
      STDMETHODIMP_(ULONG) Release(void);

      /* IVirusScanner methods */
      STDMETHODIMP ScanForVirus(HWND hwnd, STGMEDIUM *pstgmed, LPWSTR pwszItemDesc,
                                DWORD dwFlags, LPVIRUSINFO pVrsInfo);

      /* IRegisterVirusProvider methods */
      STDMETHODIMP RegisterScanEngine(REFCLSID rclsid, LPWSTR pwszDescription, DWORD dwFlags,
                            DWORD dwReserved, DWORD *lpdwCookie);
      STDMETHODIMP UnRegisterScanEngine(REFCLSID rclsid, LPWSTR pwszDescription, DWORD dwFlags,
                            DWORD dwReserved, DWORD dwCookie);
};


typedef struct _VirusDlgParam_
{
   LPWSTR pwszDesc;
   LPVIRUSINFO pvrsinfo;

} VIRUSDLGPARAM;

//-------------------------------------------------------------------------------
//
// Function prototype
//
//-------------------------------------------------------------------------------

void InitVirusFoundDlg(HWND hwnd, VIRUSDLGPARAM *pvrs, RECT rctDlg);
INT_PTR CALLBACK VirusFoundDlg(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LPWSTR ConvertIStreamToFile(LPSTREAM pIStream );
HRESULT IsScannerSigned(LPSTR pszCLSID);
HRESULT RemoveScanner( REFCLSID rclsid, LPWSTR pDesc, DWORD dwCookie );
HRESULT AddScanner(LPSTR pszCLSID, LPSTR psz, LPDWORD pdwCookie, BOOL bNewEntry);
BOOL CALLBACK RegisterVirusScannerDlg(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL DoCheckSum( LPSTR pszCLSID, ULONG *pulCRC );
void CatWideStr(LPWSTR pwszTarget, LPWSTR pwszSource);

#endif _VIRUSCHK_H_
