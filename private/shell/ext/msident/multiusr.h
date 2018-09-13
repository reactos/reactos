/************************************************
    MultiUsr.h

    Header for multiple user functionality.

    Initially by Christopher Evans (cevans) 7/16/98
*************************************************/
#ifndef _MULTIUSR_H
#define _MULTIUSR_H


#define CCH_USERPASSWORD_MAX_LENGTH 16
#define CCH_USERNAME_MAX_LENGTH     CCH_IDENTITY_NAME_MAX_LENGTH
typedef struct 
{
    BOOL        fUsePassword;
    BOOL        fPasswordValid;
    GUID        uidUserID;
    TCHAR       szUsername[CCH_USERNAME_MAX_LENGTH];
    TCHAR       szPassword[CCH_USERPASSWORD_MAX_LENGTH];
}USERINFO, *LPUSERINFO;


typedef struct 
{
    TCHAR       szUsername[CCH_USERNAME_MAX_LENGTH];
    BOOL        fDeleteMessages;
}DELETEUSERDIALOGINFO, *LPDELETEUSERDIALOGINFO;


typedef struct 
{
    TCHAR        szPassword[CCH_USERPASSWORD_MAX_LENGTH];
    TCHAR        szMsg[255];
}CONFIRMPWDDIALOGINFO, *LPCONFIRMPWDDIALOGINFO;

typedef struct  
{
    BOOL        fUsePassword;
    TCHAR       szPassword[CCH_USERPASSWORD_MAX_LENGTH];
}PASSWORD_STORE;


#define  ID_LOGIN_AS_LAST                       0
#define  ID_LOGIN_ASK_ME                        1

#define ASK_BEFORE_LOGIN    (0xffffffff)

class CStringList
{
public:
                            CStringList();
    virtual                 ~CStringList();

    inline  int             GetLength       (void)      {return m_count;}
            void            AddString       (TCHAR*  lpszInString);
            void            RemoveString    (int    iIndex);
            TCHAR*          GetString       (int    iIndex);   
            void            Sort            ();
private:
    int         m_count;
    int         m_ptrCount;
    TCHAR**      m_strings;      
};

EXTERN_C void   MU_Init();
CStringList*    MU_GetUsernameList(void);
HRESULT         MU_UsernameToUserId(TCHAR *lpszUsername, GUID *puidUserId);
BOOL            MU_GetPasswordForUsername(TCHAR *lpszInUsername, TCHAR *szOutPassword, BOOL *pfUsePassword);
BOOL            MU_UsernameExists(TCHAR*    lpszUsername);
BOOL            MU_GetUserInfo(GUID *puidUserId, LPUSERINFO lpUserInfo);
BOOL            MU_SetUserInfo(LPUSERINFO lpUserInfo);
HRESULT         MU_SwitchToUser(TCHAR *lpszUsername);
void            MU_SwitchToLastUser();
HRESULT         MU_GetUserDirectoryRoot(GUID *uidUserID, DWORD dwFlags, WCHAR   *lpszUserRoot, int cch);
ULONG           MU_CountUsers(void);
HRESULT         MU_GetRegRootForUserID(GUID *puidUserId, LPSTR pszPath);
HRESULT         MU_CreateUser(LPUSERINFO   lpUserInfo);
HRESULT         MU_DeleteUser(GUID *puidUserId);
LPCTSTR         MU_GetRegRoot();
HRESULT         MU_MakeDefaultUser(GUID *puidUserId);
BOOL            MU_GetCurrentUserID(GUID *puidUserId);
BOOL            MU_GetDefaultUserID(GUID *puidUserId);
BOOL            MU_UserIdToUsername(GUID *puidUserId, TCHAR *lpszUsername, ULONG cch);
BOOL            MU_IdentitiesDisabled();
DWORD           MU_GetDefaultOptionIndex(HWND hwndCombo);
DWORD           MU_GetLoginOptionIndex(HWND hwndCombo);
void            MU_GetLoginOption(GUID *puidStartAs);
BOOL            MU_SetLoginOption(HWND hwndCombo,  LRESULT dOption);
BOOL            MU_CanEditIdentity(HWND hwndParent, GUID *puidIdentityId);
DWORD           MU_GenerateDirectoryNameForIdentity(GUID *puidIdentityId);
HRESULT         MU_GetDirectoryIdForIdentity(GUID *puidIdentityId, DWORD *pdwDirId);
HRESULT         MU_GetDirectoryIdForIdentity(GUID *puidIdentityId, DWORD *pdwDirId);
HRESULT         _ClaimNextUserId(GUID *puidUserId);
void            _MakeDefaultFirstUser();
BOOL            _FillListBoxWithUsernames(HWND hListbox);
BOOL            _FillComboBoxWithUsernames(HWND hCombobox, HWND hListbox);
void            _ResetRememberedLoginOption(void);
void            _RememberLoginOption(HWND hwndCombo);
void            _MigratePasswords();

#endif _MULTIUSR_H
