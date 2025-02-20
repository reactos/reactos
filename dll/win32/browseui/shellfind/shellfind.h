#ifndef _SHELLFIND_PCH_
#define _SHELLFIND_PCH_

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#include <windef.h>
#include <winbase.h>
#include <shlobj.h>
#include <shlobj_undoc.h>
#include <shlguid_undoc.h>
#include <shdeprecated.h>
#include <tchar.h>
#include <atlbase.h>
#include <atlcom.h>
#include <atlwin.h>
#include <atlsimpcoll.h>
#include <atlstr.h>
#include <shlwapi.h>
#include <shlwapi_undoc.h>
#include <undocshell.h>
#include <shellutils.h>
#include <strsafe.h>
#include <wine/debug.h>

#include "../resource.h"

#define WM_SEARCH_START          WM_USER + 0
#define WM_SEARCH_STOP           WM_USER + 1
#define WM_SEARCH_ADD_RESULT     WM_USER + 2
#define WM_SEARCH_UPDATE_STATUS  WM_USER + 3

typedef struct tagLOCATIONITEM
{
    struct tagLOCATIONITEM *pNext;
    WCHAR szPath[ANYSIZE_ARRAY];
} LOCATIONITEM;

void FreeList(LOCATIONITEM *pLI);

struct ScopedFreeLocationItems
{
    LOCATIONITEM *m_ptr;
    ScopedFreeLocationItems(LOCATIONITEM *ptr) : m_ptr(ptr) {}
    ~ScopedFreeLocationItems() { FreeList(m_ptr); }
    void Detach() { m_ptr = NULL; }
};

struct SearchStart
{
    LOCATIONITEM *pPaths;
    WCHAR szFileName[MAX_PATH];
    WCHAR szQuery[MAX_PATH];
    BOOL  SearchHidden;
};

template<class T, class F, class R>
static INT_PTR FindItemInComboEx(HWND hCombo, T &FindItem, F CompareFunc, R RetMatch)
{
    COMBOBOXEXITEMW item;
    item.mask = CBEIF_LPARAM;
    item.cchTextMax = 0;
    for (item.iItem = 0; SendMessageW(hCombo, CBEM_GETITEMW, 0, (LPARAM)&item); item.iItem++)
    {
        if (CompareFunc((T&)item.lParam, FindItem) == RetMatch)
            return item.iItem;
    }
    return -1;
}

static inline bool PathIsOnDrive(PCWSTR Path)
{
    return PathGetDriveNumberW(Path) >= 0 && (Path[2] == '\\' || !Path[2]);
}

static inline BOOL PathIsOnUnc(PCWSTR Path)
{
    return PathIsUNCW(Path); // FIXME: Verify the path starts with <\\Server\Share>[\]
}

static inline bool PathIsAbsolute(PCWSTR Path)
{
    // Note: PathIsRelativeW is too forgiving
    return PathIsOnDrive(Path) || PathIsOnUnc(Path);
}

#endif /* _SHELLFIND_PCH_ */
