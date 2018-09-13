
/***************************************************************************/
/**                  Microsoft Windows                                    **/
/**            Copyright(c) Microsoft Corp., 1991,1992                    **/
/***************************************************************************/
// ts=4

// CVCOMMAN.C - command processing for ClipBook viewer
// 4-92 clausgi created
// 11-92 a-mgates ported to NT


#define    WINVER 0x0310
#define    NOAUTOUPDATE 1
#define    MAX_FILENAME_LENGTH 255

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <nddeapi.h>
#include <shellapi.h>
#include <assert.h>

#include ".\clipbook.h"
#include ".\clipdsp.h"
#include ".\dialogs.h"
#include "..\common\common.h"
#include ".\helpids.h"
#include "uniconv.h"

// Typedef used for GetOpen/SaveFileName pointer
typedef BOOL (CALLBACK *LPFNGOF)(LPOPENFILENAME);
// Typedef for the ShellAbout function
typedef void (WINAPI *LPFNSHELLABOUT)(HWND, LPTSTR, LPTSTR, HICON);

// helper function forward
extern BOOL SetListboxEntryToPageWindow ( HWND, PMDIINFO, int );
extern DWORD dwCurrentHelpId;
extern HSZ hszDataSrv;

// Flags and typedef for the NT LanMan computer browser dialog.
// The actual function is I_SystemFocusDialog, in NTLANMAN.DLL.
#define FOCUSDLG_DOMAINS_ONLY        (1)
#define FOCUSDLG_SERVERS_ONLY        (2)
#define FOCUSDLG_SERVERS_AND_DOMAINS (3)
typedef UINT (APIENTRY *LPFNSYSFOCUS)(HWND, UINT, LPWSTR, UINT, PBOOL,
      LPWSTR, DWORD);

static TCHAR szDirName[256] = {TEXT('\0'),};

// Functions in DEBUG.C
extern LRESULT EditPermissions(BOOL);

// extern PSECURITY_DESCRIPTOR CurrentUserOnlySD(void);
extern LRESULT Properties(HWND, PLISTENTRY);

//
// Purpose: Delete the selected share.
//
///////////////////////////////////////////////////////////////////////
LRESULT OnIDMDelete(
HWND hwnd,
UINT msg,
WPARAM wparam,
LPARAM lparam)
{
int tmp;
LPLISTENTRY lpLE;
LISTENTRY   LE;
LRESULT     ret = 0;

// Doing a "delete" on the clipboard window clears clipboard
if ( pActiveMDI->flags & F_CLPBRD )
   {
   if ( ClearClipboard(hwndApp) == IDOK )
      {
      EmptyClipboard();
      InitializeMenu ( GetMenu(hwnd) );

      // Force redraw of clipboard window
      if (hwndClpbrd)
         {
         InvalidateRect(hwndClpbrd, NULL, TRUE);
         }
      }
   }
else
   {
   tmp = (int)SendMessage (  pActiveMDI->hWndListbox,
       LB_GETCURSEL, 0, 0L );

   if ( tmp != LB_ERR )
      {
      SendMessage (  pActiveMDI->hWndListbox, LB_GETTEXT, tmp,
          (LPARAM)(LPTSTR)&lpLE);
      memcpy(&LE, lpLE, sizeof(LE));

      wsprintf(szBuf, szDeleteConfirmFmt, (LPTSTR)((lpLE->name) + 1));
      MessageBeep ( MB_ICONEXCLAMATION );

      if ( MessageBox ( hwndApp, szBuf, szDelete,
             MB_ICONEXCLAMATION | MB_OKCANCEL ) != IDCANCEL )
         {
         AssertConnection ( hwndActiveChild );

         if ( hwndActiveChild == hwndClpOwner )
            {
            ForceRenderAll( hwnd, NULL );
            }


         // Perform an execute to the server to let it know that
         // we're not sharing anymore.
         wsprintf(szBuf,TEXT("%s%s"), SZCMD_DELETE, lpLE->name);
         if ( MySyncXact((LPBYTE) szBuf, ByteCountOf(lstrlen(szBuf) + 1),
                pActiveMDI->hExeConv, 0L, CF_TEXT,
                XTYP_EXECUTE, SHORT_SYNC_TIMEOUT, NULL)
            )
            {
            if ( pActiveMDI->DisplayMode == DSP_PAGE )
               {
               PINFO(TEXT("forcing back to list mode\n\r"));
               SendMessage ( hwndApp, WM_COMMAND,
                   pActiveMDI->OldDisplayMode == DSP_PREV ?
                       IDM_PREVIEWS : IDM_LISTVIEW, 0L );
               }

            UpdateListBox(hwndActiveChild, pActiveMDI->hExeConv);
            InitializeMenu(GetMenu(hwndApp));
            }
         else
            {
            if (ERROR_ACCESS_DENIED == GetLastError())
               {
               MessageBoxID(hInst, hwndApp, IDS_PRIVILEGEERROR, IDS_APPNAME,
                     MB_OK | MB_ICONHAND);
               }
            else
               {
               PERROR(TEXT("NddeShareDel fail %ld\r\n"), ret);
               MessageBoxID(hInst, hwndApp, IDS_INTERNALERR, IDS_APPNAME,
                     MB_OK | MB_ICONHAND);
               }
            }
         }
      }
   else
      {
      PERROR(TEXT("Could not figure out which item was selected!\r\n"));
      }
   }
return ret;
}


// Purpose: Create and activate a window showing the contents of the
// clipboard.
//
///////////////////////////////////////////////////////////////////////////
static void CreateClipboardWindow(
void)
{
WINDOWPLACEMENT wpl;
HMENU   hSysMenu;
UINT    uMaxed;

// create Clipboard Window
hwndClpbrd = hwndActiveChild = NewWindow();
if (NULL == hwndActiveChild)
  {
  return;
  }

pActiveMDI = GETMDIINFO(hwndActiveChild);

pActiveMDI->flags = F_CLPBRD;
pActiveMDI->DisplayMode = DSP_PAGE;

AdjustControlSizes ( hwndClpbrd );
ShowHideControls ( hwndClpbrd );

lstrcpy ( pActiveMDI->szBaseName, szSysClpBrd );
SetWindowText ( hwndClpbrd, szSysClpBrd );

// Grey out close item on sys menu
hSysMenu = GetSystemMenu ( hwndClpbrd, FALSE );
EnableMenuItem ( hSysMenu, SC_CLOSE, MF_GRAYED | MF_BYCOMMAND );

// Tell MDI where the Window menu is -- must do this BEFORE placing
// the clipboard window. (If the clipboard window's maximized, its
// System menu is the first menu-- not the app's File menu.)
hSysMenu = GetSubMenu(GetMenu(hwndApp), WINDOW_MENU_INDEX);
SendMessage(hwndMDIClient, WM_MDISETMENU, 0, (LPARAM)hSysMenu);

if ( ReadWindowPlacement ( szSysClpBrd, &wpl ))
   {
   wpl.length = sizeof(WINDOWPLACEMENT);
   wpl.flags = WPF_SETMINPOSITION;
   SetWindowPlacement ( hwndClpbrd, &wpl );
   PINFO(TEXT("sizing %s from .ini\n\r"), szSysClpBrd);
   UpdateWindow ( hwndClpbrd );
   }
else
   {
   PINFO(TEXT("showing %s in default size/posiiton\n\r"),
       szSysClpBrd );
   ShowWindow ( hwndClpbrd, SW_MINIMIZE );
   }

SendMessage ( hwndMDIClient, WM_MDIACTIVATE, (WPARAM)hwndClpbrd, 0L );
}

//
// Purpose: Create the "Local Clipbook" window.
//
// Parameters: None.
//
// Returns: Void.
//
///////////////////////////////////////////////////////////////////////
static void CreateLocalWindow(
void)
{
WINDOWPLACEMENT wpl;
HMENU hSysMenu;
UINT  uMaxed;

hwndLocal = hwndActiveChild = NewWindow();
if (NULL == hwndActiveChild)
  {
  return;
  }
pActiveMDI = GETMDIINFO(hwndActiveChild);
ShowHideControls ( hwndLocal );

pActiveMDI->hszConvPartner =
pActiveMDI->hszConvPartnerNP = hszDataSrv;

pActiveMDI->hExeConv =
   InitSysConv( hwndLocal, pActiveMDI->hszConvPartner, hszSystem, TRUE);

if ( pActiveMDI->hExeConv )
   {
   pActiveMDI->flags = F_LOCAL;
   UpdateListBox ( hwndLocal, pActiveMDI->hExeConv );
   SetWindowText ( hwndLocal, szLocalClpBk );
   lstrcpy ( pActiveMDI->szBaseName, szLocalClpBk );

   hSysMenu = GetSystemMenu ( hwndLocal, FALSE );
   EnableMenuItem ( hSysMenu, SC_CLOSE, MF_GRAYED );

   if ( ReadWindowPlacement ( szLocalClpBk, &wpl ))
      {
      wpl.length = sizeof(WINDOWPLACEMENT);
      wpl.flags = WPF_SETMINPOSITION;
      SetWindowPlacement ( hwndLocal, &wpl );
      PINFO(TEXT("sizing Local Clipbook from .ini\n\r"));
      UpdateWindow ( hwndLocal );
      }
   else
      {
      if ( !IsIconic(hwndApp))
         {
         RECT MDIrect;

         PINFO(TEXT("calculating size for Local Clipbook window\n\r"));
         GetClientRect ( hwndMDIClient, &MDIrect );
         MoveWindow ( hwndLocal,
             MDIrect.left, MDIrect.top, MDIrect.right - MDIrect.left,
             ( MDIrect.bottom - MDIrect.top )
             - GetSystemMetrics(SM_CYICONSPACING), FALSE );
         }
      else
         {
         fNeedToTileWindows = TRUE;
         }
      ShowWindow ( hwndLocal, SW_SHOWNORMAL );
      }

   SendMessage ( hwndMDIClient, WM_MDIACTIVATE, (WPARAM)hwndLocal, 0L );
   SendMessage ( hwndMDIClient, WM_MDIREFRESHMENU, 0, 0L);

   if (NULL != hkeyRoot)
      {
      DWORD dwDefView = IDM_LISTVIEW;
      DWORD dwSize = sizeof(dwDefView);

      if (ERROR_SUCCESS != RegQueryValueEx(hkeyRoot,
            (LPTSTR)szDefView, NULL, NULL, (LPBYTE)&dwDefView, &dwSize));
         {
         PINFO(TEXT("Couldn't get DefView value\r\n"));
         }

      SendMessage ( hwndApp, WM_COMMAND, dwDefView, 0L );
      }
   }
else
   {
#if DEBUG
   MessageBox( hwndApp, TEXT("No Local Server"),
         TEXT("ClipBook Initialization"), MB_OK | MB_ICONEXCLAMATION );
#endif
   PostMessage ( hwndLocal, WM_CLOSE, 0, 0L );
   }
}

//
// Purpose: Unshare the selected page in the active window.
//
// Parameters: None.
//
// Returns: Void. All error handling is provided within the function.
//
//////////////////////////////////////////////////////////////////////
void UnsharePage(
void)
{
DWORD adwTrust[3];
int   tmp;
LPLISTENTRY lpLE;
DWORD ret;

assert(pActiveMDI);

tmp = (int)SendMessage (  pActiveMDI->hWndListbox,
    LB_GETCURSEL, 0, 0L );

if ( tmp != LB_ERR )
   {
   WORD wAddlItems;
   PNDDESHAREINFO lpDdeI;
   DWORD dwRet = ByteCountOf(2048);

   if (lpDdeI = LocalAlloc(LPTR, ByteCountOf(2048)))
      {
      SendMessage (  pActiveMDI->hWndListbox, LB_GETTEXT, tmp,
          (LPARAM)(LPTSTR)&lpLE);

      AssertConnection(hwndActiveChild);

      PINFO(TEXT("for share [%s]"), lpLE->name);
      wAddlItems = 0;
      ret = NDdeShareGetInfo ( NULL, lpLE->name, 2,
          (LPBYTE)lpDdeI, ByteCountOf(2048), &dwRet, &wAddlItems );
      if (NDDE_NO_ERROR == ret)
         {
         register LPTSTR lpOog;

         lpOog = lpDdeI->lpszAppTopicList;

         // Jump over the first two NULL chars you find-- these
         // are the old- and new-style app/topic pairs, we don't
         // mess with them. Then jump over the next BAR_CHAR you find.
         // The first character after that is the first char of the
         // static topic-- change that to a UNSHR_CHAR.
         while (*lpOog++)
            {
            }
         while (*lpOog++)
            {
            }
         // BUGBUG: TEXT('|') should == BAR_CHAR. If not, this needs to
         // be adjusted.
         while (*lpOog++ != TEXT('|'))
            {
            }
         *lpOog = UNSHR_CHAR;
         lpDdeI->fSharedFlag = 0L;

         DumpDdeInfo(lpDdeI, NULL);

         // We want to get trusted info BEFORE we start changing
         // the share.
         NDdeGetTrustedShare(NULL, lpLE->name, adwTrust,
            adwTrust + 1, adwTrust + 2);

         ret = NDdeShareSetInfo ( NULL, lpLE->name, 2,
             (LPBYTE)lpDdeI, ByteCountOf(2048), 0 );
         if ( NDDE_NO_ERROR == ret)
            {
      #if 0
            PSECURITY_DESCRIPTOR pSD;
            DWORD dwSize;
            TCHAR atch[2048];
            BOOL  fDacl, fDefault;
            PACL  pacl;

            pSD = atch;
            if (NDDE_NO_ERROR == NDdeGetShareSecurity(NULL, lpLE->name,
                  DACL_SECURITY_INFORMATION, pSD, ByteCountOf(2048),
                  &dwSize))
               {
               if (GetSecurityDescriptorDacl(pSD, &fDacl,
                     &pacl, &fDefault))
                  {
                  if (fDefault || !fDacl)
                     {
                     if (pSD = CurrentUserOnlySD())
                        {
                        NDdeSetShareSecurity(NULL, lpLE->name,
                           DACL_SECURITY_INFORMATION, pSD);

                        LocalFree(pSD);
                        }
                     }
                  }
               }
      #endif

            // We've finished mucking with the share, now set
            // trust info
            PINFO(TEXT("Setting trust info to 0x%lx\r\n"),
               adwTrust[0]);
            NDdeSetTrustedShare(NULL, lpLE->name,
               adwTrust[0]);

            ///////////////////////////////////////////////
            // do the execute to change the server state
            lstrcat(lstrcpy(szBuf,SZCMD_UNSHARE),lpLE->name);
            PINFO(TEXT("sending cmd [%s]\n\r"), szBuf);

            if (MySyncXact ( (LPBYTE)szBuf, ByteCountOf(lstrlen(szBuf) + 1),
                GETMDIINFO(hwndLocal)->hExeConv, 0L, CF_TEXT,
                XTYP_EXECUTE, SHORT_SYNC_TIMEOUT, NULL))
               {
               SetShared(lpLE, FALSE);
               InitializeMenu(GetMenu(hwndApp));
               }
            else
               {
               MessageBoxID ( hInst, hwndApp, IDS_INTERNALERR, IDS_APPNAME,
                   MB_OK | MB_ICONSTOP );
               PERROR(TEXT("Ddeml error %ld during exec\n\r"),
                     DdeGetLastError(idInst));
               }
            }
         }
      else if (NDDE_ACCESS_DENIED == ret)
         {
         MessageBoxID(hInst, hwndApp, IDS_PRIVILEGEERROR, IDS_APPNAME,
               MB_OK | MB_ICONHAND);
         }
      else
         {
         PERROR(TEXT("Error from NDdeShareSetInfo %d\n\r"), ret );
         MessageBoxID ( hInst, hwndApp, IDS_SHARINGERROR,
             IDS_SHAREDLGTITLE, MB_ICONHAND | MB_OK );
         }
      }
   else
      {
      MessageBoxID(hInst, hwndApp, IDS_INTERNALERR, IDS_APPNAME,
           MB_OK | MB_ICONHAND);
      }
   }
}

//
// Purpose: Set the currently selected page in the active MDI window
//    to 'unshared'.
//
// Parameters: None.
//
// Returns: 0L always, function handles its own errors.
//
//////////////////////////////////////////////////////////////////////
LRESULT OnIdmUnshare(
void)
{
DWORD          adwTrust[3];
int            tmp;
WORD           wAddlItems;
PNDDESHAREINFO lpDdeI;
PLISTENTRY     lpLE;
DWORD          ret;

tmp = (int)SendMessage (  pActiveMDI->hWndListbox,
     LB_GETCURSEL, 0, 0L );

if ( tmp != LB_ERR )
   {
   DWORD dwRet = ByteCountOf(2048);

   if (lpDdeI = LocalAlloc(LPTR, ByteCountOf(2048)))
      {
      SendMessage (  pActiveMDI->hWndListbox, LB_GETTEXT, tmp,
          (LPARAM)(LPTSTR)&lpLE);

      AssertConnection(hwndActiveChild);

      PINFO(TEXT("for share [%s]"), lpLE->name);
      wAddlItems = 0;
      ret = NDdeShareGetInfo ( NULL, lpLE->name, 2,
          (LPBYTE)lpDdeI, ByteCountOf(2048), &dwRet, &wAddlItems );
      if (NDDE_NO_ERROR == ret)
         {
         register LPTSTR lpOog;

         lpOog = lpDdeI->lpszAppTopicList;

         // Jump over the first two NULL chars you find-- these
         // are the old- and new-style app/topic pairs, we don't
         // mess with them. Then jump over the next BAR_CHAR you find.
         // The first character after that is the first char of the
         // static topic-- change that to a SHR_CHAR.
         while (*lpOog++)
            {
            }
         while (*lpOog++)
            {
            }
         // BUGBUG: TEXT('|') should == BAR_CHAR. If not, this needs to
         // be adjusted.
         while (*lpOog++ != TEXT('|'))
            {
            }
         *lpOog = UNSHR_CHAR;
         lpDdeI->fSharedFlag = 1L;

         // Have to get trusted share settings before we modify
         // the share, 'cuz they'll be invalid.
         NDdeGetTrustedShare(NULL, lpDdeI->lpszShareName, adwTrust,
               adwTrust + 1, adwTrust + 2);

         DumpDdeInfo(lpDdeI, NULL);
         ret = NDdeShareSetInfo ( NULL, lpDdeI->lpszShareName, 2,
             (LPBYTE)lpDdeI, ByteCountOf(2048), 0 );
         if ( NDDE_NO_ERROR == ret)
            {
      #if 0
            PSECURITY_DESCRIPTOR pSD;
            DWORD dwSize;
            BOOL  fDacl, fDefault;
            PACL  pacl;

            pSD = LocalAlloc(LPTR, 30);
            ret = NDdeGetShareSecurity(NULL, lpDdeI->lpszShareName,
                  DACL_SECURITY_INFORMATION, pSD, 30,
                  &dwSize);
            if (NDDE_NO_ERROR != ret && dwSize < 65535L)
               {
               LocalFree(pSD);
               pSD = LocalAlloc(LPTR, dwSize);

               ret =  NDdeGetShareSecurity(NULL, lpDdeI->lpszShareName,
                     DACL_SECURITY_INFORMATION, pSD, 30,
                     &dwSize);
               }

            if (NDDE_NO_ERROR == ret)
               {
               if (GetSecurityDescriptorDacl(pSD, &fDacl,
                     &pacl, &fDefault))
                  {
                  LocalFree(pSD);

                  if (fDefault || !fDacl)
                     {
                     if (pSD = CurrentUserOnlySD())
                        {
                        NDdeSetShareSecurity(NULL, lpDdeI->lpszShareName,
                           DACL_SECURITY_INFORMATION, pSD);
                        }
                     else
                        {
                        PERROR(TEXT("Couldn't make CUOnlySD"));
                        }
                     }
                  else
                     {
                     PINFO(TEXT("Non-default DACL"));
                     }
                  }
               else
                  {
                  PINFO(TEXT("No DACL"));
                  }
               }
            else
               {
               PERROR(TEXT("Couldn't get security for share"));
               }
            PERROR(TEXT("\r\n"));

            if (pSD)
               {
               LocalFree(pSD);
               }
      #endif
            // Setting trusted share info needs to be the last
            // operation we do on the share.
            if (NDDE_NO_ERROR != NDdeSetTrustedShare(NULL,
                  lpDdeI->lpszShareName, adwTrust[0]))
               {
               PERROR(TEXT("Couldn't set trust status\r\n"));
               }

            ///////////////////////////////////////////////
            // do the execute to change the server state
            lstrcat(lstrcpy(szBuf,SZCMD_UNSHARE),lpLE->name);
            PINFO(TEXT("sending cmd [%s]\n\r"), szBuf);

            if (MySyncXact ( (LPBYTE)szBuf, ByteCountOf(lstrlen(szBuf) + 1),
                GETMDIINFO(hwndLocal)->hExeConv, 0L, CF_TEXT,
                XTYP_EXECUTE, SHORT_SYNC_TIMEOUT, NULL))
               {
               SetShared(lpLE, FALSE);
               InitializeMenu(GetMenu(hwndApp));
               }
            else
               {
               MessageBoxID ( hInst, hwndApp, IDS_INTERNALERR, IDS_APPNAME,
                   MB_OK | MB_ICONSTOP );
               PERROR(TEXT("Ddeml error %ld during exec\n\r"),
                     DdeGetLastError(idInst));
               }
            }
         else if (NDDE_ACCESS_DENIED == ret)
            {
            MessageBoxID(hInst, hwndApp, IDS_PRIVILEGEERROR, IDS_APPNAME,
                  MB_OK | MB_ICONHAND);
            }
         else
            {
            MessageBoxID(hInst, hwndApp, IDS_INTERNALERR, IDS_APPNAME,
                  MB_OK | MB_ICONHAND);
            PERROR(TEXT("Couldn't set share info\r\n"));
            }
         }
      else if (NDDE_ACCESS_DENIED == ret)
         {
         MessageBoxID(hInst, hwndApp, IDS_PRIVILEGEERROR, IDS_APPNAME,
               MB_OK | MB_ICONHAND);
         }
      else
         {
         PERROR(TEXT("Error from NDdeShareSetInfo %d\n\r"), ret );
         MessageBoxID ( hInst, hwndApp, IDS_SHARINGERROR,
             IDS_SHAREDLGTITLE, MB_ICONHAND | MB_OK );
         }
      }
   else
      {
      MessageBoxID(hInst, hwndApp, IDS_INTERNALERR, IDS_APPNAME,
           MB_OK | MB_ICONHAND);
      }
   }
else
   {
   PERROR(TEXT("IDM_UNSHARE w/no page selected\r\n"));
   }
return(0L);
}
//
// Purpose: Process menu commands for the Clipbook Viewer.
//
// Parameters: As wndproc.
//
// Returns: 0L, or DefWindowProc() if wParam isn't a WM_COMMAND id I
//    know about.
//
/////////////////////////////////////////////////////////////////////////
LRESULT ClipBookCommand (
HWND   hwnd,
UINT   msg,
WPARAM wParam,
LPARAM lParam)
{
    int tmp;
    FARPROC lpProc;
    UINT wNewFormat;
    UINT wOldFormat;
    LPLISTENTRY lpLE;
    UINT ret;
    HMENU hSysMenu;
    WINDOWPLACEMENT wpl;
    PMDIINFO pMDIc;
    TCHAR tchTmp;

    switch (LOWORD(wParam))
       {
    case IDM_AUDITING:
       return(EditPermissions(TRUE));
       // return EditAuditing();
       break;

    case IDM_OWNER:
       return EditOwner();
       break;

    case IDM_PERMISSIONS:
       return EditPermissions(FALSE);
       break;

    case IDC_TOOLBAR:
        MenuHelp( WM_COMMAND, wParam, lParam, GetMenu(hwnd), hInst,
              hwndStatus, nIDs );
        break;

    case IDM_EXIT:
        SendMessage (hwnd, WM_CLOSE, 0, 0L);
        break;

    case IDM_TILEVERT:
    case IDM_TILEHORZ:
        SendMessage(hwndMDIClient, WM_MDITILE,
            wParam == IDM_TILEHORZ ? MDITILE_HORIZONTAL : MDITILE_VERTICAL,
            0L);
        break;

    case IDM_CASCADE:
        SendMessage (hwndMDIClient, WM_MDICASCADE, 0, 0L);
        break;

    case IDM_ARRANGEICONS:
        SendMessage (hwndMDIClient, WM_MDIICONARRANGE, 0, 0L);
        break;

    case IDM_COPY:
       {
       HCONV hConv;
       LPLISTENTRY lpLE;
       PMDIINFO pMDIc;
       PDATAREQ pDataReq;

       // make a copy to ensure that the global is not
       // changed from under us in case proc is reentered

       pMDIc = GETMDIINFO(hwndActiveChild);

       tmp = (int)SendMessage ( pMDIc->hWndListbox, LB_GETCURSEL, 0, 0L );

       if ( tmp == LB_ERR )
          break;

       if ( SendMessage ( pMDIc->hWndListbox,
               LB_GETTEXT, tmp, (LPARAM)(LPTSTR)&lpLE) == LB_ERR )
          {
          PERROR(TEXT("IDM_COPY: bad listbox index: %d\n\r"), tmp );
          break;
          }

       if ( !( pDataReq = CreateNewDataReq() ))
          {
          PERROR(TEXT("error from CreateNewDataReq\n\r"));
          break;
          }

       if ( pMDIc->hszClpTopic )
          {
          DdeFreeStringHandle ( idInst, pMDIc->hszClpTopic );
          }

       tchTmp = lpLE->name[0];
       lpLE->name[0] = SHR_CHAR;
       pMDIc->hszClpTopic = DdeCreateStringHandle(idInst, lpLE->name, 0);

       // If we're local, trust the share so we can copy through it
       if (hwndActiveChild == hwndLocal)
          {
          DWORD adwTrust[3];

          NDdeGetTrustedShare(NULL, lpLE->name, adwTrust, adwTrust + 1,
               adwTrust + 2);
          adwTrust[0] |= NDDE_TRUST_SHARE_INIT;

          NDdeSetTrustedShare(NULL, lpLE->name, adwTrust[0]);
          }

       lpLE->name[0] = tchTmp;

       if ( !pMDIc->hszClpTopic )
          {
          MessageBoxID ( hInst, hwndActiveChild, IDS_DATAUNAVAIL, IDS_APPNAME,
              MB_OK | MB_ICONEXCLAMATION );
          break;
          }

       if ( pMDIc->hClpConv )
          {
          DdeDisconnect ( pMDIc->hClpConv );
          }

       hConv = DdeConnect (idInst, pMDIc->hszConvPartner,
           pMDIc->hszClpTopic, NULL);

       if ( !hConv )
          {
          PERROR(TEXT("DdeConnect to (%s) failed %d\n\r"), (LPTSTR)(lpLE->name),
              DdeGetLastError(idInst) );
          MessageBoxID ( hInst, hwndActiveChild, IDS_DATAUNAVAIL, IDS_APPNAME,
              MB_OK | MB_ICONEXCLAMATION );
          break;
          }

       pMDIc->hClpConv = hConv;

       pDataReq->rqType = RQ_COPY;
       pDataReq->hwndList = pMDIc->hWndListbox;
       pDataReq->iListbox = tmp;
       pDataReq->hwndMDI = hwndActiveChild;
       pDataReq->fDisconnect = FALSE;
       pDataReq->wFmt        = CF_TEXT;

       DdeSetUserHandle ( hConv, (DWORD)QID_SYNC, (DWORD)pDataReq );
       DdeKeepStringHandle ( idInst, hszFormatList );

       DdeClientTransaction ( NULL, 0L, hConv, hszFormatList,
           CF_TEXT, XTYP_REQUEST, (DWORD)TIMEOUT_ASYNC, NULL );

       // now a copy request packed will arrive at the call-back... or
       // not. BUGBUG report an error if no callback?
       break;
       }

    case IDM_TOOLBAR:

        if ( fToolBar ) {
            fToolBar = FALSE;
            ShowWindow ( hwndToolbar, SW_HIDE );
            AdjustMDIClientSize();
        }
        else {
            fToolBar = TRUE;
            AdjustMDIClientSize();
            ShowWindow ( hwndToolbar, SW_SHOW );
        }
        break;

    case IDM_STATUSBAR:

        if ( fStatus ) {
            fStatus = FALSE;
            ShowWindow ( hwndStatus, SW_HIDE );
            AdjustMDIClientSize();
        }
        else {
            fStatus = TRUE;
            AdjustMDIClientSize();
            ShowWindow ( hwndStatus, SW_SHOW );
        }
        break;

    case ID_PAGEUP:
    case ID_PAGEDOWN:
    {
        HWND hwndc;
        PMDIINFO pMDIc;
        UINT iLstbox, iLstboxOld;

        // copy to make sure this value doesn't change when we yield
        hwndc = hwndActiveChild;
        pMDIc = GETMDIINFO(hwndc);
        SetFocus ( hwndc );

        // make sure this is not clipboard window...
        if ( pMDIc->flags & F_CLPBRD )
            break;

        // must be in page view
        if ( pMDIc->DisplayMode != DSP_PAGE )
            break;

        iLstbox = (int)SendMessage ( pMDIc->hWndListbox,
            LB_GETCURSEL, 0, 0L );
        if ( iLstbox == LB_ERR )
            break;

        // page up on first entry?
        if ( iLstbox == 0 && wParam == ID_PAGEUP ) {
            MessageBeep(0);
            break;
        }

        // page down on last entry?
        if ( (int)iLstbox == (int)SendMessage(pMDIc->hWndListbox,
            LB_GETCOUNT,0,0L) - 1 && wParam == (WPARAM)ID_PAGEDOWN ) {
            MessageBeep(0);
            break;
        }

        // move selection up/down as appropriate
        iLstboxOld;
        if ( wParam == ID_PAGEDOWN )
            iLstbox++;
        else
            iLstbox--;

        SetListboxEntryToPageWindow ( hwndc, pMDIc, iLstbox );
    }
    break;

    case IDM_LISTVIEW:
    case IDM_PREVIEWS:
       {
       HWND  hwndtmp;
       int   OldSel;
       int   OldDisplayMode;
       DWORD dwDefView;
       DWORD dwDefViewSize;
       TCHAR szBuf[80];

       SetFocus ( hwndActiveChild );

       // make sure this is not clipboard window...
       if ( pActiveMDI->flags & F_CLPBRD )
           break;

       // NOP?
       if ( pActiveMDI->DisplayMode == DSP_PREV && wParam == IDM_PREVIEWS ||
           pActiveMDI->DisplayMode == DSP_LIST && wParam == IDM_LISTVIEW )
           break;

       OldDisplayMode = pActiveMDI->DisplayMode;

       // nuke vclipboard if there is one
       if ( pActiveMDI->pVClpbrd ) {
           DestroyVClipboard( pActiveMDI->pVClpbrd );
           pActiveMDI->pVClpbrd = NULL;
       }

        // Save selection... (extra code to avoid strange lb div-by-zero)
        OldSel = (int)SendMessage( pActiveMDI->hWndListbox,
            LB_GETCURSEL, 0, 0L );
        SendMessage( pActiveMDI->hWndListbox,
            LB_SETCURSEL, (WPARAM)-1, 0L );
        UpdateNofMStatus ( hwndActiveChild );
        SendMessage( pActiveMDI->hWndListbox, WM_SETREDRAW, 0, 0L );

        // set new display mode so listbox will get created right
        pActiveMDI->DisplayMode = ( wParam == IDM_PREVIEWS ) ?
            DSP_PREV : DSP_LIST ;

        // save handle to old listbox
        hwndtmp =  pActiveMDI->hWndListbox;

        // hide the old listbox - will soon destroy
        ShowWindow ( hwndtmp, SW_HIDE );

        // make new listbox and save handle in extra window data
        pActiveMDI->hWndListbox = CreateNewListBox ( hwndActiveChild,
                        ( pActiveMDI->DisplayMode == DSP_PREV ) ?
                        LBS_PREVIEW : LBS_LISTVIEW );

        // loop, extracting items from one box and into other
        while ( SendMessage( hwndtmp, LB_GETTEXT, 0,
                                (LPARAM)(LPTSTR)&lpLE ) != LB_ERR ) {

            // mark this item not to be deleted in WM_DELETEITEM
            lpLE->fDelete = FALSE;
            // remove from listbox
            SendMessage ( hwndtmp, LB_DELETESTRING, 0, 0L );
            // reset fDelete flag
            lpLE->fDelete = TRUE;

            // add to new listbox
            SendMessage (  pActiveMDI->hWndListbox,
                LB_ADDSTRING, 0, (DWORD)(LPCSTR)lpLE );
        }

        // kill old (empty) listbox
        DestroyWindow ( hwndtmp );

        if ( pActiveMDI->flags & F_LOCAL )
           {
           SetWindowText ( hwndLocal, szLocalClpBk );
           lstrcpy(szBuf, szDefView);
           }
        else
           {
           wsprintf(szBuf, szClipBookOnFmt, pActiveMDI->szBaseName);
           SetWindowText ( hwndActiveChild, szBuf );
           lstrcpy(szBuf, pActiveMDI->szBaseName);
           lstrcat(szBuf, szConn);
           }

        if (NULL != hkeyRoot)
           {
           DWORD dwValue;

           dwValue = pActiveMDI->DisplayMode == DSP_LIST ? IDM_LISTVIEW :
                     pActiveMDI->DisplayMode == DSP_PREV ? IDM_PREVIEWS :
                     IDM_PAGEVIEW;

           RegSetValueEx(hkeyRoot, (LPTSTR)szBuf, 0L, REG_DWORD,
                 (LPBYTE)&dwValue, sizeof(DWORD));
           }

        // adjust size and show
        AdjustControlSizes( hwndActiveChild );
        ShowHideControls ( hwndActiveChild );

        // restore selection
        SendMessage( pActiveMDI->hWndListbox, LB_SETCURSEL,
            OldSel, 0L );
        UpdateNofMStatus ( hwndActiveChild );

        InitializeMenu ( GetMenu(hwndApp) );
        SetFocus ( pActiveMDI->hWndListbox );
        break;
       }
    case IDM_PAGEVIEW:
       {
        HWND hwndc;
        PMDIINFO pMDIc;

        // copy to make sure this value doesn't change when we yield
        hwndc = hwndActiveChild;
        pMDIc = GETMDIINFO(hwndc);
        SetFocus ( hwndc );

        // make sure this is not clipboard window...
        if ( pMDIc->flags & F_CLPBRD )
            break;

        // already in page view?
        if ( pMDIc->DisplayMode == DSP_PAGE )
            break;

        tmp = (int)SendMessage ( pMDIc->hWndListbox,
            LB_GETCURSEL, 0, 0L );
        if ( tmp == LB_ERR )
            break;

        SetListboxEntryToPageWindow ( hwndc, pMDIc, tmp );
        break;
       }

    case IDM_SHARE:
       tmp = (WPARAM)SendMessage (  pActiveMDI->hWndListbox,LB_GETCURSEL, 0, 0L );

       if ( tmp != LB_ERR )
          {
          SendMessage (pActiveMDI->hWndListbox, LB_GETTEXT, tmp, (LPARAM)(LPTSTR)&lpLE);

          // We create the NetDDE share when we create the page, not when we
          // share it. Thus, we're always 'editing the properties' of an existing
          // share, even if the user thinks that he's sharing the page NOW.
          Properties(hwnd, lpLE);

          // Redraw the listbox.
          if (pActiveMDI->DisplayMode == DSP_PREV)
             {
             InvalidateRect(pActiveMDI->hWndListbox, NULL, FALSE);
             }
          else
             {
             SendMessage(pActiveMDI->hWndListbox,LB_SETCURSEL, tmp, 0L);
             UpdateNofMStatus(hwndActiveChild);
             }
          }
       break;

    case IDM_CLPWND:
       CreateClipboardWindow();
       break;

    case IDM_LOCAL:
       CreateLocalWindow();
       break;

    case IDM_UNSHARE:
       return(OnIdmUnshare());
       break;

    case IDM_DELETE:
       return OnIDMDelete(hwnd, msg, wParam, lParam);
       break;

    case IDM_KEEP:
       return OnIDMKeep(hwnd, msg, wParam, lParam);
       break;

    case IDM_SAVEAS:
       {
       OPENFILENAME ofn;
       LPFNGOF lpfnSOF;
       HMODULE hMod;
       TCHAR szFile[MAX_FILENAME_LENGTH + 1];

       if ( CountClipboardFormats())
          {
          szFile[0] = TEXT('\0');
          // Initialize the OPENFILENAME members
          ofn.lStructSize = sizeof(OPENFILENAME);
          ofn.hwndOwner = hwnd;
          ofn.lpstrFilter = szFilter;
          ofn.lpstrCustomFilter = (LPTSTR) NULL;
          ofn.nMaxCustFilter = 0L;
          ofn.nFilterIndex = 1;
          ofn.lpstrFile = szFile;
          ofn.nMaxFile = CharSizeOf(szFile);
          ofn.lpstrFileTitle = NULL;
          ofn.nMaxFileTitle = 0L;
          ofn.lpstrInitialDir = szDirName;
          ofn.lpstrTitle = (LPTSTR) NULL;
          ofn.Flags = OFN_HIDEREADONLY | OFN_NOREADONLYRETURN |
                      OFN_OVERWRITEPROMPT;
          ofn.lpstrDefExt = TEXT("CLP");

          if (hMod = LoadLibrary(TEXT("COMDLG32.DLL")))
             {
             if (lpfnSOF = (LPFNGOF)GetProcAddress(hMod,
                        "GetSaveFileNameW"
                     ))
                {
                if ((*lpfnSOF)(&ofn) && szFile[0])
                   {
                   // NOTE must force all formats rendered!
                   ForceRenderAll ( hwnd, NULL );

                   AssertConnection ( hwndLocal );

                   // If user picked first filter ("NT Clipboard"), use save as..
                   // other filters would use save as old.
                   wsprintf(szBuf, TEXT("%s%s"),
                        (ofn.nFilterIndex == 1) ? SZCMD_SAVEAS : SZCMD_SAVEASOLD,
                        szFile );
                   if (!MySyncXact ( (LPBYTE)szBuf, ByteCountOf(lstrlen(szBuf) +1),
                       GETMDIINFO(hwndLocal)->hExeConv, 0L, CF_TEXT,
                       XTYP_EXECUTE, LONG_SYNC_TIMEOUT, NULL))
                      {
                      MessageBoxID(hInst, hwnd, IDS_INTERNALERR,
                           IDS_APPNAME, MB_OK | MB_ICONHAND);
                      }
                   }
                // Else user cancelled dialog or didn't enter a name.
                }
             else
                {
                MessageBoxID(hInst, hwnd, IDS_INTERNALERR,
                     IDS_APPNAME, MB_OK | MB_ICONHAND);
                }
             FreeLibrary(hMod);
             }
          }
       break;
       }
    case IDM_OPEN:
        {
        OPENFILENAME ofn;
        LPFNGOF lpfnGOF;
        HMODULE hMod;
        TCHAR szFile[128] = TEXT("*.clp");

        // Initialize the OPENFILENAME members
        ofn.lStructSize = sizeof(OPENFILENAME);
        ofn.hwndOwner = hwnd;
        ofn.lpstrFilter = szFilter;
        ofn.lpstrCustomFilter = (LPTSTR) NULL;
        ofn.nMaxCustFilter = 0L;
        ofn.nFilterIndex = 1;
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = CharSizeOf(szFile);
        ofn.lpstrFileTitle = NULL;
        ofn.nMaxFileTitle = 0L;
        ofn.lpstrInitialDir = szDirName;
        ofn.lpstrTitle = (LPTSTR) NULL;
        ofn.Flags = OFN_HIDEREADONLY | OFN_PATHMUSTEXIST;
        ofn.lpstrDefExt = TEXT("CLP");

        if (hMod = LoadLibrary(TEXT("COMDLG32.DLL")))
           {
           if (lpfnGOF = (LPFNGOF)GetProcAddress(hMod,
                "GetOpenFileNameW"
              ))
              {
              if ((*lpfnGOF)(&ofn) && szFile[0])
                 {
                 // prompt for clearing clipboard
                 if (ClearClipboard(hwnd))
                    {
                    AssertConnection ( hwndLocal );

                    wsprintf(szBuf,TEXT("%s%s"), (LPTSTR)SZCMD_OPEN, szFile );
                    MySyncXact ((LPBYTE)szBuf, ByteCountOf(lstrlen(szBuf) +1),
                       GETMDIINFO(hwndLocal)->hExeConv, 0L, CF_TEXT,
                       XTYP_EXECUTE, SHORT_SYNC_TIMEOUT, NULL );
                    InitializeMenu ( GetMenu(hwnd) );
                    }
                 // Else user didn't want to clear existing clipboard
                 }
              // Else user cancelled "open" dialog
              }
           else
              {
              PERROR(TEXT("Couldn't get proc!\r\n"));
              }
           FreeLibrary(hMod);
           }
        else
           {
           PERROR(TEXT("Couldn't find COMDLG32.DLL!\r\n"));
           }

        break;
        }

    case IDM_DISCONNECT:

        // don't allow close of local or clipboard window
        if ( pActiveMDI->flags & ( F_LOCAL | F_CLPBRD ))
            break;
        SendMessage ( hwndActiveChild, WM_CLOSE, 0, 0L );
        break;

    case IDM_CONNECT:
       {
       WCHAR rgwch[MAX_COMPUTERNAME_LENGTH + 1];
       BOOL  bOK = FALSE;
       BOOL  fFoundLMDlg = FALSE;
       HMODULE hMod;
       LPFNSYSFOCUS lpfn;

       *szConvPartner = TEXT('\0');
       rgwch[0] = L'\0';

       if (hMod = LoadLibraryW(L"NTLANMAN.DLL"))
          {
          if (lpfn = (LPFNSYSFOCUS)GetProcAddress(hMod, "I_SystemFocusDialog"))
             {
             fFoundLMDlg = TRUE;
             (*lpfn)(hwnd, FOCUSDLG_SERVERS_ONLY, rgwch,
                  MAX_COMPUTERNAME_LENGTH, &bOK, szHelpFile, IDH_CONNECT);

             if (IDOK == bOK)
                {
                lstrcpy(szConvPartner, rgwch);
                }
             else
                {
                szConvPartner[0] = TEXT('\0');
                }
             }
          else
             {
             PERROR(TEXT("Couldn't find connect proc!\r\n"));
             }
          FreeLibrary(hMod);
          }
       else
          {
          PERROR(TEXT("Couldn't find NTLANMAN.DLL\r\n"));
          }

       // If we didn't find the fancy LanMan dialog, we still can get
       // by with our own cheesy version-- 'course, ours comes up faster, too.
       if (!fFoundLMDlg)
          {
          bOK = DialogBox(hInst, MAKEINTRESOURCE(IDD_CONNECT), hwnd,
               ConnectDlgProc);
          }

       if ( *szConvPartner )
          {
          CreateNewRemoteWindow ( szConvPartner, TRUE );
          }
       // Else user entered "empty" computername

       UpdateWindow ( hwnd );
       break;
       }

    case IDM_REFRESH:

#if DEBUG
        {
        DWORD cbDBL = sizeof(DebugLevel);

        RegQueryValueEx(hkeyRoot, szDebug, NULL, NULL,
            (LPBYTE)&DebugLevel, &cbDBL);
        }
#endif
        if ( pActiveMDI->flags & F_CLPBRD )
            break;
        AssertConnection ( hwndActiveChild );
        UpdateListBox ( hwndActiveChild, pActiveMDI->hExeConv );
        break;

    case IDM_CONTENTS:
        WinHelp(hwnd, szHelpFile, HELP_INDEX,  0L );
        break;

    case IDM_SEARCHHELP:
        WinHelp(hwnd, szHelpFile, HELP_PARTIALKEY, (DWORD)szNull );
        break;

    case IDM_HELPHELP:
        WinHelp(hwnd, (LPTSTR)NULL, HELP_HELPONHELP, 0);
        break;

    case IDM_ABOUT:
       {
       HMODULE hMod;
       LPFNSHELLABOUT lpfn;

       if (hMod = LoadLibrary(TEXT("SHELL32")))
          {
          if (lpfn = (LPFNSHELLABOUT)GetProcAddress(hMod,
             #ifdef UNICODE
               "ShellAboutW"
             #else
               "ShellAboutA"
             #endif
             ))
             {
             (*lpfn)(hwnd, szAppName, szNull,
                  LoadIcon(hInst, (LPTSTR) MAKEINTRESOURCE(IDFRAMEICON)));
             }
          FreeLibrary(hMod);
          }
       else
          {
          PERROR(TEXT("Couldn't get SHELL32.DLL\r\n"));
          }
       }
       break;

    case CBM_AUTO:
    case CF_PALETTE:
    case CF_TEXT:
    case CF_BITMAP:
    case CF_METAFILEPICT:
    case CF_SYLK:
    case CF_DIF:
    case CF_TIFF:
    case CF_OEMTEXT:
    case CF_DIB:
    case CF_OWNERDISPLAY:
    case CF_DSPTEXT:
    case CF_DSPBITMAP:
    case CF_DSPMETAFILEPICT:
    case CF_PENDATA:
    case CF_RIFF:
    case CF_WAVE:
    case CF_ENHMETAFILE:
    case CF_UNICODETEXT:
    case CF_DSPENHMETAFILE:

       if ( pActiveMDI->CurSelFormat != wParam)
          {
          CheckMenuItem(hDispMenu, pActiveMDI->CurSelFormat,
              MF_BYCOMMAND | MF_UNCHECKED);
          CheckMenuItem(hDispMenu, wParam, MF_BYCOMMAND | MF_CHECKED);

          DrawMenuBar(hwnd);

          wOldFormat = GetBestFormat( hwndActiveChild, pActiveMDI->CurSelFormat);
          wNewFormat = GetBestFormat( hwndActiveChild, wParam);

          if (wOldFormat == wNewFormat)
             {
             /* An equivalent format is selected; No change */
             pActiveMDI->CurSelFormat = wParam;
             }
          else
             {
             /* A different format is selected; So, refresh... */

             /* Change the character sizes based on new format. */
             ChangeCharDimensions(hwndActiveChild, wOldFormat, wNewFormat);

             pActiveMDI->fDisplayFormatChanged = TRUE;
             pActiveMDI->CurSelFormat = wParam;

             // NOTE OwnerDisplay stuff applies only to the "real" clipboard!

             if (wOldFormat == CF_OWNERDISPLAY)
                {
                /* Save the owner Display Scroll info */
                SaveOwnerScrollInfo(hwndClpbrd);
                ShowScrollBar ( hwndClpbrd, SB_BOTH, FALSE );
                ShowHideControls(hwndClpbrd);
                ResetScrollInfo( hwndActiveChild );
                InvalidateRect ( hwndActiveChild, NULL, TRUE );
                break;
                }

             if (wNewFormat == CF_OWNERDISPLAY)
                {
                /* Restore the owner display scroll info */
                ShowHideControls(hwndClpbrd);
                ShowWindow ( pActiveMDI->hwndSizeBox, SW_HIDE );
                RestoreOwnerScrollInfo(hwndClpbrd);
                InvalidateRect ( hwndActiveChild, NULL, TRUE );
                break;
                }

             InvalidateRect ( hwndActiveChild, NULL, TRUE );
             ResetScrollInfo( hwndActiveChild );
             }
          }
       break;

    default:
        return DefFrameProc ( hwnd,hwndMDIClient,msg,wParam,lParam);
    }
    // return DefFrameProc ( hwnd,hwndMDIClient,msg,wParam,lParam);
    return 0;
}

BOOL SetListboxEntryToPageWindow(
HWND hwndc,
PMDIINFO pMDIc,
int lbindex )
{
HCONV hConv;
LPLISTENTRY lpLE;
PVCLPBRD pVClp;
PDATAREQ pDataReq;
BOOL     fOK = FALSE;
TCHAR    tchTmp;

// get listbox entry data

if (LB_ERR != SendMessage (pMDIc->hWndListbox, LB_GETTEXT, lbindex,
         (LPARAM)(LPTSTR)&lpLE)
    && lpLE
    && (pDataReq = CreateNewDataReq()))
   {
   // make new clipboard
   if (pVClp = CreateVClipboard(hwndc))
      {
      // nuke previous vclipboard if any
      if ( pMDIc->pVClpbrd )
         DestroyVClipboard( pMDIc->pVClpbrd );
      pMDIc->pVClpbrd = pVClp;

      // Set up $<page name> for topic
      if ( pMDIc->hszClpTopic )
         DdeFreeStringHandle ( idInst, pMDIc->hszClpTopic );
      tchTmp = lpLE->name[0];
      lpLE->name[0] = SHR_CHAR;
      pMDIc->hszVClpTopic =
         DdeCreateStringHandle ( idInst, lpLE->name, 0 );
      lpLE->name[0] = tchTmp;

      if ( pMDIc->hszVClpTopic )
         {
         if ( pMDIc->hVClpConv )
            DdeDisconnect ( pMDIc->hVClpConv );
         hConv = DdeConnect (idInst, pMDIc->hszConvPartner,
            pMDIc->hszVClpTopic, NULL);

         if (hConv)
            {
            pMDIc->hVClpConv = hConv;

            DdeKeepStringHandle ( idInst, hszFormatList );

            pDataReq->rqType      = RQ_SETPAGE;
            pDataReq->hwndList    = pMDIc->hWndListbox;
            pDataReq->iListbox    = lbindex;
            pDataReq->hwndMDI     = hwndc;
            pDataReq->fDisconnect = FALSE;
            pDataReq->wFmt        = CF_TEXT;

            DdeSetUserHandle ( hConv, (DWORD)QID_SYNC, (DWORD)pDataReq );

            DdeClientTransaction ( NULL, 0L, hConv, hszFormatList,
               CF_TEXT, XTYP_REQUEST, (DWORD)TIMEOUT_ASYNC, NULL );

            fOK = TRUE;
            }
         else
            {
            PERROR(TEXT("DdeConnect for Vclip failed: %x\n\r"),
                DdeGetLastError(idInst) );
            }
         }
      else
         {
         PERROR(TEXT("Couldn't make string handle for %s\r\n"),
               lpLE->name);
         }
      }
   else
      {
      PERROR(TEXT("Failed to create Vclipboard\n\r"));
      }
   }
else
   {
   PERROR(TEXT("error from CreateNewDataReq\n\r"));
   }

if (!fOK)
   {
   MessageBoxID ( hInst, hwndc, IDS_INTERNALERR, IDS_APPNAME,
       MB_OK | MB_ICONSTOP );
   }
return(fOK);
}
