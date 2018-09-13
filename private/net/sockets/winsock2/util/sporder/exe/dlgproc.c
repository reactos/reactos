/*++

Copyright (c) 1995-1996 Microsoft Corporation

Module Name:

    dlgproc

Abstract:

    The dialog procedures for the different tabs in the main dialog.

Author:

    Steve Firebaugh (stevefir)         31-Dec-1995

Revision History:

--*/


#include <windows.h>
#include <commdlg.h>
#include <winsock2.h>
#include <nspapi.h>
#include "globals.h"


//
// Keep a dirty bit for the ordering of the service providers.  Set it if
//  the order changes, clear it if we push apply.
//

int gDirty = FALSE;



LRESULT CALLBACK SortDlgProc(HWND hwnd,
                             UINT message,
                             WPARAM wParam,
                             LPPROPSHEETPAGE ppsp)
/*++

  This is the main dialog proc for the window that lists all of the service
   providers and lets the user push them up and down.

  Uses GLOBAL:  gNumRows

--*/
{
    int iSelection;

    switch (message) {

      case WM_INITDIALOG:
          CatReadRegistry (hwnd);
          SendMessage (GetDlgItem (hwnd, DID_LISTCTL), LB_SETCURSEL, 0, 0);
          return FALSE;
      break;


      case WM_NOTIFY: {
          NMHDR * pnmhdr;
          pnmhdr = (NMHDR *) ppsp;

          if (pnmhdr->code == PSN_APPLY) {
              if (gDirty)
                  if ( IDYES == MessageBox (hwnd,
                                        TEXT("This operation may change the behavior of the networking components on your system.\nDo you wish to continue?"),
                                        TEXT("Warning:"),
                                        MB_ICONWARNING | MB_YESNO)) {
                      CatDoWriteEntries (hwnd);
                      gDirty = FALSE;
                  }

          }



      } break;


      case WM_COMMAND:
        switch (LOWORD (wParam)) {

          //
          // On the up & down buttons, screen out the no-ops (up on top row,
          //  or down on bottom), reorder the catalog entries, and set the
          //  dirty bit.
          //

          case DID_UP: {
              iSelection = (int)SendMessage (HWNDLISTCTL, LB_GETCURSEL, 0, 0);
              if (iSelection ==  0) return FALSE;

              CatDoUpDown (hwnd, LOWORD (wParam));
              SendMessage (GetParent(hwnd), PSM_CHANGED, (WPARAM)hwnd, 0);
              gDirty = TRUE;
          } break;


          case DID_DOWN: {
              iSelection = (int)SendMessage (HWNDLISTCTL, LB_GETCURSEL, 0, 0);
              if (iSelection ==  (gNumRows-1)) return FALSE;

              CatDoUpDown (hwnd, LOWORD (wParam));
              SendMessage (GetParent(hwnd), PSM_CHANGED, (WPARAM)hwnd, 0);
              gDirty = TRUE;
          } break;

          //
          // If the listbox is double clicked, re-send the message as if it
          //  was a more-info button press.  If it is a selection change, then
          //  set the state of the buttons appropriately.
          //

          case DID_LISTCTL:
              if (HIWORD (wParam) == LBN_DBLCLK)
                  SendMessage (hwnd, WM_COMMAND, DID_MORE, 0);
              else if (HIWORD (wParam) == LBN_SELCHANGE) {

              // here we can enable/disable buttons...
              //  not implemented yet

              }

          break;

          //
          // If they request more information, figure out which item is selected,
          //  then map that to an index value from the initial ordering.  Finally
          //  popup a dialog that will show the information from the catalog at
          //  that index.
          //

          case DID_MORE: {
              int iIndex;
              int notUsed;
              TCHAR szBuffer[MAX_STR];

              iSelection = (int)SendMessage (HWNDLISTCTL, LB_GETCURSEL, 0, 0);

              if (iSelection != LB_ERR) {

                  //
                  // Dig the chosen string out of the listbox, find the original
                  //  index hidden in it, and popup the more information dialog
                  //  for the appropriate entry.
                  //

                  SendMessage (HWNDLISTCTL, LB_GETTEXT, iSelection, (LPARAM) szBuffer);

                  ASSERT (CatGetIndex (szBuffer, &iIndex, &notUsed),
                          TEXT("SortDlgProc, CatGetIndex failed."));

                  DialogBoxParam (ghInst,
                                  TEXT("MoreInfoDlg"),
                                  hwnd,
                                  MoreInfoDlgProc,
                                  iIndex);

              }
          } break;
        }
      break; // WM_COMMAND
    } // end switch
    return FALSE;
}


INT_PTR CALLBACK MoreInfoDlgProc(HWND hwnd,
                              UINT message,
                              WPARAM wParam,
                              LPARAM lParam)
/*++

  This is the window proc for the simple "more info" dialog.  All that we
   do here is fill our listbox with interesting info on wm_initdialog, and
   then wait to be dismissed.

--*/
{

  switch (message) {

    case WM_INITDIALOG:
       CatDoMoreInfo (hwnd, (int) lParam);
    break;


    case WM_COMMAND:
      if (wParam == IDCANCEL)
        EndDialog (hwnd, FALSE);

      if (wParam == IDOK)
        EndDialog (hwnd, TRUE);
    break;

    case WM_SYSCOMMAND:
      if (wParam == SC_CLOSE)
        EndDialog (hwnd, TRUE);
    break;


  } // end switch
  return FALSE;
}



INT_PTR CALLBACK RNRDlgProc(HWND hwnd,
                            UINT message,
                            WPARAM wParam,
                            LPARAM lParam)
/*++

  For this version, simply list the installed providers.

--*/
{

  switch (message) {

    case WM_INITDIALOG: {

    //
    // at init time, query all of the installed name space providers
    //  and put their identifier in the listbox.  Notice. that this
    //  function assumes WSAStartup has already been called.
    //

#define MAX_NAMESPACE 100  // hack, arbitrary value, should be dynamic
      WSANAMESPACE_INFO  arnspBuf[MAX_NAMESPACE];
      DWORD dwBufLen;
      int   i, r;
      int   iTab = 50;

      SendMessage (HWNDLISTCTL, LB_SETTABSTOPS, 1, (LPARAM) &iTab);

      //
      // Call the WinSock2 name space enumeration function with enough
      //  free space such that we expect to get all of the information.
      //

      dwBufLen = sizeof (arnspBuf);
      r = WSAEnumNameSpaceProviders(&dwBufLen, arnspBuf);
      if ( r == SOCKET_ERROR) {
        DBGOUT((TEXT("WSAEnumNameSpaceProviders failed w/ %d\n"), WSAGetLastError()));
        return (INT_PTR)-1;
      }


      //
      // WSAEnumNameSpaceProviders succeeded so write results to listbox
      //

      for (i = 0; i< r; i++) {
        ADDSTRING(arnspBuf[i].lpszIdentifier);
      }

    } break;

  } // end switch
  return FALSE;
}
