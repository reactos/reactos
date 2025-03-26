/*
 * PROJECT:     ReactOS Services
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/applications/mscutils/servman/export.c
 * PURPOSE:     Save services to a file
 * COPYRIGHT:   Copyright 2006 Ged Murphy <gedmurphy@gmail.com>
 *
 */

#include "precomp.h"

#include <cderr.h>

static DWORD
GetTextFromListView(PMAIN_WND_INFO Info,
                    LPWSTR Text,
                    INT row,
                    INT col)
{
    LVITEM item;
    DWORD NumChars;

    ZeroMemory(&item, sizeof(item));
    item.mask = LVIF_TEXT;
    item.iSubItem = col;
    item.pszText = Text;
    item.cchTextMax = 500;
    NumChars = (INT)SendMessageW(Info->hListView,
                                 LVM_GETITEMTEXTW,
                                 row,
                                 (LPARAM)&item);
    return NumChars;
}

static BOOL
SaveServicesToFile(PMAIN_WND_INFO Info,
                   LPCWSTR pszFileName,
                   DWORD nFormat)
{
    HANDLE hFile;
    BOOL bSuccess = FALSE;

    if (!nFormat || nFormat > 2)
    {
        return bSuccess;
    }

    hFile = CreateFileW(pszFileName,
                       GENERIC_WRITE,
                       0,
                       NULL,
                       CREATE_ALWAYS,
                       FILE_ATTRIBUTE_NORMAL,
                       NULL);

    if(hFile != INVALID_HANDLE_VALUE)
    {
        WCHAR LVText[500];
        WCHAR newl[2] = {L'\r', L'\n'};
        WCHAR seps[2] = {L'\t', L','};
        DWORD dwTextLength, dwWritten;
        INT NumListedServ = 0;
        INT i, k;

        NumListedServ = ListView_GetItemCount(Info->hListView);

        for (i=0; i < NumListedServ; i++)
        {
            for (k=0; k < LVMAX; k++)
            {
                dwTextLength = GetTextFromListView(Info,
                                                   LVText,
                                                   i,
                                                   k);
                if (dwTextLength != 0)
                {
                    WriteFile(hFile,
                              LVText,
                              sizeof(WCHAR) * dwTextLength,
                              &dwWritten,
                              NULL);
                }

                if (k < LVMAX - 1)
                {
                    /* Do not add separator after the last table cell */
                    WriteFile(hFile,
                              &seps[nFormat-1],
                              sizeof(WCHAR),
                              &dwWritten,
                              NULL);
                }
            }
            WriteFile(hFile,
                      newl,
                      sizeof(newl),
                      &dwWritten,
                      NULL);
        }

        CloseHandle(hFile);
        bSuccess = TRUE;
    }

    return bSuccess;
}

VOID ExportFile(PMAIN_WND_INFO Info)
{
    OPENFILENAMEW ofn;
    WCHAR szFileName[MAX_PATH];

    ZeroMemory(&ofn, sizeof(ofn));
    szFileName[0] = UNICODE_NULL;

    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hwndOwner = Info->hMainWnd;
    ofn.lpstrFilter = L"Text (Tab Delimited)(*.txt)\0*.txt\0Text (Comma Delimited)(*.csv)\0*.csv\0";
    ofn.lpstrFile = szFileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrDefExt = L"txt";
    ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;

    if(GetSaveFileName(&ofn))
    {
        if (SaveServicesToFile(Info, szFileName, ofn.nFilterIndex))
            return;
    }

    if (CommDlgExtendedError() != CDERR_GENERALCODES)
        MessageBoxW(NULL, L"Export to file failed", NULL, 0);
}
