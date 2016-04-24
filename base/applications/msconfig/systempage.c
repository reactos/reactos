/*
 * PROJECT:     ReactOS Applications
 * LICENSE:     LGPL - See COPYING in the top level directory
 * FILE:        base/applications/msconfig/systempage.c
 * PURPOSE:     System page message handler
 * COPYRIGHT:   Copyright 2005-2006 Christoph von Wittich <Christoph@ApiViewer.de>
 *                        2011      Gregor Schneider <Gregor.Schneider@reactos.org>
 */

#include "precomp.h"

HWND hSystemPage;
HWND hSystemDialog;

#define BUFFER_SIZE 512

static BOOL
LoadSystemIni(WCHAR * szPath, HWND hDlg)
{
    WCHAR szBuffer[BUFFER_SIZE];
    HWND hDlgCtrl;
    HTREEITEM parent = NULL;
    FILE* file;
    UINT length;
    TVINSERTSTRUCT insert;
    HRESULT hr;

    hr = StringCbCopyW(szBuffer, sizeof(szBuffer), szPath);
    if (FAILED(hr))
        return FALSE;

    hr = StringCbCatW(szBuffer, sizeof(szBuffer), L"\\system.ini");
    if (FAILED(hr))
        return FALSE;

    file = _wfopen(szBuffer, L"rt");
    if (!file)
       return FALSE;

    hDlgCtrl = GetDlgItem(hDlg, IDC_SYSTEM_TREE);

    while(!feof(file))
    {
        if (fgetws(szBuffer, BUFFER_SIZE, file))
        {
            length = wcslen(szBuffer);
            if (length > 1)
            {
                szBuffer[length] = L'\0';
                szBuffer[length - 1] = L'\0';
                insert.hInsertAfter = TVI_LAST;
                insert.item.mask = TVIF_TEXT;
                insert.item.pszText = szBuffer;

                if (szBuffer[0] == L';' || szBuffer[0] == L'[')
                {
                    /* Parent */
                    insert.hParent = NULL;
                    parent = TreeView_InsertItem(hDlgCtrl, &insert);
                }
                else
                {
                    /* Child */
                    insert.hParent = parent;
                    TreeView_InsertItem(hDlgCtrl, &insert);
                }
            }
        }
    }

    fclose(file);

    return TRUE;
}

static BOOL
InitializeSystemDialog(HWND hDlg)
{
    WCHAR winDir[PATH_MAX];

    GetWindowsDirectoryW(winDir, PATH_MAX);
    return LoadSystemIni(winDir, hDlg);
}


INT_PTR CALLBACK
SystemPageWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(wParam);
    switch (message) {
        case WM_INITDIALOG:
        {
            hSystemDialog = hDlg;
            SetWindowPos(hDlg, NULL, 10, 32, 0, 0, SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOSIZE | SWP_NOZORDER);
            InitializeSystemDialog(hDlg);
            return TRUE;
        }
    }

    return 0;
}
