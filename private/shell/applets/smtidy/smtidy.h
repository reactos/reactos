//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------

#undef DEBUG

#define DONT_WANT_SHELLDEBUG
#define STRICT
#define _INC_OLE

#include <windows.h>
#include <windowsx.h>
#include <shlwapi.h>
#ifdef WINNT_ENV
#include <shlwapip.h>
#endif


#include <commctrl.h>
#include <shellapi.h>
#include <shlobj.h>
#include <shsemip.h>
#include <limits.h>

#include <shellp.h>

//----------------------------------------------------------------------------
// Custom warnings - Optimisations must be enable for these to have any effect.
// 4100    "'%$S' : unreferenced formal parameter"
// 4101    "'%$S' : unreferenced local variable"
// 4102    "'%$S' : unreferenced label"
// 4700    "local variable '%s' used without having been initialized"
// 4701    "local variable '%s' may be used without having been initialized"
// 4702    "unreachable code"
// 4706    "assignment within conditional expression"
#pragma warning(3 : 4100 4101 4102 4700 4701 4702 4706)

//----------------------------------------------------------------------------
#ifndef SIZEOF
#define SIZEOF              sizeof
#endif
#ifndef ARRAYSIZE
#define ARRAYSIZE(a)                (sizeof(a)/sizeof(a[0]))
#endif
#define MAX_PATH            260
#define CB_MAX_PATH         (MAX_PATH*SIZEOF(TCHAR))
#define USE(x)        x = x

typedef unsigned __int64 QWORD;
typedef LPITEMIDLIST PIDL;
typedef LPITEMIDLIST *PPIDL;


//----------------------------------------------------------------------------
extern HINSTANCE g_hinstApp;

//----------------------------------------------------------------------------
#define     SMIF_NONE                   0x0000
#define     SMIF_BROKEN_SHORTCUT        0x0001
#define     SMIF_README                 0x0002
#define     SMIF_SINGLE_ITEM            0x0004
#define     SMIF_DELETE                 0x0008
#define     SMIF_UNUSED_SHORTCUT        0x0010
#define     SMIF_FOLDER                 0x0020
#define     SMIF_GROUP                  0x0040
#define     SMIF_MOVE_UP                0x0080
#define     SMIF_EMPTY_FOLDER           0x0100
#define     SMIF_FIX                    0x0200
#define     SMIF_TARGET_NOT_FILE        0x0400

//----------------------------------------------------------------------------
typedef struct
{
    DWORD               dwFlags;
    PIDL                pidlItem;
    LPTSTR              pszTarget;
    LPTSTR              pszNewTarget;
    DWORD               dwMatch;
    int                 nScore;
    WIN32_FIND_DATA     *pfd;
} SMITEM;
typedef SMITEM *PSMITEM;

//----------------------------------------------------------------------------
#define     SMTIF_NONE                          0x0000
#define     SMTIF_BUILT_LIST                    0x0001
#define     SMTIF_LOST_TARGETS                  0x0002
#define     SMTIF_READMES                       0x0004
#define     SMTIF_UNUSED_SHORTCUTS              0x0008
#define     SMTIF_SINGLE_ITEM_FOLDERS           0x0010
#define     SMTIF_FIX_BROKEN_SHORTCUTS          0x0020
#define     SMTIF_GROUP_BROKEN_SHORTCUTS        0x0040
#define     SMTIF_DELETE_BROKEN_SHORTCUTS       0x0080
#define     SMTIF_FIX_SINGLE_ITEM_FOLDERS       0x0100
#define     SMTIF_GROUP_UNUSED_SHORTCUTS        0x0200
#define     SMTIF_DELETE_UNUSED_SHORTCUTS       0x0400
#define     SMTIF_STOP_THREAD                   0x0800
#define     SMTIF_EMPTY_FOLDERS                 0x1000
#define     SMTIF_REMOVE_EMPTY_FOLDERS          0x2000
#define     SMTIF_GROUP_READMES                 0x4000
#define     SMTIF_DELETE_READMES                0x8000

//----------------------------------------------------------------------------
typedef struct
{
    DWORD           dwFlags;
    HBITMAP         hbmp;
    HDSA            hdsaSMI;
    IShellLink      *psl;
    IPersistFile    *ppf;
    PTSTR           pszLostTargetGroup;
    PTSTR           pszReadMeGroup;
    PTSTR           pszUnusedShortcutGroup;
    PTSTR           pszSearchOrigin;
    HANDLE          hThread;
    HWND            hDlg;
    HDPA            hdpa;
    QWORD           qwSeen;
    DWORD           dwBPC;
    HFONT           hfontLarge;
    HWND            hwnd;
    RECT            rc;
} SMTIDYINFO;
typedef SMTIDYINFO *PSMTIDYINFO;

// Don't use StrToOleStr unless ansi to ansi system or unicode to unicode
// as it it s private import to shell32...
#ifdef UNICODE
#define STRTOOLESTR(olestr, tstr) lstrcpy(olestr, tstr)
#else
#define STRTOOLESTR(olestr, tstr) MultiByteToWideChar(CP_ACP, 0, tstr, -1, olestr, MAX_PATH)
#endif
