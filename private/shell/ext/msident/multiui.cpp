/*******************************************************
    MultiUI.cpp

    Code for handling multiple user interface in IE
    and friends

    Initially by Christopher Evans (cevans) 4/28/98
********************************************************/

#include "private.h"
#include "resource.h"
#include "multiui.h"
#include "multiutl.h"
#include "multiusr.h"
#include "mluisup.h"
#include "strconst.h"
#include "commctrl.h"
extern HINSTANCE g_hInst;

static const GUID GUID_NULL = { /* 00000000-0000-0000-0000-000000000000 */ 
    0x0,
    0x0,
    0x0,
    {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0}
  };

static const HELPMAP g_rgCtxMapMultiUserGeneral[] = {
    {IDC_NO_HELP_1, NO_HELP},
    {IDC_NO_HELP_2, NO_HELP},
    {IDC_NO_HELP_3, NO_HELP},
    {IDC_NO_HELP_4, NO_HELP},
    {idcWarningIcon, NO_HELP},
    {idcConfirmMsg, NO_HELP},
    {idcErrorMsg, NO_HELP},
    {idcLoginInstr, NO_HELP},
    {idcWelcomeMsg, NO_HELP},
    {idcAdd, IDH_IDENTITY_ADD},
    {idcNewPwd, IDH_IDENTITY_PWORD_NEW},
    {idcPwdCaption,IDH_IDENTITY_ENTER_PWORD}, 
    {idcPwd, IDH_IDENTITY_ENTER_PWORD},
    {idcProperties, IDH_IDENTITY_PROPERTIES},
    {idcConfPwd, IDH_IDENTITY_CONFIRM_PWORD},
    {idcUserName, IDH_IDENTITY_NAME},
    {idcDefault, IDH_IDENTITY_DEFAULT},
    {idcOldPwd, IDH_IDENTITY_PWORD_OLD},
    {idcStartupCombo, IDH_IDENTITY_STARTAS},
    {idcDelete, IDH_IDENTITY_DELETE},
    {idcStaticName, IDH_IDENTITY_LIST},
    {idcUserNameList, IDH_IDENTITY_LIST},
    {idcTellMeMore, /*IDH_IDENTITY_TELLMEMORE_CONTENT */IDH_IDENTITY_TELLMEMORE},
    {idcStaticNames, IDH_IDENTITY_LIST},
    {idcStaticStartUp, IDH_IDENTITY_STARTAS},
    {idcUsePwd, IDH_IDENTITY_PROMPT_PWORD},
    {idcChgPwd, IDH_IDENTITY_CHANGE_PWORD},
    {idcConfirmPwd, IDH_MULTI_DELETE_PWORD},
    {idcManage, IDH_IDENTITY_MANAGE},
    {idcLogoff, IDH_MULTI_LOG_OFF},
    {idcCheckDefault, IDH_MULTI_MNG_IDENT_DEFAULT},
    {idcDefaultCombo, IDH_MULTI_MNG_DEFAULT_LIST},
    {0,0}};


/*
    MU_ShowErrorMessage

    Simple wrapper around resource string table based call to MessageBox
*/
void MU_ShowErrorMessage(HWND hwnd, UINT iMsgID, UINT iTitleID)
{
    TCHAR    szMsg[255], szTitle[63];

    MLLoadStringA(iMsgID, szMsg, ARRAYSIZE(szMsg));
    MLLoadStringA(iTitleID, szTitle, ARRAYSIZE(szTitle));
    MessageBox(hwnd, szMsg, szTitle, MB_OK | MB_ICONEXCLAMATION);
}

/*
    _StripDefault

    Remove the (Default) string from the user's name, if it
    appears.  Should be called after getting a username from
    the listbox since the default user has the string (Default) 
    appended to it
*/
void _StripDefault(LPSTR psz)
{
    TCHAR   szResString[CCH_USERNAME_MAX_LENGTH], *pszStr;
    MLLoadStringA(idsDefault, szResString, CCH_USERNAME_MAX_LENGTH);
    
    pszStr = strstr(psz, szResString);
    if(pszStr)
    {
        *pszStr = 0;
    }
}

#ifdef IDENTITY_PASSWORDS

// ****************************************************************************************************
//  C   H   A   N   G   E       U   S   E   R       P   A   S   S   W   O   R   D
/*
    _ValidateChangePasswordValues

    Validate the data entered by the user.  Return true only if everything is
    legitimate, 
*/

static BOOL _ValidateChangePasswordValues(HWND   hDlg, 
                                         TCHAR*  lpszOldNewPassword)
{
    TCHAR    szOldPW[255], szPW1[255], szPW2[255];

    GetDlgItemText(hDlg,idcOldPwd,  szOldPW, ARRAYSIZE(szOldPW));
    GetDlgItemText(hDlg,idcNewPwd,  szPW1,   ARRAYSIZE(szPW1));
    GetDlgItemText(hDlg,idcConfPwd, szPW2,   ARRAYSIZE(szPW2));

    if (strcmp(lpszOldNewPassword, szOldPW) != 0)
    {
        MU_ShowErrorMessage(hDlg, idsPwdDoesntMatch, idsPwdError);
        SetFocus(GetDlgItem(hDlg,idcOldPwd));
        SendDlgItemMessage(hDlg,idcOldPwd,EM_SETSEL,0,-1);
        return false;
    }

    if (strcmp(szPW1, szPW2) != 0)
    {
        MU_ShowErrorMessage(hDlg, idsPwdChgNotMatch, idsPwdError);
        SetFocus(GetDlgItem(hDlg,idcNewPwd));
        SendDlgItemMessage(hDlg,idcNewPwd,EM_SETSEL,0,-1);
        return false;
    }

    strcpy(lpszOldNewPassword, szPW1);

    return true;
}


/*
    _ChangeUserPwdDlgProc

    Description: Dialog proc for handling the change user password dialog.
*/

INT_PTR CALLBACK _ChangeUserPwdDlgProc(HWND     hDlg,
                                   UINT     iMsg, 
                                   WPARAM   wParam, 
                                   LPARAM   lParam)
{
    static TCHAR *sOldNewPassword;

    switch (iMsg)
    {
    case WM_INITDIALOG:
        SendMessage(GetDlgItem(hDlg, idcNewPwd), EM_LIMITTEXT, CCH_USERPASSWORD_MAX_LENGTH-1, 0);
        SendMessage(GetDlgItem(hDlg, idcOldPwd), EM_LIMITTEXT, CCH_USERPASSWORD_MAX_LENGTH-1, 0);
        SendMessage(GetDlgItem(hDlg,idcConfPwd), EM_LIMITTEXT, CCH_USERPASSWORD_MAX_LENGTH-1, 0);
        sOldNewPassword = (TCHAR *)lParam;
        return TRUE;

    case WM_HELP:
    case WM_CONTEXTMENU:
        return OnContextHelp(hDlg, iMsg, wParam, lParam, g_rgCtxMapMultiUserGeneral);

    case WM_COMMAND:
        switch(LOWORD(wParam))
        {
        case IDOK:
            if (_ValidateChangePasswordValues(hDlg, sOldNewPassword))
                MLEndDialogWrap(hDlg, IDOK);
            return true;

        case IDCANCEL:
            MLEndDialogWrap(hDlg, IDCANCEL);
            return true;

        }
        break;

    }
    return false;
}

/*
    ChangeUserPassword

    Wrapper routine for changing the user password.  Pass in the current
    password in lpszOldNewPassword which is used to confirm the current 
    password with what the user entered.  If the user enters the old 
    password correctly and enters the new password twice correctly, 
    and clicks OK, then the new password is returned in lpszOldNewPassword
    and this function returns TRUE.  Otherwise, the value in lpszOldNewPassword
    is unchanged and it returns false.

    lpszOldNewPassword must point to a TCHAR buffer large enough to hold a password 
    (CCH_USERPASSWORD_MAX_LENGTH characters)
*/

BOOL        ChangeUserPassword(HWND hwnd, TCHAR *lpszOldNewPassword) 
{
    INT_PTR bResult;
    
    Assert(hwnd);
    Assert(lpszOldNewPassword);
    
    bResult = MLDialogBoxParamWrap(MLGetHinst(), MAKEINTRESOURCEW(iddChgPwd), hwnd, _ChangeUserPwdDlgProc, (LPARAM)lpszOldNewPassword);

    //Don't actually change it here, the caller will do the right thing
    //since this may be (and is) called from another dialog with a cancel 
    //button on it

    return (bResult == IDOK);   
}


// ****************************************************************************************************
//  C   O   N   F   I   R   M       U   S   E   R       P   A   S   S   W   O   R   D

/*
    _ConfirmUserPwdDlgProc

    Description: Dialog proc for handling the confirming user password dialog.
*/  
INT_PTR CALLBACK _ConfirmUserPwdDlgProc(HWND    hDlg,
                                    UINT    iMsg, 
                                    WPARAM  wParam, 
                                    LPARAM  lParam)
{
    static LPCONFIRMPWDDIALOGINFO sConfirmPwdInfo;

    switch (iMsg)
    {
    case WM_INITDIALOG:
        Assert(lParam);

        SendMessage(GetDlgItem(hDlg, idcConfirmPwd), EM_LIMITTEXT, CCH_USERPASSWORD_MAX_LENGTH-1, 0);
        sConfirmPwdInfo = (LPCONFIRMPWDDIALOGINFO)lParam;
        SetDlgItemText(hDlg, idcConfirmMsg, sConfirmPwdInfo->szMsg);
        return TRUE;

    case WM_HELP:
    case WM_CONTEXTMENU:
        return OnContextHelp(hDlg, iMsg, wParam, lParam, g_rgCtxMapMultiUserGeneral);

    case WM_COMMAND:
        switch(LOWORD(wParam))
        {
        case IDOK:
            TCHAR    szPW[255];
            
            //if the password matches the provided password, then
            //everything is OK and the dialog can complete, otherwise,
            //barf an error message and keep waiting for a good password
            //or Cancel.
            GetDlgItemText(hDlg,idcConfirmPwd,  szPW, ARRAYSIZE(szPW));
            if (strcmp(szPW, sConfirmPwdInfo->szPassword) == 0)
                MLEndDialogWrap(hDlg, IDOK);
            else
            {
                MU_ShowErrorMessage(hDlg, idsPwdDoesntMatch, idsPwdError);
                SetFocus(GetDlgItem(hDlg,idcConfirmPwd));
                SendDlgItemMessage(hDlg,idcConfirmPwd,EM_SETSEL,0,-1);
            }
            return true;

        case IDCANCEL:
            MLEndDialogWrap(hDlg, IDCANCEL);
            return true;

        }
        break;

    }
    return false;
}

/*
    MU_ConfirmUserPassword

    Confirm that the user knows the password before it is disabled 
    in the registry.  If they enter the correct password, simply return
    true since the calling dialog box will do the right thing if the 
    user clicks cancel there.
*/

BOOL        MU_ConfirmUserPassword(HWND hwnd, TCHAR *lpszMsg, TCHAR *lpszPassword) 
{
    INT_PTR bResult;
    CONFIRMPWDDIALOGINFO    vConfirmInfo;

    Assert(hwnd);
    Assert(lpszPassword);
    Assert(lpszMsg);
    Assert(lstrlen(lpszMsg) < ARRAYSIZE(vConfirmInfo.szMsg));
    Assert(lstrlen(lpszPassword) < ARRAYSIZE(vConfirmInfo.szPassword));

    strcpy(vConfirmInfo.szMsg, lpszMsg);
    strcpy(vConfirmInfo.szPassword, lpszPassword);

    bResult = MLDialogBoxParamWrap(MLGetHinst(), MAKEINTRESOURCEW(iddPasswordOff), hwnd, _ConfirmUserPwdDlgProc, (LPARAM)&vConfirmInfo);

    return (bResult == IDOK);   
}

// ****************************************************************************************************
//  E   N   T   E   R       U   S   E   R       P   A   S   S   W   O   R   D

/*
    _ValidateNewPasswordValues

    Description: Make sure that the entered values in the new password
    dialog are legit and consistant.
*/  
static BOOL _ValidateNewPasswordValues(HWND  hDlg, 
                                         TCHAR*  lpszNewPassword)
{
    TCHAR    szPW1[255], szPW2[255];

    GetDlgItemText(hDlg,idcNewPwd,  szPW1,   ARRAYSIZE(szPW1));
    GetDlgItemText(hDlg,idcConfPwd, szPW2,   ARRAYSIZE(szPW2));

    if (strcmp(szPW1, szPW2) != 0)
    {
        MU_ShowErrorMessage(hDlg, idsPwdChgNotMatch, idsPwdError);
        SetFocus(GetDlgItem(hDlg,idcNewPwd));
        SendDlgItemMessage(hDlg,idcNewPwd,EM_SETSEL,0,-1);
        return false;
    }

    strcpy(lpszNewPassword, szPW1);

    return true;
}


/*
    _EnterUserPwdDlgProc

    Description: Dialog proc for handling the enter user password dialog.
*/
INT_PTR CALLBACK _EnterUserPwdDlgProc(HWND      hDlg,
                                   UINT     iMsg, 
                                   WPARAM   wParam, 
                                   LPARAM   lParam)
{
    static TCHAR *sNewPassword;

    switch (iMsg)
    {
    case WM_INITDIALOG:
        SendMessage(GetDlgItem(hDlg, idcNewPwd),     EM_LIMITTEXT, CCH_USERPASSWORD_MAX_LENGTH-1, 0);
        SendMessage(GetDlgItem(hDlg, idcConfPwd), EM_LIMITTEXT, CCH_USERPASSWORD_MAX_LENGTH-1, 0);
        sNewPassword = (TCHAR *)lParam;
        return TRUE;

    case WM_HELP:
    case WM_CONTEXTMENU:
        return OnContextHelp(hDlg, iMsg, wParam, lParam, g_rgCtxMapMultiUserGeneral);

    case WM_COMMAND:
        switch(LOWORD(wParam))
        {
        case IDOK:
            if (_ValidateNewPasswordValues(hDlg, sNewPassword))
                MLEndDialogWrap(hDlg, IDOK);
            return true;

        case IDCANCEL:
            MLEndDialogWrap(hDlg, IDCANCEL);
            return true;

        }
        break;

    }
    return false;
}

/*
    EnterUserPassword

    Wrapper routine for getting a new user password.  If the user enters the 
    password and confirms it correctly, and clicks OK, then the new password is 
    returned in lpszNewPassword and this function returns TRUE.  
    Otherwise, the value in lpszNewPassword is unchanged and it returns false.

    lpszNewPassword must point to a TCHAR buffer large enough to hold a password 
    (CCH_USERPASSWORD_MAX_LENGTH characters)
*/
BOOL        EnterUserPassword(HWND hwnd, TCHAR *lpszNewPassword) 
{
    INT_PTR bResult;
    
    Assert(hwnd);
    Assert(lpszNewPassword);
    
    bResult = MLDialogBoxParamWrap(MLGetHinst(), MAKEINTRESOURCEW(iddNewPwd), hwnd, _EnterUserPwdDlgProc, (LPARAM)lpszNewPassword);

    return (bResult == IDOK);   
}

#endif //IDENTITY_PASSWORDS

// ****************************************************************************************************
//  C   O   N   F   I   R   M       D   E   L   E   T   E       U   S   E   R       D   I   A   L   O   G


/*
    ConfirmDeleteUserDlgProc

    Description: Dialog proc for handling the confirm delete user dialog.

*/

INT_PTR CALLBACK _ConfirmDeleteUserDlgProc(HWND hDlg,
                                    UINT    iMsg, 
                                    WPARAM  wParam, 
                                    LPARAM  lParam)
{

    switch (iMsg)
    {
    case WM_INITDIALOG:
        Assert(lParam);

        SendDlgItemMessage(hDlg, idcWarningIcon, STM_SETICON, (WPARAM)::LoadIcon(NULL, IDI_EXCLAMATION), 0);
        SetDlgItemText(hDlg, idcErrorMsg, (TCHAR *)lParam);
        return TRUE;

    case WM_HELP:
    case WM_CONTEXTMENU:
        return OnContextHelp(hDlg, iMsg, wParam, lParam, g_rgCtxMapMultiUserGeneral);

    case WM_COMMAND:
        switch(LOWORD(wParam))
        {
        case IDOK:
        case IDCANCEL:
            MLEndDialogWrap(hDlg, LOWORD(wParam));
            return true;
        }
        break;

    }
    return false;
}

BOOL        MU_ConfirmDeleteUser(HWND hwnd, TCHAR *lpszUsername)
{
    LPTSTR  lpString = NULL;
    TCHAR   szUsername[63];
    TCHAR*  rgdw[1] = {szUsername};
    TCHAR   szBuffer[255];    // really ought to be big enough
    TCHAR   szPassword[CCH_USERPASSWORD_MAX_LENGTH];
    strcpy(szUsername, lpszUsername);


    // format the message with the username scattered throughout.
    MLLoadStringA(idsConfirmDeleteMsg, szBuffer, ARRAYSIZE(szBuffer));

    if (szBuffer[0])
    {
        FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                      FORMAT_MESSAGE_FROM_STRING |
                      FORMAT_MESSAGE_ARGUMENT_ARRAY,
                      szBuffer,
                      0, 0,
                      (LPTSTR)&lpString, 0, (va_list *)rgdw);
    }

    if (lpString)
    {
        INT_PTR bResult;
        
        // Show the Confirm Delete dialog box to make sure they really want to delete the user
        bResult = MLDialogBoxParamWrap(MLGetHinst(), MAKEINTRESOURCEW(iddConfirmUserDelete), hwnd, _ConfirmDeleteUserDlgProc, (LPARAM)lpString);
        
        LocalFree(lpString);

#ifdef IDENTITY_PASSWORDS

        if (IDOK == bResult)
        {
            BOOL    fUsePassword;
            // check to see if this user has a password, if so, then make sure that
            // they know the password before blowing it all away.
            if (MU_GetPasswordForUsername(lpszUsername, szPassword, &fUsePassword))
            {
                if (fUsePassword)
                {
                    MLLoadStringA(idsConfirmDelPwd, szBuffer, ARRAYSIZE(szBuffer));

                    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                                  FORMAT_MESSAGE_FROM_STRING |
                                  FORMAT_MESSAGE_ARGUMENT_ARRAY,
                                  szBuffer,
                                  0, 0,
                                  (LPTSTR)&lpString, 0, (va_list *)rgdw);
                    
                    if (lpString)
                    {

                        if (!MU_ConfirmUserPassword(hwnd,lpString, szPassword))
                            bResult = IDCANCEL;

                        LocalFree(lpString);
                    }
                    else
                    {
                        bResult = IDCANCEL;
                    }
                }
            }
            else    //couldn't load the password, can't delete them either
            {
                MU_ShowErrorMessage(hwnd, idsPwdNotFound, idsPwdError);
                bResult = IDCANCEL;
            }
            
            return (IDOK == bResult);
        }
#else
        return (IDOK == bResult);
#endif //IDENTITY_PASSWORDS
    }
    
    return false;
}

// ****************************************************************************************************
//  C   H   A   N   G   E       U   S   E   R       S   E   T   T   I   N   G   S   
/*
    _ValidateChangeUserValues

    Validate the data entered by the user.  Return true only if everything is
    legit, 
*/
static BOOL _ValidateChangeUserValues(HWND          hDlg, 
                                     LPUSERINFO     lpUserInfo)
{
    TCHAR   szResString[CCH_USERNAME_MAX_LENGTH], *pszStr;
    TCHAR   szUsername[255];
    ULONG   cb;
    
    GetDlgItemText(hDlg,idcUserName, szUsername, ARRAYSIZE(szUsername));
    
    cb = lstrlen(szUsername);
    UlStripWhitespace(szUsername, false, true, &cb);    //remove trailing whitespace

    // Make sure the username wasn't all spaces
    if (!cb)
    {
        MU_ShowErrorMessage(hDlg, idsUserNameTooShort, idsNameTooShort);
        SetFocus(GetDlgItem(hDlg,idcUserName));
        SendDlgItemMessage(hDlg,idcUserName,EM_SETSEL,0,-1);
        return false;
    }

    // if the username exists, and its not the same as the account currently, then
    // it is not allowed.
    if (MU_UsernameExists(szUsername) && strcmp(szUsername, lpUserInfo->szUsername) != 0)
    {
        MU_ShowErrorMessage(hDlg, idsUserNameExists, idsUserNameInUse);
        SetFocus(GetDlgItem(hDlg,idcUserName));
        SendDlgItemMessage(hDlg,idcUserName,EM_SETSEL,0,-1);
        return false;
    }
    
    lstrcpy(lpUserInfo->szUsername, szUsername);
    lpUserInfo->fUsePassword = IsDlgButtonChecked(hDlg, idcUsePwd);
    if (!lpUserInfo->fUsePassword)
        lpUserInfo->szPassword[0] = 0;

    return true;
}


/*
    ChangeUserSettingsDlgProc

    Description: Dialog proc for handling the Change user settings dialog.
*/
INT_PTR CALLBACK _ChangeUserSettingsDlgProc(HWND        hDlg,
                                   UINT     iMsg, 
                                   WPARAM   wParam, 
                                   LPARAM   lParam)
{
    static LPUSERINFO sUserInfo;
    TCHAR    szMsg[255];
    TCHAR    szPassword[CCH_USERPASSWORD_MAX_LENGTH];

    switch (iMsg)
    {
    case WM_INITDIALOG:
        Assert(lParam);
        
        sUserInfo = (LPUSERINFO)lParam;
        
        MLLoadStringA((*sUserInfo->szUsername) ? idsIdentityProperties : idsNewIdentity, szMsg, ARRAYSIZE(szMsg));
        SendMessage(hDlg, WM_SETTEXT, 0, (LPARAM)szMsg);

        SetDlgItemText(hDlg, idcUserName, sUserInfo->szUsername);
        SendMessage(GetDlgItem(hDlg, idcUserName), EM_LIMITTEXT, CCH_IDENTITY_NAME_MAX_LENGTH/2, 0);
        CheckDlgButton(hDlg, idcUsePwd, sUserInfo->fUsePassword ? BST_CHECKED : BST_UNCHECKED);
        EnableWindow(GetDlgItem(hDlg, idcChgPwd), sUserInfo->fUsePassword);

        // Don't allow zero length names by disabling OK
        if (!lstrlen(sUserInfo->szUsername))
            EnableWindow(GetDlgItem(hDlg, IDOK), FALSE);
        return TRUE;

    case WM_HELP:
    case WM_CONTEXTMENU:
        return OnContextHelp(hDlg, iMsg, wParam, lParam, g_rgCtxMapMultiUserGeneral);

    case WM_COMMAND:
        switch(LOWORD(wParam))
        {
        case IDOK:
            if (_ValidateChangeUserValues(hDlg, sUserInfo))
                MLEndDialogWrap(hDlg, IDOK);
            return true;

        case IDCANCEL:
            MLEndDialogWrap(hDlg, IDCANCEL);
            return true;

        case idcUserName:
            if (EN_CHANGE == HIWORD(wParam))
            {
                EnableWindow(GetDlgItem(hDlg, IDOK), SendMessage((HWND)lParam, WM_GETTEXTLENGTH, 0, 0) != 0);
                return TRUE;
            }
            break;
        
#ifdef IDENTITY_PASSWORDS
        case idcTellMeMore:
//            WinHelp ((HWND)GetDlgItem(hDlg, idcTellMeMore),
//                        c_szCtxHelpFile,
//                        HELP_WM_HELP,
//                        (DWORD_PTR)(LPVOID)g_rgCtxMapMultiUserGeneral);
            WinHelp(hDlg, c_szCtxHelpFile, HELP_CONTEXT, IDH_IDENTITY_TELLMEMORE_CONTENT);
            return true;

        case idcUsePwd:
            // if they are turning off the password, they need to confirm it first.
            if (!IsDlgButtonChecked(hDlg, idcUsePwd))
            {               
                strcpy(szPassword, sUserInfo->szPassword);
                MLLoadStringA(idsConfirmDisablePwd, szMsg, ARRAYSIZE(szMsg));
                if (!MU_ConfirmUserPassword(hDlg,szMsg, szPassword))
                    CheckDlgButton(hDlg, idcUsePwd, BST_CHECKED);
            }
            else
            {
                // if they are turning it on, they should set the password.
                if (EnterUserPassword(hDlg, szPassword))
                {
                    sUserInfo->fUsePassword = true;
                    strcpy(sUserInfo->szPassword, szPassword);
                }
                else
                {
                    CheckDlgButton(hDlg, idcUsePwd, BST_UNCHECKED);
                }
            }
            EnableWindow(GetDlgItem(hDlg, idcChgPwd), IsDlgButtonChecked(hDlg, idcUsePwd));
            return true;
        
        case idcChgPwd:
            if(sUserInfo->fUsePassword || (0 != *sUserInfo->szPassword))
            {
                strcpy(szPassword, sUserInfo->szPassword);
                
                if (ChangeUserPassword(hDlg, szPassword))
                    strcpy(sUserInfo->szPassword, szPassword);
            }
            return true;
#endif //IDENTITY_PASSWORDS
        }
        break;

    }
    return false;
}

/*
    MU_UserProperties

    Allow the user the change their username or password.
*/
BOOL        MU_UserProperties(HWND hwnd, LPUSERINFO lpUserInfo) 
{
    INT_PTR                 fResult;
    USERINFO                nuInfo;
    TCHAR                   szOldUsername[CCH_IDENTITY_NAME_MAX_LENGTH+1];
    USERINFO                uiCurrent;
    LPARAM                  lpNotify = IIC_CURRENT_IDENTITY_CHANGED;
    INITCOMMONCONTROLSEX    icex;

    Assert(hwnd);
    Assert(lpUserInfo);
    
    // get the current info so we know who to change later.
    MU_GetUserInfo(NULL, &nuInfo);    

    lstrcpy(szOldUsername, lpUserInfo->szUsername);

    // make sure ICC_NATIVEFNTCTL_CLASS is inited
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC  = ICC_NATIVEFNTCTL_CLASS;
    InitCommonControlsEx(&icex);

    fResult = MLDialogBoxParamWrap(MLGetHinst(), MAKEINTRESOURCEW(iddUserProperties), hwnd, _ChangeUserSettingsDlgProc, (LPARAM)lpUserInfo);

    if (IDOK == fResult)
    {
        if (GUID_NULL == lpUserInfo->uidUserID)
            _ClaimNextUserId(&lpUserInfo->uidUserID);

        MU_SetUserInfo(lpUserInfo);

        // if its not the current identity, then just broadcast that an identity changed
        if (MU_GetUserInfo(NULL, &uiCurrent) && (lpUserInfo->uidUserID != uiCurrent.uidUserID))
            lpNotify = IIC_IDENTITY_CHANGED;

        // if the name changd, tell other apps 
        if (lstrcmp(szOldUsername, lpUserInfo->szUsername) != 0)
            PostMessage(HWND_BROADCAST, WM_IDENTITY_INFO_CHANGED, 0, lpNotify);

    }
    
    return (IDOK == fResult);   
}



// ****************************************************************************************************
//  L   O   G   I   N       S   C   R   E   E   N
/*
    _ValidateLoginValues

    Validate the data entered by the user.  Return true only if everything is
    legit, 
*/
static BOOL _ValidateLoginValues(HWND  hDlg, 
                                    TCHAR*   lpszOldNewPassword)
{
    TCHAR    szUsername[255];
    TCHAR    szPW[255], szRealPW[CCH_USERPASSWORD_MAX_LENGTH];
    LRESULT dSelItem;
    BOOL    rResult = false;

    dSelItem = SendDlgItemMessage(hDlg, idcUserNameList, LB_GETCURSEL, 0, 0);
    if (LB_ERR != dSelItem)
    {
        if (SendDlgItemMessage(hDlg, idcUserNameList, LB_GETTEXTLEN, dSelItem, 0) < ARRAYSIZE(szUsername))
        {
            SendDlgItemMessage(hDlg, idcUserNameList, LB_GETTEXT, dSelItem, (LPARAM)szUsername);

#ifdef IDENTITY_PASSWORDS
            BOOL fUsePassword;
            if (MU_GetPasswordForUsername(szUsername, szRealPW, &fUsePassword))
            {
                if (fUsePassword)
                {
                    GetDlgItemText(hDlg,idcPwd,szPW, ARRAYSIZE(szPW));

                    if (strcmp(szPW, szRealPW) == 0)
                    {
                        strcpy(lpszOldNewPassword, szUsername);
                        rResult = true;
                    }
                    else
                    {
                        MU_ShowErrorMessage(hDlg, idsPwdDoesntMatch, idsPwdError);
                        SetFocus(GetDlgItem(hDlg,idcPwd));
                        SendDlgItemMessage(hDlg,idcPwd,EM_SETSEL,0,-1);
                        return false;
                    }
                }
                else    // if there is no password, then it does match up.
                {
                    strcpy(lpszOldNewPassword, szUsername);
                    rResult = true;
                }
            }
            else    //can't load identity password, do not allow access
            {
                MU_ShowErrorMessage(hDlg, idsPwdNotFound, idsPwdError);
                return false;
            }
#else  //IDENTITY_PASSWORDS
            strcpy(lpszOldNewPassword, szUsername);
            rResult = true;
#endif //IDENTITY_PASSWORDS
        }
    }
    return rResult;
}

static void _LoginEnableDisablePwdField(HWND hDlg)
{
#ifdef IDENTITY_PASSWORDS
    TCHAR    szUsername[255], szRealPW[255];
    BOOL    bEnabled = false;
#endif //IDENTITY_PASSWORDS
    LRESULT dSelItem;

    dSelItem = SendDlgItemMessage(hDlg, idcUserNameList, LB_GETCURSEL, 0, 0);
#ifdef IDENTITY_PASSWORDS
    if (LB_ERR != dSelItem)
    {
        if (SendDlgItemMessage(hDlg, idcUserNameList, LB_GETTEXTLEN, dSelItem, 0) < ARRAYSIZE(szUsername))
        {
            SendDlgItemMessage(hDlg, idcUserNameList, LB_GETTEXT, dSelItem, (LPARAM)szUsername);

            BOOL fUsePassword;
            if (MU_GetPasswordForUsername(szUsername, szRealPW, &fUsePassword) && fUsePassword)
            {
                bEnabled = true;
            }
        } 
    }
    EnableWindow(GetDlgItem(hDlg,idcPwd),bEnabled);
    EnableWindow(GetDlgItem(hDlg,idcPwdCaption),bEnabled);
#endif //IDENTITY_PASSWORDS

    EnableWindow(GetDlgItem(hDlg,IDOK),(dSelItem != -1));
}

typedef struct 
{
    TCHAR   *pszUsername;
    DWORD    dwFlags;
} LOGIN_PARAMS;

/*
    _LoginDlgProc

    Description: Dialog proc for handling the OE Login dialog.
*/
INT_PTR CALLBACK _LoginDlgProc(HWND       hDlg,
                                   UINT     iMsg, 
                                   WPARAM   wParam, 
                                   LPARAM   lParam)
{
    static TCHAR        *sResultUsername;
    static LOGIN_PARAMS *plpParams;
    TCHAR                szMsg[1024], szRes[1024];
    USERINFO            nuInfo;

    switch (iMsg)
    {
    case WM_INITDIALOG:
        Assert(lParam);
        
        plpParams = (LOGIN_PARAMS *)lParam;
        sResultUsername = plpParams->pszUsername;

        MLLoadStringA(!!(plpParams->dwFlags & UIL_FORCE_UI) ? idsSwitchIdentities : idsIdentityLogin, szMsg, ARRAYSIZE(szMsg));
        SendMessage(hDlg, WM_SETTEXT, 0, (LPARAM)szMsg);
        _FillListBoxWithUsernames(GetDlgItem(hDlg,idcUserNameList));
        
        if (MU_GetUserInfo(NULL, &nuInfo))
        {
            MLLoadStringA(idsLoginWithCurrent, szRes, ARRAYSIZE(szRes));
            wsprintf(szMsg, szRes, nuInfo.szUsername);
            SetDlgItemText(hDlg, idcWelcomeMsg, szMsg);

            MLLoadStringA(idsCurrIdentityInstr, szMsg, ARRAYSIZE(szMsg));
            SetDlgItemText(hDlg, idcLoginInstr, szMsg);
        }
        else
        {
            MLLoadStringA(idsLoginNoCurrent, szMsg, ARRAYSIZE(szMsg));
            SetDlgItemText(hDlg, idcWelcomeMsg, szMsg);
            MLLoadStringA(idsNoIdentityInstr, szMsg, ARRAYSIZE(szMsg));
            SetDlgItemText(hDlg, idcLoginInstr, szMsg);
        }


        if (sResultUsername[0] == 0)
            strcpy(sResultUsername, nuInfo.szUsername);

        if (sResultUsername[0])
        {
            LRESULT dFoundItem;
            
            dFoundItem = SendDlgItemMessage(hDlg, idcUserNameList, LB_FINDSTRING, 0, (LPARAM)sResultUsername);
            if (LB_ERR != dFoundItem)
            {
                SendDlgItemMessage(hDlg, idcUserNameList, LB_SETCURSEL, dFoundItem, 0);
            }
        }
        else
            SendDlgItemMessage(hDlg, idcUserNameList, LB_SETCURSEL, 0, 0);

        
        _LoginEnableDisablePwdField(hDlg);
        return TRUE;

    case WM_HELP:
    case WM_CONTEXTMENU:
        return OnContextHelp(hDlg, iMsg, wParam, lParam, g_rgCtxMapMultiUserGeneral);

    case WM_COMMAND:
        switch(HIWORD(wParam))
        {
            case LBN_DBLCLK:
                wParam = IDOK;
                break;
            case LBN_SELCHANGE:
                _LoginEnableDisablePwdField(hDlg);
                break;
        }

        switch(LOWORD(wParam))
        {
            case IDOK:
                if (_ValidateLoginValues(hDlg, sResultUsername))
                    MLEndDialogWrap(hDlg, IDOK);
                return true;

            case IDCANCEL:
                MLEndDialogWrap(hDlg, IDCANCEL);
                return true;

            case idcLogoff:
                MLLoadStringA(idsLogoff, sResultUsername, CCH_USERNAME_MAX_LENGTH);
                MLEndDialogWrap(hDlg, IDOK);
                return true;
                
            case idcManage:
                {
                    TCHAR   szUsername[CCH_USERNAME_MAX_LENGTH+1] = "";

                    MU_ManageUsers(hDlg, szUsername, 0);
                    _FillListBoxWithUsernames(GetDlgItem(hDlg,idcUserNameList));
                    SendDlgItemMessage(hDlg, idcUserNameList, LB_SETCURSEL, 0, 0);
                    _LoginEnableDisablePwdField(hDlg);

                    if (*szUsername)
                    {
                        lstrcpy(sResultUsername, szUsername);
                        MLEndDialogWrap(hDlg, IDOK);
                    }
                }
                return true;
        
        }
        break;

    }
    return false;
}


/*
    MU_Login

    Wrapper routine for logging in to OE.  Asks the user to choose a username
    and, if necessary, enter the password for that user.  The user can also
    create an account at this point.  

    lpszUsername should contain the name of the person who should be the default
    selection in the list.  If the name is empty ("") then it will look up the
    default from the registry.

    Returns the username that was selected in lpszUsername.  Returns true
    if that username is valid.
*/
BOOL        MU_Login(HWND hwnd, DWORD dwFlags, TCHAR *lpszUsername) 
{
    INT_PTR bResult;
    CStringList *csList;
    INITCOMMONCONTROLSEX    icex;

    Assert(hwnd);
    Assert(lpszUsername);
    
    // make sure ICC_NATIVEFNTCTL_CLASS is inited
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC  = ICC_NATIVEFNTCTL_CLASS;
    InitCommonControlsEx(&icex);

    csList = MU_GetUsernameList();

    // if there is only one username and they do not have a password, just return it.
    if (csList && csList->GetLength() == 1 && !(dwFlags & UIL_FORCE_UI))
    {
        TCHAR   *pszUsername;
        TCHAR   szPassword[255];
        BOOL    fUsePassword;
        pszUsername = csList->GetString(0);

        if(MU_GetPasswordForUsername(pszUsername, szPassword, &fUsePassword) && !fUsePassword)
        {
            lstrcpy(lpszUsername, pszUsername);
            delete csList;
            return TRUE;
        }
    }

    LOGIN_PARAMS lpParams;

    lpParams.dwFlags = dwFlags;
    lpParams.pszUsername = lpszUsername;
    bResult = MLDialogBoxParamWrap(MLGetHinst(), MAKEINTRESOURCEW(iddLogin), hwnd, _LoginDlgProc, (LPARAM)&lpParams);

    if (csList)
        delete csList;

    return (IDOK == bResult);   
}

void _ManagerUpdateButtons(HWND hDlg)
{
    LRESULT     dFoundItem;
    USERINFO    rUserInfo;
    GUID        uidDefaultId;

    // make sure that the delete button is only available if the
    // current user is not selected.
    dFoundItem = SendDlgItemMessage(hDlg, idcUserNameList, LB_GETCURSEL, 0, 0);
    if (dFoundItem != -1)
    {
        SendDlgItemMessage(hDlg, idcUserNameList, LB_GETTEXT, dFoundItem, (LPARAM)rUserInfo.szUsername); 
    
        MU_UsernameToUserId(rUserInfo.szUsername, &rUserInfo.uidUserID);
        MU_GetCurrentUserID(&uidDefaultId);

        // if there is no current user, don't allow deletion of the default user.
        if (GUID_NULL == uidDefaultId)
            MU_GetDefaultUserID(&uidDefaultId);
    }

    EnableWindow(GetDlgItem(hDlg, idcDelete), uidDefaultId != rUserInfo.uidUserID && dFoundItem != -1);
}

typedef struct 
{
    TCHAR   *pszUsername;
    DWORD    dwFlags;
} MANAGE_PARAMS;

/*
    _ManagerDlgProc

    Description: Dialog proc for handling the identity manager dialog.
*/
INT_PTR CALLBACK _ManagerDlgProc(HWND       hDlg,
                                   UINT     iMsg, 
                                   WPARAM   wParam, 
                                   LPARAM   lParam)
{
    USERINFO        rUserInfo;
    static MANAGE_PARAMS  *pmpParams;
    static TCHAR    sResultUsername[MAX_PATH] = "";
    static DWORD    sdwFlags = 0;
    LRESULT         dFoundItem;
    ULONG           uidUserId;
    HRESULT         hr;
    TCHAR           szRes[256];
    USERINFO        nuInfo;
    DWORD           dwIndex;
    GUID            uidDefault;
    switch (iMsg)
    {
    case WM_INITDIALOG:
        Assert(lParam);
        _ResetRememberedLoginOption();
        
        pmpParams = (MANAGE_PARAMS*)lParam;
        sdwFlags = pmpParams->dwFlags;

        _FillListBoxWithUsernames(GetDlgItem(hDlg,idcUserNameList));

        _FillComboBoxWithUsernames(GetDlgItem(hDlg,idcStartupCombo), GetDlgItem(hDlg,idcUserNameList));

        _FillComboBoxWithUsernames(GetDlgItem(hDlg,idcDefaultCombo), GetDlgItem(hDlg,idcUserNameList));

        dwIndex = MU_GetLoginOptionIndex(GetDlgItem(hDlg,idcStartupCombo));

        CheckDlgButton(hDlg, idcCheckDefault, dwIndex != ASK_BEFORE_LOGIN);
        EnableWindow(GetDlgItem(hDlg, idcStartupCombo), dwIndex != ASK_BEFORE_LOGIN);
        if (dwIndex != ASK_BEFORE_LOGIN)
            SendDlgItemMessage(hDlg, idcStartupCombo, CB_SETCURSEL, dwIndex, 0);
        else
            SendDlgItemMessage(hDlg, idcStartupCombo, CB_SETCURSEL, 0, 0);
        
        MU_GetUserInfo(NULL, &nuInfo);
        strcpy(szRes, nuInfo.szUsername);

        if (szRes[0])
        {
            dFoundItem = SendDlgItemMessage(hDlg, idcUserNameList, LB_FINDSTRING, 0, (LPARAM)szRes);
            if (LB_ERR != dFoundItem)
            {
                SendDlgItemMessage(hDlg, idcUserNameList, LB_SETCURSEL, dFoundItem, 0);
            }
        }

        SendDlgItemMessage(hDlg, idcDefaultCombo, CB_SETCURSEL, MU_GetDefaultOptionIndex(GetDlgItem(hDlg, idcDefaultCombo)), 0);
        
        _ManagerUpdateButtons(hDlg);
        if (!!(sdwFlags & UIMI_CREATE_NEW_IDENTITY))
        {
            ShowWindow(hDlg, SW_SHOW);
            PostMessage(hDlg, WM_COMMAND, idcAdd, 0);
        }
        return TRUE;

    case WM_HELP:
    case WM_CONTEXTMENU:
        return OnContextHelp(hDlg, iMsg, wParam, lParam, g_rgCtxMapMultiUserGeneral);

    case WM_COMMAND:
        switch(HIWORD(wParam))
        {
            case LBN_DBLCLK:
                wParam = idcProperties;
                break;
            case LBN_SELCHANGE:
                _ManagerUpdateButtons(hDlg);
                break;
        }

        switch(LOWORD(wParam))
        {
            case IDCANCEL:
            case idcClose:
            case IDOK:
                dFoundItem = SendDlgItemMessage(hDlg, idcStartupCombo, CB_GETCURSEL, 0, 0);
                if (CB_ERR == dFoundItem)
                    dFoundItem = 0;

                if (IsDlgButtonChecked(hDlg, idcCheckDefault))
                    MU_SetLoginOption(GetDlgItem(hDlg,idcStartupCombo), dFoundItem);
                else
                    MU_SetLoginOption(GetDlgItem(hDlg,idcStartupCombo), ASK_BEFORE_LOGIN);

                dFoundItem = SendDlgItemMessage(hDlg, idcDefaultCombo, CB_GETCURSEL, 0, 0);
                if (CB_ERR == dFoundItem)
                    dFoundItem = 0;

                SendDlgItemMessage(hDlg, idcUserNameList, LB_GETTEXT, dFoundItem, (LPARAM)rUserInfo.szUsername); 
                hr = MU_UsernameToUserId(rUserInfo.szUsername, &rUserInfo.uidUserID);
                Assert(SUCCEEDED(hr));

                MU_MakeDefaultUser(&rUserInfo.uidUserID);
                MLEndDialogWrap(hDlg, IDOK);
                return true;

            case idcAdd:
                ZeroMemory(&rUserInfo, sizeof(USERINFO));

                if (MU_UserProperties(hDlg,&rUserInfo))
                {
                    TCHAR   szMsg[ARRAYSIZE(szRes) + CCH_IDENTITY_NAME_MAX_LENGTH];
                    
                    // rebuild the username list and select the newly added one
                    _RememberLoginOption(GetDlgItem(hDlg,idcStartupCombo));
                    strcpy(sResultUsername, rUserInfo.szUsername);
                    _FillListBoxWithUsernames(GetDlgItem(hDlg,idcUserNameList));
                    _FillComboBoxWithUsernames(GetDlgItem(hDlg,idcStartupCombo), GetDlgItem(hDlg,idcUserNameList));
                    _FillComboBoxWithUsernames(GetDlgItem(hDlg,idcDefaultCombo), GetDlgItem(hDlg,idcUserNameList));

                    dwIndex = MU_GetLoginOptionIndex(GetDlgItem(hDlg,idcStartupCombo));
                    SendDlgItemMessage(hDlg, idcStartupCombo, CB_SETCURSEL,(dwIndex == ASK_BEFORE_LOGIN ? 0 : dwIndex) , 0);
                    SendDlgItemMessage(hDlg, idcDefaultCombo, CB_SETCURSEL, MU_GetDefaultOptionIndex(GetDlgItem(hDlg, idcDefaultCombo)), 0);

                    dFoundItem = SendDlgItemMessage(hDlg, idcUserNameList, LB_FINDSTRING, 0, (LPARAM)sResultUsername);
                    if (LB_ERR != dFoundItem)
                    {
                        SendDlgItemMessage(hDlg, idcUserNameList, LB_SETCURSEL, dFoundItem, 0);
                    }
                    PostMessage(HWND_BROADCAST, WM_IDENTITY_INFO_CHANGED, 0, IIC_IDENTITY_ADDED);

                    if (pmpParams->pszUsername)
                    {
                        MLLoadStringA(idsLoginAsUser, szRes, ARRAYSIZE(szRes));
                        wsprintf(szMsg, szRes, rUserInfo.szUsername);

                        MLLoadStringA(idsUserAdded, szRes, ARRAYSIZE(szRes));
                        if (IDYES == MessageBox(hDlg, szMsg, szRes, MB_YESNO))
                        {
                            lstrcpy(pmpParams->pszUsername, rUserInfo.szUsername);
                            PostMessage(hDlg, WM_COMMAND, idcClose, 0);
                        }
                    }
                }
                _ManagerUpdateButtons(hDlg);
                return true;

            case idcDefaultCombo:
                dFoundItem = SendDlgItemMessage(hDlg, idcDefaultCombo, CB_GETCURSEL, 0, 0);
                if (CB_ERR == dFoundItem)
                    dFoundItem = 0;

                SendDlgItemMessage(hDlg, idcUserNameList, LB_GETTEXT, dFoundItem, (LPARAM)rUserInfo.szUsername); 
                hr = MU_UsernameToUserId(rUserInfo.szUsername, &rUserInfo.uidUserID);
                Assert(SUCCEEDED(hr));

                MU_MakeDefaultUser(&rUserInfo.uidUserID);
                _ManagerUpdateButtons(hDlg);
                break;

            case idcCheckDefault:
                EnableWindow(GetDlgItem(hDlg, idcStartupCombo), IsDlgButtonChecked(hDlg, idcCheckDefault));
                return true;

            case idcDelete:
                dFoundItem = SendDlgItemMessage(hDlg, idcUserNameList, LB_GETCURSEL, 0, 0);
                SendDlgItemMessage(hDlg, idcUserNameList, LB_GETTEXT, dFoundItem, (LPARAM)rUserInfo.szUsername); 

                hr = MU_UsernameToUserId(rUserInfo.szUsername, &rUserInfo.uidUserID);
                Assert(SUCCEEDED(hr));

                if (MU_ConfirmDeleteUser(hDlg, rUserInfo.szUsername))
                {
                    MU_DeleteUser(&rUserInfo.uidUserID);
                    _RememberLoginOption(GetDlgItem(hDlg,idcStartupCombo));
                    _FillListBoxWithUsernames(GetDlgItem(hDlg,idcUserNameList));
                    _FillComboBoxWithUsernames(GetDlgItem(hDlg,idcStartupCombo), GetDlgItem(hDlg,idcUserNameList));
                    _FillComboBoxWithUsernames(GetDlgItem(hDlg,idcDefaultCombo), GetDlgItem(hDlg,idcUserNameList));

                    dwIndex = MU_GetLoginOptionIndex(GetDlgItem(hDlg,idcStartupCombo));
                    SendDlgItemMessage(hDlg, idcStartupCombo, CB_SETCURSEL,(dwIndex == ASK_BEFORE_LOGIN ? 0 : dwIndex) , 0);
                    SendDlgItemMessage(hDlg, idcDefaultCombo, CB_SETCURSEL, MU_GetDefaultOptionIndex(GetDlgItem(hDlg, idcDefaultCombo)), 0);
                    _ManagerUpdateButtons(hDlg);
                }
                return true;

            case idcProperties:
                dFoundItem = SendDlgItemMessage(hDlg, idcUserNameList, LB_GETCURSEL, 0, 0);
                SendDlgItemMessage(hDlg, idcUserNameList, LB_GETTEXT, dFoundItem, (LPARAM)rUserInfo.szUsername); 

                hr = MU_UsernameToUserId(rUserInfo.szUsername, &rUserInfo.uidUserID);
                Assert(SUCCEEDED(hr));

#ifdef IDENTITY_PASSWORDS
                if (SUCCEEDED(hr) && MU_GetUserInfo(&rUserInfo.uidUserID, &rUserInfo) && MU_CanEditIdentity(hDlg, &rUserInfo.uidUserID))
#else
                if (SUCCEEDED(hr) && MU_GetUserInfo(&rUserInfo.uidUserID, &rUserInfo))
#endif //IDENTITY_PASSWORDS

                {
                    if (MU_UserProperties(hDlg,&rUserInfo))
                    {
                        // rebuild the username list and select the newly added one
                        _RememberLoginOption(GetDlgItem(hDlg,idcStartupCombo));
                        strcpy(sResultUsername, rUserInfo.szUsername);
                        _FillListBoxWithUsernames(GetDlgItem(hDlg,idcUserNameList));
                        _FillComboBoxWithUsernames(GetDlgItem(hDlg,idcStartupCombo), GetDlgItem(hDlg,idcUserNameList));
                        _FillComboBoxWithUsernames(GetDlgItem(hDlg,idcDefaultCombo), GetDlgItem(hDlg,idcUserNameList));

                        dwIndex = MU_GetLoginOptionIndex(GetDlgItem(hDlg,idcStartupCombo));
                        SendDlgItemMessage(hDlg, idcStartupCombo, CB_SETCURSEL,(dwIndex == ASK_BEFORE_LOGIN ? 0 : dwIndex) , 0);
                        SendDlgItemMessage(hDlg, idcDefaultCombo, CB_SETCURSEL, MU_GetDefaultOptionIndex(GetDlgItem(hDlg, idcDefaultCombo)), 0);

                        dFoundItem = SendDlgItemMessage(hDlg, idcUserNameList, LB_FINDSTRING, 0, (LPARAM)sResultUsername);
                        if (LB_ERR != dFoundItem)
                        {
                            SendDlgItemMessage(hDlg, idcUserNameList, LB_SETCURSEL, dFoundItem, 0);
                        }
                    }
                }
                _ManagerUpdateButtons(hDlg);
                break;
/*          
            case idcDefault:
                dFoundItem = SendDlgItemMessage(hDlg, idcUserNameList, LB_GETCURSEL, 0, 0);
                SendDlgItemMessage(hDlg, idcUserNameList, LB_GETTEXT, dFoundItem, (LPARAM)rUserInfo.szUsername); 
//                _StripDefault(rUserInfo.szUsername);

                hr = MU_UsernameToUserId(rUserInfo.szUsername, &rUserInfo.uidUserID);
                Assert(SUCCEEDED(hr));

                MU_MakeDefaultUser(&rUserInfo.uidUserID);
                _RememberLoginOption(GetDlgItem(hDlg,idcStartupCombo));
                _FillListBoxWithUsernames(GetDlgItem(hDlg,idcUserNameList));
                _FillComboBoxWithUsernames(GetDlgItem(hDlg,idcStartupCombo), GetDlgItem(hDlg,idcUserNameList));

                SendDlgItemMessage(hDlg, idcStartupCombo, CB_SETCURSEL, MU_GetLoginOptionIndex(GetDlgItem(hDlg,idcStartupCombo)), 0);
                SendDlgItemMessage(hDlg, idcUserNameList, LB_SETCURSEL, dFoundItem, 0);
                _ManagerUpdateButtons(hDlg);
                break;
*/
        }
        break;

    }
    return false;
}

/*
    MU_ManageUsers
*/
BOOL        MU_ManageUsers(HWND hwnd, TCHAR *lpszSwitchtoUsername, DWORD dwFlags) 
{
    INT_PTR         bResult;
    MANAGE_PARAMS   rParams;
    INITCOMMONCONTROLSEX    icex;

    Assert(hwnd);
    Assert(lpszUsername);
    
    // make sure ICC_NATIVEFNTCTL_CLASS is inited
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC  = ICC_NATIVEFNTCTL_CLASS;
    InitCommonControlsEx(&icex);

    rParams.dwFlags = dwFlags;
    rParams.pszUsername = lpszSwitchtoUsername;

    bResult = MLDialogBoxParamWrap(MLGetHinst(), MAKEINTRESOURCEW(iddManager), hwnd, _ManagerDlgProc, (LPARAM)&rParams);

    return (IDOK == bResult);   
}
