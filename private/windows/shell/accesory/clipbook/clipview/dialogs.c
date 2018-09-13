/*****************************************************************************

                                D I A L O G S

    Name:       dialogs.c
    Date:       21-Jan-1994
    Creator:    Unknown

    Description:
        Dialog handling routines.

    History:
        13-Mar-1995 John Fu     allow KeepAsDlgProc to "Paste to Page"

        03-Nov-1997 Drewm       Added support for context sensitive help

*****************************************************************************/



#include <windows.h>
#include <nddeapi.h>

#include "common.h"
#include "clipbook.h"
#include "clipbrd.h"
#include "clpbkdlg.h"
#include "cvutil.h"
#include "dialogs.h"
#include "helpids.h"
#include "debugout.h"
#include "ismember.h"
#include "shares.h"



#define      PERM_READ  (NDDEACCESS_REQUEST|NDDEACCESS_ADVISE)
#define      PERM_WRITE (NDDEACCESS_REQUEST|NDDEACCESS_ADVISE|NDDEACCESS_POKE|NDDEACCESS_EXECUTE)





static BOOL    IsUniqueName (PKEEPASDLG_PARAM key);
static BOOL    StrHas2BkSlash (LPTSTR  str);




/*
 *      ConnectDlgProc
 */

INT_PTR CALLBACK ConnectDlgProc (
    HWND    hwnd,
    UINT    message,
    WPARAM  wParam,
    LPARAM  lParam)
{

    switch (message)
        {
        case WM_INITDIALOG:
            szConvPartner[0] = '\0';
            break;

        case WM_COMMAND:
            switch (wParam)
                {
                case IDOK:
                    GetDlgItemText (hwnd, IDC_CONNECTNAME, szConvPartner, 32);
                    EndDialog (hwnd, 1);
                    break;

                case IDCANCEL:
                    szConvPartner[0] = '\0';
                    EndDialog (hwnd, 0);
                    break;

                default:
                    return FALSE;
                }
            break;

        default:
            return FALSE;
        }

    return TRUE;

}






/*
 *      ShareDlgProc
 *
 *  Note: this routine expectes a PNDDESHAREINFO in lParam!
 */

INT_PTR CALLBACK ShareDlgProc(
    HWND    hwnd,
    UINT    message,
    WPARAM  wParam,
    LPARAM  lParam)
{
static PNDDESHAREINFO   lpDdeS;
DWORD                   dwTrustOptions;
DWORD                   adwTrust[3];

BOOL                    bRet = TRUE;

// These vars are used for determining if I'm owner of the page
PSID                    psidPage;
BOOL                    fDump;
DWORD                   cbSD;
UINT                    uRet;
PSECURITY_DESCRIPTOR    pSD = NULL;

LPHELPINFO              lphi;
const DWORD aHelpIDs[] =
{
    IDC_STARTAPP, IDH_STARTAPP,
    IDC_MINIMIZED, IDH_MINIMIZED,
    IDC_PERMISSIONS, IDH_PERMISSIONS,
    0, 0
};
UINT    iCtrlId;



    switch (message)
        {
        case WM_INITDIALOG:

            lpDdeS = (PNDDESHAREINFO)lParam;

            // set share, always static
            SetDlgItemText (hwnd, IDC_STATICSHARENAME, lpDdeS->lpszShareName+1 );

            // If the current user doesn't own the page, we gray out the
            // "start app" and "run minimized" checkboxes.. basically, people
            // who aren't the owner don't get to trust the share.
            EnableWindow(GetDlgItem(hwnd, IDC_STARTAPP), FALSE);
            EnableWindow(GetDlgItem(hwnd, IDC_MINIMIZED), FALSE);
            EnableWindow(GetDlgItem(hwnd, 207), FALSE);



            // Figure out who owns the page
            psidPage = NULL;
            if (!(pSD = LocalAlloc(LPTR, 50)))
                {
                PERROR(TEXT("Couldn't alloc 50 bytes\r\n"));
                break;
                }


            uRet = NDdeGetShareSecurity (NULL,
                                         lpDdeS->lpszShareName,
                                         OWNER_SECURITY_INFORMATION,
                                         pSD,
                                         50,
                                         &cbSD);

            if (uRet == NDDE_BUF_TOO_SMALL)
                {
                LocalFree (pSD);

                if (!(pSD = LocalAlloc(LPTR, cbSD)))
                   {
                   PERROR(TEXT("Couldn't alloc %ld bytes\r\n"), cbSD);
                   break;
                   }


                uRet = NDdeGetShareSecurity (NULL,
                                             lpDdeS->lpszShareName,
                                             OWNER_SECURITY_INFORMATION,
                                             pSD,
                                             cbSD,
                                             &cbSD);
                }


            if (NDDE_NO_ERROR != uRet)
                {
                PERROR(TEXT("GetSec fail %d"), uRet);
                break;
                }


            if (!GetSecurityDescriptorOwner(pSD, &psidPage, &fDump))
                {
                PERROR(TEXT("Couldn't get owner, even tho we asked\r\n"));
                break;
                }

            if (!psidPage || !IsUserMember(psidPage))
                {
                PINFO(TEXT("User isn't member of owner\r\n"));
                break;
                }



            EnableWindow (GetDlgItem (hwnd, IDC_STARTAPP), TRUE);

            // 207 is the group box around the checkboxes
            EnableWindow (GetDlgItem (hwnd, 207), TRUE);

            NDdeGetTrustedShare (NULL,
                                 lpDdeS->lpszShareName,
                                 adwTrust,
                                 adwTrust + 1,
                                 adwTrust + 2);

            if (!(adwTrust[0] & NDDE_TRUST_SHARE_START))
                {
                PINFO (TEXT("Buttons shouldn't check\r\n"));
                }
            else
                {
                CheckDlgButton(hwnd, IDC_STARTAPP, 1);

                EnableWindow (GetDlgItem (hwnd, IDC_MINIMIZED), TRUE);
                CheckDlgButton (hwnd,
                                IDC_MINIMIZED,
                                (SW_MINIMIZE == (adwTrust[0] & NDDE_CMD_SHOW_MASK)) ? 1 : 0);
               }

            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
                {
                case IDOK:
                   dwTrustOptions = NDDE_TRUST_SHARE_INIT;

                   if (IsDlgButtonChecked(hwnd, IDC_STARTAPP))
                      {
                      dwTrustOptions |= NDDE_TRUST_SHARE_START;

                      if (IsDlgButtonChecked(hwnd, IDC_MINIMIZED))
                         {
                         dwTrustOptions |= NDDE_TRUST_CMD_SHOW | SW_MINIMIZE;
                         }
                      }

                   // Update the share start flag.
                   if (dwTrustOptions & NDDE_TRUST_SHARE_START)
                       lpDdeS->fStartAppFlag = TRUE;
                   else
                       lpDdeS->fStartAppFlag = FALSE;

                   NDdeSetTrustedShare(NULL, lpDdeS->lpszShareName, dwTrustOptions);
                   EndDialog (hwnd, TRUE);
                   break;

                case IDCANCEL:
                   EndDialog (hwnd, FALSE);
                   break;

                case IDC_PERMISSIONS:
                   EditPermissions2 (hwnd, lpDdeS->lpszShareName, FALSE);
                   break;

                case  IDC_STARTAPP:
                   EnableWindow(GetDlgItem(hwnd, IDC_MINIMIZED),
                         IsDlgButtonChecked(hwnd, IDC_STARTAPP));
                   break;

                default:
                   bRet = FALSE;
                }
            break;

        case WM_HELP:
            lphi = (LPHELPINFO) lParam;
            if (lphi->iContextType == HELPINFO_WINDOW)
            {
                if ((lphi->iCtrlId == IDC_STARTAPP) ||
                    (lphi->iCtrlId == IDC_MINIMIZED) ||
                    (lphi->iCtrlId == IDC_PERMISSIONS) ||
                    (lphi->iCtrlId == IDOK) ||
                    (lphi->iCtrlId == IDCANCEL) )
                {
                WinHelp ( lphi->hItemHandle,
                          szHelpFile,
                          HELP_WM_HELP,
                          (DWORD_PTR) (LPVOID) aHelpIDs);
                }
            }
            else
            {
                bRet = FALSE;
            }
            break;

        case WM_CONTEXTMENU:
            iCtrlId = GetDlgCtrlID( (HWND) wParam );
            if ((iCtrlId == IDC_STATICSHARENAME) ||
                (iCtrlId == IDC_STARTAPP) ||
                (iCtrlId == IDC_MINIMIZED) ||
                (iCtrlId == IDC_PERMISSIONS) ||
                (iCtrlId == IDOK) ||
                (iCtrlId == IDCANCEL) )
            {
                WinHelp( (HWND) wParam,
                         szHelpFile,
                         HELP_CONTEXTMENU,
                         (DWORD_PTR) (LPVOID) aHelpIDs);
            }
            break;
        //

        default:
        bRet = FALSE;
        }



    if (pSD)
        LocalFree (pSD);

    return bRet;

}










/*
 *      IsUniqueName
 *
 *  Check to see if the name is unique
 */

static BOOL IsUniqueName (PKEEPASDLG_PARAM pParam)
{
PMDIINFO    pMDI;
LISTENTRY   ListEntry;
PLISTENTRY  pLE;
INT         i;


    if (!(pMDI = GETMDIINFO(hwndLocal)))
        return FALSE;


    lstrcpy (ListEntry.name, pParam->ShareName);

    i = (INT)SendMessage (pMDI->hWndListbox,
                          LB_FINDSTRING,
                          (WPARAM)-1,
                          (LPARAM)(LPCSTR) &ListEntry);


    if (LB_ERR != i)
        {
        SendMessage (pMDI->hWndListbox,
                     LB_GETTEXT,
                     i,
                     (LPARAM)&pLE);

        pParam->bAlreadyShared = IsShared (pLE);

        return FALSE;
        }

    return TRUE;

}







/*
 *      StrHas2BkSlash
 *
 *  Check to see if the string has
 *  double back slashes.  If double
 *  back slashes exists then return
 *  TRUE else return FALSE.
 */

static BOOL    StrHas2BkSlash (LPTSTR  str)
{
TCHAR   c;

    while (c = *str++)
        {
        if (c == TEXT('\\') && *str == TEXT('\\'))
            return TRUE;
        }

    return FALSE;

}






/*
 *      KeepAsDlgProc
 *
 *  Ask the user for a page name.
 */

INT_PTR CALLBACK KeepAsDlgProc(
    HWND    hwnd,
    UINT    msg,
    WPARAM  wParam,
    LPARAM  lParam)
{
static PKEEPASDLG_PARAM pParam;

LPHELPINFO              lphi;
const DWORD aHelpIDs[] =
{
    IDC_STATICPAGENAME, IDH_KEEPASEDIT,
    IDC_KEEPASEDIT, IDH_KEEPASEDIT,
    IDC_SHARECHECKBOX, IDH_SHARECHECKBOX,
    0, 0
};



    switch (msg)
        {
        case WM_INITDIALOG:
            pParam = (PKEEPASDLG_PARAM)lParam;

            pParam->bAlreadyExist  = FALSE;
            pParam->bAlreadyShared = FALSE;
            pParam->ShareName[0]   = '\0';

            SendDlgItemMessage (hwnd, IDC_KEEPASEDIT, EM_LIMITTEXT, MAX_NDDESHARENAME - 15, 0L);
            SendDlgItemMessage (hwnd, IDC_SHARECHECKBOX, BM_SETCHECK, fSharePreference, 0L);
            break;

        case WM_COMMAND:
            switch (wParam)
                {
                case IDOK:
                    fSharePreference = (BOOL)SendDlgItemMessage (hwnd,
                                                                 IDC_SHARECHECKBOX,
                                                                 BM_GETCHECK,
                                                                 0,
                                                                 0L );

                    if (!GetDlgItemText(hwnd, IDC_KEEPASEDIT, pParam->ShareName+1, MAX_PAGENAME_LENGTH))
                        {
                        SetFocus (GetDlgItem (hwnd, IDC_KEEPASEDIT));
                        break;
                        }

                    pParam->ShareName[0] = SHR_CHAR;

                    if (!NDdeIsValidShareName(pParam->ShareName + 1))
                        //|| StrHas2BkSlash (pParam->ShareName)) // this was a temp fix for NDde bug
                        {
                        MessageBoxID (hInst,
                                      hwnd,
                                      IDS_PAGENAMESYNTAX,
                                      IDS_PASTEDLGTITLE,
                                      MB_OK|MB_ICONEXCLAMATION);
                        break;
                        }

                    pParam->ShareName[0] = UNSHR_CHAR;

                    // make sure name is unique
                    if ( !IsUniqueName (pParam))
                        {
                        if (IDOK != MessageBoxID (hInst,
                                                 hwnd,
                                                 IDS_NAMEEXISTS,
                                                 IDS_PASTEDLGTITLE,
                                                 MB_OKCANCEL|MB_ICONEXCLAMATION))
                             break;

                        pParam->bAlreadyExist = TRUE;

                        //MessageBoxID (hInst,
                        //              hwnd,
                        //              IDS_NAMEEXISTS,
                        //              IDS_PASTEDLGTITLE,
                        //              MB_OK|MB_ICONEXCLAMATION);
                        //break;
                        }

                    EndDialog( hwnd, TRUE );
                    break;

                case IDCANCEL:
                     EndDialog( hwnd, FALSE );
                     break;

                default:
                     return FALSE;
                }
            break;

        case WM_HELP:
            lphi = (LPHELPINFO) lParam;
            if (lphi->iContextType == HELPINFO_WINDOW)
            {
                WinHelp ( lphi->hItemHandle,
                          szHelpFile,
                          HELP_WM_HELP,
                          (DWORD_PTR) (LPVOID) aHelpIDs);
            }
            else
            {
                return FALSE;
            }
            break;

        case WM_CONTEXTMENU:
            WinHelp( (HWND) wParam,
                     szHelpFile,
                     HELP_CONTEXTMENU,
                     (DWORD_PTR) (LPVOID) aHelpIDs);
            break;
        //


        default:
            return FALSE;
        }

    return TRUE;

}
