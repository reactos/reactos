#ifndef USERINFO_H_INCLUDED
#define USERINFO_H_INCLUDED

#include "dpa.h"

class CUserInfo
{
public:
    // Typedefs
    enum USERTYPE
    {
        LOCALUSER = 0,
        DOMAINUSER,
        GROUP
    };

    // Group pseudonym tells any functions that may change a user's group that the
    // user selected an option button that says something like "standard user" or
    // "restricted user" instead of selecting the real group name from a list.
    // In this case, the group change functions may display custom error messages that
    // mention "standard user access" instead of "power users group", for example.
    enum GROUPPSEUDONYM
    {
        RESTRICTED = 0,
        STANDARD,
        USEGROUPNAME
    };

public:
    // Functions
    CUserInfo();
    ~CUserInfo();
    HRESULT Load(PSID psid, BOOL fLoadExtraInfo = NULL);
    HRESULT Reload(BOOL fLoadExtraInfo = NULL);

    HRESULT Create(HWND hwndError, GROUPPSEUDONYM grouppseudonym);
    
    HRESULT UpdateUsername(LPTSTR pszNewUsername);
    HRESULT UpdateFullName(LPTSTR pszFullName);
    HRESULT UpdatePassword(BOOL* pfBadPWFormat);
    HRESULT UpdateGroup(HWND hwndError, LPTSTR pszGroup, GROUPPSEUDONYM grouppseudonym);
    HRESULT UpdateDescription(LPTSTR pszDescription);
    
    HRESULT Remove();
    HRESULT InitializeForNewUser();
    HRESULT GetExtraUserInfo();
    HRESULT SetUserType();
    HRESULT SetLocalGroups();

    void HidePassword();
    void RevealPassword();
    void ZeroPassword();

public:
    // Data

    // Index of this user's icon (local, domain, group)
    USERTYPE m_userType;

    TCHAR m_szUsername[MAX_USER + 1];
    TCHAR m_szDomain[MAX_DOMAIN + 1];
    TCHAR m_szComment[MAXCOMMENTSZ];
    TCHAR m_szFullName[MAXCOMMENTSZ];

    // Only if we're creating a new user:
    TCHAR m_szPasswordBuffer[MAX_PASSWORD + 1];
    UNICODE_STRING m_Password;
    UCHAR m_Seed;

    // Room for AT LEAST two group names plus a ';' a ' ' and a '\0'
    TCHAR m_szGroups[MAX_GROUP * 2 + 3];

    // The user's SID
    PSID m_psid;
    SID_NAME_USE m_sUse;

    // Have we read the user's full name and comment yet?
    BOOL m_fHaveExtraUserInfo;
private:
    // Helpers
    HRESULT RemoveFromLocalGroups();
    HRESULT ChangeLocalGroups(HWND hwndError, GROUPPSEUDONYM grouppseudonym);
};

class CUserListLoader
{
public:
    CUserListLoader();
    ~CUserListLoader();

    HRESULT Initialize(HWND hwndUserListPage);

    void EndInitNow() {m_fEndInitNow = TRUE;}
    BOOL InitInProgress() 
    {return (WAIT_OBJECT_0 != WaitForSingleObject(m_hInitDoneEvent, 0));}

private:
    HRESULT UpdateFromLocalGroup(LPWSTR szLocalGroup);
    HRESULT AddUserInformation(PSID psid);
    BOOL HasUserBeenAdded(PSID psid);    

    static DWORD WINAPI InitializeThread(LPVOID pvoid);
private:
    // Data
    HWND m_hwndUserListPage;
    HANDLE m_hInitDoneEvent;
    BOOL m_fEndInitNow;
    CDPA<CUserInfo> m_dpaAddedUsers;
};

// User info functions
BOOL UserAlreadyHasPermission(CUserInfo* pUserInfo, HWND hwndMsgParent);

#endif // !USERINFO_H_INCLUDED