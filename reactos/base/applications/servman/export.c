/*
 * PROJECT:     ReactOS Services
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/system/servman/export.c
 * PURPOSE:     Save services to a file
 * COPYRIGHT:   Copyright 2006 Ged Murphy <gedmurphy@gmail.com>
 *
 */

#include "precomp.h"

static DWORD
GetTextFromListView(PMAIN_WND_INFO Info,
                    TCHAR Text[500],
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
    NumChars = (INT)SendMessage(Info->hListView,
                                LVM_GETITEMTEXT,
                                row,
                                (LPARAM)&item);

    return NumChars;
}


static BOOL
SaveServicesToFile(PMAIN_WND_INFO Info,
                   LPCTSTR pszFileName)
{
	HANDLE hFile;
	BOOL bSuccess = FALSE;

	hFile = CreateFile(pszFileName,
                       GENERIC_WRITE,
                       0,
                       NULL,
                       CREATE_ALWAYS,
                       FILE_ATTRIBUTE_NORMAL,
                       NULL);

	if(hFile != INVALID_HANDLE_VALUE)
	{
        TCHAR LVText[500];
        TCHAR newl = _T('\n');
        TCHAR tab = _T('\t');
		DWORD dwTextLength, dwWritten;
		INT NumListedServ = 0;
		INT i, k;

		NumListedServ = ListView_GetItemCount(Info->hListView);

		for (i=0; i < NumListedServ; i++)
		{
		    for (k=0; k<5; k++)
		    {
                dwTextLength = GetTextFromListView(Info,
                                                   LVText,
                                                   i,
                                                   k);
                if (LVText != NULL)
                {
                    WriteFile(hFile,
                              LVText,
                              sizeof(TCHAR) * dwTextLength,
                              &dwWritten,
                              NULL);

                    WriteFile(hFile,
                              &tab,
                              sizeof(TCHAR),
                              &dwWritten,
                              NULL);
                }
		    }
		    WriteFile(hFile,
                      &newl,
                      sizeof(TCHAR),
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
	OPENFILENAME ofn;
	TCHAR szFileName[MAX_PATH] = _T("");

	ZeroMemory(&ofn, sizeof(ofn));

	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = Info->hMainWnd;
	ofn.lpstrFilter = _T("Text (Tab Delimited)(*.txt)\0*.txt\0Text (Comma Delimited)(*.csv)\0*.csv\0");
	ofn.lpstrFile = szFileName;
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrDefExt = _T("txt");
	ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;

	if(GetSaveFileName(&ofn))
	{
		if (SaveServicesToFile(Info, szFileName))
            return;
	}

	if (CommDlgExtendedError() != CDERR_GENERALCODES)
        MessageBox(NULL, _T("Export to file failed"), NULL, 0);
}



