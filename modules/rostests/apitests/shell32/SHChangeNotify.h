#pragma once

#define TEMP_FILE "shell-notify-temporary.txt"
#define CLASSNAME L"SHChangeNotify testcase"
#define EVENT_NAME "SHChangeNotify testcase event"

typedef enum TYPE
{
    TYPE_RENAMEITEM,
    TYPE_CREATE,
    TYPE_DELETE,
    TYPE_MKDIR,
    TYPE_RMDIR,
    TYPE_RENAMEFOLDER,
    TYPE_UPDATEDIR,
    TYPE_MAX = TYPE_UPDATEDIR
} TYPE;

#define WM_SHELL_NOTIFY (WM_USER + 100)
#define WM_GET_NOTIFY_FLAGS (WM_USER + 101)
#define WM_CLEAR_FLAGS (WM_USER + 102)
#define WM_SET_PATHS (WM_USER + 103)

typedef enum WATCHDIR
{
    WATCHDIR_NULL = 0,
    WATCHDIR_DESKTOP,
    WATCHDIR_MYCOMPUTER,
    WATCHDIR_MYDOCUMENTS
} WATCHDIR;

inline LPITEMIDLIST GetWatchPidl(WATCHDIR iWatchDir)
{
    LPITEMIDLIST ret;

    switch (iWatchDir)
    {
        case WATCHDIR_NULL:
            return NULL;

        case WATCHDIR_DESKTOP:
            SHGetSpecialFolderLocation(NULL, CSIDL_DESKTOP, &ret);
            break;

        case WATCHDIR_MYCOMPUTER:
            SHGetSpecialFolderLocation(NULL, CSIDL_DRIVES, &ret);
            break;

        case WATCHDIR_MYDOCUMENTS:
            SHGetSpecialFolderLocation(NULL, CSIDL_PERSONAL, &ret);
            break;
    }

    return ret;
}

inline LPWSTR GetWatchDir(WATCHDIR iWatchDir)
{
    static size_t s_index = 0;
    static WCHAR s_pathes[4][MAX_PATH];
    LPWSTR psz = s_pathes[s_index];
    psz[0] = 0;
    LPITEMIDLIST pidl = GetWatchPidl(iWatchDir);
    SHGetPathFromIDListW(pidl, psz);
    CoTaskMemFree(pidl);
    ++s_index;
    s_index %= _countof(s_pathes);
    return psz;
}
