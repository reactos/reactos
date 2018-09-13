#ifndef __INSTAPP_H_
#define __INSTAPP_H_


/////////////////////////////////////////////////////////////////////////////
// CInstalledApp
class CInstalledApp : public IInstalledApp
{
public:
    // Constructor for Legacy Apps
    CInstalledApp(HKEY hkeySub, LPCTSTR pszKeyName, LPCTSTR pszProducts, LPCTSTR pszUninstall, BOOL bHKCU);

    // Constructor for Darwin Apps
    CInstalledApp(LPTSTR pszProductID);

    ~CInstalledApp(void);
    
    // *** IUnknown Methods
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);
    virtual STDMETHODIMP_(ULONG) AddRef(void) ;
    virtual STDMETHODIMP_(ULONG) Release(void);

    // *** IShellApp
    STDMETHODIMP GetAppInfo(PAPPINFODATA pai);
    STDMETHODIMP GetPossibleActions(DWORD * pdwActions);
    STDMETHODIMP GetSlowAppInfo(PSLOWAPPINFO psai);
    STDMETHODIMP GetCachedSlowAppInfo(PSLOWAPPINFO psai);
    STDMETHODIMP IsInstalled(void);
    
    // *** IInstalledApp
    STDMETHODIMP Uninstall(HWND hwndParent);
    STDMETHODIMP Modify(HWND hwndParent);
    STDMETHODIMP Repair(BOOL bReinstall);
    STDMETHODIMP Upgrade(void);
    
protected:

    LONG _cRef;
#define IA_LEGACY     1
#define IA_DARWIN     2
#define IA_SMS        4

    DWORD _dwSource;            // App install source (IA_*)  
    DWORD _dwAction;            // APPACTION_*

    // products name
    TCHAR _szProduct[MAX_PATH];

    // action strings 
    TCHAR _szModifyPath[MAX_INFO_STRING];
    TCHAR _szUninstall[MAX_INFO_STRING];

    // info strings
    TCHAR _szInstallLocation[MAX_PATH];

    // for Darwin apps only
    TCHAR _szProductID[GUIDSTR_MAX];
    LPTSTR _pszUpdateUrl;
    
    // for Legacy apps only 
    TCHAR _szKeyName[MAX_PATH];
    TCHAR _szCleanedKeyName[MAX_PATH];
    
    // app size
    BOOL _bTriedToFindFolder;        // TRUE: we have attempted to find the 
                                     //   install folder already
    BOOL _bHKCU;                     // Legacy app from HKCU

    // GUID identifying this InstalledApp
    GUID _guid;

#define PERSISTSLOWINFO_IMAGE 0x00000001
    // Structure used to persist SLOWAPPINFO
    typedef struct _PersistSlowInfo
    {
        DWORD dwSize;
        DWORD dwMasks;
        ULONGLONG ullSize;
        FILETIME  ftLastUsed;
        int       iTimesUsed;
        WCHAR     szImage[MAX_PATH];
    } PERSISTSLOWINFO;

    HKEY _MyHkeyRoot() { return _bHKCU ? HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE; };
    BOOL _LegacyUninstall(HWND hwndParent);
    BOOL _LegacyModify(HWND hwndParent);
    LONG _DarRepair(BOOL bReinstall);

    HKEY    _OpenRelatedRegKey(HKEY hkey, LPCTSTR pszRegLoc, REGSAM samDesired, BOOL bCreate);
    HKEY    _OpenUninstallRegKey(REGSAM samDesired);
    void    _GetUpdateUrl();
    void    _GetInstallLocationFromRegistry(HKEY hkeySub);
    LPWSTR  _GetLegacyInfoString(HKEY hkeySub, LPTSTR pszInfoName);
    BOOL    _GetDarwinAppSize(ULONGLONG * pullTotal);
    
    DWORD   _QueryActionBlockInfo(HKEY hkey);
    DWORD   _QueryBlockedActions(HKEY hkey);
            
#ifndef DOWNLEVEL_PLATFORM
    BOOL    _FindAppFolderFromStrings();
#endif //DOWNLEVEL_PLATFORM
    HRESULT _DarwinGetAppInfo(DWORD dwInfoFlags, PAPPINFODATA pai);
    HRESULT _LegacyGetAppInfo(DWORD dwInfoFlags, PAPPINFODATA pai);
    HRESULT _PersistSlowAppInfo(PSLOWAPPINFO psai);
    BOOL    _CreateAppModifyProcess(HWND hwndParent, LPTSTR pszExePath);

    HRESULT _SetSlowAppInfoChanged(HKEY hkeyCache, DWORD dwValue);
    HRESULT _IsSlowAppInfoChanged();
};

#endif //__INSTAPP_H_
