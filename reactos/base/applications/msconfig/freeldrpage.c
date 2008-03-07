/*
 * PROJECT:     ReactOS Applications
 * LICENSE:     LGPL - See COPYING in the top level directory
 * FILE:        base/applications/freeldrpage.c
 * PURPOSE:     Freeloader configuration page message handler
 * COPYRIGHT:   Copyright 2005-2006 Christoph von Wittich <Christoph@ApiViewer.de>
 *
 */

#include <precomp.h>

HWND hFreeLdrPage;
HWND hFreeLdrDialog;

typedef struct
{
    ULONG TimeOut;
    WCHAR szDefaultOS[512];
    ULONG szDefaultPos;
    ULONG OSConfigurationCount;
    BOOL  UseBootIni;
}FREELDR_SETTINGS;

static FREELDR_SETTINGS Settings = { 0, {0}, 0 };


BOOL
LoadBootIni(WCHAR * szDrive, HWND hDlg)
{
   WCHAR szBuffer[512];
   HWND hDlgCtrl;
   FILE * file;
   UINT length;
   LRESULT pos;

   wcscpy(szBuffer, szDrive);
   wcscat(szBuffer, L"boot.ini");
   
   file = _wfopen(szBuffer, L"rt");
   if (!file)
       return FALSE;
            
   hDlgCtrl = GetDlgItem(hDlg, IDC_LIST_BOX);

   while(!feof(file))
   {
        if (fgetws(szBuffer, sizeof(szBuffer) / sizeof(WCHAR), file))
        {
            length = wcslen(szBuffer);
            while(szBuffer[length] < 14) //FIXME remove line feeds
                szBuffer[length--] = 0;

            pos = SendMessageW(hDlgCtrl, LB_ADDSTRING, 0, (LPARAM)szBuffer);


            if (szBuffer[0] == L'[')
                continue;

            if (!wcsncmp(szBuffer, L"timeout=", 8))
            {
                Settings.TimeOut = _wtoi(&szBuffer[8]);
                continue;
            }

            if (!wcsncmp(szBuffer, L"default=", 8))
            {
                wcscpy(Settings.szDefaultOS, &szBuffer[8]);
                continue;
            }
            if (pos != LB_ERR)
                SendMessage(hDlgCtrl, LB_SETITEMDATA, pos, 1); // indicate that this item is an boot entry
            Settings.OSConfigurationCount++;
        }
   }

   fclose(file);
   Settings.UseBootIni = TRUE;

   pos = SendMessageW(hDlgCtrl, LB_FINDSTRING, 3, (LPARAM)Settings.szDefaultOS);
   if (pos != LB_ERR)
   {
       Settings.szDefaultPos = pos;
       SendMessage(hDlgCtrl, LB_SETCURSEL, pos, 0);
   }

   SetDlgItemInt(hDlg, IDC_TXT_BOOT_TIMEOUT, Settings.TimeOut, FALSE);
   if (Settings.OSConfigurationCount < 2)
   {
        EnableWindow(GetDlgItem(hDlg, IDC_BTN_SET_DEFAULT_BOOT), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_BTN_MOVE_UP_BOOT_OPTION), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_BTN_MOVE_DOWN_BOOT_OPTION), FALSE);
   }
   return TRUE;
}


BOOL
InitializeDialog(HWND hDlg)
{
   // FIXME
   // find real boot drive and handle freeldr configuration ini
    return LoadBootIni(L"C:\\", hDlg);
}



INT_PTR CALLBACK
FreeLdrPageWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    LRESULT pos;

    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(wParam);

    switch (message) {
    case WM_INITDIALOG:
        hFreeLdrDialog = hDlg;
        SetWindowPos(hDlg, NULL, 10, 32, 0, 0, SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOSIZE | SWP_NOZORDER);
        InitializeDialog(hDlg);
        return TRUE;
    case WM_COMMAND:
        switch(HIWORD(wParam))
        {
            case LBN_SELCHANGE:
                pos = SendMessage((HWND)lParam, LB_GETCURSEL, 0, 0);
                if (pos != LB_ERR)
                {
                    LPARAM res = SendMessage((HWND)lParam, LB_GETITEMDATA, pos, 0);
                    if (!res) //line is not a default one
                        SendMessage((HWND)lParam, LB_SETCURSEL, Settings.szDefaultPos, 0);
                    else
                        Settings.szDefaultPos = pos;


                }
            break;
        }
    }
    return 0;
}
