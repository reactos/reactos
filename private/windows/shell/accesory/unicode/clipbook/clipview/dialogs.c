
/***************************************************************************/
/**                  Microsoft Windows                                    **/
/**            Copyright(c) Microsoft Corp., 1991,1992                    **/
/***************************************************************************/
// ts=4

// DIALOGS.C - ClipBook viewer dialog procedures
// 11-91 clausgi created


#include "windows.h"
#include "clipbook.h"
#include "..\common\common.h"
#include "dialogs.h"
#include "nddeapi.h"
#include "helpids.h"
#include "uniconv.h"


#define      PERM_READ   (NDDEACCESS_REQUEST|NDDEACCESS_ADVISE)
#define      PERM_WRITE   (NDDEACCESS_REQUEST|NDDEACCESS_ADVISE|NDDEACCESS_POKE|NDDEACCESS_EXECUTE)


BOOL CALLBACK ConnectDlgProc (
HWND hwnd,
UINT message,WPARAM wParam,
LPARAM lParam)
{
switch (message)
   {
case WM_INITDIALOG:

   szConvPartner[0] = TEXT('\0');
      break;

case WM_COMMAND:
   switch (wParam)
      {

   case IDOK:
      GetDlgItemText ( hwnd, IDC_CONNECTNAME, szConvPartner, 32 );
      EndDialog (hwnd, 1);
      break;

   case IDCANCEL:
      szConvPartner[0] = TEXT('\0');
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

// Note: this routine expectes a PNDDESHAREINFO in lParam!
// if
BOOL CALLBACK ShareDlgProc(
HWND hwnd,
UINT message,
WPARAM wParam,
LPARAM lParam)
{
static PNDDESHAREINFO lpDdeS;
DWORD                 dwTrustOptions;
// These vars are used for determining if I'm owner of the page
PSID                  psidPage;
PSID                  psidCurrent;
BOOL                  fDump;
PSECURITY_DESCRIPTOR  pSD;
TOKEN_USER           *pusr;
HANDLE                htok;
DWORD                 cbSD;
UINT                  uRet;

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
   if (pSD = LocalAlloc(LPTR, 50))
      {
      NDdeGetShareSecurity(NULL, lpDdeS->lpszShareName,
            OWNER_SECURITY_INFORMATION, pSD, 50, &cbSD);
      LocalFree(pSD);

      if (pSD = LocalAlloc(LPTR, cbSD))
         {
         uRet = NDdeGetShareSecurity(NULL, lpDdeS->lpszShareName,
               OWNER_SECURITY_INFORMATION, pSD, cbSD, &cbSD);
         if (NDDE_NO_ERROR == uRet)
            {
            if (GetSecurityDescriptorOwner(pSD, &psidPage, &fDump))
               {
               if (psidPage && IsUserMember(psidPage))
                  {
                  DWORD adwTrust[3];

                  EnableWindow(GetDlgItem(hwnd, IDC_STARTAPP), TRUE);
                  // 207 is the group box around the checkboxes
                  EnableWindow(GetDlgItem(hwnd, 207), TRUE);

                  NDdeGetTrustedShare(NULL, lpDdeS->lpszShareName,
                        adwTrust, adwTrust + 1, adwTrust + 2);

                  if (adwTrust[0] & NDDE_TRUST_SHARE_START)
                     {
                     CheckDlgButton(hwnd, IDC_STARTAPP, 1);

                     EnableWindow(GetDlgItem(hwnd, IDC_MINIMIZED), TRUE);
                     CheckDlgButton(hwnd, IDC_MINIMIZED,
                           (SW_MINIMIZE == (adwTrust[0] &
                             NDDE_CMD_SHOW_MASK)) ? 1 : 0);
                     }
                  else
                     {
                     PINFO(TEXT("Buttons shouldn't check\r\n"));
                     }
                  }
               else
                  {
                  PINFO(TEXT("User isn't member of owner\r\n"));
                  }
               }
            else
               {
               PERROR(TEXT("Couldn't get owner, even tho we asked\r\n"));
               }

            }
         else
            {
            PERROR(TEXT("GetSec fail %d"), uRet);
            }
         LocalFree(pSD);
         }
      else
         {
         PERROR(TEXT("Couldn't alloc %ld bytes\r\n"), cbSD);
         }
      }
   else
      {
      PERROR(TEXT("Couldn't alloc 50 bytes\r\n"));
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

   case IDHELP:
      WinHelp(hwnd, szHelpFile, HELP_CONTEXT, IDH_DLG_SHARE );
      break;

   case IDC_PERMISSIONS:
      PermissionsEdit(hwnd, lpDdeS->lpszShareName, FALSE);
      break;

   case IDC_STARTAPP:
      EnableWindow(GetDlgItem(hwnd, IDC_MINIMIZED),
            IsDlgButtonChecked(hwnd, IDC_STARTAPP));
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

static BOOL IsUniqueName (
LPTSTR key)
{
PMDIINFO pMDI = GETMDIINFO(hwndLocal);
LISTENTRY ListEntry;

lstrcpy ( ListEntry.name, key );
if ( SendMessage( pMDI->hWndListbox, LB_FINDSTRING, (WPARAM)-1,
                  (LPARAM)(LPCSTR) &ListEntry ) != LB_ERR )
   {
   return FALSE;
   }
return TRUE;
}


BOOL CALLBACK KeepAsDlgProc(
HWND         hwnd,
UINT         msg,
WPARAM       wParam,
LPARAM       lParam)
{

switch (msg)
   {
case WM_INITDIALOG:
   SendDlgItemMessage ( hwnd, IDC_KEEPASEDIT, EM_LIMITTEXT,
      MAX_NDDESHARENAME - 5, 0L );
   SendDlgItemMessage ( hwnd, IDC_SHARECHECKBOX, BM_SETCHECK,
      fSharePreference, 0L );
    break;

case WM_COMMAND:
   switch (wParam)
      {
   case IDOK:
      fSharePreference = (BOOL)SendDlgItemMessage ( hwnd,
         IDC_SHARECHECKBOX, BM_GETCHECK, 0, 0L );

      if (!GetDlgItemText(hwnd, IDC_KEEPASEDIT, szKeepAs+1,
            MAX_PAGENAME_LENGTH))
         {
         SetFocus ( GetDlgItem( hwnd, IDC_KEEPASEDIT ) );
         break;
         }

      szKeepAs[0] = SHR_CHAR;

      if (!NDdeIsValidShareName(szKeepAs + 1))
         {
         MessageBoxID ( hInst, hwnd, IDS_PAGENAMESYNTAX,
            IDS_PASTEDLGTITLE, MB_OK | MB_ICONEXCLAMATION );
         break;
         }

      szKeepAs[0] = UNSHR_CHAR;

      // make sure name is unique
      if ( !IsUniqueName ( szKeepAs ))
         {
         MessageBoxID ( hInst, hwnd, IDS_NAMEEXISTS, IDS_PASTEDLGTITLE,
            MB_OK | MB_ICONEXCLAMATION );
         break;
         }
      EndDialog( hwnd, TRUE );
      break;

   case IDCANCEL:
       EndDialog( hwnd, FALSE );
       break;

   case IDHELP:
      WinHelp(hwnd, szHelpFile, HELP_CONTEXT, IDH_DLG_PASTEDATA );
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
