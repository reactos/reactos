#pragma once

#define TEMP_FILE "shell-notify-temporary.txt"

typedef enum TYPE
{
    TYPE_RENAMEITEM,
    TYPE_CREATE,
    TYPE_DELETE,
    TYPE_MKDIR,
    TYPE_RMDIR,
    TYPE_UPDATEDIR,
    TYPE_UPDATEITEM,
    TYPE_RENAMEFOLDER
} TYPE;

#define WM_SHELL_NOTIFY (WM_USER + 100)
#define WM_GET_NOTIFY_FLAGS (WM_USER + 101)
#define WM_CLEAR_FLAGS (WM_USER + 102)
#define WM_SET_PATHS (WM_USER + 103)

static WCHAR s_dir1[MAX_PATH];  // "%TEMP%\\WatchDir1"
static WCHAR s_dir2[MAX_PATH];  // "%TEMP%\\WatchDir1\\Dir2"
static WCHAR s_dir3[MAX_PATH];  // "%TEMP%\\WatchDir1\\Dir3"
static WCHAR s_file1[MAX_PATH]; // "%TEMP%\\WatchDir1\\File1.txt"
static WCHAR s_file2[MAX_PATH]; // "%TEMP%\\WatchDir1\\File2.txt"

inline void DoInitPaths(void)
{
    WCHAR szTemp[MAX_PATH], szPath[MAX_PATH];
    GetTempPathW(_countof(szTemp), szTemp);
    GetLongPathNameW(szTemp, szPath, _countof(szPath));

    lstrcpyW(s_dir1, szPath);
    PathAppendW(s_dir1, L"WatchDir1");

    lstrcpyW(s_dir2, s_dir1);
    PathAppendW(s_dir2, L"Dir2");

    lstrcpyW(s_dir3, s_dir1);
    PathAppendW(s_dir3, L"Dir3");

    lstrcpyW(s_file1, s_dir1);
    PathAppendW(s_file1, L"File1.txt");

    lstrcpyW(s_file2, s_dir1);
    PathAppendW(s_file2, L"File2.txt");
}
