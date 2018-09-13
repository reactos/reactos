#include <windows.h>
#include <ole2.h>
#include <docobj.h>
#include <advpub.h>
#include <msident.h>
#include "factory.h"
#include "multiutl.h"
#include <ocidl.h>
#include <shlwapi.h>
#include <shlwapip.h>

#ifndef ASSERT
#ifdef DEBUG
#define ASSERT	Assert
#else
#define ASSERT(x)
#endif
#endif

#define IDENTITY_PASSWORDS
#define ARRAYSIZE(a) (sizeof(a) / sizeof(a[0]))

extern ULONG    g_cLock, g_cObj;
extern HANDLE   g_hMutex;
extern GUID     g_uidOldUserId;
extern GUID     g_uidNewUserId;
extern BOOL     g_fNotifyComplete;

inline ULONG DllLock()     { return ++g_cLock; }
inline ULONG DllUnlock()   { return --g_cLock; }
inline ULONG DllGetLock()  { return g_cLock; }

inline ULONG DllAddRef()   { return ++g_cObj; }
inline ULONG DllRelease()  { return --g_cObj; }
inline ULONG DllGetRef()   { return g_cObj; }

#define WM_IDENTITY_CHANGED                 (WM_USER+11401)
#define WM_QUERY_IDENTITY_CHANGE            (WM_USER+11402)
#define WM_IDENTITY_INFO_CHANGED            (WM_USER+11403)

#define CCH_USERPASSWORD_MAX_LENGTH         16
#define CCH_USERNAME_MAX_LENGTH             CCH_IDENTITY_NAME_MAX_LENGTH

//
// CUserIdentity object
//
class CUserIdentity : public IUserIdentity
{
protected:
    ULONG           m_cRef;
    GUID            m_uidCookie;
    BOOL            m_fSaved;
    BOOL            m_fUsePassword;
    TCHAR           m_szUsername[CCH_USERNAME_MAX_LENGTH];
    TCHAR           m_szPassword[CCH_USERPASSWORD_MAX_LENGTH];


public:
    CUserIdentity();
    ~CUserIdentity();

    // IUnknown members
    STDMETHODIMP         QueryInterface(REFIID, void **);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // IUserIdentity members
    STDMETHODIMP         GetCookie(GUID *puidCookie);
    STDMETHODIMP         OpenIdentityRegKey(DWORD dwDesiredAccess, HKEY *phKey);
    STDMETHODIMP         GetIdentityFolder(DWORD dwFlags, WCHAR *pszPath, ULONG ulBuffSize);
    STDMETHODIMP         GetName(WCHAR *pszName, ULONG ulBuffSize);

    // Other members
    STDMETHODIMP         SetName(WCHAR *pszName);
    STDMETHODIMP         SetPassword(WCHAR *pszPassword);
    STDMETHODIMP         InitFromUsername(TCHAR *pszUsername);
    STDMETHODIMP         InitFromCookie(GUID *uidCookie);
private:
    STDMETHODIMP         _SaveUser();
};

//
// CEnumUserIdentity object
//
class CEnumUserIdentity : public IEnumUserIdentity
{
protected:
    ULONG           m_cRef;
    DWORD           m_dwCurrentUser;     // Maintain current index into the reg list
    DWORD           m_cCountUsers;      // number of accounts in the registry
    GUID           *m_rguidUsers;
    BOOL            m_fInited;

public:
    CEnumUserIdentity();
    ~CEnumUserIdentity();

    // IUnknown members
    STDMETHODIMP         QueryInterface(REFIID, void **);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // IEnumUserIdentity members
    STDMETHODIMP         Next(ULONG celt, IUnknown **rgelt, ULONG *pceltFetched);
    STDMETHODIMP         Skip(ULONG celt);
    STDMETHODIMP         Reset(void);
    STDMETHODIMP         Clone(IEnumUserIdentity **ppenum);
    STDMETHODIMP         GetCount(ULONG *pnCount);

private:
    // Other methods
    STDMETHODIMP         _Init();
    STDMETHODIMP         _Init(DWORD dwCurrentUser, DWORD dwCountUsers, GUID *prguidUserCookies);
    STDMETHODIMP         _Cleanup();
};


//
// CUserIdentityManager object
//
class CUserIdentityManager : public IUserIdentityManager, public IConnectionPoint, public IPrivateIdentityManager
{
protected:
    ULONG               m_cRef;
    CRITICAL_SECTION    m_rCritSect;
    CNotifierList       *m_pAdviseRegistry;
	BOOL				m_fWndRegistered;
	HWND				m_hwnd;

public:
    CUserIdentityManager();
    ~CUserIdentityManager();

    // IUnknown members
    STDMETHODIMP         QueryInterface(REFIID, void **);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // IUserIdentityManager members
    STDMETHODIMP        EnumIdentities(IEnumUserIdentity **ppEnumUserIdentity);
    STDMETHODIMP        ManageIdentities(HWND hwndParent, DWORD dwFlags);
    STDMETHODIMP        Logon(HWND hwndParent, DWORD dwFlags, IUserIdentity **ppUserIdentity);
    STDMETHODIMP        Logoff(HWND hwndParent);
    STDMETHODIMP        GetIdentityByCookie(GUID *uidCookie, IUserIdentity **ppUserIdentity);

    // IConnectionPoint functions
    STDMETHODIMP        GetConnectionInterface(IID *pIID);        
    STDMETHODIMP        GetConnectionPointContainer(IConnectionPointContainer **ppCPC);
    STDMETHODIMP        Advise(IUnknown *pUnkSink, DWORD *pdwCookie);        
    STDMETHODIMP        Unadvise(DWORD dwCookie);        
    STDMETHODIMP        EnumConnections(IEnumConnections **ppEnum);

    // IPrivateIdentityManager functions
    STDMETHODIMP        CreateIdentity(WCHAR *pszName, IUserIdentity **ppIdentity);
    STDMETHODIMP        ConfirmPassword(GUID *uidCookie, WCHAR *pszPassword);

    // Other methods
    STDMETHODIMP        DestroyIdentity(GUID *uidCookie);
    STDMETHODIMP        QuerySwitchIdentities();
    STDMETHODIMP        NotifySwitchIdentities();

    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
    STDMETHODIMP        _NotifyIdentitiesSwitched();
    STDMETHODIMP        _QueryProcessesCanSwitch();
    STDMETHODIMP        _CreateWindowClass();
    STDMETHODIMP        _SwitchToUser(GUID *puidFromUser, GUID *puidToUser);
    STDMETHODIMP        _PersistChangingIdentities();
    STDMETHODIMP        _LoadChangingIdentities();
    STDMETHODIMP        _ClearChangingIdentities();
};
