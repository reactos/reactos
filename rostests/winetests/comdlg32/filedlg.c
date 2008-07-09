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
        } else if (nmh->code == CDN_FOLDERCHANGE )
        {
            char buf[1024];
            int ret;

            memset(buf, 0x66, sizeof(buf));
            ret = SendMessage( GetParent(hDlg), CDM_GETFOLDERIDLIST, 5, (LPARAM)buf);
            ok(ret > 0, "CMD_GETFOLDERIDLIST not implemented\n");
            if (ret > 5)
                ok(buf[0] == 0x66 && buf[1] == 0x66, "CMD_GETFOLDERIDLIST: The buffer was touched on failure\n");
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
    char szInitialDir[MAX_PATH];

    GetWindowsDirectory(szInitialDir, MAX_PATH);

    ZeroMemory(&ofn, sizeof(ofn));

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFilter = "Text Files (*.txt)\0*.txt\0All Files (*.*)\0*.*\0";
    ofn.lpstrFile = szFileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_ENABLEHOOK;
    ofn.lpstrDefExt = "txt";
    ofn.lpfnHook = (LPOFNHOOKPROC) OFNHookProc;
    ofn.lpstrInitialDir = szInitialDir;

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

    result = GetSaveFileNameA(&ofn);
    ok(0 == result, "expected %d, got %d\n", 0, result);
    ok(0 == CommDlgExtendedError(), "expected %d, got %d\n", 0,
       CommDlgExtendedError());

    PrintDlgA(NULL);
    ok(CDERR_INITIALIZATION == CommDlgExtendedError(), "expected %d, got %d\n",
              CDERR_INITIALIZATION, CommDlgExtendedError());

    /* Before passing the ofn to Unicode functions, remove the ANSI strings */
    ofn.lpstrFilter = NULL;
    ofn.lpstrInitialDir = NULL;
    ofn.lpstrDefExt = NULL;

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
        ok(0 == CommDlgExtendedError() ||
           CDERR_INITIALIZATION == CommDlgExtendedError(), /* win9x */
           "expected %d or %d, got %d\n", 0, CDERR_INITIALIZATION,
           CommDlgExtendedError());
    }

    SetLastError(0xdeadbeef);
    result = GetSaveFileNameW((LPOPENFILENAMEW) &ofn);
    if (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
        skip("GetSaveFileNameW is not implemented\n");
    else
    {
        ok(0 == result, "expected %d, got %d\n", 0, result);
        ok(0 == CommDlgExtendedError() ||
           CDERR_INITIALIZATION == CommDlgExtendedError(), /* win9x */
           "expected %d or %d, got %d\n", 0, CDERR_INITIALIZATION,
           CommDlgExtendedError());
    }
}


START_TEST(filedlg)
{
    test_DialogCancel();

}
