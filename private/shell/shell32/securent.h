#ifndef _SECURENT_INC
#define _SECURENT_INC

// all this seurity mumbo-jumbo is only necessary on NT
#ifdef WINNT

//
// we have some structs to help make sense of the horrible ACL api's
//

typedef struct _SHELL_USER_SID
{
    SID_IDENTIFIER_AUTHORITY sidAuthority;
    DWORD dwUserGroupID;
    DWORD dwUserID;
} SHELL_USER_SID, *PSHELL_USER_SID;


//
// common SHELL_USER_SID's
//
extern const SHELL_USER_SID susCurrentUser;     // the current user 
extern const SHELL_USER_SID susSystem;          // the "SYSTEM" group
extern const SHELL_USER_SID susAdministrators;  // the "Administrators" group
extern const SHELL_USER_SID susPowerUsers;      // the "Power Users" group
extern const SHELL_USER_SID susGuests;          // the "Guests" group
extern const SHELL_USER_SID susEveryone;        // the "Everyone" group


typedef struct _SHELL_USER_PERMISSION
{
    SHELL_USER_SID susID;       // identifies the user for whom you want to grant permissions to
    DWORD dwAccessType;         // currently, this is either ACCESS_ALLOWED_ACE_TYPE or  ACCESS_DENIED_ACE_TYPE
    BOOL fInherit;              // the permissions inheritable? (eg a directory or reg key and you want new children to inherit this permission)
    DWORD dwAccessMask;         // access granted (eg FILE_LIST_CONTENTS or KEY_ALL_ACCESS, etc...)
    DWORD dwInheritMask;        // mask used for inheritance, usually (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE)
    DWORD dwInheritAccessMask;  // the inheritable access granted (eg GENERIC_ALL)
} SHELL_USER_PERMISSION, *PSHELL_USER_PERMISSION;


//
// Shell helper functions for security
//
STDAPI_(SECURITY_DESCRIPTOR*) GetShellSecurityDescriptor(PSHELL_USER_PERMISSION* apUserPerm, int cUserPerm);
STDAPI_(PTOKEN_USER) GetUserToken(HANDLE hUser);
STDAPI_(LPTSTR) GetUserSid(HANDLE hToken);

STDAPI_(BOOL) GetUserProfileKey(HANDLE hToken, HKEY *phkey);
STDAPI_(BOOL) IsUserAnAdmin();
#else
// on win95 we just return FALSE here
#define GetUserProfileKey(hToken, phkey) FALSE
#endif // WINNT

#endif // _SECURENT_INC