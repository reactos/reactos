//#define DBCS

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <regstr.h>
#define WIN31
#include "pwlapi.h"

#include <string.h>
#include <netlib.h>

#ifdef DEBUG
#define SAVE_DEBUG
#undef DEBUG
#endif

#include <npstring.h>
#include <npdefs.h>
#include <prsht.h>

#ifdef SAVE_DEBUG
#define DEBUG
#endif

#include <ole2.h>
#include "msluguid.h"

#include "msluapi.h"

void Netlib_EnterCriticalSection(void);
void Netlib_LeaveCriticalSection(void);
#ifdef DEBUG
extern BOOL g_fCritical;
#endif
#define ENTERCRITICAL   Netlib_EnterCriticalSection();
#define LEAVECRITICAL   Netlib_LeaveCriticalSection();
#define ASSERTCRITICAL  ASSERT(g_fCritical);

#define ARRAYSIZE(a)	(sizeof(a)/sizeof(a[0]))

extern "C" {
HRESULT VerifySupervisorPassword(LPCSTR pszPassword);
HRESULT ChangeSupervisorPassword(LPCSTR pszOldPassword, LPCSTR pszNewPassword);
};

const UINT cchMaxUsername = 128;

extern UINT g_cRefThisDll;
extern UINT g_cLocks;
extern void LockThisDLL(BOOL fLock);
extern void RefThisDLL(BOOL fRef);

extern void UnloadShellEntrypoint(void);
extern void CleanupWinINet(void);

UINT NPSCopyNLS (NLS_STR FAR *pnlsSourceString, LPVOID lpDestBuffer, LPDWORD lpBufferSize);

extern APIERR MakeSupervisor(HPWL hPWL, LPCSTR pszSupervisorPassword);

HRESULT GetSystemCurrentUser(NLS_STR *pnlsCurrentUser);
HRESULT GetUserPasswordCache(LPCSTR pszUsername, LPCSTR pszPassword, LPHANDLE phOut, BOOL fCreate);

typedef void (*PFNSELNOTIFY)(HWND hwndLB, int iSel);
HRESULT FillUserList(HWND hwndLB, IUserDatabase *pDB, LPCSTR pszDefaultSelection,
                     BOOL fIncludeGuest, PFNSELNOTIFY pfnSelNotify);
void DestroyUserList(HWND hwndLB);
void AddPage(LPPROPSHEETHEADER ppsh, UINT id, DLGPROC pfn, LPVOID pwd);
void InitWizDataPtr(HWND hDlg, LPARAM lParam);
void InsertControlText(HWND hDlg, UINT idCtrl, const NLS_STR *pnlsInsert);
HRESULT GetControlText(HWND hDlg, UINT idCtrl, NLS_STR *pnls);
HRESULT DoAddUserWizard(HWND hwndParent, IUserDatabase *pDB,
                        BOOL fPickUserPage, IUser *pUserToClone);
BOOL InstallLogonDialog(void);
void DeinstallLogonDialog(void);

HRESULT DoUserDialog(HWND hwndOwner, DWORD dwFlags, IUser **ppOut);
void CacheLogonCredentials(LPCSTR pszUsername, LPCSTR pszPassword);

void ReportUserError(HWND hwndOwner, HRESULT hres);
void SetErrorFocus(HWND hDlg, UINT idCtrl, BOOL fClear = TRUE);

class CLUClassFactory : public IClassFactory
{
public:
	STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppv);
	STDMETHODIMP_(ULONG) AddRef(void);
	STDMETHODIMP_(ULONG) Release(void);

	STDMETHODIMP CreateInstance( 
            /* [unique][in] */ IUnknown __RPC_FAR *pUnkOuter,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
	STDMETHODIMP LockServer( 
            /* [in] */ BOOL fLock);
};


HRESULT CreateUserDatabase(REFIID riid, void **ppOut);

class CLUUser;
class BUFFER;

class CLUDatabase : public IUserDatabase
{
//friend class CLUClassFactory;
friend HRESULT CreateUserDatabase(REFIID riid, void **ppOut);

private:
	ULONG m_cRef;
	CLUUser *m_CurrentUser;

	CLUDatabase();
	~CLUDatabase();

	STDMETHODIMP CreateUser(LPCSTR pszName, IUser *pCloneFrom,
	                        BOOL fFixInstallStubs, IUserProfileInit *pInit);

public:
	// *** IUnknown methods ***
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

	STDMETHODIMP Install(LPCSTR pszSupervisorName, LPCSTR pszSupervisorPassword,
						 LPCSTR pszRatingsPassword, IUserProfileInit *pInit);

	STDMETHODIMP AddUser(LPCSTR pszName, IUser *pSourceUser,
	                     IUserProfileInit *pInit, IUser **ppOut);
	STDMETHODIMP GetUser(LPCSTR pszName, IUser **ppOut);
	STDMETHODIMP GetSpecialUser(DWORD nSpecialUserCode, IUser **ppOut);
	STDMETHODIMP GetCurrentUser(IUser **ppOut);
	STDMETHODIMP SetCurrentUser(IUser *pUser);
	STDMETHODIMP DeleteUser(LPCSTR pszName);
	STDMETHODIMP RenameUser(LPCSTR pszOldName, LPCSTR pszNewName);
	STDMETHODIMP EnumUsers(IEnumUnknown **ppOut);

	STDMETHODIMP Authenticate(HWND hwndOwner, DWORD dwFlags,
							 LPCSTR pszName, LPCSTR pszPassword,
							 IUser **ppOut);

	STDMETHODIMP InstallComponent(REFCLSID clsidComponent, LPCSTR pszName,
								 DWORD dwFlags);
	STDMETHODIMP RemoveComponent(REFCLSID clsidComponent, LPCSTR pszName);
    STDMETHODIMP InstallWizard(HWND hwndParent);
    STDMETHODIMP AddUserWizard(HWND hwndParent);
    STDMETHODIMP UserCPL(HWND hwndParent);
};


class CLUUser : public IUser
{
friend class CLUDatabase;
friend class CLUEnum;

private:
	NLS_STR m_nlsUsername;
	NLS_STR m_nlsDir;
	NLS_STR m_nlsPassword;
	BOOL m_fAuthenticated;
    BOOL m_fTempSupervisor;
	HKEY m_hkeyDB;
	HKEY m_hkeyUser;
	BOOL m_fUserExists;
	BOOL m_fAppearsSupervisor;
    BOOL m_fLoadedProfile;
	ULONG m_cRef;

	CLUDatabase *m_pDB;

	CLUUser(CLUDatabase *m_pDB);
	~CLUUser();
	HRESULT Init(LPCSTR pszUsername);

	BOOL Exists() { return m_fUserExists; }
	HRESULT GetSupervisorPassword(BUFFER *pbufPCE);
    BOOL IsSystemCurrentUser(void);

public: 
	// *** IUnknown methods ***
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

	STDMETHODIMP GetName(LPSTR pbBuffer, LPDWORD pcbBuffer);
	STDMETHODIMP GetProfileDirectory(LPSTR pbBuffer, LPDWORD pcbBuffer);

	STDMETHODIMP IsSupervisor(void);
	STDMETHODIMP SetSupervisorPrivilege(BOOL fMakeSupervisor, LPCSTR pszSupervisorPassword);
	STDMETHODIMP MakeTempSupervisor(BOOL fMakeSupervisor, LPCSTR pszSupervisorPassword);
	STDMETHODIMP AppearsSupervisor(void);

	STDMETHODIMP Authenticate(LPCSTR pszPassword);
	STDMETHODIMP ChangePassword(LPCSTR pszOldPassword, LPCSTR pszNewPassword);
	STDMETHODIMP GetPasswordCache(LPCSTR pszPassword, LPHANDLE phOut);

    STDMETHODIMP LoadProfile(HKEY *phkeyUser);
    STDMETHODIMP UnloadProfile(HKEY hkeyUser);

	STDMETHODIMP GetComponentSettings(REFCLSID clsidComponent,
									  LPCSTR pszName, IUnknown **ppOut,
									  DWORD fdwAccess);
	STDMETHODIMP EnumerateComponentSettings(IEnumUnknown **ppOut,
										    DWORD fdwAccess);
};


class CLUEnum : public IEnumUnknown
{
friend class CLUDatabase;

private:
	ULONG m_cRef;
	HKEY m_hkeyDB;
	LPSTR *m_papszNames;
	UINT m_cNames;
	UINT m_cAlloc;
	UINT m_iCurrent;

	CLUDatabase *m_pDB;

	CLUEnum(CLUDatabase *m_pDB);
	~CLUEnum();
	HRESULT Init(void);
	void Cleanup(void);

public:
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

	STDMETHODIMP Next(ULONG celt, IUnknown __RPC_FAR *__RPC_FAR *rgelt,
					  ULONG __RPC_FAR *pceltFetched);
	STDMETHODIMP Skip(ULONG celt);
	STDMETHODIMP Reset(void);
	STDMETHODIMP Clone(IEnumUnknown __RPC_FAR *__RPC_FAR *ppenum);
};


#ifdef USER_SETTINGS_IMPLEMENTED

class CUserSettings : public IUserSettings
{
private:
	ULONG m_cRef;
	CLSID m_clsid;
	NLS_STR m_nlsName;
	HKEY m_hkey;

	CUserSettings();
	~CUserSettings();

public:
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

	STDMETHODIMP GetCLSID(CLSID *pclsidOut);
	STDMETHODIMP GetName(LPSTR pbBuffer, LPDWORD pcbBuffer);
	STDMETHODIMP GetDisplayName(LPSTR pbBuffer, LPDWORD pcbBuffer);

	STDMETHODIMP QueryKey(HKEY *phkeyOut);
};

#endif

class CWizData : public IUserProfileInit
{
public:
    HRESULT m_hresRatings;          /* result of VerifySupervisorPassword("") */
    BOOL m_fGoMultiWizard;          /* TRUE if this is the big go-multiuser wizard */
    NLS_STR m_nlsSupervisorPassword;
    NLS_STR m_nlsUsername;
    NLS_STR m_nlsUserPassword;
    IUserDatabase *m_pDB;
    IUser *m_pUserToClone;
    int m_idPrevPage;               /* ID of page before Finish */
    UINT m_cRef;
    DWORD m_fdwOriginalPerUserFolders;
    DWORD m_fdwNewPerUserFolders;
    DWORD m_fdwCloneFromDefault;
    BOOL m_fCreatingProfile;
    IUser *m_pNewUser;
    BOOL m_fChannelHack;

    CWizData();
    ~CWizData();

	// *** IUnknown methods ***
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    STDMETHODIMP PreInitProfile(HKEY hkeyUser, LPCSTR pszProfileDir);
    STDMETHODIMP PostInitProfile(HKEY hkeyUser, LPCSTR pszProfileDir);
};

void InitFolderCheckboxes(HWND hDlg, CWizData *pwd);
void FinishChooseFolders(HWND hDlg, CWizData *pwd);

BOOL ProfileUIRestricted(void);
void ReportRestrictionError(HWND hwndOwner);

typedef HRESULT (*PFNPROGRESS)(LPARAM lParam);
HRESULT CallWithinProgressDialog(HWND hwndOwner, UINT idResource, PFNPROGRESS pfn,
                                 LPARAM lParam);


HRESULT IsCurrentUserSupervisor(IUserDatabase *pDB);


/* NOTE: Keep the following flags in the same order as the corresponding
 * entries in the folder descriptor table in msluwiz.cpp.
 */
const DWORD FOLDER_DESKTOP   = 0x00000001;
const DWORD FOLDER_NETHOOD   = 0x00000002;
const DWORD FOLDER_RECENT    = 0x00000004;
const DWORD FOLDER_STARTMENU = 0x00000008;
const DWORD FOLDER_PROGRAMS  = 0x00000010;
const DWORD FOLDER_STARTUP   = 0x00000020;
const DWORD FOLDER_FAVORITES = 0x00000040;
const DWORD FOLDER_CACHE     = 0x00000080;
const DWORD FOLDER_MYDOCS    = 0x00000100;
