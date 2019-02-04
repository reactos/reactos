/*
 * Unit test of the SHBrowseForFolder function.
 *
 * Copyright 2009-2010 Michael Mc Donnell
 * Copyright 2011 André Hentschel
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
 */

#define COBJMACROS

#include <windows.h>
#include <shlobj.h>
#include <shobjidl.h>
#include <string.h>
#include "shellapi.h"

#include "wine/test.h"
#define IDD_MAKENEWFOLDER 0x3746 /* From "../shresdef.h" */
#define TIMER_WAIT_MS 50 /* Should be long enough for slow systems */

static const char new_folder_name[] = "foo";
static LPITEMIDLIST selected_folder_pidl;

/*
 * Returns the number of folders in a folder.
 */
static int get_number_of_folders(LPCSTR path)
{
    int number_of_folders = 0;
    char path_search_string[MAX_PATH];
    WIN32_FIND_DATAA find_data;
    HANDLE find_handle;

    lstrcpynA(path_search_string, path, MAX_PATH - 1);
    strcat(path_search_string, "*");

    find_handle = FindFirstFileA(path_search_string, &find_data);
    if (find_handle == INVALID_HANDLE_VALUE)
        return -1;

    do
    {
        if ((find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
            strcmp(find_data.cFileName, ".") != 0 &&
            strcmp(find_data.cFileName, "..") != 0)
        {
            number_of_folders++;
        }
    }
    while (FindNextFileA(find_handle, &find_data) != 0);

    FindClose(find_handle);
    return number_of_folders;
}

static BOOL does_folder_or_file_exist(LPCSTR folder_path)
{
    DWORD file_attributes = GetFileAttributesA(folder_path);
    return !(file_attributes == INVALID_FILE_ATTRIBUTES);
}

/*
 * Timer callback used by test_click_make_new_folder_button. It simulates a user
 * making a new folder and calling it "foo".
 */
static void CALLBACK make_new_folder_timer_callback(HWND hwnd, UINT uMsg,
                                                    UINT_PTR idEvent, DWORD dwTime)
{
    static int step = 0;

    switch (step++)
    {
    case 0:
        /* Click "Make New Folder" button */
        PostMessageA(hwnd, WM_COMMAND, IDD_MAKENEWFOLDER, 0);
        break;
    case 1:
        /* Set the new folder name to foo by replacing text in edit control */
        SendMessageA(GetFocus(), EM_REPLACESEL, 0, (LPARAM) new_folder_name);
        SetFocus(hwnd);
        break;
    case 2:
        /*
         * The test does not trigger the correct state on Windows. This results
         * in the new folder pidl not being returned. The result is as
         * expected if the same steps are done manually.
         * Sending the down key selects the new folder again which sets the
         * correct state. This ensures that the correct pidl is returned.
         */
        keybd_event(VK_DOWN, 0, 0, 0);
        break;
    case 3:
        keybd_event(VK_DOWN, 0, KEYEVENTF_KEYUP, 0);
        break;
    case 4:
        KillTimer(hwnd, idEvent);
        /* Close dialog box */
        SendMessageA(hwnd, WM_COMMAND, IDOK, 0);
        break;
    default:
        break;
    }
}

/*
 * Callback used by test_click_make_new_folder_button. It sets up a timer to
 * simulate user input.
 */
static int CALLBACK create_new_folder_callback(HWND hwnd, UINT uMsg,
                                               LPARAM lParam, LPARAM lpData)
{
    switch (uMsg)
    {
    case BFFM_INITIALIZED:
        /* User input is simulated in timer callback */
        SetTimer(hwnd, 0, TIMER_WAIT_MS, make_new_folder_timer_callback);
        return TRUE;
    default:
        return FALSE;
    }
}

/*
 * Tests if clicking the "Make New Folder" button in a SHBrowseForFolder
 * dialog box creates a new folder. (Bug 17986).
 *
 * Here follows a description of what happens on W2K,Vista, W2K8, W7:
 * When the "Make New Folder" button is clicked a new folder is created and
 * inserted into the tree. The folder is given a default name that depends on
 * the locale (e.g. "New Folder"). The folder name is selected and the dialog
 * waits for the user to type in a new name. The folder is renamed when the user
 * types in a name and presses enter.
 *
 * Note that XP and W2K3 do not select the folder name or wait for the user
 * to type in a new folder name. This behavior is considered broken as most
 * users would like to give the folder a name after creating it. The fact that
 * it originally waited for the user to type in a new folder name(W2K), and then
 * again was changed back wait for the new folder name(Vista, W2K8, W7),
 * indicates that MS also believes that it was broken in XP and W2K3.
 */
static void test_click_make_new_folder_button(void)
{
    HRESULT resCoInit, hr;
    BROWSEINFOA bi;
    LPITEMIDLIST pidl = NULL;
    LPITEMIDLIST test_folder_pidl;
    IShellFolder *test_folder_object;
    char test_folder_path[MAX_PATH];
    WCHAR test_folder_pathW[MAX_PATH];
    CHAR new_folder_path[MAX_PATH];
    CHAR new_folder_pidl_path[MAX_PATH];
    char selected_folder[MAX_PATH];
    const CHAR title[] = "test_click_make_new_folder_button";
    int number_of_folders = -1;
    SHFILEOPSTRUCTA shfileop;

    if (does_folder_or_file_exist(title))
    {
        skip("The test folder already exists.\n");
        return;
    }

    /* Must initialize COM if using the NEWDIAlOGSTYLE according to MSDN. */
    resCoInit = CoInitialize(NULL);
    if(!(resCoInit == S_OK || resCoInit == S_FALSE))
    {
        skip("COM could not be initialized %u\n", GetLastError());
        return;
    }

    /* Leave room for concatenating title, two backslashes, and an extra NULL. */
    if (!GetCurrentDirectoryA(MAX_PATH-strlen(title)-3, test_folder_path))
    {
        skip("GetCurrentDirectoryA failed %u\n", GetLastError());
    }
    strcat(test_folder_path, "\\");
    strcat(test_folder_path, title);
    strcat(test_folder_path, "\\");

    /* Avoid conflicts by creating a test folder. */
    if (!CreateDirectoryA(title, NULL))
    {
        skip("CreateDirectoryA failed %u\n", GetLastError());
        return;
    }

    /* Initialize browse info struct for SHBrowseForFolder */
    bi.hwndOwner = NULL;
    bi.pszDisplayName = selected_folder;
    bi.lpszTitle = title;
    bi.ulFlags = BIF_NEWDIALOGSTYLE;
    bi.lpfn = create_new_folder_callback;
    /* Use test folder as the root folder for dialog box */
    MultiByteToWideChar(CP_UTF8, 0, test_folder_path, -1,
        test_folder_pathW, MAX_PATH);
    hr = SHGetDesktopFolder(&test_folder_object);
    ok (SUCCEEDED(hr), "SHGetDesktopFolder failed with hr 0x%08x\n", hr);
    if (FAILED(hr)) {
        skip("SHGetDesktopFolder failed - skipping\n");
        return;
    }
    test_folder_object->lpVtbl->ParseDisplayName(test_folder_object, NULL, NULL,
        test_folder_pathW, 0UL, &test_folder_pidl, 0UL);
    bi.pidlRoot = test_folder_pidl;

    /* Display dialog box and let callback click the buttons */
    pidl = SHBrowseForFolderA(&bi);

    number_of_folders = get_number_of_folders(test_folder_path);
    ok(number_of_folders == 1 || broken(number_of_folders == 0) /* W95, W98 */,
        "Clicking \"Make New Folder\" button did not result in a new folder.\n");

    /* There should be a new folder foo inside the test folder */
    strcpy(new_folder_path, test_folder_path);
    strcat(new_folder_path, new_folder_name);
    ok(does_folder_or_file_exist(new_folder_path)
        || broken(!does_folder_or_file_exist(new_folder_path)) /* W95, W98, XP, W2K3 */,
        "The new folder did not get the name %s\n", new_folder_name);

    /* Dialog should return a pidl pointing to the new folder */
    ok(SHGetPathFromIDListA(pidl, new_folder_pidl_path),
        "SHGetPathFromIDList failed for new folder.\n");
    ok(strcmp(new_folder_path, new_folder_pidl_path) == 0
        || broken(strcmp(new_folder_path, new_folder_pidl_path) != 0) /* earlier than Vista */,
        "SHBrowseForFolder did not return the pidl for the new folder. "
        "Expected '%s' got '%s'\n", new_folder_path, new_folder_pidl_path);

    /* Remove test folder and any subfolders created in this test */
    shfileop.hwnd = NULL;
    shfileop.wFunc = FO_DELETE;
    /* Path must be double NULL terminated */
    test_folder_path[strlen(test_folder_path)+1] = '\0';
    shfileop.pFrom = test_folder_path;
    shfileop.pTo = NULL;
    shfileop.fFlags = FOF_NOCONFIRMATION|FOF_NOERRORUI|FOF_SILENT;
    SHFileOperationA(&shfileop);

    CoTaskMemFree(pidl);
    CoTaskMemFree(test_folder_pidl);
    test_folder_object->lpVtbl->Release(test_folder_object);

    CoUninitialize();
}


/*
 * Callback used by test_selection.
 */
static int CALLBACK selection_callback(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
    DWORD ret;

    switch (uMsg)
    {
    case BFFM_INITIALIZED:
        /* test with zero values */
        ret = SendMessageA(hwnd, BFFM_SETSELECTIONA, 0, 0);
        ok(!ret, "SendMessage returned: %u\n", ret);
        ret = SendMessageA(hwnd, BFFM_SETSELECTIONW, 0, 0);
        ok(!ret, "SendMessage returned: %u\n", ret);

        ret = SendMessageA(hwnd, BFFM_SETSELECTIONA, 1, 0);
        ok(!ret, "SendMessage returned: %u\n", ret);

        if(0)
        {
            /* Crashes on NT4 */
            ret = SendMessageA(hwnd, BFFM_SETSELECTIONW, 1, 0);
            ok(!ret, "SendMessage returned: %u\n", ret);
        }

        ret = SendMessageA(hwnd, BFFM_SETSELECTIONA, 0, (LPARAM)selected_folder_pidl);
        ok(!ret, "SendMessage returned: %u\n", ret);
        ret = SendMessageW(hwnd, BFFM_SETSELECTIONW, 0, (LPARAM)selected_folder_pidl);
        ok(!ret, "SendMessage returned: %u\n", ret);

        ret = SendMessageA(hwnd, BFFM_SETSELECTIONA, 1, (LPARAM)selected_folder_pidl);
        ok(!ret, "SendMessage returned: %u\n", ret);
        ret = SendMessageW(hwnd, BFFM_SETSELECTIONW, 1, (LPARAM)selected_folder_pidl);
        ok(!ret, "SendMessage returned: %u\n", ret);

        ret = SendMessageA(hwnd, BFFM_SETSELECTIONA, 1, (LPARAM)new_folder_name);
        ok(!ret, "SendMessage returned: %u\n", ret);
        ret = SendMessageW(hwnd, BFFM_SETSELECTIONW, 1, (LPARAM)new_folder_name);
        ok(!ret, "SendMessage returned: %u\n", ret);

        SendMessageA(hwnd, WM_COMMAND, IDOK, 0);
        return 1;
    default:
        return 0;
    }
}

static void test_selection(void)
{
    HRESULT resCoInit, hr;
    BROWSEINFOA bi;
    LPITEMIDLIST pidl = NULL;
    IShellFolder *desktop_object;
    WCHAR selected_folderW[MAX_PATH];
    const CHAR title[] = "test_selection";

    resCoInit = CoInitialize(NULL);
    if(!(resCoInit == S_OK || resCoInit == S_FALSE))
    {
        skip("COM could not be initialized %u\n", GetLastError());
        return;
    }

    if (!GetCurrentDirectoryW(MAX_PATH, selected_folderW))
    {
        skip("GetCurrentDirectoryW failed %u\n", GetLastError());
    }

    /* Initialize browse info struct for SHBrowseForFolder */
    bi.hwndOwner = NULL;
    bi.pszDisplayName = NULL;
    bi.lpszTitle = title;
    bi.lpfn = selection_callback;

    hr = SHGetDesktopFolder(&desktop_object);
    ok (SUCCEEDED(hr), "SHGetDesktopFolder failed with hr 0x%08x\n", hr);
    if (FAILED(hr)) {
        skip("SHGetDesktopFolder failed - skipping\n");
        return;
    }
    desktop_object->lpVtbl->ParseDisplayName(desktop_object, NULL, NULL,
        selected_folderW, 0UL, &selected_folder_pidl, 0UL);
    bi.pidlRoot = selected_folder_pidl;

    /* test without flags */
    bi.ulFlags = 0;
    pidl = SHBrowseForFolderA(&bi);
    CoTaskMemFree(pidl);

    /* test with flag */
    bi.ulFlags = BIF_NEWDIALOGSTYLE;
    pidl = SHBrowseForFolderA(&bi);
    CoTaskMemFree(pidl);

    IShellFolder_Release(desktop_object);

    CoUninitialize();
}

START_TEST(brsfolder)
{
    test_click_make_new_folder_button();
    test_selection();
}
