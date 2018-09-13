//  File:       secmgr.h
//
//  Contents:   The object that implements the base IInternetSecurityManager interface
//
//  Classes:    CSecurityManager
//
//  Functions:
//
//  History: 
//
//----------------------------------------------------------------------------

#ifndef _SECMGR_H_
#define _SECMGR_H_

#pragma warning(disable:4200)

#define MAX_SEC_MGR_CACHE   4
#define URLZONE_INVALID     URLZONE_USER_MAX+1

#define MUTZ_NOCACHE    0x80000000  // start private flags from high end

struct ZONEMAP_COMPONENTS;

struct RANGE_ITEM
{
    BYTE  bLow[4];    // high byte values for range
    BYTE  bHigh[4];   // low byte values for range
    TCHAR szName[1];  // actually variable length
};

class CSecurityManager : public IInternetSecurityManager
{
public:
    // IUnknown methods
    STDMETHODIMP QueryInterface(REFIID iid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    STDMETHODIMP MapUrlToZone( 
        /* [in] */ LPCWSTR pwszUrl,
        /* [out] */ DWORD *pdwZone,
        /* [in] */ DWORD dwReserved);

    STDMETHODIMP GetSecurityId( 
        /* [in] */ LPCWSTR pwszUrl,
        /* [size_is][out] */ BYTE* pbSecurityId,
        /* [out][in] */ DWORD *pcbSecurityId,
        /* [in] */ DWORD_PTR dwReserved);
        
    STDMETHODIMP ProcessUrlAction( 
        /* [in] */ LPCWSTR pwszUrl,
        /* [in] */ DWORD dwAction,
        /* [size_is][out] */ BYTE *pPolicy,
        /* [in] */ DWORD cbPolicy,
        /* [in] */ BYTE *pContext,
        /* [in] */ DWORD cbContext,
        /* [in] */ DWORD dwFlags,
        /* [in] */ DWORD dwReserved);
        
    STDMETHODIMP QueryCustomPolicy(
        /* [in] */ LPCWSTR     pwszUrl,
        /* [in] */ REFGUID     guidKey,
        /* [size_is][size_is][out] */ BYTE **ppPolicy,
        /* [out] */ DWORD *pcbPolicy,
        /* [in] */ BYTE *pContext,
        /* [in] */ DWORD cbContext,
        /* [in] */ DWORD dwReserved
    );

    STDMETHODIMP SetSecuritySite(
        /* [in] */  IInternetSecurityMgrSite *pSite
    );

    STDMETHODIMP GetSecuritySite(
        /* [out] */  IInternetSecurityMgrSite **ppSite
    );

    STDMETHODIMP SetZoneMapping( 
        /* [in] */ DWORD dwZone,
        /* [in] */ LPCWSTR lpszPattern,
        /* [in] */ DWORD dwFlags);
    
    STDMETHODIMP GetZoneMappings( 
        /* [in] */ DWORD dwZone,
        /* [out] */ IEnumString **ppEnumString,
        /* [in] */ DWORD dwFlags);

// Constructors/destructors
public:
    CSecurityManager(IUnknown *pUnkOuter, IUnknown **ppUnkInner);
    virtual ~CSecurityManager();

    static BOOL GlobalInit( ) ;
            
    static BOOL GlobalCleanup( );


// Aggregation and RefCount support.
protected:
    CRefCount m_ref;
        
    class CPrivUnknown : public IUnknown
    {
    public:
        STDMETHOD(QueryInterface) ( REFIID riid, LPVOID FAR* ppvObj);
        STDMETHOD_(ULONG,AddRef) (void);
        STDMETHOD_(ULONG,Release) (void);

        ~CPrivUnknown() {}
        CPrivUnknown() : m_ref () {}

    private:
        CRefCount   m_ref;          // the total refcount of this object
    };

    friend class CPrivUnknown;
    CPrivUnknown m_Unknown;

    IUnknown*   m_pUnkOuter;

    STDMETHODIMP_(ULONG) PrivAddRef()
    {
        return m_Unknown.AddRef();
    }
    STDMETHODIMP_(ULONG) PrivRelease()
    {
        return m_Unknown.Release();
    }

protected:

    BOOL EnsureZoneManager();
    VOID PickZCString(ZONEMAP_COMPONENTS *pzc, LPCWSTR *ppwsz, DWORD *pcch, LPCWSTR pwszDocDomain);

    // Helper methods to deal with IP Rules
    HRESULT ReadAllIPRules( );
    HRESULT AddDeleteIPRule(ZONEMAP_COMPONENTS *pzc, DWORD dwZone, DWORD dwFlags);

    // helper methods to do GetZoneMappings.
    HRESULT AddUrlsToEnum(CRegKey *pRegKey, DWORD dwZone, LPCTSTR lpsz, int cch, BOOL bAddWildCard, CEnumString *);
    HRESULT AddIPRulesToEnum(DWORD dwZone, CEnumString *);

    static HRESULT ComposeUrlSansProtocol(LPCTSTR pszDomain, int cchDomain, LPCTSTR pszSite, int cchSite,
                                        LPTSTR * ppszRet, int * cchRet);
    static HRESULT ComposeUrl(LPCTSTR pszUrlSansProt, int cchUrlSansProt, LPCTSTR pszProt, int cchProt, BOOL bAddWildCard,
                                LPTSTR * ppszRet, int * cchRet);

protected:  // UI related definitions.

    enum    { MAX_ALERT_SIZE = 256 };
    // Return values from DialogProc's
    enum  {  ZALERT_NO = 0 /* should be 0*/ , ZALERT_YES, ZALERT_YESPERSIST };

    // This structure is used to exchange data between the security manager 
    // and the dialog proc's.
    struct DlgData 
    {
        DWORD dwAction;
        DWORD dwZone;
        LPWSTR pstr;
        DWORD dwFlags;
    };

    typedef DlgData * LPDLGDATA;
                            
    // helper methods to display generic UI.
    static DWORD GetAlertIdForAction(DWORD dwAction);
    static DWORD GetWarnIdForAction(DWORD dwAction);

    // Dialog proc's etc.
    static INT_PTR ZonesAlertDialogProc(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam); 
    static INT_PTR ZonesWarnDialogProc(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam); 

    static inline BOOL IsFormsSubmitAction(DWORD dwAction);

    static INT_PTR FormsAlertDialogProc(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam);
                
    INT CSecurityManager::ShowFormsAlertDialog(HWND hwndParent, LPDLGDATA lpDlgData);

protected: 
    // Methods to help Map a URL to a zone. 

    HRESULT MapUrlToZone
        (ZONEMAP_COMPONENTS* pzc, DWORD* pdwZone, DWORD dwFlags, BOOL *pfMarked = NULL, LPWSTR *ppszURLMark = NULL);

    HRESULT CheckAddressAgainstRanges
        (ZONEMAP_COMPONENTS* pzc, DWORD* pdwZone, LPCTSTR pszProt);

    HRESULT CheckSiteAndDomainMappings
        (ZONEMAP_COMPONENTS* pzc, DWORD* pdwZone, LPCTSTR pszProt);

    HRESULT CheckUNCAsIntranet
        (ZONEMAP_COMPONENTS* pzc, DWORD* pdwZone, LPCTSTR pszProt);

    HRESULT CheckIntranetName
        (ZONEMAP_COMPONENTS* pzc, DWORD* pdwZone, LPCTSTR pszProt);

    HRESULT CheckProxyBypassRule
        (ZONEMAP_COMPONENTS* pzc, DWORD* pdwZone, LPCTSTR pszProt);

    HRESULT CheckMKURL
        (ZONEMAP_COMPONENTS* pzc, DWORD* pdwZone, LPCTSTR pszProt);

protected:
    // Class to remember persistent actions.
    class CPersistAnswers 
    {
    public:
            CPersistAnswers( ) : m_pAnswerEntry(NULL) { };
            ~CPersistAnswers( );

    public:
       BOOL GetPrevAnswer(LPCWSTR pszUrl, DWORD dwAction, INT* piAnswer);
        VOID RememberAnswer(LPCWSTR pszUrl, DWORD dwAction, INT iAnswer);
        static inline BOOL IsPersistentAnswerAction(DWORD dwAction);

    private:                            
        struct CAnswerEntry 
        {
            // Construction
            CAnswerEntry(LPCWSTR pszUrl, DWORD dwAction, INT iAnswer);
            ~CAnswerEntry( );

            // Methods.
            BOOL MatchEntry (LPCWSTR pszUrl, DWORD dwAction);
            INT GetAnswer( ) const { return m_iAnswer; }
            LPCWSTR GetUrl( ) const { return m_pszUrl; }
            CAnswerEntry * GetNext( ) const { return m_pNext; }
            VOID SetNext(CAnswerEntry * pNext) { m_pNext = pNext; }

            private:
            CAnswerEntry * m_pNext;
            LPWSTR m_pszUrl;
            DWORD  m_dwAction;
            INT    m_iAnswer;
        };

        CAnswerEntry* m_pAnswerEntry;
    };

    CPersistAnswers m_persistAnswers;

// Methods/members to support caching so we optimize MapUrlToZone, etc.
protected:

    class CSecMgrCache {
    public:
        CSecMgrCache(void);
        ~CSecMgrCache(void);

        BOOL Lookup(LPCWSTR pwszURL,
                    DWORD *pdwZone = NULL,
                    BOOL *pfMarked = NULL,
                    BYTE *pbSecurityID = NULL,
                    DWORD *pcbSecurityID = NULL, 
                    LPCWSTR pwszDocDomain = NULL);
        void Add(LPCWSTR pwszURL,
                 DWORD dwZone,
                 BOOL fMarked,
                 const BYTE *pbSecurityID = NULL,
                 DWORD cbSecurityID = NULL,
                 LPCWSTR pwszDocDomain = NULL );
        void Flush(void);

        static VOID IncrementGlobalCounter( );

    protected:

        // Counters to flag cross-process cache invalidation.
        DWORD         m_dwPrevCounter ; // Global counter so we can correctly invalidate the cache if 
                                        // user changes options.
        static HANDLE s_hMutexCounter;  // mutex controlling access to shared memory counter 
 
        BOOL IsCounterEqual() const;
        VOID SetToCurrentCounter();

        // The body of the cache is this array of cache entries.
        // Cross-thread access control for the array is by critical section.

        CRITICAL_SECTION m_csectZoneCache; // assumes only one, static instance of the cache 


        struct CSecMgrCacheEntry {
            CSecMgrCacheEntry(void) :
                m_pwszURL(NULL),
                m_pbSecurityID(NULL),
                m_cbSecurityID(NULL),
                m_dwZone(URLZONE_INVALID),
                m_fMarked(FALSE),
                m_pwszDocDomain(NULL) {};
            ~CSecMgrCacheEntry(void) { Flush(); };

            void Set(LPCWSTR pwszURL, DWORD dwZone, BOOL fMarked, 
                    const BYTE *pbSecurityID, DWORD cbSecurityID, LPCWSTR pwszDocDomain);
            void Flush(void);
            
            LPWSTR  m_pwszURL;
            BYTE*   m_pbSecurityID;
            DWORD   m_cbSecurityID;
            DWORD   m_dwZone;
            BOOL    m_fMarked;
            LPWSTR  m_pwszDocDomain;
        }; // CSecMgrCacheEntry

        CSecMgrCacheEntry   m_asmce[MAX_SEC_MGR_CACHE];
        int                 m_iAdd;         // index in m_asmce to add the next element

        BOOL FindCacheEntry( LPCWSTR pwszURL, int& riEntry ); // must be called under critical section.

    }; // CSecMgrCache

    static CSecMgrCache s_smcache;

protected:
    // Methods to manage List of Allowed ActiveX controls

    static BOOL EnsureListReady(BOOL bForce = FALSE);
    static void IntializeAllowedControls();
    static HRESULT GetControlPermissions(BYTE * raw_CLSID, DWORD & dwPerm);

    // Get the final decision on the whether to run a CLSID (for this zone, etc)
    static HRESULT GetActiveXRunPermissions(BYTE * raw_CLSID, DWORD & dwPerm);

public:
    static VOID IncrementGlobalCounter( );

private:
    IInternetSecurityMgrSite*   m_pSite;
    IInternetZoneManager* m_pZoneManager;
    IInternetSecurityManager* m_pDelegateSecMgr;

    CRegKey m_regZoneMap;


    // Static members to do remember the correct IP Ranges.
    static BOOL   s_bIPInit;     // have we read the IP ranges. 
    static BYTE*  s_pRanges;     // array of range items
    static DWORD  s_cNumRanges;  // number of range items
    static DWORD  s_cbRangeItem; // size of each range item
    static DWORD  s_dwNextRangeIndex; // Next index to use to add numbers in the range entry.

    static CRITICAL_SECTION s_csectIP; // crit section to protect it all. 
    static BOOL s_bcsectInit;
    static CLSID * s_clsidAllowedList;
    static CRITICAL_SECTION s_csectAList;
    static DWORD s_dwNumAllowedControls;
};

#pragma warning(default:4200)

#endif // _SECMGR_H_
