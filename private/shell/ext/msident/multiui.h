/************************************************
    MultiUI.h

    Header for multiple user functionality.

    Initially by Christopher Evans (cevans) 7/16/98
*************************************************/
#ifndef _MULTIUI_H
#define _MULTIUI_H
#include "multiusr.h"


#define     IDH_IDENTITY_NAME                   50100
#define     IDH_IDENTITY_MANAGE                 50140
#define     IDH_IDENTITY_LIST                   50155
#define     IDH_IDENTITY_DELETE                 50165
#define     IDH_IDENTITY_ADD                    50175
#define     IDH_IDENTITY_PROPERTIES             50180
#define     IDH_IDENTITY_DEFAULT                50185
#define     IDH_IDENTITY_STARTAS                50190
#define     IDH_IDENTITY_PROMPT_PWORD           50105 //Add new identity; ask for password
#define     IDH_IDENTITY_ENTER_PWORD            50110 //Add new identity; password
#define     IDH_IDENTITY_CONFIRM_PWORD          50115 //Add new identity; confirm password
#define     IDH_IDENTITY_ASK_PWORD              50125 //Change user; ask for pword
#define     IDH_IDENTITY_CHANGE_PWORD           50130 //Change user; change pword button
#define     IDH_IDENTITY_PWORD_OLD              50145 //Change user; old pword
#define     IDH_IDENTITY_PWORD_NEW              50150 //Change user; new pword
#define     IDH_IDENTITY_DELETE_PWORD           50170
#define     IDH_IDENTITY_TELLMEMORE             50195  
#define     IDH_IDENTITY_TELLMEMORE_CONTENT     50200
#define     IDH_MULTI_LOG_OFF                   50120 
#define     IDH_MULTI_MNG_IDENT_DEFAULT	        50185
#define     IDH_MULTI_MNG_DEFAULT_LIST          50160
#define     IDH_MULTI_DELETE_PWORD	            50170

#define  ID_LOGIN_AS_LAST                       0
#define  ID_LOGIN_ASK_ME                        1

void            MU_ShowErrorMessage(HWND hwnd, UINT iMsgID, UINT iTitleID);

BOOL            MU_CreateNewUser(HWND  hwnd, LPUSERINFO  lpUserInfo);
BOOL CALLBACK   AddUserDlgProc(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK   ChangeUserPwdDlgProc(HWND   hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam);
BOOL            MU_ChangeUserPassword(HWND hwnd, TCHAR *lpszOldNewPassword);
BOOL CALLBACK   EnterUserPwdDlgProc(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam);
BOOL            MU_EnterUserPassword(HWND hwnd, TCHAR *lpszNewPassword);
BOOL CALLBACK   ConfirmUserPwdDlgProc(HWND hDlg, UINT iMsg, WPARAM  wParam, LPARAM  lParam);
BOOL            MU_ConfirmUserPassword(HWND hwnd, TCHAR *lpszMsg, TCHAR *lpszPassword);
BOOL CALLBACK   DeleteUserDlgProc(HWND  hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam);
BOOL            MU_GetUserToDelete(HWND hwnd, LPDELETEUSERDIALOGINFO lpszOutUserInfo);
BOOL CALLBACK   ConfirmDeleteUserDlgProc(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam);
BOOL            MU_ConfirmDeleteUser(HWND hwnd, TCHAR *lpszUsername);
BOOL            MU_Login(HWND hwnd, DWORD dwFlags, TCHAR *lpszUsername); 
BOOL            MU_ChangeUserSettings(HWND hwnd, LPUSERINFO lpUserInfo);
BOOL            MU_ManageUsers(HWND hwnd, TCHAR *lpszUsername, DWORD dwFlags); 
BOOL            MU_ConfirmUserPassword(HWND hwnd, TCHAR *lpszMsg, TCHAR *lpszPassword);
void            _StripDefault(LPSTR psz);


#endif //_MULTIUI_H
