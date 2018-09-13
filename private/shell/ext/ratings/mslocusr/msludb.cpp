#include "mslocusr.h"
#include "msluglob.h"
#include "profiles.h"

#include <regentry.h>

#include <ole2.h>

CLUDatabase::CLUDatabase(void)
	: m_cRef(0),
	  m_CurrentUser(NULL)
{
    RefThisDLL(TRUE);
}


CLUDatabase::~CLUDatabase(void)
{
	if (m_CurrentUser != NULL) {
		m_CurrentUser->Release();
		m_CurrentUser = NULL;
	}
    RefThisDLL(FALSE);
}


STDMETHODIMP CLUDatabase::QueryInterface(REFIID riid, LPVOID * ppvObj)
{
	if (!IsEqualIID(riid, IID_IUnknown) &&
		!IsEqualIID(riid, IID_IUserDatabase)) {
        *ppvObj = NULL;
		return ResultFromScode(E_NOINTERFACE);
	}

	*ppvObj = this;
	AddRef();
	return NOERROR;
}


STDMETHODIMP_(ULONG) CLUDatabase::AddRef(void)
{
	return ++m_cRef;
}


STDMETHODIMP_(ULONG) CLUDatabase::Release(void)
{
	ULONG cRef;

	cRef = --m_cRef;

	if (0L == m_cRef) {
		delete this;
	}
	/* Handle circular refcount because of cached current user object. */
	else if (1L == m_cRef && m_CurrentUser != NULL) {
		IUser *pCurrentUser = m_CurrentUser;
		m_CurrentUser = NULL;
		pCurrentUser->Release();
	}

	return cRef;
}


STDMETHODIMP CLUDatabase::Install(LPCSTR pszSupervisorName,
								  LPCSTR pszSupervisorPassword,
								  LPCSTR pszRatingsPassword,
								  IUserProfileInit *pInit)
{
	/* If the system already has a supervisor password, make sure the caller's
	 * password matches.  If there isn't already a password, the caller's
	 * (account) password is it.  We use the account password because the
	 * caller (the setup program) probably didn't pass us a ratings password
	 * in that case -- he also checks to see if there's an old ratings
	 * password and knows to prompt for one only if it's already there.
	 */
	HRESULT hres = ::VerifySupervisorPassword(pszRatingsPassword);
	
	if (FAILED(hres)) {
        if (pszRatingsPassword == NULL)
    		pszRatingsPassword = pszSupervisorPassword;
		::ChangeSupervisorPassword(::szNULL, pszRatingsPassword);
	}
	else if (hres == S_FALSE)
		return E_ACCESSDENIED;


	/* User profiles and password caching have to be enabled for us to work.
	 * We also have to be able to open or create the supervisor's PWL using
	 * the given password.  Thus we validate the password at the same time.
	 */

	{
		RegEntry re(::szLogonKey, HKEY_LOCAL_MACHINE);
		if (re.GetError() != ERROR_SUCCESS)
			return HRESULT_FROM_WIN32(re.GetError());
		if (!re.GetNumber(::szUserProfiles))
			re.SetValue(::szUserProfiles, 1);
		if (re.GetError() != ERROR_SUCCESS)
			return HRESULT_FROM_WIN32(re.GetError());
	}

    /* Make copies of the username and password for passing to the PWL APIs.
     * They need to be in OEM (PWL is accessible from DOS), and must be upper
     * case since the Windows logon dialog uppercases all PWL passwords.
     */
    NLS_STR nlsPWLName(pszSupervisorName);
    NLS_STR nlsPWLPassword(pszSupervisorPassword);
    if (nlsPWLName.QueryError() != ERROR_SUCCESS)
        return HRESULT_FROM_WIN32(nlsPWLName.QueryError());
    if (nlsPWLPassword.QueryError() != ERROR_SUCCESS)
        return HRESULT_FROM_WIN32(nlsPWLPassword.QueryError());
    nlsPWLName.strupr();
    nlsPWLName.ToOEM();
    nlsPWLPassword.strupr();
    nlsPWLPassword.ToOEM();

	HPWL hPWL = NULL;
	APIERR err = ::OpenPasswordCache(&hPWL, nlsPWLName.QueryPch(),
									 nlsPWLPassword.QueryPch(), TRUE);

	if (err != ERROR_SUCCESS) {
        if (err != IERR_IncorrectUsername)
    		err = ::CreatePasswordCache(&hPWL, nlsPWLName.QueryPch(), nlsPWLPassword.QueryPch());
		if (err != ERROR_SUCCESS)	
			return HRESULT_FROM_WIN32(err);
	}


	/* Now that the system has a supervisor password, call a worker function
	 * to clone the supervisor account from the default profile.  The worker
	 * function assumes that the caller has validated that the current user is
	 * a supervisor.
	 */

	err = ::MakeSupervisor(hPWL, pszRatingsPassword);
	::ClosePasswordCache(hPWL, TRUE);
	if (err != ERROR_SUCCESS)
		return HRESULT_FROM_WIN32(err);

	IUser *pSupervisor = NULL;
    hres = GetUser(pszSupervisorName, &pSupervisor);
	if (FAILED(hres)) {
		hres = CreateUser(pszSupervisorName, NULL, TRUE, pInit);
		if (pSupervisor != NULL) {
			pSupervisor->Release();
			pSupervisor = NULL;
		}
		if (SUCCEEDED(hres))
			hres = GetUser(pszSupervisorName, &pSupervisor);	/* reinitialize with created profile */
	}

	if (pSupervisor != NULL) {
		if (SUCCEEDED(hres))
			hres = pSupervisor->Authenticate(pszSupervisorPassword);
		if (SUCCEEDED(hres))
			hres = SetCurrentUser(pSupervisor);
        if (SUCCEEDED(hres))
            pSupervisor->SetSupervisorPrivilege(TRUE, pszRatingsPassword);  /* set appears-supervisor flag */

		pSupervisor->Release();
		pSupervisor = NULL;
	}

	return hres;
}


/* Some install stubs are "clone-user" install stubs, that get re-run if a
 * profile is cloned to become a new user's profile.  For example, if you
 * clone Fred to make Barney, Outlook Express doesn't want Barney to inherit
 * Fred's mailbox.
 *
 * When you run the go-multiuser wizard, we assume that the first user being
 * created is the one who's been using the machine all along, so that one
 * copy should be exempt from this.  So we go through all the install stub
 * keys for the newly created profile and, for any that are marked with a
 * username (even a blank one indicates that it's a clone-user install stub),
 * we mark it with the new username so it won't get re-run.
 */
void FixInstallStubs(LPCSTR pszName, HKEY hkeyProfile)
{
    HKEY hkeyList;
    LONG err = RegOpenKeyEx(hkeyProfile, "Software\\Microsoft\\Active Setup\\Installed Components", 0,
                            KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE, &hkeyList);

    if (err == ERROR_SUCCESS) {
        DWORD cbKeyName, iKey;
        TCHAR szKeyName[80];

        /* Enumerate components that are installed for the profile. */
        for (iKey = 0; ; iKey++)
        {
            LONG lEnum;

            cbKeyName = ARRAYSIZE(szKeyName);

            if ((lEnum = RegEnumKey(hkeyList, iKey, szKeyName, cbKeyName)) == ERROR_MORE_DATA)
            {
                // ERROR_MORE_DATA means the value name or data was too large
                // skip to the next item
                continue;
            }
            else if( lEnum != ERROR_SUCCESS )
            {
                // could be ERROR_NO_MORE_ENTRIES, or some kind of failure
                // we can't recover from any other registry problem, anyway
                break;
            }

            HKEY hkeyComponent;
            if (RegOpenKeyEx(hkeyList, szKeyName, 0,
                             KEY_QUERY_VALUE | KEY_SET_VALUE, &hkeyComponent) == ERROR_SUCCESS) {
                cbKeyName = sizeof(szKeyName);
                err = RegQueryValueEx(hkeyComponent, "Username", NULL, NULL,
                                      (LPBYTE)szKeyName, &cbKeyName);
                if (err == ERROR_SUCCESS || err == ERROR_MORE_DATA) {
                    RegSetValueEx(hkeyComponent, "Username",
                                  0, REG_SZ,
                                  (LPBYTE)pszName,
                                  lstrlen(pszName)+1);
                }
                RegCloseKey(hkeyComponent);
            }
        }
        RegCloseKey(hkeyList);
    }
}


STDMETHODIMP CLUDatabase::CreateUser(LPCSTR pszName, IUser *pCloneFrom,
                                     BOOL fFixInstallStubs, IUserProfileInit *pInit)
{
    if (::strlenf(pszName) > cchMaxUsername)
        return HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);

	RegEntry reRoot(::szProfileList, HKEY_LOCAL_MACHINE);
	if (reRoot.GetError() != ERROR_SUCCESS)
		return HRESULT_FROM_WIN32(reRoot.GetError());

	/* See if the user's subkey exists.  If it doesn't, create it. */
	reRoot.MoveToSubKey(pszName);
	if (reRoot.GetError() != ERROR_SUCCESS) {
		RegEntry reUser(pszName, reRoot.GetKey());
		if (reUser.GetError() != ERROR_SUCCESS)
			return HRESULT_FROM_WIN32(reUser.GetError());

		reRoot.MoveToSubKey(pszName);
		if (reRoot.GetError() != ERROR_SUCCESS)
			return HRESULT_FROM_WIN32(reRoot.GetError());
	}

	NLS_STR nlsProfilePath(MAX_PATH);
	if (nlsProfilePath.QueryError() != ERROR_SUCCESS)
		return E_OUTOFMEMORY;

	reRoot.GetValue(::szProfileImagePath, &nlsProfilePath);

	/* If the profile path is already recorded for the user, see if the
	 * profile itself exists.  If it does, then CreateUser is an error.
	 */
	BOOL fComputePath = FALSE;

	if (reRoot.GetError() == ERROR_SUCCESS) {
		if (!DirExists(nlsProfilePath.QueryPch())) {
			if (!::CreateDirectory(nlsProfilePath.QueryPch(), NULL)) {
				fComputePath = TRUE;
			}
		}
	}
	else {
		fComputePath = TRUE;
	}

	if (fComputePath) {
		ComputeLocalProfileName(pszName, &nlsProfilePath);
		reRoot.SetValue(::szProfileImagePath, nlsProfilePath.QueryPch());
	}

	AddBackslash(nlsProfilePath);
	nlsProfilePath.strcat(::szStdNormalProfile);
	if (FileExists(nlsProfilePath.QueryPch()))
		return HRESULT_FROM_WIN32(ERROR_USER_EXISTS);

	/* The user's profile directory now exists, and its path is recorded
	 * in the registry.  nlsProfilePath is now the full pathname for the
	 * user's profile file, which does not exist yet.
	 */

	NLS_STR nlsOtherProfilePath(MAX_PATH);
	if (nlsOtherProfilePath.QueryError() != ERROR_SUCCESS)
		return HRESULT_FROM_WIN32(nlsOtherProfilePath.QueryError());

	HRESULT hres;
	DWORD cbPath = nlsOtherProfilePath.QueryAllocSize();
	if (pCloneFrom == NULL ||
		FAILED(pCloneFrom->GetProfileDirectory(nlsOtherProfilePath.Party(), &cbPath)))
	{
		/* Cloning default profile. */

		hres = GiveUserDefaultProfile(nlsProfilePath.QueryPch());
        nlsOtherProfilePath.DonePartying();
        nlsOtherProfilePath = "";
	}
	else {
		/* Cloning other user's profile. */
        nlsOtherProfilePath.DonePartying();
		AddBackslash(nlsOtherProfilePath);
		nlsOtherProfilePath.strcat(::szStdNormalProfile);
		hres = CopyProfile(nlsOtherProfilePath.QueryPch(), nlsProfilePath.QueryPch());
	}

	if (FAILED(hres))
		return hres;

	/* Now the user has a profile.  Load it and perform directory
	 * reconciliation.
	 */

	LONG err = ::MyRegLoadKey(HKEY_USERS, pszName, nlsProfilePath.QueryPch());
	if (err == ERROR_SUCCESS) {
		HKEY hkeyNewProfile;
		err = ::RegOpenKey(HKEY_USERS, pszName, &hkeyNewProfile);
		if (err == ERROR_SUCCESS) {

            /* Build just the profile directory, no "user.dat" on the end. */
        	ISTR istrBackslash(nlsProfilePath);
	        if (nlsProfilePath.strrchr(&istrBackslash, '\\')) {
                ++istrBackslash;
		        nlsProfilePath.DelSubStr(istrBackslash);
            }

            if (pInit != NULL) {
                hres = pInit->PreInitProfile(hkeyNewProfile, nlsProfilePath.QueryPch());
                if (hres == E_NOTIMPL)
                    hres = S_OK;
            }
            else
                hres = S_OK;

            if (SUCCEEDED(hres)) {
    			err = ReconcileFiles(hkeyNewProfile, nlsProfilePath, nlsOtherProfilePath);	/* modifies nlsProfilePath */
                hres = HRESULT_FROM_WIN32(err);

                if (fFixInstallStubs) {
                    ::FixInstallStubs(pszName, hkeyNewProfile);
                }

                if (pInit != NULL) {
                    hres = pInit->PostInitProfile(hkeyNewProfile, nlsProfilePath.QueryPch());
                    if (hres == E_NOTIMPL)
                        hres = S_OK;
                }
            }
			::RegFlushKey(hkeyNewProfile);
			::RegCloseKey(hkeyNewProfile);
		}
		::RegUnLoadKey(HKEY_USERS, pszName);
	}

	return hres;
}


STDMETHODIMP CLUDatabase::AddUser(LPCSTR pszName, IUser *pSourceUser,
                                  IUserProfileInit *pInit, IUser **ppOut)
{
	if (ppOut != NULL)
		*ppOut = NULL;

    if (IsCurrentUserSupervisor(this) != S_OK)
		return E_ACCESSDENIED;

	HRESULT hres = CreateUser(pszName, pSourceUser, FALSE, pInit);
	if (FAILED(hres))
		return hres;

	if (ppOut != NULL)
		hres = GetUser(pszName, ppOut);

	return hres;
}


STDMETHODIMP CLUDatabase::GetUser(LPCSTR pszName, IUser **ppOut)
{
	*ppOut = NULL;

	CLUUser *pUser = new CLUUser(this);

	if (pUser == NULL) {
		return ResultFromScode(E_OUTOFMEMORY);
	}

	HRESULT err = pUser->Init(pszName);
	if (SUCCEEDED(err) && !pUser->Exists()) {
		err = HRESULT_FROM_WIN32(ERROR_NO_SUCH_USER);
	}

	if (FAILED(err) || !pUser->Exists()) {
		pUser->Release();
		return err;
	}

	*ppOut = pUser;

	return NOERROR;
}


STDMETHODIMP CLUDatabase::GetSpecialUser(DWORD nSpecialUserCode, IUser **ppOut)
{
	switch (nSpecialUserCode) {
	case GSU_CURRENT:
		return GetCurrentUser(ppOut);
		break;

	case GSU_DEFAULT:
		return GetUser(szDefaultUserName, ppOut);
		break;

	default:
		return ResultFromScode(E_INVALIDARG);
	};

	return NOERROR;
}


HRESULT GetSystemCurrentUser(NLS_STR *pnlsCurrentUser)
{
	DWORD cbBuffer = pnlsCurrentUser->QueryAllocSize();
	UINT err;
	if (!::GetUserName(pnlsCurrentUser->Party(), &cbBuffer))
		err = ::GetLastError();
	else
		err = NOERROR;
	pnlsCurrentUser->DonePartying();

	return HRESULT_FROM_WIN32(err);
}


STDMETHODIMP CLUDatabase::GetCurrentUser(IUser **ppOut)
{
	if (m_CurrentUser == NULL) {
		NLS_STR nlsCurrentUser(cchMaxUsername+1);
		UINT err = nlsCurrentUser.QueryError();
		if (err)
			return HRESULT_FROM_WIN32(err);

		HRESULT hres = GetSystemCurrentUser(&nlsCurrentUser);
		if (FAILED(hres))
			return hres;

		hres = GetUser(nlsCurrentUser.QueryPch(), (IUser **)&m_CurrentUser);
		if (FAILED(hres))
			return hres;
	}

	*ppOut = m_CurrentUser;
	m_CurrentUser->AddRef();

	return NOERROR;
}


STDMETHODIMP CLUDatabase::SetCurrentUser(IUser *pUser)
{
	if (pUser->QueryInterface != CLUUser::QueryInterface)
		return E_POINTER;	/* we didn't create this IUser! */

	CLUUser *pCLUUser = (CLUUser *)pUser;
	HPWL hpwlUser;
	if (!pCLUUser->m_fAuthenticated ||
		FAILED(pCLUUser->GetPasswordCache(pCLUUser->m_nlsPassword.QueryPch(), &hpwlUser)))
	{
		return HRESULT_FROM_WIN32(ERROR_NOT_AUTHENTICATED);
	}
	::ClosePasswordCache(hpwlUser, TRUE);

	CLUUser *pClone;

	HRESULT hres = GetUser(pCLUUser->m_nlsUsername.QueryPch(), (IUser **)&pClone);
	if (FAILED(hres))
		return hres;

	/* Make sure the clone object is authenticated properly. */
	hres = pClone->Authenticate(pCLUUser->m_nlsPassword.QueryPch());
	if (FAILED(hres)) {
		return HRESULT_FROM_WIN32(ERROR_NOT_AUTHENTICATED);
	}

	if (m_CurrentUser != NULL) {
		m_CurrentUser->Release();
	}

	m_CurrentUser = pClone;
	return NOERROR;
}


STDMETHODIMP CLUDatabase::DeleteUser(LPCSTR pszName)
{
	NLS_STR nlsName(MAX_PATH);
	if (nlsName.QueryError() != ERROR_SUCCESS)
		return HRESULT_FROM_WIN32(nlsName.QueryError());

    /* Check supervisor privilege up front, this'll handle the not-logged-on
     * case later if we re-enable supervisor stuff.
     */
    if (IsCurrentUserSupervisor(this) != S_OK)
        return E_ACCESSDENIED;

	IUser *pCurrentUser;

    HRESULT hres = GetCurrentUser(&pCurrentUser);
    if (SUCCEEDED(hres)) {

    	/* Check current user's name and make sure we're not deleting him.
    	 * Note that because the current user must be an authenticated supervisor,
    	 * and you can't delete the current user, you can never delete the last
    	 * supervisor using this function.
    	 */
    	DWORD cb = nlsName.QueryAllocSize();
    	hres = pCurrentUser->GetName(nlsName.Party(), &cb);
    	nlsName.DonePartying();
    	if (SUCCEEDED(hres) && !::stricmpf(pszName, nlsName.QueryPch()))
            hres = HRESULT_FROM_WIN32(ERROR_BUSY);

    	if (FAILED(hres))
            return hres;
    }

    /* Check system's idea of current user as well. */

    hres = GetSystemCurrentUser(&nlsName);
    if (SUCCEEDED(hres)) {
        if (!::stricmpf(pszName, nlsName.QueryPch()))
            return HRESULT_FROM_WIN32(ERROR_BUSY);
    }

    return DeleteProfile(pszName);
}


STDMETHODIMP CLUDatabase::RenameUser(LPCSTR pszOldName, LPCSTR pszNewName)
{
	return ResultFromScode(E_NOTIMPL);
}


STDMETHODIMP CLUDatabase::EnumUsers(IEnumUnknown **ppOut)
{
	*ppOut = NULL;

	CLUEnum *pEnum = new CLUEnum(this);

	if (pEnum == NULL) {
		return ResultFromScode(E_OUTOFMEMORY);
	}

	HRESULT err = pEnum->Init();
	if (FAILED(err)) {
		pEnum->Release();
		return err;
	}

	*ppOut = pEnum;

	return NOERROR;
}



STDMETHODIMP CLUDatabase::Authenticate(HWND hwndOwner, DWORD dwFlags,
							 LPCSTR pszName, LPCSTR pszPassword,
							 IUser **ppOut)
{
	if (dwFlags & LUA_DIALOG) {
        if (!UseUserProfiles() || FAILED(VerifySupervisorPassword(szNULL))) {
            return InstallWizard(hwndOwner);
        }
        return ::DoUserDialog(hwndOwner, dwFlags, ppOut);
	}

	/* Null out return pointer for error cases. */
	if (ppOut != NULL)
		*ppOut = NULL;

	IUser *pUser;
	BOOL fReleaseMe = TRUE;

	HRESULT hres = GetUser(pszName, &pUser);
	if (SUCCEEDED(hres)) {
		hres = pUser->Authenticate(pszPassword);
		if (SUCCEEDED(hres)) {
			if ((dwFlags & LUA_SUPERVISORONLY) && (pUser->IsSupervisor() != S_OK)) {
				hres = E_ACCESSDENIED;
			}
			else if (ppOut != NULL) {
				*ppOut = pUser;
				fReleaseMe = FALSE;
			}
		}
		if (fReleaseMe)
			pUser->Release();
	}
	return hres;
}


STDMETHODIMP CLUDatabase::InstallComponent(REFCLSID clsidComponent,
										   LPCSTR pszName, DWORD dwFlags)
{
	return ResultFromScode(E_NOTIMPL);
}


STDMETHODIMP CLUDatabase::RemoveComponent(REFCLSID clsidComponent, LPCSTR pszName)
{
	return ResultFromScode(E_NOTIMPL);
}


#ifdef MSLOCUSR_USE_SUPERVISOR_PASSWORD

HRESULT IsCurrentUserSupervisor(IUserDatabase *pDB)
{
    IUser *pCurrentUser = NULL;

    HRESULT hres = pDB->GetCurrentUser(&pCurrentUser);
    if (SUCCEEDED(hres)) {
        hres = pCurrentUser->IsSupervisor();
    }
    if (pCurrentUser != NULL) {
        pCurrentUser->Release();
    }
    return hres;
}
#else
HRESULT IsCurrentUserSupervisor(IUserDatabase *pDB) { return S_OK; }
#endif
