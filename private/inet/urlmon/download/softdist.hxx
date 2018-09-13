#ifndef _SOFTDIST_H_
#define _SOFTDIST_H_

#ifdef __cplusplus

typedef struct tagDISTUNITINST {

    DWORD           dwInstalledVersionMS;
    DWORD           dwInstalledVersionLS;

} DISTUNITINST, *LPDISTUNITINST;


typedef struct tagDISTUNITADVT {

    DWORD           dwVersionMS;
    DWORD           dwVersionLS;

    LPCSTR          pszTitle;
    LPCSTR          pszAbstract;
    LPCSTR          pszHREF;

} DISTUNITADVT, *LPDISTUNITADVT;

/*
STDAPI
GetDistributionUnitAdvertisement(
    LPCWSTR szDistUnit,
    LPDISTUNITINST *ppdunitinst,
    LPDISTUNITADVT *ppdunitadvt,
    LPVOID pvReserved,                  // Must be NULL
    DWORD flags);
*/

//  SoftDist tag handler for CDF

#define CBH_FLAGS_DOWNLOADED                0x1
#define CBH_FLAGS_MAIN_CODEBASE             0x2

class CCodeBaseHold {
    public:
        CCodeBaseHold() {
            wszCodeBase = NULL;
            dwSize = 0;
            bHREF = FALSE;
            wszDLGroup = NULL;
            dwFlags = 0;
        }

        ~CCodeBaseHold() { 
            if (wszCodeBase) {
                delete wszCodeBase;
            }
            if (wszDLGroup) {
                delete wszDLGroup;
            }
        }
    
    LPWSTR  wszCodeBase;
    DWORD   dwSize;
    BOOL    bHREF;
    LPWSTR  wszDLGroup;
    DWORD   dwFlags;
};

// Helper prototypes
BOOL AreAllLanguagesPresent(LPCSTR lpszLang1, LPCSTR lpszLang2);
HRESULT GetStyleFromString(LPSTR szStyle, LPDWORD lpdwStyle);
HRESULT CheckLanguage(LCID localeID, LPTSTR szLanguages);

class CSoftDist : public ISoftDistExt {

    public:
    CSoftDist();
    ~CSoftDist();

    // IUnknown methods
    STDMETHODIMP            QueryInterface(REFIID riid,void ** ppv);
    STDMETHODIMP_(ULONG)    AddRef();
    STDMETHODIMP_(ULONG)    Release();

    // ISoftDistExt methods
    STDMETHODIMP ProcessSoftDist(LPCWSTR szCDFURL, IXMLElement *pSoftDistElement, LPSOFTDISTINFO);
    STDMETHODIMP GetFirstCodeBase(LPWSTR *szCodeBase, DWORD *dwMaxSize);
    STDMETHODIMP GetNextCodeBase(LPWSTR *szCodeBase, DWORD *dwMaxSize);
    STDMETHODIMP AsyncInstallDistributionUnit(IBindCtx *pbc, LPVOID pvReserved, DWORD flags, LPCODEBASEHOLD lpcbh);

    // helper methods for GetSoftwareUpdateInfo
    HRESULT GetSoftwareUpdateInfo( LPCWSTR szDistUnit, LPSOFTDISTINFO psdi );
    HRESULT SetSoftwareUpdateAdvertisementState( LPCWSTR szDistUnit,
                                                 DWORD dwAdState,
                                                 DWORD dwAdvertisedVersionMS,
                                                 DWORD dwAdvertisedVersionLS );
    // Other functions
    STDMETHODIMP Logo3Download(LPWSTR szCodeBase);
    STDMETHODIMP Logo3DownloadNext();
    STDMETHODIMP Logo3DownloadRedundant();

    private:

    HRESULT IsLocallyInstalled(LPCWSTR szDistUnit, DWORD dwVersionMS, DWORD dwVersionLS, LPCSTR szLanguages, DWORD style);
    HRESULT IsICDLocallyInstalled(LPCWSTR szDistUnit, DWORD dwVersionMS, DWORD dwVersionLS, LPCSTR szLanguages);
    HRESULT IsActSetupLocallyInstalled(LPCWSTR szDistUnit, DWORD dwVersionMS, DWORD dwVersionLS, LPCSTR szLanguages);
    HRESULT IsLogo3LocallyInstalled(LPCWSTR szDistUnit, DWORD dwVersionMS, DWORD dwVersionLS, LPCSTR szLanguages);

    HRESULT IsLogo3Advertised(LPBOOL lpfIsPrecached, LPBOOL lpfIsAuthorizedCDF);
    HRESULT IsICDAdvertised(LPBOOL lpfIsPrecached, LPBOOL lpfIsAuthorizedCDF);
    HRESULT IsActSetupAdvertised(LPBOOL lpfIsPrecached, LPBOOL lpfIsAuthorizedCDF);
    HRESULT IsAdvertised(LPBOOL lpfIsPrecached, LPBOOL lpfIsAuthorizedCDF);

    HRESULT Advertise(BOOL bFullAdvt);
    HRESULT ICDAdvertise(BOOL bFullAdvt);
    HRESULT ActSetupAdvertise(BOOL bFullAdvt);
    HRESULT Logo3Advertise(BOOL bFullAdvt);

    HRESULT CheckDependency(IXMLElement *pSoftDist);
    HRESULT CheckConfig(IXMLElement *pSoftDist);
    
    BOOL IsCDFNewerVersion(DWORD dwCurMS, DWORD dwCurLS);
    BOOL IsAuthorizedCDF(HKEY hkRootDU, BOOL bOptional=FALSE);

    BOOL IsAnyInstalled() {

        return (m_distunitinst.dwInstalledVersionMS | m_distunitinst.dwInstalledVersionLS);
    }

    LPCWSTR GetMainDistUnit() const { return m_szDistUnit;}
    LPCSTR GetCDF() const { return m_szCDFURL;}

    HRESULT PrepSoftwareUpdate( LPCWSTR szDistUnit, DWORD *pdwStyle );

#ifdef WX86
    CMultiArch *        GetMultiArch() { return &m_MultiArch; }
#endif

    DWORD               m_cRef;

    LPWSTR              m_szDistUnit;

    DWORD               m_dwVersionMS;
    DWORD               m_dwVersionLS;

    DWORD               m_dwVersionAdvertisedMS;
    DWORD               m_dwVersionAdvertisedLS;
    DWORD               m_dwAdState;

    DWORD               m_Style;

    LPSTR               m_szCDFURL;
    LPSTR               m_szTitle;
    LPSTR               m_szLanguages;

    LPSTR               m_szAbstract;
    LPSTR               m_szHREF;

    CList<CCodeBaseHold*,CCodeBaseHold*>  m_cbh;
    POSITION            m_curPos;

    ISoftDistExt        *m_sdMSInstall;         // Darwin interface

    DISTUNITINST        m_distunitinst;
    IBindStatusCallback *m_pClientBSC;
    LPWSTR              m_szBaseURL;
#ifdef WX86
    CMultiArch          m_MultiArch;
#endif

};

class CActiveSetupBinding : IBinding
{
    friend DWORD StartActiveSetup(void *dwArg);

public:
    CActiveSetupBinding(IBindCtx *pbc, IBindStatusCallback *pbsc, LPWSTR szCodeBase, 
                        LPWSTR szDistUnit, HRESULT *hr);
    ~CActiveSetupBinding();
    
    // IUnknown methods
    STDMETHODIMP            QueryInterface(REFIID riid,void ** ppv);
    STDMETHODIMP_(ULONG)    AddRef();
    STDMETHODIMP_(ULONG)    Release();

    // IBinding methods
    STDMETHOD(Abort)( void);
    STDMETHOD(Suspend)( void);
    STDMETHOD(Resume)( void);
    STDMETHOD(SetPriority)(LONG nPriority);
    STDMETHOD(GetPriority)(LONG *pnPriority);
    STDMETHOD(GetBindResult)(CLSID *pclsidProtocol, DWORD *pdwResult, LPWSTR *pszResult,DWORD *pdwReserved);

private:
    void StartActiveSetup(void);
    void DoCleanUp(DWORD dwExitCode);
    BSTR GetErrorMessage(HRESULT hr);
    HRESULT SaveHresult(HRESULT hr);
    HRESULT SetDefaultDownloadSite(LPSTR szSite); 

    IBindStatusCallback *m_pbsc;
    IBindCtx *m_pbc;
    LPSTR m_szCodeBase;
    CHAR m_szActSetupPath[MAX_PATH+1];
    LPSTR m_szDistUnit;

    HANDLE m_hWaitThread;
    PROCESS_INFORMATION m_piChild;

    DWORD dwThreadID;
    DWORD m_dwRef;
    BOOL fSilent;
};

#endif
#endif // _SOFTDIST_H_
