/*
 * SHFileOperation
 *
 * Copyright 2000 Juergen Schmied
 * Copyright 2002 Andriy Palamarchuk
 * Copyright 2004 Dietrich Teickner (from Odin)
 * Copyright 2004 Rolf Kalbermatter
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

#include "config.h"
#include "wine/port.h"

#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "shellapi.h"
#include "wingdi.h"
#include "winuser.h"
#include "shlobj.h"
#include "shresdef.h"
#define NO_SHLWAPI_STREAM
#include "shlwapi.h"
#include "shell32_main.h"
#include "undocshell.h"
#include "wine/debug.h"
#include "xdg.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

#define IsAttrib(x, y)  ((INVALID_FILE_ATTRIBUTES != (x)) && ((x) & (y)))
#define IsAttribFile(x) (!((x) & FILE_ATTRIBUTE_DIRECTORY))
#define IsAttribDir(x)  IsAttrib(x, FILE_ATTRIBUTE_DIRECTORY)
#define IsDotDir(x)     ((x[0] == '.') && ((x[1] == 0) || ((x[1] == '.') && (x[2] == 0))))

#define FO_MASK         0xF

static const WCHAR wWildcardFile[] = {'*',0};
static const WCHAR wWildcardChars[] = {'*','?',0};

static DWORD SHNotifyCreateDirectoryA(LPCSTR path, LPSECURITY_ATTRIBUTES sec);
static DWORD SHNotifyCreateDirectoryW(LPCWSTR path, LPSECURITY_ATTRIBUTES sec);
static DWORD SHNotifyRemoveDirectoryA(LPCSTR path);
static DWORD SHNotifyRemoveDirectoryW(LPCWSTR path);
static DWORD SHNotifyDeleteFileA(LPCSTR path);
static DWORD SHNotifyDeleteFileW(LPCWSTR path);
static DWORD SHNotifyMoveFileW(LPCWSTR src, LPCWSTR dest);
static DWORD SHNotifyCopyFileW(LPCWSTR src, LPCWSTR dest, BOOL bFailIfExists);
static DWORD SHFindAttrW(LPCWSTR pName, BOOL fileOnly);

typedef struct
{
    SHFILEOPSTRUCTW *req;
    DWORD dwYesToAllMask;
    BOOL bManyItems;
    BOOL bCancelled;
} FILE_OPERATION;

/* Confirm dialogs with an optional "Yes To All" as used in file operations confirmations
 */
static const WCHAR CONFIRM_MSG_PROP[] = {'W','I','N','E','_','C','O','N','F','I','R','M',0};

struct confirm_msg_info
{
    LPWSTR lpszText;
    LPWSTR lpszCaption;
    HICON hIcon;
    BOOL bYesToAll;
};

/* as some buttons may be hidden and the dialog height may change we may need
 * to move the controls */
static void confirm_msg_move_button(HWND hDlg, INT iId, INT *xPos, INT yOffset, BOOL bShow)
{
    HWND hButton = GetDlgItem(hDlg, iId);
    RECT r;

    if (bShow) {
        POINT pt;
        int width;

        GetWindowRect(hButton, &r);
        width = r.right - r.left;
        pt.x = r.left;
        pt.y = r.top;
        ScreenToClient(hDlg, &pt);
        MoveWindow(hButton, *xPos - width, pt.y - yOffset, width, r.bottom - r.top, FALSE);
        *xPos -= width + 5;
    }
    else
        ShowWindow(hButton, SW_HIDE);
}

/* Note: we paint the text manually and don't use the static control to make
 * sure the text has the same height as the one computed in WM_INITDIALOG
 */
static INT_PTR CALLBACK ConfirmMsgBox_Paint(HWND hDlg)
{
    PAINTSTRUCT ps;
    HFONT hOldFont;
    RECT r;
    HDC hdc;

    BeginPaint(hDlg, &ps);
    hdc = ps.hdc;

    GetClientRect(GetDlgItem(hDlg, IDD_MESSAGE), &r);
    /* this will remap the rect to dialog coords */
    MapWindowPoints(GetDlgItem(hDlg, IDD_MESSAGE), hDlg, (LPPOINT)&r, 2);
    hOldFont = SelectObject(hdc, (HFONT)SendDlgItemMessageW(hDlg, IDD_MESSAGE, WM_GETFONT, 0, 0));
    DrawTextW(hdc, (LPWSTR)GetPropW(hDlg, CONFIRM_MSG_PROP), -1, &r, DT_NOPREFIX | DT_PATH_ELLIPSIS | DT_WORDBREAK);
    SelectObject(hdc, hOldFont);
    EndPaint(hDlg, &ps);
    return TRUE;
}

static INT_PTR CALLBACK ConfirmMsgBox_Init(HWND hDlg, LPARAM lParam)
{
    struct confirm_msg_info *info = (struct confirm_msg_info *)lParam;
    INT xPos, yOffset;
    int width, height;
    HFONT hOldFont;
    HDC hdc;
    RECT r;

    SetWindowTextW(hDlg, info->lpszCaption);
    ShowWindow(GetDlgItem(hDlg, IDD_MESSAGE), SW_HIDE);
    SetPropW(hDlg, CONFIRM_MSG_PROP, (HANDLE)info->lpszText);
    SendDlgItemMessageW(hDlg, IDD_ICON, STM_SETICON, (WPARAM)info->hIcon, 0);

    /* compute the text height and resize the dialog */
    GetClientRect(GetDlgItem(hDlg, IDD_MESSAGE), &r);
    hdc = GetDC(hDlg);
    yOffset = r.bottom;
    hOldFont = SelectObject(hdc, (HFONT)SendDlgItemMessageW(hDlg, IDD_MESSAGE, WM_GETFONT, 0, 0));
    DrawTextW(hdc, info->lpszText, -1, &r, DT_NOPREFIX | DT_PATH_ELLIPSIS | DT_WORDBREAK | DT_CALCRECT);
    SelectObject(hdc, hOldFont);
    yOffset -= r.bottom;
    yOffset = min(yOffset, 35);  /* don't make the dialog too small */
    ReleaseDC(hDlg, hdc);

    GetClientRect(hDlg, &r);
    xPos = r.right - 7;
    GetWindowRect(hDlg, &r);
    width = r.right - r.left;
    height = r.bottom - r.top - yOffset;
    MoveWindow(hDlg, (GetSystemMetrics(SM_CXSCREEN) - width)/2,
        (GetSystemMetrics(SM_CYSCREEN) - height)/2, width, height, FALSE);

    confirm_msg_move_button(hDlg, IDCANCEL,     &xPos, yOffset, info->bYesToAll);
    confirm_msg_move_button(hDlg, IDNO,         &xPos, yOffset, TRUE);
    confirm_msg_move_button(hDlg, IDD_YESTOALL, &xPos, yOffset, info->bYesToAll);
    confirm_msg_move_button(hDlg, IDYES,        &xPos, yOffset, TRUE);
    return TRUE;
}

static INT_PTR CALLBACK ConfirmMsgBoxProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
            return ConfirmMsgBox_Init(hDlg, lParam);
        case WM_PAINT:
            return ConfirmMsgBox_Paint(hDlg);
        case WM_COMMAND:
            EndDialog(hDlg, wParam);
            break;
        case WM_CLOSE:
            EndDialog(hDlg, IDCANCEL);
            break;
    }
    return FALSE;
}

static int SHELL_ConfirmMsgBox(HWND hWnd, LPWSTR lpszText, LPWSTR lpszCaption, HICON hIcon, BOOL bYesToAll)
{
    static const WCHAR wszTemplate[] = {'S','H','E','L','L','_','Y','E','S','T','O','A','L','L','_','M','S','G','B','O','X',0};
    struct confirm_msg_info info;

    info.lpszText = lpszText;
    info.lpszCaption = lpszCaption;
    info.hIcon = hIcon;
    info.bYesToAll = bYesToAll;
    return DialogBoxParamW(shell32_hInstance, wszTemplate, hWnd, ConfirmMsgBoxProc, (LPARAM)&info);
}

/* confirmation dialogs content */
typedef struct
{
        HINSTANCE hIconInstance;
        UINT icon_resource_id;
	UINT caption_resource_id, text_resource_id;
} SHELL_ConfirmIDstruc;

static BOOL SHELL_ConfirmIDs(int nKindOfDialog, SHELL_ConfirmIDstruc *ids)
{
        ids->hIconInstance = shell32_hInstance;
	switch (nKindOfDialog) {
	  case ASK_DELETE_FILE:
            ids->icon_resource_id = IDI_SHELL_CONFIRM_DELETE;
	    ids->caption_resource_id  = IDS_DELETEITEM_CAPTION;
	    ids->text_resource_id  = IDS_DELETEITEM_TEXT;
	    return TRUE;
	  case ASK_DELETE_FOLDER:
            ids->icon_resource_id = IDI_SHELL_CONFIRM_DELETE;
	    ids->caption_resource_id  = IDS_DELETEFOLDER_CAPTION;
	    ids->text_resource_id  = IDS_DELETEITEM_TEXT;
	    return TRUE;
	  case ASK_DELETE_MULTIPLE_ITEM:
            ids->icon_resource_id = IDI_SHELL_CONFIRM_DELETE;
	    ids->caption_resource_id  = IDS_DELETEITEM_CAPTION;
	    ids->text_resource_id  = IDS_DELETEMULTIPLE_TEXT;
	    return TRUE;
          case ASK_TRASH_FILE:
            ids->icon_resource_id = IDI_SHELL_TRASH_FILE;
            ids->caption_resource_id = IDS_DELETEITEM_CAPTION;
            ids->text_resource_id = IDS_TRASHITEM_TEXT;
            return TRUE;
          case ASK_TRASH_FOLDER:
            ids->icon_resource_id = IDI_SHELL_TRASH_FILE;
            ids->caption_resource_id = IDS_DELETEFOLDER_CAPTION;
            ids->text_resource_id = IDS_TRASHFOLDER_TEXT;
            return TRUE;
          case ASK_TRASH_MULTIPLE_ITEM:
            ids->icon_resource_id = IDI_SHELL_TRASH_FILE;
            ids->caption_resource_id = IDS_DELETEITEM_CAPTION;
            ids->text_resource_id = IDS_TRASHMULTIPLE_TEXT;
            return TRUE;
          case ASK_CANT_TRASH_ITEM:
            ids->icon_resource_id = IDI_SHELL_CONFIRM_DELETE;
            ids->caption_resource_id  = IDS_DELETEITEM_CAPTION;
            ids->text_resource_id  = IDS_CANTTRASH_TEXT;
            return TRUE;
	  case ASK_DELETE_SELECTED:
            ids->icon_resource_id = IDI_SHELL_CONFIRM_DELETE;
            ids->caption_resource_id  = IDS_DELETEITEM_CAPTION;
            ids->text_resource_id  = IDS_DELETESELECTED_TEXT;
            return TRUE;
	  case ASK_OVERWRITE_FILE:
            ids->hIconInstance = NULL;
            ids->icon_resource_id = IDI_WARNING;
	    ids->caption_resource_id  = IDS_OVERWRITEFILE_CAPTION;
	    ids->text_resource_id  = IDS_OVERWRITEFILE_TEXT;
            return TRUE;
	  case ASK_OVERWRITE_FOLDER:
            ids->hIconInstance = NULL;
            ids->icon_resource_id = IDI_WARNING;
            ids->caption_resource_id  = IDS_OVERWRITEFILE_CAPTION;
            ids->text_resource_id  = IDS_OVERWRITEFOLDER_TEXT;
            return TRUE;
	  default:
	    FIXME(" Unhandled nKindOfDialog %d stub\n", nKindOfDialog);
	}
	return FALSE;
}

static BOOL SHELL_ConfirmDialogW(HWND hWnd, int nKindOfDialog, LPCWSTR szDir, FILE_OPERATION *op)
{
	WCHAR szCaption[255], szText[255], szBuffer[MAX_PATH + 256];
	SHELL_ConfirmIDstruc ids;
	DWORD_PTR args[1];
	HICON hIcon;
	int ret;

        assert(nKindOfDialog >= 0 && nKindOfDialog < 32);
        if (op && (op->dwYesToAllMask & (1 << nKindOfDialog)))
            return TRUE;

        if (!SHELL_ConfirmIDs(nKindOfDialog, &ids)) return FALSE;

	LoadStringW(shell32_hInstance, ids.caption_resource_id, szCaption, sizeof(szCaption)/sizeof(WCHAR));
	LoadStringW(shell32_hInstance, ids.text_resource_id, szText, sizeof(szText)/sizeof(WCHAR));

	args[0] = (DWORD_PTR)szDir;
	FormatMessageW(FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ARGUMENT_ARRAY,
	               szText, 0, 0, szBuffer, sizeof(szBuffer), (va_list*)args);
        hIcon = LoadIconW(ids.hIconInstance, (LPWSTR)MAKEINTRESOURCE(ids.icon_resource_id));

        ret = SHELL_ConfirmMsgBox(hWnd, szBuffer, szCaption, hIcon, op && op->bManyItems);
        if (op) {
            if (ret == IDD_YESTOALL) {
                op->dwYesToAllMask |= (1 << nKindOfDialog);
                ret = IDYES;
            }
            if (ret == IDCANCEL)
                op->bCancelled = TRUE;
            if (ret != IDYES)
                op->req->fAnyOperationsAborted = TRUE;
        }
        return ret == IDYES;
}

BOOL SHELL_ConfirmYesNoW(HWND hWnd, int nKindOfDialog, LPCWSTR szDir)
{
    return SHELL_ConfirmDialogW(hWnd, nKindOfDialog, szDir, NULL);
}

static DWORD SHELL32_AnsiToUnicodeBuf(LPCSTR aPath, LPWSTR *wPath, DWORD minChars)
{
	DWORD len = MultiByteToWideChar(CP_ACP, 0, aPath, -1, NULL, 0);

	if (len < minChars)
	  len = minChars;

	*wPath = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
	if (*wPath)
	{
	  MultiByteToWideChar(CP_ACP, 0, aPath, -1, *wPath, len);
	  return NO_ERROR;
	}
	return E_OUTOFMEMORY;
}

static void SHELL32_FreeUnicodeBuf(LPWSTR wPath)
{
	HeapFree(GetProcessHeap(), 0, wPath);
}

HRESULT WINAPI SHIsFileAvailableOffline(LPCWSTR path, LPDWORD status)
{
    FIXME("(%s, %p) stub\n", debugstr_w(path), status);
    return E_FAIL;
}

/**************************************************************************
 * SHELL_DeleteDirectory()  [internal]
 *
 * Asks for confirmation when bShowUI is true and deletes the directory and
 * all its subdirectories and files if necessary.
 */
BOOL SHELL_DeleteDirectoryW(HWND hwnd, LPCWSTR pszDir, BOOL bShowUI)
{
	BOOL    ret = TRUE;
	HANDLE  hFind;
	WIN32_FIND_DATAW wfd;
	WCHAR   szTemp[MAX_PATH];

	/* Make sure the directory exists before eventually prompting the user */
	PathCombineW(szTemp, pszDir, wWildcardFile);
	hFind = FindFirstFileW(szTemp, &wfd);
	if (hFind == INVALID_HANDLE_VALUE)
	  return FALSE;

	if (!bShowUI || (ret = SHELL_ConfirmDialogW(hwnd, ASK_DELETE_FOLDER, pszDir, NULL)))
	{
	  do
	  {
	    if (IsDotDir(wfd.cFileName))
	      continue;
	    PathCombineW(szTemp, pszDir, wfd.cFileName);
	    if (FILE_ATTRIBUTE_DIRECTORY & wfd.dwFileAttributes)
	      ret = SHELL_DeleteDirectoryW(hwnd, szTemp, FALSE);
	    else
	      ret = (SHNotifyDeleteFileW(szTemp) == ERROR_SUCCESS);
	  } while (ret && FindNextFileW(hFind, &wfd));
	}
	FindClose(hFind);
	if (ret)
	  ret = (SHNotifyRemoveDirectoryW(pszDir) == ERROR_SUCCESS);
	return ret;
}

/**************************************************************************
 * Win32CreateDirectory      [SHELL32.93]
 *
 * Creates a directory. Also triggers a change notify if one exists.
 *
 * PARAMS
 *  path       [I]   path to directory to create
 *
 * RETURNS
 *  TRUE if successful, FALSE otherwise
 *
 * NOTES
 *  Verified on Win98 / IE 5 (SHELL32 4.72, March 1999 build) to be ANSI.
 *  This is Unicode on NT/2000
 */
static DWORD SHNotifyCreateDirectoryA(LPCSTR path, LPSECURITY_ATTRIBUTES sec)
{
	LPWSTR wPath;
	DWORD retCode;

	TRACE("(%s, %p)\n", debugstr_a(path), sec);

	retCode = SHELL32_AnsiToUnicodeBuf(path, &wPath, 0);
	if (!retCode)
	{
	  retCode = SHNotifyCreateDirectoryW(wPath, sec);
	  SHELL32_FreeUnicodeBuf(wPath);
	}
	return retCode;
}

/**********************************************************************/

static DWORD SHNotifyCreateDirectoryW(LPCWSTR path, LPSECURITY_ATTRIBUTES sec)
{
	TRACE("(%s, %p)\n", debugstr_w(path), sec);

	if (CreateDirectoryW(path, sec))
	{
	  SHChangeNotify(SHCNE_MKDIR, SHCNF_PATHW, path, NULL);
	  return ERROR_SUCCESS;
	}
	return GetLastError();
}

/**********************************************************************/

BOOL WINAPI Win32CreateDirectoryAW(LPCVOID path, LPSECURITY_ATTRIBUTES sec)
{
	if (SHELL_OsIsUnicode())
	  return (SHNotifyCreateDirectoryW(path, sec) == ERROR_SUCCESS);
	return (SHNotifyCreateDirectoryA(path, sec) == ERROR_SUCCESS);
}

/************************************************************************
 * Win32RemoveDirectory      [SHELL32.94]
 *
 * Deletes a directory. Also triggers a change notify if one exists.
 *
 * PARAMS
 *  path       [I]   path to directory to delete
 *
 * RETURNS
 *  TRUE if successful, FALSE otherwise
 *
 * NOTES
 *  Verified on Win98 / IE 5 (SHELL32 4.72, March 1999 build) to be ANSI.
 *  This is Unicode on NT/2000
 */
static DWORD SHNotifyRemoveDirectoryA(LPCSTR path)
{
	LPWSTR wPath;
	DWORD retCode;

	TRACE("(%s)\n", debugstr_a(path));

	retCode = SHELL32_AnsiToUnicodeBuf(path, &wPath, 0);
	if (!retCode)
	{
	  retCode = SHNotifyRemoveDirectoryW(wPath);
	  SHELL32_FreeUnicodeBuf(wPath);
	}
	return retCode;
}

/***********************************************************************/

static DWORD SHNotifyRemoveDirectoryW(LPCWSTR path)
{
	BOOL ret;
	TRACE("(%s)\n", debugstr_w(path));

	ret = RemoveDirectoryW(path);
	if (!ret)
	{
	  /* Directory may be write protected */
	  DWORD dwAttr = GetFileAttributesW(path);
	  if (IsAttrib(dwAttr, FILE_ATTRIBUTE_READONLY))
	    if (SetFileAttributesW(path, dwAttr & ~FILE_ATTRIBUTE_READONLY))
	      ret = RemoveDirectoryW(path);
	}
	if (ret)
	{
	  SHChangeNotify(SHCNE_RMDIR, SHCNF_PATHW, path, NULL);
	  return ERROR_SUCCESS;
	}
	return GetLastError();
}

/***********************************************************************/

BOOL WINAPI Win32RemoveDirectoryAW(LPCVOID path)
{
	if (SHELL_OsIsUnicode())
	  return (SHNotifyRemoveDirectoryW(path) == ERROR_SUCCESS);
	return (SHNotifyRemoveDirectoryA(path) == ERROR_SUCCESS);
}

/************************************************************************
 * Win32DeleteFile           [SHELL32.164]
 *
 * Deletes a file. Also triggers a change notify if one exists.
 *
 * PARAMS
 *  path       [I]   path to file to delete
 *
 * RETURNS
 *  TRUE if successful, FALSE otherwise
 *
 * NOTES
 *  Verified on Win98 / IE 5 (SHELL32 4.72, March 1999 build) to be ANSI.
 *  This is Unicode on NT/2000
 */
static DWORD SHNotifyDeleteFileA(LPCSTR path)
{
	LPWSTR wPath;
	DWORD retCode;

	TRACE("(%s)\n", debugstr_a(path));

	retCode = SHELL32_AnsiToUnicodeBuf(path, &wPath, 0);
	if (!retCode)
	{
	  retCode = SHNotifyDeleteFileW(wPath);
	  SHELL32_FreeUnicodeBuf(wPath);
	}
	return retCode;
}

/***********************************************************************/

static DWORD SHNotifyDeleteFileW(LPCWSTR path)
{
	BOOL ret;

	TRACE("(%s)\n", debugstr_w(path));

	ret = DeleteFileW(path);
	if (!ret)
	{
	  /* File may be write protected or a system file */
	  DWORD dwAttr = GetFileAttributesW(path);
	  if (IsAttrib(dwAttr, FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_SYSTEM))
	    if (SetFileAttributesW(path, dwAttr & ~(FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_SYSTEM)))
	      ret = DeleteFileW(path);
	}
	if (ret)
	{
	  SHChangeNotify(SHCNE_DELETE, SHCNF_PATHW, path, NULL);
	  return ERROR_SUCCESS;
	}
	return GetLastError();
}

/***********************************************************************/

DWORD WINAPI Win32DeleteFileAW(LPCVOID path)
{
	if (SHELL_OsIsUnicode())
	  return (SHNotifyDeleteFileW(path) == ERROR_SUCCESS);
	return (SHNotifyDeleteFileA(path) == ERROR_SUCCESS);
}

/************************************************************************
 * SHNotifyMoveFile          [internal]
 *
 * Moves a file. Also triggers a change notify if one exists.
 *
 * PARAMS
 *  src        [I]   path to source file to move
 *  dest       [I]   path to target file to move to
 *
 * RETURNS
 *  ERORR_SUCCESS if successful
 */
static DWORD SHNotifyMoveFileW(LPCWSTR src, LPCWSTR dest)
{
	BOOL ret;

	TRACE("(%s %s)\n", debugstr_w(src), debugstr_w(dest));

        ret = MoveFileExW(src, dest, MOVEFILE_REPLACE_EXISTING);

        /* MOVEFILE_REPLACE_EXISTING fails with dirs, so try MoveFile */
        if (!ret)
            ret = MoveFileW(src, dest);

	if (!ret)
	{
	  DWORD dwAttr;

	  dwAttr = SHFindAttrW(dest, FALSE);
	  if (INVALID_FILE_ATTRIBUTES == dwAttr)
	  {
	    /* Source file may be write protected or a system file */
	    dwAttr = GetFileAttributesW(src);
	    if (IsAttrib(dwAttr, FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_SYSTEM))
	      if (SetFileAttributesW(src, dwAttr & ~(FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_SYSTEM)))
	        ret = MoveFileW(src, dest);
	  }
	}
	if (ret)
	{
	  SHChangeNotify(SHCNE_RENAMEITEM, SHCNF_PATHW, src, dest);
	  return ERROR_SUCCESS;
	}
	return GetLastError();
}

/************************************************************************
 * SHNotifyCopyFile          [internal]
 *
 * Copies a file. Also triggers a change notify if one exists.
 *
 * PARAMS
 *  src           [I]   path to source file to move
 *  dest          [I]   path to target file to move to
 *  bFailIfExists [I]   if TRUE, the target file will not be overwritten if
 *                      a file with this name already exists
 *
 * RETURNS
 *  ERROR_SUCCESS if successful
 */
static DWORD SHNotifyCopyFileW(LPCWSTR src, LPCWSTR dest, BOOL bFailIfExists)
{
	BOOL ret;

	TRACE("(%s %s %s)\n", debugstr_w(src), debugstr_w(dest), bFailIfExists ? "failIfExists" : "");

	ret = CopyFileW(src, dest, bFailIfExists);
	if (ret)
	{
	  SHChangeNotify(SHCNE_CREATE, SHCNF_PATHW, dest, NULL);
	  return ERROR_SUCCESS;
	}

	return GetLastError();
}

/*************************************************************************
 * SHCreateDirectory         [SHELL32.165]
 *
 * This function creates a file system folder whose fully qualified path is
 * given by path. If one or more of the intermediate folders do not exist,
 * they will be created as well.
 *
 * PARAMS
 *  hWnd       [I]
 *  path       [I]   path of directory to create
 *
 * RETURNS
 *  ERRROR_SUCCESS or one of the following values:
 *  ERROR_BAD_PATHNAME if the path is relative
 *  ERROR_FILE_EXISTS when a file with that name exists
 *  ERROR_PATH_NOT_FOUND can't find the path, probably invalid
 *  ERROR_INVLID_NAME if the path contains invalid chars
 *  ERROR_ALREADY_EXISTS when the directory already exists
 *  ERROR_FILENAME_EXCED_RANGE if the filename was to long to process
 *
 * NOTES
 *  exported by ordinal
 *  Win9x exports ANSI
 *  WinNT/2000 exports Unicode
 */
DWORD WINAPI SHCreateDirectory(HWND hWnd, LPCVOID path)
{
	if (SHELL_OsIsUnicode())
	  return SHCreateDirectoryExW(hWnd, path, NULL);
	return SHCreateDirectoryExA(hWnd, path, NULL);
}

/*************************************************************************
 * SHCreateDirectoryExA      [SHELL32.@]
 *
 * This function creates a file system folder whose fully qualified path is
 * given by path. If one or more of the intermediate folders do not exist,
 * they will be created as well.
 *
 * PARAMS
 *  hWnd       [I]
 *  path       [I]   path of directory to create
 *  sec        [I]   security attributes to use or NULL
 *
 * RETURNS
 *  ERRROR_SUCCESS or one of the following values:
 *  ERROR_BAD_PATHNAME or ERROR_PATH_NOT_FOUND if the path is relative
 *  ERROR_INVLID_NAME if the path contains invalid chars
 *  ERROR_FILE_EXISTS when a file with that name exists
 *  ERROR_ALREADY_EXISTS when the directory already exists
 *  ERROR_FILENAME_EXCED_RANGE if the filename was to long to process
 *
 *  FIXME: Not implemented yet;
 *  SHCreateDirectoryEx also verifies that the files in the directory will be visible
 *  if the path is a network path to deal with network drivers which might have a limited
 *  but unknown maximum path length. If not:
 *
 *  If hWnd is set to a valid window handle, a message box is displayed warning
 *  the user that the files may not be accessible. If the user chooses not to
 *  proceed, the function returns ERROR_CANCELLED.
 *
 *  If hWnd is set to NULL, no user interface is displayed and the function
 *  returns ERROR_CANCELLED.
 */
int WINAPI SHCreateDirectoryExA(HWND hWnd, LPCSTR path, LPSECURITY_ATTRIBUTES sec)
{
	LPWSTR wPath;
	DWORD retCode;

	TRACE("(%s, %p)\n", debugstr_a(path), sec);

	retCode = SHELL32_AnsiToUnicodeBuf(path, &wPath, 0);
	if (!retCode)
	{
	  retCode = SHCreateDirectoryExW(hWnd, wPath, sec);
	  SHELL32_FreeUnicodeBuf(wPath);
	}
	return retCode;
}

/*************************************************************************
 * SHCreateDirectoryExW      [SHELL32.@]
 *
 * See SHCreateDirectoryExA.
 */
int WINAPI SHCreateDirectoryExW(HWND hWnd, LPCWSTR path, LPSECURITY_ATTRIBUTES sec)
{
	int ret = ERROR_BAD_PATHNAME;
	TRACE("(%p, %s, %p)\n", hWnd, debugstr_w(path), sec);

	if (PathIsRelativeW(path))
	{
	  SetLastError(ret);
	}
	else
	{
	  ret = SHNotifyCreateDirectoryW(path, sec);
	  /* Refuse to work on certain error codes before trying to create directories recursively */
	  if (ret != ERROR_SUCCESS &&
	      ret != ERROR_FILE_EXISTS &&
	      ret != ERROR_ALREADY_EXISTS &&
	      ret != ERROR_FILENAME_EXCED_RANGE)
	  {
	    WCHAR *pEnd, *pSlash, szTemp[MAX_PATH + 1];  /* extra for PathAddBackslash() */

	    lstrcpynW(szTemp, path, MAX_PATH);
	    pEnd = PathAddBackslashW(szTemp);
	    pSlash = szTemp + 3;

	    while (*pSlash)
	    {
              while (*pSlash && *pSlash != '\\') pSlash++;
	      if (*pSlash)
	      {
	        *pSlash = 0;    /* terminate path at separator */

	        ret = SHNotifyCreateDirectoryW(szTemp, pSlash + 1 == pEnd ? sec : NULL);
	      }
	      *pSlash++ = '\\'; /* put the separator back */
	    }
	  }

	  if (ret && hWnd && (ERROR_CANCELLED != ret))
	  {
	    /* We failed and should show a dialog box */
	    FIXME("Show system error message, creating path %s, failed with error %d\n", debugstr_w(path), ret);
	    ret = ERROR_CANCELLED; /* Error has been already presented to user (not really yet!) */
	  }
	}
	return ret;
}

/*************************************************************************
 * SHFindAttrW      [internal]
 *
 * Get the Attributes for a file or directory. The difference to GetAttributes()
 * is that this function will also work for paths containing wildcard characters
 * in its filename.

 * PARAMS
 *  path       [I]   path of directory or file to check
 *  fileOnly   [I]   TRUE if only files should be found
 *
 * RETURNS
 *  INVALID_FILE_ATTRIBUTES if the path does not exist, the actual attributes of
 *  the first file or directory found otherwise
 */
static DWORD SHFindAttrW(LPCWSTR pName, BOOL fileOnly)
{
	WIN32_FIND_DATAW wfd;
	BOOL b_FileMask = fileOnly && (NULL != StrPBrkW(pName, wWildcardChars));
	DWORD dwAttr = INVALID_FILE_ATTRIBUTES;
	HANDLE hFind = FindFirstFileW(pName, &wfd);

	TRACE("%s %d\n", debugstr_w(pName), fileOnly);
	if (INVALID_HANDLE_VALUE != hFind)
	{
	  do
	  {
	    if (b_FileMask && IsAttribDir(wfd.dwFileAttributes))
	       continue;
	    dwAttr = wfd.dwFileAttributes;
	    break;
	  }
	  while (FindNextFileW(hFind, &wfd));
	  FindClose(hFind);
	}
	return dwAttr;
}

/*************************************************************************
 *
 * SHNameTranslate HelperFunction for SHFileOperationA
 *
 * Translates a list of 0 terminated ASCII strings into Unicode. If *wString
 * is NULL, only the necessary size of the string is determined and returned,
 * otherwise the ASCII strings are copied into it and the buffer is increased
 * to point to the location after the final 0 termination char.
 */
static DWORD SHNameTranslate(LPWSTR* wString, LPCWSTR* pWToFrom, BOOL more)
{
	DWORD size = 0, aSize = 0;
	LPCSTR aString = (LPCSTR)*pWToFrom;

	if (aString)
	{
	  do
	  {
	    size = lstrlenA(aString) + 1;
	    aSize += size;
	    aString += size;
	  } while ((size != 1) && more);
	  /* The two sizes might be different in the case of multibyte chars */
	  size = MultiByteToWideChar(CP_ACP, 0, (LPCSTR)*pWToFrom, aSize, *wString, 0);
	  if (*wString) /* only in the second loop */
	  {
	    MultiByteToWideChar(CP_ACP, 0, (LPCSTR)*pWToFrom, aSize, *wString, size);
	    *pWToFrom = *wString;
	    *wString += size;
	  }
	}
	return size;
}
/*************************************************************************
 * SHFileOperationA          [SHELL32.@]
 *
 * Function to copy, move, delete and create one or more files with optional
 * user prompts.
 *
 * PARAMS
 *  lpFileOp   [I/O] pointer to a structure containing all the necessary information
 *
 * RETURNS
 *  Success: ERROR_SUCCESS.
 *  Failure: ERROR_CANCELLED.
 *
 * NOTES
 *  exported by name
 */
int WINAPI SHFileOperationA(LPSHFILEOPSTRUCTA lpFileOp)
{
	SHFILEOPSTRUCTW nFileOp = *((LPSHFILEOPSTRUCTW)lpFileOp);
	int retCode = 0;
	DWORD size;
	LPWSTR ForFree = NULL, /* we change wString in SHNameTranslate and can't use it for freeing */
	       wString = NULL; /* we change this in SHNameTranslate */

	TRACE("\n");
	if (FO_DELETE == (nFileOp.wFunc & FO_MASK))
	  nFileOp.pTo = NULL; /* we need a NULL or a valid pointer for translation */
	if (!(nFileOp.fFlags & FOF_SIMPLEPROGRESS))
	  nFileOp.lpszProgressTitle = NULL; /* we need a NULL or a valid pointer for translation */
	while (1) /* every loop calculate size, second translate also, if we have storage for this */
	{
	  size = SHNameTranslate(&wString, &nFileOp.lpszProgressTitle, FALSE); /* no loop */
	  size += SHNameTranslate(&wString, &nFileOp.pFrom, TRUE); /* internal loop */
	  size += SHNameTranslate(&wString, &nFileOp.pTo, TRUE); /* internal loop */

	  if (ForFree)
	  {
	    retCode = SHFileOperationW(&nFileOp);
	    HeapFree(GetProcessHeap(), 0, ForFree); /* we cannot use wString, it was changed */
	    break;
	  }
	  else
	  {
	    wString = ForFree = HeapAlloc(GetProcessHeap(), 0, size * sizeof(WCHAR));
	    if (ForFree) continue;
	    retCode = ERROR_OUTOFMEMORY;
	    nFileOp.fAnyOperationsAborted = TRUE;
	    SetLastError(retCode);
	    return retCode;
	  }
	}

	lpFileOp->hNameMappings = nFileOp.hNameMappings;
	lpFileOp->fAnyOperationsAborted = nFileOp.fAnyOperationsAborted;
	return retCode;
}

#define ERROR_SHELL_INTERNAL_FILE_NOT_FOUND 1026

typedef struct
{
    DWORD attributes;
    LPWSTR szDirectory;
    LPWSTR szFilename;
    LPWSTR szFullPath;
    BOOL bFromWildcard;
    BOOL bFromRelative;
    BOOL bExists;
} FILE_ENTRY;

typedef struct
{
    FILE_ENTRY *feFiles;
    DWORD num_alloc;
    DWORD dwNumFiles;
    BOOL bAnyFromWildcard;
    BOOL bAnyDirectories;
    BOOL bAnyDontExist;
} FILE_LIST;


static inline void grow_list(FILE_LIST *list)
{
    FILE_ENTRY *new = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, list->feFiles,
                                  list->num_alloc * 2 * sizeof(*new) );
    list->feFiles = new;
    list->num_alloc *= 2;
}

/* adds a file to the FILE_ENTRY struct
 */
static void add_file_to_entry(FILE_ENTRY *feFile, LPWSTR szFile)
{
    DWORD dwLen = lstrlenW(szFile) + 1;
    LPWSTR ptr;

    feFile->szFullPath = HeapAlloc(GetProcessHeap(), 0, dwLen * sizeof(WCHAR));
    lstrcpyW(feFile->szFullPath, szFile);

    ptr = StrRChrW(szFile, NULL, '\\');
    if (ptr)
    {
        dwLen = ptr - szFile + 1;
        feFile->szDirectory = HeapAlloc(GetProcessHeap(), 0, dwLen * sizeof(WCHAR));
        lstrcpynW(feFile->szDirectory, szFile, dwLen);

        dwLen = lstrlenW(feFile->szFullPath) - dwLen + 1;
        feFile->szFilename = HeapAlloc(GetProcessHeap(), 0, dwLen * sizeof(WCHAR));
        lstrcpyW(feFile->szFilename, ptr + 1); /* skip over backslash */
    }
    feFile->bFromWildcard = FALSE;
}

static LPWSTR wildcard_to_file(LPWSTR szWildCard, LPWSTR szFileName)
{
    LPWSTR szFullPath, ptr;
    DWORD dwDirLen, dwFullLen;

    ptr = StrRChrW(szWildCard, NULL, '\\');
    dwDirLen = ptr - szWildCard + 1;

    dwFullLen = dwDirLen + lstrlenW(szFileName) + 1;
    szFullPath = HeapAlloc(GetProcessHeap(), 0, dwFullLen * sizeof(WCHAR));

    lstrcpynW(szFullPath, szWildCard, dwDirLen + 1);
    lstrcatW(szFullPath, szFileName);

    return szFullPath;
}

static void parse_wildcard_files(FILE_LIST *flList, LPWSTR szFile, LPDWORD pdwListIndex)
{
    WIN32_FIND_DATAW wfd;
    HANDLE hFile = FindFirstFileW(szFile, &wfd);
    FILE_ENTRY *file;
    LPWSTR szFullPath;
    BOOL res;

    if (hFile == INVALID_HANDLE_VALUE) return;

    for (res = TRUE; res; res = FindNextFileW(hFile, &wfd))
    {
        if (IsDotDir(wfd.cFileName)) continue;
        if (*pdwListIndex >= flList->num_alloc) grow_list( flList );
        szFullPath = wildcard_to_file(szFile, wfd.cFileName);
        file = &flList->feFiles[(*pdwListIndex)++];
        add_file_to_entry(file, szFullPath);
        file->bFromWildcard = TRUE;
        file->attributes = wfd.dwFileAttributes;
        if (IsAttribDir(file->attributes)) flList->bAnyDirectories = TRUE;
        HeapFree(GetProcessHeap(), 0, szFullPath);
    }

    FindClose(hFile);
}

/* takes the null-separated file list and fills out the FILE_LIST */
static HRESULT parse_file_list(FILE_LIST *flList, LPCWSTR szFiles)
{
    LPCWSTR ptr = szFiles;
    WCHAR szCurFile[MAX_PATH];
    DWORD i = 0, dwDirLen;

    if (!szFiles)
        return ERROR_INVALID_PARAMETER;

    flList->bAnyFromWildcard = FALSE;
    flList->bAnyDirectories = FALSE;
    flList->bAnyDontExist = FALSE;
    flList->num_alloc = 32;
    flList->dwNumFiles = 0;

    /* empty list */
    if (!szFiles[0])
        return ERROR_ACCESS_DENIED;

    flList->feFiles = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                                flList->num_alloc * sizeof(FILE_ENTRY));

    while (*ptr)
    {
        if (i >= flList->num_alloc) grow_list( flList );

        /* change relative to absolute path */
        if (PathIsRelativeW(ptr))
        {
            dwDirLen = GetCurrentDirectoryW(MAX_PATH, szCurFile) + 1;
            PathCombineW(szCurFile, szCurFile, ptr);
            flList->feFiles[i].bFromRelative = TRUE;
        }
        else
        {
            lstrcpyW(szCurFile, ptr);
            flList->feFiles[i].bFromRelative = FALSE;
        }

        /* parse wildcard files if they are in the filename */
        if (StrPBrkW(szCurFile, wWildcardChars))
        {
            parse_wildcard_files(flList, szCurFile, &i);
            flList->bAnyFromWildcard = TRUE;
            i--;
        }
        else
        {
            FILE_ENTRY *file = &flList->feFiles[i];
            add_file_to_entry(file, szCurFile);
            file->attributes = GetFileAttributesW( file->szFullPath );
            file->bExists = (file->attributes != INVALID_FILE_ATTRIBUTES);
            if (!file->bExists) flList->bAnyDontExist = TRUE;
            if (IsAttribDir(file->attributes)) flList->bAnyDirectories = TRUE;
        }

        /* advance to the next string */
        ptr += lstrlenW(ptr) + 1;
        i++;
    }
    flList->dwNumFiles = i;

    return S_OK;
}

/* free the FILE_LIST */
static void destroy_file_list(FILE_LIST *flList)
{
    DWORD i;

    if (!flList || !flList->feFiles)
        return;

    for (i = 0; i < flList->dwNumFiles; i++)
    {
        HeapFree(GetProcessHeap(), 0, flList->feFiles[i].szDirectory);
        HeapFree(GetProcessHeap(), 0, flList->feFiles[i].szFilename);
        HeapFree(GetProcessHeap(), 0, flList->feFiles[i].szFullPath);
    }

    HeapFree(GetProcessHeap(), 0, flList->feFiles);
}

static void copy_dir_to_dir(FILE_OPERATION *op, FILE_ENTRY *feFrom, LPWSTR szDestPath)
{
    WCHAR szFrom[MAX_PATH], szTo[MAX_PATH];
    SHFILEOPSTRUCTW fileOp;

    static const WCHAR wildCardFiles[] = {'*','.','*',0};

    if (IsDotDir(feFrom->szFilename))
        return;

    if (PathFileExistsW(szDestPath))
        PathCombineW(szTo, szDestPath, feFrom->szFilename);
    else
        lstrcpyW(szTo, szDestPath);

    if (!(op->req->fFlags & FOF_NOCONFIRMATION) && PathFileExistsW(szTo)) {
        if (!SHELL_ConfirmDialogW(op->req->hwnd, ASK_OVERWRITE_FOLDER, feFrom->szFilename, op))
        {
            /* Vista returns an ERROR_CANCELLED even if user pressed "No" */
            if (!op->bManyItems)
                op->bCancelled = TRUE;
            return;
        }
    }

    szTo[lstrlenW(szTo) + 1] = '\0';
    SHNotifyCreateDirectoryW(szTo, NULL);

    PathCombineW(szFrom, feFrom->szFullPath, wildCardFiles);
    szFrom[lstrlenW(szFrom) + 1] = '\0';

    memcpy(&fileOp, op->req, sizeof(fileOp));
    fileOp.pFrom = szFrom;
    fileOp.pTo = szTo;
    fileOp.fFlags &= ~FOF_MULTIDESTFILES; /* we know we're copying to one dir */

    /* Don't ask the user about overwriting files when he accepted to overwrite the
       folder. FIXME: this is not exactly what Windows does - e.g. there would be
       an additional confirmation for a nested folder */
    fileOp.fFlags |= FOF_NOCONFIRMATION;

    SHFileOperationW(&fileOp);
}

static BOOL copy_file_to_file(FILE_OPERATION *op, WCHAR *szFrom, WCHAR *szTo)
{
    if (!(op->req->fFlags & FOF_NOCONFIRMATION) && PathFileExistsW(szTo))
    {
        if (!SHELL_ConfirmDialogW(op->req->hwnd, ASK_OVERWRITE_FILE, PathFindFileNameW(szTo), op))
            return 0;
    }

    return SHNotifyCopyFileW(szFrom, szTo, FALSE) == 0;
}

/* copy a file or directory to another directory */
static void copy_to_dir(FILE_OPERATION *op, FILE_ENTRY *feFrom, FILE_ENTRY *feTo)
{
    if (!PathFileExistsW(feTo->szFullPath))
        SHNotifyCreateDirectoryW(feTo->szFullPath, NULL);

    if (IsAttribFile(feFrom->attributes))
    {
        WCHAR szDestPath[MAX_PATH];

        PathCombineW(szDestPath, feTo->szFullPath, feFrom->szFilename);
        copy_file_to_file(op, feFrom->szFullPath, szDestPath);
    }
    else if (!(op->req->fFlags & FOF_FILESONLY && feFrom->bFromWildcard))
        copy_dir_to_dir(op, feFrom, feTo->szFullPath);
}

static void create_dest_dirs(LPWSTR szDestDir)
{
    WCHAR dir[MAX_PATH];
    LPWSTR ptr = StrChrW(szDestDir, '\\');

    /* make sure all directories up to last one are created */
    while (ptr && (ptr = StrChrW(ptr + 1, '\\')))
    {
        lstrcpynW(dir, szDestDir, ptr - szDestDir + 1);

        if (!PathFileExistsW(dir))
            SHNotifyCreateDirectoryW(dir, NULL);
    }

    /* create last directory */
    if (!PathFileExistsW(szDestDir))
        SHNotifyCreateDirectoryW(szDestDir, NULL);
}

/* the FO_COPY operation */
static HRESULT copy_files(FILE_OPERATION *op, FILE_LIST *flFrom, FILE_LIST *flTo)
{
    DWORD i;
    FILE_ENTRY *entryToCopy;
    FILE_ENTRY *fileDest = &flTo->feFiles[0];
    BOOL bCancelIfAnyDirectories = FALSE;

    if (flFrom->bAnyDontExist)
        return ERROR_SHELL_INTERNAL_FILE_NOT_FOUND;

    if (op->req->fFlags & FOF_MULTIDESTFILES && flFrom->bAnyFromWildcard)
        return ERROR_CANCELLED;

    if (!(op->req->fFlags & FOF_MULTIDESTFILES) && flTo->dwNumFiles != 1)
        return ERROR_CANCELLED;

    if (op->req->fFlags & FOF_MULTIDESTFILES && flFrom->dwNumFiles != 1 &&
        flFrom->dwNumFiles != flTo->dwNumFiles)
    {
        return ERROR_CANCELLED;
    }

    if (flFrom->dwNumFiles > 1 && flTo->dwNumFiles == 1 &&
        !PathFileExistsW(flTo->feFiles[0].szFullPath) &&
        IsAttribFile(fileDest->attributes))
    {
        bCancelIfAnyDirectories = TRUE;
    }

    if (flFrom->dwNumFiles > 1 && flTo->dwNumFiles == 1 && fileDest->bFromRelative &&
        !PathFileExistsW(fileDest->szFullPath))
    {
        op->req->fAnyOperationsAborted = TRUE;
        return ERROR_CANCELLED;
    }

    if (!(op->req->fFlags & FOF_MULTIDESTFILES) && flFrom->dwNumFiles != 1 &&
        PathFileExistsW(fileDest->szFullPath) &&
        IsAttribFile(fileDest->attributes))
    {
        return ERROR_CANCELLED;
    }

    for (i = 0; i < flFrom->dwNumFiles; i++)
    {
        entryToCopy = &flFrom->feFiles[i];

        if (op->req->fFlags & FOF_MULTIDESTFILES)
            fileDest = &flTo->feFiles[i];

        if (IsAttribDir(entryToCopy->attributes) &&
            !lstrcmpiW(entryToCopy->szFullPath, fileDest->szDirectory))
        {
            return ERROR_SUCCESS;
        }

        if (IsAttribDir(entryToCopy->attributes) && bCancelIfAnyDirectories)
            return ERROR_CANCELLED;

        create_dest_dirs(fileDest->szDirectory);

        if (!lstrcmpiW(entryToCopy->szFullPath, fileDest->szFullPath))
        {
            if (IsAttribFile(entryToCopy->attributes))
                return ERROR_NO_MORE_SEARCH_HANDLES;
            else
                return ERROR_SUCCESS;
        }

        if ((flFrom->dwNumFiles > 1 && flTo->dwNumFiles == 1) ||
            (flFrom->dwNumFiles == 1 && IsAttribDir(fileDest->attributes)))
        {
            copy_to_dir(op, entryToCopy, fileDest);
        }
        else if (IsAttribDir(entryToCopy->attributes))
        {
            copy_dir_to_dir(op, entryToCopy, fileDest->szFullPath);
        }
        else
        {
            if (!copy_file_to_file(op, entryToCopy->szFullPath, fileDest->szFullPath))
            {
                op->req->fAnyOperationsAborted = TRUE;
                return ERROR_CANCELLED;
            }
        }

        /* Vista return code. XP would return e.g. ERROR_FILE_NOT_FOUND, ERROR_ALREADY_EXISTS */
        if (op->bCancelled)
            return ERROR_CANCELLED;
    }

    /* Vista return code. On XP if the used pressed "No" for the last item,
     * ERROR_ARENA_TRASHED would be returned */
    return ERROR_SUCCESS;
}

static BOOL confirm_delete_list(HWND hWnd, DWORD fFlags, BOOL fTrash, FILE_LIST *flFrom)
{
    if (flFrom->dwNumFiles > 1)
    {
        WCHAR tmp[8];
        const WCHAR format[] = {'%','d',0};

        wnsprintfW(tmp, sizeof(tmp)/sizeof(tmp[0]), format, flFrom->dwNumFiles);
        return SHELL_ConfirmDialogW(hWnd, (fTrash?ASK_TRASH_MULTIPLE_ITEM:ASK_DELETE_MULTIPLE_ITEM), tmp, NULL);
    }
    else
    {
        FILE_ENTRY *fileEntry = &flFrom->feFiles[0];

        if (IsAttribFile(fileEntry->attributes))
            return SHELL_ConfirmDialogW(hWnd, (fTrash?ASK_TRASH_FILE:ASK_DELETE_FILE), fileEntry->szFullPath, NULL);
        else if (!(fFlags & FOF_FILESONLY && fileEntry->bFromWildcard))
            return SHELL_ConfirmDialogW(hWnd, (fTrash?ASK_TRASH_FOLDER:ASK_DELETE_FOLDER), fileEntry->szFullPath, NULL);
    }
    return TRUE;
}

/* the FO_DELETE operation */
static HRESULT delete_files(LPSHFILEOPSTRUCTW lpFileOp, FILE_LIST *flFrom)
{
    FILE_ENTRY *fileEntry;
    DWORD i;
    BOOL bPathExists;
    BOOL bTrash;

    if (!flFrom->dwNumFiles)
        return ERROR_SUCCESS;

    /* Windows also checks only the first item */
    bTrash = (lpFileOp->fFlags & FOF_ALLOWUNDO)
        && TRASH_CanTrashFile(flFrom->feFiles[0].szFullPath);

    if (!(lpFileOp->fFlags & FOF_NOCONFIRMATION) || (!bTrash && lpFileOp->fFlags & FOF_WANTNUKEWARNING))
        if (!confirm_delete_list(lpFileOp->hwnd, lpFileOp->fFlags, bTrash, flFrom))
        {
            lpFileOp->fAnyOperationsAborted = TRUE;
            return 0;
        }

    for (i = 0; i < flFrom->dwNumFiles; i++)
    {
        bPathExists = TRUE;
        fileEntry = &flFrom->feFiles[i];

        if (!IsAttribFile(fileEntry->attributes) &&
            (lpFileOp->fFlags & FOF_FILESONLY && fileEntry->bFromWildcard))
            continue;

        if (bTrash)
        {
            BOOL bDelete;
            if (TRASH_TrashFile(fileEntry->szFullPath))
                continue;

            /* Note: Windows silently deletes the file in such a situation, we show a dialog */
            if (!(lpFileOp->fFlags & FOF_NOCONFIRMATION) || (lpFileOp->fFlags & FOF_WANTNUKEWARNING))
                bDelete = SHELL_ConfirmDialogW(lpFileOp->hwnd, ASK_CANT_TRASH_ITEM, fileEntry->szFullPath, NULL);
            else
                bDelete = TRUE;

            if (!bDelete)
            {
                lpFileOp->fAnyOperationsAborted = TRUE;
                break;
            }
        }

        /* delete the file or directory */
        if (IsAttribFile(fileEntry->attributes))
            bPathExists = DeleteFileW(fileEntry->szFullPath);
        else
            bPathExists = SHELL_DeleteDirectoryW(lpFileOp->hwnd, fileEntry->szFullPath, FALSE);

        if (!bPathExists)
            return ERROR_PATH_NOT_FOUND;
    }

    return ERROR_SUCCESS;
}

static void move_dir_to_dir(LPSHFILEOPSTRUCTW lpFileOp, FILE_ENTRY *feFrom, LPWSTR szDestPath)
{
    WCHAR szFrom[MAX_PATH], szTo[MAX_PATH];
    SHFILEOPSTRUCTW fileOp;

    static const WCHAR wildCardFiles[] = {'*','.','*',0};

    if (IsDotDir(feFrom->szFilename))
        return;

    SHNotifyCreateDirectoryW(szDestPath, NULL);

    PathCombineW(szFrom, feFrom->szFullPath, wildCardFiles);
    szFrom[lstrlenW(szFrom) + 1] = '\0';

    lstrcpyW(szTo, szDestPath);
    szTo[lstrlenW(szDestPath) + 1] = '\0';

    memcpy(&fileOp, lpFileOp, sizeof(fileOp));
    fileOp.pFrom = szFrom;
    fileOp.pTo = szTo;

    SHFileOperationW(&fileOp);
}

/* moves a file or directory to another directory */
static void move_to_dir(LPSHFILEOPSTRUCTW lpFileOp, FILE_ENTRY *feFrom, FILE_ENTRY *feTo)
{
    WCHAR szDestPath[MAX_PATH];

    PathCombineW(szDestPath, feTo->szFullPath, feFrom->szFilename);

    if (IsAttribFile(feFrom->attributes))
        SHNotifyMoveFileW(feFrom->szFullPath, szDestPath);
    else if (!(lpFileOp->fFlags & FOF_FILESONLY && feFrom->bFromWildcard))
        move_dir_to_dir(lpFileOp, feFrom, szDestPath);
}

/* the FO_MOVE operation */
static HRESULT move_files(LPSHFILEOPSTRUCTW lpFileOp, FILE_LIST *flFrom, FILE_LIST *flTo)
{
    DWORD i;
    FILE_ENTRY *entryToMove;
    FILE_ENTRY *fileDest;

    if (!flFrom->dwNumFiles || !flTo->dwNumFiles)
        return ERROR_CANCELLED;

    if (!(lpFileOp->fFlags & FOF_MULTIDESTFILES) &&
        flTo->dwNumFiles > 1 && flFrom->dwNumFiles > 1)
    {
        return ERROR_CANCELLED;
    }

    if (!(lpFileOp->fFlags & FOF_MULTIDESTFILES) &&
        !flFrom->bAnyDirectories &&
        flFrom->dwNumFiles > flTo->dwNumFiles)
    {
        return ERROR_CANCELLED;
    }

    if (!PathFileExistsW(flTo->feFiles[0].szDirectory))
        return ERROR_CANCELLED;

    if ((lpFileOp->fFlags & FOF_MULTIDESTFILES) &&
        flFrom->dwNumFiles != flTo->dwNumFiles)
    {
        return ERROR_CANCELLED;
    }

    fileDest = &flTo->feFiles[0];
    for (i = 0; i < flFrom->dwNumFiles; i++)
    {
        entryToMove = &flFrom->feFiles[i];

        if (lpFileOp->fFlags & FOF_MULTIDESTFILES)
            fileDest = &flTo->feFiles[i];

        if (!PathFileExistsW(fileDest->szDirectory))
            return ERROR_CANCELLED;

        if (fileDest->bExists && IsAttribDir(fileDest->attributes))
            move_to_dir(lpFileOp, entryToMove, fileDest);
        else
            SHNotifyMoveFileW(entryToMove->szFullPath, fileDest->szFullPath);
    }

    return ERROR_SUCCESS;
}

/* the FO_RENAME files */
static HRESULT rename_files(LPSHFILEOPSTRUCTW lpFileOp, FILE_LIST *flFrom, FILE_LIST *flTo)
{
    FILE_ENTRY *feFrom;
    FILE_ENTRY *feTo;

    if (flFrom->dwNumFiles != 1)
        return ERROR_GEN_FAILURE;

    if (flTo->dwNumFiles != 1)
        return ERROR_CANCELLED;

    feFrom = &flFrom->feFiles[0];
    feTo= &flTo->feFiles[0];

    /* fail if destination doesn't exist */
    if (!feFrom->bExists)
        return ERROR_SHELL_INTERNAL_FILE_NOT_FOUND;

    /* fail if destination already exists */
    if (feTo->bExists)
        return ERROR_ALREADY_EXISTS;

    return SHNotifyMoveFileW(feFrom->szFullPath, feTo->szFullPath);
}

/* alert the user if an unsupported flag is used */
static void check_flags(FILEOP_FLAGS fFlags)
{
    WORD wUnsupportedFlags = FOF_NO_CONNECTED_ELEMENTS |
        FOF_NOCOPYSECURITYATTRIBS | FOF_NORECURSEREPARSE |
        FOF_RENAMEONCOLLISION | FOF_WANTMAPPINGHANDLE;

    if (fFlags & wUnsupportedFlags)
        FIXME("Unsupported flags: %04x\n", fFlags);
}

/*************************************************************************
 * SHFileOperationW          [SHELL32.@]
 *
 * See SHFileOperationA
 */
int WINAPI SHFileOperationW(LPSHFILEOPSTRUCTW lpFileOp)
{
    FILE_OPERATION op;
    FILE_LIST flFrom, flTo;
    int ret = 0;

    if (!lpFileOp)
        return ERROR_INVALID_PARAMETER;

    check_flags(lpFileOp->fFlags);

    ZeroMemory(&flFrom, sizeof(FILE_LIST));
    ZeroMemory(&flTo, sizeof(FILE_LIST));

    if ((ret = parse_file_list(&flFrom, lpFileOp->pFrom)))
        return ret;

    if (lpFileOp->wFunc != FO_DELETE)
        parse_file_list(&flTo, lpFileOp->pTo);

    ZeroMemory(&op, sizeof(op));
    op.req = lpFileOp;
    op.bManyItems = (flFrom.dwNumFiles > 1);

    switch (lpFileOp->wFunc)
    {
        case FO_COPY:
            ret = copy_files(&op, &flFrom, &flTo);
            break;
        case FO_DELETE:
            ret = delete_files(lpFileOp, &flFrom);
            break;
        case FO_MOVE:
            ret = move_files(lpFileOp, &flFrom, &flTo);
            break;
        case FO_RENAME:
            ret = rename_files(lpFileOp, &flFrom, &flTo);
            break;
        default:
            ret = ERROR_INVALID_PARAMETER;
            break;
    }

    destroy_file_list(&flFrom);

    if (lpFileOp->wFunc != FO_DELETE)
        destroy_file_list(&flTo);

    if (ret == ERROR_CANCELLED)
        lpFileOp->fAnyOperationsAborted = TRUE;

    return ret;
}

#define SHDSA_GetItemCount(hdsa) (*(int*)(hdsa))

/*************************************************************************
 * SHFreeNameMappings      [shell32.246]
 *
 * Free the mapping handle returned by SHFileoperation if FOF_WANTSMAPPINGHANDLE
 * was specified.
 *
 * PARAMS
 *  hNameMapping [I] handle to the name mappings used during renaming of files
 *
 * RETURNS
 *  Nothing
 */
void WINAPI SHFreeNameMappings(HANDLE hNameMapping)
{
	if (hNameMapping)
	{
	  int i = SHDSA_GetItemCount((HDSA)hNameMapping) - 1;

	  for (; i>= 0; i--)
	  {
	    LPSHNAMEMAPPINGW lp = DSA_GetItemPtr((HDSA)hNameMapping, i);

	    SHFree(lp->pszOldPath);
	    SHFree(lp->pszNewPath);
	  }
	  DSA_Destroy((HDSA)hNameMapping);
	}
}

/*************************************************************************
 * SheGetDirA [SHELL32.@]
 *
 * drive = 0: returns the current directory path
 * drive > 0: returns the current directory path of the specified drive
 *            drive=1 -> A:  drive=2 -> B:  ...
 * returns 0 if successful
*/
DWORD WINAPI SheGetDirA(DWORD drive, LPSTR buffer)
{
    WCHAR org_path[MAX_PATH];
    DWORD ret;
    char drv_path[3];

    /* change current directory to the specified drive */
    if (drive) {
        strcpy(drv_path, "A:");
        drv_path[0] += (char)drive-1;

        GetCurrentDirectoryW(MAX_PATH, org_path);

        SetCurrentDirectoryA(drv_path);
    }

    /* query current directory path of the specified drive */
    ret = GetCurrentDirectoryA(MAX_PATH, buffer);

    /* back to the original drive */
    if (drive)
        SetCurrentDirectoryW(org_path);

    if (!ret)
        return GetLastError();

    return 0;
}

/*************************************************************************
 * SheGetDirW [SHELL32.@]
 *
 * drive = 0: returns the current directory path
 * drive > 0: returns the current directory path of the specified drive
 *            drive=1 -> A:  drive=2 -> B:  ...
 * returns 0 if successful
 */
DWORD WINAPI SheGetDirW(DWORD drive, LPWSTR buffer)
{
    WCHAR org_path[MAX_PATH];
    DWORD ret;
    char drv_path[3];

    /* change current directory to the specified drive */
    if (drive) {
        strcpy(drv_path, "A:");
        drv_path[0] += (char)drive-1;

        GetCurrentDirectoryW(MAX_PATH, org_path);

        SetCurrentDirectoryA(drv_path);
    }

    /* query current directory path of the specified drive */
    ret = GetCurrentDirectoryW(MAX_PATH, buffer);

    /* back to the original drive */
    if (drive)
        SetCurrentDirectoryW(org_path);

    if (!ret)
        return GetLastError();

    return 0;
}

/*************************************************************************
 * SheChangeDirA [SHELL32.@]
 *
 * changes the current directory to the specified path
 * and returns 0 if successful
 */
DWORD WINAPI SheChangeDirA(LPSTR path)
{
    if (SetCurrentDirectoryA(path))
        return 0;
    else
        return GetLastError();
}

/*************************************************************************
 * SheChangeDirW [SHELL32.@]
 *
 * changes the current directory to the specified path
 * and returns 0 if successful
 */
DWORD WINAPI SheChangeDirW(LPWSTR path)
{
    if (SetCurrentDirectoryW(path))
        return 0;
    else
        return GetLastError();
}

/*************************************************************************
 * IsNetDrive			[SHELL32.66]
 */
BOOL WINAPI IsNetDrive(DWORD drive)
{
	char root[4];
	strcpy(root, "A:\\");
	root[0] += (char)drive;
	return (GetDriveTypeA(root) == DRIVE_REMOTE);
}


/*************************************************************************
 * RealDriveType                [SHELL32.524]
 */
INT WINAPI RealDriveType(INT drive, BOOL bQueryNet)
{
    char root[] = "A:\\";
    root[0] += (char)drive;
    return GetDriveTypeA(root);
}

/***********************************************************************
 *              SHPathPrepareForWriteW (SHELL32.@)
 */
HRESULT WINAPI SHPathPrepareForWriteW(HWND hwnd, IUnknown *modless, LPCWSTR path, DWORD flags)
{
    HRESULT res;
    DWORD err;
    LPCWSTR realpath;
    int len;
    WCHAR* last_slash;
    WCHAR* temppath=NULL;

    TRACE("%p %p %s 0x%80x\n", hwnd, modless, debugstr_w(path), flags);

    if (flags & ~(SHPPFW_DIRCREATE|SHPPFW_ASKDIRCREATE|SHPPFW_IGNOREFILENAME))
        FIXME("unimplemented flags 0x%08x\n", flags);

    /* cut off filename if necessary */
    if (flags & SHPPFW_IGNOREFILENAME)
    {
        last_slash = StrRChrW(path, NULL, '\\');
        if (last_slash == NULL)
            len = 1;
        else
            len = last_slash - path + 1;
        temppath = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        if (!temppath)
            return E_OUTOFMEMORY;
        StrCpyNW(temppath, path, len);
        realpath = temppath;
    }
    else
    {
        realpath = path;
    }

    /* try to create the directory if asked to */
    if (flags & (SHPPFW_DIRCREATE|SHPPFW_ASKDIRCREATE))
    {
        if (flags & SHPPFW_ASKDIRCREATE)
            FIXME("treating SHPPFW_ASKDIRCREATE as SHPPFW_DIRCREATE\n");

        SHCreateDirectoryExW(0, realpath, NULL);
    }

    /* check if we can access the directory */
    res = GetFileAttributesW(realpath);

    HeapFree(GetProcessHeap(), 0, temppath);

    if (res == INVALID_FILE_ATTRIBUTES)
    {
        err = GetLastError();
        if (err == ERROR_FILE_NOT_FOUND)
            return HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND);
        return HRESULT_FROM_WIN32(err);
    }
    else if (res & FILE_ATTRIBUTE_DIRECTORY)
        return S_OK;
    else
        return HRESULT_FROM_WIN32(ERROR_DIRECTORY);
}

/***********************************************************************
 *              SHPathPrepareForWriteA (SHELL32.@)
 */
HRESULT WINAPI SHPathPrepareForWriteA(HWND hwnd, IUnknown *modless, LPCSTR path, DWORD flags)
{
    WCHAR wpath[MAX_PATH];
    MultiByteToWideChar( CP_ACP, 0, path, -1, wpath, MAX_PATH);
    return SHPathPrepareForWriteW(hwnd, modless, wpath, flags);
}
