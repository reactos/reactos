#pragma once

#include <shlwapi.h>
#include <stdlib.h>
#include <stdio.h>
#include <strsafe.h>

#define TEMP_FILE   L"shell-notify-temporary.txt"
#define CLASSNAME   L"SHChangeNotify testcase window"
#define EVENT_NAME  L"SHChangeNotify testcase event"

#define WM_SHELL_NOTIFY     (WM_USER + 100)
#define WM_GET_NOTIFY_FLAGS (WM_USER + 101)
#define WM_CLEAR_FLAGS      (WM_USER + 102)
#define WM_SET_PATHS        (WM_USER + 103)

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

typedef enum DIRTYPE
{
    DIRTYPE_NULL = 0,
    DIRTYPE_DESKTOP,
    DIRTYPE_MYCOMPUTER,
    DIRTYPE_MYDOCUMENTS
} DIRTYPE;

inline LPITEMIDLIST DoGetPidl(DIRTYPE iDir)
{
    LPITEMIDLIST ret = NULL;

    switch (iDir)
    {
        case DIRTYPE_NULL:
            break;

        case DIRTYPE_DESKTOP:
            SHGetSpecialFolderLocation(NULL, CSIDL_DESKTOP, &ret);
            break;

        case DIRTYPE_MYCOMPUTER:
            SHGetSpecialFolderLocation(NULL, CSIDL_DRIVES, &ret);
            break;

        case DIRTYPE_MYDOCUMENTS:
            SHGetSpecialFolderLocation(NULL, CSIDL_PERSONAL, &ret);
            break;
    }

    return ret;
}

static inline LPWSTR DoGetDir(DIRTYPE iDir)
{
    static size_t s_index = 0;
    static WCHAR s_pathes[3][MAX_PATH];
    LPWSTR psz = s_pathes[s_index];
    LPITEMIDLIST pidl = DoGetPidl(iDir);
    psz[0] = 0;
    SHGetPathFromIDListW(pidl, psz);
    CoTaskMemFree(pidl);
    s_index = (s_index + 1) % _countof(s_pathes);
    return psz;
}

static inline HWND DoWaitForWindow(LPCWSTR clsname, LPCWSTR text, BOOL bClosing, BOOL bForce)
{
    HWND hwnd = NULL;
    for (INT i = 0; i < 50; ++i)
    {
        hwnd = FindWindowW(clsname, text);
        if (bClosing)
        {
            if (!hwnd)
                break;

            if (bForce)
                PostMessage(hwnd, WM_CLOSE, 0, 0);
        }
        else
        {
            if (hwnd)
                break;
        }

        Sleep(1);
    }
    return hwnd;
}
