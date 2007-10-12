/*
 *	common shell dialogs
 *
 * Copyright 2000 Juergen Schmied
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"
#include "wine/port.h"

#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include "winerror.h"
#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "wingdi.h"
#include "winuser.h"
#include "commdlg.h"
#include "wine/debug.h"

#include "shellapi.h"
#include "shlobj.h"
#include "shell32_main.h"
#include "shresdef.h"
#include "undocshell.h"

typedef struct
    {
	HWND hwndOwner ;
	HICON hIcon ;
	LPCSTR lpstrDirectory ;
	LPCSTR lpstrTitle ;
	LPCSTR lpstrDescription ;
	UINT uFlags ;
    } RUNFILEDLGPARAMS ;

typedef BOOL (*LPFNOFN) (OPENFILENAMEA *) ;

WINE_DEFAULT_DEBUG_CHANNEL(shell);
static INT_PTR CALLBACK RunDlgProc (HWND, UINT, WPARAM, LPARAM) ;
static void FillList (HWND, char *) ;


/*************************************************************************
 * PickIconDlg					[SHELL32.62]
 *
 */
BOOL WINAPI PickIconDlg(
	HWND hwndOwner,
	LPWSTR lpstrFile,
	UINT nMaxFile,
	INT* lpdwIconIndex)
{
	FIXME("(%p,%s,%08x,%p):stub.\n",
	  hwndOwner, lpstrFile, nMaxFile,lpdwIconIndex);
	return 0xffffffff;
}

/*************************************************************************
 * RunFileDlg					[SHELL32.61]
 *
 * NOTES
 *     Original name: RunFileDlg (exported by ordinal)
 */
void WINAPI RunFileDlg(
	HWND hwndOwner,
	HICON hIcon,
	LPCSTR lpstrDirectory,
	LPCSTR lpstrTitle,
	LPCSTR lpstrDescription,
	UINT uFlags)
{

    RUNFILEDLGPARAMS rfdp;
    HRSRC hRes;
    LPVOID template;
    TRACE("\n");

    rfdp.hwndOwner        = hwndOwner;
    rfdp.hIcon            = hIcon;
    rfdp.lpstrDirectory   = lpstrDirectory;
    rfdp.lpstrTitle       = lpstrTitle;
    rfdp.lpstrDescription = lpstrDescription;
    rfdp.uFlags           = uFlags;

    if(!(hRes = FindResourceA(shell32_hInstance, "SHELL_RUN_DLG", (LPSTR)RT_DIALOG)))
        {
        MessageBoxA (hwndOwner, "Couldn't find dialog.", "Nix", MB_OK) ;
        return;
        }
    if(!(template = (LPVOID)LoadResource(shell32_hInstance, hRes)))
        {
        MessageBoxA (hwndOwner, "Couldn't load dialog.", "Nix", MB_OK) ;
        return;
        }

    DialogBoxIndirectParamA((HINSTANCE)GetWindowLongPtrW( hwndOwner,
						       GWLP_HINSTANCE ),
			    template, hwndOwner, RunDlgProc, (LPARAM)&rfdp);

}

/* Dialog procedure for RunFileDlg */
static INT_PTR CALLBACK RunDlgProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
    int ic ;
    char *psz, szMsg[256] ;
    static RUNFILEDLGPARAMS *prfdp = NULL ;

    switch (message)
        {
        case WM_INITDIALOG :
            prfdp = (RUNFILEDLGPARAMS *)lParam ;
            SetWindowTextA (hwnd, prfdp->lpstrTitle) ;
            SetClassLongPtrW (hwnd, GCLP_HICON, (LPARAM)prfdp->hIcon) ;
            SendMessageW (GetDlgItem (hwnd, 12297), STM_SETICON,
                          (WPARAM)LoadIconW (NULL, (LPCWSTR)IDI_WINLOGO), 0);
            FillList (GetDlgItem (hwnd, 12298), NULL) ;
            SetFocus (GetDlgItem (hwnd, 12298)) ;
            return TRUE ;

        case WM_COMMAND :
            switch (LOWORD (wParam))
                {
                case IDOK :
                    {
                    HWND htxt = NULL ;
                    if ((ic = GetWindowTextLengthA (htxt = GetDlgItem (hwnd, 12298))))
                        {
                        psz = HeapAlloc( GetProcessHeap(), 0, (ic + 2) );
                        GetWindowTextA (htxt, psz, ic + 1) ;

                        if (ShellExecuteA(NULL, "open", psz, NULL, NULL, SW_SHOWNORMAL) < (HINSTANCE)33)
                            {
                            char *pszSysMsg = NULL ;
                            FormatMessageA (
                                FORMAT_MESSAGE_ALLOCATE_BUFFER |
                                FORMAT_MESSAGE_FROM_SYSTEM |
                                FORMAT_MESSAGE_IGNORE_INSERTS,
                                NULL, GetLastError (),
                                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                                (LPSTR)&pszSysMsg, 0, NULL
                                ) ;
                            sprintf (szMsg, "Error: %s", pszSysMsg) ;
                            LocalFree ((HLOCAL)pszSysMsg) ;
                            MessageBoxA (hwnd, szMsg, "Nix", MB_OK | MB_ICONEXCLAMATION) ;

                            HeapFree(GetProcessHeap(), 0, psz);
                            SendMessageA (htxt, CB_SETEDITSEL, 0, MAKELPARAM (0, -1)) ;
                            return TRUE ;
                            }
                        FillList (htxt, psz) ;
                        HeapFree(GetProcessHeap(), 0, psz);
                        EndDialog (hwnd, 0) ;
                        }
                    }

                case IDCANCEL :
                    EndDialog (hwnd, 0) ;
                    return TRUE ;

                case 12288 :
                    {
                    HMODULE hComdlg = NULL ;
                    LPFNOFN ofnProc = NULL ;
                    static char szFName[1024] = "", szFileTitle[256] = "", szInitDir[768] = "" ;
                    static OPENFILENAMEA ofn =
                        {
                        sizeof (OPENFILENAMEA),
                        NULL,
                        NULL,
                        "Executable Files\0*.exe\0All Files\0*.*\0\0\0\0",
                        NULL,
                        0,
                        0,
                        szFName,
                        1023,
                        szFileTitle,
                        255,
                        (LPCSTR)szInitDir,
                        "Browse",
                        OFN_ENABLESIZING | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_PATHMUSTEXIST,
                        0,
                        0,
                        NULL,
                        0,
                        (LPOFNHOOKPROC)NULL,
                        NULL
                        } ;

                    ofn.hwndOwner = hwnd ;

                    if (NULL == (hComdlg = LoadLibraryExA ("comdlg32", NULL, 0)))
                        {
                        MessageBoxA (hwnd, "Unable to display dialog box (LoadLibraryEx) !", "Nix", MB_OK | MB_ICONEXCLAMATION) ;
                        return TRUE ;
                        }

                    if ((LPFNOFN)NULL == (ofnProc = (LPFNOFN)GetProcAddress (hComdlg, "GetOpenFileNameA")))
                        {
                        MessageBoxA (hwnd, "Unable to display dialog box (GetProcAddress) !", "Nix", MB_OK | MB_ICONEXCLAMATION) ;
                        return TRUE ;
                        }

                    ofnProc (&ofn) ;

                    SetFocus (GetDlgItem (hwnd, IDOK)) ;
                    SetWindowTextA (GetDlgItem (hwnd, 12298), szFName) ;
                    SendMessageA (GetDlgItem (hwnd, 12298), CB_SETEDITSEL, 0, MAKELPARAM (0, -1)) ;
                    SetFocus (GetDlgItem (hwnd, IDOK)) ;

                    FreeLibrary (hComdlg) ;

                    return TRUE ;
                    }
                }
            return TRUE ;
        }
    return FALSE ;
    }

/* This grabs the MRU list from the registry and fills the combo for the "Run" dialog above */
static void FillList (HWND hCb, char *pszLatest)
    {
    HKEY hkey ;
/*    char szDbgMsg[256] = "" ; */
    char *pszList = NULL, *pszCmd = NULL, cMatch = 0, cMax = 0x60, szIndex[2] = "-" ;
    DWORD icList = 0, icCmd = 0 ;
    UINT Nix ;

    SendMessageA (hCb, CB_RESETCONTENT, 0, 0) ;

    if (ERROR_SUCCESS != RegCreateKeyExA (
        HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\RunMRU",
        0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hkey, NULL))
        MessageBoxA (hCb, "Unable to open registry key !", "Nix", MB_OK) ;

    RegQueryValueExA (hkey, "MRUList", NULL, NULL, NULL, &icList) ;

    if (icList > 0)
        {
        pszList = HeapAlloc( GetProcessHeap(), 0, icList) ;
        if (ERROR_SUCCESS != RegQueryValueExA (hkey, "MRUList", NULL, NULL, (LPBYTE)pszList, &icList))
            MessageBoxA (hCb, "Unable to grab MRUList !", "Nix", MB_OK) ;
        }
    else
        {
        icList = 1 ;
        pszList = HeapAlloc( GetProcessHeap(), 0, icList) ;
        pszList[0] = 0 ;
        }

    for (Nix = 0 ; Nix < icList - 1 ; Nix++)
        {
        if (pszList[Nix] > cMax)
            cMax = pszList[Nix] ;

        szIndex[0] = pszList[Nix] ;

        if (ERROR_SUCCESS != RegQueryValueExA (hkey, szIndex, NULL, NULL, NULL, &icCmd))
            MessageBoxA (hCb, "Unable to grab size of index", "Nix", MB_OK) ;
        if( pszCmd )
            pszCmd = HeapReAlloc(GetProcessHeap(), 0, pszCmd, icCmd) ;
        else
            pszCmd = HeapAlloc(GetProcessHeap(), 0, icCmd) ;
        if (ERROR_SUCCESS != RegQueryValueExA (hkey, szIndex, NULL, NULL, (LPBYTE)pszCmd, &icCmd))
            MessageBoxA (hCb, "Unable to grab index", "Nix", MB_OK) ;

        if (NULL != pszLatest)
            {
            if (!lstrcmpiA(pszCmd, pszLatest))
                {
                /*
                sprintf (szDbgMsg, "Found existing (%d).\n", Nix) ;
                MessageBoxA (hCb, szDbgMsg, "Nix", MB_OK) ;
                */
                SendMessageA (hCb, CB_INSERTSTRING, 0, (LPARAM)pszCmd) ;
                SetWindowTextA (hCb, pszCmd) ;
                SendMessageA (hCb, CB_SETEDITSEL, 0, MAKELPARAM (0, -1)) ;

                cMatch = pszList[Nix] ;
                memmove (&pszList[1], pszList, Nix) ;
                pszList[0] = cMatch ;
                continue ;
                }
            }

        if (26 != icList - 1 || icList - 2 != Nix || cMatch || NULL == pszLatest)
            {
            /*
            sprintf (szDbgMsg, "Happily appending (%d).\n", Nix) ;
            MessageBoxA (hCb, szDbgMsg, "Nix", MB_OK) ;
            */
            SendMessageA (hCb, CB_ADDSTRING, 0, (LPARAM)pszCmd) ;
            if (!Nix)
                {
                SetWindowTextA (hCb, pszCmd) ;
                SendMessageA (hCb, CB_SETEDITSEL, 0, MAKELPARAM (0, -1)) ;
                }

            }
        else
            {
            /*
            sprintf (szDbgMsg, "Doing loop thing.\n") ;
            MessageBoxA (hCb, szDbgMsg, "Nix", MB_OK) ;
            */
            SendMessageA (hCb, CB_INSERTSTRING, 0, (LPARAM)pszLatest) ;
            SetWindowTextA (hCb, pszLatest) ;
            SendMessageA (hCb, CB_SETEDITSEL, 0, MAKELPARAM (0, -1)) ;

            cMatch = pszList[Nix] ;
            memmove (&pszList[1], pszList, Nix) ;
            pszList[0] = cMatch ;
            szIndex[0] = cMatch ;
            RegSetValueExA (hkey, szIndex, 0, REG_SZ, (LPBYTE)pszLatest, strlen (pszLatest) + 1) ;
            }
        }

    if (!cMatch && NULL != pszLatest)
        {
        /*
        sprintf (szDbgMsg, "Simply inserting (increasing list).\n") ;
        MessageBoxA (hCb, szDbgMsg, "Nix", MB_OK) ;
        */
        SendMessageA (hCb, CB_INSERTSTRING, 0, (LPARAM)pszLatest) ;
        SetWindowTextA (hCb, pszLatest) ;
        SendMessageA (hCb, CB_SETEDITSEL, 0, MAKELPARAM (0, -1)) ;

        cMatch = ++cMax ;
        if( pszList )
            pszList = HeapReAlloc(GetProcessHeap(), 0, pszList, ++icList) ;
        else
            pszList = HeapAlloc(GetProcessHeap(), 0, ++icList) ;
        memmove (&pszList[1], pszList, icList - 1) ;
        pszList[0] = cMatch ;
        szIndex[0] = cMatch ;
        RegSetValueExA (hkey, szIndex, 0, REG_SZ, (LPBYTE)pszLatest, strlen (pszLatest) + 1) ;
        }

    RegSetValueExA (hkey, "MRUList", 0, REG_SZ, (LPBYTE)pszList, strlen (pszList) + 1) ;

    HeapFree( GetProcessHeap(), 0, pszCmd) ;
    HeapFree( GetProcessHeap(), 0, pszList) ;
    }


/*************************************************************************
 * ConfirmDialog				[internal]
 *
 * Put up a confirm box, return TRUE if the user confirmed
 */
static BOOL ConfirmDialog(HWND hWndOwner, UINT PromptId, UINT TitleId)
{
  WCHAR Prompt[256];
  WCHAR Title[256];

  LoadStringW(shell32_hInstance, PromptId, Prompt, sizeof(Prompt) / sizeof(WCHAR));
  LoadStringW(shell32_hInstance, TitleId, Title, sizeof(Title) / sizeof(WCHAR));
  return MessageBoxW(hWndOwner, Prompt, Title, MB_YESNO|MB_ICONQUESTION) == IDYES;
}


/*************************************************************************
 * RestartDialogEx				[SHELL32.730]
 */

int WINAPI RestartDialogEx(HWND hWndOwner, LPCWSTR lpwstrReason, DWORD uFlags, DWORD uReason)
{
    TRACE("(%p)\n", hWndOwner);

    /*FIXME: use uReason */

    if (ConfirmDialog(hWndOwner, IDS_RESTART_PROMPT, IDS_RESTART_TITLE))
    {
        HANDLE hToken;
        TOKEN_PRIVILEGES npr;

        /* enable shutdown privilege for current process */
        if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken))
        {
            LookupPrivilegeValueA(0, "SeShutdownPrivilege", &npr.Privileges[0].Luid);
            npr.PrivilegeCount = 1;
            npr.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
            AdjustTokenPrivileges(hToken, FALSE, &npr, 0, 0, 0);
            CloseHandle(hToken);
        }
        ExitWindowsEx(EWX_REBOOT, 0);
    }

    return 0;
}

/************************************************************************* 	 
 * LogoffWindowsDialog                          [SHELL32.54] 	 
 */ 	 
               
int WINAPI LogoffWindowsDialog(DWORD uFlags) 	 
{ 	 
   ERR("LogoffWindowsDialog is UNIMPLEMENTED\n"); 	 
   ExitWindowsEx(EWX_LOGOFF, 0); 	 
   return 0; 	 
}

/*************************************************************************
 * RestartDialog				[SHELL32.59]
 */

int WINAPI RestartDialog(HWND hWndOwner, LPCWSTR lpstrReason, DWORD uFlags)
{
    return RestartDialogEx(hWndOwner, lpstrReason, uFlags, 0);
}


/*************************************************************************
 * ExitWindowsDialog				[SHELL32.60]
 *
 * NOTES
 *     exported by ordinal
 */
void WINAPI ExitWindowsDialog (HWND hWndOwner)
{
    TRACE("(%p)\n", hWndOwner);

    if (ConfirmDialog(hWndOwner, IDS_SHUTDOWN_PROMPT, IDS_SHUTDOWN_TITLE))
    {
        HANDLE hToken;
        TOKEN_PRIVILEGES npr;

        /* enable shutdown privilege for current process */
        if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken))
        {
            LookupPrivilegeValueA(0, "SeShutdownPrivilege", &npr.Privileges[0].Luid);
            npr.PrivilegeCount = 1;
            npr.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
            AdjustTokenPrivileges(hToken, FALSE, &npr, 0, 0, 0);
            CloseHandle(hToken);
        }
        ExitWindowsEx(EWX_SHUTDOWN, 0);
    }
}
