//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// persist.h 
//
//   The definitions for cdf IPersistFile and IPersistFolder interfaces.  This
//   class is used as a base class by the CCdfView and CIconHandler.
//
//   History:
//
//       4/23/97  edwardp   Created.
//
////////////////////////////////////////////////////////////////////////////////

//
// Check for previous includes of this file.
//

#ifndef _PERSIST_H_

#define _PERSIST_H_

//
// Function prototypes.
//

HRESULT ClearGleamFlag(LPCTSTR pszURL, LPCTSTR pszPath);

HRESULT URLGetLocalFileName(LPCTSTR pszURL,
                            LPTSTR szLocalFile,
                            int cch,
                            FILETIME* pftLastMod);

HRESULT URLGetLastModTime(LPCTSTR pszURL, FILETIME* pftLastMod);

//
// Data types.
//

typedef enum _tagINITTYPE
{
    IT_UNKNOWN = 0,
    IT_FILE,
    IT_INI,
    IT_SHORTCUT
} INITTYPE;

//
// Parse flags.
//

#define PARSE_LOCAL                   0x00000001
#define PARSE_NET                     0x00000002
#define PARSE_REPARSE                 0x00000004
#define PARSE_REMOVEGLEAM             0x00000008

//
// Strings used by initialization helper functions.
//

#define TSTR_INI_FILE        TEXT(FILENAME_SEPARATOR_STR)##TEXT("desktop.ini")   // Must include leading \.
#define TSTR_INI_SECTION     TEXT("Channel")
#define TSTR_INI_URL         TEXT("CDFURL")
#define TSTR_INI_LOGO        TEXT("Logo")
#define TSTR_INI_WIDELOGO    TEXT("WideLogo")
#define TSTR_INI_ICON        TEXT("Icon")

//
// Function protoypes
//

#define WM_NAVIGATE         (WM_USER+1)
#define WM_NAVIGATE_PANE    (WM_USER+2)

LRESULT CALLBACK NavigateWndProc(HWND hwnd, UINT msg, WPARAM wParam,
                                 LPARAM lParam);


//
// Class definition for CPersist
//

class CPersist : public IPersistFile,
                 public IPersistFolder,
                 public IPersistMoniker,
                 public IOleObject
{
//
// Methods.
//

public:
    
    // Constructor and destructor.
    CPersist(void);
    CPersist(BOOL bCdfParsed);
    ~CPersist(void);

    //IUnknown - Pure virtual functions.
    // IUnknown
    virtual STDMETHODIMP         QueryInterface(REFIID, void **) PURE;
    virtual STDMETHODIMP_(ULONG) AddRef(void) PURE;
    virtual STDMETHODIMP_(ULONG) Release(void) PURE;

    //IPersist - Shared by IPersistFile and IPersistFolder.
    STDMETHODIMP GetClassID(LPCLSID lpClassID);

    //IPersistFile
    STDMETHODIMP IsDirty(void);
    STDMETHODIMP Load(LPCOLESTR pszFileName, DWORD dwMode);
    STDMETHODIMP Save(LPCOLESTR pszFileName, BOOL fRemember);
    STDMETHODIMP SaveCompleted(LPCOLESTR pszFileName);
    STDMETHODIMP GetCurFile(LPOLESTR* ppszFileName);

    // IPersistFolder
    virtual STDMETHODIMP Initialize(LPCITEMIDLIST pidl);

    // IPersistMoniker
    //STDMETHODIMP IsDirty(void);

    STDMETHODIMP Load(BOOL fFullyAvailable, IMoniker* pIMoniker,
                      IBindCtx* pIBindCtx, DWORD grfMode);

    STDMETHODIMP Save(IMoniker* pIMoniker, IBindCtx* pIBindCtx, BOOL fRemember);
    STDMETHODIMP SaveCompleted(IMoniker* pIMoniker, IBindCtx* pIBindCtx);
    STDMETHODIMP GetCurMoniker(IMoniker** ppIMoniker);

    // IOleObject.
    STDMETHODIMP SetClientSite(IOleClientSite *pClientSite);
    STDMETHODIMP GetClientSite(IOleClientSite **ppClientSite);
    STDMETHODIMP SetHostNames(LPCOLESTR szContainerApp,
                              LPCOLESTR szContainerObj);
    STDMETHODIMP Close(DWORD dwSaveOption);
    STDMETHODIMP SetMoniker(DWORD dwWhichMoniker, IMoniker *pmk);
    STDMETHODIMP GetMoniker(DWORD dwAssign, DWORD dwWhichMoniker,
                            IMoniker **ppmk);
    STDMETHODIMP InitFromData(IDataObject *pDataObject, BOOL fCreation,
                              DWORD dwReserved);
    STDMETHODIMP GetClipboardData(DWORD dwReserved,IDataObject **ppDataObject);
    STDMETHODIMP DoVerb(LONG iVerb, LPMSG lpmsg, IOleClientSite *pActiveSite,
                        LONG lindex,HWND hwndParent,LPCRECT lprcPosRect);
    STDMETHODIMP EnumVerbs(IEnumOLEVERB **ppEnumOleVerb);
    STDMETHODIMP Update(void);
    STDMETHODIMP IsUpToDate(void);
    STDMETHODIMP GetUserClassID(CLSID *pClsid);
    STDMETHODIMP GetUserType(DWORD dwFormOfType, LPOLESTR *pszUserType);
    STDMETHODIMP SetExtent(DWORD dwDrawAspect, SIZEL *psizel);
    STDMETHODIMP GetExtent(DWORD dwDrawAspect, SIZEL *psizel);
    STDMETHODIMP Advise(IAdviseSink *pAdvSink, DWORD *pdwConnection);
    STDMETHODIMP Unadvise(DWORD dwConnection);
    STDMETHODIMP EnumAdvise(IEnumSTATDATA **ppenumAdvise);
    STDMETHODIMP GetMiscStatus(DWORD dwAspect, DWORD *pdwStatus);
    STDMETHODIMP SetColorScheme(LOGPALETTE *pLogpal);

//protected:

    //Helper functions derived classes can call.
    HRESULT  ParseCdf(HWND hwndOwner, IXMLDocument** ppIXMLDocument, 
                      DWORD dwParseFlags);

    BSTR     ReadFromIni(LPCTSTR pszKey);

    BOOL     IsUnreadCdf(void);
    BOOL     IsRecentlyChangedURL(LPCTSTR pszURL);

private:

    // Internal helper functions.
    HRESULT  Parse(LPTSTR szURL, IXMLDocument** ppIXMLDocument);
    INITTYPE GetInitType(LPTSTR szPath);
    BOOL     ReadFromIni(LPCTSTR pszKey, LPTSTR szOut, int cch);
    HRESULT  InitializeFromURL(LPTSTR szURL, IXMLDocument** ppIXMLDocument,
                               DWORD dwParseFlags);

    HRESULT  OpenChannel(LPCWSTR pszSubscribedURL);
    HWND     CreateNavigationWorkerWindow(HWND hwndParent,
                                          IWebBrowser2* pIWebBrowser2);

    void QuickCheckInitType( void );
//
// Member variables.
//

protected:

    BOOL            m_bCdfParsed;
    TCHAR           m_szPath[INTERNET_MAX_URL_LENGTH];
    LPOLESTR        m_polestrURL;
    IWebBrowser2*   m_pIWebBrowser2;
    HWND            m_hwnd;
    IXMLDocument*   m_pIXMLDocument;
    BOOL            m_fPendingNavigation;
    INITTYPE        m_rgInitType;

#ifdef IMP_CLIENTSITE

    IOleClientSite* m_pOleClientSite;

#endif
};

#endif // _PERSIST_H_
