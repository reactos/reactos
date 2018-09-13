/****************************Module*Header******************************\
* Copyright (c) 1987 - 1991  Microsoft Corporation                      *
\***********************************************************************/
/********************************************************
*                                                       *
*       file:   AbortDlg.c                              *
*       system: PC Paintbrush for MS-Windows            *
*       descr:  abort print dialog proc                 *
*       date:   03/17/87 @ 10:15                        *
*                                                       *
********************************************************/

#include "onlypbr.h"
#undef NOWINMESSAGES
#undef NOSYSCOMMANDS
#undef NOMENUS

#include <windows.h>
#include "port1632.h"
#include "pbrush.h"

BOOL FAR PASCAL AbortDlg(HWND hDlg, UINT message, WPARAM wParam, LONG lParam)
{
   switch(message) {
   case WM_COMMAND:
      EnableWindow(pbrushWnd[PARENTid], TRUE);
      DestroyWindow(hDlg);
      bUserAbort = TRUE;
      hDlgPrint = NULL;
      break;

   case WM_INITDIALOG:
      CenterWindow(hDlg);
      SetWindowText(hDlg, pgmTitle);   
      EnableMenuItem(GetSystemMenu(hDlg, FALSE), SC_CLOSE, MF_GRAYED);
      break;

   default:
      return(FALSE);
   }

   return(TRUE);
    wParam;
    lParam;
}
