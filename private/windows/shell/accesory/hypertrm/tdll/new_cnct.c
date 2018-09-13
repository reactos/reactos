/* File: \WACKER\TDLL\new_cnct.c (Created: 2-Feb-1994)
 *
 * Copyright 1990,1995 by Hilgraeve Inc. -- Monroe, MI
 * All rights reserved
 *
 * $Revision: 3 $
 * $Date: 3/26/99 8:07a $
 */

#include <windows.h>
#pragma hdrstop

#include <commctrl.h>

extern BOOL WINAPI SetWindowStyle(HWND hwnd, DWORD style, BOOL fExtended);
int gnrlPickIconDlg(HWND hDlg);

#include <term\res.h>

#include "stdtyp.h"
#include "session.h"
#include "mc.h"
#include "globals.h"
#include "misc.h"
#include "tdll.h"
#include "tchar.h"
#include "errorbox.h"
#include "assert.h"
#include "hlptable.h"

#if !defined(DlgParseCmd)
#define DlgParseCmd(i,n,c,w,l) i=LOWORD(w);n=HIWORD(w);c=(HWND)l;
#endif

struct stSaveDlgStuff
   {
   /*
    * Put in whatever else you might need to access later
    */
   HSESSION hSession;

   TCHAR  achSessName[256];
   };

typedef  struct stSaveDlgStuff SDS;

#define  IDC_IC_ICON    101
#define IDC_TF_NAME  102
#define IDC_LB_NAME     105
#define  IDC_EF_NAME    106
#define IDC_TF_ICON     107
#define  IDC_LB_LIST    108
#define  IDC_PB_BROWSE  109

#define  NC_CUT1     103
#define  NC_CUT2     110

// Design change - 4/14/94: Don't show Wackers New Connection icon
// in selection list. - mrw
//
#define ICON_COUNT   16

BOOL NCD_WM_DRAWITEM(LPDRAWITEMSTRUCT pD);
BOOL NCD_WM_COMPAREITEM(LPCOMPAREITEMSTRUCT pC);
BOOL NCD_WM_DELETEITEM(LPDELETEITEMSTRUCT pD);
BOOL NCD_WM_MEASUREITEM(LPMEASUREITEMSTRUCT pM);

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:   NewConnectionDlg
 *
 * DESCRIPTION: Dialog manager stub
 *
 * ARGUMENTS:  Standard Windows dialog manager
 *
 * RETURNS:    Standard Windows dialog manager
 *
 */
INT_PTR CALLBACK NewConnectionDlg(HWND hDlg, UINT wMsg, WPARAM wPar, LPARAM lPar)
   {
   HWND  hwndChild;
   INT      nId;
   INT   nNtfy, fBad;
   SDS    *pS;
   int   nLoop;
   HWND  hwnd;
   BOOL  fRc;
   DWORD dwMaxComponentLength;
   DWORD dwFileSystemFlags;
   HICON hIcon;
   int   nSelected;
   int   cy;
   RECT  rc;
#if defined(CHAR_MIXED)
    BOOL    bLeadByte = FALSE;
#endif               

   static  fLongNamesSupported;
   static   DWORD aHlpTable[] = {   IDC_EF_NAME,   IDH_TERM_NEWCONN_NAME,
                           IDC_LB_NAME,    IDH_TERM_NEWCONN_NAME,
                           //IDC_TF_NAME,   IDH_TERM_NEWCONN_NAME,
                                    IDC_TF_ICON,   IDH_TERM_NEWCONN_ICON,
                           IDC_LB_LIST,   IDH_TERM_NEWCONN_ICON,
                           //IDC_IC_ICON,   IDH_TERM_NEWCONN_ICON,
                                    IDCANCEL,                           IDH_CANCEL,
                                    IDOK,                               IDH_OK,
                           0,          0};

   static const TCHAR *apszBadNames[] =
      {
      "LPT1", "LPT2", "LPT3", "LPT4", "COM1", "COM2", "COM3", "COM4",
      "COM5", "COM6", "COM7", "COM8", "EPT", "NUL", "PRN", "CLOCK$",
      "CON", "AUX", NULL
      };

#if defined(FAR_EAST)
   TCHAR ach[2 * FNAME_LEN];
#else
   TCHAR ach[FNAME_LEN];
#endif
   LPTSTR   psz;

   switch (wMsg)
      {
   case WM_INITDIALOG:
      pS = (SDS *)malloc(sizeof(SDS));

      // Set no matter what so we can always free
      //
      SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)pS);

      if (pS == (SDS *)0)
         {
            /* TODO: decide if we need to display an error here */
         EndDialog(hDlg, FALSE);
         break;
         }

      pS->hSession = (HSESSION)lPar;
      mscCenterWindowOnWindow(hDlg, GetParent(hDlg));

      // Determine whether long filenames are supported.  JRJ  12/94
      fRc = GetVolumeInformation(NULL,  // pointer to root dir path buffer
                         NULL,     // pointer to volume name buffer
                         0,        // length of volume name buffer
                         NULL,    // pointer to volume serial number buffer
                         &dwMaxComponentLength, // the prize - what I'm after
                         &dwFileSystemFlags,  // ptr to file system flag DWORD
                         NULL,     // pointer to file system name buffer
                         0);    // length of file system name buffer

      if(dwMaxComponentLength == 255)
         {
         // There is support for long file names.

         // Allow a max name lenght of 249.  That's 255 minus the
         // extension length (3), minus the smallest path length,
         // (i.e. C:\) also (3).
         //
         SendDlgItemMessage(hDlg, IDC_EF_NAME, EM_SETLIMITTEXT, 249, 0);
         fLongNamesSupported = TRUE;
         }
      else
         {
         // There IS NOT support for long file names. Limit to eight.
         SendDlgItemMessage(hDlg, IDC_EF_NAME, EM_SETLIMITTEXT, 8, 0);
         fLongNamesSupported = FALSE;
         }

      // This dialog may also be called to change the session icon,
      // so display the name and icon if we already have one.
      //
      sessQueryName(pS->hSession, ach, sizeof(ach));
      StrCharCopy(pS->achSessName, ach);
      if (!sessIsSessNameDefault(ach))
         {
         SetDlgItemText(hDlg, IDC_EF_NAME, ach);
         mscModifyToFit(GetDlgItem(hDlg, IDC_TF_NAME), ach);
         SetDlgItemText(hDlg, IDC_TF_NAME, ach);
         }
      else if (ach[0] != TEXT('\0'))
         {
         SetDlgItemText(hDlg, IDC_TF_NAME, ach);

         // Set the new connection icon ID, if it's a new connection.
         // --jcm 2/23/95.
         //
         sessSetIconID(pS->hSession, IDI_PROG);
         }

      hIcon = sessQueryIcon(pS->hSession);

      if (hIcon != (HICON)0)
         SendDlgItemMessage(hDlg, IDC_IC_ICON, STM_SETICON,
            (WPARAM)hIcon, 0);

      /* Fiddle with the list box */
      hwnd = GetDlgItem(hDlg, IDC_LB_LIST);

      SendMessage(hwnd,
               LB_SETCOLUMNWIDTH,
               GetSystemMetrics(SM_CXICON) + 12,
               0L);

      /* compute the height of the listbox based on icon dimensions */
      GetClientRect(hwnd, &rc);
      cy = GetSystemMetrics(SM_CYICON);
      cy += GetSystemMetrics(SM_CYHSCROLL);
      cy += GetSystemMetrics(SM_CYEDGE) * 3;
      SetWindowPos(hwnd,
               NULL,
               0, 0,
               rc.right, cy,
               SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);

      /* It's an owner drawn list box, just set the ICON ids for later */
      nSelected = FALSE;
      for (nLoop = 0; nLoop < ICON_COUNT; nLoop += 1)
         {
         SendMessage(hwnd,
                  LB_INSERTSTRING,
                  nLoop,
                  (LPARAM)"Hilgraeve is Great !!!");

         // Design change - 4/14/94: Don't show Wackers New Connection
         // icon in selection list. - mrw
         //
         SendMessage(hwnd,
                  LB_SETITEMDATA,
                  nLoop,
                  nLoop + IDI_PROG1);

         if (sessQueryIconID(pS->hSession) == (IDI_PROG1 + nLoop))
            {
            SendMessage(hwnd,
                     LB_SETCURSEL,
                     nLoop, 0L);
            nSelected = TRUE;
            }
         }

      if (!nSelected)
         {
         SendMessage(hwnd,
                  LB_SETCURSEL,
                  0, 0L);
         }
      break;

   case WM_DRAWITEM:
      if (wPar == IDC_LB_LIST)
         return NCD_WM_DRAWITEM((LPDRAWITEMSTRUCT)lPar);
      break;

   case WM_COMPAREITEM:
      if (wPar == IDC_LB_LIST)
         return NCD_WM_COMPAREITEM((LPCOMPAREITEMSTRUCT)lPar);
      break;

   case WM_DELETEITEM:
      if (wPar == IDC_LB_LIST)
         return NCD_WM_DELETEITEM((LPDELETEITEMSTRUCT)lPar);
      break;

   case WM_MEASUREITEM:
      if (wPar == IDC_LB_LIST)
         return NCD_WM_MEASUREITEM((LPMEASUREITEMSTRUCT)lPar);
      break;

   case WM_CONTEXTMENU:
        doContextHelp(aHlpTable, wPar, lPar, TRUE, TRUE);
      break;

   case WM_HELP:
        doContextHelp(aHlpTable, wPar, lPar, FALSE, FALSE);
      break;

   case WM_DESTROY:
      pS = (SDS *)GetWindowLongPtr(hDlg, DWLP_USER);

      if (pS)
         {
         free(pS);
         pS = NULL;
         }

      break;

   case WM_COMMAND:
      /*
       * Did we plan to put a macro in here to do the parsing ?
       */
      DlgParseCmd(nId, nNtfy, hwndChild, wPar, lPar);

      switch (nId)
         {
      case IDOK:
         pS = (SDS *)GetWindowLongPtr(hDlg, DWLP_USER);
         fBad = FALSE;

         // Set session name and icon.
         //
         ach[0] = TEXT('\0');
         nSelected = GetDlgItemText(hDlg, IDC_EF_NAME, ach, sizeof(ach) / sizeof(TCHAR));

         if (ach[0] == TEXT('\0'))
            {
            MessageBeep(MB_ICONHAND);
            SetFocus(GetDlgItem(hDlg, IDC_EF_NAME));
            break;
            }

         else
            {
            // For now let's not allow the user to enter the fully
            // qualified name for a session name (i.e., do not allow any
            // of the valid path characters, '\', ':', and '.'.
            // At a later time we may want to actually interpret the path...
            // -jac 10-06-94 03:52pm
            //
            for (psz = ach; *psz && fBad == FALSE; psz = StrCharNext(psz))
               {
               switch (*psz)
                  {
               case TEXT('\\'):
               case TEXT('/'):
               case TEXT(':'):
               case TEXT('*'):
               case TEXT('?'):
               case TEXT('"'):
               case TEXT('<'):
               case TEXT('>'):
               case TEXT('|'):
                  {
#if defined(CHAR_MIXED)
                        if ( IsDBCSLeadByte(*psz) )
                            {
                            bLeadByte = TRUE;
                            break;
                            }
                        else if ( bLeadByte )
                            {
                            bLeadByte = FALSE;
                            break;
                            }
#endif               
                        LoadString(glblQueryDllHinst(),IDS_GNRL_INVALID_CHARS,
                      ach, sizeof(ach) / sizeof(TCHAR));

                  TimedMessageBox(hDlg, ach, 0,
                     MB_OK | MB_ICONEXCLAMATION, 0);

                  SetFocus(GetDlgItem(hDlg, IDC_EF_NAME));
                  fBad = TRUE;
                  break;
                  }

               default:
                  break;
                  }
               }

            if (fBad)
               break;

         TCHAR_Trim(ach);

         // Check the name agains known device names.
         //
         fBad = FALSE;

         for(nLoop = 0; apszBadNames[nLoop] != NULL; nLoop++)
            {
                                if (_stricmp(apszBadNames[nLoop], ach) == 0)
               {
               LoadString(glblQueryDllHinst(),IDS_GNRL_INVALID_NAME,
                  ach, sizeof(ach) / sizeof(TCHAR));

               TimedMessageBox(hDlg, ach, 0,
                  MB_OK | MB_ICONEXCLAMATION, 0);

               SetFocus(GetDlgItem(hDlg, IDC_EF_NAME));
               fBad = TRUE;
               break;
               }
            }

         if (fBad)
            break;

         sessSetName(pS->hSession, ach);
         sessUpdateAppTitle(pS->hSession);

         /*
          * Check and see if a new Icon has been selected
          */
         hwnd = GetDlgItem(hDlg, IDC_LB_LIST);
         assert(hwnd);

         if (hwnd)
            {
            /* Get the ICON from the list box */
            nLoop = (int)SendMessage(hwnd, LB_GETCURSEL, 0, 0L);

            if (nLoop == LB_ERR)
               nLoop = 0;

            nLoop = (int)SendMessage(hwnd, LB_GETITEMDATA, nLoop, 0);
            sessSetIconID(pS->hSession, nLoop);

            PostMessage(sessQueryHwnd(pS->hSession), WM_SETICON,
               (WPARAM)TRUE, (LPARAM)sessQueryIcon(pS->hSession));
            }

         EndDialog(hDlg, TRUE);
         break;

      case IDCANCEL:
         pS = (SDS *)GetWindowLongPtr(hDlg, DWLP_USER);

         sessQueryName(pS->hSession, ach, sizeof(ach));
         if (!sessIsSessNameDefault(ach))
            {
            sessSetName(pS->hSession, pS->achSessName);
            }

         EndDialog(hDlg, FALSE);
         break;

      default:
         return FALSE;
         }
      }
      break; // WM_COMMAND

   default:
      return FALSE;
      }

   return TRUE;
   }

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 * NCD_WM_DRAWITEM
 *
 * DESCRIPTION:
 * This function is called when the owner drawn list box used to display
 * ICONs sends its parent a WM_DRAWITEM message.
 *
 * ARGUMENTS:
 * pD -- pointer to the draw structure
 *
 * RETURNS:
 *
 */
BOOL NCD_WM_DRAWITEM(LPDRAWITEMSTRUCT pD)
   {
   int x, y;
   HICON hicon;
   DWORD dwOldLayout;

   //hicon = LoadIcon(glblQueryDllHinst(), MAKEINTRESOURCE(pD->itemData));
   hicon = extLoadIcon(MAKEINTRESOURCE(pD->itemData));

   if (hicon == (HICON)0)
      return FALSE;

   if (pD->itemState & ODS_SELECTED)
      SetBkColor(pD->hDC, GetSysColor(COLOR_HIGHLIGHT));
   else
      SetBkColor(pD->hDC, GetSysColor(COLOR_WINDOW));
   /* repaint the selection state */
   ExtTextOut(pD->hDC, 0, 0, ETO_OPAQUE, &pD->rcItem, NULL, 0, NULL);

   x = (pD->rcItem.left + pD->rcItem.right - GetSystemMetrics(SM_CXICON)) / 2;
   y = (pD->rcItem.top + pD->rcItem.bottom - GetSystemMetrics(SM_CYICON)) / 2;
   /* Bug #345406 : Don't mirror the icon. */
   dwOldLayout = GetLayout(pD->hDC);
   if (dwOldLayout && dwOldLayout !=GDI_ERROR) {
       SetLayout(pD->hDC, dwOldLayout | LAYOUT_BITMAPORIENTATIONPRESERVED);
   }
   DrawIcon(pD->hDC, x, y, hicon);
   if (dwOldLayout && dwOldLayout !=GDI_ERROR) {
       SetLayout(pD->hDC, dwOldLayout);
   }
   /* if it has the focus, draw the focus */
   if (pD->itemState & ODS_FOCUS)
       DrawFocusRect(pD->hDC, &pD->rcItem);

   return TRUE;
   }

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 * NCD_WM_COMPAREITEM
 *
 * DESCRIPTION:
 * This function is called when the owner drawn list box used to display
 * ICONs sends its parent a WM_COMPAREITEM message.
 *
 * ARGUMENTS:
 * pC -- pointer to the structure to fill in.
 *
 * RETURNS:
 * ZERO -- they all compare the same.
 *
 */
BOOL NCD_WM_COMPAREITEM(LPCOMPAREITEMSTRUCT pC)
   {
   return 0;
   }

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 * NCD_WM_DELETEITEM
 *
 * DESCRIPTION:
 * This function is called when the owner drawn list box used to display
 * ICONs sends its parent a WM_DELETEITEM message.
 *
 * ARGUMENTS:
 * pD -- pointer to the structure to fill in.
 *
 * RETURNS:
 * TRUE;
 *
 */
BOOL NCD_WM_DELETEITEM(LPDELETEITEMSTRUCT pD)
   {

   return TRUE;
   }

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 * NCD_WM_MEASUREITEM
 *
 * DESCRIPTION:
 * This function is called when the owner drawn list box used to display
 * ICONs sends its parent a WM_MEASUREITEM message.  It fills in the
 * structure and returns.
 *
 * ARGUMENTS:
 * pM -- pointer to the structure to fill in.
 *
 * RETURNS:
 * TRUE.
 *
 */
BOOL NCD_WM_MEASUREITEM(LPMEASUREITEMSTRUCT pM)
   {

   pM->itemWidth = GetSystemMetrics(SM_CXICON);
   pM->itemWidth += 12;

   pM->itemHeight = GetSystemMetrics(SM_CYICON);
   pM->itemHeight += 4;

   return TRUE;
   }

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 * DisplayError
 *
 * DESCRIPTION:
 * Displays and error message.
 *
 * ARGUMENTS:
 * hwnd  - dialog box handle
 * idText   - id of text
 * idTitle - id of title
 *
 * RETURNS:
 *
 * AUTHOR: Mike Ward, 19-Jan-1995
 */
static void DisplayError(const HWND hwnd, const int idText, const int idTitle)
   {

   }
