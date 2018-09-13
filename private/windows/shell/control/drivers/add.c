/*  ADD.C
**
**  Copyright (C) Microsoft, 1990, All Rights Reserved.
**
**  Multimedia Control Panel Applet for removing
**  device drivers.  See the ispec doc DRIVERS.DOC for more information.
**
**  This file deals with the case where an OEM driver being installed.
**
**  History:
**
**      Thu Nov 1 1991 -by- Sanjaya
**      Created. Originally part of drivers.c
*/

#include <windows.h>
#include <mmsystem.h>
#include <memory.h>
#include <string.h>
#include <cpl.h>

#include "drivers.h"
#include <cphelp.h>
#include "sulib.h"
extern PINF       pinfOldDefault;
extern BOOL       bBadOemSetup;
char *szFilter[] = {"Inf Files(*.inf)", "*.inf", "Drv Files(*.drv)", "*.drv", ""};

BOOL GetDir          (HWND);
void BrowseDlg           (HWND, int);

/*  AddDriversDlg
 *
 * Returns 2 if dialog needs to be redrawn
 * Returns 1 if the oem file has been succesfully located
 * Returns 0 if Cancel has been pressed
 */

int AddDriversDlg(HWND hDlg, UINT message, UINT wParam, LONG lParam)
{
    switch (message)
    {
        case WM_INITDIALOG:

            SetDlgItemText(hDlg, ID_TEXT, (LPSTR)szUnlisted);
            SetDlgItemText(hDlg, ID_EDIT, (LPSTR)szDirOfSrc);
            return(TRUE);

        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                case IDOK:
                    if (GetDir(hDlg))
                    {
                        DialogBox(myInstance, MAKEINTRESOURCE(DLG_UPDATE),
                        GetParent(hDlg), AddUnlistedDlg);
                        break;
                    } else {
                        EndDialog(hDlg, 2);
                    }
                    break;

                case IDCANCEL:
                    EndDialog(hDlg, 0);
                    return(TRUE);

                case IDS_BROWSE:
                    lstrcpy(szDrv, szOemInf);
                    BrowseDlg(hDlg, 1);
                    break;

                case IDH_DLG_INSERT_DISK:
                    goto DoHelp;
            }
            break;

        default:
            if (message == wHelpMessage) {
DoHelp:
                WinHelp(hDlg, szDriversHlp, HELP_CONTEXT, IDH_DLG_INSERT_DISK);
                return TRUE;
            } else
                return FALSE;

    }
    return (FALSE);                         /* Didn't process a message    */
}


BOOL GetDir(HWND hDlg)
{
    PSTR    pstr;
    OFSTRUCT of;

    wsStartWait();

   /*
    * Test the edit box for a proper path
    * and look for the oemsetup.inf
    * file.  If we don't find it , highlight the
    * text in the edit box and bring up a dialog box
    */

    GetDlgItemText( hDlg, ID_EDIT, szDirOfSrc, MAX_PATH);
    RemoveSpaces(szFullPath, szDirOfSrc);
    lstrcpy(szDirOfSrc, szFullPath);
    for (pstr = szFullPath;*pstr;pstr++);
        if (*(pstr-1) != '\\')
            *pstr++ = '\\';

    *pstr = '\0';

    lstrcpy(szDiskPath, szFullPath);

   /*
    * Look for an oemsetup.inf
    * If you can't find it return false
    *
    */

    lstrcpy(pstr, szOemInf);

    if (OpenFile(szFullPath, &of, OF_EXIST) == -1)
    {
       SendDlgItemMessage(hDlg, ID_EDIT, EM_SETSEL, 0, -1L);
       wsEndWait();
       return (FALSE);
    }

   /*
    * Change the default inf to this new oemsetup.inf
    * Discard the value of the previous .inf, since it might
    * just be another set of oem drivers.
    */

    if (bBadOemSetup)
       infSetDefault(infOpen(of.szPathName));
    else
       pinfOldDefault = infSetDefault(infOpen(of.szPathName));
    EndDialog(hDlg, 1);
    wsEndWait();
    return(TRUE);
}


/*
 * Hooks into common dialog to show only directories
 */

BOOL CALLBACK AddFileHookProc(HWND hDlg, UINT iMessage,
                              DWORD wParam, LONG lParam)
{
  char szTemp[200];
  HWND hTemp;

    switch (iMessage)
    {
        case WM_INITDIALOG:

            GetDlgItemText(((LPOPENFILENAME)lParam)->hwndOwner, ID_TEXT,
                  szTemp, sizeof(szTemp));
            SetDlgItemText(hDlg, ctlLast+1, szTemp);

            goto PostMyMessage;

        case WM_COMMAND:

            switch (LOWORD(wParam))
            {
                case lst2:
                case cmb2:
                case IDOK:

  PostMyMessage:
                  PostMessage(hDlg, WM_COMMAND, ctlLast+2, 0L);
                  break;

                case IDH_DLG_BROWSE:
                  goto DoHelp;

                case ctlLast+2:
                   if (bFindOEM)
                   {
                     if (SendMessage(hTemp=GetDlgItem(hDlg, lst1), LB_GETCOUNT,
                       0, 0L))
                       {
                         SendMessage(hTemp, LB_SETCURSEL, 0, 0L);
                         SendMessage(hDlg, WM_COMMAND, MAKELONG(lst1, LBN_SELCHANGE),
                                     (LONG)hTemp);
                         break;
                       }
                   }
                   SetDlgItemText(hDlg, edt1, szDrv);
                   break;
            }
            break;

        default:

            if (iMessage == wHelpMessage)
            {
DoHelp:
                WinHelp(hDlg, szDriversHlp, HELP_CONTEXT, IDH_DLG_BROWSE);
                return(TRUE);
            }
    }

    return FALSE;  // commdlg, do your thing
}


/*
 * Function : BrowseDlg
 *
 *     Call the GetOpenFileName dialog to open a file
 *
 * Parameters :
 *
 *     hDlg : Parent Dialog box
 *
 *     iIndex : Index into szFilter to determine which filter(s) to use
 */


void BrowseDlg(HWND hDlg, int iIndex)
{
    OPENFILENAME OpenFileName;
    char szPath[MAX_PATH];
    char szFile[MAX_PATH];

    *szPath = '\0';
    *szFile = '\0';
    OpenFileName.lStructSize = sizeof(OPENFILENAME);
    OpenFileName.hwndOwner = hDlg;
    OpenFileName.hInstance = myInstance;
    OpenFileName.lpstrFilter = szFilter[0];
    OpenFileName.lpstrCustomFilter = NULL;
    OpenFileName.nMaxCustFilter = 0;
    OpenFileName.nFilterIndex = iIndex;
    OpenFileName.lpstrFile = (LPSTR)szPath;
    OpenFileName.nMaxFile = sizeof(szPath);
    OpenFileName.lpstrFileTitle = szFile;
    OpenFileName.nMaxFileTitle = sizeof(szFile);
    OpenFileName.lpstrInitialDir = NULL;
    OpenFileName.lpstrTitle = NULL;
    OpenFileName.Flags = OFN_HIDEREADONLY | OFN_ENABLEHOOK |
      /*  OFN_FILEMUSTEXIST | */ OFN_ENABLETEMPLATE | OFN_NOCHANGEDIR |
            OFN_SHOWHELP;
    OpenFileName.lCustData = (LONG)hDlg;
    OpenFileName.lpfnHook = (LPOFNHOOKPROC)AddFileHookProc;

    OpenFileName.lpTemplateName = (LPSTR)MAKEINTRESOURCE(DLG_BROWSE);
    OpenFileName.nFileOffset = 0;
    OpenFileName.nFileExtension = 0;
    OpenFileName.lpstrDefExt = NULL;
    if (GetOpenFileName(&OpenFileName))
    {
        UpdateWindow(hDlg); // force buttons to repaint
        szPath[OpenFileName.nFileOffset] = '\0';
        SetDlgItemText(hDlg, ID_EDIT, szPath);
    }
}
