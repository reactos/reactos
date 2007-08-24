/*
 * Unit test suite for comdlg32 API functions: file dialogs
 *
 * Copyright 2007 Google (Lei Zhang)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 *
 */

#include <windows.h>
#include <wine/test.h>


/* ##### */

static UINT CALLBACK OFNHookProc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    LPNMHDR nmh;

    if( msg == WM_NOTIFY)
    {
        nmh = (LPNMHDR) lParam;
        if( nmh->code == CDN_INITDONE)
        {
            PostMessage( GetParent(hDlg), WM_COMMAND, IDCANCEL, FALSE);
        }
    }

    return 0;
}

/* bug 6829 */
static void test_DialogCancel(void)
{
    OPENFILENAMEA ofn;
    BOOL result;
    char szFileName[MAX_PATH] = "";

    ZeroMemory(&ofn, sizeof(ofn));

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFilter = "Text Files (*.txt)\0*.txt\0All Files (*.*)\0*.*\0";
    ofn.lpstrFile = szFileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_ENABLEHOOK;
    ofn.lpstrDefExt = "txt";
    ofn.lpfnHook = (LPOFNHOOKPROC) OFNHookProc;

    PrintDlgA(NULL);
    ok(CDERR_INITIALIZATION == CommDlgExtendedError(), "expected %d, got %d\n",
       CDERR_INITIALIZATION, CommDlgExtendedError());

    result = GetOpenFileNameA(&ofn);
    ok(0 == result, "expected %d, got %d\n", 0, result);
    ok(0 == CommDlgExtendedError(), "expected %d, got %d\n", 0,
       CommDlgExtendedError());

    PrintDlgA(NULL);
    ok(CDERR_INITIALIZATION == CommDlgExtendedError(), "expected %d, got %d\n",
              CDERR_INITIALIZATION, CommDlgExtendedError());

    SetLastError(0xdeadbeef);
    result = GetOpenFileNameW((LPOPENFILENAMEW) &ofn);
    if (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
        skip("GetOpenFileNameW is not implemented\n");
    else
    {
        ok(0 == result, "expected %d, got %d\n", 0, result);
        ok(0 == CommDlgExtendedError(), "expected %d, got %d\n", 0,
           CommDlgExtendedError());
    }

    PrintDlgA(NULL);
    ok(CDERR_INITIALIZATION == CommDlgExtendedError(), "expected %d, got %d\n",
              CDERR_INITIALIZATION, CommDlgExtendedError());

    result = GetSaveFileNameA(&ofn);
    ok(0 == result, "expected %d, got %d\n", 0, result);
    ok(0 == CommDlgExtendedError(), "expected %d, got %d\n", 0,
       CommDlgExtendedError());

    PrintDlgA(NULL);
    ok(CDERR_INITIALIZATION == CommDlgExtendedError(), "expected %d, got %d\n",
              CDERR_INITIALIZATION, CommDlgExtendedError());

    SetLastError(0xdeadbeef);
    result = GetSaveFileNameW((LPOPENFILENAMEW) &ofn);
    if (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
        skip("GetSaveFileNameW is not implemented\n");
    else
    {
        ok(0 == result, "expected %d, got %d\n", 0, result);
        ok(0 == CommDlgExtendedError(), "expected %d, got %d\n", 0,
           CommDlgExtendedError());
    }
}


START_TEST(filedlg)
{
    test_DialogCancel();

}
