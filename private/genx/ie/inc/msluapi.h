//*********************************************************************
//*                  Microsoft Internet Explorer                     **
//*            Copyright(c) Microsoft Corp., 1996-1998               **
//*********************************************************************

#ifndef _MSLUAPI_H_
#define _MSLUAPI_H_

#ifdef USER_SETTINGS_IMPLEMENTED

/************************************************************************

IUserSettings interface

This interface is used to manipulate the settings for a particular component,
corresponding to a local user account.  An IUserSettings interface may be
obtained by CLSID or name, or through enumeration;  in both cases, this is
relative to a particular user.

Member functions, other than IUnknown:

GetCLSID(CLSID *pclsidOut)
	Returns the CLSID identifying the component.  May be GUID_NULL if no
	CLSID is defined for the component.

GetName(LPSTR pbBuffer, UINT cbBuffer)
	Returns a unique name identifying the component.  This may be used
	instead of a CLSID if the component provider does not wish to provide
	a COM server to help administer the settings.

GetDisplayName(LPSTR pbBuffer, UINT cbBuffer)
	Returns a user-friendly name for the component, suitable for presentation
	to the user.

QueryKey(HKEY *phkeyOut)
	Returns a registry key where the component stores settings for the
	specified user.  The key is owned by the interface and must not be
	closed by the application using RegCloseKey, otherwise changes will
	not be propagated correctly.

((((
OpenKey(HKEY *phkeyOut, DWORD fdwAccess)
	Returns a registry key where the component stores settings for the
	specified user.  The key MUST be closed using IUserSettings::CloseKey
	so that changes will be propagated correctly.  fdwAccess indicates
	the type of access desired;  valid values include GENERIC_READ and
	GENERIC_WRITE.

CloseKey(HKEY hKey)
	Closes a registry key obtained via IUserSettings::OpenKey.

Lock(BOOL fLock)
	Locks or unlocks the settings for updates.  Attempting to lock the
	settings will fail if they are already locked.  Locking the settings
	does not, however, affect any of the other member functions
))))
************************************************************************/

DECLARE_INTERFACE_(IUserSettings, IUnknown)
{
	// *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

	STDMETHOD(GetCLSID) (THIS_ CLSID *pclsidOut) PURE;
	STDMETHOD(GetName) (THIS_ LPSTR pbBuffer, LPDWORD pcbBuffer) PURE;
	STDMETHOD(GetDisplayName) (THIS_ LPSTR pbBuffer, LPDWORD pcbBuffer) PURE;

	STDMETHOD(QueryKey) (THIS_ HKEY *phkeyOut) PURE;
};
#endif  /* USER_SETTINGS_IMPLEMENTED */


/************************************************************************

IUser interface

This interface is used to manipulate a local user account.  It allows
various operations to be performed on a particular user.  To obtain one
of these interfaces, the companion interface IUserDatabase must be
used -- its AddUser, GetUser, and GetCurrentUser member functions all
return IUser objects, as does IEnumUsers::Next.

In all descriptions here, "the user" refers to the user which this
ILocalUser object describes.  "The current user" means the user who
is currently logged on at the workstation.

If the current user is a supervisor, all functions are allowed.  Otherwise,
a more limited set of member functions is available if the IUser object
corresponds to the current user.  If the current user is not a supervisor
and the IUser object refers to a different user, a still more limited set
of functions is allowed.

Member functions, other than IUnknown:

GetName(LPSTR pbBuffer, UINT cbBuffer)
	Returns the user's logon name.

GetProfileDirectory(LPSTR pbBuffer, UINT cbBuffer)
	Returns the user's local profile directory (e.g., C:\WINDOWS\PROFILES\gregj).
	May fail if the user is the default user (doesn't really have a profile
	directory as such).

IsSupervisor()
	Returns whether the user is a supervisor or not.  This is not a generic
	property because it's actually based on the presence of security info
	in the user's PWL (at least on win95).

SetSupervisorPrivilege(BOOL fMakeSupervisor, LPCSTR pszSupervisorPassword)
	Grants or revokes supervisor privilege for the user.  Only supervisors
	can grant or revoke that privilege, of course.  If pszSupervisorPassword
    is not NULL, it is used to determine whether the current user is a
    supervisor.  If it is NULL, then the current user's password cache is
    used instead.  This allows making any user into a supervisor without
    the current user being one.

MakeTempSupervisor(BOOL fMakeSupervisor, LPCSTR pszSupervisorPassword)
    Grants or revokes supervisor privilege for the user, but only for the
    lifetime of this IUser object.  As soon as the object is destroyed,
    the user is no longer considered a supervisor, and in fact other IUser
    objects currently in existence which refer to the same user will not
    indicate him as a supervisor.

    Note that MakeTempSupervisor(FALSE) only revokes temporary-supervisor
    privilege granted by MakeTempSupervisor(TRUE).  If the user still has
    the supervisor password in his PWL, he will still be considered a
    supervisor.

AppearsSupervisor()
	Returns whether or not the user should appear as a supervisor in a list
	of users.  This allows querying this property on each user for display
	purposes without taking the large performance hit to locate each user's
	PWL, open it up, get the supervisor key out, and validate it.  Instead,
	a registry value under the user's key is used to maintain this value.
	It should NOT be used to determine whether the user has permission to
	do something, because the simple registry value is not as secure.

Authenticate(LPCSTR pszPassword)
	Attempts to authenticate the user using the given password.  Returns
	S_OK if the password is correct for the user, or an error otherwise.
	No user interface is displayed by this function.

ChangePassword(LPCSTR pszOldPassword, LPCSTR pszNewPassword)
	Attempts to change the user's password from the given old password
	to the given new password.  Returns an error code indicating success
	or failure.  If the current user is a supervisor, the old password
	may be NULL, in which case the supervisor's credentials are used to
	get the password via other means.

GetPasswordCache(LPCSTR pszPassword, LPHPWL phOut)
	Returns a handle to the user's password cache, suitable for use with
	the MSPWL32.DLL APIs.  May fail, of course, if password caching is
	disabled.

LoadProfile(LPHKEY phkeyUser)
    Loads the user's profile into the registry and returns a handle to the
    root key.  The current user can always load his own profile (just returns
    HKEY_CURRENT_USER);  to load other users' profiles, the current user must
    be a supervisor.  IUser::UnloadProfile() should always be called when the
    caller is done playing with the user's profile.

UnloadProfile(HKEY hkeyUser)
    Unloads the user's profile from the registry if possible, and closes the
    key handle returned by IUser::LoadProfile.  If the specified user is the
    current user, this function does nothing.

GetComponentSettings(REFCLSID clsidComponent, LPCSTR pszName,
					 IUnknown **ppOut, DWORD fdwAccess)
    CURRENTLY NOT IMPLEMENTED
	Returns an IUserSettings interface which can be used to access the
	user's settings for a particular component.  Either clsidComponent or
	pszName may be used to refer to the component whose settings are to be
	accessed.  If pszName is not NULL, it takes precedence over clsidComponent.
	fdwAccess specifies whether the caller wants read or write access to the
	settings.  If the component's settings are restricted and the current user
	is not a supervisor, only GENERIC_READ access will be allowed;
	GENERIC_WRITE will fail.

EnumerateComponentSettings(IEnumUnknown **ppOut, DWORD fdwAccess)
    CURRENTLY NOT IMPLEMENTED
	Returns an IEnumUnknown interface which can be used to enumerate all
	components which have settings recorded for the user.  fdwAccess
	specifies the desired access, read or write.  If the current user
	is not a supervisor and the caller requests write access, the enumerator
	will not return any components which do not permit such access.

************************************************************************/

DECLARE_INTERFACE_(IUser, IUnknown)
{
	// *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

	STDMETHOD(GetName) (THIS_ LPSTR pbBuffer, LPDWORD pcbBuffer) PURE;
	STDMETHOD(GetProfileDirectory) (THIS_ LPSTR pbBuffer, LPDWORD pcbBuffer) PURE;

	STDMETHOD(IsSupervisor) (THIS) PURE;
	STDMETHOD(SetSupervisorPrivilege) (THIS_ BOOL fMakeSupervisor, LPCSTR pszSupervisorPassword) PURE;
	STDMETHOD(MakeTempSupervisor) (THIS_ BOOL fMakeSupervisor, LPCSTR pszSupervisorPassword) PURE;
	STDMETHOD(AppearsSupervisor) (THIS) PURE;

	STDMETHOD(Authenticate) (THIS_ LPCSTR pszPassword) PURE;
	STDMETHOD(ChangePassword) (THIS_ LPCSTR pszOldPassword, LPCSTR pszNewPassword) PURE;
	STDMETHOD(GetPasswordCache) (THIS_ LPCSTR pszPassword, LPHANDLE phOut) PURE;

    STDMETHOD(LoadProfile) (THIS_ HKEY *phkeyUser) PURE;
    STDMETHOD(UnloadProfile) (THIS_ HKEY hkeyUser) PURE;

	STDMETHOD(GetComponentSettings) (THIS_ REFCLSID clsidComponent,
									 LPCSTR pszName, IUnknown **ppOut,
									 DWORD fdwAccess) PURE;
	STDMETHOD(EnumerateComponentSettings) (THIS_ IEnumUnknown **ppOut,
										   DWORD fdwAccess) PURE;
};


/************************************************************************

IUserProfileInit interface

This interface is a helper for IUserDatabase::Install and IUserDatabase::Create.
It allows the client of those functions to perform initialization of a new
user's profile before and after the new user's per-user folders are set up.

Member functions, other than IUnknown:

PreInitProfile(HKEY hkeyUser, LPCSTR pszProfileDir)
	Called when the user's profile has been created, but no per-user folders
    have been created or initialized yet.  Here the implementer can add keys to
    the user's profile which will affect the initialization of those per-user
    folders.  hkeyUser is the root of the user's profile, which would be
    HKEY_CURRENT_USER if the user were currently logged on.

PostInitProfile(HKEY hkeyUser, LPCSTR pszProfileDir)
    Called after the user's per-user folders have been created and initialized.
    Here the implementer can add keys to the user's profile which will control
    roaming of per-user folders, without causing the IUserDatabase profile
    cloning code to want to initialize those folders from their default
    locations.

************************************************************************/

DECLARE_INTERFACE_(IUserProfileInit, IUnknown)
{
	// *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    STDMETHOD(PreInitProfile) (THIS_ HKEY hkeyUser, LPCSTR pszProfileDir) PURE;
    STDMETHOD(PostInitProfile) (THIS_ HKEY hkeyUser, LPCSTR pszProfileDir) PURE;
};


/************************************************************************

IUserDatabase interface

This interface is used to manage the local user database as a whole.  Any
activities which deal with the list of users in any way are done through
this interface;  operations which deal with the properties (other than the
name) of an existing user are done through IUser.

Member functions, other than IUnknown:

Install(LPCSTR pszSupervisorName, LPCSTR pszSupervisorPassword,
        LPCSTR pszRatingsPassword, IUserProfileInit *pInit)
	Installs the user settings subsystem.  This includes creating an account
	for the supervisor.  A separate member function is necessary for doing
	this because all the others would insist that the current user already
	be a supervisor.  The pInit object (optional, may be NULL) is called
    back to allow the installer to do initialization of the profile being
    created, before and after its per-user files are copied.

AddUser(LPCSTR pszName, IUser *pSourceUser, IUserProfileInit *pInit,
        IUser **ppOut)
	Creates a new user on the system.  This includes creating a profile
	for the user.  It does not, however, include creating a password list
	file.  IUser::ChangePassword can be used to configure the password
	for the user.  An IUser object is returned to the caller so that the
	caller can configure the properties of the user.  This function will
	fail if the current user is not a supervisor.  The caller can optionally
	specify a user account to be cloned.  The pInit object (optional, may be
	NULL) is called back to allow the installer to do initialization of the
	profile being created, before and after its per-user files are copied.

GetUser(LPCSTR pszName, IUser **ppOut)
	Gets an IUser object corresponding to the specified user.  The current
	user need not be a supervisor to call this function, and any user's
	name may be specified.  The IUser interface will control what a non-
	supervisor can and cannot do to the user object.

GetSpecialUser(DWORD nSpecialUserCode, IUser **ppOut)
	Gets an IUser object corresponding to a special particular user.
	Current values for nSpecialUserCode include GSU_CURRENT, meaning
	the currently logged on user, and GSU_DEFAULT, meaning the default
	user identity (i.e., the identity used when nobody is logged on,
	also used as a template when creating new identities).

GetCurrentUser(IUser **ppOut)
	Gets an IUser object corresponding to the currently logged on user.
	Shorthand for GetSpecialUser(GSU_CURRENT, ppOut).

SetCurrentUser(IUser *pUser)
	Sets this IUserDatabase object's idea of who the current user is.
	The user must have previously been authenticated.  This user object
	is used for all checks which, for example, determine whether the
	"current user" is a supervisor, or whether a user can access his
	or her own settings, etc.  SetCurrentUser does not AddRef the IUser
	object passed.

DeleteUser(LPCSTR pszName)
	Deletes the profile and password cache for the specified user,
	effectively destroying that user's identity.  This function may
	only be called if the current user is a supervisor.  Any existing
	IUser objects which refer to the user are no longer useful, but
	still must be destroyed in the ordinary way (Release()).

RenameUser(LPCSTR pszOldName, LPCSTR pszNewName)
	Changes the username of a user.  This function may only be called
	if the current user is a supervisor.

EnumUsers(IEnumUnknown **ppOut)
	Returns an IEnumUnknown object which the caller can use to enumerate
	the local users on the system.

Authenticate(HWND hwndOwner, DWORD dwFlags, LPCSTR pszName, LPCSTR pszPassword,
			 IUser **ppOut)
	Attempts to authenticate a user.  dwFlags specifies whether or not
	to prompt for credentials, and whether or not non-supervisors are
	acceptable.  If no dialog is to be displayed by the API, then the
	pszName and pszPassword parameters are used instead.  If the credentials
	are authenticated succcessfully, S_OK is returned.  The ppOut parameter,
	if not NULL, is filled with a pointer to an IUser object describing the
	user who was authenticated, in case the caller cares to find out about
	who typed in their name and password.

    The dwFlags parameter specifies whether UI will be displayed by the
    function, and whether or not the credentials will be cached in memory
    for use at the next logon.

InstallComponent(REFCLSID clsidComponent, LPCSTR pszName, DWORD dwFlags)
    CURRENTLY NOT IMPLEMENTED

	Installs a component into the settings database, so that it will appear
	in the settings UI.  clsidComponent or pszName can be used to refer to
	the component being installed;  use of a CLSID is preferable because
	then the component can provide server code which renders the settings
	UI for that component, and knows how to initialize the settings for a
	new user.

	The only bit currently defined for dwFlags is:

	SETTINGS_NS_CAN_WRITE:		Non-supervisors can change their own settings
								for this component.

	A component's settings for the current user can always be read, at least
	programmatically -- there is no point in storing settings which can only
	be accessed if the current user is a supervisor.  If non-supervisors
	should not be shown the UI for restricted settings (even a read-only UI),
	that decision can be made at the UI level.

	InstallComponent fails if the current user is not a supervisor.

RemoveComponent(REFCLSID clsidComponent, LPCSTR pszName)
    CURRENTLY NOT IMPLEMENTED

	Removes a component from the settings database, so that it will no longer
	appear in the settings UI.  This also removes this component's settings
	from all user identities.

	RemoveComponent fails if the current user is not a supervisor.

InstallWizard(HWND hwndParent)
    Runs the wizard that switches to multiuser mode.

AddUserWizard(HWND hwndParent)
    Runs the wizard that adds a new user, invoking the go-multiuser wizard
    if necessary.

UserCPL(HWND hwndParent)
    Invokes the general user management UI as seen in Control panel, invoking
    the go-multiuser wizard if necessary.

************************************************************************/

DECLARE_INTERFACE_(IUserDatabase, IUnknown)
{
	// *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

	STDMETHOD(Install) (THIS_ LPCSTR pszSupervisorName, LPCSTR pszSupervisorPassword,
	                    LPCSTR pszRatingsPassword, IUserProfileInit *pInit) PURE;
	STDMETHOD(AddUser) (THIS_ LPCSTR pszName, IUser *pSourceUser,
	                    IUserProfileInit *pInit, IUser **ppOut) PURE;
	STDMETHOD(GetUser) (THIS_ LPCSTR pszName, IUser **ppOut) PURE;
	STDMETHOD(GetSpecialUser) (THIS_ DWORD nSpecialUserCode, IUser **ppOut) PURE;
	STDMETHOD(GetCurrentUser) (THIS_ IUser **ppOut) PURE;
	STDMETHOD(SetCurrentUser) (THIS_ IUser *pUser) PURE;
	STDMETHOD(DeleteUser) (THIS_ LPCSTR pszName) PURE;
	STDMETHOD(RenameUser) (THIS_ LPCSTR pszOldName, LPCSTR pszNewName) PURE;
	STDMETHOD(EnumUsers) (THIS_ IEnumUnknown **ppOut) PURE;

	STDMETHOD(Authenticate) (THIS_ HWND hwndOwner, DWORD dwFlags,
							 LPCSTR pszName, LPCSTR pszPassword,
							 IUser **ppOut) PURE;

	STDMETHOD(InstallComponent) (THIS_ REFCLSID clsidComponent, LPCSTR pszName,
								 DWORD dwFlags) PURE;
	STDMETHOD(RemoveComponent) (THIS_ REFCLSID clsidComponent, LPCSTR pszName) PURE;
    STDMETHOD(InstallWizard) (THIS_ HWND hwndParent) PURE;
    STDMETHOD(AddUserWizard) (THIS_ HWND hwndParent) PURE;

    STDMETHOD(UserCPL) (THIS_ HWND hwndParent) PURE;
};

// codes for IUserDatabase::GetSpecialUser

const DWORD GSU_CURRENT = 0;				// current user
const DWORD GSU_DEFAULT = 1;				// default user profile

// flags for IUserDatabase::Authenticate
const DWORD LUA_DIALOG = 0x00000001;			// display dialog to get credentials
												// otherwise use pszName, pszPassword
const DWORD LUA_SUPERVISORONLY = 0x00000002;	// authenticate supervisors only
const DWORD LUA_FORNEXTLOGON = 0x00000004;      // cache credentials for next logon

// flags for IUserDatabase::InstallComponent
const DWORD SETTINGS_NS_CAN_WRITE = 0x01;	// non-supervisors can change their own settings

#endif  // _MSLUAPI_H_
