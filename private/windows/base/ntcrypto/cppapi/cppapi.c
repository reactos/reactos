/////////////////////////////////////////////////////////////////////////////
//  FILE          : cppapi.c                                               //
//  DESCRIPTION   : Cryptography Provider Private APIs                     //
//  AUTHOR        :                                                        //
//  HISTORY       :                                                        //
//  May  9 1995 larrys  New                                                //
//                                                                         //
//  Copyright (C) 1993 Microsoft Corporation   All Rights Reserved         //
/////////////////////////////////////////////////////////////////////////////

#include <tchar.h>
#include "precomp.h"
#include "ntagimp1.h"
#include "cppapi.h"
#include "resource.h"

#define MAX_STRING_RSC_SIZE			512
#define	MAX_AT_KEY_RSC_SIZE			32

#define SIGN_TXT                    1
#define EXPORT_PRIV_TXT             2
#define GENKEY_NO_OVER_TXT          3
#define GENKEY_OVER_TXT             4
#define IMPORT_SIMPLE_TXT           5
#define IMPORT_PRIV_NO_OVER_TXT     6
#define IMPORT_PRIV_OVER_TXT        7
#define SETKEYPARAM_CHGPWD_TXT      8

extern HINSTANCE hInstance;

typedef struct _DIALOG_INFO
{
    DWORD	dwDialogType;
    LPCTSTR szContainer;
    DWORD	dwKeySpec;
} DIALOG_INFO, *PDIALOG_INFO;

DWORD GetShortFileName(
			HINSTANCE hInstance,
			LPTSTR szText,
			DWORD dwSize)
{
	DWORD dwText;
    TCHAR *psz;
    int i;
	dwText = GetModuleFileName(
		hInstance,
		szText,
		dwSize);

	psz = szText+dwText;		

	for (i=0; ( (*psz != '\\') && (psz != szText)); i++)
		psz--;

	// don't copy the first \, instead copy the end \n 
	MoveMemory(szText, psz+1, i*sizeof(TCHAR));
	return (i*sizeof(TCHAR));
}

/*
void WriteStuff(LPCSTR sz, DWORD dw, BOOL f)
{
    HANDLE hFile;
    DWORD dw1;
    BOOL fResult;

    if( INVALID_HANDLE_VALUE == (hFile = CreateFile("stuff", GENERIC_WRITE,
	    FILE_SHARE_READ | FILE_SHARE_WRITE,
	    NULL, OPEN_ALWAYS, 0, NULL)))
        goto Ret;
    if (f)
    {
	    if (!(fResult = WriteFile(hFile, sz, strlen(sz), &dw1, NULL)))
	        goto Ret;
    }
    else
    {
	    if (!(fResult = WriteFile(hFile, &dw, sizeof(DWORD), &dw1, NULL)))
	        goto Ret;
    }
Ret:
    if (hFile)
	    CloseHandle(hFile);
}
*/
#ifdef _USE_UI

BOOL GetDialogInfo(
                   IN PNTAGUserList pTmpUser,
                   IN DWORD dwDialogType,
                   IN DWORD dwKeySpec,
                   IN OUT PDIALOG_INFO pInfo
                   )
{
    pInfo->dwDialogType = dwDialogType;
    pInfo->dwKeySpec = dwKeySpec;
    pInfo->szContainer = pTmpUser->szUserName;
    return TRUE;
}

LRESULT CALLBACK DialogSolicitPassword(
                                       HWND hDlg,
                                       UINT message,
                                       WPARAM wParam,
                                       LPARAM lParam
                                       )
{
	TCHAR	szText[MAX_STRING_RSC_SIZE];
    TCHAR	szKeyType[MAX_AT_KEY_RSC_SIZE];
	TCHAR	szCallerName[MAX_PATH];
	TCHAR*	pszFinalText = NULL;
	TCHAR	szBoxText[MAX_STRING_RSC_SIZE];

	LPCTSTR list[3];

	UINT	uiKeyText = 0;
	UINT	uiBoxText = 0;
	PDIALOG_INFO pDlgInfo = (PDIALOG_INFO)lParam;

	switch (message)
    {
        case WM_INITDIALOG:
            ShowWindow (hDlg, SW_HIDE);

			switch(pDlgInfo->dwDialogType)
            {
                case SIGN_TXT:
					uiBoxText = IDS_SIGN_BOXTEXT;
                break;
                case EXPORT_PRIV_TXT:
					uiBoxText = IDS_EXPORT_PRIVATE_BOXTEXT;
                break;
                case IMPORT_SIMPLE_TXT:
					uiBoxText = IDS_IMPORT_SIMPLE_BOXTEXT;
                break;
            }

			// set boxtext
			{
				// Key type: KeyX or Sig?
				if (AT_SIGNATURE == pDlgInfo->dwKeySpec)
					uiKeyText = IDS_AT_SIGNATURE;
				else
					uiKeyText = IDS_AT_KEYEXCHANGE;

				LoadString(
						hInstance, 
						uiKeyText,
						szKeyType, 
						MAX_AT_KEY_RSC_SIZE);

				// Caller?
				GetShortFileName(
						NULL,
						szCallerName,
						MAX_PATH);

				LoadString(
						hInstance, 
						uiBoxText,
						szBoxText, 
						MAX_STRING_RSC_SIZE);

				list[0]=szCallerName;
				list[1]=pDlgInfo->szContainer;
				list[2]=szKeyType;

				if (0 != FormatMessage(
						FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ARGUMENT_ARRAY,	
						szBoxText,
						0,
					    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), 
						(LPTSTR)&pszFinalText,	
						0,	
						(va_list*)&list
					   ))
				{
					SetWindowText(GetDlgItem(hDlg, IDC_BOXTEXT), pszFinalText);
					LocalFree(pszFinalText);
				}
			}

			// titlebar
			if (0 != LoadString(
					hInstance,
					IDS_SOLICIT_PASSWORD_TITLE,
					szText,
					MAX_STRING_RSC_SIZE))
				SetWindowText (hDlg, szText);

			// prompts
			if (0 != LoadString(
					hInstance,
					IDS_PASSWORD_PROMPT,
					szText, 
					MAX_STRING_RSC_SIZE))
				SetWindowText(GetDlgItem(hDlg, IDC_PASSWORD_STATICTEXT), szText);

			ShowWindow (hDlg, SW_SHOW);
			return (TRUE);

      case WM_COMMAND:
         if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
         {
            EndDialog(hDlg, (LOWORD(wParam) == IDOK));
            return TRUE;
         }
         break;
   }

    return FALSE;
}

LRESULT CALLBACK DialogNewPassword(
                                   HWND hDlg,
                                   UINT message,
                                   WPARAM wParam,
                                   LPARAM lParam
                                   )
{
    TCHAR	szText[MAX_STRING_RSC_SIZE];
    TCHAR	szKeyType[MAX_AT_KEY_RSC_SIZE];
	TCHAR	szCallerName[MAX_PATH];
	TCHAR*	pszFinalText = NULL;
	TCHAR	szBoxText[MAX_STRING_RSC_SIZE];

	LPCTSTR list[3];

	UINT	uiBoxText = 0;
	UINT	uiKeyText = 0;
	PDIALOG_INFO pDlgInfo = (PDIALOG_INFO)lParam;

    DWORD dw;

    switch (message)
    {
        case WM_INITDIALOG:
            ShowWindow (hDlg, SW_HIDE);

			switch(pDlgInfo->dwDialogType)
            {
                case GENKEY_NO_OVER_TXT:
					uiBoxText = IDS_GENKEY_BOXTEXT;
                break;
                case IMPORT_PRIV_NO_OVER_TXT:
					uiBoxText = IDS_IMPORT_PRIVATE_BOXTEXT;
                break;
            }
            
			// set boxtext
			{
				// Key type: KeyX or Sig?
				if (AT_SIGNATURE == pDlgInfo->dwKeySpec)
					uiKeyText = IDS_AT_SIGNATURE;
				else
					uiKeyText = IDS_AT_KEYEXCHANGE;

				LoadString(
						hInstance, 
						uiKeyText,
						szKeyType, 
						MAX_AT_KEY_RSC_SIZE);


				// Caller?
				GetShortFileName(
						NULL,
						szCallerName,
						MAX_PATH);

				LoadString(
						hInstance, 
						uiBoxText,
						szBoxText, 
						MAX_STRING_RSC_SIZE);

				list[0]=szCallerName;
				list[1]=pDlgInfo->szContainer;
				list[2]=szKeyType;

				if (0 != FormatMessage(
						FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ARGUMENT_ARRAY,	
						szBoxText,
						0,
					    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), 
						(LPTSTR)&pszFinalText,	
						0,	
						(va_list*)&list
					   ))
                {

					SetWindowText(GetDlgItem(hDlg, IDC_BOXTEXT), pszFinalText);
					LocalFree(pszFinalText);
                }
			}

			// titlebar
			if (0 != LoadString(
					hInstance,
					IDS_NEW_PASSWORD_TITLE,
					szText,
					MAX_STRING_RSC_SIZE))
				SetWindowText (hDlg, szText);

			// prompts
			if (0 != LoadString(
					hInstance,
					IDS_PASSWORD_PROMPT,
					szText, 
					MAX_STRING_RSC_SIZE))
				SetWindowText(GetDlgItem(hDlg, IDC_PASSWORD_STATICTEXT), szText);

			if (0 != LoadString(
					hInstance,
					IDS_CONFIRM_PASSWORD_PROMPT,
					szText, 
					MAX_STRING_RSC_SIZE))
				SetWindowText(GetDlgItem(hDlg, IDC_CONFIRM_PASSWORD_STATICTEXT), szText);

			ShowWindow (hDlg, SW_SHOW);
            return (TRUE);

        case WM_COMMAND:
            if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
            {
                EndDialog(hDlg, (LOWORD(wParam) == IDOK));
                return TRUE;
            }
            break;
    }

    return FALSE;
}

LRESULT CALLBACK DialogOverwritePassword(
                                         HWND hDlg,
                                         UINT message,
                                         WPARAM wParam,
                                         LPARAM lParam
                                         )
{
    TCHAR	szText[MAX_STRING_RSC_SIZE];
    TCHAR	szKeyType[MAX_AT_KEY_RSC_SIZE];
	TCHAR	szCallerName[MAX_PATH];
	TCHAR*	pszFinalText = NULL;
	TCHAR	szBoxText[MAX_STRING_RSC_SIZE];

	LPCTSTR list[3];

	UINT	uiKeyText = 0;
	UINT	uiBoxText = 0;
	PDIALOG_INFO pDlgInfo = (PDIALOG_INFO)lParam;

    switch (message)
    {
        case WM_INITDIALOG:
            ShowWindow (hDlg, SW_HIDE);

			switch(pDlgInfo->dwDialogType)
            {
                case SETKEYPARAM_CHGPWD_TXT:
					uiBoxText = IDS_CHANGEPASS_BOXTEXT;
                break;
                case GENKEY_OVER_TXT:
					uiBoxText = IDS_GENKEY_OW_BOXTEXT;
                break;
                case IMPORT_PRIV_OVER_TXT:
					uiBoxText = IDS_IMPORT_PRIVATE_OW_BOXTEXT;
                break;
            }

			// set boxtext
			{
				// Key type: KeyX or Sig?
				if (AT_SIGNATURE == pDlgInfo->dwKeySpec)
					uiKeyText = IDS_AT_SIGNATURE;
				else
					uiKeyText = IDS_AT_KEYEXCHANGE;

				LoadString(
						hInstance, 
						uiKeyText,
						szKeyType, 
						MAX_AT_KEY_RSC_SIZE);

				// Caller?
				GetShortFileName(
						NULL,
						szCallerName,
						MAX_PATH);

				LoadString(
						hInstance, 
						uiBoxText,
						szBoxText, 
						MAX_STRING_RSC_SIZE);

				list[0]=szCallerName;
				list[1]=pDlgInfo->szContainer;
				list[2]=szKeyType;

				if (0 != FormatMessage(
						FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ARGUMENT_ARRAY,	
						szBoxText,
						0,
					    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), 
						(LPTSTR)&pszFinalText,	
						0,	
						(va_list*)&list
					   ))
				{
					SetWindowText(GetDlgItem(hDlg, IDC_BOXTEXT), pszFinalText);
					LocalFree(pszFinalText);
				}
			}

			// titlebar
			if (0 != LoadString(
					hInstance,
					IDS_OVERWRITE_PASSWORD_TITLE,
					szText,
					MAX_STRING_RSC_SIZE))
				SetWindowText (hDlg, szText);

			// prompts
			if (0 != LoadString(
					hInstance,
					IDS_PASSWORD_PROMPT,
					szText, 
					MAX_STRING_RSC_SIZE))
				SetWindowText(GetDlgItem(hDlg, IDC_PASSWORD_STATICTEXT), szText);

			if (0 != LoadString(
					hInstance,
					IDS_CONFIRM_PASSWORD_PROMPT,
					szText, 
					MAX_STRING_RSC_SIZE))
				SetWindowText(GetDlgItem(hDlg, IDC_CONFIRM_PASSWORD_STATICTEXT), szText);

			if (0 != LoadString(
					hInstance,
					IDS_OLD_PASSWORD_PROMPT,
					szText, 
					MAX_STRING_RSC_SIZE))
				SetWindowText(GetDlgItem(hDlg, IDC_OLD_PASSWORD_STATICTEXT), szText);

            ShowWindow (hDlg, SW_SHOW);
            return (TRUE);

        case WM_COMMAND:
            if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
            {
                EndDialog(hDlg, (LOWORD(wParam) == IDOK));
                return TRUE;
            }
            break;
    }

    return FALSE;
}

#endif	// _USE_UI

/*
 -  CryptLogonVerify
 -
 *  Purpose:
 *                Used by CryptAcquireContext to verify logon password.
 *
 *
 *  Parameters:
 *               OUT     hPrivid -  Handle to the id of the user
 *
 *  Returns:
 */
BOOL CryptLogonVerify(OUT HPRIVUID *hUID)
{
#ifdef MESSAGEBOXES
    int     ret;

    ret = MessageBox(NULL, "Type Password", "CryptLogonVerify", MB_OK);

    *hUID = 0xdeadbeef;
#endif

    return CPPAPI_SUCCEED;

}

/*
 -  CryptGetUserData
 -
 *  Purpose:
 *                Get required data from user.
 *
 *
 *  Parameters:
 *               IN      hPrivid  -  Handle to the id of the user
 *               OUT     pbData   -  bufer containing user-supplied data
 *               OUT     dwBufLen -  lenght of user-supplied data
 *
 *  Returns:
 */
BOOL CryptGetUserData(IN  HPRIVUID hUID,
                      OUT BYTE **pbData,
                      OUT DWORD *dwBufLen)
{
#ifdef MESSAGEBOXES
    int     ret;
    BYTE    *pbTmp;

    ret = MessageBox(NULL, "Type User Data", "CryptGetUserData", MB_OK);

    if (hUID !=  0xdeadbeef)
    {
	SetLastError(NTE_BAD_UID);
	return CPPAPI_FAILED;
    }

    *dwBufLen = strlen("This is a string to hash");
    if ((pbTmp = (BYTE *) malloc(*dwBufLen)) == NULL)
    {
	SetLastError(NTE_NO_MEMORY);
	return CPPAPI_FAILED;
    }

    strcpy(pbTmp, "This is a string to hash");

    *pbData = pbTmp;
#endif

    return CPPAPI_SUCCEED;

}

/*
 -  CryptConfirmSignature
 -
 *  Purpose:
 *                Determine weather the signing should proceed.
 *
 *
 *  Parameters:
 *               IN      pTmpUser     -  Pointer to the user list structure
 *               IN      dwKeySpec    -  Type of key to be used for signing
 *               IN      sDescription -  Description of document to be signed
 *
 *  Returns:
 */
BOOL CryptConfirmSignature(
                           IN PNTAGUserList pTmpUser,
                           IN DWORD dwKeySpec,
                           IN LPCTSTR sDescription
                           )
{
    DWORD   dwResult = TRUE;

#ifdef _USE_UI
    DIALOG_INFO Info;

    if (!GetDialogInfo(pTmpUser, SIGN_TXT, dwKeySpec, &Info))
        dwResult = FALSE;
    else
    {
        if (-1 == (dwResult = DialogBoxParam(hInstance,
                                             MAKEINTRESOURCE(IDD_SOLICIT_PASSWORD),
                                             pTmpUser->hWnd,
                                             (DLGPROC)DialogSolicitPassword,
                                             (long)&Info)))
            dwResult = FALSE;
    }
#endif // _USE_UI
#ifdef MESSAGEBOXES
    int     ret;

    if (pTmpUser->hUID !=  0xdeadbeef)
    {
	SetLastError(NTE_BAD_UID);
	return CPPAPI_FAILED;
    }

    if (strcmp(sDescription, "signed") != 0)
    {
	SetLastError(NTE_BAD_SIGNATURE);
	return CPPAPI_FAILED;
    }

    ret = MessageBox(NULL, "Type password", "CryptConfirmSignature", MB_OK);
#endif

    return dwResult;

}

/*
 -  CryptUserProtectKey
 -
 *  Purpose:      Obtain or determine user protection information.
 *                
 *
 *
 *  Parameters:
 *               IN      hPrivid      -  Handle to the id of the user
 *               IN      hKey         -  Handle to key
 *
 *  Returns:
 */
BOOL CryptUserProtectKey(IN HPRIVUID hUID,
                         IN HCRYPTKEY hKey)
{

#ifdef MESSAGEBOXES
    int     ret;

    if (hUID !=  0xdeadbeef)
    {
	SetLastError(NTE_BAD_UID);
	return CPPAPI_FAILED;
    }

    if (hKey == 0)
    {
	SetLastError(NTE_BAD_KEY);
	return CPPAPI_FAILED;
    }

    ret = MessageBox(NULL, "Type password", "CryptUserProtectKey", MB_OK);
#endif

    return CPPAPI_SUCCEED;

}


/*
 -  CryptConfirmEncryption
 -
 *  Purpose:
 *                Determine weather the encryption should proceed.
 *
 *
 *  Parameters:
 *               IN      hPrivid      -  Handle to the id of the user
 *               IN      hKey         -  Handle to key
 *               IN      final        -  flag indicating last encrypt for a
 *                                       block of data
 *
 *  Returns:
 */
BOOL CryptConfirmEncryption(IN HPRIVUID hUID,
                            IN HCRYPTKEY hKey,
                            IN BOOL final)
{

#ifdef MESSAGEBOXES
    int     ret;

    if (hUID !=  0xdeadbeef)
    {
	SetLastError(NTE_BAD_UID);
	return CPPAPI_FAILED;
    }

    if (hKey == 0)
    {
	SetLastError(NTE_BAD_KEY);
	return CPPAPI_FAILED;
    }

    ret = MessageBox(NULL, "Type password", "CryptConfirmEncryption", MB_OK);
#endif

    return CPPAPI_SUCCEED;

}


/*
 -  CryptConfirmDecryption
 -
 *  Purpose:
 *                Determine weather the DEcryption should proceed.
 *
 *
 *  Parameters:
 *               IN      hPrivid      -  Handle to the id of the user
 *               IN      hKey         -  Handle to key
 *               IN      final        -  flag indicating last encrypt for a
 *                                       block of data
 *
 *  Returns:
 */
BOOL CryptConfirmDecryption(IN HPRIVUID hUID,
                            IN HCRYPTKEY hKey,
                            IN BOOL final)
{
#ifdef MESSAGEBOXES
    int     ret;

    if (hUID !=  0xdeadbeef)
    {
	SetLastError(NTE_BAD_UID);
	return CPPAPI_FAILED;
    }

    if (hKey == 0)
    {
	SetLastError(NTE_BAD_KEY);
	return CPPAPI_FAILED;
    }

    ret = MessageBox(NULL, "Type password", "CryptConfirmDecryption", MB_OK);
#endif

    return CPPAPI_SUCCEED;

}


/*
 -  CryptConfirmTranslation
 -
 *  Purpose:
 *                Determine weather the translation should proceed.
 *
 *
 *  Parameters:
 *               IN      hPrivid      -  Handle to the id of the user
 *               IN      hKey         -  Handle to key
 *               IN      final        -  flag indicating last encrypt for a
 *                                       block of data
 *
 *  Returns:
 */
BOOL CryptConfirmTranslation(IN HPRIVUID hUID,
                             IN HCRYPTKEY hKey,
                             IN BOOL final)
{

#ifdef MESSAGEBOXES
    int     ret;

    if (hUID !=  0xdeadbeef)
    {
	SetLastError(NTE_BAD_UID);
	return CPPAPI_FAILED;
    }

    if (hKey == 0)
    {
	SetLastError(NTE_BAD_KEY);
	return CPPAPI_FAILED;
    }

    ret = MessageBox(NULL, "Type password", "CryptConfirmTranslation", MB_OK);
#endif

    return CPPAPI_SUCCEED;

}


/*
 -  CryptConfirmExportKey
 -
 *  Purpose:
 *                Determine whether the export key should proceed.
 *
 *
 *  Parameters:
 *               IN      pTmpUser     -  Pointer to the user list structure
 *               IN      dwKeySpec    -  Type of key to be exported
 *
 *  Returns:
 */
BOOL CryptConfirmExportKey(
                           IN PNTAGUserList pTmpUser,
                           IN DWORD dwKeySpec
                           )
{
    DWORD   dwResult = TRUE;

#ifdef _USE_UI
    DIALOG_INFO Info;

    if (!GetDialogInfo(pTmpUser, EXPORT_PRIV_TXT, dwKeySpec, &Info))
        dwResult = FALSE;
    else
    {
        if (-1 == (dwResult = DialogBoxParam(hInstance,
                                             MAKEINTRESOURCE(IDD_SOLICIT_PASSWORD),
                                             pTmpUser->hWnd,
                                             (DLGPROC)DialogSolicitPassword,
                                             (long)&Info)))
            dwResult = FALSE;
    }
#endif // _USE_UI
#ifdef MESSAGEBOXES
    int     ret;

    if (pTmpUser->hUID !=  0xdeadbeef)
    {
	SetLastError(NTE_BAD_UID);
	return CPPAPI_FAILED;
    }

    if (hKey == 0)
    {
	SetLastError(NTE_BAD_KEY);
	return CPPAPI_FAILED;
    }

    ret = MessageBox(NULL, "Type password", "CryptConfirmExportKey", MB_OK);
#endif

    return dwResult;
}

/*
 -  CryptConfirmImportKey
 -
 *  Purpose:
 *                Determine whether the import key should proceed.
 *
 *
 *  Parameters:
 *               IN      pTmpUser     -  Pointer to the user list structure
 *               IN      pKey         -  Pointer to the key list structure
 *               IN      dwBlobType   -  Type of blob to be imported
 *               IN      dwKeySpec    -  Type of key to be imported
 *
 *  Returns:
 */
BOOL CryptConfirmImportKey(
                           IN PNTAGUserList pTmpUser,
                           IN DWORD dwBlobType,
                           IN DWORD dwKeySpec
                           )
{
    BOOL    fOverwrite = FALSE;
    DWORD   dwResult = TRUE;

#ifdef _USE_UI
    DIALOG_INFO Info;

    if (PRIVATEKEYBLOB == dwBlobType)
    {
        if (AT_SIGNATURE == dwKeySpec)
        {
            if (pTmpUser->SigPrivLen)
                fOverwrite = TRUE;
        }
        else
        {
            if (pTmpUser->ExchPrivLen)
                fOverwrite = TRUE;
        }

        if (fOverwrite)
        {
            if (!GetDialogInfo(pTmpUser, IMPORT_PRIV_OVER_TXT, dwKeySpec, &Info))
                dwResult = FALSE;
            else
            {
                if (-1 == (dwResult = DialogBoxParam(hInstance,
                                                     MAKEINTRESOURCE(IDD_OVERWRITE_PASSWORD),
                                                     pTmpUser->hWnd,
                                                     (DLGPROC)DialogOverwritePassword,
                                                     (long)&Info)))
                    dwResult = FALSE;
            }
        }
        else
        {
            if (!GetDialogInfo(pTmpUser, IMPORT_PRIV_NO_OVER_TXT, dwKeySpec, &Info))
                dwResult = FALSE;
            else
            {
                if (-1 == (dwResult = DialogBoxParam(hInstance,
                                                     MAKEINTRESOURCE(IDD_NEW_PASSWORD),
                                                     pTmpUser->hWnd,
                                                     (DLGPROC)DialogNewPassword,
                                                     (long)&Info)))
                    dwResult = FALSE;
            }
        }
    }
    else if (SIMPLEBLOB == dwBlobType)
    {
        if (!GetDialogInfo(pTmpUser, IMPORT_SIMPLE_TXT, dwKeySpec, &Info))
            dwResult = FALSE;
        else
        {
            if (-1 == (dwResult = DialogBoxParam(hInstance,
                                                 MAKEINTRESOURCE(IDD_SOLICIT_PASSWORD),
                                                 pTmpUser->hWnd,
                                                 (DLGPROC)DialogSolicitPassword,
                                                 (long)&Info)))
                dwResult = FALSE;
        }
    }

#endif // _USE_UI

    return dwResult;
}

/*
 -  CryptConfirmGenKey
 -
 *  Purpose:
 *                Determine whether the gen key should proceed.
 *
 *
 *  Parameters:
 *               IN      pTmpUser     -  Pointer to the user list structure
 *               IN      pKey         -  Pointer to the key list structure
 *               IN      dwKeySpec    -  Type of key to be imported
 *
 *  Returns:
 */
BOOL CryptConfirmGenKey(
                        IN PNTAGUserList pTmpUser,
                        IN DWORD dwKeySpec
                        )
{
    BOOL    fOverwrite = FALSE;
    DWORD   dwResult = TRUE;

#ifdef _USE_UI
    DIALOG_INFO Info;

    if (AT_SIGNATURE == dwKeySpec)
    {
        if (pTmpUser->SigPrivLen)
            fOverwrite = TRUE;
    }
    else
    {
        if (pTmpUser->ExchPrivLen)
            fOverwrite = TRUE;
    }

    if (fOverwrite)
    {
        if (!GetDialogInfo(pTmpUser, GENKEY_OVER_TXT, dwKeySpec, &Info))
            dwResult = FALSE;
        else
        {
            if (-1 == (dwResult = DialogBoxParam(hInstance,
                                                 MAKEINTRESOURCE(IDD_OVERWRITE_PASSWORD),
                                                 pTmpUser->hWnd,
                                                 (DLGPROC)DialogOverwritePassword,
                                                 (long)&Info)))
                dwResult = FALSE;
        }
    }
    else
    {
        if (!GetDialogInfo(pTmpUser, GENKEY_NO_OVER_TXT, dwKeySpec, &Info))
            dwResult = FALSE;
        else
        {
            if (-1 == (dwResult = DialogBoxParam(hInstance,
                                                 MAKEINTRESOURCE(IDD_NEW_PASSWORD),
                                                 pTmpUser->hWnd,
                                                 (DLGPROC)DialogNewPassword,
                                                 (long)&Info)))
                dwResult = FALSE;
        }
    }
#endif // _USE_UI

    return dwResult;
}

/*
 -  CryptConfirmChangePassword
 -
 *  Purpose:
 *                Determine whether the key password should be changed.
 *
 *
 *  Parameters:
 *               IN      pTmpUser     -  Pointer to the user list structure
 *               IN      dwKeySpec    -  Type of key changing the password for
 *
 *  Returns:
 */
BOOL CryptConfirmChangePassword(
                                IN PNTAGUserList pTmpUser,
                                IN DWORD dwKeySpec
                                )
{
    DWORD   dwResult = TRUE;

#ifdef _USE_UI
    DIALOG_INFO Info;

    if (!GetDialogInfo(pTmpUser, SETKEYPARAM_CHGPWD_TXT, dwKeySpec, &Info))
        dwResult = FALSE;
    else
    {
        if (-1 == (dwResult = DialogBoxParam(hInstance,
                                             MAKEINTRESOURCE(IDD_OVERWRITE_PASSWORD),
                                             pTmpUser->hWnd,
                                             (DLGPROC)DialogOverwritePassword,
                                             (long)&Info)))
            dwResult = FALSE;
    }
#endif // _USE_UI

    return dwResult;
}

